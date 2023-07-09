/****************************************************************************
 *
 *  AVIFILE.D
 *
 *  Autodoc for the AVI clipboard and editing functions.
 *
 ***************************************************************************/


//
// Defines for the dwFlags field of the AVICOMPRESSOPTIONS struct
//
#define AVICOMPRESSF_INTERLEAVE		0x00000001
#define AVICOMPRESSF_DATARATE		0x00000002
#define AVICOMPRESSF_KEYFRAMES		0x00000004
#define AVICOMPRESSF_VALID		0x00000008	// has valid data?

STDAPI_(void) AVIFileInit(void);   // Call this first!
STDAPI_(void) AVIFileExit(void);


#define MAKE_AVIERR(error)	MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x4000 + error)

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


/*****************************************************************************
* @doc EXTERNAL AVISTREAMINFO
* 
* @types AVISTREAMINFO | This structure contains information
*        for a single stream. It is similar to, but not 
*        identical to <t AVIStreamHeader> found in AVI files.
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
*        four-character code of the compressor handler that 
*        will compress this stream when it is saved 
*        (For example, mmioFOURCC('M','S','V','C')). 
*        This member is not used for audio streams.
* 
* @field DWORD | dwFlags | Specifies any applicable flags. The bits 
*        in the high-order word of these flags are specific 
*        to the type of data contained in the stream. The following 
*        flags are defined:
*
* @flag AVISF_DISABLED | Indicates this stream should not be enabled by default.
* @flag AVISF_VIDEO_PALCHANGES | Indicates this video stream contains 
*       palette changes. This flag warns the playback software that it will need 
*       to animate the palette.
*
* @field DWORD | dwCaps | Specifies capability flags. This is currently 
*        unused.
*
* @field WORD | wPriority | Specifies the priority of the stream.
*
* @field WORD | wLanguage | Specifies the language of the stream.
*
* @field DWORD | dwScale | This member is used with <e AVISTREAMINFO.dwRate> 
*        to specify the time scale this stream will use.
*        Dividing <e AVISTREAMINFO.dwRate> by <e AVISTREAMINFO.dwScale> 
*        gives the number of samples per second.
*        For video streams, this rate should be the frame rate.
*        For audio streams, this rate should correspond to the time 
*        needed for <e WAVEFORMAT.nBlockAlign> bytes of audio, 
*        which for PCM audio simply reduces to the sample rate.
*
* @field DWORD | dwRate | See <e AVISTREAMINFO.dwScale>.
* 
* @field DWORD | dwStart | Specifies the starting time of the AVI file. 
*        The units are defined by the <e AVISTREAMINFO.dwRate> 
*        and <e AVISTREAMINFO.dwScale> members of this structure.
*        Normally, this is zero, but it can specify a 
*        delay time for a stream which does not start concurrently 
*        with the file.
*        Note: The 1.0 release of the AVI tools does not support 
*        a non-zero starting time.
* 
* @field DWORD | dwLength | Specifies the length of this stream. 
*        The units are defined by the <e AVISTREAMINFO.dwRate> 
*        and <e AVISTREAMINFO.dwScale> members of this structure.
*
* @field DWORD | dwInitialFrames | Specifies how far audio 
*        data is skewed ahead of the video frames in 
*        interleaved files. Typically, this is about 0.75 seconds.
*
* @field DWORD | dwSuggestedBufferSize | Suggests how large a buffer 
*        should be used to read this stream. Typically, this 
*        contains a value corresponding to the largest chunk 
*        present in the stream. Using the correct buffer size 
*        makes playback more efficient. Use zero if you 
*        do not know the correct buffer size.
*
* @field DWORD | dwQuality | Specifies an indicator of the quality of 
*        the data in the stream. Quality is represented as a number 
*        between 0 and 10000. For compressed data, this typically represents 
*        the value of the quality parameter passed to the compression software. 
*        If set to -1, drivers use the default quality value. 
*
* @field DWORD | dwSampleSize | Specifies the size of a single 
*        sample of data. This is set to zero if the samples can 
*        vary in size. If this number is non-zero, then multiple 
*        samples of data can be grouped into a single chunk 
*        within the file. If it is zero, each sample of data (such as a 
*        video frame) must be in a separate chunk.
*
*        For video streams, this number is typically zero, although it can 
*        be non-zero if all video frames are the same size. For audio streams, 
*        this number should be the same as the <e WAVEFORMAT.nBlockAlign> 
*        member of the <t WAVEFORMAT> structure describing the audio.
*
* @field RECT | rcFrame | Specifies the dimensions of the destination 
*        rectangle for the video. The coordinates rpresent the 
*        the upper-left corner, and the height and width.
* 
* @field DWORD | dwEditCount | Specifies the number of times the 
*        stream has been edited. The stream handler maintains this 
*        count. 
*
* @field DWORD | dwFormatChangeCount | Specifies the number of times 
*        stream format has changed. The stream handler maintains this 
*        count.
* 
* @field char | szName[64] | Specifies a zero-terminated string containing 
*        a description of the stream.
 * 
 ***************************************************************************/

