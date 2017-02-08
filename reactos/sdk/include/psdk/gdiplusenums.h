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

#ifndef _GDIPLUSENUMS_H
#define _GDIPLUSENUMS_H

typedef UINT GraphicsState;
typedef UINT GraphicsContainer;

enum Unit
{
    UnitWorld       = 0,
    UnitDisplay     = 1,
    UnitPixel       = 2,
    UnitPoint       = 3,
    UnitInch        = 4,
    UnitDocument    = 5,
    UnitMillimeter  = 6
};

enum BrushType
{
   BrushTypeSolidColor       = 0,
   BrushTypeHatchFill        = 1,
   BrushTypeTextureFill      = 2,
   BrushTypePathGradient     = 3,
   BrushTypeLinearGradient   = 4
};

enum DriverStringOptions
{
   DriverStringOptionsCmapLookup      = 1,
   DriverStringOptionsVertical        = 2,
   DriverStringOptionsRealizedAdvance = 4,
   DriverStringOptionsLimitSubpixel   = 4
};

enum FillMode
{
    FillModeAlternate   = 0,
    FillModeWinding     = 1
};

enum LineCap
{
    LineCapFlat             = 0x00,
    LineCapSquare           = 0x01,
    LineCapRound            = 0x02,
    LineCapTriangle         = 0x03,

    LineCapNoAnchor         = 0x10,
    LineCapSquareAnchor     = 0x11,
    LineCapRoundAnchor      = 0x12,
    LineCapDiamondAnchor    = 0x13,
    LineCapArrowAnchor      = 0x14,

    LineCapCustom           = 0xff,
    LineCapAnchorMask       = 0xf0
};

enum PathPointType{
    PathPointTypeStart          = 0,    /* start of a figure */
    PathPointTypeLine           = 1,
    PathPointTypeBezier         = 3,
    PathPointTypePathTypeMask   = 7,
    PathPointTypePathDashMode   = 16,   /* not used */
    PathPointTypePathMarker     = 32,
    PathPointTypeCloseSubpath   = 128,  /* end of a closed figure */
    PathPointTypeBezier3        = 3
};

enum PenType
{
   PenTypeSolidColor       = BrushTypeSolidColor,
   PenTypeHatchFill        = BrushTypeHatchFill,
   PenTypeTextureFill      = BrushTypeTextureFill,
   PenTypePathGradient     = BrushTypePathGradient,
   PenTypeLinearGradient   = BrushTypeLinearGradient,
   PenTypeUnknown          = -1
};

enum LineJoin
{
    LineJoinMiter           = 0,
    LineJoinBevel           = 1,
    LineJoinRound           = 2,
    LineJoinMiterClipped    = 3
};

enum QualityMode
{
    QualityModeInvalid  = -1,
    QualityModeDefault  = 0,
    QualityModeLow      = 1,
    QualityModeHigh     = 2
};

enum SmoothingMode
{
    SmoothingModeInvalid     = QualityModeInvalid,
    SmoothingModeDefault     = QualityModeDefault,
    SmoothingModeHighSpeed   = QualityModeLow,
    SmoothingModeHighQuality = QualityModeHigh,
    SmoothingModeNone,
    SmoothingModeAntiAlias
};

enum CompositingQuality
{
    CompositingQualityInvalid          = QualityModeInvalid,
    CompositingQualityDefault          = QualityModeDefault,
    CompositingQualityHighSpeed        = QualityModeLow,
    CompositingQualityHighQuality      = QualityModeHigh,
    CompositingQualityGammaCorrected,
    CompositingQualityAssumeLinear
};

enum InterpolationMode
{
    InterpolationModeInvalid        = QualityModeInvalid,
    InterpolationModeDefault        = QualityModeDefault,
    InterpolationModeLowQuality     = QualityModeLow,
    InterpolationModeHighQuality    = QualityModeHigh,
    InterpolationModeBilinear,
    InterpolationModeBicubic,
    InterpolationModeNearestNeighbor,
    InterpolationModeHighQualityBilinear,
    InterpolationModeHighQualityBicubic
};

enum PenAlignment
{
    PenAlignmentCenter   = 0,
    PenAlignmentInset    = 1
};

enum PixelOffsetMode
{
    PixelOffsetModeInvalid     = QualityModeInvalid,
    PixelOffsetModeDefault     = QualityModeDefault,
    PixelOffsetModeHighSpeed   = QualityModeLow,
    PixelOffsetModeHighQuality = QualityModeHigh,
    PixelOffsetModeNone,
    PixelOffsetModeHalf
};

enum DashCap
{
    DashCapFlat     = 0,
    DashCapRound    = 2,
    DashCapTriangle = 3
};

enum DashStyle
{
    DashStyleSolid,
    DashStyleDash,
    DashStyleDot,
    DashStyleDashDot,
    DashStyleDashDotDot,
    DashStyleCustom
};

enum MatrixOrder
{
    MatrixOrderPrepend = 0,
    MatrixOrderAppend  = 1
};

