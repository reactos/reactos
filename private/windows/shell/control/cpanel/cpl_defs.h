/*  CPL_DEFS.H
**
**  Copyright Microsoft Corporation 1990, All Rights Reserved
**
**  Multimedia Control Panel private defintions.
**
**  Modifications:
**
**  03/05/90    MichaelE    Created.
**  04/09/91    JohnYG      Added default sizing.
**
**  04 Feb 1992  -by-  Steve Cathcart   [stevecat]
**        Updated code to latest Win 3.1 sources
**
**  21 Sept 1993  -by-  Steve Cathcart   [stevecat]
**        Added Caching of Control Panel modules
**
**  Copyright (C) 1990-1993 Microsoft Corporation
**
*/

//==========================================================================
//                     Global Definitions
//==========================================================================

#define NUMCPLSTRINGS       4

#ifdef UNICODE
typedef TCHAR          TUCHAR;
#else
typedef unsigned char  TUCHAR;
#endif

#define INITS               0x400
#define CONTROLABOUT        0x410

#define MMCPL               1001

#define CPL_MAXMODS         64
#define CPL_MAXAPPS         128

#define CPL_DEFDIMX         98    // Default Dimensions for the control panel
#define CPL_DEFDIMY         101   // Any changes to SETUP.INF should be
#define CPL_DEFDIMW         442   // reflected here
#define CPL_DEFDIMH         230

#define MENU_HELP           30
#define MENU_SCHHELP        33
#define MENU_INDHELP        40
#define MENU_USEHELP        41
#define MENU_ABOUT          50
#define MENU_EXIT           51
#define MENU_F1             52
#define MENU_CACHE          53
#define MENU_SETTINGS      100

#ifndef IDH_SYSMENU
#define IDH_SYSMENU        1000
#endif
#ifndef IDH_HELPFIRST
#define IDH_HELPFIRST      1001
#endif

#define ID_LISTBOX          20
#define ID_INFOBOX          21

#define WM_CPL_EXIT         0x1000  // send CPL_EXIT msgs to all modules

typedef struct tagCPLAPPLET
{
    HICON   hIcon;              // handle of icon
    LPTSTR  pszName;            // ptr to name string
    LPTSTR  pszFullName;        // ptr to orig name string with '&' char
    LPTSTR  pszInfo;            // ptr to info string
    int     iNameW;             // width of name str in pixels
    LONG    lData;              // user supplied data
    DWORD   dwContext;
    LPTSTR  pszHelpFile;
} CPLAPPLET;

typedef CPLAPPLET * PCPLAPPLET;


typedef struct tagCPLMODULE
{
    HANDLE      hLibrary;       // handle to the library module
    APPLET_PROC lpfnCPlApplet;  // far ptr to exported function
    PCPLAPPLET  pCPlApps;       // ptr to first in array CPLAPPLET structs
    int         numApplets;     // number of applets pointed to by pCPlApps
    TCHAR       szPathname[MAX_PATH];  // Full pathname of module
    BOOL        bLoaded;               // Is module loaded into memory?
} CPLMODULE;

typedef CPLMODULE * PCPLMODULE;


typedef struct
{
    PCPLMODULE pCPlMod;            //  ptr to a base CPL Module struct
    int        iApplet;            //  CPL Applet number in that CPL Module
} NTCPL, *LPNTCPL;


//  Registry keyname linked-list structure
typedef struct _regkey
{
    struct _regkey *prkNext;
    LPTSTR pszKeyName;
} REGKEY;


//  Used by cache routines
typedef struct tagCPLFILES
{
    BOOL        bAlreadyMatched;      //  Has this module already been matched up
                                      //   with a corresponding fs/cache module?
    DWORD       dwSize;               //  Module Filesize
    FILETIME    ftModule;             //  Module Filetime
    int         numApplets;           //  Number of applets
    TCHAR       szPathname[MAX_PATH]; //  Full pathname of module
} CPLFILES, *PCPLFILES;


//==========================================================================
//                             Debug Macros
//==========================================================================
// NOTE:  These macros will print out the Source file name and line number
//        where the RIP occurred.
//      * Use of these macros require the string "szAppName" to always be
//        defined in global data space.  (e.g.  char  szAppName = "Control Panel";)
//      * Use of these macros require the global string "szErrorText" to always be
//        defined in global data space.  (e.g.  TCHAR  szErrorText[255];)
//      * The RIP() macro requires that a Message parameter be present, but
//        RIP("") and RIP(NULL) are valid if no message is required.
//      * The ERROR_ROUTINE define is used to allow a driver to still log
//        errors to a file (or somewhere else) in the case where DBG is
//        defined as 0 (FALSE).
//
#if DBG
#define RIPBREAK(Msg) {wsprintf (szErrorText, TEXT("\nRIP %s - %s\n  Source File: %s, line %ld\n"), \
                  szAppName, Msg ? Msg : TEXT(""), TEXT(__FILE__), __LINE__ ); \
                  OutputDebugString (szErrorText); DebugBreak(); }

