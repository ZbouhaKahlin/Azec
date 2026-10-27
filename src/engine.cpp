#include <windows.h>
#include "engine.hpp"
#include "profile_store.hpp"
#include "diagnostics.hpp"
#include "driver_input.hpp"
#include <chrono>
#include <cmath>
#include <random>

namespace r6 {
Engine::Engine(bool dry_run):dry_run_(dry_run) {
    // Fixed compatibility accent for the unchanged binary layout; the visible look is fixed in theme.hpp.
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
bool Engine::load(const std::filesystem::path& file,bool* weaponProfilesValid){
    std::lock_guard lock(config_mutex_);
    if(!config_.load(file))return false;
    // The accent and the background response radius are fixed materials now;
    // old custom values are normalized on load and on the next save.
    config_.neon[0]=114.0f/255;config_.neon[1]=72.0f/255;config_.neon[2]=215.0f/255;config_.radius=130;
    bool focusOnly=true,adsOnly=false;
    const bool valid=readWeaponProfiles(config_,focusOnly,adsOnly,file);
    if(weaponProfilesValid)*weaponProfilesValid=valid;
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
void Engine::start(){if(running.exchange(true))return;if(!dry_run_&&!physical_mouse_.start())diagnostic("INPUT-PHYSICAL-START","Physical mouse input could not be registered; rapid fire remains disabled");recoil_=std::thread(&Engine::recoil_worker,this);rapid_=std::thread(&Engine::rapid_worker,this);}
void Engine::stop(){
    {std::lock_guard lock(stop_mutex_);running=false;}
    stop_condition_.notify_all();
    if(recoil_.joinable())recoil_.join();if(rapid_.joinable())rapid_.join();physical_mouse_.stop();
}
void Engine::sleep(int ms){std::unique_lock lock(stop_mutex_);stop_condition_.wait_for(lock,std::chrono::milliseconds(ms),[this]{return !running.load();});}
void Engine::input(unsigned long flags,int x,int y){
    if(dry_run_)return;
    // The Interception driver is the only output: it takes relative moves and
    // left-button strokes at the device level, below Windows' injected flag.
    const bool sent=flags==MOUSEEVENTF_MOVE?driver_input::move(x,y)
                   :flags==MOUSEEVENTF_LEFTDOWN?driver_input::button(true)
                   :flags==MOUSEEVENTF_LEFTUP?driver_input::button(false)
                   :false;
    if(sent)++injected_events;
}
std::pair<Operator,int> Engine::current_profile() const {std::lock_guard lock(config_mutex_);return {config_.active_operator(),config_.selected*2+config_.active_slot()};}
RapidSettings Engine::rapid_settings() const {std::lock_guard lock(config_mutex_);return {config_.active_operator(),config_.rapid_mode,config_.selected*2+config_.active_slot()};}
void Engine::recoil_worker(){
    std::mt19937 random(std::random_device{}());std::uniform_int_distribution<int> jitter(-1,1);
    RecoilAccumulator accumulator;int previous_profile=-1;
    while(running){
        const auto [op,profile]=current_profile();if(profile!=previous_profile){accumulator.reset();previous_profile=profile;}
        if(!recoil_active||!input_allowed||menu_visible){accumulator.reset();sleep(8);continue;}
        // ADS gate: aiming down sights means physically holding the right button.
        if(ads_only&&!(GetAsyncKeyState(VK_RBUTTON)&0x8000)){accumulator.reset();sleep(8);continue;}
        if(!physical_mouse_.snapshot().down){accumulator.reset();sleep(4);continue;}
        const auto movement=accumulator.step(op,op.humanize?jitter(random):0);
        input(MOUSEEVENTF_MOVE,movement.x,movement.y);sleep(recoil_interval(op.rapid!=0));
    }
}
void Engine::rapid_worker(){
    RapidSequence sequence;HWND burstWindow=nullptr,lastForeground=nullptr;
    const auto clock=[](){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();};
    while(running){
        const auto [op,mode,profile]=rapid_settings();
        const HWND foreground=GetForegroundWindow();
        // A release on another/secure desktop may not reach Raw Input. Require
        // a fresh physical press after an output gate or foreground transition.
        if(!input_allowed||menu_visible||!foreground||(lastForeground&&foreground!=lastForeground))physical_mouse_.releaseAll();
        lastForeground=foreground;
        const auto physical=physical_mouse_.snapshot();
        const bool enabled=physical_mouse_.ready()&&op.rapid&&input_allowed&&!menu_visible&&
            (!ads_only||(GetAsyncKeyState(VK_RBUTTON)&0x8000));
        if(enabled&&!burstWindow)burstWindow=foreground;
        const bool allowed=enabled&&foreground&&foreground==burstWindow;
        const int stroke=sequence.step(clock(),physical.down,physical.presses,allowed,mode,profile,op.delay);
        // Recheck the live physical state immediately before a generated down.
        // No queued down survives a Hold release, permission loss or focus change.
        if(stroke>=0&&burstWindow&&foreground==burstWindow&&input_allowed&&!menu_visible&&GetForegroundWindow()==foreground&&
           (stroke==0||allowed)&&(stroke==0||mode!=RapidMode::Hold||physical_mouse_.snapshot().down))
            input(stroke?MOUSEEVENTF_LEFTDOWN:MOUSEEVENTF_LEFTUP);
        if(!allowed)burstWindow=nullptr;
        sleep(2);
    }
    const auto physical=physical_mouse_.snapshot();
    const int release=sequence.step(clock(),physical.down,physical.presses,false,RapidMode::DoubleClick,-1,50);
    if(release==0&&burstWindow&&GetForegroundWindow()==burstWindow&&input_allowed&&!menu_visible)input(MOUSEEVENTF_LEFTUP);
}
}
