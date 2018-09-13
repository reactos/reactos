//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: update.c
//
// This files contains code for the Update UI and dialog.
//
// History:
//  08-17-93 ScottH     Created.
//
//---------------------------------------------------------------------------


#include "brfprv.h"     // common headers

#include "res.h"
#include "recact.h"
#ifdef WINNT
#include <help.h>
#else
#include "..\..\..\win\core\inc\help.h"   // help IDs
#endif


// This structure contains all the important counts
// that determine the specific course of action when
// the user wants to update something.
typedef struct
    {
    // These are 1 to 1
    UINT    cFiles;
    UINT    cOrphans;
    UINT    cSubfolders;

    // These are 1 to 1
    UINT    cUnavailable;
    UINT    cDoSomething;
    UINT    cConflict;
    UINT    cTombstone;
    } UPDCOUNT;

// This is the structure passed to the dialog at WM_INITDIALOG
typedef struct
    {
    PRECLIST lprl;              // Supplied reclist
    CBS *    pcbs;
    UINT     uFlags;            // UF_ Flags
    HDPA     hdpa;              // List of RA_ITEMs
    UINT     cDoSomething;
    } XUPDSTRUCT,  * LPXUPDSTRUCT;


typedef struct tagUPD
    {
    HWND hwnd;              // dialog handle

    LPXUPDSTRUCT pxupd;

    } UPD, * PUPD;

#define Upd_Prl(this)           ((this)->pxupd->lprl)
#define Upd_AtomBrf(this)       ((this)->pxupd->pcbs->atomBrf)
#define Upd_GetBrfPtr(this)     Atom_GetName(Upd_AtomBrf(this))

#define Upd_GetPtr(hwnd)        (PUPD)GetWindowLongPtr(hwnd, DWLP_USER)
#define Upd_SetPtr(hwnd, lp)    (PUPD)SetWindowLongPtr(hwnd, DWLP_USER, (LRESULT)(lp))

// These flags are used for DoUpdateMsg
#define DUM_ALL             0x0001
#define DUM_SELECTION       0x0002
#define DUM_ORPHAN          0x0004
#define DUM_UPTODATE        0x0008
#define DUM_UNAVAILABLE     0x0010
#define DUM_SUBFOLDER_TWIN  0x0020

// These flags are returned by PassedSpecialCases
#define PSC_SHOWDIALOG      0x0001
#define PSC_POSTMSGBOX      0x0002


//---------------------------------------------------------------------------
// Some comments
//---------------------------------------------------------------------------

// There are several special cases and conditions which must be 
// handled.  Lets break them down now.  The case numbers on on the far
// right, and are referenced in comments thru out this file.
//
// There are two user actions: Update All and Update Selection.
//
// Update All:
// 
//               Case                                     What to do
//         -----------------                            --------------
//  A1.  * No files in the briefcase                --->  MB (messagebox)
//  A2.  * All files are orphans                    --->  MB
//       * Some files are twins and...
//          * they are all available and...
//  A3.        * they are all up-to-date            --->  MB
//  A4.        * some of them need updating         --->  Update dialog
//          * some are unavailable and...
//  A5.        * the available ones are up-to-date  --->  MB then Update 
//  A6.        * some need updating                 --->  MB then Update 
// 
//
// Update Selection:
//
//               Case                                     What to do
//         -----------------                            --------------
//       * Single selection and...
//  S1.     * is an orphan                          --->  MB
//          * is available and...
//  S2.        * up-to-date                         --->  MB
//  S3.        * needs updating                     --->  Update 
//  S4.     * is unavailable                        --->  MB then Update 
//       * Multi selection and...
//  S5.     * all are orphans                       --->  MB
//          * some (non-orphans) are unavailable and...
//             * some need updating and...
//  S6.           * none are orphans                --->  MB then Update 
//  S7.           * some are orphans                --->  MB then Update then MB
//             * the available ones are up-to-date and...
//  S8.           * none are orphans                --->  MB then Update 
//  S9.           * some are orphans                --->  MB then Update then MB
//          * all (non-orphans) are available and...
//             * some need updating and...
// S10.           * none are orphans                --->  Update 
// S11.           * some are orphans                --->  Update then MB
//             * all up-to-date and...
// S12.           * none are orphans                --->  MB
// S13.           * some are orphans                --->  MB



//---------------------------------------------------------------------------
// Dialog code
//---------------------------------------------------------------------------

