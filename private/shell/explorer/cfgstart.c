//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#include "cabinet.h"
#include "rcids.h"
#include <help.h>       // help ids
#include <shellp.h>
#include <shdguid.h>
#include <trayp.h>
#include <regstr.h>
#include <shguidp.h>
#include <windowsx.h>

const static DWORD aInitStartMenuHelpIDs[] = {
    IDC_NO_HELP_1,       NO_HELP,
    IDC_NO_HELP_2,       NO_HELP,
    IDC_NO_HELP_3,       NO_HELP,
    IDC_NO_HELP_4,       NO_HELP,
    IDC_GROUPBOX,        IDH_COMM_GROUPBOX,
    IDC_GROUPBOX_2,      IDH_MENUCONFIG_CLEAR,
    IDC_GROUPBOX_3,      IDH_COMM_GROUPBOX,
#ifndef NEW_SM_CONFIG
    IDC_ADDSHORTCUT,     IDH_TRAY_ADD_PROGRAM,
    IDC_DELSHORTCUT,     IDH_TRAY_REMOVE_PROGRAM,
#else
#ifndef IDH_TRAY_SMWIZARD
#define IDH_TRAY_SMWIZARD   NO_HELP
#endif
    IDC_SMWIZARD,        IDH_TRAY_SMWIZARD,
#endif
    IDC_EXPLOREMENUS,    IDH_TRAY_ADVANCED,
    IDC_KILLDOCUMENTS,   IDH_MENUCONFIG_CLEAR,
    IDC_RESORT,          IDH_TRAY_RESORT_BUTTON,
    IDC_STARTMENUSETTINGSTEXT, IDH_TRAY_START_MENU_SETTINGS,
    0, 0
};

#define REG_EXPLORER_ADVANCED TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced")
#define REGSTR_PATH_SMADVANCED    REGSTR_PATH_EXPLORER TEXT("\\StartMenu")

typedef struct
{
    HWND hwndTree;
    IRegTreeOptions *pTO;
} SMADVANCED;



typedef BOOL (* PFCFGSTART) (HWND, BOOL);

void CallAppWiz(HWND hDlg, BOOL bDelItems)
{
    HANDLE hmodWiz = LoadLibrary(TEXT("AppWiz.Cpl"));
    if (hmodWiz) {
        PFCFGSTART pfnCfgStart = (PFCFGSTART)GetProcAddress(hmodWiz, "ConfigStartMenu");
        if (pfnCfgStart) {
            pfnCfgStart(hDlg, bDelItems);
        }
        FreeLibrary(hmodWiz);
    }
}

BOOL ExecExplorerAtStartMenu(HWND hDlg)
{
    SHELLEXECUTEINFO ei;
    BOOL fWorked= FALSE;
    TCHAR    szParams[MAX_PATH];

    ei.cbSize = SIZEOF(ei);
    ei.hwnd = hDlg;
    ei.lpVerb = NULL;
    ei.fMask = 0;
    ei.lpFile = TEXT("Explorer.exe");

    if (IsUserAnAdmin()) {
        lstrcpy(szParams, TEXT("/E,"));
        SHGetSpecialFolderPath(hDlg, &(szParams[ARRAYSIZE(TEXT("/E,"))-1]),
                                CSIDL_STARTMENU, FALSE);
    } else {
        lstrcpy(szParams, TEXT("/E,/Root,"));
        SHGetSpecialFolderPath(hDlg, &(szParams[ARRAYSIZE(TEXT("/E,/Root,"))-1]),
                                CSIDL_STARTMENU, FALSE);
    }

    ei.lpParameters = szParams;
    ei.lpDirectory = NULL;
    ei.lpClass = NULL;
    ei.nShow = SW_SHOWDEFAULT;
    ei.hInstApp = hinstCabinet;

    return(ShellExecuteEx(&ei));
}

const TCHAR *c_szRegMruKeysToDelete[] =
{
    TEXT("Software\\Microsoft\\Internet Explorer\\TypedURLs"),
    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU"),
    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Doc Find Spec MRU")
};

