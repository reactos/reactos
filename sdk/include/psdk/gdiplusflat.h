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

#ifndef _FLATAPI_H
#define _FLATAPI_H

#define WINGDIPAPI __stdcall

#define GDIPCONST const

#ifdef __cplusplus
extern "C" {
#endif

/* AdjustableArrowCap */
GpStatus WINGDIPAPI GdipCreateAdjustableArrowCap(REAL,REAL,BOOL,GpAdjustableArrowCap**);
GpStatus WINGDIPAPI GdipGetAdjustableArrowCapFillState(GpAdjustableArrowCap*,BOOL*);
GpStatus WINGDIPAPI GdipGetAdjustableArrowCapHeight(GpAdjustableArrowCap*,REAL*);
GpStatus WINGDIPAPI GdipGetAdjustableArrowCapMiddleInset(GpAdjustableArrowCap*,REAL*);
GpStatus WINGDIPAPI GdipGetAdjustableArrowCapWidth(GpAdjustableArrowCap*,REAL*);
GpStatus WINGDIPAPI GdipSetAdjustableArrowCapFillState(GpAdjustableArrowCap*,BOOL);
GpStatus WINGDIPAPI GdipSetAdjustableArrowCapHeight(GpAdjustableArrowCap*,REAL);
GpStatus WINGDIPAPI GdipSetAdjustableArrowCapMiddleInset(GpAdjustableArrowCap*,REAL);
GpStatus WINGDIPAPI GdipSetAdjustableArrowCapWidth(GpAdjustableArrowCap*,REAL);

/* Bitmap */
GpStatus WINGDIPAPI GdipBitmapApplyEffect(GpBitmap*,CGpEffect*,RECT*,BOOL,VOID**,INT*);
GpStatus WINGDIPAPI GdipBitmapCreateApplyEffect(GpBitmap**,INT,CGpEffect*,RECT*,RECT*,GpBitmap**,BOOL,VOID**,INT*);
GpStatus WINGDIPAPI GdipBitmapGetHistogram(GpBitmap*,HistogramFormat,UINT,UINT*,UINT*,UINT*,UINT*);
GpStatus WINGDIPAPI GdipBitmapGetHistogramSize(HistogramFormat,UINT*);
GpStatus WINGDIPAPI GdipBitmapGetPixel(GpBitmap*,INT,INT,ARGB*);
GpStatus WINGDIPAPI GdipBitmapLockBits(GpBitmap*,GDIPCONST GpRect*,UINT,
    PixelFormat,BitmapData*);
GpStatus WINGDIPAPI GdipBitmapSetPixel(GpBitmap*,INT,INT,ARGB);
GpStatus WINGDIPAPI GdipBitmapSetResolution(GpBitmap*,REAL,REAL);
GpStatus WINGDIPAPI GdipBitmapUnlockBits(GpBitmap*,BitmapData*);
GpStatus WINGDIPAPI GdipCloneBitmapArea(REAL,REAL,REAL,REAL,PixelFormat,GpBitmap*,GpBitmap**);
GpStatus WINGDIPAPI GdipCloneBitmapAreaI(INT,INT,INT,INT,PixelFormat,GpBitmap*,GpBitmap**);
GpStatus WINGDIPAPI GdipCreateBitmapFromFile(GDIPCONST WCHAR*,GpBitmap**);
GpStatus WINGDIPAPI GdipCreateBitmapFromFileICM(GDIPCONST WCHAR*,GpBitmap**);
GpStatus WINGDIPAPI GdipCreateBitmapFromGdiDib(GDIPCONST BITMAPINFO*,VOID*,GpBitmap**);
GpStatus WINGDIPAPI GdipCreateBitmapFromGraphics(INT,INT,GpGraphics*,GpBitmap**);
GpStatus WINGDIPAPI GdipCreateBitmapFromHBITMAP(HBITMAP, HPALETTE, GpBitmap**);
GpStatus WINGDIPAPI GdipCreateBitmapFromHICON(HICON, GpBitmap**);
GpStatus WINGDIPAPI GdipCreateBitmapFromResource(HINSTANCE,GDIPCONST WCHAR*,GpBitmap**);
GpStatus WINGDIPAPI GdipCreateBitmapFromScan0(INT,INT,INT,PixelFormat,BYTE*,
    GpBitmap**);
GpStatus WINGDIPAPI GdipCreateBitmapFromStream(IStream*,GpBitmap**);
GpStatus WINGDIPAPI GdipCreateBitmapFromStreamICM(IStream*,GpBitmap**);
GpStatus WINGDIPAPI GdipCreateHBITMAPFromBitmap(GpBitmap*,HBITMAP*,ARGB);
GpStatus WINGDIPAPI GdipCreateHICONFromBitmap(GpBitmap*,HICON*);
GpStatus WINGDIPAPI GdipDeleteEffect(CGpEffect*);
GpStatus WINGDIPAPI GdipSetEffectParameters(CGpEffect*,const VOID*,const UINT);

/* Brush */
GpStatus WINGDIPAPI GdipCloneBrush(GpBrush*,GpBrush**);
GpStatus WINGDIPAPI GdipDeleteBrush(GpBrush*);
GpStatus WINGDIPAPI GdipGetBrushType(GpBrush*,GpBrushType*);

/* CachedBitmap */
GpStatus WINGDIPAPI GdipCreateCachedBitmap(GpBitmap*,GpGraphics*,
    GpCachedBitmap**);
GpStatus WINGDIPAPI GdipDeleteCachedBitmap(GpCachedBitmap*);
GpStatus WINGDIPAPI GdipDrawCachedBitmap(GpGraphics*,GpCachedBitmap*,INT,INT);

/* CustomLineCap */
GpStatus WINGDIPAPI GdipCloneCustomLineCap(GpCustomLineCap*,GpCustomLineCap**);
GpStatus WINGDIPAPI GdipCreateCustomLineCap(GpPath*,GpPath*,GpLineCap,REAL,
    GpCustomLineCap**);
GpStatus WINGDIPAPI GdipDeleteCustomLineCap(GpCustomLineCap*);
GpStatus WINGDIPAPI GdipGetCustomLineCapBaseCap(GpCustomLineCap*,GpLineCap*);
GpStatus WINGDIPAPI GdipSetCustomLineCapBaseCap(GpCustomLineCap*,GpLineCap);
GpStatus WINGDIPAPI GdipGetCustomLineCapBaseInset(GpCustomLineCap*,REAL*);
GpStatus WINGDIPAPI GdipSetCustomLineCapBaseInset(GpCustomLineCap*,REAL);
GpStatus WINGDIPAPI GdipSetCustomLineCapStrokeCaps(GpCustomLineCap*,GpLineCap,
    GpLineCap);
GpStatus WINGDIPAPI GdipGetCustomLineCapStrokeJoin(GpCustomLineCap*,GpLineJoin*);
GpStatus WINGDIPAPI GdipSetCustomLineCapStrokeJoin(GpCustomLineCap*,GpLineJoin);
GpStatus WINGDIPAPI GdipGetCustomLineCapWidthScale(GpCustomLineCap*,REAL*);
GpStatus WINGDIPAPI GdipSetCustomLineCapWidthScale(GpCustomLineCap*,REAL);
GpStatus WINGDIPAPI GdipSetCustomLineCapBaseInset(GpCustomLineCap*,REAL);
GpStatus WINGDIPAPI GdipGetCustomLineCapType(GpCustomLineCap*,CustomLineCapType*);

/* Font */
GpStatus WINGDIPAPI GdipCloneFont(GpFont*,GpFont**);
GpStatus WINGDIPAPI GdipCreateFont(GDIPCONST GpFontFamily*, REAL, INT, Unit,
    GpFont**);
GpStatus WINGDIPAPI GdipCreateFontFromDC(HDC,GpFont**);
GpStatus WINGDIPAPI GdipCreateFontFromLogfontA(HDC,GDIPCONST LOGFONTA*,GpFont**);
GpStatus WINGDIPAPI GdipCreateFontFromLogfontW(HDC,GDIPCONST LOGFONTW*,GpFont**);
GpStatus WINGDIPAPI GdipDeleteFont(GpFont*);
GpStatus WINGDIPAPI GdipGetLogFontA(GpFont*,GpGraphics*,LOGFONTA*);
GpStatus WINGDIPAPI GdipGetLogFontW(GpFont*,GpGraphics*,LOGFONTW*);
GpStatus WINGDIPAPI GdipGetFamily(GpFont*, GpFontFamily**);
GpStatus WINGDIPAPI GdipGetFontUnit(GpFont*, Unit*);
GpStatus WINGDIPAPI GdipGetFontSize(GpFont*, REAL*);
GpStatus WINGDIPAPI GdipGetFontStyle(GpFont*, INT*);
GpStatus WINGDIPAPI GdipGetFontHeight(GDIPCONST GpFont*, GDIPCONST GpGraphics*,
        REAL*);
GpStatus WINGDIPAPI GdipGetFontHeightGivenDPI(GDIPCONST GpFont*, REAL, REAL*);

/* FontCollection */
GpStatus WINGDIPAPI GdipNewInstalledFontCollection(GpFontCollection**);
GpStatus WINGDIPAPI GdipNewPrivateFontCollection(GpFontCollection**);
GpStatus WINGDIPAPI GdipDeletePrivateFontCollection(GpFontCollection**);
GpStatus WINGDIPAPI GdipPrivateAddFontFile(GpFontCollection*, GDIPCONST WCHAR*);
GpStatus WINGDIPAPI GdipPrivateAddMemoryFont(GpFontCollection*,
        GDIPCONST void*,INT);
GpStatus WINGDIPAPI GdipGetFontCollectionFamilyCount(GpFontCollection*, INT*);
GpStatus WINGDIPAPI GdipGetFontCollectionFamilyList(GpFontCollection*, INT,
        GpFontFamily*[], INT*);

/* FontFamily */
GpStatus WINGDIPAPI GdipCloneFontFamily(GpFontFamily*, GpFontFamily**);
GpStatus WINGDIPAPI GdipCreateFontFamilyFromName(GDIPCONST WCHAR*,
    GpFontCollection*, GpFontFamily**);
GpStatus WINGDIPAPI GdipDeleteFontFamily(GpFontFamily*);
GpStatus WINGDIPAPI GdipGetFamilyName(GDIPCONST GpFontFamily*, WCHAR*, LANGID);
GpStatus WINGDIPAPI GdipGetCellAscent(GDIPCONST GpFontFamily*, INT, UINT16*);
GpStatus WINGDIPAPI GdipGetCellDescent(GDIPCONST GpFontFamily*, INT, UINT16*);
GpStatus WINGDIPAPI GdipGetEmHeight(GDIPCONST GpFontFamily*, INT, UINT16*);
GpStatus WINGDIPAPI GdipGetGenericFontFamilySansSerif(GpFontFamily**);
GpStatus WINGDIPAPI GdipGetGenericFontFamilySerif(GpFontFamily**);
GpStatus WINGDIPAPI GdipGetGenericFontFamilyMonospace(GpFontFamily**);
GpStatus WINGDIPAPI GdipGetLineSpacing(GDIPCONST GpFontFamily*, INT, UINT16*);
GpStatus WINGDIPAPI GdipIsStyleAvailable(GDIPCONST GpFontFamily *, INT, BOOL*);

/* Graphics */
GpStatus WINGDIPAPI GdipFlush(GpGraphics*, GpFlushIntention);
GpStatus WINGDIPAPI GdipBeginContainer(GpGraphics*,GDIPCONST GpRectF*,GDIPCONST GpRectF*,GpUnit,GraphicsContainer*);
GpStatus WINGDIPAPI GdipBeginContainer2(GpGraphics*,GraphicsContainer*);
GpStatus WINGDIPAPI GdipBeginContainerI(GpGraphics*,GDIPCONST GpRect*,GDIPCONST GpRect*,GpUnit,GraphicsContainer*);
GpStatus WINGDIPAPI GdipEndContainer(GpGraphics*,GraphicsContainer);
GpStatus WINGDIPAPI GdipComment(GpGraphics*,UINT,GDIPCONST BYTE*);
GpStatus WINGDIPAPI GdipCreateFromHDC(HDC,GpGraphics**);
GpStatus WINGDIPAPI GdipCreateFromHDC2(HDC,HANDLE,GpGraphics**);
GpStatus WINGDIPAPI GdipCreateFromHWND(HWND,GpGraphics**);
GpStatus WINGDIPAPI GdipCreateFromHWNDICM(HWND,GpGraphics**);
HPALETTE WINGDIPAPI GdipCreateHalftonePalette(void);
GpStatus WINGDIPAPI GdipDeleteGraphics(GpGraphics *);
GpStatus WINGDIPAPI GdipDrawArc(GpGraphics*,GpPen*,REAL,REAL,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipDrawArcI(GpGraphics*,GpPen*,INT,INT,INT,INT,REAL,REAL);
GpStatus WINGDIPAPI GdipDrawBezier(GpGraphics*,GpPen*,REAL,REAL,REAL,REAL,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipDrawBezierI(GpGraphics*,GpPen*,INT,INT,INT,INT,INT,INT,INT,INT);
GpStatus WINGDIPAPI GdipDrawBeziers(GpGraphics*,GpPen*,GDIPCONST GpPointF*,INT);
GpStatus WINGDIPAPI GdipDrawBeziersI(GpGraphics*,GpPen*,GDIPCONST GpPoint*,INT);
GpStatus WINGDIPAPI GdipDrawClosedCurve(GpGraphics*,GpPen*,GDIPCONST GpPointF*,INT);
GpStatus WINGDIPAPI GdipDrawClosedCurveI(GpGraphics*,GpPen*,GDIPCONST GpPoint*,INT);
GpStatus WINGDIPAPI GdipDrawClosedCurve2(GpGraphics*,GpPen*,GDIPCONST GpPointF*,INT,REAL);
GpStatus WINGDIPAPI GdipDrawClosedCurve2I(GpGraphics*,GpPen*,GDIPCONST GpPoint*,INT,REAL);
GpStatus WINGDIPAPI GdipDrawCurve(GpGraphics*,GpPen*,GDIPCONST GpPointF*,INT);
GpStatus WINGDIPAPI GdipDrawCurveI(GpGraphics*,GpPen*,GDIPCONST GpPoint*,INT);
GpStatus WINGDIPAPI GdipDrawCurve2(GpGraphics*,GpPen*,GDIPCONST GpPointF*,INT,REAL);
GpStatus WINGDIPAPI GdipDrawCurve2I(GpGraphics*,GpPen*,GDIPCONST GpPoint*,INT,REAL);
GpStatus WINGDIPAPI GdipDrawCurve3(GpGraphics*,GpPen*,GDIPCONST GpPointF*,INT,INT,INT,REAL);
GpStatus WINGDIPAPI GdipDrawCurve3I(GpGraphics*,GpPen*,GDIPCONST GpPoint*,INT,INT,INT,REAL);
GpStatus WINGDIPAPI GdipDrawDriverString(GpGraphics*,GDIPCONST UINT16*,INT,
    GDIPCONST GpFont*,GDIPCONST GpBrush*,GDIPCONST PointF*,INT,GDIPCONST GpMatrix*);
GpStatus WINGDIPAPI GdipDrawEllipse(GpGraphics*,GpPen*,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipDrawEllipseI(GpGraphics*,GpPen*,INT,INT,INT,INT);
GpStatus WINGDIPAPI GdipDrawImage(GpGraphics*,GpImage*,REAL,REAL);
GpStatus WINGDIPAPI GdipDrawImageI(GpGraphics*,GpImage*,INT,INT);
GpStatus WINGDIPAPI GdipDrawImagePointRect(GpGraphics*,GpImage*,REAL,REAL,REAL,REAL,REAL,REAL,GpUnit);
GpStatus WINGDIPAPI GdipDrawImagePointRectI(GpGraphics*,GpImage*,INT,INT,INT,INT,INT,INT,GpUnit);
GpStatus WINGDIPAPI GdipDrawImagePoints(GpGraphics*,GpImage*,GDIPCONST GpPointF*,INT);
GpStatus WINGDIPAPI GdipDrawImagePointsI(GpGraphics*,GpImage*,GDIPCONST GpPoint*,INT);
GpStatus WINGDIPAPI GdipDrawImagePointsRect(GpGraphics*,GpImage*,
    GDIPCONST GpPointF*,INT,REAL,REAL,REAL,REAL,GpUnit,
    GDIPCONST GpImageAttributes*,DrawImageAbort,VOID*);
GpStatus WINGDIPAPI GdipDrawImagePointsRectI(GpGraphics*,GpImage*,
    GDIPCONST GpPoint*,INT,INT,INT,INT,INT,GpUnit,
    GDIPCONST GpImageAttributes*,DrawImageAbort,VOID*);
GpStatus WINGDIPAPI GdipDrawImageRect(GpGraphics*,GpImage*,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipDrawImageRectI(GpGraphics*,GpImage*,INT,INT,INT,INT);
GpStatus WINGDIPAPI GdipDrawImageRectRect(GpGraphics*,GpImage*,REAL,REAL,REAL,
    REAL,REAL,REAL,REAL,REAL,GpUnit,GDIPCONST GpImageAttributes*,DrawImageAbort,
    VOID*);
GpStatus WINGDIPAPI GdipDrawImageRectRectI(GpGraphics*,GpImage*,INT,INT,INT,
    INT,INT,INT,INT,INT,GpUnit,GDIPCONST GpImageAttributes*,DrawImageAbort,
    VOID*);
GpStatus WINGDIPAPI GdipDrawLine(GpGraphics*,GpPen*,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipDrawLineI(GpGraphics*,GpPen*,INT,INT,INT,INT);
GpStatus WINGDIPAPI GdipDrawLines(GpGraphics*,GpPen*,GDIPCONST GpPointF*,INT);
GpStatus WINGDIPAPI GdipDrawLinesI(GpGraphics*,GpPen*,GDIPCONST GpPoint*,INT);
GpStatus WINGDIPAPI GdipDrawPath(GpGraphics*,GpPen*,GpPath*);
GpStatus WINGDIPAPI GdipDrawPie(GpGraphics*,GpPen*,REAL,REAL,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipDrawPieI(GpGraphics*,GpPen*,INT,INT,INT,INT,REAL,REAL);
GpStatus WINGDIPAPI GdipDrawPolygon(GpGraphics*,GpPen*,GDIPCONST GpPointF*, INT);
GpStatus WINGDIPAPI GdipDrawPolygonI(GpGraphics*,GpPen*,GDIPCONST GpPoint*, INT);
GpStatus WINGDIPAPI GdipDrawRectangle(GpGraphics*,GpPen*,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipDrawRectangleI(GpGraphics*,GpPen*,INT,INT,INT,INT);
GpStatus WINGDIPAPI GdipDrawRectangles(GpGraphics*,GpPen*,GDIPCONST GpRectF*,INT);
GpStatus WINGDIPAPI GdipDrawRectanglesI(GpGraphics*,GpPen*,GDIPCONST GpRect*,INT);
GpStatus WINGDIPAPI GdipDrawString(GpGraphics*,GDIPCONST WCHAR*,INT,
    GDIPCONST GpFont*,GDIPCONST RectF*, GDIPCONST GpStringFormat*,
    GDIPCONST GpBrush*);
GpStatus WINGDIPAPI GdipEnumerateMetafileDestPoint(GpGraphics*,GDIPCONST GpMetafile*,
    GDIPCONST GpPointF*,EnumerateMetafileProc,VOID*,GDIPCONST GpImageAttributes*);
GpStatus WINGDIPAPI GdipEnumerateMetafileDestPointI(GpGraphics*,GDIPCONST GpMetafile*,
    GDIPCONST GpPoint*,EnumerateMetafileProc,VOID*,GDIPCONST GpImageAttributes*);
GpStatus WINGDIPAPI GdipEnumerateMetafileDestRect(GpGraphics*,GDIPCONST GpMetafile*,
    GDIPCONST GpRectF*,EnumerateMetafileProc,VOID*,GDIPCONST GpImageAttributes*);
GpStatus WINGDIPAPI GdipEnumerateMetafileDestRectI(GpGraphics*,GDIPCONST GpMetafile*,
    GDIPCONST GpRect*,EnumerateMetafileProc,VOID*,GDIPCONST GpImageAttributes*);
GpStatus WINGDIPAPI GdipEnumerateMetafileSrcRectDestPoints(GpGraphics*,
    GDIPCONST GpMetafile*,GDIPCONST GpPointF*,INT,GDIPCONST GpRectF*,Unit,
    EnumerateMetafileProc,VOID*,GDIPCONST GpImageAttributes*);
GpStatus WINGDIPAPI GdipFillClosedCurve2(GpGraphics*,GpBrush*,GDIPCONST GpPointF*,INT,
    REAL,GpFillMode);
GpStatus WINGDIPAPI GdipFillClosedCurve2I(GpGraphics*,GpBrush*,GDIPCONST GpPoint*,INT,
    REAL,GpFillMode);
GpStatus WINGDIPAPI GdipFillClosedCurve(GpGraphics*,GpBrush*,GDIPCONST GpPointF*,INT);
GpStatus WINGDIPAPI GdipFillClosedCurveI(GpGraphics*,GpBrush*,GDIPCONST GpPoint*,INT);
GpStatus WINGDIPAPI GdipFillEllipse(GpGraphics*,GpBrush*,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipFillEllipseI(GpGraphics*,GpBrush*,INT,INT,INT,INT);
GpStatus WINGDIPAPI GdipFillPath(GpGraphics*,GpBrush*,GpPath*);
GpStatus WINGDIPAPI GdipFillPie(GpGraphics*,GpBrush*,REAL,REAL,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipFillPieI(GpGraphics*,GpBrush*,INT,INT,INT,INT,REAL,REAL);
GpStatus WINGDIPAPI GdipFillPolygon(GpGraphics*,GpBrush*,GDIPCONST GpPointF*,
    INT,GpFillMode);
GpStatus WINGDIPAPI GdipFillPolygonI(GpGraphics*,GpBrush*,GDIPCONST GpPoint*,
    INT,GpFillMode);
GpStatus WINGDIPAPI GdipFillPolygon2(GpGraphics*,GpBrush*,GDIPCONST GpPointF*,INT);
GpStatus WINGDIPAPI GdipFillPolygon2I(GpGraphics*,GpBrush*,GDIPCONST GpPoint*,INT);
GpStatus WINGDIPAPI GdipFillRectangle(GpGraphics*,GpBrush*,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipFillRectangleI(GpGraphics*,GpBrush*,INT,INT,INT,INT);
GpStatus WINGDIPAPI GdipFillRectangles(GpGraphics*,GpBrush*,GDIPCONST GpRectF*,INT);
GpStatus WINGDIPAPI GdipFillRectanglesI(GpGraphics*,GpBrush*,GDIPCONST GpRect*,INT);
GpStatus WINGDIPAPI GdipFillRegion(GpGraphics*,GpBrush*,GpRegion*);
GpStatus WINGDIPAPI GdipGetClip(GpGraphics*,GpRegion*);
GpStatus WINGDIPAPI GdipGetClipBounds(GpGraphics*,GpRectF*);
GpStatus WINGDIPAPI GdipGetClipBoundsI(GpGraphics*,GpRect*);
GpStatus WINGDIPAPI GdipGetCompositingMode(GpGraphics*,CompositingMode*);
GpStatus WINGDIPAPI GdipGetCompositingQuality(GpGraphics*,CompositingQuality*);
GpStatus WINGDIPAPI GdipGetDC(GpGraphics*,HDC*);
GpStatus WINGDIPAPI GdipGetDpiX(GpGraphics*,REAL*);
GpStatus WINGDIPAPI GdipGetDpiY(GpGraphics*,REAL*);
GpStatus WINGDIPAPI GdipGetImageDecoders(UINT,UINT,ImageCodecInfo*);
GpStatus WINGDIPAPI GdipGetImageDecodersSize(UINT*,UINT*);
GpStatus WINGDIPAPI GdipGetImageGraphicsContext(GpImage*,GpGraphics**);
GpStatus WINGDIPAPI GdipGetInterpolationMode(GpGraphics*,InterpolationMode*);
GpStatus WINGDIPAPI GdipGetNearestColor(GpGraphics*,ARGB*);
GpStatus WINGDIPAPI GdipGetPageScale(GpGraphics*,REAL*);
GpStatus WINGDIPAPI GdipGetPageUnit(GpGraphics*,GpUnit*);
GpStatus WINGDIPAPI GdipGetPixelOffsetMode(GpGraphics*,PixelOffsetMode*);
GpStatus WINGDIPAPI GdipGetSmoothingMode(GpGraphics*,SmoothingMode*);
GpStatus WINGDIPAPI GdipGetTextContrast(GpGraphics*,UINT*);
GpStatus WINGDIPAPI GdipGetTextRenderingHint(GpGraphics*,TextRenderingHint*);
GpStatus WINGDIPAPI GdipGetWorldTransform(GpGraphics*,GpMatrix*);
GpStatus WINGDIPAPI GdipGraphicsClear(GpGraphics*,ARGB);
GpStatus WINGDIPAPI GdipGraphicsSetAbort(GpGraphics*,GdiplusAbort*);
GpStatus WINGDIPAPI GdipGetVisibleClipBounds(GpGraphics*,GpRectF*);
GpStatus WINGDIPAPI GdipGetVisibleClipBoundsI(GpGraphics*,GpRect*);
GpStatus WINGDIPAPI GdipInitializePalette(ColorPalette*,PaletteType,INT,BOOL,GpBitmap*);
GpStatus WINGDIPAPI GdipIsClipEmpty(GpGraphics*, BOOL*);
GpStatus WINGDIPAPI GdipIsVisiblePoint(GpGraphics*,REAL,REAL,BOOL*);
GpStatus WINGDIPAPI GdipIsVisiblePointI(GpGraphics*,INT,INT,BOOL*);
GpStatus WINGDIPAPI GdipIsVisibleRect(GpGraphics*,REAL,REAL,REAL,REAL,BOOL*);
GpStatus WINGDIPAPI GdipIsVisibleRectI(GpGraphics*,INT,INT,INT,INT,BOOL*);
GpStatus WINGDIPAPI GdipMeasureCharacterRanges(GpGraphics*, GDIPCONST WCHAR*,
    INT, GDIPCONST GpFont*, GDIPCONST RectF*, GDIPCONST GpStringFormat*, INT,
    GpRegion**);
GpStatus WINGDIPAPI GdipMeasureDriverString(GpGraphics*,GDIPCONST UINT16*,INT,
    GDIPCONST GpFont*,GDIPCONST PointF*,INT,GDIPCONST GpMatrix*,RectF*);
GpStatus WINGDIPAPI GdipMeasureString(GpGraphics*,GDIPCONST WCHAR*,INT,
    GDIPCONST GpFont*,GDIPCONST RectF*,GDIPCONST GpStringFormat*,RectF*,INT*,INT*);
GpStatus WINGDIPAPI GdipMultiplyWorldTransform(GpGraphics*,GDIPCONST GpMatrix*,GpMatrixOrder);
GpStatus WINGDIPAPI GdipRecordMetafileFileName(GDIPCONST WCHAR*,HDC,EmfType,
    GDIPCONST GpRectF*,MetafileFrameUnit,GDIPCONST WCHAR*,GpMetafile**);
GpStatus WINGDIPAPI GdipRecordMetafileFileNameI(GDIPCONST WCHAR*,HDC,EmfType,
    GDIPCONST GpRect*,MetafileFrameUnit,GDIPCONST WCHAR*,GpMetafile**);
GpStatus WINGDIPAPI GdipRecordMetafileI(HDC,EmfType,GDIPCONST GpRect*,
    MetafileFrameUnit,GDIPCONST WCHAR*,GpMetafile**);
GpStatus WINGDIPAPI GdipReleaseDC(GpGraphics*,HDC);
GpStatus WINGDIPAPI GdipResetClip(GpGraphics*);
GpStatus WINGDIPAPI GdipResetWorldTransform(GpGraphics*);
GpStatus WINGDIPAPI GdipRestoreGraphics(GpGraphics*,GraphicsState);
GpStatus WINGDIPAPI GdipRotateWorldTransform(GpGraphics*,REAL,GpMatrixOrder);
GpStatus WINGDIPAPI GdipSaveGraphics(GpGraphics*,GraphicsState*);
GpStatus WINGDIPAPI GdipScaleWorldTransform(GpGraphics*,REAL,REAL,GpMatrixOrder);
GpStatus WINGDIPAPI GdipSetClipHrgn(GpGraphics*,HRGN,CombineMode);
GpStatus WINGDIPAPI GdipSetClipGraphics(GpGraphics*,GpGraphics*,CombineMode);
GpStatus WINGDIPAPI GdipSetClipPath(GpGraphics*,GpPath*,CombineMode);
GpStatus WINGDIPAPI GdipSetClipRect(GpGraphics*,REAL,REAL,REAL,REAL,CombineMode);
GpStatus WINGDIPAPI GdipSetClipRectI(GpGraphics*,INT,INT,INT,INT,CombineMode);
GpStatus WINGDIPAPI GdipSetClipRegion(GpGraphics*,GpRegion*,CombineMode);
GpStatus WINGDIPAPI GdipSetCompositingMode(GpGraphics*,CompositingMode);
GpStatus WINGDIPAPI GdipSetCompositingQuality(GpGraphics*,CompositingQuality);
GpStatus WINGDIPAPI GdipSetInterpolationMode(GpGraphics*,InterpolationMode);
GpStatus WINGDIPAPI GdipSetPageScale(GpGraphics*,REAL);
GpStatus WINGDIPAPI GdipSetPageUnit(GpGraphics*,GpUnit);
GpStatus WINGDIPAPI GdipSetPixelOffsetMode(GpGraphics*,PixelOffsetMode);
GpStatus WINGDIPAPI GdipSetRenderingOrigin(GpGraphics*,INT,INT);
GpStatus WINGDIPAPI GdipSetSmoothingMode(GpGraphics*,SmoothingMode);
GpStatus WINGDIPAPI GdipSetTextContrast(GpGraphics*,UINT);
GpStatus WINGDIPAPI GdipSetTextRenderingHint(GpGraphics*,TextRenderingHint);
GpStatus WINGDIPAPI GdipSetWorldTransform(GpGraphics*,GpMatrix*);
GpStatus WINGDIPAPI GdipTransformPoints(GpGraphics*, GpCoordinateSpace, GpCoordinateSpace,
                                        GpPointF *, INT);
GpStatus WINGDIPAPI GdipTransformPointsI(GpGraphics*, GpCoordinateSpace, GpCoordinateSpace,
                                         GpPoint *, INT);
GpStatus WINGDIPAPI GdipTranslateClip(GpGraphics*,REAL,REAL);
GpStatus WINGDIPAPI GdipTranslateClipI(GpGraphics*,INT,INT);
GpStatus WINGDIPAPI GdipTranslateWorldTransform(GpGraphics*,REAL,REAL,GpMatrixOrder);

/* GraphicsPath */
GpStatus WINGDIPAPI GdipAddPathArc(GpPath*,REAL,REAL,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipAddPathArcI(GpPath*,INT,INT,INT,INT,REAL,REAL);
GpStatus WINGDIPAPI GdipAddPathBezier(GpPath*,REAL,REAL,REAL,REAL,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipAddPathBezierI(GpPath*,INT,INT,INT,INT,INT,INT,INT,INT);
GpStatus WINGDIPAPI GdipAddPathBeziers(GpPath*,GDIPCONST GpPointF*,INT);
GpStatus WINGDIPAPI GdipAddPathBeziersI(GpPath*,GDIPCONST GpPoint*,INT);
GpStatus WINGDIPAPI GdipAddPathClosedCurve(GpPath*,GDIPCONST GpPointF*,INT);
GpStatus WINGDIPAPI GdipAddPathClosedCurveI(GpPath*,GDIPCONST GpPoint*,INT);
GpStatus WINGDIPAPI GdipAddPathClosedCurve2(GpPath*,GDIPCONST GpPointF*,INT,REAL);
GpStatus WINGDIPAPI GdipAddPathClosedCurve2I(GpPath*,GDIPCONST GpPoint*,INT,REAL);
GpStatus WINGDIPAPI GdipAddPathCurve(GpPath*,GDIPCONST GpPointF*,INT);
GpStatus WINGDIPAPI GdipAddPathCurveI(GpPath*,GDIPCONST GpPoint*,INT);
GpStatus WINGDIPAPI GdipAddPathCurve2(GpPath*,GDIPCONST GpPointF*,INT,REAL);
GpStatus WINGDIPAPI GdipAddPathCurve2I(GpPath*,GDIPCONST GpPoint*,INT,REAL);
GpStatus WINGDIPAPI GdipAddPathCurve3(GpPath*,GDIPCONST GpPointF*,INT,INT,INT,REAL);
GpStatus WINGDIPAPI GdipAddPathCurve3I(GpPath*,GDIPCONST GpPoint*,INT,INT,INT,REAL);
GpStatus WINGDIPAPI GdipAddPathEllipse(GpPath*,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipAddPathEllipseI(GpPath*,INT,INT,INT,INT);
GpStatus WINGDIPAPI GdipAddPathLine(GpPath*,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipAddPathLineI(GpPath*,INT,INT,INT,INT);
GpStatus WINGDIPAPI GdipAddPathLine2(GpPath*,GDIPCONST GpPointF*,INT);
GpStatus WINGDIPAPI GdipAddPathLine2I(GpPath*,GDIPCONST GpPoint*,INT);
GpStatus WINGDIPAPI GdipAddPathPath(GpPath*,GDIPCONST GpPath*,BOOL);
GpStatus WINGDIPAPI GdipAddPathPie(GpPath*,REAL,REAL,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipAddPathPieI(GpPath*,INT,INT,INT,INT,REAL,REAL);
GpStatus WINGDIPAPI GdipAddPathPolygon(GpPath*,GDIPCONST GpPointF*,INT);
GpStatus WINGDIPAPI GdipAddPathPolygonI(GpPath*,GDIPCONST GpPoint*,INT);
GpStatus WINGDIPAPI GdipAddPathRectangle(GpPath*,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipAddPathRectangleI(GpPath*,INT,INT,INT,INT);
GpStatus WINGDIPAPI GdipAddPathRectangles(GpPath*,GDIPCONST GpRectF*,INT);
GpStatus WINGDIPAPI GdipAddPathRectanglesI(GpPath*,GDIPCONST GpRect*,INT);
GpStatus WINGDIPAPI GdipAddPathString(GpPath*,GDIPCONST WCHAR*,INT,GDIPCONST GpFontFamily*,INT,REAL,GDIPCONST RectF*,GDIPCONST GpStringFormat*);
GpStatus WINGDIPAPI GdipAddPathStringI(GpPath*,GDIPCONST WCHAR*,INT,GDIPCONST GpFontFamily*,INT,REAL,GDIPCONST Rect*,GDIPCONST GpStringFormat*);
GpStatus WINGDIPAPI GdipClearPathMarkers(GpPath*);
GpStatus WINGDIPAPI GdipClonePath(GpPath*,GpPath**);
GpStatus WINGDIPAPI GdipClosePathFigure(GpPath*);
GpStatus WINGDIPAPI GdipClosePathFigures(GpPath*);
GpStatus WINGDIPAPI GdipCreatePath(GpFillMode,GpPath**);
GpStatus WINGDIPAPI GdipCreatePath2(GDIPCONST GpPointF*,GDIPCONST BYTE*,INT,
    GpFillMode,GpPath**);
GpStatus WINGDIPAPI GdipCreatePath2I(GDIPCONST GpPoint*,GDIPCONST BYTE*,INT,GpFillMode,GpPath**);
GpStatus WINGDIPAPI GdipDeletePath(GpPath*);
GpStatus WINGDIPAPI GdipFlattenPath(GpPath*,GpMatrix*,REAL);
GpStatus WINGDIPAPI GdipIsOutlineVisiblePathPoint(GpPath*,REAL,REAL,GpPen*,
    GpGraphics*,BOOL*);
GpStatus WINGDIPAPI GdipIsOutlineVisiblePathPointI(GpPath*,INT,INT,GpPen*,
    GpGraphics*,BOOL*);
GpStatus WINGDIPAPI GdipIsVisiblePathPoint(GpPath*,REAL,REAL,GpGraphics*,BOOL*);
GpStatus WINGDIPAPI GdipIsVisiblePathPointI(GpPath*,INT,INT,GpGraphics*,BOOL*);
GpStatus WINGDIPAPI GdipGetPathData(GpPath*,GpPathData*);
GpStatus WINGDIPAPI GdipGetPathFillMode(GpPath*,GpFillMode*);
GpStatus WINGDIPAPI GdipGetPathLastPoint(GpPath*,GpPointF*);
GpStatus WINGDIPAPI GdipGetPathPoints(GpPath*,GpPointF*,INT);
GpStatus WINGDIPAPI GdipGetPathPointsI(GpPath*,GpPoint*,INT);
GpStatus WINGDIPAPI GdipGetPathTypes(GpPath*,BYTE*,INT);
GpStatus WINGDIPAPI GdipGetPathWorldBounds(GpPath*,GpRectF*,GDIPCONST GpMatrix*,GDIPCONST GpPen*);
GpStatus WINGDIPAPI GdipGetPathWorldBoundsI(GpPath*,GpRect*,GDIPCONST GpMatrix*,GDIPCONST GpPen*);
GpStatus WINGDIPAPI GdipGetPointCount(GpPath*,INT*);
GpStatus WINGDIPAPI GdipResetPath(GpPath*);
GpStatus WINGDIPAPI GdipReversePath(GpPath*);
GpStatus WINGDIPAPI GdipSetPathFillMode(GpPath*,GpFillMode);
GpStatus WINGDIPAPI GdipSetPathMarker(GpPath*);
GpStatus WINGDIPAPI GdipStartPathFigure(GpPath*);
GpStatus WINGDIPAPI GdipTransformPath(GpPath*,GpMatrix*);
GpStatus WINGDIPAPI GdipWarpPath(GpPath*,GpMatrix*,GDIPCONST GpPointF*,INT,REAL,
    REAL,REAL,REAL,WarpMode,REAL);
GpStatus WINGDIPAPI GdipWidenPath(GpPath*,GpPen*,GpMatrix*,REAL);

/* HatchBrush */
GpStatus WINGDIPAPI GdipCreateHatchBrush(GpHatchStyle,ARGB,ARGB,GpHatch**);
GpStatus WINGDIPAPI GdipGetHatchBackgroundColor(GpHatch*,ARGB*);
GpStatus WINGDIPAPI GdipGetHatchForegroundColor(GpHatch*,ARGB*);
GpStatus WINGDIPAPI GdipGetHatchStyle(GpHatch*,GpHatchStyle*);

/* Image */
GpStatus WINGDIPAPI GdipCloneImage(GpImage*, GpImage**);
GpStatus WINGDIPAPI GdipCloneImageAttributes(GDIPCONST GpImageAttributes*,GpImageAttributes**);
GpStatus WINGDIPAPI GdipDisposeImage(GpImage*);
GpStatus WINGDIPAPI GdipEmfToWmfBits(HENHMETAFILE,UINT,LPBYTE,INT,INT);
GpStatus WINGDIPAPI GdipFindFirstImageItem(GpImage*,ImageItemData*);
GpStatus WINGDIPAPI GdipFindNextImageItem(GpImage*,ImageItemData*);
GpStatus WINGDIPAPI GdipGetAllPropertyItems(GpImage*,UINT,UINT,PropertyItem*);
GpStatus WINGDIPAPI GdipGetImageBounds(GpImage*,GpRectF*,GpUnit*);
GpStatus WINGDIPAPI GdipGetImageDimension(GpImage*,REAL*,REAL*);
GpStatus WINGDIPAPI GdipGetImageFlags(GpImage*,UINT*);
GpStatus WINGDIPAPI GdipGetImageHeight(GpImage*,UINT*);
GpStatus WINGDIPAPI GdipGetImageHorizontalResolution(GpImage*,REAL*);
GpStatus WINGDIPAPI GdipGetImageItemData(GpImage*,ImageItemData*);
GpStatus WINGDIPAPI GdipGetImagePalette(GpImage*,ColorPalette*,INT);
GpStatus WINGDIPAPI GdipGetImagePaletteSize(GpImage*,INT*);
GpStatus WINGDIPAPI GdipGetImagePixelFormat(GpImage*,PixelFormat*);
GpStatus WINGDIPAPI GdipGetImageRawFormat(GpImage*,GUID*);
GpStatus WINGDIPAPI GdipGetImageThumbnail(GpImage*,UINT,UINT,GpImage**,GetThumbnailImageAbort,VOID*);
GpStatus WINGDIPAPI GdipGetImageType(GpImage*,ImageType*);
GpStatus WINGDIPAPI GdipGetImageVerticalResolution(GpImage*,REAL*);
GpStatus WINGDIPAPI GdipGetImageWidth(GpImage*,UINT*);
GpStatus WINGDIPAPI GdipGetPropertyCount(GpImage*,UINT*);
GpStatus WINGDIPAPI GdipGetPropertyIdList(GpImage*,UINT,PROPID*);
GpStatus WINGDIPAPI GdipGetPropertyItem(GpImage*,PROPID,UINT,PropertyItem*);
GpStatus WINGDIPAPI GdipGetPropertyItemSize(GpImage*,PROPID,UINT*);
GpStatus WINGDIPAPI GdipGetPropertySize(GpImage*,UINT*,UINT*);
GpStatus WINGDIPAPI GdipImageForceValidation(GpImage*);
GpStatus WINGDIPAPI GdipImageGetFrameCount(GpImage*,GDIPCONST GUID*,UINT*);
GpStatus WINGDIPAPI GdipImageGetFrameDimensionsCount(GpImage*,UINT*);
GpStatus WINGDIPAPI GdipImageGetFrameDimensionsList(GpImage*,GUID*,UINT);
GpStatus WINGDIPAPI GdipImageRotateFlip(GpImage*,RotateFlipType);
GpStatus WINGDIPAPI GdipImageSelectActiveFrame(GpImage*,GDIPCONST GUID*,UINT);
GpStatus WINGDIPAPI GdipImageSetAbort(GpImage*,GdiplusAbort*);
GpStatus WINGDIPAPI GdipLoadImageFromFile(GDIPCONST WCHAR*,GpImage**);
GpStatus WINGDIPAPI GdipLoadImageFromFileICM(GDIPCONST WCHAR*,GpImage**);
GpStatus WINGDIPAPI GdipLoadImageFromStream(IStream*,GpImage**);
GpStatus WINGDIPAPI GdipLoadImageFromStreamICM(IStream*,GpImage**);
GpStatus WINGDIPAPI GdipRemovePropertyItem(GpImage*,PROPID);
GpStatus WINGDIPAPI GdipSaveImageToFile(GpImage*,GDIPCONST WCHAR*,GDIPCONST CLSID*,GDIPCONST EncoderParameters*);
GpStatus WINGDIPAPI GdipSaveImageToStream(GpImage*,IStream*,
    GDIPCONST CLSID*,GDIPCONST EncoderParameters*);
GpStatus WINGDIPAPI GdipSetImagePalette(GpImage*,GDIPCONST ColorPalette*);
GpStatus WINGDIPAPI GdipSetPropertyItem(GpImage*,GDIPCONST PropertyItem*);

/* ImageAttributes */
GpStatus WINGDIPAPI GdipCreateImageAttributes(GpImageAttributes**);
GpStatus WINGDIPAPI GdipDisposeImageAttributes(GpImageAttributes*);
GpStatus WINGDIPAPI GdipGetImageAttributesAdjustedPalette(GpImageAttributes*,
    ColorPalette*,ColorAdjustType);
GpStatus WINGDIPAPI GdipSetImageAttributesCachedBackground(GpImageAttributes*,
    BOOL);
GpStatus WINGDIPAPI GdipSetImageAttributesColorKeys(GpImageAttributes*,
    ColorAdjustType,BOOL,ARGB,ARGB);
GpStatus WINGDIPAPI GdipSetImageAttributesColorMatrix(GpImageAttributes*,
    ColorAdjustType,BOOL,GDIPCONST ColorMatrix*,GDIPCONST ColorMatrix*,
    ColorMatrixFlags);
GpStatus WINGDIPAPI GdipSetImageAttributesGamma(GpImageAttributes*,
    ColorAdjustType,BOOL,REAL);
GpStatus WINGDIPAPI GdipSetImageAttributesNoOp(GpImageAttributes*,
    ColorAdjustType,BOOL);
GpStatus WINGDIPAPI GdipSetImageAttributesOutputChannel(GpImageAttributes*,
    ColorAdjustType,BOOL,ColorChannelFlags);
GpStatus WINGDIPAPI GdipSetImageAttributesOutputChannelColorProfile(
    GpImageAttributes*,ColorAdjustType,BOOL,GDIPCONST WCHAR*);
GpStatus WINGDIPAPI GdipSetImageAttributesRemapTable(GpImageAttributes*,
    ColorAdjustType,BOOL,UINT,GDIPCONST ColorMap*);
GpStatus WINGDIPAPI GdipSetImageAttributesThreshold(GpImageAttributes*,
    ColorAdjustType,BOOL,REAL);
GpStatus WINGDIPAPI GdipSetImageAttributesToIdentity(GpImageAttributes*,
    ColorAdjustType);
GpStatus WINGDIPAPI GdipSetImageAttributesWrapMode(GpImageAttributes*,WrapMode,
    ARGB,BOOL);
GpStatus WINGDIPAPI GdipResetImageAttributes(GpImageAttributes*,
    ColorAdjustType);

/* LinearGradientBrush */
GpStatus WINGDIPAPI GdipCreateLineBrush(GDIPCONST GpPointF*,GDIPCONST GpPointF*,
    ARGB,ARGB,GpWrapMode,GpLineGradient**);
GpStatus WINGDIPAPI GdipCreateLineBrushI(GDIPCONST GpPoint*,GDIPCONST GpPoint*,
    ARGB,ARGB,GpWrapMode,GpLineGradient**);
GpStatus WINGDIPAPI GdipCreateLineBrushFromRect(GDIPCONST GpRectF*,ARGB,ARGB,
    LinearGradientMode,GpWrapMode,GpLineGradient**);
GpStatus WINGDIPAPI GdipCreateLineBrushFromRectI(GDIPCONST GpRect*,ARGB,ARGB,
    LinearGradientMode,GpWrapMode,GpLineGradient**);
GpStatus WINGDIPAPI GdipCreateLineBrushFromRectWithAngle(GDIPCONST GpRectF*,
    ARGB,ARGB,REAL,BOOL,GpWrapMode,GpLineGradient**);
GpStatus WINGDIPAPI GdipCreateLineBrushFromRectWithAngleI(GDIPCONST GpRect*,
    ARGB,ARGB,REAL,BOOL,GpWrapMode,GpLineGradient**);
GpStatus WINGDIPAPI GdipGetLineColors(GpLineGradient*,ARGB*);
GpStatus WINGDIPAPI GdipGetLineGammaCorrection(GpLineGradient*,BOOL*);
GpStatus WINGDIPAPI GdipGetLineRect(GpLineGradient*,GpRectF*);
GpStatus WINGDIPAPI GdipGetLineRectI(GpLineGradient*,GpRect*);
GpStatus WINGDIPAPI GdipGetLineWrapMode(GpLineGradient*,GpWrapMode*);
GpStatus WINGDIPAPI GdipSetLineBlend(GpLineGradient*,GDIPCONST REAL*,
    GDIPCONST REAL*,INT);
GpStatus WINGDIPAPI GdipGetLineBlend(GpLineGradient*,REAL*,REAL*,INT);
GpStatus WINGDIPAPI GdipGetLineBlendCount(GpLineGradient*,INT*);
GpStatus WINGDIPAPI GdipSetLinePresetBlend(GpLineGradient*,GDIPCONST ARGB*,
    GDIPCONST REAL*,INT);
GpStatus WINGDIPAPI GdipGetLinePresetBlend(GpLineGradient*,ARGB*,REAL*,INT);
GpStatus WINGDIPAPI GdipGetLinePresetBlendCount(GpLineGradient*,INT*);
GpStatus WINGDIPAPI GdipGetLineTransform(GpLineGradient*,GpMatrix*);
GpStatus WINGDIPAPI GdipMultiplyLineTransform(GpLineGradient*,GDIPCONST GpMatrix*,GpMatrixOrder);
GpStatus WINGDIPAPI GdipResetLineTransform(GpLineGradient*);
GpStatus WINGDIPAPI GdipRotateLineTransform(GpLineGradient*,REAL,GpMatrixOrder);
GpStatus WINGDIPAPI GdipScaleLineTransform(GpLineGradient*,REAL,REAL,
    GpMatrixOrder);
GpStatus WINGDIPAPI GdipSetLineColors(GpLineGradient*,ARGB,ARGB);
GpStatus WINGDIPAPI GdipSetLineGammaCorrection(GpLineGradient*,BOOL);
GpStatus WINGDIPAPI GdipSetLineSigmaBlend(GpLineGradient*,REAL,REAL);
GpStatus WINGDIPAPI GdipSetLineTransform(GpLineGradient*,GDIPCONST GpMatrix*);
GpStatus WINGDIPAPI GdipSetLineLinearBlend(GpLineGradient*,REAL,REAL);
GpStatus WINGDIPAPI GdipSetLineWrapMode(GpLineGradient*,GpWrapMode);
GpStatus WINGDIPAPI GdipTranslateLineTransform(GpLineGradient*,REAL,REAL,
    GpMatrixOrder);

/* Matrix */
GpStatus WINGDIPAPI GdipCloneMatrix(GpMatrix*,GpMatrix**);
GpStatus WINGDIPAPI GdipCreateMatrix(GpMatrix**);
GpStatus WINGDIPAPI GdipCreateMatrix2(REAL,REAL,REAL,REAL,REAL,REAL,GpMatrix**);
GpStatus WINGDIPAPI GdipCreateMatrix3(GDIPCONST GpRectF *,GDIPCONST GpPointF*,GpMatrix**);
GpStatus WINGDIPAPI GdipCreateMatrix3I(GDIPCONST GpRect*,GDIPCONST GpPoint*,GpMatrix**);
GpStatus WINGDIPAPI GdipDeleteMatrix(GpMatrix*);
GpStatus WINGDIPAPI GdipGetMatrixElements(GDIPCONST GpMatrix*,REAL*);
GpStatus WINGDIPAPI GdipInvertMatrix(GpMatrix*);
GpStatus WINGDIPAPI GdipIsMatrixEqual(GDIPCONST GpMatrix*, GDIPCONST GpMatrix*, BOOL*);
GpStatus WINGDIPAPI GdipIsMatrixIdentity(GDIPCONST GpMatrix*, BOOL*);
GpStatus WINGDIPAPI GdipIsMatrixInvertible(GDIPCONST GpMatrix*, BOOL*);
GpStatus WINGDIPAPI GdipMultiplyMatrix(GpMatrix*,GDIPCONST GpMatrix*,GpMatrixOrder);
GpStatus WINGDIPAPI GdipRotateMatrix(GpMatrix*,REAL,GpMatrixOrder);
GpStatus WINGDIPAPI GdipShearMatrix(GpMatrix*,REAL,REAL,GpMatrixOrder);
GpStatus WINGDIPAPI GdipScaleMatrix(GpMatrix*,REAL,REAL,GpMatrixOrder);
GpStatus WINGDIPAPI GdipSetMatrixElements(GpMatrix*,REAL,REAL,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipTransformMatrixPoints(GpMatrix*,GpPointF*,INT);
GpStatus WINGDIPAPI GdipTransformMatrixPointsI(GpMatrix*,GpPoint*,INT);
GpStatus WINGDIPAPI GdipTranslateMatrix(GpMatrix*,REAL,REAL,GpMatrixOrder);
GpStatus WINGDIPAPI GdipVectorTransformMatrixPoints(GpMatrix*,GpPointF*,INT);
GpStatus WINGDIPAPI GdipVectorTransformMatrixPointsI(GpMatrix*,GpPoint*,INT);

/* Metafile */
GpStatus WINGDIPAPI GdipConvertToEmfPlus(const GpGraphics*,GpMetafile*,INT*,
    EmfType,const WCHAR*,GpMetafile**);
GpStatus WINGDIPAPI GdipConvertToEmfPlusToFile(const GpGraphics*,GpMetafile*,INT*,const WCHAR*,EmfType,const WCHAR*,GpMetafile**);
GpStatus WINGDIPAPI GdipConvertToEmfPlusToStream(const GpGraphics*,GpMetafile*,INT*,IStream*,EmfType,const WCHAR*,GpMetafile**);
GpStatus WINGDIPAPI GdipCreateMetafileFromEmf(HENHMETAFILE,BOOL,GpMetafile**);
GpStatus WINGDIPAPI GdipCreateMetafileFromWmf(HMETAFILE,BOOL,
    GDIPCONST WmfPlaceableFileHeader*,GpMetafile**);
GpStatus WINGDIPAPI GdipCreateMetafileFromWmfFile(GDIPCONST WCHAR*, GDIPCONST WmfPlaceableFileHeader*,
    GpMetafile**);
GpStatus WINGDIPAPI GdipCreateMetafileFromFile(GDIPCONST WCHAR*,GpMetafile**);
GpStatus WINGDIPAPI GdipCreateMetafileFromStream(IStream*,GpMetafile**);
GpStatus WINGDIPAPI GdipGetHemfFromMetafile(GpMetafile*,HENHMETAFILE*);
GpStatus WINGDIPAPI GdipPlayMetafileRecord(GDIPCONST GpMetafile*,EmfPlusRecordType,UINT,UINT,GDIPCONST BYTE*);
GpStatus WINGDIPAPI GdipSetMetafileDownLevelRasterizationLimit(GpMetafile*,UINT);
GpStatus WINGDIPAPI GdipRecordMetafile(HDC,EmfType,GDIPCONST GpRectF*,MetafileFrameUnit,GDIPCONST WCHAR*,GpMetafile**);

/* MetafileHeader */
GpStatus WINGDIPAPI GdipGetMetafileHeaderFromEmf(HENHMETAFILE,MetafileHeader*);
GpStatus WINGDIPAPI GdipGetMetafileHeaderFromFile(GDIPCONST WCHAR*,MetafileHeader*);
GpStatus WINGDIPAPI GdipGetMetafileHeaderFromMetafile(GpMetafile*,MetafileHeader*);
GpStatus WINGDIPAPI GdipGetMetafileHeaderFromStream(IStream*,MetafileHeader*);
GpStatus WINGDIPAPI GdipGetMetafileHeaderFromWmf(HMETAFILE,GDIPCONST WmfPlaceableFileHeader*,MetafileHeader*);

/* Notification */
GpStatus WINAPI GdiplusNotificationHook(ULONG_PTR*);
void WINAPI GdiplusNotificationUnhook(ULONG_PTR);

/* PathGradientBrush */
GpStatus WINGDIPAPI GdipCreatePathGradient(GDIPCONST GpPointF*,INT,GpWrapMode,GpPathGradient**);
GpStatus WINGDIPAPI GdipCreatePathGradientI(GDIPCONST GpPoint*,INT,GpWrapMode,GpPathGradient**);
GpStatus WINGDIPAPI GdipCreatePathGradientFromPath(GDIPCONST GpPath*,
    GpPathGradient**);
GpStatus WINGDIPAPI GdipGetPathGradientBlend(GpPathGradient*,REAL*,REAL*,INT);
GpStatus WINGDIPAPI GdipGetPathGradientBlendCount(GpPathGradient*,INT*);
GpStatus WINGDIPAPI GdipGetPathGradientCenterColor(GpPathGradient*,ARGB*);
GpStatus WINGDIPAPI GdipGetPathGradientCenterPoint(GpPathGradient*,GpPointF*);
GpStatus WINGDIPAPI GdipGetPathGradientCenterPointI(GpPathGradient*,GpPoint*);
GpStatus WINGDIPAPI GdipGetPathGradientFocusScales(GpPathGradient*,REAL*,REAL*);
GpStatus WINGDIPAPI GdipGetPathGradientGammaCorrection(GpPathGradient*,BOOL*);
GpStatus WINGDIPAPI GdipGetPathGradientPath(GpPathGradient*,GpPath*);
GpStatus WINGDIPAPI GdipGetPathGradientPresetBlend(GpPathGradient*,ARGB*,REAL*,INT);
GpStatus WINGDIPAPI GdipGetPathGradientPresetBlendCount(GpPathGradient*,INT*);
GpStatus WINGDIPAPI GdipGetPathGradientPointCount(GpPathGradient*,INT*);
GpStatus WINGDIPAPI GdipSetPathGradientPresetBlend(GpPathGradient*,
    GDIPCONST ARGB*,GDIPCONST REAL*,INT);
GpStatus WINGDIPAPI GdipGetPathGradientRect(GpPathGradient*,GpRectF*);
GpStatus WINGDIPAPI GdipGetPathGradientRectI(GpPathGradient*,GpRect*);
GpStatus WINGDIPAPI GdipGetPathGradientSurroundColorsWithCount(GpPathGradient*,
    ARGB*,INT*);
GpStatus WINGDIPAPI GdipGetPathGradientWrapMode(GpPathGradient*,GpWrapMode*);
GpStatus WINGDIPAPI GdipSetPathGradientBlend(GpPathGradient*,GDIPCONST REAL*,GDIPCONST REAL*,INT);
GpStatus WINGDIPAPI GdipSetPathGradientCenterColor(GpPathGradient*,ARGB);
GpStatus WINGDIPAPI GdipSetPathGradientCenterPoint(GpPathGradient*,GpPointF*);
GpStatus WINGDIPAPI GdipSetPathGradientCenterPointI(GpPathGradient*,GpPoint*);
GpStatus WINGDIPAPI GdipSetPathGradientFocusScales(GpPathGradient*,REAL,REAL);
GpStatus WINGDIPAPI GdipSetPathGradientGammaCorrection(GpPathGradient*,BOOL);
GpStatus WINGDIPAPI GdipSetPathGradientPath(GpPathGradient*,GDIPCONST GpPath*);
GpStatus WINGDIPAPI GdipSetPathGradientSigmaBlend(GpPathGradient*,REAL,REAL);
GpStatus WINGDIPAPI GdipSetPathGradientSurroundColorsWithCount(GpPathGradient*,
    GDIPCONST ARGB*,INT*);
GpStatus WINGDIPAPI GdipSetPathGradientWrapMode(GpPathGradient*,GpWrapMode);
GpStatus WINGDIPAPI GdipGetPathGradientSurroundColorCount(GpPathGradient*,INT*);

/* PathIterator */
GpStatus WINGDIPAPI GdipCreatePathIter(GpPathIterator**,GpPath*);
GpStatus WINGDIPAPI GdipDeletePathIter(GpPathIterator*);
GpStatus WINGDIPAPI GdipPathIterCopyData(GpPathIterator*,INT*,GpPointF*,BYTE*,
    INT,INT);
GpStatus WINGDIPAPI GdipPathIterGetCount(GpPathIterator*,INT*);
GpStatus WINGDIPAPI GdipPathIterGetSubpathCount(GpPathIterator*,INT*);
GpStatus WINGDIPAPI GdipPathIterEnumerate(GpPathIterator*,INT*,GpPointF*,BYTE*,INT);
GpStatus WINGDIPAPI GdipPathIterHasCurve(GpPathIterator*,BOOL*);
GpStatus WINGDIPAPI GdipPathIterIsValid(GpPathIterator*,BOOL*);
GpStatus WINGDIPAPI GdipPathIterNextMarker(GpPathIterator*,INT*,INT*,INT*);
GpStatus WINGDIPAPI GdipPathIterNextMarkerPath(GpPathIterator*,INT*,GpPath*);
GpStatus WINGDIPAPI GdipPathIterNextPathType(GpPathIterator*,INT*,BYTE*,INT*,INT*);
GpStatus WINGDIPAPI GdipPathIterNextSubpath(GpPathIterator*,INT*,INT*,INT*,BOOL*);
GpStatus WINGDIPAPI GdipPathIterNextSubpathPath(GpPathIterator*,INT*,GpPath*,BOOL*);
GpStatus WINGDIPAPI GdipPathIterRewind(GpPathIterator*);

/* Pen */
GpStatus WINGDIPAPI GdipClonePen(GpPen*,GpPen**);
GpStatus WINGDIPAPI GdipCreatePen1(ARGB,REAL,GpUnit,GpPen**);
GpStatus WINGDIPAPI GdipCreatePen2(GpBrush*,REAL,GpUnit,GpPen**);
GpStatus WINGDIPAPI GdipDeletePen(GpPen*);
GpStatus WINGDIPAPI GdipGetPenBrushFill(GpPen*,GpBrush**);
GpStatus WINGDIPAPI GdipGetPenColor(GpPen*,ARGB*);
GpStatus WINGDIPAPI GdipGetPenCompoundCount(GpPen*,INT*);
GpStatus WINGDIPAPI GdipGetPenCustomStartCap(GpPen*,GpCustomLineCap**);
GpStatus WINGDIPAPI GdipGetPenCustomEndCap(GpPen*,GpCustomLineCap**);
GpStatus WINGDIPAPI GdipGetPenDashArray(GpPen*,REAL*,INT);
GpStatus WINGDIPAPI GdipGetPenDashCount(GpPen*,INT*);
GpStatus WINGDIPAPI GdipGetPenDashOffset(GpPen*,REAL*);
GpStatus WINGDIPAPI GdipGetPenDashStyle(GpPen*,GpDashStyle*);
GpStatus WINGDIPAPI GdipGetPenMode(GpPen*,GpPenAlignment*);
GpStatus WINGDIPAPI GdipGetPenTransform(GpPen *, GpMatrix *);
GpStatus WINGDIPAPI GdipMultiplyPenTransform(GpPen *,GDIPCONST GpMatrix *,GpMatrixOrder);
GpStatus WINGDIPAPI GdipResetPenTransform(GpPen*);
GpStatus WINGDIPAPI GdipRotatePenTransform(GpPen*,REAL,GpMatrixOrder);
GpStatus WINGDIPAPI GdipScalePenTransform(GpPen*,REAL,REAL,GpMatrixOrder);
GpStatus WINGDIPAPI GdipSetPenBrushFill(GpPen*,GpBrush*);
GpStatus WINGDIPAPI GdipSetPenColor(GpPen*,ARGB);
GpStatus WINGDIPAPI GdipSetPenCompoundArray(GpPen*,GDIPCONST REAL*,INT);
GpStatus WINGDIPAPI GdipSetPenCustomEndCap(GpPen*,GpCustomLineCap*);
GpStatus WINGDIPAPI GdipSetPenCustomStartCap(GpPen*,GpCustomLineCap*);
GpStatus WINGDIPAPI GdipSetPenDashArray(GpPen*,GDIPCONST REAL*,INT);
GpStatus WINGDIPAPI GdipSetPenDashCap197819(GpPen*,GpDashCap);
GpStatus WINGDIPAPI GdipSetPenDashOffset(GpPen*,REAL);
GpStatus WINGDIPAPI GdipSetPenDashStyle(GpPen*,GpDashStyle);
GpStatus WINGDIPAPI GdipSetPenEndCap(GpPen*,GpLineCap);
GpStatus WINGDIPAPI GdipGetPenFillType(GpPen*,GpPenType*);
GpStatus WINGDIPAPI GdipSetPenLineCap197819(GpPen*,GpLineCap,GpLineCap,GpDashCap);
GpStatus WINGDIPAPI GdipSetPenLineJoin(GpPen*,GpLineJoin);
GpStatus WINGDIPAPI GdipSetPenMode(GpPen*,GpPenAlignment);
GpStatus WINGDIPAPI GdipSetPenMiterLimit(GpPen*,REAL);
GpStatus WINGDIPAPI GdipSetPenStartCap(GpPen*,GpLineCap);
GpStatus WINGDIPAPI GdipSetPenTransform(GpPen *, GpMatrix *);
GpStatus WINGDIPAPI GdipSetPenWidth(GpPen*,REAL);
GpStatus WINGDIPAPI GdipGetPenDashCap197819(GpPen*,GpDashCap*);
GpStatus WINGDIPAPI GdipGetPenEndCap(GpPen*,GpLineCap*);
GpStatus WINGDIPAPI GdipGetPenLineJoin(GpPen*,GpLineJoin*);
GpStatus WINGDIPAPI GdipGetPenMiterLimit(GpPen*,REAL*);
GpStatus WINGDIPAPI GdipGetPenStartCap(GpPen*,GpLineCap*);
GpStatus WINGDIPAPI GdipGetPenUnit(GpPen*,GpUnit*);
GpStatus WINGDIPAPI GdipGetPenWidth(GpPen*,REAL*);
GpStatus WINGDIPAPI GdipTranslatePenTransform(GpPen*,REAL,REAL,GpMatrixOrder);

/* Region */
GpStatus WINGDIPAPI GdipCloneRegion(GpRegion *, GpRegion **);
GpStatus WINGDIPAPI GdipCombineRegionPath(GpRegion *, GpPath *, CombineMode);
GpStatus WINGDIPAPI GdipCombineRegionRect(GpRegion *, GDIPCONST GpRectF *, CombineMode);
GpStatus WINGDIPAPI GdipCombineRegionRectI(GpRegion *, GDIPCONST GpRect *, CombineMode);
GpStatus WINGDIPAPI GdipCombineRegionRegion(GpRegion *, GpRegion *, CombineMode);
GpStatus WINGDIPAPI GdipCreateRegion(GpRegion **);
GpStatus WINGDIPAPI GdipCreateRegionPath(GpPath *, GpRegion **);
GpStatus WINGDIPAPI GdipCreateRegionRect(GDIPCONST GpRectF *, GpRegion **);
GpStatus WINGDIPAPI GdipCreateRegionRectI(GDIPCONST GpRect *, GpRegion **);
GpStatus WINGDIPAPI GdipCreateRegionRgnData(GDIPCONST BYTE *, INT, GpRegion **);
GpStatus WINGDIPAPI GdipCreateRegionHrgn(HRGN, GpRegion **);
GpStatus WINGDIPAPI GdipDeleteRegion(GpRegion *);
GpStatus WINGDIPAPI GdipGetRegionBounds(GpRegion *, GpGraphics *, GpRectF *);
GpStatus WINGDIPAPI GdipGetRegionBoundsI(GpRegion *, GpGraphics *, GpRect *);
GpStatus WINGDIPAPI GdipGetRegionData(GpRegion *, BYTE *, UINT, UINT *);
GpStatus WINGDIPAPI GdipGetRegionDataSize(GpRegion *, UINT *);
GpStatus WINGDIPAPI GdipGetRegionHRgn(GpRegion *, GpGraphics *, HRGN *);
GpStatus WINGDIPAPI GdipGetRegionScans(GpRegion *, GpRectF *, INT *, GpMatrix *);
GpStatus WINGDIPAPI GdipGetRegionScansI(GpRegion *, GpRect *, INT *, GpMatrix *);
GpStatus WINGDIPAPI GdipGetRegionScansCount(GpRegion *, UINT *, GpMatrix *);
GpStatus WINGDIPAPI GdipIsEmptyRegion(GpRegion *, GpGraphics *, BOOL *);
GpStatus WINGDIPAPI GdipIsEqualRegion(GpRegion *, GpRegion *, GpGraphics *, BOOL *);
GpStatus WINGDIPAPI GdipIsInfiniteRegion(GpRegion *, GpGraphics *, BOOL *);
GpStatus WINGDIPAPI GdipIsVisibleRegionPoint(GpRegion *, REAL, REAL, GpGraphics *, BOOL *);
GpStatus WINGDIPAPI GdipIsVisibleRegionPointI(GpRegion *, INT, INT, GpGraphics *, BOOL *);
GpStatus WINGDIPAPI GdipIsVisibleRegionRect(GpRegion *, REAL, REAL, REAL, REAL, GpGraphics *, BOOL *);
GpStatus WINGDIPAPI GdipIsVisibleRegionRectI(GpRegion *, INT, INT, INT, INT, GpGraphics *, BOOL *);
GpStatus WINGDIPAPI GdipSetEmpty(GpRegion *);
GpStatus WINGDIPAPI GdipSetInfinite(GpRegion *);
GpStatus WINGDIPAPI GdipTransformRegion(GpRegion *, GpMatrix *);
GpStatus WINGDIPAPI GdipTranslateRegion(GpRegion *, REAL, REAL);
GpStatus WINGDIPAPI GdipTranslateRegionI(GpRegion *, INT, INT);

/* SolidBrush */
GpStatus WINGDIPAPI GdipCreateSolidFill(ARGB,GpSolidFill**);
GpStatus WINGDIPAPI GdipGetSolidFillColor(GpSolidFill*,ARGB*);
GpStatus WINGDIPAPI GdipSetSolidFillColor(GpSolidFill*,ARGB);

/* StringFormat */
GpStatus WINGDIPAPI GdipCloneStringFormat(GDIPCONST GpStringFormat*,GpStringFormat**);
GpStatus WINGDIPAPI GdipCreateStringFormat(INT,LANGID,GpStringFormat**);
GpStatus WINGDIPAPI GdipDeleteStringFormat(GpStringFormat*);
GpStatus WINGDIPAPI GdipGetStringFormatAlign(GpStringFormat*,StringAlignment*);
GpStatus WINGDIPAPI GdipGetStringFormatDigitSubstitution(GDIPCONST GpStringFormat*,LANGID*,
        StringDigitSubstitute*);
GpStatus WINGDIPAPI GdipGetStringFormatFlags(GDIPCONST GpStringFormat*, INT*);
GpStatus WINGDIPAPI GdipGetStringFormatHotkeyPrefix(GDIPCONST GpStringFormat*,INT*);
GpStatus WINGDIPAPI GdipGetStringFormatLineAlign(GpStringFormat*,StringAlignment*);
GpStatus WINGDIPAPI GdipGetStringFormatMeasurableCharacterRangeCount(
        GDIPCONST GpStringFormat*, INT*);
GpStatus WINGDIPAPI GdipGetStringFormatTabStopCount(GDIPCONST GpStringFormat*,INT*);
GpStatus WINGDIPAPI GdipGetStringFormatTabStops(GDIPCONST GpStringFormat*,INT,REAL*,REAL*);
GpStatus WINGDIPAPI GdipGetStringFormatTrimming(GpStringFormat*,StringTrimming*);
GpStatus WINGDIPAPI GdipSetStringFormatAlign(GpStringFormat*,StringAlignment);
GpStatus WINGDIPAPI GdipSetStringFormatDigitSubstitution(GpStringFormat*,LANGID,StringDigitSubstitute);
GpStatus WINGDIPAPI GdipSetStringFormatHotkeyPrefix(GpStringFormat*,INT);
GpStatus WINGDIPAPI GdipSetStringFormatLineAlign(GpStringFormat*,StringAlignment);
GpStatus WINGDIPAPI GdipSetStringFormatMeasurableCharacterRanges(
        GpStringFormat*, INT, GDIPCONST CharacterRange*);
GpStatus WINGDIPAPI GdipSetStringFormatTabStops(GpStringFormat*,REAL,INT,GDIPCONST REAL*);
GpStatus WINGDIPAPI GdipSetStringFormatTrimming(GpStringFormat*,StringTrimming);
GpStatus WINGDIPAPI GdipSetStringFormatFlags(GpStringFormat*, INT);
GpStatus WINGDIPAPI GdipStringFormatGetGenericDefault(GpStringFormat **);
GpStatus WINGDIPAPI GdipStringFormatGetGenericTypographic(GpStringFormat **);

/* Texture */
GpStatus WINGDIPAPI GdipCreateTexture(GpImage*,GpWrapMode,GpTexture**);
GpStatus WINGDIPAPI GdipCreateTexture2(GpImage*,GpWrapMode,REAL,REAL,REAL,REAL,GpTexture**);
GpStatus WINGDIPAPI GdipCreateTexture2I(GpImage*,GpWrapMode,INT,INT,INT,INT,GpTexture**);
GpStatus WINGDIPAPI GdipCreateTextureIA(GpImage*,GDIPCONST GpImageAttributes*,
    REAL,REAL,REAL,REAL,GpTexture**);
GpStatus WINGDIPAPI GdipCreateTextureIAI(GpImage*,GDIPCONST GpImageAttributes*,
    INT,INT,INT,INT,GpTexture**);
GpStatus WINGDIPAPI GdipGetTextureTransform(GpTexture*,GpMatrix*);
GpStatus WINGDIPAPI GdipGetTextureWrapMode(GpTexture*, GpWrapMode*);
GpStatus WINGDIPAPI GdipMultiplyTextureTransform(GpTexture*,
    GDIPCONST GpMatrix*,GpMatrixOrder);
GpStatus WINGDIPAPI GdipResetTextureTransform(GpTexture*);
GpStatus WINGDIPAPI GdipRotateTextureTransform(GpTexture*,REAL,GpMatrixOrder);
GpStatus WINGDIPAPI GdipScaleTextureTransform(GpTexture*,REAL,REAL,GpMatrixOrder);
GpStatus WINGDIPAPI GdipSetTextureTransform(GpTexture *,GDIPCONST GpMatrix*);
GpStatus WINGDIPAPI GdipSetTextureWrapMode(GpTexture*, GpWrapMode);
GpStatus WINGDIPAPI GdipTranslateTextureTransform(GpTexture*,REAL,REAL,
    GpMatrixOrder);

/* Without wrapper methods */
GpStatus WINGDIPAPI GdipCreateStreamOnFile(GDIPCONST WCHAR*,UINT,IStream**);
GpStatus WINGDIPAPI GdipGetImageEncodersSize(UINT *numEncoders, UINT *size);
GpStatus WINGDIPAPI GdipGetImageEncoders(UINT numEncoders, UINT size, ImageCodecInfo *encoders);
GpStatus WINGDIPAPI GdipTestControl(GpTestControlEnum,void*);

#ifdef __cplusplus
}
#endif

#endif
