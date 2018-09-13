/**************************************************************************\
* Module Name: layout.c (corresponds to Win95 ime.c)
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* IME Keyboard Layout related functionality
*
* History:
* 03-Jan-1996 wkwok       Created
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * Local Defines.
 */
#define szLZOpenFileW "LZOpenFileW"
#define szLZCopy      "LZCopy"
#define szLZClose     "LZClose"

typedef HFILE (WINAPI *LPFNLZOPENFILEW)(LPTSTR, LPOFSTRUCT, WORD);
typedef LONG  (WINAPI *LPFNLZCOPY)(INT, INT);
typedef VOID  (WINAPI *LPFNLZCLOSE)(INT);

/*
 * Local Routines.
 */
UINT StrToUInt(LPWSTR);
VOID UIntToStr(UINT, ULONG, LPWSTR, USHORT);
BOOL CopyImeFile(LPWSTR, LPCWSTR);
INT  GetImeLayout(PIMELAYOUT, INT);
BOOL WriteImeLayout(HKL, LPCWSTR, LPCWSTR);
HKL  AssignNewLayout(INT, PIMELAYOUT, HKL);


/***************************************************************************\
* ImmGetIMEFileNameW
*
* Gets the description of the IME with the specified HKL.
*
* History:
* 28-Feb-1995   wkwok   Created
\***************************************************************************/

UINT WINAPI ImmGetDescriptionW(
    HKL    hKL,
    LPWSTR lpwszDescription,
    UINT   uBufLen)
{
    IMEINFOEX iiex;
    UINT uRet;

    if (!ImmGetImeInfoEx(&iiex, ImeInfoExKeyboardLayout, &hKL))
        return 0;

    uRet = wcslen(iiex.wszImeDescription);

    /*
     * ask buffer length
     */
    if (uBufLen == 0)
        return uRet;

    if (uBufLen > uRet) {
        wcscpy(lpwszDescription, iiex.wszImeDescription);
    }
    else {
        uRet = uBufLen - 1;
        wcsncpy(lpwszDescription, iiex.wszImeDescription, uRet);
        lpwszDescription[uRet] = L'\0';
    }

    return uRet;
}


/***************************************************************************\
* ImmGetIMEFileNameA
*
* Gets the description of the IME with the specified HKL.
*
* History:
* 28-Feb-1995   wkwok   Created
\***************************************************************************/

UINT WINAPI ImmGetDescriptionA(
    HKL   hKL,
    LPSTR lpszDescription,
    UINT  uBufLen)
{
    IMEINFOEX iiex;
    INT       i;
    BOOL      bUDC;

    if (!ImmGetImeInfoEx(&iiex, ImeInfoExKeyboardLayout, &hKL))
        return 0;

    i = WideCharToMultiByte(CP_ACP,
                            (DWORD)0,
                            (LPWSTR)iiex.wszImeDescription,       // src
                            wcslen(iiex.wszImeDescription),
                            lpszDescription,                      // dest
                            uBufLen,
                            (LPSTR)NULL,
                            (LPBOOL)&bUDC);

    if (uBufLen != 0)
        lpszDescription[i] = '\0';

    return (UINT)i;
}


/***************************************************************************\
* ImmGetIMEFileNameW
*
* Gets the file name of the IME with the specified HKL.
*
* History:
* 28-Feb-1995   wkwok   Created
\***************************************************************************/

UINT WINAPI ImmGetIMEFileNameW(
    HKL    hKL,
    LPWSTR lpwszFile,
    UINT   uBufLen)
{
    IMEINFOEX iiex;
    UINT uRet;

    if (!ImmGetImeInfoEx(&iiex, ImeInfoExKeyboardLayout, &hKL))
        return 0;

    uRet = wcslen(iiex.wszImeFile);

    /*
     * ask buffer length
     */
    if (uBufLen == 0)
        return uRet;

    if (uBufLen > uRet) {
        wcscpy(lpwszFile, iiex.wszImeFile);
    }
    else {
        uRet = uBufLen - 1;
        wcsncpy(lpwszFile, iiex.wszImeFile, uRet);
        lpwszFile[uRet] = L'\0';
    }

    return uRet;
}


