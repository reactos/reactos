/*
 * PROJECT:     ReactOS Disk Cleanup
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Disk cleanup entrypoint
 * COPYRIGHT:   Copyright 2023-2025 Mark Jansen <mark.jansen@reactos.org>
 */

#include "cleanmgr.h"

// for listview with extend style LVS_EX_CHECKBOXES, State image 1 is the unchecked box, and state image 2 is the
// checked box. see this: https://docs.microsoft.com/en-us/windows/win32/controls/extended-list-view-styles
#define STATEIMAGETOINDEX(x) (((x)&LVIS_STATEIMAGEMASK) >> 12)
#define STATEIMAGE_UNCHECKED 1
#define STATEIMAGE_CHECKED 2


struct CCleanMgrProperties :
    public CPropertyPageImpl<CCleanMgrProperties>
{
    enum { IDD = IDD_PROPERTIES_MAIN };
    CWindow m_HandlerListControl;
    WCHAR m_Drive;
    DWORDLONG m_TotalSpaceUsed;
    CCleanupHandlerList* m_HandlerList;
    bool m_IgnoreChanges = true;


    CCleanMgrProperties(WCHAR Drive, DWORDLONG TotalSpaceUsed, CCleanupHandlerList *handlerList)
        : m_Drive(Drive)
        , m_TotalSpaceUsed(TotalSpaceUsed)
        , m_HandlerList(handlerList)
    {
    }

    int OnApply()
    {
        CStringW Title(MAKEINTRESOURCE(IDS_DISK_CLEANUP));
        CStringW Text(MAKEINTRESOURCE(IDS_CONFIRM_DELETE));

        if (MessageBoxW(Text, Title, MB_YESNO | MB_ICONQUESTION) != IDYES)
            return PSNRET_INVALID;

        return PSNRET_NOERROR;
    }

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        HICON hIcon = (HICON)::LoadImageW(
            _AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCEW(IDI_CLEANMGR), IMAGE_ICON, 0, 0,
            LR_DEFAULTSIZE | LR_SHARED);
        SendDlgItemMessage(IDC_DISKICON, STM_SETICON, (WPARAM)hIcon);

        m_HandlerListControl = GetDlgItem(IDC_HANDLERLIST);
        RECT rc;
        m_HandlerListControl.GetClientRect(&rc);
        rc.right -= GetSystemMetrics(SM_CXVSCROLL);

        LV_COLUMN column = {};
        column.mask = LVCF_FMT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = rc.right * 80 / 100;
        ListView_InsertColumn(m_HandlerListControl, 0, &column);
        column.fmt = LVCFMT_RIGHT;
        column.cx = rc.right * 20 / 100;

        ListView_InsertColumn(m_HandlerListControl, 1, &column);
        HIMAGELIST hImagelist = ImageList_Create(16, 16, ILC_MASK | ILC_COLOR32, 1, 1);
        ListView_SetImageList(m_HandlerListControl, hImagelist, LVSIL_SMALL);

        ListView_SetExtendedListViewStyleEx(m_HandlerListControl, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);

        m_HandlerList->ForEach(
            [&](CCleanupHandler *current)
            {
                if (!current->ShowHandler)
                    return;

                LV_ITEM item = {};
                item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
                item.lParam = (LPARAM)current;
                item.pszText = (LPWSTR)current->wszDisplayName;
                item.iItem = ListView_GetItemCount(m_HandlerListControl);
                item.iImage = ImageList_AddIcon(hImagelist, current->hIcon);
                item.iItem = ListView_InsertItem(m_HandlerListControl, &item);
                ListView_SetCheckState(
                    m_HandlerListControl, item.iItem, !!(current->StateFlags & HANDLER_STATE_SELECTED));

                item.mask = LVIF_TEXT;
                WCHAR ByteSize[100] = {};
                StrFormatByteSizeW(current->SpaceUsed, ByteSize, _countof(ByteSize));
                ListView_SetItemText(m_HandlerListControl, item.iItem, 1, ByteSize);
            });

        // Now we should start responding to changes
        m_IgnoreChanges = false;

        // Select the first item
        ListView_SetItemState(m_HandlerListControl, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

        UpdateSpaceUsed();
        return TRUE;
    }

    CCleanupHandler* GetHandler(int Index)
    {
        LVITEMW item = {};
        item.iItem = Index;
        if (item.iItem >= 0)
        {
            item.mask = LVIF_PARAM;
            ListView_GetItem(m_HandlerListControl, &item);
            return (CCleanupHandler*)item.lParam;
        }
        return nullptr;
    }


    LRESULT OnDetails(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        CCleanupHandler *handler = GetHandler(ListView_GetNextItem(m_HandlerListControl, -1, LVIS_FOCUSED));
        if (handler)
        {
            handler->Handler->ShowProperties(m_hWnd);
        }
        return 0L;
    }

    LRESULT OnHandlerItemchanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
    {
        if (idCtrl == IDC_HANDLERLIST)
        {
            // We are still initializing, don't respond to changes just yet!
            if (m_IgnoreChanges)
                return 0L;

            LPNMLISTVIEW pnic = (LPNMLISTVIEW)pnmh;

            // We only care about state changes
            if (!(pnic->uChanged & LVIF_STATE))
                return 0L;


            INT ItemIndex = pnic->iItem;
            if (ItemIndex == -1 || ItemIndex >= ListView_GetItemCount(pnic->hdr.hwndFrom))
            {
                return 0L;
            }

            bool GotSelected = (pnic->uNewState & LVIS_SELECTED) && !(pnic->uOldState & LVIS_SELECTED);
            if (GotSelected)
            {
                CWindow DetailsButton = GetDlgItem(IDC_DETAILS);
                CCleanupHandler* handler = (CCleanupHandler*)pnic->lParam;

                SetDlgItemText(IDC_DESCRIPTION, handler->wszDescription ? handler->wszDescription : L"");
                if (handler->HasSettings())
                {
                    DetailsButton.ShowWindow(SW_SHOW);
                    DetailsButton.SetWindowText(handler->wszBtnText);
                }
                else
                {
                    DetailsButton.ShowWindow(SW_HIDE);
                }
            }

            int iOldState = STATEIMAGETOINDEX(pnic->uOldState);
            int iNewState = STATEIMAGETOINDEX(pnic->uNewState);

            if ((iOldState ^ iNewState) == (STATEIMAGE_UNCHECKED ^ STATEIMAGE_CHECKED))
            {
                CCleanupHandler* handler = (CCleanupHandler*)pnic->lParam;
                if (iNewState == STATEIMAGE_CHECKED)
                    handler->StateFlags |= HANDLER_STATE_SELECTED;
                else
                    handler->StateFlags &= ~HANDLER_STATE_SELECTED;
                UpdateSpaceUsed();
            }
        }
        return 0L;
    }

    void UpdateSpaceUsed()
    {
        CStringW tmp;
        WCHAR ByteSize[100];
        StrFormatByteSizeW(m_TotalSpaceUsed, ByteSize, _countof(ByteSize));

        tmp.Format(IDS_TOTAL_CLEANABLE_CAPTION, ByteSize, m_Drive);
        SetDlgItemText(IDC_TOTAL_CLEANABLE, tmp);

        DWORDLONG SelectedGained = 0;

        m_HandlerList->ForEach(
            [&](CCleanupHandler *current)
            {
                if (current->StateFlags & HANDLER_STATE_SELECTED)
                {
                    SelectedGained += current->SpaceUsed;
                }
            });

        StrFormatByteSizeW(SelectedGained, ByteSize, _countof(ByteSize));
        SetDlgItemText(IDC_SELECTED_GAINED, ByteSize);
    }

    BEGIN_MSG_MAP(CCleanMgrProperties)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDC_DETAILS, OnDetails)
        NOTIFY_HANDLER(IDC_HANDLERLIST, LVN_ITEMCHANGED, OnHandlerItemchanged)
        CHAIN_MSG_MAP(CPropertyPageImpl<CCleanMgrProperties>)  // Allow the default handler to call 'OnApply' etc
    END_MSG_MAP()
};



