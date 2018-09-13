/****************************** Module Header ******************************\
* Module Name: crecv.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Client side receiving stubs
*
* 07-06-91 ScottLu Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <dbt.h>

#define CALLBACKPROC 1
#define CLIENTSIDE 1

#include "callback.h"

/*
 * Receive setup and control macros
 */
#define RECVSIDE 1

#define RECVCALL(lower, upper) \
    DWORD __ ## lower (upper *pmsg)

#define BEGINRECV(err, p, cb) \
    CALLBACKSTATUS CallbackStatus;      \
    NTSTATUS Status = STATUS_SUCCESS;   \
    ULONG_PTR retval = (ULONG_PTR)err;    \
    CallbackStatus.cbOutput = cb;       \
    CallbackStatus.pOutput = p;

#define FIXUPPOINTERS() \
    if (pmsg->CaptureBuf.cCapturedPointers &&           \
            pmsg->CaptureBuf.pvVirtualAddress == NULL)  \
        FixupCallbackPointers(&pmsg->CaptureBuf);

#if defined(_X86_) && !defined(BUILD_WOW6432)

NTSTATUS
FASTCALL
XyCallbackReturn(
    IN PVOID Buffer,
    IN ULONG Length,
    IN NTSTATUS Status
    );

#define ENDRECV() \
    goto errorexit;                                                     \
errorexit:                                                              \
    CallbackStatus.retval = retval;                                     \
    return XyCallbackReturn(&CallbackStatus, sizeof(CallbackStatus),    \
            Status)

#else

#define ENDRECV() \
    goto errorexit;                                                     \
errorexit:                                                              \
    CallbackStatus.retval = retval;                                     \
    return NtCallbackReturn(&CallbackStatus, sizeof(CallbackStatus),    \
            Status)

#endif

#define MSGERROR() \
    goto errorexit

#define MSGERRORCODE(code) { \
    RIPERR0(code, RIP_WARNING, "Unspecified error"); \
    goto errorexit; }

#define MSGNTERRORCODE(code) { \
    RIPNTERR0(code, RIP_WARNING, "Unspecified error"); \
    goto errorexit; }

/*
 * Callback parameter fixup macros
 */
//#undef CALLPROC
//#define CALLPROC(p) FNID(p)

#define CALLDATA(x) (pmsg->x)
#define PCALLDATA(x) (&(pmsg->x))
#define PCALLDATAOPT(x) (pmsg->p ## x ? (PVOID)&(pmsg->x) : NULL)
#define FIRSTFIXUP(x) (pmsg->x)
#define FIXUPOPT(x) (pmsg->x)
#define FIRSTFIXUPOPT(x) FIXUPOPT(x)
#define FIXUP(x) (pmsg->x)
#define FIXUPID(x) (pmsg->x)
#define FIXUPIDOPT(x) (pmsg->x)
#define FIXUPSTRING(x) (pmsg->x.Buffer)
#define FIXUPSTRINGID(x) (pmsg->x.Buffer)
#define FIXUPSTRINGIDOPT(x) (pmsg->x.Buffer)

/***************************************************************************\
* FixupCallbackPointers
*
* Fixes up captured callback pointers.
*
* 03-13-95 JimA             Created.
\***************************************************************************/

VOID FixupCallbackPointers(
    PCAPTUREBUF pcb)
{
    DWORD i;
    LPDWORD lpdwOffset;
    PVOID *ppFixup;

    lpdwOffset = (LPDWORD)((PBYTE)pcb + pcb->offPointers);
    for (i = 0; i < pcb->cCapturedPointers; ++i, ++lpdwOffset) {
        ppFixup = (PVOID *)((PBYTE)pcb + *lpdwOffset);
        *ppFixup = (PBYTE)pcb + (LONG_PTR)*ppFixup;
    }
}

/**************************************************************************\
*
* include the stub definition file
*
\**************************************************************************/

#include "ntcb.h"
