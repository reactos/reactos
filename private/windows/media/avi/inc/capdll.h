/* CAPDLL.h
 *
 * Capture DLL.
 *
 * (C) Copyright Microsoft Corporation 1991. All rights reserved.
 */

/* flat addressing  - Get a selector to the memory */
LPSTR FAR PASCAL CreatePhysicalSelector( DWORD dwBase, WORD wLimit );

/* Interrupt enable/disable */
WORD FAR PASCAL IRQEnable( void );
WORD FAR PASCAL IRQDisable( void );

/* check to see if card is there */
WORD FAR PASCAL videoInDetect( WORD wBase );

/* Targa card init/fini */
WORD FAR PASCAL TargaInit( WORD wBase, BYTE bIRQ, BYTE bAddress );
void FAR PASCAL TargaFini( void );

/* Assumes TargaInit has been called */
/* Return the value in the advanced reg */
WORD FAR PASCAL TargaAdvancedVal( void );

/* Set the border colour on the targa card */
void FAR PASCAL TargaBorder( WORD wColour );

/* Set the targa memory to the given colour */
void FAR PASCAL TargaFill( WORD wColour );

/* Set the Zoom bits of the MODE2 regester */
void FAR PASCAL TargaZoom( WORD wZoom );

#define ZOOM_1		0
#define ZOOM_2		1
#define ZOOM_4		2
#define ZOOM_8		3


/* Set the Display Mode bits of the MODE2 regester */
void FAR PASCAL TargaDispMode( WORD wDisp );

#define DISP_MEM_BORDER		0
#define DISP_LIVE_BORDER	1
#define DISP_MEM		2
#define DISP_LIVE		3


/* Set/Clear the Genlock bit of the MODE2 regester  */
void FAR PASCAL TargaGenlockBit( BOOL fSet );


/* Set/Clear the Capture bit of the MODE2 regester  */
void FAR PASCAL TargaCaptureBit( BOOL fSet );

/* Capture a frame from the targa card */
BOOL FAR PASCAL CaptureFrame( LPBITMAPINFO lpbi, LPBYTE lpBits );

/* Calculate the new translation table from the palette */
BOOL FAR PASCAL TransRecalcPal( HPALETTE hPal );
BOOL FAR PASCAL TransSet( LPBYTE );

/* Where is the input coming from? */
void FAR PASCAL CapRGB( void );
void FAR PASCAL CapSVideo( void );
void FAR PASCAL CapComp( void );

DWORD FAR PASCAL videoInError( void );



/* Memory list structure */
typedef struct _DIBNode {
    DWORD       dwBufferLength;         // length of data buffer
    DWORD       dwFlags;                // assorted flags (see defines)
    DWORD       reserved;               // reserved for driver
    struct _DIBNode FAR *	fpdnNext;
    struct _DIBNode FAR *	fpdnPrev;
    DWORD	ckid;
    DWORD	cksize;
    BYTE	abBits[0];
} DIBNode;

typedef DIBNode FAR * FPDIBNode;

#define VIDEOIN_PREPARED	1
#define VIDEOIN_DONE		2


/* Video routines for AVI capture */
WORD FAR PASCAL videoInOpen( DWORD dwTime );
WORD FAR PASCAL videoInClose( void );
WORD FAR PASCAL videoInAddBuffer( FPDIBNode fpdn );
WORD FAR PASCAL videoInUnprepareBuffer( FPDIBNode fpdn );
WORD FAR PASCAL videoInPrepareBuffer( FPDIBNode fpdn );
WORD FAR PASCAL videoInReset( void );
WORD FAR PASCAL videoInStart( void );
WORD FAR PASCAL videoInStop( void );
DWORD FAR PASCAL videoInGetPos( void );
