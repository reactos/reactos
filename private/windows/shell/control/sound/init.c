#include <windows.h>
#include <port1632.h>
#include <cpl.h>
#include <cphelp.h>
#include "snd.h"

/*---------------------------------------------------------------------------*/

#define NUM_APPLETS 1

/*---------------------------------------------------------------------------*/

typedef struct tagAppletInfo {
   int   idIcon;
    int idName;
    int idInfo;
    BOOL    bEnabled;
    DWORD   dwContext;
    PSTR    szHelp;
}   APPLET_INFO;

/*---------------------------------------------------------------------------*/

char    aszSoundHlp[24];
char    aszErrorPlayTitle[32];
char    aszErrorPlayMessage[64];
char    aszWarningTitle[64];
char    aszWarningMessage[128];
char    aszNoSound[16];
char    aszNoDevice[128];
char    aszAppName[30];
char    aszWriteErr[100];
DWORD   dwContext;
HINSTANCE   hInstance;
UINT    uHelpMessage;

/*---------------------------------------------------------------------------*/

static  SZCODE aszHelpMessage[] = "ShellHelp";
static  APPLET_INFO applets[NUM_APPLETS];

/*---------------------------------------------------------------------------*/

BOOL DllInitialize( IN PVOID hmod
                  , IN DWORD ulReason
                  , IN PCONTEXT pctx OPTIONAL
                  )
{
    if (ulReason != DLL_PROCESS_ATTACH)
        return TRUE;

    hInstance = hmod;
    applets[0].idIcon = ID_ICON;
    applets[0].idName = IDS_NAME;
    applets[0].idInfo = IDS_INFO;
    applets[0].bEnabled = TRUE;
    applets[0].dwContext = IDH_CHILD_SND;
    applets[0].szHelp = aszSoundHlp;

    LoadString(hInstance, IDS_UNABLETITLE, aszErrorPlayTitle, sizeof(aszErrorPlayTitle));
    LoadString(hInstance, IDS_UNABLEMESSAGE, aszErrorPlayMessage, sizeof(aszErrorPlayMessage));
    LoadString(hInstance, IDS_WARNINGTITLE, aszWarningTitle, sizeof(aszWarningTitle));
    LoadString(hInstance, IDS_WARNINGMESSAGE, aszWarningMessage, sizeof(aszWarningMessage));
    LoadString(hInstance, IDS_NONE, aszNoSound, sizeof(aszNoSound));
    LoadString(hInstance, IDS_APPNAME, aszAppName, sizeof(aszAppName));
    LoadString(hInstance, IDS_HELPFILE, aszSoundHlp, sizeof(aszSoundHlp));
    LoadString(hInstance, IDS_NODEVICE, aszNoDevice, sizeof(aszNoDevice));
    LoadString(hInstance, IDS_WRITEERR, aszWriteErr, sizeof(aszWriteErr));
    return TRUE;
}/* DllInitialize */

/*---------------------------------------------------------------------------*/

static void RunApplet( HWND hwnd, int cmd)
{
    dwContext = applets[cmd].dwContext;
    DialogBox(hInstance, MAKEINTRESOURCE(DLG_SOUND), hwnd, (DLGPROC)SoundDlg);
}/* RunApplet */

/*---------------------------------------------------------------------------*/

LONG CPlApplet( HWND hwnd
              , WORD wMsg
              , LPARAM lParam1
              , LPARAM lParam2
              )
{
    LPCPLINFO lpCPlInfo;         // was LPNEWCLPINFO in 3.1  -- LKG ???
    int iApplet;

    switch (wMsg) {
    case CPL_INIT:
        uHelpMessage = RegisterWindowMessage(aszHelpMessage);
        return (LONG)TRUE;

    case CPL_GETCOUNT:
        // second message to CPlApplet(), sent once only
        return (LONG)NUM_APPLETS;

    case CPL_INQUIRE:            // was NEWINQUIRE in 3.1 -- LKG ???
        /* third message to CPlApplet().  It is sent as many times
           as the number of applets returned by CPL_GETCOUNT message
        */
        /* Your DLL must contain an icon and two string resources.
           idIcon is the icon resource ID, idName and idInfo are
           string resource ID's for a short name, and description.
        */
        lpCPlInfo = (LPCPLINFO)lParam2;       // was LPNEWCPLINFO in 3.1 ???
        iApplet = (int)(LONG)lParam1;
      /***************3.1*******************
         lpCPlInfo->hIcon = LoadIcon( hInstance
                                    , MAKEINTRESOURCE(applets[iApplet].idIcon)
                                    );
         if ( !LoadString( hInstance
                         , applets[iApplet].idName
                         , lpCPlInfo->szName
                         , sizeof(lpCPlInfo->szName)
                         )
            )
            lpCPlInfo->szName[0] = 0;
         if( !LoadString( hInstance
                        , applets[iApplet].idInfo
                        , lpCPlInfo->szInfo
                        , sizeof(lpCPlInfo->szInfo)
                        )
           )
            lpCPlInfo->szInfo[0] = 0;
         lpCPlInfo->dwSize = sizeof(NEWCPLINFO);
         lpCPlInfo->dwHelpContext = applets[iApplet].dwContext;
         lstrcpy(lpCPlInfo->szHelpFile, applets[iApplet].szHelp);
      ************************************/
      lpCPlInfo->idIcon = ID_ICON;
      lpCPlInfo->idName = IDS_NAME;
        lpCPlInfo->lData = (LONG)iApplet;
        lpCPlInfo->idInfo = IDS_INFO;
        return (LONG)TRUE;
    case CPL_DBLCLK:
        RunApplet(hwnd, (int)(LONG)lParam2);
        break;
    case CPL_SELECT:
        /* One of your applets has been selected.
           lParam1 is an index from 0 to (NUM_APPLETS-1)
           lParam2 is the lData value associated with the applet
        */
        break;
    case CPL_STOP:
        /* Sent once for each applet prior to the CPL_EXIT msg.
           lParam1 is an index from 0 to (NUM_APPLETS-1)
           lParam2 is the lData value associated with the applet
        */
        break;
    case CPL_EXIT:
        /* Last message, sent once only, before CONTROL.EXE calls
           FreeLibrary() on your DLL.
        */
        break;
    }
    return (LONG)0;

}/* CPlApplet */
