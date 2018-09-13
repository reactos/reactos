//*************************************************************
//
//  Functions to copy the profile directory
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include "uenv.h"


//
// Local function proto-types
//

BOOL RecurseDirectory (LPTSTR lpSrcDir, LPTSTR lpDestDir, DWORD dwFlags,
                       LPFILEINFO *llSrcDirs, LPFILEINFO *llSrcFiles,
                       BOOL bSkipNtUser, LPTSTR lpExcludeList, BOOL bRecurseDest);
BOOL AddFileInfoNode (LPFILEINFO *lpFileInfo, LPTSTR lpSrcFile,
                      LPTSTR lpDestFile, LPFILETIME ftLastWrite, LPFILETIME ftCreate,
                      DWORD dwFileSize, DWORD dwFileAttribs);
BOOL FreeFileInfoList (LPFILEINFO lpFileInfo);
BOOL SyncItems (LPFILEINFO lpSrcItems, LPFILEINFO lpDestItems,
                BOOL bFile, LPFILETIME ftDelRefTime);
void CopyFileFunc (LPTHREADINFO lpThreadInfo);
void CopyStatusThread (LPTHREADINFO lpThreadInfo);
LPTSTR ConvertExclusionList (LPCTSTR lpSourceDir, LPCTSTR lpExclusionList);
DWORD FindDirectorySize(LPTSTR lpDir, LPFILEINFO lpFiles, DWORD dwFlags);
BOOL ReconcileDirectory(LPTSTR lpSrcDir, LPTSTR lpDestDir, 
                        DWORD dwFlags, DWORD dwSrcAttribs);

//*************************************************************
//
//  CopyProfileDirectoryEx()
//
//  Purpose:    Copies the profile directory from the source
//              to the destination
//
//
//  Parameters: LPCTSTR     lpSourceDir     -  Source directory
//              LPCTSTR     lpDestDir       -  Destination directory
//              DWORD       dwFlags         -  Flags
//              LPFILETIME  ftDelRefTime    -  Delete file reference time
//              LPCTSTR     lpExclusionList -  List of directories to exclude
//
//
//  Return:     (BOOL) TRUE if successful
//                     FALSE if an error occurs
//
//
//  Comments:
//
//
//  History:    Date        Author     Comment
//              5/24/95     ericflo    Created
//              4/09/98     ericflo    Converted to CopyProfileDirectoryEx
//              9/28/98     ushaji     Modified to check for free space
//
//*************************************************************

