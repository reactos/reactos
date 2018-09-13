//
//  APITHK.C
//
//  This file has API thunks that allow Explorer to load and run on
//  multiple versions of NT or Win95.  Since this component needs
//  to load on the base-level NT 4.0 and Win95, any calls to system
//  APIs introduced in later OS versions must be done via GetProcAddress.
// 
//  Also, any code that may need to access data structures that are
//  post-4.0 specific can be added here.
//
//  NOTE:  this file does *not* use the standard precompiled header,
//         so it can set _WIN32_WINNT to a later version.
//


#include "cabinet.h"        // Don't use precompiled header here

#ifdef WINNT
// NT APM support
#include <ntddapmt.h>
#else
// Win95 APM support
#include <vmm.h>
#include <bios.h>
#define NOPOWERSTATUSDEFINES
#include <pwrioctl.h>
#include <wshioctl.h>
#include <dbt.h>
#include <pbt.h>

#define Not_VxD
#define No_CM_Calls
#include <configmg.h>
#endif // WINNT

BOOL GetDockedState();      //tray.c
#define DOCKSTATE_DOCKED            0
#define DOCKSTATE_UNDOCKED          1
#define DOCKSTATE_UNKNOWN           2

/*----------------------------------------------------------
Purpose: Attempt to get the module handle first (in case the
         DLL is already loaded).  If that doesn't work,
         use LoadLibrary.  

         Warning!!  Be careful how you use this function.
         Theoretically, the 1st thread could use LoadLibrary,
         the 2nd thread could use GetModuleHandle, then
         the 1st thread would FreeLibrary, and suddenly 
         the 2nd thread would have an invalid handle.

         The saving grace is -- for these functions -- the
         DLL is never freed.  But if there's a chance the
         DLL can be freed, don't use this function, just
         use LoadLibrary.
        
*/
HINSTANCE _QuickLoadLibrary(LPCTSTR pszModule)
{
    HINSTANCE hinst = GetModuleHandle(pszModule);  // this is fast

    // Since we don't actually bump up the loader count, the module
    // had better be one of the following that is not going to be
    // unloaded except at process termination.
    ASSERT(hinst && 
           (!lstrcmpi(pszModule, TEXT("KERNEL32.DLL"))  || 
            !lstrcmpi(pszModule, TEXT("USER32.DLL"))    ||
            !lstrcmpi(pszModule, TEXT("GDI32.DLL"))     ||
            !lstrcmpi(pszModule, TEXT("ADVAPI32.DLL"))));

    if (NULL == hinst)
        hinst = LoadLibrary(pszModule);

    return hinst;
}

#define PFN_FIRSTTIME   ((void *) -1)



#if 0  // Here is an example of how shell32 does it

typedef BOOL (__stdcall * PFNENCRYPTFILEW)(LPCWSTR lpFileName);

/*----------------------------------------------------------
Purpose: Thunk for NT 5's EncryptFileW
*/
BOOL NT5_EncryptFileW(LPCWSTR lpFileName)
{
    static PFNENCRYPTFILEW s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        // !! WARNING !! use LoadLibrary instead of QuickLoadLibrary for things
        // that could possibly get freed.
        HINSTANCE hinst = _QuickLoadLibrary(TEXT("ADVAPI32.DLL"));

        if (hinst)
        {
            s_pfn = (PFNENCRYPTFILEW)GetProcAddress(hinst, "EncryptFileW");
        }
        else
        {
            s_pfn = NULL;
        }
    }

    if (s_pfn)
    {
        return s_pfn(lpFileName);
    }

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;    
}
#endif  // End of example



typedef void (__stdcall * PFNISEJECTALLOWED)(PBOOL pfPresent);

