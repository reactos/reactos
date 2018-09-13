/*++

Copyright (c) 1996-1999 Microsoft Corporation

Module Name:

    icm.h

Abstract:

    Public header file for Image Color Management

Revision History:

--*/

#ifndef _ICM_H_
#define _ICM_H_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Support for named color profiles
//

typedef char COLOR_NAME[32];
typedef COLOR_NAME *PCOLOR_NAME, *LPCOLOR_NAME;

typedef struct tagNAMED_PROFILE_INFO{
	DWORD		dwFlags;
	DWORD		dwCount;
	DWORD		dwCountDevCoordinates;
	COLOR_NAME	szPrefix;
	COLOR_NAME	szSuffix;
}NAMED_PROFILE_INFO;
typedef NAMED_PROFILE_INFO *PNAMED_PROFILE_INFO, *LPNAMED_PROFILE_INFO;


//
// Color spaces
//
// The following color spaces are supported.
// Gray, RGB, CMYK, XYZ, Yxy, Lab, generic 3 channel color spaces where
// the profiles defines how to interpret the 3 channels, named color spaces
// which can either be indices into the space or have color names, and
// multichannel spaces with 1 byte per channel upto MAX_COLOR_CHANNELS.
//

#define MAX_COLOR_CHANNELS  8   // maximum number of HiFi color channels

struct GRAYCOLOR {
    WORD    gray;
};

struct RGBCOLOR {
    WORD    red;
    WORD    green;
    WORD    blue;
};

struct CMYKCOLOR {
    WORD    cyan;
    WORD    magenta;
    WORD    yellow;
    WORD    black;
};

struct XYZCOLOR {
    WORD    X;
    WORD    Y;
    WORD    Z;
};

struct YxyCOLOR {
    WORD    Y;
    WORD    x;
    WORD    y;
};

struct LabCOLOR {
    WORD    L;
    WORD    a;
    WORD    b;
};

struct GENERIC3CHANNEL {
    WORD    ch1;
    WORD    ch2;
    WORD    ch3;
};

struct NAMEDCOLOR {
    DWORD        dwIndex;
};

struct HiFiCOLOR {
    BYTE    channel[MAX_COLOR_CHANNELS];
};


typedef union tagCOLOR {
    struct GRAYCOLOR        gray;
    struct RGBCOLOR         rgb;
    struct CMYKCOLOR        cmyk;
    struct XYZCOLOR         XYZ;
    struct YxyCOLOR         Yxy;
    struct LabCOLOR         Lab;
    struct GENERIC3CHANNEL  gen3ch;
    struct NAMEDCOLOR       named;
    struct HiFiCOLOR        hifi;
} COLOR;
typedef COLOR *PCOLOR, *LPCOLOR;

typedef enum {
    COLOR_GRAY       =   1,
    COLOR_RGB,
    COLOR_XYZ,
    COLOR_Yxy,
    COLOR_Lab,
    COLOR_3_CHANNEL,        // WORD per channel
    COLOR_CMYK,
    COLOR_5_CHANNEL,        // BYTE per channel
    COLOR_6_CHANNEL,        //      - do -
    COLOR_7_CHANNEL,        //      - do -
    COLOR_8_CHANNEL,        //      - do -
    COLOR_NAMED,
} COLORTYPE;
typedef COLORTYPE *PCOLORTYPE, *LPCOLORTYPE;

//
// Bitmap formats supported
//

typedef enum {

    //
    // 16bpp - 5 bits per channel. The most significant bit is ignored.
    //

    BM_x555RGB      = 0x0000,
    BM_x555XYZ      = 0x0101,
    BM_x555Yxy,
    BM_x555Lab,
    BM_x555G3CH,

    //
    // Packed 8 bits per channel => 8bpp for GRAY and
    // 24bpp for the 3 channel colors, more for hifi channels
    //

    BM_RGBTRIPLETS  = 0x0002,
    BM_BGRTRIPLETS  = 0x0004,
    BM_XYZTRIPLETS  = 0x0201,
    BM_YxyTRIPLETS,
    BM_LabTRIPLETS,
    BM_G3CHTRIPLETS,
    BM_5CHANNEL,
    BM_6CHANNEL,
    BM_7CHANNEL,
    BM_8CHANNEL,
    BM_GRAY,

    //
    // 32bpp - 8 bits per channel. The most significant byte is ignored
    // for the 3 channel colors.
    //

    BM_xRGBQUADS    = 0x0008,
    BM_xBGRQUADS    = 0x0010,
    BM_xG3CHQUADS   = 0x0304,
    BM_KYMCQUADS,
    BM_CMYKQUADS    = 0x0020,

    //
    // 32bpp - 10 bits per channel. The 2 most significant bits are ignored.
    //

    BM_10b_RGB      = 0x0009,
    BM_10b_XYZ      = 0x0401,
    BM_10b_Yxy,
    BM_10b_Lab,
    BM_10b_G3CH,

    //
    // 32bpp - named color indices (1-based)
    //

    BM_NAMED_INDEX,

    //
    // Packed 16 bits per channel => 16bpp for GRAY and
    // 48bpp for the 3 channel colors.
    //

    BM_16b_RGB      = 0x000A,
    BM_16b_XYZ      = 0x0501,
    BM_16b_Yxy,
    BM_16b_Lab,
    BM_16b_G3CH,
    BM_16b_GRAY,

    //
    // 16 bpp - 5 bits for Red & Blue, 6 bits for Green
    //

    BM_565RGB       = 0x0001,

} BMFORMAT;
typedef BMFORMAT *PBMFORMAT, *LPBMFORMAT;

