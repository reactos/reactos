//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: status.c
//
//  This file contains the dialog code for the Status property sheet
//
// History:
//  08-06-93 ScottH     Transferred from twin code
//
//---------------------------------------------------------------------------


#include "brfprv.h"         // common headers
#include <brfcasep.h>

#include "res.h"
#include "recact.h"
#ifdef WINNT
#include <help.h>
#else
#include "..\..\..\win\core\inc\help.h"   // help IDs
#endif


typedef struct tagSTAT
    {
    HWND        hwnd;              // dialog handle
    PPAGEDATA   ppagedata;
    FileInfo *  pfi;
    TCHAR        szFolder[MAX_PATH];
    BOOL        bInit;
    } STAT, * PSTAT;

#define Stat_Pcbs(this)         ((this)->ppagedata->pcbs)
#define Stat_AtomBrf(this)      ((this)->ppagedata->pcbs->atomBrf)
#define Stat_GetPtr(hwnd)       (PSTAT)GetWindowLongPtr(hwnd, DWLP_USER)
#define Stat_SetPtr(hwnd, lp)   (PSTAT)SetWindowLongPtr(hwnd, DWLP_USER, (LRESULT)(lp))

#define LNKM_ACTIVATEOTHER      (WM_USER + 0)

/*----------------------------------------------------------
Purpose: Disable all the controls in the property page
Returns: --
Cond:    --
*/
void PRIVATE Stat_DisableAll(
    PSTAT this)
    {
    HWND hwnd = this->hwnd;
    HWND hwndFocus = GetFocus();

    RecAct_DeleteAllItems(GetDlgItem(hwnd, IDC_UPDATEACTIONS));
    RecAct_Enable(GetDlgItem(hwnd, IDC_UPDATEACTIONS), FALSE);
        
    Button_Enable(GetDlgItem(hwnd, IDC_PBTSRECON), FALSE);
    Button_Enable(GetDlgItem(hwnd, IDC_PBTSFIND), FALSE);
    Button_Enable(GetDlgItem(hwnd, IDC_PBTSSPLIT), FALSE);

    if ( !hwndFocus || !IsWindowEnabled(hwndFocus) )
        {
        SetFocus(GetDlgItem(GetParent(hwnd), IDOK));
        SendMessage(GetParent(hwnd), DM_SETDEFID, IDOK, 0);
        }
    }


/*----------------------------------------------------------
Purpose: Set the directions static text
Returns: --
Cond:    --
*/
void PRIVATE Stat_SetDirections(
    PSTAT this)
    {
    HWND hwnd = this->hwnd;
    HWND hwndRA = GetDlgItem(this->hwnd, IDC_UPDATEACTIONS);
    RA_ITEM item;
    TCHAR sz[MAXBUFLEN];

    *sz = 0;

    // This function shouldn't be called if this is an orphan
    ASSERT(S_OK == PageData_Query(this->ppagedata, hwnd, NULL, NULL));

    item.mask = RAIF_INSIDE | RAIF_OUTSIDE | RAIF_ACTION;
    item.iItem = 0;

    ASSERT(RecAct_GetItemCount(hwndRA) == 1);

    if (RecAct_GetItem(hwndRA, &item))
        {
        UINT ids;

        ASSERT(IsFlagSet(item.mask, RAIF_INSIDE | RAIF_OUTSIDE));

        switch (item.uAction)
            {
        case RAIA_TOIN:
        case RAIA_TOOUT:
        case RAIA_DELETEOUT:
        case RAIA_DELETEIN:
        case RAIA_MERGE:
        case RAIA_SOMETHING:
            // Instructions to update
            if (this->ppagedata->bFolder)
                ids = IDS_STATPROP_PressButton;
            else
                ids = IDS_STATPROP_Update;
            break;

        case RAIA_CONFLICT:
            ids = IDS_STATPROP_Conflict;
            break;

        default:
            if (SI_UNAVAILABLE == item.siOutside.uState)
                {
                // The original file is unavailable.  We don't know if 
                // everything is up-to-date.
                ids = IDS_STATPROP_Unavailable;
                }
            else
                {
                // They are up-to-date
                ids = IDS_STATPROP_Uptodate;
                }
            break;
            }

        SzFromIDS(ids, sz, ARRAYSIZE(sz));
        }
    Static_SetText(GetDlgItem(hwnd, IDC_STTSDIRECT), sz);
    }