void SetDocButton(HWND hDlg)
{
    LPITEMIDLIST pidl;
    BOOL    bAreDocs = FALSE;

    SHGetSpecialFolderLocation(hDlg, CSIDL_RECENT, &pidl);
    if (pidl) {
        LPSHELLFOLDER psf = BindToFolder(pidl);
        if (psf) {
            HRESULT hres;
            LPENUMIDLIST penum;
            hres = psf->lpVtbl->EnumObjects(psf, hDlg, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &penum);
            if (SUCCEEDED(hres)) {
                UINT celt;
                LPITEMIDLIST pidlenum;
                if (penum->lpVtbl->Next(penum, 1, &pidlenum, &celt)==NOERROR &&
                    celt==1) {
                    SHFree(pidlenum);
                    bAreDocs = TRUE;
                }
                penum->lpVtbl->Release(penum);
            }
            psf->lpVtbl->Release(psf);
        }
        SHFree(pidl);
    }

    // Check other MRU registry keys
    if (!bAreDocs) {
        int  i;
        for (i=0; i < ARRAYSIZE(c_szRegMruKeysToDelete); i++) {
            HKEY hkey;
            if (RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegMruKeysToDelete[i], 0L, KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS) {
                bAreDocs = TRUE;
                RegCloseKey(hkey);
            }
        }
    }
    
    Button_Enable(GetDlgItem(hDlg, IDC_KILLDOCUMENTS), bAreDocs);
}

void ClearRecentDocumentsAndMRUStuff(BOOL fBroadcastChange)
{
    int i;
    SHAddToRecentDocs(0, NULL);

    // Flush other MRUs in the registry for privacy
    for (i = 0; i < ARRAYSIZE(c_szRegMruKeysToDelete); i++) {
        RegDeleteKey(HKEY_CURRENT_USER, c_szRegMruKeysToDelete[i]);

        if (fBroadcastChange)
            SHSendMessageBroadcast(WM_SETTINGCHANGE, 0,
                        (LPARAM)c_szRegMruKeysToDelete[i]);
    }    
}


void ExecSMTidyWizard(HWND hdlg)
{
    SHELLEXECUTEINFO ei = { SIZEOF(ei), 0, NULL, NULL, TEXT("SMTidy.exe"), NULL, NULL, SW_SHOWNORMAL};
    ShellExecuteEx(&ei);
}

void Reorder(HDPA hdpa)
{
    int i;

    for (i = DPA_GetPtrCount(hdpa)-1 ; i >= 0 ; i--)
    {
        PORDERITEM poi = (PORDERITEM)DPA_FastGetPtr(hdpa, i);

        poi->nOrder = i;
    }
}

void MenuOrderSort(HKEY hkeyRoot, IShellFolder* psf);

void MenuOrderSortKeyWithFolder(HKEY hkeyRoot, LPTSTR pszKey, IShellFolder* psf)
{
    HKEY     hkey;
    if (ERROR_SUCCESS == RegOpenKeyEx(hkeyRoot, pszKey, 0, KEY_READ | KEY_WRITE, &hkey))
    {
        MenuOrderSort(hkey, psf);
        RegCloseKey(hkey);
    }
}

// Binds to the Key pszKey, under hkey root, using psf, and sorts the resultant order.
void MenuOrderSortSubKey(HKEY hkeyRoot, LPTSTR szFolder, LPTSTR pszKey, IShellFolder* psf)
{
    LPITEMIDLIST pidl;
    DWORD cbEaten;
    DWORD dwAttrib;
    WCHAR wszKey[MAX_PATH];
    SHTCharToUnicode(szFolder, wszKey, ARRAYSIZE(wszKey));

    if (SUCCEEDED(psf->lpVtbl->ParseDisplayName(psf, NULL, NULL, wszKey, &cbEaten, &pidl, &dwAttrib)))
    {
        IShellFolder* psfSub;
        if (SUCCEEDED(psf->lpVtbl->BindToObject(psf, pidl, NULL, &IID_IShellFolder, (void**)&psfSub)))
        {
            MenuOrderSortKeyWithFolder(hkeyRoot, pszKey, psfSub);
            psfSub->lpVtbl->Release(psfSub);
        }
        ILFree(pidl);
    }
}


