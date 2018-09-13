/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    exdata.c

Abstract:

    This module contains the global read/write data for the I/O system.

Author:

    Ken Reneris (kenr)

Revision History:


--*/

#include "exp.h"

//
// Executive callbacks.
//

PCALLBACK_OBJECT ExCbSetSystemTime;
PCALLBACK_OBJECT ExCbSetSystemState;
PCALLBACK_OBJECT ExCbPowerState;

#ifdef _PNP_POWER_

//
// Work Item to scan SystemInformation levels
//

WORK_QUEUE_ITEM ExpCheckSystemInfoWorkItem;
LONG            ExpCheckSystemInfoBusy;
KSPIN_LOCK      ExpCheckSystemInfoLock;

#endif


//
// Pageable data
//

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGE")
#endif

#ifdef _PNP_POWER_

WCHAR ExpWstrSystemInformation[] = L"Control\\System Information";
WCHAR ExpWstrSystemInformationValue[] = L"Value";

#endif

//
// Initialization time data
//

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INIT")
#endif

WCHAR ExpWstrCallback[] = L"\\Callback";

EXP_INITIALIZE_GLOBAL_CALLBACKS  ExpInitializeCallback[] = {
    &ExCbSetSystemTime,             L"\\Callback\\SetSystemTime",
    &ExCbSetSystemState,            L"\\Callback\\SetSystemState",
    &ExCbPowerState,                L"\\Callback\\PowerState",
    NULL,                           NULL
};
