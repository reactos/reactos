/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/haltypes.h
 * PURPOSE:         Prototypes for Boot Video Driver not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _INBVFUNCS_H
#define _INBVFUNCS_H

/* DEPENDENCIES **************************************************************/

/* FUNCTION TYPES ************************************************************/

/* PROTOTYPES ****************************************************************/

VOID
NTAPI
InbvAcquireDisplayOwnership(VOID);

BOOLEAN
NTAPI
InbvCheckDisplayOwnership(VOID);

BOOLEAN
NTAPI
InbvDisplayString(
    IN PCHAR String
);

VOID
NTAPI
InbvEnableBootDriver(
    IN BOOLEAN Enable
);

BOOLEAN
NTAPI
InbvEnableDisplayString(
    IN BOOLEAN Enable
);

VOID
NTAPI
InbvInstallDisplayStringFilter(
    IN PVOID Unknown
);

BOOLEAN
NTAPI
InbvIsBootDriverInstalled(VOID);

VOID
NTAPI
InbvNotifyDisplayOwnershipLost(
    IN PVOID Callback
);

BOOLEAN
NTAPI
InbvResetDisplay(VOID);

VOID
NTAPI
InbvSetScrollRegion(
    IN ULONG Left,
    IN ULONG Top,
    IN ULONG Width,
    IN ULONG Height
);

VOID
NTAPI
InbvSetTextColor(
    IN ULONG Color
);

VOID
NTAPI
InbvSolidColorFill(
    IN ULONG Left,
    IN ULONG Top,
    IN ULONG Width,
    IN ULONG Height,
    IN ULONG Color
);

VOID
NTAPI
VidCleanUp(VOID);

BOOLEAN
NTAPI
VidResetDisplay(VOID);

BOOLEAN
NTAPI
VidIsBootDriverInstalled(VOID);

#endif
