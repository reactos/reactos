/****************************************************************************
 *
 *   capmci.c
 * 
 *   Control of MCI devices during capture.
 *
 *   Microsoft Video for Windows Sample Capture Class
 *
 *   Copyright (c) 1992, 1993 Microsoft Corporation.  All Rights Reserved.
 *
 *    You have a royalty-free right to use, modify, reproduce and 
 *    distribute the Sample Files (and/or any modified version) in 
 *    any way you find useful, provided that you agree that 
 *    Microsoft has no warranty obligations or liability for any 
 *    Sample Application Files which are modified. 
 *
 ***************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <memory.h>         // for _fmemset 
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <msvideo.h>
#include <drawdib.h>
#include <mmddk.h>

#include "avicap.h"
#include "avicapi.h"        

#ifdef _DEBUG
    #define DSTATUS(lpcs, sz) statusUpdateStatus(lpcs, IDS_CAP_INFO, (LPSTR) sz)
#else
    #define DSTATUS(lpcs, sz) 
#endif

DWORD SendDriverFormat (LPCAPSTREAM lpcs, LPBITMAPINFOHEADER lpbih, DWORD dwInfoHeaderSize);

/*--------------------------------------------------------------+
| TimeMSToHMSString() - change milliseconds into SMPTE time     |
+--------------------------------------------------------------*/
void FAR PASCAL TimeMSToSMPTE (DWORD dwMS, LPSTR lpTime)
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
	/* KLUDGE, force hundredths to SMPTE approximation of PAL frames */
	wsprintf((char far *)lpTime, "%02u:%02u:%02u:%02lu", wHours, wMins,
		    wSecs, (lHundredths * 25) / 100);
}


/*--------------------------------------------------------------+
| START OF MCI CONTROL SECTION                                 |
+--------------------------------------------------------------*/

/*
 *  CountMCIDevicesByType 
 *      Returns a count of the number of VCR or Videodisc
 *      devices that MCI claims to know about.
 */

int CountMCIDevicesByType ( WORD wType )
{
   int nTotal;
   DWORD dwCount;
   MCI_SYSINFO_PARMS mciSIP;

   mciSIP.dwCallback = NULL;
   mciSIP.lpstrReturn = (LPSTR) (LPVOID) &dwCount;
   mciSIP.dwRetSize = sizeof (dwCount);

   mciSIP.wDeviceType = wType;
   
   if (!mciSendCommand (NULL, MCI_SYSINFO, MCI_SYSINFO_QUANTITY,
        (DWORD) (LPVOID) &mciSIP))
       nTotal = (int) *( (LPDWORD) mciSIP.lpstrReturn);

   return nTotal;
}

/*
 *  MCIDeviceClose
 *      This routine closes the open MCI device.
 */

void MCIDeviceClose (LPCAPSTREAM lpcs)
{
    mciSendString( "close mciframes", NULL, 0, NULL );
}

/*
 *  MCIDeviceOpen
 *      This routine opens the mci device for use, and sets the
 *      time format to milliseconds.
 *      Return FALSE on error;
 */

BOOL MCIDeviceOpen (LPCAPSTREAM lpcs)
{
    char        ach[160];

    wsprintf( ach, "open %s shareable alias mciframes",
                (LPSTR) lpcs-> achMCIDevice);
    lpcs-> dwMCIError = mciSendString( ach, NULL, 0, NULL );
    if( lpcs-> dwMCIError ) {
        DPF (" MCI Error, open %s shareable alias mciframes", lpcs-> achMCIDevice);
        goto err_return;
    }    
    lpcs-> dwMCIError = mciSendString( "set mciframes time format milliseconds", 
        NULL, 0, NULL );
    if( lpcs-> dwMCIError ) {
        DPF (" MCI Error, set mciframes time format milliseconds");
        goto err_close;
    }
    return ( TRUE );
   
err_close:
    MCIDeviceClose (lpcs);
err_return:
    return ( FALSE );
}


/*
 *  MCIDeviceGetPosition
 *      Stores the current device position in milliseconds in lpdwPos.
 *      Returns TRUE on success, FALSE if error.
 */
BOOL FAR PASCAL MCIDeviceGetPosition (LPCAPSTREAM lpcs, LPDWORD lpdwPos)
{
    char        ach[80];
    LPSTR       p;
    LONG        lv;
        
    lpcs-> dwMCIError = mciSendString( "status mciframes position wait", 
        ach, sizeof(ach), NULL );
    if( lpcs-> dwMCIError ) {
        DPF (" MCI Error, status mciframes position wait");
        *lpdwPos = 0L;
        return FALSE;
    }

    p = ach; 
    
    while (*p == ' ') p++;
    for (lv = 0; *p >= '0' && *p <= '9'; p++)
        lv = (10 * lv) + (*p - '0');
    *lpdwPos = lv;
    return TRUE;
}

/*
 *  MCIDeviceSetPosition
 *      Sets the current device position in milliseconds.
 *      Returns TRUE on success, FALSE if error.
 */