BOOL CopyProfileDirectoryEx (LPCTSTR lpSourceDir, LPCTSTR lpDestinationDir,
                             DWORD dwFlags, LPFILETIME ftDelRefTime,
                             LPCTSTR lpExclusionList)
{
    LPTSTR lpSrcDir = NULL, lpDestDir = NULL;
    LPTSTR lpSrcEnd, lpDestEnd;
    LPTSTR lpExcludeListSrc = NULL;
    LPTSTR lpExcludeListDest = NULL;
    LPFILEINFO lpSrcFiles = NULL, lpDestFiles = NULL;
    LPFILEINFO lpSrcDirs = NULL, lpDestDirs = NULL;
    LPFILEINFO lpTemp;
    THREADINFO ThreadInfo = {0, NULL, NULL, 0, NULL, NULL, NULL, NULL};
    DWORD dwThreadId;
    HANDLE hThreads[NUM_COPY_THREADS];
    HANDLE hStatusThread = 0;
    DWORD dwThreadCount = 0;
    HANDLE hFile;
    WIN32_FIND_DATA fd;
    BOOL bResult = FALSE;
    BOOL bSynchronize = FALSE;
    UINT i;
    DWORD dwTotalSrcFiles = 0;
    DWORD dwTotalDestFiles = 0;
    ULARGE_INTEGER ulFreeBytesAvailableToCaller, ulTotalNumberOfBytes, ulTotalNumberOfFreeBytes;   
    DWORD dwErr, dwErr1=0;
    TCHAR szErr[MAX_PATH];

    dwErr = GetLastError();

    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("CopyProfileDirectoryEx: Entering, lpSourceDir = <%s>, lpDestinationDir = <%s>, dwFlags = 0x%x"),
             lpSourceDir, lpDestinationDir, dwFlags));


    //
    // Validate parameters
    //

    if (!lpSourceDir || !lpDestinationDir) {
        DebugMsg((DM_WARNING, TEXT("CopyProfileDirectoryEx: received NULL pointer")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    //
    // If there is an exclusion list, convert it into an array of null
    // terminate strings (double null at the end) based upon the source
    // directory
    //

    if ((dwFlags & CPD_USEEXCLUSIONLIST) && lpExclusionList) {

        DebugMsg((DM_VERBOSE, TEXT("CopyProfileDirectoryEx: lpExclusionList = <%s>"),
                 lpExclusionList));

        lpExcludeListSrc = ConvertExclusionList (lpSourceDir, lpExclusionList);

        if (!lpExcludeListSrc) {
            DebugMsg((DM_WARNING, TEXT("CopyProfileDirectoryEx:  Failed to convert exclusion list for source")));
            goto Exit;
        }

        if (!(dwFlags & CPD_DELDESTEXCLUSIONS)) {
            lpExcludeListDest = ConvertExclusionList (lpDestinationDir, lpExclusionList);

            if (!lpExcludeListDest) {
                DebugMsg((DM_WARNING, TEXT("CopyProfileDirectoryEx:  Failed to convert exclusion list for destination")));
                dwErr = ERROR_INVALID_DATA;
                goto Exit;
            }
        }
    }


    //
    // Get the desktop handle
    //

    ThreadInfo.hDesktop = GetThreadDesktop(GetCurrentThreadId());

    //
    // Is this a full sync copy (delete extra files / directories in dest).
    //

    if (dwFlags & CPD_SYNCHRONIZE) {
        bSynchronize = TRUE;
    }


    //
    // Test / Create the destination directory
    //

    if (!CreateNestedDirectory(lpDestinationDir, NULL)) {
        DebugMsg((DM_WARNING, TEXT("CopyProfileDirectoryEx: Failed to create the destination directory.  Error = %d"),
                 GetLastError()));
        dwErr = GetLastError();
        goto Exit;
    }


    //
    // Create and set up the directory buffers
    //

    lpSrcDir = LocalAlloc(LPTR, (2 * MAX_PATH) * sizeof(TCHAR));
    lpDestDir = LocalAlloc(LPTR, (2 * MAX_PATH) * sizeof(TCHAR));

    if (!lpSrcDir || !lpDestDir) {
        DebugMsg((DM_WARNING, TEXT("CopyProfileDirectoryEx: Failed to allocate memory for working directories")));
        dwErr = GetLastError();
        goto Exit;
    }


    lstrcpy (lpSrcDir, lpSourceDir);
    lstrcpy (lpDestDir, lpDestinationDir);


    //
    // Setup ending pointers
    //

    lpSrcEnd = CheckSlash (lpSrcDir);
    lpDestEnd = CheckSlash (lpDestDir);

    //
    // Recurse through the folders gathering info
    //

    if (!(dwFlags & CPD_COPYHIVEONLY)) {


        //
        // Recurse the source directory
        //

        if (!RecurseDirectory(lpSrcDir, lpDestDir, dwFlags,
                              &lpSrcDirs, &lpSrcFiles, TRUE, lpExcludeListSrc, FALSE)) {
            DebugMsg((DM_VERBOSE, TEXT("CopyProfileDirectoryEx: RecurseDirectory returned FALSE")));
            dwErr = GetLastError();
            goto Exit;
        }


        if (bSynchronize) {

            //
            // Recurse the destination directory
            //

            if (!RecurseDirectory(lpDestDir, lpSrcDir, dwFlags,
                                  &lpDestDirs, &lpDestFiles, TRUE, lpExcludeListDest, TRUE)) {
                DebugMsg((DM_VERBOSE, TEXT("CopyProfileDirectoryEx: RecurseDirectory returned FALSE")));
                dwErr = GetLastError();
                goto Exit;
            }
        }
    }

    // 
    // determine the sizes
    //

    dwTotalSrcFiles = FindDirectorySize(lpSrcDir, lpSrcFiles, dwFlags);
    dwTotalDestFiles = FindDirectorySize(lpDestDir, lpDestFiles, dwFlags);

    //
    // CopyProfileDirectoryEx is called with impersonation on 
    //

    if (!GetDiskFreeSpaceEx(lpDestDir,  &ulFreeBytesAvailableToCaller, &ulTotalNumberOfBytes, &ulTotalNumberOfFreeBytes)) {
        DebugMsg((DM_WARNING, TEXT("CopyProfileDirectoryEx: Failed to get the Free Disk Space <%s>.  Error = %d"),
                         lpDestDir, GetLastError()));
        dwErr = GetLastError();
        goto Exit;
    }

    // 
    // Check if we have enough disk space, expecting twice the space for safety
    //

    if ((2*((ULONGLONG)dwTotalSrcFiles) > ((ULONGLONG)dwTotalDestFiles)) &&
        ((ulFreeBytesAvailableToCaller.QuadPart) < (2*((ULONGLONG)dwTotalSrcFiles) - ((ULONGLONG)dwTotalDestFiles)))) {

        dwErr = ERROR_DISK_FULL;
        goto Exit;
    }


    
    //
    // If we are showing the status dialog
    //

    if (dwFlags & CPD_SHOWSTATUS) {

        //
        // Create status dialog
        //

        ThreadInfo.dwFlags = dwFlags;

        ThreadInfo.hStatusInitEvent = CreateEvent (NULL, TRUE, FALSE, NULL);

        ThreadInfo.hStatusTermEvent = CreateEvent (NULL, TRUE, FALSE, NULL);


        if (ThreadInfo.hStatusInitEvent && ThreadInfo.hStatusTermEvent) {

            hStatusThread = CreateThread (NULL,
                                          0,
                                          (LPTHREAD_START_ROUTINE) CopyStatusThread,
                                          (LPVOID) &ThreadInfo,
                                          0,
                                          &dwThreadId);
            if (hStatusThread) {
                WaitForSingleObject (ThreadInfo.hStatusInitEvent, INFINITE);
            }

            SendDlgItemMessage (ThreadInfo.hStatusDlg, IDC_PROGRESS, PBM_SETRANGE32,
                                0, (LPARAM) dwTotalSrcFiles);
        }
    }


    //
    // Copy the actual hive, log, ini files first
    //

    if (!(dwFlags & CPD_IGNOREHIVE)) {


        //
        // Search for all user hives
        //

        if (dwFlags & CPD_WIN95HIVE) {

            lstrcpy (lpSrcEnd, c_szUserStar);

        } else {

            lstrcpy (lpSrcEnd, c_szNTUserStar);

        }


        //
        // Enumerate
        //

        hFile = FindFirstFile(lpSrcDir, &fd);

        if (hFile != INVALID_HANDLE_VALUE) {

            do  {

                //
                // Setup the filename
                //

                lstrcpy (lpSrcEnd, fd.cFileName);
                lstrcpy (lpDestEnd, fd.cFileName);


                //
                // Copy the hive
                //

                if (!ReconcileFile(lpSrcDir, lpDestDir, dwFlags, NULL,
                                               ThreadInfo.hStatusDlg, fd.nFileSizeLow)) {

                    DebugMsg((DM_WARNING, TEXT("CopyProfileDirectoryEx: ReconcileFile failed with error = %d"), GetLastError()));

                    if (!(dwFlags & CPD_IGNORECOPYERRORS)) {

                        dwErr = GetLastError();
                        ReportError((ThreadInfo.hStatusDlg != NULL) ? 0 : PI_NOUI, IDS_COPYERROR, lpSrcDir, lpDestDir, GetErrString(dwErr, szErr));

                        FindClose(hFile);
                        goto Exit;
                    }
                }


            //
            // Find the next entry
            //

            } while (FindNextFile(hFile, &fd));

            FindClose(hFile);

        } else {
            dwErr = GetLastError();
            DebugMsg((DM_VERBOSE, TEXT("CopyProfileDirectoryEx: FindFirstFile failed to find a hive!.  Error = %d"),
                     GetLastError()));
        }
    }


    //
    //  Create all the directories
    //

    if (!(dwFlags & CPD_COPYHIVEONLY)) {

        DebugMsg((DM_VERBOSE, TEXT("CopyProfileDirectoryEx: Calling ReconcileDirectory for all Directories")));

        lpTemp = lpSrcDirs;

        while (lpTemp) {

            if (!ReconcileDirectory(lpTemp->szSrc, lpTemp->szDest, dwFlags, lpTemp->dwFileAttribs)) {
                dwErr1 = GetLastError();

                DebugMsg((DM_WARNING, TEXT("CopyProfileDirectoryEx: Failed to create the destination directory <%s>.  Error = %d"),
                         lpTemp->szDest, GetLastError()));

                //
                // Show the error UI.
                //

                ReportError(((dwFlags & CPD_NOERRORUI)? PI_NOUI:0) , IDS_COPYERROR, 
                            lpTemp->szSrc, lpTemp->szDest, GetErrString(GetLastError(), szErr));

                dwErr = dwErr1;
                goto Exit;
            }

            lpTemp = lpTemp->pNext;
        }

        DebugMsg((DM_VERBOSE, TEXT("CopyProfileDirectoryEx: Reconcile Directory Done for all Directories")));

        //
        // Copy the files
        //

        if (dwFlags & CPD_SLOWCOPY) {

            //
            // Copy the files one at a time...
            //

            lpTemp = lpSrcFiles;

            while (lpTemp) {

                if (!ReconcileFile (lpTemp->szSrc, lpTemp->szDest, dwFlags,
                                    &lpTemp->ftLastWrite, ThreadInfo.hStatusDlg,
                                    lpTemp->dwFileSize)) {

                    dwErr1 = GetLastError();
                    DebugMsg((DM_WARNING, TEXT("CopyProfileDirectoryEx: Failed to copy the file <%s> to <%s> due to error = %d"),
                             lpTemp->szSrc, lpTemp->szDest, GetLastError()));

                    //
                    // If there is no UI, or the user picks to abort
                    // then we leave now.
                    //

                    ReportError(((dwFlags & CPD_NOERRORUI)? PI_NOUI:0) , IDS_COPYERROR, 
                                lpTemp->szSrc, lpTemp->szDest, GetErrString(GetLastError(), szErr));

                    dwErr = dwErr1;
                    goto Exit;
                }

                lpTemp = lpTemp->pNext;
            }

        } else {

            if (lpSrcFiles) {

                HANDLE hThreadToken=NULL;


                if (!OpenThreadToken (GetCurrentThread(), TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_DUPLICATE,
                                 TRUE, &hThreadToken)) {
                    DebugMsg((DM_VERBOSE, TEXT("CopyProfileDirectoryEx: Failed to get token with %d. This is ok if thread is not impersonating"),
                             GetLastError()));
                }


                //
                // Multi-threaded copy
                //

                // Null sd, auto set, initially signalled, unnamed..

                if (!(ThreadInfo.hCopyEvent = CreateEvent(NULL, FALSE, TRUE, NULL))) {
                    DebugMsg((DM_WARNING, TEXT("CopyProfileDirectoryEx: CreateEvent for CopyEvent failed with error %d"), GetLastError()));
                    dwErr = GetLastError();
                    goto Exit;
                }

                ThreadInfo.dwFlags = dwFlags;
                ThreadInfo.lpSrcFiles = lpSrcFiles;

                //
                // Required for PrivCopyFileEx to work, threads should be created using the
                // process token
                //
                
                RevertToSelf();

                //
                // Create the file copy threads
                //

                for (i = 0; i < NUM_COPY_THREADS; i++) {

                    if (hThreads[dwThreadCount] = CreateThread (NULL,
                                                    0,
                                                    (LPTHREAD_START_ROUTINE) CopyFileFunc,
                                                    (LPVOID) &ThreadInfo,
                                                    CREATE_SUSPENDED,
                                                    &dwThreadId)) {
                        dwThreadCount++;
                    }
                }


                //
                // Put the token back.
                //

                if (!SetThreadToken(NULL, hThreadToken)) {
                    DebugMsg((DM_WARNING, TEXT("CopyProfileDirectoryEx: Impersonation failed with error %d"), GetLastError()));
                    dwErr = GetLastError();       

                    // terminate and close handles for all the threads 
                    for (i = 0; i < dwThreadCount; i++) {
                        TerminateThread (hThreads[i], 1);
                        CloseHandle (hThreads[i]);                        
                    }

                    if (hThreadToken)
                        CloseHandle (hThreadToken);

                    goto Exit;
                }


                for (i = 0; i < dwThreadCount; i++) {
                    SetThreadToken(&hThreads[i], hThreadToken);
                    ResumeThread (hThreads[i]);
                }

                //
                // Wait for the threads to finish
                //

                if (WaitForMultipleObjects (dwThreadCount, hThreads, TRUE, INFINITE) == WAIT_FAILED) {
                    for (i = 0; i < dwThreadCount; i++) {
                        TerminateThread (hThreads[i], 1);
                    }
                }

                //
                // Clean up
                //

                if (hThreadToken)
                    CloseHandle (hThreadToken);

                for (i = 0; i < dwThreadCount; i++) {
                    CloseHandle (hThreads[i]);
                }


                if (ThreadInfo.dwError) {
                    dwErr = ThreadInfo.dwError;
                    goto Exit;
                }
            }
        }
    }


    //
    // Synchronize the directories and files if appropriate
    //

    if (bSynchronize) {

        //
        //  Files first...
        //

        SyncItems (lpSrcFiles, lpDestFiles, TRUE,
                   (dwFlags & CPD_USEDELREFTIME) ? ftDelRefTime : NULL);

        //
        //  Now the directories...
        //

        SyncItems (lpSrcDirs, lpDestDirs, FALSE,
                   (dwFlags & CPD_USEDELREFTIME) ? ftDelRefTime : NULL);
    }


    //
    // Restore the time on the directories to be the same as from Src.
    // This is required because the times on directory have been modified by 
    // creation and deletion of files above.
    //

    DebugMsg((DM_VERBOSE, TEXT("CopyProfileDirectoryEx: Setting Directory TimeStamps all Directories")));

    lpTemp = lpSrcDirs;

    while (lpTemp) {

        HANDLE hFile;

        SetFileAttributes (lpTemp->szDest, FILE_ATTRIBUTE_NORMAL);

        hFile = CreateFile(lpTemp->szDest, GENERIC_WRITE,
                            FILE_SHARE_DELETE | FILE_SHARE_WRITE,
                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, NULL);

        if (hFile != INVALID_HANDLE_VALUE) {
            if (!SetFileTime(hFile, NULL, NULL, &(lpTemp->ftLastWrite))) {
                DebugMsg((DM_WARNING, TEXT("CopyProfileDirectoryEx: Failed to set the write time on the directory <%s>.  Error = %d"),
                             lpTemp->szDest, GetLastError()));
            }

            CloseHandle(hFile);
        }
        else {
            DebugMsg((DM_WARNING, TEXT("CopyProfileDirectoryEx: CreateFile failed for directory %s with error %d"), lpTemp->szSrc, 
                                      GetLastError()));
        }

        SetFileAttributes (lpTemp->szDest, lpTemp->dwFileAttribs);

        lpTemp = lpTemp->pNext;
    }

    DebugMsg((DM_VERBOSE, TEXT("CopyProfileDirectoryEx: Set times on all directories")));


    //
    // Finish the progress control
    //

    if (dwFlags & CPD_SHOWSTATUS) {
        SendDlgItemMessage (ThreadInfo.hStatusDlg, IDC_PROGRESS, PBM_SETPOS,
                            (WPARAM) dwTotalSrcFiles, 0);
    }


    //
    // Success
    //

    bResult = TRUE;


Exit:

    //
    // Destroy the status dialog
    //

    if (ThreadInfo.hStatusTermEvent) {

        SetEvent(ThreadInfo.hStatusTermEvent);

        if (hStatusThread) {

            if (ThreadInfo.hStatusDlg) {
                WaitForSingleObject (hStatusThread, 10000);
            }

            CloseHandle (hStatusThread);
        }

        CloseHandle (ThreadInfo.hStatusTermEvent);
    }

    if (ThreadInfo.hStatusInitEvent) {
        CloseHandle (ThreadInfo.hStatusInitEvent);
    }

    if (ThreadInfo.hCopyEvent) {
        CloseHandle (ThreadInfo.hCopyEvent);
    }

    if ( ThreadInfo.hDesktop ) {
        CloseDesktop( ThreadInfo.hDesktop );
    }

    //
    // Free the memory allocated above
    //

    if (lpSrcDir) {
        LocalFree(lpSrcDir);
    }

    if (lpDestDir) {
        LocalFree(lpDestDir);
    }

    if (lpExcludeListSrc) {
        LocalFree (lpExcludeListSrc);
    }

    if (lpExcludeListDest) {
        LocalFree (lpExcludeListDest);
    }

    if (lpSrcFiles) {
        FreeFileInfoList(lpSrcFiles);
    }

    if (lpDestFiles) {
        FreeFileInfoList(lpDestFiles);
    }

    if (lpSrcDirs) {
        FreeFileInfoList(lpSrcDirs);
    }

    if (lpDestDirs) {
        FreeFileInfoList(lpDestDirs);
    }

    SetLastError(dwErr);

    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("CopyProfileDirectoryEx: Leaving with a return value of %d"), bResult));

    return bResult;
}