/*----------------------------------------------------------
Purpose: Thunk for Memphis' CM_Is_Dock_Station_Present.

Returns: If the Eject PC option is available
Cond:    
*/
BOOL IsEjectAllowed(BOOL fForceUpdateCache)
{
    static PFNISEJECTALLOWED s_pfnIsEjectAllowed = PFN_FIRSTTIME;
    static BOOL fCachedEjectAllowed = FALSE;

    if (PFN_FIRSTTIME == s_pfnIsEjectAllowed)
    {
        HINSTANCE hinst = LoadLibrary(TEXT("CfgMgr32"));
        
        if (hinst)
        {
            s_pfnIsEjectAllowed = (PFNISEJECTALLOWED)GetProcAddress(hinst, "CM_Is_Dock_Station_Present");
        }
        else
        {
            FreeLibrary(hinst);
            s_pfnIsEjectAllowed = NULL;
        }
    }
    else
    {
        // we called the function before and the caller did not
        // pass fForceUpdateCache so just use the cached value
        if (s_pfnIsEjectAllowed && !fForceUpdateCache)
        {
            return fCachedEjectAllowed;
        }
    }


    if (s_pfnIsEjectAllowed)
    {
        BOOL fEject;

        s_pfnIsEjectAllowed(&fEject);
        fCachedEjectAllowed = fEject;
        return fCachedEjectAllowed;
    }
#ifdef WINNT

    return FALSE;
#else
    else
    {
        return (g_ts.hBIOS != INVALID_HANDLE_VALUE && GetDockedState() == DOCKSTATE_DOCKED);
    }
#endif
}


typedef void (__stdcall * PFNDOEJECTPC)();

/*----------------------------------------------------------
Purpose: Thunk for Memphis' CM_Request_Eject_PC.

Returns: requests an Ejection of a PC
Cond:    --
*/
void DoEjectPC()
{
    static PFNDOEJECTPC s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibrary(TEXT("CfgMgr32"));
        
        if (hinst)
        {
            s_pfn = (PFNDOEJECTPC)GetProcAddress(hinst, "CM_Request_Eject_PC");
        }
        else
        {
            FreeLibrary(hinst);
            s_pfn = NULL;
        }
    }
            
    if (s_pfn)
    {
        s_pfn();
        return;
    }
#ifndef WINNT
    else
    {
        // Old Style Windows 95 eject pc
        BIOSPARAMS bp;
        DWORD cbOut;
        DECLAREWAITCURSOR;

        SetWaitCursor();

        if (g_ts.hBIOS!=INVALID_HANDLE_VALUE)
            DeviceIoControl(
                g_ts.hBIOS, 
                PNPBIOS_SERVICE_SOFTUNDOCK, 
                &bp, 
                SIZEOF(bp), 
                &bp, 
                SIZEOF(bp), 
                &cbOut, 
                NULL);

        ResetWaitCursor();
    }
#endif
}


#ifdef WINNT

typedef BOOL (__stdcall * PFNSETTERMSRVAPPINSTALLMODE)(BOOL);

//
// This function is used by hydra (Terminal Server) to set itself into app
// install mode
//
BOOL NT5_SetTermsrvAppInstallMode(BOOL bState)
{
    static PFNSETTERMSRVAPPINSTALLMODE s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = _QuickLoadLibrary(TEXT("KERNEL32.DLL"));

        if (hinst)
        {
            s_pfn = (PFNSETTERMSRVAPPINSTALLMODE)GetProcAddress(hinst, "SetTermsrvAppInstallMode");
        }
        else
        {
            s_pfn = NULL;
        }
    }

    if (s_pfn)
    {
        return s_pfn(bState);
    }

    return FALSE;
}


#ifdef WINNT

#undef SetTermsrvAppInstallMode
#include <tsappcmp.h>
#include "trayp.h"

ULONG  WINAPI GetTermsrCompatFlagsEx(LPWSTR, LPDWORD, TERMSRV_COMPATIBILITY_CLASS);

BOOL _TryHydra(LPCTSTR pszCmd, RRA_FLAGS *pflags)
{
    // See if we are on NT5, and if the terminal-services is enabled
    if (IsTerminalServicesEnabled() && NT5_SetTermsrvAppInstallMode(TRUE))
    {
        WCHAR   sz[MAX_PATH];
        
        *pflags |= RRA_WAIT; 
        // Changing timing blows up IE 4.0, but IE5 is ok!
        // we are on a TS machine, NT version 4 or 5, with admin priv

        // see if the app-compatability flag is set for this executable
        // to use the special job-objects for executing module

        // get the module name, without the arguments
        if (0 < PathProcessCommand(pszCmd, sz, ARRAYSIZE(sz), 
                               PPCF_NODIRECTORIES))
        {
            ULONG   ulCompat;
            GetTermsrCompatFlagsEx(sz, &ulCompat, CompatibilityApp);

            // if the special flag for this module-name is set...
            if ( ulCompat & TERMSRV_COMPAT_WAIT_USING_JOB_OBJECTS )
            {
                *pflags |= RRA_USEJOBOBJECTS;
            }
        }

        return TRUE;
    }

    return FALSE;
}
#endif //WINNT

