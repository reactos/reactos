//
// brfprv.h:  Includes all files that are to be part of the precompiled
//             header.
//

#ifndef __BRFPRV_H__
#define __BRFPRV_H__

/////////////////////////////////////////////////////  INCLUDES

#define NEW_REC

#define STRICT
#define NOWINDOWSX
#define NOSHELLDEBUG
//#define NO_COMMCTRL_DA
#define NO_COMMCTRL_ALLOCFCNS
#define USE_MONIKER

#define _INC_OLE            // WIN32

#include <windows.h>
#include <windowsx.h>

#include <shellapi.h>       // for registration functions
#include <port32.h>

#include <shlobj.h>         // WIN32
#include <shlobjp.h>
#include <shlapip.h>
#include <shsemip.h>
#include <winuserp.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <comctrlp.h>
#include <ccstock.h>

#ifdef CbFromCch
#undef CbFromCch
#endif // CbFromCch

#ifdef ZeroInit
#undef ZeroInit
#endif // ZeroInit

#ifdef InRange
#undef InRange
#endif // InRange

#include <ole2.h>           // object binding

// Internal Shell headers
#include <shellp.h>
#include <brfcasep.h>

#include <prsht.h>          // Property sheet stuff

#include <synceng.h>        // Twin Engine include file
#include <indirect.h>       // For type-safe indirect calling

#define PUBLIC
#define CPUBLIC          _cdecl
#define PRIVATE

#define MAXBUFLEN       260
#define MAXMSGLEN       520
#define MAXMEDLEN       64
#define MAXSHORTLEN     32

#define NULL_CHAR       TEXT('\0')

#define HIDWORD(_qw)    (DWORD)((_qw)>>32)
#define LODWORD(_qw)    (DWORD)(_qw)

#define DPA_ERR         (-1)

#define CRL_FLAGS       CRL_FL_DELETE_DELETED_TWINS

//---------------------------------------------------------------------------
// misc.c
//---------------------------------------------------------------------------

// Structure for handling abort events
typedef struct
    {
    UINT    uFlags;
    } ABORTEVT, * PABORTEVT;

// Flags for ABORTEVT struct
#define AEF_DEFAULT     0x0000
#define AEF_SHARED      0x0001
#define AEF_ABORT       0x0002

BOOL PUBLIC AbortEvt_Create(PABORTEVT * ppabortevt, UINT uFlags);
void PUBLIC AbortEvt_Free(PABORTEVT this);
BOOL PUBLIC AbortEvt_Set(PABORTEVT this, BOOL bAbort);
BOOL PUBLIC AbortEvt_Query(PABORTEVT this);


// Structure for the update progress bar
typedef struct
    {
    UINT uFlags;
    PABORTEVT pabortevt;
    HWND hwndParent;
    DWORD dwTickShow;   // Tick count at which to display dialog
    HCURSOR hcurSav;

    } UPDBAR, * PUPDBAR;      // Update progress bar struct

HWND PUBLIC UpdBar_Show (HWND hwndParent, UINT uFlags, UINT nSecs);
void PUBLIC UpdBar_Kill (HWND hdlg);
void PUBLIC UpdBar_SetCount (HWND hdlg, ULONG ulcFiles);
void PUBLIC UpdBar_SetRange (HWND hdlg, WORD wRangeMax);
void PUBLIC UpdBar_DeltaPos (HWND hdlg, WORD wdelta);
void PUBLIC UpdBar_SetPos(HWND hdlg, WORD wPos);
void PUBLIC UpdBar_SetName (HWND hdlg, LPCTSTR lpcszName);
void PUBLIC UpdBar_SetDescription(HWND hdlg, LPCTSTR psz);
void PUBLIC UpdBar_SetAvi(HWND hdlg, UINT uFlags);
HWND PUBLIC UpdBar_GetStatusWindow(HWND hdlg);
PABORTEVT PUBLIC UpdBar_GetAbortEvt(HWND hdlg);
BOOL PUBLIC UpdBar_QueryAbort (HWND hdlg);

#define DELAY_UPDBAR    3       // 3 seconds

// Flags for UpdBar_Show
#define UB_UPDATEAVI    0x0001
#define UB_CHECKAVI     0x0002
#define UB_NOSHOW       0x0004
#define UB_NOCANCEL     0x0008
#define UB_TIMER        0x0010
#define UB_NOPROGRESS   0x0020

#define UB_CHECKING     (UB_CHECKAVI | UB_NOPROGRESS | UB_TIMER)
#define UB_UPDATING     UB_UPDATEAVI


// Additional MB_ flags
#define MB_WARNING  (MB_OK | MB_ICONWARNING)
#define MB_INFO     (MB_OK | MB_ICONINFORMATION)
#define MB_ERROR    (MB_OK | MB_ICONERROR)
#define MB_QUESTION (MB_YESNO | MB_ICONQUESTION)

