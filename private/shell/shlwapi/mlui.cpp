#include "priv.h"
#include <regapix.h>
#include <htmlhelp.h>
#include <shlwapi.h>
#include <wininet.h>    // INTERNET_MAX_URL_LENGTH
#include "mlui.h"
#include "unicwrap.h"
#include "thunk.h"

//
//  Do this in every wrapper function to make sure the wrapper
//  prototype matches the function it is intending to replace.
//
#define VALIDATE_PROTOTYPE(f) if (f##W == f##WrapW) 0
#define VALIDATE_PROTOTYPE_DELAYLOAD(fWrap, fDelay) if (fDelay##W == fWrap##WrapW) 0
#define VALIDATE_PROTOTYPE_NO_W(f) if (f## == f##Wrap) 0

//
//  Registry Key
//
const CHAR c_szInstall[] = "Software\\Microsoft\\Active Setup\\Installed Components\\{89820200-ECBD-11CF-8B85-00AA005B4383}";
const CHAR c_szLocale[] = "Locale";
const CHAR c_szOffice9[] = "Software\\Microsoft\\Office\\9.0\\Common\\LanguageResources";
const CHAR c_szUILanguage[] = "UILanguage";
const CHAR c_szInternational[] = "Software\\Microsoft\\Internet Explorer\\International";
const CHAR c_szResourceLocale[] = "ResourceLocale";
const WCHAR c_wszAppPaths[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\iexplore.exe";
const WCHAR c_wszMUI[] = L"mui";
const TCHAR s_szLangPackPath[]   = TEXT("Software\\Microsoft\\Internet Explorer");
const TCHAR s_szVersion[] = TEXT("LPKInstalled");
const WCHAR c_wszWebTemplate[] = L"\\Web\\%s";
const WCHAR c_wszMuiTemplate[] = L"\\Web\\mui\\%04x\\%s";
const CHAR c_szNT4ResourceLocale[] = ".DEFAULT\\Control Panel\\International";
const CHAR c_szWin9xResourceLocale[] = ".Default\\Control Panel\\desktop\\ResourceLocale";
const CHAR c_szCheckVersion[] = "CheckVersion";
//
//  ISO639 ID table
//
typedef struct tagISO639
{
    LPCSTR ISO639;
    LANGID LangID;
}   ISO639, *LPISO639;

const ISO639 c_ISO639[] =
{
    { "EN", 0x0409 },
    { "DE", 0x0407 },
    { "JA", 0x0411 },
    { "KO", 0x0412 },
    { "TW", 0x0404 },
    { "CN", 0x0804 },
    { "FR", 0x040C },
    { "ES", 0x0C0A },
    { "BR", 0x0416 },
    { "IT", 0x0410 },
    { "NL", 0x0413 },
    { "SV", 0x041D },
    { "DA", 0x0406 },
    { "FI", 0x040B },
    { "HU", 0x040E },
    { "NO", 0x0414 },
    { "EL", 0x0408 },
    { "PL", 0x0415 },
    { "RU", 0x0419 },
    { "CS", 0x0405 },
    { "PT", 0x0816 },
    { "TR", 0x041F },
    { "SK", 0x041B },
    { "SL", 0x0424 },
    { "AR", 0x0401 },
    { "HE", 0x040D }
};

// NOTE!  See warnings in subclass.c before futzing with the way we manage
// atoms.  Need to be careful to avoid a Win95 bug.
ATOM g_atmML;
#define c_szML  TEXT("shlwapi.ML")


LANGID GetInstallLanguage(void)
{
    static LANGID LangID = 0;

    if (0 == LangID)
    {
        if (g_bRunningOnNT5OrHigher)
        {
            static LANGID (CALLBACK* pfnGetSystemDefaultUILanguage)(void) = NULL;

            if (pfnGetSystemDefaultUILanguage == NULL)
            {
                HMODULE hmod = GetModuleHandle(TEXT("KERNEL32"));

                if (hmod)
                    pfnGetSystemDefaultUILanguage = (LANGID (CALLBACK*)(void))GetProcAddress(hmod, "GetSystemDefaultUILanguage");
            }
            if (pfnGetSystemDefaultUILanguage)
                return pfnGetSystemDefaultUILanguage();
        }
        else
        {
            CHAR szISO639[3];
            DWORD cb;

            cb = sizeof(szISO639);
            if (ERROR_SUCCESS == SHGetValueA(HKEY_LOCAL_MACHINE, c_szInstall, c_szLocale, NULL, szISO639, &cb))
            {
                int i;

                for (i = 0; i < ARRAYSIZE(c_ISO639); i++)
                {
                    if (!StrCmpNIA(szISO639, c_ISO639[i].ISO639, ARRAYSIZE(szISO639)))
                    {
                        LangID = c_ISO639[i].LangID;
                        break;
                    }
                }
            }
        }
    }
    return LangID;
}

//
//  MLGetUILanguage(void)
//
LWSTDAPI_(LANGID) MLGetUILanguage(void)
{
    static LANGID LangID = 0;
    CHAR szLangID[8];

    if (0 == LangID)  // no cached LANGID
    {
        if (g_bRunningOnNT5OrHigher)
        {
            static LANGID (CALLBACK* pfnGetUserDefaultUILanguage)(void) = NULL;

            if (pfnGetUserDefaultUILanguage == NULL)
            {
                HMODULE hmod = GetModuleHandle(TEXT("KERNEL32"));

                if (hmod)
                    pfnGetUserDefaultUILanguage = (LANGID (CALLBACK*)(void))GetProcAddress(hmod, "GetUserDefaultUILanguage");
            }
            if (pfnGetUserDefaultUILanguage)
                return pfnGetUserDefaultUILanguage();
        }
        else
        {
            DWORD   dw;
            
            // try the office 9 lang key (integer value)
            DWORD cb = sizeof(dw);
            if (ERROR_SUCCESS ==
                SHGetValue(HKEY_LOCAL_MACHINE, s_szLangPackPath, s_szVersion, NULL, &dw, &cb)
                && dw > 0) // magic number - christw tells me so
            {
                // Office Language Pack is installed
                cb = sizeof(dw);
                if (ERROR_SUCCESS == SHGetValueA(HKEY_CURRENT_USER, c_szOffice9, c_szUILanguage, NULL, &dw, &cb))
                {
                    LangID = (LANGID)dw;
                }
            }
            // try the IE5 lang key (string rep of hex value)
            if (LangID == 0)
            {
                cb = sizeof(szLangID) - 2;
                if (ERROR_SUCCESS == SHGetValueA(HKEY_CURRENT_USER, c_szInternational, c_szResourceLocale, NULL, szLangID + 2, &cb))
                {
                    // IE uses a string rep of the hex value
                    szLangID[0] = '0';
                    szLangID[1] = 'x';
                    StrToIntEx(szLangID, STIF_SUPPORT_HEX, (LPINT)&LangID);
                }
                else
                    LangID = GetInstallLanguage();
            }
        }
    }
    return LangID;
}

static const TCHAR s_szUrlMon[] = TEXT("urlmon.dll");
static const TCHAR s_szFncFaultInIEFeature[] = TEXT("FaultInIEFeature");
const CLSID CLSID_Satellite =  {0x85e57160,0x2c09,0x11d2,{0xb5,0x46,0x00,0xc0,0x4f,0xc3,0x24,0xa1}};

HRESULT 
_FaultInIEFeature(HWND hwnd, uCLSSPEC *pclsspec, QUERYCONTEXT *pQ, DWORD dwFlags)
{
    HRESULT hr = E_FAIL;
    typedef HRESULT (WINAPI *PFNJIT)(
        HWND hwnd, 
        uCLSSPEC *pclsspec, 
        QUERYCONTEXT *pQ, 
        DWORD dwFlags);
    PFNJIT  pfnJIT = NULL;
    BOOL fDidLoadLib = FALSE;

    HINSTANCE hUrlMon = GetModuleHandle(s_szUrlMon);
    if (!hUrlMon)
    {
        hUrlMon = LoadLibrary(s_szUrlMon);
        fDidLoadLib = TRUE;
    }
    
    if (hUrlMon)
    {
        pfnJIT = (PFNJIT)GetProcAddress(hUrlMon, s_szFncFaultInIEFeature);
    }
    
    if (pfnJIT)
       hr = pfnJIT(hwnd, pclsspec, pQ, dwFlags);
       
    if (fDidLoadLib && hUrlMon)
        FreeLibrary(hUrlMon);

    return hr;
}

#ifdef LATER_IE5
HRESULT InstallIEFeature(HWND hWnd, LCID lcid, const CLSID *clsid)
{
   
    HRESULT     hr  = REGDB_E_CLASSNOTREG;
    uCLSSPEC    classpec;
    QUERYCONTEXT qc = {0};
        
    classpec.tyspec=TYSPEC_CLSID;
    classpec.tagged_union.clsid=*clsid;
    qc.Locale = lcid;

    hr = _FaultInIEFeature(hWnd, &classpec, &qc, FIEF_FLAG_FORCE_JITUI);

    if (hr != S_OK) {
        hr = REGDB_E_CLASSNOTREG;
    }
    return hr;
}
#endif

HRESULT GetMUIPathOfIEFileW(LPWSTR pszMUIFilePath, int cchMUIFilePath, LPCWSTR pcszFileName, LANGID lidUI)
{
    HRESULT hr = S_OK;
    
    ASSERT(pszMUIFilePath);
    ASSERT(pcszFileName);

    // deal with the case that pcszFileName has full path
    LPWSTR pchT = StrRChrW(pcszFileName, NULL, L'\\');
    if (pchT)
    {
        pcszFileName = pchT;
    }

    static WCHAR s_szMUIPath[MAX_PATH] = { L'\0' };
    static LANGID s_lidLast = 0;

    int cchPath;
    DWORD cb;

    // use cached string if possible
    if ( !s_szMUIPath[0] || s_lidLast != lidUI)
    {
        WCHAR szAppPath[MAXIMUM_VALUE_NAME_LENGTH];

        s_lidLast = lidUI;

        cb = sizeof(szAppPath);
        if (ERROR_SUCCESS == SHGetValueW(HKEY_LOCAL_MACHINE, c_wszAppPaths, NULL, NULL, szAppPath, &cb))
            PathRemoveFileSpecW(szAppPath);
        else
            szAppPath[0] = L'0';

        wnsprintfW(s_szMUIPath, cchMUIFilePath, L"%s\\%s\\%04x\\", szAppPath, c_wszMUI, lidUI );
    }
    StrCpyNW(pszMUIFilePath, s_szMUIPath, cchMUIFilePath);
    cchPath = lstrlenW(pszMUIFilePath);
    cchMUIFilePath -= cchPath;
    StrCpyNW(pszMUIFilePath+cchPath, pcszFileName, cchMUIFilePath);

    return hr;
}

HRESULT GetMUIPathOfIEFileA(LPSTR pszMUIFilePath, int cchMUIFilePath, LPCSTR pcszFileName, LANGID lidUI)
{
    WCHAR szMUIFilePath[MAX_PATH];
    WCHAR szFileName[MAX_PATH];
    HRESULT hr;

    SHAnsiToUnicode(pcszFileName, szFileName, ARRAYSIZE(szFileName));
    hr = GetMUIPathOfIEFileW(szMUIFilePath, cchMUIFilePath, szFileName, lidUI);
    if (SUCCEEDED(hr))
        SHUnicodeToAnsi(szMUIFilePath, pszMUIFilePath, cchMUIFilePath);

    return hr;
}

BOOL fDoMungeLangId(LANGID lidUI)
{
    LANGID lidInstall = GetInstallLanguage();
    BOOL fRet = FALSE;

    if (0x0409 != lidUI && lidUI != lidInstall) // US resource is always no need to munge
    {
        CHAR szUICP[8];
        CHAR szInstallCP[8];

        GetLocaleInfoA(MAKELCID(lidUI, SORT_DEFAULT), LOCALE_IDEFAULTANSICODEPAGE, szUICP, ARRAYSIZE(szUICP));
        GetLocaleInfoA(LOCALE_SYSTEM_DEFAULT, LOCALE_IDEFAULTANSICODEPAGE, szInstallCP, ARRAYSIZE(szInstallCP));
        if (0 != lstrcmpiA(szUICP, szInstallCP))
            fRet = TRUE;
    }
    return fRet;
}

#define CP_THAI     874
#define CP_ARABIC   1256
#define CP_HEBREW   1255

LANGID GetNormalizedLangId(DWORD dwFlag)
{
    LANGID lidUI = 0;

    dwFlag &= ML_CROSSCODEPAGE_MASK;
    if (ML_SHELL_LANGUAGE == dwFlag)
    {
        if (g_bRunningOnNT5OrHigher)
        {
            static LANGID (CALLBACK* pfnGetUserDefaultUILanguage)(void) = NULL;

            if (pfnGetUserDefaultUILanguage == NULL)
            {
                HMODULE hmod = GetModuleHandle(TEXT("KERNEL32"));

                if (hmod)
                    pfnGetUserDefaultUILanguage = (LANGID (CALLBACK*)(void))GetProcAddress(hmod, "GetUserDefaultUILanguage");
            }
            if (pfnGetUserDefaultUILanguage)
                lidUI = pfnGetUserDefaultUILanguage();
        }
        else
        {
            CHAR szLangID[12];
            DWORD cb, dwRet;

            cb = sizeof(szLangID) - 2;
            if (g_bRunningOnNT)
                dwRet = SHGetValueA(HKEY_USERS, c_szNT4ResourceLocale, c_szLocale, NULL, szLangID + 2, &cb);
            else
                dwRet = SHGetValueA(HKEY_USERS, c_szWin9xResourceLocale, NULL, NULL, szLangID + 2, &cb);

            if (ERROR_SUCCESS == dwRet)
            {
                // IE uses a string rep of the hex value
                szLangID[0] = '0';
                szLangID[1] = 'x';
                StrToIntEx(szLangID, STIF_SUPPORT_HEX, (LPINT)&lidUI);
            }
        }
    }
    else
    {
        UINT uiACP = GetACP();
        lidUI = MLGetUILanguage();

        // we don't support cross codepage PlugUI on MiddleEast platform
        if (!g_bRunningOnNT5OrHigher && (uiACP == CP_THAI || uiACP == CP_ARABIC || uiACP == CP_HEBREW))
            dwFlag = ML_NO_CROSSCODEPAGE;

        if ((ML_NO_CROSSCODEPAGE == dwFlag) || (!g_bRunningOnNT &&  (ML_CROSSCODEPAGE_NT == dwFlag)))
        {
            if (fDoMungeLangId(lidUI))
                lidUI = 0;
        }
    }
    return (lidUI)? lidUI: GetInstallLanguage();
}

//
//  MLLoadLibrary
//

HDPA g_hdpaPUI = NULL;

typedef struct tagPUIITEM
{
    HINSTANCE hinstRes;
    LANGID lidUI;
    BOOL fMunged;
} PUIITEM, *PPUIITEM;

EXTERN_C BOOL InitPUI(void)
{
    if (NULL == g_hdpaPUI)
    {
        ENTERCRITICAL;
        if (NULL == g_hdpaPUI)
            g_hdpaPUI = DPA_Create(4);
        LEAVECRITICAL;
    }
    return (g_hdpaPUI)? TRUE: FALSE;
}

int GetPUIITEM(HINSTANCE hinst)
{
    int i, cItems = 0;

    ASSERTCRITICAL;

    if (InitPUI())
    {
        cItems = DPA_GetPtrCount(g_hdpaPUI);

        for (i = 0; i < cItems; i++)
        {
            PPUIITEM pItem = (PPUIITEM)DPA_FastGetPtr(g_hdpaPUI, i);

            if (pItem && pItem->hinstRes == hinst)
                break;
        }
    }
    return (i < cItems)? i: -1;
}

EXTERN_C void DeinitPUI(void)
{
    if (g_hdpaPUI)
    {
        ENTERCRITICAL;
        if (g_hdpaPUI)
        {
            int i, cItems = 0;
        
            cItems = DPA_GetPtrCount(g_hdpaPUI);

            // clean up if there is anything left
            for (i = 0; i < cItems; i++)
                LocalFree(DPA_FastGetPtr(g_hdpaPUI, i));

            DPA_DeleteAllPtrs(g_hdpaPUI);
            DPA_Destroy(g_hdpaPUI);
            g_hdpaPUI = NULL;
        }
        LEAVECRITICAL;
    }
}

LWSTDAPI MLSetMLHInstance(HINSTANCE hInst, LANGID lidUI)
{
    int i=-1;
    
    if (hInst)
    {
        PPUIITEM pItem = (PPUIITEM)LocalAlloc(LPTR, sizeof(PUIITEM));

        if (pItem)
        {
            // BUGBUG: We might not need to put this if fMunged is FALSE
            pItem->hinstRes = hInst;
            pItem->lidUI = lidUI;
            pItem->fMunged = fDoMungeLangId(lidUI);
            if (InitPUI())
            {
                ENTERCRITICAL;
                i = DPA_AppendPtr(g_hdpaPUI, pItem);
                LEAVECRITICAL;
                if (-1 == i)
                    LocalFree(pItem);
            }
        }
    }

    return (-1 == i) ? E_OUTOFMEMORY : S_OK;
}

LWSTDAPI MLClearMLHInstance(HINSTANCE hInst)
{
    int i;

    ENTERCRITICAL;
    i = GetPUIITEM(hInst);
    if (0 <= i)
    {
        LocalFree(DPA_FastGetPtr(g_hdpaPUI, i));
        DPA_DeletePtr(g_hdpaPUI, i);
    }
    LEAVECRITICAL;

    return S_OK;
}

LWSTDAPI
SHGetWebFolderFilePathW(LPCWSTR pszFileName, LPWSTR pszMUIPath, UINT cchMUIPath)
{
    HRESULT hr;
    UINT    cchWinPath;
    LANGID  lidUI;
    LANGID  lidInstall;
    LPWSTR  pszWrite;
    UINT    cchMaxWrite;
    BOOL    fPathChosen;

    RIP(IS_VALID_STRING_PTRW(pszFileName, -1));
    RIP(IS_VALID_WRITE_BUFFER(pszMUIPath, WCHAR, cchMUIPath));

    hr = E_FAIL;
    fPathChosen = FALSE;

    //
    // build the path to the windows\web folder...
    //

    cchWinPath = SHGetSystemWindowsDirectoryW(pszMUIPath, cchMUIPath);
    if (cchWinPath > 0 &&
        pszMUIPath[cchWinPath-1] == L'\\')
    {
        // don't have two L'\\' in a row
        cchWinPath--;
    }

    lidUI = GetNormalizedLangId(ML_CROSSCODEPAGE);
    lidInstall = GetInstallLanguage();

    pszWrite = pszMUIPath+cchWinPath;
    cchMaxWrite = cchMUIPath-cchWinPath;

    if (lidUI != lidInstall)
    {
        //
        // add L"\\Web\\mui\\xxxx\\<filename>"
        // where xxxx is the langid specific folder name
        //

        wnsprintfW(pszWrite, cchMaxWrite, c_wszMuiTemplate, lidUI, pszFileName);

        if (PathFileExistsW(pszMUIPath))
        {
            fPathChosen = TRUE;
        }
    }

    if (!fPathChosen)
    {
        //
        // add L"\\Web\\<filename>"
        //

        wnsprintfW(pszWrite, cchMaxWrite, c_wszWebTemplate, pszFileName);

        if (PathFileExistsW(pszMUIPath))
        {
            fPathChosen = TRUE;
        }
    }

    if (fPathChosen)
    {
        hr = S_OK;
    }

    return hr;
}

LWSTDAPI
SHGetWebFolderFilePathA(LPCSTR pszFileName, LPSTR pszMUIPath, UINT cchMUIPath)
{
    RIP(IS_VALID_STRING_PTRA(pszFileName, -1));
    RIP(IS_VALID_WRITE_BUFFER(pszMUIPath, CHAR, cchMUIPath));

    HRESULT     hr;
    CStrInW     strFN(pszFileName);
    CStrOutW    strMP(pszMUIPath, cchMUIPath);

    hr = SHGetWebFolderFilePathW(strFN, strMP, strMP.BufSize());

    return hr;
}

//  Given a string of form 5.00.2919.6300, this function gets the equivalent dword
//  representation of it.

#define NUM_VERSION_NUM 4
void ConvertVersionStrToDwords(LPSTR pszVer, LPDWORD pdwVer, LPDWORD pdwBuild)
{
    WORD rwVer[NUM_VERSION_NUM];

    for(int i = 0; i < NUM_VERSION_NUM; i++)
        rwVer[i] = 0;

    for(i = 0; i < NUM_VERSION_NUM && pszVer; i++)
    {
        rwVer[i] = (WORD) StrToInt(pszVer);
        pszVer = StrChr(pszVer, TEXT('.'));
        if (pszVer)
            pszVer++;
    }

   *pdwVer = (rwVer[0]<< 16) + rwVer[1];
   *pdwBuild = (rwVer[2] << 16) + rwVer[3];

}

/*
    For SP's we don't update the MUI package. So in order for the golden MUI package to work
    with SP's, we now check if the MUI package is compatible with range of version numbers.
    Since we have different modules calling into this, and they have different version numbers,
    we read the version range from registry for a particular module.

    This Function takes the lower and upper version number of the MUI package. It gets the caller's
    info and reads the version range from registry. If the MUI package version lies in the version
    range specified in the registry, it returns TRUE.
*/


BOOL IsMUICompatible(DWORD dwMUIFileVersionMS, DWORD dwMUIFileVersionLS)
{
    TCHAR szVersionInfo[MAX_PATH];
    DWORD dwType, dwSize;
    TCHAR szProcess[MAX_PATH];

    dwSize = sizeof(szVersionInfo);

    //Get the caller process
    if(!GetModuleFileName(NULL, szProcess, MAX_PATH))
        return FALSE;

    //Get the file name from the path
    LPTSTR lpszFileName = PathFindFileName(szProcess);

    //Query the registry for version info. If the key doesn't exists or there is an 
    //error, return false.
    if(ERROR_SUCCESS != SHRegGetUSValueA(c_szInternational, lpszFileName, 
                        &dwType, (LPVOID)szVersionInfo, &dwSize, TRUE, NULL, 0))
    {
        return FALSE;
    }

    LPTSTR lpszLowerBound = szVersionInfo;

    LPTSTR lpszUpperBound = StrChr(szVersionInfo, TEXT('-'));
    if(!lpszUpperBound || !*(lpszUpperBound+1))
        return FALSE;
    
    *(lpszUpperBound++) = NULL;

    DWORD dwLBMS, dwLBLS, dwUBMS, dwUBLS;

    ConvertVersionStrToDwords(lpszLowerBound, &dwLBMS, &dwLBLS);
    ConvertVersionStrToDwords(lpszUpperBound, &dwUBMS, &dwUBLS);

    //check if MUI version is in the specified range.
    if( (dwMUIFileVersionMS < dwLBMS) ||
        (dwMUIFileVersionMS == dwLBMS && dwMUIFileVersionLS < dwLBLS) ||
        (dwMUIFileVersionMS > dwUBMS) ||
        (dwMUIFileVersionMS == dwUBMS && dwMUIFileVersionLS > dwUBLS) )
    {
        return FALSE;
    }

    return TRUE;
}


BOOL CheckFileVersion(LPWSTR lpFile, LPWSTR lpFileMUI)
{
    DWORD dwSize, dwHandle, dwSizeMUI, dwHandleMUI;
    LPVOID lpVerInfo, lpVerInfoMUI;
    VS_FIXEDFILEINFO *pvsffi, *pvsffiMUI;
    BOOL fRet = FALSE;

    dwSize = GetFileVersionInfoSizeWrapW(lpFile, &dwHandle);
    dwSizeMUI = GetFileVersionInfoSizeWrapW(lpFileMUI, &dwHandleMUI);
    if (dwSize && dwSizeMUI)
    {
        if (lpVerInfo = LocalAlloc(LPTR, dwSize))
        {
            if (lpVerInfoMUI = LocalAlloc(LPTR, dwSizeMUI))
            {
                if (GetFileVersionInfoWrapW(lpFile, dwHandle, dwSize, lpVerInfo) &&
                    GetFileVersionInfoWrapW(lpFileMUI, dwHandleMUI, dwSizeMUI, lpVerInfoMUI))
                {
                    if (VerQueryValueWrapW(lpVerInfo, L"\\", (LPVOID *)&pvsffi, (PUINT)&dwSize) &&
                        VerQueryValueWrapW(lpVerInfoMUI, L"\\", (LPVOID *)&pvsffiMUI, (PUINT)&dwSizeMUI))
                    {
                        if ((pvsffi->dwFileVersionMS == pvsffiMUI->dwFileVersionMS &&
                            pvsffi->dwFileVersionLS == pvsffiMUI->dwFileVersionLS) ||
                            IsMUICompatible(pvsffiMUI->dwFileVersionMS, pvsffiMUI->dwFileVersionLS))

                        {
                            fRet = TRUE;
                        }
                    }
                }
                LocalFree(lpVerInfoMUI);
            }
            LocalFree(lpVerInfo);
        }
    }
    return fRet;
}

LWSTDAPI_(HINSTANCE) MLLoadLibraryW(LPCWSTR lpLibFileName, HMODULE hModule, DWORD dwCrossCodePage)
{
    LANGID lidUI;
    WCHAR szPath[MAX_PATH], szMUIPath[MAX_PATH];
    LPCWSTR lpPath = NULL;
    HINSTANCE hInst;
    static BOOL fCheckVersion = SHRegGetBoolUSValueA(c_szInternational, c_szCheckVersion, TRUE, TRUE);;

    if (!lpLibFileName)
        return NULL;

    szPath[0] = szMUIPath[0] = NULL;
    lidUI = GetNormalizedLangId(dwCrossCodePage);

    if (hModule)
    {
        if (GetModuleFileNameWrapW(hModule, szPath, ARRAYSIZE(szPath)))
        {
            PathRemoveFileSpecW(szPath);
            PathAppendW(szPath, lpLibFileName);

            if (GetInstallLanguage() == lidUI)
                lpPath = szPath;
        }
    }

    if (!lpPath)
    {
        GetMUIPathOfIEFileW(szMUIPath, ARRAYSIZE(szMUIPath), lpLibFileName, lidUI);
        lpPath = szMUIPath;
    }

    // Check version between module and resource. If different, use default one.
    if (fCheckVersion && szPath[0] && szMUIPath[0] && !CheckFileVersion(szPath, szMUIPath))
    {
        lidUI = GetInstallLanguage();
        lpPath = szPath;
    }

    ASSERT(lpPath);
    
    // BUGBUG: This should use PathFileExist first then load what exists
    //         failing in LoadLibrary is slow
    hInst = LoadLibraryWrapW(lpPath);

    if (NULL == hInst)
    {
#ifdef LATER_IE5
        HWND  hwnd = GetForegroundWindow();   
        HRESULT hr = InstallIEFeature(hwnd, MAKELCID(lidUI, SORT_DEFAULT), &CLSID_Satellite);
        if (hr == S_OK)
            hInst = LoadLibraryWrapW(lpPath);
#endif
        // All failed. Try to load default one lastly.
        if (!hInst && lpPath != szPath)
        {
            lidUI = GetInstallLanguage();
            hInst = LoadLibraryWrapW(szPath);
        }
    }

    if (NULL == hInst)
        hInst = LoadLibraryWrapW(lpLibFileName);

    // if we load any resource, save info into dpa table
    MLSetMLHInstance(hInst, lidUI);

    return hInst;
}

//
//  Wide-char wrapper for MLLoadLibraryA
//
LWSTDAPI_(HINSTANCE) MLLoadLibraryA(LPCSTR lpLibFileName, HMODULE hModule, DWORD dwCrossCodePage)
{
    WCHAR szLibFileName[MAX_PATH];

    SHAnsiToUnicode(lpLibFileName, szLibFileName, ARRAYSIZE(szLibFileName));

    return MLLoadLibraryW(szLibFileName, hModule, dwCrossCodePage);
}

LWSTDAPI_(BOOL) MLFreeLibrary(HMODULE hModule)
{
    MLClearMLHInstance(hModule);
    return FreeLibrary(hModule);
}

LWSTDAPI_(BOOL) MLIsMLHInstance(HINSTANCE hInst)
{
    int i;

    ENTERCRITICAL;
    i = GetPUIITEM(hInst);
    LEAVECRITICAL;

    return (0 <= i);
}


//
//  PlugUI DialogBox APIs
//

#include <pshpack2.h>
// DLGTEMPLATEEX is defined in shell\inc\ccstock.h
// What a DIALOGEX item header would look like
typedef struct
{
    DWORD helpID;
    DWORD dwExtendedStyle;
    DWORD style;
    WORD x;
    WORD y;
    WORD cx;
    WORD cy;
    DWORD id;
} DLGITEMTEMPLATEEX, *LPDLGITEMTEMPLATEEX;

#include <poppack.h> /* Resume normal packing */

// Skips string (or ID) and returns the next aligned WORD.
PBYTE SkipIDorString(LPBYTE pb)
{
    LPWORD pw = (LPWORD)pb;

    if (*pw == 0xFFFF)
        return (LPBYTE)(pw + 2);

    while (*pw++ != 0)
        ;

    return (LPBYTE)pw;
}

PBYTE SkipDialogHeader(LPCDLGTEMPLATE pdt)
{
    LPBYTE pb;
    BOOL fEx;
    DLGTEMPLATEEX *pdtex;

    if (HIWORD(pdt->style) == 0xFFFF)
    {
        pdtex = (DLGTEMPLATEEX *)pdt;
        fEx = TRUE;
        pb = (LPBYTE)(((LPDLGTEMPLATEEX)pdt) + 1);
    }
    else
    {
        fEx = FALSE;
        pb = (LPBYTE)(pdt + 1);
    }

    // If there is a menu ordinal, add 4 bytes skip it. Otherwise it is a string or just a 0.
    pb = SkipIDorString(pb);

    // Skip window class and window text, adjust to next word boundary.
    pb = SkipIDorString(pb);    // class
    pb = SkipIDorString(pb);    // window text

    // Skip font type, size and name, adjust to next dword boundary.
    if ((fEx ? pdtex->dwStyle : pdt->style) & DS_SETFONT)
    {
        pb += fEx ? sizeof(DWORD) + sizeof(WORD): sizeof(WORD);
        pb = SkipIDorString(pb);
    }
    pb = (LPBYTE)(((ULONG_PTR)pb + 3) & ~3);    // DWORD align

    return pb;
}

// This gets called by thank produced stubs. It returns the size of a dialog template.
DWORD GetSizeOfDialogTemplate(LPCDLGTEMPLATE pdt)
{
    LPBYTE pb;
    BOOL fEx;
    UINT cItems; 
    DLGTEMPLATEEX *pdtex;

    if (HIWORD(pdt->style) == 0xFFFF)
    {
        pdtex = (DLGTEMPLATEEX *)pdt;
        fEx = TRUE;
        cItems = pdtex->cDlgItems;
    }
    else
    {
        fEx = FALSE;
        cItems = pdt->cdit;
    }

    // skip DLGTEMPLATE(EX) part
    pb = SkipDialogHeader(pdt);

    while (cItems--)
    {
        UINT cbCreateParams;

        pb += fEx ? sizeof(DLGITEMTEMPLATEEX) : sizeof(DLGITEMTEMPLATE);

        // Skip the dialog control class name.
        pb = SkipIDorString(pb);

        // Look at window text now.
        pb = SkipIDorString(pb);

        cbCreateParams = *((LPWORD)pb);

        // skip any CreateParams which include the generated size WORD.
        if (cbCreateParams)
            pb += cbCreateParams;

        pb += sizeof(WORD);

        // Point at the next dialog item. (DWORD aligned)
        pb = (LPBYTE)(((ULONG_PTR)pb + 3) & ~3);
    }
    // Return template size.
    return (DWORD)(pb - (LPBYTE)pdt);
}

void CopyIDorString(LPBYTE *ppbDest, LPBYTE *ppbSrc)
{
    if (*(LPWORD)(*ppbSrc) == 0xFFFF)
    {
        memcpy(*ppbDest, *ppbSrc, 4);
        *ppbSrc += 4;
        *ppbDest += 4;
        return;
    }

    while (*(LPWORD)(*ppbSrc) != 0)
    {
        memcpy(*ppbDest, *ppbSrc, 2);
        *ppbSrc += 2;
        *ppbDest += 2;
    }

    // copy 0
    memcpy(*ppbDest, *ppbSrc, 2);
    *ppbSrc += 2;
    *ppbDest += 2;

    return;
}

void StripIDorString(LPBYTE *ppbDest, LPBYTE *ppbSrc)
{
    *ppbSrc = SkipIDorString(*ppbSrc);
    *(LPWORD)(*ppbDest) = 0;
    *ppbDest += 2;

    return;
}

BOOL CheckID(LPBYTE pb, UINT c, BOOL fEx, int id, int max)
{
    int n = 0;

    if (0 > id)
        return FALSE;

    while (c--)
    {
        UINT cbCreateParams;
        LPDLGITEMTEMPLATE lpdit;
        LPDLGITEMTEMPLATEEX lpditex;
        int i;

        if (fEx)
        {
            lpditex = (LPDLGITEMTEMPLATEEX)pb;
            i = lpditex->id;
            pb += sizeof(DLGITEMTEMPLATEEX);
        }
        else
        {
            lpdit = (LPDLGITEMTEMPLATE)pb;
            i = lpdit->id;
            pb += sizeof(DLGITEMTEMPLATE);
        }

        if (i == id)
        {
            n++;
            if (n > max)
                return FALSE;
        }

        // Skip the dialog control class name.
        pb = SkipIDorString(pb);

        // Look at window text now.
        pb = SkipIDorString(pb);

        cbCreateParams = *((LPWORD)pb);

        // skip any CreateParams which include the generated size WORD.
        if (cbCreateParams)
            pb += cbCreateParams;

        pb += sizeof(WORD);

        // Point at the next dialog item. (DWORD aligned)
        pb = (LPBYTE)(((ULONG_PTR)pb + 3) & ~3);
    }

    return TRUE;
}

int GetUniqueID(LPBYTE pbSrc, UINT cSrc, LPBYTE pbDest, UINT cDest, BOOL fEx)
{
    // BUGBUG: Need to be improved !!!
    while (1)
    {
        int id;

        for (id = 100; id < 0x7FFF; id++)
        {
            if (CheckID(pbSrc, cSrc, fEx, id, 0))
                if (CheckID(pbDest, cDest, fEx, id, 0))
                    return id;
        }
}   }


#define MAX_REG_WINMSG      0xFFFF      // max value returned from RegisterWindowMessageA()
#define MIN_REG_WINMSG      0xC000      // min value returned from RegisterWindowMessageA()

LRESULT StaticSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
LRESULT ButtonSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
LRESULT ListBoxSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
LRESULT ComboBoxSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

#define ML_SSC_MASK     0x0000001F      // Static control style bit check mask
#define ML_BSC_MASK     0x0000000F      // Button control style bit check mask
    
BOOL StaticStyleCheck(DWORD dwStyle);
BOOL ButtonStyleCheck(DWORD dwStyle);
BOOL ListBoxStyleCheck(DWORD dwStyle);
BOOL ComboBoxStyleCheck(DWORD dwStyle);


typedef BOOL (* STYLECHECKPROC)(DWORD);

typedef struct tagControl
{
    WORD wControl;
    LPCSTR szControl;
    SUBCLASSPROC SubclassProc;
    STYLECHECKPROC StyleCheckProc;
} CONTROL, *LPCONTROL;

const CONTROL c_CtrlTbl[] =
{
    { 0x0082L, "STATIC", StaticSubclassProc, StaticStyleCheck },
    { 0x0080L, "BUTTON", ButtonSubclassProc, ButtonStyleCheck },
    { 0x0083L, "LISTBOX", ListBoxSubclassProc, ListBoxStyleCheck },
    { 0x0085L, "COMBOBOX", ComboBoxSubclassProc, ComboBoxStyleCheck },
};

// registered window messages global variables, initial value is 0.
UINT g_ML_GETTEXT               = 0,
     g_ML_GETTEXTLENGTH         = 0,
     g_ML_SETTEXT               = 0;

UINT g_ML_LB_ADDSTRING          = 0,
     g_ML_LB_FINDSTRING         = 0,
     g_ML_LB_FINDSTRINGEXACT    = 0,
     g_ML_LB_GETTEXT            = 0,
     g_ML_LB_GETTEXTLEN         = 0,
     g_ML_LB_INSERTSTRING       = 0,
     g_ML_LB_SELECTSTRING       = 0;

UINT g_ML_CB_ADDSTRING          = 0,
     g_ML_CB_FINDSTRING         = 0,
     g_ML_CB_FINDSTRINGEXACT    = 0,
     g_ML_CB_GETLBTEXT          = 0,
     g_ML_CB_GETLBTEXTLEN       = 0,
     g_ML_CB_INSERTSTRING       = 0,
     g_ML_CB_SELECTSTRING       = 0;


int DoMungeControl(LPBYTE pb, DWORD dwStyle)
{
    int i = ARRAYSIZE(c_CtrlTbl);

    ASSERT(!g_bRunningOnNT5OrHigher);

    if (*(LPWORD)pb == 0xFFFF)
    {
        for (i = 0; i < ARRAYSIZE(c_CtrlTbl); i++)
        {
            if (*(LPWSTR)(pb + 2) == c_CtrlTbl[i].wControl)
                return (c_CtrlTbl[i].StyleCheckProc(dwStyle)) ? i : ARRAYSIZE(c_CtrlTbl);
        }
    }
    return i;
}

typedef struct tagCItem
{
    DWORD   dwStyle;
    RECT    rc;
    LPWSTR  lpwz;
    int     nBuf;
} CITEM, *LPCITEM;

void PutSpaceString(HWND hwnd, LPCITEM lpCItem)
{
    HDC hdc;
    HFONT hfont;
    int nSpace;
    SIZE sizSpace, sizString;

    hdc = GetDC(hwnd);
    hfont = (HFONT)SelectObject(hdc, GetWindowFont(hwnd));
    GetTextExtentPointFLW(hdc, L" ", 1, &sizSpace);
    GetTextExtentPointFLW(hdc, lpCItem->lpwz, lstrlenW(lpCItem->lpwz), &sizString);
    nSpace = (sizString.cx + sizSpace.cx - 1) / sizSpace.cx;

    // Build space string and pass it to original WndProc
    if (IsWindowUnicode(hwnd))
    {
        LPWSTR lpSpaceW;

        lpSpaceW = (LPWSTR)LocalAlloc(LPTR, (nSpace + 1) * sizeof(WCHAR));
        if (lpSpaceW)
        {
            int i;

            for (i = 0; i < nSpace; i++)
                lpSpaceW[i] = L' ';
            lpSpaceW[nSpace] = 0;
            DefSubclassProc(hwnd, WM_SETTEXT, 0, (LPARAM)lpSpaceW);
            LocalFree(lpSpaceW);
        }
    }
    else
    {
        LPSTR lpSpace;

        lpSpace = (LPSTR)LocalAlloc(LPTR, (nSpace + 1) * sizeof(CHAR));
        if (lpSpace)
        {
            int i;

            for (i = 0; i < nSpace; i++)
                lpSpace[i] = ' ';
            lpSpace[nSpace] = 0;
            DefSubclassProc(hwnd, WM_SETTEXT, 0, (LPARAM)lpSpace);
            LocalFree(lpSpace);
        }
    }
    if (hfont)
        SelectObject(hdc, hfont);
    ReleaseDC(hwnd, hdc);
}

LRESULT ControlSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    LPCITEM lpCItem = (LPCITEM)dwRefData;

    ASSERT(g_ML_GETTEXT);
    ASSERT(g_ML_SETTEXT);

    switch (uMsg)
    {
        case WM_GETTEXT:
        {
            if (lpCItem->lpwz && lParam)
            {
                if (IsWindowUnicode(hwnd))
                {
                    StrCpyNW((LPWSTR)lParam, lpCItem->lpwz, (int)wParam);
                    return min((int)wParam, lstrlenW(lpCItem->lpwz));
                }
                else
                {
                    return WideCharToMultiByte(CP_ACP, 0, lpCItem->lpwz, -1, (LPSTR)lParam, (int)wParam, NULL, NULL);
                }
            }

            return 0;
        }

        case WM_GETTEXTLENGTH:
        {
            if (IsWindowUnicode(hwnd))
            {
                if (lpCItem->lpwz)
                    return lstrlenW(lpCItem->lpwz);
                else
                    return 0;
            }
            else
            {
                return WideCharToMultiByte(CP_ACP, 0, lpCItem->lpwz, -1, NULL, 0, NULL, NULL);
            }
        }

        case WM_SETTEXT:
        {
            if (lParam)
            {
                if (IsWindowUnicode(hwnd))
                    return SendMessage(hwnd, g_ML_SETTEXT, 0, lParam);
                else
                {
                    int nStr = lstrlenA((LPSTR)lParam);

                    LPWSTR lpTmp = (LPWSTR)LocalAlloc(LPTR, (nStr + 1) * sizeof(WCHAR));

                    if (lpTmp)
                    {
                        MultiByteToWideChar(CP_ACP, 0, (LPSTR)lParam, -1, lpTmp, nStr + 1);
                        SendMessage(hwnd, g_ML_SETTEXT, 0, (LPARAM)lpTmp);
                        LocalFree(lpTmp);
                        return TRUE;
                    }
                }
            }
            return FALSE;
        }

        default:
        {
            // ML_GETTEXT:
            if (uMsg == g_ML_GETTEXT)
            {
                if (lpCItem->lpwz && lParam)
                {
                    StrCpyNW((LPWSTR)lParam, lpCItem->lpwz, (int)wParam);
                    return min((int)wParam, lstrlenW(lpCItem->lpwz));
                }

                return 0;
            }

            // ML_GETTEXTLENGTH:
            if (uMsg == g_ML_GETTEXTLENGTH)
            {
                if (lpCItem->lpwz)
                    return lstrlenW(lpCItem->lpwz);
                else
                    return 0;
            }

            // ML_SETTEXT:
            if (uMsg == g_ML_SETTEXT)
            {
                if (lParam)
                {
                    int nStr = lstrlenW((LPWSTR)lParam);

                    if (nStr >= lpCItem->nBuf)
                    {
                        LPWSTR lpTmp = (LPWSTR)LocalAlloc(LPTR, (nStr + 1) * sizeof(WCHAR));

                        if (lpTmp)
                        {
                            if (lpCItem->lpwz)
                                LocalFree(lpCItem->lpwz);
                            lpCItem->lpwz = lpTmp;
                            lpCItem->nBuf = nStr + 1;
                        }
                    }
                    if (lpCItem->lpwz)
                    {
                        InvalidateRect(hwnd, NULL, TRUE);
                        StrCpyNW(lpCItem->lpwz, (LPWSTR)lParam, lpCItem->nBuf);
                        PutSpaceString(hwnd, lpCItem);
                        return TRUE;
                    }
                }

                return FALSE;
            }

        }

    }

    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void DrawControlString(HDC hdc, LPRECT lprc, LPCITEM lpCItem, UINT uFormat, BOOL fDisabled)
{
    int iTxt, iBk;

    iBk = SetBkMode(hdc, TRANSPARENT);
    if (fDisabled)
    {
        RECT rcOffset = *lprc;

        iTxt = SetTextColor(hdc, GetSysColor(COLOR_BTNHIGHLIGHT));
        OffsetRect(&rcOffset, 1, 1);
        DrawTextFLW(hdc, lpCItem->lpwz, lstrlenW(lpCItem->lpwz), &rcOffset, uFormat);
        SetTextColor(hdc, GetSysColor(COLOR_BTNSHADOW));
    }
    else
    {
        iTxt = SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
    }
    DrawTextFLW(hdc, lpCItem->lpwz, lstrlenW(lpCItem->lpwz), lprc, uFormat);
    SetTextColor(hdc, iTxt);
    SetBkMode(hdc, iBk);
}

void AdjustTextPosition(HDC hdc, BYTE bStyle, BYTE bState, UINT uFormat, LPRECT lprc)
{
    static int s_cxEdge = GetSystemMetrics(SM_CXEDGE);
    static int s_cxFrame = GetSystemMetrics(SM_CXFRAME);
    static int s_cxBorder = GetSystemMetrics(SM_CXBORDER);
    static int s_cyBorder = GetSystemMetrics(SM_CYBORDER);
    static int s_cxMenuCheck = GetSystemMetrics(SM_CXMENUCHECK);

    switch (bStyle)
    {
        case BS_PUSHBUTTON:
        case BS_DEFPUSHBUTTON:
            if (!(bState & BST_PUSHED))
            {
                lprc->top -= s_cyBorder;
                lprc->bottom -= s_cyBorder;
            }
            break;

        case BS_CHECKBOX:
        case BS_AUTOCHECKBOX:
        case BS_RADIOBUTTON:
        case BS_AUTORADIOBUTTON:
        case BS_3STATE:
        case BS_AUTO3STATE:
            lprc->left += s_cxMenuCheck + s_cxFrame + s_cxBorder + s_cxEdge;
            break;

        case BS_GROUPBOX:
            lprc->left += s_cxMenuCheck / 2 + s_cxBorder + s_cxEdge;
            break;
    }

    if (uFormat & DT_VCENTER)
    {
        SIZE size;
        LONG cyGap;

        GetTextExtentPointFLW(hdc, L" ", 1, &size);
        cyGap = (lprc->bottom - lprc->top - size.cy + 1) / 2;
        lprc->top += cyGap;
        lprc->bottom += cyGap;
    }
    else
    {
        lprc->top += s_cyBorder;
        lprc->bottom += s_cyBorder;
    }
}

BOOL ButtonStyleCheck(DWORD dwStyle)
{
    switch (dwStyle & ML_BSC_MASK)
    {
        case BS_PUSHBUTTON:
        case BS_DEFPUSHBUTTON:
        case BS_CHECKBOX:
        case BS_AUTOCHECKBOX:
        case BS_RADIOBUTTON:
        case BS_AUTORADIOBUTTON:
        case BS_3STATE:
        case BS_AUTO3STATE:
        case BS_GROUPBOX:
        case BS_OWNERDRAW:
        {
            static BOOL fRegMsgOK = FALSE;

            if (FALSE == fRegMsgOK)
            {
                if (g_ML_GETTEXT && g_ML_SETTEXT)
                {
                    // messages been registered
                    fRegMsgOK = TRUE;
                }
                else
                {
                    // register window messages
                    fRegMsgOK = (BOOL)(g_ML_GETTEXT = RegisterWindowMessageA("ML_GETTEXT")) &&
                                (BOOL)(g_ML_SETTEXT = RegisterWindowMessageA("ML_SETTEXT"));
                }
            }

            return fRegMsgOK;
        }
    }

    return FALSE;
}

LRESULT ButtonSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    LPCITEM lpCItem = (LPCITEM)dwRefData;
    BYTE bStyle = (BYTE)(lpCItem->dwStyle & ML_BSC_MASK);

    switch (uMsg)
    {
        case WM_DESTROY:
        {
            if (hwnd)
            {
                // Must remove by atom - Win95 compat
                RemoveProp(hwnd, MAKEINTATOM(g_atmML));
                RemoveWindowSubclass(hwnd, ButtonSubclassProc,  0);
            }
            if (lpCItem->lpwz)
            {
                LocalFree(lpCItem->lpwz);
                lpCItem->lpwz = NULL;
                lpCItem->nBuf = 0;
            }
            LocalFree(lpCItem);
            break;
        }

        case WM_GETTEXT:
        case WM_SETTEXT:
            return ControlSubclassProc(hwnd, uMsg, wParam, lParam, uIdSubclass, dwRefData);

        case BM_SETSTATE:
        {
            if (bStyle != BS_PUSHBUTTON && bStyle != BS_DEFPUSHBUTTON)
                break;

            // fall thru ...
        }

        case WM_ENABLE:
        case WM_PAINT:
        {
            UINT uFormat;
            BOOL fDoPaint = TRUE;

            switch (bStyle)
            {
                case BS_PUSHBUTTON:
                case BS_DEFPUSHBUTTON:
                    uFormat = DT_CENTER | DT_VCENTER;
                    break;

                case BS_CHECKBOX:
                case BS_AUTOCHECKBOX:
                case BS_RADIOBUTTON:
                case BS_AUTORADIOBUTTON:
                case BS_3STATE:
                case BS_AUTO3STATE:
                case BS_GROUPBOX:
                    uFormat = DT_LEFT | DT_WORDBREAK;
                    break;

                default:
                    fDoPaint = FALSE;
            }

            if (fDoPaint && lpCItem->lpwz)
            {
                HDC hdc;
                HFONT hfont;
                RECT rc;
                BYTE bState;

                DefSubclassProc(hwnd, uMsg, wParam, lParam);

                bState = (BYTE)SendMessage(hwnd, BM_GETSTATE, 0, 0L);

                hdc = GetDC(hwnd);
                hfont = (HFONT)SelectObject(hdc, GetWindowFont(hwnd));
                CopyRect(&rc, &lpCItem->rc);
                AdjustTextPosition(hdc, bStyle, bState, uFormat, &rc);
                DrawControlString(hdc, &rc, lpCItem, uFormat, GetWindowLong(hwnd, GWL_STYLE) & WS_DISABLED);
                if (hfont)
                    SelectObject(hdc, hfont);
                ReleaseDC(hwnd, hdc);
                return FALSE;
            }
            break;
        }

        default:
        {
            // non-registered window message
            if ((uMsg < MIN_REG_WINMSG) || (uMsg > MAX_REG_WINMSG))
                break;   // fall through default process

            // ML_GETTEXT:
            // ML_SETTEXT:
            if ((uMsg == g_ML_GETTEXT) || (uMsg == g_ML_SETTEXT))
                return ControlSubclassProc(hwnd, uMsg, wParam, lParam, uIdSubclass, dwRefData);

        }   // switch-default

    }   // switch

    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}


