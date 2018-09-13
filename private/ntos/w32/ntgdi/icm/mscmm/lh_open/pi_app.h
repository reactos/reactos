/*
	File:		PI_App.h

	Contains:	

	Written by:	U. J. Krabbenhoeft

	Version:

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

*/

#ifndef PI_Application_h
#define PI_Application_h

#ifndef PI_Machine_h
/*#include "PI_Machine.h"*/
#endif

#define CM_MAX_COLOR_CHANNELS 8
typedef unsigned long CMBitmapColorSpace;
typedef long CMError;
typedef unsigned int PI_OSType;
typedef struct CMPrivateProfileRecord *CMProfileRef;

#ifndef LHApplication_h
#include "App.h"
#endif

#ifdef MS_Icm
#include "Windef.h"
#include "WinGdi.h"
#include <wtypes.h>
#include "icm.h"
#endif

struct PI_NAMEDCOLOR {
    unsigned long   namedColorIndex;
    char			*pName;
};

struct PI_GRAYCOLOR {
    unsigned short    gray;
};

struct PI_RGBCOLOR {
    unsigned short    red;
    unsigned short    green;
    unsigned short    blue;
};

struct PI_CMYKCOLOR {
    unsigned short    cyan;
    unsigned short    magenta;
    unsigned short    yellow;
    unsigned short    black;
};

struct PI_XYZCOLOR {
    unsigned short    X;
    unsigned short    Y;
    unsigned short    Z;
};

struct PI_YxyCOLOR {
    unsigned short    Y;
    unsigned short    x;
    unsigned short    y;
};

struct PI_LabCOLOR {
    unsigned short    L;
    unsigned short    a;
    unsigned short    b;
};

struct PI_GENERIC3CHANNEL {
    unsigned short    ch1;
    unsigned short    ch2;
    unsigned short    ch3;
};

struct PI_HiFiCOLOR {
    unsigned char    channel[CM_MAX_COLOR_CHANNELS];
};


typedef union CMColor {
    struct PI_GRAYCOLOR        gray;
    struct PI_RGBCOLOR         rgb;
    struct PI_CMYKCOLOR        cmyk;
    struct PI_XYZCOLOR         XYZ;
    struct PI_YxyCOLOR         Yxy;
    struct PI_LabCOLOR         Lab;
    struct PI_GENERIC3CHANNEL  gen3ch;
    struct PI_NAMEDCOLOR       namedColor;
    struct PI_HiFiCOLOR        hifi;
} CMColor;

#ifdef MS_Icm
enum {
/* General Errors */
	cmopenErr               	= ERROR_INVALID_PARAMETER, /* I/O Error used in ProfileAccess.c only           */
	cmparamErr              	= ERROR_INVALID_PARAMETER,

	cmProfileError				= ERROR_INVALID_PROFILE,

	cmMethodError				= ERROR_INVALID_TRANSFORM, /* This is an internal error, no CalcFunction found */
	cmCantConcatenateError		= ERROR_INVALID_TRANSFORM, /* No concatenation possible                        */
														
	cmInvalidColorSpace			= ERROR_COLORSPACE_MISMATCH, /* no match between Profile colorspace bitmap type */

	cmInvalidSrcMap				= ERROR_INVALID_COLORSPACE,	/* Source bitmap color space is invalid	     */
    cmInvalidDstMap				= ERROR_INVALID_COLORSPACE,	/* Destination bitmap color space is invalid */

	cmNamedColorNotFound		= ERROR_INVALID_COLORINDEX,	/* index > count of named colors            */

	cmElementTagNotFound		= ERROR_TAG_NOT_FOUND,

    userCanceledErr				= ERROR_CANCELLED,        /* callback proc returned to cancel calculation	*/
	badProfileError      		= ERROR_INVALID_PROFILE,  /* header->magic != icMagicNumber used in ProfileAccess.c only  */
    memFullErr					= ERROR_NOT_ENOUGH_MEMORY
};
#else
enum cmErrorCodes{
/* General Errors */
	cmopenErr               	= -200,		/* I/O Error used in ProfileAccess.c only						*/
	cmparamErr              	= 86,

