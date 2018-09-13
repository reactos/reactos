// Copyright (c) 1997-1999 Microsoft Corporation
// KBMAIN.C 
// Additions, Bug Fixes 1999
// a-anilk, v-mjgran
//  
#define STRICT


#include <windows.h>
#include <commctrl.h>
#include <TCHAR.h>
#include <WinSvc.h>
#include <crtdbg.h>


#include "kbmain.h"
#include "Init_End.h"     // all the functions, buttons control for dialogs
#include "kbus.h"
#include "resource.h"
#include "htmlhelp.h"
#include "Msswch.h"
#include "About.h"
#include "door.h"


/**************************************************************************/
// FUNCTIONS IN THIS FILE
/**************************************************************************/
static BOOL  AssignDesktop(LPDWORD desktopID, LPTSTR pname);
static BOOL InitMyProcessDesktopAccess(VOID);
static VOID ExitMyProcessDesktopAccess(VOID);

/**************************************************************************/
// FUNCTIONS CALLED FROM THIS FILE
/**************************************************************************/
#include "sdgutil.h"
#include "kbfunc.h"
#include "scan.h"
#include "ms32dll.h"
#include "journal.h"
#include "fileutil.h"


/**************************************************************************/
// global initial
/**************************************************************************/
extern LPKBPREFINFO  lpkbPref = NULL;                    // Pointer to Preferences KB structure
extern LPKBPREFINFO  lpkbDefault = NULL;                 // ditto Default
extern HWND          *lpkeyhwnd = NULL;                  // ptr to array of HWND
extern HWND          numBasehwnd = NULL;                 // HWND to the num base window
extern HWND          kbmainhwnd = NULL;                  // HWND to the kbmain window
extern int           lenKBkey = 0;                       // How Many Keys?
extern int           scrCY = 0;                          // Screen Height
extern int           scrCX = 0;                          // Screen Width
extern int           captionCY = 0;                      // Caption Bar Height
extern int           g_margin = 0;                       // Margin between rows and columns
extern BOOL          smallKb = FALSE;                    // TRUE when working with Small Keyboard
extern COLORREF      PrefTextKeyColor    = 0x00000000;   // Prefered Color for text in keys
extern COLORREF      PrefCharKeyColor  = 0x00C0C0C0;     // ditto normal key
extern COLORREF      PrefModifierKeyColor= 0x00B5B500;   // ditto modifier key
extern COLORREF      PrefDeadKeyColor  = 0x0040ff00;     // ditto dead key
extern COLORREF       PrefBackgroundColor = 0x00C0C0C0;  // ditto Keyboard backgraund
extern BOOL          PrefAlwaysontop = TRUE;             // Always on Top control
extern int           PrefDeltakeysize = 2;               // Preference increment in key size
extern BOOL          PrefshowActivekey = TRUE;           // Show cap letters in keys
extern int           KBLayout = 101;                     // 101, 102, 106, KB layout
extern BOOL          Pref3dkey = TRUE;                   // Use 3d keys
extern BOOL          Prefusesound = FALSE;               // Use click sound
extern BOOL          newFont = FALSE;                    // Font is changed
extern HGDIOBJ       oldFontHdle = NULL;                 // Old object handle
extern LOGFONT       *plf = NULL;                        // pointer to the actual char font
extern COLORREF      InvertTextColor = 0xFFFFFFFF;       // Font color on inversion
extern COLORREF      InvertBKGColor = 0x00000000;        // BKG color on inversion
extern BOOL          Prefhilitekey = TRUE;               // True for hilite key under cursor
// Dwelling time control variables
extern BOOL          PrefDwellinkey = FALSE;             // use dwelling system
extern UINT          PrefDwellTime = 1000;               // Dwell time preference  (ms)

extern BOOL          PrefScanning = FALSE;               // use scanning
extern UINT          PrefScanTime = 1000;                // Prefer scan time

extern BOOL          g_fShowWarningAgain = 1;            // Show initial warning dialog again

extern HWND          Dwellwindow = NULL;                 // dwelling window HANDLE
                                                         
extern int           stopPaint = FALSE;                  // stop the bucket paint on keys
                                                         
extern UINT_PTR      timerK1 = 0;                        // timer id
extern UINT_PTR      timerK2 = 0;                        // timer for bucket

extern DWORD         platform;
extern BOOL          kbfCapLetter=FALSE;                 // flag Capital Letters key
extern BOOL          kbfCapLock=FALSE;

extern BOOL          g_fDrawCapital = FALSE;
extern HWND          g_hBitmapLockHwnd;

extern HINSTANCE     hInst = NULL;
extern KBPREFINFO    *kbPref = NULL;
extern BOOL          prefUM;
HANDLE               g_hMutexOSKRunning;
// Global variable to indicate if it was started from UM
extern BOOL			g_startUM = FALSE;
UINT taskBarStart;


#define UTILMAN_DESKTOP_CHANGED_MESSAGE   __TEXT("UtilityManagerDesktopChanged")
static HWINSTA origWinStation = NULL;
static HWINSTA userWinStation = NULL;

// For Link Window
EXTERN_C BOOL WINAPI LinkWindow_RegisterClass() ;
DWORD GetDesktop();

#define        MAX_TOOLTIP_SIZE  256
TOOLINFO       ti;
HWND           g_hToolTip;

