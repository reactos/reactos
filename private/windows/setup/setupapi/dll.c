#include "precomp.h"
#pragma hdrstop

#ifndef UNICODE

#include <locale.h>

#endif


// Delay load support
//
#include <delayimp.h>

EXTERN_C
FARPROC
WINAPI
DelayLoadFailureHook (
    UINT            unReason,
    PDelayLoadInfo  pDelayInfo
    );

PfnDliHook __pfnDliFailureHook = DelayLoadFailureHook;

// When we link statically to dload.lib (for Win98), we need to
// provide the instance handle of our DLL as a symbol named
// 'BaseDllHandle'. This is because the preferred implementation,
// which lives in kernel32.dll, uses this name.
//
#ifdef ANSI_SETUPAPI
HANDLE BaseDllHandle;
#endif

HANDLE MyDllModuleHandle;
HINSTANCE SecurityDllHandle;
BOOL AlreadyCleanedUp;

OSVERSIONINFO OSVersionInfo;

//
// Static strings we retreive once.
//
PCTSTR WindowsDirectory,SystemDirectory,InfDirectory,ConfigDirectory,DriversDirectory,System16Directory;
PCTSTR SystemSourcePath,ServicePackSourcePath,DriverCacheSourcePath;
PCTSTR OsLoaderRelativePath;    // may be NULL
PCTSTR WindowsBackupDirectory;  // Directory to write uninstall backups to
PCTSTR ProcessFileName;         // Filename of app calling setupapi

//
// we only check this once
//
BOOL   GuiSetupInProgress;

//
// various control flags
//
DWORD GlobalSetupFlags = 0;

//
// Multi-sz list of fully-qualified directories where INFs are to be searched for.
//
PCTSTR InfSearchPaths;

//
// Declare a (non-CONST) array of strings that specifies what lines to look for
// in an INF's [ControlFlags] section when determining whether a particular device
// ID should be excluded.  These lines are of the form "ExcludeFromSelect[.<suffix>]",
// where <suffix> is determined and filled in during process attach as an optimization.
//
// The max string length (including NULL) is 32, and there can be a maximum of 3
// such strings.  E.g.: ExcludeFromSelect, ExcludeFromSelect.NT, ExcludeFromSelect.NTAlpha
//
// WARNING!! Be very careful when mucking with the order/number of these entries.  Check
// the assumptions made in devdrv.c!pSetupShouldDevBeExcluded.
//
TCHAR pszExcludeFromSelectList[3][32] = { INFSTR_KEY_EXCLUDEFROMSELECT,
                                          INFSTR_KEY_EXCLUDEFROMSELECT,
                                          INFSTR_KEY_EXCLUDEFROMSELECT
                                        };

DWORD ExcludeFromSelectListUb;  // contains the number of strings in the above list (2 or 3).

//
// Current platform name
//
// BUGBUG (lonnym): We should be using the same platform designations defined in infstr.h
//
#if defined(_AXP64_)
PCTSTR PlatformName = TEXT("axp64");
#elif defined(_ALPHA_)
PCTSTR PlatformName = TEXT("Alpha");
#elif defined(_MIPS_)
PCTSTR PlatformName = TEXT("Mips");
#elif defined(_PPC_)
PCTSTR PlatformName = TEXT("PPC");
#elif defined(_X86_)
PCTSTR PlatformName = TEXT("x86");
#elif defined(_IA64_)
PCTSTR PlatformName = TEXT("ia64");
#endif

BOOL
CommonProcessAttach(
    IN BOOL Attach
    );

PCTSTR
GetDriverCacheSourcePath(
    VOID
    );

PCTSTR
pSetupGetOsLoaderPath(
    VOID
    );

PCTSTR
pSetupGetProcessPath(
    VOID
    );

BOOL
pGetGuiSetupInProgress(
    VOID
    );


BOOL
CfgmgrEntry(
    PVOID hModule,
    ULONG Reason,
    PCONTEXT pContext
    );

