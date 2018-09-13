//----------------------------------------------------------------------------
// Stolen and modified from Shell32.
//----------------------------------------------------------------------------
#include "smtidy.h"
#include "lnktrack.h"
#include "util.h"
#include "resource.h"

//----------------------------------------------------------------------------
BOOL IsNullTime(const FILETIME *pft)
{
    FILETIME ftNull = {0, 0};

    return CompareFileTime(&ftNull, pft) == 0;
}

//----------------------------------------------------------------------------
// compute a weighted score for a given find
int ScoreFindData(PSMITEM psmi, LPCTSTR pszNewPath, const WIN32_FIND_DATA *pfd)
{
    int iScore = 0;
    BOOL bSameName, bSameCreateDate, bSameWriteTime, bSameExt, bHasCreateDate;
    TCHAR szOrig[MAX_PATH];
    
    bSameName = lstrcmpi(psmi->pfd->cFileName, pfd->cFileName) == 0;
    bSameExt = lstrcmpi(PathFindExtension(psmi->pfd->cFileName), PathFindExtension(pfd->cFileName)) == 0;
    bHasCreateDate = !IsNullTime(&pfd->ftCreationTime);
    bSameCreateDate = bHasCreateDate && (CompareFileTime(&pfd->ftCreationTime, &psmi->pfd->ftCreationTime) == 0);
    bSameWriteTime  = !IsNullTime(&pfd->ftLastWriteTime) && (CompareFileTime(&pfd->ftLastWriteTime, &psmi->pfd->ftLastWriteTime) == 0);

    if (bSameName || bSameCreateDate)
    {
        if (bSameName)
            iScore += bHasCreateDate ? 16 : 32;
        if (bSameCreateDate)
        {
            iScore += 32;
            if (bSameExt)
                iScore += 8;
        }
        if (bSameWriteTime)
            iScore += 8;
        if (pfd->nFileSizeLow == psmi->pfd->nFileSizeLow)
            iScore += 4;
        // If it is in the same folder as the original give it a slight bonus.
        lstrcpy(szOrig, psmi->pszTarget);
        PathRemoveFileSpec(szOrig);
        if (lstrcmpi(szOrig, pszNewPath) == 0)
            iScore += 2;
    }
    else
    {
        // doesn't have create date, apply different rules
        if (bSameExt)
            iScore += 8;
        if (bSameWriteTime)
            iScore += 8;
        if (pfd->nFileSizeLow == psmi->pfd->nFileSizeLow)
            iScore += 4;
    }

    return iScore;
}

//----------------------------------------------------------------------------
// REVIEW IANEL - I stole this from Shell32, there must be a better way.
LPCTSTR StrSlash(LPCTSTR psz)
{
    for (; *psz && *psz != TEXT('\\'); psz = CharNext(psz));

    return psz;
}

//----------------------------------------------------------------------------
// REVIEW IANEL - I stole this from Shell32, there must be a better way.
__inline BOOL DBL_BSLASH(LPCTSTR psz)
{
    return (psz[0] == TEXT('\\') && psz[1] == TEXT('\\'));
}

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
BOOL BeenThereDoneThat(LPCTSTR pszOriginal, LPCTSTR pszPath)
{
    return PathCommonPrefix(pszOriginal, pszPath, NULL) == lstrlen(pszPath);
}

//----------------------------------------------------------------------------
typedef struct _PATH_NODE {
    struct _PATH_NODE *pNext;
    TCHAR szPath[1];
} PATH_NODE;

//----------------------------------------------------------------------------
PATH_NODE *LAllocPathNode(LPCTSTR pszStr)
{
    PATH_NODE *p;

    if (LAlloc(lstrlen(pszStr)*SIZEOF(TCHAR) + SIZEOF(PATH_NODE), &p))
        lstrcpy(p->szPath, pszStr);

    return p;
}

//----------------------------------------------------------------------------
BOOL IsNormalDirectoryEntry(const WIN32_FIND_DATA *pcfd)
{
    BOOL fRet = TRUE;
    Assert(pcfd);
    Assert(*pcfd->cFileName);
    
    if (pcfd->cFileName[0] == TEXT('.'))
    {
        if (pcfd->cFileName[1] == TEXT('.'))
        {
            if (pcfd->cFileName[2] == TEXT('\0'))
                fRet = FALSE;
        }
        else if (pcfd->cFileName[1] == TEXT('\0'))
        {
            fRet = FALSE;
        }
    }

    return fRet;
}

//----------------------------------------------------------------------------
typedef BOOL (*PFNCBBREADTHFIRSTSEARCH)(LPCTSTR pszPath, const WIN32_FIND_DATA *pfd, LPVOID pv);

