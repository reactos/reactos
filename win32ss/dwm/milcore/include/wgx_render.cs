// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  File:       milrender.cs
//------------------------------------------------------------------------------

using System;
using System.IO;
using System.Windows;
using System.Windows.Media.Composition;
using System.Runtime.InteropServices;
using System.Windows.Media;
using System.Security;
using System.Security.Permissions;
using SR=MS.Internal.PresentationCore.SR;
using SRID=MS.Internal.PresentationCore.SRID;

namespace MS.Internal
{
    #region Enumeration Types

    internal enum NtStatusErrors
    {
        NT_STATUS_NO_MEMORY = unchecked((int)0xC0000017)
    }

    internal enum MediaPlayerErrors
    {
        NS_E_WMP_LOGON_FAILURE              =   unchecked((int)0xC00D1196L),
        NS_E_WMP_CANNOT_FIND_FILE           =   unchecked((int)0xC00D1197L),
        NS_E_WMP_UNSUPPORTED_FORMAT         =   unchecked((int)0xC00D1199L),
        NS_E_WMP_DSHOW_UNSUPPORTED_FORMAT   =   unchecked((int)0xC00D119AL),
        NS_E_WMP_INVALID_ASX                =   unchecked((int)0xC00D119DL),
        NS_E_WMP_URLDOWNLOADFAILED          =   unchecked((int)0xC00D0FEAL),
    }

    internal enum LockFlags
    {
        MIL_LOCK_READ        = 0x00000001,
        MIL_LOCK_WRITE       = 0x00000002,
    }

    internal enum WICBitmapAlphaChannelOption
    {
        WICBitmapUseAlpha              = 0,
        WICBitmapUsePremultipliedAlpha = 1,
        WICBitmapIgnoreAlpha           = 2,
    }

    internal enum WICBitmapCreateCacheOptions
    {
        WICBitmapNoCache            = 0x00000000,
        WICBitmapCacheOnDemand      = 0x00000001,
        WICBitmapCacheOnLoad        = 0x00000002,
    }

    internal enum WICBitmapEncodeCacheOption
    {
        WICBitmapEncodeCacheInMemory = 0x00000000,
        WICBitmapEncodeCacheTempFile = 0x00000001,
        WICBitmapEncodeNoCache       = 0x00000002,
    }

    internal enum WICMetadataCacheOptions
    {
        WICMetadataCacheOnDemand = 0x00000000,
        WICMetadataCacheOnLoad   = 0x00000001
    };

    internal enum WICInterpolationMode
    {
        NearestNeighbor = 0,
        Linear          = 1,
        Cubic           = 2,
        Fant            = 3
    }

    /// <summary>
    /// PixelFormatEnum represents the format of the bits of an image or surface.
    /// </summary>
    internal enum PixelFormatEnum
    {
        /// <summary>
        /// Default: (DontCare) the format is not important
        /// </summary>
        Default    = 0,

        /// <summary>
        /// Extended: the pixel format is 3rd party - we don't know anything about it.
        /// </summary>
        Extended   = Default,

        /// <summary>
        /// Indexed1: Paletted image with 2 colors.
        /// </summary>
        Indexed1    = 0x1,

        /// <summary>
        /// Indexed2: Paletted image with 4 colors.
        /// </summary>
        Indexed2    = 0x2,

        /// <summary>
        /// Indexed4: Paletted image with 16 colors.
        /// </summary>
        Indexed4    = 0x3,

        /// <summary>
        /// Indexed8: Paletted image with 256 colors.
        /// </summary>
        Indexed8    = 0x4,

        /// <summary>
        /// BlackWhite: Monochrome, 2-color image, black and white only.
        /// </summary>
        BlackWhite  = 0x5,

        /// <summary>
        /// Gray2: Image with 4 shades of gray
        /// </summary>
        Gray2       = 0x6,

        /// <summary>
        /// Gray4: Image with 16 shades of gray
        /// </summary>
        Gray4       = 0x7,

        /// <summary>
        /// Gray8: Image with 256 shades of gray
        /// </summary>
        Gray8       = 0x8,

        /// <summary>
        /// Bgr555: 16 bpp SRGB format
        /// </summary>
        Bgr555      = 0x9,

        /// <summary>
        /// Bgr565: 16 bpp SRGB format
        /// </summary>
        Bgr565      = 0xA,

        /// <summary>
        /// Gray16: 16 bpp Gray format
        /// </summary>
        Gray16 = 0xB,

        /// <summary>
        /// Bgr24: 24 bpp SRGB format
        /// </summary>
        Bgr24       = 0xC,

        /// <summary>
        /// BGR24: 24 bpp SRGB format
        /// </summary>
        Rgb24       = 0xD,

        /// <summary>
        /// Bgr32: 32 bpp SRGB format
        /// </summary>
        Bgr32       = 0xE,

        /// <summary>
        /// Bgra32: 32 bpp SRGB format
        /// </summary>
        Bgra32      = 0xF,

        /// <summary>
        /// Pbgra32: 32 bpp SRGB format
        /// </summary>
        Pbgra32     = 0x10,

        /// <summary>
        /// Gray32Float: 32 bpp Gray format, gamma is 1.0
        /// </summary>
        Gray32Float = 0x11,

