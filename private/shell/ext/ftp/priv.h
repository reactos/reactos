#ifndef _PRIV_H_
#define _PRIV_H_


/*****************************************************************************
 *
 *      Magic comments:
 *
 *      BUGBUG: Something that needs to be fixed before being released.
 *
 *      _UNDOCUMENTED_: Something that is not documented in the SDK.
 *
 *      _UNOBVIOUS_: Some unusual feature that isn't obvious from the
 *      documentation.  A candidate for a "Tips and Tricks" chapter.
 *
 *      _HACKHACK_: Something that is gross but necessary.
 *
 *      _CHARSET_: Character set issues.
 *
 *      Magic ifdefs:
 *
  *****************************************************************************/


/*****************************************************************************
 *
 *      Global Includes
 *
 *****************************************************************************/

#define WIN32_LEAN_AND_MEAN
#define NOIME
#define NOSERVICE

// This stuff must run on Win95
#define _WIN32_WINDOWS      0x0400

#ifndef WINVER
#define WINVER              0x0400
#endif // WINVER

#define _OLEAUT32_      // get DECLSPEC_IMPORT stuff right, we are defing these
#define _FSMENU_        // for DECLSPEC_IMPORT
#define _WINMM_         // for DECLSPEC_IMPORT in mmsystem.h
#define _SHDOCVW_       // for DECLSPEC_IMPORT in shlobj.h
#define _WINX32_        // get DECLSPEC_IMPORT stuff right for WININET API

#define _URLCACHEAPI_   // get DECLSPEC_IMPORT stuff right for wininet urlcache
#define STRICT

#define POST_IE5_BETA
#include <w95wraps.h>

#include <windows.h>

#ifdef  RC_INVOKED              /* Define some tags to speed up rc.exe */
#define __RPCNDR_H__            /* Don't need RPC network data representation */
#define __RPC_H__               /* Don't need RPC */
#include <oleidl.h>             /* Get the DROPEFFECT stuff */
#define _OLE2_H_                /* But none of the rest */
#define _WINDEF_
#define _WINBASE_
#define _WINGDI_
#define NONLS
#define _WINCON_
#define _WINREG_
#define _WINNETWK_
#define _INC_COMMCTRL
#define _INC_SHELLAPI
#define _SHSEMIP_H_             /* _UNDOCUMENTED_: Internal header */
#else // RC_INVOKED
#include <windowsx.h>
#endif // RC_INVOKED


#ifdef __cplusplus
extern "C" {                        /* Assume C declarations for C++. */
#endif   /* __cplusplus */


#ifdef __cplusplus
}                                   /* End of extern "C" {. */
#endif   /* __cplusplus */


#include "resource.h"

#define _FIX_ENABLEMODELESS_CONFLICT  // for shlobj.h
//WinInet need to be included BEFORE ShlObjp.h
#include <wininet.h>
#include <urlmon.h>
#include <shlobj.h>
#include <shlobjp.h>             // For IProgressDialog
#include <exdisp.h>
#include <objidl.h>

#include <shlwapi.h>
#include <shlwapip.h>

#include <shellapi.h>
#include <shlapip.h>

#include <shsemip.h>
#include <crtfree.h>

#include <ole2ver.h>
#include <olectl.h>
#include <shellp.h>
#include <commctrl.h>
#include <comctrlp.h>
#include <shdocvw.h>
#include <shguidp.h>
#include <shlguidp.h>
#include <isguids.h>
#include <shdguid.h>
#include <mimeinfo.h>
#include <hlguids.h>
#include <mshtmdid.h>
#include <dispex.h>     // IDispatchEx
#include <perhist.h>


#include <help.h>
#include <krnlcmn.h>    // GetProcessDword

#include <multimon.h>

#define DISALLOW_Assert             // Force to use ASSERT instead of Assert
#define DISALLOW_DebugMsg           // Force to use TraceMsg instead of DebugMsg
#include <debug.h>

#include <urlhist.h>

#include <regstr.h>     // for REGSTR_PATH_EXPLORE

#define USE_SYSTEM_URL_MONIKER
#include <urlmon.h>
//#include <winineti.h>    // Cache APIs & structures
#include <inetreg.h>

#define _INTSHCUT_    // get DECLSPEC_IMPORT stuff right for INTSHCUT.h
#include <intshcut.h>

#include <propset.h>        // BUGBUG (scotth): remove this once OLE adds an official header

#define HLINK_NO_GUIDS
#include <hlink.h>
#include <hliface.h>
#include <docobj.h>
#include <ccstock.h>
#include <port32.h>
#include <prsht.h>
#include <inetcpl.h>

#include <commctrl.h>

#include "unithunk.h"