/*****************************************************************************
* @doc EXTERNAL AVIFILEINFO
* 
* @types AVIFILEINFO | This structure contains global information
*        for an entire AVI file. It is similar to, but not 
*        identical to <t MainAVIHeader> found in AVI files.
*
* @field DWORD | dwMaxBytesPerSec | Specifies the approximate maximum 
*        data rate of file.
*
* @field DWORD | dwFlags | Specifies any applicable flags. 
*        The following flags are defined:
*
* @flag AVIF_HASINDEX | Indicates the AVI file has 
*       an index at the end of the file. For good performance, 
*       all AVI files should contain an index.
* @flag AVIF_MUSTUSEINDEX | Indicates that the index, rather than the 
*       physical ordering of the chunks in the file, should be used to 
*       determine the order of presentation of the data. For example, this 
*       could be used for creating a list frames for editing.
* @flag AVIF_ISINTERLEAVED | Indicates the AVI file is interleaved.
* @flag AVIF_WASCAPTUREFILE | Indicates the AVI file is a specially 
*       allocated file used for capturing real-time video. Applications 
*       should warn the user before writing over a file with this flag set 
*       because the user probably defragmented this file.
* @flag AVIF_COPYRIGHTED | Indicates the AVI file contains copyrighted 
*       data and software. When this flag is used, software should 
*       not permit the data to be duplicated.
*
* @field DWORD | dwCaps | Specifies capability flags. This is currently unused.
* 
* @field DWORD | dwStreams | Specifies the number of streams in the file.
*        For example, a file with audio and video has two streams.
* 
* @field DWORD | dwSuggestedBufferSize | Specifies the suggested buffer 
*        size for reading the file. Generally, this size should be large 
*        enough to contain the largest chunk in the file. If set to zero, or 
*        if it is too small, the playback software will have to reallocate 
*        memory during playback which will reduce performance. 
* 
*        For an interleaved file, this buffer size should be large enough to read 
*        an entire record and not just a chunk.
* 
* @field DWORD | dwWidth | Specifies the width of the AVI file in pixels.
* 
* @field DWORD | dwHeight | Specifies the height of the AVI file in pixels.
* 
* @field DWORD | dwScale | This field is used with <e AVIFILEINFO.dwRate> 
*        to specify the time scale that the file as a whole will use. In addition, 
*        each stream can have its own time scale. 
*        Dividing <e AVIFILEINFO.dwRate> by <e AVIFILEINFO.dwScale> 
*        gives the number of samples per second.
* 
* @field DWORD | dwRate | See <e AVIFILEINFO.dwScale>.
* 
* @field DWORD | dwLength | Specifies the length of the AVI file. 
*        The units are defined by <e AVIFILEINFO.dwRate> and 
*        <e AVIFILEINFO.dwScale> .
* 
* @field DWORD | dwEditCount | Specifies the number of streams that have 
*        been added or deleted.
* 
* @field char | szFileType[64] | Specifies a zero-terminated string 
*        containing descriptive information for the file type.
* 
 ***************************************************************************/

/**************************************************************************
* @doc EXTERNAL AVIPutFileOnClipboard
*
* @api STDAPI | AVIPutFileOnClipboard | Copies an AVI file onto the Clipboard.
*
* @parm PAVIFILE | pf | Specifies a handle to an open AVI file.
*
* @rdesc Returns zero if successful; otherwise returns an error code.
*
* @comm This function will also copy data with the CF_DIB and 
*       CF_WAVE Clipboard flags onto the Clipboard.
*
*************************************************************************/ 

/**************************************************************************
* @doc EXTERNAL AVIGetFromClipboard
*
* @api STDAPI | AVIGetFromClipboard | Copies an AVI file from the Clipboard.
*
* @parm PAVIFILE * | lppf | Specifies the location used to return 
*       the handle created for the AVI file.
*
* @rdesc Returns zero if successful; otherwise returns an error code.
* 
* @comm This function will also copy data with the CF_DIB or CF_WAVE 
*       Clipboard flags.
*
*************************************************************************/ 