//*************************************************************
//
//  RecurseDirectory()
//
//  Purpose:    Recurses through the subdirectory coping files.
//
//  Parameters: lpSrcDir      -   Source directory working buffer
//              lpDestDir     -   Destination directory working buffer
//              dwFlags       -   dwFlags
//              llSrcDirs     -   Link list of directories
//              llSrcFiles    -   Link list of files
//              bSkipNtUser   -   Skip ntuser.* files
//              lpExcludeList -   List of directories to exclude
//              bRecurseDest  -   The destination Dir is being recursed
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:   1)  The source and dest directories will already have
//                  the trailing backslash when entering this function.
//              2)  The current working directory is the source directory.
//
//
// Notes:
//      CPD_SYSTEMDIRSONLY  Do not keep track of anything unless the dir is
//                          marked with system bit.
//      CPD_SYSTEMFILES     Only Systemfiles
//      CPD_NONENCRYPTEDONLY Only Non EncryptedFile/Directory.
//
//  History:    Date        Author     Comment
//              5/25/95     ericflo    Created
//
//*************************************************************

BOOL RecurseDirectory (LPTSTR lpSrcDir, LPTSTR lpDestDir, DWORD dwFlags,
                       LPFILEINFO *llSrcDirs, LPFILEINFO *llSrcFiles,
                       BOOL bSkipNtUser, LPTSTR lpExcludeList, BOOL bRecurseDest)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA fd;
    LPTSTR lpSrcEnd, lpDestEnd, lpTemp;
    BOOL bResult = TRUE;
    BOOL bSkip;
    DWORD dwErr, dwErr1 = 0;
    TCHAR szErr[MAX_PATH];

    dwErr = GetLastError();


    //
    // Setup the ending pointers
    //

    lpSrcEnd = CheckSlash (lpSrcDir);
    lpDestEnd = CheckSlash (lpDestDir);


    //
    // Append *.* to the source directory
    //

    lstrcpy(lpSrcEnd, c_szStarDotStar);



    //
    // Search through the source directory
    //

    hFile = FindFirstFile(lpSrcDir, &fd);

    if (hFile == INVALID_HANDLE_VALUE) {

        if ( (GetLastError() == ERROR_FILE_NOT_FOUND) ||
             (GetLastError() == ERROR_PATH_NOT_FOUND) ) {

            //
            // bResult is already initialized to TRUE, so
            // just fall through.
            //

        } else {
            DebugMsg((DM_WARNING, TEXT("RecurseDirectory: FindFirstFile failed.  Error = %d"),
                     GetLastError()));

            dwErr1 = GetLastError();

            *lpSrcEnd = TEXT('\0');
            *lpDestEnd = TEXT('\0');

            if (!bRecurseDest) {
            
                ReportError(((dwFlags & CPD_NOERRORUI)? PI_NOUI:0) , IDS_COPYERROR, 
                             lpSrcDir, lpDestDir, GetErrString(GetLastError(), szErr));
            }
            else {
                ReportError(((dwFlags & CPD_NOERRORUI)? PI_NOUI:0) , IDS_COPYERROR, 
                             lpDestDir, lpSrcDir, GetErrString(GetLastError(), szErr));
            }
            
            dwErr = dwErr1;
            bResult = FALSE;
        }

        goto RecurseDir_Exit;
    }


    do {

        //
        // Check whether we have enough space in our buffers
        //

        *lpSrcEnd = TEXT('\0');
        *lpDestEnd = TEXT('\0');

        if ((lstrlen(lpSrcDir)+lstrlen(fd.cFileName) >= MAX_PATH) ||
            (lstrlen(lpDestDir)+lstrlen(fd.cFileName) >= MAX_PATH)) {

            LPTSTR lpErrSrc=NULL, lpErrDest=NULL;
            BOOL bRet;

            DebugMsg((DM_WARNING, TEXT("RecurseDirectory: %s is too long. src = %s, dest = %s"), fd.cFileName, lpSrcDir, lpDestDir));

            if (dwFlags & CPD_IGNORELONGFILENAMES) 
                continue;

            //
            // Allocate a buffer to show the file names
            //
            
            lpErrSrc = LocalAlloc(LPTR, (lstrlen(lpSrcDir)+lstrlen(fd.cFileName)+1)*sizeof(TCHAR));
            lpErrDest = LocalAlloc(LPTR, (lstrlen(lpDestDir)+lstrlen(fd.cFileName)+1)*sizeof(TCHAR));
            
            //
            // Show the UI
            //
            
            if ((!lpErrSrc) || (!lpErrDest)) {
            
                if (!bRecurseDest) {
                
                    ReportError(((dwFlags & CPD_NOERRORUI)? PI_NOUI:0) , IDS_COPYERROR, 
                            lpSrcDir, lpDestDir, GetErrString(ERROR_FILENAME_EXCED_RANGE, szErr));
                } else {
                
                    ReportError(((dwFlags & CPD_NOERRORUI)? PI_NOUI:0) , IDS_COPYERROR, 
                            lpDestDir, lpSrcDir, GetErrString(ERROR_FILENAME_EXCED_RANGE, szErr));
                }
            }
            else {
                
                lstrcpy(lpErrSrc, lpSrcDir); 
                lstrcpy(lpErrSrc+lstrlen(lpErrSrc), fd.cFileName);
                
                lstrcpy(lpErrDest, lpDestDir);
                lstrcpy(lpErrDest+lstrlen(lpErrDest), fd.cFileName);
                
                if (!bRecurseDest) {
                
                    ReportError(((dwFlags & CPD_NOERRORUI)? PI_NOUI:0) , IDS_COPYERROR, 
                            lpErrSrc, lpErrDest, GetErrString(ERROR_FILENAME_EXCED_RANGE, szErr));
                } else {
                
                    ReportError(((dwFlags & CPD_NOERRORUI)? PI_NOUI:0) , IDS_COPYERROR, 
                            lpErrDest, lpErrSrc, GetErrString(ERROR_FILENAME_EXCED_RANGE, szErr));
                }
            }
            
            if (lpErrSrc)
                LocalFree(lpErrSrc);
            if (lpErrDest)
                LocalFree(lpErrDest);
            
               
            //
            // Set the error and quit.
            //
                
            dwErr = ERROR_FILENAME_EXCED_RANGE;
            bResult = FALSE;
            goto RecurseDir_Exit;
            
        }

        //
        // Append the file / directory name to the working buffers
        //

        lstrcpy (lpSrcEnd, fd.cFileName);
        lstrcpy (lpDestEnd, fd.cFileName);


        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

            //
            // Check for "." and ".."
            //

            if (!lstrcmpi(fd.cFileName, c_szDot)) {
                continue;
            }

            if (!lstrcmpi(fd.cFileName, c_szDotDot)) {
                continue;
            }


            //
            // Check if this directory should be excluded
            //

            if (lpExcludeList) {

                bSkip = FALSE;
                lpTemp = lpExcludeList;

                while (*lpTemp) {

                    if (lstrcmpi (lpTemp, lpSrcDir) == 0) {
                        bSkip = TRUE;
                        break;
                    }

                    lpTemp += lstrlen (lpTemp) + 1;
                }

                if (bSkip) {
                    DebugMsg((DM_VERBOSE, TEXT("RecurseDirectory: Skipping <%s> due to exclusion list."),
                             lpSrcDir));
                    continue;
               }
            }


            //
            // Found a directory.
            //
            // 1)  Change into that subdirectory on the source drive.
            // 2)  Recurse down that tree.
            // 3)  Back up one level.
            //

            //
            // Add to the list of directories
            //

            if (dwFlags & CPD_SYSTEMDIRSONLY) {

                //
                // if it is encrypted, don't recurse into it
                //

                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED)) {

                    DWORD dwNewFlags = dwFlags;

                    //
                    // Add to the list of directories only if marked as system, o/w just recurse through
                    //

                    if (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) {
                        if (!AddFileInfoNode (llSrcDirs, lpSrcDir, lpDestDir, &fd.ftLastWriteTime,
                                              &fd.ftCreationTime, 0, fd.dwFileAttributes)) {
                            DebugMsg((DM_WARNING, TEXT("RecurseDirectory: AddFileInfoNode failed")));
                            dwErr = GetLastError();
                            goto RecurseDir_Exit;
                        }

                        dwNewFlags ^= CPD_SYSTEMDIRSONLY;
                        DebugMsg((DM_VERBOSE, TEXT("RecurseDirectory: Adding %s to the list of directories because system bit is on"), lpSrcDir));
                    }

                    //
                    // Recurse the subdirectory
                    //

                    if (!RecurseDirectory(lpSrcDir, lpDestDir, dwNewFlags,
                                          llSrcDirs, llSrcFiles, FALSE, lpExcludeList, bRecurseDest)) {
                        DebugMsg((DM_VERBOSE, TEXT("RecurseDirectory: RecurseDirectory returned FALSE")));
                        bResult = FALSE;
                        dwErr = GetLastError();
                        goto RecurseDir_Exit;
                    }

                } else {
                    DebugMsg((DM_VERBOSE, TEXT("RecurseDirectory: Skipping <%s> since the encrypted attribute is set."),
                             lpSrcDir));
                }

                continue;
            }

            //
            // Popup time
            //

            if (dwFlags & CPD_NONENCRYPTEDONLY) {
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) {

                    DebugMsg((DM_VERBOSE, TEXT("RecurseDirectory: Detected Encrypted file %s, Aborting.."), lpSrcDir));
                    dwErr = ERROR_FILE_ENCRYPTED;
                    bResult = FALSE;
                    goto RecurseDir_Exit;
               }
            }

            //
            // Ignore encrypted file
            //

            if (dwFlags & CPD_IGNOREENCRYPTEDFILES) {
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) {
                    DebugMsg((DM_VERBOSE, TEXT("RecurseDirectory: Skipping <%s> since the encrypted attribute is set..."),
                             lpSrcDir));
                    continue;
               }
            }

            //
            // Add it to the list
            //

            if (!AddFileInfoNode (llSrcDirs, lpSrcDir, lpDestDir, &fd.ftLastWriteTime,
                &fd.ftCreationTime, 0, fd.dwFileAttributes)) {
                DebugMsg((DM_WARNING, TEXT("RecurseDirectory: AddFileInfoNode failed")));
                dwErr = GetLastError();
                goto RecurseDir_Exit;
            }
            
            //
            // Recurse the subdirectory
            //
            
            if (!RecurseDirectory(lpSrcDir, lpDestDir, dwFlags,
                llSrcDirs, llSrcFiles, FALSE, lpExcludeList, bRecurseDest)) {
                DebugMsg((DM_VERBOSE, TEXT("RecurseDirectory: RecurseDirectory returned FALSE")));
                bResult = FALSE;
                dwErr = GetLastError();
                goto RecurseDir_Exit;
            }
            
            DebugMsg((DM_VERBOSE, TEXT("RecurseDirectory: Adding %s to the list of directories"), lpSrcDir));

        } else {

            //
            // if the directories only bit is set, don't copy anything else
            //

            if (dwFlags & CPD_SYSTEMDIRSONLY) {
                DebugMsg((DM_VERBOSE, TEXT("RecurseDirectory: Skipping <%s> since the system directories only attribute is set."),
                             lpSrcDir));
                continue;
            }

            //
            // If the filename found starts with "ntuser", then ignore
            // it because the hive will be copied below (if appropriate).
            //

            if (bSkipNtUser && lstrlen(fd.cFileName) >= 6) {
                if (CompareString (LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                                   fd.cFileName, 6,
                                   TEXT("ntuser"), 6) == CSTR_EQUAL) {
                    continue;
                }
            }

            //
            // Check if this file should be excluded
            //

            if (dwFlags & CPD_SYSTEMFILES) {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)) {
                    DebugMsg((DM_VERBOSE, TEXT("RecurseDirectory: Skipping <%s> since the system attribute is not set."),
                             lpSrcDir));
                    continue;
               }
            }

            //
            // if it is systemfile, it can not be encrypted. 
            //

            if (dwFlags & CPD_NONENCRYPTEDONLY) {
                
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) {

                    DebugMsg((DM_VERBOSE, TEXT("RecurseDirectory: Detected Encrypted file %s, Aborting..."), lpSrcDir));
                    dwErr = ERROR_FILE_ENCRYPTED;
                    bResult = FALSE;
                    goto RecurseDir_Exit;
               }
            }


            if (dwFlags & CPD_IGNOREENCRYPTEDFILES) {
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) {
                    DebugMsg((DM_VERBOSE, TEXT("RecurseDirectory: Skipping <%s> since the encrypted attribute is set."),
                             lpSrcDir));
                    continue;
               }
            }


            //
            // We found a file.  Add it to the list.
            //

            if (!AddFileInfoNode (llSrcFiles, lpSrcDir, lpDestDir,
                                  &fd.ftLastWriteTime, &fd.ftCreationTime,
                                  fd.nFileSizeLow, fd.dwFileAttributes)) {
                DebugMsg((DM_WARNING, TEXT("RecurseDirectory: AddFileInfoNode failed")));
                dwErr = GetLastError();
                goto RecurseDir_Exit;
            }

        }


        //
        // Find the next entry
        //

    } while (FindNextFile(hFile, &fd));


