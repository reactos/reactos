#ifndef _SHSEMIP_H_
#define _SHSEMIP_H_

#ifndef LPNTSTR_DEFINED
#define LPNTSTR_DEFINED

typedef UNALIGNED const WCHAR * LPNCWSTR;
typedef UNALIGNED WCHAR *       LPNWSTR;

#ifdef UNICODE
#define LPNCTSTR        LPNCWSTR
#define LPNTSTR         LPNWSTR
#else
#define LPNCTSTR        LPCSTR
#define LPNTSTR         LPSTR
#endif

#endif // LPNTSTR_DEFINED

#ifndef DONT_WANT_SHELLDEBUG

#ifndef DebugMsg                                                                /* ;Internal */
#define DM_TRACE    0x0001      // Trace messages                               /* ;Internal */
#define DM_WARNING  0x0002      // Warning                                      /* ;Internal */
#define DM_ERROR    0x0004      // Error                                        /* ;Internal */
#define DM_ASSERT   0x0008      // Assertions                                   /* ;Internal */
#define Assert(f)                                                               /* ;Internal */
#define AssertE(f)      (f)                                                     /* ;Internal */
#define AssertMsg   1 ? (void)0 : (void)                                        /* ;Internal */
#define DebugMsg    1 ? (void)0 : (void)                                        /* ;Internal */
#endif                                                                          /* ;Internal */
                                                                                /* ;Internal */
#endif



//
// Define API decoration for direct importing of DLL references.
//
#ifndef WINSHELLAPI
#if !defined(_SHELL32_)
#define WINSHELLAPI DECLSPEC_IMPORT
#else
#define WINSHELLAPI
#endif
#endif // WINSHELLAPI

#ifndef NOPRAGMAS
#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif /* !RC_INVOKED */
#endif

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

//====== Ranges for WM_NOTIFY codes ==================================
// If a new set of codes is defined, make sure the range goes   /* ;Internal */
// here so that we can keep them distinct                       /* ;Internal */
// Note that these are defined to be unsigned to avoid compiler warnings
// since NMHDR.code is declared as UINT.
//
// NM_FIRST - NM_LAST defined in commctrl.h (0U-0U) - (OU-99U)
//
// LVN_FIRST - LVN_LAST defined in commctrl.h (0U-100U) - (OU-199U)
//
// PSN_FIRST - PSN_LAST defined in prsht.h (0U-200U) - (0U-299U)
//
// HDN_FIRST - HDN_LAST defined in commctrl.h (0U-300U) - (OU-399U)
//
// TVN_FIRST - TVN_LAST defined in commctrl.h (0U-400U) - (OU-499U)

// TTN_FIRST - TTN_LAST defined in commctrl.h (0U-520U) - (OU-549U)

#define SEN_FIRST       (0U-550U)       // ;Internal
#define SEN_LAST        (0U-559U)       // ;Internal


#ifndef UNIX
#define MAXPATHLEN      MAX_PATH        // ;Internal
#endif

#ifdef UNICODE
#define OTHER_TCHAR_NAME(sz)      sz##A
#else // !UNICODE
#define OTHER_TCHAR_NAME(sz)      sz##W
#endif

//===========================================================================
// ITEMIDLIST
//===========================================================================

// unsafe macros
#define _ILSkip(pidl, cb)       ((LPITEMIDLIST)(((BYTE*)(pidl))+cb))
#define _ILNext(pidl)           _ILSkip(pidl, (pidl)->mkid.cb)


//===================================================================
// Smart tiling API's
WINSHELLAPI WORD WINAPI ArrangeWindows(HWND hwndParent, WORD flags, LPCRECT lpRect, WORD chwnd, const HWND FAR *ahwnd);


/* Avoid multiple typedefs C warnings. Defined in shlapip.h as well. */
#ifndef RUNDLLPROC

typedef void (WINAPI FAR* RUNDLLPROCA)(HWND hwndStub,
        HINSTANCE hAppInstance,
        LPSTR lpszCmdLine, int nCmdShow);

typedef void (WINAPI FAR* RUNDLLPROCW)(HWND hwndStub,
        HINSTANCE hAppInstance,
        LPWSTR lpszCmdLine, int nCmdShow);

#ifdef UNICODE
#define RUNDLLPROC  RUNDLLPROCW
#else
#define RUNDLLPROC  RUNDLLPROCA
#endif
#endif

// Maximum length of a path string
#define CCHPATHMAX      MAX_PATH
#define MAXSPECLEN      MAX_PATH
#define MAX_PATH_URL    INTERNET_MAX_URL_LENGTH
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

#ifndef SIZEOF
#define SIZEOF(a)       sizeof(a)
#endif

#define PathRemoveBlanksORD     33
#define PathFindFileNameORD     34
#define PathFindExtensionORD    31

#ifdef OVERRIDE_SHLWAPI_PATH_FUNCTIONS
// SHLWAPI provides the majority of the Path functions.  There are
// some cases where the shell code (shell32 and explorer) need to
// call a different variation of these calls.  Because of this, we
// have OVERRIDE_SHLWAPI_PATH_FUNCTIONS.  Components such as shdocvw
// should strive to *not* have this defined.
//
// Some reasons why something like shell32 would need this:
//   1)  Shell32 calls some WNet APIs due to the NT merge.  Shlwapi
//       cannot take these.
//   2)  Shell32 needs the unaligned version PathBuildRoot,
//       PathCombine, etc.
//