BOOL FAR PASCAL MCIDeviceSetPosition (LPCAPSTREAM lpcs, DWORD dwPos)
{
    char        achCommand[40];
    char        ach[80];
    
    lpcs-> dwMCIError = mciSendString( "pause mciframes wait", ach, sizeof(ach), NULL );
    if (lpcs-> dwMCIError) {
        DPF (" MCI Error, pause mciframes wait");
        return FALSE;
    }    
    wsprintf(achCommand, "seek mciframes to %ld wait", dwPos);
    lpcs-> dwMCIError = mciSendString( achCommand, ach, sizeof(ach), NULL );
    if (lpcs-> dwMCIError)
        DPF (" MCI Error, seek mciframes to %ld wait", dwPos);
    return ( lpcs-> dwMCIError == 0 ? TRUE : FALSE );
}


/*
 *  MCIDevicePlay
 *      Start playing the current MCI device from the current position
 *      Returns TRUE on success, FALSE if error.
 */
BOOL FAR PASCAL MCIDevicePlay (LPCAPSTREAM lpcs)
{
    char        ach[80];
    
    lpcs-> dwMCIError = mciSendString( "play mciframes", ach, sizeof(ach), NULL );
    if (lpcs-> dwMCIError)
        DPF (" MCI Error, play mciframes");
    return ( lpcs-> dwMCIError == 0 ? TRUE : FALSE );
}

/*
 *  MCIDevicePause
 *      Pauses the current MCI device at the current position
 *      Returns TRUE on success, FALSE if error.
 */
BOOL FAR PASCAL MCIDevicePause (LPCAPSTREAM lpcs)
{
    char        ach[80];
    
    lpcs-> dwMCIError = mciSendString( "pause mciframes wait", ach, sizeof(ach), NULL );
    if (lpcs-> dwMCIError)
        DPF (" MCI Error, pause mciframes wait");
    return ( lpcs-> dwMCIError == 0 ? TRUE : FALSE );
}

/*
 *  MCIDeviceStop
 *      Stops the current MCI device
 *      Returns TRUE on success, FALSE if error.
 */
BOOL FAR PASCAL MCIDeviceStop (LPCAPSTREAM lpcs)
{
    char        ach[80];
    
    lpcs-> dwMCIError = mciSendString( "stop mciframes wait", ach, sizeof(ach), NULL );
    if (lpcs-> dwMCIError)
        DPF (" MCI Error, stop mciframes wait");
    return ( lpcs-> dwMCIError == 0 ? TRUE : FALSE );
}

/*
 *  MCIDeviceStep
 *      Step the current MCI at the current position
 *      Returns TRUE on success, FALSE if error.
 */
BOOL FAR PASCAL MCIDeviceStep (LPCAPSTREAM lpcs, BOOL fForward)
{
    char        ach[80];
    
    lpcs-> dwMCIError = mciSendString( fForward ? "step mciframes wait" : 
                "step mciframes reverse wait", ach, sizeof(ach), NULL );
    if (lpcs-> dwMCIError)
        DPF (" MCI Error, step mciframes wait");
    return ( lpcs-> dwMCIError == 0 ? TRUE : FALSE );
}

/*
 *  MCIDeviceFreeze
 *      freeze the current frame
 *      Returns TRUE on success, FALSE if error.
 */
BOOL FAR PASCAL MCIDeviceFreeze(LPCAPSTREAM lpcs, BOOL fFreeze)
{
    lpcs-> dwMCIError = mciSendString( fFreeze ? "freeze mciframes wait" : 
                "unfreeze mciframes wait", NULL, 0, NULL);
    if (lpcs-> dwMCIError)
        DPF (" MCI Error, freeze mciframes wait");
    return ( lpcs-> dwMCIError == 0 ? TRUE : FALSE );
}


/*
 *  MCIStepCapture
 *      Main routine for performing MCI step capture.
 *      
 */