//
// Callback function definition
//

typedef BOOL (WINAPI *PBMCALLBACKFN)(ULONG, ULONG, LPARAM);
typedef PBMCALLBACKFN LPBMCALLBACKFN;

//
// ICC profile header
//

typedef struct tagPROFILEHEADER {
    DWORD   phSize;             // profile size in bytes
    DWORD   phCMMType;          // CMM for this profile
    DWORD   phVersion;          // profile format version number
    DWORD   phClass;            // type of profile
    DWORD   phDataColorSpace;   // color space of data
    DWORD   phConnectionSpace;  // PCS
    DWORD   phDateTime[3];      // date profile was created
    DWORD   phSignature;        // magic number
    DWORD   phPlatform;         // primary platform
    DWORD   phProfileFlags;     // various bit settings
    DWORD   phManufacturer;     // device manufacturer
    DWORD   phModel;            // device model number
    DWORD   phAttributes[2];    // device attributes
    DWORD   phRenderingIntent;  // rendering intent
    CIEXYZ  phIlluminant;       // profile illuminant
    DWORD   phCreator;          // profile creator
    BYTE    phReserved[44];     // reserved for future use
} PROFILEHEADER;
typedef PROFILEHEADER *PPROFILEHEADER, *LPPROFILEHEADER;

//
// Profile class values
//

#define CLASS_MONITOR           'mntr'
#define CLASS_PRINTER           'prtr'
#define CLASS_SCANNER           'scnr'
#define CLASS_LINK              'link'
#define CLASS_ABSTRACT          'abst'
#define CLASS_COLORSPACE        'spac'
#define CLASS_NAMED             'nmcl'

//
// Color space values
//

#define SPACE_XYZ               'XYZ '
#define SPACE_Lab               'Lab '
#define SPACE_Luv               'Luv '
#define SPACE_YCbCr             'YCbr'
#define SPACE_Yxy               'Yxy '
#define SPACE_RGB               'RGB '
#define SPACE_GRAY              'GRAY'
#define SPACE_HSV               'HSV '
#define SPACE_HLS               'HLS '
#define SPACE_CMYK              'CMYK'
#define SPACE_CMY               'CMY '
#define SPACE_2_CHANNEL         '2CLR'
#define SPACE_3_CHANNEL         '3CLR'
#define SPACE_4_CHANNEL         '4CLR'
#define SPACE_5_CHANNEL         '5CLR'
#define SPACE_6_CHANNEL         '6CLR'
#define SPACE_7_CHANNEL         '7CLR'
#define SPACE_8_CHANNEL         '8CLR'

//
// Profile flag bitfield values
//

#define FLAG_EMBEDDEDPROFILE    0x00000001
#define FLAG_DEPENDENTONDATA    0x00000002

//
// Profile attributes bitfield values
//

#define ATTRIB_TRANSPARENCY     0x00000001
#define ATTRIB_MATTE            0x00000002

//
// Rendering Intents
//
// + INTENT_PERCEPTUAL            = LCS_GM_IMAGES for LOGCOLORSPACE
//                                = DMICM_CONTRAST for DEVMODE
//                                = "Pictures" for SetupColorMathing/Printer UI
//
// + INTENT_RELATIVE_COLORIMETRIC = LCS_GM_GRAPHICS for LOGCOLORSPACE
//                                = DMICM_COLORIMETRIC for DEVMODE
//                                = "Proof" for SetupColorMatching/Printer UI
//
// + INTENT_SATURATION            = LCS_GM_BUSINESS for LOGCOLORSPACE
//                                = DMICM_SATURATE for DEVMODE
//                                = "Graphics" for SetupColorMatching/Printer UI
//
// + INTENT_ABSOLUTE_COLORIMETRIC = LCS_GM_ABS_COLORIMETRIC for LOGCOLORSPACE
//                                = DMICM_ABS_COLORIMETRIC for DEVMODE
//                                = "Match" for SetupColorMatching/Printer UI
//