//
// Called by CRT when _DllMainCRTStartup is the DLL entry point
//
BOOL
WINAPI
DllMain(
    IN HANDLE DllHandle,
    IN DWORD  Reason,
    IN LPVOID Reserved
    )
{
    BOOL b;

    UNREFERENCED_PARAMETER(Reserved);

    b = TRUE;

    switch(Reason) {

    case DLL_PROCESS_ATTACH:

        AlreadyCleanedUp = FALSE;

        InitCommonControls();
        MyDllModuleHandle = DllHandle;

        //
        // When we link statically to dload.lib (for Win98), we need to
        // provide the instance handle of our DLL as a symbol named
        // 'BaseDllHandle'. This is because the preferred implementation,
        // which lives in kernel32.dll, uses this name.
        //
#ifndef UNICODE
        BaseDllHandle = DllHandle;

        //
        // Initialize the C runtime locale (ANSI version of setupapi.dll only)
        //

        setlocale(LC_ALL,"");
#endif

        SecurityDllHandle = NULL;

        //
        // *THIS MUST ALWAYS BE* before any other calls (other than InitCommonControls)
        //
        if(b = MemoryInitialize(TRUE)) {
            //
            // Dynamically load proc addresses of NT-specific APIs
            // must be before any attach's, but after MemoryInitialize
            //
            InitializeStubFnPtrs();

            if(b = CommonProcessAttach(TRUE)) {
                b = DiamondProcessAttach(TRUE);
            }
        }

#ifdef UNICODE
        //
        // Since we've incorporated cfgmgr32 into setupapi, we need
        // to make sure it gets initialized just like it did when it was
        // its own DLL. - must do AFTER everything else
        //
        if (b) {
            b = CfgmgrEntry(DllHandle, Reason, Reserved);
        }
#endif

        //
        // Initialize code in logapi.c
        //

        InitLogApi();

        //
        // Fall through to process first thread
        //

    case DLL_THREAD_ATTACH:

        if(b) {
            DiamondThreadAttach(TRUE);
        }
        break;

    case DLL_PROCESS_DETACH:

        if(!AlreadyCleanedUp) {

#ifdef UNICODE
            // Since we've incorporated cfgmgr32 into setupapi, we need
            // to make sure it gets uninitialized just like it did when it was
            // its own DLL. - must do BEFORE anything else
            //
            b = CfgmgrEntry(DllHandle, Reason, Reserved);
#endif
            //
            // Process last thread
            //
            DiamondThreadAttach(FALSE);

            //
            // Even if uninitializing cfgmgr32 above returned failure, we go
            // ahead and do as much clean-up as we can.  We do preserve the
            // overall failure/success, however.
            //
            b = (DiamondProcessAttach(FALSE) && b);

            CommonProcessAttach(FALSE);

            CleanUpStubFns();

            if(SecurityDllHandle) {
                FreeLibrary( SecurityDllHandle );
            }

            //
            // Clean up the resources used by logapi.c
            //

            TerminateLogApi();

            //
            // *THIS MUST ALWAYS BE* very last thing
            //
            MemoryInitialize(FALSE);
        }
        break;

    case DLL_THREAD_DETACH:

        DiamondThreadAttach(FALSE);
        break;
    }

    return(b);
}