void FAR PASCAL _loadds MCIStepCapture (LPCAPSTREAM lpcs)
{
    BOOL        fOK = TRUE;
    BOOL        fT;
    BOOL        fKey;
    BOOL        fStopping;         // True when finishing capture
    DWORD       dw;
    WORD        w;
    WORD        wError;         // Error String ID
    LPVIDEOHDR  lpVidHdr;
    LPWAVEHDR   lpWaveHdr;
    DWORD       dwTimeToStop;   // Lesser of MCI capture time or frame limit
    BOOL        fTryToPaint = FALSE;
    HDC         hdc;
    HPALETTE    hpalT;
    RECT        rcDrawRect;
    LONG        lSize;

    statusUpdateStatus(lpcs, IDS_CAP_BEGIN);  // Always the first message

    // Verify capture parameters
    if ((!lpcs->sCapParms.fMCIControl) ||
        (!lpcs->sCapParms.fStepMCIDevice))
        goto EarlyExit;

    lpcs->MCICaptureState = CAPMCI_STATE_Uninitialized;

    lpcs-> fCapturingNow = TRUE;
    lpcs-> fStepCapturingNow = TRUE;
    lpcs-> dwReturn = DV_ERR_OK;

    // If not 1 Meg. free, give it up!!!
    if (GetFreePhysicalMemory () < (1024L * 1024L)) {
        errorUpdateError (lpcs, IDS_CAP_OUTOFMEM);
        lpcs-> dwReturn = IDS_CAP_OUTOFMEM;
        goto EarlyExit;
    }

    statusUpdateStatus(lpcs, IDS_CAP_STAT_CAP_INIT);

    // Try painting the DIB only if Live window
    fTryToPaint = lpcs->fLiveWindow;

    if (fTryToPaint) {
        hdc = GetDC(lpcs->hwnd);
        SetWindowOrg(hdc, lpcs->ptScroll.x, lpcs->ptScroll.y);
        hpalT = DrawDibGetPalette (lpcs->hdd);
        if (hpalT)
            hpalT = SelectPalette( hdc, hpalT, FALSE);
        RealizePalette(hdc);
        if (lpcs-> fScale)
            GetClientRect (lpcs->hwnd, &rcDrawRect);
        else
            SetRect (&rcDrawRect, 0, 0, lpcs->dxBits, lpcs->dyBits);
    }

    // -------------------------------------------------------
    //   When should capture stop?
    // -------------------------------------------------------

    // If using MCI, capture for the shorter of the MCI period,
    // or the capture limit

    if (lpcs->sCapParms.fLimitEnabled)
        dwTimeToStop = (DWORD) ((DWORD) 1000 * lpcs->sCapParms.wTimeLimit);
    else
        dwTimeToStop = (DWORD) -1L; // very large

    if (lpcs->sCapParms.fMCIControl) {
        // if MCI stop time not given, use lpcs->sCapParms.wTimeLimit
        if (lpcs->sCapParms.dwMCIStopTime == lpcs->sCapParms.dwMCIStartTime)
                    lpcs->sCapParms.dwMCIStopTime = lpcs->sCapParms.dwMCIStartTime +
                    (DWORD) ((DWORD)1000 * lpcs->sCapParms.wTimeLimit);

        dw = lpcs->sCapParms.dwMCIStopTime - lpcs->sCapParms.dwMCIStartTime;

        if (lpcs->sCapParms.fLimitEnabled) 
            dwTimeToStop = min (dw, dwTimeToStop);
        else 
            dwTimeToStop = dw;
    }

    //
    // never ever try to capture more than the index size!
    //
    if (lpcs->fCapturingToDisk) {
        dw = muldiv32(lpcs->sCapParms.dwIndexSize,
                lpcs->sCapParms.dwRequestMicroSecPerFrame,
                1000l);
        dwTimeToStop = min (dw, dwTimeToStop);
    }

    fOK = FALSE;            // Assume the worst
    if (MCIDeviceOpen (lpcs)) {
        if (MCIDeviceSetPosition (lpcs, lpcs->sCapParms.dwMCIStartTime))
            if (MCIDeviceStep (lpcs, TRUE))
                fOK = TRUE;
    }
    if (!fOK) {
        errorUpdateError (lpcs, IDS_CAP_MCI_CONTROL_ERROR);
        statusUpdateStatus(lpcs, NULL);    // Clear status
        lpcs-> dwReturn = IDS_CAP_MCI_CONTROL_ERROR;
        goto EarlyExit;
    }

    // -------------------------------------------------------
    //  Spatial and temporal averaging
    // -------------------------------------------------------

    // Frame Averaging, capture the same frame multiple times...
    lpcs->lpia = NULL;
    if (lpcs->sCapParms.wStepCaptureAverageFrames == 0)
        lpcs->sCapParms.wStepCaptureAverageFrames = 1;

    // Only allow averaging if an RGB format
    if (lpcs->lpBitsInfo->bmiHeader.biCompression != BI_RGB)
        lpcs->sCapParms.wStepCaptureAverageFrames = 1;

    // 2x Scaling
    lpcs->lpbmih2x = NULL;
    lpcs->VidHdr2x = lpcs->VidHdr;        // Init the 2x copy 

    if (lpcs->sCapParms.fStepCaptureAt2x && 
                lpcs->lpBitsInfo->bmiHeader.biCompression == BI_RGB) {
        lpcs->VidHdr2x.lpData = NULL;
        lpcs->lpbmih2x = (LPBITMAPINFOHEADER) GlobalAllocPtr (GHND, 
                sizeof (BITMAPINFOHEADER) +
                256 * sizeof (RGBQUAD));
        _fmemcpy (lpcs->lpbmih2x, lpcs->lpBitsInfo, sizeof (BITMAPINFOHEADER) +
                256 * sizeof (RGBQUAD));

        // Try to force the driver into 2x mode
        lpcs->lpbmih2x->biHeight    *= 2;
        lpcs->lpbmih2x->biWidth     *= 2;
        lpcs->lpbmih2x->biSizeImage *= 4;
        if (!SendDriverFormat (lpcs, lpcs->lpbmih2x, sizeof (BITMAPINFOHEADER))) {
            // Success, allocate new bitspace
            lpcs->VidHdr2x.lpData = GlobalAllocPtr (GHND, 
                        lpcs->lpbmih2x->biSizeImage);
            lpcs->VidHdr2x.dwBufferLength = lpcs->lpbmih2x->biSizeImage;
        }

        // Something went wrong, no memory, or driver failed request
        // so revert back to original settings
        if (!lpcs->VidHdr2x.lpData) {
            SendDriverFormat (lpcs, (LPBITMAPINFOHEADER) lpcs->lpBitsInfo, 
                sizeof (BITMAPINFOHEADER));
            lpcs->sCapParms.fStepCaptureAt2x = FALSE;
            lpcs->VidHdr2x = lpcs->VidHdr;        // Back to the original settings
        }
    }
    else
        lpcs->sCapParms.fStepCaptureAt2x = FALSE;

    DPF (" StepCaptureAt2x = %d\r\n", (int) lpcs->sCapParms.fStepCaptureAt2x);

    //
    // If we're compressing while capturing, warm up the compressor
    //
    if (lpcs->CompVars.hic) {
        if (ICSeqCompressFrameStart(&lpcs->CompVars, lpcs->lpBitsInfo) == NULL) {

	    // !!! We're in trouble here!  
            dprintf("ICSeqCompressFrameStart failed !!!\n");
            lpcs-> dwReturn = IDS_CAP_COMPRESSOR_ERROR;
            errorUpdateError (lpcs, IDS_CAP_COMPRESSOR_ERROR);
            goto EarlyExit;
        }
        // Kludge, offset the lpBitsOut ptr 
        // Compman allocates the compress buffer too large by 
        // 2048 + 16 so we will still have room 
        ((LPBYTE) lpcs->CompVars.lpBitsOut) += 8;
    }

    // No compression desired
    if (!lpcs->CompVars.hic)
	WinAssert(lpcs->CompVars.lpbiOut == NULL);

    // -------------------------------------------------------
    //  Open the output file
    // -------------------------------------------------------

    if (lpcs->fCapturingToDisk) {
        if (!AVIFileInit(lpcs)) {
            lpcs-> dwReturn = IDS_CAP_FILE_OPEN_ERROR;
            errorUpdateError (lpcs, IDS_CAP_FILE_OPEN_ERROR);
            goto EarlyExit;
        }
    }    

    /* Make sure the parent has been repainted */
    UpdateWindow(lpcs->hwnd);

    //
    // AVIInit will allocate sound buffers, but not video buffers
    // when performing step capture.
    //

    wError = AVIInit(lpcs);

    if (wError) {
        lpcs->sCapParms.fUsingDOSMemory = FALSE;
        wError = AVIInit(lpcs);
    }

    if (wError) {
        /* Error in initalization - return */
        errorUpdateError (lpcs, wError);
        AVIFini(lpcs);  
        AVIFileFini(lpcs, TRUE /* fWroteJunkChunks */, TRUE /* fAbort */);
        statusUpdateStatus(lpcs, NULL);    // Clear status
        lpcs-> dwReturn = wError;
        goto EarlyExit;
    }

    /* update the status, so the user knows how to stop */
    statusUpdateStatus(lpcs, IDS_CAP_SEQ_MSGSTOP);
    UpdateWindow(lpcs->hwnd);


    if (lpcs->sCapParms.fStepCaptureAt2x || (lpcs->sCapParms.wStepCaptureAverageFrames != 1)) {
        LPIAVERAGE FAR * lppia = (LPIAVERAGE FAR *) &lpcs->lpia;

        statusUpdateStatus (lpcs, IDS_CAP_STAT_PALETTE_BUILD);
        if (!iaverageInit (lppia, lpcs->lpBitsInfo, lpcs->hPalCurrent)) {
            lpcs-> dwReturn = IDS_CAP_OUTOFMEM;
            goto CompressFrameFailure;                
        }
        statusUpdateStatus(lpcs, NULL);
    }
    DPF (" Averaging %d frames\r\n", lpcs->sCapParms.wStepCaptureAverageFrames);

    GetAsyncKeyState(lpcs->sCapParms.vKeyAbort);
    GetAsyncKeyState(VK_ESCAPE);
    GetAsyncKeyState(VK_LBUTTON);
    GetAsyncKeyState(VK_RBUTTON);


    // -------------------------------------------------------
    //   MAIN VIDEO CAPTURE LOOP
    // -------------------------------------------------------

    fOK=TRUE;             // Set FALSE on write errors
    fStopping = FALSE;    // TRUE when we need to stop

    lpVidHdr  = &lpcs->VidHdr;
    lpWaveHdr = lpcs->alpWaveHdr[lpcs->iNextWave];

    lpcs->MCICaptureState = CAPMCI_STATE_Initialized;
    lpcs->dwTimeElapsedMS = 0;

    // Move back to the starting position
    MCIDeviceSetPosition (lpcs, lpcs->sCapParms.dwMCIStartTime);
    MCIDevicePause (lpcs);

    // Where are we *really*
    MCIDeviceGetPosition (lpcs, &lpcs->dwMCIActualStartMS);

    // freeze video
    MCIDeviceFreeze(lpcs, TRUE);
    
    while (lpcs->MCICaptureState != CAPMCI_STATE_AllFini) {

        // -------------------------------------------------------
        //   is there any reason to stop or change states
        // -------------------------------------------------------
        if (lpcs->sCapParms.vKeyAbort) {
            if (GetAsyncKeyState(lpcs->sCapParms.vKeyAbort & 0x00ff) & 0x0001) {
                fT = TRUE;
                if (lpcs->sCapParms.vKeyAbort & 0x8000)  // Ctrl?
                    fT = fT && (GetAsyncKeyState(VK_CONTROL) & 0x8000);
                if (lpcs->sCapParms.vKeyAbort & 0x4000)  // Shift?
                    fT = fT && (GetAsyncKeyState(VK_SHIFT) & 0x8000);
                fStopping = fT;      // User aborts
            }
        }
#if 0
        // Ignore Left mouse on MCI Capture!!!
        if (lpcs->sCapParms.fAbortLeftMouse)
            if (GetAsyncKeyState(VK_LBUTTON) & 0x0001)
                fStopping = TRUE;      // User aborts
#endif

        if (lpcs->sCapParms.fAbortRightMouse)
            if (GetAsyncKeyState(VK_RBUTTON) & 0x0001)
                fStopping = TRUE;      // User aborts
        if (lpcs-> fAbortCapture) {
            fStopping = TRUE;          // Somebody above wants us to quit
        }
        if (lpcs-> dwTimeElapsedMS > dwTimeToStop)
            fStopping = TRUE;      // all done


        // -------------------------------------------------------
        //    State machine 
        // -------------------------------------------------------
        switch (lpcs-> MCICaptureState) {

        case CAPMCI_STATE_Initialized:
            // Begin video step capture
            DSTATUS(lpcs, "MCIState: Initialized");
            lpcs->MCICaptureState = CAPMCI_STATE_StartVideo;
            break;

        case CAPMCI_STATE_StartVideo:
            // Begin video step capture
            lpcs->dwTimeElapsedMS = 0;
            lpcs->MCICaptureState = CAPMCI_STATE_CapturingVideo;
            break;

        case CAPMCI_STATE_CapturingVideo:
            // In the state of capturing video
            if (lpcs-> fStopCapture || lpcs-> fAbortCapture)
                fStopping = TRUE;

            if (fStopping) {
                MCIDeviceGetPosition (lpcs, &lpcs->dwMCIActualEndMS);
                MCIDevicePause (lpcs);

                DSTATUS(lpcs, "MCIState: StoppingVideo");

                if (fOK && !lpcs-> fAbortCapture)
                    lpcs->MCICaptureState = CAPMCI_STATE_VideoFini;
                else
                    lpcs->MCICaptureState = CAPMCI_STATE_AllFini;

                lpcs-> fStopCapture  = FALSE;
                lpcs-> fAbortCapture = FALSE;
                fStopping = FALSE;
            }
            break;

        case CAPMCI_STATE_VideoFini:
            // Wait for all buffers to be returned from the driver
            // Then move on to audio capture
            lpcs->MCICaptureState = CAPMCI_STATE_StartAudio;
            DSTATUS(lpcs, "MCIState: VideoFini");
            break;

        case CAPMCI_STATE_StartAudio:
            // If no audio, go to AllFini state
            if (!lpcs->sCapParms.fCaptureAudio || !fOK) {
                lpcs->MCICaptureState = CAPMCI_STATE_AllFini;
                break;
            }

            // Move back to the starting position
            MCIDeviceSetPosition (lpcs, lpcs->dwMCIActualStartMS);
            MCIDeviceGetPosition (lpcs, &lpcs->dwMCICurrentMS);
            DSTATUS(lpcs, "MCIState: StartAudio");
            MCIDevicePlay (lpcs);
            waveInStart(lpcs->hWaveIn);
            lpcs->MCICaptureState = CAPMCI_STATE_CapturingAudio;
            lpcs->dwTimeElapsedMS = 0;
            fStopping = FALSE;
            break;

        case CAPMCI_STATE_CapturingAudio:
            // In the state of capturing audio
            if (lpcs-> fStopCapture || lpcs-> fAbortCapture)
                fStopping = TRUE;

            MCIDeviceGetPosition (lpcs, &lpcs->dwMCICurrentMS);
            if (lpcs->dwMCICurrentMS + 100 > lpcs->dwMCIActualEndMS)
                fStopping = TRUE;
            if (fStopping) {
                waveInStop(lpcs->hWaveIn);
                MCIDevicePause (lpcs);
                waveInReset(lpcs->hWaveIn);
                lpcs->MCICaptureState = CAPMCI_STATE_AudioFini;
            }
            break;

        case CAPMCI_STATE_AudioFini:
            // While more audio buffers to process
            if (lpWaveHdr-> dwFlags & WHDR_DONE)
                break;
            lpcs->MCICaptureState = CAPMCI_STATE_AllFini;
            break;

        case CAPMCI_STATE_AllFini:
            DSTATUS(lpcs, "MCIState: AllFini");
            if (fOK)
                statusUpdateStatus(lpcs, IDS_CAP_STAT_CAP_FINI, lpcs->dwVideoChunkCount);
            else 
                statusUpdateStatus(lpcs, IDS_CAP_RECORDING_ERROR2);
            break;
        }

        // -------------------------------------------------------
        //        If we are in the video capture phase
        // -------------------------------------------------------

        if (lpcs->MCICaptureState == CAPMCI_STATE_CapturingVideo) {

            // if averaging...
            if (lpcs-> lpia) {
                int j;

                iaverageZero (lpcs-> lpia);

                // sum together a bunch of frames
                for (j = 0; j < (int)lpcs->sCapParms.wStepCaptureAverageFrames; j++) {
                        
	            videoFrame( lpcs-> hVideoIn, &lpcs-> VidHdr2x);

    	            // Shrink by 2x??
                    if (lpcs-> sCapParms.fStepCaptureAt2x) {
                        CrunchDIB(
                            lpcs-> lpia,        // image averaging structure
                            (LPBITMAPINFOHEADER)  lpcs-> lpbmih2x,  // BITMAPINFO src
                            (LPVOID) lpcs-> VidHdr2x.lpData,      // input bits 
                            (LPBITMAPINFOHEADER)  lpcs->lpBitsInfo, // BITMAPINFO dst
                            (LPVOID) lpcs->VidHdr.lpData);       // output bits
                    }
                    iaverageSum (lpcs-> lpia, lpcs->lpBits);
                }
                iaverageDivide (lpcs-> lpia, lpcs->lpBits);
            }
            // otherwise, not averaging, just get a frame
            else {
	        videoFrame( lpcs-> hVideoIn, &lpcs->VidHdr);
            }                           
                                    
            if (lpcs->CallbackOnVideoFrame)
                (*(lpcs->CallbackOnVideoFrame)) (lpcs->hwnd, &lpcs->VidHdr);

            // Update the display
	    InvalidateRect(lpcs->hwnd, NULL, TRUE);
	    UpdateWindow(lpcs->hwnd);
	    
            if (lpcs-> fCapturingToDisk) {
                if (!SingleFrameWrite (lpcs, lpVidHdr, &fKey, &lSize)) {
                    fOK = FALSE;
                    fStopping = TRUE;
                    // "ERROR: Could not write to file."
                    errorUpdateError(lpcs, IDS_CAP_FILE_WRITE_ERROR);
                } 
                else {
                    if (!IndexVideo(lpcs, lSize, fKey))
                        fStopping = TRUE;
                }
            } // endif fCapturingToDisk
            // Warning: Kludge to create frame chunk count when net capture
            // follows.
            else
                lpcs->dwVideoChunkCount++;
            
            // if there is still more time, (or at least every 100 frames) 
            // show status if we're not ending the capture
            if ((!fStopping) && (lpcs-> fCapturingToDisk) &&
                    (lpcs->dwVideoChunkCount)) {

                // "Captured %ld frames (Dropped %ld) %d.%03d sec. Hit Escape to Stop"
                statusUpdateStatus(lpcs, IDS_CAP_STAT_VIDEOCURRENT, 
                        lpcs->dwVideoChunkCount, lpcs->dwFramesDropped,
                        (int)(lpcs-> dwTimeElapsedMS/1000),
                        (int)(lpcs-> dwTimeElapsedMS%1000)
                        );
            } // endif next buffer not ready

            // Move the MCI source to the next capture point
            // unfreeze video
            MCIDeviceFreeze(lpcs, FALSE);
            for (;;) {
                MCIDeviceGetPosition (lpcs, &lpcs->dwMCICurrentMS);
                if (lpcs->dwMCICurrentMS > ((DWORD) (lpcs->dwMCIActualStartMS + 
                          muldiv32 (lpcs->dwVideoChunkCount,
                                lpcs->sCapParms.dwRequestMicroSecPerFrame,
                                1000L))))
                    break;
                MCIDeviceStep (lpcs, TRUE);
            }
            // freeze video
            MCIDeviceFreeze(lpcs, TRUE);
            lpcs-> dwTimeElapsedMS =
                    lpcs->dwMCICurrentMS - lpcs->dwMCIActualStartMS;

             /* return the emptied buffer to the que */
            lpVidHdr->dwFlags &= ~VHDR_DONE;
        }

        if (lpcs-> CallbackOnYield) {
            // If the yield callback returns FALSE, abort
            if (!((*(lpcs->CallbackOnYield)) (lpcs->hwnd)))
                fStopping = TRUE;
        }

        if (lpcs->sCapParms.fYield) { 
            MSG msg;

            if (PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
                // Kludge to get rid of timers from lpcs->hwnd
                if (msg.message == WM_TIMER && msg.hwnd == lpcs->hwnd)
                    ;
                else {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }

        // -------------------------------------------------------
        //        Is audio buffer ready to be written?
        // -------------------------------------------------------
        if (lpcs->sCapParms.fCaptureAudio && 
                (lpcs-> MCICaptureState == CAPMCI_STATE_CapturingAudio ||
                lpcs-> MCICaptureState == CAPMCI_STATE_StartAudio ||
                lpcs-> MCICaptureState == CAPMCI_STATE_AudioFini)) {
            int iLastWave;

            //
            // we may need to yield for audio to get converted.
            //
            if (lpcs->fAudioYield)
                Yield();

            //
            // if all buffers are done, we have broke audio.
            //
            iLastWave = lpcs->iNextWave == 0 ? 
                        lpcs->iNumAudio -1 : lpcs->iNextWave-1;

            if (!fStopping && 
                    lpcs->alpWaveHdr[iLastWave]->dwFlags & WHDR_DONE)
                lpcs->fAudioBreak = TRUE;

            w = lpcs->iNumAudio; // don't get stuck here forever...
            while (w && fOK && (lpWaveHdr-> dwFlags & WHDR_DONE)) {
                w--;
                if (lpWaveHdr-> dwBytesRecorded) {
                    /* Chunk info is included in the wave data */
                    /* Reset Chunk Size in buffer */
                    ((LPRIFF)(lpWaveHdr->lpData))[-1].dwSize = 
                                lpWaveHdr-> dwBytesRecorded;
                    if (lpcs-> CallbackOnWaveStream) {
                        (*(lpcs->CallbackOnWaveStream)) (lpcs->hwnd, lpWaveHdr);
                    }
                    if (lpcs-> fCapturingToDisk) {
                        if(!AVIWrite (lpcs, lpWaveHdr-> lpData - sizeof(RIFF),
                                (lpWaveHdr-> dwBytesRecorded +
                                sizeof (RIFF) + 1) & ~1L)) {
                            fOK = FALSE;
                            fStopping = TRUE;
                            errorUpdateError (lpcs, IDS_CAP_FILE_WRITE_ERROR);
                         } else {
                            if (IndexAudio (lpcs, lpWaveHdr-> dwBytesRecorded))
                                lpcs->dwWaveBytes += lpWaveHdr-> dwBytesRecorded;
                            else
                                fStopping = TRUE;                                
                         }
                    } // endif capturing to disk
                    // Warning: Kludge to create wave chunk count when net capture
                    // follows.
                    else {
                        lpcs->dwWaveChunkCount++;
                        lpcs->dwWaveBytes += lpWaveHdr-> dwBytesRecorded;
                    }
                } // endif dwBytesRecorded
                
                lpWaveHdr-> dwBytesRecorded = 0;
                lpWaveHdr-> dwFlags &= ~WHDR_DONE;

                 /* return the emptied buffer to the que */
                if(waveInAddBuffer(lpcs->hWaveIn, lpWaveHdr, sizeof(WAVEHDR))) {
                    fOK = FALSE;
                    fStopping = TRUE;
                    errorUpdateError(lpcs, IDS_CAP_WAVE_ADD_ERROR);
                }

                /* increment the next wave buffer pointer */
                if(++lpcs->iNextWave >= lpcs->iNumAudio)
                    lpcs->iNextWave = 0;

                lpWaveHdr = lpcs->alpWaveHdr[lpcs->iNextWave];

            } // endwhile buffer available
        } // endif sound enabled
    } // end of forever

CompressFrameFailure:

    iaverageFini (lpcs->lpia);

    // Switch back to the normal format
    if (lpcs->sCapParms.fStepCaptureAt2x) {
        SendDriverFormat (lpcs, (LPBITMAPINFOHEADER) lpcs->lpBitsInfo, 
                sizeof (BITMAPINFOHEADER));
        GlobalFreePtr (lpcs->VidHdr2x.lpData);
        lpcs->VidHdr2x.lpData = NULL;
    }
    
    // And free the 2x memory
    if (lpcs->lpbmih2x) {
        GlobalFreePtr (lpcs->lpbmih2x);
        lpcs->lpbmih2x = NULL;
    }

    // -------------------------------------------------------
    //   END OF MAIN CAPTURE LOOP
    // -------------------------------------------------------
    
    lpcs-> dwTimeElapsedMS = lpcs-> dwMCIActualEndMS - lpcs->dwMCIActualStartMS;

    /* eat any keys that have been pressed */
    while(GetKey(FALSE))
        ;

    AVIFini(lpcs);  // does the Reset, and frees all buffers
    AVIFileFini(lpcs, FALSE /* fWroteJunkChunks */, FALSE /* fAbort */);
    

    /* Notify if there was an error while recording */

    if(!fOK) {
        errorUpdateError (lpcs, IDS_CAP_RECORDING_ERROR);
    }
    
    if (lpcs-> fCapturingToDisk) {
        if (lpcs->dwVideoChunkCount)
            dw = muldiv32(lpcs->dwVideoChunkCount,1000000,lpcs-> dwTimeElapsedMS);
        else 
            dw = 0;     // The muldiv32 doesn't give 0 if numerator is zero

        if(lpcs->sCapParms.fCaptureAudio) {
            // "Captured %d.%03d sec.  %ld frames (%d dropped) (%d.%03d fps).  %ld audio bytes (%d.%03d sps)"
            statusUpdateStatus(lpcs, IDS_CAP_STAT_VIDEOAUDIO,
                  (WORD)(lpcs-> dwTimeElapsedMS/1000),
                  (WORD)(lpcs-> dwTimeElapsedMS%1000),
                  lpcs->dwVideoChunkCount,
                  lpcs->dwFramesDropped,
                  (WORD)(dw / 1000),
                  (WORD)(dw % 1000),
                  lpcs->dwWaveBytes,
                  (WORD) lpcs->lpWaveFormat->nSamplesPerSec / 1000,
                  (WORD) lpcs->lpWaveFormat->nSamplesPerSec % 1000);
        } else {
            // "Captured %d.%03d sec.  %ld frames (%d dropped) (%d.%03d fps)."
            statusUpdateStatus(lpcs, IDS_CAP_STAT_VIDEOONLY,
                  (WORD)(lpcs-> dwTimeElapsedMS/1000),
                  (WORD)(lpcs-> dwTimeElapsedMS%1000),
                  lpcs->dwVideoChunkCount,
                  lpcs->dwFramesDropped,
                  (WORD)(dw / 1000),
                  (WORD)(dw % 1000));
        }
    } // endif capturing to disk (no warnings or errors if to net)

    // No frames captured, warn user that interrupts are probably not enabled.
    if (fOK && (lpcs->dwVideoChunkCount == 0)) {
        errorUpdateError (lpcs, IDS_CAP_NO_FRAME_CAP_ERROR);
    }
    // No audio captured, (but enabled), warn user audio card is hosed
    else if (fOK && lpcs->sCapParms.fCaptureAudio && (lpcs->dwWaveBytes == 0)) {
        errorUpdateError (lpcs, IDS_CAP_NO_AUDIO_CAP_ERROR);
    }

    // Audio underrun, inform user
    else if (fOK && lpcs->sCapParms.fCaptureAudio && lpcs->fAudioBreak) {
        errorUpdateError (lpcs, IDS_CAP_AUDIO_DROP_ERROR);
    }

    // If frames dropped, or changed capture rate, warn the user
    else if (fOK && lpcs->dwVideoChunkCount && lpcs->fCapturingToDisk) {

        // Warn user if dropped > 10% (default) of the frames
        if ((DWORD)100 * lpcs->dwFramesDropped / lpcs->dwVideoChunkCount >
                    lpcs-> sCapParms.wPercentDropForError) {

            // "%d of %ld frames (%d.%03d\%) dropped during capture."
            errorUpdateError (lpcs, IDS_CAP_STAT_FRAMESDROPPED,
                  lpcs->dwFramesDropped,
                  lpcs->dwVideoChunkCount,
                  (WORD)(muldiv32(lpcs->dwFramesDropped,10000,lpcs->dwVideoChunkCount)/100),
                  (WORD)(muldiv32(lpcs->dwFramesDropped,10000,lpcs->dwVideoChunkCount)%100)
                  );
        }
    }

EarlyExit:

    //
    // If we were compressing while capturing, close it down
    //
    if (lpcs->CompVars.hic) {
        // Kludge, reset the lpBitsOut pointer
        if (lpcs->CompVars.lpBitsOut)
            ((LPBYTE) lpcs->CompVars.lpBitsOut) -= 8;
	ICSeqCompressFrameEnd(&lpcs->CompVars);
    }

    if (fTryToPaint) {
        if (hpalT)
             SelectPalette(hdc, hpalT, FALSE);
        ReleaseDC (lpcs->hwnd, hdc);
    }

    if (lpcs->sCapParms.fMCIControl) 
        MCIDeviceClose (lpcs);

    // Let the user see where capture stopped
    if ((!lpcs->fLiveWindow) && (!lpcs->fOverlayWindow)) 
        videoFrame( lpcs->hVideoIn, &lpcs->VidHdr );  
    InvalidateRect( lpcs->hwnd, NULL, TRUE);

    lpcs->fCapFileExists = (lpcs-> dwReturn == DV_ERR_OK);
    lpcs->fCapturingNow      = FALSE;
    lpcs-> fStepCapturingNow = FALSE;
    
    statusUpdateStatus(lpcs, IDS_CAP_END);  // Always the last message

    return;
}



       


