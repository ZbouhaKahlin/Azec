#include <windows.h>
#include "engine.hpp"
#include "profile_store.hpp"
#include <chrono>
#include <cmath>
#include <random>

namespace r6 {
Engine::Engine(bool dry_run):dry_run_(dry_run) {
    // Screenshot palette is the new default; persisted binary layout is unchanged.
    config_.neon[0]=1;config_.neon[1]=91.0f/255;config_.neon[2]=99.0f/255;
    if(!dry_run_)for(const char* path:{"logitechg_discord.dll","C:\\Program Files\\LGHUB\\logitechg_discord.dll","C:\\Program Files (x86)\\LGHUB\\logitechg_discord.dll"}) {
        logitech_=LoadLibraryA(path);
        if(logitech_){(void)GetProcAddress(static_cast<HMODULE>(logitech_),"LogiLedSetTargetDevice");break;}
    }
}
Engine::~Engine(){stop();if(logitech_)FreeLibrary(static_cast<HMODULE>(logitech_));}
Config Engine::snapshot() const {std::lock_guard lock(config_mutex_);return config_;}
void Engine::edit(const std::function<void(Config&)>& change){std::lock_guard lock(config_mutex_);change(config_);}
void Engine::set_rapid_master(bool enabled){rapid_master=enabled;edit([=](Config& c){c.rapid_master=enabled;});}
bool Engine::select_weapon(int slot){std::lock_guard lock(config_mutex_);if(slot<0||slot>1||(slot==1&&!config_.secondary_enabled[config_.selected]))return false;config_.weapon_slot=slot;return true;}
bool Engine::load(const std::filesystem::path& file){
    std::lock_guard lock(config_mutex_);
    if(!config_.load(file))return false;
    // Update only the two former defaults; keep deliberately chosen custom colors.
    const bool legacyGreen=config_.neon[0]==0&&config_.neon[1]==1&&std::abs(config_.neon[2]-.4f)<.00001f;
    const bool legacyTeal=std::abs(config_.neon[0]-5.f/255)<.00001f&&std::abs(config_.neon[1]-134.f/255)<.00001f&&std::abs(config_.neon[2]-105.f/255)<.00001f;
    if(legacyGreen||legacyTeal){
        config_.neon[0]=1;config_.neon[1]=91.0f/255;config_.neon[2]=99.0f/255;
    }
    readWeaponProfiles(config_,file);rapid_master=bool(config_.rapid_master);return true;
}
bool Engine::save(const std::filesystem::path& file) const {const auto config=snapshot();return writeWeaponProfiles(config,file)&&config.save(file);}
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
        if(!recoil_active||!recoil_master||!(GetAsyncKeyState(VK_LBUTTON)&0x8000)){accumulator.reset();sleep(1);continue;}
        const auto movement=accumulator.step(op,op.humanize?jitter(random):0);
        input(MOUSEEVENTF_MOVE,movement.x,movement.y);sleep(recoil_interval(op,rapid_master));
    }
}
void Engine::rapid_worker(){
    while(running){
        const auto op=current_profile().first;
        if(!rapid_master||!op.rapid){sleep(50);continue;}
        if((GetAsyncKeyState(VK_LBUTTON)&0x8000)&&!menu_visible){
            input(MOUSEEVENTF_LEFTUP);sleep(5);if(!running)break;
            input(MOUSEEVENTF_LEFTDOWN);sleep(25);input(MOUSEEVENTF_LEFTUP);sleep(rapid_interval(op));
        }else sleep(1);
    }
}
}
