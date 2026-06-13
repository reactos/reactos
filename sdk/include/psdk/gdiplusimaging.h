/*
 * Copyright (C) 2007 Google (Evan Stade)
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

#ifndef _GDIPLUSIMAGING_H
#define _GDIPLUSIMAGING_H

DEFINE_GUID(ImageFormatUndefined, 0xb96b3ca9, 0x728, 0x11d3, 0x9d, 0x7b, 0, 0, 0xf8, 0x1e, 0xf3, 0x2e);
DEFINE_GUID(ImageFormatMemoryBMP, 0xb96b3caa, 0x728, 0x11d3, 0x9d, 0x7b, 0, 0, 0xf8, 0x1e, 0xf3, 0x2e);
DEFINE_GUID(ImageFormatBMP,       0xb96b3cab, 0x728, 0x11d3, 0x9d, 0x7b, 0, 0, 0xf8, 0x1e, 0xf3, 0x2e);
DEFINE_GUID(ImageFormatEMF,       0xb96b3cac, 0x728, 0x11d3, 0x9d, 0x7b, 0, 0, 0xf8, 0x1e, 0xf3, 0x2e);
DEFINE_GUID(ImageFormatWMF,       0xb96b3cad, 0x728, 0x11d3, 0x9d, 0x7b, 0, 0, 0xf8, 0x1e, 0xf3, 0x2e);
DEFINE_GUID(ImageFormatJPEG,      0xb96b3cae, 0x728, 0x11d3, 0x9d, 0x7b, 0, 0, 0xf8, 0x1e, 0xf3, 0x2e);
DEFINE_GUID(ImageFormatPNG,       0xb96b3caf, 0x728, 0x11d3, 0x9d, 0x7b, 0, 0, 0xf8, 0x1e, 0xf3, 0x2e);
DEFINE_GUID(ImageFormatGIF,       0xb96b3cb0, 0x728, 0x11d3, 0x9d, 0x7b, 0, 0, 0xf8, 0x1e, 0xf3, 0x2e);
DEFINE_GUID(ImageFormatTIFF,      0xb96b3cb1, 0x728, 0x11d3, 0x9d, 0x7b, 0, 0, 0xf8, 0x1e, 0xf3, 0x2e);
DEFINE_GUID(ImageFormatEXIF,      0xb96b3cb2, 0x728, 0x11d3, 0x9d, 0x7b, 0, 0, 0xf8, 0x1e, 0xf3, 0x2e);
DEFINE_GUID(ImageFormatIcon,      0xb96b3cb5, 0x728, 0x11d3, 0x9d, 0x7b, 0, 0, 0xf8, 0x1e, 0xf3, 0x2e);

DEFINE_GUID(FrameDimensionTime,       0x6aedbd6d, 0x3fb5, 0x418a, 0x83, 0xa6, 0x7f, 0x45, 0x22, 0x9d, 0xc8, 0x72);
DEFINE_GUID(FrameDimensionPage,       0x7462dc86, 0x6180, 0x4c7e, 0x8e, 0x3f, 0xee, 0x73, 0x33, 0xa7, 0xa4, 0x83);
DEFINE_GUID(FrameDimensionResolution, 0x84236f7b, 0x3bd3, 0x428f, 0x8d, 0xab, 0x4e, 0xa1, 0x43, 0x9c, 0xa3, 0x15);

DEFINE_GUID(EncoderSaveFlag, 0x292266fc, 0xac40, 0x47bf, 0x8c, 0xfc, 0xa8, 0x5b, 0x89, 0xa6, 0x55, 0xde);

enum ImageLockMode
{
    ImageLockModeRead           = 1,
    ImageLockModeWrite          = 2,
    ImageLockModeUserInputBuf   = 4
};

enum RotateFlipType
{
    RotateNoneFlipNone = 0,
    Rotate180FlipXY    = RotateNoneFlipNone,

    Rotate90FlipNone   = 1,
    Rotate270FlipXY    = Rotate90FlipNone,

    Rotate180FlipNone  = 2,
    RotateNoneFlipXY   = Rotate180FlipNone,

    Rotate270FlipNone  = 3,
    Rotate90FlipXY     = Rotate270FlipNone,

    RotateNoneFlipX    = 4,
    Rotate180FlipY     = RotateNoneFlipX,

    Rotate90FlipX      = 5,
    Rotate270FlipY     = Rotate90FlipX,

    Rotate180FlipX     = 6,
    RotateNoneFlipY    = Rotate180FlipX,

    Rotate270FlipX     = 7,
    Rotate90FlipY      = Rotate270FlipX
};

#ifdef __cplusplus
class EncoderParameter
{
public:
    GUID    Guid;
    ULONG   NumberOfValues;
    ULONG   Type;
    VOID*   Value;
};

class EncoderParameters
{
public:
    UINT Count;
    EncoderParameter Parameter[1];
};

class ImageCodecInfo
{
public:
    CLSID Clsid;
    GUID  FormatID;
    const WCHAR* CodecName;
    const WCHAR* DllName;
    const WCHAR* FormatDescription;
    const WCHAR* FilenameExtension;
    const WCHAR* MimeType;
    DWORD Flags;
    DWORD Version;
    DWORD SigCount;
    DWORD SigSize;
    const BYTE* SigPattern;
    const BYTE* SigMask;
};

class BitmapData
{
public:
    UINT Width;
    UINT Height;
    INT Stride;
    Gdiplus::PixelFormat PixelFormat;
    VOID* Scan0;
    UINT_PTR Reserved;
};

class ImageItemData
{
public:
    UINT  Size;
    UINT  Position;
    VOID* Desc;
    UINT  DescSize;
    VOID* Data;
    UINT  DataSize;
    UINT  Cookie;
};

class PropertyItem
{
public:
    PROPID id;
    ULONG  length;
    WORD   type;
    VOID*  value;
};

#else /* end of c++ typedefs */