//----------------------------------------------------------------------------
#define BFSF_CONTINUE       0x0000
#define BFSF_SKIP_FOLDER    0x0001
#define BFSF_STOP           0x0002

//----------------------------------------------------------------------------
BOOL BreadthFirstSearch(LPCTSTR pszSearchOrigin, PFNCBBREADTHFIRSTSEARCH pfn, LPVOID pv)
{
    DWORD dwFlags = BFSF_CONTINUE;
    BOOL fRet = TRUE;
    PATH_NODE *pFree, *pFirst, *pLast;  // list in FIFO order

    // Initial list of the one folder we want to look in.
    pLast = pFirst = LAllocPathNode(pszSearchOrigin);

    while (pFirst && (dwFlags != BFSF_STOP))
    {
        TCHAR szPath[MAX_PATH];
        HANDLE hfind;
        WIN32_FIND_DATA fd;
        
        // Dbg(TEXT("sif: %s"), pFirst->szPath);
        PathCombine(szPath, pFirst->szPath, TEXT("*.*"));
        hfind = FindFirstFile(szPath, &fd);
        if (hfind != INVALID_HANDLE_VALUE)
        {
            do {
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    if (IsNormalDirectoryEntry(&fd))
                    {
                        dwFlags = (*pfn)(pFirst->szPath, &fd, pv);
                        if (dwFlags != BFSF_SKIP_FOLDER)
                        {
                            PATH_NODE *p;
                            PathCombine(szPath, pFirst->szPath, fd.cFileName);
                            p = LAllocPathNode(szPath);
                            if (p)
                            {
                                pLast->pNext = p;
                                pLast = p;
                            }
                        }
                    }
                }
                else
                {
                    dwFlags = (*pfn)(pFirst->szPath, &fd, pv);
                }
            } while((dwFlags != BFSF_STOP) && FindNextFile(hfind, &fd));
            FindClose(hfind);
        }

        // Remove the element we just searched from the list.
        Assert(pFirst && pLast);
        pFree = pFirst;
        pFirst = pFirst->pNext;
        Assert(pFirst || pFree == pLast);
        LFree(pFree);
    }

    // if we were canceled make sure we clean up
    while (pFirst)
    {
        pFree = pFirst;
        pFirst = pFirst->pNext;
        LFree(pFree);
    }

    return fRet;
}

//----------------------------------------------------------------------------
BOOL PathIsLink(LPCTSTR szFile)
{
    return lstrcmpi(TEXT(".lnk"), PathFindExtension(szFile)) == 0;
}

//----------------------------------------------------------------------------
// REVIEW UNDONE IANEL - Ick! there's no easy way to do this.
BOOL FileIsInBitBucket(LPCTSTR psz)
{
    USE(psz);
    return FALSE;
}

//----------------------------------------------------------------------------
#define BYTES_PER_MBYTE 0x100000

//----------------------------------------------------------------------------
DWORD BFSFolderCallback(PSMTIDYINFO psmti, LPCTSTR pszPath, const WIN32_FIND_DATA *pfd)
{
    DWORD dwRet = BFSF_CONTINUE;
    TCHAR szPath[MAX_PATH];
    
    // Avoid revisiting a subtree as we got up from the start folder.
    PathCombine(szPath, pszPath, pfd->cFileName);
    if (BeenThereDoneThat(psmti->pszSearchOrigin, szPath) || FileIsInBitBucket(szPath))
    {
        dwRet = BFSF_SKIP_FOLDER;
    }
    else
    {
        WORD wMbSeen;
        // Falsely assume each directory eats a cluster.
        psmti->qwSeen += psmti->dwBPC;
        wMbSeen = (WORD)(psmti->qwSeen / BYTES_PER_MBYTE);
    	SendDlgItemMessage(psmti->hDlg, IDC_PROGRESS, PBM_SETPOS, wMbSeen, 0);
	}
    return dwRet;
}

//----------------------------------------------------------------------------
__inline BOOL SameDrive(LPTSTR pszPath1, LPTSTR pszPath2)
{
    CharLowerBuff(pszPath1, 1);    
    CharLowerBuff(pszPath2, 1);    
    return *pszPath1 == *pszPath2;
}

