/*
 * ReactOS Explorer
 *
 * Copyright 2015 Jared Smudde <computerwhiz02@hotmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"

class TrayIconView : public CListView
{
    HIMAGELIST hImageList;
    HWND hListView;
    HWND hCombo;
    CAtlArray<NOTIFYICONDATAW> m_TrayIconData;

    HWND FindWindowHandleByPath(LPCWSTR path);
public:
    ~TrayIconView();
    HWND Create(HWND hwndParent, DWORD id);
    BOOL OnNotify(HWND hwnd, WPARAM wParam, LPARAM lParam);
    BOOL OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
    BOOL AddColumn(INT Index, INT Width, DWORD strId, INT fmt = LVCFMT_LEFT);
    BOOL InsertItem(LPWSTR data, INT iSubItem, INT id);
    VOID BuildIconList();
    VOID UpdateIcon(USHORT iItem, USHORT action);
};

TrayIconView::~TrayIconView()
{
    DeleteAllItems();
    if (hImageList)
        ImageList_Destroy(hImageList);
    SendMessage(hCombo, CB_RESETCONTENT, 0, 0);
}


HWND TrayIconView::FindWindowHandleByPath(LPCWSTR path)
{
    HWND hwnd;;
    CStringW root(path);
    int count = root.Replace(L'/',L'\0');
    LPWSTR ptr = (LPWSTR)&root[lstrlenW(root)+1];

    hwnd = FindWindowW(root, NULL);

    for (int i = 0; i < count; i++)
    {
        hwnd = FindWindowExW(hwnd, NULL, ptr, NULL);
        ptr = &ptr[lstrlenW(ptr)+1];
    }
    return hwnd;
}

HWND TrayIconView::Create(HWND hwndParent, DWORD id)
{
    hListView = m_hWnd = ::GetDlgItem(hwndParent, id);
    hCombo = ::GetDlgItem(hwndParent, IDC_NOTIFICATION_BEHAVIOUR);

    SetExtendedListViewStyle(LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

    AddColumn(0, 176, IDS_NOTIFY_COLUMN1);
    AddColumn(1, 150, IDS_NOTIFY_COLUMN2);

    for (int i = 0; i < 3; i++)
    {
        CStringW szNotify(MAKEINTRESOURCEW(IDS_NOTIFY_HIDEINACTIVE + i));
        SendMessageW(hCombo, CB_ADDSTRING, i, (LPARAM)szNotify.GetString());
    }
    SendMessage(hCombo, CB_SETCURSEL, 1, 0);

    return  m_hWnd;
}

BOOL TrayIconView::OnNotify(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HWND hHeader = ListView_GetHeader(hListView);
    LPNMHDR nmh = (LPNMHDR)lParam;

    if (nmh->hwndFrom == hHeader)
    {
  /*      if (((LPNMHDR)lParam)->code == HDN_BEGINTRACKW)
        {

        }
        else if (((LPNMHDR)lParam)->code == HDN_TRACKW)
        {

        }
        else */if (((LPNMHDR)lParam)->code == HDN_ENDTRACKW)
        {
            HD_NOTIFY *hdn = (HD_NOTIFY*)lParam;
            if (hdn->iItem == 0)
            {
                RECT rcItem, rcHdr, rcWnd;
                INT iItem = ListView_GetNextItem(hListView, -1, LVNI_ALL | LVNI_SELECTED);
                ListView_GetItemRect(hListView, iItem, &rcItem, LVIR_BOUNDS);
                Header_GetItemRect(hHeader, 1, &rcHdr);
                ::GetWindowRect(hListView, &rcWnd);
                ::ScreenToClient(hwnd, (LPPOINT)&rcWnd); 
                ::SetWindowPos(hCombo, NULL, rcWnd.left + rcHdr.left, rcWnd.top + rcItem.top, 0, 0, SWP_NOSIZE);
            }
            else
            {
                RECT rcCombo;
                ::GetWindowRect(hCombo, &rcCombo);
                ::SetWindowPos(hCombo, NULL, 0, 0, hdn->pitem->cxy, rcCombo.bottom, SWP_NOMOVE);
            }
        }
        return TRUE;
    }
    else if (nmh->idFrom == IDC_NOTIFICATION_LIST)
    {
         switch (nmh->code)
        {
            case LVN_ITEMCHANGED:
            {
                RECT rcItem, rcWnd, rcParent, rcHdr;
                NM_LISTVIEW *nmlv = (NM_LISTVIEW*)lParam;

                ::GetWindowRect(hListView, &rcWnd);
                ::GetWindowRect(hwnd, &rcParent);
                ListView_GetSubItemRect(nmlv->hdr.hwndFrom, nmlv->iItem, 1, LVIR_LABEL, &rcItem);

                LVITEMW iItem;
                iItem.iItem = nmlv->iItem;
                iItem.iSubItem = 0;
                iItem.mask =  LVIF_PARAM;
                ListView_GetItem(nmlv->hdr.hwndFrom, &iItem);

                Header_GetItemRect(hHeader, 0, &rcHdr);
                int cy = GetSystemMetrics(SM_CYBORDER) * 4;
                int x = rcItem.left + (rcWnd.left - rcParent.left) - 2;
                int y = rcItem.top + (rcWnd.top - rcParent.top) - rcHdr.bottom - cy;

                SendMessage(hCombo, CB_SETCURSEL, iItem.lParam, 0);
                ::SetWindowPos(hCombo, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
                return FALSE;
            }
        }
    }
    return TRUE;
}

