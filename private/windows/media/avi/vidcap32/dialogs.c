/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992 - 1995  Microsoft Corporation.  All Rights Reserved.
 *
 **************************************************************************/
/****************************************************************************
 *
 *   dialogs.c: Dialog box processing
 *
 *   Vidcap32 Source code
 *
 ***************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <dos.h>
#include <vfw.h>

#include "arrow.h"
#include "rlmeter.h"
#include "vidcap.h"
#include "vidframe.h"
#include "help.h"

static long GetFreeDiskSpaceInKB(LPTSTR) ;
static int  CountMCIDevices(UINT) ;

LRESULT FAR PASCAL MCISetupProc(HWND, unsigned, WPARAM, LPARAM);


//--- utility functions  ---------------------------------------------------



/*----------------------------------------------------------------------------*\
|   SmartWindowPosition (HWND hWndDlg, HWND hWndShow)
|                                                                              |
|   Description:                                                               |
|       This function attempts to position a dialog box so that it
|       does not obscure the hWndShow window. This function is
|       typically called during WM_INITDIALOG processing.
|                                                                              |
|   Arguments:                                                                 |
|       hWndDlg         handle of the soon to be displayed dialog
|       hWndShow        handle of the window to keep visible
|                                                                              |
|   Returns:                                                                   |
|       1 if the windows overlap and positions were adjusted
|       0 if the windows don't overlap
|                                                                              |
\*----------------------------------------------------------------------------*/
int SmartWindowPosition (HWND hWndDlg, HWND hWndShow)
{
    RECT rc, rcDlg, rcShow;
    int iHeight, iWidth;
    int iScreenHeight, iScreenWidth;

    GetWindowRect(hWndDlg, &rcDlg);
    GetWindowRect(hWndShow, &rcShow);

    iScreenHeight = GetSystemMetrics(SM_CYSCREEN);
    iScreenWidth = GetSystemMetrics(SM_CXSCREEN);

    InflateRect (&rcShow, 5, 5); // allow a small border
    if (IntersectRect(&rc, &rcDlg, &rcShow)){
        /* the two do intersect, now figure out where to place */
        /* this dialog window.  Try to go below the Show window*/
        /* first and then to the right, top and left.	   */

        /* get the size of this dialog */
        iHeight = rcDlg.bottom - rcDlg.top;
        iWidth = rcDlg.right - rcDlg.left;

        if ((UINT)(rcShow.bottom + iHeight + 1) <  (UINT)iScreenHeight){
                /* will fit on bottom, go for it */
                rc.top = rcShow.bottom + 1;
                rc.left = (((rcShow.right - rcShow.left)/2) + rcShow.left)
    		        - (iWidth/2);
        } else if ((UINT)(rcShow.right + iWidth + 1) < (UINT)iScreenWidth){
                /* will fit to right, go for it */
                rc.left = rcShow.right + 1;
                rc.top = (((rcShow.bottom - rcShow.top)/2) + rcShow.top)
    	        - (iHeight/2);
        } else if ((UINT)(rcShow.top - iHeight - 1) > 0){
                /* will fit on top, handle that */
                rc.top = rcShow.top - iHeight - 1;
                rc.left = (((rcShow.right - rcShow.left)/2) + rcShow.left)
    		        - (iWidth/2);
        } else if ((UINT)(rcShow.left - iWidth - 1) > 0){
                /* will fit to left, do it */
                rc.left = rcShow.left - iWidth - 1;
                rc.top = (((rcShow.bottom - rcShow.top)/2) + rcShow.top)
    	        - (iHeight/2);
        } else {
                /* we are hosed, they cannot be placed so that there is */
                /* no overlap anywhere.  To minimize the damage just put*/
                /* the dialog in the lower left corner of the screen    */
                rc.top = (int)iScreenHeight - iHeight;
                rc.left = (int)iScreenWidth - iWidth;
        }

        /* make any adjustments necessary to keep it on the screen */
        if (rc.left < 0) rc.left = 0;
        else if ((UINT)(rc.left + iWidth) > (UINT)iScreenWidth)
                rc.left = (int)(iScreenWidth - iWidth);

        if (rc.top < 0)  rc.top = 0;
        else if ((UINT)(rc.top + iHeight) > (UINT)iScreenHeight)
                rc.top = (int)iScreenHeight - iHeight;

        SetWindowPos(hWndDlg, NULL, rc.left, rc.top, 0, 0,
    	        SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
        return 1;
    } // if the windows overlap by default

    return 0;
}

//
// GetFreeDiskSpace: Function to Measure Available Disk Space
//
static long GetFreeDiskSpaceInKB(LPTSTR pFile)
{
    DWORD dwFreeClusters, dwBytesPerSector, dwSectorsPerCluster, dwClusters;
    TCHAR RootName[MAX_PATH];
    LPTSTR ptmp;    //required arg

    // need to find path for root directory on drive containing
    // this file.

    GetFullPathName(pFile, sizeof(RootName)/sizeof(RootName[0]), RootName, &ptmp);

    // truncate this to the name of the root directory (god how tedious)
    if ((RootName[0] == TEXT('\\')) && (RootName[1] == TEXT('\\'))) {

        // path begins with  \\server\share\path so skip the first
        // three backslashes
        ptmp = &RootName[2];
        while (*ptmp && (*ptmp != TEXT('\\'))) {
            ptmp++;
        }
        if (*ptmp) {
            // advance past the third backslash
            ptmp++;
        }
    } else {
        // path must be drv:\path
        ptmp = RootName;
    }

    // find next backslash and put a null after it
    while (*ptmp && (*ptmp != TEXT('\\'))) {
        ptmp++;
    }
    // found a backslash ?
    if (*ptmp) {
        // skip it and insert null
        ptmp++;
        *ptmp = TEXT('\0');
    }



    if (!GetDiskFreeSpace(RootName,
		&dwSectorsPerCluster,
		&dwBytesPerSector,
		&dwFreeClusters,
		&dwClusters)) {
	    MessageBoxID(IDS_ERR_MEASUREFREEDISK, MB_OK | MB_ICONINFORMATION);
	    return (-1);
    }
    return(MulDiv (dwSectorsPerCluster * dwBytesPerSector,
		   dwFreeClusters,
		   1024));
}

//
// CountMCIDevices: Function to Find the Number of MCI Devices of a Type
//
static int CountMCIDevices(UINT wType)
{
    int               nTotal = 0 ;
    DWORD             dwCount ;
    MCI_SYSINFO_PARMS mciSIP ;

    mciSIP.dwCallback = 0 ;
    mciSIP.lpstrReturn = (LPTSTR)(LPVOID) &dwCount ;
    mciSIP.dwRetSize = sizeof(DWORD) ;
    mciSIP.wDeviceType = wType ;

    // Use an MCI command to get the info
    if (! mciSendCommand(0, MCI_SYSINFO, MCI_SYSINFO_QUANTITY,
                         (DWORD_PTR)(LPVOID) &mciSIP))
        nTotal = (int) *((LPDWORD) mciSIP.lpstrReturn) ;

    return nTotal ;
}



/* lMicroSec = StringRateToMicroSec(szRate)
 *
 * Convert <szRate> (e.g. "3.75" representing 3.75 frames per second)
 * to microseconds (e.g. 266667L microseconds per frame).
 *
 * If the rate is close to zero or negative, then 0L is returned.
 */
DWORD StringRateToMicroSec(PSTR szRate)
{
	double		dRate;

	dRate = atof(szRate);
	
	if (dRate < 0.0001) {
		return 0L;
	} else {
		return (DWORD) /*floor*/((1e6 / dRate) + 0.5);
        }
}

/* ach = MicroSecToStringRate(achRate, lMicroSec)
 *
 * Convert <lMicroSec> (e.g. 266667L microseconds per frame) to a
 * string rate (e.g. "3.75" representing 3.75 frames per second).
 * Returns <achRate>.
 */
PSTR MicroSecToStringRate(PSTR achRate, DWORD dwMicroSec)
{
	sprintf(achRate, "%.3f",
		(dwMicroSec == 0L) ? 0.0 : (1e6 / (double) dwMicroSec));

	return achRate;
}

/*
 * update the text of an edit field based on a comarrow up or down change
 * - write the text in N.NNN format (truncated to an integer)
 */
LONG FAR PASCAL
MilliSecVarArrowEditChange(
    HWND hwndEdit,
    UINT uCode,
    LONG lMin,
    LONG lMax,
    UINT uInc
)
{
    TCHAR achTemp[32];
    LONG l;

    GetWindowText(hwndEdit, achTemp, sizeof(achTemp));

    l = atol(achTemp);
    if(uCode == SB_LINEUP ) {

	if(l + (long)uInc <= lMax ) {
	    l += uInc;
	    wsprintf(achTemp, "%ld.000", l );
	    SetWindowText(hwndEdit, achTemp );
        } else {
	    MessageBeep( 0 );
	}
    } else if (uCode == SB_LINEDOWN ) {
	if( l-(long)uInc >= lMin ) {
	    l -= uInc;
	    wsprintf( achTemp, "%ld.000", l );
	    SetWindowText( hwndEdit, achTemp );
        } else {
	    MessageBeep( 0 );
	}
    }
    return( l );
}


BOOL MCIGetDeviceNameAndIndex (HWND hwnd, LPINT lpnIndex, LPTSTR lpName)
{
    HWND hwndCB;
    TCHAR buf[160];
    TCHAR *cp;

    hwndCB = GetDlgItem( hwnd, IDD_MCI_SOURCE );
    *lpnIndex = (int)SendMessage( hwndCB, CB_GETCURSEL, 0, 0L);
    SendMessage( hwndCB, CB_GETLBTEXT, *lpnIndex,
    		(LONG_PTR)(LPTSTR) buf );
    // Point cp to the system name
    for (cp = buf + lstrlen(buf); cp > buf; cp--) {
        if (*cp == ' ' && *(cp-1) == ',') {
            cp++;
            break;
	}
    }
    lstrcpy (lpName, cp);
    return TRUE;
}


/*--------------------------------------------------------------+
| TimeMSToHMSString() - change milliseconds into a time string   |
+--------------------------------------------------------------*/
void FAR PASCAL TimeMSToHMSString (DWORD dwMS, LPTSTR lpTime)
{
	DWORD	dwTotalSecs;
	LONG	lHundredths;
	WORD	wSecs;
	WORD	wMins;
	WORD	wHours;

	/* convert to number of seconds */
	dwTotalSecs = dwMS / 1000;
	
	/* keep the remainder part */
	lHundredths = (dwMS - (dwTotalSecs * 1000)) / 10;
		
	/* break down into other components */
	wHours = (WORD)(dwTotalSecs / 3600);	// get # Hours
	dwTotalSecs -= (wHours * 3600);
	
	wMins = (WORD)(dwTotalSecs / 60);	// get # Mins
	dwTotalSecs -= (wMins * 60);
	
	wSecs = (WORD)dwTotalSecs;	// what's left is # seconds
	
	/* build the string */
	wsprintf((TCHAR far *)lpTime, "%02u:%02u:%02u.%02lu", wHours, wMins,
		    wSecs, lHundredths);
}


/*--------------------------------------------------------------+
| TimeHMSStringToMS() - change Time string to milliseconds     |
|                       returns dwMilliseconds or -1 if error  |
+--------------------------------------------------------------*/
LONG NEAR PASCAL  TimeHMSStringToMS (LPTSTR lpsz)
{
    TCHAR	achTime[12];	// buffer for time string (input)
    DWORD	dwMSecs;	// total MSecs for this thing */
    TCHAR	*pDelim;	// pointer to next delimeter
    TCHAR	*p;		// general pointer
    DWORD	dwHours = 0;	// # of hours
    DWORD	dwMins = 0;	// # of minutes
    DWORD	dwSecs = 0;		// # of seconds
    UINT	wHundredths = 0;	// # hundredths

    _fstrncpy(achTime, lpsz, sizeof (achTime));

    if (achTime[0] == '\0')
        return -1;	// bad TCHAR so error out
    	
    /* rip through the whole string and look for illegal TCHARs */
    for (p = achTime; *p ; p++){
        if (!isdigit(*p) && *p != '.' && *p != ':')
    	return -1;	// bad char so error out
    }

    /* go find the hundredths portion if it exists */
    pDelim = strchr(achTime, '.');
    if (pDelim && *pDelim){
        p = strrchr(achTime, '.');
        if (pDelim != p) {
    	    return -1;		// string has > 1 '.', return error
        }

        p++;			// move up past delim
        if (strlen(p) > 2) {
    	    *(p+2) = '\0';		// knock off all but hundredths
        }

        wHundredths = atoi(p);	// get the fractional part

        *pDelim = '\0';		// null out this terminator
    }

    /* try and find seconds */
    pDelim = strrchr(achTime, ':');	// get last ':'
    if (*pDelim) {
        p = (pDelim+1);
    } else {
        // no colon - assume just seconds in string
        p = achTime;
    }
    dwSecs = atoi(p);

    if (*pDelim) {
        *pDelim = '\0';

        /* go and get the minutes part */
        pDelim = strrchr(achTime, ':');
        if (*pDelim) {
            p = (pDelim + 1);
        } else {
            // no more colons - assume remainder is just minutes
            p = achTime;
        }
        dwMins = atoi(p);

        if (*pDelim) {
            *pDelim = '\0';

            /* get the hours */
            p = achTime;
            dwHours = atoi(p);
        }
    }

    /* now we've got the hours, minutes, seconds and any	*/
    /* fractional part.  Time to build up the total time	*/

    dwSecs += (dwHours * 3600);	// add in hours worth of seconds
    dwSecs += (dwMins * 60);	// add in minutes worth of seconds
    dwMSecs = (dwSecs * 1000L);
    dwMSecs += (wHundredths * 10L);

    /* now we've got the total number of milliseconds */
    return dwMSecs;
}


/*
 *  MCIDeviceClose
 *      This routine closes the open MCI device.
 */

void MCIDeviceClose (void)
{
    mciSendString( "close mciframes", NULL, 0, NULL );
}



/*
 *  MCIDeviceOpen
 *      This routine opens the mci device for use, and sets the
 *      time format to milliseconds.
 *      Return FALSE on error;
 */

BOOL MCIDeviceOpen (LPTSTR lpDevName)
{
    TCHAR        ach[160];
    DWORD dwMCIError;

    wsprintf( ach, "open %s shareable wait alias mciframes", (LPTSTR) lpDevName);
    dwMCIError = mciSendString( ach, NULL, 0, NULL );
    if( dwMCIError )  {
        return(FALSE);
    }

    dwMCIError = mciSendString( "set mciframes time format milliseconds",
	    NULL, 0, NULL );
    if( dwMCIError ) {
        MCIDeviceClose();
        return(FALSE);
    }
    return ( TRUE );

}


/*
 *  MCIDeviceGetPosition
 *      Stores the current device position in milliseconds in lpdwPos.
 *      Returns TRUE on success, FALSE if error.
 */
BOOL FAR PASCAL MCIDeviceGetPosition (LPDWORD lpdwPos)
{
    TCHAR        ach[80];
    DWORD dwMCIError;

    dwMCIError = mciSendString( "status mciframes position wait",
	    ach, sizeof(ach), NULL );
    if( dwMCIError ) {
        *lpdwPos = 0L;
        return FALSE;
    }

    *lpdwPos = atol( ach );
    return TRUE;
}

#ifndef USE_ACM

// --- audio streaming ------------------------------------------------

// the ShowLevel dialog streams data in from the input and
// shows the current volume.

// buffers into which sound data is recorded
#define NUM_LEVEL_BUFFERS   2

// the buffer size is calculated to be about 1/20 sec
#define UPDATES_PER_SEC     20

/*
 * we save all our data in one of these, and write a pointer to it
 * into the dialog DWL_USER field.
 */

typedef struct _LevelStreamData {
    LPWAVEHDR alpWave[NUM_LEVEL_BUFFERS];
    PCMWAVEFORMAT FAR * pwf;
    HWAVEIN hwav;
    int buffersize;
} LEVELSTREAMDATA, FAR * PLEVELSTREAMDATA;


//open the wave-device in the given format, queue all the buffers and
//start data streaming. Save the wavein device to the dialog DWL_USER window
//data area so we can close it on dialog dismissal.
BOOL
OpenStream(HWND hDlg, PCMWAVEFORMAT FAR * pwf)
{
    PLEVELSTREAMDATA pInfo;
    int i;


    pInfo = (PLEVELSTREAMDATA) GlobalLock(GlobalAlloc(GHND, sizeof(LEVELSTREAMDATA)));

    if (pInfo == NULL) {
        return(FALSE);
    }


    // complete remaining areas of wf
    pwf->wf.wFormatTag = WAVE_FORMAT_PCM;
    pwf->wf.nBlockAlign = pwf->wf.nChannels * pwf->wBitsPerSample / 8;
    pwf->wf.nAvgBytesPerSec = pwf->wf.nSamplesPerSec * pwf->wf.nBlockAlign;

    // save for later use
    pInfo->pwf = pwf;

    // buffer size a fixed fraction of a second
    pInfo->buffersize = pwf->wf.nAvgBytesPerSec/UPDATES_PER_SEC;


    pInfo->hwav = NULL;

    if (waveInOpen(
        &pInfo->hwav,
        WAVE_MAPPER,
        (LPWAVEFORMATEX)pwf,
        (DWORD) hDlg,               // callback via MM_WIM_ messages to dialogproc
        0,
        CALLBACK_WINDOW)) {
            SetWindowLong(hDlg, DWL_USER, 0);
            return(FALSE);
    }

    // store the info structure in the dialog, so that even if we fail
    // on this routine we will clean up correctly
    SetWindowLong(hDlg, DWL_USER, (long) pInfo);

    // set all the wave headers to null (for cleanup if error)
    for (i = 0; i < NUM_LEVEL_BUFFERS; i++) {
        pInfo->alpWave[i] = NULL;
    }

    // alloc, prepare and add all the buffers
    for (i = 0; i < NUM_LEVEL_BUFFERS; i++) {

        pInfo->alpWave[i] = GlobalLock(GlobalAlloc(GMEM_MOVEABLE|GMEM_SHARE,
                        sizeof(WAVEHDR) + pInfo->buffersize));
        if (pInfo->alpWave[i] == NULL) {
            return(FALSE);
        }

        pInfo->alpWave[i]->lpData = (LPBYTE) (pInfo->alpWave[i] + 1);
        pInfo->alpWave[i]->dwBufferLength = pInfo->buffersize;
        pInfo->alpWave[i]->dwBytesRecorded = 0;
        pInfo->alpWave[i]->dwUser = 0;
        pInfo->alpWave[i]->dwFlags = 0;
        pInfo->alpWave[i]->dwLoops = 0;

        if (waveInPrepareHeader(pInfo->hwav, pInfo->alpWave[i], sizeof(WAVEHDR))) {
            return(FALSE);
        }

        if (waveInAddBuffer(pInfo->hwav, pInfo->alpWave[i], sizeof(WAVEHDR))) {
            return(FALSE);
        }
    }

    waveInStart(pInfo->hwav);

    return(TRUE);
}

// terminate the data streaming on a wavein device associated with a
// dialog, and clean up the buffers allocated
void
CloseStream(HWND hDlg)
{
    PLEVELSTREAMDATA pInfo;
    int i;


    // pick up our info from the dialog
    pInfo = (PLEVELSTREAMDATA) GetWindowLong(hDlg, DWL_USER);
    if ((pInfo == NULL) || (pInfo->hwav == NULL)) {
        return;
    }

    // stop streaming data
    waveInStop(pInfo->hwav);

    // release all buffers
    waveInReset(pInfo->hwav);

    // unlock and free buffers
    for (i = 0; i < NUM_LEVEL_BUFFERS; i++) {
        if (pInfo->alpWave[i]) {
            waveInUnprepareHeader(pInfo->hwav, pInfo->alpWave[i], sizeof(WAVEHDR));
            GlobalFree(GlobalHandle(pInfo->alpWave[i]));
            pInfo->alpWave[i] = NULL;
        }

    }
    waveInClose(pInfo->hwav);

    GlobalFree(GlobalHandle(pInfo));

    SetWindowLong(hDlg, DWL_USER, 0);


}

// we have received a block of data. work out the level(s) and send to
// the appropriate control on the dialog, and then requeue the buffer.
// return FALSE if any error occurs, otherwise TRUE
BOOL
StreamData(HWND hDlg, HWAVEIN hwav, LPWAVEHDR pHdr)
{
    PLEVELSTREAMDATA pInfo;
    int n = 0;
    int LevelLeft = 0, LevelRight = 0;
    int i, l;

    // pick up our info from the dialog
    pInfo = (PLEVELSTREAMDATA) GetWindowLong(hDlg, DWL_USER);
    if ((pInfo == NULL) || (pInfo->hwav != hwav)) {
        return FALSE;
    }

    // go through all samples in buffer looking for maximum absolute level
    while (n < pInfo->buffersize) {

        /*
         * volumes go above and below the mean level - we are
         * interested in the absolute volume
         * 8 bit samples are in the range 0..255
         * 16-bit samples are in the range -32768..+32767
         */

        // skip the first byte if 16-bit
        // and adjust to be in range -127..+128
        if (pInfo->pwf->wBitsPerSample == 16) {
            n++;
            i = (int) (signed char) pHdr->lpData[n];
        } else {
            i = (int) ((unsigned char) pHdr->lpData[n]) - 128;
        }

        // skip past the byte we've picked up
        n++;

        // take absolute volume level
        if (i < 0) {
            i = -i;
        }

        // convert to percentage
        l = (i*100) / 128;

        // compare against current max
        if (LevelLeft < l) {
            LevelLeft = l;
        }


        // if stereo, repeat for right channel
        if (pInfo->pwf->wf.nChannels == 2) {
            // skip the first byte if 16-bit
            if (pInfo->pwf->wBitsPerSample == 16) {
                n++;
                i = (int) (signed char) pHdr->lpData[n];
            } else {
                i = (int) ((unsigned char) pHdr->lpData[n]) - 128;
            }

            // skip past the byte we've picked up
            n++;

            // take absolute volume level
            if (i < 0) {
                i = -i;
            }

            // convert to percentage
            l = (i*100) / 128;

            // compare against current max
            if (LevelRight < l) {
                LevelRight = l;
            }
        }
    }

    // put the buffer back on the queue
    if (waveInAddBuffer(pInfo->hwav, pHdr, sizeof(WAVEHDR))) {
        return(FALSE);
    }

    // send new level to dialog control
    SendDlgItemMessage(hDlg, IDRL_LEVEL1, WMRL_SETLEVEL, 0, LevelLeft);
    if (pInfo->pwf->wf.nChannels == 2) {
        SendDlgItemMessage(hDlg, IDRL_LEVEL2, WMRL_SETLEVEL, 0, LevelRight);
    }

    return(TRUE);
}

#endif  // ! USE_ACM


// --- dialog procs -----------------------------------------------------


//
// AboutProc: About Dialog Box Procedure
//
LRESULT FAR PASCAL AboutProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message) {
        case WM_INITDIALOG :
             return TRUE ;

        case WM_COMMAND :
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDOK :
                    EndDialog(hDlg, TRUE) ;
                    return TRUE ;

                case IDCANCEL :
                    EndDialog(hDlg, FALSE) ;
                    return TRUE ;
            }
            break ;
    }

    return FALSE ;
}

