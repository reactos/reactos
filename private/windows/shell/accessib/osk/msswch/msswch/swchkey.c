/****************************************************************************
   Switch Input Library DLL - Keyboard hook routines

   Copyright (c) 1992-1997 Bloorview MacMillan Centre


*******************************************************************************/

#include <windows.h>

#include <msswch.h>

#include "msswchh.h"
#include "dbg.h"

// Hook proc
LRESULT CALLBACK swcKeyboardHookProc( int nCode, WPARAM wParam, LPARAM lParam );
HHOOK XswcKeyOpen( void );
BOOL XswcKeySet( PSWITCHCONFIG_KEYS pK );
BOOL swcKeyModKeysDown( UINT dwMod );

#pragma data_seg("sh_data")		/***** start of shared data segment *****/
// all shared variables must be initialized or they will not be shared

BOOL g_fNextKeyboardHook = TRUE;	//v-mjgran: Send, or not, the key to next hook

HHOOK    ghKey=0;

SWITCHCONFIG_KEYS scDefaultKeys = {0, 0};

// Note, there is only one port, therefore no need for port numbers.
SWITCHCONFIG scKeys = {sizeof(SWITCHCONFIG), SC_TYPE_KEYS, 1, 0, 0};

#define NUM_KEYS  2

typedef struct _HOTKEY
	{
	UINT mod;
	UINT vkey;
	UINT dwSwitch;
	} HOTKEY;

HOTKEY dwKey[NUM_KEYS] = 
	{
		{0,0, SWITCH_1},
		{0,0, SWITCH_2}
	};

#pragma data_seg()	 /***** end of shared data segment *****/



/****************************************************************************

   FUNCTION: swchAvoidScanChar (BOOL fSendScanCharacter)

	DESCRIPTION:
      Change the value of the g_fNextKeyboardHook flag
****************************************************************************/

void APIENTRY swchAvoidScanChar (BOOL fSendScanCharacter)
{
	g_fNextKeyboardHook = fSendScanCharacter;
}



/****************************************************************************

   FUNCTION: swcKeyboardHookProc()

	DESCRIPTION:
      When the hook is set set, this blocks the specified keys from
		being processed by anyone else.

		We could use this to set the switch status as well, but for 
		consistency we do it in the PollStatus routine.

      This must be released before unloading the DLL.

****************************************************************************/

LRESULT CALLBACK swcKeyboardHookProc(
   int nCode,
   WPARAM wParam,
   LPARAM lParam)
{
	int i;


   if (nCode < 0)
      return CallNextHookEx( ghKey, nCode, wParam, lParam );

	for (i=0; i<NUM_KEYS; i++ )
	{
		if (dwKey[i].vkey && (wParam == dwKey[i].vkey))
		{
			if (swcKeyModKeysDown(dwKey[i].mod) && !g_fNextKeyboardHook)
			{
				return 1;  // tell Windows to discard it
			}
		}
	}
	
	// if not blocked, pass it on.
   return CallNextHookEx( ghKey, nCode, wParam, lParam );
}



/****************************************************************************

   FUNCTION: XswcKeyInit()

	DESCRIPTION:

****************************************************************************/

BOOL XswcKeyInit( HSWITCHDEVICE hsd )
	{
	BOOL bSuccess = TRUE;

	scKeys.u.Keys = scDefaultKeys;

	return bSuccess;
	}


/****************************************************************************

   FUNCTION: XswcKeyEnd()

	DESCRIPTION:

****************************************************************************/

BOOL XswcKeyEnd( HSWITCHDEVICE hsd )
	{
   // clear the keyboard hook
	if (ghKey)
      {
      UnhookWindowsHookEx( ghKey );
      ghKey = 0;
      }
	scKeys.dwSwitches = 0;
	return TRUE;
	}


/****************************************************************************

   FUNCTION: swcKeyGetConfig()

	DESCRIPTION:

****************************************************************************/

BOOL swcKeyGetConfig(
	HSWITCHDEVICE	hsd,
	PSWITCHCONFIG	psc )
	{
	*psc = scKeys;
	return TRUE;
	}


/****************************************************************************

   FUNCTION: XswcKeySetConfig()

	DESCRIPTION:
		Activate/Deactivate the hook.
		
		Four cases: 
		1) ghKey = 0 and active = 0		- do nothing
		2)	ghKey = x and active = 1		- just set the configuration
		3) ghKey = 0 and active = 1		- activate and set the configuration
		4) ghKey = x and active = 0		- deactivate

		If there are no errors, TRUE is returned and ListSetConfig
		will write the configuration to the registry.
		If there is any error, FALSE is returned so the registry
		entry remains unchanged.

		Plug and Play can check the registry for SC_FLAG_ACTIVE and
		start up the device if it is set. This all probably needs some work.

****************************************************************************/

