/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vrdefld.h

Abstract:

    Contains offsets in VDM redir code segment for deferred load address info

Author:

    Richard L Firth (rfirth) 21-Oct-1992

Revision History:

--*/

/* XLATOFF */
#include <packon.h>
/* XLATON */

typedef struct _VDM_LOAD_INFO { /* */
    DWORD   DlcWindowAddr;
    BYTE    VrInitialized;
} VDM_LOAD_INFO;

/* XLATOFF */
#include <packoff.h>
/* XLATON */