#ifndef USE_ACM

/*
 * dialog proc for IDD_RECLVLMONO and IDD_RECLVLSTEREO - show current
 * volume level
 */
LRESULT FAR PASCAL
ShowLevelProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message) {

    case WM_INITDIALOG:
        if (!OpenStream(hDlg, (PCMWAVEFORMAT FAR *) lParam)) {
            MessageBoxID(IDS_ERR_ACCESS_SOUNDDRIVER, MB_OK|MB_ICONSTOP);
            EndDialog(hDlg, FALSE);
        }
        return(TRUE);

    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDOK:
        case IDCANCEL:

            CloseStream(hDlg);
            EndDialog(hDlg, TRUE);
            return(TRUE);
        }
        break;

    case MM_WIM_DATA:
        if (!StreamData(hDlg, (HWAVEIN)wParam, (LPWAVEHDR)lParam)) {
            MessageBoxID(IDS_ERR_ACCESS_SOUNDDRIVER, MB_OK|MB_ICONSTOP);
            CloseStream(hDlg);
            EndDialog(hDlg, FALSE);
        }
        return(TRUE);

    }
    return FALSE;
}





//
// AudioFormatProc: Audio Format Setting Dialog Box Procedure
//
LRESULT FAR PASCAL AudioFormatProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam)
{
    static int                nChannels ;
    static UINT               wSample ;
    static DWORD              dwFrequency ;

    switch (Message) {
        case WM_INITDIALOG :
            nChannels = IDD_ChannelIDs + glpwfex->nChannels ;
            CheckRadioButton(hDlg, IDD_ChannelMono, IDD_ChannelStereo, nChannels) ;
            wSample = IDD_SampleIDs + glpwfex->wBitsPerSample / 8 ;
            CheckRadioButton(hDlg, IDD_Sample8Bit, IDD_Sample16Bit, wSample) ;
            dwFrequency = IDD_FreqIDs + glpwfex->nSamplesPerSec / 11025 ;
            CheckRadioButton(hDlg, IDD_Freq11kHz, IDD_Freq44kHz, (UINT)dwFrequency) ;
            return TRUE ;

        case WM_COMMAND :
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDD_SetLevel:
                {
                    // get the current data into a PCMWAVEFORMAT struct,
                    // and run the ShowLevel dialog
                    PCMWAVEFORMAT wf;
                    UINT dlgid;

                    if (IsDlgButtonChecked(hDlg, IDD_ChannelMono)) {
                        wf.wf.nChannels = 1;
                        dlgid = IDD_RECLVLMONO;
                    } else {
                        wf.wf.nChannels = 2;
                        dlgid = IDD_RECLVLSTEREO;
                    }

                    if (IsDlgButtonChecked(hDlg, IDD_Sample8Bit)) {
                        wf.wBitsPerSample = 8;
                    } else {
                        wf.wBitsPerSample = 16;
                    }

                    if (IsDlgButtonChecked(hDlg, IDD_Freq11kHz)) {
                        wf.wf.nSamplesPerSec = 11025 ;
                    } else if (IsDlgButtonChecked(hDlg, IDD_Freq22kHz)) {
                        wf.wf.nSamplesPerSec = 22050 ;
                    } else {
                        wf.wf.nSamplesPerSec =  44100 ;
                    }

                    DoDialog(
                        hDlg,
                        dlgid,
                        ShowLevelProc,
                        (LPARAM) &wf);
                    break;
                }

                case IDOK :
                    if (IsDlgButtonChecked(hDlg, IDD_ChannelMono))
                        nChannels = 1 ;
                    else
                        if (IsDlgButtonChecked(hDlg, IDD_ChannelStereo))
                            nChannels = 2 ;
                        else {
                            MessageBeep(MB_ICONEXCLAMATION) ;
                            return FALSE ;
                        }

                    if (IsDlgButtonChecked(hDlg, IDD_Sample8Bit))
                        wSample = 8 ;
                    else
                        if (IsDlgButtonChecked(hDlg, IDD_Sample16Bit))
                            wSample = 16 ;
                        else {
                            MessageBeep(MB_ICONEXCLAMATION) ;
                            return FALSE ;
                        }

                    if (IsDlgButtonChecked(hDlg, IDD_Freq11kHz))
                        dwFrequency = 11025 ;
                    else
                        if (IsDlgButtonChecked(hDlg, IDD_Freq22kHz))
                            dwFrequency = 22050 ;
                        else
                            if (IsDlgButtonChecked(hDlg, IDD_Freq44kHz))
                                dwFrequency = 44100 ;
                            else {
                                MessageBeep(MB_ICONEXCLAMATION) ;
                                return FALSE ;
                            }

                    // All the entries verfied OK -- save them now
                    glpwfex->nChannels = nChannels ;
                    glpwfex->wBitsPerSample = wSample ;
                    glpwfex->nSamplesPerSec = dwFrequency ;
                    glpwfex->nBlockAlign =  glpwfex->nChannels * (glpwfex->wBitsPerSample / 8) ;
                    glpwfex->nAvgBytesPerSec = (long) glpwfex->nSamplesPerSec *
                                                      glpwfex->nBlockAlign ;
                    glpwfex->cbSize = 0 ;
                    glpwfex->wFormatTag = WAVE_FORMAT_PCM ;
                    EndDialog(hDlg, TRUE) ;
                    return TRUE ;

                case IDCANCEL :
                    EndDialog(hDlg, FALSE) ;
                    return TRUE ;
            }
            break ;
    }

    return FALSE ;
}

