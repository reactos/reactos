/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WSHTBL2.h
 *  WOW32 16-bit SHELL API tables
 *
 *  History:
 *  Created 14-April-1992 by Chandan Chauhan (ChandanC)
--*/

    {W32FUN(UNIMPLEMENTEDAPI,          "DUMMYENTRY",          MOD_SHELL,  0)},
    {W32FUN(WS32RegOpenKey,            "RegOpenKey",          MOD_SHELL,  sizeof(REGOPENKEY16))},
    {W32FUN(WS32RegCreateKey,          "RegCreateKey",        MOD_SHELL,  sizeof(REGCREATEKEY16))},
    {W32FUN(WS32RegCloseKey,           "RegCloseKey",         MOD_SHELL,  sizeof(REGCLOSEKEY16))},
    {W32FUN(WS32RegDeleteKey,          "RegDeleteKey",        MOD_SHELL,  sizeof(REGDELETEKEY16))},
    {W32FUN(WS32RegSetValue,           "RegSetValue",         MOD_SHELL,  sizeof(REGSETVALUE16))},
    {W32FUN(WS32RegQueryValue,         "RegQueryValue",       MOD_SHELL,  sizeof(REGQUERYVALUE16))},
    {W32FUN(WS32RegEnumKey,            "RegEnumKey",          MOD_SHELL,  sizeof(REGENUMKEY16))},
    {W32FUN(UNIMPLEMENTEDAPI,          "",                    MOD_SHELL,  0)},
    {W32FUN(WS32DragAcceptFiles,       "DragAcceptFiles",     MOD_SHELL,  sizeof(DRAGACCEPTFILES16))},

  /*** 0010 ***/
    {W32FUN(UNIMPLEMENTEDAPI,          "",                    MOD_SHELL,  0)},
    {W32FUN(WS32DragQueryFile,         "DragQueryFile",       MOD_SHELL,  sizeof(DRAGQUERYFILE16))},
    {W32FUN(WS32DragFinish,            "DragFinish",          MOD_SHELL,  sizeof(DRAGFINISH16))},
    {W32FUN(LOCALAPI,                  "DragQueryPoint",      MOD_SHELL,  sizeof(DRAGQUERYPOINT16))},
    {W32FUN(UNIMPLEMENTEDAPI,          "",                    MOD_SHELL,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,          "",                    MOD_SHELL,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,          "",                    MOD_SHELL,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,          "",                    MOD_SHELL,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,          "",                    MOD_SHELL,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,          "",                    MOD_SHELL,  0)},

  /*** 0020 ***/
    {W32FUN(WS32ShellExecute,          "ShellExecute",        MOD_SHELL,  sizeof(SHELLEXECUTE16))},
    {W32FUN(WS32FindExecutable,        "FindExecutable",      MOD_SHELL,  sizeof(FINDEXECUTABLE16))},
    {W32FUN(WS32ShellAbout,            "ShellAbout",          MOD_SHELL,  sizeof(SHELLABOUT16))},
    {W32FUN(UNIMPLEMENTEDAPI,          "",                    MOD_SHELL,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,          "",                    MOD_SHELL,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,          "",                    MOD_SHELL,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,          "",                    MOD_SHELL,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,          "",                    MOD_SHELL,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,          "",                    MOD_SHELL,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,          "",                    MOD_SHELL,  0)},

  /*** 0030 ***/
    {W32FUN(UNIMPLEMENTEDAPI,          "",                    MOD_SHELL,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,          "",                    MOD_SHELL,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,          "WCI",                 MOD_SHELL,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,          "ABOUTDLGPROC",        MOD_SHELL,  0)},
    {W32FUN(WS32ExtractIcon,           "ExtractIcon",         MOD_SHELL,  sizeof(EXTRACTICON16))},
    {W32FUN(UNIMPLEMENTEDAPI,          "",                    MOD_SHELL,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,          "ExtractAssociatedIcon",MOD_SHELL, sizeof(EXTRACTASSOCIATEDICON16))},
    {W32FUN(WS32DoEnvironmentSubst,    "DoEnvironmentSubst",  MOD_SHELL,  sizeof(DOENVIRONMENTSUBST16))},
    {W32FUN(UNIMPLEMENTEDAPI,          "FINDENVIRONMENTSTRING",MOD_SHELL, 0)},
    {W32FUN(UNIMPLEMENTEDAPI,          "INTERNALEXTRACTICON", MOD_SHELL,  0)},

  /*** 0040 ***/
    {W32FUN(UNIMPLEMENTED95API,        "ExtractIconEx",       MOD_SHELL,  sizeof(EXTRACTICONEX16))},
    {W32FUN(UNIMPLEMENTEDAPI,          "HERETHARBETYGARS",    MOD_SHELL,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,          "FINDEXEDLGPROC",      MOD_SHELL,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,          "REGISTERSHELLHOOK",   MOD_SHELL,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,          "SHELLHOOKPROC",       MOD_SHELL,  0)},
    {W32FUN(UNIMPLEMENTED95API,        "RestartDialog",       MOD_SHELL,  sizeof(RESTARTDIALOG16))},
    {W32FUN(UNIMPLEMENTED95API,        "PickIconDlg",         MOD_SHELL,  sizeof(PICKICONDLG16))},
    {W32FUN(UNIMPLEMENTED95API,        "DriveType",           MOD_SHELL,  sizeof(DRIVETYPE16))},
    {W32FUN(UNIMPLEMENTED95API,        "SH16to32DriveIOCtl",  MOD_SHELL,  sizeof(SH16TO32DRIVEIOCTL16))},
    {W32FUN(UNIMPLEMENTED95API,        "SH16to32Int2526",     MOD_SHELL,  sizeof(SH16TO32INT252616))},

  /*** 0050 ***/
    {W32FUN(UNIMPLEMENTED95API,        "SHGetFileInfo",       MOD_SHELL,  sizeof(SHGETFILEINFO16))},
    {W32FUN(UNIMPLEMENTED95API,        "SHFormatDrive",       MOD_SHELL,  sizeof(SHFORMATDRIVE16))},
    {W32FUN(UNIMPLEMENTED95API,        "SHCheckDrive",        MOD_SHELL,  sizeof(SHCHECKDRIVE16))},
    {W32FUN(UNIMPLEMENTED95API,        "_RunDllCheckDrive",   MOD_SHELL,  sizeof(_RUNDLLCHECKDRIVE16))},
