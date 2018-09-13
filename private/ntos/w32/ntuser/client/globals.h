/****************************** Module Header ******************************\
* Module Name: globals.h
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains all of USER.DLL's global variables.  These are all
* instance-specific, i.e. each client has his own copy of these.  In general,
* there shouldn't be much reason to create instance globals.
*
* History:
* 10-18-90 DarrinM      Created.
\***************************************************************************/

#ifndef _GLOBALS_
#define _GLOBALS_

// Debug globals
#if DBG
extern INT gbCheckHandleLevel;
#endif

extern CONST ALWAYSZERO gZero;

extern int gcWheelDelta;

extern WORD gDispatchTableValues;

extern WCHAR awchSlashStar[];
extern CHAR achSlashStar[];

extern PSERVERINFO gpsi;
extern SHAREDINFO gSharedInfo;

extern HMODULE hmodUser;            // USER.DLL's hmodule
extern ULONG_PTR gHighestUserAddress;

extern BOOL gfServerProcess;        // USER is linked on the CSR server side.
extern BOOL gfSystemInitialized;    // System has been initialized
extern ACCESS_MASK gamWinSta;       // ACCESS_MASK for the current WindowStation

extern PVOID pUserHeap;

extern CONST CFNSCSENDMESSAGE gapfnScSendMessage[];

extern WCHAR szUSER32[];
extern CONST WCHAR szNull[];
extern CONST WCHAR szOneChar[];
extern WCHAR szSLASHSTARDOTSTAR[];

extern CONST BYTE mpTypeCcmd[];
extern CONST BYTE mpTypeIich[];
extern CONST UINT SEBbuttons[];
extern CONST BYTE rgReturn[];

extern ATOM atomMDIActivateProp;
extern ATOM gatomReaderMode;

extern CRITICAL_SECTION gcsLookaside;
extern CRITICAL_SECTION gcsHdc;
extern CRITICAL_SECTION gcsClipboard;
extern CRITICAL_SECTION gcsAccelCache;

#ifdef _JANUS_
extern BOOL gfEMIEnable;
extern DWORD gdwEMIControl;
extern BOOL gfDMREnable;
extern HINSTANCE ghAdvApi;
#endif


extern HDC    ghdcBits2;
extern HDC    ghdcGray;
extern HFONT  ghFontSys;
extern HBRUSH ghbrWindowText;
extern int    gcxGray;
extern int    gcyGray;

extern LPWSTR pwcHighContrastScheme;
extern LPSTR   pcHighContrastScheme;


/*
 * LATER: client-side user needs to use moveable memory objects for
 * WOW compatibility (at least until/if/when we copy all the edit control
 * code into 16-bit space);  that's also why we can't just party with
 * handles like LMHtoP does... -JeffPar
 */
#ifndef RC_INVOKED       // RC can't handle #pragmas
#undef  LHND
#define LHND                (LMEM_MOVEABLE | LMEM_ZEROINIT)

#undef  LMHtoP
#define LMHtoP(handle)      // Don't use this macro
#endif


/*
 * WOW HACK - apps can pass a global handle as the hInstance on a call
 * to CreateWindow for an edit control and expect allocations for the
 * control to come out of that global block. (MSJ 1/91 p.122)
 * WOW needs this hInstance during the LocalAlloc callback to set up
 * the DS for the LocalAlloc, so we pass hInstance as an 'extra' parameter.
 * !!! this is dependent on calling convention !!!
 * (SAS 6-18-92) added hack for all macros
 */

#define LOCALALLOC(dwFlags, dwBytes, hInstance)         \
                            (*pfnLocalAlloc)(dwFlags, dwBytes, hInstance)
#define LOCALREALLOC(hMem, dwBytes, dwFlags, hInstance, ppv) \
                            (*pfnLocalReAlloc)(hMem, dwBytes, dwFlags, hInstance, ppv)
#define LOCALLOCK(hMem, hInstance)                      \
                            (*pfnLocalLock)(hMem, hInstance)
#define LOCALUNLOCK(hMem, hInstance)                    \
                            (*pfnLocalUnlock)(hMem, hInstance)
#define LOCALSIZE(hMem, hInstance)                      \
                            (*pfnLocalSize)(hMem, hInstance)
#define LOCALFREE(hMem, hInstance)                      \
                            (*pfnLocalFree)(hMem, hInstance)

extern PFNFINDA             pfnFindResourceExA;
extern PFNFINDW             pfnFindResourceExW;
extern PFNLOAD              pfnLoadResource;
extern PFNLOCK              pfnLockResource;
extern PFNUNLOCK            pfnUnlockResource;
extern PFNFREE              pfnFreeResource;
extern PFNSIZEOF            pfnSizeofResource;
extern PFNLALLOC            pfnLocalAlloc;
extern PFNLREALLOC          pfnLocalReAlloc;
extern PFNLLOCK             pfnLocalLock;
extern PFNLUNLOCK           pfnLocalUnlock;
extern PFNLSIZE             pfnLocalSize;
extern PFNLFREE             pfnLocalFree;
extern PFNGETEXPWINVER      pfnGetExpWinVer;
extern PFN16GALLOC          pfn16GlobalAlloc;
extern PFN16GFREE           pfn16GlobalFree;
extern PFNGETMODFNAME       pfnGetModFileName;
extern PFNEMPTYCB           pfnWowEmptyClipBoard;
extern PFNWOWWNDPROCEX      pfnWowWndProcEx;
extern PFNWOWDLGPROCEX      pfnWowDlgProcEx;
extern PFNWOWEDITNEXTWORD   pfnWowEditNextWord;
extern PFNWOWCBSTOREHANDLE  pfnWowCBStoreHandle;
extern PFNGETPROCMODULE16   pfnWowGetProcModule;
extern PFNWOWTELLWOWTHEHDLG pfnWOWTellWOWThehDlg;
extern PFNWOWMSGBOXINDIRECTCALLBACK pfnWowMsgBoxIndirectCallback;
extern PFNWOWILSTRCMP       pfnWowIlstrcmp;

extern UNICODE_STRING strRootDirectory;

#ifdef WX86
/*
 *  Client Global variables for Wx86.
 *
 */

extern int Wx86LoadCount;
extern HMODULE hWx86Dll;
extern PFNWX86LOADX86DLL pfnWx86LoadX86Dll;
extern PFNWX86FREEX86DLL pfnWx86FreeX86Dll;
extern PFNWX86HOOKCALLBACK pfnWx86HookCallBack;
extern RTL_CRITICAL_SECTION gcsWx86Load;

#endif

/*
 * Menu Drag and Drop
 */
extern HINSTANCE ghinstOLE;
extern FARPROC gpfnOLEOleUninitialize;
extern FARPROC gpfnOLERegisterDD;
extern FARPROC gpfnOLERevokeDD;
extern FARPROC gpfnOLEDoDD;

/*
 * Accelerator table resources list.
 */
extern PACCELCACHE gpac;

/*
 * IME Window Handling.
 */
extern DWORD gfConIme;
#define UNKNOWN_CONIME ~0

extern UNICODE_STRING strRootDirectory;

/*
 * Used for TS Services Message Box handling
 */
extern FARPROC      gfnWinStationSendMessageW;
extern HINSTANCE    ghinstWinStaDll;

#endif // ndef _GLOBALS_