RecurseDir_Exit:

    //
    // Remove the file / directory name appended above
    //

    *lpSrcEnd = TEXT('\0');
    *lpDestEnd = TEXT('\0');


    //
    // Close the search handle
    //

    if (hFile != INVALID_HANDLE_VALUE) {
        FindClose(hFile);
    }

    SetLastError(dwErr);
    return bResult;
}

//*************************************************************
//
//  CopyProgressRoutine()
//
//  Purpose:    Callback function for CopyFileEx
//
//  Parameters:  See doc's.
//
//  Return:     PROGRESS_CONTINUE
//
//*************************************************************

DWORD WINAPI CopyProgressRoutine(LARGE_INTEGER TotalFileSize,
                                 LARGE_INTEGER TotalBytesTransferred,
                                 LARGE_INTEGER StreamSize,
                                 LARGE_INTEGER StreamBytesTransferred,
                                 DWORD dwStreamNumber,
                                 DWORD dwCallbackReason,
                                 HANDLE hSourceFile,
                                 HANDLE hDestinationFile,
                                 LPVOID lpData)
{
    if (dwCallbackReason == CALLBACK_CHUNK_FINISHED) {

        if (lpData) {
            ShowWindow ((HWND)lpData, SW_SHOWNORMAL);
            SendDlgItemMessage ((HWND)lpData, IDC_PROGRESS, PBM_DELTAPOS,
                                TotalBytesTransferred.LowPart, 0);
        }
    }

    switch (dwCallbackReason)
    {
    case PRIVCALLBACK_ENCRYPTION_FAILED:
    case PRIVCALLBACK_COMPRESSION_FAILED:
    case PRIVCALLBACK_SPARSE_FAILED:
    case PRIVCALLBACK_OWNER_GROUP_FAILED:
    case PRIVCALLBACK_DACL_ACCESS_DENIED:
    case PRIVCALLBACK_SACL_ACCESS_DENIED:
    case PRIVCALLBACK_OWNER_GROUP_ACCESS_DENIED:
        return PROGRESS_CANCEL;
    default:
        return PROGRESS_CONTINUE;   //all other conditions can be safely ignored
    }
}