BOOL StaticStyleCheck(DWORD dwStyle)
{
    switch (dwStyle & ML_SSC_MASK)
    {
        case SS_LEFT:
        case SS_CENTER:
        case SS_RIGHT:
        case SS_SIMPLE:
        case SS_LEFTNOWORDWRAP:
        case SS_OWNERDRAW:
        {
            static BOOL fRegMsgOK = FALSE;

            if (FALSE == fRegMsgOK)
            {
                if (g_ML_GETTEXT && g_ML_SETTEXT)
                {
                    // messages been registered
                    fRegMsgOK = TRUE;
                }
                else
                {
                    // register window messages
                    fRegMsgOK = (BOOL)(g_ML_GETTEXT = RegisterWindowMessageA("ML_GETTEXT")) &&
                                (BOOL)(g_ML_SETTEXT = RegisterWindowMessageA("ML_SETTEXT"));
                }
            }

            return fRegMsgOK;
        }
    }

    return FALSE;
}

LRESULT StaticSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    LPCITEM lpCItem = (LPCITEM)dwRefData;

    switch (uMsg)
    {
        case WM_DESTROY:
        {
            if (hwnd)
            {
                // Must remove by atom - Win95 compat
                RemoveProp(hwnd, MAKEINTATOM(g_atmML));
                RemoveWindowSubclass(hwnd, StaticSubclassProc,  0);
            }
            if (lpCItem->lpwz)
            {
                LocalFree(lpCItem->lpwz);
                lpCItem->lpwz = NULL;
                lpCItem->nBuf = 0;
            }
            LocalFree(lpCItem);
            break;
        }

        case WM_GETTEXT:
        case WM_SETTEXT:
            return ControlSubclassProc(hwnd, uMsg, wParam, lParam, uIdSubclass, dwRefData);

        case WM_PAINT:
        {
            UINT uFormat;
            BOOL fDoPaint = TRUE;

            switch (lpCItem->dwStyle & ML_SSC_MASK)
            {
                case SS_LEFT:
                    uFormat = DT_LEFT | DT_WORDBREAK;
                    break;

                case SS_CENTER:
                    uFormat = DT_CENTER | DT_WORDBREAK;
                    break;

                case SS_RIGHT:
                    uFormat = DT_RIGHT | DT_WORDBREAK;
                    break;

                case SS_SIMPLE:
                case SS_LEFTNOWORDWRAP:
                    uFormat = DT_LEFT;
                    break;

                default:
                    fDoPaint = FALSE;
            }

            if (fDoPaint && lpCItem->lpwz)
            {
                HDC hdc;
                PAINTSTRUCT ps;
                HFONT hfont;

                hdc = BeginPaint(hwnd, &ps);
                hfont = (HFONT)SelectObject(hdc, GetWindowFont(hwnd));
                DrawControlString(hdc, &lpCItem->rc, lpCItem, uFormat, GetWindowLong(hwnd, GWL_STYLE) & WS_DISABLED);
                if (hfont)
                    SelectObject(hdc, hfont);
                EndPaint(hwnd, &ps);
            }
            break;
        }

        default:
        {
            // non-registered window message
            if ((uMsg < MIN_REG_WINMSG) || (uMsg > MAX_REG_WINMSG))
                break;   // fall through default process

            // ML_GETTEXT:
            // ML_SETTEXT:
            if ((uMsg == g_ML_GETTEXT) || (uMsg == g_ML_SETTEXT))
                return ControlSubclassProc(hwnd, uMsg, wParam, lParam, uIdSubclass, dwRefData);

        }   // switch-default

    }   // switch

    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}


