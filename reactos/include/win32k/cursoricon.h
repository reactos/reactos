
#ifndef __WIN32K_CURSORICON_H
#define __WIN32K_CURSORICON_H

#include <win32k/dc.h>
#include <win32k/gdiobj.h>

/* Structures for reading icon/cursor files and resources */
// Structures for reading icon files and resources 
typedef struct _ICONIMAGE
{
   BITMAPINFOHEADER   icHeader;      // DIB header
   RGBQUAD         icColors[1];   // Color table
   BYTE            icXOR[1];      // DIB bits for XOR mask
   BYTE            icAND[1];      // DIB bits for AND mask
} ICONIMAGE, *LPICONIMAGE __attribute__((packed));

typedef struct
{
    BYTE   bWidth;				// Width, in pixels, of the icon image
    BYTE   bHeight;				// Height, in pixels, of the icon image
    BYTE   bColorCount;			// Number of colors in image (0 if >=8bpp)
    BYTE   bReserved;			// Reserved ( must be 0)
} ICONDIR __attribute__((packed));

typedef struct
{
    WORD   wWidth;				//Width, in pixels of the cursor image
    WORD   wHeight;				//Hight, in pixles of the cursor image
} CURSORDIR __attribute__((packed));

typedef struct
{   union
    { ICONDIR icon;
      CURSORDIR  cursor;
    } Info;
    WORD   wPlanes;				// Number of Color Planes in the XOR image
    WORD   wBitCount;			// Bits per pixel in the XOR image
    DWORD  dwBytesInRes;		// How many bytes in this resource?
    DWORD  dwImageOffset;		// Where in the file is this image?
} CURSORICONDIRENTRY __attribute__((packed));

typedef struct
{
    WORD				idReserved;		// Reserved (must be 0)
    WORD				idType;			// Resource Type (1 for icons, 0 for cursors)
    WORD				idCount;		// How many images?
    CURSORICONDIRENTRY  idEntries[1] __attribute__((packed));   // An entry for idCount number of images
} CURSORICONDIR __attribute__((packed));

typedef struct
{  union
   { ICONDIR icon;
     CURSORDIR  cursor;
   } Info;
   WORD   wPlanes;              // Color Planes
   WORD   wBitCount;            // Bits per pixel
   DWORD  dwBytesInRes;         // how many bytes in this resource?
   WORD   nID;                  // the ID
} GRPICONDIRENTRY __attribute__((packed));

typedef struct 
{
   WORD            idReserved;   // Reserved (must be 0)
   WORD            idType;       // Resource type (1 for icons)
   WORD            idCount;      // How many images?
   GRPICONDIRENTRY   idEntries[1] __attribute__((packed)); // The entries for each image
} GRPICONDIR __attribute__((packed));

/* GDI logical Icon/Cursor object */
typedef struct _ICONCURSOROBJ
{
	BOOL		fIcon;
	DWORD		xHotspot;
	DWORD		yHotspot;
	BITMAP		ANDBitmap;
	BITMAP		XORBitmap;
} ICONCURSOROBJ, *PICONCURSOROBJ;

/*  Internal interfaces  */
#define  ICONCURSOROBJ_AllocIconCursor()  \
  ((HICON) GDIOBJ_AllocObj (sizeof (ICONCURSOROBJ), GO_ICONCURSOR_MAGIC))

#define  ICONCURSOROBJ_HandleToPtr(hICObj)  \
  ((PICONCURSOROBJ) GDIOBJ_LockObj ((HGDIOBJ) hICObj, GO_ICONCURSOR_MAGIC))
  
#define  ICONCURSOROBJ_ReleasePtr(hICObj) GDIOBJ_UnlockObj ((HGDIOBJ) hICObj, GO_ICONCURSOR_MAGIC)


BOOL IconCursor_InternalDelete( PICONCURSOROBJ pIconCursor );

/*  User Entry Points  */
HICON 
STDCALL 
W32kCreateIcon (
    BOOL fIcon,
	INT  Width,
	INT  Height,
	UINT  Planes,
	UINT  BitsPerPel,
	DWORD xHotspot,
	DWORD yHotspot,
	const VOID *ANDBits,
	const VOID *XORBits
	);



#endif
