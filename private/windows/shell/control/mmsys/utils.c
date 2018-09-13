#include "mmcpl.h"
#include "utils.h"

/*
 ***************************************************************
 *  Typedefs
 ***************************************************************
 */
typedef struct _DYNLOAD_INFO
{
    LPCTSTR  pszLib;
    HMODULE hLib;
    short   iRefCnt;
}
DYNLOAD_INFO, *PDYNLOAD_INFO;

/*
 ***************************************************************
 * File Globals
 ***************************************************************
 */
static SZCODE aszMSACM32[] = TEXT("MSACM32.DLL");
static SZCODE aszAVIFIL32[] = TEXT("AVIFIL32.DLL");
static SZCODE aszMSVFW32[] = TEXT("MSVFW32.DLL");
static SZCODE aszVERSION[] = TEXT("VERSION.DLL");


DYNLOAD_INFO DynLoadInfo[] =
{
    aszMSACM32,   0, 0,
    aszAVIFIL32,  0, 0,
    aszMSVFW32,   0, 0,
    aszVERSION,   0, 0,
    NULL,         0, 0
};

static const char cszTacmFormatDetailsW[] = "acmFormatDetailsW";
static const char cszTacmFormatTagDetailsW[] = "acmFormatTagDetailsW";
static const char cszTacmDriverDetailsW[] = "acmDriverDetailsW";
static const char cszTacmDriverMessage[]  = "acmDriverMessage";
static const char cszTacmDriverAddW[]     = "acmDriverAddW";
static const char cszTacmDriverEnum[]     = "acmDriverEnum";
static const char cszTacmDriverPriority[] = "acmDriverPriority";
static const char cszTacmDriverRemove[]   = "acmDriverRemove";
static const char cszTacmMetrics[]        = "acmMetrics";
static const char cszTacmFormatChooseW[]  = "acmFormatChooseW";

PROC_INFO ACMProcs[] =
{
    cszTacmFormatDetailsW,    0,
    cszTacmFormatTagDetailsW, 0,
    cszTacmDriverDetailsW,    0,
    cszTacmDriverMessage,     0,
    cszTacmDriverAddW,        0,
    cszTacmDriverEnum,        0,
    cszTacmDriverPriority,    0,
    cszTacmDriverRemove,      0,
    cszTacmMetrics,           0,
    cszTacmFormatChooseW,     0,

    NULL, 0
};

static const char cszICClose[]       = "ICClose";
static const char cszICGetInfo[]     = "ICGetInfo";
static const char cszICLocate[]      = "ICLocate";
static const char cszMCIWndCreateW[] = "MCIWndCreateW";

PROC_INFO VFWProcs[] =
{
    cszICClose,             0,
    cszICGetInfo,           0,
    cszICLocate,            0,
    cszMCIWndCreateW,       0,

    NULL, 0
};

static const char cszAVIFileRelease[]         = "AVIFileRelease";
static const char cszAVIStreamRelease[]       = "AVIStreamRelease";
static const char cszAVIStreamSampleToTime[]  = "AVIStreamSampleToTime";
static const char cszAVIStreamStart[]         = "AVIStreamStart";
static const char cszAVIStreamLength[]        = "AVIStreamLength";
static const char cszAVIStreamReadFormat[]    = "AVIStreamReadFormat";
static const char cszAVIStreamInfoW[]         = "AVIStreamInfoW";
static const char cszAVIFileGetStream[]       = "AVIFileGetStream";
static const char cszAVIFileOpenW[]           = "AVIFileOpenW";
static const char cszAVIFileInit[]            = "AVIFileInit";
static const char cszAVIFileExit[]            = "AVIFileExit";


PROC_INFO AVIProcs[] =
{
    cszAVIFileRelease,          0,
    cszAVIStreamRelease,        0,
    cszAVIStreamSampleToTime,   0,
    cszAVIStreamStart,          0,
    cszAVIStreamLength,         0,
    cszAVIStreamReadFormat,     0,
    cszAVIStreamInfoW,          0,
    cszAVIFileGetStream,        0,
    cszAVIFileOpenW,            0,
    cszAVIFileInit,             0,
    cszAVIFileExit,             0,

    NULL, 0
};

