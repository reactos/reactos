/****************************************************************************
   Switch Input Library DLL - Joystick routines

   Copyright (c) 1992-1997 Bloorview MacMillan Centre

	Link with winmm.lib
*******************************************************************************/

#include <windows.h>
#include <mmsystem.h>

#include <assert.h>

#include <msswch.h>

#include "msswchh.h"
#include "dbg.h"

HJOYDEVICE XswcJoyOpen( DWORD uiPort );
BOOL XswcJoySet(
	HJOYDEVICE hJoy,
	PSWITCHCONFIG_JOYSTICK pJ );

#pragma data_seg("sh_data")		/***** start of shared data segment *****/
// all shared variables must be initialized or they will not be shared

#define MAX_JOYSTICKS	2

SWITCHCONFIG_JOYSTICK   scDefaultJoy = 
	{
   SC_JOY_DEFAULT,
   SC_JOYVALUE_DEFAULT,
   SC_JOYVALUE_DEFAULT,
   SC_JOYVALUE_DEFAULT,
   SC_JOYVALUE_DEFAULT,
   SC_JOYVALUE_DEFAULT
	};

SWITCHCONFIG	gscJoy[MAX_JOYSTICKS] =
	{
		{sizeof(SWITCHCONFIG), SC_TYPE_JOYSTICK, 1, SC_FLAG_DEFAULT, 0, 0},
      {sizeof(SWITCHCONFIG), SC_TYPE_JOYSTICK, 2, SC_FLAG_DEFAULT, 0, 0}
	};

typedef struct _JOYSETTINGS
	{
	DWORD		XMaxOn;
	DWORD		XMaxOff;
	DWORD		XMinOn;
	DWORD		XMinOff;
	DWORD		YMaxOn;
	DWORD		YMaxOff;
	DWORD		YMinOn;
	DWORD		YMinOff;
	} JOYSETTINGS;

JOYSETTINGS	gJoySet[MAX_JOYSTICKS] = {0};

#pragma data_seg()	 /***** end of shared data segment *****/

// Handles cannot be shared across processes
// These are faked handles, to keep the module logic similar to the serial port.
HJOYDEVICE ghJoy[MAX_JOYSTICKS] = {0,0};


/****************************************************************************

   FUNCTION: XswcJoyInit()

	DESCRIPTION:

****************************************************************************/

BOOL XswcJoyInit( HSWITCHDEVICE hsd )
	{
    UINT uiDeviceNumber  = swcListGetDeviceNumber( NULL, hsd );

    gscJoy[uiDeviceNumber-1].u.Joystick = scDefaultJoy;
	ghJoy[uiDeviceNumber-1] = 0;
	return TRUE;
	}


/****************************************************************************

   FUNCTION: XswcJoyEnd()

	DESCRIPTION:

****************************************************************************/

BOOL XswcJoyEnd( HSWITCHDEVICE hsd )
	{
	BOOL bSuccess = TRUE;
	UINT uiDeviceNumber  = swcListGetDeviceNumber( NULL, hsd );

	ghJoy[uiDeviceNumber-1] = 0;
	gscJoy[uiDeviceNumber-1].dwSwitches = 0;

	// ignore bSuccess since we can't do anything anyways.
	return TRUE;
	}


/****************************************************************************

   FUNCTION: swcJoyGetConfig()

	DESCRIPTION:

****************************************************************************/

BOOL swcJoyGetConfig(
	HSWITCHDEVICE	hsd,
	PSWITCHCONFIG	psc )
	{
	UINT uiDeviceNumber  = swcListGetDeviceNumber( NULL, hsd );
	*psc = gscJoy[uiDeviceNumber-1];
	return TRUE;
	}


/****************************************************************************

   FUNCTION: XswcJoySetConfig()

	DESCRIPTION:
		Activate/Deactivate the device.
		
		Four cases: 
		1) hJoy = 0 and active = 0		- do nothing
		2)	hJoy = x and active = 1		- just set the configuration
		3) hJoy = 0 and active = 1		- activate and set the configuration
		4) hJoy = x and active = 0		- deactivate

		If there are no errors, TRUE is returned and ListSetConfig
		will write the configuration to the registry.
		If there is any error, FALSE is returned so the registry
		entry remains unchanged.

		Plug and Play can check the registry for SC_FLAG_ACTIVE and
		start up the device if it is set. This all probably needs some work.

****************************************************************************/

