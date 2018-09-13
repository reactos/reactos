//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
#include "smtidy.h"
#include "finish.h"
#include "util.h"
#include "resource.h"
#include "lnktrack.h"
#include "filter.h"
#include "smwiz.h"

//----------------------------------------------------------------------------
void FixSingleItemFolders(PSMTIDYINFO psmti)
{
    int cItems = DSA_GetItemCount(psmti->hdsaSMI);
    int i;

    for (i=0; i<cItems; i++)
    {
        PSMITEM psmi = DSA_GetItemPtr(psmti->hdsaSMI, i);
        Assert(psmi);
        if (psmi->dwFlags & SMIF_SINGLE_ITEM)
            psmi->dwFlags |= SMIF_MOVE_UP;
    }   
}

//----------------------------------------------------------------------------
void FixEmptyFolders(PSMTIDYINFO psmti)
{
    int cItems = DSA_GetItemCount(psmti->hdsaSMI);
    int i;

    for (i=0; i<cItems; i++)
    {
        PSMITEM psmi = DSA_GetItemPtr(psmti->hdsaSMI, i);
        Assert(psmi);
        if (psmi->dwFlags & SMIF_EMPTY_FOLDER)
            psmi->dwFlags |= SMIF_DELETE;
    }   
}

//----------------------------------------------------------------------------
void GroupReadMes(PSMTIDYINFO psmti)
{
    int cItems = DSA_GetItemCount(psmti->hdsaSMI);
    int i;

    for (i=0; i<cItems; i++)
    {
        PSMITEM psmi = DSA_GetItemPtr(psmti->hdsaSMI, i);
        Assert(psmi);
        if (psmi->dwFlags & SMIF_README)
            psmi->dwFlags |= SMIF_GROUP;
    }   
}

//----------------------------------------------------------------------------
void GroupUnusedShortcuts(PSMTIDYINFO psmti)
{
    int cItems = DSA_GetItemCount(psmti->hdsaSMI);
    int i;

    for (i=0; i<cItems; i++)
    {
        PSMITEM psmi = DSA_GetItemPtr(psmti->hdsaSMI, i);
        Assert(psmi);
        if (psmi->dwFlags & SMIF_UNUSED_SHORTCUT)
            psmi->dwFlags |= SMIF_GROUP;
    }   
}

//----------------------------------------------------------------------------
void DeleteUnusedShortcuts(PSMTIDYINFO psmti)
{
    int cItems = DSA_GetItemCount(psmti->hdsaSMI);
    int i;

    for (i=0; i<cItems; i++)
    {
        PSMITEM psmi = DSA_GetItemPtr(psmti->hdsaSMI, i);
        Assert(psmi);
        if (psmi->dwFlags & SMIF_UNUSED_SHORTCUT)
            psmi->dwFlags |= SMIF_DELETE;
    }   
}

//----------------------------------------------------------------------------
void DeleteReadMes(PSMTIDYINFO psmti)
{
    int cItems = DSA_GetItemCount(psmti->hdsaSMI);
    int i;

    for (i=0; i<cItems; i++)
    {
        PSMITEM psmi = DSA_GetItemPtr(psmti->hdsaSMI, i);
        Assert(psmi);
        if (psmi->dwFlags & SMIF_README)
            psmi->dwFlags |= SMIF_DELETE;
    }   
}

//----------------------------------------------------------------------------
void GroupBrokenShortcuts(PSMTIDYINFO psmti)
{
    int cItems = DSA_GetItemCount(psmti->hdsaSMI);
    int i;

    for (i=0; i<cItems; i++)
    {
        PSMITEM psmi = DSA_GetItemPtr(psmti->hdsaSMI, i);
        Assert(psmi);
        if (psmi->dwFlags & SMIF_BROKEN_SHORTCUT)
            psmi->dwFlags |= SMIF_GROUP;
    }   
}

//----------------------------------------------------------------------------
void DeleteBrokenShortcuts(PSMTIDYINFO psmti)
{
    int cItems = DSA_GetItemCount(psmti->hdsaSMI);
    int i;

    for (i=0; i<cItems; i++)
    {
        PSMITEM psmi = DSA_GetItemPtr(psmti->hdsaSMI, i);
        Assert(psmi);
        if (psmi->dwFlags & SMIF_BROKEN_SHORTCUT)
            psmi->dwFlags |= SMIF_DELETE;
    }   
}