/*----------------------------------------------------------
Purpose: Sets the reconciliation action control

Returns: standard result
         S_OK if the item is still a twin
         S_FALSE if the item is an orphan

Cond:    --
*/
HRESULT PRIVATE Stat_SetRecAct(
    PSTAT this,
    PRECLIST prl,
    PFOLDERTWINLIST pftl)
    {
    HRESULT hres;
    HWND hwnd = this->hwnd;
    HWND hwndRA = GetDlgItem(hwnd, IDC_UPDATEACTIONS);
    LPCTSTR pszPath = Atom_GetName(this->ppagedata->atomPath);
    RA_ITEM * pitem;

    // This function shouldn't be called if this is an orphan
    ASSERT(S_OK == PageData_Query(this->ppagedata, hwnd, NULL, NULL));

    hres = RAI_Create(&pitem, Atom_GetName(Stat_AtomBrf(this)), pszPath, 
        prl, pftl);

    if (SUCCEEDED(hres))
        {
        if (RAIA_ORPHAN == pitem->uAction)
            {
            // This is a pending orphan
            PageData_Orphanize(this->ppagedata);
            hres = S_FALSE;
            }
        else if ( !this->ppagedata->bFolder )
            {
            pitem->mask |= RAIF_LPARAM;
            pitem->lParam = (LPARAM)prl->priFirst;
            }

        if (S_OK == hres)
            {
            BOOL bEnable;
            HWND hwndFocus = GetFocus();

            // Add the item to the recact control.
            RecAct_InsertItem(hwndRA, pitem);

            // Determine the state of the buttons
            bEnable = !(pitem->uAction == RAIA_SKIP ||
                        pitem->uAction == RAIA_CONFLICT || 
                        pitem->uAction == RAIA_NOTHING);
            Button_Enable(GetDlgItem(hwnd, IDC_PBTSRECON), bEnable);

            bEnable = (SI_UNAVAILABLE != pitem->siOutside.uState);
            Button_Enable(GetDlgItem(hwnd, IDC_PBTSFIND), bEnable);

            if ( !hwndFocus || !IsWindowEnabled(hwndFocus) )
                {
                SetFocus(GetDlgItem(hwnd, IDC_PBTSSPLIT));
                SendMessage(hwnd, DM_SETDEFID, IDC_PBTSSPLIT, 0);
                }
            }
        RAI_Free(pitem);
        }

    return hres;
    }


/*----------------------------------------------------------
Purpose: Sets the controls in the status property page.

Returns: --
Cond:    --
*/
void PRIVATE Stat_SetControls(
    PSTAT this)
    {
    HWND hwnd = this->hwnd;
    HRESULT hres;
    PRECLIST prl;
    PFOLDERTWINLIST pftl;

    // Is this a twin?
    hres = PageData_Query(this->ppagedata, hwnd, &prl, &pftl);
    if (S_OK == hres)
        {
        // Yes
        RecAct_DeleteAllItems(GetDlgItem(hwnd, IDC_UPDATEACTIONS));

        // Is it still a twin?
        hres = Stat_SetRecAct(this, prl, pftl);
        if (S_OK == hres)
            {
            // Yes
            Stat_SetDirections(this);
            }
        else if (S_FALSE == hres)
            {
            // No
            goto WipeOut;
            }
        }
    else if (S_FALSE == hres)
        {
        // No; disable the controls
        TCHAR sz[MAXBUFLEN];
WipeOut:

        Stat_DisableAll(this);

        // Is this a subfolder twin?
        if (IsSubfolderTwin(PageData_GetHbrf(this->ppagedata), 
            Atom_GetName(this->ppagedata->atomPath)))
            {
            // Yes; use subfolder twin message.
            SzFromIDS(IDS_STATPROP_SubfolderTwin, sz, ARRAYSIZE(sz));
            }
        else
            {
            // No; use orphan message.
            if (this->ppagedata->bFolder)
                SzFromIDS(IDS_STATPROP_OrphanFolder, sz, ARRAYSIZE(sz));
            else
                SzFromIDS(IDS_STATPROP_OrphanFile, sz, ARRAYSIZE(sz));
            }
        Static_SetText(GetDlgItem(hwnd, IDC_STTSDIRECT), sz);
        }
    }


