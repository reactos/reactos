@ stdcall AbortDoc(long)
@ stdcall AbortPath(long)
@ stdcall AddFontMemResourceEx(ptr long ptr ptr)
@ stdcall AddFontResourceA(str)
@ stdcall AddFontResourceExA(str long ptr)
@ stdcall AddFontResourceExW(wstr long ptr)
@ stub AddFontResourceTracking
@ stdcall AddFontResourceW(wstr)
@ stdcall AngleArc(long long long long long long)
@ stdcall AnimatePalette(long long long ptr)
@ stdcall AnyLinkedFonts() NtGdiAnyLinkedFonts
@ stdcall Arc(long long long long long long long long long)
@ stdcall ArcTo(long long long long long long long long long)
@ stdcall BeginPath(long)
@ stdcall bInitSystemAndFontsDirectoriesW(wstr wstr)
@ stdcall BitBlt(long long long long long long long long long)
@ stdcall bMakePathNameW(wstr wstr wstr long)
@ stdcall BRUSHOBJ_hGetColorTransform(ptr) NtGdiBRUSHOBJ_hGetColorTransform
@ stdcall BRUSHOBJ_pvAllocRbrush(ptr long) NtGdiBRUSHOBJ_pvAllocRbrush
@ stdcall BRUSHOBJ_pvGetRbrush(ptr) NtGdiBRUSHOBJ_pvGetRbrush
@ stdcall BRUSHOBJ_ulGetBrushColor(ptr) NtGdiBRUSHOBJ_ulGetBrushColor
@ stub ByeByeGDI
@ stdcall CancelDC(long)
@ stdcall cGetTTFFromFOT(long long long long long long long)
@ stdcall CheckColorsInGamut(ptr ptr ptr long)
@ stdcall ChoosePixelFormat(long ptr)
@ stdcall Chord(long long long long long long long long long)
@ stdcall ClearBitmapAttributes(ptr long)
@ stdcall ClearBrushAttributes(ptr long)
@ stdcall CLIPOBJ_bEnum(ptr long long) NtGdiCLIPOBJ_bEnum
@ stdcall CLIPOBJ_cEnumStart(ptr long long long long) NtGdiCLIPOBJ_cEnumStart
@ stdcall CLIPOBJ_ppoGetPath(ptr) NtGdiCLIPOBJ_ppoGetPath
@ stdcall CloseEnhMetaFile(long)
@ stdcall CloseFigure(long)
@ stdcall CloseMetaFile(long)
@ stdcall ColorCorrectPalette(ptr ptr long long)
@ stdcall ColorMatchToTarget(ptr ptr long)
@ stdcall CombineRgn(long long long long)
@ stdcall CombineTransform(ptr ptr ptr) NtGdiCombineTransform
@ stdcall CopyEnhMetaFileA(long str)
@ stdcall CopyEnhMetaFileW(long wstr)
@ stdcall CopyMetaFileA(long str)
@ stdcall CopyMetaFileW(long wstr)
@ stdcall CreateBitmap(long long long long ptr)
@ stdcall CreateBitmapIndirect(ptr)
@ stdcall CreateBrushIndirect(ptr)
@ stdcall CreateColorSpaceA(ptr)
@ stdcall CreateColorSpaceW(ptr)
@ stdcall CreateCompatibleBitmap(long long long)
@ stdcall CreateCompatibleDC(long)
@ stdcall CreateDCA(str str str ptr)
@ stdcall CreateDCW(wstr wstr wstr ptr)
@ stdcall CreateDIBitmap(long ptr long ptr ptr long)
@ stdcall CreateDIBPatternBrush(long long)
@ stdcall CreateDIBPatternBrushPt(long long)
@ stdcall CreateDIBSection(long ptr long ptr long long)
@ stdcall CreateDiscardableBitmap(long long long)
@ stdcall CreateEllipticRgn(long long long long) NtGdiCreateEllipticRgn
@ stdcall CreateEllipticRgnIndirect(ptr)
@ stdcall CreateEnhMetaFileA(long str ptr str)
@ stdcall CreateEnhMetaFileW(long wstr ptr wstr)
@ stdcall CreateFontA(long long long long long long long long long long long long long str)
@ stdcall CreateFontIndirectA(ptr)
# @ stub CreateFontIndirectExA
# @ stub CreateFontIndirectExW
@ stdcall CreateFontIndirectW(ptr)
@ stdcall CreateFontW(long long long long long long long long long long long long long wstr)
@ stdcall CreateHalftonePalette(long) NtGdiCreateHalftonePalette
@ stdcall CreateHatchBrush(long long)
@ stdcall CreateICA(str str str ptr)
@ stdcall CreateICW(wstr wstr wstr ptr)
@ stdcall CreateMetaFileA(str)
@ stdcall CreateMetaFileW(wstr)
@ stdcall CreatePalette(ptr)
@ stdcall CreatePatternBrush(long)
@ stdcall CreatePen(long long long)
@ stdcall CreatePenIndirect(ptr)
@ stdcall CreatePolyPolygonRgn(ptr ptr long long)
@ stdcall CreatePolygonRgn(ptr long long)
@ stdcall CreateRectRgn(long long long long)
@ stdcall CreateRectRgnIndirect(ptr)
@ stdcall CreateRoundRectRgn(long long long long long long) NtGdiCreateRoundRectRgn
@ stdcall CreateScalableFontResourceA(long str str str)
@ stdcall CreateScalableFontResourceW(long wstr wstr wstr)
@ stdcall CreateSolidBrush(long)
@ stdcall DdEntry0(ptr ptr ptr ptr ptr ptr) NtGdiDxgGenericThunk
@ stdcall DdEntry10(ptr ptr) NtGdiDdBeginMoCompFrame
@ stdcall DdEntry11(ptr ptr ptr) NtGdiDdBlt
@ stdcall DdEntry12(ptr ptr) NtGdiDdCanCreateSurface
@ stdcall DdEntry13(ptr ptr) NtGdiDdCanCreateD3DBuffer
@ stdcall DdEntry14(ptr ptr) NtGdiDdColorControl
@ stdcall DdEntry15(ptr) NtGdiDdCreateDirectDrawObject
@ stdcall DdEntry16(ptr ptr ptr ptr ptr ptr ptr ptr) NtGdiDdCreateSurface
@ stdcall DdEntry17(ptr ptr ptr ptr ptr ptr ptr ptr) NtGdiDdCreateD3DBuffer
@ stdcall DdEntry18(ptr ptr) NtGdiDdCreateMoComp
@ stdcall DdEntry19(ptr ptr ptr ptr ptr long) NtGdiDdCreateSurfaceObject
@ stdcall DdEntry1(ptr ptr ptr ptr) NtGdiD3dContextCreate
@ stdcall DdEntry20(ptr) NtGdiDdDeleteDirectDrawObject
@ stdcall DdEntry21(ptr) NtGdiDdDeleteSurfaceObject
@ stdcall DdEntry22(ptr ptr) NtGdiDdDestroyMoComp
@ stdcall DdEntry23(ptr long) NtGdiDdDestroySurface
@ stdcall DdEntry24(ptr) NtGdiDdDestroyD3DBuffer
@ stdcall DdEntry25(ptr ptr) NtGdiDdEndMoCompFrame
@ stdcall DdEntry26(ptr ptr ptr ptr ptr) NtGdiDdFlip
@ stdcall DdEntry27(ptr long) NtGdiDdFlipToGDISurface
@ stdcall DdEntry28(ptr ptr) NtGdiDdGetAvailDriverMemory
@ stdcall DdEntry29(ptr ptr) NtGdiDdGetBltStatus
@ stdcall DdEntry2(ptr)NtGdiD3dContextDestroy
@ stdcall DdEntry30(ptr ptr) NtGdiDdGetDC
@ stdcall DdEntry31(ptr ptr) NtGdiDdGetDriverInfo
@ stdcall DdEntry32(ptr ptr long) NtGdiDdGetDxHandle
@ stdcall DdEntry33(ptr ptr) NtGdiDdGetFlipStatus
@ stdcall DdEntry34(ptr ptr) NtGdiDdGetInternalMoCompInfo
@ stdcall DdEntry35(ptr ptr) NtGdiDdGetMoCompBuffInfo
@ stdcall DdEntry36(ptr ptr) NtGdiDdGetMoCompGuids
@ stdcall DdEntry37(ptr ptr) NtGdiDdGetMoCompFormats
@ stdcall DdEntry38(ptr ptr) NtGdiDdGetScanLine
@ stdcall DdEntry39(ptr ptr ptr) NtGdiDdLock
@ stdcall DdEntry3(ptr) NtGdiD3dContextDestroyAll
@ stdcall DdEntry40(ptr ptr) NtGdiDdLockD3D
@ stdcall DdEntry41(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) NtGdiDdQueryDirectDrawObject
@ stdcall DdEntry42(ptr ptr) NtGdiDdQueryMoCompStatus
@ stdcall DdEntry43(ptr ptr) NtGdiDdReenableDirectDrawObject
@ stdcall DdEntry44(ptr) NtGdiDdReleaseDC
@ stdcall DdEntry45(ptr ptr) NtGdiDdRenderMoComp
@ stdcall DdEntry46(ptr ptr) NtGdiDdResetVisrgn
@ stdcall DdEntry47(ptr ptr) NtGdiDdSetColorKey
@ stdcall DdEntry48(ptr ptr) NtGdiDdSetExclusiveMode
@ stdcall DdEntry49(ptr ptr ptr) NtGdiDdSetGammaRamp
@ stdcall DdEntry4(ptr) NtGdiD3dValidateTextureStageState
@ stdcall DdEntry50(ptr ptr long)NtGdiDdCreateSurfaceEx
@ stdcall DdEntry51(ptr ptr ptr) NtGdiDdSetOverlayPosition
@ stdcall DdEntry52(ptr ptr) NtGdiDdUnattachSurface
@ stdcall DdEntry53(ptr ptr) NtGdiDdUnlock
@ stdcall DdEntry54(ptr ptr) NtGdiDdUnlockD3D
@ stdcall DdEntry55(ptr ptr long) NtGdiDdUpdateOverlay
@ stdcall DdEntry56(ptr ptr) NtGdiDdWaitForVerticalBlank
@ stdcall DdEntry5(ptr ptr ptr ptr ptr ptr ptr) NtGdiD3dDrawPrimitives2
@ stdcall DdEntry6(ptr) NtGdiDdGetDriverState
@ stdcall DdEntry7(ptr ptr ptr) NtGdiDdAddAttachedSurface
@ stdcall DdEntry8(ptr ptr ptr) NtGdiDdAlphaBlt
@ stdcall DdEntry9(ptr ptr) NtGdiDdAttachSurface
@ stdcall DeleteColorSpace(long) NtGdiDeleteColorSpace
@ stdcall DeleteDC(long)
@ stdcall DeleteEnhMetaFile(long)
@ stdcall DeleteMetaFile(long)
@ stdcall DeleteObject(long)
@ stdcall DescribePixelFormat(long long long ptr)
@ stub DeviceCapabilitiesEx
@ stdcall DeviceCapabilitiesExA(str str long str ptr)
@ stdcall DeviceCapabilitiesExW(wstr wstr long wstr ptr)
@ stdcall DPtoLP(long ptr long)
@ stdcall DrawEscape(long long long ptr)
@ stdcall Ellipse(long long long long long)
@ stdcall EnableEUDC(long) NtGdiEnableEudc
@ stdcall EndDoc(long)
@ stdcall EndFormPage(ptr)
@ stdcall EndPage(long)
@ stdcall EndPath(long)
@ stdcall EngAcquireSemaphore(ptr)
@ stdcall EngAlphaBlend(ptr ptr ptr ptr ptr ptr ptr) NtGdiEngAlphaBlend
@ stdcall EngAssociateSurface(ptr ptr long) NtGdiEngAssociateSurface
@ stdcall EngBitBlt(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) NtGdiEngBitBlt
@ stdcall EngCheckAbort(ptr) NtGdiEngCheckAbort
@ stdcall EngComputeGlyphSet(ptr ptr ptr)
@ stdcall EngCopyBits(ptr ptr ptr ptr ptr ptr) NtGdiEngCopyBits
@ stdcall EngCreateBitmap(ptr long long ptr ptr) NtGdiEngCreateBitmap
@ stdcall EngCreateClip() NtGdiEngCreateClip
@ stdcall EngCreateDeviceBitmap(ptr ptr long) NtGdiEngCreateDeviceBitmap
@ stdcall EngCreateDeviceSurface(ptr ptr long) NtGdiEngCreateDeviceSurface
@ stdcall EngCreatePalette(long long ptr long long long) NtGdiEngCreatePalette
@ stdcall EngCreateSemaphore()
@ stdcall EngDeleteClip(ptr) NtGdiEngDeleteClip
@ stdcall EngDeletePalette(ptr) NtGdiEngDeletePalette
@ stdcall EngDeletePath(ptr) NtGdiEngDeletePath
@ stdcall EngDeleteSemaphore(ptr)
@ stdcall EngDeleteSurface(ptr) NtGdiEngDeleteSurface
@ stdcall EngEraseSurface(ptr ptr long) NtGdiEngEraseSurface
@ stdcall EngFillPath(ptr ptr ptr ptr ptr ptr ptr) NtGdiEngFillPath
@ stdcall EngFindResource(ptr long long ptr)
@ stdcall EngFreeModule(ptr)
@ stdcall EngGetCurrentCodePage(ptr ptr)
@ stdcall EngGetDriverName(ptr)
@ stdcall EngGetPrinterDataFileName(ptr)
@ stdcall EngGradientFill(ptr ptr ptr ptr long ptr long ptr ptr long) NtGdiEngGradientFill
@ stdcall EngLineTo(ptr ptr ptr long long long long ptr ptr) NtGdiEngLineTo
@ stdcall EngLoadModule(ptr)
@ stdcall EngLockSurface(ptr) NtGdiEngLockSurface
@ stdcall EngMarkBandingSurface(ptr) NtGdiEngMarkBandingSurface
@ stdcall EngMultiByteToUnicodeN(wstr long ptr str long) RtlMultiByteToUnicodeN
@ stdcall EngMultiByteToWideChar(long wstr long str long)
@ stdcall EngPaint(ptr ptr ptr ptr ptr) NtGdiEngPaint
@ stdcall EngPlgBlt(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr long) NtGdiEngPlgBlt
@ stdcall EngQueryEMFInfo(ptr ptr)
@ stdcall EngQueryLocalTime(ptr)
@ stdcall EngReleaseSemaphore(ptr)
@ stdcall EngStretchBlt(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr long) NtGdiEngStretchBlt
@ stdcall EngStretchBltROP(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr long ptr long) NtGdiEngStretchBltROP
@ stdcall EngStrokeAndFillPath(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) NtGdiEngStrokeAndFillPath
@ stdcall EngStrokePath(ptr ptr ptr ptr ptr ptr ptr ptr) NtGdiEngStrokePath
@ stdcall EngTextOut(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) NtGdiEngTextOut
@ stdcall EngTransparentBlt(ptr ptr ptr ptr ptr ptr long long) NtGdiEngTransparentBlt
@ stdcall EngUnicodeToMultiByteN(str long long wstr long) RtlUnicodeToMultiByteN
@ stdcall EngUnlockSurface(ptr) NtGdiEngUnlockSurface
@ stdcall EngWideCharToMultiByte(long wstr long str long)
@ stdcall EnumEnhMetaFile(long long ptr ptr ptr)
@ stdcall EnumFontFamiliesA(long str ptr long)
@ stdcall EnumFontFamiliesExA(long ptr ptr long long)
@ stdcall EnumFontFamiliesExW(long ptr ptr long long)
@ stdcall EnumFontFamiliesW(long wstr ptr long)
@ stdcall EnumFontsA(long str ptr long)
@ stdcall EnumFontsW(long wstr ptr long)
@ stdcall EnumICMProfilesA(long ptr long)
@ stdcall EnumICMProfilesW(long ptr long)
@ stdcall EnumMetaFile(long long ptr ptr)
@ stdcall EnumObjects(long long ptr long)
@ stdcall EqualRgn(long long) NtGdiEqualRgn
@ stdcall Escape(long long long ptr ptr)
@ stdcall EudcLoadLinkW(wstr wstr long long)
@ stdcall EudcUnloadLinkW(wstr wstr)
@ stdcall ExcludeClipRect(long long long long long)
@ stdcall ExtCreatePen(long long ptr long ptr)
@ stdcall ExtCreateRegion(ptr long ptr)
@ stdcall ExtEscape(long long long ptr long ptr)
@ stdcall ExtFloodFill(long long long long long)
@ stdcall ExtSelectClipRgn(long long long)
@ stdcall ExtTextOutA(long long long long ptr str long ptr)
@ stdcall ExtTextOutW(long long long long ptr wstr long ptr)
@ stdcall FillPath(long)
@ stdcall FillRgn(long long long)
@ stdcall FixBrushOrgEx(long long long ptr)
@ stdcall FlattenPath(long)
@ stdcall FloodFill(long long long long)
@ stdcall FontIsLinked(long) NtGdiFontIsLinked
@ stdcall FONTOBJ_cGetAllGlyphHandles(ptr ptr) NtGdiFONTOBJ_cGetAllGlyphHandles
@ stdcall FONTOBJ_cGetGlyphs(ptr long long ptr ptr) NtGdiFONTOBJ_cGetGlyphs
@ stdcall FONTOBJ_pfdg(ptr) NtGdiFONTOBJ_pfdg
@ stdcall FONTOBJ_pifi(ptr) NtGdiFONTOBJ_pifi
@ stdcall FONTOBJ_pQueryGlyphAttrs(ptr long) NtGdiFONTOBJ_pQueryGlyphAttrs
@ stdcall FONTOBJ_pvTrueTypeFontFile(ptr ptr) NtGdiFONTOBJ_pvTrueTypeFontFile
@ stdcall FONTOBJ_pxoGetXform(ptr) NtGdiFONTOBJ_pxoGetXform
@ stdcall FONTOBJ_vGetInfo(ptr long ptr) NtGdiFONTOBJ_vGetInfo
@ stdcall FrameRgn(long long long long long)
@ stub FreeImageColorMatcher
@ stdcall GdiAddFontResourceW(ptr ptr ptr)
@ stdcall GdiAddGlsBounds(ptr ptr)
@ stdcall GdiAddGlsRecord(ptr long ptr ptr)
@ stdcall GdiAlphaBlend(long long long long long long long long long long long)
@ stdcall GdiArtificialDecrementDriver(wstr long)
@ stub GdiAssociateObject
@ stdcall GdiCleanCacheDC(ptr)
@ stdcall GdiComment(long long ptr)
@ stdcall GdiConsoleTextOut(ptr ptr long ptr) NtGdiConsoleTextOut
@ stub GdiConvertAndCheckDC
@ stub GdiConvertBitmap
@ stdcall GdiConvertBitmapV5(ptr ptr long long)
@ stdcall GdiConvertBrush(ptr)
@ stdcall GdiConvertDC(ptr)
@ stdcall GdiConvertEnhMetaFile(ptr)
@ stdcall GdiConvertFont(ptr)
@ stdcall GdiConvertMetaFilePict(ptr)
@ stdcall GdiConvertPalette(ptr)
@ stdcall GdiConvertRegion(ptr)
@ stdcall GdiConvertToDevmodeW(ptr)
@ stub GdiCreateLocalBitmap
@ stub GdiCreateLocalBrush
@ stub GdiCreateLocalEnhMetaFile
@ stub GdiCreateLocalFont
@ stub GdiCreateLocalMetaFilePict
@ stub GdiCreateLocalPalette
@ stub GdiCreateLocalRegion
@ stub GdiDciBeginAccess
@ stub GdiDciCreateOffscreenSurface
@ stub GdiDciCreateOverlaySurface
@ stub GdiDciCreatePrimarySurface
@ stub GdiDciDestroySurface
@ stub GdiDciDrawSurface
@ stub GdiDciEndAccess
@ stub GdiDciEnumSurface
@ stub GdiDciInitialize
@ stub GdiDciSetClipList
@ stub GdiDciSetDestination
@ stub GdiDeleteLocalDC
@ stub GdiDeleteLocalObject
@ stdcall GdiDeleteSpoolFileHandle(ptr)
@ stdcall GdiDescribePixelFormat(ptr long long ptr) NtGdiDescribePixelFormat
@ stdcall GdiDllInitialize(ptr long ptr)
@ stdcall GdiDrawStream(ptr long ptr)
@ stdcall GdiEndDocEMF(ptr)
@ stdcall GdiEndPageEMF(ptr long)
@ stdcall GdiEntry1(ptr ptr) gdi32.DdCreateDirectDrawObject
@ stdcall GdiEntry10(ptr long) gdi32.DdReenableDirectDrawObject
@ stdcall GdiEntry11(ptr ptr) gdi32.DdAttachSurface
@ stdcall GdiEntry12(ptr ptr) gdi32.DdUnattachSurface
@ stdcall GdiEntry13() gdi32.DdQueryDisplaySettingsUniqueness
@ stdcall GdiEntry14(ptr ptr long) gdi32.DdGetDxHandle
@ stdcall GdiEntry15(ptr ptr ptr) gdi32.DdSetGammaRamp
@ stub GdiEntry16
@ stdcall GdiEntry2(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) gdi32.DdQueryDirectDrawObject
@ stdcall GdiEntry3(ptr) gdi32.DdDeleteDirectDrawObject
@ stdcall GdiEntry4(ptr long) gdi32.DdCreateSurfaceObject
@ stdcall GdiEntry5(ptr) gdi32.DdDeleteSurfaceObject
@ stdcall GdiEntry6(ptr ptr) gdi32.DdResetVisrgn
@ stdcall GdiEntry7(ptr ptr) gdi32.DdGetDC
@ stdcall GdiEntry8(ptr) gdi32.DdReleaseDC
@ stdcall GdiEntry9(ptr ptr ptr ptr ptr ptr) gdi32.DdCreateDIBSection
@ stdcall GdiFixUpHandle(ptr)
@ stdcall GdiFlush()
@ stdcall GdiFullscreenControl(ptr ptr long ptr ptr) NtGdiFullscreenControl 
@ stdcall GdiGetBatchLimit()
@ stdcall GdiGetCharDimensions(long ptr ptr)
@ stdcall GdiGetCodePage(long)
@ stdcall GdiGetDC(ptr)
@ stdcall GdiGetDevmodeForPage(ptr long ptr ptr)
@ stub GdiGetLocalBitmap
@ stub GdiGetLocalBrush
@ stub GdiGetLocalDC
@ stub GdiGetLocalFont
@ stdcall GdiGetPageCount(ptr)
@ stdcall GdiGetPageHandle(ptr long ptr)
@ stdcall GdiGetSpoolFileHandle(wstr ptr wstr)
@ stdcall GdiGetSpoolMessage(ptr long ptr long) NtGdiGetSpoolMessage
@ stdcall GdiGradientFill(long ptr long ptr long long) 
@ stdcall GdiInitializeLanguagePack(long)
@ stdcall GdiInitSpool() NtGdiInitSpool
@ stdcall GdiIsMetaFileDC(long)
@ stdcall GdiIsMetaPrintDC(long)
@ stdcall GdiIsPlayMetafileDC(long)
@ stdcall GdiPlayDCScript(long long long long long long)
@ stdcall GdiPlayEMF(wstr ptr wstr ptr ptr)
@ stdcall GdiPlayJournal(long long long long long)
@ stdcall GdiPlayPageEMF(ptr ptr ptr ptr ptr)
@ stdcall GdiPlayPrivatePageEMF(ptr long ptr)
@ stdcall GdiPlayScript(ptr ptr ptr ptr ptr ptr ptr)
@ stdcall GdiPlaySpoolStream(long long long long long long)
@ stdcall GdiPrinterThunk(ptr ptr long)
@ stdcall GdiProcessSetup()
@ stdcall GdiQueryFonts(ptr long ptr) NtGdiQueryFonts
@ stdcall GdiQueryTable()
@ stdcall GdiRealizationInfo(long ptr)
@ stdcall GdiReleaseDC(ptr)
@ stdcall GdiReleaseLocalDC(ptr)
@ stdcall GdiResetDCEMF(ptr ptr)
@ stdcall GdiSetAttrs(ptr)
@ stdcall GdiSetBatchLimit(long)
@ stdcall GdiSetLastError(long)
@ stdcall GdiSetPixelFormat(ptr long) NtGdiSetPixelFormat
@ stdcall GdiSetServerAttr(ptr long)
@ stdcall GdiStartDocEMF(ptr ptr)
@ stdcall GdiStartPageEMF(ptr)
@ stub GdiSwapBuffers
@ stdcall GdiTransparentBlt(long long long long long long long long long long long)
@ stdcall GdiValidateHandle(ptr)
@ stub GdiWinWatchClose
@ stub GdiWinWatchDidStatusChange
@ stub GdiWinWatchGetClipList
@ stub GdiWinWatchOpen
@ stdcall GetArcDirection(long)
@ stdcall GetAspectRatioFilterEx(long ptr)
@ stdcall GetBitmapAttributes(ptr)
@ stdcall GetBitmapBits(long long ptr) NtGdiGetBitmapBits
@ stdcall GetBitmapDimensionEx(long ptr) NtGdiGetBitmapDimension
@ stdcall GetBkColor(long)
@ stdcall GetBkMode(long)
@ stdcall GetBoundsRect(long ptr long)
@ stdcall GetBrushAttributes(ptr)
@ stdcall GetBrushOrgEx(long ptr)
@ stdcall GetCharABCWidthsA(long long long ptr)
@ stdcall GetCharABCWidthsFloatA(long long long ptr)
@ stdcall GetCharABCWidthsFloatW(long long long ptr)
@ stdcall GetCharABCWidthsI(long long long ptr ptr)
@ stdcall GetCharABCWidthsW(long long long ptr)
@ stdcall GetCharacterPlacementA(long str long long ptr long)
@ stdcall GetCharacterPlacementW(long wstr long long ptr long)
@ stdcall GetCharWidth32A(long long long long)
@ stdcall GetCharWidth32W(long long long long)
@ stdcall GetCharWidthA(long long long long) GetCharWidth32A
@ stdcall GetCharWidthFloatA(long long long ptr)
@ stdcall GetCharWidthFloatW(long long long ptr)
@ stdcall GetCharWidthI(ptr long long ptr ptr)
@ stdcall GetCharWidthInfo(ptr ptr) NtGdiGetCharWidthInfo
@ stdcall GetCharWidthW(long long long long)
@ stub GetCharWidthWOW
@ stdcall GetClipBox(long ptr)
@ stdcall GetClipRgn(long long)
@ stdcall GetColorAdjustment(long ptr) NtGdiGetColorAdjustment
@ stdcall GetColorSpace(long)
@ stdcall GetCurrentObject(long long)
@ stdcall GetCurrentPositionEx(long ptr)
@ stdcall GetDCBrushColor(long)
@ stdcall GetDCOrgEx(long ptr)
@ stdcall GetDCPenColor(long)
@ stdcall GetDeviceCaps(long long)
@ stdcall GetDeviceGammaRamp(long ptr)
@ stdcall GetDIBColorTable(long long long ptr)
@ stdcall GetDIBits(long long long long ptr ptr long)
@ stdcall GetEnhMetaFileA(str)
@ stdcall GetEnhMetaFileBits(long long ptr)
@ stdcall GetEnhMetaFileDescriptionA(long long ptr)
@ stdcall GetEnhMetaFileDescriptionW(long long ptr)
@ stdcall GetEnhMetaFileHeader(long long ptr)
@ stdcall GetEnhMetaFilePaletteEntries (long long ptr)
@ stdcall GetEnhMetaFilePixelFormat(ptr long ptr)
@ stdcall GetEnhMetaFileW(wstr)
@ stub GetETM
# @ stub GetEUDCTimeStamp
# @ stub GetEUDCTimeStampExW
# @ stub GetFontAssocStatus
@ stdcall GetFontData(long long long ptr long)
@ stdcall GetFontLanguageInfo(long)
@ stub GetFontResourceInfo
@ stub GetFontResourceInfoW
@ stdcall GetFontUnicodeRanges(ptr ptr) NtGdiGetFontUnicodeRanges
@ stdcall GetGlyphIndicesA(long ptr long ptr long)
@ stdcall GetGlyphIndicesW(long ptr long ptr long) NtGdiGetGlyphIndicesW
@ stdcall GetGlyphOutline(long long long ptr long ptr ptr) GetGlyphOutlineA
@ stdcall GetGlyphOutlineA(long long long ptr long ptr ptr)
@ stdcall GetGlyphOutlineW(long long long ptr long ptr ptr)
@ stdcall GetGlyphOutlineWow(long long long long long long long)
@ stdcall GetGraphicsMode(long)
@ stdcall GetHFONT(ptr)
@ stdcall GetICMProfileA(long ptr ptr)
@ stdcall GetICMProfileW(long ptr ptr)
@ stdcall GetKerningPairs(long long ptr) GetKerningPairsA
@ stdcall GetKerningPairsA(long long ptr)
@ stdcall GetKerningPairsW(long long ptr)
@ stdcall GetLayout(long)
@ stdcall GetLogColorSpaceA(long ptr long)
@ stdcall GetLogColorSpaceW(long ptr long)
@ stdcall GetMapMode(long)
@ stdcall GetMetaFileA(str)
@ stdcall GetMetaFileBitsEx(long long ptr)
@ stdcall GetMetaFileW(wstr)
@ stdcall GetMetaRgn(long long)
@ stdcall GetMiterLimit(long ptr) NtGdiGetMiterLimit
@ stdcall GetNearestColor(long long) NtGdiGetNearestColor
@ stdcall GetNearestPaletteIndex(long long) NtGdiGetNearestPaletteIndex
@ stdcall GetObjectA(long long ptr)
@ stdcall GetObjectType(long)
@ stdcall GetObjectW(long long ptr)
@ stdcall GetOutlineTextMetricsA(long long ptr)
@ stdcall GetOutlineTextMetricsW(long long ptr)
@ stdcall GetPaletteEntries(long long long ptr)
@ stdcall GetPath(long ptr ptr long)
@ stdcall GetPixel(long long long)
@ stdcall GetPixelFormat(long)
@ stdcall GetPolyFillMode(long)
@ stdcall GetRandomRgn(long long long) NtGdiGetRandomRgn
@ stdcall GetRasterizerCaps(ptr long) NtGdiGetRasterizerCaps
@ stdcall GetRegionData(long long ptr)
@ stdcall GetRelAbs(long long)
@ stdcall GetRgnBox(long ptr)
@ stdcall GetROP2(long)
@ stdcall GetStockObject(long)
@ stdcall GetStretchBltMode(long)
@ stdcall GetStringBitmapA(ptr str long long ptr)
@ stdcall GetStringBitmapW(ptr wstr long long ptr)
@ stdcall GetSystemPaletteEntries(long long long ptr)
@ stdcall GetSystemPaletteUse(long) NtGdiGetSystemPaletteUse
@ stdcall GetTextAlign(long)
@ stdcall GetTextCharacterExtra(long)
@ stdcall GetTextCharset(long)
@ stdcall GetTextCharsetInfo(long ptr long) NtGdiGetTextCharsetInfo
@ stdcall GetTextColor(long)
@ stdcall GetTextExtentExPointA(long str long long ptr ptr ptr)
@ stdcall GetTextExtentExPointI(long ptr long long ptr ptr ptr)
@ stdcall GetTextExtentExPointW(long wstr long long ptr ptr ptr)
@ stdcall GetTextExtentExPointWPri(ptr wstr long long long ptr ptr)
@ stdcall GetTextExtentPoint32A(long str long ptr)
@ stdcall GetTextExtentPoint32W(long wstr long ptr)
@ stdcall GetTextExtentPointA(long str long ptr)
@ stdcall GetTextExtentPointI(long ptr long ptr)
@ stdcall GetTextExtentPointW(long wstr long ptr)
@ stdcall GetTextFaceA(long long ptr)
@ stdcall GetTextFaceAliasW(ptr long wstr)
@ stdcall GetTextFaceW(long long ptr)
@ stdcall GetTextMetricsA(long ptr)
@ stdcall GetTextMetricsW(long ptr)
@ stdcall GetTransform(long long ptr) NtGdiGetTransform
@ stdcall GetViewportExtEx(long ptr)
@ stdcall GetViewportOrgEx(long ptr)
@ stdcall GetWindowExtEx(long ptr)
@ stdcall GetWindowOrgEx(long ptr)
@ stdcall GetWinMetaFileBits(long long ptr long long)
@ stdcall GetWorldTransform(long ptr)
@ stdcall HT_Get8BPPFormatPalette(ptr long long long) NtGdiHT_Get8BPPFormatPalette
@ stdcall HT_Get8BPPMaskPalette(ptr long long long long long) NtGdiHT_Get8BPPMaskPalette
@ stdcall IntersectClipRect(long long long long long)
@ stdcall InvertRgn(long long)
@ stdcall IsValidEnhMetaRecord(long long)
@ stdcall IsValidEnhMetaRecordOffExt(long long long long)
@ stdcall LineDDA(long long long long ptr long)
@ stdcall LineTo(long long long)
@ stdcall LPtoDP(long ptr long)
@ stub LoadImageColorMatcherA
@ stub LoadImageColorMatcherW
@ stdcall MaskBlt(long long long long long long long long long long long long)
@ stdcall MirrorRgn(ptr ptr)
@ stdcall ModifyWorldTransform(long ptr long)
@ stdcall MoveToEx(long long long ptr)
@ stdcall NamedEscape(ptr wstr long long str long str)
@ stdcall OffsetClipRgn(long long long)
@ stdcall OffsetRgn(long long long)
@ stdcall OffsetViewportOrgEx(long long long ptr)
@ stdcall OffsetWindowOrgEx(long long long ptr)
@ stdcall PaintRgn(long long)
@ stdcall PatBlt(long long long long long long)
@ stdcall PATHOBJ_bEnum(ptr ptr) NtGdiPATHOBJ_bEnum
@ stdcall PATHOBJ_bEnumClipLines(ptr long ptr) NtGdiPATHOBJ_bEnumClipLines
@ stdcall PATHOBJ_vEnumStart(ptr) NtGdiPATHOBJ_vEnumStart
@ stdcall PATHOBJ_vEnumStartClipLines(ptr ptr ptr ptr) NtGdiPATHOBJ_vEnumStartClipLines
@ stdcall PATHOBJ_vGetBounds(ptr ptr) NtGdiPATHOBJ_vGetBounds
@ stdcall PathToRegion(long)
@ stdcall Pie(long long long long long long long long long)
@ stdcall PlayEnhMetaFile(long long ptr)
@ stdcall PlayEnhMetaFileRecord(long ptr ptr long)
@ stdcall PlayMetaFile(long long)
@ stdcall PlayMetaFileRecord(long ptr ptr long)
@ stdcall PlgBlt(long ptr long long long long long long long long)
@ stdcall PolyBezier(long ptr long)
@ stdcall PolyBezierTo(long ptr long)
@ stdcall PolyDraw(long ptr ptr long)
@ stdcall Polygon(long ptr long)
@ stdcall Polyline(long ptr long)
@ stdcall PolylineTo(long ptr long)
@ stdcall PolyPatBlt(ptr long ptr long long)
@ stdcall PolyPolygon(long ptr ptr long)
@ stdcall PolyPolyline(long ptr ptr long)
@ stdcall PolyTextOutA(long ptr long)
@ stdcall PolyTextOutW(long ptr long)
@ stdcall PtInRegion(long long long)
@ stdcall PtVisible(long long long) NtGdiPtVisible
@ stdcall QueryFontAssocStatus()
@ stdcall RealizePalette(long)
@ stdcall Rectangle(long long long long long)
@ stdcall RectInRegion(long ptr)
@ stdcall RectVisible(long ptr) NtGdiRectVisible
@ stdcall RemoveFontMemResourceEx(ptr)
@ stdcall RemoveFontResourceA(str)
@ stdcall RemoveFontResourceExA(str long ptr)
@ stdcall RemoveFontResourceExW(wstr long ptr)
@ stdcall RemoveFontResourceTracking(ptr long)
@ stdcall RemoveFontResourceW(wstr)
@ stdcall ResetDCA(long ptr)
@ stdcall ResetDCW(long ptr)
@ stdcall ResizePalette(long long)
@ stdcall RestoreDC(long long)
@ stdcall RoundRect(long long long long long long long)
@ stdcall SaveDC(long)
@ stdcall ScaleViewportExtEx(long long long long long ptr)
@ stdcall ScaleWindowExtEx(long long long long long ptr)
@ stdcall SelectBrushLocal(ptr ptr)
@ stdcall SelectClipPath(long long)
@ stdcall SelectClipRgn(long long)
@ stdcall SelectFontLocal(ptr ptr)
@ stdcall SelectObject(long long)
@ stdcall SelectPalette(long long long)
@ stdcall SetAbortProc(long ptr)
@ stdcall SetArcDirection(long long)
@ stdcall SetBitmapAttributes(ptr long)
@ stdcall SetBitmapBits(long long ptr) NtGdiSetBitmapBits
@ stdcall SetBitmapDimensionEx(long long long ptr) NtGdiSetBitmapDimension
@ stdcall SetBkColor(long long)
@ stdcall SetBkMode(long long)
@ stdcall SetBoundsRect(long ptr long)
@ stdcall SetBrushAttributes(ptr long)
@ stdcall SetBrushOrgEx(long long long ptr)
@ stdcall SetColorAdjustment(long ptr)
@ stdcall SetColorSpace(long long)
@ stdcall SetDCBrushColor(long long)
@ stdcall SetDCPenColor(long long)
@ stdcall SetDeviceGammaRamp(long ptr)
@ stdcall SetDIBColorTable(long long long ptr)
@ stdcall SetDIBits(long long long long ptr ptr long)
@ stdcall SetDIBitsToDevice(long long long long long long long long long ptr ptr long)
@ stdcall SetEnhMetaFileBits(long ptr)
@ stdcall SetFontEnumeration(ptr) NtGdiSetFontEnumeration
@ stdcall SetGraphicsMode(long long)
@ stdcall SetICMMode(long long)
@ stdcall SetICMProfileA(long str)
@ stdcall SetICMProfileW(long wstr)
@ stdcall SetLayout(long long)
@ stdcall SetLayoutWidth(ptr long long)
@ stdcall SetMagicColors(ptr long long) NtGdiSetMagicColors
@ stdcall SetMapMode(long long)
@ stdcall SetMapperFlags(long long)
@ stdcall SetMetaFileBitsEx(long ptr)
@ stdcall SetMetaRgn(long)
@ stdcall SetMiterLimit(long long ptr)
#@ stdcall SetObjectOwner(long long)
@ stdcall SetPaletteEntries(long long long ptr)
@ stdcall SetPixel(long long long long)
@ stdcall SetPixelFormat(long long ptr)
@ stdcall SetPixelV(long long long long)
@ stdcall SetPolyFillMode(long long)
@ stdcall SetRectRgn(long long long long long)
@ stdcall SetRelAbs(long long)
@ stdcall SetROP2(long long)
@ stdcall SetStretchBltMode(long long)
@ stdcall SetSystemPaletteUse(long long) NtGdiSetSystemPaletteUse
@ stdcall SetTextAlign(long long)
@ stdcall SetTextCharacterExtra(long long)
@ stdcall SetTextColor(long long)
@ stdcall SetTextJustification(long long long)
@ stdcall SetViewportExtEx(long long long ptr)
@ stdcall SetViewportOrgEx(long long long ptr)
@ stdcall SetVirtualResolution(long long long long long) NtGdiSetVirtualResolution
@ stdcall SetWindowExtEx(long long long ptr)
@ stdcall SetWindowOrgEx(long long long ptr)
@ stdcall SetWinMetaFileBits(long ptr long ptr)
@ stdcall SetWorldTransform(long ptr)
@ stdcall StartDocA(long ptr)
@ stdcall StartDocW(long ptr)
@ stdcall StartFormPage(ptr)
@ stdcall StartPage(long)
@ stdcall StretchBlt(long long long long long long long long long long long)
@ stdcall StretchDIBits(long long long long long long long long long ptr ptr long long)
@ stdcall STROBJ_bEnum(ptr ptr ptr) NtGdiSTROBJ_bEnum
@ stdcall STROBJ_bEnumPositionsOnly(ptr ptr ptr) NtGdiSTROBJ_bEnumPositionsOnly
@ stdcall STROBJ_bGetAdvanceWidths(ptr long long ptr) NtGdiSTROBJ_bGetAdvanceWidths
@ stdcall STROBJ_dwGetCodePage(ptr) NtGdiSTROBJ_dwGetCodePage
@ stdcall STROBJ_vEnumStart(ptr) NtGdiSTROBJ_vEnumStart
@ stdcall StrokeAndFillPath(long)
@ stdcall StrokePath(long)
@ stdcall SwapBuffers(long)
@ stdcall TextOutA(long long long str long)
@ stdcall TextOutW(long long long wstr long)
@ stdcall TranslateCharsetInfo(ptr ptr long)
@ stdcall UnloadNetworkFonts(long)
@ stdcall UnrealizeObject(long)
@ stdcall UpdateColors(long)
@ stdcall UpdateICMRegKey(long str str long) UpdateICMRegKeyA
@ stdcall UpdateICMRegKeyA(long str str long)
@ stdcall UpdateICMRegKeyW(long wstr wstr long)
@ stdcall WidenPath(long)
@ stdcall XFORMOBJ_bApplyXform(ptr long long ptr ptr) NtGdiXFORMOBJ_bApplyXform
@ stdcall XFORMOBJ_iGetXform(ptr ptr) NtGdiXFORMOBJ_iGetXform
@ stdcall XLATEOBJ_cGetPalette(ptr long long ptr) NtGdiXLATEOBJ_cGetPalette
@ stdcall XLATEOBJ_hGetColorTransform(ptr) NtGdiXLATEOBJ_hGetColorTransform
@ stdcall XLATEOBJ_iXlate(ptr long)
@ stdcall XLATEOBJ_piVector(ptr)
#@ extern pfnRealizePalette
#@ extern pfnSelectPalette
#@ stub pstackConnect
