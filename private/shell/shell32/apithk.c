//
//  APITHK.C
//
//  This file has API thunks that allow shell32 to load and run on
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


#include "shellprv.h"       // Don't use precompiled header here
#include "msi.h"
#include <appmgmt.h>
#include <userenv.h>

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

// GetVolumeNameForVolumeMountPoint
typedef BOOL (__stdcall * PFNGETVOLUMENAMEFORVOLUMEMOUNTPOINTW)(
        LPCWSTR lpszFileName,
        LPWSTR lpszVolumePathName,
        DWORD cchBufferLength);

/*----------------------------------------------------------
Purpose: Thunk for NT 5's GetVolumeNameForVolumeMountPointW
*/
BOOL NT5_GetVolumeNameForVolumeMountPointW(LPCWSTR lpszFileName,
                LPWSTR lpszVolumePathName, DWORD cchBufferLength)
{
    static PFNGETVOLUMENAMEFORVOLUMEMOUNTPOINTW s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = _QuickLoadLibrary(TEXT("KERNEL32.DLL"));
        if (hinst)
            s_pfn = (PFNGETVOLUMENAMEFORVOLUMEMOUNTPOINTW)GetProcAddress(hinst, "GetVolumeNameForVolumeMountPointW");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(lpszFileName,lpszVolumePathName,cchBufferLength);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;    
}

// GetVolumePathName
typedef BOOL (__stdcall * PFNGETVOLUMEPATHNAMEW)(
        LPCWSTR lpszFileName,
        LPWSTR lpszVolumePathName,
        DWORD cchBufferLength);

/*----------------------------------------------------------
Purpose: Thunk for NT 5's GetVolumeNameForVolumeMountPointW
*/
BOOL NT5_GetVolumePathNameW(LPCWSTR lpszFileName, LPWSTR lpszVolumePathName,
                DWORD cchBufferLength)
{
    static PFNGETVOLUMEPATHNAMEW s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = _QuickLoadLibrary(TEXT("KERNEL32.DLL"));
        if (hinst)
            s_pfn = (PFNGETVOLUMEPATHNAMEW)GetProcAddress(hinst, "GetVolumePathNameW");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(lpszFileName,lpszVolumePathName,cchBufferLength);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;    
}

// EncryptFileW
typedef BOOL (__stdcall * PFNENCRYPTFILEW)(LPCWSTR lpFileName);

/*----------------------------------------------------------
Purpose: Thunk for NT 5's EncryptFileW
*/
BOOL NT5_EncryptFileW(LPCWSTR lpFileName)
{
    static PFNENCRYPTFILEW s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = _QuickLoadLibrary(TEXT("ADVAPI32.DLL"));

        if (hinst)
            s_pfn = (PFNENCRYPTFILEW)GetProcAddress(hinst, "EncryptFileW");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(lpFileName);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;    
}


// DecryptFileW
typedef BOOL (__stdcall * PFNDECRYPTFILEW)(LPCWSTR lpFileName, DWORD dwReserved);

/*----------------------------------------------------------
Purpose: Thunk for NT 5's DecryptFileW
*/
BOOL NT5_DecryptFileW(LPCWSTR lpFileName, DWORD dwReserved)
{
    static PFNDECRYPTFILEW s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = _QuickLoadLibrary(TEXT("ADVAPI32.DLL"));

        if (hinst)
            s_pfn = (PFNDECRYPTFILEW)GetProcAddress(hinst, "DecryptFileW");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(lpFileName, dwReserved);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;    
}


// InstallApplication
typedef DWORD (__stdcall * PFNINSTALLAPPLICATION)(PINSTALLDATA pInstallInfo);

/*----------------------------------------------------------
Purpose: Thunk for NT 5's InstallApplication
*/
DWORD NT5_InstallApplication(PINSTALLDATA pInstallInfo)
{
    static PFNINSTALLAPPLICATION s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibrary(TEXT("ADVAPI32.DLL"));

        if (hinst)
            s_pfn = (PFNINSTALLAPPLICATION)GetProcAddress(hinst, "InstallApplication");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(pInstallInfo);

    return ERROR_CALL_NOT_IMPLEMENTED;    
}