/***************************************************************************\
* ImmGetIMEFileNameA
*
* Gets the file name of the IME with the specified HKL.
*
* History:
* 28-Feb-1995   wkwok   Created
\***************************************************************************/

UINT WINAPI ImmGetIMEFileNameA(
    HKL   hKL,
    LPSTR lpszFile,
    UINT  uBufLen)
{
    IMEINFOEX iiex;
    INT       i;
    BOOL      bUDC;

    if (!ImmGetImeInfoEx(&iiex, ImeInfoExKeyboardLayout, &hKL))
        return 0;

    i = WideCharToMultiByte(CP_ACP,
                            (DWORD)0,
                            (LPWSTR)iiex.wszImeFile,       // src
                            wcslen(iiex.wszImeFile),
                            lpszFile,                      // dest
                            uBufLen,
                            (LPSTR)NULL,
                            (LPBOOL)&bUDC);

    if (uBufLen != 0)
        lpszFile[i] = '\0';

    return i;
}


/***************************************************************************\
* ImmGetProperty
*
* Gets the property and capability of the IME with the specified HKL.
*
* History:
* 28-Feb-1995   wkwok   Created
\***************************************************************************/

DWORD WINAPI ImmGetProperty(
    HKL     hKL,
    DWORD   dwIndex)
{
    IMEINFOEX iiex;
    PIMEDPI   pImeDpi = NULL;
    PIMEINFO  pImeInfo;
    DWORD     dwRet;

    if (!ImmGetImeInfoEx(&iiex, ImeInfoExKeyboardLayout, &hKL))
        return 0;

    if (dwIndex == IGP_GETIMEVERSION)
        return iiex.dwImeWinVersion;

    if (iiex.fLoadFlag != IMEF_LOADED) {
        pImeDpi = FindOrLoadImeDpi(hKL);
        if (pImeDpi == NULL) {
            RIPMSG0(RIP_WARNING, "ImmGetProperty: load IME failure.");
            return 0;
        }
        pImeInfo = &pImeDpi->ImeInfo;
    }
    else {
        pImeInfo = &iiex.ImeInfo;
    }

    switch (dwIndex) {
    case IGP_PROPERTY:
        dwRet = pImeInfo->fdwProperty;
        break;

    case IGP_CONVERSION:
        dwRet = pImeInfo->fdwConversionCaps;
        break;

    case IGP_SENTENCE:
        dwRet = pImeInfo->fdwSentenceCaps;
        break;

    case IGP_UI:
        dwRet = pImeInfo->fdwUICaps;
        break;

    case IGP_SETCOMPSTR:
        dwRet = pImeInfo->fdwSCSCaps;
        break;

    case IGP_SELECT:
        dwRet = pImeInfo->fdwSelectCaps;
        break;

    default:
        RIPMSG1(RIP_WARNING, "ImmGetProperty: wrong index %lx.", dwIndex);
        dwRet = 0;
        break;
    }

    ImmUnlockImeDpi(pImeDpi);

    return dwRet;
}


