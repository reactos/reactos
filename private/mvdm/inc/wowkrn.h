/*++ BUILD Version: 0002
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WOWKRN.H
 *  16-bit Kernel API argument structures
 *
 *  History:
 *  Created 02-Feb-1991 by Jeff Parsons (jeffpar)
 *  01-May-91 Matt Felton (mattfe) added Private Callback CHECKLOADMODULEDRV
--*/


/* Kernel API IDs -- loosely based on kernel export ordinals, attempting to keep the table size
 * down.
 */

#define FUN_FATALEXIT                    1   //
#define FUN_EXITKERNEL                   2   // Internal
#define FUN_WRITEOUTPROFILES             3   // really 315 export 3 GetVersion not thunked
#define FUN_MAPSL                        4   // really 357 export 4 LocalInit not thunked
#define FUN_MAPLS                        5   // really 358 export 5 LocalAlloc not thunked
#define FUN_UNMAPLS                      6   // really 359 export 6 LocalReAlloc not thunked
#define FUN_OPENFILEEX                   7   // really 360 export 7 LocalFree not thunked
#define FUN_FASTANDDIRTYGLOBALFIX        8   // really 365 export 8 LocalLock not thunked
#define FUN_WRITEPRIVATEPROFILESTRUCT    9   // really 406 export 9 LocalUnlock not thunked
#define FUN_GETPRIVATEPROFILESTRUCT      10  // really 407 export 10 LocalSize not thunked
#define FUN_GETCURRENTDIRECTORY          11  // really 411 export 11 LocalHandle not thunked
#define FUN_SETCURRENTDIRECTORY          12  // really 412 export 12 LocalFlags not thunked
#define FUN_FINDFIRSTFILE                13  // really 413 export 13 LocalCompact not thunked
#define FUN_FINDNEXTFILE                 14  // really 414 export 14 LocalNotify not thunked
#define FUN_FINDCLOSE                    15  // really 415 export 15 GlobalAlloc not thunked
#define FUN_WRITEPRIVATEPROFILESECTION   16  // really 416 export 16 GlobalReAlloc not thunked
#define FUN_WRITEPROFILESECTION          17  // really 417 export 17 GlobalFree not thunked
#define FUN_GETPRIVATEPROFILESECTION     18  // really 418 export 18 GlobalLock not thunked
#define FUN_GETPROFILESECTION            19  // really 419 export 19 GlobalUnlock not thunked
#define FUN_GETFILEATTRIBUTES            20  // really 420 export 20 GlobalSize not thunked
#define FUN_SETFILEATTRIBUTES            21  // really 421 export 21 GlobalHandle not thunked
#define FUN_GETDISKFREESPACE             22  // really 422 export 22 GlobalFlags not thunked
#define FUN_ISPEFORMAT                   23  // really 431 export 23 LockSegment not thunked
#define FUN_FILETIMETOLOCALFILETIME      24  // really 432 export 24 UnlockSegment not thunked
#define FUN_UNITOANSI                    25  // really 434 export 25 GlobalCompact not thunked
#define FUN_GETVDMPOINTER32W             26  // really 516 export 26 GlobalFreeAll not thunked
#define FUN_CREATETHREAD                 27  // really 441 export 27 GetModuleName not thunked
#define FUN_ICALLPROC32W                 28  // really 517 export 28 GlobalMasterHandle not thunked
#define FUN_YIELD                        29  //
#define FUN_WAITEVENT                    30  // Internal
#define FUN_POSTEVENT                    31  // Internal
#define FUN_SETPRIORITY                  32  // Internal
#define FUN_LOCKCURRENTTASK              33  // Internal
#define FUN_LEAVEENTERWIN16LOCK          34  // really 447 export 34 formerly SetTaskQueue
#define FUN_REGLOADKEY32                 35  // really 232 export 35 GetTaskQueue not thunked
#define FUN_REGUNLOADKEY32               36  // really 233 export 36 GetCurrentTask not thunked
#define FUN_REGSAVEKEY32                 37  // really 234 export 37 GetCurrentPDB not thunked
#define FUN_GETWIN16LOCK                 38  // really 449 export 38 formerly SetTaskSignalProc
#define FUN_LOADLIBRARY32                39  // really 452 export 39 formerly SetTaskSwitchProc
#define FUN_GETPROCADDRESS32             40  // really 453 export 40 formerly SetTaskInterchange
#define FUN_WOWFINDFIRST                 41  // WOW internal export 41 EnableDOS not thunked
#define FUN_WOWFINDNEXT                  42  // WOW internal export 42 DisableDOS not thunked
#define FUN_CREATEWIN32EVENT             43  // really 457 export 43 formerly IsScreenGrab
#define FUN_SETWIN32EVENT                44  // really 458 export 44 formerly BuildPDB
#define FUN_WOWLOADMODULE                45  // reusing LoadModule export, not thunked to wow32
#define FUN_RESETWIN32EVENT              46  // really 459 export 46 FreeModule not thunked
#define FUN_GETMODULEHANDLE              47  //
#define FUN_WAITFORSINGLEOBJECT          48  // really 460 export 48 GetModuleUsage not thunked
#define FUN_GETMODULEFILENAME            49  //
#define FUN_WAITFORMULTIPLEOBJECTS       50  // really 461 export 50 GetProcAddress not thunked
#define FUN_GETCURRENTTHREADID           51  // really 462 export 51 MakeProcInstance not thunked
#define FUN_SETTHREADQUEUE               52  // really 463 export 52 FreeProcInstance not thunked
#define FUN_CONVERTTOGLOBALHANDLE        53  // really 476 export 53 CallProcInstance not thunked
#define FUN_GETTHREADQUEUE               54  // really 464 export 54 GetInstanceData not thunked
#define FUN_NUKEPROCESS                  55  // really 465 export 55 Catch not thunked
#define FUN_EXITPROCESS                  56  // really 466 export 56 Throw not thunked
#define FUN_GETPROFILEINT                57  //
#define FUN_GETPROFILESTRING             58  //
#define FUN_WRITEPROFILESTRING           59  //
#define FUN_GETCURRENTPROCESSID          60  // really 471 export 60 FindResource not thunked
#define FUN_MAPHINSTLS                   61  // really 472 export 61 LoadResource not thunked
#define FUN_MAPHINSTSL                   62  // really 473 export 62 LockResource not thunked
#define FUN_CLOSEWIN32HANDLE             63  // really 474 export 63 FreeResource not thunked
#define FUN_LOADSYSTEMLIBRARY32          64  // really 482 export 64 AccessResource not thunked
#define FUN_FREELIBRARY32                65  // really 486 export 65 ...Resource not thunked
#define FUN_GETMODULEFILENAME32          66  // really 487 export 66 AllocResource not thunked
#define FUN_GETMODULEHANDLE32            67  // really 488 export 67 SetResourceHandler not thunked
#define FUN_REGISTERSERVICEPROCESS       68  // really 491 export 68 InitAtomTable not thunked
#define FUN_CHANGEALLOCFIXEDBEHAVIOUR    69  // really 501 export 69 FindAtom not thunked
#define FUN_INITCB                       70  // really 560 export 70 AddAtom not thunked
#define FUN_GETSTDCBLS                   71  // really 561 export 71 DeleteAtom not thunked
#define FUN_GETSTDCBSL                   72  // really 562 export 72 GetAtomName not thunked
#define FUN_GETEXISTINGSTDCBLS           73  // really 563 export 73 GetAtomHandle not thunked
#define FUN_GETEXISTINGSTDCBSL           74  // really 564 export 74 OpenFile not thunked
#define FUN_GETFLEXCBSL                  75  // really 566 export 75 OpenPathName not thunked
#define FUN_GETSTDCBLSEX                 76  // really 567 export 76 DeletePathName not thunked
#define FUN_GETSTDCBSLEX                 77  // really 568 export 77 AnsiNext not thunked
#define FUN_CALLBACK2                    78  // really 802 export 78 AnsiPrev not thunked
#define FUN_CALLBACK4                    79  // really 804 export 79 AnsiUpper not thunked
#define FUN_CALLBACK6                    80  // really 806 export 80 AnsiLower not thunked
#define FUN_CALLBACK8                    81  // really 808 export 81 _lclose not thunked
#define FUN_CALLBACK10                   82  // really 810 export 82 _lread not thunked
#define FUN_CALLBACK12                   83  // really 812 export 83 _lcreat not thunked
#define FUN_CALLBACK14                   84  // really 814 export 84 _lseek not thunked
#define FUN_CALLBACK16                   85  // really 816 export 85 _lopen not thunked
#define FUN_CALLBACK18                   86  // really 818 export 86 _lwrite not thunked
#define FUN_CALLBACK20                   87  // really 820 export 87 lstroriginal not thunked
#define FUN_CALLBACK22                   88  // really 822 export 88 lstrcpy not thunked
#define FUN_CALLBACK24                   89  // really 824 export 89 lstrcat not thunked
#define FUN_CALLBACK26                   90  // really 826 export 90 lstrlen not thunked
#define FUN_CALLBACK28                   91  // really 828 export 91 InitTask not thunked
#define FUN_CALLBACK30                   92  // really 830 export 92 GetTempDrive not thunked
#define FUN_CALLBACK32                   93  // really 832 export 93 GetCodeHandle not thunked
#define FUN_CALLBACK34                   94  // really 834 export 94 DefineHandleTable not thunked
#define FUN_CALLBACK36                   95  // really 836 export 95 LoadLibrary not thunked
#define FUN_CALLBACK38                   96  // really 838 export 96 FreeLibrary not thunked
#define FUN_CALLBACK40                   97  // really 840 export 97 GetTempFilename not thunked
#define FUN_CALLBACK42                   98  // really 842 export 98 GetLastDiskChange not thunked
#define FUN_CALLBACK44                   99  // really 844 export 99 GetLPErrMode not thunked
#define FUN_CALLBACK46                   100 // really 846 export 100 ValidateCodeSegments not thunked
#define FUN_CALLBACK48                   101 // really 848 export 101 NoHookDosCall not thunked
#define FUN_CALLBACK50                   102 // really 850 export 102 Dos3Call not thunked
#define FUN_CALLBACK52                   103 // really 852 export 103 NetBiosCall not thunked
#define FUN_CALLBACK54                   104 // really 854 export 104 GetCodeInfo not thunked
#define FUN_CALLBACK56                   105 // really 856 export 105 GetExeVersion not thunked
#define FUN_CALLBACK58                   106 // really 858 export 106 SetSwapAreaSize not thunked
#define FUN_CALLBACK60                   107 // really 860 export 107 SetErrorMode not thunked
#define FUN_CALLBACK62                   108 // really 862 export 108 SwitchStackTo not thunked
#define FUN_CALLBACK64                   109 // really 864 export 109 SwitchStackBack not thunked
#define FUN_WOWKILLTASK                  110 // WOW internal export 110 PatchCodeHandle not thunked
#define FUN_WOWFILEWRITE                 111 // WOW internal export 111 GlobalWire not thunked
#define FUN_WOWGETNEXTVDMCOMMAND         112 // really 502 export 112 GlobalUnWire not thunked
#define FUN_WOWFILELOCK                  113 // WOW internal export 113 is data __AHSHIFT
#define FUN_WOWFREERESOURCE              114 // WOW internal export 114 is data __AHINCR
#define FUN_WOWOUTPUTDEBUGSTRING         115 // export 115 is OutputDebugString, not directly thunked.
#define FUN_WOWINITTASK                  116 // WOW internal export 116 InitLib not thunked
#define FUN_OLDYIELD                     117 //
#define FUN_WOWFILESETDATETIME           118 // WOW internal export 118 GetTaskQueueDS no longer exported
#define FUN_WOWFILECREATE                119 // WOW internal export 119 GetTaskQueueES no longer exported
#define FUN_WOWDOSWOWINIT                120 // WOW internal export 120 UndefDynLink not thunked
#define FUN_WOWCHECKUSERGDI              121 // WOW internal export 121 LocalShrink not thunked
#define FUN_WOWPARTYBYNUMBER             122 // really 273 export 122 IsTaskLocked not thunked
#define FUN_GETSHORTPATHNAME             123 // really 274 export 123 KbdRst not thunked
#define FUN_FINDANDRELEASEDIB            124 // WOW internal export 124 EnableKernel not thunked
#define FUN_WOWRESERVEHTASK              125 // WOW internal export 125 DisableKernel not thunked
#define FUN_WOWFILESETATTRIBUTES         126 // WOW internal export 126 MemoryFreed not thunked
#define FUN_GETPRIVATEPROFILEINT         127 //
#define FUN_GETPRIVATEPROFILESTRING      128 //
#define FUN_WRITEPRIVATEPROFILESTRING    129 //
#define FUN_WOWSETCURRENTDIRECTORY       130 // WOW internal export 130 FileCDR not thunked
#define FUN_WOWWAITFORMSGANDEVENT        131 // really 262 export 131 GetDosEnvironment not thunked
#define FUN_WOWMSGBOX                    132 // really 263 export 132 GetWinFlags not thunked
#define FUN_WOWGETFLATADDRESSARRAY       133 // WOW internal export 133 GetExePtr not thunked
#define FUN_WOWGETCURRENTDATE            134 // WOW internal export 134 GetWindowsDirectory not thunked
#define FUN_WOWDEVICEIOCTL               135 // WOW internal export 135 GetSystemDirectory not thunked
#define FUN_GETDRIVETYPE                 136 //
#define FUN_WOWFILEGETDATETIME           137 // WOW internal export 137 FatalAppExit not thunked
#define FUN_SETAPPCOMPATFLAGS            138 // WOW internal export 138 GetHeapSpaces not thunked
#define FUN_WOWREGISTERSHELLWINDOWHANDLE 139 // really 251 export 139 DoSignal not thunked
#define FUN_FREELIBRARY32W               140 // really 514 export 140 SetSigHandler not thunked
#define FUN_GETPROCADDRESS32W            141 // really 515 export 141 InitTask1 not thunked
#define FUN_GETPROFILESECTIONNAMES       142 //
#define FUN_GETPRIVATEPROFILESECTIONNAMES 143 //
#define FUN_CREATEDIRECTORY              144 //
#define FUN_REMOVEDIRECTORY              145 //
#define FUN_DELETEFILE                   146 //
#define FUN_SETLASTERROR                 147 //
#define FUN_GETLASTERROR                 148 //
#define FUN_GETVERSIONEX                 149 //
#define FUN_DIRECTEDYIELD                150 //
#define FUN_WOWFILEREAD                  151 // WOW internal export 151 WinOldApCall not thunked
#define FUN_WOWFILELSEEK                 152 // WOW internal export 152 GetNumTasks not thunked
#define FUN_WOWKERNELTRACE               153 // WOW internal export 153 DiscardCodeSegment no longer exported
#define FUN_LOADLIBRARYEX32W             154 // really 513 export 154 GlobalNotify not thunked
#define FUN_WOWQUERYPERFORMANCECOUNTER   155 // really 505 export 155 GetTaskDS not thunked
#define FUN_WOWCURSORICONOP              156 // really 507 export 156 LimitEMSPages not thunked
#define FUN_WOWFAILEDEXEC                157 // WOW internal export 157 GetCurPID not thunked
#define FUN_WOWGETFASTADDRESS            158 // WOW internal export 158 IsWinOldApTask not thunked
#define FUN_WOWCLOSECOMPORT              159 // really 509 export 159 GlobalHandleNoRIP not thunked
#define FUN_LOCAL32INIT                  160 // really 208 export 160 EMSCopy not thunked
#define FUN_LOCAL32ALLOC                 161 // really 209 export 161 LocalCountFree not thunked
#define FUN_LOCAL32REALLOC               162 // really 210 export 162 LocalHeapSize not thunked
#define FUN_LOCAL32FREE                  163 // really 211 export 163 GlobalLRUOldest not thunked
#define FUN_LOCAL32TRANSLATE             164 // really 213 export 164 GlobalLRUNewest not thunked
#define FUN_LOCAL32SIZE                  165 // really 214 export 165 A20Proc not thunked
#define FUN_LOCAL32VALIDHANDLE           166 // really 215 export 166 WinExec not thunked
#define FUN_REGENUMKEY32                 167 // really 216 export 167 GetExpWinVer not thunked
#define FUN_REGOPENKEY32                 168 // really 217 export 168 DirectResAlloc not thunked
#define FUN_REGCREATEKEY32               169 // really 218 export 169 GetFreeSpace not thunked
#define FUN_REGDELETEKEY32               170 // really 219 export 170 AllocCStoDSAlias not thunked
#define FUN_REGCLOSEKEY32                171 // really 220 export 171 AllocDStoCSAlias not thunked
#define FUN_REGSETVALUE32                172 // really 221 export 172 AllocAlias not thunked
#define FUN_REGDELETEVALUE32             173 // really 222 export 173 is data __ROMBIOS
#define FUN_REGENUMVALUE32               174 // really 223 export 174 is data __A000h
#define FUN_REGQUERYVALUE32              175 // really 224 export 175 AllocSelector not thunked
#define FUN_REGQUERYVALUEEX32            176 // really 225 export 176 FreeSelector not thunked
#define FUN_REGSETVALUEEX32              177 // really 226 export 177 PrestoChangoSelector not thunked
#define FUN_REGFLUSHKEY32                178 // really 227 export 178 is data __WINFLAGS
#define FUN_COMPUTEOBJECTOWNER           179 // really 228 export 179 is data __D000h
#define FUN_LOCAL32GETSEL                180 // really 229 export 180 LongPtrAdd not thunked
#define FUN_MAPPROCESSHANDLE             181 // really 483 export 181 is data __B000h
#define FUN_INVALIDATENLSCACHE           182 // really 235 export 182 is data __B800h
#define FUN_WOWDELFILE                   183 // WOW internal export 183 is data __0000h
#define FUN_VIRTUALALLOC                 184 // WOW internal export 184 GlobalDOSAlloc not thunked
#define FUN_VIRTUALFREE                  185 // WOW internal export 185 GlobalDOSFree not thunked
#define FUN_VIRTUALLOCK                  186 // WOW internal export 186 GetSelectorBase not thunked
#define FUN_VIRTUALUNLOCK                187 // WOW internal export 187 SetSelectorBase not thunked
#define FUN_GLOBALMEMORYSTATUS           188 // WOW internal export 188 GetSelectorLimit not thunked
#define FUN_WOWGETFASTCBRETADDRESS       189 // WOW internal export 189 SetSelectorLimit not thunked
#define FUN_WOWGETTABLEOFFSETS           190 // WOW internal export 190 is data __E000h
#define FUN_WOWKILLREMOTETASK            191 // really 511 export 191 GlobalPageLock not thunked
#define FUN_WOWNOTIFYWOW32               192 // WOW internal export 192 GlobalPageUnlock not thunked
#define FUN_WOWFILEOPEN                  193 // WOW internal export 193 is data __0040h
#define FUN_WOWFILECLOSE                 194 // WOW internal export 194 is data __F000h
#define FUN_WOWSETIDLEHOOK               195 // WOW internal export 195 is data __C000h
#define FUN_KSYSERRORBOX                 196 // WOW internal export 196 SelectorAccessRights not thunked
#define FUN_WOWISKNOWNDLL                197 // WOW internal export 197 GlobalFix not thunked
#define FUN_WOWDDEFREEHANDLE             198 // WOW internal export 198 GlobalUnfix not thunked
#define FUN_WOWFILEGETATTRIBUTES         199 // WOW internal export 199 SetHandleCount not thunked
#define FUN_WOWSETDEFAULTDRIVE           200 // WOW internal export 200 ValidateFreeSpaces not thunked
#define FUN_WOWGETCURRENTDIRECTORY       201 // WOW internal export 201 ReplaceInst not thunked
#define FUN_GETPRODUCTNAME               202 // really 236 export 202 RegisterPTrace not thunked
#define FUN_ISSAFEMODE                   203 // really 237 export 203 DebugBreak not thunked
#define FUN_WOWLFNENTRY                  204 // WOW internal export 204 SwapRecording not thunked
#define FUN_WOWSHUTDOWNTIMER             205 // WOW internal export 205 CVWBreak not thunked
#define FUN_WOWTRIMWORKINGSET            206 // WOW internal export 206 AllocSelectorArray not thunked
#ifdef FE_SB
#define FUN_GETSYSTEMDEFAULTLANGID       207 // really 521 export 207 ISDBCSLEADBYTE not thunked 
#endif
#define FUN_TERMSRVGETWINDOWSDIR         208 // internal
//
// Note the following "special" FUN_ identifiers are not used as offsets
// in a thunk table, but rather as arguments to some WOW private APIs,
// WowCursorIconOp and FindAndReleaseDib.
//

