//
//  Folder.C
//
//  Copyright (C) Microsoft, 1994,1995 All Rights Reserved.
//
//  History:
//  ral 6/23/94 - First pass
//  3/20/95  [stevecat] - NT port & real clean up, unicode, etc.
//
//

#include "appwiz.h"
#include "..\..\..\inc\help.h"        // Help context IDs

typedef struct _FILEITEMDATA {
    DWORD   dwFlags;
    TCHAR   szPath[1];
} FILEITEMDATA, FAR * LPFILEITEMDATA;

#define FIDFLAG_CANADDNEW      0x00000001
#define FIDFLAG_CANDEL         0x00000002
#define FIDFLAG_ISFOLDER       0x00000004
#define FIDFLAG_ISPROGS        0x00000008


//
//
//

int CALLBACK CompareFolderCB(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    #define lpfid1  ((LPFILEITEMDATA)lParam1)
    #define lpfid2  ((LPFILEITEMDATA)lParam2)
    #define b1IsDir (lpfid1->dwFlags & FIDFLAG_ISFOLDER)
    #define b2IsDir (lpfid2->dwFlags & FIDFLAG_ISFOLDER)

    //
    // Programs folder always goes to top
    //

    if (lpfid1->dwFlags & FIDFLAG_ISPROGS)
    {
        return(-1);
    }

    if (lpfid2->dwFlags & FIDFLAG_ISPROGS)
    {
        return(1);
    }

    if (b1IsDir == b2IsDir)
    {
        return(lstrcmpi(lpfid1->szPath, lpfid2->szPath));
    }
    else
    {
        if (b1IsDir)
        {
            return(-1);
        }
        else
        {
            return(1);
        }
    }

    #undef  b1IsDir
    #undef  b2IsDir
    #undef  lpfid1
    #undef  lpfid2
}


//
//  Sorts the specified folder so that folders appear at the top and all
//  files appear in alphabetical order below.
//

void SortFolder(HWND hwndTree, HTREEITEM hParent)
{
    TV_SORTCB sSortCB;

    sSortCB.hParent = hParent;
    sSortCB.lpfnCompare = CompareFolderCB;
    sSortCB.lParam = 0;

    TreeView_SortChildrenCB(hwndTree, &sSortCB, FALSE);
}



//
//  Adds a new folder for the specifed path and returns its HTREEITEM.        If
//  it is unable to add the item then NULL is returned.
//  NOTE:  If dwFileAttributes == AI_NOATTRIB (-1) then no attributes specified.
//           If pidl is NULL then no pidl specified.
//

HTREEITEM AddItem(HWND hwndTree, LPCTSTR lpszPath,
                    HTREEITEM hParent, LPITEMIDLIST pidl,
                    DWORD dwFlags)
{
    LPFILEITEMDATA  lpfid = (LPFILEITEMDATA)LocalAlloc(LMEM_FIXED,
                             sizeof(FILEITEMDATA)+(lstrlen(lpszPath)+1)*sizeof(TCHAR));
    TV_ITEM         tvi;
    TV_INSERTSTRUCT tvis;
    SHFILEINFO      fi;
    HTREEITEM       newhti = NULL;

    if (!lpfid)
    {
        return(NULL);
    }

    lpfid->dwFlags = dwFlags;
    lstrcpy(lpfid->szPath, lpszPath);

    //
    // There are 3 cases handled here:
    //     If a PIDL is specified then we'll get the description AND the icon
    //     If a path is specified and it's a folder the use index 0 and the
    //         description is the folder name.
    //     If a path is specified and it's not a folder then we'll use
    //         call-backs to fill in the data
    //

    if (pidl)
    {
        if (!SHGetFileInfo((LPTSTR)pidl, 0, &fi, sizeof(fi),
                      SHGFI_ICON | SHGFI_DISPLAYNAME | SHGFI_SMALLICON | SHGFI_PIDL))
        {
            goto CleanUp;
        }

        tvi.iImage = tvi.iSelectedImage =
                  ImageList_AddIcon(TreeView_GetImageList(hwndTree, TVSIL_NORMAL),
                                    fi.hIcon);

        tvi.pszText = fi.szDisplayName;

        DestroyIcon(fi.hIcon);

    }
    else
    {
        if (dwFlags & FIDFLAG_ISFOLDER)
        {
            tvi.iImage = tvi.iSelectedImage = 0;
            tvi.pszText = PathFindFileName(lpszPath);
        }
        else
        {
            tvi.iImage = tvi.iSelectedImage = I_IMAGECALLBACK;
            tvi.pszText = LPSTR_TEXTCALLBACK;
        }
    }

    tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.lParam = (LPARAM)lpfid;

    tvis.item = tvi;
    tvis.hParent = hParent;
    tvis.hInsertAfter = TVI_LAST;

    newhti = TreeView_InsertItem(hwndTree, &tvis);

CleanUp:
    if (!newhti)
    {
        LocalFree((LPVOID)lpfid);
    }
    return(newhti);
}

