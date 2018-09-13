/*
 * This code contains thunk enabling.  If we fail to open on the 16 bit side,
 * we will try and open a 32 bit codec.  (The reason for not trying the 32
 * bit codec first is an attempt to keep most things on the 16 bit side.
 * The performance under NT appears reasonable, and for frame specific
 * operations it reduces the number of 16/32 transitions.
 */


#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <win32.h>
#ifdef WIN32
#include <mmddk.h>  // needed for definition of DRIVERS_SECTION
#endif

//
// define these before compman.h, so our functions get declared right.
//
#ifndef WIN32
#define VFWAPI  FAR PASCAL _loadds
#define VFWAPIV FAR CDECL  _loadds
#endif

#include "compman.h"
#include "icm.rc"

#ifdef WIN32
#include <wchar.h>
#endif

#ifndef NOTHUNKS
#include "thunks.h"    // Building
#endif //NOTHUNKS

#ifndef streamtypeVIDEO
    #define streamtypeVIDEO mmioFOURCC('v', 'i', 'd', 's')
#endif

#define ICTYPE_VCAP mmioFOURCC('v', 'c', 'a', 'p')
#define ICTYPE_ACM  mmioFOURCC('a', 'u', 'd', 'c')
#define SMAG        mmioFOURCC('S', 'm', 'a', 'g')
#define GONE        mmioFOURCC('G', 'o', 'n', 'e')

#define IC_INI      TEXT("Installable Compressors")

static TCHAR   szIniSect[]       = IC_INI;
static TCHAR   szDrivers[]       = DRIVERS_SECTION;
static TCHAR   szSystemIni[]     = TEXT("SYSTEM.INI");
static TCHAR   szNull[]          = TEXT("");
static TCHAR   sz44s[]           = TEXT("%4.4hs");
static TCHAR   szICKey[]         = TEXT("%4.4hs.%4.4hs");
static TCHAR   szMSVideo[]       = TEXT("MSVideo");
static TCHAR   szMSACM[]         = TEXT("MSACM");
static TCHAR   szVIDC[]          = TEXT("VIDC");
static SZCODEA szDriverProc[]    = "DriverProc";

#ifdef DEBUG
    #define DPF( x ) dprintfc x
    #define DEBUG_RETAIL
#else
    #define DPF(x)
#endif

#ifdef DEBUG_RETAIL
    static void CDECL dprintfc(LPSTR, ...);
    #define RPF( x ) dprintfc x
    #define ROUT(sz) {static SZCODE ach[] = sz; dprintfc(ach); }
    void  ICDebugMessage(HIC hic, UINT msg, DWORD dw1, DWORD dw2);
    LRESULT ICDebugReturn(LRESULT err);
    #define DebugErr(sz) {static SZCODE ach[] = "COMPMAN: "sz; DebugOutput(DBF_ERROR | DBF_MMSYSTEM, ach); }
#else
    #define RPF(x)
    #define ROUT(sz)
    #define ICDebugMessage(hic, msg, dw1, dw2)
    #define ICDebugReturn(err)  err
    #define DebugErr(sz)
#endif

#ifdef DEBUG_RETAIL
#else
    #define DebugErr(flags)
#endif

#ifndef WF_WINNT
#define WF_WINNT 0x4000
#endif

#ifdef WIN32
#define IsWow() FALSE
#else
#define IsWow() ((BOOL) (GetWinFlags() & WF_WINNT))
#define GetDriverModuleHandle(h) (IsWow() ? h : GetDriverModuleHandle(h))
#endif

__inline void ictokey(DWORD fccType, DWORD fcc, LPSTR sz)
{
    int i = wsprintf(sz, szICKey, (LPSTR)&(fccType),(LPSTR)&(fcc));

    while (i>0 && sz[i-1] == ' ')
	sz[--i] = 0;
}

#define WIDTHBYTES(i)     ((unsigned)((i+31)&(~31))/8)  /* ULONG aligned ! */
#define DIBWIDTHBYTES(bi) (int)WIDTHBYTES((int)(bi).biWidth * (int)(bi).biBitCount)

static void ICDump(void);


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
    DWORD       dwDriver;           // driver id for functions
    DRIVERPROC  DriverProc;         // function to call
#ifndef NOTHUNKS
    DWORD       h32;                // 32-bit driver handle
#endif //!NOTHUNKS
}   IC, *PIC;

IC aicConverters[MAX_CONVERTERS];

/*****************************************************************************
 ****************************************************************************/

