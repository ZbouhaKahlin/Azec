#include "window_backdrop.hpp"
#include <roapi.h>
#include <wrl/client.h>
#include <wrl/wrappers/corewrappers.h>
#include <windows.ui.composition.h>
#include <windows.ui.composition.desktop.h>
#include <windows.ui.composition.interop.h>
#include <DispatcherQueue.h>
#include <algorithm>

using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Wrappers::HStringReference;
namespace composition=ABI::Windows::UI::Composition;
namespace systemQueue=ABI::Windows::System;
namespace {
void require(HRESULT result){if(FAILED(result))throw result;}
struct Context {
    Microsoft::WRL::Wrappers::RoInitializeWrapper apartment{RO_INIT_SINGLETHREADED};
    ComPtr<systemQueue::IDispatcherQueueController> queue;
    ComPtr<composition::ICompositor> compositor;
    void initialize(){
        require(apartment);
        ComPtr<systemQueue::IDispatcherQueueStatics> statics;
        require(RoGetActivationFactory(HStringReference(RuntimeClass_Windows_System_DispatcherQueue).Get(),IID_PPV_ARGS(&statics)));
        ComPtr<systemQueue::IDispatcherQueue> current;require(statics->GetForCurrentThread(&current));
        if(!current){const DispatcherQueueOptions options{sizeof(DispatcherQueueOptions),DQTYPE_THREAD_CURRENT,DQTAT_COM_NONE};require(CreateDispatcherQueueController(options,&queue));}
        ComPtr<IInspectable> instance;require(RoActivateInstance(HStringReference(RuntimeClass_Windows_UI_Composition_Compositor).Get(),&instance));require(instance.As(&compositor));
    }
    ~Context(){
        compositor.Reset();
        if(queue){
            ComPtr<ABI::Windows::Foundation::IAsyncAction> shutdown;
            if(SUCCEEDED(queue->ShutdownQueueAsync(&shutdown))){
                ComPtr<ABI::Windows::Foundation::IAsyncInfo> info;
                if(SUCCEEDED(shutdown.As(&info))){
                    ABI::Windows::Foundation::AsyncStatus status=ABI::Windows::Foundation::AsyncStatus::Started;
                    bool quit=false;WPARAM exitCode=0;
                    while(SUCCEEDED(info->get_Status(&status))&&status==ABI::Windows::Foundation::AsyncStatus::Started){
                        MSG message{};while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){
                            if(message.message==WM_QUIT){quit=true;exitCode=message.wParam;}
                            else{TranslateMessage(&message);DispatchMessageW(&message);}
                        }
                        Sleep(1);
                    }
                    if(quit)PostQuitMessage(int(exitCode));
                }
            }
        }
    }
};
std::shared_ptr<Context> context(){
    static thread_local std::weak_ptr<Context> cached;
    auto result=cached.lock();if(!result){result=std::make_shared<Context>();result->initialize();cached=result;}return result;
}
LRESULT CALLBACK backdropEvent(HWND window,UINT message,WPARAM wParam,LPARAM lParam){
    if(message==WM_NCHITTEST)return HTTRANSPARENT;
    if(message==WM_MOUSEACTIVATE)return MA_NOACTIVATE;
    return DefWindowProcW(window,message,wParam,lParam);
}
HWND createSurface(){
    static const auto registered=[]{
        WNDCLASSW type{};type.lpfnWndProc=backdropEvent;type.hInstance=GetModuleHandleW(nullptr);type.lpszClassName=L"AzecClippedBackdrop";
        return RegisterClassW(&type)!=0||GetLastError()==ERROR_CLASS_ALREADY_EXISTS;
    }();
    if(!registered)return nullptr;
    return CreateWindowExW(WS_EX_NOREDIRECTIONBITMAP|WS_EX_NOACTIVATE|WS_EX_TRANSPARENT|WS_EX_TOOLWINDOW,L"AzecClippedBackdrop",L"",WS_POPUP,0,0,0,0,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
}
}

struct WindowBackdrop::State {
    std::shared_ptr<Context> shared;
    HWND owner=nullptr,window=nullptr;
    ComPtr<composition::Desktop::IDesktopWindowTarget> target;
    ComPtr<composition::ICompositionTarget> root;
    ComPtr<composition::IVisual> visual;
    ComPtr<composition::ICompositionRoundedRectangleGeometry> geometry;
    float width=-1,height=-1,radius=-1;
    ~State(){
        if(window)ShowWindow(window,SW_HIDE);
        if(root)root->put_Root(nullptr);
        ComPtr<ABI::Windows::Foundation::IClosable> closable;
        if(target&&SUCCEEDED(target.As(&closable)))closable->Close();
        geometry.Reset();visual.Reset();root.Reset();target.Reset();
        if(window)DestroyWindow(window);
    }
};

