#ifndef _PRIV_H_
#define _PRIV_H_



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
#include <exdisp.h>
#include <objidl.h>

#include <shlwapi.h>

#include <shellapi.h>

#include <shsemip.h>
#include <crtfree.h>

#include <ole2ver.h>
#include <olectl.h>
#include <shellp.h>
#include <shdocvw.h>
#include <shguidp.h>
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

#include <commctrl.h>


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
#define TF_FTP_PERF         0x00200000      // Perf


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


//
// we may not be part of the namespace on IE3/Win95
//
typedef BOOL (WINAPI *PFNILISEQUAL)(LPCITEMIDLIST, LPCITEMIDLIST);


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

#define OBJAT(T, v) (*(T *)(v))         /* Pointer punning */
#define _PASTE(a,b) a##b                /* So you can paste outside macros */
#define PASTE(a,b) _PASTE(a,b)


/*
 * Convert an array name (A) to a generic count (c).
 */
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

/*
 * Convert an array name (A) to a pointer to its Max.
 * (I.e., one past the last element.)
 */
#define pvMaxA(a) (&a[ARRAYSIZE(a)])

#define pvSubPvCb(pv, cb) ((LPVOID)((PBYTE)pv - (cb)))
#define pvAddPvCb(pv, cb) ((LPVOID)((PBYTE)pv + (cb)))
#define cbSubPvPv(p1, p2) ((PBYTE)(p1) - (PBYTE)(p2))


/*
 * lfNeVV
 *
 * Given two values, return zero if they are equal and nonzero if they
 * are different.  This is the same as (v1) != (v2), except that the
 * return value on unequal is a random nonzero value instead of 1.
 * (lf = logical flag)
 *
 * lfNePvPv
 *
 * The same as lfNeVV, but for pointers.
 *
 * lfPv
 *
 * Nonzero if pv is not null.
 *
 */
#define lfNeVV(v1, v2) ((v1) - (v2))
#define lfNePvPv(v1, v2) lfNeVV((DWORD)(LPVOID)(v1), (DWORD)(LPVOID)(v2))


/*
 * InOrder - checks that i1 <= i2 < i3.
 */
#define fInOrder(i1, i2, i3) ((unsigned)((i2)-(i1)) < (unsigned)((i3)-(i1)))

/*
 * CopyPvPvCb - Copy some memory around
 * MovePvPvCb - Move some memory around
 */
#define CopyPvPvCb RtlCopyMemory
#define MovePvPvCb RtlMoveMemory

/*
 * memeq - Reverse of memcmp
 */
#define memeq !memcmp

#ifdef DEBUG
#define DEBUG_CODE(x)            x
#else // DEBUG
#define DEBUG_CODE(x)
#endif // DEBUG

#define comma           ,

#undef lstrcatnW

// #define lstrcpyW            BUGBUG_BAD_lstrcpyW
#define lstrcpynW           BUGBUG_BAD_lstrcpynW
#define lstrcmpW            BUGBUG_BAD_lstrcmpW
#define lstrcmpiW           BUGBUG_BAD_lstrcmpiW
#define lstrcatW            BUGBUG_BAD_lstrcatW
#define lstrcatnW           BUGBUG_BAD_lstrcatnW


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

extern HINSTANCE                g_hinst;              /* My instance handle */
extern HANDLE                   g_hthWorker;             /* Background worker thread */
DEFINE_GUID(CLSID_FtpFolder, 0x63da6ec0, 0x2e98, 0x11cf, 0x8d,0x82,0x44,0x45,0x53,0x54,0,0);


// Detect "." or ".." as invalid files
#define IS_VALID_FILE(str)        (!(('.' == str[0]) && (('\0' == str[1]) || (('.' == str[1]) && ('\0' == str[2])))))

#define Gobble(x)               /* eats its argument */
extern HINSTANCE g_hinst;
#define HINST_THISDLL g_hinst


/*****************************************************************************
 *
 *      Global state management.
 *
 *      DLL reference count, DLL critical section.
 *
 *****************************************************************************/

void DllAddRef(void);
void DllRelease(void);


#define NULL_FOR_EMPTYSTR(str)          (((str) && (str)[0]) ? str : NULL)

typedef void (*LISTPROC)(UINT flm, LPVOID pv);

/*****************************************************************************
 *      Local Includes
 *****************************************************************************/





typedef unsigned __int64 QWORD, * LPQWORD;

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
    DELAYEDACTIONPROC       pfn;	/* Callback procedure */
    LPVOID                  pvRef;		/* Reference data for timer */
};



#define QW_MAC              0xFFFFFFFFFFFFFFFF