/*----------------------------------------------------------
Purpose: Fill the reconciliation action listbox
Returns: TRUE on success
Cond:    --
*/
BOOL PRIVATE Upd_FillList(
    PUPD this)
    {
    HWND hwndCtl = GetDlgItem(this->hwnd, IDC_UPDATEACTIONS);
    HDPA hdpa = this->pxupd->hdpa;
    int cItems;
    int i;

    cItems = DPA_GetPtrCount(hdpa);
    for (i = 0; i < cItems; i++)
        {
        RA_ITEM * pitem = DPA_FastGetPtr(hdpa, i);
        RecAct_InsertItem(hwndCtl, pitem);
        RAI_Free(pitem);
        }

    return TRUE;
    }


/*----------------------------------------------------------
Purpose: Sets the Update and Cancel buttons according to the
         bDisableUpdate parameter.

Returns: --
Cond:    --
*/
void PRIVATE Upd_SetExitButtons(
    PUPD this,
    BOOL bDisableUpdate)
    {
    HWND hwndOK = GetDlgItem(this->hwnd, IDOK);

    // Disable the update button?
    if (bDisableUpdate)
        {
        // Yes
        if (GetFocus() == hwndOK)
            {
            SetFocus(GetDlgItem(this->hwnd, IDCANCEL));
            }
        Button_Enable(hwndOK, FALSE);
        }
    else
        {
        // No
        Button_Enable(hwndOK, TRUE);
        }
    }
    

/*----------------------------------------------------------
Purpose: WM_INITDIALOG Handler
Returns: 
Cond:    --
*/
BOOL PRIVATE Upd_OnInitDialog(
    PUPD this,
    HWND hwndFocus,
    LPARAM lParam)
    {
    HWND hwnd = this->hwnd;
    TCHAR szFmt[MAXBUFLEN];
    TCHAR sz[MAXMSGLEN];

    ASSERT(lParam != 0L);

    this->pxupd = (LPXUPDSTRUCT)lParam;

    if (Upd_FillList(this))
        {
        // Set the title caption
        wsprintf(sz, SzFromIDS(IDS_CAP_UpdateFmt, szFmt, ARRAYSIZE(szFmt)),
                 PathFindFileName(Upd_GetBrfPtr(this)));
        SetWindowText(hwnd, sz);

        // Do any files need updating?
        if (0 == this->pxupd->cDoSomething)
            {
            // No
            Upd_SetExitButtons(this, TRUE);
            }
        }
    else
        {
        // Failed
        EndDialog(hwnd, -1);
        }
    return(TRUE);
    }


/*----------------------------------------------------------
Purpose: Handle RN_ITEMCHANGED
Returns: --
Cond:    --
*/
void PRIVATE Upd_HandleItemChange(
    PUPD this,
    NM_RECACT  * lpnm)
    {
    PRECITEM lpri;

    ASSERT((lpnm->mask & RAIF_LPARAM) != 0);

    lpri = (PRECITEM)lpnm->lParam;

    // The action has changed, update the recnode accordingly
    if (lpnm->mask & RAIF_ACTION)
        {
        LPCTSTR pszDir = Upd_GetBrfPtr(this);
        Sync_ChangeRecItemAction(lpri, pszDir, pszDir, lpnm->uAction);

        switch (lpnm->uActionOld)
            {
        case RAIA_TOOUT:
        case RAIA_TOIN:
        case RAIA_MERGE:
            // Is this a change from "do something" to "skip"?
            if (RAIA_SKIP == lpnm->uAction)
                {
                // Yes
                ASSERT(0 < this->pxupd->cDoSomething);
                this->pxupd->cDoSomething--;
                }
            break;

        case RAIA_SKIP:
        case RAIA_CONFLICT:
            // Is this a change from "skip"/"conflict" to "do something"?
            if (RAIA_TOOUT == lpnm->uAction ||
                RAIA_TOIN == lpnm->uAction ||
                RAIA_MERGE == lpnm->uAction)
                {
                // Yes
                this->pxupd->cDoSomething++;
                }
            break;
            }

        Upd_SetExitButtons(this, 0 == this->pxupd->cDoSomething);
        }
    }


/*----------------------------------------------------------
Purpose: WM_NOTIFY handler
Returns: varies
Cond:    --
*/
LRESULT PRIVATE Upd_OnNotify(
    PUPD this,
    int idFrom,
    NMHDR  * lpnmhdr)
    {
    LRESULT lRet = 0;
    
    switch (lpnmhdr->code)
        {
    case RN_ITEMCHANGED:
        Upd_HandleItemChange(this, (NM_RECACT  *)lpnmhdr);
        break;
        
    default:
        break;
        }
    
    return lRet;
    }


