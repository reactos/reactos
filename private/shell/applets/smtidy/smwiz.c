//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
#include "smtidy.h"
#include "smwiz.h"
#include "resource.h"
#include "util.h"
#include "killactv.h"
#include "finish.h"

//---------------------------------------------------------------------------
#define POINTS_PER_INCH      72

//---------------------------------------------------------------------------
DWORD GetOriginalTargetAttribs(IShellLink *psl)
{
    DWORD dwRet = 0;
    WIN32_FIND_DATA fd;
    
    if (SUCCEEDED(psl->lpVtbl->GetPath(psl, NULL, 0, &fd, 0)))
        dwRet = fd.dwFileAttributes;

    return dwRet;
}

//---------------------------------------------------------------------------
// Given a pidl for a link, extract the appropriate info and append it to 
// the app list.
void SMIList_AppendItem(PSMTIDYINFO psmti, PIDL pidlFolder, PIDL pidlItem, DWORD dwFlags)
{
    TCHAR szPath[MAX_PATH];
    WCHAR wszPath[MAX_PATH];
    SMITEM smi;
    PIDL pidl;
        
    Assert(psmti->hdsaSMI);
    Assert(pidlFolder);
    Assert(pidlItem);
    Assert(psmti->psl);
    Assert(psmti->ppf);
        
    
    pidl = ILCombine(pidlFolder, pidlItem);
    
    memset(&smi, 0, SIZEOF(smi));
    smi.dwFlags = dwFlags;
    smi.pidlItem = pidl;
    
    SHGetPathFromIDList(pidl, szPath);
    STRTOOLESTR(wszPath, szPath);
    if (SUCCEEDED(psmti->ppf->lpVtbl->Load(psmti->ppf, wszPath, 0)))
    {
        PIDL pidlTarget;
        if (SUCCEEDED(psmti->psl->lpVtbl->GetIDList(psmti->psl, &pidlTarget)))
        {
            SHGetPathFromIDList(pidlTarget, szPath);
            if (*szPath)
            {
                Sz_AllocCopy(szPath, &smi.pszTarget);
                smi.dwMatch = GetOriginalTargetAttribs(psmti->psl) & FILE_ATTRIBUTE_DIRECTORY;    // Needed for link tracking.
            }
            else
            {
                smi.dwFlags |= SMIF_TARGET_NOT_FILE;
            }
        }
    }
    DSA_AppendItem(psmti->hdsaSMI, &smi);
}

//----------------------------------------------------------------------------
typedef BOOL (*PFNENUMFOLDERCALLBACK)(LPSHELLFOLDER psf, HWND hwndOwner, PIDL pidlFolder, PIDL pidlItem, LPVOID pv);

//----------------------------------------------------------------------------
BOOL EnumFolder(HWND hwndOwner, PIDL pidlFolder, DWORD grfFlags, PFNENUMFOLDERCALLBACK pfn, LPVOID pv)
{
    LPSHELLFOLDER psf;
    LPSHELLFOLDER psfDesktop;
    BOOL fRet = FALSE;

    if (SUCCEEDED(ICoCreateInstance(&CLSID_ShellDesktop, &IID_IShellFolder, &psfDesktop)))
    {
        if (SUCCEEDED(psfDesktop->lpVtbl->BindToObject(psfDesktop, pidlFolder, NULL, &IID_IShellFolder, &psf)))
        {
            LPENUMIDLIST penum;
            if (SUCCEEDED(psf->lpVtbl->EnumObjects(psf, hwndOwner, grfFlags, &penum)))
            {
                PIDL pidl;
                UINT celt;
                while (penum->lpVtbl->Next(penum, 1, &pidl, &celt)==NOERROR && celt==1)
                {
                    if (!(*pfn)(psf, hwndOwner, pidlFolder, pidl, pv))
                    {
                        ILFree(pidl);
                        break;
                    }
                    ILFree(pidl);
                }
                fRet = TRUE;
                penum->lpVtbl->Release(penum);
            }
            psf->lpVtbl->Release(psf);
        }
        psfDesktop->lpVtbl->Release(psfDesktop);
    }
    
    return fRet;
}

