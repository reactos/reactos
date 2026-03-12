/*
 * PROJECT:     ReactOS llvm-mingw compatibility shims
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Provide missing Win32 sync/process APIs for older export sets
 */

#include <psdk/windef.h>
#include <psdk/winnt.h>
#define _KERNEL32_
#include <psdk/winbase.h>
#undef _KERNEL32_
#undef NTSYSAPI
#undef NTSYSCALLAPI
#define NTSYSAPI
#define NTSYSCALLAPI
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmacro-redefined"
#endif
#include <ndk/rtlfuncs.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#include <psapi.h>
#include <stddef.h>
#include <stdarg.h>
#include <vcruntime/_mingw_mac.h>

#define LLVMCOMPAT_PRINTF_STANDARD_SNPRINTF_BEHAVIOR       (1ULL << 1)

int
__cdecl
__stdio_common_vsprintf(
    unsigned __int64 Options,
    char *Buffer,
    size_t BufferCount,
    char const *Format,
    void *Locale,
    va_list ArgList);

#if (_WIN32_WINNT < 0x0600)
typedef RTL_SRWLOCK SRWLOCK, *PSRWLOCK;
typedef RTL_CONDITION_VARIABLE CONDITION_VARIABLE, *PCONDITION_VARIABLE;
#endif

#if (_WIN32_WINNT < _WIN32_WINNT_VISTA) && (DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA)
NTSYSAPI
VOID
NTAPI
RtlRunOnceInitialize(
    _Out_ PRTL_RUN_ONCE RunOnce);

_Maybe_raises_SEH_exception_
NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceExecuteOnce(
    _Inout_ PRTL_RUN_ONCE RunOnce,
    _In_ __inner_callback PRTL_RUN_ONCE_INIT_FN InitFn,
    _Inout_opt_ PVOID Parameter,
    _Outptr_opt_result_maybenull_ PVOID *Context);

_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceBeginInitialize(
    _Inout_ PRTL_RUN_ONCE RunOnce,
    _In_ ULONG Flags,
    _Outptr_opt_result_maybenull_ PVOID *Context);

NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceComplete(
    _Inout_ PRTL_RUN_ONCE RunOnce,
    _In_ ULONG Flags,
    _In_opt_ PVOID Context);
#endif

#if (_WIN32_WINNT < _WIN32_WINNT_WIN7)
BOOLEAN
NTAPI
RtlTryAcquireSRWLockShared(
    _Inout_ PRTL_SRWLOCK SRWLock);

BOOLEAN
NTAPI
RtlTryAcquireSRWLockExclusive(
    _Inout_ PRTL_SRWLOCK SRWLock);
#endif

#ifdef _M_IX86
#define DEFINE_STDCALL_IMPORT_ALIAS(name, bytes) \
    extern const void * const llvmcompat_imp_##name __asm__("_imp__" #name "@" #bytes); \
    const void * const llvmcompat_imp_##name = (const void *)(ULONG_PTR)name
#else
#define DEFINE_STDCALL_IMPORT_ALIAS(name, bytes) \
    const void * const __MINGW_IMP_SYMBOL(name) = (const void *)(ULONG_PTR)name
#endif

BOOLEAN
WINAPI
TryAcquireSRWLockExclusive(PSRWLOCK Lock)
{
    return RtlTryAcquireSRWLockExclusive((PRTL_SRWLOCK)Lock);
}
DEFINE_STDCALL_IMPORT_ALIAS(TryAcquireSRWLockExclusive, 4);

BOOLEAN
WINAPI
TryAcquireSRWLockShared(PSRWLOCK Lock)
{
    return RtlTryAcquireSRWLockShared((PRTL_SRWLOCK)Lock);
}
DEFINE_STDCALL_IMPORT_ALIAS(TryAcquireSRWLockShared, 4);

BOOL
WINAPI
K32EnumProcessModules(
    HANDLE hProcess,
    HMODULE *lphModule,
    DWORD cb,
    LPDWORD lpcbNeeded)
{
    return EnumProcessModules(hProcess, lphModule, cb, lpcbNeeded);
}
DEFINE_STDCALL_IMPORT_ALIAS(K32EnumProcessModules, 16);

int
__CRTDECL
vsnprintf(
    char *Buffer,
    size_t BufferCount,
    char const *Format,
    va_list ArgList)
{
    int const Result = __stdio_common_vsprintf(
        LLVMCOMPAT_PRINTF_STANDARD_SNPRINTF_BEHAVIOR,
        Buffer,
        BufferCount,
        Format,
        NULL,
        ArgList);

    return Result < 0 ? -1 : Result;
}