#define FUN_GLOBALFREE                   1000
#define FUN_GLOBALREALLOC                1001
#define FUN_GLOBALLOCK                   1002
#define FUN_GLOBALUNLOCK                 1003


/* XLATOFF */
#pragma pack(2)
/* XLATON */


/* NOTE that the tag (like "/* k1 * /") on each typedef line is used by
 * h2inc when building wowkrn.inc, as a prefix for that structures
 * members, since our assembler has only a single flat namespace. */


typedef struct _FATALEXIT16 {                              /* k1 */
    SHORT f1;
} FATALEXIT16;
typedef FATALEXIT16 UNALIGNED *PFATALEXIT16;

typedef struct _EXITKERNEL16 {                             /* k2 */
    WORD wExitCode;
} EXITKERNEL16;
typedef EXITKERNEL16 UNALIGNED *PEXITKERNEL16;

#ifdef NULLSTRUCT
typedef struct _WRITEOUTPROFILES16 {                       /* k3 */
} WRITEOUTPROFILES16;
typedef WRITEOUTPROFILES16 UNALIGNED *PWRITEOUTPROFILES16;
#endif

typedef struct _MAPSL16 {                                  /* k4 */
    DWORD vp;
} MAPSL16;
typedef MAPSL16 UNALIGNED *PMAPSL16;