/*----------------------------------------------------------
Purpose: Gets the icon of the file

Returns: HICON
Cond:    --
*/
HICON PRIVATE GetIconHelper(
    LPCTSTR pszPath)
    {
    SHFILEINFO sfi;

    if (SHGetFileInfo(pszPath, 0, &sfi, sizeof(sfi), SHGFI_ICON))
        {
        return sfi.hIcon;
        }
    return NULL;
    }


/*----------------------------------------------------------
Purpose: Stat WM_INITDIALOG Handler

Returns: FALSE when we assign the control focus
Cond:    --
*/
BOOL PRIVATE Stat_OnInitDialog(
    PSTAT this,
    HWND hwndFocus,
    LPARAM lParam)              // expected to be LPPROPSHEETPAGE
    {
    HWND hwnd = this->hwnd;
    LPCTSTR pszPath;

    this->ppagedata = (PPAGEDATA)((LPPROPSHEETPAGE)lParam)->lParam;

    // Set up the display of the dialog
    pszPath = Atom_GetName(this->ppagedata->atomPath);

    if (SUCCEEDED(FICreate(pszPath, &this->pfi, FIF_ICON)))
        {
        Static_SetIcon(GetDlgItem(hwnd, IDC_ICTSMAIN), this->pfi->hicon);
        Static_SetText(GetDlgItem(hwnd, IDC_NAME), FIGetDisplayName(this->pfi));
        }

    // Save the folder of the twin away.
    lstrcpy(this->szFolder, pszPath);
    if (!this->ppagedata->bFolder)
        PathRemoveFileSpec(this->szFolder);

    this->bInit = TRUE;

    return FALSE;   // we set the initial focus
    }


/*----------------------------------------------------------
Purpose: PSN_SETACTIVE handler
Returns: --
Cond:    --
*/
void PRIVATE Stat_OnSetActive(
    PSTAT this)
    {
    HWND hwnd = this->hwnd;

    // Cause the page to be painted right away 
    HideCaret(NULL);
    SetWindowRedraw(hwnd, TRUE);
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);

    if (this->bInit)
        {
        PageData_Init(this->ppagedata, GetParent(this->hwnd));
        this->bInit = FALSE;
        }

    ShowCaret(NULL);

    Stat_SetControls(this);
    }


/*----------------------------------------------------------
Purpose: Reconcile the twins in this property sheet.  
          For folder twins, we invoke the Update dialog.  
          For object twins, we reconcile from here.

Returns: --
Cond:    --
*/
void PRIVATE Stat_OnUpdate(
    PSTAT this,
    PRECLIST prl)
    {
    HWND hwnd = this->hwnd;
    HWND hwndRA = GetDlgItem(hwnd, IDC_UPDATEACTIONS);
    LPCTSTR pszPath = Atom_GetName(this->ppagedata->atomPath);

    if (0 == RecAct_GetItemCount(hwndRA))
        return;

    ASSERT(S_OK == PageData_Query(this->ppagedata, hwnd, NULL, NULL));

    // Is this a folder?
    if (this->ppagedata->bFolder)
        {
        // Yes; let the Update dialog do the work
        Upd_DoModal(hwnd, Stat_Pcbs(this), pszPath, 1, UF_SELECTION);
        }
    else
        {
        // No; we do the work.  
        HWND hwndProgress;

        hwndProgress = UpdBar_Show(hwnd, UB_UPDATING, 0);
            
        Sync_ReconcileRecList(prl, Atom_GetName(Stat_AtomBrf(this)), 
            hwndProgress, RF_DEFAULT);

        UpdBar_Kill(hwndProgress);
        }

    this->ppagedata->bRecalc = TRUE;

    PropSheet_CancelToClose(GetParent(hwnd));
    }