BOOL
CommonProcessAttach(
    IN BOOL Attach
    )
{
    BOOL b;
    TCHAR Buffer[MAX_PATH];
    PTCHAR p;
    UINT u;

    b = !Attach;

    if(Attach) {

        pSetupInitPlatformPathOverrideSupport(TRUE);
        pSetupInitSourceListSupport(TRUE);
        pSetupInitNetConnectionList(TRUE);
        OsLoaderRelativePath = pSetupGetOsLoaderPath();

        //
        // Fill in system and windows directories.
        //
        ProcessFileName = pSetupGetProcessPath();
        if (ProcessFileName == NULL) {
            goto cleanAll;
        }

        if((u = Dyn_GetSystemWindowsDirectory(Buffer,MAX_PATH))
        && (p = Buffer + u)
        && (WindowsDirectory = DuplicateString(Buffer))) {

            if(ConcatenatePaths(Buffer,TEXT("INF"),MAX_PATH,NULL)
            && (InfDirectory = DuplicateString(Buffer))) {

                *p = 0;

                if(ConcatenatePaths(Buffer,TEXT("SYSTEM"),MAX_PATH,NULL)
                && (System16Directory = DuplicateString(Buffer))) {
                    
                    if((u = GetSystemDirectory(Buffer,MAX_PATH))
                    && (p = Buffer + u)
                    && (SystemDirectory = DuplicateString(Buffer))) {
    
                        if(ConcatenatePaths(Buffer,TEXT("ReinstallBackups"),MAX_PATH,NULL)
                        && (WindowsBackupDirectory = DuplicateString(Buffer))) {
    
                            *p = 0;
    
                            if(ConcatenatePaths(Buffer,TEXT("CONFIG"),MAX_PATH,NULL)
                            && (ConfigDirectory = DuplicateString(Buffer))) {
    
                                *p = 0;
    
                                if(ConcatenatePaths(Buffer,TEXT("DRIVERS"),MAX_PATH,NULL)
                                && (DriversDirectory = DuplicateString(Buffer))) {
    
                                    if((SystemSourcePath = GetSystemSourcePath())
                                       && (ServicePackSourcePath = GetServicePackSourcePath())) {
    
                                        DriverCacheSourcePath = GetDriverCacheSourcePath();
                                        GuiSetupInProgress = pGetGuiSetupInProgress();
    
                                        if(InfSearchPaths = AllocAndReturnDriverSearchList(INFINFO_INF_PATH_LIST_SEARCH)) {
    
                                            if(InitMiniIconList()) {
    
                                                if(InitDrvSearchInProgressList()) {
    
                                                    OSVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    
                                                    if(GetVersionEx(&OSVersionInfo)) {
                                                        //
                                                        // Now fill in our ExcludeFromSelect string list which
                                                        // we pre-compute as an optimization.
                                                        //
                                                        if(OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
                                                            lstrcat(pszExcludeFromSelectList[1],
                                                                    pszNtSuffix
                                                                   );
                                                            lstrcat(pszExcludeFromSelectList[2],
                                                                    pszNtPlatformSuffix
                                                                   );
                                                            ExcludeFromSelectListUb = 3;
                                                        } else {
                                                            lstrcat(pszExcludeFromSelectList[1],
                                                                    pszWinSuffix
                                                                   );
                                                            ExcludeFromSelectListUb = 2;
                                                        }
                                                        //
                                                        // Now lower-case all the strings in this list, so that it
                                                        // doesn't have to be done at each string table lookup.
                                                        //
                                                        for(u = 0; u < ExcludeFromSelectListUb; u++) {
                                                            CharLower(pszExcludeFromSelectList[u]);
                                                        }
    
                                                        b = TRUE;
                                                        goto Done;
                                                    }
    
                                                    goto clean9;
                                                }
    
                                                goto clean8;
                                            }
    
                                            goto clean7;
                                        }
    
                                        goto clean6;
                                    }
    
                                    goto clean5;
                                }
    
                                goto clean4;
                            }
    
                            goto clean3a;
                        }
    
                        goto clean3;
                    }

                    goto clean2a;
                }

                goto clean2;
            }

            goto clean1;
        }

        goto clean0;
    }
clean9:
    DestroyDrvSearchInProgressList();
clean8:
    DestroyMiniIconList();
clean7:
    MyFree(InfSearchPaths);
clean6:
    MyFree(SystemSourcePath);
    if (ServicePackSourcePath) {
        MyFree (ServicePackSourcePath);
    }
    if (DriverCacheSourcePath) {
        MyFree(DriverCacheSourcePath);
    }
clean5:
    MyFree(DriversDirectory);
clean4:
    MyFree(ConfigDirectory);
clean3a:
    MyFree(WindowsBackupDirectory);
clean3:
    MyFree(SystemDirectory);
clean2a:
    MyFree(System16Directory);
clean2:
    MyFree(InfDirectory);
clean1:
    MyFree(WindowsDirectory);
clean0:
    if(OsLoaderRelativePath) {
        MyFree(OsLoaderRelativePath);
    }
    //
    // BugBug!!! (jamiehun) ideally all the above should be merged into something like this case below (save all those labels)
    //
cleanAll:
    if (ProcessFileName != NULL) {
        MyFree(ProcessFileName);
        ProcessFileName = NULL;
    }
    pSetupInitNetConnectionList(FALSE);
    pSetupInitSourceListSupport(FALSE);
    pSetupInitPlatformPathOverrideSupport(FALSE);
    AlreadyCleanedUp = TRUE;
Done:
    return(b);
}

#if MEM_DBG
#undef GetSystemSourcePath          // defined again below
#endif