int PUBLIC MsgBox(HWND hwndParent, LPCTSTR pszText, LPCTSTR pszCaption, HICON hicon, UINT uStyle, ...);

int PUBLIC ConfirmReplace_DoModal(HWND hwndOwner, LPCTSTR pszPathExisting, LPCTSTR pszPathOther, UINT uFlags);

// Flags for ConfirmReplace_DoModal
#define CRF_DEFAULT     0x0000
#define CRF_MULTI       0x0001
#define CRF_FOLDER      0x0002      // Internal

int PUBLIC Intro_DoModal(HWND hwndParent);


//---------------------------------------------------------------------------
// Local includes
//---------------------------------------------------------------------------

#include "mem.h"            // Shared heap functions
#include "da.h"             // Dynamic array functions
#include "cstrings.h"       // Read-only string constants
#include "init.h"           // Global DLL and initialization
#include "strings.h"        // Private string include
#include "comm.h"           // Common functions
#include "err.h"            // Error/debug code
#include "twin.h"           // Engine specific macros
#include "cache.h"          // Cache functions
#include "atoms.h"          // Atom functions

//---------------------------------------------------------------------------
// Critical section stuff
//---------------------------------------------------------------------------

// Notes:
//  1. Never "return" from the critical section.
//  2. Never "SendMessage" or "Yield" from the critical section.
//  3. Never call USER API which may yield.
//  4. Always make the critical section as small as possible.
//

void PUBLIC Brief_EnterExclusive(void);
void PUBLIC Brief_LeaveExclusive(void);
extern UINT g_cRefSyncUI;

#define ENTEREXCLUSIVE()    Brief_EnterExclusive();
#define LEAVEEXCLUSIVE()    Brief_LeaveExclusive();
#define ASSERTEXCLUSIVE()       ASSERT(0 < g_cRefSyncUI)
#define ASSERT_NOT_EXCLUSIVE()  ASSERT(0 == g_cRefSyncUI)

UINT PUBLIC Delay_Own(void);
UINT PUBLIC Delay_Release(void);


//---------------------------------------------------------------------------
// IDataObject prototypes
//---------------------------------------------------------------------------

BOOL    PUBLIC DataObj_KnowsBriefObj(LPDATAOBJECT pdtobj);
HRESULT PUBLIC DataObj_QueryBriefPath(LPDATAOBJECT pdtobj, LPTSTR pszBriefPath);
HRESULT PUBLIC DataObj_QueryPath(LPDATAOBJECT pdtobj, LPTSTR pszPath);
HRESULT PUBLIC DataObj_QueryFileList(LPDATAOBJECT pdtobj, LPTSTR * ppszList, LPUINT puCount);
void    PUBLIC DataObj_FreeList(LPTSTR pszList);

// Helper macros
#define NextString(psz)             while (*(psz)++)
#define DataObj_NextFile(psz)       NextString(psz)

//---------------------------------------------------------------------------
// path.c
//---------------------------------------------------------------------------

// Events for PathNotifyShell
typedef enum _notifyshellevent
    {
    NSE_CREATE       = 0,
    NSE_MKDIR,
    NSE_UPDATEITEM,
    NSE_UPDATEDIR
    } NOTIFYSHELLEVENT;

LPTSTR   PUBLIC MyPathRemoveBackslash(LPTSTR lpszPath);
LPTSTR   PUBLIC PathRemoveExt(LPCTSTR pszPath, LPTSTR pszBuf);
void    PUBLIC PathMakePresentable(LPTSTR pszPath);
LPTSTR   PUBLIC PathGetDisplayName(LPCTSTR pszPath, LPTSTR pszBuf);
void    PUBLIC BrfPathCanonicalize(LPCTSTR pszPath, LPTSTR pszBuf);
BOOL    PUBLIC PathCheckForBriefcase(LPCTSTR pszPath, DWORD dwAttrib);
BOOL    PUBLIC PathIsBriefcase(LPCTSTR pszPath);
BOOL    PUBLIC PathExists(LPCTSTR pszPath);
UINT    PUBLIC PathGetLocality(LPCTSTR pszPath, LPTSTR pszBuf);
void    PUBLIC PathNotifyShell(LPCTSTR pszPath, NOTIFYSHELLEVENT nse, BOOL bDoNow);
LPCTSTR  PUBLIC PathFindEndOfRoot(LPCTSTR pszPath);
LPTSTR   PUBLIC PathFindNextComponentI(LPCTSTR lpszPath);
BOOL    PUBLIC PathsTooLong(LPCTSTR pszFolder, LPCTSTR pszName);

// Path locality values, relative to a briefcase
//
#define PL_FALSE   0       // path is not related at all to a briefcase
#define PL_ROOT    1       // path directly references the root of a briefcase
#define PL_INSIDE  2       // path is somewhere inside a briefcase