typedef struct _MAPLS16 {                                  /* k5 */
    PVOID p;
} MAPLS16;
typedef MAPLS16 UNALIGNED *PMAPLS16;

typedef struct _UNMAPLS16 {                                /* k6 */
    PVOID vp;
} UNMAPLS16;
typedef UNMAPLS16 UNALIGNED *PUNMAPLS16;

typedef struct _OPENFILEEX16 {                             /* k7 */
    WORD  wFlags;
    DWORD lpOFStructEx;
    DWORD lpSrcFile;
} OPENFILEEX16;
typedef OPENFILEEX16 UNALIGNED *POPENFILEEX16;

typedef struct _FASTANDDIRTYGLOBALFIX16 {                  /* k8 */
    WORD  selFix;
    WORD  wAction;
} FASTANDDIRTYGLOBALFIX16;
typedef FASTANDDIRTYGLOBALFIX16 UNALIGNED *PFASTANDDIRTYGLOBALFIX16;

typedef struct _WRITEPRIVATEPROFILESTRUCT16 {              /* k9 */
    DWORD lpszFile;
    WORD  cbStruct;
    DWORD lpStruct;
    DWORD lpszKey;
    DWORD lpszSection;
} WRITEPRIVATEPROFILESTRUCT16;
typedef WRITEPRIVATEPROFILESTRUCT16 UNALIGNED *PWRITEPRIVATEPROFILESTRUCT16;

typedef struct _GETPRIVATEPROFILESTRUCT16 {                /* k10 */
    DWORD lpszFile;
    WORD  cbStruct;
    DWORD lpStruct;
    DWORD lpszKey;
    DWORD lpszSection;
} GETPRIVATEPROFILESTRUCT16;
typedef GETPRIVATEPROFILESTRUCT16 UNALIGNED *PGETPRIVATEPROFILESTRUCT16;

typedef struct _GETCURRENTDIRECTORY16 {                    /* k11 */
    DWORD lpszDir;
    DWORD cchDir;
} GETCURRENTDIRECTORY16;
typedef GETCURRENTDIRECTORY16 UNALIGNED *PGETCURRENTDIRECTORY16;

typedef struct _SETCURRENTDIRECTORY16 {                    /* k12 */
    DWORD lpszDir;
} SETCURRENTDIRECTORY16;
typedef SETCURRENTDIRECTORY16 UNALIGNED *PSETCURRENTDIRECTORY16;

typedef struct _FINDFIRSTFILE16 {                          /* k13 */
    DWORD lpFindData;
    DWORD lpszSearchFile;
} FINDFIRSTFILE16;
typedef FINDFIRSTFILE16 UNALIGNED *PFINDFIRSTFILE16;

typedef struct _FINDNEXTFILE16 {                           /* k14 */
    DWORD lpFindData;
    DWORD hFindFile;
} FINDNEXTFILE16;
typedef FINDNEXTFILE16 UNALIGNED *PFINDNEXTFILE16;

typedef struct _FINDCLOSE16 {                              /* k15 */
    DWORD hFindFile;
} FINDCLOSE16;
typedef FINDCLOSE16 UNALIGNED *PFINDCLOSE16;

typedef struct _WRITEPRIVATEPROFILESECTION16 {             /* k16 */
    DWORD lpszFile;
    DWORD lpKeysAndValues;
    DWORD lpszSection;
} WRITEPRIVATEPROFILESECTION16;
typedef WRITEPRIVATEPROFILESECTION16 UNALIGNED *PWRITEPRIVATEPROFILESECTION16;

typedef struct _WRITEPROFILESECTION16 {                    /* k17 */
    DWORD lpKeysAndValues;
    DWORD lpszSection;
} WRITEPROFILESECTION16;
typedef WRITEPROFILESECTION16 UNALIGNED *PWRITEPROFILESECTION16;

typedef struct _GETPRIVATEPROFILESECTION16 {               /* k18 */
    DWORD lpszFile;
    WORD  cchResult;
    DWORD lpResult;
    DWORD lpszSection;
} GETPRIVATEPROFILESECTION16;
typedef GETPRIVATEPROFILESECTION16 UNALIGNED *PGETPRIVATEPROFILESECTION16;

typedef struct _GETPROFILESECTION16 {                      /* k19 */
    WORD  cchResult;
    DWORD lpResult;
    DWORD lpszSection;
} GETPROFILESECTION16;
typedef GETPROFILESECTION16 UNALIGNED *PGETPROFILESECTION16;

typedef struct _GETFILEATTRIBUTES16 {                      /* k20 */
    DWORD lpszFile;
} GETFILEATTRIBUTES16;
typedef GETFILEATTRIBUTES16 UNALIGNED *PGETFILEATTRIBUTES16;

typedef struct _SETFILEATTRIBUTES16 {                      /* k21 */
    DWORD dwFileAttributes;
    DWORD lpszFile;
} SETFILEATTRIBUTES16;
typedef SETFILEATTRIBUTES16 UNALIGNED *PSETFILEATTRIBUTES16;

typedef struct _GETDISKFREESPACE16 {                       /* k22 */
    DWORD lpdwClusters;
    DWORD lpdwFreeClusters;
    DWORD lpdwBytesPerSector;
    DWORD lpdwSectorsPerCluster;
    DWORD lpszRootPathName;
} GETDISKFREESPACE16;
typedef GETDISKFREESPACE16 UNALIGNED *PGETDISKFREESPACE16;

typedef struct _ISPEFORMAT16 {                             /* k23 */
    WORD  hFile;
    DWORD lpszFile;
} ISPEFORMAT16;
typedef ISPEFORMAT16 UNALIGNED *PISPEFORMAT16;

typedef struct _FILETIMETOLOCALFILETIME16 {                /* k24 */
    DWORD lpLocalFileTime;
    DWORD lpUTCFileTime;
} FILETIMETOLOCALFILETIME16;
typedef FILETIMETOLOCALFILETIME16 UNALIGNED *PFILETIMETOLOCALFILETIME16;

typedef struct _UNITOANSI16 {                              /* k25 */
    WORD  cch;
    DWORD pchDst;
    DWORD pchSrc;
} UNITOANSI16;
typedef UNITOANSI16 UNALIGNED *PUNITOANSI16;

typedef struct _GETVDMPOINTER32W16 {                       /* k26 */
    SHORT  fMode;
    VPVOID lpAddress;
} GETVDMPOINTER32W16;
typedef GETVDMPOINTER32W16 UNALIGNED *PGETVDMPOINTER32W16;

typedef struct _CREATETHREAD16 {                           /* k27 */
    DWORD lpThreadID;
    DWORD dwCreateFlags;
    DWORD lpParameter;
    DWORD lpStartAddress;
    DWORD dwStackSize;
    DWORD lpSecurityAttributes;
} CREATETHREAD16;
typedef CREATETHREAD16 UNALIGNED *PCREATETHREAD16;

