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

// Interception reserves 1-10 for keyboards and numbers mice from 11
// (INTERCEPTION_MOUSE(0) = INTERCEPTION_MAX_KEYBOARD + 0 + 1). Sending to 1
// addresses a keyboard device, which silently produced no pointer movement.
inline constexpr int first_mouse_device=11;
inline constexpr int mouse_device=first_mouse_device;

namespace detail {
using CreateContext=void*(*)();
using DestroyContext=void(*)(void*);
using SendStrokes=int(*)(void*,int,const void*,unsigned int);
using IsInvalidHandle=int(*)(void*);
using IsMouse=int(*)(int);

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
    IsInvalidHandle invalid=nullptr;
    IsMouse is_mouse=nullptr;
    int device=mouse_device;
    std::mutex mutex;
    bool tried=false;

    // Resolves the exports and opens a context. The context only exists when the
    // driver itself answered, so this doubles as the availability check.
    bool ensure(){
        if(tried)return context!=nullptr;
        tried=true;
        module=loadModule();
        if(!module)return false;
        const auto create=reinterpret_cast<CreateContext>(GetProcAddress(module,"interception_create_context"));
        send=reinterpret_cast<SendStrokes>(GetProcAddress(module,"interception_send"));
        destroy=reinterpret_cast<DestroyContext>(GetProcAddress(module,"interception_destroy_context"));
        invalid=reinterpret_cast<IsInvalidHandle>(GetProcAddress(module,"interception_is_invalid_handle"));
        is_mouse=reinterpret_cast<IsMouse>(GetProcAddress(module,"interception_is_mouse"));
        if(!create||!send)return false;
        context=create();
        if(!context)return false;
        // An invalid handle means the driver refused the open, usually because
        // the caller is not elevated.
        if(invalid&&invalid(context)){context=nullptr;refused=true;return false;}
        if(is_mouse)for(int candidate=first_mouse_device;candidate<first_mouse_device+10;++candidate)
            if(is_mouse(candidate)){device=candidate;break;}
        return true;
    }

    bool refused=false;

    bool stroke(MouseStroke& value){
        std::lock_guard lock(mutex);
        if(!ensure())return false;
        return send(context,device,&value,1)>0;
    }
};

inline Backend& backend(){static Backend instance;return instance;}
}

// Ready: the driver answered and strokes can be sent.
// Refused: the driver is installed but rejected the open (usually no admin).
// DriverMissing: the DLL loaded but the driver did not answer.
// DllMissing: interception.dll could not be loaded at all.
enum class State { Ready, Refused, DriverMissing, DllMissing };

inline std::wstring dllPath(){
    wchar_t path[MAX_PATH]{};
    const DWORD size=GetModuleFileNameW(nullptr,path,MAX_PATH);
    if(!size)return L"interception.dll";
    std::wstring full(path,size);
    const auto slash=full.find_last_of(L'\\');
    return (slash==std::wstring::npos?std::wstring():full.substr(0,slash+1))+L"interception.dll";
}

inline State state(){
    auto& backend=detail::backend();
    std::lock_guard lock(backend.mutex);
    if(backend.ensure())return State::Ready;
    if(backend.refused)return State::Refused;
    return backend.module?State::DriverMissing:State::DllMissing;
}

inline bool available(){return state()==State::Ready;}

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

// Short right/left sweep for the Settings test button: the cursor visibly moves
// when the driver path works, independently of any game.
inline bool test(){
    if(!move(40,0))return false;
    Sleep(60);
    return move(-40,0);
}
} // namespace driver_input