/****************************************************************************/
/* LRESULT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,     */
/*                   LPSTR lpCmdLine, int nCmdShow)                    */
/****************************************************************************/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow)
{
   MSG  msg;               /* message                        */
   
   // For UM
   DWORD desktopID;
   TCHAR name[300];
   TCHAR szToolTipText[MAX_TOOLTIP_SIZE];

   UINT deskSwitchMsg;

      // Get the commandline so that it works for MUI/Unicode
   LPTSTR lpCmdLineW = GetCommandLine();
   
   if(NULL != lpCmdLineW && lstrlen(lpCmdLineW))
   {
      if ( lstrcmpi(lpCmdLineW, TEXT("/UM")) == 0 )
         g_startUM = TRUE;
   }

   SetLastError(0);
   // Allow only ONE instance of the program running.
   g_hMutexOSKRunning = CreateMutex(NULL, TRUE, TEXT("OSKRunning"));
   if ( (g_hMutexOSKRunning == NULL) ||
       (GetLastError() == ERROR_ALREADY_EXISTS) )
   {
       // Exit without starting
       return 0;
   }

   // for the Link Window in finish page...
   LinkWindow_RegisterClass();

   deskSwitchMsg = RegisterWindowMessage(UTILMAN_DESKTOP_CHANGED_MESSAGE);// support UM
   taskBarStart = RegisterWindowMessage(TEXT("TaskbarCreated"));
   
   // If we are on Terminal server, Then exit
   
   // Check to see if we are called from a Terminal Client session. 
   if (GetSystemMetrics(SM_REMOTESESSION)) 
   {
      TCHAR buff[256];
      // Error message box. 
      LoadString(hInstance, IDS_TSERROR, buff, 100);
      MessageBox(NULL, buff, NULL, MB_ICONERROR );
      return 0;
   }

   //*** Find out what operating system is running ***
    platform=WhatPlatform();


    hInst = hInstance;          /* Saves the current instance     */

    //*** Loading the setting file and init setting ***
   PredictInit();    //init screen doors preferences


   //*******************************************************

    // UM
    InitMyProcessDesktopAccess();
   AssignDesktop(&desktopID,name);

   prefUM = CheckUM();

   if(!InitProc())
      return 0;               // Process terminated before Msg loop

   if(!RegisterWndClass(hInst))
      return 0;               // Process terminated before Msg loop

   mlGetSystemParam();             // Get system parameters

   kbmainhwnd = CreateMainWindow(FALSE);

   if(kbmainhwnd == NULL)
   {
      SendErrorMessage(IDS_CANNOT_CREATE_KB);
      return 0;          // Process terminated before Msg loop
   }

   if(!rSetWindowPos())   // Set the main window position (topmost/non-topmost)
      return 0;


   //*** Init all the keys' color before show them ***
   DeleteChildBackground();

   ShowWindow(kbmainhwnd, SW_SHOWNORMAL);
   UpdateWindow (kbmainhwnd);

   //Create the helpballon
   InitCommonControls();
     
   g_hToolTip = CreateWindowEx(WS_EX_TOPMOST,TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_BALLOON ,
                               0, 0, 0, 0, NULL, NULL, hInstance, NULL);
   if (g_hToolTip)
   {
      ti.cbSize = sizeof(ti);
      ti.uFlags = TTF_TRANSPARENT | TTF_CENTERTIP | TTF_TRACK;
      ti.hwnd = kbmainhwnd;
      ti.uId = 0;
      ti.hinst = hInstance;
      
      LoadString(hInstance, IDS_TOOLTIP, szToolTipText, MAX_TOOLTIP_SIZE);
      ti.lpszText = szToolTipText;
      
      SendMessage(g_hToolTip, TTM_ADDTOOL, 0, (LPARAM) &ti );
   }

   Create_The_Rest(lpCmdLine, hInstance);

   // check if there is necessary to show the initial warning msg
   if (g_fShowWarningAgain && ((GetDesktop() == DESKTOP_DEFAULT)))
      WarningMsgDlgFunc(kbmainhwnd);


    //***  Message loop (Main loop) ***
   while (GetMessage(&msg, 0, 0, 0))
   {
#ifndef _DEBUG
         //Handle the WM_CANCELJOURNAL message
         if(msg.message== WM_CANCELJOURNAL)
            {
            hkJRec = SetWindowsHookEx(WH_JOURNALRECORD,  JournalRecordProc, 
                                          hInst, 0);
            }

#endif


            TranslateMessage(&msg); /* Translates character keys             */
            DispatchMessage(&msg);  /* Dispatches message to window          */

            if ( msg.message == deskSwitchMsg )
            {
               // ReleaseMutex(g_hMutexOSKRunning);
               DestroyWindow(kbmainhwnd);
               FinishProcess();

               Sleep(100);
                
               AssignDesktop(&desktopID,name);
                
               Sleep(100);
               // Create the window and set the position.
               kbmainhwnd = CreateMainWindow(TRUE);
               rSetWindowPos();
               // DeleteChildBackground();

               // Show Window and Re-Create the thread
               ShowWindow(kbmainhwnd, SW_SHOWNORMAL);
               UpdateWindow (kbmainhwnd);
               Create_The_Rest(lpCmdLine, hInstance);
            }
   }
    // UM
    ExitMyProcessDesktopAccess();


   // v-mjgran: Add Memory Leak control
   #ifdef _DEBUG
      _CrtDumpMemoryLeaks();
   #endif


   return((int)msg.wParam);     /* Returns the value from PostQuitMessage   */
}




/****************************************************************************/
extern BOOL  Setting_ReadSuccess;    //read the setting file success ?

extern BOOL ForStartUp1=TRUE;
extern BOOL ForStartUp2=TRUE;

HWND htip=NULL;

extern float g_KBC_length = 0;

