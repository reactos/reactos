//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1998.
//
//  File:       propsht.h
//
//  Contents:   prop shell structs
//
//----------------------------------------------------------------------------
#ifndef _MULPRSHT_H
#define _MULPRSHT_H

#include "idlcomm.h" // for HIDA
#include "ids.h"

// we recycle this string from the printer stringtable in shell32.rc
#define IDS_UNKNOWNERROR IDS_PRTPROP_UNKNOWNERROR

#define IDT_SIZE 1

// file attribute state
typedef struct 
{
    DWORD   fReadOnly;      // each dword is one of BST_CHECKED, BST_UNCHECKED, or BST_INDETERMINATE 
    DWORD   fHidden;
    DWORD   fArchive;     
    DWORD   fIndex;
    DWORD   fCompress;
    DWORD   fEncrypt;
} ATTRIBUTESTATE;

typedef struct { // fpsp
    PROPSHEETPAGE       psp;
    BOOL                fMountedDrive;              // Are we dealing with a mounted drive or not?
    BOOL                fIsLink;                    // Is this a .lnk file?

    //the following fields are used by both the structures
    HWND                hDlg;
    TCHAR               szPath[MAX_PATH];           // full path to the file (single file case)
    TCHAR               szLinkTarget[MAX_PATH];     // full path of link target (if the file is a .lnk)
    ATTRIBUTESTATE      asInitial;                  // initial file attribute state
    ATTRIBUTESTATE      asCurrent;                  // current file attribute state
    BOOL                fIsCompressionAvailable;    // is comrpession supported on the volume?
    BOOL                fIsEncryptionAvailable;     // is encryption supported on the volume?
    BOOL                fIsIndexAvailable;          // is conten indexing supported in the filesystem?

    //BUGBUG : Check if union  can be used   
    //This is used only by the single file dialog proc
    HIDA                hida;
    HANDLE              hThread;                    // for computing folder size
    LPVOID              pAssocStore;                // pointer to the Association Store, we use it to query type info
    IProgressDialog*    pProgressDlg;               // pointer to the IProgressDialog object
    ULARGE_INTEGER      ulTotalNumberOfBytes;       // total # of bytes to apply attributes to (for progress dlg)
    ULARGE_INTEGER      ulNumberOfBytesDone;        // # of bytes that we have already applied attribs to (for progress dlg)    
    FOLDERCONTENTSINFO  fci;                        // the background size thread fills this structure with size info
    WIN32_FIND_DATA     fd;                         // info about the file we are currently applying attribs to
    HWND                hwndTip;                    // window handle for location tooltip
    HDPA                hdpaBadFiles;               // this dpa holds the names of the files that we dont want to apply attribs to

    TCHAR               szInitialName[MAX_PATH];    // the original "short" name we display in the edit box

    BOOL                fWMInitFinshed;             // are we finished processing the WM_INITDIALOG message (needed for rename)
    BOOL                fMultipleFiles;             // are there multiple files?
    BOOL                fRecursive;                 // should we recurse into subdirs when applying attributes?
    BOOL                fIsDirectory;               // is this file a directory (in multiple files case: are any of the files a directory?)
    BOOL                fIsExe;                     // if this is an .exe, we ask if they want to support user logon
    BOOL                fRename;                    // has the user renamed the file/folder?
    BOOL                fIgnoreAllErrors;           // has the user hit "ignore all" to the error message?
    BOOL                fShowExtension;             // are we showing the real extension for this file in the name editbox?
    BOOL                fFolderShortcut;

    int                 cItemsDone;                 // Number of items we have already applied attribs to (for progress dlg)
    BOOL                fDisableRename;             // Should the name edit box be disabled?

    //The following is used only by the mounted drv dialog proc
    int                 iDrive;                     // Drive id of the mounted drive if there is one
    TCHAR               szFileSys[48] ;             // file system name.
    BOOL                fCanRename;                 // is the name a valid name for renaming?

    // Folder shortcut specific stuff.
    LPITEMIDLIST        pidlFolder;
    LPITEMIDLIST        pidlTarget;
    BOOL                fValidateEdit;

} FILEPROPSHEETPAGE;

typedef struct { // dpsp
    PROPSHEETPAGE   psp;

    HWND            hDlg;

    //szDrive will contain the mountpoint (e.g. c:\ or c:\folder\folder2\)
    TCHAR           szDrive[MAX_PATH];
    int             iDrive;

    _int64          qwTot;
    _int64          qwFree;

    DWORD           dwPieShadowHgt;

    ULARGE_INTEGER  ulTotalNumberOfBytes;       // total # of bytes to apply attributes to (for progress dlg)
    ATTRIBUTESTATE  asInitial;                  // initial attribute state
    ATTRIBUTESTATE  asCurrent;                  // current attribute state

    BOOL            fIsCompressionAvailable;    // is file-based compression available on this volume (NTFS?)
    BOOL            fIsIndexAvailable;   // is content indexing available on this volume?
    BOOL            fRecursive;                 // should we recurse into subdirs when applying attributes?
    BOOL            fMountedDrive;              // is the proppage invoked from mounted point proppage
} DRIVEPROPSHEETPAGE;


typedef struct 
{ // AttributeError
    LPCTSTR pszPath;
    DWORD dwLastError;
} ATTRIBUTEERROR;


STDAPI_(BOOL) ApplyFileAttributes(LPCTSTR pszPath, FILEPROPSHEETPAGE* pfpsp, HWND hWndParent, BOOL* pbSomethingChanged);
STDAPI_(BOOL) ApplySingleFileAttributes(FILEPROPSHEETPAGE* pfpsp);
STDAPI_(BOOL_PTR) CALLBACK RecursivePromptDlgProc(HWND hDlgRecurse, UINT uMessage, WPARAM wParam, LPARAM lParam);
void SetDateTimeText(HWND hdlg, int id, const FILETIME *pftUTC);
void SetDateTimeTextEx(HWND hdlg, int id, const FILETIME *pftUTC, DWORD dwFlags);
STDAPI_(DWORD) GetVolumeFlags(LPCTSTR pszPath, OUT OPTIONAL LPTSTR pszFileSys, int cchFileSys);
STDAPI_(void) SetInitialFileAttribs(FILEPROPSHEETPAGE* pfpsp, DWORD dwFlags, DWORD dwMask);
BOOL_PTR CALLBACK AdvancedFileAttribsDlgProc(HWND hDlgAttribs, UINT uMessage, WPARAM wParam, LPARAM lParam);

#endif // _MULPRSHT_H
