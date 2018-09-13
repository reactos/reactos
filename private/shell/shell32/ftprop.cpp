#include "shellprv.h"
#include "ids.h"
#include "help.h"

#include "ascstr.h"
#include "ftcmmn.h"

#include "ftprop.h"
#include "ftedit.h"
#include "ftadv.h"

#define SUBITEM_EXT         0
#define SUBITEM_PROGIDDESCR 1

#define MAX_PROIDDESCR MAX_PATH

#define WM_FINISHFILLLISTVIEW (WM_USER + 1)

static DWORD s_rgdwHelpIDsArray[] =
{  // Context Help IDs
    IDC_NO_HELP_1,                NO_HELP,
    IDC_FT_PROP_LV_FILETYPES,     IDH_FCAB_FT_PROP_LV_FILETYPES,
    IDC_FT_PROP_ANIM,             IDH_FCAB_FT_PROP_LV_FILETYPES,
    IDC_FT_PROP_NEW,              IDH_FCAB_FT_PROP_NEW,
    IDC_FT_PROP_OPENEXE_TXT,      IDH_FCAB_FT_PROP_DETAILS,
    IDC_FT_PROP_OPENICON,         IDH_FCAB_FT_PROP_DETAILS,
    IDC_FT_PROP_OPENEXE,          IDH_FCAB_FT_PROP_DETAILS,
    IDC_FT_PROP_CHANGEOPENSWITH,  IDH_FPROP_GEN_CHANGE,
    IDC_FT_PROP_TYPEOFFILE_TXT,   IDH_FCAB_FT_PROP_DETAILS,
    IDC_FT_PROP_EDITTYPEOFFILE,   IDH_FCAB_FT_PROP_EDIT,
    IDC_FT_PROP_GROUPBOX,         IDH_FCAB_FT_PROP_DETAILS,
    IDC_FT_PROP_REMOVE,           IDH_FCAB_FT_PROP_REMOVE,
    0, 0
};

CFTPropDlg::CFTPropDlg(BOOL fAutoDelete) :
    CFTDlg((ULONG_PTR)s_rgdwHelpIDsArray, fAutoDelete), _iLVSel(-1),
    _fStopThread(FALSE)
{
}

CFTPropDlg::~CFTPropDlg()
{
}

LRESULT CFTPropDlg::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
    HRESULT hres = _InitAssocStore();

    if (SUCCEEDED(hres))
        hres = _InitListView();

    if (SUCCEEDED(hres))
        _InitPreFillListView();
    
    if (SUCCEEDED(hres))
    {
        DWORD dwThreadID = 0;

        _hThread = CreateThread(NULL, 0, _FillListViewWrapper, (LPVOID)this, 0,
            &dwThreadID);
    }

    return TRUE;
}

LRESULT CFTPropDlg::OnFinishInitDialog()
{
    HRESULT hres = E_FAIL;

    ASSERT(sizeof(HRESULT) == sizeof(DWORD));

    if (!GetExitCodeThread(_hThread, (DWORD*)&hres))
    {
        hres = E_FAIL;
    }
 
    CloseHandle(_hThread);

    _hThread = NULL;

    if (SUCCEEDED(hres))
    {
        _InitPostFillListView();
        hres = _SelectListViewItem(0);
    }

    if (FAILED(hres))
    {
        if (E_OUTOFMEMORY == hres)
        {
            ShellMessageBox(g_hinst, _hwnd, MAKEINTRESOURCE(IDS_ERROR + 
                ERROR_NOT_ENOUGH_MEMORY), MAKEINTRESOURCE(IDS_FT), 
                MB_OK | MB_ICONSTOP);
        }

        EndDialog(_hwnd, -1);
    }
    else
    {
        DWORD dwThreadID = 0;

        _hThread = CreateThread(NULL, 0, _UpdateAllListViewItemImagesWrapper,
            (LPVOID)this, 0, &dwThreadID);
    }

    return TRUE;
}

LRESULT CFTPropDlg::OnCtlColorStatic(WPARAM wParam, LPARAM lParam)
{
    LRESULT fRet = FALSE;
    // This is to set the color of the background of the animate control
    // see doc on ACS_TRANSPARENT and WM_CTLCOLORSTATIC
    if ((HWND)lParam == GetDlgItem(_hwnd, IDC_FT_PROP_ANIM))
    {
        SetBkColor(GET_WM_CTLCOLOR_HDC(wParam, lParam, WM_CTLCOLORSTATIC), GetSysColor(COLOR_WINDOW));
        fRet = (LRESULT)GetSysColorBrush(COLOR_WINDOW);
    }

    return fRet;
}

//static
DWORD WINAPI CFTPropDlg::_FillListViewWrapper(LPVOID lpParameter)
{
    return ((CFTPropDlg*)lpParameter)->_FillListView();
}

//static
DWORD WINAPI CFTPropDlg::_UpdateAllListViewItemImagesWrapper(LPVOID lpParameter)
{
    return ((CFTPropDlg*)lpParameter)->_UpdateAllListViewItemImages();
}

LRESULT CFTPropDlg::OnDestroy(WPARAM wParam, LPARAM lParam)
{
    DWORD dwRet = FALSE;
    int iCount = 0;
    LVITEM lvItem = {0};
    HWND hwndLV = _GetLVHWND();

    _fStopThread = TRUE;

    HICON hIconOld = (HICON)SendDlgItemMessage(_hwnd, IDC_FT_PROP_OPENICON, STM_GETIMAGE, IMAGE_ICON,
        (LPARAM)0);

    if (hIconOld)
        DeleteObject(hIconOld);

    // go through all the items in the listview and delete the strings dynamically
    // allocated for progIDs
    lvItem.mask = LVIF_PARAM;
    lvItem.iSubItem = SUBITEM_EXT;

    iCount = ListView_GetItemCount(hwndLV);

    for (lvItem.iItem = 0; lvItem.iItem < iCount; ++lvItem.iItem)
    {
        ListView_GetItem(hwndLV, &lvItem);

        if (lvItem.lParam)
        {
            LocalFree((HLOCAL)lvItem.lParam);
        }
    }

    do
    {
        MSG msg;

        //Get and process the messages!
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // MsgWaitForMultipleObjects can fail with -1 being returned!
        dwRet = MsgWaitForMultipleObjects(1, &_hThread, FALSE, 10 * 1000, QS_ALLINPUT);
    }
    while ((WAIT_OBJECT_0 != dwRet) && (WAIT_TIMEOUT != dwRet) && (-1 != dwRet));

    // Did soemthing bad happened (timed out or MsgWait... failed?
    if ((WAIT_TIMEOUT == dwRet) || (-1 == dwRet))
        // yes, kill the thread
        TerminateThread(_hThread, 0);

    CloseHandle(_hThread);
    _hThread = NULL;

    CFTDlg::OnDestroy(wParam, lParam);

    return TRUE;
}