        /// <summary>
        /// Bgr101010: 32 bpp Gray fixed point format
        /// </summary>
        Bgr101010 = 0x14,

        /// <summary>
        /// Rgb48: 48 bpp RGB format
        /// </summary>
        Rgb48 = 0x15,

        /// <summary>
        /// Rgba64: 64 bpp extended format; Gamma is 1.0
        /// </summary>
        Rgba64      = 0x16,

        /// <summary>
        /// Prgba64: 64 bpp extended format; Gamma is 1.0
        /// </summary>
        Prgba64     = 0x17,

        /// <summary>
        /// Rgba128Float: 128 bpp extended format; Gamma is 1.0
        /// </summary>
        Rgba128Float     = 0x19,

        /// <summary>
        /// Prgba128Float: 128 bpp extended format; Gamma is 1.0
        /// </summary>
        Prgba128Float    = 0x1A,

        /// <summary>
        /// PABGR128Float: 128 bpp extended format; Gamma is 1.0
        /// </summary>
        Rgb128Float    = 0x1B,

        /// <summary>
        /// CMYK32: 32 bpp CMYK format.
        /// </summary>
        Cmyk32      = 0x1C
    }

    internal enum DitherType
    {
        // Solid color - picks the nearest matching color with no attempt to
        // halftone or dither. May be used on an arbitrary palette.

        DitherTypeNone          = 0,
        DitherTypeSolid         = 0,

        // Ordered dithers and spiral dithers must be used with a fixed palette or
        // a fixed palette translation.

        // NOTE: DitherOrdered4x4 is unique in that it may apply to 16bpp
        // conversions also.

        DitherTypeOrdered4x4    = 1,

        DitherTypeOrdered8x8    = 2,
        DitherTypeOrdered16x16  = 3,
        DitherTypeSpiral4x4     = 4,
        DitherTypeSpiral8x8     = 5,
        DitherTypeDualSpiral4x4 = 6,
        DitherTypeDualSpiral8x8 = 7,

        // Error diffusion. May be used with any palette.

        DitherTypeErrorDiffusion = 8,
    }


    /// <summary>
    /// WICPaletteType
    /// </summary>
    internal enum WICPaletteType
    {
        /// <summary>
        /// Arbitrary custom palette provided by caller.
        /// </summary>
        WICPaletteTypeCustom           = 0,

        /// <summary>
        /// Optimal palette generated using a median-cut algorithm.
        /// </summary>
        WICPaletteTypeOptimal          = 1,

        /// <summary>
        /// Black and white palette.
        /// </summary>
        WICPaletteTypeFixedBW          = 2,

        // Symmetric halftone palettes.
        // Each of these halftone palettes will be a superset of the system palette.
        // E.g. Halftone8 will have it's 8-color on-off primaries and the 16 system
        // colors added. With duplicates removed, that leaves 16 colors.

        /// <summary>
        /// 8-color, on-off primaries
        /// </summary>
        WICPaletteTypeFixedHalftone8   = 3,

        /// <summary>
        /// 3 intensity levels of each color
        /// </summary>
        WICPaletteTypeFixedHalftone27  = 4,

        /// <summary>
        /// 4 intensity levels of each color
        /// </summary>
        WICPaletteTypeFixedHalftone64  = 5,

        /// <summary>
        /// 5 intensity levels of each color
        /// </summary>
        WICPaletteTypeFixedHalftone125 = 6,

        /// <summary>
        /// 6 intensity levels of each color
        /// </summary>
        WICPaletteTypeFixedHalftone216 = 7,

        /// <summary>
        /// convenient web palette, same as WICPaletteTypeFixedHalftone216
        /// </summary>
        WICPaletteTypeFixedWebPalette  = 7,

        // Assymetric halftone palettes.
        // These are somewhat less useful than the symmetric ones, but are
        // included for completeness. These do not include all of the system
        // colors.

        /// <summary>
        /// 6-red, 7-green, 6-blue intensities
        /// </summary>
        WICPaletteTypeFixedHalftone252 = 8,

        /// <summary>
        /// 8-red, 8-green, 4-blue intensities
        /// </summary>
        WICPaletteTypeFixedHalftone256 = 9,

        /// <summary>
        /// 4 shades of gray
        /// </summary>
        WICPaletteTypeFixedGray4 = 10,

        /// <summary>
        /// 16 shades of gray
        /// </summary>
        WICPaletteTypeFixedGray16 = 11,

        /// <summary>
        /// 256 shades of gray
        /// </summary>
        WICPaletteTypeFixedGray256 = 12
    };

    internal enum MILCompoundStyle
    {
        MILCompoundStyleSingle = 0,
        MILCompoundStyleDouble = 1,
        MILCompoundStyleTriple = 2,
        MILCompoundStyleCustom = 3,
        MILCOMPOUNDSTYLE_FORCE_DWORD = 0x7FFFFFFF // MIL_FORCE_DWORD
    }

    internal enum MILLineJoin
    {
        MILLineJoinMiter        = 0,
        MILLineJoinBevel        = 1,
        MILLineJoinRound        = 2,
        MILLineJoinMiterClipped = 3,
        MILLINEJOIN_FORCE_DWORD = 0x7FFFFFFF // MIL_FORCE_DWORD
    }