#endif // ! USE_ACM

//
// AllocCapFileProc: Capture file Space Allocation Dialog Box Procedure
//
LRESULT FAR PASCAL AllocCapFileProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam)
{
    static int      nFreeMBs = 0 ;

    switch (Message) {
        case WM_INITDIALOG :
        {
            int              fh ;
            long             lFileSize = 0 ;
            long             lFreeSpaceInKB ;
            TCHAR	     achCapFile[_MAX_PATH] ;

            // Get current capture file name and measure its size
            capFileGetCaptureFile(ghWndCap, achCapFile, sizeof(achCapFile) / sizeof(TCHAR)) ;
            if ((fh = _open(achCapFile, _O_RDONLY)) != -1) {
                if ((lFileSize = _lseek(fh, 0L, SEEK_END)) == -1L) {
                    MessageBoxID(IDS_ERR_SIZECAPFILE,
#ifdef BIDI
                MB_RTL_READING |
#endif

                    MB_OK | MB_ICONEXCLAMATION) ;
                    lFileSize = 0 ;
                }
                _close(fh) ;
            }

            // Get free disk space and add current capture file size to that.
            // Convert the available space to MBs.
            if ((lFreeSpaceInKB = GetFreeDiskSpaceInKB(achCapFile)) != -1L) {
                lFreeSpaceInKB += lFileSize / 1024 ;
                nFreeMBs = lFreeSpaceInKB / 1024 ;
                SetDlgItemInt(hDlg, IDD_SetCapFileFree, nFreeMBs, TRUE) ;
            } else {

                EnableWindow(GetDlgItem(hDlg, IDD_SetCapFileFree), FALSE);

            }

            gwCapFileSize = (WORD) (lFileSize / ONEMEG);

            SetDlgItemInt(hDlg, IDD_SetCapFileSize, gwCapFileSize, TRUE) ;
            return TRUE ;
        }

        case WM_COMMAND :
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDOK :
                {
                    int         iCapFileSize ;

                    iCapFileSize = (int) GetDlgItemInt(hDlg, IDD_SetCapFileSize, NULL, TRUE) ;
                    if (iCapFileSize <= 0 || iCapFileSize > nFreeMBs) {
                        // You are asking for more than we have !! Sorry, ...
                        SetDlgItemInt(hDlg, IDD_SetCapFileSize, iCapFileSize, TRUE) ;
                        SetFocus(GetDlgItem(hDlg, IDD_SetCapFileSize)) ;
                        MessageBeep(MB_ICONEXCLAMATION) ;
                        return FALSE ;
                    }
                    gwCapFileSize = (WORD) iCapFileSize ;

                    EndDialog(hDlg, TRUE) ;
                    return TRUE ;
                }

                case IDCANCEL :
                    EndDialog(hDlg, FALSE) ;
                    return TRUE ;

                case IDD_SetCapFileSize:
                {
                    long l;
                    BOOL bchanged;
                    TCHAR achBuffer[21];

                    // check that entered size is a valid number
                    GetDlgItemText(hDlg, IDD_SetCapFileSize, achBuffer, sizeof(achBuffer));
                    l = atol(achBuffer);
                    bchanged = FALSE;
                    if (l < 1) {
                        l = 1;
                        bchanged = TRUE;
                    } else if (l > nFreeMBs) {
                        l = nFreeMBs;
                        bchanged = TRUE;
                    } else {
                        // make sure there are no non-digit chars
                        // atol() will ignore trailing non-digit characters
                        int c = 0;
                        while (achBuffer[c]) {
                            if (IsCharAlpha(achBuffer[c]) ||
                                !IsCharAlphaNumeric(achBuffer[c])) {

                                // string contains non-digit chars - reset
                                l = 1;
                                bchanged = TRUE;
                                break;
                            }
                            c++;
                        }
                    }
                    if (bchanged) {
                        wsprintf(achBuffer, "%ld", l);
                        SetDlgItemText(hDlg, IDD_SetCapFileSize, achBuffer);
                    }
                    break;
                }
            }
            break ;
    }

    return FALSE ;

}

