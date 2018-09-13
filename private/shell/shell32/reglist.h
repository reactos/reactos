
//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1991-1993
//
// File: reglist.h
//
// History:
//   5-30-94 KurtE      Created.
//
//---------------------------------------------------------------------------

// Define Callback used for different enumeration functions.
typedef BOOL (CALLBACK *PRLCALLBACK)(HDPA hdpa, HKEY hkey, LPCTSTR pszKey,
        LPCTSTR pszValueName, LPTSTR pszValue, LPCTSTR pszSrc,
        LPCTSTR pszDest);       

// Called to force rebuilding the list of paths.
BOOL WINAPI RLBuildListOfPaths(void);

// Terminate and cleanup our use of the Registry list.
void WINAPI RLTerminate(void);

// Define the main iterater function that uses the call back.
BOOL WINAPI RLEnumRegistry(HDPA hdpa, PRLCALLBACK prlcb,
        LPCTSTR pszSrc, LPCTSTR pszDest);


// RLIsPathInList: This function returns ht index of which item a string
// is in the list or -1 if not found.

int WINAPI RLIsPathInList(LPCTSTR pszPath);


// Function to call when the files really have changed...
int WINAPI RLFSChanged (LONG lEvent, LPITEMIDLIST pidl, LPITEMIDLIST pidlExtra);