    internal enum MILDashStyle
    {
        MILDashStyleSolid      = 0,
        MILDashStyleDash       = 1,
        MILDashStyleDot        = 2,
        MILDashStyleDashDot    = 3,
        MILDashStyleDashDotDot = 4,
        MILDashStyleCustom     = 5,
        MILDASHSTYLE_FORCE_DWORD = 0x7FFFFFFF // MIL_FORCE_DWORD
    }

    internal enum MILPropTypes
    {
        MILPropertyItemTypeInvalid     = 0,
        MILPropertyItemTypeByte        = 1,
        MILPropertyItemTypeASCII       = 2,
        MILPropertyItemTypeWord        = 3,
        MILPropertyItemTypeLong        = 4,
        MILPropertyItemTypeRational    = 5,
        MILPropertyItemTypeUndefined   = 7,
        MILPropertyItemTypeSLong       = 9,
        MILPropertyItemTypeSRational   = 10,

        MILPropertyItemTypeMax         = 0xFFFF,
    }

    internal enum MILPropIDs: uint
    {
        MILPropertyItemIdInvalid             = 0x0,
        MILPropertyItemIdNewSubfileType      = 0x00FE,
        MILPropertyItemIdSubfileType         = 0x00FF,
        MILPropertyItemIdImageWidth          = 0x0100,
        MILPropertyItemIdImageHeight         = 0x0101,
        MILPropertyItemIdBitsPerSample       = 0x0102,
        MILPropertyItemIdCompression         = 0x0103,
        MILPropertyItemIdPhotometricInterp   = 0x0106,
        MILPropertyItemIdThreshHolding       = 0x0107,
        MILPropertyItemIdCellWidth           = 0x0108,
        MILPropertyItemIdCellHeight          = 0x0109,
        MILPropertyItemIdFillOrder           = 0x010A,
        MILPropertyItemIdDocumentName        = 0x010D,
        MILPropertyItemIdImageDescription    = 0x010E,
        MILPropertyItemIdEquipMake           = 0x010F,
        MILPropertyItemIdEquipModel          = 0x0110,
        MILPropertyItemIdStripOffsets        = 0x0111,
        MILPropertyItemIdOrientation         = 0x0112,
        MILPropertyItemIdSamplesPerPixel     = 0x0115,
        MILPropertyItemIdRowsPerStrip        = 0x0116,
        MILPropertyItemIdStripBytesCount     = 0x0117,
        MILPropertyItemIdMinSampleValue      = 0x0118,
        MILPropertyItemIdMaxSampleValue      = 0x0119,
        MILPropertyItemIdXResolution         = 0x011A,
        MILPropertyItemIdYResolution         = 0x011B,
        MILPropertyItemIdPlanarConfig        = 0x011C,
        MILPropertyItemIdPageName            = 0x011D,
        MILPropertyItemIdXPosition           = 0x011E,
        MILPropertyItemIdYPosition           = 0x011F,
        MILPropertyItemIdFreeOffset          = 0x0120,
        MILPropertyItemIdFreeByteCounts      = 0x0121,
        MILPropertyItemIdGrayResponseUnit    = 0x0122,
        MILPropertyItemIdGrayResponseCurve   = 0x0123,
        MILPropertyItemIdT4Option            = 0x0124,
        MILPropertyItemIdT6Option            = 0x0125,
        MILPropertyItemIdResolutionUnit      = 0x0128,
        MILPropertyItemIdPageNumber          = 0x0129,
        MILPropertyItemIdTransferFuncition   = 0x012D,
        MILPropertyItemIdSoftwareUsed        = 0x0131,
        MILPropertyItemIdDateTime            = 0x0132,
        MILPropertyItemIdArtist              = 0x013B,
        MILPropertyItemIdHostComputer        = 0x013C,
        MILPropertyItemIdPredictor           = 0x013D,
        MILPropertyItemIdWhitePoint          = 0x013E,
        MILPropertyItemIdPrimaryChromaticities = 0x013F,
        MILPropertyItemIdColorMap            = 0x0140,
        MILPropertyItemIdHalftoneHints       = 0x0141,
        MILPropertyItemIdTileWidth           = 0x0142,
        MILPropertyItemIdTileLength          = 0x0143,
        MILPropertyItemIdTileOffset          = 0x0144,
        MILPropertyItemIdTileByteCounts      = 0x0145,
        MILPropertyItemIdInkSet              = 0x014C,
        MILPropertyItemIdInkNames            = 0x014D,
        MILPropertyItemIdNumberOfInks        = 0x014E,
        MILPropertyItemIdDotRange            = 0x0150,
        MILPropertyItemIdTargetPrinter       = 0x0151,
        MILPropertyItemIdExtraSamples        = 0x0152,
        MILPropertyItemIdSampleFormat        = 0x0153,
        MILPropertyItemIdSMinSampleValue     = 0x0154,
        MILPropertyItemIdSMaxSampleValue     = 0x0155,
        MILPropertyItemIdTransferRange       = 0x0156,
        MILPropertyItemIdScreenWidth         = 0x0157,
        MILPropertyItemIdScreenHeight        = 0x0158,

