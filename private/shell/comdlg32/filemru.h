/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    filemru.cpp

Abstract:

    This module contains the functions for implementing file mru
    in file open and file save dialog boxes

Revision History:
    01/22/98                arulk                   created
 
--*/


#define MAX_MRU   25
BOOL  LoadMRU(LPCTSTR pszFilter, HWND hwndCombo, int nMax);
BOOL  AddToMRU(LPOPENFILENAME lpOFN);

BOOL GetPathFromLastVisitedMRU(LPTSTR pszDir, DWORD cchDir);
BOOL AddToLastVisitedMRU(LPCTSTR pszFile, int nFileOffset);