//---------------------------------------------------------------------------
// state.c
//---------------------------------------------------------------------------

#ifdef DEBUG

BOOL PUBLIC ProcessIniFile(void);
BOOL PUBLIC CommitIniFile(void);

#else

#define ProcessIniFile()
#define CommitIniFile()

#endif

//---------------------------------------------------------------------------
// oledup.c
//---------------------------------------------------------------------------

HRESULT MyReleaseStgMedium(LPSTGMEDIUM pmedium);

//---------------------------------------------------------------------------
// thread.c
//---------------------------------------------------------------------------

BOOL PUBLIC RunDLLThread(HWND hwnd, LPCTSTR pszCmdLine, int nCmdShow);


//---------------------------------------------------------------------------
// ibrfext.c
//---------------------------------------------------------------------------

// This structure shares common data between all briefcase
// property pages
typedef struct tagPAGEDATA
    {
    LPBRIEFCASESTG  pbrfstg;        // IBriefcaseStg instance
    int             atomPath;
    PCBS            pcbs;
    UINT            cRef;

    PRECLIST        prl;
    PFOLDERTWINLIST pftl;
    BOOL            bOrphan:1;      // TRUE: This is an orphan
    BOOL            bFolder:1;      // TRUE: This is a folder
    BOOL            bRecalc:1;      // TRUE: Need to recalc

    LPARAM          lParam;         // Page-specific data
    } PAGEDATA, * PPAGEDATA;

HRESULT PUBLIC PageData_Init(PPAGEDATA this, HWND hwndOwner);
HRESULT PUBLIC PageData_Query(PPAGEDATA this, HWND hwndOwner, PRECLIST * pprl, PFOLDERTWINLIST * ppftl);
void    PUBLIC PageData_Orphanize(PPAGEDATA this);

#define PageData_GetHbrf(this)      ((this)->pcbs->hbrf)

HRESULT CALLBACK BriefExt_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, LPVOID * ppvOut);


//---------------------------------------------------------------------------
// status.c
//---------------------------------------------------------------------------

INT_PTR _export CALLBACK Stat_WrapperProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void PUBLIC SelectItemInCabinet(HWND hwndCabinet, LPCITEMIDLIST pidl, BOOL bEdit);
void PUBLIC OpenCabinet(HWND hwnd, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidl, BOOL bEdit);


//---------------------------------------------------------------------------
// info.c
//---------------------------------------------------------------------------

typedef struct
    {
    int     atomTo;
    HDPA    hdpaTwins;    // handle to array of twin handles which will
                          //  be filled by dialog.
                          //  N.b.  Caller must release these twins!
    BOOL    bStandAlone;  // private: should only be set by Info_DoModal
    } INFODATA, * PINFODATA;

INT_PTR _export CALLBACK Info_WrapperProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

HRESULT PUBLIC Info_DoModal(HWND hwndParent, LPCTSTR pszPath1, LPCTSTR pszPath2, HDPA hdpaTwin, PCBS pcbs);


//---------------------------------------------------------------------------
// ibrfstg.c
//---------------------------------------------------------------------------

void    PUBLIC TermCacheTables(void);
HRESULT CALLBACK BriefStg_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, LPVOID * ppvOut);

//---------------------------------------------------------------------------
// update.c
//---------------------------------------------------------------------------

// Flags for Upd_DoModal
#define UF_SELECTION    0x0001
#define UF_ALL          0x0002

HRESULT PUBLIC Upd_DoModal(HWND hwndOwner, CBS * pcbs, LPCTSTR pszList, UINT cFiles, UINT uFlags);

//---------------------------------------------------------------------------
// init.c
//---------------------------------------------------------------------------

LPSHELLFOLDER PUBLIC GetDesktopShellFolder(void);


//---------------------------------------------------------------------------
// Semaphores
//---------------------------------------------------------------------------

// The BusySemaphore is used only for areas of code that do not
// rely on the sync engine v-table.  One example is the IContextMenu
// code.
//
// The BriefSemaphore is used for any code that opens/closes a
// Briefcase storage interface.
//
// These must be serialized.

extern UINT g_cBusyRef;            // Semaphore
extern UINT g_cBriefRef;           // Semaphore

#define IsBusySemaphore()       (g_cBusyRef > 0)
#define IncBusySemaphore()      (g_cBusyRef++)
#define DecBusySemaphore()      (g_cBusyRef--)

#define IncBriefSemaphore()     (g_cBriefRef++)
#define DecBriefSemaphore()     (g_cBriefRef--)
#define IsOpenBriefSemaphore()  (g_cBriefRef > 0)
#define IsFirstBriefSemaphore() (g_cBriefRef == 1)
#define IsLastBriefSemaphore()  (g_cBriefRef == 0)

#endif  //!__BRFPRV_H__
