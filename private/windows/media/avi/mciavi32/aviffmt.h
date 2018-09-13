/****************************************************************************/
/*                                                                          */
/*        AVIFFMT.H - Include file for working with AVI files               */
/*                                                                          */
/*        Note: You must include WINDOWS.H and MMSYSTEM.H before            */
/*        including this file.                                              */
/*                                                                          */
/*        Copyright (c) 1991-1992, Microsoft Corp.  All rights reserved.    */
/*                                                                          */
/****************************************************************************/

/*
 *
 * An AVI file is the following RIFF form:
 *
 *	RIFF('AVI' 
 *	      LIST('hdrl'
 *		    avih(<MainAVIHeader>)
 *                  LIST ('strl'
 *                      strh(<Stream header>)
 *                      strf(<Stream format>)
 *                      ... additional header data
 *            LIST('movi'	 
 *      	  { LIST('rec' 
 *      		      SubChunk...
 *      		   )
 *      	      | SubChunk } ....	    
 *            )
 *            [ <AVIIndex> ]
 *      )
 *
 *      The first two characters of each chunk are the track number.
 *      SubChunk = {  xxdh(<AVI DIB header>)
 *                  | xxdb(<AVI DIB bits>)
 *                  | xxdc(<AVI compressed DIB bits>)
 *                  | xxpc(<AVI Palette Change>)
 *                  | xxwb(<AVI WAVE bytes>)
 *                  | xxws(<AVI Silence record>)
 *                  | xxmd(<MIDI data>)
 *                  | additional custom chunks }
 *
 */
/*
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * We need a better description of the AVI file header here.
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 * The grouping into LIST 'rec' chunks implies only that the contents of
 *   the chunk should be read into memory at the same time.  This
 *   grouping is only necessary for interleaved files.
 *       
 * For loading efficiency, the beginning of each LIST 'rec' chunk may
 * be aligned on a 2K boundary.  (Actually, the beginning of the LIST
 * chunk should be 12 bytes before a 2K boundary, so that the data chunks
 * inside the LIST chunk are aligned.)
 *
 * If the AVI file is being played from CD-ROM in, it is recommended that
 * the file be padded.
 *
 * Limitations for the Alpha release:
 *	If the AVI file has audio, each record LIST must contain exactly
 *	one audio chunk, which must be the first chunk.
 *	Each record must contain exactly one video chunk (possibly preceded
 *	by one or more palette change chunks).
 *	No wave format or DIB header chunks may occur outside of the header.
 */

#ifndef _INC_AVIFFMT
#define _INC_AVIFFMT

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif  /* RC_INVOKED */

#ifndef mmioFOURCC
#define mmioFOURCC( ch0, ch1, ch2, ch3 )				\
		( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |	\
		( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )
#endif

/* Macro to make a TWOCC out of two characters */
#ifndef aviTWOCC
#define aviTWOCC(ch0, ch1) ((WORD)(BYTE)(ch0) | ((WORD)(BYTE)(ch1) << 8))
#endif

typedef WORD TWOCC;

/* form types, list types, and chunk types */
#define formtypeAVI             mmioFOURCC('A', 'V', 'I', ' ')
#define listtypeAVIHEADER       mmioFOURCC('h', 'd', 'r', 'l')
#define ckidAVIMAINHDR          mmioFOURCC('a', 'v', 'i', 'h')
#define listtypeSTREAMHEADER    mmioFOURCC('s', 't', 'r', 'l')
#define ckidSTREAMHEADER        mmioFOURCC('s', 't', 'r', 'h')
#define ckidSTREAMFORMAT        mmioFOURCC('s', 't', 'r', 'f')
#define ckidSTREAMHANDLERDATA   mmioFOURCC('s', 't', 'r', 'd')

#define listtypeAVIMOVIE        mmioFOURCC('m', 'o', 'v', 'i')
#define listtypeAVIRECORD       mmioFOURCC('r', 'e', 'c', ' ')

#define ckidAVINEWINDEX         mmioFOURCC('i', 'd', 'x', '1')

/*
** Here are some stream types.  Currently, only audio and video
** are supported.
*/
#define streamtypeVIDEO         mmioFOURCC('v', 'i', 'd', 's')
#define streamtypeAUDIO         mmioFOURCC('a', 'u', 'd', 's')
#define streamtypeMIDI          mmioFOURCC('m', 'i', 'd', 's')
#define streamtypeTEXT          mmioFOURCC('t', 'x', 't', 's')

/*
** Here are some compression types.
*/
#define comptypeRLE0            mmioFOURCC('R','L','E','0')
#define comptypeRLE             mmioFOURCC('R','L','E',' ')
#define comptypeDIB             mmioFOURCC('D','I','B',' ')

#define cktypeDIBbits           aviTWOCC('d', 'b')
#define cktypeDIBcompressed     aviTWOCC('d', 'c')
#define cktypeDIBhalf           aviTWOCC('d', 'x')
#define cktypePALchange         aviTWOCC('p', 'c')
#define cktypeWAVEbytes         aviTWOCC('w', 'b')
#define cktypeWAVEsilence       aviTWOCC('w', 's')

#define cktypeMIDIdata          aviTWOCC('m', 'd')

#define cktypeDIBheader         aviTWOCC('d', 'h')
#define cktypeWAVEformat        aviTWOCC('w', 'f')

#define ckidAVIPADDING          mmioFOURCC('J', 'U', 'N', 'K')
#define ckidOLDPADDING          mmioFOURCC('p', 'a', 'd', 'd')


/*
** Useful macros
*/
#define ToHex(n)	((BYTE) (((n) > 9) ? ((n) - 10 + 'A') : ((n) + '0')))
#define FromHex(n)	(((n) >= 'A') ? ((n) + 10 - 'A') : ((n) - '0'))

/* Macro to get stream number out of a FOURCC ckid */
#define StreamFromFOURCC(fcc) ((WORD) ((FromHex(LOBYTE(LOWORD(fcc))) << 4) + \
                                             (FromHex(HIBYTE(LOWORD(fcc))))))