//*************************************************************
//
//  ReconcileDirectory()
//
//  Purpose:     Compares the source and destination file.
//               If the source is newer, then it is copied
//               over the destination.
//
//  Parameters:  lpSrcDir   -   source filename
//               lpDestDir  -   destination filename
//               dwFlags    -   flags
//               dwSrcAttribs   Source Attributes for decompression,
//                              decryption later on.
//
//
//  Return:     1 if successful (no file copied)
//              0 if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/26/99     ushaji     Created
//
//*************************************************************

BOOL ReconcileDirectory(LPTSTR lpSrcDir, LPTSTR lpDestDir, 
                        DWORD dwFlags, DWORD dwSrcAttribs)
{
    
    DWORD  dwCopyFlags=0;
    BOOL   bCancel = FALSE;
    
    //
    // Clear any existing attributes
    //

    SetFileAttributes (lpDestDir, FILE_ATTRIBUTE_NORMAL);

    if (!CreateNestedDirectory(lpDestDir, NULL)) {        
        DebugMsg((DM_WARNING, TEXT("ReconcileDirectory: Failed to create the destination directory <%s>.  Error = %d"),
            lpDestDir, GetLastError()));
        return FALSE;
    }

    // 
    // Set up the copy flags to copy the encryption/compression on dirs over.
    //
    
    if (!(dwFlags & CPD_IGNORESECURITY))
        dwCopyFlags = PRIVCOPY_FILE_METADATA;

     dwCopyFlags |= PRIVCOPY_FILE_DIRECTORY | PRIVCOPY_FILE_SUPERSEDE;


    if (!PrivCopyFileExW(lpSrcDir, lpDestDir,
                        (LPPROGRESS_ROUTINE) CopyProgressRoutine,
                         NULL, &bCancel, dwCopyFlags)) {
        
        DebugMsg((DM_WARNING, TEXT("ReconcileDirectory: Failed to copy over the attributes src <%s> to destination directory <%s>.  Error = %d"),
            lpSrcDir, lpDestDir, GetLastError()));
        return FALSE;
    }

    // decompression/decryption

    return TRUE;
}

//*************************************************************
//
//  ReconcileFile()
//
//  Purpose:     Compares the source and destination file.
//               If the source is newer, then it is copied
//               over the destination.
//
//  Parameters:  lpSrcFile  -   source filename
//               lpDestFile -   destination filename
//               dwFlags    -   flags
//               ftSrcTime  -   Src file time (can be NULL)
//               hStatusDlg -   Status dialog
//               dwFileSize -   File size
//
//
//  Return:     1 if successful (no file copied)
//              2 if successful (and a file was copied)
//              0 if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              5/25/95     ericflo    Created
//
//*************************************************************

INT ReconcileFile (LPCTSTR lpSrcFile, LPCTSTR lpDestFile,
                    DWORD dwFlags, LPFILETIME ftSrcTime,
                    HWND hStatusDlg, DWORD dwFileSize)
{
    WIN32_FILE_ATTRIBUTE_DATA fad;
    FILETIME ftWriteSrc, ftWriteDest;
    INT iCopyFile = 0;
    DWORD dwErr, dwErr1 = 0;

    dwErr = GetLastError();

    //
    // If the flags have CPD_FORCECOPY, then skip to the
    // copy file call without checking the timestamps.
    //

    if (!(dwFlags & CPD_FORCECOPY)) {


        //
        // If we were given a source file time, use that
        //

        if (ftSrcTime) {
            ftWriteSrc.dwLowDateTime = ftSrcTime->dwLowDateTime;
            ftWriteSrc.dwHighDateTime = ftSrcTime->dwHighDateTime;

        } else {


            //
            // Query for the source file time
            //

            if (!GetFileAttributesEx (lpSrcFile, GetFileExInfoStandard, &fad)) {
                DebugMsg((DM_WARNING, TEXT("ReconcileFile: GetFileAttributes on the source failed with error = %d"),
                         GetLastError()));
                dwErr = GetLastError();
                goto Exit;
            }

            ftWriteSrc.dwLowDateTime = fad.ftLastWriteTime.dwLowDateTime;
            ftWriteSrc.dwHighDateTime = fad.ftLastWriteTime.dwHighDateTime;
        }


        //
        // Attempt to open the destination file
        //

        if (!GetFileAttributesEx (lpDestFile, GetFileExInfoStandard, &fad)) {
            DWORD dwError;

            //
            // GetFileAttributesEx failed to query the destination
            // file.  If the last error is file not found
            // then we automaticaly will copy the file.
            //

            dwError = GetLastError();

            if (dwError == ERROR_FILE_NOT_FOUND) {

                iCopyFile = 1;

            } else {

                //
                // GetFileAttributesEx failed with some other error
                //

                DebugMsg((DM_WARNING, TEXT("ReconcileFile: GetFileAttributesEx on the destination failed with error = %d"),
                         dwError));
                dwErr = dwError;
                goto Exit;
            }

        } else {

            ftWriteDest.dwLowDateTime = fad.ftLastWriteTime.dwLowDateTime;
            ftWriteDest.dwHighDateTime = fad.ftLastWriteTime.dwHighDateTime;
        }

    } else {

        //
        // The CPD_FORCECOPY flag is turned on, set iCopyFile to 1.
        //

        iCopyFile = 1;
    }


    //
    // If iCopyFile is still zero, then we need to compare
    // the last write time stamps.
    //

    if (!iCopyFile) {
        LONG lResult;

        //
        // If the source is later than the destination
        // we need to copy the file.
        //

        lResult = CompareFileTime(&ftWriteSrc, &ftWriteDest);

        if (lResult == 1) {
            iCopyFile = 1;
        }

        if ( (dwFlags & CPD_COPYIFDIFFERENT) && (lResult == -1) ) {
            iCopyFile = 1;
        }
    }


    //
    // Copy the file if appropriate
    //

    if (iCopyFile) {
        BOOL bCancel = FALSE;
        TCHAR szTempFile[MAX_PATH + 1];
        TCHAR szTempDir[MAX_PATH + 1];
        LPTSTR lpTemp;
        DWORD  dwCopyFlags;

        //
        // Clear any existing attributes
        //

        SetFileAttributes (lpDestFile, FILE_ATTRIBUTE_NORMAL);
    
        if (!(dwFlags & CPD_IGNORESECURITY))
            dwCopyFlags = PRIVCOPY_FILE_METADATA;
        else
            dwCopyFlags = 0;

        dwCopyFlags |= PRIVCOPY_FILE_SUPERSEDE;

        //
        // Figure out what the destination directory is
        //

        lstrcpy (szTempDir, lpDestFile);
        lpTemp = szTempDir + lstrlen (szTempDir);

        while ((lpTemp > szTempDir) && (*lpTemp != TEXT('\\'))) {
            lpTemp--;
        }

        if (lpTemp == szTempDir) {
            lstrcpy (szTempDir, TEXT("."));
        } else {
            *lpTemp = TEXT('\0');
        }


        //
        // Generate a temporary file name
        //

        if (GetTempFileName (szTempDir, TEXT("prf"), 0, szTempFile)) {


            //
            // Copy the file to the temp file name
            //

            if (PrivCopyFileExW(lpSrcFile, szTempFile,
                            (LPPROGRESS_ROUTINE) CopyProgressRoutine,
                            (LPVOID) hStatusDlg, &bCancel, dwCopyFlags)) {

                //
                // Delete the original file
                //

                if (!DeleteFile (lpDestFile)) {
                    if (GetLastError() != ERROR_FILE_NOT_FOUND) {
                        DebugMsg((DM_WARNING, TEXT("ReconcileFile: Failed to delete file <%s> with error = %d"),
                                 lpDestFile, GetLastError()));

                        dwErr1 = GetLastError();
                        DeleteFile(szTempFile);

                        if (!(dwFlags & CPD_IGNORECOPYERRORS)) {
                            iCopyFile = 0;
                            dwErr = dwErr1;
                            goto Exit;
                        }
                    }
                }


                //
                // Rename the temp file to the original file name
                //

                if (!MoveFile (szTempFile, lpDestFile)) {
                    DebugMsg((DM_WARNING, TEXT("ReconcileFile: Failed to rename file <%s> to <%s> with error = %d"),
                             szTempFile, lpDestFile, GetLastError()));

                    dwErr1 = GetLastError();
                    // do not remove it in this case.

                    if (!(dwFlags & CPD_IGNORECOPYERRORS)) {
                        iCopyFile = 0;
                        dwErr = dwErr1;
                        goto Exit;
                    }
                }

                DebugMsg((DM_VERBOSE, TEXT("ReconcileFile: %s ==> %s  [OK]"),
                         lpSrcFile, lpDestFile));
                iCopyFile = 2;

            } else {
                dwErr1 = GetLastError();
                DeleteFile(szTempFile);

                DebugMsg((DM_WARNING, TEXT("ReconcileFile: %s ==> %s  [FAILED!!!]"),
                         lpSrcFile, szTempFile));

                DebugMsg((DM_WARNING, TEXT("ReconcileFile: CopyFile failed with error = %d"),
                         dwErr1));

                if (!(dwFlags & CPD_IGNORECOPYERRORS)) {
                    iCopyFile = 0;
                    dwErr = dwErr1;
                    goto Exit;
                }
            }

        } else {
            DebugMsg((DM_WARNING, TEXT("ReconcileFile: GetTempFileName failed with %d"),
                     GetLastError()));

            if (!(dwFlags & CPD_IGNORECOPYERRORS)) {
                iCopyFile = 0;
                dwErr = GetLastError();
                goto Exit;
            }
        }

    } else {

        //
        // No need to copy the file since the time stamps are the same
        // Set iCopyFile to 1 so the return value is success without
        // copying a file.
        //

        iCopyFile = 1;
    }


Exit:


    //
    // Update the progress indicator
    //

    if (hStatusDlg && !iCopyFile) {
        SendDlgItemMessage (hStatusDlg, IDC_PROGRESS,
                            PBM_DELTAPOS, dwFileSize, 0);
    }


    SetLastError(dwErr);
    return iCopyFile;
}