WindowBackdrop::WindowBackdrop()=default;
WindowBackdrop::~WindowBackdrop()=default;
bool WindowBackdrop::create(HWND owner){
    if(state_)return true;
    auto next=std::make_unique<State>();
    try{
        next->shared=context();next->owner=owner;next->window=createSurface();if(!next->window)return false;
        auto compositor=next->shared->compositor;
        ComPtr<composition::Desktop::ICompositorDesktopInterop> interop;require(compositor.As(&interop));require(interop->CreateDesktopWindowTarget(next->window,FALSE,&next->target));require(next->target.As(&next->root));
        ComPtr<composition::ISpriteVisual> sprite;require(compositor->CreateSpriteVisual(&sprite));require(sprite.As(&next->visual));
        ComPtr<composition::ICompositor3> brushes;require(compositor.As(&brushes));ComPtr<composition::ICompositionBackdropBrush> backdrop;require(brushes->CreateHostBackdropBrush(&backdrop));
        ComPtr<composition::ICompositionBrush> brush;require(backdrop.As(&brush));require(sprite->put_Brush(brush.Get()));
        ComPtr<composition::ICompositor5> shapes;require(compositor.As(&shapes));require(shapes->CreateRoundedRectangleGeometry(&next->geometry));
        ComPtr<composition::ICompositionGeometry> geometry;require(next->geometry.As(&geometry));
        ComPtr<composition::ICompositor6> clips;require(compositor.As(&clips));ComPtr<composition::ICompositionGeometricClip> geometricClip;require(clips->CreateGeometricClipWithGeometry(geometry.Get(),&geometricClip));
        ComPtr<composition::ICompositionClip> clip;require(geometricClip.As(&clip));require(next->visual->put_Clip(clip.Get()));require(next->root->put_Root(next->visual.Get()));
        // HostBackdrop supplies the live, system-blurred desktop to this visual
        // without adding the rectangular acrylic material to the Qt HWND.
        struct Policy{DWORD state,flags,color,animation;};struct Attribute{DWORD id;PVOID data;SIZE_T size;};
        const auto setComposition=reinterpret_cast<BOOL(WINAPI*)(HWND,Attribute*)>(GetProcAddress(GetModuleHandleW(L"user32.dll"),"SetWindowCompositionAttribute"));
        Policy policy{5,0,0,0};Attribute attribute{19,&policy,sizeof(policy)};
        if(!setComposition||!setComposition(next->window,&attribute))return false;
        state_=std::move(next);return true;
    }catch(HRESULT){return false;}
}
void WindowBackdrop::sync(float radius,bool visible){
    if(!state_)return;
    const auto owner=state_->owner;
    if(!visible||!(GetWindowLongPtrW(owner,GWL_STYLE)&WS_VISIBLE)||IsIconic(owner)){hide();return;}
    RECT rect{};if(!GetClientRect(owner,&rect)){hide();return;}
    POINT origin{};if(!ClientToScreen(owner,&origin)){hide();return;}
    const float width=float(rect.right),height=float(rect.bottom);radius=std::clamp(radius,0.f,std::min(width,height)/2);
    if(width!=state_->width||height!=state_->height||radius!=state_->radius){
        if(FAILED(state_->geometry->put_Size({width,height}))||FAILED(state_->geometry->put_CornerRadius({radius,radius}))||FAILED(state_->visual->put_Size({width,height}))){hide();return;}
        state_->width=width;state_->height=height;state_->radius=radius;
        // Match the content's hit-test cutouts too; the helper must never catch
        // clicks in the transparent corners, including over another process.
        HRGN region=CreateRectRgn(0,0,0,0);
        if(region){const int kind=GetWindowRgn(owner,region);if(kind==ERROR){DeleteObject(region);SetWindowRgn(state_->window,nullptr,FALSE);}else if(!SetWindowRgn(state_->window,region,FALSE))DeleteObject(region);}
    }
    // Behind the content, without activation, input, a taskbar entry, or polling.
    SetWindowPos(state_->window,owner,origin.x,origin.y,rect.right,rect.bottom,SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_SHOWWINDOW);
}
void WindowBackdrop::hide(){if(state_&&(GetWindowLongPtrW(state_->window,GWL_STYLE)&WS_VISIBLE))ShowWindow(state_->window,SW_HIDE);}
void WindowBackdrop::reset(){state_.reset();}
#ifdef AZEC_UI_TESTS
HWND WindowBackdrop::handle() const{return state_?state_->window:nullptr;}
bool WindowBackdrop::clipMatchesOwner(float radius) const{
    if(!state_)return false;
    ComPtr<composition::IVisual> root;ComPtr<composition::ICompositionClip> clip;ComPtr<composition::ICompositionGeometricClip> geometricClip;
    ComPtr<composition::ICompositionGeometry> geometry;ComPtr<composition::ICompositionRoundedRectangleGeometry> rounded;
    if(FAILED(state_->root->get_Root(&root))||!root||FAILED(root->get_Clip(&clip))||!clip||FAILED(clip.As(&geometricClip))||FAILED(geometricClip->get_Geometry(&geometry))||!geometry||FAILED(geometry.As(&rounded)))return false;
    ABI::Windows::Foundation::Numerics::Vector2 size{},corners{};RECT bounds{};GetClientRect(state_->owner,&bounds);
    return SUCCEEDED(rounded->get_Size(&size))&&SUCCEEDED(rounded->get_CornerRadius(&corners))&&size.X==bounds.right&&size.Y==bounds.bottom&&corners.X==radius&&corners.Y==radius;
}
#endif