PCTSTR
GetSystemSourcePath(
    TRACK_ARG_DECLARE
    )
/*++

Routine Description:

    This routine returns a newly-allocated buffer containing the source path from
    which the system was installed, or "A:\" if that value cannot be determined.
    This value is retrieved from the following registry location:

    \HKLM\Software\Microsoft\Windows\CurrentVersion\Setup

        SourcePath : REG_SZ : "\\ntalpha1\1300fre.wks"  // for example.

Arguments:

    None.

Return Value:

    If the function succeeds, the return value is a pointer to the path string.
    This memory must be freed via MyFree().

    If the function fails due to out-of-memory, the return value is NULL.

--*/
{
    HKEY hKey;
    TCHAR CharBuffer[CSTRLEN(REGSTR_PATH_SETUP) + SIZECHARS(REGSTR_KEY_SETUP)];
    DWORD Err, DataType, DataSize;
    PTSTR Value;
    PCTSTR ReturnVal;

    TRACK_PUSH

    CopyMemory(CharBuffer,
               pszPathSetup,
               sizeof(pszPathSetup) - sizeof(TCHAR)
              );
    CopyMemory((PBYTE)CharBuffer + (sizeof(pszPathSetup) - sizeof(TCHAR)),
               pszKeySetup,
               sizeof(pszKeySetup)
              );

    if((Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           CharBuffer,
                           0,
                           KEY_READ,
                           &hKey)) == ERROR_SUCCESS) {
        //
        // Attempt to read the the "SourcePath" value.
        //
        Err = QueryRegistryValue(hKey, pszSourcePath, &Value, &DataType, &DataSize);

        RegCloseKey(hKey);
    }

    ReturnVal = NULL;

    if(Err == NO_ERROR) {

        ReturnVal = Value;

    }

    if(!ReturnVal && Err != ERROR_NOT_ENOUGH_MEMORY) {
        //
        // We failed to retrieve the SourcePath value, and it wasn't due to an out-of-memory
        // condition.  Fall back to our default of "A:\".
        //
        ReturnVal = DuplicateString(pszOemInfDefaultPath);
    }

    TRACK_POP

    return ReturnVal;
}

#if MEM_DBG
#define GetSystemSourcePath()   GetSystemSourcePath(TRACK_ARG_CALL)
#endif



#if MEM_DBG
#undef GetServicePackSourcePath         // defined again below
#endif

PCTSTR
GetServicePackSourcePath(
    TRACK_ARG_DECLARE
    )
/*++

Routine Description:

    This routine returns a newly-allocated buffer containing the service pack source path
    where we should look for service pack source files, or "CDM" if that value cannot be determined.
    This value is retrieved from the following registry location:

    \HKLM\Software\Microsoft\Windows\CurrentVersion\Setup

        ServicePackSourcePath : REG_SZ : "\\ntalpha1\1300fre.wks"  // for example.

Arguments:

    None.

Return Value:

    If the function succeeds, the return value is a pointer to the path string.
    This memory must be freed via MyFree().

    If the function fails due to out-of-memory, the return value is NULL.

--*/
{
    HKEY hKey;
    TCHAR CharBuffer[CSTRLEN(REGSTR_PATH_SETUP) + SIZECHARS(REGSTR_KEY_SETUP)];
    DWORD Err, DataType, DataSize;
    PTSTR Value;
    PCTSTR ReturnStr = NULL;

    TRACK_PUSH

    CopyMemory(CharBuffer,
               pszPathSetup,
               sizeof(pszPathSetup) - sizeof(TCHAR)
              );
    CopyMemory((PBYTE)CharBuffer + (sizeof(pszPathSetup) - sizeof(TCHAR)),
               pszKeySetup,
               sizeof(pszKeySetup)
              );

    if((Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           CharBuffer,
                           0,
                           KEY_READ,
                           &hKey)) == ERROR_SUCCESS) {
        //
        // Attempt to read the the "ServicePackSourcePath" value.
        //
        Err = QueryRegistryValue(hKey, pszSvcPackPath, &Value, &DataType, &DataSize);

        RegCloseKey(hKey);
    }

    if(Err == NO_ERROR) {

        ReturnStr = Value;

    }

    if(!ReturnStr && Err != ERROR_NOT_ENOUGH_MEMORY) {
        //
        // We failed to retrieve the ServicePackSourcePath value, and it wasn't due to an out-of-memory
        // condition.  Fall back to the SourcePath value in the registry
        //

        ReturnStr = GetSystemSourcePath();
    }

    TRACK_POP

    return ReturnStr;
}

