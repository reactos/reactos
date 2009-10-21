/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kddll/kddll.h
 * PURPOSE:         Base definitions for the kernel debugger.
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@ewactos.org)
 */

#define NOEXTAPI
#include <ntddk.h>
#define NDEBUG
#include <halfuncs.h>
#include <stdio.h>
#include <stdlib.h>
#include <debug.h>
#include "arc/arc.h"
#include "windbgkd.h"

#include <wdbgexts.h>
#include <ioaccess.h> /* port intrinsics */

typedef UCHAR BYTE, *PBYTE;

typedef ULONG (*DBGRNT)(const char *Format, ...);
extern DBGRNT KdpDbgPrint;

typedef enum
{
    KDP_PACKET_RECEIVED = 0,
    KDP_PACKET_TIMEOUT = 1,
    KDP_PACKET_RESEND = 2
} KDP_STATUS;