#if 0
//
// MakePaletteProc: Palette Details Dialog Box Procedure
//
BOOL CALLBACK MakePaletteProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message) {
        case WM_INITDIALOG :
            SetDlgItemInt(hDlg, IDD_MakePalColors, gwPalColors, FALSE) ;
            SetDlgItemInt(hDlg, IDD_MakePalFrames, gwPalFrames, FALSE) ;
            return TRUE ;

        case WM_COMMAND :
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDOK :
                {
                    int         iColors ;
                    int         iFrames ;

                    iColors = (int) GetDlgItemInt(hDlg, IDD_MakePalColors, NULL, TRUE) ;
                    if (! (iColors > 0 && iColors <= 236 || iColors == 256)) {
                        // invalid number of palette colors
                        SetDlgItemInt(hDlg, IDD_MakePalColors, iColors, TRUE) ;
                        SetFocus(GetDlgItem(hDlg, IDD_MakePalColors)) ;
                        MessageBeep(MB_ICONEXCLAMATION) ;
                        return FALSE ;
                    }
                    iFrames = (int) GetDlgItemInt(hDlg, IDD_MakePalFrames, NULL, TRUE) ;
                    if (iFrames <= 0 || iFrames > 10000) {
                        // no frame or way t-o-o many frames !!!
                        SetDlgItemInt(hDlg, IDD_MakePalFrames, iFrames, TRUE) ;
                        SetFocus(GetDlgItem(hDlg, IDD_MakePalFrames)) ;
                        MessageBeep(MB_ICONEXCLAMATION) ;
                        return FALSE ;
                    }
                    gwPalColors = iColors ;
                    gwPalFrames = iFrames ;

                    EndDialog(hDlg, TRUE) ;
                    return TRUE ;
                }

                case IDCANCEL :
                    EndDialog(hDlg, FALSE) ;
                    return TRUE ;
            }
            break ;
    }

    return FALSE ;

}

