/***************************************************************************
 * globals.h - TrueType Font Manager global data declaration.
 *
 * $keywords: globals.h 1.3 17-Mar-94 2:38:47 PM$
 *
 * Copyright (C) 1992-93 ElseWare Corporation.  All rights reserved.
 ***************************************************************************/

#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#define ERR_FAIL 0
#define NOERR 1

#include <fstream.h>

//
//  Forward declarations.
//
class CFontFolder;

//
//  Constants
//

#define MAX_NAME_LEN            64
#define MAX_LOG_NAME_LEN        32
#define MAX_PATH_LEN            MAX_PATH
#define MAX_FILE_LEN            MAX_PATH_LEN
// #define MAX_DIRS                32
#define IDX_NULL                0xFFFF      // index out of range
#define IDX_ZERO                0x0000
#define PANOSE_LEN              10
#define COPYRIGHT_LEN           60

//
//  Defines
//

#define PATHMAX MAX_PATH   // path length max - used for Get...Directory() calls
#define DESCMAX 129        // max description in newexe header
#define MODNAMEMAX 20      // max module name in newexe header

//
//  Name/string types
//

typedef TCHAR   FullPathName_t[ PATHMAX ];
typedef TCHAR   FontDesc_t[ DESCMAX ];
typedef TCHAR   ModName_t[ MODNAMEMAX ];

typedef TCHAR   LOGNAME[ MAX_LOG_NAME_LEN + 1 ]; // log font name
typedef TCHAR   FAMNAME[ MAX_LOG_NAME_LEN + 1 ]; // font Family name
typedef TCHAR   FONTNAME[ MAX_NAME_LEN + 1 ];
typedef TCHAR   PATHNAME[ MAX_PATH_LEN + 1 ];
typedef TCHAR   FILENAME[ MAX_FILE_LEN + 1 ];


//
//  Globals
//

extern BOOL      g_bTrace;
extern BOOL      g_bDiag;
extern BOOL      g_bTimer;

extern HINSTANCE g_hInst;

extern TCHAR     g_szType1Key[];

extern char      g_szFontsDirA[];

extern TCHAR     c_szTrueType[];
extern TCHAR     c_szOpenType[];
extern TCHAR     c_szPostScript[];
extern TCHAR     c_szDescFormat[];
extern TCHAR     szNull[];

extern FullPathName_t   s_szSharedDir;


//
// Far East character handling.
//
extern BOOL g_bDBCS;

//
//  Number of references to objects in this dll
//

extern LONG      g_cRefThisDll;

extern CFontFolder * g_poFontFolder;

//
//  Types
//

typedef short  RC;
typedef WORD   SFIDX;      /* display sorted index into font list */
typedef WORD   FTIDX;
typedef WORD   POINTSIZE;     /* Size of point measurement */

// EMR: This needs to be a WORD so it fits in the ListITEMDATA struct.
//typedef int         FONTNUM;    /* Font number */
typedef WORD         FONTNUM;    /* Font number */
typedef FONTNUM*     PFONTNUM;   /* Font number */
typedef FONTNUM FAR* LPFONTNUM;  /* Font number */

//
//  typedefs.
//

//
//  Font description info. Used during installation and to retrieve info
//  about a font.
//
//  szFile must is filled in by calling proc. The rest is filled in depending
//       on the flags.
//

#define  FDI_DESC    1
#define  FDI_FAMILY  2
#define  FDI_PANOSE  4
#define  FDI_STYLE   8
#define  FDI_ALL     (DWORD)(15)
#define  FDI_NONE    0

#define  FDI_VTC     16    // Version Trademark and copyright.

//
// Style bits. These can be OR'd together
//

#define  FDI_S_REGULAR  0
#define  FDI_S_BOLD     1
#define  FDI_S_ITALIC   2


