/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    helpid.h

Abstract:

    Resource IDs for the System Control Panel Applet

Author:

    Scott Hallock (scotthal) 17-Oct-1997

Revision History:

    15-Oct-1997 scotthal
        Split Help IDs into their own header

--*/
#ifndef _SYSDM_HELPID_H_
#define _SYSDM_HELPID_H_

#define HELP_FILE           TEXT("sysdm.hlp")

#define IDH_HELPFIRST       5000

//
// Help IDs for the General tab
//
#define IDH_GENERAL         (IDH_HELPFIRST + 0000)
#define IDH_PERF            (IDH_HELPFIRST + 1000)
#define IDH_ENV             (IDH_HELPFIRST + 2000)
#define IDH_ENV_EDIT        (IDH_HELPFIRST + 2500)
#define IDH_STARTUP         (IDH_HELPFIRST + 3000)
#define IDH_HWPROFILE       (IDH_HELPFIRST + 4000)
#define IDH_USERPROFILE     (IDH_HELPFIRST + 5000)
#define IDH_HARDWARE        (IDH_HELPFIRST + 6000)
#define IDH_ADVANCED        (IDH_HELPFIRST + 7000)
#define IDH_DLGFIRST        (IDH_HELPFIRST + 3000)
#define IDH_DLG_VIRTUALMEM  (IDH_DLGFIRST + DLG_VIRTUALMEM)

#define IDH_HWP_PROPERTIES_SELECTION_CHECKBOX     9327

#endif // _SYSDM_HELPID_H_
