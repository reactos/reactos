/*
 *  Entry Point for win32k.sys
 */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <ddk/winddi.h>
#include <internal/service.h>
#include <internal/hal.h>

#include <win32k/win32k.h>

SERVICE_TABLE W32kServiceTable[] =
{
#if 0
  {?, (ULONG) W32kAbortDoc},
  {?, (ULONG) W32kAbortPath},
#endif
  {4, (ULONG) W32kAddFontResource},
  {24, (ULONG) W32kAngleArc},
  {16, (ULONG) W32kAnimatePalette},
  {36, (ULONG) W32kArc},
  {36, (ULONG) W32kArcTo},
#if 0
  {?, (ULONG) W32kBeginPath},
#endif
  {36, (ULONG) W32kBitBlt},
  {4, (ULONG) W32kCancelDC},
#if 0
  {?, (ULONG) W32kCheckColorsInGamut},
  {?, (ULONG) W32kChoosePixelFormat},
#endif
  {36, (ULONG) W32kChord},
  {4, (ULONG) W32kCloseEnhMetaFile},
#if 0
  {?, (ULONG) W32kCloseFigure},
#endif
  {4, (ULONG) W32kCloseMetaFile},
#if 0
  {?, (ULONG) W32kColorMatchToTarget},
  {?, (ULONG) W32kCombineRgn},
#endif
  {12, (ULONG) W32kCombineTransform},
  {8, (ULONG) W32kCopyEnhMetaFile},
  {8, (ULONG) W32kCopyMetaFile},
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
#endif
  {16, (ULONG) W32kCreateEnhMetaFile},
  {56, (ULONG) W32kCreateFont},
  {4, (ULONG) W32kCreateFontIndirect},
  {4, (ULONG) W32kCreateHalftonePalette},
  {8, (ULONG) W32kCreateHatchBrush},
#if 0
  {?, (ULONG) W32kCreateIC},
#endif
  {4, (ULONG) W32kCreateMetaFile},
  {4, (ULONG) W32kCreatePalette},
  {4, (ULONG) W32kCreatePatternBrush},
#if 0
  {?, (ULONG) W32kCreatePen},
  {?, (ULONG) W32kCreatePenIndirect},
  {?, (ULONG) W32kCreatePolyPolygonRgn},
  {?, (ULONG) W32kCreatePolygonRgn},
  {?, (ULONG) W32kCreateRectRgn},
  {?, (ULONG) W32kCreateRectRgnIndirect},
  {?, (ULONG) W32kCreateRoundRectRgn},
#endif
  {16, (ULONG) W32kCreateScalableFontResource},
  {4, (ULONG) W32kCreateSolidBrush},
  {12, (ULONG) W32kDPtoLP},
#if 0
  {?, (ULONG) W32kDeleteColorSpace},
#endif
  {4, (ULONG) W32kDeleteDC},
  {4, (ULONG) W32kDeleteEnhMetaFile},
  {4, (ULONG) W32kDeleteMetaFile},
#if 0
  {?, (ULONG) W32kDeleteObject},
  {?, (ULONG) W32kDescribePixelFormat},
  {?, (ULONG) W32kDeviceCapabilitiesEx},
#endif
  {16, (ULONG) W32kDrawEscape},
  {20, (ULONG) W32kEllipse},
#if 0
  {?, (ULONG) W32kEndDoc},
  {?, (ULONG) W32kEndPage},
  {?, (ULONG) W32kEndPath},
#endif
  {20, (ULONG) W32kEnumEnhMetaFile},
  {16, (ULONG) W32kEnumFontFamilies},
  {20, (ULONG) W32kEnumFontFamiliesEx},
  {16, (ULONG) W32kEnumFonts},
#if 0
  {?, (ULONG) W32kEnumICMProfiles},
#endif
  {16, (ULONG) W32kEnumMetaFile},
  {16, (ULONG) W32kEnumObjects},
#if 0
  {?, (ULONG) W32kEqualRgn},
  {?, (ULONG) W32kEscape},
#endif
  {20, (ULONG) W32kExcludeClipRect},
#if 0
  {?, (ULONG) W32kExtCreatePen},
  {?, (ULONG) W32kExtCreateRegion},
  {?, (ULONG) W32kExtEscape},
#endif
  {20, (ULONG) W32kExtFloodFill},
  {12, (ULONG) W32kExtSelectClipRgn},
  {32, (ULONG) W32kExtTextOut},
#if 0
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
#endif
  {12, (ULONG) W32kGdiComment},
#if 0
  {?, (ULONG) W32kGdiFlush},
  {?, (ULONG) W32kGdiGetBatchLimit},
  {?, (ULONG) W32kGdiPlayDCScript},
  {?, (ULONG) W32kGdiPlayJournal},
  {?, (ULONG) W32kGdiPlayScript},
  {?, (ULONG) W32kGdiSetBatchLimit},
#endif
  {4, (ULONG) W32kGetArcDirection},
  {8, (ULONG) W32kGetAspectRatioFilterEx},
  {12, (ULONG) W32kGetBitmapBits},
  {8, (ULONG) W32kGetBitmapDimensionEx},
  {4, (ULONG) W32kGetBkColor},
  {4, (ULONG) W32kGetBkMode},
#if 0
  {?, (ULONG) W32kGetBoundsRect},
#endif
  {8, (ULONG) W32kGetBrushOrgEx},
  {16, (ULONG) W32kGetCharABCWidths},
  {16, (ULONG) W32kGetCharABCWidthsFloat},
  {24, (ULONG) W32kGetCharacterPlacement},
  {16, (ULONG) W32kGetCharWidth},
  {16, (ULONG) W32kGetCharWidth32},
  {16, (ULONG) W32kGetCharWidthFloat},
  {8, (ULONG) W32kGetClipBox},
  {4, (ULONG) W32kGetClipRgn},
  {8, (ULONG) W32kGetColorAdjustment},
#if 0
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
#endif
  {4, (ULONG) W32kGetEnhMetaFile},
#if 0
  {?, (ULONG) W32kGetEnhMetaFileBits},
  {?, (ULONG) W32kGetEnhMetaFileDescription},
  {?, (ULONG) W32kGetEnhMetaFileHeader},
  {?, (ULONG) W32kGetEnhMetaFilePaletteEntries},
  {?, (ULONG) W32kGetEnhMetaFilePixelFormat},
#endif
  {4, (ULONG) W32kGetFontLanguageInfo},
#if 0
  {?, (ULONG) W32kGetFontResourceInfo},
#endif
  {28, (ULONG) W32kGetGlyphOutline},
#if 0
  {?, (ULONG) W32kGetGlyphOutlineWow},
#endif
  {4, (ULONG) W32kGetGraphicsMode},
#if 0
  {?, (ULONG) W32kGetICMProfile},
#endif
  {12, (ULONG) W32kGetKerningPairs},
#if 0
  {?, (ULONG) W32kGetLogColorSpace},
#endif
  {4, (ULONG) W32kGetMapMode},
#if 0
  {?, (ULONG) W32kGetMetaFileBitsEx},
  {?, (ULONG) W32kGetMetaFile},
#endif
  {8, (ULONG) W32kGetMetaRgn},
#if 0
  {?, (ULONG) W32kGetMiterLimit},
#endif
  {8, (ULONG) W32kGetNearestColor},
  {8, (ULONG) W32kGetNearestPaletteIndex},
#if 0
  {?, (ULONG) W32kGetObject},
  {?, (ULONG) W32kGetObjectType},
#endif
  {12, (ULONG) W32kGetOutlineTextMetrics},
  {16, (ULONG) W32kGetPaletteEntries},
#if 0
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
#endif
  {8, (ULONG) W32kGetRasterizerCaps},
  {4, (ULONG) W32kGetRelAbs},
#if 0
  {?, (ULONG) W32kGetRgnBox},
  {?, (ULONG) W32kGetStockObject},
#endif
  {4, (ULONG) W32kGetStretchBltMode},
  {16, (ULONG) W32kGetSystemPaletteEntries},
  {4, (ULONG) W32kGetSystemPaletteUse},
  {4, (ULONG) W32kGetTextAlign},
#if 0
  {?, (ULONG) W32kGetTextCharset},
  {?, (ULONG) W32kGetTextCharsetInfo},
#endif
  {4, (ULONG) W32kGetTextColor},
  {28, (ULONG) W32kGetTextExtentExPoint},
  {16, (ULONG) W32kGetTextExtentPoint},
  {16, (ULONG) W32kGetTextExtentPoint32},
  {12, (ULONG) W32kGetTextFace},
  {8, (ULONG) W32kGetTextMetrics},
  {8, (ULONG) W32kGetViewportExtEx},
  {8, (ULONG) W32kGetViewportOrgEx},
#if 0
  {?, (ULONG) W32kGetWinMetaFileBits},
#endif
  {8, (ULONG) W32kGetWindowExtEx},
  {8, (ULONG) W32kGetWindowOrgEx},
  {8, (ULONG) W32kGetWorldTransform},
  {20, (ULONG) W32kIntersectClipRect},
#if 0
  {?, (ULONG) W32kInvertRgn},
#endif
  {12, (ULONG) W32kLPtoDP},
  {12, (ULONG) W32kLineTo},
  {48, (ULONG) W32kMaskBlt},
  {12, (ULONG) W32kModifyWorldTransform},
  {16, (ULONG) W32kMoveToEx},
  {12, (ULONG) W32kOffsetClipRgn},
#if 0
  {?, (ULONG) W32kOffsetRgn},
#endif
  {16, (ULONG) W32kOffsetViewportOrgEx},
  {16, (ULONG) W32kOffsetWindowOrgEx},
#if 0
  {?, (ULONG) W32kPaintRgn},
#endif
  {24, (ULONG) W32kPatBlt},
#if 0
  {?, (ULONG) W32kPathToRegion},
#endif
  {36, (ULONG) W32kPie},
#if 0
  {?, (ULONG) W32kPlayEnhMetaFile},
  {?, (ULONG) W32kPlayEnhMetaFileRecord},
  {?, (ULONG) W32kPlayMetaFile},
  {?, (ULONG) W32kPlayMetaFileRecord},
#endif
  {40, (ULONG) W32kPlgBlt},
  {12, (ULONG) W32kPolyBezier},
  {12, (ULONG) W32kPolyBezierTo},
  {16, (ULONG) W32kPolyDraw},
  {12, (ULONG) W32kPolyline},
  {12, (ULONG) W32kPolylineTo},
  {16, (ULONG) W32kPolyPolyline},
  {12, (ULONG) W32kPolyTextOut},
  {12, (ULONG) W32kPolygon},
  {16, (ULONG) W32kPolyPolygon},
#if 0
  {?, (ULONG) W32kPtInRegion},
#endif 
  {12, (ULONG) W32kPtVisible},
  {4, (ULONG) W32kRealizePalette},
#if 0
  {?, (ULONG) W32kRectInRegion},
#endif
  {8, (ULONG) W32kRectVisible},
  {20, (ULONG) W32kRectangle},
  {4, (ULONG) W32kRemoveFontResource},
  {8, (ULONG) W32kResetDC},
  {8, (ULONG) W32kResizePalette},
  {8, (ULONG) W32kRestoreDC},
  {28, (ULONG) W32kRoundRect},
  {4, (ULONG) W32kSaveDC},
  {24, (ULONG) W32kScaleViewportExtEx},
  {24, (ULONG) W32kScaleWindowExtEx},
#if 0
  {?, (ULONG) W32kSelectBrushLocal},
#endif
  {8, (ULONG) W32kSelectClipPath},
  {8, (ULONG) W32kSelectClipRgn},
#if 0
  {?, (ULONG) W32kSelectFontLocal},
#endif
  {8, (ULONG) W32kSelectObject},
  {12, (ULONG) W32kSelectPalette},
#if 0
  {?, (ULONG) W32kSetAbortProc},
#endif
  {8, (ULONG) W32kSetArcDirection},
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
  {8, (ULONG) W32kSetColorAdjustment},
#if 0
  {?, (ULONG) W32kSetColorSpace},
#endif
  {16, (ULONG) W32kSetDIBColorTable},
  {28, (ULONG) W32kSetDIBits},
  {48, (ULONG) W32kSetDIBitsToDevice},
#if 0
  {?, (ULONG) W32kSetDeviceGammaRamp},
  {?, (ULONG) W32kSetEnhMetaFileBits},
  {?, (ULONG) W32kSetFontEnumeration},
#endif
  {8, (ULONG) W32kSetGraphicsMode},
#if 0
  {?, (ULONG) W32kSetICMMode},
  {?, (ULONG) W32kSetICMProfile},
#endif
  {8, (ULONG) W32kSetMapMode},
  {8, (ULONG) W32kSetMapperFlags},
#if 0
  {?, (ULONG) W32kSetMetaFileBitsEx},
#if 0
  {4, (ULONG) W32kSetMetaRgn},
#endif
  {?, (ULONG) W32kSetMiterLimit},
#endif
  {16, (ULONG) W32kSetPaletteEntries},
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
  {8, (ULONG) W32kSetSystemPaletteUse},
  {8, (ULONG) W32kSetTextAlign},
  {8, (ULONG) W32kSetTextColor},
  {12, (ULONG) W32kSetTextJustification},
  {16, (ULONG) W32kSetViewportExtEx},
  {16, (ULONG) W32kSetViewportOrgEx},
#if 0
  {?, (ULONG) W32kSetWinMetaFileBits},
#endif
  {16, (ULONG) W32kSetWindowExtEx},
  {16, (ULONG) W32kSetWindowOrgEx},
  {8, (ULONG) W32kSetWorldTransform},
#if 0
  {?, (ULONG) W32kStartDoc},
  {?, (ULONG) W32kStartPage},
#endif
  {44, (ULONG) W32kStretchBlt},
  {52, (ULONG) W32kStretchDIBits},
#if 0
  {?, (ULONG) W32kStrokeAndFillPath},
  {?, (ULONG) W32kStrokePath},
  {?, (ULONG) W32kSwapBuffers},
#endif
  {20, (ULONG) W32kTextOut},
#if 0
  {?, (ULONG) W32kTranslateCharsetInfo},
#endif
  {8, (ULONG) W32kUnrealizeObject},
  {4, (ULONG) W32kUpdateColors},
#if 0
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