LRESULT CALLBACK DriverProcNull(DWORD dwDriverID, HANDLE hDriver, UINT wMessage,DWORD dwParam1, DWORD dwParam2)
{
    DPF(("codec called after it has been removed with ICRemove\r\n"));
    return ICERR_UNSUPPORTED;
}

/*****************************************************************************
 ****************************************************************************/

static HDRVR LoadDriver(LPSTR szDriver, DRIVERPROC FAR *lpDriverProc);
static void FreeDriver(HDRVR hDriver);

/*****************************************************************************

    driver cache - to make enuming/loading faster we keep the last N
    module's open for a while.

 ****************************************************************************/

#define N_MODULES   10      //!!!????

HMODULE ahModule[N_MODULES];
int     iModule = 0;

static void CacheModule(HMODULE hModule)
{
    TCHAR ach[128];

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
#ifndef WIN32  // On NT GetModuleUsage always returns 1.  So... we cache
    if (hModule)
    {
	extern HMODULE ghInst;          // in MSVIDEO/init.c
	int iUsage;

	GetModuleFileName(hModule, ach, sizeof(ach));
	DPF(("Loading module: %s\r\n", (LPSTR)ach));
	iUsage = GetModuleUsage(ghInst);
	LoadLibrary(ach);

	//
	// dont cache modules that link to MSVIDEO
	// we should realy do a toolhelp thing!
	// or force apps to call VFWInit and VFWExit()
	//
	if (iUsage != GetModuleUsage(ghInst))
	{
	    DPF(("Not caching this module because it links to MSVIDEO\r\n"));
	    FreeLibrary(hModule);
	    return;
	}
    }
#endif

    //
    // free module in our slot.
    //
    if (ahModule[iModule] != NULL)
    {
#ifdef DEBUG
	GetModuleFileName(ahModule[iModule], ach, sizeof(ach));
	DPF(("Freeing module: %s\r\n", (LPSTR)ach));
#endif
	FreeLibrary(ahModule[iModule]);
    }

    ahModule[iModule] = hModule;
    iModule++;

    if (iModule >= N_MODULES)
	iModule = 0;
}


/*****************************************************************************
 ****************************************************************************/

/*****************************************************************************
 * FixFOURCC - clean up a FOURCC
 ****************************************************************************/

static DWORD Fix4CC(DWORD fcc)
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

static PIC FindConverter(DWORD fccType, DWORD fccHandler)
{
    int i;
    PIC pic;

    for (i=0; i<MAX_CONVERTERS; i++)
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
                DPF(("NO driver for fccType=%4.4s, Handler=%4.4s\n", (LPSTR)&fccType, (LPSTR)&fccHandler));
		return NULL;
	    }
            DPF(("Possible driver for fccType=%4.4s, Handler=%4.4s\n", (LPSTR)&fccType, (LPSTR)&fccHandler));
	    return pic;
	}
    }

    return NULL;
}

#ifdef WIN32
/*
 * we need to hold a critical section around the ICOpen code to protect
 * multi-thread simultaneous opens. This critsec is initialized by
 * IC_Load (called from video\init.c at dll attach time) and is deleted
 * by IC_Unload (called from video\init.c at dll detach time).
 */
CRITICAL_SECTION ICOpenCritSec;

void
IC_Load(void)
{
    InitializeCriticalSection(&ICOpenCritSec);
}

void
IC_Unload(void)
{
    DeleteCriticalSection(&ICOpenCritSec);
}

#define ICEnterCrit(p)  (EnterCriticalSection(p))
#define ICLeaveCrit(p)  (LeaveCriticalSection(p))

#else