struct LVCOMPAREINFO
{
    HWND    hwndLV;
    int     iCol;
};

int CALLBACK AlphaCompareItem(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    struct LVCOMPAREINFO   *plvci = (struct LVCOMPAREINFO *)lParamSort;
    TCHAR   sz1[MAX_PATH];
    TCHAR   sz2[MAX_PATH];

    ListView_GetItemText(plvci->hwndLV, lParam1, plvci->iCol, sz1, ARRAYSIZE(sz1));
    ListView_GetItemText(plvci->hwndLV, lParam2, plvci->iCol, sz2, ARRAYSIZE(sz2));

    return lstrcmpi(sz1, sz2);
}

LRESULT CFTPropDlg::OnListViewColumnClick(int iCol)
{
    struct LVCOMPAREINFO lvci;
    
    lvci.hwndLV = _GetLVHWND();
    lvci.iCol = iCol;

    _fUpdateImageAgain = TRUE;

    return SendMessage(_GetLVHWND(), LVM_SORTITEMSEX, (WPARAM)&lvci, (LPARAM)AlphaCompareItem);    
}

LRESULT CFTPropDlg::OnListViewSelItem(int iItem, LPARAM lParam)
{
    //
    // Need to update the lower pane of the dialog
    //
    // Get the extension
    TCHAR szExt[MAX_EXT];
    TCHAR szProgIDDescr[MAX_PROIDDESCR];
    LVITEM lvItem = {0};

    _iLVSel = iItem;

    lvItem.mask = LVIF_TEXT | LVIF_PARAM;
    lvItem.iItem = iItem;
    lvItem.iSubItem = SUBITEM_EXT;
    lvItem.pszText = szExt;
    lvItem.cchTextMax = ARRAYSIZE(szExt);

    ListView_GetItem(_GetLVHWND(), &lvItem);

    ListView_GetItemText(_GetLVHWND(), iItem, SUBITEM_PROGIDDESCR, szProgIDDescr,
        ARRAYSIZE(szProgIDDescr));

    _EnableLowerPane(TRUE);

    if (!lvItem.lParam)
    {
        _UpdateGroupBox(szExt, TRUE);
    }
    else
    {
        _UpdateGroupBox(szProgIDDescr, FALSE);
    }

    _UpdateProgIDButtons(szExt, (LPTSTR)lvItem.lParam);

    // We rely on this being after _UpdateProgIDButtons (see _fPerUserAdvButton)
    _UpdateDeleteButton(lvItem.lParam ? FALSE : TRUE);
    _UpdateAdvancedText(szExt, szProgIDDescr, lvItem.lParam ? FALSE : TRUE);

    _UpdateOpensWith(szExt, (LPTSTR)lvItem.lParam);

    return FALSE;
}

HRESULT CFTPropDlg::_UpdateDeleteButton(BOOL fExt)
{
    BOOL fTrue = _ShouldEnableButtons();

    EnableWindow(GetDlgItem(_hwnd, IDC_FT_PROP_REMOVE),
        (_fPerUserAdvButton || !fExt) ? FALSE : fTrue);

    return S_OK;
}

HRESULT CFTPropDlg::_UpdateProgIDButtons(LPTSTR pszExt, LPTSTR pszProgID)
{
    HRESULT hres = E_FAIL;

    if (pszExt && *pszExt)
    {
        TCHAR szButtonText[50];
        HWND hwndAdvButton = GetDlgItem(_hwnd, IDC_FT_PROP_EDITTYPEOFFILE);

        _SetAdvancedRestoreButtonHelpID(IDH_FCAB_FT_PROP_EDIT);

        // Is this a progID only association?
        if (!pszProgID)
        {
            // No
            IAssocInfo* pAI;

            hres = _pAssocStore->GetAssocInfo(pszExt, AIINIT_EXT, &pAI);

            if (SUCCEEDED(hres))
            {
                hres = pAI->GetBOOL(AIBOOL_PERUSERINFOAVAILABLE, &_fPerUserAdvButton);

                ASSERT(SUCCEEDED(hres) || (FAILED(hres) && (FALSE == _fPerUserAdvButton)));

                if (_fPerUserAdvButton)
                {
                    // Restore mode
                    LoadString(g_hinst, IDS_FT_PROP_BTN_RESTORE, szButtonText, ARRAYSIZE(szButtonText));

                    _SetAdvancedRestoreButtonHelpID(IDH_FCAB_FT_PROP_EDIT_RESTORE);
                }
                else
                {
                    TCHAR szProgID[MAX_PROGID];
                    DWORD cchProgID = ARRAYSIZE(szProgID);

                    hres = pAI->GetString(AISTR_PROGID, szProgID, &cchProgID);

                    LoadString(g_hinst, IDS_FT_PROP_BTN_ADVANCED, szButtonText, ARRAYSIZE(szButtonText));

                    if (SUCCEEDED(hres))
                    {
                        IAssocInfo * pAIProgID;

                        hres = _pAssocStore->GetAssocInfo(szProgID, AIINIT_PROGID, &pAIProgID);

                        if (SUCCEEDED(hres))
                        {
                            BOOL fEdit = _ShouldEnableButtons();

                            if (fEdit)
                            {
                                pAIProgID->GetBOOL(AIBOOL_EDIT, &fEdit);
                            }

                            EnableWindow(hwndAdvButton, fEdit);

                            pAIProgID->Release();
                        }
                    }
                }

                pAI->Release();
            }
        }
        else
        {
            // Yes
            IAssocInfo* pAIProgID;

            LoadString(g_hinst, IDS_FT_PROP_BTN_ADVANCED, szButtonText, ARRAYSIZE(szButtonText));

            hres = _pAssocStore->GetAssocInfo(pszProgID, AIINIT_PROGID, &pAIProgID);

            if (SUCCEEDED(hres))
            {
                BOOL fEdit = _ShouldEnableButtons();

                if (fEdit)
                {
                    pAIProgID->GetBOOL(AIBOOL_EDIT, &fEdit);
                }

                EnableWindow(hwndAdvButton, fEdit);

                pAIProgID->Release();
            }

            EnableWindow(GetDlgItem(_hwnd, IDC_FT_PROP_CHANGEOPENSWITH), FALSE);
        }

        SetWindowText(hwndAdvButton, szButtonText);
    }

    return hres;
}