enum ImageType
{
    ImageTypeUnknown,
    ImageTypeBitmap,
    ImageTypeMetafile
};

enum WarpMode {
    WarpModePerspective,
    WarpModeBilinear
};

enum WrapMode
{
    WrapModeTile,
    WrapModeTileFlipX,
    WrapModeTileFlipY,
    WrapModeTileFlipXY,
    WrapModeClamp
};

enum MetafileType
{
    MetafileTypeInvalid,
    MetafileTypeWmf,
    MetafileTypeWmfPlaceable,
    MetafileTypeEmf,
    MetafileTypeEmfPlusOnly,
    MetafileTypeEmfPlusDual
};

enum LinearGradientMode
{
    LinearGradientModeHorizontal,
    LinearGradientModeVertical,
    LinearGradientModeForwardDiagonal,
    LinearGradientModeBackwardDiagonal
};

enum EmfType
{
    EmfTypeEmfOnly     = MetafileTypeEmf,
    EmfTypeEmfPlusOnly = MetafileTypeEmfPlusOnly,
    EmfTypeEmfPlusDual = MetafileTypeEmfPlusDual
};

enum CompositingMode
{
    CompositingModeSourceOver,
    CompositingModeSourceCopy
};

enum TextRenderingHint
{
    TextRenderingHintSystemDefault = 0,
    TextRenderingHintSingleBitPerPixelGridFit,
    TextRenderingHintSingleBitPerPixel,
    TextRenderingHintAntiAliasGridFit,
    TextRenderingHintAntiAlias,
    TextRenderingHintClearTypeGridFit
};

enum StringAlignment
{
    StringAlignmentNear    = 0,
    StringAlignmentCenter  = 1,
    StringAlignmentFar     = 2
};

enum  StringDigitSubstitute
{
    StringDigitSubstituteUser        = 0,
    StringDigitSubstituteNone        = 1,
    StringDigitSubstituteNational    = 2,
    StringDigitSubstituteTraditional = 3
};

enum StringFormatFlags
{
    StringFormatFlagsDirectionRightToLeft  = 0x00000001,
    StringFormatFlagsDirectionVertical     = 0x00000002,
    StringFormatFlagsNoFitBlackBox         = 0x00000004,
    StringFormatFlagsDisplayFormatControl  = 0x00000020,
    StringFormatFlagsNoFontFallback        = 0x00000400,
    StringFormatFlagsMeasureTrailingSpaces = 0x00000800,
    StringFormatFlagsNoWrap                = 0x00001000,
    StringFormatFlagsLineLimit             = 0x00002000,
    StringFormatFlagsNoClip                = 0x00004000
};

enum StringTrimming
{
    StringTrimmingNone                 = 0,
    StringTrimmingCharacter            = 1,
    StringTrimmingWord                 = 2,
    StringTrimmingEllipsisCharacter    = 3,
    StringTrimmingEllipsisWord         = 4,
    StringTrimmingEllipsisPath         = 5
};

enum FontStyle
{
    FontStyleRegular    = 0,
    FontStyleBold       = 1,
    FontStyleItalic     = 2,
    FontStyleBoldItalic = 3,
    FontStyleUnderline  = 4,
    FontStyleStrikeout  = 8
};

enum HotkeyPrefix
{
    HotkeyPrefixNone   = 0,
    HotkeyPrefixShow   = 1,
    HotkeyPrefixHide   = 2
};

enum ImageCodecFlags
{
    ImageCodecFlagsEncoder          = 1,
    ImageCodecFlagsDecoder          = 2,
    ImageCodecFlagsSupportBitmap    = 4,
    ImageCodecFlagsSupportVector    = 8,
    ImageCodecFlagsSeekableEncode   = 16,
    ImageCodecFlagsBlockingDecode   = 32,
    ImageCodecFlagsBuiltin          = 65536,
    ImageCodecFlagsSystem           = 131072,
    ImageCodecFlagsUser             = 262144
};

enum ImageFlags
{
    ImageFlagsNone              = 0,
    ImageFlagsScalable          = 0x0001,
    ImageFlagsHasAlpha          = 0x0002,
    ImageFlagsHasTranslucent    = 0x0004,
    ImageFlagsPartiallyScalable = 0x0008,
    ImageFlagsColorSpaceRGB     = 0x0010,
    ImageFlagsColorSpaceCMYK    = 0x0020,
    ImageFlagsColorSpaceGRAY    = 0x0040,
    ImageFlagsColorSpaceYCBCR   = 0x0080,
    ImageFlagsColorSpaceYCCK    = 0x0100,
    ImageFlagsHasRealDPI        = 0x1000,
    ImageFlagsHasRealPixelSize  = 0x2000,
    ImageFlagsReadOnly          = 0x00010000,
    ImageFlagsCaching           = 0x00020000
};

enum CombineMode
{
    CombineModeReplace,
    CombineModeIntersect,
    CombineModeUnion,
    CombineModeXor,
    CombineModeExclude,
    CombineModeComplement
};