//----------------------------------------------------------------------------
BOOL IsGenericName(LPCTSTR pszName)
{
    TCHAR sz[MAX_PATH];
    BOOL fRet = FALSE;
    int i;
        
    lstrcpy(sz, pszName);
    PathRemoveExtension(sz);

    for (i=IDS_FIRST_GENERIC_NAME; i <= IDS_LAST_GENERIC_NAME; i++)
    {    
        TCHAR szGen[MAX_PATH];
        
        if (LoadString(g_hinstApp, i, szGen, ARRAYSIZE(szGen)))
        {
            Assert(*szGen);
            // Dbg(TEXT("ign: Checking %s and %s"), sz, szGen);
            if (lstrcmpi(szGen, sz) == 0)
            {
                fRet = TRUE;
                break;
            }
        }
        else
        {
            // Hit the end of the list.
            break;
        }                
    }
    
    return fRet;
}

//----------------------------------------------------------------------------
// Special case things like ReadMe and Release Notes and prepend/append the 
// original folder name to make them unique.
// eg From = "c:\windows\Start Menu\Programs\Test\Readme.txt
//    To   = "c:\windows\Start Menu\Programs\Info"
// becomes
//    To   = "c:\windows\Start Menu\Programs\Info\Test Readme.txt"
BOOL MakeGenericNamesUnique(LPCTSTR pszFrom, PTSTR pszTo)
{
    TCHAR sz[MAX_PATH];
    LPTSTR pszName;
    BOOL fRes = FALSE;
    
    lstrcpy(sz, pszFrom);
    pszName = PathFindFileName(sz);
    Assert(pszName);
    if (IsGenericName(pszName))
    {
        TCHAR szMap[MAX_PATH];
        LPTSTR pszFolder;
        LPTSTR args[2];
        LPTSTR pszRes;
        
        PathRemoveFileSpec(sz);
        pszFolder = PathFindFileName(sz);
        Assert(pszFolder);
        LoadString(g_hinstApp, IDS_MAPPING, szMap, ARRAYSIZE(szMap));
        args[0] = pszFolder;
        args[1] = pszName;
        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY, 
            szMap, 0, 0, (LPTSTR)&pszRes, 0, (va_list*)args))
        {
            Assert(pszRes);
            // Is the combined string going to be too long?
            if (lstrlen(pszRes)+lstrlen(pszTo)+4 < MAX_PATH)
            {
                PathAppend(pszTo, pszRes);
                // Dbg(TEXT("mgnu: Dest is now %s"), pszTo);
                fRes = TRUE;
            }
            LFree(pszRes);
        }
    }
    else
    {
        PathAppend(pszTo, pszName);
    }
    
    return fRes;
}

//----------------------------------------------------------------------------
BOOL _MoveFile(LPTSTR pszSrc, LPTSTR pszDst)
{
    BOOL fRet = FALSE;
    SHFILEOPSTRUCT sFileOp =
    {
        NULL,
        FO_MOVE,
        pszSrc,
        pszDst,
        FOF_RENAMEONCOLLISION | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_SILENT | FOF_ALLOWUNDO,
    } ;

    if (lstrcmpi(pszSrc, pszDst) == 0)
    {
        // Do nothing if the src and dest are the same.
        fRet = TRUE;
    }
    else
    {
        pszSrc[lstrlen(pszSrc) + 1] = TEXT('\0');     // double NULL terminate
        pszDst[lstrlen(pszDst) + 1] = TEXT('\0');     // double NULL terminate
        Dbg(TEXT("mf: Moving %s to %s"), pszSrc, pszDst);
        fRet = !SHFileOperation(&sFileOp);
    }
    return fRet;
}

//----------------------------------------------------------------------------
BOOL _DeleteFile(LPTSTR psz)
{
    BOOL fRet = FALSE;
    
    // We won't delete non-empty folders.
    if (PathIsDirectory(psz))
    {
        if (RemoveDirectory(psz))
        {
            SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, psz, NULL);
            fRet = TRUE;
        }
    }
    else
    {
        SHFILEOPSTRUCT sFileOp =
        {
            NULL,
            FO_DELETE,
            psz,
            NULL,
            FOF_RENAMEONCOLLISION | FOF_NOCONFIRMATION | FOF_SILENT | FOF_ALLOWUNDO,
        } ;
        psz[lstrlen(psz) + 1] = TEXT('\0');     // double NULL terminate
        fRet = !SHFileOperation(&sFileOp);
    }
    return fRet;
}