typedef INSTALLSTATE (__stdcall *PFNMSIQUERYFEATURESTATEFROMDESCRITORW)(LPCWSTR);
BOOL IsDarwinAdW(LPCWSTR pszDescriptor)
{
    static PFNMSIQUERYFEATURESTATEFROMDESCRITORW s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibrary(TEXT("MSI.DLL"));

        if (hinst)
            s_pfn = (PFNMSIQUERYFEATURESTATEFROMDESCRITORW)
                GetProcAddress(hinst, "MsiQueryFeatureStateFromDescriptorW");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(pszDescriptor) == INSTALLSTATE_ADVERTISED;

    return FALSE;    
}

//---- 
int GetCOLOR_HOTLIGHT()
{
    return (g_bRunOnNT5 || g_bRunOnMemphis) ? COLOR_HOTLIGHT : COLOR_HIGHLIGHT;
}

// these two functions are duplicated from browseui
HINSTANCE GetComctl32Hinst()
{
    static HINSTANCE s_hinst = NULL;
    if (!s_hinst)
        s_hinst = GetModuleHandle(TEXT("comctl32.dll"));
    return s_hinst;
}
STDAPI_(HCURSOR) LoadHandCursor(DWORD dwRes)
{
    if(g_bRunOnNT5 || g_bRunOnMemphis)
    {
        HCURSOR hcur = LoadCursor(NULL, IDC_HAND);  // from USER, system supplied
        if (hcur)
            return hcur;
    }
    return LoadCursor(GetComctl32Hinst(), IDC_HAND_INTERNAL);
}

// GlobalMemoryStatusEx

BOOL NT5_EmulateGlobalMemoryStatusEx(LPMEMORYSTATUSEX pmsex)
{
    if (pmsex->dwLength == sizeof(MEMORYSTATUSEX))
    {
        MEMORYSTATUS ms;
        GlobalMemoryStatus(&ms);
        pmsex->dwMemoryLoad            = ms.dwMemoryLoad;
        pmsex->ullTotalPhys            = ms.dwTotalPhys;
        pmsex->ullAvailPhys            = ms.dwAvailPhys;
        pmsex->ullTotalPageFile        = ms.dwTotalPageFile;
        pmsex->ullAvailPageFile        = ms.dwAvailPageFile;
        pmsex->ullTotalVirtual         = ms.dwTotalVirtual;
        pmsex->ullAvailVirtual         = ms.dwAvailVirtual;
        pmsex->ullAvailExtendedVirtual = 0;
        return TRUE;
    } else {
        return FALSE;
    }
}


typedef BOOL (__stdcall * PFNGLOBALMEMORYSTATUSEX)(LPMEMORYSTATUSEX pmsex);

/*----------------------------------------------------------
Purpose: Thunk for NT 5's GlobalMemoryStatusEx, with fallback on Win9x/NT4
*/
BOOL NT5_GlobalMemoryStatusEx(LPMEMORYSTATUSEX pmsex)
{
    static PFNGLOBALMEMORYSTATUSEX s_pfn;

    if (s_pfn == NULL)
    {
        HINSTANCE hinst = _QuickLoadLibrary(TEXT("KERNEL32.DLL"));

        if (hinst)
            s_pfn = (PFNGLOBALMEMORYSTATUSEX)GetProcAddress(hinst, "GlobalMemoryStatusEx");

        if (s_pfn == NULL)
            s_pfn = NT5_EmulateGlobalMemoryStatusEx;
    }

    return s_pfn(pmsex);
}

