/****************************Module*Header******************************\
* Module Name: winmain.c                                                *
*                                                                       *
*                                                                       *
*                                                                       *
* Created: 1989                                                         *
*                                                                       *
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
*                                                                       *
* A general description of how the module is used goes here.            *
*                                                                       *
* Additional information such as restrictions, limitations, or special  *
* algorithms used if they are externally visible or effect proper use   *
* of the module.                                                        *
\***********************************************************************/

#include <windows.h>

#include "port1632.h"

//#define NOEXTERN

#include "pbrush.h"
#ifdef BLOCKONMSG
#include "pbserver.h"
#endif

extern HWND   hDlgModeless;
extern TCHAR pgmName[], pgmTitle[];
extern HWND pbrushWnd[];

extern PRINTDLG PD;

static HANDLE haccelTbl;

static TCHAR TableHolder[100];

void FAR PASCAL FreePrintHandles(void)
{
  if(PD.hDevMode)
    GlobalFree(PD.hDevMode);

  if(PD.hDevNames)
    GlobalFree(PD.hDevNames);

  PD.hDevMode = PD.hDevNames = NULL;
}

LPTSTR GetTableString(WORD stringid)
{
   LoadString(hInst, stringid, TableHolder, CharSizeOf(TableHolder));
   return TableHolder;
}

LPTSTR GetFarString(WORD stringid)
{
   GetTableString(stringid);
   return (LPTSTR) TableHolder;
}

#ifdef BETA_VER
BOOL BetaChecks(int hPrevInstance)
{
   DWORD   dwWinFlags;
   int     fBetaMsgs;
   TCHAR   buf[10];

   if (hPrevInstance) {
       MessageBox(NULL,
           TEXT("Multiple instances of Paintbrush are disabled (beta")
           TEXT(" release only) for your protection.  This instance")
           TEXT(" will be terminated."),
           TEXT("Beta Warning"), MB_OK);
       return FALSE;
   }

   dwWinFlags = MGetWinFlags();

   if (dwWinFlags & (WF_LARGEFRAME | WF_SMALLFRAME)) {
       MessageBox(NULL,
           TEXT("Paintbrush is disabled under real mode EMS systems")
           TEXT(" (beta release only) for your protection.  This")
           TEXT(" instance will be terminated."),
           TEXT("Beta Warning"), MB_OK);
       return FALSE;
   }

   /* get current message flags from win.ini */
   fBetaMsgs = GetProfileInt(TEXT("Paintbrush"), TEXT("flags"), 3);

   if ((fBetaMsgs & 1) && !(dwWinFlags & WF_PMODE)) {
       MessageBox(NULL,
           TEXT("Paintbrush does not deal with low memory situations")
           TEXT(" well in this beta release.  To avoid losing work,")
           TEXT(" run Paintbrush under 286 or 386 protect mode.  This")
           TEXT(" is the only time that you will receive this warning."),
           TEXT("Beta Warning"), MB_OK);
       fBetaMsgs &= ~1;
   }

   if (fBetaMsgs & 2) {
       MessageBox(NULL,
           TEXT("There is a known bug in Paintbrush that can cause the")
           TEXT(" contents of overlapping windows to be placed in your")
           TEXT(" drawing.  For this reason it is not recommended to")
           TEXT(" bring up other applications on top of the Paintbrush")
           TEXT(" window.  This is only a problem for the beta release.")
           TEXT(" This is the only time that you will receive this")
           TEXT(" warning."),
           TEXT("Beta Warning"), MB_OK);
       fBetaMsgs &= ~2;
   }

   wsprintf(buf, TEXT("%d"), fBetaMsgs);
   WriteProfileString(TEXT("Paintbrush"), TEXT("flags"), buf);

   return TRUE;
}
#endif

MMain(hInstance,hPrevInstance,
                   lpAnsiCmdLine, cmdShow)

   MSG msg;
   LPTSTR lpCmdLine = GetCommandLine ();

#ifdef BLOCKONMSG
   PBSRVR FAR * lpOleServer;
   extern int fBlocked;
   extern HANDLE hServer;
#endif

#ifdef BETA_VER
   if (!BetaChecks(hPrevInstance))
       return FALSE;
#endif

   hInst = hInstance;

   /* if this is the first instance then call initialization procedure */
   if (!hPrevInstance)
   {
       if (!WndInit(hInstance))
           return FALSE;
       haccelTbl = LoadAccelerators(hInstance, pgmName);
   }
   else
   {
       /* else copy data from previous instance */
       GetInstanceData(hPrevInstance, pgmName,    APPNAMElen);
       GetInstanceData(hPrevInstance, pgmTitle,   TITLElen);
       GetInstanceData(hPrevInstance, (LPTSTR) &haccelTbl, sizeof(HANDLE));
   }

   if (!WndInitGlob (hInst, SkipProgramName (lpCmdLine), cmdShow))
      return (FALSE);

   /* poll messages from event queue until WM_QUIT */

      DB_OUT("Starting Loop\n");

   while (GetMessage(&msg, NULL, 0, 0)) {
       DB_OUTF((acDbgBfr,TEXT("pbrush.c, msg = %x\n"),msg.message));

       if (!hDlgModeless || !IsDialogMessage(hDlgModeless, &msg)) {
           if (pbrushWnd[PARENTid] &&
               !TranslateAccelerator(pbrushWnd[PARENTid], haccelTbl,
                                       (LPMSG) &msg)) {
               TranslateMessage(&msg);
               DispatchMessage(&msg);
           }
       }
#ifdef BLOCKONMSG
       if (fBlocked &&
           hServer && (lpOleServer = (PBSRVR FAR *)GlobalLock(hServer)))
       {
            BOOL bMore = TRUE;

            while (bMore)
                OleUnblockServer(lpOleServer->lhsrvr, &bMore);
            GlobalUnlock(hServer);
            fBlocked = FALSE;
       }
#endif
   }
   FreePrintHandles();


   return msg.wParam;
}