//--------------------------------------------------------------------------
// Subclass of ListBox, ComboBox controls
//--------------------------------------------------------------------------
//
// To enable the subclassed control with capability to process unicode
// string as well as font linking.
//
// Implementation:
//      these subclassing procedures will allocate memory to store the
//      unicode string and then convert unicode string pointer into
//      "pointer string," like:
//          0x004b5c30 --> "4b5c30"
//      and then store this pointer string as regular string (unicode or
//      ANSI, depends on the window system) into the control.
//
//--------------------------------------------------------------------------
#define MAX_BUFFER_BYTE         1024            // bytes
#define MAX_WINCLASS_NAME       32              // bytes

#define LB_WINCLASS_NAME        "ListBox"       // window class name of ListBox control
#define CB_WINCLASS_NAME        "ComboBox"      // window class name of ComboBox control
#define CB_SUBWINCLASS_LB_NAME  "ComboLBox"     // window class name of ListBox control inside ComboBox
#define CB_SUBWINCLASS_ED_NAME  "Edit"          // window class name of Edit control

#define ML_CB_SUBWIN            TEXT("Shlwapi.ML.CB.SubWinHnd")
#define ML_LB_NTBUG             TEXT("Shlwapi.ML.LB.NTBug")

#define LB_SUB_PROC             ListBoxSubclassProc

#define VK_A                    0x41        // virtual keyboard value, 'A', cAsE iNsEnSiTiVe
#define VK_Z                    0x5A        // virtual keyboard value, 'Z', cAsE iNsEnSiTiVe
#define VK_0                    0x30        // virtual keyboard value, '0'
#define VK_9                    0x39        // virtual keyboard value, '9'

typedef struct
{
    HWND hwndLBSubWin;
} MLCBSUBWINHND, *LPMLCBSUBWINHND;