typedef struct {
   DWORD          dwFlags;
   FullPathName_t szFile;
   FontDesc_t     szDesc;
   FAMNAME        szFamily;
   DWORD          dwStyle;
   WORD           wWeight;
   TCHAR          jPanose[ PANOSE_LEN ];

   //
   // These strings are allocated by the routine that actually retrieves
   // the strings. They should be deleted using the C++ delete operator.
   //

   TCHAR *  lpszVersion;
   TCHAR *  lpszTrademark;
   TCHAR *  lpszCopyright;
   
} FONTDESCINFO, FAR * LPFONTDESCINFO;



//
//  Enums
//

typedef enum {
   eFKNone     = 0,
   eFKDevice,
   eFKTrueType,
   eFKOpenType,
   eFKTTC,
   eFKType1
} eFileKind;

typedef enum _tagMPVIEW {
   eVFont    = 0,
   eVFamily,
   eVFile,
   eVPanose
} MPVIEW;


extern BOOL  NEAR PASCAL bIsTrueType( LPFONTDESCINFO lpFile, LPDWORD pdwTableTags = NULL, LPDWORD lpdwStatus = NULL );
extern BOOL  NEAR PASCAL bIsNewExe( LPFONTDESCINFO lpFile );

extern int GetFontsDirectory( LPTSTR lpDir, int nSize);
extern BOOL WriteToRegistry( LPTSTR lpDesc, LPTSTR lpFile );

extern int  FAR PASCAL iUIErrMemDlg(HWND hwndParent);   // Everyone needs this
extern int  FAR PASCAL iUIMsgBox( HWND hwndParent, 
                                  WORD wIDStr,
                                  WORD wCAPStr,
                                  UINT uiMBFlags,
                                  LPCTSTR wArg1 = 0,
                                  LPCTSTR wArg2 = 0,
                                  LPCTSTR wArg3 = 0,
                                  LPCTSTR wArg4 = 0);
//
//  These are 3 simple covers which make calling the message routine
//  much simpler.
//

int FAR PASCAL iUIMsgBoxWithCaption(HWND hwndParent, WORD wIDStr, WORD wCaption);
int FAR PASCAL iUIMsgYesNoExclaim(HWND hwndParent, WORD wIDStr, LPCTSTR wArg = 0);
int FAR PASCAL iUIMsgYesNoExclaim(HWND hwndParent, WORD wIDStr, WORD wCap, LPCTSTR wArg=0);
int FAR PASCAL iUIMsgOkCancelExclaim(HWND hwndParent, WORD wIDStr, WORD wCap, LPCTSTR wArg=0);
int FAR PASCAL iUIMsgRetryCancelExclaim(HWND hwndParent, WORD wIDStr, LPCTSTR wArg=0 );
int FAR PASCAL iUIMsgExclaim(HWND hwndParent, WORD wIDStr, LPCTSTR wArg = 0);
int FAR PASCAL iUIMsgBox(HWND hwndParent, WORD wIDStr, LPCTSTR wArg = 0);
int FAR PASCAL iUIMsgInfo(HWND hwndParent, WORD wIDStr, LPCTSTR wArg = 0);

//
// Special-purpose message box for reporting font validation failures.
// dwStatusCode must be one of FVS_XXXXXXXX values as defined in fvscodes.h
// 
int iUIMsgBoxInvalidFont(HWND hwndParent, LPCTSTR pszFontFile, LPCTSTR pszFontDesc,
                         DWORD dwStatusCode,
                         UINT uStyle = (MB_OKCANCEL | MB_ICONEXCLAMATION));

#include "fvscodes.h"  // \nt\private\windows\shell\control\t1instal\fvscodes.h
                       // Contains FVS_xxxxx codes and related macros.


#endif /* __GLOBALS_H_ */

/****************************************************************************
 * $lgb$
 * 1.0     7-Mar-94   eric Initial revision.
 * 1.1     9-Mar-94   eric Background thread and g_hDBMutex
 * 1.2     9-Mar-94   eric Added Gdi mutex.
 * 1.3    17-Mar-94   eric Removed mutex handles.
 * $lge$
 *
 ****************************************************************************/