//
// 	This code is right out of :  private\tsext\setup\ists\ists.c
//
//  It basically grovles the registry to see if the given product suite
//  in installed. We call this on NT4 since we dont have the VerifyVersionInfo
//  api available.
//
// 	Usage :   Call ValidateProductSuite with the product name, in this
//            case, "Terminal Server".
//
// 	Return:   TRUE  if product is present and enabled
//
BOOL
ValidateProductSuite(
    LPTSTR SuiteName
    )
{
    BOOL rVal = FALSE;
    LONG Rslt;
    HKEY hKey = NULL;
    DWORD Type = 0;
    DWORD Size = 0;
    LPTSTR ProductSuite = NULL;
    LPTSTR p;

    Rslt = RegOpenKey(
        HKEY_LOCAL_MACHINE,
        TEXT("System\\CurrentControlSet\\Control\\ProductOptions"),
        &hKey
        );
    if (Rslt != ERROR_SUCCESS)
        goto exit;

    Rslt = RegQueryValueEx( hKey, TEXT("ProductSuite"), NULL, &Type, NULL, &Size );
    if (Rslt != ERROR_SUCCESS || !Size)
        goto exit;

    ProductSuite = (LPTSTR) LocalAlloc( LPTR, Size );
    if (!ProductSuite)
        goto exit;

    Rslt = RegQueryValueEx( hKey, TEXT("ProductSuite"), NULL, &Type,
        (LPBYTE) ProductSuite, &Size );
     if (Rslt != ERROR_SUCCESS || Type != REG_MULTI_SZ)
        goto exit;

    p = ProductSuite;
    while (*p) {
        if (lstrcmp( p, SuiteName ) == 0) {
            rVal = TRUE;
            break;
        }
        p += (lstrlen( p ) + 1);
    }

exit:
    if (ProductSuite)
        LocalFree( ProductSuite );

    if (hKey)
        RegCloseKey( hKey );

    return rVal;
}


typedef BOOL (__stdcall * PFNVERIFYVERSIONINFOW)(POSVERSIONINFOEXW, DWORD, DWORDLONG);
typedef ULONGLONG (__stdcall * PFNVERSETCONDITIONMASK)(ULONGLONG, ULONG, BYTE);

BOOL IsTerminalServicesEnabled()
{
    // VerSetConditionMask local variable must have this precise name because
    // the VER_SET_CONDITION() macro relies on the name.
    static PFNVERIFYVERSIONINFOW s_pfn = PFN_FIRSTTIME;
    static PFNVERSETCONDITIONMASK VerSetConditionMask = PFN_FIRSTTIME;

    if (g_bRunOnNT5) 
    {
        // In NT5 we need to use the Product Suite APIs
        // Don't static link because it won't load on NT4 systems

        if (PFN_FIRSTTIME == s_pfn)
        {
            HINSTANCE hinst = _QuickLoadLibrary(TEXT("KERNEL32.DLL"));

            if (hinst)
            {
                s_pfn = (PFNVERIFYVERSIONINFOW)GetProcAddress(hinst, "VerifyVersionInfoW");
                VerSetConditionMask = (PFNVERSETCONDITIONMASK)GetProcAddress(hinst, "VerSetConditionMask");
            }
            else
            {
                s_pfn = NULL;
                VerSetConditionMask = NULL;
            }
        }

        if (s_pfn && VerSetConditionMask)
        {
            OSVERSIONINFOEXW osVersionInfo = {0};
            DWORDLONG dwlConditionMask = 0;

            osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
            osVersionInfo.wSuiteMask = VER_SUITE_TERMINAL;
            VER_SET_CONDITION(dwlConditionMask, VER_SUITENAME, VER_AND);

            return s_pfn(&osVersionInfo, VER_SUITENAME, dwlConditionMask);
        }
    }

    // on NT4 (or if we failed the GetProcAddress) we go look in the reigstry
    return ValidateProductSuite(TEXT("Terminal Server"));
}
#endif // WINNT



