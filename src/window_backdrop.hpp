#pragma once
#include <windows.h>
#include <memory>

// Windows 10's whole-window acrylic ignores HWND regions. Keep its backdrop
// in a noninteractive composition surface, where the brush itself is clipped.
class WindowBackdrop {
    struct State;
    std::unique_ptr<State> state_;
public:
    WindowBackdrop();
    ~WindowBackdrop();
    bool create(HWND owner);
    void sync(float radius,bool visible);
    void hide();
    void reset();
#ifdef AZEC_UI_TESTS
    HWND handle() const;
    bool clipMatchesOwner(float radius) const;
#endif
};
