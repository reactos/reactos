#ifndef _SHSEMIP_H_
#define _SHSEMIP_H_

#if (defined(UNICODE) && !defined(_X86_)) // all non-x86 systems require alignment

#define ALIGNMENT_SCENARIO

#endif

typedef UNALIGNED const WCHAR * LPNCWSTR;
typedef UNALIGNED WCHAR *       LPNWSTR;

#ifdef UNICODE
#define LPNCTSTR        LPNCWSTR
#define LPNTSTR         LPNWSTR
#else
#define LPNCTSTR        LPCSTR
#define LPNTSTR         LPSTR
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

#define RFN_FIRST       (0U-510U) // run file dialog notify
#define RFN_LAST        (0U-519U)

#define SEN_FIRST       (0U-550U)       // ;Internal
#define SEN_LAST        (0U-559U)       // ;Internal


#ifndef UNIX
#define MAXPATHLEN      MAX_PATH        // ;Internal
#endif



//===========================================================================
// ITEMIDLIST
//===========================================================================

// flags for ILGetDisplayNameEx
#define ILGDN_FULLNAME  0
#define ILGDN_ITEMONLY  1
#define ILGDN_INFOLDER  2

#ifndef USE_SHLWAPI_IDLIST
WINSHELLAPI LPITEMIDLIST  WINAPI ILGetNext(LPCITEMIDLIST pidl);
WINSHELLAPI UINT          WINAPI ILGetSize(LPCITEMIDLIST pidl);
WINSHELLAPI LPITEMIDLIST  WINAPI ILFindLastID(LPCITEMIDLIST pidl);
WINSHELLAPI BOOL          WINAPI ILRemoveLastID(LPITEMIDLIST pidl);

#define ILIsEmpty(pidl)     ((pidl)->mkid.cb==0)
#endif

