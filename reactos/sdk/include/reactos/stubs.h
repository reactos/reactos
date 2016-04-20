#include <stdarg.h>
#define WIN32_NO_STATUS
#include <windef.h>

#include <wine/config.h>
#include <wine/exception.h>

ULONG __cdecl DbgPrint(_In_z_ _Printf_format_string_ PCSTR Format, ...);

VOID
WINAPI
RaiseException(_In_ DWORD dwExceptionCode,
               _In_ DWORD dwExceptionFlags,
               _In_ DWORD nNumberOfArguments,
               _In_ CONST ULONG_PTR *lpArguments OPTIONAL);

#define __wine_spec_unimplemented_stub(module, function) \
{ \
    ULONG_PTR args[2]; \
    args[0] = (ULONG_PTR)module; \
    args[1] = (ULONG_PTR)function; \
    RaiseException( EXCEPTION_WINE_STUB, EH_NONCONTINUABLE, 2, args ); \
}