//----------------------------------------------------------------------------
void MoveUp(PSMTIDYINFO psmti, PSMITEM psmi)
{
    PIDL pidlTo;
    USE(psmti);
    
    pidlTo = ILClone(psmi->pidlItem);
    if (pidlTo)
    {
        TCHAR szFrom[MAX_PATH];
        TCHAR szTo[MAX_PATH];
        
        ILRemoveLastID(pidlTo);
        ILRemoveLastID(pidlTo);
        SHGetPathFromIDList(psmi->pidlItem, szFrom);
        SHGetPathFromIDList(pidlTo, szTo);
        MakeGenericNamesUnique(szFrom, szTo);
        _MoveFile(szFrom, szTo);
        // Dbg(TEXT("mu: Moving %s up to %s"), szFrom, szTo);
        ILFree(pidlTo);
    }
}

//----------------------------------------------------------------------------
void MoveItemToCustomGroup(PSMTIDYINFO psmti, PSMITEM psmi)
{
    TCHAR szFrom[MAX_PATH];
    TCHAR szTo[MAX_PATH];
    PTSTR *ppszGroup = NULL;
    UINT id = 0;
        
    if (psmi->dwFlags & SMIF_UNUSED_SHORTCUT)
    {
        ppszGroup = &psmti->pszUnusedShortcutGroup;
        id = IDS_DEFAULT_UNUSED_SHORTCUT_FOLDER;
    }
    else if (psmi->dwFlags & SMIF_BROKEN_SHORTCUT)
    {
        ppszGroup = &psmti->pszLostTargetGroup;
        id = IDS_DEFAULT_LOST_TARGET_FOLDER;
    }
    else if (psmi->dwFlags & SMIF_README)
    {
        ppszGroup = &psmti->pszReadMeGroup;
        id = IDS_DEFAULT_README_FOLDER;
    }
    else
    {
        Assert(0);
    }

    // Fill in the group name if we haven't done so yet.
    if (!*ppszGroup)
    {
        TCHAR sz[MAX_PATH];
        if (LoadString(g_hinstApp, id, sz, ARRAYSIZE(sz)))
            Sz_AllocCopy(sz, ppszGroup);
    }
    
    Assert(*ppszGroup);

    // Set up the paths.
    if (SHGetSpecialFolderPath(NULL, szTo, CSIDL_PROGRAMS, FALSE))
    {
        PathAppend(szTo, *ppszGroup);
        SHCreateDirectory(psmti->hDlg, szTo);
        SHGetPathFromIDList(psmi->pidlItem, szFrom);
        MakeGenericNamesUnique(szFrom, szTo);
        _MoveFile(szFrom, szTo);
        // Dbg(TEXT("mitcg: Moving %s to %s."), szFrom, szTo);
    }
}

//----------------------------------------------------------------------------
void DeleteItem(PSMTIDYINFO psmti, PSMITEM psmi)
{
    TCHAR sz[MAX_PATH];
    USE(psmti);
    SHGetPathFromIDList(psmi->pidlItem, sz);
    _DeleteFile(sz);
    Dbg(TEXT("Deleting %s."), sz);
}

//----------------------------------------------------------------------------
void FixItem(PSMTIDYINFO psmti, PSMITEM psmi)
{
    TCHAR szPath[MAX_PATH];
    WCHAR wszPath[MAX_PATH];

    Assert(psmti->psl);
    Assert(psmti->ppf);
    
    SHGetPathFromIDList(psmi->pidlItem, szPath);
    STRTOOLESTR(wszPath, szPath);
    if (SUCCEEDED(psmti->ppf->lpVtbl->Load(psmti->ppf, wszPath, 0)))
    {
#ifdef DEBUG
        TCHAR sz[MAX_PATH];
        psmti->psl->lpVtbl->GetPath(psmti->psl, sz, ARRAYSIZE(sz), NULL, 0);
        Dbg(TEXT("ufs: Old %s New %s"), sz, psmi->pszNewTarget);
#endif                        
        psmti->psl->lpVtbl->SetPath(psmti->psl, psmi->pszNewTarget);
        psmti->ppf->lpVtbl->Save(psmti->ppf, NULL, TRUE);
    }
}

