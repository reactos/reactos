/*
 *
 *
 * !!!!!!!!! THIS FILE IS NOT USED !!!!!!!!!!!!!!!
 *
 *
 */
#error THIS FILE IS NOT USED
#include <windows.h>

#undef WINAPI
#define WINAPI FAR PASCAL _loadds

#include "dciman.h"

static char szSystemIni[] = "system.ini";
static char szDrivers[]   = "Drivers";
static char szDCI[]       = "dci";
static char szDISPLAY[]   = "display";
static char szDCISVGA[]   = "dcisvga";

static BOOL fDVA;
extern LONG DVAEscape(HDC hdc, UINT function, UINT size, LPVOID lp_in_data, LPVOID lp_out_data);

/****************************************************************************
 ***************************************************************************/

HDC WINAPI DCIOpenProvider(void)
{
    char ach[128];
    HDC hdc;
    UINT u;

    //
    //  get the DCI provider, default to the display driver
    //
    GetPrivateProfileString(szDrivers, szDCI, szDISPLAY, ach, sizeof(ach), szSystemIni);

    //
    // if dci=dcisvga in system.ini, ignore it and try the display driver first.
    //
    if (lstrcmpi(ach, szDCISVGA) == 0) {
	lstrcpy(ach, szDISPLAY);
    }

again:
    u = SetErrorMode(SEM_NOOPENFILEERRORBOX);
    hdc = CreateDC(ach, NULL, NULL, NULL);
    SetErrorMode(u);

    if (hdc == NULL)
    {
fail:
        if (lstrcmpi(ach, szDCISVGA) == 0) {
            return NULL;
	}

        if (lstrcmpi(ach, szDISPLAY) == 0) {
	    lstrcpy(ach, szDISPLAY);
	    goto again;
	}

        lstrcpy(ach, szDISPLAY);
        goto again;
    }

    //
    //  now check for the Escape
    //
    u = DCICOMMAND;
    if (Escape(hdc, QUERYESCSUPPORT,sizeof(u),(LPCSTR)&u,NULL) == 0)
    {
        //
        // driver does not do escape, punt it, try old DVA first though
        //
        if (lstrcmpi(ach, szDISPLAY) == 0)
        {
            if (DVAEscape(hdc,QUERYESCSUPPORT,sizeof(u),(LPCSTR)&u,NULL) != 0)
            {
                fDVA = TRUE;
                return hdc;
            }
        }

        DeleteDC(hdc);
        goto fail;
    }

    return hdc;

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
        return DVAEscape(hdc, DCICOMMAND, sizeof(DCICMD),(LPCSTR)pcmd, lplpOut);
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