	cmProfileError				= 2301,
	cmMethodError				= -203,		/* This is an internal error, no CalcFunction found				*/
	cmCantConcatenateError		= -208,		/* No concatenation possible									*/
														
	cmInvalidColorSpace			= -209,		/* no match between Profile colorspace bitmap type				*/
	cmInvalidSrcMap				= -210,		/* Source bitmap color space is invalid							*/
	cmInvalidDstMap				= -211,		/* Destination bitmap color space is invalid					*/
															
	cmNamedColorNotFound		= -216,		/* index > count of named colors								*/

	cmElementTagNotFound		= 2302,

    userCanceledErr				= -128,		/* callback proc returned to cancel calculation					*/
	badProfileError      		= -228,		/* header->magic != icMagicNumber used in ProfileAccess.c only	*/
    memFullErr					= 8
};
#endif

#if RenderInt
#define CallCMBitmapCallBackProc(f,a,b,c) (!((*f)(a,b,c)))
#else
#define CallCMBitmapCallBackProc(f,a,b,c ) ((*f)((a)-(b),c))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=mac68k
#endif

#if PRAGMA_IMPORT_SUPPORTED
#pragma import on
#endif

typedef icHeader CMCoreProfileHeader;


/* Param for CWConcatColorWorld() */
struct CMConcatProfileSet {
	unsigned short 					keyIndex;					/* Zero-based */
	unsigned short 					count;						/* Min 1 */
	CMProfileRef 					profileSet[1];				/* Variable. Ordered from Source -> Dest */
};
typedef struct CMConcatProfileSet CMConcatProfileSet;

typedef icDateTimeNumber CMDateTime;
struct CMUniqueIdentification {
	icHeader 						profileHeader;
	CMDateTime 						calibrationDate;
	unsigned long 					ASCIIProfileDescriptionLen;
	char 							ASCIIProfileDescription[1];	/* variable length */
};
typedef struct CMUniqueIdentification CMUniqueIdentification;


enum {
	cmNoColorPacking			= 0x0000,
	cmAlphaSpace				= 0x0080,
	cmWord5ColorPacking			= 0x0500,
	cmWord565ColorPacking		= 0x0600,
	cmLong8ColorPacking			= 0x0800,
	cmLong10ColorPacking		= 0x0A00,
	cmAlphaFirstPacking			= 0x1000,
	cmOneBitDirectPacking		= 0x0B00,  /* for gamut check. highest bit first */
	cmAlphaLastPacking			= 0x0000,
	cm8PerChannelPacking		= 0x2000,
	cm10PerChannelPacking		= 0x0A00,
	cm16PerChannelPacking		= 0x4000,
	
	cm32_32ColorPacking			= 0x2700
};


enum {
	cmNoSpace					= 0,
	cmRGBSpace					= 1,
	cmCMYKSpace					= 2,
	cmHSVSpace					= 3,
	cmHLSSpace					= 4,
	cmYXYSpace					= 5,
	cmXYZSpace					= 6,
	cmLUVSpace					= 7,
	cmLABSpace					= 8,
	cmCMYSpace					= 9,
	cmGraySpace					= 10,
	cmReservedSpace2			= 11,
	cmGamutResultSpace			= 12,
	
	cmGenericSpace				= 13,			/*UWE: GenericDataFormat  */
	cmBGRSpace					= 14,			/*UWE: BGR  */
	cmYCCSpace					= 15,			/*UWE: YCC  */
	cmNamedIndexedSpace			= 16,			/* */

	cmMCFiveSpace				= 17,
	cmMCSixSpace				= 18,
	cmMCSevenSpace				= 19,
	cmMCEightSpace				= 20,
	
