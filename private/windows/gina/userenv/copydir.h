//*************************************************************
//
//  Header file for copydir.c
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************


//
// File copy structure
//

typedef struct _FILEINFO {
    TCHAR            szSrc[MAX_PATH];
    TCHAR            szDest[MAX_PATH];
    FILETIME         ftLastWrite;
    FILETIME         ftCreationTime;
    DWORD            dwFileSize;
    DWORD            dwFileAttribs;
    struct _FILEINFO *pNext;
} FILEINFO, * LPFILEINFO;


#define NUM_COPY_THREADS        7

//
// ThreadInfo structure
//

typedef struct _THREADINFO {
    DWORD              dwFlags;
    HANDLE             hCopyEvent;
    LPFILEINFO         lpSrcFiles;
    DWORD              dwError;
    HWND               hStatusDlg;
    HANDLE             hStatusInitEvent;
    HANDLE             hStatusTermEvent;
    HDESK              hDesktop;
} THREADINFO, * LPTHREADINFO;


//
// Error dialog structure
//

typedef struct _COPYERRORINFO {
    LPTSTR     lpSrc;
    LPTSTR     lpDest;
    DWORD      dwError;
    DWORD      dwTimeout;
} COPYERRORINFO, * LPCOPYERRORINFO;



INT ReconcileFile (LPCTSTR lpSrcFile, LPCTSTR lpDestFile,
                    DWORD dwFlags, LPFILETIME ftSrcTime,
                    HWND hStatusDlg, DWORD dwFileSize);

INT_PTR APIENTRY CopyStatusDlgProc (HWND hDlg, UINT uMsg,
                                 WPARAM wParam, LPARAM lParam);

INT_PTR APIENTRY CopyErrorDlgProc (HWND hDlg, UINT uMsg,
                                WPARAM wParam, LPARAM lParam);