//----------------------------------------------------------------------------
BOOL SetupItemForFindInFolder(PSMTIDYINFO psmti, int i, IShellLink *psl, IPersistFile *ppf)
{
    BOOL fRet = FALSE;
    PSMITEM psmi = DSA_GetItemPtr(psmti->hdsaSMI, i);
    Assert(psmi);

    if (!(psmi->dwFlags & (SMIF_DELETE|SMIF_GROUP)) && (psmi->dwFlags & SMIF_BROKEN_SHORTCUT))
    {
        TCHAR szPath[MAX_PATH];
        WCHAR wszPath[MAX_PATH];
        
        SHGetPathFromIDList(psmi->pidlItem, szPath);
        STRTOOLESTR(wszPath, szPath);
        if (SUCCEEDED(ppf->lpVtbl->Load(ppf, wszPath, 0)))
        {
            // Get Find_Data to do comparisons with.
            if (LAlloc(SIZEOF(WIN32_FIND_DATA), &psmi->pfd))
            {
                if (SUCCEEDED(psl->lpVtbl->GetPath(psl, NULL, 0, psmi->pfd, 0)))
                {
                    // Make room for the new target.
                    Assert(!psmi->pszNewTarget);
                    LAlloc(CB_MAX_PATH, &psmi->pszNewTarget);
                    // Keep track of our items.
                    DPA_AppendPtr(psmti->hdpa, psmi);
                }
            }
        }
        fRet = TRUE;
    }
    return fRet;
}

//----------------------------------------------------------------------------
// FindInFolder needs a bit more info about the broken links to be able
// to do seatches properly so we set them up here.
BOOL SetupForFindInFolder(PSMTIDYINFO psmti)
{    
    BOOL fRet = FALSE;    
    IShellLink *psl;

    if (SUCCEEDED(ICoCreateInstance(&CLSID_ShellLink, &IID_IShellLink, &psl)))
    {
        IPersistFile *ppf;
        if (SUCCEEDED(psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, &ppf)))
        {
            int cItems = DSA_GetItemCount(psmti->hdsaSMI);
            int i;

            for (i=0; i<cItems; i++)
            {
                fRet |= SetupItemForFindInFolder(psmti, i, psl, ppf);
            }
            ppf->lpVtbl->Release(ppf);
        }
        psl->lpVtbl->Release(psl);
    }
    
    return fRet;
}

//----------------------------------------------------------------------------
// If we found a good match mark the item to be fixed otherwise mark it to
// be deleted.
void ApplyFixes(PSMTIDYINFO psmti)
{    
    int cItems = DPA_GetPtrCount(psmti->hdpa);
    int i;

    for (i=0; i<cItems; i++)
    {
        PSMITEM psmi = DPA_GetPtr(psmti->hdpa, i);
        if (psmi->nScore > MIN_NO_UI_SCORE)
            psmi->dwFlags |= SMIF_FIX;
        else
            psmi->dwFlags |= SMIF_DELETE;
    }
}

#if 0
//----------------------------------------------------------------------------
void UpdateFixedShortcuts(PSMTIDYINFO psmti)
{    
    int cItems = DPA_GetPtrCount(psmti->hdpa);
    IShellLink *psl;
    int i;

    // Create a link.
    if (SUCCEEDED(ICoCreateInstance(&CLSID_ShellLink, &IID_IShellLink, &psl)))
    {
        IPersistFile *ppf;
        if (SUCCEEDED(psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, &ppf)))
        {
            for (i=0; i<cItems; i++)
            {
                PSMITEM psmi = DPA_GetPtr(psmti->hdpa, i);
                if (psmi->nScore > MIN_NO_UI_SCORE)
                {
                    TCHAR szPath[MAX_PATH];
                    WCHAR wszPath[MAX_PATH];
                    
                    SHGetPathFromIDList(psmi->pidlItem, szPath);
                    STRTOOLESTR(wszPath, szPath);
                    if (SUCCEEDED(ppf->lpVtbl->Load(ppf, wszPath, 0)))
                    {
#ifndef NO_LOGGIN
                        TCHAR sz[MAX_PATH];
                        psl->lpVtbl->GetPath(psl, sz, ARRAYSIZE(sz), NULL, 0);
                        Dbg(TEXT("ufs: Old %s New %s"), sz, psmi->pszNewTarget);
#endif                        
                        psl->lpVtbl->SetPath(psl, psmi->pszNewTarget);
                        ppf->lpVtbl->Save(ppf, NULL, TRUE);
                    }
                }
            }
            ppf->lpVtbl->Release(ppf);
        }
        psl->lpVtbl->Release(psl);
    }
}
#endif

