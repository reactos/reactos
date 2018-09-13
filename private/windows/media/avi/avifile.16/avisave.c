/****************************************************************************
 *
 *  AVISAVE.C
 *
 *  routine for writing Standard AVI files
 *
 *      AVISave()
 *
 *  Copyright (c) 1992 Microsoft Corporation.  All Rights Reserved.
 *
 *  You have a royalty-free right to use, modify, reproduce and
 *  distribute the Sample Files (and/or any modified version) in
 *  any way you find useful, provided that you agree that
 *  Microsoft has no warranty obligations or liability for any
 *  Sample Application Files which are modified.
 *
 ***************************************************************************/

#include <win32.h>
#include <compobj.h>
#include <valid.h>
#include <compman.h>
#include "avifmt.h"
#include "avifile.h"
#include "avicmprs.h"
#include "debug.h"
//extern LONG FAR PASCAL muldiv32(LONG,LONG,LONG);

/************************************************************************/
/* Auto-doc for the AVICOMPRESSOPTIONS structure.  Make sure it matches	*/
/* the declarations in avifile.h !!!                                    */
/************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICOMPRESSOPTIONS
 * 
 * @types AVICOMPRESSOPTIONS | This structure contains information
 *	       about a stream and how it is to be compressed and saved. 
 *        This structure passes data to <f AVIMakeCompressedStream> 
 *        (or <f AVISave> which uses <f AVIMakeCompressedStream>).
 * 
 * @field DWORD | fccType | Specifies a four-character code 
 *        indicating the stream type. The following 
 *        constants have been defined for the data commonly 
 *        found in AVI streams:
 *
 * @flag  streamtypeAUDIO | Indicates an audio stream.
 * @flag  streamtypeMIDI | Indicates a MIDI stream.
 * @flag  streamtypeTEXT | Indicates a text stream.
 * @flag  streamtypeVIDEO | Indicates a video stream.
 * 
 * @field DWORD | fccHandler | For a video stream, specifies the 
 *        four-character code for the compressor handler that 
 *        will compress this stream when it is saved 
 *        (For example, mmioFOURCC('M','S','V','C')). 
*         This member is not used for audio streams.
 * 
 * @field DWORD | dwKeyFrameEvery | Specifies the maximum period 
 *        between key frames. This member is used only 
 *        if the AVICOMPRESSF_KEYFRAMES flag is set, otherwise 
 *        every frame is a key frame. 
 * 
 * @field DWORD | dwQuality | Specifies the quality value passed 
 *        to a video compressor. This member is not used for 
 *        an audio compressor.
 * 
 * @field DWORD | dwBytesPerSecond | Specifies the data rate a video
 *	       compressor should use.  This member is used only 
 *        if the AVICOMPRESSF_DATARATE flag is set.
 * 
 * @field DWORD | dwFlags | Specifies the flags used for compression:
 * 
 *   @flag AVICOMPRESSF_INTERLEAVE | Indicates this stream is to be interleaved
 *         every <e AVICOMPRESSOPTIONS.dwInterleaveEvery> frames 
 *         with respect to the first stream.
 * 
 *   @flag AVICOMPRESSF_KEYFRAMES | Indicates this video stream 
 *         is to be saved with key frames at least 
 *         every <e AVICOMPRESSOPTIONS.dwKeyFrameEvery> frames.
 *	   By default, every frame will be a key frame.
 * 
 *   @flag AVICOMPRESSF_DATARATE | Indicates this video stream 
 *         is to be compressed with the data rate 
 *         specified in <e AVICOMPRESSOPTIONS.dwBytesPerSecond>.
 * 
 *   @flag AVICOMPRESSF_VALID | Indicates this structure contains 
 *         valid data. If this flag is set, AVIFile uses the structure 
 *         data to set the default compression values for <f AVISaveOptions>.  
 *         If an empty structure is passed and this flag is not set, 
 *         some defaults will be chosen.
 * 
 * @field LPVOID | lpFormat | Specifies a pointer to a structure 
 *        defining the data format. For an audio stream, 
 *        this is an <t LPWAVEFORMAT> structure.
 * 
 * @field DWORD | cbFormat | Specifies the size of the data referenced by 
 *	       <e AVICOMPRESSOPTIONS.lpFormat>
 * 
 * @field LPVOID | lpParms | Used internally to store compressor 
 *        specific data.
 * 
 * @field DWORD | cbParms | Specifies the size of the data referenced by 
 *        <e AVICOMPRESSOPTIONS.lpParms>
 * 
 * @field DWORD | dwInterleaveEvery | Specifies how often 
 *        to interleave stream data with the data 
 *        from the first stream.  Used only if the
 *	       AVICOMPRESSF_INTERLEAVE flag is set.
 * 
 ***************************************************************************/