#define INTENT_PERCEPTUAL               0
#define INTENT_RELATIVE_COLORIMETRIC    1
#define INTENT_SATURATION               2
#define INTENT_ABSOLUTE_COLORIMETRIC    3

//
// Profile data structure
//

typedef struct tagPROFILE {
    DWORD   dwType;             // profile type
    PVOID   pProfileData;       // filename or buffer containing profile
    DWORD   cbDataSize;         // size of profile data
} PROFILE;
typedef PROFILE *PPROFILE, *LPPROFILE;


//
// Profile types to be used in the PROFILE structure
//

#define PROFILE_FILENAME    1   // profile data is NULL terminated filename
#define PROFILE_MEMBUFFER   2   // profile data is a buffer containing
                                // the profile
//
// Desired access mode for opening profiles
//

#define PROFILE_READ        1   // opened for read access
#define PROFILE_READWRITE   2   // opened for read and write access

//
// Handles returned to applications
//

typedef HANDLE HPROFILE;        // handle to profile object
typedef HPROFILE *PHPROFILE;
typedef HANDLE HTRANSFORM;      // handle to color transform object

//
// CMM selection for CreateMultiProfileTransform and SelectCMM.
//

#define INDEX_DONT_CARE     0

#define CMM_FROM_PROFILE    INDEX_DONT_CARE // Use CMM specified in profile.
#define CMM_WINDOWS_DEFAULT 'Win '          // Use Windows default CMM always.

//
// Tags found in ICC profiles
//

typedef DWORD      TAGTYPE;
typedef TAGTYPE   *PTAGTYPE, *LPTAGTYPE;

//
// Profile enumeration data structure
//

#define ENUM_TYPE_VERSION    0x0300

typedef struct tagENUMTYPEA {
    DWORD   dwSize;             // structure size
    DWORD   dwVersion;          // structure version
    DWORD   dwFields;           // bit fields
    PCSTR   pDeviceName;        // device friendly name
    DWORD   dwMediaType;        // media type
    DWORD   dwDitheringMode;    // dithering mode
    DWORD   dwResolution[2];    // x and y resolutions
    DWORD   dwCMMType;          // cmm ID
    DWORD   dwClass;            // profile class
    DWORD   dwDataColorSpace;   // color space of data
    DWORD   dwConnectionSpace;  // pcs
    DWORD   dwSignature;        // magic number
    DWORD   dwPlatform;         // primary platform
    DWORD   dwProfileFlags;     // various bit settings in profile
    DWORD   dwManufacturer;     // manufacturer ID
    DWORD   dwModel;            // model ID
    DWORD   dwAttributes[2];    // device attributes
    DWORD   dwRenderingIntent;  // rendering intent
    DWORD   dwCreator;          // profile creator
    DWORD   dwDeviceClass;      // device class
} ENUMTYPEA, *PENUMTYPEA, *LPENUMTYPEA;


typedef struct tagENUMTYPEW {
    DWORD   dwSize;             // structure size
    DWORD   dwVersion;          // structure version
    DWORD   dwFields;           // bit fields
    PCWSTR  pDeviceName;        // device friendly name
    DWORD   dwMediaType;        // media type
    DWORD   dwDitheringMode;    // dithering mode
    DWORD   dwResolution[2];    // x and y resolutions
    DWORD   dwCMMType;          // cmm ID
    DWORD   dwClass;            // profile class
    DWORD   dwDataColorSpace;   // color space of data
    DWORD   dwConnectionSpace;  // pcs
    DWORD   dwSignature;        // magic number
    DWORD   dwPlatform;         // primary platform
    DWORD   dwProfileFlags;     // various bit settings in profile
    DWORD   dwManufacturer;     // manufacturer ID
    DWORD   dwModel;            // model ID
    DWORD   dwAttributes[2];    // device attributes
    DWORD   dwRenderingIntent;  // rendering intent
    DWORD   dwCreator;          // profile creator
    DWORD   dwDeviceClass;      // device class
} ENUMTYPEW, *PENUMTYPEW, *LPENUMTYPEW;

//
// Bitfields for enumeration record above
//