// Trace flags
#define TF_FTPREF           0x00000100      // Dll Reference
#define TF_FTPPERF          0x00000200      // Perf
#define TF_FTPALLOCS        0x00000400      // Object Allocs
#define TF_FTPDRAGDROP      0x00000800      // Drag and Drop
#define TF_FTPLIST          0x00001000      // HDPA Wrapper
#define TF_FTPISF           0x00002000      // IShellFolder
#define TF_FTPQI            0x00004000      // QueryInterface
#define TF_FTPSTATUSBAR     0x00008000      // Status Bar Spew
#define TF_FTPOPERATION     0x00010000      // Ftp Operation (Put File, Get File, CreateDir, DeleteDir, ...)
#define TF_FTPURL_UTILS     0x00020000      // Ftp Url Operations (Pidl->Url, Url->Pidl, ...)
#define TF_FTP_DLLLOADING   0x00040000      // Loading Other DLLs
#define TF_FTP_OTHER        0x00080000      // Misc.
#define TF_FTP_IDENUM       0x00100000      // IDList Enum (IIDEnum).
#define TF_CHANGENOTIFY     0x00200000      // Change Notify
#define TF_PIDLLIST_DUMP    0x00400000      // Dump the contents in the Pidl List
#define TF_WININET_DEBUG    0x00800000      // Dump wininet calls
#define TF_BKGD_THREAD      0x01000000      // Background thread that caches wininet handles.
#define TF_FOLDER_SHRTCUTS  0x02000000      // Info about Folder Shortcuts.


/*****************************************************************************
 *
 *      Global Helper Macros/Typedefs
 *
 *****************************************************************************/

//////////////////////////// IE 5 vs IE 4 /////////////////////////////////
// These are functions that IE5 exposes (normally in shlwapi), but
// if we want to be compatible with IE4, we need to have our own copy.
// If we turn on USE_IE5_UTILS, we won't work with IE4's DLLs (like shlwapi).
//
// #define USE_IE5_UTILS
//////////////////////////// IE 5 vs IE 4 /////////////////////////////////



#ifdef OLD_HLIFACE
#define HLNF_OPENINNEWWINDOW HLBF_OPENINNEWWINDOW
#endif // OLD_HLIFACE

#define ISVISIBLE(hwnd)  ((GetWindowStyle(hwnd) & WS_VISIBLE) == WS_VISIBLE)

// shorthand
#ifndef ATOMICRELEASE
#define ATOMICRELEASET(p,type) { type* punkT=p; p=NULL; punkT->Release(); }

// doing this as a function instead of inline seems to be a size win.
//
#ifdef NOATOMICRELESEFUNC
#define ATOMICRELEASE(p) ATOMICRELEASET(p, IUnknown)
#else // NOATOMICRELESEFUNC

//////////////////////////// IE 5 vs IE 4 /////////////////////////////////
#ifndef USE_IE5_UTILS
#define ATOMICRELEASE(p)                FtpCopy_IUnknown_AtomicRelease((LPVOID*)&p)
void FtpCopy_IUnknown_AtomicRelease(LPVOID* ppunk);
#endif // USE_IE5_UTILS
//////////////////////////// IE 5 vs IE 4 /////////////////////////////////
#endif // NOATOMICRELESEFUNC

#endif // ATOMICRELEASE

#ifdef SAFERELEASE
#undef SAFERELEASE
#endif // SAFERELEASE
#define SAFERELEASE(p) ATOMICRELEASE(p)


#define IsInRange               InRange

// Include the automation definitions...
#include <exdisp.h>
#include <exdispid.h>
#include <shldispp.h>
#include <ocmm.h>
#include <mshtmhst.h>
#include <simpdata.h>
#include <htiface.h>
#include <objsafe.h>

//
// Neutral ANSI/UNICODE types and macros... 'cus Chicago seems to lack them
//

#ifdef  UNICODE
   typedef WCHAR TUCHAR, *PTUCHAR;

#else   /* UNICODE */

   typedef unsigned char TUCHAR, *PTUCHAR;
#endif /* UNICODE */



STDAPI_(void) DllAddRef(void);
STDAPI_(void) DllRelease(void);


#define CALLWNDPROC WNDPROC


extern HINSTANCE g_hinst;
#define HINST_THISDLL g_hinst


#include "idispids.h"


// Count of characters to count of bytes
//
#define CbFromCchW(cch)             ((cch)*sizeof(WCHAR))
#define CbFromCchA(cch)             ((cch)*sizeof(CHAR))
#ifdef UNICODE
#define CbFromCch                   CbFromCchW
#else  // UNICODE
#define CbFromCch                   CbFromCchA
#endif // UNICODE

// General flag macros
//
#define SetFlag(obj, f)             do {obj |= (f);} while (0)
#define ToggleFlag(obj, f)          do {obj ^= (f);} while (0)
#define ClearFlag(obj, f)           do {obj &= ~(f);} while (0)
#define IsFlagSet(obj, f)           (BOOL)(((obj) & (f)) == (f))
#define IsFlagClear(obj, f)         (BOOL)(((obj) & (f)) != (f))


