/****************************************************************************
   Switch Input Library DLL - Parallel port routines

   Copyright (c) 1992-1997 Bloorview MacMillan Centre


*******************************************************************************/

#include <windows.h>
#include <tchar.h>
#include <conio.h>
#include <stdio.h>
#include <winioctl.h>

#include <msswch.h>

#include "msswchh.h"
#include "ntddpar.h"

#include "dbg.h"
#include "dbgerr.h"

// Internal functions
HANDLE XswcLptOpen( DWORD uiPort );
BOOL XswcLptSet(
	HANDLE hCom,
	PSWITCHCONFIG_LPT pC );

HANDLE swcLptOpen_Win( DWORD uiPort );
BOOL swcLptEnd_Win( HANDLE hsd );
DWORD swcLptStatus_Win( HANDLE hsd );
BOOL swcLptSet_Win(
	HANDLE hLpt,
	PSWITCHCONFIG_LPT pC );

HANDLE swcLptOpen_NT( DWORD uiPort );
BOOL swcLptEnd_NT( HANDLE hsd );
DWORD swcLptStatus_NT( HANDLE hsd );
BOOL swcLptSet_NT(
	HANDLE hLpt,
	PSWITCHCONFIG_LPT pC );

#pragma data_seg("sh_data")		/***** start of shared data segment *****/
// all shared variables must be initialized or they will not be shared

OSVERSIONINFO g_osv = {0};

WORD		gwPrtStatus=0;					// Printer status byte
WORD		gwCtrStatus=0;					// Printer control byte
WORD		gwData=0;						// Current data byte

// bios_data_area is filled in wivswch.c
// it should be done here, but is easier there because of the startup 
// logic and DDE connection between wivswchx (32 bit) and wivswchy (16 bit).
BYTE		bios_data_area[16] = {0};

#define MAX_LPT	3

SWITCHCONFIG_LPT	scDefaultLpt = 
	{SC_LPT_DEFAULT, SC_LPTDATA_DEFAULT};

SWITCHCONFIG	scLpt[MAX_LPT] =
	{
		{sizeof(SWITCHCONFIG_LPT), SC_TYPE_LPT, 1, SC_FLAG_DEFAULT, 0, 0},
		{sizeof(SWITCHCONFIG_LPT), SC_TYPE_LPT, 2, SC_FLAG_DEFAULT, 0, 0},
		{sizeof(SWITCHCONFIG_LPT), SC_TYPE_LPT, 3, SC_FLAG_DEFAULT, 0, 0}
	};

#pragma data_seg()	 /***** end of shared data segment *****/

// Handles cannot be shared across processes.
// For NT, these are port/file handles, for '95 these are port addresses
HANDLE hLpt[MAX_LPT] = {0,0,0};


// Printer Ports
#define PRT_DATA		0x00
#define PRT_STAT		0x01
#define PRT_CTRL		0x02

// Printer Status Port
//			- inputs are straight in, switch pulls pin low = bit low
//			ERR, SEL, ACK are active low
//			PE is active high
//			NB is inverted state of BUSY line, BUSY is active high
//				- therefore, NB is sort of active low

#define PRT_TO		0x01
#define PRT_Resv1	0x02
#define PRT_IRQS	0x04	// IRQ pending 
#define PRT_ERR	0x08	// pin 15 - MSI switch 3 - PRC sw2/5
#define PRT_SEL	0x10	// pin 13 - MSI switch 1 - PRC sw1/4
#define PRT_PE		0x20	// pin 12 - MSI switch 2 
#define PRT_ACK	0x40	// pin 10 - switch 4 -		PRC sw3/6
#define PRT_NB		0x80	// pin 11 - switch 5 	*inverted*

// Printer Control Port

#define PRT_STRB	0x01	// pin 1
#define PRT_AUTO	0x02	// pin 14 *inverted*
#define PRT_INIT	0x04	// pin 16
#define PRT_SELI	0x08	// pin 17 *inverted*
#define PRT_IRQ	0x10	// IRQ enable - is this writable?

#define PRT_CTRL_IO 0x20	// input enable for parallel port - not possible

