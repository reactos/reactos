/*
 * PROJECT:     ReactOS msctf.dll
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Multi-language handling of Cicero
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#define CTF_COMPAT_DELAY_FIRST_ACTIVATE 2

struct MLNGINFO
{
    HKL m_hKL;
    BOOL m_bInitDesc;
    BOOL m_bInitIcon;
    INT m_iIconIndex;
    WCHAR m_szDesc[128];

    void InitDesc();
    void InitIcon();

    INT GetIconIndex();
    LPCWSTR GetDesc();
    void SetDesc(LPCWSTR pszDesc);
};

class CStaticIconList : public CicArray<HICON>
{
public:
    static INT s_cx;
    static INT s_cy;

    CStaticIconList() { }

    void Init(INT cxIcon, INT cyIcon);
    INT AddIcon(HICON hIcon);
    HICON ExtractIcon(INT iIcon);
    void GetIconSize(INT *pcx, INT *pcy);
    INT GetImageCount();
    void RemoveAll(BOOL bNoLock);
};

INT InatAddIcon(_In_ HICON hIcon);
HICON InatCreateIcon(_In_ LANGID LangID);

HICON
InatCreateIconBySize(
    _In_ LANGID LangID,
    _In_ INT nWidth,
    _In_ INT nHeight,
    _In_ const LOGFONTW *plf);

BOOL InatGetIconSize(_Out_ INT *pcx, _Out_ INT *pcy);
INT InatGetImageCount(VOID);
VOID InatRemoveAll(VOID);

DWORD GetHKLSubstitute(_In_ HKL hKL);
HICON GetIconFromFile(_In_ INT cx, _In_ INT cy, _In_ LPCWSTR pszFileName, _In_ INT iIcon);

VOID UninitINAT(VOID);