//--- LB & CB common routines ---
LRESULT MLLBCBDefSubClassProcWrap(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!IsWindowUnicode(hWnd))
    {
        if ((uMsg == LB_GETTEXT) || (uMsg == CB_GETLBTEXT))
        {
            // buffer size 256 is big enough for this
            CStrOut cStr((LPWSTR)lParam, 256);

            return DefSubclassProc(hWnd, uMsg, wParam, (LPARAM)((LPSTR)cStr));
        }

        if ((uMsg == LB_INSERTSTRING) || (uMsg == CB_INSERTSTRING))
        {
            CStrIn cStr((LPWSTR)lParam);

            return DefSubclassProc(hWnd, uMsg, wParam, (LPARAM)((LPSTR)cStr));
        }
    }
    
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}


BOOL MLLBCBGetWStrPtr(HWND hWnd, INT_PTR iIndex, LPWSTR* lppwszStr)
{
    WCHAR szWPtr[sizeof(LPVOID) * 2 + 2 + 1] = {0};
    CHAR szWinClass[MAX_WINCLASS_NAME];
    BOOL fContinue = FALSE;

    // initial the destination pointer to NULL...
    (*lppwszStr) = NULL;

    GetClassNameA(hWnd, szWinClass, ARRAYSIZE(szWinClass));
    if (((0 == lstrcmpiA(szWinClass, LB_WINCLASS_NAME)) || (0 == lstrcmpiA(szWinClass, CB_SUBWINCLASS_LB_NAME))) &&
        (LB_ERR != MLLBCBDefSubClassProcWrap(hWnd, LB_GETTEXT, (WPARAM)iIndex, (LPARAM)(szWPtr + 2))))
            fContinue = TRUE;

    if ((!fContinue) && (0 == lstrcmpiA(szWinClass, CB_WINCLASS_NAME)) &&
        (CB_ERR != MLLBCBDefSubClassProcWrap(hWnd, CB_GETLBTEXT, (WPARAM)iIndex, (LPARAM)(szWPtr + 2))))
            fContinue = TRUE;

    if (!fContinue)
        return FALSE;   // Invalid WinClass name or fail to get the item.

    if (lstrlenW(szWPtr + 2))
    {
        szWPtr[0] = L'0';
        szWPtr[1] = L'x';
        StrToIntExW(szWPtr, STIF_SUPPORT_HEX, (LPINT)lppwszStr);
        ASSERT(*lppwszStr);   // to warn some conversion failure case (not ALL).
    }
    else
        return FALSE;   // for caution: impossible case of zero length

    return TRUE;
}


BOOL MLLBCBDoDeleteItem(HWND hWnd, INT_PTR iIndex)
{
    LPWSTR lpwszStr;

    // get the string pointer...
    if (!MLLBCBGetWStrPtr(hWnd, iIndex, &lpwszStr))
        return FALSE;

    // free the unicode string allocated by subclassing routine.
    // the "pointer string" stored inside LB/CB will be deleted/freed
    // in the subsequence default process call in the calling routine.
    if (lpwszStr)
        LocalFree(lpwszStr);
    else
        return FALSE;   // for caution: NULL pointer

    return TRUE;
}


INT_PTR MLLBCBAddInsertString(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam, INT_PTR iCnt, DWORD dwStyle, BOOL fIsLB)
{
    INT_PTR iIndex;
    LPWSTR lpwszInStr = NULL;
    WCHAR szWPtr[sizeof(LPVOID) * 2 + 1] = {0};

    ASSERT(uiMsg);

    if (!lParam)
        return (fIsLB ? LB_ERR : CB_ERR);

    // initial wide string pointer holder and copy input string...
    lpwszInStr = (LPWSTR)LocalAlloc(LPTR, ((lstrlenW((LPCWSTR)lParam) + 1) * sizeof(WCHAR)));
    if (!lpwszInStr)
        return (fIsLB ? LB_ERRSPACE : CB_ERRSPACE);
    else
        StrCpyW(lpwszInStr, (LPCWSTR)lParam);

    // convert unicode string pointer into "pointer string"...
    wnsprintfW(szWPtr, ARRAYSIZE(szWPtr), L"%lx", lpwszInStr);

    if (uiMsg == (fIsLB ? g_ML_LB_INSERTSTRING : g_ML_CB_INSERTSTRING))
        iIndex = (INT_PTR)wParam;
    else
    {
        // find the proper place to add
        if ((iCnt == 0) || (!(dwStyle & (fIsLB ? LBS_SORT : CBS_SORT))))
            iIndex = iCnt;
        else
        {
            int iLP;

            for (iLP = 0; iLP < iCnt; iLP++)
            {
                LPWSTR lpwszStrTmp;

                // get the string pointer...
                if (!MLLBCBGetWStrPtr(hWnd, iLP, &lpwszStrTmp))
                    return (fIsLB ? LB_ERR : CB_ERR);

                if (lpwszStrTmp)
                {
                    if (StrCmpW(lpwszInStr, lpwszStrTmp) <= 0)
                        break;
                }
                else
                    return (fIsLB ? LB_ERR : CB_ERR);
            }

            iIndex = iLP;
        }
    }

    // do adding...
    return (INT_PTR)MLLBCBDefSubClassProcWrap(hWnd, (fIsLB ? LB_INSERTSTRING : CB_INSERTSTRING),
                                            (WPARAM)iIndex, (LPARAM)szWPtr);
}


int MLLBCBGetLBTextNLength(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam, INT_PTR iCnt, BOOL fIsLB)
{
    LPWSTR lpwszStr;
    INT_PTR iIndex = (INT_PTR)wParam;

    ASSERT(uiMsg);

    // get the string pointer...
    if ((iIndex >= iCnt) || (!MLLBCBGetWStrPtr(hWnd, iIndex, &lpwszStr)))
        return (fIsLB ? LB_ERR : CB_ERR);

    if (lpwszStr)
    {
        if (uiMsg == (fIsLB ? g_ML_LB_GETTEXT : g_ML_CB_GETLBTEXT))
        {
            if (lParam)
                StrCpyW((LPWSTR)lParam, lpwszStr);
            else
                return (fIsLB ? LB_ERR : CB_ERR);   // for caution: return string buffer is NULL
        }

        return lstrlenW(lpwszStr);
    }
    else
        return (fIsLB ? LB_ERR : CB_ERR);   // for caution: NULL pointer
}


INT_PTR MLLBCBFindStringNExact(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam, INT_PTR iCnt, BOOL fIsLB)
{
    INT_PTR iStartIdx = (INT_PTR)wParam;

    ASSERT(uiMsg);

    if (iStartIdx >= iCnt)
        return (fIsLB ? LB_ERR : CB_ERR);

    if (iStartIdx == -1)
        iStartIdx = 0;

    if (lParam)
    {
        for (int iLP = 0; iLP < iCnt; iLP++)
        {
            LPWSTR lpwszStr;
            INT_PTR iIndex = (iStartIdx + iLP) % iCnt;

            // get the string pointer...
            if (!MLLBCBGetWStrPtr(hWnd, iIndex, &lpwszStr))
                return (fIsLB ? LB_ERR : CB_ERR);

            if (lpwszStr)
            {
                if (uiMsg == (fIsLB ? g_ML_LB_FINDSTRING : g_ML_CB_FINDSTRING))
                {
                    if (lpwszStr == StrStrIW(lpwszStr, (LPCWSTR)lParam))
                        return iIndex;
                }
                else
                {
                    if (0 == StrCmpIW(lpwszStr, (LPCWSTR)lParam))
                        return iIndex;
                }
            }
        }

        return (fIsLB ? LB_ERR : CB_ERR);   // can not find the subject
    }
    else
        return (fIsLB ? LB_ERR : CB_ERR);   // for caution
}
//--- LB & CB common routines ---

//--- ListBox -------------------------------------------------
BOOL ListBoxStyleCheck(DWORD dwStyle)
{
    // HasStrings   0   0   1   1
    // OwnerDraw    0   1   0   1
    // SubClassing  Y   N   Y   Y
    //              ^
    //              LBS_HASSTRINGS is a default setting if not OwnerDraw.
    if ((!(dwStyle & LBS_HASSTRINGS)) &&
        (dwStyle & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)))
        return FALSE;

    static BOOL fRegMsgOK = FALSE;

    if (FALSE == fRegMsgOK)
    {
        // register window message(s)
        fRegMsgOK = (BOOL)(g_ML_LB_ADDSTRING        = RegisterWindowMessageA("ML_LB_ADDSTRING"))       &&
                    (BOOL)(g_ML_LB_FINDSTRING       = RegisterWindowMessageA("ML_LB_FINDSTRING"))      &&
                    (BOOL)(g_ML_LB_FINDSTRINGEXACT  = RegisterWindowMessageA("ML_LB_FINDSTRINGEXACT")) &&
                    (BOOL)(g_ML_LB_GETTEXT          = RegisterWindowMessageA("ML_LB_GETTEXT"))         &&
                    (BOOL)(g_ML_LB_GETTEXTLEN       = RegisterWindowMessageA("ML_LB_GETTEXTLEN"))      &&
                    (BOOL)(g_ML_LB_INSERTSTRING     = RegisterWindowMessageA("ML_LB_INSERTSTRING"))    &&
                    (BOOL)(g_ML_LB_SELECTSTRING     = RegisterWindowMessageA("ML_LB_SELECTSTRING"));
    }

    return fRegMsgOK;
}


BOOL MLIsLBFromCB(HWND hWnd)
{
    // decide if this is sub win, LB inside CB.
    CHAR szWinClass[MAX_WINCLASS_NAME];

    GetClassNameA(hWnd, szWinClass, ARRAYSIZE(szWinClass));

    return (BOOL)(0 == lstrcmpiA(szWinClass, CB_SUBWINCLASS_LB_NAME));
}


