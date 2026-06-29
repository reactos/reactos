/*
 * PROJECT:     ReactOS Certificate Manager
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:
 * COPYRIGHT:   Copyright 2025 Mark Jansen <mark.jansen@reactos.org>
 */

#include "certmgr.h"

struct CCertMgrProperties : public CDialogImpl<CCertMgrProperties>
{
    enum
    {
        IDD = IDD_CERTMGR_MAIN
    };
    CTreeView m_StoreTree;
    CListView m_CertList;
    CSplitter m_Splitter;
    CSortableHeader m_SortableHeader;

    CStoreList m_UserStoreList;
    CStoreList m_ComputerStoreList;

    CCertMgrProperties() : m_UserStoreList(StoreType::User), m_ComputerStoreList(StoreType::Computer)
    {

    }

    LRESULT
    OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        HICON hIcon = (HICON)::LoadImageW(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCEW(IDI_CERTMGR), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
        SetIcon(hIcon, TRUE);
        SetIcon(hIcon, FALSE);

        m_StoreTree.Attach(GetDlgItem(IDC_STORE_TREE));
        m_CertList.Attach(GetDlgItem(IDC_CERT_LIST));
        m_Splitter.Init(m_hWnd, m_StoreTree.m_hWnd, m_CertList.m_hWnd);

        ListView_SetExtendedListViewStyleEx(m_CertList, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

        auto certRoot = m_StoreTree.AddItem(NULL, (LPWSTR)L"Certificates", 0, 0, 0);
        auto userRoot = m_StoreTree.AddItem(certRoot, (LPWSTR)L"User Store", 0, 0, 0);
        m_UserStoreList.ForEach(
            [&](const CStore *current)
            {m_StoreTree.AddItem(userRoot, (LPWSTR)current->GetStoreName(), 0, 0, (LPARAM)current); });
        auto computerRoot = m_StoreTree.AddItem(certRoot, (LPWSTR)L"Computer Store", 0, 0, 0);
        m_ComputerStoreList.ForEach(
            [&](const CStore *current)
            { m_StoreTree.AddItem(computerRoot, (LPWSTR)current->GetStoreName(), 0, 0, (LPARAM)current); });

        m_StoreTree.Expand(certRoot, TVE_EXPAND);

        m_CertList.InsertColumn(0, (LPWSTR)L"Issued To", LVCFMT_LEFT, 200);
        m_CertList.InsertColumn(1, (LPWSTR)L"Issued By", LVCFMT_LEFT, 200);
        m_CertList.InsertColumn(2, (LPWSTR)L"Expiration Date", LVCFMT_LEFT, 100);
        m_CertList.InsertColumn(3, (LPWSTR)L"Friendly Name", LVCFMT_LEFT, 200);

        return TRUE;
    }

    LRESULT
    OnEndDialog(WORD, WORD wID, HWND, BOOL &)
    {
        EndDialog(wID);
        return 0L;
    }

    LRESULT
    OnStoreChanged(int idCtrl, LPNMHDR pnmh, BOOL &bHandled)
    {
        if (idCtrl == IDC_STORE_TREE)
        {
            LPNMTREEVIEW pntv = (LPNMTREEVIEW)pnmh;
            CStore *store = (CStore *)pntv->itemNew.lParam;
            if (store)
            {
                m_CertList.DeleteAllItems();
                store->Expand();
                store->ForEach(
                    [&](const CCert *current)
                    {
                        LV_ITEM item = {};
                        item.mask = LVIF_TEXT | LVIF_PARAM;
                        item.lParam = (LPARAM)current;
                        item.pszText = (LPWSTR)(LPCWSTR)current->GetSubjectName();
                        item.iItem = m_CertList.GetItemCount();
                        item.iItem = m_CertList.InsertItem(&item);
                        m_CertList.SetItemText(item.iItem, 1, (LPCWSTR)current->GetIssuerName());
                        FILETIME notAfter = current->GetNotAfter();
                        SYSTEMTIME st;
                        FileTimeToSystemTime(&notAfter, &st);
                        WCHAR dateStr[100];
                        GetDateFormatW(LOCALE_USER_DEFAULT, 0, &st, NULL, dateStr, _countof(dateStr));
                        m_CertList.SetItemText(item.iItem, 2, dateStr);
                        m_CertList.SetItemText(item.iItem, 3, (LPCWSTR)current->GetFriendlyName());
                    });
                m_SortableHeader.ReapplySorting(m_CertList);
            }
        }
        return 0L;
    }

    LRESULT
    OnCertDoubleClicked(int idCtrl, LPNMHDR pnmh, BOOL &bHandled)
    {
        if (idCtrl == IDC_CERT_LIST)
        {
            auto lpnmitem = (LPNMITEMACTIVATE)pnmh;
            if (lpnmitem->iItem < 0)
                return 0L;

            const CCert *current = (const CCert *)m_CertList.GetItemData(lpnmitem->iItem);

            CRYPTUI_VIEWCERTIFICATE_STRUCTW CertViewInfo = {0};
            CertViewInfo.dwSize = sizeof(CertViewInfo);
            CertViewInfo.pCertContext = current->GetCertContext();
            CertViewInfo.hwndParent = m_hWnd;
            CryptUIDlgViewCertificateW(&CertViewInfo, NULL);
        }
        return 0L;
    }

    LRESULT
    OnHeaderClicked(int idCtrl, LPNMHDR pnmh, BOOL &bHandled)
    {
        auto pnmv = (LPNMLISTVIEW)pnmh;
        m_SortableHeader.ColumnClick(m_CertList, pnmv);
        return 0L;
    }

    BEGIN_MSG_MAP(CCertMgrProperties)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDCANCEL, OnEndDialog)
    NOTIFY_HANDLER(IDC_STORE_TREE, TVN_SELCHANGED, OnStoreChanged)
    NOTIFY_HANDLER(IDC_CERT_LIST, NM_DBLCLK, OnCertDoubleClicked)
    NOTIFY_HANDLER(IDC_CERT_LIST, LVN_COLUMNCLICK, OnHeaderClicked)

    MESSAGE_HANDLER(WM_SIZE, m_Splitter.OnSize)
    MESSAGE_HANDLER(WM_SETCURSOR, m_Splitter.OnSetCursor)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, m_Splitter.OnLButtonDown)
    MESSAGE_HANDLER(WM_MOUSEMOVE, m_Splitter.OnMouseMove)
    MESSAGE_HANDLER(WM_LBUTTONUP, m_Splitter.OnLButtonUp)
    END_MSG_MAP()
};

class CCertMgrModule : public ATL::CAtlExeModuleT<CCertMgrModule>
{
  public:
    HRESULT
    Run(_In_ int nShowCmd) throw()
    {
        CStringW Title(MAKEINTRESOURCEW(IDS_PROPERTIES_MAIN_TITLE));
        CCertMgrProperties certMgr;
        certMgr.DoModal();
        return S_OK;
    }
};

CCertMgrModule _AtlModule;

extern "C" int WINAPI
wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPWSTR /*lpCmdLine*/, int nShowCmd)
{
    return _AtlModule.WinMain(nShowCmd);
}
