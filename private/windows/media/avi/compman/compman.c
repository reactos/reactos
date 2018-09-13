//////////////////////////////////////////////////////////////////////////////o
//
//  Compman.c
//
//      Manager routines for compressing/decompressing/and choosing compressors.
//
//      (C) Copyright Microsoft Corp. 1991-1995.  All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////

/*
 * This code contains 16 thunk code for NT. If the 16 bit open fails
 * we will try and open a 32 bit codec.  (The reason for not trying the 32
 * bit codec first is an attempt to keep most things on the 16 bit side.
 * The performance under NT appears reasonable, and for frame specific
 * operations it reduces the number of 16/32 transitions.
 */



#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <win32.h>

#ifdef _WIN32
 #include <mmddk.h>  // needed for definition of DRIVERS_SECTION
BOOL IsAdmin(void);
#endif

#ifdef NT_THUNK16
#include "thunks.h"    // Define the thunk stuff
#endif

#ifdef _WIN32
#ifdef DEBUGLOAD
#define ICEnterCrit(p)  \
		    if (!(gdwLoadFlags & ICLOAD_CALLED)) {  \
			OutputDebugStringA("ICOPEN Crit Sec not setup (ENTER)\n"); \
			DebugBreak(); \
		    }                 \
		    (EnterCriticalSection(p))

#define ICLeaveCrit(p)  \
		    if (!(gdwLoadFlags & ICLOAD_CALLED)) {  \
			OutputDebugStringA("ICOPEN Crit Sec not setup (LEAVE)\n"); \
			DebugBreak(); \
		    }                 \
		    (LeaveCriticalSection(p))

#else

#define ICEnterCrit(p)  (EnterCriticalSection(p))
#define ICLeaveCrit(p)  (LeaveCriticalSection(p))
#endif

#else

// non-win32 code has no critsecs
#define ICEnterCrit(p)
#define ICLeaveCrit(p)

#endif

#include <profile.h>

//
// define these before compman.h, so our functions get declared right.
//
#ifndef _WIN32
#define VFWAPI  FAR PASCAL _loadds
#define VFWAPIV FAR CDECL  _loadds
#endif

#include <vfw.h>
#include "icm.rc"

#ifndef _WIN32
#define LoadLibraryA    LoadLibrary
#define CharLowerA      AnsiLower
#endif

#ifndef streamtypeVIDEO
    #define streamtypeVIDEO mmioFOURCC('v', 'i', 'd', 's')
#endif

#define ICTYPE_VCAP mmioFOURCC('v', 'c', 'a', 'p')
#define ICTYPE_ACM  mmioFOURCC('a', 'u', 'd', 'c')
#define SMAG        mmioFOURCC('S', 'm', 'a', 'g')

#define IC_INI      TEXT("Installable Compressors")

//STATICDT TCHAR   sz44s[]           = TEXT("%4.4hs");
STATICDT TCHAR   szMSACM[]         = TEXT("MSACM");
STATICDT TCHAR   szVIDC[]          = TEXT("VIDC");

STATICDT TCHAR   gszIniSect[]       = IC_INI;
STATICDT TCHAR   gszSystemIni[]     = TEXT("SYSTEM.INI");
STATICDT TCHAR   gszDrivers[]       = DRIVERS_SECTION;


STATICDT TCHAR   szNull[]          = TEXT("");
STATICDT TCHAR   szICKey[]         = TEXT("%4.4hs.%4.4hs");
STATICDT TCHAR   szMSVideo[]       = TEXT("MSVideo");
STATICDT SZCODEA szDriverProc[]    = "DriverProc";

#ifdef _WIN32
// Use a mapping to get stuff into and out of the registry
BOOL myWritePrivateProfileString(

    LPCTSTR  lpszSection,       // address of section name
    LPCTSTR  lpszKey,           // address of key name
    LPCTSTR  lpszString         // address of string to add
);

DWORD myGetPrivateProfileString(

    LPCTSTR  lpszSection,       // address of section name
    LPCTSTR  lpszKey,           // address of key name
    LPCTSTR  lpszDefault,       // address of default string
    LPTSTR   lpszReturnBuffer,  // address of destination buffer
    DWORD    cchReturnBuffer    // size of destination buffer
    );

#endif

#ifdef DEBUG
    #define DPF( x ) dprintfc x
    #define DEBUG_RETAIL
#else
    #define DPF(x)
#endif

#ifdef DEBUG_RETAIL
    STATICFN void CDECL dprintfc(LPSTR, ...);
    static  char gszModname[] = "COMPMAN";
    #define MODNAME gszModname

    #define RPF( x ) dprintfc x
    #define ROUT(sz) {static SZCODEA ach[] = sz; dprintfc(ach); }
    void  ICDebugMessage(HIC hic, UINT msg, DWORD_PTR dw1, DWORD_PTR dw2);
    LRESULT ICDebugReturn(LRESULT err);
#ifdef _WIN32
    #define DebugErr(flags, sz) {static SZCODEA ach[] = "COMPMAN: "sz; OutputDebugStringA(ach); }
#else
    #define DebugErr(flags, sz) {static SZCODE ach[] = "COMPMAN: "sz; DebugOutput(flags | DBF_MMSYSTEM, ach); }
#endif

#else     // !DEBUG_RETAIL
    #define RPF(x)
    #define ROUT(sz)
    #define ICDebugMessage(hic, msg, dw1, dw2)
    #define ICDebugReturn(err)  err
    #define DebugErr(flags, sz)
#endif

#ifndef WF_WINNT
#define WF_WINNT 0x4000
#endif

#ifdef _WIN32
#define IsWow() FALSE
#else
#define IsWow() ((BOOL) (GetWinFlags() & WF_WINNT))
#define GetDriverModuleHandle(h) (IsWow() ? h : GetDriverModuleHandle(h))
#endif

// HACK!
//
//
#if defined _WIN32 && !defined UNICODE
 #pragma message ("hack! use DrvGetModuleHandle on Chicago")
 #undef GetDriverModuleHandle
 #define GetDriverModuleHandle(h) DrvGetModuleHandle(h)
 extern HMODULE _stdcall DrvGetModuleHandle(HDRVR);
#endif

__inline void ictokey(DWORD fccType, DWORD fcc, LPTSTR sz)
{
    int i = wsprintf(sz, szICKey, (LPSTR)&(fccType),(LPSTR)&(fcc));

    while (i>0 && sz[i-1] == ' ')
	sz[--i] = 0;
}

#define WIDTHBYTES(i)     ((unsigned)((i+31)&(~31))/8)  /* ULONG aligned ! */
#define DIBWIDTHBYTES(bi) (int)WIDTHBYTES((int)(bi).biWidth * (int)(bi).biBitCount)

#ifdef DEBUG_RETAIL
STATICFN void ICDump(void);
#endif

//
//  the following array is used for 'installed' converters
//
//  converters are either driver handles or indexes into this array
//
//  'function' converters are installed into this array, 'driver' converters
//  are installed in SYSTEM.INI
//

#define MAX_CONVERTERS 75           // maximum installed converters.

typedef struct  {
    DWORD       dwSmag;             // 'Smag'
    HTASK       hTask;              // owner task.
    DWORD       fccType;            // converter type ie 'vidc'
    DWORD       fccHandler;         // converter id ie 'rle '
    HDRVR       hDriver;            // handle of driver
    LPARAM      dwDriver;           // driver id for functions
    DRIVERPROC  DriverProc;         // function to call
#ifdef NT_THUNK16
    DWORD       h32;                // 32-bit driver handle
#endif
}   IC, *PIC;

IC aicConverters[MAX_CONVERTERS];
int giMaxConverters = 0;             // High water mark of installed converters

/*
 * We dynamically allocate a buffer used in ICInfo to read all the
 * installable compressor definitions from system.ini.
 * The buffer is freed when the driver is unloaded (in IC_Unload).
 * The previous code had a buffer which was only freed when the executable
 * was unloaded, and not freed on DLL unload.
 */
static LPVOID lpICInfoMem = NULL;

/*****************************************************************************
 ****************************************************************************/

LRESULT CALLBACK DriverProcNull(LPARAM dwDriverID, HANDLE hDriver, UINT wMessage, LPARAM dwParam1, LPARAM dwParam2)
{
    DPF(("codec called after it has been removed with ICRemove\r\n"));
    return ICERR_UNSUPPORTED;
}


/*****************************************************************************
 ****************************************************************************/

#if defined _WIN32
STATICFN HDRVR LoadDriver(LPWSTR szDriver, DRIVERPROC FAR *lpDriverProc);
#else
STATICFN HDRVR LoadDriver(LPSTR szDriver, DRIVERPROC FAR *lpDriverProc);
#endif
STATICFN void FreeDriver(HDRVR hDriver);

/*****************************************************************************

    driver cache - to make enuming/loading faster we keep the last N
    module's open for a while.

 ****************************************************************************/

#define NEVERCACHECODECS    // turn caching off for M6....

#if defined _WIN32 || defined NEVERCACHECODECS
#define CacheModule(x)
#else
#define N_MODULES   10      //!!!????

HMODULE ahModule[N_MODULES];
int     iModule = 0;

STATICFN void CacheModule(HMODULE hModule)
{
    char ach[128];

    //
    // what if this module is in the list currently?
    //
#if 0
    // we dont do this so unused compressors will fall off the end....
    int i;

    for (i=0; i<N_MODULES; i++)
    {
	if (ahModule[i] && ahModule[i] == hModule)
	    return;
    }
#endif

    //
    // add this module to the cache
    //
    if (hModule)
    {
	extern HMODULE ghInst;          // in MSVIDEO/init.c
	int iUsage;

	GetModuleFileNameA(hModule, ach, sizeof(ach));
	DPF(("Loading module: %s\r\n", (LPSTR)ach));
#ifndef _WIN32  // On NT GetModuleUsage always returns 1.  So... we cache
	iUsage = GetModuleUsage(ghInst);
#endif
	LoadLibraryA(ach);

#ifndef _WIN32  // On NT GetModuleUsage always returns 1.  So... we cache
	//
	// dont cache modules that link to MSVIDEO
	// we should realy do a toolhelp thing!
	// or force apps to call VFWInit and VFWExit()
	//
	// The NT position is more awkward..!
	//
	if (iUsage != GetModuleUsage(ghInst))
	{
	    DPF(("Not caching this module because it links to MSVIDEO\r\n"));
	    FreeLibrary(hModule);
	    return;
	}
#endif
    }

    //
    // free module in our slot.
    //
    if (ahModule[iModule] != NULL)
    {
#ifdef DEBUG
	GetModuleFileNameA(ahModule[iModule], ach, sizeof(ach));
	DPF(("Freeing module: %s  Handle==%8x\r\n", (LPSTR)ach, ahModule[iModule]));
	if (hModule!=NULL) {
	    GetModuleFileNameA(hModule, ach, sizeof(ach));
	    DPF(("Replacing with: %s  Handle==%8x\r\n", (LPSTR)ach, hModule));
	} else
	    DPF(("Slot now empty\r\n"));
#endif
	FreeLibrary(ahModule[iModule]);
    }

    ahModule[iModule] = hModule;
    iModule++;

    if (iModule >= N_MODULES)
	iModule = 0;
}
#endif


/*****************************************************************************
 ****************************************************************************/

/*****************************************************************************
 * FixFOURCC - clean up a FOURCC
 ****************************************************************************/

INLINE STATICFN DWORD Fix4CC(DWORD fcc)
{
    int i;

    if (fcc > 256)
    {
	AnsiLowerBuff((LPSTR)&fcc, sizeof(fcc));

	for (i=0; i<4; i++)
	{
	    if (((LPSTR)&fcc)[i] == 0)
		for (; i<4; i++)
		    ((LPSTR)&fcc)[i] = ' ';
	}
    }

    return fcc;
}

/*****************************************************************************
 * @doc INTERNAL IC
 *
 * @api PIC | FindConverter |
 *      search the converter list for a un-opened converter
 *
 ****************************************************************************/

STATICFN PIC FindConverter(DWORD fccType, DWORD fccHandler)
{
    int i;
    PIC pic;

    // By running the loop to <= giMaxConverters we allow an empty slot to
    // be found.
    for (i=0; i<=giMaxConverters; i++)
    {
	pic = &aicConverters[i];

	if (pic->fccType  == fccType &&
	    pic->fccHandler  == fccHandler &&
	    pic->dwDriver == 0L)
	{
	    if (pic->DriverProc != NULL && IsBadCodePtr((FARPROC)pic->DriverProc))
	    {
		pic->DriverProc = NULL;
		ICClose((HIC)pic);
		DPF(("NO driver for fccType=%4.4hs, Handler=%4.4hs\n", (LPSTR)&fccType, (LPSTR)&fccHandler));
		return NULL;
	    }

	    if ((0 == fccType + fccHandler)
	      && (i < (MAX_CONVERTERS-1))
	      && (i==giMaxConverters))
	    {
		++giMaxConverters;     // Up the high water mark
	    }
	    DPF(("Possible driver for fccType=%4.4hs, Handler=%4.4hs,  Slot %d\n", (LPSTR)&fccType, (LPSTR)&fccHandler, i));
	    return pic;
	}
    }

    DPF(("FindConverter: NO drivers for fccType=%4.4hs, Handler=%4.4hs\n", (LPSTR)&fccType, (LPSTR)&fccHandler));
    return NULL;
}


#ifdef _WIN32

/*
 * we need to hold a critical section around the ICOpen code to protect
 * multi-thread simultaneous opens. This critsec is initialized by
 * ICDllEntryPoint (called from video\init.c at dll attach time) and is deleted
 * by ICDllEntryPoint (called from video\init.c at dll detach time).
 */
CRITICAL_SECTION ICOpenCritSec;

#ifdef DEBUGLOAD
// There is a suspicion that a nasty problem exists on NT whereby the DLL
// load/unload routines might not be called in certain esoteric cases.  As
// we rely on these routines to set up the ICOpenCritSec code has been
// added to verify that the critical section has indeed been set up.  On
// LOAD we turn one bit on in a global variable.  On UNLOAD we turn that
// bit off and turn another bit on.
DWORD gdwLoadFlags = 0;
#define ICLOAD_CALLED   0x00010000
#define ICUNLOAD_CALLED 0x00000001
#endif

