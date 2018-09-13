/*******************************************************************
 *
 *	FILE:		dciman.h
 *	
 *	DESCRIPTION:	definitions for MS/Intel-defined DCI interface
 *
 *	Copyright (c) 1994 Intel/Microsoft Corporation
 *
 *******************************************************************/

#ifndef _INC_DCIDDI
#define _INC_DCIDDI

#ifdef __cplusplus
extern "C" {
#endif

/* DCI Command Escapes */                                                               
#define DCICOMMAND			3075
#define DCI_VERSION			0x0100

#define DCICREATEPRIMARYSURFACE		1 
#define DCICREATEOFFSCREENSURFACE       2 
#define DCICREATEOVERLAYSURFACE         3
#define DCIENUMSURFACE                  4 
#define DCIESCAPE                       5

/* DCI-Defined error codes */
#define DCI_OK                              	0 /* success */

/* Hard errors -- DCI will be unavailable */
#define DCI_FAIL_GENERIC                     -1
#define DCI_FAIL_UNSUPPORTEDVERSION          -2
#define DCI_FAIL_INVALIDSURFACE              -3
#define DCI_FAIL_UNSUPPORTED                 -4    

/* Soft errors -- DCI may be available later */
#define DCI_ERR_CURRENTLYNOTAVAIL           -5
#define DCI_ERR_INVALIDRECT                 -6
#define DCI_ERR_UNSUPPORTEDFORMAT           -7
#define DCI_ERR_UNSUPPORTEDMASK             -8
#define DCI_ERR_TOOBIGHEIGHT                -9
#define DCI_ERR_TOOBIGWIDTH                 -10
#define DCI_ERR_TOOBIGSIZE                  -11
#define DCI_ERR_OUTOFMEMORY                 -12
#define DCI_ERR_INVALIDPOSITION             -13
#define DCI_ERR_INVALIDSTRETCH              -14
#define DCI_ERR_INVALIDCLIPLIST             -15
#define DCI_ERR_SURFACEISOBSCURED           -16
#define DCI_ERR_XALIGN			    -18
#define DCI_ERR_YALIGN			    -19
#define DCI_ERR_XYALIGN			    -20
#define DCI_ERR_WIDTHALIGN		    -21
#define DCI_ERR_HEIGHTALIGN		    -22
											 
/* success messages -- DCI call succeeded, but specified item changed */
#define DCI_STATUS_POINTERCHANGED           1
#define DCI_STATUS_STRIDECHANGED            2
#define DCI_STATUS_FORMATCHANGED            4
#define DCI_STATUS_SURFACEINFOCHANGED       8
#define DCI_STATUS_CHROMAKEYCHANGED        16				
#define DCI_STATUS_WASSTILLDRAWING         32


/* DCI Capability Flags */
#define DCI_SURFACE_TYPE			0x0000000F
#define DCI_PRIMARY                 		0x00000000
#define DCI_OFFSCREEN               		0x00000001
#define DCI_OVERLAY                 		0x00000002

#define DCI_VISIBLE                 		0x00000010
#define DCI_CHROMAKEY               		0x00000020
#define DCI_1632_ACCESS             		0x00000040
#define DCI_DWORDSIZE               		0x00000080
#define DCI_DWORDALIGN              		0x00000100
#define DCI_WRITEONLY               		0x00000200
#define DCI_ASYNC                   		0x00000400

#define DCI_CAN_STRETCHX            		0x00001000
#define DCI_CAN_STRETCHY            		0x00002000
#define DCI_CAN_STRETCHXY           		(DCI_CAN_STRETCHX | DCI_CAN_STRETCHY)

#define DCI_CAN_STRETCHXN           		0x00004000
#define DCI_CAN_STRETCHYN           		0x00008000
#define DCI_CAN_STRETCHXYN          		(DCI_CAN_STRETCHXN | DCI_CAN_STRETCHYN)


#define DCI_CANOVERLAY                          0x00010000

/*
 * Win32 RGNDATA structure.  This will be used for  cliplist info. passing.
 */
#if (WINVER < 0x0400)

#ifndef RDH_RECTANGLES

typedef struct tagRECTL
{                      
   LONG     left;      
   LONG     top;       
   LONG     right;     
   LONG     bottom;    
                       
} RECTL;               
typedef RECTL*       PRECTL; 
typedef RECTL NEAR*  NPRECTL; 
typedef RECTL FAR*   LPRECTL;  
typedef const RECTL FAR* LPCRECTL;

#define RDH_RECTANGLES  0

typedef struct tagRGNDATAHEADER {
   DWORD   dwSize;                              /* size of structure             */
   DWORD   iType;                               /* Will be RDH_RECTANGLES        */
   DWORD   nCount;                              /* # of clipping rectangles      */
   DWORD   nRgnSize;                            /* size of buffer -- can be zero */
   RECTL   rcBound;                             /* bounding  rectangle for region*/
} RGNDATAHEADER;
typedef RGNDATAHEADER*       PRGNDATAHEADER;
typedef RGNDATAHEADER NEAR*  NPRGNDATAHEADER;
typedef RGNDATAHEADER FAR*   LPRGNDATAHEADER;
typedef const RGNDATAHEADER FAR* LPCRGNDATAHEADER;

typedef struct tagRGNDATA {
   RGNDATAHEADER   rdh;
   char            Buffer[1];
} RGNDATA;
typedef RGNDATA*       PRGNDATA;
typedef RGNDATA NEAR*  NPRGNDATA;
typedef RGNDATA FAR*   LPRGNDATA;
typedef const RGNDATA FAR* LPCRGNDATA;

#endif
#endif

typedef int     DCIRVAL;                /* return for callbacks */

/**************************************************************************
 *	input structures
 **************************************************************************/

/*
 * Used by a DCI client to provide input parameters for the 
 * DCICREATEPRIMARYSURFACE escape.
 */
typedef struct _DCICMD {
	DWORD	dwCommand;
	DWORD	dwParam1;
	DWORD 	dwParam2;
	DWORD	dwVersion;
	DWORD	dwReserved;
} DCICMD;

/*
 * This structure is used by a DCI client to provide input parameters for 
 * the DCICREATE... calls.  The fields that are actually relevant differ for 
 * each of the three calls.  Details are in the DCI Spec chapter providing 
 * the function specifications.
 */
typedef struct _DCICREATEINPUT {
	DCICMD	cmd;							/* common header structure */
	DWORD   dwCompression;          		/* format of surface to be created                      */
	DWORD   dwMask[3];                      /* for  nonstandard RGB (e.g. 5-6-5, RGB32) */
	DWORD   dwWidth;                        /* height of the surface to be created          */
	DWORD   dwHeight;                       /* width of input surfaces                                      */
	DWORD	dwDCICaps;						/* capabilities of surface wanted */
	LPVOID  lpSurface;                      /* pointer to an associated surface             */      
} DCICREATEINPUT, FAR *LPDCICREATEINPUT;
		

/*
 * This structure is used by a DCI client to provide input parameters for the 
 * DCIEnumSurface call.
 */
typedef struct _DCIENUMINPUT {
	DCICMD	cmd;							/* common header structure */
	RECT    rSrc;                           /* source rect. for stretch  */
	RECT    rDst;                           /* dest. rect. for stretch       */
	void    (CALLBACK *EnumCallback)(LPDCISURFACEINFO, LPVOID);        /* callback for supported formats */
	LPVOID  lpContext;
} DCIENUMINPUT, FAR *LPDCIENUMINPUT;

/**************************************************************************
 *	surface info. structures
 **************************************************************************/

/*
 * This structure is used to return information about available support
 * during a DCIEnumSurface call.  It is also used to create a primary 
 * surface, and as a member of the larger structures returned by the 
 * offscreen and overlay calls.
 */
 typedef struct _DCISURFACEINFO {
        DWORD   dwSize;                         /* size of structure                                            */
        DWORD   dwDCICaps;                  /* capability flags (stretch, etc.)             */
        DWORD   dwCompression;                  /* format of surface to be created                      */
        DWORD   dwMask[3];                  /* for BI_BITMASK surfaces                                      */

        DWORD   dwWidth;                    /* width of surface                                             */
        DWORD   dwHeight;                   /* height of surface                                            */
        LONG    lStride;                    /* distance in bytes betw. one pixel            */
                                                                                /* and the pixel directly below it                      */
        DWORD   dwBitCount;                 /* Bits per pixel for this dwCompression    */
        DWORD   dwOffSurface;               /* offset of surface pointer                            */
        WORD    wSelSurface;                /* selector of surface pointer                          */
        WORD    wReserved;

        DWORD   dwReserved1;                /* reserved for provider */
        DWORD   dwReserved2;                /* reserved for DCIMAN */
        DWORD   dwReserved3;                /* reserved for future */
        DCIRVAL (CALLBACK *BeginAccess) (LPVOID, LPRECT);    /* BeginAccess callback         */
        void (CALLBACK *EndAccess) (LPVOID);                   /* EndAcess callback            */
        void (CALLBACK *DestroySurface) (LPVOID);               /* Destroy surface callback     */
} DCISURFACEINFO, FAR *LPDCISURFACEINFO;

/*
 * This structure must be allocated and returned by the DCI provider in 
 * response to a DCICREATEPRIMARYSURFACE call.
 */
 typedef DCISURFACEINFO DCIPRIMARY, FAR *LPDCIPRIMARY;
								   
/*
 * This structure must be allocated and returned by the DCI provider in 
 * response to a DCICREATEOFFSCREENSURFACE call.
 */
 typedef struct _DCIOFFSCREEN {

	DCISURFACEINFO  dciInfo;                                                           /* surface info                  */
        DCIRVAL (CALLBACK *Draw) (LPVOID);                                            /* copy to onscreen buffer   */
        DCIRVAL (CALLBACK *SetClipList) (LPVOID, LPRGNDATA);          /* SetCliplist callback              */
        DCIRVAL (CALLBACK *SetDestination) (LPVOID, LPRECT, LPRECT);  /* SetDestination callback       */
} DCIOFFSCREEN, FAR *LPDCIOFFSCREEN;


/*
 * This structure must be allocated and returned by the DCI provider in response
 * to a DCICREATEOVERLAYSURFACE call.
 */
 typedef struct _DCIOVERLAY{

	DCISURFACEINFO  dciInfo;                                                /* surface info                  */
	DWORD   dwChromakeyValue;                                               /* chromakey color value                 */
	DWORD   dwChromakeyMask;                                                /* specifies valid bits of value */
} DCIOVERLAY, FAR *LPDCIOVERLAY;


/* DCI FOURCC def.s for extended DIB formats */                    

#ifndef YVU9
#define YVU9                        mmioFOURCC('Y','V','U','9')
#endif
#ifndef Y411
#define Y411                        mmioFOURCC('Y','4','1','1')                                             
#endif
#ifndef YUY2
#define YUY2                        mmioFOURCC('Y','U','Y','2')
#endif
#ifndef YVYU
#define YVYU                        mmioFOURCC('Y','V','Y','U')
#endif
#ifndef UYVY
#define UYVY                        mmioFOURCC('U','Y','V','Y')
#endif
#ifndef Y211
#define Y211                        mmioFOURCC('Y','2','1','1')
#endif

#ifdef __cplusplus
}
#endif

#endif // _INC_DCIDDI