LRESULT ListBoxSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
    BOOL fOwnerDraw = (BOOL)(dwStyle & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE));

    ASSERT(uMsg);   // if happened, most possible cause is non-registered message

    // get the item count...
    INT_PTR iLBCnt = DefSubclassProc(hwnd, LB_GETCOUNT, (WPARAM)0L, (LPARAM)0L);

    switch (uMsg)
    {
        case WM_DESTROY:
        {
            if (hwnd)
            {
                // delete any "item" if remained and this LB is not from CB
                if (!MLIsLBFromCB(hwnd))
                    SendMessage(hwnd, LB_RESETCONTENT, wParam, lParam);

                RemoveProp(hwnd, ML_LB_NTBUG);
                // Must remove by atom - Win95 compat
                RemoveProp(hwnd, MAKEINTATOM(g_atmML));
                RemoveWindowSubclass(hwnd, ListBoxSubclassProc,  0);
            }

            LocalFree(((LPCITEM)dwRefData));

            break;   // fall through default process
        }

        case LB_DELETESTRING:
        {
            INT_PTR iIndex = (INT_PTR)wParam;
            
            if ((iIndex >= iLBCnt) || MLIsLBFromCB(hwnd))
                break;   // fall through default process

            if (MLLBCBDoDeleteItem(hwnd, iIndex))
            {
                // for taking care NT bug: not adjusting item count before send LB_RESETCONTENT message
                if (g_bRunningOnNT && (iLBCnt == 1))
                    SetProp(hwnd, ML_LB_NTBUG, (HANDLE)(1L));

                break;   // fall through default process
            }
            else
                return LB_ERR;
        }

        case LB_RESETCONTENT:
        {
            if ((iLBCnt > 0) && (!MLIsLBFromCB(hwnd)))
            {
                // for taking care NT bug: not adjusting item count before send LB_RESETCONTENT message
                if (g_bRunningOnNT && (iLBCnt == 1) && ((HANDLE)(1L) == GetProp(hwnd, ML_LB_NTBUG)))
                {
                    // reset the flag first
                    SetProp(hwnd, ML_LB_NTBUG, (HANDLE)(0L));

                    break;   // fall through default process
                }

                for (INT_PTR iLP = 0; iLP < iLBCnt; iLP++)
                    MLLBCBDoDeleteItem(hwnd, iLP);
            }

            break;   // fall through default process
        }

        case WM_MOUSEMOVE:
        {
            RECT rcListBox;

            // don't need to take care drawing if OwnerDraw...
            if (fOwnerDraw)
                break;      // fall through default process

            // decide if this is sub win, LB inside CB.
            BOOL fIsCB_LB = MLIsLBFromCB(hwnd);

            GetClientRect(hwnd, &rcListBox);
            int iXPos = LOWORD(lParam) | ((LOWORD(lParam) & (WORD)(0x8000)) ? (int)(0xFFFF0000) : (int)(0));
            int iYPos = HIWORD(lParam) | ((HIWORD(lParam) & (WORD)(0x8000)) ? (int)(0xFFFF0000) : (int)(0));
            if ((wParam & MK_LBUTTON) ||
                (fIsCB_LB && (iXPos < rcListBox.right) && (iYPos < rcListBox.bottom) &&
                             (iXPos >= rcListBox.left) && (iYPos >= rcListBox.top))
               )
            {
                RECT rcRePaint;
                INT_PTR iPrevSel, iCurSel;
                LRESULT fPrevStat, fCurStat;
                BOOL fRePaint = FALSE;

                int iItemHeight = (int)DefSubclassProc(hwnd, LB_GETITEMHEIGHT, (WPARAM)0L, (LPARAM)0L);
                iPrevSel = iCurSel 
                         = DefSubclassProc(
                                hwnd, 
                                ((dwStyle & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL)) ? LB_GETCARETINDEX : LB_GETCURSEL),
                                (WPARAM)0L, (LPARAM)0L
                           );
                fPrevStat = DefSubclassProc(hwnd, LB_GETSEL, (WPARAM)iPrevSel, (LPARAM)0L);
                if (iPrevSel == LB_ERR)
                    iPrevSel = DefSubclassProc(hwnd, LB_GETTOPINDEX, (WPARAM)0L, (LPARAM)0L);
                DefSubclassProc(hwnd, LB_GETITEMRECT, (WPARAM)iPrevSel, (LPARAM)&rcRePaint);

                if ((max(0, iYPos) < rcRePaint.top) || ((!fIsCB_LB) && (iYPos >= (rcListBox.bottom - 1))))
                {
                    rcRePaint.top -= iItemHeight;
                    fRePaint = TRUE;
                }

                if ((!fRePaint) &&
                    ((min((rcListBox.bottom - 1), iYPos) >= rcRePaint.bottom) || ((!fIsCB_LB) && (iYPos < 0))))
                {
                    rcRePaint.bottom += iItemHeight;
                    rcRePaint.bottom = min(rcRePaint.bottom, rcListBox.bottom);
                    fRePaint = TRUE;
                }

                if (fRePaint)
                {
                    LRESULT lrRtVal = DefSubclassProc(hwnd, uMsg, wParam, lParam);

                    // check if new selection is more than one item away (for LB inside CB only)...
                    if (fIsCB_LB)
                    {
                        iCurSel = DefSubclassProc(hwnd, LB_GETCURSEL, (WPARAM)0L, (LPARAM)0L);
                        if (((iCurSel > iPrevSel) ? (iCurSel - iPrevSel) : (iPrevSel - iCurSel)) > 1)
                        {
                            RECT rcCurSel;

                            DefSubclassProc(hwnd, LB_GETITEMRECT, (WPARAM)iCurSel, (LPARAM)&rcCurSel);
                            rcRePaint.top = min(rcRePaint.top, rcCurSel.top);
                            rcRePaint.bottom = max(rcRePaint.bottom, rcCurSel.bottom);
                        }
                    }

                    InvalidateRect(hwnd, &rcRePaint, FALSE);
                    return lrRtVal;
                }
                else if ((fIsCB_LB) && (wParam & MK_LBUTTON))
                {
                    LRESULT lrRtVal = DefSubclassProc(hwnd, uMsg, wParam, lParam);
                    if (iCurSel == LB_ERR)
                        iCurSel = DefSubclassProc( hwnd, LB_GETCURSEL, (WPARAM)0L, (LPARAM)0L);
                    fCurStat = DefSubclassProc(hwnd, LB_GETSEL, (WPARAM)iCurSel, (LPARAM)0L);

                    if (fPrevStat != fCurStat)
                    {
                        DefSubclassProc(hwnd, LB_GETITEMRECT, (WPARAM)iCurSel, (LPARAM)&rcRePaint);
                        InvalidateRect(hwnd, &rcRePaint, FALSE);
                    }

                    return lrRtVal;
                }
            }

            break;      // fall through default process
        }

        case WM_LBUTTONDOWN:
        case WM_KEYDOWN:
        {
            // don't need to take care drawing if OwnerDraw...
            if (fOwnerDraw)
                break;      // fall through default process

            RECT rcListBox;
            BOOL fPaint = TRUE;

            GetClientRect(hwnd, &rcListBox);

            if (uMsg == WM_KEYDOWN)
            {
                // WM_KEYDOWN
                INT_PTR iCaretIdx, iTopIdx, iBottomIdx;
                int iItemHeight;

                iCaretIdx = DefSubclassProc(hwnd, LB_GETCARETINDEX, (WPARAM)0L, (LPARAM)0L);
                iTopIdx = DefSubclassProc(hwnd, LB_GETTOPINDEX, (WPARAM)0L, (LPARAM)0L);
                iItemHeight = (int)DefSubclassProc(hwnd, LB_GETITEMHEIGHT, (WPARAM)0L, (LPARAM)0L);
                iBottomIdx = iTopIdx + min(iLBCnt, ((rcListBox.bottom - rcListBox.top) / iItemHeight)) - 1;

                switch (wParam)
                {
                    case VK_UP:
                    case VK_LEFT:
                    case VK_PRIOR:
                    case VK_HOME:
                    {
                        if ((iTopIdx == 0) && (iCaretIdx == 0) &&
                            (0 < DefSubclassProc(hwnd, LB_GETSEL, (WPARAM)0L, (LPARAM)0L)))
                            fPaint = FALSE;

                        break;
                    }

                    case VK_DOWN:
                    case VK_RIGHT:
                    case VK_NEXT:
                    case VK_END:
                    {
                        iLBCnt--;
                        if ((iBottomIdx == iLBCnt) && (iCaretIdx == iLBCnt) &&
                            (0 < DefSubclassProc(hwnd, LB_GETSEL, (WPARAM)iLBCnt, (LPARAM)0L)))
                            fPaint = FALSE;

                        break;
                    }

                    default:
                        if ((!InRange((int)wParam, VK_A, VK_Z)) && (!InRange((int)wParam, VK_0, VK_9)))
                            fPaint = FALSE;
                }
            }
            // always paint if it is WM_LBBUTTONDOWN

            if (fPaint)
            {
                LRESULT lrRtVal = DefSubclassProc(hwnd, uMsg, wParam, lParam);
                InvalidateRect(hwnd, &rcListBox, FALSE);
                UpdateWindow(hwnd);
                return lrRtVal;
            }

            break;      // fall through default process
        }

        case WM_LBUTTONUP:
        {
            // don't need to take care drawing if OwnerDraw...
            if (fOwnerDraw)
                break;      // fall through default process

            // For cooperate with window's 'behavior':
            //      Window's LB routine only updates the selection status at mouse button up.
            //      This only affects to multiple select LB.
            if (dwStyle & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL))
            {
                INT_PTR iAnchorIdx, iCaretIdx;
                RECT rcAnchor, rcCaret, rcClient, rcUpdate;

                GetClientRect(hwnd, &rcClient);
                iAnchorIdx = DefSubclassProc(hwnd, LB_GETANCHORINDEX, (WPARAM)0L, (LPARAM)0L);
                DefSubclassProc(hwnd, LB_GETITEMRECT, (WPARAM)iAnchorIdx, (LPARAM)&rcAnchor);
                iCaretIdx = DefSubclassProc(hwnd, LB_GETCARETINDEX, (WPARAM)0L, (LPARAM)0L);
                DefSubclassProc(hwnd, LB_GETITEMRECT, (WPARAM)iCaretIdx, (LPARAM)&rcCaret);

                rcUpdate.left = rcClient.left;  rcUpdate.right = rcClient.right;
                rcUpdate.top    = max(rcClient.top, min(rcAnchor.top, rcCaret.top));
                rcUpdate.bottom = min(rcClient.bottom, max(rcAnchor.bottom, rcCaret.bottom));

                LRESULT lrRtVal = DefSubclassProc(hwnd, uMsg, wParam, lParam);
                InvalidateRect(hwnd, &rcUpdate, FALSE);
                UpdateWindow(hwnd);

                return lrRtVal;
            }

            break;      // fall through default process
        }

        case LB_SETSEL:
        case LB_SETCURSEL:
        {
            INT_PTR iPrevSel, iCurSel, iPrevTopIdx, iCurTopIdx;
            RECT rcPrevSel, rcCurSel, rcUpdate;
            UINT uiSelMsg;

            // don't need to take care drawing if OwnerDraw...
            if (fOwnerDraw)
                break;      // fall through default process

            // TODO: ??? single or multiple selection LB ???
            if (uMsg == LB_SETSEL)
            {
                // multiple selection LB
                iCurSel = (INT_PTR)lParam;
                uiSelMsg = LB_GETCARETINDEX;
            }
            else // LB_SETCURSEL
            {
                // single selection LB
                iCurSel = (INT_PTR)wParam;
                uiSelMsg = LB_GETCURSEL;
            }

            if ((iCurSel < 0) || (iCurSel >= iLBCnt))
                break;      // out of range, fall through default process

            // get the prev. selection info.
            iPrevTopIdx = DefSubclassProc(hwnd, LB_GETTOPINDEX, (WPARAM)0L, (LPARAM)0L);
            iPrevSel = DefSubclassProc(hwnd, uiSelMsg, (WPARAM)0L, (LPARAM)0L);
            if (iPrevSel != LB_ERR)   // item is currently selected.
                DefSubclassProc(hwnd, LB_GETITEMRECT, (WPARAM)iPrevSel, (LPARAM)&rcPrevSel);

            // let the system do the work...
            DefSubclassProc(hwnd, uMsg, wParam, lParam);

            // get the current selection info.
            iCurTopIdx = DefSubclassProc(hwnd, LB_GETTOPINDEX, (WPARAM)0L, (LPARAM)0L);
            DefSubclassProc(hwnd, LB_GETITEMRECT, (WPARAM)iCurSel, (LPARAM)&rcCurSel);

            // adjust the update range
            if (iCurTopIdx != iPrevTopIdx)
            {
                GetClientRect(hwnd, &rcUpdate);
            }
            else
            {
                CopyRect(&rcUpdate, &rcCurSel);

                if (iPrevSel != LB_ERR)
                {
                    rcUpdate.top = min(rcPrevSel.top, rcCurSel.top);
                    rcUpdate.bottom = max(rcPrevSel.bottom, rcCurSel.bottom);
                }
                else
                {
                    rcUpdate.top = rcCurSel.top;
                    rcUpdate.bottom = rcCurSel.bottom;
                }
            }

            // do the update...
            InvalidateRect(hwnd, &rcUpdate, FALSE);
            UpdateWindow(hwnd);

            return 0;   // message has been processed
        }

        case WM_KILLFOCUS:
        case WM_SETFOCUS:
        case WM_ENABLE:
        {
            RECT rcItem,
                 *prcRePaint = &rcItem;

            if (uMsg != WM_ENABLE)
            {
                INT_PTR iCurSel;

                if (LB_ERR == (iCurSel = DefSubclassProc(hwnd, LB_GETCURSEL, (WPARAM)0L, (LPARAM)0L)))
                    break;   // fall through default process
                else
                    DefSubclassProc(hwnd, LB_GETITEMRECT, (WPARAM)iCurSel, (LPARAM)&rcItem);
            }
            else
                prcRePaint = NULL;   // paint whole client area

            LRESULT lrRtVal = DefSubclassProc(hwnd, uMsg, wParam, lParam);
            InvalidateRect(hwnd, prcRePaint, FALSE);
            UpdateWindow(hwnd);

            return lrRtVal;
        }

        case WM_PRINTCLIENT:
        case WM_PAINT:
        {
            // don't need to take care drawing if OwnerDraw...
            if ((iLBCnt == 0) || (fOwnerDraw))
                break;   // nothing to draw, fall through default process

            HDC hdcDC;
            PAINTSTRUCT psCtrl;
            HFONT hfontFontSav;
            RECT rcItemRC;
            int iBgModeSav, iItemHeight;
            COLORREF clrTxtSav, clrBkSav, clrHiTxt, clrHiBk;
            UINT uiFormat;
            BOOL fDisabled = (BOOL)(dwStyle & WS_DISABLED),
                 fPrtClient = (BOOL)(uMsg == WM_PRINTCLIENT),
                 fHighLighted;

            // get system info
            if (fPrtClient)
            {
                hdcDC = psCtrl.hdc = (HDC)wParam;
                GetClientRect(hwnd, &(psCtrl.rcPaint));
            }
            else
                hdcDC = BeginPaint(hwnd, &psCtrl);
            hfontFontSav = (HFONT)SelectObject(hdcDC, GetWindowFont(hwnd));
            iBgModeSav = SetBkMode(hdcDC, TRANSPARENT);
            if (fDisabled)   // ListBox is disabled
            {
                clrTxtSav = SetTextColor(hdcDC, GetSysColor(COLOR_GRAYTEXT));
                clrBkSav = SetBkColor(hdcDC, GetSysColor(COLOR_BTNFACE));
            }
            else
            {
                clrHiTxt   = GetSysColor(COLOR_HIGHLIGHTTEXT);
                clrHiBk    = GetSysColor(COLOR_HIGHLIGHT);
            }

            // get item info
            iItemHeight = (int)DefSubclassProc(hwnd, LB_GETITEMHEIGHT, (WPARAM)0L, (LPARAM)0L);

            // process the format of the text
            //TODO: process the text format
            uiFormat = DT_LEFT | DT_VCENTER;

            INT_PTR iFocusIdx = DefSubclassProc(hwnd, LB_GETCARETINDEX, (WPARAM)0L, (LPARAM)0L);

            // find the first item has to be painted
            INT_PTR iTopIdx = DefSubclassProc(hwnd, LB_GETTOPINDEX, (WPARAM)0L, (LPARAM)0L);
            // if the paint area does not start from top
            if (psCtrl.rcPaint.top != 0)
                iTopIdx += (psCtrl.rcPaint.top - 0) / iItemHeight;

            // decide the number of items to be painted
            int iPaintHeight = psCtrl.rcPaint.bottom - psCtrl.rcPaint.top;
            int iNumItems = iPaintHeight / iItemHeight;
            // if there is space (more than 1 pt) left at bottom or paint start in the middle of an item
            if (((iPaintHeight % iItemHeight) > 0) || (((psCtrl.rcPaint.top - 0) % iItemHeight) > 0))
                iNumItems++;
            
            for (INT_PTR iLP = iTopIdx;
                 iLP < min(iLBCnt, (iTopIdx + iNumItems));
                 iLP++)
            {
                RECT rcTempRC;

                // get item info
                LPWSTR lpwszStr = NULL;

                if (!MLLBCBGetWStrPtr(hwnd, iLP, &lpwszStr))
                    goto BailOut;   // message processed but failed.

                DefSubclassProc(hwnd, LB_GETITEMRECT, (WPARAM)iLP, (LPARAM)&rcItemRC);

                // adjust drawing area if we have to draw part of an item
                rcItemRC.bottom = min(rcItemRC.bottom, psCtrl.rcPaint.bottom);

                // decide the text color...
                fHighLighted = (BOOL)(DefSubclassProc(hwnd, LB_GETSEL, (WPARAM)iLP, (LPARAM)0L) > 0);
                if ((!fDisabled) && (fHighLighted))
                {
                    clrTxtSav   = SetTextColor(hdcDC, clrHiTxt);
                    clrBkSav    = SetBkColor(hdcDC, clrHiBk);
                }

                // draw the string...
                CopyRect(&rcTempRC, &rcItemRC);
                rcTempRC.left += 2; rcTempRC.right -= 2;
                ExtTextOut(hdcDC, 0, 0, ETO_OPAQUE, &rcItemRC, TEXT(""), 0, NULL);   // fill the background
                DrawTextFLW(hdcDC, lpwszStr, lstrlenW(lpwszStr), &rcTempRC, uiFormat);

                // restore the text color
                if ((!fDisabled) && (fHighLighted))
                {
                    SetTextColor(hdcDC, clrTxtSav);
                    SetBkColor(hdcDC, clrBkSav);
                }

                // put focus rectangle if ListBox is active
                if (rcItemRC.top < psCtrl.rcPaint.top)
                    rcItemRC.top = psCtrl.rcPaint.top - 1;

                // TODO: should not draw focus rect within Begin-End paint.
                if ((!fDisabled) && (iLP == iFocusIdx) && ((hwnd == GetFocus()) || MLIsLBFromCB(hwnd)))
                    DrawFocusRect(hdcDC, &rcItemRC);
            }

BailOut:
            // restore system info
            if (fDisabled)
            {
                SetTextColor(hdcDC, clrTxtSav);
                SetBkColor(hdcDC, clrBkSav);
            }
            SetBkMode(hdcDC, iBgModeSav);
            if (hfontFontSav)
                SelectObject(hdcDC, hfontFontSav);
            if (!fPrtClient)
                EndPaint(hwnd, &psCtrl);

            return 0;   // message has been processed
        }

        case LB_ADDSTRING:
        case LB_FINDSTRING:
        case LB_FINDSTRINGEXACT:
        case LB_GETTEXT:
        case LB_GETTEXTLEN:
        case LB_INSERTSTRING:
        case LB_SELECTSTRING:
        {
            // take care for the OS can handle unicode, but msg not get thunk (like NT4).
            if (MLIsEnabled(hwnd))
            {
                UINT uiMsgTx;

                switch (uMsg)
                {
                    case LB_ADDSTRING:
                        uiMsgTx = g_ML_LB_ADDSTRING;
                        break;
                    case LB_FINDSTRING:
                        uiMsgTx = g_ML_LB_FINDSTRING;
                        break;
                    case LB_FINDSTRINGEXACT:
                        uiMsgTx = g_ML_LB_FINDSTRINGEXACT;
                        break;
                    case LB_GETTEXT:
                        uiMsgTx = g_ML_LB_GETTEXT;
                        break;
                    case LB_GETTEXTLEN:
                        uiMsgTx = g_ML_LB_GETTEXTLEN;
                        break;
                    case LB_INSERTSTRING:
                        uiMsgTx = g_ML_LB_INSERTSTRING;
                        break;
                    case LB_SELECTSTRING:
                        uiMsgTx = g_ML_LB_SELECTSTRING;
                        break;

                    default:
                        ASSERT(0);
                }

                return SendMessage(hwnd, uiMsgTx, wParam, lParam);
            }

            break;   // should not happen, fall through default process
        }

        default:
        {
            // non-registered window message
            if ((uMsg < MIN_REG_WINMSG) || (uMsg > MAX_REG_WINMSG))
                break;   // fall through default process

            // ML_LB_ADDSTRING:
            // ML_LB_INSERTSTRING:
            if ((uMsg == g_ML_LB_ADDSTRING) || (uMsg == g_ML_LB_INSERTSTRING))
            {
                if (!lParam)
                    break;   // fall through default process

                if (MLIsLBFromCB(hwnd))
                    return DefSubclassProc(hwnd, ((uMsg == g_ML_LB_ADDSTRING) ? LB_ADDSTRING : LB_INSERTSTRING),
                                           wParam, lParam);
                else
                    return (LRESULT)MLLBCBAddInsertString(hwnd, uMsg, wParam, lParam, iLBCnt, dwStyle, TRUE);
            }

            // ML_LB_GETTEXT:
            // ML_LB_GETTEXTLEN:
            if ((uMsg == g_ML_LB_GETTEXT) || (uMsg == g_ML_LB_GETTEXTLEN))
            {
                if (((INT_PTR)wParam >= iLBCnt) || ((uMsg == g_ML_LB_GETTEXT) && (!lParam)))
                    break;   // fall through default process

                if (MLIsLBFromCB(hwnd))
                    return DefSubclassProc(hwnd, ((uMsg == g_ML_LB_GETTEXT) ? LB_GETTEXT : LB_GETTEXTLEN),
                                           wParam, lParam);
                else
                    return (LRESULT)MLLBCBGetLBTextNLength(hwnd, uMsg, wParam, lParam, iLBCnt, TRUE);
            }

            // ML_LB_FINDSTRING:
            // ML_LB_FINDSTRINGEXACT:
            if ((uMsg == g_ML_LB_FINDSTRING) || (uMsg == g_ML_LB_FINDSTRINGEXACT))
            {
                if ((!lParam) || ((INT_PTR)wParam >= iLBCnt))
                    break;   // fall through default process

                return (LRESULT)MLLBCBFindStringNExact(hwnd, uMsg, wParam, lParam, iLBCnt, TRUE);
            }

            // ML_LB_SELECTSTRING:
            if (uMsg == g_ML_LB_SELECTSTRING)
            {
                INT_PTR iIndex = SendMessage(hwnd, g_ML_LB_FINDSTRING, wParam, lParam);

                if (iIndex >= 0)
                {
                    if (dwStyle & LBS_EXTENDEDSEL)
                        SendMessage(hwnd, LB_SETSEL, (WPARAM)TRUE, (LPARAM)iIndex);
                    else
                        SendMessage(hwnd, LB_SETCURSEL, (WPARAM)iIndex, (LPARAM)0L);
                }

                return (LRESULT)iIndex;
            }

        }   // switch-default

    }   // switch

    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}
//--- ListBox -------------------------------------------------


//--- ComboBox -------------------------------------------------
BOOL ComboBoxStyleCheck(DWORD dwStyle)
{
    // HasStrings   0   0   1   1
    // OwnerDraw    0   1   0   1
    // SubClassing  Y   N   Y   N
    //              ^
    //              CBS_HASSTRINGS is a default setting if not OwnerDraw.

    switch (LOBYTE(dwStyle) & 0x0F)
    {
        case CBS_DROPDOWN:
        case CBS_DROPDOWNLIST:
        {
            if (dwStyle & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE))
                return FALSE;

            static BOOL fRegMsgOK = FALSE;

            if (FALSE == fRegMsgOK)
            {
                // register window message(s)
                fRegMsgOK = (BOOL)(g_ML_GETTEXT             = RegisterWindowMessageA("ML_GETTEXT"))            &&
                            (BOOL)(g_ML_GETTEXTLENGTH       = RegisterWindowMessageA("ML_GETTEXTLENGTH"))      &&
                            (BOOL)(g_ML_SETTEXT             = RegisterWindowMessageA("ML_SETTEXT"))            &&
                            (BOOL)(g_ML_CB_ADDSTRING        = RegisterWindowMessageA("ML_CB_ADDSTRING"))       &&
                            (BOOL)(g_ML_CB_FINDSTRING       = RegisterWindowMessageA("ML_CB_FINDSTRING"))      &&
                            (BOOL)(g_ML_CB_FINDSTRINGEXACT  = RegisterWindowMessageA("ML_CB_FINDSTRINGEXACT")) &&
                            (BOOL)(g_ML_CB_GETLBTEXT        = RegisterWindowMessageA("ML_CB_GETLBTEXT"))       &&
                            (BOOL)(g_ML_CB_GETLBTEXTLEN     = RegisterWindowMessageA("ML_CB_GETLBTEXTLEN"))    &&
                            (BOOL)(g_ML_CB_INSERTSTRING     = RegisterWindowMessageA("ML_CB_INSERTSTRING"))    &&
                            (BOOL)(g_ML_CB_SELECTSTRING     = RegisterWindowMessageA("ML_CB_SELECTSTRING"));
            }

            return fRegMsgOK;
        }

        case CBS_SIMPLE:
            //TODO: implement this style of CB
        default:
            return FALSE;
    }
}


void MLCBReDrawSelection(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static int g_iSM_CXEDGE     = GetSystemMetrics(SM_CXEDGE),
               g_iSM_CYEDGE     = GetSystemMetrics(SM_CYEDGE),
               g_iSM_CXVSCROLL  = GetSystemMetrics(SM_CXVSCROLL);
    RECT rcRePaint;

    // adjust repaint area...
    GetClientRect(hWnd, &rcRePaint);
    rcRePaint.left += g_iSM_CXEDGE;   rcRePaint.right -= (g_iSM_CXEDGE + g_iSM_CXVSCROLL);
    rcRePaint.top += g_iSM_CYEDGE;    rcRePaint.bottom -= g_iSM_CYEDGE;

    // default process...(must!!)
    DefSubclassProc(hWnd, uMsg, wParam, lParam);

    // repaint the area...
    InvalidateRect(hWnd, &rcRePaint, FALSE);
    UpdateWindow(hWnd);

    return;
}