//
//  We load/unload wow32.dll here.  This is used in the thunking code which
//  just does GetModuleHandle on it.  This is not really necessary in
//  Daytona but is in Chicago.
//
//  WORSE: IT IS IMPERATIVE TO NOT DO THIS ON DAYTONA.  The Daytona code
//  uses the fact the WOW32.DLL is loaded in the context of this process
//  as the indication that it is executing in the WOW process.

#ifdef CHICAGO
HMODULE hWow32 = NULL;
#endif

#endif

#ifdef _WIN32
//--------------------------------------------------------------------------;
//
//  BOOL ICDllEntryPoint [32-bit]
//
//  Description:
//      Called by msvideo's DllEntryPoint
//
//  History:
//      11/02/94    [frankye]
//
//--------------------------------------------------------------------------;
#ifdef LS_THUNK32
BOOL PASCAL ls_ThunkConnect32(LPCSTR pszDll16, LPCSTR pszDll32, HINSTANCE hinst, DWORD dwReason);
BOOL PASCAL sl_ThunkConnect32(LPCSTR pszDll16, LPCSTR pszDll32, HINSTANCE hinst, DWORD dwReason);
#endif
BOOL WINAPI ICDllEntryPoint(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
	case DLL_PROCESS_ATTACH:
	{
	    //DPFS(dbgInit, 0, "ICDllEntryPoint(DLL_PROCESS_ATTACH)");

#ifdef DEBUGLOAD
	    if (gdwLoadFlags & ICLOAD_CALLED) {
#ifdef DEBUG
		DPF(("!IC open crit sec already set up"));
#endif
	    }
	    gdwLoadFlags |= ICLOAD_CALLED;
	    gdwLoadFlags &= ~ICUNLOAD_CALLED;
#endif

	    InitializeCriticalSection(&ICOpenCritSec);

#ifdef LS_THUNK32
	    hWow32 = LoadLibrary(TEXT("WOW32.DLL"));
	    ls_ThunkConnect32(TEXT("MSVIDEO.DLL"), TEXT("MSVFW32.DLL"), hinstDLL, fdwReason);
	    sl_ThunkConnect32(TEXT("MSVIDEO.DLL"), TEXT("MSVFW32.DLL"), hinstDLL, fdwReason);
#endif

	    return TRUE;
	}

	case DLL_PROCESS_DETACH:
	{
	    //DPFS(dbgInit, 0, "ICDllEntryPoint(DLL_PROCESS_DETACH)");

#ifdef LS_THUNK32
	    ls_ThunkConnect32(TEXT("MSVIDEO.DLL"), TEXT("MSVFW32.DLL"), hinstDLL, fdwReason);
	    sl_ThunkConnect32(TEXT("MSVIDEO.DLL"), TEXT("MSVFW32.DLL"), hinstDLL, fdwReason);
	    if (NULL != hWow32) FreeLibrary(hWow32);
#endif

	    DeleteCriticalSection(&ICOpenCritSec);

#ifdef DEBUGLOAD
	    gdwLoadFlags |= ICUNLOAD_CALLED;
	    gdwLoadFlags &= ~ICLOAD_CALLED;
#endif
	
	    if (lpICInfoMem) {
		GlobalFreePtr(lpICInfoMem);
		lpICInfoMem = NULL;
	    }

#ifdef CHICAGO
	    dbgCheckShutdown();
#endif
	    return TRUE;
	}

    }

    return TRUE;
}

#else
//--------------------------------------------------------------------------;
//
//  BOOL ICDllEntryPoint [16-bit]
//
//  Description:
//      Called by msvideo's DllEntryPoint
//
//  History:
//      11/02/94    [frankye]
//
//--------------------------------------------------------------------------;
BOOL FAR PASCAL ls_ThunkConnect16(LPCSTR pszDll16, LPCSTR pszDll32, HINSTANCE hinst, DWORD dwReason);
BOOL FAR PASCAL sl_ThunkConnect16(LPCSTR pszDll16, LPCSTR pszDll32, HINSTANCE hinst, DWORD dwReason);
BOOL FAR PASCAL ICDllEntryPoint(DWORD dwReason, HINSTANCE hinstDLL, WORD wDS, WORD wHeapSize, DWORD dwReserved1, WORD wReserved2)
{
    PICMGARB pig;

    DPFS(dbgInit, 0, "ICDllEntryPoint()");

    switch (dwReason)
    {
	case 1:
	{
	    if (NULL == (pig = pigFind()))
	    {
		if (NULL == (pig = pigNew()))
		{
		    return FALSE;
		}
	    }

#ifdef LS_THUNK16
	    ls_ThunkConnect16(TEXT("MSVIDEO.DLL"), TEXT("MSVFW32.DLL"), hinstDLL, dwReason);
	    sl_ThunkConnect16(TEXT("MSVIDEO.DLL"), TEXT("MSVFW32.DLL"), hinstDLL, dwReason);
#endif
	
	    if (1 == ++pig->cUsage)
	    {
		DPFS(dbgInit, 0, "ICProcessAttach: New process %08lXh", pig->pid);
		//
		//  We can do one-time-per-process init here...
		//
	    }

	    return TRUE;
	}

	case 0:
	{
	    if (NULL == (pig = pigFind()))
	    {
		DPF(0, "!ICProcessDetach: ERROR: Being freed by process %08lXh in which it was not loaded", GetCurrentProcessId());
		DebugErr(DBF_ERROR, "ICProcessDetach: ERROR: Being freed by a process in which it was not loaded");
		return FALSE;
	    }

	    if (0 == --pig->cUsage)
	    {
		//
		//  We can do one-time-per-process termination here...
		//
		DPFS(dbgInit, 0, "ICProcessDetach: Terminating for process %08lXh", pig->pid);
#ifdef NT_THUNK16
		genthunkTerminate(pig);
#endif
		pigDelete(pig);
		
		dbgCheckShutdown();
	    }

#ifdef LS_THUNK16
	    ls_ThunkConnect16(TEXT("MSVIDEO.DLL"), TEXT("MSVFW32.DLL"), hinstDLL, dwReason);
	    sl_ThunkConnect16(TEXT("MSVIDEO.DLL"), TEXT("MSVFW32.DLL"), hinstDLL, dwReason);
#endif
	    return TRUE;
	}

    }

    return TRUE;
}

#endif
/*****************************************************************************
 ****************************************************************************/

__inline BOOL ICValid(HIC hic)
{
    PIC pic = (PIC)hic;

    if (pic <  &aicConverters[0] ||
	pic >= &aicConverters[MAX_CONVERTERS] ||
	pic->dwSmag != SMAG)
    {
	DebugErr(DBF_ERROR, "Invalid HIC\r\n");
	return FALSE;
    }

    return TRUE;
}

/*****************************************************************************
 ****************************************************************************/

#define V_HIC(hic)              \
    if (!ICValid(hic))          \
	return ICERR_BADHANDLE;

/*****************************************************************************
 * @doc INTERNAL IC
 *
 * @api BOOL | ICCleanup | This function is called when a task exits or
 *      MSVIDEO.DLL is being unloaded.
 *
 * @parm HTASK | hTask | the task being terminated, NULL if DLL being unloaded
 *
 * @rdesc Returns nothing
 *
 * @comm  currently MSVIDEO only calles this function from it's WEP()
 *
 ****************************************************************************/

void FAR PASCAL ICCleanup(HTASK hTask)
{
    int i;
    PIC pic;

    //
    // free all HICs
    //
    for (i=0; i < giMaxConverters; i++)
    {
	pic = &aicConverters[i];

	if (pic->dwDriver != 0L && (pic->hTask == hTask || hTask == NULL))
	{
	    ROUT("Decompressor left open, closing\r\n");
	    ICClose((HIC)pic);
	}
    }

#ifdef N_MODULES
    //
    // free the module cache.
    //
    for (i=0; i<N_MODULES; i++)
	CacheModule(NULL);
#endif
}

/*****************************************************************************
 * @doc EXTERNAL IC  ICAPPS
 *
 * @api BOOL | ICInstall | This function installs a new compressor
 *      or decompressor.
 *
 * @parm DWORD | fccType | Specifies a four-character code indicating the
 *       type of data used by the compressor or decompressor.  Use 'vidc'
 *       for a video compressor or decompressor.
 *
 * @parm DWORD | fccHandler | Specifies a four-character code identifying
 *      a specific compressor or decompressor.
 *
 * @parm LPARAM | lParam | Specifies a pointer to a zero-terminated
 *       string containing the name of the compressor or decompressor,
 *       or it specifies a far pointer to a function used for compression
 *       or decompression. The contents of this parameter are defined
 *       by the flags set for <p wFlags>.
 *
 * @parm LPSTR | szDesc | Specifies a pointer to a zero-terminated string
 *        describing the installed compressor. Not use.
 *
 * @parm UINT | wFlags | Specifies flags defining the contents of <p lParam>.
 * The following flags are defined:
 *
 * @flag ICINSTALL_DRIVER | Indicates <p lParam> is a pointer to a zero-terminated
 *      string containing the name of the compressor to install.
 *
 * @flag ICINSTALL_FUNCTION | Indicates <p lParam> is a far pointer to
 *       a compressor function.  This function should
 *       be structured like the <f DriverProc> entry
 *       point function used by compressors.
 *
 * @rdesc Returns TRUE if successful.
 *
 * @comm  Applications must still open the installed compressor or
 *        decompressor before it can use the compressor or decompressor.
 *
 *        Usually, compressors and decompressors are installed by the user
 *        with the Drivers option of the Control Panel.
 *
 *        If your application installs a function as a compressor or
 *        decompressor, it should remove the compressor or decompressor
 *        with <f ICRemove> before it terminates. This prevents other
 *        applications from trying to access the function when it is not
 *        available.
 *
 *
 * @xref <f ICRemove>
 ****************************************************************************/
BOOL VFWAPI ICInstall(DWORD fccType, DWORD fccHandler, LPARAM lParam, LPSTR szDesc, UINT wFlags)
{
    TCHAR achKey[20];
    TCHAR buf[256];
    DWORD n;
    PIC  pic;

    ICEnterCrit(&ICOpenCritSec);
    fccType    = Fix4CC(fccType);
    fccHandler = Fix4CC(fccHandler);

    if ((pic = FindConverter(fccType, fccHandler)) == NULL)
	pic = FindConverter(0L, 0L);

    if (wFlags & ICINSTALL_DRIVER)
    {
	//
	//  dwConverter is the file name of a driver to install.
	//
	ictokey(fccType, fccHandler, achKey);

#ifdef UNICODE
	if (wFlags & ICINSTALL_UNICODE) {
	    lstrcpy(buf, (LPWSTR)lParam);
	    n = lstrlen(buf) + 1;    // Point past the terminating zero;
	    if (szDesc)
	    {
		lstrcpyn(buf+n, (LPWSTR)szDesc, NUMELMS(buf)-n);
		n += lstrlen(buf+n);
	    }
	    buf[n]=0;  // Always guarantee a second string - even if a null one
	} else {
	    // Convert the ANSI strings to UNICODE
	    n = 1 + wsprintf(buf, TEXT("%hs"), (LPSTR) lParam);
	    if (szDesc) {
		n += 1 + wsprintf(buf+n, TEXT("%hs"), szDesc);
	    }
	}
	// Buf now contains two strings, the second of which may be null (the description)
#else
	lstrcpy(buf, (LPSTR)lParam);

	if (szDesc)
	{
	    lstrcat(buf, TEXT(" "));
	    lstrcat(buf, szDesc);
	}
#endif

	ICLeaveCrit(&ICOpenCritSec);
	// Data is written (via the inifilemapping) to
	// HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\Drivers32
	// HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\Drivers.desc
	if (myWritePrivateProfileString(gszDrivers,achKey,buf))
	{
	    // delete the installable compressors entry for this driver
	    myWritePrivateProfileString(gszIniSect,achKey,NULL);
	    return TRUE;
	}

	return(FALSE);
    }
    else if (wFlags & ICINSTALL_FUNCTION)
    {
	if (pic == NULL)
	{
	    ICLeaveCrit(&ICOpenCritSec);
	    return FALSE;
	}

	pic->dwSmag     = SMAG;
	pic->fccType    = fccType;
	pic->fccHandler = fccHandler;
	pic->dwDriver   = 0L;
	pic->hDriver    = NULL;
	pic->DriverProc = (DRIVERPROC)lParam;
	DPF(("ICInstall, fccType=%4.4hs, Handler=%4.4hs,  Pic %x\n", (LPSTR)&fccType, (LPSTR)&fccHandler, pic));

	ICLeaveCrit(&ICOpenCritSec);

	return TRUE;
    }

#if 0
    else if (wFlags & ICINSTALL_HDRV)
    {
	if (pic == NULL)
	{
	    ICLeaveCrit(&ICOpenCritSec);
	    return FALSE;
	}

	pic->fccType  = fccType;
	pic->fccHandler  = fccHandler;
	pic->hDriver  = (HDRVR)lParam;
	pic->dwDriver = 0L;
	pic->DrvProc  = NULL;

	ICLeaveCrit(&ICOpenCritSec);

	return TRUE;
    }
#endif

    ICLeaveCrit(&ICOpenCritSec);

    return FALSE;
}

/*****************************************************************************
 * @doc EXTERNAL IC ICAPPS
 *
 * @api BOOL | ICRemove | This function removes an installed compressor.
 *
 * @parm DWORD | fccType | Specifies a four-character code indicating the
 * type of data used by the compressor.  Use 'vidc' for video compressors.
 *
 * @parm DWORD | fccHandler | Specifies a four-character code identifying
 * a specific compressor.
 *
 * @parm UINT | wFlags | Not used.
 *
 * @rdesc Returns TRUE if successful.
 *
 * @xref <f ICInstall>
 ****************************************************************************/