/* Macro to get TWOCC chunk type out of a FOURCC ckid */
#define TWOCCFromFOURCC(fcc)    HIWORD(fcc)

/* Macro to make a ckid for a chunk out of a TWOCC and a stream number
** from 0-255.
**
** Warning: This is a nasty macro, and MS C 6.0 compiles it incorrectly
** if optimizations are on.  Ack.
*/
#define MAKEAVICKID(tcc, stream) \
        MAKELONG((ToHex((stream) & 0x0f) << 8) | ToHex(((stream) & 0xf0) >> 4), tcc)



/*
** Main AVI File Header 
*/	     
		     
/* flags for use in <dwFlags> in AVIFileHdr */
#define AVIF_HASINDEX		0x00000010	// Index at end of file?
#define AVIF_MUSTUSEINDEX	0x00000020
#define AVIF_ISINTERLEAVED	0x00000100
#define AVIF_VARIABLESIZEREC	0x00000200
#define AVIF_NOPADDING		0x00000400
#define AVIF_WASCAPTUREFILE	0x00010000
#define AVIF_COPYRIGHTED	0x00020000

/* The AVI File Header LIST chunk should be padded to this size */
#define AVI_HEADERSIZE  2048                    // size of AVI header list

/*****************************************************************************
 * @doc EXTERNAL AVI_FFMT
 * 
 * @types MainAVIHeader | The <t MainAVIHeader> structure contains 
 *	global information for the entire AVI file.  It is contained 
 *	within an 'avih' chunk within the LIST 'hdrl' chunk at the
 *	beginning of an AVI RIFF file.
 * 
 * @field DWORD | dwMicroSecPerFrame | Specifies the number of 
 *    microseconds between frames.
 *
 * @field DWORD | dwMaxBytesPerSec | Specifies the approximate 
 *    maximum data rate of file.
 *
 * @field DWORD | dwReserved1 | Reserved. (This field should be set to 0.)
 *
 * @field DWORD | dwFlags | Specifies any applicable flags. 
 *    The following flags are defined: 
 *
 *	@flag AVIF_HASINDEX | Indicates
 *		the AVI file has an 'idx1' chunk containing an index
 *		at the end of the file.  For good performance, all AVI 
 *		files should contain an index.
 *
 *	@flag AVIF_MUSTUSEINDEX | Indicates that the
 *		index, rather than the physical ordering of the chunks
 *		in the file, should be used to determine the order of
 *		presentation of the data.  For example, this could be
 *		used for creating a list frames for editing.
 *		
 *	@flag AVIF_ISINTERLEAVED | Indicates 
 *		the AVI file is interleaved.  
 *
 *	@flag AVIF_WASCAPTUREFILE | Indicates 
 *		the AVI file is a specially allocated file used for
 *		capturing real-time video.  Applications should warn the
 *		user before writing over a file with this flag set 
 *		because the user probably defragmented
 *		this file.
 *
 *	@flag AVIF_COPYRIGHTED | Indicates the
 *		AVI file contains copyrighted data and software.
 *    When this flag is used, 
 *    software should not permit the data to be duplicated. 
 *
 * @field DWORD | dwTotalFrames | Specifies the number of 
 *    frames of data in file.
 *
 * @field DWORD | dwInitialFrames | Specifies the initial frame 
 * for interleaved files. Non-interleaved files should specify 
 *	zero.
 *
 * @field DWORD | dwStreams | Specifies the number of streams in the file.
 *	   For example, a file with audio and video has 2 streams.
 *
 * @field DWORD | dwSuggestedBufferSize | Specifies the suggested 
 *    buffer size for reading the file.  Generally, this size 
 *    should be large enough to contain the largest chunk in 
 *    the file. If set to zero, or if it is too small, the playback
 *	   software will have to reallocate memory during playback 
 *	   which will reduce performance.
 *    
 *	   For an interleaved file, this buffer size should be large
 *	   enough to read an entire record and not just a chunk.
 *
 * @field DWORD | dwWidth | Specifies the width of the AVI file in pixels.
 *
 * @field DWORD | dwHeight | Specifies the height of the AVI file in pixels.
 *
 * @field DWORD | dwScale | This field is used with
 *	<e MainAVIHeader.dwRate> to specify the time scale that
 *	applies to the AVI file. In addition, each stream 
 * can have its own time scale.
 *
 *	Dividing <e MainAVIHeader.dwRate> by <e AVIStreamHeader.dwScale>
 *	gives the number of samples per second.
 *
 * @field DWORD | dwRate | See <e MainAVIHeader.dwScale>.
 *
 * @field DWORD | dwStart | Specifies the starting time of the AVI file.
 * The units are defined by <e MainAVIHeader.dwRate> and 
 * <e MainAVIHeader.dwScale>. This field is usually set to zero.
 *
 * @field DWORD | dwLength | Specifies the length of the AVI file. 
 * The units are defined by <e AVIStreamHeader.dwRate> and 
 * <e AVIStreamHeader.dwScale>. This length is returned by MCIAVI when 
 * using the frames time format.
 *
 ****************************************************************************/