/*******************************************************************
* @doc EXTERNAL AVISave
*
* @api LONG | AVISave | This function is used to save an AVI file.
*
* @parm LPCSTR | szFile | Specifies a zero-terminated string 
*       containing the name of the file to save.
*
* @parm CLSID FAR * | pclsidHandler | Specifies a pointer to the
*       file handler used to write the file. The file will
*       be created by calling <f AVIFileOpen> using this handler. If
*       a handler is not specified, a default one is selected based 
*       upon the file extension.
*
* @parm AVISAVECALLBACK | lpfnCallback | Specifies a far pointer to 
 *      a callback function for the save operation.
*
* @parm int | nStreams | Specifies the number of streams saved in the 
*       the file. 
*
* @parm PAVISTREAM | pavi | Specifies a pointer an AVI stream. 
*       This parameter is paired with <p lpOptions>. The parameter 
*       pair can be repeated as a variable number of arguments.
*
* @parm LPAVICOMPRESSOPTIONS | lpOptions | Specifies a pointer to an 
*       <t AVICOMPRESSOPTIONS> structure containing the compression 
*       options for the stream referenced by <p pavi>.
*       This parameter is paired with <p pavi>. The parameter 
*       pair can be repeated as a variable number of arguments.
*
* @parm .| . . | Additional streams can be appened 
*       by including more <p pavi> and <p lpOptions> parameter pairs.
*
* @rdesc Returns AVIERR_OK if successful; otherwise it returns an error code.
*
* @comm This function saves an AVI sequence to the file
*       specified by <p szFile>. The <p pavi> and <p lpOptions> parameters 
*       define the streams saved. If saving more than one stream, 
*       repeat the <p pavi> and <p lpOptions> parameter pair for 
*       each additional stream.
*
*      A callback function can be supplied in <p lpfnCallback> to 
*      display status information and let the user cancel the 
*      save operation.  The callback uses the following format:
*
*      LONG FAR PASCAL SaveCallback(int nPercent)
*	
*	    The <p nPercent> parameter specifies the percentage of the 
*      file saved.
*
*	    The callback function should return AVIERR_OK if the
*      operation should continue and AVIERR_USERABORT if the 
*      user wishes to abort the save operation.
*    
*
* @xref <f AVISaveV> <f AVISaveOptions>
*
*******************************************************************/
EXTERN_C HRESULT CDECL AVISave(LPCSTR               szFile,
		    CLSID FAR *pclsidHandler,   
                    AVISAVECALLBACK     lpfnCallback,
		    int			nStreams,
                    PAVISTREAM          pavi,
                    LPAVICOMPRESSOPTIONS lpOptions,
		    ...
		    )
{
    PAVISTREAM FAR 		*apavi;
    LPAVICOMPRESSOPTIONS FAR	*alpOptions;
    int	i;
    HRESULT	hr;

    //
    // We were passed arguments of the form PAVI, OPTIONS, PAVI, OPTIONS, etc.
    // for AVISaveV, we need to separate these into an array of PAVI's and
    // an array of LPAVICOMPRESSOPTIONS.
    //

    apavi = (PAVISTREAM FAR *)GlobalAllocPtr(GMEM_MOVEABLE,
			nStreams * sizeof(PAVISTREAM));
    alpOptions = (LPAVICOMPRESSOPTIONS FAR *)GlobalAllocPtr(GMEM_MOVEABLE,
			nStreams * sizeof(LPAVICOMPRESSOPTIONS));
    if (!apavi || !alpOptions)
	return ResultFromScode(AVIERR_MEMORY);

    for (i = 0; i < nStreams; i++) {
	apavi[i] = *(PAVISTREAM FAR *)((LPBYTE)&pavi +
		(sizeof(PAVISTREAM) + sizeof(LPAVICOMPRESSOPTIONS)) * i);
	alpOptions[i] = *(LPAVICOMPRESSOPTIONS FAR *)((LPBYTE)&lpOptions +
		(sizeof(PAVISTREAM) + sizeof(LPAVICOMPRESSOPTIONS)) * i);
    }

    hr = AVISaveV(szFile, pclsidHandler, lpfnCallback, nStreams, apavi,
			alpOptions);

    GlobalFreePtr(apavi);
    GlobalFreePtr(alpOptions);
    return hr;
}

