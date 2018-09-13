/*----------------------------------------------------------------------------
	%%File: FILTAPI.H
	%%Unit: FILTER32
	%%Contact: rlittle@microsoft.com

	This header is distributed as part of the 32 bit Filter SDK.
	
	Changes to this header file should be sent to rlittle@microsoft.com
	or doneill@microsoft.com

	Revision History: (Current=1.03)

	1/12/96 Created
	1/23/96 Renamed grt values and synchronized with grfSupport values
	1/24/96 Extra SetFilterPref arguments (smueller)
	1/25/96 Correct packing (rlittle)
----------------------------------------------------------------------------*/

#ifndef FILTAPI_H
#define FILTAPI_H

// Definitions

#ifdef MAC
#include "macos\types.h"
#include "macos\files.h"

// Mac type equivalents

typedef Handle HANDLE;
typedef Handle HMETAFILE;
typedef Handle HENHMETAFILE;
typedef Rect RECT;
typedef long HDC;	// unused
typedef short FH;
#endif // MAC

#ifdef WIN16
typedef HANDLE HENHMETAFILE;	// win16 doesn't support enhanced metafiles
typedef HFILE FH;
#endif // WIN16

#ifdef WIN32
typedef HANDLE FH;
#endif // WIN32

// useful macros (mainly for Mac; windows.h defines most of these, so this
// will just be a failsafe.

typedef unsigned short ushort;
typedef unsigned long ulong;
typedef unsigned char uchar;
typedef int BOOL;

// these are the graphics definitions for Version 2 & Version 3

#ifdef WIN16
#define cchMaxGrName	124	   // max file path length for graphics filter
#else // !WIN16
#define cchMaxGrName    260	   // max file path length for graphics filter
#endif

#define cchMaxGrExt       4	   // chars + end-of-string mark ('\0')

#pragma pack(2)
typedef struct _FILESPEC {
	union 
		{
		struct 
			{
			ushort slippery: 1;	// True if file may disappear.
			ushort write : 1;	// True if open for write.
			ushort unnamed: 1;	// True if unnamed.
			ushort linked : 1;	// Linked to an FS FCB.
			ushort mark : 1;	// Generic mark bit.
			ushort unused : 11;
			};
		ushort wFlags;
		};
	union
		{
		char rgchExt[cchMaxGrExt];	// file extension, not used on MacPPC
		FH hfEmbed;					// embedded file handle
		};
		
	ushort wUnused;	
#ifdef MACPPC
	FSSpec fsSpec;
#else
	char szName[cchMaxGrName];		// fully qualified path
#endif // MACPPC
	ulong dcbFile;					// file position in hfEmbed
	
	/*** END VERSION 2 FIELDS
	 *** 
	 *** Fields above this point are IMMUTABLE.  They are guaranteed
	 *** to be in the above format for backwards compatibility with
	 *** existing Version 2 filters.
	 ***/
	 
	ulong dcbFileHigh;
	} FILESPEC;

// NOTE:  the client application will arbitrarily decide which type to
// send if the filter returns multiple support types

#define GrfSupportFromGrt(grt)		(ulong)(1 << ((grt) + 15))
#define grfSupportEMF	GrfSupportFromGrt(grtEMF)	// 0x00010000
#define grfSupportWMF	GrfSupportFromGrt(grtWMF)	// 0x00020000
#define grfSupportPNG	GrfSupportFromGrt(grtPNG)	// 0x00040000
#define grfSupportPICT	GrfSupportFromGrt(grtPICT)	// 0x00080000
#define grfSupportJFIF	GrfSupportFromGrt(grtJFIF)	// 0x00100000

// NOTE:  grfImport/grfExport are not mutually exclusive.  They can be
// OR'ed together for a filter that does both.  Values 2 and 4 cannot be
// used as they would be indistinguishable from version 2 return values.

#define grfImport		0x00000008
#define grfExport		0x00000010


// Version 2 support:

typedef struct _GRPI {	// GRaPhic Interface
	HMETAFILE hmf;	// metafile
	RECT   bbox;	// tightly bounds the image (in metafile units)
	ushort inch;	// metafile units per inch
} GRPI;