typedef struct 
{
    DWORD		dwMicroSecPerFrame;	// frame display rate (or 0L)
    DWORD		dwMaxBytesPerSec;	// max. transfer rate
    DWORD		dwPaddingGranularity;	// pad to multiples of this
                                                // size; normally 2K.
    DWORD		dwFlags;		// the ever-present flags
    DWORD		dwTotalFrames;		// # frames in file
    DWORD		dwInitialFrames;
    DWORD		dwStreams;
    DWORD		dwSuggestedBufferSize;
    
    DWORD		dwWidth;
    DWORD		dwHeight;
    
    /* Do we want the stuff below for the whole movie, or just
    ** for the individual streams?
    */
    DWORD		dwScale;	
    DWORD		dwRate;	/* dwRate / dwScale == samples/second */
    DWORD		dwStart;  /* Is this always zero? */
    DWORD		dwLength; /* In units above... */
} MainAVIHeader;


/*
** Stream header
*/

/* !!! Do we need to distinguish between discrete and continuous streams? */

#define AVISF_DISABLED			0x00000001
#define AVISF_VIDEO_PALCHANGES		0x00010000
/* Do we need identity palette support? */

/*****************************************************************************
 * @doc EXTERNAL AVI_FFMT
 * 
 * @types AVIStreamHeader | The <t AVIStreamHeader> structure contains 
 *	   header information for a single stream of an file. It is contained 
 *    within an 'strh' chunk within a LIST 'strl' chunk that is itself
 *	   contained within the LIST 'hdrl' chunk at the beginning of
 *    an AVI RIFF file.
 * 
 * @field FOURCC | fccType | Contains a four-character code which specifies
 *	   the type of data contained in the stream. The following values are 
 *	   currently defined:
 *
 *	@flag 'vids' | Indicates the stream contains video data.  The stream 
 *    format chunk contains a <t BITMAPINFO> structure which can include
 *		palette information.
 *
 *	@flag 'auds' | Indicates the stream contains video data.  The stream 
 *    format chunk contains a <t WAVEFORMATEX> or <t PCMWAVEFORMAT>
 *		structure.
 *
 *    New data types should be registered with the <MI>Multimedia Developer 
 *    Registration Kit<D>.
 *
 * @field FOURCC | fccHandler | Contains a four-character code that 
 *	   identifies a specific data handler.
 *
 * @field DWORD | dwFlags | Specifies any applicable flags. 
 *    The bits in the high-order word of these flags 
 *    are specific to the type of data contained in the stream.
 *    The following flags are currently defined:
 *
 *	@flag AVISF_DISABLED | Indicates 
 *		this stream should not be enabled by default.
 *
 *	@flag AVISF_VIDEO_PALCHANGES | Indicates 
 *		this video stream contains palette changes. This flag warns
 *		the playback software that it will need to animate the 
 *		palette.
 *
 * @field DWORD | dwReserved1 | Reserved. (Should be set to 0.)
 *
 * @field DWORD | dwInitialFrames | Reserved for interleaved files. 
 *	   (Set this to 0 for non-interleaved files.)
 *
 * @field DWORD | dwScale | This field is used together with
 *	<e AVIStreamHeader.dwRate> to specify the time scale that
 *	this stream will use.
 *
 *	Dividing <e AVIStreamHeader.dwRate> by <e AVIStreamHeader.dwScale>
 *	gives the number of samples per second.
 *
 *	For video streams, this rate should be the frame rate.
 *
 *	For audio streams, this rate should correspond to the time needed for
 *	<e WAVEFORMATEX.nBlockAlign> bytes of audio, which for PCM audio simply
 *	reduces to the sample rate.
 *
 * @field DWORD | dwRate | See <e AVIStreamHeader.dwScale>.
 *
 * @field DWORD | dwStart | Specifies the starting time of the AVI file.
 * The units are defined by the 
 *	<e MainAVIHeader.dwRate> and <e MainAVIHeader.dwScale> fields
 *	in the main file header. Normally, this is zero, but it can
 *	specify a delay time for a stream which does not start concurrently 
 *	with the file.
 *
 *	Note: The 1.0 release of the AVI tools does not support a non-zero
 *	starting time.
 *
 * @field DWORD | dwLength | Specifies the length of this stream. 
 * The units are defined by the 
 *	<e AVIStreamHeader.dwRate> and <e AVIStreamHeader.dwScale>
 *	fields of the stream's header. 
 *
 * @field DWORD | dwSuggestedBufferSize | Suggests how large a buffer
 *	should be used to read this stream.  Typically, this contains a
 *	value corresponding to the largest chunk present in the stream. 
 * Using the correct buffer size makes playback more efficient.
 * Use zero if you do not know the correct buffer size. 
 *
 * @field DWORD | dwQuality | Specifies an indicator of the quality 
 * of the data in the stream. Quality is 
 *	represented as a number between 0 and 10000.  For compressed data,
 *	this typically represent the value of the quality parameter
 *	passed to the compression software.
 *
 * @field DWORD | dwSampleSize | Specifies the size of a single sample 
 * of data. This is set to 
 *	zero if the samples can vary in size.  If this number is non-zero, then
 *	multiple samples of data can be grouped into a single chunk within
 *	the file.  If it is zero, each sample of data (such as a video
 *	frame) must be in a separate chunk.
 *
 *	For video streams, this number is typically zero, although it
 *	can be non-zero if all video frames are the same size.
 *
 *	For audio streams, this number should be the same as the
 *	<e WAVEFORMATEX.nBlockAlign> field of the <t WAVEFORMATEX> structure
 *	describing the audio.
 *
 ****************************************************************************/
