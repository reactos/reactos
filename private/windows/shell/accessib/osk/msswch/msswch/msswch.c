/****************************************************************************
   Switch Input Library DLL

   Copyright (c) 1992-1997 Bloorview MacMillan Centre

  MSSWCH.C - Global and external communication

		This module handles the list of calling windows, starts up the
		hidden helper window, and communicates with the helper window.

		The first user window that opens a port causes this DLL to start.
		The DLL executes the hidden helper window, which registers itself
		to the DLL through XswchRegHelpeWnd. When the last user window 
		closes a port, the  helper window is asked to shut down.
		The helper window calls down to to XswchEndAll to close down all
		global resources.

		For polling the switches, the helper window has started a timer, 
		which causes XswchTimerProc to be called continuously. This causes
		the switch devices to be polled, and swchPostSwitches to be
		called for each switch on each device that has changed status.

		When a user window tries to change the configuration, the request
		is posted up to the hidden window which calls down to
		XswchSetConfig on behalf of the user window in order to manipulate
		the global switch devices.

		Ideally all of this should be written as interrupt driven
		device drivers.

		IPC is handled in several ways:
		1) Communication between the 16-bit and 32-bit helper apps is via DDE
		2) The SetConfig call uses the WM_COPYDATA message to get the information
			transferred from the using app to the helper app.
		3) Currently all other information is in a global static, shared memory 
			area. Much of it could be in a memory-mapped file area which would allow
			more dynamic allocation of memory, but the static model is simpler to 
			implement for now, and provides faster performance with less overhead.
			When USB is added	it may be worthwhile moving to a memory-mapped file.

  18-Jul-97	GEH	First Microsoft version


*******************************************************************************/

#include <windows.h>
#include <assert.h>
#include <mmsystem.h>

#include <msswch.h>

#include "msswchh.h"
#include "msswcher.h"

#include "dbg.h"

// Helper Window / Timer related procs

// These defines could be in a common include file, but we'll just
// hardcode them here and in MSSWCHX for now.
#define	SW_X_POLLSWITCHES		WM_USER

BOOL swchStartHelperWnd( void );
BOOL APIENTRY XswchRegHelperWnd( HWND, PBYTE );
BOOL swchCloseHelperWnd( void );
BOOL APIENTRY XswchEndAll( void );

LRESULT APIENTRY XswchSetSwitchConfig(
	WPARAM				uParam,
	PCOPYDATASTRUCT	pCopyData );
void APIENTRY XswchPollSwitches( HWND );
void APIENTRY XswchTimerProc( HWND );

#pragma data_seg("sh_data")		/***** start of shared data segment *****/
// all shared variables must be initialized or they will not be shared

HWND		ghHelperWnd=0;			// The helper windows which owns shared resources
DWORD		gdwLastError=0;		// The last error caused within this library

/*
	Clock resolution is 55 ms but clock time is 54.9 ms.

	Boundary condition 55 ms:
		0-54 = 1 tick, 56-109 = 2 ticks, 55 = 1 or 2 ticks
	
	*** AVOID THE BOUNDARY CONDITION ***

	SetTimer() experience:
		0 -54 	steady 55ms ticks
		55			55 ms ticks, misses (every tenth?)
		56-109 	steady 110ms ticks
		110		110 ms ticks, misses some
		111-		steady 165 ms ticks

	54.9 ms intervals:

		 55, 110, 165, 220, 275, 330, 385, 440, 495, 549,
		604, 659, 714, 769, 824

	We check the time because WM_TIMER messages may be combined
*/


#define MAXWNDS	64
typedef struct _USEWNDLIST
	{
	HWND		hWnd;
	DWORD		dwPortStyle;
	DWORD		dwLastError;
	} USEWNDLIST, *PUSEWNDLIST;

USEWNDLIST	ghUseWndList[MAXWNDS+1] = {0};	// list of using apps
int			gnUseWndTotal = 0;
#define		SZMUTEXWNDLIST  _T("MutexWndList")

