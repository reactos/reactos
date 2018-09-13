/* aviffmt.h
 *
 * This header file describes the AVI File Format.
 *
 * Initial version: David Maymudes 1/7/91, based heavily on EricLe's avi0.h
 * Last updated: David Maymudes 12/5/91.
 *
 * Prerequisites: <windows.h>, <mmsystem.h>
 *
 * An AVI file is the following RIFF form:
 *
 *	RIFF('AVI'
 *	      LIST('hdrl')
 *		    hdra(<AVIFileHdr>)
 *		    dibh(<BITMAPINFO>)
 *		    [ wavf(<WAVEFORMAT>) ]
 *		    [ vidc(<COMPRESSIONINFO>) ]
 *		    [ audc(<COMPRESSIONINFO>) ]
 *		    [ JUNK(<padding>) ]
 *            LIST('movi'	
 *      	  { LIST('rec'
 *      		      SubChunk...
 *      		   )
 *      	      | SubChunk } ....	
 *            )
 *            [ <AVIIndex> ]
 *      )
 *
 *      SubChunk = { dibh(<AVI DIB header>)
 *      		| dibb(<AVI DIB bits>)
 *      		| dibc(<AVI compressed DIB bits>)
 *      		| palc(<AVI Palette Change>)
 *      		| wavb(<AVI WAVE bytes>)
 *      		| wavs(<AVI Silence record>)
 *      		| midi(<MIDI data>)
 *			| additonal custom chunks }
 *
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * We need a better description of the AVI file header here.
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
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
 *
 * Limitations for the Alpha release:
 *	If the AVI file has audio, each record LIST must contain exactly
 *	one audio chunk, which must be the first chunk.
 *	Each record must contain exactly one video chunk (possibly preceded
 *	by one or more palette change chunks).
 *	No wave format or DIB header chunks may occur outside of the header.
 */