// Version 3 support:

#define grtEMF			0x01
#define grtWMF			0x02
#define grtPNG			0x03
#define grtPICT			0x04
#define grtJFIF			0x05

// NOTE: 
// if fPointer is fTrue, then the information is represented as
// a pointer to data rather than a handle to data.  This is not
// valid for HMETAFILE and HENHMETAFILE (as there is no pointer
// equivalent)

typedef struct _GRPIX { 	// GRaPhic Interface Extended
	ushort cbGrpix;	// size of this structure
	uchar grt;		// GRaphic Type
	ulong cbData;	// number of bytes in the graphic
	BOOL fPointer;
	union
		{
		HMETAFILE hmf;		// metafile 
		HENHMETAFILE hemf;	// enhanced metafile
		HANDLE hPng;		// handle to PNG bits
		void *pPng;			// pointer to PNG bits	(fPointer = fTrue)
		HANDLE hPict;		// handle to MacPict
		void *pPict;		// pointer to MacPict	(fPointer = fTrue)
		HANDLE hJpeg;		// handle to JPEG/JFIF
		void *pJpeg;		// pointer to JPEG/JFIF (fPointer = fTrue)
		};
	RECT bbox;			// tightly bounds the image (in metafile units)
	ulong inch;			// metafile units per inch
} GRPIX;


#ifndef WIN16

// Update the percent complete (if return value is fTrue, then
// abort the conversion) lPct is the percent
// pfnPctComplete MUST be called frequently (every 2 or 3 percent)

typedef BOOL (*PFN_PCTCOMPLETE)(long lPct, void *pvData);

#if defined(RISC)	// mips,alpha,ibm ppc,mac ppc
#define FILTAPI _cdecl
#else
#define FILTAPI PASCAL
#endif


// NOTE:  For version 3 handling, pgrpi should be cast as
//		  pgrpix = (GRPIX *)pgrpi

typedef int  (FILTAPI *PFNGetFilterInfo)(short, char *, HANDLE *, ulong);
typedef void (FILTAPI *PFNGetFilterPref)(HANDLE, HANDLE, HANDLE, ushort);
typedef int  (FILTAPI *PFNExportGr)(FILESPEC *, GRPI *, HANDLE);
typedef int  (FILTAPI *PFNExportEmbeddedGr)(FILESPEC *, GRPI *, HANDLE, ulong);
typedef int  (FILTAPI *PFNImportGr)(HDC, FILESPEC *, GRPI *, HANDLE);
typedef int  (FILTAPI *PFNImportEmbeddedGr)(HDC, FILESPEC *, GRPI *, HANDLE, ulong, char *);
typedef int  (FILTAPI *PFNRegisterPercentCallback)(HANDLE, PFN_PCTCOMPLETE, void *);
typedef int  (FILTAPI *PFNSetFilterPref)(HANDLE, char *, void *, ulong, ulong);

int  FILTAPI GetFilterInfo(short wVersion, char *pIni, 
						   HANDLE *phPrefMem, ulong lFlags);
						   
void FILTAPI GetFilterPref(HANDLE hInst, HANDLE hWnd, HANDLE hPrefMem, 
						   ushort wFlags);
						   
int  FILTAPI ExportGr(FILESPEC *pFileSpec, GRPI *pgrpi, HANDLE hPrefMem);

int  FILTAPI ExportEmbeddedGr(FILESPEC *pFileSpec, GRPI *pgrpi, HANDLE hPrefMem, ulong *pdwSize);

int  FILTAPI ImportGr(HDC hdcPrint, FILESPEC *pFileSpec, GRPI *pgrpi, 
					  HANDLE hPrefMem);
					  
int  FILTAPI ImportEmbeddedGr(HDC hdcPrint, FILESPEC *pFileSpec, GRPI *pgrpi, 
							  HANDLE hPrefMem, ulong ulSize, char *szMetaFileName);

int  FILTAPI RegisterPercentCallback(HANDLE hPrefMem, PFN_PCTCOMPLETE pfnPctComplete, void *pvData);

int  FILTAPI SetFilterPref(HANDLE hPrefMem, char *szOption, void *pvValue, ulong dwSize, ulong dwType);