        MILPropertyItemIdJPEGProc            = 0x0200,
        MILPropertyItemIdJPEGInterFormat     = 0x0201,
        MILPropertyItemIdJPEGInterLength     = 0x0202,
        MILPropertyItemIdJPEGRestartInterval = 0x0203,
        MILPropertyItemIdJPEGLosslessPredictors  = 0x0205,
        MILPropertyItemIdJPEGPointTransforms     = 0x0206,
        MILPropertyItemIdJPEGQTables         = 0x0207,
        MILPropertyItemIdJPEGDCTables        = 0x0208,
        MILPropertyItemIdJPEGACTables        = 0x0209,
        MILPropertyItemIdYCbCrCoefficients   = 0x0211,
        MILPropertyItemIdYCbCrSubsampling    = 0x0212,
        MILPropertyItemIdYCbCrPositioning    = 0x0213,
        MILPropertyItemIdREFBlackWhite       = 0x0214,
        MILPropertyItemIdInterlaced          = 0x0215,

        MILPropertyItemIdGamma               = 0x0301,
        MILPropertyItemIdICCProfileDescriptor = 0x0302,
        MILPropertyItemIdSRGBRenderingIntent = 0x0303,
        MILPropertyItemIdICCProfile          = 0x8773,

        MILPropertyItemIdImageTitle          = 0x0320,
        MILPropertyItemIdCopyright           = 0x8298,

        // Extra IDs (Like Adobe Image Information ids etc.)

        MILPropertyItemIdResolutionXUnit           = 0x5001,
        MILPropertyItemIdResolutionYUnit           = 0x5002,
        MILPropertyItemIdResolutionXLengthUnit     = 0x5003,
        MILPropertyItemIdResolutionYLengthUnit     = 0x5004,
        MILPropertyItemIdPrintFlags                = 0x5005,
        MILPropertyItemIdPrintFlagsVersion         = 0x5006,
        MILPropertyItemIdPrintFlagsCrop            = 0x5007,
        MILPropertyItemIdPrintFlagsBleedWidth      = 0x5008,
        MILPropertyItemIdPrintFlagsBleedWidthScale = 0x5009,
        MILPropertyItemIdHalftoneLPI               = 0x500A,
        MILPropertyItemIdHalftoneLPIUnit           = 0x500B,
        MILPropertyItemIdHalftoneDegree            = 0x500C,
        MILPropertyItemIdHalftoneShape             = 0x500D,
        MILPropertyItemIdHalftoneMisc              = 0x500E,
        MILPropertyItemIdHalftoneScreen            = 0x500F,
        MILPropertyItemIdJPEGQuality               = 0x5010,
        MILPropertyItemIdGridSize                  = 0x5011,
        MILPropertyItemIdThumbnailFormat           = 0x5012,
        MILPropertyItemIdThumbnailWidth            = 0x5013,
        MILPropertyItemIdThumbnailHeight           = 0x5014,
        MILPropertyItemIdThumbnailColorDepth       = 0x5015,
        MILPropertyItemIdThumbnailPlanes           = 0x5016,
        MILPropertyItemIdThumbnailRawBytes         = 0x5017,
        MILPropertyItemIdThumbnailSize             = 0x5018,
        MILPropertyItemIdThumbnailCompressedSize   = 0x5019,
        MILPropertyItemIdColorTransferFunction     = 0x501A,
        MILPropertyItemIdThumbnailData             = 0x501B,

        // Thumbnail related ids

        MILPropertyItemIdThumbnailImageWidth        = 0x5020,
        MILPropertyItemIdThumbnailImageHeight       = 0x5021,
        MILPropertyItemIdThumbnailBitsPerSample     = 0x5022,
        MILPropertyItemIdThumbnailCompression       = 0x5023,
        MILPropertyItemIdThumbnailPhotometricInterp = 0x5024,
        MILPropertyItemIdThumbnailImageDescription  = 0x5025,
        MILPropertyItemIdThumbnailEquipMake         = 0x5026,
        MILPropertyItemIdThumbnailEquipModel        = 0x5027,
        MILPropertyItemIdThumbnailStripOffsets      = 0x5028,
        MILPropertyItemIdThumbnailOrientation       = 0x5029,
        MILPropertyItemIdThumbnailSamplesPerPixel   = 0x502A,
        MILPropertyItemIdThumbnailRowsPerStrip      = 0x502B,
        MILPropertyItemIdThumbnailStripBytesCount   = 0x502C,
        MILPropertyItemIdThumbnailResolutionX       = 0x502D,
        MILPropertyItemIdThumbnailResolutionY       = 0x502E,
        MILPropertyItemIdThumbnailPlanarConfig      = 0x502F,
        MILPropertyItemIdThumbnailResolutionUnit    = 0x5030,
        MILPropertyItemIdThumbnailTransferFunction  = 0x5031,
        MILPropertyItemIdThumbnailSoftwareUsed      = 0x5032,
        MILPropertyItemIdThumbnailDateTime          = 0x5033,
        MILPropertyItemIdThumbnailArtist            = 0x5034,
        MILPropertyItemIdThumbnailWhitePoint        = 0x5035,
        MILPropertyItemIdThumbnailPrimaryChromaticities = 0x5036,
        MILPropertyItemIdThumbnailYCbCrCoefficients = 0x5037,
        MILPropertyItemIdThumbnailYCbCrSubsampling  = 0x5038,
        MILPropertyItemIdThumbnailYCbCrPositioning  = 0x5039,
        MILPropertyItemIdThumbnailRefBlackWhite     = 0x503A,
        MILPropertyItemIdThumbnailCopyRight         = 0x503B,