static const char cszVerQueryValueW[]          = "VerQueryValueW";
static const char cszGetFileVersionInfoW[]     = "GetFileVersionInfoW";
static const char cszGetFileVersionInfoSizeW[] = "GetFileVersionInfoSizeW";

PROC_INFO VERSIONProcs[] =
{
    cszVerQueryValueW,          0,
    cszGetFileVersionInfoW,     0,
    cszGetFileVersionInfoSizeW, 0,

    NULL, 0
};

/*
 ***************************************************************
 ***************************************************************
 */
STATIC BOOL LoadLibraryAndProcs(LPCTSTR pLibrary, PPROC_INFO pProcInfo)
{
    HMODULE    hLibrary;
    PPROC_INFO p;
	PDYNLOAD_INFO pLib;
	BOOL	fPrevLoaded = FALSE;

#ifdef DEBUG_BUILT_LINKED
	return TRUE;
#endif

	if (pProcInfo->Address)	//Already loaded
	{
		fPrevLoaded = TRUE;
		goto UpdateDynLoadInfo;
	}	
    hLibrary = LoadLibrary(pLibrary);

    if (hLibrary == NULL)
    {
		DPF("LoadLibrary failed for %s \r\n", pLibrary);
		return FALSE;
    }

    p = pProcInfo;

    while (p->Name)
    {
        p->Address = GetProcAddress(hLibrary, p->Name);

        if (p->Address == NULL)
        {
			DPF("GetProcAddress failed for %s \r\n", p->Name);
			FreeLibrary(hLibrary);
			return FALSE;
        }

        p++;
    }

UpdateDynLoadInfo:
	pLib = DynLoadInfo;

	while (pLib->pszLib)
	{
		if (!lstrcmpi(pLib->pszLib, pLibrary))
		{
			pLib->iRefCnt++;
			if (!fPrevLoaded)
			{
				pLib->hLib = hLibrary;
			}
			break;
		}
		pLib++;
	}


    return TRUE;
}

STATIC BOOL FreeLibraryAndProcs(LPCTSTR pLibrary, PPROC_INFO pProcInfo)
{
	PDYNLOAD_INFO p;

#ifdef DEBUG_BUILT_LINKED    
	return TRUE;
#endif

	p = DynLoadInfo;

	while (p->pszLib)
	{
		if (!lstrcmpi(p->pszLib, pLibrary))
		{
		    PPROC_INFO ppi;

			p->iRefCnt--;
			if (p->iRefCnt > 0)
				return TRUE;
			if (!p->hLib)
				return FALSE;
			DPF("Freeing Library %s \r\n",p->pszLib);
			FreeLibrary(p->hLib);
			p->hLib = 0;
			
			ppi = pProcInfo;
			while (ppi->Name)
			{
				ppi->Address = 0;
				ppi++;
			}
			return TRUE;
		}
		p++;
	}
	return FALSE;
}

BOOL LoadACM()
{
	DPF("***LOADING ACM***\r\n");
	return LoadLibraryAndProcs(aszMSACM32, ACMProcs);	
}

BOOL FreeACM()
{
	DPF("***FREEING ACM***\r\n");
	return FreeLibraryAndProcs(aszMSACM32, ACMProcs);	
}


BOOL LoadAVI()
{
	DPF("***LOADING AVI***\r\n");
	return LoadLibraryAndProcs(aszAVIFIL32, AVIProcs);	
}

BOOL FreeAVI()
{
	DPF("***FREEING AVI***\r\n");
	return FreeLibraryAndProcs(aszAVIFIL32, AVIProcs);	
}

BOOL LoadVFW()
{
	DPF("***LOADING VFW***\r\n");
	return LoadLibraryAndProcs(aszMSVFW32, VFWProcs);	
}

BOOL FreeVFW()						 
{
	DPF("***FREEING VFW***\r\n");
	return FreeLibraryAndProcs(aszMSVFW32, VFWProcs);	
}

BOOL LoadVERSION()
{
	DPF("***LOADING VERSION***\r\n");
	return LoadLibraryAndProcs(aszVERSION, VERSIONProcs);	
}

BOOL FreeVERSION()
{
	DPF("***FREEING VERSION***\r\n");
	return FreeLibraryAndProcs(aszVERSION, VERSIONProcs);	
}