/**************************************************************************
* @doc EXTERNAL AVIClearClipboard
*
* @api STDAPI | AVIClearClipboard | Clears an AVI file from the Clipboard. 
*      Use this function before ending your application to clear the 
*      contents of the Clipboard.
*
* @rdesc Returns zero if successful; otherwise returns an error code.
*
*************************************************************************/ 

/**************************************************************************
* @doc EXTERNAL CreateEditableStream
*
* @api STDAPI | CreateEditableStream | Creates an editable stream. 
*      Use this function before using other <f EditStream> functions.
*
* @parm PAVISTREAM FAR * | ppsEditable | Specifies a pointer to the location 
*       used to return the handle to the new stream.
*
* @parm PAVISTREAM | psSource | Specifies a handle to the open stream 
*       used as the source of data. Specify NULL to create 
*       an empty editable string that you can copy and paste data 
*       into. 
*
* @rdesc Returns zero if successful; otherwise returns an error code.
*
* @comm The stream pointer returned in <p ppsEditable> must be 
*       used as the source stream in the other <f EditStream> functions. 
*
*       Internally, this function creates tables to keep track of 
*       changes to a stream. The original stream is never changed 
*       by the <f EditStream> functions. The stream pointer 
*       created by this function can be used in any AVIFile function 
*       that accepts stream pointers. You can use this function on 
*       the same stream multiple times. A copy of a stream is not 
*       affected by changes in another copy.
*
*************************************************************************/ 

/**************************************************************************
* @doc EXTERNAL EditStreamCut
*
* @api STDAPI | EditStreamCut | Deletes an editable 
*      stream (or a portion of it) and copies it into a temporary stream.
*
* @parm PAVISTREAM | pavi | Specifies a handle to the editable stream used 
*       as the source of the data.
*
* @parm LONG FAR * | plStart | Specifies the starting position of the data 
*       to be cut from the stream referenced by <p pavi>.
*
* @parm LONG FAR * | plLength | Specifies the length of the data 
*       to be cut from the stream referenced by <p pavi>.
*
* @parm PAVISTREAM FAR * | ppResult | Specifies a pointer to the location 
*       used to return the handle created for the new stream.
*
* @comm The editable stream must be created by <f CreateEditableStream> 
*       or one of the <f EditStream> functions.
*
*       The temporary stream is an editable stream and 
*       can be treated as any other AVI stream. 
*
* @rdesc Returns zero if successful; otherwise returns an error code.
*
*************************************************************************/ 

/**************************************************************************
* @doc EXTERNAL EditStreamCopy
*
* @api STDAPI | EditStreamCopy | Copies an editable 
*      stream (or a portion of it) into a temporary stream.
*
* @parm PAVISTREAM | pavi | Specifies a handle to an editable stream used 
*       as the source of the data.
*
* @parm LONG FAR * | plStart | Specifies the starting position of the data 
*       to be copied from the stream referenced by <p pavi>. The 
*       actual starting postion is returned.
*
* @parm LONG FAR * | plLength | Specifies the length of the data 
*       to be copied from the stream referenced by <p pavi>. 
*       The actual length of the copied data is returned.
*
* @parm PAVISTREAM FAR * | ppResult | Specifies a pointer to the location 
*       used to return the handle created for the new stream.
*
* @rdesc Returns zero if successful; otherwise returns an error code.
*
* @comm The editable stream must be created by <f CreateEditableStream> 
*       or one of the <f EditStream> functions.
*
*       The temporary stream can be treated as any other AVI stream. 
*
*************************************************************************/ 

/**************************************************************************
* @doc EXTERNAL EditStreamPaste
*
* @api STDAPI | EditStreamPaste | Copies a stream (or a portion of it) into 
*      another stream.
*
* @parm PAVISTREAM | pavi | Specifies a handle to a editable stream 
*	     used as the destination of data from the stream specified 
*       for <p pstream>.
*
* @parm LONG FAR * | plPos | Specifies the starting position of the data 
*       to be copied from the stream referenced by <p pavi>.  On returning
*	from the function, <p plPos>
*
* @parm LONG FAR * | plLength | Specifies a pointer to where the length
*	of the data pasted in will be returned.  
*
* @parm PAVISTREAM | pstream | Specifies a handle to a stream used 
*       as the source of the data.  This stream does not need to 
*       be an editable stream.
*
* @parm LONG | lStart | Specifies the starting position within the stream
*	referenced by <p pstream> of the portion that will be pasted into
*	stream <p pavi>.
*
* @parm LONG | lLength | Specifies the length of the data 
*       to be copied to the stream specified by <p pavi>.  If <p lLength>
*	is -1, the entire stream <p pstream> will be pasted in.
*
* @rdesc Returns zero if successful; otherwise returns an error code.
*
* @comm The stream <p pavi> must be created by <f CreateEditableStream> 
*       or one of the <f EditStream> functions.
*
* @xref	<f CreateEditableStream> <f EditStreamCopy> <f EditStreamCut>
*
*************************************************************************/ 