BOOL XswcJoySetConfig(
	HSWITCHDEVICE	hsd,
	PSWITCHCONFIG	psc )
	{
	BOOL		bSuccess;
	BOOL		bJustOpened;
	UINT		uiDeviceNumber;
    HJOYDEVICE  *pghJoy;
	PSWITCHCONFIG pgscJoy;

	bSuccess = FALSE;
	bJustOpened = FALSE;

	// Simplify our code
	uiDeviceNumber  = swcListGetDeviceNumber( NULL, hsd );
	pghJoy = &ghJoy[uiDeviceNumber-1];
	pgscJoy = &gscJoy[uiDeviceNumber-1];
	
	// Should we activate?
	if (	(0==*pghJoy)
		&&	(psc->dwFlags & SC_FLAG_ACTIVE)
		)
		{ // Yes
		*pghJoy = XswcJoyOpen( uiDeviceNumber );
		if (*pghJoy)
			{ //OK
			bSuccess = TRUE;
			bJustOpened = TRUE;
			pgscJoy->dwFlags |= SC_FLAG_ACTIVE;
			pgscJoy->dwFlags &= ~SC_FLAG_UNAVAILABLE;
			}
		else
			{ // Not OK
			bSuccess = FALSE;
			pgscJoy->dwFlags &= ~SC_FLAG_ACTIVE;
			pgscJoy->dwFlags |= SC_FLAG_UNAVAILABLE;
			}
		}

	// Should we deactivate?
	else if (	(0!=*pghJoy)
		&&	!(psc->dwFlags & SC_FLAG_ACTIVE)
		)
		{
		XswcJoyEnd( hsd ); // This will also zero out *pghJoy
		bSuccess = TRUE;
		pgscJoy->dwFlags &= ~SC_FLAG_ACTIVE;
		}
	
	// If the above steps leave a valid hJoy, let's try setting the config
	if ( 0!=*pghJoy )
		{
		if (psc->dwFlags & SC_FLAG_DEFAULT)
			{
			bSuccess = XswcJoySet( *pghJoy, &scDefaultJoy );
			if (bSuccess)
				{
				pgscJoy->dwFlags |= SC_FLAG_DEFAULT;
				pgscJoy->u.Joystick = scDefaultJoy;
				}
			}
		else
			{
			bSuccess = XswcJoySet( *pghJoy, &(psc->u.Joystick) );
			if (bSuccess)
				{
            pgscJoy->u.Joystick = psc->u.Joystick;
				}
			}

		// If we can't set config and we just opened the port, better close it up.
		if (bJustOpened && !bSuccess)
			{
			XswcJoyEnd( hsd );
			pgscJoy->dwFlags &= ~SC_FLAG_ACTIVE;
			}
		}

	return bSuccess;
	}


/****************************************************************************

   FUNCTION: XswcJoyPollStatus()

	DESCRIPTION:
		Must be called in the context of the helper window.
		(Actually it's not strictly necessary for the joystick,
		but we say so in order to be consistent with the other ports.)
****************************************************************************/