#define ET_DEVICENAME           0x00000001
#define ET_MEDIATYPE            0x00000002
#define ET_DITHERMODE           0x00000004
#define ET_RESOLUTION           0x00000008
#define ET_CMMTYPE              0x00000010
#define ET_CLASS                0x00000020
#define ET_DATACOLORSPACE       0x00000040
#define ET_CONNECTIONSPACE      0x00000080
#define ET_SIGNATURE            0x00000100
#define ET_PLATFORM             0x00000200
#define ET_PROFILEFLAGS         0x00000400
#define ET_MANUFACTURER         0x00000800
#define ET_MODEL                0x00001000
#define ET_ATTRIBUTES           0x00002000
#define ET_RENDERINGINTENT      0x00004000
#define ET_CREATOR              0x00008000
#define ET_DEVICECLASS          0x00010000

//
// Flags for creating color transforms
//

#define PROOF_MODE                  0x00000001
#define NORMAL_MODE                 0x00000002
#define BEST_MODE                   0x00000003
#define ENABLE_GAMUT_CHECKING       0x00010000
#define USE_RELATIVE_COLORIMETRIC   0x00020000
#define FAST_TRANSLATE              0x00040000
#define RESERVED                    0x80000000

//
// Paremeter for GetPS2ColorSpaceArray
//

#define CSA_A                   1
#define CSA_ABC                 2
#define CSA_DEF                 3
#define CSA_DEFG                4
#define CSA_GRAY                5
#define CSA_RGB                 6
#define CSA_CMYK                7
#define CSA_Lab                 8

//
// Parameter for CMGetInfo()
//

#define CMM_WIN_VERSION     0
#define CMM_IDENT           1
#define CMM_DRIVER_VERSION  2
#define CMM_DLL_VERSION     3
#define CMM_VERSION         4
#define CMM_DESCRIPTION     5
#define CMM_LOGOICON        6

//
// Parameter for CMTranslateRGBs()
//

#define CMS_FORWARD         0
#define CMS_BACKWARD        1

//
//  Constants for SetupColorMatching()
//

#define COLOR_MATCH_VERSION  0x0200

//
//  Constants for flags
//

#define CMS_DISABLEICM          1     // Disable color matching
#define CMS_ENABLEPROOFING      2     // Enable proofing

#define CMS_SETRENDERINTENT     4     // Use passed in value
#define CMS_SETPROOFINTENT      8
#define CMS_SETMONITORPROFILE   0x10  // Use passed in profile name initially
#define CMS_SETPRINTERPROFILE   0x20
#define CMS_SETTARGETPROFILE    0x40

#define CMS_USEHOOK             0x80  // Use hook procedure in lpfnHook
#define CMS_USEAPPLYCALLBACK    0x100 // Use callback procedure when applied
#define CMS_USEDESCRIPTION      0x200 // Use profile description in UI
                                      //   (default is filename)

#define CMS_DISABLEINTENT       0x400 // Disable intent selection (render & proofing) always
#define CMS_DISABLERENDERINTENT 0x800 // Disable rendering intent selection while in proofing mode
                                      // Only proofing intent selection is enabled.

//
//  Used to denote too-small buffers (output only)
//

#define CMS_MONITOROVERFLOW     0x80000000L
#define CMS_PRINTEROVERFLOW     0x40000000L
#define CMS_TARGETOVERFLOW      0x20000000L

//
//  Structures (both ANSI and Unicode)
//
struct _tagCOLORMATCHSETUPW;
struct _tagCOLORMATCHSETUPA;

typedef BOOL (WINAPI *PCMSCALLBACKW)(struct _tagCOLORMATCHSETUPW *,LPARAM);
typedef BOOL (WINAPI *PCMSCALLBACKA)(struct _tagCOLORMATCHSETUPA *,LPARAM);

typedef struct _tagCOLORMATCHSETUPW {

    DWORD   dwSize;                 //  Size of structure in bytes
    DWORD   dwVersion;              //  Set to COLOR_MATCH_VERSION

    DWORD   dwFlags;                //  See constants listed previously
    HWND    hwndOwner;              //  Window handle of owner

    PCWSTR  pSourceName;            //  Name of Image Source, defaults to "sRGB Color Space"
    PCWSTR  pDisplayName;           //  If null, defaults to first enumerated monitor
    PCWSTR  pPrinterName;           //  If null, defaults to default printer.

    DWORD   dwRenderIntent;         //  Rendering Intent
    DWORD   dwProofingIntent;       //  Rendering Intent for Proofing

    PWSTR   pMonitorProfile;        //  Monitor profile name
    DWORD   ccMonitorProfile;       //  Size of above in characters

    PWSTR   pPrinterProfile;        //  Printer profile name
    DWORD   ccPrinterProfile;       //  Size of above in characters

    PWSTR   pTargetProfile;         //  Target profile name
    DWORD   ccTargetProfile;        //  Size of above in characters

    DLGPROC lpfnHook;               //  Hook Procedure address
    LPARAM  lParam;                 //  Given to hook procedure at WM_INITDIALOG

    PCMSCALLBACKW lpfnApplyCallback;   //  Callback Procedure address when apply is pushed
    LPARAM        lParamApplyCallback; //  Given to callback Procedure for apply

}   COLORMATCHSETUPW, *PCOLORMATCHSETUPW, *LPCOLORMATCHSETUPW;