#endif


#define CAPPAL_TIMER    902    
#define CAPTIMER_DELAY  100       // get timers as fast as possible
//
// MakePaletteProc: Palette Details Dialog Box Procedure
//
static int      siNumColors = 256;

LRESULT CALLBACK MakePaletteProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static UINT_PTR shTimer;
    static int  siNumFrames;
    UINT        w;
    TCHAR        ach[40];
    TCHAR        achFormat[40];
    int         i, k;

    switch(msg) {
        case WM_INITDIALOG:
            siNumFrames = 0;
            SetDlgItemInt(hwnd, IDD_MakePalColors, siNumColors, FALSE);
            SmartWindowPosition (hwnd, ghWndCap);
            return TRUE;
            break;

        case WM_VSCROLL:
            /* now handle the scroll */
            i = GetDlgItemInt(hwnd, IDD_MakePalColors, NULL, FALSE);
            ArrowEditChange(GetDlgItem(hwnd, IDD_MakePalColors),
                GET_WM_VSCROLL_CODE(wParam, lParam), 2, 256);
            k = GetDlgItemInt(hwnd, IDD_MakePalColors, NULL, FALSE);
            // Jump over the range 237 to 255
            if (k > 236 && k < 256) {
                if (k > i) 
                   w = 256;
                else
                   w = 236;
                SetDlgItemInt (hwnd, IDD_MakePalColors, w, TRUE);
            }
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDCANCEL:
                    if (siNumFrames) {
                        // The following finishes building the new palette
                        capPaletteManual (ghWndCap, FALSE, siNumColors);
                    }

                    if (shTimer){
                        KillTimer(hwnd, CAPPAL_TIMER);
                        shTimer = 0;
                    }
                    siNumColors = GetDlgItemInt(hwnd, IDD_MakePalColors, (BOOL FAR *)ach, FALSE);
                    siNumColors = max (2, min (256, siNumColors)); 
                    EndDialog(hwnd, siNumFrames);
                    break;
                    
                case IDD_MakePalStart:
                    /* see if we are in START or STOP mode at   */
                    /* this time and handle each one.           */
                    SetFocus (GetDlgItem (hwnd, IDD_MakePalStart));
                    if (!siNumFrames){
                        /* this is the first frame, change the CANCEL */
                        /* button to CLOSE                              */
                        LoadString(ghInstApp, IDS_CAPPAL_CLOSE, ach, sizeof(ach));
                        SetDlgItemText(hwnd, IDCANCEL, ach);
                    }
                    if (!shTimer) {

                        shTimer = SetTimer(hwnd, CAPPAL_TIMER, CAPTIMER_DELAY, NULL);

                        if (shTimer == 0) {
                            //!!!error message here.
                            MessageBeep(0);
                            return TRUE;
                        }

                        /* button said START, let's set up to   */
                        /* do continuous capture.  This involves*/
                        /*   1 - disabling FRAME button         */
                        /*   2 - turning myself to STOP button  */
                        /*   3 - setting up frame timer         */
                        EnableWindow(GetDlgItem(hwnd, IDD_MakePalSingleFrame), FALSE);
                        LoadString(ghInstApp, IDS_CAPPAL_STOP, ach, sizeof(ach));
                        SetDlgItemText(hwnd, IDD_MakePalStart, ach);
                    } else {
                        /* button said STOP, turn things around */
                        /* by:                                  */
                        /*   1 - killing off timers             *
                        /*   2 - turning back into START button */
                        /*   3 - re-enabling FRAME button       */
                        // "&Start"
                        LoadString(ghInstApp, IDS_CAPPAL_START, ach, sizeof(ach));
                        SetDlgItemText(hwnd, IDD_MakePalStart, ach);
                        EnableWindow(GetDlgItem(hwnd, IDD_MakePalSingleFrame), TRUE);
                        KillTimer(hwnd, CAPPAL_TIMER);
                        shTimer = 0;
                    }
                    return TRUE;
                    break;
                    
                case IDD_MakePalSingleFrame:
                    if (!siNumFrames){
                        /* this is the first frame, change the CANCEL */
                        /* button to CLOSE                              */
                        LoadString(ghInstApp, IDS_CAPPAL_CLOSE, ach, sizeof(ach));
                        SetDlgItemText(hwnd, IDCANCEL, ach);
                        siNumColors = GetDlgItemInt(hwnd, IDD_MakePalColors, (BOOL FAR *)ach, FALSE);
                        siNumColors = max (2, min (256, siNumColors)); 
                    }
                    // Get the palette for a single frame
                    capPaletteManual (ghWndCap, TRUE, siNumColors);

                    siNumFrames++;
                    LoadString(ghInstApp, IDS_CAPPAL_STATUS, achFormat, sizeof(achFormat));
                    wsprintf(ach, achFormat, siNumFrames);
                    SetDlgItemText(hwnd, IDD_MakePalNumFrames, ach);
                    return TRUE;
                    break;

                case IDD_MakePalColors:
                    if (HIWORD (lParam) == EN_KILLFOCUS) {
                        w = GetDlgItemInt (hwnd, (UINT) wParam, NULL, FALSE);
                        if ( w < 2) {
                            MessageBeep (0);
                            SetDlgItemInt (hwnd, (UINT) wParam, 2, FALSE);
                        }
                        else if (w > 256) {
                            MessageBeep (0);
                            SetDlgItemInt (hwnd, (UINT) wParam, 256, FALSE);
                        }
                    }
                    return TRUE;
                    break;

                default:
                    return FALSE;
                    
            } // switch(wParam) on WM_COMMAND 
            break;
            
        case WM_TIMER:
            if (wParam == CAPPAL_TIMER){
                SendMessage(hwnd, WM_COMMAND, IDD_MakePalSingleFrame, 0L);
            }
            break;
        default:
            return FALSE;
            
    } // switch(msg)
    return FALSE;
}




