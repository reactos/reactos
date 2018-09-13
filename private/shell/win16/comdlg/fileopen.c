/*++

Copyright (c) 1990-1995,  Microsoft Corporation  All rights reserved.

Module Name:

    fileopen.c

Abstract:

    This module implements the Win32 fileopen dialogs.

Revision History:

--*/



//
//  Include Files.
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <port1632.h>
#include <lm.h>
#include <winnetwk.h>
#include <npapi.h>
#include <shellapi.h>
#include <shlapip.h>
#include <commctrl.h>
#include <comctrlp.h>
#include <tchar.h>
#include "privcomd.h"
#include "fileopen.h"




//
//  Constant Declarations.
//

#define WNTYPE_DRIVE    1

#define MIN_DEFEXT_LEN  4

#define BMPHIOFFSET     9

//
//  hbmpDirs array index values.
//  Note:  Two copies: for standard background, and hilite.
//         Relative order is important.
//
#define OPENDIRBMP    0
#define CURDIRBMP     1
#define STDDIRBMP     2
#define FLOPPYBMP     3
#define HARDDRVBMP    4
#define CDDRVBMP      5
#define NETDRVBMP     6
#define RAMDRVBMP     7
#define REMDRVBMP     8
  //
  //  If the following disktype is passed to AddDisk, then bTmp will be
  //  set to true in the DISKINFO structure (if the disk is new).
  //
#define TMPNETDRV     9

#define MAXDOSFILENAMELEN (12 + 1)     // 8.3 filename + 1 for NULL




//
//  Global Variables.
//

//
//  Caching drive list.
//
extern DWORD dwNumDisks;
extern OFN_DISKINFO gaDiskInfo[MAX_DISKS];

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
//  For WNet apis.
//
HANDLE hLNDThread = NULL;

extern HANDLE hMPR;
extern HANDLE hMPRUI;

TCHAR szCOMDLG32[] = TEXT("comdlg32.dll");
TCHAR szMPR[]      = TEXT("mpr.dll");
TCHAR szMPRUI[]    = TEXT("mprui.dll");

//
//  WNet stuff from mpr.dll.
//
typedef DWORD (WINAPI *LPFNWNETCONNDLG)(HWND, DWORD);
typedef DWORD (WINAPI *LPFNWNETOPENENUM)(DWORD, DWORD, DWORD, LPNETRESOURCE, LPHANDLE);
typedef DWORD (WINAPI *LPFNWNETENUMRESOURCE)(HANDLE, LPDWORD, LPVOID, LPDWORD);
typedef DWORD (WINAPI *LPFNWNETCLOSEENUM)(HANDLE);
typedef DWORD (WINAPI *LPFNWNETFORMATNETNAME)(LPTSTR, LPTSTR, LPTSTR, LPDWORD, DWORD, DWORD);
typedef DWORD (WINAPI *LPFNWNETRESTORECONN)(HWND, LPTSTR);

LPFNWNETCONNDLG lpfnWNetConnDlg;
LPFNWNETOPENENUM lpfnWNetOpenEnum;
LPFNWNETENUMRESOURCE lpfnWNetEnumResource;
LPFNWNETCLOSEENUM lpfnWNetCloseEnum;
LPFNWNETFORMATNETNAME lpfnWNetFormatNetName;
LPFNWNETRESTORECONN lpfnWNetRestoreConn;

//
//  !!!!!
//  Keep CHAR until unicode GetProcAddrW.
//
CHAR szWNetGetConn[] = "WNetGetConnectionW";
CHAR szWNetConnDlg[] = "WNetConnectionDialog";
CHAR szWNetOpenEnum[] = "WNetOpenEnumW";
CHAR szWNetEnumResource[] = "WNetEnumResourceW";
CHAR szWNetCloseEnum[] = "WNetCloseEnum";
CHAR szWNetFormatNetName[] = "WNetFormatNetworkNameW";
CHAR szWNetRestoreConn[] = "WNetRestoreConnectionW";

WNDPROC lpLBProc = NULL;
WNDPROC lpOKProc = NULL;

//
//  Drive/Dir bitmap dimensions.
//
LONG dxDirDrive = 0;
LONG dyDirDrive = 0;

//
//  BUG! This needs to be on a per dialog basis for multi-threaded apps.
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

extern DWORD g_tlsiCurDir;
extern DWORD g_tlsiCurThread;

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
//  BUG!!
//  Of course, in the case where there is a multi-threaded process
//  that has > 1 threads simultaneously calling GetFileOpen, the
//  following globals may cause problems.  Ntvdm???
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
    WNDPROC qfnDlgProc);

BOOL
FileOpenDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam);

BOOL
FileSaveDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam);

BOOL
InitFileDlg(
    HWND hDlg,
    WPARAM wParam,
    POPENFILEINFO pOFI);

INT
InitTlsValues();

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

BOOL
FileOpenCmd(
    HANDLE hDlg,
    WPARAM wP,
    DWORD lParam,
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

BOOL WINAPI
dwOKSubclass(
    HWND hOK,
    UINT msg,
    WPARAM wP,
    LPARAM lP);

BOOL WINAPI
dwLBSubclass(
    HWND hLB,
    UINT msg,
    WPARAM wP,
    LPARAM lP);

INT
InvalidFileWarning(
    HWND hDlg,
    LPTSTR szFile,
    DWORD wErrCode,
    UINT mbType);

VOID
MeasureItem(
    HWND hDlg,
    LPMEASUREITEMSTRUCT mis);

INT
Signum(
    INT nTest);

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
    LPTSTR lpDelim);

LPTSTR
ChopText(
    HWND hwndDlg,
    INT idStatic,
    LPTSTR lpch);

BOOL
FillOutPath(
    HWND hList,
    POPENFILEINFO pOFI);

INT
FListAll(
    POPENFILEINFO pOFI,
    HWND hDlg,
    LPTSTR szSpec);

INT
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

INT
DiskAddedPreviously(
    TCHAR wcDrive,
    LPTSTR lpszName);

INT
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