void MenuOrderSort(HKEY hkeyRoot, IShellFolder* psf)
{
    IStream* pstm;
    HDPA     hdpa;
    TCHAR    szKey[MAX_PATH];
    int      iIndex = 0;
    DWORD    cbKey = ARRAYSIZE(szKey);
    LARGE_INTEGER liZero = {0};

    //Try to open Value Order
    pstm = SHOpenRegStream(hkeyRoot, TEXT(""), TEXT("Order"), STGM_READ);
    if (pstm)
    {
        IOrderList2* pol2;
        if (SUCCEEDED(CoCreateInstance(&CLSID_OrderListExport, NULL, CLSCTX_INPROC_SERVER, &IID_IOrderList2, (void**)&pol2)))
        {
            if (SUCCEEDED(pol2->lpVtbl->LoadFromStream(pol2, pstm, &hdpa, psf)))
            {
                // Since it's stored ordered by name, this should be no problem.
                Reorder(hdpa);

                //Set the seek pointer at the beginning.
                pstm->lpVtbl->Seek(pstm, liZero, STREAM_SEEK_SET, NULL);

                pol2->lpVtbl->SaveToStream(pol2, pstm, hdpa);
                DPA_Destroy(hdpa);
            }
            pol2->lpVtbl->Release(pol2);
        }
        pstm->lpVtbl->Release(pstm);
    }

    // Now enumerate sub keys.

    while(ERROR_SUCCESS == RegEnumKeyEx(hkeyRoot, iIndex, szKey, &cbKey, NULL,
                                        NULL, NULL, NULL))
    {
        MenuOrderSortSubKey(hkeyRoot, szKey, szKey, psf);
        iIndex++;
        cbKey = ARRAYSIZE(szKey);
    }
}

// Defined in Tray.c
IShellFolder* BindToFolder(LPCITEMIDLIST pidl);

void StartMenuSort()
{
    IShellFolder* psf;
    LPITEMIDLIST pidl;

    SHGetSpecialFolderLocation(NULL, CSIDL_STARTMENU, &pidl);
    if (pidl)
    {
#ifdef WINNT
        HRESULT hres;
        LPITEMIDLIST pidl2;
        if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_STARTMENU, &pidl2)))
        {
            IAugmentedShellFolder2* pasf;
            IShellFolder* psfCommon;
            IShellFolder* psfUser;
            hres = CoCreateInstance(&CLSID_AugmentedShellFolder2, NULL, CLSCTX_INPROC, 
                                    &IID_IAugmentedShellFolder2, (void **)&pasf);
            if (SUCCEEDED(hres))
            {
                psfUser = BindToFolder(pidl);
                if (psfUser)
                {
                    pasf->lpVtbl->AddNameSpace(pasf, NULL, psfUser, pidl, ASFF_DEFAULT | ASFF_DEFNAMESPACE_ALL);
                    psfUser->lpVtbl->Release(psfUser);
                }

                psfCommon = BindToFolder(pidl2);

                if (psfCommon)
                {
                    pasf->lpVtbl->AddNameSpace(pasf, NULL, psfCommon, pidl2, ASFF_DEFAULT);
                    psfCommon->lpVtbl->Release(psfCommon);
                }

                hres = pasf->lpVtbl->QueryInterface(pasf, &IID_IShellFolder, (void**)&psf);
                pasf->lpVtbl->Release(pasf);
            }

            ILFree(pidl2);
        }
        else
#endif
        {
            psf = BindToFolder(pidl);
        }
        ILFree(pidl);                        
    }

    if (psf)
    {
        HKEY hkeyRoot;

        // Recursivly sort the orders. Should this be on another thread?
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, STRREG_STARTMENU, 
            0, KEY_READ | KEY_WRITE, &hkeyRoot))
        {
            LPTSTR pszName;
            TCHAR szPath[MAX_PATH];
            LPITEMIDLIST pidlMyDocs;

            MenuOrderSort(hkeyRoot, psf);


            // Sort the My Documents menu item:
            SHGetFolderLocation(NULL, CSIDL_PERSONAL, NULL, 0, &pidlMyDocs);
            if (pidlMyDocs)
            {
                IShellFolder* psfMyDocs;
                if (SUCCEEDED(SHBindToObject(NULL, &IID_IShellFolder, pidlMyDocs, (void**)&psfMyDocs)))
                {
                    MenuOrderSortKeyWithFolder(hkeyRoot, TEXT("MyDocuments"), psfMyDocs);
                    psfMyDocs->lpVtbl->Release(psfMyDocs);
                }
                ILFree(pidlMyDocs);
            }


            // What happens if the Filesystem programs is not equal to the hard coded string "Programs"? This
            // happens on German: Programme != Programs and we fail to sort. So let's verify:
            SHGetFolderPath(NULL, CSIDL_PROGRAMS, NULL, 0, szPath);
            pszName = PathFindFileName(szPath);

            if (StrCmpI(pszName, TEXT("Programs")) != 0)
            {
                // Ok, It's not the same, so go bind to that sub tree and sort it.
                MenuOrderSortSubKey(hkeyRoot, pszName, TEXT("Programs"), psf);
            }
            RegCloseKey(hkeyRoot);
        }

        psf->lpVtbl->Release(psf);
    }
}