BOOL XswcKeySetConfig(
	HSWITCHDEVICE	hsd,
	PSWITCHCONFIG	psc )
	{	
	BOOL bSuccess = FALSE;
	BOOL bJustOpened = FALSE;

	// Should we activate?
	if (	(0==ghKey)
		&&	(psc->dwFlags & SC_FLAG_ACTIVE)
		)
		{ // Yes
		ghKey = XswcKeyOpen();
		if (ghKey)
			{ //OK
			bSuccess = TRUE;
			bJustOpened = TRUE;
			scKeys.dwFlags |= SC_FLAG_ACTIVE;
			scKeys.dwFlags &= ~SC_FLAG_UNAVAILABLE;
			}
		else
			{ // Not OK
			bSuccess = FALSE;
			scKeys.dwFlags &= ~SC_FLAG_ACTIVE;
			scKeys.dwFlags |= SC_FLAG_UNAVAILABLE;
			}
		}

	// Should we deactivate?
	else if (	(0!=ghKey)
		&&	!(psc->dwFlags & SC_FLAG_ACTIVE)
		)
		{
		XswcKeyEnd( hsd ); // This will also zero out ghKey
		bSuccess = TRUE;
		scKeys.dwFlags &= ~SC_FLAG_ACTIVE;
		}

	// If the above steps leave a valid ghKey, let's try setting the config
	// currently we don't do any error checking, so anything goes.
	if ( 0!=ghKey )
		{
		if (psc->dwFlags & SC_FLAG_DEFAULT)
			{
			bSuccess = XswcKeySet( &scDefaultKeys );
			if (bSuccess)
				{
				scKeys.dwFlags |= SC_FLAG_DEFAULT;
				scKeys.u.Keys = scDefaultKeys;
				}
			}
		else
			{
			bSuccess = XswcKeySet( &(psc->u.Keys) );
			if (bSuccess)
				{
				scKeys.u.Keys = psc->u.Keys;
				}
			}

		// If we can't set config and we just opened the port, better close it up.
		if (bJustOpened && !bSuccess)
			{
			XswcKeyEnd( hsd );
			scKeys.dwFlags &= ~SC_FLAG_ACTIVE;
			}
		}

	return bSuccess;
	}


/****************************************************************************

   FUNCTION: XswcKeyPollStatus()

	DESCRIPTION:

  Assumes that if there is no keyboard hook, then this "device" is not active.

****************************************************************************/

DWORD XswcKeyPollStatus( HSWITCHDEVICE	hsd )
	{
	int i;
	DWORD dwStatus;

	dwStatus = 0;
	if (ghKey)
		{
		for (i=0; i<NUM_KEYS; i++)
			{
			if (	(GetAsyncKeyState( dwKey[i].vkey ) & 0x8000)
				&& swcKeyModKeysDown( dwKey[i].mod )
				)
				dwStatus |= dwKey[i].dwSwitch;
			}
		}
	scKeys.dwSwitches = dwStatus;

	return dwStatus;
	}


/****************************************************************************

   FUNCTION: XswcKeyOpen()

	DESCRIPTION:
	Set the Windows keyboard hook.

****************************************************************************/

HHOOK XswcKeyOpen( void )
	{
	HHOOK hKey;

	hKey = SetWindowsHookEx(
      WH_KEYBOARD,
      (HOOKPROC) swcKeyboardHookProc,
      GetModuleHandle(SZ_DLLMODULENAME),
		0 );
	
	return hKey;
	}


/****************************************************************************

   FUNCTION: XswcKeySet()

	DESCRIPTION:

  Sets the configuration of the keys, storing the virtual key number and
  modifier states.

  In the future this routine can be used to limit the valid virtual keys.
		
****************************************************************************/

BOOL XswcKeySet( PSWITCHCONFIG_KEYS pK )
	{
	BOOL bSuccess;

	dwKey[0].mod = HIWORD( pK->dwKeySwitch1 );
	dwKey[0].vkey = LOWORD( pK->dwKeySwitch1 );
	dwKey[1].mod = HIWORD( pK->dwKeySwitch2 );
	dwKey[1].vkey = LOWORD( pK->dwKeySwitch2 );

	bSuccess = TRUE;
	return bSuccess;
	}


/****************************************************************************

   FUNCTION: swcKeyModKeysDown()

	DESCRIPTION:
	
	Are all the requested modifier keys down?
	If any requested key is not down, return FALSE.
	If any key is down but not requested, return FALSE.

****************************************************************************/

BOOL swcKeyModKeysDown( UINT dwMod )
	{
	DWORD		dwTest = 0;

	/*if (GetAsyncKeyState( VK_MENU ) & 0x8000)	
		dwTest |= MOD_ALT;
	if (GetAsyncKeyState( VK_CONTROL ) & 0x8000)
		dwTest |= MOD_CONTROL;
	if (GetAsyncKeyState( VK_SHIFT ) & 0x8000)
		dwTest |= MOD_SHIFT;

	if ((
			(GetAsyncKeyState( VK_LWIN ) & 0x8000)
		|| (GetAsyncKeyState( VK_RWIN ) & 0x8000)
		))
		dwTest |= MOD_WIN;*/

	if (dwMod ^ dwTest)
		return FALSE;
	else
		return TRUE;
	}
