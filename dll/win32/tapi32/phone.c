/*
 * TAPI32 phone services
 *
 * Copyright 1999  Andreas Mohr
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "tapi.h"
#include "wine/debug.h"

/*
 * Additional TSPI functions:
 * - voiceGetHandles
 * - TSPI_ProviderInit
 * - TSPI_ProviderShutdown
 * - TSPI_ProviderEnumDevices
 * - TSPI_ProviderConfig
*/
WINE_DEFAULT_DEBUG_CHANNEL(tapi);

/***********************************************************************
 *		phoneClose (TAPI32.@)
 */
DWORD WINAPI phoneClose(HPHONE hPhone)
{
    FIXME("(%p), stub.\n", hPhone);
    /* call TSPI function here ! */
    return 0;
}

/***********************************************************************
 *		phoneConfigDialog (TAPI32.@)
 */
DWORD WINAPI phoneConfigDialogA(DWORD dwDeviceID, HWND hwndOwner, LPCSTR lpszDeviceClass)
{
    FIXME("(%08lx, %p, %s): stub.\n", dwDeviceID, hwndOwner, lpszDeviceClass);
    return 0;
}

/***********************************************************************
 *		phoneDevSpecific (TAPI32.@)
 */
DWORD WINAPI phoneDevSpecific(HPHONE hPhone, LPVOID lpParams, DWORD dwSize)
{
    FIXME("(%p, %p, %ld): stub.\n", hPhone, lpParams, dwSize);
    return 1;
}

/***********************************************************************
 *		phoneGetButtonInfo (TAPI32.@)
 */
DWORD WINAPI phoneGetButtonInfoA(HPHONE hPhone, DWORD dwButtonLampID,
                                LPPHONEBUTTONINFO lpButtonInfo)
{
    FIXME("(%p, %08lx, %p): stub.\n", hPhone, dwButtonLampID, lpButtonInfo);
    return 0;
}

/***********************************************************************
 *		phoneGetData (TAPI32.@)
 */
DWORD WINAPI phoneGetData(HPHONE hPhone, DWORD dwDataID, LPVOID lpData, DWORD dwSize)
{
    FIXME("(%p, %08lx, %p, %ld): stub.\n", hPhone, dwDataID, lpData, dwSize);
    return 0;
}

/***********************************************************************
 *		phoneGetDevCaps (TAPI32.@)
 */
DWORD WINAPI phoneGetDevCapsA(HPHONEAPP hPhoneApp, DWORD dwDeviceID,
               DWORD dwAPIVersion, DWORD dwExtVersion, LPPHONECAPS lpPhoneCaps)
{
    FIXME("(%p, %08lx, %08lx, %08lx, %p): stub.\n", hPhoneApp, dwDeviceID, dwAPIVersion, dwExtVersion, lpPhoneCaps);
    /* call TSPI function here ! */
    return 0;
}

/***********************************************************************
 *		phoneGetDisplay (TAPI32.@)
 */
DWORD WINAPI phoneGetDisplay(HPHONE hPhone, LPVARSTRING lpDisplay)
{
    FIXME("(%p, %p): stub.\n", hPhone, lpDisplay);
    return 0;
}

/***********************************************************************
 *		phoneGetGain (TAPI32.@)
 */
DWORD WINAPI phoneGetGain(HPHONE hPhone, DWORD dwHookSwitchDev, LPDWORD lpdwGain)
{
    FIXME("(%p, %08lx, %p): stub.\n", hPhone, dwHookSwitchDev, lpdwGain);
    /* call TSPI function here ! */
    return 0;
}

/***********************************************************************
 *		phoneGetHookSwitch (TAPI32.@)
 */
DWORD WINAPI phoneGetHookSwitch(HPHONE hPhone, LPDWORD lpdwHookSwitchDevs)
{
   FIXME("(%p, %p): stub.\n", hPhone, lpdwHookSwitchDevs);
    /* call TSPI function here ! */
   return 0;
}

/***********************************************************************
 *		phoneGetID (TAPI32.@)
 */
DWORD WINAPI phoneGetIDA(HPHONE hPhone, LPVARSTRING lpDeviceID,
                        LPCSTR lpszDeviceClass)
{
    FIXME("(%p, %p, %s): stub.\n", hPhone, lpDeviceID, lpszDeviceClass);
    /* call TSPI function here ! */
    return 0;
}

/***********************************************************************
 *		phoneGetIcon (TAPI32.@)
 */
DWORD WINAPI phoneGetIconA(DWORD dwDeviceID, LPCSTR lpszDeviceClass,
		          HICON *lphIcon)
{
    FIXME("(%08lx, %s, %p): stub.\n", dwDeviceID, lpszDeviceClass, lphIcon);
    /* call TSPI function here ! */
    return 0;
}

/***********************************************************************
 *		phoneGetLamp (TAPI32.@)
 */
