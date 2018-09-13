/*++

Copyright (c) 1990-1995,  Microsoft Corporation  All rights reserved.

Module Name:

    cdids.h

Abstract:

    This module contains the resource ID definitions for the Win32
    common dialogs.

Revision History:

--*/



//
//  Include Files.
//

#include <shlobjp.h>




//
//  Constant Declarations.
//

#define IDA_OPENFILE         100
#define IDA_OPENFILEVIEW     101

#define IDC_PARENT           (FCIDM_BROWSERFIRST + 1)
#define IDC_NEWFOLDER        (FCIDM_BROWSERFIRST + 2)
#define IDC_VIEWLIST         (FCIDM_BROWSERFIRST + 3)
#define IDC_VIEWDETAILS      (FCIDM_BROWSERFIRST + 4)
#define IDC_DROPDRIVLIST     (FCIDM_BROWSERFIRST + 5)
#define IDC_REFRESH          (FCIDM_BROWSERFIRST + 6)
#define IDC_PREVIOUSFOLDER   (FCIDM_BROWSERFIRST + 7)

#define DUMMYFILEOPENORD     400

#define FCIDM_FIRST          FCIDM_GLOBALFIRST
#define FCIDM_LAST           FCIDM_GLOBALLAST

#define MH_POPUPS            600

#define MH_ITEMS             (700 - FCIDM_FIRST)
#define MH_TOOLTIPBASE       (MH_ITEMS - (FCIDM_LAST - FCIDM_FIRST))