BOOL TrayIconView::OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    switch(LOWORD(wParam))
    {
        case IDC_NOTIFICATION_BEHAVIOUR:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                WCHAR szText[128];
                HWND hComBox = (HWND)lParam;
                LVITEMW iItem;

                INT idx = SendMessageW(hComBox, CB_GETCURSEL, 0, 0);
                SendMessageW(hComBox, CB_GETLBTEXT, (WPARAM)idx, (LPARAM)szText);

                INT lvItem = GetNextItem(-1, LVNI_SELECTED);

                iItem.mask = LVIF_TEXT;
                iItem.cchTextMax = lstrlenW(szText);
                iItem.pszText = szText;
                iItem.iItem = lvItem;
                iItem.iSubItem = 1;
                ListView_SetItem(hListView, &iItem);

                iItem.mask = LVIF_PARAM;
                iItem.iItem = lvItem;
                iItem.iSubItem = 0;
                iItem.lParam = idx;
                ListView_SetItem(hListView, &iItem);

                ::ShowWindow(hComBox, SW_HIDE);
                UpdateIcon(lvItem, idx);
            }
            return FALSE;

        case IDOK:
        case IDCANCEL:
            SendMessage(hwnd, WM_CLOSE, 0, 0);
            break;
    
        default:
            return FALSE;
    }
    return TRUE;
}

BOOL TrayIconView::AddColumn(INT Index, INT Width, DWORD strId, INT fmt)
{
    CStringW strLabel(MAKEINTRESOURCEW(strId));
    LVCOLUMNW Column;
    ZeroMemory(&Column, sizeof(Column));

    Column.mask =  LVCF_WIDTH | LVCF_SUBITEM | LVCF_TEXT | LVCF_FMT;
    Column.iSubItem = Index;
    Column.cx = Width;
    Column.pszText = (LPWSTR)strLabel.GetString();
    Column.cchTextMax = strLabel.GetLength();
    Column.fmt = fmt;

    return (InsertColumn(Index, &Column) == -1) ? FALSE : TRUE;
}

BOOL TrayIconView::InsertItem(LPWSTR data, INT iSubItem, INT id)
{
    LVITEMW iItem;

    iItem.mask = LVIF_TEXT | LVIF_IMAGE;
    iItem.pszText = data;
    iItem.cchTextMax = lstrlenW(data);
    iItem.iSubItem = iSubItem;
    iItem.iItem = id;
    iItem.iImage = id;

    if (iSubItem)
        return (ListView_SetItem(m_hWnd, &iItem) != -1);
    else
        return (ListView_InsertItem(m_hWnd, &iItem) != -1);
}

VOID TrayIconView::UpdateIcon(USHORT iItem, USHORT Preference)
{
    HWND m_hwndPager = FindWindowHandleByPath(L"Shell_TrayWnd/TrayNotifyWnd/SysPager");
    NOTIFYICONDATAW *pIcon = &m_TrayIconData[iItem];
    SendMessage(m_hwndPager, TNWM_RESIZETRAYICON, Preference, (LPARAM)pIcon);
}

VOID TrayIconView::BuildIconList()
{
    HWND hToolbar = FindWindowHandleByPath(L"Shell_TrayWnd/TrayNotifyWnd/SysPager/ToolbarWindow32");
    int count = SendMessageW(hToolbar, TB_BUTTONCOUNT, 0, 0);
    hImageList = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR32 | ILC_MASK, count, 0);
    TBBUTTON tb = {0};

    for(int idx = 0 ; idx < count; idx++)
    {
        ::SendMessageW(hToolbar, TB_GETBUTTON, idx, (LPARAM)&tb);
        NOTIFYICONDATAW* idata = (NOTIFYICONDATAW*)tb.dwData;
  ///      if (idata->uFlags & NIF_ICON)
        {
            ImageList_AddIcon(hImageList, idata->hIcon);
        }
        idata->cbSize = sizeof(NOTIFYICONDATAW);
        m_TrayIconData.Add(*idata);

        CStringW szNotification;
        szNotification.LoadString(IDS_NOTIFY_HIDEINACTIVE);

   ///     if (idata->uFlags & NIF_TIP)
        {
            InsertItem(idata->szTip, 0, idx);
        }
        InsertItem((LPWSTR)szNotification.GetString(), 1, idx);
    }

    SetImageList(hImageList, LVSIL_SMALL);
}

static TrayIconView *m_pListView = NULL;

INT_PTR CALLBACK CustomizeNotifyIconsProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message)
    {
        case WM_INITDIALOG:
        {
            m_pListView = new TrayIconView();
            m_pListView->Create(hwnd, IDC_NOTIFICATION_LIST);
            m_pListView->BuildIconList();
            return TRUE;
        }

        case WM_NOTIFY:
            return m_pListView->OnNotify(hwnd, wParam, lParam);

        case WM_COMMAND:
            return m_pListView->OnCommand(hwnd, wParam, lParam);

        case WM_CLOSE:
            SendMessage(hwnd, WM_DESTROY, 0, 0);
            break;

        case WM_DESTROY:
            delete m_pListView;
            m_pListView = NULL;
            EndDialog(hwnd, 0);
            break;

        default:
            return FALSE;
    }
    return TRUE;
}

VOID ShowCustomizeNotifyIcons(HINSTANCE hInst, HWND hExplorer)
{
    DialogBoxW(hInst, MAKEINTRESOURCEW(IDD_NOTIFICATIONS_CUSTOMIZE), hExplorer, CustomizeNotifyIconsProc);
}