// non-win32 code has no critsecs
#define ICEnterCrit(p)
#define ICLeaveCrit(p)

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
	DebugErr("Invalid HIC\r\n");
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
    for (i=0; i < MAX_CONVERTERS; i++)
    {
	pic = &aicConverters[i];

	if (pic->dwDriver != 0L && (pic->hTask == hTask || hTask == NULL))
	{
	    ROUT("Decompressor left open, closing\r\n");
	    ICClose((HIC)pic);
	}
    }

    //
    // free the module cache.
    //
    for (i=0; i<N_MODULES; i++)
	CacheModule(NULL);
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
    TCHAR buf[128];
    PIC  pic;

    ICEnterCrit(&ICOpenCritSec);
    fccType    = Fix4CC(fccType);
    fccHandler = Fix4CC(fccHandler);

    DPF(("ICInstall, fccType=%4.4s, Handler=%4.4s, >>%s<<\n", (LPSTR)&fccType, (LPSTR)&fccHandler, szDesc));
    if ((pic = FindConverter(fccType, fccHandler)) == NULL)
	pic = FindConverter(0L, 0L);

    if (wFlags & ICINSTALL_DRIVER)
    {
	//
	//  dwConverter is the file name of a driver to install.
	//
	ictokey(fccType, fccHandler, achKey);

#ifdef WIN32
	if (szDesc)
	    wsprintf(buf, TEXT("%hs %hs"), (LPSTR) lParam, szDesc);
	else
	    wsprintf(buf, TEXT("%hs"), (LPSTR) lParam);
#else
	lstrcpy(buf, (LPSTR)lParam);

	if (szDesc)
	{
	    lstrcat(buf, TEXT(" "));
	    lstrcat(buf, szDesc);
	}
#endif

	ICLeaveCrit(&ICOpenCritSec);
	if (WritePrivateProfileString(szDrivers,achKey,buf,szSystemIni))
	{
	    WritePrivateProfileString(szIniSect,achKey,NULL,szSystemIni);
	    return TRUE;
	}
	else
	{
	    return(FALSE);
	}
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

    DPF(("ICRemove, fccType=%4.4s, Handler=%4.4s\n", (LPSTR)&fccType, (LPSTR)&fccHandler));
    if (pic = FindConverter(fccType, fccHandler))
    {
	int i;

	//
	// we should realy keep usage counts!!!
	//
	for (i=0; i<MAX_CONVERTERS; i++)
	{
	    if (pic->DriverProc == aicConverters[i].DriverProc)
	    {
		DPF(("ACK! Handler is in use\r\n"));
		pic->DriverProc = DriverProcNull;
	    }
	}

	ICClose((HIC)pic);
    }
    else
    {
	ictokey(fccType, fccHandler, achKey);
	WritePrivateProfileString(szIniSect,achKey,NULL,szSystemIni);
	WritePrivateProfileString(szDrivers,achKey,NULL,szSystemIni);
    }

    ICLeaveCrit(&ICOpenCritSec);

    return TRUE;
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

#ifndef NOTHUNKS
BOOL VFWAPI ICInfoInternal(DWORD fccType, DWORD fccHandler, ICINFO FAR * lpicinfo);

// If we are compiling the thunks, then the ICINFO entry point calls
// the 32 bit thunk, or calls the real ICInfo code (as ICInfoInternal).
// We deliberately give precedence to 32 bit compressors, although this
// ordering can be trivially changed.
// ??: Should we allow an INI setting to change the order?

BOOL VFWAPI ICInfo(DWORD fccType, DWORD fccHandler, ICINFO FAR * lpicinfo)
{
    BOOL fResult;

    fResult = (ICInfo32(fccType, fccHandler, lpicinfo));
    DPF(("ICInfo32 returned %ls\r\n", (fResult ? (LPSTR)"TRUE" : (LPSTR)"FALSE")));
    if (fResult) return fResult;

    // If we are enumerating the drivers, then we want to adjust the 16
    // bit index by the count of 32 bit drivers.  The thunk will have
    // passed back the number of 32 bit drivers installed in [Drivers32]
    // in ICINFO.fccHandler.
    if ((fccType==0) || (fccHandler < 256)) {
	DPF(("Enumerating... no 32 bit match, Count is %ld, max count is %ld\n", fccType, lpicinfo->fccHandler));

	if (fccHandler >= lpicinfo->fccHandler)
	    fccHandler -= lpicinfo->fccHandler;
	else
	    ; // This should be an assertion.  This leg is invalid.
    }

    //
    //  See if there is a 16-bit compressor we can use
    //  Because we always try 32 bit compressors first, if the user is
    //  enumerating the list of compressors we need to subtract the count
    //  of 32 bit compressors.
    //
    DPF(("ICInfo, fccType=%4.4hs, Handler=%4.4hs\n", (LPSTR)&fccType, (LPSTR)&fccHandler));
    return (ICInfoInternal(fccType, fccHandler, lpicinfo));
}
// Now map all ICInfo calls to ICInfoInternal

#define ICInfo ICInfoInternal
#endif //NOTHUNKS

BOOL VFWAPI ICInfo(DWORD fccType, DWORD fccHandler, ICINFO FAR * lpicinfo)
{
    char buf[128];
    static LPTSTR pszBuf = NULL;
    TCHAR achKey[20];
#ifdef WIN32
    TCHAR achTypeCopy[5];
    char newHandler[5];
    char newType[5];
#endif
    int  i;
    int  iComp;
    PIC  pic;

    DPF(("ICInfoInternal(16), fccType=%4.4hs, Handler=%4.4hs\n", (LPSTR)&fccType, (LPSTR)&fccHandler));
    if (lpicinfo == NULL)
	return FALSE;

    // THIS IS NOT REDUNDANT.  what if fccType == 0
    if (fccType > 0 && fccType < 256) {
        DPF(("fcctype invalid\r\n"));
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

	    ictokey(fccType, fccHandler, achKey);

	    if (!GetPrivateProfileString(szDrivers,achKey,szNull,buf,sizeof(buf)/sizeof(TCHAR),szSystemIni) &&
		!GetPrivateProfileString(szIniSect,achKey,szNull,buf,sizeof(buf)/sizeof(TCHAR),szSystemIni))
            {
                DPF(("NO information in DRIVERS section\n"));
		return FALSE;
            }

	    for (i=0; buf[i] && buf[i] != TEXT(' '); i++)
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
	for (i=0; i < MAX_CONVERTERS; i++)
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
	// read all the keys. from [Drivers] and [Installable Compressors]
	//

	if (pszBuf == NULL) {
	    UINT cbBuffer = 128 * sizeof(TCHAR);
	    UINT cchBuffer;
	
	    for (;;)
	    {
		pszBuf = GlobalAllocPtr(GMEM_SHARE | GHND, cbBuffer);

		if (!pszBuf) {
		    DPF(("Out of memory for SYSTEM.INI keys\r\n"));
		    return FALSE;
		}

		cchBuffer = (UINT)GetPrivateProfileString(szDrivers,
							  NULL,
							  szNull,
							  pszBuf,
							  cbBuffer / sizeof(TCHAR),
							  szSystemIni);

		if (cchBuffer < ((cbBuffer / sizeof(TCHAR)) - 5)) {
		    cchBuffer += (UINT)GetPrivateProfileString(szIniSect,
							  NULL,
							  szNull,
							  pszBuf + cchBuffer,
							  (cbBuffer / sizeof(TCHAR)) - cchBuffer,
							  szSystemIni);

		    if (cchBuffer < ((cbBuffer / sizeof(TCHAR)) - 5))
			break;
		}

		GlobalFreePtr(pszBuf);
		pszBuf = NULL;

		//
		//  if cannot fit drivers section in 32k, then something is horked
		//  with the section... so let's bail.
		//
		if (cbBuffer >= 0x8000) {
		    DPF(("SYSTEM.INI keys won't fit in 32K????\r\n"));
		    return FALSE;
		}
		
		cbBuffer *= 2;
		DPF(("Increasing size of SYSTEM.INI buffer to %d\r\n", cbBuffer));
	    }
	}


#ifdef WIN32
	/* make a widechar copy of the Ansi fccType so we can compare it with
	 * the wide copy returned from GetPrivateProfileString()
	 */
	MultiByteToWideChar(CP_ACP, 0, (LPSTR) &fccType, sizeof(fccType),
			    achTypeCopy, sizeof(achTypeCopy)/sizeof(TCHAR) );
#endif
	for (i=0; pszBuf[i] != 0; i += lstrlen(&pszBuf[i]) + 1)
	{
	    if (pszBuf[i+4]!=TEXT('.'))
		continue;
#ifdef WIN32
	    CharLower(&pszBuf[i]);

	    if ((fccType == 0 ||
		(wcsncmp(achTypeCopy, &pszBuf[i], sizeof(fccType)) == 0)) &&
		iComp-- == 0)
	    {
		WideCharToMultiByte(CP_ACP, 0, &pszBuf[i], sizeof(fccType),
			newType, sizeof(newType),NULL, NULL);

		WideCharToMultiByte(CP_ACP, 0, &pszBuf[i+5], sizeof(fccType),
			newHandler, sizeof(newHandler),NULL, NULL);

		return ICInfo(*(LPDWORD)&newType[0],*(LPDWORD)&newHandler[0], lpicinfo);
	    }
#else
	    AnsiLower(&pszBuf[i]);

	    if ((fccType == 0 || fccType == *(LPDWORD)&pszBuf[i]) && iComp-- == 0)
	    {
		return ICInfo(*(LPDWORD)&pszBuf[i],*(LPDWORD)&pszBuf[i+5], lpicinfo);
	    }
#endif
	}

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

	    if (!GetPrivateProfileString(szDrivers,achKey,szNull,buf,sizeof(buf)/sizeof(TCHAR),szSystemIni))
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
    DWORD dw;

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

#ifndef NOTHUNKS
    if (!Is32bitHandle(hic))
#endif //!NOTHUNKS
	if (pic->hDriver)
	{
	    GetModuleFileName(GetDriverModuleHandle(pic->hDriver),
		picinfo->szDriver, sizeof(picinfo->szDriver));
	}

    dw = ICSendMessage((HIC)pic, ICM_GETINFO, (DWORD)picinfo, cb);

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
LRESULT VFWAPI ICSendMessage(HIC hic, UINT msg, DWORD dw1, DWORD dw2)
{
    PIC pic = (PIC)hic;
    LRESULT l;

#ifndef NOTHUNKS

    //
    // If it's a 32-bit handle then send it to the 32-bit code
    // We need to take some extra care with ICM_DRAW_SUGGESTFORMAT
    // which can include a HIC in the ICDRAWSUGGEST structure.
    //

#define ICD(dw1)  ((ICDRAWSUGGEST FAR *)(dw1))

    if (pic->h32) {

        ICDebugMessage(hic, msg, dw1, dw2);

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
	l = ICSendMessage32(pic->h32, msg, dw1, dw2);
        return ICDebugReturn(l);
    }

#endif //!NOTHUNKS

    V_HIC(hic);

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
#ifndef WIN32
    return ICSendMessage(hic, msg, (DWORD)(LPVOID)(&cb+1), cb);
#else
    va_list va;

    va_start(va, cb);
    va_end(va);
    return ICSendMessage(hic, msg, (DWORD)(LPVOID)va, cb);
#endif
}

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


INLINE PIC NEAR PASCAL ICOpenInternal(PIC pic, DWORD fccType, DWORD fccHandler,
                ICINFO FAR * picinfo, ICOPEN FAR * picopen, UINT wMode)
{
    PIC picT;

    if (picinfo->szDriver[0])
    {
#ifdef DEBUG
	DWORD time = timeGetTime();
	char ach[80];
#endif
	pic->hDriver = LoadDriver(picinfo->szDriver, &pic->DriverProc);

#ifdef DEBUG
	time = timeGetTime() - time;
	wsprintfA(ach, "COMPMAN: LoadDriver(%ls) (%ldms)\r\n", (LPSTR)picinfo->szDriver, time);
	OutputDebugStringA(ach);
#endif

	if (pic->hDriver == NULL)
	{
	    pic->dwSmag = 0;
	    return NULL;
	}

	//
	// now try to open the driver as a codec.
	//
	pic->dwDriver = ICSendMessage((HIC)pic, DRV_OPEN, 0, (DWORD)(LPVOID)picopen);

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
	    picopen->dwError != 0 &&
	    fccType == streamtypeVIDEO)
	{
	    if (wMode == ICMODE_DRAW)
		picopen->dwFlags = ICMODE_DECOMPRESS;

	    picopen->fccType = ICTYPE_VIDEO;
	    pic->dwDriver = ICSendMessage((HIC)pic, DRV_OPEN, 0, (DWORD)(LPVOID)picopen);
	}

	if (pic->dwDriver == 0)
	{
	    ICClose((HIC)pic);
	    return NULL;
	}

	// open'ed ok mark these
	pic->fccType    = fccType;
	pic->fccHandler = fccHandler;
    }
    else if (picT = FindConverter(fccType, fccHandler))
    {
        DWORD dw;
	picT->dwSmag = SMAG;
	dw = ICSendMessage((HIC)picT, DRV_OPEN, 0, (DWORD)(LPVOID)picopen);

	if (dw == 0)
	{
	    pic->dwSmag = 0;
	    return NULL;
	}

	*pic = *picT;
	pic->dwDriver = dw;
    }

    return pic;
}

/* Helper functions for compression library */
HIC VFWAPI ICOpen(DWORD fccType, DWORD fccHandler, UINT wMode)
{
    ICOPEN      icopen;
    ICINFO      icinfo;
    PIC         pic;
    HIC         hic = NULL;   // Initialise

    ICEnterCrit(&ICOpenCritSec);

    AnsiLowerBuff((LPSTR) &fccType, sizeof(DWORD));
    AnsiLowerBuff((LPSTR) &fccHandler, sizeof(DWORD));
    icopen.dwSize  = sizeof(ICOPEN);
    icopen.fccType = fccType;
    icopen.fccHandler = fccHandler;
    icopen.dwFlags = wMode;
    icopen.dwError = 0;

    DPF(("\nICOpen('%4.4s','%4.4s)'\r\n", (LPSTR)&fccType, (LPSTR)&fccHandler));

    if (!ICInfo(fccType, fccHandler, &icinfo))
    {
	RPF(("Unable to locate Compression module '%4.4s' '%4.4s'\r\n", (LPSTR)&fccType, (LPSTR)&fccHandler));

	ICLeaveCrit(&ICOpenCritSec);
	return NULL;
    }

    pic = FindConverter(0L, 0L);

    if (pic == NULL)
    {
	ICLeaveCrit(&ICOpenCritSec);
	return NULL;
    }


#ifndef NOTHUNKS
    // Try and open on the 32 bit side first.
    // This block and the one below can be interchanged to alter the order
    // in which we try and open the compressor.
    if (hic == NULL)
    {
	pic->dwSmag     = SMAG;
	pic->hTask      = (HTASK)GetCurrentTask();
	pic->h32 = ICOpen32(fccType, fccHandler, wMode);

	if (pic->h32 != 0) {
	    pic->fccType    = fccType;
	    pic->fccHandler = fccHandler;
	    pic->dwDriver   = (DWORD) -1;
	    pic->DriverProc = NULL;
	    hic = (HIC)pic;
	}
    }
#endif //NOTHUNKS

    // Open on the 32 bit side first, then try and open 16 bit...
    if (hic == NULL) {
        pic->dwSmag     = SMAG;
        pic->hTask      = (HTASK)GetCurrentTask();

        hic = (HIC)ICOpenInternal(pic, fccType, fccHandler, &icinfo, &icopen, wMode);
    }
    ICLeaveCrit(&ICOpenCritSec);

    return(hic);
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
    DWORD       dw;

    if (IsBadCodePtr(lpfnHandler))
	return NULL;

#ifndef NOTHUNKS
    // lpfnHandler points to 16 bit code that will be used as a compressor.
    // We do not want this to go over to the 32 bit side.
#endif

    AnsiLowerBuff((LPSTR) &fccType, sizeof(DWORD));
    AnsiLowerBuff((LPSTR) &fccHandler, sizeof(DWORD));
    icopen.dwSize  = sizeof(ICOPEN);
    icopen.fccType = fccType;
    icopen.fccHandler = fccHandler;
    icopen.dwFlags = wMode;

    pic = FindConverter(0L, 0L);

    if (pic == NULL)
	return NULL;

    pic->dwSmag   = SMAG;
    pic->fccType  = fccType;
    pic->fccHandler  = fccHandler;
    pic->dwDriver = 0L;
    pic->hDriver  = NULL;
    pic->DriverProc  = (DRIVERPROC)lpfnHandler;

    dw = ICSendMessage((HIC)pic, DRV_OPEN, 0, (DWORD)(LPVOID)&icopen);

    if (dw == 0)
    {
	ICClose((HIC) pic);
	return NULL;
    }

    pic->dwDriver = dw;

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

#ifndef NOTHUNKS
    if (pic->h32 != 0) {
	LRESULT lres = ICClose32(pic->h32);
        pic->dwSmag   = GONE;
        pic->fccType  = 0L;
        pic->fccHandler  = 0L;
        pic->dwDriver = 0;
        pic->hDriver = NULL;
        pic->DriverProc = NULL;
	pic->h32 = 0;       // Next user of this slot does not want h32 set
	return(lres);
    }
#endif //!NOTHUNKS

#ifdef DEBUG
    {
    char ach[80];

    if (pic->hDriver)
	GetModuleFileName(GetDriverModuleHandle (pic->hDriver), ach, sizeof(ach));
    else
	ach[0] = 0;

    DPF(("ICClose(%04X) %4.4s.%4.4s %s\r\n", hic, (LPSTR)&pic->fccType, (LPSTR)&pic->fccHandler, (LPSTR)ach));
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
#ifdef WIN32
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
    return ICSendMessage(hic, ICM_COMPRESS, (DWORD)(LPVOID)&icc, sizeof(ICCOMPRESS));
    // NOTE: We do NOT copy any results from this temporary structure back
    // to the input variables.
#else
    return ICSendMessage(hic, ICM_COMPRESS, (DWORD)(LPVOID)&dwFlags, sizeof(ICCOMPRESS));
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
#if 1
    ICDECOMPRESS icd;
    // We cannot rely on the stack alignment giving us the right layout
    icd.dwFlags    = dwFlags;

    icd.lpbiInput  = lpbiFormat;

    icd.lpInput    = lpData;

    icd.lpbiOutput = lpbi;
    icd.lpOutput   = lpBits;
    icd.ckid       = 0;
    return ICSendMessage(hic, ICM_DECOMPRESS, (DWORD)(LPVOID)&icd, sizeof(ICDECOMPRESS));
#else
    return ICSendMessage(hic, ICM_DECOMPRESS, (DWORD)(LPVOID)&dwFlags, sizeof(ICDECOMPRESS));
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
#ifdef WIN32
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

    return ICSendMessage(hic, ICM_DRAW_BEGIN, (DWORD)(LPVOID)&icdraw, sizeof(ICDRAWBEGIN));
#else
    return ICSendMessage(hic, ICM_DRAW_BEGIN, (DWORD)(LPVOID)&dwFlags, sizeof(ICDRAWBEGIN));
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
#ifdef WIN32
    ICDRAW  icdraw;
    icdraw.dwFlags  =   dwFlags;
    icdraw.lpFormat =   lpFormat;
    icdraw.lpData   =   lpData;
    icdraw.cbData   =   cbData;
    icdraw.lTime    =   lTime;

    return ICSendMessage(hic, ICM_DRAW, (DWORD)(LPVOID)&icdraw, sizeof(ICDRAW));
#else
    return ICSendMessage(hic, ICM_DRAW, (DWORD)(LPVOID)&dwFlags, sizeof(ICDRAW));
#endif
}

/*****************************************************************************
 * @doc EXTERNAL IC ICAPPS
 *
 * @api HIC | ICGetDisplayFormat | This function returns the "best"
 *      format available for display a compressed image. The function
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
    DWORD dw;
    HDC hdc;
    BOOL fNukeHic = (hic == NULL);
    static int ScreenBitDepth = -1;

    if (hic == NULL)
	hic = ICDecompressOpen(ICTYPE_VIDEO, 0L, lpbiIn, NULL);

    if (hic == NULL)
	return NULL;

    //
    // dy = 0 and dy = 0 means don't stretch.
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
	    DWORD FAR PASCAL DrawDibProfileDisplay(LPBITMAPINFOHEADER lpbi);

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
	}
#ifdef DEBUG
	ScreenBitDepth = GetProfileInt(TEXT("DrawDib"),
				       TEXT("ScreenBitDepth"),
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
    lpbiOut->biSize = sizeof(BITMAPINFOHEADER);
    lpbiOut->biCompression = BI_RGB;
    lpbiOut->biBitCount = BitDepth;

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
//  16:  16,24,32,X
//  24:  24,32,16,X
//  32:  32,24,16,X

    //
    // try another bit depth in this order 8,16,24,32
    //
    if (BitDepth <= 8)
    {
	BitDepth = 16;
	goto try_bit_depth;
    }

    if (BitDepth == 16)
    {
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
	dx = 0;
	dy = 0;

	//
	// try to find a non stretched format.  but don't let the
	// device dither if we are going to stretch!
	//
	if (lpbiIn->biBitCount > 8)
	    BitDepth = 16;
	else
	    BitDepth = 0;

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

	if (hic && ICSendMessage(hic, msg, (DWORD)lpbiIn, (DWORD)lpbiOut) == ICERR_OK)
	    return hic;
	else if (hic)
	    ICClose(hic);
    }

    if (fccType == ICTYPE_VIDEO && lpbiIn)
    {
	if (lpbiIn->biCompression > 256)
	{
	    hic = ICOpen(fccType, lpbiIn->biCompression, wFlags);

	    if (hic && ICSendMessage(hic, msg, (DWORD)lpbiIn, (DWORD)lpbiOut) == ICERR_OK)
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
	hic = ICOpen(fccType, icinfo.fccHandler, wFlags);

	if (hic == NULL)
	    continue;

	if (ICSendMessage(hic, msg, (DWORD)lpbiIn, (DWORD)lpbiOut) != ICERR_OK)
	{
	    ICClose(hic);
	    continue;
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
 ****************************************************************************/

static HDRVR LoadDriver(LPSTR szDriver, DRIVERPROC FAR *lpDriverProc)
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
	hModule = LoadLibrary(szDriver);
	SetErrorMode(u);

	if (hModule <= HINSTANCE_ERROR)
	    return NULL;
	hDriver = (HMODULE) hModule;
    }
    else
    {
	hDriver = OpenDriver (szDriver, NULL, NULL);
	if (!hDriver)
	    return NULL;
	hModule = GetDriverModuleHandle (hDriver);
    }
    DPF(("LoadDriver: %s\r\n", szDriver));

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
	return NULL;
    }

    if (fWow && GetModuleUsage(hModule) == 1)   //!!!this is not exacly like USER
    {
	if (!DriverProc(0, (HDRVR)1, DRV_LOAD, 0L, 0L))
	{
	    FreeLibrary(hModule);
	    return NULL;
	}

	DriverProc(0, (HDRVR)1, DRV_ENABLE, 0L, 0L);
    }

    CacheModule (hModule);

    *lpDriverProc = DriverProc;
    return hDriver;
}