DWORD			gdwSwitchStatus=0;					// Bit field of status of switches
#define		SZMUTEXSWITCHSTATUS _T("MutexSwitchStatus")

/* Synchronize configuration
   XswcListInit, swcListGetList, swcListGetConfig, XswcListSetConfig
   These are protected at this level, before they are called,
   so that internal to swchlist.c they can call each other.
	Alternatively, swchlist.c could have two levels of the functions, 
	non-synched and synched.
*/
#define		SZMUTEXCONFIG _T("MutexConfig")

// global links to swchlpt.c
extern BYTE		bios_data_area[];

#pragma data_seg()	 /***** end of shared data segment *****/


/****************************************************************************

   FUNCTION: swchStartHelperWnd()

	DESCRIPTION:
		Called by the DLL to start the hidden application

****************************************************************************/

BOOL swchStartHelperWnd( void )
	{
	UINT uRtn;

	// WinExec returns after first GetMessage call
	uRtn = WinExec( "MSSWCHX.EXE SWCH",
	#ifdef _DEBUG
		SW_SHOW );
	#else
		SW_HIDE );
	#endif

	return TRUE;
	}


/****************************************************************************

   FUNCTION: XswchRegHelperWnd()

	DESCRIPTION:
		Called by the helper window (SWCHX) to register itself for
		shutdown later on. 

		In Windows 95 this also starts the 16-bit app to get the 
		bios_data_area information (16 bytes). Timer messages will not start
		until this information is passed down.

****************************************************************************/

BOOL APIENTRY XswchRegHelperWnd(
	HWND				hWndApp,
	PBYTE				pbda )
	{
	HANDLE			hMutex;

	ghHelperWnd = hWndApp;
	memcpy( bios_data_area, pbda, 16 );

	hMutex = CreateMutex( NULL, FALSE, SZMUTEXCONFIG );
	WaitForSingleObject( hMutex, INFINITE );

	XswcListInit();

	ReleaseMutex( hMutex );
	CloseHandle( hMutex );
	return TRUE;
	}


/****************************************************************************

   FUNCTION: swchCloseHelperWnd()

	DESCRIPTION:
				Called by the DLL to close the hidden timer window.
****************************************************************************/

BOOL swchCloseHelperWnd( void )
	{
	if (IsWindow( ghHelperWnd ))
		{
		PostMessage( ghHelperWnd, WM_CLOSE, 0, 0L );
		ghHelperWnd = NULL;
		}
	return TRUE;
	}


/****************************************************************************

   FUNCTION: XswchEndAll()

	DESCRIPTION:
		Called by the helper window.
		Frees all the switch resources

****************************************************************************/

BOOL APIENTRY XswchEndAll( void )
	{
	return XswcListEnd();
	}


/****************************************************************************

    FUNCTION: swchOpenSwitchPort()

    DESCRIPTION:
		All applications using the DLL whether in event or in polling
		mode must call swchOpenSwitchPort() with their window handle.
		On exit they must call swchCloseSwitchPort().

		Perform any initialization needed when someone loads the DLL.
		Also keeps track of how many windows are using this DLL.

		In the future this should be a dynamically allocated list, not limited
		to a compile time array.

		Also, we assume that window handles are unique and therefore simply
		turn around and use the passed in windows handle as the HSWITCHPORTs.
****************************************************************************/