/*----------------------------------------------------------
Purpose: Checks if system is BiDi locale, and if so sets the 
         date format to DATE_RTLREADING.

         Implemented here since DATE_RTLREADING is implemented
         on NT5, BiDi/Win9x, NT4/Arabic and the flag
         is defined only for NT5/Memphis.
*/
void SetBiDiDateFlags(int *piDateFormat)
{
    TCHAR wchLCIDFontSignature[16];
    CALTYPE defCalendar;
    LCID lcidUserDefault;

    lcidUserDefault = GetUserDefaultLCID();
    if (!lcidUserDefault)
        return;

    // Let's verify the bits we have a BiDi UI locale. This will work for Win9x and NT
    if ((LANG_ARABIC == PRIMARYLANGID(LANGIDFROMLCID(lcidUserDefault))) ||
        (LANG_HEBREW == PRIMARYLANGID(LANGIDFROMLCID(lcidUserDefault))) ||
        ((GetLocaleInfo(LOCALE_USER_DEFAULT,
                        LOCALE_FONTSIGNATURE,
                        (TCHAR *) &wchLCIDFontSignature[0],
                        (sizeof(wchLCIDFontSignature)/sizeof(TCHAR)))) &&
         (wchLCIDFontSignature[7] & (WCHAR)0x0800))
       )
    {
        //
        // Let's verify the calendar type. I reuse the same above buffer here.
        if (GetLocaleInfo(LOCALE_USER_DEFAULT, 
                          LOCALE_ICALENDARTYPE, 
                          (TCHAR *) &wchLCIDFontSignature[0],
                          (sizeof(wchLCIDFontSignature)/sizeof(TCHAR))))
        {
            defCalendar = StrToInt((TCHAR *)&wchLCIDFontSignature[0]);
            if ((defCalendar == CAL_GREGORIAN)              ||
                (defCalendar == CAL_HIJRI)                  ||
                (defCalendar == CAL_GREGORIAN_ARABIC)       ||
                (defCalendar == CAL_HEBREW)                 ||
                (defCalendar == CAL_GREGORIAN_XLIT_ENGLISH) ||
                (defCalendar == CAL_GREGORIAN_XLIT_FRENCH))
            {
                *piDateFormat |= DATE_RTLREADING;
            }
            else
            {
                *piDateFormat |= DATE_LTRREADING;
            }
        }
    }
}

#ifdef WINNT
//---------- start HSHELL_APPCOMMAND handling -----------
extern UINT g_WM_ShellHook;
#define MAIL_DEF_KEY            L"Mail"
LRESULT Task_HandleAppCommand(WPARAM wParam, LPARAM lParam)
{
    if (wParam == HSHELL_APPCOMMAND)
    {
        short cmd = GET_APPCOMMAND_LPARAM(lParam);
        switch (cmd)
        {
        case APPCOMMAND_BROWSER_HOME:
            {
                TCHAR szDefault[MAX_PATH + 80];
                LONG cbSize = sizeof(szDefault);
                LPTSTR pszDotExe;

                if (RegQueryValue(HKEY_CLASSES_ROOT, TEXT("http\\shell\\open\\command"), szDefault, &cbSize) == ERROR_SUCCESS) 
                {
                    // path and exe may be wrapped in quotes, in which case, strip
                    //  them off, and stop with the closing quote (ignoring cmd params).
                    //

                    if (szDefault[0] == TEXT('\"')) 
                    {
                        UINT i;
                        for (i = 0; ((szDefault[i + 1] != TEXT('\"')) && (i < MAX_PATH)); i++)
                            szDefault[i] = szDefault[i+1];
                        szDefault[i] = 0;
                    }
                    // ignoring cmd params
                    if (pszDotExe = StrStr(szDefault, TEXT(".exe ")))
                        pszDotExe[4] = 0;

                    ShellExecute(NULL, NULL, szDefault, NULL, NULL, SW_SHOWNORMAL);
                }
                return TRUE;
            }
        case APPCOMMAND_LAUNCH_MAIL:
            SHRunIndirectRegClientCommand(NULL, MAIL_DEF_KEY);
            return TRUE;
        case APPCOMMAND_BROWSER_SEARCH:
            SHFindFiles(NULL, NULL);
            return TRUE;
        }
    }
    return FALSE;
}
//---------- end HSHELL_APPCOMMAND handling -----------
#endif // WINNT
//
//  Can't assert this in apithk.h since it has casts.
//
#ifdef ASFW_ANY
C_ASSERT(ASFW_ANY == PrivateASFW_ANY);
#endif