LRESULT CFTPropDlg::OnDeleteButton(WORD wNotif)
{
    // Warn user about the evil consequences of his act
    if (ShellMessageBox(g_hinst, _hwnd, MAKEINTRESOURCE(IDS_FT_MB_REMOVETYPE),
        MAKEINTRESOURCE(IDS_FT), MB_YESNO | MB_ICONQUESTION) == IDYES)
    {
        LVITEM lvItem = {0};
        TCHAR szExt[MAX_EXT];

        // Set stuff
        lvItem.iSubItem = SUBITEM_EXT;
        lvItem.pszText = szExt;
        lvItem.cchTextMax = ARRAYSIZE(szExt);

        if (_GetListViewSelectedItem(LVIF_TEXT | LVIF_IMAGE, 0, &lvItem))
        {
            HRESULT hres;
            IAssocInfo* pAI;

            hres = _pAssocStore->GetAssocInfo(szExt, AIINIT_EXT, &pAI);

            if (SUCCEEDED(hres))
            {
                hres = pAI->Delete(AIALL_NONE);

                if (SUCCEEDED(hres))
                {
                    _DeleteListViewItem(lvItem.iItem);

                    PropSheet_CancelToClose(GetParent(_hwnd));
                }

                pAI->Release();
            }
        }
    }

    return FALSE;
}

LRESULT CFTPropDlg::OnNewButton(WORD wNotif)
{
    FTEDITPARAM ftEditParam;
    CFTEditDlg* pEditDlg = NULL;

    // Fill structure
    ftEditParam.dwExt = ARRAYSIZE(ftEditParam.szExt);
    ftEditParam.dwProgIDDescr = ARRAYSIZE(ftEditParam.szProgIDDescr);

    // This one should be one way, it will come back with a value
    *ftEditParam.szProgID = 0;
    ftEditParam.dwProgID = ARRAYSIZE(ftEditParam.szProgID);

    pEditDlg = new CFTEditDlg(&ftEditParam);

    if (pEditDlg)
    {
        if (IDOK == pEditDlg->DoModal(g_hinst, MAKEINTRESOURCE(DLG_FILETYPEOPTIONSEDITNEW),
                        _hwnd))
        {
            HWND hwndLV = _GetLVHWND();
            LRESULT lRes = 0;
            int iIndex = -1;
            HRESULT hres = E_FAIL;
            IAssocInfo* pAI = NULL;
            LVFINDINFO lvFindInfo = {0};
            LPTSTR pszExtNoDot = NULL;
            LVITEM lvItem = {0};
            TCHAR szExt[MAX_EXT];

            lvItem.pszText = szExt;
            lvItem.cchTextMax = ARRAYSIZE(szExt);

            pszExtNoDot = (TEXT('.') != *(ftEditParam.szExt)) ? ftEditParam.szExt :
                ftEditParam.szExt + 1;

            lvFindInfo.flags = LVFI_STRING;
            lvFindInfo.psz = pszExtNoDot;

            iIndex = ListView_FindItem(hwndLV, -1, &lvFindInfo);

            // Is this a brand new Ext-ProgID association?
            if (-1 == iIndex)
            {
                // Yes, Insert a new item
                SetWindowRedraw(hwndLV, FALSE);
            
                // Add new ext-progID association
                hres = _pAssocStore->GetAssocInfo(ftEditParam.szExt, AIINIT_EXT, &pAI);

                if (SUCCEEDED(hres))
                {
                    TCHAR szProgIDDescr[MAX_PROGIDDESCR];
                    DWORD cchProgIDDescr = ARRAYSIZE(szProgIDDescr);

                    hres = pAI->GetString(AISTR_PROGIDDESCR, szProgIDDescr, &cchProgIDDescr);

                    if (FAILED(hres) || !*szProgIDDescr)
                    {
                        MakeDefaultProgIDDescrFromExt(szProgIDDescr, ARRAYSIZE(szProgIDDescr), pszExtNoDot);
                    }

                    // Add to the listview
                    iIndex = _InsertListViewItem(0, pszExtNoDot, szProgIDDescr);
                    pAI->Release();
                }

                // Select newly inserted item
                if (-1 != iIndex)
                {
                    _SelectListViewItem(iIndex);
                }

                // Redraw our list
                SetWindowRedraw(hwndLV, TRUE);

                _GetListViewSelectedItem(LVIF_PARAM | LVIF_TEXT, 0, &lvItem);

                lvItem.mask = LVIF_PARAM | LVIF_TEXT | LVIF_IMAGE;
            }
            else
            {
                // No just update the item
                lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
                lvItem.iItem = iIndex;

                ListView_GetItem(hwndLV, &lvItem);
            }

            _UpdateListViewItem(&lvItem);

            PropSheet_CancelToClose(GetParent(_hwnd));
        }

        delete pEditDlg;
    }

    return FALSE;
}

LRESULT CFTPropDlg::OnAdvancedButton(WORD wNotif)
{
    LVITEM lvItem = {0};
    TCHAR szExt[MAX_EXT];

    // Set stuff
    lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    lvItem.iSubItem = SUBITEM_EXT;
    lvItem.pszText = szExt;
    lvItem.cchTextMax = ARRAYSIZE(szExt);

    if (_GetListViewSelectedItem(LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM, 0, &lvItem))
    {
        HRESULT hres;
        IAssocInfo* pAI;

        if (_fPerUserAdvButton)
        {
            // Restore mode
            hres = _pAssocStore->GetAssocInfo(szExt, AIINIT_EXT, &pAI);

            if (SUCCEEDED(hres))
            {
                hres = pAI->Delete(AIALL_PERUSER);

                _UpdateListViewItem(&lvItem);

                OnListViewSelItem(lvItem.iItem, (LPARAM)NULL);

                pAI->Release();

                PropSheet_CancelToClose(GetParent(_hwnd));
            }
        }
        else
        {
            // we might deal with an ext-progid assoc or only a progID
            TCHAR szProgID[MAX_PROGID];

            // Is this a progID only?
            if (lvItem.lParam)
            {
                // Yes
                StrCpyN(szProgID, (LPTSTR)lvItem.lParam, ARRAYSIZE(szProgID));

                hres = S_OK;
            }
            else
            {
                // No
                DWORD cchProgID = ARRAYSIZE(szProgID);

                hres = _pAssocStore->GetAssocInfo(szExt, AIINIT_EXT, &pAI);

                if (SUCCEEDED(hres))
                {
                    hres = THR(pAI->GetString(AISTR_PROGID, szProgID, &cchProgID));

                    pAI->Release();
                }
            }

            if (SUCCEEDED(hres))
            {
                CFTAdvDlg* pAdvDlg = new CFTAdvDlg(szProgID, FALSE);

                if (pAdvDlg)
                {
                    if (IDOK == pAdvDlg->DoModal(g_hinst, MAKEINTRESOURCE(DLG_FILETYPEOPTIONSEDIT), _hwnd))
                    {
                        _UpdateListViewItem(&lvItem);

                        OnListViewSelItem(lvItem.iItem, (LPARAM)NULL);

                        PropSheet_CancelToClose(GetParent(_hwnd));
                    }

                    delete pAdvDlg;
                }
            }
        }
    }

    return FALSE;
}