enum FlushIntention
{
    FlushIntentionFlush = 0,
    FlushIntentionSync  = 1
};

enum CoordinateSpace
{
    CoordinateSpaceWorld,
    CoordinateSpacePage,
    CoordinateSpaceDevice
};

enum GpTestControlEnum
{
    TestControlForceBilinear  = 0,
    TestControlNoICM          = 1,
    TestControlGetBuildNumber = 2
};

enum MetafileFrameUnit
{
    MetafileFrameUnitPixel      = UnitPixel,
    MetafileFrameUnitPoint      = UnitPoint,
    MetafileFrameUnitInch       = UnitInch,
    MetafileFrameUnitDocument   = UnitDocument,
    MetafileFrameUnitMillimeter = UnitMillimeter,
    MetafileFrameUnitGdi
};

enum HatchStyle
{
	HatchStyleHorizontal = 0,
	HatchStyleVertical = 1,
	HatchStyleForwardDiagonal = 2,
	HatchStyleBackwardDiagonal = 3,
	HatchStyleCross = 4,
	HatchStyleDiagonalCross = 5,
	HatchStyle05Percent = 6,
	HatchStyle10Percent = 7,
	HatchStyle20Percent = 8,
	HatchStyle25Percent = 9,
	HatchStyle30Percent = 10,
	HatchStyle40Percent = 11,
	HatchStyle50Percent = 12,
	HatchStyle60Percent = 13,
	HatchStyle70Percent = 14,
	HatchStyle75Percent = 15,
	HatchStyle80Percent = 16,
	HatchStyle90Percent = 17,
	HatchStyleLightDownwardDiagonal = 18,
	HatchStyleLightUpwardDiagonal = 19,
	HatchStyleDarkDownwardDiagonal = 20,
	HatchStyleDarkUpwardDiagonal = 21,
	HatchStyleWideDownwardDiagonal = 22,
	HatchStyleWideUpwardDiagonal = 23,
	HatchStyleLightVertical = 24,
	HatchStyleLightHorizontal = 25,
	HatchStyleNarrowVertical = 26,
	HatchStyleNarrowHorizontal = 27,
	HatchStyleDarkVertical = 28,
	HatchStyleDarkHorizontal = 29,
	HatchStyleDashedDownwardDiagonal = 30,
	HatchStyleDashedUpwardDiagonal = 31,
	HatchStyleDashedHorizontal = 32,
	HatchStyleDashedVertical = 33,
	HatchStyleSmallConfetti = 34,
	HatchStyleLargeConfetti = 35,
	HatchStyleZigZag = 36,
	HatchStyleWave = 37,
	HatchStyleDiagonalBrick = 38,
	HatchStyleHorizontalBrick = 39,
	HatchStyleWeave = 40,
	HatchStylePlaid = 41,
	HatchStyleDivot = 42,
	HatchStyleDottedGrid = 43,
	HatchStyleDottedDiamond = 44,
	HatchStyleShingle = 45,
	HatchStyleTrellis = 46,
	HatchStyleSphere = 47,
	HatchStyleSmallGrid = 48,
	HatchStyleSmallCheckerBoard = 49,
	HatchStyleLargeCheckerBoard = 50,
	HatchStyleOutlinedDiamond = 51,
	HatchStyleSolidDiamond = 52,
	HatchStyleTotal = 53,
	HatchStyleLargeGrid = HatchStyleCross,
	HatchStyleMin = HatchStyleHorizontal,
	HatchStyleMax = HatchStyleTotal - 1
};

#define GDIP_EMFPLUS_RECORD_BASE 0x00004000
#define GDIP_WMF_RECORD_BASE 0x00010000
#define GDIP_WMF_RECORD_TO_EMFPLUS(x) ((x)|GDIP_WMF_RECORD_BASE)