HSWITCHPORT APIENTRY swchOpenSwitchPort(
	HWND		hWnd,
	DWORD		dwPortStyle )
	{
	HANDLE			hMutex;
	int				i;
	HSWITCHPORT		hSwitchPort = NULL;

	hMutex = CreateMutex( NULL, FALSE, SZMUTEXWNDLIST );
	WaitForSingleObject( hMutex, INFINITE );

	if (NULL == hWnd)
		{
		swchSetLastError( SWCHERROR_INVALID_HWND );
		goto Exit_OpenSwitchPort;
		}

	if (!IsWindow( hWnd ))
		{
		swchSetLastError( SWCHERROR_INVALID_HWND );
		goto Exit_OpenSwitchPort;
		}

	if (gnUseWndTotal >= MAXWNDS)
		{
		swchSetLastError( SWCHERROR_MAXIMUM_PORTS );
		goto Exit_OpenSwitchPort;
		}

	for (i=0; i<gnUseWndTotal; i++)
		{
		if (hWnd == ghUseWndList[i].hWnd)
			break;
		}
		
	// If found, do not reregister, say goodbye
	if (i != gnUseWndTotal)
		{
		swchSetLastError( SWCHERROR_HWND_ALREADY_USED );
		goto Exit_OpenSwitchPort;
		}

	ghUseWndList[gnUseWndTotal].hWnd = hWnd;
	ghUseWndList[gnUseWndTotal].dwPortStyle = dwPortStyle;
	gnUseWndTotal++;
	if (1 == gnUseWndTotal)	// first registered window
		{
		swchStartHelperWnd();
		}
	
	// Return a handle to the switch port
	// For now, this is the same as the passed-in window handle.
	hSwitchPort = (HSWITCHPORT) hWnd;
	SetLastError( SWCHERROR_SUCCESS );

Exit_OpenSwitchPort:
	ReleaseMutex( hMutex );
	CloseHandle( hMutex );

	return hSwitchPort;
	}


/****************************************************************************

    FUNCTION: swchCloseSwitchPort()

	 DESCRIPTION:

		Perform any deallocation/cleanup when the DLL is no longer needed.
		
		Note that the passed in HSWITCHPORT handle is simply a window handle,
		but the user doesn't know that.

****************************************************************************/

BOOL APIENTRY swchCloseSwitchPort(
	HSWITCHPORT hSwitchPort )
	{
	HWND		hWndRemove;
	HANDLE	hMutex;
	short		i,j;

	hWndRemove = (HWND) hSwitchPort;

	if (NULL == hWndRemove) return FALSE;

	hMutex = CreateMutex( NULL, FALSE, SZMUTEXWNDLIST );
	WaitForSingleObject( hMutex, INFINITE );
	for (i=0; i<gnUseWndTotal; i++)
		{
		if (hWndRemove == ghUseWndList[i].hWnd)
			break;
		}
		
	// If not found, goodbye
	if (i == gnUseWndTotal)
		{
		ReleaseMutex( hMutex );
		CloseHandle( hMutex );
		return FALSE;
		}

	for (j=i; j<gnUseWndTotal; j++)
		{
		ghUseWndList[j] = ghUseWndList[j+1];
		}

	gnUseWndTotal--;


	if (0 == gnUseWndTotal)
		{
		swchCloseHelperWnd();
		}

	ReleaseMutex( hMutex );
	CloseHandle( hMutex );

	return TRUE;
	}


/****************************************************************************

    FUNCTION: swchGetSwitchList()

	 DESCRIPTION:
		Returns the list of configurable switch devices.

****************************************************************************/

BOOL swchGetSwitchList(
	HSWITCHPORT		hSwitchPort,
	PSWITCHLIST		pSL,
	DWORD				dwSize,
	PDWORD			pdwReturnSize )
	{
	BOOL				bReturn;
	HANDLE			hMutex;

	hMutex = CreateMutex( NULL, FALSE, SZMUTEXCONFIG );
	WaitForSingleObject( hMutex, INFINITE );

	bReturn = swcListGetList( hSwitchPort, pSL, dwSize, pdwReturnSize );

	ReleaseMutex( hMutex );
	CloseHandle( hMutex );
	
	return bReturn;
	}


