//
//  Thunks for calling PIFMGR.DLL from Shell32.dll
//
//	PifMgr_OpenProperties
//	PifMgr_CloseProperties
//	PifMgr_GetProperties
//	PifMgr_SetProperties
//
//  PIFMGR.DLL will be kept loaded as long as a PIF PROPERTIES handle is open
//  and will be free'ed (via FreeLibrary) when the last handle is closed.
//
#include "shprv.h"
#include <pif.h>

static HMODULE hPifMgrDll;
static UINT    cRef;

static int  (WINAPI *XOpenProperties)(LPCSTR lpszApp, LPCSTR lpszPIF, int hInf, int flOpt);
static int  (WINAPI *XGetProperties)(int hProps, LPCSTR lpszGroup, LPVOID lpProps, int cbProps, int flOpt);
static int  (WINAPI *XSetProperties)(int hProps, LPCSTR lpszGroup, const VOID FAR *lpProps, int cbProps, int flOpt);
static int  (WINAPI *XCloseProperties)(int hProps, int flOpt);

static char szPifMgrDll[] = "PIFMGR.DLL";

/*****************************************************************************
 ****************************************************************************/
BOOL PifMgr_Load()
{
    if (hPifMgrDll == NULL)
    {
        DebugMsg(DM_TRACE, "pif: Loading PIFMGR.DLL");
        hPifMgrDll = LoadLibrary(szPifMgrDll);

        if (hPifMgrDll <= HINSTANCE_ERROR)
        {
            hPifMgrDll = NULL;
            return FALSE;
        }

        (FARPROC)XOpenProperties  = GetProcAddress(hPifMgrDll, MAKEINTATOM(ORD_OPENPROPERTIES));
        (FARPROC)XCloseProperties = GetProcAddress(hPifMgrDll, MAKEINTATOM(ORD_CLOSEPROPERTIES));
        (FARPROC)XGetProperties   = GetProcAddress(hPifMgrDll, MAKEINTATOM(ORD_GETPROPERTIES));
        (FARPROC)XSetProperties   = GetProcAddress(hPifMgrDll, MAKEINTATOM(ORD_SETPROPERTIES));
    }
    return TRUE;
}

/*****************************************************************************
 ****************************************************************************/
void PifMgr_Free()
{
    HMODULE h;

    if (cRef == 0 && (h=hPifMgrDll))
    {
        DebugMsg(DM_TRACE, "pif: Unloading PIFMGR.DLL");
        hPifMgrDll = NULL;
        FreeLibrary(h);
        XOpenProperties  = NULL;
        XCloseProperties = NULL;
        XGetProperties   = NULL;
        XSetProperties   = NULL;
    }
}

/*****************************************************************************
 ****************************************************************************/
int WINAPI PifMgr_OpenProperties(LPCSTR lpszApp, LPCSTR lpszPIF, int hInf, int flOpt)
{
    int hpif=NULL;

    if (PifMgr_Load())
    {
        cRef++;
        hpif = XOpenProperties(lpszApp, lpszPIF, hInf, flOpt);

        if (hpif == NULL)
            cRef--;
    }
    return hpif;
}

/*****************************************************************************
 ****************************************************************************/
int WINAPI PifMgr_GetProperties(int hProps, LPCSTR lpszGroup, LPVOID lpProps, int cbProps, int flOpt)
{
    if (PifMgr_Load())
        return XGetProperties(hProps, lpszGroup, lpProps, cbProps,  flOpt);
    else
        return 0;
}

/*****************************************************************************
 ****************************************************************************/
int WINAPI PifMgr_SetProperties(int hProps, LPCSTR lpszGroup, const VOID FAR *lpProps, int cbProps, int flOpt)
{
    if (PifMgr_Load())
        return XSetProperties(hProps, lpszGroup, lpProps, cbProps,  flOpt);
    else
        return 0;
}

/*****************************************************************************
 ****************************************************************************/
int WINAPI PifMgr_CloseProperties(int hProps, int flOpt)
{
    int i;

    //
    // SHELL32.DLL calls this (with NULL) on thread detach to
    // give us a chance to free PIFMGR if we dont need it.
    //
    if (hProps == NULL)
    {
        PifMgr_Free();
    }
    else if (cRef > 0)
    {
        i = XCloseProperties(hProps, flOpt);
        cRef--;
    }

    return i;
}