/*****************************************************************************/
//
//  kbMainWndProc
//  Explain how Large and Small KB switching:
//  All the keys are sizing according to the size of the KB window. So change
//  from Large KB to Small KB and make the KB to (2/3) of the original but
//  same key size. We need to set the KB size to (2/3) first. But use the 
//  original KB client window length to calculate "colMargin" to get the same
//  key size.
/*****************************************************************************/
LRESULT WINAPI kbMainWndProc(HWND hwnd, UINT message, 
                             WPARAM wParam, LPARAM lParam)
{
   register int  i;

   static   int  oldWidth  = 0;
   static   int  oldHeight = 0;

   int   x;
   TCHAR Wclass[50]=TEXT("");
   BOOL  isvisible;
   RECT  rect, rectC;
   int   rmargin, bmargin;          //will set to smallest width and height
   LONG_PTR ExStyle;

   TCHAR str[128];

    //
    // explain: rowMargin is the ratio to the smallest hegiht(KB_CHARBMARGIN)
    //
   float rowMargin, colMargin; 
                           
   //e.g. rowMargin=4, that means the current KB height is 4 times of
    //     KB_CHARBMARGIN

    static  BOOL StopIt=FALSE;
   static  HWND hPreWindow=NULL;   //save the previous window handle

   switch (message)
      {
      case WM_CREATE:

         if(lpkeyhwnd==NULL)
            lpkeyhwnd = LocalAlloc(LPTR, sizeof(HWND) * lenKBkey);
         
         //Check to see the CapsLock on the keyboard is On or Off

         if(LOBYTE(GetKeyState(VK_CAPITAL))& 0x01)
         {
            kbfCapLetter = TRUE;
            kbfCapLock = TRUE;
            g_fDrawCapital = TRUE;
         }

         // Turn off mirroring for the main hwnd to create thw keyboard layout unmirrored.
         ExStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
         SetWindowLongPtr(hwnd, GWL_EXSTYLE, ExStyle & ~WS_EX_LAYOUTRTL); 
         for (i = 1; i < lenKBkey; i++)
         {

            switch (KBkey[i].ktype)
               {
               case KNORMAL_TYPE:   
                        wsprintf(Wclass, TEXT("N%d"), i); 
                    break;
               case KMODIFIER_TYPE: 
                        wsprintf(Wclass, TEXT("M%d"), i);
                    break;
               case KDEAD_TYPE:
                        wsprintf(Wclass, TEXT("D%d"), i); 
                    break;
               case NUMLOCK_TYPE:
                  wsprintf(Wclass, TEXT("NL%d"), i); 
               break;
               case SCROLLOCK_TYPE:
                  wsprintf(Wclass, TEXT("SL%d"), i);
               break;

               }
            
            //Decide to Show or Hide the keys
            if(((smallKb == TRUE) && (KBkey[i].smallKb == SMALL)) ||
                  ((smallKb == FALSE) && (KBkey[i].smallKb == LARGE)) ||
                  (KBkey[i].smallKb == BOTH))
               isvisible = TRUE;   //Show the keys
            else
               isvisible = FALSE;  //Hide the keys 

                lpkeyhwnd[i] = CreateWindow(Wclass, 
                                            KBkey[i].textC,
                                            (WS_CHILD | 
                                             (WS_VISIBLE * isvisible) |
                                             BS_PUSHBUTTON | WS_BORDER),
                                            KBkey[i].posX * g_margin,
                                   KBkey[i].posY * g_margin,
                                   (KBkey[i].ksizeX * g_margin + 
                                             PrefDeltakeysize),
                                            (KBkey[i].ksizeY * g_margin + 
                                             PrefDeltakeysize),
                                            hwnd, (HMENU)i, hInst, NULL);

            if(lpkeyhwnd[i] == NULL)
            {
               SendErrorMessage(IDS_CANNOT_CREATE_KEY);
               break;
            }

         }

         SetWindowLongPtr(hwnd, GWL_EXSTYLE, ExStyle); 
   

      return 0;

      case WM_SIZE:

      if(!StopIt)
      {  int KB_SMALLRMARGIN= 137;

         GetClientRect(kbmainhwnd, &rectC);
         GetWindowRect(kbmainhwnd, &rect);

         if ((oldWidth == rect.right) && (oldHeight == rect.bottom))
            return 0;

         bmargin  = KB_CHARBMARGIN;      //smallest height


         // SmallMargin for Actual / Block layout
         if(kbPref->Actual)
            KB_SMALLRMARGIN = KB_LARGERMARGIN;  //Actual
         else
            KB_SMALLRMARGIN = 224;  //Block


         if(smallKb == TRUE)        //small keyboard ??
            rmargin = KB_SMALLRMARGIN;   //smallest width (with smallKB)
         else
            rmargin = KB_LARGERMARGIN;   //smallest width (with largeKB)


            //See explain to see how it works.
            if(smallKb && ForStartUp1)   //Start up with Small KB
            {
                colMargin = ((float)rectC.right * 3 / 2 - 10) / (float)rmargin;

                //why - 10? -> The number not really match the origianl size,
                // so - 10
            }
            else if(smallKb)        //Small KB but NOT at start up
            {
                colMargin = g_KBC_length / (float)rmargin;
            }
            else        //Large KB
            {
                //rmargin is smallest width; colMargin is the ratio ;
                // see explain

                colMargin = (float)rectC.right / (float)rmargin; 
            }


            //bmargin is smallest height; rowMargin is the ratio; see explain
            rowMargin = (float)rectC.bottom  / (float)bmargin;  




         /*****   place to the right place on screen at STARTUP TIME   *****/


            //*** At StartUp and CANNOT read setting file  ***
         if(ForStartUp1 && !Setting_ReadSuccess)    
            {
                //place to the lower left conner
                ForStartUp1= FALSE;

                rect.bottom = rect.bottom - 
                              (rectC.bottom - ((int)rowMargin * bmargin));
            rect.right = rect.right - 
                             (rectC.right - ((int)colMargin * rmargin));

            StopIt= TRUE;
            MoveWindow(kbmainhwnd, rect.left,              //Move Keyboard
                                scrCY-30-(rect.bottom - rect.top),
                                rect.right - rect.left,
                                rect.bottom - rect.top,  TRUE);
         }


         //*** At StartUp and SUCCESS read setting file ***
         else if(ForStartUp1 && Setting_ReadSuccess)
         {  
                ForStartUp1= FALSE;

            StopIt= TRUE;           
            // Check to see the KB is  not out of screen with the current resolution
            if(IsOutOfScreen(scrCX, scrCY))
               //Out of screen, so put it to the default position
               MoveWindow(kbmainhwnd, scrCX/2 - (kbPref->KB_Rect.right - kbPref->KB_Rect.left)/2,        //left
                           scrCY - 30 - (kbPref->KB_Rect.bottom - kbPref->KB_Rect.top),    //top
                          kbPref->KB_Rect.right - kbPref->KB_Rect.left,           //length
                           kbPref->KB_Rect.bottom - kbPref->KB_Rect.top, TRUE);    //height
            
            else     //Put it to the last saved position
               MoveWindow(kbmainhwnd, kbPref->KB_Rect.left,        //left
                           kbPref->KB_Rect.top,
                          kbPref->KB_Rect.right - kbPref->KB_Rect.left,           //length
                           kbPref->KB_Rect.bottom - kbPref->KB_Rect.top, TRUE);    //height

         }



         StopIt = FALSE;

         oldWidth = rect.right;
         oldHeight = rect.top;



         // Turn off mirroring for the main hwnd to position the buttons from left to right.
         ExStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
         SetWindowLongPtr(hwnd, GWL_EXSTYLE, ExStyle & ~WS_EX_LAYOUTRTL); 

         /*****   for each key  *****/
         for (i = 1 ; i < lenKBkey ; i++)
         {

            int w, h;   //width and height of each window key
            
            
            // *** show / not show the keys between small/large keyboard
            if(((smallKb == TRUE) && (KBkey[i].smallKb == SMALL)) ||
               ((smallKb == FALSE) && (KBkey[i].smallKb == LARGE)) ||
                (KBkey[i].smallKb == BOTH))

               ShowWindow(lpkeyhwnd[i], SW_SHOW);
            else
               ShowWindow(lpkeyhwnd[i], SW_HIDE);


            //*** At StartUp and CANNOT read setting file  ***
            if(ForStartUp2 && !Setting_ReadSuccess)
            {
                    //move Keys
                  MoveWindow(lpkeyhwnd[i],   
                        KBkey[i].posX * (int)colMargin,
                        KBkey[i].posY * (int)rowMargin,
                                (KBkey[i].ksizeX * (int)colMargin + 
                                 PrefDeltakeysize),
                        (KBkey[i].ksizeY * (int)rowMargin + 
                                 PrefDeltakeysize),
                        TRUE);
            }

                //
            // At not startup / at startup and SUCCESS read setting file 
                //
            else
            {  MoveWindow(lpkeyhwnd[i],                        //move Keys
                        (int)((float)KBkey[i].posX * colMargin),
                        (int)((float)KBkey[i].posY * rowMargin),
                                ((int)((float)KBkey[i].ksizeX * colMargin) + 
                                 PrefDeltakeysize),
                        ((int)((float)KBkey[i].ksizeY * rowMargin) + 
                                 PrefDeltakeysize),
                        TRUE);
            }


            w = (int) ((KBkey[i].ksizeX * colMargin) + PrefDeltakeysize);
            h = (int) ((KBkey[i].ksizeY * rowMargin) + PrefDeltakeysize);

            SetKeyRegion(lpkeyhwnd[i], w, h);  //set the region we want for each key

         }   //end for loop

         SetWindowLongPtr(hwnd, GWL_EXSTYLE, ExStyle); 
         ForStartUp2= FALSE;

      }  //StopIt

      if(!IsIconic(kbmainhwnd))
         GetWindowRect(kbmainhwnd, &kbPref->KB_Rect);

      return 0;

      case WM_SHOWWINDOW:
         //Hilit the NUMLOCK key if it is on
         RedrawNumLock();

         //hilite the Scroll Key if it is on
         RedrawScrollLock();
      return 0;


/*
        case WM_SIZING:      //Show ToolTip and stop user resizing
      {
            LPRECT lprc;
         TOOLINFO ti;

            LoadString(hInst, IDS_FULLRESIZE, &str[0], 256);

         //fill in the tool tip info.
         ti.cbSize = sizeof(TOOLINFO);
         ti.uFlags = TTF_CENTERTIP|TTF_IDISHWND|TTF_SUBCLASS;
         ti.hwnd = kbmainhwnd;
         ti.uId = (UINT)kbmainhwnd;
         ti.lpszText = &str[0];
         ti.lParam = 0;

         if(htip == NULL)
            {
            htip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL, WS_POPUP, 
                                      CW_USEDEFAULT, CW_USEDEFAULT, 
                                      CW_USEDEFAULT, CW_USEDEFAULT,
                               kbmainhwnd, NULL, hInst, NULL);
          } 
         
         SendMessage(htip, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
         SendMessage(htip, TTM_SETDELAYTIME, (WPARAM)(DWORD) TTDT_INITIAL, 
                        (LPARAM)(INT) MAKELONG(0,0));
         SendMessage(htip, TTM_SETMAXTIPWIDTH, 0, (LPARAM)(INT) 500);
         SendMessage(htip, TTM_SETTIPTEXTCOLOR, 
                        (WPARAM)(COLORREF) RGB(255,255,255), 0);
         SendMessage(htip, TTM_SETTIPBKCOLOR, 
                        (WPARAM)(COLORREF) RGB(0,0,255), 0);
         SendMessage(htip, TTM_ACTIVATE, (WPARAM) (BOOL) TRUE, 0);
      
         //Stop the user from resizing 
         lprc = (LPRECT) lParam;  
         CopyRect(lprc, &kbPref->KB_Rect);
      }
      return 1;
*/


      case WM_MOVE:      //Show ToolTip and save KB position
      {  
/*
         TOOLINFO ti;
         static int count=0;

            LoadString(hInst, IDS_AUTOPOSITION, &str[0], 256);

         //fill in the tool tip info.
         ti.cbSize = sizeof(TOOLINFO);
         ti.uFlags = TTF_CENTERTIP|TTF_IDISHWND|TTF_SUBCLASS;
         ti.hwnd = kbmainhwnd;
         ti.uId = (UINT)kbmainhwnd;
         ti.lpszText = &str[0];
         ti.lParam = 0;

         if(htip == NULL)
            {
            htip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL, WS_POPUP, 
                                      CW_USEDEFAULT, CW_USEDEFAULT, 
                                      CW_USEDEFAULT, CW_USEDEFAULT,
                                 kbmainhwnd, NULL, hInst, NULL);
            }

         //Show the tool tip
         if(count == 2)
         {
            SendMessage(htip, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
            SendMessage(htip, TTM_SETDELAYTIME,  
                            (WPARAM)(DWORD) TTDT_INITIAL, 
                            (LPARAM)(INT) MAKELONG(0,0));
            SendMessage(htip, TTM_SETMAXTIPWIDTH, 0, (LPARAM)(INT) 500);
            SendMessage(htip, TTM_SETTIPTEXTCOLOR, 
                            (WPARAM)(COLORREF) RGB(255,255,255), 0);
            SendMessage(htip, TTM_SETTIPBKCOLOR, 
                            (WPARAM)(COLORREF) RGB(0,0,255), 0);
            SendMessage(htip, TTM_ACTIVATE, (WPARAM) (BOOL) TRUE, 0);

            count = 0;
         }

         //Not show the tool tip
         else
         {  count++;
            SendMessage(htip, TTM_DELTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);

         }
*/


         //Save the KB position
         if(!IsIconic(kbmainhwnd))
            GetWindowRect(kbmainhwnd, &kbPref->KB_Rect);

      }
      return 0;


/*
      case WM_MOUSEMOVE:
         {  
            TOOLINFO ti;
            TCHAR str[]= TEXT("");
         

            //fill in the tool tip info.
            ti.cbSize = sizeof(TOOLINFO);
            ti.uFlags = TTF_CENTERTIP|TTF_IDISHWND|TTF_SUBCLASS;
            ti.hwnd = kbmainhwnd;
            ti.uId = (UINT)kbmainhwnd;
            ti.lpszText = &str[0];
            ti.lParam = 0;
            
            SendMessage(htip, TTM_DELTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
         }
      return 0;
*/



      //When user drag the keyboard or re-size
        case WM_ENTERSIZEMOVE:
        return 0;

      //When user finish draging or re-sizing
      case WM_EXITSIZEMOVE:
            //
            //switch back the focus to previous window
            //
         SetForegroundWindow(hPreWindow);  
      return 0;



      case WM_MOUSEACTIVATE:
         GetWindowRect(kbmainhwnd, &rectC);
         x = LOWORD(lParam);

         if(x==HTCAPTION     || x==HTSIZE     || x==HTREDUCE     || 
               x==HTSYSMENU     || x==HTLEFT     || x==HTTOP        || 
               x==HTRIGHT       || x==HTBOTTOM   || x==HTZOOM       || 
               x==HTTOPLEFT     || x==HTTOPRIGHT || x==HTBOTTOMLEFT ||
               x==HTBOTTOMRIGHT || x==HTMENU     ||x==HTCLOSE)
            {
                // save the current window handle
                hPreWindow = GetActiveWindow();

                // make window active
            return MA_ACTIVATE;  
            }
         else
            {
                //  not activate and discard the mouse message
                return MA_NOACTIVATEANDEAT; 
            }


      case WM_COMMAND:
         BLDMenuCommand(hwnd, message, wParam, lParam);
      break;

      case WM_CLOSE:
      return(BLDExitApplication(hwnd));       /* Clean up if necessary*/

      case WM_QUERYENDSESSION:
      return TRUE;

      case WM_ENDSESSION:
      {
          HKEY hKey;
          DWORD dwPosition;
          const TCHAR szSubKey[] =  __TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce");
          const TCHAR szImageName[] = __TEXT("OSK.exe");

          BLDExitApplication(hwnd);

          if ( ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, szSubKey, 0, NULL,
             REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &hKey, &dwPosition))
          {
             RegSetValueEx(hKey, NULL, 0, REG_SZ, (CONST BYTE*)szImageName, (lstrlen(szImageName)+1)*sizeof(TCHAR) );
             RegCloseKey(hKey);
          }
          else
             MessageBox(hwnd, 0, 0, MB_OK);
  
      }
      return 0;
      
      case WM_TIMER:
		  KillTimer(hwnd, 1014);
		  SendMessage(g_hToolTip,TTM_TRACKACTIVATE,(WPARAM)FALSE,(LPARAM)&ti);
			
		  break;

      case WM_DESTROY:            /* window being destroyed                   */

         FinishProcess();   //Unhook journal and mouse hook
         PostQuitMessage(0);
      return TRUE;

    //Call the scanning again
    case WM_USER + 1:
      Scanning(1);
    return TRUE;

     case WM_INITMENUPOPUP:
        {
            HMENU   hMenu;

            hMenu = (HMENU) wParam;

            CheckMenuItem(hMenu, IDM_ALWAYS_ON_TOP, 
                          (PrefAlwaysontop ? MF_CHECKED : MF_UNCHECKED));

            CheckMenuItem(hMenu, IDM_CLICK_SOUND, 
                          (Prefusesound ? MF_CHECKED : MF_UNCHECKED));



          if (prefUM)
            LoadString(hInst, IDS_REMUM, str, 128); 
          else
            LoadString(hInst, IDS_ADDTOUM, str, 128);
         
            ModifyMenu(hMenu, IDM_ADDUM, MF_BYCOMMAND | MF_STRING, IDM_ADDUM, str );
         
         if (!RunningAsAdministrator() || (GetDesktop() != DESKTOP_DEFAULT))
         {
            //Disable the "Add to / Remove from Utility Manager" options
            EnableMenuItem(hMenu, IDM_ADDUM, MF_GRAYED);
         }

            //Small or Large KB
            if(kbPref->smallKb)
            {        
                CheckMenuRadioItem(hMenu, IDM_LARGE_KB, IDM_SMALL_KB, 
                                   IDM_SMALL_KB, MF_BYCOMMAND);
            }
            else
            {
                CheckMenuRadioItem(hMenu, IDM_LARGE_KB, IDM_SMALL_KB, 
                                   IDM_LARGE_KB, MF_BYCOMMAND);
            }

            //Regular or Block Layout
            if(kbPref->Actual)
            {
                CheckMenuRadioItem(hMenu, IDM_REGULAR_LAYOUT, IDM_BLOCK_LAYOUT,
                                   IDM_REGULAR_LAYOUT, MF_BYCOMMAND);
            //Able the 102, 106 menu 
            EnableMenuItem(hMenu, IDM_102_LAYOUT, MF_ENABLED);
            EnableMenuItem(hMenu, IDM_106_LAYOUT, MF_ENABLED);

         }

            else   //Block layout 
            {
                CheckMenuRadioItem(hMenu, IDM_REGULAR_LAYOUT, IDM_BLOCK_LAYOUT,
                                   IDM_BLOCK_LAYOUT, MF_BYCOMMAND);

            //Disable the 102, 106 menu
            EnableMenuItem(hMenu, IDM_102_LAYOUT, MF_GRAYED);
            EnableMenuItem(hMenu, IDM_106_LAYOUT, MF_GRAYED);
            }

         switch (kbPref->KBLayout)
            {
         case 101:
            CheckMenuRadioItem(hMenu,IDM_101_LAYOUT, IDM_106_LAYOUT,
                           IDM_101_LAYOUT, MF_BYCOMMAND);
      
            //disable these two menus
            EnableMenuItem(hMenu, IDM_REGULAR_LAYOUT, MF_ENABLED);
            EnableMenuItem(hMenu, IDM_BLOCK_LAYOUT, MF_ENABLED);
         break;

         case 102:
            CheckMenuRadioItem(hMenu,IDM_101_LAYOUT, IDM_106_LAYOUT,
                           IDM_102_LAYOUT, MF_BYCOMMAND);
      
            //disable these two menus
            EnableMenuItem(hMenu, IDM_REGULAR_LAYOUT, MF_GRAYED);
            EnableMenuItem(hMenu, IDM_BLOCK_LAYOUT, MF_GRAYED);
         break;

         case 106:
            CheckMenuRadioItem(hMenu,IDM_101_LAYOUT, IDM_106_LAYOUT,
                           IDM_106_LAYOUT, MF_BYCOMMAND);
      
            //disable these two menus
            EnableMenuItem(hMenu, IDM_REGULAR_LAYOUT, MF_GRAYED);
            EnableMenuItem(hMenu, IDM_BLOCK_LAYOUT, MF_GRAYED);
         break;
            }

		 // Disable help menus
		 if ( (GetDesktop() != DESKTOP_DEFAULT) )
		 {
            EnableMenuItem(hMenu, CM_HELPABOUT, MF_GRAYED);
            EnableMenuItem(hMenu, CM_HELPTOPICS, MF_GRAYED);
		 }

        }
        return 0;


      case WM_HELP:
       {
			 if ( (GetDesktop() == DESKTOP_DEFAULT) )
			{

				TCHAR buf[MAX_PATH]=TEXT("");

				GetWindowsDirectory(buf,MAX_PATH);
				lstrcat(buf, TEXT("\\HELP\\OSK.CHM"));

				HtmlHelp(NULL, buf, HH_DISPLAY_TOPIC, 0);
			}
       }
      return TRUE;


        case SW_SWITCH1DOWN:

            if(PrefScanning)
                Scanning(1);

        break;

		default:
			if ( (message == taskBarStart) && !hkJRec )
			{
				hkJRec = SetWindowsHookEx(WH_JOURNALRECORD,	JournalRecordProc, hInst, 0);
				if(hkJRec == NULL)
				{	
					SendErrorMessage(IDS_JOURNAL_HOOK);
					SendMessage(kbmainhwnd, WM_DESTROY,0L,0L);  //destroy myself#include "journal.h"
				}
			}
				break;

//      case SW_SWITCH2DOWN:    //Stop scanning

//          if(PrefScanning)
//              KillScanTimer(TRUE);   //Reset the scanning
//      break;

    //  case SW_SWITCH3DOWN:
    //  case SW_SWITCH4DOWN:
    //  case SW_SWITCH5DOWN:
    //  case SW_SWITCH6DOWN:



//      case SW_SWITCH1UP:
    //  case SW_SWITCH2UP:
    //  case SW_SWITCH3UP:
    //  case SW_SWITCH4UP:
    //  case SW_SWITCH5UP:
    //  case SW_SWITCH6UP:

//      break;

    }
   return DefWindowProc (hwnd, message, wParam, lParam) ;
}