LRESULT ComboBoxSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
    BOOL fSimpleCB      = ((LOBYTE(dwStyle) & 0x0F) == CBS_SIMPLE),         // == 1L
         fDropDnCB      = ((LOBYTE(dwStyle) & 0x0F) == CBS_DROPDOWN),       // == 2L
         fDropDnListCB  = ((LOBYTE(dwStyle) & 0x0F) == CBS_DROPDOWNLIST);   // == 3L

    ASSERT(uMsg);   // if happened, most possible cause is non-registered message

    // get the item count first
    INT_PTR iCBCnt = DefSubclassProc(hwnd, CB_GETCOUNT, (WPARAM)0L, (LPARAM)0L);

    switch (uMsg)
    {
        case WM_DESTROY:
        {
            if (hwnd)
            {
                //    delete any "item" if remained
                SendMessage(hwnd, CB_RESETCONTENT, wParam, lParam);

                LPMLCBSUBWINHND lpmlcbsubwin = (LPMLCBSUBWINHND)RemoveProp(hwnd, ML_CB_SUBWIN);
                if (lpmlcbsubwin)
                    LocalFree(lpmlcbsubwin);

                // Must remove by atom - Win95 compat
                RemoveProp(hwnd, MAKEINTATOM(g_atmML));
                RemoveWindowSubclass(hwnd, ComboBoxSubclassProc,  0);
            }

            LocalFree(((LPCITEM)dwRefData));

            break;   // fall through default process
        }

        case CB_DELETESTRING:
        {
            INT_PTR iIndex = (INT_PTR)wParam;
         
            if ((iIndex >= iCBCnt) || (MLLBCBDoDeleteItem(hwnd, iIndex)))
                break;   // fall through default process
            else
                return CB_ERR;   // deletion failed
        }

        case CB_RESETCONTENT:
        {
            if (iCBCnt > 0)
            {
                for (INT_PTR iLP = 0; iLP < iCBCnt; iLP++)
                    MLLBCBDoDeleteItem(hwnd, iLP);
            }

            break;   // fall through default process
        }

        case WM_GETTEXT:
        case WM_GETTEXTLENGTH:
        {
            BOOL fGetText = (BOOL)(uMsg == WM_GETTEXT);
         
            if (fGetText && (!lParam))
                return CB_ERR;

            if (IsWindowUnicode(hwnd))
            {
                return SendMessage(hwnd, (fGetText ? g_ML_GETTEXT : g_ML_GETTEXTLENGTH), wParam, lParam);
            }
            else
            {
                // get the string out
                LPWSTR lpwszTemp = (LPWSTR)LocalAlloc(LPTR, MAX_BUFFER_BYTE);
                if (!lpwszTemp)
                    return CB_ERRSPACE;

                if (CB_ERR == SendMessage(hwnd, g_ML_GETTEXT, wParam, (LPARAM)lpwszTemp))
                    return CB_ERR;

                int iStrLenA = WideCharToMultiByte(CP_ACP, 0, lpwszTemp, -1, NULL, 0, NULL, NULL);

                if (fGetText)
                {
                    if (lParam)
                    {
                        WideCharToMultiByte(CP_ACP, 0, lpwszTemp, -1, (LPSTR)lParam, iStrLenA, NULL, NULL);
                        ((LPSTR)lParam)[iStrLenA] = 0;   // ending the string
                    }
                    else
                        return CB_ERR;
                }

                LocalFree(lpwszTemp);

                return iStrLenA;
            }
        }

        case WM_SETTEXT:
        {
            if (fDropDnListCB)
                return CB_ERR;

#ifdef LATER_IE5
            // convert ANSI string to unicode string
            if (lParam)
            {
                // get child (edit ctl) window handler
                HWND hwndEdit = GetWindow(hwnd, GW_CHILD);
                if ((hwndEdit) && (fSimpleCB))
                    hwndEdit = GetWindow(hwndEdit, GW_HWNDNEXT);
                if (!hwndEdit)
                    return CB_ERR;

  #ifdef DEBUG
                CHAR szWinClass[MAX_WINCLASS_NAME];

                GetClassNameA(hwndEdit, szWinClass, ARRAYSIZE(szWinClass));
                ASSERT(0 == lstrcmpiA(szWinClass, CB_SUBWINCLASS_ED_NAME));
  #endif

                if (MLIsEnabled(hwndEdit))
                {
                    int iStrLen = lstrlenA((LPSTR)lParam) + 1;
                    LPWSTR lpwszTemp = (LPWSTR)LocalAlloc(LPTR, (iStrLen * sizeof(WCHAR)));
                    if (!lpwszTemp)
                        return CB_ERRSPACE;

                    MultiByteToWideChar(CP_ACP, 0, (LPSTR)lParam, -1, lpwszTemp, iStrLen);
                    INT_PTR iRtVal = SendMessage(hwnd, g_ML_SETTEXT, wParam, (LPARAM)lpwszTemp);
                    LocalFree(lpwszTemp);

                    return (iRtVal == CB_ERR) ? CB_ERR : TRUE;
                }
                else
                    return SendMessage(hwndEdit, WM_SETTEXT, (WPARAM)0L, lParam);
            }
            else
                return CB_ERR;
#else
            break;   // fall through default process
#endif
        }

        case WM_PRINTCLIENT:
        case WM_PAINT:
        {
            static int g_iSM_CXEDGE     = GetSystemMetrics(SM_CXEDGE),
                       g_iSM_CYEDGE     = GetSystemMetrics(SM_CYEDGE),
                       g_iSM_CXVSCROLL  = GetSystemMetrics(SM_CXVSCROLL);

            HDC hdcDC;
            HFONT hfontFontSav;
            RECT rcComboBox, rcUpdate, rcBgDraw;
            int iBgModeSav;
            INT_PTR iCurSel = CB_ERR * 100;   // set to an invalid value
            COLORREF clrTxtSav, clrBkSav, clrHiTxt, clrHiBk;
            UINT uiFormat;
            BOOL fUpdateRect, fFocused, fPedDropped,
                 fPrtClient = (BOOL)(uMsg == WM_PRINTCLIENT),
                 fDisabled = (dwStyle & WS_DISABLED);

            GetClientRect(hwnd, &rcComboBox);
            rcBgDraw.left = rcComboBox.left + g_iSM_CXEDGE;
            rcBgDraw.right = rcComboBox.right - g_iSM_CXEDGE - g_iSM_CXVSCROLL;
            rcBgDraw.top = rcComboBox.top + g_iSM_CYEDGE;
            rcBgDraw.bottom = rcComboBox.bottom - g_iSM_CYEDGE;
            fUpdateRect = GetUpdateRect(hwnd, &rcUpdate, FALSE);
            // chk if we need to draw anything...
            if ((rcUpdate.left >= rcBgDraw.right) || (!fUpdateRect) ||
                (iCBCnt == 0))
                break;   // fall through the default process

            if (!(EqualRect(&rcUpdate, &rcBgDraw)))
                DefSubclassProc(hwnd, uMsg, wParam, lParam);
            ValidateRect(hwnd, &rcUpdate);

            // adjust the drawing area...
            RECT rcTxtDraw;
            int iItemHeight, iOffset;

            CopyRect(&rcTxtDraw, &rcBgDraw);
            rcTxtDraw.left += g_iSM_CXEDGE;    rcTxtDraw.right -= g_iSM_CXEDGE;
            iItemHeight = (int)DefSubclassProc(hwnd, CB_GETITEMHEIGHT, (WPARAM)-1L, (LPARAM)0L);   // item height in selected area
            iOffset = max(((rcTxtDraw.bottom - rcTxtDraw.top - iItemHeight) / 2), 0);
            rcTxtDraw.top += iOffset;   rcTxtDraw.bottom -= iOffset;

            // process the format of the text...
            // TODO: process the text format
            uiFormat = DT_LEFT | DT_VCENTER;

            // get item info...
            LPWSTR lpwszStr = NULL;

            iCurSel = DefSubclassProc(hwnd, CB_GETCURSEL, (WPARAM)0L, (LPARAM)0L);
            if ((iCurSel == CB_ERR) || (!MLLBCBGetWStrPtr(hwnd, iCurSel, &lpwszStr)))
                return 1;   // message not processed

            // get system info...
            if (fPrtClient)
                hdcDC = (HDC)wParam;
            else
                hdcDC = GetDC(hwnd);
            hfontFontSav = (HFONT)SelectObject(hdcDC, GetWindowFont(hwnd));
            iBgModeSav = SetBkMode(hdcDC, TRANSPARENT);
            fFocused = (BOOL)(hwnd == GetFocus());
            fPedDropped = (BOOL)DefSubclassProc(hwnd, CB_GETDROPPEDSTATE, (WPARAM)0L, (LPARAM)0L);
            if (fDisabled)   // ComboBox is disabled
            {
                clrTxtSav = SetTextColor(hdcDC, GetSysColor(COLOR_GRAYTEXT));
                clrBkSav = SetBkColor(hdcDC, GetSysColor(COLOR_BTNFACE));
            }
            else
            {
                clrHiTxt   = GetSysColor(COLOR_HIGHLIGHTTEXT);
                clrHiBk    = GetSysColor(COLOR_HIGHLIGHT);
            }

            // decide the drawing color...
            if ((!fDisabled) && (fFocused) && (!fPedDropped))
            {
                clrTxtSav   = SetTextColor(hdcDC, clrHiTxt);
                clrBkSav    = SetBkColor(hdcDC, clrHiBk);
            }

            // draw the string & background...
            ExtTextOut(hdcDC, 0, 0, ETO_OPAQUE, &rcBgDraw, TEXT(""), 0, NULL);   // fill the background
            DrawTextFLW(hdcDC, lpwszStr, lstrlenW(lpwszStr), &rcTxtDraw, uiFormat);

            // restore the text color...
            if ((!fDisabled) && (fFocused) && (!fPedDropped))
            {
                SetTextColor(hdcDC, clrTxtSav);
                SetBkColor(hdcDC, clrBkSav);

                // draw the focus rect...
                DrawFocusRect(hdcDC, &rcBgDraw);
            }

            // restore system info...
            if (fDisabled)
            {
                SetTextColor(hdcDC, clrTxtSav);
                SetBkColor(hdcDC, clrBkSav);
            }
            SetBkMode(hdcDC, iBgModeSav);
            if (hfontFontSav)
                SelectObject(hdcDC, hfontFontSav);
            if (!fPrtClient)
                ReleaseDC(hwnd, hdcDC);

            return 0;   // message has been processed
        }

        case WM_ENABLE:
        case WM_SETFOCUS:
        case WM_KILLFOCUS:
        {
            MLCBReDrawSelection(hwnd, uMsg, wParam, lParam);

            return 0;
        }

        case WM_CTLCOLORLISTBOX:
        {
            HWND hwndLB = (HWND)lParam;

            LPMLCBSUBWINHND lpmlcbsubwin = (LPMLCBSUBWINHND)GetProp(hwnd, ML_CB_SUBWIN);
            if ((!lpmlcbsubwin) || (!IsWindow(lpmlcbsubwin->hwndLBSubWin)) || (hwndLB != lpmlcbsubwin->hwndLBSubWin))
            {
                // subclassing this LB...
                LPCITEM lpLBCItem = (LPCITEM)LocalAlloc(LPTR, sizeof(CITEM));
                if ((lpLBCItem) && (ListBoxStyleCheck(dwStyle = GetWindowLong(hwndLB, GWL_STYLE))))
                {
                    lpLBCItem->dwStyle = dwStyle;
                    GetClientRect(hwndLB, &(lpLBCItem->rc));
                    SetWindowSubclass(hwndLB, LB_SUB_PROC, 0, (DWORD_PTR)lpLBCItem);
                    // Must add by name - Win95 compat
                    SetProp(hwndLB, c_szML, (HANDLE)1);

                    // add prop. value into win prop...
                    if (!lpmlcbsubwin)
                        lpmlcbsubwin = (LPMLCBSUBWINHND)LocalAlloc(LPTR, sizeof(MLCBSUBWINHND));

                    if (lpmlcbsubwin)
                    {
                        lpmlcbsubwin->hwndLBSubWin = hwndLB;
                        SetProp(hwnd, ML_CB_SUBWIN, (HANDLE)lpmlcbsubwin);
                    }
                }
            }

            break;   // fall through default process
        }

        case WM_COMMAND:
        {
            switch (HIWORD(wParam))
            {
                case CBN_SELCHANGE:
                case CBN_SETFOCUS:
                case CBN_SELENDOK:
                case CBN_SELENDCANCEL:
                {
                    if (fDropDnListCB)
                    {
                        MLCBReDrawSelection(hwnd, uMsg, wParam, lParam);

                        return 0;
                    }

                    break;   // fall through default process
                }

                default:
                    break;   // fall through default process
            }

            break;   // fall through default process
        }

        case CB_SETCURSEL:
        {
            if (fDropDnListCB)
            {
                MLCBReDrawSelection(hwnd, uMsg, wParam, lParam);

                return 0;
            }

            break;   // fall through default process

        }

        case CB_ADDSTRING:
        case CB_FINDSTRING:
        case CB_FINDSTRINGEXACT:
        case CB_GETLBTEXT:
        case CB_GETLBTEXTLEN:
        case CB_INSERTSTRING:
        case CB_SELECTSTRING:
        {
            // take care for the OS can handle unicode, but msg not get thunk (like NT4).
            if (MLIsEnabled(hwnd))
            {
                UINT uiMsgTx;

                switch (uMsg)
                {
                    case CB_ADDSTRING:
                        uiMsgTx = g_ML_CB_ADDSTRING;
                        break;
                    case CB_FINDSTRING:
                        uiMsgTx = g_ML_CB_FINDSTRING;
                        break;
                    case CB_FINDSTRINGEXACT:
                        uiMsgTx = g_ML_CB_FINDSTRINGEXACT;
                        break;
                    case CB_GETLBTEXT:
                        uiMsgTx = g_ML_CB_GETLBTEXT;
                        break;
                    case CB_GETLBTEXTLEN:
                        uiMsgTx = g_ML_CB_GETLBTEXTLEN;
                        break;
                    case CB_INSERTSTRING:
                        uiMsgTx = g_ML_CB_INSERTSTRING;
                        break;
                    case CB_SELECTSTRING:
                        uiMsgTx = g_ML_CB_SELECTSTRING;
                        break;

                    default:
                        ASSERT(0);
                }

                return SendMessage(hwnd, uiMsgTx, wParam, lParam);
            }

            break;   // should not happen, fall through default process
        }

        default:
        {
            // non-registered window message
            if ((uMsg < MIN_REG_WINMSG) || (uMsg > MAX_REG_WINMSG))
                break;   // fall through default process

            // ML_CB_ADDSTRING:
            // ML_CB_INSERTSTRING:
            if ((uMsg == g_ML_CB_ADDSTRING) || (uMsg == g_ML_CB_INSERTSTRING))
            {
                if (!lParam)
                    break;   // fall through default process

                return (LRESULT)MLLBCBAddInsertString(hwnd, uMsg, wParam, lParam, iCBCnt, dwStyle, FALSE);
            }

            // ML_CB_GETLBTEXT:
            // ML_CB_GETLBTEXTLEN:
            if ((uMsg == g_ML_CB_GETLBTEXT) || (uMsg == g_ML_CB_GETLBTEXTLEN))
            {
                if (((INT_PTR)wParam >= iCBCnt) || ((uMsg == g_ML_CB_GETLBTEXT) && (!lParam)))
                    break;   // fall through default process

                return (LRESULT)MLLBCBGetLBTextNLength(hwnd, uMsg, wParam, lParam, iCBCnt, FALSE);
            }

            // ML_GETTEXT:
            // ML_GETTEXTLENGTH:
            if ((uMsg == g_ML_GETTEXT) || (uMsg == g_ML_GETTEXTLENGTH))
            {
                UINT uiTextMax = (UINT)wParam;
                LPWSTR lpwszText = (LPWSTR)lParam;
         
                if ((uMsg == g_ML_GETTEXT) && (!lpwszText))
                    break;   // fall through default process

                if (fDropDnCB)
                {
                    // dropdow list w/Edit Ctl
                    // TODO: not implement yet
                    break;   // fall through default process
                }
                else
                {
                    // DropDownList CB
                    INT_PTR iCurSel = DefSubclassProc(hwnd, CB_GETCURSEL, (WPARAM)0L, (LPARAM)0L);
                    if (iCurSel == CB_ERR)
                    {
                        // no text show up yet
                        if (uMsg == g_ML_GETTEXT)
                            lpwszText[0] = 0;
                 
                        return 0;
                    }
                    else
                    {
                        UINT uiBuffLen = (UINT)SendMessage(hwnd, g_ML_CB_GETLBTEXTLEN, (WPARAM)iCurSel, (LPARAM)0L);

                        if (uMsg == g_ML_GETTEXT)
                        {
                            LPWSTR lpwszTemp = (LPWSTR)LocalAlloc(LPTR, ((uiBuffLen + 1) * sizeof(WCHAR)));
                            if (!lpwszTemp)
                                return CB_ERRSPACE;

                            SendMessage(hwnd, g_ML_CB_GETLBTEXT, (WPARAM)iCurSel, (LPARAM)lpwszTemp);
                            StrCpyNW(lpwszText, lpwszTemp, min(uiTextMax, (uiBuffLen + 1)));
                            LocalFree(lpwszTemp);
                        }

                        return uiBuffLen;
                    }
                }
            }

            // ML_SETTEXT:
            if (uMsg == g_ML_SETTEXT)
            {
                LPWSTR lpwszText = (LPWSTR)lParam;

                if ((fDropDnListCB) || (!lpwszText) || wParam)
                    break;   // fall through default process

#ifdef LATER_IE5
                // get child (edit ctl) window handler
                HWND hwndEdit = GetWindow(hwnd, GW_CHILD);
                if ((hwndEdit) && (fSimpleCB))
                    hwndEdit = GetWindow(hwndEdit, GW_HWNDNEXT);
                if (!hwndEdit)
                    return CB_ERR;

  #ifdef DEBUG
                CHAR szWinClass[MAX_WINCLASS_NAME];

                GetClassNameA(hwndEdit, szWinClass, ARRAYSIZE(szWinClass));
                ASSERT(0 == lstrcmpiA(szWinClass, CB_SUBWINCLASS_ED_NAME));
  #endif

                if (MLIsEnabled(hwndEdit))
                    return SendMessage(hwndEdit, g_ML_SETTEXT, (WPARAM)0L, (LPARAM)lpwszText);
                else
                    return CB_ERR;
#else
                break;   // fall through default process
#endif
            }

            // ML_CB_FINDSTRING:
            // ML_CB_FINDSTRINGEXACT:
            if ((uMsg == g_ML_CB_FINDSTRING) || (uMsg == g_ML_CB_FINDSTRINGEXACT))
            {
                if ((!lParam) || ((INT_PTR)wParam >= iCBCnt))
                    break;   // fall through default process

                return (LRESULT)MLLBCBFindStringNExact(hwnd, uMsg, wParam, lParam, iCBCnt, FALSE);
            }

            // ML_CB_SELECTSTRING:
            if (uMsg == g_ML_CB_SELECTSTRING)
            {
                INT_PTR iIndex = SendMessage(hwnd, g_ML_CB_FINDSTRING, wParam, lParam);
                if (iIndex >= 0)
                    SendMessage(hwnd, CB_SETCURSEL, (WPARAM)iIndex, (LPARAM)0L);

                return iIndex;
            }

        }   // switch-default

    }   // switch

    // default procedure
    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}
