/*
 * PROJECT:     ReactOS msctf.dll
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Multi-language handling of Cicero
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

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
    void GetIconSize(int *pcx, int *pcy);
    INT GetImageCount();
    void RemoveAll(BOOL bNoLock);
};

VOID UninitINAT(VOID);