typedef enum ImageLockMode ImageLockMode;
typedef enum RotateFlipType RotateFlipType;

typedef struct EncoderParameter
{
    GUID Guid;
    ULONG NumberOfValues;
    ULONG Type;
    VOID* Value;
} EncoderParameter;

typedef struct EncoderParameters
{
    UINT Count;
    EncoderParameter Parameter[1];
} EncoderParameters;

typedef struct ImageCodecInfo
{
    CLSID Clsid;
    GUID  FormatID;
    const WCHAR* CodecName;
    const WCHAR* DllName;
    const WCHAR* FormatDescription;
    const WCHAR* FilenameExtension;
    const WCHAR* MimeType;
    DWORD Flags;
    DWORD Version;
    DWORD SigCount;
    DWORD SigSize;
    const BYTE* SigPattern;
    const BYTE* SigMask;
} ImageCodecInfo;

typedef struct BitmapData
{
    UINT Width;
    UINT Height;
    INT Stride;
    PixelFormat PixelFormat;
    VOID* Scan0;
    UINT_PTR Reserved; /* undocumented: stores the lock mode */
} BitmapData;

typedef struct ImageItemData
{
    UINT  Size;
    UINT  Position;
    VOID* Desc;
    UINT  DescSize;
    VOID* Data;
    UINT  DataSize;
    UINT  Cookie;
} ImageItemData;

typedef struct PropertyItem
{
    PROPID id;
    ULONG  length;
    WORD   type;
    VOID*  value;
} PropertyItem;

#endif /* end of c typedefs */

/* property types */
#define PropertyTagTypeByte       1
#define PropertyTagTypeASCII      2
#define PropertyTagTypeShort      3
#define PropertyTagTypeLong       4
#define PropertyTagTypeRational   5
#define PropertyTagTypeUndefined  7
#define PropertyTagTypeSLONG      9
#define PropertyTagTypeSRational 10

/* property IDs */
#define PropertyTagExifIFD                0x8769
#define PropertyTagGpsIFD                 0x8825

