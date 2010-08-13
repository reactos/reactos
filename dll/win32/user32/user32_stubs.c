
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wine/config.h"
#include "wine/exception.h"

void __wine_spec_unimplemented_stub( const char *module, const char *function )
{
    ULONG_PTR args[2];

    args[0] = (ULONG_PTR)module;
    args[1] = (ULONG_PTR)function;
    RaiseException( EXCEPTION_WINE_STUB, EH_NONCONTINUABLE, 2, args );
}

static const char __wine_spec_file_name[] = "user32.dll";

void __wine_stub_user32_dll_22(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "BuildReasonArray"); }
void __wine_stub_user32_dll_23(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "CalcMenuBar"); }
void __wine_stub_user32_dll_96(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "CreateSystemThreads"); }
void __wine_stub_user32_dll_153(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "DestroyReasons"); }
void __wine_stub_user32_dll_155(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "DeviceEventWorker"); }
void __wine_stub_user32_dll_203(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "EnterReaderModeHelper"); }
void __wine_stub_user32_dll_266(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "GetCursorFrameInfo"); }
void __wine_stub_user32_dll_339(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "GetReasonTitleFromReasonCode"); }
void __wine_stub_user32_dll_396(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "InitializeWin32EntryTable"); }
void __wine_stub_user32_dll_424(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "IsProcess16Bit"); }
void __wine_stub_user32_dll_426(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "IsSETEnabled"); }
void __wine_stub_user32_dll_434(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "IsWow64Message"); }
void __wine_stub_user32_dll_451(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "LoadKeyboardLayoutEx"); }
void __wine_stub_user32_dll_484(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "MessageBoxTimeoutA"); }
void __wine_stub_user32_dll_485(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "MessageBoxTimeoutW"); }
void __wine_stub_user32_dll_511(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "PaintMenuBar"); }
void __wine_stub_user32_dll_529(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "ReasonCodeNeedsBugID"); }
void __wine_stub_user32_dll_530(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "ReasonCodeNeedsComment"); }
void __wine_stub_user32_dll_531(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "RecordShutdownReason"); }
void __wine_stub_user32_dll_543(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "RegisterMessagePumpHook"); }
void __wine_stub_user32_dll_586(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "SetConsoleReserveKeys"); }
void __wine_stub_user32_dll_658(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "SoftModalMessageBox"); }
void __wine_stub_user32_dll_681(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "TranslateMessageEx"); }
void __wine_stub_user32_dll_693(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "UnregisterMessagePumpHook"); }
void __wine_stub_user32_dll_702(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "UserLpkPSMTextOut"); }
void __wine_stub_user32_dll_703(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "UserLpkTabbedTextOut"); }
void __wine_stub_user32_dll_718(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "Win32PoolAllocationStats"); }
