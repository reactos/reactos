/*
 * TAPI32 line services
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

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wingdi.h"
#include "winreg.h"
#include "winnls.h"
#include "winerror.h"
#include "objbase.h"
#include "tapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(tapi);

/* registry keys */
static const char szCountrylistKey[] =
    "Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Country List";
static const char szLocationsKey[] =
    "Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Locations";
static const char szCardsKey[] =
    "Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Cards";


/***********************************************************************
 *		lineAccept (TAPI32.@)
 */
DWORD WINAPI lineAccept(HCALL hCall, LPCSTR lpsUserUserInfo, DWORD dwSize)
{
    FIXME("(%p, %s, %ld): stub.\n", hCall, lpsUserUserInfo, dwSize);
    return 1;
}

/***********************************************************************
 *		lineAddProviderA (TAPI32.@)
 */
DWORD WINAPI lineAddProviderA(LPCSTR lpszProviderName, HWND hwndOwner, LPDWORD lpdwPermanentProviderID)
{
    FIXME("(%s, %p, %p): stub.\n", lpszProviderName, hwndOwner, lpdwPermanentProviderID);
    return LINEERR_OPERATIONFAILED;
}

/***********************************************************************
 *		lineAddProviderW (TAPI32.@)
 */
DWORD WINAPI lineAddProviderW(LPCWSTR lpszProviderName, HWND hwndOwner, LPDWORD lpdwPermanentProviderID)
{
    FIXME("(%s, %p, %p): stub.\n", wine_dbgstr_w(lpszProviderName), hwndOwner, lpdwPermanentProviderID);
    return LINEERR_OPERATIONFAILED;
}

/***********************************************************************
 *		lineAddToConference (TAPI32.@)
 */
DWORD WINAPI lineAddToConference(HCALL hConfCall, HCALL hConsultCall)
{
    FIXME("(%p, %p): stub.\n", hConfCall, hConsultCall);
    return 1;
}

/***********************************************************************
 *		lineAnswer (TAPI32.@)
 */
DWORD WINAPI lineAnswer(HCALL hCall, LPCSTR lpsUserUserInfo, DWORD dwSize)
{
    FIXME("(%p, %s, %ld): stub.\n", hCall, lpsUserUserInfo, dwSize);
    return 1;
}

/***********************************************************************
 *		lineBlindTransfer (TAPI32.@)
 */
DWORD WINAPI lineBlindTransferA(HCALL hCall, LPCSTR lpszDestAddress, DWORD dwCountryCode)
{
    FIXME("(%p, %s, %08lx): stub.\n", hCall, lpszDestAddress, dwCountryCode);
    return 1;
}

/***********************************************************************
 *		lineClose (TAPI32.@)
 */
DWORD WINAPI lineClose(HLINE hLine)
{
    FIXME("(%p): stub.\n", hLine);
    return 0;
}

/***********************************************************************
 *		lineCompleteCall (TAPI32.@)
 */
DWORD WINAPI lineCompleteCall(HCALL hCall, LPDWORD lpdwCompletionID, DWORD dwCompletionMode, DWORD dwMessageID)
{
    FIXME("(%p, %p, %08lx, %08lx): stub.\n", hCall, lpdwCompletionID, dwCompletionMode, dwMessageID);
    return 1;
}

/***********************************************************************
 *		lineCompleteTransfer (TAPI32.@)
 */
DWORD WINAPI lineCompleteTransfer(HCALL hCall, HCALL hConsultCall, LPHCALL lphConfCall, DWORD dwTransferMode)
{
    FIXME("(%p, %p, %p, %08lx): stub.\n", hCall, hConsultCall, lphConfCall, dwTransferMode);
    return 1;
}

/***********************************************************************
 *		lineConfigDialog (TAPI32.@)
 */
DWORD WINAPI lineConfigDialogA(DWORD dwDeviceID, HWND hwndOwner, LPCSTR lpszDeviceClass)
{
    FIXME("(%08lx, %p, %s): stub.\n", dwDeviceID, hwndOwner, lpszDeviceClass);
    return 0;
}

/***********************************************************************
 *		lineConfigDialogW (TAPI32.@)
 */
DWORD WINAPI lineConfigDialogW(DWORD dwDeviceID, HWND hwndOwner, LPCWSTR lpszDeviceClass)
{
    FIXME("(%08lx, %p, %s): stub.\n", dwDeviceID, hwndOwner, debugstr_w(lpszDeviceClass));
    return 0;
}

/***********************************************************************
 *		lineConfigDialogEdit (TAPI32.@)
 */
DWORD WINAPI lineConfigDialogEditA(DWORD dwDeviceID, HWND hwndOwner, LPCSTR lpszDeviceClass, LPVOID const lpDeviceConfigIn, DWORD dwSize, LPVARSTRING lpDeviceConfigOut)
{
    FIXME("stub.\n");
    return 0;
}

/***********************************************************************
 *		lineConfigProvider (TAPI32.@)
 */
DWORD WINAPI lineConfigProvider(HWND hwndOwner, DWORD dwPermanentProviderID)
{
    FIXME("(%p, %08lx): stub.\n", hwndOwner, dwPermanentProviderID);
    return 0;
}

/***********************************************************************
 *		lineDeallocateCall (TAPI32.@)
 */
DWORD WINAPI lineDeallocateCall(HCALL hCall)
{
    FIXME("(%p): stub.\n", hCall);
    return 0;
}

/***********************************************************************
 *		lineDevSpecific (TAPI32.@)
 */
DWORD WINAPI lineDevSpecific(HLINE hLine, DWORD dwAddressId, HCALL hCall, LPVOID lpParams, DWORD dwSize)
{
    FIXME("(%p, %08lx, %p, %p, %ld): stub.\n", hLine, dwAddressId, hCall, lpParams, dwSize);
    return 1;
}

/***********************************************************************
 *		lineDevSpecificFeature (TAPI32.@)
 */
DWORD WINAPI lineDevSpecificFeature(HLINE hLine, DWORD dwFeature, LPVOID lpParams, DWORD dwSize)
{
    FIXME("(%p, %08lx, %p, %ld): stub.\n", hLine, dwFeature, lpParams, dwSize);
    return 1;
}

/***********************************************************************
 *		lineDial (TAPI32.@)
 */
DWORD WINAPI lineDialA(HCALL hCall, LPCSTR lpszDestAddress, DWORD dwCountryCode)
{
    FIXME("(%p, %s, %08lx): stub.\n", hCall, lpszDestAddress, dwCountryCode);
    return 1;
}

/***********************************************************************
 *		lineDialW (TAPI32.@)
 */
DWORD WINAPI lineDialW(HCALL hCall, LPCWSTR lpszDestAddress, DWORD dwCountryCode)
{
    FIXME("(%p, %s, %08lx): stub.\n", hCall, debugstr_w(lpszDestAddress), dwCountryCode);
    return 1;
}

/***********************************************************************
 *		lineDrop (TAPI32.@)
 */
DWORD WINAPI lineDrop(HCALL hCall, LPCSTR lpsUserUserInfo, DWORD dwSize)
{
    FIXME("(%p, %s, %08lx): stub.\n", hCall, lpsUserUserInfo, dwSize);
    return 1;
}

/***********************************************************************
 *		lineForward (TAPI32.@)
 */
DWORD WINAPI lineForwardA(HLINE hLine, DWORD bAllAddress, DWORD dwAddressID, LPLINEFORWARDLIST lpForwardList, DWORD dwNumRingsNoAnswer, LPHCALL lphConsultCall, LPLINECALLPARAMS lpCallParams)
{
    FIXME("stub.\n");
    return 1;
}

/***********************************************************************
 *		lineGatherDigits (TAPI32.@)
 */
DWORD WINAPI lineGatherDigitsA(HCALL hCall, DWORD dwDigitModes, LPSTR lpsDigits, DWORD dwNumDigits, LPCSTR lpszTerminationDigits, DWORD dwFirstDigitTimeout, DWORD dwInterDigitTimeout)
{
    FIXME("stub.\n");
    return 0;
}

/***********************************************************************
 *		lineGenerateDigits (TAPI32.@)
 */
DWORD WINAPI lineGenerateDigitsA(HCALL hCall, DWORD dwDigitModes, LPCSTR lpszDigits, DWORD dwDuration)
{
    FIXME("(%p, %08lx, %s, %ld): stub.\n", hCall, dwDigitModes, lpszDigits, dwDuration);
    return 0;
}

/***********************************************************************
 *		lineGenerateTone (TAPI32.@)
 */
DWORD WINAPI lineGenerateTone(HCALL hCall, DWORD dwToneMode, DWORD dwDuration, DWORD dwNumTones, LPLINEGENERATETONE lpTones)
{
    FIXME("(%p, %08lx, %ld, %ld, %p): stub.\n", hCall, dwToneMode, dwDuration, dwNumTones, lpTones);
    return 0;
}

/***********************************************************************
 *		lineGetAddressCaps (TAPI32.@)
 */