//----------------------------------------------------------------------------
BOOL GetSearchOriginForDrive(PSMTIDYINFO psmti, UINT iDrive, LPTSTR pszOrigin)
{
    BOOL fRet = FALSE;
    int cItems = DPA_GetPtrCount(psmti->hdpa);
    int i;
    
    for (i=0; i<cItems; i++)
    {
        PSMITEM psmi = DPA_GetPtr(psmti->hdpa, i);
        Assert(psmi);
        CharLowerBuff(psmi->pszTarget, 1);
        if ((TCHAR)iDrive + TEXT('a') == *psmi->pszTarget)
        {
            lstrcpy(pszOrigin, psmi->pszTarget);
            Dbg(TEXT("gdofd: Drive %d start at %s"), iDrive, pszOrigin);
            fRet = TRUE;
            break;
        }
    }

    return fRet;
}

#if 0
//----------------------------------------------------------------------------
void ResolveBrokenShortcuts(HWND hDlg, PSMTIDYINFO psmti)
{
    HDPA hdpa = DPA_Create(0);

    if (hdpa)
    {
        psmti->hdpa = hdpa;
        if (SetupForFindInFolder(psmti))
        {
            UINT iDrive;
            // Do a seperate search for each local drive.
            for (iDrive=0; iDrive<26; iDrive++)
            {          
                TCHAR szOrigin[MAX_PATH];
                
                // Try to fix them.
                if (GetSearchOriginForDrive(psmti, iDrive, szOrigin))
                {
                    psmti->pszSearchOrigin = szOrigin;
                    FindInFolder(hDlg, psmti);
                    UpdateFixedShortcuts(psmti);
                }
            }
        }
        DPA_Destroy(psmti->hdpa);
        psmti->hdpa = NULL;
    }
}
#endif

//----------------------------------------------------------------------------
void FixBrokenShortcuts(HWND hDlg, PSMTIDYINFO psmti)
{
    psmti->hdpa = DPA_Create(0);

    if (psmti->hdpa)
    {
        if (SetupForFindInFolder(psmti))
        {
            UINT iDrive;
            // Do a seperate search for each local drive.
            for (iDrive=0; iDrive<26; iDrive++)
            {          
                TCHAR szOrigin[MAX_PATH];
                
                // Try to fix them.
                if (GetSearchOriginForDrive(psmti, iDrive, szOrigin))
                {
                    psmti->pszSearchOrigin = szOrigin;
                    if (FindInFolder(hDlg, psmti) == IDOK)
                        ApplyFixes(psmti);
                }
            }
        }
    }
}

//----------------------------------------------------------------------------
void ApplyOptions(HWND hDlg, PSMTIDYINFO psmti)
{
    if (FilterLostTargets(psmti))
    {
        // if (psmti->dwFlags & SMTIF_DELETE_BROKEN_SHORTCUTS)
        //      DeleteBrokenShortcuts(psmti);
        // if (psmti->dwFlags & SMTIF_GROUP_BROKEN_SHORTCUTS)
        //      GroupBrokenShortcuts(psmti);
        if (psmti->dwFlags & SMTIF_FIX_BROKEN_SHORTCUTS)
             FixBrokenShortcuts(hDlg, psmti);
    }

    if (FilterReadMes(psmti))
    {
        if (psmti->dwFlags & SMTIF_GROUP_READMES)
            GroupReadMes(psmti);
        if (psmti->dwFlags & SMTIF_DELETE_READMES)
            DeleteReadMes(psmti);
    }

        
    if (FilterUnusedShortcuts(psmti))
    {
        if (psmti->dwFlags & SMTIF_DELETE_UNUSED_SHORTCUTS)
             DeleteUnusedShortcuts(psmti);
        if (psmti->dwFlags & SMTIF_GROUP_UNUSED_SHORTCUTS)
             GroupUnusedShortcuts(psmti);
    }
    
    if (FilterSingleItemFolders(psmti))
    {
        if (psmti->dwFlags & SMTIF_FIX_SINGLE_ITEM_FOLDERS)
            FixSingleItemFolders(psmti);
    }

    if (FilterEmptyFolders(psmti))
    {
        if (psmti->dwFlags & SMTIF_REMOVE_EMPTY_FOLDERS)
            FixEmptyFolders(psmti);
    }
}