BOOL IsColorKey(RGBQUAD rgbPixel, COLORREF crKey)
{
    // COLORREF is backwards to RGBQUAD
    return InRange( rgbPixel.rgbBlue,  ((crKey & 0xFF0000) >> 16) - 5, ((crKey & 0xFF0000) >> 16) + 5) &&
           InRange( rgbPixel.rgbGreen, ((crKey & 0x00FF00) >>  8) - 5, ((crKey & 0x00FF00) >>  8) + 5) &&
           InRange( rgbPixel.rgbRed,   ((crKey & 0x0000FF) >>  0) - 5, ((crKey & 0x0000FF) >>  0) + 5);
}


/*----------------------------------------------------------*/

typedef BOOL (* PFNUPDATELAYEREDWINDOW)
    (HWND hwnd, 
    HDC hdcDest,
    POINT *pptDst,
    SIZE *psize,
    HDC hdcSrc,
    POINT *pptSrc,
    COLORREF crKey,
    BLENDFUNCTION *pblend,
    DWORD dwFlags);

BOOL NT5_UpdateLayeredWindow(HWND hwnd, HDC hdcDest, POINT* pptDest, SIZE* psize, 
                        HDC hdcSrc, POINT* pptSrc, COLORREF crKey, BLENDFUNCTION* pblend, DWORD dwFlags)
{
    BOOL bRet = FALSE;
#if(_WIN32_WINNT >= 0x0500)
    static PFNUPDATELAYEREDWINDOW pfn = NULL;

    if (NULL == pfn)
    {
        HMODULE hmod = GetModuleHandle(TEXT("USER32"));
        
        if (hmod)
            pfn = (PFNUPDATELAYEREDWINDOW)GetProcAddress(hmod, "UpdateLayeredWindow");
    }

    if (pfn)
    {
        // The user implementation is lame and does not implement this functionality
        BITMAPINFO      bmi;
        HDC             hdcRGBA;
        HBITMAP         hbmRGBA;
        VOID*           pBits;
        LONG            i;
        BLENDFUNCTION   blend;
        ULONG*          pul;
        POINT           ptSrc;
        BOOL            bRet;
 
        hdcRGBA = NULL;
 
        if ((dwFlags & (ULW_ALPHA | ULW_COLORKEY)) == (ULW_ALPHA | ULW_COLORKEY))
        {
            if (hdcSrc)
            {
                RtlZeroMemory(&bmi, sizeof(bmi));
        
                bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
                bmi.bmiHeader.biWidth       = psize->cx;
                bmi.bmiHeader.biHeight      = psize->cy;
                bmi.bmiHeader.biPlanes      = 1;
                bmi.bmiHeader.biBitCount    = 32;
                bmi.bmiHeader.biCompression = BI_RGB;
        
                hbmRGBA = CreateDIBSection(hdcDest,
                                           &bmi,
                                           DIB_RGB_COLORS,
                                           &pBits,
                                           NULL,
                                           0);
    
                hdcRGBA = CreateCompatibleDC(hdcDest);
    
                if (!hbmRGBA || !hdcRGBA)
                {
                    DeleteObject(hbmRGBA);
                    DeleteObject(hdcRGBA);
                    return(FALSE);
                }
    
                SelectObject(hdcRGBA, hbmRGBA);
    
                BitBlt(hdcRGBA, 0, 0, psize->cx, psize->cy,
                       hdcSrc, pptSrc->x, pptSrc->y, SRCCOPY);
    
                pul = pBits;
    
                for (i = psize->cx * psize->cy; i != 0; i--)
                {
                    if (IsColorKey(*(RGBQUAD*)pul, crKey))
                    {
                        // Write a pre-multiplied value of 0:
 
                        *pul = 0;
                    }
                    else
                    {
                        // Where the bitmap is not the transparent color, change the
                        // alpha value to opaque:
    
                        ((RGBQUAD*) pul)->rgbReserved = 0xff;
                    }
    
                    pul++;
                }
 
                // Change the parameters to account for the fact that we're now
                // providing only a 32-bit per-pixel alpha source:
 
                ptSrc.x = 0;
                ptSrc.y = 0;
                pptSrc = &ptSrc;
                hdcSrc = hdcRGBA;
            }
 
            blend = *pblend;
            blend.AlphaFormat = AC_SRC_ALPHA;   
 
            pblend = &blend;
            dwFlags = ULW_ALPHA;
        }

        bRet = pfn(hwnd, hdcDest, pptDest, psize, hdcSrc, pptSrc, crKey, pblend, dwFlags);

        if (hdcRGBA)
        {
            DeleteObject(hdcRGBA);
            DeleteObject(hbmRGBA);
        }
    }
#endif
    return bRet;    
}

