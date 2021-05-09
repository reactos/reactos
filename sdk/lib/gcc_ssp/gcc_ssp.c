
#ifdef _GCC_SSP_MSVCRT_

#include <windef.h>
#include <winbase.h>
#include <stdio.h>

#define print_caller() do {                                                                                                             \
    char buffer[64];                                                                                                                    \
    _snprintf(buffer, sizeof(buffer), "STACK PROTECTOR FAULT AT %p\n", __builtin_extract_return_addr(__builtin_return_address (0)));    \
    OutputDebugStringA(buffer);                                                                                                         \
} while(0)

#elif defined(_GCC_SSP_WIN32K_)

#include <windef.h>
#include <wingdi.h>
#include <winddi.h>
#include <stdarg.h>

static inline
void
print_caller_helper(char* fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    EngDebugPrint("", fmt, args);
    va_end(args);
}

#define print_caller() print_caller_helper("STACK PROTECTOR FAULT AT %p\n", __builtin_extract_return_addr(__builtin_return_address(0)))

#elif defined(_GCC_SSP_SCSIPORT_)

#include <ntddk.h>
#include <srb.h>

#define print_caller() ScsiDebugPrint(0, "STACK PROTECTOR FAULT AT %p\n", __builtin_extract_return_addr(__builtin_return_address(0)))

#elif defined(_GCC_SSP_VIDEOPRT_)

#include <ntdef.h>
#include <miniport.h>
#include <video.h>

#define print_caller() VideoPortDebugPrint(0, "STACK PROTECTOR FAULT AT %p\n", __builtin_extract_return_addr(__builtin_return_address(0)))

#else

#include <ntdef.h>
#include <debug.h>

#define print_caller() DbgPrint("STACK PROTECTOR FAULT AT %p\n", __builtin_extract_return_addr(__builtin_return_address(0)))

#endif

/* Should be random :-/ */
void * __stack_chk_guard = (void*)0xb00fbeefbaafb00f;

void __stack_chk_fail()
{
    print_caller();
    __asm__("int $3");
}
