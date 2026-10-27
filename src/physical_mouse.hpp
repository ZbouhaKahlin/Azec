#pragma once
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <future>
#include <mutex>
#include <thread>
#include <unordered_set>

namespace physical_mouse {
// Interception carries this through MOUSE_INPUT_DATA into RAWMOUSE. Generated
// button events must never become physical presses or cancel a physical hold.
inline constexpr unsigned int generated_marker=0x415A4543;
struct Snapshot {bool down=false;std::uint64_t presses=0;};
class Buttons {
    std::unordered_set<std::uintptr_t> held_;
    std::uint64_t presses_=0;
public:
    void event(std::uintptr_t device,unsigned short flags,unsigned int information){
        if(information==generated_marker)return;
        if((flags&RI_MOUSE_LEFT_BUTTON_DOWN)&&held_.insert(device).second)++presses_;
        if(flags&RI_MOUSE_LEFT_BUTTON_UP)held_.erase(device);
    }
    void remove(std::uintptr_t device){held_.erase(device);}
    void releaseAll(){held_.clear();}
    Snapshot snapshot() const{return {!held_.empty(),presses_};}
};
class Monitor {
    Buttons buttons_;
    mutable std::mutex mutex_;
    std::thread thread_;
    std::atomic<HWND> window_{nullptr};
    std::atomic_bool ready_{false};
    static LRESULT CALLBACK procedure(HWND window,UINT message,WPARAM w,LPARAM l){
        auto* self=reinterpret_cast<Monitor*>(GetWindowLongPtrW(window,GWLP_USERDATA));
        if(message==WM_NCCREATE){
            self=static_cast<Monitor*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams);
            SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));
        }
        if(self&&message==WM_INPUT){
            RAWINPUT input{};UINT bytes=sizeof(input);
            const auto received=GetRawInputData(reinterpret_cast<HRAWINPUT>(l),RID_INPUT,&input,&bytes,sizeof(RAWINPUTHEADER));
            if(received==UINT(-1)){std::lock_guard lock(self->mutex_);self->buttons_.releaseAll();}
            else if(input.header.dwType==RIM_TYPEMOUSE){
                std::lock_guard lock(self->mutex_);
                self->buttons_.event(reinterpret_cast<std::uintptr_t>(input.header.hDevice),input.data.mouse.usButtonFlags,input.data.mouse.ulExtraInformation);
            }
        }else if(self&&message==WM_INPUT_DEVICE_CHANGE&&w==GIDC_REMOVAL){
            std::lock_guard lock(self->mutex_);self->buttons_.remove(std::uintptr_t(l));
        }else if(message==WM_CLOSE){DestroyWindow(window);return 0;}
        else if(message==WM_DESTROY){PostQuitMessage(0);return 0;}
        return DefWindowProcW(window,message,w,l);
    }
public:
    ~Monitor(){stop();}
    bool start(){
        if(thread_.joinable())return ready_;
        std::promise<bool> created;auto result=created.get_future();
        thread_=std::thread([this,created=std::move(created)]() mutable {
            const auto instance=GetModuleHandleW(nullptr);
            WNDCLASSW kind{};kind.hInstance=instance;kind.lpszClassName=L"AzecPhysicalMouse";kind.lpfnWndProc=procedure;
            const ATOM registered=RegisterClassW(&kind);
            if(!registered&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS){created.set_value(false);return;}
            const HWND window=CreateWindowExW(0,kind.lpszClassName,L"",0,0,0,0,0,HWND_MESSAGE,nullptr,instance,this);
            RAWINPUTDEVICE device{0x01,0x02,RIDEV_INPUTSINK|RIDEV_DEVNOTIFY,window};
            if(!window||!RegisterRawInputDevices(&device,1,sizeof(device))){
                if(window)DestroyWindow(window);if(registered)UnregisterClassW(kind.lpszClassName,instance);
                created.set_value(false);return;
            }
            window_=window;ready_=true;created.set_value(true);
            MSG message{};while(GetMessageW(&message,nullptr,0,0)>0){TranslateMessage(&message);DispatchMessageW(&message);}
            ready_=false;window_=nullptr;
            RAWINPUTDEVICE remove{0x01,0x02,RIDEV_REMOVE,nullptr};RegisterRawInputDevices(&remove,1,sizeof(remove));
            if(IsWindow(window))DestroyWindow(window);
            if(registered)UnregisterClassW(kind.lpszClassName,instance);
        });
        return result.get();
    }
    void stop(){
        if(const auto window=window_.load())PostMessageW(window,WM_CLOSE,0,0);
        if(thread_.joinable())thread_.join();ready_=false;
        std::lock_guard lock(mutex_);buttons_=Buttons{};
    }
    void releaseAll(){std::lock_guard lock(mutex_);buttons_.releaseAll();}
    bool ready() const{return ready_;}
    Snapshot snapshot() const{std::lock_guard lock(mutex_);return buttons_.snapshot();}
};
}