BOOL VFWAPI ICRemove(DWORD fccType, DWORD fccHandler, UINT wFlags)
{
    TCHAR achKey[20];
    PIC  pic;

    ICEnterCrit(&ICOpenCritSec);
    fccType    = Fix4CC(fccType);
    fccHandler = Fix4CC(fccHandler);

    if (pic = FindConverter(fccType, fccHandler))
    {
	int i;

	//
	// we should realy keep usage counts!!!
	//
	for (i=0; i<giMaxConverters; i++)
	{
	    if (pic->DriverProc == aicConverters[i].DriverProc)
	    {
		DPF(("ACK! Handler is in use\r\n"));
		pic->DriverProc = (DRIVERPROC)DriverProcNull;
	    }
	}

	ICClose((HIC)pic);
    }
    else
    {
	// Remove the information
	ictokey(fccType, fccHandler, achKey);
	myWritePrivateProfileString(gszIniSect,achKey,NULL);
	myWritePrivateProfileString(gszDrivers,achKey,NULL);
    }

    ICLeaveCrit(&ICOpenCritSec);

    return TRUE;
}

//
//  Internal routine to enumerate all the installed drivers
//

BOOL ReadDriversInfo()
{
    LPSTR psz = NULL; // THIS IS ALWAYS an ANSI string pointer!
    if (lpICInfoMem == NULL) {
	UINT cbBuffer = 125 * sizeof(TCHAR);
	UINT cchBuffer;

	ICEnterCrit(&ICOpenCritSec);
	for (;;)
	{
	    lpICInfoMem = GlobalAllocPtr(GMEM_SHARE | GHND, cbBuffer);

	    if (!lpICInfoMem) {
		DPF(("Out of memory for SYSTEM.INI keys\r\n"));
		ICLeaveCrit(&ICOpenCritSec);
		return FALSE;
	    }

	    cchBuffer = (UINT)myGetPrivateProfileString(gszDrivers,
						      NULL,
						      szNull,
						      lpICInfoMem,
						      cbBuffer / sizeof(TCHAR));

	    if (cchBuffer < ((cbBuffer/sizeof(TCHAR)) - 5)) {
		cchBuffer += (UINT)myGetPrivateProfileString(gszIniSect,
						      NULL,
						      szNull,
						      (LPTSTR)lpICInfoMem + cchBuffer,
						      (cbBuffer/sizeof(TCHAR)) - cchBuffer);
		//
		// if all of the INI data fit, we can
		// leave the loop
		//
		if (cchBuffer < ((cbBuffer/sizeof(TCHAR)) - 5))
		    break;
	    }

	    GlobalFreePtr(lpICInfoMem), lpICInfoMem = NULL;

	    //
	    //  if cannot fit drivers section in 32k, then something is horked
	    //  with the section... so let's bail.
	    //
	    if (cbBuffer >= 0x8000) {
		DPF(("SYSTEM.INI keys won't fit in 32K????\r\n"));
		ICLeaveCrit(&ICOpenCritSec);
		return FALSE;
	    }

	    //
	    // double the size of our buffer and try again.
	    //
	    cbBuffer *= 2;
	    DPF(("Increasing size of SYSTEM.INI buffer to %d\r\n", cbBuffer));
	}

#if defined UNICODE
	// convert the INI data from UNICODE to ANSI
	//
	psz = GlobalAllocPtr (GMEM_SHARE | GHND, cchBuffer + 7);
	if ( ! psz) {
	    GlobalFreePtr (lpICInfoMem), lpICInfoMem = NULL;
	    ICLeaveCrit(&ICOpenCritSec);
	    return FALSE;
	}

	mmWideToAnsi (psz, lpICInfoMem, cchBuffer+2);
	GlobalFreePtr (lpICInfoMem);
	lpICInfoMem = psz;
#endif

	// convert codec information to lowercase
	for (psz = lpICInfoMem; *psz != 0; psz += lstrlenA(psz) + 1)
	{
#if 0  // we only put valid codecs into lpICInfoMem these days
	    if (psz[4] != '.')
		continue;
#endif

	    // convert this piece to lowercase
	    CharLowerA (psz);
	    DPF(("Compressor: %hs\n", psz));
	}
	ICLeaveCrit(&ICOpenCritSec);
    }
    return (lpICInfoMem != NULL);
}


/*****************************************************************************
 * @doc EXTERNAL IC ICAPPS
 *
 * @api BOOL | ICInfo | This function returns information about
 *      specific installed compressors, or it enumerates
 *      the compressors installed.
 *
 * @parm DWORD | fccType | Specifies a four-character code indicating
 *       the type of compressor.  To match all compressor types specify zero.
 *
 * @parm DWORD | fccHandler | Specifies a four-character code identifying
 *       a specific compressor, or a number between 0 and the number
 *       of installed compressors of the type specified by <t fccType>.
 *
 * @parm ICINFO FAR * | lpicinfo | Specifies a far pointer to a
 *       <t ICINFO> structure used to return
 *      information about the compressor.
 *
 * @comm This function does not return full informaiton about
 *       a compressor or decompressor. Use <f ICGetInfo> for full
 *       information.
 *
 * @rdesc Returns TRUE if successful.
 ****************************************************************************/
#ifdef NT_THUNK16
BOOL VFWAPI ICInfoInternal(DWORD fccType, DWORD fccHandler, ICINFO FAR * lpicinfo);

// If we are compiling the thunks, then the ICINFO entry point calls
// the 32 bit thunk, or calls the real ICInfo code (as ICInfoInternal).
// We deliberately give precedence to 16 bit compressors, although this
// ordering can be trivially changed.
// ??: Should we allow an INI setting to change the order?

BOOL VFWAPI ICInfo(DWORD fccType, DWORD fccHandler, ICINFO FAR * lpicinfo)
{
#ifdef DEBUG
    BOOL fResult;
#endif
    //
    //  See if there is a 32-bit compressor we can use
    //
    if (ICInfoInternal(fccType, fccHandler, lpicinfo)) {
	return(TRUE);
    }

#ifdef DEBUG
    fResult = (ICInfo32(fccType, fccHandler, lpicinfo));
    DPF(("ICInfo32 returned %s\r\n", (fResult ? "TRUE" : "FALSE")));
    return fResult;
#else
    return (ICInfo32(fccType, fccHandler, lpicinfo));
#endif
}

// Now map ICInfo calls to ICInfoInternal for the duration of the ICInfo
// routine.  This affects the two recursive calls within ICInfo.
#define ICInfo ICInfoInternal

#endif // NT_THUNK16


BOOL VFWAPI ICInfo(DWORD fccType, DWORD fccHandler, ICINFO FAR * lpicinfo)
{
    LPSTR psz = NULL; // THIS IS ALWAYS an ANSI string pointer!
    TCHAR buf[128];
    TCHAR achKey[20];
    int  i;
    int  iComp;
    PIC  pic;

    if (lpicinfo == NULL)
	return FALSE;

    if (fccType > 0 && fccType < 256) {
	DPF(("fcctype invalid (%d)\n", fccType));
	return FALSE;
    }

    fccType    = Fix4CC(fccType);
    fccHandler = Fix4CC(fccHandler);

    if (fccType != 0 && fccHandler > 256)
    {
	//
	//  the user has given us a specific fccType and fccHandler
	//  get the info and return.
	//
	if (pic = FindConverter(fccType, fccHandler))
	{
	    ICGetInfo((HIC)pic, lpicinfo, sizeof(ICINFO));
	    return TRUE;
	}
	else
	{
	    lpicinfo->dwSize            = sizeof(ICINFO);
	    lpicinfo->fccType           = fccType;
	    lpicinfo->fccHandler        = fccHandler;
	    lpicinfo->dwFlags           = 0;
	    lpicinfo->dwVersionICM      = ICVERSION;
	    lpicinfo->dwVersion         = 0;
	    lpicinfo->szDriver[0]       = 0;
	    lpicinfo->szDescription[0]  = 0;
	    lpicinfo->szName[0]         = 0;
	    DPF(("ICInfo, fccType=%4.4hs, Handler=%4.4hs\n", (LPSTR)&fccType, (LPSTR)&fccHandler));

	    ictokey(fccType, fccHandler, achKey);

	    if (!myGetPrivateProfileString(gszDrivers,achKey,szNull,buf,NUMELMS(buf)) &&
		!myGetPrivateProfileString(gszIniSect,achKey,szNull,buf,NUMELMS(buf)))
	    {
		DPF(("NO information in DRIVERS section\n"));
		return FALSE;
	    }

	    for (i=0; buf[i] && buf[i] != TEXT(' '); ++i)
		lpicinfo->szDriver[i] = buf[i];

	    lpicinfo->szDriver[i] = 0;

	    //
	    // the driver must be opened to get description
	    //
	    lpicinfo->szDescription[0] = 0;

	    return TRUE;
	}
    }
    else
    {
	//
	//  the user has given us a specific fccType and a
	//  ordinal for fccHandler, enum the compressors, looking for
	//  the nth compressor of 'fccType'
	//

	iComp = (int)fccHandler;

	//
	//  walk the installed converters.
	//
	for (i=0; i < giMaxConverters; i++)
	{
	    pic = &aicConverters[i];

	    if (pic->fccType != 0 &&
		(fccType == 0 || pic->fccType == fccType) &&
		pic->dwDriver == 0L && iComp-- == 0)
	    {
		return ICInfo(pic->fccType, pic->fccHandler, lpicinfo);
	    }
	}

	//
	// read all the keys. from [Drivers] and [Instalable Compressors]
	// if we havent read them before.
	//
	// NOTE: what we get back will always be ANSI or WIDE depending
	// on whether UNICODE is defined.  If WIDE, we convert to
	// ANSI before exiting the if statement.
	//

	if (lpICInfoMem == NULL) {
	    if (!ReadDriversInfo())
		return(FALSE);
	}

	// set our pointer psz to point to the beginning of
	// the buffer of INI information we just read.
	// remember that we KNOW that this is ANSI data now.
	//
	//assert (sizeof(*psz) == 1);
	//assert (lpICInfoMem != NULL);

	// loop through the buffer until we get to a double '\0'
	// which indicates the end of the data.
	//
	for (psz = lpICInfoMem; *psz != 0; psz += lstrlenA(psz) + 1)
	{
#if 0       // there can only be valid codec in the memory block
	    if (psz[4] != '.')
		continue;
#endif

	    // convert this piece to lowercase and check to see
	    // if it matches the requested type signature
	    //
	    // NO.  Done when first read.  CharLowerA (psz);

	    // if this is a match, and it's the one we wanted,
	    // return its ICINFO
	    //
	    if ((fccType == 0 || fccType == *(DWORD UNALIGNED FAR *)psz)
	      && iComp-- == 0)
	    {
		return ICInfo(*(DWORD UNALIGNED FAR *)psz,
			      *(DWORD UNALIGNED FAR *)&psz[5],
			      lpicinfo);
	    }
	}

#ifdef DAYTONA
	// If we get to here, then the index is higher than the number
	// of installed compressors.
	//
	// Write the number of compressors found into the structure.
	// This value is used by the NT thunks to pass back to the 16
	// bit side the maximum number of 32 bit compressors.

	lpicinfo->fccHandler = (int)fccHandler-iComp;

// LATER: we MUST enumerate the count of installed msvideo drivers
// as well.  However, lets see if this fixes the Adobe Premiere problem.
#endif

	//
	// now walk the msvideo drivers. these are listed in system.ini
	// like so:
	//
	//      [Drivers]
	//          MSVideo = driver
	//          MSVideo1 = driver
	//          MSVideoN =
	//
	if (fccType == 0 || fccType == ICTYPE_VCAP)
	{
	    lstrcpy(achKey, szMSVideo);

	    if (iComp > 0)
		wsprintf(achKey+lstrlen(achKey), (LPVOID)"%d", iComp);

	    if (!myGetPrivateProfileString(gszDrivers,achKey,szNull,buf,NUMELMS(buf)))
		return FALSE;

	    lpicinfo->dwSize            = sizeof(ICINFO);
	    lpicinfo->fccType           = ICTYPE_VCAP;
	    lpicinfo->fccHandler        = iComp;
	    lpicinfo->dwFlags           = 0;
	    lpicinfo->dwVersionICM      = ICVERSION;    //??? right for video?
	    lpicinfo->dwVersion         = 0;
	    lpicinfo->szDriver[0]       = 0;
	    lpicinfo->szDescription[0]  = 0;
	    lpicinfo->szName[0]         = 0;

	    for (i=0; buf[i] && buf[i] != TEXT(' '); i++)
		lpicinfo->szDriver[i] = buf[i];

	    lpicinfo->szDriver[i] = 0;
	    return TRUE;
	}

	return FALSE;
    }
}
#undef ICInfo

BOOL VFWAPI ICInfoInternal(DWORD fccType, DWORD fccHandler, ICINFO FAR * lpicinfo)
{
    return(ICInfo(fccType, fccHandler, lpicinfo));
}
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

/*****************************************************************************
 * @doc EXTERNAL IC ICAPPS
 *
 * @api LRESULT | ICGetInfo | This function obtains information about
 *      a compressor.
 *
 * @parm HIC | hic | Specifies a handle to a compressor.
 *
 * @parm ICINFO FAR * | lpicinfo | Specifies a far pointer to <t ICINFO> structure
 *       used to return information about the compressor.
 *
 * @parm DWORD | cb | Specifies the size, in bytes, of the structure pointed to
 *       by <p lpicinfo>.
 *
 * @rdesc Return the number of bytes copied into the data structure,
 *        or zero if an error occurs.
 *
 * @comm Use <f ICInfo> for full information about a compressor.
 *
 ****************************************************************************/
LRESULT VFWAPI ICGetInfo(HIC hic, ICINFO FAR *picinfo, DWORD cb)
{
    PIC pic = (PIC)hic;
    LRESULT dw;

    V_HIC(hic);

    picinfo->dwSize            = sizeof(ICINFO);
    picinfo->fccType           = 0;
    picinfo->fccHandler        = 0;
    picinfo->dwFlags           = 0;
    picinfo->dwVersionICM      = ICVERSION;
    picinfo->dwVersion         = 0;
    picinfo->szDriver[0]       = 0;
    picinfo->szDescription[0]  = 0;
    picinfo->szName[0]         = 0;

#ifdef NT_THUNK16
    if (!Is32bitHandle(hic))
#endif //NT_THUNK16

    if (pic->hDriver)
    {
       #if defined _WIN32 && ! defined UNICODE
	char szDriver[NUMELMS(picinfo->szDriver)];

	GetModuleFileName (GetDriverModuleHandle (pic->hDriver),
	    szDriver, sizeof(szDriver));

	mmAnsiToWide (picinfo->szDriver, szDriver, NUMELMS(szDriver));
       #else
	GetModuleFileName(GetDriverModuleHandle (pic->hDriver),
	    picinfo->szDriver, sizeof(picinfo->szDriver));
       #endif
    }

    dw = ICSendMessage((HIC)pic, ICM_GETINFO, (DWORD_PTR)picinfo, cb);

    return dw;
}