#endif // WIN16


// Definitions of ordinal values for entry points
// backwards compatibility only
#define ordGetFilterInfo ((DWORD)1)
#define ordImportGr ((DWORD)2)


// SetFilterPref data types
// these exactly parallel a subset of Win32 registry value data types
#if !defined(REG_NONE) || !defined(REG_SZ) || !defined(REG_BINARY) || !defined(REG_DWORD)
#define REG_NONE                    ( 0 )   // No value type
#define REG_SZ                      ( 1 )   // '\0' terminated string
#define REG_BINARY                  ( 3 )   // Free form binary
#define REG_DWORD                   ( 4 )   // 32-bit number
#endif


// ERROR RETURN VALUES
#define IE_NO_ERROR				0
#define IE_INTERNAL_ERROR		(-1)

#define IE_BASE				0x14B4
#define IE(err)				(IE_BASE + err)

// IMPORT/EXPORT ERRORS
#define IE_NOT_MY_FILE		IE(0x0001)	// generic not my file error
#define IE_TOO_BIG			IE(0x0002)	// bitmap or pict too big error
#define IE_DUMB_BITMAP		IE(0x0003)	// bitmap all white
#define IE_BAD_VCHAR		IE(0x0004)	// bad vchar in ImportString
#define IE_BAD_TOKEN		IE(0x0005)	// illegal wp token
#define IE_NO_VERIFY		IE(0x0006)	// failed to verify imported story
#define IE_UNKNOWN_TYPE		IE(0x0007)	// unknown file type
#define IE_NOT_WP_FILE		IE(0x0008)	// not a wp file
#define IE_BAD_FILE_DATA	IE(0x0009)	// current file data is bad
#define IE_IMPORT_ABORT		IE(0x000A)	// import abort alert
#define IE_MEM_FULL			IE(0x000B)	// ran out of memory during import
#define IE_MSNG_FONTS		IE(0x000C)	// system font not found
#define IE_META_TOO_BIG		IE(0x000D)	// metafile too big
#define IE_MEM_FAIL			IE(0x000F)	// couldn't lock memory during import
#define IE_NO_FILTER		IE(0x0012)	// expected filter not found

#define IE_UNSUPP_COMPR		IE(0x0029)	// unsupported compress style
#define IE_UNSUPP_VERSION	IE(0x002A)	// unsupported file version
#define IE_UNSUPP_COLOR		IE(0x002B)	// unsupported color style

#define IE_ERROR_NOMSG		IE(0x0037)	// dialog box cancel
#define IE_FILE_NOT_FOUND	IE(0x003C)	// file not found
#define IE_BUG				IE(0x0051)
#define IE_BAD_METAFILE		IE(0x0053)	// inconsistent metafile data
#define IE_BAD_METAFILE2	0xCCCC		// inconsistent metafile data

#define IE_BAD_PARAM		IE(0x0100)	// bad parameter passed by client
#define IE_UNSUPP_FORMAT	IE(0x0101)	// cannot provide/accept format
#define FA_DISK_ERROR		7015


// values for WPG-specific PRF fields (for GetFilterPref)
// backwards compatibility only
#define bBGIni			0	// do what the mstxtcnv.ini file says
#define bBGDiscard		1	// discard the background
#define bBGKeep			2	// keep the background

#define bCCNone			0
#define bCCOutline		1	// convert black to black, all others to white
#define bCCBlackWhite	2	// convert white to white, all others to black
#define bCCInvert		3	// invert all colours, except black and white
#define bCCOutline6		4	// true outline
#define bCCSilhouette	5	// everything to black
#define bCCInvert6		6	// invert all colours, including black<->white

#define bMRNone			0
#define bMRHorizontal	1	// flip image horizontally, across y-axis
#define bMRVertical		2	// flip image vertically, across x-axis

typedef struct _PRF
	{
	uchar fSilent;
	uchar bBackground;
	uchar bColorChange;
	uchar bMirror;	// formerly fMirror
	unsigned dgRotate;
	} PRF;
#pragma pack()

#endif // !FILTAPI_H