/****************************************************************************

   FUNCTION: XswcLptInit()

	DESCRIPTION:
		Initialize the particular hardware device structures and variables.
		Any global initialization of resources will have to be done based
		on some version of a reference counter

****************************************************************************/

BOOL XswcLptInit( HSWITCHDEVICE hsd )
	{
	UINT uiDeviceNumber  = swcListGetDeviceNumber( NULL, hsd );		

	g_osv.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
	GetVersionEx( &g_osv );
	DBGMSG( TEXT("swch> GetVersionEx: %d.%d.%d - %d\n"),
		g_osv.dwMajorVersion,
		g_osv.dwMinorVersion,
	 g_osv.dwBuildNumber,
		g_osv.dwPlatformId );

   scLpt[uiDeviceNumber-1].u.Lpt = scDefaultLpt;
	hLpt[uiDeviceNumber-1] = (HANDLE) 0;
	
	return TRUE;
	}


/****************************************************************************

   FUNCTION: XswcLptEnd

	DESCRIPTION:
		Free the resources for the given hardware port.
		We assume that if CloseHandle fails, the handle is
		invalid and/or already closed, so we zero it out anyways,
		and return TRUE for success.
		Global releases will need to be based on a reference counter.

****************************************************************************/

BOOL XswcLptEnd( HANDLE hsd )
	{
	BOOL bSuccess;

	if (VER_PLATFORM_WIN32_WINDOWS == g_osv.dwPlatformId) // Windows 95
		{
		bSuccess = swcLptEnd_Win( hsd );
		}
	else	// Windows NT
		{	
		bSuccess = swcLptEnd_NT( hsd );
		}

	return bSuccess;
	}


/****************************************************************************

   FUNCTION: swcLptGetConfig()

	DESCRIPTION:

****************************************************************************/

BOOL swcLptGetConfig(
	HSWITCHDEVICE	hsd,
	PSWITCHCONFIG	psc )
	{
	UINT uiDeviceNumber  = swcListGetDeviceNumber( NULL, hsd );
	
	*psc = scLpt[uiDeviceNumber-1];
 	return TRUE;
	}


/****************************************************************************

   FUNCTION: XswcLptSetConfig

	DESCRIPTION:
		Activate/Deactivate the device.
		
		Four cases: 
		1) hLpt = 0 and active = 0		- do nothing
		2)	hLpt = x and active = 1		- just set the configuration
		3) hLpt = 0 and active = 1		- activate and set the configuration
		4) hLpt = x and active = 0		- deactivate

		If there are no errors, TRUE is returned and ListSetConfig
		will write the configuration to the registry.
		If there is any error, FALSE is returned so the registry
		entry remains unchanged.

		Plug and Play can check the registry for SC_FLAG_ACTIVE and
		start up the device if it is set.

****************************************************************************/