//*************************************************************
//
//  AddFileInfoNode()
//
//  Purpose:    Adds a node to the linklist of files
//
//  Parameters: lpFileInfo     -   Link list to add to
//              lpSrcFile      -   Source filename
//              lpDestFile     -   Destination filename
//              ftLastWrite    -   Last write time stamp
//              ftCreationTime -   File creation time
//              dwFileSize     -   Size of the file
//              dwFileAttribs  - File attributes
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              9/28/95     ericflo    Created
//
//*************************************************************

BOOL AddFileInfoNode (LPFILEINFO *lpFileInfo, LPTSTR lpSrcFile,
                      LPTSTR lpDestFile, LPFILETIME ftLastWrite,
                      LPFILETIME ftCreationTime, DWORD dwFileSize,
                      DWORD dwFileAttribs)
{
    LPFILEINFO lpNode;


    lpNode = (LPFILEINFO) LocalAlloc(LPTR, sizeof(FILEINFO));

    if (!lpNode) {
        return FALSE;
    }


    lstrcpy (lpNode->szSrc, lpSrcFile);
    lstrcpy (lpNode->szDest, lpDestFile);

    lpNode->ftLastWrite.dwLowDateTime = ftLastWrite->dwLowDateTime;
    lpNode->ftLastWrite.dwHighDateTime = ftLastWrite->dwHighDateTime;

    lpNode->ftCreationTime.dwLowDateTime = ftCreationTime->dwLowDateTime;
    lpNode->ftCreationTime.dwHighDateTime = ftCreationTime->dwHighDateTime;

    lpNode->dwFileSize = dwFileSize;
    lpNode->dwFileAttribs = (dwFileAttribs & ~FILE_ATTRIBUTE_DIRECTORY);

    lpNode->pNext = *lpFileInfo;

    *lpFileInfo = lpNode;

    return TRUE;

}

//*************************************************************
//
//  FreeFileInfoList()
//
//  Purpose:    Free's a file info link list
//
//  Parameters: lpFileInfo  -   List to be freed
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              9/28/95     ericflo    Created
//
//*************************************************************

BOOL FreeFileInfoList (LPFILEINFO lpFileInfo)
{
    LPFILEINFO lpNext;


    if (!lpFileInfo) {
        return TRUE;
    }


    lpNext = lpFileInfo->pNext;

    while (lpFileInfo) {
        LocalFree (lpFileInfo);
        lpFileInfo = lpNext;

        if (lpFileInfo) {
            lpNext = lpFileInfo->pNext;
        }
    }

    return TRUE;
}

//*************************************************************
//
//  SyncItems()
//
//  Purpose:    Removes unnecessary items from the destination
//              directory tree
//
//  Parameters: lpSrcItems  -   Link list of source items
//              lpDestItems -   Link list of dest items
//              bFile       -   File or directory list
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//  Comments:
//
//  History:    Date        Author     Comment
//              9/28/95     ericflo    Created
//
//*************************************************************

BOOL SyncItems (LPFILEINFO lpSrcItems, LPFILEINFO lpDestItems,
                BOOL bFile, LPFILETIME ftDelRefTime)
{
    LPFILEINFO lpTempSrc, lpTempDest;


    //
    // Check for NULL pointers
    //

#ifdef DBG
    if (ftDelRefTime)
    {
        SYSTEMTIME SystemTime;
        FileTimeToSystemTime(ftDelRefTime, &SystemTime); 
        DebugMsg((DM_VERBOSE, TEXT("SyncItems: DelRefTime. Year: %d, Month %d, Day %d, Hour %d, Minute %d"), SystemTime.wYear, 
                                SystemTime.wMonth, SystemTime.wDay, SystemTime.wHour, SystemTime.wMinute));                
    }
#endif                 

    if (!lpSrcItems || !lpDestItems) {
        return TRUE;
    }


    //
    // Loop through everyitem in lpDestItems to see if it
    // is in lpSrcItems.  If not, delete it.
    //

    lpTempDest = lpDestItems;

    while (lpTempDest) {

        lpTempSrc = lpSrcItems;

        while (lpTempSrc) {

            if (lstrcmpi(lpTempDest->szSrc, lpTempSrc->szDest) == 0) {
                break;
            }

            lpTempSrc = lpTempSrc->pNext;
        }

        //
        // If lpTempSrc is NULL, then this file / directory is a candidate
        // for being deleted
        //

        if (!lpTempSrc) {
            BOOL bDelete = TRUE;


            //
            // If a delete reference time was offered, compare the
            // source time with the ref time and only delete files
            // which have a source time older than the ref time
            //

            if (ftDelRefTime) {

                if (CompareFileTime (&lpTempDest->ftLastWrite, ftDelRefTime) == 1) {                    
                    bDelete = FALSE;
                }
                else if (CompareFileTime (&lpTempDest->ftCreationTime, ftDelRefTime) == 1) {
                    bDelete = FALSE;
                }
            }


            if (bDelete) {

                //
                // Delete the file / directory
                //

                DebugMsg((DM_VERBOSE, TEXT("SyncItems: removing <%s>"),
                         lpTempDest->szSrc));


                if (bFile) {
                   SetFileAttributes(lpTempDest->szSrc, FILE_ATTRIBUTE_NORMAL);
                   if (!DeleteFile (lpTempDest->szSrc)) {
                       DebugMsg((DM_WARNING, TEXT("SyncItems: Failed to delete <%s>.  Error = %d."),
                                lpTempDest->szSrc, GetLastError()));
                   }

                } else {
                   SetFileAttributes(lpTempDest->szSrc, FILE_ATTRIBUTE_NORMAL);
                   if (!RemoveDirectory (lpTempDest->szSrc)) {
                       DebugMsg((DM_WARNING, TEXT("SyncItems: Failed to remove <%s>.  Error = %d"),
                                lpTempDest->szSrc, GetLastError()));
                   }

                }
            }
            else
            {
                    DebugMsg((DM_VERBOSE, TEXT("SyncItems: New file or directory <%s> in destination since this profile was loaded.  This will NOT be deleted."),
                              lpTempDest->szSrc));
#ifdef DBG
                    {
                        SYSTEMTIME SystemTime;
                        FileTimeToSystemTime(&lpTempDest->ftLastWrite, &SystemTime); 
                        DebugMsg((DM_VERBOSE, TEXT("SyncItems: File WriteTime. Year: %d, Month %d, Day %d, Hour %d, Minute %d"), SystemTime.wYear, 
                                                                     SystemTime.wMonth, SystemTime.wDay, SystemTime.wHour, SystemTime.wMinute));                
                        FileTimeToSystemTime(&lpTempDest->ftCreationTime, &SystemTime); 
                        DebugMsg((DM_VERBOSE, TEXT("SyncItems: File CreationTime. Year: %d, Month %d, Day %d, Hour %d, Minute %d"), SystemTime.wYear, 
                                                                     SystemTime.wMonth, SystemTime.wDay, SystemTime.wHour, SystemTime.wMinute));                
                    }
#endif                 
            }
        }

        lpTempDest = lpTempDest->pNext;
    }

    return TRUE;
}