BOOL InitDialog(HWND hwndDlg)
{
    HRESULT hr;
    SMADVANCED*  pAdv = (SMADVANCED*)LocalAlloc(LPTR, sizeof(SMADVANCED));
    if (!pAdv)
    {
        EndDialog(hwndDlg, 0);
        return FALSE;   // no memory?
    }

    SetWindowPtr(hwndDlg, DWLP_USER, pAdv);

    pAdv->hwndTree = GetDlgItem( hwndDlg, IDC_STARTMENUSETTINGS );

    hr = CoCreateInstance(&CLSID_CRegTreeOptions, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IRegTreeOptions, (LPVOID *)&(pAdv->pTO));
    if (SUCCEEDED(hr))
    {
        // HACKHACK - IRegTreeOptions is ANSI, so we temporarily turn off UNICODE
        #undef TEXT
        #define TEXT(s) s
        hr = pAdv->pTO->lpVtbl->InitTree(pAdv->pTO, pAdv->hwndTree, HKEY_LOCAL_MACHINE, REGSTR_PATH_SMADVANCED, NULL);
        #undef TEXT
        #define TEXT(s) __TEXT(s)
    }

    // find the first root and make sure that it is visible
    TreeView_EnsureVisible(pAdv->hwndTree, TreeView_GetRoot( pAdv->hwndTree ));

    SetDocButton(hwndDlg);

    return SUCCEEDED(hr);
}