DWORD WINAPI lineGetAddressCapsA(HLINEAPP hLineApp, DWORD dwDeviceID, DWORD dwAddressID, DWORD dwAPIVersion, DWORD dwExtVersion, LPLINEADDRESSCAPS lpAddressCaps)
{
    FIXME("(%p, %08lx, %08lx, %08lx, %08lx, %p): stub.\n", hLineApp, dwDeviceID, dwAddressID, dwAPIVersion, dwExtVersion, lpAddressCaps);
    return 0;
}

/***********************************************************************
 *		lineGetAddressID (TAPI32.@)
 */
DWORD WINAPI lineGetAddressIDA(HLINE hLine, LPDWORD lpdwAddressID, DWORD dwAddressMode, LPCSTR lpsAddress, DWORD dwSize)
{
    FIXME("%p, %p, %08lx, %s, %ld): stub.\n", hLine, lpdwAddressID, dwAddressMode, lpsAddress, dwSize);
    return 0;
}

/***********************************************************************
 *		lineGetAddressStatus (TAPI32.@)
 */
DWORD WINAPI lineGetAddressStatusA(HLINE hLine, DWORD dwAddressID, LPLINEADDRESSSTATUS lpAddressStatus)
{
    FIXME("(%p, %08lx, %p): stub.\n", hLine, dwAddressID, lpAddressStatus);
    return 0;
}

/***********************************************************************
 *		lineGetAppPriority (TAPI32.@)
 */
DWORD WINAPI lineGetAppPriorityA(LPCSTR lpszAppFilename, DWORD dwMediaMode, LPLINEEXTENSIONID const lpExtensionID, DWORD dwRequestMode, LPVARSTRING lpExtensionName, LPDWORD lpdwPriority)
{
    FIXME("(%s, %08lx, %p, %08lx, %p, %p): stub.\n", lpszAppFilename, dwMediaMode, lpExtensionID, dwRequestMode, lpExtensionName, lpdwPriority);
    return 0;
}

/***********************************************************************
 *		lineGetCallInfo (TAPI32.@)
 */
DWORD WINAPI lineGetCallInfoA(HCALL hCall, LPLINECALLINFO lpCallInfo)
{
    FIXME("(%p, %p): stub.\n", hCall, lpCallInfo);
    return 0;
}

/***********************************************************************
 *		lineGetCallInfoW (TAPI32.@)
 */
DWORD WINAPI lineGetCallInfoW(HCALL hCall, LPLINECALLINFO lpCallInfo)
{
    FIXME("(%p, %p): stub.\n", hCall, lpCallInfo);
    return 0;
}

/***********************************************************************
 *		lineGetCallStatus (TAPI32.@)
 */
DWORD WINAPI lineGetCallStatus(HCALL hCall, LPLINECALLSTATUS lpCallStatus)
{
    FIXME("(%p, %p): stub.\n", hCall, lpCallStatus);
    return 0;
}

/***********************************************************************
 *		lineGetConfRelatedCalls (TAPI32.@)
 */
DWORD WINAPI lineGetConfRelatedCalls(HCALL hCall, LPLINECALLLIST lpCallList)
{
    FIXME("(%p, %p): stub.\n", hCall, lpCallList);
    return 0;
}

/***********************************************************************
 *		lineGetCountryA (TAPI32.@)
 */