	cmKYMCSpace					= 29,
	cmRGBASpace					= cmRGBSpace + cmAlphaSpace,
	cmGrayASpace				= cmGraySpace + cmAlphaSpace,
	cmRGB16Space				= cmWord5ColorPacking + cmRGBSpace,
	cmRGB16_565Space			= cmWord565ColorPacking + cmRGBSpace,
	cmRGB32Space				= cmLong8ColorPacking + cmRGBSpace,
	cmARGB32Space				= cmLong8ColorPacking + cmAlphaFirstPacking + cmRGBASpace,
	cmCMYK32Space				= cmLong8ColorPacking + cmCMYKSpace,
	cmKYMC32Space				= cmLong8ColorPacking + cmKYMCSpace,
	cmHSV32Space				= cmLong10ColorPacking + cmHSVSpace,
	cmHLS32Space				= cmLong10ColorPacking + cmHLSSpace,
	cmYXY32Space				= cmLong10ColorPacking + cmYXYSpace,
	cmXYZ32Space				= cmLong10ColorPacking + cmXYZSpace,
	cmLUV32Space				= cmLong10ColorPacking + cmLUVSpace,
	cmLAB32Space				= cmLong10ColorPacking + cmLABSpace,
	cmGamutResult1Space			= cmOneBitDirectPacking + cmGamutResultSpace,
	cmRGB24Space				= cm8PerChannelPacking + cmRGBSpace,
	cmRGBA32Space				= cm8PerChannelPacking + cmAlphaLastPacking + cmRGBASpace,
	cmCMY24Space				= cm8PerChannelPacking + cmCMYSpace,
	cmLAB24Space				= cm8PerChannelPacking + cmLABSpace,
	
	cmGraySpace8Bit				= cmGraySpace + cm8PerChannelPacking,
	cmYCC24Space				= cm8PerChannelPacking + cmYCCSpace,
	cmYCC32Space				= cmLong8ColorPacking + cmYCCSpace,
	cmYCCASpace					= cmYCCSpace + cmAlphaSpace,
	cmYCCA32Space				= cm8PerChannelPacking + cmAlphaLastPacking + cmYCCASpace,
	cmAYCC32Space				= cmLong8ColorPacking + cmAlphaFirstPacking + cmYCCASpace,
	cmBGR24Space				= cm8PerChannelPacking + cmBGRSpace,
	cmBGR32Space				= cmLong8ColorPacking + cmBGRSpace + cmAlphaSpace,

	cmNamedIndexed24Space		= cmNamedIndexedSpace,
	cmNamedIndexed32Space		= cm32_32ColorPacking + cmNamedIndexedSpace,

	cmMCFive8Space				= cmMCFiveSpace + cm8PerChannelPacking,
	cmMCSix8Space				= cmMCSixSpace + cm8PerChannelPacking,
	cmMCSeven8Space				= cmMCSevenSpace + cm8PerChannelPacking,
	cmMCEight8Space				= cmMCEightSpace + cm8PerChannelPacking
#if ( CM_MAX_COLOR_CHANNELS == 16 )
	,
	cmMC9Space				= cmMCEight8Space + 1,
	cmMCaSpace				= cmMCEight8Space + 2,
	cmMCbSpace				= cmMCEight8Space + 3,
	cmMCcSpace				= cmMCEight8Space + 4,
	cmMCdSpace				= cmMCEight8Space + 5,
	cmMCeSpace				= cmMCEight8Space + 6,
	cmMCfSpace				= cmMCEight8Space + 7,
	cmMC2Space				= cmMCEight8Space + 8,
	cmMC98Space				= cmMC9Space + cm8PerChannelPacking,
	cmMCa8Space				= cmMCaSpace + cm8PerChannelPacking,
	cmMCb8Space				= cmMCbSpace + cm8PerChannelPacking,
	cmMCc8Space				= cmMCcSpace + cm8PerChannelPacking,
	cmMCd8Space				= cmMCdSpace + cm8PerChannelPacking,
	cmMCe8Space				= cmMCeSpace + cm8PerChannelPacking,
	cmMCf8Space				= cmMCfSpace + cm8PerChannelPacking,
	cmMC28Space				= cmMC2Space + cm8PerChannelPacking
#endif
};

