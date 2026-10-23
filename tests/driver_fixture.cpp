// Test-only DLL: models driver availability without installing a driver or
// emitting input. It is loaded explicitly by the console reliability checks.
namespace {int mode=0,creates=0,destroys=0,context=0;}
extern "C" {
__declspec(dllexport) void fixture_mode(int value){mode=value;}
__declspec(dllexport) int fixture_creates(){return creates;}
__declspec(dllexport) int fixture_destroys(){return destroys;}
__declspec(dllexport) void* interception_create_context(){++creates;return mode?&context:nullptr;}
__declspec(dllexport) void interception_destroy_context(void*){++destroys;}
__declspec(dllexport) int interception_send(void*,int,const void*,unsigned int){return 0;}
__declspec(dllexport) int interception_is_invalid_handle(void*){return mode==2;}
}
