/****************************Module*Header******************************\
* Module Name: pbrush.c                                                 *
*                                                                       *
* Created: 1989                                                         *
*                                                                       *
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
*                                                                       *
\***********************************************************************/

#include <windows.h>
#include <port1632.h>

#include "oleglue.h"
#include "pbrush.h"

extern HWND hDlgModeless;
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


//MMain is a macro from PWIN32.H (included via port1632.h)
MMain( hInstance, hPrevInstance, lpAnsiCmdLine, cmdShow )


   HRESULT hr;
   MSG msg;
   LPTSTR lpCmdLine = GetCommandLine();

   hInst = hInstance;

   SetMessageQueue(96);                   // CG recommended size for OLE apps
   dwOleBuildVersion = OleBuildVersion(); // CG
   //
   // CGTODO: add a build version check here
   //
   gfOleInitialized = ((hr = OleInitialize(NULL)) == NOERROR) ? TRUE : FALSE;

   /* if this is the first instance then call initialization procedure */
   if (!hPrevInstance)
   {
       if (!WndInit(hInstance))
       {
           msg.wParam = FALSE;
           DOUT(L"PBrush: WndInit Failed!\r\n");
           goto LExit;
       }
       haccelTbl = LoadAccelerators(hInstance, pgmName);
   }
   else
   {
       DOUT(L"PBrush: n'th Instance\r\n");
       /* else copy data from previous instance */
       GetInstanceData(hPrevInstance, pgmName, APPNAMElen);
       GetInstanceData(hPrevInstance, pgmTitle, TITLElen);
       GetInstanceData(hPrevInstance, (LPTSTR) &haccelTbl, sizeof(HANDLE));
   }

   if (!WndInitGlob (hInst, SkipProgramName (lpCmdLine), cmdShow))
   {
       msg.wParam = FALSE;
       DOUT(L"PBrush: WndInitGlob Failed!\r\n");
       goto LExit;
   }


   DB_OUT("Starting Loop\n");

   /* poll messages from event queue until WM_QUIT */


   while (GetMessage(&msg, NULL, 0, 0)) {
       DB_OUTF((acDbgBfr,TEXT("pbrush.c, msg = %x\n"),msg.message))

       if (gfInPlace)
       {
            // Only key messages need to be sent to OleTranslateAccelerator
            if ( (msg.message >= WM_KEYFIRST) && (msg.message <= WM_KEYLAST) )
            {
                LPOLEINPLACEFRAME pFrame;
                OLEINPLACEFRAMEINFO *pInfo;
                GetInPlaceInfo(&pFrame, &pInfo);
                // OleTranslateAccelerator MUST be called in order for the
                // mneumonics for the top level menu items to work properly
                if ( OleTranslateAccelerator ( pFrame, pInfo, &msg ) == NOERROR)
                {
                   continue;
                }
           }
       }

       if (!hDlgModeless || !IsDialogMessage(hDlgModeless, &msg))
       {
           if (pbrushWnd[PARENTid] &&
               !TranslateAccelerator(pbrushWnd[PARENTid],
                                        haccelTbl,
                                        (LPMSG)&msg))
           {
               TranslateMessage(&msg);
               DispatchMessage(&msg);
           }
       }
   }

   FreePrintHandles();

LExit:
   if(gfOleInitialized)            //CG
   {
        OleUninitialize();         //CG
        gfOleInitialized = FALSE;  //CG
   }

   return msg.wParam;
}