BOOL IsRemoteSession()
{
    return GetSystemMetrics(SM_REMOTESESSION);
}

// GetLongPathName

BOOL NT5_EmulateGetLongPathName(LPCTSTR pszShort, LPTSTR pszLong, DWORD cchBuf)
{
    //  To convert a short name to a long name, convert it to a pidl
    //  and then convert the pidl to a long name.  This is bad because
    //  if the file lives inside a junction point, we end up loading
    //  the junction handler even though we don't use it for anything!

    LPITEMIDLIST pidl;
    BOOL fRc = FALSE;

    ASSERT(cchBuf >= MAX_PATH);

    pidl = ILCreateFromPath(pszShort);

    if (pidl)
    {
        fRc = SHGetPathFromIDList(pidl, pszLong);
        ILFree(pidl);
    }

    return fRc;
}


typedef BOOL (__stdcall * PFNGETLONGPATHNAME)(LPCTSTR pszShort, LPTSTR pszLong, DWORD cchBuf);

/*----------------------------------------------------------
Purpose: Thunk for NT4/Win98's GetLongPathName, with fallback on Win95
*/
BOOL NT5_GetLongPathName(LPCTSTR pszShort, LPTSTR pszLong, DWORD cchBuf)
{
    static PFNGETLONGPATHNAME s_pfn;
    if (s_pfn == NULL)
    {
        HINSTANCE hinst = _QuickLoadLibrary(TEXT("KERNEL32.DLL"));

        if (hinst)
#ifdef UNICODE
            s_pfn = (PFNGETLONGPATHNAME)GetProcAddress(hinst, "GetLongPathNameW");
#else
            s_pfn = (PFNGETLONGPATHNAME)GetProcAddress(hinst, "GetLongPathNameA");
#endif
        if (s_pfn == NULL)
            s_pfn = NT5_EmulateGetLongPathName;
    }
    return s_pfn(pszShort, pszLong, cchBuf);
}

typedef HDEVNOTIFY (__stdcall * PFNREGISTERDEVICENOTIFICATION)(
    IN HANDLE hRecipient,
    IN LPVOID NotificationFilter,
    IN DWORD Flags
    );

/*----------------------------------------------------------
Purpose: Thunk for NT5/Win98's RegisterDeviceNotification
*/
STDAPI_(HDEVNOTIFY) Win98_RegisterDeviceNotification(
    IN HANDLE hRecipient,
    IN LPVOID NotificationFilter,
    IN DWORD Flags
    )
{
    static PFNREGISTERDEVICENOTIFICATION s_pfn = PFN_FIRSTTIME;
    if (s_pfn == PFN_FIRSTTIME)
    {
        HINSTANCE hinst = _QuickLoadLibrary(TEXT("USER32.DLL"));

        if (hinst)
#ifdef UNICODE
            s_pfn = (PFNREGISTERDEVICENOTIFICATION)GetProcAddress(hinst, "RegisterDeviceNotificationW");
#else
            s_pfn = (PFNREGISTERDEVICENOTIFICATION)GetProcAddress(hinst, "RegisterDeviceNotificationA");
#endif
        else
            s_pfn = NULL;
    }

    if (s_pfn)
    {
        return s_pfn(hRecipient, NotificationFilter, Flags);
    }
    else
    {
        return NULL;
    }
}