#pragma intrinsic(memcmp, memcpy)


/*****************************************************************************
 *
 *      Baggage - Stuff I carry everywhere
 *
 *****************************************************************************/

#define BEGIN_CONST_DATA data_seg(".text", "CODE")
#define END_CONST_DATA data_seg(".data", "DATA")


// Convert an array name (A) to a generic count (c).
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

#define pvByteIndexCb(pv, cb) ((LPVOID)((PBYTE)pv + (cb)))


#ifdef DEBUG
#define DEBUG_CODE(x)            x
#else // DEBUG
#define DEBUG_CODE(x)
#endif // DEBUG


#undef lstrcatnW

// #undef  lstrcpyW
// #define lstrcpyW            BUGBUG_BAD_lstrcpyW

#undef  lstrcpynW
#define lstrcpynW           BUGBUG_BAD_lstrcpynW

#undef  lstrcmpW
#define lstrcmpW            BUGBUG_BAD_lstrcmpW

#undef  lstrcmpiW
#define lstrcmpiW           BUGBUG_BAD_lstrcmpiW

#undef  lstrcatW
#define lstrcatW            BUGBUG_BAD_lstrcatW

#undef  lstrcatnW
#define lstrcatnW           BUGBUG_BAD_lstrcatnW

// Fix shell32 bugs:
#define ILCombine           ILCombineWrapper
#define ILClone             ILCloneWrapper
#define ILFree              ILFreeWrapper