/****************************************************************************

    FUNCTION: swchGetSwitchDevice()

	 DESCRIPTION:
	   Return a handle to a switch device, given the PortType and PortNumber.
****************************************************************************/

HSWITCHDEVICE swchGetSwitchDevice(
	HSWITCHPORT		hSwitchPort,
	UINT				uiDeviceType,
	UINT				uiDeviceNumber	)
	{
	return swcListGetSwitchDevice( hSwitchPort, uiDeviceType, uiDeviceNumber );
	}


/****************************************************************************

    FUNCTION: swchGetDeviceType()

	 DESCRIPTION:
		Return the Device Type value given the handle to the switch device.
****************************************************************************/

UINT swchGetDeviceType(
	HSWITCHPORT		hSwitchPort,
	HSWITCHDEVICE	hsd )
	{
	return swcListGetDeviceType( hSwitchPort, hsd );
	}


/****************************************************************************

    FUNCTION: swchGetDeviceNumber()

	 DESCRIPTION:
		Return the Device Number value, given the handle to the switch device.
****************************************************************************/

UINT swchGetDeviceNumber(
	HSWITCHPORT		hSwitchPort,
	HSWITCHDEVICE	hsd	)
	{
	return swcListGetDeviceNumber( hSwitchPort, hsd );
	}


/****************************************************************************

    FUNCTION: swchGetSwitchConfig()

	 DESCRIPTION:
		Returns with the buffer filled in with the configuration information
		for the given switch device.
****************************************************************************/

BOOL swchGetSwitchConfig(
	HSWITCHPORT		hSwitchPort,
	HSWITCHDEVICE	hsd,
	PSWITCHCONFIG	psc )
	{
	BOOL				bReturn;
	HANDLE			hMutex;

	hMutex = CreateMutex( NULL, FALSE, SZMUTEXCONFIG );
	WaitForSingleObject( hMutex, INFINITE );

	bReturn = swcListGetConfig( hSwitchPort, hsd, psc );

	ReleaseMutex( hMutex );
	CloseHandle( hMutex );
	return bReturn;
	}


/****************************************************************************

    FUNCTION: swchSetSwitchConfig()

	 DESCRIPTION: Called by user applications that wish to change the configuration
		of a switch device.  Since all devices must be owned by the helper
		window, the parameters are copied to shared address space and a message
		posted to the helper window which will call down to XswchSetSwitchConfig.

	 Currently we send a WM_COPYDATA message for cheap IPC.
	 In the memory block we simply include the SWITCHCONFIG information.

	 Another way to do this would be to write to the registry here,
	 and have the XswchSetSwitchConfig read from the registry to make
	 or reject the change.

	 Problem: Non-reentrant critical section flags may need to be added,
	 perhaps at the XswcListSetConfig() level?
****************************************************************************/

BOOL swchSetSwitchConfig(
	HSWITCHPORT		hSwitchPort,
	HSWITCHDEVICE	hsd,
	PSWITCHCONFIG	psc )
	{
	DWORD		dwAllocSize;
	COPYDATASTRUCT CopyData;
	PBYTE		pData;
	LRESULT	lr;

	//DBGMSG( "Set> %X flags:%X\n", hsd, psc->dwFlags);

	if (!psc)
		{
		swchSetLastError( SWCHERROR_INVALID_PARAMETER );
		return FALSE;
		}

	dwAllocSize = sizeof(SWITCHCONFIG);
   if (!(psc->cbSize) || (dwAllocSize < psc->cbSize))
		{
      swchSetLastError( SWCHERROR_INSUFFICIENT_BUFFER );
		return FALSE;
		}

	// Sanity checks for developers and testers
	assert( sizeof(HSWITCHDEVICE) == sizeof(DWORD_PTR) );
	assert( sizeof(HSWITCHPORT) == sizeof(WPARAM) );

	pData = (PBYTE) LocalAlloc( LPTR, dwAllocSize );
	if (pData)
		{
		CopyData.dwData = (DWORD_PTR)hsd;
		CopyData.lpData = pData;
		CopyData.cbData = dwAllocSize;
		memcpy( pData, psc, sizeof(SWITCHCONFIG));
		lr = SendMessage( 
			ghHelperWnd, WM_COPYDATA, (WPARAM) hSwitchPort, (LPARAM) &CopyData );
		LocalFree( pData );
		return (BOOL)lr;
		}
	else
		{
		// LocalAlloc will set LastError;
		return FALSE;
		}
	}