HKL WINAPI ImmInstallIMEW(
    LPCWSTR lpszIMEFileName,
    LPCWSTR lpszLayoutText)
{
    LPWSTR     lpwszImeFileName;
    LPWSTR     lpwszImeFilePart;
    LPWSTR     lpwszImeCopiedPath;
    int        i, nIMEs;
    PIMELAYOUT pImeLayout = NULL;
    HKL        hImeKL, hLangKL;
    WCHAR      szKeyName[HEX_ASCII_SIZE];
    IMEINFOEX  iiex;

    lpwszImeFileName = ImmLocalAlloc(0, (MAX_PATH+1) * sizeof(WCHAR));
    if (lpwszImeFileName == NULL)
        return (HKL)0;

    lpwszImeCopiedPath = ImmLocalAlloc(0, (MAX_PATH+1) * sizeof(WCHAR));
    if (lpwszImeCopiedPath == NULL) {
        ImmLocalFree(lpwszImeFileName);
        return (HKL)0;
    }

    /*
     * Get the file name only into lpwszImeFilePart
     */
    GetFullPathNameW(lpszIMEFileName, MAX_PATH,
                lpwszImeFileName, &lpwszImeFilePart);

    CharUpper(lpwszImeFileName);

    if (lpwszImeFilePart == NULL) {
        ImmLocalFree(lpwszImeFileName);
        ImmLocalFree(lpwszImeCopiedPath);
        return (HKL)0;
    }

    hImeKL = hLangKL = iiex.hkl = (HKL)0;

    wcsncpy(iiex.wszImeFile, lpwszImeFilePart, IM_FILE_SIZE-1);
    iiex.wszImeFile[IM_FILE_SIZE - 1] = L'\0';

    if (LoadVersionInfo(&iiex) && iiex.hkl != (HKL)0) {
        hLangKL = iiex.hkl;
    }
    else {
        ImmLocalFree(lpwszImeFileName);
        ImmLocalFree(lpwszImeCopiedPath);
        return (HKL)0;
    }

    nIMEs = GetImeLayout(NULL, 0);
    if (nIMEs != 0) {
        pImeLayout = (PIMELAYOUT)ImmLocalAlloc(0, nIMEs * sizeof(IMELAYOUT));
        if (pImeLayout == NULL) {
            ImmLocalFree(lpwszImeFileName);
            ImmLocalFree(lpwszImeCopiedPath);
            return (HKL)0;
        }

        GetImeLayout(pImeLayout, nIMEs);

        for (i=0; i < nIMEs; i++) {
            if (_wcsicmp(pImeLayout[i].szImeName, lpwszImeFilePart) == 0) {
                /*
                 * We got the same IME name, ISV wants to upgrade.
                 */
                if (LOWORD(HandleToUlong(hLangKL)) != LOWORD(HandleToUlong(pImeLayout[i].hImeKL))) {
                    /*
                     * IME name conflict, blow out!
                     */
                    RIPMSG0(RIP_WARNING, "ImmInstallIME: different language!");
                    goto ImmInstallIMEWFailed;
                }

                hImeKL = pImeLayout[i].hImeKL;
                break;
            }
        }
    }

    if (ImmGetImeInfoEx(&iiex, ImeInfoExImeFileName, lpwszImeFilePart)) {
        /*
         * The specified IME has been activated. Unload it first.
         */
        if (!UnloadKeyboardLayout(iiex.hkl)) {
            hImeKL = (HKL)0;
            goto ImmInstallIMEWFailed;
        }
    }

    /*
     * We will copy to system directory
     */
#if 0
    i = (INT)GetSystemDirectory(lpwszImeCopiedPath, MAX_PATH);
    lpwszImeCopiedPath[i] = L'\0';
    AddBackslash(lpwszImeCopiedPath);
    wcscat(lpwszImeCopiedPath, lpwszImeFilePart);
#else
    GetSystemPathName(lpwszImeCopiedPath, lpwszImeFilePart, MAX_PATH);
#endif
    CharUpper(lpwszImeCopiedPath);

    if (_wcsicmp(lpwszImeFileName, lpwszImeCopiedPath) != 0) {
        /*
         * path is different, need to copy into system directory
         */
        if (!CopyImeFile(lpwszImeFileName, lpwszImeCopiedPath)) {
            hImeKL = (HKL)0;
            goto ImmInstallIMEWFailed;
        }
    }

    if (hImeKL == 0) {
        hImeKL = AssignNewLayout(nIMEs, pImeLayout, hLangKL);
    }

    if (hImeKL != 0) {
        /*
         * Write HKL under "keyboard layouts"
         */
        if (WriteImeLayout(hImeKL, lpwszImeFilePart, lpszLayoutText)) {
            UIntToStr(HandleToUlong(hImeKL), 16, szKeyName, sizeof(szKeyName));
            hImeKL = LoadKeyboardLayout(szKeyName, KLF_REPLACELANG);
        }
        else {
            hImeKL = (HKL)0;
        }
    }

ImmInstallIMEWFailed:
    if (pImeLayout != NULL)
        ImmLocalFree(pImeLayout);
    ImmLocalFree(lpwszImeFileName);
    ImmLocalFree(lpwszImeCopiedPath);

    return (HKL)hImeKL;
}