#undef PathIsDirectory
#undef PathMakePretty

WINSHELLAPI BOOL  WINAPI PathIsDirectory(LPCTSTR lpszPath);
WINSHELLAPI BOOL  WINAPI PathMakePretty(LPTSTR lpszPath);

#endif // OVERRIDE_SHLWAPI_PATH_FUNCTIONS


WINSHELLAPI void  WINAPI ExitWindowsDialog(HWND hwnd);
WINSHELLAPI void  WINAPI LogoffWindowsDialog(HWND hwnd);
WINSHELLAPI BOOL  WINAPI IsSuspendAllowed(void);

WINSHELLAPI int   WINAPI PickIconDlg(HWND hwnd, LPTSTR pszIconPath, UINT cbIconPath, int FAR *piIconIndex);

WINSHELLAPI void WINAPI SHRefreshSettings(void);
WINSHELLAPI LRESULT WINAPI SHRenameFile(HWND hwndParent, LPCTSTR pszDir, LPCTSTR pszOldName, LPCTSTR pszNewName, BOOL bRetainExtension);


WINSHELLAPI BOOL    WINAPI SHRunControlPanel(LPCTSTR lpcszCmdLine, HWND hwndMsgParent);

EXTERN_C WINSHELLAPI HRESULT STDAPICALLTYPE SHCLSIDFromString(LPCTSTR lpsz, LPCLSID lpclsid);

EXTERN_C WINSHELLAPI HRESULT STDAPICALLTYPE SHCopyMonikerToTemp(IMoniker *pmk, LPCWSTR pszIn, LPWSTR pszOut, int cchOut);

WINSHELLAPI BOOL WINAPI IsVolumeNTFS(LPCTSTR pszRootPath);

#ifdef WINNT
WINSHELLAPI LPWSTR WINAPI GetDownlevelCopyDataLossText(LPCWSTR pszSrcFile, LPCWSTR pszDestDir, BOOL bIsADir, BOOL * pbLossPossibleThisDir);
#endif

//-------- file engine stuff ----------

// "current directory" management routines.  used to set parameters
// that paths are qualfied against in MoveCopyDeleteRename()

WINSHELLAPI int  WINAPI GetDefaultDrive();
WINSHELLAPI int  WINAPI SetDefaultDrive(int iDrive);
WINSHELLAPI int  WINAPI SetDefaultDirectory(LPCTSTR lpPath);
WINSHELLAPI void WINAPI GetDefaultDirectory(int iDrive, LPSTR lpPath);
//
// NOTES: No reason to have this one here, but I don't want to break the build.
//
#ifndef WINCOMMCTRLAPI
int WINAPI StrToInt(LPCTSTR lpSrc);  // atoi()
#endif

#define POSINVALID  32767       // values for invalid position

#define IDCMD_SYSTEMFIRST       0x8000
#define IDCMD_SYSTEMLAST        0xbfff
#define IDCMD_CANCELED          0xbfff
#define IDCMD_PROCESSED         0xbffe
#define IDCMD_DEFAULT           0xbffe

/* timedate.c */

// **********************************************************************
//  DATE is a structure with a date packed into a WORD size value. It
//  is compatible with a file date in a directory entry structure.
// **********************************************************************

#ifndef DATE_DEFINED
typedef struct
{
    WORD    Day     :5; // Day number 1 - 31
    WORD    Month   :4; // Month number 1 - 12
    WORD    Year    :7; // Year subtracted from 1980, 0-127
} WORD_DATE;

typedef union
{
    WORD            wDate;
    WORD_DATE       sDate;
} WDATE;

#define DATE_DEFINED
#endif

// **********************************************************************
//  TIME is a structure with a 24 hour time packed into a WORD size value.
//  It is compatible with a file time in a directory entry structure.
// **********************************************************************

#ifndef TIME_DEFINED

typedef struct
{
        WORD    Sec     :5;     // Seconds divided by 2 (0 - 29).
        WORD    Min     :6;     // Minutes 0 - 59
        WORD    Hour    :5;     // Hours 0 - 24
} WORD_TIME;

typedef union
{
        WORD        wTime;
        WORD_TIME   sTime;
} WTIME;

#define TIME_DEFINED
#endif

WINSHELLAPI WORD WINAPI Shell_GetCurrentDate(void);
WINSHELLAPI WORD WINAPI Shell_GetCurrentTime(void);

//====== SEMI-PRIVATE API ===============================
#ifndef HPSXA_DEFINED
#define HPSXA_DEFINED
DECLARE_HANDLE( HPSXA );
#endif // HPSXA_DEFINED

//====== SEMI-PRIVATE API ORDINALS ===============================
// This is the list of semi-private ordinals we semi-publish.
#define SHObjectPropertiesORD                   178
#define SHCreateDefClassObjectORD                70
#define SHGetNetResourceORD                      69

#define SHEXP_SHCREATEDEFCLASSOBJECT            MAKEINTRESOURCE(SHCreateDefClassObjectORD)
#define SHEXP_SHGETNETRESOURCE                  MAKEINTRESOURCE(SHGetNetResourceORD)


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#ifndef RC_INVOKED
#ifndef NOPRAGMAS
#pragma pack()
#endif /* NOPRAGMAS */
#endif  /* !RC_INVOKED */

#endif // _SHSEMIP_H_