        MILPropertyItemIdLuminanceTable             = 0x5090,
        MILPropertyItemIdChrominanceTable           = 0x5091,
        MILPropertyItemIdFrameDelay                 = 0x5100,
        MILPropertyItemIdLoopCount                  = 0x5101,
        MILPropertyItemIdGlobalPalette              = 0x5102,
        MILPropertyItemIdIndexBackground            = 0x5103,
        MILPropertyItemIdIndexTransparent           = 0x5104,

        MILPropertyItemIdPixelUnit                  = 0x5110,
        MILPropertyItemIdPixelPerUnitX              = 0x5111,
        MILPropertyItemIdPixelPerUnitY              = 0x5112,
        MILPropertyItemIdPaletteHistogram           = 0x5113,

        // EXIF specific ids
        MILPropertyItemIdExifIFD            = 0x8769,
        MILPropertyItemIdExifExposureTime   = 0x829A,
        MILPropertyItemIdExifFNumber        = 0x829D,
        MILPropertyItemIdExifExposureProg   = 0x8822,
        MILPropertyItemIdExifSpectralSense  = 0x8824,
        MILPropertyItemIdExifISOSpeed       = 0x8827,
        MILPropertyItemIdExifOECF           = 0x8828,
        MILPropertyItemIdExifVer            = 0x9000,
        MILPropertyItemIdExifDTOrig         = 0x9003,
        MILPropertyItemIdExifDTDigitized    = 0x9004,
        MILPropertyItemIdExifCompConfig     = 0x9101,
        MILPropertyItemIdExifCompBPP        = 0x9102,
        MILPropertyItemIdExifShutterSpeed   = 0x9201,
        MILPropertyItemIdExifAperture       = 0x9202,
        MILPropertyItemIdExifBrightness     = 0x9203,
        MILPropertyItemIdExifExposureBias   = 0x9204,
        MILPropertyItemIdExifMaxAperture    = 0x9205,
        MILPropertyItemIdExifSubjectDist    = 0x9206,
        MILPropertyItemIdExifMeteringMode   = 0x9207,
        MILPropertyItemIdExifLightSource    = 0x9208,
        MILPropertyItemIdExifFlash          = 0x9209,
        MILPropertyItemIdExifFocalLength    = 0x920A,
        MILPropertyItemIdExifMakerNote      = 0x927C,
        MILPropertyItemIdExifUserComment    = 0x9286,
        MILPropertyItemIdExifDTSubsec       = 0x9290,
        MILPropertyItemIdExifDTOrigSS       = 0x9291,
        MILPropertyItemIdExifDTDigSS        = 0x9292,
        MILPropertyItemIdExifFPXVer         = 0xA000,
        MILPropertyItemIdExifColorSpace     = 0xA001,
        MILPropertyItemIdExifPixXDim        = 0xA002,
        MILPropertyItemIdExifPixYDim        = 0xA003,
        MILPropertyItemIdExifRelatedWav     = 0xA004,
        MILPropertyItemIdExifInterop        = 0xA005,
        MILPropertyItemIdExifFlashEnergy    = 0xA20B,
        MILPropertyItemIdExifSpatialFR      = 0xA20C,
        MILPropertyItemIdExifFocalXRes      = 0xA20E,
        MILPropertyItemIdExifFocalYRes      = 0xA20F,
        MILPropertyItemIdExifFocalResUnit   = 0xA210,
        MILPropertyItemIdExifSubjectLoc     = 0xA214,
        MILPropertyItemIdExifExposureIndex  = 0xA215,
        MILPropertyItemIdExifSensingMethod  = 0xA217,
        MILPropertyItemIdExifFileSource     = 0xA300,
        MILPropertyItemIdExifSceneType      = 0xA301,
        MILPropertyItemIdExifCfaPattern     = 0xA302,

        MILPropertyItemIdMax                = 0xFFFF
    };

    internal enum ChildType
    {
        eCOB =       0x0,
        eCOBGROUP =  0x1,
        eChildLast = 0x2
    };

    enum MILResourceType
    {
        eMILResourceVideo = 0,
        eMILCOB = 1,
        eMILChain = 2,
        eMILTarget = 3,
        eMILResource = 4,
        eMILResourceLast = 5
    };

    enum MILAVInstructionType
    {
        eAVPlay = 0,
        eAVStop,
        eAVPause,
        eAVResume,
        eAVSetSeek,
        eAVGetVolume,
        eAVSetVolume,
        eAVGetState,
        eAVHasVideo,
        eAVHasAudio,
        eAVWidth,
        eAVHeight,
        eAVMediaLength
    };
    #endregion