//----------------------------------------------------------------------------
DWORD BFSFileCallback(PSMTIDYINFO psmti, LPCTSTR pszPath, const WIN32_FIND_DATA *pfd)
{
    DWORD dwRet = BFSF_STOP;
    WORD wMbSeen;
    int cItems = DPA_GetPtrCount(psmti->hdpa);
    int i;
    
    // Rescore all the missing shortcuts.
    for (i=0; i<cItems; i++)
    {
        PSMITEM psmi = DPA_GetPtr(psmti->hdpa, i);

        // Is it still broken?
        if ((psmi->dwFlags & SMIF_BROKEN_SHORTCUT) && SameDrive(psmti->pszSearchOrigin, psmi->pszTarget))
        {
            DWORD dwFind = pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
            dwRet = BFSF_CONTINUE;
            
            if (!(dwFind ^ psmi->dwMatch) && !PathIsLink(pfd->cFileName))
            {
                // Both are files or folders, see how it scores.
                int nScore = ScoreFindData(psmi, pszPath, pfd);
                if (nScore > psmi->nScore)
                {
                    // Store the score and fully qualified path.
                    psmi->nScore = nScore;
                    Assert(psmi->pszNewTarget);
                    PathCombine(psmi->pszNewTarget, pszPath, pfd->cFileName);
                    Dbg(TEXT("Better match found %s, %d"), pfd->cFileName, nScore);
                }
            }
        }
    }

    // Assume no single file is bigger than 4Gb (!) and round up to the sector size.
    psmti->qwSeen += ((pfd->nFileSizeLow / psmti->dwBPC) + 1) * (psmti->dwBPC); 
    wMbSeen = (WORD)(psmti->qwSeen / BYTES_PER_MBYTE);
	SendDlgItemMessage(psmti->hDlg, IDC_PROGRESS, PBM_SETPOS, wMbSeen, 0);

    return dwRet;
}

//----------------------------------------------------------------------------
DWORD BFSCallback(LPCTSTR pszPath, const WIN32_FIND_DATA *pfd, LPVOID pv)
{
    PSMTIDYINFO psmti = pv;
    DWORD dwRet = BFSF_CONTINUE;

    Assert(pszPath && *pszPath);
    Assert(pfd);
    Assert(pv);
    
    if (psmti->dwFlags & SMTIF_STOP_THREAD)
    {
        dwRet = BFSF_STOP;
    }
    else
    {
        if (pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            dwRet = BFSFolderCallback(psmti, pszPath, pfd);
        else
            dwRet = BFSFileCallback(psmti, pszPath, pfd);
    }    
    
    return dwRet;
}

//----------------------------------------------------------------------------
DWORD CALLBACK LinkFindThreadProc(LPVOID pv)
{
    PSMTIDYINFO psmti = pv;
    TCHAR szPath[MAX_PATH];

    lstrcpy(szPath, psmti->pszSearchOrigin);
    psmti->qwSeen = 0;
    while (BreadthFirstSearch(szPath, BFSCallback, psmti))
    {
        if (PathIsRoot(szPath) || !PathRemoveFileSpec(szPath))
            break;
    }

    // We're all done.
    if (psmti->hDlg)
        PostMessage(psmti->hDlg, WM_COMMAND, IDOK, 0);

    return 0;
}

//----------------------------------------------------------------------------
// Mb used on given drive.
DWORD DriveMbUsed(PSMTIDYINFO psmti)
{
    DWORD dwRet = 0;
    TCHAR szRoot[4];
    DWORD dwSPC, dwBPS, dwFC, dwC;
    QWORD qwSize;
    QWORD qwFree;
    
    lstrcpyn(szRoot, psmti->pszSearchOrigin, ARRAYSIZE(szRoot));
    if (GetDiskFreeSpace(szRoot, &dwSPC, &dwBPS, &dwFC, &dwC)) 
    {
        psmti->dwBPC = dwSPC * dwBPS;
        qwSize = (QWORD)dwC * (QWORD)psmti->dwBPC;
        qwFree = (QWORD)dwFC * (QWORD)psmti->dwBPC;
        qwSize -= qwFree;
        dwRet = (DWORD)(qwSize / BYTES_PER_MBYTE);
    }
    
    return dwRet;
}


//----------------------------------------------------------------------------
// NB We can cheat somewhat here because we only call this for non-removable
// local drives.
// eg pszPath = "c:\foo\bar\fred.exe"
//    pszDriveName = "MyDrive (C:)"
void GetDriveName(LPCTSTR pszPath, LPTSTR pszDriveName, int cchDriveName)
{
    TCHAR szFmt[MAX_PATH];
    TCHAR szLabel[MAX_PATH];
    TCHAR szRoot[4];
    TCHAR szDrive[2];
    LPTSTR args[2];
    
    LoadString(g_hinstApp, IDS_DRIVE_NAME_FMT, szFmt, ARRAYSIZE(szFmt));
    lstrcpyn(szRoot, pszPath, ARRAYSIZE(szRoot));
    GetVolumeInformation(szRoot, szLabel, ARRAYSIZE(szLabel), NULL, NULL, NULL, NULL, 0);
    PathMakePretty(szLabel);
    szDrive[0] = pszPath[0];
    szDrive[1] = TEXT('\0');
    args[0] = szLabel;
    args[1] = szDrive;
    FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY, 
        szFmt, 0, 0, pszDriveName, cchDriveName, (va_list*)args);
}

