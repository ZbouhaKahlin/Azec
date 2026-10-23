#pragma once
#include <windows.h>
#include <mutex>
#include <string>

// Optional Interception backend (github.com/oblitum/Interception).
//
// The library is loaded at runtime and never linked or shipped with Azec: the
// signed kernel driver stays the user's own installation, and `interception.dll`
// must sit next to Azec.exe or in the Windows directory. Strokes produced here
// enter the mouse device stack below the "injected input" flag that the game
// now filters, while SendInput and G HUB scripting both carry that flag.
//
// Failure is silent by design: callers check available() once and the engine
// simply emits nothing when the driver is missing.
namespace driver_input {
// InterceptionMouseStroke from the Interception headers. Flags and state follow
// the MOUSE_INPUT_DATA layout the driver forwards.
struct MouseStroke {
    unsigned short state;
    unsigned short flags;
    short rolling;
    int x;
    int y;
    unsigned int information;
};
static_assert(sizeof(MouseStroke)==20,"Interception mouse stroke ABI");

inline constexpr unsigned short move_relative=0x000;
inline constexpr unsigned short left_button_down=0x001;
inline constexpr unsigned short left_button_up=0x002;

// The virtual device Azec writes to. Interception numbers mice 1-10; injecting
// on the first mouse path behaves like the primary physical mouse.
inline constexpr int mouse_device=1;

namespace detail {
using CreateContext=void*(*)();
using DestroyContext=void(*)(void*);
using SendStrokes=int(*)(void*,int,const void*,unsigned int);

inline HMODULE loadModule(){
    if(const HMODULE module=LoadLibraryW(L"interception.dll"))return module;
    wchar_t directory[MAX_PATH]{};
    if(!GetSystemDirectoryW(directory,MAX_PATH))return nullptr;
    std::wstring path(directory);path+=L"\\interception.dll";
    return LoadLibraryW(path.c_str());
}

struct Backend {
    HMODULE module=nullptr;
    void* context=nullptr;
    SendStrokes send=nullptr;
    DestroyContext destroy=nullptr;
    std::mutex mutex;
    bool tried=false;

    // Resolves the exports and opens a context. The context only exists when the
    // driver itself is installed, so this doubles as the availability check.
    bool ensure(){
        if(tried)return context!=nullptr;
        tried=true;
        module=loadModule();
        if(!module)return false;
        const auto create=reinterpret_cast<CreateContext>(GetProcAddress(module,"interception_create_context"));
        send=reinterpret_cast<SendStrokes>(GetProcAddress(module,"interception_send"));
        destroy=reinterpret_cast<DestroyContext>(GetProcAddress(module,"interception_destroy_context"));
        if(!create||!send)return false;
        context=create();
        return context!=nullptr;
    }

    bool stroke(MouseStroke& value){
        std::lock_guard lock(mutex);
        if(!ensure())return false;
        return send(context,mouse_device,&value,1)>0;
    }
};

inline Backend& backend(){static Backend instance;return instance;}
}

// True when interception.dll resolved and its driver answered with a context.
inline bool available(){
    auto& state=detail::backend();
    std::lock_guard lock(state.mutex);
    return state.ensure();
}

inline bool move(int dx,int dy){
    if(dx==0&&dy==0)return true;
    MouseStroke value{};
    value.flags=move_relative;value.x=dx;value.y=dy;
    return detail::backend().stroke(value);
}

inline bool button(bool down){
    MouseStroke value{};
    value.state=down?left_button_down:left_button_up;
    return detail::backend().stroke(value);
}
} // namespace driver_input
