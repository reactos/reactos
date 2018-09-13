/****************************************************************************
   Hidden 32-bit window for
   Switch Input Library DLL

   Copyright (c) 1992-1997 Bloorview MacMillan Centre
   
   This application performs several helper tasks:

   1) It owns any global resources (hooks, hardware devices) that are
      opened on behalf of applications using switch input

   2) It catches timer messages to keep polling the hardware devices

   3) In Windows 95 it receives the 16-bit bios table address information
      from the 16-bit hidden application and forwards it into the 
      32-bit world of the Switch Input Library

   If the window is not hidden on startup, it is in debug mode.
****************************************************************************/

/**************************************************************** Headers */

#include <windows.h>
#include <tchar.h>
#include "dbg.h"

#include "resource.h"


// These defines could be in a common include file, but we'll just
// hardcode them here and in WIVSWCH for now.

#define	SW_X_POLLSWITCHES		WM_USER
BOOL (APIENTRY *lpfnXswchRegHelperWnd)( HWND hWnd, PBYTE bda );
void (APIENTRY *lpfnXswchPollSwitches)( HWND hWnd );
void (APIENTRY *lpfnXswchTimerProc)( HWND hWnd );
LRESULT (APIENTRY *lpfnXswchSetSwitchConfig)( WPARAM uParam, PCOPYDATASTRUCT pcd );
BOOL (APIENTRY *lpfnXswchEndAll)( void );

INT_PTR APIENTRY WndProc( HWND hWnd, UINT uMsg, WPARAM uParam, LPARAM lParam );

TCHAR	   szAppName[] = _TEXT("MSSWCHX");
HINSTANCE  ghInst      = NULL;

int	nShow;		// if Debug this will be SHOW
BYTE	bios_data_area[16];

void ErrMessage(LPTSTR szTitle, UINT uMsg, UINT uFlags);

/********************************************* Windows Callback Functions */

	/********************************************************\
	 Windows initialization
	\********************************************************/

int PASCAL WinMain(
	HINSTANCE	hInstance,
	HINSTANCE   hPrevInstance,
	LPSTR	    lpszCmdLine,
	int		    nCmdShow )
	{
	HWND		hWnd;
	MSG		msg;
	WNDCLASS	wndclass;
	LPTSTR	lptCmdLine;

	// Look for magic word, note that we have to skip over MSSWCHX.EXE
	lptCmdLine = GetCommandLine();
	if (
			( lptCmdLine[12] != (TCHAR)'S' )
		||	( lptCmdLine[13] != (TCHAR)'W' )
		||	( lptCmdLine[14] != (TCHAR)'C' )
		||	( lptCmdLine[15] != (TCHAR)'H' )
		)
		{
		ErrMessage( lptCmdLine, IDS_NOT_USER_PROG, MB_OK | MB_ICONHAND );
		return FALSE;
		}

	if(!hPrevInstance) 
		{
		wndclass.style		= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wndclass.lpfnWndProc	= WndProc;
		wndclass.cbClsExtra	= 0;
		wndclass.cbWndExtra	= 0;
		wndclass.hInstance	= hInstance;
		wndclass.hIcon		= NULL;
		wndclass.hCursor		= NULL;
		wndclass.hbrBackground	= GetStockObject(WHITE_BRUSH);
		wndclass.lpszMenuName	= NULL;
		wndclass.lpszClassName	= szAppName;

		if(!RegisterClass(&wndclass))
			return FALSE;
		}

    ghInst=hInstance;

	nShow = nCmdShow;
	hWnd = CreateWindow(szAppName, szAppName,
					WS_OVERLAPPEDWINDOW,
					0,0,10,10,
					NULL, NULL, hInstance, NULL);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	while(GetMessage(&msg, NULL, 0, 0))
		{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		}

	return (int)msg.wParam;
	}

	/********************************************************\
	 Main window procedure
	\********************************************************/

