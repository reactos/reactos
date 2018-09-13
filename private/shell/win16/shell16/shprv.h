/*
 *  shprv.h
 *
 *  Header file for shell association database management functions
 */

#ifndef RC_INVOKED
#pragma warning(disable:4001)
#endif

// #define BUILDDLL



#define UNALIGNED
#include <windows.h>            // This is the 16-bit version of windows.h

typedef const short far * LPCWSTR ;
typedef short far * LPWSTR ;
typedef char CHAR;
typedef char far * LPTSTR;
typedef const char far * LPCTSTR;
typedef char TCHAR;

#ifdef WINNT
#define OemToChar
#define CharToOem
#else
#define OemToChar OemToAnsi
#define CharToOem AnsiToOem
#endif

#include <dde.h>
#include <ccstock.h>
#include <shellapi.h>
#include <commctrl.h>   // make sure we don't conflict
#include <prsht.h>

#include "shelldlg.h"

// DEBUG stuff
#undef DebugMsg
#ifdef DEBUG
#define DebugMsg(x, sz) OutputDebugString(sz "\r\n")
#else
#define DebugMsg(x, sz)
#endif

#define SHGetFileInfo SHGetFileInfoA

// in MSGBOX.C
int WINAPI ShellMessageBox(HINSTANCE hAppInst, HWND hWnd, LPCSTR lpcText, LPCSTR lpcTitle, UINT fuStyle);

/* These are aligned on a ID which is a multiple of 16 (this will save a bit of memmory)
 */
#define IDS_PROGFOUND         68
#define IDS_PROGNOTFOUND      69
#define IDS_NOCOMMDLG         70
#define IDS_STILLNOTFOUND     71

LPSTR FAR PASCAL GAlloc(WORD,LPCATCHBUF);
LPSTR FAR PASCAL GFree(LPSTR);
LPSTR FAR PASCAL GRealloc(LPSTR,WORD,LPCATCHBUF);
BOOL  FAR PASCAL IsStringInList(LPSTR,LPSTR);
int   FAR PASCAL OpenSharedFile(LPCSTR szName, LPOFSTRUCT pof, UINT wFlags);

#define SE_ERR_FNF                      2       // ShellExec() error returns
#define SE_ERR_PNF                      3
#define SE_ERR_OOM                      8

#ifndef DBCS
#define AnsiNext(x) ((x)+1)
#define AnsiPrev(y,x) ((x)-1)
#define IsDBCSLeadByte(x) ((x),FALSE)
#endif

#define CBPATHMAX   256
#define CBFILENAME  256
#define CBCOMMAND   256

//
// For shared code
//
#define _SHELLPRV_H_
#define _IDS_H_

extern int g_cxSmIcon, g_cySmIcon;
extern int g_cxIcon, g_cyIcon;
extern HINSTANCE g_hinst;
extern char const FAR c_szNull[];
#define HINST_THISDLL   g_hinst

#define IDD_TEXT                0x3003
#define IDD_SHUTDOWN                0x3800

#define IDI_SHUTDOWN            26

#define IDS_REASONS             0x1800
#define IDS_REASONS_INSMEM      (IDS_REASONS + DE_INSMEM)
#define IDS_CLOSE               0x1041

#define IDS_EXITHELP            0x2140
#define IDS_WINDOWS_HLP         0x2141

#define DLG_FORMAT_FIRST	400
#define IDS_FORMAT_FIRST	0x2800

LPSTR   WINAPI  ResourceCStrToStr(HINSTANCE hAppInst, LPCSTR lpcText);
void FAR PASCAL PathRemoveArgs(LPSTR pszPath);
BOOL 	WINAPI 	SetWaitCursor(BOOL bSet);
#define DATASEG_READONLY        "_TEXT"