HKL WINAPI ImmInstallIMEA(
    LPCSTR lpszIMEFileName,
    LPCSTR lpszLayoutText)
{
    HKL    hKL;
    LPWSTR lpwszIMEFileName;
    LPWSTR lpwszLayoutText;
    DWORD  cbIMEFileName;
    DWORD  cbLayoutText;
    INT    i;

    cbIMEFileName = strlen(lpszIMEFileName) + sizeof(CHAR);
    cbLayoutText  = strlen(lpszLayoutText)  + sizeof(CHAR);

    lpwszIMEFileName = ImmLocalAlloc(0, cbIMEFileName * sizeof(WCHAR));
    if (lpwszIMEFileName == NULL) {
        RIPMSG0(RIP_WARNING, "ImmInstallIMEA: memory failure!");
        return (HKL)0;
    }

    lpwszLayoutText = ImmLocalAlloc(0, cbLayoutText * sizeof(WCHAR));
    if (lpwszLayoutText == NULL) {
        RIPMSG0(RIP_WARNING, "ImmInstallIMEA: memory failure!");
        ImmLocalFree(lpwszIMEFileName);
        return (HKL)0;
    }

    i = MultiByteToWideChar(CP_ACP,
                            (DWORD)MB_PRECOMPOSED,
                            (LPSTR)lpszIMEFileName,              // src
                            (INT)strlen(lpszIMEFileName),
                            (LPWSTR)lpwszIMEFileName,            // dest
                            (INT)cbIMEFileName);
    lpwszIMEFileName[i] = L'\0';

    i = MultiByteToWideChar(CP_ACP,
                            (DWORD)MB_PRECOMPOSED,
                            (LPSTR)lpszLayoutText,              // src
                            (INT)strlen(lpszLayoutText),
                            (LPWSTR)lpwszLayoutText,            // dest
                            (INT)cbLayoutText);
    lpwszLayoutText[i] = L'\0';

    hKL = ImmInstallIMEW(lpwszIMEFileName, lpwszLayoutText);

    ImmLocalFree(lpwszLayoutText);
    ImmLocalFree(lpwszIMEFileName);

    return hKL;
}


/***************************************************************************\
* ImmIsIME
*
* Checks whether the specified hKL is a HKL of an IME or not.
*
* History:
* 28-Feb-1995   wkwok   Created
\***************************************************************************/

BOOL WINAPI ImmIsIME(
    HKL hKL)
{
    IMEINFOEX iiex;

    if (!ImmGetImeInfoEx(&iiex, ImeInfoExKeyboardLayout, &hKL))
        return FALSE;

    return TRUE;
}


UINT StrToUInt(
    LPWSTR lpsz)
{
    UNICODE_STRING Value;
    UINT ReturnValue;

    Value.Length = wcslen(lpsz) * sizeof(WCHAR);
    Value.Buffer = lpsz;

    /*
     * Convert string to int.
     */
    RtlUnicodeStringToInteger(&Value, 16, &ReturnValue);
    return(ReturnValue);
}


VOID UIntToStr(
    UINT   Value,
    ULONG  Base,
    LPWSTR lpsz,
    USHORT dwBufLen)
{
    UNICODE_STRING String;

    String.Length = dwBufLen;
    String.MaximumLength = dwBufLen;
    String.Buffer = lpsz;

    /*
     * Convert int to string.
     */
    RtlIntegerToUnicodeString(Value, Base, &String);
}