typedef struct _ICALLPROC32W16 {                           /* k28 */
    WORD  rbp;
    DWORD retaddr;
    DWORD cParams;
    DWORD fAddressConvert;
    DWORD lpProcAddress;
    DWORD p1;
    DWORD p2;
    DWORD p3;
    DWORD p4;
    DWORD p5;
    DWORD p6;
    DWORD p7;
    DWORD p8;
    DWORD p9;
    DWORD p10;
    DWORD p11;
    DWORD p12;
    DWORD p13;
    DWORD p14;
    DWORD p15;
    DWORD p16;
    DWORD p17;
    DWORD p18;
    DWORD p19;
    DWORD p20;
    DWORD p21;
    DWORD p22;
    DWORD p23;
    DWORD p24;
    DWORD p25;
    DWORD p26;
    DWORD p27;
    DWORD p28;
    DWORD p29;
    DWORD p30;
    DWORD p31;
    DWORD p32;
} ICALLPROC32W16;
typedef ICALLPROC32W16 UNALIGNED *PICALLPROC32W16;

#define CPEX32_DEST_CDECL   0x8000L
#define CPEX32_SOURCE_CDECL 0x4000L

#ifdef NULLSTRUCT
typedef struct _YIELD16 {                                  /* k29 */
} YIELD16;
typedef YIELD16 UNALIGNED *PYIELD16;
#endif

typedef struct _WAITEVENT16 {                              /* k30 */
    WORD    wTaskID;
} WAITEVENT16;
typedef WAITEVENT16 UNALIGNED *PWAITEVENT16;

typedef struct _POSTEVENT16 {                              /* k31 */
    WORD    hTask16;
} POSTEVENT16;
typedef POSTEVENT16 UNALIGNED *PPOSTEVENT16;

typedef struct _SETPRIORITY16 {                            /* k32 */
    WORD    wPriority;
    WORD    hTask16;
} SETPRIORITY16;
typedef SETPRIORITY16 UNALIGNED *PSETPRIORITY16;

typedef struct _LOCKCURRENTTASK16 {                        /* k33 */
    WORD    fLock;
} LOCKCURRENTTASK16;
typedef LOCKCURRENTTASK16 UNALIGNED *PLOCKCURRENTTASK16;

#ifdef NULLSTRUCT
typedef struct _LEAVEENTERWIN16LOCK {                      /* k34 */
} LEAVEENTERWIN16LOCK;
typedef LEAVEENTERWIN16LOCK UNALIGNED *PLEAVEENTERWIN16LOCK;
#endif

typedef struct _REGLOADKEY3216 {                           /* k35 */
    VPSTR lpszFileName;
    VPSTR lpszSubkey;
    DWORD hKey;
} REGLOADKEY3216;
typedef REGLOADKEY3216 UNALIGNED *PREGLOADKEY3216;

typedef struct _REGUNLOADKEY3216 {                         /* k36 */
    VPSTR lpszSubkey;
    DWORD hKey;
} REGUNLOADKEY3216;
typedef REGUNLOADKEY3216 UNALIGNED *PREGUNLOADKEY3216;

typedef struct _REGSAVEKEY3216 {                           /* k37 */
    VPVOID lpSA;
    VPSTR  lpszSubkey;
    DWORD  hKey;
} REGSAVEKEY3216;
typedef REGSAVEKEY3216 UNALIGNED *PREGSAVEKEY3216;

#ifdef NULLSTRUCT
typedef struct _GETWIN16LOCK16 {                           /* k38 */
} GETWIN16LOCK16;
typedef GETWIN16LOCK16 UNALIGNED *PGETWIN16LOCK16;
#endif

typedef struct _LOADLIBRARY3216 {                          /* k39 */
    DWORD lpszLibrary;
} LOADLIBRARY3216;
typedef LOADLIBRARY3216 UNALIGNED *PLOADLIBRARY3216;

typedef struct _GETPROCADDRESS3216 {                       /* k40 */
    DWORD lpszProc;
    DWORD hLib;
} GETPROCADDRESS3216;
typedef GETPROCADDRESS3216 UNALIGNED *PGETPROCADDRESS3216;

typedef struct _WOWFINDFIRST16 {                           /* k41 */
    DWORD lpDTA;
    WORD  pszPathOffset;
    WORD  pszPathSegment;
    WORD  wAttributes;
} WOWFINDFIRST16;
typedef WOWFINDFIRST16 UNALIGNED *PWOWFINDFIRST16;

typedef struct _WOWFINDNEXT16 {                            /* k42 */
    DWORD lpDTA;
} WOWFINDNEXT16;
typedef WOWFINDNEXT16 UNALIGNED *PWOWFINDNEXT16;

typedef struct _CREATEWIN32EVENT16 {                       /* k43 */
    DWORD bInitialState;
    DWORD bManualReset;
} CREATEWIN32EVENT16;
typedef CREATEWIN32EVENT16 UNALIGNED *PCREATEWIN32EVENT16;

typedef struct _SETWIN32EVENT16 {                          /* k44 */
    DWORD hEvent;
} SETWIN32EVENT16;
typedef SETWIN32EVENT16 UNALIGNED *PSETWIN32EVENT16;

typedef struct _WOWLOADMODULE16 {                          /* k45 */
    VPSTR  lpWinOldAppCmd;
    VPVOID lpParameterBlock;
    VPSTR  lpModuleName;
} WOWLOADMODULE16;
typedef WOWLOADMODULE16 UNALIGNED *PWOWLOADMODULE16;

typedef struct _PARAMETERBLOCK16 {                         /* k45_2 */
    WORD    wEnvSeg;
    VPVOID  lpCmdLine;
    VPVOID  lpCmdShow;
    DWORD   dwReserved;
} PARAMETERBLOCK16;
typedef PARAMETERBLOCK16 UNALIGNED *PPARAMETERBLOCK16;

typedef struct _RESETWIN32EVENT16 {                        /* k46 */
    DWORD hEvent;
} RESETWIN32EVENT16;
typedef RESETWIN32EVENT16 UNALIGNED *PRESETWIN32EVENT16;

typedef struct _WOWGETMODULEHANDLE16 {                     /* k47 */
    VPSTR lpszModuleName;
} WOWGETMODULEHANDLE16;
typedef WOWGETMODULEHANDLE16 UNALIGNED *PWOWGETMODULEHANDLE16;

typedef struct _WAITFORSINGLEOBJECT16 {                    /* k48 */
    DWORD dwTimeout;
    DWORD h;
} WAITFORSINGLEOBJECT16;
typedef WAITFORSINGLEOBJECT16 UNALIGNED *PWAITFORSINGLEOBJECT16;

typedef struct _GETMODULEFILENAME16 {                      /* k49 */
    SHORT f3;
    VPSTR f2;
    HAND16 f1;
} GETMODULEFILENAME16;
typedef GETMODULEFILENAME16 UNALIGNED *PGETMODULEFILENAME16;

typedef struct _WAITFORMULTIPLEOBJECTS16 {                 /* k50 */
    DWORD dwTimeout;
    DWORD bWaitForAll;
    DWORD lphObjects;
    DWORD cObjects;
} WAITFORMULTIPLEOBJECTS16;
typedef WAITFORMULTIPLEOBJECTS16 UNALIGNED *PWAITFORMULTIPLEOBJECTS16;

#ifdef NULLSTRUCT
typedef struct _GETCURRENTTHREADID16 {                     /* k51 */
} GETCURRENTTHREADID16;
typedef GETCURRENTTHREADID16 UNALIGNED *PGETCURRENTTHREADID16;
#endif

typedef struct _SETTHREADQUEUE16 {                         /* k52 */
    WORD  NewQueueSel;
    DWORD dwThreadID;
} SETTHREADQUEUE16;
typedef SETTHREADQUEUE16 UNALIGNED *PSETTHREADQUEUE16;

typedef struct _CONVERTTOGLOBALHANDLE16 {                  /* k53 */
    DWORD dwHandle;
} CONVERTTOGLOBALHANDLE16;
typedef CONVERTTOGLOBALHANDLE16 UNALIGNED *PCONVERTTOGLOBALHANDLE16;

typedef struct _GETTHREADQUEUE16 {                         /* k54 */
    DWORD dwThreadID;
} GETTHREADQUEUE16;
typedef GETTHREADQUEUE16 UNALIGNED *PGETTHREADQUEUE16;

typedef struct _NUKEPROCESS16 {                            /* k55 */
    DWORD ulFlags;
    WORD  uExitCode;
    DWORD ppdb;
} NUKEPROCESS16;
typedef NUKEPROCESS16 UNALIGNED *PNUKEPROCESS16;

typedef struct _EXITPROCESS16 {                            /* k56 */
    WORD wStatus;
} EXITPROCESS16;
typedef EXITPROCESS16 UNALIGNED *PEXITPROCESS16;

typedef struct _GETPROFILEINT16 {                          /* k57 */
    SHORT f3;
    VPSTR f2;
    VPSTR f1;
} GETPROFILEINT16;
typedef GETPROFILEINT16 UNALIGNED *PGETPROFILEINT16;

typedef struct _GETPROFILESTRING16 {                       /* k58 */
    USHORT f5;
    VPSTR f4;
    VPSTR f3;
    VPSTR f2;
    VPSTR f1;
} GETPROFILESTRING16;
typedef GETPROFILESTRING16 UNALIGNED *PGETPROFILESTRING16;

typedef struct _WRITEPROFILESTRING16 {                     /* k59 */
    VPSTR f3;
    VPSTR f2;
    VPSTR f1;
} WRITEPROFILESTRING16;
typedef WRITEPROFILESTRING16 UNALIGNED *PWRITEPROFILESTRING16;