/*----------------------------------------------------------
Purpose: Separate the twins.  
Returns: --
Cond:    --
*/
void PRIVATE Stat_OnSplit(
    PSTAT this)
    {
    HWND hwnd = this->hwnd;
    HWND hwndRA = GetDlgItem(hwnd, IDC_UPDATEACTIONS);
    LPCTSTR pszPath = Atom_GetName(this->ppagedata->atomPath);

    if (0 == RecAct_GetItemCount(hwndRA))
        return;

    ASSERT(S_OK == PageData_Query(this->ppagedata, hwnd, NULL, NULL));

    // Was the twin successfully deleted?
    if (S_OK == Sync_Split(PageData_GetHbrf(this->ppagedata), pszPath, 1, hwnd, 0))
        {
        // Yes; remove the cache references
        CRL_Nuke(this->ppagedata->atomPath);
        Stat_DisableAll(this);

        PropSheet_CancelToClose(GetParent(hwnd));

        // Notify the shell of the change
        PathNotifyShell(pszPath, NSE_UPDATEITEM, FALSE);
        }
    }


/*----------------------------------------------------------
Purpose: Attempt to bind to an object to see if it exists.

Returns: TRUE if the object exists
Cond:    --
*/
BOOL PRIVATE VerifyExists(
    LPCITEMIDLIST pidlParent,
    LPCITEMIDLIST pidl)
    {
    BOOL bRet = FALSE;
    LPSHELLFOLDER psfDesktop;

    ASSERT(pidlParent);
    ASSERT(pidl);

    psfDesktop = GetDesktopShellFolder();
    if (psfDesktop)
        {
        LPSHELLFOLDER psf;
        HRESULT hres;

        hres = psfDesktop->lpVtbl->BindToObject(psfDesktop, pidlParent, NULL, &IID_IShellFolder, &psf);
        if (SUCCEEDED(hres))
            {
            ULONG rgfAttr = SFGAO_VALIDATE;
            bRet = SUCCEEDED(psf->lpVtbl->GetAttributesOf(psf, 1, &pidl, &rgfAttr));
            psf->lpVtbl->Release(psf);
            }
        }
    return bRet;
    }


/*----------------------------------------------------------
Purpose: Open a file (taken from ShellExecFile)

Returns: value of ShellExecuteEx

Cond:    --
*/
BOOL PUBLIC ExecFile(
    HWND hwnd, 
    LPCTSTR pszVerb, 
    LPCTSTR pszFile,        // Fully qualified and fully resolved path to the file
    LPCTSTR pszParams,
    LPCTSTR pszDir,         // If NULL then working dir is derived from lpszFile (except for UNC's)
    LPCITEMIDLIST pidl,
    int nShow)
    {
    SHELLEXECUTEINFO execinfo;

    execinfo.cbSize          = sizeof(execinfo);
    execinfo.hwnd            = hwnd;
    execinfo.lpVerb          = pszVerb;
    execinfo.lpFile          = pszFile;
    execinfo.lpParameters    = pszParams;
    execinfo.lpDirectory     = pszDir;
    execinfo.nShow           = nShow;
    execinfo.fMask           = 0;
    execinfo.lpIDList        = (LPITEMIDLIST)pidl;

    if (pidl)
        {
        execinfo.fMask |= SEE_MASK_IDLIST;
        }

    return ShellExecuteEx(&execinfo);
    }


/*----------------------------------------------------------
Purpose: Selects an item in the given cabinet window.  Optionally
         sets it to be renamed.

         This function does not verify if the window is really
         a cabinet window.

Returns: --
Cond:    --
*/
void PUBLIC SelectItemInCabinet(
    HWND hwndCabinet,
    LPCITEMIDLIST pidl,
    BOOL bEdit)
    {
    if (IsWindow(hwndCabinet)) 
        {
        if (pidl)
            {
            LPITEMIDLIST pidlItem;

            // we need to global clone this because hwndCabinet might be
            // in a different process...  could happen with common dialog
            pidlItem = ILGlobalClone(pidl);
            if (pidlItem) 
                {
                UINT uFlagsEx;

                if (bEdit)
                    uFlagsEx = SVSI_EDIT;
                else
                    uFlagsEx = 0;

                SendMessage(hwndCabinet, CWM_SELECTITEM, 
                    uFlagsEx | SVSI_SELECT | SVSI_ENSUREVISIBLE | 
                    SVSI_FOCUSED | SVSI_DESELECTOTHERS, 
                    (LPARAM)pidlItem);
                ILGlobalFree(pidlItem);
                }
            }
        }
    }