/**************************************************************************
* @doc EXTERNAL EditStreamClone
*
* @api STDAPI | EditStreamClone | Creates a duplicate editable 
*      stream.
*
* @parm PAVISTREAM | pavi | Specifies a handle to an editable stream.
*
* @parm PAVISTREAM FAR * | ppResult | Specifies the location used 
*       to return the handle to the stream created. This stream is 
*       an editable stream.
*
* @rdesc Returns zero if successful; otherwise returns an error code.
*
* @comm The editable stream must be created by <f CreateEditableStream> 
*       or one of the <f EditStream> functions.
*
*       The stream created can be treated as any other AVI stream. 
*
*       You can use this function as the basis of an "undo" feature. 
*       You can also use this function to create a stream copied to 
*       the clipboard. 
*
*************************************************************************/ 

/**************************************************************************
* @doc EXTERNAL EditStreamSetInfo
*
* @api SDTAPI | EditStreamSetInfo | Changes characteristics of an 
*	editable stream.
*
* @parm PAVISTREAM | pavi | Specifies a handle to an open stream.
*
* @parm AVISTREAMINFO FAR * | lpInfo | Points to an <t AVISTREAMINFO>
*	structure containing new information.
*
* @parm LONG | cbInfo | Specifies the size of the structure pointed to
*	by <p lpInfo>.
*
* @rdesc Returns zero if successful; otherwise returns an error code.
*
* @comm You must fill out the <t AVISTREAMINFO> structure, including 
*	the members you will not use. You can use <f AVIStreamInfo> to 
*	to initialize the structure and then updated selected members 
*  with your data.
*
*	This function does not change the following members: 
*
*		<e AVISTREAMINFO.fccType> 
*
*		<e AVISTREAMINFO.fccHandler>
*
*		<e AVISTREAMINFO.dwFlags> 
*
*		<e AVISTREAMINFO.dwCaps> 
*
*		<e AVISTREAMINFO.dwLength> 
*
*		<e AVISTREAMINFO.dwInitialFrames> 
*
*		<e AVISTREAMINFO.dwSuggestedBufferSize> 
*
*		<e AVISTREAMINFO.dwSampleSize> 
*
*		<e AVISTREAMINFO.dwEditCount>
*
*	It does change: 
*
*		<e AVISTREAMINFO.wPriority> 
*
*		<e AVISTREAMINFO.wLanguage> 
*
*		<e AVISTREAMINFO.dwScale> 
*
*		<e AVISTREAMINFO.dwRate> 
*
*		<e AVISTREAMINFO.dwStart> 
*
*		<e AVISTREAMINFO.dwQuality> 
*
*		<e AVISTREAMINFO.rcFrame> 
*
*		<e AVISTREAMINFO.szName>
*
*************************************************************************/ 

/**************************************************************************
* @doc EXTERNAL EditStreamSetName
*
* @api SDTAPI | EditStreamSetName | Assigns a descriptive string to a stream.
*
* @parm PAVISTREAM | pavi | Specifies a handle to an open stream.
*
* @parm LPCSTR | lpszName | Specifies a zero-terminated string 
*       containing the description of the stream.
*
* @rdesc Returns zero if successful; otherwise returns an error code.
*
* @comm This function updates the <e AVISTREAMINFO.szName> member 
*       of the <t AVISTREAMINFO> structure.
*
*************************************************************************/ 

STDAPI AVIMakeStreamFromClipboard(UINT cfFormat, HANDLE hGlobal, PAVISTREAM FAR *ppstream);

/**************************************************************************
* @doc EXTERNAL AVIMakeStreamFromClipboard
*
* @api SDTAPI | AVIMakeStreamFromClipboard | Creates a stream from a 
*      stream on the Clipboard.
*
* @parm UINT | cfFormat | Specifies a Clipboard flag.
*
* @parm HANDLE | hGlobal | Specifies a handle to an open stream.
*
* @parm PAVISTREAM FAR * | ppstream | Specifies a handle to an open stream.
*
* @rdesc Returns zero if successful; otherwise returns an error code.
*
*************************************************************************/ 
