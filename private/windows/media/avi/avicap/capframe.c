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

#define INC_OLE2
#pragma warning(disable:4103)
#include <windows.h>
#include <windowsx.h>
#include <win32.h>
#include <mmsystem.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <msvideo.h>
#include <drawdib.h>
#include <mmddk.h>
#include <avifmt.h>

#include "ivideo32.h"
#include "avicap.h"
#include "avicapi.h"

#include "mmdebug.h"

#ifdef _DEBUG
    #define DSTATUS(lpcs, sz) statusUpdateStatus(lpcs, IDS_CAP_INFO, (LPTSTR) TEXT(sz))
#else
    #define DSTATUS(lpcs, sz)
#endif


/*
 *  SingleFrameCaptureOpen
 *
 */
BOOL FAR PASCAL SingleFrameCaptureOpen (LPCAPSTREAM lpcs)
{
    UINT err;
    if ((lpcs->fCaptureFlags & CAP_fCapturingNow) || (lpcs->fCaptureFlags & CAP_fFrameCapturingNow)) {
        err = IDS_CAP_FILE_OPEN_ERROR;
        goto EarlyExit;
    }

#ifdef NEW_COMPMAN
    /* Warm up the compressor function */
    if (lpcs->CompVars.hic) {
        if (ICSeqCompressFrameStart(&lpcs->CompVars, lpcs->lpBitsInfo) == FALSE) {
            err = IDS_CAP_COMPRESSOR_ERROR;
            goto EarlyExit;
	}
        // Kludge, offset the lpBitsOut ptr
        // Compman allocates the compress buffer too large by
        // 2048 + 16 so we will still have room
        ((LPBYTE) lpcs->CompVars.lpBitsOut) += 8;
    }
#endif

    if (!CapFileInit(lpcs)) {
        err = IDS_CAP_FILE_OPEN_ERROR;
        goto EarlyExit;
    }

    lpcs->fCaptureFlags |= (CAP_fCapturingNow | CAP_fFrameCapturingNow);
    lpcs->dwReturn = DV_ERR_OK;

    statusUpdateStatus(lpcs, IDS_CAP_BEGIN);  // Always the first message

    return TRUE;

EarlyExit:
    errorUpdateError(lpcs, (UINT) err);
    return FALSE;
}


/*
 *  SingleFrameCaptureClose
 *
 *
 */
BOOL FAR PASCAL SingleFrameCaptureClose (LPCAPSTREAM lpcs)
{

    if ((!(lpcs->fCaptureFlags & CAP_fCapturingNow)) && (!(lpcs->fCaptureFlags & CAP_fFrameCapturingNow))) {
        errorUpdateError(lpcs, IDS_CAP_FILE_OPEN_ERROR);
        return FALSE;
    }

    AVIFileFini(lpcs, TRUE /* fWroteJunkChunks */, FALSE /* fAbort */);

#ifdef NEW_COMPMAN
    if (lpcs->CompVars.hic) {
        // Kludge, offset the lpBitsOut ptr
        if (lpcs->CompVars.lpBitsOut)
        ((LPBYTE) lpcs->CompVars.lpBitsOut) -= 8;
	ICSeqCompressFrameEnd(&lpcs->CompVars);
    }
#endif

    lpcs->fCapFileExists = (lpcs->dwReturn == DV_ERR_OK);
    lpcs->fCaptureFlags &= ~(CAP_fCapturingNow | CAP_fFrameCapturingNow);

    statusUpdateStatus(lpcs, IDS_CAP_END);  // Always the last message

    return TRUE;
}

/*
 *  SingleFrameCapture
 *
 *  Append to the open single frame capture file.
 */
BOOL FAR PASCAL SingleFrameCapture (LPCAPSTREAM lpcs)
{
    LPVIDEOHDR lpVidHdr = &lpcs->VidHdr;
    BOOL       fOK = FALSE;
    DWORD      dwBytesUsed;
    BOOL       fKeyFrame;
    LPSTR      lpData;

    if ((!(lpcs->fCaptureFlags & CAP_fCapturingNow)) ||
        (!((lpcs->fCaptureFlags & CAP_fStepCapturingNow) || (lpcs->fCaptureFlags & CAP_fFrameCapturingNow)))
        ) {
        errorUpdateError(lpcs, IDS_CAP_FILE_OPEN_ERROR);
        return FALSE;
    }

    videoFrame (lpcs->hVideoIn, &lpcs->VidHdr);
    InvalidateRect (lpcs->hwnd, NULL, TRUE);

    if (lpVidHdr->dwBytesUsed) {
        UINT wError;
        BOOL bPending = FALSE;

        if (lpcs->CallbackOnVideoFrame)
            lpcs->CallbackOnVideoFrame (lpcs->hwnd, lpVidHdr);

        // Prepend a RIFF chunk
        ((LPRIFF)lpVidHdr->lpData)[-1].dwType = MAKEAVICKID(cktypeDIBbits, 0);
        ((LPRIFF)lpVidHdr->lpData)[-1].dwSize = lpcs->VidHdr.dwBytesUsed;

       #ifdef NEW_COMPMAN
        //
        // We are automatically compressing during capture, so
        // compress the frame before we pass it on to be written
        //
        if (lpcs->CompVars.hic)
        {
            LPRIFF priff;

            dwBytesUsed = 0;
            lpData = ICSeqCompressFrame(&lpcs->CompVars, 0,
                                        lpVidHdr->lpData,
                                        &fKeyFrame,
                                        &dwBytesUsed);

            priff = ((LPRIFF)lpData) -1;
            priff->dwType = MAKEAVICKID(cktypeDIBbits, 0);
            priff->dwSize = dwBytesUsed;
        }
        else {
            lpData = lpVidHdr->lpData;
            dwBytesUsed = lpVidHdr->dwBytesUsed;
            fKeyFrame = lpVidHdr->dwFlags & VHDR_KEYFRAME;
        }
       #endif // NEW_COMPMAN

        // AVIWriteVideoFrame can compress while writing,
        // in this case, the dwBytesUsed and KeyFrame settings
        // may be modified, so pick these up after the write is finished

        AVIWriteVideoFrame (lpcs,
                        lpData,
                        dwBytesUsed,
                        fKeyFrame,
                        (UINT)-1, 0, &wError, &bPending);
        if (wError) {
            errorUpdateError(lpcs, wError);
        }
        else {
            fOK = TRUE;
            statusUpdateStatus (lpcs, IDS_CAP_STAT_CAP_L_FRAMES,
                                lpcs->dwVideoChunkCount);
        }
    } // if the frame is done
    else
        errorUpdateError (lpcs, IDS_CAP_RECORDING_ERROR2);

    return fOK;
}