class CCleanMgrModule : public ATL::CAtlExeModuleT< CCleanMgrModule >
{
public:
    WCHAR m_Drive = UNICODE_NULL;

    bool ParseCommandLine(
        _In_z_ LPCTSTR lpCmdLine,
        _Out_ HRESULT* pnRetCode) throw()
    {
        int argc = 0;
        CLocalPtr<LPWSTR> argv(CommandLineToArgvW(lpCmdLine, &argc));

        for (int n = 1; n < argc; ++n)
        {
            if ((argv[n][0] == '/' || argv[n][0] == '-') && towlower(argv[n][1]) == 'd')
            {
                if (iswalpha(argv[n][2]))
                {
                    m_Drive = towupper(argv[n][2]);
                    continue;
                }
                if ((n + 1) < argc)
                {
                    m_Drive = towupper(argv[n + 1][0]);
                    ++n;
                    continue;
                }
            }
        }
        *pnRetCode = S_OK;
        return true;
    }

    static inline UINT GetWindowProcessId(_In_ HWND hWnd)
    {
        DWORD pid;
        return GetWindowThreadProcessId(hWnd, &pid) ? pid : 0;
    }

    static BOOL CALLBACK EnumSingleInstanceCallback(_In_ HWND hWnd, _In_ LPARAM lParam)
    {
        if (::IsWindowVisible(hWnd) && (LPARAM)GetWindowProcessId(hWnd) == lParam)
        {
            ::SetForegroundWindow(hWnd);
            return FALSE;
        }
        return TRUE;
    }