typedef struct _tagCOLORMATCHSETUPA {

    DWORD   dwSize;                 //  Size of structure in bytes
    DWORD   dwVersion;              //  Set to COLOR_MATCH_VERSION

    DWORD   dwFlags;                //  See constants listed previously
    HWND    hwndOwner;              //  Window handle of owner

    PCSTR   pSourceName;            //  Name of Image Source, defaults to  "This Document"
    PCSTR   pDisplayName;           //  If null, defaults to first enumerated monitor
    PCSTR   pPrinterName;           //  If null, defaults to default printer.

    DWORD   dwRenderIntent;         //  Rendering Intent
    DWORD   dwProofingIntent;       //  Rendering Intent for Proofing

    PSTR    pMonitorProfile;        //  Monitor profile name
    DWORD   ccMonitorProfile;       //  Size of above in characters

    PSTR    pPrinterProfile;        //  Printer profile name
    DWORD   ccPrinterProfile;       //  Size of above in characters

    PSTR    pTargetProfile;         //  Target profile name
    DWORD   ccTargetProfile;        //  Size of above in characters

    DLGPROC lpfnHook;               //  Hook Procedure address
    LPARAM  lParam;                 //  Given to hook procedure at WM_INITDIALOG

    PCMSCALLBACKA lpfnApplyCallback;   //  Callback Procedure address when apply is pushed
    LPARAM        lParamApplyCallback; //  Given to callback Procedure for apply

}   COLORMATCHSETUPA, *PCOLORMATCHSETUPA, *LPCOLORMATCHSETUPA;

//
// Windows API definitions
//