#define PropertyTagNewSubfileType         0x00FE
#define PropertyTagSubfileType            0x00FF
#define PropertyTagImageWidth             0x0100
#define PropertyTagImageHeight            0x0101
#define PropertyTagBitsPerSample          0x0102
#define PropertyTagCompression            0x0103
#define PropertyTagPhotometricInterp      0x0106
#define PropertyTagThreshHolding          0x0107
#define PropertyTagCellWidth              0x0108
#define PropertyTagCellHeight             0x0109
#define PropertyTagFillOrder              0x010A
#define PropertyTagDocumentName           0x010D
#define PropertyTagImageDescription       0x010E
#define PropertyTagEquipMake              0x010F
#define PropertyTagEquipModel             0x0110
#define PropertyTagStripOffsets           0x0111
#define PropertyTagOrientation            0x0112
#define PropertyTagSamplesPerPixel        0x0115
#define PropertyTagRowsPerStrip           0x0116
#define PropertyTagStripBytesCount        0x0117
#define PropertyTagMinSampleValue         0x0118
#define PropertyTagMaxSampleValue         0x0119
#define PropertyTagXResolution            0x011A
#define PropertyTagYResolution            0x011B
#define PropertyTagPlanarConfig           0x011C
#define PropertyTagPageName               0x011D
#define PropertyTagXPosition              0x011E
#define PropertyTagYPosition              0x011F
#define PropertyTagFreeOffset             0x0120
#define PropertyTagFreeByteCounts         0x0121
#define PropertyTagGrayResponseUnit       0x0122
#define PropertyTagGrayResponseCurve      0x0123
#define PropertyTagT4Option               0x0124
#define PropertyTagT6Option               0x0125
#define PropertyTagResolutionUnit         0x0128
#define PropertyTagPageNumber             0x0129
#define PropertyTagTransferFuncition      0x012D
#define PropertyTagSoftwareUsed           0x0131
#define PropertyTagDateTime               0x0132
#define PropertyTagArtist                 0x013B
#define PropertyTagHostComputer           0x013C
#define PropertyTagPredictor              0x013D
#define PropertyTagWhitePoint             0x013E
#define PropertyTagPrimaryChromaticities  0x013F
#define PropertyTagColorMap               0x0140
#define PropertyTagHalftoneHints          0x0141
#define PropertyTagTileWidth              0x0142
#define PropertyTagTileLength             0x0143
#define PropertyTagTileOffset             0x0144
#define PropertyTagTileByteCounts         0x0145
#define PropertyTagInkSet                 0x014C
#define PropertyTagInkNames               0x014D
#define PropertyTagNumberOfInks           0x014E
#define PropertyTagDotRange               0x0150
#define PropertyTagTargetPrinter          0x0151
#define PropertyTagExtraSamples           0x0152
#define PropertyTagSampleFormat           0x0153
#define PropertyTagSMinSampleValue        0x0154
#define PropertyTagSMaxSampleValue        0x0155
#define PropertyTagTransferRange          0x0156

#define PropertyTagJPEGProc               0x0200
#define PropertyTagJPEGInterFormat        0x0201
#define PropertyTagJPEGInterLength        0x0202
#define PropertyTagJPEGRestartInterval    0x0203
#define PropertyTagJPEGLosslessPredictors 0x0205
#define PropertyTagJPEGPointTransforms    0x0206
#define PropertyTagJPEGQTables            0x0207
#define PropertyTagJPEGDCTables           0x0208
#define PropertyTagJPEGACTables           0x0209

#define PropertyTagYCbCrCoefficients      0x0211
#define PropertyTagYCbCrSubsampling       0x0212
#define PropertyTagYCbCrPositioning       0x0213
#define PropertyTagREFBlackWhite          0x0214

#define PropertyTagICCProfile          0x8773

#define PropertyTagGamma                0x0301
#define PropertyTagICCProfileDescriptor 0x0302
#define PropertyTagSRGBRenderingIntent  0x0303