    #region Structures
    /// <summary>
    /// Transform options when doing a lossless JPEG image save
    /// </summary>
    /// <ExternalAPI Inherit="true"/>
    internal enum WICBitmapTransformOptions
    {
        /// <summary>
        /// Don't Rotate
        /// </summary>
        WICBitmapTransformRotate0                = 0,
        /// <summary>
        /// Rotate 90 degree clockwise
        /// </summary>
        WICBitmapTransformRotate90            = 0x1,
        /// <summary>
        /// Rotate 180 degree
        /// </summary>
        WICBitmapTransformRotate180           = 0x2,
        /// <summary>
        /// Rotate 270 degree clockwise
        /// </summary>
        WICBitmapTransformRotate270           = 0x3,
        /// <summary>
        /// Flip the image horizontally
        /// </summary>
        WICBitmapTransformFlipHorizontal      = 0x8,
        /// <summary>
        /// Flip the image vertically
        /// </summary>
        WICBitmapTransformFlipVertical        = 0x10
    }

    internal enum WICComponentType
    {
        WICDecoder         = 0x00000001,
        WICEncoder         = 0x00000002,
        WICFormat          = 0x00000004,
        WICFormatConverter = 0x00000008,
        WICMetadataReader  = 0x00000010,
        WICMetadataWriter  = 0x00000020
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct WICBitmapPattern
    {
        Int64 Offset;
        UInt32 Length;
        IntPtr Pattern;
        IntPtr Mask;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct MILBitmapItem
    {
        uint          Size;
        IntPtr        Desc;
        uint          DescSize;
        IntPtr        Data;
        uint          DataSize;
        uint          Cookie;
    };

    [StructLayout(LayoutKind.Sequential)]
    internal struct BitmapTransformCaps
    {
        // Size of this structure.

        int nSize;

        // minimum number of inputs required.

        int cMinInputs;

        // maximum number of inputs that will be processed.

        int cMaxInputs;

        // Set to false requires all the inputs and the
        // output to have the same pixel format determined
        // by calling IsPixelFormatSupported() for any
        // index. Set to true allows different input/output
        // pixel formats.

        [MarshalAs(UnmanagedType.Bool)] bool fSupportMultiFormat;

        // Supports auxilliary data out.

        [MarshalAs(UnmanagedType.Bool)] bool fAuxiliaryData;

        // TRUE if the effect supports multiple output

        [MarshalAs(UnmanagedType.Bool)] bool fSupportMultiOutput;

        // TRUE if the effect can provide output band by band

        [MarshalAs(UnmanagedType.Bool)] bool fSupportBanding;

        // TRUE if the effect supports multi-resolution

        [MarshalAs(UnmanagedType.Bool)] bool fSupportMultiResolution;

    };

    [StructLayout(LayoutKind.Sequential)]
    internal struct HWND
    {
        public int hwnd;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct MILAVInstruction
    {
        internal IntPtr m_pMedia;
        internal MILAVInstructionType m_instType;
        internal nested_u u;

        [StructLayout(LayoutKind.Explicit)]
        internal struct nested_u
        {
            [FieldOffset(0)] [MarshalAs(UnmanagedType.Bool)] internal bool m_fValue;
            [FieldOffset(0)] internal double m_dblValue;
            [FieldOffset(0)] internal int m_iValue;
            [FieldOffset(0)] internal long m_lValue;
            [FieldOffset(0)] internal float m_flValue;
        };

    };

    #endregion

    #region HRESULT
    /// <summary>
    /// HRESULT
    /// </summary>
    /// <ExternalAPI/>
    // [StructLayout(LayoutKind.Sequential)]
    internal struct HRESULT
    {
        // NTSTATUS error codes are converted to HRESULT by ORing the FACILITY_NT_BIT
        // into the status code.
        internal const int FACILITY_NT_BIT = 0x10000000;

        internal const int FACILITY_MASK = 0x7FFF0000;

        internal const int FACILITY_WINCODEC_ERROR = 0x08980000;

        internal const int COMPONENT_MASK = 0x0000E000;

        internal const int COMPONENT_WINCODEC_ERROR = 0x00002000;

        internal static bool IsWindowsCodecError(int hr)
        {
            return (hr & (FACILITY_MASK | COMPONENT_MASK)) == (FACILITY_WINCODEC_ERROR | COMPONENT_WINCODEC_ERROR);
        }

        /// <summary>
        /// If the result is not a success, then throw the appropriate exception.
        /// </summary>
        /// <param name="hr"></param>
        /// <ExternalAPI/>
        /// <SecurityNote>
        ///   Critical: This code calls into Marshal.GetExceptionForHR which has a link demand on it
        ///   Safe: Throwing an exception is deemed as a safe operation (throwing exceptions is allowed in Partial Trust). 
        ///         We ensure the call to GetExceptionForHR is safe since we pass an IntPtr that has a value of -1 so that 
        ///         GetExceptionForHR ignores IErrorInfo of the current thread, which could reveal critical information otherwise.
        /// </SecurityNote>
        [SecuritySafeCritical]
        internal static Exception ConvertHRToException(int hr)
        {
            Exception exceptionForHR = Marshal.GetExceptionForHR(hr, (IntPtr)(-1));

            if ((hr & FACILITY_NT_BIT) == FACILITY_NT_BIT)
            {
                // Convert HRESULT to NTSTATUS code.
                switch(hr & ~FACILITY_NT_BIT)
                {
                    case (int) NtStatusErrors.NT_STATUS_NO_MEMORY:
                        return new OutOfMemoryException();

                    default:
                        return exceptionForHR;
                }
            }
            else
            {
                switch (hr)
                {
                    case (int) NtStatusErrors.NT_STATUS_NO_MEMORY:
                        return new System.OutOfMemoryException();

                    case (int)WinCodecErrors.WINCODEC_ERR_WRONGSTATE:
                        return new System.InvalidOperationException(SR.Get(SRID.Image_WrongState), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_VALUEOUTOFRANGE:
                    case (int)WinCodecErrors.WINCODEC_ERR_VALUEOVERFLOW:
                        return new System.OverflowException(SR.Get(SRID.Image_Overflow), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_UNKNOWNIMAGEFORMAT:
                        return new System.IO.FileFormatException(null, SR.Get(SRID.Image_UnknownFormat), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_UNSUPPORTEDVERSION:
                        return new System.IO.FileLoadException(SR.Get(SRID.MilErr_UnsupportedVersion), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_NOTINITIALIZED:
                        return new System.InvalidOperationException(SR.Get(SRID.WIC_NotInitialized), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_PROPERTYNOTFOUND:
                        return new System.ArgumentException(SR.Get(SRID.Image_PropertyNotFound), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_PROPERTYNOTSUPPORTED:
                        return new System.NotSupportedException(SR.Get(SRID.Image_PropertyNotSupported), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_PROPERTYSIZE:
                        return new System.ArgumentException(SR.Get(SRID.Image_PropertySize), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_CODECPRESENT:
                        return new System.InvalidOperationException(SR.Get(SRID.Image_CodecPresent), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_CODECNOTHUMBNAIL:
                        return new System.NotSupportedException(SR.Get(SRID.Image_NoThumbnail), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_PALETTEUNAVAILABLE:
                        return new System.InvalidOperationException(SR.Get(SRID.Image_NoPalette), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_CODECTOOMANYSCANLINES:
                        return new System.ArgumentException(SR.Get(SRID.Image_TooManyScanlines), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_INTERNALERROR:
                        return new System.InvalidOperationException(SR.Get(SRID.Image_InternalError), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_SOURCERECTDOESNOTMATCHDIMENSIONS:
                        return new System.ArgumentException(SR.Get(SRID.Image_BadDimensions), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_COMPONENTINITIALIZEFAILURE:
                    case (int)WinCodecErrors.WINCODEC_ERR_COMPONENTNOTFOUND:
                        return new System.NotSupportedException(SR.Get(SRID.Image_ComponentNotFound), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_UNEXPECTEDSIZE:
                    case (int)WinCodecErrors.WINCODEC_ERR_BADIMAGE:             // error decoding image file
                        return new System.IO.FileFormatException(null, SR.Get(SRID.Image_DecoderError), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_BADHEADER:             // error decoding header
                        return new System.IO.FileFormatException(null, SR.Get(SRID.Image_HeaderError), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_FRAMEMISSING:
                        return new System.ArgumentException(SR.Get(SRID.Image_FrameMissing), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_BADMETADATAHEADER:
                        return new System.ArgumentException(SR.Get(SRID.Image_BadMetadataHeader), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_BADSTREAMDATA:
                        return new System.ArgumentException(SR.Get(SRID.Image_BadStreamData), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_STREAMWRITE:
                        return new System.InvalidOperationException(SR.Get(SRID.Image_StreamWrite), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT:
                        return new System.NotSupportedException(SR.Get(SRID.Image_UnsupportedPixelFormat), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_UNSUPPORTEDOPERATION:
                        return new System.NotSupportedException(SR.Get(SRID.Image_UnsupportedOperation), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_IMAGESIZEOUTOFRANGE:
                        return new System.ArgumentException(SR.Get(SRID.Image_SizeOutOfRange), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_STREAMREAD:
                        return new System.IO.IOException(SR.Get(SRID.Image_StreamRead), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_INVALIDQUERYREQUEST:
                        return new System.IO.IOException(SR.Get(SRID.Image_InvalidQueryRequest), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_UNEXPECTEDMETADATATYPE:
                        return new System.IO.FileFormatException(null, SR.Get(SRID.Image_UnexpectedMetadataType), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_REQUESTONLYVALIDATMETADATAROOT:
                        return new System.IO.FileFormatException(null, SR.Get(SRID.Image_RequestOnlyValidAtMetadataRoot), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_INVALIDQUERYCHARACTER:
                        return new System.IO.IOException(SR.Get(SRID.Image_InvalidQueryCharacter), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_DUPLICATEMETADATAPRESENT:
                        return new System.IO.FileFormatException(null, SR.Get(SRID.Image_DuplicateMetadataPresent), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_PROPERTYUNEXPECTEDTYPE:
                        return new System.IO.FileFormatException(null, SR.Get(SRID.Image_PropertyUnexpectedType), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_TOOMUCHMETADATA:
                        return new System.IO.FileFormatException(null, SR.Get(SRID.Image_TooMuchMetadata), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_STREAMNOTAVAILABLE:
                        return new System.NotSupportedException(SR.Get(SRID.Image_StreamNotAvailable), exceptionForHR);

                    case (int)WinCodecErrors.WINCODEC_ERR_INSUFFICIENTBUFFER:
                        return new System.ArgumentException(SR.Get(SRID.Image_InsufficientBuffer), exceptionForHR);

                    case unchecked((int)0x80070057):
                        return new System.ArgumentException(SR.Get(SRID.Media_InvalidArgument, null), exceptionForHR);

                    case unchecked((int)0x800707db):
                        return new System.IO.FileFormatException(null, SR.Get(SRID.Image_InvalidColorContext), exceptionForHR);

                    case (int)MILErrors.WGXERR_DISPLAYSTATEINVALID:
                        return new System.InvalidOperationException(SR.Get(SRID.Image_DisplayStateInvalid), exceptionForHR);

                    case (int)MILErrors.WGXERR_NONINVERTIBLEMATRIX:
                        return new System.ArithmeticException(SR.Get(SRID.Image_SingularMatrix), exceptionForHR);

                    case (int)MILErrors.WGXERR_AV_INVALIDWMPVERSION:
                        return new System.Windows.Media.InvalidWmpVersionException(SR.Get(SRID.Media_InvalidWmpVersion, null), exceptionForHR);

                    case (int)MILErrors.WGXERR_AV_INSUFFICIENTVIDEORESOURCES:
                        return new System.NotSupportedException(SR.Get(SRID.Media_InsufficientVideoResources, null), exceptionForHR);

                    case (int)MILErrors.WGXERR_AV_VIDEOACCELERATIONNOTAVAILABLE:
                        return new System.NotSupportedException(SR.Get(SRID.Media_HardwareVideoAccelerationNotAvailable, null), exceptionForHR);

                    case (int)MILErrors.WGXERR_AV_MEDIAPLAYERCLOSED:
                        return new System.NotSupportedException(SR.Get(SRID.Media_PlayerIsClosed, null), exceptionForHR);

                    case (int)MediaPlayerErrors.NS_E_WMP_URLDOWNLOADFAILED:
                        return new System.IO.FileNotFoundException(SR.Get(SRID.Media_DownloadFailed, null), exceptionForHR);

                    case (int)MediaPlayerErrors.NS_E_WMP_LOGON_FAILURE:
                        return new System.Security.SecurityException(SR.Get(SRID.Media_LogonFailure), exceptionForHR);

                    case (int)MediaPlayerErrors.NS_E_WMP_CANNOT_FIND_FILE:
                        return new System.IO.FileNotFoundException(SR.Get(SRID.Media_FileNotFound), exceptionForHR);

                    case (int)MediaPlayerErrors.NS_E_WMP_UNSUPPORTED_FORMAT:
                    case (int)MediaPlayerErrors.NS_E_WMP_DSHOW_UNSUPPORTED_FORMAT:
                        return new System.IO.FileFormatException(SR.Get(SRID.Media_FileFormatNotSupported), exceptionForHR);

                    case (int)MediaPlayerErrors.NS_E_WMP_INVALID_ASX:
                        return new System.IO.FileFormatException(SR.Get(SRID.Media_PlaylistFormatNotSupported), exceptionForHR);

                    case (int)MILErrors.WGXERR_BADNUMBER:
                        return new System.ArithmeticException(SR.Get(SRID.Geometry_BadNumber), exceptionForHR);

                    case (int)MILErrors.WGXERR_D3DI_INVALIDSURFACEUSAGE:
                        return new System.ArgumentException(SR.Get(SRID.D3DImage_InvalidUsage), exceptionForHR);
                    case (int)MILErrors.WGXERR_D3DI_INVALIDSURFACESIZE:
                        return new System.ArgumentException(SR.Get(SRID.D3DImage_SurfaceTooBig), exceptionForHR);
                    case (int)MILErrors.WGXERR_D3DI_INVALIDSURFACEPOOL:
                        return new System.ArgumentException(SR.Get(SRID.D3DImage_InvalidPool), exceptionForHR);
                    case (int)MILErrors.WGXERR_D3DI_INVALIDSURFACEDEVICE:
                        return new System.ArgumentException(SR.Get(SRID.D3DImage_InvalidDevice), exceptionForHR);
                    case (int)MILErrors.WGXERR_D3DI_INVALIDANTIALIASINGSETTINGS:
                        return new System.ArgumentException(SR.Get(SRID.D3DImage_AARequires9Ex), exceptionForHR);

                    default:
                        return exceptionForHR;
                }
            }
        }

        /// <summary>
        /// If the result is not a success, then throw the appropriate exception.
        /// </summary>
        /// <param name="hr"></param>
        /// <ExternalAPI/>
        public static void Check(int hr)
        {
            if (hr >= 0)
            {
                // The call succeeded, don't bother calling Marshal.ThrowExceptionForHr
                return;
            }
            else
            {
                throw ConvertHRToException(hr);
            }
        }

        /// <summary>
        /// HRESULT succeeded.
        /// </summary>
        /// <ExternalAPI/>
        public static bool Succeeded(int hr)
        {
            if (hr >= 0)
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        /// <summary>
        /// HRESULT succeeded.
        /// </summary>
        /// <ExternalAPI/>
        public static bool Failed(int hr)
        {
            return !(HRESULT.Succeeded(hr));
        }

        internal const int S_OK = 0;
        internal const int E_FAIL = unchecked((int)0x80004005);
        internal const int E_OUTOFMEMORY = unchecked((int)0x8007000E);
        internal const int D3DERR_OUTOFVIDEOMEMORY = unchecked((int)0x8876017C);
    }
    #endregion
}

