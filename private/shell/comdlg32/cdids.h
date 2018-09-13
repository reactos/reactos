/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

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

#include <shlobj.h>




//
//  Constant Declarations.
//

#define IDA_OPENFILE         100
#define IDA_OPENFILEVIEW     101
#define IDA_PRINTFOLDER      102

#define IDC_PARENT           (FCIDM_BROWSERFIRST + 1)
#define IDC_NEWFOLDER        (FCIDM_BROWSERFIRST + 2)
#define IDC_VIEWLIST         (FCIDM_BROWSERFIRST + 3)
#define IDC_VIEWDETAILS      (FCIDM_BROWSERFIRST + 4)
#define IDC_DROPDRIVLIST     (FCIDM_BROWSERFIRST + 5)
#define IDC_REFRESH          (FCIDM_BROWSERFIRST + 6)
#define IDC_PREVIOUSFOLDER   (FCIDM_BROWSERFIRST + 7)
#define IDC_JUMPDESKTOP      (FCIDM_BROWSERFIRST + 9)
#define IDC_VIEWMENU         (FCIDM_BROWSERFIRST + 10)
#define IDC_BACK             (FCIDM_BROWSERFIRST + 11)


#define IDC_PLACESBAR_BASE   (FCIDM_BROWSERFIRST  + 100)

#define DUMMYFILEOPENORD     400
#define FONTDLGMMAXES        401

#define FCIDM_FIRST          FCIDM_GLOBALFIRST
#define FCIDM_LAST           FCIDM_GLOBALLAST

#define MH_POPUPS            600

#define MH_ITEMS             (700 - FCIDM_FIRST)
#define MH_TOOLTIPBASE       (MH_ITEMS - (FCIDM_LAST - FCIDM_FIRST))


#define IDB_JUMPDESKTOP      800