BOOL CopyImeFile(
    LPWSTR lpwszImeFileName,
    LPCWSTR lpwszImeCopiedPath)
{
    HMODULE         hLzExpandDll;
    BOOL            fUnloadExpandDll;
    LPFNLZOPENFILEW lpfnLZOpenFileW;
    LPFNLZCOPY      lpfnLZCopy;
    LPFNLZCLOSE     lpfnLZClose;
    OFSTRUCT        ofStruc;
    HFILE           hfSource, hfDest;
    LPSTR           lpszImeCopiedPath;
    INT             i, cbBuffer;
    BOOL            fRet = FALSE;

    hLzExpandDll = GetModuleHandle(L"LZ32");
    if (hLzExpandDll) {
        fUnloadExpandDll = FALSE;
    } else {
        WCHAR szLzExpand[MAX_PATH];

        GetSystemPathName(szLzExpand, L"LZ32", MAX_PATH);
        hLzExpandDll = LoadLibrary(szLzExpand);
        if (!hLzExpandDll) {
            return FALSE;
        }

        fUnloadExpandDll = TRUE;
    }

#define GET_PROC(x) \
    if (!(lpfn##x = (PVOID) GetProcAddress(hLzExpandDll, sz##x))) { \
        goto CopyImeFileFailed; }

    GET_PROC(LZOpenFileW);
    GET_PROC(LZCopy);
    GET_PROC(LZClose);

#undef GET_PROC

    cbBuffer = (wcslen(lpwszImeCopiedPath) + 1) * sizeof(WCHAR);

    if ((lpszImeCopiedPath = ImmLocalAlloc(0, cbBuffer)) == NULL)
        goto CopyImeFileFailed;

    i = WideCharToMultiByte(CP_ACP,
                            (DWORD)0,
                            lpwszImeCopiedPath,          // src
                            wcslen(lpwszImeCopiedPath),
                            lpszImeCopiedPath,           // dest
                            cbBuffer,
                            (LPSTR)NULL,
                            (LPBOOL)NULL);
    if (i == 0) {
        ImmLocalFree(lpszImeCopiedPath);
        goto CopyImeFileFailed;
    }

    lpszImeCopiedPath[i] = '\0';

    hfSource = (*lpfnLZOpenFileW)(lpwszImeFileName, &ofStruc, OF_READ);
    if (hfSource < 0) {
        ImmLocalFree(lpszImeCopiedPath);
        goto CopyImeFileFailed;
    }

    hfDest = OpenFile(lpszImeCopiedPath, &ofStruc, OF_CREATE);
    if (hfDest != HFILE_ERROR) {
        if ((*lpfnLZCopy)(hfSource, hfDest) >= 0) {
            fRet = TRUE;
        }
        _lclose(hfDest);
    }

    (*lpfnLZClose)(hfSource);

    ImmLocalFree(lpszImeCopiedPath);

CopyImeFileFailed:
    if (fUnloadExpandDll)
        FreeLibrary(hLzExpandDll);

    return fRet;
}


INT GetImeLayout(
    PIMELAYOUT pImeLayout,
    INT        cEntery)
{
    int      i, nIMEs;
    HKEY     hKeyKbdLayout;
    HKEY     hKeyOneIME;
    WCHAR    szKeyName[HEX_ASCII_SIZE];
    WCHAR    szImeFileName[IM_FILE_SIZE];
    DWORD    dwKeyNameSize;
    DWORD    dwTmp;

    RegOpenKey(HKEY_LOCAL_MACHINE, gszRegKbdLayout, &hKeyKbdLayout);

    dwKeyNameSize = sizeof(szKeyName);

    for (i = 0, nIMEs = 0;
         RegEnumKey(hKeyKbdLayout, i, szKeyName, dwKeyNameSize) == ERROR_SUCCESS;
         i++)
    {
        if (szKeyName[0] != L'E' && szKeyName[0] != L'e')
            continue;   // this is not an IME based keyboard layout.

        if (pImeLayout != NULL) {

            if (nIMEs >= cEntery)
                break;

            RegOpenKey(hKeyKbdLayout, szKeyName, &hKeyOneIME);

            dwTmp = IM_FILE_SIZE;

            RegQueryValueEx(hKeyOneIME,
                    gszValImeFile,
                    NULL,
                    NULL,
                    (LPBYTE)szImeFileName,
                    &dwTmp);

            // avoid length problem
            szImeFileName[IM_FILE_SIZE - 1] = L'\0';

            RegCloseKey(hKeyOneIME);

            CharUpper(szImeFileName);

            pImeLayout[nIMEs].hImeKL = (HKL)IntToPtr( StrToUInt(szKeyName) );
            wcscpy(pImeLayout[nIMEs].szKeyName, szKeyName);
            wcscpy(pImeLayout[nIMEs].szImeName, szImeFileName);
        }

        nIMEs++;
    }

    RegCloseKey(hKeyKbdLayout);

    return nIMEs;
}


BOOL WriteImeLayout(
    HKL     hImeKL,
    LPCWSTR lpwszImeFilePart,
    LPCWSTR lpszLayoutText)
{
    int      i;
    HKEY     hKeyKbdLayout;
    HKEY     hKeyOneIME;
    HKEY     hKeyKbdOrder;
    WCHAR    szKeyName[HEX_ASCII_SIZE];
    WCHAR    szImeFileName[IM_FILE_SIZE];
    WCHAR    szOrderNum[HEX_ASCII_SIZE];
    WCHAR    szOrderKeyName[HEX_ASCII_SIZE];
    DWORD    dwTmp;

    if (RegOpenKey(HKEY_LOCAL_MACHINE,
                   gszRegKbdLayout,
                   &hKeyKbdLayout) != ERROR_SUCCESS) {
        RIPMSG0(RIP_WARNING, "WriteImeLayout: RegOpenKey() failed!");
        return FALSE;
    }

    UIntToStr(HandleToUlong(hImeKL), 16, szKeyName, sizeof(szKeyName));

    if (RegCreateKey(hKeyKbdLayout,
                szKeyName,
                &hKeyOneIME) != ERROR_SUCCESS) {
        RIPMSG0(RIP_WARNING, "WriteImeLayout: RegCreateKey() failed!");
        RegCloseKey(hKeyKbdLayout);
        return FALSE;
    }

    if (RegSetValueExW(hKeyOneIME,
                gszValImeFile,
                0,
                REG_SZ,
                (CONST BYTE*)lpwszImeFilePart,
                (wcslen(lpwszImeFilePart) + 1) * sizeof(WCHAR)) != ERROR_SUCCESS) {
        goto WriteImeLayoutFail;
    }

    if (RegSetValueExW(hKeyOneIME,
                gszValLayoutText,
                0,
                REG_SZ,
                (CONST BYTE*)lpszLayoutText,
                (wcslen(lpszLayoutText) + 1) * sizeof(WCHAR)) != ERROR_SUCCESS) {
        goto WriteImeLayoutFail;
    }

    switch (LANGIDFROMHKL(hImeKL)) {
        case LANG_JAPANESE:
            wcscpy(szImeFileName, L"kbdjpn.dll");
            break;
        case LANG_KOREAN:
            wcscpy(szImeFileName, L"kbdkor.dll");
            break;
        case LANG_CHINESE:
        default:
            wcscpy(szImeFileName, L"kbdus.dll");
            break;
    }

    if (RegSetValueExW(hKeyOneIME,
                gszValLayoutFile,
                0,
                REG_SZ,
                (CONST BYTE*)szImeFileName,
                (wcslen(szImeFileName) + 1) * sizeof(WCHAR)) != ERROR_SUCCESS) {
        goto WriteImeLayoutFail;
    }

    RegCloseKey(hKeyOneIME);
    RegCloseKey(hKeyKbdLayout);

    /*
     * Update CurrentUser's preload keyboard layout setting
     */
    RegCreateKey(HKEY_CURRENT_USER, gszRegKbdOrder, &hKeyKbdOrder);

    for (i = 1; i < 1024; i++) {
        UIntToStr(i, 10, szOrderNum, sizeof(szOrderNum));

        dwTmp = sizeof(szOrderKeyName);
        if (RegQueryValueEx(hKeyKbdOrder,
                    szOrderNum,
                    NULL,
                    NULL,
                    (LPBYTE)szOrderKeyName,
                    &dwTmp) != ERROR_SUCCESS) {
            break;
        }

        if (_wcsicmp(szKeyName, szOrderKeyName) == 0) {
            /*
             * We have the same value in the preload!
             * OK, ISV is developing their IMEs
             * so even it is in preload, but it can not be loaded
             */
            break;
        }
    }

    if (i < 1024) {
        /*
         * Write a subkey under "preload"
         */
        RegSetValueExW(hKeyKbdOrder,
                       szOrderNum,
                       0,
                       REG_SZ,
                       (CONST BYTE*)szKeyName,
                       (lstrlen(szKeyName) + 1) * sizeof(WCHAR));
        RegCloseKey(hKeyKbdOrder);
    }
    else {
        RegCloseKey(hKeyKbdOrder);
        return FALSE;
    }

    return TRUE;

WriteImeLayoutFail:
    RegCloseKey(hKeyOneIME);
    RegDeleteKey(hKeyKbdLayout, szKeyName);
    RegCloseKey(hKeyKbdLayout);

    return FALSE;
}


HKL AssignNewLayout(
    INT        nIMEs,
    PIMELAYOUT pImeLayout,
    HKL        hLangKL)
{
    int   i;
    HKL   hHighKL, hLowKL;
    HKL   hImeNewKL;

    /*
     * We prefer the value higher than E01F for ISVs, we will use
     * E001 ~ E01F in Microsoft .INF file
     */
    hHighKL = (HKL)((DWORD)0xE01F0000 + (ULONG_PTR)hLangKL);
    hLowKL  = (HKL)((DWORD)0xE0FF0000 + (ULONG_PTR)hLangKL);

    /*
     * Find out the high and low one
     */
    for (i = 0; i < nIMEs; i++) {
        if (pImeLayout[i].hImeKL > hHighKL) {
            hHighKL = pImeLayout[i].hImeKL;
        }

        if (pImeLayout[i].hImeKL < hLowKL) {
            hLowKL = pImeLayout[i].hImeKL;
        }
    }

    /*
     * This way is more preferable for app consideration
     * we don't want app think this is a old keyboard layout
     * some apps will put hKL into their documents.
     */
    if (hHighKL < (HKL)UIntToPtr( 0xE0FF0000 )) {
        hImeNewKL = (HKL)((ULONG_PTR)hHighKL + (DWORD)0x10000);
    } else if (hLowKL  > (HKL)UIntToPtr( 0xE0010000 )) {
        hImeNewKL = (HKL)((ULONG_PTR)hLowKL  - (DWORD)0x10000);
    } else {
        /*
         * find out a hKL using search, find it one by one
         */
        for (hLowKL = (HKL)((DWORD)0xE0200000 + (ULONG_PTR)hLangKL);
             hLowKL < (HKL)UIntToPtr( 0xE1000000 );
             hLowKL = (HKL)((ULONG_PTR)hLowKL + (DWORD)0x10000)) {

            for (i = 0; i < nIMEs; i++) {
                 if (HIWORD(HandleToUlong(pImeLayout[i].hImeKL)) == HIWORD(HandleToUlong(hLowKL))) {
                     // conflict with existing IME, try next hLowKL
                     break;
                 }
            }

            if (i >= nIMEs) {
                break;
            }
        }

        if (hLowKL >= (HKL)UIntToPtr( 0xE1000000 )) {
            hImeNewKL = (HKL)0;
        }
        else {
            hImeNewKL = hLowKL;
        }
    }

    return hImeNewKL;
}