DWORD WINAPI phoneGetLamp(HPHONE hPhone, DWORD dwButtonLampID,
		          LPDWORD lpdwLampMode)
{
    FIXME("(%p, %08lx, %p): stub.\n", hPhone, dwButtonLampID, lpdwLampMode);
    return 0;
}

/***********************************************************************
 *              phoneGetMessage (TAPI32.@)
 */
DWORD WINAPI phoneGetMessage(HPHONEAPP hPhoneApp, LPPHONEMESSAGE lpMessage, DWORD dwTimeout)
{
    FIXME("(%p, %p, %08lx): stub.\n", hPhoneApp, lpMessage, dwTimeout);
    return 0;
}

/***********************************************************************
 *		phoneGetRing (TAPI32.@)
 */
DWORD WINAPI phoneGetRing(HPHONE hPhone, LPDWORD lpdwRingMode, LPDWORD lpdwVolume)
{
    FIXME("(%p, %p, %p): stub.\n", hPhone, lpdwRingMode, lpdwVolume);
    return 0;
}

/***********************************************************************
 *		phoneGetStatus (TAPI32.@)
 */
DWORD WINAPI phoneGetStatusA(HPHONE hPhone, LPPHONESTATUS lpPhoneStatus)
{
    FIXME("(%p, %p): stub.\n", hPhone, lpPhoneStatus);
    /* call TSPI function here ! */
    return 0;
}

/***********************************************************************
 *		phoneGetStatusMessages (TAPI32.@)
 */
DWORD WINAPI phoneGetStatusMessages(HPHONE hPhone, LPDWORD lpdwPhoneStates,
		          LPDWORD lpdwButtonModes, LPDWORD lpdwButtonStates)
{
    FIXME("(%p, %p, %p, %p): stub.\n", hPhone, lpdwPhoneStates, lpdwButtonModes, lpdwButtonStates);
    return 0;
}

/***********************************************************************
 *		phoneGetVolume (TAPI32.@)
 */
DWORD WINAPI phoneGetVolume(HPHONE hPhone, DWORD dwHookSwitchDevs,
		            LPDWORD lpdwVolume)
{
    FIXME("(%p, %08lx, %p): stub.\n", hPhone, dwHookSwitchDevs, lpdwVolume);
    /* call TSPI function here ! */
    return 0;
}

/***********************************************************************
 *		phoneInitialize (TAPI32.@)
 */
DWORD WINAPI phoneInitialize(LPHPHONEAPP lphPhoneApp, HINSTANCE hInstance, PHONECALLBACK lpfnCallback, LPCSTR lpszAppName, LPDWORD lpdwNumDevs)
{
    FIXME("(%p, %p, %p, %s, %p): stub.\n", lphPhoneApp, hInstance, lpfnCallback, lpszAppName, lpdwNumDevs);
    return 0;
}

/***********************************************************************
 *              phoneInitializeiExA (TAPI32.@)
 */
DWORD WINAPI phoneInitializeExA(LPHPHONEAPP lphPhoneApp, HINSTANCE hInstance, PHONECALLBACK lpfnCallback, LPCSTR lpszAppName, LPDWORD lpdwNumDevs, LPDWORD lpdwAPIVersion, LPPHONEINITIALIZEEXPARAMS lpPhoneInitializeExParams)
{
    FIXME("(%p, %p, %p, %s, %p, %p, %p): stub.\n", lphPhoneApp, hInstance, lpfnCallback, lpszAppName, lpdwNumDevs, lpdwAPIVersion, lpPhoneInitializeExParams);
    *lpdwNumDevs = 0;
    return 0;
}

/***********************************************************************
 *              phoneInitializeiExW (TAPI32.@)
 */
DWORD WINAPI phoneInitializeExW(LPHPHONEAPP lphPhoneApp, HINSTANCE hInstance, PHONECALLBACK lpfnCallback, LPCWSTR lpszAppName, LPDWORD lpdwNumDevs, LPDWORD lpdwAPIVersion, LPPHONEINITIALIZEEXPARAMS lpPhoneInitializeExParams)
{
    FIXME("(%p, %p, %p, %s, %p, %p, %p): stub.\n", lphPhoneApp, hInstance, lpfnCallback, debugstr_w(lpszAppName), lpdwNumDevs, lpdwAPIVersion, lpPhoneInitializeExParams);
    *lpdwNumDevs = 0;
    return 0;
}

/***********************************************************************
 *		phoneNegotiateAPIVersion (TAPI32.@)
 */
DWORD WINAPI phoneNegotiateAPIVersion(HPHONEAPP hPhoneApp, DWORD dwDeviceID, DWORD dwAPILowVersion, DWORD dwAPIHighVersion, LPDWORD lpdwAPIVersion, LPPHONEEXTENSIONID lpExtensionID)
{
    FIXME("(): stub.\n");
    return 0;
}

