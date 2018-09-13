/*
**------------------------------------------------------------------------------
** Module:  Disk Space Cleanup Property Sheets
** File:    dmgrdlg.h
**
** Purpose: Implements Disk Space Cleanup Dialog Property Sheets
** Notes:   
** Mod Log: Created by Jason Cobb (2/97)
**
** Copyright (c)1997 Microsoft Corporation, All Rights Reserved
**------------------------------------------------------------------------------
*/
#ifndef DMGRDLG_H
#define DMGRDLG_H

/*
**------------------------------------------------------------------------------
** Project include files
**------------------------------------------------------------------------------
*/
#ifndef COMMON_H
   #include "common.h"
#endif


/*
**------------------------------------------------------------------------------
** Global function prototypes
**------------------------------------------------------------------------------
*/
DWORD 
DisplayCleanMgrProperties(
	HWND	hWnd,
	LPARAM	lParam
	);

INT_PTR CALLBACK
DiskCleanupManagerProc(
    HWND hDlg, 
    UINT uMessage, 
    WPARAM wParam, 
    LPARAM lParam
    );

//void 
//CleanupMgrUpdateUI(
//    HWND hDlg
//    );

HWND
GetDiskCleanupManagerWindowHandle(
    void
    );

#endif // DMGRDLG_H
/*
**------------------------------------------------------------------------------
**  End of file
**------------------------------------------------------------------------------
*/
