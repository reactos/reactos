/****************************************************************************
   Hidden 16-bit window for
   Switch Input Library DLL

   Copyright (c) 1992-1997 Bloorview MacMillan Centre
   
   This mini-application reports the 16-bit bios table address information
   back to the 32-bit world of the Switch Input Library
****************************************************************************/

//#define DEBUGMSG
/**************************************************************** Headers */

#include <windows.h>
#include <dde.h>
#include <memory.h>

#define APIENTRY FAR PASCAL

long APIENTRY WndProc( HWND hWnd, UINT uMsg, WPARAM uParam, LPARAM lParam );
   
char  szAppName[]       = "MSSWCHY";

int   nShow;      // if Debug this will be SHOW
BYTE  bios_data_area[16];
HANDLE   hData;
DDEDATA FAR *pdata;

extern WORD _0040h;

#ifdef DEBUGMSG
char  szDbgMsg[80];
#endif

/********************************************* Windows Callback Functions */

   /********************************************************\
    Windows initialization
   \********************************************************/

int PASCAL WinMain(hInstance, hPrevInstance, lpszCmdLine, nCmdShow)
   HANDLE   hInstance, hPrevInstance;
   LPSTR    lpszCmdLine;
   int      nCmdShow;
   {
   HWND     hWnd;
   MSG      msg;
   WNDCLASS wndclass;


   // Look for magic word

   if (
         ( lpszCmdLine[0] != 'S' )
      || ( lpszCmdLine[1] != 'W' )
      || ( lpszCmdLine[2] != 'C' )
      || ( lpszCmdLine[3] != 'H' )
      )
      {
      MessageBox( GetFocus(), "This is not a user program", "MSSWCHY",
         MB_OK | MB_ICONHAND );
      return FALSE;
      }

   if(!hPrevInstance) 
      {
      wndclass.style    = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
      wndclass.lpfnWndProc = WndProc;
      wndclass.cbClsExtra  = 0;
      wndclass.cbWndExtra  = 0;
      wndclass.hInstance   = hInstance;
      wndclass.hIcon    = NULL;
      wndclass.hCursor     = NULL;
      wndclass.hbrBackground  = GetStockObject(WHITE_BRUSH);
      wndclass.lpszMenuName   = NULL;
      wndclass.lpszClassName  = szAppName;

      if(!RegisterClass(&wndclass))
         return FALSE;
      }

   hWnd = CreateWindow(szAppName, szAppName,
               WS_OVERLAPPEDWINDOW,
               0,0,10,10,
               NULL, NULL, hInstance, NULL);

   nShow = nCmdShow;
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   while(GetMessage(&msg, NULL, 0, 0))
      {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
      }

   return msg.wParam;
   }

   /********************************************************\
    Main window procedure
   \********************************************************/

long APIENTRY WndProc(HWND hWnd, UINT uMsg, WPARAM uParam, LPARAM lParam)
   {
   #ifdef xDEBUGMSG
   sprintf( szDbgMsg, "MP Msg: %X %X %lX\r\n", iMessage, wParam, lParam );
   OutputDebugString( szDbgMsg );
   #endif

   switch(uMsg)
      {
      case WM_CREATE:
         {
         HWND hSwchx;
         
         hSwchx = FindWindow( "MSSWCHX", NULL );
         _fmemcpy( bios_data_area,
          (LPSTR)(MAKELONG(0,&_0040h)),
           sizeof( bios_data_area ) );
         
         hData = GlobalAlloc( GMEM_DDESHARE, sizeof( DDEDATA ) + sizeof( bios_data_area ));
         if (hData)
            {
            pdata = (DDEDATA FAR *) GlobalLock( hData );
            pdata->fResponse =  TRUE;
            pdata->fRelease = FALSE;
            pdata->fAckReq = FALSE;
            pdata->cfFormat = CF_OWNERDISPLAY;
            _fmemcpy( pdata->Value, bios_data_area, sizeof( bios_data_area ) );
            GlobalUnlock( hData );
            
            PostMessage( hSwchx, WM_DDE_DATA, hWnd, MAKELONG( hData, 0 ));
            }
         }
         break;
         
      case WM_DDE_ACK:
         {
         if (hData)
            GlobalFree( hData );
         if (nShow != SW_SHOW)
            PostMessage( hWnd, WM_CLOSE, 0, 0L );
         }
            break;
            
      case WM_PAINT: // should only happen in debug mode
         {
         PAINTSTRUCT ps;
         char  szOutBuff[100];
         int i;

         BeginPaint( hWnd, &ps );
         i = 0;
         wsprintf( szOutBuff, "Bios Data Area" );
         TextOut( ps.hdc,0,10 * i,szOutBuff,lstrlen(szOutBuff));
      
         for ( i=0;i<8;i++ )
            {// note reversed pairs
            wsprintf( szOutBuff, "Port%d: %02X %02X", i,
             bios_data_area[2*i+1], bios_data_area[2*i] );
            TextOut( ps.hdc,0,30 + 30 * i,szOutBuff,lstrlen(szOutBuff));
            }
         EndPaint( hWnd, &ps );
         }
         break;
      
      case WM_CLOSE:
      case WM_QUERYENDSESSION:

         if (uMsg == WM_QUERYENDSESSION)
            return 1L;
         else
            DestroyWindow( hWnd );

         break;

      /* Window has just been closed   */
      case WM_DESTROY:

         PostQuitMessage(0);

      case WM_ENDSESSION:
         if (!uParam && (uMsg == WM_ENDSESSION))      /* Windows is not terminating */
            break;
         /* else continue */

         break;

      default:
         return DefWindowProc(hWnd, uMsg, uParam, lParam);
      }
   return 0L;
   }