LPITEMIDLIST ILCombineWrapper(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
LPITEMIDLIST ILCloneWrapper(LPCITEMIDLIST pidl);
void ILFreeWrapper(LPITEMIDLIST pidl);


/*****************************************************************************
 *
 *      Wrappers and other quickies
 *
 *****************************************************************************/

#define HRESULT_FROM_SUCCESS_VALUE(us) MAKE_HRESULT(SEVERITY_SUCCESS, 0, (USHORT)(us))

#define ILIsSimple(pidl) (ILIsEmpty(_ILNext(pidl)))

typedef DWORD DROPEFFECT;

#define ProtocolIdlInnerData(pidl)  ((LPVOID)&(((PDELEGATEITEMID)(pidl))->rgb[0]))
#define ProtocolIdlInnerDataSize  (sizeof(DELEGATEITEMID) - (sizeof(BYTE)*2))

/*****************************************************************************
 *
 *      Const globals:  Never change.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

extern char c_szSlash[];        /* "/" */

extern WORD c_wZero;            /* A word of zeros */

#pragma END_CONST_DATA

#define c_pidlNil               ((LPCITEMIDLIST)&c_wZero)       /* null pidl */
#define c_tszNil                ((LPCTSTR)&c_wZero)     /* null string */
#define c_szNil                 ((LPCSTR)&c_wZero)      /* null string */

/*****************************************************************************
 *
 *      Static globals:  Initialized at PROCESS_ATTACH and never modified.
 *
 *      WARNING! <shelldll\idlcomm.h> #define's various g_cf's, so we need
 *      to #undef them before we start partying on them again.
 *
 *****************************************************************************/

#undef g_cfFileDescriptor
#undef g_cfFileContents
#undef g_cfShellIDList
#undef g_cfFileNameMap
#undef g_cfPreferredDe

extern HINSTANCE                g_hinst;                    // My instance handle
extern CHAR                     g_szShell32[MAX_PATH];      // Full path to shell32.dll (must be ANSI)


// Detect "." or ".." as invalid files
#define IS_VALID_FILE(str)        (!(('.' == str[0]) && (('\0' == str[1]) || (('.' == str[1]) && ('\0' == str[2])))))

extern FORMATETC g_formatEtcOffsets;
extern FORMATETC g_formatPasteSucceeded;
extern FORMATETC g_dropTypes[];

extern CLIPFORMAT g_cfTargetCLSID;

// TODO: 
//     Create a DROP_FTP clipboard format that will
//     give a source URL, a destination URL, and
//     a list of files that need to be moved or copied
//     from relative to the source URL to relative to
//     the destination URL.

enum DROPTYPES
{
    // File contents is inherently weird
    DROP_FCont = 0,     // File contents

    // Clipboard formats we offer from our DataObject
    DROP_FGDW,          // File Group DescriptorW 1
    DROP_FGDA,          // File Group DescriptorA 2
    DROP_IDList,        // ID List Array 3
    DROP_URL,           // Uniform Resource Locator 4
//    DROP_Offsets,       // Shell Object Offsets
    DROP_PrefDe,        // Preferred Drop Effect 5
    DROP_PerfDe,        // Performed Drop Effect 6
    DROP_FTP_PRIVATE,   // Ftp Private Data - Flag to indicate Ftp was the drag source 7
    DROP_OLEPERSIST,        // Used by ole to realize that they should persist via IPersistStream across OleFlushClipboard calls.

    // Clipboard formats we query but do not offer
    DROP_Hdrop,         // File Drop
    DROP_FNMA,          // File Name Map (Ansi)
    DROP_FNMW           // File Name Map (Unicode)
};

#define DROP_OFFERMIN           DROP_FGDW
#define DROP_OFFERMAX           DROP_Hdrop
#define DROP_MAX                (DROP_FNMW+1)

/*****************************************************************************
 *
 *      Global state management.
 *
 *      DLL reference count, DLL critical section.
 *
 *****************************************************************************/

void DllAddRef(void);
void DllRelease(void);

#ifdef UNICODE
#define EMPTYSTR_FOR_NULL EMPTYSTR_FOR_NULLW
#else // UNICODE
#define EMPTYSTR_FOR_NULL EMPTYSTR_FOR_NULLA
#endif // UNICODE

#define NULL_FOR_EMPTYSTR(str)          (((str) && (str)[0]) ? str : NULL)
#define EMPTYSTR_FOR_NULLA(str)          ((str) ? (str) : "")
#define EMPTYSTR_FOR_NULLW(str)          ((str) ? (str) : L"")

typedef void (*LISTPROC)(UINT flm, LPVOID pv);

/*****************************************************************************
 *      Local Includes
 *****************************************************************************/


class CFtpFolder;
class CFtpSite;
class CFtpList;
class CFtpDir;
class CFtpPidlList;
class CFtpMenu;
class CFtpDrop;
class CFtpEidl;
class CFtpObj;
class CFtpProp;
class CFtpView;
class CFtpIcon;
class CStatusBar;


#define FTP_SHCNE_EVENTS            (SHCNE_DISKEVENTS | SHCNE_ASSOCCHANGED | SHCNE_RMDIR | SHCNE_DELETE | SHCNE_MKDIR | SHCNE_CREATE | SHCNE_RENAMEFOLDER | SHCNE_RENAMEITEM | SHCNE_ATTRIBUTES)


typedef struct HINTPROCINFO {
    CFtpDir *   pfd;                    /* CFtpDir in which it's happening */
    HWND        hwnd;                   /* The window to use for UI */
    CStatusBar * psb;                   /* The status bar object. */
} HPI, *PHPI;

typedef HRESULT (*HINTPROC)(HINTERNET hint, PHPI phpi, LPVOID pv, BOOL * pfReleaseHint);

void GetCfBufA(UINT cf, PSTR psz, int cch);

// This is defined in WININET.CPP
typedef LPVOID HINTERNET;
typedef HGLOBAL HIDA;


typedef void (*DELAYEDACTIONPROC)(LPVOID);

typedef struct GLOBALTIMEOUTINFO GLOBALTIMEOUTINFO, * LPGLOBALTIMEOUTINFO;

struct GLOBALTIMEOUTINFO
{
    LPGLOBALTIMEOUTINFO     hgtiNext;
    LPGLOBALTIMEOUTINFO     hgtiPrev;
    LPGLOBALTIMEOUTINFO *   phgtiOwner;
    DWORD                   dwTrigger;
    DELAYEDACTIONPROC       pfn;        // Callback procedure
    LPVOID                  pvRef;      // Reference data for timer
};



#define INTERNET_MAX_PATH_LENGTH        2048
#define INTERNET_MAX_SCHEME_LENGTH      32          // longest protocol name length
#define MAX_URL_STRING                  (INTERNET_MAX_SCHEME_LENGTH \
                                        + sizeof("://") \
                                        + INTERNET_MAX_PATH_LENGTH)


#define LEAK_CFtpFolder                 0
#define LEAK_CFtpDir                    1
#define LEAK_CFtpSite                   2
#define LEAK_CFtpObj                    3
#define LEAK_CFtpEidl                   4
#define LEAK_CFtpDrop                   5
#define LEAK_CFtpList                   6
#define LEAK_CFtpStm                    7
#define LEAK_CAccount                   8
#define LEAK_CFtpFactory                9
#define LEAK_CFtpContextMenu            10
#define LEAK_CFtpEfe                    11
#define LEAK_CFtpGlob                   12
#define LEAK_CFtpIcon                   13
#define LEAK_CMallocItem                14
#define LEAK_CFtpPidlList               15
#define LEAK_CFtpProp                   16
#define LEAK_CStatusBar                 17
#define LEAK_CFtpView                   18
#define LEAK_CFtpWebView                19
#define LEAK_CCookieList                20
#define LEAK_CDropOperation             21


#ifdef DEBUG
typedef struct tagLEAKSTRUCT
{
    DWORD dwRef;
    CHAR szObject[MAX_PATH];
} LEAKSTRUCT;