//----------------------------------------------------------------------------
BOOL EnumFolder_Callback(LPSHELLFOLDER psf, HWND hwndOwner, PIDL pidlFolder, 
    PIDL pidlItem, LPVOID pv)
{
    DWORD dwAttribs = SFGAO_LINK|SFGAO_FOLDER;
    PSMTIDYINFO psmti = pv;
    BOOL fRet = FALSE;

    Assert(psmti);
            
    if (SUCCEEDED(psf->lpVtbl->GetAttributesOf(psf, 1, &pidlItem, &dwAttribs)))
    {
        // Is it a folder?
        if (dwAttribs & SFGAO_FOLDER)
        {
            PIDL pidlRecurse = ILCombine(pidlFolder, pidlItem);
            SMIList_AppendItem(psmti, pidlFolder, pidlItem, SMIF_FOLDER);
            // Recurse.
            if (pidlRecurse)
            {
                EnumFolder(hwndOwner, pidlRecurse, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, EnumFolder_Callback, psmti);
                ILFree(pidlRecurse);
            }
        }
        else if (dwAttribs & SFGAO_LINK)
        {
            // Regular link, add it to the list.
            SMIList_AppendItem(psmti, pidlFolder, /*psf,*/ pidlItem, SMIF_NONE);
        }
        fRet = TRUE;
    }
    
    return fRet;
}

//----------------------------------------------------------------------------
BOOL SMIList_Build(PSMTIDYINFO psmti)
{
    BOOL fRet = FALSE;
    PIDL pidlStartmenu;
    PIDL pidlPrograms;
    
    Assert(psmti);
    Assert(psmti->hdsaSMI);
    
    // Build a DPA of pidls for each item.
    Dbg(TEXT("smw.smt_ea: Enumerating everything..."));

    pidlStartmenu = SHCloneSpecialIDList(NULL, CSIDL_STARTMENU, TRUE);
    if (pidlStartmenu)
    {
        pidlPrograms = SHCloneSpecialIDList(NULL, CSIDL_PROGRAMS, TRUE);
        if (pidlPrograms)
        {
            if (SUCCEEDED(ICoCreateInstance(&CLSID_ShellLink, &IID_IShellLink, &psmti->psl)))
            {
                if (SUCCEEDED(psmti->psl->lpVtbl->QueryInterface(psmti->psl, &IID_IPersistFile, &psmti->ppf)))
                {
                    // Enum Startmenu.
                    EnumFolder(NULL, pidlStartmenu, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, 
                        EnumFolder_Callback, psmti);
                    // Is programs under the StartMenu?
                    if (!ILIsParent(pidlStartmenu, pidlPrograms, FALSE))
                    {
                        // Nope, enum it seperately.
                        EnumFolder(NULL, pidlPrograms, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, 
                            EnumFolder_Callback, psmti);
                    }
                    psmti->ppf->lpVtbl->Release(psmti->ppf);
                    psmti->ppf = NULL;
                    fRet = TRUE;
                }
                psmti->psl->lpVtbl->Release(psmti->psl);
                psmti->psl = NULL;
            }
            ILFree(pidlPrograms);
        }
        ILFree(pidlStartmenu);
    }

    return fRet;
}

//----------------------------------------------------------------------------
// REVIEW UNDONE IANEL - Allow these defaults to be overriden from the
// registry (preserve them their too).
void SetDefaultOptions(HWND hDlg, LPPROPSHEETPAGE pps)
{
    UINT idPage = PtrToUlong(pps->pszTemplate);

    switch (idPage)
    {
        case DLG_SMINTRO:
            CheckDlgButton(hDlg, IDC_REMOVE_BROKEN_SHORTCUTS, BST_CHECKED);
            CheckDlgButton(hDlg, IDC_REMOVE_EMPTY_FOLDERS, BST_CHECKED);
            CheckDlgButton(hDlg, IDC_MOVE_SINGLE_ITEMS, BST_CHECKED);
            break;        
        case DLG_GROUP_READMES:
            CheckRadioButton(hDlg, IDC_REMOVE_READMES, IDC_LEAVE_READMES_ALONE, IDC_GROUP_READMES);
            break;
        case DLG_UNUSED_SHORTCUTS:
            CheckRadioButton(hDlg, IDC_REMOVE_UNUSED_SHORTCUTS, IDC_LEAVE_UNUSED_SHORTCUTS_ALONE, IDC_GROUP_UNUSED_SHORTCUTS);
            break;
    }
}

