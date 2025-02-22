/*
 * PROJECT:     ReactOS Compatibility Layer Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     CEditCompatModes implementation
 * COPYRIGHT:   Copyright 2015-2019 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"
#include <windowsx.h>


class CEditCompatModes : public CDialogImpl<CEditCompatModes>
{
private:
    CLayerUIPropPage* m_pPage;
    HWND m_hListAdd;
    HWND m_hListActive;

    CStringW GetListText(HWND ListBox, int Cur)
    {
        CStringW Str;
        int Length = ListBox_GetTextLen(ListBox, Cur);
        LPWSTR Buffer = Str.GetBuffer(Length + 1);
        ListBox_GetText(ListBox, Cur, Buffer);
        Str.ReleaseBuffer(Length);
        return Str;
    }

public:
    CEditCompatModes(CLayerUIPropPage* page)
        : m_pPage(page)
    {
        m_pPage->AddRef();
    }

    ~CEditCompatModes()
    {
        m_pPage->Release();
    }

    LRESULT OnInitDialog(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        CenterWindow(GetParent());

        m_hListActive = GetDlgItem(IDC_COMPATIBILITYMODE);
        m_hListAdd = GetDlgItem(IDC_NEWCOMPATIBILITYMODE);

        CComObject<CLayerStringList> pList;

        while (TRUE)
        {
            CComHeapPtr<OLECHAR> str;
            HRESULT hr = pList.Next(1, &str, NULL);
            if (hr != S_OK)
                break;
            ListBox_AddString(m_hListAdd, str);
        }

        for (int n = 0; n < m_pPage->m_CustomLayers.GetSize(); ++n)
        {
            const WCHAR* Str = m_pPage->m_CustomLayers[n].GetString();
            int Index = ListBox_FindStringExact(m_hListActive, -1, Str);
            if (Index == LB_ERR)
                Index = ListBox_AddString(m_hListActive, Str);
        }

        OnListboxUpdated(0, 0, 0, bHandled);
        return 0;
    }

    LRESULT OnButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        if (wID == IDOK)
        {
            int Count = ListBox_GetCount(m_hListActive);
            m_pPage->m_CustomLayers.RemoveAll();
            for (int Cur = 0; Cur < Count; ++Cur)
            {
                CString Str = GetListText(m_hListActive, Cur);
                m_pPage->m_CustomLayers.Add(Str);
            }
        }
        EndDialog(wID);
        return 0;
    }

    LRESULT OnAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        int Sel = ListBox_GetCurSel(m_hListAdd);
        CStringW Str = GetListText(m_hListAdd, Sel);

        int Index = ListBox_FindStringExact(m_hListActive, -1, Str);
        if (Index == LB_ERR)
            Index = ListBox_AddString(m_hListActive, Str);

        ::SetFocus(m_hListAdd);
        return 0;
    }

    LRESULT OnRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        int Sel = ListBox_GetCurSel(m_hListActive);
        CStringW Str = GetListText(m_hListActive, Sel);

        ListBox_DeleteString(m_hListActive, Sel);
        int Index = ListBox_FindStringExact(m_hListAdd, -1, Str);
        if (Index != LB_ERR)
            Index = ListBox_SetCurSel(m_hListAdd, Index);
        OnListboxUpdated(wNotifyCode, wID, hWndCtl, bHandled);
        return 0;
    }

    LRESULT OnRemoveAll(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        ListBox_ResetContent(m_hListActive);
        OnListboxUpdated(wNotifyCode, wID, hWndCtl, bHandled);
        return 0;
    }

    LRESULT OnListboxUpdated(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        if (m_hListAdd == hWndCtl)
            ListBox_SetCurSel(m_hListActive, -1);
        else if (m_hListActive == hWndCtl)
            ListBox_SetCurSel(m_hListAdd, -1);

        ::EnableWindow(GetDlgItem(IDC_ADD), ListBox_GetCurSel(m_hListAdd) >= 0);
        ::EnableWindow(GetDlgItem(IDC_REMOVE), ListBox_GetCurSel(m_hListActive) >= 0);
        ::EnableWindow(GetDlgItem(IDC_REMOVEALL), ListBox_GetCount(m_hListActive) > 0);
        bHandled = TRUE;
        return 0;
    }

public:
    enum { IDD = IDD_EDITCOMPATIBILITYMODES };

    BEGIN_MSG_MAP(CEditCompatModes)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDC_ADD, OnAdd)
        COMMAND_ID_HANDLER(IDC_REMOVE, OnRemove)
        COMMAND_ID_HANDLER(IDC_REMOVEALL, OnRemoveAll)

        COMMAND_ID_HANDLER(IDOK, OnButton)
        COMMAND_ID_HANDLER(IDCANCEL, OnButton)
        COMMAND_ID_HANDLER(IDC_COMPATIBILITYMODE, OnListboxUpdated)
        COMMAND_ID_HANDLER(IDC_NEWCOMPATIBILITYMODE, OnListboxUpdated)
    END_MSG_MAP()
};


BOOL ShowEditCompatModes(HWND hWnd, CLayerUIPropPage* page)
{
    CEditCompatModes modes(page);
    INT_PTR Result = modes.DoModal(hWnd);
    return Result == IDOK;
}


