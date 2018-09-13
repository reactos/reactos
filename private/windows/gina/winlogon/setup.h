//*************************************************************
//  File name: Setup.h
//
//  Description:  Header file for setup.c
//
//  Also used by videoprt.sys ...
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1991-1996
//  All rights reserved
//
//*************************************************************

//
//  "SetupType" values
//

#define SETUPTYPE_NONE     0
#define SETUPTYPE_FULL     1
#define SETUPTYPE_NOREBOOT 2
#define SETUPTYPE_UPGRADE  4


#define APPNAME_WINLOGON  TEXT("Winlogon")
#define VARNAME_SETUPTYPE TEXT("SetupType")
#define VARNAME_SETUPCMD  TEXT("Cmdline")
#define VARNAME_AUTOLOGON TEXT("AutoAdminLogon")
#define VARNAME_ENABLEDESKTOPSWITCHING TEXT("EnableDesktopSwitching")
#define VARNAME_SETUPINPROGRESS TEXT("SystemSetupInProgress")
#define VARNAME_REPAIR    TEXT("Repair")
#define KEYNAME_SETUP     TEXT("SYSTEM\\Setup")


DWORD
CheckSetupType (
   VOID
   );

BOOL
SetSetupType (
   DWORD type
   );

VOID
ExecuteSetup(
    PTERMINAL pTerm
    );

VOID
FixupRegistry(
    VOID
    );

VOID
CreateLsaStallEvent(
    VOID
    );


VOID
CheckForIncompleteSetup (
   PTERMINAL pTerm
   );


typedef
VOID (WINAPI * REPAIRSTARTMENUITEMS)(
    VOID
    );

VOID
CheckForRepairRequest (void);

//
// post setup wizard types
//

BOOL
CheckForNetAccessWizard (
    PTERMINAL pTerm
    );
