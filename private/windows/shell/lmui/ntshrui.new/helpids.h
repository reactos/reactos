//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       helpids.h
//
//  Contents:   Help context identifiers
//
//  History:    13-Sep-95 BruceFo  Created
//
//--------------------------------------------------------------------------

#define HC_OK                       1
#define HC_CANCEL                   2
#define HC_SHARE_SHARENAME          3
#define HC_SHARE_COMMENT            4
#define HC_SHARE_MAXIMUM            5
#define HC_SHARE_ALLOW              6
#define HC_SHARE_ALLOW_VALUE        7
#define HC_SHARE_PERMISSIONS        8
#define HC_SHARE_NOTSHARED          9
#define HC_SHARE_SHAREDAS           10
#define HC_SHARE_SHARENAME_COMBO    11
#define HC_SHARE_REMOVE             12
#define HC_SHARE_NEWSHARE           13
#define HC_SHARE_LIMIT              14

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// The following are help ids for the ACL editor

// stolen from \nt\private\net\ui\common\h\uihelp.h
#define HC_UI_BASE              7000
#define HC_UI_SHELL_BASE        (HC_UI_BASE+10000)

// stolen from \nt\private\net\ui\shellui\h\helpnums.h
#define HC_NTSHAREPERMS              11 // Main share perm dialog
// The following four have to be consecutive
#define HC_SHAREADDUSER              12 // Share perm add dlg
#define HC_SHAREADDUSER_LOCALGROUP   13 // Share perm add->Members
#define HC_SHAREADDUSER_GLOBALGROUP  14 // Share perm add->Members
#define HC_SHAREADDUSER_FINDUSER     15 // Share perm add->FindUser
