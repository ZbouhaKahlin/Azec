#pragma once
#include <windows.h>
#include <mutex>
#include <string>
#include "driver_setup.hpp"
#include "diagnostics.hpp"

// Optional Interception backend (github.com/oblitum/Interception).
//
// The library is loaded at runtime and never linked or shipped with Azec: the
// signed kernel driver stays the user's own installation, and `interception.dll`
// is kept in Azec's persistent driver folder. Legacy adjacent/system copies
// remain supported after verification against the pinned upstream DLL. Strokes produced here
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

// Interception reserves 1-10 for keyboards and 11-20 for possible mouse slots.
// A slot's number does not prove a mouse is connected; query its hardware ID.
inline constexpr int first_mouse_device=11;

namespace detail {
using CreateContext=void*(*)();
using DestroyContext=void(*)(void*);
using SendStrokes=int(*)(void*,int,const void*,unsigned int);
using IsInvalidHandle=int(*)(void*);
using HardwareId=unsigned int(*)(void*,int,void*,unsigned int);

inline HMODULE loadModule(QString* failure=nullptr){
    const auto target=driver_setup::dllTarget();
    const auto load=[failure](const QString& path,const char* location)->HMODULE{
        driver_setup::VerifiedFile file;
        if(!file.open(path,driver_setup::dllSha256())){
            if(failure)*failure+=QString::fromLatin1(location)+": "+file.error()+"; ";
            return nullptr;
        }
        // The pinned DLL only needs Windows dependencies. Never search its
        // writable directory for additional executable code.
        const auto module=LoadLibraryExW(reinterpret_cast<LPCWSTR>(file.path().utf16()),nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
        const DWORD error=module?ERROR_SUCCESS:GetLastError();
        if(failure)*failure+=QString::fromLatin1(location)+": verified-dll; load_win32="+QString::number(error)+"; ";
        return module;
    };
    if(const HMODULE module=load(target,"persistent"))return module;
    {
        const auto adjacent=QDir(QCoreApplication::applicationDirPath()).filePath("interception.dll");
        if(driver_setup::place(adjacent)){
            if(const HMODULE module=load(target,"persistent"))return module;
        }
        const auto source=driver_setup::findDll(driver_setup::workRoot());
        if(!source.isEmpty()&&driver_setup::place(source)){
            if(const HMODULE module=load(target,"persistent"))return module;
        }
        if(const HMODULE module=load(adjacent,"adjacent"))return module;
    }
    wchar_t directory[MAX_PATH]{};
    if(!GetSystemDirectoryW(directory,MAX_PATH))return nullptr;
    return load(QString::fromWCharArray(directory)+QStringLiteral("/interception.dll"),"system");
}

struct Backend {
    HMODULE module=nullptr;
    void* context=nullptr;
    SendStrokes send=nullptr;
    DestroyContext destroy=nullptr;
    IsInvalidHandle invalid=nullptr;
    HardwareId hardware_id=nullptr;
    int device=0;
    std::mutex mutex;
    bool tried=false;
    QString failure_code,failure_detail;

    bool fail(const QString& code,const QString& detail){
        // Workers can retry thousands of times; log only a changed failure.
        if(failure_code!=code||failure_detail!=detail)diagnostic(code,detail);
        failure_code=code;failure_detail=detail;return false;
    }
    bool connectedMouse(int candidate) const{
        if(!context||!hardware_id)return false;
        wchar_t buffer[512]{};
        return hardware_id(context,candidate,buffer,sizeof(buffer))>0;
    }
    QString deviceSummary(){
        QStringList present;
        if(hardware_id&&context)for(int candidate=first_mouse_device;candidate<first_mouse_device+10;++candidate){
            if(connectedMouse(candidate))present<<QString::number(candidate);
        }
        HANDLE token=nullptr;TOKEN_ELEVATION elevation{};DWORD size=0;
        bool known=OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token)!=FALSE;
        if(known){known=GetTokenInformation(token,TokenElevation,&elevation,sizeof(elevation),&size)!=FALSE;CloseHandle(token);}
        return "selected_mouse="+QString::number(device)+"; hardware_id_slots="+
            (!hardware_id?QString("unavailable"):!context?QString("not-probed"):present.isEmpty()?QString("none"):present.join(','))+
            "; elevated="+(!known?QString("unknown"):QString(elevation.TokenIsElevated?"yes":"no"))+
            "; invalid_handle_export="+QString(invalid?"yes":"no");
    }

    // Opening the context and finding a connected mouse establish availability;
    // only an actual send can establish whether that device accepts output.
    bool ensure(){
        if(tried)return context!=nullptr;
        tried=true;
        QString loading;
        if(!module)module=loadModule(&loading);
        if(!module)return fail("DRV-DLL-LOAD",loading);
        const auto create=reinterpret_cast<CreateContext>(GetProcAddress(module,"interception_create_context"));
        send=reinterpret_cast<SendStrokes>(GetProcAddress(module,"interception_send"));
        destroy=reinterpret_cast<DestroyContext>(GetProcAddress(module,"interception_destroy_context"));
        invalid=reinterpret_cast<IsInvalidHandle>(GetProcAddress(module,"interception_is_invalid_handle"));
        hardware_id=reinterpret_cast<HardwareId>(GetProcAddress(module,"interception_get_hardware_id"));
        if(!create||!send||!destroy||!hardware_id)return fail("DRV-DLL-EXPORT","create="+QString::number(create!=nullptr)+
            "; send="+QString::number(send!=nullptr)+"; destroy="+QString::number(destroy!=nullptr)+"; hardware_id="+QString::number(hardware_id!=nullptr));
        SetLastError(ERROR_SUCCESS);
        context=create();
        const DWORD context_error=GetLastError();
        if(!context)return fail(context_error==ERROR_ACCESS_DENIED?"DRV-CONTEXT-ACCESS":"DRV-CONTEXT-OPEN",
            "context_win32="+QString::number(context_error)+"; "+deviceSummary());
        // An invalid handle means the driver refused the open, usually because
        // the caller is not elevated.
        if(invalid&&invalid(context)){if(destroy)destroy(context);context=nullptr;refused=true;return fail("DRV-CONTEXT-REFUSED",deviceSummary());}
        device=0;
        for(int candidate=first_mouse_device;candidate<first_mouse_device+10;++candidate)
            if(connectedMouse(candidate)){device=candidate;break;}
        if(!device){
            const auto summary=deviceSummary();destroy(context);context=nullptr;
            return fail("DRV-MOUSE-NOT-FOUND",summary);
        }
        return true;
    }

    bool refused=false;

    // Keep the loaded library reference, but reopen a failed driver context on
    // an explicit retry. A working context is never interrupted.
    void retryInitialization(){if(!context){tried=false;refused=false;failure_code.clear();failure_detail.clear();}}

    bool stroke(MouseStroke& value){
        std::lock_guard lock(mutex);
        if(!ensure())return false;
        SetLastError(ERROR_SUCCESS);
        const int sent=send(context,device,&value,1);
        const DWORD error=GetLastError();
        if(sent>0)return true;
        const QString code=value.state?"DRV-SEND-BUTTON":"DRV-SEND-MOVE";
        const QString prefix="sent="+QString::number(sent)+"; send_win32="+QString::number(error)+"; ";
        if(failure_code==code&&failure_detail.startsWith(prefix))return false;
        return fail(code,prefix+deviceSummary()+"; Windows error is the DLL call's last error, not a confirmed cause");
    }
};

inline Backend& backend(){static Backend instance;return instance;}
}

