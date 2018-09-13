//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
#include "smtidy.h"
#include "filter.h"
#include "util.h"
#include "resource.h"

//----------------------------------------------------------------------------
typedef BOOL (*PFNENUMTARGETSCALLBACK)(PSMTIDYINFO psmti, PSMITEM psmi, PTSTR pszPath, BOOL fArgs);

//----------------------------------------------------------------------------
void Enum_Targets(PSMTIDYINFO psmti, PFNENUMTARGETSCALLBACK pfnCallback)
{
    UINT cMissing = 0;
    Assert(!psmti->psl);
        
    if (SUCCEEDED(ICoCreateInstance(&CLSID_ShellLink, &IID_IShellLink, &psmti->psl)))
    {
        if (SUCCEEDED(psmti->psl->lpVtbl->QueryInterface(psmti->psl, &IID_IPersistFile, &psmti->ppf)))
        {
            int cItems = DSA_GetItemCount(psmti->hdsaSMI);
            int i;
            
            for (i=0; i<cItems; i++)
            {
                PSMITEM psmi = DSA_GetItemPtr(psmti->hdsaSMI, i);
                TCHAR szPath[MAX_PATH];
                WCHAR wszPath[MAX_PATH];

                Assert(psmi);

                // Only enum links to files.
                if (!(psmi->dwFlags & (SMIF_FOLDER|SMIF_TARGET_NOT_FILE)))
                {
                    SHGetPathFromIDList(psmi->pidlItem, szPath);
                    STRTOOLESTR(wszPath, szPath);
                    if (SUCCEEDED(psmti->ppf->lpVtbl->Load(psmti->ppf, wszPath, 0)))
                    {
                        if (SUCCEEDED(psmti->psl->lpVtbl->GetPath(psmti->psl, szPath, ARRAYSIZE(szPath), NULL, 0)))
                        {
                            TCHAR szArgs[MAX_PATH];
                            BOOL fArgs = FALSE;
                            
                            if ((SUCCEEDED(psmti->psl->lpVtbl->GetArguments(psmti->psl, szArgs, ARRAYSIZE(szArgs)))) && (*szArgs))
                                fArgs = TRUE;
                            if (!(*pfnCallback)(psmti, psmi, szPath, fArgs))
                                break;
                        }
                    }
                }
            }
            
            psmti->ppf->lpVtbl->Release(psmti->ppf);
            psmti->ppf = NULL;
        }
        psmti->psl->lpVtbl->Release(psmti->psl);
        psmti->psl = NULL;
    }
}

//----------------------------------------------------------------------------
BOOL FilterLostTargetsCallback(PSMTIDYINFO psmti, PSMITEM psmi, PTSTR pszPath, BOOL fArgs)
{
    USE(fArgs);
    
    Assert(psmti);
    Assert(pszPath);
    
    if (DriveType(DRIVEID(pszPath)) == DRIVE_FIXED)
    {
        // Yep, local path.
        if (!PathFileExists(pszPath))
        {
            Dbg(TEXT("flt: Target %s is missing."), pszPath);
            psmi->dwFlags |= SMIF_BROKEN_SHORTCUT;
            psmti->dwFlags |= SMTIF_LOST_TARGETS;
        }
    }
    else
    {
        Dbg(TEXT("flt: Target %s isn't local - ignoring."), pszPath);
    }

    return TRUE;
}

//----------------------------------------------------------------------------
BOOL FilterLostTargets(PSMTIDYINFO psmti)
{
    Enum_Targets(psmti, FilterLostTargetsCallback);

    return (psmti->dwFlags & SMTIF_LOST_TARGETS);
}

//----------------------------------------------------------------------------
BOOL FilterReadMesCallback(PSMTIDYINFO psmti, PSMITEM psmi, LPTSTR pszPath, BOOL fArgs)
{
    LPTSTR pszExt = PathFindExtension(pszPath);
    LPTSTR pszApp = PathFindFileName(pszPath);

    if (!(psmi->dwFlags & (SMIF_DELETE|SMIF_UNUSED_SHORTCUT|SMIF_BROKEN_SHORTCUT)))
    {
        if (lstrcmpi(pszExt, TEXT(".wri")) == 0 || 
            lstrcmpi(pszExt, TEXT(".txt")) == 0 ||
            lstrcmpi(pszExt, TEXT(".hlp")) == 0)
        {
            Dbg(TEXT("flt: Target %s is info doc."), pszPath);
            psmi->dwFlags |= SMIF_README;
            psmti->dwFlags |= SMTIF_READMES;
        }
        else if (fArgs && ((lstrcmpi(pszApp, TEXT("winhelp.exe")) == 0) ||
            lstrcmpi(pszApp, TEXT("write.exe")) == 0 ||
            lstrcmpi(pszApp, TEXT("wordpad.exe")) == 0 ||
            lstrcmpi(pszApp, TEXT("notepad.exe")) == 0))
        {
            Dbg(TEXT("flt: Target %s is info doc."), pszPath);
            psmi->dwFlags |= SMIF_README;
            psmti->dwFlags |= SMTIF_READMES;
        }
    }    
    return TRUE;
}

