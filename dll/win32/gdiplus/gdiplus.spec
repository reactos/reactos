@ stdcall GdipAddPathArc(ptr long long long long long long)
@ stdcall GdipAddPathArcI(ptr long long long long long long)
@ stdcall GdipAddPathBezier(ptr long long long long long long long long)
@ stdcall GdipAddPathBezierI(ptr long long long long long long long long)
@ stdcall GdipAddPathBeziers(ptr ptr long)
@ stdcall GdipAddPathBeziersI(ptr ptr long)
@ stub GdipAddPathClosedCurve2
@ stub GdipAddPathClosedCurve2I
@ stub GdipAddPathClosedCurve
@ stub GdipAddPathClosedCurveI
@ stub GdipAddPathCurve2
@ stub GdipAddPathCurve2I
@ stub GdipAddPathCurve3
@ stub GdipAddPathCurve3I
@ stub GdipAddPathCurve
@ stub GdipAddPathCurveI
@ stdcall GdipAddPathEllipse(ptr long long long long)
@ stdcall GdipAddPathEllipseI(ptr long long long long)
@ stdcall GdipAddPathLine2(ptr ptr long)
@ stdcall GdipAddPathLine2I(ptr ptr long)
@ stdcall GdipAddPathLine(ptr long long long long)
@ stdcall GdipAddPathLineI(ptr long long long long)
@ stdcall GdipAddPathPath(ptr ptr long)
@ stub GdipAddPathPie
@ stub GdipAddPathPieI
@ stub GdipAddPathPolygon
@ stub GdipAddPathPolygonI
@ stdcall GdipAddPathRectangle(ptr long long long long)
@ stdcall GdipAddPathRectangleI(ptr long long long long)
@ stub GdipAddPathRectangles
@ stub GdipAddPathRectanglesI
@ stub GdipAddPathString
@ stub GdipAddPathStringI
@ stdcall GdipAlloc(long)
@ stub GdipBeginContainer2
@ stub GdipBeginContainer
@ stub GdipBeginContainerI
@ stub GdipBitmapApplyEffect
@ stub GdipBitmapConvertFormat
@ stub GdipBitmapCreateApplyEffect
@ stub GdipBitmapGetHistogram
@ stub GdipBitmapGetHistogramSize
@ stdcall GdipBitmapGetPixel(ptr long long ptr)
@ stdcall GdipBitmapLockBits(ptr ptr long long ptr)
@ stub GdipBitmapSetPixel
@ stub GdipBitmapSetResolution
@ stdcall GdipBitmapUnlockBits(ptr ptr)
@ stub GdipClearPathMarkers
@ stub GdipCloneBitmapArea
@ stub GdipCloneBitmapAreaI
@ stdcall GdipCloneBrush(ptr ptr)
@ stdcall GdipCloneCustomLineCap(ptr ptr)
@ stdcall GdipCloneFont(ptr ptr)
@ stub GdipCloneFontFamily
@ stub GdipCloneImage
@ stub GdipCloneImageAttributes
@ stdcall GdipCloneMatrix(ptr ptr)
@ stdcall GdipClonePath(ptr ptr)
@ stdcall GdipClonePen(ptr ptr)
@ stub GdipCloneRegion
@ stdcall GdipCloneStringFormat(ptr ptr)
@ stdcall GdipClosePathFigure(ptr)
@ stdcall GdipClosePathFigures(ptr)
@ stub GdipCombineRegionPath
@ stub GdipCombineRegionRect
@ stub GdipCombineRegionRectI
@ stub GdipCombineRegionRegion
@ stub GdipComment
@ stdcall GdipConvertToEmfPlus(ptr ptr ptr long ptr ptr)
@ stub GdipConvertToEmfPlusToFile
@ stub GdipConvertToEmfPlusToStream
@ stub GdipCreateAdjustableArrowCap
@ stub GdipCreateBitmapFromDirectDrawSurface
@ stdcall GdipCreateBitmapFromFile(wstr ptr)
@ stdcall GdipCreateBitmapFromFileICM(wstr ptr)
@ stub GdipCreateBitmapFromGdiDib
@ stdcall GdipCreateBitmapFromGraphics(long long ptr ptr)
@ stdcall GdipCreateBitmapFromHBITMAP(ptr ptr ptr)
@ stub GdipCreateBitmapFromHICON
@ stub GdipCreateBitmapFromResource
@ stdcall GdipCreateBitmapFromScan0(long long long long ptr ptr)
@ stdcall GdipCreateBitmapFromStream(ptr ptr)
@ stdcall GdipCreateBitmapFromStreamICM(ptr ptr)
@ stub GdipCreateCachedBitmap
@ stdcall GdipCreateCustomLineCap(ptr ptr long long ptr)
@ stub GdipCreateEffect
@ stub GdipCreateFont
@ stub GdipCreateFontFamilyFromName
@ stdcall GdipCreateFontFromDC(long ptr)
@ stdcall GdipCreateFontFromLogfontA(long ptr ptr)
@ stdcall GdipCreateFontFromLogfontW(long ptr ptr)
@ stdcall GdipCreateFromHDC2(long long ptr)
@ stdcall GdipCreateFromHDC(long ptr)
@ stdcall GdipCreateFromHWND(long ptr)
@ stdcall GdipCreateFromHWNDICM(long ptr)
@ stdcall GdipCreateHBITMAPFromBitmap(ptr ptr long)
@ stub GdipCreateHICONFromBitmap
@ stub GdipCreateHalftonePalette
@ stub GdipCreateHatchBrush
@ stdcall GdipCreateImageAttributes(ptr)
@ stdcall GdipCreateLineBrush(ptr ptr long long long ptr)
@ stdcall GdipCreateLineBrushFromRect(ptr long long long long ptr)
@ stdcall GdipCreateLineBrushFromRectI(ptr long long long long ptr)
@ stub GdipCreateLineBrushFromRectWithAngle
@ stub GdipCreateLineBrushFromRectWithAngleI
@ stdcall GdipCreateLineBrushI(ptr ptr long long long ptr)
@ stdcall GdipCreateMatrix2(long long long long long long ptr)
@ stdcall GdipCreateMatrix3(ptr ptr ptr)
@ stdcall GdipCreateMatrix3I(ptr ptr ptr)
@ stdcall GdipCreateMatrix(ptr)
@ stdcall GdipCreateMetafileFromEmf(ptr long ptr)
@ stub GdipCreateMetafileFromFile
@ stub GdipCreateMetafileFromStream
@ stdcall GdipCreateMetafileFromWmf(ptr long ptr ptr)
@ stub GdipCreateMetafileFromWmfFile
@ stdcall GdipCreatePath2(ptr ptr long long ptr)
@ stdcall GdipCreatePath2I(ptr ptr long long ptr)
@ stdcall GdipCreatePath(long ptr)
@ stdcall GdipCreatePathGradient(ptr long long ptr)
@ stdcall GdipCreatePathGradientFromPath(ptr ptr)
@ stdcall GdipCreatePathGradientI(ptr long long ptr)
@ stdcall GdipCreatePathIter(ptr ptr)
@ stdcall GdipCreatePen1(long long long ptr)
@ stdcall GdipCreatePen2(ptr long long ptr)
@ stdcall GdipCreateRegion(ptr)
@ stub GdipCreateRegionHrgn
@ stdcall GdipCreateRegionPath(ptr ptr)
@ stub GdipCreateRegionRect
@ stub GdipCreateRegionRectI
@ stub GdipCreateRegionRgnData
@ stdcall GdipCreateSolidFill(long ptr)
@ stdcall GdipCreateStreamOnFile(ptr long ptr)
@ stdcall GdipCreateStringFormat(long long ptr)
@ stub GdipCreateTexture2
@ stub GdipCreateTexture2I
@ stub GdipCreateTexture
@ stdcall GdipCreateTextureIA(ptr ptr long long long long ptr)
@ stub GdipCreateTextureIAI
@ stdcall GdipDeleteBrush(ptr)
@ stub GdipDeleteCachedBitmap
@ stdcall GdipDeleteCustomLineCap(ptr)
@ stub GdipDeleteEffect
@ stdcall GdipDeleteFont(ptr)
@ stub GdipDeleteFontFamily
@ stdcall GdipDeleteGraphics(ptr)
@ stdcall GdipDeleteMatrix(ptr)
@ stdcall GdipDeletePath(ptr)
@ stdcall GdipDeletePathIter(ptr)
@ stdcall GdipDeletePen(ptr)
@ stub GdipDeletePrivateFontCollection
@ stdcall GdipDeleteRegion(ptr)
@ stdcall GdipDeleteStringFormat(ptr)
@ stdcall GdipDisposeImage(ptr)
@ stdcall GdipDisposeImageAttributes(ptr)
@ stdcall GdipDrawArc(ptr ptr long long long long long long)
@ stdcall GdipDrawArcI(ptr ptr long long long long long long)
@ stdcall GdipDrawBezier(ptr ptr long long long long long long long long)
@ stdcall GdipDrawBezierI(ptr ptr long long long long long long long long)
@ stub GdipDrawBeziers
@ stub GdipDrawBeziersI
@ stub GdipDrawCachedBitmap
@ stub GdipDrawClosedCurve2
@ stub GdipDrawClosedCurve2I
@ stub GdipDrawClosedCurve
@ stub GdipDrawClosedCurveI
@ stdcall GdipDrawCurve2(ptr ptr ptr long long)
@ stdcall GdipDrawCurve2I(ptr ptr ptr long long)
@ stub GdipDrawCurve3
@ stub GdipDrawCurve3I
@ stdcall GdipDrawCurve(ptr ptr ptr long)
@ stdcall GdipDrawCurveI(ptr ptr ptr long)
@ stub GdipDrawDriverString
@ stub GdipDrawEllipse
@ stub GdipDrawEllipseI
@ stdcall GdipDrawImage(ptr ptr long long)
@ stub GdipDrawImageFX
@ stdcall GdipDrawImageI(ptr ptr long long)
@ stub GdipDrawImagePointRect
@ stub GdipDrawImagePointRectI
@ stub GdipDrawImagePoints
@ stub GdipDrawImagePointsI
@ stdcall GdipDrawImagePointsRect(ptr ptr ptr long long long long long long ptr ptr ptr)
@ stdcall GdipDrawImagePointsRectI(ptr ptr ptr long long long long long long ptr ptr ptr)
@ stdcall GdipDrawImageRect(ptr ptr long long long long)
@ stdcall GdipDrawImageRectI(ptr ptr long long long long)
@ stdcall GdipDrawImageRectRect(ptr ptr long long long long long long long long long ptr long ptr)
@ stdcall GdipDrawImageRectRectI(ptr ptr long long long long long long long long long ptr long ptr)
@ stdcall GdipDrawLine(ptr ptr long long long long)
@ stdcall GdipDrawLineI(ptr ptr long long long long)
@ stdcall GdipDrawLines(ptr ptr ptr long)
@ stdcall GdipDrawLinesI(ptr ptr ptr long)
@ stdcall GdipDrawPath(ptr ptr ptr)
@ stdcall GdipDrawPie(ptr ptr long long long long long long)
@ stdcall GdipDrawPieI(ptr ptr long long long long long long)
@ stdcall GdipDrawPolygon(ptr ptr ptr long)
@ stdcall GdipDrawPolygonI(ptr ptr ptr long)
@ stdcall GdipDrawRectangle(ptr ptr long long long long)
@ stdcall GdipDrawRectangleI(ptr ptr long long long long)
@ stdcall GdipDrawRectangles(ptr ptr ptr long)
@ stdcall GdipDrawRectanglesI(ptr ptr ptr long)
@ stdcall GdipDrawString(ptr ptr long ptr ptr ptr ptr)
@ stub GdipEmfToWmfBits
@ stub GdipEndContainer
@ stub GdipEnumerateMetafileDestPoint
@ stub GdipEnumerateMetafileDestPointI
@ stub GdipEnumerateMetafileDestPoints
@ stub GdipEnumerateMetafileDestPointsI
@ stub GdipEnumerateMetafileDestRect
@ stub GdipEnumerateMetafileDestRectI
@ stub GdipEnumerateMetafileSrcRectDestPoint
@ stub GdipEnumerateMetafileSrcRectDestPointI
@ stub GdipEnumerateMetafileSrcRectDestPoints
@ stub GdipEnumerateMetafileSrcRectDestPointsI
@ stub GdipEnumerateMetafileSrcRectDestRect
@ stub GdipEnumerateMetafileSrcRectDestRectI
@ stub GdipFillClosedCurve2
@ stub GdipFillClosedCurve2I
@ stub GdipFillClosedCurve
@ stub GdipFillClosedCurveI
@ stdcall GdipFillEllipse(ptr ptr long long long long)
@ stdcall GdipFillEllipseI(ptr ptr long long long long)
@ stdcall GdipFillPath(ptr ptr ptr)
@ stdcall GdipFillPie(ptr ptr long long long long long long)
@ stdcall GdipFillPieI(ptr ptr long long long long long long)
@ stub GdipFillPolygon2
@ stub GdipFillPolygon2I
@ stdcall GdipFillPolygon(ptr ptr ptr long long)
@ stdcall GdipFillPolygonI(ptr ptr ptr long long)
@ stdcall GdipFillRectangle(ptr ptr long long long long)
@ stdcall GdipFillRectangleI(ptr ptr long long long long)
@ stdcall GdipFillRectangles(ptr ptr ptr long)
@ stdcall GdipFillRectanglesI(ptr ptr ptr long)
@ stub GdipFillRegion
@ stdcall GdipFindFirstImageItem(ptr ptr)
@ stub GdipFindNextImageItem
@ stub GdipFlattenPath
@ stub GdipFlush
@ stdcall GdipFree(ptr)
@ stub GdipGetAdjustableArrowCapFillState
@ stub GdipGetAdjustableArrowCapHeight
@ stub GdipGetAdjustableArrowCapMiddleInset
@ stub GdipGetAdjustableArrowCapWidth
@ stub GdipGetAllPropertyItems
@ stdcall GdipGetBrushType(ptr ptr)
@ stub GdipGetCellAscent
@ stub GdipGetCellDescent
@ stdcall GdipGetClip(ptr ptr)
@ stub GdipGetClipBounds
@ stub GdipGetClipBoundsI
@ stdcall GdipGetCompositingMode(ptr ptr)
@ stdcall GdipGetCompositingQuality(ptr ptr)
@ stdcall GdipGetCustomLineCapBaseCap(ptr ptr)
@ stdcall GdipGetCustomLineCapBaseInset(ptr ptr)
@ stub GdipGetCustomLineCapStrokeCaps
@ stub GdipGetCustomLineCapStrokeJoin
@ stub GdipGetCustomLineCapType
@ stub GdipGetCustomLineCapWidthScale
@ stdcall GdipGetDC(ptr ptr)
@ stdcall GdipGetDpiX(ptr ptr)
@ stdcall GdipGetDpiY(ptr ptr)
@ stub GdipGetEffectParameterSize
@ stub GdipGetEffectParameters
@ stub GdipGetEmHeight
@ stub GdipGetEncoderParameterList
@ stub GdipGetEncoderParameterListSize
@ stub GdipGetFamily
@ stub GdipGetFamilyName
@ stub GdipGetFontCollectionFamilyCount
@ stub GdipGetFontCollectionFamilyList
@ stub GdipGetFontHeight
@ stub GdipGetFontHeightGivenDPI
@ stub GdipGetFontSize
@ stub GdipGetFontStyle
@ stub GdipGetFontUnit
@ stub GdipGetGenericFontFamilyMonospace
@ stub GdipGetGenericFontFamilySansSerif
@ stub GdipGetGenericFontFamilySerif
@ stub GdipGetHatchBackgroundColor
@ stub GdipGetHatchForegroundColor
@ stub GdipGetHatchStyle
@ stub GdipGetHemfFromMetafile
@ stub GdipGetImageAttributesAdjustedPalette
@ stdcall GdipGetImageBounds(ptr ptr ptr)
@ stub GdipGetImageDecoders
@ stub GdipGetImageDecodersSize
@ stdcall GdipGetImageDimension(ptr ptr ptr)
@ stdcall GdipGetImageEncoders(long long ptr)
@ stdcall GdipGetImageEncodersSize(ptr ptr)
@ stdcall GdipGetImageFlags(ptr ptr)
@ stdcall GdipGetImageGraphicsContext(ptr ptr)
@ stdcall GdipGetImageHeight(ptr ptr)
@ stdcall GdipGetImageHorizontalResolution(ptr ptr)
@ stub GdipGetImageItemData
@ stub GdipGetImagePalette
@ stub GdipGetImagePaletteSize
@ stdcall GdipGetImagePixelFormat(ptr ptr)
@ stdcall GdipGetImageRawFormat(ptr ptr)
@ stub GdipGetImageThumbnail
@ stdcall GdipGetImageType(ptr ptr)
@ stdcall GdipGetImageVerticalResolution(ptr ptr)
@ stdcall GdipGetImageWidth(ptr ptr)
@ stdcall GdipGetInterpolationMode(ptr ptr)
@ stub GdipGetLineBlend
@ stub GdipGetLineBlendCount
@ stdcall GdipGetLineColors(ptr ptr)
@ stdcall GdipGetLineGammaCorrection(ptr ptr)
@ stub GdipGetLinePresetBlend
@ stub GdipGetLinePresetBlendCount
@ stdcall GdipGetLineRect(ptr ptr)
@ stdcall GdipGetLineRectI(ptr ptr)
@ stub GdipGetLineSpacing
@ stub GdipGetLineTransform
@ stub GdipGetLineWrapMode
@ stub GdipGetLogFontA
@ stdcall GdipGetLogFontW(ptr ptr ptr)
@ stdcall GdipGetMatrixElements(ptr ptr)
@ stub GdipGetMetafileDownLevelRasterizationLimit
@ stub GdipGetMetafileHeaderFromEmf
@ stub GdipGetMetafileHeaderFromFile
@ stdcall GdipGetMetafileHeaderFromMetafile(ptr ptr)
@ stub GdipGetMetafileHeaderFromStream
@ stub GdipGetMetafileHeaderFromWmf
@ stub GdipGetNearestColor
@ stdcall GdipGetPageScale(ptr ptr)
@ stdcall GdipGetPageUnit(ptr ptr)
@ stub GdipGetPathData
@ stdcall GdipGetPathFillMode(ptr ptr)
@ stub GdipGetPathGradientBlend
@ stub GdipGetPathGradientBlendCount
@ stub GdipGetPathGradientCenterColor
@ stdcall GdipGetPathGradientCenterPoint(ptr ptr)
@ stdcall GdipGetPathGradientCenterPointI(ptr ptr)
@ stdcall GdipGetPathGradientFocusScales(ptr ptr ptr)
@ stdcall GdipGetPathGradientGammaCorrection(ptr ptr)
@ stub GdipGetPathGradientPath
@ stdcall GdipGetPathGradientPointCount(ptr ptr)
@ stub GdipGetPathGradientPresetBlend
@ stub GdipGetPathGradientPresetBlendCount
@ stub GdipGetPathGradientRect
@ stub GdipGetPathGradientRectI
@ stub GdipGetPathGradientSurroundColorCount
@ stdcall GdipGetPathGradientSurroundColorsWithCount(ptr ptr ptr)
@ stub GdipGetPathGradientTransform
@ stub GdipGetPathGradientWrapMode
@ stub GdipGetPathLastPoint
@ stdcall GdipGetPathPoints(ptr ptr long)
@ stdcall GdipGetPathPointsI(ptr ptr long)
@ stdcall GdipGetPathTypes(ptr ptr long)
@ stdcall GdipGetPathWorldBounds(ptr ptr ptr ptr)
@ stdcall GdipGetPathWorldBoundsI(ptr ptr ptr ptr)
@ stdcall GdipGetPenBrushFill(ptr ptr)
@ stdcall GdipGetPenColor(ptr ptr)
@ stub GdipGetPenCompoundArray
@ stub GdipGetPenCompoundCount
@ stub GdipGetPenCustomEndCap
@ stub GdipGetPenCustomStartCap
@ stdcall GdipGetPenDashArray(ptr ptr long)
@ stub GdipGetPenDashCap197819
@ stub GdipGetPenDashCount
@ stdcall GdipGetPenDashOffset(ptr ptr)
@ stdcall GdipGetPenDashStyle(ptr ptr)
@ stub GdipGetPenEndCap
@ stub GdipGetPenFillType
@ stub GdipGetPenLineJoin
@ stub GdipGetPenMiterLimit
@ stub GdipGetPenMode
@ stub GdipGetPenStartCap
@ stub GdipGetPenTransform
@ stub GdipGetPenUnit
@ stub GdipGetPenWidth
@ stdcall GdipGetPixelOffsetMode(ptr ptr)
@ stdcall GdipGetPointCount(ptr ptr)
@ stub GdipGetPropertyCount
@ stub GdipGetPropertyIdList
@ stub GdipGetPropertyItem
@ stdcall GdipGetPropertyItemSize(ptr long ptr)
@ stub GdipGetPropertySize
@ stub GdipGetRegionBounds
@ stub GdipGetRegionBoundsI
@ stub GdipGetRegionData
@ stub GdipGetRegionDataSize
@ stdcall GdipGetRegionHRgn(ptr ptr ptr)
@ stub GdipGetRegionScans
@ stub GdipGetRegionScansCount
@ stub GdipGetRegionScansI
@ stub GdipGetRenderingOrigin
@ stdcall GdipGetSmoothingMode(ptr ptr)
@ stdcall GdipGetSolidFillColor(ptr ptr)
@ stdcall GdipGetStringFormatAlign(ptr ptr)
@ stub GdipGetStringFormatDigitSubstitution
@ stub GdipGetStringFormatFlags
@ stdcall GdipGetStringFormatHotkeyPrefix(ptr ptr)
@ stdcall GdipGetStringFormatLineAlign(ptr ptr)
@ stub GdipGetStringFormatMeasurableCharacterRangeCount
@ stub GdipGetStringFormatTabStopCount
@ stub GdipGetStringFormatTabStops
@ stdcall GdipGetStringFormatTrimming(ptr ptr)
@ stub GdipGetTextContrast
@ stdcall GdipGetTextRenderingHint(ptr ptr)
@ stub GdipGetTextureImage
@ stub GdipGetTextureTransform
@ stub GdipGetTextureWrapMode
@ stub GdipGetVisibleClipBounds
@ stub GdipGetVisibleClipBoundsI
@ stdcall GdipGetWorldTransform(ptr ptr)
@ stub GdipGraphicsClear
@ stub GdipGraphicsSetAbort
@ stub GdipImageForceValidation
@ stdcall GdipImageGetFrameCount(ptr ptr ptr)
@ stub GdipImageGetFrameDimensionsCount
@ stdcall GdipImageGetFrameDimensionsList(ptr ptr long)
@ stub GdipImageRotateFlip
@ stdcall GdipImageSelectActiveFrame(ptr ptr long)
@ stub GdipImageSetAbort
@ stub GdipInitializePalette
@ stub GdipInvertMatrix
@ stub GdipIsClipEmpty
@ stub GdipIsEmptyRegion
@ stub GdipIsEqualRegion
@ stub GdipIsInfiniteRegion
@ stdcall GdipIsMatrixEqual(ptr ptr ptr)
@ stdcall GdipIsMatrixIdentity(ptr ptr)
@ stub GdipIsMatrixInvertible
@ stub GdipIsOutlineVisiblePathPoint
@ stdcall GdipIsOutlineVisiblePathPointI(ptr long long ptr ptr ptr)
@ stub GdipIsStyleAvailable
@ stub GdipIsVisibleClipEmpty
@ stub GdipIsVisiblePathPoint
@ stub GdipIsVisiblePathPointI
@ stub GdipIsVisiblePoint
@ stub GdipIsVisiblePointI
@ stub GdipIsVisibleRect
@ stub GdipIsVisibleRectI
@ stub GdipIsVisibleRegionPoint
@ stub GdipIsVisibleRegionPointI
@ stub GdipIsVisibleRegionRect
@ stub GdipIsVisibleRegionRectI
@ stdcall GdipLoadImageFromFile(wstr ptr)
@ stdcall GdipLoadImageFromFileICM(wstr ptr)
@ stdcall GdipLoadImageFromStream(ptr ptr)
@ stdcall GdipLoadImageFromStreamICM(ptr ptr)
@ stub GdipMeasureCharacterRanges
@ stub GdipMeasureDriverString
@ stdcall GdipMeasureString(ptr ptr long ptr ptr ptr ptr ptr ptr)
@ stub GdipMultiplyLineTransform
@ stdcall GdipMultiplyMatrix(ptr ptr long)
@ stub GdipMultiplyPathGradientTransform
@ stub GdipMultiplyPenTransform
@ stub GdipMultiplyTextureTransform
@ stdcall GdipMultiplyWorldTransform(ptr ptr long)
@ stub GdipNewInstalledFontCollection
@ stub GdipNewPrivateFontCollection
@ stdcall GdipPathIterCopyData(ptr ptr ptr ptr long long)
@ stub GdipPathIterEnumerate
@ stub GdipPathIterGetCount
@ stub GdipPathIterGetSubpathCount
@ stub GdipPathIterHasCurve
@ stub GdipPathIterIsValid
@ stub GdipPathIterNextMarker
@ stub GdipPathIterNextMarkerPath
@ stub GdipPathIterNextPathType
@ stdcall GdipPathIterNextSubpath(ptr ptr ptr ptr ptr)
@ stub GdipPathIterNextSubpathPath
@ stdcall GdipPathIterRewind(ptr)
@ stub GdipPlayMetafileRecord
@ stub GdipPlayTSClientRecord
@ stub GdipPrivateAddFontFile
@ stub GdipPrivateAddMemoryFont
@ stub GdipRecordMetafile
@ stub GdipRecordMetafileFileName
@ stub GdipRecordMetafileFileNameI
@ stub GdipRecordMetafileI
@ stub GdipRecordMetafileStream
@ stub GdipRecordMetafileStreamI
@ stdcall GdipReleaseDC(ptr ptr)
@ stdcall GdipRemovePropertyItem(ptr long)
@ stub GdipResetClip
@ stub GdipResetImageAttributes
@ stub GdipResetLineTransform
@ stub GdipResetPageTransform
@ stdcall GdipResetPath(ptr)
@ stub GdipResetPathGradientTransform
@ stub GdipResetPenTransform
@ stub GdipResetTextureTransform
@ stub GdipResetWorldTransform
@ stdcall GdipRestoreGraphics(ptr long)
@ stub GdipReversePath
@ stub GdipRotateLineTransform
@ stdcall GdipRotateMatrix(ptr long long)
@ stub GdipRotatePathGradientTransform
@ stub GdipRotatePenTransform
@ stub GdipRotateTextureTransform
@ stdcall GdipRotateWorldTransform(ptr long long)
@ stub GdipSaveAdd
@ stub GdipSaveAddImage
@ stdcall GdipSaveGraphics(ptr ptr)
@ stdcall GdipSaveImageToFile(ptr ptr ptr ptr)
@ stdcall GdipSaveImageToStream(ptr ptr ptr ptr)
@ stub GdipScaleLineTransform
@ stdcall GdipScaleMatrix(ptr long long long)
@ stub GdipScalePathGradientTransform
@ stub GdipScalePenTransform
@ stub GdipScaleTextureTransform
@ stdcall GdipScaleWorldTransform(ptr long long long)
@ stub GdipSetAdjustableArrowCapFillState
@ stub GdipSetAdjustableArrowCapHeight
@ stub GdipSetAdjustableArrowCapMiddleInset
@ stub GdipSetAdjustableArrowCapWidth
@ stub GdipSetClipGraphics
@ stub GdipSetClipHrgn
@ stub GdipSetClipPath
@ stub GdipSetClipRect
@ stdcall GdipSetClipRectI(ptr long long long long long)
@ stdcall GdipSetClipRegion(ptr ptr long)
@ stdcall GdipSetCompositingMode(ptr long)
@ stdcall GdipSetCompositingQuality(ptr long)
@ stdcall GdipSetCustomLineCapBaseCap(ptr long)
@ stdcall GdipSetCustomLineCapBaseInset(ptr long)
@ stdcall GdipSetCustomLineCapStrokeCaps(ptr long long)
@ stdcall GdipSetCustomLineCapStrokeJoin(ptr long)
@ stdcall GdipSetCustomLineCapWidthScale(ptr long)
@ stdcall GdipSetEffectParameters(ptr ptr long)
@ stdcall GdipSetEmpty(ptr)
@ stdcall GdipSetImageAttributesCachedBackground(ptr long)
@ stdcall GdipSetImageAttributesColorKeys(ptr long long long long)
@ stdcall GdipSetImageAttributesColorMatrix(ptr long long ptr ptr long)
@ stdcall GdipSetImageAttributesGamma(ptr long long long)
@ stdcall GdipSetImageAttributesNoOp(ptr long long)
@ stdcall GdipSetImageAttributesOutputChannel(ptr long long long)
@ stdcall GdipSetImageAttributesOutputChannelColorProfile(ptr long long ptr)
@ stdcall GdipSetImageAttributesRemapTable(ptr long long long ptr)
@ stdcall GdipSetImageAttributesThreshold(ptr long long long)
@ stdcall GdipSetImageAttributesToIdentity(ptr long)
@ stdcall GdipSetImageAttributesWrapMode(ptr long long long)
@ stdcall GdipSetImagePalette(ptr ptr)
@ stdcall GdipSetInfinite(ptr)
@ stdcall GdipSetInterpolationMode(ptr long)
@ stdcall GdipSetLineBlend(ptr ptr ptr long)
@ stdcall GdipSetLineColors(ptr long long)
@ stdcall GdipSetLineGammaCorrection(ptr long)
@ stdcall GdipSetLineLinearBlend(ptr long long)
@ stdcall GdipSetLinePresetBlend(ptr ptr ptr long)
@ stdcall GdipSetLineSigmaBlend(ptr long long)
@ stdcall GdipSetLineTransform(ptr ptr)
@ stdcall GdipSetLineWrapMode(ptr long)
@ stdcall GdipSetMatrixElements(ptr long long long long long long)
@ stdcall GdipSetMetafileDownLevelRasterizationLimit(ptr long)
@ stdcall GdipSetPageScale(ptr long)
@ stdcall GdipSetPageUnit(ptr long)
@ stdcall GdipSetPathFillMode(ptr long)
@ stub GdipSetPathGradientBlend
@ stdcall GdipSetPathGradientCenterColor(ptr long)
@ stdcall GdipSetPathGradientCenterPoint(ptr ptr)
@ stdcall GdipSetPathGradientCenterPointI(ptr ptr)
@ stdcall GdipSetPathGradientFocusScales(ptr long long)
@ stdcall GdipSetPathGradientGammaCorrection(ptr long)
@ stub GdipSetPathGradientLinearBlend
@ stub GdipSetPathGradientPath
@ stub GdipSetPathGradientPresetBlend
@ stdcall GdipSetPathGradientSigmaBlend(ptr long long)
@ stdcall GdipSetPathGradientSurroundColorsWithCount(ptr ptr ptr)
@ stub GdipSetPathGradientTransform
@ stdcall GdipSetPathGradientWrapMode(ptr long)
@ stub GdipSetPathMarker
@ stdcall GdipSetPenBrushFill(ptr ptr)
@ stdcall GdipSetPenColor(ptr long)
@ stub GdipSetPenCompoundArray
@ stdcall GdipSetPenCustomEndCap(ptr ptr)
@ stdcall GdipSetPenCustomStartCap(ptr ptr)
@ stdcall GdipSetPenDashArray(ptr ptr long)
@ stub GdipSetPenDashCap197819
@ stdcall GdipSetPenDashOffset(ptr long)
@ stdcall GdipSetPenDashStyle(ptr long)
@ stdcall GdipSetPenEndCap(ptr long)
@ stdcall GdipSetPenLineCap197819(ptr long long long)
@ stdcall GdipSetPenLineJoin(ptr long)
@ stdcall GdipSetPenMiterLimit(ptr long)
@ stdcall GdipSetPenMode(ptr long)
@ stdcall GdipSetPenStartCap(ptr long)
@ stub GdipSetPenTransform
@ stub GdipSetPenUnit
@ stdcall GdipSetPenWidth(ptr long)
@ stdcall GdipSetPixelOffsetMode(ptr long)
@ stub GdipSetPropertyItem
@ stub GdipSetRenderingOrigin
@ stdcall GdipSetSmoothingMode(ptr long)
@ stdcall GdipSetSolidFillColor(ptr ptr)
@ stdcall GdipSetStringFormatAlign(ptr long)
@ stub GdipSetStringFormatDigitSubstitution
@ stdcall GdipSetStringFormatFlags(ptr long)
@ stdcall GdipSetStringFormatHotkeyPrefix(ptr long)
@ stdcall GdipSetStringFormatLineAlign(ptr long)
@ stub GdipSetStringFormatMeasurableCharacterRanges
@ stub GdipSetStringFormatTabStops
@ stdcall GdipSetStringFormatTrimming(ptr long)
@ stub GdipSetTextContrast
@ stdcall GdipSetTextRenderingHint(ptr long)
@ stdcall GdipSetTextureTransform(ptr ptr)
@ stub GdipSetTextureWrapMode
@ stdcall GdipSetWorldTransform(ptr ptr)
@ stub GdipShearMatrix
@ stdcall GdipStartPathFigure(ptr)
@ stub GdipStringFormatGetGenericDefault
@ stub GdipStringFormatGetGenericTypographic
@ stub GdipTestControl
@ stdcall GdipTransformMatrixPoints(ptr ptr long)
@ stdcall GdipTransformMatrixPointsI(ptr ptr long)
@ stdcall GdipTransformPath(ptr ptr)
@ stub GdipTransformPoints
@ stub GdipTransformPointsI
@ stub GdipTransformRegion
@ stub GdipTranslateClip
@ stub GdipTranslateClipI
@ stub GdipTranslateLineTransform
@ stdcall GdipTranslateMatrix(ptr long long long)
@ stub GdipTranslatePathGradientTransform
@ stub GdipTranslatePenTransform
@ stub GdipTranslateRegion
@ stub GdipTranslateRegionI
@ stub GdipTranslateTextureTransform
@ stdcall GdipTranslateWorldTransform(ptr long long long)
@ stdcall GdipVectorTransformMatrixPoints(ptr ptr long)
@ stdcall GdipVectorTransformMatrixPointsI(ptr ptr long)
@ stub GdipWarpPath
@ stub GdipWidenPath
@ stub GdipWindingModeOutline
@ stub GdiplusNotificationHook
@ stub GdiplusNotificationUnhook
@ stdcall GdiplusShutdown(ptr)
@ stdcall GdiplusStartup(ptr ptr ptr)