//
// CapSetUpProc: Capture SetUp Details Dialog Box Procedure
//
LRESULT FAR PASCAL CapSetUpProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam)
{
    static TCHAR     achBuffer[21] ;
    UINT fValue;

    switch (Message) {
        case WM_INITDIALOG :
        {

            // Convert from MicroSecPerFrame to FPS -- that's easier !!
            MicroSecToStringRate(achBuffer, gCapParms.dwRequestMicroSecPerFrame);
            SetDlgItemText(hDlg, IDD_FrameRateData, achBuffer);


            // If time limit isn't enabled, disable the time data part
            CheckDlgButton(hDlg, IDD_TimeLimitFlag, (fValue = gCapParms.fLimitEnabled)) ;
            EnableWindow(GetDlgItem(hDlg, IDD_SecondsText), fValue) ;
            EnableWindow(GetDlgItem(hDlg, IDD_SecondsData), fValue) ;
            EnableWindow(GetDlgItem(hDlg, IDD_SecondsArrow), fValue);

            SetDlgItemInt(hDlg, IDD_SecondsData, gCapParms.wTimeLimit, FALSE) ;


            // disable audio buttons if no audio hardware
            {
                CAPSTATUS cs;

                capGetStatus(ghWndCap, &cs, sizeof(cs));
                EnableWindow(GetDlgItem(hDlg, IDD_CapAudioFlag), cs.fAudioHardware);
                EnableWindow(GetDlgItem(hDlg, IDD_AudioConfig), cs.fAudioHardware);

                CheckDlgButton(hDlg, IDD_CapAudioFlag, gCapParms.fCaptureAudio);
            }



            /*
             * Capture To Memory means allocate as many memory buffers
             *  as possible.
             * Capture To Disk means only allocate enough buffers
             *  to get us through disk seeks and thermal recalibrations.
             */

            // The use of fUsingDOSMemory is now just a means of keeping
            // track of whether using lots of buffers.  We never actually
            // allocate exclusively from memory under 1Meg.

            CheckRadioButton(hDlg, IDD_CaptureToDisk, IDD_CaptureToMemory,
              (gCapParms.fUsingDOSMemory)? IDD_CaptureToDisk : IDD_CaptureToMemory);

            // Find out how many MCI devices can source video
            if (CountMCIDevices(MCI_DEVTYPE_VCR) +
                CountMCIDevices(MCI_DEVTYPE_VIDEODISC) == 0) {
                // if no VCRs or Videodiscs, disable the controls
                EnableWindow(GetDlgItem(hDlg, IDD_MCIControlFlag), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDD_MCISetup), FALSE);
            } else {
                EnableWindow(GetDlgItem(hDlg, IDD_MCIControlFlag), TRUE);

                // if MCI Control is selected, enable the setup button
                CheckDlgButton(hDlg, IDD_MCIControlFlag,
                    gCapParms.fMCIControl);
                EnableWindow(GetDlgItem(hDlg, IDD_MCISetup), gCapParms.fMCIControl);
            }

            // place the dialog to avoid covering the capture window
            SmartWindowPosition(hDlg, ghWndCap);
            return TRUE ;
        }

        case WM_COMMAND :
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDD_TimeLimitFlag :
                    // If this flag changes, en/dis-able time limit data part
                    fValue = IsDlgButtonChecked(hDlg, IDD_TimeLimitFlag) ;
                    EnableWindow(GetDlgItem(hDlg, IDD_SecondsText), fValue) ;
                    EnableWindow(GetDlgItem(hDlg, IDD_SecondsData), fValue) ;
                    EnableWindow(GetDlgItem(hDlg, IDD_SecondsArrow), fValue);
                    return TRUE ;

                case IDD_MCIControlFlag :
                    // If this flag changes, en/dis-able MCI Setup button
                    fValue = IsDlgButtonChecked(hDlg, IDD_MCIControlFlag) ;
                    EnableWindow(GetDlgItem(hDlg, IDD_MCISetup), fValue) ;
                    return TRUE ;

                case IDD_CapAudioFlag:
                    fValue = IsDlgButtonChecked(hDlg, IDD_CapAudioFlag) ;
                    EnableWindow(GetDlgItem(hDlg, IDD_AudioConfig), fValue) ;
                    return TRUE ;


                case IDD_FrameRateData:
                    // get the requested frame rate and check it against bounds
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_KILLFOCUS) {
                        long l, new_l;

                        GetDlgItemText(hDlg, IDD_FrameRateData, achBuffer, sizeof(achBuffer));
                        new_l = l = StringRateToMicroSec(achBuffer);

                        // note that the MAX rate is SMALL! hence <max, >min
                        if (l == 0) {
                            new_l = DEF_CAPTURE_RATE;
                        } else if (l < MAX_CAPTURE_RATE) {
                            new_l = MAX_CAPTURE_RATE;
                        } else if (l > MIN_CAPTURE_RATE) {
                            new_l = MIN_CAPTURE_RATE;
                        }
                        if (l != new_l) {
                            MicroSecToStringRate(achBuffer, new_l);
                            SetDlgItemText(hDlg, IDD_FrameRateData, achBuffer);
                        }
                    }
                    break;

                case IDD_SecondsData:
                {
                    long l, new_l;

                    // get requested time limit and check validity
                    GetDlgItemText(hDlg, IDD_SecondsData, achBuffer, sizeof(achBuffer));
                    new_l = l = atol(achBuffer);
                    if (l < 1) {
                        new_l = 1;
                    } else if (l > 9999) {
                        new_l = 9999;
                    } else {
                        // make sure there are no non-digit chars
                        // atol() will ignore trailing non-digit characters
                        int c = 0;
                        while (achBuffer[c]) {
                            if (IsCharAlpha(achBuffer[c]) ||
                                !IsCharAlphaNumeric(achBuffer[c])) {

                                // string contains non-digit chars - reset
                                new_l = 1;
                                break;
                            }
                            c++;
                        }
                    }
                    if (new_l != l) {
                        wsprintf(achBuffer, "%ld", new_l);
                        SetDlgItemText(hDlg, IDD_SecondsData, achBuffer);
                        // select the changed text so that if you delete the
                        // '1' and then insert '10' you get 10 not 110
                        SendDlgItemMessage(hDlg, IDD_SecondsData,
                                EM_SETSEL, 0, -1);

                    }
                    break;
                }

                // show audio format setup dialog
                case IDD_AudioConfig:

                    // rather than duplicate lots of code from the
                    // main vidcap winproc, lets just ask it to show the dlg...
                    SendMessage(ghWndMain, WM_COMMAND,
                            GET_WM_COMMAND_MPS(IDM_O_AUDIOFORMAT, NULL, 0));

                    break;


                // show MCI step control dialog
                case IDD_MCISetup:
                    DoDialog(hDlg, IDD_MCISETUP, MCISetupProc, 0);
                    break;

                // show video format setup dialog
                case IDD_VideoConfig:
                    // rather than duplicate lots of code from the
                    // main vidcap winproc, lets just ask it to show the dlg...
                    SendMessage(ghWndMain, WM_COMMAND,
                            GET_WM_COMMAND_MPS(IDM_O_VIDEOFORMAT, NULL, 0));
                    break;

                // show the compressor selector dialog
                case IDD_CompConfig:
                    capDlgVideoCompression(ghWndCap);
                    break;



                case IDOK :
                {

                    gCapParms.fCaptureAudio =
                                IsDlgButtonChecked(hDlg, IDD_CapAudioFlag) ;
                    gCapParms.fMCIControl =
                            IsDlgButtonChecked(hDlg, IDD_MCIControlFlag);
                    gCapParms.fLimitEnabled = IsDlgButtonChecked(hDlg, IDD_TimeLimitFlag) ;

                    GetDlgItemText(hDlg, IDD_FrameRateData, achBuffer, sizeof(achBuffer));
                    gCapParms.dwRequestMicroSecPerFrame = StringRateToMicroSec(achBuffer);
                    if (gCapParms.dwRequestMicroSecPerFrame == 0) {
                        gCapParms.dwRequestMicroSecPerFrame = DEF_CAPTURE_RATE;
                    }

                    GetDlgItemText(hDlg, IDD_SecondsData, achBuffer, sizeof(achBuffer));
                    if (gCapParms.fLimitEnabled) {
                         gCapParms.wTimeLimit  = (UINT) atol(achBuffer);
                    }

                    // fUsingDOSMemory is archaic and is now just a flag reflecting
                    // the "CaptureToDisk" selection.
                    // 
                    gCapParms.fUsingDOSMemory = 
                                IsDlgButtonChecked(hDlg, IDD_CaptureToDisk);

                    EndDialog(hDlg, TRUE) ;
                    return TRUE ;
                }

                case IDCANCEL :
                    EndDialog(hDlg, FALSE) ;
                    return TRUE ;
            }
            break ;

        case WM_VSCROLL:
        // message from one of the arrow spinbuttons
        {
            UINT id;

            id = GetDlgCtrlID(GET_WM_COMMAND_HWND(wParam, lParam));
            if (id == IDD_FrameRateArrow) {
                // format n.nnn
                MilliSecVarArrowEditChange(
                    GetDlgItem(hDlg, IDD_FrameRateData),
                    GET_WM_VSCROLL_CODE(wParam, lParam),
                    1, 100, 1);
            } else {
                // simple integer format
                ArrowEditChange(
                    GetDlgItem(hDlg, IDD_SecondsData),
                    GET_WM_VSCROLL_CODE(wParam, lParam),
                    1, 30000);
            }
            break;
        }

    }

    return FALSE ;
}