extern LEAKSTRUCT g_LeakList[];
#define LEAK_ADDREF(dwIndex)    DEBUG_CODE(g_LeakList[dwIndex].dwRef++)
#define LEAK_DELREF(dwIndex)    DEBUG_CODE(g_LeakList[dwIndex].dwRef--)

#else // DEBUG
#define LEAK_ADDREF(dwIndex)    NULL
#define LEAK_DELREF(dwIndex)    NULL
#endif // DEBUG

/*****************************************************************************
 *
 *      ftpdhlp.c - DialogBox helpers
 *
 *****************************************************************************/

// Only update after 50k bytes were copied.
#define SIZE_PROGRESS_AFTERBYTES    50000

typedef struct tagPROGRESSINFO
{
    IProgressDialog * ppd;
    ULARGE_INTEGER uliBytesCompleted;
    ULARGE_INTEGER uliBytesTotal;
    DWORD dwCompletedInCurFile;
    DWORD dwLastDisplayed;              // We only display in chuncks of 50k, so what was the last chunk we displayed. Chunck #3 means between 100k and 150k.
    HINTERNET hint;                     // Used to cancel operation.
} PROGRESSINFO, * LPPROGRESSINFO;


typedef const BYTE *LPCBYTE;


typedef union FDI {
    struct {
        WORD    id;
        WORD    fdio;
    };
    DWORD dw;
} FDI, *PFDI;

typedef const FDI *PCFDI;

#define FDIO_ICON       0
#define FDIO_NAME       1
#define FDIO_TYPE       2
#define FDIO_LOCATION   3
#define FDIO_SIZE       4
#define FDIO_DATE       5
#define FDIO_COUNT      7
#define FDIO_CANMULTI   8

#define FDII_HFPL       0
#define FDII_WFDA       3

void FtpDlg_InitDlg(HWND hdlg, UINT id, CFtpFolder * pff, LPCVOID pv, UINT fdii);

#define FDI_FILEICON    { IDC_FILEICON, FDIO_ICON,      }
#define FDI_FILENAME    { IDC_FILENAME, FDIO_NAME,      }
#define FDI_FILETYPE    { IDC_FILETYPE, FDIO_TYPE,      }
#define FDI_LOCATION    { IDC_LOCATION, FDIO_LOCATION,  }
#define FDI_FILESIZE    { IDC_FILESIZE, FDIO_SIZE,      }
#define FDI_FILETIME    { IDC_FILETIME, FDIO_DATE,      }


#define imiTop          0
#define imiBottom       ((UINT)-1)

typedef void (*GLOBALTIMEOUTPROC)(LPVOID);


/*****************************************************************************\
   UNIX Chmod Bits
\*****************************************************************************/
#define UNIX_CHMOD_READ_OWNER               0x00000400
#define UNIX_CHMOD_WRITE_OWNER              0x00000200
#define UNIX_CHMOD_EXEC_OWNER               0x00000100
#define UNIX_CHMOD_READ_GROUP               0x00000040
#define UNIX_CHMOD_WRITE_GROUP              0x00000020
#define UNIX_CHMOD_EXEC_GROUP               0x00000010
#define UNIX_CHMOD_READ_ALL                 0x00000004
#define UNIX_CHMOD_WRITE_ALL                0x00000002
#define UNIX_CHMOD_EXEC_ALL                 0x00000001



/*****************************************************************************\
   STRINGS
\*****************************************************************************/
// BUGBUG: Fix the names
#define SZ_FTPURLA                          "ftp://"
#define SZ_EMPTYA                           ""
#define SZ_MESSAGE_FILEA                    "MESSAGE.TXT"
#define SZ_ALL_FILESA                       "*.*"
#define SZ_URL_SLASHA                       "/"
#define SZ_FTP_URL_TYPEA                    ";type="  // This is the section of the url that contains the download type.
#define SZ_ESCAPED_SPACEA                   "%20"
#define SZ_ESCAPED_SLASHA                   "%5c"
#define SZ_DOTA                             "."
#define SZ_ASTRICSA                         "*"
#define SZ_DOS_SLASHA                       "\\"
#define SZ_SPACEA                           " "
#define SZ_HASH_ENCODEDA                    "%23"

#define SZ_FTPURLW                          L"ftp://"
#define SZ_EMPTYW                           L""
#define SZ_MESSAGE_FILEW                    L"MESSAGE.TXT"
#define SZ_ALL_FILESW                       L"*.*"
#define SZ_URL_SLASHW                       L"/"
#define SZ_FTP_URL_TYPEW                    L";type="  // This is the section of the url that contains the download type.
#define SZ_ESCAPED_SPACEW                   L"%20"
#define SZ_ESCAPED_SLASHW                   L"%5c"
#define SZ_DOTW                             L"."
#define SZ_ASTRICSW                         L"*"
#define SZ_DOS_SLASHW                       L"\\"
#define SZ_SPACEW                           L" "
#define SZ_HASH_ENCODEDW                    L"%23"