// Ready: the driver opened and a mouse slot exposes a hardware ID; sends are untested.
// Refused: the driver is installed but rejected the open (usually no admin).
// DriverMissing: the DLL loaded but the driver did not answer.
// DllMissing: interception.dll could not be loaded at all.
enum class State { Ready, Refused, DriverMissing, DllMissing };

inline std::wstring dllPath(){return driver_setup::dllTarget().toStdWString();}

inline State state(){
    auto& backend=detail::backend();
    std::lock_guard lock(backend.mutex);
    if(backend.ensure())return State::Ready;
    if(backend.refused)return State::Refused;
    return backend.module?State::DriverMissing:State::DllMissing;
}

inline bool available(){return state()==State::Ready;}

inline QString failureCode(){auto& b=detail::backend();std::lock_guard lock(b.mutex);return b.failure_code;}
inline QString failureDetail(){auto& b=detail::backend();std::lock_guard lock(b.mutex);return b.failure_detail;}

inline void retryInitialization(){
    auto& backend=detail::backend();std::lock_guard lock(backend.mutex);
    backend.retryInitialization();
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

// Short right/left sweep for the Settings test button: the cursor visibly moves
// when the driver path works, independently of any game.
inline bool test(QString* failedPhase=nullptr){
    if(failedPhase)*failedPhase="DRV-TEST-RIGHT";
    if(!move(40,0))return false;
    Sleep(60);
    if(failedPhase)*failedPhase="DRV-TEST-RETURN";
    const bool sent=move(-40,0);
    if(sent&&failedPhase)failedPhase->clear();
    return sent;
}
} // namespace driver_input