//----------------------------------------------------------------------------
typedef void (*PFNCOMMIT)(PSMTIDYINFO psmti, PSMITEM psmi);

//----------------------------------------------------------------------------
#define CBTF_NONE               0x0000
#define CBTF_FILES              0x0001
#define CBTF_FOLDERS            0x0002

//----------------------------------------------------------------------------
void CommitByType(PSMTIDYINFO psmti, DWORD dwFlags, PFNCOMMIT pfn, DWORD dwCBTFlags)
{
    int cItems = DSA_GetItemCount(psmti->hdsaSMI);
    int i;

    // We can handle most changes on an item by item basis.
    for (i=0; i<cItems; i++)
    {
        PSMITEM psmi = DSA_GetItemPtr(psmti->hdsaSMI, i);
        Assert(psmi);

        if (psmi->dwFlags & dwFlags)
        {
            if (psmi->dwFlags & SMIF_FOLDER)
            {
                if (dwCBTFlags & CBTF_FOLDERS)
                    (*pfn)(psmti, psmi);
            }
            else
            {
                if (dwCBTFlags & CBTF_FILES)
                    (*pfn)(psmti, psmi);
            }
        }
    } 
}

//----------------------------------------------------------------------------
void CommitChanges(PSMTIDYINFO psmti)
{
    int cItems = DSA_GetItemCount(psmti->hdsaSMI);
    BOOL fFixBrokenShortcuts = FALSE;

    CommitByType(psmti, SMIF_GROUP, MoveItemToCustomGroup, CBTF_FILES|CBTF_FOLDERS);
    CommitByType(psmti, SMIF_MOVE_UP, MoveUp, CBTF_FILES|CBTF_FOLDERS);

    if (SUCCEEDED(ICoCreateInstance(&CLSID_ShellLink, &IID_IShellLink, &psmti->psl)))
    {
        if (SUCCEEDED(psmti->psl->lpVtbl->QueryInterface(psmti->psl, &IID_IPersistFile, &psmti->ppf)))
        {
            CommitByType(psmti, SMIF_FIX, FixItem, CBTF_FILES|CBTF_FOLDERS);
            psmti->ppf->lpVtbl->Release(psmti->ppf);
            psmti->ppf = NULL;
        }
        psmti->psl->lpVtbl->Release(psmti->psl);
        psmti->psl = NULL;
    }
    
    CommitByType(psmti, SMIF_DELETE, DeleteItem, CBTF_FILES);
    CommitByType(psmti, SMIF_DELETE, DeleteItem, CBTF_FOLDERS);
}

//----------------------------------------------------------------------------
void BuildList(PSMTIDYINFO psmti)
{
    // Build the total list of links.
    if (!(psmti->dwFlags & SMTIF_BUILT_LIST))
    {
        // Build list of the shortcuts.
        if (SMIList_Build(psmti))
        {
            psmti->dwFlags |= SMTIF_BUILT_LIST;
        }
    }

    Assert(psmti->dwFlags & SMTIF_BUILT_LIST);
}

//----------------------------------------------------------------------------
void OnFinish(HWND hDlg, LPPROPSHEETPAGE pps)
{
    PSMTIDYINFO psmti = (PSMTIDYINFO) pps->lParam;
    HCURSOR hcur;
    
    if (Cursor_Wait(&hcur))
    {
        SendMessage(GetParent(hDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_DISABLEDFINISH);
        // UpdateWindow(hDlg);
        BuildList(psmti);
        ApplyOptions(hDlg, psmti);
        CommitChanges(psmti);
        Cursor_UnWait(hcur);
    }
}
