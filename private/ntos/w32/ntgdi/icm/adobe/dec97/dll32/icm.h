#ifndef ICM_H
#define ICM_H
#include "windows.h"
#include "icc.h"
#include "icc_i386.h"

#define CMS_GET_VERSION     0x00000000
#define CMS_GET_IDENT       0x00000001
#define CMS_GET_DRIVER_LEVEL 0x00000002
#define CMS_GET_RESERVED    0xFFFFFFFC

#define CMS_LEVEL_1         0x00000001
#define CMS_LEVEL_2         0x00000002
#define CMS_LEVEL_3         0x00000004
#define CMS_LEVEL_RESERVED  0xFFFFFFFC



#define CMS_FORWARD         0x00000000
#define CMS_BACKWARD        0x00000001

#define CMS_X555WORD        0x00000000
#define CMS_565WORD         0x00000001
#define CMS_RGBTRIPLETS     0x00000002
#define CMS_BGRTRIPLETS     0x00000004
#define CMS_XRGBQUADS       0x00000008
#define CMS_XBGRQUADS       0x00000010
#define CMS_QUADS           0x00000020

#ifndef ICMDLL
#define LCS_CALIBRATED_RGB  0x00000000
#define LCS_DEVICE_RGB	    0x00000001
#define LCS_DEVICE_CMYK     0x00000002

#define LCS_GM_DEFAULT      0x00000000
#define LCS_GM_BUSINESS     0x00000001
#define LCS_GM_GRAPHICS     0x00000002
#define LCS_GM_IMAGES       0x00000004
#endif

// Use printer colors       == 0
// Change all RGBs          == CM_USE_ICM;
// Select downloaded CRD    == CM_USE_CS | CM_USE_CRD
// Download/select CRD      == CM_USE_CS | CM_USE_CRD | CM_SEND_CRD
// Use Sony                 == CM_USE_CS
#define CM_USE_CS           0x00000001
#define CM_USE_CRD          0x00000002
#define CM_SEND_CRD         0x00000004
#define CM_USE_ICM          0x00000008
#define CM_CMYK_IN          0x00000010
#define CM_CMYK_DIB_IN	    0x00000020
#define CM_CMYK_OUT         0x00000040
#define CM_CMYK 	    CM_CMYK_IN | CM_CMYK_DIB_IN | CM_CMYK_OUT


// typedef HANDLE      HCOLORSPACE;
typedef DWORD       HCTMTRANSFORM;


/*  Logical Color Space Structure */

#ifndef ICMDLL
typedef struct tagLOGCOLORSPACE {
DWORD lcsSignature;
DWORD lcsVersion;
DWORD lcsSize;
DWORD lcsCSType;
DWORD lcsGamutMatch;
CIEXYZTRIPLE lcsEndpoints;
DWORD lcsGammaRed;
DWORD lcsGammaGreen;
DWORD lcsGammaBlue;
char    lcsFilename[MAX_PATH];
} LOGCOLORSPACE;
typedef LOGCOLORSPACE FAR *LPLOGCOLORSPACE;
#endif

typedef struct tagICMINFO {
	LOGCOLORSPACE   lcsSource;  // source image colorspace
	HCTMTRANSFORM   hICMT;      // Handle to the associated transform
        char            lcsDestFilename[256];
        char            lcsTargetFilename[256];
        LPSTR           lppd;       // Used to find the buffered bitmap.
                                    // Fix bug 195632. jjia  2/20/97.
} ICMINFO , FAR *LPICMINFO;


typedef enum {CS_DEVICE_RGB = 0, CS_DEVICE_CMYK,
			CS_CALIBRATED_RGB, CS_SONY_TRINITRON } CSPACESET;