#ifdef NULLSTRUCT
typedef struct _GETCURRENTPROCESSID16 {                    /* k60 */
} GETCURRENTPROCESSID16;
typedef GETCURRENTPROCESSID16 UNALIGNED *PGETCURRENTPROCESSID16;
#endif

#ifdef NULLSTRUCT
typedef struct _MAPHINSTLS {                               /* k61 */
    /* NOTE if you implement this the interface is register-based */
} MAPHINSTLS;
typedef MAPHINSTLS UNALIGNED *PMAPHINSTLS;
#endif

#ifdef NULLSTRUCT
typedef struct _MAPHINSTSL {                               /* k62 */
    /* NOTE if you implement this the interface is register-based */
} MAPHINSTSL;
typedef MAPHINSTSL UNALIGNED *PMAPHINSTSL;
#endif

typedef struct _CLOSEWIN32HANDLE16 {                       /* k63 */
    DWORD h32;
} CLOSEWIN32HANDLE16;
typedef CLOSEWIN32HANDLE16 UNALIGNED *PCLOSEWIN32HANDLE16;

typedef struct _LOADSYSTEMLIBRARY3216 {                    /* k64 */
    VPSTR lpszLibrary;
} LOADSYSTEMLIBRARY3216;
typedef LOADSYSTEMLIBRARY3216 UNALIGNED *PLOADSYSTEMLIBRARY3216;

typedef struct _FREELIBRARY3216 {                          /* k65 */
    DWORD hModule;
} FREELIBRARY3216;
typedef FREELIBRARY3216 UNALIGNED *PFREELIBRARY3216;

typedef struct _GETMODULEFILENAME3216 {                    /* k66 */
    WORD    wBufferSize;
    VPSTR   lpBuffer;
    DWORD   hModule;
} GETMODULEFILENAME3216;
typedef GETMODULEFILENAME3216 UNALIGNED *PGETMODULEFILENAME3216;

typedef struct _GETMODULEHANDLE3216 {                      /* k67 */
    VPSTR lpszModule;
} GETMODULEHANDLE3216;
typedef GETMODULEHANDLE3216 UNALIGNED *PGETMODULEHANDLE3216;

typedef struct _REGISTERSERVICEPROCESS16 {                 /* k68 */
    DWORD dwServiceType;
    DWORD dwProcessID;
} REGISTERSERVICEPROCESS16;
typedef REGISTERSERVICEPROCESS16 UNALIGNED *PREGISTERSERVICEPROCESS16;

typedef struct _CHANGEALLOCFIXEDBEHAVIOUR16 {              /* k69 */
    WORD fWin31Behavior;
} CHANGEALLOCFIXEDBEHAVIOUR16;
typedef CHANGEALLOCFIXEDBEHAVIOUR16 UNALIGNED *PCHANGEALLOCFIXEDBEHAVIOUR16;

typedef struct _INITCB16 {                                 /* k70 */
    DWORD fnStdCBSLDispatch;
    DWORD fnStdCBLSDispatch;
} INITCB16;
typedef INITCB16 UNALIGNED *PINITCB16;

typedef struct _GETSTDCBLS16 {                             /* k71 */
    DWORD CBID;
    DWORD pfnTarg16;
} GETSTDCBLS16;
typedef GETSTDCBLS16 UNALIGNED *PGETSTDCBLS16;

typedef struct _GETSTDCBSL16 {                             /* k72 */
    DWORD CBID;
    DWORD pfnTarg32;
} GETSTDCBSL16;
typedef GETSTDCBSL16 UNALIGNED *PGETSTDCBSL16;

typedef struct _GETEXISTINGSTDCBLS16 {                     /* k73 */
    DWORD CBID;
    DWORD pfnTarg16;
} GETEXISTINGSTDCBLS16;
typedef GETEXISTINGSTDCBLS16 UNALIGNED *PGETEXISTINGSTDCBLS16;

typedef struct _GETEXISTINGSTDCBSL16 {                     /* k74 */
    DWORD CBID;
    DWORD pfnTarg32;
} GETEXISTINGSTDCBSL16;
typedef GETEXISTINGSTDCBSL16 UNALIGNED *PGETEXISTINGSTDCBSL16;

typedef struct _GETFLEXCBSL16 {                            /* k75 */
    DWORD pfnThunk;
    DWORD pfnTarg32;
} GETFLEXCBSL16;
typedef GETFLEXCBSL16 UNALIGNED *PGETFLEXCBSL16;

typedef struct _GETSTDCBLSEX16 {                           /* k76 */
    WORD  wOwner;
    DWORD CBID;
    DWORD pfnTarg16;
} GETSTDCBLSEX16;
typedef GETSTDCBLSEX16 UNALIGNED *PGETSTDCBLSEX16;

typedef struct _GETSTDCBSLEX16 {                           /* k77 */
    WORD  wOwner;
    DWORD CBID;
    DWORD pfnTarg32;
} GETSTDCBSLEX16;
typedef GETSTDCBSLEX16 UNALIGNED *PGETSTDCBSLEX16;

typedef struct _CALLBACK216 {                              /* k78 */
    WORD rgwArgs[1];
} CALLBACK216;
typedef CALLBACK216 UNALIGNED *PCALLBACK216;

typedef struct _CALLBACK416 {                              /* k79 */
    WORD rgwArgs[2];
} CALLBACK416;
typedef CALLBACK416 UNALIGNED *PCALLBACK416;

typedef struct _CALLBACK616 {                              /* k80 */
    WORD rgwArgs[3];
} CALLBACK616;
typedef CALLBACK616 UNALIGNED *PCALLBACK616;

typedef struct _CALLBACK816 {                              /* k81 */
    WORD rgwArgs[4];
} CALLBACK816;
typedef CALLBACK816 UNALIGNED *PCALLBACK816;

typedef struct _CALLBACK1016 {                             /* k82 */
    WORD rgwArgs[5];
} CALLBACK1016;
typedef CALLBACK1016 UNALIGNED *PCALLBACK1016;

typedef struct _CALLBACK1216 {                             /* k83 */
    WORD rgwArgs[6];
} CALLBACK1216;
typedef CALLBACK1216 UNALIGNED *PCALLBACK1216;

typedef struct _CALLBACK1416 {                             /* k84 */
    WORD rgwArgs[7];
} CALLBACK1416;
typedef CALLBACK1416 UNALIGNED *PCALLBACK1416;

typedef struct _CALLBACK1616 {                             /* k85 */
    WORD rgwArgs[8];
} CALLBACK1616;
typedef CALLBACK1616 UNALIGNED *PCALLBACK1616;

typedef struct _CALLBACK1816 {                             /* k86 */
    WORD rgwArgs[9];
} CALLBACK1816;
typedef CALLBACK1816 UNALIGNED *PCALLBACK1816;

typedef struct _CALLBACK2016 {                             /* k87 */
    WORD rgwArgs[10];
} CALLBACK2016;
typedef CALLBACK2016 UNALIGNED *PCALLBACK2016;

typedef struct _CALLBACK2216 {                             /* k88 */
    WORD rgwArgs[11];
} CALLBACK2216;
typedef CALLBACK2216 UNALIGNED *PCALLBACK2216;

typedef struct _CALLBACK2416 {                             /* k89 */
    WORD rgwArgs[12];
} CALLBACK2416;
typedef CALLBACK2416 UNALIGNED *PCALLBACK2416;

typedef struct _CALLBACK2616 {                             /* k90 */
    WORD rgwArgs[13];
} CALLBACK2616;
typedef CALLBACK2616 UNALIGNED *PCALLBACK2616;

typedef struct _CALLBACK2816 {                             /* k91 */
    WORD rgwArgs[14];
} CALLBACK2816;
typedef CALLBACK2816 UNALIGNED *PCALLBACK2816;

typedef struct _CALLBACK3016 {                             /* k92 */
    WORD rgwArgs[15];
} CALLBACK3016;
typedef CALLBACK3016 UNALIGNED *PCALLBACK3016;

typedef struct _CALLBACK3216 {                             /* k93 */
    WORD rgwArgs[16];
} CALLBACK3216;
typedef CALLBACK3216 UNALIGNED *PCALLBACK3216;

typedef struct _CALLBACK3416 {                             /* k94 */
    WORD rgwArgs[17];
} CALLBACK3416;
typedef CALLBACK3416 UNALIGNED *PCALLBACK3416;

typedef struct _CALLBACK3616 {                             /* k95 */
    WORD rgwArgs[18];
} CALLBACK3616;
typedef CALLBACK3616 UNALIGNED *PCALLBACK3616;

typedef struct _CALLBACK3816 {                             /* k96 */
    WORD rgwArgs[19];
} CALLBACK3816;
typedef CALLBACK3816 UNALIGNED *PCALLBACK3816;

typedef struct _CALLBACK4016 {                             /* k97 */
    WORD rgwArgs[20];
} CALLBACK4016;
typedef CALLBACK4016 UNALIGNED *PCALLBACK4016;

typedef struct _CALLBACK4216 {                             /* k98 */
    WORD rgwArgs[21];
} CALLBACK4216;
typedef CALLBACK4216 UNALIGNED *PCALLBACK4216;

typedef struct _CALLBACK4416 {                             /* k99 */
    WORD rgwArgs[22];
} CALLBACK4416;
typedef CALLBACK4416 UNALIGNED *PCALLBACK4416;

typedef struct _CALLBACK4616 {                             /* k100 */
    WORD rgwArgs[23];
} CALLBACK4616;
typedef CALLBACK4616 UNALIGNED *PCALLBACK4616;