#if 0
//*************************************************************
//
//  AbortFileCopy()
//
//  Purpose:    Displays the file copy error message box
//
//  Parameters: lpSrc   -    Source filename
//              lpDest  -    Destination filename
//
//
//  Return:     TRUE if the profile copy should abort
//              FALSE if not (the file should be skipped)
//
//*************************************************************

BOOL AbortFileCopy2(LPTSTR lpSrc, LPTSTR lpDest, DWORD dwError)
{
    DWORD dwDlgTimeOut = PROFILE_DLG_TIMEOUT;
    DWORD dwType, dwSize;
    BOOL bRetVal;
    HKEY hKey;
    LONG lResult;
    COPYERRORINFO CopyErrorInfo;
    HANDLE hToken=NULL;
    DWORD dwErr = 0;

    DebugMsg((DM_VERBOSE, TEXT("AbortFileCopy: entering")));               

    dwErr = GetLastError();

    //
    // Get the dialog box timeout.
    //

    dwDlgTimeOut = PROFILE_DLG_TIMEOUT;

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           WINLOGON_KEY,
                           0,
                           KEY_READ,
                           &hKey);

    if (lResult == ERROR_SUCCESS) {

        dwSize = sizeof(DWORD);
        RegQueryValueEx (hKey,
                         TEXT("ProfileDlgTimeOut"),
                         NULL,
                         &dwType,
                         (LPBYTE) &dwDlgTimeOut,
                         &dwSize);

        RegCloseKey (hKey);
    }


    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           SYSTEM_POLICIES_KEY,
                           0,
                           KEY_READ,
                           &hKey);

    if (lResult == ERROR_SUCCESS) {

        dwSize = sizeof(DWORD);
        RegQueryValueEx (hKey,
                         TEXT("ProfileDlgTimeOut"),
                         NULL,
                         &dwType,
                         (LPBYTE) &dwDlgTimeOut,
                         &dwSize);

        RegCloseKey (hKey);
    }


    //
    // Log the error to the event log
    //

    ReportError(PI_NOUI, IDS_COPYERROR, lpSrc, lpDest, dwError);


    //
    // Display the error dialog
    //

    CopyErrorInfo.lpSrc = lpSrc;
    CopyErrorInfo.lpDest = lpDest;
    CopyErrorInfo.dwError = dwError;
    CopyErrorInfo.dwTimeout = dwDlgTimeOut;

    //
    // Thread impersonation might be lost in DialogBoxParam
    //

    if (!OpenThreadToken (GetCurrentThread(), TOKEN_IMPERSONATE | TOKEN_READ,
                          TRUE, &hToken)) {
        DebugMsg((DM_VERBOSE, TEXT("AbortFileCopy: ThreadToken could not be opened with error %d"), GetLastError()));                
    }

    bRetVal = (BOOL)DialogBoxParam (g_hDllInstance, MAKEINTRESOURCE(IDD_COPYERROR),
                                    NULL, CopyErrorDlgProc, (LPARAM) &CopyErrorInfo);


    if (!SetThreadToken(NULL, hToken)) {
        DebugMsg((DM_VERBOSE, TEXT("AbortFileCopy: ThreadToken could not be set with error %d."), GetLastError()));                
    }

    SetLastError(dwErr);

    if (bRetVal == IDC_ABORT) {
        return TRUE;
    }

    CloseHandle( hToken );
    
    DebugMsg((DM_VERBOSE, TEXT("AbortFileCopy: Leaving")));               

    return FALSE;
}

//*************************************************************
//
//  CopyErrorDlgProc()
//
//  Purpose:    Dialog box procedure for the copy error dialog
//
//  Parameters: hDlg    -   handle to the dialog box
//              uMsg    -   window message
//              wParam  -   wParam
//              lParam  -   lParam
//
//  Return:     TRUE if message was processed
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/25/96    ericflo    Created
//
//*************************************************************

INT_PTR APIENTRY CopyErrorDlgProc (HWND hDlg, UINT uMsg,
                                WPARAM wParam, LPARAM lParam)
{
    TCHAR szBuffer[10];
    static DWORD dwCopyErrorTime;
    LPCOPYERRORINFO lpCopyErrorInfo;
    TCHAR szMessage[MAX_PATH];

    switch (uMsg) {

        case WM_INITDIALOG:
           lpCopyErrorInfo = (LPCOPYERRORINFO) lParam;

           SetForegroundWindow (hDlg);
           CenterWindow (hDlg);

           GetErrString(lpCopyErrorInfo->dwError, szMessage);

           SetDlgItemText (hDlg, IDC_ERRORMSG, szMessage);

           SetDlgItemText (hDlg, IDC_SOURCE, lpCopyErrorInfo->lpSrc);
           SetDlgItemText (hDlg, IDC_DESTINATION, lpCopyErrorInfo->lpDest);

           dwCopyErrorTime = lpCopyErrorInfo->dwTimeout;

           if (dwCopyErrorTime > 0) {
               wsprintf (szBuffer, TEXT("%d"), dwCopyErrorTime);
               SetDlgItemText (hDlg, IDC_TIME, szBuffer);

               SetTimer (hDlg, 1, 1000, NULL);
           }
           return TRUE;

        case WM_TIMER:

           if (dwCopyErrorTime >= 1) {

               dwCopyErrorTime--;
               wsprintf (szBuffer, TEXT("%d"), dwCopyErrorTime);
               SetDlgItemText (hDlg, IDC_TIME, szBuffer);

           } else {

               //
               // Time's up.  Skip the file.
               //

               PostMessage (hDlg, WM_COMMAND, IDC_SKIPFILE, 0);
           }
           break;

        case WM_COMMAND:

          switch (LOWORD(wParam)) {

              case IDC_ABORT:
                  if (HIWORD(wParam) == BN_KILLFOCUS) {
                      KillTimer (hDlg, 1);
                      ShowWindow(GetDlgItem(hDlg, IDC_TIME), SW_HIDE);
                      ShowWindow(GetDlgItem(hDlg, IDC_TIMETITLE), SW_HIDE);

                  } else if (HIWORD(wParam) == BN_CLICKED) {
                      KillTimer (hDlg, 1);
                      EndDialog(hDlg, IDC_ABORT);
                  }
                  break;


              case IDC_SKIPFILE:
                  if (HIWORD(wParam) == BN_KILLFOCUS) {
                      KillTimer (hDlg, 1);
                      ShowWindow(GetDlgItem(hDlg, IDC_TIME), SW_HIDE);
                      ShowWindow(GetDlgItem(hDlg, IDC_TIMETITLE), SW_HIDE);

                  } else if (HIWORD(wParam) == BN_CLICKED) {
                      KillTimer (hDlg, 1);
                      EndDialog(hDlg, IDC_SKIPFILE);
                  }
                  break;

              default:
                  break;

          }
          break;

    }

    return FALSE;
}
#endif

//*************************************************************
//
//  CopyFileFunc()
//
//  Purpose:    Copies files
//
//  Parameters: lpThreadInfo    -   Thread information
//
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/23/96     ericflo    Created
//
//*************************************************************

void CopyFileFunc (LPTHREADINFO lpThreadInfo)
{
    HANDLE      hInstDll;
    LPFILEINFO  lpSrcFile;
    BOOL        bRetVal = TRUE;
    DWORD       dwError;
    TCHAR       szErr[MAX_PATH];

    hInstDll = LoadLibrary (TEXT("userenv.dll"));

    SetThreadDesktop (lpThreadInfo->hDesktop);


    while (TRUE) {

        if (lpThreadInfo->dwError) {
            break;
        }

        //
        // Query for the next file to copy
        //

        WaitForSingleObject(lpThreadInfo->hCopyEvent, INFINITE);

        lpSrcFile = lpThreadInfo->lpSrcFiles;

        if (lpSrcFile) {
            lpThreadInfo->lpSrcFiles = lpThreadInfo->lpSrcFiles->pNext;
        }

        SetEvent(lpThreadInfo->hCopyEvent);


        //
        // If NULL, then we're finished.
        //

        if (!lpSrcFile || lpThreadInfo->dwError) {
            break;
        }


        //
        // Copy the file
        //

        if (!ReconcileFile (lpSrcFile->szSrc, lpSrcFile->szDest,
                            lpThreadInfo->dwFlags, &lpSrcFile->ftLastWrite,
                            lpThreadInfo->hStatusDlg, lpSrcFile->dwFileSize)) {

            if (!(lpThreadInfo->dwFlags & CPD_IGNORECOPYERRORS)) {

                WaitForSingleObject(lpThreadInfo->hCopyEvent, INFINITE);
                
                if (!(lpThreadInfo->dwError)) {
                    dwError = GetLastError();

                    ReportError(((lpThreadInfo->dwFlags & CPD_NOERRORUI) ? PI_NOUI:0), IDS_COPYERROR, 
                            lpSrcFile->szSrc, lpSrcFile->szDest, GetErrString(dwError, szErr));

                    lpThreadInfo->dwError = dwError;
                    bRetVal = FALSE;
                }

                SetEvent(lpThreadInfo->hCopyEvent);
                break;
            }
        }
    }
    
    //
    // Clean up
    //

    if (hInstDll) {
        FreeLibraryAndExitThread(hInstDll, bRetVal);
    } else {
        ExitThread (bRetVal);
    }
}