/****************************************************************************

    FUNCTION: XswchSetSwitchConfig()

	 DESCRIPTION:
		Gets called by the helper window to set the configuration.
		Note: we are in SendMessage().

****************************************************************************/

LRESULT APIENTRY XswchSetSwitchConfig(
	WPARAM				uParam,
	PCOPYDATASTRUCT	pCopyData )
	{
	HSWITCHPORT			hSwitchPort;
	HSWITCHDEVICE		hsd;
	SWITCHCONFIG		scConfig;
	BOOL					bReturn;
	HANDLE				hMutex;

	//DBGMSG( "XSet CopyData> %X flags:%X\n",
			//pCopyData->dwData,
			//((PSWITCHCONFIG)(pCopyData->lpData))->dwFlags);
	hSwitchPort = (HSWITCHPORT) uParam;
	memcpy( &scConfig, pCopyData->lpData, sizeof(SWITCHCONFIG));
	hsd = (HSWITCHDEVICE) pCopyData->dwData;

	hMutex = CreateMutex( NULL, FALSE, SZMUTEXCONFIG );
	WaitForSingleObject( hMutex, INFINITE );

	bReturn = XswcListSetConfig( hSwitchPort, hsd, &scConfig );
		
	ReleaseMutex( hMutex );
	CloseHandle( hMutex );
	if (bReturn)
		return SWCHERR_NO_ERROR;
	else
		return SWCHERR_ERROR;
	}


/****************************************************************************

   FUNCTION: swchReadSwitches()

	DESCRIPTION:
		
		Returns a bitfield representing the current status of the switches.

		Currently we don't need the hSwitchPort, but for robustness in the future
		we may wish to check that it is a valid handle.

		If the user calls this from within their handling of a posted
		switch message, the mutex will politely ask them to wait until
		all messages have been posted.

****************************************************************************/

BOOL APIENTRY swchReadSwitches( 
	HSWITCHPORT	hSwitchPort,
	PDWORD		pdwSwitches )
	{
	HANDLE		hMutex;
	BOOL			bReturn;
	// Need to keep global switch status consistent with posts
	// But if something goes wrong don't hang around forever.
	hMutex = CreateMutex( NULL, FALSE, SZMUTEXSWITCHSTATUS );
	WaitForSingleObject( hMutex, 5000 );

	bReturn = (BOOL)SendMessage( ghHelperWnd, SW_X_POLLSWITCHES, 0, 0 );
	*pdwSwitches = gdwSwitchStatus;

	ReleaseMutex( hMutex );
	CloseHandle( hMutex );

	return TRUE;
	}


/****************************************************************************

    FUNCTION: XswchPollSwitches()

    DESCRIPTION:
		The helper windows calls this when swchReadSwitches is called.
		XswcListPollSwitches() ultimately calls swchPostSwitches() for each switch
		on each device that has changed state.
****************************************************************************/

void APIENTRY XswchPollSwitches( HWND hWnd )
	{
	// Mutex is in swchReadSwitches

	gdwSwitchStatus = XswcListPollSwitches();

	return;
	}
	
	
/****************************************************************************

    FUNCTION: XswchTimerProc()

    DESCRIPTION:
		Timer call-back function for the regularly scheduled timer.
		The helper app calls this proc everytime it receives a timer message.
		XswcListPollSwitches() ultimately calls swchPostSwitches() for each switch
		on each device that has changed state.

		Currently we use the timer whether or not any application has requested
		events. In the future we could check and see if any applications are
		requesting events and only then start up the timer for non-interrupt
		driven devices.
****************************************************************************/

