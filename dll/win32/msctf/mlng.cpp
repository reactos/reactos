/*
 * PROJECT:     ReactOS msctf.dll
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Multi-language handling of Cicero
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define WIN32_NO_STATUS

#include <windows.h>
#include <shellapi.h>
#include <imm.h>
#include <imm32_undoc.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shlwapi_undoc.h>
#include <msctf.h>
#include <msctf_undoc.h>
#include <strsafe.h>
#include <assert.h>

#include <cicreg.h>
#include <cicarray.h>

#include <wine/debug.h>

#include "mlng.h"

WINE_DEFAULT_DEBUG_CHANNEL(msctf);

extern CRITICAL_SECTION g_cs;

CicArray<MLNGINFO> *g_pMlngInfo = NULL;
INT CStaticIconList::s_cx = 0;
INT CStaticIconList::s_cy = 0;
CStaticIconList g_IconList;

// Cache for GetSpecialKLID
static HKL s_hCacheKL = NULL;
static DWORD s_dwCacheKLID = 0;

/***********************************************************************
 * The helper funtions
 */

/// @implemented
DWORD GetSpecialKLID(_In_ HKL hKL)
{
    assert(IS_SPECIAL_HKL(hKL));

    if (s_hCacheKL == hKL && s_dwCacheKLID != 0)
        return s_dwCacheKLID;

    s_dwCacheKLID = 0;

    CicRegKey regKey1;
    LSTATUS error = regKey1.Open(HKEY_LOCAL_MACHINE,
                                 L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts");
    if (error != ERROR_SUCCESS)
        return 0;

    WCHAR szName[16], szLayoutId[16];
    const DWORD dwSpecialId = SPECIALIDFROMHKL(hKL);
    for (DWORD dwIndex = 0; ; ++dwIndex)
    {
        error = ::RegEnumKeyW(regKey1, dwIndex, szName, _countof(szName));
        szName[_countof(szName) - 1] = UNICODE_NULL; // Avoid buffer overrun
        if (error != ERROR_SUCCESS)
            break;

        CicRegKey regKey2;
        error = regKey2.Open(regKey1, szName);
        if (error != ERROR_SUCCESS)
            break;

        error = regKey2.QuerySz(L"Layout Id", szLayoutId, _countof(szLayoutId));
        szLayoutId[_countof(szLayoutId) - 1] = UNICODE_NULL; // Avoid buffer overrun
        if (error == ERROR_SUCCESS)
            continue;

        DWORD dwLayoutId = wcstoul(szLayoutId, NULL, 16);
        if (dwLayoutId == dwSpecialId)
        {
            s_hCacheKL = hKL;
            s_dwCacheKLID = wcstoul(szName, NULL, 16);
            break;
        }
    }

    return s_dwCacheKLID;
}

/// @implemented
DWORD GetHKLSubstitute(_In_ HKL hKL)
{
    if (IS_IME_HKL(hKL))
        return HandleToUlong(hKL);

    DWORD dwKLID;
    if (HIWORD(hKL) == LOWORD(hKL))
        dwKLID = LOWORD(hKL);
    else if (IS_SPECIAL_HKL(hKL))
        dwKLID = GetSpecialKLID(hKL);
    else
        dwKLID = HandleToUlong(hKL);

    if (dwKLID == 0)
        return HandleToUlong(hKL);

    CicRegKey regKey;
    LSTATUS error = regKey.Open(HKEY_CURRENT_USER, L"Keyboard Layout\\Substitutes");
    if (error == ERROR_SUCCESS)
    {
        WCHAR szName[MAX_PATH], szValue[MAX_PATH];
        DWORD dwIndex, dwValue;
        for (dwIndex = 0; ; ++dwIndex)
        {
            error = regKey.EnumValue(dwIndex, szName, _countof(szName));
            szName[_countof(szName) - 1] = UNICODE_NULL; // Avoid buffer overrun
            if (error != ERROR_SUCCESS)
                break;

            error = regKey.QuerySz(szName, szValue, _countof(szValue));
            szValue[_countof(szValue) - 1] = UNICODE_NULL; // Avoid buffer overrun
            if (error != ERROR_SUCCESS)
                break;

            dwValue = wcstoul(szValue, NULL, 16);
            if ((dwKLID & ~SPECIAL_MASK) == dwValue)
            {
                dwKLID = wcstoul(szName, NULL, 16);
                break;
            }
        }
    }

    return dwKLID;
}

/// @implemented
static BOOL
GetKbdLayoutNameFromReg(_In_ HKL hKL, _Out_ LPWSTR pszDesc, _In_ UINT cchDesc)
{
    const DWORD dwKLID = GetHKLSubstitute(hKL);

    WCHAR szSubKey[MAX_PATH];
    StringCchPrintfW(szSubKey, _countof(szSubKey),
                     L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\%08lX",
                     dwKLID);

    CicRegKey regKey;
    LSTATUS error = regKey.Open(HKEY_LOCAL_MACHINE, szSubKey);
    if (error != ERROR_SUCCESS)
        return FALSE;

    if (SHLoadRegUIStringW(regKey, L"Layout Display Name", pszDesc, cchDesc) == S_OK)
    {
        pszDesc[cchDesc - 1] = UNICODE_NULL; // Avoid buffer overrun
        return TRUE;
    }

    error = regKey.QuerySz(L"Layout Text", pszDesc, cchDesc);
    pszDesc[cchDesc - 1] = UNICODE_NULL; // Avoid buffer overrun
    return (error == ERROR_SUCCESS);
}

/// @implemented
static BOOL
GetHKLName(_In_ HKL hKL, _Out_ LPWSTR pszDesc, _In_ UINT cchDesc)
{
    if (::GetLocaleInfoW(LOWORD(hKL), LOCALE_SLANGUAGE, pszDesc, cchDesc))
        return TRUE;

    *pszDesc = UNICODE_NULL;

    if (LOWORD(hKL) == HIWORD(hKL))
        return FALSE;

    return GetKbdLayoutNameFromReg(hKL, pszDesc, cchDesc);
}

/// @implemented
static BOOL
GetHKLDesctription(
    _In_ HKL hKL,
    _Out_ LPWSTR pszDesc,
    _In_ UINT cchDesc,
    _Out_ LPWSTR pszImeFileName,
    _In_ UINT cchImeFileName)
{
    pszDesc[0] = pszImeFileName[0] = UNICODE_NULL;

    if (!IS_IME_HKL(hKL))
        return GetHKLName(hKL, pszDesc, cchDesc);

    if (GetKbdLayoutNameFromReg(hKL, pszDesc, cchDesc))
        return TRUE;

    if (!::ImmGetDescriptionW(hKL, pszDesc, cchDesc))
    {
        *pszDesc = UNICODE_NULL;
        return GetHKLName(hKL, pszDesc, cchDesc);
    }

    if (!::ImmGetIMEFileNameW(hKL, pszImeFileName, cchImeFileName))
        *pszImeFileName = UNICODE_NULL;

    return TRUE;
}

/// @implemented
HICON GetIconFromFile(_In_ INT cx, _In_ INT cy, _In_ LPCWSTR pszFileName, _In_ INT iIcon)
{
    HICON hIcon;

    if (cx <= GetSystemMetrics(SM_CXSMICON))
        ::ExtractIconExW(pszFileName, iIcon, NULL, &hIcon, 1);
    else
        ::ExtractIconExW(pszFileName, iIcon, &hIcon, NULL, 1);

    return hIcon;
}

/// @implemented
static BOOL EnsureIconImageList(VOID)
{
    if (!CStaticIconList::s_cx)
        g_IconList.Init(::GetSystemMetrics(SM_CYSMICON), ::GetSystemMetrics(SM_CXSMICON));

    return TRUE;
}

/// @implemented
static INT GetPhysicalFontHeight(LOGFONTW *plf)
{
    HDC hDC = ::GetDC(NULL);
    HFONT hFont = ::CreateFontIndirectW(plf);
    HGDIOBJ hFontOld = ::SelectObject(hDC, hFont);
    TEXTMETRICW tm;
    ::GetTextMetricsW(hDC, &tm);
    INT ret = tm.tmExternalLeading + tm.tmHeight;
    ::SelectObject(hDC, hFontOld);
    ::DeleteObject(hFont);
    ::ReleaseDC(NULL, hDC);
    return ret;
}

/***********************************************************************
 * Inat helper functions
 */

/// @implemented
INT InatAddIcon(_In_ HICON hIcon)
{
    if (!EnsureIconImageList())
        return -1;
    return g_IconList.AddIcon(hIcon);
}

/// @implemented
HICON
InatCreateIconBySize(
    _In_ LANGID LangID,
    _In_ INT nWidth,
    _In_ INT nHeight,
    _In_ const LOGFONTW *plf)
{
    WCHAR szText[64];
    BOOL ret = ::GetLocaleInfoW(LangID, LOCALE_NOUSEROVERRIDE | LOCALE_SABBREVLANGNAME,
                                szText, _countof(szText));
    if (!ret)
        szText[0] = szText[1] = L'?';

    szText[2] = UNICODE_NULL;
    CharUpperW(szText);

    HFONT hFont = ::CreateFontIndirectW(plf);
    if (!hFont)
        return NULL;

    HDC hDC = ::GetDC(NULL);
    HDC hMemDC = ::CreateCompatibleDC(hDC);
    HBITMAP hbmColor = ::CreateCompatibleBitmap(hDC, nWidth, nHeight);
    HBITMAP hbmMask = ::CreateBitmap(nWidth, nHeight, 1, 1, NULL);
    ::ReleaseDC(NULL, hDC);

    HICON hIcon = NULL;
    HGDIOBJ hbmOld = ::SelectObject(hMemDC, hbmColor);
    HGDIOBJ hFontOld = ::SelectObject(hMemDC, hFont);
    if (hMemDC && hbmColor && hbmMask)
    {
        ::SetBkColor(hMemDC, ::GetSysColor(COLOR_HIGHLIGHT));
        ::SetTextColor(hMemDC, ::GetSysColor(COLOR_HIGHLIGHTTEXT));

        RECT rc = { 0, 0, nWidth, nHeight };
        ::ExtTextOutW(hMemDC, 0, 0, ETO_OPAQUE, &rc, L"", 0, NULL);

        ::DrawTextW(hMemDC, szText, 2, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        ::SelectObject(hMemDC, hbmMask);

        ::PatBlt(hMemDC, 0, 0, nWidth, nHeight, BLACKNESS);

        ICONINFO IconInfo = { TRUE, 0, 0, hbmMask, hbmColor };
        hIcon = ::CreateIconIndirect(&IconInfo);
    }
    ::SelectObject(hMemDC, hFontOld);
    ::SelectObject(hMemDC, hbmOld);

    ::DeleteObject(hbmMask);
    ::DeleteObject(hbmColor);
    ::DeleteDC(hMemDC);
    ::DeleteObject(hFont);
    return hIcon;
}

/// @implemented
HICON InatCreateIcon(_In_ LANGID LangID)
{
    INT cxSmIcon = ::GetSystemMetrics(SM_CXSMICON), cySmIcon = ::GetSystemMetrics(SM_CYSMICON);

    LOGFONTW lf;
    if (!SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(LOGFONTW), &lf, 0))
        return NULL;

    if (cySmIcon < GetPhysicalFontHeight(&lf))
    {
        lf.lfWidth = 0;
        lf.lfHeight = - (7 * cySmIcon) / 10;
    }

    return InatCreateIconBySize(LangID, cxSmIcon, cySmIcon, &lf);
}

/// @implemented
BOOL InatGetIconSize(_Out_ INT *pcx, _Out_ INT *pcy)
{
    g_IconList.GetIconSize(pcx, pcy);
    return TRUE;
}

/// @implemented
INT InatGetImageCount(VOID)
{
    return g_IconList.GetImageCount();
}

/// @implemented
VOID InatRemoveAll(VOID)
{
    if (CStaticIconList::s_cx)
        g_IconList.RemoveAll(FALSE);
}

/// @implemented
VOID UninitINAT(VOID)
{
    g_IconList.RemoveAll(TRUE);

    if (g_pMlngInfo)
    {
        delete g_pMlngInfo;
        g_pMlngInfo = NULL;
    }
}

/***********************************************************************
 * MLNGINFO
 */

/// @implemented
void MLNGINFO::InitDesc()
{
    if (m_bInitDesc)
        return;

    WCHAR szDesc[MAX_PATH], szImeFileName[MAX_PATH];
    GetHKLDesctription(m_hKL, szDesc, (UINT)_countof(szDesc),
                       szImeFileName, (UINT)_countof(szImeFileName));
    SetDesc(szDesc);
    m_bInitDesc = TRUE;
}

/// @implemented
void MLNGINFO::InitIcon()
{
    if (m_bInitIcon)
        return;

    WCHAR szDesc[MAX_PATH], szImeFileName[MAX_PATH];
    GetHKLDesctription(m_hKL, szDesc, (UINT)_countof(szDesc),
                       szImeFileName, (UINT)_countof(szImeFileName));
    SetDesc(szDesc);
    m_bInitDesc = TRUE;

    INT cxIcon, cyIcon;
    InatGetIconSize(&cxIcon, &cyIcon);

    HICON hIcon = NULL;
    if (szImeFileName[0])
        hIcon = GetIconFromFile(cxIcon, cyIcon, szImeFileName, 0);

    if (!hIcon)
        hIcon = InatCreateIcon(LOWORD(m_hKL));

    if (hIcon)
    {
        m_iIconIndex = InatAddIcon(hIcon);
        ::DestroyIcon(hIcon);
    }

    m_bInitIcon = TRUE;
}

/// @implemented
LPCWSTR MLNGINFO::GetDesc()
{
    if (!m_bInitDesc)
        InitDesc();

    return m_szDesc;
}

/// @implemented
void MLNGINFO::SetDesc(LPCWSTR pszDesc)
{
    StringCchCopyW(m_szDesc, _countof(m_szDesc), pszDesc);
}

/// @implemented
INT MLNGINFO::GetIconIndex()
{
    if (!m_bInitIcon)
        InitIcon();

    return m_iIconIndex;
}

/***********************************************************************
 * CStaticIconList
 */

/// @implemented
void CStaticIconList::Init(INT cxIcon, INT cyIcon)
{
    ::EnterCriticalSection(&g_cs);
    s_cx = cxIcon;
    s_cy = cyIcon;
    ::LeaveCriticalSection(&g_cs);
}

/// @implemented
INT CStaticIconList::AddIcon(HICON hIcon)
{
    ::EnterCriticalSection(&g_cs);

    INT iItem = -1;
    HICON hCopyIcon = ::CopyIcon(hIcon);
    if (hCopyIcon)
    {
        if (g_IconList.Add(hIcon))
            iItem = INT(g_IconList.size() - 1);
    }

    ::LeaveCriticalSection(&g_cs);
    return iItem;
}

/// @implemented
HICON CStaticIconList::ExtractIcon(INT iIcon)
{
    HICON hCopyIcon = NULL;
    ::EnterCriticalSection(&g_cs);
    if (iIcon <= (INT)g_IconList.size())
        hCopyIcon = ::CopyIcon(g_IconList[iIcon]);
    ::LeaveCriticalSection(&g_cs);
    return hCopyIcon;
}

/// @implemented
void CStaticIconList::GetIconSize(INT *pcx, INT *pcy)
{
    ::EnterCriticalSection(&g_cs);
    *pcx = s_cx;
    *pcy = s_cy;
    ::LeaveCriticalSection(&g_cs);
}

/// @implemented
INT CStaticIconList::GetImageCount()
{
    ::EnterCriticalSection(&g_cs);
    INT cItems = (INT)g_IconList.size();
    ::LeaveCriticalSection(&g_cs);
    return cItems;
}

/// @implemented
void CStaticIconList::RemoveAll(BOOL bNoLock)
{
    if (!bNoLock)
        ::EnterCriticalSection(&g_cs);

    for (size_t iItem = 0; iItem < g_IconList.size(); ++iItem)
    {
        ::DestroyIcon(g_IconList[iItem]);
    }

    clear();

    if (!bNoLock)
       ::LeaveCriticalSection(&g_cs);
}

/// @implemented
static BOOL CheckMlngInfo(VOID)
{
    if (!g_pMlngInfo)
        return TRUE; // Needs creation

    INT cKLs = ::GetKeyboardLayoutList(0, NULL);
    if (cKLs != TF_MlngInfoCount())
        return TRUE; // Needs refresh

    if (!cKLs)
        return FALSE;

    HKL *phKLs = (HKL*)cicMemAlloc(cKLs * sizeof(HKL));
    if (!phKLs)
        return FALSE;

    ::GetKeyboardLayoutList(cKLs, phKLs);

    assert(g_pMlngInfo);

    BOOL ret = FALSE;
    for (INT iKL = 0; iKL < cKLs; ++iKL)
    {
        if ((*g_pMlngInfo)[iKL].m_hKL != phKLs[iKL])
        {
            ret = TRUE; // Needs refresh
            break;
        }
    }

    cicMemFree(phKLs);
    return ret;
}

/// @implemented
static VOID DestroyMlngInfo(VOID)
{
    if (!g_pMlngInfo)
        return;

    delete g_pMlngInfo;
    g_pMlngInfo = NULL;
}

/// @implemented
static VOID CreateMlngInfo(VOID)
{
    if (!g_pMlngInfo)
    {
        g_pMlngInfo = new(cicNoThrow) CicArray<MLNGINFO>();
        if (!g_pMlngInfo)
            return;
    }

    if (!EnsureIconImageList())
        return;

    INT cKLs = ::GetKeyboardLayoutList(0, NULL);
    HKL *phKLs = (HKL*)cicMemAllocClear(cKLs * sizeof(HKL));
    if (!phKLs)
        return;

    ::GetKeyboardLayoutList(cKLs, phKLs);

    for (INT iKL = 0; iKL < cKLs; ++iKL)
    {
        MLNGINFO& info = (*g_pMlngInfo)[iKL];
        info.m_hKL = phKLs[iKL];
        info.m_bInitDesc = FALSE;
        info.m_bInitIcon = FALSE;
    }

    cicMemFree(phKLs);
}

/***********************************************************************
 *              TF_InitMlngInfo (MSCTF.@)
 *
 * @implemented
 */
EXTERN_C VOID WINAPI TF_InitMlngInfo(VOID)
{
    TRACE("()\n");

    ::EnterCriticalSection(&g_cs);

    if (CheckMlngInfo())
    {
        DestroyMlngInfo();
        CreateMlngInfo();
    }

    ::LeaveCriticalSection(&g_cs);
}

/***********************************************************************
 *              TF_MlngInfoCount (MSCTF.@)
 *
 * @implemented
 */
EXTERN_C INT WINAPI TF_MlngInfoCount(VOID)
{
    TRACE("()\n");

    if (!g_pMlngInfo)
        return 0;

    return (INT)g_pMlngInfo->size();
}

/***********************************************************************
 *              TF_InatExtractIcon (MSCTF.@)
 *
 * @implemented
 */
EXTERN_C HICON WINAPI TF_InatExtractIcon(_In_ INT iKL)
{
    TRACE("(%d)\n", iKL);
    return g_IconList.ExtractIcon(iKL);
}

/***********************************************************************
 *              TF_GetMlngIconIndex (MSCTF.@)
 *
 * @implemented
 */
EXTERN_C INT WINAPI TF_GetMlngIconIndex(_In_ INT iKL)
{
    TRACE("(%d)\n", iKL);

    INT iIcon = -1;

    ::EnterCriticalSection(&g_cs);

    assert(g_pMlngInfo);

    if (iKL < (INT)g_pMlngInfo->size())
        iIcon = (*g_pMlngInfo)[iKL].GetIconIndex();

    ::LeaveCriticalSection(&g_cs);

    return iIcon;
}

/***********************************************************************
 *              TF_GetMlngHKL (MSCTF.@)
 *
 * @implemented
 */
EXTERN_C BOOL WINAPI
TF_GetMlngHKL(
    _In_ INT iKL,
    _Out_opt_ HKL *phKL,
    _Out_opt_ LPWSTR pszDesc,
    _In_ INT cchDesc)
{
    TRACE("(%d, %p, %p, %d)\n", iKL, phKL, pszDesc, cchDesc);

    BOOL ret = FALSE;

    ::EnterCriticalSection(&g_cs);

    assert(g_pMlngInfo);

    if (iKL < (INT)g_pMlngInfo->size())
    {
        MLNGINFO& info = (*g_pMlngInfo)[iKL];

        if (phKL)
            *phKL = info.m_hKL;

        if (pszDesc)
            StringCchCopyW(pszDesc, cchDesc, info.GetDesc());

        ret = TRUE;
    }

    ::LeaveCriticalSection(&g_cs);

    return ret;
}
