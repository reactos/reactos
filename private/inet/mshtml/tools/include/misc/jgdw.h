/*----------------------------------------------------------------------------
;
; jgdw.h -- JGDW decoder API
;
; Copyright (c) 1994-1996 Johnson-Grace Company, all rights reserved
;
; This file contains all the defines necessary to interface the
; Johnson-Grace decompressor.
;
;---------------------------------------------------------------------------*/

#ifndef JGDW_H
#define JGDW_H 1

#ifdef _MAC
#include <Types.h>
#include <Palettes.h>
#include <QuickDraw.h>
#include <QDOffscreen.h>
#else
#pragma warning(disable:4201)
#include <windows.h>
#pragma warning(default:4201)
#endif

#include "jgtypes.h"

#ifdef __cplusplus
extern "C" {         /* Indicate C declarations for C++ */
#endif


#define JG_ARTIMAGE_INFO		JG_RAWIMAGE_INFO
#define JgGetImageHandleDIB		JgGetImage
#define JgGetMaskDIB			JgGetMask
#define NormalUpdateRect		OldRect

#ifdef _MAC

#define JGDW_CALL

typedef struct JGDW_RECORD_TAG JGDW_RECORD;
typedef JGDW_RECORD JGPTR	JGDW_HDEC;

typedef GWorldPtr			JGDW_HBITMAP;
typedef Rect				JGDW_RECT;
typedef RGBColor			JGDW_COLOR;
typedef ColorInfo			JGDW_PALETTEENTRY;

#else

#if defined(_WIN32) || defined(__FLAT__) /* 16/32 bit compatibility */
  #ifndef JGDW_DECLSPEC
    #define JGDW_DECLSPEC __declspec(dllimport)
  #endif
  #define JGDW_CALL JGDW_DECLSPEC WINAPI
#else
  #ifndef JGDW_DECLSPEC
    #define JGDW_DECLSPEC _export
  #endif
  #define JGDW_CALL JGDW_DECLSPEC WINAPI
#endif

typedef HGLOBAL				JGDW_HDEC;
typedef HGLOBAL				JGDW_HBITMAP;
typedef RECT				JGDW_RECT;
typedef PALETTEENTRY		JGDW_COLOR;
typedef PALETTEENTRY		JGDW_PALETTEENTRY;

#endif

typedef struct JGDW_CONTEXT JGDW_CONTEXT;
typedef JGDW_CONTEXT JGPTR JGDW_HCONTEXT;


#define JG_HEADER_SIZE      40  /* Min # of bytes for JgQueryArtImage() */

/*
** Note: the Decode library returns status codes as integers.
** Zero is returned for success.  A non-zero return means that some
** exception occurred.  Non-documented exceptions should be treated as
** non-recoverable errors.  In this case the application should print the
** full integer value to aid support after the product is in the field.
**
** The application can determine the class of the exception by masking
** off the high order bits of the word. See JgTypes.h for JG_ERR_xxx
** defines.
**
** Final Note: Currently all nonzero returns should be treated as
** non-recoverable errors.
*/

#define JGD_STATUS_SUCCESS      0
#define JGD_STATUS_MEMERR       (300 | JG_ERR_MEMORY)
#define JGD_STATUS_BADARG       (301 | JG_ERR_ARG)
#define JGD_STATUS_ERROR        (302 | JG_ERR_STATUS)
#define JGD_STATUS_NOPALETTE    (303 | JG_ERR_STATUS)
#define JGD_STATUS_BADDATA      (304 | JG_ERR_DATA)
#define JGD_STATUS_IERROR       (305 | JG_ERR_CHECK)
#define JGD_STATUS_TOOWIDE      (306 | JG_ERR_ARG)

                                /* Not Valid because not enough data */
#define JGD_STATUS_INVALID      (307 | JG_ERR_ARG)      

                                /* Unsupported version of compression */
#define JGD_STATUS_BADVERSION   (308 | JG_ERR_VERSION)   
                                /* Unsupported Sub Version of file */
#define JGD_STATUS_BADSUBVER    (309 | JG_ERR_VERSION)  
                                /* File segments in wrong order */
#define JGD_STATUS_BADORDER     (310 | JG_ERR_DATA)     
                                /* Segment is shorter than expected */
#define JGD_STATUS_SHORTSEG     (311 | JG_ERR_DATA)     
                                /* Input Buffer is shorter than necessary */
#define JGD_STATUS_SHORTBUF     (312 | JG_ERR_DATA)     
                                /* Decodeable with some degradation */ 
#define JGD_STATUS_OLDRESOURCE  (313 | JG_ERR_STATUS)   
                                /* Decodeable with extreme degradation */
#define JGD_STATUS_NORESOURCE   (314 | JG_ERR_STATUS)   
                                /* Not decodeable because missing resource */
#define JGD_STATUS_BADRESOURCE  (315 | JG_ERR_STATUS)   
                                /* Resource not found */
#define JGD_STATUS_NOTFOUND     (316 | JG_ERR_STATUS)   
                                /* The Resource data is corrupted */
#define JGD_STATUS_BADRCDATA    (317 | JG_ERR_DATA)     

#define JGD_STATUS_READY        (318 | JG_ERR_STATUS)
#define JGD_STATUS_WAITING      (319 | JG_ERR_STATUS)
#define JGD_STATUS_DONE         (320 | JG_ERR_STATUS)

                                /* CB Patterns Missing */
#define JGD_STATUS_NOPATTERNS   (321 | JG_ERR_STATUS)
                                /* Data is not in ART format */
#define JGD_STATUS_NOTART       (322 | JG_ERR_STATUS)
                                /* End of file found */
#define JGD_STATUS_EOF          (323 | JG_ERR_STATUS)
                                /* Result is too big */
#define JGD_STATUS_TOOBIG       (324 | JG_ERR_STATUS) 
				/* Invalid state for requested operation */
#define JGD_STATUS_BADSTATE     (325 | JG_ERR_STATE)
				/* Invalid or corrupted handle */
#define JGD_STATUS_BADHANDLE	(326 | JGD_STATUS_BADARG)
#define JGD_STATUS_LIB_NOT_FOUND (327 | JG_ERR_STATUS)
#define JGD_STATUS_UNSUPPORTED	(328 | JG_ERR_DATA)
#define JGD_STATUS_UNKNOWN		(329 | JG_ERR_DATA)
#define JGD_STATUS_OBSOLETE	(330 | JG_ERR_VERSION)
#define JGD_STATUS_BADGAMMA	(331 | JG_ERR_ARG) /* Bad gamma argument(s) */

/*
** These defines are used to select the various image decoding options.
*/

#define JG_OPTION_DITHER        0x0001  /* To Request dithering */
#define JG_OPTION_USEDEFAULT_PALETTE 0x0002  /* To force Default Palette */
#define JG_OPTION_FULLIMAGE     0x0008  /* To request full image */
#define JG_OPTION_BACKGROUNDCOLOR 0x10  /* To enable Background color */
#define JG_OPTION_INHIBIT_AUDIO   0x20  /* To disable audio, if any */
#define JG_OPTION_ONEPASS       0x0080  /* To request one pass decoding. */
#define JG_OPTION_MASK_BITMAP   0x0100  /* Create transparency mask 1=opaque */
#define JG_OPTION_IMAGEFORMAT	0x0200  /* Decode specific image format */
#define JG_OPTION_GAMMACORRECT	0x0400  /* Perform gamma correction */
#define JG_OPTION_COMMONDEVICE	0x0800  /* Mac only: use common GDevice */
#define JG_OPTION_TRANSINDEX	0x1000  /* Enables TransIndex field */


/*
** These defines indicate the type of palettizing actually done by
** the decoder.  They are returned in JgGetImageInfo().
*/

#define JG_PALETTE_MODE_OFF     0  /* No Palette, 24-bit */
#define JG_PALETTE_MODE_OPT     1  /* Use Optimal Palette, if Possible */
#define JG_PALETTE_MODE_DEFAULT 2  /* Use the input default palette */
#define JG_PALETTE_MODE_332     3  /* Use the standard 332 palette */

#define JG_POSTSCALE_LONGSIDE   0x0001  /* Do Post Scaling by long side */
#define JG_POSTSCALE_X          0x0002  /* Do Post Scaling in X */
#define JG_POSTSCALE_Y          0x0004  /* Do Post Scaling in Y */
#define JG_POSTSCALE_BESTFIT    0x0008  /* Do Post Scale with BestFit method */

// Use these defines for both GammaIn and GammaOut
#define JG_GAMMA_NONE			100  /* No gamma correction */
#define JG_GAMMA_MAC			180  /* Correction required for Mac = 1.8 */
#define JG_GAMMA_PC				250  /* Correction required for PC = 2.5 */

typedef struct {
    UINTW nSize;            /* Size of this structure, set be caller */
    UINTW ColorDepth;       /* Color depth to use (4, 8, or 24) */
    UINTW DecodeOptions;    /* Decoding Options */
    JGDW_PALETTEENTRY JGPTR DefaultPalette; /* Default Palette, or NULL if none */
    UINTW PaletteSize;      /* Size of Default Palette, if any */
    UINTW SplashDupFactor;  /* Replication Factor of Miniature in Full */
                           /*    image; 0=Off, >100=Full Size */
    JGBOOL bTrueSplash;      /* Save splash image until completely rdy */
    UINTW PostScaleFlags;   /* Defines Post Scale.  Or of JG_POSTSCALE_xxx */ 
    UINTW ScaledLongSide;   /* Used for Post scale, for JG_POSTSCALE_LONGSIDE */
    UINTW ScaledX;          /* Used for Post scale, for JG_POSTSCALE_X */
    UINTW ScaledY;          /* Used for Post scale, for JG_POSTSCALE_Y */
    JGDW_COLOR BackgroundColor; /* Used to specify background color */
    UINTW AudioOptions;     /* Defined elsewhere (in JGAW.H) */
	JGFOURCHAR ImageFormat;/* Specify image format or 'auto' for autodetect */
	UINTW GammaIn;          /* Default input gamma correction */
	UINTW GammaOut;         /* Desired output gamma correction */
    UINTW TransIndex;       /* Make this color transparent */
} JG_DECOMPRESS_INIT;

typedef struct {
    UINTW  nSize;                /* Size of this structure, set by caller */
    JGBOOL  bError;              /* Out: True if error detected which
                                    prevents further decoding */
    JGBOOL  bImageDone;          /* True if no further input required */
    JGBOOL  bNewNormalPixels;    /* True if Pixels ready in normal image */
    JGBOOL  Reserved1;
    JGDW_RECT  OldRect;          /* For compatibility */
    JGDW_RECT  UpdateRect;       /* Update region of image */
    UINTW  PaletteMode;          /* Type of palettizing being done */
    JGERR  iErrorReason;         /* Status code for error, if any */
    JGFOURCHAR ImageFormat;      /* Format of compressed image */
    UINTW	PaletteColors;	     /* Number of colors in palette */
    UINTW TransIndex;		     /* Index of transparent color (0xffff=none) */
} JG_DECOMPRESS_INFO;

typedef struct {
    UINTW  nSize;                /* Size of this structure, set by caller */
    UINTW  Version;              /* File's Version */
    UINTW  SubVersion;           /* File's SubVersion */
    JGBOOL  Decodeable;          /* Nonzero if file can be decoded */
                                 /* The following elements are only */
                                 /* Valid if the image is decodeable */
    UINTW  Rows;                 /* Actual Rows at compress time */
    UINTW  Cols;                 /* Actual Cols at compress time */
    JGBOOL  HasPalette;          /* If Nonzero, image contains a palette */
    JGBOOL  HasOverlays;         /* If Nonzero, image has enhancements */
    JGFOURCHAR ImageFormat;      /* Four-character image type code */
    UINTW  ColorDepth;           /* Native color depth of image */
} JG_RAWIMAGE_INFO;

/*
** The following are prototypes to Decoder functions.
*/

#ifdef __CFM68K__
 #pragma import on
#endif

//JGERR JGDW_CALL JgSetMemCallbacks(
//	void * (* JGFUNC memAlloc)(UINT32 size),	/* malloc */
//	void (* JGFUNC memFree)(void *ptr)			/* free */
//);

JGERR JGDW_CALL JgSetDecompressResourceBuffer(
    UINT8 JGPTR pBuffer,               /* The pointer to the resource buffer */
    UINT32 BufSize                 /* The size of the buffer */
);


/* The pointer returned by JgGetDecompressCaps points to an array of */
/* JG_READER_DESC structures.  A last dummy element exists with .nSize == 0 */
JGERR JGDW_CALL JgGetDecompressCaps(
	JG_READER_DESC JGPTR JGPTR FormatList /* Pointer to reader list. */
);


JGERR JGDW_CALL JgCreateDecompressContext(
    JGDW_HCONTEXT JGPTR hContext,        // OUT: context handle
    JG_DECOMPRESS_INIT JGPTR InitStruct  // IN: filled init structure
);

JGERR JGDW_CALL JgCreateDecompressor(
    JGDW_HDEC JGPTR hDec,                // OUT: decompression handle
    JGDW_HCONTEXT hContext,              // IN: context handle
    JG_DECOMPRESS_INIT JGPTR Init        // IN: null, or override of context
);

JGERR JGDW_CALL JgDestroyDecompressor(
    JGDW_HDEC hDec                       // IN: decompression handle
);

JGERR JGDW_CALL JgDestroyDecompressContext(
    JGDW_HCONTEXT hContext               // IN: context handle
);

JGERR JGDW_CALL JgInitDecompress(
    JGDW_HDEC JGPTR hJgImageOutput,        /* A pntr to recve the Img handle. */
    JG_DECOMPRESS_INIT JGPTR InitStruct  /* A filled init structure */
);

JGERR JGDW_CALL JgQueryArtImage(
    UINT8 JGHPTR pBuf,      /* First bytes of the compressed image */
    UINT32 nBufSize              /* Size of Buffer */
);

JGERR JGDW_CALL JgGetImageInfo(
    UINT8 JGHPTR pBuf,      /* First bytes of the compressed image */
    UINT32 nBufSize,             /* Number of bytes in buffer */
    JG_RAWIMAGE_INFO JGPTR Info
);

JGERR JGDW_CALL JgDecompressDone(
    JGDW_HDEC hJgImage              /* Handle to Decompress Struct */
);

JGERR JGDW_CALL JgGetImage(
    JGDW_HDEC hJgImage,             /* Handle to Decompression Structure */
    JGDW_HBITMAP JGPTR hBitmap      /* Output Handle to bitmap, if it exists */
);

JGERR JGDW_CALL JgDecompressImageBlock(
    JGDW_HDEC hJgImage,           /* Handle to Decompress Struct */
    UINT8 JGHPTR pImageBuf, /* Input buffer of compressed image data */
    UINT32 nBufSize,             /* Number of bytes of data in buffer */
    JGBOOL JGPTR bNewData          /* True if new data are available */
);

JGERR JGDW_CALL JgGetDecompressInfo(
    JGDW_HDEC hJgImage,             /* Handle to Decompress Struct */
    JG_DECOMPRESS_INFO JGPTR Info  /* Out: Filled Info struct */
);

JGERR JGDW_CALL JgGetDecoderVersion(
    char JGPTR Version
);

JGERR JGDW_CALL JgGetMiniatureOffset(
    UINT8 JGHPTR pBuf,      /* Entire compressed image */
    UINT32 nBufSize,             /* Size of compressed image */
    UINT32 JGPTR Offset              /* Output Offset to End of Miniature */
);

JGERR JGDW_CALL JgGetMask(
	JGDW_HDEC hJgImage,           /* Handle to Decompression Structure */
    JGDW_HBITMAP JGPTR hMask      /* Output Handle to Mask, if it exists */
);


JGERR JGDW_CALL JgSetDebug(JGDW_HDEC hJgVars,
                        UINTW DebugOptions);

/*
** New (8/95) Lossless decompression definitions.
*/

#ifndef JG_LOSSLESS_INFO_DEFINED // (also defined in jgew.h)
#define JG_LOSSLESS_INFO_DEFINED 1
typedef struct {
	UINT16 nSize;                /* Size of structure in bytes */
	INT16  SearchSize;           /* (Compression control) */
	UINT32 CompressedSize;       /* Total compressed block bytes */
	UINT32 CompressedSoFar;      /* Compressed processed so far */
	UINT32 CompressedLastCall;   /* Compressed processed last call */
	UINT32 DecompressedSize;     /* Total decompressed block bytes */
	UINT32 DecompressedSoFar;    /* Decompressed processed so far */
	UINT32 DecompressedLastCall; /* Decompressed processed last call */
} JG_LOSSLESS_INFO;
#endif

typedef void JGPTR JG_LOSSLESS_HDEC; /* lossless decompression handle type */

JGERR JGDW_CALL JgLosslessDecompressQuery(  /* Interrogate lossless stream */
    UINT8 JGHPTR InBuffer,   /* IN: Beginning of compressed stream */
    UINT32 InBufferSize,         /* IN: Bytes in InBuffer (0-n) */
    JG_LOSSLESS_INFO JGPTR LosslessInfo); /* OUT: Stream info returned here */

JGERR JGDW_CALL JgLosslessDecompressCreate( /* Create decompression handle */
    JG_LOSSLESS_HDEC JGPTR DecHandle);    /* IN: Pointer to new handle */

void JGDW_CALL JgLosslessDecompressDestroy( /* Destroy decompression handle */
    JG_LOSSLESS_HDEC DecHandle); /* IN: Handle from decompress create */

JGERR JGDW_CALL JgLosslessDecompressReset( /* Reset existing handle */
    JG_LOSSLESS_HDEC DecHandle); /* IN: Handle from decompress create */

JGERR JGDW_CALL JgLosslessDecompressBlock( /* decompress block of data */
    JG_LOSSLESS_HDEC DecHandle,  /* IN: Handle from decompress create */
    UINT8 JGHPTR InBuffer,    /* IN: Input (compressed) data */
    UINT32 InBufferSize,          /* IN: Bytes at *InBuffer (0-n) */
    UINT8 JGHPTR OutBuffer,   /* OUT: Output (decompressed result) buff */
    UINT32 OutBufferSize,         /* IN: Free bytes at *OutBuffer */
    JG_LOSSLESS_INFO JGPTR LosslessInfo); /* OUT: Updated info returned here */

JGERR JGDW_CALL JgLosslessDecompressPartitionReset( /* new partition reset */
    JG_LOSSLESS_HDEC DecHandle);  /* IN: Handle from decompress create */

#ifdef __CFM68K__
 #pragma import off
#endif

#ifdef __cplusplus
}
#endif

#endif