#define SZ_FTPURL                           TEXT(SZ_FTPURLA)
#define SZ_EMPTY                            TEXT(SZ_EMPTYA)
#define SZ_MESSAGE_FILE                     TEXT(SZ_MESSAGE_FILEA)
#define SZ_ALL_FILES                        TEXT(SZ_ALL_FILESA)
#define SZ_URL_SLASH                        TEXT(SZ_URL_SLASHA)
#define SZ_FTP_URL_TYPE                     TEXT(SZ_FTP_URL_TYPEA)  // This is the section of the url that contains the download type.
#define SZ_ESCAPED_SPACE                    TEXT(SZ_ESCAPED_SPACEA)
#define SZ_ESCAPED_SLASH                    TEXT(SZ_ESCAPED_SLASHA)
#define SZ_DOT                              TEXT(SZ_DOTA)
#define SZ_ASTRICS                          TEXT(SZ_ASTRICSA)
#define SZ_DOS_SLASH                        TEXT(SZ_DOS_SLASHA)
#define SZ_SPACE                            TEXT(SZ_SPACEA)
#define SZ_HASH_ENCODED                     TEXT(SZ_HASH_ENCODEDA)

#define SZ_ANONYMOUS                        TEXT("anonymous")

#define CH_URL_SLASHA                       '\\'
#define CH_URL_URL_SLASHA                   '/'
#define CH_URL_LOGON_SEPARATORA             '@'
#define CH_URL_PASSWORD_SEPARATORA          ':'
#define CH_URL_TEMP_LOGON_SEPARATORA        '-'
#define CH_HASH_DECODEDA                    '#'

#define CH_URL_SLASHW                       L'\\'
#define CH_URL_URL_SLASHW                   L'/'
#define CH_URL_LOGON_SEPARATORW             L'@'
#define CH_URL_PASSWORD_SEPARATORW          L':'
#define CH_URL_TEMP_LOGON_SEPARATORW        L'-'
#define CH_HASH_DECODEDW                    L'#'

#define CH_URL_SLASH                        TEXT(CH_URL_SLASHA)
#define CH_URL_URL_SLASH                    TEXT(CH_URL_URL_SLASHA)
#define CH_URL_LOGON_SEPARATOR              TEXT(CH_URL_LOGON_SEPARATORA)
#define CH_URL_PASSWORD_SEPARATOR           TEXT(CH_URL_PASSWORD_SEPARATORA)
#define CH_URL_TEMP_LOGON_SEPARATOR         TEXT(CH_URL_TEMP_LOGON_SEPARATORA)
#define CH_HASH_DECODED                     TEXT(CH_HASH_DECODEDA)

// FTP Commands
#define FTP_CMD_SYSTEM                      "syst"
#define FTP_CMD_SITE                        "site"
#define FTP_CMD_SITE_HELP                   "site help"
#define FTP_CMD_SITE_CHMOD_TEMPL            "site chmod %lx %s"
#define FTP_CMD_FEAT                        "feat"
#define FTP_CMD_UTF8                        "opts utf8 on"
#define FTP_CMD_NO_OP                       "noop"

// UNIX Commands (Used via SITE FTP command)
#define FTP_UNIXCMD_CHMODA                  "chmod"

#define FTP_SYST_VMS                        "VMS "


/*****************************************************************************\
   REG Keys and Values
\*****************************************************************************/
// FTP Registry Keys
#define SZ_REGKEY_FTPCLASS                  TEXT("ftp")
#define SZ_REGKEY_MICROSOFTSOFTWARE         TEXT("Software\\Microsoft")
#define SZ_REGKEY_FTPFOLDER                 TEXT("Software\\Microsoft\\Ftp")
#define SZ_REGKEY_FTPFOLDER_ACCOUNTS        TEXT("Software\\Microsoft\\Ftp\\Accounts\\")
#define SZ_REGKEY_INTERNET_SETTINGS         TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings")
#define SZ_REGKEY_INTERNET_SETTINGS_LAN     TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\LAN")
#define SZ_REGKEY_INTERNET_EXPLORER         TEXT("Software\\Microsoft\\Internet Explorer")


