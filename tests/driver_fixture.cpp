// Test-only DLL: models driver availability without installing a driver or
// emitting input. It is loaded explicitly by the console reliability checks.
#include <windows.h>
namespace {int mode=0,creates=0,destroys=0,context=0;}
extern "C" {
__declspec(dllexport) void fixture_mode(int value){mode=value;}
__declspec(dllexport) int fixture_creates(){return creates;}
__declspec(dllexport) int fixture_destroys(){return destroys;}
__declspec(dllexport) void* interception_create_context(){++creates;if(mode==0)SetLastError(ERROR_ACCESS_DENIED);return mode?&context:nullptr;}
__declspec(dllexport) void interception_destroy_context(void*){++destroys;}
__declspec(dllexport) int interception_send(void*,int,const void*,unsigned int){SetLastError(ERROR_NOT_READY);return mode==3?1:0;}
__declspec(dllexport) int interception_is_invalid_handle(void*){return mode==2;}
__declspec(dllexport) unsigned int interception_get_hardware_id(void*,int device,void*,unsigned int){return device==12?4:0;}
}
