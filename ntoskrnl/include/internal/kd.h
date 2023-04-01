#pragma once

#include <cportlib/cportlib.h>

//
// Kernel Debugger Port Definition
//

BOOLEAN
NTAPI
KdPortInitializeEx(
    PCPPORT PortInformation,
    ULONG ComPortNumber
);

BOOLEAN
NTAPI
KdPortGetByteEx(
    PCPPORT PortInformation,
    PUCHAR ByteReceived);

VOID
NTAPI
KdPortPutByteEx(
    PCPPORT PortInformation,
    UCHAR ByteToSend
);

#ifdef _NTOSKRNL_

/* KD GLOBALS ****************************************************************/

typedef enum _KD_CONTINUE_TYPE
{
    kdContinue = 0,
    kdDoNotHandleException,
    kdHandleException
} KD_CONTINUE_TYPE;

/* KD Internal Debug Services */
typedef enum _KDP_DEBUG_SERVICE
{
    DumpNonPagedPool = 0x1e, /* a */
    ManualBugCheck = 0x30, /* b */
    DumpNonPagedPoolStats = 0x2e, /* c */
    DumpNewNonPagedPool = 0x20, /* d */
    DumpNewNonPagedPoolStats = 0x12, /* e */
    DumpAllThreads = 0x21, /* f */
    DumpUserThreads = 0x22, /* g */
    KdSpare1 = 0x23, /* h */
    KdSpare2 = 0x17, /* i */
    KdSpare3 = 0x24, /* j */
    EnterDebugger = 0x25,  /* k */
    ThatsWhatSheSaid = 69 /* FIGURE IT OUT */
} KDP_DEBUG_SERVICE;

#endif // _NTOSKRNL_

#if DBG && defined(_M_IX86) && !defined(_WINKD_) // See ke/i386/traphdlr.c
#define ID_Win32PreServiceHook 'WSH0'
#define ID_Win32PostServiceHook 'WSH1'
typedef void (NTAPI *PKDBG_PRESERVICEHOOK)(ULONG, PULONG_PTR);
typedef ULONG_PTR (NTAPI *PKDBG_POSTSERVICEHOOK)(ULONG, ULONG_PTR);
extern PKDBG_PRESERVICEHOOK KeWin32PreServiceHook;
extern PKDBG_POSTSERVICEHOOK KeWin32PostServiceHook;
#endif