    HRESULT Run(_In_ int nShowCmd) throw()
    {
        if (m_Drive == UNICODE_NULL)
        {
            SelectDrive(m_Drive);
        }

        if (m_Drive == UNICODE_NULL)
            return E_FAIL;

        CStringW Title;
        Title.Format(IDS_PROPERTIES_MAIN_TITLE, m_Drive);

        HWND hWndInstance = ::CreateWindowExW(WS_EX_TOOLWINDOW, WC_STATIC, Title, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
        for (HWND hNext = NULL, hFind; (hFind = ::FindWindowExW(NULL, hNext, WC_STATIC, Title)) != NULL; hNext = hFind)
        {
            if (hFind != hWndInstance)
            {
                ::EnumWindows(EnumSingleInstanceCallback, GetWindowProcessId(hFind));
                return S_FALSE;
            }
        }

        CCleanupHandlerList Handlers;
        CEmptyVolumeCacheCallBack CacheCallBack;

        Handlers.LoadHandlers(m_Drive);
        DWORDLONG TotalSpaceUsed = Handlers.ScanDrive(&CacheCallBack);

        CCleanMgrProperties cleanMgr(m_Drive, TotalSpaceUsed, &Handlers);
        HPROPSHEETPAGE hpsp[1] = { cleanMgr.Create() };

        PROPSHEETHEADERW psh = {  };
        psh.dwSize = sizeof(psh);
        psh.dwFlags = PSH_NOAPPLYNOW | PSH_USEICONID | PSH_NOCONTEXTHELP;
        psh.hInstance = _AtlBaseModule.GetResourceInstance();
        psh.pszIcon = MAKEINTRESOURCEW(IDI_CLEANMGR);
        psh.pszCaption = Title;
        psh.nPages = _countof(hpsp);
        psh.phpage = hpsp;

        if (PropertySheetW(&psh) >= 1)
        {
            ::DestroyWindow(hWndInstance); // Allow new "cleanmgr /D" without waiting for these handlers
            Handlers.ExecuteCleanup(&CacheCallBack);
        }
        else
        {
            ::DestroyWindow(hWndInstance);
        }
        return S_OK;
    }
};

CCleanMgrModule _AtlModule;



extern "C" int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/,
								LPWSTR /*lpCmdLine*/, int nShowCmd)
{
    return _AtlModule.WinMain(nShowCmd);
}