BOOL XswcLptSetConfig(
	HSWITCHDEVICE	hsd,
	PSWITCHCONFIG	psc )
	{
	BOOL		bSuccess;
	BOOL		bJustOpened;
	UINT		uiDeviceNumber;
	HANDLE	*phLpt;
	PSWITCHCONFIG pscLpt;

	bSuccess = FALSE;
	bJustOpened = FALSE;

	// Simplify our code
	uiDeviceNumber  = swcListGetDeviceNumber( NULL, hsd );
	phLpt = &hLpt[uiDeviceNumber-1];
	pscLpt = &scLpt[uiDeviceNumber-1];
	
	// Should we activate?
	if (	(0==*phLpt)
		&&	(psc->dwFlags & SC_FLAG_ACTIVE)
		)
		{ // Yes
		*phLpt = XswcLptOpen( uiDeviceNumber );
		if (*phLpt)
			{ //OK
			DBGMSG( TEXT("lpt> open succeeded %x\n"), *phLpt );
			bSuccess = TRUE;
			bJustOpened = TRUE;
			pscLpt->dwFlags |= SC_FLAG_ACTIVE;
			pscLpt->dwFlags &= ~SC_FLAG_UNAVAILABLE;
			}
		else
			{ // Not OK
			DBGMSG( TEXT("lpt> open failed %x\n"), *phLpt );
			bSuccess = FALSE;
			pscLpt->dwFlags &= ~SC_FLAG_ACTIVE;
			pscLpt->dwFlags |= SC_FLAG_UNAVAILABLE;
			}
		}

	// Should we deactivate?
	else if (	(0!=*phLpt)
		&&	!(psc->dwFlags & SC_FLAG_ACTIVE)
		)
		{
		XswcLptEnd( hsd ); // This will also zero out *phLpt
		bSuccess = TRUE;
		pscLpt->dwFlags &= ~SC_FLAG_ACTIVE;
		}
	
	// If the above steps leave a valid hLpt, let's try setting the config
	if ( 0!=*phLpt )
		{
		if (psc->dwFlags & SC_FLAG_DEFAULT)
			{
			bSuccess = XswcLptSet( *phLpt, &scDefaultLpt );
			if (bSuccess)
				{
				pscLpt->dwFlags |= SC_FLAG_DEFAULT;
            pscLpt->u.Lpt = scDefaultLpt;
				}
			}
		else
			{
			bSuccess = XswcLptSet( *phLpt, &(psc->u.Lpt) );
			if (bSuccess)
				{
            pscLpt->u.Lpt = psc->u.Lpt;
				}
			}
		// If we can't set config and we just opened the port, better close it up.
		if (bJustOpened && !bSuccess)
			{
			XswcLptEnd( *phLpt );
			pscLpt->dwFlags &= ~SC_FLAG_ACTIVE;
			}
		}

	return bSuccess;
	}


/****************************************************************************

   FUNCTION: XswcLptPollStatus

	DESCRIPTION:
		Must be called in the context of the helper window.
****************************************************************************/

DWORD XswcLptPollStatus( HSWITCHDEVICE	hsd )
	{
	DWORD		dwNewStatus;
	UINT		uiDeviceNumber;

	uiDeviceNumber  = swcListGetDeviceNumber( NULL, hsd );

	//DBGMSG( TEXT("swclpt> poll 1") );
	if (VER_PLATFORM_WIN32_WINDOWS == g_osv.dwPlatformId)
		{
		//DBGMSG( TEXT("...Win95\n") );
		dwNewStatus = swcLptStatus_Win( hsd );
		}
	else
		{
		//DBGMSG( TEXT("...NT\n ") );
		dwNewStatus = swcLptStatus_NT( hsd );
		}

	scLpt[uiDeviceNumber-1].dwSwitches = dwNewStatus;
	return dwNewStatus;
	}


/****************************************************************************

   FUNCTION: XswcLptOpen()

	DESCRIPTION:
		Opens a file handle to the particular lpt port, based on
		the 1-based nPort.
		
		If nPort is valid, this will automatically set up GetLastError().
****************************************************************************/

HANDLE XswcLptOpen( DWORD uiPort )
	{
	HANDLE hLpt;

	hLpt = 0;
	if (VER_PLATFORM_WIN32_WINDOWS == g_osv.dwPlatformId) // Windows 95
		{
		hLpt = swcLptOpen_Win( uiPort );
		}
	else	// Windows NT
		{
		hLpt = swcLptOpen_NT( uiPort );
		}

	return hLpt;
	}


/****************************************************************************

   FUNCTION: XswcLptSet()

	DESCRIPTION:
		Sets the configuration of the particular nPort.
		Return FALSE (0) if an error occurs.
		GetLastError is automatically set up for us.
		
****************************************************************************/

BOOL XswcLptSet(
	HANDLE hLpt,
	PSWITCHCONFIG_LPT pC )
	{
	BOOL bSuccess;;
	if (VER_PLATFORM_WIN32_WINDOWS == g_osv.dwPlatformId) // Windows 95
		{
		bSuccess = swcLptSet_Win( hLpt, pC );
		}
	else	// Windows NT
		{
		bSuccess = swcLptSet_NT( hLpt, pC );
		}
	return bSuccess;
	}


/***** Internal functions for Windows95 *****/