typedef BOOL (__stdcall * PFNUNREGISTERDEVICENOTIFICATION)(
    IN HDEVNOTIFY Handle
    );

/*----------------------------------------------------------
Purpose: Thunk for NT5/Win98's UnregisterDeviceNotification
*/
STDAPI_(BOOL) Win98_UnregisterDeviceNotification(
    IN HDEVNOTIFY Handle
    )
{
    static PFNUNREGISTERDEVICENOTIFICATION s_pfn = PFN_FIRSTTIME;
    if (s_pfn == PFN_FIRSTTIME)
    {
        HINSTANCE hinst = _QuickLoadLibrary(TEXT("USER32.DLL"));

        if (hinst)
            s_pfn = (PFNUNREGISTERDEVICENOTIFICATION)GetProcAddress(hinst, "UnregisterDeviceNotification");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
    {
        return s_pfn(Handle);
    }
    else
    {
        return FALSE;
    }
}



#ifdef WINNT
typedef UINT (__stdcall * PFNGETSYSTEMWINDOWSDIRECTORYW)(LPWSTR pwszBuffer, UINT cchBuff);

//
// GetSystemWindowsDirectory is NT5 only. Basically this api exists because on HYDRA systems
// kernel32 lies and returns a directory under %userprofile% to "help app compat on hydra"
//
UINT NT5_GetSystemWindowsDirectoryW(LPWSTR pwszBuffer, UINT cchBuff)
{
    static PFNGETSYSTEMWINDOWSDIRECTORYW s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = _QuickLoadLibrary(TEXT("KERNEL32.DLL"));

        if (hinst)
            s_pfn = (PFNGETSYSTEMWINDOWSDIRECTORYW)GetProcAddress(hinst, "GetSystemWindowsDirectoryW");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
    {
        // we use the new API so we dont get lied to by hydra
        return s_pfn(pwszBuffer, cchBuff);
    }
    else
    {
        // must be on NT4, fall back to the ol'e GetWindowsDirectory
        return GetWindowsDirectoryW(pwszBuffer, cchBuff);
    }
}
#endif // WINNT

#ifdef WINNT

typedef BOOL (__stdcall * PFNCONVERTSIDTOSTRINGSIDW)(
    IN   PSID               psid,
    OUT  LPTSTR *           StringSid
);

BOOL NT5_ConvertSidToStringSidW(
    IN   PSID               psid,
    OUT  LPTSTR *           StringSid
)
{
    static PFNCONVERTSIDTOSTRINGSIDW s_pfn = PFN_FIRSTTIME;
    if (s_pfn == PFN_FIRSTTIME)
        s_pfn = (PFNCONVERTSIDTOSTRINGSIDW)GetProcAddress(_QuickLoadLibrary(TEXT("ADVAPI32.DLL")), "ConvertSidToStringSidW");

    if (s_pfn)
        return s_pfn(psid, StringSid);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

#endif

// GetSystemDefaultUILanguage
typedef LANGID (__stdcall * PFNGETSYSTEMDEFAULTUILANGUAGE)();

/*----------------------------------------------------------
Purpose: Thunk for NT 5's GetSystemDefaultUILanguage
*/
LANGID NT5_GetSystemDefaultUILanguage()
{
    static PFNGETSYSTEMDEFAULTUILANGUAGE s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibrary(TEXT("KERNEL32.DLL"));

        if (hinst)
            s_pfn = (PFNGETSYSTEMDEFAULTUILANGUAGE)
                GetProcAddress(hinst, "GetSystemDefaultUILanguage");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn();

    return MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT);
}


// MsiGetProductInfo
typedef UINT (WINAPI * PFNMSIGETPRODUCTINFO) (LPCTSTR szProduct, LPCTSTR szAttribute, LPTSTR lpValueBuf, DWORD *pcchValueBuf); 

/*----------------------------------------------------------
Purpose: Thunk for NT 5's MsiGetProductInfo
*/
UINT MSI_MsiGetProductInfo(LPCTSTR szProduct, LPCTSTR szAttribute, LPTSTR lpValueBuf, DWORD* pcchValueBuf)
{
    static PFNMSIGETPRODUCTINFO s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibrary(TEXT("MSI.DLL"));

        if (hinst)
        {
            // The unicode-decorated MSI APIs translate to ansi internally
            // on Win95, so it should be safe to call them all the time.
#ifdef UNICODE 
            s_pfn = (PFNMSIGETPRODUCTINFO)GetProcAddress(hinst, "MsiGetProductInfoW");
#else
            s_pfn = (PFNMSIGETPRODUCTINFO)GetProcAddress(hinst, "MsiGetProductInfoA");
#endif
        }
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(szProduct, szAttribute, lpValueBuf, pcchValueBuf);

    return ERROR_CALL_NOT_IMPLEMENTED;
}


typedef BOOL (__stdcall * PFNTERMSRVAPPINSTALLMODE)(void);

//
// This function is used by hydra (Terminal Server) to see if we
// are in application install mode
//
BOOL NT5_TermsrvAppInstallMode()
{
    static PFNTERMSRVAPPINSTALLMODE s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = _QuickLoadLibrary(TEXT("KERNEL32.DLL"));

        if (hinst)
        {
            s_pfn = (PFNTERMSRVAPPINSTALLMODE)GetProcAddress(hinst, "TermsrvAppInstallMode");
        }
        else
        {
            s_pfn = NULL;
        }
    }

    if (s_pfn)
    {
        return s_pfn();
    }

    return FALSE;
}