void APIENTRY XswchTimerProc( HWND hWnd )
	{
	HANDLE		hMutex;
	// Need to keep global switch status consistent with posts
	// But if something goes wrong don't hang around forever.
	hMutex = CreateMutex( NULL, FALSE, SZMUTEXSWITCHSTATUS );
	WaitForSingleObject( hMutex, 5000 );	// 

	gdwSwitchStatus = XswcListPollSwitches();

	ReleaseMutex( hMutex );
	CloseHandle( hMutex );
	return;
	}


/****************************************************************************

   FUNCTION: swchPostSwitches()

	DESCRIPTION:
		Post the given switch up or down message to all applications
		which have requested posted messages.

		We would like to use timeGetTime() instead of GetTickCount, but
		it is unclear under what circumstances the multimedia timer is
		working at a more precise level. On my old PS/2 Model 95 with
		a 16-bit Microchannel Sound Blaster Pro, there does not seem
		to be a multimedia timer.

****************************************************************************/

BOOL swchPostSwitches(
	HSWITCHDEVICE hsd,
	DWORD		dwSwitch )
	{
	int		i;
	//DWORD		dwMsec = GetTickCount();
	DWORD	dwMsec = timeGetTime();

	assert( sizeof(WPARAM) >= sizeof(HSWITCHDEVICE) );
	for (i=0; i<gnUseWndTotal; i++)
		{
		if ( (PS_EVENTS == ghUseWndList[i].dwPortStyle)
			&& IsWindow( ghUseWndList[i].hWnd )
			)
			{
			PostMessage( ghUseWndList[i].hWnd, dwSwitch, (WPARAM)hsd, dwMsec );
			}
		}
	return TRUE;
	}


/****************************************************************************

   FUNCTION: swchPostConfigChanged()

	DESCRIPTION:
		Post the CONFIGCHANGED message to all apps which have registered
		with this DLL.
				
****************************************************************************/

BOOL swchPostConfigChanged( void )
	{
	int		i;

	for (i=0; i<gnUseWndTotal; i++)
		{
		if (IsWindow( ghUseWndList[i].hWnd ))
			{
			PostMessage( ghUseWndList[i].hWnd, SW_SWITCHCONFIGCHANGED, 0, 0 );
			}
		}
	return TRUE;
	}


/****************************************************************************

   FUNCTION: XswchStoreLastError()

	DESCRIPTION:

	Store the last error code for the process specific switch port,
	which is also the window handle at this point.
	If no switch port handle is passed, store the error code in
	a global variable.
	
	The Windows SetLastError call can be made
	just before the process specific call which caused the error
	returns to the application.
	
****************************************************************************/

void XswchStoreLastError(
	HSWITCHPORT		hSwitchPort,
	DWORD				dwError )
	{
	int i;

	if (NULL == hSwitchPort)
		{
		gdwLastError = dwError;
		}
	else
		{
		for (i=0; i<gnUseWndTotal; i++)
			{
			if (hSwitchPort == ghUseWndList[i].hWnd)
				break;
			}
			
		// If not found, goodbye
		if (i == gnUseWndTotal)
			{
			return;
			}
		else
			{
			ghUseWndList[i].dwLastError = dwError;
			}
		}
	return;
	}

/****************************************************************************

   FUNCTION: swchSetLastError()

	DESCRIPTION:
		Calls the Windows SetLastError API. It must be called in the
		context of the calling application.
		Currently nothing else is done, but this can also be used
		to set any special flags like facility and severity codes,
		without changing all the calls throughout MSSWCH.
				
****************************************************************************/

void swchSetLastError(
	DWORD			dwError )
	{
	SetLastError( dwError );
	return;
	}