struct CMBitmap {
	char *				image;			/*	pointer to image data						*/
	long 				width;			/*	count of pixel in one line					*/
	long 				height;			/*	count of lines								*/
	long 				rowBytes;		/*	offset in bytes from one line to next line	*/
	long 				pixelSize;		/*	not used									*/
	CMBitmapColorSpace	space;			/*	color space see above, e.g. cmRGB24Space	*/
	long 				user1;			/*	not used									*/
	long 				user2;			/*	not used									*/
};
typedef struct CMBitmap CMBitmap;

typedef char CMColorName[32];
typedef CMColorName *pCMColorName;
typedef const CMColorName *pcCMColorName;
typedef struct tagCMNamedProfileInfo{
	unsigned long	dwVendorFlags;
	unsigned long	dwCount;
	unsigned long   dwCountDevCoordinates;
	CMColorName		szPrefix;
	CMColorName		szSuffix;
}CMNamedProfileInfo;
typedef CMNamedProfileInfo *pCMNamedProfileInfo;

/* rendering intent element values  */

enum {
	cmPerceptual				= 0,							/* Photographic images */
	cmRelativeColorimetric		= 1,							/* Logo Colors */
	cmSaturation				= 2,							/* Business graphics */
	cmAbsoluteColorimetric		= 3								/* Logo Colors */
};

/* speed and quality flag options */
enum {
    cmNormalMode				= 0,							/* it uses the least significent two bits in the high word of flag */
    cmDraftMode					= 1,							/* it should be evaulated like this: right shift 16 bits first, mask off the */
    cmBestMode					= 2,							/* high 14 bits, and then compare with the enum to determine the option value. Do NOT shift if CWConcatColorWorld4MS is used */
    cmBestMode16Bit				= 3								/* calculate 16 bit combi LUT */
};

/* constants for the profheader-flags */
#define		kQualityMask		0x00030000		/* see Modes obove ( e.g. cmBestMode<<16 ) */
#define		kLookupOnlyMask		0x00040000
#define		kCreateGamutLutMask	0x00080000		/* Set Bit disables gamut lut creation */
#define		kUseRelColorimetric	0x00100000
#define		kStartWithXyzPCS	0x00200000
#define		kStartWithLabPCS	0x00400000
#define		kEndWithXyzPCS		0x00800000
#define		kEndWithLabPCS		0x01000000

typedef unsigned char PI_Boolean;

#if RenderInt
typedef  PI_Boolean  (__stdcall *CMBitmapCallBackProcPtr)(long max, long progress, void *refCon);
#else
typedef  PI_Boolean (*CMBitmapCallBackProcPtr)(long progress, void *refCon);
#endif
typedef CMBitmapCallBackProcPtr CMBitmapCallBackUPP;

/* Abstract data type for ColorWorld reference */
typedef struct CMPrivateColorWorldRecord *CMWorldRef;

/* Profile file and element access */
extern  PI_Boolean CMProfileElementExists(CMProfileRef prof, PI_OSType tag);
extern  CMError CMGetProfileElement(CMProfileRef prof, PI_OSType tag, unsigned long *elementSize, void *elementData);
extern  CMError CMGetProfileHeader(CMProfileRef prof, CMCoreProfileHeader *header);
extern  CMError CMGetPartialProfileElement(CMProfileRef prof, PI_OSType tag, unsigned long offset, unsigned long *byteCount, void *elementData);
extern  CMError CMSetProfileElementSize(CMProfileRef prof, PI_OSType tag, unsigned long elementSize);
extern  CMError CMSetPartialProfileElement(CMProfileRef prof, PI_OSType tag, unsigned long offset, unsigned long byteCount, void *elementData);
extern  CMError CMSetProfileElement(CMProfileRef prof, PI_OSType tag, unsigned long elementSize, void *elementData);
extern  CMError CMSetProfileHeader(CMProfileRef prof, const CMCoreProfileHeader *header);
/* Low-level matching functions */
extern  CMError CWNewColorWorld(CMWorldRef *cw, CMProfileRef src, CMProfileRef dst);
extern  CMError CWConcatColorWorld(CMWorldRef *cw, CMConcatProfileSet *profileSet);
extern  CMError CWConcatColorWorld4MS (	CMWorldRef *storage, CMConcatProfileSet	*profileSet,
									    UINT32	*aIntentArr, UINT32 nIntents,
										UINT32 dwFlags );