/****************************************************************************

   FUNCTION: swcLptOpen_Win()

	DESCRIPTION:
		Gets the base wInPort address for the given device, and
		increments by one to get the address for the "status in" port.
		
		This is cast into a HANDLE value for convenience.

		bios_data_area has 7 entries of 2 bytes each.
		The first 4 are the com ports.
		The next 3 are the lpt ports.
****************************************************************************/

HANDLE swcLptOpen_Win( DWORD uiPort )
	{
	WORD wInPort = 0;

	switch (uiPort)
		{
		case 1:
		 	wInPort = (bios_data_area[0x09] << 8)
				| (bios_data_area[0x08] & 0x00FF);
			break;

		case 2:
		 	wInPort = (bios_data_area[0x0B] << 8)
				| (bios_data_area[0x0A] & 0x00FF);
			break;

		case 3:
		 	wInPort = (bios_data_area[0x0D] << 8)
				| (bios_data_area[0x0C] & 0x00FF);
			break;

		default:
			wInPort = 0;
			break;
		}

	if (wInPort)
		{
		wInPort += 1;	// Status In is base + 1;
		}
	else
		{
		//DBGMSG( TEXT("swch lpt: no wInport\n") );
		}
	return (HANDLE) wInPort;
	}


/****************************************************************************

   FUNCTION: swcLptEnd_Win()

	DESCRIPTION:
		Close the given lpt port.
		For Windows95, there is nothing to close, so just zero the port "handle"
		which is actually the wInPort address.
****************************************************************************/

BOOL swcLptEnd_Win( HANDLE hsd )
	{	
	BOOL bSuccess = TRUE;
	UINT uiDeviceNumber  = swcListGetDeviceNumber( NULL, hsd );

	if (hLpt[uiDeviceNumber-1])
		{
		hLpt[uiDeviceNumber-1] = 0;
		}
	scLpt[uiDeviceNumber-1].dwSwitches = 0;

	// ignore bSuccess since we can't do anything anyways.
	return TRUE;
	}


/****************************************************************************

   FUNCTION: swcLptStatus_Win()

	DESCRIPTION:

****************************************************************************/

DWORD swcLptStatus_Win( HSWITCHDEVICE hsd )
	{
	UINT		uiDeviceNumber;
	DWORD		dwStatus;
	DWORD		dwLptStatus;
	WORD		wInPort;

	uiDeviceNumber  = swcListGetDeviceNumber( NULL, hsd );
	dwStatus = 0;

    // on a risc platform _inp() will not be defined, but since we should
    // not enter this code anyways, just #ifdef it so it will compile on
    // risc.

#if defined(_M_IX86) || defined(_X86_)

	wInPort = (WORD) hLpt[uiDeviceNumber-1];
	if (wInPort)
		{
		dwLptStatus = _inp( wInPort );	

		gwData = _inp( (USHORT)(wInPort-1) );	// save data for dbg
		gwPrtStatus = (WORD)dwLptStatus;
		gwCtrStatus = _inp( (USHORT)(wInPort+1) );

		dwStatus |= (dwLptStatus & PRT_SEL) ? 0 : SWITCH_1;
		dwStatus |= (dwLptStatus & PRT_PE ) ? 0 : SWITCH_2;
		dwStatus |= (dwLptStatus & PRT_ERR) ? 0 : SWITCH_3;
		dwStatus |= (dwLptStatus & PRT_ACK) ? 0 : SWITCH_4;
		dwStatus |= (dwLptStatus & PRT_NB ) ? SWITCH_5 : 0;
		}
#endif

	return dwStatus;
	}


/****************************************************************************

   FUNCTION: swcLptSet_Win()

	DESCRIPTION:

  The original intent was to activate the data lines to use as pullups,
  and whatever control lines were requested.
  Since we have so little control over this on the NT side, we are leaving
  this for future improvement if there is demand for it.

****************************************************************************/

BOOL swcLptSet_Win(
	HANDLE hLpt,
	PSWITCHCONFIG_LPT pC )
	{
	//WORD wInPort;
	BOOL bSuccess = TRUE;

	//	wInPort = (WORD) hLpt;
	//	_outp( (USHORT)(wInPort - 1), 0xFF );				// activate pull ups

	return bSuccess;
	}


