/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    fileopen.c

Abstract:

    This module implements the Win32 fileopen dialogs.

Revision History:

--*/



//
//  Include Files.
//

#if (_WIN32_WINNT < 0x0500)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#include "comdlg32.h"

#include <lm.h>
#include <winnetwk.h>
#include <winnetp.h>
#include <shellapi.h>
#include <shlobj.h>
#include <commctrl.h>
#include <shsemip.h>
#include "fileopen.h"
#include "util.h"




//
//  Constant Declarations.
//

#define WNTYPE_DRIVE         1

#define MIN_DEFEXT_LEN       4

#define BMPHIOFFSET          9

//
//  hbmpDirs array index values.
//  Note:  Two copies: for standard background, and hilite.
//         Relative order is important.
//
#define OPENDIRBMP           0
#define CURDIRBMP            1
#define STDDIRBMP            2
#define FLOPPYBMP            3
#define HARDDRVBMP           4
#define CDDRVBMP             5
#define NETDRVBMP            6
#define RAMDRVBMP            7
#define REMDRVBMP            8
  //
  //  If the following disktype is passed to AddDisk, then bTmp will be
  //  set to true in the DISKINFO structure (if the disk is new).
  //
#define TMPNETDRV            9

#define MAXDOSFILENAMELEN    (12 + 1)     // 8.3 filename + 1 for NULL

//
//  Maximum number of filters on one filter line.
//
#define MAXFILTERS           36

//
//  File exclusion bits (don't show files of these types).
//
#define EXCLBITS             (FILE_ATTRIBUTE_HIDDEN)




//
//  Global Variables.
//

//
//  Caching drive list.
//
extern DWORD dwNumDisks;
extern OFN_DISKINFO gaDiskInfo[MAX_DISKS];
extern TCHAR g_szInitialCurDir[MAX_PATH];

DWORD dwNumDlgs = 0;

//
//  Used to update the dialogs after coming back from the net dlg button.
//
BOOL bGetNetDrivesSync = FALSE;
LPTSTR lpNetDriveSync = NULL;
BOOL bNetworkInstalled = TRUE;

//
//  Following array is used to send messages to all dialog box threads
//  that have requested enumeration updating from the worker
//  thread.  The worker thread sends off a message to each slot
//  in the array that is non-NULL.
//
HWND gahDlg[MAX_THREADS];

//
//  Strings for Filter Parsing.
//
const static TCHAR szSemiColonSpaceTab[] = TEXT("; \t");
const static TCHAR szSemiColonTab[] = TEXT(";\t");

//
//  For WNet apis.
//
HANDLE hLNDThread = NULL;

WNDPROC lpLBProc = NULL;
WNDPROC lpOKProc = NULL;

//
//  Drive/Dir bitmap dimensions.
//
LONG dxDirDrive = 0;
LONG dyDirDrive = 0;

//
//  BUGBUG: This needs to be on a per dialog basis for multi-threaded apps.
//
WORD wNoRedraw = 0;

UINT msgWOWDIRCHANGE;
UINT msgLBCHANGEA;
UINT msgSHAREVIOLATIONA;
UINT msgFILEOKA;

UINT msgLBCHANGEW;
UINT msgSHAREVIOLATIONW;
UINT msgFILEOKW;

BOOL bInChildDlg;
BOOL bFirstTime;
BOOL bInitializing;

//
//  Used by the worker thread to enumerate network disk resources.
//
extern DWORD cbNetEnumBuf;
extern LPTSTR gpcNetEnumBuf;

//
//  List Net Drives global variables.
//
extern HANDLE hLNDEvent;
BOOL bLNDExit = FALSE;

extern CRITICAL_SECTION g_csLocal;
extern CRITICAL_SECTION g_csNetThread;

extern DWORD g_tlsiCurDlg;

extern HDC hdcMemory;
extern HBITMAP hbmpOrigMemBmp;

HBITMAP hbmpDirDrive = HNULL;




//
//  Static Declarations.
//

static WORD cLock = 0;

//
//  Not valid RGB color.
//
static DWORD rgbWindowColor = 0xFF000000;
static DWORD rgbHiliteColor = 0xFF000000;
static DWORD rgbWindowText  = 0xFF000000;
static DWORD rgbHiliteText  = 0xFF000000;
static DWORD rgbGrayText    = 0xFF000000;
static DWORD rgbDDWindow    = 0xFF000000;
static DWORD rgbDDHilite    = 0xFF000000;

TCHAR szCaption[TOOLONGLIMIT + WARNINGMSGLENGTH];
TCHAR szWarning[TOOLONGLIMIT + WARNINGMSGLENGTH];

LPOFNHOOKPROC glpfnFileHook = 0;

//
//  BUGBUG:
//  Of course, in the case where there is a multi-threaded process
//  that has > 1 threads simultaneously calling GetFileOpen, the
//  following globals may cause problems.
//
static LONG dyItem = 0;
static LONG dyText;
static BOOL bChangeDir = FALSE;
static BOOL bCasePreserved;

//
//  Used for formatting long unc names (ex. banyan).
//
static DWORD dwAveCharPerLine = 10;


//
//  Context Help IDs.
//

const static DWORD aFileOpenHelpIDs[] =
{
    edt1,        IDH_OPEN_FILENAME,
    stc3,        IDH_OPEN_FILENAME,
    lst1,        IDH_OPEN_FILENAME,
    stc1,        IDH_OPEN_PATH,
    lst2,        IDH_OPEN_PATH,
    stc2,        IDH_OPEN_FILETYPE,
    cmb1,        IDH_OPEN_FILETYPE,
    stc4,        IDH_OPEN_DRIVES,
    cmb2,        IDH_OPEN_DRIVES,
    chx1,        IDH_OPEN_READONLY,
    pshHelp,     IDH_HELP,
    psh14,       IDH_PRINT_NETWORK,

    0, 0
};

const static DWORD aFileSaveHelpIDs[] =
{
    edt1,        IDH_OPEN_FILENAME,
    stc3,        IDH_OPEN_FILENAME,
    lst1,        IDH_OPEN_FILENAME,
    stc1,        IDH_OPEN_PATH,
    lst2,        IDH_OPEN_PATH,
    stc2,        IDH_SAVE_FILETYPE,
    cmb1,        IDH_SAVE_FILETYPE,
    stc4,        IDH_OPEN_DRIVES,
    cmb2,        IDH_OPEN_DRIVES,
    chx1,        IDH_OPEN_READONLY,
    pshHelp,     IDH_HELP,
    psh14,       IDH_PRINT_NETWORK,

    0, 0
};




//
//  Function Prototypes.
//

SHORT
GetFileTitleX(
    LPTSTR lpszFile,
    LPTSTR lpszTitle,
    WORD wBufSize);

BOOL
GetFileName(
    POPENFILEINFO pOFI,
    DLGPROC qfnDlgProc);

BOOL_PTR CALLBACK
FileOpenDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam);

BOOL_PTR CALLBACK
FileSaveDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam);

BOOL_PTR
InitFileDlg(
    HWND hDlg,
    WPARAM wParam,
    POPENFILEINFO pOFI);

int
InitTlsValues(
    POPENFILEINFO pOFI);

DWORD
InitFilterBox(
    HANDLE hDlg,
    LPCTSTR lpszFilter);

VOID
InitCurrentDisk(
    HWND hDlg,
    POPENFILEINFO pOFI,
    WORD cmb);

VOID
vDeleteDirDriveBitmap();

BOOL
LoadDirDriveBitmap();

void
SetRGBValues();

BOOL
FSetUpFile();

BOOL_PTR
FileOpenCmd(
    HANDLE hDlg,
    WPARAM wParam,
    LPARAM lParam,
    POPENFILEINFO pOFI,
    BOOL bSave);

BOOL
UpdateListBoxes(
    HWND hDlg,
    POPENFILEINFO pOFI,
    LPTSTR lpszFilter,
    WORD wMask);

BOOL
OKButtonPressed(
    HWND hDlg,
    POPENFILEINFO pOFI,
    BOOL bSave);

BOOL
MultiSelectOKButton(
    HWND hDlg,
    POPENFILEINFO pOFI,
    BOOL bSave);

LRESULT WINAPI
dwOKSubclass(
    HWND hOK,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam);

LRESULT WINAPI
dwLBSubclass(
    HWND hLB,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam);

int
InvalidFileWarning(
    HWND hDlg,
    LPTSTR szFile,
    DWORD wErrCode,
    UINT mbType);

VOID
MeasureItem(
    HWND hDlg,
    LPMEASUREITEMSTRUCT mis);

int
Signum(
    int nTest);

VOID
DrawItem(
    POPENFILEINFO pOFI,
    HWND hDlg,
    WPARAM wParam,
    LPDRAWITEMSTRUCT lpdis,
    BOOL bSave);

BOOL
SpacesExist(
    LPTSTR szFileName);

void
StripFileName(
    HANDLE hDlg,
    BOOL bWowApp);

LPTSTR
lstrtok(
    LPTSTR lpStr,
    LPCTSTR lpDelim);

LPTSTR
ChopText(
    HWND hwndDlg,
    int idStatic,
    LPTSTR lpch);

BOOL
FillOutPath(
    HWND hList,
    POPENFILEINFO pOFI);

BOOL
ShortenThePath(
    LPTSTR pPath);

int
FListAll(
    POPENFILEINFO pOFI,
    HWND hDlg,
    LPTSTR szSpec);

int
ChangeDir(
    HWND hDlg,
    LPCTSTR lpszDir,
    BOOL bForce,
    BOOL bError);

BOOL
IsFileSystemCasePreserving(
    LPTSTR lpszDisk);

BOOL
IsLFNDriveX(
    HWND hDlg,
    LPTSTR szPath);

int
DiskAddedPreviously(
    TCHAR wcDrive,
    LPTSTR lpszName);

int
AddDisk(
    TCHAR wcDrive,
    LPTSTR lpName,
    LPTSTR lpProvider,
    DWORD dwType);

VOID
EnableDiskInfo(
    BOOL bValid,
    BOOL bDoUnc);

VOID
FlushDiskInfoToCmb2();

BOOL
CallNetDlg(
    HWND hWnd);

UINT
GetDiskType(
    LPTSTR lpszDisk);

DWORD
GetUNCDirectoryFromLB(
    HWND hDlg,
    WORD nLB,
    POPENFILEINFO pOFI);

VOID
SelDisk(
    HWND hDlg,
    LPTSTR lpszDisk);

VOID
LNDSetEvent(
    HWND hDlg);

VOID
UpdateLocalDrive(
    LPTSTR szDrive,
    BOOL bGetVolName);

VOID
GetNetDrives(
    DWORD dwScope);

VOID
ListNetDrivesHandler();

VOID
LoadDrives(
    HWND hDlg);

DWORD
GetDiskIndex(
    DWORD dwDriveType);

VOID
CleanUpFile();

VOID
FileOpenAbort();

VOID
TermFile();


#ifdef UNICODE
//VOID                                 // prototype in fileopen.h
//ThunkOpenFileNameA2WDelayed(
//    POPENFILEINFO pOFI);

//BOOL                                 // prototype in fileopen.h
//ThunkOpenFileNameA2W(
//    POPENFILEINFO pOFI);

//BOOL                                 // prototype in fileopen.h
//ThunkOpenFileNameW2A(
//    POPENFILEINFO pOFI);

BOOL
GenericGetFileNameA(
    LPOPENFILENAMEA pOFNA,
    DLGPROC qfnDlgProc);

LPWSTR
ThunkANSIStrToWIDE(
    LPWSTR pDestW,
    LPSTR pSrcA,
    int cChars);

LPWSTR
ThunkMultiANSIStrToWIDE(
    LPWSTR pDestW,
    LPSTR pSrcA,
    int cChars);

BOOL
Multi_strcpyAtoW(
    LPWSTR pDestW,
    LPCSTR pSrcA,
    int cChars);

INT
Multi_strlenA(
    LPCSTR str);

#endif



#ifndef SheChangeDirEx
#undef SheChangeDirEx
#define SheChangeDirEx SetCurrentDirectory
#endif





#ifdef UNICODE

////////////////////////////////////////////////////////////////////////////
//
//  GetFileTitleA
//
//  ANSI entry point for GetFileTitle when this code is built UNICODE.
//
////////////////////////////////////////////////////////////////////////////

SHORT WINAPI GetFileTitleA(
    LPCSTR lpszFileA,
    LPSTR lpszTitleA,
    WORD cbBuf)
{
    LPWSTR lpszFileW;
    LPWSTR lpszTitleW;
    BOOL fResult;
    DWORD cbLen;

    //
    //  Init File string.
    //
    if (lpszFileA)
    {
        cbLen = lstrlenA(lpszFileA) + 1;
        if (!(lpszFileW = (LPWSTR)LocalAlloc(LPTR, (cbLen * sizeof(WCHAR)))))
        {
            StoreExtendedError(CDERR_MEMALLOCFAILURE);
            return (FALSE);
        }
        else
        {
            SHAnsiToUnicode((LPSTR)lpszFileA,lpszFileW,cbLen );
        }
    }
    else
    {
        lpszFileW = NULL;
    }

    if (!(lpszTitleW = (LPWSTR)LocalAlloc(LPTR, (cbBuf * sizeof(WCHAR)))))
    {
        StoreExtendedError(CDERR_MEMALLOCFAILURE);
        if (lpszFileW)
        {
            LocalFree(lpszFileW);
        }
        return (FALSE);
    }

    if (!(fResult = GetFileTitleW(lpszFileW, lpszTitleW, cbBuf)))
    {
        SHUnicodeToAnsi(lpszTitleW,lpszTitleA,cbBuf);
    }
    else if (fResult > 0)
    {
        //
        //  Buffer is too small - Ansi size needed (including null terminator).
        //  Get the offset to the filename.
        //
        SHORT nNeeded = (SHORT)(INT)LOWORD(ParseFile(lpszFileW, TRUE, FALSE, FALSE));
        LPSTR lpA = (LPSTR)lpszFileA;

        lpA += WideCharToMultiByte( CP_ACP,
                                    0,
                                    lpszFileW,
                                    nNeeded,
                                    NULL,
                                    0,
                                    NULL,
                                    NULL );

        fResult = lstrlenA(lpA) + 1;
        if (fResult <= cbBuf)
        {
            lstrcpyA(lpszTitleA, lpA);
            fResult = 0;
        }
    }

    //
    //  Clean up memory.
    //
    LocalFree(lpszTitleW);

    if (lpszFileW)
    {
        LocalFree(lpszFileW);
    }

    return ((SHORT)fResult);
}

#else

////////////////////////////////////////////////////////////////////////////
//
//  GetFileTitleW
//
//  Stub UNICODE function for GetFileTitle when this code is built ANSI.
//
////////////////////////////////////////////////////////////////////////////

SHORT WINAPI GetFileTitleW(
    LPCWSTR lpszFileW,
    LPWSTR lpszTitleW,
    WORD cbBuf)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
}

#endif


////////////////////////////////////////////////////////////////////////////
//
//  GetFileTitle
//
//  The GetFileTitle function returns the name of the file identified
//  by the lpCFile parameter.  This is useful if the file name was
//  received via some method other than GetOpenFileName
//  (e.g. command line, drag drop).
//
//  Returns:  0 on success
//            < 0, Parsing failure (invalid file name)
//            > 0, buffer too small, size needed (including NULL terminator)
//
////////////////////////////////////////////////////////////////////////////

SHORT WINAPI GetFileTitle(
    LPCTSTR lpCFile,
    LPTSTR lpTitle,
    WORD cbBuf)
{
    LPTSTR lpFile;
    DWORD cbLen;
    SHORT fResult;

    //
    //  Init File string.
    //
    if (lpCFile)
    {
        cbLen = lstrlen(lpCFile) + 1;
        if (!(lpFile = (LPTSTR)LocalAlloc(LPTR, (cbLen * sizeof(TCHAR)))))
        {
            StoreExtendedError(CDERR_MEMALLOCFAILURE);
            return (FALSE);
        }
        else
        {
            lstrcpy(lpFile, lpCFile);
        }
    }
    else
    {
        lpFile = NULL;
    }

    fResult = GetFileTitleX(lpFile, lpTitle, cbBuf);

    //
    //  Clean up memory.
    //
    if (lpFile)
    {
        LocalFree(lpFile);
    }

    return (fResult);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetFileTitleX
//
//  Worker routine for the GetFileTitle api.
//
//  Assumes:  lpszFile  points to NULL terminated DOS filename (may have path)
//            lpszTitle points to buffer to receive NULL terminated file title
//            wBufSize  is the size of buffer pointed to by lpszTitle
//
//  Returns:  0 on success
//            < 0, Parsing failure (invalid file name)
//            > 0, buffer too small, size needed (including NULL terminator)
//
////////////////////////////////////////////////////////////////////////////

SHORT GetFileTitleX(
    LPTSTR lpszFile,
    LPTSTR lpszTitle,
    WORD wBufSize)
{
    SHORT nNeeded;
    LPTSTR lpszPtr;

    //
    //  New 32 bit apps will get a title based on the user's preferences.
    //
    if ((GetProcessVersion(0) >= 0x040000) && !(CDGetAppCompatFlags() & CDACF_FILETITLE))
    {
        SHFILEINFO info;
        DWORD_PTR result;


        if (!lpszFile || !*lpszFile)
        {
            return (PARSE_EMPTYSTRING);
        }

        //
        //  If we have a root directory name (eg. c:\), then we need to go
        //  to the old implementation so that it will return -1.
        //  SHGetFileInfo will return the display name for the directory
        //  (which is the volume name).  This is incompatible with Win95
        //  and previous versions of NT.
        //
        if ((lstrlen(lpszFile) != 3) ||
            (lpszFile[1] != CHAR_COLON) || (!ISBACKSLASH(lpszFile, 2)))
        {
            result = SHGetFileInfo( lpszFile,
                                    FILE_ATTRIBUTE_NORMAL,
                                    &info,
                                    sizeof(info),
                                    SHGFI_DISPLAYNAME | SHGFI_USEFILEATTRIBUTES );

            if (result && (*info.szDisplayName))
            {
                UINT uDisplayLen = lstrlen(info.szDisplayName);

                //
                //  If no buffer or insufficient size, return the required chars.
                //  Original GetFileTitle API did not copy on failure.
                //
                if (!lpszTitle || (uDisplayLen >= (UINT)wBufSize))
                {
                    return ( (SHORT)(uDisplayLen + 1) );
                }

                //
                //  We already know it fits, so we don't need lstrcpyn.
                //
                lstrcpy(lpszTitle, info.szDisplayName);
                return (0);
            }
        }
    }

    //
    //  Use the old implementation.
    //
    nNeeded = (SHORT)(int)LOWORD(ParseFile(lpszFile, TRUE, FALSE, FALSE));
    if (nNeeded >= 0)
    {
        //
        //  Is the filename valid?
        //
        lpszPtr = lpszFile + nNeeded;
        if ((nNeeded = (SHORT)lstrlen(lpszPtr) + 1) <= (int)wBufSize)
        {
            //
            //  ParseFile() fails if wildcards in directory, but OK if in name.
            //  Since they arent OK here, the check is needed here.
            //
            if (StrChr(lpszPtr, CHAR_STAR) || StrChr(lpszPtr, CHAR_QMARK))
            {
                nNeeded = PARSE_WILDCARDINFILE;
            }
            else
            {
                lstrcpy(lpszTitle, lpszPtr);

                //
                //  Remove trailing spaces.
                //
                lpszPtr = lpszTitle + lstrlen(lpszTitle) - 1;
                while (*lpszPtr && *lpszPtr == CHAR_SPACE)
                {
                    *lpszPtr-- = CHAR_NULL;
                }

                nNeeded = 0;
            }
        }
    }

    return (nNeeded);
}


#ifdef UNICODE

////////////////////////////////////////////////////////////////////////////
//
//  GetOpenFileNameA
//
//  ANSI entry point for GetOpenFileName when this code is built UNICODE.
//
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI GetOpenFileNameA(
    LPOPENFILENAMEA pOFNA)
{
    if (!pOFNA)
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    return ( GenericGetFileNameA(pOFNA, FileOpenDlgProc) );
}

#else

////////////////////////////////////////////////////////////////////////////
//
//  GetOpenFileNameW
//
//  Stub UNICODE function for GetOpenFileName when this code is built ANSI.
//
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI GetOpenFileNameW(
    LPOPENFILENAMEW pOFNW)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
}

#endif


////////////////////////////////////////////////////////////////////////////
//
//  GetOpenFileName
//
//  The GetOpenFileName function creates a system-defined dialog box
//  that enables the user to select a file to open.
//
//  Returns:  TRUE    if user specified name
//            FALSE   if not
//
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI GetOpenFileName(
    LPOPENFILENAME pOFN)
{
    OPENFILEINFO OFI;

    ZeroMemory(&OFI, sizeof(OPENFILEINFO));

    if (!pOFN)
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    OFI.pOFN = pOFN;
    OFI.ApiType = COMDLG_WIDE;
    OFI.iVersion = OPENFILEVERSION;

    return (GetFileName(&OFI, FileOpenDlgProc));
}


#ifdef UNICODE

////////////////////////////////////////////////////////////////////////////
//
//  GetSaveFileNameA
//
//  ANSI entry point for GetSaveFileName when this code is built UNICODE.
//
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI GetSaveFileNameA(
    LPOPENFILENAMEA pOFNA)
{
    return (GenericGetFileNameA(pOFNA, FileSaveDlgProc));
}

#else

////////////////////////////////////////////////////////////////////////////
//
//  GetSaveFileNameW
//
//  Stub UNICODE function for GetSaveFileName when this code is built ANSI.
//
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI GetSaveFileNameW(
    LPOPENFILENAMEW pOFNW)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
}

#endif


////////////////////////////////////////////////////////////////////////////
//
//  GetSaveFileName
//
//  The GetSaveFileName function creates a system-defined dialog box
//  that enables the user to select a file to save.
//
//  Returns:  TRUE    if user desires to save file and gave a proper name
//            FALSE   if not
//
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI GetSaveFileName(
    LPOPENFILENAME pOFN)
{
    OPENFILEINFO OFI;

    ZeroMemory(&OFI, sizeof(OPENFILEINFO));

    OFI.pOFN = pOFN;
    OFI.ApiType = COMDLG_WIDE;
    OFI.iVersion = OPENFILEVERSION;

    return ( GetFileName(&OFI, FileSaveDlgProc) );
}


////////////////////////////////////////////////////////////////////////////
//
//  GetFileName
//
//  This is the meat of both GetOpenFileName and GetSaveFileName.
//
//  Returns:  TRUE    if user specified name
//            FALSE   if not
//
////////////////////////////////////////////////////////////////////////////

BOOL GetFileName(
    POPENFILEINFO pOFI,
    DLGPROC qfnDlgProc)
{
    LPOPENFILENAME pOFN = pOFI->pOFN;
    LPOPENFILENAME pofnOld = NULL;
    INT_PTR iRet;
    LPTSTR lpDlg;
    HANDLE hRes, hDlgTemplate;
    WORD wErrorMode;
    HDC hdcScreen;
    HBITMAP hbmpTemp;
    LPCURDLG lpCurDlg;
    static fFirstTime = TRUE;
#ifdef UNICODE
    UINT uiWOWFlag = 0;
#endif
    OPENFILENAME ofn = {0};
    LANGID LangID = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL); 

    if (!pOFN)
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    if (pOFN->lStructSize == OPENFILENAME_SIZE_VERSION_400)
    {
        //  lets go ahead and convert it over to the new stuff
        ofn  = *pOFN;
        ofn.lStructSize = SIZEOF(ofn);
        
        //Make sure the additional members are set to NULL
        ofn.pvReserved  = NULL;
        ofn.dwReserved    = 0;
        ofn.FlagsEx      = 0;

        pofnOld = pOFN;
        pOFN = &ofn;
        pOFI->pOFN = pOFN;
        pOFI->iVersion = OPENFILEVERSION_NT4;
    }

    if ((pOFN->lStructSize != sizeof(OPENFILENAME))
       )
    {
        StoreExtendedError(CDERR_STRUCTSIZE);
        return (FALSE);
    }

#ifdef FEATURE_MONIKER_SUPPORT    
    if (pOFN->Flags & OFN_USEMONIKERS)
    {
        if ( !(pOFN->Flags & OFN_EXPLORER)      ||
             (!pOFN->rgpMonikers || !pOFN->cMonikers)
           )
        {
            StoreExtendedError(CDERR_INITIALIZATION);
            return (FALSE);
        }
    }
    else 
#endif // FEATURE_MONIKER_SUPPORT    
    if (pOFN->nMaxFile == 0)
    {
        //Bail out for NULL lpstrFile Only for NT5 and above applications
        if (!IS16BITWOWAPP(pOFN) && (pOFI->iVersion >= OPENFILEVERSION_NT5))
        {
            StoreExtendedError(CDERR_INITIALIZATION);
            return (FALSE);
        }
    }


    //
    //  See if the application should get the new look.
    //
    //  Do not allow the new look if they have hooks, templates, or
    //  multi select without the OFN_EXPLORER bit.
    //
    //  Also don't allow the new look if we are in the context of
    //  a 16 bit process.
    //
    if ( ((pOFN->Flags & OFN_EXPLORER) ||
          (!(pOFN->Flags & (OFN_ENABLEHOOK |
                            OFN_ENABLETEMPLATE |
                            OFN_ENABLETEMPLATEHANDLE |
                            OFN_ALLOWMULTISELECT)))) &&
         (!IS16BITWOWAPP(pOFN)) )
    {
        BOOL fRet;
#ifdef UNICODE
        //
        //  To be used by the thunking routines for multi selection.
        //
        pOFI->bUseNewDialog = TRUE;
#endif
        //
        //  Show the new explorer look.
        //
        StoreExtendedError(0);
        bUserPressedCancel = FALSE;

        if (qfnDlgProc == FileOpenDlgProc)
        {
            fRet = (NewGetOpenFileName(pOFI));
        }
        else
        {
            fRet = (NewGetSaveFileName(pOFI));
        }

        //  copy it back to the original if necessary.
        if (pofnOld)
            CopyMemory(pofnOld, (pOFI->pOFN), pofnOld->lStructSize);

        return fRet;
    }

    if (fFirstTime)
    {
        //
        //  Create a DC that is compatible with the screen and find the
        //  handle of the null bitmap.
        //
        hdcScreen = GetDC(HNULL);
        if (!hdcScreen)
        {
            goto CantInit;
        }
        hdcMemory = CreateCompatibleDC(hdcScreen);
        if (!hdcMemory)
        {
            goto ReleaseScreenDC;
        }

        hbmpTemp = CreateCompatibleBitmap(hdcMemory, 1, 1);
        if (!hbmpTemp)
        {
            goto ReleaseMemDC;
        }
        hbmpOrigMemBmp = SelectObject(hdcMemory, hbmpTemp);
        if (!hbmpOrigMemBmp)
        {
            goto ReleaseMemDC;
        }
        SelectObject(hdcMemory, hbmpOrigMemBmp);
        DeleteObject(hbmpTemp);
        ReleaseDC(HNULL, hdcScreen);

        fFirstTime = FALSE;
    }

    if (pOFN->Flags & OFN_ENABLEHOOK)
    {
        if (!pOFN->lpfnHook)
        {
            StoreExtendedError(CDERR_NOHOOK);
            return (FALSE);
        }
    }
    else
    {
        pOFN->lpfnHook = NULL;
    }

    HourGlass(TRUE);
    StoreExtendedError(0);

    //
    //  Force re-compute for font changes between calls.
    //
    dyItem = dyText = 0;

    bUserPressedCancel = FALSE;

    if (!FSetUpFile())
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        goto TERMINATE;
    }

    if (pOFN->Flags & OFN_ENABLETEMPLATE)
    {
        if (!(hRes = FindResource( pOFN->hInstance,
                                   pOFN->lpTemplateName,
                                   RT_DIALOG )))
        {
            StoreExtendedError(CDERR_FINDRESFAILURE);
            goto TERMINATE;
        }
        if (!(hDlgTemplate = LoadResource(pOFN->hInstance, hRes)))
        {
            StoreExtendedError(CDERR_LOADRESFAILURE);
            goto TERMINATE;
        }
        LangID = GetDialogLanguage(pOFN->hwndOwner, hDlgTemplate);
    }
    else if (pOFN->Flags & OFN_ENABLETEMPLATEHANDLE)
    {
        hDlgTemplate = pOFN->hInstance;
        LangID = GetDialogLanguage(pOFN->hwndOwner, hDlgTemplate);
    }
    else
    {
        if (pOFN->Flags & OFN_ALLOWMULTISELECT)
        {
            lpDlg = MAKEINTRESOURCE(MULTIFILEOPENORD);
        }
        else
        {
            lpDlg = MAKEINTRESOURCE(FILEOPENORD);
        }

        LangID = GetDialogLanguage(pOFN->hwndOwner, NULL);
        if (!(hRes = FindResourceEx(g_hinst, RT_DIALOG, lpDlg, LangID)))
        {
            StoreExtendedError(CDERR_FINDRESFAILURE);
            goto TERMINATE;
        }
        if (!(hDlgTemplate = LoadResource(g_hinst, hRes)))
        {
            StoreExtendedError(CDERR_LOADRESFAILURE);
            goto TERMINATE;
        }
    }

    //
    // Warning! Warning! Warning!
    //
    // We have to set g_tlsLangID before any call for CDLoadString
    //
    TlsSetValue(g_tlsLangID, (LPVOID) LangID);

    //
    //  No kernel network error dialogs.
    //
    wErrorMode = (WORD)SetErrorMode(SEM_NOERROR);
    SetErrorMode(SEM_NOERROR | wErrorMode);

    if (LockResource(hDlgTemplate))
    {
        if (pOFN->Flags & OFN_ENABLEHOOK)
        {
            glpfnFileHook = GETHOOKFN(pOFN);
        }

#ifdef UNICODE
        if (IS16BITWOWAPP(pOFN))
        {
            uiWOWFlag = SCDLG_16BIT;
        }

        iRet = DialogBoxIndirectParamAorW( g_hinst,
                                           (LPDLGTEMPLATE)hDlgTemplate,
                                           pOFN->hwndOwner,
                                           qfnDlgProc,
                                           (DWORD_PTR)pOFI,
                                           uiWOWFlag );
#else
        iRet = DialogBoxIndirectParam( g_hinst,
                                       (LPDLGTEMPLATE)hDlgTemplate,
                                       pOFN->hwndOwner,
                                       qfnDlgProc,
                                       (DWORD)pOFI );
#endif

        if (iRet == -1 || ((iRet == 0) && (!bUserPressedCancel) && (!GetStoredExtendedError())))
        {
            StoreExtendedError(CDERR_DIALOGFAILURE);
        }
        else
        {
            FileOpenAbort();
        }

        glpfnFileHook = 0;
    }
    else
    {
        StoreExtendedError(CDERR_LOCKRESFAILURE);
        goto TERMINATE;
    }

    SetErrorMode(wErrorMode);

    if (lpCurDlg = (LPCURDLG)TlsGetValue(g_tlsiCurDlg))
    {
        // restore the thread list to the previous dialog (if any)
        TlsSetValue(g_tlsiCurDlg, (LPVOID)lpCurDlg->next);
        LocalFree(lpCurDlg->lpstrCurDir);
        LocalFree(lpCurDlg);
    }

TERMINATE:
    //  make sure the original
    if (pofnOld)
        CopyMemory(pofnOld, (pOFI->pOFN), pofnOld->lStructSize);

    CleanUpFile();
    HourGlass(FALSE);
    return (iRet == IDOK);

ReleaseMemDC:
    DeleteDC(hdcMemory);

ReleaseScreenDC:
    ReleaseDC(HNULL, hdcScreen);

CantInit:
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  FileHookCmd
//
//  Called when a hook function processes a WM_COMMAND message.
//  Called by FileOpenDlgProc and FileSaveDlgProc.
//
////////////////////////////////////////////////////////////////////////////

BOOL FileHookCmd(
    HANDLE hDlg,
    WPARAM wParam,
    LPARAM lParam,
    POPENFILEINFO pOFI)
{
    switch (GET_WM_COMMAND_ID(wParam, lParam))
    {
        case ( IDCANCEL ) :
        {
            //
            //  Set global flag stating that the
            //  user pressed cancel.
            //
            bUserPressedCancel = TRUE;

            //  Fall Thru...
        }
        case ( IDOK ) :
        case ( IDABORT ) :
        {
#ifdef UNICODE
            //
            //  Apps that side-effect these messages may
            //  not have their internal unicode strings
            //  updated.  They may also forget to gracefully
            //  exit the network enum'ing worker thread.
            //
            if (pOFI->ApiType == COMDLG_ANSI)
            {
                ThunkOpenFileNameA2W(pOFI);
            }
#endif
            break;
        }
        case ( cmb1 ) :
        case ( cmb2 ) :
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
                case ( MYCBN_DRAW ) :
                case ( MYCBN_LIST ) :
                case ( MYCBN_REPAINT ) :
                case ( MYCBN_CHANGEDIR ) :
                {
                    //
                    //  In case an app has a hook, and returns
                    //  true for processing WM_COMMAND messages,
                    //  we still have to worry about our
                    //  internal message that came through via
                    //  WM_COMMAND.
                    //
                    FileOpenCmd( hDlg,
                                 wParam,
                                 lParam,
                                 pOFI,
                                 FALSE );
                    break;
                }
            }
            break;
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  FileOpenDlgProc
//
//  Gets the name of a file to open from the user.
//
//  edt1 = file name
//  lst1 = list of files in current directory matching current pattern
//  cmb1 = lists file patterns
//  stc1 = is current directory
//  lst2 = lists directories on current drive
//  cmb2 = lists drives
//  IDOK = is Open pushbutton
//  IDCANCEL = is Cancel pushbutton
//  chx1 = is for opening read only files
//
//  Returns the normal dialog proc values.
//
////////////////////////////////////////////////////////////////////////////

BOOL_PTR CALLBACK FileOpenDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    POPENFILEINFO pOFI;
    BOOL_PTR bRet, bHookRet;


    if (pOFI = (POPENFILEINFO)GetProp(hDlg, FILEPROP))
    {
        if (pOFI->pOFN->lpfnHook)
        {
            LPOFNHOOKPROC lpfnHook = GETHOOKFN(pOFI->pOFN);

            bHookRet = (*lpfnHook)(hDlg, wMsg, wParam, lParam);

            if (bHookRet)
            {

                if (wMsg == WM_COMMAND)
                {
                    return (FileHookCmd(hDlg, wParam, lParam, pOFI));
                }

                return (bHookRet);
            }
        }
    }
    else if (glpfnFileHook &&
             (wMsg != WM_INITDIALOG) &&
             (bHookRet = (*glpfnFileHook)(hDlg, wMsg, wParam, lParam)))
    {
        return (bHookRet);
    }

    switch (wMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            pOFI = (POPENFILEINFO)lParam;

            SetProp(hDlg, FILEPROP, (HANDLE)pOFI);
            glpfnFileHook = 0;

            //
            //  If we are being called from a Unicode app, turn off
            //  the ES_OEMCONVERT style on the filename edit control.
            //
//          if (pOFI->ApiType == COMDLG_WIDE)
            {
                LONG lStyle;
                HWND hEdit = GetDlgItem(hDlg, edt1);

                //
                //  Grab the window style.
                //
                lStyle = GetWindowLong(hEdit, GWL_STYLE);

                //
                //  If the window style bits include ES_OEMCONVERT,
                //  remove this flag and reset the style.
                //
                if (lStyle & ES_OEMCONVERT)
                {
                    lStyle &= ~ES_OEMCONVERT;
                    SetWindowLong(hEdit, GWL_STYLE, lStyle);
                }
            }

            bInitializing = TRUE;
            bRet = InitFileDlg(hDlg, wParam, pOFI);
            bInitializing = FALSE;

            HourGlass(FALSE);
            return (bRet);
            break;
        }
        case ( WM_ACTIVATE ) :
        {
            if (!bInChildDlg)
            {
                if (bFirstTime == TRUE)
                {
                    bFirstTime = FALSE;
                }
                else if (wParam)
                {
                    //
                    //  If becoming active.
                    //
                    LNDSetEvent(hDlg);
                }
            }
            return (FALSE);
            break;
        }
        case ( WM_MEASUREITEM ) :
        {
            MeasureItem(hDlg, (LPMEASUREITEMSTRUCT)lParam);
            break;
        }
        case ( WM_DRAWITEM ) :
        {
            if (wNoRedraw < 2)
            {
                DrawItem(pOFI, hDlg, wParam, (LPDRAWITEMSTRUCT)lParam, FALSE);
            }
            break;
        }
        case ( WM_SYSCOLORCHANGE ) :
        {
            SetRGBValues();
            LoadDirDriveBitmap();
            break;
        }
        case ( WM_COMMAND ) :
        {
            return (FileOpenCmd(hDlg, wParam, lParam, pOFI, FALSE));
            break;
        }
        case ( WM_SETFOCUS ) :
        {
            //
            //  This logic used to be in CBN_SETFOCUS in fileopencmd,
            //  but CBN_SETFOCUS is called whenever there is a click on
            //  the List Drives combo.  This causes the worker thread
            //  to start up and flicker when the combo box is refreshed.
            //
            //  But, refreshes are only needed when someone focuses out of
            //  the common dialog and then back in (unless someone is logged
            //  in remote, or there is a background thread busy connecting!)
            //  so fix the flicker by moving the logic here.
            //
            if (!wNoRedraw)
            {
                LNDSetEvent(hDlg);
            }

            return (FALSE);
            break;
        }
        case ( WM_HELP ) :
        {
            if (IsWindowEnabled(hDlg))
            {
                WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                         NULL,
                         HELP_WM_HELP,
                         (ULONG_PTR)(LPTSTR)aFileOpenHelpIDs );
            }
            break;
        }
        case ( WM_CONTEXTMENU ) :
        {
            if (IsWindowEnabled(hDlg))
            {
                WinHelp( (HWND)wParam,
                         NULL,
                         HELP_CONTEXTMENU,
                         (ULONG_PTR)(LPVOID)aFileOpenHelpIDs );
            }
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}

