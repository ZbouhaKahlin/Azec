#include <windows.h>
#include "engine.hpp"
#include "profile_store.hpp"
#include "diagnostics.hpp"
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
void Engine::edit(const std::function<void(Config&)>& change){std::lock_guard lock(config_mutex_);change(config_);}
bool Engine::select_weapon(int slot){std::lock_guard lock(config_mutex_);if(slot<0||slot>1||(slot==1&&!config_.secondary_enabled[config_.selected]))return false;config_.weapon_slot=slot;return true;}
bool Engine::load(const std::filesystem::path& file){
    std::lock_guard lock(config_mutex_);
    if(!config_.load(file))return false;
    // The accent and the background response radius are fixed materials now;
    // old custom values are normalized on load and on the next save.
    config_.neon[0]=114.0f/255;config_.neon[1]=72.0f/255;config_.neon[2]=215.0f/255;config_.radius=130;
    bool master=recoil_master.load();
    readWeaponProfiles(config_,master,file);
    recoil_master=master;
    return true;
}
bool Engine::save(const std::filesystem::path& file) const {
    const auto config=snapshot();
    // Prepare the JSON sidecar first; if the binary write fails, the sidecar is
    // discarded so the two files never disagree about the saved state.
    QSaveFile sidecar;
    if(!stageWeaponProfiles(config,recoil_master.load(),file,sidecar)){diagnostic("save weapon profiles",sidecar.errorString());return false;}
    if(!config.save(file)){
        sidecar.cancelWriting();
        diagnostic("save binary","Atomic replacement failed (Windows error "+QString::number(GetLastError())+")");return false;
    }
    if(!sidecar.commit()){diagnostic("save weapon profiles",sidecar.errorString());return false;}
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
    if(dry_run_)return;
    INPUT event{};event.type=INPUT_MOUSE;event.mi.dwFlags=flags;event.mi.dx=x;event.mi.dy=y;
    SendInput(1,&event,sizeof(event));++injected_events;
}
std::pair<Operator,int> Engine::current_profile() const {std::lock_guard lock(config_mutex_);return {config_.active_operator(),config_.selected*2+config_.active_slot()};}
void Engine::recoil_worker(){
    std::mt19937 random(std::random_device{}());std::uniform_int_distribution<int> jitter(-1,1);
    RecoilAccumulator accumulator;int previous_profile=-1;
    while(running){
        const auto [op,profile]=current_profile();if(profile!=previous_profile){accumulator.reset();previous_profile=profile;}
        if(!recoil_active||!recoil_master||menu_visible){accumulator.reset();sleep(8);continue;}
        if(!(GetAsyncKeyState(VK_LBUTTON)&0x8000)){accumulator.reset();sleep(4);continue;}
        const auto movement=accumulator.step(op,op.humanize?jitter(random):0);
        input(MOUSEEVENTF_MOVE,movement.x,movement.y);sleep(recoil_interval(op));
    }
}
void Engine::rapid_worker(){
    bool enabled=false;
    while(running){
        const auto op=current_profile().first;
        if(!op.rapid){
            // Rapid fire stopped mid-burst: hand the trigger back to the player
            // instead of leaving our injected release as the last button event.
            if(enabled){enabled=false;if(GetAsyncKeyState(VK_LBUTTON)&0x8000)input(MOUSEEVENTF_LEFTDOWN);}
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