LRESULT CFTPropDlg::OnChangeButton(WORD wNotif)
{
    // Bring up the "Open With" dialog
    LVITEM lvItem = {0};
    TCHAR szExt[MAX_EXT];

    // Set stuff
    lvItem.mask = LVIF_TEXT | LVIF_IMAGE;
    lvItem.iSubItem = SUBITEM_EXT;
    lvItem.pszText = szExt;
    lvItem.cchTextMax = ARRAYSIZE(szExt);

    if (_GetListViewSelectedItem(LVIF_TEXT, 0, &lvItem))
    {
        TCHAR szDotExt[MAX_EXT];
        OPENASINFO oai;

        *szDotExt = TEXT('.');
        StrCpyN(szDotExt + 1, szExt, ARRAYSIZE(szDotExt) - 1);

        oai.pcszFile = szDotExt;
        oai.pcszClass = NULL;
        oai.dwInFlags = (OAIF_REGISTER_EXT | OAIF_FORCE_REGISTRATION); // we want the association to be made

        if (SUCCEEDED(OpenAsDialog(GetParent(_hwnd), &oai)))
        {
            // we changed the association so update the "Opens with:" text
            _UpdateOpensWith(szExt, NULL);

            // we don't need LVIF_PARAM since we enable the Change button only for Ext-ProgID asssoc
            lvItem.mask = LVIF_TEXT | LVIF_IMAGE;

            _UpdateListViewItem(&lvItem);

            OnListViewSelItem(lvItem.iItem, (LPARAM)NULL);

            PropSheet_CancelToClose(GetParent(_hwnd));
        }
    }

    return FALSE;
}

HRESULT CFTPropDlg::_UpdateGroupBox(LPTSTR pszText, BOOL fExt)
{
    HRESULT hres = E_OUTOFMEMORY;
    LPTSTR psz = NULL;

    if (fExt)
    {
        psz = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(IDS_FT_PROP_DETAILSFOR), pszText);
    }
    else
    {
        psz = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(IDS_FT_PROP_DETAILSFORPROGID), pszText);
    }

    if (psz)
    {
        SetDlgItemText(_hwnd, IDC_FT_PROP_GROUPBOX, psz);
        LocalFree(psz);
        hres = S_OK;
    }

    return hres;
}

HRESULT CFTPropDlg::_UpdateOpensWith(LPTSTR pszExt, LPTSTR pszProgID)
{
    HICON hIconOld = NULL;

    if (!pszProgID)
    {
        IAssocInfo* pAI = NULL;

        HRESULT hres = _pAssocStore->GetAssocInfo(pszExt, AIINIT_EXT, &pAI);

        if (SUCCEEDED(hres))
        {
            TCHAR szAppFriendlyName[MAX_APPFRIENDLYNAME];
            DWORD dwAppFriendlyName = ARRAYSIZE(szAppFriendlyName);

            hres = pAI->GetString(AISTR_APPFRIENDLY, szAppFriendlyName, &dwAppFriendlyName);

            if (SUCCEEDED(hres))
            {
                HICON hIcon = NULL;
                int iIcon;
                SetDlgItemText(_hwnd, IDC_FT_PROP_OPENEXE, szAppFriendlyName);

                hres = pAI->GetDWORD(AIDWORD_APPSMALLICON, (DWORD*)&iIcon);

                HIMAGELIST hIL = NULL;

                // BUGBUG: Why don't we just use _hImageList?  Or ListView_GetImageList()?
                Shell_GetImageLists(NULL, &hIL);

                if (hIL && SUCCEEDED(hres))
                {
                    hIcon = ImageList_ExtractIcon(g_hinst, hIL, iIcon);
                }

                hIconOld = (HICON)SendDlgItemMessage(_hwnd, IDC_FT_PROP_OPENICON, STM_SETIMAGE, IMAGE_ICON,
                    (LPARAM)hIcon);

                if (hIconOld)
                    DestroyIcon(hIconOld);
            }
            else
            {
                SetDlgItemText(_hwnd, IDC_FT_PROP_OPENEXE, TEXT(" "));

                hIconOld = (HICON)SendDlgItemMessage(_hwnd, IDC_FT_PROP_OPENICON, STM_SETIMAGE, IMAGE_ICON,
                    (LPARAM)NULL);

                if (hIconOld)
                    DestroyIcon(hIconOld);
            }

            pAI->Release();
        }
    }
    else
    {
        SetDlgItemText(_hwnd, IDC_FT_PROP_OPENEXE, TEXT(" "));

        hIconOld = (HICON)SendDlgItemMessage(_hwnd, IDC_FT_PROP_OPENICON, STM_SETIMAGE, IMAGE_ICON,
            (LPARAM)NULL);

        if (hIconOld)
            DestroyIcon(hIconOld);
    }
    
    return S_OK;
}

HRESULT CFTPropDlg::_UpdateAdvancedText(LPTSTR pszExt, LPTSTR pszFileType, BOOL fExt)
{
    HRESULT hres = S_OK;
    LPTSTR psz = NULL;

    if (_fPerUserAdvButton)
    {
        TCHAR szProgIDDescr[MAX_PROGIDDESCR];
        DWORD cchProgIDDescr = ARRAYSIZE(szProgIDDescr);
        IAssocInfo* pAI = NULL;

        // we need to show the previous progIDDescr
        hres = _pAssocStore->GetAssocInfo(pszExt, AIINIT_EXT, &pAI);

        if (SUCCEEDED(hres))
        {
            hres = pAI->GetString(AISTR_PROGIDDESCR, szProgIDDescr,
                    &cchProgIDDescr);

            if (SUCCEEDED(hres))
            {
                // Restore mode
                psz = ShellConstructMessageString(HINST_THISDLL,
                    MAKEINTRESOURCE(IDS_FT_PROP_RESTORE),
                    pszExt, szProgIDDescr);
            }

            pAI->Release();
        }
    }
    else
    {
        if (fExt)
        {
            psz = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(IDS_FT_PROP_ADVANCED),
                        pszExt, pszFileType, pszFileType);
        }
        else
        {
            psz = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(IDS_FT_PROP_ADVANCED_PROGID),
                        pszFileType);
        }
    }

    if (SUCCEEDED(hres))
    {
        if (psz)
        {
            SetDlgItemText(_hwnd, IDC_FT_PROP_TYPEOFFILE_TXT, psz);
            LocalFree(psz);
        }
        else
            hres = E_OUTOFMEMORY;
    }

    return hres;
}