#if MEM_DBG
#define GetServicePackSourcePath()   GetServicePackSourcePath(TRACK_ARG_CALL)
#endif



PCTSTR
GetDriverCacheSourcePath(
    VOID
    )
/*++

Routine Description:

    This routine returns a newly-allocated buffer containing the source path to the local driver cache
    cab-file.

    This value is retrieved from the following registry location:

    \HKLM\Software\Microsoft\Windows\CurrentVersion\Setup

        DriverCachePath : REG_SZ : "\\ntalpha1\1300fre.wks"  // for example.

Arguments:

    None.

Return Value:

    If the function succeeds, the return value is a pointer to the path string.
    This memory must be freed via MyFree().

    If the function fails due to out-of-memory, the return value is NULL.

--*/
{
    HKEY hKey;
    TCHAR CharBuffer[CSTRLEN(REGSTR_PATH_SETUP) + SIZECHARS(REGSTR_KEY_SETUP)];
    DWORD Err, DataType, DataSize;
    PTSTR Value;
    TCHAR Path[MAX_PATH];

    CopyMemory(CharBuffer,
               pszPathSetup,
               sizeof(pszPathSetup) - sizeof(TCHAR)
              );
    CopyMemory((PBYTE)CharBuffer + (sizeof(pszPathSetup) - sizeof(TCHAR)),
               pszKeySetup,
               sizeof(pszKeySetup)
              );

    if((Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           CharBuffer,
                           0,
                           KEY_READ,
                           &hKey)) == ERROR_SUCCESS) {
        //
        // Attempt to read the the "DriverCachePath" value.
        //
        Err = QueryRegistryValue(hKey, pszDriverCachePath, &Value, &DataType, &DataSize);

        RegCloseKey(hKey);
    }

    if(Err == NO_ERROR) {
        if(Value) {

            ExpandEnvironmentStrings(Value,Path,MAX_PATH);

            MyFree(Value);

            Value = NULL;

            if (*Path) {
                Value = DuplicateString( Path );
            }

            return (PCTSTR)Value;
        }
    } else if(Err == ERROR_NOT_ENOUGH_MEMORY) {
        return NULL;
    }

    return NULL;

}




BOOL
pSetupSetSystemSourcePath(
    IN PCTSTR NewSourcePath,
    IN PCTSTR NewSvcPackSourcePath
    )
/*++

Routine Description:

    This routine is used to override the system source path used by setupapi (as
    retrieved by GetSystemSourcePath during DLL initialization).  This is used by
    syssetup.dll to set the system source path appropriately during GUI-mode setup,
    so that the device installer APIs will copy files from the correct source location.
    
    We do the same thing for the service pack source path

    NOTE:  This routine IS NOT thread safe!

Arguments:

    NewSourcePath - supplies the new source path to be used.
    NewSvcPackSourcePath - supplies the new svcpack source path to be used.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails (due to out-of-memory), the return value is FALSE.

--*/
{
    PCTSTR p,q;

    p = (PCTSTR)DuplicateString(NewSourcePath);
    q = (PCTSTR)DuplicateString(NewSvcPackSourcePath);

    if(p) {
        MyFree(SystemSourcePath);
        SystemSourcePath = p;        
    }

    if (q) {
        MyFree(ServicePackSourcePath);
        ServicePackSourcePath = q;
    }

    if (!p || !q) {
        //
        // failed due to out of memory!
        //
        return(FALSE);
    }
    
    return TRUE;
}


PCTSTR
pSetupGetOsLoaderPath(
    VOID
    )
