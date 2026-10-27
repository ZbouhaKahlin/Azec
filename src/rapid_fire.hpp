#pragma once
#include <algorithm>
#include <cstdint>

namespace r6 {
enum class RapidMode {DoubleClick=0,Hold=1};
// A physical press already supplies the first click. The controller produces
// the extra click(s), independent of the global state changed by those events.
class RapidSequence {
    enum class Phase {Idle,ExtraDown,ExtraUp,Delay};
    Phase phase_=Phase::Idle;
    std::uint64_t seen_=0,pending_=0;
    bool initialized_=false,synthetic_down_=false;
    int profile_=-1;
    RapidMode mode_=RapidMode::DoubleClick;
    std::int64_t due_=0;
public:
    // -1 means no stroke; 0/1 mean left-button up/down.
    int step(std::int64_t now,bool physicalDown,std::uint64_t presses,bool allowed,RapidMode mode,int profile,int delay){
        if(!initialized_){initialized_=true;seen_=presses;mode_=mode;profile_=profile;return -1;}
        if(!allowed||profile_!=profile||mode_!=mode){
            const bool release=synthetic_down_;phase_=Phase::Idle;synthetic_down_=false;pending_=0;
            seen_=presses;profile_=profile;mode_=mode;return release?0:-1;
        }
        if(presses>seen_){pending_+=presses-seen_;seen_=presses;}
        if(mode==RapidMode::Hold&&!physicalDown){
            const bool release=synthetic_down_;phase_=Phase::Idle;synthetic_down_=false;pending_=0;return release?0:-1;
        }
        if(phase_==Phase::ExtraDown&&now>=due_){phase_=Phase::ExtraUp;due_=now+25;synthetic_down_=true;return 1;}
        if(phase_==Phase::ExtraUp&&now>=due_){
            synthetic_down_=false;phase_=mode==RapidMode::Hold?Phase::Delay:Phase::Idle;
            due_=now+std::clamp(delay,30,150);return 0;
        }
        if(phase_==Phase::Delay&&now>=due_)phase_=Phase::Idle;
        if(phase_==Phase::Idle&&pending_){
            if(mode==RapidMode::Hold)pending_=1;
            --pending_;phase_=Phase::ExtraDown;due_=now+5;return 0;
        }
        if(phase_==Phase::Idle&&mode==RapidMode::Hold&&physicalDown){
            phase_=Phase::ExtraDown;due_=now+5;return 0;
        }
        return -1;
    }
};
}
