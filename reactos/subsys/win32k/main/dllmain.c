/*
 *  Entry Point for win32k.sys
 */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <ddk/winddi.h>
#include <internal/service.h>
#include <internal/hal.h>

#include <win32k/dc.h>
#include <win32k/bitmaps.h>
#include <win32k/brush.h>

SERVICE_TABLE W32kServiceTable[] =
{
#if 0
  {?, (ULONG) W32kAbortDoc},
  {?, (ULONG) W32kAbortPath},
  {?, (ULONG) W32kAddFontResource},
  {?, (ULONG) W32kAngleArc},
  {?, (ULONG) W32kAnimatePalette},
  {?, (ULONG) W32kArc},
  {?, (ULONG) W32kArcTo},
  {?, (ULONG) W32kBeginPath},
#endif
  {36, (ULONG) W32kBitBlt},
  {4, (ULONG) W32kCancelDC},
#if 0
  {?, (ULONG) W32kCheckColorsInGamut},
  {?, (ULONG) W32kChoosePixelFormat},
  {?, (ULONG) W32kChord},
  {?, (ULONG) W32kCloseEnhMetaFile},
  {?, (ULONG) W32kCloseFigure},
  {?, (ULONG) W32kCloseMetaFile},
  {?, (ULONG) W32kColorMatchToTarget},
  {?, (ULONG) W32kCombineRgn},
  {?, (ULONG) W32kCombineTransform},
  {?, (ULONG) W32kCopyEnhMetaFile},
  {?, (ULONG) W32kCopyMetaFile},
#endif
  {20, (ULONG) W32kCreateBitmap},
  {4, (ULONG) W32kCreateBitmapIndirect},
  {4, (ULONG) W32kCreateBrushIndirect},
#if 0
  {?, (ULONG) W32kCreateColorSpace},
  {?, (ULONG) W32kCreateCompatibleBitmap},
#endif
  {4, (ULONG) W32kCreateCompatableDC},
  {16, (ULONG) W32kCreateDC},
  {24, (ULONG) W32kCreateDIBitmap},
  {8, (ULONG) W32kCreateDIBPatternBrush},
  {8, (ULONG) W32kCreateDIBPatternBrushPt},
  {24, (ULONG) W32kCreateDIBSection},
  {12, (ULONG) W32kCreateDiscardableBitmap},
#if 0
  {?, (ULONG) W32kCreateEllipticRgn},
  {?, (ULONG) W32kCreateEllipticRgnIndirect},
  {?, (ULONG) W32kCreateEnhMetaFile},
  {?, (ULONG) W32kCreateFontIndirect},
  {?, (ULONG) W32kCreateFont},
  {?, (ULONG) W32kCreateHalftonePalette},
#endif
  {8, (ULONG) W32kCreateHatchBrush},
#if 0
  {?, (ULONG) W32kCreateIC},
  {?, (ULONG) W32kCreateMetaFile},
  {?, (ULONG) W32kCreatePalette},
#endif
  {4, (ULONG) W32kCreatePatternBrush},
#if 0
  {?, (ULONG) W32kCreatePen},
  {?, (ULONG) W32kCreatePenIndirect},
  {?, (ULONG) W32kCreatePolyPolygonRgn},
  {?, (ULONG) W32kCreatePolygonRgn},
  {?, (ULONG) W32kCreateRectRgn},
  {?, (ULONG) W32kCreateRectRgnIndirect},
  {?, (ULONG) W32kCreateRoundRectRgn},
  {?, (ULONG) W32kCreateScalableFontResource},
#endif
  {4, (ULONG) W32kCreateSolidBrush},
#if 0
  {?, (ULONG) W32kDPtoLP},
  {?, (ULONG) W32kDeleteColorSpace},
#endif
  {4, (ULONG) W32kDeleteDC},
#if 0
  {?, (ULONG) W32kDeleteEnhMetaFile},
  {?, (ULONG) W32kDeleteMetaFile},
  {?, (ULONG) W32kDeleteObject},
  {?, (ULONG) W32kDescribePixelFormat},
  {?, (ULONG) W32kDeviceCapabilitiesEx},
#endif
  {16, (ULONG) W32kDrawEscape},
#if 0
  {?, (ULONG) W32kEllipse},
  {?, (ULONG) W32kEndDoc},
  {?, (ULONG) W32kEndPage},
  {?, (ULONG) W32kEndPath},
  {?, (ULONG) W32kEnumEnhMetaFile},
  {?, (ULONG) W32kEnumFontFamiliesEx},
  {?, (ULONG) W32kEnumFontFamilies},
  {?, (ULONG) W32kEnumFonts},
  {?, (ULONG) W32kEnumICMProfiles},
  {?, (ULONG) W32kEnumMetaFile},
#endif
  {16, (ULONG) W32kEnumObjects},
#if 0
  {?, (ULONG) W32kEqualRgn},
  {?, (ULONG) W32kEscape},
  {?, (ULONG) W32kExcludeClipRect},
  {?, (ULONG) W32kExtCreatePen},
  {?, (ULONG) W32kExtCreateRegion},
  {?, (ULONG) W32kExtEscape},
#endif
  {20, (ULONG) W32kExtFloodFill},
#if 0
  {?, (ULONG) W32kExtSelectClipRgn},
  {?, (ULONG) W32kExtTextOut},
  {?, (ULONG) W32kFillPath},
  {?, (ULONG) W32kFillRgn},
#endif
  {0, (ULONG) W32kFixBrushOrgEx},
#if 0
  {?, (ULONG) W32kFlattenPath},
#endif
  {16, (ULONG) W32kFloodFill},
#if 0
  {?, (ULONG) W32kFrameRgn},
  {?, (ULONG) W32kGdiComment},
  {?, (ULONG) W32kGdiFlush},
  {?, (ULONG) W32kGdiGetBatchLimit},
  {?, (ULONG) W32kGdiPlayDCScript},
  {?, (ULONG) W32kGdiPlayJournal},
  {?, (ULONG) W32kGdiPlayScript},
  {?, (ULONG) W32kGdiSetBatchLimit},
  {?, (ULONG) W32kGetArcDirection},
  {?, (ULONG) W32kGetAspectRatioFilterEx},
#endif
  {12, (ULONG) W32kGetBitmapBits},
  {8, (ULONG) W32kGetBitmapDimensionEx},
  {4, (ULONG) W32kGetBkColor},
  {4, (ULONG) W32kGetBkMode},
#if 0
  {?, (ULONG) W32kGetBoundsRect},
#endif
  {8, (ULONG) W32kGetBrushOrgEx},
#if 0
  {?, (ULONG) W32kGetCharABCWidthsFloat},
  {?, (ULONG) W32kGetCharABCWidths},
  {?, (ULONG) W32kGetCharWidth32},
  {?, (ULONG) W32kGetCharWidthFloat},
  {?, (ULONG) W32kGetCharWidth},
  {?, (ULONG) W32kGetCharacterPlacement},
  {?, (ULONG) W32kGetClipBox},
#endif
  {4, (ULONG) W32kGetClipRgn},
#if 0
  {?, (ULONG) W32kGetColorAdjustment},
  {?, (ULONG) W32kGetColorSpace},
#endif
  {8, (ULONG) W32kGetCurrentObject},
  {8, (ULONG) W32kGetCurrentPositionEx},
  {8, (ULONG) W32kGetDCOrgEx},
  {16, (ULONG) W32kGetDIBColorTable},
  {28, (ULONG) W32kGetDIBits},
  {8, (ULONG) W32kGetDeviceCaps},
#if 0
  {?, (ULONG) W32kGetDeviceGammaRamp},
  {?, (ULONG) W32kGetEnhMetaFileBits},
  {?, (ULONG) W32kGetEnhMetaFileDescription},
  {?, (ULONG) W32kGetEnhMetaFileHeader},
  {?, (ULONG) W32kGetEnhMetaFilePaletteEntries},
  {?, (ULONG) W32kGetEnhMetaFilePixelFormat},
  {?, (ULONG) W32kGetEnhMetaFile},
  {?, (ULONG) W32kGetFontLanguageInfo},
  {?, (ULONG) W32kGetFontResourceInfo},
  {?, (ULONG) W32kGetGlyphOutline},
  {?, (ULONG) W32kGetGlyphOutlineWow},
  {?, (ULONG) W32kGetGraphicsMode},
  {?, (ULONG) W32kGetICMProfile},
  {?, (ULONG) W32kGetKerningPairs},
  {?, (ULONG) W32kGetLogColorSpace},
#endif
  {4, (ULONG) W32kGetMapMode},
#if 0
  {?, (ULONG) W32kGetMetaFileBitsEx},
  {?, (ULONG) W32kGetMetaFile},
  {?, (ULONG) W32kGetMetaRgn},
  {?, (ULONG) W32kGetMiterLimit},
  {?, (ULONG) W32kGetNearestColor},
  {?, (ULONG) W32kGetNearestPaletteIndex},
  {?, (ULONG) W32kGetObject},
  {?, (ULONG) W32kGetObjectType},
  {?, (ULONG) W32kGetOutlineTextMetrics},
  {?, (ULONG) W32kGetPaletteEntries},
  {?, (ULONG) W32kGetPath},
#endif
  {4, (ULONG) W32kGetPixel},
#if 0
  {?, (ULONG) W32kGetPixelFormat},
#endif
  {4, (ULONG) W32kGetPolyFillMode},
  {4, (ULONG) W32kGetROP2},
#if 0
  {?, (ULONG) W32kGetRandomRgn},
  {?, (ULONG) W32kGetRasterizerCaps},
#endif
  {4, (ULONG) W32kGetRelAbs},
#if 0
  {?, (ULONG) W32kGetRgnBox},
  {?, (ULONG) W32kGetStockObject},
#endif
  {4, (ULONG) W32kGetStretchBltMode},
#if 0
  {?, (ULONG) W32kGetSystemPaletteEntries},
  {?, (ULONG) W32kGetSystemPaletteUse},
#endif
  {4, (ULONG) W32kGetTextAlign},
#if 0
  {?, (ULONG) W32kGetTextCharset},
  {?, (ULONG) W32kGetTextCharsetInfo},
#endif
  {4, (ULONG) W32kGetTextColor},
#if 0
  {?, (ULONG) W32kGetTextExtentExPoint},
  {?, (ULONG) W32kGetTextExtentPoint32},
  {?, (ULONG) W32kGetTextExtentPoint},
  {?, (ULONG) W32kGetTextFace},
  {?, (ULONG) W32kGetTextMetrics},
#endif
  {8, (ULONG) W32kGetViewportExtEx},
  {8, (ULONG) W32kGetViewportOrgEx},
#if 0
  {?, (ULONG) W32kGetWinMetaFileBits},
#endif
  {8, (ULONG) W32kGetWindowExtEx},
  {8, (ULONG) W32kGetWindowOrgEx},
#if 0
  {?, (ULONG) W32kGetWorldTransform},
  {?, (ULONG) W32kIntersectClipRect},
  {?, (ULONG) W32kInvertRgn},
  {?, (ULONG) W32kLPtoDP},
  {?, (ULONG) W32kLineTo},
#endif
  {48, (ULONG) W32kMaskBlt},
#if 0
  {?, (ULONG) W32kModifyWorldTransform},
  {?, (ULONG) W32kMoveToEx},
  {?, (ULONG) W32kOffsetClipRgn},
  {?, (ULONG) W32kOffsetRgn},
  {?, (ULONG) W32kOffsetViewportOrgEx},
  {?, (ULONG) W32kOffsetWindowOrgEx},
  {?, (ULONG) W32kPaintRgn},
#endif
  {24, (ULONG) W32kPatBlt},
#if 0
  {?, (ULONG) W32kPathToRegion},
  {?, (ULONG) W32kPie},
  {?, (ULONG) W32kPlayEnhMetaFile},
  {?, (ULONG) W32kPlayEnhMetaFileRecord},
  {?, (ULONG) W32kPlayMetaFile},
  {?, (ULONG) W32kPlayMetaFileRecord},
#endif
  {40, (ULONG) W32kPlgBlt},
#if 0
  {?, (ULONG) W32kPolyBezier},
  {?, (ULONG) W32kPolyBezierTo},
  {?, (ULONG) W32kPolyDraw},
  {?, (ULONG) W32kPolyPolygon},
  {?, (ULONG) W32kPolyPolyline},
  {?, (ULONG) W32kPolyTextOut},
  {?, (ULONG) W32kPolygon},
  {?, (ULONG) W32kPolyline},
  {?, (ULONG) W32kPolylineTo},
  {?, (ULONG) W32kPtInRegion},
  {?, (ULONG) W32kPtVisible},
  {?, (ULONG) W32kRealizePalette},
  {?, (ULONG) W32kRectInRegion},
  {?, (ULONG) W32kRectVisible},
  {?, (ULONG) W32kRectangle},
  {?, (ULONG) W32kRemoveFontResource},
#endif
  {8, (ULONG) W32kResetDC},
#if 0
  {?, (ULONG) W32kResizePalette},
#endif
  {8, (ULONG) W32kRestoreDC},
#if 0
  {?, (ULONG) W32kRoundRect},
#endif
  {4, (ULONG) W32kSaveDC},
#if 0
  {?, (ULONG) W32kScaleViewportExtEx},
  {?, (ULONG) W32kScaleWindowExtEx},
  {?, (ULONG) W32kSelectBrushLocal},
  {?, (ULONG) W32kSelectClipPath},
  {?, (ULONG) W32kSelectClipRgn},
  {?, (ULONG) W32kSelectFontLocal},
#endif
  {8, (ULONG) W32kSelectObject},
#if 0
  {?, (ULONG) W32kSelectPalette},
  {?, (ULONG) W32kSetAbortProc},
  {?, (ULONG) W32kSetArcDirection},
#endif
  {12, (ULONG) W32kSetBitmapBits},
  {16, (ULONG) W32kSetBitmapDimensionEx},
#if 0
  {?, (ULONG) W32kSetBkColor},
#endif
  {8, (ULONG) W32kSetBkMode},
#if 0
  {?, (ULONG) W32kSetBoundsRect},
#endif
  {16, (ULONG) W32kSetBrushOrgEx},
#if 0
  {?, (ULONG) W32kSetColorAdjustment},
  {?, (ULONG) W32kSetColorSpace},
#endif
  {16, (ULONG) W32kSetDIBColorTable},
  {28, (ULONG) W32kSetDIBits},
  {48, (ULONG) W32kSetDIBitsToDevice},
#if 0
  {?, (ULONG) W32kSetDeviceGammaRamp},
  {?, (ULONG) W32kSetEnhMetaFileBits},
  {?, (ULONG) W32kSetFontEnumeration},
  {?, (ULONG) W32kSetGraphicsMode},
  {?, (ULONG) W32kSetICMMode},
  {?, (ULONG) W32kSetICMProfile},
  {?, (ULONG) W32kSetMapMode},
  {?, (ULONG) W32kSetMapperFlags},
  {?, (ULONG) W32kSetMetaFileBitsEx},
  {?, (ULONG) W32kSetMetaRgn},
  {?, (ULONG) W32kSetMiterLimit},
  {?, (ULONG) W32kSetPaletteEntries},
#endif
  {16, (ULONG) W32kSetPixel},
#if 0
  {?, (ULONG) W32kSetPixelFormat},
#endif
  {16, (ULONG) W32kSetPixelV},
  {8, (ULONG) W32kSetPolyFillMode},
  {8, (ULONG) W32kSetROP2},
#if 0
  {?, (ULONG) W32kSetRectRgn},
#endif
//  {8, (ULONG) W32kSetRelAbs},
  {8, (ULONG) W32kSetStretchBltMode},
#if 0
  {?, (ULONG) W32kSetSystemPaletteUse},
  {?, (ULONG) W32kSetTextAlign},
  {?, (ULONG) W32kSetTextColor},
  {?, (ULONG) W32kSetTextJustification},
  {?, (ULONG) W32kSetViewportExtEx},
  {?, (ULONG) W32kSetViewportOrgEx},
  {?, (ULONG) W32kSetWinMetaFileBits},
  {?, (ULONG) W32kSetWindowExtEx},
  {?, (ULONG) W32kSetWindowOrgEx},
  {?, (ULONG) W32kSetWorldTransform},
  {?, (ULONG) W32kStartDoc},
  {?, (ULONG) W32kStartPage},
#endif
  {44, (ULONG) W32kStretchBlt},
  {52, (ULONG) W32kStretchDIBits},
#if 0
  {?, (ULONG) W32kStrokeAndFillPath},
  {?, (ULONG) W32kStrokePath},
  {?, (ULONG) W32kSwapBuffers},
  {?, (ULONG) W32kTextOut},
  {?, (ULONG) W32kTranslateCharsetInfo},
  {?, (ULONG) W32kUnrealizeObject},
  {?, (ULONG) W32kUpdateColors},
  {?, (ULONG) W32kUpdateICMRegKey},
  {?, (ULONG) W32kWidenPath},
  {?, (ULONG) W32kgdiPlaySpoolStream},
#endif
};

WINBOOL STDCALL  DllMain(VOID)
{
  NTSTATUS  Status;
  
  /*  Register user mode call interface (svc mask is 0x10000000)  */
  Status = HalRegisterServiceTable(0xF0000000, 
                                   0x10000000, 
                                   W32kServiceTable,
                                   sizeof(W32kServiceTable) / 
                                     sizeof(W32kServiceTable[0]));
  if (!NT_SUCCESS(Status))
    {
      return FALSE;
    }
  
  return TRUE;
}