enum EmfPlusRecordType {
    WmfRecordTypeSetBkColor = GDIP_WMF_RECORD_TO_EMFPLUS(META_SETBKCOLOR),
    WmfRecordTypeSetBkMode = GDIP_WMF_RECORD_TO_EMFPLUS(META_SETBKMODE),
    WmfRecordTypeSetMapMode = GDIP_WMF_RECORD_TO_EMFPLUS(META_SETMAPMODE),
    WmfRecordTypeSetROP2 = GDIP_WMF_RECORD_TO_EMFPLUS(META_SETROP2),
    WmfRecordTypeSetRelAbs = GDIP_WMF_RECORD_TO_EMFPLUS(META_SETRELABS),
    WmfRecordTypeSetPolyFillMode = GDIP_WMF_RECORD_TO_EMFPLUS(META_SETPOLYFILLMODE),
    WmfRecordTypeSetStretchBltMode = GDIP_WMF_RECORD_TO_EMFPLUS(META_SETSTRETCHBLTMODE),
    WmfRecordTypeSetTextCharExtra = GDIP_WMF_RECORD_TO_EMFPLUS(META_SETTEXTCHAREXTRA),
    WmfRecordTypeSetTextColor = GDIP_WMF_RECORD_TO_EMFPLUS(META_SETTEXTCOLOR),
    WmfRecordTypeSetTextJustification = GDIP_WMF_RECORD_TO_EMFPLUS(META_SETTEXTJUSTIFICATION),
    WmfRecordTypeSetWindowOrg = GDIP_WMF_RECORD_TO_EMFPLUS(META_SETWINDOWORG),
    WmfRecordTypeSetWindowExt = GDIP_WMF_RECORD_TO_EMFPLUS(META_SETWINDOWEXT),
    WmfRecordTypeSetViewportOrg = GDIP_WMF_RECORD_TO_EMFPLUS(META_SETVIEWPORTORG),
    WmfRecordTypeSetViewportExt = GDIP_WMF_RECORD_TO_EMFPLUS(META_SETVIEWPORTEXT),
    WmfRecordTypeOffsetWindowOrg = GDIP_WMF_RECORD_TO_EMFPLUS(META_OFFSETWINDOWORG),
    WmfRecordTypeScaleWindowExt = GDIP_WMF_RECORD_TO_EMFPLUS(META_SCALEWINDOWEXT),
    WmfRecordTypeOffsetViewportOrg = GDIP_WMF_RECORD_TO_EMFPLUS(META_OFFSETVIEWPORTORG),
    WmfRecordTypeScaleViewportExt = GDIP_WMF_RECORD_TO_EMFPLUS(META_SCALEVIEWPORTEXT),
    WmfRecordTypeLineTo = GDIP_WMF_RECORD_TO_EMFPLUS(META_LINETO),
    WmfRecordTypeMoveTo = GDIP_WMF_RECORD_TO_EMFPLUS(META_MOVETO),
    WmfRecordTypeExcludeClipRect = GDIP_WMF_RECORD_TO_EMFPLUS(META_EXCLUDECLIPRECT),
    WmfRecordTypeIntersectClipRect = GDIP_WMF_RECORD_TO_EMFPLUS(META_INTERSECTCLIPRECT),
    WmfRecordTypeArc = GDIP_WMF_RECORD_TO_EMFPLUS(META_ARC),
    WmfRecordTypeEllipse = GDIP_WMF_RECORD_TO_EMFPLUS(META_ELLIPSE),
    WmfRecordTypeFloodFill = GDIP_WMF_RECORD_TO_EMFPLUS(META_FLOODFILL),
    WmfRecordTypePie = GDIP_WMF_RECORD_TO_EMFPLUS(META_PIE),
    WmfRecordTypeRectangle = GDIP_WMF_RECORD_TO_EMFPLUS(META_RECTANGLE),
    WmfRecordTypeRoundRect = GDIP_WMF_RECORD_TO_EMFPLUS(META_ROUNDRECT),
    WmfRecordTypePatBlt = GDIP_WMF_RECORD_TO_EMFPLUS(META_PATBLT),
    WmfRecordTypeSaveDC = GDIP_WMF_RECORD_TO_EMFPLUS(META_SAVEDC),
    WmfRecordTypeSetPixel = GDIP_WMF_RECORD_TO_EMFPLUS(META_SETPIXEL),
    WmfRecordTypeOffsetClipRgn = GDIP_WMF_RECORD_TO_EMFPLUS(META_OFFSETCLIPRGN),
    WmfRecordTypeTextOut = GDIP_WMF_RECORD_TO_EMFPLUS(META_TEXTOUT),
    WmfRecordTypeBitBlt = GDIP_WMF_RECORD_TO_EMFPLUS(META_BITBLT),
    WmfRecordTypeStretchBlt = GDIP_WMF_RECORD_TO_EMFPLUS(META_STRETCHBLT),
    WmfRecordTypePolygon = GDIP_WMF_RECORD_TO_EMFPLUS(META_POLYGON),
    WmfRecordTypePolyline = GDIP_WMF_RECORD_TO_EMFPLUS(META_POLYLINE),
    WmfRecordTypeEscape = GDIP_WMF_RECORD_TO_EMFPLUS(META_ESCAPE),
    WmfRecordTypeRestoreDC = GDIP_WMF_RECORD_TO_EMFPLUS(META_RESTOREDC),
    WmfRecordTypeFillRegion = GDIP_WMF_RECORD_TO_EMFPLUS(META_FILLREGION),
    WmfRecordTypeFrameRegion = GDIP_WMF_RECORD_TO_EMFPLUS(META_FRAMEREGION),
    WmfRecordTypeInvertRegion = GDIP_WMF_RECORD_TO_EMFPLUS(META_INVERTREGION),
    WmfRecordTypePaintRegion = GDIP_WMF_RECORD_TO_EMFPLUS(META_PAINTREGION),
    WmfRecordTypeSelectClipRegion = GDIP_WMF_RECORD_TO_EMFPLUS(META_SELECTCLIPREGION),
    WmfRecordTypeSelectObject = GDIP_WMF_RECORD_TO_EMFPLUS(META_SELECTOBJECT),
    WmfRecordTypeSetTextAlign = GDIP_WMF_RECORD_TO_EMFPLUS(META_SETTEXTALIGN),
    WmfRecordTypeDrawText = GDIP_WMF_RECORD_TO_EMFPLUS(0x062F),
    WmfRecordTypeChord = GDIP_WMF_RECORD_TO_EMFPLUS(META_CHORD),
    WmfRecordTypeSetMapperFlags = GDIP_WMF_RECORD_TO_EMFPLUS(META_SETMAPPERFLAGS),
    WmfRecordTypeExtTextOut = GDIP_WMF_RECORD_TO_EMFPLUS(META_EXTTEXTOUT),
    WmfRecordTypeSetDIBToDev = GDIP_WMF_RECORD_TO_EMFPLUS(META_SETDIBTODEV),
    WmfRecordTypeSelectPalette = GDIP_WMF_RECORD_TO_EMFPLUS(META_SELECTPALETTE),
    WmfRecordTypeRealizePalette = GDIP_WMF_RECORD_TO_EMFPLUS(META_REALIZEPALETTE),
    WmfRecordTypeAnimatePalette = GDIP_WMF_RECORD_TO_EMFPLUS(META_ANIMATEPALETTE),
    WmfRecordTypeSetPalEntries = GDIP_WMF_RECORD_TO_EMFPLUS(META_SETPALENTRIES),
    WmfRecordTypePolyPolygon = GDIP_WMF_RECORD_TO_EMFPLUS(META_POLYPOLYGON),
    WmfRecordTypeResizePalette = GDIP_WMF_RECORD_TO_EMFPLUS(META_RESIZEPALETTE),
    WmfRecordTypeDIBBitBlt = GDIP_WMF_RECORD_TO_EMFPLUS(META_DIBBITBLT),
    WmfRecordTypeDIBStretchBlt = GDIP_WMF_RECORD_TO_EMFPLUS(META_DIBSTRETCHBLT),
    WmfRecordTypeDIBCreatePatternBrush = GDIP_WMF_RECORD_TO_EMFPLUS(META_DIBCREATEPATTERNBRUSH),
    WmfRecordTypeStretchDIB = GDIP_WMF_RECORD_TO_EMFPLUS(META_STRETCHDIB),
    WmfRecordTypeExtFloodFill = GDIP_WMF_RECORD_TO_EMFPLUS(META_EXTFLOODFILL),
    WmfRecordTypeSetLayout = GDIP_WMF_RECORD_TO_EMFPLUS(0x0149),
    WmfRecordTypeResetDC = GDIP_WMF_RECORD_TO_EMFPLUS(0x014C),
    WmfRecordTypeStartDoc = GDIP_WMF_RECORD_TO_EMFPLUS(0x014D),
    WmfRecordTypeStartPage = GDIP_WMF_RECORD_TO_EMFPLUS(0x004F),
    WmfRecordTypeEndPage = GDIP_WMF_RECORD_TO_EMFPLUS(0x0050),
    WmfRecordTypeAbortDoc = GDIP_WMF_RECORD_TO_EMFPLUS(0x0052),
    WmfRecordTypeEndDoc = GDIP_WMF_RECORD_TO_EMFPLUS(0x005E),
    WmfRecordTypeDeleteObject = GDIP_WMF_RECORD_TO_EMFPLUS(META_DELETEOBJECT),
    WmfRecordTypeCreatePalette = GDIP_WMF_RECORD_TO_EMFPLUS(META_CREATEPALETTE),
    WmfRecordTypeCreateBrush = GDIP_WMF_RECORD_TO_EMFPLUS(0x00F8),
    WmfRecordTypeCreatePatternBrush = GDIP_WMF_RECORD_TO_EMFPLUS(META_CREATEPATTERNBRUSH),
    WmfRecordTypeCreatePenIndirect = GDIP_WMF_RECORD_TO_EMFPLUS(META_CREATEPENINDIRECT),
    WmfRecordTypeCreateFontIndirect = GDIP_WMF_RECORD_TO_EMFPLUS(META_CREATEFONTINDIRECT),
    WmfRecordTypeCreateBrushIndirect = GDIP_WMF_RECORD_TO_EMFPLUS(META_CREATEBRUSHINDIRECT),
    WmfRecordTypeCreateBitmapIndirect = GDIP_WMF_RECORD_TO_EMFPLUS(0x02FD),
    WmfRecordTypeCreateBitmap = GDIP_WMF_RECORD_TO_EMFPLUS(0x06FE),
    WmfRecordTypeCreateRegion = GDIP_WMF_RECORD_TO_EMFPLUS(META_CREATEREGION),
    EmfRecordTypeHeader = EMR_HEADER,
    EmfRecordTypePolyBezier = EMR_POLYBEZIER,
    EmfRecordTypePolygon = EMR_POLYGON,
    EmfRecordTypePolyline = EMR_POLYLINE,
    EmfRecordTypePolyBezierTo = EMR_POLYBEZIERTO,
    EmfRecordTypePolyLineTo = EMR_POLYLINETO,
    EmfRecordTypePolyPolyline = EMR_POLYPOLYLINE,
    EmfRecordTypePolyPolygon = EMR_POLYPOLYGON,
    EmfRecordTypeSetWindowExtEx = EMR_SETWINDOWEXTEX,
    EmfRecordTypeSetWindowOrgEx = EMR_SETWINDOWORGEX,
    EmfRecordTypeSetViewportExtEx = EMR_SETVIEWPORTEXTEX,
    EmfRecordTypeSetViewportOrgEx = EMR_SETVIEWPORTORGEX,
    EmfRecordTypeSetBrushOrgEx = EMR_SETBRUSHORGEX,
    EmfRecordTypeEOF = EMR_EOF,
    EmfRecordTypeSetPixelV = EMR_SETPIXELV,
    EmfRecordTypeSetMapperFlags = EMR_SETMAPPERFLAGS,
    EmfRecordTypeSetMapMode = EMR_SETMAPMODE,
    EmfRecordTypeSetBkMode = EMR_SETBKMODE,
    EmfRecordTypeSetPolyFillMode = EMR_SETPOLYFILLMODE,
    EmfRecordTypeSetROP2 = EMR_SETROP2,
    EmfRecordTypeSetStretchBltMode = EMR_SETSTRETCHBLTMODE,
    EmfRecordTypeSetTextAlign = EMR_SETTEXTALIGN,
    EmfRecordTypeSetColorAdjustment = EMR_SETCOLORADJUSTMENT,
    EmfRecordTypeSetTextColor = EMR_SETTEXTCOLOR,
    EmfRecordTypeSetBkColor = EMR_SETBKCOLOR,
    EmfRecordTypeOffsetClipRgn = EMR_OFFSETCLIPRGN,
    EmfRecordTypeMoveToEx = EMR_MOVETOEX,
    EmfRecordTypeSetMetaRgn = EMR_SETMETARGN,
    EmfRecordTypeExcludeClipRect = EMR_EXCLUDECLIPRECT,
    EmfRecordTypeIntersectClipRect = EMR_INTERSECTCLIPRECT,
    EmfRecordTypeScaleViewportExtEx = EMR_SCALEVIEWPORTEXTEX,
    EmfRecordTypeScaleWindowExtEx = EMR_SCALEWINDOWEXTEX,
    EmfRecordTypeSaveDC = EMR_SAVEDC,
    EmfRecordTypeRestoreDC = EMR_RESTOREDC,
    EmfRecordTypeSetWorldTransform = EMR_SETWORLDTRANSFORM,
    EmfRecordTypeModifyWorldTransform = EMR_MODIFYWORLDTRANSFORM,
    EmfRecordTypeSelectObject = EMR_SELECTOBJECT,
    EmfRecordTypeCreatePen = EMR_CREATEPEN,
    EmfRecordTypeCreateBrushIndirect = EMR_CREATEBRUSHINDIRECT,
    EmfRecordTypeDeleteObject = EMR_DELETEOBJECT,
    EmfRecordTypeAngleArc = EMR_ANGLEARC,
    EmfRecordTypeEllipse = EMR_ELLIPSE,
    EmfRecordTypeRectangle = EMR_RECTANGLE,
    EmfRecordTypeRoundRect = EMR_ROUNDRECT,
    EmfRecordTypeArc = EMR_ARC,
    EmfRecordTypeChord = EMR_CHORD,
    EmfRecordTypePie = EMR_PIE,
    EmfRecordTypeSelectPalette = EMR_SELECTPALETTE,
    EmfRecordTypeCreatePalette = EMR_CREATEPALETTE,
    EmfRecordTypeSetPaletteEntries = EMR_SETPALETTEENTRIES,
    EmfRecordTypeResizePalette = EMR_RESIZEPALETTE,
    EmfRecordTypeRealizePalette = EMR_REALIZEPALETTE,
    EmfRecordTypeExtFloodFill = EMR_EXTFLOODFILL,
    EmfRecordTypeLineTo = EMR_LINETO,
    EmfRecordTypeArcTo = EMR_ARCTO,
    EmfRecordTypePolyDraw = EMR_POLYDRAW,
    EmfRecordTypeSetArcDirection = EMR_SETARCDIRECTION,
    EmfRecordTypeSetMiterLimit = EMR_SETMITERLIMIT,
    EmfRecordTypeBeginPath = EMR_BEGINPATH,
    EmfRecordTypeEndPath = EMR_ENDPATH,
    EmfRecordTypeCloseFigure = EMR_CLOSEFIGURE,
    EmfRecordTypeFillPath = EMR_FILLPATH,
    EmfRecordTypeStrokeAndFillPath = EMR_STROKEANDFILLPATH,
    EmfRecordTypeStrokePath = EMR_STROKEPATH,
    EmfRecordTypeFlattenPath = EMR_FLATTENPATH,
    EmfRecordTypeWidenPath = EMR_WIDENPATH,
    EmfRecordTypeSelectClipPath = EMR_SELECTCLIPPATH,
    EmfRecordTypeAbortPath = EMR_ABORTPATH,
    EmfRecordTypeReserved_069 = 69,
    EmfRecordTypeGdiComment = EMR_GDICOMMENT,
    EmfRecordTypeFillRgn = EMR_FILLRGN,
    EmfRecordTypeFrameRgn = EMR_FRAMERGN,
    EmfRecordTypeInvertRgn = EMR_INVERTRGN,
    EmfRecordTypePaintRgn = EMR_PAINTRGN,
    EmfRecordTypeExtSelectClipRgn = EMR_EXTSELECTCLIPRGN,
    EmfRecordTypeBitBlt = EMR_BITBLT,
    EmfRecordTypeStretchBlt = EMR_STRETCHBLT,
    EmfRecordTypeMaskBlt = EMR_MASKBLT,
    EmfRecordTypePlgBlt = EMR_PLGBLT,
    EmfRecordTypeSetDIBitsToDevice = 80,
    EmfRecordTypeStretchDIBits = EMR_STRETCHDIBITS,
    EmfRecordTypeExtCreateFontIndirect = EMR_EXTCREATEFONTINDIRECTW,
    EmfRecordTypeExtTextOutA = EMR_EXTTEXTOUTA,
    EmfRecordTypeExtTextOutW = EMR_EXTTEXTOUTW,
    EmfRecordTypePolyBezier16 = EMR_POLYBEZIER16,
    EmfRecordTypePolygon16 = EMR_POLYGON16,
    EmfRecordTypePolyline16 = EMR_POLYLINE16,
    EmfRecordTypePolyBezierTo16 = EMR_POLYBEZIERTO16,
    EmfRecordTypePolylineTo16 = EMR_POLYLINETO16,
    EmfRecordTypePolyPolyline16 = EMR_POLYPOLYLINE16,
    EmfRecordTypePolyPolygon16 = EMR_POLYPOLYGON16,
    EmfRecordTypePolyDraw16 = EMR_POLYDRAW16,
    EmfRecordTypeCreateMonoBrush = EMR_CREATEMONOBRUSH,
    EmfRecordTypeCreateDIBPatternBrushPt = EMR_CREATEDIBPATTERNBRUSHPT,
    EmfRecordTypeExtCreatePen = EMR_EXTCREATEPEN,
    EmfRecordTypePolyTextOutA = EMR_POLYTEXTOUTA,
    EmfRecordTypePolyTextOutW = EMR_POLYTEXTOUTW,
    EmfRecordTypeSetICMMode = 98,
    EmfRecordTypeCreateColorSpace = 99,
    EmfRecordTypeSetColorSpace = 100,
    EmfRecordTypeDeleteColorSpace = 101,
    EmfRecordTypeGLSRecord = 102,
    EmfRecordTypeGLSBoundedRecord = 103,
    EmfRecordTypePixelFormat = 104,
    EmfRecordTypeDrawEscape = 105,
    EmfRecordTypeExtEscape = 106,
    EmfRecordTypeStartDoc = 107,
    EmfRecordTypeSmallTextOut = 108,
    EmfRecordTypeForceUFIMapping = 109,
    EmfRecordTypeNamedEscape = 110,
    EmfRecordTypeColorCorrectPalette = 111,
    EmfRecordTypeSetICMProfileA = 112,
    EmfRecordTypeSetICMProfileW = 113,
    EmfRecordTypeAlphaBlend = 114,
    EmfRecordTypeSetLayout = 115,
    EmfRecordTypeTransparentBlt = 116,
    EmfRecordTypeReserved_117 = 117,
    EmfRecordTypeGradientFill = 118,
    EmfRecordTypeSetLinkedUFIs = 119,
    EmfRecordTypeSetTextJustification = 120,
    EmfRecordTypeColorMatchToTargetW = 121,
    EmfRecordTypeCreateColorSpaceW = 122,
    EmfRecordTypeMax = 122,
    EmfRecordTypeMin = 1,
    EmfPlusRecordTypeInvalid = GDIP_EMFPLUS_RECORD_BASE,
    EmfPlusRecordTypeHeader,
    EmfPlusRecordTypeEndOfFile,
    EmfPlusRecordTypeComment,
    EmfPlusRecordTypeGetDC,
    EmfPlusRecordTypeMultiFormatStart,
    EmfPlusRecordTypeMultiFormatSection,
    EmfPlusRecordTypeMultiFormatEnd,
    EmfPlusRecordTypeObject,
    EmfPlusRecordTypeClear,
    EmfPlusRecordTypeFillRects,
    EmfPlusRecordTypeDrawRects,
    EmfPlusRecordTypeFillPolygon,
    EmfPlusRecordTypeDrawLines,
    EmfPlusRecordTypeFillEllipse,
    EmfPlusRecordTypeDrawEllipse,
    EmfPlusRecordTypeFillPie,
    EmfPlusRecordTypeDrawPie,
    EmfPlusRecordTypeDrawArc,
    EmfPlusRecordTypeFillRegion,
    EmfPlusRecordTypeFillPath,
    EmfPlusRecordTypeDrawPath,
    EmfPlusRecordTypeFillClosedCurve,
    EmfPlusRecordTypeDrawClosedCurve,
    EmfPlusRecordTypeDrawCurve,
    EmfPlusRecordTypeDrawBeziers,
    EmfPlusRecordTypeDrawImage,
    EmfPlusRecordTypeDrawImagePoints,
    EmfPlusRecordTypeDrawString,
    EmfPlusRecordTypeSetRenderingOrigin,
    EmfPlusRecordTypeSetAntiAliasMode,
    EmfPlusRecordTypeSetTextRenderingHint,
    EmfPlusRecordTypeSetTextContrast,
    EmfPlusRecordTypeSetInterpolationMode,
    EmfPlusRecordTypeSetPixelOffsetMode,
    EmfPlusRecordTypeSetCompositingMode,
    EmfPlusRecordTypeSetCompositingQuality,
    EmfPlusRecordTypeSave,
    EmfPlusRecordTypeRestore,
    EmfPlusRecordTypeBeginContainer,
    EmfPlusRecordTypeBeginContainerNoParams,
    EmfPlusRecordTypeEndContainer,
    EmfPlusRecordTypeSetWorldTransform,
    EmfPlusRecordTypeResetWorldTransform,
    EmfPlusRecordTypeMultiplyWorldTransform,
    EmfPlusRecordTypeTranslateWorldTransform,
    EmfPlusRecordTypeScaleWorldTransform,
    EmfPlusRecordTypeRotateWorldTransform,
    EmfPlusRecordTypeSetPageTransform,
    EmfPlusRecordTypeResetClip,
    EmfPlusRecordTypeSetClipRect,
    EmfPlusRecordTypeSetClipPath,
    EmfPlusRecordTypeSetClipRegion,
    EmfPlusRecordTypeOffsetClip,
    EmfPlusRecordTypeDrawDriverString,
    EmfPlusRecordTypeStrokeFillPath,
    EmfPlusRecordTypeSerializableObject,
    EmfPlusRecordTypeSetTSGraphics,
    EmfPlusRecordTypeSetTSClip,
    EmfPlusRecordTotal,
    EmfPlusRecordTypeMax = EmfPlusRecordTotal-1,
    EmfPlusRecordTypeMin = EmfPlusRecordTypeHeader
};

