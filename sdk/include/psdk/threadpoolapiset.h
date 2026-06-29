
#pragma once

#ifndef _THREADPOOLAPISET_H_
#define _THREADPOOLAPISET_H_

#include <minwindef.h>
#include <minwinbase.h>

#ifdef __cplusplus
extern "C" {
#endif

WINBASEAPI
_Must_inspect_result_
PTP_POOL
WINAPI
CreateThreadpool(
    _Reserved_ PVOID reserved);

WINBASEAPI
VOID
WINAPI
CloseThreadpool(
    _Inout_ PTP_POOL ptpp);

WINBASEAPI
BOOL
WINAPI
SetThreadpoolThreadMinimum(
    _Inout_ PTP_POOL ptpp,
    _In_ DWORD cthrdMic);

WINBASEAPI
VOID
WINAPI
SetThreadpoolThreadMaximum(
    _Inout_ PTP_POOL ptpp,
    _In_ DWORD cthrdMost);

WINBASEAPI
BOOL
WINAPI
QueryThreadpoolStackInformation(
    _In_ PTP_POOL ptpp,
    _Out_ PTP_POOL_STACK_INFORMATION ptpsi);

WINBASEAPI
BOOL
WINAPI
SetThreadpoolStackInformation(
    _Inout_ PTP_POOL ptpp,
    _In_ PTP_POOL_STACK_INFORMATION ptpsi);

WINBASEAPI
_Must_inspect_result_
BOOL
WINAPI
TrySubmitThreadpoolCallback(
    _In_ PTP_SIMPLE_CALLBACK pfns,
    _Inout_opt_ PVOID pv,
    _In_opt_ PTP_CALLBACK_ENVIRON pcbe);

WINBASEAPI
BOOL
WINAPI
CallbackMayRunLong(
    _Inout_ PTP_CALLBACK_INSTANCE pci);

WINBASEAPI
VOID
WINAPI
DisassociateCurrentThreadFromCallback(
    _Inout_ PTP_CALLBACK_INSTANCE pci);

WINBASEAPI
VOID
WINAPI
FreeLibraryWhenCallbackReturns(
    _Inout_ PTP_CALLBACK_INSTANCE pci,
    _In_ HMODULE mod);

WINBASEAPI
VOID
WINAPI
LeaveCriticalSectionWhenCallbackReturns(
    _Inout_ PTP_CALLBACK_INSTANCE pci,
    _Inout_ PCRITICAL_SECTION pcs);

WINBASEAPI
VOID
WINAPI
ReleaseMutexWhenCallbackReturns(
    _Inout_ PTP_CALLBACK_INSTANCE pci,
    _In_ HANDLE mut);

WINBASEAPI
VOID
WINAPI
ReleaseSemaphoreWhenCallbackReturns(
    _Inout_ PTP_CALLBACK_INSTANCE pci,
    _In_ HANDLE sem,
    _In_ DWORD crel);

WINBASEAPI
VOID
WINAPI
SetEventWhenCallbackReturns(
    _Inout_ PTP_CALLBACK_INSTANCE pci,
    _In_ HANDLE evt);

WINBASEAPI
_Must_inspect_result_
PTP_CLEANUP_GROUP
WINAPI
CreateThreadpoolCleanupGroup(
    VOID);

WINBASEAPI
VOID
WINAPI
CloseThreadpoolCleanupGroup(
    _Inout_ PTP_CLEANUP_GROUP ptpcg);

WINBASEAPI
VOID
WINAPI
CloseThreadpoolCleanupGroupMembers(
    _Inout_ PTP_CLEANUP_GROUP ptpcg,
    _In_ BOOL fCancelPendingCallbacks,
    _Inout_opt_ PVOID pvCleanupContext);

typedef
VOID
(WINAPI *PTP_WIN32_IO_CALLBACK)(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID Context,
    _Inout_opt_ PVOID Overlapped,
    _In_ ULONG IoResult,
    _In_ ULONG_PTR NumberOfBytesTransferred,
    _Inout_ PTP_IO Io);

WINBASEAPI
_Must_inspect_result_
PTP_IO
WINAPI
CreateThreadpoolIo(
    _In_ HANDLE fl,
    _In_ PTP_WIN32_IO_CALLBACK pfnio,
    _Inout_opt_ PVOID pv,
    _In_opt_ PTP_CALLBACK_ENVIRON pcbe);

WINBASEAPI
VOID
WINAPI
CloseThreadpoolIo(
    _Inout_ PTP_IO pio);

WINBASEAPI
VOID
WINAPI
StartThreadpoolIo(
    _Inout_ PTP_IO pio);

WINBASEAPI
VOID
WINAPI
CancelThreadpoolIo(
    _Inout_ PTP_IO pio);

WINBASEAPI
VOID
WINAPI
WaitForThreadpoolIoCallbacks(
    _Inout_ PTP_IO pio,
    _In_ BOOL fCancelPendingCallbacks);

WINBASEAPI
_Must_inspect_result_
PTP_TIMER
WINAPI
CreateThreadpoolTimer(
    _In_ PTP_TIMER_CALLBACK pfnti,
    _Inout_opt_ PVOID pv,
    _In_opt_ PTP_CALLBACK_ENVIRON pcbe);

WINBASEAPI
VOID
WINAPI
CloseThreadpoolTimer(
    _Inout_ PTP_TIMER pti);

WINBASEAPI
VOID
WINAPI
SetThreadpoolTimer(
    _Inout_ PTP_TIMER pti,
    _In_opt_ PFILETIME pftDueTime,
    _In_ DWORD msPeriod,
    _In_opt_ DWORD msWindowLength);

WINBASEAPI
BOOL
WINAPI
SetThreadpoolTimerEx(
    _Inout_ PTP_TIMER pti,
    _In_opt_ PFILETIME pftDueTime,
    _In_ DWORD msPeriod,
    _In_opt_ DWORD msWindowLength);

WINBASEAPI
BOOL
WINAPI
IsThreadpoolTimerSet(
    _Inout_ PTP_TIMER pti);

WINBASEAPI
VOID
WINAPI
WaitForThreadpoolTimerCallbacks(
    _Inout_ PTP_TIMER pti,
    _In_ BOOL fCancelPendingCallbacks);

WINBASEAPI
_Must_inspect_result_
PTP_WAIT
WINAPI
CreateThreadpoolWait(
    _In_ PTP_WAIT_CALLBACK pfnwa,
    _Inout_opt_ PVOID pv,
    _In_opt_ PTP_CALLBACK_ENVIRON pcbe);

WINBASEAPI
VOID
WINAPI
CloseThreadpoolWait(
    _Inout_ PTP_WAIT pwa);

WINBASEAPI
VOID
WINAPI
SetThreadpoolWait(
    _Inout_ PTP_WAIT pwa,
    _In_opt_ HANDLE h,
    _In_opt_ PFILETIME pftTimeout);

WINBASEAPI
BOOL
WINAPI
SetThreadpoolWaitEx(
    _Inout_ PTP_WAIT pwa,
    _In_opt_ HANDLE h,
    _In_opt_ PFILETIME pftTimeout,
    _Reserved_ PVOID Reserved);

WINBASEAPI
VOID
WINAPI
WaitForThreadpoolWaitCallbacks(
    _Inout_ PTP_WAIT pwa,
    _In_ BOOL fCancelPendingCallbacks);

WINBASEAPI
_Must_inspect_result_
PTP_WORK
WINAPI
CreateThreadpoolWork(
    _In_ PTP_WORK_CALLBACK pfnwk,
    _Inout_opt_ PVOID pv,
    _In_opt_ PTP_CALLBACK_ENVIRON pcbe);

WINBASEAPI
VOID
WINAPI
CloseThreadpoolWork(
    _Inout_ PTP_WORK pwk);

WINBASEAPI
VOID
WINAPI
SubmitThreadpoolWork(
    _Inout_ PTP_WORK pwk);

WINBASEAPI
VOID
WINAPI
WaitForThreadpoolWorkCallbacks(
    _Inout_ PTP_WORK pwk,
    _In_ BOOL fCancelPendingCallbacks);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _THREADPOOLAPISET_H_
