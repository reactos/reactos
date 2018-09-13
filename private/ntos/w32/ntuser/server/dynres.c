/*************************************************************************
*
* dynres.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* WinStation Dynamic display resolution change
*
* History:
* July 25th 1997 v-mohamb Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include "ntuser.h"

#include <winsta.h>
#include <wstmsg.h>

NTSTATUS
DrChangeDisplaySettings(WINSTATIONDORECONNECTMSG *pDoReconnectMessage);

NTSTATUS
DrGetDeviceName(OUT PUNICODE_STRING pustrDeviceName,
                WINSTATIONDORECONNECTMSG *pDoReconnectMessage);
NTSTATUS
DrSetDevMode(WINSTATIONDORECONNECTMSG *pDoReconnectMessage,
             PDEVMODEW pDevmodew);


NTSTATUS
DrChangeDisplaySettings(
    WINSTATIONDORECONNECTMSG *pDoReconnectMessage)
{
    DEVMODEW devmodew;
    UNICODE_STRING ustrDeviceName;
    NTSTATUS Status;


    //BUGBUG Reconnecting at a resolution different than the orignal connection
    // is not functional on Hydra 5
    return STATUS_NOT_IMPLEMENTED;

#if DBG
    DbgPrint("\nDrChangeDisplaySettings - Entering...\n");
#endif

    // Setup DEVMODE for Display settings change

    Status = DrSetDevMode(pDoReconnectMessage, &devmodew);

    if (!NT_SUCCESS(Status))
        return Status;

    // Get device name from reconnect message

    Status = DrGetDeviceName(&ustrDeviceName,pDoReconnectMessage);

    if (!NT_SUCCESS(Status))
        return Status;

    // Change display settings for every desktop

    Status = NtUserChangeDisplaySettings(&ustrDeviceName,
                                         &devmodew,
                                         NULL,
                                         (CDS_RESET | CDS_GLOBAL),
                                         NULL);

    return Status;
}


NTSTATUS
DrGetDeviceName(
    OUT PUNICODE_STRING pustrDeviceName,
    WINSTATIONDORECONNECTMSG *pDoReconnectMessage)
{
    NTSTATUS Status = STATUS_SUCCESS;

    pDoReconnectMessage;

    // Get Device name from reconnect message ( LATTER )
    // for now, just use a static name for device

    RtlInitUnicodeString(pustrDeviceName, DR_RECONNECT_DEVICE_NAMEW);

#if DBG
    DbgPrint("DrGetDeviceName - Device name is : %ws\n", pustrDeviceName->Buffer);
#endif

    return Status;
}

NTSTATUS
DrSetDevMode(
    WINSTATIONDORECONNECTMSG *pDoReconnectMessage,
    PDEVMODEW pDevmodew)
{
    NTSTATUS Status = STATUS_SUCCESS;

    // Initialize unused fields

    RtlZeroMemory(pDevmodew, sizeof(DEVMODEW));

    // Get Display resolution data from reconnect data

    pDevmodew->dmPelsHeight = pDoReconnectMessage->VRes;
    pDevmodew->dmPelsWidth  = pDoReconnectMessage->HRes;
    pDevmodew->dmBitsPerPel = pDoReconnectMessage->ColorDepth;
    pDevmodew->dmFields     = DM_BITSPERPEL  | DM_PELSWIDTH  | DM_PELSHEIGHT;
    pDevmodew->dmSize       = sizeof(DEVMODEW);

    return Status;
}