/***********************************************************************
 *		phoneNegotiateExtVersion (TAPI32.@)
 */
DWORD WINAPI phoneNegotiateExtVersion(HPHONEAPP hPhoneApp, DWORD dwDeviceID,
		                 DWORD dwAPIVersion, DWORD dwExtLowVersion,
				 DWORD dwExtHighVersion, LPDWORD lpdwExtVersion)
{
    FIXME("(): stub.\n");
    /* call TSPI function here ! */
    return 0;
}

/***********************************************************************
 *		phoneOpen (TAPI32.@)
 */
DWORD WINAPI phoneOpen(HPHONEAPP hPhoneApp, DWORD dwDeviceID, LPHPHONE lphPhone, DWORD dwAPIVersion, DWORD dwExtVersion, DWORD dwCallbackInstance, DWORD dwPrivileges)
{
    FIXME("(): stub.\n");
    /* call TSPI function here ! */
    return 0;
}

/***********************************************************************
 *		phoneSetButtonInfo (TAPI32.@)
 */
DWORD WINAPI phoneSetButtonInfoA(HPHONE hPhone, DWORD dwButtonLampID, LPPHONEBUTTONINFO lpButtonInfo)
{
    FIXME("(%p, %08lx, %p): stub.\n", hPhone, dwButtonLampID, lpButtonInfo);
    return 0;
}

/***********************************************************************
 *		phoneSetData (TAPI32.@)
 */
DWORD WINAPI phoneSetData(HPHONE hPhone, DWORD dwDataID, LPVOID lpData, DWORD dwSize)
{
    FIXME("(%p, %08lx, %p, %ld): stub.\n", hPhone, dwDataID, lpData, dwSize);
    return 1;
}

/***********************************************************************
 *		phoneSetDisplay (TAPI32.@)
 */
DWORD WINAPI phoneSetDisplay(HPHONE hPhone, DWORD dwRow, DWORD dwColumn, LPCSTR lpszDisplay, DWORD dwSize)
{
    FIXME("(%p, '%s' at %ld/%ld, len %ld): stub.\n", hPhone, lpszDisplay, dwRow, dwColumn, dwSize);
    return 1;
}

/***********************************************************************
 *		phoneSetGain (TAPI32.@)
 */
DWORD WINAPI phoneSetGain(HPHONE hPhone, DWORD dwHookSwitchDev, DWORD dwGain)
{
    FIXME("(%p, %08lx, %ld): stub.\n", hPhone, dwHookSwitchDev, dwGain);
    /* call TSPI function here ! */
    return 1;
}

/***********************************************************************
 *		phoneSetHookSwitch (TAPI32.@)
 */
DWORD WINAPI phoneSetHookSwitch(HPHONE hPhone, DWORD dwHookSwitchDevs, DWORD dwHookSwitchMode)
{
    FIXME("(%p, %08lx, %08lx): stub.\n", hPhone, dwHookSwitchDevs, dwHookSwitchMode);
    /* call TSPI function here ! */
    return 1;
}

/***********************************************************************
 *		phoneSetLamp (TAPI32.@)
 */
DWORD WINAPI phoneSetLamp(HPHONE hPhone, DWORD dwButtonLampID, DWORD lpdwLampMode)
{
    FIXME("(%p, %08lx, %08lx): stub.\n", hPhone, dwButtonLampID, lpdwLampMode);
    return 1;
}

/***********************************************************************
 *		phoneSetRing (TAPI32.@)
 */
DWORD WINAPI phoneSetRing(HPHONE hPhone, DWORD dwRingMode, DWORD dwVolume)
{
    FIXME("(%p, %08lx, %08lx): stub.\n", hPhone, dwRingMode, dwVolume);
    return 1;
}

/***********************************************************************
 *		phoneSetStatusMessages (TAPI32.@)
 */
DWORD WINAPI phoneSetStatusMessages(HPHONE hPhone, DWORD dwPhoneStates, DWORD dwButtonModes, DWORD dwButtonStates)
{
    FIXME("(%p, %08lx, %08lx, %08lx): stub.\n", hPhone, dwPhoneStates, dwButtonModes, dwButtonStates);
    /* call TSPI function here ! */
    return 0; /* FIXME ? */
}

/***********************************************************************
 *		phoneSetVolume (TAPI32.@)
 */
DWORD WINAPI phoneSetVolume(HPHONE hPhone, DWORD dwHookSwitchDev, DWORD dwVolume)
{
    FIXME("(%p, %08lx, %08lx): stub.\n", hPhone, dwHookSwitchDev, dwVolume);
    /* call TSPI function here ! */
    return 1;
}

/***********************************************************************
 *		phoneShutdown (TAPI32.@)
 */
DWORD WINAPI phoneShutdown(HPHONEAPP hPhoneApp)
{
    FIXME("(%p): stub.\n", hPhoneApp);
    return 0;
}
