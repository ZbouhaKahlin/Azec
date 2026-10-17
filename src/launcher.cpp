#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR arguments,int){
    wchar_t module[32768]{};
    if(!GetModuleFileNameW(nullptr,module,32768))return 1;
    std::wstring folder(module);folder.resize(folder.find_last_of(L"\\/")+1);
    const auto executable=folder+L"runtime\\Azec.exe";
    std::wstring command=L"\""+executable+L"\" "+arguments;
    STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION process{};
    if(!CreateProcessW(executable.c_str(),command.data(),nullptr,nullptr,FALSE,0,nullptr,folder.c_str(),&startup,&process)){
        MessageBoxW(nullptr,L"The application runtime is missing. Run build.cmd to rebuild it.",L"Azec",MB_OK|MB_ICONERROR);return 1;
    }
    CloseHandle(process.hThread);CloseHandle(process.hProcess);return 0;
}