DWORD XswcJoyPollStatus( HSWITCHDEVICE	hsd )
	{
	JOYINFOEX	joyinfoex;
	MMRESULT		mmr;
	DWORD			dwStatus;
	UINT			uiDeviceNumber;
	UINT			uiJoyID;

	joyinfoex.dwSize = sizeof( JOYINFOEX );
	uiDeviceNumber = swcListGetDeviceNumber( NULL, hsd );

	assert( JOYSTICKID1 == 0 );	// assume JOYSTICKIDx is zero based
	uiJoyID = uiDeviceNumber -1;

	if (SC_FLAG_ACTIVE & gscJoy[uiDeviceNumber-1].dwFlags)
		{
		switch (gscJoy[uiDeviceNumber-1].u.Joystick.dwJoySubType)
			{
			case SC_JOY_BUTTONS:
				{
				dwStatus = 0;
				joyinfoex.dwFlags = JOY_RETURNBUTTONS;
				mmr = joyGetPosEx( uiJoyID, &joyinfoex );

				if (JOYERR_NOERROR == mmr)
					{
					dwStatus |= (joyinfoex.dwButtons & JOY_BUTTON1) ? SWITCH_1 : 0;
					dwStatus |= (joyinfoex.dwButtons & JOY_BUTTON2) ? SWITCH_2 : 0;
					dwStatus |= (joyinfoex.dwButtons & JOY_BUTTON3) ? SWITCH_3 : 0;
					dwStatus |= (joyinfoex.dwButtons & JOY_BUTTON4) ? SWITCH_4 : 0;
					dwStatus |= (joyinfoex.dwButtons & JOY_BUTTON5) ? SWITCH_5 : 0;
					dwStatus |= (joyinfoex.dwButtons & JOY_BUTTON6) ? SWITCH_6 : 0;
					}
				}
				break;

			case SC_JOY_XYSWITCH:
				{
				dwStatus = 0;
				joyinfoex.dwFlags = JOY_RETURNBUTTONS | JOY_RETURNX | JOY_RETURNY;
				mmr = joyGetPosEx( uiJoyID, &joyinfoex );
				if (JOYERR_NOERROR == mmr)
					{
					dwStatus |= (joyinfoex.dwButtons & JOY_BUTTON1) ? SWITCH_1 : 0;
					dwStatus |= (joyinfoex.dwButtons & JOY_BUTTON2) ? SWITCH_2 : 0;
					dwStatus |= (joyinfoex.dwButtons & JOY_BUTTON3) ? SWITCH_3 : 0;
					dwStatus |= (joyinfoex.dwButtons & JOY_BUTTON4) ? SWITCH_4 : 0;
					// No hysteresis needed, since it should be a switch
					if (joyinfoex.dwXpos < gJoySet[uiJoyID].XMinOn)
						dwStatus |=  SWITCH_5;
					if (joyinfoex.dwYpos < gJoySet[uiJoyID].YMinOn)
						dwStatus |=  SWITCH_6;
					}
				}
				break;

			case SC_JOY_XYANALOG:
				{
				// Hysteresis is necessary because of the "noisiness" of the joystick
				dwStatus = 0;
				joyinfoex.dwFlags = JOY_RETURNBUTTONS | JOY_RETURNX | JOY_RETURNY;
				mmr = joyGetPosEx( uiJoyID, &joyinfoex );

				if (JOYERR_NOERROR == mmr)
					{
					// In order to deal with the hysteresis,
					// we must explicity turn on or off each switch bit.
					dwStatus = gscJoy[uiDeviceNumber-1].dwSwitches;

					DBGMSG(TEXT("Joy%d> "), uiJoyID);
					// left and right
					if (joyinfoex.dwXpos < gJoySet[uiJoyID].XMinOn)
						dwStatus |=  SWITCH_4;
					if (joyinfoex.dwXpos > gJoySet[uiJoyID].XMinOff)
						dwStatus &= ~SWITCH_4;
					DBGMSG( TEXT("sw3:%d (%d,%d)\n"), 
						joyinfoex.dwXpos,
						gJoySet[uiJoyID].XMinOn,
						gJoySet[uiJoyID].XMinOff );

					if (joyinfoex.dwXpos > gJoySet[uiJoyID].XMaxOn)
						dwStatus |=  SWITCH_1;
					if (joyinfoex.dwXpos < gJoySet[uiJoyID].XMaxOff)
						dwStatus &= ~SWITCH_1;

					// top and bottom
					if (joyinfoex.dwYpos < gJoySet[uiJoyID].YMinOn)
						dwStatus |=  SWITCH_1;
					if (joyinfoex.dwYpos > gJoySet[uiJoyID].YMinOff)
						dwStatus &= ~SWITCH_1;

					if (joyinfoex.dwYpos > gJoySet[uiJoyID].YMaxOn)
						dwStatus |=  SWITCH_3;
					if (joyinfoex.dwYpos < gJoySet[uiJoyID].YMaxOff)
						dwStatus &= ~SWITCH_3;

					// 2 buttons
					if (joyinfoex.dwButtons & JOY_BUTTON1)
						dwStatus |=  SWITCH_5;
					else
						dwStatus &= ~SWITCH_5;

					if (joyinfoex.dwButtons & JOY_BUTTON2)
						dwStatus |=  SWITCH_6;
					else
						dwStatus &= ~SWITCH_6;
					}
				}
				break;

			default:
				dwStatus = 0;
				break;
			}
		gscJoy[uiDeviceNumber-1].dwSwitches = dwStatus;
		}

	return dwStatus;
	}

/****************************************************************************

   FUNCTION: XswcJoyOpen()

	DESCRIPTION:
	 uiPort is 1 based.
	 Return a non-zero value if the port is useable.
	 The joystick driver doesn't have a port to open, so
	 we fake the handle by using the PortNumber.

****************************************************************************/

