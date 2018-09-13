/****************************************************************************/
/*                                                                          */
/*        DISPDIB.H - Include file for DisplayDib() function.               */
/*                                                                          */
/*        Note: You must include WINDOWS.H before including this file.      */
/*                                                                          */
/*        Copyright (c) 1990-1994, Microsoft Corp.  All rights reserved.    */
/*                                                                          */
/****************************************************************************/


// DisplayDib() error return codes
#define DISPLAYDIB_NOERROR          0x0000  // success
#define DISPLAYDIB_NOTSUPPORTED     0x0001  // function not supported
#define DISPLAYDIB_INVALIDDIB       0x0002  // null or invalid DIB header
#define DISPLAYDIB_INVALIDFORMAT    0x0003  // invalid DIB format
#define DISPLAYDIB_INVALIDTASK      0x0004  // not called from current task
#define DISPLAYDIB_STOP             0x0005  // stop requested

// flags for <wFlags> parameter of DisplayDib()
#define DISPLAYDIB_NOPALETTE        0x0010  // don't set palette
#define DISPLAYDIB_NOCENTER         0x0020  // don't center image
#define DISPLAYDIB_NOWAIT           0x0040  // don't wait before returning
#define DISPLAYDIB_NOIMAGE          0x0080  // don't draw image
#define DISPLAYDIB_ZOOM2            0x0100  // stretch by 2
#define DISPLAYDIB_DONTLOCKTASK     0x0200  // don't lock current task
#define DISPLAYDIB_TEST             0x0400  // testing the command
#define DISPLAYDIB_NOFLIP           0x0800  // dont page flip
#define DISPLAYDIB_BEGIN            0x8000  // start of multiple calls
#define DISPLAYDIB_END              0x4000  // end of multiple calls

#define DISPLAYDIB_MODE             0x000F  // mask for display mode
#define DISPLAYDIB_MODE_DEFAULT     0x0000  // default display mode
#define DISPLAYDIB_MODE_320x200x8   0x0001  // 320-by-200
#define DISPLAYDIB_MODE_320x400x8   0x0002  // 320-by-400	/* ;Internal */
#define DISPLAYDIB_MODE_360x480x8   0x0003  // 360-by-480	/* ;Internal */
#define DISPLAYDIB_MODE_320x480x8   0x0004  // 320-by-480	/* ;Internal */
#define DISPLAYDIB_MODE_320x240x8   0x0005  // 320-by-240

#ifdef WIN32
// flags for the 32 bit version of DisplayDibEx
#define DISPLAYDIB_ANIMATE          0x00010000
#define DISPLAYDIB_HALFTONE         0x00020000
#endif


// function prototypes
UINT FAR PASCAL DisplayDib(LPBITMAPINFOHEADER lpbi, LPSTR lpBits, UINT wFlags);
UINT FAR PASCAL DisplayDibEx(LPBITMAPINFOHEADER lpbi, int x, int y, LPSTR lpBits, UINT wFlags);

#define DisplayDibBegin() DisplayDib(NULL, NULL, DISPLAYDIB_BEGIN)
#define DisplayDibEnd()   DisplayDib(NULL, NULL, DISPLAYDIB_END)
