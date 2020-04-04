/*
 * Copyright 2004 (C) Mike McCormack
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_ICM_H
#define __WINE_ICM_H

#ifdef __cplusplus
extern "C" {
#endif

typedef HANDLE HPROFILE;
typedef HPROFILE *PHPROFILE;
typedef HANDLE HTRANSFORM;

typedef DWORD TAGTYPE, *PTAGTYPE, *LPTAGTYPE;

typedef char COLOR_NAME[32];
typedef COLOR_NAME *PCOLOR_NAME, *LPCOLOR_NAME;

typedef struct tagNAMED_PROFILE_INFO
{
    DWORD       dwFlags;
    DWORD       dwCount;
    DWORD       dwCountDevCoordinates;
    COLOR_NAME  szPrefix;
    COLOR_NAME  szSuffix;
} NAMED_PROFILE_INFO, *PNAMED_PROFILE_INFO, *LPNAMED_PROFILE_INFO;

#define MAX_COLOR_CHANNELS  8

struct GRAYCOLOR
{
    WORD    gray;
};

struct RGBCOLOR
{
    WORD    red;
    WORD    green;
    WORD    blue;
};

struct CMYKCOLOR
{
    WORD    cyan;
    WORD    magenta;
    WORD    yellow;
    WORD    black;
};

struct XYZCOLOR
{
    WORD    X;
    WORD    Y;
    WORD    Z;
};

struct YxyCOLOR
{
    WORD    Y;
    WORD    x;
    WORD    y;
};

struct LabCOLOR
{
    WORD    L;
    WORD    a;
    WORD    b;
};

struct GENERIC3CHANNEL
{
    WORD    ch1;
    WORD    ch2;
    WORD    ch3;
};

struct NAMEDCOLOR
{
    DWORD   dwIndex;
};

struct HiFiCOLOR
{
    BYTE    channel[MAX_COLOR_CHANNELS];
};

typedef union tagCOLOR
{
    struct GRAYCOLOR        gray;
    struct RGBCOLOR         rgb;
    struct CMYKCOLOR        cmyk;
    struct XYZCOLOR         XYZ;
    struct YxyCOLOR         Yxy;
    struct LabCOLOR         Lab;
    struct GENERIC3CHANNEL  gen3ch;
    struct NAMEDCOLOR       named;
    struct HiFiCOLOR        hifi;
    struct
    {
        DWORD reserved1;
        VOID *reserved2;
    } DUMMYSTRUCTNAME;
} COLOR, *PCOLOR, *LPCOLOR;

typedef enum
{
    COLOR_GRAY  = 1,
    COLOR_RGB,
    COLOR_XYZ,
    COLOR_Yxy,
    COLOR_Lab,
    COLOR_3_CHANNEL,
    COLOR_CMYK,
    COLOR_5_CHANNEL,
    COLOR_6_CHANNEL,
    COLOR_7_CHANNEL,
    COLOR_8_CHANNEL,
    COLOR_NAMED,
} COLORTYPE, *PCOLORTYPE, *LPCOLORTYPE;

typedef enum
{
    BM_x555RGB     = 0x00,
    BM_565RGB      = 0x01,
    BM_RGBTRIPLETS = 0x02,
    BM_BGRTRIPLETS = 0x04,
    BM_xRGBQUADS   = 0x08,
    BM_10b_RGB     = 0x09,
    BM_16b_RGB     = 0x0a,
    BM_xBGRQUADS   = 0x10,
    BM_CMYKQUADS   = 0x20,
    BM_x555XYZ     = 0x101,
    BM_x555Yxz,
    BM_x555Lab,
    BM_x555G3CH,
    BM_XYZTRIPLETS = 0x201,
    BM_YxyTRIPLETS,
    BM_LabTRIPLETS,
    BM_G3CHTRIPLETS,
    BM_5CHANNEL,
    BM_6CHANNEL,
    BM_7CHANNEL,
    BM_8CHANNEL,
    BM_GRAY,
    BM_xXYZQUADS   = 0x301,
    BM_xYxyQUADS,
    BM_xLabQUADS,
    BM_xG3CHQUADS,
    BM_KYMCQUADS,
    BM_10b_XYZ     = 0x401,
    BM_10b_Yxy,
    BM_10b_Lab,
    BM_10b_G3CH,
    BM_NAMED_INDEX,
    BM_16b_XYZ     = 0x501,
    BM_16b_Yxy,
    BM_16b_Lab,
    BM_16b_G3CH,
    BM_16b_GRAY,
} BMFORMAT, *PBMFORMAT, *LPBMFORMAT;

typedef enum
{
    WCS_PROFILE_MANAGEMENT_SCOPE_SYSTEM_WIDE,
    WCS_PROFILE_MANAGEMENT_SCOPE_CURRENT_USER
} WCS_PROFILE_MANAGEMENT_SCOPE;

typedef BOOL (CALLBACK *PBMCALLBACKFN)(ULONG,ULONG,LPARAM);
typedef PBMCALLBACKFN LPPBMCALLBACKFN;

#define INTENT_PERCEPTUAL               0
#define INTENT_RELATIVE_COLORIMETRIC    1
#define INTENT_SATURATION               2
#define INTENT_ABSOLUTE_COLORIMETRIC    3

typedef struct tagPROFILEHEADER
{
    DWORD phSize;
    DWORD phCMMType;
    DWORD phVersion;
    DWORD phClass;
    DWORD phDataColorSpace;
    DWORD phConnectionSpace;
    DWORD phDateTime[3];
    DWORD phSignature;
    DWORD phPlatform;
    DWORD phProfileFlags;
    DWORD phManufacturer;
    DWORD phModel;
    DWORD phAttributes[2];
    DWORD phRenderingIntent;
    CIEXYZ phIlluminant;
    DWORD phCreator;
    BYTE phReserved[44];
} PROFILEHEADER, *PPROFILEHEADER, *LPPROFILEHEADER;

typedef struct tagPROFILE
{
    DWORD dwType;
    PVOID pProfileData;
    DWORD cbDataSize;
} PROFILE, *PPROFILE, *LPPROFILE;

#define ENUM_TYPE_VERSION   0x0300

typedef struct tagENUMTYPEA
{
    DWORD   dwSize;
    DWORD   dwVersion;
    DWORD   dwFields;
    PCSTR   pDeviceName;
    DWORD   dwMediaType;
    DWORD   dwDitheringMode;
    DWORD   dwResolution[2];
    DWORD   dwCMMType;
    DWORD   dwClass;
    DWORD   dwDataColorSpace;
    DWORD   dwConnectionSpace;
    DWORD   dwSignature;
    DWORD   dwPlatform;
    DWORD   dwProfileFlags;
    DWORD   dwManufacturer;
    DWORD   dwModel;
    DWORD   dwAttributes[2];
    DWORD   dwRenderingIntent;
    DWORD   dwCreator;
    DWORD   dwDeviceClass;
} ENUMTYPEA, *PENUMTYPEA, *LPENUMTYPEA;

typedef struct tagENUMTYPEW
{
    DWORD   dwSize;
    DWORD   dwVersion;
    DWORD   dwFields;
    PCWSTR  pDeviceName;
    DWORD   dwMediaType;
    DWORD   dwDitheringMode;
    DWORD   dwResolution[2];
    DWORD   dwCMMType;
    DWORD   dwClass;
    DWORD   dwDataColorSpace;
    DWORD   dwConnectionSpace;
    DWORD   dwSignature;
    DWORD   dwPlatform;
    DWORD   dwProfileFlags;
    DWORD   dwManufacturer;
    DWORD   dwModel;
    DWORD   dwAttributes[2];
    DWORD   dwRenderingIntent;
    DWORD   dwCreator;
    DWORD   dwDeviceClass;
} ENUMTYPEW, *PENUMTYPEW, *LPENUMTYPEW;

#define ET_DEVICENAME       0x00000001
#define ET_MEDIATYPE        0x00000002
#define ET_DITHERMODE       0x00000004
#define ET_RESOLUTION       0x00000008
#define ET_CMMTYPE          0x00000010
#define ET_CLASS            0x00000020
#define ET_DATACOLORSPACE   0x00000040
#define ET_CONNECTIONSPACE  0x00000080
#define ET_SIGNATURE        0x00000100
#define ET_PLATFORM         0x00000200
#define ET_PROFILEFLAGS     0x00000400
#define ET_MANUFACTURER     0x00000800
#define ET_MODEL            0x00001000
#define ET_ATTRIBUTES       0x00002000
#define ET_RENDERINGINTENT  0x00004000
#define ET_CREATOR          0x00008000
#define ET_DEVICECLASS      0x00010000

struct _tagCOLORMATCHSETUPA;
struct _tagCOLORMATCHSETUPW;

typedef BOOL (WINAPI *PCMSCALLBACKA)(struct _tagCOLORMATCHSETUPA*,LPARAM);
typedef BOOL (WINAPI *PCMSCALLBACKW)(struct _tagCOLORMATCHSETUPW*,LPARAM);

typedef struct _tagCOLORMATCHSETUPA
{
    DWORD dwSize;
    DWORD dwVersion;
    DWORD dwFlags;
    HWND  hwndOwner;
    PCSTR pSourceName;
    PCSTR pDisplayName;
    PCSTR pPrinterName;
    DWORD dwRenderIntent;
    DWORD dwProofingIntent;
    PSTR  pMonitorProfile;
    DWORD ccMonitorProfile;
    PSTR  pPrinterProfile;
    DWORD ccPrinterProfile;
    PSTR  pTargetProfile;
    DWORD ccTargetProfile;
    DLGPROC lpfnHook;
    LPARAM lParam;
    PCMSCALLBACKA lpfnApplyCallback;
    LPARAM lParamApplyCallback;
} COLORMATCHSETUPA, *PCOLORMATCHSETUPA, *LPCOLORMATCHSETUPA;

typedef struct _tagCOLORMATCHSETUPW
{
    DWORD dwSize;
    DWORD dwVersion;
    DWORD dwFlags;
    HWND  hwndOwner;
    PCWSTR pSourceName;
    PCWSTR pDisplayName;
    PCWSTR pPrinterName;
    DWORD dwRenderIntent;
    DWORD dwProofingIntent;
    PWSTR  pMonitorProfile;
    DWORD ccMonitorProfile;
    PWSTR  pPrinterProfile;
    DWORD ccPrinterProfile;
    PWSTR  pTargetProfile;
    DWORD ccTargetProfile;
    DLGPROC lpfnHook;
    LPARAM lParam;
    PCMSCALLBACKW lpfnApplyCallback;
    LPARAM lParamApplyCallback;
} COLORMATCHSETUPW, *PCOLORMATCHSETUPW, *LPCOLORMATCHSETUPW;

BOOL       WINAPI AssociateColorProfileWithDeviceA(PCSTR,PCSTR,PCSTR);
BOOL       WINAPI AssociateColorProfileWithDeviceW(PCWSTR,PCWSTR,PCWSTR);
#define    AssociateColorProfileWithDevice WINELIB_NAME_AW(AssociateColorProfileWithDevice)
BOOL       WINAPI CheckBitmapBits(HTRANSFORM,PVOID,BMFORMAT,DWORD,DWORD,DWORD,PBYTE,PBMCALLBACKFN,LPARAM);
BOOL       WINAPI CheckColors(HTRANSFORM,PCOLOR,DWORD,COLORTYPE,PBYTE);
BOOL       WINAPI ConvertColorNameToIndex(HPROFILE,PCOLOR_NAME,PDWORD,DWORD);
BOOL       WINAPI ConvertIndexToColorName(HPROFILE,PDWORD,PCOLOR_NAME,DWORD);
BOOL       WINAPI CloseColorProfile(HPROFILE);
HTRANSFORM WINAPI CreateColorTransformA(LPLOGCOLORSPACEA,HPROFILE,HPROFILE,DWORD);
HTRANSFORM WINAPI CreateColorTransformW(LPLOGCOLORSPACEW,HPROFILE,HPROFILE,DWORD);
#define    CreateColorTransform WINELIB_NAME_AW(CreateColorTransform)
BOOL       WINAPI CreateDeviceLinkProfile(PHPROFILE,DWORD,PDWORD,DWORD,DWORD,PBYTE*,DWORD);
HTRANSFORM WINAPI CreateMultiProfileTransform(PHPROFILE,DWORD,PDWORD,DWORD,DWORD,DWORD);
BOOL       WINAPI CreateProfileFromLogColorSpaceA(LPLOGCOLORSPACEA,PBYTE*);
BOOL       WINAPI CreateProfileFromLogColorSpaceW(LPLOGCOLORSPACEW,PBYTE*);
#define    CreateProfileFromLogColorSpace WINELIB_NAME_AW(CreateProfileFromLogColorSpace)
BOOL       WINAPI DeleteColorTransform(HTRANSFORM);
BOOL       WINAPI DisassociateColorProfileFromDeviceA(PCSTR,PCSTR,PCSTR);
BOOL       WINAPI DisassociateColorProfileFromDeviceW(PCWSTR,PCWSTR,PCWSTR);
#define    DisassociateColorProfileFromDevice WINELIB_NAME_AW(DisassociateColorProfileFromDevice)
BOOL       WINAPI EnumColorProfilesA(PCSTR,PENUMTYPEA,PBYTE,PDWORD,PDWORD);
BOOL       WINAPI EnumColorProfilesW(PCWSTR,PENUMTYPEW,PBYTE,PDWORD,PDWORD);
#define    EnumColorProfiles  WINELIB_NAME_AW(EnumColorProfiles)
DWORD      WINAPI GenerateCopyFilePaths(LPCWSTR,LPCWSTR,LPBYTE,DWORD,LPWSTR,LPDWORD,LPWSTR,LPDWORD,DWORD);
DWORD      WINAPI GetCMMInfo(HTRANSFORM,DWORD);
BOOL       WINAPI GetColorDirectoryA(PCSTR,PSTR,PDWORD);
BOOL       WINAPI GetColorDirectoryW(PCWSTR,PWSTR,PDWORD);
#define    GetColorDirectory WINELIB_NAME_AW(GetColorDirectory)
BOOL       WINAPI GetColorProfileElement(HPROFILE,TAGTYPE,DWORD,PDWORD,PVOID,PBOOL);
BOOL       WINAPI GetColorProfileElementTag(HPROFILE,DWORD,PTAGTYPE);
BOOL       WINAPI GetColorProfileFromHandle(HPROFILE,PBYTE,PDWORD);
BOOL       WINAPI GetColorProfileHeader(HPROFILE,PPROFILEHEADER);
BOOL       WINAPI GetCountColorProfileElements(HPROFILE,PDWORD);
BOOL       WINAPI GetNamedProfileInfo(HPROFILE,PNAMED_PROFILE_INFO);
BOOL       WINAPI GetPS2ColorRenderingDictionary(HPROFILE,DWORD,PBYTE,PDWORD,PBOOL);
BOOL       WINAPI GetPS2ColorRenderingIntent(HPROFILE,DWORD,PBYTE,PDWORD);
BOOL       WINAPI GetPS2ColorSpaceArray(HPROFILE,DWORD,DWORD,PBYTE,PDWORD,PBOOL);
BOOL       WINAPI GetStandardColorSpaceProfileA(PCSTR,DWORD,PSTR,PDWORD);
BOOL       WINAPI GetStandardColorSpaceProfileW(PCWSTR,DWORD,PWSTR,PDWORD);
#define    GetStandardColorSpaceProfile WINELIB_NAME_AW(GetStandardColorSpaceProfile)
BOOL       WINAPI InstallColorProfileA(PCSTR,PCSTR);
BOOL       WINAPI InstallColorProfileW(PCWSTR,PCWSTR);
#define    InstallColorProfile WINELIB_NAME_AW(InstallColorProfile)
BOOL       WINAPI IsColorProfileTagPresent(HPROFILE,TAGTYPE,PBOOL);
BOOL       WINAPI IsColorProfileValid(HPROFILE,PBOOL);
HPROFILE   WINAPI OpenColorProfileA(PPROFILE,DWORD,DWORD,DWORD);
HPROFILE   WINAPI OpenColorProfileW(PPROFILE,DWORD,DWORD,DWORD);
#define    OpenColorProfile WINELIB_NAME_AW(OpenColorProfile)
BOOL       WINAPI RegisterCMMA(PCSTR,DWORD,PCSTR);
BOOL       WINAPI RegisterCMMW(PCWSTR,DWORD,PCWSTR);
#define    RegisterCMM WINELIB_NAME_AW(RegisterCMM)
BOOL       WINAPI SelectCMM(DWORD id);
BOOL       WINAPI SetColorProfileElement(HPROFILE,TAGTYPE,DWORD,PDWORD,PVOID);
BOOL       WINAPI SetColorProfileElementReference(HPROFILE,TAGTYPE,TAGTYPE);
BOOL       WINAPI SetColorProfileElementSize(HPROFILE,TAGTYPE,DWORD);
BOOL       WINAPI SetColorProfileHeader(HPROFILE,PPROFILEHEADER);
BOOL       WINAPI SetStandardColorSpaceProfileA(PCSTR,DWORD,PSTR);
BOOL       WINAPI SetStandardColorSpaceProfileW(PCWSTR,DWORD,PWSTR);
#define    SetStandardColorSpaceProfile WINELIB_NAME_AW(SetStandardColorSpaceProfile)
BOOL       WINAPI SetupColorMatchingA(PCOLORMATCHSETUPA);
BOOL       WINAPI SetupColorMatchingW(PCOLORMATCHSETUPW);
#define    SetupColorMatching WINELIB_NAME_AW(SetupColorMatching)
BOOL       WINAPI SpoolerCopyFileEvent(LPWSTR,LPWSTR,DWORD);
BOOL       WINAPI TranslateBitmapBits(HTRANSFORM,PVOID,BMFORMAT,DWORD,DWORD,DWORD,PVOID,BMFORMAT,DWORD,PBMCALLBACKFN,ULONG);
BOOL       WINAPI TranslateColors(HTRANSFORM,PCOLOR,DWORD,COLORTYPE,PCOLOR,COLORTYPE);
BOOL       WINAPI UninstallColorProfileA(PCSTR,PCSTR,BOOL);
BOOL       WINAPI UninstallColorProfileW(PCWSTR,PCWSTR,BOOL);
#define    UninstallColorProfile WINELIB_NAME_AW(UninstallColorProfile)
BOOL       WINAPI UnregisterCMMA(PCSTR,DWORD);
BOOL       WINAPI UnregisterCMMW(PCWSTR,DWORD);
#define    UnregisterCMM WINELIB_NAME_AW(UnregisterCMM)
BOOL       WINAPI WcsEnumColorProfilesSize(WCS_PROFILE_MANAGEMENT_SCOPE,ENUMTYPEW*,DWORD*);
BOOL       WINAPI WcsGetUsePerUserProfiles(const WCHAR*,DWORD,BOOL*);
HPROFILE   WINAPI WcsOpenColorProfileA(PROFILE*,PROFILE*,PROFILE*,DWORD,DWORD,DWORD,DWORD);
HPROFILE   WINAPI WcsOpenColorProfileW(PROFILE*,PROFILE*,PROFILE*,DWORD,DWORD,DWORD,DWORD);

#define PROFILE_FILENAME    1
#define PROFILE_MEMBUFFER   2

#define PROFILE_READ        1
#define PROFILE_READWRITE   2

#define SPACE_XYZ   0x58595A20   /* 'XYZ ' */
#define SPACE_Lab   0x4C616220   /* 'Lab ' */
#define SPACE_Luv   0x4C757620   /* 'Luv ' */
#define SPACE_YCbCr 0x59436272   /* 'YCbr' */
#define SPACE_Yxy   0x59787920   /* 'Yxy ' */
#define SPACE_RGB   0x52474220   /* 'RGB ' */
#define SPACE_GRAY  0x47524159   /* 'GRAY' */
#define SPACE_HSV   0x48535620   /* 'HSV ' */
#define SPACE_HLS   0x484C5320   /* 'HLS ' */
#define SPACE_CMYK  0x434D594B   /* 'CMYK' */
#define SPACE_CMY   0x434D5920   /* 'CMY ' */
#define SPACE_2_CHANNEL 0x32434c52   /* '2CLR' */
#define SPACE_3_CHANNEL 0x33434c52   /* '3CLR' */
#define SPACE_4_CHANNEL 0x34434c52   /* '4CLR' */
#define SPACE_5_CHANNEL 0x35434c52   /* '5CLR' */
#define SPACE_6_CHANNEL 0x36434c52   /* '6CLR' */
#define SPACE_7_CHANNEL 0x37434c52   /* '7CLR' */
#define SPACE_8_CHANNEL 0x38434c52   /* '8CLR' */

#ifdef __cplusplus
}
#endif

#endif /* __WINE_ICM_H */