////////////////////////////////////////////////////////////////////////////
//
//  FileSaveDlgProc
//
//  Obtains the name of the file that the user wants to save.
//
//  Returns the normal dialog proc values.
//
////////////////////////////////////////////////////////////////////////////

BOOL_PTR CALLBACK FileSaveDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    POPENFILEINFO pOFI;
    BOOL_PTR bRet, bHookRet;
    TCHAR szTitle[cbCaption];


    if (pOFI = (POPENFILEINFO)GetProp(hDlg, FILEPROP))
    {
        if (pOFI->pOFN->lpfnHook)
        {
            LPOFNHOOKPROC lpfnHook = GETHOOKFN(pOFI->pOFN);

            bHookRet = (*lpfnHook)(hDlg, wMsg, wParam, lParam);

            if (bHookRet)
            {
                if (wMsg == WM_COMMAND)
                {
                    return (FileHookCmd(hDlg, wParam, lParam, pOFI));
                }

                return (bHookRet);
            }
        }
    }
    else if (glpfnFileHook &&
             (wMsg != WM_INITDIALOG) &&
             (bHookRet = (*glpfnFileHook)(hDlg, wMsg, wParam, lParam)))
        {
            return (bHookRet);
        }
    
    switch (wMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            pOFI = (POPENFILEINFO)lParam;
            if (!(pOFI->pOFN->Flags &
                  (OFN_ENABLETEMPLATE | OFN_ENABLETEMPLATEHANDLE)))
            {
                CDLoadString(g_hinst, iszFileSaveTitle, szTitle, cbCaption);
                SetWindowText(hDlg, szTitle);
                CDLoadString(g_hinst, iszSaveFileAsType, szTitle, cbCaption);
                SetDlgItemText(hDlg, stc2, szTitle);
            }
            glpfnFileHook = 0;
            SetProp(hDlg, FILEPROP, (HANDLE)pOFI);

            //
            //  If we are being called from a Unicode app, turn off
            //  the ES_OEMCONVERT style on the filename edit control.
            //
//          if (pOFI->ApiType == COMDLG_WIDE)
            {
                LONG lStyle;
                HWND hEdit = GetDlgItem(hDlg, edt1);

                //
                //  Grab the window style.
                //
                lStyle = GetWindowLong(hEdit, GWL_STYLE);

                //
                //  If the window style bits include ES_OEMCONVERT,
                //  remove this flag and reset the style.
                //
                if (lStyle & ES_OEMCONVERT)
                {
                    lStyle &= ~ES_OEMCONVERT;
                    SetWindowLong (hEdit, GWL_STYLE, lStyle);
                }
            }

            bInitializing = TRUE;
            bRet = InitFileDlg(hDlg, wParam, pOFI);
            bInitializing = FALSE;

            HourGlass(FALSE);
            return (bRet);
            break;
        }
        case ( WM_ACTIVATE ) :
        {
            if (!bInChildDlg)
            {
                if (bFirstTime == TRUE)
                {
                    bFirstTime = FALSE;
                }
                else if (wParam)
                {
                    //
                    //  If becoming active.
                    //
                    if (!wNoRedraw)
                    {
                        LNDSetEvent(hDlg);
                    }
                }
            }
            return (FALSE);
            break;
        }
        case ( WM_MEASUREITEM ) :
        {
            MeasureItem(hDlg, (LPMEASUREITEMSTRUCT)lParam);
            break;
        }
        case ( WM_DRAWITEM ) :
        {
            if (wNoRedraw < 2)
            {
                DrawItem(pOFI, hDlg, wParam, (LPDRAWITEMSTRUCT)lParam, TRUE);
            }
            break;
        }
        case ( WM_SYSCOLORCHANGE ) :
        {
            SetRGBValues();
            LoadDirDriveBitmap();
            break;
        }
        case ( WM_COMMAND ) :
        {
            return (FileOpenCmd(hDlg, wParam, lParam, pOFI, TRUE));
            break;
        }
        case ( WM_SETFOCUS ) :
        {
            //
            //  This logic used to be in CBN_SETFOCUS in fileopencmd,
            //  but CBN_SETFOCUS is called whenever there is a click on
            //  the List Drives combo.  This causes the worker thread
            //  to start up and flicker when the combo box is refreshed.
            //
            //  But, refreshes are only needed when someone focuses out of
            //  the common dialog and then back in (unless someone is logged
            //  in remote, or there is a background thread busy connecting!)
            //  so fix the flicker by moving the logic here.
            //
            if (!wNoRedraw)
            {
                LNDSetEvent(hDlg);
            }

            return (FALSE);
            break;
        }
        case ( WM_HELP ) :
        {
            if (IsWindowEnabled(hDlg))
            {
                WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                         NULL,
                         HELP_WM_HELP,
                         (ULONG_PTR)(LPTSTR)aFileSaveHelpIDs );
            }
            break;
        }
        case ( WM_CONTEXTMENU ) :
        {
            if (IsWindowEnabled(hDlg))
            {
                WinHelp( (HWND)wParam,
                         NULL,
                         HELP_CONTEXTMENU,
                         (ULONG_PTR)(LPVOID)aFileSaveHelpIDs );
            }
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitFileDlg
//
////////////////////////////////////////////////////////////////////////////

BOOL_PTR InitFileDlg(
    HWND hDlg,
    WPARAM wParam,
    POPENFILEINFO pOFI)
{
    DWORD lRet, nFilterIndex;
    LPOPENFILENAME pOFN = pOFI->pOFN;
    int nFileOffset, nExtOffset;
    RECT rRect;
    RECT rLbox;
    BOOL_PTR bRet;

    if (!InitTlsValues(pOFI))
    {
        //
        //  The extended error is set inside of the above call.
        //
        EndDialog(hDlg, FALSE);
        return (FALSE);
    }

    lpLBProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hDlg, lst2), GWLP_WNDPROC);
    lpOKProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hDlg, IDOK), GWLP_WNDPROC);

    if (!lpLBProc || !lpOKProc)
    {
        StoreExtendedError(FNERR_SUBCLASSFAILURE);
        EndDialog(hDlg, FALSE);
        return (FALSE);
    }

    //
    //  Save original directory for later restoration if necessary.
    //
    *pOFI->szCurDir = 0;
    GetCurrentDirectory(MAX_FULLPATHNAME + 1, pOFI->szCurDir);

    //
    //  Check out if the filename contains a path.  If so, override whatever
    //  is contained in lpstrInitialDir.  Chop off the path and put up only
    //  the filename.
    //
    if ( pOFN->lpstrFile &&
         *pOFN->lpstrFile &&
         !(pOFN->Flags & OFN_NOVALIDATE) )
    {
        if (DBL_BSLASH(pOFN->lpstrFile + 2) &&
            ((*(pOFN->lpstrFile + 1) == CHAR_COLON)))
        {
            lstrcpy(pOFN->lpstrFile , pOFN->lpstrFile + sizeof(TCHAR));
        }

        lRet = ParseFile(pOFN->lpstrFile, TRUE, IS16BITWOWAPP(pOFN), FALSE);
        nFileOffset = (int)(SHORT)LOWORD(lRet);
        nExtOffset  = (int)(SHORT)HIWORD(lRet);

        //
        //  Is the filename invalid?
        //
        if ( (nFileOffset < 0) &&
             (nFileOffset != PARSE_EMPTYSTRING) &&
             (pOFN->lpstrFile[nExtOffset] != CHAR_SEMICOLON) )
        {
            StoreExtendedError(FNERR_INVALIDFILENAME);
            EndDialog(hDlg, FALSE);
            return (FALSE);
        }
    }

    pOFN->Flags &= ~(OFN_FILTERDOWN | OFN_DRIVEDOWN | OFN_DIRSELCHANGED);

    pOFI->idirSub = 0;

    if (!(pOFN->Flags & OFN_SHOWHELP))
    {
        HWND hHelp;

        EnableWindow(hHelp = GetDlgItem(hDlg, pshHelp), FALSE);

        //
        //  Move the window out of this spot so that no overlap will be
        //  detected.
        //
        MoveWindow(hHelp, -8000, -8000, 20, 20, FALSE);
        ShowWindow(hHelp, SW_HIDE);
    }

    if (pOFN->Flags & OFN_CREATEPROMPT)
    {
        pOFN->Flags |= (OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST);
    }
    else if (pOFN->Flags & OFN_FILEMUSTEXIST)
    {
        pOFN->Flags |= OFN_PATHMUSTEXIST;
    }

    if (pOFN->Flags & OFN_HIDEREADONLY)
    {
        HWND hReadOnly;

        EnableWindow(hReadOnly = GetDlgItem(hDlg, chx1), FALSE);

        //
        //  Move the window out of this spot so that no overlap will be
        //  detected.
        //
        MoveWindow(hReadOnly, -8000, -8000, 20, 20, FALSE);
        ShowWindow(hReadOnly, SW_HIDE);
    }
    else
    {
        CheckDlgButton(hDlg, chx1, (pOFN->Flags & OFN_READONLY) != 0);
    }

    SendDlgItemMessage(hDlg, edt1, EM_LIMITTEXT, (WPARAM)MAX_PATH, 0L);

    //
    //  Insert file specs into cmb1.
    //  Custom filter first.
    //  Must also check if filter contains anything.
    //
    if ( pOFN->lpstrFile &&
         (StrChr(pOFN->lpstrFile, CHAR_STAR) ||
          StrChr(pOFN->lpstrFile, CHAR_QMARK)) )
    {
        lstrcpy(pOFI->szLastFilter, pOFN->lpstrFile);
    }
    else
    {
        pOFI->szLastFilter[0] = CHAR_NULL;
    }

    if (pOFN->lpstrCustomFilter && *pOFN->lpstrCustomFilter)
    {
        SHORT nLength;

        SendDlgItemMessage( hDlg,
                            cmb1,
                            CB_INSERTSTRING,
                            0,
                            (LONG_PTR)pOFN->lpstrCustomFilter );

        nLength = (SHORT)(lstrlen(pOFN->lpstrCustomFilter) + 1);
        SendDlgItemMessage( hDlg,
                            cmb1,
                            CB_SETITEMDATA,
                            0,
                            (LONG)(nLength) );

        SendDlgItemMessage( hDlg,
                            cmb1,
                            CB_LIMITTEXT,
                            (WPARAM)(pOFN->nMaxCustFilter),
                            0L );

        if (pOFI->szLastFilter[0] == CHAR_NULL)
        {
            lstrcpy(pOFI->szLastFilter, pOFN->lpstrCustomFilter + nLength);
        }
    }
    else
    {
        //
        //  Given no custom filter, the index will be off by one.
        //
        if (pOFN->nFilterIndex != 0)
        {
            pOFN->nFilterIndex--;
        }
    }

    //
    //  Listed filters next.
    //
    if (pOFN->lpstrFilter && *pOFN->lpstrFilter)
    {
        if (pOFN->nFilterIndex > InitFilterBox(hDlg, pOFN->lpstrFilter))
        {
            pOFN->nFilterIndex = 0;
        }
    }
    else
    {
        pOFN->nFilterIndex = 0;
    }
    pOFI->szSpecCur[0] = CHAR_NULL;

    //
    //  If an entry exists, select the one indicated by nFilterIndex.
    //
    if ((pOFN->lpstrFilter && *pOFN->lpstrFilter) ||
        (pOFN->lpstrCustomFilter && *pOFN->lpstrCustomFilter))
    {
        LPCTSTR lpFilter;

        SendDlgItemMessage( hDlg,
                            cmb1,
                            CB_SETCURSEL,
                            (WPARAM)(pOFN->nFilterIndex),
                            0L );

        nFilterIndex = pOFN->nFilterIndex;
        SendMessage( hDlg,
                     WM_COMMAND,
                     GET_WM_COMMAND_MPS( cmb1,
                                         GetDlgItem(hDlg, cmb1),
                                         MYCBN_DRAW ) );
        pOFN->nFilterIndex = nFilterIndex;

        if (pOFN->nFilterIndex ||
            !(pOFN->lpstrCustomFilter && *pOFN->lpstrCustomFilter))
        {
            lpFilter = pOFN->lpstrFilter +
                       SendDlgItemMessage( hDlg,
                                           cmb1,
                                           CB_GETITEMDATA,
                                           (WPARAM)pOFN->nFilterIndex,
                                           0L );
        }
        else
        {
            lpFilter = pOFN->lpstrCustomFilter +
                       lstrlen(pOFN->lpstrCustomFilter) + 1;
        }
        if (*lpFilter)
        {
            TCHAR szText[MAX_FULLPATHNAME];

            // BUGBUG: lpFilter can be longer than MAX_FULLPATHNAME!!!
            lstrcpy(szText, lpFilter);

            //
            //  Filtering is case-insensitive.
            //
            CharLower(szText);

            if (pOFI->szLastFilter[0] == CHAR_NULL)
            {
                lstrcpy(pOFI->szLastFilter, szText);
            }

            if (!(pOFN->lpstrFile && *pOFN->lpstrFile))
            {
                SetDlgItemText(hDlg, edt1, szText);
            }
        }
    }

    InitCurrentDisk(hDlg, pOFI, cmb2);

    bFirstTime = TRUE;
    bInChildDlg = FALSE;

    SendMessage( hDlg,
                 WM_COMMAND,
                 GET_WM_COMMAND_MPS(cmb2, GetDlgItem(hDlg, cmb2), MYCBN_DRAW) );
    SendMessage( hDlg,
                 WM_COMMAND,
                 GET_WM_COMMAND_MPS(cmb2, GetDlgItem(hDlg, cmb2), MYCBN_LIST) );

    if (pOFN->lpstrFile && *pOFN->lpstrFile)
    {
        TCHAR szText[MAX_FULLPATHNAME];

        lRet = ParseFile( pOFN->lpstrFile,
                          IsLFNDriveX(hDlg, pOFN->lpstrFile),
                          IS16BITWOWAPP(pOFN),
                          FALSE );
        nFileOffset = (int)(SHORT)LOWORD(lRet);
        nExtOffset  = (int)(SHORT)HIWORD(lRet);

        //
        //  Is the filename invalid?
        //
        if ( !(pOFN->Flags & OFN_NOVALIDATE) &&
             (nFileOffset < 0) &&
             (nFileOffset != PARSE_EMPTYSTRING) &&
             (pOFN->lpstrFile[nExtOffset] != CHAR_SEMICOLON) )
        {
            StoreExtendedError(FNERR_INVALIDFILENAME);
            EndDialog(hDlg, FALSE);
            return (FALSE);
        }
        lstrcpy(szText, pOFN->lpstrFile);
        SetDlgItemText(hDlg, edt1, szText);
    }

    SetWindowLongPtr(GetDlgItem(hDlg, lst2), GWLP_WNDPROC, (LONG_PTR)dwLBSubclass);
    SetWindowLongPtr(GetDlgItem(hDlg, IDOK), GWLP_WNDPROC, (LONG_PTR)dwOKSubclass);

    if (pOFN->lpstrTitle && *pOFN->lpstrTitle)
    {
        SetWindowText(hDlg, pOFN->lpstrTitle);
    }

    //
    //  By setting dyText to rRect.bottom/8, dyText defaults to 8 items showing
    //  in the listbox.  This only matters if the applications hook function
    //  steals all WM_MEASUREITEM messages.  Otherwise, dyText will be set in
    //  the MeasureItem() routine.  Check for !dyItem in case message ordering
    //  has already sent WM_MEASUREITEM and dyText is already initialized.
    //
    if (!dyItem)
    {
        GetClientRect(GetDlgItem(hDlg, lst1), (LPRECT) &rRect);
        if (!(dyText = (rRect.bottom / 8)))
        {
            //
            //  If no size to rectangle.
            //
            dyText = 8;
        }
    }

    //  The template has changed to make it extremely clear that
    //  this is not a combobox, but rather an edit control and a listbox.  The
    //  problem is that the new templates try to align the edit box and listbox.
    //  Unfortunately, when listboxes add borders, they expand beyond their
    //  borders.  When edit controls add borders, they stay within their
    //  borders.  This makes it impossible to align the two controls strictly
    //  within the template.  The code below will align the controls, but only
    //  if they are using the standard dialog template.
    //
    if (!(pOFN->Flags & (OFN_ENABLETEMPLATE | OFN_ENABLETEMPLATEHANDLE)))
    {
        GetWindowRect(GetDlgItem(hDlg, lst1), (LPRECT)&rLbox);
        GetWindowRect(GetDlgItem(hDlg, edt1), (LPRECT)&rRect);
        rRect.left = rLbox.left;
        rRect.right = rLbox.right;
        MapWindowPoints(NULL, hDlg, (LPPOINT)&rRect, 2);
        SetWindowPos( GetDlgItem(hDlg, edt1),
                      0,
                      rRect.left,
                      rRect.top,
                      rRect.right - rRect.left,
                      rRect.bottom - rRect.top,
                      SWP_NOZORDER );
    }

    if (pOFN->lpfnHook)
    {
        LPOFNHOOKPROC lpfnHook = GETHOOKFN(pOFN);

#ifdef UNICODE
        if (pOFI->ApiType == COMDLG_ANSI)
        {
            ThunkOpenFileNameW2A(pOFI);
            bRet = ((*lpfnHook)( hDlg,
                                 WM_INITDIALOG,
                                 wParam,
                                 (LPARAM)pOFI->pOFNA ));
            //
            //  Strange win 31 example uses lCustData to
            //  hold a temporary variable that it passes back to
            //  calling function.
            //
            ThunkOpenFileNameA2W(pOFI);
        }
        else
#endif
        {
            bRet = ((*lpfnHook)( hDlg,
                                 WM_INITDIALOG,
                                 wParam,
                                 (LPARAM)pOFN ));
        }
    }
    else
    {
#ifdef UNICODE
        //
        //  Have to thunk A version even when there isn't a hook proc so it
        //  doesn't reset W version on delayed thunk back.
        //
        if (pOFI->ApiType == COMDLG_ANSI)
        {
            pOFI->pOFNA->Flags = pOFN->Flags;
        }
#endif
        bRet = TRUE;
    }

    //
    //  At first, assume there is net support !
    //
    if ((pOFN->Flags & OFN_NONETWORKBUTTON))
    {
        HWND hNet;

        if (hNet = GetDlgItem(hDlg, psh14))
        {
            EnableWindow(hNet = GetDlgItem(hDlg, psh14), FALSE);

            ShowWindow(hNet, SW_HIDE);
        }
    }
    else
    {
        AddNetButton( hDlg,
                      ((pOFN->Flags & OFN_ENABLETEMPLATE)
                          ? pOFN->hInstance
                          : g_hinst),
                      FILE_BOTTOM_MARGIN,
                      (pOFN->Flags & (OFN_ENABLETEMPLATE |
                                       OFN_ENABLETEMPLATEHANDLE))
                          ? FALSE
                          : TRUE,
                      (pOFN->Flags & OFN_NOLONGNAMES)
                          ? FALSE
                          : TRUE,
                      FALSE);
    }

    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitTlsValues
//
////////////////////////////////////////////////////////////////////////////

int InitTlsValues(
    POPENFILEINFO pOFI)
{
    //
    //  As long as we do not call TlsGetValue before this,
    //  everything should be ok.
    //
    LPCURDLG lpCurDlg, lpPrevDlg;
    DWORD    dwError;
    LPTSTR   lpCurDir;

    if (dwNumDlgs == MAX_THREADS)
    {
        dwError = CDERR_INITIALIZATION;
        goto ErrorExit0;
    }

    // alloc for the current directory
    if (lpCurDir = (LPTSTR)LocalAlloc(LPTR, CCHNETPATH * sizeof(TCHAR)))
    {
        GetCurrentDirectory(CCHNETPATH, lpCurDir);

        if ( (pOFI->pOFN->Flags & OFN_ALLOWMULTISELECT) &&
             (StrChr(lpCurDir, CHAR_SPACE)) )
        {
            GetShortPathName(lpCurDir, lpCurDir, CCHNETPATH);
        }

    }
    else
    {
        dwError = CDERR_MEMALLOCFAILURE;
        goto ErrorExit0;
    }

    // add a CurDlg struct to the list for this thread
    if (lpCurDlg = (LPCURDLG)LocalAlloc(LPTR, sizeof(CURDLG)))
    {
        // get start of CURDLG list for this thread
        // Note: lpPrevDlg will be NULL if there wasn't a previous dialog
        lpPrevDlg = (LPCURDLG)TlsGetValue(g_tlsiCurDlg);

        // make sure TlsGetValue() actually succeeded (a NULL return could
        // mean there wasn't a previous dialog in the list)
        if (GetLastError() != NO_ERROR)
        {
            dwError = CDERR_INITIALIZATION;
            goto ErrorExit2;
        }

        // push the new dlg to the front of the list
        lpCurDlg->next = lpPrevDlg;

        lpCurDlg->lpstrCurDir = lpCurDir;
        PathAddBackslash(lpCurDlg->lpstrCurDir);

        EnterCriticalSection(&g_csLocal);
        lpCurDlg->dwCurDlgNum = dwNumDlgs++;
        LeaveCriticalSection(&g_csLocal);

        // save the new head of the list for the thread
        if (!TlsSetValue(g_tlsiCurDlg, (LPVOID)lpCurDlg))
        {
            dwError = CDERR_INITIALIZATION;
            goto ErrorExit2;
        }
    }
    else
    {
        dwError = CDERR_MEMALLOCFAILURE;
        goto ErrorExit1;
    }

    return(TRUE);


ErrorExit2:
    LocalFree(lpCurDlg);

ErrorExit1:
    LocalFree(lpCurDir);

ErrorExit0:
    StoreExtendedError(dwError);
    return (FALSE);

}


////////////////////////////////////////////////////////////////////////////
//
//  InitFilterBox
//
//  Places the double null terminated list of filters in the combo box.
//  The list should consist of pairs of null terminated strings, with
//  an additional null terminating the list.
//
////////////////////////////////////////////////////////////////////////////

DWORD InitFilterBox(
    HANDLE hDlg,
    LPCTSTR lpszFilter)
{
    DWORD nOffset = 0;
    DWORD nIndex = 0;
    register WORD nLen;


    while (*lpszFilter)
    {
        //
        //  First string put in as string to show.
        //
        nIndex = (DWORD) SendDlgItemMessage( hDlg,
                                             cmb1,
                                             CB_ADDSTRING,
                                             0,
                                             (LPARAM)lpszFilter );
        nLen = (WORD)(lstrlen(lpszFilter) + 1);
        (LPTSTR)lpszFilter += nLen;
        nOffset += nLen;

        //
        //  Second string put in as itemdata.
        //
        SendDlgItemMessage( hDlg,
                            cmb1,
                            CB_SETITEMDATA,
                            (WPARAM)nIndex,
                            nOffset );

        //
        //  Advance to next element.
        //
        nLen = (WORD)(lstrlen(lpszFilter) + 1);
        (LPTSTR)lpszFilter += nLen;
        nOffset += nLen;
    }

    return (nIndex);
}

void TokenizeFilterString(LPTSTR pszFilterString, LPTSTR *ppszFilterArray, int cFilterArray, BOOL bLFN)
{
    LPCTSTR pszDelim = bLFN ? szSemiColonTab : szSemiColonSpaceTab;
    int nFilters = 0;

    //
    //  Find the first filter in the string, and add it to the
    //  array.
    //
    ppszFilterArray[nFilters] = lstrtok(pszFilterString, pszDelim);

    //
    //  Now we are going to loop through all the filters in the string
    //  parsing the one we already have, and then finding the next one
    //  and starting the loop over again.
    //
    while (ppszFilterArray[nFilters] && (nFilters < cFilterArray))
    {
        //
        //  Check to see if the first character is a space.  If so, remove
        //  the spaces, and save the pointer back into the same spot.  We
        //  need to do this because the FindFirstFile/Next api will still
        //  work on filenames that begin with a space since they also
        //  look at the short names.  The short names will begin with the
        //  same first real letter as the long filename.  For example, the
        //  long filename is "  my document" the first letter of this short
        //  name is "m", so searching on "m*.*" or " m*.*" will yield the
        //  same results.
        //
        if (bLFN && (*ppszFilterArray[nFilters] == CHAR_SPACE))
        {
            LPTSTR pszTemp = ppszFilterArray[nFilters];
            while ((*pszTemp == CHAR_SPACE) && *pszTemp)
            {
                pszTemp = CharNext(pszTemp);
            }
            ppszFilterArray[nFilters] = pszTemp;
        }

        //
        //  Ready to move on to the next filter.  Find the next
        //  filter based upon the type of file system we're using.
        //
        ppszFilterArray[++nFilters] = lstrtok(NULL, pszDelim);

        //
        //  In case we found a pointer to NULL, then look for the
        //  next filter.
        //
        while (ppszFilterArray[nFilters] && !*ppszFilterArray[nFilters])
        {
            ppszFilterArray[nFilters] = lstrtok(NULL, pszDelim);
        }
    }
}