HPROFILE   WINAPI OpenColorProfileA(PPROFILE, DWORD, DWORD, DWORD);
HPROFILE   WINAPI OpenColorProfileW(PPROFILE, DWORD, DWORD, DWORD);
BOOL       WINAPI CloseColorProfile(HPROFILE);
BOOL       WINAPI GetColorProfileFromHandle(HPROFILE, PBYTE, PDWORD);
BOOL       WINAPI IsColorProfileValid(HPROFILE, PBOOL);
BOOL       WINAPI CreateProfileFromLogColorSpaceA(LPLOGCOLORSPACEA, PBYTE*);
BOOL       WINAPI CreateProfileFromLogColorSpaceW(LPLOGCOLORSPACEW, PBYTE*);
BOOL       WINAPI GetCountColorProfileElements(HPROFILE, PDWORD);
BOOL       WINAPI GetColorProfileHeader(HPROFILE, PPROFILEHEADER);
BOOL       WINAPI GetColorProfileElementTag(HPROFILE, DWORD, PTAGTYPE);
BOOL       WINAPI IsColorProfileTagPresent(HPROFILE, TAGTYPE, PBOOL);
BOOL       WINAPI GetColorProfileElement(HPROFILE, TAGTYPE, DWORD, PDWORD, PVOID, PBOOL);
BOOL       WINAPI SetColorProfileHeader(HPROFILE, PPROFILEHEADER);
BOOL       WINAPI SetColorProfileElementSize(HPROFILE, TAGTYPE, DWORD);
BOOL       WINAPI SetColorProfileElement(HPROFILE, TAGTYPE, DWORD, PDWORD, PVOID);
BOOL       WINAPI SetColorProfileElementReference(HPROFILE, TAGTYPE, TAGTYPE);
BOOL       WINAPI GetPS2ColorSpaceArray (HPROFILE, DWORD, DWORD, PBYTE, PDWORD, PBOOL);
BOOL       WINAPI GetPS2ColorRenderingIntent(HPROFILE, DWORD, PBYTE, PDWORD);
BOOL       WINAPI GetPS2ColorRenderingDictionary(HPROFILE, DWORD, PBYTE, PDWORD, PBOOL);
BOOL       WINAPI GetNamedProfileInfo(HPROFILE, PNAMED_PROFILE_INFO);
BOOL       WINAPI ConvertColorNameToIndex(HPROFILE, PCOLOR_NAME, PDWORD, DWORD);
BOOL       WINAPI ConvertIndexToColorName(HPROFILE, PDWORD, PCOLOR_NAME, DWORD);
BOOL       WINAPI CreateDeviceLinkProfile(PHPROFILE, DWORD, PDWORD, DWORD, DWORD, PBYTE*, DWORD);
HTRANSFORM WINAPI CreateColorTransformA(LPLOGCOLORSPACEA, HPROFILE, HPROFILE, DWORD);
HTRANSFORM WINAPI CreateColorTransformW(LPLOGCOLORSPACEW, HPROFILE, HPROFILE, DWORD);
HTRANSFORM WINAPI CreateMultiProfileTransform(PHPROFILE, DWORD, PDWORD, DWORD, DWORD, DWORD);
BOOL       WINAPI DeleteColorTransform(HTRANSFORM);
BOOL       WINAPI TranslateBitmapBits(HTRANSFORM, PVOID, BMFORMAT, DWORD, DWORD, DWORD, PVOID, BMFORMAT, DWORD, PBMCALLBACKFN, LPARAM);
BOOL       WINAPI CheckBitmapBits(HTRANSFORM , PVOID, BMFORMAT, DWORD, DWORD, DWORD, PBYTE, PBMCALLBACKFN, LPARAM);
BOOL       WINAPI TranslateColors(HTRANSFORM, PCOLOR, DWORD, COLORTYPE, PCOLOR, COLORTYPE);
BOOL       WINAPI CheckColors(HTRANSFORM, PCOLOR, DWORD, COLORTYPE, PBYTE);
DWORD      WINAPI GetCMMInfo(HTRANSFORM, DWORD);
BOOL       WINAPI RegisterCMMA(PCSTR, DWORD, PCSTR);
BOOL       WINAPI RegisterCMMW(PCWSTR, DWORD, PCWSTR);
BOOL       WINAPI UnregisterCMMA(PCSTR, DWORD);
BOOL       WINAPI UnregisterCMMW(PCWSTR, DWORD);
BOOL       WINAPI SelectCMM(DWORD);
BOOL       WINAPI GetColorDirectoryA(PCSTR, PSTR, PDWORD);
BOOL       WINAPI GetColorDirectoryW(PCWSTR, PWSTR, PDWORD);
BOOL       WINAPI InstallColorProfileA(PCSTR, PCSTR);
BOOL       WINAPI InstallColorProfileW(PCWSTR, PCWSTR);
BOOL       WINAPI UninstallColorProfileA(PCSTR, PCSTR, BOOL);
BOOL       WINAPI UninstallColorProfileW(PCWSTR, PCWSTR, BOOL);
BOOL       WINAPI EnumColorProfilesA(PCSTR, PENUMTYPEA, PBYTE, PDWORD, PDWORD);
BOOL       WINAPI EnumColorProfilesW(PCWSTR, PENUMTYPEW, PBYTE, PDWORD, PDWORD);
BOOL       WINAPI SetStandardColorSpaceProfileA(PCSTR, DWORD, PCSTR);
BOOL       WINAPI SetStandardColorSpaceProfileW(PCWSTR, DWORD, PCWSTR);
BOOL       WINAPI GetStandardColorSpaceProfileA(PCSTR, DWORD, PSTR, PDWORD);
BOOL       WINAPI GetStandardColorSpaceProfileW(PCWSTR, DWORD, PWSTR, PDWORD);
BOOL       WINAPI AssociateColorProfileWithDeviceA(PCSTR, PCSTR, PCSTR);
BOOL       WINAPI AssociateColorProfileWithDeviceW(PCWSTR, PCWSTR, PCWSTR);
BOOL       WINAPI DisassociateColorProfileFromDeviceA(PCSTR, PCSTR, PCSTR);
BOOL       WINAPI DisassociateColorProfileFromDeviceW(PCWSTR, PCWSTR, PCWSTR);
BOOL       WINAPI SetupColorMatchingW(PCOLORMATCHSETUPW pcms);
BOOL       WINAPI SetupColorMatchingA(PCOLORMATCHSETUPA pcms);

#ifdef UNICODE