BOOL FAR PASCAL DummySaveCallback(int iProgress)
{
    return FALSE;   // do nothing, allow save to continue
}

/**********************************************************************
* @doc EXTERNAL AVISaveV
*
* @api LONG | AVISaveV | This function is used to save an AVI file.
*
* @parm LPCSTR | szFile | Specifies a zero-terminated string 
*       containing the name of the file to save.
*
* @parm CLSID FAR * | pclsidHandler | Specifies a pointer to the
*       file handler used to write the file. The file will
*       be created by calling <f AVIFileOpen> using this handler. If
*       a handler is not specified, a default one is selected based upon 
*       the file extension.
*
* @parm AVISAVECALLBACK | lpfnCallback | Specifies a pointer to a callback
*       function used to display status information and let the use 
*       cancel the save operation.
*
* @parm int | nStreams | Specifies the number of streams to save.
*
* @parm PAVISTREAM FAR * | ppavi | Specifies a pointer to an 
*       array of <t PAVISTREAM> pointers. The array uses one pointer 
*       for each stream.
*
* @parm LPAVICOMPRESSOPTIONS FAR * | plpOptions | Specifies a pointer 
*       to an array of <t LPAVICOMPRESSOPTIONS> pointers. The 
*       uses one pointer for each stream.
*
* @rdesc Returns AVIERR_OK on success, an error code otherwise.
*
* @comm This function is equivalent to <f AVISave> except 
*       the streams are passed in an array instead of as a
*       variable number of arguments. (<f AVISaveV> is to <f AVISave> 
*       as <f wvsprintf> is to <f wsprintf>.)
*
* @xref <f AVISave> <f AVISaveOptions>
*
********************************************************************/
STDAPI AVISaveV(LPCSTR               szFile,
		CLSID FAR *pclsidHandler,
                    AVISAVECALLBACK     lpfnCallback,
		    int			nStreams,
		    PAVISTREAM FAR *	ppavi,
		    LPAVICOMPRESSOPTIONS FAR * plpOptions)
{
    int		    stream;
    MainAVIHeader   hdrNew;
    PAVIFILE	    pfilesave = 0;
    HRESULT	    hr;
    AVISTREAMINFO   strhdr;
    AVIFILEINFO	    finfo;
    LONG	    cbFormat;
    DWORD	    dwSamplesRead;
    LPVOID	    lpBuffer = 0;
    DWORD	    dwBufferSize;
    LONG	    l;
    DWORD	    dwSize;
    DWORD	    dwFlags;
    WORD	    cktype;
    LPBITMAPINFOHEADER lpbi;
    DWORD	    dwInterleaveEvery = 0;

#define MAXSTREAMS  64
    
    int		    iVideoStream = -1;
    PAVISTREAM	    apavi[MAXSTREAMS];
    PAVISTREAM	    apaviNew[MAXSTREAMS];
    LONG	    lDone[MAXSTREAMS];
    LONG	    lInterval;
    

    if (nStreams > MAXSTREAMS)
	return ResultFromScode(AVIERR_INTERNAL);
    for (stream = 0; stream < nStreams; stream++) {
	apavi[stream] = NULL;
	apaviNew[stream] = NULL;
    }

    //
    // Open file and write out the main header
    //
    DPF("Creating new file\n");
    
    hr = AVIFileOpen(&pfilesave, szFile, OF_CREATE | OF_WRITE | OF_SHARE_EXCLUSIVE, pclsidHandler);
    if (hr != 0)
	goto Error;

    AVIFileInfo(pfilesave, &finfo, sizeof(finfo));

    DPF("Creating compressed streams\n");
    
    for (stream = 0; stream < nStreams; stream++) {
	if (!IsValidInterface(ppavi[stream])) {
	    hr = ResultFromScode(AVIERR_INTERNAL);
	    goto Error;
	}

	hr = AVIStreamInfo(ppavi[stream], &strhdr, sizeof(strhdr));

	if (hr != AVIERR_OK) {
	    DPF("Error from AVIStreamInfo!\n");
	    goto Error;
	}

	// Find the video stream....
	if (strhdr.fccType == streamtypeVIDEO) {
	    if (iVideoStream < 0) {
		iVideoStream = stream;
	    }
	} else if (strhdr.fccType == streamtypeAUDIO) {
	    if (dwInterleaveEvery == 0) {
		// Should the interleave factor be in the options at all?
		if (plpOptions && plpOptions[stream] &&
			plpOptions[stream]->dwFlags & AVICOMPRESSF_INTERLEAVE)
		    dwInterleaveEvery = plpOptions[stream]->dwInterleaveEvery;
	    }
	}

	apavi[stream] = NULL;
	
	if (plpOptions && plpOptions[stream] &&
		    (plpOptions[stream]->fccHandler ||
		     plpOptions[stream]->lpFormat)) {
	    DWORD   dwKeyFrameEvery = plpOptions[stream]->dwKeyFrameEvery;

	    if (finfo.dwCaps & AVIFILECAPS_ALLKEYFRAMES)
		plpOptions[stream]->dwKeyFrameEvery = 1;
	    
	    // If they've given compression options for this stream,
	    // use them....
	    hr = AVIMakeCompressedStream(&apavi[stream],
					 ppavi[stream],
					 plpOptions[stream],
					 NULL);

	    plpOptions[stream]->dwKeyFrameEvery = dwKeyFrameEvery;
	    
	    if (hr != 0) {
		DPF("AVISave: Failed to create compressed stream!\n");
		apavi[stream] = NULL;
		goto Error;	// !!!
	    } else {
		hr = AVIStreamInfo(apavi[stream], &strhdr, sizeof(strhdr));
		if (hr != 0) {
		    DPF("AVISave: Failed to create compressed stream!\n");
		    AVIStreamClose(apavi[stream]);
		    apavi[stream] = NULL;
		    goto Error;	// !!!
		}
	    }
	}

	if (apavi[stream] == NULL) {
	    // otherwise just copy the stream over....
	    apavi[stream] = ppavi[stream];
	    AVIStreamAddRef(apavi[stream]);
	}

	lDone[stream] = AVIStreamStart(apavi[stream]);
    }

    // Put the video stream first, so interleaving will work.
    // !!!
    if (iVideoStream > 0) {
	PAVISTREAM p;

	p = apavi[iVideoStream];
	apavi[iVideoStream] = apavi[0];
	apavi[0] = p;
	iVideoStream = 0;
    }
    
    if (lpfnCallback == NULL)
	lpfnCallback = &DummySaveCallback;

    /* pick a good buffer size and go for it.... */
    dwBufferSize = 32768L;

    lpBuffer = GlobalAllocPtr(GMEM_MOVEABLE, dwBufferSize);
    if (!lpBuffer) {
	hr = ResultFromScode(AVIERR_MEMORY);
	goto Error;
    }

    //
    // Construct AVI file header
    //
    AVIStreamInfo(apavi[0], &strhdr, sizeof(strhdr));
    hdrNew.dwMicroSecPerFrame = muldiv32(1000000L, strhdr.dwScale, strhdr.dwRate);
    hdrNew.dwMaxBytesPerSec = 0;      
    hdrNew.dwPaddingGranularity = 0;  
                       
    hdrNew.dwFlags = AVIF_HASINDEX;	       
    hdrNew.dwFlags &= ~(AVIF_ISINTERLEAVED | AVIF_WASCAPTUREFILE |
					AVIF_MUSTUSEINDEX);
    
    hdrNew.dwTotalFrames = strhdr.dwLength;       // !!!
    hdrNew.dwInitialFrames = 0;			  // !!!
    
    hdrNew.dwStreams = nStreams;	       
    hdrNew.dwSuggestedBufferSize = 32768; 
		       
    if (iVideoStream >= 0) {
	cbFormat = dwBufferSize;
	hr = AVIStreamReadFormat(apavi[iVideoStream],
				 AVIStreamStart(apavi[iVideoStream]),
				 lpBuffer,
				 &cbFormat);

	if (cbFormat < sizeof(BITMAPINFOHEADER)) {
	    hr = ResultFromScode(AVIERR_INTERNAL);
	}

	if (hr != 0) {
	    DPF("AVISave: Error from initial ReadFormat!\n");
	    goto Error;
	}
	
	lpbi = (LPBITMAPINFOHEADER) lpBuffer;

	hdrNew.dwWidth = lpbi->biWidth;
	hdrNew.dwHeight = lpbi->biHeight;
	lInterval = 1;
    } else {
	hdrNew.dwWidth = 0;
	hdrNew.dwHeight = 0;
	lInterval = AVIStreamTimeToSample(apavi[0], 500);
    }
		           
    //
    // Loop through streams and write out stream header
    //
    for (stream = 0; stream < nStreams; stream++) {
	// DPF2("Making stream %d header LIST\n", stream);

        AVIStreamInfo(apavi[stream], &strhdr, sizeof(strhdr));
	strhdr.dwInitialFrames = 0;

	// If we're interleaving, skew the audio by 3/4 of a second.
	if (dwInterleaveEvery > 0 && stream > 0) {
	    if (strhdr.fccType == streamtypeAUDIO) {
		strhdr.dwInitialFrames = AVIStreamTimeToSample(apavi[0], 750);
		DPF("Stream %d has %lu initial frames\n", stream, strhdr.dwInitialFrames);
	    }
	}
	
	
	//
	// Get stream format and write it out
	//
	cbFormat = dwBufferSize;
	hr = AVIStreamReadFormat(apavi[stream], AVIStreamStart(apavi[stream]),
				 lpBuffer, &cbFormat);
	if (hr != AVIERR_OK)
	    goto Error;

	// !!! Overflow?
	if (!cbFormat) {
	    // !!!
	}
	
	hr = AVIFileCreateStream(pfilesave, &apaviNew[stream], &strhdr);

#if 0
	if (hr != AVIERR_OK || apaviNew[stream] == NULL)
	    goto Error;
#else
	// If we can't make a stream, continue with the other streams....
	if (hr != AVIERR_OK || apaviNew[stream] == NULL) {
	    int i;

	    DPF("AVISave: Couldn't create stream in new file!\n");
	    AVIStreamClose(apavi[stream]);
	    
	    for (i = stream + 1; i < nStreams; i++) {
		apavi[stream] = apavi[stream + 1];
	    }
	    --nStreams;
	    --stream;
	    continue;
	}    
#endif

	hr = AVIStreamSetFormat(apaviNew[stream], 0, lpBuffer, cbFormat);
	if (hr != AVIERR_OK) {
	    DPF("Initial set format failed!\n");
	    goto Error;
	}
	
	cbFormat = dwBufferSize;
	hr = AVIStreamReadData(apavi[stream], ckidSTREAMHANDLERDATA,
				     lpBuffer, &cbFormat);
	// !!! overflow?
	
	if (hr == AVIERR_OK && cbFormat) {
	    /* 
	    ** Make the stream Data data chunk
	    */
	    // DPF2("Making stream %ld Data data chunk\n", stream);
	    hr = AVIStreamWriteData(apaviNew[stream], ckidSTREAMHANDLERDATA,
			lpBuffer, cbFormat);
	    if (hr != AVIERR_OK)
		goto Error;
	}

	if (strhdr.dwInitialFrames > hdrNew.dwInitialFrames)
	    hdrNew.dwInitialFrames = strhdr.dwInitialFrames;

	// !!! Should call ReadExtra and WriteExtra to move over information!
    }

    if (nStreams <= 0) {
	DPF("No streams at all accepted by the file!\n");
	goto Error;
    }
    
    //
    // We've written the header.  Now, there are two possibilities:
    //
    // 1.) File is interleaved.  We loop in time from beginning to end,
    //	    then loop through the streams and write out any data for the
    //	    current time.
    //
    // 2.) File is not interleaved.  We loop through the streams and
    //	    write each one out separately.
    //

    if (dwInterleaveEvery > 0) {
	DPF("Saving interleaved: factor = %lu, intial = %lu, total = %lu\n", dwInterleaveEvery, hdrNew.dwInitialFrames, hdrNew.dwTotalFrames);
    
	if (dwInterleaveEvery == 1) {
	    hdrNew.dwFlags |= AVIF_ISINTERLEAVED;
	    AVIFileEndRecord(pfilesave); // Make first record....
	}

	//
	// Interleaved case: loop from start to end...
	//
	for (l = - (LONG) hdrNew.dwInitialFrames;
		l < (LONG) hdrNew.dwTotalFrames;
		l += lInterval) {
	    //
	    // Loop through all of the streams to see what needs to be
	    // done at this time...
	    //	   
	    for (stream = 0; stream < nStreams; stream++) {
		LONG	lPos;
		LONG	lPosNext;
		
		LONG	lStart;
		LONG	lEnd;

		hr = AVIStreamInfo(apaviNew[stream], &strhdr, sizeof(strhdr));

		if (hr != AVIERR_OK)
		    goto Error;

		if (l < - (LONG) strhdr.dwInitialFrames)
		    continue;
		
		// !!! Better use of TWOCCs...
		if (strhdr.fccType == streamtypeAUDIO)
		    cktype = cktypeWAVEbytes;
		else if (strhdr.fccType == streamtypeVIDEO) {
		    if (strhdr.fccHandler == comptypeDIB)
			cktype = cktypeDIBbits;
		    else
			cktype = cktypeDIBcompressed;
		} else
		    cktype = aviTWOCC('x', 'x');

		//
		// Time is based on the first stream:
		// Right now, we want to write out any data in the current
		// stream that lines up between time <l> and <l+1> in the
		// first stream.
		//
		lPos = l + strhdr.dwInitialFrames;
		lPosNext = lPos + lInterval;

		lStart = lDone[stream];
		
		if (l >= (LONG) hdrNew.dwTotalFrames - lInterval) {
		    // If this is going to be the last time through the
		    // interleave loop, make sure everything gets written.
		    lEnd = AVIStreamEnd(apavi[stream]);
		} else {
		    //
		    // Complication: to make the audio come in bigger chunks,
		    // we only write it out every once in a while.
		    //
		    if (strhdr.fccType == streamtypeAUDIO && stream != 0) {
			if ((lPos % dwInterleaveEvery) != 0)
			    continue;

			lPosNext = lPos + dwInterleaveEvery;
		    }

		    if (stream != 0) {
			//
			// Figure out the data for this stream that needs to be
			// written this time....
			//
			lEnd = AVIStreamSampleToSample(apavi[stream], apavi[0], lPosNext);
		    } else {
			lEnd = min(lPosNext, (LONG) hdrNew.dwTotalFrames);
		    }
		}

		lDone[stream] = lEnd;

		//
		// Loop until we've read all we want.
		//
		while (lEnd > lStart) {
		    // !!! Right here, we should call AVIStreamGetFormat
		    // and then call AVIStreamSetFormat on the new
		    // streams.
		    // !!! Whose job is it to tell if the format has really
		    // changed?
		    cbFormat = dwBufferSize;
		    hr = AVIStreamReadFormat(apavi[stream],
					lStart,
					lpBuffer,
					&cbFormat);
		    if (hr != AVIERR_OK) {
			DPF("AVIStreamReadFormat failed!\n");
			goto Error;
		    }
		    
		    hr = AVIStreamSetFormat(apaviNew[stream],
					   lStart,
					   lpBuffer,
					   cbFormat);
		    if (hr != AVIERR_OK) {
			// !!! Oh, well: we couldn't write the palette change...
			DPF("AVIStreamSetFormat failed!\n");
		    }

ReadAgain0:
		    cbFormat = dwBufferSize;
		    dwSamplesRead = 0;
		   
		    hr = AVIStreamRead(apavi[stream], lStart,
					    lEnd - lStart,
					    lpBuffer, dwBufferSize,
					    &dwSize, &dwSamplesRead);

		    if (// dwSamplesRead == 0 &&
			    (GetScode(hr) == AVIERR_BUFFERTOOSMALL)) {
			//
			// The frame didn't fit in our buffer.
			// Make a bigger buffer.
			//
			dwBufferSize *= 2;
			DPF("Resizing buffer to be %lx bytes\n", dwBufferSize);
			lpBuffer = GlobalReAllocPtr(lpBuffer, dwBufferSize, GMEM_MOVEABLE);
			if (lpBuffer)
			    goto ReadAgain0;

			hr = ResultFromScode(AVIERR_MEMORY);
		    }

		    if (hr != 0) {
			DPF("AVISave: Error %08lx reading stream %d, position %ld!\n", (DWORD) hr, stream, lStart);
			goto Error;
		    }

		    dwFlags = 0; 

		    if (AVIStreamFindSample(apavi[stream], lStart, 
				FIND_KEY | FIND_PREV) == lStart)
			dwFlags |= AVIIF_KEYFRAME;
		    
		    hr = AVIStreamWrite(apaviNew[stream],
					  -1, dwSamplesRead,
					  lpBuffer, dwSize,
					  // cktype, // !!!
					  dwFlags, 0L, 0L);

		    if (hr != AVIERR_OK)
			goto Error;

		    lStart += dwSamplesRead;
		}
	    }

	    //
	    // Mark the end of the frame, in case we're writing out
	    // the "strict" interleaved format with LIST 'rec' chunks...
	    //
	    if (dwInterleaveEvery == 1) {
		hr = AVIFileEndRecord(pfilesave);
		if (hr != AVIERR_OK) {
		    DPF("AVISave: Error from EndRecord!\n");
		    goto Error;
		}
	    }

	    // Give the application a chance to update status and the user
	    // a chance to abort... 
	    if (lpfnCallback((int)
			     muldiv32(l + hdrNew.dwInitialFrames, 100,
				      hdrNew.dwInitialFrames +
					      hdrNew.dwTotalFrames))) {
		hr = ResultFromScode(AVIERR_USERABORT);
		DPF("AVISave: Aborted!\n");
		goto Error;
	    }
	}
    } else {
	//
	// Non-interleaved case: loop through the streams and write 
	// each one out by itself.
	//
	DPF("Saving non-interleaved.\n");
    
	for (stream = 0; stream < nStreams; stream++) {
	    if (lpfnCallback(MulDiv(stream, 100, nStreams))) {
		hr = ResultFromScode(AVIERR_USERABORT);
		goto Error;
	    }
		    
            AVIStreamInfo(apavi[stream], &strhdr, sizeof(strhdr));

	    DPF("Saving stream %d: start=%lx, len=%lx\n", stream, strhdr.dwStart, strhdr.dwLength);
	    
	    // !!! Need better cktype handling....
	    if (strhdr.fccType == streamtypeAUDIO)
		cktype = cktypeWAVEbytes;
	    else if (strhdr.fccType == streamtypeVIDEO) {
		if (strhdr.fccHandler == comptypeDIB)
		    cktype = cktypeDIBbits;
		else
		    cktype = cktypeDIBcompressed;
	    } else
		cktype = aviTWOCC('x', 'x');

	    //
	    // As usual, there are two possibilities:
	    //
	    // 1.) "wave-like" data, where lots of samples can be in
	    // a single chunk.  In this case, we write out big chunks
	    // with many samples at a time.
	    //
	    // 2.) "video-like" data, where each sample is a different
	    // size, and thus each must be written individually.
	    //
	    if (strhdr.dwSampleSize != 0) {
		/* It's wave-like data: lots of samples per chunk */

		l = strhdr.dwStart;
		while (l < (LONG) strhdr.dwLength) {
		    DWORD	dwRead;
		    
		    // Make the format of the new stream
		    // match the old one at every point....
		    //
		    // !!! Whose job is it to tell if the format has really
		    // changed?
		    cbFormat = dwBufferSize;
		    hr = AVIStreamReadFormat(apavi[stream],
					l,
					lpBuffer,
					&cbFormat);
		    if (hr != AVIERR_OK) {
			DPF("AVIStreamReadFormat failed!\n");
			goto Error;
		    }

		    hr = AVIStreamSetFormat(apaviNew[stream],
				       l,
				       lpBuffer,
				       cbFormat);
		    if (hr != AVIERR_OK) {
			DPF("AVIStreamSetFormat failed!\n");
			// !!! Oh, well: we couldn't write the palette change...
		    }


		    //
		    // Read some data...
		    //
ReadAgain1:
		    dwSize = dwBufferSize;
		    dwSamplesRead = 0;
		    dwRead = min(dwBufferSize / strhdr.dwSampleSize,
				 strhdr.dwLength - (DWORD) l);

		    hr = AVIStreamRead(apavi[stream], l, dwRead,
				       lpBuffer, dwBufferSize,
				       &dwSize, &dwSamplesRead);

		    if (dwSamplesRead == 0 &&
				(GetScode(hr) == AVIERR_BUFFERTOOSMALL)) {
			//
			// The frame didn't fit in our buffer.
			// Make a bigger buffer.
			//
			dwBufferSize *= 2;
			lpBuffer = GlobalReAllocPtr(lpBuffer, dwBufferSize, GMEM_MOVEABLE);
			if (lpBuffer)
			    goto ReadAgain1;
		    }

		    // !!! Check if format has changed

		    dwFlags = 0; // !!! KEYFRAME?

		    DPF("Save: Read %lx/%lx samples at %lx\n", dwSamplesRead, dwRead, l);
		    
		    if (hr != AVIERR_OK) {
			DPF("Save: Read failed! (%08lx) pos=%lx, len=%lx\n", (DWORD) hr, l, dwRead);

			goto Error;
		    }

		    if (dwSamplesRead == 0) {
			DPF("Ack: Read zero samples!");
			
			if (l + 1 == (LONG) strhdr.dwLength) {
			    DPF("Pretending it's OK, since this was the last one....");
			    break;
			}

			hr = ResultFromScode(AVIERR_FILEREAD);
			goto Error;
		    }
		    
		    l += dwSamplesRead;

		    //
		    // Write the data out...
		    //
		    hr = AVIStreamWrite(apaviNew[stream],
					  -1, dwSamplesRead,
					  lpBuffer, dwSize,
// !!!					  cktype, // !!!TWOCCFromFOURCC(ckid),
					  dwFlags, 0L, 0L);

		    if (hr != AVIERR_OK) {
			DPF("AVIStreamWrite failed! (%08lx)\n", (DWORD) hr);

			goto Error;
		    }
		    
		    if (lpfnCallback(MulDiv(stream, 100, nStreams) +
			   (int) muldiv32(l, 100,
					   nStreams * strhdr.dwLength))) {
			hr = ResultFromScode(AVIERR_USERABORT);
			goto Error;
		    }
		}
	    } else {
		/* It's video-like data: one sample (frame) per chunk */

		for (l = strhdr.dwStart;
			l < (LONG) strhdr.dwLength;
			l++) {
		    // !!! Right here, we should call AVIStreamGetFormat
		    // and then call AVIStreamSetFormat on the new
		    // streams.
		    // !!! Whose job is it to tell if the format has really
		    // changed?
		    
		    cbFormat = dwBufferSize;
		    hr = AVIStreamReadFormat(apavi[stream],
					l,
					lpBuffer,
					&cbFormat);
		    if (hr != AVIERR_OK) {
			DPF("AVIStreamReadFormat failed!\n");
			goto Error;
		    }

		    hr = AVIStreamSetFormat(apaviNew[stream],
				       l,
				       lpBuffer,
				       cbFormat);
		    if (hr != AVIERR_OK) {
			// !!! Oh, well: we couldn't write the palette change...
			DPF("AVIStreamSetFormat failed!\n");
		    }


    ReadAgain:
		    dwSize = dwBufferSize;
		    /* Write out a single frame.... */
		    dwSamplesRead = 0;
		    hr = AVIStreamRead(apavi[stream], l, 1,
					    lpBuffer, dwBufferSize,
					    &dwSize, &dwSamplesRead);

		    // !!! Check if format has changed (palette change)

		    if (dwSamplesRead == 0 &&
				(GetScode(hr) == AVIERR_BUFFERTOOSMALL)) {
			//
			// The frame didn't fit in our buffer.
			// Make a bigger buffer.
			//
			dwBufferSize *= 2;
			lpBuffer = GlobalReAllocPtr(lpBuffer, dwBufferSize, GMEM_MOVEABLE);
			if (lpBuffer)
			    goto ReadAgain;
		    }

		    if (dwSamplesRead != 1) {
			hr = ResultFromScode(AVIERR_FILEREAD);
			goto Error;
		    }

		    dwFlags = 0; // !!!!

		    //
		    // Check whether this should be marked a key frame.
		    //
		    // !!! shouldn't this be returned from AVIStreamRead()?
		    //
		    if (AVIStreamFindSample(apavi[stream], l, 
				FIND_KEY | FIND_PREV) == l)
			dwFlags |= AVIIF_KEYFRAME;

		    //
		    // Write the chunk out.
		    //
		    hr = AVIStreamWrite(apaviNew[stream],
					  -1, dwSamplesRead,
					  lpBuffer, dwSize,
// !!!					  cktype, // !!!TWOCCFromFOURCC(ckid),
					  dwFlags, 0L, 0L);

		    if (hr != AVIERR_OK)
			goto Error;

		    //
		    // Video frames can be big, so call back every time.
		    //
		    if (lpfnCallback(MulDiv(stream, 100, nStreams) +
			   (int) muldiv32(l, 100, nStreams * strhdr.dwLength))) {
			hr = ResultFromScode(AVIERR_USERABORT);
			goto Error;
		    }
		}
	    }
	}
    }

Error:
    //
    // We're done, one way or another.
    //
    
    /* Free buffer */
    if (lpBuffer) {
	GlobalFreePtr(lpBuffer);
    }

    // If everything's OK so far, finish writing the file.
    // Close the file, free resources associated with writing it.
    if (pfilesave) {
	// Release all of our new streams
	for (stream = 0; stream < nStreams; stream++) {
	    if (apaviNew[stream])
		AVIStreamClose(apaviNew[stream]);
	}
	
	if (hr != AVIERR_OK)
	    AVIFileClose(pfilesave);
	else {
	    // !!! ACK: AVIFileClose doesn't return an error! How do I tell
	    // if it worked?
	    // !!! does this mean I need a Flush() call?
	    /* hr = */ AVIFileClose(pfilesave);
	}
	
    }

    // Release all of our streams
    for (stream = 0; stream < nStreams; stream++) {
	if (apavi[stream])
	    AVIStreamClose(apavi[stream]);
    }
    
    if (hr != 0) {
	DPF("AVISave: Returning error %08lx\n", (DWORD) hr);
    }
    
    return hr;
}