/*****************************************************************************
 * @doc EXTERNAL IC ICAPPS
 *
 * @api LRESULT | ICSendMessage | This function sends a
 *      message to a compressor.
 *
 * @parm HIC  | hic  | Specifies the handle of the
 *       compressor to receive the message.
 *
 * @parm UINT | wMsg | Specifies the message to send.
 *
 * @parm DWORD | dw1 | Specifies additional message-specific information.
 *
 * @parm DWORD | dw2 | Specifies additional message-specific information.
 *
 * @rdesc Returns a message-specific result.
 ****************************************************************************/
LRESULT VFWAPI ICSendMessage(HIC hic, UINT msg, DWORD_PTR dw1, DWORD_PTR dw2)
{
    PIC pic = (PIC)hic;
    LRESULT l;

    V_HIC(hic);
#ifdef NT_THUNK16

    //
    // If it's a 32-bit handle then send it to the 32-bit code
    // We need to take some extra care with ICM_DRAW_SUGGESTFORMAT
    // which can include a HIC in the ICDRAWSUGGEST structure.
    //

#define ICD(dw1)  ((ICDRAWSUGGEST FAR *)(dw1))

    if (pic->h32) {
	if ((msg == ICM_DRAW_SUGGESTFORMAT)
	    && (((ICDRAWSUGGEST FAR *)dw1)->hicDecompressor))
	{
	    // We are in the problem area.
	    //   IF the hicDecompressor field is NULL, pass as is.
	    //   IF it identifies a 32 bit decompressor, translate the handle
	    //   OTHERWISE... what?  We have a 32 bit compressor, that is
	    //      being told it can use a 16 bit decompressor!!
	    if ( ((PIC) (((ICDRAWSUGGEST FAR *)dw1)->hicDecompressor))->h32)
	    {
		ICD(dw1)->hicDecompressor
			= (HIC)((PIC)(ICD(dw1)->hicDecompressor))->h32;
	    } else
	    {
		ICD(dw1)->hicDecompressor = NULL;  // Sigh...
	    }

	}
	return ICSendMessage32(pic->h32, msg, dw1, dw2);
    }

#endif //NT_THUNK16

    ICDebugMessage(hic, msg, dw1, dw2);

    l = pic->DriverProc(pic->dwDriver, (HDRVR)1, msg, dw1, dw2);

#if 1 //!!! is this realy needed!  !!!yes I think it is
    //
    // special case some messages and give default values.
    //
    if (l == ICERR_UNSUPPORTED)
    {
	switch (msg)
	{
	    case ICM_GETDEFAULTQUALITY:
		*((LPDWORD)dw1) = ICQUALITY_HIGH;
		l = ICERR_OK;
		break;

	    case ICM_GETDEFAULTKEYFRAMERATE:
		*((LPDWORD)dw1) = 15;
		l = ICERR_OK;
		break;
	}
    }
#endif

    return ICDebugReturn(l);
}

#ifndef _WIN32
/*****************************************************************************
 * @doc EXTERNAL IC ICAPPS
 *
 * @api LRESULT | ICMessage | This function sends a
 *      message and a variable number of arguments to a compressor.
 *      If a macro is defined for the message you want to send,
 *      use the macro rather than this function.
 *
 * @parm HIC  | hic  | Specifies the handle of the
 *       compressor to receive the message.
 *
 * @parm UINT | msg | Specifies the message to send.
 *
 * @parm UINT | cb  | Specifies the size, in bytes, of the
 *       optional parameters. (This is usually the size of the data
 *       structure used to store the parameters.)
 *
 * @parm . | . . | Represents the variable number of arguments used
 *       for the optional parameters.
 *
 * @rdesc Returns a message-specific result.
 ****************************************************************************/
LRESULT VFWAPIV ICMessage(HIC hic, UINT msg, UINT cb, ...)
{
    // NOTE no LOADDS!
#ifndef _WIN32
    return ICSendMessage(hic, msg, (DWORD_PTR)(LPVOID)(&cb+1), cb);
#else
    va_list va;

    va_start(va, cb);
    va_end(va);

    // nice try, but doesn't work. va is larger than 4 bytes.
    return ICSendMessage(hic, msg, (DWORD_PTR)va, cb);
#endif
}

// on Win32, ICMessage is not supported. All compman.h macros that call
// it are defined in compman.h as static inline functions

#endif







/*****************************************************************************
 * @doc EXTERNAL IC ICAPPS
 *
 * @api HIC | ICOpen | This function opens a compressor or decompressor.
 *
 * @parm DWORD | fccType | Specifies the type of compressor
 *      the caller is trying to open.  For video, this is ICTYPE_VIDEO.
 *
 * @parm DWORD | fccHandler | Specifies a single preferred handler of the
 *      given type that should be tried first.  Typically, this comes
 *      from the stream header in an AVI file.
 *
 * @parm UINT | wMode | Specifies a flag to defining the use of
 *       the compressor or decompressor.
 *       This parameter can contain one of the following values:
 *
 * @flag ICMODE_COMPRESS | Advises a compressor it is opened for compression.
 *
 * @flag ICMODE_FASTCOMPRESS | Advise a compressor it is open
 *       for fast (real-time) compression.
 *
 * @flag ICMODE_DECOMPRESS | Advises a decompressor it is opened for decompression.
 *
 * @flag ICMODE_FASTDECOMPRESS | Advises a decompressor it is opened
 *       for fast (real-time) decompression.
 *
 * @flag ICMODE_DRAW | Advises a decompressor it is opened
 *       to decompress an image and draw it directly to hardware.
 *
 * @flag ICMODE_QUERY | Advise a compressor or decompressor it is opened
 *       to obtain information.
 *
 * @rdesc Returns a handle to a compressor or decompressor
 *        if successful, otherwise it returns zero.
 ****************************************************************************/

/* Helper functions for compression library */
HIC VFWAPI ICOpen(DWORD fccType, DWORD fccHandler, UINT wMode)
{
    ICOPEN      icopen;
    ICINFO      icinfo;
    PIC         pic, picT;
    LRESULT     dw;

    ICEnterCrit(&ICOpenCritSec);

    AnsiLowerBuff((LPSTR) &fccType, sizeof(DWORD));
    AnsiLowerBuff((LPSTR) &fccHandler, sizeof(DWORD));
    icopen.dwSize  = sizeof(ICOPEN);
    icopen.fccType = fccType;
    icopen.fccHandler = fccHandler;
    icopen.dwFlags = wMode;
    icopen.dwError = 0;

    DPF(("ICOpen('%4.4hs','%4.4hs)'\r\n", (LPSTR)&fccType, (LPSTR)&fccHandler));

    if (!ICInfo(fccType, fccHandler, &icinfo))
    {
	RPF(("Unable to locate Compression module '%4.4hs' '%4.4hs'\r\n", (LPSTR)&fccType, (LPSTR)&fccHandler));

	ICLeaveCrit(&ICOpenCritSec);
	return NULL;
    }

    pic = FindConverter(0L, 0L);

    if (pic == NULL)
    {
	ICLeaveCrit(&ICOpenCritSec);
	return NULL;
    }

#ifdef NT_THUNK16
    // Try and open on the 32 bit side first.
    // This block and the one below can be interchanged to alter the order
    // in which we try and open the compressor.

    pic->dwSmag     = SMAG;
    pic->hTask      = (HTASK)GetCurrentTask();
    pic->h32 = ICOpen32(fccType, fccHandler, wMode);

    if (pic->h32 != 0) {
	pic->fccType    = fccType;
	pic->fccHandler = fccHandler;
	pic->dwDriver   = (DWORD_PTR) -1;
	pic->DriverProc = NULL;
	ICLeaveCrit(&ICOpenCritSec);  // A noop for 16 bit code...but...
	return (HIC)pic;
    }
    // Try and open on the 16 bit side
#endif //NT_THUNK16

    pic->dwSmag     = SMAG;
    pic->hTask      = GetCurrentTask();

    if (icinfo.szDriver[0])
    {
#ifdef DEBUG
	DWORD time = timeGetTime();
	//char ach[80];
#endif
	pic->hDriver = LoadDriver(icinfo.szDriver, &pic->DriverProc);

#ifdef DEBUG
	time = timeGetTime() - time;
	DPF(("ICOPEN: LoadDriver(%ls) (%ldms)  Module Handle==%8x\r\n", (LPSTR)icinfo.szDriver, time, pic->hDriver));
	//wsprintfA(ach, "COMPMAN: LoadDriver(%ls) (%ldms)\r\n", (LPSTR)icinfo.szDriver, time);
	//OutputDebugStringA(ach);
#endif

	if (pic->hDriver == NULL)
	{
	    pic->dwSmag = 0;
	    ICLeaveCrit(&ICOpenCritSec);
	    return NULL;
	}

	//
	// now try to open the driver as a codec.
	//
	pic->dwDriver = (DWORD) ICSendMessage((HIC)pic, DRV_OPEN, 0, (DWORD_PTR)(LPVOID)&icopen);

	//
	//  we want to be able to install 1.0 draw handlers in SYSTEM.INI as:
	//
	//      VIDS.SMAG = SMAG.DRV
	//
	//  but old driver's may not open iff fccType == 'vids' only if
	//  fccType == 'vidc'
	//
	//  they also may not like ICMODE_DRAW
	//
	if (pic->dwDriver == 0 &&
	    icopen.dwError != 0 &&
	    fccType == streamtypeVIDEO)
	{
	    if (wMode == ICMODE_DRAW)
		icopen.dwFlags = ICMODE_DECOMPRESS;

	    icopen.fccType = ICTYPE_VIDEO;
	    pic->dwDriver = ICSendMessage((HIC)pic, DRV_OPEN, 0, (DWORD_PTR)(LPVOID)&icopen);
	}

	if (pic->dwDriver == 0)
	{
	    ICClose((HIC)pic);
	    ICLeaveCrit(&ICOpenCritSec);
	    return NULL;
	}

	// open'ed ok mark these
	pic->fccType    = fccType;
	pic->fccHandler = fccHandler;
    }
    else if (picT = FindConverter(fccType, fccHandler))
    {
	picT->dwSmag = SMAG;
	dw = ICSendMessage((HIC)picT, DRV_OPEN, 0, (DWORD_PTR)(LPVOID)&icopen);

	if (dw == 0)
	{
	    pic->dwSmag = 0;
	    ICLeaveCrit(&ICOpenCritSec);
	    return NULL;
	}

	*pic = *picT;
	pic->dwDriver = (DWORD) dw;
    }

    ICLeaveCrit(&ICOpenCritSec);
    return (HIC)pic;
}

/*****************************************************************************
 * @doc EXTERNAL IC ICAPPS
 *
 * @api HIC | ICOpenFunction | This function opens
 *      a compressor or decompressor defined as a function.
 *
 * @parm DWORD | fccType | Specifies the type of compressor
 *      the caller is trying to open.  For video, this is ICTYPE_VIDEO.
 *
 * @parm DWORD | fccHandler | Specifies a single preferred handler of the
 *      given type that should be tried first.  Typically, this comes
 *      from the stream header in an AVI file.
 *
 * @parm UINT | wMode | Specifies a flag to defining the use of
 *       the compressor or decompressor.
 *       This parameter can contain one of the following values:
 *
 * @flag ICMODE_COMPRESS | Advises a compressor it is opened for compression.
 *
 * @flag ICMODE_FASTCOMPRESS | Advises a compressor it is open
 *       for fast (real-time) compression.
 *
 * @flag ICMODE_DECOMPRESS | Advises a decompressor it is opened for decompression.
 *
 * @flag ICMODE_FASTDECOMPRESS | Advises a decompressor it is opened
 *       for fast (real-time) decompression.
 *
 * @flag ICMODE_DRAW | Advises a decompressor it is opened
 *       to decompress an image and draw it directly to hardware.
 *
 * @flag ICMODE_QUERY | Advises a compressor or decompressor it is opened
 *       to obtain information.
 *
 * @parm FARPROC | lpfnHandler | Specifies a pointer to the function
 *       used as the compressor or decompressor.
 *
 * @rdesc Returns a handle to a compressor or decompressor
 *        if successful, otherwise it returns zero.
 ****************************************************************************/

