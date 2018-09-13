//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       menuutil.hxx
//
//  Contents:   Context menu utilities, stolen from the shell. This is
//              basically CDefFolderMenu_MergeMenu and its support.
//
//  History:    20-Dec-95    BruceFo     Created
//
//----------------------------------------------------------------------------

#ifndef __MENUUTIL_HXX__
#define __MENUUTIL_HXX__

VOID
MyMergeMenu(
    HINSTANCE hinst,
    UINT idMainMerge,
    UINT idPopupMerge,
    LPQCMINFO pqcm);

VOID
MyInsertMenu(
    HINSTANCE hinst,
    UINT idInsert,
    LPQCMINFO pqcm);

// Cannonical command names stolen from the shell
extern TCHAR const c_szDelete[];
extern TCHAR const c_szCut[];
extern TCHAR const c_szCopy[];
extern TCHAR const c_szLink[];
extern TCHAR const c_szProperties[];
extern TCHAR const c_szPaste[];
extern TCHAR const c_szPasteLink[];
extern TCHAR const c_szRename[];

#endif // __MENUUTIL_HXX__