/*****************************************************************************/
/* LRESULT WINAPI kbKeyWndProc (HWND Childhwnd, UINT message, WPARAM wParam, */
/*                       LPARAM lParam)                              */
/* BitMap Additions : a-anilk: 02-16-99                               */
/*****************************************************************************/
LRESULT WINAPI kbKeyWndProc (HWND Childhwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   HDC         hdc ;
   PAINTSTRUCT ps ;
   RECT        rect;
   int         index, bpush;



   index = GetWindowLong(Childhwnd, GWL_ID);  //order of the key in the array
   switch (message)
      {
      case WM_CREATE:
         SetWindowLong(Childhwnd, 0, 0) ;       // on/off flag
         return 0 ;

      case WM_PAINT:
         hdc = BeginPaint(Childhwnd, &ps);
         GetClientRect(Childhwnd, &rect);
         bpush = GetWindowLong(Childhwnd,0);

         switch(bpush)
            {
            case 0:           //*** Normal Button ***
               if (KBkey[index].name == BITMAP)
               {
                  if (g_fDrawCapital)
                  {
                     if (KBkey[index].scancode[0] == 0x3a)
                     {
                        SetWindowLong(Childhwnd, 0, 1);
                        SetClassLongPtr(Childhwnd, GCLP_HBRBACKGROUND, 
                                     (LONG_PTR)CreateSolidBrush(RGB(0,0,0)));

                        InvalidateRect(Childhwnd, NULL, TRUE);




                        RDrawBitMap(hdc, KBkey[index].skLow, bpush, rect, FALSE);
                        g_hBitmapLockHwnd = Childhwnd;
                      }
                      else
                        RDrawBitMap(hdc, KBkey[index].textL, bpush, rect, TRUE);
                  }
                  else
                     RDrawBitMap(hdc, KBkey[index].textL, bpush, rect, TRUE);
               }
               //////////////////////
               else
               {
                  if (g_fDrawCapital&& KBkey[index].scancode[0] == 0x3a)
                  {
                     SetWindowLong(Childhwnd, 0, 1);
                     SetClassLongPtr(Childhwnd, GCLP_HBRBACKGROUND, 
                                     (LONG_PTR)CreateSolidBrush(RGB(0,0,0)));

                     InvalidateRect(Childhwnd, NULL, TRUE);
               
               
                  }
               }
               //////////////////////

               if(Pref3dkey == TRUE)
                  udfDraw3D(hdc, rect);         // draw borders

               if(KBkey[index].name == ICON)
                  RDrawIcon(hdc, KBkey[index].textL, bpush, rect);

                    // *** Draw the LED light
/*                    if(KBkey[index].name == KB_CAPLOCK ||
                           KBkey[index].name == KB_NUMLOCK ||
                           KBkey[index].name == KB_SCROLL)
                        DrawIcon_KeyLight(hdc, KBkey[index].name, rect);
*/
               break;

            case 1:          //*** Button down ***
               if(KBkey[index].name == BITMAP)
                  RDrawBitMap(hdc, KBkey[index].skLow, bpush, rect, TRUE);
               
               if(Pref3dkey == TRUE)
                  udfDraw3Dpush(hdc, rect, FALSE);
               
               if(KBkey[index].name == ICON)
                  RDrawIcon(hdc, KBkey[index].textC, bpush, rect);
               break;

            case 3:          //*** Do Nothing ***   (delete later)
//             InvertRect(hdc, &rect);
               break;

            case 4:    
                    //
                    //*** draw a red rect around the key when move around
                    //
               
               if(PrefScanning)  //if in scanning, don't put red outline
                  udfDraw3Dpush(hdc, rect, FALSE);
               else
                  udfDraw3Dpush(hdc, rect, TRUE);
               
               
               if(KBkey[index].name == ICON)
                  RDrawIcon(hdc, KBkey[index].skLow, bpush, rect);

               else if(KBkey[index].name == BITMAP)
                  RDrawBitMap(hdc, KBkey[index].skLow, bpush, rect, FALSE);
                                        // *** Draw the LED light
/*                    if(KBkey[index].name == KB_CAPLOCK ||
                           KBkey[index].name == KB_NUMLOCK ||
                           KBkey[index].name == KB_SCROLL)
                            DrawIcon_KeyLight(hdc, KBkey[index].name, rect);
*/
               break;

            case 5:          //*** Dwell (Paint bucket) ***
               PaintLine(Childhwnd, hdc, rect);
               EndPaint(Childhwnd, &ps);

               if (KBkey[index].name != BITMAP)
                  SetWindowLong(Dwellwindow, 0, 1);
               else
                  SetWindowLong(Dwellwindow, 0, 4);
               
               return 0;
            }

            if(Pref3dkey != TRUE && bpush != 4)
               bpush = 0;

            //*** Print the text on each button ***
            if(KBkey[index].name != ICON)
               vPrintCenter(Childhwnd, hdc, rect, index, bpush);
         EndPaint(Childhwnd, &ps);
         return 0;

      case WM_MOUSEACTIVATE:
         return MA_NOACTIVATEANDEAT;
      default:
         break;
      }
   return DefWindowProc (Childhwnd, message, wParam, lParam) ;
}