//----------------------------------------------------------------------------
BOOL FilterReadMes(PSMTIDYINFO psmti)
{
    // Targets extension is .txt .wri or .hlp.
    Enum_Targets(psmti, FilterReadMesCallback);

    return (psmti->dwFlags & SMTIF_READMES);
}

//----------------------------------------------------------------------------
// Last access time for a file.
// Returns FALSE for missing file.
BOOL File_GetLastAccessTime(PTSTR pszPath, FILETIME* pft)
{
    WIN32_FIND_DATA fd;
    HANDLE hff = FindFirstFile(pszPath, &fd);
    BOOL fRet = FALSE;
    
    Assert(pft);
    
    if (hff != INVALID_HANDLE_VALUE)
    {
        *pft = fd.ftLastAccessTime;
        // Return FALSE for dirs and roots.
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !PathIsRoot(pszPath))
            fRet = TRUE;
        FindClose(hff);
    }

    return fRet;
}

//----------------------------------------------------------------------------
// Stolen from kernel32\ctime.c - modified to assume large integers are the
// same as filetimes.
typedef struct {ULONG LowPart; LONG HiPart;} MY_LARGE_INTEGER;

FILETIME NTAPI RtlExtendedMagicDivide(FILETIME Dividend, FILETIME MagicDivisor, CCHAR ShiftCount);
FILETIME NTAPI RtlLargeIntegerSubtract(FILETIME Minuend, FILETIME Subtrahend);

FILETIME MyMagic10000 = {0xe219652c, 0xd1b71758};
#define SHIFT10000  13

FILETIME MyMagic86400000 = {0xfa67b90e, 0xc6d750eb};
#define SHIFT86400000   26

#define Convert100nsToMilliseconds(TIME) (RtlExtendedMagicDivide((TIME), MyMagic10000, SHIFT10000))
#define ConvertMillisecondsToDays(TIME) (RtlExtendedMagicDivide((TIME), MyMagic86400000, SHIFT86400000))

//----------------------------------------------------------------------------
BOOL FilterUnusedShortcutsCallback(PSMTIDYINFO psmti, PSMITEM psmi, PTSTR pszPath, BOOL fArgs)
{
    FILETIME ftPath;

    USE(fArgs);
    
    // Ignore targets we can't access.
    if (!(psmi->dwFlags & (SMIF_DELETE|SMIF_BROKEN_SHORTCUT)))
    {
        // Flag files that haven't been accessed for 90 days.
        if (File_GetLastAccessTime(pszPath, &ftPath))
        {
            FILETIME ft;
            
            GetSystemTimeAsFileTime(&ft);

            ft = RtlLargeIntegerSubtract(ft, ftPath);
            ft = Convert100nsToMilliseconds(ft);
            ft = ConvertMillisecondsToDays(ft);
            // Highword must be 0 by now so don't worry about it.
            if (ft.dwLowDateTime > 90)
            {
                Dbg(TEXT("flt: Target %s hasn't been accessed recently."), pszPath);
                psmi->dwFlags |= SMIF_UNUSED_SHORTCUT;
                psmti->dwFlags |= SMTIF_UNUSED_SHORTCUTS;
            }
            else
            {
                // Dbg(TEXT("flt: Target %s accessed %d days ago."), pszPath, ft.dwLowDateTime);
            }
        }
        else
        {
            Dbg(TEXT("flt: Target %s has no access time."), pszPath);
        }
    }
    return TRUE;
}

//----------------------------------------------------------------------------
BOOL FilterUnusedShortcuts(PSMTIDYINFO psmti)
{
    Enum_Targets(psmti, FilterUnusedShortcutsCallback);

    return (psmti->dwFlags & SMTIF_UNUSED_SHORTCUTS);
}