/*++

Routine Description:

    This routine returns a newly-allocated buffer containing the path to the OsLoader
    (relative to the system partition drive).  This value is retrieved from the
    following registry location:

        HKLM\System\Setup
            OsLoaderPath : REG_SZ : <path>    // e.g., "\os\winnt40"

Arguments:

    None.

Return Value:

    If the registry entry is found, the return value is a pointer to the string containing
    the path.  The caller must free this buffer via MyFree().

    If the registry entry is not found, or memory cannot be allocated for the buffer, the
    return value is NULL.

--*/
{
    HKEY hKey;
    PTSTR Value;
    DWORD Err, DataType, DataSize;

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    TEXT("SYSTEM\\Setup"),
                    0,
                    KEY_READ,
                    &hKey) == ERROR_SUCCESS) {

        Err = QueryRegistryValue(hKey, TEXT("OsLoaderPath"), &Value, &DataType, &DataSize);

        RegCloseKey(hKey);

        return (Err == NO_ERROR) ? (PCTSTR)Value : NULL;
    }

    return NULL;
}

PCTSTR
pSetupGetProcessPath(
    VOID
    )
/*++

Routine Description:

    Get the name of the EXE that we're running in.

Arguments:

    NONE.

Return Value:

    Pointer to a dynamically allocated string containing the name.

--*/

{
    LPTSTR modname;

    modname = MyMalloc(MAX_PATH * sizeof(TCHAR));

    if(modname != NULL) {
        if(GetModuleFileName(NULL, modname, MAX_PATH) > 0) {
            LPTSTR modname2;
            modname2 = MyRealloc(modname, (lstrlen(modname)+1)*sizeof(TCHAR));
            if(modname2) {
                modname = modname2;
            }
            return modname;
        } else {
            DebugBreak();
            MyFree(modname);
        }
    }

    return NULL;
}

#ifdef UNICODE
BOOL
pGetGuiSetupInProgress(
    VOID
    )
/*++

Routine Description:

    This routine determines if we're doing a gui-mode setup.

    This value is retrieved from the following registry location:

    \HKLM\System\Setup\

        SystemSetupInProgress : REG_DWORD : 0x00 (where nonzero means we're doing a gui-setup)

Arguments:

    None.

Return Value:

    If the function succeeds, the return value is a pointer to the path string.
    This memory must be freed via MyFree().

    If the function fails due to out-of-memory, the return value is NULL.

--*/
{
    HKEY hKey;
    TCHAR CharBuffer[CSTRLEN(REGSTR_PATH_SETUP) + SIZECHARS(REGSTR_KEY_SETUP)];
    DWORD Err, DataType, DataSize = sizeof(DWORD);
    DWORD Value;

    if((Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           TEXT("System\\Setup"),
                           0,
                           KEY_READ,
                           &hKey)) == ERROR_SUCCESS) {
        //
        // Attempt to read the the "DriverCachePath" value.
        //
        Err = RegQueryValueEx(
                    hKey,
                    TEXT("SystemSetupInProgress"),
                    NULL,
                    &DataType,
                    (LPBYTE)&Value,
                    &DataSize);

        RegCloseKey(hKey);
    }

    if(Err == NO_ERROR) {
        if(Value) {
            return(TRUE);
        }
    }

    return(FALSE);

}

#else

BOOL
pGetGuiSetupInProgress(
    VOID
    )
{
    return FALSE;
}

#endif

VOID pSetupSetGlobalFlags(
    IN DWORD Flags
    )
/*++

    exported as a private function
    
Routine Description:

    Sets global flags to change certain setupapi features,
    such as "should we call runonce after installing every device" (set if we will manually call it)
    or "should we backup every file"

Arguments:

    Flags: combination of:
    
        PSPGF_NO_RUNONCE     - set to inhibit runonce calls (e.g., during GUI-
                               mode setup)
                           
        PSPGF_NO_BACKUP      - set to inhibit automatic backup (e.g., during 
                               GUI-mode setup)
        
        PSPGF_NONINTERACTIVE - set to inhibit _all_ UI (e.g., for server-side 
                               device installation)

        PSPGF_SERVER_SIDE_RUNONCE - batch RunOnce entries for server-side
                                    processing (for use only by umpnpmgr)
                                    
        PSPGF_NO_VERIFY_INF  - set to inhibit verification (digital signature) of
                               INF files until after the cyrpto DLLs have been registered.                                    
                                    
Return Value:

    none

--*/
{
    GlobalSetupFlags = Flags;
}

DWORD pSetupGetGlobalFlags(
    VOID
    )
/*++

    exported as a private function, also called internally
    
Routine Description:

    Return flags previously set

Arguments:

    none

Return value:

    Flags (combination of values described above for pSetupSetGlobalFlags)

--*/
{
    return GlobalSetupFlags;
}