INT_PTR APIENTRY WndProc(
	HWND		hWnd,
	UINT		uMsg,
	WPARAM	uParam,
	LPARAM	lParam)
	{

	static BOOL		bUseDLL;			/* Are we hooked into the switch DLL? */
	static HANDLE	hLibrary;			/* switch message hook */

	switch(uMsg)
		{
		case WM_TIMER:
			(*lpfnXswchTimerProc)( hWnd );
			break;

		case SW_X_POLLSWITCHES:
			(*lpfnXswchPollSwitches)( hWnd );
			break;

		// port resources must be owned and polled by this
		// hidden window
		// wParam carries the hSwitchPort parameter to help the
		// library transfer errors across the process boundaries
		case WM_COPYDATA:
			return (*lpfnXswchSetSwitchConfig)( uParam, (PCOPYDATASTRUCT) lParam );
			break;
		
		case WM_CREATE:
			{
			// note in Win95 the error messages may not appear while msswchx is hidden
			OSVERSIONINFO	osv;

			osv.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
			GetVersionEx( &osv );

			SetErrorMode( 1 );	/* Bypass Windows error message */
			hLibrary = LoadLibrary( _TEXT("MSSWCH.DLL") );
			SetErrorMode( 0 );

			bUseDLL = FALSE;
			if (hLibrary)
				{
				if (!(lpfnXswchRegHelperWnd = (BOOL (APIENTRY *)( HWND hWnd, PBYTE pbda ))
					GetProcAddress( hLibrary, "XswchRegHelperWnd" )))
					ErrMessage(TEXT("XswchRegHelperWnd"), IDS_PROC_NOT_FOUND, 
                               0 );
				else if (!(lpfnXswchPollSwitches = (void (APIENTRY *)( HWND hWnd ))
    						GetProcAddress( hLibrary, "XswchPollSwitches" )))
        				ErrMessage(TEXT("XswchPollSwitches"), IDS_PROC_NOT_FOUND, 0 );
				else if (!(lpfnXswchTimerProc = (void (APIENTRY *)( HWND hWnd ))
						GetProcAddress( hLibrary, "XswchTimerProc" )))
					ErrMessage( TEXT("XswchTimerProc"), IDS_PROC_NOT_FOUND, 0 );
				else if (!(lpfnXswchSetSwitchConfig = (LRESULT (APIENTRY *)( WPARAM wParam, PCOPYDATASTRUCT pcd ))
						GetProcAddress( hLibrary, "XswchSetSwitchConfig" )))
					ErrMessage(TEXT("XswchSetSwitchConfig"), 
                               IDS_PROC_NOT_FOUND , 0 );
				else if (!(lpfnXswchEndAll = (BOOL (APIENTRY *)( void ))
						GetProcAddress( hLibrary, "XswchEndAll" )))
					ErrMessage(TEXT("XswchEndAll"), IDS_PROC_NOT_FOUND, 0 );
				else
					{
					bUseDLL = TRUE;
					if (VER_PLATFORM_WIN32_WINDOWS == osv.dwPlatformId)
						{
						// In Win95 Start up the 16-bit helper app to get the bios data area
                        // NOTENOTE: ANSI only call

						WinExec( "MSSWCHY.EXE SWCH", nShow );
						}
					else
						{// In WinNT we cannot access the bios data area or the ports
						//DBGMSG( "swchx> non-dde reg\n" );
						(*lpfnXswchRegHelperWnd)( hWnd, bios_data_area );
						SetTimer( hWnd, 1, 0, NULL );
						}
					}
				}
			else
				ErrMessage(NULL, IDS_MSSWCH_DLL_NOT_FOUND, 0 );
			}
			break;

		// Get the bios table date from the 16-bit SWCHY window
		case WM_DDE_DATA:
			{
			UINT_PTR Lo;
			UINT_PTR Hi;
			DDEDATA *pdata;

			UnpackDDElParam( WM_DDE_DATA, lParam, &Lo, &Hi ); 
			pdata = GlobalLock( (HANDLE)Lo );
			memcpy( bios_data_area, pdata->Value, 16 );
			GlobalUnlock( (HANDLE)Lo );
			// In non-debug, this will also cause the server to close
			PostMessage( (HWND)uParam, WM_DDE_ACK, 0, 0L );
			//DBGMSG( "swchx> dde reg\n" );
			(*lpfnXswchRegHelperWnd)( hWnd, bios_data_area );
			SetTimer( hWnd, 1, 0, NULL );
			}
			break;

		case WM_PAINT:	// should only happen in debug mode
			{
			PAINTSTRUCT ps;
			TCHAR	szOutBuff[100];
			int i;

			BeginPaint( hWnd, &ps );
			wsprintf( szOutBuff, TEXT("Bios Data Area") );
			TextOut( ps.hdc,0,0,szOutBuff,lstrlen(szOutBuff));
		
			for ( i=0;i<8;i++ )
				{ // note reversed pairs
				wsprintf( szOutBuff, TEXT("Port%d: %02X %02X"), i,
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

		/* Window has just been closed	*/
		case WM_DESTROY:

			PostQuitMessage(0);

		case WM_ENDSESSION:
			if (!uParam && (uMsg == WM_ENDSESSION))		/* Windows is not terminating */
				break;
			/* else continue */

			if (bUseDLL)
				{
				KillTimer( hWnd, 1 );
				(*lpfnXswchEndAll)( );
				FreeLibrary( hLibrary );
				}
			break;

		default:
			return DefWindowProc(hWnd, uMsg, uParam, lParam);
		}
	return 0L;
	}

void
ErrMessage(LPTSTR szTitle, UINT uMsg, UINT uFlags)
{
    TCHAR szMessage[256];
    TCHAR szTitle2[256];

    if (szTitle == NULL)
    {
        LoadString(ghInst,IDS_TITLE,szTitle2,256);
        szTitle=&szTitle2[0];
    }
    
    LoadString(ghInst,uMsg,szMessage,256);

    MessageBox(GetFocus(), szMessage, szTitle, uFlags);
}