//----------------------------------------------------------------------------
typedef HDSA HFOLDERLIST;
typedef HFOLDERLIST* PHFOLDERLIST;
typedef struct
{
    PIDL pidlFolder;
    UINT cRef;
} FOLDERITEM, *PFOLDERITEM;

//----------------------------------------------------------------------------
BOOL FolderList_Create(PHFOLDERLIST phfl)
{
    BOOL fRet = FALSE;
    
    Assert(phfl);
    *phfl = (HFOLDERLIST)DSA_Create(sizeof(FOLDERITEM), 0);
    if (*phfl)
        fRet = TRUE;

    return fRet;
}

//----------------------------------------------------------------------------
BOOL ILIdentical(PIDL p1, PIDL p2)
{
    BOOL fRet = FALSE;
    UINT cb1 = ILGetSize(p1);

    if (cb1 == ILGetSize(p2))
    {
        if (memcmp(p1, p2, cb1) == 0)
            fRet = TRUE;
    }

    return fRet;
}

//----------------------------------------------------------------------------
// NB This clones the pidl if needed.
BOOL FolderList_Add(HFOLDERLIST hfl, PIDL pidlFolder)
{
    int cItems = DSA_GetItemCount(hfl);
    BOOL fFound = FALSE;
    BOOL fRet = FALSE;
    int i;

    
    for (i=0; i<cItems; i++)
    {
        PFOLDERITEM pfi = DSA_GetItemPtr(hfl, i);
        
        Assert(pfi);
        
        if (ILIdentical(pidlFolder, pfi->pidlFolder))
        {
#ifdef DEBUG
            {
                TCHAR sz1[MAX_PATH];
                TCHAR sz2[MAX_PATH];
                SHGetPathFromIDList(pidlFolder, sz1);
                SHGetPathFromIDList(pfi->pidlFolder, sz2);
    			if (lstrcmp(sz1, sz2) != 0)
    				DebugBreak();
    			// Dbg(TEXT("fl_a: %s (%d)"), sz1, pfi->cRef+1);
            }
#endif
            pfi->cRef++;
            fFound = TRUE;
            fRet = TRUE;
            break;
        }
    }

    // If it's not in the list add it now.
    if (!fFound)
    {
        FOLDERITEM fi;
#ifdef DEBUG
        {
            TCHAR sz1[MAX_PATH];
            SHGetPathFromIDList(pidlFolder, sz1);
			// Dbg(TEXT("fl_a: %s (%d)"), sz1, 1);
        }
#endif
        
        fi.pidlFolder = ILClone(pidlFolder);
        fi.cRef = 1;
        if (DSA_AppendItem(hfl, &fi))
            fRet = TRUE;
    }

    return fRet;
}

//----------------------------------------------------------------------------
BOOL FolderList_Destroy(HFOLDERLIST hfl)
{
    int cItems = DSA_GetItemCount(hfl);
    BOOL fRet = FALSE;
    int i;

    Assert(hfl);
    
    for (i=0; i<cItems; i++)
    {
        PFOLDERITEM pfi = DSA_GetItemPtr(hfl, i);
        Assert(pfi);
        Assert(pfi->pidlFolder);
        Assert(pfi->cRef);
        ILFree(pfi->pidlFolder);
        pfi->pidlFolder = NULL;
        pfi->cRef = 0;
    }
    DSA_Destroy(hfl);

    return fRet;
}

//----------------------------------------------------------------------------
// NB ppidl is the one in the list not a clone so don't free it.
BOOL FolderList_GetItemRef(HFOLDERLIST hfl, UINT i, PPIDL ppidl, PUINT pcRef)
{   
    PFOLDERITEM pfi;
    
    Assert(hfl);
    Assert(ppidl);
    Assert(pcRef);

    pfi = DSA_GetItemPtr(hfl, i);
    Assert(pfi);
    *ppidl = pfi->pidlFolder;
    *pcRef = pfi->cRef;

    return TRUE;
}

//----------------------------------------------------------------------------
BOOL FolderList_GetCount(HFOLDERLIST hfl, PUINT pc)
{
    Assert(pc);
    Assert(hfl);
    
    *pc = DSA_GetItemCount(hfl);
    
    return TRUE;
}
                
