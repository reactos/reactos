/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *
 *
 *  Compddk.h - include file for implementing installable compressors
 *
 *  Copyright (c) 1990-1995, Microsoft Corp.  All rights reserved.
 *
 **********************************************************************
 *
 * To register FOURCC's for codec types please obtain a
 * copy of the Multimedia Developer Registration Kit from:
 *
 *  Microsoft Corporation
 *  Multimedia Systems Group
 *  Product Marketing
 *  One Microsoft Way
 *  Redmond, WA 98052-6399
 *
 *
*/

#ifndef _INC_COMPDDK
#define _INC_COMPDDK	50	/* version number */

#ifndef RC_INVOKED
#ifndef _WIN32
#pragma pack(1)         /* Assume byte packing throughout */
#endif
#endif  /* RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

// begin_vfw32

#define ICVERSION       0x0104

DECLARE_HANDLE(HIC);     /* Handle to a Installable Compressor */

//
// this code in biCompression means the DIB must be accesed via
// 48 bit pointers! using *ONLY* the selector given.
//
#define BI_1632  0x32333631     // '1632'

#ifndef mmioFOURCC
#define mmioFOURCC( ch0, ch1, ch2, ch3 )				\
		( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |	\
		( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )
#endif

#ifndef aviTWOCC
#define aviTWOCC(ch0, ch1) ((WORD)(BYTE)(ch0) | ((WORD)(BYTE)(ch1) << 8))
#endif

#ifndef ICTYPE_VIDEO
#define ICTYPE_VIDEO    mmioFOURCC('v', 'i', 'd', 'c')
#define ICTYPE_AUDIO    mmioFOURCC('a', 'u', 'd', 'c')
#endif

#ifndef ICERR_OK
#define ICERR_OK                0L
#define ICERR_DONTDRAW          1L
#define ICERR_NEWPALETTE        2L
#define ICERR_GOTOKEYFRAME	3L
#define ICERR_STOPDRAWING 	4L

#define ICERR_UNSUPPORTED      -1L
#define ICERR_BADFORMAT        -2L
#define ICERR_MEMORY           -3L
#define ICERR_INTERNAL         -4L
#define ICERR_BADFLAGS         -5L
#define ICERR_BADPARAM         -6L
#define ICERR_BADSIZE          -7L
#define ICERR_BADHANDLE        -8L
#define ICERR_CANTUPDATE       -9L
#define ICERR_ABORT	       -10L
#define ICERR_ERROR            -100L
#define ICERR_BADBITDEPTH      -200L
#define ICERR_BADIMAGESIZE     -201L

#define ICERR_CUSTOM           -400L    // errors less than ICERR_CUSTOM...
#endif

/* Values for dwFlags of ICOpen() */
#ifndef ICMODE_COMPRESS
#define ICMODE_COMPRESS		1
#define ICMODE_DECOMPRESS	2
#define ICMODE_FASTDECOMPRESS   3
#define ICMODE_QUERY            4
#define ICMODE_FASTCOMPRESS     5
#define ICMODE_DRAW             8
#endif
#ifndef _WIN32					// ;Internal
#define ICMODE_INTERNALF_FUNCTION32	0x8000	// ;Internal
#define ICMODE_INTERNALF_MASK		0x8000	// ;Internal
#endif						// ;Internal

/* Flags for AVI file index */
#define AVIIF_LIST	0x00000001L
#define AVIIF_TWOCC	0x00000002L
#define AVIIF_KEYFRAME	0x00000010L

/* quality flags */
#define ICQUALITY_LOW       0
#define ICQUALITY_HIGH      10000
#define ICQUALITY_DEFAULT   -1

/************************************************************************
************************************************************************/

#define ICM_USER          (DRV_USER+0x0000)

#define ICM_RESERVED      ICM_RESERVED_LOW
#define ICM_RESERVED_LOW  (DRV_USER+0x1000)
#define ICM_RESERVED_HIGH (DRV_USER+0x2000)

/************************************************************************

    messages.

************************************************************************/

#define ICM_GETSTATE                (ICM_RESERVED+0)    // Get compressor state
#define ICM_SETSTATE                (ICM_RESERVED+1)    // Set compressor state
#define ICM_GETINFO                 (ICM_RESERVED+2)    // Query info about the compressor

#define ICM_CONFIGURE               (ICM_RESERVED+10)   // show the configure dialog
#define ICM_ABOUT                   (ICM_RESERVED+11)   // show the about box

#define ICM_GETERRORTEXT            (ICM_RESERVED+12)   // get error text TBD ;Internal
#define ICM_GETFORMATNAME	    (ICM_RESERVED+20)	// get a name for a format ;Internal
#define ICM_ENUMFORMATS		    (ICM_RESERVED+21)	// cycle through formats ;Internal

#define ICM_GETDEFAULTQUALITY       (ICM_RESERVED+30)   // get the default value for quality
#define ICM_GETQUALITY              (ICM_RESERVED+31)   // get the current value for quality
#define ICM_SETQUALITY              (ICM_RESERVED+32)   // set the default value for quality

#define ICM_SET			    (ICM_RESERVED+40)	// Tell the driver something
#define ICM_GET			    (ICM_RESERVED+41)	// Ask the driver something

// Constants for ICM_SET:
#define ICM_FRAMERATE       mmioFOURCC('F','r','m','R')
#define ICM_KEYFRAMERATE    mmioFOURCC('K','e','y','R')

/************************************************************************

    ICM specific messages.

************************************************************************/

#define ICM_COMPRESS_GET_FORMAT     (ICM_USER+4)    // get compress format or size
#define ICM_COMPRESS_GET_SIZE       (ICM_USER+5)    // get output size
#define ICM_COMPRESS_QUERY          (ICM_USER+6)    // query support for compress
#define ICM_COMPRESS_BEGIN          (ICM_USER+7)    // begin a series of compress calls.
#define ICM_COMPRESS                (ICM_USER+8)    // compress a frame
#define ICM_COMPRESS_END            (ICM_USER+9)    // end of a series of compress calls.

#define ICM_DECOMPRESS_GET_FORMAT   (ICM_USER+10)   // get decompress format or size
#define ICM_DECOMPRESS_QUERY        (ICM_USER+11)   // query support for dempress
#define ICM_DECOMPRESS_BEGIN        (ICM_USER+12)   // start a series of decompress calls
#define ICM_DECOMPRESS              (ICM_USER+13)   // decompress a frame
#define ICM_DECOMPRESS_END          (ICM_USER+14)   // end a series of decompress calls
#define ICM_DECOMPRESS_SET_PALETTE  (ICM_USER+29)   // fill in the DIB color table
#define ICM_DECOMPRESS_GET_PALETTE  (ICM_USER+30)   // fill in the DIB color table

#define ICM_DRAW_QUERY              (ICM_USER+31)   // query support for dempress
#define ICM_DRAW_BEGIN              (ICM_USER+15)   // start a series of draw calls
#define ICM_DRAW_GET_PALETTE        (ICM_USER+16)   // get the palette needed for drawing
#define ICM_DRAW_UPDATE             (ICM_USER+17)   // update screen with current frame ;Internal
#define ICM_DRAW_START              (ICM_USER+18)   // start decompress clock
#define ICM_DRAW_STOP               (ICM_USER+19)   // stop decompress clock
#define ICM_DRAW_BITS               (ICM_USER+20)   // decompress a frame to screen ;Internal
#define ICM_DRAW_END                (ICM_USER+21)   // end a series of draw calls
#define ICM_DRAW_GETTIME            (ICM_USER+32)   // get value of decompress clock
#define ICM_DRAW                    (ICM_USER+33)   // generalized "render" message
#define ICM_DRAW_WINDOW             (ICM_USER+34)   // drawing window has moved or hidden
#define ICM_DRAW_SETTIME            (ICM_USER+35)   // set correct value for decompress clock
#define ICM_DRAW_REALIZE            (ICM_USER+36)   // realize palette for drawing
#define ICM_DRAW_FLUSH	            (ICM_USER+37)   // clear out buffered frames
#define ICM_DRAW_RENDERBUFFER       (ICM_USER+38)   // draw undrawn things in queue

#define ICM_DRAW_START_PLAY         (ICM_USER+39)   // start of a play
#define ICM_DRAW_STOP_PLAY          (ICM_USER+40)   // end of a play

#define ICM_DRAW_SUGGESTFORMAT      (ICM_USER+50)   // Like ICGetDisplayFormat
#define ICM_DRAW_CHANGEPALETTE      (ICM_USER+51)   // for animating palette

#define ICM_DRAW_IDLE               (ICM_USER+52)   // send each frame time ;Internal

#define ICM_GETBUFFERSWANTED        (ICM_USER+41)   // ask about prebuffering

#define ICM_GETDEFAULTKEYFRAMERATE  (ICM_USER+42)   // get the default value for key frames


#define ICM_DECOMPRESSEX_BEGIN      (ICM_USER+60)   // start a series of decompress calls
#define ICM_DECOMPRESSEX_QUERY      (ICM_USER+61)   // start a series of decompress calls
#define ICM_DECOMPRESSEX            (ICM_USER+62)   // decompress a frame
#define ICM_DECOMPRESSEX_END        (ICM_USER+63)   // end a series of decompress calls

#define ICM_COMPRESS_FRAMES_INFO    (ICM_USER+70)   // tell about compress to come
#define ICM_COMPRESS_FRAMES         (ICM_USER+71)   // compress a bunch of frames ;Internal
#define ICM_SET_STATUS_PROC	        (ICM_USER+72)   // set status callback

/************************************************************************
************************************************************************/

typedef struct {
    DWORD               dwSize;         // sizeof(ICOPEN)
    DWORD               fccType;        // 'vidc'
    DWORD               fccHandler;     //
    DWORD               dwVersion;      // version of compman opening you
    DWORD               dwFlags;        // LOWORD is type specific
    LRESULT             dwError;        // error return.
    LPVOID              pV1Reserved;    // Reserved
    LPVOID              pV2Reserved;    // Reserved
    DWORD               dnDevNode;      // Devnode for PnP devices
} ICOPEN;

/************************************************************************
************************************************************************/

typedef struct {
    DWORD   dwSize;                 // sizeof(ICINFO)
    DWORD   fccType;                // compressor type     'vidc' 'audc'
    DWORD   fccHandler;             // compressor sub-type 'rle ' 'jpeg' 'pcm '
    DWORD   dwFlags;                // flags LOWORD is type specific
    DWORD   dwVersion;              // version of the driver
    DWORD   dwVersionICM;           // version of the ICM used
// end_vfw32
#ifdef _WIN32
// begin_vfw32
    //
    // under Win32, the driver always returns UNICODE strings.
    //
    WCHAR   szName[16];             // short name
    WCHAR   szDescription[128];     // long name
    WCHAR   szDriver[128];          // driver that contains compressor
// end_vfw32
#else
    char    szName[16];             // short name
    char    szDescription[128];     // long name
    char    szDriver[128];          // driver that contains compressor
#endif
// begin_vfw32
}   ICINFO;

/* Flags for the <dwFlags> field of the <ICINFO> structure. */
#define VIDCF_QUALITY        0x0001  // supports quality
#define VIDCF_CRUNCH         0x0002  // supports crunching to a frame size
#define VIDCF_TEMPORAL       0x0004  // supports inter-frame compress
#define VIDCF_COMPRESSFRAMES 0x0008  // wants the compress all frames message
#define VIDCF_DRAW           0x0010  // supports drawing
#define VIDCF_FASTTEMPORALC  0x0020  // does not need prev frame on compress
#define VIDCF_FASTTEMPORALD  0x0080  // does not need prev frame on decompress
//#define VIDCF_QUALITYTIME    0x0040  // supports temporal quality

//#define VIDCF_FASTTEMPORAL   (VIDCF_FASTTEMPORALC|VIDCF_FASTTEMPORALD)

/************************************************************************
************************************************************************/

#define ICCOMPRESS_KEYFRAME	0x00000001L

typedef struct {
    DWORD               dwFlags;        // flags

    LPBITMAPINFOHEADER  lpbiOutput;     // output format
    LPVOID              lpOutput;       // output data

    LPBITMAPINFOHEADER  lpbiInput;      // format of frame to compress
    LPVOID              lpInput;        // frame data to compress

    LPDWORD             lpckid;         // ckid for data in AVI file
    LPDWORD             lpdwFlags;      // flags in the AVI index.
    LONG                lFrameNum;      // frame number of seq.
    DWORD               dwFrameSize;    // reqested size in bytes. (if non zero)

    DWORD               dwQuality;      // quality

    // these are new fields
    LPBITMAPINFOHEADER  lpbiPrev;       // format of previous frame
    LPVOID              lpPrev;         // previous frame

} ICCOMPRESS;

/************************************************************************
************************************************************************/

#define ICCOMPRESSFRAMES_PADDING	0x00000001

typedef struct {
    DWORD               dwFlags;        // flags

    LPBITMAPINFOHEADER  lpbiOutput;     // output format
    LPARAM              lOutput;        // output identifier

    LPBITMAPINFOHEADER  lpbiInput;      // format of frame to compress
    LPARAM              lInput;         // input identifier

    LONG                lStartFrame;    // start frame
    LONG                lFrameCount;    // # of frames

    LONG                lQuality;       // quality
    LONG                lDataRate;      // data rate
    LONG                lKeyRate;       // key frame rate

    DWORD		dwRate;		// frame rate, as always
    DWORD		dwScale;

    DWORD		dwOverheadPerFrame;
    DWORD		dwReserved2;

    LONG (CALLBACK *GetData)(LPARAM lInput, LONG lFrame, LPVOID lpBits, LONG len);
    LONG (CALLBACK *PutData)(LPARAM lOutput, LONG lFrame, LPVOID lpBits, LONG len);
} ICCOMPRESSFRAMES;

typedef struct {
    DWORD		dwFlags;
    LPARAM		lParam;

    // messages for Status callback
    #define ICSTATUS_START	    0
    #define ICSTATUS_STATUS	    1	    // l == % done
    #define ICSTATUS_END	    2
    #define ICSTATUS_ERROR	    3	    // l == error string (LPSTR)
    #define ICSTATUS_YIELD	    4
    // return nonzero means abort operation in progress

    LONG (CALLBACK *Status) (LPARAM lParam, UINT message, LONG l);
} ICSETSTATUSPROC;

/************************************************************************
************************************************************************/

#define ICDECOMPRESS_HURRYUP      0x80000000L   // don't draw just buffer (hurry up!)
#define ICDECOMPRESS_UPDATE       0x40000000L   // don't draw just update screen
#define ICDECOMPRESS_PREROLL      0x20000000L   // this frame is before real start
#define ICDECOMPRESS_NULLFRAME    0x10000000L   // repeat last frame
#define ICDECOMPRESS_NOTKEYFRAME  0x08000000L   // this frame is not a key frame

typedef struct {
    DWORD               dwFlags;    // flags (from AVI index...)

    LPBITMAPINFOHEADER  lpbiInput;  // BITMAPINFO of compressed data
                                    // biSizeImage has the chunk size
    LPVOID              lpInput;    // compressed data

    LPBITMAPINFOHEADER  lpbiOutput; // DIB to decompress to
    LPVOID              lpOutput;
    DWORD		ckid;	    // ckid from AVI file
} ICDECOMPRESS;

typedef struct {
    //
    // same as ICM_DECOMPRESS
    //
    DWORD               dwFlags;

    LPBITMAPINFOHEADER  lpbiSrc;    // BITMAPINFO of compressed data
    LPVOID              lpSrc;      // compressed data

    LPBITMAPINFOHEADER  lpbiDst;    // DIB to decompress to
    LPVOID              lpDst;      // output data

    //
    // new for ICM_DECOMPRESSEX
    //
    int                 xDst;       // destination rectangle
    int                 yDst;
    int                 dxDst;
    int                 dyDst;

    int                 xSrc;       // source rectangle
    int                 ySrc;
    int                 dxSrc;
    int                 dySrc;

} ICDECOMPRESSEX;

/************************************************************************
************************************************************************/

#define ICDRAW_QUERY        0x00000001L   // test for support
#define ICDRAW_FULLSCREEN   0x00000002L   // draw to full screen
#define ICDRAW_HDC          0x00000004L   // draw to a HDC/HWND
#define ICDRAW_ANIMATE	    0x00000008L	  // expect palette animation
#define ICDRAW_CONTINUE	    0x00000010L	  // draw is a continuation of previous draw
#define ICDRAW_MEMORYDC	    0x00000020L	  // DC is offscreen, by the way
#define ICDRAW_UPDATING	    0x00000040L	  // We're updating, as opposed to playing
#define ICDRAW_RENDER       0x00000080L   // used to render data not draw it
#define ICDRAW_BUFFER       0x00000100L   // please buffer this data offscreen, we will need to update it

typedef struct {
    DWORD               dwFlags;        // flags

    HPALETTE            hpal;           // palette to draw with
    HWND                hwnd;           // window to draw to
    HDC                 hdc;            // HDC to draw to

    int                 xDst;           // destination rectangle
    int                 yDst;
    int                 dxDst;
    int                 dyDst;

    LPBITMAPINFOHEADER  lpbi;           // format of frame to draw

    int                 xSrc;           // source rectangle
    int                 ySrc;
    int                 dxSrc;
    int                 dySrc;

    DWORD               dwRate;         // frames/second = (dwRate/dwScale)
    DWORD               dwScale;

} ICDRAWBEGIN;

/************************************************************************
************************************************************************/

#define ICDRAW_HURRYUP      0x80000000L   // don't draw just buffer (hurry up!)
#define ICDRAW_UPDATE       0x40000000L   // don't draw just update screen
#define ICDRAW_PREROLL	    0x20000000L	  // this frame is before real start
#define ICDRAW_NULLFRAME    0x10000000L	  // repeat last frame
#define ICDRAW_NOTKEYFRAME  0x08000000L   // this frame is not a key frame

typedef struct {
    DWORD               dwFlags;        // flags
    LPVOID		lpFormat;       // format of frame to decompress
    LPVOID              lpData;         // frame data to decompress
    DWORD               cbData;
    LONG                lTime;          // time in drawbegin units (see dwRate and dwScale)
} ICDRAW;

typedef struct {
    LPBITMAPINFOHEADER	lpbiIn;		// format to be drawn
    LPBITMAPINFOHEADER	lpbiSuggest;	// location for suggested format (or NULL to get size)
    int			dxSrc;		// source extent or 0
    int			dySrc;
    int			dxDst;		// dest extent or 0
    int			dyDst;
    HIC			hicDecompressor;// decompressor you can talk to
} ICDRAWSUGGEST;

/************************************************************************
************************************************************************/

typedef struct {
    DWORD               dwFlags;    // flags (from AVI index...)
    int                 iStart;     // first palette to change
    int                 iLen;       // count of entries to change.
    LPPALETTEENTRY      lppe;       // palette
} ICPALETTE;

// end_vfw32

#ifndef RC_INVOKED
#ifndef _WIN32
#pragma pack()          /* Revert to default packing */
#endif
#endif  /* RC_INVOKED */

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif	/* __cplusplus */

#endif  /* _INC_COMPDDK */