//----------------------------------------------------------------------------
void OnSetActive(HWND hDlg, LPPROPSHEETPAGE pps)
{
    PSMTIDYINFO psmti = (PSMTIDYINFO) pps->lParam;
    UINT idPage = PtrToUlong(pps->pszTemplate);
    LRESULT lRes = 0;
    
    Assert(psmti);
    Assert(idPage);
    
    if (psmti->hbmp)
        SendMessage(GetDlgItem(hDlg, IDC_WIZBMP), STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)psmti->hbmp);

    // Only show pages that actually apply.
    switch (idPage)
    {
        case DLG_SMINTRO:
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
            break;
        case DLG_SMFINISH:
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_FINISH|PSWIZB_BACK);
            break;
        case DLG_GROUP_READMES:
        case DLG_UNUSED_SHORTCUTS:
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT|PSWIZB_BACK);
            break;
    }
    SetDlgMsgResult(hDlg, WM_NOTIFY, lRes);
}

//----------------------------------------------------------------------------
void OnKillActive(HWND hDlg, LPPROPSHEETPAGE pps)
{
    PSMTIDYINFO psmti = (PSMTIDYINFO) pps->lParam;
    UINT idPage = PtrToUlong(pps->pszTemplate);
    LRESULT lRes = 0;
    
    Assert(psmti);
    Assert(idPage);
    
    switch (idPage)
    {
        case DLG_SMINTRO:
            KillActiveLostTargets(psmti, hDlg);
            KillActiveEmptyFolders(psmti, hDlg);
            KillActiveSingleItemFolder(psmti, hDlg);
            break;
        case DLG_GROUP_READMES:
            KillActiveGroupReadMes(psmti, hDlg);
            break;
        case DLG_UNUSED_SHORTCUTS:
            KillActiveUnusedShortcuts(psmti, hDlg);
            break;
    }
}

//----------------------------------------------------------------------------
BOOL OnWizNotify(HWND hDlg, LPPROPSHEETPAGE pps, NMHDR *pnm)
{
    switch (pnm->code)
    {
        case PSN_SETACTIVE:
            OnSetActive(hDlg, pps);
            break;

        case PSN_KILLACTIVE:
            OnKillActive(hDlg, pps);
            break;

        case PSN_WIZFINISH:
            OnKillActive(hDlg, pps);    // Should apply the changes of current page
            OnFinish(hDlg, pps);
            // fall through
        case PSN_WIZNEXT:
        case PSN_WIZBACK:
        case PSN_RESET:
            SetDlgMsgResult(hDlg, WM_NOTIFY, 0);
            return FALSE;

        default:
            return FALSE;
    }
    return TRUE;
}

//----------------------------------------------------------------------------
typedef BOOL (*PFCFGSTART)(HWND, BOOL);

//----------------------------------------------------------------------------
void CallAppWiz(HWND hDlg, BOOL bDelItems)
{
    HANDLE hmodWiz = LoadLibrary(TEXT("AppWiz.Cpl"));
    if (hmodWiz) 
    {
        PFCFGSTART pfnCfgStart = (PFCFGSTART)GetProcAddress(hmodWiz, "ConfigStartMenu");
        if (pfnCfgStart) 
        {
            pfnCfgStart(hDlg, bDelItems);
        }
        FreeLibrary(hmodWiz);
    }
}

//----------------------------------------------------------------------------
void OnCommand(HWND hDlg, WORD cmd)
{                        
    switch (cmd) 
    {
        case IDHELP:
            break;
        case IDC_ADD_SHORTCUTS:
            CallAppWiz(hDlg, FALSE);
            break;
        case IDC_REMOVE_SHORTCUTS:
            CallAppWiz(hDlg, TRUE);
            break;
    }
}