HRESULT CFTPropDlg::_EnableLowerPane(BOOL fEnable)
{
    BOOL fTrue = _ShouldEnableButtons();

    EnableWindow(GetDlgItem(_hwnd, IDC_FT_PROP_OPENEXE_TXT), fEnable);
    EnableWindow(GetDlgItem(_hwnd, IDC_FT_PROP_OPENEXE), fEnable);
    EnableWindow(GetDlgItem(_hwnd, IDC_FT_PROP_TYPEOFFILE_TXT), fEnable);
    EnableWindow(GetDlgItem(_hwnd, IDC_FT_PROP_GROUPBOX ), fEnable);

    // if user is locked down then we do not enable the buttons
    if (!fTrue)
        fEnable = FALSE;

    EnableWindow(GetDlgItem(_hwnd, IDC_FT_PROP_CHANGEOPENSWITH), fEnable);
    EnableWindow(GetDlgItem(_hwnd, IDC_FT_PROP_EDITTYPEOFFILE), fEnable);

    return S_OK;
}

HRESULT CFTPropDlg::_InitPreFillListView()
{
    // Disable New and Delete
    EnableWindow(GetDlgItem(_hwnd, IDC_FT_PROP_NEW), FALSE);
    EnableWindow(GetDlgItem(_hwnd, IDC_FT_PROP_REMOVE), FALSE);

    _EnableLowerPane(FALSE);
    _UpdateGroupBox(TEXT(""), TRUE);

    // Hide the advanced text
    ShowWindow(GetDlgItem(_hwnd, IDC_FT_PROP_TYPEOFFILE_TXT), SW_HIDE);

    return S_OK;
}

HRESULT CFTPropDlg::_InitPostFillListView()
{
    BOOL fTrue = _ShouldEnableButtons();

    // Enable New and Delete
    EnableWindow(GetDlgItem(_hwnd, IDC_FT_PROP_NEW),  fTrue);
    EnableWindow(GetDlgItem(_hwnd, IDC_FT_PROP_REMOVE), fTrue);

    // Show the advanced text
    ShowWindow(GetDlgItem(_hwnd, IDC_FT_PROP_TYPEOFFILE_TXT), SW_SHOW);

    Animate_Stop(GetDlgItem(_hwnd, IDC_FT_PROP_ANIM));
    ShowWindow(GetDlgItem(_hwnd, IDC_FT_PROP_ANIM), SW_HIDE);
    ShowWindow(_GetLVHWND(), SW_SHOW);

    SetFocus(_GetLVHWND());

    return S_OK;
}