#define ENUMTYPE                            ENUMTYPEW
#define PENUMTYPE                           PENUMTYPEW
#define COLORMATCHSETUP                     COLORMATCHSETUPW
#define PCOLORMATCHSETUP                    PCOLORMATCHSETUPW
#define LPCOLORMATCHSETUP                   LPCOLORMATCHSETUPW
#define PCMSCALLBACK                        PCMSCALLBACKW
#define CreateColorTransform                CreateColorTransformW
#define OpenColorProfile                    OpenColorProfileW
#define CreateProfileFromLogColorSpace      CreateProfileFromLogColorSpaceW
#define RegisterCMM                         RegisterCMMW
#define UnregisterCMM                       UnregisterCMMW
#define GetColorDirectory                   GetColorDirectoryW
#define InstallColorProfile                 InstallColorProfileW
#define UninstallColorProfile               UninstallColorProfileW
#define AssociateColorProfileWithDevice     AssociateColorProfileWithDeviceW
#define DisassociateColorProfileFromDevice  DisassociateColorProfileFromDeviceW
#define EnumColorProfiles                   EnumColorProfilesW
#define SetStandardColorSpaceProfile        SetStandardColorSpaceProfileW
#define GetStandardColorSpaceProfile        GetStandardColorSpaceProfileW
#define SetupColorMatching                  SetupColorMatchingW

#else

#define ENUMTYPE                            ENUMTYPEA
#define PENUMTYPE                           PENUMTYPEA
#define COLORMATCHSETUP                     COLORMATCHSETUPA
#define PCOLORMATCHSETUP                    PCOLORMATCHSETUPA
#define LPCOLORMATCHSETUP                   LPCOLORMATCHSETUPA
#define PCMSCALLBACK                        PCMSCALLBACKA
#define CreateColorTransform                CreateColorTransformA
#define OpenColorProfile                    OpenColorProfileA
#define CreateProfileFromLogColorSpace      CreateProfileFromLogColorSpaceA
#define RegisterCMM                         RegisterCMMA
#define UnregisterCMM                       UnregisterCMMA
#define GetColorDirectory                   GetColorDirectoryA
#define InstallColorProfile                 InstallColorProfileA
#define UninstallColorProfile               UninstallColorProfileA
#define AssociateColorProfileWithDevice     AssociateColorProfileWithDeviceA
#define DisassociateColorProfileFromDevice  DisassociateColorProfileFromDeviceA
#define EnumColorProfiles                   EnumColorProfilesA
#define SetStandardColorSpaceProfile        SetStandardColorSpaceProfileA
#define GetStandardColorSpaceProfile        GetStandardColorSpaceProfileA
#define SetupColorMatching                  SetupColorMatchingA

#endif  // !UNICODE

//
// Transform returned by CMM
//

typedef HANDLE  HCMTRANSFORM;

//
// Pointer to ICC color profile data.
//

typedef PVOID   LPDEVCHARACTER;

//
// CMM API definition
//

BOOL WINAPI CMCheckColors(
    HCMTRANSFORM hcmTransform,  // transform handle
    LPCOLOR lpaInputColors,     // array of COLORs
    DWORD nColors,              // COLOR array size
    COLORTYPE ctInput,          // input color type
    LPBYTE lpaResult            // buffer for results
);

BOOL WINAPI CMCheckColorsInGamut(
    HCMTRANSFORM hcmTransform,  // transform handle
    RGBTRIPLE *lpaRGBTriple,    // RGB triple array
    LPBYTE lpaResult,           // buffer for results
    UINT nCount                 // result buffer size
);

BOOL WINAPI CMCheckRGBs(
    HCMTRANSFORM hcmTransform,  // transform handle
    LPVOID lpSrcBits,           // source bitmap bits
    BMFORMAT bmInput,           // source bitmap format
    DWORD dwWidth,              // source bitmap width
    DWORD dwHeight,             // source bitmap hight
    DWORD dwStride,             // source bitmap delta
    LPBYTE lpaResult,           // buffer for results
    PBMCALLBACKFN pfnCallback,  // pointer to callback function
    LPARAM ulCallbackData       // caller-defined parameter to callback
);

BOOL WINAPI CMConvertColorNameToIndex(
    HPROFILE hProfile,
    PCOLOR_NAME paColorName,
    PDWORD paIndex,
    DWORD dwCount
);

BOOL WINAPI CMConvertIndexToColorName(
    HPROFILE hProfile,
    PDWORD paIndex,
    PCOLOR_NAME paColorName,
    DWORD dwCount
);

BOOL WINAPI CMCreateDeviceLinkProfile(
    PHPROFILE pahProfiles,    // array of profile handles
    DWORD nProfiles,          // profile handle array size
    PDWORD padwIntents,       // array of rendering intents
    DWORD nIntents,           // intent array size
    DWORD dwFlags,            // transform creation flags
    LPBYTE *lpProfileData     // pointer to pointer to buffer
);

HCMTRANSFORM WINAPI CMCreateMultiProfileTransform(
    PHPROFILE pahProfiles,    // array of profile handles
    DWORD nProfiles,          // profile handle array size
    PDWORD padwIntents,       // array of rendering intents
    DWORD nIntents,           // intent array size
    DWORD dwFlags             // transform creation flags
);

