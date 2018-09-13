#include <windows.h>

#undef WINAPI
#define WINAPI FAR PASCAL _loadds

#include "dciman.h"

static char szSystemIni[] = "system.ini";
static char szWinIni[] = "win.ini";
static char szDrivers[]   = "Drivers";
static char szDrawDib[]	  = "DrawDib";
static char szDCI[]       = "dci";
static char szDVA[]       = "dva";
static char szDISPLAY[]   = "display";
static char szDCISVGA[]   = "dcisvga";
static char szDCIDVA[]   = "dcidva";

static BOOL fDVA;
extern LONG DVAEscape(HDC hdc, UINT function, UINT size, LPVOID lp_in_data, LPVOID lp_out_data);

/****************************************************************************
 ***************************************************************************/

#define USE_DCI_EQUALS	0x01
#define USE_DRIVER	0x02
#define USE_DCISVGA	0x04

HDC WINAPI DCIOpenProvider(void)
{
    char 	ach[128];
    HDC 	hdc;
    UINT	u;
    int		dva;
    int		dcidva;
    BOOL	try_dcisvga = 0;
    BOOL	try_display = 0;
    BOOL	try_driver = 0;

    //
    // get dcidva variable.
    // 1 = use DCI= line
    // 2 = use display drivetr
    // 4 = use DCISVGA
    //
    dcidva = USE_DCI_EQUALS | USE_DRIVER | USE_DCISVGA;
    dcidva = GetPrivateProfileInt( szDrawDib, szDCIDVA, dcidva, szWinIni );
    if( dcidva == 0 ) {
	return NULL;
    }

    /*
     *	get the DCI provider, order of attempts
     *	1) try DCI=
     *	2) try display driver
     *	3) try DCISVGA
     */
    if( dcidva & USE_DCI_EQUALS ) {
	GetPrivateProfileString(szDrivers, szDCI, "", ach, sizeof(ach), szSystemIni);
    } else if( dcidva & USE_DRIVER ) {
	try_driver = 1;
	lstrcpy( ach,szDISPLAY );
    } else {
	try_dcisvga = 1;
	lstrcpy( ach,szDCISVGA );
    }

again:
    u = SetErrorMode(SEM_NOOPENFILEERRORBOX);
    hdc = CreateDC(ach, NULL, NULL, NULL);
    SetErrorMode(u);

    if (hdc == NULL)
    {
fail:
	if( try_dcisvga ) {
	    goto LastResort;
	}
	if( !try_driver ) {
	    if( dcidva & USE_DRIVER ) {
		try_driver = 1;
		lstrcpy(ach, szDISPLAY);
	    } else if( dcidva & USE_DCISVGA ) {
		try_dcisvga = 1;
		lstrcpy(ach, szDCISVGA);
	    } else {
		goto LastResort;
	    }
	} else { 
	    if( dcidva & USE_DCISVGA ) {
		try_dcisvga = 1;
		lstrcpy(ach, szDCISVGA);
	    } else {
		goto LastResort;
	    }
	}
	goto again;
    }

    //
    //  now check for the Escape.  If not, continue looping
    //
    u = DCICOMMAND;
    if (Escape(hdc, QUERYESCSUPPORT,sizeof(u),(LPCSTR)&u,NULL) == 0)
    {
        DeleteDC(hdc);
        goto fail;
    }

    return hdc;

LastResort:
    //
    // driver does not do escape, punt it, try old DVA first though
    //
    if (DVAEscape(hdc,QUERYESCSUPPORT,sizeof(u),(LPVOID)&u,NULL) != 0)
    {
	fDVA = TRUE;
	return hdc;
    }

    return NULL;
}

/****************************************************************************
 ***************************************************************************/

void WINAPI DCICloseProvider(HDC hdc)
{
    if (hdc)
        DeleteDC(hdc);
}

/****************************************************************************
 ***************************************************************************/

int WINAPI DCISendCommand(HDC hdc, DCICMD FAR *pcmd, VOID FAR * FAR * lplpOut)
{
    if (lplpOut)            // in case it fails, make sure this is NULL
        *lplpOut = NULL;

    if (fDVA)
        return DVAEscape(hdc, DCICOMMAND, sizeof(DCICMD),(LPVOID)pcmd, lplpOut);
    else
        return Escape(hdc, DCICOMMAND, sizeof(DCICMD),(LPCSTR)pcmd, lplpOut);
}

/****************************************************************************
 ***************************************************************************/

extern int WINAPI DCICreatePrimary(HDC hdc, DCISURFACEINFO FAR * FAR *lplpSurface)
{
    DCICMD  cmd;

    cmd.dwCommand   = DCICREATEPRIMARYSURFACE;
    cmd.dwParam1    = 0;
    cmd.dwParam2    = 0;
    cmd.dwVersion   = DCI_VERSION;
    cmd.dwReserved  = 0;

    return DCISendCommand(hdc, &cmd, lplpSurface);
}