HRESULT CFTPropDlg::_InitListView()
{
    HRESULT hres = S_OK;
    LVCOLUMN lvColumn = {0};
    HWND hwndLV = _GetLVHWND();
    TCHAR szColumnTitle[40];
    UINT uFlags = ILC_MASK | ILC_SHARED;
    RECT rc = {0};
    int iWidth = 80;
    HWND hwndAni;

    //
    // Styles
    //
    ListView_SetExtendedListViewStyleEx(hwndLV, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

    //
    // Set the columns
    //
    lvColumn.mask = LVCF_TEXT|LVCF_SUBITEM|LVCF_WIDTH;

    // Extensions column
    LoadString(g_hinst, IDS_FT_PROP_EXTENSIONS, szColumnTitle, ARRAYSIZE(szColumnTitle));

    lvColumn.cx = 60;
    lvColumn.pszText = szColumnTitle;
    lvColumn.cchTextMax = lstrlen(szColumnTitle);
    lvColumn.iSubItem = SUBITEM_EXT;

    ListView_InsertColumn(hwndLV, SUBITEM_EXT, &lvColumn);

    // ProgIDs column
    LoadString(g_hinst, IDS_FT, szColumnTitle, ARRAYSIZE(szColumnTitle));

    lvColumn.cchTextMax = lstrlen(szColumnTitle);
    lvColumn.iSubItem = SUBITEM_PROGIDDESCR;
    ListView_InsertColumn(hwndLV, SUBITEM_PROGIDDESCR, &lvColumn);

    // Adjust columns width
    // we need to do it after inserting both col, cause the last column resizing 
    // is special cased in list view code.

    // Ext column
    ListView_SetColumnWidth(hwndLV, SUBITEM_EXT, LVSCW_AUTOSIZE_USEHEADER);
    iWidth = ListView_GetColumnWidth(hwndLV, SUBITEM_EXT);

    // File type column
    GetClientRect(hwndLV, &rc);
    ListView_SetColumnWidth(hwndLV, SUBITEM_PROGIDDESCR,
        rc.right - iWidth - GetSystemMetrics(SM_CXBORDER) - GetSystemMetrics(SM_CXVSCROLL));

    //
    // ImageList
    //
    Shell_GetImageLists(NULL, &_hImageList);

    if (_hImageList)
        ListView_SetImageList(hwndLV, _hImageList, LVSIL_SMALL);

    GetWindowRect(hwndLV, &rc);
    MapWindowPoints(NULL, _hwnd, (POINT*)&rc, 2);

    hwndAni = GetDlgItem(_hwnd, IDC_FT_PROP_ANIM);

    Animate_Open(hwndAni, MAKEINTRESOURCE(IDA_SEARCH)); // open the resource
    Animate_Play(hwndAni, 0, -1, -1);     // play from start to finish and repeat

    MoveWindow(hwndAni, rc.left, rc.top,
        rc.right - rc.left, rc.bottom - rc.top, TRUE);

    ShowWindow(hwndLV, SW_HIDE);

    ShowWindow(hwndAni, SW_SHOW);

    return hres;
}

HRESULT CFTPropDlg::_FillListView()
{
    // Data stuff
    IEnumAssocInfo* pEnum = NULL;
    HRESULT hres = E_FAIL;
    int iFirstNAItem = -1;
    HWND hwndLV = _GetLVHWND();
    int iItem = 0;
    TCHAR szNA[50];

    ASSERT(_pAssocStore);

    LoadString(g_hinst, IDS_FT_NA, szNA, ARRAYSIZE(szNA));

    SetWindowRedraw(hwndLV, FALSE);

    // Do the extension first
    hres = _pAssocStore->EnumAssocInfo(ASENUM_EXT |
        ASENUM_ASSOC_YES | ASENUM_NOEXCLUDED | ASENUM_NOEXPLORERSHELLACTION |
        ASENUM_NOEXE, NULL, AIINIT_NONE, &pEnum);

    if (SUCCEEDED(hres))
    {
        IAssocInfo* pAI = NULL;
    
        while (!_fStopThread && (S_OK == pEnum->Next(&pAI)))
        {
            TCHAR szExt[MAX_EXT];
            DWORD cchExt = ARRAYSIZE(szExt);

            hres = pAI->GetString(AISTR_EXT, szExt, &cchExt);

            if (SUCCEEDED(hres))
            {
                BOOL fPerUser = FALSE;
                TCHAR szProgIDDescr[MAX_PROGIDDESCR];
                DWORD cchProgIDDescr = ARRAYSIZE(szProgIDDescr);
                HRESULT hresTmp = E_FAIL;

                hresTmp = pAI->GetBOOL(AIBOOL_PERUSERINFOAVAILABLE, &fPerUser);

                ASSERT(SUCCEEDED(hresTmp) || (FAILED(hresTmp) && (FALSE == fPerUser)));

                if (!fPerUser)
                {
                    hresTmp = pAI->GetString(AISTR_PROGIDDESCR, szProgIDDescr,
                            &cchProgIDDescr);
                }
            
                if (fPerUser || FAILED(hresTmp) || !*szProgIDDescr)
                    MakeDefaultProgIDDescrFromExt(szProgIDDescr, ARRAYSIZE(szProgIDDescr), szExt);

                _InsertListViewItem(iItem, szExt, szProgIDDescr);

                // Check if this is where we need to insert the N/A item later
                if ((-1 == iFirstNAItem) && (lstrcmpi(szExt, szNA) > 0))
                {
                    iFirstNAItem = iItem;
                }

                ++iItem;
            }

            pAI->Release();

            hres = S_OK;
        }

        pEnum->Release();
        pEnum = NULL;
    }

    // Then do the ProgIDs
    hres = _pAssocStore->EnumAssocInfo(ASENUM_PROGID | ASENUM_SHOWONLY, NULL, 
        AIINIT_NONE, &pEnum);

    if (SUCCEEDED(hres))
    {
        IAssocInfo* pAI = NULL;
        int cNAItem = 0;

        while (!_fStopThread && (S_OK == pEnum->Next(&pAI)))
        {
            TCHAR szProgIDDescr[MAX_PROGIDDESCR];
            DWORD cchProgIDDescr = ARRAYSIZE(szProgIDDescr);

            hres = pAI->GetString(AISTR_PROGIDDESCR, szProgIDDescr, &cchProgIDDescr);

            if (SUCCEEDED(hres))
            {
                TCHAR szProgID[MAX_PROGID];
                DWORD cchProgID = ARRAYSIZE(szProgID);

                hres = pAI->GetString(AISTR_PROGID, szProgID, &cchProgID);

                if (SUCCEEDED(hres))
                {
                    // we need to sort the N/A items by the description since they all begin with "N/A"
                    int iNAItem;

                    if (!cNAItem)
                    {
                        iNAItem = iFirstNAItem;
                    }
                    else
                    {
                        iNAItem = _GetNextNAItemPos(iFirstNAItem, cNAItem, szProgIDDescr);
                    }

                    _InsertListViewItem(iNAItem, szNA, szProgIDDescr, szProgID);

                    ++cNAItem;
                }
            }

            pAI->Release();

            hres = S_OK;
        }

        pEnum->Release();
    }
    
    SetWindowRedraw(hwndLV, TRUE);

    PostMessage(_hwnd, WM_FINISHFILLLISTVIEW, 0, 0);

    return hres;
}

int CFTPropDlg::_GetNextNAItemPos(int iFirstNAItem, int cNAItem, LPTSTR pszProgIDDescr)
{
    LVITEM lvItem = {0};
    TCHAR szProgIDDescr[MAX_PROGIDDESCR];
    int iItem = iFirstNAItem;
    HWND hwndLV = _GetLVHWND();

    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = iItem;
    lvItem.iSubItem = SUBITEM_PROGIDDESCR;
    lvItem.pszText = szProgIDDescr;
    lvItem.cchTextMax = ARRAYSIZE(szProgIDDescr);

    while (iItem < (iFirstNAItem + cNAItem))
    {
        if (-1 != ListView_GetItem(hwndLV, &lvItem))
        {
            if (lstrcmpi(pszProgIDDescr, lvItem.pszText) >= 0)
            {
                ++iItem;
                lvItem.iItem = iItem;
            }
            else
            {
                break;
            }
        }
        else
        {
            AssertMsg(FALSE, TEXT("Invalid 'N/A' item in sorting fct"));
        }
    }

    return iItem;
}

DWORD CFTPropDlg::_UpdateAllListViewItemImages()
{
    HWND hwndLV = _GetLVHWND();
    int iCount = 0;
    LVITEM lvItem = {0};
    TCHAR szExt[MAX_EXT];
    HRESULT hres = E_FAIL;

    HRESULT hrInit = SHCoInitialize();
 
    lvItem.iSubItem = SUBITEM_EXT;
    lvItem.pszText = szExt;
    lvItem.cchTextMax = ARRAYSIZE(szExt);

    do
    {
        _fUpdateImageAgain = FALSE;

        iCount = ListView_GetItemCount(hwndLV);
    
        for (lvItem.iItem = 0; !_fStopThread && (lvItem.iItem < iCount);
            ++lvItem.iItem)
        {
            IAssocInfo* pAI = NULL;

            lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
            ListView_GetItem(hwndLV, &lvItem);

            if (!lvItem.lParam)
            {
                hres = _pAssocStore->GetAssocInfo(szExt, AIINIT_EXT, &pAI);

                if (SUCCEEDED(hres))
                {
                    BOOL fPerUser = FALSE;

                    hres = pAI->GetBOOL(AIBOOL_PERUSERINFOAVAILABLE, &fPerUser);

                    ASSERT(SUCCEEDED(hres) || (FAILED(hres) && (FALSE == fPerUser)));

                    if (fPerUser)
                        hres = pAI->GetDWORD(AIDWORD_DOCSMALLICON | AIALL_PERUSER, (DWORD*)&lvItem.iImage);
                    else
                        hres = pAI->GetDWORD(AIDWORD_DOCSMALLICON, (DWORD*)&lvItem.iImage);
                }
            }
            else
            {
                hres = _pAssocStore->GetAssocInfo((LPTSTR)lvItem.lParam, AIINIT_PROGID, &pAI);

                if (SUCCEEDED(hres))
                {
                    hres = pAI->GetDWORD(AIDWORD_DOCSMALLICON, (DWORD*)&lvItem.iImage);
                }
            }

            if (SUCCEEDED(hres))
            {
                lvItem.mask = LVIF_IMAGE;
                ListView_SetItem(hwndLV, &lvItem);
            }

            if (pAI)
                pAI->Release();
        }
    }
    while (_fUpdateImageAgain && !_fStopThread);

    SHCoUninitialize(hrInit);

    return (DWORD)_fStopThread;
}

void CFTPropDlg::_UpdateListViewItem(LVITEM* plvItem)
{
    HWND hwndLV = _GetLVHWND();
    LVITEM lvItem = *plvItem;

    // Need to:
    //  - update image
    //  - update progIDDescr

    if (!lvItem.lParam)
    {
        IAssocInfo* pAI = NULL;

        HRESULT hres = _pAssocStore->GetAssocInfo(lvItem.pszText, AIINIT_EXT, &pAI);

        if (SUCCEEDED(hres))
        {
            TCHAR szProgIDDescr[MAX_PROGIDDESCR];
            DWORD cchProgIDDescr = ARRAYSIZE(szProgIDDescr);
            HRESULT hresTmp = E_FAIL;

            SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
        
            // Icon
            BOOL fPerUser = FALSE;

            hres = pAI->GetBOOL(AIBOOL_PERUSERINFOAVAILABLE, &fPerUser);

            ASSERT(SUCCEEDED(hres) || (FAILED(hres) && (FALSE == fPerUser)));

            if (fPerUser)
                hres = pAI->GetDWORD(AIDWORD_DOCSMALLICON | AIALL_PERUSER, (DWORD*)&lvItem.iImage);
            else
                hres = pAI->GetDWORD(AIDWORD_DOCSMALLICON, (DWORD*)&lvItem.iImage);

            if (SUCCEEDED(hres))
            {
                lvItem.mask = LVIF_IMAGE;
                ListView_SetItem(hwndLV, &lvItem);
            }

            // ProgID Description
            if (!fPerUser)
            {
                hresTmp = pAI->GetString(AISTR_PROGIDDESCR, szProgIDDescr,
                        &cchProgIDDescr);
            }
    
            if (fPerUser || FAILED(hresTmp) || !*szProgIDDescr)
                MakeDefaultProgIDDescrFromExt(szProgIDDescr, ARRAYSIZE(szProgIDDescr), lvItem.pszText);

            if (SUCCEEDED(hres))
            {
                lvItem.mask = LVIF_TEXT;
                lvItem.iSubItem = SUBITEM_PROGIDDESCR;
                lvItem.pszText = szProgIDDescr;
                lvItem.cchTextMax = lstrlen(szProgIDDescr);

                ListView_SetItem(hwndLV, &lvItem);
            }

            ListView_RedrawItems(hwndLV, lvItem.iItem, lvItem.iItem);

            pAI->Release();
        }
    }
    else
    {
        IAssocInfo* pAI = NULL;

        HRESULT hres = _pAssocStore->GetAssocInfo((LPTSTR)lvItem.lParam, AIINIT_PROGID, &pAI);

        if (SUCCEEDED(hres))
        {
            TCHAR szProgIDDescr[MAX_PROGIDDESCR];
            DWORD cchProgIDDescr = ARRAYSIZE(szProgIDDescr);
            HRESULT hresTmp = E_FAIL;

            SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
        
            // Icon
            hres = pAI->GetDWORD(AIDWORD_DOCSMALLICON, (DWORD*)&lvItem.iImage);

            if (SUCCEEDED(hres))
            {
                lvItem.mask = LVIF_IMAGE;
                ListView_SetItem(hwndLV, &lvItem);
            }

            // ProgID Description
            pAI->GetString(AISTR_PROGIDDESCR, szProgIDDescr,
                    &cchProgIDDescr);

            if (SUCCEEDED(hres))
            {
                lvItem.mask = LVIF_TEXT;
                lvItem.iSubItem = SUBITEM_PROGIDDESCR;
                lvItem.pszText = szProgIDDescr;
                lvItem.cchTextMax = lstrlen(szProgIDDescr);

                ListView_SetItem(hwndLV, &lvItem);
            }
            ListView_RedrawItems(hwndLV, lvItem.iItem, lvItem.iItem);

            pAI->Release();
        }
    }
}

int CFTPropDlg::_InsertListViewItem(int iItem, LPTSTR pszExt, LPTSTR pszProgIDDescr,
                                    LPTSTR pszProgID)
{
    HWND hwndLV = _GetLVHWND();
    LVITEM lvItem = {0};
    lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;

    // Put generic icon

    lvItem.iImage = Shell_GetCachedImageIndex(TEXT("shell32.dll"), II_DOCNOASSOC, 0);

    CharUpper(pszExt);

    // Extension
    if (pszProgID)
    {
        lvItem.lParam = (LPARAM)LocalAlloc(LPTR, (lstrlen(pszProgID) + 1) * sizeof(TCHAR));

        lstrcpy((LPTSTR)lvItem.lParam, pszProgID);
    }
    else
    {
        lvItem.lParam = NULL;
    }

    lvItem.iItem = iItem;
    lvItem.iSubItem = SUBITEM_EXT;
    lvItem.pszText = pszExt;
    lvItem.cchTextMax = lstrlen(pszExt);

    lvItem.iItem = ListView_InsertItem(hwndLV, &lvItem);

    // ProgID Description
    lvItem.mask = LVIF_TEXT;
    lvItem.iSubItem = SUBITEM_PROGIDDESCR;
    lvItem.pszText = pszProgIDDescr;
    lvItem.cchTextMax = lstrlen(pszProgIDDescr);

    ListView_SetItem(hwndLV, &lvItem);

    return lvItem.iItem;
}

HRESULT CFTPropDlg::_SelectListViewItem(int i)
{
    LVITEM lvItem = {0};

    lvItem.iItem = i;
    lvItem.mask = LVIF_STATE;
    lvItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
    lvItem.state = LVIS_SELECTED | LVIS_FOCUSED;

    ListView_SetItem(_GetLVHWND(), &lvItem);
    ListView_EnsureVisible(_GetLVHWND(), i, FALSE);

    return S_OK;
}

HRESULT CFTPropDlg::_DeleteListViewItem(int i)
{
    HWND hwndLV = _GetLVHWND();
    int iCount = ListView_GetItemCount(hwndLV);
    int iNextSel = -1;        
    LVITEM lvItem = {0};

    lvItem.mask = LVIF_PARAM;
    lvItem.iItem = i;
    lvItem.iSubItem = SUBITEM_EXT;

    ListView_GetItem(hwndLV, &lvItem);

    if (lvItem.lParam)
    {
        LocalFree((HLOCAL)lvItem.lParam);
    }

    ListView_DeleteItem(hwndLV, i);

    if (iCount > i)
        iNextSel = i;
    else
        if (i > 0)
            iNextSel = i - 1;

    if (-1 != iNextSel)
        _SelectListViewItem(iNextSel);

    return S_OK;
}

BOOL CFTPropDlg::_ShouldEnableButtons()
{
    // if we have a locked down user, then we never enable the buttons
    BOOL fRet = TRUE;

    if (S_FALSE == _pAssocStore->CheckAccess())
    {
        fRet = FALSE;
    }

#ifdef WINNT
            // If the REST_NOFILEASSOCIATE is set (TRUE), 
            // then we want to NOT enable buttons.
    fRet &= !SHRestricted(REST_NOFILEASSOCIATE);
#endif

    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
// Misc
BOOL CFTPropDlg::_GetListViewSelectedItem(UINT uMask, UINT uStateMask, LVITEM* plvItem)
{
    BOOL fSel = FALSE;
    HWND hwndLV = _GetLVHWND();

    plvItem->mask = uMask | LVIF_STATE;
    plvItem->stateMask = uStateMask | LVIS_SELECTED;

    // Do we have the selection cached?
    if (-1 != _iLVSel)
    {
        // Yes, make sure it's valid
        plvItem->iItem = _iLVSel;

        ListView_GetItem(hwndLV, plvItem);

        if (plvItem->state & LVIS_SELECTED)
            fSel = TRUE;
    }
 
    // Cache was wrong
    if (!fSel)
    {
        int iCount = ListView_GetItemCount(hwndLV);

        for (int i=0; (i < iCount) && !fSel; ++i)
        {
            plvItem->iItem = i;
            ListView_GetItem(hwndLV, plvItem);

            if (plvItem->state & LVIS_SELECTED)
                fSel = TRUE;
        }

        if (fSel)
            _iLVSel = i;
    }

    return fSel;
}

HWND CFTPropDlg::_GetLVHWND()
{
    return GetDlgItem(_hwnd, IDC_FT_PROP_LV_FILETYPES);
}

void CFTPropDlg::_SetAdvancedRestoreButtonHelpID(DWORD dwID)
{
    for (int i = 0; i < ARRAYSIZE(s_rgdwHelpIDsArray); i += 2)
    {
        if (IDC_FT_PROP_EDITTYPEOFFILE == s_rgdwHelpIDsArray[i])
        {
            if (i + 1 < ARRAYSIZE(s_rgdwHelpIDsArray))
                s_rgdwHelpIDsArray[i + 1] = dwID;

            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Windows boiler plate code
LRESULT CFTPropDlg::OnNotifyListView(UINT uCode, LPNMHDR pNMHDR)
{
    LRESULT lRes = FALSE;

    switch(uCode)
    {
        case LVN_GETINFOTIP:
        {
            NMLVGETINFOTIP* plvn = (NMLVGETINFOTIP*)pNMHDR;

            break;
        }
        case LVN_ITEMCHANGED:
        {
            NMLISTVIEW* pNMLV = (NMLISTVIEW*)pNMHDR;

            // Is a new item being selected?
            if ((pNMLV->uChanged & LVIF_STATE) &&
                (pNMLV->uNewState & (LVIS_SELECTED | LVIS_FOCUSED)))
            {
                // Yes
                OnListViewSelItem(pNMLV->iItem, pNMLV->lParam);
            }
            break;
        }

        case LVN_COLUMNCLICK:
        {
            NMLISTVIEW* pNMLV = (NMLISTVIEW*)pNMHDR;

            OnListViewColumnClick(pNMLV->iSubItem);
            break;
        }

        case NM_DBLCLK:
            if (IsWindowEnabled(GetDlgItem(_hwnd, IDC_FT_PROP_EDIT)))
                PostMessage(_hwnd, WM_COMMAND, (WPARAM)IDC_FT_PROP_EDIT, 0);
            break;
    }

    return lRes;
}

LRESULT CFTPropDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = FALSE;

    switch (GET_WM_COMMAND_ID(wParam, lParam))
    {
        case IDC_FT_PROP_NEW:
            lRes = OnNewButton(GET_WM_COMMAND_CMD(wParam, lParam));
            break;

        case IDC_FT_PROP_REMOVE:
            lRes = OnDeleteButton(GET_WM_COMMAND_CMD(wParam, lParam));
            break;

        case IDC_FT_PROP_EDITTYPEOFFILE:
            lRes = OnAdvancedButton(GET_WM_COMMAND_CMD(wParam, lParam));
            break;

        case IDC_FT_PROP_CHANGEOPENSWITH:
            lRes = OnChangeButton(GET_WM_COMMAND_CMD(wParam, lParam));
            break;

        default:
            lRes = CFTDlg::OnCommand(wParam, lParam);
            break;
    }

    return lRes;    
}

LRESULT CFTPropDlg::OnNotify(WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = FALSE;

    LPNMHDR pNMHDR = (LPNMHDR)lParam;
    UINT_PTR idFrom = pNMHDR->idFrom;
    UINT uCode = pNMHDR->code;

    //GET_WM_COMMAND_CMD
    switch(idFrom)
    {
        case IDC_FT_PROP_LV_FILETYPES:
            lRes = OnNotifyListView(uCode, pNMHDR);
            break;
        default:
            lRes = CFTDlg::OnNotify(wParam, lParam);
            break;
    }

    return lRes;    
}

LRESULT CFTPropDlg::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = FALSE;

    switch(uMsg)
    {
        case WM_CTLCOLORSTATIC:
            lRes = OnCtlColorStatic(wParam, lParam);
            break;

        case WM_FINISHFILLLISTVIEW:
            lRes = OnFinishInitDialog();
            break;

        default:
            lRes = CFTDlg::WndProc(uMsg, wParam, lParam);
            break;
    }

    return lRes;
}