typedef struct {
    FOURCC		fccType;
    FOURCC		fccHandler;
    DWORD		dwFlags;	/* Contains AVITF_* flags */
    WORD		wPriority;
    WORD		wLanguage;
    DWORD		dwInitialFrames;
    DWORD		dwScale;	
    DWORD		dwRate;	/* dwRate / dwScale == samples/second */
    DWORD		dwStart;
    DWORD		dwLength; /* In units above... */

    // new....
    DWORD		dwSuggestedBufferSize;
    DWORD		dwQuality;
    DWORD		dwSampleSize;
    RECT		rcFrame;    /* does each frame need this? */

    /* additional type-specific data goes in StreamInfo chunk */
    
    /* For video: position within rectangle... */
    /* For audio: volume?  stereo channel? */
} AVIStreamHeader;

typedef struct {
    RECT    rcFrame;
} AVIVideoStreamInfo;

typedef struct {
    WORD    wLeftVolume;    // !!! Range?
    WORD    wRightVolume;
    DWORD   dwLanguage;	    // !!! Is there a standard representation of this?
} AVIAudioStreamInfo;


#define AVIIF_LIST          0x00000001L // chunk is a 'LIST'
#define AVIIF_TWOCC         0x00000002L // ckid is a TWOCC?
#define AVIIF_KEYFRAME      0x00000010L // this frame is a key frame.
#define AVIIF_FIRSTPART     0x00000020L // this frame is the start of a partial frame.
#define AVIIF_LASTPART      0x00000040L // this frame is the end of a partial frame.
#define AVIIF_MIDPART       (AVIIF_LASTPART|AVIIF_FIRSTPART)
#define AVIIF_NOTIME	    0x00000100L // this frame doesn't take any time