HIC VFWAPI ICOpenFunction(DWORD fccType, DWORD fccHandler, UINT wMode, FARPROC lpfnHandler)
{
    ICOPEN      icopen;
    PIC         pic;
    LRESULT     dw;

    if (IsBadCodePtr(lpfnHandler))
	return NULL;

#ifdef NT_THUNK16
    // lpfnHandler points to 16 bit code that will be used as a compressor.
    // We do not want this to go over to the 32 bit side, so only open on
    // the 16 bit side.
#endif // NT_THUNK16

    ICEnterCrit(&ICOpenCritSec);

    AnsiLowerBuff((LPSTR) &fccType, sizeof(DWORD));
    AnsiLowerBuff((LPSTR) &fccHandler, sizeof(DWORD));
    icopen.dwSize  = sizeof(ICOPEN);
    icopen.fccType = fccType;
    icopen.fccHandler = fccHandler;
    icopen.dwFlags = wMode;

    pic = FindConverter(0L, 0L);

    if (pic == NULL) {
	ICLeaveCrit(&ICOpenCritSec);
	return NULL;
    }

    pic->dwSmag   = SMAG;
    pic->fccType  = fccType;
    pic->fccHandler  = fccHandler;
    pic->dwDriver = 0L;
    pic->hDriver  = NULL;
    pic->DriverProc  = (DRIVERPROC)lpfnHandler;

    dw = ICSendMessage((HIC)pic, DRV_OPEN, 0, (DWORD_PTR)(LPVOID)&icopen);

    if (dw == 0)
    {
	ICClose((HIC) pic);
	ICLeaveCrit(&ICOpenCritSec);
	return NULL;
    }

    pic->dwDriver = dw;

    ICLeaveCrit(&ICOpenCritSec);
    return (HIC)pic;
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
/*****************************************************************************
 * @doc EXTERNAL IC ICAPPS
 *
 * @api LRESULT | ICClose | This function closes a compressor or decompressor.
 *
 * @parm HIC | hic | Specifies a handle to a compressor or decompressor.
 *
 * @rdesc Returns ICERR_OK if successful, otherwise it returns an error number.
 *
 ****************************************************************************/

LRESULT VFWAPI ICClose(HIC hic)
{
    PIC pic = (PIC)hic;

    V_HIC(hic);

#ifdef NT_THUNK16
    if (pic->h32 != 0) {
	LRESULT lres = ICClose32(pic->h32);
	pic->h32 = 0;       // Next user of this slot does not want h32 set
	return(lres);
    }
#endif //NT_THUNK16

#ifdef DEBUG
    {
    char ach[80];

    if (pic->hDriver)
	GetModuleFileNameA(GetDriverModuleHandle (pic->hDriver), ach, sizeof(ach));
    else
	ach[0] = 0;

    DPF(("ICClose(%04X) %4.4hs.%4.4hs %s\r\n", hic, (LPSTR)&pic->fccType, (LPSTR)&pic->fccHandler, (LPSTR)ach));
    }
#endif

#ifdef DEBUG
    ICDump();
#endif

    ICEnterCrit(&ICOpenCritSec);

    if (pic->dwDriver)
    {
	if (pic->DriverProc)
	    ICSendMessage((HIC)pic, DRV_CLOSE, 0, 0);
    }

    if (pic->hDriver)
	FreeDriver(pic->hDriver);

    pic->dwSmag   = 0L;
    pic->fccType  = 0L;
    pic->fccHandler  = 0L;
    pic->dwDriver = 0;
    pic->hDriver = NULL;
    pic->DriverProc = NULL;

    ICLeaveCrit(&ICOpenCritSec);

    return ICERR_OK;
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

/****************************************************************
* @doc EXTERNAL IC ICAPPS
*
* @api DWORD | ICCompress | This function compresses a single video
* image.
*
* @parm HIC | hic | Specifies the handle of the compressor to
*       use.
*
* @parm DWORD | dwFlags | Specifies applicable flags for the compression.
*       The following flag is defined:
*
* @flag ICCOMPRESS_KEYFRAME | Indicates that the compressor
*       should make this frame a key frame.
*
* @parm LPBITMAPINFOHEADER | lpbiOutput | Specifies a far pointer
*       to a <t BITMAPINFO> structure holding the output format.
*
* @parm LPVOID | lpData | Specifies a far pointer to output data buffer.
*
* @parm LPBITMAPINFOHEADER | lpbiInput | Specifies a far pointer
*       to a <t BITMAPINFO> structure containing the input format.
*
* @parm LPVOID | lpBits | Specifies a far pointer to the input data buffer.
*
* @parm LPDWORD | lpckid | Not used.
*
* @parm LPDWORD | lpdwFlags | Specifies a far pointer to a <t DWORD>
*       holding the return flags used in the AVI index. The following
*       flag is defined:
*
* @flag AVIIF_KEYFRAME | Indicates this frame should be used as a key-frame.
*
* @parm LONG | lFrameNum | Specifies the frame number.
*
* @parm DWORD | dwFrameSize | Specifies the requested frame size in bytes.
*       If set to zero, the compressor chooses the frame size.
*
* @parm DWORD | dwQuality | Specifies the requested quality value for the frame.
*
* @parm LPBITMAPINFOHEADER | lpbiPrev | Specifies a far pointer to
*       a <t BITMAPINFO> structure holding the previous frame's format.
*       This parameter is not used for fast temporal compression.
*
* @parm LPVOID | lpPrev | Specifies a far pointer to the
*       previous frame's data buffer. This parameter is not used for fast
*       temporal compression.
*
* @comm The <p lpData> buffer should be large enough to hold a compressed
*       frame. You can obtain the size of this buffer by calling
*       <f ICCompressGetSize>.
*
* Set the <p dwFrameSize> parameter to a requested frame
*     size only if the compressor returns the VIDCF_CRUNCH flag in
*     response to <f ICGetInfo>. If this flag is not set, or if a data
*     rate is not specified, set this parameter to zero.
*
*     Set the <p dwQuality> parameter to a quality value only
*     if the compressor returns the VIDCF_QUALITY flag in response
*     to <f ICGetInfo>. Without this flag, set this parameter to zero.
*
* @rdesc This function returns ICERR_OK if successful. Otherwise,
*        it returns an error code.
*
* @xref <f ICCompressBegin> <f ICCompressEnd> <f ICCompressGetSize> <f ICGetInfo>
*
**********************************************************************/
DWORD VFWAPIV ICCompress(
    HIC                 hic,
    DWORD               dwFlags,        // flags
    LPBITMAPINFOHEADER  lpbiOutput,     // output format
    LPVOID              lpData,         // output data
    LPBITMAPINFOHEADER  lpbiInput,      // format of frame to compress
    LPVOID              lpBits,         // frame data to compress
    LPDWORD             lpckid,         // ckid for data in AVI file
    LPDWORD             lpdwFlags,      // flags in the AVI index.
    LONG                lFrameNum,      // frame number of seq.
    DWORD               dwFrameSize,    // reqested size in bytes. (if non zero)
    DWORD               dwQuality,      // quality
    LPBITMAPINFOHEADER  lpbiPrev,       // format of previous frame
    LPVOID              lpPrev)         // previous frame
{
#ifdef _WIN32
    // We cannot rely on the stack alignment giving us the right layout
    ICCOMPRESS icc;
    icc.dwFlags     =  dwFlags;
    icc.lpbiOutput  =  lpbiOutput;
    icc.lpOutput    =  lpData;
    icc.lpbiInput   =  lpbiInput;
    icc.lpInput     =  lpBits;
    icc.lpckid      =  lpckid;
    icc.lpdwFlags   =  lpdwFlags;
    icc.lFrameNum   =  lFrameNum;
    icc.dwFrameSize =  dwFrameSize;
    icc.dwQuality   =  dwQuality;
    icc.lpbiPrev    =  lpbiPrev;
    icc.lpPrev      =  lpPrev;
    return (DWORD) ICSendMessage(hic, ICM_COMPRESS, (DWORD_PTR)(LPVOID)&icc, sizeof(ICCOMPRESS));
    // NOTE: We do NOT copy any results from this temporary structure back
    // to the input variables.
#else
    return ICSendMessage(hic, ICM_COMPRESS, (DWORD_PTR)(LPVOID)&dwFlags, sizeof(ICCOMPRESS));
#endif
}

/************************************************************************

    decompression functions

************************************************************************/

/*******************************************************************
* @doc EXTERNAL IC ICAPPS
*
* @api DWORD | ICDecompress | The function decompresses a single video frame.
*
* @parm HIC | hic | Specifies a handle to the decompressor to use.
*
* @parm DWORD | dwFlags | Specifies applicable flags for decompression.
*       The following flags are defined:
*
* @flag ICDECOMPRESS_HURRYUP | Indicates the decompressor should try to
*       decompress at a faster rate. When an application uses this flag,
*       it should not draw the decompressed data.
*
* @flag ICDECOMPRESS_UPDATE | Indicates that the screen is being updated.
*
* @flag ICDECOMPRESS_PREROLL | Indicates that this frame will not actually
*            be drawn, because it is before the point in the movie where play
*            will start.
*
* @flag ICDECOMPRESS_NULLFRAME | Indicates that this frame does not actually
*            have any data, and the decompressed image should be left the same.
*
* @flag ICDECOMPRESS_NOTKEYFRAME | Indicates that this frame is not a
*            key frame.
*
* @parm LPBITMAPINFOHEADER | lpbiFormat | Specifies a far pointer
*       to a <t BITMAPINFO> structure containing the format of
*       the compressed data.
*
* @parm LPVOID | lpData | Specifies a far pointer to the input data.
*
* @parm LPBITMAPINFOHEADER | lpbi | Specifies a far pointer to a
*       <t BITMAPINFO> structure containing the output format.
*
* @parm LPVOID | lpBits | Specifies a far pointer to a data buffer for the
*       decompressed data.
*
* @comm The <p lpBits> parameter should point to a buffer large
*       enough to hold the decompressed data. Applications can obtain
*       the size of this buffer with <f ICDecompressGetSize>.
*
* @rdesc Returns ICERR_OK on success, otherwise it returns an error code.
*
* @xref <f ICDecompressBegin< <f ICDecompressEnd> <f ICDecompressGetSize>
*
********************************************************************/
DWORD VFWAPIV ICDecompress(
    HIC                 hic,
    DWORD               dwFlags,    // flags (from AVI index...)
    LPBITMAPINFOHEADER  lpbiFormat, // BITMAPINFO of compressed data
				    // biSizeImage has the chunk size
				    // biCompression has the ckid (AVI only)
    LPVOID              lpData,     // data
    LPBITMAPINFOHEADER  lpbi,       // DIB to decompress to
    LPVOID              lpBits)
{
#ifdef _WIN32
    ICDECOMPRESS icd;
    // We cannot rely on the stack alignment giving us the right layout
    icd.dwFlags    = dwFlags;

    icd.lpbiInput  = lpbiFormat;

    icd.lpInput    = lpData;

    icd.lpbiOutput = lpbi;
    icd.lpOutput   = lpBits;
    icd.ckid       = 0;
    return (DWORD) ICSendMessage(hic, ICM_DECOMPRESS, (DWORD_PTR)(LPVOID)&icd, sizeof(ICDECOMPRESS));
#else
    return ICSendMessage(hic, ICM_DECOMPRESS, (DWORD_PTR)(LPVOID)&dwFlags, sizeof(ICDECOMPRESS));
#endif
}

/************************************************************************

    drawing functions

************************************************************************/

/**********************************************************************
* @doc EXTERNAL IC ICAPPS
*
* @api DWORD | ICDrawBegin | This function starts decompressing
* data directly to the screen.
*
* @parm HIC | hic | Specifies a handle to the decompressor to use.
*
* @parm DWORD | dwFlags | Specifies flags for the decompression. The
*       following flags are defined:
*
* @flag ICDRAW_QUERY | Determines if the decompressor can handle
*       the decompression.  The driver does not actually decompress the data.
*
* @flag ICDRAW_FULLSCREEN | Tells the decompressor to draw
*       the decompressed data on the full screen.
*
* @flag ICDRAW_HDC | Indicates the decompressor should use the window
*       handle specified by <p hwnd> and the display context
*       handle specified by <p hdc> for drawing the decompressed data.
*
* @flag ICDRAW_ANIMATE | Indicates the palette might be animated.
*
* @flag ICDRAW_CONTINUE | Indicates drawing is a
*       continuation of the previous frame.
*
* @flag ICDRAW_MEMORYDC | Indicates the display context is offscreen.
*
* @flag ICDRAW_UPDATING | Indicates the frame is being
*       updated rather than played.
*
* @parm HPALETTE | hpal | Specifies a handle to the palette used for drawing.
*
* @parm HWND | hwnd | Specifies a handle for the window used for drawing.
*
* @parm HDC | hdc | Specifies the display context used for drawing.
*
* @parm int | xDst | Specifies the x-position of the upper-right
*       corner of the destination rectangle.
*
* @parm int | yDst | Specifies the y-position of the upper-right
*       corner of the destination rectangle.
*
* @parm int | dxDst | Specifies the width of the destination rectangle.
*
* @parm int | dyDst | Specifies the height of the destination rectangle.
*
* @parm LPBITMAPINFOHEADER | lpbi | Specifies a far pointer to
*       a <t BITMAPINFO> structure containing the format of
*       the input data to be decompressed.
*
* @parm int | xSrc | Specifies the x-position of the upper-right corner
*       of the source rectangle.
*
* @parm int | ySrc | Specifies the y-position of the upper-right corner
*       of the source rectangle.
*
* @parm int | dxSrc | Specifies the width of the source rectangle.
*
* @parm int | dySrc | Specifies the height of the source rectangle.
*
* @parm DWORD | dwRate | Specifies the data rate. The
*       data rate in frames per second equals <p dwRate> divided
*       by <p dwScale>.
*
* @parm DWORD | dwScale | Specifies the data rate.
*
* @comm Decompressors use the <p hwnd> and <p hdc> parameters
*       only if an application sets ICDRAW_HDC flag in <p dwFlags>.
*       It will ignore these parameters if an application sets
*       the ICDRAW_FULLSCREEN flag. When an application uses the
*       ICDRAW_FULLSCREEN flag, it should set <p hwnd> and <p hdc>
*       to NULL.
*
*       The destination rectangle is specified only if ICDRAW_HDC is used.
*       If an application sets the ICDRAW_FULLSCREEN flag, the destination
*       rectangle is ignored and its parameters can be set to zero.
*
*       The source rectangle is relative to the full video frame.
*       The portion of the video frame specified by the source
*       rectangle will be stretched to fit in the destination rectangle.
*
* @rdesc Returns ICERR_OK if it can handle the decompression, otherwise
*        it returns ICERR_UNSUPPORTED.
*
* @xref <f ICDraw> <f ICDrawEnd>
*
*********************************************************************/
DWORD VFWAPIV ICDrawBegin(
    HIC                 hic,
    DWORD               dwFlags,        // flags
    HPALETTE            hpal,           // palette to draw with
    HWND                hwnd,           // window to draw to
    HDC                 hdc,            // HDC to draw to
    int                 xDst,           // destination rectangle
    int                 yDst,
    int                 dxDst,
    int                 dyDst,
    LPBITMAPINFOHEADER  lpbi,           // format of frame to draw
    int                 xSrc,           // source rectangle
    int                 ySrc,
    int                 dxSrc,
    int                 dySrc,
    DWORD               dwRate,         // frames/second = (dwRate/dwScale)
    DWORD               dwScale)
{
#ifdef _WIN32
    ICDRAWBEGIN icdraw;
    icdraw.dwFlags   =  dwFlags;
    icdraw.hpal      =  hpal;
    icdraw.hwnd      =  hwnd;
    icdraw.hdc       =  hdc;
    icdraw.xDst      =  xDst;
    icdraw.yDst      =  yDst;
    icdraw.dxDst     =  dxDst;
    icdraw.dyDst     =  dyDst;
    icdraw.lpbi      =  lpbi;
    icdraw.xSrc      =  xSrc;
    icdraw.ySrc      =  ySrc;
    icdraw.dxSrc     =  dxSrc;
    icdraw.dySrc     =  dySrc;
    icdraw.dwRate    =  dwRate;
    icdraw.dwScale   =  dwScale;

    return (DWORD) ICSendMessage(hic, ICM_DRAW_BEGIN, (DWORD_PTR)(LPVOID)&icdraw, sizeof(ICDRAWBEGIN));
#else
    return ICSendMessage(hic, ICM_DRAW_BEGIN, (DWORD_PTR)(LPVOID)&dwFlags, sizeof(ICDRAWBEGIN));
#endif
}

/**********************************************************************
* @doc EXTERNAL IC ICAPPS
*
* @api DWORD | ICDraw | This function decompress an image for drawing.
*
* @parm HIC | hic | Specifies a handle to an decompressor.
*
* @parm DWORD | dwFlags | Specifies any flags for the decompression.
*       The following flags are defined:
*
* @flag ICDRAW_HURRYUP | Indicates the decompressor should
*       just buffer the data if it needs it for decompression
*       and not draw it to the screen.
*
* @flag ICDRAW_UPDATE | Tells the decompressor to update the screen based
*       on data previously received. Set <p lpData> to NULL when
*       this flag is used.
*
* @flag ICDRAW_PREROLL | Indicates that this frame of video occurs before
*       actual playback should start. For example, if playback is to
*       begin on frame 10, and frame 0 is the nearest previous keyframe,
*       frames 0 through 9 are sent to the driver with the ICDRAW_PREROLL
*       flag set. The driver needs this data so it can displya frmae 10
*       properly, but frames 0 through 9 need not be individually displayed.
*
* @flag ICDRAW_NULLFRAME | Indicates that this frame does not actually
*            have any data, and the previous frame should be redrawn.
*
* @flag ICDRAW_NOTKEYFRAME | Indicates that this frame is not a
*            key frame.
*
* @parm LPVOID | lpFormat | Specifies a far pointer to a
*       <t BITMAPINFOHEADER> structure containing the input
*       format of the data.
*
* @parm LPVOID | lpData | Specifies a far pointer to the actual input data.
*
* @parm DWORD | cbData | Specifies the size of the input data (in bytes).
*
* @parm LONG | lTime | Specifies the time to draw this frame based on the
*       time scale sent with <f ICDrawBegin>.
*
* @comm This function is used to decompress the image data for drawing
* by the decompressor.  Actual drawing of frames does not occur
* until <f ICDrawStart> is called. The application should be sure to
* pre-buffer the required number of frames before drawing is started
* (you can obtain this value with <f ICGetBuffersWanted>).
*
* @rdesc Returns ICERR_OK on success, otherwise it returns an appropriate error
* number.
*
* @xref <f ICDrawBegin> <f ICDrawEnd> <f ICDrawStart> <f ICDrawStop> <f ICGetBuffersRequired>
*
**********************************************************************/
DWORD VFWAPIV ICDraw(
    HIC                 hic,
    DWORD               dwFlags,        // flags
    LPVOID              lpFormat,       // format of frame to decompress
    LPVOID              lpData,         // frame data to decompress
    DWORD               cbData,         // size in bytes of data
    LONG                lTime)          // time to draw this frame (see drawbegin dwRate and dwScale)
{
#ifdef _WIN32
    ICDRAW  icdraw;
    icdraw.dwFlags  =   dwFlags;
    icdraw.lpFormat =   lpFormat;
    icdraw.lpData   =   lpData;
    icdraw.cbData   =   cbData;
    icdraw.lTime    =   lTime;

    return (DWORD) ICSendMessage(hic, ICM_DRAW, (DWORD_PTR)(LPVOID)&icdraw, sizeof(ICDRAW));
#else
    return ICSendMessage(hic, ICM_DRAW, (DWORD_PTR)(LPVOID)&dwFlags, sizeof(ICDRAW));
#endif
}

/*****************************************************************************
 * @doc EXTERNAL IC ICAPPS
 *
 * @api HIC | ICGetDisplayFormat | This function returns the "best"
 *      format available for displaying a compressed image. The function
 *      will also open a compressor if a handle to an open compressor
 *      is not specified.
 *
 * @parm HIC | hic | Specifies the decompressor that should be used.  If
 *      this is NULL, an appropriate compressor will be opened and returned.
 *
 * @parm LPBITMAPINFOHEADER | lpbiIn | Specifies a pointer to
 *       <t BITMAPINFOHEADER> structure containing the compressed format.
 *
 * @parm LPBITMAPINFOHEADER | lpbiOut | Specifies a pointer
 *       to a buffer used to return the decompressed format.
 *            The buffer should be large enough for a <t BITMAPINFOHEADER>
 *       structure and 256 color entries.
 *
 * @parm int | BitDepth | If non-zero, specifies the preferred bit depth.
 *
 * @parm int | dx | If non-zero, specifies the width to which the image
 *      is to be stretched.
 *
 * @parm int | dy | If non-zero, specifies the height to which the image
 *      is to be stretched.
 *
 * @rdesc Returns a handle to a decompressor if successful, otherwise, it
 *        returns zero.
 ****************************************************************************/

HIC VFWAPI ICGetDisplayFormat(HIC hic, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut, int BitDepth, int dx, int dy)
{
    LRESULT dw;
    HDC hdc;
    BOOL fNukeHic = (hic == NULL);
    static int ScreenBitDepth = -1;
    // HACK: We link to some internal DrawDib stuff to find out whether
    // the current display driver is using 565 RGB dibs....
    extern UINT FAR GetBitmapType(VOID);
#define BM_16565        0x06        // most HiDAC cards
#define HACK_565_DEPTH  17

    if (hic == NULL)
	hic = ICDecompressOpen(ICTYPE_VIDEO, 0L, lpbiIn, NULL);

    if (hic == NULL)
	return NULL;

    //
    // dx = 0 and dy = 0 means don't stretch.
    //
    if (dx == (int)lpbiIn->biWidth && dy == (int)lpbiIn->biHeight)
	dx = dy = 0;

    //
    // ask the compressor if it likes the format.
    //
    dw = ICDecompressQuery(hic, lpbiIn, NULL);

    if (dw != ICERR_OK)
    {
	DPF(("Decompressor did not recognize the input data format\r\n"));
	goto error;
    }

try_again:
    //
    //  ask the compressor first. (so it can set the palette)
    //  this is a HACK, we will send the ICM_GET_PALETTE message later.
    //
    dw = ICDecompressGetFormat(hic, lpbiIn, lpbiOut);

    //
    // init the output format
    //
    *lpbiOut = *lpbiIn;
    lpbiOut->biSize = sizeof(BITMAPINFOHEADER);
    lpbiOut->biCompression = BI_RGB;

    //
    // default to the screen depth.
    //
    if (BitDepth == 0)
    {
	if (ScreenBitDepth < 0)
	{
	    hdc = GetDC(NULL);
	    ScreenBitDepth = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);
	    ReleaseDC(NULL, hdc);

	    if (ScreenBitDepth == 15)
		ScreenBitDepth = 16;

	    if (ScreenBitDepth < 8)
		ScreenBitDepth = 8;

	    //
	    // only try 16 bpp if the display supports drawing it.
	    //
	    if (ScreenBitDepth == 16)
	    {
		lpbiOut->biBitCount = 16;

		if (!DrawDibProfileDisplay(lpbiOut))
		    ScreenBitDepth = 24;
	    }

	    if (ScreenBitDepth > 24)
	    {
		lpbiOut->biBitCount = 32;

		if (!DrawDibProfileDisplay(lpbiOut))
		    ScreenBitDepth = 24;
	    }

	    if (ScreenBitDepth == 16 && GetBitmapType() == BM_16565) {
		// If the display is really 565, take this into account.
		ScreenBitDepth = HACK_565_DEPTH;
	    }
	}
#ifdef DEBUG
	ScreenBitDepth = mmGetProfileIntA("DrawDib",
				       "ScreenBitDepth",
				       ScreenBitDepth);
#endif
	BitDepth = ScreenBitDepth;
    }

    //
    //  always try 8bit first for '8' bit data
    //
    if (lpbiIn->biBitCount == 8)
	BitDepth = 8;

    //
    // lets suggest a format to the device.
    //
try_bit_depth:
    if (BitDepth != HACK_565_DEPTH) {
	lpbiOut->biSize = sizeof(BITMAPINFOHEADER);
	lpbiOut->biCompression = BI_RGB;
	lpbiOut->biBitCount = (WORD) BitDepth;
    } else {
#ifndef BI_BITFIELDS
#define BI_BITFIELDS  3L
#endif
	// For RGB565, we need to use BI_BITFIELDS.
	lpbiOut->biSize = sizeof(BITMAPINFOHEADER);
	lpbiOut->biCompression = BI_BITFIELDS;
	lpbiOut->biBitCount = 16;
	((LPDWORD)(lpbiOut+1))[0] = 0x00F800;
	((LPDWORD)(lpbiOut+1))[1] = 0x0007E0;
	((LPDWORD)(lpbiOut+1))[2] = 0x00001F;
	// Set lpbiOut->biClrUsed = 3?
    }

    //
    // should we suggest a stretched decompress
    //
    if (dx > 0 && dy > 0)
    {
	lpbiOut->biWidth  = dx;
	lpbiOut->biHeight = dy;
    }

    lpbiOut->biSizeImage = (DWORD)(UINT)DIBWIDTHBYTES(*lpbiOut) *
			   (DWORD)(UINT)lpbiOut->biHeight;

    //
    // ask the compressor if it likes the suggested format.
    //
    dw = ICDecompressQuery(hic, lpbiIn, lpbiOut);

    //
    // if it likes it then return success.
    //
    if (dw == ICERR_OK)
	goto success;

//  8:   8, 16,24,32,X
//  16:  16,565,24,32,X
//  565: 565,16,24,32,X
//  24:  24,32,16,X
//  32:  32,24,16,X

    //
    // try another bit depth in this order 8,16,RGB565,24,32
    //
    if (BitDepth <= 8)
    {
	BitDepth = 16;
	goto try_bit_depth;
    }

    if (ScreenBitDepth == HACK_565_DEPTH) {
	// If the screen is RGB565, we try 565 before 555.
	if (BitDepth == 16) {
	    BitDepth = 24;
	    goto try_bit_depth;
	}

	if (BitDepth == HACK_565_DEPTH) {
	    BitDepth = 16;
	    goto try_bit_depth;
	}
    }

    if (BitDepth == 16) {
	// otherwise, we try 565 after 555.
	BitDepth = HACK_565_DEPTH;
	goto try_bit_depth;
    }

    if (BitDepth == HACK_565_DEPTH) {
	BitDepth = 24;
	goto try_bit_depth;
    }
	
    if (BitDepth == 24)
    {
	BitDepth = 32;
	goto try_bit_depth;
    }

    if (BitDepth != 32)
    {
	BitDepth = 32;
	goto try_bit_depth;
    }

    if (dx > 0 && dy > 0)
    {
#ifndef DAYTONA // it is not clear that this is correct for Daytona
		// while we work it out disable the code, but match blues
		// as closely as possible.
	//
	// If it's already stretched "pretty big", try decompressing
	// stretched by two, and then stretching/shrinking from there.
	// Otherwise, give up and try decompressing normally.
	//
	if ((dx > (lpbiIn->biWidth * 3) / 2) &&
	    (dy > (lpbiIn->biHeight * 3) / 2) &&
	    ((dx != lpbiIn->biWidth * 2) || (dy != lpbiIn->biHeight * 2))) {
	    dx = (int) lpbiIn->biWidth * 2;
	    dy = (int) lpbiIn->biHeight * 2;
	} else {
	    dx = 0;
	    dy = 0;
	}
	
	//
	// try to find a non stretched format.  but don't let the
	// device dither if we are going to stretch!
	//  - note that this only applies for palettised displays.
	// for 16-bit displays we need to restart to ensure we get the
	// right format (555, 565). On 4-bit displays we can also restart
	// (ask DavidMay about the 4-bit cases).
	//
	    BitDepth = 0;
#else
	    dx = 0;
	    dy = 0;
	    if ((lpbiIn->biBitCount > 8) && (ScreenBitDepth == 8))
		BitDepth = 16;
	    else
		BitDepth = 0;
#endif

	goto try_again;
    }
    else
    {
	//
	// let the compressor suggest a format
	//
	dw = ICDecompressGetFormat(hic, lpbiIn, lpbiOut);

	if (dw == ICERR_OK)
	    goto success;
    }

error:
    if (hic && fNukeHic)
	ICClose(hic);

    return NULL;

success:
    if (lpbiOut->biBitCount == 8)
	ICDecompressGetPalette(hic, lpbiIn, lpbiOut);

    return hic;
}

/*****************************************************************************
 * @doc EXTERNAL IC ICAPPS
 *
 * @api HIC | ICLocate | This function finds a compressor or decompressor
 *      that can handle images with the formats specified, or it finds a
 *      driver that can decompress an image with a specified
 *      format directly to hardware. Applications must close the
 *      compressor when it has finished using the compressor.
 *
 * @parm DWORD | fccType | Specifies the type of compressor
 *      the caller is trying to open.  For video, this is ICTYPE_VIDEO.
 *
 * @parm DWORD | fccHandler | Specifies a single preferred handler of the
 *      given type that should be tried first.  Typically, this comes
 *      from the stream header in an AVI file.
 *
 * @parm LPBITMAPINFOHEADER | lpbiIn | Specifies a pointer to
 *       <t BITMAPINFOHEADER> structure defining the input format.
 *            A compressor handle will not be returned unless it
 *       can handle this format.
 *
 * @parm LPBITMAPINFOHEADER | lpbiOut | Specifies zero or a pointer to
 *       <t BITMAPINFOHEADER> structure defining an optional decompressed
 *            format. If <p lpbiOut> is nonzero, a compressor handle will not
 *       be returned unless it can create this output format.
 *
 * @parm WORD | wFlags | Specifies a flag to defining the use of the compressor.
 *       This parameter must contain one of the following values:
 *
 * @flag ICMODE_COMPRESS | Indicates the compressor should
 *       be able to compress an image with a format defined by <p lpbiIn>
 *       to the format defined by <p lpbiOut>.
 *
 * @flag ICMODE_DECOMPRESS | Indicates the decompressor should
 *       be able to decompress an image with a format defined by <p lpbiIn>
 *       to the format defined by <p lpbiOut>.
 *
 * @flag ICMODE_FASTDECOMPRESS | Has the same definition as ICMODE_DECOMPRESS except the
 *       decompressor is being used for a real-time operation and should trade off speed
 *       for quality if possible.
 *
 * @flag ICMODE_FASTCOMPRESS | Has the same definition as ICMODE_COMPRESS except the
 *       compressor is being used for a real-time operation and should trade off speed
 *       for quality if possible.
 *
 * @flag ICMODE_DRAW | Indicates the decompressor should
 *       be able to decompress an image with a format defined by <p lpbiIn>
 *       and draw it directly to hardware.
 *
 * @rdesc Returns a handle to a compressor or decompressor
 *        if successful, otherwise it returns zero.
 ****************************************************************************/
HIC VFWAPI ICLocate(DWORD fccType, DWORD fccHandler, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut, WORD wFlags)
{
    HIC hic=NULL;
    int i;
    ICINFO icinfo;
    UINT msg;

    if (fccType == 0)
	return NULL;

    switch (wFlags)
    {
	case ICMODE_FASTCOMPRESS:
	case ICMODE_COMPRESS:
	    msg = ICM_COMPRESS_QUERY;
	    break;

	case ICMODE_FASTDECOMPRESS:
	case ICMODE_DECOMPRESS:
	    msg = ICM_DECOMPRESS_QUERY;
	    break;

	case ICMODE_DRAW:
	    msg = ICM_DRAW_QUERY;
	    break;

	default:
	    return NULL;
    }

    if (fccHandler)
    {
	hic = ICOpen(fccType, fccHandler, wFlags);

	if (hic && ICSendMessage(hic, msg, (DWORD_PTR)lpbiIn, (DWORD_PTR)lpbiOut) == ICERR_OK)
	    return hic;
	else if (hic)
	    ICClose(hic);
    }

    if (fccType == ICTYPE_VIDEO && lpbiIn)
    {
	DWORD fccHandler = lpbiIn->biCompression;

	// They're decompressed already.. use our RLE handler so we don't
	// waste time looking for a decompressor or fail and think we don't
	// support these formats!
	if (fccHandler == BI_RLE8 || fccHandler == BI_RGB)
	    fccHandler = mmioFOURCC('M', 'R', 'L', 'E');

	if (fccHandler > 256)
	{
	    if (fccHandler == mmioFOURCC('C', 'R', 'A', 'M'))
		fccHandler = mmioFOURCC('M', 'S', 'V', 'C');
	
	    hic = ICOpen(fccType, fccHandler, wFlags);

	    if (hic && ICSendMessage(hic, msg, (DWORD_PTR)lpbiIn, (DWORD_PTR)lpbiOut) == ICERR_OK)
		return hic;
	    else if (hic)
		ICClose(hic);
	}
    }

    //
    // Search through all of the compressors, to see if one can do what we
    // want.
    //
    for (i=0; ICInfo(fccType, i, &icinfo); i++)
    {
      // Protect against arbitrary 3rd party code crashing us
      try {
	hic = ICOpen(fccType, icinfo.fccHandler, wFlags);

	if (hic == NULL)
	    continue;

	if (ICSendMessage(hic, msg, (DWORD_PTR)lpbiIn, (DWORD_PTR)lpbiOut) != ICERR_OK)
	{
	    ICClose(hic);
	    continue;
	}
	} except (EXCEPTION_EXECUTE_HANDLER) {
	    if (hic) {
		ICClose(hic);
		hic = NULL;
	    }
	}
	if (hic) {
	    return hic;
	}
	return hic;
    }

    return NULL;
}

/*****************************************************************************
 * @doc INTERNAL IC
 *
 * @api HDRVR | LoadDriver | load a driver
 *
 * Note: on chicago, the string szDriver may not be longer than
 *       the number of characters in ICINFO.szDriver
 *
 ****************************************************************************/
#if defined _WIN32
STATICFN HDRVR LoadDriver(LPWSTR szDriver, DRIVERPROC FAR *lpDriverProc)
#else
STATICFN HDRVR LoadDriver(LPSTR szDriver, DRIVERPROC FAR *lpDriverProc)
#endif
{
    HMODULE hModule;
    UINT u;
    DRIVERPROC DriverProc;
    BOOL fWow;
    HDRVR hDriver;

    fWow = IsWow();

    if (fWow)
    {
	u = SetErrorMode(SEM_NOOPENFILEERRORBOX);

       #if defined _WIN32 && ! defined UNICODE
	{
	char ach[NUMELMS(((ICINFO *)0)->szDriver)]; // same size as PICINFO.szDriver

	hModule = LoadLibrary (mmWideToAnsi(ach, szDriver, NUMELMS(ach)));
	}
       #else
	hModule = LoadLibrary(szDriver);
       #endif

	SetErrorMode(u);

	if (hModule <= (HMODULE)HINSTANCE_ERROR)
	    return NULL;
	hDriver = (HDRVR) hModule;
    }
    else
    {
	hDriver = OpenDriver (szDriver, NULL, 0);
	if (!hDriver)
	    return NULL;
	hModule = GetDriverModuleHandle (hDriver);
    }
    DPF(("LoadDriver: %ls, handle %8x   hModule %8x\n", szDriver, hDriver, hModule));

    DriverProc = (DRIVERPROC)GetProcAddress(hModule, szDriverProc);

    if (DriverProc == NULL)
    {
	if (fWow)
	{
	    FreeLibrary(hModule);
	}
	else
	{
	    CloseDriver (hDriver, 0L, 0L);
	}
	DPF(("Freeing library %8x as no driverproc found\r\n",hModule));
	return NULL;
    }

#if ! defined _WIN32
    if (fWow && GetModuleUsage(hModule) == 1)   //!!!this is not exacly like USER
    {
	if (!DriverProc(0, (HDRVR)1, DRV_LOAD, 0L, 0L))
	{
	    DPF(("Freeing library %8x as driverproc returned an error\r\n",hModule));
	    FreeLibrary(hModule);
	    return NULL;
	}

	DriverProc(0, (HDRVR)1, DRV_ENABLE, 0L, 0L);
    }

    CacheModule (hModule);
#endif

    *lpDriverProc = DriverProc;
    return hDriver;
}

/*****************************************************************************
 * @doc INTERNAL IC
 *
 * @api void | FreeDriver | unload a driver
 *
 ****************************************************************************/

STATICFN void FreeDriver(HDRVR hDriver)
{
    if (!IsWow())
    {
	DPF(("FreeDriver, driver handle is %x\n", hDriver));
	CloseDriver (hDriver, 0L, 0L);
    }
#ifndef _WIN32
    else
    {
	// This cannot be WIN32 code due to the definition of IsWow()
	if (GetModuleUsage((HMODULE) hDriver) == 1)
	{
	    DRIVERPROC DriverProc;

	    DriverProc = (DRIVERPROC)GetProcAddress((HMODULE) hDriver, szDriverProc);

	    if (DriverProc)
	    {
		DriverProc(0, (HDRVR)1, DRV_DISABLE, 0L, 0L);
		DriverProc(0, (HDRVR)1, DRV_FREE, 0L, 0L);
	    }
	}

	FreeLibrary((HMODULE) hDriver);
	DPF(("Freeing library %8x in FreeDriver\r\n",hDriver));
    }
#endif
}

#ifdef DEBUG_RETAIL

/************************************************************************

    messages.

************************************************************************/

static const struct {
    UINT  msg;
    char *szMsg;
}   aMsg[] = {

DRV_OPEN                        , "DRV_OPEN",
DRV_CLOSE                       , "DRV_CLOSE",
ICM_GETSTATE                    , "ICM_GETSTATE",
ICM_SETSTATE                    , "ICM_SETSTATE",
ICM_GETINFO                     , "ICM_GETINFO",
ICM_CONFIGURE                   , "ICM_CONFIGURE",
ICM_ABOUT                       , "ICM_ABOUT",
ICM_GETERRORTEXT                , "ICM_GETERRORTEXT",
ICM_GETFORMATNAME               , "ICM_GETFORMATNAME",
ICM_ENUMFORMATS                 , "ICM_ENUMFORMATS",
ICM_GETDEFAULTQUALITY           , "ICM_GETDEFAULTQUALITY",
ICM_GETQUALITY                  , "ICM_GETQUALITY",
ICM_SETQUALITY                  , "ICM_SETQUALITY",
ICM_COMPRESS_GET_FORMAT         , "ICM_COMPRESS_GET_FORMAT",
ICM_COMPRESS_GET_SIZE           , "ICM_COMPRESS_GET_SIZE",
ICM_COMPRESS_QUERY              , "ICM_COMPRESS_QUERY",
ICM_COMPRESS_BEGIN              , "ICM_COMPRESS_BEGIN",
ICM_COMPRESS                    , "ICM_COMPRESS",
ICM_COMPRESS_END                , "ICM_COMPRESS_END",
ICM_DECOMPRESS_GET_FORMAT       , "ICM_DECOMPRESS_GET_FORMAT",
ICM_DECOMPRESS_QUERY            , "ICM_DECOMPRESS_QUERY",
ICM_DECOMPRESS_BEGIN            , "ICM_DECOMPRESS_BEGIN",
ICM_DECOMPRESS                  , "ICM_DECOMPRESS",
ICM_DECOMPRESS_END              , "ICM_DECOMPRESS_END",
ICM_DECOMPRESS_GET_PALETTE      , "ICM_DECOMPRESS_GET_PALETTE",
ICM_DECOMPRESS_SET_PALETTE      , "ICM_DECOMPRESS_SET_PALETTE",
ICM_DECOMPRESSEX_QUERY          , "ICM_DECOMPRESSEX_QUERY",
ICM_DECOMPRESSEX_BEGIN          , "ICM_DECOMPRESSEX_BEGIN",
ICM_DECOMPRESSEX                , "ICM_DECOMPRESSEX",
ICM_DECOMPRESSEX_END            , "ICM_DECOMPRESSEX_END",
ICM_DRAW_QUERY                  , "ICM_DRAW_QUERY",
ICM_DRAW_BEGIN                  , "ICM_DRAW_BEGIN",
ICM_DRAW_GET_PALETTE            , "ICM_DRAW_GET_PALETTE",
ICM_DRAW_UPDATE                 , "ICM_DRAW_UPDATE",
ICM_DRAW_START                  , "ICM_DRAW_START",
ICM_DRAW_STOP                   , "ICM_DRAW_STOP",
ICM_DRAW_BITS                   , "ICM_DRAW_BITS",
ICM_DRAW_END                    , "ICM_DRAW_END",
ICM_DRAW_GETTIME                , "ICM_DRAW_GETTIME",
ICM_DRAW                        , "ICM_DRAW",
ICM_DRAW_WINDOW                 , "ICM_DRAW_WINDOW",
ICM_DRAW_SETTIME                , "ICM_DRAW_SETTIME",
ICM_DRAW_REALIZE                , "ICM_DRAW_REALIZE",
ICM_GETBUFFERSWANTED            , "ICM_GETBUFFERSWANTED",
ICM_GETDEFAULTKEYFRAMERATE      , "ICM_GETDEFAULTKEYFRAMERATE",
0                               , NULL
};

static const struct {
    LRESULT err;
    char *szErr;
}   aErr[] = {

ICERR_DONTDRAW              , "ICERR_DONTDRAW",
ICERR_NEWPALETTE            , "ICERR_NEWPALETTE",
ICERR_UNSUPPORTED           , "ICERR_UNSUPPORTED",
ICERR_BADFORMAT             , "ICERR_BADFORMAT",
ICERR_MEMORY                , "ICERR_MEMORY",
ICERR_INTERNAL              , "ICERR_INTERNAL",
ICERR_BADFLAGS              , "ICERR_BADFLAGS",
ICERR_BADPARAM              , "ICERR_BADPARAM",
ICERR_BADSIZE               , "ICERR_BADSIZE",
ICERR_BADHANDLE             , "ICERR_BADHANDLE",
ICERR_CANTUPDATE            , "ICERR_CANTUPDATE",
ICERR_ERROR                 , "ICERR_ERROR",
ICERR_BADBITDEPTH           , "ICERR_BADBITDEPTH",
ICERR_BADIMAGESIZE          , "ICERR_BADIMAGESIZE",
ICERR_OK                    , "ICERR_OK"
};

STATICDT BOOL  cmfDebug = -1;
STATICDT DWORD dwTime;

void ICDebugMessage(HIC hic, UINT msg, DWORD_PTR dw1, DWORD_PTR dw2)
{
    int i;

    if (!cmfDebug)
	return;

    for (i=0; aMsg[i].msg && aMsg[i].msg != msg; i++)
	;

    if (aMsg[i].msg == 0)
	RPF(("ICM(%04X,ICM_%04X,%08lX,%08lX) ", hic, msg, dw1, dw2));
    else
	RPF(("ICM(%04X,%s,%08lX,%08lX) ", hic, (LPSTR)aMsg[i].szMsg, dw1, dw2));

    dwTime = timeGetTime();
}

LRESULT ICDebugReturn(LRESULT err)
{
    int i;

    if (!cmfDebug)
	return err;

    dwTime = timeGetTime() - dwTime;

    for (i=0; aErr[i].err && aErr[i].err != err; i++)
	;

    if (aErr[i].err != err)
	RPF(("! : 0x%08lX (%ldms)\r\n", err, dwTime));
    else
	RPF(("! : %s (%ldms)\r\n", (LPSTR)aErr[i].szErr, dwTime));

    return err;
}

STATICFN void ICDump()
{
    int i;
    PIC pic;
    TCHAR ach[80];

    DPF(("ICDump ---------------------------------------\r\n"));

    for (i=0; i<giMaxConverters; i++)
    {
	pic = &aicConverters[i];

	if (pic->fccType == 0)
	    continue;

	if (pic->dwSmag == 0)
	    continue;

	if (pic->hDriver)
	    GetModuleFileName(GetDriverModuleHandle (pic->hDriver), ach, NUMELMS(ach));
	else
	    ach[0] = 0;

#ifdef _WIN32
	DPF(("  HIC: %04X %4.4hs.%4.4hs hTask=%04X Proc=%08lx %ls\r\n", (HIC)pic, (LPSTR)&pic->fccType, (LPSTR)&pic->fccHandler, pic->hTask, pic->DriverProc, ach));
#else
	DPF(("  HIC: %04X %4.4s.%4.4s hTask=%04X Proc=%08lx %s\r\n", (HIC)pic, (LPSTR)&pic->fccType, (LPSTR)&pic->fccHandler, pic->hTask, pic->DriverProc, (LPSTR)ach));
#endif
    }

    DPF(("----------------------------------------------\r\n"));
}

#endif

/*****************************************************************************
 *
 * dprintf() is called by the DPF macro if DEBUG is defined at compile time.
 *
 * The messages will be send to COM1: like any debug message. To
 * enable debug output, add the following to WIN.INI :
 *
 * [debug]
 * COMPMAN=1
 *
 ****************************************************************************/

char szDebug[] = "Debug";
#ifdef DEBUG_RETAIL


STATICFN void cdecl dprintfc(LPSTR szFormat, ...)
{
    char ach[128];

#ifdef _WIN32
    va_list va;
    if (cmfDebug == -1)
	cmfDebug = mmGetProfileIntA(szDebug, MODNAME, 0);

    if (!cmfDebug)
	return;

    va_start(va, szFormat);
    if (szFormat[0] == '!')
	ach[0]=0, szFormat++;
    else
	wsprintfA(ach, "%s: (tid %x) ", MODNAME, GetCurrentThreadId());

    wvsprintfA(ach+lstrlenA(ach),szFormat,va);
    va_end(va);
//  lstrcatA(ach, "\r\r\n");
#else  // Following is WIN16 code...
    if (cmfDebug == -1)
	cmfDebug = GetProfileIntA("Debug",MODNAME, 0);

    if (!cmfDebug)
	return;

    if (szFormat[0] == '!')
	ach[0]=0, szFormat++;
    else
	lstrcpyA(ach, MODNAME ": ");

    wvsprintfA(ach+lstrlenA(ach),szFormat,(LPSTR)(&szFormat+1));
//  lstrcatA(ach, "\r\r\n");
#endif

    OutputDebugStringA(ach);
}

#endif

#ifdef _WIN32
#define FADMIN_NOT_CACHED 15
int fIsAdmin = FADMIN_NOT_CACHED;   // any arbitrary value that will not be TRUE or FALSE
BOOL IsAdmin(void)
{
    BOOL IsMember;
    SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
    PSID        AdminSid;

#ifdef DEBUG_RETAIL
    // see if we should run as a normal user.
    // ADMINs can pretend to be normal users; vice versa does not work
    // Hence you have to pass the security checks below to be recognised
    // as an admin
    if (mmGetProfileIntA(MODNAME, "NormalUser", FALSE)) {
	DPF(("Forcing NON admin"));
	return(FALSE);
    }
#endif

    // If we have cached a value, return the cached value
    if (FADMIN_NOT_CACHED != fIsAdmin) {
	return(fIsAdmin);
    }

    if (!AllocateAndInitializeSid(&sia,                            // identifier authority
				  2,                               // subauthority count
				  SECURITY_BUILTIN_DOMAIN_RID,     // subauthority 0
				  DOMAIN_ALIAS_RID_ADMINS,         // subauthority 1
				  0,0,0,0,0,0,                     // subauthority 2-7
				  &AdminSid)) {                     // result target
	//
	// Failed, don't assume we are an admin.
	//

	return FALSE;
    } else if (!CheckTokenMembership(NULL,
				AdminSid,
				&IsMember)) {
	//
	// Failed, don't assume we are an admin.
	//

	FreeSid(AdminSid);

	return FALSE;
    } else {
	//
	// We have a definitive answer, set the cached value.
	//

	fIsAdmin = IsMember;

	FreeSid(AdminSid);
	
	return fIsAdmin;
    }

    // NOT REACHED

    return FALSE;
}
#endif // _WIN32

#ifdef DAYTONA

#define KEYSECTION TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\")


LONG OpenUserKey(PHKEY pKey, LPCTSTR lp, LPCTSTR pSection)
{
    DWORD disposition;
    TCHAR section[256];
    lstrcpy(section, KEYSECTION);
    lstrcat(section, pSection);
    if (lp) {
	// Note: we should never need to create the user section in order
	// to query the data.  If the section does not exist, then nothing is
	// the right thing to return.
	return(RegCreateKeyExW(HKEY_CURRENT_USER, section, 0, NULL, 0,
				    KEY_SET_VALUE, NULL, pKey, &disposition));
    } else {
	// We are only reading what is there...
	return(RegOpenKeyExW(HKEY_CURRENT_USER, section, 0, KEY_QUERY_VALUE, pKey));
    }
}

LONG OpenSystemKey(PHKEY pKey, LPCTSTR lp, LPCTSTR pSection)
{
    DWORD disposition;
    TCHAR section[256];
    lstrcpy(section, KEYSECTION);
    lstrcat(section, pSection);
    if (lp) {
	return(RegCreateKeyExW(HKEY_LOCAL_MACHINE, section, 0, NULL, 0,
		    KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, pKey, &disposition));
    } else {
	// We are only reading what is there...
	return(RegOpenKeyExW(HKEY_LOCAL_MACHINE, section, 0, KEY_QUERY_VALUE, pKey));
    }
}


// Use a mapping to get the stuff into the registry
BOOL myWritePrivateProfileString(

    LPCTSTR  lpszSection,       // address of section name
    LPCTSTR  lpszKeyName,       // address of key name
    LPCTSTR  lpszString         // address of string to add
)
{
	if (IsAdmin()) {
	    return WritePrivateProfileString(lpszSection, lpszKeyName, lpszString, gszSystemIni);
	} else /* NOT ADMIN */ {
	    // write to HKEY_CURRENT_USER
	    // Data is written to

	    HKEY key;

	    // If we have something to write, then we must create the key
	    // If we are about to delete something that might not exist we only
	    // want to open the key.  Hence OpenUserKey needs to know if lpszString is NULL
	    if (ERROR_SUCCESS == OpenUserKey(&key, lpszString, lpszSection)) {
		
		// We have access.  Now write the data
		if (lpszString) {
		    LPCTSTR lpStr;
		    RegSetValueEx(key, lpszKeyName, 0, REG_SZ,
			(LPCVOID)lpszString, sizeof(TCHAR)*(lstrlen(lpszString)+1));
		    lpStr = lpszString + 1 + lstrlen(lpszString);

		    // Is there an associated description ??
		    if (*(lpStr)) {
			HKEY key2;
			DWORD disposition;
			TCHAR section[256];
			// Write the description
			lstrcpy(section, KEYSECTION);
			lstrcat(section, TEXT("Drivers.desc"));
			if (ERROR_SUCCESS ==
			    (RegCreateKeyExW(HKEY_CURRENT_USER, section, 0, NULL, 0,
						KEY_SET_VALUE, NULL, &key2, &disposition))) {

			    RegSetValueEx(key2, lpszString, 0, REG_SZ,
				(LPCVOID)lpStr, sizeof(TCHAR)*(lstrlen(lpStr)+1));
			    RegCloseKey(key2);
			}
		    }
		} else {
		    // delete the data
		    RegDeleteValue(key, lpszKeyName);
		}

		RegCloseKey(key);
		return(TRUE);
	    }
	    return(FALSE);
	}

}


DWORD myGetPrivateProfileString(

    LPCTSTR  lpszSection,       // address of section name
    LPCTSTR  lpszKey,           // address of key name
    LPCTSTR  lpszDefault,       // address of default string
    LPTSTR  lpszReturnBuffer,   // address of destination buffer
    DWORD  cchReturnBuffer)     // size of destination buffer
{
    // Whether we are an admin or not we have to read data from HKEY_CURRENT_USER
    // first as that overrides the SYSTEM installed details.
    // If we are enumerating the section, then we want to delete duplicate definitions
    // in the system block.  This is where the complexity enters.

    DWORD dwType;
    HKEY key;
    UINT nSize;
    UINT nRet=ERROR_NO_MORE_ITEMS;
    LPTSTR lpBuf;
    LPVOID lpEnd;
    UINT size = cchReturnBuffer * sizeof(TCHAR);

    lpBuf = lpszReturnBuffer;
    lpEnd = ((LPBYTE)lpBuf)+size;

#define CUSERDRIVERS 20

    if (!lpszKey) {

	// Will enumerate the list of installed drivers - first USER
	// then SYSTEM.  User installed drivers take precendence (if
	// there is duplication).  We used to read the whole section,
	// but we are only interested in fcctype.fcchandler type entries.
	// Therefore, any that do not match this pattern are skipped.

	UINT   cch1, count;
	TCHAR section[256];
	LPTSTR aszUserDrivers[CUSERDRIVERS];
	UINT cUserDrivers=0;
	UINT iKey;

#ifdef DEBUG
	memset(lpszReturnBuffer, 0xfe, cchReturnBuffer*sizeof(TCHAR));
	// verify that we do not write more data than we should
#endif

#if 0
// This is the old code.  Use this to verify that the registry enumeration is
// correct.
	// Check that the registry enumeration code produces the same result
	cch1 = GetPrivateProfileString(lpszSection, lpszKey, lpszDefault,
			    lpszReturnBuffer, cchReturnBuffer, gszSystemIni);
#endif
	// Read the user section
	// then read the system section, skipping codecs already found
	if (ERROR_SUCCESS == OpenUserKey(&key, NULL, lpszSection)) {

	    for (iKey = 0; ; ++iKey) {

		// Calculate - in characters - how much space is left in the
		// name buffer
		nSize = (UINT) (UINT_PTR) ((LPTSTR)lpEnd-lpBuf);

		// Enumerate the name.  We do not at this point need the values
		// associated with the names, only the list of installed driver types
		nRet = RegEnumValue(key, iKey, lpBuf, &nSize, NULL,
			    &dwType, NULL, NULL);
		if (nRet!= ERROR_SUCCESS) break;  // bail out

		// If this is not xxxx.yyyy then ignore it.
		if ((nSize != sizeof(FOURCC)+sizeof(FOURCC)+1)
		   || (lpBuf[4] != TEXT('.')))
		{
		    continue;
		}
		
		if (cUserDrivers<CUSERDRIVERS) {
		    // Remember the name of this driver
		    aszUserDrivers[cUserDrivers++] = lpBuf;
		} else {
		    // Too many user installed drivers... let there be duplicates
		}
		lpBuf += nSize+1;  // Step over this name and its terminating null
	    }
	    RegCloseKey(key);
	}

	// Unless we ran out of room we need to read the system section
	if (nRet == ERROR_NO_MORE_ITEMS)
	if (ERROR_SUCCESS == OpenSystemKey(&key, NULL, lpszSection)) {

	    for (iKey = 0; ; ++iKey) {

		// Calculate - in characters - how much space is left in the
		// name buffer
		nSize = (DWORD) (DWORD_PTR) ((LPTSTR)lpEnd-lpBuf);

		// Enumerate the name.  We do not at this point need the values
		// associated with the names, only the list of installed driver types
		nRet = RegEnumValue(key, iKey, lpBuf, &nSize, NULL,
			    &dwType, NULL, NULL);
		if (nRet!= ERROR_SUCCESS) break;  // bail out

		// If this is not xxxx.yyyy then ignore it.
		if (nSize != sizeof(FOURCC)+sizeof(FOURCC)+1) {
		    continue;
		}
		
		// If we have already found this driver in the user section, then
		// ignore the system definition
		for (count=0; count<cUserDrivers; ++count) {
		    if (0 == lstrcmpi(lpBuf, aszUserDrivers[count])) {
			*lpBuf = 0;       // wipeout the last value
			goto skipped;
		    }
		}
		lpBuf += nSize+1;  // Step over this name and its terminating null
		skipped: ;
	    }
	    RegCloseKey(key);
	}

	if (ERROR_MORE_DATA == nRet) {
	    // we ran out of room
	    nSize = cchReturnBuffer-2;  // same return as GetPrivateProfileString
	} else if (nRet == ERROR_NO_MORE_ITEMS) {
	    // Success.  Calculate the number of characters in the buffer
	    *lpBuf = 0;  // Write a second terminating zero
	    // Now calculate how many characters we are returning, excluding one
	    // of the two terminating zeros
	    nSize = (DWORD) (DWORD_PTR) (lpBuf-lpszReturnBuffer);
	} else {
	    // something went wrong.  No data, or another error, return nothing
	    // Make sure the buffer has a double terminating null
	    *lpBuf++ = 0;
	    *lpBuf++ = 0;
	    nSize=0;
	}

	return(nSize);
    } else {
	// Not enumerating.  We have a specific value to look for
	if (ERROR_SUCCESS == OpenUserKey(&key, NULL, lpszSection)) {

	    // Calculate - in bytes - how much space is in the buffer
	    nSize = (DWORD) (DWORD_PTR) ((LPBYTE)lpEnd-(LPBYTE)lpBuf);

	    // Get the data
	    nRet = RegQueryValueEx(key, lpszKey, NULL,
					&dwType, (LPBYTE)lpBuf, &nSize);
	    RegCloseKey(key);
	}
	// If we could not find the data in the user key then try SYSTEM
	if ((ERROR_SUCCESS != nRet) && (ERROR_MORE_DATA != nRet)) {
	    // Try the system key
	    if (ERROR_SUCCESS == OpenSystemKey(&key, NULL, lpszSection)) {

		// Calculate - in bytes - how much space is in the buffer
		nSize = (DWORD) (DWORD_PTR) ((LPBYTE)lpEnd-(LPBYTE)lpBuf);

		// Get the data
		nRet = RegQueryValueEx(key, lpszKey, NULL,
					&dwType, (LPBYTE)lpBuf, &nSize);
		RegCloseKey(key);
	    }
	}
	if (ERROR_MORE_DATA == nRet) {
	    return(cchReturnBuffer-1);  // not enough room for the data
	}
	if (nRet != ERROR_SUCCESS) {
	    return(0);  // cannot find the data
	}
	if (REG_SZ != dwType) {
	    return(0);  // we must have string data
	}
	//RegQueryValueEx returns length in bytes and includes the terminating zero
	return (DWORD) (nSize/sizeof(TCHAR) - 1);
    }
}

#endif
