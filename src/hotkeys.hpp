#pragma once
#include "core.hpp"
#include <array>

inline std::array<int,6> hotkeys(const r6::Config& config){
    return {config.menu_key,config.recoil_key,config.rapid_key,config.panic_key,config.primary_key,config.secondary_key};
}
inline bool uniqueHotkeys(const r6::Config& config){
    const auto keys=hotkeys(config);
    for(size_t i=0;i<keys.size();++i)for(size_t j=i+1;j<keys.size();++j)if(keys[i]&&keys[i]==keys[j])return false;
    return true;
}
inline int hotkeyConflict(const r6::Config& config,int index,int key){
    const auto keys=hotkeys(config);
    for(int i=0;i<int(keys.size());++i)if(i!=index&&key&&keys[i]==key)return i;
    return -1;
}