//
//  Flags for FillFolder
//

#define FFF_AddFiles                1
#define FFF_AddDirs                2

//
//  Recursively add all folders below CurDir to the tree blow hParent
//

void FillFolder(HWND hwndTree, LPTSTR lpszCurDir, LPTSTR lpszExclude,
                    HTREEITEM hParent, DWORD dwFlags)
{
    int     iStrTerm = lstrlen(lpszCurDir);
    WIN32_FIND_DATA fd;
    HANDLE  hfind;
    HTREEITEM hNewItem = NULL;
    #define bAddFiles (dwFlags & FFF_AddFiles)
    #define bAddDirs  (dwFlags & FFF_AddDirs)

    lstrcat(lpszCurDir, TEXT("\\*.*"));

    hfind = FindFirstFile(lpszCurDir, &fd);

    if (hfind != INVALID_HANDLE_VALUE)
    {
        do
        {
            BOOL bIsDir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

            if (((bAddFiles && !bIsDir) ||
                // skip "." and ".." and hidden files
                (bAddDirs && bIsDir && (fd.cFileName[0] != TEXT('.')))) &&
                !(fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
            {
                lpszCurDir[iStrTerm] = TEXT('\\');
                lstrcpy(lpszCurDir + iStrTerm + 1, fd.cFileName);

                //
                // Don't add this if it's supposed to be excluded
                //

                if (!lpszExclude || !bIsDir ||
                    lstrcmpi(lpszExclude, lpszCurDir) != 0)
                {
                    hNewItem = AddItem(hwndTree, lpszCurDir, hParent, NULL,
                                        FIDFLAG_CANADDNEW | FIDFLAG_CANDEL |
                                        (bIsDir ? FIDFLAG_ISFOLDER : 0));
                    if (bIsDir)
                    {
                        FillFolder(hwndTree, lpszCurDir, NULL,
                                   hNewItem, dwFlags);
                    }
                }
            }
        } while (FindNextFile(hfind, &fd));

        FindClose(hfind);
    }

    lpszCurDir[iStrTerm] = 0;

    //
    //  Non-null if any items added to folder.
    //

    if (hNewItem)
    {
        SortFolder(hwndTree, hParent);
        if (!bAddFiles)
        {
            TreeView_Expand(hwndTree, hParent, TVE_EXPAND);
        }
    }
    #undef  bAddFiles
    #undef  bRecurse
}


//
//  Returns a pointer to the directory string for the currently selected
//  item.
//

LPFILEITEMDATA GetCurSel(HWND hwndTree, HTREEITEM FAR * lphtiSel)
{
    TV_ITEM  tvi;

    tvi.hItem = TreeView_GetSelection(hwndTree);

    if (lphtiSel)
    {
        *lphtiSel = tvi.hItem;
    }

    if (tvi.hItem == NULL)
    {
        return(NULL);
    }

    tvi.mask = TVIF_PARAM;
    TreeView_GetItem(hwndTree, &tvi);

    return((LPFILEITEMDATA)tvi.lParam);
}


//
//  Add the specified special folder..
//

HTREEITEM AddSpecialFolder(HWND hwndTree, HTREEITEM htiParent, int nFolder,
                           LPTSTR lpszPath, DWORD dwFlags)
{
    LPITEMIDLIST pidl;
    HTREEITEM         hti = NULL;

    SHGetSpecialFolderLocation(hwndTree, nFolder, &pidl);

    if (pidl)
    {
        SHGetPathFromIDList(pidl, lpszPath);

        //
        //  For the desktop, we want the desktop directory, but the icon
        //  for the magic desktop PIDL.
        //

        if (nFolder == CSIDL_DESKTOPDIRECTORY)
        {
            SHFree(pidl);
            SHGetSpecialFolderLocation(hwndTree, CSIDL_DESKTOP, &pidl);
        }

        hti = AddItem(hwndTree, lpszPath, htiParent, pidl,
                      FIDFLAG_ISFOLDER | dwFlags);
        SHFree(pidl);
    }
    return(hti);
}


BOOL _inline MakePrgIcon0Index(HWND hwndTree, HIMAGELIST himl)
{
    LPITEMIDLIST pidl;
    SHFILEINFO   fi;
    BOOL         fOk;

    SHGetSpecialFolderLocation(hwndTree, CSIDL_PROGRAMS, &pidl);

    if (pidl)
    {
        fOk = SHGetFileInfo( (LPTSTR) pidl, 0, &fi, sizeof( fi ),
                              SHGFI_ICON | SHGFI_SMALLICON | SHGFI_PIDL );

        SHFree( pidl );
        
        if (fOk)
        {
            ImageList_AddIcon(himl, fi.hIcon);
            DestroyIcon(fi.hIcon);
            return(TRUE);
        }
    }
    return FALSE;
}


//
//  Initialize the tree
//

void InitFolderTree( HWND hwndTree, BOOL bAddFiles, HIMAGELIST *phiml )
{
    HCURSOR    hcurOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
    HTREEITEM  htiStart = NULL;
    HTREEITEM  htiPrgs = NULL;
    TCHAR      szPathStart[MAX_PATH];
    TCHAR      szPathPrgs[MAX_PATH];

    HIMAGELIST himl = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                                       GetSystemMetrics(SM_CYSMICON),
                                       TRUE, 10, 1);

    if (phiml)
        *phiml = himl;

    if (!himl)
    {
        return;
    }

    TreeView_SetImageList(hwndTree, himl, TVSIL_NORMAL);

    //
    // Add the programs folder as index 0.  All sub-folders of programs
    // will also have the same icon.  This saves both memory and time.
    //

    if (!MakePrgIcon0Index(hwndTree, himl))
    {
        return;
    }

    if (!bAddFiles)
    {
        AddSpecialFolder(hwndTree, TVI_ROOT, CSIDL_DESKTOPDIRECTORY, szPathStart, 0);
    }

    htiStart = AddSpecialFolder(hwndTree, TVI_ROOT, CSIDL_STARTMENU, szPathStart, FIDFLAG_CANADDNEW);

    if (htiStart)
    {
        htiPrgs = AddSpecialFolder(hwndTree, htiStart, CSIDL_PROGRAMS, szPathPrgs, FIDFLAG_CANADDNEW | FIDFLAG_ISPROGS);
        if (htiPrgs)
        {
            FillFolder(hwndTree, szPathPrgs, NULL, htiPrgs,
                       FFF_AddDirs | (bAddFiles ? FFF_AddFiles : 0));
            //
            // Now fill in the rest of the start menu, excluding programs
            //

            FillFolder(hwndTree, szPathStart, szPathPrgs, htiStart,
                       FFF_AddDirs | (bAddFiles ? FFF_AddFiles : 0));
        }
    }

    //
    // Now select and expand the programs folder.
    //

    if (htiPrgs)
    {
        TreeView_SelectItem(hwndTree, htiPrgs);
        if (bAddFiles)
        {
            TreeView_Expand(hwndTree, htiPrgs, TVE_EXPAND);
        }
    }
    SetCursor(hcurOld);
}


//
//  Delete Selected Item
//

VOID RemoveSelItem(HWND hDlg, HWND hwndTree)
{
    HTREEITEM hCur;
    LPFILEITEMDATA lpfid = GetCurSel(hwndTree, &hCur);

    if (!lpfid)
    {
        ShellMessageBox(hInstance, hDlg, MAKEINTRESOURCE(IDS_NONESEL),
                        0, MB_OK | MB_ICONEXCLAMATION);
    }
    else
    {
        if (lpfid->dwFlags & FIDFLAG_CANDEL)
        {
            TCHAR szFileDblNull[MAX_PATH+1];

            SHFILEOPSTRUCT sFileOp =
            {
                hDlg,
                FO_DELETE,
                szFileDblNull,
                NULL,
                (lpfid->dwFlags & FIDFLAG_ISFOLDER) ?
                FOF_ALLOWUNDO :
                FOF_SILENT | FOF_ALLOWUNDO,
            };

            lstrcpy(szFileDblNull, lpfid->szPath);

            szFileDblNull[lstrlen(szFileDblNull)+1] = 0;

            if (!SHFileOperation(&sFileOp))
            {
                if (!(sFileOp.fAnyOperationsAborted))
                {
                    TreeView_DeleteItem(hwndTree, hCur);
                }
            }

        }
        else
        {
            ShellMessageBox(hInstance, hDlg, MAKEINTRESOURCE(IDS_CANTDELETE),
                            0, MB_OK | MB_ICONEXCLAMATION, PathFindFileName(lpfid->szPath));
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//  END SHARED CODE.  BEGIN WIZARD SPECIFIC CODE.
/////////////////////////////////////////////////////////////////////////////


//
//  Returns -1 if no item is selected, otherwise, sets lpwd->lpszFolder
//  to point to the appropriate string, and returns 0.
//

LPARAM PickFolderNextHit(LPWIZDATA lpwd)
{
    LPFILEITEMDATA lpfid = GetCurSel(GetDlgItem(lpwd->hwnd, IDC_FOLDERTREE), NULL);

    if (lpfid)
    {
        lpwd->lpszFolder = (LPTSTR)&(lpfid->szPath);
        lpwd->szProgDesc[0] = 0;
        return(0);
    }
    else
    {
        return(-1);
    }
}


//
//  Creates a new, empty folder.
//

VOID CreateNewFolder(LPWIZDATA lpwd)
{
    TCHAR          szNewName[MAX_PATH];
    HTREEITEM      hParent;
    LPFILEITEMDATA lpfidParent = GetCurSel(GetDlgItem(lpwd->hwnd, IDC_FOLDERTREE), &hParent);

    if (lpfidParent && (lpfidParent->dwFlags & FIDFLAG_CANADDNEW))
    {
        int   iDirLen = lstrlen(lpfidParent->szPath);
        TCHAR szNewShort[10];
        TCHAR szNewLong[80];

        LoadString(hInstance, IDS_NEWFOLDERSHORT, szNewShort, ARRAYSIZE(szNewShort));
        LoadString(hInstance, IDS_NEWFOLDERLONG, szNewLong, ARRAYSIZE(szNewLong));

        lstrcpyn(szNewName, lpfidParent->szPath, ARRAYSIZE(szNewName));

        szNewName[iDirLen] = TEXT('\\');

        PathMakeUniqueName(&(szNewName[iDirLen+1]), ARRAYSIZE(szNewName)-iDirLen-1,
                           szNewShort, szNewLong, lpfidParent->szPath);

        if (CreateDirectory(szNewName, NULL))
        {
            HWND    hwndTree = GetDlgItem(lpwd->hwnd, IDC_FOLDERTREE);
            HTREEITEM hNewDude = AddItem(hwndTree, szNewName, hParent, NULL,
                              FIDFLAG_ISFOLDER | FIDFLAG_CANADDNEW | FIDFLAG_CANDEL);

            WIZERRORIF((hNewDude == NULL), TEXT("Unable to add new folder to tree."));

            if (hNewDude)
            {
                SortFolder(hwndTree, hParent);
                TreeView_SelectItem(hwndTree, hNewDude);
                TreeView_EditLabel(hwndTree, hNewDude);
            }
        }
        else
        {
            WIZERROR(TEXT("Unable to create new directory"));
        }
    }
    else
    {
        WIZERROR(TEXT("No group selected.  Can't create directory."));
    }
}


//
//  Begin editing a tree label.  This function returns FALSE for success, and
//  TRUE for failure.
//

BOOL BeginEdit(LPWIZDATA lpwd, TV_DISPINFO FAR * lptvdi)
{
    if (TreeView_GetParent(lptvdi->hdr.hwndFrom, lptvdi->item.hItem))
    {
        lpwd->dwFlags |= WDFLAG_INEDITMODE;
        return(FALSE);
    }
    else
    {
        return(TRUE);
    }
}


//
//  Return FALSE if rename can't happen.  True if it worked.
//

BOOL EndEdit(LPWIZDATA lpwd, TV_DISPINFO FAR * lptvdi)
{
    BOOL bWorked = FALSE;
    #define lpszNewName (LPTSTR)lptvdi->item.pszText
    #define lpfidOld ((LPFILEITEMDATA)(lptvdi->item.lParam))
    #define hCurItem lptvdi->item.hItem;

    lpwd->dwFlags &= ~WDFLAG_INEDITMODE;

    if (lpszNewName)
    {
        LPFILEITEMDATA lpfidNew = (LPFILEITEMDATA)LocalAlloc(LMEM_FIXED,
                                   sizeof(LPFILEITEMDATA)+MAX_PATH*sizeof(TCHAR));

        if (lpfidNew)
        {
            lpfidNew->dwFlags = lpfidOld->dwFlags;

            lstrcpy(lpfidNew->szPath, lpfidOld->szPath);

            PathRemoveFileSpec(lpfidNew->szPath);

            PathCleanupSpec(lpfidNew->szPath, lpszNewName);

            PathCombine(lpfidNew->szPath, lpfidNew->szPath, lpszNewName);

            if (MoveFile(lpfidOld->szPath, lpfidNew->szPath))
            {
                TV_ITEM tvi;
                tvi.hItem = hCurItem;
                tvi.mask = TVIF_PARAM;
                tvi.lParam = (LPARAM)lpfidNew;
                TreeView_SetItem(lptvdi->hdr.hwndFrom, &tvi);
                bWorked = TRUE;
            }
            else
            {
                WIZERROR(TEXT("Unable to rename directory"));
            }
            LocalFree(bWorked ? lpfidOld : lpfidNew);
        }
    }

    return(bWorked);

    #undef lpszNewName
    #undef lpfidOld
    #undef hCurItem
}


//
//  Called when Next or Back is hit to force the end of label editing.
//

void ForceEndEdit(LPWIZDATA lpwd)

{
    if (lpwd->dwFlags & WDFLAG_INEDITMODE)
    {
        TreeView_EndEditLabelNow(GetDlgItem(lpwd->hwnd, IDC_FOLDERTREE), FALSE);
    }
}


//
//  Main dialog procedure for tree of folders
//

BOOL CALLBACK PickFolderDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR *lpnm;
    LPPROPSHEETPAGE lpp = (LPPROPSHEETPAGE)(GetWindowLong(hDlg, DWL_USER));
    LPWIZDATA lpwd = lpp ? (LPWIZDATA)lpp->lParam : NULL;

    switch(message)
    {
        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;
            switch(lpnm->code)
            {
                case PSN_SETACTIVE:
                    if (lpwd->dwFlags & WDFLAG_LINKHEREWIZ)
                    {
                        SetDlgMsgResult(hDlg, WM_NOTIFY, -1);
                    }
                    else
                    {
                        lpwd->hwnd = hDlg;
                        PropSheet_SetWizButtons(GetParent(hDlg),
                                                (lpwd->dwFlags & WDFLAG_NOBROWSEPAGE) ?
                                                  PSWIZB_NEXT : PSWIZB_BACK | PSWIZB_NEXT);

                        PostMessage(hDlg, WMPRIV_POKEFOCUS, 0, 0);
                    }
                    break;

                case PSN_WIZBACK:
                    ForceEndEdit(lpwd);
                    SetDlgMsgResult(hDlg, WM_NOTIFY, 0);
                    break;

                case PSN_WIZNEXT:
                    ForceEndEdit(lpwd);
                    SetDlgMsgResult(hDlg, WM_NOTIFY, PickFolderNextHit(lpwd));
                    break;

                case PSN_RESET:
                    CleanUpWizData(lpwd);
                    break;

                case NM_DBLCLK:
                    PropSheet_PressButton(GetParent(hDlg), PSBTN_NEXT);
                    break;

                #define lpfidNew ((LPFILEITEMDATA)(((LPNM_TREEVIEW)lParam)->itemNew.lParam))

                case TVN_SELCHANGED:
                    Button_Enable(GetDlgItem(hDlg, IDC_NEWFOLDER),
                                  (lpfidNew->dwFlags & FIDFLAG_CANADDNEW));
                    break;
                #undef lpfidNew

                #define lptvdi ((TV_DISPINFO FAR *)lParam)

                case TVN_BEGINLABELEDIT:
                    SetDlgMsgResult(hDlg, WM_NOTIFY, BeginEdit(lpwd, lptvdi));
                    break;

                case TVN_ENDLABELEDIT:
                    SetDlgMsgResult(hDlg, WM_NOTIFY, EndEdit(lpwd, lptvdi));
                    break;
                #undef lptvdi

                #define lptvn ((LPNM_TREEVIEW)lParam)

                case TVN_ITEMEXPANDING:
                    if (lptvn->action != TVE_EXPAND)
                    {
                        SetDlgMsgResult(hDlg, WM_NOTIFY, -1);
                    }
                    break;

                case TVN_DELETEITEM:
                    if (lptvn->itemOld.lParam)
                    {
                        LocalFree((LPVOID)lptvn->itemOld.lParam);
                    }
                    break;
                #undef lptvn

                default:
                    return FALSE;
            }
            break;

        case WM_INITDIALOG:
            lpwd = InitWizSheet(hDlg, lParam, 0);
            lpwd->himl = NULL;

            if( !( lpwd->dwFlags & WDFLAG_LINKHEREWIZ ) )
            {
                InitFolderTree( GetDlgItem( hDlg, IDC_FOLDERTREE ),
                                FALSE, &lpwd->himl );
            }
            break;


        case WM_NCDESTROY:
            //
            //  See if we should destroy the himl...
            //

            if (lpwd->himl)
            {
                ImageList_Destroy(lpwd->himl);
                lpwd->himl = NULL;  // make sure not twice
            }
            return FALSE;


        case WMPRIV_POKEFOCUS:
            SetFocus(GetDlgItem(hDlg, IDC_FOLDERTREE));
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDC_NEWFOLDER:
                    CreateNewFolder(lpwd);
                    break;

             ///   case IDC_DELFOLDER:
             ///   {
             ///       HWND    hTree = GetDlgItem(hDlg, IDC_FOLDERTREE);
             ///       RemoveSelItem(hDlg, hTree);
             ///       SetFocus(hTree);
             ///       break;
             ///   }
            }

        default:
            return FALSE;

    }
    return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
//  END WIZARD SPECIFIC CODE.  BEGIN DELETE ITEM DIALOG CODE.
/////////////////////////////////////////////////////////////////////////////

typedef struct _FOLDERTHREADINFO {
    HANDLE     hThread;
    HWND       hwndTree;
    HIMAGELIST himl;
} FOLDERTHREADINFO, FAR * PFOLDERTHREADINFO;


void CALLBACK FolderEnumItems(PFOLDERTHREADINFO pfti, HTREEITEM hParent)
{
    HTREEITEM hitem;

    hitem = hParent;
    while (hitem && pfti->hThread)
    {
        TV_ITEM tvi;
        tvi.mask  = TVIF_IMAGE;
        tvi.hItem = hitem;
        TreeView_GetItem(pfti->hwndTree, &tvi);
        hitem = TreeView_GetNextSibling(pfti->hwndTree, hitem);
    }

    hitem = TreeView_GetChild(pfti->hwndTree, hParent);

    while (hitem && pfti->hThread)
    {
        FolderEnumItems(pfti, hitem);
        hitem = TreeView_GetNextSibling(pfti->hwndTree, hitem);
    }
}


DWORD CALLBACK FolderThread(PFOLDERTHREADINFO pfti)
{
    HANDLE hThread = pfti->hThread;

    FolderEnumItems(pfti, TreeView_GetRoot(pfti->hwndTree));

    CloseHandle(hThread);

    pfti->hThread = 0;

    return 0;
}


VOID CreateFolderThread(PFOLDERTHREADINFO pfti)
{
    //
    // Create background thread to force list view to draw items
    //

    DWORD idThread;

    if (pfti->hThread)
            return;

    pfti->hThread = CreateThread(NULL, 0, FolderThread, pfti, 0, &idThread);

    if(pfti->hThread)
            SetThreadPriority(pfti->hThread, THREAD_PRIORITY_BELOW_NORMAL);
}


void FillInItem(TV_DISPINFO FAR * lptvdi)
{
    SHFILEINFO        fi;

    #define lpfid ((LPFILEITEMDATA)(lptvdi->item.lParam))

    if (SHGetFileInfo(lpfid->szPath, 0, &fi, sizeof(fi),
                      SHGFI_ICON | SHGFI_DISPLAYNAME | SHGFI_SMALLICON))
    {
        lptvdi->item.iImage = lptvdi->item.iSelectedImage =
                  ImageList_AddIcon(TreeView_GetImageList(lptvdi->hdr.hwndFrom, TVSIL_NORMAL),
                                    fi.hIcon);

        lptvdi->item.pszText = fi.szDisplayName;

        DestroyIcon(fi.hIcon);

        lptvdi->item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;

        TreeView_SetItem(lptvdi->hdr.hwndFrom, &(lptvdi->item));
    }
}


void _inline WaitForThreadDeath(HWND hDlg, HANDLE FAR * phThread)
{
    HANDLE hThread = *phThread;
    MSG msg;

    if (hThread)
    {
        *phThread = 0;

        while(TRUE)
        {
            DWORD result = MsgWaitForMultipleObjects(1, &hThread, FALSE, 5000, QS_SENDMESSAGE);

            switch (result)
            {
            case WAIT_OBJECT_0:
            case WAIT_FAILED:
                return;

            case WAIT_TIMEOUT:
                #ifdef DEBUG
                ShellMessageBox(hInstance, hDlg, TEXT("Forced to terminate background thread"), 0, MB_OK | MB_ICONEXCLAMATION);
                #endif

                TerminateThread(hThread, 0);

                return;

            case WAIT_OBJECT_0 + 1:
                while (PeekMessage(&msg, hDlg, TVN_FIRST, TVN_LAST, PM_REMOVE))
                {
                    DispatchMessage(&msg);
                }
                break;
            }
        }
    }
}


//
//  Main dialog procedure for delete items dialog.
//

const static DWORD aDelItemHelpIDs[] = {  // Context Help IDs
    IDC_TEXT,         NO_HELP,
    IDC_FOLDERTREE,   IDH_TRAY_REMOVEDLG_LIST,
    IDC_DELETEITEM,   IDH_TRAY_REMOVEDLG_DEL,

    0, 0
};

BOOL CALLBACK DelItemDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
    PFOLDERTHREADINFO pfti = (PFOLDERTHREADINFO)GetWindowLong(hDlg, DWL_USER);

    switch(message)
    {
        case WM_NOTIFY:
            #define lpnm ((NMHDR FAR *)lParam)

            switch(lpnm->code)
            {
                #define lpfidNew ((LPFILEITEMDATA)(((LPNM_TREEVIEW)lParam)->itemNew.lParam))

                case TVN_SELCHANGED:
                {
                    BOOL fCanDel = (lpfidNew->dwFlags & FIDFLAG_CANDEL);
                    HWND hwndDelItem = GetDlgItem(hDlg, IDC_DELETEITEM);

                    if ((!fCanDel) && (GetFocus() == hwndDelItem))
                    {
                        SetFocus(GetDlgItem(hDlg, IDOK));
                        SendMessage(hDlg, DM_SETDEFID, IDOK, 0);
                    }
                    Button_Enable(hwndDelItem, fCanDel);
                    break;
                }

                #undef lpfidNew

                #define lptvn ((LPNM_TREEVIEW)lParam)

                case TVN_DELETEITEM:
                    if (lptvn->itemOld.lParam)
                    {
                        LocalFree((LPVOID)lptvn->itemOld.lParam);
                    }
                    break;

                #undef lptvn

                #define lptkd ((TV_KEYDOWN FAR *)lParam)

                case TVN_KEYDOWN:
                    if (lptkd->wVKey == VK_DELETE)
                    {
		                WaitForThreadDeath(hDlg, &(pfti->hThread));
                        RemoveSelItem(hDlg, GetDlgItem(hDlg, IDC_FOLDERTREE));
                        CreateFolderThread(pfti);
                        return TRUE;
                    }
                    break;

                #undef lptkd

                case TVN_GETDISPINFO:
                    FillInItem(((TV_DISPINFO FAR *)lParam));
                    break;

                default:
                    return FALSE;

            #undef lpnm
            }
            break;

        case WM_INITDIALOG:

            SetWindowLong(hDlg, DWL_USER, lParam);

            pfti = (PFOLDERTHREADINFO)lParam;

            InitFolderTree(GetDlgItem(hDlg, IDC_FOLDERTREE), TRUE, &pfti->himl);

            pfti->hwndTree = GetDlgItem(hDlg, IDC_FOLDERTREE);
            pfti->hThread = 0;

            CreateFolderThread(pfti);
            break;

        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
                HELP_WM_HELP, (DWORD)(LPTSTR) aDelItemHelpIDs);
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
                (DWORD)(LPVOID) aDelItemHelpIDs);
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDOK:
                case IDCANCEL:
                    WaitForThreadDeath(hDlg, &(pfti->hThread));
                    EndDialog(hDlg, GET_WM_COMMAND_ID(wParam, lParam));
                    break;

                case IDC_DELETEITEM:
                    WaitForThreadDeath(hDlg, &(pfti->hThread));
                    RemoveSelItem(hDlg, GetDlgItem(hDlg, IDC_FOLDERTREE));
                    CreateFolderThread(pfti);
                    break;
            }

        default:
            return FALSE;

    }
    return TRUE;
}


BOOL RemoveItemsDialog( HWND hParent )
{
    BOOL fReturn;

    FOLDERTHREADINFO fti;

    fti.himl = NULL;    // incase we can not create the window

    fReturn = DialogBoxParam( hInstance, MAKEINTRESOURCE( DLG_DELITEM ),
			                  hParent, DelItemDlgProc, (LPARAM) &fti );

    if( fti.himl )
        ImageList_Destroy( fti.himl );

    return fReturn;
}