//----------------------------------------------------------------------------
void LinkFindInit(HWND hDlg, PSMTIDYINFO psmti)
{
    WORD wMax = (WORD)DriveMbUsed(psmti);
    DWORD idThread;

    psmti->hDlg = hDlg;
    SendDlgItemMessage(hDlg, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, wMax));
    psmti->hThread = CreateThread(NULL, 0, LinkFindThreadProc, psmti, 0, &idThread);
    if (!psmti->hThread)
    {
        Dbg(TEXT("lfi:Failed to create search thread."));
        EndDialog(hDlg, IDCANCEL);
    }
    else
    {
        HWND hwndAni = GetDlgItem(hDlg, IDC_STATUS);
        TCHAR szFormat[MAX_PATH];
        TCHAR szDriveName[MAX_PATH];
        TCHAR sz[MAX_PATH+MAX_PATH];
        
        Animate_Open(hwndAni, MAKEINTRESOURCE(IDA_SEARCH));
        Animate_Play(hwndAni, 0, -1, -1);
        GetDlgItemText(hDlg, IDC_LINK_SEARCH_TITLE, szFormat, ARRAYSIZE(szFormat));
        GetDriveName(psmti->pszSearchOrigin, szDriveName, ARRAYSIZE(szDriveName));
        wsprintf(sz, szFormat, szDriveName);
        SetDlgItemText(hDlg, IDC_LINK_SEARCH_TITLE, sz);
    }
}

//----------------------------------------------------------------------------
// Wait for an event but allow sent messages to get through so we don't hang
// anyone up.
DWORD MsgWaitForSingleObject(HANDLE hEvent, DWORD dwTimeout)
{
    BOOL fCont = TRUE;
    DWORD dwObj = WAIT_FAILED;
    
    while (fCont)
    {
        dwObj = MsgWaitForMultipleObjects(1, &hEvent, FALSE, dwTimeout, QS_SENDMESSAGE);
        // Are we done waiting?
        switch (dwObj) 
        {
            MSG msg;
            // Dispatch sent message.
            case WAIT_OBJECT_0 + 1:
                PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
                break;
            default:
                fCont = FALSE;
        }
    }
    return dwObj;    
}

//----------------------------------------------------------------------------
INT_PTR CALLBACK LinkFindDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    PSMTIDYINFO psmti = (PSMTIDYINFO)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (wMsg) 
    {
        case WM_INITDIALOG:
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);
            LinkFindInit(hDlg, (PSMTIDYINFO)lParam);
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) 
            {
                case IDCANCEL:
                    psmti->dwFlags |= SMTIF_STOP_THREAD;     // tell searching thread to stop
                    // Fall through...

                case IDOK:
                    // thread posts this to us
                    Assert(psmti->hThread);
                    // We will attempt to wait up to 5 seconds for the thread to terminate
                    if (MsgWaitForSingleObject(psmti->hThread, 5000) == WAIT_TIMEOUT)
                    {
                        Assert(0);
                        // If this timed out we potentially leaked the list
                        // of paths that we are searching (PATH_NODE list)
                        TerminateThread(psmti->hThread, (DWORD)-1);       // Blow it away!
                    }
                    CloseHandle(psmti->hThread);
                    EndDialog(hDlg, GET_WM_COMMAND_ID(wParam, lParam));
                    break;
            }
            break;
        default:
            return FALSE;
    }
    return TRUE;
}


//----------------------------------------------------------------------------
// in:
//      hwnd            NULL implies NOUI
//      uFlags          IShellLink::Resolve flags parameter
//      pszFolder       place to look
//
//
// in/out:
//      pfd             in: thing we are looking for on input (cFileName unqualified path)
//                      out: if return is TRUE filled in with the new find info
// returns:
//      IDOK            found something
//      IDNO            didn't find it
//      IDCANCEL        user canceled the operation
//
int FindInFolder(HWND hwnd, PSMTIDYINFO psmti)
{
    TCHAR szSearchStart[MAX_PATH];

    Assert(psmti);
    Assert(psmti->pszSearchOrigin);
    
    lstrcpy(szSearchStart, psmti->pszSearchOrigin);
    PathRemoveFileSpec(szSearchStart);
    while (!PathIsDirectory(szSearchStart))
    {
        if (PathIsRoot(szSearchStart) || !PathRemoveFileSpec(szSearchStart))
        {
            Dbg(TEXT("Root path %s does not exists."), szSearchStart);
            return IDNO;
        }
    }

    if (DialogBoxParam(g_hinstApp, MAKEINTRESOURCE(DLG_LINK_SEARCH), hwnd, 
        LinkFindDlgProc, (LPARAM)psmti) != IDOK)
        return IDCANCEL;

    return IDOK;
}
