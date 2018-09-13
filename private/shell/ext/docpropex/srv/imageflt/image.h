//***************************************************************************
//
//	IMAGE.h
//
// Proposed new interface!
//
//		Include file for the Image API Library.
//    This is the external containing header data required
//    by the outside world.
//
//	Revision History
//		17-Nov-95	TerryJ		Original
//    04-Jan-96   TerryJ      Code cleaned up. Memory mode added.
//    16-Jan-96   TerryJ      Registry validation capacity added.
//    
//
//***************************************************************************

#ifndef  _IMAGEFILELIB_H
#define  _IMAGEFILELIB_H

#include "msffdefs.h"   //include platform dependent defs

#ifdef _MAC
#include <Macname1.h>
#include "Types.h"
#include "Files.h"
#include <Macname2.h>
#endif // _MAC

#ifdef __cplusplus
extern "C" {            // Assume C declarations for C++
#endif   // __cplusplus

/***********
| IFLMODE:	Open modes
\***********/
typedef enum
   {
   IFLM_READ	    =0x00,
   IFLM_WRITE	    =0x01,
   IFLM_MEMORY     =0x80,    // OR to operate in memory
   IFLM_EXTRACT_ALPHA  =0x40,    // OR to extract separate image and alpha channel info
   IFLM_CHUNKY_ALPHA   =0x20,    //OR to extract RGBA chunky data
   } IFLMODE;

/***********
| IFLCLASS:	Image classes
\***********/
typedef enum
   {
   IFLCL_BILEVEL     = 0,   // 1 BPP
   IFLCL_GRAY        = 1,   // 2,4,6,8 BPP
   IFLCL_GRAYA       =11,   // 16 BPP chunky 
   IFLCL_PALETTE     = 2,   // 2,4,6,8 BPP
   IFLCL_RGB         = 3,   // 24 BPP chunky
   IFLCL_RGBPLANAR   = 4,   // 24 BPP in 8 bit planes
   IFLCL_RGBA        = 5,   // 32 BPP chunky
   IFLCL_RGBAPLANAR  = 6,   // 32 BPP in four 8 bit planes
   IFLCL_CMYK        = 7,
   IFLCL_YCC         = 8,
   IFLCL_CIELAB      = 9,
   IFLCL_NONE        =10   // no class set! (error)
   } IFLCLASS;

typedef enum
   {
   IFLTF_NONE     =0,
   IFLTF_STRIPS   =1,
   IFLTF_TILES    =2
   } IFLTILEFORMAT;

/***********
| IFLCOMMAND:	 Commands to IFL_Control
\***********/
typedef enum
   {
   IFLCMD_GETERROR      =0,   // get error code
   IFLCMD_GETLINESIZE   =1,   // compute line size
   IFLCMD_PALETTE       =2,   // get or set palette or map
   IFLCMD_SETPACKMODE   =3,   // set mode for packing/unpacking pixels
   IFLCMD_RESOLUTION    =7,   // get dots per meter
   IFLCMD_GETNUMIMAGES  =10,  // get the number of images
   IFLCMD_IMAGESEEK     =11,  // seek to the next image
   IFLCMD_DELETE        =12,  // delete current image
   IFLCMD_TILEFORMAT    =13,  // set/get the tiling format
   IFLCMD_YCCINFO       =14,  // set/get YCC information
   IFLCMD_YCCRGBCONVERT =15,  // set/get YCC/RGB conversion state
   IFLCMD_COLORIMETRY   =16,  // set/get Colorimetry info
   IFLCMD_CMYKINFO      =17,  // set/get CMYK specific data

   IFLCMD_BKGD_IDX      =18,  // set/get background color by index
   IFLCMD_BKGD_RGB      =19,  // set/get background color
   IFLCMD_TRANS_IDX     =20,  // set/get transparency color index
   IFLCMD_TRANS_RGB     =21,  // set/get transparency color
   IFLCMD_TRANS_MASK_INFO     =22,  // set/get transparency mask info
   IFLCMD_TRANS_MASK          =23,  // set/get transparency mask
   IFLCMD_ALPHA_PALETTE_INFO  =24,  // set/get alpha palette info
   IFLCMD_ALPHA_PALETTE       =25,  // set/get alpha palette
   IFLCMD_ALPHA_CHANNEL_INFO  =26,  // set/get alpha channel info
   IFLCMD_ALPHA_CHANNEL       =27,  // set/get alpha channel
   IFLCMD_GAMMA_VALUE         =28,  // set/get gamma value

   IFLCMD_FIRST_TEXT          =29,  // get first text string
   IFLCMD_NEXT_TEXT           =30,  // get next text strings.
   IFLCMD_DATETIME_STRUCT     =31,  // retrieve date/time as a structure.

   IFLCMD_TIFF          =0x4000,    // TIFF specific commands
   IFLCMD_TIFFTAG       =0x4001,
   IFLCMD_TIFFTAGDATA   =0x4002,
   IFLCMD_PCX           =0x4200,    // PCX specific commands
   IFLCMD_BMP           =0x4400,    // BMP specific commands
   IFLCMD_BMP_VERSION   =0x4401,    // Windows os2 1.2 os2 2.0 versions
   IFLCMD_TGA           =0x4800,    // TGA specific commands
   IFLCMD_GIF           =0x4E00,    // GIF specific commands
   IFLCMD_GIF_WHITE_IS_ZERO =0x4E01,   // White == 0 in GIF file
   IFLCMD_JPEG          =0x5700,    // WPG specific commands
   IFLCMD_JPEGQ         =0x5701,    // Quality
   IFLCMD_PCD           =0x5800,    // Kodak PCD specific commands
   IFLCMD_PCDGETTRANSFORM  =0x5801,
   IFLCMD_PCDSETTRANSFORM  =0x5802,
   IFLCMD_PCDSETCLASS      =0x5803,
   IFLCMD_PNG           =0x5900,    // PNG specific commands
   IFLCMD_PNG_SET_FILTER   =0x590A, // Set PNG filter type
   IFLCMD_PNG_sBIT         =0x590B, // set/get PNG sBIT chunk

   IFLCMD_GETDATASIZE      =0x8000  // OR with this to get the size
   } IFLCOMMAND;

typedef IFLCOMMAND IFLCMD;    // this is done as backwards 
                              // compatibility and may be able
                              // to be eliminated eventually

/***********
| IFLDESC:	 Available description strings (use as bitfields)
\***********/
typedef enum
   {
   IFLDESC_NONE          =0,   // no descriptions supported
   IFLDESC_DESCRIPTION   =1,   // image description field (TIFF TGA PNG)
   IFLDESC_SOFTWARENAME  =2,   // software name (TIFF TGA)  Software (PNG)
   IFLDESC_ARTISTNAME    =4,   // artist name (TIFF TGA)    Author (PNG)
   IFLDESC_DOCUMENTNAME  =8,   // the document name field   Title (PNG)
   IFLDESC_DATETIME      =16,  // the date/time field
   IFLDESC_COPYRIGHT     =32,  // copyright notice (PNG)
   IFLDESC_DISCLAIMER    =64,  // Legal disclaimer (PNG)
   IFLDESC_WARNING       =128, // content warning (PNG)
   IFLDESC_SOURCE        =256, // source device (PNG)
   IFLDESC_COMMENT       =512, // misc comment (PNG)
   } IFLDESC;

/***********
| IFLPACKMODE:	 Packing modes
\***********/
typedef enum
   {
   IFLPM_PACKED         =0,
   IFLPM_UNPACKED       =1,
   IFLPM_LEFTJUSTIFIED  =2,
   IFLPM_NORMALIZED     =3,
   IFLPM_RAW            =4
   } IFLPACKMODE;

/***********
| IFLSEQUENCE:	Line sequences
\***********/
typedef enum
   {
   IFLSEQ_TOPDOWN    =0,         // most
   IFLSEQ_BOTTOMUP   =1,         // BMP and TGA compressed
   IFLSEQ_GIF_INTERLACED =2,     // for GIF
   IFLSEQ_ADAM7_INTERLACED = 3   // for PNG
   } IFLSEQUENCE;

/***********
| IFLERROR:	Possible errors
\***********/
typedef enum
   {
   IFLERR_NONE          =0,   // no error
   IFLERR_HANDLELIMIT   =1,   // too many open files
   IFLERR_PARAMETER     =2,   // programmer error
   IFLERR_NOTSUPPORTED  =3,   // feature not supported by format
   IFLERR_NOTAVAILABLE  =4,   // item not available
   IFLERR_MEMORY        =5,   // insufficient memory
   IFLERR_IMAGE         =6,   // bad image data (decompression error)
   IFLERR_HEADER        =7,   // header has bad fields
   IFLERR_IO_OPEN       =8,   // error on open()
   IFLERR_IO_CLOSE      =9,   // error on close()
   IFLERR_IO_READ       =10,  // error on read()
   IFLERR_IO_WRITE      =11,  // error on write()
   IFLERR_IO_SEEK       =12,  // error on lseek()
   } IFLERROR;

 typedef enum    // new error messages to go here. This error
                  // info is maintained here rather than IFLERROR
                  // to retain backwards compatibility
 
   {
   IFLEXTERR_NONE,
   IFLEXTERR_NO_DLL,             // open
   IFLEXTERR_NO_LIBRARY,         // open: no library specified
   IFLEXTERR_BAD_DLL,            // DLL doesn't have right entry points
   IFLEXTERR_CANNOT_IMPORT,      // open
   IFLEXTERR_CANNOT_EXPORT,      // open
   IFLEXTERR_CANNOT_COMPRESS,    // open
   IFLEXTERR_BAD_FORMAT,         // read
   IFLEXTERR_UNKNOWN_VARIANT,    // open/read: for example, JFIFs and
                                 // BMPs have many variants - some may
                                 // not be supported
   IFLEXTERR_SHARING_VIOLATION,        // read
   IFLEXTERR_NO_BACKGROUND_COLOR,      // read: no background color specified
                                       // when doing transparencies
   IFLEXTERR_BACKGROUND_NOT_SUPPORTED, // background colors not supported
                                       // (currently) by this format
   IFLEXTERR_NO_FILE,            // file doesn't exist
   IFLEXTERR_END_OF_FILE,        // read
   IFLEXTERR_MEMORY,             // insufficient memory
   IFLEXTERR_DESC_CANNOT_GET,    // file is write mode: can't get descriptions
   IFLEXTERR_DESC_CANNOT_SET,    // file is read mode: can't set descriptions
   IFLEXTERR_NO_PATH_IN_REGISTRY,   // the filter path isn't in the registry
   IFLEXTERR_NOT_IFL_HANDLE,     // the pointer passed isn't an IFLHANDLE
   IFLEXTERR_REGISTRY_DAMAGED,   // entry in registry not correct format
   IFLEXTERR_BAD_COMPRESSION,    // error in data compression; cannot read.
   } IFLEXTERROR;


/***********
| IFLCOMPRESSION:	Compression options
\***********/
typedef enum
   {
   IFLCOMP_NONE      =0,   // no compression
   IFLCOMP_DEFAULT   =1,   // whatever is defined for the format
   IFLCOMP_RLE       =2,   // various RLE schemes (PACKBITS in TIFF)
   IFLCOMP_CCITT1D   =3,   // TIFF modified G3
   IFLCOMP_CCITTG3   =4,   // TIFF raw G3
   IFLCOMP_CCITTG4   =5,   // TIFF G4
   IFLCOMP_LZW       =6,   // Lempel-Zif
   IFLCOMP_LZWHPRED  =7,   // LZW with TIFF horizontal differencing
   IFLCOMP_JPEG      =8    // JPEG compression
   } IFLCOMPRESSION;

/***********
| Date Time structure for IFL
\***********/
typedef struct
   {
   short Year;
   short Month;
   short Day;
   short Hour;
   short Minute;
   short Second;
   } IFL_DATETIME;

/***********
| RGB color structure for IFL
\***********/
typedef struct  // rgb color values
   {
   BYTE  bRed;
   BYTE  bGreen;
   BYTE  bBlue;
   } IFLCOLORRGB;

typedef struct  // new color struct capable of 16 bit values. 
   {            
   WORD  wRed;
   WORD  wGreen;
   WORD  wBlue;
   }  IFLCOLOR;

/***********
| Types for multiple images
\***********/
typedef enum
   {
   IFLIT_PRIMARY     =0,
   IFLIT_THUMBNAIL   =1,
   IFLIT_MASK        =2
   } IFLIMAGETYPE;

/***********
| Bitmap types
\***********/
typedef enum
   {
   IFLBV_WIN_3    =0x10,   // Windows 3.x
   IFLBV_OS2_1    =0x20,   // OS2 1.2
   IFLBV_OS2_2S   =0x40,   // OS2 2.0 single image
   IFLBV_OS2_2M   =0x41    // OS2 2.0 multiple image
   } IFLBMPVERSION;


/***********
| Capabilities Type, for interpreting Registry info
\***********/
typedef enum
   {
   IFLCAP_NOT_AVAILABLE =0x0000,    // if option not available
      
      // ** compression options **
   IFLCAP_COMPNONE      =0x0001,
   IFLCAP_COMPRLE       =0x0002,
   IFLCAP_COMPG3        =0x0004,
   IFLCAP_COMPG4        =0x0008,
   IFLCAP_COMPLZW       =0x0010,
   IFLCAP_COMPLZWPRED   =0x0020,
   IFLCAP_COMPJPEG      =0x0040,
   IFLCAP_COMPDEFAULT   =0x0080,

      // ** bit plane options **     // note that for RGB, RGB QUAD
   IFLCAP_1BPP          =0x0001,     //  and RGBA bit plane depth must
   IFLCAP_2BPP          =0x0002,     //  be multiplied by 3, 4 and 4
   IFLCAP_3BPP          =0x0004,     //  respectively for the full
   IFLCAP_4BPP          =0x0008,     //  pixel depth size.
   IFLCAP_5BPP          =0x0010,
   IFLCAP_6BPP          =0x0020,
   IFLCAP_7BPP          =0x0040,
   IFLCAP_8BPP          =0x0080,
   IFLCAP_8BPP_QUAD     =0x0100,
   IFLCAP_12BPP         =0x0200,
   IFLCAP_16BPP         =0x0400,

      // ** Transparency options
   IFLCAP_NO_TRANS      =0x0000,
   IFLCAP_1BITMASK      =0x0001,
   IFLCAP_ALPHACHANNEL  =0x0002,
   IFLCAP_ALPHAPALETTE  =0x0004,
   IFLCAP_TRANSCOLOR    =0x0008,

   } IFLCAPABILITIES;


/***********
| Alpha/Transparency info structs
\***********/

typedef struct
   {
   DWORD dwWidth;
   DWORD dwHeight;
   }  IFL_TRANS_MASK_INFO;

typedef struct
   {
   DWORD dwWidth;
   DWORD dwHeight;
   WORD wBitsPerPixel;
   }  IFL_ALPHA_CHANNEL_INFO;

typedef struct
   {
   char  *szKey;
   char  *szText;
   } IFL_COMMENT_STRING;

typedef struct
   {
   unsigned char bPNGType;
   unsigned char bGrayBits;
   unsigned char bRedBits;
   unsigned char bGreenBits;
   unsigned char bBlueBits;
   unsigned char bAlphaBits;
   } IFLPNGsBIT;


/***********
| Handle types
| Use FILTERHANDLE to access filters
\***********/

typedef void far * IFLHANDLE;  // handle is a void pointer to hide the
                                 // details of the file handle from other
                                 // programmers. 

/* -------- new stuff   ---------------------- */

typedef enum
   {
   IFLT_UNKNOWN,     // unknown or unsupported file type
   IFLT_GIF,
   IFLT_BMP,
   IFLT_JPEG,
   IFLT_TIFF,
   IFLT_PNG,
   IFLT_PCD,
   IFLT_PCX,
   IFLT_TGA,
   IFLT_PICT,
   IFLT_NIF         // not supported for MAC, but left in to avoid enum val
                    // changes
   } IFLTYPE;


/***********
| IFL virtual (memory) mode types
\***********/

// internal virtual (memory) file i/o routine pointers
typedef int	   (__cdecl _vopen)  (LPSTR, int, int);
typedef int    (__cdecl _vclose) (int);
typedef int	   (__cdecl _vread)  (int, LPVOID, int);
typedef int	   (__cdecl _vwrite) (int, LPVOID, int);
typedef long   (__cdecl _vlseek) (int, long, int);

// structure used to hold virtual (memory) i/o functions
// when using IFLM_MEMORY mode.
typedef struct ImageIOFuncs    
   {                           
   _vopen  *vopen;             
   _vclose *vclose;
   _vread  *vread;
   _vwrite *vwrite;
   _vlseek *vlseek;

   LPVOID  userdata;
   } IFLIOF, far * LPIFLIOF;

// structure used to hold virtual (memory) memory info
// when using IFLM_MEMORY mode.
typedef struct ImageMemStruct  
   {                           
   long    pos;                
   long    alloced;
   long    length;
   LPVOID  data;
   } IFLIOM, far * LPIFLIOM;


/***********
| IFL function prototypes
\***********/
IFLERROR HILAPI iflOpen(IFLHANDLE iflh, LPSTR FileName, IFLMODE Mode);
IFLERROR HILAPI iflClose(IFLHANDLE iflh);
IFLERROR HILAPI iflRead(IFLHANDLE iflh, LPBYTE Buffer, int NumLines);
IFLERROR HILAPI iflWrite(IFLHANDLE iflh, LPBYTE Buffer, int NumLines);
IFLERROR HILAPI iflSeek(IFLHANDLE iflh, int Line);
IFLERROR HILAPI iflControl(IFLHANDLE iflh, IFLCMD Command, short sParam, long lParam, LPVOID pParam);

   // new commands (general)

IFLERROR HILAPI iflImageType(LPSTR FileName, IFLTYPE *ImageType);
IFLTYPE  HILAPI iflTypeFromExtension (char far * Filename);
void            iflGetLibName(IFLTYPE fileType, LPSTR libName);
IFLERROR HILAPI iflFilterCap(IFLTYPE ImageType, IFLCLASS ImageClass,
                             WORD *Color, WORD *Compression,
                             WORD *Transparency);
IFLERROR HILAPI iflInstalledFilterQuery(IFLTYPE filterType,
                                        BOOL    *bImports,
                                        BOOL    *bExports);
IFLERROR HILAPI iflExtensionCount(IFLTYPE filterType,
                                  short   *sCount);
IFLERROR HILAPI iflExtensionQuery(IFLTYPE filterType,
                                  short   sExtNum,
                                  LPSTR   szExtension);
IFLERROR HILAPI iflFormatNameQuery(IFLTYPE filterType,
                                  LPSTR   szFormatName,
                                  short   sFormatNameSize);
IFLEXTERROR HILAPI iflGetExtendedError(IFLHANDLE iflh);

   // description manipulation

IFLERROR HILAPI iflGetDesc(IFLHANDLE iflh, IFLDESC DescType, LPSTR *pDescription);
IFLERROR HILAPI iflPutDesc(IFLHANDLE iflh, IFLDESC DescType, LPSTR Description);
IFLERROR HILAPI iflSupportedDesc(IFLHANDLE iflh, IFLDESC *Supports);

   // handle manipulation

IFLHANDLE HILAPI iflCreateReadHandle( IFLTYPE        ImageType);
IFLHANDLE HILAPI iflCreateWriteHandle(int           Width,         // Width of image in pixels
                                     int            Height,        // Height of image in pixels
                                     IFLCLASS       ImageClass,    // image class
                                     int            BitsPerSample, // Number of bits per sample
                                     IFLCOMPRESSION Compression,   // defined above
                                     IFLTYPE        ImageType      // Type of image (GIF, PCX, etc)
                                     );
IFLERROR HILAPI iflFreeHandle(IFLHANDLE iflh);

   // background manipulation
IFLERROR HILAPI iflGetBackgroundColor(IFLHANDLE iflh, IFLCOLOR *clBackColor);
IFLERROR HILAPI iflSetBackgroundColor(IFLHANDLE iflh, IFLCOLOR clBackColor);
IFLERROR HILAPI iflSetBackgroundColorByIndex(IFLHANDLE iflh, short iColorIndex);

   // accessors and manipulators

#ifdef _MAC
IFLERROR HILAPI iflSetMacCreator(OSType OSCreator);
#endif // _MAC
IFLCLASS    HILAPI iflGetClass(IFLHANDLE iflh);
int         HILAPI iflGetHeight(IFLHANDLE iflh);
int         HILAPI iflGetWidth(IFLHANDLE iflh);
int         HILAPI iflGetRasterLineCount(IFLHANDLE iflh);
IFLSEQUENCE HILAPI iflGetSequence(IFLHANDLE iflh);
IFLERROR    HILAPI iflSetSequence(IFLHANDLE iflh, IFLSEQUENCE iflsSeq);
IFLCOMPRESSION HILAPI iflGetCompression(IFLHANDLE iflh);
int         HILAPI iflGetBitsPerChannel(IFLHANDLE iflh);
int         HILAPI iflGetBitsPerPixel(IFLHANDLE iflh);
IFLTYPE     HILAPI iflGetImageType(IFLHANDLE iflh);

#ifdef __cplusplus
}
#endif   // __cplusplus

#endif   // _IMAGEFILELIB_H

/////////////////////////////////////////////////////////////////////////////
