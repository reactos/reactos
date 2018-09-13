/*      File: D:\WACKER\cncttapi\enum.c (Created: 23-Mar-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 7 $
 *	$Date: 9/24/99 5:01p $
 */

#define TAPI_CURRENT_VERSION 0x00010004     // cab:11/14/96 - required!

#include <tapi.h>
#pragma hdrstop

#include <time.h>
#include <string.h>

#include <tdll\stdtyp.h>
#include <tdll\session.h>
#include <tdll\tdll.h>
#include <tdll\mc.h>
#include <tdll\assert.h>
#include <tdll\errorbox.h>
#include <tdll\cnct.h>
#include <tdll\hlptable.h>
#include <tdll\globals.h>
#include <tdll\com.h>
#include <term\res.h>

#include "cncttapi.hh"
#include "cncttapi.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	EnumerateTapiLocations
 *
 * DESCRIPTION:
 *	Enumerates tapi locations and puts them in the given combo box.
 *
 * ARGUMENTS:
 *	hhDriver	- private driver handle
 *	hwndCB		- window handle of combo box
 *	hwndTB		- calling card text window
 *
 * RETURNS:
 *	0 on success, else error
 *
 */
int EnumerateTapiLocations(const HHDRIVER hhDriver, const HWND hwndCB,
    					   const HWND hwndTB)
    {
    DWORD i, dwSize;
    LRESULT lr;
    TCHAR *pach = NULL;
    TCHAR ach[256];
    DWORD dwPreferredCardID = (DWORD)-1;
    DWORD dwCountryID = 1;
    LINETRANSLATECAPS *pLnTransCap = NULL;
    LINELOCATIONENTRY *pLnLocEntry = NULL;
    LINECARDENTRY *pLnCardEntry = NULL;

    /* --- Enumerate locations --- */

    if ((pLnTransCap = malloc(sizeof(LINETRANSLATECAPS))) == 0)
    	{
    	assert(FALSE);
    	return -2;
    	}

    dwSize = 0; // used in this loop to call the dialog only once.

    do	{
    	memset(pLnTransCap, 0, sizeof(LINETRANSLATECAPS)); //* temp
    	pLnTransCap->dwTotalSize = sizeof(LINETRANSLATECAPS); //* temp

    	if ((i = TRAP(lineGetTranslateCaps(hhDriver->hLineApp, TAPI_VER,
    			pLnTransCap))) != 0)
    		{
    		if ( i == LINEERR_INIFILECORRUPT )
    			{
    			// Unfortunately, lineTranslateDialog does not return
    			// a failure code if the user clicks cancel.  So if
    			// we fail the second time on lineGetTranslateCaps()
    			// don't bother to do anything else.
    			//
    			if (dwSize == 0)
    				{
    				LoadString(glblQueryDllHinst(), IDS_ER_TAPI_NEEDS_INFO,
    					ach, sizeof(ach));

    				TimedMessageBox(sessQueryHwnd(hhDriver->hSession), ach,
    					0, MB_OK | MB_ICONINFORMATION, 0);

    				return -3;
    				}

    			if (TRAP(lineTranslateDialog(hhDriver->hLineApp, 0,
    					TAPI_VER, sessQueryHwnd(hhDriver->hSession), 0))
    						== 0)
    				{
    				dwSize = 1;
    				continue;
    				}
    			}

    		free(pLnTransCap);
  		    pLnTransCap = NULL;
    		return -4;
    		}
    	}
    while (i);	// end of do.

    if (pLnTransCap->dwNeededSize > pLnTransCap->dwTotalSize)
    	{
    	dwSize = pLnTransCap->dwNeededSize;
    	free(pLnTransCap);
  	    pLnTransCap = NULL;

    	if ((pLnTransCap = malloc(dwSize)) == 0)
    		{
    		assert(FALSE);
    		return -5;
    		}

    	pLnTransCap->dwTotalSize = dwSize;

    	if (TRAP(lineGetTranslateCaps(hhDriver->hLineApp, TAPI_VER,
    			pLnTransCap)) != 0)
    		{
            free(pLnTransCap);
  		    pLnTransCap = NULL;
    		return -6;
    		}
    	}

    /* --- Clear combo box --- */

    if (IsWindow(hwndCB))
    	SendMessage(hwndCB, CB_RESETCONTENT, 0, 0);

    /* --- Setup pointer to entry structure and enumerate --- */

    pLnLocEntry = (LINELOCATIONENTRY *)
    	((LPSTR)pLnTransCap + pLnTransCap->dwLocationListOffset);

    for (i = 0 ; i < pLnTransCap->dwNumLocations ; ++i)
    	{
    	if (pLnLocEntry->dwLocationNameSize == 0)
    		continue;

    	pach = (LPSTR)pLnTransCap + pLnLocEntry->dwLocationNameOffset;
        if (pLnLocEntry->dwLocationNameSize)   		
            MemCopy(ach, pach, pLnLocEntry->dwLocationNameSize);
    	ach[pLnLocEntry->dwLocationNameSize] = TEXT('\0');

    	if (IsWindow(hwndCB))
    		{
    		lr = SendMessage(hwndCB, CB_ADDSTRING, 0, (LPARAM)ach);

    		if (lr != CB_ERR)
    			{
    			SendMessage(hwndCB, CB_SETITEMDATA, (WPARAM)lr,
    				(LPARAM)pLnLocEntry->dwPermanentLocationID);
    			}

    		else
    			{
    			assert(FALSE);
    			}
    		}

    	// Make sure we have a default by setting the first valid entry
    	// we ecounter to the default.	Later in the enumeration, if we
    	// encounter another ID as the default, we can reset it.

    	if (pLnLocEntry->dwPermanentLocationID ==
    			pLnTransCap->dwCurrentLocationID
    				|| dwPreferredCardID == (DWORD)-1)
    		{
    		dwPreferredCardID = pLnLocEntry->dwPreferredCardID;

    		if (hhDriver->dwCountryID == (DWORD)-1)
    			dwCountryID = pLnLocEntry->dwCountryID;

    		/* --- Get default location area code if not specified --- */

    		if (pLnLocEntry->dwCityCodeSize)
    			{
    			pach = (LPSTR)pLnTransCap +
    				pLnLocEntry->dwCityCodeOffset;

   				if (pLnLocEntry->dwCityCodeSize)
                    MemCopy(hhDriver->achDefaultAreaCode, pach, pLnLocEntry->dwCityCodeSize);

    			hhDriver->achDefaultAreaCode[pLnLocEntry->dwCityCodeSize] =
    				TEXT('\0');
    			}
    		}

    	pLnLocEntry += 1;
    	}

    // If we don't have a country code loaded for this session, then
    // use the country code of the current location.
    //
    if (hhDriver->dwCountryID == (DWORD)-1)
    	hhDriver->dwCountryID = dwCountryID;

    /* --- Select the default location --- */
    	
    if (IsWindow(hwndCB))
    	{
    	// mrw,2/13/95 - changed so that selection is made by quering
    	// the combo box rather than saving the index which proved
    	// unreliable.
    	//
    	for (i = 0 ; i < pLnTransCap->dwNumLocations ; ++i)
    		{
    		lr = SendMessage(hwndCB, CB_GETITEMDATA, (WPARAM)i, 0);

    		if (lr != CB_ERR)
    			{
    			if ((DWORD)lr == pLnTransCap->dwCurrentLocationID)
    				SendMessage(hwndCB, CB_SETCURSEL, i, 0);
    			}
    		}
    	}

    /* --- Now find the card entry --- */

    if (dwPreferredCardID != (DWORD)-1)
    	{
    	pLnCardEntry = (LINECARDENTRY *)
    		((LPSTR)pLnTransCap + pLnTransCap->dwCardListOffset);

    	for (i = 0 ; i < pLnTransCap->dwNumCards ; ++i)
    		{
    		if (pLnCardEntry->dwPermanentCardID == dwPreferredCardID)
    			{
    			if (pLnCardEntry->dwCardNameSize == 0)
    				break;

    			pach = (LPSTR)pLnTransCap + pLnCardEntry->dwCardNameOffset;
   				if (pLnCardEntry->dwCardNameSize)
                    MemCopy(ach, pach, pLnCardEntry->dwCardNameSize);
    			ach[pLnCardEntry->dwCardNameSize] = TEXT('\0');

    			if (IsWindow(hwndTB))
    				SetWindowText(hwndTB, ach);

    			break;
    			}

    		pLnCardEntry += 1;
    		}
    	}

    free(pLnTransCap);
    pLnTransCap = NULL;
    return 0;
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	EnumerateCountryCodes
 *
 * DESCRIPTION:
 *	Enumerates available country codes.
 *
 * ARGUMENTS:
 *	hhDriver	- private driver handle
 *	hwndCB		- combobox to fill
 *
 * RETURNS:
 *	0=success, else error code.
 *
 */
int EnumerateCountryCodes(const HHDRIVER hhDriver, const HWND hwndCB)
    {
    int iIdx;
    DWORD dw;
    DWORD dwID;
    DWORD dwSize;
    TCHAR ach[100];
    LPLINECOUNTRYLIST pcl = 0;
    LPLINECOUNTRYENTRY pce;

    if (hhDriver == 0)
        goto ERROR_EXIT;

    /* --- Usual junk to make a TAPI call --- */

    if ((pcl = (LPLINECOUNTRYLIST)malloc(sizeof(LINECOUNTRYLIST))) == 0)
        {
        assert(0);
        goto ERROR_EXIT;
        }

    pcl->dwTotalSize = sizeof(LINECOUNTRYLIST);

    // Get the country list all at once.
    //
    if (lineGetCountry(0, TAPI_VER, pcl) != 0)
        {
        assert(0);
        goto ERROR_EXIT;
        }

    if (pcl->dwNeededSize > pcl->dwTotalSize)
        {
        dwSize = pcl->dwNeededSize;
        free(pcl);
  	    pcl = NULL;

        if ((pcl = (LPLINECOUNTRYLIST)malloc(dwSize)) == 0)
            {
            assert(0);
            goto ERROR_EXIT;
            }

        pcl->dwTotalSize = dwSize;

        if (lineGetCountry(0, TAPI_VER, pcl) != 0)
            {
            assert(0);
            goto ERROR_EXIT;
            }
        }

    // Empty contents of combo box.
    //
    if (hwndCB)
        SendMessage(hwndCB, CB_RESETCONTENT, 0, 0);

    // Country List array starts here...
    //
    pce = (LPLINECOUNTRYENTRY)((BYTE *)pcl + pcl->dwCountryListOffset);

    // Loop thru list of countries and insert into combo box.
    //
    for (dw = 0 ; dw < pcl->dwNumCountries ; ++dw, ++pce)
        {
        // Format so country name is first.
        //
        wsprintf(ach, "%s (%d)", (BYTE *)pcl + pce->dwCountryNameOffset,
            pce->dwCountryCode);

        // Add to combo box
        //
    	iIdx = (int)SendMessage(hwndCB, CB_ADDSTRING, 0, (LPARAM)ach);

        if (iIdx != CB_ERR)
            {
        	SendMessage(hwndCB, CB_SETITEMDATA, (WPARAM)iIdx,
    			    (LPARAM)pce->dwCountryID);
            }
        }

    // Find the current ID and select it.
    //
    for (dw = 0 ; dw < pcl->dwNumCountries ; ++dw)
        {
    	dwID = (DWORD)SendMessage(hwndCB, CB_GETITEMDATA, (WPARAM)dw, 0);

        if (dwID == hhDriver->dwCountryID)
            {
            SendMessage(hwndCB, CB_SETCURSEL, (WPARAM)dw, 0);
            break;
            }
        }

    // Clean up and exit
    //
    free(pcl);
    pcl = NULL;
    return 0;

    /*==========*/
ERROR_EXIT:
    /*==========*/
    if (pcl)
        {
        free(pcl);
  	    pcl = NULL;
        }

    return -1;
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	EnumerateAreaCodes
 *
 * DESCRIPTION:
 *	Lists last 10 area codes used.
 *
 * ARGUMENTS:
 *	hhDriver	- private driver handle
 *	hwndCB		- combobox to fill
 *
 * RETURNS:
 *	0=success, else error.
 *
 */
int EnumerateAreaCodes(const HHDRIVER hhDriver, const HWND hwndCB)
    {
    if (hhDriver == 0)
    	{
    	assert(FALSE);
    	return -1;
    	}

    if (hhDriver->achAreaCode[0] == TEXT('\0'))
    	lstrcpy(hhDriver->achAreaCode, hhDriver->achDefaultAreaCode);

    SetWindowText(hwndCB, hhDriver->achAreaCode);
    return 0;
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	EnumerateLines
 *
 * DESCRIPTION:
 *	Enumerates available lines.  If hwndCB is non-zero, loads names.
 *
 * ARGUMENTS:
 *	hhDriver	- private driver handle
 *	hwndCB		- combo box
 *
 * RETURNS:
 *	0=success, -1=error
 *
 */
int EnumerateLines(const HHDRIVER hhDriver, const HWND hwndCB)
    {
    int fHotPhone;
    DWORD i, dwSize;
    DWORD dwAPIVersion;
    LINEEXTENSIONID LnExtId;
    LPLINEDEVCAPS pLnDevCaps;
    PSTLINEIDS	pstLineIds;
    TCHAR *pachLine;
    TCHAR  achLine[256];
    TCHAR  ach[256];
    LRESULT lr;

    // This call knows to free the itemdata associated with this combo.
    //
    ResetComboBox(hwndCB);

    /* --- Initialize stuff --- */

    pLnDevCaps = 0;
    hhDriver->dwLine = (DWORD)-1;
    hhDriver->fMatchedPermanentLineID = FALSE;

    /* --- Enumerate the devices --- */

    for (i = 0 ; i < hhDriver->dwLineCnt ; ++i)
    	{
    	if (lineNegotiateAPIVersion(hhDriver->hLineApp, i, TAPI_VER,
    			TAPI_VER, &dwAPIVersion, &LnExtId) != 0)
    		{
            // Could be a 1.3 driver, we continue.
    		continue;
    		}

    	if (CheckHotPhone(hhDriver, i, &fHotPhone) == 0)
    		{
    		if (fHotPhone)
    			continue;
    		}

    	if ((pLnDevCaps = malloc(sizeof(LINEDEVCAPS))) == 0)
    		{
    		assert(0);
    		continue;
    		}

    	// TAPI says its too small if we just allocate sizeof(LINEDEVCAPS)
    	//
    	pLnDevCaps->dwTotalSize = sizeof(LINEDEVCAPS);

    	/* --- Make call to find out how much we need for this device --- */

    	if (TRAP(lineGetDevCaps(hhDriver->hLineApp, i, dwAPIVersion, 0,
    			pLnDevCaps)) != 0)
    		{
    		assert(0);
    		continue;
    		}

    	/* --- Find out how big structure really needs to be --- */

    	if (pLnDevCaps->dwNeededSize > pLnDevCaps->dwTotalSize)
    		{
    		dwSize = pLnDevCaps->dwNeededSize;
    		free(pLnDevCaps);
  		    pLnDevCaps = NULL;

    		pLnDevCaps = malloc(dwSize);

    		if (pLnDevCaps == 0)
    			{
    			assert(FALSE);
    			continue;
    			}

    		pLnDevCaps->dwTotalSize = dwSize;

    		/* --- Try again --- */

    		if (lineGetDevCaps(hhDriver->hLineApp, i, dwAPIVersion, 0,
    				pLnDevCaps))
    			{
    			assert(FALSE);
    			continue;
    			}
    		}

    	/* --- Check the information we're interested in --- */
        //mpt:03-19-98 added a MaxRate check to eliminate the MS VPN adapter
        //             from the list of available devices.
        if (pLnDevCaps->dwLineNameSize == 0 || pLnDevCaps->dwMaxRate == 0)
    		{
    		free(pLnDevCaps);
  		    pLnDevCaps = NULL;
    		continue;
    		}

    	pachLine = (BYTE *)pLnDevCaps + pLnDevCaps->dwLineNameOffset;
   		if (pLnDevCaps->dwLineNameSize)
            MemCopy(achLine, pachLine, pLnDevCaps->dwLineNameSize);
    	achLine[pLnDevCaps->dwLineNameSize] = TEXT('\0');

    	/* --- Put name in combo box if given one --- */

    	if (IsWindow(hwndCB))
    		{
    		// I need to associate two pieces of data with each
    		// item (permanent line id and relative line id).  Both
    		// are double words and CB_SETITEMDATA only stores a
    		// a double word.  So malloc a structure to hold both
    		// ids and store a pointer to the memory in the combobox.
    		// Call the ResetComboBox() defined in the file to reset
    		// the contents of the combobox and free the associated
    		// memory.  ResetComboBox() is also called in the dialog
    		// destroy.
    		//
    		pstLineIds = malloc(sizeof(*pstLineIds));

    		if (pstLineIds == 0)
    			{
    			assert(FALSE);
    			free(pLnDevCaps);
                pLnDevCaps = NULL;
    			continue;
    			}

    		pstLineIds->dwLineId = i;
    		pstLineIds->dwPermanentLineId = pLnDevCaps->dwPermanentLineID;

    		// Add the name to the combobox.  Since names are sorted,
    		// the index of the item is returned from SendMessage and
    		// stored in lr.  Save this index for use below.
    		//
    		lr = SendMessage(hwndCB, CB_ADDSTRING, 0, (LPARAM)achLine);

    		if (lr != CB_ERR)
    			{
    			// Note: lr was set above CB_ADDSTRING call.
    			//
    			if (SendMessage(hwndCB, CB_SETITEMDATA, (WPARAM)lr,
    					(LPARAM)pstLineIds) == CB_ERR)
    				{
    				assert(FALSE);
    				free(pstLineIds);
    				free(pLnDevCaps);
  				    pstLineIds = NULL;
  				    pLnDevCaps = NULL;
    				continue;
    				}
    			}

    		else
    			{
    			free(pstLineIds);
    			free(pLnDevCaps);
  			    pstLineIds = NULL;
  			    pLnDevCaps = NULL;
    			continue;
    			}
    		}

    	if (pLnDevCaps->dwPermanentLineID == hhDriver->dwPermanentLineId ||
    			hhDriver->dwLine == (DWORD)-1)
    		{
    		hhDriver->dwLine = i;
    		hhDriver->dwAPIVersion = dwAPIVersion;
    		lstrcpy(hhDriver->achLineName, achLine);

    		if (IsWindow(hwndCB))
    			{
    			// Note: lr was set above CB_ADDSTRING call.
    			//
    			SendMessage(hwndCB, CB_SETCURSEL, (WPARAM)lr, 0);
    			}

    		if (pLnDevCaps->dwPermanentLineID == hhDriver->dwPermanentLineId)
    			hhDriver->fMatchedPermanentLineID = TRUE;
    		}

    	/* --- Free allocated space --- */

    	free(pLnDevCaps);
  	    pLnDevCaps = NULL;
    	}

    // Load the direct to com port stuff first

    if (LoadString(glblQueryDllHinst(), IDS_CNCT_DIRECTCOM, achLine,
    	    sizeof(achLine)) == 0)
        {
        assert(FALSE);
        // The loading of the string has failed from the resource, so
        // add the non-localized string here (I don't believe this string
        // is ever translated). REV 8/13/99
        //
        lstrcpy(ach, TEXT("Com%d"));
        //return -1;
        }

    // Another nasty bug, DIRECT_COM4 is defined as 0x5A2175d4, which
    // makes this one heck of a loop. I think we only want to do this
    // four times (as opposed to 1.5 billion). - cab:11/14/96
    //
    // for (i = 0 ; i < DIRECT_COM4 ; ++i)
    //
    for( i = 0; i < 4; i++ )
        {
    	wsprintf(ach, achLine, i+1);

    	if (IsWindow(hwndCB))
            {
    	    lr = SendMessage(hwndCB, CB_INSERTSTRING, (WPARAM)-1,
    		    (LPARAM)ach);

    	    pstLineIds = malloc(sizeof(*pstLineIds));

    	    if (pstLineIds == 0)
    		    {
    		    assert(FALSE);
    		    continue;
    		    }

    	    // We don't use a line id here, only a permanent line id.
    	    //
    	    pstLineIds->dwPermanentLineId = DIRECT_COM1+i;

    		// Note: lr was set above CB_INSERTSTRING call.
    	    //
    	    if (SendMessage(hwndCB, CB_SETITEMDATA, (WPARAM)lr,
    			    (LPARAM)pstLineIds) == CB_ERR)
    		    {
    		    assert(FALSE);
    		    free(pstLineIds);
                pstLineIds = NULL;
    		    continue;
    		    }
            }

    	// If this is what was saved in the data file, then set
    	// the line ids.
        //
    	if ((DIRECT_COM1+i) == hhDriver->dwPermanentLineId ||
    			hhDriver->dwLine == (DWORD)-1)
    		{
    		hhDriver->dwLine = 0;
    		lstrcpy(hhDriver->achLineName, ach);

    		if (IsWindow(hwndCB))
    			{
    			// Note: lr was set above CB_ADDSTRING call.
    			//
    			SendMessage(hwndCB, CB_SETCURSEL, (WPARAM)lr, 0);
    			}

    		if ((DIRECT_COM1+i) == hhDriver->dwPermanentLineId)
    			hhDriver->fMatchedPermanentLineID = TRUE;
    		}
    	}

#if defined(INCL_WINSOCK)
    // This is causing a syntax error, so I am fixing it. Why nobody
    // found this sooner, I have no idea. - cab:11/14/96
    //
    //if (LoadString(glblQueryDllHinst(), IDS_WINSOCK_SETTINGS_STR, ach,
    //    sizeof(ach));
    //
    if (LoadString(glblQueryDllHinst(), IDS_WINSOCK_SETTINGS_STR, ach,
            sizeof(ach)) == 0)
        {
        assert(FALSE);
        // The loading of the string has failed from the resource, so
        // add the non-localized string here (I don't believe this string
        // is ever translated). REV 8/13/99
        //
        lstrcpy(ach, TEXT("TCP/IP (Winsock)"));
        //return -1;
        }

    if (IsWindow(hwndCB))
        {
    	lr = SendMessage(hwndCB, CB_INSERTSTRING, (WPARAM)-1,
    		(LPARAM)ach);

    	pstLineIds = malloc(sizeof(*pstLineIds));

    	if (pstLineIds == 0)
    		{
    		assert(FALSE);
    		free(pstLineIds);
  		    pstLineIds = NULL;
    		return 0;
    		}

    	// We don't use a line id here, only a permanent line id.
    	//
    	pstLineIds->dwPermanentLineId = DIRECT_COMWINSOCK;

    	// Note: lr was set above CB_INSERTSTRING call.
    	//
    	if (SendMessage(hwndCB, CB_SETITEMDATA, (WPARAM)lr,
    			(LPARAM)pstLineIds) == CB_ERR)
    		{
    		assert(FALSE);
    		}
        }

    // Check to see if the current connection is winsock. - cab:11/15/96
    //
    if (DIRECT_COMWINSOCK == hhDriver->dwPermanentLineId ||
    		hhDriver->dwLine == (DWORD)-1)
    	{
    	hhDriver->dwLine = 0;
    	lstrcpy(hhDriver->achLineName, ach);

    	if (IsWindow(hwndCB))
    		{
    		// Note: lr was set above CB_INSERTSTRING call.
    		//
    		SendMessage(hwndCB, CB_SETCURSEL, (WPARAM)lr, 0);
    		}

    	if (DIRECT_COMWINSOCK == hhDriver->dwPermanentLineId)
            {
    		hhDriver->fMatchedPermanentLineID = TRUE;
            }

        // Don't free the pstLineIds since it will be freed in the
  	    // ResetComboBox() function.  We were previously freeing the
  	    // memory twice causing a crash with the MSVC 6.0 runtime DLL's.
  	    // I'm suprised this did not present itself earlier. REV 8/17/98
  	    //
        //free(pstLineIds);
        }
#endif

    return 0;
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	EnumerateLinesNT
 *
 * DESCRIPTION:
 *	Enumerates available lines.  This function is similar to EnumerateLines, but
 *  we use different methods to enumerate the ports under Windows NT.
 *
 * ARGUMENTS:
 *	hhDriver	- private driver handle
 *	hwndCB		- combo box
 *
 * RETURNS:
 *	0=success, -1=error
 *
 */
int EnumerateLinesNT(const HHDRIVER hhDriver, const HWND hwndCB)
    {
    int fHotPhone;
    int nTest;
    DWORD i, dwSize;
    DWORD dwAPIVersion;
    LINEEXTENSIONID LnExtId;
    LPLINEDEVCAPS pLnDevCaps;
    PSTLINEIDS	pstLineIds;
    TCHAR *pachLine;
    TCHAR  achLine[256];
    TCHAR  ach[256];
    LRESULT lr;
    HKEY hKey;
    LONG retval;
    DWORD iMaxComPortEnum = 256;

    // This call knows to free the itemdata associated with this combo.
    //
    ResetComboBox(hwndCB);

    /* --- Initialize stuff --- */

    pLnDevCaps = 0;
    if ( lstrcmp(hhDriver->achLineName, "") == 0 )
        {
        hhDriver->dwLine = (DWORD)-1;
        }
    else
        {
        hhDriver->dwLine = 0;
        }

    hhDriver->fMatchedPermanentLineID = FALSE;

    /* --- Enumerate the devices --- */

    for (i = 0 ; i < hhDriver->dwLineCnt ; ++i)
    	{
    	if (retval = lineNegotiateAPIVersion(hhDriver->hLineApp, i, TAPI_VER,
    			TAPI_VER, &dwAPIVersion, &LnExtId) != 0)
    		{
            // Could be a 1.3 driver, we continue.
    		continue;
    		}

        if (CheckHotPhone(hhDriver, i, &fHotPhone) == 0)
    		{
    		if (fHotPhone)
    			continue;
    		}

    	if ((pLnDevCaps = malloc(sizeof(LINEDEVCAPS))) == 0)
    		{
    		assert(0);
    		continue;
    		}

    	// TAPI says its too small if we just allocate sizeof(LINEDEVCAPS)
    	//
    	pLnDevCaps->dwTotalSize = sizeof(LINEDEVCAPS);

    	/* --- Make call to find out how much we need for this device --- */

    	if (TRAP(lineGetDevCaps(hhDriver->hLineApp, i, dwAPIVersion, 0,
    			pLnDevCaps)) != 0)
    		{
    		assert(0);
    		continue;
    		}

    	/* --- Find out how big structure really needs to be --- */

    	if (pLnDevCaps->dwNeededSize > pLnDevCaps->dwTotalSize)
    		{
    		dwSize = pLnDevCaps->dwNeededSize;
    		free(pLnDevCaps);
  		    pLnDevCaps = NULL;

    		pLnDevCaps = malloc(dwSize);

    		if (pLnDevCaps == 0)
    			{
    			assert(FALSE);
    			continue;
    			}

    		pLnDevCaps->dwTotalSize = dwSize;

    		/* --- Try again --- */

    		if (lineGetDevCaps(hhDriver->hLineApp, i, dwAPIVersion, 0,
    				pLnDevCaps))
    			{
    			assert(FALSE);
    			continue;
    			}
    		}

    	/* --- Check the information we're interested in --- */

        //mpt:03-19-98 added a MaxRate check to eliminate the MS VPN adapter
        //             from the list of available devices.
    	//mpt 06-23-98 added a MaxNumActiveCalls check to eliminate the
    	//             H323 and Line0 devices from showing up in our list
        if (pLnDevCaps->dwLineNameSize == 0 ||
    		(pLnDevCaps->dwMaxRate == 0 || pLnDevCaps->dwMaxRate >= 1048576) ||
    		(pLnDevCaps->dwMaxNumActiveCalls > 1 && pLnDevCaps->dwMaxNumActiveCalls != 32768) )
    		{
    		free(pLnDevCaps);
  		    pLnDevCaps = NULL;
    		continue;
    		}

    	pachLine = (BYTE *)pLnDevCaps + pLnDevCaps->dwLineNameOffset;
   		if (pLnDevCaps->dwLineNameSize)
            MemCopy(achLine, pachLine, pLnDevCaps->dwLineNameSize);
    	
        achLine[pLnDevCaps->dwLineNameSize] = TEXT('\0');

    	/* --- Put name in combo box if given one --- */

    	if (IsWindow(hwndCB))
    		{
    		// I need to associate two pieces of data with each
    		// item (permanent line id and relative line id).  Both
    		// are double words and CB_SETITEMDATA only stores a
    		// a double word.  So malloc a structure to hold both
    		// ids and store a pointer to the memory in the combobox.
    		// Call the ResetComboBox() defined in the file to reset
    		// the contents of the combobox and free the associated
    		// memory.  ResetComboBox() is also called in the dialog
    		// destroy.
    		//
    		pstLineIds = malloc(sizeof(*pstLineIds));

    		if (pstLineIds == 0)
    			{
    			assert(FALSE);
    			free(pLnDevCaps);
  			    pLnDevCaps = NULL;
    			continue;
    			}

    		pstLineIds->dwLineId = i;
    		pstLineIds->dwPermanentLineId = pLnDevCaps->dwPermanentLineID;

    		// Add the name to the combobox.  Since names are sorted,
    		// the index of the item is returned from SendMessage and
    		// stored in lr.  Save this index for use below.
    		//
    		lr = SendMessage(hwndCB, CB_ADDSTRING, 0, (LPARAM)achLine);

    		if (lr != CB_ERR)
    			{
    			// Note: lr was set above CB_ADDSTRING call.
    			//
    			if (SendMessage(hwndCB, CB_SETITEMDATA, (WPARAM)lr,
    					(LPARAM)pstLineIds) == CB_ERR)
    				{
    				assert(FALSE);
    				free(pstLineIds);
    				free(pLnDevCaps);
  				    pstLineIds = NULL;
  				    pLnDevCaps = NULL;
    				continue;
    				}
    			}

    		else
    			{
    			free(pstLineIds);
    			free(pLnDevCaps);
  			    pstLineIds = NULL;
  			    pLnDevCaps = NULL;
    			continue;
    			}
    		}

    	if (pLnDevCaps->dwPermanentLineID == hhDriver->dwPermanentLineId ||
    			hhDriver->dwLine == (DWORD)-1)
    		{
    		hhDriver->dwLine = i;
    		hhDriver->dwAPIVersion = dwAPIVersion;
    		lstrcpy(hhDriver->achLineName, achLine);

    		if (IsWindow(hwndCB))
    			{
    			// Note: lr was set above CB_ADDSTRING call.
    			//
    			SendMessage(hwndCB, CB_SETCURSEL, (WPARAM)lr, 0);
    			}

    		if (pLnDevCaps->dwPermanentLineID == hhDriver->dwPermanentLineId)
    			hhDriver->fMatchedPermanentLineID = TRUE;
    		}

    	/* --- Free allocated space --- */

    	free(pLnDevCaps);
  	    pLnDevCaps = NULL;
    	}

    // Load the direct to com port stuff first

    if (RegOpenKey(HKEY_LOCAL_MACHINE,
    	TEXT("hardware\\devicemap\\serialcomm"), &hKey) != ERROR_SUCCESS)
    	{
    	assert(FALSE);
        // We used to return FALSE here which would mean the TCP/IP
        // would not be in the enumerated connection methods (modem,
        // COM port or Winsock) in the "Connect Using:" dropdown
        // combobox in the properties for the entries.  Since the
        // return value was never checked, we can just continue on
        // to finish the enumerations for the combobox.  Now we just
        // set the number of COM ports to enumerate to 0. REV 8/13/99.
        //
        //return FALSE;
        iMaxComPortEnum = 0;
    	}

    // We now use a variable for the number of drives to enumerate.
    // We have set the number of COM ports to enumerate in a variable
    // above (iMaxComPortEnum == 0 if no COM ports installed). REV 8/13/99.
    //
    for (i = 0 ; i < iMaxComPortEnum ; ++i)
        {
        BYTE ab[256];
        DWORD dwType;
        DWORD dwSizeBuf = sizeof(ab);
        dwSize = sizeof(ach);

        // Enumerate devices under our serialcomm key
        //
        if (RegEnumValue(hKey, i, ach, &dwSize, 0, &dwType, ab,
            &dwSizeBuf) != ERROR_SUCCESS)
            {
            break;
            }

        // Ignore anything that isn't a string.
        //
        if (dwType != REG_SZ)
            continue;

        lstrcpy(ach, ab);

    	if (IsWindow(hwndCB))
            {
    	    lr = SendMessage(hwndCB, CB_INSERTSTRING, (WPARAM)-1,
    		    (LPARAM)ach);

    	    pstLineIds = malloc(sizeof(*pstLineIds));

    	    if (pstLineIds == 0)
    		    {
    		    assert(FALSE);
    		    continue;
    		    }

    	    // We don't use a line id here, only a permanent line id.
    	    //
            pstLineIds->dwPermanentLineId = DIRECT_COM_DEVICE;

    	    // Note: lr was set above CB_INSERTSTRING call.
    	    //
    	    if (SendMessage(hwndCB, CB_SETITEMDATA, (WPARAM)lr,
    			    (LPARAM)pstLineIds) == CB_ERR)
    		    {
    		    assert(FALSE);
    		    free(pstLineIds);
                pstLineIds = NULL;
    		    continue;
    		    }
            }


        if (hhDriver->fMatchedPermanentLineID == FALSE &&
    	    lstrcmp(hhDriver->achComDeviceName, ab) == 0 || 
            hhDriver->dwLine == (DWORD)-1 )
            {
    		hhDriver->dwLine = 0;
    		lstrcpy(hhDriver->achLineName, ab);

    		if (IsWindow(hwndCB))
    			{
    			// Note: lr was set above CB_ADDSTRING call.
    			//
    			SendMessage(hwndCB, CB_SETCURSEL, (WPARAM)lr, 0);
    			}

    		hhDriver->fMatchedPermanentLineID = TRUE;
            }
    	}

#if defined(INCL_WINSOCK)
    // This is causing a syntax error, so I am fixing it. Why nobody
    // found this sooner, I have no idea. - cab:11/14/96
    //
    //if (LoadString(glblQueryDllHinst(), IDS_WINSOCK_SETTINGS_STR, ach,
    //    sizeof(ach));
    //
    if (LoadString(glblQueryDllHinst(), IDS_WINSOCK_SETTINGS_STR, ach,
            sizeof(ach)) == 0)
        {
        assert(FALSE);
        // The loading of the string has failed from the resource, so
        // add the non-localized string here (I don't believe this string
        // is ever translated). REV 8/13/99
        //
        lstrcpy(ach, TEXT("TCP/IP (Winsock)"));
        //return -1;
        }

    if (IsWindow(hwndCB))
        {
    	lr = SendMessage(hwndCB, CB_INSERTSTRING, (WPARAM)-1,
    		(LPARAM)ach);

    	pstLineIds = malloc(sizeof(*pstLineIds));

    	if (pstLineIds == 0)
    		{
    		assert(FALSE);
    		free(pstLineIds);
            pstLineIds = NULL;
    		return 0;
    		}

        // We don't use a line id here, only a permanent line id.
        //
        pstLineIds->dwPermanentLineId = DIRECT_COMWINSOCK;

        // Note: lr was set above CB_INSERTSTRING call.
        //
        if (SendMessage(hwndCB, CB_SETITEMDATA, (WPARAM)lr,
    		    (LPARAM)pstLineIds) == CB_ERR)
    	    {
    	    assert(FALSE);
    	    }
        }


    // Check to see if the current connection is winsock. - cab:11/15/96
    //
    if (DIRECT_COMWINSOCK == hhDriver->dwPermanentLineId ||
    		hhDriver->dwLine == (DWORD)-1)
    	{
    	hhDriver->dwLine = 0;
    	lstrcpy(hhDriver->achLineName, ach);

    	if (IsWindow(hwndCB))
    		{
    		// Note: lr was set above CB_INSERTSTRING call.
    		//
    		SendMessage(hwndCB, CB_SETCURSEL, (WPARAM)lr, 0);
    		}

    	if (DIRECT_COMWINSOCK == hhDriver->dwPermanentLineId)
            {
    		hhDriver->fMatchedPermanentLineID = TRUE;
            }

        // Don't free the pstLineIds since it will be freed in the
  	    // ResetComboBox() function.  We were previously freeing the
  	    // memory twice causing a crash with the MSVC 6.0 runtime DLL's.
  	    // I'm suprised this did not present itself earlier. REV 8/17/98
  	    //
  	    //free(pstLineIds);
        }
#endif

    return 0;
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	DoLineGetCountry
 *
 * DESCRIPTION:
 *	Wrapper indended to query for a single country.  The caller must
 *	free the pcl when finished.
 *
 * ARGUMENTS:
 *	dwCountryID - ID of country
 *	dwApiVersion - Api version (no longer used)
 *	ppcl		- pointer to a LPLINECOUNTRYLIST
 *
 * RETURNS:
 *	0=OK
 *
 */
int DoLineGetCountry(const DWORD dwCountryID, const DWORD dwAPIVersion,
        LPLINECOUNTRYLIST *ppcl)
    {
    DWORD dwSize;
    LPLINECOUNTRYLIST pcl;

    if ((pcl = malloc(sizeof(LINECOUNTRYLIST))) == 0)
    	{
    	assert(FALSE);
    	return -1;
    	}

    pcl->dwTotalSize = sizeof(LINECOUNTRYLIST);

    if (lineGetCountry(dwCountryID, TAPI_VER, pcl) != 0)
    	{
    	assert(FALSE);
    	free(pcl);
  	    pcl = NULL;
    	return -1;
    	}

    if (pcl->dwNeededSize > pcl->dwTotalSize)
    	{
    	dwSize = pcl->dwNeededSize;
    	free(pcl);
  	    pcl = NULL;

    	if ((pcl = malloc(dwSize)) == 0)
    		{
    		assert(FALSE);
    		return -1;
    		}

    	pcl->dwTotalSize = dwSize;

    	if (lineGetCountry(dwCountryID, TAPI_VER, pcl) != 0)
    		{
    		assert(FALSE);
    		free(pcl);
  		    pcl = NULL;
    		return -1;
    		}
    	}

    *ppcl = pcl;
    return 0;
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctdrvGetComSettingsString
 *
 * DESCRIPTION:
 *	Retrieves a string formatted for display on the status line.
 *
 * ARGUMENTS:
 *	hhDriver	- private driver handle
 *	pachStr 	- buffer to store string
 *	cb			- size of buffer
 *
 * RETURNS:
 *	0=OK,else error
 *
 */
int cnctdrvGetComSettingsString(const HHDRIVER hhDriver, LPTSTR pachStr,
    							const size_t cb)
    {
    static CHAR acParity[] = "NOEMS";  // see com.h
    static CHAR *pachStop[] = {"1", "1.5", "2"};

    HCOM hCom;
    TCHAR ach[100];
    DWORD dwSize;
    LPVARSTRING pvs;
    int fAutoDetect = FALSE;
    long  lBaud = 0;
    int   iDataBits = 0;
    int   iParity = 0;
    int   iStopBits = 0;


    // Check the parameters
    //
    if (hhDriver == 0)
    	{
    	assert(0);
    	return -1;
    	}

    if (pachStr == 0 || cb == 0)
    	{
    	assert(0);
    	return -2;
    	}

    ach[0] = TEXT('\0');

    if ((hCom = sessQueryComHdl(hhDriver->hSession)) == 0)
    	return -7;

	//MPT:11-01-98 Microsoft made these changes to fix a bug relating
	//             to working with multiple connection devices.
	if (ComGetAutoDetect(hCom, &fAutoDetect) == COM_OK && fAutoDetect)
		{
		LoadString(glblQueryDllHinst(), IDS_STATUSBR_AUTODETECT, ach,
			sizeof(ach));
		}
#if defined(INCL_WINSOCK)
    else if (hhDriver->dwPermanentLineId == DIRECT_COMWINSOCK)
        {
        // Baud rate, data bits, parity, stop bits don't make sense in
        // TCP/IP. Load an alternate string.
        //
        LoadString(glblQueryDllHinst(), IDS_STATUSBR_COM_TCPIP, ach,
            sizeof(ach));
        }
#endif
    else if (IN_RANGE(hhDriver->dwPermanentLineId, DIRECT_COM1, DIRECT_COM4)
            || hhDriver->dwPermanentLineId == DIRECT_COM_DEVICE)
    	{
		long  lBaud = 0;
		int   iDataBits = 0;
		int   iParity = 0;
		int   iStopBits = 0;

    	ComGetBaud(hCom, &lBaud);
    	ComGetDataBits(hCom, &iDataBits);
    	ComGetParity(hCom, &iParity);
    	ComGetStopBits(hCom, &iStopBits);

    	wsprintf(ach, "%ld %d-%c-%s", lBaud, iDataBits,
    			acParity[iParity], pachStop[iStopBits]);
    	}

    // Usual 100 lines of code for a TAPI call
    //
    else if (hhDriver->dwLine != (DWORD)-1)
    	{
    	if ((pvs = malloc(sizeof(VARSTRING))) == 0)
    		{
    		assert(FALSE);
    		return -3;
    		}

    	pvs->dwTotalSize = sizeof(VARSTRING);

    	if (lineGetDevConfig(hhDriver->dwLine, pvs, DEVCLASS) != 0)
    		{
    		assert(FALSE);
    		free(pvs);
  		    pvs = NULL;
    		return -4;
    		}

    	if (pvs->dwNeededSize > pvs->dwTotalSize)
    		{
    		dwSize = pvs->dwNeededSize;
    		free(pvs);
  		    pvs = NULL;

    		if ((pvs = malloc(dwSize)) == 0)
    			{
    			assert(FALSE);
    			return -5;
    			}

    		pvs->dwTotalSize = dwSize;

    		if (lineGetDevConfig(hhDriver->dwLine, pvs, DEVCLASS) != 0)
    			{
    			assert(FALSE);
    			free(pvs);
  			    pvs = NULL;
    			return -6;
    			}
    		}

    	// The structure of the DevConfig block is as follows
    	//
    	//	VARSTRING
    	//	DEVCFGHDR
    	//	COMMCONFIG
    	//	MODEMSETTINGS
    	//
    	// These structures are not defined below yet so stub them
    	// in for now. - mrw (10/7/94)
    	//
    		{
    		typedef struct tagDEVCFGGDR
    			{
    			DWORD	dwSize;
    			DWORD	dwVersion;
    			DWORD	fTerminalMode;
    			}
    			DEVCFGHDR;

    		typedef struct tagDEVCFG
    			{
    			DEVCFGHDR	dfgHdr;
    			COMMCONFIG	commconfig;
    			}
    			DEVCFG, *PDEVCFG;

    		PDEVCFG pDevCfg;

    		pDevCfg = (PDEVCFG)((BYTE *)pvs + pvs->dwStringOffset);

    		// commconfig struct has a DCB structure we dereference for the
    		// com settings.
    		//
			lBaud = pDevCfg->commconfig.dcb.BaudRate;
			iDataBits = pDevCfg->commconfig.dcb.ByteSize;
			iParity = pDevCfg->commconfig.dcb.Parity;
			iStopBits = pDevCfg->commconfig.dcb.StopBits;
			wsprintf(ach, "%ld %d-%c-%s", lBaud, iDataBits,
    			acParity[iParity], pachStop[iStopBits]);
#if 0	//DEADWOOD:jkh 9/9/98
    		wsprintf(ach, "%u %d-%c-%s", pDevCfg->commconfig.dcb.BaudRate,
    			pDevCfg->commconfig.dcb.ByteSize,
    			acParity[pDevCfg->commconfig.dcb.Parity],
    			pachStop[pDevCfg->commconfig.dcb.StopBits]);
#endif
    		}
    	}

	// Moved this test to last so any change from 8N1 will not show auto-detect jkh 9/9/98
    if (iDataBits == 8 && iParity == NOPARITY && iStopBits == ONESTOPBIT &&
			ComGetAutoDetect(hCom, &fAutoDetect) == COM_OK && fAutoDetect)
    	{
    	LoadString(glblQueryDllHinst(), IDS_STATUSBR_AUTODETECT, ach,
    		sizeof(ach));
    	}


    strncpy(pachStr, ach, cb);
    pachStr[cb-sizeof(TCHAR)] = TEXT('\0');
    return 0;
    }

#if !defined(NDEBUG)
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	tapiTrap
 *
 * DESCRIPTION:
 *	Take one part stupidity, add two parts frustration, and stir
 *	until throughly confused.
 *
 * ARGUMENTS:
 *	dw	- result code from tapi
 *	file - file where error occured
 *	line - line where error occured
 *
 * RETURNS:
 *	dw
 *
 */
DWORD tapiTrap(const DWORD dw, const TCHAR *file, const int line)
    {
    char ach[256];

    if (dw != 0)
    	{
    	wsprintf(ach, "TAPI returned %x on line %d of file %s", dw, line, file);
    	MessageBox(GetFocus(), ach, "TAPI Trap", MB_OK | MB_ICONINFORMATION);
    	}

    return dw;
    }
#endif