extern  CMError	CWCreateLink4MS( CMWorldRef storage, CMConcatProfileSet *profileSet, UINT32 aIntentArr, icProfile **theLinkProfile );
extern  CMError CWLinkColorWorld(CMWorldRef *cw, CMConcatProfileSet *profileSet);
extern  void	CWDisposeColorWorld(CMWorldRef cw);
extern  CMError CWMatchColors(CMWorldRef cw, CMColor *myColors, unsigned long count);
extern  CMError CWCheckColors(CMWorldRef cw, CMColor *myColors, unsigned long count, unsigned char *result);
extern  CMError CWCheckColorsMS(CMWorldRef cw, CMColor *myColors, unsigned long count, unsigned char *result);
extern  CMError CWGetColorSpaces(CMWorldRef cw, CMBitmapColorSpace *In, CMBitmapColorSpace *Out );
/* Bitmap matching */
extern  CMError CWMatchBitmap(CMWorldRef cw, CMBitmap *bitmap, CMBitmapCallBackUPP progressProc, void *refCon, CMBitmap *matchedBitmap);
extern  CMError CWCheckBitmap(CMWorldRef cw, const CMBitmap *bitmap, CMBitmapCallBackUPP progressProc, void *refCon, CMBitmap *resultBitmap);
extern  CMError CWMatchBitmapPlane(CMWorldRef cw, LH_CMBitmapPlane *bitmap, CMBitmapCallBackUPP progressProc, void *refCon, LH_CMBitmapPlane *matchedBitmap);
extern  CMError CWCheckBitmapPlane(CMWorldRef cw, LH_CMBitmapPlane *bitmap, CMBitmapCallBackUPP progressProc, void *refCon, LH_CMBitmapPlane *matchedBitmap);

extern  void	CMFullColorRemains( CMWorldRef Storage, long ColorMask ); /* Special function for cmyk to cmyk match */
extern  void	CMSetLookupOnlyMode( CMWorldRef Storage, PI_Boolean Mode ); /* Special function for setting or resetting LookupOnly Mode after NCMInit.., CMConcat.. */
extern  CMError CMValidateProfile( CMProfileRef prof, PI_Boolean* valid );

extern  CMError CMConvNameToIndexProfile( CMProfileRef prof, pcCMColorName, unsigned long *, unsigned long );
extern  CMError CMConvNameToIndexCW( CMWorldRef *Storage, pcCMColorName, unsigned long *, unsigned long );
extern  CMError CMConvIndexToNameProfile( CMProfileRef prof, unsigned long *, pCMColorName, unsigned long );
extern  CMError CMConvIndexToNameCW( CMWorldRef *Storage, unsigned long *, pCMColorName, unsigned long );
extern  CMError CMGetNamedProfileInfoProfile( CMProfileRef prof, pCMNamedProfileInfo );
extern  CMError CMGetNamedProfileInfoCW( CMWorldRef *Storage, pCMNamedProfileInfo );
/*
extern	CMError CMConvertNamedIndexToPCS( CMWorldRef cw, CMColor *theData, unsigned long pixCnt );
extern	CMError CMConvertNamedIndexToColors( CMWorldRef cw, CMColor *theData, unsigned long pixCnt );
extern	CMError CMConvertNamedIndexBitMap( CMWorldRef cw, CMBitmap *BitMap, CMBitmap *resultBitMap );
*/
#if PRAGMA_IMPORT_SUPPORTED
#pragma import off
#endif

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=reset
#endif

#ifdef __cplusplus
}
#endif


#endif