/*----------------------------------------------------------
Purpose: Open a cabinet window and set the focus on the object.

Returns: --
Cond:    --
*/
void PUBLIC OpenCabinet(
    HWND hwnd,
    LPCITEMIDLIST pidlFolder,
    LPCITEMIDLIST pidl,
    BOOL bEdit)             // TRUE: set the focus to edit the label
    {
    if (!VerifyExists(pidlFolder, pidl))
        {
        MsgBox(hwnd, MAKEINTRESOURCE(IDS_MSG_CantFindOriginal), MAKEINTRESOURCE(IDS_CAP_STATUS),
               NULL, MB_INFO);
        }
    else
        {
        HWND hwndCabinet;

        SHWaitForFileToOpen(pidlFolder, WFFO_ADD, 0L);
        if (ExecFile(hwnd, c_szOpen, NULL, NULL, NULL, pidlFolder, SW_NORMAL))
            {
            // This will wait for the window to open or time out
            // We need to disable the dialog box while we are waiting.
            DECLAREHOURGLASS;

            SetHourglass();
            EnableWindow(hwnd, FALSE);
            SHWaitForFileToOpen(pidlFolder, WFFO_REMOVE | WFFO_WAIT, WFFO_WAITTIME);
            EnableWindow(hwnd, TRUE);
            ResetHourglass();

            hwndCabinet = FindWindow(c_szCabinetClass, NULL);
            }
        else
            {
            // If it failed clear out our wait
            hwndCabinet = NULL;
            SHWaitForFileToOpen(pidlFolder, WFFO_REMOVE, 0L);
            }

        if (hwndCabinet)
            {
            SelectItemInCabinet(hwndCabinet, pidl, bEdit);

            // we need to post to the other because we can't activate another
            // thread from within a button's callback
            PostMessage(hwnd, LNKM_ACTIVATEOTHER, 0, (LPARAM)hwndCabinet);
            }
        }
    }


/*----------------------------------------------------------
Purpose: Opens the cabinet with the item pointed to by the twin.
         (copied and modified from link.c in shelldll)
Returns: --
Cond:    --
*/
void PRIVATE Stat_OnFind(
    PSTAT this)
    {
    HWND hwnd = this->hwnd;
    HWND hwndRA = GetDlgItem(hwnd, IDC_UPDATEACTIONS);
    RA_ITEM item;

    if (0 == RecAct_GetItemCount(hwndRA))
        return;

    ASSERT(S_OK == PageData_Query(this->ppagedata, hwnd, NULL, NULL));

    item.mask = RAIF_OUTSIDE | RAIF_NAME;
    item.iItem = 0;

    if (RecAct_GetItem(hwndRA, &item))
        {
        TCHAR szCanon[MAX_PATH];
        LPITEMIDLIST pidlFolder;
        LPITEMIDLIST pidl;

        // Use UNC name to find it on the net
        BrfPathCanonicalize(item.siOutside.pszDir, szCanon);
        pidlFolder = ILCreateFromPath(szCanon);
        if (pidlFolder)
            {
            pidl = ILCreateFromPath(item.pszName);
            if (pidl)
                {
                OpenCabinet(hwnd, pidlFolder, ILFindLastID(pidl), FALSE);
                ILFree(pidl);
                }
            ILFree(pidlFolder);
            }
        }
    }


