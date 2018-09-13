/*
	File:		PI_Basic.h

	Contains:	

	Written by:	U. J. Krabbenhoeft

	Version:

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

*/


#ifndef PI_BasicTypes_h
#define PI_BasicTypes_h

#define TRUE 1
#define FALSE 0
#define nil 0

#ifndef NULL
#define NULL 0
#endif

typedef double DREAL;			/* double Werte für Datenaustausch */
typedef float REAL;				/* Floating Werte für Datenaustausch */


typedef	unsigned char	UINT8;
typedef	unsigned short	UINT16;
typedef	unsigned int	UINT32;

typedef	signed char		INT8;
typedef	signed short	INT16;
typedef	signed int		INT32;

typedef	char	SINT8;
typedef	short	SINT16;
typedef	long	SINT32;

typedef	char	SInt8;
typedef	short	SInt16;
typedef	long	SInt32;

typedef	float			Float32;			/* IEEE 	32bits (04Byte), 1 for sign, 08 for exponent, 23 mantissa */
typedef	double			Float64;			/* IEEE 	64bits (08Byte), 1 for sign, 11 for exponent, 52 mantissa */
typedef	long double		Float80;			/* IEEE 	80bits (10Byte), 1 for sign, 15 for exponent, 64 mantissa */

typedef unsigned char Boolean;
typedef unsigned char Str255[256], Str63[64], Str32[33], Str31[32], Str27[28], Str15[16];
typedef char *Ptr;
typedef char **Handle;

struct Point {
    short							v;
    short							h;
};
typedef struct Point Point;

typedef Point *PointPtr;
struct Rect {
    short							top;
    short							left;
    short							bottom;
    short							right;
};
typedef struct Rect Rect;
typedef	UINT32	Fixed;
typedef UINT32 OSType;
typedef short OSErr;

typedef Rect *RectPtr;

/*
 *	Here ends the list of things that "belong" in Windows.
 */
struct RGBColor {
    unsigned short					red;						/*magnitude of red component*/
    unsigned short					green;						/*magnitude of green component*/
    unsigned short					blue;						/*magnitude of blue component*/
};
typedef struct RGBColor RGBColor, *RGBColorPtr, **RGBColorHdl;

struct ColorSpec {
    short							value;						/*index or other value*/
    RGBColor						rgb;						/*true color*/
};
typedef struct ColorSpec ColorSpec;

typedef ColorSpec *ColorSpecPtr;

typedef ColorSpec CSpecArray[1];

struct ColorTable {
    long							ctSeed;						/*unique identifier for table*/
    short							ctFlags;					/*high bit: 0 = PixMap; 1 = device*/
    short							ctSize;						/*number of entries in CTTable*/
    CSpecArray						ctTable;					/*array [0..0] of ColorSpec*/
};
typedef struct ColorTable ColorTable, *CTabPtr, **CTabHandle;

struct PixMap {
    Ptr								baseAddr;					/*pointer to pixels*/
    short							rowBytes;					/*offset to next line*/
    Rect							bounds;						/*encloses bitmap*/
    short							pmVersion;					/*pixMap version number*/
    short							packType;					/*defines packing format*/
    long							packSize;					/*length of pixel data*/
    Fixed							hRes;						/*horiz. resolution (ppi)*/
    Fixed							vRes;						/*vert. resolution (ppi)*/
    short							pixelType;					/*defines pixel type*/
    short							pixelSize;					/*# bits in pixel*/
    short							cmpCount;					/*# components in pixel*/
    short							cmpSize;					/*# bits per component*/
    long							planeBytes;					/*offset to next plane*/
    CTabHandle						pmTable;					/*color map for this pixMap*/
    long							pmReserved;					/*for future use. MUST BE 0*/
};
typedef struct PixMap PixMap, *PixMapPtr, **PixMapHandle;

struct ColorWorldInstanceRecord {
    long							data[4];
};
typedef struct ColorWorldInstanceRecord ColorWorldInstanceRecord;

typedef ColorWorldInstanceRecord *ColorWorldInstance;

struct BitMap {
    Ptr								baseAddr;
    short							rowBytes;
    Rect							bounds;
};
typedef struct BitMap BitMap;

typedef BitMap *BitMapPtr, **BitMapHandle;

struct Picture {
    short							picSize;
    Rect							picFrame;
};
typedef struct Picture Picture;

typedef Picture *PicPtr, **PicHandle;

struct DateTimeRec {
    short							year;
    short							month;
    short							day;
    short							hour;
    short							minute;
    short							second;
    short							dayOfWeek;
};
typedef struct DateTimeRec DateTimeRec;

enum {
    noErr						= 0,
    unimpErr					= -4,		/* unimplemented core routine, should NOT occure */
	notEnoughMemoryErr    		= 8L	    /* mem Error used in ProfileAccess.c only, from winerror.h		*/
};

#endif
