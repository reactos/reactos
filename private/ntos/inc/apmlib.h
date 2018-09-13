/*++

Module Name:

    apmlib.h

Abstract:

    Interfaces used for apm support, setup, etc.

Author:

Revision History:

--*/

BOOLEAN IsSystemACPI();

BOOLEAN IsApmActive();

ULONG   IsApmPresent();

#define APM_NOT_PRESENT             0
#define APM_PRESENT_BUT_NOT_USABLE  1
#define APM_ON_GOOD_LIST            2
#define APM_NEUTRAL                 3
#define APM_ON_BAD_LIST             4


