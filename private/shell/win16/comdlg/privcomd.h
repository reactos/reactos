/*++

Copyright (c) 1990-1995,  Microsoft Corporation  All rights reserved.

Module Name:

    privcomd.h

Abstract:

    This module contains the private header information for the Win32
    common dialogs.

Revision History:

--*/



#ifndef COMDLG_PRIVCOMD
#define COMDLG_PRIVCOMD

#ifdef __cplusplus
extern "C" {
#endif




//
//  Include Files.
//

#include "commdlg.h"
#include <wowcmndg.h>
#include "dlgs.h"
#include "isz.h"
#include "cderr.h"
#include <winuserp.h>




//
//  Constant Declarations.
//

#define SEM_NOERROR               0x8003

//
//  There really should be no "max" path lengths, but for now,
//  since unc will take up RMLEN (lmcons.h), make it 98.
//
#define CCHUNCPATH                98
#define CCHNETPATH                358

#define MAX_THREADS               128

#define CHAR_A                    TEXT('a')
#define CHAR_CAP_A                TEXT('A')
#define CHAR_C                    TEXT('c')
#define CHAR_Z                    TEXT('z')
#define CHAR_NULL                 TEXT('\0')
#define CHAR_COLON                TEXT(':')
#define CHAR_BSLASH               TEXT('\\')
#define CHAR_DOT                  TEXT('.')
#define CHAR_QMARK                TEXT('?')
#define CHAR_STAR                 TEXT('*')
#define CHAR_SLASH                TEXT('/')
#define CHAR_SPACE                TEXT(' ')
#define CHAR_QUOTE                TEXT('"')
#define CHAR_PLUS                 TEXT('+')
#define CHAR_LTHAN                TEXT('<')
#define CHAR_BAR                  TEXT('|')
#define CHAR_COMMA                TEXT(',')
#define CHAR_LBRACKET             TEXT('[')
#define CHAR_RBRACKET             TEXT(']')
#define CHAR_EQUAL                TEXT('=')
#define CHAR_SEMICOLON            TEXT(';')

#define STR_BLANK                 TEXT("")
#define STR_SEMICOLON             TEXT(";")

#define IS_DOTEND(ch)   ((ch) == CHAR_DOT || (ch) == 0 || ((ch) != CHAR_STAR))

#define PARSE_DIRECTORYNAME       -1
#define PARSE_INVALIDDRIVE        -2
#define PARSE_INVALIDPERIOD       -3
#define PARSE_MISSINGDIRCHAR      -4
#define PARSE_INVALIDCHAR         -5
#define PARSE_INVALIDDIRCHAR      -6
#define PARSE_INVALIDSPACE        -7
#define PARSE_EXTENSIONTOOLONG    -8
#define PARSE_FILETOOLONG         -9
#define PARSE_EMPTYSTRING         -10
#define PARSE_WILDCARDINDIR       -11
#define PARSE_WILDCARDINFILE      -12
#define PARSE_INVALIDNETPATH      -13
#define PARSE_NOXMEMORY           -14

#define OF_FILENOTFOUND           2
#define OF_PATHNOTFOUND           3
#define OF_NOFILEHANDLES          4
#define OF_ACCESSDENIED           5         // OF_NODISKINFLOPPY
#define OF_WRITEPROTECTION        19
#define OF_SHARINGVIOLATION       32
#define OF_NETACCESSDENIED        65
#define OF_DISKFULL               82
#define OF_INT24FAILURE           83
#define OF_CREATENOMODIFY         96
#define OF_NODRIVE                97
#define OF_PORTNAME               98
#define OF_LAZYREADONLY           99

#ifndef DCE_UNICODIZED
  #define DeviceCapabilitiesExA DeviceCapabilitiesEx
#endif

//
//  Used to determine which type of message to send to the app.
//
#define COMDLG_ANSI               0x0
#define COMDLG_WIDE               0x1

#define HNULL     ((HANDLE) 0)

#define cbResNameMax    32
#define cbDlgNameMax    32




//
//  Typedef Declarations.
//




//
//  External Declarations.
//

extern HINSTANCE g_hinst;              // instance handle of library

extern SHORT cyCaption, cyBorder, cyVScroll;
extern SHORT cxVScroll, cxBorder, cxSize;

extern TCHAR szNull[];
extern TCHAR szStar[];
extern TCHAR szStarDotStar[];

extern BOOL bMouse;                    // system has a mouse
extern BOOL bCursorLock;
extern BOOL bWLO;                      // running with WLO
extern BOOL bDBCS;                     // running DBCS
extern WORD wWinVer;                   // windows version
extern WORD wDOSVer;                   // DOS version
extern BOOL bUserPressedCancel;        // user pressed cancel button

//
//  initialized via RegisterWindowMessage
//
extern UINT msgWOWLFCHANGE;
extern UINT msgWOWDIRCHANGE;
extern UINT msgWOWCHOOSEFONT_GETLOGFONT;

extern UINT msgLBCHANGEA;
extern UINT msgSHAREVIOLATIONA;
extern UINT msgFILEOKA;
extern UINT msgCOLOROKA;
extern UINT msgSETRGBA;
extern UINT msgHELPA;

extern UINT msgLBCHANGEW;
extern UINT msgSHAREVIOLATIONW;
extern UINT msgFILEOKW;
extern UINT msgCOLOROKW;
extern UINT msgSETRGBW;
extern UINT msgHELPW;

extern UINT g_cfCIDA;




//
//  Function Prototypes.
//

VOID TermFind(void);
VOID TermColor(void);
VOID TermFont(void);
VOID TermFile(void);
VOID TermPrint(void);

void FreeImports(void);

//
//  dlgs.c
//
VOID
HourGlass(
    BOOL bOn);

void
StoreExtendedError(
    DWORD dwError);

DWORD
GetStoredExtendedError(void);

HBITMAP WINAPI
LoadAlterBitmap(
    INT id,
    DWORD rgbReplace,
    DWORD rgbInstead);

VOID
AddNetButton(
    HWND hDlg,
    HANDLE hInstance,
    INT dyBottomMargin,
    BOOL bAddAccel,
    BOOL bTryLowerRight,
    BOOL bTryLowerLeft);

BOOL
IsNetworkInstalled(void);

//
//  parse.c
//
int
ParseFileNew(
    LPTSTR pszPath,
    int *pnExtOffset,
    BOOL bWowApp);

int
ParseFileOld(
    LPTSTR pszPath,
    int *pnExtOffset,
    int *pnOldExt,
    BOOL bWowApp);

DWORD
ParseFile(
    LPTSTR lpstrFileName,
    BOOL bLFNFileSystem,
    BOOL bWowApp);

LPTSTR
PathRemoveBslash(
    LPTSTR lpszPath);

BOOL
IsWild(
    LPTSTR lpsz);

LPTSTR
mystrchr(
    LPCTSTR str,
    TCHAR ch);

LPTSTR
mystrrchr(
    LPCTSTR lpStr,
    LPCTSTR lpEnd,
    TCHAR ch);

VOID
AppendExt(
    LPTSTR lpszPath,
    LPCTSTR lpExtension,
    BOOL bWildcard);

BOOL
IsUNC(
    LPCTSTR lpszPath);

BOOL
PortName(
    LPTSTR lpszFileName);

BOOL
IsDirectory(
    LPTSTR pszPath);

int
WriteProtectedDirCheck(
    LPTSTR lpszFile);

BOOL
FOkToWriteOver(
    HWND hDlg,
    LPTSTR szFileName);

INT
CreateFileDlg(
    HWND hDlg,
    LPTSTR szPath);





//
//  Fileopen specific stuff stashed here so we can free it upon
//  a DLL_PROCESS_DETACH.
//
typedef struct _OFN_DISKINFO {
    UINT   cchLen;           // number of chars allocated in 4 lptstrs
    LPTSTR lpAbbrName;       // single line form
    LPTSTR lpMultiName;      // drop-down form
    LPTSTR lpName;           // true form (for comparisons)
    LPTSTR lpPath;           // path prefix (a:, or \\server\share) for file searches
    TCHAR  wcDrive;          // drive letter, 0 for unc
    BOOL   bCasePreserved;
    DWORD  dwType;
    BOOL   bValid;
} OFN_DISKINFO;

#define MAX_DISKS                 100
#define WNETENUM_BUFFSIZE         0x4000

//
//  Defines for AddNetButton.
//
#define FILE_LEFT_MARGIN          5
#define FILE_RIGHT_MARGIN         3
#define FILE_TOP_MARGIN           0
#define FILE_BOTTOM_MARGIN        3

#ifdef __cplusplus
};  // extern "C"
#endif

#endif