/*****	Internal functions for WindowsNT *****/

/****************************************************************************

   FUNCTION: swcLptOpen_NT()

	DESCRIPTION:
	Open a file handle to the port.

	CreateFile will return no error for non-existant, valid ports,
	so we need to check the return from a DeviceIoControl to see if the device
	is available.

 ****************************************************************************/

HANDLE swcLptOpen_NT( DWORD uiPort )
	{
	BOOL		bSuccess;
	TCHAR		szLptPort[40];
	PAR_QUERY_INFORMATION ParQueryInfo;
	DWORD		dwBytesRet;
	HANDLE		hLpt;

	wsprintf( szLptPort, _TEXT("\\\\.\\lpt%1.1d"), uiPort );
	
	// Use overlapped i/o in order for the Write operation to be asynchronous?
	hLpt = CreateFile( 
		szLptPort, 
		GENERIC_WRITE,
		0, NULL,
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL //| FILE_FLAG_OVERLAPPED,
		,NULL );

	if (INVALID_HANDLE_VALUE != hLpt)
		{
		bSuccess = DeviceIoControl(
				hLpt, 
				IOCTL_PAR_QUERY_INFORMATION, 
				NULL, 0, 
				&ParQueryInfo, sizeof(PAR_QUERY_INFORMATION),  
				&dwBytesRet, NULL);

		if (!bSuccess)
			{
			CloseHandle( hLpt );
			hLpt = 0;
			}
		}
	else
		{
		DBGERR( TEXT("lpt> CreateFile Failed: "), TRUE );
		hLpt = 0;
		}

	return hLpt;
	}


/****************************************************************************

   FUNCTION: swcLptEnd_NT()

	DESCRIPTION:
		Close the given lpt port.
****************************************************************************/

BOOL swcLptEnd_NT( HANDLE hsd )
	{
	BOOL bSuccess = TRUE;
	UINT uiDeviceNumber  = swcListGetDeviceNumber( NULL, hsd );

	if (hLpt[uiDeviceNumber-1])
		{
		bSuccess = CloseHandle( hLpt[uiDeviceNumber-1] );
		DBGERR( TEXT("XswcLptEnd_NT"), !bSuccess );
		hLpt[uiDeviceNumber-1] = 0;
		}
	scLpt[uiDeviceNumber-1].dwSwitches = 0;

	// ignore bSuccess since we can't do anything anyways.
	return TRUE;
	}


/******************************************************************************
	FUNCTION: swcLptStatus_NT()

   DESCRIPTION:

  Under construction.

  For the "standard" passive box, we can read information on four of 
  the five status lines, although two of them look the same.

  To read these more explicitly we will need to write a parallel port
  "class driver".
******************************************************************************/

DWORD swcLptStatus_NT( HSWITCHDEVICE hsd )
	{
	PAR_QUERY_INFORMATION Pqi;
	DWORD		dwNewStatus;
	BOOL		bResult;
	DWORD		dwBytesRet;
	UINT		uiDeviceNumber;
	HANDLE		*phLpt;

	uiDeviceNumber  = swcListGetDeviceNumber( NULL, hsd );
	phLpt = &hLpt[uiDeviceNumber-1];

	if (*phLpt)
		{
		bResult = DeviceIoControl( *phLpt, 
						  IOCTL_PAR_QUERY_INFORMATION, 
						  NULL, 0, 
						  &Pqi, sizeof(PAR_QUERY_INFORMATION),  
						  &dwBytesRet, NULL);
		DBGERR( TEXT("swclpt1> DevIoControl"), !bResult );

		gwPrtStatus = Pqi.Status;
		dwNewStatus = 0;

		dwNewStatus |= (Pqi.Status & PARALLEL_SELECTED) ? 0 : SWITCH_1;

		// When nothing is pulled low, NT says the power is off
		// AND sets PAPER_EMPTY to zero (active).
		// So we need to check for PAPER_EMPTY active and no power off
		if (    !(Pqi.Status & PARALLEL_PAPER_EMPTY)
			  && !(Pqi.Status & PARALLEL_POWER_OFF )
			)
			dwNewStatus |= SWITCH_2;

		dwNewStatus |= (Pqi.Status & PARALLEL_BUSY ) ? 0 : SWITCH_3;

		// These don't work because they cannot be disambiguated
		//dwNewStatus |= (Pqi.Status & PRT_ERR) ? 0 : 0;//SWITCH_3;
		//dwNewStatus |= (Pqi.Status & PRT_ACK) ? 0 : 0;//SWITCH_4;
		//dwNewStatus |= (Pqi.Status & PARALLEL_BUSY ) ? 0 : SWITCH_5;
		}

	return dwNewStatus;
	}