/*
 * preferences dialog - sets global options about background colour,
 * presence of toolbar, status bar etc
 */
LRESULT FAR PASCAL
PrefsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    DWORD indexsz;

    switch(message) {


    case WM_INITDIALOG:
        CheckDlgButton(hDlg, IDD_PrefsStatus, gbStatusBar);
        CheckDlgButton(hDlg, IDD_PrefsToolbar, gbToolBar);
        CheckDlgButton(hDlg, IDD_PrefsCentre, gbCentre);
        CheckDlgButton(hDlg, IDD_PrefsSizeFrame, gbAutoSizeFrame);
        CheckRadioButton(hDlg, IDD_PrefsDefBackground, IDD_PrefsBlack, gBackColour);

        CheckRadioButton(hDlg, IDD_PrefsSmallIndex, IDD_PrefsBigIndex,
                    (gCapParms.dwIndexSize == CAP_LARGE_INDEX) ?
                    IDD_PrefsBigIndex : IDD_PrefsSmallIndex);

        CheckRadioButton(hDlg, IDD_PrefsMasterAudio, IDD_PrefsMasterNone,
                    gCapParms.AVStreamMaster + IDD_PrefsMasterAudio);

        return(TRUE);

    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDCANCEL:
            EndDialog(hDlg, FALSE);
            return(TRUE);

        case IDOK:
            gbStatusBar = IsDlgButtonChecked(hDlg, IDD_PrefsStatus);
            gbToolBar = IsDlgButtonChecked(hDlg, IDD_PrefsToolbar);
            gbCentre = IsDlgButtonChecked(hDlg, IDD_PrefsCentre);
            gbAutoSizeFrame = IsDlgButtonChecked(hDlg, IDD_PrefsSizeFrame);

            if (IsDlgButtonChecked(hDlg, IDD_PrefsDefBackground)) {
                gBackColour = IDD_PrefsDefBackground;
            } else if (IsDlgButtonChecked(hDlg, IDD_PrefsLtGrey)) {
                gBackColour = IDD_PrefsLtGrey;
            } else if (IsDlgButtonChecked(hDlg, IDD_PrefsDkGrey)) {
                gBackColour = IDD_PrefsDkGrey;
            } else {
                gBackColour = IDD_PrefsBlack;
            }

            if (IsDlgButtonChecked(hDlg, IDD_PrefsSmallIndex)) {
                indexsz = CAP_SMALL_INDEX;

            } else {
                indexsz = CAP_LARGE_INDEX;
            }
            if (indexsz != gCapParms.dwIndexSize) {
                gCapParms.dwIndexSize = indexsz;
            }

            if (IsDlgButtonChecked(hDlg, IDD_PrefsMasterAudio)) {
                gCapParms.AVStreamMaster = AVSTREAMMASTER_AUDIO;
            }
            else {
                gCapParms.AVStreamMaster = AVSTREAMMASTER_NONE;
            }

            EndDialog(hDlg, TRUE);
            return(TRUE);
        }
        break;
    }
    return FALSE;
}


LRESULT FAR PASCAL
NoHardwareDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HBRUSH hbr;

    switch(message) {
    case WM_INITDIALOG:
        // lParam contains the argument to DialogBoxParam which is the
        // reason text
        SetDlgItemText(hDlg, IDD_FailReason, (LPTSTR) lParam);

        hbr = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
        return TRUE;

    case WM_DESTROY:
        DeleteObject(hbr);

#ifdef _WIN32
    case WM_CTLCOLORSTATIC:
#else
    case WM_CTLCOLOR:
#endif
        if (GET_WM_CTLCOLOR_HWND(wParam, lParam, message) == GetDlgItem(hDlg, IDD_FailReason)) {

            HDC hdc;

            hdc = GET_WM_CTLCOLOR_HDC(wParam, lParam, message);

            SetTextColor(hdc, RGB(0xff, 0, 0));
            SetBkColor(hdc, GetSysColor(COLOR_WINDOW));

            // in order to ensure that the text colour we have chosen for
            // this control is used, we need to actually return a brush.
            // for win31, we also need to align the brush
#ifndef _WIN32
            {
                POINT pt;

                pt.x = 0;
                pt.y = 0;
                ClientToScreen(hDlg, &pt);
                UnrealizeObject(hbr);
                SetBrushOrg(hdc, pt.x, pt.y);
            }
#endif

            return((INT_PTR) hbr);

        }
        break;

    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDOK:
            EndDialog(hDlg, TRUE);
            return(TRUE);
        case IDCANCEL:
            EndDialog(hDlg, FALSE);
            return(TRUE);
        }
        break;
    }

    return(FALSE);
}


//capture selected single frames
LRESULT
FAR PASCAL
CapFramesProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam)
{
    TCHAR ach[MAX_PATH*2];
    TCHAR achName[MAX_PATH];

    static BOOL bFirst;
    static int iFrames;

    switch(Message) {
    case WM_INITDIALOG:

        // write out the prompt message including the capture file name
        capFileGetCaptureFile(ghWndCap, achName, sizeof(achName));
        wsprintf(ach, tmpString(IDS_PROMPT_CAPFRAMES), achName);
        SetDlgItemText(hDlg, IDD_CapMessage, ach);

        bFirst = TRUE;

        //move dialog so it doesn't obscure the capture window
        SmartWindowPosition(hDlg, ghWndCap);

        return(TRUE);

    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam, lParam)) {

        case IDCANCEL:
            if (!bFirst) {
                capCaptureSingleFrameClose(ghWndCap);
                EndDialog(hDlg, TRUE);
            } else {
                EndDialog(hDlg, FALSE);
            }
            return(TRUE);

        case IDOK:
            if (bFirst) {
                bFirst = FALSE;
                iFrames = 0;
                capCaptureSingleFrameOpen(ghWndCap);

                SetDlgItemText(hDlg, IDCANCEL, tmpString(IDS_CAP_CLOSE));

            }
            capCaptureSingleFrame(ghWndCap);
            iFrames++;

            wsprintf(ach, tmpString(IDS_STATUS_NUMFRAMES), iFrames);
            SetDlgItemText(hDlg, IDD_CapNumFrames, ach);
            return(TRUE);

        }
        break;
    }
    return(FALSE);
}

// enumerate all the MCI devices of a particular type and add them and
// their descriptions to a combo box list.
//
void
AddMCIDeviceNames(UINT wDeviceType, HWND hwndCB)
{
    int   nIndex;
    MCI_OPEN_PARMS mciOp;
    MCI_INFO_PARMS mciIp;
    MCI_SYSINFO_PARMS mciSIP;
    MCI_GENERIC_PARMS mciGp;
    TCHAR buf[MAXPNAMELEN + 128]; // Contains eg. Name\t\tVideodisc1
    TCHAR buf2 [64];
    int maxdevs;
    DWORD dwRet;

    // To get the user readable names of the devices, we
    // must open all appropriate devices, and then get info.

    // MCI Open structure
    mciOp.dwCallback = 0;
    mciOp.lpstrElementName = NULL;
    mciOp.lpstrAlias = NULL;

    // MCI Info structure
    mciIp.dwCallback = 0;
    mciIp.lpstrReturn = (LPTSTR) buf;
    mciIp.dwRetSize = MAXPNAMELEN - 1;

    // MCI SysInfo structure
    mciSIP.dwCallback = 0;
    mciSIP.lpstrReturn = (LPTSTR) buf2;
    mciSIP.dwRetSize = sizeof (buf2);
    mciSIP.wDeviceType = wDeviceType;

    // MCI Generic structure
    mciGp.dwCallback = 0;

    // Load the combobox with the product info name, followed by
    // a comma, then a space, and then the mci device name. This allows a
    // single alphabetized list to be kept.

    // eg.
    // Pioneer Laserdisc, videodisc1

    maxdevs = CountMCIDevices((UINT)mciSIP.wDeviceType);
    for (nIndex = 0; nIndex < maxdevs; nIndex++) {

       // Get the system name eg. Videodisc1
       mciSIP.dwNumber = nIndex + 1;
       dwRet = mciSendCommand (0, MCI_SYSINFO,
                    MCI_SYSINFO_NAME,
                    (DWORD_PTR) (LPVOID) &mciSIP);

       mciOp.lpstrDeviceType =
            (LPTSTR) MAKELONG (wDeviceType, nIndex);

       if (!(dwRet = mciSendCommand(0, MCI_OPEN,
                    MCI_WAIT | MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID |
                    MCI_OPEN_SHAREABLE,
                    (DWORD_PTR) (LPVOID) &mciOp))) {
            if (!(dwRet = mciSendCommand (mciOp.wDeviceID, MCI_INFO,
                            MCI_WAIT | MCI_INFO_PRODUCT,
                            (DWORD_PTR) (LPVOID) &mciIp))) {
                lstrcat (buf, ", ");         // append the delimiter
                lstrcat (buf, buf2);         // append the system name
                // Whew, finally put it in the listbox
                SendMessage( hwndCB, CB_ADDSTRING, 0,
                                (LONG_PTR)(LPTSTR) buf);
            } //endif got INFO
            // Close it now
            mciSendCommand (mciOp.wDeviceID, MCI_CLOSE,
                            MCI_WAIT,
                            (DWORD_PTR) (LPVOID) &mciGp);
       } // endif OPEN
    } // endif for all devices of this type
}


