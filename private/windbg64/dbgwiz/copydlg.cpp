#include "precomp.hxx"
#pragma hdrstop



typedef
class LIST_OF_FILES_IN_DIR {
public:
    char m_szDirPath[_MAX_PATH * 2];
    TList<PSTR> m_listFileNames;

    LIST_OF_FILES_IN_DIR() { ZeroMemory(m_szDirPath, sizeof(m_szDirPath)); }
    ~LIST_OF_FILES_IN_DIR();

} * PLIST_OF_FILES_IN_DIR;

LIST_OF_FILES_IN_DIR::
~LIST_OF_FILES_IN_DIR()
{
    while (!m_listFileNames.IsEmpty()) {
        free( m_listFileNames.GetHeadData() );
        m_listFileNames.RemoveHead();
    }
}


// The friggin error handling bloated these functions

BOOL
GetListOfFiles(
    long & lStopCopying,
    PSTR pszPath, 
    TList<PLIST_OF_FILES_IN_DIR> & ListOfDirectories,
    DWORD & dwTotalFilesToCopy
    )
{
    BOOL bRet = TRUE;
    char szTmpCurPath[_MAX_PATH * 2];   

    PLIST_OF_FILES_IN_DIR pListOfFiles = new LIST_OF_FILES_IN_DIR;
    Assert(pListOfFiles);
    
    ListOfDirectories.InsertTail(pListOfFiles);

    //
    // Record current directory path
    //
    strcpy(pListOfFiles->m_szDirPath, pszPath);
    strcat(pListOfFiles->m_szDirPath, "\\");
    Assert(strlen(pListOfFiles->m_szDirPath) < sizeof(pListOfFiles->m_szDirPath));

    //
    // Build search path
    //
    strcpy(szTmpCurPath, pListOfFiles->m_szDirPath);
    strcat(szTmpCurPath, "*");
    Assert(strlen(szTmpCurPath) < sizeof(szTmpCurPath));


    WIN32_FIND_DATA wfd;
    HANDLE hFindFile = FindFirstFile(szTmpCurPath, &wfd);
    BOOL bContinue = (INVALID_HANDLE_VALUE != hFindFile);
    while (bContinue) {
        if (InterlockedExchangeAdd(&lStopCopying, 0)) {
            // user pressed cancel, stop searching
            bRet = FALSE;
            goto EXIT;
        }

        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (strcmp(".", wfd.cFileName) && strcmp("..", wfd.cFileName)) {
                    
                //
                // We found a new dir, build the path and recurse into it.
                //
                strcpy(szTmpCurPath, pListOfFiles->m_szDirPath);
                strcat(szTmpCurPath, wfd.cFileName);
                Assert(strlen(szTmpCurPath) < sizeof(szTmpCurPath));
                
                if (!GetListOfFiles(lStopCopying, szTmpCurPath, ListOfDirectories, dwTotalFilesToCopy)) {
                    // user pressed cancel, stop searching
                    bRet = FALSE;
                    goto EXIT;
                }
            }
        } else {
            //
            // Found a file, inc the counter
            //
            dwTotalFilesToCopy++;

            PSTR pszNewFile = NT4SafeStrDup(wfd.cFileName);
            Assert(pszNewFile);
            pListOfFiles->m_listFileNames.InsertTail(pszNewFile);
        }
        bContinue = FindNextFile(hFindFile, &wfd);
    }
    
EXIT:
    if (INVALID_HANDLE_VALUE != hFindFile) {
        Assert(FindClose(hFindFile));   
    }

    return bRet;
}

