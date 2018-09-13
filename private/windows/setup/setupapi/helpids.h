/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name:

    helpids.h

Abstract:

    Private header file defining help id's for dialogs.

Author:

    Jamie Hunter (jamiehun) Aug 20 1998

Revision History:

--*/

#define IDH_NOHELP  ((DWORD) -1) // Disables Help for a control (for help compiles)

//
// Main property page on the Resources tab
// Created 5/11/98 by WGruber NTUA, JamieHun NTDEV
// also see DevResHelpIDs in DevRes.c
// dialog IDD_DEF_DEVRESOURCE_PROP
//

#define IDH_DEVMGR_RESOURCES_SETTINGS   2001100
#define IDH_DEVMGR_RESOURCES_BASEDON    2001110
#define IDH_DEVMGR_RESOURCES_CHANGE     2001120
#define IDH_DEVMGR_RESOURCES_AUTO       2001130
#define IDH_DEVMGR_RESOURCES_CONFLICTS  2001140
#define IDH_DEVMGR_RESOURCES_PARENT     2001150
#define IDH_DEVMGR_RESOURCES_SETMANUALLY 2001160

//
// Edit property page invoked from Resources tab
// Created 5/11/98 by WGruber NTUA, JamieHun NTDEV
// also see EditResHelpIDs in DevRes1.c
// dialog IDD_EDIT_RESOURCE
//

#define IDH_DEVMGR_RESOURCES_EDIT_VALUE 2100100
#define IDH_DEVMGR_RESOURCES_EDIT_INFO  2100110