#define AVIIF_COMPUSE       0x0FFF0000L // these bits are for compressor use

/*****************************************************************************
 * @doc EXTERNAL AVI_FFMT
 * 
 * @types AVIINDEXENTRY | The AVI file index consists of an array
 *	of <t AVIINDEXENTRY> structures contained within an 'idx1'
 *	chunk at the end of an AVI file. This chunk follows the main LIST 'movi'
 *	chunk which contains the actual data.
 * 
 * @field DWORD | ckid | Specifies a four-character code corresponding 
 *    to the chunk ID of a data chunk in the file.
 *
 * @field DWORD | dwFlags | Specifies any applicable flags. 
 *    The flags in the low-order word are reserved for AVI, 
 *    while those in the high-order word can be used
 *    for stream- and compressor/decompressor-specific information.
 *    
 *	The following values are currently defined:
 *
 *	@flag AVIIF_LIST | Indicates the specified
 *		chunk is a 'LIST' chunk, and the <e AVIINDEXENTRY.ckid>
 *		field contains the list type of the chunk.
 *
 *	@flag AVIIF_KEYFRAME | Indicates this chunk
 *		is a key frame. Key frames do not require
 *		additional preceding chunks to be properly decoded.
 *
 *	@flag AVIIF_NOTIME | Indicates this chunk should have no effect
 *		on timing or calculating time values based on the number of chunks.
 *		For example, palette change chunks in a video stream
 *		should have this flag set, so that they are not counted
 *		as taking up a frame's worth of time.
 *
 * @field DWORD | dwChunkOffset | Specifies the position in the file of the 
 *    specified chunk. The position value includes the eight byte RIFF header.
 *
 * @field DWORD | dwChunkLength | Specifies the length of the 
 *    specified chunk. The length value does not include the eight
 *    byte RIFF header.
 *
 ****************************************************************************/
typedef struct
{
    DWORD		ckid;
    DWORD		dwFlags;
    DWORD		dwChunkOffset;		// Position of chunk
    DWORD		dwChunkLength;		// Length of chunk
} AVIINDEXENTRY;


/*
** Palette change chunk
**
** Used in video streams.
*/
typedef struct 
{
    BYTE		bFirstEntry;	/* first entry to change */
    BYTE		bNumEntries;	/* # entries to change (0 if 256) */
    WORD		wFlags;		/* Mostly to preserve alignment... */
    PALETTEENTRY	peNew[];	/* New color specifications */
} AVIPALCHANGE;

/*****************************************************************************
 * @doc EXTERNAL AVI_FFMT
 * 
 * @types AVIPALCHANGE | The <t AVIPALCHANGE> structure is used in 
 *	video streams containing palettized data to indicate the
 *	palette should change for subsequent video data.
 * 
 * @field BYTE | bFirstEntry | Specifies the first palette entry to change.
 *
 * @field BYTE | bNumEntries | Specifies the number of entries to change.
 * 
 * @field WORD | wFlags | Reserved. (This should be set to 0.)
 * 
 * @field PALETTEENTRY | peNew | Specifies an array of new palette entries.
 *
 ****************************************************************************/

#ifndef RC_INVOKED
#pragma pack()          /* Revert to default packing */
#endif  /* RC_INVOKED */

#endif /* INC_AVIFFMT */