BOOL
DecompressCopyFile(
    HWND hwndDlg,
    PSTR pszDestPath, 
    PSTR pszSrcPath, 
    PSTR pszSrcFile,
    BOOL & bWarnAboutUnknownFiles
    )
{
    //
    // Let's figure out the destination name
    //
    const int nNumSyms = 3;
    PSTR rgpszSymExtensions[nNumSyms][2] = {
        { ".PDB", ".PD_" },
        { ".DBG", ".DB_" },
        { ".SYM", ".SY_" }
    };

    // Get the file name and extensions
    char szName[_MAX_FNAME];
    char szExt[_MAX_EXT];
    _splitpath(pszSrcFile, NULL, NULL, szName, szExt);

    // Figure out the expanded extension
    for (int i=0; i<nNumSyms; i++) {
        if (!_stricmp(szExt, rgpszSymExtensions[i][0])) {
            // The extension is not compressed
            goto FILE_NAME_FOUND;
        } else if (!_stricmp(szExt, rgpszSymExtensions[i][1])) {
            // Expand the extension 
            strcpy(szExt, rgpszSymExtensions[i][0]);
            goto FILE_NAME_FOUND;
        }
    }

    //
    // The extension name was not found, should we warn the user
    //
    if (bWarnAboutUnknownFiles) {
        PSTR pszErr = WKSP_DynaLoadStringWithArgs(g_hInst, IDS_WARN_NOT_A_SYMBOL, pszSrcFile);
        PSTR pszWarning = WKSP_DynaLoadString(g_hInst, IDS_WARNING);

        int nRes = MessageBox(NULL, pszErr, pszWarning, MB_TASKMODAL | MB_YESNOCANCEL | MB_ICONEXCLAMATION);
        if (IDCANCEL == nRes) {
            return FALSE;
        } else if (IDNO == nRes) {
            bWarnAboutUnknownFiles = FALSE;
        }

        free(pszWarning);
        free(pszErr);
    }

    //
    // Build the src and dest paths
    //
FILE_NAME_FOUND:

    BOOL bResult = TRUE;

    char szSrc[_MAX_PATH * 2];
    strcpy(szSrc, pszSrcPath);
    strcat(szSrc, pszSrcFile);
    Assert(strlen(szSrc) < sizeof(szSrc));

    char szDest[_MAX_PATH * 2];
    strcpy(szDest, pszDestPath);
    strcat(szDest, szName);
    strcat(szDest, szExt);
    Assert(strlen(szDest) < sizeof(szDest));


    //
    // Decompress copy the file
    //
RETRY_COPY:
    if (ERROR_SUCCESS != SetupDecompressOrCopyFile(szSrc, szDest, NULL)) {       
        PSTR pszErr = WKSP_DynaLoadStringWithArgs(g_hInst, IDS_ERR_COPYING_SYMBOL, szSrc, szDest);
        Assert(pszErr);

        int nRes = MessageBox(NULL, pszErr, NULL, MB_TASKMODAL | MB_ICONSTOP | MB_ABORTRETRYIGNORE);
        if (IDABORT == nRes) {
            bResult = FALSE;
            goto EXIT;
        } else if (IDRETRY == nRes) {
            goto RETRY_COPY;
        } else if (IDIGNORE == nRes) {
            bResult = TRUE;
        }
    }

EXIT:
    return bResult;
}