BOOL_PTR CALLBACK InitStartMenuDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam,
        LPARAM lParam)
{
    SMADVANCED* pAdv = (SMADVANCED*)GetWindowPtr(hwndDlg, DWLP_USER);
    INSTRUMENT_WNDPROC(SHCNFI_INITSTARTMENU_DLGPROC, hwndDlg, msg, wParam, lParam);

    if (msg != WM_INITDIALOG && !pAdv)
    {
        // We've been re-entered after being destroyed.  Bail.
        return FALSE;
    }

    switch (msg) 
    {
    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam, lParam)) 
        {
#ifndef NEW_SM_CONFIG
        case IDC_ADDSHORTCUT:
            CallAppWiz(hwndDlg, FALSE);
            break;

        case IDC_DELSHORTCUT:
            CallAppWiz(hwndDlg, TRUE);
            break;
#else
        case IDC_SMWIZARD:
            ExecSMTidyWizard(hwndDlg);
            break;
#endif
            
        case IDC_RESORT:
        {
            SHChangeDWORDAsIDList dwidl;

            StartMenuSort();

            // Notify everyone that the order changed
            dwidl.cb      = SIZEOF(dwidl) - SIZEOF(dwidl.cbZero);
            dwidl.dwItem1 = SHCNEE_ORDERCHANGED;
            dwidl.dwItem2 = 0; 
            dwidl.cbZero  = 0;

            SHChangeNotify(SHCNE_EXTENDED_EVENT, SHCNF_FLUSH, (LPCITEMIDLIST)&dwidl, NULL);
            break;
        }
        
        case IDC_EXPLOREMENUS:
            ExecExplorerAtStartMenu(hwndDlg);
            break;

        case IDC_KILLDOCUMENTS:
            {
                HCURSOR hc = SetCursor(LoadCursor(NULL, IDC_WAIT));
                ClearRecentDocumentsAndMRUStuff(TRUE);
                SetCursor(hc);
                Button_Enable(GET_WM_COMMAND_HWND(wParam, lParam), FALSE);
                SetFocus(GetDlgItem(GetParent(hwndDlg), IDOK));
            }
            break;
        }
        break;

    case WM_INITDIALOG:
        return InitDialog(hwndDlg);

    case WM_NOTIFY:
        SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, 0); // handled
        switch (((NMHDR *)lParam)->code) 
        {
            case TVN_KEYDOWN:
            {
                TV_KEYDOWN *pnm = (TV_KEYDOWN*)((NMHDR *)lParam);
                if (pnm->wVKey == VK_SPACE)
                {
                    pAdv->pTO->lpVtbl->ToggleItem(pAdv->pTO, (HTREEITEM)SendMessage(pAdv->hwndTree, TVM_GETNEXTITEM, (WPARAM)TVGN_CARET, 0L));
                    SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0L);
                    SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, TRUE); // eat the key
                }
                break;
            }

            case NM_CLICK:
            case NM_DBLCLK:
                // is this click in our tree?
                if ( ((NMHDR *)lParam)->idFrom == IDC_STARTMENUSETTINGS )
                {
                    TV_HITTESTINFO ht;

                    // BUGBUG REVIEW: shouldn't this be GetMessagePos?
                    GetCursorPos( &ht.pt );                         // get where we were hit
                    ScreenToClient( pAdv->hwndTree, &ht.pt );       // translate it to our window

                    // retrieve the item hit
                    pAdv->pTO->lpVtbl->ToggleItem(pAdv->pTO, TreeView_HitTest( pAdv->hwndTree, &ht));
                    SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0L);
                }

                break;

            case PSN_APPLY:
            {
                pAdv->pTO->lpVtbl->WalkTree(pAdv->pTO, WALK_TREE_SAVE);
                SHSendMessageBroadcast(WM_SETTINGCHANGE, 0, 0l);
            }
            break;
        }
        break;

    case WM_HELP:                   // F1
    {
        LPHELPINFO lphelpinfo;
        lphelpinfo = (LPHELPINFO)lParam;

        if (lphelpinfo->iCtrlId != IDC_STARTMENUSETTINGS)
        {
            WinHelp(((LPHELPINFO) lParam)->hItemHandle, NULL,
                HELP_WM_HELP, (ULONG_PTR)(LPTSTR)aInitStartMenuHelpIDs);

        }
        else
        {
            HTREEITEM hItem;
            //Is this help invoked throught F1 key
            if (GetAsyncKeyState(VK_F1) < 0)
            {
                // Yes. WE need to give help for the currently selected item
                hItem = TreeView_GetSelection(pAdv->hwndTree);
            }
            else
            {
                //No, We need to give help for the item at the cursor position
                TV_HITTESTINFO ht;
                ht.pt =((LPHELPINFO)lParam)->MousePos;
                ScreenToClient(pAdv->hwndTree, &ht.pt); // Translate it to our window
                hItem = TreeView_HitTest(pAdv->hwndTree, &ht);
            }

            pAdv->pTO->lpVtbl->ShowHelp(pAdv->pTO, hItem, HELP_WM_HELP);
        }
        break;
    }

    case WM_CONTEXTMENU:        // right mouse click
    {
        TV_HITTESTINFO ht;

        GetCursorPos( &ht.pt );                         // get where we were hit

        if (pAdv->hwndTree == WindowFromPoint(ht.pt))
        {
            ScreenToClient( pAdv->hwndTree, &ht.pt );       // translate it to our window

            // retrieve the item hit
            pAdv->pTO->lpVtbl->ShowHelp(pAdv->pTO, TreeView_HitTest( pAdv->hwndTree, &ht),HELP_CONTEXTMENU);
        }
        else
        {
            WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
                (ULONG_PTR)(LPVOID)aInitStartMenuHelpIDs);
        }
        break;
    }
        
    case WM_DESTROY:
        {
            // free the tree
            if (pAdv->pTO)
            {
                pAdv->pTO->lpVtbl->WalkTree(pAdv->pTO, WALK_TREE_DELETE);
                ATOMICRELEASE(pAdv->pTO);
            }

            // free local memory
            ASSERT(pAdv);
            LocalFree(pAdv);

            // make sure we don't re-enter
            SetWindowPtr( hwndDlg, DWLP_USER, NULL );
        }
        break; // WM_DESTORY

    default:
        return FALSE;
    }

    return TRUE;
}