#ifndef mmioFOURCC
#define mmioFOURCC( ch0, ch1, ch2, ch3 )				\
		( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |	\
		( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )
#endif

/* form types, list types, and chunk types */
#define formtypeAVI		mmioFOURCC('A', 'V', 'I', ' ')
#define listtypeAVIHEADER	mmioFOURCC('h', 'd', 'r', 'l')
#define listtypeAVIRECORD	mmioFOURCC('r', 'e', 'c', ' ')
#define listtypeAVIMOVIE	mmioFOURCC('m', 'o', 'v', 'i')

#define ckidAVIHDR		mmioFOURCC('h', 'd', 'r', 'a')

#define ckidAVIINDEX		mmioFOURCC('i', 'n', 'd', 'x')
#define ckidAVINEWINDEX		mmioFOURCC('i', 'd', 'x', '1')

#define ckidAVIAuthorInfo	mmioFOURCC('a', 'v', 'i', 'a')
#define ckidAVIVideoCompInfo	mmioFOURCC('v', 'i', 'd', 'c')
#define ckidAVIAudioCompInfo	mmioFOURCC('a', 'u', 'd', 'c')
#define ckidAVIAdditionalInfo	mmioFOURCC('i', 'n', 'f', 'o')

#define ckidDIBheader   	mmioFOURCC('d', 'i', 'b', 'h')
#define ckidPALchange 		mmioFOURCC('p', 'a', 'l', 'c')
#define ckidDIBbits		mmioFOURCC('d', 'i', 'b', 'b')
#define ckidDIBcompressed	mmioFOURCC('d', 'i', 'b', 'c')
#define ckidDIBhalfframe	mmioFOURCC('d', 'i', 'b', 'x')
#define ckidCCCbits		mmioFOURCC('C', 'C', 'C', 'b')
#define BI_CCC      0x20434343

#define ckidWAVEformat		mmioFOURCC('w', 'a', 'v', 'f')
#define ckidWAVEbytes 		mmioFOURCC('w', 'a', 'v', 'b')
#define ckidWAVEsilence 	mmioFOURCC('w', 'a', 'v', 's')

#define ckidMIDIdata		mmioFOURCC('m', 'i', 'd', 'i')

#define ckidAVIPADDING		mmioFOURCC('J', 'U', 'N', 'K')
#define ckidOLDPADDING		mmioFOURCC('p', 'a', 'd', 'd')

// #define comptypeCCC		mmioFOURCC('C','C','C',' ')
#define comptypeRLE0		mmioFOURCC('R','L','E','0')
#define comptypeRLE		mmioFOURCC('R','L','E',' ')
#define comptypeDIB		mmioFOURCC('D','I','B',' ')
#define comptypeNONE		mmioFOURCC('N','O','N','E')

#define comptypePCM		mmioFOURCC('P','C','M',' ')

#define OLDRLEF_MERGECOLORS	0x0010
#define OLDRLEF_SKIPSINGLE	0x0020
#define OLDRLEF_ADAPTIVE	0x0040


/* flags for use in <dwFlags> in AVIFileHdr */
#define AVIF_HASWAVE		0x00000001
#define AVIF_HASMIDI		0x00000002
#define AVIF_HASINDEX		0x00000010	// Index at end of file?
#define AVIF_ISINTERLEAVED	0x00000100
#define AVIF_VARIABLESIZEREC	0x00000200
#define AVIF_NOPADDING		0x00000400
#define AVIF_ONEPALETTE		0x00001000	// No palette changes?

/* The AVI File Header LIST chunk should be padded to this size */
#define AVI_HEADERSIZE	2048			// size of AVI header list


typedef struct _AVIFileHdr
{
    DWORD		dwMicroSecPerFrame;	// frame display rate (or 0L)
    DWORD		dwMaxBytesPerSec;	// max. transfer rate
    DWORD		dwPaddingGranularity;	// pad to multiples of this
					        // size; normally 2K.
    DWORD		dwFlags;		// the ever-present flags
    DWORD		dwTotalFrames;		// # frames in file
    DWORD		dwInitialVideoFrames;
    DWORD		dwNumAudioChunks;
    DWORD		dwAudioOffsetFrames;	// how many frames is audio
						// ahead of video in file?
} AVIFileHdr;


typedef struct _AVIWAVEheader
{
    PCMWAVEFORMAT	waveformat;		// or some other format
} AVIWAVEheader;

#ifndef WIN32
/*
 * this section not used and causes warnings on NT (zero-length arrays
 * are disallowed).
 */

// Note: no time information here: wave audio always starts at time 0,
// and continues without stopping.
typedef struct _AVIWAVEbytes
{
    BYTE		abBits[0];		// bits of audio
} AVIWAVEbytes;

typedef struct _AVIWAVEsilence
{
    DWORD		dwSamples;		// # samples with no sound
} AVIWAVEsilence;

/* Possibly, we shouldn't have a whole BITMAPINFOHEADER here. */
typedef struct _AVIDIBheader
{
    BITMAPINFOHEADER	bih;			// DIB header to use
    RGBQUAD		argbq[0];		// optional colors
} AVIDIBheader;


typedef struct _AVIDIBbits
{
    BYTE		abBits[0];		// bits of video
} AVIDIBbits;

typedef struct _AVIPALchange
{
    BYTE		bFirstEntry;	/* first entry to change */
    BYTE		bNumEntries;	/* # entries to change (0 if 256) */
    WORD		wFlags;		/* Mostly to preserve alignment... */
    PALETTEENTRY	peNew[];	/* New color specifications */
} AVIPALchange;

typedef struct _MIDIdata
{
    BYTE		abData[0];		// Raw MIDI data
} MIDIdata;
#endif //WIN32

#define AVIIF_LIST	0x00000001L
#define AVIIF_KEYFRAME	0x00000010L
typedef struct _AVIIndexEntry
{
    DWORD		ckid;
    DWORD		dwFlags;
    DWORD		dwChunkOffset;		// Position of chunk
    DWORD		dwChunkLength;		// Length of chunk
} AVIIndexEntry;

#ifndef WIN32
typedef struct tagCOMPRESSIONINFO
{
    DWORD		fccCompType;	/* Which compressor to use */
    BYTE		abData[];	/* Compressor-dependent data */
} COMPRESSIONINFO;

/* OBSOLETE BELOW THIS LINE */
/* flags for use in <dwAuthorFlags> in AVIAuthorInfo */
#define AVIF_PADTOMAX	0x0001			// pad records to maximum size
#define AVIF_IDENTITY	0x0002			// translate to identity palette

#define	AVIF_MERGECOLORS	0x0010		// use <wInterFrameThreshold>
#define AVIF_SKIPSINGLE		0x0020
#define	AVIF_FILTERFRAMES	0x0100		// use <wIntraFrameThreshold>
typedef struct _AVIAuthorInfo
{
    DWORD		dwAuthorFlags;		// flags provided at author time
    WORD		wInterFrameThreshold;
    WORD		wIntraFrameThreshold;
} AVIAuthorInfo;

#endif //WIN32