#define INTERNET_MAX_PATH_LENGTH        2048
#define INTERNET_MAX_SCHEME_LENGTH      32          // longest protocol name length
#define MAX_URL_STRING                  (INTERNET_MAX_SCHEME_LENGTH \
                                        + sizeof("://") \
                                        + INTERNET_MAX_PATH_LENGTH)


/*****************************************************************************
 *
 *      ftpdhlp.c - DialogBox helpers
 *
 *****************************************************************************/

typedef const BYTE *LPCBYTE;

typedef struct FTPLISTINFO
{
    LISTPROC		lp;		/* Who you gonna call? */
    UINT		cpvGrow;	/* Growth increment */
} FTPLISTINFO, * LPFTPLISTINFO;
typedef const FTPLISTINFO * LPCFTPLISTINFO;


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


#define FDI_FILEICON    { IDC_FILEICON, FDIO_ICON,      }
#define FDI_FILENAME    { IDC_FILENAME, FDIO_NAME,      }
#define FDI_FILETYPE    { IDC_FILETYPE, FDIO_TYPE,      }
#define FDI_LOCATION    { IDC_LOCATION, FDIO_LOCATION,  }
#define FDI_FILESIZE    { IDC_FILESIZE, FDIO_SIZE,      }
#define FDI_FILETIME    { IDC_FILETIME, FDIO_DATE,      }


#define imiTop          0
#define imiBottom       ((UINT)-1)

typedef void (*GLOBALTIMEOUTPROC)(LPVOID);

// FTP Strings
#define SZ_FTPURL                           TEXT("ftp://")
#define SZ_EMPTY                            TEXT("")
#define SZ_MESSAGE_FILE                     TEXT("MESSAGE.TXT")
#define SZ_ALL_FILES                        TEXT("*.*")
#define SZ_URL_SLASH                        TEXT("/")
#define SZ_FTP_URL_TYPE                     TEXT(";type=")  // This is the section of the url that contains the download type.
#define SZ_ESCAPED_SPACE                    TEXT("%20")
#define SZ_ESCAPED_SLASH                    TEXT("%5c")
#define SZ_DOT                              TEXT(".")
#define SZ_ASTRICS                          TEXT("*")
#define SZ_DOS_SLASH                        TEXT("\\")
#define SZ_SPACE                            TEXT(" ")

#define SZ_ANONYMOUS                        TEXT("anonymous")

#define CH_URL_SLASH                        TEXT('\\')
#define CH_URL_URL_SLASH                    TEXT('/')
#define CH_URL_LOGON_SEPARATOR              TEXT('@')
#define CH_URL_PASSWORD_SEPARATOR           TEXT(':')
#define CH_URL_TEMP_LOGON_SEPARATOR         TEXT('-')


// FTP Registry Keys
#define SZ_REGKEY_FTPFOLDER                 TEXT("Software\\Microsoft\\Ftp")
#define SZ_REGKEY_FTPFOLDER_ACCOUNTS        TEXT("Software\\Microsoft\\Ftp\\Accounts\\")
#define SZ_REGKEY_INTERNET_SETTINGS         TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings")
#define SZ_REGKEY_INTERNET_EXPLORER         TEXT("Software\\Microsoft\\Internet Explorer")


// FTP Registry Values
#define SZ_REGVALUE_PASSWDSIN_ADDRBAR       TEXT("PasswordsInAddressBar")
#define SZ_REGVALUE_DOWNLOAD_DIR            TEXT("Download Directory")
#define SZ_REGVALUE_DOWNLOAD_TYPE           TEXT("Download Type")

// Accounts
#define SZ_REGVALUE_DEFAULT_USER            TEXT("Default User")
#define SZ_REGVALUE_ACCOUNTNAME             TEXT("Name")
#define SZ_REGVALUE_PASSWORD                TEXT("Password")
#define SZ_ACCOUNT_PROP                     TEXT("CAccount_This")
#define SZ_REGKEY_LOGIN_ATTRIBS             TEXT("Login Attributes")
#define SZ_REGKEY_EMAIL_NAME                TEXT("EmailName")



/*****************************************************************************
 *      Local Includes
 *****************************************************************************/

#include "dllload.h"
#include "util.h"



/*****************************************************************************
 *      Object Constructors
 *****************************************************************************/

HRESULT CFtpFactory_Create(REFCLSID rclsid, REFIID riid, LPVOID * ppvObj);
//HRESULT CFtpMenu_Create(CFtpFolder * pff, CFtpPidlList * pflHfpl, HWND hwnd, REFIID riid, LPVOID * ppvObj);



#endif // _PRIV_H_