//--- ComboBox -------------------------------------------------


BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam)
{
    int i;
    CHAR szClass[32];

    GetClassNameA(hwnd, szClass, ARRAYSIZE(szClass));
    for (i = 0; i < ARRAYSIZE(c_CtrlTbl); i++)
    {
        DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);

        if (!lstrcmpiA(szClass, c_CtrlTbl[i].szControl) && c_CtrlTbl[i].StyleCheckProc(dwStyle)
            && GetParent(hwnd) == (HWND)lParam)
        {
            LPCITEM lpCItem = (LPCITEM)LocalAlloc(LPTR, sizeof(CITEM));

            if (lpCItem)
            {
                lpCItem->dwStyle = dwStyle;
                GetClientRect(hwnd, &lpCItem->rc);
                SetWindowSubclass(hwnd, c_CtrlTbl[i].SubclassProc, 0, (DWORD_PTR)lpCItem);
                // Must add by name - Win95 compat
                SetProp(hwnd, c_szML, (HANDLE)1);

                // NOTE: need to ML_SETTEXT the original strings, otherwise the get
                // lost when user converts to ansi while init the dialog.  Not a problem
                // on nt because nt uses createwindoww.  On nt, we could GetWindowTextW
                // and do a ML_SETTEXT.  Fortunately our caller does this for us, it seems.
            }
            break;
        }
    }
    return TRUE;
}

void SetDlgControlText(HWND hDlg, LPDLGTEMPLATE pdtNew, LPCDLGTEMPLATE pdtOrg)
{
    LPBYTE pbNew, pbOrg;
    BOOL fEx;
    UINT cItems; 

    if (HIWORD(pdtNew->style) == 0xFFFF)
    {
        DLGTEMPLATEEX *pdtex = (DLGTEMPLATEEX *)pdtNew;
        fEx = TRUE;
        cItems = pdtex->cDlgItems;
    }
    else
    {
        fEx = FALSE;
        cItems = pdtNew->cdit;
    }

    // skip DLGTEMPLATE(EX) part
    pbNew = SkipDialogHeader(pdtNew);
    pbOrg = SkipDialogHeader(pdtOrg);

    while (cItems--)
    {
        int i;
        UINT cbCreateParams;
        LPDLGITEMTEMPLATE lpdit;
        LPDLGITEMTEMPLATEEX lpditex;
        DWORD dwStyle;

        if (fEx)
        {
            lpditex = (LPDLGITEMTEMPLATEEX)pbNew;
            dwStyle = lpditex->style;
            pbNew += sizeof(DLGITEMTEMPLATEEX);
            pbOrg += sizeof(DLGITEMTEMPLATEEX);
        }
        else
        {
            lpdit = (LPDLGITEMTEMPLATE)pbNew;
            dwStyle = lpdit->style;
            pbNew += sizeof(DLGITEMTEMPLATE);
            pbOrg += sizeof(DLGITEMTEMPLATE);
        }

        i = DoMungeControl(pbOrg, dwStyle);

        // Skip the dialog control class name.
        pbNew = SkipIDorString(pbNew);
        pbOrg = SkipIDorString(pbOrg);

        if (i < ARRAYSIZE(c_CtrlTbl))
        {
            int id = (fEx)? lpditex->id: lpdit->id;

            MLSetControlTextI(GetDlgItem(hDlg, id), (LPWSTR)pbOrg);
        }

        // Look at window text now.
        pbNew = SkipIDorString(pbNew);
        pbOrg = SkipIDorString(pbOrg);

        cbCreateParams = *((LPWORD)pbNew);
        // skip any CreateParams which include the generated size WORD.
        if (cbCreateParams)
        {
            pbNew += cbCreateParams;
            pbOrg += cbCreateParams;
        }
        pbNew += sizeof(WORD);
        pbOrg += sizeof(WORD);

        // Point at the next dialog item. (DWORD aligned)
        pbNew = (LPBYTE)(((ULONG_PTR)pbNew + 3) & ~3);
        pbOrg = (LPBYTE)(((ULONG_PTR)pbOrg + 3) & ~3);
    }

    return;
}

typedef struct tagFontFace
{
    BOOL fBitCmp;
    LPCWSTR lpEnglish;
    LPCWSTR lpNative;
} FONTFACE, *LPFONTFACE;

//
// Because StrCmpIW(lstrcmpiW) converts unicode string to ansi depends on user locale
// on Win9x platform, we can't compare two different locale's unicode string properly.
// This is why we use small private helper function to compare limited DBCS font facename
//
BOOL CompareFontFaceW(LPCWSTR lpwz1, LPCWSTR lpwz2, BOOL fBitCmp)
{
    BOOL fRet;  // Return FALSE if strings are same, otherwise return TRUE

    if (g_bRunningOnNT)
        return StrCmpIW(lpwz1, lpwz2);

    if (fBitCmp)
    {
        int iLen1, iLen2;

        fRet = TRUE;
        iLen1 = lstrlenW(lpwz1);
        iLen2 = lstrlenW(lpwz2);
        if (iLen1 == iLen2)
        {
            int i;

            for (i = 0; i < iLen1; i++)
            {
                if (lpwz1[i] != lpwz2[i])
                    break;
            }

            if (i >= iLen1)
                fRet = FALSE;
        }
    }
    else
        fRet = StrCmpIW(lpwz1, lpwz2);

    return fRet;
}

void ReplaceFontFace(LPBYTE *ppbDest, LPBYTE *ppbSrc)
{
    static FONTFACE s_FontTbl[] = 
    {
        {   FALSE, L"MS Gothic", L"MS UI Gothic"                                   },
        {   TRUE,  L"MS Gothic", L"\xff2d\xff33 \xff30\x30b4\x30b7\x30c3\x30af"    },
        {   TRUE,  L"GulimChe",  L"\xad74\xb9bc"                                   },
        {   TRUE,  L"MS Song",   L"\x5b8b\x4f53"                                   },
        {   TRUE,  L"MingLiU",   L"\x65b0\x7d30\x660e\x9ad4"                       }
    };
    int i;

    for (i = 0; i < ARRAYSIZE(s_FontTbl); i++)
    {
        if (!CompareFontFaceW((LPWSTR)*ppbSrc, s_FontTbl[i].lpNative, s_FontTbl[i].fBitCmp))
        {
            // Do replacement
            StrCpyW((LPWSTR)*ppbDest, s_FontTbl[i].lpEnglish);
            *ppbSrc += (lstrlenW((LPWSTR)*ppbSrc) + 1) * sizeof(WCHAR);
            *ppbDest += (lstrlenW(s_FontTbl[i].lpEnglish) + 1) * sizeof(WCHAR);
            break;
        }
    }
    
    if (i >= ARRAYSIZE(s_FontTbl))
        CopyIDorString(ppbDest, ppbSrc);

    return;
}

LPDLGTEMPLATE MungeDialogTemplate(LPCDLGTEMPLATE pdtSrc)
{
    DWORD dwSize;
    LPDLGTEMPLATE pdtDest;

    dwSize = GetSizeOfDialogTemplate(pdtSrc);
    if (pdtDest = (LPDLGTEMPLATE)LocalAlloc(LPTR, dwSize * 2))
    {
        LPBYTE pbSrc = (LPBYTE)pdtSrc, pbDest = (LPBYTE)pdtDest;
        LPBYTE pbItemSrc, pbItemDest;
        UINT cItemsSrc, cItems; 
        LPDLGTEMPLATEEX pdtex;
        BOOL fEx;

        if (HIWORD(pdtSrc->style) == 0xFFFF)
        {
            pdtex = (DLGTEMPLATEEX *)pdtSrc;
            fEx = TRUE;
            cItems = pdtex->cDlgItems;
            memcpy(pdtDest, pdtSrc, sizeof(DLGTEMPLATEEX));
            pbSrc = (LPBYTE)(((LPDLGTEMPLATEEX)pdtSrc) + 1);
            pbDest = (LPBYTE)(((LPDLGTEMPLATEEX)pdtDest) + 1);
        }
        else
        {
            fEx = FALSE;
            cItems = pdtSrc->cdit;
            memcpy(pdtDest, pdtSrc, sizeof(DLGTEMPLATE));
            pbSrc = (LPBYTE)(pdtSrc + 1);
            pbDest = (LPBYTE)(pdtDest + 1);
        }

        CopyIDorString(&pbDest, &pbSrc);    // menu
        CopyIDorString(&pbDest, &pbSrc);    // class
        StripIDorString(&pbDest, &pbSrc);   // window text

        // font type, size and name
        if ((fEx ? pdtex->dwStyle : pdtSrc->style) & DS_SETFONT)
        {
            if (fEx)
            {
                memcpy(pbDest, pbSrc, sizeof(DWORD) + sizeof(WORD));
                pbSrc += sizeof(DWORD) + sizeof(WORD);
                pbDest += sizeof(DWORD) + sizeof(WORD);
            }
            else
            {
                memcpy(pbDest, pbSrc, sizeof(WORD));
                pbSrc += sizeof(WORD);
                pbDest += sizeof(WORD);
            }
            ReplaceFontFace(&pbDest, &pbSrc);
        }
        pbSrc = (LPBYTE)(((ULONG_PTR)pbSrc + 3) & ~3);      // DWORD align
        pbDest = (LPBYTE)(((ULONG_PTR)pbDest + 3) & ~3);    // DWORD align

        // keep items information
        cItemsSrc = cItems;
        pbItemSrc = pbSrc;
        pbItemDest = pbDest;

        while (cItems--)
        {
            int i;
            UINT cbCreateParams;
            LPDLGITEMTEMPLATE lpditSrc, lpditDest;
            LPDLGITEMTEMPLATEEX lpditexSrc, lpditexDest;
            DWORD dwStyle;

            if (fEx)
            {
                lpditexSrc = (LPDLGITEMTEMPLATEEX)pbSrc;
                lpditexDest = (LPDLGITEMTEMPLATEEX)pbDest;

                memcpy(pbDest, pbSrc, sizeof(DLGITEMTEMPLATEEX));
                dwStyle = lpditexSrc->style;

                pbSrc += sizeof(DLGITEMTEMPLATEEX);
                pbDest += sizeof(DLGITEMTEMPLATEEX);
            }
            else
            {
                lpditSrc = (LPDLGITEMTEMPLATE)pbSrc;
                lpditDest = (LPDLGITEMTEMPLATE)pbDest;

                memcpy(pbDest, pbSrc, sizeof(DLGITEMTEMPLATE));
                dwStyle = lpditSrc->style;

                pbSrc += sizeof(DLGITEMTEMPLATE);
                pbDest += sizeof(DLGITEMTEMPLATE);
            }

            i = DoMungeControl(pbSrc, dwStyle);

            CopyIDorString(&pbDest, &pbSrc);

            if (i < ARRAYSIZE(c_CtrlTbl))
            {
                int id = (fEx)? lpditexDest->id: lpditDest->id;

                if (!CheckID(pbItemSrc, cItemsSrc, fEx, id, 1))
                    id = GetUniqueID(pbItemSrc, cItemsSrc, pbItemDest, cItemsSrc - cItems, fEx);
                if (fEx)
                    lpditexDest->id = id;
                else
                    lpditDest->id = (USHORT)id;
                StripIDorString(&pbDest, &pbSrc);
            }
            else
                CopyIDorString(&pbDest, &pbSrc);

            cbCreateParams = *((LPWORD)pbSrc);

            // copy any CreateParams which include the generated size WORD.
            if (cbCreateParams)
            {
                memcpy(pbDest, pbSrc, cbCreateParams);
                pbSrc += cbCreateParams;
                pbDest += cbCreateParams;
            }
            pbSrc += sizeof(WORD);
            pbDest += sizeof(WORD);

            // Point at the next dialog item. (DWORD aligned)
            pbSrc = (LPBYTE)(((ULONG_PTR)pbSrc + 3) & ~3);      // DWORD align
            pbDest = (LPBYTE)(((ULONG_PTR)pbDest + 3) & ~3);    // DWORD align
        }
    }
    return pdtDest;
}

// BEGIN Remove these soon
LWSTDAPI_(BOOL) EndDialogWrap(HWND hDlg, INT_PTR nResult)
{
    VALIDATE_PROTOTYPE_NO_W(EndDialog);

    return EndDialog(hDlg, nResult);
}
// END Remove these soon

BOOL fDoMungeUI(HINSTANCE hinst)
{
    LANGID lidUI = MLGetUILanguage();
    BOOL fMunged = FALSE;

    // We don't do our plugui on NT5...
    if (!g_bRunningOnNT5OrHigher)
    {
        // We don't need to munge if UI language is same as install language - perf.
        if (lidUI && lidUI != GetInstallLanguage())
        {
            ENTERCRITICAL;
            int i = GetPUIITEM(hinst);

            if (0 <= i)
            {
                PPUIITEM pItem = (PPUIITEM)DPA_FastGetPtr(g_hdpaPUI, i);
                if (pItem)
                    fMunged = pItem->fMunged;
            }
            LEAVECRITICAL;
        }
    }
    return fMunged;
}

typedef struct tagMLDLGPROCPARAM
{
    LPARAM dwInitParam;
    LPDLGTEMPLATE lpNewTemplate;
    LPCDLGTEMPLATE lpOrgTemplate;
    DLGPROC DlgProc;
} MLDLGPROCPARAM, *LPMLDLGPROCPARAM;

BOOL_PTR MLDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            LPMLDLGPROCPARAM lpDlgProcParam = (LPMLDLGPROCPARAM)lParam;

            // Bump the refcount on the global atom just to make sure
            if (g_atmML == 0) {
                g_atmML = GlobalAddAtom(c_szML);
            }

            // subclass control windows
            EnumChildWindows(hwnd, EnumChildProc, (LPARAM)hwnd);
            SetDlgControlText(hwnd, lpDlgProcParam->lpNewTemplate, lpDlgProcParam->lpOrgTemplate);
            if (g_bRunningOnNT)
                SetWindowLongPtrW(hwnd, DWLP_DLGPROC, (LPARAM)lpDlgProcParam->DlgProc);
            else
                SetWindowLongPtrA(hwnd, DWLP_DLGPROC, (LPARAM)lpDlgProcParam->DlgProc);
            return SendMessageWrapW(hwnd, uMsg, wParam, lpDlgProcParam->dwInitParam);
        }
    }
    return FALSE;
}

#undef DialogBoxIndirectParamW

INT_PTR MLDialogBoxIndirectParamI(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hwndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
    LPDLGTEMPLATE p;
    INT_PTR iRet = -1;
    
    ASSERT(fDoMungeUI(hInstance));

    if (p = MungeDialogTemplate(lpTemplate))
    {
        MLDLGPROCPARAM MLDlgProcParam;

        MLDlgProcParam.dwInitParam = dwInitParam;
        MLDlgProcParam.lpNewTemplate = p;
        MLDlgProcParam.lpOrgTemplate = lpTemplate;
        MLDlgProcParam.DlgProc = lpDialogFunc;

        if (g_bRunningOnNT)
            iRet = DialogBoxIndirectParamW(hInstance, p, hwndParent, MLDialogProc, (LPARAM)&MLDlgProcParam);
        else
            iRet = DialogBoxIndirectParamA(hInstance, p, hwndParent, MLDialogProc, (LPARAM)&MLDlgProcParam);
        LocalFree(p);
    }
    return iRet;
}

INT_PTR MLDialogBoxParamI(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hwndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
    HRSRC hrsr;
    HGLOBAL h;
    LPDLGTEMPLATE p;
    INT_PTR iRet = -1;

    if (hrsr = FindResourceWrapW(hInstance, lpTemplateName, (LPCWSTR)RT_DIALOG))
    {
        if (h = LoadResource(hInstance, hrsr))
        {
            if (p = (LPDLGTEMPLATE)LockResource(h))
                iRet = MLDialogBoxIndirectParamI(hInstance, p, hwndParent, lpDialogFunc, dwInitParam);
        }
    }
    return iRet;
}

#undef CreateDialogIndirectParamW

HWND MLCreateDialogIndirectParamI(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hwndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
    LPDLGTEMPLATE p;
    HWND hwndRet = NULL;

    ASSERT(fDoMungeUI(hInstance));

    if (p = MungeDialogTemplate(lpTemplate))
    {
        MLDLGPROCPARAM MLDlgProcParam;

        MLDlgProcParam.dwInitParam = dwInitParam;
        MLDlgProcParam.lpNewTemplate = p;
        MLDlgProcParam.lpOrgTemplate = lpTemplate;
        MLDlgProcParam.DlgProc = lpDialogFunc;

        if (g_bRunningOnNT)
            hwndRet = CreateDialogIndirectParamW(hInstance, p, hwndParent, MLDialogProc, (LPARAM)&MLDlgProcParam);
        else
            hwndRet = CreateDialogIndirectParamA(hInstance, p, hwndParent, MLDialogProc, (LPARAM)&MLDlgProcParam);
        LocalFree(p);
    }
    return hwndRet;
}

HWND MLCreateDialogParamI(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hwndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
    HRSRC hrsr;
    HGLOBAL h;
    LPDLGTEMPLATE p;
    HWND hwndRet = NULL;

    if (hrsr = FindResourceWrapW(hInstance, lpTemplateName, (LPCWSTR)RT_DIALOG))
    {
        if (h = LoadResource(hInstance, hrsr))
        {
            if (p = (LPDLGTEMPLATE)LockResource(h))
                hwndRet = MLCreateDialogIndirectParamI(hInstance, p, hwndParent, lpDialogFunc, dwInitParam);
        }
    }
    return hwndRet;
}