typedef struct _CALLBACK4816 {                             /* k101 */
    WORD rgwArgs[24];
} CALLBACK4816;
typedef CALLBACK4816 UNALIGNED *PCALLBACK4816;

typedef struct _CALLBACK5016 {                             /* k102 */
    WORD rgwArgs[25];
} CALLBACK5016;
typedef CALLBACK5016 UNALIGNED *PCALLBACK5016;

typedef struct _CALLBACK5216 {                             /* k103 */
    WORD rgwArgs[26];
} CALLBACK5216;
typedef CALLBACK5216 UNALIGNED *PCALLBACK5216;

typedef struct _CALLBACK5416 {                             /* k104 */
    WORD rgwArgs[27];
} CALLBACK5416;
typedef CALLBACK5416 UNALIGNED *PCALLBACK5416;

typedef struct _CALLBACK5616 {                             /* k105 */
    WORD rgwArgs[28];
} CALLBACK5616;
typedef CALLBACK5616 UNALIGNED *PCALLBACK5616;

typedef struct _CALLBACK5816 {                             /* k106 */
    WORD rgwArgs[29];
} CALLBACK5816;
typedef CALLBACK5816 UNALIGNED *PCALLBACK5816;

typedef struct _CALLBACK6016 {                             /* k107 */
    WORD rgwArgs[30];
} CALLBACK6016;
typedef CALLBACK6016 UNALIGNED *PCALLBACK6016;

typedef struct _CALLBACK6216 {                             /* k108 */
    WORD rgwArgs[31];
} CALLBACK6216;
typedef CALLBACK6216 UNALIGNED *PCALLBACK6216;

typedef struct _CALLBACK6416 {                             /* k109 */
    WORD rgwArgs[32];
} CALLBACK6416;
typedef CALLBACK6416 UNALIGNED *PCALLBACK6416;

typedef struct _WOWFILEWRITE16 {                           /* k111 */
    DWORD lpSFT;
    DWORD lpPDB;
    DWORD  bufsize;
    DWORD lpBuf;
    WORD  fh;
} WOWFILEWRITE16;
typedef WOWFILEWRITE16 UNALIGNED *PWOWFILEWRITE16;

typedef struct _WOWGETNEXTVDMCOMMAND16 {                   /* k112 */
    VPVOID  lpWowInfo;
} WOWGETNEXTVDMCOMMAND16;
typedef WOWGETNEXTVDMCOMMAND16 UNALIGNED *PWOWGETNEXTVDMCOMMAND16;

typedef struct _WOWFILELOCK16 {                            /* k113 */
    DWORD lpSFT;
    DWORD lpPDB;
    DWORD cbRegionLength;
    DWORD cbRegionOffset;
    WORD  fh;
    WORD  ax;
} WOWFILELOCK16;
typedef WOWFILELOCK16 UNALIGNED *PWOWFILELOCK16;

typedef struct _WOWFREERESOURCE16 {                        /* k114 */
    HAND16 f1;
} WOWFREERESOURCE16;
typedef WOWFREERESOURCE16 UNALIGNED *PWOWFREERESOURCE16;

typedef struct _WOWOUTPUTDEBUGSTRING16 {                   /* k115 */
    VPSTR   vpString;
} WOWOUTPUTDEBUGSTRING16;
typedef WOWOUTPUTDEBUGSTRING16 UNALIGNED *PWOWOUTPUTDEBUGSTRING16;

typedef struct _WOWINITTASK16 {                            /* k116 */
    DWORD dwExpWinVer;
} WOWINITTASK16;
typedef WOWINITTASK16 UNALIGNED *PWOWINITTASK16;

typedef struct _WOWFILESETDATETIME16 {                     /* k118 */
    DWORD lpSFT;
    DWORD lpPDB;
    WORD  date;
    WORD  time;
    WORD  fh;
} WOWFILESETDATETIME16;
typedef WOWFILESETDATETIME16 UNALIGNED *PWOWFILESETDATETIME16;

typedef struct _WOWFILECREATE16 {                          /* k119 */
    DWORD lpSFT;
    DWORD lpPDB;
    WORD  pszPathOffset;
    WORD  pszPathSegment;
    WORD  wAttributes;
} WOWFILECREATE16;
typedef WOWFILECREATE16 UNALIGNED *PWOWFILECREATE16;

typedef struct _WOWDOSWOWINIT16 {                          /* k120 */
    VPVOID  lpDosWowData;
} WOWDOSWOWINIT16;
typedef WOWDOSWOWINIT16 UNALIGNED *PWOWDOSWOWINIT16;

typedef struct _WOWCHECKUSERGDI16 {                        /* k121 */
    WORD  pszPathOffset;
    WORD  pszPathSegment;
} WOWCHECKUSERGDI16;
typedef WOWCHECKUSERGDI16 UNALIGNED *PWOWCHECKUSERGDI16;

typedef struct _WOWPARTYBYNUMBER16 {                       /* k122 */
    VPSZ  psz;
    DWORD dw;
} WOWPARTYBYNUMBER16;
typedef WOWPARTYBYNUMBER16 UNALIGNED *PWOWPARTYBYNUMBER16;

typedef struct _GETSHORTPATHNAME16 {                       /* k123 */
    WORD  cchShortPath;
    VPSZ  pszShortPath;
    VPSZ  pszLongPath;
} GETSHORTPATHNAME16;
typedef GETSHORTPATHNAME16 UNALIGNED *PGETSHORTPATHNAME16;

typedef struct _FINDANDRELEASEDIB16 {                      /* k124 */
    WORD wFunId;
    HAND16 hdib;     /* handle which we are messing with */
} FINDANDRELEASEDIB16;
typedef FINDANDRELEASEDIB16 UNALIGNED *PFINDANDRELEASEDIB16;

typedef struct _WOWRESERVEHTASK16 {                        /* k125 */
    WORD  htask;
} WOWRESERVEHTASK16;
typedef WOWRESERVEHTASK16 UNALIGNED *PWOWRESERVEHTASK16;

typedef struct _WOWFILESETATTRIBUTES16 {                   /* k126 */
    WORD  pszPathOffset;
    WORD  pszPathSegment;
    WORD  wAttributes;
} WOWFILESETATTRIBUTES16;
typedef WOWFILESETATTRIBUTES16 UNALIGNED *PWOWFILESETATTRIBUTES16;

typedef struct _GETPRIVATEPROFILEINT16 {                   /* k127 */
    VPSTR f4;
    SHORT f3;
    VPSTR f2;
    VPSTR f1;
} GETPRIVATEPROFILEINT16;
typedef GETPRIVATEPROFILEINT16 UNALIGNED *PGETPRIVATEPROFILEINT16;

typedef struct _GETPRIVATEPROFILESTRING16 {                /* k128 */
    VPSTR f6;
    USHORT f5;
    VPSTR f4;
    VPSTR f3;
    VPSTR f2;
    VPSTR f1;
} GETPRIVATEPROFILESTRING16;
typedef GETPRIVATEPROFILESTRING16 UNALIGNED *PGETPRIVATEPROFILESTRING16;

typedef struct _WRITEPRIVATEPROFILESTRING16 {              /* k129 */
    VPSTR f4;
    VPSTR f3;
    VPSTR f2;
    VPSTR f1;
} WRITEPRIVATEPROFILESTRING16;
typedef WRITEPRIVATEPROFILESTRING16 UNALIGNED *PWRITEPRIVATEPROFILESTRING16;

typedef struct _WOWSETCURRENTDIRECTORY16 {                 /* k130 */
    DWORD lpCurDir;
} WOWSETCURRENTDIRECTORY16;
typedef WOWSETCURRENTDIRECTORY16 UNALIGNED *PWOWSETCURRENTDIRECTORY16;

typedef struct _WOWWAITFORMSGANDEVENT16 {                  /* k131 */
    HWND16 hwnd;
} WOWWAITFORMSGANDEVENT16;
typedef WOWWAITFORMSGANDEVENT16 UNALIGNED *PWOWWAITFORMSGANDEVENT16;

typedef struct _WOWMSGBOX16 {                              /* k132 */
    DWORD   dwOptionalStyle;
    VPSZ    pszTitle;
    VPSZ    pszMsg;
} WOWMSGBOX16;
typedef WOWMSGBOX16 UNALIGNED *PWOWMSGBOX16;

typedef struct _WOWDEVICEIOCTL16 {                         /* k135 */
    WORD  wCmd;
    WORD  wDriveNum;
} WOWDEVICEIOCTL16;
typedef WOWDEVICEIOCTL16 UNALIGNED *PWOWDEVICEIOCTL16;

typedef struct _GETDRIVETYPE16 {                           /* k136 */
    SHORT f1;
} GETDRIVETYPE16;
typedef GETDRIVETYPE16 UNALIGNED *PGETDRIVETYPE16;

typedef struct _WOWFILEGETDATETIME16 {                     /* k137 */
    DWORD lpSFT;
    DWORD lpPDB;
    WORD  fh;
} WOWFILEGETDATETIME16;
typedef WOWFILEGETDATETIME16 UNALIGNED *PWOWFILEGETDATETIME16;

typedef struct _SETAPPCOMPATFLAGS16 {           /* k139 */
    WORD  TDB;
} SETAPPCOMPATFLAGS16;
typedef SETAPPCOMPATFLAGS16 UNALIGNED *PSETAPPCOMPATFLAGS16;

typedef struct _WOWREGISTERSHELLWINDOWHANDLE16 {           /* k139 */
    HWND16 hwndFax;
    VPWORD lpwCmdShow;
    HWND16 hwndShell;
} WOWREGISTERSHELLWINDOWHANDLE16;
typedef WOWREGISTERSHELLWINDOWHANDLE16 UNALIGNED *PWOWREGISTERSHELLWINDOWHANDLE16;

