#pragma once
#include <windows.h>
#include <QString>
#include <iterator>

namespace r6 {
// Keep this pure so the verification executable can test it without a game.
inline bool isGameProcessName(const QString& processPath) {
    QString name = processPath;
    const int slash = name.lastIndexOf('\\');
    if (slash >= 0) name = name.mid(slash + 1);
    name = name.toLower();
    return name=="rainbowsix.exe"||name=="rainbowsix_vulkan.exe"||
           name=="rainbowsix_dx11.exe"||name=="rainbowsix_dx12.exe";
}

inline bool gameIsActive() {
    const HWND window = GetForegroundWindow();
    if (!window) return false;
    DWORD processId = 0;
    GetWindowThreadProcessId(window, &processId);
    // Each input thread has its own cache; focus changes invalidate it immediately.
    thread_local HWND lastWindow = nullptr;
    thread_local DWORD lastProcess = 0;
    thread_local ULONGLONG checkedAt = 0;
    thread_local bool lastAnswer = false;
    const auto now=GetTickCount64();
    if (window==lastWindow&&processId==lastProcess&&now-checkedAt<200) return lastAnswer;
    lastWindow = window;
    lastProcess=processId;checkedAt=now;
    lastAnswer = false;
    if (processId) {
        const HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
        if (process) {
            wchar_t path[32768]{};
            DWORD size = DWORD(std::size(path));
            if (QueryFullProcessImageNameW(process, 0, path, &size))
                lastAnswer = isGameProcessName(QString::fromWCharArray(path, int(size)));
            CloseHandle(process);
        }
    }
    return lastAnswer;
}
} // namespace r6