WINSHELLAPI LPITEMIDLIST  WINAPI ILCreate(void);
WINSHELLAPI LPITEMIDLIST  WINAPI ILAppendID(LPITEMIDLIST pidl, LPCSHITEMID pmkid, BOOL fAppend);
WINSHELLAPI void          WINAPI ILFree(LPITEMIDLIST pidl);
WINSHELLAPI void          WINAPI ILGlobalFree(LPITEMIDLIST pidl);
WINSHELLAPI BOOL          WINAPI ILGetDisplayName(LPCITEMIDLIST pidl, LPTSTR pszName);
WINSHELLAPI BOOL          WINAPI ILGetDisplayNameEx(LPSHELLFOLDER psfRoot, LPCITEMIDLIST pidl, LPTSTR pszName, int fType);
WINSHELLAPI BOOL          WINAPI ILGetPseudoNameW(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlBase, LPWSTR pwzName, int fType);
#ifdef UNICODE
#define ILGetPseudoName         ILGetPseudoNameW
#else
#define ILGetPseudoName         ILGetPseudoNameA    // client-side thunk req'd
BOOL ILGetPseudoNameA(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlBase, LPTSTR pszName, int fType);
#endif
WINSHELLAPI LPITEMIDLIST  WINAPI ILClone(LPCITEMIDLIST pidl);
WINSHELLAPI LPITEMIDLIST  WINAPI ILCloneFirst(LPCITEMIDLIST pidl);
WINSHELLAPI LPITEMIDLIST  WINAPI ILGlobalClone(LPCITEMIDLIST pidl);
WINSHELLAPI BOOL          WINAPI ILIsEqual(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
WINSHELLAPI BOOL          WINAPI ILIsParent(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, BOOL fImmediate);
WINSHELLAPI LPITEMIDLIST  WINAPI ILFindChild(LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidlChild);
WINSHELLAPI LPITEMIDLIST  WINAPI ILCombine(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
WINSHELLAPI HRESULT       WINAPI ILLoadFromStream(LPSTREAM pstm, LPITEMIDLIST *pidl);
WINSHELLAPI HRESULT       WINAPI ILSaveToStream(LPSTREAM pstm, LPCITEMIDLIST pidl);
WINSHELLAPI HRESULT       WINAPI ILLoadFromFile(HFILE hfile, LPITEMIDLIST *pidl);
WINSHELLAPI HRESULT       WINAPI ILSaveToFile(HFILE hfile, LPCITEMIDLIST pidl);
WINSHELLAPI LPITEMIDLIST  WINAPI _ILCreate(UINT cbSize);

WINSHELLAPI LPITEMIDLIST  WINAPI ILCreateFromPathA(LPCSTR pszPath);
WINSHELLAPI LPITEMIDLIST  WINAPI ILCreateFromPathW(LPCWSTR pszPath);
#ifdef UNICODE
#define ILCreateFromPath        ILCreateFromPathW
#else
#define ILCreateFromPath        ILCreateFromPathA
#endif

WINSHELLAPI HRESULT       WINAPI SHILCreateFromPath(LPCTSTR szPath, LPITEMIDLIST *ppidl, DWORD *rgfInOut);

// helper macros
#define ILCreateFromID(pmkid)   ILAppendID(NULL, pmkid, TRUE)

// unsafe macros
#define _ILSkip(pidl, cb)       ((LPITEMIDLIST)(((BYTE*)(pidl))+cb))
#define _ILNext(pidl)           _ILSkip(pidl, (pidl)->mkid.cb)

/*
 * The SHObjectProperties API provides an easy way to invoke
 *   the Properties context menu command on shell objects.
 *
 *   PARAMETERS
 *
 *     hwndOwner    The window handle of the window which will own the dialog
 *     dwType       A SHOP_ value as defined below
 *     lpObject     Name of the object, see SHOP_ values below
 *     lpPage       The name of the property sheet page to open to or NULL.
 *
 *   RETURN
 *
 *     TRUE if the Properties command was invoked
 */
WINSHELLAPI BOOL WINAPI SHObjectProperties(HWND hwndOwner, DWORD dwType, LPCTSTR lpObject, LPCTSTR lpPage);

#define SHOP_PRINTERNAME 1  // lpObject points to a printer friendly name
#define SHOP_FILEPATH    2  // lpObject points to a fully qualified path+file name
#define SHOP_TYPEMASK   0x00000003
#define SHOP_MODAL      0x80000000




//===================================================================
// Smart tiling API's
WINSHELLAPI WORD WINAPI ArrangeWindows(HWND hwndParent, WORD flags, LPCRECT lpRect, WORD chwnd, const HWND FAR *ahwnd);

//
// for SHGetNetResource
//
typedef HANDLE HNRES;

//
// For SHCreateDefClassObject
//
typedef HRESULT (CALLBACK *LPFNCREATEINSTANCE)(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObject);

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

//=======================================================================
// String constants for
//  1. Registration database keywords       (prefix STRREG_)
//  2. Exported functions from handler dlls (prefix STREXP_)
//  3. .INI file keywords                   (prefix STRINI_)
//  4. Others                               (prefix STR_)
//=======================================================================
#define STRREG_SHELLUI          TEXT("ShellUIHandler")
#define STRREG_SHELL            TEXT("Shell")
#define STRREG_DEFICON          TEXT("DefaultIcon")
#define STRREG_SHEX             TEXT("shellex")
#define STRREG_SHEX_PROPSHEET   STRREG_SHEX TEXT("\\PropertySheetHandlers")
#define STRREG_SHEX_DDHANDLER   STRREG_SHEX TEXT("\\DragDropHandlers")
#define STRREG_SHEX_MENUHANDLER STRREG_SHEX TEXT("\\ContextMenuHandlers")
#define STRREG_SHEX_COPYHOOK    TEXT("Directory\\") STRREG_SHEX TEXT("\\CopyHookHandlers")
#define STRREG_SHEX_PRNCOPYHOOK TEXT("Printers\\") STRREG_SHEX TEXT("\\CopyHookHandlers")
#define STRREG_STARTMENU TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MenuOrder\\Start Menu")
#define STRREG_FAVORITES TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MenuOrder\\Favorites")
#define STRREG_DISCARDABLE      TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Discardable")
#define STRREG_POSTSETUP        TEXT("\\PostSetup")

#define STREXP_CANUNLOAD        "DllCanUnloadNow"       // From OLE 2.0

#define STRINI_CLASSINFO        TEXT(".ShellClassInfo")       // secton name
#define STRINI_SHELLUI          TEXT("ShellUIHandler")
#define STRINI_OPENDIRICON      TEXT("OpenDirIcon")
#define STRINI_DIRICON          TEXT("DirIcon")

#define STR_DESKTOPINI          TEXT("desktop.ini")
#define STR_DESKTOPINIA         "desktop.ini"



// Maximum length of a path string
#define CCHPATHMAX      MAX_PATH
#define MAXSPECLEN      MAX_PATH
#define MAX_PATH_URL    INTERNET_MAX_URL_LENGTH
#define DRIVEID(path)   ((path[0] - 'A') & 31)
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

#ifndef SIZEOF
#define SIZEOF(a)       sizeof(a)
#endif

#define PATH_CCH_EXT    64
// PathResolve flags
#define PRF_VERIFYEXISTS            0x0001
#define PRF_TRYPROGRAMEXTENSIONS    (0x0002 | PRF_VERIFYEXISTS)
#define PRF_FIRSTDIRDEF             0x0004
#define PRF_DONTFINDLNK             0x0008      // if PRF_TRYPROGRAMEXTENSIONS is specified


//
// For CallCPLEntry16
//
DECLARE_HANDLE(FARPROC16);

#ifdef RFN_FIRST
#define RFN_EXECUTE             (RFN_FIRST - 0)
typedef struct {
    NMHDR hdr;
    LPCTSTR lpszCmd;
    LPCTSTR lpszWorkingDir;
    int nShowCmd;
} NMRUNFILEA, FAR *LPNMRUNFILEA;

typedef struct {
    NMHDR hdr;
    LPCWSTR lpszCmd;
    LPCWSTR lpszWorkingDir;
    int nShowCmd;
} NMRUNFILEW, FAR *LPNMRUNFILEW;

#ifdef UNICODE
#define NMRUNFILE       NMRUNFILEW
#define LPNMRUNFILE     LPNMRUNFILEW
#else
#define NMRUNFILE       NMRUNFILEA
#define LPNMRUNFILE     LPNMRUNFILEA
#endif


#endif

// RUN FILE RETURN values from notify message
#define RFR_NOTHANDLED 0
#define RFR_SUCCESS 1
#define RFR_FAILURE 2

#define PathRemoveBlanksORD     33
#define PathFindFileNameORD     34
#define PathGetExtensionORD     158
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
#undef PathFileExists
#undef PathMakePretty

WINSHELLAPI BOOL  WINAPI PathIsDirectory(LPCTSTR lpszPath);
WINSHELLAPI BOOL  WINAPI PathFileExists(LPCTSTR lpszPath);
WINSHELLAPI BOOL  WINAPI PathMakePretty(LPTSTR lpszPath);

// Need to use the unaligned versions for non-Intel platforms
//
#undef PathBuildRoot
#undef PathCombine
#undef PathAppend
#undef PathIsUNC
#undef PathGetDriveNumber
#undef PathIsRelative

WINSHELLAPI LPNTSTR WINAPI PathBuildRoot(LPNTSTR szRoot, int iDrive);
WINSHELLAPI LPTSTR  WINAPI PathCombine(LPTSTR szDest, LPCTSTR lpszDir, LPNCTSTR lpszFile);
WINSHELLAPI BOOL    WINAPI PathAppend(LPTSTR pPath, LPNCTSTR pMore);
WINSHELLAPI BOOL    WINAPI PathIsUNC(LPNCTSTR lpsz);
WINSHELLAPI int     WINAPI PathGetDriveNumber(LPNCTSTR lpszPath);
WINSHELLAPI BOOL    WINAPI PathIsRelative(LPNCTSTR lpszPath);

#endif // OVERRIDE_SHLWAPI_PATH_FUNCTIONS

WINSHELLAPI LPTSTR WINAPI PathGetExtension(LPCTSTR lpszPath, LPTSTR lpszExtension, int cchExt);
WINSHELLAPI BOOL  WINAPI PathMakeUniqueName(LPTSTR pszUniqueName, UINT cchMax, LPCTSTR pszTemplate, LPCTSTR pszLongPlate, LPCTSTR pszDir);
WINSHELLAPI BOOL  WINAPI PathGetShortName(LPCTSTR lpszLongName, LPTSTR lpszShortName, UINT cbShortName);
WINSHELLAPI BOOL  WINAPI PathGetLongName(LPCTSTR lpszShortName, LPTSTR lpszLongName, UINT cbLongName);
WINSHELLAPI BOOL  WINAPI PathDirectoryExists(LPCTSTR lpszDir);
WINSHELLAPI void  WINAPI PathQualify(LPTSTR lpsz);
WINSHELLAPI LPTSTR WINAPI PathGetNextComponent(LPCTSTR lpszPath, LPTSTR lpszComponent);
WINSHELLAPI BOOL  WINAPI PathIsExe(LPCTSTR lpszPath);

WINSHELLAPI BOOL WINAPI PathIsSlowW( LPCWSTR pszFile, DWORD dwAttr );
WINSHELLAPI BOOL WINAPI PathIsSlowA( LPCSTR pszFile, DWORD dwAttr );
#ifdef UNICODE
#define PathIsSlow	PathIsSlowW
#else
#define PathIsSlow	PathIsSlowA
#endif


//
//  Return codes from PathCleanupSpec.  Negative return values are
//  unrecoverable errors
//
#define PCS_FATAL           0x80000000
#define PCS_REPLACEDCHAR    0x00000001
#define PCS_REMOVEDCHAR     0x00000002
#define PCS_TRUNCATED       0x00000004
#define PCS_PATHTOOLONG     0x00000008  // Always combined with FATAL

STDAPI_(int) PathCleanupSpec(LPCTSTR pszDir, LPTSTR pszSpec);
STDAPI_(int) PathCleanupSpecEx(LPCTSTR pszDir, LPTSTR pszSpec);

WINSHELLAPI int   WINAPI PathResolve(LPTSTR lpszPath, LPCTSTR FAR dirs[], UINT fFlags);
WINSHELLAPI BOOL  WINAPI ParseField(LPCTSTR szData, int n, LPTSTR szBuf, int iBufLen);
WINSHELLAPI LPNTSTR WINAPI uaPathFindFileName(LPNCTSTR pPath);

WINSHELLAPI int   WINAPI RestartDialog(HWND hwnd, LPCTSTR lpPrompt, DWORD dwReturn);
WINSHELLAPI void  WINAPI ExitWindowsDialog(HWND hwnd);
WINSHELLAPI void  WINAPI LogoffWindowsDialog(HWND hwnd);
WINSHELLAPI BOOL  WINAPI IsSuspendAllowed(void);

// Needed for RunFileDlg
#define RFD_NOBROWSE            0x00000001
#define RFD_NODEFFILE           0x00000002
#define RFD_USEFULLPATHDIR      0x00000004
#define RFD_NOSHOWOPEN          0x00000008
#define RFD_WOW_APP             0x00000010
#define RFD_NOSEPMEMORY_BOX     0x00000020


WINSHELLAPI int   WINAPI RunFileDlg(HWND hwndParent, HICON hIcon, LPCTSTR lpszWorkingDir, LPCTSTR lpszTitle,
                                    LPCTSTR lpszPrompt, DWORD dwFlags);
WINSHELLAPI int   WINAPI PickIconDlg(HWND hwnd, LPTSTR pszIconPath, UINT cbIconPath, int FAR *piIconIndex);
WINSHELLAPI BOOL  WINAPI GetFileNameFromBrowse(HWND hwnd, LPTSTR szFilePath, UINT cbFilePath, LPCTSTR szWorkingDir, LPCTSTR szDefExt, LPCTSTR szFilters, LPCTSTR szTitle);

WINSHELLAPI int  WINAPI DriveType(int iDrive);
WINSHELLAPI int  WINAPI RealDriveTypeFlags(int iDrive, BOOL fOKToHitNet);
WINSHELLAPI int  WINAPI RealDriveType(int iDrive, BOOL fOKToHitNet);
WINSHELLAPI void WINAPI InvalidateDriveType(int iDrive);
WINSHELLAPI int  WINAPI IsNetDrive(int iDrive);

WINSHELLAPI UINT WINAPI Shell_MergeMenus(HMENU hmDst, HMENU hmSrc, UINT uInsert, UINT uIDAdjust, UINT uIDAdjustMax, ULONG uFlags);

WINSHELLAPI void WINAPI SHRefreshSettings(void);
WINSHELLAPI LRESULT WINAPI SHRenameFile(HWND hwndParent, LPCTSTR pszDir, LPCTSTR pszOldName, LPCTSTR pszNewName, BOOL bRetainExtension);

WINSHELLAPI UINT WINAPI SHGetNetResource(HNRES hnres, UINT iItem, LPNETRESOURCE pnres, UINT cbMax);

EXTERN_C WINSHELLAPI HRESULT STDAPICALLTYPE SHCreateDefClassObject(REFIID riid, LPVOID FAR* ppv, LPFNCREATEINSTANCE lpfn, UINT FAR * pcRefDll, REFIID riidInstance);

WINSHELLAPI LRESULT WINAPI CallCPLEntry16(HINSTANCE hinst, FARPROC16 lpfnEntry, HWND hwndCPL, UINT msg, LPARAM lParam1, LPARAM lParam2);
WINSHELLAPI BOOL    WINAPI SHRunControlPanel(LPCTSTR lpcszCmdLine, HWND hwndMsgParent);

EXTERN_C WINSHELLAPI HRESULT STDAPICALLTYPE SHCLSIDFromString(LPCTSTR lpsz, LPCLSID lpclsid);

WINSHELLAPI INT WINAPI LargeIntegerToString(LARGE_INTEGER *pN, LPTSTR szOutStr, UINT nSize, BOOL bFormat, NUMBERFMT *pFmt, DWORD dwNumFmtFlags);
WINSHELLAPI INT WINAPI Int64ToString(_int64 n, LPTSTR szOutStr, UINT nSize, BOOL bFormat, NUMBERFMT *pFmt, DWORD dwNumFmtFlags);

//
// Constants used for dwNumFmtFlags argument in Int64ToString and LargeIntegerToString.
//
#define NUMFMT_IDIGITS    0x00000001
#define NUMFMT_ILZERO     0x00000002
#define NUMFMT_SGROUPING  0x00000004
#define NUMFMT_SDECIMAL   0x00000008
#define NUMFMT_STHOUSAND  0x00000010
#define NUMFMT_INEGNUMBER 0x00000020
#define NUMFMT_ALL        0xFFFFFFFF

#define SHObjectPropertiesORD   178
WINSHELLAPI BOOL WINAPI SHObjectProperties(HWND hwndOwner, DWORD dwType, LPCTSTR lpObject, LPCTSTR lpPage);


//===================================================================
// Shell_MergeMenu parameter
//
#define MM_ADDSEPARATOR         0x00000001L
#define MM_SUBMENUSHAVEIDS      0x00000002L
#define MM_DONTREMOVESEPS       0x00000004L

//-------- drive type identification --------------
// iDrive      drive index (0=A, 1=B, ...)
//
#define DRIVE_CDROM     5           // extended DriveType() types
#define DRIVE_RAMDRIVE  6
#define DRIVE_TYPE      0x000F      // type masek
#define DRIVE_SLOW      0x0010      // drive is on a slow link
#define DRIVE_LFN       0x0020      // drive supports LFNs
#define DRIVE_AUTORUN   0x0040      // drive has AutoRun.inf in root.
#define DRIVE_AUDIOCD   0x0080      // drive is a AudioCD
#define DRIVE_AUTOOPEN  0x0100      // should *always* auto open on insert
#define DRIVE_NETUNAVAIL 0x0200     // Network drive that is not available
#define DRIVE_SHELLOPEN  0x0400     // should auto open on insert, if shell has focus
#define DRIVE_SECURITY   0x0800     // Supports ACLs
#define DRIVE_COMPRESSED 0x1000     // Root of volume is compressed
#define DRIVE_ISCOMPRESSIBLE 0x2000 // Drive supports compression (not nescesarrily compressed)
#define DRIVE_DVD       0x4000      // drive is a DVD

#define DriveTypeFlags(iDrive)      DriveType('A' + (iDrive))
#define DriveIsSlow(iDrive)         (RealDriveTypeFlags(iDrive, FALSE) & DRIVE_SLOW)
#define DriveIsLFN(iDrive)          (RealDriveTypeFlags(iDrive, TRUE)  & DRIVE_LFN)
#define DriveIsAutoRun(iDrive)      (RealDriveTypeFlags(iDrive, FALSE) & DRIVE_AUTORUN)
#define DriveIsAutoOpen(iDrive)     (RealDriveTypeFlags(iDrive, FALSE) & DRIVE_AUTOOPEN)
#define DriveIsShellOpen(iDrive)    (RealDriveTypeFlags(iDrive, FALSE) & DRIVE_SHELLOPEN)
#define DriveIsAudioCD(iDrive)      (RealDriveTypeFlags(iDrive, FALSE) & DRIVE_AUDIOCD)
#define DriveIsNetUnAvail(iDrive)   (RealDriveTypeFlags(iDrive, FALSE) & DRIVE_NETUNAVAIL)
#define DriveIsSecure(iDrive)       (RealDriveTypeFlags(iDrive, TRUE)  & DRIVE_SECURITY)
#define DriveIsCompressed(iDrive)   (RealDriveTypeFlags(iDrive, TRUE)  & DRIVE_COMPRESSED)
#define DriveIsCompressible(iDrive) (RealDriveTypeFlags(iDrive, TRUE)  & DRIVE_ISCOMPRESSIBLE)
#define DriveIsDVD(iDrive)          (RealDriveTypeFlags(iDrive, FALSE) & DRIVE_DVD)

#define IsCDRomDrive(iDrive)        (RealDriveType(iDrive, FALSE) == DRIVE_CDROM)
#define IsRamDrive(iDrive)          (RealDriveType(iDrive, FALSE) == DRIVE_RAMDRIVE)
#define IsRemovableDrive(iDrive)    (RealDriveType(iDrive, FALSE) == DRIVE_REMOVABLE)
#define IsRemoteDrive(iDrive)       (RealDriveType(iDrive, FALSE) == DRIVE_REMOTE)

WINSHELLAPI BOOL WINAPI IsVolumeNTFS(LPCTSTR pszRootPath);

#ifdef WINNT
WINSHELLAPI LPWSTR WINAPI GetDownlevelCopyDataLossText(LPCWSTR pszSrcFile, LPCWSTR pszDestDir);
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
#ifndef _SHELLP_H_
DECLARE_HANDLE( HPSXA );
#endif
WINSHELLAPI HPSXA SHCreatePropSheetExtArray( HKEY hKey, LPCTSTR pszSubKey, UINT max_iface );
WINSHELLAPI void SHDestroyPropSheetExtArray( HPSXA hpsxa );
WINSHELLAPI UINT SHAddFromPropSheetExtArray( HPSXA hpsxa, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam );
WINSHELLAPI UINT SHReplaceFromPropSheetExtArray( HPSXA hpsxa, UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam );

//====== SEMI-PRIVATE API ORDINALS ===============================
// This is the list of semi-private ordinals we semi-publish.
#define SHAddFromPropSheetExtArrayORD           167
#define SHCreatePropSheetExtArrayORD            168
#define SHDestroyPropSheetExtArrayORD           169
#define SHReplaceFromPropSheetExtArrayORD       170
#define SHCreateDefClassObjectORD                70
#define SHGetNetResourceORD                      69

#define SHEXP_SHADDFROMPROPSHEETEXTARRAY        MAKEINTRESOURCE(SHAddFromPropSheetExtArrayORD)
#define SHEXP_SHCREATEPROPSHEETEXTARRAY         MAKEINTRESOURCE(SHCreatePropSheetExtArrayORD)
#define SHEXP_SHDESTROYPROPSHEETEXTARRAY        MAKEINTRESOURCE(SHDestroyPropSheetExtArrayORD)
#define SHEXP_SHREPLACEFROMPROPSHEETEXTARRAY    MAKEINTRESOURCE(SHReplaceFromPropSheetExtArrayORD)
#define SHEXP_SHCREATEDEFCLASSOBJECT            MAKEINTRESOURCE(SHCreateDefClassObjectORD)
#define SHEXP_SHGETNETRESOURCE                  MAKEINTRESOURCE(SHGetNetResourceORD)


/*
 * The SHFormatDrive API provides access to the Shell
 *   format dialog. This allows apps which want to format disks
 *   to bring up the same dialog that the Shell does to do it.
 *
 *   This dialog is not sub-classable. You cannot put custom
 *   controls in it. If you want this ability, you will have
 *   to write your own front end for the DMaint_FormatDrive
 *   engine.
 *
 *   NOTE that the user can format as many diskettes in the specified
 *   drive, or as many times, as he/she wishes to. There is no way to
 *   force any specififc number of disks to format. If you want this
 *   ability, you will have to write your own front end for the
 *   DMaint_FormatDrive engine.
 *
 *   NOTE also that the format will not start till the user pushes the
 *   start button in the dialog. There is no way to do auto start. If
 *   you want this ability, you will have to write your own front end
 *   for the DMaint_FormatDrive engine.
 *
 *   PARAMETERS
 *
 *     hwnd    = The window handle of the window which will own the dialog
 *               NOTE that unlike SHCheckDrive, hwnd == NULL does not cause
 *               this dialog to come up as a "top level application" window.
 *               This parameter should always be non-null, this dialog is
 *               only designed to be the child of another window, not a
 *               stand-alone application.
 *     drive   = The 0 based (A: == 0) drive number of the drive to format
 *     fmtID   = The ID of the physical format to format the disk with
 *               NOTE: The special value SHFMT_ID_DEFAULT means "use the
 *                     default format specified by the DMaint_FormatDrive
 *                     engine". If you want to FORCE a particular format
 *                     ID "up front" you will have to call
 *                     DMaint_GetFormatOptions yourself before calling
 *                     this to obtain the valid list of phys format IDs
 *                     (contents of the PhysFmtIDList array in the
 *                     FMTINFOSTRUCT).
 *     options = There is currently only two option bits defined
 *
 *                SHFMT_OPT_FULL
 *                SHFMT_OPT_SYSONLY
 *
 *               The normal defualt in the Shell format dialog is
 *               "Quick Format", setting this option bit indicates that
 *               the caller wants to start with FULL format selected
 *               (this is useful for folks detecting "unformatted" disks
 *               and wanting to bring up the format dialog).
 *
 *               The SHFMT_OPT_SYSONLY initializes the dialog to
 *               default to just sys the disk.
 *
 *               All other bits are reserved for future expansion and
 *               must be 0.
 *
 *               Please note that this is a bit field and not a value
 *               and treat it accordingly.
 *
 *   RETURN
 *      The return is either one of the SHFMT_* values, or if the
 *      returned DWORD value is not == to one of these values, then
 *      the return is the physical format ID of the last succesful
 *      format. The LOWORD of this value can be passed on subsequent
 *      calls as the fmtID parameter to "format the same type you did
 *      last time".
 *
 */
DWORD WINAPI SHFormatDrive(HWND hwnd, UINT drive, UINT fmtID, UINT options);

DWORD WINAPI SHChkDskDrive(HWND hwnd, UINT drive);

//
// Special value of fmtID which means "use the default format"
//
#define SHFMT_ID_DEFAULT    0xFFFF

//
// Option bits for options parameter
//
#define SHFMT_OPT_FULL     0x0001
#define SHFMT_OPT_SYSONLY  0x0002

//
// Special return values. PLEASE NOTE that these are DWORD values.
//
#define SHFMT_ERROR     0xFFFFFFFFL     // Error on last format, drive may be formatable
#define SHFMT_CANCEL    0xFFFFFFFEL     // Last format was canceled
#define SHFMT_NOFORMAT  0xFFFFFFFDL     // Drive is not formatable


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#ifndef RC_INVOKED
#ifndef NOPRAGMAS
#pragma pack()
#endif /* NOPRAGMAS */
#endif  /* !RC_INVOKED */

#endif // _SHSEMIP_H_