#define PropertyTagImageTitle          0x0320
#define PropertyTagCopyright           0x8298

#define PropertyTagResolutionXUnit            0x5001
#define PropertyTagResolutionYUnit            0x5002
#define PropertyTagResolutionXLengthUnit      0x5003
#define PropertyTagResolutionYLengthUnit      0x5004
#define PropertyTagPrintFlags                 0x5005
#define PropertyTagPrintFlagsVersion          0x5006
#define PropertyTagPrintFlagsCrop             0x5007
#define PropertyTagPrintFlagsBleedWidth       0x5008
#define PropertyTagPrintFlagsBleedWidthScale  0x5009
#define PropertyTagHalftoneLPI                0x500A
#define PropertyTagHalftoneLPIUnit            0x500B
#define PropertyTagHalftoneDegree             0x500C
#define PropertyTagHalftoneShape              0x500D
#define PropertyTagHalftoneMisc               0x500E
#define PropertyTagHalftoneScreen             0x500F
#define PropertyTagJPEGQuality                0x5010
#define PropertyTagGridSize                   0x5011
#define PropertyTagThumbnailFormat            0x5012
#define PropertyTagThumbnailWidth             0x5013
#define PropertyTagThumbnailHeight            0x5014
#define PropertyTagThumbnailColorDepth        0x5015
#define PropertyTagThumbnailPlanes            0x5016
#define PropertyTagThumbnailRawBytes          0x5017
#define PropertyTagThumbnailSize              0x5018
#define PropertyTagThumbnailCompressedSize    0x5019
#define PropertyTagColorTransferFunction      0x501A
#define PropertyTagThumbnailData              0x501B

#define PropertyTagThumbnailImageWidth        0x5020
#define PropertyTagThumbnailImageHeight       0x5021
#define PropertyTagThumbnailBitsPerSample     0x5022
#define PropertyTagThumbnailCompression       0x5023
#define PropertyTagThumbnailPhotometricInterp 0x5024
#define PropertyTagThumbnailImageDescription  0x5025
#define PropertyTagThumbnailEquipMake         0x5026
#define PropertyTagThumbnailEquipModel        0x5027
#define PropertyTagThumbnailStripOffsets      0x5028
#define PropertyTagThumbnailOrientation       0x5029
#define PropertyTagThumbnailSamplesPerPixel   0x502A
#define PropertyTagThumbnailRowsPerStrip      0x502B
#define PropertyTagThumbnailStripBytesCount   0x502C
#define PropertyTagThumbnailResolutionX       0x502D
#define PropertyTagThumbnailResolutionY       0x502E
#define PropertyTagThumbnailPlanarConfig      0x502F
#define PropertyTagThumbnailResolutionUnit    0x5030
#define PropertyTagThumbnailTransferFunction  0x5031
#define PropertyTagThumbnailSoftwareUsed      0x5032
#define PropertyTagThumbnailDateTime          0x5033
#define PropertyTagThumbnailArtist            0x5034
#define PropertyTagThumbnailWhitePoint        0x5035
#define PropertyTagThumbnailPrimaryChromaticities 0x5036
#define PropertyTagThumbnailYCbCrCoefficients 0x5037
#define PropertyTagThumbnailYCbCrSubsampling  0x5038
#define PropertyTagThumbnailYCbCrPositioning  0x5039
#define PropertyTagThumbnailRefBlackWhite     0x503A
#define PropertyTagThumbnailCopyRight         0x503B

#define PropertyTagLuminanceTable    0x5090
#define PropertyTagChrominanceTable  0x5091

#define PropertyTagFrameDelay        0x5100
#define PropertyTagLoopCount         0x5101
#define PropertyTagGlobalPalette     0x5102
#define PropertyTagIndexBackground   0x5103
#define PropertyTagIndexTransparent  0x5104

#define PropertyTagPixelUnit         0x5110
#define PropertyTagPixelPerUnitX     0x5111
#define PropertyTagPixelPerUnitY     0x5112
#define PropertyTagPaletteHistogram  0x5113