BOOL MLIsEnabled(HWND hwnd)
{
#ifndef UNIX
    if (hwnd && g_atmML)
        return (BOOL)PtrToLong(GetProp(hwnd, MAKEINTATOM(g_atmML)));
#endif
    return FALSE;
}

int MLGetControlTextI(HWND hWnd, LPCWSTR lpString, int nMaxCount)
{
    ASSERT(MLIsEnabled(hWnd));
    ASSERT(g_ML_GETTEXT);

    if (lpString && nMaxCount > 0)
    {
        return (int)SendMessage(hWnd, g_ML_GETTEXT, nMaxCount, (LPARAM)lpString);
    }
    else
        return 0;
}

BOOL MLSetControlTextI(HWND hWnd, LPCWSTR lpString)
{
    ASSERT(MLIsEnabled(hWnd));
    ASSERT(g_ML_SETTEXT);

    if (lpString)
    {
        return (BOOL)SendMessage(hWnd, g_ML_SETTEXT, 0, (LPARAM)lpString);
    }
    else
        return FALSE;
}

#define MAXRCSTRING 258

// this will check to see if lpcstr is a resource id or not.  if it
// is, it will return a LPSTR containing the loaded resource.
// the caller must LocalFree this lpstr.  if pszText IS a string, it
// will return pszText
//
// returns:
//      pszText if it is already a string
//      or
//      LocalAlloced() memory to be freed with LocalFree
//      if pszRet != pszText free pszRet

LPWSTR ResourceCStrToStr(HINSTANCE hInst, LPCWSTR pszText)
{
    WCHAR szTemp[MAXRCSTRING];
    LPWSTR pszRet = NULL;

    if (!IS_INTRESOURCE(pszText))
        return (LPWSTR)pszText;

    if (LOWORD((DWORD_PTR)pszText) && LoadStringWrapW(hInst, LOWORD((DWORD_PTR)pszText), szTemp, ARRAYSIZE(szTemp)))
    {
        pszRet = (LPWSTR)LocalAlloc(LPTR, (lstrlenW(szTemp) + 1) * SIZEOF(WCHAR));
        if (pszRet)
            StrCpyW(pszRet, szTemp);
    }
    return pszRet;
}

LPWSTR _ConstructMessageString(HINSTANCE hInst, LPCWSTR pszMsg, va_list *ArgList)
{
    LPWSTR pszRet;
    LPWSTR pszRes = ResourceCStrToStr(hInst, pszMsg);
    if (!pszRes)
    {
        DebugMsg(DM_ERROR, TEXT("_ConstructMessageString: Failed to load string template"));
        return NULL;
    }

    if (!FormatMessageWrapW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                       pszRes, 0, 0, (LPWSTR)&pszRet, 0, ArgList))
    {
        DebugMsg(DM_ERROR, TEXT("_ConstructMessageString: FormatMessage failed %d"),GetLastError());
        DebugMsg(DM_ERROR, TEXT("                         pszRes = %s"), pszRes );
        DebugMsg(DM_ERROR, !IS_INTRESOURCE(pszMsg) ? 
            TEXT("                         pszMsg = %s") : 
            TEXT("                         pszMsg = 0x%x"), pszMsg );
        pszRet = NULL;
    }


    if (pszRes != pszMsg)
        LocalFree(pszRes);

    return pszRet;      // free with LocalFree()
}

LWSTDAPIV_(int) ShellMessageBoxWrapW(HINSTANCE hInst, HWND hWnd, LPCWSTR pszMsg, LPCWSTR pszTitle, UINT fuStyle, ...)
{
    LPWSTR pszText;
    int result;
    WCHAR szBuffer[80];
    va_list ArgList;

    if (!IS_INTRESOURCE(pszTitle))
    {
        // do nothing
    }
    else if (LoadStringWrapW(hInst, LOWORD((DWORD_PTR)pszTitle), szBuffer, ARRAYSIZE(szBuffer)))
    {
        // Allow this to be a resource ID or NULL to specifiy the parent's title
        pszTitle = szBuffer;
    }
    else if (hWnd)
    {
        // The caller didn't give us a Title, so let's use the Window Text.

        // Grab the title of the parent
        GetWindowTextWrapW(hWnd, szBuffer, ARRAYSIZE(szBuffer));

        // HACKHACK YUCK!!!!
        // Is the window the Desktop window?
        if (!StrCmpW(szBuffer, L"Program Manager"))
        {
            // Yes, so we now have two problems,
            // 1. The title should be "Desktop" and not "Program Manager", and
            // 2. Only the desktop thread can call this or it will hang the desktop
            //    window.

            // Is the window Prop valid?
            if (GetWindowThreadProcessId(hWnd, 0) == GetCurrentThreadId())
            {
                // Yes, so let's get it...

                // Problem #1, load a localized version of "Desktop"
                pszTitle = (LPCWSTR) GetProp(hWnd, TEXT("pszDesktopTitleW"));

                if (!pszTitle)
                {
                    // Oops, this must have been some app with "Program Manager" as the title.
                    pszTitle = szBuffer;
                }
            }
            else
            {
                // No, so we hit problem 2...

                // Problem #2, Someone is going to
                //             hang the desktop window by using it as the parent
                //             of a dialog that belongs to a thread other than
                //             the desktop thread.
                RIPMSG(0, "****************ERROR********** The caller is going to hang the desktop window by putting a modal dlg on it.");
            }
        }
        else
            pszTitle = szBuffer;
    }
    else
    {
        pszTitle = L"";
    }

    va_start(ArgList, fuStyle);
    pszText = _ConstructMessageString(hInst, pszMsg, &ArgList);
    va_end(ArgList);

    if (pszText)
    {
        result = MessageBoxWrapW(hWnd, pszText, pszTitle, fuStyle | MB_SETFOREGROUND);
        LocalFree(pszText);
    }
    else
    {
        DebugMsg(DM_ERROR, TEXT("smb: Not enough memory to put up dialog."));
        result = -1;    // memory failure
    }

    return result;
}

HRESULT GetFilePathFromLangId (LPCSTR pszFile, LPSTR pszOut, int cchOut, DWORD dwFlag)
{
    HRESULT hr = S_OK;
    char szMUIPath[MAX_PATH];
    LPCSTR lpPath;
    LANGID lidUI;
    
    if (pszFile)
    {
        // BUGBUG: should support '>' format but not now
        if (*pszFile == '>') return E_FAIL;

        lidUI = GetNormalizedLangId(dwFlag);
        if (0 == lidUI || GetInstallLanguage() == lidUI)
            lpPath = pszFile;
        else
        {
            GetMUIPathOfIEFileA(szMUIPath, ARRAYSIZE(szMUIPath), pszFile, lidUI);
            lpPath = (LPCSTR)szMUIPath;
        }
        lstrcpyn(pszOut, lpPath, min(MAX_PATH, cchOut));
    }
    else
        hr = E_FAIL;

    return hr;
}

// 
// MLHtmlHelp / MLWinHelp
//
// Function: load a help file corresponding to the current UI lang setting 
//           from \mui\<Lang ID>
//
//
#ifndef UNIX
HWND MLHtmlHelpA(HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData, DWORD dwCrossCodePage)
{

    CHAR szPath[MAX_PATH];
    HRESULT hr = E_FAIL;
    HWND hwnd;

    // BUGBUG: 1) At this moment we only support the cases that pszFile points to 
    //         a fully qualified file path, like when uCommand == HH_DISPLAY_TOPIC
    //         or uCommand == HH_DISPLAY_TEXT_POPUP. 
    //         2) We should support '>' format to deal with secondary window
    //         3) we may need to thunk file names within HH_WINTYPE structures?
    //
    if (uCommand == HH_DISPLAY_TOPIC || uCommand == HH_DISPLAY_TEXT_POPUP)
    {
        hr = GetFilePathFromLangId(pszFile, szPath, ARRAYSIZE(szPath), dwCrossCodePage);
        if (hr == S_OK)
            hwnd = HtmlHelp(hwndCaller, szPath, uCommand, dwData); 
    }

    // if there was any failure in getting ML path of help file
    // we call the help engine with original file path.
    if (hr != S_OK)
    {
        hwnd = HtmlHelp(hwndCaller, pszFile, uCommand, dwData); 
    }
    return hwnd;
}
#endif

BOOL MLWinHelpA(HWND hwndCaller, LPCSTR lpszHelp, UINT uCommand, DWORD_PTR dwData)
{

    CHAR szPath[MAX_PATH];
    BOOL fret;

    HRESULT hr = GetFilePathFromLangId(lpszHelp, szPath, ARRAYSIZE(szPath), ML_NO_CROSSCODEPAGE);
    if (hr == S_OK)
    {
        fret = WinHelp(hwndCaller, szPath, uCommand, dwData);
    }
    else
        fret = WinHelp(hwndCaller, lpszHelp, uCommand, dwData);

    return fret;
}

#ifndef UNIX
HWND MLHtmlHelpW(HWND hwndCaller, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData, DWORD dwCrossCodePage)
{
    HRESULT hr = E_FAIL;
    HWND hwnd;

    // BUGBUG: 1) At this moment we only support the cases that pszFile points to 
    //         a fully qualified file path, like when uCommand == HH_DISPLAY_TOPIC
    //         or uCommand == HH_DISPLAY_TEXT_POPUP. 
    //         2) We should support '>' format to deal with secondary window
    //         3) we may need to thunk file names within HH_WINTYPE structures?
    //
    if (uCommand == HH_DISPLAY_TOPIC || uCommand == HH_DISPLAY_TEXT_POPUP)
    {
        CHAR szFileName[MAX_PATH];
        LPCSTR pszFileParam = NULL;

        if (pszFile)
        {
            SHUnicodeToAnsi(pszFile, szFileName, ARRAYSIZE(szFileName));
            pszFileParam = szFileName;
        }

        hr = GetFilePathFromLangId(pszFileParam, szFileName, ARRAYSIZE(szFileName), dwCrossCodePage);
        if (hr == S_OK)
        {
            ASSERT(NULL != pszFileParam);   // GetFilePathFromLangId returns E_FAIL with NULL input
            
            WCHAR wszFileName[MAX_PATH];

            SHAnsiToUnicode(szFileName, wszFileName, ARRAYSIZE(wszFileName));
            hwnd = HtmlHelpW(hwndCaller, wszFileName, uCommand, dwData); 
        }
    }

    // if there was any failure in getting ML path of help file
    // we call the help engine with original file path.
    if (hr != S_OK)
    {
        hwnd = HtmlHelpW(hwndCaller, pszFile, uCommand, dwData); 
    }
    return hwnd;
}
#endif


BOOL MLWinHelpW(HWND hWndMain, LPCWSTR lpszHelp, UINT uCommand, DWORD_PTR dwData)
{
    CHAR szFileName[MAX_PATH];
    LPCSTR pszHelpParam = NULL;

    if (lpszHelp)
    {
        SHUnicodeToAnsi(lpszHelp, szFileName, ARRAYSIZE(szFileName));
        pszHelpParam = szFileName;
    }
    return  MLWinHelpA(hWndMain, pszHelpParam, uCommand, dwData);
}

//
//  Font link wrappers
//
int DrawTextFLW(HDC hdc, LPCWSTR lpString, int nCount, LPRECT lpRect, UINT uFormat)
{
    typedef int (* PFNDRAWTEXT)(HDC, LPCWSTR, int, LPRECT, UINT);
    static PFNDRAWTEXT pfnDrawTextW = NULL;

    if (NULL == pfnDrawTextW)
    {
        HMODULE hComctl32 = LoadLibrary("comctl32.dll");

        if (hComctl32)
            pfnDrawTextW = (PFNDRAWTEXT)GetProcAddress(hComctl32, (LPCSTR)415);
    }

    if (pfnDrawTextW)
        return pfnDrawTextW(hdc, lpString, nCount, lpRect, uFormat);

    return 0;
}

int DrawTextExFLW(HDC hdc, LPWSTR pwzText, int cchText, LPRECT lprc, UINT dwDTFormat, LPDRAWTEXTPARAMS lpDTParams)
{
    typedef int (* PFNDRAWTEXTEX)(HDC, LPWSTR, int, LPRECT, UINT, LPDRAWTEXTPARAMS);
    static PFNDRAWTEXTEX pfnDrawTextExW = NULL;

    if (NULL == pfnDrawTextExW)
    {
        HMODULE hComctl32 = LoadLibrary("comctl32.dll");

        if (hComctl32)
            pfnDrawTextExW = (PFNDRAWTEXTEX)GetProcAddress(hComctl32, (LPCSTR)416);
    }

    if (pfnDrawTextExW)
        return pfnDrawTextExW(hdc, pwzText, cchText, lprc, dwDTFormat, lpDTParams);

    return 0;
}

BOOL GetTextExtentPointFLW(HDC hdc, LPCWSTR lpString, int nCount, LPSIZE lpSize)
{
    typedef BOOL (* PFNGETTEXTEXTENTPOINT)(HDC, LPCWSTR, int, LPSIZE);
    static PFNGETTEXTEXTENTPOINT pfnGetTextExtentPointW = NULL;

    if (NULL == pfnGetTextExtentPointW)
    {
        HMODULE hComctl32 = LoadLibrary("comctl32.dll");

        if (hComctl32)
            pfnGetTextExtentPointW = (PFNGETTEXTEXTENTPOINT)GetProcAddress(hComctl32, (LPCSTR)419);
    }

    if (pfnGetTextExtentPointW)
        return pfnGetTextExtentPointW(hdc, lpString, nCount, lpSize);

    return FALSE;
}

int ExtTextOutFLW(HDC hdc, int xp, int yp, UINT eto, CONST RECT *lprect, LPCWSTR lpwch, UINT cLen, CONST INT *lpdxp)
{
    typedef int (* PFNEXTTEXTOUT)(HDC, int, int, UINT, CONST RECT*, LPCWSTR, UINT, CONST INT*);
    static PFNEXTTEXTOUT pfnExtTextOutW = NULL;

    if (NULL == pfnExtTextOutW)
    {
        HMODULE hComctl32 = LoadLibrary("comctl32.dll");

        if (hComctl32)
            pfnExtTextOutW = (PFNEXTTEXTOUT)GetProcAddress(hComctl32, (LPCSTR)417);
    }

    if (pfnExtTextOutW)
        return pfnExtTextOutW(hdc, xp, yp, eto, lprect, lpwch, cLen, lpdxp);

    return 0;
}

const WCHAR c_szResPrefix[] = L"res://";

LWSTDAPI
MLBuildResURLW(LPCWSTR  pszLibFile,
               HMODULE  hModule,
               DWORD    dwCrossCodePage,
               LPCWSTR  pszResName,
               LPWSTR   pszResUrlOut,
               int      cchResUrlOut)
{
    HRESULT hr;
    LPWSTR  pszWrite;
    int     cchBufRemaining;
    int     cchWrite;

    RIP(IS_VALID_STRING_PTRW(pszLibFile, -1));
    RIP(hModule != INVALID_HANDLE_VALUE);
    RIP(hModule != NULL);
    RIP(IS_VALID_STRING_PTRW(pszResName, -1));
    RIP(IS_VALID_WRITE_BUFFER(pszResUrlOut, WCHAR, cchResUrlOut));

    hr = E_INVALIDARG;

    if (pszLibFile != NULL &&
        hModule != NULL &&
        hModule != INVALID_HANDLE_VALUE &&
        (dwCrossCodePage == ML_CROSSCODEPAGE || dwCrossCodePage == ML_NO_CROSSCODEPAGE) &&
        pszResName != NULL &&
        pszResUrlOut != NULL)
    {
        hr = E_FAIL;

        pszWrite = pszResUrlOut;
        cchBufRemaining = cchResUrlOut;

        // write in the res protocol prefix
        cchWrite = wcslen(c_szResPrefix);
        if (cchBufRemaining >= cchWrite+1)
        {
            HINSTANCE   hinstLocRes;

            StrCpyNW(pszWrite, c_szResPrefix, cchBufRemaining);
            pszWrite += cchWrite;
            cchBufRemaining -= cchWrite;

            // figure out the module path
            // unfortunately the module path might only exist
            // after necessary components are JIT'd, and
            // we don't know whether a JIT is necessary unless
            // certain LoadLibrary's have failed.
            hinstLocRes = MLLoadLibraryW(pszLibFile, hModule, dwCrossCodePage);
            if (hinstLocRes != NULL)
            {
                BOOL    fGotModulePath;
                WCHAR   szLocResPath[MAX_PATH];

                fGotModulePath = GetModuleFileNameWrapW(hinstLocRes, szLocResPath, ARRAYSIZE(szLocResPath));

                MLFreeLibrary(hinstLocRes);

                if (fGotModulePath)
                {
                    // copy in the module path
                    cchWrite = wcslen(szLocResPath);
                    if (cchBufRemaining >= cchWrite+1)
                    {
                        StrCpyNW(pszWrite, szLocResPath, cchBufRemaining);
                        pszWrite += cchWrite;
                        cchBufRemaining -= cchWrite;

                        // write the next L'/' and the resource name
                        cchWrite = 1 + wcslen(pszResName);
                        if (cchBufRemaining >= cchWrite+1)
                        {
                            *(pszWrite++) = L'/';
                            cchBufRemaining--;
                            StrCpyNW(pszWrite, pszResName, cchBufRemaining);

                            ASSERT(pszWrite[wcslen(pszResName)] == '\0');

                            hr = S_OK;
                        }
                    }
                }
            }
        }

        if (FAILED(hr))
        {
            pszResUrlOut[0] = L'\0';
        }
    }

    return hr;
}

LWSTDAPI
MLBuildResURLA(LPCSTR    pszLibFile,
               HMODULE  hModule,
               DWORD    dwCrossCodePage,
               LPCSTR   pszResName,
               LPSTR   pszResUrlOut,
               int      cchResUrlOut)
{
    HRESULT hr;

    RIP(IS_VALID_STRING_PTR(pszLibFile, -1));
    RIP(hModule != INVALID_HANDLE_VALUE);
    RIP(hModule != NULL);
    RIP(IS_VALID_STRING_PTRA(pszResName, -1));
    RIP(IS_VALID_WRITE_BUFFER(pszResUrlOut, CHAR, cchResUrlOut));

    CStrInW     strLF(pszLibFile);
    CStrInW     strRN(pszResName);
    CStrOutW    strRUO(pszResUrlOut, cchResUrlOut);

    hr = MLBuildResURLW(strLF, hModule, dwCrossCodePage, strRN, strRUO, strRUO.BufSize());

    return hr;
}
