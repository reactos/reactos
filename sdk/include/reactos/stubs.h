#define WIN32_NO_STATUS
#include <windef.h>

#ifndef PRIx64
#define PRIx64 "I64x"
#endif

#define EXCEPTION_WINE_STUB     0x80000100
#define EH_NONCONTINUABLE       0x01

NTSYSAPI
VOID
NTAPI
RtlRaiseException(
    _In_ PEXCEPTION_RECORD ExceptionRecord
);

ULONG
__cdecl
DbgPrint(
    _In_z_ _Printf_format_string_ PCSTR Format,
    ...
);

#define __wine_spec_unimplemented_stub(module, function) \
{ \
    EXCEPTION_RECORD ExceptionRecord = {0}; \
    ExceptionRecord.ExceptionRecord = NULL; \
    ExceptionRecord.ExceptionCode = EXCEPTION_WINE_STUB; \
    ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE; \
    ExceptionRecord.ExceptionInformation[0] = (ULONG_PTR)module; \
    ExceptionRecord.ExceptionInformation[1] = (ULONG_PTR)function; \
    ExceptionRecord.NumberParameters = 2; \
    RtlRaiseException(&ExceptionRecord); \
}