#define PropertyTagExifExposureTime  0x829A
#define PropertyTagExifFNumber       0x829D

#define PropertyTagExifExposureProg  0x8822
#define PropertyTagExifSpectralSense 0x8824
#define PropertyTagExifISOSpeed      0x8827
#define PropertyTagExifOECF          0x8828

#define PropertyTagExifVer           0x9000
#define PropertyTagExifDTOrig        0x9003
#define PropertyTagExifDTDigitized   0x9004

#define PropertyTagExifCompConfig    0x9101
#define PropertyTagExifCompBPP       0x9102

#define PropertyTagExifShutterSpeed  0x9201
#define PropertyTagExifAperture      0x9202
#define PropertyTagExifBrightness    0x9203
#define PropertyTagExifExposureBias  0x9204
#define PropertyTagExifMaxAperture   0x9205
#define PropertyTagExifSubjectDist   0x9206
#define PropertyTagExifMeteringMode  0x9207
#define PropertyTagExifLightSource   0x9208
#define PropertyTagExifFlash         0x9209
#define PropertyTagExifFocalLength   0x920A
#define PropertyTagExifMakerNote     0x927C
#define PropertyTagExifUserComment   0x9286
#define PropertyTagExifDTSubsec      0x9290
#define PropertyTagExifDTOrigSS      0x9291
#define PropertyTagExifDTDigSS       0x9292

#define PropertyTagExifFPXVer        0xA000
#define PropertyTagExifColorSpace    0xA001
#define PropertyTagExifPixXDim       0xA002
#define PropertyTagExifPixYDim       0xA003
#define PropertyTagExifRelatedWav    0xA004
#define PropertyTagExifInterop       0xA005
#define PropertyTagExifFlashEnergy   0xA20B
#define PropertyTagExifSpatialFR     0xA20C
#define PropertyTagExifFocalXRes     0xA20E
#define PropertyTagExifFocalYRes     0xA20F
#define PropertyTagExifFocalResUnit  0xA210
#define PropertyTagExifSubjectLoc    0xA214
#define PropertyTagExifExposureIndex 0xA215
#define PropertyTagExifSensingMethod 0xA217
#define PropertyTagExifFileSource    0xA300
#define PropertyTagExifSceneType     0xA301
#define PropertyTagExifCfaPattern    0xA302

#define PropertyTagGpsVer            0x0000
#define PropertyTagGpsLatitudeRef    0x0001
#define PropertyTagGpsLatitude       0x0002
#define PropertyTagGpsLongitudeRef   0x0003
#define PropertyTagGpsLongitude      0x0004
#define PropertyTagGpsAltitudeRef    0x0005
#define PropertyTagGpsAltitude       0x0006
#define PropertyTagGpsGpsTime        0x0007
#define PropertyTagGpsGpsSatellites  0x0008
#define PropertyTagGpsGpsStatus      0x0009
#define PropertyTagGpsGpsMeasureMode 0x000A
#define PropertyTagGpsGpsDop         0x000B
#define PropertyTagGpsSpeedRef       0x000C
#define PropertyTagGpsSpeed          0x000D
#define PropertyTagGpsTrackRef       0x000E
#define PropertyTagGpsTrack          0x000F
#define PropertyTagGpsImgDirRef      0x0010
#define PropertyTagGpsImgDir         0x0011
#define PropertyTagGpsMapDatum       0x0012
#define PropertyTagGpsDestLatRef     0x0013
#define PropertyTagGpsDestLat        0x0014
#define PropertyTagGpsDestLongRef    0x0015
#define PropertyTagGpsDestLong       0x0016
#define PropertyTagGpsDestBearRef    0x0017
#define PropertyTagGpsDestBear       0x0018
#define PropertyTagGpsDestDistRef    0x0019
#define PropertyTagGpsDestDist       0x001A

#endif /* _GDIPLUSIMAGING_H */