/******************************************************************************/
/*LRESULT WINAPI kbNumBaseWndProc (HWND NumBasehwnd, UINT message,            */
/*                         WPARAM wParam, LPARAM lParam)             */
/******************************************************************************/
LRESULT WINAPI kbNumBaseWndProc(HWND NumBasehwnd, UINT message,
                                WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc (NumBasehwnd, message, wParam, lParam) ;
} 

/******************************************************************************/
/*LRESULT WINAPI kbNumWndProc (HWND Numhwnd, UINT message,                    */
/*                      WPARAM wParam, LPARAM lParam)                 */
/******************************************************************************/
LRESULT WINAPI kbNumWndProc(HWND Numhwnd, UINT message,
                            WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc (Numhwnd, message, wParam, lParam) ;
} 

/**************************************************************************/


// AssignDeskTop() For UM
// a-anilk. 1-12-98
static BOOL  AssignDesktop(LPDWORD desktopID, LPTSTR pname)
{
    HDESK hdesk;
    wchar_t name[300];
    DWORD nl;
    // Beep(1000,1000);

    *desktopID = DESKTOP_ACCESSDENIED;
    hdesk = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
    if (!hdesk)
    {
        // OpenInputDesktop will mostly fail on "Winlogon" desktop
        hdesk = OpenDesktop(__TEXT("Winlogon"),0,FALSE,MAXIMUM_ALLOWED);
        if (!hdesk)
            return FALSE;
    }
    GetUserObjectInformation(hdesk,UOI_NAME,name,300,&nl);
    if (pname)
        wcscpy(pname, name);
    if (!_wcsicmp(name, __TEXT("Default")))
        *desktopID = DESKTOP_DEFAULT;
    else if (!_wcsicmp(name, __TEXT("Winlogon")))
    {
        *desktopID = DESKTOP_WINLOGON;
    }
    else if (!_wcsicmp(name, __TEXT("screen-saver")))
        *desktopID = DESKTOP_SCREENSAVER;
    else if (!_wcsicmp(name, __TEXT("Display.Cpl Desktop")))
        *desktopID = DESKTOP_TESTDISPLAY;
    else
        *desktopID = DESKTOP_OTHER;
    if ( CloseDesktop(GetThreadDesktop(GetCurrentThreadId())) == 0)
    {
        TCHAR str[10];
        DWORD err = GetLastError();
        wsprintf((LPTSTR)str, (LPCTSTR)"%l", err);
    }

    if ( SetThreadDesktop(hdesk) == 0)
    {
        TCHAR str[10];
        DWORD err = GetLastError();
        wsprintf((LPTSTR)str, (LPCTSTR)"%l", err);
    }
    return TRUE;
}