typedef struct _FREELIBRARY32W16 {                         /* k140 */
    DWORD  hLibModule;
} FREELIBRARY32W16;
typedef FREELIBRARY32W16 UNALIGNED *PFREELIBRARY32W16;

typedef struct _GETPROCADDRESS32W16 {                      /* k141 */
    VPVOID lpszProc;
    DWORD  hModule;
} GETPROCADDRESS32W16;
typedef GETPROCADDRESS32W16 UNALIGNED *PGETPROCADDRESS32W16;

typedef struct _GETPROFILESECTIONNAMES16 {                 /* k142 */
    WORD  cbBuffer;
    VPSTR lpszBuffer;
} GETPROFILESECTIONNAMES16;
typedef GETPROFILESECTIONNAMES16 UNALIGNED *PGETPROFILESECTIONNAMES16;

typedef struct _GETPRIVATEPROFILESECTIONNAMES16 {          /* k143 */
    VPSTR lpszFile;
    WORD  cbBuffer;
    VPSTR lpszBuffer;
} GETPRIVATEPROFILESECTIONNAMES16;
typedef GETPRIVATEPROFILESECTIONNAMES16 UNALIGNED *PGETPRIVATEPROFILESECTIONNAMES16;

typedef struct _CREATEDIRECTORY16 {                        /* k144 */
    VPVOID lpSA;
    VPSTR  lpszName;
} CREATEDIRECTORY16;
typedef CREATEDIRECTORY16 UNALIGNED *PCREATEDIRECTORY16;

typedef struct _REMOVEDIRECTORY16 {                        /* k145 */
    VPSTR  lpszName;
} REMOVEDIRECTORY16;
typedef REMOVEDIRECTORY16 UNALIGNED *PREMOVEDIRECTORY16;

typedef struct _DELETEFILE16 {                             /* k146 */
    VPSTR  lpszName;
} DELETEFILE16;
typedef DELETEFILE16 UNALIGNED *PDELETEFILE16;

typedef struct _SETLASTERROR16 {                           /* k147 */
    DWORD dwError;
} SETLASTERROR16;
typedef SETLASTERROR16 UNALIGNED *PSETLASTERROR16;

#ifdef NULLSTRUCT
typedef struct _GETLASTERROR16 {                           /* k148 */
} GETLASTERROR16;
typedef GETLASTERROR16 UNALIGNED *PGETLASTERROR16;
#endif

typedef struct _GETVERSIONEX16 {                           /* k149 */
    VPVOID lpVersionInfo;
} GETVERSIONEX16;
typedef GETVERSIONEX16 UNALIGNED *PGETVERSIONEX16;

typedef struct _DIRECTEDYIELD16 {                          /* k150 */
    WORD    hTask16;
} DIRECTEDYIELD16;
typedef DIRECTEDYIELD16 UNALIGNED *PDIRECTEDYIELD16;

typedef struct _WOWFILEREAD16 {                            /* k151 */
    DWORD lpSFT;
    DWORD lpPDB;
    DWORD bufsize;
    DWORD lpBuf;
    WORD  fh;
} WOWFILEREAD16;
typedef WOWFILEREAD16 UNALIGNED *PWOWFILEREAD16;

typedef struct _WOWFILELSEEK16 {                           /* k152 */
    DWORD lpSFT;
    DWORD lpPDB;
    WORD  mode;
    DWORD fileOffset;
    WORD  fh;
} WOWFILELSEEK16;
typedef WOWFILELSEEK16 UNALIGNED *PWOWFILELSEEK16;

typedef struct _WOWKERNELTRACE16 {                         /* k153 */
    DWORD lpUserArgs;
    WORD  cParms;
    VPSTR lpRoutineName;
} WOWKERNELTRACE16;
typedef WOWKERNELTRACE16 UNALIGNED *PWOWKERNELTRACE16;

typedef struct _LOADLIBRARYEX32W16 {                       /* k154 */
    DWORD  dwFlags;
    DWORD  hFile;
    VPVOID lpszLibFile;
} LOADLIBRARYEX32W16;
typedef LOADLIBRARYEX32W16 UNALIGNED *PLOADLIBRARYEX32W16;

typedef struct _WOWQUERYPERFORMANCECOUNTER16 {             /* k155 */
    VPVOID lpPerformanceFrequency;
    VPVOID lpPerformanceCounter;
} WOWQUERYPERFORMANCECOUNTER16;
typedef WOWQUERYPERFORMANCECOUNTER16 UNALIGNED *PWOWQUERYPERFORMANCECOUNTER16;

typedef struct _WOWCURSORICONOP16 {                        /* k156 */
    WORD   wFuncId;
    WORD   h16;
} WOWCURSORICONOP16;
typedef WOWCURSORICONOP16 UNALIGNED *PWOWCURSORICONOP16;

typedef struct _WOWCLOSECOMPORT16 {                        /* k159 */
    WORD    wPortId;
} WOWCLOSECOMPORT16;
typedef WOWCLOSECOMPORT16 UNALIGNED *PWOWCLOSECOMPORT16;

typedef struct _LOCAL32INIT16 {                            /* k160 */
    DWORD dwFlags;
    DWORD dwcbMax;
    DWORD dwcbInit;
    WORD  wSel;
} LOCAL32INIT16;
typedef LOCAL32INIT16 UNALIGNED *PLOCAL32INIT16;

typedef struct _LOCAL32ALLOC16 {                           /* k161 */
    DWORD dwFlags;
    WORD  wType;
    DWORD dwcbRequest;
    WORD  wSel;
} LOCAL32ALLOC16;
typedef LOCAL32ALLOC16 UNALIGNED *PLOCAL32ALLOC16;

typedef struct _LOCAL32REALLOC16 {                         /* k162 */
    DWORD dwFlags;
    DWORD dwcbNew;
    WORD  wType;
    DWORD dwMem;
    DWORD dwLinHeader;
} LOCAL32REALLOC16;
typedef LOCAL32REALLOC16 UNALIGNED *PLOCAL32REALLOC16;

typedef struct _LOCAL32FREE16 {                            /* k163 */
    WORD  wType;
    DWORD dwMem;
    DWORD dwLinHeader;
} LOCAL32FREE16;
typedef LOCAL32FREE16 UNALIGNED *PLOCAL32FREE16;

typedef struct _LOCAL32TRANSLATE16 {                       /* k164 */
    WORD  wRetType;
    WORD  wMemType;
    DWORD dwMem;
    DWORD dwLinHeader;
} LOCAL32TRANSLATE16;
typedef LOCAL32TRANSLATE16 UNALIGNED *PLOCAL32TRANSLATE16;

typedef struct _LOCAL32SIZE16 {                            /* k165 */
    WORD  wType;
    DWORD dwMem;
    DWORD dwLinHeader;
} LOCAL32SIZE16;
typedef LOCAL32SIZE16 UNALIGNED *PLOCAL32SIZE16;

typedef struct _LOCAL32VALIDHANDLE16 {                     /* k166 */
    WORD  hMem;
    DWORD dwLinHeader;
} LOCAL32VALIDHANDLE16;
typedef LOCAL32VALIDHANDLE16 UNALIGNED *PLOCAL32VALIDHANDLE16;

typedef struct _REGENUMKEY3216 {                           /* k167 */
    DWORD  cchName;
    VPSTR  lpszName;
    DWORD  iSubKey;
    DWORD  hKey;
} REGENUMKEY3216;
typedef REGENUMKEY3216 UNALIGNED *PREGENUMKEY3216;

typedef struct _REGOPENKEY3216 {                           /* k168 */
    VPVOID  phkResult;
    VPSTR   lpszSubKey;
    DWORD   hKey;
} REGOPENKEY3216;
typedef REGOPENKEY3216 UNALIGNED *PREGOPENKEY3216;

typedef struct _REGCREATEKEY3216 {                         /* k169 */
    VPVOID  phkResult;
    VPSTR   lpszSubKey;
    DWORD   hKey;
} REGCREATEKEY3216;
typedef REGCREATEKEY3216 UNALIGNED *PREGCREATEKEY3216;

typedef struct _REGDELETEKEY3216 {                         /* k170 */
    VPSTR   lpszSubKey;
    DWORD   hKey;
} REGDELETEKEY3216;
typedef REGDELETEKEY3216 UNALIGNED *PREGDELETEKEY3216;

typedef struct _REGCLOSEKEY3216 {                          /* k171 */
    DWORD  hKey;
} REGCLOSEKEY3216;
typedef REGCLOSEKEY3216 UNALIGNED *PREGCLOSEKEY3216;

typedef struct _REGSETVALUE3216 {                          /* k172 */
    DWORD   cbValue;
    VPSTR   lpValue;
    DWORD   dwType;
    VPSTR   lpszSubKey;
    DWORD   hKey;
} REGSETVALUE3216;
typedef REGSETVALUE3216 UNALIGNED *PREGSETVALUE3216;

typedef struct _REGDELETEVALUE3216 {                       /* k173 */
    VPSTR   lpszValue;
    DWORD   hKey;
} REGDELETEVALUE3216;
typedef REGDELETEVALUE3216 UNALIGNED *PREGDELETEVALUE3216;

typedef struct _REGENUMVALUE3216 {                         /* k174 */
    VPVOID lpcbData;
    VPVOID lpbData;
    DWORD  lpdwType;
    DWORD  lpdwReserved;
    DWORD  lpcchValue;
    VPSTR  lpszValue;
    DWORD  iValue;
    DWORD  hKey;
} REGENUMVALUE3216;
typedef REGENUMVALUE3216 UNALIGNED *PREGENUMVALUE3216;