/*****************************************************************************
 * @doc INTERNAL IC
 *
 * @api void | FreeDriver | unload a driver
 *
 ****************************************************************************/

static void FreeDriver(HDRVR hDriver)
{
    if (!IsWow())
    {
	CloseDriver (hDriver, 0L, 0L);
    }
    else
    {
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
    }
}

#ifdef DEBUG_RETAIL

/************************************************************************

    messages.

************************************************************************/

struct {
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
ICM_DECOMPRESS_SET_PALETTE      , "ICM_DECOMPRESS_SET_PALETTE",
ICM_DECOMPRESS_GET_PALETTE      , "ICM_DECOMPRESS_GET_PALETTE",
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
ICM_COMPRESS_FRAMES_INFO        , "ICM_COMPRESS_FRAMES_INFO",
ICM_COMPRESS_FRAMES             , "ICM_COMPRESS_FRAMES",
ICM_SET_STATUS_PROC             , "ICM_SET_STATUS_PROC",
ICM_ENUM_FORMATS                , "ICM_ENUM_FORMATS",
ICM_GET_FORMAT_NAME             , "ICM_GET_FORMAT_NAME",
0                               , NULL
};

struct {
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

static BOOL  fDebug = -1;
static DWORD dwTime;

void ICDebugMessage(HIC hic, UINT msg, DWORD dw1, DWORD dw2)
{
    int i;

    if (!fDebug)
	return;

    for (i=0; aMsg[i].msg && aMsg[i].msg != msg; i++)
	;

    if (aMsg[i].msg == 0)
	RPF(("ICM(%04X,ICM_%04X,%08lX,%08lX)", hic, msg, dw1, dw2));
    else
	RPF(("ICM(%04X,%s,%08lX,%08lX)", hic, (LPSTR)aMsg[i].szMsg, dw1, dw2));

    dwTime = timeGetTime();
}

LRESULT ICDebugReturn(LRESULT err)
{
    int i;

    if (!fDebug)
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

static void ICDump()
{
    int i;
    PIC pic;
    char ach[80];

    DPF(("ICDump ---------------------------------------\r\n"));

    for (i=0; i<MAX_CONVERTERS; i++)
    {
	pic = &aicConverters[i];

	if (pic->fccType == 0)
	    continue;

	if (pic->dwSmag == 0)
	    continue;

	if (pic->hDriver)
	    GetModuleFileName(GetDriverModuleHandle (pic->hDriver), ach, sizeof(ach));
	else
	    ach[0] = 0;

	DPF(("  HIC: %04X %4.4s.%4.4s hTask=%04X Proc=%08lx %s\r\n", (HIC)pic, (LPSTR)&pic->fccType, (LPSTR)&pic->fccHandler, pic->hTask, pic->DriverProc, (LPSTR)ach));
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

#ifdef DEBUG_RETAIL

#define MODNAME "COMPMAN"

static void cdecl dprintfc(LPSTR szFormat, ...)
{
    char ach[128];

#ifdef WIN32
    va_list va;
    if (fDebug == -1)
	fDebug = GetProfileIntA("Debug",MODNAME, FALSE);

    if (!fDebug)
	return;

    va_start(va, szFormat);
    if (szFormat[0] == '!')
	ach[0]=0, szFormat++;
    else
	lstrcpyA(ach, MODNAME ": ");

    wvsprintfA(ach+lstrlenA(ach),szFormat,va);
    va_end(va);
//  lstrcatA(ach, "\r\r\n");
#else
    if (fDebug == -1) {
        fDebug = GetProfileIntA("Debug",MODNAME, FALSE);
        switch (fDebug) {
            case -1:
                OutputDebugStringA("fDebug still set at -1\n");
                break;
            case 0:
                OutputDebugStringA("fDebug set to 0\n");
                break;
            case 1:
                OutputDebugStringA("fDebug set to 1\n");
                break;
            case 2:
                OutputDebugStringA("fDebug set to 2\n");
                break;
            default:
                OutputDebugStringA("fDebug set to something else\n");
        }
    }

    if (!fDebug)
	return;

    if (szFormat[0] == '!')
	ach[0]=0, szFormat++;
    else
	lstrcpyA(ach, MODNAME ": ");

    wvsprintfA(ach+lstrlenA(ach),szFormat,(LPSTR)(&szFormat+1));
    lstrcatA(ach, "\r\r\n");
#endif

    OutputDebugStringA(ach);
}

#endif
