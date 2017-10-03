#ifndef _RXLOWIO_
#define _RXLOWIO_

#include "mrx.h"

extern FAST_MUTEX RxLowIoPagingIoSyncMutex;

#define RxLowIoIsMdlLocked(MDL) (RxMdlIsLocked((MDL)) || RxMdlSourceIsNonPaged((MDL)))

#define RxLowIoIsBufferLocked(LowIoContext)                              \
    (((LowIoContext)->Operation > LOWIO_OP_WRITE) ||                     \
     ((LowIoContext)->ParamsFor.ReadWrite.Buffer == NULL) ||             \
     (((LowIoContext)->ParamsFor.ReadWrite.Buffer != NULL) &&            \
       RxLowIoIsMdlLocked(((LowIoContext)->ParamsFor.ReadWrite.Buffer))))


typedef struct _LOWIO_PER_FCB_INFO
{
    LIST_ENTRY PagingIoReadsOutstanding;
    LIST_ENTRY PagingIoWritesOutstanding;
} LOWIO_PER_FCB_INFO, *PLOWIO_PER_FCB_INFO;

#if (_WIN32_WINNT >= 0x0600)
NTSTATUS
NTAPI
RxLowIoPopulateFsctlInfo(
    _In_ PRX_CONTEXT RxContext,
    _In_ PIRP Irp);
#else
NTSTATUS
NTAPI
RxLowIoPopulateFsctlInfo(
    _In_ PRX_CONTEXT RxContext);
#endif

#if (_WIN32_WINNT >= 0x0600)
NTSTATUS
NTAPI
RxLowIoSubmit(
    _In_ PRX_CONTEXT RxContext,
    _In_ PIRP Irp,
    _In_ PFCB Fcb,
    _In_ PLOWIO_COMPLETION_ROUTINE CompletionRoutine);
#else
NTSTATUS
NTAPI
RxLowIoSubmit(
    _In_ PRX_CONTEXT RxContext,
    _In_ PLOWIO_COMPLETION_ROUTINE CompletionRoutine);
#endif

NTSTATUS
NTAPI
RxLowIoCompletion(
    _In_ PRX_CONTEXT RxContext);

#if (_WIN32_WINNT >= 0x0600)
VOID
NTAPI
RxInitializeLowIoContext(
    _In_ PRX_CONTEXT RxContext,
    _In_ ULONG Operation,
    _Out_ PLOWIO_CONTEXT LowIoContext);
#else
VOID
NTAPI
RxInitializeLowIoContext(
    _Out_ PLOWIO_CONTEXT LowIoContext,
    _In_ ULONG Operation);
#endif

VOID
RxInitializeLowIoPerFcbInfo(
    _Inout_ PLOWIO_PER_FCB_INFO LowIoPerFcbInfo);

#endif
