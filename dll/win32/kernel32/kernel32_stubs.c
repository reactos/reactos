
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

static const char __wine_spec_file_name[] = "kernel32.dll";

void __wine_stub_kernel32_dll_14(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "AllocLSCallback"); }
void __wine_stub_kernel32_dll_23(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "BaseCheckRunApp"); }
void __wine_stub_kernel32_dll_33(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "BasepCheckWinSaferRestrictions"); }
void __wine_stub_kernel32_dll_34(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "BasepDebugDump"); }
void __wine_stub_kernel32_dll_60(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "CloseSystemHandle"); }
void __wine_stub_kernel32_dll_69(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "ConsoleSubst"); }
void __wine_stub_kernel32_dll_103(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "CreateKernelThread"); }
void __wine_stub_kernel32_dll_347(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "GetDaylightFlag"); }
void __wine_stub_kernel32_dll_391(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "GetSCallbackTarget"); }
void __wine_stub_kernel32_dll_392(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "GetSCallbackTemplate"); }
void __wine_stub_kernel32_dll_456(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "GetProductName"); }
void __wine_stub_kernel32_dll_464(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "GetLSCallbackTarget"); }
void __wine_stub_kernel32_dll_465(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "GetLSCallbackTemplate"); }
void __wine_stub_kernel32_dll_567(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "HeapSetFlags"); }
void __wine_stub_kernel32_dll_591(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "InvalidateNSCache"); }
void __wine_stub_kernel32_dll_664(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "NlsResetProcessLocale"); }
void __wine_stub_kernel32_dll_665(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "NotifyNLSUserCache"); }
void __wine_stub_kernel32_dll_709(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "QueryNumberOfEventLogRecords"); }
void __wine_stub_kernel32_dll_710(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "QueryOldestEventLogRecord"); }
void __wine_stub_kernel32_dll_717(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "ReOpenFile"); }
void __wine_stub_kernel32_dll_738(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "RegisterSysMsgHandler"); }
void __wine_stub_kernel32_dll_822(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "SetDaylightFlag"); }
void __wine_stub_kernel32_dll_848(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "SetLastConsoleEventActive"); }
void __wine_stub_kernel32_dll_850(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "SetLocalPrimaryComputerNameA"); }
void __wine_stub_kernel32_dll_851(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "SetLocalPrimaryComputerNameW"); }
void __wine_stub_kernel32_dll_906(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "TlsAllocInternal"); }
void __wine_stub_kernel32_dll_908(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "TlsFreeInternal"); }
void __wine_stub_kernel32_dll_930(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "ValidateCType"); }
void __wine_stub_kernel32_dll_931(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "ValidateLocale"); }
void __wine_stub_kernel32_dll_992(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_DebugOut"); }
void __wine_stub_kernel32_dll_993(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "_DebugPrintf"); }
void __wine_stub_kernel32_dll_1002(void) { __wine_spec_unimplemented_stub(__wine_spec_file_name, "dprintf"); }