//----------------------------------------------------------------------------
BOOL BuildListOfFolders(HFOLDERLIST hFL, PSMTIDYINFO psmti)
{
    int cItems = DSA_GetItemCount(psmti->hdsaSMI);
    int i;
    
    Assert(hFL);

    for (i=0; i<cItems; i++)
    {
        PSMITEM psmi = DSA_GetItemPtr(psmti->hdsaSMI, i);
        Assert(psmi);

        if (!(psmi->dwFlags & (SMIF_DELETE|SMIF_GROUP|SMIF_MOVE_UP)))
        {
            PIDL pidlFolder = ILClone(psmi->pidlItem);                
            if (pidlFolder)
            {
                // 1 ref for the folder itself and 1 each for every item 
                // it contains.
                if (psmi->dwFlags & SMIF_FOLDER)
                    FolderList_Add(hFL, pidlFolder);
                ILRemoveLastID(pidlFolder);
                FolderList_Add(hFL, pidlFolder);
                ILFree(pidlFolder);
            }
        }
    }

    return TRUE;
}

//----------------------------------------------------------------------------
void MarkAllItemsInFolderAsSingle(PSMTIDYINFO psmti, PIDL pidlFolder)
{
    int cItems = DSA_GetItemCount(psmti->hdsaSMI);
    int i;

    for (i=0; i<cItems; i++)
    {
        PSMITEM psmi = DSA_GetItemPtr(psmti->hdsaSMI, i);
        Assert(psmi);
        if (ILIsParent(pidlFolder, psmi->pidlItem, TRUE) && 
            !(psmi->dwFlags & (SMIF_DELETE|SMIF_GROUP|SMIF_MOVE_UP)))
        {
            psmi->dwFlags |= SMIF_SINGLE_ITEM;
        }
    }    
}

//----------------------------------------------------------------------------
void MarkFolderAsEmpty(PSMTIDYINFO psmti, PIDL pidlFolder)
{
    int cItems = DSA_GetItemCount(psmti->hdsaSMI);
    int i;

    for (i=0; i<cItems; i++)
    {
        PSMITEM psmi = DSA_GetItemPtr(psmti->hdsaSMI, i);
        Assert(psmi);
        if (ILIdentical(psmi->pidlItem, pidlFolder))
        {
            psmi->dwFlags |= SMIF_EMPTY_FOLDER;
        }
    }    
}                                          

//----------------------------------------------------------------------------
// Is this start menu, programs or startup.
BOOL IsSpecialFolder(LPCTSTR pszPath)
{

    // Setup an array of ids to check against.
    static const int s_aiSpecialFolders[]= {CSIDL_PROGRAMS, CSIDL_STARTMENU, CSIDL_STARTUP,
                     CSIDL_COMMON_STARTMENU,CSIDL_COMMON_PROGRAMS,CSIDL_COMMON_STARTUP};

    int i;
    TCHAR szSpecial[MAX_PATH];
    
    for (i=0; i < ARRAYSIZE(s_aiSpecialFolders); i++)
    {
        SHGetSpecialFolderPath(NULL, szSpecial, s_aiSpecialFolders[i], FALSE);
        if (lstrcmpi(szSpecial, pszPath) == 0)
            return TRUE;
    }
    return FALSE;
}

//----------------------------------------------------------------------------
// There are a bunch of folders which we allow to be empty including the root.
BOOL EmptyFolderAllowed(PIDL pidl)
{
    TCHAR szFolder[MAX_PATH];
    BOOL fRet = FALSE;
    LPTSTR pszFolder;
    int i;

    SHGetPathFromIDList(pidl, szFolder);
    if (IsSpecialFolder(szFolder))
    {
        fRet = TRUE;
    }
    else
    {
        pszFolder = PathFindFileName(szFolder);
        for (i=IDS_FIRST_ALLOW_EMPTY_FOLDER; i <= IDS_LAST_ALLOW_EMPTY_FOLDER; i++)
        {
            TCHAR sz[MAX_PATH];
            if (LoadString(g_hinstApp, i, sz, ARRAYSIZE(sz)))
            {
                if (lstrcmpi(sz, pszFolder) == 0)
                {
                    Dbg(TEXT("sia: %s is allowed to be empty."), szFolder);
                    fRet = TRUE;
                    break;
                }
            }    
        }
    }
    
    return fRet;
}