#define RIP(Msg) {wsprintf (szErrorText, TEXT("\nRIP %s - %s\n  Source File: %s, line %ld\n"), \
                  szAppName, Msg ? Msg : TEXT(""), TEXT(__FILE__), __LINE__ ); \
                  OutputDebugString (szErrorText); }

#define RIPREG() {wsprintf (szErrorText, TEXT("\nRIP %s - %s\n  Source File: %s, line %ld, GetLastError returns: %d\n"), \
                  szAppName, TEXT("Registry Error"), TEXT(__FILE__),  \
                   __LINE__, GetLastError()); \
                  OutputDebugString (szErrorText); }

#define RIPGEN() {wsprintf (szErrorText, TEXT("\nRIP %s - %s\n  Source File: %s, line %ld, GetLastError returns: %d\n"), \
                  szAppName, TEXT("General Error"), TEXT(__FILE__), \
                  __LINE__, GetLastError()); \
                  OutputDebugString (szErrorText); }

#define RIPMEM() {wsprintf (szErrorText, TEXT("\nRIP %s - %s\n  Source File: %s, line %ld, GetLastError returns: %d\n"), \
                  szAppName, TEXT("Memory Error"), TEXT(__FILE__), \
                  __LINE__, GetLastError()); \
                  OutputDebugString (szErrorText); }

#define ASSERT(a) if (!(a)) RIP(TEXT("Assertion Failure"))

#define ASSERTMSG(a,Msg) if (!(a)) RIP(Msg)

#define ERROR_ROUTINE RIP

#else

#define RIP(Msg)
#define RIPREG()
#define RIPGEN()
#define RIPMEM()
#define ASSERT(a)
#define ASSERTMSG(a,Msg)
#define ERROR_ROUTINE vLogError(szAppName, TEXT("An error has just reared its' ugly head"), \
                                TEXT(__FILE__), __LINE__)
#endif // DBG


#define vLogError(a,b,c,d) {wsprintf (szErrorText, TEXT("\n%s.%s\n  Source File: %s, line %ld\n"),  \
                                      a, b ? b : TEXT(""), c, d); OutputDebugString (szErrorText) }


//==========================================================================
//                            External Data
//==========================================================================

extern PCPLMODULE CPlMods[];            //  Array of ptrs to CPLMODULE struct
extern HWND       hCPlWnd;              //  MMCPL main window handle
extern HWND       hCPlLB;               //  List box window handle
extern HWND       hCPlTB;               //  Text box window handle
extern HFONT      hCPlFont;             //  Handle to MMCPl font for applet names
extern WORD       numApps;              //  Number of lData's in lCPlApps[]
extern int        iWidth;               //  Width of the owner-draw columns
extern TCHAR     *szWndDim[];           //  .ini file key names under MMCPL
extern TCHAR      szNumApps[];
extern TCHAR      szMaxWidth[];

extern LPTSTR     pStrings[];           //  Global strings array

#define szAppName   (pStrings[0])
#define szLoading   (pStrings[1])
#define szErrMem    (pStrings[2])
#define szErrNoApps (pStrings[3])

extern TCHAR      aszControlIni[];
extern TCHAR      aszControlIniPath[];

extern TCHAR      szMAINCPL[];
extern TCHAR      szMMCPL[];
extern TCHAR      szNULL[];
extern TCHAR      szCPL[];
extern TCHAR      szDots[];
extern TCHAR      cBackslash;
extern BYTE       szCPlApplet[];        //  Procedure name

extern TCHAR      szUsername[];         //  Global Username string

extern TCHAR      szErrorText[];        //  Used in ASSERT and Debug macros

extern BOOL       bValidationDone;

//==========================================================================
//                          External Functions
//==========================================================================

void AbsoluteValidation (void);
BOOL AskModule (LPTSTR, int);
void BuildCache (void);
void CatPath (LPTSTR, LPTSTR, int);
BOOL CheckCache (void);
void ClearCacheValid(void);
void CreateMutexNameFromPath (LPTSTR pszPathname, LPTSTR pszMutex);
void ErrMemDlg (HWND);
void FreeApps (void);
void FreeCachedModule (PCPLMODULE pCPlMod);
int  GetNameExtent (HDC, LPTSTR);
BOOL IsCacheValid (void);
BOOL LoadAndSizeApplets (void);
BOOL LoadCachedModule (PCPLMODULE pCPlMod);
BOOL LoadFromCache (void);
void MakeSysPath (LPTSTR, LPTSTR);
BOOL NameCmp (LPTSTR, LPTSTR);
void RebuildCache(void);
void ScanName (LPTSTR);
void UpdateCache (void);