BOOL
CopyDirectory(
    long & lStopCopying,
    HWND hwndDlg,
    HWND hwndProgress,
    HWND hwndCurFile,
    DWORD & dwFilesCopied,
    const DWORD dwTotalFilesToCopy,
    PSTR pszOrigDestPath,
    PSTR pszOrigSrcPath,
    LIST_OF_FILES_IN_DIR & ListOfFiles,
    BOOL & bWarnAboutUnknownFiles
    )
{
    Assert(hwndDlg);
    Assert(hwndProgress);
    Assert(hwndCurFile);

    Assert(!_strnicmp(ListOfFiles.m_szDirPath, pszOrigSrcPath, strlen(pszOrigSrcPath)));

    // Point 
    PSTR psz = ListOfFiles.m_szDirPath + strlen(pszOrigSrcPath);

    char szDestPath[_MAX_PATH * 2] = {0};
    strcpy(szDestPath, pszOrigDestPath);
    strcat(szDestPath, psz);
    Assert(strlen(szDestPath) < sizeof(szDestPath));

RETRY_DIR_CREATION:
    if (!CreateDirIfNotExist(szDestPath)) {
        int nRes = WKSP_CustMsgBox(MB_ICONHAND | MB_TASKMODAL | MB_ABORTRETRYIGNORE, 
            NULL, IDS_ERROR_CANT_CREATE_DIR, szDestPath);
        if (IDABORT == nRes) {
            return FALSE;
        } else if (IDRETRY == nRes) {
            goto RETRY_DIR_CREATION;
        } else if (IDIGNORE == nRes) {
            // was copied but, we need to increment the progress bar
            dwFilesCopied += ListOfFiles.m_listFileNames.Size();
            return TRUE;
        } else {
            Assert(0);
        }
    }

    while (!ListOfFiles.m_listFileNames.IsEmpty()) {
        // Don't modify it, just test it
        if (InterlockedExchangeAdd(&lStopCopying, 0)) {
            // user pressed cancel
            // stop copying
            return FALSE;
        }

        // Give UI feedback
        PSTR pszFileName = ListOfFiles.m_listFileNames.GetHeadData();
        ListOfFiles.m_listFileNames.RemoveHead();
        {
            char sz[_MAX_PATH * 2];
            strcpy(sz, ListOfFiles.m_szDirPath);
            strcat(sz, pszFileName);
            Assert(strlen(sz) < sizeof(sz));

            SendMessage(hwndCurFile, WM_SETTEXT, 0, (LPARAM) sz);
        }
        
        // Actually copy
        if (!DecompressCopyFile(hwndDlg, szDestPath, ListOfFiles.m_szDirPath, pszFileName, bWarnAboutUnknownFiles)) {
            // Abort copying
            return FALSE;
        }
        
        dwFilesCopied++;

        PostMessage(hwndProgress, PBM_SETPOS, USHRT_MAX * dwFilesCopied / dwTotalFilesToCopy, 0);

        free(pszFileName);
    }

    // Continue copying
    return TRUE;
}

DWORD 
WINAPI 
CopySymbols(
    PVOID pvParam
    )
{
    BOOL bWarnAboutUnknownFiles = TRUE;
    DWORD dwFilesCopied = 0;

    Assert(pvParam);

    PCOPY_SYMBOLS_DATA_STRUCT pCopySymsData = (PCOPY_SYMBOLS_DATA_STRUCT) pvParam;

    Assert(pCopySymsData->m_hwndDlgParent);

    {
        PSTR psz = pCopySymsData->m_szSrcPath + strlen(pCopySymsData->m_szSrcPath) -1;
        if ('\\' == *psz) {
            *psz = NULL;
        }

        psz = pCopySymsData->m_szDestPath + strlen(pCopySymsData->m_szDestPath) -1;
        if ('\\' == *psz) {
            *psz = NULL;
        }
    }


    HWND hwndProgress = GetDlgItem(pCopySymsData->m_hwndDlgParent, IDC_PROGRESS);
    Assert(hwndProgress);
    HWND hwndCurFile = GetDlgItem(pCopySymsData->m_hwndDlgParent, IDC_STATIC_CURRENT_FILE);
    Assert(hwndCurFile);


    SendMessage(hwndProgress, PBM_SETPOS, 0, 0);
    SendMessage(hwndProgress, PBM_SETRANGE, 0, MAKELPARAM(0, USHRT_MAX));
    
    // Get the list of files, and the total size in files to copy.
    TList<PLIST_OF_FILES_IN_DIR> ListOfDirectories;
    DWORD dwTotalFilesToCopy = 0;
    if (!GetListOfFiles(pCopySymsData->m_lStopCopying, pCopySymsData->m_szSrcPath, 
        ListOfDirectories, dwTotalFilesToCopy)) {

        // User canceled
        goto EXIT;
    }

    // Did we find any files to copy
    if (0 == dwTotalFilesToCopy) {
        WKSP_MsgBox(NULL, IDS_ERROR_NO_FILES_FOUND);
        // simulate the user pressing cancel
        InterlockedExchange(&pCopySymsData->m_lStopCopying, TRUE);
        goto EXIT;
    }
            
RETRY_DIR_CREATION:
    if (!CreateDirTree(pCopySymsData->m_szDestPath)) {
        int nRes = WKSP_CustMsgBox(MB_ICONHAND | MB_TASKMODAL | MB_RETRYCANCEL, 
            NULL, IDS_ERROR_RETRYCANCEL_CANT_CREATE_DIR, pCopySymsData->m_szDestPath);
        if (IDRETRY == nRes) {
            goto RETRY_DIR_CREATION;
        } else if (IDCANCEL == nRes) {
            goto EXIT;
        } else {
            Assert(0);
        }
    }
    
    // If the symbols are being copied from \\ntstress assume that are files 
    //  copied are correct. Do not warn about any of the files.
    {
        PSTR psz = "\\\\ntstress";
        if (!_strnicmp(pCopySymsData->m_szSrcPath, psz, strlen(psz))) {
            bWarnAboutUnknownFiles = FALSE;
        } else {
            bWarnAboutUnknownFiles = TRUE;
        }
    }

    dwFilesCopied = 0;
    while (!ListOfDirectories.IsEmpty()) {
        PLIST_OF_FILES_IN_DIR pListOfDirectories = ListOfDirectories.GetHeadData();
        ListOfDirectories.RemoveHead();
        Assert(pListOfDirectories);
        
        if (!CopyDirectory(pCopySymsData->m_lStopCopying, pCopySymsData->m_hwndDlgParent, hwndProgress, 
            hwndCurFile, dwFilesCopied, dwTotalFilesToCopy, 
            pCopySymsData->m_szDestPath, pCopySymsData->m_szSrcPath, 
            *pListOfDirectories, bWarnAboutUnknownFiles)) {
            
            // Copying aborted
            delete pListOfDirectories;
            goto EXIT;
        }
        
        delete pListOfDirectories;
    }

EXIT:
    if (InterlockedExchangeAdd(&pCopySymsData->m_lStopCopying, 0)) {
        PostMessage(pCopySymsData->m_hwndDlgParent, UM_FINISHED_ENDDIALOG, 0, IDCANCEL);
    } else {
        PostMessage(pCopySymsData->m_hwndDlgParent, UM_FINISHED_ENDDIALOG, 0, IDOK);
    }
    return 0;
}