//----------------------------------------------------------------------------
void SetFonts(PSMTIDYINFO psmti, HWND hDlg)
{
    HWND hwndTitle = GetDlgItem(hDlg, IDC_TITLE);

    if (hwndTitle)
    {
        if (!psmti->hfontLarge)
        {
            HFONT hfont = (HFONT)SendMessage(hwndTitle, WM_GETFONT, 0, 0);
            if (hfont)
            {
                LOGFONT lf;
                if (GetObject(hfont, SIZEOF(lf), &lf))
                {
                    HDC hdc = GetDC(NULL);
                    lf.lfHeight = -MulDiv(14, GetDeviceCaps(hdc, LOGPIXELSY), POINTS_PER_INCH );
                    // lf.lfWeight = FW_BOLD;
                    ReleaseDC(NULL, hdc);
                    psmti->hfontLarge = CreateFontIndirect(&lf);
                    
                }
            }
        }
        
        if (psmti->hfontLarge)
            SendMessage(hwndTitle, WM_SETFONT, (WPARAM)psmti->hfontLarge, 0);
    }
}

//----------------------------------------------------------------------------
INT_PTR CALLBACK SMTidyProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPPROPSHEETPAGE pps = (LPPROPSHEETPAGE)(GetWindowLongPtr(hDlg, DWLP_USER));
    
    switch(msg) 
    {
        case WM_INITDIALOG:
        {
            LPPROPSHEETPAGE pps = (LPPROPSHEETPAGE)lParam;
            PSMTIDYINFO psmti = (PSMTIDYINFO)(pps->lParam);

            Assert(pps);
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);
            SetDefaultOptions(hDlg, pps);
            SetFonts(psmti, hDlg);
            SetWindowPos(GetParent(hDlg), NULL,  psmti->rc.left, psmti->rc.top, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
            break;
        }
        
        case WM_NOTIFY:
        {
            NMHDR *pnm = (NMHDR*)lParam;
            return OnWizNotify(hDlg, pps, pnm);
        }
        
        case WM_DESTROY:
        case WM_HELP:
        case WM_CONTEXTMENU:
            break;

        case WM_COMMAND:
            OnCommand(hDlg, LOWORD(wParam));
            break;

        default:
            return FALSE;

    }

    return TRUE;
}

//----------------------------------------------------------------------------
#define CMAXPAGES  10

//----------------------------------------------------------------------------
void PropertySheet_AddPage(LPPROPSHEETHEADER ppsh, UINT id, DLGPROC pfn, LPVOID pv)
{
    PROPSHEETPAGE psp;

    if (ppsh->nPages >= CMAXPAGES)
    {
        Dbg(TEXT("smt.ps_ap: Too many pages."));
        Assert(0);
        return;
    }
    
    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = g_hinstApp;
    psp.pszTemplate = MAKEINTRESOURCE(id);
    psp.pfnDlgProc = pfn;
    psp.lParam = (LPARAM)pv;

    ppsh->phpage[ppsh->nPages] = CreatePropertySheetPage(&psp);
    
    if (ppsh->phpage[ppsh->nPages])
        ppsh->nPages++;
} 

//----------------------------------------------------------------------------
// StartMenu wizard.
BOOL TidyStartMenuWizard(PSMTIDYINFO psmti)
{
    HPROPSHEETPAGE rPages[CMAXPAGES];
    PROPSHEETHEADER psh;
    
    psh.pszCaption = NULL;
    psh.nPages = 0;
    psh.nStartPage = 0;
    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_PROPTITLE | PSH_WIZARD | PSH_WIZARDHASFINISH;
    psh.hwndParent = psmti->hwnd;
    psh.hInstance = g_hinstApp;
    psh.phpage = rPages;

    PropertySheet_AddPage(&psh, DLG_SMINTRO, SMTidyProc, psmti);
    PropertySheet_AddPage(&psh, DLG_UNUSED_SHORTCUTS, SMTidyProc, psmti);
    PropertySheet_AddPage(&psh, DLG_GROUP_READMES, SMTidyProc, psmti);
    PropertySheet_AddPage(&psh, DLG_SMFINISH, SMTidyProc, psmti);
    
    PropertySheet(&psh);

    return TRUE;
}
