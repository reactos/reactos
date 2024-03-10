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
#include <msctf.h>
#include <strsafe.h>

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

/***********************************************************************
 * The helper funtions
 */

/// @unimplemented
static VOID
GetLocaleInfoString(_In_ HKL hKL, _Out_ LPWSTR pszDesc, _In_ UINT cchDesc)
{
    if (!::GetLocaleInfoW(LOWORD(hKL), LOCALE_SLANGUAGE, pszDesc, cchDesc))
        *pszDesc = UNICODE_NULL;
#if 0
    if (HIWORD(hKL) != LOWORD(hKL))
        GetKbdLayoutName(hKL, pszDesc, cchDesc);
#endif
}

/// @unimplemented
HKL GetHKLSubstitute(_In_ HKL hKL)
{
    return hKL;
}

/// @implemented
static VOID
GetHKLDesctription(
    _In_ HKL hKL,
    _Out_ LPWSTR pszDesc,
    _In_ UINT cchDesc,
    _Out_ LPWSTR pszImeFileName,
    _In_ UINT cchImeFileName)
{
    pszDesc[0] = pszImeFileName[0] = UNICODE_NULL;

    if (!IS_IME_HKL(hKL))
    {
        GetLocaleInfoString(hKL, pszDesc, cchDesc);
        return;
    }

    hKL = GetHKLSubstitute(hKL);

    WCHAR szSubKey[MAX_PATH];
    StringCchPrintfW(szSubKey, _countof(szSubKey),
                     L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\%08lX",
                     HandleToUlong(hKL));

    HKEY hKey;
    LSTATUS error = ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, szSubKey, 0, KEY_READ, &hKey);
    if (error == ERROR_SUCCESS)
    {
        DWORD cbDesc = cchDesc * sizeof(WCHAR);
        error = ::RegQueryValueExW(hKey, L"Layout Display Name", NULL, NULL,
                                   (LPBYTE)pszDesc, &cbDesc);
        pszDesc[cbDesc / sizeof(WCHAR) - 1] = UNICODE_NULL; // Avoid buffer overrun
        ::RegCloseKey(hKey);
    }

    if (pszDesc[0] == UNICODE_NULL && !::ImmGetDescriptionW(hKL, pszDesc, cchDesc))
    {
        *pszDesc = UNICODE_NULL;
        GetLocaleInfoString(hKL, pszDesc, cchDesc);
    }
    else if (!::ImmGetIMEFileNameW(hKL, pszImeFileName, cchImeFileName))
    {
        *pszImeFileName = UNICODE_NULL;
    }
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

    WCHAR szDesc[MAX_PATH], szFileName[MAX_PATH];
    GetHKLDesctription(m_hKL, szDesc, (UINT)_countof(szDesc),
                       szFileName, (UINT)_countof(szFileName));
    SetDesc(szDesc);
    m_bInitDesc = TRUE;

    ::EnterCriticalSection(&g_cs);

    for (size_t iKL = 0; iKL < g_pMlngInfo->size(); ++iKL)
    {
        auto& info = (*g_pMlngInfo)[iKL];
        if (info.m_hKL == m_hKL)
        {
            info.m_bInitDesc = TRUE;
            break;
        }
    }

    ::LeaveCriticalSection(&g_cs);
}

/// @implemented
void MLNGINFO::InitIcon()
{
    if (m_bInitIcon)
        return;

    WCHAR szDesc[MAX_PATH], szFileName[MAX_PATH];
    GetHKLDesctription(m_hKL, szDesc, _countof(szDesc), szFileName, _countof(szFileName));
    SetDesc(szDesc);
    m_bInitDesc = TRUE;

    INT cxIcon, cyIcon;
    InatGetIconSize(&cxIcon, &cyIcon);
    if (szFileName[0])
    {
        HICON hIcon = GetIconFromFile(cxIcon, cyIcon, szFileName, 0);
        if (!hIcon)
            hIcon = InatCreateIcon(LOWORD(m_hKL));
        if (hIcon)
        {
            m_iIconIndex = InatAddIcon(hIcon);
            ::DestroyIcon(hIcon);
        }
    }

    ::EnterCriticalSection(&g_cs);

    for (size_t iItem = 0; iItem < g_pMlngInfo->size(); ++iItem)
    {
        auto& item = (*g_pMlngInfo)[iItem];
        if (item.m_hKL == m_hKL)
        {
            item.m_bInitDesc = TRUE;
            item.m_bInitIcon = TRUE;
            item.m_iIconIndex = m_iIconIndex;
            item.SetDesc(szDesc);
            break;
        }
    }

    ::LeaveCriticalSection(&g_cs);
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