INT_PTR
CALLBACK
Copying_DlgProc(
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam 
    )
{
    PCOPY_SYMBOLS_DATA_STRUCT pCopySymsData;

    pCopySymsData = (PCOPY_SYMBOLS_DATA_STRUCT) GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg) {
    case WM_DESTROY:
        Assert(pCopySymsData);
        ZeroMemory(pCopySymsData, sizeof(COPY_SYMBOLS_DATA_STRUCT));
        pCopySymsData = NULL;
        return FALSE;

    case WM_INITDIALOG:
        pCopySymsData = (PCOPY_SYMBOLS_DATA_STRUCT) lParam;
        Assert(pCopySymsData);

        pCopySymsData->m_hwndDlgParent = hDlg;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pCopySymsData);       
        PostMessage(hDlg, UM_STARTCOPYING, 0, 0);
        return TRUE;

    case UM_STARTCOPYING:
        {
            // TODO: Add your control notification handler code here
            DWORD dwThreadId;
            HANDLE hThread = CreateThread( 
                NULL,                        // no security attributes 
                0,                           // use default stack size  
                CopySymbols,                 // thread function 
                pCopySymsData,               // argument to thread function 
                0,                           // use default creation flags 
                &dwThreadId);                // returns the thread identifier  
            
            // Check the return value for success.     
            Assert(hThread);
            
            CloseHandle(hThread);
            
            return TRUE;
        }

    case UM_FINISHED_ENDDIALOG:
        EndDialog(hDlg, lParam);
        return FALSE;

    case WM_COMMAND:
        {
            WORD wNotifyCode = HIWORD(wParam); // notification code 
            WORD wID = LOWORD(wParam);         // item, control, or accelerator identifier 
            HWND hwndCtl = (HWND) lParam;      // handle of control 

            if (hwndCtl) {
                switch (wID) {
                case IDCANCEL:
                    InterlockedExchange(&pCopySymsData->m_lStopCopying, TRUE);
                    
                    // Give the user feedback that cancel has been pressed,
                    // because the user may think that something is wrong since it doesn't
                    // stop immediately but waits until the current file copy has finished.
                    EnableWindow(hwndCtl, FALSE);

                    PSTR psz = WKSP_DynaLoadString(g_hInst, IDS_STOPPING);
                    SetWindowText(hwndCtl, psz);
                    free(psz);
                    return TRUE;
                }
            }
        }
        return FALSE;
    }

    return FALSE;
}

  