/*----------------------------------------------------------
Purpose: Info WM_COMMAND Handler
Returns: --
Cond:    --
*/
VOID PRIVATE Upd_OnCommand(
    PUPD this,
    int id,
    HWND hwndCtl,
    UINT uNotifyCode)
    {
    HWND hwnd = this->hwnd;
    
    switch (id)
        {
    case IDOK:
    case IDCANCEL:
        EndDialog(hwnd, id);
        break;
        }
    }


/*----------------------------------------------------------
Purpose: WM_DESTROY handler
Returns: --
Cond:    --
*/
void PRIVATE Upd_OnDestroy(
    PUPD this)
    {
    }


static BOOL s_bUpdRecurse = FALSE;

LRESULT INLINE Upd_DefProc(
    HWND hDlg, 
    UINT msg,
    WPARAM wParam,
    LPARAM lParam) 
    {
    ENTEREXCLUSIVE()
        {
        s_bUpdRecurse = TRUE;
        }
    LEAVEEXCLUSIVE()

    return DefDlgProc(hDlg, msg, wParam, lParam); 
    }


/*----------------------------------------------------------
Purpose: Real Create Folder Twin dialog proc
Returns: varies
Cond:    --
*/
LRESULT Upd_DlgProc(
    PUPD this,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
    {
#pragma data_seg(DATASEG_READONLY)
    const static DWORD rgHelpIDs[] = {
        IDC_UPDATEACTIONS,  IDH_BFC_UPDATE_SCREEN,      // different
        IDOK,               IDH_BFC_UPDATE_BUTTON,
        0, 0 };
#pragma data_seg()

    switch (message)
        {
        HANDLE_MSG(this, WM_INITDIALOG, Upd_OnInitDialog);
        HANDLE_MSG(this, WM_COMMAND, Upd_OnCommand);
        HANDLE_MSG(this, WM_NOTIFY, Upd_OnNotify);
        HANDLE_MSG(this, WM_DESTROY, Upd_OnDestroy);

    case WM_HELP:
        WinHelp(((LPHELPINFO)lParam)->hItemHandle, c_szWinHelpFile, HELP_WM_HELP, (DWORD_PTR)(LPVOID)rgHelpIDs);
        return 0;

    case WM_CONTEXTMENU:
        WinHelp((HWND)wParam, c_szWinHelpFile, HELP_CONTEXTMENU, (DWORD_PTR)(LPVOID)rgHelpIDs);
        return 0;

    default:
        return Upd_DefProc(this->hwnd, message, wParam, lParam);
        }
    }


/*----------------------------------------------------------
Purpose: Create Folder Twin Dialog Wrapper
Returns: varies
Cond:    --
*/
INT_PTR _export CALLBACK Upd_WrapperProc(
    HWND hDlg,      // std params
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
    {
    PUPD this;
    
    // Cool windowsx.h dialog technique.  For full explanation, see
    //  WINDOWSX.TXT.  This supports multiple-instancing of dialogs.
    //
    ENTEREXCLUSIVE()
        {
        if (s_bUpdRecurse)
            {
            s_bUpdRecurse = FALSE;
            LEAVEEXCLUSIVE()
            return FALSE;
            }
        }
    LEAVEEXCLUSIVE()

    this = Upd_GetPtr(hDlg);
    if (this == NULL)
        {
        if (message == WM_INITDIALOG)
            {
            this = GAlloc(sizeof(*this));
            if (!this)
                {
                MsgBox(hDlg, MAKEINTRESOURCE(IDS_OOM_UPDATEDIALOG), MAKEINTRESOURCE(IDS_CAP_UPDATE),
                       NULL, MB_ERROR);
                EndDialog(hDlg, IDCANCEL);
                return Upd_DefProc(hDlg, message, wParam, lParam);
                }
            this->hwnd = hDlg;
            Upd_SetPtr(hDlg, this);
            }
        else
            {
            return Upd_DefProc(hDlg, message, wParam, lParam);
            }
        }
    
    if (message == WM_DESTROY)
        {
        Upd_DlgProc(this, message, wParam, lParam);
        GFree(this);
        Upd_SetPtr(hDlg, NULL);
        return 0;
        }
    
    return SetDlgMsgResult(hDlg, message, Upd_DlgProc(this, message, wParam, lParam));
    }


//---------------------------------------------------------------------------
// Update detection code
//---------------------------------------------------------------------------


/*----------------------------------------------------------
Purpose: Checks if the briefcase is empty.  This function skips the
         "desktop.ini" and "Briefcase Database" files.

Returns: TRUE if the briefcase is empty

Cond:    --
*/
BOOL PRIVATE IsBriefcaseEmpty(
    LPCTSTR pszPath)
    {
    BOOL bRet = FALSE;

    ASSERT(pszPath);

    if (pszPath)
        {
        // Enumerate thru folder
        TCHAR szSearch[MAXPATHLEN];
        WIN32_FIND_DATA fd;
        HANDLE hfile;
#pragma data_seg(DATASEG_PERINSTANCE)
        // This must be per instance, else it will cause a fixup in 
        // shared data segment.
        const static LPCTSTR s_rgszIgnore[] = { TEXT("."), TEXT(".."), g_szDBName, g_szDBNameShort, c_szDesktopIni };
#pragma data_seg()

        PathCombine(szSearch, pszPath, TEXT("*.*"));
        hfile = FindFirstFile(szSearch, &fd);
        if (INVALID_HANDLE_VALUE != hfile)
            {
            BOOL bCont = TRUE;

            bRet = TRUE;        // Default to empty folder
            while (bCont)
                {
                int bIgnore = FALSE;
                int i;

                // Is this file one of the files to ignore?
                for (i = 0; i < ARRAYSIZE(s_rgszIgnore); i++)
                    {
                    if (IsSzEqual(fd.cFileName, s_rgszIgnore[i]))
                        {
                        // Yes
                        bIgnore = TRUE;
                        break;
                        }
                    }

                // Is this a valid file/folder?
                if (FALSE == bIgnore)
                    {
                    // Yes; return the briefcase is not empty
                    bRet = FALSE;
                    bCont = FALSE;  // stop the enumeration
                    }
                else
                    {
                    bCont = FindNextFile(hfile, &fd);
                    }
                }

            FindClose(hfile);
            }
        }

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Create a DSA of RA_ITEMs

         Sets the cDoSomething, cUnavailable, cConflict and 
         cTombstone fields of pupdcount.

Returns: TRUE on success
Cond:    --
*/
HDPA PRIVATE ComposeUpdateList(
    PCBS pcbs,
    PRECLIST prl,
    UPDCOUNT * pupdcount,
    HWND hwndOwner)
    {
    HRESULT hres;
    HDPA hdpa;

    ASSERT(prl);

    hdpa = DPA_Create(20);
    if (NULL != hdpa)
        {
        LPRA_ITEM pitem;
        PRECITEM pri;
        LPCTSTR pszBrf = Atom_GetName(pcbs->atomBrf);

        ASSERT(pszBrf);

        DEBUG_CODE( Sync_DumpRecList(TR_SUCCESS, prl, TEXT("ComposeUpdateList")); )

        pupdcount->cUnavailable = 0;
        pupdcount->cDoSomething = 0;
        pupdcount->cTombstone = 0;
        pupdcount->cConflict = 0;
        
        for (pri = prl->priFirst; pri; pri = pri->priNext)
            {
            hres = RAI_CreateFromRecItem(&pitem, pszBrf, pri);
            if (SUCCEEDED(hres))
                {
                // Is this a NOP?
                if (RAIA_NOTHING == pitem->uAction ||
                    RAIA_ORPHAN == pitem->uAction)
                    {
                    // Yes; skip these guys altogether
                    }
                else
                    {
                    pitem->mask |= RAIF_LPARAM;
                    pitem->lParam = (LPARAM)pri;

#ifndef NEW_REC
                    // Has the file inside or outside the briefcase been deleted?
                    if (SI_DELETED == pitem->siInside.uState ||
                        SI_DELETED == pitem->siOutside.uState)
                        {
                        // Yes
                        pupdcount->cTombstone++;
                        }
                    else
#endif
                    // Is this a file entry?
                    if (IsFileRecItem(pri))
                        {
                        // Yes; add the item to the list.  
                        pitem->iItem = 0x7fff;
                        DPA_InsertPtr(hdpa, DPA_APPEND, pitem);

                        // Is this unavailable?
                        if (RAIA_SKIP == pitem->uAction)
                            {
                            // Yes
                            ASSERT(SI_UNAVAILABLE == pitem->siInside.uState ||
                                   SI_UNAVAILABLE == pitem->siOutside.uState ||
                                   SI_NOEXIST == pitem->siInside.uState ||
                                   SI_NOEXIST == pitem->siOutside.uState);
                            pupdcount->cUnavailable++;
                            }
                        else if (RAIA_CONFLICT == pitem->uAction)
                            {
                            pupdcount->cConflict++;
                            }
                        else 
                            {
                            pupdcount->cDoSomething++;
                            }

                        // (prevent pitem from being freed until 
                        // the dialog fills its list in Upd_FillList)
                        pitem = NULL;
                        }
                    }

                RAI_Free(pitem);
                }
            }
        }

    return hdpa;
    }


/*----------------------------------------------------------
Purpose: Displays a messagebox error specific to updating files

Returns: id of button
Cond:    --
*/
int PRIVATE DoUpdateMsg(
    HWND hwndOwner,
    LPCTSTR pszPath,
    UINT cFiles,
    UINT uFlags)            // DUM_ flags
    {
    UINT ids;
    UINT idi;
    int idRet;

    // Is this for Update All?
    if (IsFlagSet(uFlags, DUM_ALL))
        {
        // Yes
        idi = IDI_UPDATE_MULT;
        if (IsFlagSet(uFlags, DUM_ORPHAN))
            {
            // In this case, pszPath should be the briefcase root
            ASSERT(pszPath);

            if (IsBriefcaseEmpty(pszPath))
                ids = IDS_MSG_NoFiles;
            else
                ids = IDS_MSG_AllOrphans;
            }
        else if (IsFlagSet(uFlags, DUM_UPTODATE))
            ids = IDS_MSG_AllUptodate;
        else if (IsFlagSet(uFlags, DUM_UNAVAILABLE))
            ids = IDS_MSG_AllSomeUnavailable;
        else
            {
            ASSERT(0);  // should never get here
            ids = (UINT)-1;
            }

        idRet = MsgBox(hwndOwner, 
                    MAKEINTRESOURCE(ids), 
                    MAKEINTRESOURCE(IDS_CAP_UPDATE), 
                    LoadIcon(g_hinst, MAKEINTRESOURCE(idi)), 
                    MB_INFO);
        }
    else
        {
        // No
        TCHAR sz[MAX_PATH];

        ASSERT(0 != cFiles);
        ASSERT(pszPath);

        // Is this a single selection?
        if (1 == cFiles)
            {
            // Yes; assume it is a folder, then decrement the count
            // of the ids it is a file 
            if (IsFlagSet(uFlags, DUM_ORPHAN))
                ids = IDS_MSG_FolderOrphan;
            else if (IsFlagSet(uFlags, DUM_UPTODATE))
                ids = IDS_MSG_FolderUptodate;
            else if (IsFlagSet(uFlags, DUM_UNAVAILABLE))
                ids = IDS_MSG_FolderUnavailable;
            else if (IsFlagSet(uFlags, DUM_SUBFOLDER_TWIN))
                ids = IDS_MSG_FolderSubfolder;
            else
                {
                ASSERT(0);  // should never get here
                ids = (UINT)-1;
                }

            if (FALSE == PathIsDirectory(pszPath))
                {
                ASSERT(IsFlagClear(uFlags, DUM_SUBFOLDER_TWIN));
                ids--;      // use file-oriented messages
                idi = IDI_UPDATE_FILE;
                }
            else
                {
                idi = IDI_UPDATE_FOLDER;
                }

            idRet = MsgBox(hwndOwner, 
                        MAKEINTRESOURCE(ids), 
                        MAKEINTRESOURCE(IDS_CAP_UPDATE), 
                        LoadIcon(g_hinst, MAKEINTRESOURCE(idi)), 
                        MB_INFO,
                        PathGetDisplayName(pszPath, sz));
            }
        else
            {
            // No; multi selection
            idi = IDI_UPDATE_MULT;

            if (IsFlagSet(uFlags, DUM_UPTODATE))
                {
                if (IsFlagSet(uFlags, DUM_ORPHAN))
                    ids = IDS_MSG_MultiUptodateOrphan;
                else
                    ids = IDS_MSG_MultiUptodate;
                }
            else if (IsFlagSet(uFlags, DUM_ORPHAN))
                ids = IDS_MSG_MultiOrphans;
            else if (IsFlagSet(uFlags, DUM_UNAVAILABLE))
                ids = IDS_MSG_MultiUnavailable;
            else if (IsFlagSet(uFlags, DUM_SUBFOLDER_TWIN))
                ids = IDS_MSG_MultiSubfolder;
            else
                {
                ASSERT(0);  // should never get here
                ids = (UINT)-1;
                }

            idRet = MsgBox(hwndOwner, 
                        MAKEINTRESOURCE(ids), 
                        MAKEINTRESOURCE(IDS_CAP_UPDATE), 
                        LoadIcon(g_hinst, MAKEINTRESOURCE(idi)), 
                        MB_INFO,
                        cFiles);
            }
        }

    return idRet;
    }


/*----------------------------------------------------------
Purpose: This function does some preliminary checks to determine
         whether the dialog box needs to be invoked at all.

         Sets the cOrphans and cSubfolders fields of pupdcount.

Returns: standard result
Cond:    --
*/
HRESULT PRIVATE PrepForUpdateAll(
    PCBS pcbs,
    PRECLIST * pprl,
    UPDCOUNT * pupdcount,
    HWND hwndProgress)
    {
    HRESULT hres = E_FAIL;
    TWINRESULT tr;
    HWND hwndOwner = GetParent(hwndProgress);
    BOOL bAnyTwins;

    pupdcount->cSubfolders = 0;

    // Are there any twins in the database?

    tr = Sync_AnyTwins(pcbs->hbrf, &bAnyTwins);
    if (TR_SUCCESS == tr)
        {
        if (FALSE == bAnyTwins)
            {
            // No
            DoUpdateMsg(hwndOwner, Atom_GetName(pcbs->atomBrf), 1, DUM_ALL | DUM_ORPHAN);
            hres = S_FALSE;
            }

        // Can we get a fresh reclist?
        else 
            {
            pupdcount->cOrphans = 0;
            hres = Sync_CreateCompleteRecList(pcbs->hbrf, UpdBar_GetAbortEvt(hwndProgress), pprl);
            if (FAILED(hres))
                {
                // No
                if (E_TR_ABORT != hres)
                    {
                    MsgBox(hwndOwner, MAKEINTRESOURCE(IDS_OOM_UPDATEDIALOG), 
                        MAKEINTRESOURCE(IDS_CAP_UPDATE), NULL, MB_ERROR);
                    }
                }
            else
                {
                // Yes
                if (*pprl)
                    {
                    hres = S_OK;
                    }
                else
                    {
                    hres = E_UNEXPECTED;
                    }
                // (reclist is freed inFinishUpdate())
                }
            }
        }

    return hres;
    }


/*----------------------------------------------------------
Purpose: This function does some preliminary checks to determine
         whether the dialog box needs to be invoked at all.

         Sets the cOrphans and cSubfolders fields of pupdcount.

Returns: standard result
Cond:    --
*/
HRESULT PRIVATE PrepForUpdateSelection(
    PCBS pcbs,
    PRECLIST  * pprl,
    LPCTSTR pszList,
    UINT cFiles,
    UPDCOUNT * pupdcount,
    HWND hwndProgress)
    {
    HRESULT hres;
    TWINRESULT tr;
    HTWINLIST htl;
    HWND hwndOwner = GetParent(hwndProgress);

    pupdcount->cSubfolders = 0;

    // Create a twin list
    tr = Sync_CreateTwinList(pcbs->hbrf, &htl);

    if (TR_SUCCESS != tr)
        {
        // Failure
        MsgBox(hwndOwner, MAKEINTRESOURCE(IDS_OOM_UPDATEDIALOG), MAKEINTRESOURCE(IDS_CAP_UPDATE),
               NULL, MB_ERROR);
        hres = E_OUTOFMEMORY;
        }
    else
        {
        LPCTSTR psz;
        UINT cOrphans = 0;
        UINT cSubfolders = 0;
        UINT i;

        for (i = 0, psz = pszList; i < cFiles; i++)
            {
            // Is this object really a twin?
            if (S_FALSE == Sync_IsTwin(pcbs->hbrf, psz, 0) )
                {
                // No; is this a subfolder twin?
                if (IsSubfolderTwin(pcbs->hbrf, psz))
                    {
                    // Yes
                    cSubfolders++;
                    }
                else
                    {
                    // No
                    cOrphans++;
                    }
                }
            else 
                {
                // Yes; add it to the twin list
                Sync_AddPathToTwinList(pcbs->hbrf, htl, psz, NULL);
                }

            DataObj_NextFile(psz);      // Set psz to next file in list
            }

        // Are all the selected objects orphans?
        if (cOrphans < cFiles)
            {
            // No; create the reclist 
            hres = Sync_CreateRecListEx(htl, UpdBar_GetAbortEvt(hwndProgress), pprl);
            }
        else
            {
            // Yes
            DoUpdateMsg(hwndOwner, pszList, cFiles, DUM_SELECTION | DUM_ORPHAN);
            hres = S_FALSE;
            }
        pupdcount->cOrphans = cOrphans;
        pupdcount->cSubfolders = cSubfolders;
        Sync_DestroyTwinList(htl);          // Don't need this anymore
        }

    return hres;
    }


/*----------------------------------------------------------
Purpose: Checks for the special cases that are listed at the top 
         of this file.

Returns: PSC_ flags

Cond:    --
*/
UINT PRIVATE PassedSpecialCases(
    HWND hwndOwner,
    LPCTSTR pszList,
    UPDCOUNT * pupdcount,
    UINT uFlags)        // UF_ flags
    {
    UINT uRet = 0;
    UINT dum = 0;
    UINT cSomeAction = pupdcount->cDoSomething + pupdcount->cConflict;

    // Is this Update All?
    if (IsFlagSet(uFlags, UF_ALL))
        {
        // Yes
        if (0 < pupdcount->cOrphans)
            {
            // Case A2
            dum = DUM_ALL | DUM_ORPHAN;
            }
        else if (0 == pupdcount->cUnavailable)
            {
            if (0 == cSomeAction)
                {
                // Case A3
                dum = DUM_ALL | DUM_UPTODATE;
                }
            else
                {
                // Case A4
                uRet = PSC_SHOWDIALOG;
                }
            }
        else
            {
            // Cases A5 and A6
            dum = DUM_ALL | DUM_UNAVAILABLE;
            uRet = PSC_SHOWDIALOG;
            }

#ifdef DEBUG

        if (IsFlagSet(g_uDumpFlags, DF_UPDATECOUNT))
            {
            TRACE_MSG(TF_ALWAYS, TEXT("Update All counts: files = %u, orphans = %u, unavailable = %u, dosomething = %u, conflict = %u, subfolders = %u"),
                        pupdcount->cFiles, pupdcount->cOrphans, 
                        pupdcount->cUnavailable, pupdcount->cDoSomething,
                        pupdcount->cConflict, pupdcount->cSubfolders);
            }

#endif
        }
    else
        {
        // No; single selection?

        // Take caution in the comparisons below.  The counts do not
        // have a 1-to-1 correspondence.  They are split into two 
        // groups:        cFiles <---> cOrphans  <---> cSubfolders
        //          cUnavailable <---> cDoSomething
        //
        // This means comparing cFiles with cDoSomething or cUnavailable
        // will produce bogus results in the case when folders are 
        // selected.
        //
        // As long as the comparisons below do not break these limits,
        // everything is okay.

        if (1 == pupdcount->cFiles)
            {
            // Yes
            ASSERT(2 > pupdcount->cOrphans);
            ASSERT(2 > pupdcount->cSubfolders);
            if (1 == pupdcount->cOrphans)
                {
                // Case S1
                dum = DUM_SELECTION | DUM_ORPHAN;
                }
            else if (0 == pupdcount->cUnavailable)
                {
                if (0 == cSomeAction)
                    {
                    if (0 == pupdcount->cSubfolders)
                        {
                        // Case S2
                        dum = DUM_SELECTION | DUM_UPTODATE;
                        }
                    else
                        {
                        dum = DUM_SELECTION | DUM_SUBFOLDER_TWIN;
                        }
                    }
                else
                    {
                    // Case S3
                    uRet = PSC_SHOWDIALOG;
                    }
                }
            else 
                {
                // Case S4
                dum = DUM_SELECTION | DUM_UNAVAILABLE;
                uRet = PSC_SHOWDIALOG;
                }
            }
        else
            {
            // No; this is a multi selection

            if (0 < pupdcount->cSubfolders)
                {
                DoUpdateMsg(hwndOwner, pszList, pupdcount->cSubfolders, DUM_SELECTION | DUM_SUBFOLDER_TWIN);
                goto Leave;  // HACK
                }

            if (pupdcount->cFiles == pupdcount->cOrphans)
                {
                // Case S5
                dum = DUM_SELECTION | DUM_ORPHAN;
                }
            else if (0 < pupdcount->cUnavailable)
                {
                if (0 < cSomeAction)
                    {
                    if (0 == pupdcount->cOrphans)
                        {
                        // Case S6
                        dum = DUM_SELECTION | DUM_UNAVAILABLE;
                        uRet = PSC_SHOWDIALOG;
                        }
                    else
                        {
                        // Case S7
                        dum = DUM_SELECTION | DUM_UNAVAILABLE;
                        uRet = PSC_SHOWDIALOG | PSC_POSTMSGBOX;
                        }
                    }
                else 
                    {
                    if (0 == pupdcount->cOrphans)
                        {
                        // Case S8
                        dum = DUM_SELECTION | DUM_UNAVAILABLE;
                        uRet = PSC_SHOWDIALOG;
                        }
                    else
                        {
                        // Case S9
                        dum = DUM_SELECTION | DUM_UNAVAILABLE;
                        uRet = PSC_SHOWDIALOG | PSC_POSTMSGBOX;
                        }
                    }
                }
            else
                {
                if (0 < cSomeAction)
                    {
                    if (0 == pupdcount->cOrphans)
                        {
                        // Case S10
                        uRet = PSC_SHOWDIALOG;
                        }
                    else
                        {
                        // Case S11
                        uRet = PSC_SHOWDIALOG | PSC_POSTMSGBOX;
                        }
                    }
                else 
                    {
                    if (0 == pupdcount->cOrphans)
                        {
                        // Case S12
                        dum = DUM_SELECTION | DUM_UPTODATE;
                        }
                    else
                        {
                        // Case S13
                        dum = DUM_SELECTION | DUM_UPTODATE | DUM_ORPHAN;
                        }
                    }
                }
            }

Leave:
        ;
#ifdef DEBUG

        if (IsFlagSet(g_uDumpFlags, DF_UPDATECOUNT))
            {
            TRACE_MSG(TF_ALWAYS, TEXT("Update selection counts: files = %u, orphans = %u, unavailable = %u, dosomething = %u, conflict = %u, subfolders = %u"),
                        pupdcount->cFiles, pupdcount->cOrphans, 
                        pupdcount->cUnavailable, pupdcount->cDoSomething,
                        pupdcount->cConflict, pupdcount->cSubfolders);
            }

#endif
        }

    if (0 != dum)
        {
        DoUpdateMsg(hwndOwner, pszList, pupdcount->cFiles, dum);
        }

    return uRet;
    }


/*----------------------------------------------------------
Purpose: Show the update dialog and perform the reconcilation
         if the user chooses OK

Returns: standard result
Cond:    --
*/
HRESULT PUBLIC Upd_DoModal(
    HWND hwndOwner,
    CBS * pcbs,
    LPCTSTR pszList,         // May be NULL if uFlags == UF_ALL
    UINT cFiles,
    UINT uFlags)
    {
    INT_PTR nRet;
    HRESULT hres;
    PRECLIST prl;
    UPDCOUNT updcount;
    HWND hwndProgress;

    hwndProgress = UpdBar_Show(hwndOwner, UB_CHECKING, DELAY_UPDBAR);

    // Get a reclist and other useful information
    updcount.cFiles = cFiles;

    if (IsFlagSet(uFlags, UF_ALL))
        {
        hres = PrepForUpdateAll(pcbs, &prl, &updcount, hwndProgress);
        }
    else
        {
        hres = PrepForUpdateSelection(pcbs, &prl, pszList, cFiles, &updcount, hwndProgress);
        }

    UpdBar_Kill(hwndProgress);

    if (S_OK == GetScode(hres))
        {
        XUPDSTRUCT xupd;
        xupd.lprl = prl;
        xupd.pcbs = pcbs;
        xupd.uFlags = uFlags;
        xupd.hdpa = ComposeUpdateList(pcbs, prl, &updcount, hwndOwner);
        xupd.cDoSomething = updcount.cDoSomething;

        if (NULL == xupd.hdpa)
            {
            hres = E_OUTOFMEMORY;
            MsgBox(hwndOwner, MAKEINTRESOURCE(IDS_OOM_UPDATEDIALOG), MAKEINTRESOURCE(IDS_CAP_UPDATE),
                   NULL, MB_ERROR);
            }
        else
            {
            // Check for some of those special cases listed at top of file
            UINT uVal = PassedSpecialCases(hwndOwner, pszList, &updcount, uFlags);

            // Show the update dialog?
            if (IsFlagSet(uVal, PSC_SHOWDIALOG))
                {
                // Yes
                nRet = DoModal(hwndOwner, Upd_WrapperProc, IDD_UPDATE, (LPARAM)&xupd);

                switch (nRet)
                    {
                case IDOK:
                    // Reconcile!

                    hwndProgress = UpdBar_Show(hwndOwner, UB_UPDATING, 0);

                    Sync_ReconcileRecList(prl, Atom_GetName(pcbs->atomBrf),
                        hwndProgress, RF_DEFAULT);

                    UpdBar_Kill(hwndProgress);

                    // Show a summary messagebox?
                    if (IsFlagSet(uVal, PSC_POSTMSGBOX))
                        {
                        // Yes
                        DoUpdateMsg(hwndOwner, pszList, updcount.cOrphans, DUM_SELECTION | DUM_ORPHAN);
                        }

                    // Fall thru
                    //  |    |
                    //  v    v

                case IDCANCEL:
                    hres = NOERROR;
                    break;

                case -1:
                    MsgBox(hwndOwner, MAKEINTRESOURCE(IDS_OOM_UPDATEDIALOG), MAKEINTRESOURCE(IDS_CAP_UPDATE),
                           NULL, MB_ERROR);
                    hres = E_OUTOFMEMORY;
                    break;

                default:
                    ASSERT(0);
                    break;
                    }
                }

            DPA_Destroy(xupd.hdpa);
            }

        Sync_DestroyRecList(prl);
        }
    return hres;
    }