typedef struct _WOWLFNFRAMEPTR16 {                         /* k204 */
    VPVOID lpUserFrame;
} WOWLFNFRAMEPTR16;
typedef WOWLFNFRAMEPTR16 UNALIGNED *PWOWLFNFRAMEPTR16;

typedef struct _REGQUERYVALUE3216 {                        /* k175 */
    DWORD   cbValue;
    VPSTR   lpValue;
    VPSTR   lpszSubKey;
    DWORD   hKey;
} REGQUERYVALUE3216;
typedef REGQUERYVALUE3216 UNALIGNED *PREGQUERYVALUE3216;

typedef struct _REGQUERYVALUEEX3216 {                      /* k176 */
    DWORD   cbBuffer;
    VPSTR   lpBuffer;
    VPDWORD vpdwType;
    VPDWORD vpdwReserved;
    VPSTR   lpszValue;
    DWORD   hKey;
} REGQUERYVALUEEX3216;
typedef REGQUERYVALUEEX3216 UNALIGNED *PREGQUERYVALUEEX3216;

typedef struct _REGSETVALUEEX3216 {                        /* k177 */
    DWORD   cbBuffer;
    VPSTR   lpBuffer;
    DWORD   dwType;
    DWORD   dwReserved;
    VPSTR   lpszValue;
    DWORD   hKey;
} REGSETVALUEEX3216;
typedef REGSETVALUEEX3216 UNALIGNED *PREGSETVALUEEX3216;

typedef struct _REGFLUSHKEY3216 {                          /* k178 */
    DWORD   hKey;
} REGFLUSHKEY3216;
typedef REGFLUSHKEY3216 UNALIGNED *PREGFLUSHKEY3216;

typedef struct _COMPUTEOBJECTOWNER16 {                     /* k179 */
    WORD wSel;
} COMPUTEOBJECTOWNER16;
typedef COMPUTEOBJECTOWNER16 UNALIGNED *PCOMPUTEOBJECTOWNER16;

typedef struct _LOCAL32GETSEL16 {                          /* k180 */
    DWORD dwLinHeader;
} LOCAL32GETSEL16;
typedef LOCAL32GETSEL16 UNALIGNED *PLOCAL32GETSEL16;

typedef struct _MAPPROCESSHANDLE16 {                       /* k181 */
    DWORD dwHandle;
} MAPPROCESSHANDLE16;
typedef MAPPROCESSHANDLE16 UNALIGNED *PMAPPROCESSHANDLE16;

#ifdef NULLSTRUCT
typedef struct _INVALIDATENLSCACHE16 {                     /* k182 */
} INVALIDATENLSCACHE16;
typedef INVALIDATENLSCACHE16 UNALIGNED *PINVALIDATENLSCACHE16;
#endif

typedef struct _WOWDELFILE16 {                             /* k183 */
    VPSTR lpFile;
} WOWDELFILE16;
typedef WOWDELFILE16 UNALIGNED *PWOWDELFILE16;

typedef struct _VIRTUALALLOC16 {                           /* k184 */
    DWORD fdwProtect;
    DWORD fdwAllocationType;
    DWORD cbSize;
    DWORD lpvAddress;
} VIRTUALALLOC16;
typedef VIRTUALALLOC16 UNALIGNED *PVIRTUALALLOC16;

typedef struct _VIRTUALFREE16 {                            /* k185 */
    DWORD fdwFreeType;
    DWORD cbSize;
    DWORD lpvAddress;
} VIRTUALFREE16;
typedef VIRTUALFREE16 UNALIGNED *PVIRTUALFREE16;

typedef struct _VIRTUALLOCK16 {                            /* k186 */
    DWORD cbSize;
    DWORD lpvAddress;
} VIRTUALLOCK16;
typedef VIRTUALLOCK16 UNALIGNED *PVIRTUALLOCK16;

typedef struct _VIRTUALUNLOCK16 {                          /* k187 */
    DWORD cbSize;
    DWORD lpvAddress;
} VIRTUALUNLOCK16;
typedef VIRTUALUNLOCK16 UNALIGNED *PVIRTUALUNLOCK16;

typedef struct _GLOBALMEMORYSTATUS16 {                     /* k188 */
    VPVOID lpmstMemStat;
} GLOBALMEMORYSTATUS16;
typedef GLOBALMEMORYSTATUS16 UNALIGNED *PGLOBALMEMORYSTATUS16;

typedef struct _WOWGETTABLEOFFSETS16 {                     /* k190 */
    VPVOID  vpThunkTableOffsets;
} WOWGETTABLEOFFSETS16;
typedef WOWGETTABLEOFFSETS16 UNALIGNED *PWOWGETTABLEOFFSETS16;

typedef struct _WOWKILLREMOTETASK16 {                      /* k191 */
    VPVOID  lpBuffer;
} WOWKILLREMOTETASK16;
typedef WOWKILLREMOTETASK16 UNALIGNED *PWOWKILLREMOTETASK16;

typedef struct _WOWNOTIFYWOW3216 {                         /* k192 */
    VPVOID  Int21Handler;
    VPVOID  lpnum_tasks;
    VPVOID  lpcurTDB;
    VPVOID  lpDebugWOW;
    VPVOID  lpLockTDB;
    VPVOID  lptopPDB;
    VPVOID  lpCurDirOwner;
} WOWNOTIFYWOW3216;
typedef WOWNOTIFYWOW3216 UNALIGNED *PWOWNOTIFYWOW3216;

typedef struct _WOWFILEOPEN16 {                            /* k193 */
    DWORD lpSFT;
    DWORD lpPDB;
    WORD  wAccess;
    WORD  pszPathOffset;
    WORD  pszPathSegment;
} WOWFILEOPEN16;
typedef WOWFILEOPEN16 UNALIGNED *PWOWFILEOPEN16;

typedef struct _WOWFILECLOSE16 {                           /* k194 */
    DWORD lpSFT;
    DWORD lpPDB;
    WORD  hFile;
} WOWFILECLOSE16;
typedef WOWFILECLOSE16 UNALIGNED *PWOWFILECLOSE16;

typedef struct _KSYSERRORBOX16 {                           /* k196 */
    SHORT sBtn3;
    SHORT sBtn2;
    SHORT sBtn1;
    VPSZ  vpszCaption;
    VPSZ  vpszText;
} KSYSERRORBOX16;
typedef KSYSERRORBOX16 UNALIGNED *PKSYSERRORBOX16;

typedef struct _WOWISKNOWNDLL16 {                          /* k197 */
    VPVOID lplpszKnownDLLPath;
    VPVOID lpszPath;
} WOWISKNOWNDLL16;

typedef struct _WOWDDEFREEHANDLE16 {                       /* k198 */
    WORD   h16;
} WOWDDEFREEHANDLE16;
typedef WOWDDEFREEHANDLE16 UNALIGNED *PWOWDDEFREEHANDLE16;

typedef struct _WOWFILEGETATTRIBUTES16 {                   /* k199 */
    WORD  pszPathOffset;
    WORD  pszPathSegment;
} WOWFILEGETATTRIBUTES16;
typedef WOWFILEGETATTRIBUTES16 UNALIGNED *PWOWFILEGETATTRIBUTES16;

typedef struct _WOWSETDEFAULTDRIVE16 {                     /* k200 */
    WORD  wDriveNum;
} WOWSETDEFAULTDRIVE16;
typedef WOWSETDEFAULTDRIVE16 UNALIGNED *PWOWSETDEFAULTDRIVE16;

typedef struct _WOWGETCURRENTDIRECTORY16 {                 /* k201 */
    DWORD lpCurDir;
    WORD  wDriveNum;
} WOWGETCURRENTDIRECTORY16;
typedef WOWGETCURRENTDIRECTORY16 UNALIGNED *PWOWGETCURRENTDIRECTORY16;

typedef struct _GETPRODUCTNAME16 {                         /* k202 */
    WORD  cbBuffer;
    VPSTR lpBuffer;
} GETPRODUCTNAME16;
typedef GETPRODUCTNAME16 UNALIGNED *PGETPRODUCTNAME16;

#ifdef NULLSTRUCT
typedef struct _ISSAFEMODE16 {                             /* k203 */
} ISSAFEMODE16;
typedef ISSAFEMODE16 UNALIGNED *PISSAFEMODE16;
#endif

typedef struct _WOWSHUTDOWNTIMER16 {                       /* k205 */
    WORD fEnable;
} WOWSHUTDOWNTIMER16;
typedef WOWSHUTDOWNTIMER16 UNALIGNED *PWOWSHUTDOWNTIMER16;

#ifdef NULLSTRUCT
typedef struct _WOWTRIMWORKINGSET16 {                   /* k206 */
} WOWTRIMWORKINGSET16;
typedef WOWTRIMWORKINGSET16 UNALIGNED *PWOWTRIMWORKINGSET16;
#endif

#ifdef FE_SB
#ifdef NULLSTRUCT
typedef struct _GETSYSTEMDEFAULTLANGID16 {                   /* k207 */
} GETSYSTEMDEFAULTLANGID16;
typedef GETSYSTEMDEFAULTLANGID16 UNALIGNED *PGETSYSTEMDEFAULTLANGID16;
#endif
#endif

typedef struct _TERMSRVGETWINDIR16 {                            /* k208 */
    WORD  usPathLen;
    WORD  pszPathOffset;
    WORD  pszPathSegment;
} TERMSRVGETWINDIR16;
typedef TERMSRVGETWINDIR16 UNALIGNED *PTERMSRVGETWINDIR16;


/* XLATOFF */
#pragma pack()
/* XLATON */