// InitMyProcessDesktopAccess
// a-anilk: 1-12-98
static BOOL InitMyProcessDesktopAccess(VOID)
{
  origWinStation = GetProcessWindowStation();
  userWinStation = OpenWindowStation(__TEXT("WinSta0"), FALSE, MAXIMUM_ALLOWED);
  if (!userWinStation)
    return FALSE;
  SetProcessWindowStation(userWinStation);
  return TRUE;
}

// ExitMyProcessDesktopAccess
// a-anilk: 1-12-98
static VOID ExitMyProcessDesktopAccess(VOID)
{
  if (origWinStation)
    SetProcessWindowStation(origWinStation);
  if (userWinStation)
  {
    CloseWindowStation(userWinStation);
    userWinStation = NULL;
  }
}

// a-anilk added
// Returns the current desktop-ID
DWORD GetDesktop()
{
    HDESK hdesk;
    TCHAR name[300];
    DWORD value, nl, desktopID = DESKTOP_ACCESSDENIED;

	hdesk = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
    if (!hdesk)
    {
        // OpenInputDesktop will mostly fail on "Winlogon" desktop
        hdesk = OpenDesktop(__TEXT("Winlogon"),0,FALSE,MAXIMUM_ALLOWED);
        if (!hdesk)
            return DESKTOP_WINLOGON;
    }
    
	GetUserObjectInformation(hdesk, UOI_NAME, name, 300, &nl);
    
	if (!_wcsicmp(name, __TEXT("Default")))
        desktopID = DESKTOP_DEFAULT;

    else if (!_wcsicmp(name, __TEXT("Winlogon")))
        desktopID = DESKTOP_WINLOGON;

    else if (!_wcsicmp(name, __TEXT("screen-saver")))
        desktopID = DESKTOP_SCREENSAVER;

    else if (!_wcsicmp(name, __TEXT("Display.Cpl Desktop")))
        desktopID = DESKTOP_TESTDISPLAY;

    else
        desktopID = DESKTOP_OTHER;
    
	return desktopID;
}

