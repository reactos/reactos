/****************************************************************************
 *
 *   capframe.c
 * 
 *   Single frame capture
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
#include <avifmt.h>

#include "avicap.h"
#include "avicapi.h"        

#ifdef _DEBUG
    #define DSTATUS(lpcs, sz) statusUpdateStatus(lpcs, IDS_CAP_INFO, (LPSTR) sz)
#else
    #define DSTATUS(lpcs, sz) 
#endif


/*
 *  SingleFrameCaptureOpen
 *      
 */
BOOL FAR PASCAL SingleFrameCaptureOpen (LPCAPSTREAM lpcs)
{
    if (lpcs-> fCapturingNow || lpcs-> fFrameCapturingNow) {
        lpcs-> dwReturn = IDS_CAP_FILE_OPEN_ERROR;
        goto EarlyExit;
    }
    
    /* Warm up the compressor function */
    if (lpcs->CompVars.hic) {
        if (ICSeqCompressFrameStart(&lpcs->CompVars, lpcs->lpBitsInfo) == FALSE) {
            lpcs-> dwReturn = IDS_CAP_COMPRESSOR_ERROR;
            goto EarlyExit;
	}
        // Kludge, offset the lpBitsOut ptr 
        // Compman allocates the compress buffer too large by 
        // 2048 + 16 so we will still have room 
        ((LPBYTE) lpcs->CompVars.lpBitsOut) += 8;
    }

    if (!AVIFileInit(lpcs)) {
        lpcs-> dwReturn = IDS_CAP_FILE_OPEN_ERROR;
        goto EarlyExit;
    }

    lpcs-> fCapturingNow = TRUE;
    lpcs-> fFrameCapturingNow = TRUE;
    lpcs-> dwReturn = DV_ERR_OK;

    statusUpdateStatus(lpcs, IDS_CAP_BEGIN);  // Always the first message

    return TRUE;
            
EarlyExit:
    errorUpdateError(lpcs, (WORD) lpcs->dwReturn);
    return FALSE;
}


/*
 *  SingleFrameCaptureClose
 *      
 *      
 */
BOOL FAR PASCAL SingleFrameCaptureClose (LPCAPSTREAM lpcs)
{

    if ((!lpcs-> fCapturingNow) && (!lpcs-> fFrameCapturingNow)) {
        lpcs-> dwReturn = IDS_CAP_FILE_OPEN_ERROR;
        errorUpdateError(lpcs, (WORD) lpcs->dwReturn);
        return FALSE;
    }
    
    AVIFileFini(lpcs, FALSE /* fWroteJunkChunks */, FALSE /* fAbort */);
    
    if (lpcs->CompVars.hic) {
        // Kludge, offset the lpBitsOut ptr 
        if (lpcs->CompVars.lpBitsOut)
            ((LPBYTE) lpcs->CompVars.lpBitsOut) -= 8;
	ICSeqCompressFrameEnd(&lpcs->CompVars);
    }    

    lpcs->fCapFileExists = (lpcs-> dwReturn == DV_ERR_OK);
    lpcs->fCapturingNow = FALSE;
    lpcs->fFrameCapturingNow = FALSE;
    
    statusUpdateStatus(lpcs, IDS_CAP_END);  // Always the last message

    return TRUE;
}


// Writes compressed or uncompressed frames to the AVI file
// returns TRUE if no error, FALSE if end of file,
// and sets pfKey and plSize on exit.

BOOL SingleFrameWrite (
    LPCAPSTREAM             lpcs,       // capture stream
    LPVIDEOHDR              lpVidHdr,   // input header
    BOOL FAR 		    *pfKey,	// did it end up being a key frame?
    LONG FAR		    *plSize)	// size of returned image
{
    MMCKINFO    ck;
    BOOL        fOK = TRUE;
    DWORD	dwBytesUsed;
    BOOL	fKeyFrame;
    LPSTR	lpBits;

    if ((!lpcs-> fCapturingNow) ||
                (!(lpcs-> fStepCapturingNow || lpcs-> fFrameCapturingNow)) ||
                (!lpcs->hmmio)) {
        lpcs-> dwReturn = IDS_CAP_FILE_OPEN_ERROR;
        return FALSE;
    }
    
    /* Now compress the DIB to the format they chose */
    if (lpcs->CompVars.hic) {
	dwBytesUsed = 0;	// don't force a data rate
        lpBits = ICSeqCompressFrame(&lpcs->CompVars, 0,
    	        lpcs->lpBits, &fKeyFrame, &dwBytesUsed);

    /* They don't want it compressed */
    } else {
        // Use current values for writing the DIB to disk
        dwBytesUsed = lpcs->VidHdr.dwBytesUsed;
        fKeyFrame = (BOOL)(lpcs->VidHdr.dwFlags & VHDR_KEYFRAME);
        lpBits = lpcs->lpBits;
    }

    /* Create DIB Bits chunk */
    ck.cksize = dwBytesUsed;
    ck.ckid = MAKEAVICKID(cktypeDIBbits,0);
    ck.fccType = 0;
    if (mmioCreateChunk(lpcs->hmmio,&ck,0)) {
        fOK = FALSE;
    }

    /* Write DIB  data */
    if (fOK && mmioWrite(lpcs->hmmio, lpBits, dwBytesUsed) != 
                (LONG) dwBytesUsed) {
        fOK = FALSE;
    }
    
    if (fOK && mmioAscend(lpcs->hmmio, &ck, 0)) {
        fOK = FALSE;
    }

    *pfKey = fKeyFrame;
    *plSize = dwBytesUsed;

    return fOK;
}


/*
 *  SingleFrameCapture
 *      
 *  Append to the open single frame capture file.
 */
BOOL FAR PASCAL SingleFrameCapture (LPCAPSTREAM lpcs)
{
    LPVIDEOHDR lpVidHdr = &lpcs->VidHdr;
    BOOL fOK = FALSE;
    BOOL fKey;
    LONG lSize;

    if ((!lpcs-> fCapturingNow) ||
                (!(lpcs-> fStepCapturingNow || lpcs-> fFrameCapturingNow)) ||
                (!lpcs->hmmio)) {
        lpcs-> dwReturn = IDS_CAP_FILE_OPEN_ERROR;
        errorUpdateError(lpcs, (WORD) lpcs->dwReturn);
        return FALSE;
    }

    videoFrame( lpcs->hVideoIn, &lpcs->VidHdr );
    InvalidateRect( lpcs->hwnd, NULL, TRUE);

    if (lpVidHdr-> dwBytesUsed) {
        if (lpcs->CallbackOnVideoFrame)
            (*(lpcs->CallbackOnVideoFrame)) (lpcs->hwnd, lpVidHdr);

        if (!SingleFrameWrite (lpcs, lpVidHdr, &fKey, &lSize)) {
            // "ERROR: Could not write to file."
            errorUpdateError(lpcs, IDS_CAP_FILE_WRITE_ERROR);
        } 
        else {
            fOK = IndexVideo(lpcs, lSize, fKey);
            statusUpdateStatus (lpcs, IDS_CAP_STAT_CAP_L_FRAMES,
                            lpcs-> dwVideoChunkCount);
        }
    } // if the frame is done
    else
        errorUpdateError (lpcs, IDS_CAP_RECORDING_ERROR2);

    return fOK;
}