/*----------------------------------------------------------
Purpose: Stat WM_COMMAND Handler
Returns: --
Cond:    --
*/
void PRIVATE Stat_OnCommand(
    PSTAT this,
    int id,
    HWND hwndCtl,
    UINT uNotifyCode)
    {
    PRECLIST prl;
    HRESULT hres;

    switch (id)
        {
    case IDC_PBTSRECON:
    case IDC_PBTSFIND:
    case IDC_PBTSSPLIT: 
        RETRY_BEGIN(FALSE)
            {
            hres = PageData_Query(this->ppagedata, this->hwnd, &prl, NULL);
            if (FAILED(hres))
                {
                // Error

                // Unavailable disk?
                if (E_TR_UNAVAILABLE_VOLUME == hres)
                    {
                    // Yes; ask user to retry/cancel
                    int id = MsgBox(this->hwnd, MAKEINTRESOURCE(IDS_ERR_UNAVAIL_VOL),
                        MAKEINTRESOURCE(IDS_CAP_STATUS), NULL, MB_RETRYCANCEL | MB_ICONWARNING);

                    if (IDRETRY == id)
                        RETRY_SET();    // Try again
                    }
                }
            }
        RETRY_END()
        
        // Is this a twin?
        if (S_OK == hres)
            {
            // Yes; do the operation
            switch (id)
                {
            case IDC_PBTSRECON:
                Stat_OnUpdate(this, prl);
                break;
            
            case IDC_PBTSFIND:
                Stat_OnFind(this);
                break;
            
            case IDC_PBTSSPLIT:
                Stat_OnSplit(this);
                break;
                }
            Stat_SetControls(this);
            }
        else if (S_FALSE == hres)
            {
            Stat_SetControls(this);
            }

        break;
        }
    }


/*----------------------------------------------------------
Purpose: Handle RN_ITEMCHANGED
Returns: --
Cond:    --
*/
void PRIVATE Stat_HandleItemChange(
    PSTAT this,
    NM_RECACT  * lpnm)
    {
    PRECITEM pri;

    ASSERT((lpnm->mask & RAIF_LPARAM) != 0);

    pri = (PRECITEM)lpnm->lParam;

    // The action has changed, update the recnode accordingly
    //
    if (lpnm->mask & RAIF_ACTION)
        {
        BOOL bEnable;
        HWND hwndFocus = GetFocus();

        Sync_ChangeRecItemAction(pri, Atom_GetName(Stat_AtomBrf(this)), 
            this->szFolder, lpnm->uAction);

        bEnable = (RAIA_SKIP != lpnm->uAction && RAIA_CONFLICT != lpnm->uAction);
        Button_Enable(GetDlgItem(this->hwnd, IDC_PBTSRECON), bEnable);

        if ( !hwndFocus || !IsWindowEnabled(hwndFocus) )
            {
            SetFocus(GetDlgItem(this->hwnd, IDC_PBTSSPLIT));
            SendMessage(this->hwnd, DM_SETDEFID, IDC_PBTSSPLIT, 0);
            }
        }
    }    


/*----------------------------------------------------------
Purpose: WM_NOTIFY handler
Returns: varies
Cond:    --
*/
LRESULT PRIVATE Stat_OnNotify(
    PSTAT this,
    int idFrom,
    NMHDR  * lpnmhdr)
    {
    LRESULT lRet = PSNRET_NOERROR;
    
    switch (lpnmhdr->code)
        {
    case RN_ITEMCHANGED:
        Stat_HandleItemChange(this, (NM_RECACT  *)lpnmhdr);
        break;

    case PSN_SETACTIVE:
        Stat_OnSetActive(this);
        break;

    case PSN_KILLACTIVE:
        // N.b. This message is not sent if user clicks Cancel!
        // N.b. This message is sent prior to PSN_APPLY
        //
        break;

    case PSN_APPLY:
        break;

    default:
        break;
        }

    return lRet;
    }


/*----------------------------------------------------------
Purpose: WM_DESTROY handler
Returns: --
Cond:    --
*/
void PRIVATE Stat_OnDestroy(
    PSTAT this)
    {
    FIFree(this->pfi);
    }


/////////////////////////////////////////////////////  EXPORTED FUNCTIONS

static BOOL s_bStatRecurse = FALSE;

LRESULT INLINE Stat_DefProc(
    HWND hDlg, 
    UINT msg,
    WPARAM wParam,
    LPARAM lParam) 
    {
    ENTEREXCLUSIVE()
        {
        s_bStatRecurse = TRUE;
        }
    LEAVEEXCLUSIVE()

    return DefDlgProc(hDlg, msg, wParam, lParam); 
    }