// Moves the dialog outside of the OSK screen area, Either on top if
// space permits or on the bottom edge of OSK: 
void RelocateDialog(HWND hDlg)
{
   RECT rKbMainRect, rDialogRect, Rect;
   int x, y, width, height;
   
   
   GetWindowRect(kbmainhwnd, &rKbMainRect);
   GetWindowRect(hDlg, &rDialogRect);
   
   width = rDialogRect.right - rDialogRect.left;
   height = rDialogRect.bottom - rDialogRect.top;
   
   GetWindowRect(GetDesktopWindow(),&Rect);
   if ((rKbMainRect.top - height) > Rect.top)
   {
      // There is enough space over OSK window, place the dialog on the top of the osk window
      y = rKbMainRect.top - height;
      x = rKbMainRect.left + (rKbMainRect.right - rKbMainRect.left)/2 - \
         (rDialogRect.right - rDialogRect.left)/2 ;
   }
   else if ((rKbMainRect.bottom + height) < Rect.bottom)
   {
      // There is enough space under OSK window, place the dialog on the bottom of the osk window
      y = rKbMainRect.bottom;
      x = rKbMainRect.left + (rKbMainRect.right - rKbMainRect.left)/2 - \
         (rDialogRect.right - rDialogRect.left)/2 ;
   }
   else
   {
      // It is not possible to see the entire dialog, don´t move it.
      return;
   }
   
   MoveWindow(hDlg, x, y, width, height, 1);
}