BOOL WINAPI CMCreateProfile(
    LPLOGCOLORSPACEA lpColorSpace,  // pointer to a logical color space
    LPDEVCHARACTER *lpProfileData   // pointer to pointer to buffer
);

BOOL WINAPI CMCreateProfileW(
    LPLOGCOLORSPACEW lpColorSpace,  // pointer to a logical color space
    LPDEVCHARACTER *lpProfileData   // pointer to pointer to buffer
);

HCMTRANSFORM WINAPI CMCreateTransform(
    LPLOGCOLORSPACEA lpColorSpace,       // pointer to logical color space
    LPDEVCHARACTER lpDevCharacter,       // profile data
    LPDEVCHARACTER lpTargetDevCharacter  // target profile data
);

HCMTRANSFORM WINAPI CMCreateTransformW(
    LPLOGCOLORSPACEW lpColorSpace,       // pointer to logical color space
    LPDEVCHARACTER lpDevCharacter,       // profile data
    LPDEVCHARACTER lpTargetDevCharacter  // target profile data
);

HCMTRANSFORM WINAPI CMCreateTransformExt(
    LPLOGCOLORSPACEA lpColorSpace,        // pointer to logical color space
    LPDEVCHARACTER lpDevCharacter,        // profile data
    LPDEVCHARACTER lpTargetDevCharacter,  // target profile data
    DWORD dwFlags                         // creation flags
);

HCMTRANSFORM WINAPI CMCreateTransformExtW(
    LPLOGCOLORSPACEW lpColorSpace,        // pointer to logical color space
    LPDEVCHARACTER lpDevCharacter,        // profile data
    LPDEVCHARACTER lpTargetDevCharacter,  // target profile data
    DWORD dwFlags                         // creation flags
);

BOOL WINAPI CMDeleteTransform(
    HCMTRANSFORM hcmTransform             // transform handle to be deleted.
);

DWORD WINAPI CMGetInfo(
    DWORD dwInfo
);

BOOL WINAPI CMGetNamedProfileInfo(
    HPROFILE hProfile,                    // profile handle
    PNAMED_PROFILE_INFO pNamedProfileInfo // pointer to named profile info
);

BOOL WINAPI CMGetPS2ColorRenderingDictionary(
    HPROFILE hProfile,
    DWORD dwIntent,
    LPBYTE lpBuffer,
    LPDWORD lpcbSize,
    LPBOOL lpbBinary
);

BOOL WINAPI CMGetPS2ColorRenderingIntent(
    HPROFILE hProfile,
    DWORD dwIntent,
    LPBYTE lpBuffer,
    LPDWORD lpcbSize
);

BOOL WINAPI CMGetPS2ColorSpaceArray(
    HPROFILE hProfile,
    DWORD dwIntent,
    DWORD dwCSAType,
    LPBYTE lpBuffer,
    LPDWORD lpcbSize,
    LPBOOL lpbBinary
);

BOOL WINAPI CMIsProfileValid(
    HPROFILE hProfile,                  // proflle handle
    LPBOOL lpbValid                     // buffer for result.
);

BOOL WINAPI CMTranslateColors(
    HCMTRANSFORM hcmTransform,          // transform handle
    LPCOLOR lpaInputColors,             // pointer to input color array
    DWORD nColors,                      // number of color in color array
    COLORTYPE ctInput,                  // input color type
    LPCOLOR lpaOutputColors,            // pointer to output color array
    COLORTYPE ctOutput                  // output color type
);

BOOL WINAPI CMTranslateRGB(
    HCMTRANSFORM hcmTransform,
    COLORREF ColorRef,
    LPCOLORREF lpColorRef,
    DWORD dwFlags
);

BOOL WINAPI CMTranslateRGBs(
    HCMTRANSFORM hcmTransform,
    LPVOID lpSrcBits,
    BMFORMAT bmInput,
    DWORD dwWidth,
    DWORD dwHeight,
    DWORD dwStride,
    LPVOID lpDestBits,
    BMFORMAT bmOutput,
    DWORD dwTranslateDirection
);

BOOL WINAPI CMTranslateRGBsExt(
    HCMTRANSFORM hcmTransform,
    LPVOID lpSrcBits,
    BMFORMAT bmInput,
    DWORD dwWidth,
    DWORD dwHeight,
    DWORD dwInputStride,
    LPVOID lpDestBits,
    BMFORMAT bmOutput,
    DWORD dwOutputStride,
    LPBMCALLBACKFN lpfnCallback,
    LPARAM ulCallbackData
);

#ifdef __cplusplus
}
#endif

#endif  // ifndef _ICM_H_