//
// dialog proc to select MCI device and parameters, including start,
// stop times.
LRESULT FAR PASCAL
MCISetupProc(HWND hwnd, unsigned msg, WPARAM wParam, LPARAM lParam)
{
  HWND  hwndCB;
  DWORD dw;
  TCHAR buf[MAXPNAMELEN];
  BOOL f;
  int j;
  static int nLastCBIndex = 0;
  static DWORD tdwMCIStartTime;
  static DWORD tdwMCIStopTime;



  switch (msg) {
    case WM_INITDIALOG:

      	CheckRadioButton(hwnd, IDD_MCI_PLAY, IDD_MCI_STEP,
			    gCapParms.fStepMCIDevice ?
                            IDD_MCI_STEP : IDD_MCI_PLAY );

        // enable averaging options only in step mode
        EnableWindow (GetDlgItem (hwnd, IDD_MCI_AVERAGE_2X), gCapParms.fStepMCIDevice);
        EnableWindow (GetDlgItem (hwnd, IDD_MCI_AVERAGE_FR), gCapParms.fStepMCIDevice);
	SetDlgItemInt(hwnd, IDD_MCI_AVERAGE_FR, gCapParms.wStepCaptureAverageFrames, FALSE);
        CheckDlgButton (hwnd, IDD_MCI_AVERAGE_2X, gCapParms.fStepCaptureAt2x);

        // save current dialog time settings
        tdwMCIStartTime = gCapParms.dwMCIStartTime;
        tdwMCIStopTime  = gCapParms.dwMCIStopTime;

        TimeMSToHMSString (gCapParms.dwMCIStartTime, buf);
        SetDlgItemText (hwnd, IDD_MCI_STARTTIME, buf);
        TimeMSToHMSString (gCapParms.dwMCIStopTime, buf);
        SetDlgItemText (hwnd, IDD_MCI_STOPTIME, buf);


        // fill combo box with list of MCI devices
	hwndCB = GetDlgItem( hwnd, IDD_MCI_SOURCE );
        AddMCIDeviceNames(MCI_DEVTYPE_VIDEODISC, hwndCB);
        AddMCIDeviceNames(MCI_DEVTYPE_VCR, hwndCB);


        // set the selection to whatever he chose last time through this dlg
        // default is the first entry.
       	SendMessage( hwndCB, CB_SETCURSEL, nLastCBIndex, 0L);
	break;

    case WM_COMMAND:
	switch (GET_WM_COMMAND_ID(wParam, lParam)) {
	    case IDOK:
                // i think the point of this is to ensure that
                // the KILLFOCUS processing for the edit boxes has been done
                // and thus the temp times are the same as the dialog text
                SetFocus(GET_WM_COMMAND_HWND(wParam, lParam));


                MCIGetDeviceNameAndIndex (hwnd, &nLastCBIndex, gachMCIDeviceName);
                capSetMCIDeviceName(ghWndCap, gachMCIDeviceName) ;
                gCapParms.fStepMCIDevice = IsDlgButtonChecked (hwnd, IDD_MCI_STEP);

                // pick up the temp times - these were set on KILLFOCUS msgs
                // (when we did validation and string->dword conversion
                gCapParms.dwMCIStartTime = tdwMCIStartTime;
                gCapParms.dwMCIStopTime  = tdwMCIStopTime;

                gCapParms.fStepCaptureAt2x = IsDlgButtonChecked (hwnd, IDD_MCI_AVERAGE_2X);
                gCapParms.wStepCaptureAverageFrames = GetDlgItemInt (hwnd, IDD_MCI_AVERAGE_FR, NULL, FALSE);

		EndDialog(hwnd, TRUE);
		break;
		
	    case IDCANCEL:
		EndDialog(hwnd, 0);
		break;

            case IDD_MCI_STEP:
            case IDD_MCI_PLAY:
                //averaging only enabled in play mode
                f = IsDlgButtonChecked (hwnd, IDD_MCI_STEP);
                EnableWindow (GetDlgItem (hwnd, IDD_MCI_AVERAGE_2X), f);
                EnableWindow (GetDlgItem (hwnd, IDD_MCI_AVERAGE_FR), f);
                break;

            case IDD_MCI_AVERAGE_FR:
                // validate the count of frames to average 1..100
                if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_KILLFOCUS) {
                    j = GetDlgItemInt(hwnd,
                            GET_WM_COMMAND_ID(wParam, lParam), NULL, FALSE);
                    // Limit frames to average between 1 and 100
                    if (j < 1 || j > 100) {
	                SetDlgItemInt (hwnd,
                            GET_WM_COMMAND_ID(wParam, lParam), 1, FALSE);
                    }
                }
                break;

            case IDD_MCI_STARTSET:
	    case IDD_MCI_STOPSET:
                // set the start or stop time to be the time
                // on the device right now.

                // MCI devices could yield and cause us to re-enter - the
                // simplest answer seems to be to disable the dialog
                EnableWindow(hwnd, FALSE);

                MCIGetDeviceNameAndIndex (hwnd, &nLastCBIndex, buf);

                if (MCIDeviceOpen (buf)) {
                    if (GET_WM_COMMAND_ID(wParam, lParam) == IDD_MCI_STARTSET) {
                        if (MCIDeviceGetPosition (&tdwMCIStartTime)) {
                           TimeMSToHMSString (tdwMCIStartTime, buf);
                           SetDlgItemText (hwnd, IDD_MCI_STARTTIME, buf);
                        }
                        else {
                            MessageBoxID(IDS_MCI_CONTROL_ERROR,
                                        MB_OK|MB_ICONEXCLAMATION);
                        }
                    }
                    else {
                        if (MCIDeviceGetPosition (&tdwMCIStopTime)) {
                            TimeMSToHMSString (tdwMCIStopTime, buf);
                            SetDlgItemText (hwnd, IDD_MCI_STOPTIME, buf);
                        }
                        else {
                            MessageBoxID(IDS_MCI_CONTROL_ERROR,
                                        MB_OK|MB_ICONEXCLAMATION);
                        }
                    }
                    MCIDeviceClose ();

                } else {
                    // cant open device
                    MessageBoxID(IDS_MCI_CONTROL_ERROR,
                                MB_OK|MB_ICONEXCLAMATION);
                }
                EnableWindow(hwnd, TRUE);
                break;


            case IDD_MCI_STARTTIME:
            case IDD_MCI_STOPTIME:
                if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_KILLFOCUS) {
                    GetDlgItemText (hwnd,
                        GET_WM_COMMAND_ID(wParam, lParam), buf, sizeof (buf));
                    if ((dw = TimeHMSStringToMS (buf)) == -1) {
                        // Error in string, reset
                        MessageBeep (0);
                        if (GET_WM_COMMAND_ID(wParam, lParam) == IDD_MCI_STARTTIME)
                            dw = tdwMCIStartTime;
                        else
                            dw = tdwMCIStopTime;
                    }
                    if (GET_WM_COMMAND_ID(wParam, lParam) == IDD_MCI_STARTTIME) {
                        tdwMCIStartTime = dw;
                        TimeMSToHMSString (tdwMCIStartTime, buf);
                        SetDlgItemText (hwnd, IDD_MCI_STARTTIME, buf);
                    }
                    else {
                        tdwMCIStopTime = dw;
                        TimeMSToHMSString (tdwMCIStopTime, buf);
                        SetDlgItemText (hwnd, IDD_MCI_STOPTIME, buf);
                    }
                }
                break;
	}
	break;

  }
  return FALSE;
}