// FTP Registry Values
#define SZ_REGVALUE_PASSWDSIN_ADDRBAR       TEXT("PasswordsInAddressBar")
#define SZ_REGVALUE_DOWNLOAD_DIR            TEXT("Download Directory")
#define SZ_REGVALUE_DOWNLOAD_TYPE           TEXT("Download Type")
#define SZ_REGVALUE_WARN_ABOUT_PROXY        TEXT("Warn About Proxy")
#define SZ_REGVALUE_DEFAULT_FTP_CLIENT      TEXT("ShellFolder")
#define SZ_REGVALUE_PREVIOUS_FTP_CLIENT     TEXT("Previous FTP Client")
#define SZ_REGVALUE_PROXY_SERVER            TEXT("ProxyServer")
#define SZ_REGVALUE_URL_ENCODING            TEXT("UrlEncoding")
#define SZ_REGVALUE_DISABLE_PASSWORD_CACHE  TEXT("DisablePasswordCaching")


// FTP Registry Data
#define SZ_REGDATA_IE_FTP_CLIENT            TEXT("{63da6ec0-2e98-11cf-8d82-444553540000}")

// Accounts
#define SZ_REGVALUE_DEFAULT_USER            TEXT("Default User")
#define SZ_REGVALUE_ACCOUNTNAME             TEXT("Name")
#define SZ_REGVALUE_PASSWORD                TEXT("Password")
#define SZ_ACCOUNT_PROP                     TEXT("CAccount_This")
#define SZ_REGKEY_LOGIN_ATTRIBS             TEXT("Login Attributes")
#define SZ_REGKEY_EMAIL_NAME                TEXT("EmailName")
#define SZ_REGKEY_USE_OLD_UI                TEXT("Use Web Based FTP")


/*****************************************************************************\
   FEATURES
\*****************************************************************************/
// Features
//#define FEATURE_DIALER
//#define FEATURE_OFFLINE

// Feature Tests
#define FEATURE_TEST_OFFLINE
#define FEATURE_TEST_DIALER

#define FEATURE_SAVE_PASSWORD
//#define FEATURE_PASSIVE_MODE
#define FEATURE_CHANGE_PERMISSIONS  // UNIX CHMOD command

#ifdef FEATURE_PASSIVE_MODE
#define FEATURE_PASSIVE_ON_OR_OFF     INTERNET_FLAG_PASSIVE
#else // FEATURE_PASSIVE_MODE
#define FEATURE_PASSIVE_ON_OR_OFF     0
#endif // FEATURE_PASSIVE_MODE

// #define FEATURE_FTP_TO_FTP_COPY
#define FEATURE_CUT_MOVE
//  #define ADD_ABOUTBOX


/*****************************************************************************\
   UNIQUE IDENTIFIERS
\*****************************************************************************/
// CLSIDs/IIDs Private to msieftp.dll
// WARNING: Never move these CLSID/IID definitions outside of the DLL.
//
// {299D0193-6DAA-11d2-B679-006097DF5BD4}
extern const GUID CLSID_FtpDataObject;
// {A11501B3-6EA4-11d2-B679-006097DF5BD4}
extern const GUID IID_CFtpFolder;


/*****************************************************************************\
   Local Includes
\*****************************************************************************/

class CFtpFolder;
class CFtpSite;
class CFtpStm;

#include "codepage.h"
#include "encoding.h"
#include "dllload.h"
#include "ftpdir.h"
#include "ftplist.h"
#include "ftppidl.h"
#include "ftpinet.h"
#include "ftppl.h"
#include "util.h"
#include "ftpapi.h"



/*****************************************************************************
 *      Object Constructors
 *****************************************************************************/

HRESULT CFtpFactory_Create(REFCLSID rclsid, REFIID riid, LPVOID * ppvObj);
HRESULT CFtpMenu_Create(CFtpFolder * pff, CFtpPidlList * pflHfpl, HWND hwnd, REFIID riid, LPVOID * ppvObj, BOOL fFromCreateViewObject);
HRESULT CFtpMenu_Create(CFtpFolder * pff, CFtpPidlList * pflHfpl, HWND hwnd, BOOL fFromCreateViewObject, CFtpMenu ** ppfm);
HRESULT CFtpDrop_Create(CFtpFolder * pff, HWND hwnd, CFtpDrop ** ppfm);
HRESULT CFtpEfe_Create(DWORD dwSize, FORMATETC rgfe[], STGMEDIUM rgstg[], CFtpObj * pfo, IEnumFORMATETC ** ppenum);
HRESULT CFtpEidl_Create(CFtpDir * pfd, CFtpFolder * pff, HWND hwndOwner, DWORD shcontf, IEnumIDList ** ppenum);
HRESULT CFtpFolder_Create(REFIID riid, LPVOID * ppvObj);
HRESULT CFtpInstaller_Create(REFIID riid, LPVOID * ppvObj);
IUnknown * CFtpGlob_Create(HGLOBAL hglob);
CFtpGlob * CFtpGlob_CreateStr(LPCTSTR pszStr);
HRESULT CFtpIcon_Create(CFtpFolder * pff, CFtpPidlList * pflHfpl, REFIID riid, LPVOID * ppv);
HRESULT CFtpIcon_Create(CFtpFolder * pff, CFtpPidlList * pflHfpl, CFtpIcon ** ppfm);
HRESULT CFtpList_Create(int cpvInit, PFNDPAENUMCALLBACK pfn, UINT nGrow, CFtpList ** ppfl);
HRESULT CFtpObj_Create(CFtpFolder * pff, CFtpPidlList * pflHfpl, REFIID riid, LPVOID * ppvObj);
HRESULT CFtpObj_Create(CFtpFolder * pff, CFtpPidlList * pflHfpl, CFtpObj ** ppfo);
HRESULT CFtpObj_Create(REFIID riid, void ** ppvObj);
HRESULT CFtpProp_Create(CFtpPidlList * pflHfpl, CFtpFolder * pff, HWND hwnd, CFtpProp ** ppfp);
HRESULT CFtpStm_Create(CFtpDir * pfd, LPCITEMIDLIST pidl, DWORD dwAccess, CFtpStm ** ppstream, ULARGE_INTEGER uliComplete, ULARGE_INTEGER uliTotal, IProgressDialog * ppd, BOOL fClosePrgDlg);
HRESULT CFtpView_Create(CFtpFolder * pff, HWND hwndOwner, CFtpView ** ppfv);
HRESULT CFtpSite_Create(LPCSTR pszSite, CFtpSite ** ppfs);
HRESULT CFtpPidlList_Create(int cpidl, LPCITEMIDLIST rgpidl[], CFtpPidlList ** ppfl);