LPTSTR GetEnvBlock(HANDLE hUserToken)
{
    LPTSTR pszRet = NULL;
#ifdef WINNT  
    if (hUserToken && IsOS(OS_NT5))
        CreateEnvironmentBlock(&pszRet, hUserToken, TRUE);
    else
#endif WINNT
        pszRet = (LPTSTR) GetEnvironmentStrings();

    return pszRet;
}

void FreeEnvBlock(HANDLE hUserToken, LPTSTR pszEnv)
{
    if (pszEnv)
    {
#ifdef WINNT  
        if (hUserToken && IsOS(OS_NT5))
            DestroyEnvironmentBlock(pszEnv);
        else
#endif WINNT
            FreeEnvironmentStrings(pszEnv);
    }
}

STDAPI_(BOOL) IsWM_GETOBJECT( UINT uMsg )
{
#if WINVER >= 0x0500 
    return WM_GETOBJECT == uMsg;
#else
    return FALSE;
#endif
}


STDAPI_(BOOL) GetAllUsersDirectory(LPTSTR pszPath)
{
#ifdef WINNT
    DWORD cbData = MAX_PATH;
    return GetAllUsersProfileDirectoryW(pszPath, &cbData);
#else    
    return GetSystemWindowsDirectory(pszPath, MAX_PATH);
#endif
}


#ifdef WINNT

#undef ExpandEnvironmentStringsForUserW

STDAPI_(BOOL) NT5_ExpandEnvironmentStringsForUserW(HANDLE hToken, LPCWSTR pszExpand, LPWSTR pszOut, DWORD cchOut)
{
    if (g_bRunOnNT5)
    {
        return ExpandEnvironmentStringsForUserW(hToken, pszExpand, pszOut, cchOut);
    }
    
    return SHExpandEnvironmentStrings(pszExpand, pszOut, cchOut);
}
#endif // WINNT

#undef GetDefaultUserProfileDirectoryW

STDAPI_(BOOL) NT5_GetDefaultUserProfileDirectoryW(LPWSTR pszPath, LPDWORD pcch)
{
    BOOL bRet;
    
    if (g_bRunOnNT5)
        bRet = GetDefaultUserProfileDirectoryW(pszPath, pcch);
    else
    {
        *pcch = 0;
        *pszPath = 0;
        bRet = FALSE;
    }

    return bRet;
}