BOOL FoundFilterMatch(LPCTSTR pszIn, BOOL bLFN)
{
    TCHAR szFilter[MAX_FULLPATHNAME];
    LPTSTR pszF[MAXFILTERS + 1];
    BOOL fFoundMatches = FALSE;
    int i;
    
    StrCpyN(szFilter, pszIn, SIZECHARS(szFilter));

    TokenizeFilterString(szFilter, pszF, ARRAYSIZE(pszF), bLFN);

    for (i = 0; i < ARRAYSIZE(pszF) && pszF[i] && !fFoundMatches; i++)
    {
        HANDLE hff;
        WIN32_FIND_DATA FindFileData;

        //
        //  Find First for each filter.
        //
        hff = FindFirstFile(pszF[i], &FindFileData);

        if (hff == INVALID_HANDLE_VALUE)
        {
            continue;
        }

        do
        {
            if ((FindFileData.dwFileAttributes & EXCLBITS) ||
                (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                continue;
            }
            fFoundMatches = TRUE;
            break;

        } while (FindNextFile(hff, &FindFileData));

        FindClose(hff);
    }

    return fFoundMatches;
}

////////////////////////////////////////////////////////////////////////////
//
//  GetAppOpenDir
//
////////////////////////////////////////////////////////////////////////////

void GetAppOpenDir(LPCTSTR pszDir, LPTSTR pszOut, LPITEMIDLIST *ppidl)
{
    BOOL fUseMyDocs = FALSE;
    TCHAR szPersonal[MAX_PATH];

    *pszOut = 0;       // prepare to return empty string
    if (ppidl)
        *ppidl = NULL;

    if (SHGetSpecialFolderPath(NULL, szPersonal, CSIDL_PERSONAL, FALSE))
    {
        
        if (*pszDir)
        {
            //
            //  if the current directory is a temp dir 
            //  or is the mydocs dir, then use my docs
            //  otherwise we should just use this directory
            //
            if ((0 == lstrcmpi(pszDir, szPersonal) || PathIsTemporary(pszDir)))
                fUseMyDocs = TRUE;
        }
        else
        {
            TCHAR szPath[MAX_FULLPATHNAME];

            if (GetCurrentDirectory(ARRAYSIZE(szPath), szPath) 
            && (PathIsTemporary(szPath) || (0 == lstrcmpi(szPath, szPersonal))))
                fUseMyDocs = TRUE;
        }
    }

    if (fUseMyDocs)
    {
        lstrcpy(pszOut, szPersonal);
        if (ppidl)
            *ppidl = CreateMyDocsIDList();
    }
    else
        lstrcpy(pszOut, pszDir);
    
}

////////////////////////////////////////////////////////////////////////////
//
//  InitCurrentDisk
//
////////////////////////////////////////////////////////////////////////////

VOID InitCurrentDisk(HWND hDlg, POPENFILEINFO pOFI, WORD cmb)
{
    TCHAR szPath[MAX_FULLPATHNAME];

    //
    //  Clear out stale unc stuff from disk info.
    //  Unc \\server\shares are persistent through one popup session
    //  and then we resync with the system.  This is to fix a bug
    //  where a user's startup dir is unc but the system no longer has
    //  a connection and hence the cmb2 appears blank.
    //
    EnableDiskInfo(FALSE, TRUE);

    if (pOFI->pOFN->lpstrInitialDir)
    {
        //
        //  Notice that we force ChangeDir to succeed here
        //  but that TlsGetValue(g_tlsiCurDlg)->lpstrCurDir will return "" which
        //  when fed to SheChangeDirEx means GetCurrentDir will be called.
        //  So, the default cd behavior at startup is:
        //      1. lpstrInitialDir
        //      2. GetCurrentDir
        //
        szPath[0] = 0;
        if ( (pOFI->pOFN->Flags & OFN_ALLOWMULTISELECT) &&
             (StrChr(pOFI->pOFN->lpstrInitialDir, CHAR_SPACE)) &&
             (GetShortPathName( pOFI->pOFN->lpstrInitialDir,
                                szPath,
                                MAX_FULLPATHNAME )) &&
             (szPath[0] != 0) )
        {
            ChangeDir(hDlg, szPath, TRUE, FALSE);
        }
        else
        {
            ChangeDir(hDlg, pOFI->pOFN->lpstrInitialDir, TRUE, FALSE);
        }
    }
    else
    {
        GetAppOpenDir(TEXT(""),szPath, NULL);
        ChangeDir(hDlg, szPath, TRUE, FALSE);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  vDeleteDirDriveBitmap
//
//  Gets rid of bitmaps, if they exist.
//
////////////////////////////////////////////////////////////////////////////

VOID vDeleteDirDriveBitmap()
{
    if (hbmpOrigMemBmp)
    {
        SelectObject(hdcMemory, hbmpOrigMemBmp);
        if (hbmpDirDrive != HNULL)
        {
            DeleteObject(hbmpDirDrive);
            hbmpDirDrive = HNULL;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  LoadDirDriveBitmap
//
//  Creates the drive/directory bitmap.  If an appropriate bitmap
//  already exists, it just returns immediately.  Otherwise, it
//  loads the bitmap and creates a larger bitmap with both regular
//  and highlight colors.
//
////////////////////////////////////////////////////////////////////////////

BOOL LoadDirDriveBitmap()
{
    BITMAP bmp;
    HANDLE hbmp, hbmpOrig;
    HDC hdcTemp;
    BOOL bWorked = FALSE;

    if ( (hbmpDirDrive != HNULL) &&
         (rgbWindowColor == rgbDDWindow) &&
         (rgbHiliteColor == rgbDDHilite))
    {
        if (SelectObject(hdcMemory, hbmpDirDrive))
        {
            return (TRUE);
        }
    }

    vDeleteDirDriveBitmap();

    rgbDDWindow = rgbWindowColor;
    rgbDDHilite = rgbHiliteColor;

    if (!(hdcTemp = CreateCompatibleDC(hdcMemory)))
    {
        goto LoadExit;
    }

    if (!(hbmp = LoadAlterBitmap(bmpDirDrive, rgbSolidBlue, rgbWindowColor)))
    {
        goto DeleteTempDC;
    }

    GetObject(hbmp, sizeof(BITMAP), (LPTSTR)&bmp);
    dyDirDrive = bmp.bmHeight;
    dxDirDrive = bmp.bmWidth;

    hbmpOrig = SelectObject(hdcTemp, hbmp);

    hbmpDirDrive = CreateDiscardableBitmap(hdcTemp, dxDirDrive * 2, dyDirDrive);
    if (!hbmpDirDrive)
    {
        goto DeleteTempBmp;
    }

    if (!SelectObject(hdcMemory, hbmpDirDrive))
    {
        vDeleteDirDriveBitmap();
        goto DeleteTempBmp;
    }

    BitBlt(hdcMemory, 0, 0, dxDirDrive, dyDirDrive, hdcTemp, 0, 0, SRCCOPY);
    SelectObject(hdcTemp, hbmpOrig);

    DeleteObject(hbmp);

    if (!(hbmp = LoadAlterBitmap(bmpDirDrive, rgbSolidBlue, rgbHiliteColor)))
    {
        goto DeleteTempDC;
    }

    hbmpOrig = SelectObject(hdcTemp, hbmp);
    BitBlt(hdcMemory, dxDirDrive, 0, dxDirDrive, dyDirDrive, hdcTemp, 0, 0, SRCCOPY);
    SelectObject(hdcTemp, hbmpOrig);

    bWorked = TRUE;

DeleteTempBmp:
    DeleteObject(hbmp);

DeleteTempDC:
    DeleteDC(hdcTemp);

LoadExit:
    return (bWorked);
}


////////////////////////////////////////////////////////////////////////////
//
//  SetRGBValues
//
//  This sets the various system colors in static variables.  It's
//  called at init time and when system colors change.
//
////////////////////////////////////////////////////////////////////////////

void SetRGBValues()
{
    rgbWindowColor = GetSysColor(COLOR_WINDOW);
    rgbHiliteColor = GetSysColor(COLOR_HIGHLIGHT);
    rgbWindowText  = GetSysColor(COLOR_WINDOWTEXT);
    rgbHiliteText  = GetSysColor(COLOR_HIGHLIGHTTEXT);
    rgbGrayText    = GetSysColor(COLOR_GRAYTEXT);
}


////////////////////////////////////////////////////////////////////////////
//
//  FSetUpFile
//
//  This loads in the resources & initializes the data used by the
//  file dialogs.
//
//  Returns:  TRUE    if successful
//            FALSE   if any bitmap fails
//
////////////////////////////////////////////////////////////////////////////

BOOL FSetUpFile()
{
    if (cLock++)
    {
        return (TRUE);
    }

    SetRGBValues();

    return (LoadDirDriveBitmap());
}


////////////////////////////////////////////////////////////////////////////
//
//  GetPathOffset
//
////////////////////////////////////////////////////////////////////////////

int GetPathOffset(LPTSTR lpszDir)
{
    LPTSTR lpszSkipRoot;

    if (!lpszDir || !*lpszDir)
    {
        return (-1);
    }

    lpszSkipRoot = PathSkipRoot(lpszDir);

    if (lpszSkipRoot)
    {
        return (int)((lpszSkipRoot - 1) - lpszDir);
    }
    else
    {
        //
        //  Unrecognized format.
        //
        return (-1);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  FileOpenCmd
//
//  Handles WM_COMMAND for Open & Save dlgs.
//
//  edt1 = file name
//  lst1 = list of files in current directory matching current pattern
//  cmb1 = lists file patterns
//  stc1 = is current directory
//  lst2 = lists directories on current drive
//  cmb2 = lists drives
//  IDOK = is Open pushbutton
//  IDCANCEL = is Cancel pushbutton
//  chx1 = is for opening read only files
//
//  Returns the normal dialog proc values.
//
////////////////////////////////////////////////////////////////////////////

BOOL_PTR FileOpenCmd(
    HANDLE hDlg,
    WPARAM wParam,
    LPARAM lParam,
    POPENFILEINFO pOFI,
    BOOL bSave)
{
    LPOPENFILENAME pOFN;
    LPTSTR pch, pch2;
    WORD i, sCount, len;
    LRESULT wFlag;
    BOOL_PTR bRet, bHookRet;
    TCHAR szText[MAX_FULLPATHNAME];
    HWND hwnd;
    LPCURDLG  lpCurDlg;


    if (!pOFI)
    {
        return (FALSE);
    }

    pOFN = pOFI->pOFN;
    switch (GET_WM_COMMAND_ID(wParam, lParam))
    {
        case ( IDOK ) :
        {
#ifdef UNICODE
            //
            //  Apps that side-effect this message may not have their
            //  internal unicode strings updated (eg. Corel Mosaic).
            //
            //  NOTE: Must preserve the internal flags.
            //
            if (pOFI->ApiType == COMDLG_ANSI)
            {
                DWORD InternalFlags = pOFN->Flags & OFN_ALL_INTERNAL_FLAGS;

                ThunkOpenFileNameA2W(pOFI);

                pOFN->Flags |= InternalFlags;
            }
#endif
            //
            //  If the focus is on the directory box, or if the selection
            //  within the box has changed since the last listing, give a
            //  new listing.
            //
            if (bChangeDir || ((GetFocus() == GetDlgItem(hDlg, lst2)) &&
                               (pOFN->Flags & OFN_DIRSELCHANGED)))
            {
                bChangeDir = FALSE;
                goto ChangingDir;
            }
            else if ((GetFocus() == (hwnd = GetDlgItem(hDlg, cmb2))) &&
                     (pOFN->Flags & OFN_DRIVEDOWN))
            {
                //
                //  If the focus is on the drive or filter combobox, give
                //  a new listing.
                //
                SendDlgItemMessage(hDlg, cmb2, CB_SHOWDROPDOWN, FALSE, 0L);
                break;
            }
            else if ((GetFocus() == (hwnd = GetDlgItem(hDlg, cmb1))) &&
                     (pOFN->Flags & OFN_FILTERDOWN))
            {
                SendDlgItemMessage(hDlg, cmb1, CB_SHOWDROPDOWN, FALSE, 0L);
                lParam = (LPARAM)hwnd;
                goto ChangingFilter;
            }
            else
            {
#ifdef UNICODE
                //
                //  Visual Basic passes in an uninitialized lpstrDefExt string.
                //  Since we only have to use it in OKButtonPressed, update
                //  lpstrDefExt here along with whatever else is only needed
                //  in OKButtonPressed.
                //
                if (pOFI->ApiType == COMDLG_ANSI)
                {
                    ThunkOpenFileNameA2WDelayed(pOFI);
                }
#endif
                if (OKButtonPressed(hDlg, pOFI, bSave))
                {
                    bRet = TRUE;

                    if (pOFN->lpstrFile)
                    {
                        if (!(pOFN->Flags & OFN_NOVALIDATE))
                        {
                            if (pOFN->nMaxFile >= 3)
                            {
                                if ((pOFN->lpstrFile[0] == 0) ||
                                    (pOFN->lpstrFile[1] == 0) ||
                                    (pOFN->lpstrFile[2] == 0))
                                {
                                    bRet = FALSE;
                                    StoreExtendedError(FNERR_BUFFERTOOSMALL);
                                }
                            }
                            else
                            {
                                bRet = FALSE;
                                StoreExtendedError(FNERR_BUFFERTOOSMALL);
                            }
                        }
                    }

                    goto AbortDialog;
                }
            }

            SendDlgItemMessage(hDlg, edt1, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
            return (TRUE);

            break;
        }
        case ( IDCANCEL ) :
        {
            bRet = FALSE;
            bUserPressedCancel = TRUE;
            goto AbortDialog;
        }
        case ( IDABORT ) :
        {
            bRet = (BYTE)lParam;
AbortDialog:
            //
            //  Return the most recently used filter.
            //
            pOFN->nFilterIndex = (WORD)SendDlgItemMessage( hDlg,
                                                           cmb1,
                                                           CB_GETCURSEL,
                                                           (WPARAM)0,
                                                           (LPARAM)0 );
            if (pOFN->lpstrCustomFilter)
            {
                len = (WORD)(lstrlen(pOFN->lpstrCustomFilter) + 1);
                sCount = (WORD)lstrlen(pOFI->szLastFilter);
                if (pOFN->nMaxCustFilter > (DWORD)(sCount + len))
                {
                    lstrcpy(pOFN->lpstrCustomFilter + len, pOFI->szLastFilter);
                }
            }

            if (!pOFN->lpstrCustomFilter ||
                (*pOFN->lpstrCustomFilter == CHAR_NULL))
            {
                pOFN->nFilterIndex++;
            }

            if (((GET_WM_COMMAND_ID(wParam, lParam)) == IDOK) && pOFN->lpfnHook)
            {
                LPOFNHOOKPROC lpfnHook = GETHOOKFN(pOFN);

#ifdef UNICODE
                if (pOFI->ApiType == COMDLG_ANSI)
                {
                    ThunkOpenFileNameW2A(pOFI);
                    bHookRet = (*lpfnHook)( hDlg,
                                            msgFILEOKA,
                                            0,
                                            (LPARAM)pOFI->pOFNA );
                    //
                    //  For apps that side-effect pOFNA stuff and expect it to
                    //  be preserved through dialog exit, update internal
                    //  struct after the hook proc is called.
                    //
                    ThunkOpenFileNameA2W(pOFI);
                }
                else
#endif
                {
                    bHookRet = (*lpfnHook)( hDlg,
                                            msgFILEOKW,
                                            0,
                                            (LPARAM)pOFI->pOFN );
                }
                if (bHookRet)
                {
                    HourGlass(FALSE);
                    break;
                }
            }

            if (pOFN->Flags & OFN_ALLOWMULTISELECT)
            {
                LocalShrink((HANDLE)0, 0);
            }

            wNoRedraw = 0;

            if (pOFI->pOFN->Flags & OFN_ENABLEHOOK)
            {
                LPOFNHOOKPROC lpfnHook = GETHOOKFN(pOFN);

                glpfnFileHook = lpfnHook;
            }

            RemoveProp(hDlg, FILEPROP);

            EndDialog(hDlg, bRet);

            if (pOFI)
            {
                if ((pOFN->Flags & OFN_NOCHANGEDIR) && *pOFI->szCurDir)
                {
                    ChangeDir(hDlg, pOFI->szCurDir, TRUE, FALSE);
                }
            }

            //
            //  BUGBUG:
            //  If the app subclasses ID_ABORT, the worker thread will never
            //  get exited.  This will cause problems.  Currently, there are
            //  no apps that do this, though.
            //

            return (TRUE);
            break;
        }
        case ( edt1 ) :
        {
            if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
            {
                int iIndex, iCount;
                HWND hLBox = GetDlgItem(hDlg, lst1);
                WORD wIndex = (WORD)SendMessage(hLBox, LB_GETCARETINDEX, 0, 0);

                szText[0] = CHAR_NULL;

                if (wIndex == (WORD)LB_ERR)
                {
                    break;
                }

                SendMessage( GET_WM_COMMAND_HWND(wParam, lParam),
                             WM_GETTEXT,
                             (WPARAM)MAX_FULLPATHNAME,
                             (LPARAM)szText );

                if ((iIndex = (int)SendMessage( hLBox,
                                                LB_FINDSTRING,
                                                (WPARAM)(wIndex - 1),
                                                (LPARAM)szText )) != LB_ERR)
                {
                    RECT rRect;

                    iCount = (int)SendMessage(hLBox, LB_GETTOPINDEX, 0, 0L);
                    GetClientRect(hLBox, (LPRECT)&rRect);

                    if ((iIndex < iCount) ||
                        (iIndex >= (iCount + rRect.bottom / dyText)))
                    {
                        SendMessage(hLBox, LB_SETCARETINDEX, (WPARAM)iIndex, 0);
                        SendMessage(hLBox, LB_SETTOPINDEX, (WPARAM)iIndex, 0);
                    }
                }
                return (TRUE);
            }
            break;
        }
        case ( lst1 ) :
        {
            //
            //  A double click means OK.
            //
            if (GET_WM_COMMAND_CMD(wParam, lParam)== LBN_DBLCLK)
            {
                SendMessage(hDlg, WM_COMMAND, GET_WM_COMMAND_MPS(IDOK, 0, 0));
                return (TRUE);
            }
            else if (pOFN && (GET_WM_COMMAND_CMD(wParam, lParam) == LBN_SELCHANGE))
            {
                if (pOFN->Flags & OFN_ALLOWMULTISELECT)
                {
                    int *pSelIndex;

                    //
                    //  Muliselection allowed.
                    //
                    sCount = (SHORT)SendMessage(GET_WM_COMMAND_HWND(wParam, lParam),
                                                LB_GETSELCOUNT,
                                                0,
                                                0L );
                    if (!sCount)
                    {
                        //
                        //  If nothing selected, clear edit control.
                        //
                        SetDlgItemText(hDlg, edt1, szNull);
                    }
                    else
                    {
                        DWORD cchMemBlockSize = 2048;
                        DWORD cchTotalLength = 0;

                        pSelIndex = (int *)LocalAlloc(LPTR, sCount * sizeof(int));
                        if (!pSelIndex)
                        {
                            goto LocalFailure1;
                        }

                        sCount = (SHORT)SendMessage(
                                            GET_WM_COMMAND_HWND(wParam, lParam),
                                            LB_GETSELITEMS,
                                            (WPARAM)sCount,
                                            (LONG_PTR)(LPTSTR)pSelIndex );

                        pch2 = pch = (LPTSTR)
                             LocalAlloc(LPTR, cchMemBlockSize * sizeof(TCHAR));
                        if (!pch)
                        {
                            goto LocalFailure2;
                        }

                        for (*pch = CHAR_NULL, i = 0; i < sCount; i++)
                        {
                            len = (WORD)SendMessage(
                                            GET_WM_COMMAND_HWND(wParam, lParam),
                                            LB_GETTEXTLEN,
                                            (WPARAM)(*(pSelIndex + i)),
                                            (LPARAM)0 );

                            //
                            //  Add the length of the selected file to the
                            //  total length of selected files. + 2 for the
                            //  space that goes in between files and for the
                            //  possible dot added at the end of the filename
                            //  if the file does not have an extension.
                            //
                            cchTotalLength += (len + 2);

                            if (cchTotalLength > cchMemBlockSize)
                            {
                                LPTSTR pTemp;
                                UINT cchPrevLen = cchTotalLength - (len + 2);

                                cchMemBlockSize = cchMemBlockSize << 1;
                                pTemp = (LPTSTR)LocalReAlloc(
                                                 pch,
                                                 cchMemBlockSize * sizeof(TCHAR),
                                                 LMEM_MOVEABLE );
                                if (pTemp)
                                {
                                    pch = pTemp;
                                    pch2 = pch + cchPrevLen;
                                }
                                else
                                {
                                    LocalFree(pch);
                                    goto LocalFailure2;
                                }

                            }

                            SendMessage( GET_WM_COMMAND_HWND(wParam, lParam),
                                         LB_GETTEXT,
                                         (WPARAM)(*(pSelIndex + i)),
                                         (LONG_PTR)pch2 );

                            if (!StrChr(pch2, CHAR_DOT))
                            {
                                *(pch2 + len++) = CHAR_DOT;
                            }

                            pch2 += len;
                            *pch2++ = CHAR_SPACE;
                        }
                        if (pch2 != pch)
                        {
                            *--pch2 = CHAR_NULL;
                        }

                        SetDlgItemText(hDlg, edt1, pch);
                        LocalFree((HANDLE)pch);
LocalFailure2:
                        LocalFree((HANDLE)pSelIndex);
                    }
LocalFailure1:
                    if (pOFN->lpfnHook)
                    {
                        i = (WORD)SendMessage( GET_WM_COMMAND_HWND(wParam, lParam),
                                               LB_GETCARETINDEX,
                                               0,
                                               0L );
                        if (!(i & 0x8000))
                        {
                            wFlag = (SendMessage(
                                         GET_WM_COMMAND_HWND(wParam, lParam),
                                         LB_GETSEL,
                                         (WPARAM)i,
                                         0L )
                                     ? CD_LBSELADD
                                     : CD_LBSELSUB);
                        }
                        else
                        {
                            wFlag = CD_LBSELNOITEMS;
                        }
                    }
                }
                else
                {
                    //
                    //  Multiselection is not allowed.
                    //  Put the file name in the edit control.
                    //
                    szText[0] = CHAR_NULL;

                    i = (WORD)SendMessage( GET_WM_COMMAND_HWND(wParam, lParam),
                                           LB_GETCURSEL,
                                           0,
                                           0L );

                    if (i != (WORD)LB_ERR)
                    {
                        i = (WORD)SendMessage( GET_WM_COMMAND_HWND(wParam, lParam),
                                               LB_GETTEXT,
                                               (WPARAM)i,
                                               (LONG_PTR)szText );

                        if (!StrChr(szText, CHAR_DOT))
                        {
                            if (i < MAX_FULLPATHNAME - 1)
                            {
                                szText[i]     = CHAR_DOT;
                                szText[i + 1] = CHAR_NULL;
                            }
                        }

                        if (!bCasePreserved)
                        {
                            CharLower(szText);
                        }

                        SetDlgItemText(hDlg, edt1, szText);
                        if (pOFN->lpfnHook)
                        {
                            i = (WORD)SendMessage(
                                          GET_WM_COMMAND_HWND(wParam, lParam),
                                          LB_GETCURSEL,
                                          0,
                                          0L );
                            wFlag = CD_LBSELCHANGE;
                        }
                    }
                }

                if (pOFN->lpfnHook)
                {
                    LPOFNHOOKPROC lpfnHook = GETHOOKFN(pOFN);

#ifdef UNICODE
                    if (pOFI->ApiType == COMDLG_ANSI)
                    {
                        (*lpfnHook)( hDlg,
                                     msgLBCHANGEA,
                                     lst1,
                                     MAKELONG(i, wFlag) );
                    }
                    else
#endif
                    {
                        (*lpfnHook)( hDlg,
                                     msgLBCHANGEW,
                                     lst1,
                                     MAKELONG(i, wFlag) );
                    }
                }

                SendDlgItemMessage(hDlg, edt1, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
                return (TRUE);
            }
            break;
        }
        case ( cmb1 ) :
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
                case ( CBN_DROPDOWN ) :
                {
                    if (wWinVer >= 0x030A)
                    {
                        pOFN->Flags |= OFN_FILTERDOWN;
                    }
                    return (TRUE);
                    break;
                }
                case ( CBN_CLOSEUP ) :
                {
                    PostMessage( hDlg,
                                 WM_COMMAND,
                                 GET_WM_COMMAND_MPS(cmb1, lParam, MYCBN_DRAW) );

                    return (TRUE);
                    break;
                }
                case ( CBN_SELCHANGE ) :
                {
                    //
                    //  Need to change the file listing in lst1.
                    //
                    if (pOFN->Flags & OFN_FILTERDOWN)
                    {
                        return (TRUE);
                        break;
                    }
                }
                case ( MYCBN_DRAW ) :
                {
                    SHORT nIndex;
                    LPCTSTR lpFilter;

                    HourGlass(TRUE);

                    pOFN->Flags &= ~OFN_FILTERDOWN;
ChangingFilter:
                    nIndex = (SHORT)SendDlgItemMessage( hDlg,
                                                        cmb1,
                                                        CB_GETCURSEL,
                                                        0,
                                                        0L );
                    if (nIndex < 0)
                    {
                        //
                        //  No current selection.
                        //
                        break;
                    }

                    //
                    //  Must also check if filter contains anything.
                    //
                    if (nIndex ||
                        !(pOFN->lpstrCustomFilter && *pOFN->lpstrCustomFilter))
                    {
                        lpFilter = pOFN->lpstrFilter +
                                   SendDlgItemMessage( hDlg,
                                                       cmb1,
                                                       CB_GETITEMDATA,
                                                       (WPARAM)nIndex,
                                                       0L );
                    }
                    else
                    {
                        lpFilter = pOFN->lpstrCustomFilter +
                                   lstrlen(pOFN->lpstrCustomFilter) + 1;
                    }
                    if (*lpFilter)
                    {
                        GetDlgItemText( hDlg,
                                        edt1,
                                        szText,
                                        MAX_FULLPATHNAME - 1 );
                        bRet = (!szText[0] ||
                                (StrChr(szText, CHAR_STAR)) ||
                                (StrChr(szText, CHAR_QMARK)));
                        lstrcpy(szText, lpFilter);
                        if (bRet)
                        {
                            CharLower(szText);
                            SetDlgItemText(hDlg, edt1, szText);
                            SendDlgItemMessage( hDlg,
                                                edt1,
                                                EM_SETSEL,
                                                (WPARAM)0,
                                                (LPARAM)-1 );
                        }
                        FListAll(pOFI, hDlg, szText);
                        if (!bInitializing)
                        {
                            lstrcpy(pOFI->szLastFilter, szText);
#ifdef WINNT
                            //
                            //  Provide dynamic lpstrDefExt updating
                            //  when lpstrDefExt is user initialized.
                            //
                            if (StrChr((LPTSTR)lpFilter, CHAR_DOT) &&
                                pOFN->lpstrDefExt)
                            {
                                DWORD cbLen = MIN_DEFEXT_LEN - 1; // only 1st 3
                                LPTSTR lpTemp = (LPTSTR)(pOFN->lpstrDefExt);

                                while (*lpFilter++ != CHAR_DOT);
                                if (!(StrChr((LPTSTR)lpFilter, CHAR_STAR)) &&
                                    !(StrChr((LPTSTR)lpFilter, CHAR_QMARK)))
                                {
                                    while (cbLen--)
                                    {
                                        *lpTemp++ = *lpFilter++;
                                    }
                                    *lpTemp = CHAR_NULL;
                                }
                            }
#endif
                        }
                    }
                    if (pOFN->lpfnHook)
                    {
                        LPOFNHOOKPROC lpfnHook = GETHOOKFN(pOFN);
#ifdef UNICODE
                        if (pOFI->ApiType == COMDLG_ANSI)
                        {
                            (*lpfnHook)( hDlg,
                                         msgLBCHANGEA,
                                         cmb1,
                                         MAKELONG(nIndex, CD_LBSELCHANGE) );
                        }
                        else
#endif
                        {
                            (*lpfnHook)( hDlg,
                                         msgLBCHANGEW,
                                         cmb1,
                                         MAKELONG(nIndex, CD_LBSELCHANGE) );
                        }
                    }
                    HourGlass(FALSE);
                    return (TRUE);

                    break;
                }

                default :
                {
                    break;
                }
            }
            break;
        }
        case ( lst2 ) :
        {
            if (GET_WM_COMMAND_CMD(wParam, lParam) == LBN_SELCHANGE)
            {
                if (!(pOFN->Flags & OFN_DIRSELCHANGED))
                {
                    if ((DWORD)SendDlgItemMessage( hDlg,
                                                   lst2,
                                                   LB_GETCURSEL,
                                                   0,
                                                   0L ) != pOFI->idirSub - 1)
                    {
                        StripFileName(hDlg, IS16BITWOWAPP(pOFN));
                        pOFN->Flags |= OFN_DIRSELCHANGED;
                    }
                }
                return (TRUE);
            }
            else if (GET_WM_COMMAND_CMD(wParam, lParam) == LBN_SETFOCUS)
            {
                EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
                SendMessage( GetDlgItem(hDlg, IDCANCEL),
                             BM_SETSTYLE,
                             (WPARAM)BS_PUSHBUTTON,
                             (LPARAM)TRUE );
            }
            else if (GET_WM_COMMAND_CMD(wParam, lParam) == LBN_KILLFOCUS)
            {
                if (pOFN && (pOFN->Flags & OFN_DIRSELCHANGED))
                {
                    pOFN->Flags &= ~OFN_DIRSELCHANGED;
                }
                else
                {
                    bChangeDir = FALSE;
                }
            }
            else if (GET_WM_COMMAND_CMD(wParam, lParam) == LBN_DBLCLK)
            {
                TCHAR szNextDir[CCHNETPATH];
                LPTSTR lpCurDir;
                DWORD idir;
                DWORD idirNew;
                int cb;
                LPTSTR pstrPath;
ChangingDir:
                bChangeDir = FALSE;
                pOFN->Flags &= ~OFN_DIRSELCHANGED;
                idirNew = (DWORD)SendDlgItemMessage( hDlg,
                                                     lst2,
                                                     LB_GETCURSEL,
                                                     0,
                                                     0L );
                //
                //  Can use relative path name.
                //
                *pOFI->szPath = 0;
                if (idirNew >= pOFI->idirSub)
                {
                    cb = (int) SendDlgItemMessage( hDlg,
                                                   lst2,
                                                   LB_GETTEXT,
                                                   (WPARAM)idirNew,
                                                   (LPARAM)pOFI->szPath );
                    //
                    //  sanity check
                    //
                    if (!(lpCurDlg = (LPCURDLG)TlsGetValue(g_tlsiCurDlg)) ||
                        !(lpCurDir = lpCurDlg->lpstrCurDir))
                    {
                        break;
                    }

                    lstrcpy(szNextDir, lpCurDir);

                    //
                    //  Fix phenom with c:\\foobar - because of inconsistency
                    //  in directory display guaranteed to have a valid
                    //  lpCurDir here, right?
                    //
                    PathAddBackslash(szNextDir);
                    lstrcat(szNextDir, pOFI->szPath);

                    pstrPath = szNextDir;

                    idirNew = pOFI->idirSub;    // for msgLBCHANGE message
                }
                else
                {
                    //
                    //  Need full path name.
                    //
                    cb = (int) SendDlgItemMessage( hDlg,
                                                   lst2,
                                                   LB_GETTEXT,
                                                   0,
                                                   (LPARAM)pOFI->szPath );

                    //
                    //  The following condition is necessary because wb displays
                    //  \\server\share (the disk resource name) for unc, but
                    //  for root paths (eg. c:\) for device conns, this in-
                    //  consistency is hacked around here and in FillOutPath.
                    //
                    if (DBL_BSLASH(pOFI->szPath))
                    {
                        lstrcat(pOFI->szPath, TEXT("\\"));
                        cb++;
                    }

                    for (idir = 1; idir <= idirNew; ++idir)
                    {
                        cb += (int) SendDlgItemMessage(
                                             hDlg,
                                             lst2,
                                             LB_GETTEXT,
                                             (WPARAM)idir,
                                             (LPARAM)&pOFI->szPath[cb] );

                        pOFI->szPath[cb++] = CHAR_BSLASH;
                    }

                    //
                    //  The root is a special case.
                    //
                    if (idirNew)
                    {
                        pOFI->szPath[cb - 1] = CHAR_NULL;
                    }

                    pstrPath = pOFI->szPath;
                }

                if (!*pstrPath ||
                    (ChangeDir(hDlg, pstrPath, FALSE, TRUE) == CHANGEDIR_FAILED))
                {
                    break;
                }

                //
                //  List all directories under this one.
                //
                UpdateListBoxes(hDlg, pOFI, NULL, mskDirectory);

                if (pOFN->lpfnHook)
                {
                    LPOFNHOOKPROC lpfnHook = GETHOOKFN(pOFN);
#ifdef UNICODE
                    if (pOFI->ApiType == COMDLG_ANSI)
                    {
                        (*lpfnHook)( hDlg,
                                     msgLBCHANGEA,
                                     lst2,
                                     MAKELONG(LOWORD(idirNew), CD_LBSELCHANGE) );
                    }
                    else
#endif
                    {
                        (*lpfnHook)( hDlg,
                                     msgLBCHANGEW,
                                     lst2,
                                     MAKELONG(LOWORD(idirNew), CD_LBSELCHANGE) );
                    }
                }
                return (TRUE);
            }
            break;
        }
        case ( cmb2 ) :
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
                case ( CBN_DROPDOWN ) :
                {
                    pOFN->Flags |= OFN_DRIVEDOWN;

                    return (TRUE);
                    break;
                }
                case ( CBN_CLOSEUP ) :
                {
                    //
                    //  It would seem reasonable to merely do the update
                    //  at this point, but that would rely on message
                    //  ordering, which isnt a smart move.  In fact, if
                    //  you hit ALT-DOWNARROW, DOWNARROW, ALT-DOWNARROW,
                    //  you receive CBN_DROPDOWN, CBN_SELCHANGE, and then
                    //  CBN_CLOSEUP.  But if you use the mouse to choose
                    //  the same element, the last two messages trade
                    //  places.  PostMessage allows all messages in the
                    //  sequence to be processed, and then updates are
                    //  done as needed.
                    //
                    PostMessage( hDlg,
                                 WM_COMMAND,
                                 GET_WM_COMMAND_MPS(
                                     cmb2,
                                     GET_WM_COMMAND_HWND(wParam, lParam),
                                     MYCBN_DRAW ) );
                    return (TRUE);
                    break;
                }
                case ( MYCBN_LIST ) :
                {
                    LoadDrives(hDlg);
                    break;
                }
                case ( MYCBN_REPAINT ) :
                {
                    int cchCurDir;
                    LPTSTR lpCurDir;

                    // sanity
                    if (!(lpCurDlg = (LPCURDLG)TlsGetValue(g_tlsiCurDlg)) ||
                        !(lpCurDir = lpCurDlg->lpstrCurDir))
                    {
                        break;
                    }

                    cchCurDir = GetPathOffset(lpCurDir);
                    if (cchCurDir != -1)
                    {
                        TCHAR szRepaintDir[CCHNETPATH];
                        HWND hCmb2 = (HWND)lParam;

                        lstrcpy(szRepaintDir, lpCurDir);
                        szRepaintDir[cchCurDir] = CHAR_NULL;
                        SendMessage( hCmb2,
                                     CB_SELECTSTRING,
                                     (WPARAM)-1,
                                     (LPARAM)szRepaintDir );
                    }
                    break;
                }
                case ( CBN_SELCHANGE ) :
                {
                    StripFileName(hDlg, IS16BITWOWAPP(pOFN));

                    //
                    //  Version check not needed, since flag never set
                    //  for versions not supporting CBN_CLOSEUP. Putting
                    //  check at CBN_DROPDOWN is more efficient since it
                    //  is less frequent than CBN_SELCHANGE.

                    if (pOFN->Flags & OFN_DRIVEDOWN)
                    {
                        //
                        //  Don't fill lst2 while the combobox is down.
                        //
                        return (TRUE);
                        break;
                    }
                }
                case ( MYCBN_CHANGEDIR ) :
                case ( MYCBN_DRAW ) :
                {
                    TCHAR szTitle[WARNINGMSGLENGTH];
                    LPTSTR lpFilter;
                    int nDiskInd, nInd;
                    DWORD dwType;
                    LPTSTR lpszPath = NULL;
                    LPTSTR lpszDisk = NULL;
                    HWND hCmb2;
                    OFN_DISKINFO *pofndiDisk = NULL;
                    static szDrawDir[CCHNETPATH];
                    int nRet;

                    HourGlass(TRUE);

                    //
                    //  Clear Flag for future CBN_SELCHANGE messeges.
                    //
                    pOFN->Flags &= ~OFN_DRIVEDOWN;

                    //
                    //  Change the drive.
                    //
                    szText[0] = CHAR_NULL;

                    hCmb2 = (HWND)lParam;

                    if (hCmb2 != NULL)
                    {
                        nInd = (int) SendMessage(hCmb2, CB_GETCURSEL, 0, 0L);

                        if (nInd != CB_ERR)
                        {
                            SendMessage( hCmb2,
                                         CB_GETLBTEXT,
                                         nInd,
                                         (LPARAM)(LPTSTR)szDrawDir );
                        }

                        if ((nInd == CB_ERR) || ((INT_PTR)pofndiDisk == CB_ERR))
                        {
                            if (lpCurDlg = (LPCURDLG)TlsGetValue(g_tlsiCurDlg))
                            {
                                if (lpCurDlg->lpstrCurDir)
                                {
                                    lstrcpy((LPTSTR)szDrawDir,
                                            lpCurDlg->lpstrCurDir);
                                }
                            }
                        }

                        CharLower((LPTSTR)szDrawDir);

                        //
                        //  Should always succeed.
                        //
                        nDiskInd = DiskAddedPreviously(0, (LPTSTR)szDrawDir);
                        if (nDiskInd != 0xFFFFFFFF)
                        {
                            pofndiDisk = &gaDiskInfo[nDiskInd];
                        }
                        else
                        {
                            //
                            //  Skip update in the case where it fails.
                            //
                            return (TRUE);
                        }

                        dwType = pofndiDisk->dwType;

                        lpszDisk = pofndiDisk->lpPath;
                    }

                    if ((GET_WM_COMMAND_CMD(wParam, lParam)) == MYCBN_CHANGEDIR)
                    {
                        if (lpNetDriveSync)
                        {
                            lpszPath = lpNetDriveSync;
                            lpNetDriveSync = NULL;
                        }
                        else
                        {
                            if (lpCurDlg = (LPCURDLG)TlsGetValue(g_tlsiCurDlg))
                            {
                                if (lpCurDlg->lpstrCurDir)
                                {
                                    lstrcpy((LPTSTR)szDrawDir,
                                            lpCurDlg->lpstrCurDir);

                                    lpszPath = (LPTSTR)szDrawDir;
                                }
                            }
                        }
                    }
                    else
                    {
                        lpszPath = lpszDisk;
                    }

                    if (bInitializing)
                    {
                        lpFilter = szTitle;
                        if (pOFN->lpstrFile &&
                            (StrChr(pOFN->lpstrFile, CHAR_STAR) ||
                             StrChr(pOFN->lpstrFile, CHAR_QMARK)))
                        {
                            lstrcpy(lpFilter, pOFN->lpstrFile);
                        }
                        else
                        {
                            HWND hcmb1 = GetDlgItem(hDlg, cmb1);

                            nInd = (int) SendMessage(hcmb1, CB_GETCURSEL, 0, 0L);
                            if (nInd == CB_ERR)
                            {
                                //
                                //  No current selection.
                                //
                                goto NullSearch;
                            }

                            //
                            //  Must also check if filter contains anything.
                            //
                            if (nInd ||
                                !(pOFN->lpstrCustomFilter &&
                                  *pOFN->lpstrCustomFilter))
                            {
                                lpFilter = (LPTSTR)(pOFN->lpstrFilter);
                                lpFilter += SendMessage( hcmb1,
                                                         CB_GETITEMDATA,
                                                         (WPARAM)nInd,
                                                         0 );
                            }
                            else
                            {
                                lpFilter = pOFN->lpstrCustomFilter;
                                lpFilter += lstrlen(pOFN->lpstrCustomFilter) + 1;
                            }
                        }
                    }
                    else
                    {
NullSearch:
                        lpFilter = NULL;
                    }

                    //
                    //  UpdateListBoxes cuts up filter string in place.
                    //
                    if (lpFilter)
                    {
                        lstrcpy(szTitle, lpFilter);
                        CharLower(szTitle);
                    }

                    if (dwType == REMDRVBMP)
                    {
                        DWORD err = WNetRestoreConnection(hDlg, lpszDisk);

                        if (err != WN_SUCCESS)
                        {
                            HourGlass(FALSE);
                            return (TRUE);
                        }

                        pofndiDisk->dwType = NETDRVBMP;

                        SendMessage(
                            hCmb2,
                            CB_SETITEMDATA,
                            (WPARAM)SendMessage(
                                   hCmb2,
                                   CB_SELECTSTRING,
                                   (WPARAM)-1,
                                   (LPARAM)(LPTSTR)pofndiDisk->lpAbbrName ),
                            (LPARAM)NETDRVBMP );
                    }

                    //
                    //  Calls to ChangeDir will call SelDisk, so no need
                    //  to update cmb2 on our own here (used to be after
                    //  updatelistboxes).
                    //
                    if ((nRet = ChangeDir( hDlg,
                                           lpszPath,
                                           FALSE,
                                           FALSE )) == CHANGEDIR_FAILED)
                    {
                        int mbRet;

                        while (nRet == CHANGEDIR_FAILED)
                        {
                            if (dwType == FLOPPYBMP)
                            {
                                mbRet = InvalidFileWarning(
                                               hDlg,
                                               lpszPath,
                                               ERROR_NO_DISK_IN_DRIVE,
                                               (UINT)(MB_RETRYCANCEL |
                                                      MB_ICONEXCLAMATION));
                            }
                            else if (dwType == CDDRVBMP)
                            {
                                mbRet = InvalidFileWarning(
                                               hDlg,
                                               lpszPath,
                                               ERROR_NO_DISK_IN_CDROM,
                                               (UINT)(MB_RETRYCANCEL |
                                                      MB_ICONEXCLAMATION) );
                            }
                            else
                            {
                                //
                                //  See if it's a RAW volume.
                                //
                                if (dwType == HARDDRVBMP &&
                                    GetLastError() == ERROR_UNRECOGNIZED_VOLUME)
                                {
                                    mbRet = InvalidFileWarning(
                                                   hDlg,
                                                   lpszPath,
                                                   ERROR_UNRECOGNIZED_VOLUME,
                                                   (UINT)(MB_OK |
                                                          MB_ICONEXCLAMATION) );
                                }
                                else
                                {
                                    mbRet = InvalidFileWarning(
                                                   hDlg,
                                                   lpszPath,
                                                   ERROR_DIR_ACCESS_DENIED,
                                                   (UINT)(MB_RETRYCANCEL |
                                                          MB_ICONEXCLAMATION) );
                                }
                            }

                            if (bFirstTime || (mbRet != IDRETRY))
                            {
                                lpszPath = NULL;
                                nRet = ChangeDir(hDlg, lpszPath, TRUE, FALSE);
                            }
                            else
                            {
                                nRet = ChangeDir(hDlg, lpszPath, FALSE, FALSE);
                            }
                        }
                    }

                    UpdateListBoxes( hDlg,
                                     pOFI,
                                     lpFilter ? szTitle : lpFilter,
                                     (WORD)(mskDrives | mskDirectory) );

                    if (pOFN->lpfnHook)
                    {
                        LPOFNHOOKPROC lpfnHook = GETHOOKFN(pOFN);

                        nInd = (int) SendDlgItemMessage( hDlg,
                                                         cmb2,
                                                         CB_GETCURSEL,
                                                         0,
                                                         0 );
#ifdef UNICODE
                        if (pOFI->ApiType == COMDLG_ANSI)
                        {
                            (*lpfnHook)( hDlg,
                                         msgLBCHANGEA,
                                         cmb2,
                                         MAKELONG(LOWORD(nInd),
                                                  CD_LBSELCHANGE) );
                        }
                        else
#endif
                        {
                            (*lpfnHook)( hDlg,
                                         msgLBCHANGEW,
                                         cmb2,
                                         MAKELONG(LOWORD(nInd),
                                                  CD_LBSELCHANGE) );
                        }
                    }

                    HourGlass(FALSE);

                    return (TRUE);

                    break;
                }
                default :
                {
                    break;
                }
            }
            break;
        }
        case ( pshHelp ) :
        {
#ifdef UNICODE
            if (pOFI->ApiType == COMDLG_ANSI)
            {
                if (msgHELPA && pOFN->hwndOwner)
                {
                    SendMessage( pOFN->hwndOwner,
                                 msgHELPA,
                                 (WPARAM)hDlg,
                                 (DWORD_PTR)pOFN );
                }
            }
            else
#endif
            {
                if (msgHELPW && pOFN->hwndOwner)
                {
                    SendMessage( pOFN->hwndOwner,
                                 msgHELPW,
                                 (WPARAM)hDlg,
                                 (DWORD_PTR)pOFN );
                }
            }
            break;
        }
        case ( psh14 ) :
        {
            bGetNetDrivesSync = TRUE;
            if (CallNetDlg(hDlg))
            {
                LNDSetEvent(hDlg);
            }
            else
            {
                bGetNetDrivesSync = FALSE;
            }
            break;
        }
        default :
        {
            break;
        }
    }
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  UpdateListBoxes
//
//  Fills out File and Directory List Boxes in a single pass
//  given (potentially) multiple filters
//
//  It assumes the string of extensions are delimited by semicolons.
//
//  hDlg        Handle to File Open/Save dialog
//  pOFI        pointer to OPENFILEINFO structure
//  lpszFilter  pointer to filter, if NULL, use pOFI->szSpecCur
//  wMask       mskDirectory and/or mskDrives, or NULL
//
//  Returns:  TRUE   if match
//            FALSE  if not
//
////////////////////////////////////////////////////////////////////////////

BOOL UpdateListBoxes(
    HWND hDlg,
    POPENFILEINFO pOFI,
    LPTSTR lpszFilter,
    WORD wMask)
{
    LPTSTR lpszF[MAXFILTERS + 1];
    LPTSTR lpszTemp;
    SHORT i, nFilters;
    HWND hFileList = GetDlgItem(hDlg, lst1);
    HWND hDirList = GetDlgItem(hDlg, lst2);
    BOOL bRet = FALSE;
    TCHAR szSpec[MAX_FULLPATHNAME];
    BOOL bDriveChange;
    BOOL bFindAll = FALSE;
    RECT rDirLBox;
    BOOL bLFN;
    HANDLE hff;
    DWORD dwErr;
    WIN32_FIND_DATA FindFileData;
    TCHAR szBuffer[MAX_FULLPATHNAME];       // add one for CHAR_DOT
    WORD wCount;
    LPCURDLG lpCurDlg;


    //
    //  Save the drive bit and then clear it out.
    //
    bDriveChange = wMask & mskDrives;
    wMask &= ~mskDrives;

    if (!lpszFilter)
    {
        GetDlgItemText( hDlg,
                        edt1,
                        lpszFilter = szSpec,
                        MAX_FULLPATHNAME - 1 );

        //
        //  If any directory or drive characters are in there, or if there
        //  are no wildcards, use the default spec.
        //
        if ( StrChr(szSpec, CHAR_BSLASH) ||
             StrChr(szSpec, CHAR_SLASH)  ||
             StrChr(szSpec, CHAR_COLON)  ||
             (!((StrChr(szSpec, CHAR_STAR)) ||
                (StrChr(szSpec, CHAR_QMARK)))) )
        {
            lstrcpy(szSpec, pOFI->szSpecCur);
        }
        else
        {
            lstrcpy(pOFI->szLastFilter, szSpec);
        }
    }

    //
    //  We need to find out what kind of a drive we are running
    //  on in order to determine if spaces are valid in a filename
    //  or not.
    //
    bLFN = IsLFNDriveX(hDlg, TEXT("\0"));

    //
    //  Find the first filter in the string, and add it to the
    //  array.
    //
    if (bLFN)
    {
        lpszF[nFilters = 0] = lstrtok(lpszFilter, szSemiColonTab);
    }
    else
    {
        lpszF[nFilters = 0] = lstrtok(lpszFilter, szSemiColonSpaceTab);
    }

    //
    //  Now we are going to loop through all the filters in the string
    //  parsing the one we already have, and then finding the next one
    //  and starting the loop over again.
    //
    while (lpszF[nFilters] && (nFilters < MAXFILTERS))
    {
        //
        //  Check to see if the first character is a space.
        //  If so, remove the spaces, and save the pointer
        //  back into the same spot.  Why?  because the
        //  FindFirstFile/Next api will _still_ work on
        //  filenames that begin with a space because
        //  they also look at the short names.  The
        //  short names will begin with the same first
        //  real letter as the long filename.  For
        //  example, the long filename is "  my document"
        //  the first letter of this short name is "m",
        //  so searching on "m*.*" or " m*.*" will yield
        //  the same results.
        //
        if (bLFN && (*lpszF[nFilters] == CHAR_SPACE))
        {
            lpszTemp = lpszF[nFilters];
            while ((*lpszTemp == CHAR_SPACE) && *lpszTemp)
            {
                lpszTemp = CharNext(lpszTemp);
            }

            lpszF[nFilters] = lpszTemp;
        }

        //
        //  The original code used to do a CharUpper here to put the
        //  filter strings in upper case.  EG:  *.TXT  However, this
        //  is not a good thing to do for Turkish.  Capital 'i' does
        //  not equal 'I', so the CharUpper is being removed.
        //
        //  CharUpper(lpszF[nFilters]);

        //
        //  Compare the filter with *.*.  If we find *.* then
        //  set the boolean bFindAll, and this will cause the
        //  files listbox to be filled in at the same time the
        //  directories listbox is filled.  This saves time
        //  from walking the directory twice (once for the directory
        //  names and once for the filenames).
        //
        if (!lstrcmpi(lpszF[nFilters], szStarDotStar))
        {
            bFindAll = TRUE;
        }

        //
        //  Now we need to check if this filter is a duplicate
        //  of an already existing filter.
        //
        for (wCount = 0; wCount < nFilters; wCount++)
        {
            //
            //  If we find a duplicate, decrement the current
            //  index pointer by one so that the last location
            //  is written over (thus removing the duplicate),
            //  and break out of this loop.
            //
            if (!lstrcmpi(lpszF[nFilters], lpszF[wCount]))
            {
                nFilters--;
                break;
            }
        }

        //
        //  Ready to move on to the next filter.  Find the next
        //  filter based upon the type of file system we're using.
        //
        if (bLFN)
        {
            lpszF[++nFilters] = lstrtok(NULL, szSemiColonTab);
        }
        else
        {
            lpszF[++nFilters] = lstrtok(NULL, szSemiColonSpaceTab);
        }

        //
        //  In case we found a pointer to NULL, then look for the
        //  next filter.
        //
        while (lpszF[nFilters] && !*lpszF[nFilters])
        {
            if (bLFN)
            {
                lpszF[nFilters] = lstrtok(NULL, szSemiColonTab);
            }
            else
            {
                lpszF[nFilters] = lstrtok(NULL, szSemiColonSpaceTab);
            }
        }
    }

    //
    //  Add NULL terminator only if needed.
    //
    if (nFilters >= MAXFILTERS)
    {
        lpszF[MAXFILTERS] = 0;
    }

    HourGlass(TRUE);

    SendMessage(hFileList, WM_SETREDRAW, FALSE, 0L);
    SendMessage(hFileList, LB_RESETCONTENT, 0, 0L);
    if (wMask & mskDirectory)
    {
        wNoRedraw |= 2;     // HACK!!! WM_SETREDRAW isn't complete
        SendMessage(hDirList, WM_SETREDRAW, FALSE, 0L);

        //
        //  LB_RESETCONTENT causes InvalidateRect(hDirList, 0, TRUE) to be
        //  sent as well as repositioning the scrollbar thumb and drawing
        //  it immediately.  This causes flicker when the LB_SETCURSEL is
        //  made, as it clears out the listbox by erasing the background of
        //  each item.
        //
        SendMessage(hDirList, LB_RESETCONTENT, 0, 0L);
    }

    //
    //  Always open enumeration for *.*
    //
    lpCurDlg = (LPCURDLG)TlsGetValue(g_tlsiCurDlg);
    SetCurrentDirectory(lpCurDlg ? lpCurDlg->lpstrCurDir : NULL);
    hff = FindFirstFile(szStarDotStar, &FindFileData);

    if ( hff == INVALID_HANDLE_VALUE)
    {
        //
        //  Error.  Call GetLastError to determine what happened.
        //
        dwErr = GetLastError();

        //
        //  With the ChangeDir logic handling AccessDenied for cds,
        //  if we are not allowed to enum files, that's ok, just get out.
        //
        if (dwErr == ERROR_ACCESS_DENIED)
        {
            wMask = mskDirectory;
            goto Func4EFailure;
        }

        //
        //  For bad path of bad filename.
        //
        if (dwErr != ERROR_FILE_NOT_FOUND)
        {
            wMask = mskDrives;
            goto Func4EFailure;
        }
    }

    //
    //  A listing was made, even if empty.
    //
    bRet = TRUE;
    wMask &= mskDirectory;

    //
    //  GetLastError says no more files.
    //
    if (hff == INVALID_HANDLE_VALUE  && dwErr == ERROR_FILE_NOT_FOUND)
    {
        //
        //  Things went well, but there are no files.
        //
        goto NoMoreFilesFound;
    }

    do
    {
        if (pOFI->pOFN->Flags & OFN_NOLONGNAMES)
        {
#ifdef UNICODE
            UNICODE_STRING Name;
            BOOLEAN fSpace = FALSE;

            RtlInitUnicodeString(&Name, FindFileData.cFileName);
            if (RtlIsNameLegalDOS8Dot3(&Name, NULL, &fSpace) && !fSpace)
            {
                //
                //  Legal 8.3 name and no spaces, so use the principal
                //  file name.
                //
                lstrcpy(szBuffer, (LPTSTR)FindFileData.cFileName);
            }
            else
#endif
            {
#ifdef WINNT
                if (FindFileData.cAlternateFileName[0] == CHAR_NULL)
                {
                    continue;
                }

                //
                //  Use the alternate file name.
                //
                lstrcpy(szBuffer, (LPTSTR)FindFileData.cAlternateFileName);
#else
                if (FindFileData.cAlternateFileName[0])
                {
                    //
                    //  Use the alternate file name.
                    //
                    lstrcpy(szBuffer, (LPTSTR)FindFileData.cAlternateFileName);
                }
                else
                {
                    //
                    //  Use the main file name.
                    //
                    lstrcpy(szBuffer, (LPTSTR)FindFileData.cFileName);
                }
#endif
            }
        }
        else
        {
            lstrcpy(szBuffer, (LPTSTR)FindFileData.cFileName);
        }

        if ((FindFileData.dwFileAttributes & EXCLBITS))
        {
            continue;
        }

        if ((pOFI->pOFN->Flags & OFN_ALLOWMULTISELECT))
        {
            if (StrChr(szBuffer, CHAR_SPACE))
            {
                //
                //  HPFS does not support alternate filenames
                //  for multiselect, bump all spacey filenames.
                //
                if (FindFileData.cAlternateFileName[0] == CHAR_NULL)
                {
                    continue;
                }

                lstrcpy(szBuffer, (LPTSTR)FindFileData.cAlternateFileName);
            }
        }

        if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (wMask & mskDirectory)
            {
                //
                //  Don't include the subdirectories "." and "..".
                //
                if (szBuffer[0] == CHAR_DOT)
                {
                    if ((szBuffer[1] == CHAR_NULL) ||
                        ((szBuffer[1] == CHAR_DOT) && (szBuffer[2] == CHAR_NULL)))
                    {
                        continue;
                    }
                }
                if (!bCasePreserved)
                {
                    CharLower(szBuffer);
                }
                i = (WORD)SendMessage( hDirList,
                                       LB_ADDSTRING,
                                       0,
                                       (DWORD_PTR)szBuffer );
            }
        }
        else if (bFindAll)
        {
            if (!bCasePreserved)
            {
                CharLower(szBuffer);
            }

            SendMessage(hFileList, LB_ADDSTRING, 0, (DWORD_PTR)szBuffer);
        }
    } while (FindNextFile(hff, &FindFileData));

    if (hff == INVALID_HANDLE_VALUE)
    {
        goto Func4EFailure;
    }

    FindClose(hff);

    if (!bFindAll)
    {
        for (i = 0; lpszF[i]; i++)
        {
            if (!lstrcmpi(lpszF[i], szStarDotStar))
            {
                continue;
            }

            //
            //  Find First for each filter.
            //
            hff = FindFirstFile(lpszF[i], &FindFileData);

            if (hff == INVALID_HANDLE_VALUE)
            {
                DWORD dwErr = GetLastError();

                if ((dwErr == ERROR_FILE_NOT_FOUND) ||
                    (dwErr == ERROR_INVALID_NAME))
                {
                    //
                    //  Things went well, but there are no files.
                    //
                    continue;
                }
                else
                {
                    wMask = mskDrives;
                    goto Func4EFailure;
                }
            }

            do
            {
                if (pOFI->pOFN->Flags & OFN_NOLONGNAMES)
                {
#ifdef UNICODE
                    UNICODE_STRING Name;
                    BOOLEAN fSpace = FALSE;

                    RtlInitUnicodeString(&Name, FindFileData.cFileName);
                    if (RtlIsNameLegalDOS8Dot3(&Name, NULL, &fSpace) && !fSpace)
                    {
                        //
                        //  Legal 8.3 name and no spaces, so use the principal
                        //  file name.
                        //
                        lstrcpy(szBuffer, (LPTSTR)FindFileData.cFileName);
                    }
                    else
#endif
                    {
#ifdef WINNT
                        if (FindFileData.cAlternateFileName[0] == CHAR_NULL)
                        {
                            continue;
                        }

                        //
                        //  Use the alternate file name.
                        //
                        lstrcpy( szBuffer,
                                 (LPTSTR)FindFileData.cAlternateFileName );
#else
                        if (FindFileData.cAlternateFileName[0])
                        {
                            //
                            //  Use the alternate file name.
                            //
                            lstrcpy( szBuffer,
                                     (LPTSTR)FindFileData.cAlternateFileName );
                        }
                        else
                        {
                            //
                            //  Use the main file name.
                            //
                            lstrcpy(szBuffer, (LPTSTR)FindFileData.cFileName);
                        }
#endif
                    }
                }
                else
                {
                    lstrcpy(szBuffer, (LPTSTR)FindFileData.cFileName);

                    if (pOFI->pOFN->Flags & OFN_ALLOWMULTISELECT)
                    {
                        if (StrChr(szBuffer, CHAR_SPACE))
                        {
                            //
                            //  HPFS does not support alternate filenames
                            //  for multiselect, bump all spacey filenames.
                            //
                            if (FindFileData.cAlternateFileName[0] == CHAR_NULL)
                            {
                                continue;
                            }

                            lstrcpy( szBuffer,
                                     (LPTSTR)FindFileData.cAlternateFileName );
                        }
                    }
                }

                if ((FindFileData.dwFileAttributes & EXCLBITS) ||
                    (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    continue;
                }

                if (!bCasePreserved)
                {
                    CharLower(szBuffer);
                }

                SendMessage(hFileList, LB_ADDSTRING, 0, (DWORD_PTR)szBuffer);
            } while (FindNextFile(hff, &FindFileData));

            if (hff != INVALID_HANDLE_VALUE)
            {
                FindClose(hff);
            }
        }
    }

NoMoreFilesFound:

Func4EFailure:
    if (wMask)
    {
        if (wMask == mskDirectory)
        {
            LPTSTR lpCurDir = NULL;

            if (lpCurDlg = (LPCURDLG)TlsGetValue(g_tlsiCurDlg))
            {
                lpCurDir = lpCurDlg->lpstrCurDir;
            }

            FillOutPath(hDirList, pOFI);

            //
            //  The win31 way of chopping the text by just passing
            //  it on to user doesn't work for unc names since user
            //  doesn't see the drivelessness of them (thinks drive is
            //  a bslash char).  So, special case it here.
            //
            lstrcpy(pOFI->szPath, lpCurDir);

            if (DBL_BSLASH(pOFI->szPath))
            {
                SetDlgItemText(hDlg, stc1, ChopText(hDlg, stc1, pOFI->szPath));
            }
            else
            {
                DlgDirList(hDlg, pOFI->szPath, 0, stc1, DDL_READONLY);
            }

            SendMessage(hDirList, LB_SETCURSEL, pOFI->idirSub - 1, 0L);

            if (bDriveChange)
            {
                //
                //  The design here is to show the selected drive whenever the
                //  user changes drives, or whenever the number of
                //  subdirectories is sufficiently low to allow them to be
                //  shown along with the drive.  Otherwise, show the
                //  immediate parent and all the children that can be shown.
                //  This all was done to meet the UITF spec.
                //
                i = 0;
            }
            else
            {
                //
                //  Show as many children as possible.
                //
                if ((i = (SHORT)(pOFI->idirSub - 2)) < 0)
                {
                    i = 0;
                }
            }

            //
            //  LB_SETTOPINDEX must be after LB_SETCURSEL, as LB_SETCURSEL will
            //  alter the top index to bring the current selection into view.
            //
            SendMessage(hDirList, LB_SETTOPINDEX, (WPARAM)i, 0L);
        }
        else
        {
            SetDlgItemText(hDlg, stc1, szNull);
        }

        wNoRedraw &= ~2;
        SendMessage(hDirList, WM_SETREDRAW, TRUE, 0L);

        GetWindowRect(hDirList, (LPRECT)&rDirLBox);
        rDirLBox.left++, rDirLBox.top++;
        rDirLBox.right--, rDirLBox.bottom--;
        MapWindowPoints(NULL, hDlg, (LPPOINT)&rDirLBox, 2);

        //
        //  If there are less than enough directories to fill the listbox,
        //  Win 3.0 doesn't clear out the bottom.  Pass TRUE as the last
        //  parameter to demand a WM_ERASEBACKGROUND message.
        //
        InvalidateRect(hDlg, (LPRECT)&rDirLBox, (BOOL)(wWinVer < 0x030A));
    }

    SendMessage(hFileList, WM_SETREDRAW, TRUE, 0L);
    InvalidateRect(hFileList, (LPRECT)0, (BOOL)TRUE);

#ifndef WIN32
   ResetDTAAddress();
#endif

   HourGlass(FALSE);
   return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  OKButtonPressed
//
//  Note:  There are 4 cases for validation of a file name:
//    1)  OFN_NOVALIDATE        allows invalid characters
//    2)  No validation flags   No invalid characters, but path need not exist
//    3)  OFN_PATHMUSTEXIST     No invalid characters, path must exist
//    4)  OFN_FILEMUSTEXIST     No invalid characters, path & file must exist
//
////////////////////////////////////////////////////////////////////////////

BOOL OKButtonPressed(
    HWND hDlg,
    POPENFILEINFO pOFI,
    BOOL bSave)
{
    DWORD nErrCode = 0;
    DWORD cch;
    DWORD cchSearchPath;
    LPOPENFILENAME pOFN = pOFI->pOFN;
    int nFileOffset, nExtOffset;
    HANDLE hFile;
    BOOL bAddExt = FALSE;
    BOOL bUNCName = FALSE;
    int nTempOffset;
    TCHAR szPathName[MAX_FULLPATHNAME];
    DWORD lRet;
    BOOL blfn;
    LPCURDLG lpCurDlg;
    TCHAR ch = 0;


    if (cch = GetUNCDirectoryFromLB(hDlg, lst2, pOFI))
    {
        nTempOffset = (WORD)(DWORD)SendDlgItemMessage( hDlg,
                                                       lst2,
                                                       LB_GETTEXTLEN,
                                                       0,
                                                       0 );
    }
    else
    {
        nTempOffset = 0;
    }

    GetDlgItemText(hDlg, edt1, pOFI->szPath + cch, MAX_FULLPATHNAME - 1);

    if (cch)
    {
        //
        //  If a drive or new UNC was specified, forget the old UNC.
        //
        if ((pOFI->szPath[cch + 1] == CHAR_COLON) ||
            (DBL_BSLASH(pOFI->szPath + cch)) )
        {
            lstrcpy(pOFI->szPath, pOFI->szPath + cch);
        }
        else if ((ISBACKSLASH(pOFI->szPath, cch)) ||
                 (pOFI->szPath[cch] == CHAR_SLASH))
        {
            //
            //  If a directory from the root is given, put it immediately
            //  after the \\server\share listing.
            //
            lstrcpy(pOFI->szPath + nTempOffset, pOFI->szPath + cch);
        }
    }

    if (pOFN->Flags & OFN_NOLONGNAMES)
    {
        blfn = FALSE;
    }
    else
    {
        blfn = IsLFNDriveX(hDlg, pOFI->szPath);
    }

    lRet = ParseFile(pOFI->szPath, blfn, IS16BITWOWAPP(pOFN), FALSE);
    nFileOffset = (int)(SHORT)LOWORD(lRet);
    nExtOffset  = (int)(SHORT)HIWORD(lRet);

    if (nFileOffset == PARSE_EMPTYSTRING)
    {
        UpdateListBoxes(hDlg, pOFI, NULL, 0);
        return (FALSE);
    }
    else if ((nFileOffset != PARSE_DIRECTORYNAME) &&
             (pOFN->Flags & OFN_NOVALIDATE))
    {
        pOFN->nFileOffset = (WORD)nFileOffset;
        pOFN->nFileExtension = (WORD)nExtOffset;
        if (pOFN->lpstrFile)
        {
            cch = lstrlen(pOFI->szPath);
            if (cch < pOFN->nMaxFile)
            {
                lstrcpy(pOFN->lpstrFile, pOFI->szPath);
            }
            else
            {
                //
                //  For single file requests, we will never go over 64K
                //  because the filesystem is limited to 256.
                //
                if (cch > 0x0000FFFF)
                {
                    pOFN->lpstrFile[0] = (TCHAR)0xFFFF;
                }
                else
                {
                    pOFN->lpstrFile[0] = (TCHAR)LOWORD(cch);
                }
                pOFN->lpstrFile[1] = CHAR_NULL;
            }
        }
        return (TRUE);
    }
    else if ((pOFN->Flags & OFN_ALLOWMULTISELECT) &&
             SpacesExist(pOFI->szPath))
    {
        return (MultiSelectOKButton(hDlg, pOFI, bSave));
    }
    else if (pOFI->szPath[nExtOffset] == CHAR_SEMICOLON)
    {
        pOFI->szPath[nExtOffset] = CHAR_NULL;
        nFileOffset = (int)(SHORT)LOWORD(ParseFile( pOFI->szPath,
                                                    blfn,
                                                    IS16BITWOWAPP(pOFN),
                                                    FALSE ));
        pOFI->szPath[nExtOffset] = CHAR_SEMICOLON;
        if ( (nFileOffset >= 0) &&
             (StrChr(pOFI->szPath + nFileOffset, CHAR_STAR) ||
              StrChr(pOFI->szPath + nFileOffset, CHAR_QMARK)) )
        {
            lstrcpy(pOFI->szLastFilter, pOFI->szPath + nFileOffset);
            if (FListAll(pOFI, hDlg, pOFI->szPath) == CHANGEDIR_FAILED)
            {
                //
                //  Conform with cchSearchPath error code settings in
                //  PathCheck.
                //
                cchSearchPath = 2;
                goto PathCheck;
            }
            return (FALSE);
        }
        else
        {
            nFileOffset = PARSE_INVALIDCHAR;
            goto Warning;
        }
    }
    else if (nFileOffset == PARSE_DIRECTORYNAME)
    {
        //
        //  End with slash?
        //
        if ((ISBACKSLASH(pOFI->szPath, nExtOffset - 1)) ||
            (pOFI->szPath[nExtOffset - 1] == CHAR_SLASH))
        {
            //
            //  ... and is not the root, get rid of the slash.
            //
            if ( (nExtOffset != 1) &&
                 (pOFI->szPath[nExtOffset - 2] != CHAR_COLON) &&
                 (nExtOffset != nTempOffset + 1) )
            {
                pOFI->szPath[nExtOffset - 1] = CHAR_NULL;
            }
        }
        else if ((pOFI->szPath[nExtOffset - 1] == CHAR_DOT) &&
                 ((pOFI->szPath[nExtOffset - 2] == CHAR_DOT) ||
                  (ISBACKSLASH(pOFI->szPath, nExtOffset - 2)) ||
                  (pOFI->szPath[nExtOffset - 2] == CHAR_SLASH)) &&
                 ((DBL_BSLASH(pOFI->szPath)) ||
                  ((*(pOFI->szPath + 1) == CHAR_COLON) &&
                   (DBL_BSLASH(pOFI->szPath + 2)))))
        {
            pOFI->szPath[nExtOffset] = CHAR_BSLASH;
            pOFI->szPath[nExtOffset + 1] = CHAR_NULL;
        }

        //
        //  Fall through to Directory Checking.
        //
    }
    else if (nFileOffset < 0)
    {
        //
        //  Put in nErrCode so that call can be used from other points.
        //
        nErrCode = (DWORD)nFileOffset;
Warning:

        //
        //  If the disk is not a floppy and they tell me there's no
        //  disk in the drive, dont believe it.  Instead, put up the error
        //  message that they should have given us.
        //  (Note that the error message is checked first since checking
        //  the drive type is slower.)
        //
        if (nErrCode == ERROR_ACCESS_DENIED)
        {
            if (bUNCName)
            {
                nErrCode = ERROR_NETWORK_ACCESS_DENIED;
            }
            else
            {
                szPathName[0] = (TCHAR)CharLower((LPTSTR)(DWORD)szPathName[0]);

                if (GetDiskType(szPathName) == DRIVE_REMOTE)
                {
                    nErrCode = ERROR_NETWORK_ACCESS_DENIED;
                }
                else if (GetDiskType(szPathName) == DRIVE_REMOVABLE)
                {
                    nErrCode = ERROR_NO_DISK_IN_DRIVE;
                }
                else if (GetDiskType(szPathName) == DRIVE_CDROM)
                {
                    nErrCode = ERROR_NO_DISK_IN_CDROM;
                }
            }
        }

        if ((nErrCode == ERROR_WRITE_PROTECT) ||
            (nErrCode == ERROR_CANNOT_MAKE) ||
            (nErrCode == ERROR_NO_DISK_IN_DRIVE) ||
            (nErrCode == ERROR_NO_DISK_IN_CDROM))
        {
            pOFI->szPath[0] = szPathName[0];
        }

        InvalidFileWarning(hDlg, pOFI->szPath, nErrCode, 0);

        //
        //  Can't cd case (don't want WM_ACTIVATE to setevent to GetNetDrives!).
        //  Reset wNoRedraw.
        //
        wNoRedraw &= ~1;
        return (FALSE);
    }

    bUNCName = ((DBL_BSLASH(pOFI->szPath)) ||
                ((*(pOFI->szPath + 1) == CHAR_COLON) &&
                (DBL_BSLASH(pOFI->szPath + 2))));

    nTempOffset = nFileOffset;

    //
    //  Get the fully-qualified path.
    //
    {
        BOOL bSlash;
        BOOL bRet;
        WORD nNullOffset;

        if (nFileOffset != PARSE_DIRECTORYNAME)
        {
            ch = *(pOFI->szPath + nFileOffset);
            *(pOFI->szPath + nFileOffset) = CHAR_NULL;
            nNullOffset = (WORD) nFileOffset;
        }

        //
        //  For files of the format c:filename where c is not the
        //  current directory, SearchPath does not return the curdir of c
        //  so, prefetch it - should searchpath be changed?
        //
        if (nFileOffset)
        {
            if (*(pOFI->szPath + nFileOffset - 1) == CHAR_COLON)
            {
                //
                //  If it fails, fall through to the error generated below.
                //
                if (ChangeDir(hDlg, pOFI->szPath, FALSE, FALSE) != CHANGEDIR_FAILED)
                {
                    //
                    //  Replace old null offset.
                    //
                    *(pOFI->szPath + nFileOffset) = ch;
                    ch = *pOFI->szPath;

                    //
                    //  Don't pass drive-colon into search path.
                    //
                    *pOFI->szPath = CHAR_NULL;
                    nNullOffset = 0;
                }
            }
        }

        if (bSlash = (*pOFI->szPath == CHAR_SLASH))
        {
            *pOFI->szPath = CHAR_BSLASH;
        }

        szPathName[0] = CHAR_NULL;

        HourGlass(TRUE);

        //
        //  BUGBUG:
        //  Each wow thread can change the current directory.
        //  Since searchpath doesn't check current dirs on a per thread basis,
        //  reset it here and hope that we don't get interrupted between
        //  setting and searching...
        //
        lpCurDlg = (LPCURDLG)TlsGetValue(g_tlsiCurDlg);
        SetCurrentDirectory(lpCurDlg ? lpCurDlg->lpstrCurDir : NULL);

        if (pOFI->szPath[0] == TEXT('\0'))  // space for name (pretend it's valid for now)
        {
            lstrcpy(szPathName, (lpCurDlg ? lpCurDlg->lpstrCurDir : NULL));
            bRet = 1;
        }
        else
        {
            bRet = GetFullPathName( pOFI->szPath,
                                    MAX_FULLPATHNAME,
                                    szPathName,
                                    NULL );
        }

        if (!bRet && (pOFI->szPath[1] == CHAR_COLON))
        {
            int nDriveIndex = DiskAddedPreviously(pOFI->szPath[0], NULL);

            //
            //  If it's a remembered connection, try to reconnect it.
            //
            if (nDriveIndex != 0xFFFFFFFF  &&
                gaDiskInfo[nDriveIndex].dwType == REMDRVBMP)
            {
                DWORD err = WNetRestoreConnection( hDlg,
                                                   gaDiskInfo[nDriveIndex].lpPath );

                if (err == WN_SUCCESS)
                {
                    gaDiskInfo[nDriveIndex].dwType = NETDRVBMP;
                    nDriveIndex = (int) SendDlgItemMessage(
                           hDlg,
                           cmb2,
                           CB_SELECTSTRING,
                           (WPARAM)-1,
                           (LPARAM)(LPTSTR)gaDiskInfo[nDriveIndex].lpPath );
                    SendDlgItemMessage( hDlg,
                                        cmb2,
                                        CB_SETITEMDATA,
                                        (WPARAM)nDriveIndex,
                                        (LPARAM)NETDRVBMP );
                    bRet = GetFullPathName( pOFI->szPath,
                                            MAX_FULLPATHNAME,
                                            szPathName,
                                            NULL);
                }
            }
        }
        HourGlass(FALSE);

        if (nFileOffset != PARSE_DIRECTORYNAME)
        {
            *(pOFI->szPath + nNullOffset) = ch;
        }

        if (bSlash)
        {
            *pOFI->szPath = CHAR_SLASH;
        }

        if (bRet)
        {
            cchSearchPath = 0;

            if (nFileOffset != PARSE_DIRECTORYNAME)
            {
                ch = *(szPathName + lstrlen(szPathName) - 1);
                if (!ISBACKSLASH(szPathName, lstrlen(szPathName) - 1))
                {
                    lstrcat(szPathName, TEXT("\\"));
                }
                lstrcat(szPathName, (LPTSTR)(pOFI->szPath + nFileOffset));
            }
            else
            {
                //
                //  Hack to get around SearchPath inconsistencies.
                //
                //  searching for c: returns c:
                //  searching for server share dir1 .. returns  server share
                //  in these two cases bypass the regular ChangeDir call that
                //  uses szPathName and use the original pOFI->szPath instead
                //  OKButtonPressed needs to be simplified!
                //
                int cch = GetPathOffset(pOFI->szPath);

                if (cch > 0)
                {
                    if (bUNCName)
                    {
                        //
                        //  If this fails, how is szPathName used?
                        //  szPathName's disk should equal pOFI->szPath's
                        //  so the cch will be valid.
                        //
                        szPathName[cch] = CHAR_BSLASH;
                        szPathName[cch + 1] = CHAR_NULL;
                        if (ChangeDir( hDlg,
                                       pOFI->szPath,
                                       FALSE,
                                       TRUE ) != CHANGEDIR_FAILED)
                        {
                            goto ChangedDir;
                        }
                    }
                    else
                    {
                        if (!pOFI->szPath[cch])
                        {
                            if (ChangeDir( hDlg,
                                           pOFI->szPath,
                                           FALSE,
                                           TRUE) != CHANGEDIR_FAILED)
                            {
                                goto ChangedDir;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            if (!(pOFN->Flags & OFN_PATHMUSTEXIST))
            {
                lstrcpy(szPathName, pOFI->szPath);
            }
            if (((nErrCode = GetLastError()) == ERROR_INVALID_DRIVE) ||
                (pOFI->szPath[1] == CHAR_COLON))
            {
                cchSearchPath = 1;
            }
            else
            {
                cchSearchPath = 2;
            }
        }
    }

    //
    //  Full pattern?
    //
    if ( !cchSearchPath &&
         ((StrChr(pOFI->szPath + nFileOffset, CHAR_STAR)) ||
          (StrChr(pOFI->szPath + nFileOffset, CHAR_QMARK))) )
    {
        TCHAR szSameDirFile[MAX_FULLPATHNAME];

        if (nTempOffset)
        {
            //
            //  Must restore character in case it is part of the filename,
            //  e.g. nTempOffset is 1 for "\foo.txt".
            //
            ch = pOFI->szPath[nTempOffset];
            pOFI->szPath[nTempOffset] = 0;
            ChangeDir(hDlg, pOFI->szPath, FALSE, TRUE);
            pOFI->szPath[nTempOffset] = ch;
        }
        if (!nExtOffset)
        {
            lstrcat(pOFI->szPath + nFileOffset, TEXT("."));
        }
        lstrcpy(szSameDirFile, pOFI->szPath + nFileOffset);
        lstrcpy(pOFI->szLastFilter, pOFI->szPath + nFileOffset);

        if (FListAll(pOFI, hDlg, szSameDirFile) < 0)
        {
            MessageBeep(0);
        }
        return (FALSE);
    }

    //
    //  We either have a file pattern or a real file.
    //  If its a directory
    //       (1) Add on default pattern
    //       (2) Act like its a pattern (goto pattern (1))
    //  Else if its a pattern
    //       (1) Update everything
    //       (2) display files in whatever dir were now in
    //  Else if its a file name!
    //       (1) Check out the syntax
    //       (2) End the dialog given OK
    //       (3) Beep/message otherwise
    //

    //
    //  Drive-letter:\dirpath ??
    //
    if (!cchSearchPath)
    {
        DWORD dwFileAttr;

        if ((dwFileAttr = GetFileAttributes(szPathName)) != 0xFFFFFFFF)
        {
            if (dwFileAttr & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (ChangeDir(hDlg, szPathName, FALSE, TRUE) != CHANGEDIR_FAILED)
                {
ChangedDir:
                    SendDlgItemMessage(hDlg, edt1, WM_SETREDRAW, FALSE, 0L);
                    if (*pOFI->szLastFilter)
                    {
                        SetDlgItemText(hDlg, edt1, pOFI->szLastFilter);
                    }
                    else
                    {
                        SetDlgItemText(hDlg, edt1, szStarDotStar);
                    }

                    SendMessage( hDlg,
                                 WM_COMMAND,
                                 GET_WM_COMMAND_MPS( cmb1,
                                                     GetDlgItem(hDlg, cmb1),
                                                     CBN_CLOSEUP ) );
                    SendMessage( hDlg,
                                 WM_COMMAND,
                                 GET_WM_COMMAND_MPS( cmb2,
                                                     GetDlgItem(hDlg, cmb2),
                                                     MYCBN_CHANGEDIR ) );

                    SendDlgItemMessage(hDlg, edt1, WM_SETREDRAW, TRUE, 0L);
                    InvalidateRect(GetDlgItem(hDlg, edt1), NULL, FALSE);
                }
                return (FALSE);
            }
        }
    }

    //
    //  Was there a path and did it fail?
    //
    if (nFileOffset && cchSearchPath && (pOFN->Flags & OFN_PATHMUSTEXIST))
    {
PathCheck:
        if (cchSearchPath == 2)
        {
            nErrCode = ERROR_PATH_NOT_FOUND;
        }
        else if (cchSearchPath == 1)
        {
            int nDriveIndex;

            //
            //  Lowercase drive letters since DiskAddedPreviously is case
            //  sensitive.
            //
            CharLower(pOFI->szPath);

            //  We can get here without performing an OpenFile call.  As such
            //  the szPathName can be filled with random garbage.  Since we
            //  only need one character for the error message, set
            //  szPathName[0] to the drive letter.
            //
            if (pOFI->szPath[1] == CHAR_COLON)
            {
                nDriveIndex = DiskAddedPreviously(pOFI->szPath[0], NULL);
            }
            else
            {
                nDriveIndex = DiskAddedPreviously(0, pOFI->szPath);
            }

            if (nDriveIndex == 0xFFFFFFFF)
            {
                nErrCode = ERROR_NO_DRIVE;
            }
            else
            {
                if (bUNCName)
                {
                    nErrCode = ERROR_NO_DRIVE;
                }
                else
                {
                    switch (GetDiskType(pOFI->szPath))
                    {
                        case ( DRIVE_REMOVABLE ) :
                        {
                            szPathName[0] = pOFI->szPath[0];
                            nErrCode = ERROR_NO_DISK_IN_DRIVE;
                            break;
                        }
                        case ( DRIVE_CDROM ) :
                        {
                           szPathName[0] = pOFI->szPath[0];
                           nErrCode = ERROR_NO_DISK_IN_CDROM;
                           break;
                        }
                        default :
                        {
                           nErrCode = ERROR_PATH_NOT_FOUND;
                        }
                    }
                }
            }
        }
        else
        {
            nErrCode = ERROR_FILE_NOT_FOUND;
        }

        //
        //  If we don't set wNoRedraw here, then WM_ACTIVATE will set the
        //  GetNetDrives event.
        //
        wNoRedraw |= 1;

        goto Warning;
    }

    if (PortName(pOFI->szPath + nFileOffset))
    {
        nErrCode = ERROR_PORTNAME;
        goto Warning;
    }

#if 0
    //
    //  Check if we've received a string in the form "C:filename.ext".
    //  If we have, convert it to the form "C:.\filename.ext".  This is done
    //  because the kernel will search the entire path, ignoring the drive
    //  specification after the initial search.  Making it include a slash
    //  causes kernel to only search at that location.
    //  Note:  Only increment nExtOffset, not nFileOffset.  This is done
    //  because only nExtOffset is used later, and nFileOffset can then be
    //  used at the Warning: label to determine if this hack has occurred,
    //  and thus it can strip out the ".\" when putting out the error.
    //
    if ((nFileOffset == 2) && (pOFI->szPath[1] == CHAR_COLON))
    {
        lstrcpy(szWarning, pOFI->szPath + 2);
        lstrcpy(pOFI->szPath + 4, szWarning);
        pOFI->szPath[2] = CHAR_DOT;
        pOFI->szPath[3] = CHAR_BSLASH;
        nExtOffset += 2;
    }
#endif

    //
    //  Add the default extension unless filename ends with period or no
    //  default extension exists.  If the file exists, consider asking
    //  permission to overwrite the file.
    //
    //  NOTE:  When no extension given, default extension is tried 1st.
    //
    if ( (nFileOffset != PARSE_DIRECTORYNAME) &&
         nExtOffset &&
         !pOFI->szPath[nExtOffset] &&
         pOFN->lpstrDefExt &&
         *pOFN->lpstrDefExt &&
         (((DWORD)nExtOffset + lstrlen(pOFN->lpstrDefExt)) < pOFN->nMaxFile) )
    {
        DWORD dwFileAttr;
        int nExtOffset2 = lstrlen(szPathName);

        bAddExt = TRUE;

        AppendExt(pOFI->szPath, pOFN->lpstrDefExt, FALSE);
        AppendExt(szPathName, pOFN->lpstrDefExt, FALSE);

        //
        //  Directory may match default extension.  Change to it as if it had
        //  been typed in.  A dir w/o the extension would have been switched
        //  to in the logic above.
        //
        if ((dwFileAttr = GetFileAttributes(pOFI->szPath)) != 0xFFFFFFFF)
        {
            if (dwFileAttr & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (ChangeDir(hDlg, szPathName, FALSE, TRUE) != CHANGEDIR_FAILED)
                {
                    goto ChangedDir;
                }
            }
        }

        hFile = CreateFile( szPathName,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL );
        if (hFile == INVALID_HANDLE_VALUE)
        {
            nErrCode = GetLastError();

            //
            //  Fix bug where progman cannot OK a file being browsed for new
            //  item because it has Execute only permission.
            //
            if (nErrCode == ERROR_ACCESS_DENIED)
            {
                hFile = CreateFile( szPathName,
                                    GENERIC_EXECUTE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL );
                if (hFile == INVALID_HANDLE_VALUE)
                {
                    nErrCode = GetLastError();
                }
            }
        }

        if (nErrCode == ERROR_SHARING_VIOLATION)
        {
            goto SharingViolationInquiry;
        }

        if (hFile != INVALID_HANDLE_VALUE)
        {
            if (!CloseHandle(hFile))
            {
                nErrCode = GetLastError();
                goto Warning;
            }

AskPermission:
            //
            //  Is the file read-only?
            //
            if (pOFN->Flags & OFN_NOREADONLYRETURN)
            {
                int nRet;
                if ((nRet = GetFileAttributes(szPathName)) != -1)
                {
                    if (nRet & ATTR_READONLY)
                    {
                        nErrCode = ERROR_LAZY_READONLY;
                        goto Warning;
                    }
                }
                else
                {
                    nErrCode = GetLastError();
                    goto Warning;
                }
            }

            if ((bSave || (pOFN->Flags & OFN_NOREADONLYRETURN)) &&
                (nErrCode == ERROR_ACCESS_DENIED))
            {
                goto Warning;
            }

            if (pOFN->Flags & OFN_OVERWRITEPROMPT)
            {
                if (bSave && !FOkToWriteOver(hDlg, szPathName))
                {
                    PostMessage( hDlg,
                                 WM_NEXTDLGCTL,
                                 (WPARAM)GetDlgItem(hDlg, edt1),
                                 (LPARAM)1L );
                    return (FALSE);
                }
            }

            if (nErrCode == ERROR_SHARING_VIOLATION)
            {
                goto SharingViolationInquiry;
            }
            goto FileNameAccepted;
        }
        else
        {
            *(pOFI->szPath + nExtOffset) = CHAR_NULL;
            szPathName[nExtOffset2] = CHAR_NULL;
        }
    }
    else
    {
        //
        //  Extension should not be added.
        //
        bAddExt = FALSE;
    }

    hFile = CreateFile( szPathName,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );
    if (hFile == INVALID_HANDLE_VALUE)
    {
        nErrCode = GetLastError();

        //
        //  Fix bug where progman cannot OK a file being browsed for new item
        //  because it has Execute only permission.
        //
        if (nErrCode == ERROR_ACCESS_DENIED)
        {
            hFile = CreateFile( szPathName,
                                GENERIC_EXECUTE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL );
            if (hFile == INVALID_HANDLE_VALUE)
            {
                nErrCode = GetLastError();
            }
        }
    }

    if (hFile != INVALID_HANDLE_VALUE)
    {
        if (!CloseHandle(hFile))
        {
            nErrCode = GetLastError();
            goto Warning;
        }
        goto AskPermission;
    }
    else
    {
        if ((nErrCode == ERROR_FILE_NOT_FOUND) ||
            (nErrCode == ERROR_PATH_NOT_FOUND))
        {
            //
            //  Figure out if the default extension should be tacked on.
            //
            if (bAddExt)
            {
                AppendExt(pOFI->szPath, pOFN->lpstrDefExt, FALSE);
                AppendExt(szPathName, pOFN->lpstrDefExt, FALSE);
            }
        }
        else if (nErrCode == ERROR_SHARING_VIOLATION)
        {

SharingViolationInquiry:
            //
            //  If the app is "share aware", fall through.
            //  Otherwise, ask the hook function.
            //
            if (!(pOFN->Flags & OFN_SHAREAWARE))
            {
                if (pOFN->lpfnHook)
                {
                    LPOFNHOOKPROC lpfnHook = GETHOOKFN(pOFN);

#ifdef UNICODE
                    if (pOFI->ApiType == COMDLG_ANSI)
                    {
                        CHAR szPathNameA[MAX_FULLPATHNAME];

                        RtlUnicodeToMultiByteSize(
                              &cch,
                              szPathName,
                              lstrlenW(szPathName) * sizeof(TCHAR) );

                        SHUnicodeToAnsi(szPathName,(LPSTR)&szPathNameA[0],cch + 1);

                        cch = (DWORD)(*lpfnHook)( hDlg,
                                           msgSHAREVIOLATIONA,
                                           0,
                                           (LONG_PTR)(LPSTR)szPathNameA );
                    }
                    else
#endif
                    {
                        cch = (DWORD)(*lpfnHook)( hDlg,
                                           msgSHAREVIOLATIONW,
                                           0,
                                           (LONG_PTR)szPathName );
                    }
                    if (cch == OFN_SHARENOWARN)
                    {
                        return (FALSE);
                    }
                    else if (cch != OFN_SHAREFALLTHROUGH)
                    {
                        goto Warning;
                    }
                }
                else
                {
                    goto Warning;
                }
            }
            goto FileNameAccepted;
        }

        if (!bSave)
        {
            if ((nErrCode == ERROR_FILE_NOT_FOUND) ||
                (nErrCode == ERROR_PATH_NOT_FOUND))
            {
                if (pOFN->Flags & OFN_FILEMUSTEXIST)
                {
                    if (pOFN->Flags & OFN_CREATEPROMPT)
                    {
                        //
                        //  Don't alter pOFI->szPath.
                        //
                        bInChildDlg = TRUE;
                        cch = (DWORD)CreateFileDlg(hDlg, pOFI->szPath);
                        bInChildDlg = FALSE;
                        if (cch == IDYES)
                        {
                            goto TestCreation;
                        }
                        else
                        {
                            return (FALSE);
                        }
                    }
                    goto Warning;
                }
            }
            else
            {
                goto Warning;
            }
        }

        //
        //  The file doesn't exist.  Can it be created?  This is needed because
        //  there are many extended characters which are invalid that won't be
        //  caught by ParseFile.
        //  Two more good reasons:  Write-protected disks & full disks.
        //
        //  BUT, if they dont want the test creation, they can request that we
        //  not do it using the OFN_NOTESTFILECREATE flag.  If they want to
        //  create files on a share that has create-but-no-modify privileges,
        //  they should set this flag but be ready for failures that couldn't
        //  be caught, such as no create privileges, invalid extended
        //  characters, a full disk, etc.
        //

TestCreation:
        if ((pOFN->Flags & OFN_PATHMUSTEXIST) &&
            (!(pOFN->Flags & OFN_NOTESTFILECREATE)))
        {
            //
            //  Must use the FILE_FLAG_DELETE_ON_CLOSE flag so that the
            //  file is automatically deleted when the handle is closed
            //  (no need to call DeleteFile).  This is necessary in the
            //  event that the directory only has Add & Read access.
            //  The CreateFile call will succeed, but the DeleteFile call
            //  will fail.  By adding the above flag to the CreateFile
            //  call, it overrides the access rights and deletes the file
            //  during the call to CloseHandle.
            //
#ifdef WINNT
            hFile = CreateFile( szPathName,
                                FILE_ADD_FILE,
                                0,
                                NULL,
                                CREATE_NEW,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
                                NULL );
#else

            // Win95/Memphis don't support FILE_ADD_FILE flag, use GENERIC_READ instead.

            hFile = CreateFile( szPathName,
                                GENERIC_READ,
                                0,
                                NULL,
                                CREATE_NEW,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
                                NULL );
#endif
            if (hFile == INVALID_HANDLE_VALUE)
            {
                nErrCode = GetLastError();
            }

            if (hFile != INVALID_HANDLE_VALUE)
            {
                if (!CloseHandle(hFile))
                {
                    nErrCode = GetLastError();
                    goto Warning;
                }
            }
            else
            {
                //
                //  Unable to create it.
                //
                //  If it's not write-protection, a full disk,
                //  network protection, or the user popping the drive door
                //  open, assume that the filename is invalid.
                //
                if ( (nErrCode != ERROR_WRITE_PROTECT) &&
                     (nErrCode != ERROR_CANNOT_MAKE) &&
                     (nErrCode != ERROR_NETWORK_ACCESS_DENIED) &&
                     (nErrCode != ERROR_ACCESS_DENIED) )
                {
                    nErrCode = 0;
                }
                goto Warning;
            }
        }
    }

FileNameAccepted:

    HourGlass(TRUE);

    lRet = ParseFile(szPathName, blfn, IS16BITWOWAPP(pOFN), FALSE);
    nFileOffset = (int)(SHORT)LOWORD(lRet);
    cch = (DWORD)HIWORD(lRet);

    pOFN->nFileOffset = (WORD)nFileOffset;
    if (nExtOffset || bAddExt)
    {
        pOFN->nFileExtension = LOWORD(cch);
    }
    else
    {
        pOFN->nFileExtension = 0;
    }

    pOFN->Flags &= ~OFN_EXTENSIONDIFFERENT;
    if (pOFN->lpstrDefExt && pOFN->nFileExtension)
    {
        TCHAR szPrivateExt[4];
        SHORT i;

        for (i = 0; i < 3; i++)
        {
            szPrivateExt[i] = *(pOFN->lpstrDefExt + i);
        }
        szPrivateExt[3] = CHAR_NULL;

        if (lstrcmpi(szPrivateExt, szPathName + cch))
        {
            pOFN->Flags |= OFN_EXTENSIONDIFFERENT;
        }
    }

    //
    //  If we're called from wow, and the user hasn't changed
    //  directories, shorten the path to abbreviated 8.3 format.
    //
    if (pOFN->Flags & OFN_NOLONGNAMES)
    {
        ShortenThePath(szPathName);

        //
        //  If the path was shortened, the offset might have changed so
        //  we must parse the file again.
        //
        lRet = ParseFile(szPathName, blfn, IS16BITWOWAPP(pOFN), FALSE);
        nFileOffset = (int)(SHORT)LOWORD(lRet);
        cch  = (DWORD)HIWORD(lRet);

        //
        //  When in Save dialog, the file may not exist yet, so the file
        //  name cannot be shortened.  So, we need to test if it's an
        //  8.3 filename and popup an error message if not.
        //
        if (bSave)
        {
            LPTSTR lptmp;
            LPTSTR lpExt = NULL;

            for (lptmp = szPathName + nFileOffset; *lptmp; lptmp++)
            {
                if (*lptmp == CHAR_DOT)
                {
                    if (lpExt)
                    {
                        //
                        //  There's more than one dot in the file, so it is
                        //  invalid.
                        //
                        nErrCode = FNERR_INVALIDFILENAME;
                        goto Warning;
                    }
                    lpExt = lptmp;
                }
                if (*lptmp == CHAR_SPACE)
                {
                    nErrCode = FNERR_INVALIDFILENAME;
                    goto Warning;
                }
            }

            if (lpExt)
            {
                //
                //  There's an extension.
                //
                *lpExt = 0;
            }

            if ((lstrlen(szPathName + nFileOffset) > 8) ||
                (lpExt && lstrlen(lpExt + 1) > 3))
            {
                if (lpExt)
                {
                    *lpExt = CHAR_DOT;
                }

                nErrCode = FNERR_INVALIDFILENAME;
                goto Warning;
            }
            if (lpExt)
            {
                *lpExt = CHAR_DOT;
            }
        }
    }

    if (pOFN->lpstrFile)
    {
        DWORD cchLen = lstrlen(szPathName);

        if (cchLen < pOFN->nMaxFile)
        {
            lstrcpy(pOFN->lpstrFile, szPathName);
        }
        else
        {
            //
            //  Buffer is too small, so return the size of the buffer
            //  required to hold the string.
            //
            //  For single file requests, we will never go over 64K
            //  because the filesystem is limited to 256.
            //
#ifdef UNICODE
            pOFN->lpstrFile[0] = (TCHAR)LOWORD(cchLen);
            if (pOFN->nMaxFile >= 2)
            {
                pOFN->lpstrFile[1] = CHAR_NULL;
            }
#else
            pOFN->lpstrFile[0] = LOBYTE(cchLen);
            pOFN->lpstrFile[1] = HIBYTE(cchLen);
            pOFN->lpstrFile[2] = CHAR_NULL;
#endif
        }
    }

    //
    //  File Title.  Note that it's cut off at whatever the buffer length
    //               is, so if the buffer is too small, no notice is given.
    //
    if (pOFN->lpstrFileTitle && pOFN->nMaxFileTitle)
    {
        cch = lstrlen(szPathName + nFileOffset);
        if (cch > pOFN->nMaxFileTitle)
        {
            szPathName[nFileOffset + pOFN->nMaxFileTitle - 1] = CHAR_NULL;
        }
        lstrcpy(pOFN->lpstrFileTitle, szPathName + nFileOffset);
    }


    if (pOFN->Flags | OFN_READONLY)
    {
        if (IsDlgButtonChecked(hDlg, chx1))
        {
            pOFN->Flags |= OFN_READONLY;
        }
        else
        {
            pOFN->Flags &= ~OFN_READONLY;
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  MultiSelectOKButton
//
////////////////////////////////////////////////////////////////////////////

BOOL MultiSelectOKButton(
    HWND hDlg,
    POPENFILEINFO pOFI,
    BOOL bSave)
{
    DWORD nErrCode;
    LPTSTR lpCurDir;
    LPTSTR lpchStart;                  // start of an individual filename
    LPTSTR lpchEnd;                    // end of an individual filename
    DWORD cch;
    HANDLE hFile;
    LPOPENFILENAME pOFN;
    BOOL EOS = FALSE;                  // end of string flag
    BOOL bRet;
    TCHAR szPathName[MAX_FULLPATHNAME - 1];
    LPCURDLG lpCurDlg;


    pOFN = pOFI->pOFN;

    //
    //  Check for space for first full path element.
    //
    if(!(lpCurDlg = (LPCURDLG)TlsGetValue(g_tlsiCurDlg)) ||
       !(lpCurDir = lpCurDlg->lpstrCurDir))
    {
        return (FALSE);
    }

    lstrcpy(pOFI->szPath, lpCurDir);
    if (StrChr(pOFI->szPath, CHAR_SPACE))
    {
        GetShortPathName(pOFI->szPath, pOFI->szPath, MAX_FULLPATHNAME);
    }

    if (!bCasePreserved)
    {
        CharLower(pOFI->szPath);
    }

    cch = (DWORD) ( lstrlen(pOFI->szPath) +
            sizeof(TCHAR) +
            SendDlgItemMessage(hDlg, edt1, WM_GETTEXTLENGTH, 0, 0L) );
    if (pOFN->lpstrFile)
    {
        if (cch > pOFN->nMaxFile)
        {
            //
            //  Buffer is too small, so return the size of the buffer
            //  required to hold the string (if possible).
            //
            if (pOFN->nMaxFile >= 3)
            {
#ifdef UNICODE
                pOFN->lpstrFile[0] = (TCHAR)LOWORD(cch);
                pOFN->lpstrFile[1] = (TCHAR)HIWORD(cch);
#else
                pOFN->lpstrFile[0] = (TCHAR)LOBYTE(cch);
                pOFN->lpstrFile[1] = (TCHAR)HIBYTE(cch);
#endif
                pOFN->lpstrFile[2] = CHAR_NULL;
            }
            else
            {
#ifdef UNICODE
                pOFN->lpstrFile[0] = (TCHAR)LOWORD(cch);
                if (pOFN->nMaxFile == 2)
                {
                    pOFN->lpstrFile[1] = (TCHAR)HIWORD(cch);
                }
#else
                pOFN->lpstrFile[0] = LOBYTE(cch);
                pOFN->lpstrFile[1] = HIBYTE(cch);
                pOFN->lpstrFile[2] = CHAR_NULL;
#endif
            }
        }
        else
        {
            //
            //  Copy in the full path as the first element.
            //
            lstrcpy(pOFN->lpstrFile, pOFI->szPath);
            lstrcat(pOFN->lpstrFile, TEXT(" "));

            //
            //  Get the other files here.
            //
            cch = lstrlen(pOFN->lpstrFile);

            //
            //  The path is guaranteed to be less than 64K (actually, < 260).
            //
            pOFN->nFileOffset = LOWORD(cch);
            lpchStart = pOFN->lpstrFile + cch;

            GetDlgItemText( hDlg,
                            edt1,
                            lpchStart,
                            (int)(pOFN->nMaxFile - cch - 1) );

            while (*lpchStart == CHAR_SPACE)
            {
                lpchStart = CharNext(lpchStart);
            }
            if (*lpchStart == CHAR_NULL)
            {
                return (FALSE);
            }

            //
            //  Go along file path looking for multiple filenames delimited by
            //  spaces.  For each filename found, try to open it to make sure
            //  it's a valid file.
            //
            while (!EOS)
            {
                //
                //  Find the end of the filename.
                //
                lpchEnd = lpchStart;
                while (*lpchEnd && *lpchEnd != CHAR_SPACE)
                {
                    lpchEnd = CharNext(lpchEnd);
                }

                //
                //  Mark the end of the filename with a NULL.
                //
                if (*lpchEnd == CHAR_SPACE)
                {
                    *lpchEnd = CHAR_NULL;
                }
                else
                {
                    //
                    //  Already NULL, found the end of the string.
                    //
                    EOS = TRUE;
                }

                //
                //  Check that the filename is valid.
                //
                bRet = GetFullPathName( lpchStart,
                                        MAX_FULLPATHNAME,
                                        szPathName,
                                        NULL);

                if (!bRet)
                {
                    nErrCode = ERROR_FILE_NOT_FOUND;
                    goto MultiFileNotFound;
                }

                hFile = CreateFile( szPathName,
                                    GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL );

                //
                //  Fix bug where progman cannot OK a file being browsed for
                //  new item because it has Execute only permission.
                //
                if (hFile == INVALID_HANDLE_VALUE)
                {
                    nErrCode = GetLastError();
                    if (nErrCode == ERROR_ACCESS_DENIED)
                    {
                        hFile = CreateFile( szPathName,
                                            GENERIC_EXECUTE,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                                            NULL,
                                            OPEN_EXISTING,
                                            FILE_ATTRIBUTE_NORMAL,
                                            NULL );
                    }
                    else
                    {
                        goto MultiFileNotFound;
                    }
                }
                if (hFile == INVALID_HANDLE_VALUE)
                {
                    nErrCode = GetLastError();
MultiFileNotFound:
                    if ( ((pOFN->Flags & OFN_FILEMUSTEXIST) ||
                          (nErrCode != ERROR_FILE_NOT_FOUND)) &&
                         ((pOFN->Flags & OFN_PATHMUSTEXIST) ||
                          (nErrCode != ERROR_PATH_NOT_FOUND)) &&
                         (!(pOFN->Flags & OFN_SHAREAWARE) ||
                          (nErrCode != ERROR_SHARING_VIOLATION)) )
                    {
                        if ( (nErrCode == ERROR_SHARING_VIOLATION) &&
                             pOFN->lpfnHook )
                        {
                            LPOFNHOOKPROC lpfnHook = GETHOOKFN(pOFN);

#ifdef UNICODE
                            if (pOFI->ApiType == COMDLG_ANSI)
                            {
                                CHAR szPathNameA[MAX_FULLPATHNAME];

                                RtlUnicodeToMultiByteSize(
                                     &cch,
                                     szPathName,
                                     lstrlenW(szPathName) * sizeof(TCHAR) );

                                SHUnicodeToAnsi(szPathName,(LPSTR)&szPathNameA[0],cch + 1);

                                cch = (DWORD)(*lpfnHook)( hDlg,
                                                   msgSHAREVIOLATIONA,
                                                   0,
                                                   (LONG_PTR)(LPSTR)szPathNameA );
                            }
                            else
#endif
                            {
                                cch = (DWORD)(*lpfnHook)( hDlg,
                                                   msgSHAREVIOLATIONW,
                                                   0,
                                                   (LONG_PTR)szPathName );
                            }
                            if (cch == OFN_SHARENOWARN)
                            {
                                return (FALSE);
                            }
                            else if (cch == OFN_SHAREFALLTHROUGH)
                            {
                                goto EscapedThroughShare;
                            }
                        }
                        else if (nErrCode == ERROR_ACCESS_DENIED)
                        {
                            szPathName[0] =
                               (TCHAR)CharLower((LPTSTR)(DWORD)szPathName[0]);

                            if (GetDiskType(szPathName) != DRIVE_REMOVABLE)
                            {
                                nErrCode = ERROR_NETWORK_ACCESS_DENIED;
                            }
                        }
                        if ((nErrCode == ERROR_WRITE_PROTECT) ||
                            (nErrCode == ERROR_CANNOT_MAKE)   ||
                            (nErrCode == ERROR_ACCESS_DENIED))
                        {
                            *lpchStart = szPathName[0];
                        }
MultiWarning:
                        InvalidFileWarning(hDlg, lpchStart, nErrCode, 0);
                        return (FALSE);
                    }
                }
EscapedThroughShare:
                if (hFile != INVALID_HANDLE_VALUE)
                {
                    if (!CloseHandle(hFile))
                    {
                        nErrCode = GetLastError();
                        goto MultiWarning;
                    }
                    if ((pOFN->Flags & OFN_NOREADONLYRETURN) &&
                        (GetFileAttributes(szPathName) & FILE_ATTRIBUTE_READONLY))
                    {
                        nErrCode = ERROR_LAZY_READONLY;
                        goto MultiWarning;
                    }

                    if ((bSave || (pOFN->Flags & OFN_NOREADONLYRETURN)) &&
                        (nErrCode == ERROR_ACCESS_DENIED))
                    {
                        goto MultiWarning;
                    }

                    if (pOFN->Flags & OFN_OVERWRITEPROMPT)
                    {
                        if (bSave && !FOkToWriteOver(hDlg, szPathName))
                        {
                            PostMessage( hDlg,
                                         WM_NEXTDLGCTL,
                                         (WPARAM)GetDlgItem(hDlg, edt1),
                                         (LPARAM)1L );
                            return (FALSE);
                        }
                    }
                }

                //
                //  This file is valid, so check the next one.
                //
                if (!EOS)
                {
                    lpchStart = lpchEnd + 1;
                    while (*lpchStart == CHAR_SPACE)
                    {
                        lpchStart = CharNext(lpchStart);
                    }
                    if (*lpchStart == CHAR_NULL)
                    {
                        EOS = TRUE;
                    }
                    else
                    {
                        //
                        //  Not at end, replace NULL with SPACE.
                        //
                        *lpchEnd = CHAR_SPACE;
                    }
                }
            }

            //
            //  Limit String.
            //
            *lpchEnd = CHAR_NULL;
        }
    }

    //
    //  This doesn't really mean anything for multiselection.
    //
    pOFN->nFileExtension = 0;

    pOFN->nFilterIndex = (int) SendDlgItemMessage(hDlg, cmb1, CB_GETCURSEL, 0, 0L);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  dwOKSubclass
//
//  Simulates a double click if the user presses OK with the mouse
//  and the focus was on the directory listbox.
//
//  The problem is that the UITF demands that when the directory
//  listbox loses the focus, the selected directory should return
//  to the current directory.  But when the user changes the item
//  selected with a single click, and then clicks the OK button to
//  have the change take effect, the focus is lost before the OK button
//  knows it was pressed.  By setting the global flag bChangeDir
//  when the directory listbox loses the focus and clearing it when
//  the OK button loses the focus, we can check whether a mouse
//  click should update the directory.
//
//  Returns:  Return value from default listbox proceedure.
//
////////////////////////////////////////////////////////////////////////////

LRESULT WINAPI dwOKSubclass(
    HWND hOK,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    HANDLE hDlg;
    POPENFILEINFO pOFI;

    if (msg == WM_KILLFOCUS)
    {
        if (bChangeDir)
        {
            if (pOFI = (POPENFILEINFO)GetProp(hDlg = GetParent(hOK), FILEPROP))
            {
                SendDlgItemMessage( hDlg,
                                    lst2,
                                    LB_SETCURSEL,
                                    (WPARAM)(pOFI->idirSub - 1),
                                    0L );
            }
            bChangeDir = FALSE;
        }
    }
    return (CallWindowProc(lpOKProc, hOK, msg, wParam, lParam));
}


////////////////////////////////////////////////////////////////////////////
//
//  dwLBSubclass
//
//  Simulates a double click if the user presses OK with the mouse.
//
//  The problem is that the UITF demands that when the directory
//  listbox loses the focus, the selected directory should return
//  to the current directory.  But when the user changes the item
//  selected with a single click, and then clicks the OK button to
//  have the change take effect, the focus is lost before the OK button
//  knows it was pressed.  By simulating a double click, the change
//  takes place.
//
//  Returns:  Return value from default listbox proceedure.
//
////////////////////////////////////////////////////////////////////////////

LRESULT WINAPI dwLBSubclass(
    HWND hLB,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    HANDLE hDlg;
    POPENFILEINFO pOFI;

    if (msg == WM_KILLFOCUS)
    {
        hDlg = GetParent(hLB);
        bChangeDir = (GetDlgItem(hDlg, IDOK) == (HWND)wParam) ? TRUE : FALSE;
        if (!bChangeDir)
        {
            if (pOFI = (POPENFILEINFO)GetProp(hDlg, FILEPROP))
            {
                SendMessage( hLB,
                             LB_SETCURSEL,
                             (WPARAM)(pOFI->idirSub - 1),
                             0L );
            }
        }
    }
    return (CallWindowProc(lpLBProc, hLB, msg, wParam, lParam));
}


////////////////////////////////////////////////////////////////////////////
//
//  InvalidFileWarning
//
////////////////////////////////////////////////////////////////////////////

int InvalidFileWarning(
    HWND hDlg,
    LPTSTR szFile,
    DWORD wErrCode,
    UINT mbType)
{
    SHORT isz;
    BOOL bDriveLetter = FALSE;
    int nRet = 0;

    if (lstrlen(szFile) > TOOLONGLIMIT)
    {
        *(szFile + TOOLONGLIMIT) = CHAR_NULL;
    }

    switch (wErrCode)
    {
        case ( ERROR_NO_DISK_IN_DRIVE ) :
        {
            isz = iszNoDiskInDrive;
            bDriveLetter = TRUE;
            break;
        }
        case ( ERROR_NO_DISK_IN_CDROM ) :
        {
            isz = iszNoDiskInCDRom;
            bDriveLetter = TRUE;
            break;
        }
        case ( ERROR_NO_DRIVE ) :
        {
            isz = iszDriveDoesNotExist;
            bDriveLetter = TRUE;
            break;
        }
        case ( ERROR_TOO_MANY_OPEN_FILES ) :
        {
            isz = iszNoFileHandles;
            break;
        }
        case ( ERROR_PATH_NOT_FOUND ) :
        {
            isz = iszPathNotFound;
            break;
        }
        case ( ERROR_FILE_NOT_FOUND ) :
        {
            isz = iszFileNotFound;
            break;
        }
        case ( ERROR_CANNOT_MAKE ) :
        case ( ERROR_DISK_FULL ) :
        {
            isz = iszDiskFull;
            bDriveLetter = TRUE;
            break;
        }
        case ( ERROR_WRITE_PROTECT ) :
        {
            isz = iszWriteProtection;
            bDriveLetter = TRUE;
            break;
        }
        case ( ERROR_SHARING_VIOLATION ) :
        {
            isz = iszSharingViolation;
            break;
        }
        case ( ERROR_CREATE_NO_MODIFY ) :
        {
            isz = iszCreateNoModify;
            break;
        }
        case ( ERROR_NETWORK_ACCESS_DENIED ) :
        {
            isz = iszNetworkAccessDenied;
            break;
        }
        case ( ERROR_PORTNAME ) :
        {
            isz = iszPortName;
            break;
        }
        case ( ERROR_LAZY_READONLY ) :
        {
            isz = iszReadOnly;
            break;
        }
        case ( ERROR_DIR_ACCESS_DENIED ) :
        {
            isz = iszDirAccessDenied;
            break;
        }
        case ( ERROR_FILE_ACCESS_DENIED ) :
        case ( ERROR_ACCESS_DENIED ) :
        {
            isz = iszFileAccessDenied;
            break;
        }
        case ( ERROR_UNRECOGNIZED_VOLUME ) :
        {
            FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM |
                               FORMAT_MESSAGE_IGNORE_INSERTS |
                               FORMAT_MESSAGE_MAX_WIDTH_MASK,
                           NULL,
                           wErrCode,
                           GetUserDefaultLCID(),
                           szWarning,
                           WARNINGMSGLENGTH,
                           NULL );
            goto DisplayError;
        }
        default :
        {
            isz = iszInvalidFileName;
            break;
        }
    }
    if (!CDLoadString( g_hinst,
                     isz,
                     szCaption,
                     WARNINGMSGLENGTH ))
    {
        wsprintf( szWarning,
                  TEXT("Error occurred, but error resource cannot be loaded.") );
    }
    else
    {
        wsprintf( szWarning,
                  szCaption,
                  bDriveLetter ? (LPTSTR)(CHAR)*szFile : szFile );

DisplayError:
        GetWindowText(hDlg, szCaption, WARNINGMSGLENGTH);

        if (!mbType)
        {
            mbType = MB_OK | MB_ICONEXCLAMATION;
        }

        nRet = MessageBox(hDlg, szWarning, szCaption, mbType);
    }

    if (isz == iszInvalidFileName)
    {
        PostMessage( hDlg,
                     WM_NEXTDLGCTL,
                     (WPARAM)GetDlgItem(hDlg, edt1),
                     (LPARAM)1L );
    }

    return (nRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  MeasureItem
//
////////////////////////////////////////////////////////////////////////////

VOID MeasureItem(
    HWND hDlg,
    LPMEASUREITEMSTRUCT mis)
{
    if (!dyItem)
    {
        HDC hDC = GetDC(hDlg);
        TEXTMETRIC TM;
        HANDLE hFont;

        hFont = (HANDLE)SendMessage(hDlg, WM_GETFONT, 0, 0L);
        if (!hFont)
        {
            hFont = GetStockObject(SYSTEM_FONT);
        }
        hFont = SelectObject(hDC, hFont);
        GetTextMetrics(hDC, &TM);
        SelectObject(hDC, hFont);
        ReleaseDC(hDlg, hDC);
        dyText = TM.tmHeight;
        dyItem = max(dyDirDrive, dyText);
    }

    if (mis->CtlID == lst1)
    {
        mis->itemHeight = dyText;
    }
    else
    {
        mis->itemHeight = dyItem;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Signum
//
//  Returns the sign of an integer:
//           -1 if integer < 0
//            0 if integer = 0
//            1 if integer > 0
//
//  Note:  Signum *could* be defined as an inline macro, but that causes
//         the C compiler to disable Loop optimization, Global register
//         optimization, and Global optimizations for common subexpressions
//         in any function that the macro would appear.  The cost of a call
//         to the function seemed worth the optimizations.
//
////////////////////////////////////////////////////////////////////////////

int Signum(
    int nTest)
{
    return ((nTest == 0) ? 0 : (nTest > 0) ? 1 : -1);
}


////////////////////////////////////////////////////////////////////////////
//
//  DrawItem
//
//  Draws the drive/directory pictures in the respective combo list boxes.
//
//  lst1 is listbox for files
//  lst2 is listbox for directories
//  cmb1 is combobox for filters
//  cmb2 is combobox for drives
//
////////////////////////////////////////////////////////////////////////////

VOID DrawItem(
    POPENFILEINFO pOFI,
    HWND hDlg,
    WPARAM wParam,
    LPDRAWITEMSTRUCT lpdis,
    BOOL bSave)
{
    HDC hdcList;
    RECT rc;
//  RECT rcCmb2;
    TCHAR szText[MAX_FULLPATHNAME + 1];
    int dxAcross;
    LONG nHeight;
    LONG rgbBack, rgbText, rgbOldBack, rgbOldText;
    SHORT nShift = 1;             // to shift directories right in lst2
    BOOL bSel;
    int BltItem;
    int nBackMode;

    if ((int)lpdis->itemID < 0)
    {
        DefWindowProc(hDlg, WM_DRAWITEM, wParam, (LPARAM)lpdis);
        return;
    }

    *szText = CHAR_NULL;

    if (lpdis->CtlID != lst1 && lpdis->CtlID != lst2 && lpdis->CtlID != cmb2)
    {
        return;
    }

    if (!pOFI)
    {
        return;
    }

    hdcList = lpdis->hDC;

    if (lpdis->CtlID != cmb2)
    {
        SendDlgItemMessage( hDlg,
                            (int)lpdis->CtlID,
                            LB_GETTEXT ,
                            (WPARAM)lpdis->itemID,
                            (LONG_PTR)szText );

        if (*szText == 0)
        {
            //
            //  If empty listing.
            //
            DefWindowProc(hDlg, WM_DRAWITEM, wParam, (LONG_PTR)lpdis);
            return;
        }

        if (!bCasePreserved)
        {
            CharLower(szText);
        }
    }

    nHeight = (lpdis->CtlID == lst1) ? dyText : dyItem;

    CopyRect((LPRECT)&rc, (LPRECT)&lpdis->rcItem);

    rc.bottom = rc.top + nHeight;

    if (bSave && (lpdis->CtlID == lst1))
    {
        rgbBack = rgbWindowColor;
        rgbText = rgbGrayText;
    }
    else
    {
        //
        //  Careful checking of bSel is needed here.  Since the file
        //  listbox (lst1) can allow multiselect, only ODS_SELECTED needs
        //  to be set.  But for the directory listbox (lst2), ODS_FOCUS
        //  also needs to be set.
        //
        bSel = (lpdis->itemState & (ODS_SELECTED | ODS_FOCUS));
        if ((bSel & ODS_SELECTED) &&
            ((lpdis->CtlID != lst2) || (bSel & ODS_FOCUS)))
        {
            rgbBack = rgbHiliteColor;
            rgbText = rgbHiliteText;
        }
        else
        {
            rgbBack = rgbWindowColor;
            rgbText = rgbWindowText;
        }
    }

    rgbOldBack = SetBkColor(hdcList, rgbBack);
    rgbOldText = SetTextColor(hdcList, rgbText);

    //
    //  Drives -- text is now in UI style, c: VolumeName/Server-Sharename.
    //
    if (lpdis->CtlID == cmb2)
    {
        HANDLE hCmb2 = GetDlgItem(hDlg, cmb2);

        dxAcross = dxDirDrive / BMPHIOFFSET;

        BltItem = (int) SendMessage(hCmb2, CB_GETITEMDATA, lpdis->itemID, 0);

        SendMessage(hCmb2, CB_GETLBTEXT, lpdis->itemID, (LPARAM)szText);

        if (bSel & ODS_SELECTED)
        {
            BltItem += BMPHIOFFSET;
        }
    }
    else if (lpdis->CtlID == lst2)
    {
        //
        //  Directories.
        //
        dxAcross = dxDirDrive / BMPHIOFFSET;

        if (lpdis->itemID > pOFI->idirSub)
        {
            nShift = (SHORT)pOFI->idirSub;
        }
        else
        {
            nShift = (SHORT)lpdis->itemID;
        }

        //
        //  Must be at least 1.
        //
        nShift++;

        BltItem = 1 + Signum(lpdis->itemID + 1 - pOFI->idirSub);
        if (bSel & ODS_FOCUS)
        {
            BltItem += BMPHIOFFSET;
        }
    }
    else if (lpdis->CtlID == lst1)
    {
        //
        //  Prep for TextOut below.
        //
        dxAcross = -dxSpace;
    }

    if (bSave && (lpdis->CtlID == lst1) && !rgbText)
    {
        HBRUSH hBrush = CreateSolidBrush(rgbBack);
        HBRUSH hOldBrush;

        nBackMode = SetBkMode(hdcList, TRANSPARENT);
        hOldBrush = SelectObject( lpdis->hDC,
                                  hBrush
                                      ? hBrush
                                      : GetStockObject(WHITE_BRUSH) );

        FillRect(lpdis->hDC, (LPRECT)(&(lpdis->rcItem)), hBrush);
        SelectObject(lpdis->hDC, hOldBrush);
        if (hBrush)
        {
            DeleteObject(hBrush);
        }

        GrayString( lpdis->hDC,
                    GetStockObject(BLACK_BRUSH),
                    NULL,
                    (LPARAM)szText,
                    0,
                    lpdis->rcItem.left + dxSpace,
                    lpdis->rcItem.top,
                    0,
                    0 );
        SetBkMode(hdcList, nBackMode);
    }

#if 0
    else if (lpdis->CtlID == cmb2)
    {
        rcCmb2.right = rc.right;
        rcCmb2.left = rc.left + (WORD)(dxSpace + dxAcross) + (dxSpace * nShift);
        rcCmb2.top = rc.top + (dyItem - dyText) / 2;
        rcCmb2.bottom = rc.top + nHeight;

        DrawText( hdcList,
                  szText,
                  -1,
                  &rcCmb2,
                  DT_LEFT | DT_EXPANDTABS | DT_NOPREFIX );
    }
#endif

    else
    {
        //
        //  Draw the name.
        //
        ExtTextOut( hdcList,
                    rc.left + (WORD)(dxSpace + dxAcross) + dxSpace * nShift,
                    rc.top + (nHeight - dyText) / 2,
                    ETO_OPAQUE | ETO_CLIPPED,
                    (LPRECT)&rc,
                    szText,
                    lstrlen(szText),
                    NULL );
    }

    //
    //  Draw the picture.
    //
    if (lpdis->CtlID != lst1)
    {
        BitBlt( hdcList,
                rc.left + dxSpace * nShift,
                rc.top + (dyItem - dyDirDrive) / 2,
                dxAcross,
                dyDirDrive,
                hdcMemory,
                BltItem * dxAcross,
                0,
                SRCCOPY );
    }

    SetTextColor(hdcList, rgbOldText);
    SetBkColor(hdcList, rgbBack);

    if (lpdis->itemState & ODS_FOCUS)
    {
        DrawFocusRect(hdcList, (LPRECT)&lpdis->rcItem);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  SpacesExist
//
////////////////////////////////////////////////////////////////////////////

BOOL SpacesExist(
    LPTSTR szFileName)
{
    while (*szFileName)
    {
        if (*szFileName == CHAR_SPACE)
        {
            return (TRUE);
        }
        else
        {
            szFileName++;
        }
    }
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  StripFileName
//
//  Removes all but the filename from editbox contents.
//  This is to be called before the user makes directory or drive
//  changes by selecting them instead of typing them.
//
////////////////////////////////////////////////////////////////////////////

void StripFileName(
    HANDLE hDlg,
    BOOL bWowApp)
{
    TCHAR szText[MAX_FULLPATHNAME];
    SHORT nFileOffset, cb;

    if (GetDlgItemText(hDlg, edt1, szText, MAX_FULLPATHNAME - 1))
    {
        DWORD lRet;

        lRet = ParseFile(szText, IsLFNDriveX(hDlg, szText), bWowApp, FALSE);
        nFileOffset = LOWORD(lRet);
        cb = HIWORD(lRet);
        if (nFileOffset < 0)
        {
            //
            //  If there was a parsing error, check for CHAR_SEMICOLON
            //  delimeter.
            //
            if (szText[cb] == CHAR_SEMICOLON)
            {
                szText[cb] = CHAR_NULL;
                nFileOffset = (WORD)ParseFile( szText,
                                               IsLFNDriveX(hDlg, szText),
                                               bWowApp,
                                               FALSE );
                szText[cb] = CHAR_SEMICOLON;
                if (nFileOffset < 0)
                {
                    //
                    //  Still trouble, so Exit.
                    //
                    szText[0] = CHAR_NULL;
                }
            }
            else
            {
                szText[0] = CHAR_NULL;
            }
        }
        if (nFileOffset > 0)
        {
            lstrcpy(szText, (LPTSTR)(szText + nFileOffset));
        }
        if (nFileOffset)
        {
            SetDlgItemText(hDlg, edt1, szText);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  lstrtok
//
////////////////////////////////////////////////////////////////////////////

LPTSTR lstrtok(
    LPTSTR lpStr,
    LPCTSTR lpDelim)
{
    static LPTSTR lpString;
    LPTSTR lpRetVal, lpTemp;

    //
    //  If we are passed new string skip leading delimiters.
    //
    if (lpStr)
    {
        lpString = lpStr;

        while (*lpString && StrChr(lpDelim, *lpString))
        {
            lpString = CharNext(lpString);
        }
    }

    //
    //  If there are no more tokens, return NULL.
    //
    if (!*lpString)
    {
        return (CHAR_NULL);
    }

    //
    //  Save head of token.
    //
    lpRetVal = lpString;

    //
    //  Find delimiter or end of string.
    //
    while (*lpString && !StrChr(lpDelim, *lpString))
    {
        lpString = CharNext(lpString);
    }

    //
    //  If we found a delimiter insert string terminator and skip.
    //
    if (*lpString)
    {
        lpTemp = CharNext(lpString);
        *lpString = CHAR_NULL;
        lpString = lpTemp;
    }

    //
    //  Return token.
    //
    return (lpRetVal);
}


////////////////////////////////////////////////////////////////////////////
//
//  ChopText
//
////////////////////////////////////////////////////////////////////////////

LPTSTR ChopText(
    HWND hwndDlg,
    int idStatic,
    LPTSTR lpch)
{
    RECT rc;
    register int cxField;
    BOOL fChop = FALSE;
    HWND hwndStatic;
    HDC hdc;
    TCHAR chDrv;
    HANDLE hOldFont;
    LPTSTR lpstrStart = lpch;
    SIZE Size;
    BOOL bRet;

    //
    //  Get length of static field.
    //
    hwndStatic = GetDlgItem(hwndDlg, idStatic);
    GetClientRect(hwndStatic, (LPRECT)&rc);
    cxField = rc.right - rc.left;

    //
    //  Chop characters off front end of text until short enough.
    //
    hdc = GetDC(hwndStatic);

    hOldFont = NULL;

    while ((bRet = GetTextExtentPoint(hdc, lpch, lstrlen(lpch), &Size)) &&
           (cxField < Size.cx))
    {
        if (!fChop)
        {
            chDrv = *lpch;

            //
            //  Proportional font support.
            //
            if (bRet = GetTextExtentPoint(hdc, lpch, 7, &Size))
            {
                cxField -= Size.cx;
            }
            else
            {
                break;
            }

            if (cxField <= 0)
            {
               break;
            }

            lpch += 7;
        }
        while (*lpch && (!ISBACKSLASH_P(lpstrStart, lpch)))
        {
            lpch++;
        }
        //Skip the backslash 
        lpch++;

        fChop = TRUE;
    }

    ReleaseDC(hwndStatic, hdc);

    //
    //  If any characters chopped off, replace first three characters in
    //  remaining text string with ellipsis.
    //
    if (fChop)
    {
        //Skip back to include the backslash
        lpch--;
        *--lpch = CHAR_DOT;
        *--lpch = CHAR_DOT;
        *--lpch = CHAR_DOT;
        *--lpch = *(lpstrStart + 2);
        *--lpch = *(lpstrStart + 1);
        *--lpch = *lpstrStart;
    }

    return (lpch);
}


////////////////////////////////////////////////////////////////////////////
//
//  FillOutPath
//
//  Fills out lst2 given that the current directory has been set.
//
//  Returns:  TRUE    if they DO NOT match
//            FALSE   if match
//
////////////////////////////////////////////////////////////////////////////

BOOL FillOutPath(
    HWND hList,
    POPENFILEINFO pOFI)
{
    TCHAR szPath[CCHNETPATH];
    LPTSTR lpCurDir;
    LPTSTR lpB, lpF;
    TCHAR wc;
    int cchPathOffset;
    LPCURDLG lpCurDlg;

    if(!(lpCurDlg = (LPCURDLG)TlsGetValue(g_tlsiCurDlg)) ||
       !(lpCurDir = lpCurDlg->lpstrCurDir))
    {
        return (FALSE);
    }

    lpF = szPath;    
    lstrcpy(lpF, lpCurDir);

    //
    //  Wow apps started from lfn dirs will set the current directory to an
    //  lfn, but only in the case where it is less than 8 chars.
    //
    if (pOFI->pOFN->Flags & OFN_NOLONGNAMES)
    {
        ShortenThePath(lpF);
    }

    *lpF = (TCHAR)CharLower((LPTSTR)*lpF);
    cchPathOffset = GetPathOffset(lpF);
    if (cchPathOffset == -1)
    {
        cchPathOffset = 0;
    }
    lpB = (lpF + cchPathOffset);

    //
    //  Hack to retain Winball display functionality.
    //  Drived disks are displayed as C:\ (the root dir).
    //  whereas unc disks are displayed as \\server\share (the disk).
    //  Hence, extend display of drived disks by one char.
    //
    if (*(lpF + 1) == CHAR_COLON)
    {
        wc = *(++lpB);
        *lpB = CHAR_NULL;
    }
    else
    {
        //
        //  Since we use lpF over and over again to speed things
        //  up, and since GetCurrentDirectory returns the disk name
        //  for unc, but the root path for drives, we have the following hack
        //  for when we are at the root of the unc directory, and lpF
        //  contains old stuff out past cchPathOffset.
        //
        PathAddBackslash(lpF);

        wc = 0;
        *lpB++ = CHAR_NULL;
    }

    //
    //  Insert the items for the path to the current dir
    //  Insert the root...
    //
    pOFI->idirSub = 0;

    SendMessage(hList, LB_INSERTSTRING, pOFI->idirSub++, (LPARAM)lpF);

    if (wc)
    {
        *lpB = wc;
    }

    for (lpF = lpB; *lpB; lpB++)
    {
        if ((ISBACKSLASH_P(szPath, lpB)) || (*lpB == CHAR_SLASH))
        {
            *lpB = CHAR_NULL;

            SendMessage(hList, LB_INSERTSTRING, pOFI->idirSub++, (LPARAM)lpF);

            lpF = lpB + 1;

            *lpB = CHAR_BSLASH;
        }
    }

    //
    //  Assumes that a path always ends with one last un-delimited dir name.
    //  Check to make sure we have at least one.
    //
    if (lpF != lpB)
    {
        SendMessage(hList, LB_INSERTSTRING, pOFI->idirSub++, (LPARAM)lpF);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  ShortenThePath
//
//  Takes a pathname and converts all dirs to shortnames if they are
//  not valid DOS 8.3 names.
//
//  Returns:  TRUE    if pathname converted
//            FALSE   if ran out of space, buffer left alone
//
////////////////////////////////////////////////////////////////////////////

BOOL ShortenThePath(
    LPTSTR pPath)
{
    TCHAR szDest[MAX_PATH];
    LPTSTR pSrcNextSpec, pReplaceSpec;
    LPTSTR pDest, p;
    LPTSTR pSrc;
    int cchPathOffset;
    HANDLE hFind;
    WIN32_FIND_DATA FindData;
    UINT i;
    int nSpaceLeft = MAX_PATH - 1;

#ifdef UNICODE
    UNICODE_STRING Name;
    BOOLEAN fSpace = FALSE;
#endif


    //
    //  Save pointer to beginning of buffer.
    //
    pSrc = pPath;

    //
    //  Eliminate double quotes.
    //
    for (p = pDest = pSrc; *p; p++, pDest++)
    {
        if (*p == CHAR_QUOTE)
        {
            p++;
        }
        *pDest = *p;
    }

    *pDest = CHAR_NULL;

    //
    //  Strip out leading spaces.
    //
    while (*pSrc == CHAR_SPACE)
    {
        pSrc++;
    }

    //
    //  Skip past \\foo\bar or <drive>:
    //
    pDest = szDest;
    pSrcNextSpec = pSrc;

    //
    //  Reuse shell32 internal api that calculates path offset.
    //  The cchPathOffset variable will be the offset that when added to
    //  the pointer will result in a pointer to the backslash before the
    //  first part of the path.
    //
    //  NOTE:  UNICODE only call.
    //
    cchPathOffset = GetPathOffset(pSrc);

    //
    //  Check to see if it's valid.  If pSrc is not of the \\foo\bar
    //  or <drive>: form we just do nothing.
    //
    if (cchPathOffset == -1)
    {
        return (TRUE);
    }

    //
    //  cchPathOffset will always be at least 1 and is the number of
    //  characters - 1 that we want to copy (that is, if 0 was
    //  permissible, it would denote 1 character).
    //
    do
    {
        *pDest++ = *pSrcNextSpec++;

        if (!--nSpaceLeft)
        {
            return (FALSE);
        }
    } while (cchPathOffset--);

    //
    //  At this point, we have just the filenames that we can shorten:
    //  \\foo\bar\it\is\here ->  it\is\here
    //  c:\angry\lions       ->  angry\lions
    //
    while (pSrcNextSpec)
    {
        //
        //  pReplaceSpec holds the current spec we need to replace.
        //  By default, if we can't find the altname, then just use this.
        //
        pReplaceSpec = pSrcNextSpec;

        //
        //  Search for trailing "\"
        //  pSrcNextSpec will point to the next spec to fix.
        //  (*pSrcNextSpec = NULL if done)
        //
        while (*pSrcNextSpec && (!ISBACKSLASH_P(pReplaceSpec, pSrcNextSpec)))
        {
            pSrcNextSpec++;
        }

        if (*pSrcNextSpec)
        {
            //
            //  If there is more, then pSrcNextSpec should point to it.
            //  Also delimit this spec.
            //
            *pSrcNextSpec = CHAR_NULL;
        }
        else
        {
            pSrcNextSpec = NULL;
        }

        hFind = FindFirstFile(pSrc, &FindData);

        //
        //  We could exit as soon as this FindFirstFileFails,
        //  but there's the special case of having execute
        //  without read permission.  This would fail since the lfn
        //  is valid for lfn apps.
        //
        if (hFind != INVALID_HANDLE_VALUE)
        {
            FindClose(hFind);

#ifdef UNICODE
            //
            //  See if it's not a legal 8.3 name or if there are spaces
            //  in the name.  If either is true, use the alternate name.
            //
            RtlInitUnicodeString(&Name, FindData.cFileName);
            if (!RtlIsNameLegalDOS8Dot3(&Name, NULL, &fSpace) || fSpace)
#endif
            {
                if (FindData.cAlternateFileName[0])
                {
                    pReplaceSpec = FindData.cAlternateFileName;
                }
            }
        }

        i = lstrlen(pReplaceSpec);
        nSpaceLeft -= i;

        if (nSpaceLeft <= 0)
        {
            return (FALSE);
        }

        lstrcpy(pDest, pReplaceSpec);
        pDest += i;

        //
        //  Now replace the CHAR_NULL with a slash if necessary.
        //
        if (pSrcNextSpec)
        {
            *pSrcNextSpec++ = CHAR_BSLASH;

            //
            //  Also add backslash to destination.
            //
            *pDest++ = CHAR_BSLASH;
            nSpaceLeft--;
        }
    }

    lstrcpy(pPath, szDest);

    return (TRUE);
}



////////////////////////////////////////////////////////////////////////////
//
//  FListAll
//
//  Given a file pattern, it changes the directory to that of the spec,
//  and updates the display.
//
////////////////////////////////////////////////////////////////////////////

int FListAll(
    POPENFILEINFO pOFI,
    HWND hDlg,
    LPTSTR szSpec)
{
    LPTSTR szPattern;
    TCHAR chSave;
    DWORD nRet = 0;
    TCHAR szDirBuf[MAX_FULLPATHNAME + 1];

    if (!bCasePreserved)
    {
        CharLower(szSpec);
    }

    //
    //  No directory.
    //
    if (!(szPattern = StrRChr( szSpec,
                               szSpec + lstrlen(szSpec),
                               CHAR_BSLASH )) &&
        !StrChr(szSpec, CHAR_COLON))
    {
        lstrcpy(pOFI->szSpecCur, szSpec);
        if (!bInitializing)
        {
            UpdateListBoxes(hDlg, pOFI, szSpec, mskDirectory);
        }
    }
    else
    {
        *szDirBuf = CHAR_NULL;

        //
        //  Just root + pattern.
        //
        if (szPattern == StrChr(szSpec, CHAR_BSLASH))
        {
            if (!szPattern)
            {
                //
                //  Didn't find a slash, must have drive.
                //
                szPattern = CharNext(CharNext(szSpec));
            }
            else if ((szPattern == szSpec) ||
                     ((szPattern - 2 == szSpec) &&
                      (*(szSpec + 1) == CHAR_COLON)))
            {
                szPattern = CharNext(szPattern);
            }
            else
            {
                goto KillSlash;
            }
            chSave = *szPattern;
            if (chSave != CHAR_DOT)
            {
                //
                //  If not c:.. or c:.
                //
                *szPattern = CHAR_NULL;
            }
            lstrcpy(szDirBuf, szSpec);
            if (chSave == CHAR_DOT)
            {
                szPattern = szSpec + lstrlen(szSpec);
                AppendExt(szPattern, pOFI->pOFN->lpstrDefExt, TRUE);
            }
            else
            {
                *szPattern = chSave;
            }
        }
        else
        {
KillSlash:
            *szPattern++ = 0;
            lstrcpy(szDirBuf, szSpec);
        }

        if ((nRet = ChangeDir(hDlg, szDirBuf, TRUE, FALSE)) < 0)
        {
            return (nRet);
        }

        lstrcpy(pOFI->szSpecCur, szPattern);
        SetDlgItemText(hDlg, edt1, pOFI->szSpecCur);

        SelDisk(hDlg, NULL);

        if (!bInitializing)
        {
            SendMessage( hDlg,
                         WM_COMMAND,
                         GET_WM_COMMAND_MPS( cmb2,
                                             GetDlgItem(hDlg, cmb2),
                                             MYCBN_DRAW ) );
        }
    }

    return (nRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  ChangeDir
//
//  Changes the current directory and/or resource.
//
//  lpszDir - Fully qualified, or partially qualified names.
//            To change to another disk and cd automatically to the
//            last directory as set in the shell's environment, specify
//            only a disk name (i.e. c: or \\triskal\scratch - must not end
//            in backslash).
//  bForce  - If True, then caller requires that ChangeDir successfully cd
//            somewhere.  Order of cding is as follows:
//                1. lpszDir
//                2. current dir for the current thread
//                3. root of current dir for the current thread
//                4. c:
//  bError - if TRUE, then pop up an AccessDenied dialog at every step
//           in the force.
//
//  Returns an index into gaDiskInfo for new disk chosen or,
//  the ADDDISK_error code.
//  Returns ADDDISK_NOCHANGE in the event that it cannot cd to the root
//  directory of the specific file.
//
////////////////////////////////////////////////////////////////////////////

int ChangeDir(
    HWND hDlg,
    LPCTSTR lpszDir,
    BOOL bForce,
    BOOL bError)
{
    TCHAR szCurDir[CCHNETPATH];
    LPTSTR lpCurDir;
    int cchDirLen;
    TCHAR wcDrive = 0;
    int nIndex;
    BOOL nRet;
    LPCURDLG lpCurDlg;


    //
    //  SheChangeDirEx will call GetCurrentDir, but will use what it
    //  gets only in the case where the path passed in was no good.
    //

    //
    //  1st, try request.
    //
    if (lpszDir && *lpszDir)
    {
        lstrcpyn(szCurDir, lpszDir, CCHNETPATH);

        //
        //  Remove trailing spaces.
        //
        lpCurDir = szCurDir + lstrlen(szCurDir) - 1;
        while (*lpCurDir && (*lpCurDir == CHAR_SPACE))
        {
            *lpCurDir-- = CHAR_NULL;
        }

        nRet = SheChangeDirEx(szCurDir);

        if (nRet == ERROR_ACCESS_DENIED)
        {
            if (bError)
            {
                //
                //  Casting to LPTSTR is ok below - InvalidFileWarning will
                //  not change this string because the path is always
                //  guaranteed to be <= MAX_FULLPATHNAME.
                //
                InvalidFileWarning( hDlg,
                                    (LPTSTR)lpszDir,
                                    ERROR_DIR_ACCESS_DENIED,
                                    0 );
            }

            if (!bForce)
            {
                return (CHANGEDIR_FAILED);
            }
        }
        else
        {
            goto ChangeDir_OK;
        }
    }

    //
    //  2nd, try lpCurDlg->lpstrCurDir value (which we got above).
    //
    //  !!! need to check for a null return value ???
    //
    lpCurDlg = (LPCURDLG)TlsGetValue(g_tlsiCurDlg);
    lpCurDir = (lpCurDlg ? lpCurDlg->lpstrCurDir : NULL);

    nRet = SheChangeDirEx(lpCurDir);

    if (nRet == ERROR_ACCESS_DENIED)
    {
        if (bError)
        {
            InvalidFileWarning( hDlg,
                                lpCurDir,
                                ERROR_DIR_ACCESS_DENIED,
                                0 );
        }
    }
    else
    {
        goto ChangeDir_OK;
    }

    //
    //  3rd, try root of lpCurDlg->lpstrCurDir or GetCurrentDir (sanity).
    //
    lstrcpy(szCurDir, lpCurDir);
    cchDirLen = GetPathOffset(szCurDir);

    //
    //  Sanity check - it's guaranteed not to fail ...
    //
    if (cchDirLen != -1)
    {
        szCurDir[cchDirLen] = CHAR_BSLASH;
        szCurDir[cchDirLen + 1] = CHAR_NULL;

        nRet = SheChangeDirEx(szCurDir);
        if (nRet == ERROR_ACCESS_DENIED)
        {
            if (bError)
            {
                InvalidFileWarning( hDlg,
                                    (LPTSTR)lpszDir,
                                    ERROR_DIR_ACCESS_DENIED,
                                    0 );
            }
        }
        else
        {
            goto ChangeDir_OK;
        }
    }

    //
    //  4th, try c:
    //
    lstrcpy(szCurDir, TEXT("c:"));
    nRet = SheChangeDirEx(szCurDir);
    if (nRet == ERROR_ACCESS_DENIED)
    {
        if (bError)
        {
            InvalidFileWarning( hDlg,
                                (LPTSTR)lpszDir,
                                ERROR_DIR_ACCESS_DENIED,
                                0 );
        }
    }
    else
    {
        goto ChangeDir_OK;
    }

    return (CHANGEDIR_FAILED);

ChangeDir_OK:

    GetCurrentDirectory(CCHNETPATH, szCurDir);

    nIndex = DiskAddedPreviously(0, szCurDir);

    //
    //  If the disk doesn't exist, add it.
    //
    if (nIndex == -1)
    {
        HWND hCmb2 = GetDlgItem(hDlg, cmb2);
        LPTSTR lpszDisk = NULL;
        DWORD dwType;
        TCHAR wc1, wc2;

        if (szCurDir[1] == CHAR_COLON)
        {
            wcDrive = szCurDir[0];
        }
        else
        {
            lpszDisk = &szCurDir[0];
        }

        cchDirLen = GetPathOffset(szCurDir);
        if (cchDirLen != -1)
        {
            wc1 = szCurDir[cchDirLen];
            wc2 = szCurDir[cchDirLen + 1];

            szCurDir[cchDirLen] = CHAR_BSLASH;
            szCurDir[cchDirLen + 1] = CHAR_NULL;
        }

        dwType = GetDiskIndex(GetDiskType(szCurDir));

        if (cchDirLen != -1)
        {
            szCurDir[cchDirLen] = CHAR_NULL;
        }

        nIndex = AddDisk(wcDrive, lpszDisk, NULL, dwType);

        SendMessage(hCmb2, WM_SETREDRAW, FALSE, 0L);

        wNoRedraw |= 1;

        SendMessage( hCmb2,
                     CB_SETITEMDATA,
                     (WPARAM)SendMessage(
                                 hCmb2,
                                 CB_ADDSTRING,
                                 (WPARAM)0,
                                 (LPARAM)(LPTSTR)gaDiskInfo[nIndex].lpAbbrName ),
                     (LPARAM)gaDiskInfo[nIndex].dwType );

        if ((dwType != NETDRVBMP) && (dwType != REMDRVBMP))
        {
            gaDiskInfo[nIndex].bCasePreserved =
                IsFileSystemCasePreserving(gaDiskInfo[nIndex].lpPath);
        }

        wNoRedraw &= ~1;

        SendMessage(hCmb2, WM_SETREDRAW, TRUE, 0L);

        if (cchDirLen != -1)
        {
            szCurDir[cchDirLen] = wc1;
            szCurDir[cchDirLen + 1] = wc2;
        }
    }
    else
    {
        //
        //  Validate the disk if it has been seen before.
        //
        //  For unc names that fade away, refresh the cmb2 box.
        //
        if (!gaDiskInfo[nIndex].bValid)
        {
            gaDiskInfo[nIndex].bValid = TRUE;

            SendDlgItemMessage(
                   hDlg,
                   cmb2,
                   CB_SETITEMDATA,
                   (WPARAM)SendDlgItemMessage(
                               hDlg,
                               cmb2,
                               CB_ADDSTRING,
                               (WPARAM)0,
                               (LPARAM)(LPTSTR)gaDiskInfo[nIndex].lpAbbrName ),
                   (LPARAM)gaDiskInfo[nIndex].dwType );
        }
    }

    //
    //  Update our global concept of Case.
    //
    if (nIndex >= 0)
    {
        //
        //  Send special WOW message to indicate the directory has
        //  changed.
        //
        SendMessage(hDlg, msgWOWDIRCHANGE, 0, 0);

        //
        //  Get pointer to current directory.
        //
        lpCurDlg = (LPCURDLG)TlsGetValue(g_tlsiCurDlg);
        lpCurDir = (lpCurDlg ? lpCurDlg->lpstrCurDir : NULL);
        if (!lpCurDlg || !lpCurDir)
        {
            return (CHANGEDIR_FAILED);
        }

        bCasePreserved = gaDiskInfo[nIndex].bCasePreserved;

        //
        //  In case the unc name already has a drive letter, correct
        //  lst2 display.
        //
        cchDirLen = 0;

        //
        //  Compare with szCurDir since it's been lowercased.
        //
        if (DBL_BSLASH(szCurDir) &&
            (*gaDiskInfo[nIndex].lpAbbrName != szCurDir[0]))
        {
            if ((cchDirLen = GetPathOffset(szCurDir)) != -1)
            {
                szCurDir[--cchDirLen] = CHAR_COLON;
                szCurDir[--cchDirLen] = *gaDiskInfo[nIndex].lpAbbrName;
            }
        }

        if ((gaDiskInfo[nIndex].dwType == CDDRVBMP) ||
            (gaDiskInfo[nIndex].dwType == FLOPPYBMP))
        {
            if (*lpCurDir != gaDiskInfo[nIndex].wcDrive)
            {
                TCHAR szDrive[5];

                //
                //  Get new volume info - should always succeed.
                //
                szDrive[0] = gaDiskInfo[nIndex].wcDrive;
                szDrive[1] = CHAR_COLON;
                szDrive[2] = CHAR_BSLASH;
                szDrive[3] = CHAR_NULL;
                UpdateLocalDrive(szDrive, TRUE);

                //
                //  Flush to the cmb before selecting the disk.
                //
                if ( lpCurDlg = (LPCURDLG)TlsGetValue(g_tlsiCurDlg) )
                {
                    gahDlg[lpCurDlg->dwCurDlgNum] = hDlg;
                    FlushDiskInfoToCmb2();
                }
            }
        }
        
        lstrcpy(lpCurDir, (LPTSTR)&szCurDir[cchDirLen]);
        PathAddBackslash(lpCurDir);

        //
        //  If the worker thread is running, then trying to select here
        //  will just render the cmb2 blank, which is what we want;
        //  otherwise, it should successfully select it.
        //
        SelDisk(hDlg, gaDiskInfo[nIndex].lpPath);
    }
//  else
//  {
//      print out error message returned from AddDisk ...
//  }

    return (nIndex);
}


////////////////////////////////////////////////////////////////////////////
//
//  IsFileSystemCasePreserving
//
////////////////////////////////////////////////////////////////////////////

BOOL IsFileSystemCasePreserving(
    LPTSTR lpszDisk)
{
    TCHAR szPath[MAX_FULLPATHNAME];
    DWORD dwFlags;

    if (!lpszDisk)
    {
        return (FALSE);
    }

    lstrcpy(szPath, lpszDisk);
    lstrcat(szPath, TEXT("\\"));

    if (GetVolumeInformation( szPath,
                              NULL,
                              0,
                              NULL,
                              NULL,
                              &dwFlags,
                              NULL,
                              0 ))
    {
        return ((dwFlags & FS_CASE_IS_PRESERVED));
    }

    //
    //  Default to FALSE if there is an error.
    //
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  IsLFNDriveX
//
////////////////////////////////////////////////////////////////////////////

BOOL IsLFNDriveX(
    HWND hDlg,
    LPTSTR szPath)
{
    TCHAR szRootPath[MAX_FULLPATHNAME];
    DWORD dwVolumeSerialNumber;
    DWORD dwMaximumComponentLength;
    DWORD dwFileSystemFlags;
    LPTSTR lpCurDir;
    LPCURDLG lpCurDlg;


    if (!szPath[0] || !szPath[1] ||
        (szPath[1] != CHAR_COLON && !(DBL_BSLASH(szPath))))
    {
        //
        //  If the path is not a full path then get the directory path
        //  from the TLS current directory.
        //
        lpCurDlg = (LPCURDLG)TlsGetValue(g_tlsiCurDlg);
        lpCurDir = (lpCurDlg ? lpCurDlg->lpstrCurDir : NULL);
        lstrcpy(szRootPath, lpCurDir);
    }
    else
    {
        lstrcpy(szRootPath, szPath);
    }

    if (szRootPath[1] == CHAR_COLON)
    {
        szRootPath[2] = CHAR_BSLASH;
        szRootPath[3] = 0;
    }
    else if (DBL_BSLASH(szRootPath))
    {
        int i;
        LPTSTR p;

        //
        //  Stop at "\\foo\bar".
        //
        for (i = 0, p = szRootPath + 2; *p && i < 2; p++)
        {
            if (ISBACKSLASH_P(szRootPath, p))
            {
                i++;
            }
        }

        switch (i)
        {
            case ( 0 ) :
            {
                return (FALSE);
            }
            case ( 1 ) :
            {
                if (lstrlen(szRootPath) < MAX_FULLPATHNAME - 2)
                {
                    *p = CHAR_BSLASH;
                    *(p + 1) = CHAR_NULL;
                }
                else
                {
                    return (FALSE);
                }
                break;
            }

            case ( 2 ) :
            {
                *p = CHAR_NULL;
                break;
            }
        }
    }

    if (GetVolumeInformation( szRootPath,
                              NULL,
                              0,
                              &dwVolumeSerialNumber,
                              &dwMaximumComponentLength,
                              &dwFileSystemFlags,
                              NULL,
                              0 ))
    {
        if (dwMaximumComponentLength == (MAXDOSFILENAMELEN - 1))
        {
            return (FALSE);
        }
        else
        {
            return (TRUE);
        }
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  DiskAddedPreviously
//
//  This routine checks to see if a disk resource has been previously
//  added to the global structure.
//
//  wcDrive  - if this is set, then there is no lpszName comparison
//  lpszName - if wcDrive is not set, but the lpszName is of the form
//               "c:\" then set wcDrive = *lpszName and index by drive letter
//             else assume lpszName is a unc name
//
//  Returns:   0xFFFFFFFF   failure (disk doesn't exist in list)
//             0 - 128      number of disk in list
//
////////////////////////////////////////////////////////////////////////////

int DiskAddedPreviously(
    TCHAR wcDrive,
    LPTSTR lpszName)
{
    WORD i;

    //
    //  There are two index schemes (by drive or by unc \\server\share).
    //  If it doesn't have a drive letter, assume unc.
    //
    if (wcDrive || (*(lpszName + 1) == CHAR_COLON))
    {
        if (!wcDrive)
        {
            wcDrive = *lpszName;
            wcDrive = (TCHAR)CharLower((LPTSTR)wcDrive);
        }

        for (i = 0; i < dwNumDisks; i++)
        {
            //
            //  See if the drive letters are the same.
            //
            if (wcDrive)
            {
                if (wcDrive == (TCHAR)CharLower((LPTSTR)gaDiskInfo[i].wcDrive))
                {
                    return (i);
                }
            }
        }
    }
    else
    {
        DWORD cchDirLen;
        TCHAR wc;

        //
        //  Check remote name (\\server\share).
        //
        cchDirLen = GetPathOffset(lpszName);

        //
        //  If we're given a unc path, get the disk name.
        //  Otherwise, assume the whole thing is a disk name.
        //
        if (cchDirLen != -1)
        {
            wc = *(lpszName + cchDirLen);
            *(lpszName + cchDirLen) = CHAR_NULL;
        }

        for (i = 0; i < dwNumDisks; i++)
        {
            if (!lstrcmpi(gaDiskInfo[i].lpName, lpszName))
            {
                if (cchDirLen != -1)
                {
                    *(lpszName + cchDirLen) = wc;
                }
                return (i);
            }
        }

        if (cchDirLen != -1)
        {
            *(lpszName + cchDirLen) = wc;
        }
    }

    return (0xFFFFFFFF);
}


////////////////////////////////////////////////////////////////////////////
//
//  AddDisk
//
//  Adds a disk to one of the global structures:
//      gaNetDiskInfo
//      gaLocalDiskInfo
//
//  wcDrive    - the drive to attach to (this parm should be 0 for unc)
//  lpName     - \\server\share name for remote disks
//               volume name for local disks
//  lpProvider - used for remote disks only, the name of the provider
//               used with WNetFormatNetworkName api
//  dwType     - type of the bitmap to display
//               except when we are adding a drive letter temporarily
//               at startup this parameter can equal TMPNETDRV in which
//               case we set the bitmap to NETDRVBMP
//
//  Returns:  -2    Cannot Add Disk
//            -1    DiskInfo did not change
//             0    dwNumDisks - DiskInfo changed
//
////////////////////////////////////////////////////////////////////////////

int AddDisk(
    TCHAR wcDrive,
    LPTSTR lpName,
    LPTSTR lpProvider,
    DWORD dwType)
{
    int nIndex, nRet;
    DWORD cchMultiLen = 0;
    DWORD cchAbbrLen = 0;
    DWORD cchLen;
    DWORD dwRet = 0;
    LPTSTR lpBuff;
    OFN_DISKINFO *pofndiDisk = NULL, *pgDI;


    //
    //  Sanity check - wcDrive and/or lpName must be set.
    //
    if (!wcDrive && (!lpName || !*lpName))
    {
        return (ADDDISK_INVALIDPARMS);
    }

    nIndex = DiskAddedPreviously(wcDrive, lpName);

    if (nIndex != 0xFFFFFFFF)
    {
        //
        //  Do not add a temporary drive letter if we already
        //  have something better (added, for example, in a previous call).
        //
        if (dwType == TMPNETDRV)
        {
            gaDiskInfo[nIndex].bValid = TRUE;
            return (ADDDISK_NOCHANGE);
        }

        //  Using a floating profile, there can be collisions between
        //  local and network drives in which case we take the former.
        //
        //  Note: If the drive is remembered, we assume that getdrivetype
        //        will return false and that the drive is not added.
        //        But if it was added, then we overwrite anyway,
        //        since it's the desired behavior.
        //
        if ((dwType == REMDRVBMP) &&
            (dwType != gaDiskInfo[nIndex].dwType))
        {
            return (ADDDISK_NOCHANGE);
        }

        //
        //  Update previous connections.
        //
        if (!lstrcmpi(lpName, gaDiskInfo[nIndex].lpName))
        {
            //
            //  Don't update a connection as remembered, unless it's been
            //  invalidated.
            //
            if (dwType != REMDRVBMP)
            {
                gaDiskInfo[nIndex].dwType = dwType;
            }
            gaDiskInfo[nIndex].bValid = TRUE;

            return (ADDDISK_NOCHANGE);
        }
        else if (!*lpName && ((dwType == CDDRVBMP) || (dwType == FLOPPYBMP)))
        {
            //
            //  Guard against lazy calls to updatelocaldrive erasing current
            //  changed dir volume name (set via changedir).
            //
            return (ADDDISK_NOCHANGE);
        }
    }

    if (dwNumDisks >= MAX_DISKS)
    {
        return (ADDDISK_MAXNUMDISKS);
    }

    //
    //  If there is a drive, then lpPath needs only 4.
    //  If it's unc, then lpPath just equals lpName.
    //
    if (wcDrive)
    {
        cchLen = 4;
    }
    else
    {
        cchLen = 0;
    }

    if (lpName && *lpName)
    {
        //
        //  Get the length of the standard (Remote/Local) name.
        //
        cchLen += (lstrlen(lpName) + 1);

        if (lpProvider && *lpProvider &&
            ((dwType == NETDRVBMP) || (dwType == REMDRVBMP)))
        {
            //
            //  Get the length for the multiline name.
            //
            dwRet = WNetFormatNetworkName( lpProvider,
                                           lpName,
                                           NULL,
                                           &cchMultiLen,
                                           WNFMT_MULTILINE,
                                           dwAveCharPerLine );
            if (dwRet != ERROR_MORE_DATA)
            {
                return (ADDDISK_NETFORMATFAILED);
            }

            //
            //  Add 4 for <drive-letter>:\ and NULL (safeguard)
            //
            if (wcDrive)
            {
                cchMultiLen += 4;
            }

            dwRet = WNetFormatNetworkName( lpProvider,
                                           lpName,
                                           NULL,
                                           &cchAbbrLen,
                                           WNFMT_ABBREVIATED,
                                           dwAveCharPerLine );
            if (dwRet != ERROR_MORE_DATA)
            {
                return (ADDDISK_NETFORMATFAILED);
            }

            //
            //  Add 4 for <drive-letter>:\ and NULL (safeguard).
            //
            if (wcDrive)
            {
                cchAbbrLen += 4;
            }
        }
        else
        {
            //
            //  Make enough room so that lpMulti and lpAbbr can point to
            //  4 characters (drive letter + : + space + null) ahead of
            //  lpremote.
            //
            if (wcDrive)
            {
                cchLen += 4;
            }
        }
    }
    else
    {
        //
        //  Make enough room so that lpMulti and lpAbbr can point to
        //  4 characters (drive letter + : + space + null) ahead of
        //  lpremote.
        //
        if (wcDrive)
        {
            cchLen += 4;
        }
    }

    //
    //  Allocate a temp OFN_DISKINFO object to work with.
    //  When we are finished, we'll request the critical section
    //  and update the global array.
    //
    pofndiDisk = (OFN_DISKINFO *)LocalAlloc(LPTR, sizeof(OFN_DISKINFO));
    if (!pofndiDisk)
    {
        //
        //  Can't alloc or realloc memory, return error.
        //
        nRet = ADDDISK_ALLOCFAILED;
        goto AddDisk_Error;
    }

    lpBuff = (LPTSTR)LocalAlloc( LPTR,
                                 (cchLen + cchMultiLen + cchAbbrLen) * sizeof(TCHAR));
    if (!lpBuff)
    {
        //
        //  Can't alloc or realloc memory, return error.
        //
        nRet = ADDDISK_ALLOCFAILED;
        goto AddDisk_Error;
    }

    if (dwType == TMPNETDRV)
    {
        pofndiDisk->dwType = NETDRVBMP;
    }
    else
    {
        pofndiDisk->dwType = dwType;
    }

    //
    //  Always set these slots, even though wcDrive can equal 0.
    //
    pofndiDisk->wcDrive = wcDrive;
    pofndiDisk->bValid = TRUE;

    pofndiDisk->cchLen = cchLen + cchAbbrLen + cchMultiLen;

    //
    //  NOTE: lpAbbrName must always point to the head of lpBuff
    //        so that we can free the block later at DLL_PROCESS_DETACH
    //
    if (lpName && *lpName && lpProvider && *lpProvider &&
        ((dwType == NETDRVBMP) || (dwType == REMDRVBMP)))
    {
        //
        //  Create an entry for a network disk.
        //
        pofndiDisk->lpAbbrName = lpBuff;

        if (wcDrive)
        {
            *lpBuff++ = wcDrive;
            *lpBuff++ = CHAR_COLON;
            *lpBuff++ = CHAR_SPACE;

            cchAbbrLen -= 3;
        }

        dwRet = WNetFormatNetworkName( lpProvider,
                                       lpName,
                                       lpBuff,
                                       &cchAbbrLen,
                                       WNFMT_ABBREVIATED,
                                       dwAveCharPerLine );
        if (dwRet != WN_SUCCESS)
        {
            nRet = ADDDISK_NETFORMATFAILED;
            LocalFree(lpBuff);
            goto AddDisk_Error;
        }

        lpBuff += cchAbbrLen;

        pofndiDisk->lpMultiName = lpBuff;

        if (wcDrive)
        {
            *lpBuff++ = wcDrive;
            *lpBuff++ = CHAR_COLON;
            *lpBuff++ = CHAR_SPACE;

            cchMultiLen -= 3;
        }

        dwRet = WNetFormatNetworkName(lpProvider, lpName,
                                      lpBuff, &cchMultiLen, WNFMT_MULTILINE, dwAveCharPerLine);
        if (dwRet != WN_SUCCESS)
        {
            nRet = ADDDISK_NETFORMATFAILED;
            LocalFree(lpBuff);
            goto AddDisk_Error;
        }

        //
        //  Note: this assumes that the lpRemoteName
        //        returned by WNetEnumResources is always in
        //        the form \\server\share (without a trailing bslash).
        //
        pofndiDisk->lpPath = lpBuff;

        //
        //  if it's not unc.
        //
        if (wcDrive)
        {
            *lpBuff++ = wcDrive;
            *lpBuff++ = CHAR_COLON;
            *lpBuff++ = CHAR_NULL;
        }

        lstrcpy(lpBuff, lpName);
        pofndiDisk->lpName = lpBuff;

        pofndiDisk->bCasePreserved =
            IsFileSystemCasePreserving(pofndiDisk->lpPath);
    }
    else
    {
        //
        //  Create entry for a local name, or a network one with
        //  no name yet.
        //
        pofndiDisk->lpAbbrName = pofndiDisk->lpMultiName = lpBuff;

        if (wcDrive)
        {
            *lpBuff++ = wcDrive;
            *lpBuff++ = CHAR_COLON;
            *lpBuff++ = CHAR_SPACE;
        }

        if (lpName)
        {
            lstrcpy(lpBuff, lpName);
        }
        else
        {
            *lpBuff = CHAR_NULL;
        }

        pofndiDisk->lpName = lpBuff;

        if (wcDrive)
        {
            lpBuff += lstrlen(lpBuff) + 1;
            *lpBuff = wcDrive;
            *(lpBuff + 1) = CHAR_COLON;
            *(lpBuff + 2) = CHAR_NULL;
        }

        pofndiDisk->lpPath = lpBuff;

        if ((dwType == NETDRVBMP) || (dwType == REMDRVBMP))
        {
            pofndiDisk->bCasePreserved =
                IsFileSystemCasePreserving(pofndiDisk->lpPath);
        }
        else
        {
            pofndiDisk->bCasePreserved = FALSE;
        }
    }

    //
    //  Now we need to update the global array.
    //
    if (nIndex == 0xFFFFFFFF)
    {
        nIndex = dwNumDisks;
    }

    pgDI = &gaDiskInfo[nIndex];

    //
    //  Enter critical section and update data.
    //
    EnterCriticalSection(&g_csLocal);

    pgDI->cchLen = pofndiDisk->cchLen;
    pgDI->lpAbbrName = pofndiDisk->lpAbbrName;
    pgDI->lpMultiName = pofndiDisk->lpMultiName;
    pgDI->lpName = pofndiDisk->lpName;
    pgDI->lpPath = pofndiDisk->lpPath;
    pgDI->wcDrive = pofndiDisk->wcDrive;
    pgDI->bCasePreserved = pofndiDisk->bCasePreserved;
    pgDI->dwType = pofndiDisk->dwType;
    pgDI->bValid = pofndiDisk->bValid;

    LeaveCriticalSection(&g_csLocal);

    if ((DWORD)nIndex == dwNumDisks)
    {
        dwNumDisks++;
    }

    nRet = nIndex;

AddDisk_Error:

    if (pofndiDisk)
    {
        LocalFree(pofndiDisk);
    }

    return (nRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  EnableDiskInfo
//
////////////////////////////////////////////////////////////////////////////

VOID EnableDiskInfo(
    BOOL bValid,
    BOOL bDoUnc)
{
    DWORD dwCnt = dwNumDisks;

    EnterCriticalSection(&g_csLocal);
    while (dwCnt--)
    {
        if (gaDiskInfo[dwCnt].dwType == NETDRVBMP)
        {
            if (!(DBL_BSLASH(gaDiskInfo[dwCnt].lpAbbrName)) || bDoUnc)
            {
                gaDiskInfo[dwCnt].bValid = bValid;
            }

            //
            //  Always re-invalidate remembered just in case someone
            //  escapes from fileopen, removes a connection
            //  overriding a remembered and comes back expecting to see
            //  the original remembered.
            //
        }
    }
    LeaveCriticalSection(&g_csLocal);
}


////////////////////////////////////////////////////////////////////////////
//
//  FlushDiskInfoToCmb2
//
////////////////////////////////////////////////////////////////////////////

VOID FlushDiskInfoToCmb2()
{
    DWORD dwDisk;
    DWORD dwDlg;

    for (dwDlg = 0; dwDlg < dwNumDlgs; dwDlg++)
    {
        if (gahDlg[dwDlg])
        {
            HWND hCmb2;

            if (hCmb2 = GetDlgItem(gahDlg[dwDlg], cmb2))
            {
                wNoRedraw |= 1;

                SendMessage(hCmb2, WM_SETREDRAW, FALSE, 0L);

                SendMessage(hCmb2, CB_RESETCONTENT, 0, 0);

                dwDisk = dwNumDisks;
                while (dwDisk--)
                {
                    if (gaDiskInfo[dwDisk].bValid)
                    {
                        SendMessage(
                            hCmb2,
                            CB_SETITEMDATA,
                            (WPARAM)SendMessage(
                                hCmb2,
                                CB_ADDSTRING,
                                (WPARAM)0,
                                (LPARAM)(LPTSTR)gaDiskInfo[dwDisk].lpAbbrName ),
                            (LPARAM)gaDiskInfo[dwDisk].dwType );
                    }
                }

                wNoRedraw &= ~1;

                SendMessage(hCmb2, WM_SETREDRAW, TRUE, 0L);
                InvalidateRect(hCmb2, NULL, FALSE);

                SendMessage( gahDlg[dwDlg],
                             WM_COMMAND,
                             GET_WM_COMMAND_MPS(cmb2, hCmb2, MYCBN_REPAINT) );
            }

            gahDlg[dwDlg] = NULL;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CallNetDlg
//
//  Calls the appropriate network dialog in winnet driver.
//
//  hwndParent - parent window of network dialog
//
//  Returns:  TRUE     there are new drives to display
//            FALSE    there are no new drives to display
//
////////////////////////////////////////////////////////////////////////////

BOOL CallNetDlg(
    HWND hWnd)
{
    DWORD wRet;

    HourGlass(TRUE);

    wRet = WNetConnectionDialog(hWnd, WNTYPE_DRIVE);

    if ((wRet != WN_SUCCESS) && (wRet != WN_CANCEL) && (wRet != 0xFFFFFFFF))
    {
        if (!CDLoadString( g_hinst,
                         iszNoNetButtonResponse,
                         szCaption,
                         WARNINGMSGLENGTH ))
        {
            //
            //  !!!!! CAUTION
            //  The following is not portable between code pages.
            //
            wsprintf( szWarning,
                      TEXT("Error occurred, but error resource cannot be loaded.") );
        }
        else
        {
            wsprintf(szWarning, szCaption);

            GetWindowText(hWnd, szCaption, WARNINGMSGLENGTH);
            MessageBox( hWnd,
                        szWarning,
                        szCaption,
                        MB_OK | MB_ICONEXCLAMATION );
        }
    }

    HourGlass(FALSE);

    return (wRet == WN_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetDiskType
//
////////////////////////////////////////////////////////////////////////////

UINT GetDiskType(
    LPTSTR lpszDisk)
{
    //
    //  Unfortunately GetDriveType is not for deviceless connections.
    //  So assume all unc stuff is just "remote" - no way of telling
    //  if it's a cdrom or not.
    //
    if (DBL_BSLASH(lpszDisk))
    {
        return (DRIVE_REMOTE);
    }
    else
    {
        return (GetDriveType(lpszDisk));
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  GetUNCDirectoryFromLB
//
//  If lb contains a UNC listing, the function returns the full UNC path.
//
//  Returns:   0 if no UNC listing in lb
//             length of UNC listing string
//
////////////////////////////////////////////////////////////////////////////

DWORD GetUNCDirectoryFromLB(
    HWND hDlg,
    WORD nLB,
    POPENFILEINFO pOFI)
{
    DWORD cch;
    DWORD idir;
    DWORD idirCurrent;

    cch = (DWORD)SendDlgItemMessage( hDlg,
                                     nLB,
                                     LB_GETTEXT,
                                     0,
                                     (LPARAM)(LPTSTR)pOFI->szPath );
    //
    //  If not UNC listing, return 0.
    //
    if (pOFI->szPath[0] != CHAR_BSLASH)
    {
        return (0);
    }

    idirCurrent = (WORD)(DWORD)SendDlgItemMessage( hDlg,
                                                   nLB,
                                                   LB_GETCURSEL,
                                                   0,
                                                   0L );
    if (idirCurrent < (pOFI->idirSub - 1))
    {
        pOFI->idirSub = idirCurrent;
    }
    pOFI->szPath[cch++] = CHAR_BSLASH;
    for (idir = 1; idir < pOFI->idirSub; ++idir)
    {
        cch += (DWORD)SendDlgItemMessage( hDlg,
                                          nLB,
                                          LB_GETTEXT,
                                          (WPARAM)idir,
                                          (LPARAM)(LPTSTR)&pOFI->szPath[cch] );
        pOFI->szPath[cch++] = CHAR_BSLASH;
    }

    //
    //  Only add the subdirectory if it's not the \\server\share point.
    //
    if (idirCurrent && (idirCurrent >= pOFI->idirSub))
    {
        cch += (DWORD)SendDlgItemMessage( hDlg,
                                          nLB,
                                          LB_GETTEXT,
                                          (WPARAM)idirCurrent,
                                          (LPARAM)(LPTSTR)&pOFI->szPath[cch] );
        pOFI->szPath[cch++] = CHAR_BSLASH;
    }

    pOFI->szPath[cch] = CHAR_NULL;

    return (cch);
}


////////////////////////////////////////////////////////////////////////////
//
//  SelDisk
//
//  Selects the given disk in the combo drive list.  Works for unc names,
//  too.
//
////////////////////////////////////////////////////////////////////////////

VOID SelDisk(
    HWND hDlg,
    LPTSTR lpszDisk)
{
    HWND hCmb = GetDlgItem(hDlg, cmb2);

    if (lpszDisk)
    {
        CharLower(lpszDisk);

        SendMessage( hCmb,
                     CB_SETCURSEL,
                     (WPARAM)SendMessage( hCmb,
                                          CB_FINDSTRING,
                                          (WPARAM)-1,
                                          (LPARAM)lpszDisk ),
                     0L );
    }
    else
    {
        TCHAR szChangeSel[CCHNETPATH];
        LPTSTR lpCurDir;
        LPCURDLG lpCurDlg;
        int cch = CCHNETPATH;

        if ((lpCurDlg = (LPCURDLG)TlsGetValue(g_tlsiCurDlg)) &&
            (lpCurDir = lpCurDlg->lpstrCurDir))
        {
            lstrcpy(szChangeSel, lpCurDir);
            GetCurrentDirectory(ARRAYSIZE(szChangeSel), szChangeSel);

            if ((cch = GetPathOffset(szChangeSel)) != -1)
            {
                szChangeSel[cch] = CHAR_NULL;
            }

            SendMessage( hCmb,
                         CB_SETCURSEL,
                         (WPARAM)SendMessage( hCmb,
                                              CB_FINDSTRING,
                                              (WPARAM)-1,
                                              (LPARAM)szChangeSel ),
                         0L );
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  LNDSetEvent
//
////////////////////////////////////////////////////////////////////////////

VOID LNDSetEvent(
    HWND hDlg)
{
    LPCURDLG lpCurDlg;

    lpCurDlg = (LPCURDLG)TlsGetValue(g_tlsiCurDlg);

    if ( lpCurDlg &&
         hLNDEvent &&
         !wNoRedraw &&
         hLNDThread &&
         bNetworkInstalled)
    {
        gahDlg[lpCurDlg->dwCurDlgNum] = hDlg;

        SetEvent(hLNDEvent);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  UpdateLocalDrive
//
////////////////////////////////////////////////////////////////////////////

VOID UpdateLocalDrive(
    LPTSTR szDrive,
    BOOL bGetVolName)
{
    DWORD dwFlags = 0;
    DWORD dwDriveType;
    TCHAR szVolLabel[MAX_PATH];

    //
    //  No unc here - so bypass extra call to GetDiskType and call
    //  GetDriveType directly.
    //
    dwDriveType = GetDriveType(szDrive);
    if ((dwDriveType != 0) && (dwDriveType != 1))
    {
        BOOL bRet = TRUE;

        szVolLabel[0] = CHAR_NULL;
        szDrive[1] = CHAR_COLON;
        szDrive[2] = CHAR_NULL;

        if ( bGetVolName ||
             ((dwDriveType != DRIVE_REMOVABLE) &&
              (dwDriveType != DRIVE_CDROM) &&
              (dwDriveType != DRIVE_REMOTE)) )
        {
            //
            //  Removing call to CharUpper since it causes trouble on
            //  turkish machines.
            //
            //  CharUpper(szDrive);

            if (GetFileAttributes(szDrive) != (DWORD)0xffffffff)
            {
                if (dwDriveType != DRIVE_REMOTE)
                {
                    szDrive[2] = CHAR_BSLASH;

                    bRet = GetVolumeInformation( szDrive,
                                                 szVolLabel,
                                                 MAX_PATH,
                                                 NULL,
                                                 NULL,
                                                 &dwFlags,
                                                 NULL,
                                                 (DWORD)0 );

                    //
                    //  The adddisk hack to prevent lazy loading from
                    //  overwriting the current removable media's label
                    //  with "" (because it never calls getvolumeinfo)
                    //  is to not allow null lpnames to overwrite, so when
                    //  the volume label really is null, we make it a space.
                    //
                    if (!szVolLabel[0])
                    {
                        szVolLabel[0] = CHAR_SPACE;
                        szVolLabel[1] = CHAR_NULL;
                    }
                }
            }
        }

        if (bRet)
        {
            int nIndex;

            CharLower(szDrive);

            if (dwDriveType == DRIVE_REMOTE)
            {
                nIndex = AddDisk( szDrive[0],
                                  szVolLabel,
                                  NULL,
                                  TMPNETDRV );
            }
            else
            {
                nIndex = AddDisk( szDrive[0],
                                  szVolLabel,
                                  NULL,
                                  GetDiskIndex(dwDriveType) );
            }

            if (nIndex != ADDDISK_NOCHANGE)
            {
                gaDiskInfo[nIndex].bCasePreserved =
                    (dwFlags & FS_CASE_IS_PRESERVED);
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  GetNetDrives
//
//  Enumerates network disk resources and updates the global disk info
//  structure.
//
//  dwScope   RESOURCE_CONNECTED or RESOURCE_REMEMBERED
//
//  Returns the last connection that did not previously exist.
//
////////////////////////////////////////////////////////////////////////////

VOID GetNetDrives(
    DWORD dwScope)
{
    DWORD dwRet;
    HANDLE hEnum = NULL;

    //
    //  Guard against termination with the enum handle open.
    //
    dwRet = WNetOpenEnum( dwScope,
                          RESOURCETYPE_DISK,
                          RESOURCEUSAGE_CONNECTABLE,
                          NULL,
                          &hEnum );
    if (dwRet == WN_SUCCESS)
    {
        while (dwRet == WN_SUCCESS)
        {
            DWORD dwCount = 0xffffffff;
            DWORD cbSize = cbNetEnumBuf;

            if (bLNDExit)
            {
                WNetCloseEnum(hEnum);
                return;
            }

            dwRet = WNetEnumResource(hEnum, &dwCount, gpcNetEnumBuf, &cbSize);
            switch (dwRet)
            {
                case ( WN_SUCCESS ) :
                {
                    //
                    //  Add the Entries to the listbox.
                    //
                    TCHAR wcDrive = 0;
                    NETRESOURCE *pNetRes;
                    WORD i;

                    pNetRes = (LPNETRESOURCE)gpcNetEnumBuf;

                    for (i = 0; dwCount; dwCount--, i++)
                    {
                        if (pNetRes[i].lpLocalName)
                        {
                            CharLower(pNetRes[i].lpLocalName);
                            wcDrive = *pNetRes[i].lpLocalName;
                        }
                        else
                        {
                            //
                            //  Skip deviceless names that are not
                            //  LanMan provided (or, in the case where there
                            //  is no LanMan provider name, skip deviceless
                            //  always).
                            //
                            wcDrive = 0;
                        }

                        if (!DBL_BSLASH(pNetRes[i].lpRemoteName))
                        {
                            continue;
                        }

                        //
                        //  When bGetNetDrivesSync is TRUE, we are coming back
                        //  from the Network button, so we want to cd to the
                        //  last connected drive.
                        //      (see last command in this routine)
                        //
                        if (bGetNetDrivesSync)
                        {
                            int nIndex;
                            WORD k;

                            nIndex = AddDisk( wcDrive,
                                              pNetRes[i].lpRemoteName,
                                              pNetRes[i].lpProvider,
                                              (dwScope == RESOURCE_REMEMBERED)
                                                  ? REMDRVBMP
                                                  : NETDRVBMP );

                            //
                            //  If it's a new connection, update global state.
                            //
                            if (nIndex >= 0)
                            {
                                //
                                //  Since flushdiskinfotocmb2 will clear out
                                //  the array below, remember it's state here.
                                //  It's a hack, but a nice way to find out
                                //  exactly which of the many threads
                                //  completed a net dlg operation.
                                //
                                for (k = 0; k < dwNumDlgs; k++)
                                {
                                    if (gahDlg[k])
                                    {
                                        //  Could encounter small problems with
                                        //  preemption here, but assume that
                                        //  user cannot simultaneously return
                                        //  from two different net dlg calls.
                                        //
                                        lpNetDriveSync = gaDiskInfo[nIndex].lpPath;

                                        SendMessage(
                                            gahDlg[k],
                                            WM_COMMAND,
                                            GET_WM_COMMAND_MPS(
                                                   cmb2,
                                                   GetDlgItem(gahDlg[k], cmb2),
                                                   MYCBN_CHANGEDIR ) );
                                    }
                                }
                            }
                        }
                        else
                        {
                            AddDisk( wcDrive,
                                     pNetRes[i].lpRemoteName,
                                     pNetRes[i].lpProvider,
                                     (dwScope == RESOURCE_REMEMBERED)
                                         ? REMDRVBMP
                                         : NETDRVBMP );
                        }
                    }
                    break;
                }
                case ( WN_MORE_DATA ) :
                {
                    LPTSTR pcTemp;

                    pcTemp = (LPTSTR)LocalReAlloc( gpcNetEnumBuf,
                                                   cbSize,
                                                   LMEM_MOVEABLE );
                    if (!pcTemp)
                    {
                        cbNetEnumBuf = 0;
                    }
                    else
                    {
                        gpcNetEnumBuf = pcTemp;
                        cbNetEnumBuf = cbSize;
                        dwRet = WN_SUCCESS;
                        break;
                    }
                }
                case ( WN_NO_MORE_ENTRIES ) :
                case ( WN_EXTENDED_ERROR ) :
                case ( WN_NO_NETWORK ) :
                {
                    //
                    //  WN_NO_MORE_ENTRIES is a success error code.
                    //  It is special cased when we fall out of the loop.
                    //
                    break;
                }
                case ( WN_BAD_HANDLE ) :
                default :
                {
                    break;
                }
            }
        }

        WNetCloseEnum(hEnum);

        //
        //  Flush once per event - there will always be a call with
        //  dwscope = connected.
        //
        if (dwScope == RESOURCE_CONNECTED)
        {
            FlushDiskInfoToCmb2();
        }

        if (bGetNetDrivesSync)
        {
            bGetNetDrivesSync = FALSE;
        }
    }
}


#if 0
// See comments in ListNetDrivesHandler

////////////////////////////////////////////////////////////////////////////
//
//  HideNetButton
//
////////////////////////////////////////////////////////////////////////////

VOID HideNetButton()
{
    DWORD dwDlg;
    HWND hNet;

    for (dwDlg = 0; dwDlg < dwNumDlgs; dwDlg++)
    {
        hNet = GetDlgItem(gahDlg[dwDlg], psh14);

        EnableWindow(hNet, FALSE);
        ShowWindow(hNet, SW_HIDE);
    }
}
#endif


////////////////////////////////////////////////////////////////////////////
//
//  ListNetDrivesHandler
//
////////////////////////////////////////////////////////////////////////////

VOID ListNetDrivesHandler()
{
    BOOL bInit = TRUE;
    HANDLE hEnum = NULL;
    WORD wCurDrive;
    TCHAR szDrive[5];

    if (!gpcNetEnumBuf &&
        !(gpcNetEnumBuf = (LPTSTR)LocalAlloc(LPTR, cbNetEnumBuf)))
    {
        hLNDThread = NULL;
        return;
    }

    if (bLNDExit)
    {
        goto LNDExitThread1;
    }

    EnterCriticalSection(&g_csNetThread);

    while (1)
    {
        if (bLNDExit)
        {
            goto LNDExitThread;
        }

        //
        //  hLNDEvent will always be valid since we have loaded ourself
        //  and FreeLibrary will not produce a DLL_PROCESS_DETACH.
        //
        WaitForSingleObject(hLNDEvent, INFINITE);

        //
        //  In case this is the exit event.
        //
        if (bLNDExit)
        {
            goto LNDExitThread;
        }

        EnableDiskInfo(FALSE, FALSE);

        //
        //  Get the drive information for all drives.
        //
        //  NOTE: If we don't redo all volume info, then a change in a volume
        //        label will never be caught by wow apps unless wowexec is
        //        killed and restarted.  Therefore, information for all drives
        //        should be retrieved here.
        //
        for (wCurDrive = 0; wCurDrive <= 25; wCurDrive++)
        {
            szDrive[0] = (CHAR_A + (TCHAR)wCurDrive);
            szDrive[1] = CHAR_COLON;
            szDrive[2] = CHAR_BSLASH;
            szDrive[3] = CHAR_NULL;

            UpdateLocalDrive(szDrive, FALSE);
        }

        if (bInit)
        {
            GetNetDrives(RESOURCE_REMEMBERED);

            //
            //  In case this is the exit event.
            //
            if (bLNDExit)
            {
                goto LNDExitThread;
            }

            GetNetDrives(RESOURCE_CONNECTED);

            //
            //  In case this is the exit event.
            //
            if (bLNDExit)
            {
                goto LNDExitThread;
            }

            bInit = FALSE;
        }
        else
        {
            //
            //  In case this is the exit event.
            //
            if (bLNDExit)
            {
                goto LNDExitThread;
            }

            GetNetDrives(RESOURCE_CONNECTED);

            //
            //  In case this is the exit event.
            //
            if (bLNDExit)
            {
                goto LNDExitThread;
            }
        }

        ResetEvent(hLNDEvent);
    }

LNDExitThread:

    bLNDExit = FALSE;
    LeaveCriticalSection(&g_csNetThread);

LNDExitThread1:

    FreeLibraryAndExitThread(g_hinst, 1);

    //
    //  The ExitThread is implicit in this return.
    //
    return;
}


////////////////////////////////////////////////////////////////////////////
//
//  LoadDrives
//
//  Lists the current drives (connected) in the combo box.
//
////////////////////////////////////////////////////////////////////////////

VOID LoadDrives(
    HWND hDlg)
{
    //
    //  Hard-code this - It's internal && always cmb2/psh14.
    //
    HWND hCmb = GetDlgItem(hDlg, cmb2);
    DWORD dwThreadID;
    LPCURDLG lpCurDlg;

    if (!hLNDEvent)
    {
        //
        //  Don't check if this succeeds since we can run without the net.
        //
        hLNDEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    }
    else
    {
        //
        //  Assume all previous connections (except unc) are valid
        //  for first display - but only when they exist.
        //
        EnableDiskInfo(TRUE, FALSE);
    }

    //
    //  Set the hDlg into the refresh array before initially
    //  creating the thread so that the worker thread can hide/disable
    //  the net button in the event that there is no network.
    //
    lpCurDlg = (LPCURDLG)TlsGetValue(g_tlsiCurDlg);

    // sanity check
    if (!lpCurDlg)
    {
        return;
    }

    gahDlg[lpCurDlg->dwCurDlgNum] = hDlg;

    //
    //  If there is no worker thread for network disk enumeration,
    //  start up here rather than in the dll, since it's only
    //  for the fileopen dlg.
    //
    //  Always start a thread if the number of active fileopen dialogs
    //  goes from 0 to 1
    //
    if ((lpCurDlg->dwCurDlgNum == 0) && (!hLNDThread))
    {
        if (hLNDEvent && (bNetworkInstalled = IsNetworkInstalled()))
        {
            TCHAR szModule[MAX_PATH];

            //
            //  Do this once when dialog thread count goes from 0 to 1.
            //
            GetModuleFileName(g_hinst, szModule, ARRAYSIZE(szModule));
            if (LoadLibrary(szModule))
            {
                hLNDThread = CreateThread(
                                   NULL,
                                   (DWORD)0,
                                   (LPTHREAD_START_ROUTINE)ListNetDrivesHandler,
                                   (LPVOID)NULL,
                                   (DWORD_PTR)NULL,
                                   &dwThreadID );
            }
        }
        else
        {
            HWND hNet = GetDlgItem(hDlg, psh14);

            EnableWindow(hNet, FALSE);
            ShowWindow(hNet, SW_HIDE);
        }
    }

    LNDSetEvent(hDlg);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetDiskIndex
//
////////////////////////////////////////////////////////////////////////////

DWORD GetDiskIndex(
    DWORD dwDriveType)
{
    if (dwDriveType == 1)
    {
        //
        //  Drive doesn't exist!
        //
        return (0);
    }
    else if (dwDriveType == DRIVE_CDROM)
    {
        return (CDDRVBMP);
    }
    else if (dwDriveType == DRIVE_REMOVABLE)
    {
        return (FLOPPYBMP);
    }
    else if (dwDriveType == DRIVE_REMOTE)
    {
        return (NETDRVBMP);
    }
    else if (dwDriveType == DRIVE_RAMDISK)
    {
        return (RAMDRVBMP);
    }

    return (HARDDRVBMP);
}


////////////////////////////////////////////////////////////////////////////
//
//  CleanUpFile
//
//  This releases the memory used by the system dialog bitmaps.
//
////////////////////////////////////////////////////////////////////////////

VOID CleanUpFile()
{
    //
    //  Check if anyone else is around.
    //
    if (--cLock)
    {
        return;
    }

    //
    //  Select the null bitmap into our memory DC so that the
    //  DirDrive bitmap can be discarded.
    //
    SelectObject(hdcMemory, hbmpOrigMemBmp);
}


////////////////////////////////////////////////////////////////////////////
//
//  FileOpenAbort
//
////////////////////////////////////////////////////////////////////////////

VOID FileOpenAbort()
{
    LPCURDLG lpCurDlg;


    lpCurDlg = (LPCURDLG)TlsGetValue(g_tlsiCurDlg);

    if (lpCurDlg)
    {
        EnterCriticalSection(&g_csLocal);

        if (dwNumDlgs > 0)
        {
            dwNumDlgs--;
        }

        if (dwNumDlgs == 0)
        {
            //
            //  If there are no more fileopen dialogs for this process,
            //  then signal the worker thread it's all over.
            //
            if (hLNDEvent && hLNDThread)
            {
                bLNDExit = TRUE;
                SetEvent(hLNDEvent);

                CloseHandle(hLNDThread);
                hLNDThread = NULL;
            }
        }

        LeaveCriticalSection(&g_csLocal);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  TermFile
//
////////////////////////////////////////////////////////////////////////////

VOID TermFile()
{
    vDeleteDirDriveBitmap();
    if (hdcMemory)
    {
        DeleteDC(hdcMemory);
    }

    if (hLNDEvent)
    {
        CloseHandle(hLNDEvent);
        hLNDEvent = NULL;
    }

    if (gpcNetEnumBuf)
    {
        LocalFree(gpcNetEnumBuf);
    }

    while (dwNumDisks)
    {
        dwNumDisks--;
        if (gaDiskInfo[dwNumDisks].lpAbbrName)
        {
            LocalFree(gaDiskInfo[dwNumDisks].lpAbbrName);
        }
    }
}






/*========================================================================*/
/*                 Ansi->Unicode Thunk routines                           */
/*========================================================================*/

#ifdef UNICODE

////////////////////////////////////////////////////////////////////////////
//
//  ThunkOpenFileNameA2WDelayed
//
////////////////////////////////////////////////////////////////////////////

VOID ThunkOpenFileNameA2WDelayed(
    POPENFILEINFO pOFI)
{
    LPOPENFILENAMEA pOFNA = pOFI->pOFNA;
    LPOPENFILENAMEW pOFNW = pOFI->pOFN;

    if (pOFNA->lpstrDefExt)
    {
        //
        //  Make sure the default extension buffer is at least 4 characters
        //  in length.
        //
        DWORD cbLen = max(lstrlenA(pOFNA->lpstrDefExt) + 1, 4);

        if (pOFNW->lpstrDefExt)
        {
            LocalFree((HLOCAL)pOFNW->lpstrDefExt);
        }
        if (!(pOFNW->lpstrDefExt = (LPWSTR)LocalAlloc(LPTR, (cbLen * sizeof(WCHAR)))))
        {
            StoreExtendedError(CDERR_MEMALLOCFAILURE);
            return;
        }
        else
        {
            if (pOFNA->lpstrDefExt)
            {
                SHAnsiToUnicode(pOFNA->lpstrDefExt,(LPWSTR)pOFNW->lpstrDefExt,cbLen );
            }
        }
    }

    //
    //  Need to thunk back to A value since Claris Filemaker side effects
    //  this in an ID_OK subclass without hooking at the very last moment.
    //  Do an |= instead of an = to preserve internal flags.
    //
    pOFNW->Flags &= OFN_ALL_INTERNAL_FLAGS;
    pOFNW->Flags |= pOFNA->Flags;
}


////////////////////////////////////////////////////////////////////////////
//
//  ThunkOpenFileNameA2W
//
////////////////////////////////////////////////////////////////////////////

BOOL ThunkOpenFileNameA2W(
    POPENFILEINFO pOFI)
{
    int    nRet;


    LPOPENFILENAMEA pOFNA = pOFI->pOFNA;
    LPOPENFILENAMEW pOFNW = pOFI->pOFN;

    pOFNW->Flags = pOFNA->Flags;
    pOFNW->lCustData = pOFNA->lCustData;

    //  we actually can have the original ver1 structure passed in here
    //  so we need to check and make sure to only copy over the valid data
    if ((pOFNA->lStructSize == SIZEOF(OPENFILENAMEA) && pOFNW->lStructSize == SIZEOF(OPENFILENAMEW)) 
       )
    {
        pOFNW->pvReserved = pOFNA->pvReserved;
        pOFNW->dwReserved = pOFNA->dwReserved;
        pOFNW->FlagsEx   = pOFNA->FlagsEx;
    }

    //
    //  Various WOW apps change the strings and *ptrs* to the strings in the
    //  OPENFILENAME struct while processing messages with their hook procs.
    //  Handle that silliness here.  (We probably don't want to promote this
    //  beyond WOW).
    //
    if (pOFNA->Flags & CD_WOWAPP)
    {
        pOFNW->lpstrFilter = (LPCWSTR)
                       ThunkMultiANSIStrToWIDE( (LPWSTR)pOFNW->lpstrFilter,
                                                (LPSTR)pOFNA->lpstrFilter,
                                                0 );

        pOFNW->lpstrCustomFilter =
                       ThunkMultiANSIStrToWIDE( pOFNW->lpstrCustomFilter,
                                                pOFNA->lpstrCustomFilter,
                                                pOFNA->nMaxCustFilter );

        pOFNW->lpstrFile =
                       ThunkANSIStrToWIDE( pOFNW->lpstrFile,
                                           pOFNA->lpstrFile,
                                           pOFNA->nMaxFile );

        pOFNW->lpstrFileTitle =
                       ThunkANSIStrToWIDE( pOFNW->lpstrFileTitle,
                                           pOFNA->lpstrFileTitle,
                                           pOFNA->nMaxFileTitle );

        pOFNW->lpstrInitialDir = (LPCWSTR)
                       ThunkANSIStrToWIDE( (LPWSTR)pOFNW->lpstrInitialDir,
                                           (LPSTR)pOFNA->lpstrInitialDir,
                                           0 );

        pOFNW->lpstrTitle = (LPCWSTR)
                       ThunkANSIStrToWIDE( (LPWSTR)pOFNW->lpstrTitle,
                                           (LPSTR)pOFNA->lpstrTitle,
                                           0 );

        pOFNW->lpstrDefExt = (LPCWSTR)
                       ThunkANSIStrToWIDE( (LPWSTR)pOFNW->lpstrDefExt,
                                           (LPSTR)pOFNA->lpstrDefExt,
                                           0 );

        pOFNW->nMaxCustFilter = pOFNA->nMaxCustFilter;
        pOFNW->nMaxFile       = pOFNA->nMaxFile;
        pOFNW->nMaxFileTitle  = pOFNA->nMaxFileTitle;
        pOFNW->nFileOffset    = pOFNA->nFileOffset;
        pOFNW->nFileExtension = pOFNA->nFileExtension;
    }
    else
    {
        if (pOFNW->lpstrFile)
        {
            if (pOFNA->lpstrFile)
            {
                nRet = SHAnsiToUnicode(pOFNA->lpstrFile,pOFNW->lpstrFile,pOFNW->nMaxFile );
                if (nRet == 0)
                {
                    return (FALSE);
                }
            }
        }

        if (pOFNW->lpstrFileTitle && pOFNW->nMaxFileTitle)
        {
            if (pOFNA->lpstrFileTitle)
            {
                nRet=MultiByteToWideChar(CP_ACP,
                              0,
                              pOFNA->lpstrFileTitle,
                              pOFNA->nMaxFileTitle,
                              pOFNW->lpstrFileTitle,
                              pOFNW->nMaxFileTitle);
                if (nRet == 0)
                {
                    return (FALSE);
                }
            }
        }

        if (pOFNW->lpstrCustomFilter)
        {
            if (pOFI->pasCustomFilter)
            {
                LPSTR psz = pOFI->pasCustomFilter->Buffer;
                DWORD cch = 0;

                if (*psz || *(psz + 1))
                {
                    cch = 2;
                    while (*psz || *(psz + 1))
                    {
                        psz++;
                        cch++;
                    }
                }

                if (cch)
                {
                    pOFI->pasCustomFilter->Length = cch;

                    nRet = MultiByteToWideChar(CP_ACP,
                                    0,
                                    pOFI->pasCustomFilter->Buffer,
                                    pOFI->pasCustomFilter->Length,
                                    pOFI->pusCustomFilter->Buffer,
                                    pOFI->pusCustomFilter->MaximumLength );
                    if (nRet == 0)
                    {
                        return (FALSE);
                    }
                }
            }
        }
    }

    pOFNW->nFilterIndex = pOFNA->nFilterIndex;

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  ThunkOpenFileNameW2A
//
////////////////////////////////////////////////////////////////////////////

BOOL ThunkOpenFileNameW2A(
    POPENFILEINFO pOFI)
{
    int nRet;

    LPOPENFILENAMEW pOFNW = pOFI->pOFN;
    LPOPENFILENAMEA pOFNA = pOFI->pOFNA;
    LPWSTR pszW;
    USHORT cch;

    //
    //  Supposedly invariant, but not necessarily.
    //    Definition: invariant - changed by 16-bit apps frequently
    //
    pOFNA->Flags = pOFNW->Flags;
    pOFNA->lCustData = pOFNW->lCustData;
    
    // this way we can assert that we have covered our ass.
    DEBUG_CODE(pOFNA->nFileOffset = 0 );

    //  we actually can have the original ver1 structure passed in here
    //  so we need to check and make sure to only copy over the valid data
    if (pOFNA->lStructSize == SIZEOF(OPENFILENAMEA) && pOFNW->lStructSize == SIZEOF(OPENFILENAMEW) 
       )
    {
        pOFNA->pvReserved = pOFNW->pvReserved;
        pOFNA->dwReserved = pOFNW->dwReserved;
        pOFNA->FlagsEx   = pOFNW->FlagsEx;
    }


    if (pOFNA->lpstrFileTitle && pOFNA->nMaxFileTitle)
    {
        nRet = SHUnicodeToAnsi(pOFNW->lpstrFileTitle,pOFNA->lpstrFileTitle,pOFNA->nMaxFileTitle);

        if (nRet == 0)
        {
            return (FALSE);
        }
    }

    if (pOFNA->lpstrCustomFilter)
    {
        pszW = pOFI->pusCustomFilter->Buffer;

        cch = 0;
        if (*pszW || *(pszW + 1))
        {
            cch = 2;
            while (*pszW || *(pszW + 1))
            {
                pszW++;
                cch++;
            }
        }

        if (cch)
        {
            pOFI->pusCustomFilter->Length = cch;
            nRet = WideCharToMultiByte(CP_ACP,
                                0,
                                pOFI->pusCustomFilter->Buffer,
                                pOFI->pusCustomFilter->Length,
                                pOFI->pasCustomFilter->Buffer,
                                pOFI->pasCustomFilter->MaximumLength,
                                NULL,
                                NULL);
            if (nRet == 0)
            {
                return (FALSE);
            }
        }
    }

    pOFNA->nFilterIndex   = pOFNW->nFilterIndex;

    if (pOFNA->lpstrFile && pOFNW->lpstrFile)
    {
        if (GetStoredExtendedError() == FNERR_BUFFERTOOSMALL)
        {
            //
            //  In the case where the lpstrFile buffer is too small,
            //  lpstrFile contains the size of the buffer needed for
            //  the string rather than the string itself.
            //
            pszW = pOFNW->lpstrFile;
            switch (pOFNA->nMaxFile)
            {
                case ( 3 ) :
                default :
                {
                    pOFNA->lpstrFile[2] = CHAR_NULL;

                    // fall thru...
                }
                case ( 2 ) :
                {
                    pOFNA->lpstrFile[1] = HIBYTE(*pszW);

                    // fall thru...
                }
                case ( 1 ) :
                {
                    pOFNA->lpstrFile[0] = LOBYTE(*pszW);

                    // fall thru...
                }
                case ( 0 ) :
                {
                    break;
                }
            }
        }
        else
        {
            LPWSTR pFileW = pOFNW->lpstrFile;
            DWORD cchFile = 0;

            // Find the length of string to be converted. This takes care of both single select (there will be only string)
            // and multiselect case (there will multiple strings with double null termination)
            while (*pFileW)
            {
                DWORD cch = lstrlenW(pFileW) +1;
                cchFile +=cch;
                pFileW += cch;
            }

            if (pOFNW->Flags & OFN_ALLOWMULTISELECT)
            {
                // for the double null terminator
                cchFile++;
            }
              
            // need to copy the whole buffer after the initial directory
            nRet =WideCharToMultiByte(CP_ACP,
                          0,
                          pOFNW->lpstrFile,
                          cchFile,
                          pOFNA->lpstrFile, pOFNA->nMaxFile,
                          NULL, NULL);

            if (nRet == 0)
            {
                return (FALSE);
            }

            if ((SHORT)pOFNW->nFileOffset > 0)
            {
                pOFNA->nFileOffset = (WORD) WideCharToMultiByte( CP_ACP,
                                                                 0,
                                                                 pOFNW->lpstrFile,
                                                                 pOFNW->nFileOffset,
                                                                 NULL,
                                                                 0,
                                                                 NULL,
                                                                 NULL );
            }
            else
            {
                pOFNA->nFileOffset = pOFNW->nFileOffset;
            }

            if ((SHORT)pOFNW->nFileExtension > 0)
            {
                pOFNA->nFileExtension = (WORD) WideCharToMultiByte( CP_ACP,
                                                                    0,
                                                                    pOFNW->lpstrFile,
                                                                    pOFNW->nFileExtension,
                                                                    NULL,
                                                                    0,
                                                                    NULL,
                                                                    NULL );
            }
            else
            {
                pOFNA->nFileExtension = pOFNW->nFileExtension;    
            }
        }
    }
    else
    {
        pOFNA->nFileOffset    = pOFNW->nFileOffset;
        pOFNA->nFileExtension = pOFNW->nFileExtension;

    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GenericGetFileNameA
//
////////////////////////////////////////////////////////////////////////////

BOOL GenericGetFileNameA(
    LPOPENFILENAMEA pOFNA,
    DLGPROC qfnDlgProc)
{
    LPOPENFILENAMEW pOFNW;
    BOOL bRet = FALSE;
    OFN_UNICODE_STRING usCustomFilter;
    OFN_ANSI_STRING asCustomFilter;
    DWORD cbLen;
    LPSTR pszA;
    DWORD cch;
    LPBYTE pStrMem = NULL;
    OPENFILEINFO OFI = {0};
        
    if (!pOFNA)
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

     //Set the Open File Version
    OFI.iVersion = OPENFILEVERSION;

    if (pOFNA->lStructSize == OPENFILENAME_SIZE_VERSION_400)
    {
        OFI.iVersion = OPENFILEVERSION_NT4;
    }

    //  we allow both sizes because we allocate a full size one anyway
    //  and we want to preserve the original structure for notifies
    if ((pOFNA->lStructSize != OPENFILENAME_SIZE_VERSION_400) &&
        (pOFNA->lStructSize != sizeof(OPENFILENAMEA))
       )
    {
        StoreExtendedError(CDERR_STRUCTSIZE);
        return (FALSE);
    }

    if (!(pOFNW = (LPOPENFILENAMEW)LocalAlloc(LPTR, sizeof(OPENFILENAMEW))))
    {
        StoreExtendedError(CDERR_MEMALLOCFAILURE);
        return (FALSE);
    }

    //
    //  Constant stuff.
    //
    pOFNW->lStructSize = sizeof(OPENFILENAMEW);
    pOFNW->hwndOwner = pOFNA->hwndOwner;
    pOFNW->hInstance = pOFNA->hInstance;
    pOFNW->lpfnHook = pOFNA->lpfnHook;

    //  it will always be a valid structsize at this point
    if (pOFNA->lStructSize != OPENFILENAME_SIZE_VERSION_400)
    {
        pOFNW->pvReserved = pOFNA->pvReserved;
        pOFNW->dwReserved = pOFNA->dwReserved;
        pOFNW->FlagsEx   = pOFNA->FlagsEx;
    }

    //
    //  Init TemplateName constant.
    //
    if (pOFNA->Flags & OFN_ENABLETEMPLATE)
    {
        if (!IS_INTRESOURCE(pOFNA->lpTemplateName))
        {
            cbLen = lstrlenA(pOFNA->lpTemplateName) + 1;
            if (!(pOFNW->lpTemplateName = (LPWSTR)LocalAlloc(LPTR, (cbLen * sizeof(WCHAR)))))
            {
                StoreExtendedError(CDERR_MEMALLOCFAILURE);
                goto GenericExit;
            }
            else
            {
                SHAnsiToUnicode(pOFNA->lpTemplateName,(LPWSTR)pOFNW->lpTemplateName,cbLen);
            }
        }
        else
        {
            (DWORD_PTR)pOFNW->lpTemplateName = (DWORD_PTR)pOFNA->lpTemplateName;
        }
    }
    else
    {
        pOFNW->lpTemplateName = NULL;
    }

    //
    //  Initialize Initial Dir constant.
    //
    if (pOFNA->lpstrInitialDir)
    {
        cbLen = lstrlenA(pOFNA->lpstrInitialDir) + 1;
        if (!(pOFNW->lpstrInitialDir = (LPWSTR)LocalAlloc(LPTR, (cbLen * sizeof(WCHAR)))))
        {
            StoreExtendedError(CDERR_MEMALLOCFAILURE);
            goto GenericExit;
        }
        else
        {
            SHAnsiToUnicode(pOFNA->lpstrInitialDir,(LPWSTR)pOFNW->lpstrInitialDir,cbLen);
        }
    }
    else
    {
        pOFNW->lpstrInitialDir = NULL;
    }

    //
    //  Initialize Title constant.
    //
    if (pOFNA->lpstrTitle)
    {
        cbLen = lstrlenA(pOFNA->lpstrTitle) + 1;
        if (!(pOFNW->lpstrTitle = (LPWSTR)LocalAlloc(LPTR, (cbLen * sizeof(WCHAR)))))
        {
            StoreExtendedError(CDERR_MEMALLOCFAILURE);
            goto GenericExit;
        }
        else
        {
            SHAnsiToUnicode(pOFNA->lpstrTitle,(LPWSTR)pOFNW->lpstrTitle,cbLen );
        }
    }
    else
    {
        pOFNW->lpstrTitle = NULL;
    }

    //
    //  Initialize Def Ext constant.
    //
    if (pOFNA->lpstrDefExt)
    {
        //
        //  Make sure the default extension buffer is at least 4 characters
        //  in length.
        //
        cbLen = max(lstrlenA(pOFNA->lpstrDefExt) + 1, 4);
        if (!(pOFNW->lpstrDefExt = (LPWSTR)LocalAlloc(LPTR, (cbLen * sizeof(WCHAR)))))
        {
            StoreExtendedError(CDERR_MEMALLOCFAILURE);
            goto GenericExit;
        }
        else
        {
            SHAnsiToUnicode(pOFNA->lpstrDefExt,(LPWSTR)pOFNW->lpstrDefExt,cbLen );
        }
    }
    else
    {
        pOFNW->lpstrDefExt = NULL;
    }

    //
    //  Initialize Filter constant.  Note: 16-bit apps change this.
    //
    if (pOFNA->lpstrFilter)
    {
        pszA = (LPSTR)pOFNA->lpstrFilter;

        cch = 0;
        if (*pszA || *(pszA + 1))
        {
            //
            //  Pick up trailing nulls.
            //
            cch = 2;
            try
            {
                while (*pszA || *(pszA + 1))
                {
                    pszA++;
                    cch++;
                }
            }
            except (EXCEPTION_EXECUTE_HANDLER)
            {
                StoreExtendedError(CDERR_INITIALIZATION);
                goto GenericExit;
            }
        }

        //
        //  Need to do cch + 1 in the Local Alloc rather than just cch.
        //  This is to make sure there is at least one extra null in the
        //  string so that if a filter does not have the second part of
        //  the pair, three nulls will be placed in the wide string.
        //
        //  Example:  "Print File (*.prn)\0\0\0"
        //
        if (!(pOFNW->lpstrFilter = (LPWSTR)LocalAlloc(LPTR, ((cch + 1) * sizeof(WCHAR)))))
        {
            StoreExtendedError(CDERR_MEMALLOCFAILURE);
            goto GenericExit;
        }
        else
        {
            MultiByteToWideChar(CP_ACP,
                     0,
                     pOFNA->lpstrFilter,
                     cch,
                     (LPWSTR)pOFNW->lpstrFilter, 
                     cch);
        }
    }
    else
    {
        pOFNW->lpstrFilter = NULL;
    }

    //
    //  Initialize File strings.
    //
    if (pOFNA->lpstrFile)
    {
        if (pOFNA->nMaxFile <= (DWORD)lstrlenA(pOFNA->lpstrFile))
        {
            StoreExtendedError(CDERR_INITIALIZATION);
            goto GenericExit;
        }
        pOFNW->nMaxFile = pOFNA->nMaxFile;

        if (!(pOFNW->lpstrFile = (LPWSTR)LocalAlloc(LPTR, pOFNW->nMaxFile * sizeof(WCHAR))))
        {
            StoreExtendedError(CDERR_MEMALLOCFAILURE);
            goto GenericExit;
        }
    }
    else
    {
        //
        //  Conversion done in thunkofna2w.
        //
        pOFNW->nMaxFile = 0;
        pOFNW->lpstrFile = NULL;
    }

    //
    //  Initialize File Title strings.
    //
    if (pOFNA->lpstrFileTitle && pOFNA->nMaxFileTitle)
    {
        //
        //  Calculate length of lpstrFileTitle.
        //
        pszA = pOFNA->lpstrFileTitle;
        cch = 0;
        try
        {
            while (*pszA++)
            {
                cch++;
            }
        }
        except (EXCEPTION_EXECUTE_HANDLER)
        {
            if (cch)
            {
                cch--;
            }
            (pOFNA->lpstrFileTitle)[cch] = CHAR_NULL;
        }

        if (pOFNA->nMaxFileTitle < cch)
        {
            //
            //  Override the incorrect length from the app.
            //  Make room for the null.
            //
            pOFNW->nMaxFileTitle = cch + 1;
        }
        else
        {
            pOFNW->nMaxFileTitle = pOFNA->nMaxFileTitle;
        }

        if (!(pOFNW->lpstrFileTitle = (LPWSTR)LocalAlloc(LPTR, pOFNW->nMaxFileTitle * sizeof(WCHAR))))
        {
            StoreExtendedError(CDERR_MEMALLOCFAILURE);
            goto GenericExit;
        }
    }
    else
    {
        //
        //  Conversion done in thunkofna2w.
        //
        pOFNW->nMaxFileTitle = 0;
        pOFNW->lpstrFileTitle = NULL;
    }

    //
    //  Initialize custom filter strings.
    //
    if ((asCustomFilter.Buffer = pOFNA->lpstrCustomFilter))
    {
        pszA = pOFNA->lpstrCustomFilter;

        cch = 0;
        if (*pszA || *(pszA + 1))
        {
            cch = 2;
            try
            {
                while (*pszA || *(pszA + 1))
                {
                    pszA++;
                    cch++;
                }
            }
            except (EXCEPTION_EXECUTE_HANDLER)
            {
                StoreExtendedError(CDERR_INITIALIZATION);
                goto GenericExit;
            }
        }

        //
        //  JVert-inspired-wow-compatibility-hack-to-make-vbasic2.0-makeexe
        //  save-as-dialog-box-work-even-though-the-boneheads-didn't-fill-in-
        //  the-whole-structure(nMaxCustFilter)-according-to-winhelp-spec fix
        //
        if (!(pOFNA->Flags & OFN_NOLONGNAMES))
        {
            if (((DWORD)cch >= pOFNA->nMaxCustFilter) ||
                (pOFNA->nMaxCustFilter < 40))
            {
                StoreExtendedError(CDERR_INITIALIZATION);
                goto GenericExit;
            }
            asCustomFilter.Length = cch;
            asCustomFilter.MaximumLength = pOFNA->nMaxCustFilter;
            pOFNW->nMaxCustFilter = pOFNA->nMaxCustFilter;
        }
        else
        {
            asCustomFilter.Length = cch;
            if (pOFNA->nMaxCustFilter < cch)
            {
                asCustomFilter.MaximumLength = cch;
                pOFNW->nMaxCustFilter = cch;
            }
            else
            {
                asCustomFilter.MaximumLength = pOFNA->nMaxCustFilter;
                pOFNW->nMaxCustFilter = pOFNA->nMaxCustFilter;
            }
        }
        usCustomFilter.MaximumLength = (asCustomFilter.MaximumLength + 1) * sizeof(WCHAR);
        usCustomFilter.Length = asCustomFilter.Length * sizeof(WCHAR);
    }
    else
    {
        pOFNW->nMaxCustFilter = usCustomFilter.MaximumLength = 0;
        pOFNW->lpstrCustomFilter = NULL;
    }

    if (usCustomFilter.MaximumLength > 0)
    {
        if (!(pStrMem = (LPBYTE)LocalAlloc(LPTR, usCustomFilter.MaximumLength)))
        {
            StoreExtendedError(CDERR_MEMALLOCFAILURE);
            goto GenericExit;
        }
        else
        {
            pOFNW->lpstrCustomFilter = usCustomFilter.Buffer = (LPWSTR)pStrMem;
        }
    }
    else
    {
        pStrMem = NULL;
    }

    OFI.pOFN = pOFNW;
    OFI.pOFNA = pOFNA;
    OFI.pasCustomFilter = &asCustomFilter;
    OFI.pusCustomFilter = &usCustomFilter;
    OFI.ApiType = COMDLG_ANSI;

    //
    //  The following should always succeed.
    //
    if (!ThunkOpenFileNameA2W(&OFI))
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        goto GenericExit;
    }

    bRet = GetFileName(&OFI, qfnDlgProc);
    if (bUserPressedCancel == FALSE)
    {
        ThunkOpenFileNameW2A(&OFI);
    }

GenericExit:

    if (pStrMem)
    {
        LocalFree(pStrMem);
    }

    if (!IS_INTRESOURCE(pOFNW->lpstrFile))
    {
        LocalFree((HLOCAL)pOFNW->lpstrFile);
    }

    if (!IS_INTRESOURCE(pOFNW->lpstrFileTitle))
    {
        LocalFree((HLOCAL)pOFNW->lpstrFileTitle);
    }

    if (!IS_INTRESOURCE(pOFNW->lpstrFilter))
    {
        LocalFree((HLOCAL)pOFNW->lpstrFilter);
    }

    if (!IS_INTRESOURCE(pOFNW->lpstrDefExt))
    {
        LocalFree((HLOCAL)pOFNW->lpstrDefExt);
    }

    if (!IS_INTRESOURCE(pOFNW->lpstrTitle))
    {
        LocalFree((HLOCAL)pOFNW->lpstrTitle);
    }

    if (!IS_INTRESOURCE(pOFNW->lpstrInitialDir))
    {
        LocalFree((HLOCAL)pOFNW->lpstrInitialDir);
    }

    if (!IS_INTRESOURCE(pOFNW->lpTemplateName))
    {
        LocalFree((HLOCAL)pOFNW->lpTemplateName);
    }

    LocalFree(pOFNW);

    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  Multi_strlenA
//
//  This is a strlen for ANSI string lists that have several strings that
//  are *separated* by a NULL char and are *terminated* by two NULL chars.
//
//  Returns length of string including all NULL *separators* but not the
//  2nd NULL *terminator*.  (ie. cat0dog00 would return length = 8)
//
////////////////////////////////////////////////////////////////////////////

int Multi_strlenA(
    LPCSTR str)
{
    int ctr = 0;

    if (str)
    {
        while (*str)
        {
            while (*str++)
            {
                ctr++;
            }
            ctr++;                // count the NULL separator
        }
    }

    return (ctr);
}


////////////////////////////////////////////////////////////////////////////
//
//  Multi_strcpyAtoW
//
//  This is a strcpy for string lists that have several strings that are
//  *separated* by a NULL char and are *terminated* by two NULL chars.
//  Returns FALSE if:
//    1. the wide buffer is determined to be too small
//    2. the ptr to either buffer is NULL
//  Returns TRUE if the copy was successful.
//
////////////////////////////////////////////////////////////////////////////

BOOL Multi_strcpyAtoW(
    LPWSTR pDestW,
    LPCSTR pSrcA,
    int cChars)
{
    int off = 0;
    int cb;

    if (!pSrcA || !pDestW)
    {
        return (FALSE);
    }

    cChars = max(cChars, (Multi_strlenA(pSrcA) + 1));

    if (LocalSize((HLOCAL)pDestW) < (cChars * sizeof(WCHAR)))
    {
        return (FALSE);
    }

    while (*pSrcA)
    {
        cb = lstrlenA(pSrcA) + 1;

        off += MultiByteToWideChar(CP_ACP,0,pSrcA,cb,pDestW + off, cb);
        pSrcA += cb;
    }

    pDestW[off] = L'\0';

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  ThunkMultiANSIStrToWIDE
//
//  Thunks an ANSI multi-string (a list of NULL *separated* strings with
//  two NULLs *terminating* the list) to the equivalent WIDE multi-string.
//
//  Note: If the original wide buffer is too small to contain the new list,
//        it will be free'd and a new wide buffer will be allocated.  If a
//        new wide buffer can't be allocated, the ptr to the original wide
//        buffer is returned with no changes to the contents.
//
//  Returns: ptr to the original WIDE buffer
//           OR ptr to a new wide buffer if original buffer was too small
//           OR NULL if pSrcA is NULL.
//
////////////////////////////////////////////////////////////////////////////

LPWSTR ThunkMultiANSIStrToWIDE(
    LPWSTR pDestW,
    LPSTR pSrcA,
    int cChars)
{
    int size;
    HLOCAL hBufW;

    if (!pSrcA)
    {
        //
        //  The app doesn't want a buffer for this anymore.
        //
        if (pDestW)
        {
            LocalFree((HLOCAL)pDestW);
        }
        return (NULL);
    }

    //
    //  First try to copy to the existing wide buffer since most of the time
    //  there will be no change to the buffer ptr anyway.
    //
    if (!(Multi_strcpyAtoW(pDestW, pSrcA, cChars)))
    {
        //
        //  If the wide buffer is too small (or NULL or invalid), allocate
        //  a bigger buffer.
        //
        size = max(cChars, (Multi_strlenA(pSrcA) + 1));
        cChars = size;

        if (hBufW = LocalAlloc(LPTR, (size * sizeof(WCHAR))))
        {
            //
            //  Try to copy to the new wide buffer.
            //
            if ((Multi_strcpyAtoW((LPWSTR)hBufW, pSrcA, cChars)))
            {
                if (pDestW)
                {
                    LocalFree((HLOCAL)pDestW);
                }
                pDestW = (LPWSTR)hBufW;
            }
            else
            {
                //
                //  Don't change anything.
                //
                LocalFree(hBufW);
            }
        }
    }

    return (pDestW);
}


////////////////////////////////////////////////////////////////////////////
//
//  ThunkANSIStrToWIDE
//
//  Thunks an ANSI string to WIDE.
//
//  Note: If the original wide buffer is too small to contain the new
//        string, it will be free'd and a new wide buffer will be allocated.
//        If a new wide buffer can't be allocated, the ptr to the original
//        wide buffer is returned with no changes to the contents.
//
//  Returns: ptr to the original WIDE buffer
//           OR ptr to a new wide buffer if original buffer was too small
//           OR NULL if pSrcA is NULL.
//
////////////////////////////////////////////////////////////////////////////

LPWSTR ThunkANSIStrToWIDE(
    LPWSTR pDestW,
    LPSTR pSrcA,
    int cChars)
{
    HLOCAL hBufW;
    int size;

    if (!pSrcA)
    {
        //
        //  The app doesn't want a buffer for this anymore.
        //
        if (pDestW)
        {
            LocalFree((HLOCAL)pDestW);
        }
        return (NULL);
    }

    size = max(cChars, (lstrlenA(pSrcA) + 1));
    cChars = size;

    //
    //  If the wide buffer is too small (or NULL or invalid), allocate a
    //  bigger buffer.
    //
    if (LocalSize((HLOCAL)pDestW) < (size * sizeof(WCHAR)))
    {
        if (hBufW = LocalAlloc(LPTR, (size * sizeof(WCHAR))))
        {
            //
            //  Try to copy to the new wide buffer.
            //
            if (SHAnsiToUnicode(pSrcA,(LPWSTR)hBufW,cChars ))
            {
                if (pDestW)
                {
                    LocalFree((HLOCAL)pDestW);
                }
                pDestW = (LPWSTR)hBufW;
            }
            else
            {
                //
                //  Don't change anything.
                //
                LocalFree(hBufW);
            }
        }
    }
    else
    {
        //
        //  Just use the original wide buffer.
        //
        SHAnsiToUnicode(pSrcA,pDestW, cChars);
    }

    return (pDestW);
}


#ifdef WINNT

////////////////////////////////////////////////////////////////////////////
//
//  Ssync_ANSI_UNICODE_OFN_For_WOW
//
//  Function to allow NT WOW to keep the ANSI & UNICODE versions of
//  the OPENFILENAME structure in ssync as required by many 16-bit apps.
//  See notes for Ssync_ANSI_UNICODE_Struct_For_WOW() in dlgs.c.
//
////////////////////////////////////////////////////////////////////////////

VOID Ssync_ANSI_UNICODE_OFN_For_WOW(
    HWND hDlg,
    BOOL f_ANSI_to_UNICODE)
{
    POPENFILEINFO pOFI;

    if (pOFI = (POPENFILEINFO)GetProp(hDlg, FILEPROP))
    {
        if (pOFI->pOFN && pOFI->pOFNA)
        {
            if (f_ANSI_to_UNICODE)
            {
                ThunkOpenFileNameA2W(pOFI);
            }
            else
            {
                ThunkOpenFileNameW2A(pOFI);
            }
        }
    }
}

#endif

#endif