#define FlatnessDefault 0.25f

#ifndef __cplusplus

typedef enum Unit Unit;
typedef enum BrushType BrushType;
typedef enum DriverStringOptions DriverStringOptions;
typedef enum FillMode FillMode;
typedef enum LineCap LineCap;
typedef enum PathPointType PathPointType;
typedef enum LineJoin LineJoin;
typedef enum QualityMode QualityMode;
typedef enum SmoothingMode SmoothingMode;
typedef enum CompositingQuality CompositingQuality;
typedef enum InterpolationMode InterpolationMode;
typedef enum PixelOffsetMode PixelOffsetMode;
typedef enum DashCap DashCap;
typedef enum DashStyle DashStyle;
typedef enum MatrixOrder MatrixOrder;
typedef enum ImageType ImageType;
typedef enum ImageFlags ImageFlags;
typedef enum WarpMode WarpMode;
typedef enum WrapMode WrapMode;
typedef enum MetafileType MetafileType;
typedef enum LinearGradientMode LinearGradientMode;
typedef enum EmfType EmfType;
typedef enum CompositingMode CompositingMode;
typedef enum TextRenderingHint TextRenderingHint;
typedef enum StringAlignment StringAlignment;
typedef enum StringDigitSubstitute StringDigitSubstitute;
typedef enum StringTrimming StringTrimming;
typedef enum FontStyle FontStyle;
typedef enum StringFormatFlags StringFormatFlags;
typedef enum HotkeyPrefix HotkeyPrefix;
typedef enum PenAlignment PenAlignment;
typedef enum PaletteFlags PaletteFlags;
typedef enum ImageCodecFlags ImageCodecFlags;
typedef enum CombineMode CombineMode;
typedef enum FlushIntention FlushIntention;
typedef enum CoordinateSpace CoordinateSpace;
typedef enum GpTestControlEnum GpTestControlEnum;
typedef enum MetafileFrameUnit MetafileFrameUnit;
typedef enum PenType PenType;
typedef enum HatchStyle HatchStyle;
typedef enum EmfPlusRecordType EmfPlusRecordType;

#endif /* end of c typedefs */

#undef GDIP_WMF_RECORD_TO_EMFPLUS
#define GDIP_WMF_RECORD_TO_EMFPLUS(x) ((EmfPlusRecordType)((x)|GDIP_WMF_RECORD_BASE))

#endif
