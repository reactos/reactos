@ stdcall BRUSHOBJ_hGetColorTransform(ptr)
@ stdcall BRUSHOBJ_pvAllocRbrush(ptr long)
@ stdcall BRUSHOBJ_pvGetRbrush(ptr)
@ stdcall BRUSHOBJ_ulGetBrushColor(ptr)
@ stdcall CLIPOBJ_bEnum(ptr long ptr)
@ stdcall CLIPOBJ_cEnumStart(ptr long long long long)
@ stdcall CLIPOBJ_ppoGetPath(ptr)
@ stdcall EngAcquireSemaphore(ptr)
@ stdcall EngAllocMem(long long long)
@ stdcall EngAllocPrivateUserMem(ptr ptr long)
@ stdcall EngAllocSectionMem(ptr long ptr long)
@ stdcall EngAllocUserMem(ptr long)
@ stdcall EngAlphaBlend(ptr ptr ptr ptr ptr ptr ptr)
@ stdcall EngAssociateSurface(ptr ptr long)
@ stdcall EngBitBlt(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr long)
@ stdcall EngBugCheckEx(long ptr ptr ptr ptr) NTOSKRNL.KeBugCheckEx
@ stdcall EngCheckAbort(ptr)
@ stdcall EngClearEvent(ptr)
@ stdcall EngComputeGlyphSet(long long long)
@ stdcall EngControlSprites(ptr long)
@ stdcall EngCopyBits(ptr ptr ptr ptr ptr ptr)
@ stdcall EngCreateBitmap(long long long long long ptr)
@ stdcall EngCreateClip()
@ stdcall EngCreateDeviceBitmap(ptr long long long)
@ stdcall EngCreateDeviceSurface(ptr long long long)
@ stdcall EngCreateDriverObj(ptr ptr ptr)
@ stdcall EngCreateEvent(ptr)
@ stdcall EngCreatePalette(long long long long long long)
@ stdcall EngCreatePath()
@ stdcall EngCreateSemaphore()
@ stdcall EngCreateWnd(ptr ptr ptr long long)
@ stdcall EngDebugBreak() NTOSKRNL.DbgBreakPoint
@ stdcall EngDebugPrint(str str ptr)
@ stdcall EngDeleteClip(ptr)
@ stdcall EngDeleteDriverObj(ptr long long)
@ stdcall EngDeleteEvent(ptr)
@ stdcall EngDeleteFile(ptr)
@ stdcall EngDeletePalette(ptr)
@ stdcall EngDeletePath(ptr)
@ stdcall EngDeleteSafeSemaphore(ptr)
@ stdcall EngDeleteSemaphore(ptr)
@ stdcall EngDeleteSurface(ptr)
@ stdcall EngDeleteWnd(ptr)
@ stdcall EngDeviceIoControl(ptr long ptr long ptr long ptr)
@ stdcall EngDitherColor(ptr long long long)
@ stdcall EngDxIoctl(long ptr long)
@ stdcall EngEnumForms(ptr long ptr long ptr ptr)
@ stdcall EngEraseSurface(ptr ptr long)
@ stdcall EngFileIoControl(ptr long ptr ptr ptr ptr ptr)
@ stdcall EngFileWrite(ptr ptr ptr ptr)
@ stdcall EngFillPath(ptr ptr ptr ptr ptr long long)
@ stdcall EngFindImageProcAddress(ptr ptr)
@ stdcall EngFindResource(ptr long long ptr)
@ stdcall EngFntCacheAlloc(long long)
@ stdcall EngFntCacheFault(long long)
@ stdcall EngFntCacheLookUp(long ptr)
@ stdcall EngFreeMem(ptr)
@ stdcall EngFreeModule(ptr)
@ stdcall EngFreePrivateUserMem(ptr ptr)
@ stdcall EngFreeSectionMem(ptr ptr)
@ stdcall EngFreeUserMem(ptr)
@ stdcall EngGetCurrentCodePage(ptr ptr)
@ stdcall EngGetCurrentProcessId() NTOSKRNL.PsGetCurrentProcessId
@ stdcall EngGetCurrentThreadId() NTOSKRNL.PsGetCurrentThreadId
@ stdcall EngGetDriverName(ptr)
@ stdcall EngGetFileChangeTime(ptr ptr)
@ stdcall EngGetFilePath(ptr ptr)
@ stdcall EngGetForm(ptr ptr long ptr long ptr)
@ stdcall EngGetLastError()
@ stdcall EngGetPrinter(ptr long ptr long ptr)
@ stdcall EngGetPrinterData(ptr ptr ptr ptr long ptr)
@ stdcall EngGetPrinterDataFileName(ptr)
@ stdcall EngGetPrinterDriver(ptr ptr long ptr long ptr)
@ stdcall EngGetProcessHandle()
@ stdcall EngGetTickCount()
@ stdcall EngGetType1FontList(ptr ptr long ptr ptr ptr)
@ stdcall EngGradientFill(ptr ptr ptr ptr long ptr long ptr ptr long)
@ stdcall EngHangNotification(ptr ptr)
@ stdcall EngInitializeSafeSemaphore(ptr)
@ stdcall EngIsSemaphoreOwned(ptr)
@ stdcall EngIsSemaphoreOwnedByCurrentThread(ptr)
@ stdcall EngLineTo(ptr ptr ptr long long long long ptr long)
@ stdcall EngLoadImage(ptr)
@ stdcall EngLoadModule(ptr)
@ stdcall EngLoadModuleForWrite(ptr long)
@ stdcall EngLockDirectDrawSurface(ptr)
@ stdcall EngLockDriverObj(ptr)
@ stdcall EngLockSurface(ptr)
@ stdcall EngLpkInstalled()
@ stdcall EngMapEvent(ptr ptr ptr ptr ptr)
@ stdcall EngMapFile(ptr long ptr)
@ stdcall EngMapFontFile(ptr ptr ptr)
@ stdcall EngMapFontFileFD(ptr ptr ptr)
@ stdcall EngMapModule(ptr ptr)
@ stdcall EngMapSection(ptr long ptr ptr)
@ stdcall EngMarkBandingSurface(ptr)
@ stdcall EngModifySurface(ptr ptr long long ptr ptr long ptr)
@ stdcall EngMovePointer(ptr long long ptr)
@ stdcall EngMulDiv(long long long)
@ stdcall EngMultiByteToUnicodeN(ptr long ptr ptr long) NTOSKRNL.RtlMultiByteToUnicodeN
@ stdcall EngMultiByteToWideChar(long ptr long ptr long)
@ stdcall EngNineGrid(ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall EngPaint(ptr ptr ptr ptr long)
@ stdcall EngPlgBlt(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr long)
@ stdcall EngProbeForRead(ptr long long) NTOSKRNL.ProbeForRead
@ stdcall EngProbeForReadAndWrite(ptr long long) NTOSKRNL.ProbeForWrite
@ stdcall EngQueryDeviceAttribute(ptr long ptr long ptr long)
@ stdcall EngQueryFileTimeStamp(ptr)
@ stdcall EngQueryLocalTime(ptr)
@ stdcall EngQueryPalette(ptr ptr long ptr)
@ stdcall EngQueryPerformanceCounter(ptr)
@ stdcall EngQueryPerformanceFrequency(ptr)
@ stdcall EngQuerySystemAttribute(long ptr)
@ stdcall EngReadStateEvent(ptr)
@ stdcall EngReleaseSemaphore(ptr)
@ stdcall EngRestoreFloatingPointState(ptr)
@ stdcall EngSaveFloatingPointState(ptr long)
@ stdcall EngSecureMem(ptr long)
@ stdcall EngSetEvent(ptr)
@ stdcall EngSetLastError(long)
@ stdcall EngSetPointerShape(ptr ptr ptr ptr long long long long ptr long)
@ stdcall EngSetPointerTag(ptr ptr ptr ptr long)
@ stdcall EngSetPrinterData(ptr ptr long ptr long)
@ stdcall EngSort(ptr long long ptr)
@ stdcall EngStretchBlt(ptr ptr ptr ptr ptr long long ptr ptr ptr long)
@ stdcall EngStretchBltROP(ptr ptr ptr ptr ptr long long ptr ptr ptr long ptr long)
@ stdcall EngStrokeAndFillPath(ptr ptr ptr ptr ptr ptr ptr ptr long long)
@ stdcall EngStrokePath(ptr ptr ptr ptr ptr ptr ptr long)
@ stdcall EngTextOut(ptr ptr ptr ptr ptr ptr ptr ptr ptr long)
@ stdcall EngTransparentBlt(ptr ptr ptr ptr ptr ptr long long)
@ stdcall EngUnicodeToMultiByteN(ptr long ptr ptr long) NTOSKRNL.RtlUnicodeToMultiByteN
@ stdcall EngUnloadImage(ptr)
@ stdcall EngUnlockDirectDrawSurface(ptr)
@ stdcall EngUnlockDriverObj(ptr)
@ stdcall EngUnlockSurface(ptr)
@ stdcall EngUnmapEvent(ptr)
@ stdcall EngUnmapFile(ptr)
@ stdcall EngUnmapFontFile(ptr)
@ stdcall EngUnmapFontFileFD(ptr)
@ stdcall EngUnsecureMem(ptr)
@ stdcall EngWaitForSingleObject(ptr ptr)
@ stdcall EngWideCharToMultiByte(long ptr long ptr long)
@ stdcall EngWritePrinter(ptr ptr long ptr)
@ stdcall -arch=i386 FLOATOBJ_Add(ptr ptr)
@ stdcall -arch=i386 FLOATOBJ_AddFloat(ptr long)
@ stdcall -arch=i386 FLOATOBJ_AddFloatObj(ptr ptr) FLOATOBJ_Add
@ stdcall -arch=i386 FLOATOBJ_AddLong(ptr long)
@ stdcall -arch=i386 FLOATOBJ_Div(ptr ptr)
@ stdcall -arch=i386 FLOATOBJ_DivFloat(ptr long)
@ stdcall -arch=i386 FLOATOBJ_DivFloatObj(ptr ptr) FLOATOBJ_Div
@ stdcall -arch=i386 FLOATOBJ_DivLong(ptr long)
@ stdcall -arch=i386 FLOATOBJ_Equal(ptr ptr)
@ stdcall -arch=i386 FLOATOBJ_EqualLong(ptr long)
@ stdcall -arch=i386 FLOATOBJ_GetFloat(ptr)
@ stdcall -arch=i386 FLOATOBJ_GetLong(ptr)
@ stdcall -arch=i386 FLOATOBJ_GreaterThan(ptr ptr)
@ stdcall -arch=i386 FLOATOBJ_GreaterThanLong(ptr long)
@ stdcall -arch=i386 FLOATOBJ_LessThan(ptr ptr)
@ stdcall -arch=i386 FLOATOBJ_LessThanLong(ptr long)
@ stdcall -arch=i386 FLOATOBJ_Mul(ptr ptr)
@ stdcall -arch=i386 FLOATOBJ_MulFloat(ptr long)
@ stdcall -arch=i386 FLOATOBJ_MulFloatObj(ptr ptr) FLOATOBJ_Mul
@ stdcall -arch=i386 FLOATOBJ_MulLong(ptr long)
@ stdcall -arch=i386 FLOATOBJ_Neg(ptr)
@ stdcall -arch=i386 FLOATOBJ_SetFloat(ptr long)
@ stdcall -arch=i386 FLOATOBJ_SetLong(ptr long)
@ stdcall -arch=i386 FLOATOBJ_Sub(ptr ptr)
@ stdcall -arch=i386 FLOATOBJ_SubFloat(ptr long)
@ stdcall -arch=i386 FLOATOBJ_SubFloatObj(ptr ptr) FLOATOBJ_Sub
@ stdcall -arch=i386 FLOATOBJ_SubLong(ptr long)
@ stdcall FONTOBJ_cGetAllGlyphHandles(ptr ptr)
@ stdcall FONTOBJ_cGetGlyphs(ptr long long ptr ptr)
@ stdcall FONTOBJ_pQueryGlyphAttrs(ptr long)
@ stdcall FONTOBJ_pfdg(ptr)
@ stdcall FONTOBJ_pifi(ptr)
@ stdcall FONTOBJ_pjOpenTypeTablePointer(ptr long ptr)
@ stdcall FONTOBJ_pvTrueTypeFontFile(ptr ptr)
@ stdcall FONTOBJ_pwszFontFilePaths(ptr ptr)
@ stdcall FONTOBJ_pxoGetXform(ptr)
@ stdcall FONTOBJ_vGetInfo(ptr long ptr)
@ stdcall HT_ComputeRGBGammaTable(long long long long long ptr)
@ stdcall HT_Get8BPPFormatPalette(ptr long long long)
@ stdcall HT_Get8BPPMaskPalette(ptr long long long long long)
@ stdcall HeapVidMemAllocAligned(ptr long long ptr ptr)
@ stdcall PALOBJ_cGetColors(ptr long long ptr)
@ stdcall PATHOBJ_bCloseFigure(ptr)
@ stdcall PATHOBJ_bEnum(ptr ptr)
@ stdcall PATHOBJ_bEnumClipLines(ptr long ptr)
@ stdcall PATHOBJ_bMoveTo(ptr long long)
@ stdcall PATHOBJ_bPolyBezierTo(ptr ptr long)
@ stdcall PATHOBJ_bPolyLineTo(ptr ptr long)
@ stdcall PATHOBJ_vEnumStart(ptr)
@ stdcall PATHOBJ_vEnumStartClipLines(ptr ptr ptr ptr)
@ stdcall PATHOBJ_vGetBounds(ptr ptr)
@ stdcall RtlAnsiCharToUnicodeChar(ptr) NTOSKRNL.RtlAnsiCharToUnicodeChar
@ stdcall -arch=x86_64,arm RtlCaptureContext(ptr) NTOSKRNL.RtlCaptureContext
@ stdcall -arch=x86_64,arm RtlCopyMemory(ptr ptr int64) NTOSKRNL.RtlCopyMemory
@ stdcall -arch=x86_64,arm RtlCopyMemoryNonTemporal(ptr ptr int64) NTOSKRNL.RtlCopyMemoryNonTemporal
@ stdcall -arch=x86_64,arm RtlFillMemory(ptr long long) NTOSKRNL.RtlFillMemory
@ cdecl -arch=x86_64,arm RtlLookupFunctionEntry(double ptr ptr) NTOSKRNL.RtlLookupFunctionEntry
@ stdcall -arch=x86_64,arm RtlMoveMemory(ptr ptr long) NTOSKRNL.RtlMoveMemory
@ stdcall RtlMultiByteToUnicodeN(ptr long ptr ptr long) NTOSKRNL.RtlMultiByteToUnicodeN
@ cdecl -arch=x86_64,arm RtlPcToFileHeader(ptr ptr) NTOSKRNL.RtlPcToFileHeader
@ stdcall RtlRaiseException(ptr) NTOSKRNL.RtlRaiseException
@ cdecl -arch=x86_64,arm RtlRestoreContext(ptr ptr) NTOSKRNL.RtlRestoreContext
@ stdcall RtlUnicodeToMultiByteN(ptr long ptr ptr long) NTOSKRNL.RtlUnicodeToMultiByteN
@ stdcall RtlUnicodeToMultiByteSize(ptr ptr long) NTOSKRNL.RtlUnicodeToMultiByteSize
@ stdcall RtlUnwind(ptr ptr ptr ptr) NTOSKRNL.RtlUnwind
@ stdcall -arch=x86_64,arm RtlUnwindEx(ptr ptr ptr ptr ptr ptr) NTOSKRNL.RtlUnwindEx
@ stdcall RtlUpcaseUnicodeChar(long) NTOSKRNL.RtlUpcaseUnicodeChar
@ stdcall RtlUpcaseUnicodeToMultiByteN(ptr long ptr ptr long) NTOSKRNL.RtlUpcaseUnicodeToMultiByteN
@ stdcall -arch=x86_64,arm RtlVirtualUnwind(long int64 int64 ptr ptr ptr ptr ptr) NTOSKRNL.RtlVirtualUnwind
@ stdcall -arch=x86_64,arm RtlZeroMemory(ptr long) NTOSKRNL.RtlZeroMemory
@ stdcall STROBJ_bEnum(ptr ptr ptr)
@ stdcall STROBJ_bEnumPositionsOnly(ptr ptr ptr)
@ stdcall STROBJ_bGetAdvanceWidths(ptr long long ptr)
@ stdcall STROBJ_dwGetCodePage(ptr)
@ stdcall STROBJ_fxBreakExtra(ptr)
@ stdcall STROBJ_fxCharacterExtra(ptr)
@ stdcall STROBJ_vEnumStart(ptr)
@ stdcall VidMemFree(ptr ptr)
@ stdcall WNDOBJ_bEnum(ptr long ptr)
@ stdcall WNDOBJ_cEnumStart(ptr long long long)
@ stdcall WNDOBJ_vSetConsumer(ptr ptr)
@ stdcall XFORMOBJ_bApplyXform(ptr long long ptr ptr) EXFORMOBJ_bApplyXform
@ stdcall XFORMOBJ_iGetFloatObjXform(ptr ptr) EXFORMOBJ_iGetFloatObjXform
@ stdcall XFORMOBJ_iGetXform(ptr ptr) EXFORMOBJ_iGetXform
@ stdcall XLATEOBJ_cGetPalette(ptr long long ptr)
@ stdcall XLATEOBJ_hGetColorTransform(ptr)
@ stdcall XLATEOBJ_iXlate(ptr long)
@ stdcall XLATEOBJ_piVector(ptr)
@ cdecl -arch=x86_64,arm __C_specific_handler(ptr long ptr ptr) NTOSKRNL.__C_specific_handler
@ cdecl -arch=x86_64,arm __chkstk(ptr long ptr ptr) NTOSKRNL.__chkstk
@ cdecl -arch=arm __jump_unwind() NTOSKRNL.__jump_unwind
@ cdecl -arch=i386 _abnormal_termination() NTOSKRNL._abnormal_termination
@ cdecl -arch=i386 _except_handler2() NTOSKRNL._except_handler2
@ cdecl -arch=i386 _global_unwind2() NTOSKRNL._global_unwind2
@ cdecl _itoa() NTOSKRNL._itoa
@ cdecl _itow() NTOSKRNL._itow
@ cdecl -arch=i386 _local_unwind2() NTOSKRNL._local_unwind2
@ cdecl -arch=x86_64 _local_unwind() NTOSKRNL._local_unwind
@ cdecl -arch=x86_64,arm _setjmp(ptr ptr) NTOSKRNL._setjmp
@ cdecl -arch=x86_64,arm _setjmpex(ptr ptr) NTOSKRNL._setjmpex
@ cdecl -arch=x86_64,arm longjmp(ptr long) NTOSKRNL.longjmp
@ cdecl -arch=x86_64,arm memcmp() NTOSKRNL.memcmp
@ cdecl -arch=x86_64,arm memcpy() NTOSKRNL.memcpy
@ cdecl -arch=x86_64,arm memmove() NTOSKRNL.memmove
@ cdecl -arch=x86_64,arm memset() NTOSKRNL.memset
