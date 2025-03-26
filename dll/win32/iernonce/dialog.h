/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Classes for displaying progress dialog.
 * COPYRIGHT:   Copyright 2021 He Yang <1160386205@qq.com>
 */

#pragma once

#include <atlbase.h>
#include <atlwin.h>

#include "resource.h"
#include "registry.h"

// When wParam < item count ==> wParam is item index (0 based)
//      wParam = item count ==> all finished, lParam = bSuccess
#define WM_SETINDEX (WM_USER + 1)

class ProgressDlg : public CDialogImpl<ProgressDlg>
{
private:
    INT_PTR m_DialogID;
    HWND m_hListBox;
    HFONT m_hBoldFont;
    HBITMAP m_hArrowBmp;
    BITMAP m_ArrowBmp;
    INT m_PointedItem;

public:
    enum { IDD = IDD_DIALOG };

    RunOnceExInstance &m_RunOnceExInst;

    ProgressDlg(_In_ RunOnceExInstance &RunOnceExInst);

    BOOL RunDialogBox();

    void CalcTextRect(_In_ LPCWSTR lpText, _Inout_ RECT *pRect);

    void ResizeListBoxAndDialog(_In_ int NewHeight);

    BOOL ProcessWindowMessage(_In_ HWND hwnd, _In_ UINT message, _In_ WPARAM wParam,
                              _In_ LPARAM lParam, _Out_ LRESULT& lResult,
                              _In_ DWORD dwMsgMapID);
};