/*----------------------------------------------------------
Purpose: Real Create Folder Twin dialog proc
Returns: varies
Cond:    --
*/
LRESULT Stat_DlgProc(
    PSTAT this,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
    {
#pragma data_seg(DATASEG_READONLY)
    const static DWORD rgHelpIDs[] = {
        IDC_ICTSMAIN,       IDH_BFC_PROP_FILEICON,
        IDC_NAME,           IDH_BFC_PROP_FILEICON,
        IDC_STTSDIRECT,     IDH_BFC_UPDATE_SCREEN,
        IDC_UPDATEACTIONS,  IDH_BFC_UPDATE_SCREEN,      // different
        IDC_PBTSRECON,      IDH_BFC_UPDATE_BUTTON,
        IDC_PBTSSPLIT,      IDH_BFC_PROP_SPLIT_BUTTON,
        IDC_PBTSFIND,       IDH_BFC_PROP_FINDORIG_BUTTON,
        0, 0 };
#pragma data_seg()
    DWORD_PTR dw;

    switch (message)
        {
        HANDLE_MSG(this, WM_INITDIALOG, Stat_OnInitDialog);
        HANDLE_MSG(this, WM_COMMAND, Stat_OnCommand);
        HANDLE_MSG(this, WM_NOTIFY, Stat_OnNotify);
        HANDLE_MSG(this, WM_DESTROY, Stat_OnDestroy);

    case WM_HELP:
        dw = (DWORD_PTR)rgHelpIDs;

        if ( IDC_STATIC != ((LPHELPINFO)lParam)->iCtrlId )
            WinHelp(((LPHELPINFO)lParam)->hItemHandle, c_szWinHelpFile, HELP_WM_HELP, dw);
        return 0;

    case WM_CONTEXTMENU:
        dw = (DWORD_PTR)rgHelpIDs;

        WinHelp((HWND)wParam, c_szWinHelpFile, HELP_CONTEXTMENU, dw);
        return 0;

    case LNKM_ACTIVATEOTHER:
        SwitchToThisWindow(GetLastActivePopup((HWND)lParam), TRUE);
        SetForegroundWindow((HWND)lParam);
        return 0;
        
    default:
        return Stat_DefProc(this->hwnd, message, wParam, lParam);
        }
    return 0;
    }


/*----------------------------------------------------------
Purpose: Create Folder Twin Dialog Wrapper
Returns: varies
Cond:    --
*/
INT_PTR CALLBACK Stat_WrapperProc(
    HWND hDlg,          // std params
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
    {
    PSTAT this;

    // Cool windowsx.h dialog technique.  For full explanation, see
    //  WINDOWSX.TXT.  This supports multiple-instancing of dialogs.
    //
    ENTEREXCLUSIVE()
        {
        if (s_bStatRecurse)
            {
            s_bStatRecurse = FALSE;
            LEAVEEXCLUSIVE()
            return FALSE;
            }
        }
    LEAVEEXCLUSIVE()

    this = Stat_GetPtr(hDlg);
    if (this == NULL)
        {
        if (message == WM_INITDIALOG)
            {
            this = GAlloc(sizeof(*this));
            if (!this)
                {
                MsgBox(hDlg, MAKEINTRESOURCE(IDS_OOM_STATUS), MAKEINTRESOURCE(IDS_CAP_STATUS),
                       NULL, MB_ERROR);
                EndDialog(hDlg, IDCANCEL);
                return Stat_DefProc(hDlg, message, wParam, lParam);
                }
            this->hwnd = hDlg;
            Stat_SetPtr(hDlg, this);
            }
        else
            {
            return Stat_DefProc(hDlg, message, wParam, lParam);
            }
        }

    if (message == WM_DESTROY)
        {
        Stat_DlgProc(this, message, wParam, lParam);
        GFree(this);
        Stat_SetPtr(hDlg, NULL);
        return 0;
        }

    return SetDlgMsgResult(hDlg, message, Stat_DlgProc(this, message, wParam, lParam));
    }