/* This code is unused. It is here as sample code.
void set( HSWITCHDEVICE hsd )
	{
	PAR_SET_INFORMATION Psi;
	DWORD		dwBytesRet;
	UINT		uiDeviceNumber;
	HANDLE		*phLpt;
	BOOL		bResult;
	
	uiDeviceNumber  = swcListGetDeviceNumber( hsd );
	phLpt = &hLpt[uiDeviceNumber-1];

	// Set SelectIn high and AF high

	//Psi.Init = PARALLEL_INIT;
	//Psi.Init = PARALLEL_AUTOFEED; 
	//Psi.Init = PARALLEL_OFF_LINE; 
	//Psi.Init = PARALLEL_NOT_CONNECTED;
	//Psi.Init = Pqi.Status;
	// For output does this set SelectIn high or just the SLCT input?
	//Psi.Init = Psi.Init & ~PARALLEL_SELECTED;
	//Psi.Init = Psi.Init & ~PARALLEL_AUTOFEED;
	bResult = DeviceIoControl( *phLpt, 
               IOCTL_PAR_SET_INFORMATION, 
               &Psi, sizeof(PAR_SET_INFORMATION),  
               NULL, 0, 
               &dwBytesRet, NULL);
	DBGERR( TEXT("swclpt> DevIoControl-set1"), !bResult );
	}
*/
	
/****************************************************************************

	FUNCTION: swcLptSet_NT()

	DESCRIPTION:

	The intent is to write data to the port to use the data lines as
	pullups.

	We cannot write to the port if there is no printer attached, since
	the NT driver times out waiting for a valid printer status.
	We only have access to the INIT and AF lines for setting the 
	control bits.

	So for now, ignore setup. We will have to depend on the "floating" 
	property of TTL outputs to keep the status lines up.

    In the future, we may try to use a parallel port "class driver" to
	make this work.

****************************************************************************/

BOOL swcLptSet_NT(
	HANDLE hLpt,
	PSWITCHCONFIG_LPT pC )
	{
	//DWORD		dwBytesWritten;
	BOOL		bSuccess = TRUE;

	/*
	if (!WriteFile( hLpt, &PullUpBuff, 2, &dwBytesWritten,
		//&Overlapped
		NULL
		))
		{
		DBGERR( TEXT("lpt> WriteFile Error"), TRUE );
		}
	else
		{
		DBGMSG( TEXT("lpt> WriteFile Success bytes:%d\n"), dwBytesWritten );
		}
	*/
	return bSuccess;
	}


/****************************************************************************

   FUNCTION: swcLptDbgGetPrtStatus()

	DESCRIPTION:
		Returns a bitfield representing the current status of the 
		printer status port.

****************************************************************************/

WORD APIENTRY swcLptDbgGetPrtStatus( void )
	{
	return gwPrtStatus;
	}


/****************************************************************************

   FUNCTION: swcLptDbgGetCtrStatus()

	DESCRIPTION:
		Returns a bitfield representing the current status of the
		printer control port.

****************************************************************************/

WORD APIENTRY swcLptDbgGetCtrStatus( void )
	{
	return gwCtrStatus;
	}


/****************************************************************************

   FUNCTION: swcLptDbgGetData()

	DESCRIPTION:
		Data byte on printer port

****************************************************************************/

WORD APIENTRY swcLptDbgGetData( void )
	{
	return gwData;
	}