//----------------------------------------------------------------------------
// There are a bunch of folders where we allow single items.
BOOL SingleItemsAllowed(PIDL pidl)
{
    TCHAR szFolder[MAX_PATH];
    BOOL fRet = FALSE;
    LPTSTR pszFolder;
    int i;

    SHGetPathFromIDList(pidl, szFolder);        
    if (IsSpecialFolder(szFolder))
    {
        fRet = TRUE;
    }
    else
    {
        pszFolder = PathFindFileName(szFolder);
        for (i=IDS_FIRST_ALLOW_SINGLE_ITEMS; i <= IDS_LAST_ALLOW_SINGLE_ITEMS; i++)
        {
            TCHAR sz[MAX_PATH];
            if (LoadString(g_hinstApp, i, sz, ARRAYSIZE(sz)))
            {
                if (lstrcmpi(sz, pszFolder) == 0)
                {
                    Dbg(TEXT("sia: %s is allowed to contain single items."), szFolder);
                    fRet = TRUE;
                    break;
                }
            }    
        }
    }
    
    return fRet;
}

//----------------------------------------------------------------------------
BOOL MarkAllSingleItems(HFOLDERLIST hFL, PSMTIDYINFO psmti)
{
    int cItems;
    
    Assert(hFL);

    if (FolderList_GetCount(hFL, &cItems))
    {
        int i;
        
        for (i=0; i<cItems; i++)
        {
            PIDL pidlFolder;
            UINT cRef;
            if (FolderList_GetItemRef(hFL, i, &pidlFolder, &cRef))
            {
                Assert(cRef);
                if ((cRef == 2) && !SingleItemsAllowed(pidlFolder))
                {
#ifndef NO_LOGGING
                    TCHAR szPath[MAX_PATH];

                    SHGetPathFromIDList(pidlFolder, szPath);
                    Dbg(TEXT("%s is a single item folder"), szPath);
#endif
                    psmti->dwFlags |= SMTIF_SINGLE_ITEM_FOLDERS;
                    MarkAllItemsInFolderAsSingle(psmti, pidlFolder);
                }
            }
        }
    }

    return TRUE;
}

//----------------------------------------------------------------------------
BOOL MarkAllEmptyFolders(HFOLDERLIST hFL, PSMTIDYINFO psmti)
{
    int cItems;
    
    Assert(hFL);

    if (FolderList_GetCount(hFL, &cItems))
    {
        int i;
        
        for (i=0; i<cItems; i++)
        {
            PIDL pidlFolder;
            UINT cRef;
            if (FolderList_GetItemRef(hFL, i, &pidlFolder, &cRef))
            {
                Assert(cRef);
#if 0
                {
                    TCHAR szPath[MAX_PATH];
                    SHGetPathFromIDList(pidlFolder, szPath);
                    Dbg(TEXT("%s has ref count of %d"), szPath, cRef);
                }
#endif
                if ((cRef == 1) && !EmptyFolderAllowed(pidlFolder))
                {
                    psmti->dwFlags |= SMTIF_EMPTY_FOLDERS;
                    MarkFolderAsEmpty(psmti, pidlFolder);
#ifndef NO_LOGGIN
                    {
                        TCHAR szPath[MAX_PATH];
                        SHGetPathFromIDList(pidlFolder, szPath);
                        Dbg(TEXT("%s is an empty folder."), szPath);
                    }
#endif
                }
            }
        }
    }

    return TRUE;
}

//----------------------------------------------------------------------------
// NB There's no point in doing all the work up front since previous
// filters can mess with the list. There should never be enough items that
// doing multiple-passes will cause problems.
BOOL FilterSingleItemFolders(PSMTIDYINFO psmti)
{
    HFOLDERLIST hFL;

    if (FolderList_Create(&hFL))
    {
        if (BuildListOfFolders(hFL, psmti))
        {
            MarkAllSingleItems(hFL, psmti);
        }
        FolderList_Destroy(hFL);
    }        

    return (psmti->dwFlags & SMTIF_SINGLE_ITEM_FOLDERS);
}

//----------------------------------------------------------------------------
// Find empty folders that haven't already been removed
BOOL FilterEmptyFolders(PSMTIDYINFO psmti)
{
    HFOLDERLIST hFL;

    if (FolderList_Create(&hFL))
    {
        if (BuildListOfFolders(hFL, psmti))
        {
            MarkAllEmptyFolders(hFL, psmti);
        }
        FolderList_Destroy(hFL);
    }        
    
    return (psmti->dwFlags & SMTIF_EMPTY_FOLDERS);
}
