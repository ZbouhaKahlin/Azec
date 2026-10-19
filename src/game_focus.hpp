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
    // Rainbow Six Siege runs as RainbowSix.exe; some builds add a suffix.
    // BattlEye's RainbowSix_BE.exe is not the game window.
    return name.startsWith("rainbowsix") && !name.contains("_be");
}

inline bool gameIsActive() {
    const HWND window = GetForegroundWindow();
    if (!window) return false;
    // Querying the process every 8 ms would be wasteful. The foreground window
    // only changes on alt-tab/focus changes, so cache the last answer.
    static HWND lastWindow = nullptr;
    static bool lastAnswer = false;
    if (window == lastWindow) return lastAnswer;
    lastWindow = window;
    lastAnswer = false;
    DWORD processId = 0;
    GetWindowThreadProcessId(window, &processId);
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
    if (!lastAnswer) {
        wchar_t title[512]{};
        if (GetWindowTextW(window, title, int(std::size(title))))
            lastAnswer = QString::fromWCharArray(title).contains("Rainbow Six", Qt::CaseInsensitive);
    }
    return lastAnswer;
}
} // namespace r6
