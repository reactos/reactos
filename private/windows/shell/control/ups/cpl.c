/** FILE: cpl.c ************ Module Header ********************************
 *
 *  Initialization module and UPS Control Panel applet procedure.
 *  Additionally, this file also contains the "CPlApplet" procedure which
 *  complies with and services the ".CPL" protocol between the Control
 *  Panel main window procedure and its' applets.
 *
 * History:
 *  12:30 on Tues  23 Apr 1991  -by-  Steve Cathcart   [stevecat]
 *        Took base code from Win 3.1 source
 *  10:30 on Tues  04 Feb 1992  -by-  Steve Cathcart   [stevecat]
 *        Updated code to latest Win 3.1 sources
 *  10:00 on Thur Aug 27 1992   -by-  Congpa You       [congpay]
 *        Specialize the code for UPS.
 ***************************************************************************/
//==========================================================================
//                                Include files
//==========================================================================
#include "ups.h"
#include "cpl.h"

//==========================================================================
//                            Local Definitions
//==========================================================================

typedef struct {
    int idIcon;
    int idName;
    int idInfo;
    int idChild;
    BOOL bEnabled;
    DWORD dwContext;
    PSTR pszHelp;
} APPLET_INFO;

//==========================================================================
//                            External Declarations
//==========================================================================
/*  functions  */
extern BOOL APIENTRY UPSDlg     (HWND, UINT, DWORD, LONG);
extern BOOL RegisterArrowClass(HANDLE);


//==========================================================================
//                            Data Declarations
//==========================================================================
HANDLE  hModule = NULL;

char szSetupInfPath[PATHMAX];

char szErrLS[133];
char szErrMem[133];
char szCtlPanel[30];

char szWinCom[PATHMAX];             /* Path to WIN.COM directory */
char szSystemIniPath[PATHMAX];      /* Path to SYSTEM.INI */
char szCtlIni[PATHMAX];             /* Path to CONTROL.INI */

char szCONTROLINI[]  = "control.ini";

// REVIEW change to lower case.
char szSYSTEMINI[]    = "SYSTEM.INI";
char szSETUPINF[]     = "SETUP.INF";

DWORD   dwContext = 0L;

char szControlHlp[] = "control.hlp";

UINT wHelpMessage;

char szCplApplet[] = "CPlApplet";

APPLET_INFO applets[] = {
    { UPSICON   , CHILDREN     , INFO     , CHILD_UPS   , TRUE,
      IDH_CHILD_UPS   , szControlHlp },
};

#define NUM_APPLETS (sizeof(applets)/sizeof(applets[0]))


//==========================================================================
//                            Local Function Prototypes
//==========================================================================

LONG APIENTRY CPlApplet (HWND, unsigned int, LONG, LONG);

void RunApplet(HWND hwnd, int cmd);


//==========================================================================
//                                Functions
//==========================================================================

//  Win32 NT Dll Initialization procedure

BOOL DllInitialize(
IN PVOID hmod,
IN ULONG ulReason,
IN PCONTEXT pctx OPTIONAL)
{
    if (ulReason != DLL_PROCESS_ATTACH)
    {
        return TRUE;
    }
    else
    {
        hModule = hmod;
    }

    return TRUE;

    UNREFERENCED_PARAMETER(pctx);
}


//---------------------------------------------------------------------------
BOOL InitControlPanel(HWND hwndParent)
{
    OFSTRUCT os;

    if (!RegisterArrowClass(hModule))
    {
        return FALSE;
    }

    LoadString (hModule,   ERRMEM,     szErrMem,       sizeof (szErrMem));
    LoadString (hModule,   LSFAIL,     szErrLS,        sizeof (szErrLS));
    LoadString (hModule,   CPCAPTION,  szCtlPanel,     sizeof (szCtlPanel));

    wsprintf (szSystemIniPath, "%s%s", (LPSTR)szWinCom, (LPSTR)szSYSTEMINI);
    wsprintf (szCtlIni,        "%s%s", (LPSTR)szWinCom, (LPSTR)szCONTROLINI);

    if (OpenFile (szSETUPINF, &os, OF_EXIST) >= 0)
        strcpy (szSetupInfPath, os.szPathName);
    else
        strcpy (szSetupInfPath, szSETUPINF);    // not found, use this anyway?

    wHelpMessage = RegisterWindowMessage("ShellHelp");

    return TRUE;

    UNREFERENCED_PARAMETER(hwndParent);
}


void TermControlPanel()
{
    UnRegisterArrowClass (hModule);
    return;
}


LONG APIENTRY CPlApplet(HWND hWnd, UINT Msg, LONG lParam1, LONG lParam2)
{
    int i, count;
    LPNEWCPLINFO lpCPlInfo;
    LPCPLINFO lpOldCPlInfo;
    static iInitCount = 0;

    switch (Msg)
    {
    case CPL_INIT:
        if (!iInitCount)
        {
            if (!InitControlPanel(hWnd))
                return FALSE;
        }

        iInitCount++;
        return TRUE;

    case CPL_GETCOUNT:
        for (i = count = 0; i < NUM_APPLETS; i++)
            if (applets[i].bEnabled)
                count++;

        return (LONG)count;

    case CPL_INQUIRE:

        lpOldCPlInfo = (LPCPLINFO)lParam2;

        // find the proper applet not counting those that are disabled
        for (i = count = 0; i < NUM_APPLETS; i++)
        {
            if (applets[i].bEnabled)
            {
                if (count == (int)lParam1)
                    break;
                count++;
            }
        }

        lpOldCPlInfo->idIcon = applets[i].idIcon;
        lpOldCPlInfo->idName = applets[i].idName;
        lpOldCPlInfo->idInfo = applets[i].idInfo;
        lpOldCPlInfo->lData = (LONG)applets[i].idChild;
        return TRUE;

    case CPL_NEWINQUIRE:

        lpCPlInfo = (LPNEWCPLINFO)lParam2;

        // find the proper applet not counting those that are disabled
        for (i = count = 0; i < NUM_APPLETS; i++)
        {
            if (applets[i].bEnabled)
            {
                if (count == (int)lParam1)
                    break;
                count++;
            }
        }

        lpCPlInfo->hIcon = LoadIcon(hModule, MAKEINTRESOURCE(applets[i].idIcon));
        LoadString(hModule, applets[i].idName, lpCPlInfo->szName, sizeof(lpCPlInfo->szName));

        if (!LoadString(hModule, applets[i].idInfo, lpCPlInfo->szInfo,
                                sizeof(lpCPlInfo->szInfo)))
            lpCPlInfo->szInfo[0] = 0;

        lpCPlInfo->dwSize = sizeof(NEWCPLINFO);
        lpCPlInfo->lData  = (LONG)applets[i].idChild;
        lpCPlInfo->dwHelpContext = applets[i].dwContext;
        strcpy(lpCPlInfo->szHelpFile, applets[i].pszHelp);

        return TRUE;

    case CPL_DBLCLK:
        dwContext = applets[(int)lParam1].dwContext;
        RunApplet(hWnd, (int)lParam2);
        break;

    case CPL_EXIT:
        iInitCount--;
        if (!iInitCount)
            TermControlPanel();
        break;

    default:
        break;
    }
    return 0L;
}


void RunApplet(HWND hwnd, int cmd)
{
    switch (cmd)
    {
    case CHILD_UPS:
        DialogBox(hModule, MAKEINTRESOURCE(DLG_UPS), hwnd, (DLGPROC)UPSDlg);
        break;
    }

}


void CPHelp (HWND hWnd)
{
    WinHelp (hWnd, szControlHlp, HELP_CONTEXT, dwContext);
}