DWORD WINAPI lineGetCountryA(DWORD dwCountryID, DWORD dwAPIVersion, LPLINECOUNTRYLIST lpLineCountryList)
{
    DWORD dwAvailSize, dwOffset, i, num_countries, max_subkey_len;
    LPLINECOUNTRYENTRY lpLCE;
    HKEY hkey;
    char *subkey_name;

    if(!lpLineCountryList) {
	TRACE("(%08lx, %08lx, %p): stub. Returning LINEERR_INVALPOINTER\n",
	      dwCountryID, dwAPIVersion, lpLineCountryList);
        return LINEERR_INVALPOINTER;
    }

    TRACE("(%08lx, %08lx, %p(%ld)): stub.\n",
	  dwCountryID, dwAPIVersion, lpLineCountryList,
	  lpLineCountryList->dwTotalSize);

    if(RegOpenKeyA(HKEY_LOCAL_MACHINE, szCountrylistKey, &hkey)
            != ERROR_SUCCESS)
        return LINEERR_INIFILECORRUPT;


    dwAvailSize = lpLineCountryList->dwTotalSize;
    dwOffset = sizeof (LINECOUNTRYLIST);

    if(dwAvailSize<dwOffset)
        return LINEERR_STRUCTURETOOSMALL;

    memset(lpLineCountryList, 0, dwAvailSize);

    lpLineCountryList->dwTotalSize         = dwAvailSize;
    lpLineCountryList->dwUsedSize          = dwOffset;
    lpLineCountryList->dwNumCountries      = 0;
    lpLineCountryList->dwCountryListSize   = 0;
    lpLineCountryList->dwCountryListOffset = dwOffset;

    lpLCE = (LPLINECOUNTRYENTRY)(&lpLineCountryList[1]);

    if(RegQueryInfoKeyA(hkey, NULL, NULL, NULL, &num_countries, &max_subkey_len,
			NULL, NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) {
        RegCloseKey(hkey);
        return LINEERR_STRUCTURETOOSMALL;
    }

    if(dwCountryID)
        dwOffset = sizeof (LINECOUNTRYENTRY);
    else
        dwOffset += num_countries * sizeof (LINECOUNTRYENTRY);

    max_subkey_len++;
    subkey_name = HeapAlloc(GetProcessHeap(), 0, max_subkey_len);
    for(i = 0; i < num_countries; i++)
    {
        DWORD len, size, size_int, size_long, size_name, size_same;
	HKEY hsubkey;

	if(RegEnumKeyA(hkey, i, subkey_name, max_subkey_len) !=
	   ERROR_SUCCESS)
	    continue;

        if(dwCountryID && (atoi(subkey_name) != dwCountryID))
            continue;

	if(RegOpenKeyA(hkey, subkey_name, &hsubkey) != ERROR_SUCCESS)
	    continue;

	RegQueryValueExA(hsubkey, "InternationalRule", NULL, NULL,
			 NULL, &size_int);
        len  = size_int;

	RegQueryValueExA(hsubkey, "LongDistanceRule", NULL, NULL,
			 NULL, &size_long);
        len += size_long;

	RegQueryValueExA(hsubkey, "Name", NULL, NULL,
			 NULL, &size_name);
        len += size_name;

	RegQueryValueExA(hsubkey, "SameAreaRule", NULL, NULL,
			 NULL, &size_same);
        len += size_same;

        if(dwAvailSize < (dwOffset+len))
        {
            dwOffset += len;
	    RegCloseKey(hsubkey);
	    if(dwCountryID)
		break;
            continue;
        }

        lpLineCountryList->dwNumCountries++;
        lpLineCountryList->dwCountryListSize += sizeof (LINECOUNTRYENTRY);
        lpLineCountryList->dwUsedSize += len + sizeof (LINECOUNTRYENTRY);

	if(dwCountryID)
	    i = 0;

        lpLCE[i].dwCountryID = atoi(subkey_name);
	size = sizeof(DWORD);
	RegQueryValueExA(hsubkey, "CountryCode", NULL, NULL,
			 (BYTE*)&lpLCE[i].dwCountryCode, &size);

	lpLCE[i].dwNextCountryID = 0;
        
	if(i > 0)
            lpLCE[i-1].dwNextCountryID = lpLCE[i].dwCountryID;

        /* add country name */
        lpLCE[i].dwCountryNameSize = size_name;
        lpLCE[i].dwCountryNameOffset = dwOffset;
	RegQueryValueExA(hsubkey, "Name", NULL, NULL,
			 ((LPBYTE)lpLineCountryList)+dwOffset,
			 &size_name);
        dwOffset += size_name;

        /* add Same Area Rule */
        lpLCE[i].dwSameAreaRuleSize = size_same;
        lpLCE[i].dwSameAreaRuleOffset = dwOffset;
	RegQueryValueExA(hsubkey, "SameAreaRule", NULL, NULL,
			 ((LPBYTE)lpLineCountryList)+dwOffset,
			 &size_same);
        dwOffset += size_same;

        /* add Long Distance Rule */
        lpLCE[i].dwLongDistanceRuleSize = size_long;
        lpLCE[i].dwLongDistanceRuleOffset = dwOffset;
	RegQueryValueExA(hsubkey, "LongDistanceRule", NULL, NULL,
			 ((LPBYTE)lpLineCountryList)+dwOffset,
			 &size_long);
        dwOffset += size_long;

        /* add Long Distance Rule */
        lpLCE[i].dwInternationalRuleSize = size_int;
        lpLCE[i].dwInternationalRuleOffset = dwOffset;
	RegQueryValueExA(hsubkey, "InternationalRule", NULL, NULL,
			 ((LPBYTE)lpLineCountryList)+dwOffset,
			 &size_int);
        dwOffset += size_int;
	RegCloseKey(hsubkey);

        TRACE("Added country %s at %p\n", (LPSTR)lpLineCountryList + lpLCE[i].dwCountryNameOffset,
	      &lpLCE[i]);

	if(dwCountryID) break;
    }

    lpLineCountryList->dwNeededSize = dwOffset;

    TRACE("%ld available %ld required\n", dwAvailSize, dwOffset);

    HeapFree(GetProcessHeap(), 0, subkey_name);
    RegCloseKey(hkey);

    return 0;
}

/***********************************************************************
 *		lineGetCountryW (TAPI32.@)
 */
DWORD WINAPI lineGetCountryW(DWORD id, DWORD version, LPLINECOUNTRYLIST list)
{
    static const WCHAR country_listW[] =
        {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
         'W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
         'T','e','l','e','p','h','o','n','y','\\','C','o','u','n','t','r','y',' ','L','i','s','t',0};
    static const WCHAR international_ruleW[] =
        {'I','n','t','e','r','n','a','t','i','o','n','a','l','R','u','l','e',0};
    static const WCHAR longdistance_ruleW[] =
        {'L','o','n','g','D','i','s','t','a','n','c','e','R','u','l','e',0};
    static const WCHAR samearea_ruleW[] =
        {'S','a','m','e','A','r','e','a','R','u','l','e',0};
    static const WCHAR nameW[] =
        {'N','a','m','e',0};
    static const WCHAR country_codeW[] =
        {'C','o','u','n','t','r','y','C','o','d','e',0};
    DWORD total_size, offset, i, num_countries, max_subkey_len;
    LINECOUNTRYENTRY *entry;
    WCHAR *subkey_name;
    HKEY hkey;

    if (!list) return LINEERR_INVALPOINTER;
    TRACE("(%08lx, %08lx, %p(%ld))\n", id, version, list, list->dwTotalSize);

    if (RegOpenKeyW(HKEY_LOCAL_MACHINE, country_listW, &hkey) != ERROR_SUCCESS)
        return LINEERR_INIFILECORRUPT;

    total_size = list->dwTotalSize;
    offset = sizeof(LINECOUNTRYLIST);
    if (total_size < offset) return LINEERR_STRUCTURETOOSMALL;

    memset(list, 0, total_size);
    list->dwTotalSize         = total_size;
    list->dwUsedSize          = offset;
    list->dwNumCountries      = 0;
    list->dwCountryListSize   = 0;
    list->dwCountryListOffset = offset;

    entry = (LINECOUNTRYENTRY *)(list + 1);

    if (RegQueryInfoKeyW(hkey, NULL, NULL, NULL, &num_countries, &max_subkey_len,
                         NULL, NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
    {
        RegCloseKey(hkey);
        return LINEERR_OPERATIONFAILED;
    }
    if (id) offset = sizeof(LINECOUNTRYENTRY);
    else offset += num_countries * sizeof(LINECOUNTRYENTRY);

    max_subkey_len++;
    if (!(subkey_name = HeapAlloc(GetProcessHeap(), 0, max_subkey_len * sizeof(WCHAR))))
    {
        RegCloseKey(hkey);
        return LINEERR_NOMEM;
    }
    for (i = 0; i < num_countries; i++)
    {
        DWORD len, size, size_int, size_long, size_name, size_same;
        HKEY hsubkey;

        if (RegEnumKeyW(hkey, i, subkey_name, max_subkey_len) != ERROR_SUCCESS) continue;
        if (id && (wcstol(subkey_name, NULL, 10) != id)) continue;
        if (RegOpenKeyW(hkey, subkey_name, &hsubkey) != ERROR_SUCCESS) continue;

        RegQueryValueExW(hsubkey, international_ruleW, NULL, NULL, NULL, &size_int);
        len = size_int;

        RegQueryValueExW(hsubkey, longdistance_ruleW, NULL, NULL, NULL, &size_long);
        len += size_long;

        RegQueryValueExW(hsubkey, nameW, NULL, NULL, NULL, &size_name);
        len += size_name;

        RegQueryValueExW(hsubkey, samearea_ruleW, NULL, NULL, NULL, &size_same);
        len += size_same;

        if (total_size < offset + len)
        {
            offset += len;
            RegCloseKey(hsubkey);
            if (id) break;
            continue;
        }
        list->dwNumCountries++;
        list->dwCountryListSize += sizeof(LINECOUNTRYENTRY);
        list->dwUsedSize += len + sizeof(LINECOUNTRYENTRY);

        if (id) i = 0;
        entry[i].dwCountryID = wcstol(subkey_name, NULL, 10);
        size = sizeof(DWORD);
        RegQueryValueExW(hsubkey, country_codeW, NULL, NULL, (BYTE *)&entry[i].dwCountryCode, &size);
        entry[i].dwNextCountryID = 0;

        if (i > 0) entry[i - 1].dwNextCountryID = entry[i].dwCountryID;

        /* add country name */
        entry[i].dwCountryNameSize = size_name;
        entry[i].dwCountryNameOffset = offset;
        RegQueryValueExW(hsubkey, nameW, NULL, NULL, (BYTE *)list + offset, &size_name);
        offset += size_name;

        /* add Same Area Rule */
        entry[i].dwSameAreaRuleSize = size_same;
        entry[i].dwSameAreaRuleOffset = offset;
        RegQueryValueExW(hsubkey, samearea_ruleW, NULL, NULL, (BYTE *)list + offset, &size_same);
        offset += size_same;

        /* add Long Distance Rule */
        entry[i].dwLongDistanceRuleSize = size_long;
        entry[i].dwLongDistanceRuleOffset = offset;
        RegQueryValueExW(hsubkey, longdistance_ruleW, NULL, NULL, (BYTE *)list + offset, &size_long);
        offset += size_long;

        /* add Long Distance Rule */
        entry[i].dwInternationalRuleSize = size_int;
        entry[i].dwInternationalRuleOffset = offset;
        RegQueryValueExW(hsubkey, international_ruleW, NULL, NULL, (BYTE *)list + offset, &size_int);
        offset += size_int;
        RegCloseKey(hsubkey);

        TRACE("added country %s at %p\n",
              debugstr_w((const WCHAR *)((const char *)list + entry[i].dwCountryNameOffset)), &entry[i]);
        if (id) break;
    }
    list->dwNeededSize = offset;

    TRACE("%ld available %ld required\n", total_size, offset);

    HeapFree(GetProcessHeap(), 0, subkey_name);
    RegCloseKey(hkey);
    return 0;
}

/***********************************************************************
 *		lineGetDevCapsW (TAPI32.@)
 */
DWORD WINAPI lineGetDevCapsW(HLINEAPP hLineApp, DWORD dwDeviceID, DWORD dwAPIVersion,
                             DWORD dwExtVersion, LPLINEDEVCAPS lpLineDevCaps)
{
    static int warn_once;

    if(!warn_once++)
        FIXME("(%p, %08lx, %08lx, %08lx, %p): stub.\n", hLineApp, dwDeviceID, dwAPIVersion,
                                                 dwExtVersion, lpLineDevCaps);
    return LINEERR_OPERATIONFAILED;
}

/***********************************************************************
 *		lineGetDevCapsA (TAPI32.@)
 */
DWORD WINAPI lineGetDevCapsA(HLINEAPP hLineApp, DWORD dwDeviceID, DWORD dwAPIVersion,
                             DWORD dwExtVersion, LPLINEDEVCAPS lpLineDevCaps)
{
    static int warn_once;

    if(!warn_once++)
        FIXME("(%p, %08lx, %08lx, %08lx, %p): stub.\n", hLineApp, dwDeviceID, dwAPIVersion,
                                                 dwExtVersion, lpLineDevCaps);
    return LINEERR_OPERATIONFAILED;
}

/***********************************************************************
 *		lineGetDevConfig (TAPI32.@)
 */
DWORD WINAPI lineGetDevConfigA(DWORD dwDeviceID, LPVARSTRING lpDeviceConfig, LPCSTR lpszDeviceClass)
{
    FIXME("(%08lx, %p, %s): stub.\n", dwDeviceID, lpDeviceConfig, lpszDeviceClass);
    return 0;
}

/***********************************************************************
 *		lineGetDevConfigW (TAPI32.@)
 */
DWORD WINAPI lineGetDevConfigW(DWORD dwDeviceID, LPVARSTRING lpDeviceConfig, LPCWSTR lpszDeviceClass)
{
    FIXME("(%08lx, %p, %s): stub.\n", dwDeviceID, lpDeviceConfig, debugstr_w(lpszDeviceClass));
    return 0;
}

/***********************************************************************
 *		lineGetIDW (TAPI32.@)
 */
DWORD WINAPI lineGetIDW(HLINE hLine, DWORD dwAddressID, HCALL hCall, DWORD dwSelect,
                        LPVARSTRING lpDeviceID, LPCWSTR lpszDeviceClass)
{
    FIXME("(%p, %08lx, %p, %08lx, %p, %s): stub.\n", hLine, dwAddressID, hCall,
                                                   dwSelect, lpDeviceID,
                                                   debugstr_w(lpszDeviceClass));
    return LINEERR_OPERATIONFAILED;
}

/***********************************************************************
 *		lineGetIDA (TAPI32.@)
 */
DWORD WINAPI lineGetIDA(HLINE hLine, DWORD dwAddressID, HCALL hCall, DWORD dwSelect,
                        LPVARSTRING lpDeviceID, LPCSTR lpszDeviceClass)
{
    FIXME("(%p, %08lx, %p, %08lx, %p, %s): stub.\n", hLine, dwAddressID, hCall,
                                                   dwSelect, lpDeviceID, lpszDeviceClass);
    return LINEERR_OPERATIONFAILED;
}

/***********************************************************************
 *		lineGetIcon (TAPI32.@)
 */
DWORD WINAPI lineGetIconA(DWORD dwDeviceID, LPCSTR lpszDeviceClass, HICON *lphIcon)
{
    FIXME("(%08lx, %s, %p): stub.\n", dwDeviceID, lpszDeviceClass, lphIcon);
    return 0;
}

/***********************************************************************
 *		lineGetIconW (TAPI32.@)
 */
DWORD WINAPI lineGetIconW(DWORD dwDeviceID, LPCWSTR lpszDeviceClass, HICON *lphIcon)
{
    FIXME("(%08lx, %s, %p): stub.\n", dwDeviceID, debugstr_w(lpszDeviceClass), lphIcon);
    return 0;
}

/***********************************************************************
 *		lineGetLineDevStatus (TAPI32.@)
 */
DWORD WINAPI lineGetLineDevStatusA(HLINE hLine, LPLINEDEVSTATUS lpLineDevStatus)
{
    FIXME("(%p, %p): stub.\n", hLine, lpLineDevStatus);
    return 0;
}

/***********************************************************************
 *              lineGetMessage (TAPI32.@)
 */
DWORD WINAPI lineGetMessage(HLINEAPP hLineApp, LPLINEMESSAGE lpMessage, DWORD dwTimeout)
{
    FIXME("(%p, %p, %08lx): stub.\n", hLineApp, lpMessage, dwTimeout);
    return 0;
}

/***********************************************************************
 *		lineGetNewCalls (TAPI32.@)
 */
DWORD WINAPI lineGetNewCalls(HLINE hLine, DWORD dwAddressID, DWORD dwSelect, LPLINECALLLIST lpCallList)
{
    FIXME("(%p, %08lx, %08lx, %p): stub.\n", hLine, dwAddressID, dwSelect, lpCallList);
    return 0;
}

/***********************************************************************
 *		lineGetNumRings (TAPI32.@)
 */
DWORD WINAPI lineGetNumRings(HLINE hLine, DWORD dwAddressID, LPDWORD lpdwNumRings)
{
    FIXME("(%p, %08lx, %p): stub.\n", hLine, dwAddressID, lpdwNumRings);
    return 0;
}

/***********************************************************************
 *		lineGetProviderListA (TAPI32.@)
 */
DWORD WINAPI lineGetProviderListA(DWORD dwAPIVersion, LPLINEPROVIDERLIST lpProviderList)
{
    FIXME("(%08lx, %p): stub.\n", dwAPIVersion, lpProviderList);
    return LINEERR_OPERATIONFAILED;
}

/***********************************************************************
 *		lineGetProviderListW (TAPI32.@)
 */
DWORD WINAPI lineGetProviderListW(DWORD dwAPIVersion, LPLINEPROVIDERLIST lpProviderList)
{
    FIXME("(%08lx, %p): stub.\n", dwAPIVersion, lpProviderList);
    return LINEERR_OPERATIONFAILED;
}

/***********************************************************************
 *		lineGetRequest (TAPI32.@)
 */
DWORD WINAPI lineGetRequestA(HLINEAPP hLineApp, DWORD dwRequestMode, LPVOID lpRequestBuffer)
{
    FIXME("%p, %08lx, %p): stub.\n", hLineApp, dwRequestMode, lpRequestBuffer);
    return 0;
}

/***********************************************************************
 *		lineGetStatusMessages (TAPI32.@)
 */
DWORD WINAPI lineGetStatusMessages(HLINE hLine, LPDWORD lpdwLineStatus, LPDWORD lpdwAddressStates)
{
    FIXME("(%p, %p, %p): stub.\n", hLine, lpdwLineStatus, lpdwAddressStates);
    return 0;
}

/***********************************************************************
 *		lineGetTranslateCapsW (TAPI32.@)
 */
DWORD WINAPI lineGetTranslateCapsW(HLINEAPP line_app, DWORD api_version, LINETRANSLATECAPS *caps)
{
    FIXME("(%p, %08lx, %p): stub.\n", line_app, api_version, caps);
    return LINEERR_OPERATIONFAILED;
}

/***********************************************************************
 *		lineGetTranslateCaps (TAPI32.@)
 *
 *      get address translate capabilities. Returns a LINETRANSLATECAPS
 *      structure:
 *
 *      +-----------------------+
 *      |TotalSize              |
 *      |NeededSize             |
 *      |UsedSize               |
 *      +-----------------------+
 *      |NumLocations           |
 *      |LocationsListSize      |
 *      |LocationsListOffset    | -+
 *      |CurrentLocationID      |  |
 *      +-----------------------+  |
 *      |NumCards               |  |
 *      |CardListSize           |  |
 *      |CardListOffset         | -|--+
 *      |CurrentPreferredCardID |  |  |
 *      +-----------------------+  |  |
 *      |                       | <+  |
 *      |LINELOCATIONENTRY #1   |     |
 *      |                       |     |
 *      +-----------------------+     |
 *      ~                       ~     |
 *      +-----------------------+     |
 *      |                       |     |
 *      |LINELOCATIONENTRY      |     |
 *      |          #NumLocations|     |
 *      +-----------------------+     |
 *      |                       | <---+
 *      |LINECARDENTRY #1       |
 *      |                       |
 *      +-----------------------+
 *      ~                       ~
 *      +-----------------------+
 *      |                       |
 *      |LINECARDENTRY #NumCards|
 *      |                       |
 *      +-----------------------+
 *      | room for strings named|
 *      | in the structures     |
 *      | above.                |
 *      +-----------------------+
 */
DWORD WINAPI lineGetTranslateCapsA(HLINEAPP hLineApp, DWORD dwAPIVersion,
        LPLINETRANSLATECAPS lpTranslateCaps)
{
    HKEY hkLocations, hkCards, hkCardLocations, hsubkey;
    int numlocations, numcards;
    DWORD maxlockeylen,
        maxcardkeylen;
    char *loc_key_name = NULL;
    char *card_key_name = NULL;
    LPBYTE strptr;
    int length;
    int i;
    DWORD lendword;
    DWORD currentid;
    LPLINELOCATIONENTRY pLocEntry;
    LPLINECARDENTRY pCardEntry;
    
    TRACE("(%p, %08lx, %p (tot. size %ld)\n", hLineApp, dwAPIVersion,
            lpTranslateCaps, lpTranslateCaps->dwTotalSize );
    if( lpTranslateCaps->dwTotalSize < sizeof(LINETRANSLATECAPS))
        return LINEERR_STRUCTURETOOSMALL;
    if( RegCreateKeyA(HKEY_LOCAL_MACHINE, szLocationsKey, &hkLocations)
            != ERROR_SUCCESS ) {
        ERR("unexpected registry error 1.\n");
        return LINEERR_INIFILECORRUPT;  
    }
    lendword = sizeof( DWORD);
    if( RegQueryValueExA( hkLocations, "CurrentID", NULL, NULL,
                (LPBYTE) &currentid, &lendword) != ERROR_SUCCESS )
        currentid = -1;  /* change this later */
    if(RegQueryInfoKeyA(hkLocations, NULL, NULL, NULL, NULL, &maxlockeylen,
			NULL, NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) {
        RegCloseKey(hkLocations);
        ERR("unexpected registry error 2.\n");
        return LINEERR_INIFILECORRUPT;
    }
    maxlockeylen++;
    if( maxlockeylen < 10)
        maxlockeylen = 10; /* need this also if there is no key */
    loc_key_name = HeapAlloc( GetProcessHeap(), 0, maxlockeylen);
    /* first time through: calculate needed space */
    length=0;
    i=0;
    numlocations=0;
    while( RegEnumKeyA(hkLocations, i, loc_key_name, maxlockeylen)
            == ERROR_SUCCESS){
        DWORD size_val;
        i++;
        if( _strnicmp(loc_key_name, "location", 8)  ||
                (RegOpenKeyA(hkLocations, loc_key_name, &hsubkey)
                 != ERROR_SUCCESS))
            continue;
        numlocations++;
        length += sizeof(LINELOCATIONENTRY);
        RegQueryValueExA(hsubkey, "Name",NULL,NULL,NULL,&size_val); 
        length += size_val;
        RegQueryValueExA(hsubkey, "AreaCode",NULL,NULL,NULL,&size_val); 
        length += size_val;
        RegQueryValueExA(hsubkey, "OutsideAccess",NULL,NULL,NULL,&size_val); 
        length += size_val;
        RegQueryValueExA(hsubkey, "LongDistanceAccess",NULL,NULL,NULL,&size_val); 
        length += size_val;
        RegQueryValueExA(hsubkey, "DisableCallWaiting",NULL,NULL,NULL,&size_val); 
        length += size_val;
        /* fixme: what about TollPrefixList???? */
        RegCloseKey(hsubkey);
    }
    if(numlocations == 0) {
        /* add one location */
        if( RegCreateKeyA( hkLocations, "Location1", &hsubkey)
                == ERROR_SUCCESS) {
            DWORD dwval;
            char buf[10];
            numlocations = 1;
            length += sizeof(LINELOCATIONENTRY) + 20 ;
            RegSetValueExA( hsubkey, "AreaCode", 0, REG_SZ, (const BYTE *)"010", 4);
            GetLocaleInfoA( LOCALE_SYSTEM_DEFAULT, LOCALE_ICOUNTRY, buf, 8);
            dwval = atoi(buf);
            RegSetValueExA( hsubkey, "Country", 0, REG_DWORD, (LPBYTE)&dwval,
                    sizeof(DWORD));
            RegSetValueExA( hsubkey, "DisableCallWaiting", 0, REG_SZ, (const BYTE *)"", 1);
            dwval = 1;  
            RegSetValueExA( hsubkey, "Flags", 0, REG_DWORD, (LPBYTE)&dwval,
                    sizeof(DWORD));
            RegSetValueExA( hsubkey, "LongDistanceAccess", 0, REG_SZ, (const BYTE *)"", 1);
            RegSetValueExA( hsubkey, "Name", 0, REG_SZ, (const BYTE *)"New Location", 13);
            RegSetValueExA( hsubkey, "OutsideAccess", 0, REG_SZ, (const BYTE *)"", 1);
            RegCloseKey(hsubkey);
            dwval = 1;  
            RegSetValueExA( hkLocations, "CurrentID", 0, REG_DWORD,
                    (LPBYTE)&dwval, sizeof(DWORD));
            dwval = 2;  
            RegSetValueExA( hkLocations, "NextID", 0, REG_DWORD, (LPBYTE)&dwval,
                    sizeof(DWORD));
        }
    }
    /* do the card list */
    numcards=0;
    if( RegCreateKeyA(HKEY_CURRENT_USER, szCardsKey, &hkCards)
            == ERROR_SUCCESS ) {
        if(RegQueryInfoKeyA(hkCards, NULL, NULL, NULL, NULL, &maxcardkeylen,
                NULL, NULL, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            maxcardkeylen++;
            if( maxcardkeylen < 6) maxcardkeylen = 6;
            card_key_name = HeapAlloc(GetProcessHeap(), 0, maxcardkeylen);
            i=0;
            while( RegEnumKeyA(hkCards, i, card_key_name, maxcardkeylen) ==
                    ERROR_SUCCESS){
                DWORD size_val;
                i++;
                if( _strnicmp(card_key_name, "card", 4)  || ERROR_SUCCESS !=
                        (RegOpenKeyA(hkCards, card_key_name, &hsubkey) ))
                    continue;
                numcards++;
                length += sizeof(LINECARDENTRY);
                RegQueryValueExA(hsubkey, "Name",NULL,NULL,NULL,&size_val); 
                length += size_val;
                RegQueryValueExA(hsubkey, "LocalRule",NULL,NULL,NULL,&size_val); 
                length += size_val;
                RegQueryValueExA(hsubkey, "LDRule",NULL,NULL,NULL,&size_val); 
                length += size_val;
                RegQueryValueExA(hsubkey, "InternationalRule",NULL,NULL,NULL,
                        &size_val); 
                length += size_val;
                RegCloseKey(hsubkey);
            }
        }
        /* add one card (direct call) */
        if (numcards == 0 &&
                ERROR_SUCCESS == RegCreateKeyA( hkCards, "Card1", &hsubkey)) {
            DWORD dwval;
            numcards = 1;
            length += sizeof(LINECARDENTRY) + 22 ;
            RegSetValueExA( hsubkey, "Name", 0, REG_SZ, (const BYTE *)"None (Direct Call)", 19);
            dwval = 1;  
            RegSetValueExA( hsubkey, "Flags", 0, REG_DWORD, (LPBYTE)&dwval,
                    sizeof(DWORD));
            RegSetValueExA( hsubkey, "InternationalRule", 0, REG_SZ, (const BYTE *)"", 1);
            RegSetValueExA( hsubkey, "LDRule", 0, REG_SZ, (const BYTE *)"", 1);
            RegSetValueExA( hsubkey, "LocalRule", 0, REG_SZ, (const BYTE *)"", 1);
            RegCloseKey(hsubkey);
            dwval = 2;  
            RegSetValueExA( hkCards, "NextID", 0, REG_DWORD, (LPBYTE)&dwval,
                    sizeof(DWORD));
        }
    } else hkCards = 0;  /* should really fail */
    /* check if sufficient room is available */
    lpTranslateCaps->dwNeededSize =  sizeof(LINETRANSLATECAPS) + length;
    if ( lpTranslateCaps->dwNeededSize > lpTranslateCaps->dwTotalSize ) {
        RegCloseKey( hkLocations);
        if( hkCards) RegCloseKey( hkCards);
        HeapFree(GetProcessHeap(), 0, loc_key_name);
        HeapFree(GetProcessHeap(), 0, card_key_name);
        lpTranslateCaps->dwUsedSize = sizeof(LINETRANSLATECAPS);
        TRACE("Insufficient space: total %ld needed %ld used %ld\n",
                lpTranslateCaps->dwTotalSize,
                lpTranslateCaps->dwNeededSize,
                lpTranslateCaps->dwUsedSize);
        return  0;
    }
    /* fill in the LINETRANSLATECAPS structure */
    lpTranslateCaps->dwUsedSize = lpTranslateCaps->dwNeededSize;
    lpTranslateCaps->dwNumLocations = numlocations;
    lpTranslateCaps->dwLocationListSize = sizeof(LINELOCATIONENTRY) *
            lpTranslateCaps->dwNumLocations;
    lpTranslateCaps->dwLocationListOffset = sizeof(LINETRANSLATECAPS);
    lpTranslateCaps->dwCurrentLocationID = currentid; 
    lpTranslateCaps->dwNumCards = numcards;
    lpTranslateCaps->dwCardListSize = sizeof(LINECARDENTRY) *
            lpTranslateCaps->dwNumCards;
    lpTranslateCaps->dwCardListOffset = lpTranslateCaps->dwLocationListOffset +
            lpTranslateCaps->dwLocationListSize;
    lpTranslateCaps->dwCurrentPreferredCardID = 0; 
    /* this is where the strings will be stored */
    strptr = ((LPBYTE) lpTranslateCaps) +
        lpTranslateCaps->dwCardListOffset + lpTranslateCaps->dwCardListSize;
    pLocEntry = (LPLINELOCATIONENTRY) (lpTranslateCaps + 1);
    /* key with Preferred CardIDs */
    if( RegOpenKeyA(HKEY_CURRENT_USER, szLocationsKey, &hkCardLocations)
            != ERROR_SUCCESS ) 
        hkCardLocations = 0;
    /* second time through all locations */
    i=0;
    while(RegEnumKeyA(hkLocations, i, loc_key_name, maxlockeylen)
            == ERROR_SUCCESS){
        DWORD size_val;
        i++;
        if( _strnicmp(loc_key_name, "location", 8)  ||
                (RegOpenKeyA(hkLocations, loc_key_name, &hsubkey)
                 != ERROR_SUCCESS))
            continue;
        size_val=sizeof(DWORD);
        if( RegQueryValueExA(hsubkey, "ID",NULL, NULL,
                (LPBYTE) &(pLocEntry->dwPermanentLocationID), &size_val) !=
                ERROR_SUCCESS)
            pLocEntry->dwPermanentLocationID = atoi( loc_key_name + 8);
        size_val=2048;
        RegQueryValueExA(hsubkey, "Name",NULL,NULL, strptr, &size_val);
        pLocEntry->dwLocationNameSize = size_val;
        pLocEntry->dwLocationNameOffset = strptr - (LPBYTE) lpTranslateCaps;
        strptr += size_val;
 
        size_val=2048;
        RegQueryValueExA(hsubkey, "AreaCode",NULL,NULL, strptr, &size_val);
        pLocEntry->dwCityCodeSize = size_val;
        pLocEntry->dwCityCodeOffset = strptr - (LPBYTE) lpTranslateCaps;
        strptr += size_val;
        
        size_val=2048;
        RegQueryValueExA(hsubkey, "OutsideAccess",NULL,NULL, strptr, &size_val);
        pLocEntry->dwLocalAccessCodeSize = size_val;
        pLocEntry->dwLocalAccessCodeOffset = strptr - (LPBYTE) lpTranslateCaps;
        strptr += size_val;
        size_val=2048;
        RegQueryValueExA(hsubkey, "LongDistanceAccess",NULL,NULL, strptr,
                &size_val);
        pLocEntry->dwLongDistanceAccessCodeSize= size_val;
        pLocEntry->dwLongDistanceAccessCodeOffset= strptr -
            (LPBYTE) lpTranslateCaps;
        strptr += size_val;
        size_val=2048;
        RegQueryValueExA(hsubkey, "DisableCallWaiting",NULL,NULL, strptr,
                &size_val);
        pLocEntry->dwCancelCallWaitingSize= size_val;
        pLocEntry->dwCancelCallWaitingOffset= strptr - (LPBYTE) lpTranslateCaps;
        strptr += size_val;

        pLocEntry->dwTollPrefixListSize = 0;    /* FIXME */
        pLocEntry->dwTollPrefixListOffset = 0;    /* FIXME */

        size_val=sizeof(DWORD);
        RegQueryValueExA(hsubkey, "Country",NULL,NULL,
                (LPBYTE) &(pLocEntry->dwCountryCode), &size_val);
        pLocEntry->dwCountryID = pLocEntry->dwCountryCode; /* FIXME */
        RegQueryValueExA(hsubkey, "Flags",NULL,NULL,
                (LPBYTE) &(pLocEntry->dwOptions), &size_val);
        RegCloseKey(hsubkey);
        /* get preferred cardid */
        pLocEntry->dwPreferredCardID = 0;
        if ( hkCardLocations) {
            size_val=sizeof(DWORD);
            if(RegOpenKeyA(hkCardLocations, loc_key_name, &hsubkey) ==
                    ERROR_SUCCESS) {
                RegQueryValueExA(hsubkey, "CallingCard",NULL,NULL,
                        (LPBYTE) &(pLocEntry->dwPreferredCardID), &size_val);
                RegCloseKey(hsubkey);
            }
                
        }
        /* make sure there is a currentID */
        if(currentid == -1){
            currentid = pLocEntry->dwPermanentLocationID;
            lpTranslateCaps->dwCurrentLocationID = currentid; 
        }
        if(pLocEntry->dwPermanentLocationID == currentid )
            lpTranslateCaps->dwCurrentPreferredCardID =
                    pLocEntry->dwPreferredCardID;
        TRACE("added: ID %ld %s CountryCode %ld CityCode %s CardID %ld "
                "LocalAccess: %s LongDistanceAccess: %s CountryID %ld "
                "Options %ld CancelCallWait %s\n",
                pLocEntry->dwPermanentLocationID,
                debugstr_a( (char*)lpTranslateCaps + pLocEntry->dwLocationNameOffset),
                pLocEntry->dwCountryCode,
                debugstr_a( (char*)lpTranslateCaps + pLocEntry->dwCityCodeOffset),
                pLocEntry->dwPreferredCardID,
                debugstr_a( (char*)lpTranslateCaps + pLocEntry->dwLocalAccessCodeOffset),
                debugstr_a( (char*)lpTranslateCaps + pLocEntry->dwLongDistanceAccessCodeOffset),
                pLocEntry->dwCountryID,
                pLocEntry->dwOptions,
                debugstr_a( (char*)lpTranslateCaps + pLocEntry->dwCancelCallWaitingOffset));
        pLocEntry++;
    }
    pCardEntry= (LPLINECARDENTRY) pLocEntry;
    /* do the card list */
    if( hkCards) {
        i=0;
        while( RegEnumKeyA(hkCards, i, card_key_name, maxcardkeylen) ==
                ERROR_SUCCESS){
            DWORD size_val;
            i++;
            if( _strnicmp(card_key_name, "card", 4)  ||
                    (RegOpenKeyA(hkCards, card_key_name, &hsubkey) != ERROR_SUCCESS))
                continue;
            size_val=sizeof(DWORD);
            if( RegQueryValueExA(hsubkey, "ID",NULL, NULL,
                    (LPBYTE) &(pCardEntry->dwPermanentCardID), &size_val) !=
                    ERROR_SUCCESS)
                pCardEntry->dwPermanentCardID= atoi( card_key_name + 4);
            size_val=2048;
            RegQueryValueExA(hsubkey, "Name",NULL,NULL, strptr, &size_val);
            pCardEntry->dwCardNameSize = size_val;
            pCardEntry->dwCardNameOffset = strptr - (LPBYTE) lpTranslateCaps;
            strptr += size_val;
            pCardEntry->dwCardNumberDigits = 1; /* FIXME */
            size_val=2048;
            RegQueryValueExA(hsubkey, "LocalRule",NULL,NULL, strptr, &size_val);
            pCardEntry->dwSameAreaRuleSize= size_val;
            pCardEntry->dwSameAreaRuleOffset= strptr - (LPBYTE) lpTranslateCaps;
            strptr += size_val;
            size_val=2048;
            RegQueryValueExA(hsubkey, "LDRule",NULL,NULL, strptr, &size_val);
            pCardEntry->dwLongDistanceRuleSize = size_val;
            pCardEntry->dwLongDistanceRuleOffset = strptr - (LPBYTE) lpTranslateCaps;
            strptr += size_val;
            size_val=2048;
            RegQueryValueExA(hsubkey, "InternationalRule",NULL,NULL, strptr,
                    &size_val);
            pCardEntry->dwInternationalRuleSize = size_val;
            pCardEntry->dwInternationalRuleOffset = strptr -
                (LPBYTE) lpTranslateCaps;
            strptr += size_val;
            size_val=sizeof(DWORD);
            RegQueryValueExA(hsubkey, "Flags",NULL, NULL,
                    (LPBYTE) &(pCardEntry->dwOptions), &size_val); 
            TRACE( "added card: ID %ld name %s SameArea %s LongDistance %s International %s Options 0x%lx\n",
                    pCardEntry->dwPermanentCardID,
                    debugstr_a( (char*)lpTranslateCaps + pCardEntry->dwCardNameOffset),
                    debugstr_a( (char*)lpTranslateCaps + pCardEntry->dwSameAreaRuleOffset),
                    debugstr_a( (char*)lpTranslateCaps + pCardEntry->dwLongDistanceRuleOffset),
                    debugstr_a( (char*)lpTranslateCaps + pCardEntry->dwInternationalRuleOffset),
                    pCardEntry->dwOptions);

            pCardEntry++;
        }
    }

    if(hkLocations) RegCloseKey(hkLocations);
    if(hkCards) RegCloseKey(hkCards);
    if(hkCardLocations) RegCloseKey(hkCardLocations);
    HeapFree(GetProcessHeap(), 0, loc_key_name);
    HeapFree(GetProcessHeap(), 0, card_key_name);
    TRACE(" returning success tot %ld needed %ld used %ld\n",
            lpTranslateCaps->dwTotalSize,
            lpTranslateCaps->dwNeededSize,
            lpTranslateCaps->dwUsedSize );
    return 0; /* success */
}

/***********************************************************************
 *		lineHandoff (TAPI32.@)
 */
DWORD WINAPI lineHandoffA(HCALL hCall, LPCSTR lpszFileName, DWORD dwMediaMode)
{
    FIXME("(%p, %s, %08lx): stub.\n", hCall, lpszFileName, dwMediaMode);
    return 0;
}

/***********************************************************************
 *		lineHold (TAPI32.@)
 */
DWORD WINAPI lineHold(HCALL hCall)
{
    FIXME("(%p): stub.\n", hCall);
    return 1;
}

/***********************************************************************
 *		lineInitialize (TAPI32.@)
 */
DWORD WINAPI lineInitialize(
  LPHLINEAPP lphLineApp,
  HINSTANCE hInstance,
  LINECALLBACK lpfnCallback,
  LPCSTR lpszAppName,
  LPDWORD lpdwNumDevs)
{
    FIXME("(%p, %p, %p, %s, %p): stub.\n", lphLineApp, hInstance,
	  lpfnCallback, debugstr_a(lpszAppName), lpdwNumDevs);
    return 0;
}

/***********************************************************************
 *              lineInitializeExA (TAPI32.@)
 */
LONG WINAPI lineInitializeExA(LPHLINEAPP lphLineApp, HINSTANCE hInstance, LINECALLBACK lpfnCallback, LPCSTR lpszFriendlyAppName, LPDWORD lpdwNumDevs, LPDWORD lpdwAPIVersion, LPLINEINITIALIZEEXPARAMS lpLineInitializeExParams)
{
    FIXME("(%p, %p, %p, %s, %p, %p, %p): stub.\n", lphLineApp, hInstance,
          lpfnCallback, debugstr_a(lpszFriendlyAppName), lpdwNumDevs, lpdwAPIVersion, lpLineInitializeExParams);
    return 0;
}

/***********************************************************************
 *              lineInitializeExW (TAPI32.@)
 */
LONG WINAPI lineInitializeExW(LPHLINEAPP lphLineApp, HINSTANCE hInstance, LINECALLBACK lpfnCallback, LPCWSTR lpszFriendlyAppName, LPDWORD lpdwNumDevs, LPDWORD lpdwAPIVersion, LPLINEINITIALIZEEXPARAMS lpLineInitializeExParams)
{
    FIXME("(%p, %p, %p, %s, %p, %p, %p): stub.\n", lphLineApp, hInstance,
          lpfnCallback, debugstr_w(lpszFriendlyAppName), lpdwNumDevs, lpdwAPIVersion, lpLineInitializeExParams);
    return 0;
}

/***********************************************************************
 *		lineMakeCallW (TAPI32.@)
 */
DWORD WINAPI lineMakeCallW(HLINE hLine, LPHCALL lphCall, LPCWSTR lpszDestAddress,
                           DWORD dwCountryCode, LPLINECALLPARAMS lpCallParams)
{
    FIXME("(%p, %p, %s, %08lx, %p): stub.\n", hLine, lphCall, debugstr_w(lpszDestAddress),
                                             dwCountryCode, lpCallParams);
    return LINEERR_OPERATIONFAILED;
}

/***********************************************************************
 *		lineMakeCallA (TAPI32.@)
 */
DWORD WINAPI lineMakeCallA(HLINE hLine, LPHCALL lphCall, LPCSTR lpszDestAddress,
                           DWORD dwCountryCode, LPLINECALLPARAMS lpCallParams)
{
    FIXME("(%p, %p, %s, %08lx, %p): stub.\n", hLine, lphCall, lpszDestAddress,
                                             dwCountryCode, lpCallParams);
    return LINEERR_OPERATIONFAILED;
}

/***********************************************************************
 *		lineMonitorDigits (TAPI32.@)
 */
DWORD WINAPI lineMonitorDigits(HCALL hCall, DWORD dwDigitModes)
{
    FIXME("(%p, %08lx): stub.\n", hCall, dwDigitModes);
    return 0;
}

/***********************************************************************
 *		lineMonitorMedia (TAPI32.@)
 */
DWORD WINAPI lineMonitorMedia(HCALL hCall, DWORD dwMediaModes)
{
    FIXME("(%p, %08lx): stub.\n", hCall, dwMediaModes);
    return 0;
}

/***********************************************************************
 *		lineMonitorTones (TAPI32.@)
 */
DWORD WINAPI lineMonitorTones(HCALL hCall, LPLINEMONITORTONE lpToneList, DWORD dwNumEntries)
{
    FIXME("(%p, %p, %08lx): stub.\n", hCall, lpToneList, dwNumEntries);
    return 0;
}

/***********************************************************************
 *		lineNegotiateAPIVersion (TAPI32.@)
 */
DWORD WINAPI lineNegotiateAPIVersion(
  HLINEAPP hLineApp,
  DWORD dwDeviceID,
  DWORD dwAPILowVersion,
  DWORD dwAPIHighVersion,
  LPDWORD lpdwAPIVersion,
  LPLINEEXTENSIONID lpExtensionID
)
{
    static int warn_once;

    if(!warn_once++)
        FIXME("(%p, %ld, %ld, %ld, %p, %p): stub.\n", hLineApp, dwDeviceID,
              dwAPILowVersion, dwAPIHighVersion, lpdwAPIVersion, lpExtensionID);
    *lpdwAPIVersion = dwAPIHighVersion;
    return 0;
}

/***********************************************************************
 *		lineNegotiateExtVersion (TAPI32.@)
 */
DWORD WINAPI lineNegotiateExtVersion(HLINEAPP hLineApp, DWORD dwDeviceID, DWORD dwAPIVersion, DWORD dwExtLowVersion, DWORD dwExtHighVersion, LPDWORD lpdwExtVersion)
{
    FIXME("stub.\n");
    return 0;
}

/***********************************************************************
 *		lineOpenW (TAPI32.@)
 */
DWORD WINAPI lineOpenW(HLINEAPP hLineApp, DWORD dwDeviceID, LPHLINE lphLine, DWORD dwAPIVersion, DWORD dwExtVersion, DWORD dwCallbackInstance, DWORD dwPrivileges, DWORD dwMediaModes, LPLINECALLPARAMS lpCallParams)
{
    FIXME("stub.\n");
    return 0;
}

/***********************************************************************
 *		lineOpen (TAPI32.@)
 */
DWORD WINAPI lineOpenA(HLINEAPP hLineApp, DWORD dwDeviceID, LPHLINE lphLine, DWORD dwAPIVersion, DWORD dwExtVersion, DWORD dwCallbackInstance, DWORD dwPrivileges, DWORD dwMediaModes, LPLINECALLPARAMS lpCallParams)
{
    FIXME("stub.\n");
    return 0;
}

/***********************************************************************
 *		linePark (TAPI32.@)
 */
DWORD WINAPI lineParkA(HCALL hCall, DWORD dwParkMode, LPCSTR lpszDirAddress, LPVARSTRING lpNonDirAddress)
{
    FIXME("(%p, %08lx, %s, %p): stub.\n", hCall, dwParkMode, lpszDirAddress, lpNonDirAddress);
    return 1;
}

/***********************************************************************
 *		linePickup (TAPI32.@)
 */
DWORD WINAPI linePickupA(HLINE hLine, DWORD dwAddressID, LPHCALL lphCall, LPCSTR lpszDestAddress, LPCSTR lpszGroupID)
{
    FIXME("(%p, %08lx, %p, %s, %s): stub.\n", hLine, dwAddressID, lphCall, lpszDestAddress, lpszGroupID);
    return 1;
}

/***********************************************************************
 *		linePrepareAddToConference (TAPI32.@)
 */
DWORD WINAPI linePrepareAddToConferenceA(HCALL hConfCall, LPHCALL lphConsultCall, LPLINECALLPARAMS lpCallParams)
{
    FIXME("(%p, %p, %p): stub.\n", hConfCall, lphConsultCall, lpCallParams);
    return 1;
}

/***********************************************************************
 *		lineRedirect (TAPI32.@)
 */
DWORD WINAPI lineRedirectA(
  HCALL hCall,
  LPCSTR lpszDestAddress,
  DWORD  dwCountryCode) {

  FIXME(": stub.\n");
  return 1;
}

/***********************************************************************
 *		lineRegisterRequestRecipient (TAPI32.@)
 */
DWORD WINAPI lineRegisterRequestRecipient(HLINEAPP hLineApp, DWORD dwRegistrationInstance, DWORD dwRequestMode, DWORD dwEnable)
{
    FIXME("(%p, %08lx, %08lx, %08lx): stub.\n", hLineApp, dwRegistrationInstance, dwRequestMode, dwEnable);
    return 1;
}

/***********************************************************************
 *		lineReleaseUserUserInfo (TAPI32.@)
 */
DWORD WINAPI lineReleaseUserUserInfo(HCALL hCall)
{
    FIXME("(%p): stub.\n", hCall);
    return 1;
}

/***********************************************************************
 *		lineRemoveFromConference (TAPI32.@)
 */
DWORD WINAPI lineRemoveFromConference(HCALL hCall)
{
    FIXME("(%p): stub.\n", hCall);
    return 1;
}

/***********************************************************************
 *		lineRemoveProvider (TAPI32.@)
 */
DWORD WINAPI lineRemoveProvider(DWORD dwPermanentProviderID, HWND hwndOwner)
{
    FIXME("(%08lx, %p): stub.\n", dwPermanentProviderID, hwndOwner);
    return 1;
}

/***********************************************************************
 *		lineSecureCall (TAPI32.@)
 */
DWORD WINAPI lineSecureCall(HCALL hCall)
{
    FIXME("(%p): stub.\n", hCall);
    return 1;
}

/***********************************************************************
 *		lineSendUserUserInfo (TAPI32.@)
 */
DWORD WINAPI lineSendUserUserInfo(HCALL hCall, LPCSTR lpsUserUserInfo, DWORD dwSize)
{
    FIXME("(%p, %s, %08lx): stub.\n", hCall, lpsUserUserInfo, dwSize);
    return 1;
}

/***********************************************************************
 *		lineSetAppPriority (TAPI32.@)
 */
DWORD WINAPI lineSetAppPriorityA(LPCSTR lpszAppFilename, DWORD dwMediaMode, LPLINEEXTENSIONID const lpExtensionID, DWORD dwRequestMode, LPCSTR lpszExtensionName, DWORD dwPriority)
{
    FIXME("(%s, %08lx, %p, %08lx, %s, %08lx): stub.\n", lpszAppFilename, dwMediaMode, lpExtensionID, dwRequestMode, lpszExtensionName, dwPriority);
    return 0;
}

/***********************************************************************
 *		lineSetAppSpecific (TAPI32.@)
 */
DWORD WINAPI lineSetAppSpecific(HCALL hCall, DWORD dwAppSpecific)
{
    FIXME("(%p, %08lx): stub.\n", hCall, dwAppSpecific);
    return 0;
}

/***********************************************************************
 *		lineSetCallParams (TAPI32.@)
 */
DWORD WINAPI lineSetCallParams(HCALL hCall, DWORD dwBearerMode, DWORD dwMinRate, DWORD dwMaxRate, LPLINEDIALPARAMS lpDialParams)
{
    FIXME("(%p, %08lx, %08lx, %08lx, %p): stub.\n", hCall, dwBearerMode, dwMinRate, dwMaxRate, lpDialParams);
    return 1;
}

/***********************************************************************
 *		lineSetCallPrivilege (TAPI32.@)
 */
DWORD WINAPI lineSetCallPrivilege(HCALL hCall, DWORD dwCallPrivilege)
{
    FIXME("(%p, %08lx): stub.\n", hCall, dwCallPrivilege);
    return 0;
}

/***********************************************************************
 *		lineSetCurrentLocation (TAPI32.@)
 */
DWORD WINAPI lineSetCurrentLocation(HLINEAPP hLineApp, DWORD dwLocation)
{
    FIXME("(%p, %08lx): stub.\n", hLineApp, dwLocation);
    return 0;
}

/***********************************************************************
 *		lineSetDevConfig (TAPI32.@)
 */
DWORD WINAPI lineSetDevConfigA(DWORD dwDeviceID, LPVOID lpDeviceConfig, DWORD dwSize, LPCSTR lpszDeviceClass)
{
    FIXME("(%08lx, %p, %08lx, %s): stub.\n", dwDeviceID, lpDeviceConfig, dwSize, lpszDeviceClass);
    return 0;
}

/***********************************************************************
 *		lineSetDevConfigW (TAPI32.@)
 */
DWORD WINAPI lineSetDevConfigW(DWORD dwDeviceID, LPVOID lpDeviceConfig, DWORD dwSize, LPCWSTR lpszDeviceClass)
{
    FIXME("(%08lx, %p, %08lx, %s): stub.\n", dwDeviceID, lpDeviceConfig, dwSize, debugstr_w(lpszDeviceClass));
    return 0;
}

/***********************************************************************
 *		lineSetMediaControl (TAPI32.@)
 */
DWORD WINAPI lineSetMediaControl(
HLINE hLine,
DWORD dwAddressID,
HCALL hCall,
DWORD dwSelect,
LPLINEMEDIACONTROLDIGIT const lpDigitList,
DWORD dwDigitNumEntries,
LPLINEMEDIACONTROLMEDIA const lpMediaList,
DWORD dwMediaNumEntries,
LPLINEMEDIACONTROLTONE const lpToneList,
DWORD dwToneNumEntries,
LPLINEMEDIACONTROLCALLSTATE const lpCallStateList,
DWORD dwCallStateNumEntries)
{
    FIXME(": stub.\n");
    return 0;
}

/***********************************************************************
 *		lineSetMediaMode (TAPI32.@)
 */
DWORD WINAPI lineSetMediaMode(HCALL hCall, DWORD dwMediaModes)
{
    FIXME("(%p, %08lx): stub.\n", hCall, dwMediaModes);
    return 0;
}

/***********************************************************************
 *		lineSetNumRings (TAPI32.@)
 */
DWORD WINAPI lineSetNumRings(HLINE hLine, DWORD dwAddressID, DWORD dwNumRings)
{
    FIXME("(%p, %08lx, %08lx): stub.\n", hLine, dwAddressID, dwNumRings);
    return 0;
}

/***********************************************************************
 *		lineSetStatusMessages (TAPI32.@)
 */
DWORD WINAPI lineSetStatusMessages(HLINE hLine, DWORD dwLineStates, DWORD dwAddressStates)
{
    FIXME("(%p, %08lx, %08lx): stub.\n", hLine, dwLineStates, dwAddressStates);
    return 0;
}

/***********************************************************************
 *		lineSetTerminal (TAPI32.@)
 */
DWORD WINAPI lineSetTerminal(HLINE hLine, DWORD dwAddressID, HCALL hCall, DWORD dwSelect, DWORD dwTerminalModes, DWORD dwTerminalID, DWORD bEnable)
{
    FIXME("(%p, %08lx, %p, %08lx, %08lx, %08lx, %08lx): stub.\n", hLine, dwAddressID, hCall, dwSelect, dwTerminalModes, dwTerminalID, bEnable);
    return 1;
}

/***********************************************************************
 *		lineSetTollList (TAPI32.@)
 */
DWORD WINAPI lineSetTollListA(HLINEAPP hLineApp, DWORD dwDeviceID, LPCSTR lpszAddressIn, DWORD dwTollListOption)
{
    FIXME("(%p, %08lx, %s, %08lx): stub.\n", hLineApp, dwDeviceID, lpszAddressIn, dwTollListOption);
    return 0;
}

/***********************************************************************
 *		lineSetupConference (TAPI32.@)
 */
DWORD WINAPI lineSetupConferenceA(HCALL hCall, HLINE hLine, LPHCALL lphConfCall, LPHCALL lphConsultCall, DWORD dwNumParties, LPLINECALLPARAMS lpCallParams)
{
    FIXME("(%p, %p, %p, %p, %08lx, %p): stub.\n", hCall, hLine, lphConfCall, lphConsultCall, dwNumParties, lpCallParams);
    return 1;
}

/***********************************************************************
 *		lineSetupTransfer (TAPI32.@)
 */
DWORD WINAPI lineSetupTransferA(HCALL hCall, LPHCALL lphConsultCall, LPLINECALLPARAMS lpCallParams)
{
    FIXME("(%p, %p, %p): stub.\n", hCall, lphConsultCall, lpCallParams);
    return 1;
}

/***********************************************************************
 *		lineShutdown (TAPI32.@)
 */
DWORD WINAPI lineShutdown(HLINEAPP hLineApp)
{
    FIXME("(%p): stub.\n", hLineApp);
    return 0;
}

/***********************************************************************
 *		lineSwapHold (TAPI32.@)
 */
DWORD WINAPI lineSwapHold(HCALL hActiveCall, HCALL hHeldCall)
{
    FIXME("(active: %p, held: %p): stub.\n", hActiveCall, hHeldCall);
    return 1;
}

/***********************************************************************
 *		lineTranslateAddress (TAPI32.@)
 */
DWORD WINAPI lineTranslateAddressA(HLINEAPP hLineApp, DWORD dwDeviceID, DWORD dwAPIVersion, LPCSTR lpszAddressIn, DWORD dwCard, DWORD dwTranslateOptions, LPLINETRANSLATEOUTPUT lpTranslateOutput)
{
    FIXME("(%p, %08lx, %08lx, %s, %08lx, %08lx, %p): stub.\n", hLineApp, dwDeviceID, dwAPIVersion, lpszAddressIn, dwCard, dwTranslateOptions, lpTranslateOutput);
    return 0;
}

/***********************************************************************
 *              lineTranslateAddressW (TAPI32.@)
 */
DWORD WINAPI lineTranslateAddressW(HLINEAPP hLineApp, DWORD dwDeviceID,
        DWORD dwAPIVersion, LPCWSTR lpszAddressIn, DWORD dwCard,
        DWORD dwTranslateOptions, LPLINETRANSLATEOUTPUT lpTranslateOutput)
{
    FIXME("(%p, %08lx, %08lx, %s, %08lx, %08lx, %p): stub.\n", hLineApp, dwDeviceID, dwAPIVersion,
            debugstr_w(lpszAddressIn), dwCard, dwTranslateOptions, lpTranslateOutput);
    return 0;
}

/***********************************************************************
 *		lineTranslateDialog (TAPI32.@)
 */
DWORD WINAPI lineTranslateDialogA(HLINEAPP hLineApp, DWORD dwDeviceID, DWORD dwAPIVersion, HWND hwndOwner, LPCSTR lpszAddressIn)
{
    FIXME("(%p, %08lx, %08lx, %p, %s): stub.\n", hLineApp, dwDeviceID, dwAPIVersion, hwndOwner, lpszAddressIn);
    return 0;
}

/***********************************************************************
 *              lineTranslateDialogW (TAPI32.@)
 */
DWORD WINAPI lineTranslateDialogW(HLINEAPP hLineApp, DWORD dwDeviceID,
        DWORD dwAPIVersion, HWND hwndOwner, LPCWSTR lpszAddressIn)
{
    FIXME("(%p, %08lx, %08lx, %p, %s): stub.\n", hLineApp, dwDeviceID,
            dwAPIVersion, hwndOwner, debugstr_w(lpszAddressIn));
    return 0;
}

/***********************************************************************
 *		lineUncompleteCall (TAPI32.@)
 */
DWORD WINAPI lineUncompleteCall(HLINE hLine, DWORD dwCompletionID)
{
    FIXME("(%p, %08lx): stub.\n", hLine, dwCompletionID);
    return 1;
}

/***********************************************************************
 *		lineUnhold (TAPI32.@)
 */
DWORD WINAPI lineUnhold(HCALL hCall)
{
    FIXME("(%p): stub.\n", hCall);
    return 1;
}

/***********************************************************************
 *		lineUnpark (TAPI32.@)
 */
DWORD WINAPI lineUnparkA(HLINE hLine, DWORD dwAddressID, LPHCALL lphCall, LPCSTR lpszDestAddress)
{
    FIXME("(%p, %08lx, %p, %s): stub.\n", hLine, dwAddressID, lphCall, lpszDestAddress);
    return 1;
}