void TriggerDelayedAction(LPGLOBALTIMEOUTINFO * phgti);
STDMETHODIMP SetDelayedAction(DELAYEDACTIONPROC pfn, LPVOID pvRef, LPGLOBALTIMEOUTINFO * phgti);
HRESULT PurgeDelayedActions(void);
BOOL AreOutstandingDelayedActions(void);

UINT FtpConfirmDeleteDialog(HWND hwnd, CFtpPidlList * pflHfpl, CFtpFolder * pff);
UINT FtpConfirmReplaceDialog(HWND hwnd, LPWIN32_FIND_DATA pwfdLocal, LPFTP_FIND_DATA pwfdRemote, int cobj, CFtpFolder * pff);
UINT FtpConfirmReplaceDialog(HWND hwnd, LPFTP_FIND_DATA pwfdLocal, LPWIN32_FIND_DATA pwfdRemote, int cobj, CFtpFolder * pff);

CFtpView * GetCFtpViewFromDefViewSite(IUnknown * punkSite);

/*****************************************************************************\
    FTP Notes and Issues

    Time/Date Stamps on FTP Files:
        There are two issues, the date on the server isn't time zone independent,
    and FTP/Wininet only do LastWriteTime (not CreationTime & LastAccessedTime).
    1. Corrdinated Universal Time (UTC) is the format to use to store time in
       a time zone indepndent manner.  Time displayed to the user then gets
       converted at the last moment so it's correct wrt the time zone.  FindFirstFile()
       does fill in the WIN32_FIND_DATA structure with UTC time, but wininet doesn't.
       This means that this FTP code can't do the UTC->LocalTime convertion when
       displaying the date/time.
    2. Only the modified date is sent via FTP.  Oh well, not to sever.

    Threading Issues:
        FTP can be called from several threads:
    1. Main UI Thread: CFtpFolder, ....
    2. Folder Contents Enum: CFtpFolder, IEnumIDList, CFtpDir, CFtpSite.
    2. AutoComplete Enum: CFtpFolder, IEnumIDList, CFtpDir, CFtpSite.
    BUGBUG/TODO: Go thru all of the code and make sure it's thread safe
                 and add asserts to assert which classes are single threaded
                 and which need to be thread safe.

    PERF:
        1. We only cache an FTP session (InternetConnect()) for 10 seconds.  We should
           increase this to 50sec to 90 or maybe 120sec. This is currently done because
           we need to force the session to close down early when we are going to shutdown
           the process or try to unload our dll.
        2. Our Tree Walker code for recusrive operations (upload, download, delete) (in ftppl.cpp)
           calls FtpGetCurrentDirectory() and FtpSetCurrentDirectory() too often. To fix
           this, we need to verify we don't recurse into SOFTLINK directories because
           this is what requires the FtpGetCurrentDirectory.  This also could cause a
           recursion death bug and infinite loop.  We would then create
           ::_CacheChangeDir() and ::_CommitChangeDir() so the could remain
           simple but we would cache several Change Directories.
        3. If the desktop or desktop toolbars have FTP Shortcuts, the icon is
           extracted in such a way that we hit the net.  This figgen sucks!!!
           Fix this, even if this mean hacking.
\*****************************************************************************/


// WARNING: Don't put any #defines or typedefs after the includes or they won't be recognized.

#endif // _PRIV_H_
