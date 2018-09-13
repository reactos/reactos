/****************************************************************************
 *
 *  AVIFILE.H
 *
 *  routines for reading Standard AVI files
 *
 *  Copyright (c) 1992, 1993 Microsoft Corporation.  All Rights Reserved.
 *
 *  You have a royalty-free right to use, modify, reproduce and
 *  distribute the Sample Files (and/or any modified version) in
 *  any way you find useful, provided that you agree that
 *  Microsoft has no warranty obligations or liability for any
 *  Sample Application Files which are modified.
 *
 ***************************************************************************/

#if !defined( _AVIFILE_H_ )
#define _AVIFILE_H_

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */


#define CreateStream CS // !!!ack
    
#ifndef mmioFOURCC
    #define mmioFOURCC( ch0, ch1, ch2, ch3 ) \
	( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |	\
	( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )
#endif

#ifndef streamtypeVIDEO
#define streamtypeVIDEO		mmioFOURCC('v', 'i', 'd', 's')
#define streamtypeAUDIO		mmioFOURCC('a', 'u', 'd', 's')
#define streamtypeMIDI		mmioFOURCC('m', 'i', 'd', 's')
#define streamtypeTEXT		mmioFOURCC('t', 'x', 't', 's')
#endif

#ifndef AVIIF_KEYFRAME
#define AVIIF_KEYFRAME      0x00000010L // this frame is a key frame.
#endif

//
// Structures used by AVIStreamInfo & AVIFileInfo.
//
// These are related to, but not identical to, the header chunks
// in an AVI file.
//
typedef struct _AVISTREAMINFO {
    DWORD		fccType;
    DWORD               fccHandler;
    DWORD               dwFlags;        /* Contains AVITF_* flags */
    DWORD		dwCaps;
    WORD		wPriority;
    WORD		wLanguage;
    DWORD               dwScale;        
    DWORD               dwRate; /* dwRate / dwScale == samples/second */
    DWORD               dwStart;
    DWORD               dwLength; /* In units above... */
    DWORD		dwInitialFrames;
    DWORD               dwSuggestedBufferSize;
    DWORD               dwQuality;
    DWORD               dwSampleSize;
    RECT                rcFrame;
    DWORD		dwEditCount;
    DWORD		dwFormatChangeCount;
    char		szName[64];
} AVISTREAMINFO, FAR * LPAVISTREAMINFO;

#define AVISTREAMINFO_DISABLED			0x00000001
#define AVISTREAMINFO_FORMATCHANGES		0x00010000

typedef struct _AVIFILEINFO {
    DWORD		dwMaxBytesPerSec;	// max. transfer rate
    DWORD		dwFlags;		// the ever-present flags
    DWORD		dwCaps;
    DWORD		dwStreams;
    DWORD		dwSuggestedBufferSize;
    
    DWORD		dwWidth;
    DWORD		dwHeight;
    
    DWORD		dwScale;	
    DWORD		dwRate;	/* dwRate / dwScale == samples/second */
    DWORD		dwLength;

    DWORD		dwEditCount;
    
    char		szFileType[64];		// descriptive string for file type?
} AVIFILEINFO, FAR * LPAVIFILEINFO;

// Flags for dwFlags
#define AVIFILEINFO_HASINDEX		0x00000010
#define AVIFILEINFO_MUSTUSEINDEX	0x00000020
#define AVIFILEINFO_ISINTERLEAVED	0x00000100
#define AVIFILEINFO_WASCAPTUREFILE	0x00010000
#define AVIFILEINFO_COPYRIGHTED		0x00020000

// Flags for dwCaps
#define AVIFILECAPS_CANREAD		0x00000001
#define AVIFILECAPS_CANWRITE		0x00000002
#define AVIFILECAPS_ALLKEYFRAMES	0x00000010
#define AVIFILECAPS_NOCOMPRESSION	0x00000020

typedef BOOL (FAR PASCAL * AVISAVECALLBACK)(int);

/************************************************************************/
/* Declaration for the AVICOMPRESSOPTIONS structure.  Make sure it 	*/
/* matches the AutoDoc in avisave.c !!!                            	*/
/************************************************************************/

typedef struct {
    DWORD	fccType;		    /* stream type, for consistency */
    DWORD       fccHandler;                 /* compressor */
    DWORD       dwKeyFrameEvery;            /* keyframe rate */
    DWORD       dwQuality;                  /* compress quality 0-10,000 */
    DWORD       dwBytesPerSecond;           /* bytes per second */
    DWORD       dwFlags;                    /* flags... see aviopts.h */
    LPVOID      lpFormat;                   /* save format */
    DWORD       cbFormat;
    LPVOID      lpParms;                    /* compressor options */
    DWORD       cbParms;
    DWORD       dwInterleaveEvery;          /* for non-video streams only */
} AVICOMPRESSOPTIONS, FAR *LPAVICOMPRESSOPTIONS;

//
// Defines for the dwFlags field of the AVICOMPRESSOPTIONS struct
// Each of these flags determines if the appropriate field in the structure
// (dwInterleaveEvery, dwBytesPerSecond, and dwKeyFrameEvery) is payed
// attention to.  See the autodoc in avisave.c for details.
// 
#define AVICOMPRESSF_INTERLEAVE		0x00000001    // interleave
#define AVICOMPRESSF_DATARATE		0x00000002    // use a data rate
#define AVICOMPRESSF_KEYFRAMES		0x00000004    // use keyframes
#define AVICOMPRESSF_VALID		0x00000008    // has valid data?

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif	/* __cplusplus */

#include "aviiface.h"


#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

//
// functions
//

STDAPI_(void) AVIFileInit(void);   // Call this first!
STDAPI_(void) AVIFileExit(void);

STDAPI_(ULONG) AVIFileAddRef       (PAVIFILE pfile);
STDAPI_(ULONG) AVIFileRelease      (PAVIFILE pfile);
STDAPI AVIFileOpen       (PAVIFILE FAR * ppfile, LPCSTR szFile,
			  UINT uMode, LPCLSID lpHandler);
STDAPI AVIFileInfo           (PAVIFILE pfile, AVIFILEINFO FAR * pfi, LONG lSize);
 
STDAPI AVIFileGetStream     (PAVIFILE pfile, PAVISTREAM FAR * ppavi, DWORD fccType, LONG lParam);
STDAPI AVIFileCreateStream  (PAVIFILE pfile,
					 PAVISTREAM FAR *ppavi,
					 AVISTREAMINFO FAR *psi);

STDAPI AVIFileWriteData	(PAVIFILE pfile,
					 DWORD ckid,
					 LPVOID lpData,
					 LONG cbData);
STDAPI AVIFileReadData	(PAVIFILE pfile,
					 DWORD ckid,
					 LPVOID lpData,
					 LONG FAR *lpcbData);
STDAPI AVIFileEndRecord	(PAVIFILE pfile);

STDAPI_(ULONG) AVIStreamAddRef       (PAVISTREAM pavi);
STDAPI_(ULONG) AVIStreamRelease      (PAVISTREAM pavi);
STDAPI AVIStreamInfo         (PAVISTREAM pavi, AVISTREAMINFO FAR * psi, LONG lSize);
STDAPI_(LONG) AVIStreamFindSample(PAVISTREAM pavi, LONG lPos, LONG lFlags);
STDAPI AVIStreamReadFormat   (PAVISTREAM pavi, LONG lPos,LPVOID lpFormat,LONG FAR *lpcbFormat);
STDAPI AVIStreamSetFormat    (PAVISTREAM pavi, LONG lPos,LPVOID lpFormat,LONG cbFormat);
STDAPI AVIStreamReadData     (PAVISTREAM pavi, DWORD fcc, LPVOID lp, LONG FAR *lpcb);
STDAPI AVIStreamWriteData    (PAVISTREAM pavi, DWORD fcc, LPVOID lp, LONG cb);

STDAPI AVIStreamRead         (PAVISTREAM pavi,
			      LONG lStart,
			      LONG lSamples,
			      LPVOID lpBuffer,
			      LONG cbBuffer,
			      LONG FAR * plBytes,
			      LONG FAR * plSamples);
#define AVISTREAMREAD_CONVENIENT	(-1L)

STDAPI AVIStreamWrite        (PAVISTREAM pavi,
			      LONG lStart, LONG lSamples,
			      LPVOID lpBuffer, LONG cbBuffer, DWORD dwFlags,
			      LONG FAR *plSampWritten,
			      LONG FAR *plBytesWritten);

// Right now, these just use AVIStreamInfo() to get information, then
// return some of it.  Can they be more efficient?
STDAPI_(LONG) AVIStreamStart        (PAVISTREAM pavi);
STDAPI_(LONG) AVIStreamLength       (PAVISTREAM pavi);
STDAPI_(LONG) AVIStreamTimeToSample (PAVISTREAM pavi, LONG lTime);
STDAPI_(LONG) AVIStreamSampleToTime (PAVISTREAM pavi, LONG lSample);


STDAPI AVIStreamBeginStreaming(PAVISTREAM pavi, LONG lStart, LONG lEnd, LONG lRate);
STDAPI AVIStreamEndStreaming(PAVISTREAM pavi);

//
// helper functions for using IGetFrame
//
STDAPI_(PGETFRAME) AVIStreamGetFrameOpen(PAVISTREAM pavi,
					 LPBITMAPINFOHEADER lpbiWanted);
STDAPI_(LPVOID) AVIStreamGetFrame(PGETFRAME pg, LONG lPos);
STDAPI AVIStreamGetFrameClose(PGETFRAME pg);


// !!! We need some way to place an advise on a stream....
// STDAPI AVIStreamHasChanged   (PAVISTREAM pavi);



// Shortcut function
STDAPI AVIStreamOpenFromFile(PAVISTREAM FAR *ppavi, LPCSTR szFile,
			     DWORD fccType, LONG lParam,
			     UINT mode, CLSID FAR *pclsidHandler);

// Use to create disembodied streams
STDAPI AVIStreamCreate(PAVISTREAM FAR *ppavi, LONG lParam1, LONG lParam2,
		       CLSID FAR *pclsidHandler);



// PHANDLER    AVIAPI AVIGetHandler         (PAVISTREAM pavi, PAVISTREAMHANDLER psh);
// PAVISTREAM  AVIAPI AVIGetStream          (PHANDLER p);

//
// flags for AVIStreamFindSample
//
#define FIND_DIR        0x0000000FL     // direction
#define FIND_NEXT       0x00000001L     // go forward
#define FIND_PREV       0x00000004L     // go backward

#define FIND_TYPE       0x000000F0L     // type mask
#define FIND_KEY        0x00000010L     // find key frame.
#define FIND_ANY        0x00000020L     // find any (non-empty) sample
#define FIND_FORMAT     0x00000040L     // find format change

#define FIND_RET        0x0000F000L     // return mask
#define FIND_POS        0x00000000L     // return logical position
#define FIND_LENGTH     0x00001000L     // return logical size
#define FIND_OFFSET     0x00002000L     // return physical position
#define FIND_SIZE       0x00003000L     // return physical size
#define FIND_INDEX      0x00004000L     // return physical index position


//
//  stuff to support backward compat.
//
#define AVIStreamFindKeyFrame AVIStreamFindSample
#define FindKeyFrame	FindSample

#define AVIStreamClose AVIStreamRelease
#define AVIFileClose   AVIFileRelease
#define AVIStreamInit  AVIFileInit
#define AVIStreamExit  AVIFileExit

#define SEARCH_NEAREST  FIND_PREV
#define SEARCH_BACKWARD FIND_PREV
#define SEARCH_FORWARD  FIND_NEXT
#define SEARCH_KEY      FIND_KEY
#define SEARCH_ANY      FIND_ANY

//
//  helper macros.
//
#define     AVIStreamSampleToSample(pavi1, pavi2, l) \
            AVIStreamTimeToSample(pavi1,AVIStreamSampleToTime(pavi2, l))

#define     AVIStreamNextSample(pavi, l) \
            AVIStreamFindSample(pavi,l+1,FIND_NEXT|FIND_ANY)

#define     AVIStreamPrevSample(pavi, l) \
            AVIStreamFindSample(pavi,l-1,FIND_PREV|FIND_ANY)

#define     AVIStreamNearestSample(pavi, l) \
            AVIStreamFindSample(pavi,l,FIND_PREV|FIND_ANY)

#define     AVIStreamNextKeyFrame(pavi,l) \
            AVIStreamFindSample(pavi,l+1,FIND_NEXT|FIND_KEY)

#define     AVIStreamPrevKeyFrame(pavi, l) \
            AVIStreamFindSample(pavi,l-1,FIND_PREV|FIND_KEY)

#define     AVIStreamNearestKeyFrame(pavi, l) \
            AVIStreamFindSample(pavi,l,FIND_PREV|FIND_KEY)

#define     AVIStreamIsKeyFrame(pavi, l) \
            (AVIStreamNearestKeyFrame(pavi,l) == l)

#define     AVIStreamPrevSampleTime(pavi, t) \
            AVIStreamSampleToTime(pavi, AVIStreamPrevSample(pavi,AVIStreamTimeToSample(pavi,t)))

#define     AVIStreamNextSampleTime(pavi, t) \
            AVIStreamSampleToTime(pavi, AVIStreamNextSample(pavi,AVIStreamTimeToSample(pavi,t)))

#define     AVIStreamNearestSampleTime(pavi, t) \
            AVIStreamSampleToTime(pavi, AVIStreamNearestSample(pavi,AVIStreamTimeToSample(pavi,t)))

#define     AVIStreamNextKeyFrameTime(pavi, t) \
            AVIStreamSampleToTime(pavi, AVIStreamNextKeyFrame(pavi,AVIStreamTimeToSample(pavi, t)))

#define     AVIStreamPrevKeyFrameTime(pavi, t) \
            AVIStreamSampleToTime(pavi, AVIStreamPrevKeyFrame(pavi,AVIStreamTimeToSample(pavi, t)))

#define     AVIStreamNearestKeyFrameTime(pavi, t) \
            AVIStreamSampleToTime(pavi, AVIStreamNearestKeyFrame(pavi,AVIStreamTimeToSample(pavi, t)))

#define     AVIStreamStartTime(pavi) \
            AVIStreamSampleToTime(pavi, AVIStreamStart(pavi))

#define     AVIStreamLengthTime(pavi) \
            AVIStreamSampleToTime(pavi, AVIStreamLength(pavi))

#define     AVIStreamEnd(pavi) \
            (AVIStreamStart(pavi) + AVIStreamLength(pavi))

#define     AVIStreamEndTime(pavi) \
            AVIStreamSampleToTime(pavi, AVIStreamEnd(pavi))

#define     AVIStreamSampleSize(pavi, lPos, plSize) \
	    AVIStreamRead(pavi,lPos,1,NULL,0,plSize,NULL)

#define     AVIStreamFormatSize(pavi, lPos, plSize) \
            AVIStreamReadFormat(pavi,lPos,NULL,plSize)

#define     AVIStreamDataSize(pavi, fcc, plSize) \
            AVIStreamReadData(pavi,fcc,NULL,plSize)

/****************************************************************************
 *
 *  AVISave routines and structures
 *
 ***************************************************************************/

#ifndef comptypeDIB
#define comptypeDIB         mmioFOURCC('D', 'I', 'B', ' ')
#endif

STDAPI AVIMakeCompressedStream(
		PAVISTREAM FAR *	    ppsCompressed,
		PAVISTREAM		    ppsSource,
		AVICOMPRESSOPTIONS FAR *    lpOptions,
		CLSID FAR *pclsidHandler);

EXTERN_C HRESULT CDECL AVISave (LPCSTR               szFile,
		CLSID FAR *pclsidHandler,
		AVISAVECALLBACK     lpfnCallback,
		int                 nStreams,
		PAVISTREAM	    pfile,
		LPAVICOMPRESSOPTIONS lpOptions,
		...);

STDAPI AVISaveV(LPCSTR               szFile,
		CLSID FAR *pclsidHandler,
		AVISAVECALLBACK     lpfnCallback,
		int                 nStreams,
		PAVISTREAM FAR *    ppavi,
		LPAVICOMPRESSOPTIONS FAR *plpOptions);

STDAPI_(BOOL) AVISaveOptions(HWND hwnd,
			     UINT	uiFlags,
			     int	nStreams,
			     PAVISTREAM FAR *ppavi,
			     LPAVICOMPRESSOPTIONS FAR *plpOptions);

STDAPI AVISaveOptionsFree(int nStreams,
			     LPAVICOMPRESSOPTIONS FAR *plpOptions);

// FLAGS FOR uiFlags:
//
// Same as the flags for ICCompressorChoose (see compman.h)
// These determine what the compression options dialog for video streams
// will look like.

STDAPI AVIBuildFilter(LPSTR lpszFilter, LONG cbFilter, BOOL fSaving);

STDAPI AVIMakeFileFromStreams(PAVIFILE FAR *	ppfile,
			       int		nStreams,
			       PAVISTREAM FAR *	papStreams);

STDAPI AVIMakeStreamFromClipboard(UINT cfFormat, HANDLE hGlobal, PAVISTREAM FAR *ppstream);

/****************************************************************************
 *
 *  Clipboard routines
 *
 ***************************************************************************/

STDAPI AVIPutFileOnClipboard(PAVIFILE pf);

STDAPI AVIGetFromClipboard(PAVIFILE FAR * lppf);

STDAPI AVIClearClipboard(void);

/****************************************************************************
 *
 *  Editing routines
 *
 ***************************************************************************/
STDAPI CreateEditableStream(
		PAVISTREAM FAR *	    ppsEditable,
		PAVISTREAM		    psSource);

STDAPI EditStreamCut(PAVISTREAM pavi, LONG FAR *plStart, LONG FAR *plLength, PAVISTREAM FAR * ppResult);

STDAPI EditStreamCopy(PAVISTREAM pavi, LONG FAR *plStart, LONG FAR *plLength, PAVISTREAM FAR * ppResult);

STDAPI EditStreamPaste(PAVISTREAM pavi, LONG FAR *plPos, LONG FAR *plLength, PAVISTREAM pstream, LONG lStart, LONG lEnd);

STDAPI EditStreamClone(PAVISTREAM pavi, PAVISTREAM FAR *ppResult);

STDAPI EditStreamSetName(PAVISTREAM pavi, LPCSTR lpszName);
STDAPI EditStreamSetInfo(PAVISTREAM pavi, AVISTREAMINFO FAR * lpInfo, LONG cbInfo);


#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif	/* __cplusplus */

#endif


/*	-	-	-	-	-	-	-	-	*/

#ifndef AVIERR_OK
#define AVIERR_OK               0L

#define MAKE_AVIERR(error)	MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x4000 + error)

// !!! Questions to be answered:
// How can you get a string form of these errors?
// Which of these errors should be replaced by errors in SCODE.H?
#define AVIERR_UNSUPPORTED      MAKE_AVIERR(101)
#define AVIERR_BADFORMAT        MAKE_AVIERR(102)
#define AVIERR_MEMORY           MAKE_AVIERR(103)
#define AVIERR_INTERNAL         MAKE_AVIERR(104)
#define AVIERR_BADFLAGS         MAKE_AVIERR(105)
#define AVIERR_BADPARAM         MAKE_AVIERR(106)
#define AVIERR_BADSIZE          MAKE_AVIERR(107)
#define AVIERR_BADHANDLE        MAKE_AVIERR(108)
#define AVIERR_FILEREAD         MAKE_AVIERR(109)
#define AVIERR_FILEWRITE        MAKE_AVIERR(110)
#define AVIERR_FILEOPEN         MAKE_AVIERR(111)
#define AVIERR_COMPRESSOR       MAKE_AVIERR(112)
#define AVIERR_NOCOMPRESSOR     MAKE_AVIERR(113)
#define AVIERR_READONLY		MAKE_AVIERR(114)
#define AVIERR_NODATA		MAKE_AVIERR(115)
#define AVIERR_BUFFERTOOSMALL	MAKE_AVIERR(116)
#define AVIERR_CANTCOMPRESS	MAKE_AVIERR(117)
#define AVIERR_USERABORT        MAKE_AVIERR(198)
#define AVIERR_ERROR            MAKE_AVIERR(199)
#endif