VOID
LoadMPR();

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
      WNDPROC qfnDlgProc);
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
            MultiByteToWideChar( CP_ACP,
                                 0,
                                 (LPSTR)lpszFileA,
                                 -1,
                                 lpszFileW,
                                 cbLen );
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
        WideCharToMultiByte( CP_ACP,
                             0,
                             lpszTitleW,
                             -1,
                             lpszTitleA,
                             cbBuf,
                             NULL,
                             NULL );
    }

    //
    // Clean up memory.
    //
    LocalFree(lpszTitleW);

    if (lpszFileW)
    {
        LocalFree(lpszFileW);
    }

    return (fResult);
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
    SetLastErrorEx(SLE_WARNING, ERROR_CALL_NOT_IMPLEMENTED);
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
    // Clean up memory.
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

    nNeeded = (SHORT)(INT)LOWORD(ParseFile(lpszFile, TRUE, FALSE));
    if (nNeeded >= 0)
    {
        //
        //  Is the filename valid?
        //
        lpszPtr = (LPTSTR)lpszFile + nNeeded;
        if ((nNeeded = (SHORT)lstrlen(lpszPtr) + 1) <= (INT)wBufSize)
        {
            //
            //  ParseFile() fails if wildcards in directory, but OK if in name.
            //  Since they arent OK here, the check is needed here.
            //
            if (mystrchr(lpszPtr, CHAR_STAR) ||
                mystrchr(lpszPtr, CHAR_QMARK))
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

    return ( GenericGetFileNameA(pOFNA, (WNDPROC)FileOpenDlgProc) );
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
    SetLastErrorEx(SLE_WARNING, ERROR_CALL_NOT_IMPLEMENTED);
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

    return ( GetFileName(&OFI, (WNDPROC)FileOpenDlgProc) );
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
    return ( GenericGetFileNameA(pOFNA, (WNDPROC)FileSaveDlgProc) );
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
    SetLastErrorEx(SLE_WARNING, ERROR_CALL_NOT_IMPLEMENTED);
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

    return ( GetFileName(&OFI, (WNDPROC)FileSaveDlgProc) );
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
    WNDPROC qfnDlgProc)
{
    LPOPENFILENAME pOFN = pOFI->pOFN;
    INT iRet;
    LPTSTR lpDlg;
    HANDLE hRes, hDlgTemplate;
    WORD wErrorMode;
    HDC hdcScreen;
    HBITMAP hbmpTemp;
    static fFirstTime = TRUE;
    LPTSTR lpCurDir, lpCurThread;
#ifdef UNICODE
    UINT uiWOWFlag = 0;
#endif

    if (!pOFN)
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    if (pOFN->lStructSize != sizeof(OPENFILENAME))
    {
        StoreExtendedError(CDERR_STRUCTSIZE);
        return (FALSE);
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
#ifdef WINNT
         (!(pOFN->Flags & CD_WOWAPP)) )
#else
         (!(GetProcessDword(GetCurrentProcessId(), GPD_FLAGS) &
            GPF_WIN16_PROCESS)) )
#endif
    {
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

        if (qfnDlgProc == (WNDPROC)FileOpenDlgProc)
        {
            return ( NewGetOpenFileName(pOFI) );
        }
        else
        {
            return ( NewGetSaveFileName(pOFI) );
        }
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
    //  Force re-compute for font changes between calls
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
    }
    else if (pOFN->Flags & OFN_ENABLETEMPLATEHANDLE)
    {
        hDlgTemplate = pOFN->hInstance;
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

        if (!(hRes = FindResource(g_hinst, lpDlg, RT_DIALOG)))
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
    //  No kernel network error dialogs.
    //
    wErrorMode = (WORD)SetErrorMode(SEM_NOERROR);
    SetErrorMode(SEM_NOERROR | wErrorMode);

    if (LockResource(hDlgTemplate))
    {
        if (pOFN->Flags & OFN_ENABLEHOOK)
        {
            glpfnFileHook = pOFN->lpfnHook;
        }

#ifdef UNICODE
        if (pOFN->Flags & CD_WOWAPP)
        {
            uiWOWFlag = SCDLG_16BIT;
        }

        iRet = DialogBoxIndirectParamAorW( g_hinst,
                                           (LPDLGTEMPLATE)hDlgTemplate,
                                           pOFN->hwndOwner,
                                           (DLGPROC)qfnDlgProc,
                                           (DWORD)pOFI,
                                           uiWOWFlag );
#else
        iRet = DialogBoxIndirectParam( g_hinst,
                                       (LPDLGTEMPLATE)hDlgTemplate,
                                       pOFN->hwndOwner,
                                       (DLGPROC)qfnDlgProc,
                                       (DWORD)pOFI );
#endif

        if ((iRet == 0) && (!bUserPressedCancel) && (!GetStoredExtendedError()))
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

    if (lpCurDir = (LPTSTR)TlsGetValue(g_tlsiCurDir))
    {
        LocalFree(lpCurDir);
    }
    if (lpCurThread = (LPTSTR)TlsGetValue(g_tlsiCurThread))
    {
        LocalFree(lpCurThread);
    }

TERMINATE:
    CleanUpFile();
    HourGlass(FALSE);
    return ((DWORD)iRet == IDOK);

ReleaseMemDC:
    DeleteDC(hdcMemory);

ReleaseScreenDC:
    ReleaseDC(HNULL, hdcScreen);

CantInit:
    return (FALSE);
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

BOOL FileOpenDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    POPENFILEINFO pOFI;
    BOOL bRet, bHookRet;


    if (pOFI = (POPENFILEINFO)GetProp(hDlg, FILEPROP))
    {
        if (pOFI->pOFN->lpfnHook)
        {
            bHookRet = (*pOFI->pOFN->lpfnHook)(hDlg, wMsg, wParam, lParam);

            if (bHookRet)
            {
                if (wMsg == WM_COMMAND)
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
                }

                return (bHookRet);
            }
        }
    }
    else if ( glpfnFileHook &&
              (wMsg != WM_INITDIALOG) &&
              (bHookRet = (*glpfnFileHook)(hDlg, wMsg, wParam, lParam)) )
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
                HWND hEdit = GetDlgItem (hDlg, edt1);

                //
                //  Grab the window style.
                //
                lStyle = GetWindowLong (hEdit, GWL_STYLE);

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

BOOL FileSaveDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    POPENFILEINFO pOFI;
    BOOL bRet, bHookRet;
    TCHAR szTitle[cbCaption];


    if (pOFI = (POPENFILEINFO) GetProp(hDlg, FILEPROP))
    {
        if (pOFI->pOFN->lpfnHook)
        {
            bHookRet = (*pOFI->pOFN->lpfnHook)(hDlg, wMsg, wParam, lParam);

            if (bHookRet)
            {
                if (wMsg == WM_COMMAND)
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
                            //  updated; they may also forget to gracefully
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
                }

                return (bHookRet);
            }
        }
    }
    else if ( glpfnFileHook &&
              (wMsg != WM_INITDIALOG) &&
              (bHookRet = (*glpfnFileHook)(hDlg, wMsg,wParam, lParam)) )
    {
        return (bHookRet);
    }

    switch(wMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            pOFI = (POPENFILEINFO)lParam;
            if (!(pOFI->pOFN->Flags &
                  (OFN_ENABLETEMPLATE | OFN_ENABLETEMPLATEHANDLE)))
            {
                LoadString(g_hinst, iszFileSaveTitle, (LPTSTR)szTitle, cbCaption);
                SetWindowText(hDlg, (LPTSTR)szTitle);
                LoadString(g_hinst, iszSaveFileAsType, (LPTSTR)szTitle, cbCaption);
                SetDlgItemText(hDlg, stc2, (LPTSTR)szTitle);
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
                HWND hEdit = GetDlgItem (hDlg, edt1);

                //
                //  Grab the window style.
                //
                lStyle = GetWindowLong (hEdit, GWL_STYLE);

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

BOOL InitFileDlg(
    HWND hDlg,
    WPARAM wParam,
    POPENFILEINFO pOFI)
{
    DWORD lRet;
    LPOPENFILENAME pOFN = pOFI->pOFN;
    INT nFileOffset, nExtOffset;
    RECT rRect;
    RECT rLbox;
    BOOL bRet;

    if (!InitTlsValues())
    {
        //
        //  The extended error is set inside of the above call.
        //
        EndDialog(hDlg, FALSE);
        return (FALSE);
    }

    lpLBProc = (WNDPROC)GetWindowLong(GetDlgItem(hDlg, lst2), GWL_WNDPROC);
    lpOKProc = (WNDPROC)GetWindowLong(GetDlgItem(hDlg, IDOK), GWL_WNDPROC);

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

        lRet = ParseFile(pOFN->lpstrFile, TRUE, pOFN->Flags & CD_WOWAPP);
        nFileOffset = (INT)(SHORT)LOWORD(lRet);
        nExtOffset  = (INT)(SHORT)HIWORD(lRet);

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

        EnableWindow(hHelp = GetDlgItem(hDlg, psh15), FALSE);

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

    SendDlgItemMessage(hDlg, edt1, EM_LIMITTEXT, (WPARAM) MAX_PATH, (LPARAM) 0L);


    //
    //  Insert file specs into cmb1.
    //  Custom filter first.
    //  Must also check if filter contains anything.
    //
    if ( pOFN->lpstrFile &&
         (mystrchr(pOFN->lpstrFile, CHAR_STAR) ||
          mystrchr(pOFN->lpstrFile, CHAR_QMARK)) )
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
                            (LONG)pOFN->lpstrCustomFilter );

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
        SendDlgItemMessage( hDlg,
                            cmb1,
                            CB_SETCURSEL,
                            (WPARAM)(pOFN->nFilterIndex),
                            0L );

        SendMessage( hDlg,
                     WM_COMMAND,
                     GET_WM_COMMAND_MPS( cmb1,
                                         GetDlgItem(hDlg, cmb1),
                                         MYCBN_DRAW ) );

        if (!(pOFN->lpstrFile && *pOFN->lpstrFile))
        {
            LPCTSTR lpFilter;

            if (pOFN->nFilterIndex ||
                !(pOFN->lpstrCustomFilter && * pOFN->lpstrCustomFilter))
            {
                lpFilter = pOFN->lpstrFilter +
                           SendDlgItemMessage( hDlg,
                                               cmb1,
                                               CB_GETITEMDATA,
                                               (WPARAM)pOFN->nFilterIndex, 0L );
            }
            else
            {
                lpFilter = pOFN->lpstrCustomFilter +
                           lstrlen(pOFN->lpstrCustomFilter) + 1;
            }
            if (*lpFilter)
            {
                TCHAR szText[MAX_FULLPATHNAME];

                lstrcpy(szText, lpFilter);

                //
                //  Filtering is case-insensitive.
                //
                CharLower(szText);

                if (pOFI->szLastFilter[0] == CHAR_NULL)
                {
                    lstrcpy(pOFI->szLastFilter, (LPTSTR)szText);
                }

                SetDlgItemText(hDlg, edt1, (LPTSTR)szText);
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
                          pOFN->Flags & CD_WOWAPP );
        nFileOffset = (INT)(SHORT)LOWORD(lRet);
        nExtOffset  = (INT)(SHORT)HIWORD(lRet);

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
        SetDlgItemText(hDlg, edt1, (LPTSTR)szText);
    }

    SetWindowLong(GetDlgItem(hDlg, lst2), GWL_WNDPROC, (LONG)dwLBSubclass);
    SetWindowLong(GetDlgItem(hDlg, IDOK), GWL_WNDPROC, (LONG)dwOKSubclass);

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
        ScreenToClient(hDlg, (LPPOINT)&(rRect.left));
        ScreenToClient(hDlg, (LPPOINT)&(rRect.right));
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
#ifdef UNICODE
        if (pOFI->ApiType == COMDLG_ANSI)
        {
            ThunkOpenFileNameW2A(pOFI);
            bRet = ((*pOFN->lpfnHook)( hDlg,
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
            bRet = ((*pOFN->lpfnHook)( hDlg,
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

INT InitTlsValues()
{
    //
    //  As long as we do not call TlsGetValue before this,
    //  everything should be ok.
    //
    LPTSTR lpCurDir;
    LPDWORD lpCurThread;

    if (dwNumDlgs == MAX_THREADS)
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    if (lpCurDir = (LPTSTR)LocalAlloc(LPTR, CCHNETPATH * sizeof(TCHAR)))
    {
        GetCurrentDirectory(CCHNETPATH, lpCurDir);

        if (!TlsSetValue(g_tlsiCurDir, (LPVOID)lpCurDir))
        {
            StoreExtendedError(CDERR_INITIALIZATION);
            LocalFree(lpCurDir);
            return (FALSE);
        }
    }
    else
    {
        StoreExtendedError(CDERR_MEMALLOCFAILURE);
        return (FALSE);
    }

    if (lpCurThread = (LPDWORD)LocalAlloc(LPTR, sizeof(DWORD)))
    {
        if (!TlsSetValue(g_tlsiCurThread, (LPVOID)lpCurThread))
        {
            StoreExtendedError(CDERR_INITIALIZATION);
            LocalFree(lpCurDir);
            LocalFree(lpCurThread);
            return (FALSE);
        }
    }
    else
    {
        StoreExtendedError(CDERR_MEMALLOCFAILURE);
        return (FALSE);
    }

    EnterCriticalSection(&g_csLocal);

    *lpCurThread = dwNumDlgs++;

    LeaveCriticalSection(&g_csLocal);

    return (TRUE);
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
        nIndex = SendDlgItemMessage( hDlg,
                                     cmb1,
                                     CB_ADDSTRING,
                                     0,
                                     (LONG)lpszFilter );
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


////////////////////////////////////////////////////////////////////////////
//
//  InitCurrentDisk
//
////////////////////////////////////////////////////////////////////////////

VOID InitCurrentDisk(
    HWND hDlg,
    POPENFILEINFO pOFI,
    WORD cmb)
{
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
        //  but that TlsGetValue(g_tlsiCurDir) will return "" which
        //  when fed to SheChangeDirEx means GetCurrentDir will be called.
        //  So, the default cd behavior at startup is:
        //      1. lpstrInitialDir
        //      2. GetCurrentDir
        //
        ChangeDir(hDlg, pOFI->pOFN->lpstrInitialDir, TRUE, FALSE);
    }
    else
    {
        ChangeDir(hDlg, NULL, TRUE, FALSE);
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

    GetObject(hbmp, sizeof(BITMAP), (LPTSTR) &bmp);
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

BOOL FileOpenCmd(
    HANDLE hDlg,
    WPARAM wP,
    DWORD lParam,
    POPENFILEINFO pOFI,
    BOOL bSave)
{
    LPOPENFILENAME pOFN;
    LPTSTR pch, pch2;
    WORD i, sCount, len;
    LRESULT wFlag;
    BOOL bRet, bHookRet;
    TCHAR szText[MAX_FULLPATHNAME];
    HWND hwnd;


    if (!pOFI)
    {
        return (FALSE);
    }

    pOFN = pOFI->pOFN;
    switch (GET_WM_COMMAND_ID(wP, lParam))
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
                //  Visual Basic passes in an uninitialized lpDefExts string.
                //  Since we only have to use it in OKButtonPressed, update
                //  lpstrDefExts here along with whatever else is only needed
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

            if (((GET_WM_COMMAND_ID(wP, lParam)) == IDOK) && pOFN->lpfnHook)
            {
#ifdef UNICODE
                if (pOFI->ApiType == COMDLG_ANSI)
                {
                    ThunkOpenFileNameW2A(pOFI);
                    bHookRet = (*pOFN->lpfnHook)( hDlg,
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
                    bHookRet = (*pOFN->lpfnHook)( hDlg,
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
                glpfnFileHook = pOFN->lpfnHook;
            }

            RemoveProp(hDlg, FILEPROP);

            EndDialog(hDlg, bRet);

            if (pOFI)
            {
                if (((pOFN->Flags & OFN_NOCHANGEDIR) && *pOFI->szCurDir) ||
                    (bUserPressedCancel))
                {
                    ChangeDir(hDlg, pOFI->szCurDir, TRUE, FALSE);
                }
            }

            //
            //  BUG BUG
            //  If the app subclasses ID_ABORT, the worker thread will never
            //  get exited.  This will cause problems.  Currently, there are
            //  no apps that do this, though.
            //

            return (TRUE);
            break;
        }
        case ( edt1 ) :
        {
            if (GET_WM_COMMAND_CMD(wP, lParam) == EN_CHANGE)
            {
                INT iIndex, iCount;
                HWND hLBox = GetDlgItem(hDlg, lst1);
                WORD wIndex = (WORD)SendMessage(hLBox, LB_GETCARETINDEX, 0, 0);

                szText[0] = CHAR_NULL;

                if (wIndex == (WORD)LB_ERR)
                {
                    break;
                }

                SendMessage( GET_WM_COMMAND_HWND(wP, lParam),
                             WM_GETTEXT,
                             (WPARAM)MAX_FULLPATHNAME,
                             (LPARAM)(LPTSTR)szText );

                if ((iIndex = (INT)SendMessage( hLBox,
                                                LB_FINDSTRING,
                                                (WPARAM)(wIndex - 1),
                                                (LPARAM)(LPTSTR)szText )) != LB_ERR)
                {
                    RECT rRect;

                    iCount = (INT)SendMessage(hLBox, LB_GETTOPINDEX, 0, 0L);
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
            if (GET_WM_COMMAND_CMD(wP, lParam)== LBN_DBLCLK)
            {
                SendMessage(hDlg, WM_COMMAND, GET_WM_COMMAND_MPS(IDOK, 0, 0));
                return (TRUE);
            }
            else if (pOFN && (GET_WM_COMMAND_CMD(wP, lParam) == LBN_SELCHANGE))
            {
                if (pOFN->Flags & OFN_ALLOWMULTISELECT)
                {
                    int *pSelIndex;

                    //
                    //  Muliselection allowed.
                    //
                    sCount = (SHORT)SendMessage(GET_WM_COMMAND_HWND(wP, lParam),
                                                LB_GETSELCOUNT,
                                                0,
                                                0L );
                    if (!sCount)
                    {
                        //
                        //  If nothing selected, clear edit control.
                        //
                        SetDlgItemText(hDlg, edt1, (LPTSTR)szNull);
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
                                            GET_WM_COMMAND_HWND(wP, lParam),
                                            LB_GETSELITEMS,
                                            (WPARAM)sCount,
                                            (LONG)(LPTSTR)pSelIndex );

                        pch2 = pch = (LPTSTR)
                             LocalAlloc(LPTR, cchMemBlockSize * sizeof(TCHAR));
                        if (!pch)
                        {
                            goto LocalFailure2;
                        }

                        for (*pch = CHAR_NULL, i = 0; i < sCount; i++)
                        {
                            len = (WORD)SendMessage(
                                            GET_WM_COMMAND_HWND(wP, lParam),
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
                                UINT cchPrevLen = cchTotalLength - (len + 2);

                                cchMemBlockSize = cchMemBlockSize << 1;
                                pch = (LPTSTR)LocalReAlloc(
                                                 pch,
                                                 cchMemBlockSize * sizeof(TCHAR),
                                                 LMEM_MOVEABLE );
                                if (pch)
                                {
                                    pch2 = pch + cchPrevLen;
                                }
                                else
                                {
                                    goto LocalFailure2;
                                }
                            }

                            SendMessage( GET_WM_COMMAND_HWND(wP, lParam),
                                         LB_GETTEXT,
                                         (WPARAM)(*(pSelIndex + i)),
                                         (LONG)(LPTSTR)pch2 );

                            if (!mystrchr((LPTSTR)pch2, CHAR_DOT))
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
                        i = (WORD)SendMessage( GET_WM_COMMAND_HWND(wP, lParam),
                                               LB_GETCARETINDEX,
                                               0,
                                               0L );
                        if (!(i & 0x8000))
                        {
                            wFlag = (SendMessage(
                                         GET_WM_COMMAND_HWND(wP, lParam),
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

                    i = (WORD)SendMessage( GET_WM_COMMAND_HWND(wP, lParam),
                                           LB_GETCURSEL,
                                           0,
                                           0L );

                    if (i != (WORD)LB_ERR)
                    {
                        i = (WORD)SendMessage( GET_WM_COMMAND_HWND(wP, lParam),
                                               LB_GETTEXT,
                                               (WPARAM)i,
                                               (LONG)(LPTSTR)szText );

                        if (!mystrchr((LPTSTR)szText, CHAR_DOT))
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

                        SetDlgItemText(hDlg, edt1, (LPTSTR)szText);
                        if (pOFN->lpfnHook)
                        {
                            i = (WORD)SendMessage(
                                          GET_WM_COMMAND_HWND(wP, lParam),
                                          LB_GETCURSEL,
                                          0,
                                          0L );
                            wFlag = CD_LBSELCHANGE;
                        }
                    }
                }

                if (pOFN->lpfnHook)
                {
#ifdef UNICODE
                    if (pOFI->ApiType == COMDLG_ANSI)
                    {
                        (*pOFN->lpfnHook)( hDlg,
                                           msgLBCHANGEA,
                                           lst1,
                                           MAKELONG(i, wFlag) );
                    }
                    else
#endif
                    {
                        (*pOFN->lpfnHook)( hDlg,
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
            switch (GET_WM_COMMAND_CMD(wP, lParam))
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
                                        (LPTSTR)szText,
                                        MAX_FULLPATHNAME - 1 );
                        bRet = (!szText[0] ||
                                (mystrchr((LPTSTR)szText, CHAR_STAR)) ||
                                (mystrchr((LPTSTR)szText, CHAR_QMARK)));
                        lstrcpy(szText, lpFilter);
                        if (bRet)
                        {
                            CharLower(szText);
                            SetDlgItemText(hDlg, edt1, (LPTSTR)szText);
                            SendDlgItemMessage( hDlg,
                                                edt1,
                                                EM_SETSEL,
                                                (WPARAM)0,
                                                (LPARAM)-1 );
                        }
                        FListAll(pOFI, hDlg, (LPTSTR)szText);
                        if (!bInitializing)
                        {
                            lstrcpy(pOFI->szLastFilter, (LPTSTR)szText);

                            //
                            //  Provide dynamic lpstrDefExt updating
                            //  when lpstrDefExt is user initialized.
                            //
                            if (mystrchr((LPTSTR)lpFilter, CHAR_DOT) &&
                                pOFN->lpstrDefExt)
                            {
                                DWORD cbLen = MIN_DEFEXT_LEN - 1; // only 1st 3
                                LPTSTR lpTemp = (LPTSTR)pOFN->lpstrDefExt;

                                while (*lpFilter++ != CHAR_DOT);
                                if (!(mystrchr((LPTSTR)lpFilter, CHAR_STAR)) &&
                                    !(mystrchr((LPTSTR)lpFilter, CHAR_QMARK)))
                                {
                                    while (cbLen--)
                                    {
                                        *lpTemp++ = *lpFilter++;
                                    }
                                    *lpTemp = CHAR_NULL;
                                }
                            }
                        }
                    }
                    if (pOFN->lpfnHook)
                    {
#ifdef UNICODE
                        if (pOFI->ApiType == COMDLG_ANSI)
                        {
                            (*pOFN->lpfnHook)( hDlg,
                                               msgLBCHANGEA,
                                               cmb1,
                                               MAKELONG(nIndex, CD_LBSELCHANGE) );
                        }
                        else
#endif
                        {
                            (*pOFN->lpfnHook)( hDlg,
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
            if (GET_WM_COMMAND_CMD(wP, lParam) == LBN_SELCHANGE)
            {
                if (!(pOFN->Flags & OFN_DIRSELCHANGED))
                {
                    if ((DWORD)SendDlgItemMessage( hDlg,
                                                   lst2,
                                                   LB_GETCURSEL,
                                                   0,
                                                   0L ) != pOFI->idirSub - 1)
                    {
                        StripFileName(hDlg, pOFN->Flags & CD_WOWAPP);
                        pOFN->Flags |= OFN_DIRSELCHANGED;
                    }
                }
                return (TRUE);
            }
            else if (GET_WM_COMMAND_CMD(wP, lParam) == LBN_SETFOCUS)
            {
                EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
                SendMessage( GetDlgItem(hDlg, IDCANCEL),
                             BM_SETSTYLE,
                             (WPARAM)BS_PUSHBUTTON,
                             (LPARAM)TRUE );
            }
            else if (GET_WM_COMMAND_CMD(wP, lParam) == LBN_KILLFOCUS)
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
            else if (GET_WM_COMMAND_CMD(wP, lParam) == LBN_DBLCLK)
            {
                TCHAR szNextDir[CCHNETPATH];
                LPTSTR lpCurDir;
                DWORD idir;
                DWORD idirNew;
                INT cb;
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
                    cb = SendDlgItemMessage( hDlg,
                                             lst2,
                                             LB_GETTEXT,
                                             (WPARAM)idirNew,
                                             (LONG)(LPTSTR)pOFI->szPath );
                    //
                    //  sanity check
                    //
                    if (!(lpCurDir = (LPTSTR)TlsGetValue(g_tlsiCurDir)))
                    {
                        break;
                    }

                    lstrcpy((LPTSTR)szNextDir, lpCurDir);

                    //
                    //  Fix phenom with c:\\foobar - cz of incnstncy in dir
                    //  dsply guaranteed to have a valid lpCurDir here, right?
                    //
                    if (szNextDir[lstrlen(lpCurDir) - 1] != CHAR_BSLASH)
                    {
                        lstrcat((LPTSTR)szNextDir, TEXT("\\"));
                    }

                    lstrcat((LPTSTR)szNextDir, pOFI->szPath);

                    pstrPath = (LPTSTR)szNextDir;

                    idirNew = pOFI->idirSub;    // for msgLBCHANGE message
                }
                else
                {
                    //
                    //  Need full path name.
                    //
                    cb = SendDlgItemMessage( hDlg,
                                             lst2,
                                             LB_GETTEXT,
                                             0,
                                             (LONG)(LPTSTR)pOFI->szPath );

                    //
                    //  The following condition is necessary because wb displays
                    //  \\server\share (the disk resource name) for unc, but
                    //  for root paths (eg. c:\) for device conns, this in-
                    //  consistency is hacked around here and in FillOutPath.
                    //
                    if (DBL_BSLASH((LPTSTR)pOFI->szPath))
                    {
                        lstrcat((LPTSTR)pOFI->szPath, TEXT("\\"));
                        cb++;
                    }

                    for (idir = 1; idir <= idirNew; ++idir)
                    {
                        cb += SendDlgItemMessage(
                                     hDlg,
                                     lst2,
                                     LB_GETTEXT,
                                     (WPARAM)idir,
                                     (LONG)(LPTSTR)&pOFI->szPath[cb] );
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
                UpdateListBoxes(hDlg, pOFI, (LPTSTR) NULL, mskDirectory);

                if (pOFN->lpfnHook)
                {
#ifdef UNICODE
                    if (pOFI->ApiType == COMDLG_ANSI)
                    {
                        (*pOFN->lpfnHook)( hDlg,
                                           msgLBCHANGEA,
                                           lst2,
                                           MAKELONG(LOWORD(idirNew), CD_LBSELCHANGE) );
                    }
                    else
#endif
                    {
                        (*pOFN->lpfnHook)( hDlg,
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
            switch (GET_WM_COMMAND_CMD(wP, lParam))
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
                                     GET_WM_COMMAND_HWND(wP, lParam),
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
                    TCHAR szRepaintDir[CCHNETPATH];
                    LPTSTR lpCurDir;
                    DWORD cchCurDir;
                    HWND hCmb2 = (HWND)lParam;

                    // sanity
                    if (!(lpCurDir = (LPTSTR)TlsGetValue(g_tlsiCurDir)))
                    {
                        break;
                    }

                    lstrcpy(szRepaintDir, lpCurDir);

                    // BUG BUG: Only Unicode
                    cchCurDir = SheGetPathOffsetW((LPWSTR)szRepaintDir);

                    szRepaintDir[cchCurDir] = CHAR_NULL;

                    // Should always succeed
                    SendMessage( hCmb2,
                                 CB_SELECTSTRING,
                                 (WPARAM)-1,
                                 (LPARAM)(LPTSTR)szRepaintDir );
                    break;
                }
                case ( CBN_SELCHANGE ) :
                {
                    StripFileName(hDlg, pOFN->Flags & CD_WOWAPP);

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
                    INT nDiskInd, nInd;
                    DWORD dwType;
                    LPTSTR lpszPath = NULL;
                    LPTSTR lpszDisk = NULL;
                    HWND hCmb2;
                    OFN_DISKINFO *pofndiDisk = NULL;
                    static szDrawDir[CCHNETPATH];
                    INT nRet;

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
                        nInd = SendMessage(hCmb2, CB_GETCURSEL, 0, 0L);

                        if (nInd != CB_ERR)
                        {
                            SendMessage( hCmb2,
                                         CB_GETLBTEXT,
                                         nInd,
                                         (LPARAM)(LPTSTR)szDrawDir );
                        }

                        if ((nInd == CB_ERR) || ((INT)pofndiDisk == CB_ERR))
                        {
                            LPTSTR lpCurDir;

                            if (lpCurDir = (LPTSTR)TlsGetValue(g_tlsiCurDir))
                            {
                                lstrcpy((LPTSTR)szDrawDir, lpCurDir);
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

                    if ((GET_WM_COMMAND_CMD(wP, lParam)) == MYCBN_CHANGEDIR)
                    {
                        if (lpNetDriveSync)
                        {
                            lpszPath = lpNetDriveSync;
                            lpNetDriveSync = NULL;
                        }
                        else
                        {
                            LPTSTR lpCurDir;

                            if (lpCurDir = (LPTSTR)TlsGetValue(g_tlsiCurDir))
                            {
                                lstrcpy((LPTSTR)szDrawDir, lpCurDir);

                                lpszPath = (LPTSTR)szDrawDir;
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
                            (mystrchr(pOFN->lpstrFile, CHAR_STAR) ||
                             mystrchr(pOFN->lpstrFile, CHAR_QMARK)))
                        {
                            lstrcpy(lpFilter, pOFN->lpstrFile);
                        }
                        else
                        {
                            HWND hcmb1 = GetDlgItem(hDlg, cmb1);

                            nInd = SendMessage(hcmb1, CB_GETCURSEL, 0, 0L);
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
                                lpFilter = (LPTSTR)pOFN->lpstrFilter;
                                lpFilter += SendMessage( hcmb1,
                                                         CB_GETITEMDATA,
                                                         (WPARAM)nInd,
                                                         0 );
                            }
                            else
                            {
                                lpFilter = (LPTSTR)pOFN->lpstrCustomFilter;
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
                        lstrcpy((LPTSTR)szTitle, lpFilter);
                        CharLower(szTitle);
                    }

                    if (dwType == REMDRVBMP)
                    {
                        if (lpfnWNetRestoreConn)
                        {
                            DWORD err;

                            err = (*lpfnWNetRestoreConn)(hDlg, lpszDisk);

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
                        INT mbRet;

                        while (nRet == CHANGEDIR_FAILED)
                        {
                            if (dwType == FLOPPYBMP)
                            {
                                mbRet = InvalidFileWarning(
                                               hDlg,
                                               lpszPath,
                                               ERROR_NO_DISK_IN_DRIVE,
                                               (UINT)(MB_RETRYCANCEL |
                                                      MB_ICONEXCLAMATION) );
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
                                     lpFilter ? (LPTSTR)szTitle : lpFilter,
                                     (WORD)(mskDrives | mskDirectory) );

                    if (pOFN->lpfnHook)
                    {
                        nInd = SendDlgItemMessage( hDlg,
                                                   cmb2,
                                                   CB_GETCURSEL,
                                                   0,
                                                   0 );
#ifdef UNICODE
                        if (pOFI->ApiType == COMDLG_ANSI)
                        {
                            (*pOFN->lpfnHook)( hDlg,
                                               msgLBCHANGEA,
                                               cmb2,
                                               MAKELONG(LOWORD(nInd),
                                                        CD_LBSELCHANGE) );
                        }
                        else
#endif
                        {
                            (*pOFN->lpfnHook)( hDlg,
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
        case ( psh15 ) :
        {
#ifdef UNICODE
            if (pOFI->ApiType == COMDLG_ANSI)
            {
                if (msgHELPA && pOFN->hwndOwner)
                {
                    SendMessage( pOFN->hwndOwner,
                                 msgHELPA,
                                 (WPARAM)hDlg,
                                 (DWORD)pOFN );
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
                                 (DWORD)pOFN );
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

#define MAXFILTERS  36
#define EXCLBITS    (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)

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
    static TCHAR szSemiColonSpaceTab[] = TEXT("; \t");
    static TCHAR szSemiColonTab[] = TEXT(";\t");
    BOOL bDriveChange;
    BOOL bFindAll = FALSE;
    RECT rDirLBox;
    BOOL bLFN;
    HANDLE hff;
    DWORD dwErr;
    WIN32_FIND_DATA FindFileData;
    TCHAR szBuffer[MAX_FULLPATHNAME];       // add one for CHAR_DOT
    WORD wCount;



    //
    //  Save the drive bit and then clear it out.
    //
    bDriveChange = wMask & mskDrives;
    wMask &= ~mskDrives;

    if (!lpszFilter)
    {
        GetDlgItemText( hDlg,
                        edt1,
                        lpszFilter = (LPTSTR)szSpec,
                        MAX_FULLPATHNAME - 1 );

        //
        //  If any directory or drive characters are in there, or if there
        //  are no wildcards, use the default spec.
        //
        if ( mystrchr((LPTSTR)szSpec, CHAR_BSLASH) ||
             mystrchr((LPTSTR)szSpec, CHAR_SLASH)  ||
             mystrchr((LPTSTR)szSpec, CHAR_COLON)  ||
             (!((mystrchr((LPTSTR)szSpec, CHAR_STAR)) ||
                (mystrchr((LPTSTR)szSpec, CHAR_QMARK)))) )
        {
            lstrcpy((LPTSTR)szSpec, (LPTSTR)pOFI->szSpecCur);
        }
        else
        {
            lstrcpy((LPTSTR)pOFI->szLastFilter, szSpec);
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
        //  CharUpper((LPTSTR)lpszF[nFilters]);

        //
        //  Compare the filter with *.*.  If we find *.* then
        //  set the boolean bFindAll, and this will cause the
        //  files listbox to be filled in at the same time the
        //  directories listbox is filled.  This saves time
        //  from walking the directory twice (once for the directory
        //  names and once for the filenames).
        //
        if (!_tcsicmp(lpszF[nFilters], (LPTSTR)szStarDotStar))
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
            if (!_tcsicmp(lpszF[nFilters], lpszF[wCount]))
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
        if ((pOFI->pOFN->Flags & OFN_NOLONGNAMES) &&
            (FindFileData.cAlternateFileName[0] != CHAR_NULL))
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
                //
                //  Use the alternate file name.
                //
                lstrcpy(szBuffer, (LPTSTR)FindFileData.cAlternateFileName);
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
            if (StrChr((LPTSTR)szBuffer, CHAR_SPACE))
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
                                       (DWORD)(LPTSTR)szBuffer );
            }
        }
        else if (bFindAll)
        {
            if (!bCasePreserved)
            {
                CharLower(szBuffer);
            }

            SendMessage(hFileList, LB_ADDSTRING, 0, (DWORD)(LPTSTR)szBuffer);
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
            if (!_tcsicmp(lpszF[i], (LPTSTR)szStarDotStar))
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
                if ((pOFI->pOFN->Flags & OFN_NOLONGNAMES) &&
                    (FindFileData.cAlternateFileName[0] != CHAR_NULL))
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
                        //
                        //  Use the alternate file name.
                        //
                        lstrcpy(szBuffer, (LPTSTR)FindFileData.cAlternateFileName);
                    }
                }
                else
                {
                    lstrcpy(szBuffer, (LPTSTR)FindFileData.cFileName);

                    if (pOFI->pOFN->Flags & OFN_ALLOWMULTISELECT)
                    {
                        if (StrChr((LPTSTR)szBuffer, CHAR_SPACE))
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

                SendMessage(hFileList, LB_ADDSTRING, 0, (DWORD)(LPTSTR)szBuffer);
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
            LPTSTR lpCurDir = (LPTSTR)TlsGetValue(g_tlsiCurDir);

            FillOutPath(hDirList, pOFI);

            //
            //  The win31 way of chopping the text by just passing
            //  it on to user doesn't work for unc names since user
            //  doesn't see the drivelessness of them (thinks drive is
            //  a bslash char).  So, special case it here.
            //
            lstrcpy((LPTSTR)pOFI->szPath, lpCurDir);

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
            SetDlgItemText(hDlg, stc1, (LPTSTR)szNull);
        }

        wNoRedraw &= ~2;
        SendMessage(hDirList, WM_SETREDRAW, TRUE, 0L);

        GetWindowRect(hDirList, (LPRECT)&rDirLBox);
        rDirLBox.left++, rDirLBox.top++;
        rDirLBox.right--, rDirLBox.bottom--;
        ScreenToClient(hDlg, (LPPOINT)&(rDirLBox.left));
        ScreenToClient(hDlg, (LPPOINT)&(rDirLBox.right));

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
    INT nFileOffset, nExtOffset;
    HANDLE hFile;
    BOOL bAddExt = FALSE;
    BOOL bUNCName;
    INT nTempOffset;
    TCHAR szPathName[MAX_FULLPATHNAME];
    DWORD lRet;
    BOOL blfn;


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
        else if ((pOFI->szPath[cch] == CHAR_BSLASH) ||
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

    lRet = ParseFile((LPTSTR)pOFI->szPath, blfn, pOFN->Flags & CD_WOWAPP);
    nFileOffset = (INT)(SHORT)LOWORD(lRet);
    nExtOffset  = (INT)(SHORT)HIWORD(lRet);

    if (nFileOffset == PARSE_EMPTYSTRING)
    {
        UpdateListBoxes(hDlg, pOFI, (LPTSTR) NULL, 0);
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
        nFileOffset = (INT)(SHORT)LOWORD(ParseFile( (LPTSTR)pOFI->szPath,
                                                    blfn,
                                                    pOFN->Flags & CD_WOWAPP ));
        pOFI->szPath[nExtOffset] = CHAR_SEMICOLON;
        if ( (nFileOffset >= 0) &&
             (mystrchr(pOFI->szPath + nFileOffset, CHAR_STAR) ||
              mystrchr(pOFI->szPath + nFileOffset, CHAR_QMARK)) )
        {
            lstrcpy(pOFI->szLastFilter, pOFI->szPath + nFileOffset);
            if (FListAll(pOFI, hDlg, (LPTSTR)pOFI->szPath) == CHANGEDIR_FAILED)
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
        if ((pOFI->szPath[nExtOffset - 1] == CHAR_BSLASH) ||
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
                  (pOFI->szPath[nExtOffset - 2] == CHAR_BSLASH) ||
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
        TCHAR ch;
        BOOL bSlash;
        BOOL bRet;
        WORD nNullOffset;

        if (nFileOffset != PARSE_DIRECTORYNAME)
        {
            ch = *(pOFI->szPath + nFileOffset);
            *(pOFI->szPath + nFileOffset) = CHAR_NULL;
            nNullOffset = nFileOffset;
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
        //  BUG BUG
        //  Each wow thread can change the current directory.
        //  Since searchpath doesn't check current dirs on a perthread basis,
        //  reset it here and hope that we don't get interrupted between
        //  setting and searching...
        //
        SetCurrentDirectory(TlsGetValue(g_tlsiCurDir));

        bRet = SearchPath( pOFI->szPath,
                           TEXT("."),
                           NULL,
                           MAX_FULLPATHNAME,
                           szPathName,
                           NULL );
        if (!bRet && (pOFI->szPath[1] == CHAR_COLON))
        {
            INT nDriveIndex;
            DWORD err;

            nDriveIndex = DiskAddedPreviously(pOFI->szPath[0], NULL);

            //
            //  If it's a remembered connection try to reconnect it.
            //
            if (nDriveIndex != 0xFFFFFFFF  &&
                gaDiskInfo[nDriveIndex].dwType == REMDRVBMP)
            {
                if (lpfnWNetRestoreConn)
                {
                    err = (*lpfnWNetRestoreConn)(
                                 hDlg,
                                 gaDiskInfo[nDriveIndex].lpPath );

                    if (err == WN_SUCCESS)
                    {
                        gaDiskInfo[nDriveIndex].dwType = NETDRVBMP;
                        nDriveIndex = SendDlgItemMessage(
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
                        bRet = SearchPath( pOFI->szPath,
                                           TEXT("."),
                                           NULL,
                                           MAX_FULLPATHNAME,
                                           szPathName,
                                           NULL );
                    }
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
                ch = *(szPathName + lstrlen((LPTSTR)szPathName) - 1);
                if (ch != CHAR_BSLASH)
                {
                    lstrcat((LPTSTR)szPathName, TEXT("\\"));
                }
                lstrcat((LPTSTR)szPathName, (LPTSTR)(pOFI->szPath + nFileOffset));
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
                //  BUG BUG: Only Unicode
                //
                DWORD cch = SheGetPathOffsetW((LPWSTR)pOFI->szPath);

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
                                       (LPTSTR)pOFI->szPath,
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
                                           (LPTSTR)pOFI->szPath,
                                           FALSE,
                                           TRUE ) != CHANGEDIR_FAILED)
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
                lstrcpy((LPTSTR)szPathName, pOFI->szPath);
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
         ((mystrchr(pOFI->szPath + nFileOffset, CHAR_STAR)) ||
          (mystrchr(pOFI->szPath + nFileOffset, CHAR_QMARK))) )
    {
        TCHAR ch;
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

        if (FListAll(pOFI, hDlg, (LPTSTR)szSameDirFile) < 0)
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
                        SetDlgItemText(hDlg, edt1, (LPTSTR)szStarDotStar);
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
            INT nDriveIndex;

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
                nDriveIndex = DiskAddedPreviously(0, (LPTSTR)pOFI->szPath);
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
        lstrcpy((LPTSTR)szWarning, (LPTSTR)pOFI->szPath + 2);
        lstrcpy((LPTSTR)pOFI->szPath + 4, (LPTSTR)szWarning);
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
         ((DWORD)nExtOffset + 4 < pOFN->nMaxFile) &&
         ((DWORD)nExtOffset + 4 < 128) )
    {
        DWORD dwFileAttr;
        INT nExtOffset2 = lstrlen((LPTSTR)szPathName);

        bAddExt = TRUE;

        AppendExt((LPTSTR)pOFI->szPath, pOFI->pOFN->lpstrDefExt, FALSE);
        AppendExt((LPTSTR)szPathName, pOFI->pOFN->lpstrDefExt, FALSE);

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
                INT nRet;
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
                if (bSave && !FOkToWriteOver(hDlg, (LPTSTR)szPathName))
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
                AppendExt((LPTSTR)pOFI->szPath, pOFI->pOFN->lpstrDefExt, FALSE);
                AppendExt((LPTSTR)szPathName, pOFI->pOFN->lpstrDefExt, FALSE);
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
#ifdef UNICODE
                    if (pOFI->ApiType == COMDLG_ANSI)
                    {
                        CHAR szPathNameA[MAX_FULLPATHNAME];

                        RtlUnicodeToMultiByteSize(
                              &cch,
                              (LPTSTR)szPathName,
                              lstrlenW(szPathName) * sizeof(TCHAR) );

                        WideCharToMultiByte( CP_ACP,
                                             0,
                                             szPathName,
                                             -1,
                                             (LPSTR)&szPathName[0],
                                             cch + 1,
                                             NULL,
                                             NULL );

                        cch = (*pOFN->lpfnHook)( hDlg,
                                                 msgSHAREVIOLATIONA,
                                                 0,
                                                 (LONG)(LPSTR)szPathNameA );
                    }
                    else
#endif
                    {
                        cch = (*pOFN->lpfnHook)( hDlg,
                                                 msgSHAREVIOLATIONW,
                                                 0,
                                                 (LONG)(LPTSTR)szPathName );
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
            hFile = CreateFile( szPathName,
                                FILE_ADD_FILE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
                                NULL );
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

    lRet = ParseFile((LPTSTR)szPathName, blfn, pOFN->Flags & CD_WOWAPP);
    nFileOffset = (INT)(SHORT)LOWORD(lRet);
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

        if (_tcsicmp((LPTSTR)szPrivateExt, (LPTSTR)szPathName + cch))
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
        SheShortenPath(szPathName, TRUE);

        //
        //  If the path was shortened, the offset might have changed so
        //  we must parse the file again.
        //
        lRet = ParseFile((LPTSTR)szPathName, blfn, pOFN->Flags & CD_WOWAPP);
        nFileOffset = (INT)(SHORT)LOWORD(lRet);
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

            for (lptmp = (LPTSTR)szPathName + nFileOffset; *lptmp; lptmp++)
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

            if ((lstrlen((LPTSTR)szPathName + nFileOffset) > 8) ||
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


    pOFN = pOFI->pOFN;

    //
    //  Check for space for first full path element.
    //
    if (!(lpCurDir = (LPTSTR)TlsGetValue(g_tlsiCurDir)))
    {
        return (FALSE);
    }

    lstrcpy(pOFI->szPath, lpCurDir);

    if (!bCasePreserved)
    {
        CharLower(pOFI->szPath);
    }

    cch = ( lstrlen(pOFI->szPath) +
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
                            (INT)(pOFN->nMaxFile - cch - 1) );

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
                bRet = SearchPath( lpchStart,
                                   TEXT("."),
                                   NULL,
                                   MAX_FULLPATHNAME,
                                   szPathName,
                                   NULL );
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
#ifdef UNICODE
                            if (pOFI->ApiType == COMDLG_ANSI)
                            {
                                CHAR szPathNameA[MAX_FULLPATHNAME];

                                RtlUnicodeToMultiByteSize(
                                     &cch,
                                     (LPTSTR)szPathName,
                                     lstrlenW(szPathName) * sizeof(TCHAR) );

                                WideCharToMultiByte( CP_ACP,
                                                     0,
                                                     szPathName,
                                                     -1,
                                                     (LPSTR)&szPathName[0],
                                                     cch + 1,
                                                     NULL,
                                                     NULL );

                                cch = (*pOFN->lpfnHook)(
                                            hDlg,
                                            msgSHAREVIOLATIONA,
                                            0,
                                            (LONG)(LPSTR)szPathNameA );
                            }
                            else
#endif
                            {
                                cch = (*pOFN->lpfnHook)(
                                            hDlg,
                                            msgSHAREVIOLATIONW,
                                            0,
                                            (LONG)(LPTSTR)szPathName );
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
                        if (bSave && !FOkToWriteOver(hDlg, (LPTSTR)szPathName))
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

    pOFN->nFilterIndex = SendDlgItemMessage(hDlg, cmb1, CB_GETCURSEL, 0, 0L);

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

BOOL WINAPI dwOKSubclass(
    HWND hOK,
    UINT msg,
    WPARAM wP,
    LPARAM lP)
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
    return (CallWindowProc(lpOKProc, hOK, msg, wP, lP));
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

BOOL WINAPI dwLBSubclass(
    HWND hLB,
    UINT msg,
    WPARAM wP,
    LPARAM lP)
{
    HANDLE hDlg;
    POPENFILEINFO pOFI;

    if (msg == WM_KILLFOCUS)
    {
        hDlg = GetParent(hLB);
        bChangeDir = (GetDlgItem(hDlg, IDOK) == (HWND)wP) ? TRUE : FALSE;
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
    return (CallWindowProc(lpLBProc, hLB, msg, wP, lP));
}


////////////////////////////////////////////////////////////////////////////
//
//  InvalidFileWarning
//
////////////////////////////////////////////////////////////////////////////

INT InvalidFileWarning(
    HWND hDlg,
    LPTSTR szFile,
    DWORD wErrCode,
    UINT mbType)
{
    SHORT isz;
    BOOL bDriveLetter = FALSE;
    INT nRet = 0;

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
    if (!LoadString( g_hinst,
                     isz,
                     (LPTSTR)szCaption,
                     WARNINGMSGLENGTH ))
    {
        wsprintf( szWarning,
                  TEXT("Error occurred, but error resource cannot be loaded.") );
    }
    else
    {
        wsprintf( (LPTSTR)szWarning,
                  (LPTSTR)szCaption,
                  bDriveLetter ? (LPTSTR)(CHAR)*szFile : (LPTSTR)szFile );

DisplayError:
        GetWindowText(hDlg, (LPTSTR)szCaption, WARNINGMSGLENGTH);

        if (!mbType)
        {
            mbType = MB_OK | MB_ICONEXCLAMATION;
        }

        nRet = MessageBox(hDlg, (LPTSTR)szWarning, (LPTSTR)szCaption, mbType);
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

        hFont = (HANDLE)(DWORD)SendMessage(hDlg, WM_GETFONT, 0, 0L);
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

INT Signum(
    INT nTest)
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
    INT dxAcross;
    LONG nHeight;
    LONG rgbBack, rgbText, rgbOldBack, rgbOldText;
    SHORT nShift = 1;             // to shift directories right in lst2
    BOOL bSel;
    int BltItem;
    int nBackMode;

    if ((INT)lpdis->itemID < 0)
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
                            (INT)lpdis->CtlID,
                            LB_GETTEXT ,
                            (WPARAM)lpdis->itemID,
                            (LONG)(LPTSTR)szText );

        if (*szText == 0)
        {
            //
            //  If empty listing.
            //
            DefWindowProc(hDlg, WM_DRAWITEM, wParam, (LONG)lpdis);
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

        BltItem = SendMessage(hCmb2, CB_GETITEMDATA, lpdis->itemID, 0);

        SendMessage(hCmb2, CB_GETLBTEXT, lpdis->itemID, (LPARAM)(LPTSTR)szText);

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
                    (LPARAM)(LPTSTR)szText,
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
                  (LPTSTR)szText,
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
                    (LPTSTR)szText,
                    lstrlen((LPTSTR)szText),
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

    if (GetDlgItemText(hDlg, edt1, (LPTSTR)szText, MAX_FULLPATHNAME - 1))
    {
        DWORD lRet;

        lRet = ParseFile((LPTSTR)szText, IsLFNDriveX(hDlg, szText), bWowApp);
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
                nFileOffset = (WORD)ParseFile( (LPTSTR)szText,
                                               IsLFNDriveX(hDlg, szText),
                                               bWowApp );
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
            lstrcpy((LPTSTR)szText, (LPTSTR)(szText + nFileOffset));
        }
        if (nFileOffset)
        {
            SetDlgItemText(hDlg, edt1, (LPTSTR)szText);
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
    LPTSTR lpDelim)
{
    static LPTSTR lpString;
    LPTSTR lpRetVal, lpTemp;

    //
    //  If we are passed new string skip leading delimiters.
    //
    if (lpStr)
    {
        lpString = lpStr;

        while (*lpString && mystrchr(lpDelim, *lpString))
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
    while (*lpString && !mystrchr(lpDelim, *lpString))
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
    INT idStatic,
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

        while (*lpch && (*lpch++ != CHAR_BSLASH));
        fChop = TRUE;
    }

    ReleaseDC(hwndStatic, hdc);

    //
    //  If any characters chopped off, replace first three characters in
    //  remaining text string with ellipsis.
    //
    if (fChop)
    {
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
//            FALSE   if match.
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
    DWORD cchPathOffset;

    if (!(lpCurDir = (LPTSTR)TlsGetValue(g_tlsiCurDir)))
    {
        return (FALSE);
    }

    lpF = (LPTSTR)szPath;
    lstrcpy(lpF, lpCurDir);

    //
    //  Wow apps started from lfn dirs will set the current directory to an
    //  lfn, but only in the case where it is less than 8 chars.
    //
    if (pOFI->pOFN->Flags & OFN_NOLONGNAMES)
    {
        SheShortenPath(lpF, TRUE);
    }

    *lpF = (TCHAR)CharLower((LPTSTR)*lpF);
    cchPathOffset = SheGetPathOffsetW((LPWSTR)lpF);
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
        lstrcat(lpF, TEXT("\\"));

        wc = 0;
        *lpB++ = CHAR_NULL;
    }

    //
    //  Insert the items for the path to the current dir
    //  Insert the root...
    //
    pOFI->idirSub=0;

    SendMessage(hList, LB_INSERTSTRING, pOFI->idirSub++, (LPARAM)lpF);

    if (wc)
    {
        *lpB = wc;
    }

    for (lpF = lpB; *lpB; lpB++)
    {
        if ((*lpB == CHAR_BSLASH) || (*lpB == CHAR_SLASH))
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
//  FListAll
//
//  Given a file pattern, it changes the directory to that of the spec,
//  and updates the display.
//
////////////////////////////////////////////////////////////////////////////

INT FListAll(
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
    if (!(szPattern = mystrrchr( (LPTSTR)szSpec,
                                 (LPTSTR)szSpec + lstrlen(szSpec),
                                 CHAR_BSLASH )) &&
        !mystrchr(szSpec, CHAR_COLON))
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
        if (szPattern == mystrchr(szSpec, CHAR_BSLASH) )
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
                AppendExt((LPTSTR)szPattern, pOFI->pOFN->lpstrDefExt, TRUE);
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
            lstrcpy((LPTSTR)szDirBuf, (LPTSTR)szSpec);
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
//                2. g_tlsiCurThread
//                3. root of g_tlsiCurThread
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

INT ChangeDir(
    HWND hDlg,
    LPCTSTR lpszDir,
    BOOL bForce,
    BOOL bError)
{
    TCHAR szCurDir[CCHNETPATH];
    LPTSTR lpCurDir;
    DWORD cchDirLen;
    TCHAR wcDrive = 0;
    INT nIndex;
    BOOL nRet;


    //
    //  SheChangeDirEx will call GetCurrentDir, but will use what it
    //  gets only in the case where the path passed in was no good.
    //

    //
    //  1st, try request.
    //
    if (lpszDir && *lpszDir)
    {
        nRet = SheChangeDirEx((LPTSTR)lpszDir);

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
    //  2nd, try g_tlsiCurDir value (which we got above).
    //
    //  !!! need to check for a null return value ???
    //
    lpCurDir = (LPTSTR)TlsGetValue(g_tlsiCurDir);

    nRet = SheChangeDirEx(lpCurDir);

    if (nRet == ERROR_ACCESS_DENIED)
    {
        if (bError)
        {
            InvalidFileWarning( hDlg,
                                (LPTSTR)lpCurDir,
                                ERROR_DIR_ACCESS_DENIED,
                                0 );
        }
    }
    else
    {
        goto ChangeDir_OK;
    }

    //
    //  3rd, try root of g_tlsiCurDir or GetCurrentDir (sanity).
    //
    lstrcpy((LPTSTR)szCurDir, lpCurDir);
    cchDirLen = SheGetPathOffsetW((LPWSTR)szCurDir);

    //
    //  Sanity check - it's guaranteed not to fail ...
    //
    if (cchDirLen != 0xFFFFFFFF)
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
    lstrcpy((LPTSTR)szCurDir, TEXT("c:"));
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

    GetCurrentDirectory(CCHNETPATH, (LPTSTR)szCurDir);

    nIndex = DiskAddedPreviously(0, (LPTSTR)szCurDir);

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

        cchDirLen = SheGetPathOffsetW((LPWSTR)szCurDir);
        wc1 = szCurDir[cchDirLen];
        wc2 = szCurDir[cchDirLen + 1];

        szCurDir[cchDirLen] = CHAR_BSLASH;
        szCurDir[cchDirLen + 1] = CHAR_NULL;
        dwType = GetDiskIndex(GetDiskType((LPTSTR)szCurDir));
        szCurDir[cchDirLen] = CHAR_NULL;

        nIndex = AddDisk( wcDrive, lpszDisk, NULL, dwType);

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

        szCurDir[cchDirLen] = wc1;
        szCurDir[cchDirLen + 1] = wc2;
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
        if (!(lpCurDir = (LPTSTR)TlsGetValue(g_tlsiCurDir)))
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
            if ((cchDirLen = SheGetPathOffsetW((LPWSTR)szCurDir)) != 0xFFFFFFFF)
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
                LPDWORD lpCurThread;

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
                if (lpCurThread = (LPDWORD)TlsGetValue(g_tlsiCurThread))
                {
                    gahDlg[*lpCurThread] = hDlg;
                    FlushDiskInfoToCmb2();
                }
            }
        }

        lstrcpy(lpCurDir, (LPTSTR)&szCurDir[cchDirLen]);

        //
        //  If the the worker thread is running, then trying to select here
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

    lstrcpy((LPTSTR)szPath, lpszDisk);
    lstrcat((LPTSTR)szPath, TEXT("\\"));

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

    if (!szPath[0] || !szPath[1] ||
        (szPath[1] != CHAR_COLON && !(DBL_BSLASH(szPath))))
    {
        //
        //  If the path is not a full path then get the directory path
        //  from the TLS current directory.
        //
        lpCurDir = (LPTSTR)TlsGetValue(g_tlsiCurDir);
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
        INT i;
        LPTSTR p;

        //
        // Stop at "\\foo\bar"
        //
        for (i = 0, p = szRootPath + 2; *p && i < 2; p++)
        {
            if (CHAR_BSLASH == *p)
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

INT DiskAddedPreviously(
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
        cchDirLen = SheGetPathOffsetW((LPWSTR)lpszName);

        //
        //  If we're given a unc path, get the disk name.
        //  Otherwise, assume the whole thing is a disk name.
        //
        if (cchDirLen != 0xFFFFFFFF)
        {
            wc = *(lpszName + cchDirLen);
            *(lpszName + cchDirLen) = CHAR_NULL;
        }

        for (i = 0; i < dwNumDisks; i++)
        {
            if (!_tcsicmp(gaDiskInfo[i].lpName, lpszName))
            {
                *(lpszName + cchDirLen) = wc;
                return (i);
            }
        }

        *(lpszName + cchDirLen) = wc;
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

INT AddDisk(
    TCHAR wcDrive,
    LPTSTR lpName,
    LPTSTR lpProvider,
    DWORD dwType)
{
    INT nIndex, nRet;
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
        if (!_tcsicmp(lpName, gaDiskInfo[nIndex].lpName))
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
            if (lpfnWNetFormatNetName)
            {
                dwRet = (*lpfnWNetFormatNetName)( lpProvider,
                                                  lpName,
                                                  NULL,
                                                  &cchMultiLen,
                                                  WNFMT_MULTILINE,
                                                  dwAveCharPerLine );
            }
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

            //
            //  Get the length for the abbreviated name.
            //
            if (lpfnWNetFormatNetName)
            {
                dwRet = (*lpfnWNetFormatNetName)( lpProvider,
                                                  lpName,
                                                  NULL,
                                                  &cchAbbrLen,
                                                  WNFMT_ABBREVIATED,
                                                  dwAveCharPerLine );
            }
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

        if (lpfnWNetFormatNetName)
        {
            dwRet = (*lpfnWNetFormatNetName)( lpProvider,
                                              lpName,
                                              lpBuff,
                                              &cchAbbrLen,
                                              WNFMT_ABBREVIATED,
                                              dwAveCharPerLine );
        }
        if ((dwRet != WN_SUCCESS) || (!lpfnWNetFormatNetName))
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

        if (lpfnWNetFormatNetName)
        {
            dwRet = (*lpfnWNetFormatNetName)( lpProvider,
                                              lpName,
                                              lpBuff,
                                              &cchMultiLen,
                                              WNFMT_MULTILINE,
                                              dwAveCharPerLine );
        }
        if ((dwRet != WN_SUCCESS) || (!lpfnWNetFormatNetName))
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
                while(dwDisk--)
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
//  LoadMPR
//
////////////////////////////////////////////////////////////////////////////

VOID LoadMPR()
{
    if (!hMPR)
    {
        lpfnWNetConnDlg = NULL;
        lpfnWNetOpenEnum = NULL;
        lpfnWNetCloseEnum = NULL;
        lpfnWNetEnumResource = NULL;
        lpfnWNetRestoreConn = NULL;

        if (hMPR = LoadLibrary(szMPR))
        {
            lpfnWNetConnDlg = (LPFNWNETCONNDLG)GetProcAddress(hMPR, szWNetConnDlg);
            lpfnWNetOpenEnum = (LPFNWNETOPENENUM)GetProcAddress(hMPR, szWNetOpenEnum);
            lpfnWNetCloseEnum = (LPFNWNETCLOSEENUM)GetProcAddress(hMPR, szWNetCloseEnum);
            lpfnWNetEnumResource = (LPFNWNETENUMRESOURCE)GetProcAddress(hMPR, szWNetEnumResource);
            lpfnWNetRestoreConn = (LPFNWNETRESTORECONN)GetProcAddress(hMPR, szWNetRestoreConn);
        }
    }

    if (!hMPRUI)
    {
        lpfnWNetFormatNetName = NULL;

        if (hMPRUI = LoadLibrary(szMPRUI))
        {
            lpfnWNetFormatNetName = (LPFNWNETFORMATNETNAME)GetProcAddress(hMPRUI, szWNetFormatNetName);
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

    LoadMPR();
    if (!hMPR)
    {
        return (FALSE);
    }

    if (lpfnWNetConnDlg)
    {
        wRet = (*lpfnWNetConnDlg)((HWND)hWnd, (DWORD)WNTYPE_DRIVE);
    }
    else
    {
        wRet = WN_NOT_SUPPORTED;
    }

    if ((wRet != WN_SUCCESS) && (wRet != 0xFFFFFFFF))
    {
        if (!LoadString( g_hinst,
                         iszNoNetButtonResponse,
                         (LPTSTR)szCaption,
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
            wsprintf((LPTSTR)szWarning, (LPTSTR)szCaption);

            GetWindowText(hWnd, (LPTSTR) szCaption, WARNINGMSGLENGTH);
            MessageBox( hWnd,
                        (LPTSTR)szWarning,
                        (LPTSTR)szCaption,
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
        DWORD cch = CCHNETPATH;

        if (lpCurDir = (LPTSTR)TlsGetValue(g_tlsiCurDir))
        {
            lstrcpy((LPTSTR)szChangeSel, lpCurDir);
            SheGetDirExW(NULL, &cch, (LPWSTR)szChangeSel);

            if ((cch = SheGetPathOffsetW((LPWSTR)szChangeSel)) != 0xFFFFFFFF)
            {
                szChangeSel[cch] = CHAR_NULL;
            }

            SendMessage( hCmb,
                         CB_SETCURSEL,
                         (WPARAM)SendMessage( hCmb,
                                              CB_FINDSTRING,
                                              (WPARAM)-1,
                                              (LPARAM)(LPTSTR)szChangeSel ),
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
    LPDWORD lpCurThread = (LPDWORD)TlsGetValue(g_tlsiCurThread);

    if ( lpCurThread &&
         hLNDEvent &&
         !wNoRedraw &&
         hLNDThread &&
         bNetworkInstalled)
    {
        gahDlg[*lpCurThread] = hDlg;

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
    dwDriveType = GetDriveType((LPTSTR)szDrive);
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
            //  CharUpper((LPTSTR)szDrive);

            if (GetFileAttributes((LPTSTR)szDrive) != (DWORD)0xffffffff)
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
            INT nIndex;

            CharLower(szDrive);

            if (dwDriveType == DRIVE_REMOTE)
            {
                nIndex = AddDisk( szDrive[0],
                                  (LPTSTR)szVolLabel,
                                  NULL,
                                  TMPNETDRV );
            }
            else
            {
                nIndex = AddDisk( szDrive[0],
                                  (LPTSTR)szVolLabel,
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
    dwRet = (*lpfnWNetOpenEnum)( dwScope,
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
                (*lpfnWNetCloseEnum)(hEnum);
                return;
            }

            dwRet = (*lpfnWNetEnumResource)( hEnum,
                                             &dwCount,
                                             gpcNetEnumBuf,
                                             &cbSize );
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
                            INT nIndex;
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

        (*lpfnWNetCloseEnum)(hEnum);

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
#if 0
    DWORD dwNetRet;
#endif

    if (!hMPR || !hMPRUI)
    {
        LoadMPR();
    }

    if (!lpfnWNetOpenEnum || !lpfnWNetEnumResource || !lpfnWNetCloseEnum)
    {
        hLNDThread = NULL;
        return;
    }

#if 0
    //
    //  This is too slow (even in the worker thread) and cannot be used
    //  from prnsetup.c since prnsetup.c doesn't load mpr.  Rather than
    //  have prnsetup.c load mpr and take the performance penalty, use
    //  IsNetworkInstalled() routine in dlgs.c for both Fileopen (LoadDrives)
    //  and prnsetup.c.
    //
    dwNetRet = (*lpfnWNetOpenEnum)( RESOURCE_GLOBALNET,
                                    RESOURCETYPE_DISK,
                                    0,
                                    NULL,
                                    &hEnum );
    //
    //  If there is a netcard installed but the service isn't started,
    //  this will return 0.
    //
    if (dwNetRet == ERROR_NO_NETWORK)
    {
        bNetworkInstalled = FALSE;
        HideNetButton();
        hLNDThread = NULL;
        return;
    }
#endif

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
            EnableDiskInfo(FALSE, FALSE);

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
    LPDWORD lpCurThread;
    DWORD dwThreadID;
    BOOL bFirstAttach = FALSE;
    WORD wCurDrive;
    TCHAR szDrive[5];

    if (!hLNDEvent)
    {
        //
        //  Don't check if this succeeds since we can run without the net.
        //
        hLNDEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

        bFirstAttach = TRUE;
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
    lpCurThread = (LPDWORD)TlsGetValue(g_tlsiCurThread);

    // sanity check
    if (!lpCurThread)
    {
        return;
    }

    gahDlg[*lpCurThread] = hDlg;

    //
    //  If there is no worker thread for network disk enumeration,
    //  start up here rather than in the dll, since it's only
    //  for the fileopen dlg.
    //
    //  Always start a thread if the number of active fileopen dialogs
    //  goes from 0 to 1
    //
    if ((*lpCurThread == 0) && (!hLNDThread))
    {
        if (hLNDEvent && (bNetworkInstalled = IsNetworkInstalled()))
        {
            //
            //  Do this once when dialog thread count goes from 0 to 1.
            //
            if (LoadLibrary(TEXT("comdlg32.dll")))
            {
                hLNDThread = CreateThread(
                                   NULL,
                                   (DWORD)0,
                                   (LPTHREAD_START_ROUTINE)ListNetDrivesHandler,
                                   (LPVOID)NULL,
                                   (DWORD)NULL,
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

    FlushDiskInfoToCmb2();

    //
    //  Now invalidate all net conns and re-enum, but only if there is
    //  indeed a worker thread too.
    //
    if (!bFirstAttach)
    {
        EnableDiskInfo(FALSE, FALSE);
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
    LPDWORD lpCurThread;

    if (lpCurThread = (LPDWORD)TlsGetValue(g_tlsiCurThread))
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

    if (hMPRUI)
    {
        FreeLibrary(hMPRUI);
        hMPRUI = NULL;
    }

    if (hMPR)
    {
        FreeLibrary(hMPR);
        hMPR = NULL;
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
        DWORD cbLen = lstrlenA(pOFNA->lpstrDefExt) + 1;

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
                MultiByteToWideChar( CP_ACP,
                                     0,
                                     pOFNA->lpstrDefExt,
                                     -1,
                                     (LPWSTR)pOFNW->lpstrDefExt,
                                     cbLen );
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
    INT nRet;

    LPOPENFILENAMEA pOFNA = pOFI->pOFNA;
    LPOPENFILENAMEW pOFNW = pOFI->pOFN;

    pOFNW->Flags = pOFNA->Flags;
    pOFNW->lCustData = pOFNA->lCustData;

    if (pOFNW->lpstrFile)
    {
        if (pOFNA->lpstrFile)
        {
            nRet = MultiByteToWideChar( CP_ACP,
                                        0,
                                        pOFNA->lpstrFile,
                                        -1,
                                        pOFNW->lpstrFile,
                                        pOFNW->nMaxFile );
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
            nRet = MultiByteToWideChar( CP_ACP,
                                        0,
                                        pOFNA->lpstrFileTitle,
                                        pOFNA->nMaxFileTitle,
                                        pOFNW->lpstrFileTitle,
                                        pOFNW->nMaxFileTitle );
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

                nRet = MultiByteToWideChar( CP_ACP,
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
    INT nRet;

    LPOPENFILENAMEW pOFNW = pOFI->pOFN;
    LPOPENFILENAMEA pOFNA = pOFI->pOFNA;
    LPWSTR pszW;
    USHORT cch;

    //
    //  Supposedly invariant, but not necessarily.
    //
    pOFNA->Flags = pOFNW->Flags;
    pOFNA->lCustData = pOFNW->lCustData;

    if (pOFNA->lpstrFile)
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
            nRet = WideCharToMultiByte( CP_ACP,
                                        0,
                                        pOFNW->lpstrFile,
                                        -1,
                                        pOFNA->lpstrFile,
                                        pOFNA->nMaxFile,
                                        NULL,
                                        NULL );
            if (nRet == 0)
            {
                return (FALSE);
            }

            //
            //  See if we are dealing with the new dialogs.  If so, each
            //  filename for multiselect is separated by a null terminator
            //  and the list is terminated by 2 null terminators.
            //
            if ((pOFI->bUseNewDialog) && (pOFNW->Flags & OFN_ALLOWMULTISELECT))
            {
                LPWSTR pFileW = pOFNW->lpstrFile + lstrlen(pOFNW->lpstrFile) + 1;
                LPSTR  pFileA = pOFNA->lpstrFile + nRet;
                DWORD  nMaxFile = pOFNA->nMaxFile - nRet;

                while (*pFileW && nMaxFile)
                {
                    nRet = WideCharToMultiByte( CP_ACP,
                                                0,
                                                pFileW,
                                                -1,
                                                pFileA,
                                                nMaxFile,
                                                NULL,
                                                NULL );
                    if (nRet == 0)
                    {
                        return (FALSE);
                    }

                    pFileW = pFileW + lstrlen(pFileW) + 1;
                    pFileA = pFileA + nRet;
                    nMaxFile = nMaxFile - nRet;
                }

                //
                //  Double null terminate the list.
                //
                if (nMaxFile)
                {
                    *pFileA = 0;
                }
            }
        }
    }

    if (pOFNA->lpstrFileTitle && pOFNA->nMaxFileTitle)
    {
        nRet = WideCharToMultiByte( CP_ACP,
                                    0,
                                    pOFNW->lpstrFileTitle,
                                    -1,
                                    pOFNA->lpstrFileTitle,
                                    pOFNA->nMaxFileTitle,
                                    NULL,
                                    NULL );
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
            nRet = WideCharToMultiByte( CP_ACP,
                                        0,
                                        pOFI->pusCustomFilter->Buffer,
                                        pOFI->pusCustomFilter->Length,
                                        pOFI->pasCustomFilter->Buffer,
                                        pOFI->pasCustomFilter->MaximumLength,
                                        NULL,
                                        NULL );
            if (nRet == 0)
            {
                return (FALSE);
            }
        }
    }

    pOFNA->nFileOffset = pOFNW->nFileOffset;
    pOFNA->nFileExtension = pOFNW->nFileExtension;
    pOFNA->nFilterIndex = pOFNW->nFilterIndex;

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GenericGetFileNameA
//
////////////////////////////////////////////////////////////////////////////

BOOL GenericGetFileNameA(
    LPOPENFILENAMEA pOFNA,
    WNDPROC qfnDlgProc)
{
    LPOPENFILENAMEW pOFNW;
    BOOL bRet = FALSE;
    OFN_UNICODE_STRING usCustomFilter;
    OFN_ANSI_STRING asCustomFilter;
    DWORD cbLen;
    LPSTR pszA;
    DWORD cch;
    LPBYTE pStrMem = NULL;
    OPENFILEINFO OFI;

    ZeroMemory(&OFI, sizeof(OPENFILEINFO));

    if (!pOFNA)
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    if (pOFNA->lStructSize != sizeof(OPENFILENAMEA))
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

    //
    //  Init TemplateName constant.
    //
    if (pOFNA->Flags & OFN_ENABLETEMPLATE)
    {
        if (HIWORD(pOFNA->lpTemplateName))
        {
            cbLen = lstrlenA(pOFNA->lpTemplateName) + 1;
            if (!(pOFNW->lpTemplateName = (LPWSTR)LocalAlloc(LPTR, (cbLen * sizeof(WCHAR)))))
            {
                StoreExtendedError(CDERR_MEMALLOCFAILURE);
                goto GenericExit;
            }
            else
            {
                MultiByteToWideChar( CP_ACP,
                                     0,
                                     pOFNA->lpTemplateName,
                                     -1,
                                     (LPWSTR)pOFNW->lpTemplateName,
                                     cbLen );
            }
        }
        else
        {
            (DWORD)pOFNW->lpTemplateName = (DWORD)pOFNA->lpTemplateName;
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
            MultiByteToWideChar( CP_ACP,
                                 0,
                                 pOFNA->lpstrInitialDir,
                                 -1,
                                 (LPWSTR)pOFNW->lpstrInitialDir,
                                 cbLen );
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
            MultiByteToWideChar( CP_ACP,
                                 0,
                                 pOFNA->lpstrTitle,
                                 -1,
                                 (LPWSTR)pOFNW->lpstrTitle,
                                 cbLen );
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
        cbLen = lstrlenA(pOFNA->lpstrDefExt) + 1;
        if (!(pOFNW->lpstrDefExt = (LPWSTR)LocalAlloc(LPTR, (cbLen * sizeof(WCHAR)))))
        {
            StoreExtendedError(CDERR_MEMALLOCFAILURE);
            goto GenericExit;
        }
        else
        {
            MultiByteToWideChar( CP_ACP,
                                 0,
                                 pOFNA->lpstrDefExt,
                                 -1,
                                 (LPWSTR)pOFNW->lpstrDefExt,
                                 cbLen );
        }
    }
    else
    {
        pOFNW->lpstrDefExt = NULL;
    }

    //
    //  Initialize Filter constant.
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
            MultiByteToWideChar( CP_ACP,
                                 0,
                                 pOFNA->lpstrFilter,
                                 cch,
                                 (LPWSTR)pOFNW->lpstrFilter,
                                 cch );
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

    bRet = GetFileName(&OFI, (WNDPROC)qfnDlgProc);

    ThunkOpenFileNameW2A(&OFI);

GenericExit:

    if (pStrMem)
    {
        LocalFree(pStrMem);
    }

    if (HIWORD(pOFNW->lpstrFile))
    {
        LocalFree((HLOCAL)pOFNW->lpstrFile);
    }

    if (HIWORD(pOFNW->lpstrFileTitle))
    {
        LocalFree((HLOCAL)pOFNW->lpstrFileTitle);
    }

    if (HIWORD(pOFNW->lpstrFilter))
    {
        LocalFree((HLOCAL)pOFNW->lpstrFilter);
    }

    if (HIWORD(pOFNW->lpstrDefExt))
    {
        LocalFree((HLOCAL)pOFNW->lpstrDefExt);
    }

    if (HIWORD(pOFNW->lpstrTitle))
    {
        LocalFree((HLOCAL)pOFNW->lpstrTitle);
    }

    if (HIWORD(pOFNW->lpstrInitialDir))
    {
        LocalFree((HLOCAL)pOFNW->lpstrInitialDir);
    }

    if (HIWORD(pOFNW->lpTemplateName))
    {
        LocalFree((HLOCAL)pOFNW->lpTemplateName);
    }

    LocalFree(pOFNW);

    return (bRet);
}

#endif