//*************************************************************
//
//  CopyStatusThread()
//
//  Purpose:    Copy dialog status thread
//
//  Parameters: lpThreadInfo    -   Thread information
//
//  Return:     void
//
//  Comments:   Only the hStatusDlg field is used in the
//              thread info structure.
//
//  History:    Date        Author     Comment
//              10/22/96     ericflo    Created
//
//*************************************************************

void CopyStatusThread (LPTHREADINFO lpThreadInfo)
{
    HANDLE hInstDll, hInstComctl32;
    PFNINITCOMMONCONTROLS pfnInitCommonControls;
    MSG msg;
    DWORD dwResult;
    HANDLE hObjects[2];


    hInstDll = LoadLibrary (TEXT("userenv.dll"));

    SetThreadDesktop (lpThreadInfo->hDesktop);

    hInstComctl32 = LoadLibrary (TEXT("comctl32.dll"));

    if (hInstComctl32) {

        pfnInitCommonControls = (PFNINITCOMMONCONTROLS)GetProcAddress (hInstComctl32,
                                            "InitCommonControls");

        if (pfnInitCommonControls) {
            pfnInitCommonControls();
        }
    }

    lpThreadInfo->hStatusDlg = CreateDialogParam (g_hDllInstance,
                                                  MAKEINTRESOURCE(IDD_COPYSTATUS),
                                                  NULL, CopyStatusDlgProc,
                                                  lpThreadInfo->dwFlags);

    SetEvent (lpThreadInfo->hStatusInitEvent);

    if (lpThreadInfo->hStatusDlg) {

        hObjects[0] = lpThreadInfo->hStatusTermEvent;

        while (TRUE) {
            dwResult = MsgWaitForMultipleObjects (1, hObjects, FALSE, INFINITE,
                                                  QS_ALLPOSTMESSAGE | QS_ALLINPUT);

            if (dwResult == WAIT_FAILED) {
                DebugMsg((DM_WARNING, TEXT("CopyStatusThread: MsgWaitForMultipleObjects failed with %d"), GetLastError()));
                break;
            }

            if (dwResult == WAIT_OBJECT_0) {
                break;
            }

            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (!IsDialogMessage (lpThreadInfo->hStatusDlg, &msg)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }

        DestroyWindow(lpThreadInfo->hStatusDlg);
        lpThreadInfo->hStatusDlg = NULL;
    }

    if (hInstComctl32) {
        FreeLibrary (hInstComctl32);
    }

    if (hInstDll) {
        FreeLibraryAndExitThread(hInstDll, TRUE);
    } else {
        ExitThread (TRUE);
    }
}

//*************************************************************
//
//  CopyStatusDlgProc()
//
//  Purpose:    Dialog box procedure for the copy status dialog
//
//  Parameters: hDlg    -   handle to the dialog box
//              uMsg    -   window message
//              wParam  -   wParam
//              lParam  -   lParam
//
//  Return:     TRUE if message was processed
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/22/96    ericflo    Created
//
//*************************************************************

INT_PTR APIENTRY CopyStatusDlgProc (HWND hDlg, UINT uMsg,
                                 WPARAM wParam, LPARAM lParam)
{

    switch (uMsg) {

        case WM_INITDIALOG:
            {
            TCHAR szString[100];

            SetForegroundWindow (hDlg);
            CenterWindow (hDlg);

            if (lParam & CPD_CREATETITLE) {
                LoadString (g_hDllInstance, IDS_CREATING, szString, ARRAYSIZE(szString));
            } else {
                LoadString (g_hDllInstance, IDS_COPYING, szString, ARRAYSIZE(szString));
            }
            SetWindowText (hDlg, szString);

            }
            return TRUE;

        default:
            break;
    }

    return FALSE;
}

//*************************************************************
//
//  ConvertExclusionList()
//
//  Purpose:    Converts the semi-colon profile relative exclusion
//              list to fully qualified null terminated exclusion
//              list
//
//  Parameters: lpSourceDir     -  Profile root directory
//              lpExclusionList -  List of directories to exclude
//
//  Return:     List if successful
//              NULL if an error occurs
//
//*************************************************************

LPTSTR ConvertExclusionList (LPCTSTR lpSourceDir, LPCTSTR lpExclusionList)
{
    LPTSTR lpExcludeList = NULL, lpInsert, lpEnd, lpTempList;
    LPCTSTR lpTemp, lpDir;
    TCHAR szTemp[MAX_PATH];
    DWORD dwSize = 2;  // double null terminator
    DWORD dwStrLen;


    //
    // Setup a temp buffer to work with
    //

    lstrcpy (szTemp, lpSourceDir);
    lpEnd = CheckSlash (szTemp);


    //
    // Loop through the list
    //

    lpTemp = lpDir = lpExclusionList;

    while (*lpTemp) {

        //
        // Look for the semicolon separator
        //

        while (*lpTemp && ((*lpTemp) != TEXT(';'))) {
            lpTemp++;
        }


        //
        // Remove any leading spaces
        //

        while (*lpDir == TEXT(' ')) {
            lpDir++;
        }

        //
        // Note: 
        // Empty Spaces will not make the whole profile dir excluded
        // in RecurseDirectory.
        //

        //
        // Put the directory name on the temp buffer
        //

        lstrcpyn (lpEnd, lpDir, (int)(lpTemp - lpDir + 1));
        DebugMsg((DM_VERBOSE, TEXT("ConvertExclusionList: Adding %s to ExclusionList"), szTemp));

        //
        // Add the string to the exclusion list
        //

        if (lpExcludeList) {

            dwStrLen = lstrlen (szTemp) + 1;
            dwSize += dwStrLen;

            lpTempList = LocalReAlloc (lpExcludeList, dwSize * sizeof(TCHAR),
                                       LMEM_MOVEABLE | LMEM_ZEROINIT);

            if (!lpTempList) {
                DebugMsg((DM_WARNING, TEXT("ConvertExclusionList: Failed to realloc memory with %d"), GetLastError()));
                LocalFree (lpExcludeList);
                lpExcludeList = NULL;
                goto Exit;
            }

            lpExcludeList = lpTempList;

            lpInsert = lpExcludeList + dwSize - dwStrLen - 1;
            lstrcpy (lpInsert, szTemp);

        } else {

            dwSize += lstrlen (szTemp);
            lpExcludeList = LocalAlloc (LPTR, dwSize * sizeof(TCHAR));

            if (!lpExcludeList) {
                DebugMsg((DM_WARNING, TEXT("ConvertExclusionList: Failed to alloc memory with %d"), GetLastError()));
                goto Exit;
            }

            lstrcpy (lpExcludeList, szTemp);
        }


        //
        // If we are at the end of the exclusion list, we're done
        //

        if (!(*lpTemp)) {
            goto Exit;
        }


        //
        // Prep for the next entry
        //

        lpTemp++;
        lpDir = lpTemp;
    }

Exit:

    return lpExcludeList;
}

//*************************************************************
//
//  FindDirectorySize()
//
//  Purpose:    Takes the Directory Name and the list of files
//              returned by RecurseDir and gets the total size.
//
//  Parameters: lpDir          -  '\' terminated Source Directory 
//              lpFiles        -  List of files to be copied
//              dwFlags        -  Flags
//
//  Return:     The size of the directory 
//
//*************************************************************

DWORD FindDirectorySize(LPTSTR lpDir, LPFILEINFO lpFiles, DWORD dwFlags)
{
    LPFILEINFO      lpTemp = NULL;
    DWORD           dwTotalFiles = 0;
    HANDLE          hFile = NULL;
    LPTSTR          lpEnd;
    WIN32_FIND_DATA fd;


    lpEnd = lpDir+lstrlen(lpDir);

    lpTemp = lpFiles;

    while (lpTemp) {
        dwTotalFiles += lpTemp->dwFileSize;
        lpTemp = lpTemp->pNext;
    }
    
    
    //
    // If the hive files will be copied, calculate their size now
    //
    
    if (!(dwFlags & CPD_IGNOREHIVE)) {
        
        if (dwFlags & CPD_WIN95HIVE) {
            lstrcpy (lpEnd, c_szUserStar);
            
        } else {
            lstrcpy (lpEnd, c_szNTUserStar);
        }
        
        
        hFile = FindFirstFile(lpDir, &fd);
        
        if (hFile != INVALID_HANDLE_VALUE) {
            
            do  {
                
                dwTotalFiles += fd.nFileSizeLow;
                
            } while (FindNextFile(hFile, &fd));
            
            FindClose(hFile);
        }
        
        *lpEnd = TEXT('\0');
    }

    return dwTotalFiles;
}