#ifndef ICMDLL
BOOL FAR PASCAL EnableICM(HDC, BOOL);
HANDLE FAR PASCAL LoadImageColorMatcher(LPSTR);
BOOL FAR PASCAL FreeImageColorMatcher(HANDLE);
int FAR PASCAL EnumColorProfiles(HDC,FARPROC,LPARAM);
BOOL FAR PASCAL CheckColorsInGamut(HDC,LPVOID,LPVOID,DWORD);
HANDLE FAR PASCAL GetColorSpace(HDC);
BOOL FAR PASCAL GetLogColorSpace(HCOLORSPACE,LPVOID,DWORD);
HCOLORSPACE FAR PASCAL CreateColorSpace(LPLOGCOLORSPACE);
BOOL FAR PASCAL SetColorSpace(HDC,HCOLORSPACE);
BOOL FAR PASCAL DeleteColorSpace(HCOLORSPACE);
BOOL FAR PASCAL GetColorProfile(HDC,LPSTR,WORD);
BOOL FAR PASCAL SetColorProfile(HDC,LPSTR);
BOOL FAR PASCAL GetDeviceGammaRamp(HDC,LPVOID);
BOOL FAR PASCAL SetDeviceGammaRamp(HDC,LPVOID);
BOOL FAR PASCAL ColorMatchToTarget(HDC,HDC,WORD);
#endif

//#define CS_ENABLE       1
//#define CS_DISABLE      2
//#define CS_DELETE_TRANSFORM 3

DWORD _loadds FAR PASCAL CMGetInfo(DWORD dwInfo);

HCTMTRANSFORM _loadds      FAR PASCAL CMCreateTransform(LPLOGCOLORSPACE lpCS, 
						LPSTR lpDevCh, LPSTR lpTargetDevCh);

BOOL _loadds    FAR PASCAL  CMDeleteTransform(HCTMTRANSFORM hTransform);

BOOL _loadds    FAR PASCAL  CMTranslateRGB(HCTMTRANSFORM hTransform,RGBQUAD RGBQuad,
						LPVOID lpResult, DWORD dwFlags);

BOOL _loadds    FAR PASCAL  CMTranslateRGBs(HCTMTRANSFORM hTransform, 
						 LPVOID    lpSrc, DWORD dwSrcFlags,
							DWORD nSrcWidth, DWORD nSrcHeight, DWORD nSrcStride,
						 LPVOID lpDest, DWORD dwDestFlags, DWORD dwFlags);

BOOL _loadds    FAR PASCAL   CMCheckColorsInGamut(HCTMTRANSFORM hTransform, 
						 LPVOID   lpSrc,
						 LPVOID lpDest, DWORD dwCount);
BOOL _loadds    FAR PASCAL   CMGetPS2ColorSpaceArray(
                                                 LPSTR       lpProfileName,
                                                 DWORD       InputIntent,
                                                 WORD        InpDrvClrSp,
                                                 MEMPTR      lpBuffer, 
                                                 LPDWORD     lpcbSize,
                                                 BOOL        AllowBinary);

BOOL _loadds    FAR PASCAL   CMGetPS2ColorRenderingDictionary(
                                                 LPSTR       lpProfileName,
                                                 DWORD       Intent, 
                                                 MEMPTR      lpMem, 
                                                 LPDWORD     lpcbSize,
                                                 BOOL        AllowBinary);

BOOL _loadds    FAR PASCAL  CMGetPS2ColorRenderingIntent(
                                                 LPSTR       lpProfileName,
                                                 DWORD       Intent,
                                                 MEMPTR      lpMem,
                                                 LPDWORD     lpcbSize);


HCTMTRANSFORM _loadds      FAR PASCAL ICMCreateTransform(LPLOGCOLORSPACE lpCS, 
						LPSTR lpDevCh, LPSTR lpTargetDevCh);

BOOL _loadds    FAR PASCAL  ICMDeleteTransform(HCTMTRANSFORM hTransform);

BOOL _loadds    FAR PASCAL  ICMTranslateRGB(HCTMTRANSFORM hTransform, RGBQUAD RGBQuad,
						LPVOID lpResult, DWORD dwFlags);

BOOL _loadds    FAR PASCAL  ICMTranslateRGBs(HCTMTRANSFORM hTransform, 
						 LPVOID    lpSrc, DWORD dwSrcFlags,	
                   DWORD nSrcWidth, DWORD nSrcHeight, DWORD nSrcStride,
						 LPVOID lpDest, DWORD dwDestFlags, DWORD dwFlags);

BOOL _loadds    FAR PASCAL   ICMCheckColorsInGamut(HCTMTRANSFORM hTransform, 
						 LPVOID   lpSrc,
						 LPVOID lpDest, DWORD dwCount);
// ALWAYS_ICM
HCTMTRANSFORM _loadds FAR PASCAL CreateDefTransform (LPVOID lppd);
BOOL _loadds FAR PASCAL DeleteDefTransform (HCTMTRANSFORM hTransform);
#endif