HJOYDEVICE XswcJoyOpen( DWORD uiPort )
	{
	JOYINFOEX   joyinfoex;
	MMRESULT    mmr;
	UINT        uiJoyID;
    HJOYDEVICE  hJoy;	//faked, for success it must be non-zero

	assert( JOYSTICKID1 == 0 );	// assume JOYSTICKIDx is zero based

	joyinfoex.dwSize = sizeof( JOYINFOEX );

	// To check if a joystick is attached, set RETURNX and RETURNY as well.
	// If no joystick is attached, we will be OK just calling RETURNBUTTONS,
	// but a user will not be able to use the Windows calibration in 
    // Control Panel.

	joyinfoex.dwFlags = JOY_RETURNBUTTONS;
	uiJoyID = uiPort - 1;

	mmr = joyGetPosEx( uiJoyID, &joyinfoex );

	if (JOYERR_NOERROR == mmr)
		{
		hJoy = (HJOYDEVICE)uiPort;
		}
	else
		{
		DBGMSG( TEXT("joyInit> mmr = %d\n"), mmr );
		hJoy = 0;
		}

	return hJoy;
	}


/****************************************************************************

   FUNCTION: XswcJoySet()

	DESCRIPTION:
		Sets the configuration of the particular Port.
		Remember that hJoy is actually the joystick port number.
		Return FALSE (0) if an error occurs.
		
****************************************************************************/

BOOL XswcJoySet(
	HJOYDEVICE hJoy,
	PSWITCHCONFIG_JOYSTICK pJ )
	{
	UINT uiJoyID = hJoy -1;
	BOOL bSuccess = TRUE;

	switch (pJ->dwJoySubType)
		{
		case SC_JOY_BUTTONS:
			bSuccess = TRUE;	//nothing to do
			break;

		case SC_JOY_XYSWITCH:
			// XY Switch only uses XMin and YMin
		case SC_JOY_XYANALOG:
			{
			DWORD	dwHy;
			DBGMSG( TEXT("JoySet> %d\n"), uiJoyID );
			// Set X values
			if (pJ->dwJoyThresholdMinX)
				gJoySet[uiJoyID].XMinOn = pJ->dwJoyThresholdMinX;
			else
				gJoySet[uiJoyID].XMinOn = 0x4000;
			gJoySet[uiJoyID].XMinOff = gJoySet[uiJoyID].XMinOn;
			if (pJ->dwJoyThresholdMaxX)
				gJoySet[uiJoyID].XMaxOn = pJ->dwJoyThresholdMaxX;
			else
				gJoySet[uiJoyID].XMaxOn = 0xC000;
			gJoySet[uiJoyID].XMaxOff = gJoySet[uiJoyID].XMaxOn;

			// Set Y values
			if (pJ->dwJoyThresholdMinY)
				gJoySet[uiJoyID].YMinOn = pJ->dwJoyThresholdMinY;
			else
				gJoySet[uiJoyID].YMinOn = 0x4000;
			gJoySet[uiJoyID].YMinOff = gJoySet[uiJoyID].YMinOn;
			if (pJ->dwJoyThresholdMaxY)
				gJoySet[uiJoyID].YMaxOn = pJ->dwJoyThresholdMaxY;
			else
				gJoySet[uiJoyID].YMaxOn = 0xC000;
			gJoySet[uiJoyID].YMaxOff = gJoySet[uiJoyID].YMaxOn;

			// Set hysteresis
			if (pJ->dwJoyHysteresis)
				dwHy = pJ->dwJoyHysteresis/2; // +/- half the value
			else
				dwHy = 0xFFFF/20;		// +/- 5%

			// Adjust for hysteresis
			DBGMSG( TEXT("Hy:%d  XMinOn:%d\n"), dwHy, gJoySet[uiJoyID].XMinOn );
			gJoySet[uiJoyID].XMinOn -= dwHy;
			gJoySet[uiJoyID].XMinOff += dwHy;
			gJoySet[uiJoyID].XMaxOn += dwHy;
			gJoySet[uiJoyID].XMaxOff -= dwHy;

			gJoySet[uiJoyID].YMinOn -= dwHy;
			gJoySet[uiJoyID].YMinOff += dwHy;
			gJoySet[uiJoyID].YMaxOn += dwHy;
			gJoySet[uiJoyID].YMaxOff -= dwHy;
			bSuccess = TRUE;
			break;
			}

		default:
			bSuccess = FALSE;
			break;
		}

	return bSuccess;
	}
