/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, 1992, 1993 Microsoft Corporation
 *
 *  WKTBL2.h
 *  WOW32 kernel API thunks
 *
 *  This file is included into the master thunk table.
 *
--*/
    {W32FUN(UNIMPLEMENTEDAPI,               "DUMMYENTRY",                 MOD_KERNEL, 0)},
    {W32FUN(WK32FatalExit,                  "FatalExit",                  MOD_KERNEL, sizeof(FATALEXIT16))},
    {W32FUN(WK32ExitKernel,                 "ExitKernel",                 MOD_KERNEL, sizeof(EXITKERNEL16))},
    {W32FUN(NOPAPI,                         "WriteOutProfiles",           MOD_KERNEL, 0)},
    {W32FUN(LOCALAPI, /* available */       "MapSL",                      MOD_KERNEL, sizeof(MAPSL16))},
    {W32FUN(LOCALAPI, /* available */       "MapLS",                      MOD_KERNEL, sizeof(MAPLS16))},
    {W32FUN(LOCALAPI, /* available */       "UnMapLS",                    MOD_KERNEL, sizeof(UNMAPLS16))},
    {W32FUN(UNIMPLEMENTED95API,             "OpenFileEx",                 MOD_KERNEL, sizeof(OPENFILEEX16))},
    {W32FUN(UNIMPLEMENTED95API,             "FastAndDirtyGlobalFix",      MOD_KERNEL, sizeof(FASTANDDIRTYGLOBALFIX16))},
    {W32FUN(IT(WritePrivateProfileStruct),  "WritePrivateProfileStruct",  MOD_KERNEL, sizeof(WRITEPRIVATEPROFILESTRUCT16))},

  /*** 0010 ***/
    {W32FUN(IT(GetPrivateProfileStruct),    "GetPrivateProfileStruct",    MOD_KERNEL, sizeof(GETPRIVATEPROFILESTRUCT16))},
    {W32FUN(IT(GetCurrentDirectory),        "GetCurrentDirectory",        MOD_KERNEL, sizeof(GETCURRENTDIRECTORY16))},
    {W32FUN(IT(SetCurrentDirectory),        "SetCurrentDirectory",        MOD_KERNEL, sizeof(SETCURRENTDIRECTORY16))},
    {W32FUN(WK32FindFirstFile,              "FindFirstFile",              MOD_KERNEL, sizeof(FINDFIRSTFILE16))},
    {W32FUN(WK32FindNextFile,               "FindNextFile",               MOD_KERNEL, sizeof(FINDNEXTFILE16))},
    {W32FUN(WK32FindClose,                  "FindClose",                  MOD_KERNEL, sizeof(FINDCLOSE16))},
    {W32FUN(IT(WritePrivateProfileSection), "WritePrivateProfileSection", MOD_KERNEL, sizeof(WRITEPRIVATEPROFILESECTION16))},
    {W32FUN(IT(WriteProfileSection),        "WriteProfileSection",        MOD_KERNEL, sizeof(WRITEPROFILESECTION16))},
    {W32FUN(IT(GetPrivateProfileSection),   "GetPrivateProfileSection",   MOD_KERNEL, sizeof(GETPRIVATEPROFILESECTION16))},
    {W32FUN(IT(GetProfileSection),          "GetProfileSection",          MOD_KERNEL, sizeof(GETPROFILESECTION16))},

  /*** 0020 ***/
    {W32FUN(IT(GetFileAttributes),          "GetFileAttributes",          MOD_KERNEL, sizeof(GETFILEATTRIBUTES16))},
    {W32FUN(IT(SetFileAttributes),          "SetFileAttributes",          MOD_KERNEL, sizeof(SETFILEATTRIBUTES16))},
    {W32FUN(IT(GetDiskFreeSpace),           "GetDiskFreeSpace",           MOD_KERNEL, sizeof(GETDISKFREESPACE16))},
    {W32FUN(UNIMPLEMENTED95API,             "IsPEFormat",                 MOD_KERNEL, sizeof(ISPEFORMAT16))},
    {W32FUN(IT(FileTimeToLocalFileTime),    "FileTimeToLocalFileTime",    MOD_KERNEL, sizeof(FILETIMETOLOCALFILETIME16))},
    {W32FUN(UNIMPLEMENTED95API,             "UniToAnsi",                  MOD_KERNEL, sizeof(UNITOANSI16))},
    {W32FUN(WK32GetVDMPointer32W,           "GetVDMPointer32W",           MOD_KERNEL, sizeof(GETVDMPOINTER32W16))},
    {W32FUN(UNIMPLEMENTED95API,             "CreateThread",               MOD_KERNEL, sizeof(CREATETHREAD16))},
    {W32FUN(WK32ICallProc32W,               "ICallProc32W",               MOD_KERNEL, sizeof(ICALLPROC32W16))},
    {W32FUN(WK32Yield,                      "YIELD",                      MOD_KERNEL, 0)},

  /*** 0030 ***/
    {W32FUN(WK32WaitEvent,                  "WAITEVENT",                  MOD_KERNEL, sizeof(WAITEVENT16))},
    {W32FUN(UNIMPLEMENTEDAPI,               "POSTEVENT",                  MOD_KERNEL, 0)},
    {W32FUN(UNIMPLEMENTEDAPI,               "SETPRIORITY",                MOD_KERNEL, 0)},
    {W32FUN(NOPAPI,                         "LockCurrentTask",            MOD_KERNEL, 0)},
    {W32FUN(UNIMPLEMENTED95API,             "LeaveEnterWin16Lock",        MOD_KERNEL, 0)},
    {W32FUN(IT(RegLoadKey),                 "RegLoadKey32",               MOD_KERNEL, sizeof(REGLOADKEY3216))},
    {W32FUN(IT(RegUnLoadKey),               "RegUnLoadKey32",             MOD_KERNEL, sizeof(REGUNLOADKEY3216))},
    {W32FUN(IT(RegSaveKey),                 "RegSaveKey32",               MOD_KERNEL, sizeof(REGSAVEKEY3216))},
    {W32FUN(UNIMPLEMENTED95API,             "GetWin16Lock",               MOD_KERNEL, 0)},
    {W32FUN(UNIMPLEMENTED95API,             "LoadLibrary32",              MOD_KERNEL, sizeof(LOADLIBRARY3216))},

  /*** 0040 ***/
    {W32FUN(UNIMPLEMENTED95API,             "GetProcAddress32",           MOD_KERNEL, sizeof(GETPROCADDRESS3216))},
    {W32FUN(WK32WOWFindFirst,               "WOWFindFirst",               MOD_KERNEL, sizeof(WOWFINDFIRST16))},
    {W32FUN(WK32WOWFindNext,                "WOWFindNext",                MOD_KERNEL, sizeof(WOWFINDNEXT16))},
    {W32FUN(UNIMPLEMENTED95API,             "CreateWin32Event",           MOD_KERNEL, sizeof(CREATEWIN32EVENT16))},
    {W32FUN(UNIMPLEMENTED95API,             "SetWin32Event",              MOD_KERNEL, sizeof(SETWIN32EVENT16))},
    {W32FUN(WK32WOWLoadModule32,            "WOWLoadModule",              MOD_KERNEL, sizeof(WOWLOADMODULE16))},
    {W32FUN(UNIMPLEMENTED95API,             "ResetWin32Event",            MOD_KERNEL, sizeof(RESETWIN32EVENT16))},
    {W32FUN(WK32WowGetModuleHandle,         "GETMODULEHANDLE",            MOD_KERNEL, sizeof(WOWGETMODULEHANDLE16))},
    {W32FUN(UNIMPLEMENTED95API,             "WaitForSingleObject",        MOD_KERNEL, sizeof(WAITFORSINGLEOBJECT16))},
    {W32FUN(WK32GetModuleFileName,          "GETMODULEFILENAME",          MOD_KERNEL, sizeof(GETMODULEFILENAME16))},

  /*** 0050 ***/
    {W32FUN(UNIMPLEMENTED95API,             "WaitForMultipleObjects",     MOD_KERNEL, sizeof(WAITFORMULTIPLEOBJECTS16))},
    {W32FUN(UNIMPLEMENTED95API,             "GetCurrentThreadID",         MOD_KERNEL, 0)},
    {W32FUN(UNIMPLEMENTED95API,             "SetThreadQueue",             MOD_KERNEL, sizeof(SETTHREADQUEUE16))},
    {W32FUN(UNIMPLEMENTED95API,             "ConvertToGlobalHandle",      MOD_KERNEL, sizeof(CONVERTTOGLOBALHANDLE16))},
    {W32FUN(UNIMPLEMENTED95API,             "GetThreadQueue",             MOD_KERNEL, sizeof(GETTHREADQUEUE16))},
    {W32FUN(UNIMPLEMENTED95API,             "NukeProcess",                MOD_KERNEL, sizeof(NUKEPROCESS16))},
    {W32FUN(UNIMPLEMENTED95API,             "ExitProcess",                MOD_KERNEL, sizeof(EXITPROCESS16))},
    {W32FUN(WK32GetProfileInt,              "GETPROFILEINT",              MOD_KERNEL, sizeof(GETPROFILEINT16))},
    {W32FUN(WK32GetProfileString,           "GETPROFILESTRING",           MOD_KERNEL, sizeof(GETPROFILESTRING16))},
    {W32FUN(WK32WriteProfileString,         "WRITEPROFILESTRING",         MOD_KERNEL, sizeof(WRITEPROFILESTRING16))},

  /*** 0060 ***/
    {W32FUN(UNIMPLEMENTED95API,             "GetCurrentProcessID",        MOD_KERNEL, 0)},
    {W32FUN(UNIMPLEMENTED95API,             "MapHinstLS",                 MOD_KERNEL, 0)},
    {W32FUN(UNIMPLEMENTED95API,             "MapHinstSL",                 MOD_KERNEL, 0)},
    {W32FUN(UNIMPLEMENTED95API,             "CloseWin32Handle",           MOD_KERNEL, sizeof(CLOSEWIN32HANDLE16))},
    {W32FUN(UNIMPLEMENTED95API,             "LoadSystemLibrary32",        MOD_KERNEL, sizeof(LOADSYSTEMLIBRARY3216))},
    {W32FUN(UNIMPLEMENTED95API,             "FreeLibrary32",              MOD_KERNEL, sizeof(FREELIBRARY3216))},
    {W32FUN(UNIMPLEMENTED95API,             "GetModuleFilename32",        MOD_KERNEL, sizeof(GETMODULEFILENAME3216))},
    {W32FUN(UNIMPLEMENTED95API,             "GetModuleHandle32",          MOD_KERNEL, sizeof(GETMODULEHANDLE3216))},
    {W32FUN(NOPAPI,                         "RegisterServiceProcess",     MOD_KERNEL, sizeof(REGISTERSERVICEPROCESS16))},
    {W32FUN(LOCALAPI,                       "ChangeAllocFixedBehaviour",  MOD_KERNEL ,sizeof(CHANGEALLOCFIXEDBEHAVIOUR16))},

  /*** 0070 ***/
    {W32FUN(UNIMPLEMENTED95API,             "InitCB",                     MOD_KERNEL, sizeof(INITCB16))},
    {W32FUN(UNIMPLEMENTED95API,             "GetStdCBLS",                 MOD_KERNEL, sizeof(GETSTDCBLS16))},
    {W32FUN(UNIMPLEMENTED95API,             "GetStdCBSL",                 MOD_KERNEL, sizeof(GETSTDCBSL16))},
    {W32FUN(UNIMPLEMENTED95API,             "GetExistingStdCBLS",         MOD_KERNEL, sizeof(GETEXISTINGSTDCBLS16))},
    {W32FUN(UNIMPLEMENTED95API,             "GetExistingStdCBSL",         MOD_KERNEL, sizeof(GETEXISTINGSTDCBSL16))},
    {W32FUN(UNIMPLEMENTED95API,             "GetFlexCBSL",                MOD_KERNEL, sizeof(GETFLEXCBSL16))},
    {W32FUN(UNIMPLEMENTED95API,             "GetStdCBLSEx",               MOD_KERNEL, sizeof(GETSTDCBLSEX16))},
    {W32FUN(UNIMPLEMENTED95API,             "GetStdCBSLEx",               MOD_KERNEL, sizeof(GETSTDCBSLEX16))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback2",                  MOD_KERNEL, sizeof(CALLBACK216))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback4",                  MOD_KERNEL, sizeof(CALLBACK416))},

  /*** 0080 ***/
    {W32FUN(UNIMPLEMENTED95API,             "Callback6",                  MOD_KERNEL, sizeof(CALLBACK616))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback8",                  MOD_KERNEL, sizeof(CALLBACK816))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback10",                 MOD_KERNEL, sizeof(CALLBACK1016))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback12",                 MOD_KERNEL, sizeof(CALLBACK1216))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback14",                 MOD_KERNEL, sizeof(CALLBACK1416))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback16",                 MOD_KERNEL, sizeof(CALLBACK1616))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback18",                 MOD_KERNEL, sizeof(CALLBACK1816))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback20",                 MOD_KERNEL, sizeof(CALLBACK2016))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback22",                 MOD_KERNEL, sizeof(CALLBACK2216))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback24",                 MOD_KERNEL, sizeof(CALLBACK2416))},

  /*** 0090 ***/
    {W32FUN(UNIMPLEMENTED95API,             "Callback26",                 MOD_KERNEL, sizeof(CALLBACK2616))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback28",                 MOD_KERNEL, sizeof(CALLBACK2816))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback30",                 MOD_KERNEL, sizeof(CALLBACK3016))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback32",                 MOD_KERNEL, sizeof(CALLBACK3216))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback34",                 MOD_KERNEL, sizeof(CALLBACK3416))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback36",                 MOD_KERNEL, sizeof(CALLBACK3616))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback38",                 MOD_KERNEL, sizeof(CALLBACK3816))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback40",                 MOD_KERNEL, sizeof(CALLBACK4016))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback42",                 MOD_KERNEL, sizeof(CALLBACK4216))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback44",                 MOD_KERNEL, sizeof(CALLBACK4416))},

  /*** 0100 ***/
    {W32FUN(UNIMPLEMENTED95API,             "Callback46",                 MOD_KERNEL, sizeof(CALLBACK4616))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback48",                 MOD_KERNEL, sizeof(CALLBACK4816))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback50",                 MOD_KERNEL, sizeof(CALLBACK5016))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback52",                 MOD_KERNEL, sizeof(CALLBACK5216))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback54",                 MOD_KERNEL, sizeof(CALLBACK5416))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback56",                 MOD_KERNEL, sizeof(CALLBACK5616))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback58",                 MOD_KERNEL, sizeof(CALLBACK5816))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback60",                 MOD_KERNEL, sizeof(CALLBACK6016))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback62",                 MOD_KERNEL, sizeof(CALLBACK6216))},
    {W32FUN(UNIMPLEMENTED95API,             "Callback64",                 MOD_KERNEL, sizeof(CALLBACK6416))},

  /*** 0110 ***/
    {W32FUN(WK32WOWKillTask,                "WOWKillTask",                MOD_KERNEL, 0)},
    {W32FUN(WK32WOWFileWrite,               "WOWFileWrite",               MOD_KERNEL, sizeof(WOWFILEWRITE16))},
    {W32FUN(WK32WowGetNextVdmCommand,       "WowGetNextVDMCommand",       MOD_KERNEL, sizeof(WOWGETNEXTVDMCOMMAND16))},
    {W32FUN(WK32WOWFileLock,                "WOWFileLock",                MOD_KERNEL, sizeof(WOWFILELOCK16))},
    {W32FUN(WK32WOWFreeResource,            "WOWFreeResource",            MOD_KERNEL, sizeof(WOWFREERESOURCE16))},
    {W32FUN(WK32WOWOutputDebugString,       "WOWOutputDebugString",       MOD_KERNEL, sizeof(WOWOUTPUTDEBUGSTRING16))},
    {W32FUN(WK32WOWInitTask,                "WOWInitTask",                MOD_KERNEL, sizeof(WOWINITTASK16))},
    {W32FUN(WK32OldYield,                   "OLDYIELD",                   MOD_KERNEL, 0)},
    {W32FUN(WK32WOWFileSetDateTime,         "WOWFileSetDateTime",         MOD_KERNEL, sizeof(WOWFILESETDATETIME16))},
    {W32FUN(WK32WOWFileCreate,              "WOWFileCreate",              MOD_KERNEL, sizeof(WOWFILECREATE16))},

  /*** 0120 ***/
    {W32FUN(WK32DosWowInit,                 "WOWDosWowInit",              MOD_KERNEL, sizeof(WOWDOSWOWINIT16))},
    {W32FUN(WK32CheckUserGdi,               "WOWCheckUserGdi",            MOD_KERNEL, sizeof(WOWCHECKUSERGDI16))},
    {W32FUN(WK32WOWPARTYBYNUMBER,           "WowPartyByNumber",           MOD_KERNEL, sizeof(WOWPARTYBYNUMBER16))},
    {W32FUN(IT(GetShortPathName),           "GetShortPathName",           MOD_KERNEL, sizeof(GETSHORTPATHNAME16))},
    {W32FUN(WK32FindAndReleaseDib,          "FindAndReleaseDib",          MOD_KERNEL, sizeof(FINDANDRELEASEDIB16))},
    {W32FUN(WK32WowReserveHtask,            "WowReserveHtask",            MOD_KERNEL, sizeof(WOWRESERVEHTASK16))},
    {W32FUN(WK32WOWFileSetAttributes,       "WOWFileSetAttributes",       MOD_KERNEL, sizeof(WOWFILESETATTRIBUTES16))},
    {W32FUN(WK32GetPrivateProfileInt,       "GETPRIVATEPROFILEINT",       MOD_KERNEL, sizeof(GETPRIVATEPROFILEINT16))},
    {W32FUN(WK32GetPrivateProfileString,    "GETPRIVATEPROFILESTRING",    MOD_KERNEL, sizeof(GETPRIVATEPROFILESTRING16))},
    {W32FUN(WK32WritePrivateProfileString,  "WRITEPRIVATEPROFILESTRING",  MOD_KERNEL, sizeof(WRITEPRIVATEPROFILESTRING16))},

  /*** 0130 ***/
    {W32FUN(WK32SetCurrentDirectory,        "WOWSetCurrentDirectory",     MOD_KERNEL, 0)},
    {W32FUN(WK32WowWaitForMsgAndEvent,      "WOWWaitForMsgAndEvent",      MOD_KERNEL, sizeof(WOWWAITFORMSGANDEVENT16))},
    {W32FUN(WK32WowMsgBox,                  "WOWMsgBox",                  MOD_KERNEL, sizeof(WOWMSGBOX16))},
    {W32FUN(WK32WOWGetFlatAddressArray,     "WowGetFlatAddressArray",     MOD_KERNEL, 0)},
    {W32FUN(WK32GetCurrentDate,             "WOWGetCurrentDate",          MOD_KERNEL, 0)},
    {W32FUN(WK32DeviceIOCTL,                "WOWDeviceIOCTL",             MOD_KERNEL, sizeof(WOWDEVICEIOCTL16))},
    {W32FUN(WK32GetDriveType,               "GETDRIVETYPE",               MOD_KERNEL, sizeof(GETDRIVETYPE16))},
    {W32FUN(WK32WOWFileGetDateTime,         "WOWFileGetDateTime",         MOD_KERNEL, sizeof(WOWFILEGETDATETIME16))},
    {W32FUN(WK32SetAppCompatFlags,          "SetAppCompatFlags",          MOD_KERNEL, sizeof(SETAPPCOMPATFLAGS16))},
    {W32FUN(WK32RegisterShellWindowHandle,  "WOWRegisterShellWindowHandle",MOD_KERNEL,sizeof(WOWREGISTERSHELLWINDOWHANDLE16))},

  /*** 0140 ***/
    {W32FUN(WK32FreeLibrary32W,             "FreeLibrary32W",             MOD_KERNEL, sizeof(FREELIBRARY32W16))},
    {W32FUN(WK32GetProcAddress32W,          "GetProcAddress32W",          MOD_KERNEL, sizeof(GETPROCADDRESS32W16))},
    {W32FUN(LOCALAPI, /* available */       "GetProfileSectionNames",     MOD_KERNEL, sizeof(GETPROFILESECTIONNAMES16))},
    {W32FUN(IT(GetPrivateProfileSectionNames), "GetPrivateProfileSectionNames", MOD_KERNEL, sizeof(GETPRIVATEPROFILESECTIONNAMES16))},
    {W32FUN(IT(CreateDirectory),            "CreateDirectory",            MOD_KERNEL, sizeof(CREATEDIRECTORY16))},
    {W32FUN(IT(RemoveDirectory),            "RemoveDirectory",            MOD_KERNEL, sizeof(REMOVEDIRECTORY16))},
    {W32FUN(IT(DeleteFile),                 "DeleteFile",                 MOD_KERNEL, sizeof(DELETEFILE16))},
    {W32FUN(IT(SetLastError),               "SetLastError",               MOD_KERNEL, sizeof(SETLASTERROR16))},
    {W32FUN(IT(GetLastError),               "GetLastError",               MOD_KERNEL, 0)},
    {W32FUN(IT(GetVersionEx),               "GetVersionEx",               MOD_KERNEL, sizeof(GETVERSIONEX16))},

  /*** 0150 ***/
    {W32FUN(WK32DirectedYield,              "DIRECTEDYIELD",              MOD_KERNEL, sizeof(DIRECTEDYIELD16))},
    {W32FUN(WK32WOWFileRead,                "WOWFileRead",                MOD_KERNEL, sizeof(WOWFILEREAD16))},
    {W32FUN(WK32WOWFileLSeek,               "WOWFileLSeek",               MOD_KERNEL, sizeof(WOWFILELSEEK16))},
    {W32FUN(WK32WOWKernelTrace,             "WOWKernelTrace",             MOD_KERNEL, sizeof(WOWKERNELTRACE16))},
    {W32FUN(WK32LoadLibraryEx32W,           "LoadLibraryEx32W",           MOD_KERNEL, sizeof(LOADLIBRARYEX32W16))},
    {W32FUN(WK32WOWQueryPerformanceCounter, "WOWQueryPerformanceCounter", MOD_KERNEL, sizeof(WOWQUERYPERFORMANCECOUNTER16))},
    {W32FUN(WK32WowCursorIconOp,            "WowCursorIconOp",            MOD_KERNEL, sizeof(WOWCURSORICONOP16))},
    {W32FUN(WK32WowFailedExec,              "WOWFailedExec",              MOD_KERNEL, 0)},
    {W32FUN(WK32WOWGetFastAddress,          "WOWGetFastAddress",          MOD_KERNEL, 0)},
    {W32FUN(WK32WowCloseComPort,            "WowCloseComPort",            MOD_KERNEL, sizeof(WOWCLOSECOMPORT16))},

  /*** 0160 ***/
    {W32FUN(UNIMPLEMENTED95API,             "Local32Init",                MOD_KERNEL, sizeof(LOCAL32INIT16))},
    {W32FUN(UNIMPLEMENTED95API,             "Local32Alloc",               MOD_KERNEL, sizeof(LOCAL32ALLOC16))},
    {W32FUN(UNIMPLEMENTED95API,             "Local32Realloc",             MOD_KERNEL, sizeof(LOCAL32REALLOC16))},
    {W32FUN(UNIMPLEMENTED95API,             "Local32Free",                MOD_KERNEL, sizeof(LOCAL32FREE16))},
    {W32FUN(UNIMPLEMENTED95API,             "Local32Translate",           MOD_KERNEL, sizeof(LOCAL32TRANSLATE16))},
    {W32FUN(UNIMPLEMENTED95API,             "Local32Size",                MOD_KERNEL, sizeof(LOCAL32SIZE16))},
    {W32FUN(UNIMPLEMENTED95API,             "Local32ValidHandle",         MOD_KERNEL, sizeof(LOCAL32VALIDHANDLE16))},
    {W32FUN(IT(RegEnumKey),                 "RegEnumKey32",               MOD_KERNEL, sizeof(REGENUMKEY3216))},
    {W32FUN(IT(RegOpenKey),                 "RegOpenKey32",               MOD_KERNEL, sizeof(REGOPENKEY3216))},
    {W32FUN(IT(RegCreateKey),               "RegCreateKey32",             MOD_KERNEL, sizeof(REGCREATEKEY3216))},

  /*** 0170 ***/
    {W32FUN(IT(RegDeleteKey),               "RegDeleteKey32",             MOD_KERNEL, sizeof(REGDELETEKEY3216))},
    {W32FUN(IT(RegCloseKey),                "RegCloseKey32",              MOD_KERNEL, sizeof(REGCLOSEKEY3216))},
    {W32FUN(IT(RegSetValue),                "RegSetValue32",              MOD_KERNEL, sizeof(REGSETVALUE3216))},
    {W32FUN(IT(RegDeleteValue),             "RegDeleteValue32",           MOD_KERNEL, sizeof(REGDELETEVALUE3216))},
    {W32FUN(IT(RegEnumValue),               "RegEnumValue32",             MOD_KERNEL, sizeof(REGENUMVALUE3216))},
    {W32FUN(IT(RegQueryValue),              "RegQueryValue32",            MOD_KERNEL, sizeof(REGQUERYVALUE3216))},
    {W32FUN(IT(RegQueryValueEx),            "RegQueryValueEx32",          MOD_KERNEL, sizeof(REGQUERYVALUEEX3216))},
    {W32FUN(IT(RegSetValueEx),              "RegSetValueEx32",            MOD_KERNEL, sizeof(REGSETVALUEEX3216))},
    {W32FUN(IT(RegFlushKey),                "RegFlushKey32",              MOD_KERNEL, sizeof(REGFLUSHKEY3216))},
    {W32FUN(UNIMPLEMENTED95API,             "ComputeObjectOwner",         MOD_KERNEL, sizeof(COMPUTEOBJECTOWNER16))},

  /*** 0180 ***/
    {W32FUN(UNIMPLEMENTED95API,             "Local32GetSel",              MOD_KERNEL, sizeof(LOCAL32GETSEL16))},
    {W32FUN(UNIMPLEMENTED95API,             "MapProcessHandle",           MOD_KERNEL, sizeof(MAPPROCESSHANDLE16))},
    {W32FUN(UNIMPLEMENTED95API,             "InvalidateNLSCache",         MOD_KERNEL, 0)},
    {W32FUN(WK32WowDelFile,                 "WowDelFile",                 MOD_KERNEL, sizeof(WOWDELFILE16))},
    {W32FUN(WK32VirtualAlloc,               "VirtualAlloc",               MOD_KERNEL, sizeof(VIRTUALALLOC16))},
    {W32FUN(WK32VirtualFree,                "VirtualFree",                MOD_KERNEL, sizeof(VIRTUALFREE16))},
    {W32FUN(UNIMPLEMENTEDAPI,               "VirtualLock",                MOD_KERNEL, sizeof(VIRTUALLOCK16))},
    {W32FUN(UNIMPLEMENTEDAPI,               "VirtualUnLock",              MOD_KERNEL, sizeof(VIRTUALUNLOCK16))},
    {W32FUN(WK32GlobalMemoryStatus,         "GlobalMemoryStatus",         MOD_KERNEL, sizeof(GLOBALMEMORYSTATUS16))},
    {W32FUN(WK32WOWGetFastCbRetAddress,     "WOWGetFastCbRetAddress",     MOD_KERNEL, 0)},

  /*** 0190 ***/
    {W32FUN(WK32WOWGetTableOffsets,         "WOWGetTableOffsets",         MOD_KERNEL, sizeof(WOWGETTABLEOFFSETS16))},
    {W32FUN(WK32KillRemoteTask,             "WowKillRemoteTask",          MOD_KERNEL, 0)},
    {W32FUN(WK32WOWNotifyWOW32,             "WOWNotifyWOW32",             MOD_KERNEL, sizeof(WOWNOTIFYWOW3216))},
    {W32FUN(WK32WOWFileOpen,                "WOWFileOpen",                MOD_KERNEL, sizeof(WOWFILEOPEN16))},
    {W32FUN(WK32WOWFileClose,               "WOWFileClose",               MOD_KERNEL, sizeof(WOWFILECLOSE16))},
    {W32FUN(WK32WowSetIdleHook,             "WOWSetIdleHook",             MOD_KERNEL, 0)},
    {W32FUN(WU32SysErrorBox,                "SysErrorBox",                MOD_KERNEL, sizeof(KSYSERRORBOX16))},
    {W32FUN(WK32WowIsKnownDLL,              "WowIsKnownDLL",              MOD_KERNEL, sizeof(WOWISKNOWNDLL16))},
    {W32FUN(WK32WowDdeFreeHandle,           "WowDdeFreeHandle",           MOD_KERNEL, sizeof(WOWDDEFREEHANDLE16))},
    {W32FUN(WK32WOWFileGetAttributes,       "WOWFileGetAttributes",       MOD_KERNEL, sizeof(WOWFILEGETATTRIBUTES16))},

  /*** 0200 ***/
    {W32FUN(WK32SetDefaultDrive,            "WOWSetDefaultDrive",         MOD_KERNEL, 0)},
    {W32FUN(WK32GetCurrentDirectory,        "WOWGetCurrentDirectory",     MOD_KERNEL, 0)},
    {W32FUN(WK32GetProductName,             "GetProductName",             MOD_KERNEL, sizeof(GETPRODUCTNAME16))},
    {W32FUN(NOPAPI,                         "IsSafeMode",                 MOD_KERNEL, 0)},
    {W32FUN(WK32WOWLFNEntry,                "WOWLFNEntry",                MOD_KERNEL, sizeof(WOWLFNFRAMEPTR16))},
    {W32FUN(WK32WowShutdownTimer,           "WowShutdownTimer",           MOD_KERNEL, sizeof(WOWSHUTDOWNTIMER16))},
    {W32FUN(WK32WowTrimWorkingSet,          "WowTrimWorkingSet",          MOD_KERNEL, 0)},
    #ifdef FE_SB //add GetSystemDefaultLangID()
    {W32FUN(IT(GetSystemDefaultLangID),     "GetSystemDefaultLangID",     MOD_KERNEL, 0)},
    #endif
    {W32FUN(WK32TermsrvGetWindowsDir,       "TermsrvGetWindowsDir",       MOD_KERNEL,sizeof(TERMSRVGETWINDIR16))},

