#include <windows.h>
#include "engine.hpp"
#include "profile_store.hpp"
#include "diagnostics.hpp"
#include "driver_input.hpp"
#include "game_focus.hpp"
#include <chrono>
#include <cmath>
#include <random>

namespace r6 {
Engine::Engine(bool dry_run):dry_run_(dry_run) {
    // GoofyUI purple is the current default; persisted binary layout is unchanged.
    config_.neon[0]=114.0f/255;config_.neon[1]=72.0f/255;config_.neon[2]=215.0f/255;
}
Engine::~Engine(){stop();}
Config Engine::snapshot() const {std::lock_guard lock(config_mutex_);return config_;}
Hotkeys Engine::hotkeys() const {
    std::lock_guard lock(config_mutex_);
    return {int(config_.menu_key),int(config_.recoil_key),int(config_.panic_key),
            int(config_.primary_key),int(config_.secondary_key)};
}
void Engine::edit(const std::function<void(Config&)>& change){std::lock_guard lock(config_mutex_);change(config_);}
bool Engine::select_weapon(int slot){std::lock_guard lock(config_mutex_);if(slot<0||slot>1||(slot==1&&!config_.secondary_enabled[config_.selected]))return false;config_.weapon_slot=slot;return true;}
bool Engine::rapid_active() const {std::lock_guard lock(config_mutex_);return config_.active_operator().rapid!=0;}
void Engine::set_rapid_active(bool on){std::lock_guard lock(config_mutex_);config_.active_operator().rapid=on?1:0;}
bool Engine::load(const std::filesystem::path& file){
    std::lock_guard lock(config_mutex_);
    if(!config_.load(file))return false;
    // The accent and the background response radius are fixed materials now;
    // old custom values are normalized on load and on the next save.
    config_.neon[0]=114.0f/255;config_.neon[1]=72.0f/255;config_.neon[2]=215.0f/255;config_.radius=130;
    bool focusOnly=true,adsOnly=false;
    readWeaponProfiles(config_,focusOnly,adsOnly,file);
    game_focus_only=focusOnly;
    ads_only=adsOnly;
    return true;
}
bool Engine::save(const std::filesystem::path& file,const QByteArray& uiPreferences) const {
    const auto config=snapshot();
    const auto binaryPath=QString::fromStdWString(file.wstring());
    const auto sidecarPath=weaponFile(file);
    struct Previous {QString path;bool existed=false;QByteArray bytes;};
    Previous binary{binaryPath},sidecar{sidecarPath};
    const auto capture=[](Previous& previous){
        QFile source(previous.path);previous.existed=source.exists();
        if(!previous.existed)return true;
        if(!source.open(QIODevice::ReadOnly)||source.size()>1024*1024)return false;
        previous.bytes=source.readAll();return source.error()==QFile::NoError;
    };
    if(!capture(binary)||(!uiPreferences.isEmpty()&&!capture(sidecar))){
        diagnostic("save configuration","Could not preserve existing files for rollback");return false;
    }
    const auto restore=[](const Previous& previous){
        if(!previous.existed)return !QFileInfo::exists(previous.path)||QFile::remove(previous.path);
        QSaveFile target(previous.path);
        return target.open(QIODevice::WriteOnly)&&target.write(previous.bytes)==previous.bytes.size()&&target.commit();
    };
    QSaveFile ui;
    if(!uiPreferences.isEmpty()){
        ui.setFileName(QString::fromStdWString((file.parent_path()/L"azec_ui.json").wstring()));
        if(!ui.open(QIODevice::WriteOnly)||ui.write(uiPreferences)!=uiPreferences.size()){
            diagnostic("save imported preferences",ui.errorString());return false;
        }
    }
    QSaveFile profiles;
    if(!stageWeaponProfiles(config,game_focus_only.load(),ads_only.load(),file,profiles)){
        diagnostic("save weapon profiles",profiles.errorString());return false;
    }
    if(!config.save(file)){
        diagnostic("save binary","Atomic replacement failed (Windows error "+QString::number(GetLastError())+")");return false;
    }
    if(!profiles.commit()){
        const bool restored=restore(binary);
        diagnostic("save weapon profiles",profiles.errorString()+(restored?"; previous binary restored":"; rollback failed"));return false;
    }
    if(!uiPreferences.isEmpty()&&!ui.commit()){
        const bool binaryRestored=restore(binary),profilesRestored=restore(sidecar);
        diagnostic("save imported preferences",ui.errorString()+(binaryRestored&&profilesRestored?"; previous profiles restored":"; rollback failed"));return false;
    }
    return true;
}
void Engine::start(){if(running.exchange(true))return;recoil_=std::thread(&Engine::recoil_worker,this);rapid_=std::thread(&Engine::rapid_worker,this);}
void Engine::stop(){
    {std::lock_guard lock(stop_mutex_);running=false;}
    stop_condition_.notify_all();
    if(recoil_.joinable())recoil_.join();if(rapid_.joinable())rapid_.join();
}
void Engine::sleep(int ms){std::unique_lock lock(stop_mutex_);stop_condition_.wait_for(lock,std::chrono::milliseconds(ms),[this]{return !running.load();});}
void Engine::input(unsigned long flags,int x,int y){
    if(dry_run_||(game_focus_only&&!gameIsActive()))return;
    // The Interception driver is the only output: it takes relative moves and
    // left-button strokes at the device level, below Windows' injected flag.
    const bool sent=flags==MOUSEEVENTF_MOVE?driver_input::move(x,y)
                   :flags==MOUSEEVENTF_LEFTDOWN?driver_input::button(true)
                   :flags==MOUSEEVENTF_LEFTUP?driver_input::button(false)
                   :false;
    if(sent)++injected_events;
}
std::pair<Operator,int> Engine::current_profile() const {std::lock_guard lock(config_mutex_);return {config_.active_operator(),config_.selected*2+config_.active_slot()};}
void Engine::recoil_worker(){
    std::mt19937 random(std::random_device{}());std::uniform_int_distribution<int> jitter(-1,1);
    RecoilAccumulator accumulator;int previous_profile=-1;
    while(running){
        const auto [op,profile]=current_profile();if(profile!=previous_profile){accumulator.reset();previous_profile=profile;}
        if(!recoil_active||!input_allowed||menu_visible){accumulator.reset();sleep(8);continue;}
        // ADS gate: aiming down sights means physically holding the right button.
        if(ads_only&&!(GetAsyncKeyState(VK_RBUTTON)&0x8000)){accumulator.reset();sleep(8);continue;}
        if(!(GetAsyncKeyState(VK_LBUTTON)&0x8000)){accumulator.reset();sleep(4);continue;}
        const auto movement=accumulator.step(op,op.humanize?jitter(random):0);
        input(MOUSEEVENTF_MOVE,movement.x,movement.y);sleep(recoil_interval(op.rapid!=0));
    }
}
void Engine::rapid_worker(){
    bool enabled=false;
    while(running){
        const auto op=current_profile().first;
        if(!op.rapid||!input_allowed||(ads_only&&!(GetAsyncKeyState(VK_RBUTTON)&0x8000))){
            // Rapid fire stopped mid-burst: hand the trigger back to the player
            // instead of leaving our injected release as the last button event.
            // Never inject the hand-back into another application.
            if(enabled){enabled=false;if(input_allowed.load()&&(GetAsyncKeyState(VK_LBUTTON)&0x8000))input(MOUSEEVENTF_LEFTDOWN);}
            sleep(50);continue;
        }
        enabled=true;
        if((GetAsyncKeyState(VK_LBUTTON)&0x8000)&&!menu_visible){
            input(MOUSEEVENTF_LEFTUP);sleep(5);if(!running)break;
            input(MOUSEEVENTF_LEFTDOWN);sleep(25);input(MOUSEEVENTF_LEFTUP);sleep(rapid_interval(op));
        }else sleep(4);
    }
}
}
