1 stdcall AbortDoc(ptr)
2 stdcall AbortPath(ptr)
3 stdcall AddFontMemResourceEx(ptr long ptr ptr)
4 stdcall AddFontResourceA(str)
5 stdcall AddFontResourceExA(str long ptr)
6 stdcall AddFontResourceExW(wstr long ptr)
7 stdcall AddFontResourceTracking(str long)
8 stdcall AddFontResourceW(wstr)
9 stdcall AngleArc(ptr long long long long long)
10 stdcall AnimatePalette(long long long ptr)
11 stdcall AnyLinkedFonts() NtGdiAnyLinkedFonts
12 stdcall Arc(long long long long long long long long long)
13 stdcall ArcTo(long long long long long long long long long)
14 stdcall BRUSHOBJ_hGetColorTransform(ptr) NtGdiBRUSHOBJ_hGetColorTransform
15 stdcall BRUSHOBJ_pvAllocRbrush(ptr long) NtGdiBRUSHOBJ_pvAllocRbrush
16 stdcall BRUSHOBJ_pvGetRbrush(ptr) NtGdiBRUSHOBJ_pvGetRbrush
17 stdcall BRUSHOBJ_ulGetBrushColor(ptr) NtGdiBRUSHOBJ_ulGetBrushColor
18 stdcall BeginPath(long)
19 stdcall BitBlt(long long long long long long long long long)
20 stdcall CLIPOBJ_bEnum(ptr long long) NtGdiCLIPOBJ_bEnum
21 stdcall CLIPOBJ_cEnumStart(ptr long long long long) NtGdiCLIPOBJ_cEnumStart
22 stdcall CLIPOBJ_ppoGetPath(ptr) NtGdiCLIPOBJ_ppoGetPath
23 stdcall CancelDC(long)
24 stdcall CheckColorsInGamut(ptr ptr ptr long)
25 stdcall ChoosePixelFormat(ptr ptr)
26 stdcall Chord(ptr long long long long long long long long)
27 stdcall ClearBitmapAttributes(ptr long)
28 stdcall ClearBrushAttributes(ptr long)
29 stdcall CloseEnhMetaFile(ptr)
30 stdcall CloseFigure(ptr)
31 stdcall CloseMetaFile(ptr)
32 stdcall ColorCorrectPalette(ptr ptr long long)
33 stdcall ColorMatchToTarget(ptr ptr long)
34 stdcall CombineRgn(long long long long)
35 stdcall CombineTransform(ptr ptr ptr)
36 stdcall CopyEnhMetaFileA(long str)
37 stdcall CopyEnhMetaFileW(long wstr)
38 stdcall CopyMetaFileA(long str)
39 stdcall CopyMetaFileW(long wstr)
40 stdcall CreateBitmap(long long long long ptr)
41 stdcall CreateBitmapIndirect(ptr)
42 stdcall CreateBrushIndirect(ptr)
43 stdcall CreateColorSpaceA(ptr)
44 stdcall CreateColorSpaceW(ptr)
45 stdcall CreateCompatibleBitmap(ptr long long)
46 stdcall CreateCompatibleDC(ptr)
47 stdcall CreateDCA(str str str ptr)
48 stdcall CreateDCW(wstr wstr wstr ptr)
49 stdcall CreateDIBPatternBrush(long long)
50 stdcall CreateDIBPatternBrushPt(long long)
51 stdcall CreateDIBSection(long ptr long ptr long long)
52 stdcall CreateDIBitmap(long ptr long ptr ptr long)
53 stdcall CreateDiscardableBitmap(long long long)
54 stdcall CreateEllipticRgn(long long long long) NtGdiCreateEllipticRgn
55 stdcall CreateEllipticRgnIndirect(ptr)
56 stdcall CreateEnhMetaFileA(long str ptr str)
57 stdcall CreateEnhMetaFileW(long wstr ptr wstr)
58 stdcall CreateFontA(long long long long long long long long long long long long long str)
59 stdcall CreateFontIndirectA(ptr)
60 stdcall CreateFontIndirectExA(ptr)
61 stdcall CreateFontIndirectExW(ptr)
62 stdcall CreateFontIndirectW(ptr)
63 stdcall CreateFontW(long long long long long long long long long long long long long wstr)
64 stdcall CreateHalftonePalette(long) NtGdiCreateHalftonePalette
65 stdcall CreateHatchBrush(long long)
66 stdcall CreateICA(str str str ptr)
67 stdcall CreateICW(wstr wstr wstr ptr)
68 stdcall CreateMetaFileA(str)
69 stdcall CreateMetaFileW(wstr)
70 stdcall CreatePalette(ptr)
71 stdcall CreatePatternBrush(long)
72 stdcall CreatePen(long long long)
73 stdcall CreatePenIndirect(ptr)
74 stdcall CreatePolyPolygonRgn(ptr ptr long long)
75 stdcall CreatePolygonRgn(ptr long long)
76 stdcall CreateRectRgn(long long long long)
77 stdcall CreateRectRgnIndirect(ptr)
78 stdcall CreateRoundRectRgn(long long long long long long) NtGdiCreateRoundRectRgn
79 stdcall CreateScalableFontResourceA(long str str str)
80 stdcall CreateScalableFontResourceW(long wstr wstr wstr)
81 stdcall CreateSolidBrush(long)
82 stdcall DPtoLP(long ptr long)
83 stdcall DdEntry0(ptr ptr ptr ptr ptr ptr) NtGdiDxgGenericThunk
84 stdcall DdEntry10(ptr ptr) NtGdiDdBeginMoCompFrame
85 stdcall DdEntry11(ptr ptr ptr) NtGdiDdBlt
86 stdcall DdEntry12(ptr ptr) NtGdiDdCanCreateSurface
87 stdcall DdEntry13(ptr ptr) NtGdiDdCanCreateD3DBuffer
88 stdcall DdEntry14(ptr ptr) NtGdiDdColorControl
89 stdcall DdEntry15(ptr) NtGdiDdCreateDirectDrawObject
90 stdcall DdEntry16(ptr ptr ptr ptr ptr ptr ptr ptr) NtGdiDdCreateSurface
91 stdcall DdEntry17(ptr ptr ptr ptr ptr ptr ptr ptr) NtGdiDdCreateD3DBuffer
92 stdcall DdEntry18(ptr ptr) NtGdiDdCreateMoComp
93 stdcall DdEntry19(ptr ptr ptr ptr ptr long) NtGdiDdCreateSurfaceObject
94 stdcall DdEntry1(ptr ptr ptr ptr) NtGdiD3dContextCreate
95 stdcall DdEntry20(ptr) NtGdiDdDeleteDirectDrawObject
96 stdcall DdEntry21(ptr) NtGdiDdDeleteSurfaceObject
97 stdcall DdEntry22(ptr ptr) NtGdiDdDestroyMoComp
98 stdcall DdEntry23(ptr long) NtGdiDdDestroySurface
99 stdcall DdEntry24(ptr) NtGdiDdDestroyD3DBuffer
100 stdcall DdEntry25(ptr ptr) NtGdiDdEndMoCompFrame
101 stdcall DdEntry26(ptr ptr ptr ptr ptr) NtGdiDdFlip
102 stdcall DdEntry27(ptr long) NtGdiDdFlipToGDISurface
103 stdcall DdEntry28(ptr ptr) NtGdiDdGetAvailDriverMemory
104 stdcall DdEntry29(ptr ptr) NtGdiDdGetBltStatus
105 stdcall DdEntry2(ptr) NtGdiD3dContextDestroy
106 stdcall DdEntry30(ptr ptr) NtGdiDdGetDC
107 stdcall DdEntry31(ptr ptr) NtGdiDdGetDriverInfo
108 stdcall DdEntry32(ptr ptr long) NtGdiDdGetDxHandle
109 stdcall DdEntry33(ptr ptr) NtGdiDdGetFlipStatus
110 stdcall DdEntry34(ptr ptr) NtGdiDdGetInternalMoCompInfo
111 stdcall DdEntry35(ptr ptr) NtGdiDdGetMoCompBuffInfo
112 stdcall DdEntry36(ptr ptr) NtGdiDdGetMoCompGuids
113 stdcall DdEntry37(ptr ptr) NtGdiDdGetMoCompFormats
114 stdcall DdEntry38(ptr ptr) NtGdiDdGetScanLine
115 stdcall DdEntry39(ptr ptr ptr) NtGdiDdLock
116 stdcall DdEntry3(ptr) NtGdiD3dContextDestroyAll
117 stdcall DdEntry40(ptr ptr) NtGdiDdLockD3D
118 stdcall DdEntry41(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) NtGdiDdQueryDirectDrawObject
119 stdcall DdEntry42(ptr ptr) NtGdiDdQueryMoCompStatus
120 stdcall DdEntry43(ptr ptr) NtGdiDdReenableDirectDrawObject
121 stdcall DdEntry44(ptr) NtGdiDdReleaseDC
122 stdcall DdEntry45(ptr ptr) NtGdiDdRenderMoComp
123 stdcall DdEntry46(ptr ptr) NtGdiDdResetVisrgn
124 stdcall DdEntry47(ptr ptr) NtGdiDdSetColorKey
125 stdcall DdEntry48(ptr ptr) NtGdiDdSetExclusiveMode
126 stdcall DdEntry49(ptr ptr ptr) NtGdiDdSetGammaRamp
127 stdcall DdEntry4(ptr) NtGdiD3dValidateTextureStageState
128 stdcall DdEntry50(ptr ptr long) NtGdiDdCreateSurfaceEx
129 stdcall DdEntry51(ptr ptr ptr) NtGdiDdSetOverlayPosition
130 stdcall DdEntry52(ptr ptr) NtGdiDdUnattachSurface
131 stdcall DdEntry53(ptr ptr) NtGdiDdUnlock
132 stdcall DdEntry54(ptr ptr) NtGdiDdUnlockD3D
133 stdcall DdEntry55(ptr ptr long) NtGdiDdUpdateOverlay
134 stdcall DdEntry56(ptr ptr) NtGdiDdWaitForVerticalBlank
135 stdcall DdEntry5(ptr ptr ptr ptr ptr ptr ptr) NtGdiD3dDrawPrimitives2
136 stdcall DdEntry6(ptr) NtGdiDdGetDriverState
137 stdcall DdEntry7(ptr ptr ptr) NtGdiDdAddAttachedSurface
138 stdcall DdEntry8(ptr ptr ptr) NtGdiDdAlphaBlt
139 stdcall DdEntry9(ptr ptr) NtGdiDdAttachSurface
140 stdcall DeleteColorSpace(long) NtGdiDeleteColorSpace
141 stdcall DeleteDC(long)
142 stdcall DeleteEnhMetaFile(long)
143 stdcall DeleteMetaFile(long)
144 stdcall DeleteObject(long)
145 stdcall DescribePixelFormat(long long long ptr)
146 stdcall DeviceCapabilitiesExA(str str long str ptr)
147 stdcall DeviceCapabilitiesExW(wstr wstr long wstr ptr)
148 stdcall DrawEscape(long long long ptr)
149 stdcall Ellipse(long long long long long)
150 stdcall EnableEUDC(long) NtGdiEnableEudc
151 stdcall EndDoc(ptr)
152 stdcall EndFormPage(ptr)
153 stdcall EndPage(ptr)
154 stdcall EndPath(ptr)
155 stdcall EngAcquireSemaphore(ptr)
156 stdcall EngAlphaBlend(ptr ptr ptr ptr ptr ptr ptr) NtGdiEngAlphaBlend
157 stdcall EngAssociateSurface(ptr ptr long) NtGdiEngAssociateSurface
158 stdcall EngBitBlt(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) NtGdiEngBitBlt
159 stdcall EngCheckAbort(ptr) NtGdiEngCheckAbort
160 stdcall EngComputeGlyphSet(ptr ptr ptr)
161 stdcall EngCopyBits(ptr ptr ptr ptr ptr ptr) NtGdiEngCopyBits
162 stdcall EngCreateBitmap(int64 long long long ptr) NtGdiEngCreateBitmap
163 stdcall EngCreateClip() NtGdiEngCreateClip
164 stdcall EngCreateDeviceBitmap(ptr int64 long) NtGdiEngCreateDeviceBitmap
165 stdcall EngCreateDeviceSurface(ptr int64 long) NtGdiEngCreateDeviceSurface
166 stdcall EngCreatePalette(long long ptr long long long) NtGdiEngCreatePalette
167 stdcall EngCreateSemaphore()
168 stdcall EngDeleteClip(ptr) NtGdiEngDeleteClip
169 stdcall EngDeletePalette(ptr) NtGdiEngDeletePalette
170 stdcall EngDeletePath(ptr) NtGdiEngDeletePath
171 stdcall EngDeleteSemaphore(ptr)
172 stdcall EngDeleteSurface(ptr) NtGdiEngDeleteSurface
173 stdcall EngEraseSurface(ptr ptr long) NtGdiEngEraseSurface
174 stdcall EngFillPath(ptr ptr ptr ptr ptr ptr ptr) NtGdiEngFillPath
175 stdcall EngFindResource(ptr long long ptr)
176 stdcall EngFreeModule(ptr)
177 stdcall EngGetCurrentCodePage(ptr ptr)
178 stdcall EngGetDriverName(ptr)
179 stdcall EngGetPrinterDataFileName(ptr)
180 stdcall EngGradientFill(ptr ptr ptr ptr long ptr long ptr ptr long) NtGdiEngGradientFill
181 stdcall EngLineTo(ptr ptr ptr long long long long ptr ptr) NtGdiEngLineTo
182 stdcall EngLoadModule(ptr)
183 stdcall EngLockSurface(ptr) NtGdiEngLockSurface
184 stdcall EngMarkBandingSurface(ptr) NtGdiEngMarkBandingSurface
185 stdcall EngMultiByteToUnicodeN(wstr long ptr str long) RtlMultiByteToUnicodeN
186 stdcall EngMultiByteToWideChar(long wstr long str long)
187 stdcall EngPaint(ptr ptr ptr ptr ptr) NtGdiEngPaint
188 stdcall EngPlgBlt(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr long) NtGdiEngPlgBlt
189 stdcall EngQueryEMFInfo(ptr ptr)
190 stdcall EngQueryLocalTime(ptr)
191 stdcall EngReleaseSemaphore(ptr)
192 stdcall EngStretchBlt(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr long) NtGdiEngStretchBlt
193 stdcall EngStretchBltROP(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr long ptr long) NtGdiEngStretchBltROP
194 stdcall EngStrokeAndFillPath(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) NtGdiEngStrokeAndFillPath
195 stdcall EngStrokePath(ptr ptr ptr ptr ptr ptr ptr ptr) NtGdiEngStrokePath
196 stdcall EngTextOut(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) NtGdiEngTextOut
197 stdcall EngTransparentBlt(ptr ptr ptr ptr ptr ptr long long) NtGdiEngTransparentBlt
198 stdcall EngUnicodeToMultiByteN(str long long wstr long) RtlUnicodeToMultiByteN
199 stdcall EngUnlockSurface(ptr) NtGdiEngUnlockSurface
200 stdcall EngWideCharToMultiByte(long wstr long str long)
201 stdcall EnumEnhMetaFile(ptr long ptr ptr ptr)
202 stdcall EnumFontFamiliesA(ptr str ptr long)
203 stdcall EnumFontFamiliesExA(ptr ptr ptr long long)
204 stdcall EnumFontFamiliesExW(ptr ptr ptr long long)
205 stdcall EnumFontFamiliesW(ptr wstr ptr long)
206 stdcall EnumFontsA(ptr str ptr long)
207 stdcall EnumFontsW(ptr wstr ptr long)
208 stdcall EnumICMProfilesA(long ptr long)
209 stdcall EnumICMProfilesW(long ptr long)
210 stdcall EnumMetaFile(long long ptr ptr)
211 stdcall EnumObjects(long long ptr long)
212 stdcall EqualRgn(long long) NtGdiEqualRgn
213 stdcall Escape(ptr long long ptr ptr)
214 stdcall EudcLoadLinkW(wstr wstr long long)
215 stdcall EudcUnloadLinkW(wstr wstr)
216 stdcall ExcludeClipRect(long long long long long)
217 stdcall ExtCreatePen(long long ptr long ptr)
218 stdcall ExtCreateRegion(ptr long ptr)
219 stdcall ExtEscape(long long long ptr long ptr)
220 stdcall ExtFloodFill(long long long long long)
221 stdcall ExtSelectClipRgn(long long long)
222 stdcall ExtTextOutA(long long long long ptr str long ptr)
223 stdcall ExtTextOutW(long long long long ptr wstr long ptr)
224 stdcall FONTOBJ_cGetAllGlyphHandles(ptr ptr) NtGdiFONTOBJ_cGetAllGlyphHandles
225 stdcall FONTOBJ_cGetGlyphs(ptr long long ptr ptr) NtGdiFONTOBJ_cGetGlyphs
226 stdcall FONTOBJ_pQueryGlyphAttrs(ptr long) NtGdiFONTOBJ_pQueryGlyphAttrs
227 stdcall FONTOBJ_pfdg(ptr) NtGdiFONTOBJ_pfdg
228 stdcall FONTOBJ_pifi(ptr) NtGdiFONTOBJ_pifi
229 stdcall FONTOBJ_pvTrueTypeFontFile(ptr ptr) NtGdiFONTOBJ_pvTrueTypeFontFile
230 stdcall FONTOBJ_pxoGetXform(ptr) NtGdiFONTOBJ_pxoGetXform
231 stdcall FONTOBJ_vGetInfo(ptr long ptr) NtGdiFONTOBJ_vGetInfo
232 stdcall FillPath(ptr)
233 stdcall FillRgn(ptr long long)
234 stdcall FixBrushOrgEx(ptr long long ptr)
235 stdcall FlattenPath(ptr)
236 stdcall FloodFill(ptr long long long)
237 stdcall FontIsLinked(ptr) NtGdiFontIsLinked
238 stdcall FrameRgn(ptr ptr ptr long long)
239 stdcall GdiAddFontResourceW(ptr ptr ptr)
240 stdcall GdiAddGlsBounds(ptr ptr)
241 stdcall GdiAddGlsRecord(ptr long ptr ptr)
242 stdcall GdiAlphaBlend(long long long long long long long long long long long)
243 stdcall GdiArtificialDecrementDriver(wstr long)
244 stdcall GdiCleanCacheDC(ptr)
245 stdcall GdiComment(long long ptr)
246 stdcall GdiConsoleTextOut(ptr ptr long ptr) NtGdiConsoleTextOut
247 stdcall GdiConvertAndCheckDC(ptr)
248 stdcall GdiConvertBitmap(ptr)
249 stdcall GdiConvertBitmapV5(ptr ptr long long)
250 stdcall GdiConvertBrush(ptr)
251 stdcall GdiConvertDC(ptr)
252 stdcall GdiConvertEnhMetaFile(ptr)
253 stdcall GdiConvertFont(ptr)
254 stdcall GdiConvertMetaFilePict(ptr)
255 stdcall GdiConvertPalette(ptr)
256 stdcall GdiConvertRegion(ptr)
257 stdcall GdiConvertToDevmodeW(ptr)
258 stdcall GdiCreateLocalEnhMetaFile(ptr)
259 stdcall GdiCreateLocalMetaFilePict(ptr)
260 stdcall GdiDeleteLocalDC(ptr)
261 stdcall GdiDeleteSpoolFileHandle(ptr)
262 stdcall GdiDescribePixelFormat(ptr long long ptr) NtGdiDescribePixelFormat
263 stdcall GdiDllInitialize(ptr long ptr)
264 stdcall GdiDrawStream(ptr long ptr)
265 stdcall GdiEndDocEMF(ptr)
266 stdcall GdiEndPageEMF(ptr long)
267 stdcall GdiEntry10(ptr long)
268 stdcall GdiEntry11(ptr ptr)
269 stdcall GdiEntry12(ptr ptr)
270 stdcall GdiEntry13()
271 stdcall GdiEntry14(ptr ptr long)
272 stdcall GdiEntry15(ptr ptr ptr)
273 stdcall GdiEntry16(ptr ptr ptr)
274 stdcall GdiEntry1(ptr ptr)
275 stdcall GdiEntry2(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
276 stdcall GdiEntry3(ptr)
277 stdcall GdiEntry4(ptr long)
278 stdcall GdiEntry5(ptr)
279 stdcall GdiEntry6(ptr ptr)
280 stdcall GdiEntry7(ptr ptr)
281 stdcall GdiEntry8(ptr)
282 stdcall GdiEntry9(ptr ptr ptr ptr ptr ptr)
283 stdcall GdiFixUpHandle(ptr)
284 stdcall GdiFlush()
285 stdcall GdiFullscreenControl(ptr ptr long ptr ptr) NtGdiFullscreenControl
286 stdcall GdiGetBatchLimit()
287 stdcall GdiGetBitmapBitsSize(ptr)
288 stdcall GdiGetCharDimensions(long ptr ptr)
289 stdcall GdiGetCodePage(long)
290 stdcall GdiGetDC(ptr)
291 stdcall GdiGetDevmodeForPage(ptr long ptr ptr)
292 stdcall GdiGetLocalBrush(ptr)
293 stdcall GdiGetLocalDC(ptr)
294 stdcall GdiGetLocalFont(ptr)
295 stdcall GdiGetPageCount(ptr)
296 stdcall GdiGetPageHandle(ptr long ptr)
297 stdcall GdiGetSpoolFileHandle(wstr ptr wstr)
298 stdcall GdiGetSpoolMessage(ptr long ptr long) NtGdiGetSpoolMessage
299 stdcall GdiGradientFill(long ptr long ptr long long)
300 stdcall GdiInitSpool() NtGdiInitSpool
301 stdcall GdiInitializeLanguagePack(long)
302 stdcall GdiIsMetaFileDC(long)
303 stdcall GdiIsMetaPrintDC(long)
304 stdcall GdiIsPlayMetafileDC(long)
305 stdcall GdiPlayDCScript(long long long long long long)
306 stdcall GdiPlayEMF(wstr ptr wstr ptr ptr)
307 stdcall GdiPlayJournal(long long long long long)
308 stdcall GdiPlayPageEMF(ptr ptr ptr ptr ptr)
309 stdcall GdiPlayPrivatePageEMF(ptr long ptr)
310 stdcall GdiPlayScript(ptr ptr ptr ptr ptr ptr ptr)
311 stdcall GdiPrinterThunk(ptr ptr long)
312 stdcall GdiProcessSetup()
313 stdcall GdiQueryFonts(ptr long ptr) NtGdiQueryFonts
314 stdcall GdiQueryTable()
315 stdcall GdiRealizationInfo(long ptr)
316 stdcall GdiReleaseDC(ptr)
317 stdcall GdiReleaseLocalDC(ptr)
318 stdcall GdiResetDCEMF(ptr ptr)
319 stdcall GdiSetAttrs(ptr)
320 stdcall GdiSetBatchLimit(long)
321 stdcall GdiSetLastError(long)
322 stdcall GdiSetPixelFormat(ptr long) NtGdiSetPixelFormat
323 stdcall GdiSetServerAttr(ptr long)
324 stdcall GdiStartDocEMF(ptr ptr)
325 stdcall GdiStartPageEMF(ptr)
326 stdcall GdiSwapBuffers(ptr) NtGdiSwapBuffers
327 stdcall GdiTransparentBlt(long long long long long long long long long long long)
328 stdcall GdiValidateHandle(ptr)
329 stdcall GetArcDirection(long)
330 stdcall GetAspectRatioFilterEx(long ptr)
331 stdcall GetBitmapAttributes(ptr)
332 stdcall GetBitmapBits(long long ptr) NtGdiGetBitmapBits
333 stdcall GetBitmapDimensionEx(long ptr) NtGdiGetBitmapDimension
334 stdcall GetBkColor(long)
335 stdcall GetBkMode(long)
336 stdcall GetBoundsRect(long ptr long)
337 stdcall GetBrushAttributes(ptr)
338 stdcall GetBrushOrgEx(long ptr)
339 stdcall GetCharABCWidthsA(long long long ptr)
340 stdcall GetCharABCWidthsFloatA(long long long ptr)
341 stdcall GetCharABCWidthsFloatW(long long long ptr)
342 stdcall GetCharABCWidthsI(long long long ptr ptr)
343 stdcall GetCharABCWidthsW(long long long ptr)
344 stdcall GetCharWidth32A(long long long long)
345 stdcall GetCharWidth32W(long long long long)
346 stdcall GetCharWidthA(long long long long) GetCharWidth32A
347 stdcall GetCharWidthFloatA(long long long ptr)
348 stdcall GetCharWidthFloatW(long long long ptr)
349 stdcall GetCharWidthI(ptr long long ptr ptr)
350 stdcall GetCharWidthInfo(ptr ptr) NtGdiGetCharWidthInfo
351 stdcall GetCharWidthW(long long long long)
352 stdcall GetCharacterPlacementA(long str long long ptr long)
353 stdcall GetCharacterPlacementW(long wstr long long ptr long)
354 stdcall GetClipBox(ptr ptr)
355 stdcall GetClipRgn(ptr long)
356 stdcall GetColorAdjustment(long ptr) NtGdiGetColorAdjustment
357 stdcall GetColorSpace(long)
358 stdcall GetCurrentObject(long long)
359 stdcall GetCurrentPositionEx(long ptr)
360 stdcall GetDCBrushColor(ptr)
361 stdcall GetDCOrgEx(ptr ptr)
362 stdcall GetDCPenColor(long)
363 stdcall GetDIBColorTable(long long long ptr)
364 stdcall GetDIBits(long long long long ptr ptr long)
365 stdcall GetDeviceCaps(long long)
366 stdcall GetDeviceGammaRamp(long ptr)
367 stdcall GetETM(ptr ptr)
368 stdcall GetEUDCTimeStamp()
369 stdcall GetEUDCTimeStampExW(str)
370 stdcall GetEnhMetaFileA(str)
371 stdcall GetEnhMetaFileBits(long long ptr)
372 stdcall GetEnhMetaFileDescriptionA(long long ptr)
373 stdcall GetEnhMetaFileDescriptionW(long long ptr)
374 stdcall GetEnhMetaFileHeader(long long ptr)
375 stdcall GetEnhMetaFilePaletteEntries (long long ptr)
376 stdcall GetEnhMetaFilePixelFormat(ptr long ptr)
377 stdcall GetEnhMetaFileW(wstr)
378 stdcall GetFontAssocStatus(ptr)
379 stdcall GetFontData(long long long ptr long)
380 stdcall GetFontLanguageInfo(long)
381 stdcall GetFontResourceInfoW(str ptr ptr long)
382 stdcall GetFontUnicodeRanges(ptr ptr) NtGdiGetFontUnicodeRanges
383 stdcall GetGlyphIndicesA(long ptr long ptr long)
384 stdcall GetGlyphIndicesW(long ptr long ptr long) NtGdiGetGlyphIndicesW
385 stdcall GetGlyphOutline(long long long ptr long ptr ptr) GetGlyphOutlineA
386 stdcall GetGlyphOutlineA(long long long ptr long ptr ptr)
387 stdcall GetGlyphOutlineW(long long long ptr long ptr ptr)
388 stdcall GetGlyphOutlineWow(long long long long long long long)
389 stdcall GetGraphicsMode(long)
390 stdcall GetHFONT(ptr)
391 stdcall GetICMProfileA(long ptr ptr)
392 stdcall GetICMProfileW(long ptr ptr)
393 stdcall GetKerningPairs(long long ptr) GetKerningPairsA
394 stdcall GetKerningPairsA(long long ptr)
395 stdcall GetKerningPairsW(long long ptr)
396 stdcall GetLayout(long)
397 stdcall GetLogColorSpaceA(long ptr long)
398 stdcall GetLogColorSpaceW(long ptr long)
399 stdcall GetMapMode(long)
400 stdcall GetMetaFileA(str)
401 stdcall GetMetaFileBitsEx(long long ptr)
402 stdcall GetMetaFileW(wstr)
403 stdcall GetMetaRgn(long long)
404 stdcall GetMiterLimit(long ptr) NtGdiGetMiterLimit
405 stdcall GetNearestColor(long long) NtGdiGetNearestColor
406 stdcall GetNearestPaletteIndex(long long) NtGdiGetNearestPaletteIndex
407 stdcall GetObjectA(long long ptr)
408 stdcall GetObjectType(long)
409 stdcall GetObjectW(long long ptr)
410 stdcall GetOutlineTextMetricsA(long long ptr)
411 stdcall GetOutlineTextMetricsW(long long ptr)
412 stdcall GetPaletteEntries(long long long ptr)
413 stdcall GetPath(long ptr ptr long)
414 stdcall GetPixel(long long long)
415 stdcall GetPixelFormat(long)
416 stdcall GetPolyFillMode(long)
417 stdcall GetROP2(long)
418 stdcall GetRandomRgn(long long long) NtGdiGetRandomRgn
419 stdcall GetRasterizerCaps(ptr long) NtGdiGetRasterizerCaps
420 stdcall GetRegionData(long long ptr)
421 stdcall GetRelAbs(long long)
422 stdcall GetRgnBox(long ptr)
423 stdcall GetStockObject(long)
424 stdcall GetStretchBltMode(long)
425 stdcall GetStringBitmapA(ptr str long long ptr)
426 stdcall GetStringBitmapW(ptr wstr long long ptr)
427 stdcall GetSystemPaletteEntries(long long long ptr)
428 stdcall GetSystemPaletteUse(long) NtGdiGetSystemPaletteUse
429 stdcall GetTextAlign(long)
430 stdcall GetTextCharacterExtra(long)
431 stdcall GetTextCharset(long)
432 stdcall GetTextCharsetInfo(long ptr long) NtGdiGetTextCharsetInfo
433 stdcall GetTextColor(long)
434 stdcall GetTextExtentExPointA(long str long long ptr ptr ptr)
435 stdcall GetTextExtentExPointI(long ptr long long ptr ptr ptr)
436 stdcall GetTextExtentExPointW(long wstr long long ptr ptr ptr)
437 stdcall GetTextExtentExPointWPri(ptr wstr long long long ptr ptr)
438 stdcall GetTextExtentPoint32A(long str long ptr)
439 stdcall GetTextExtentPoint32W(long wstr long ptr)
440 stdcall GetTextExtentPointA(long str long ptr)
441 stdcall GetTextExtentPointI(long ptr long ptr)
442 stdcall GetTextExtentPointW(long wstr long ptr)
443 stdcall GetTextFaceA(long long ptr)
444 stdcall GetTextFaceAliasW(ptr long wstr)
445 stdcall GetTextFaceW(long long ptr)
446 stdcall GetTextMetricsA(long ptr)
447 stdcall GetTextMetricsW(long ptr)
448 stdcall GetTransform(long long ptr) NtGdiGetTransform
449 stdcall GetViewportExtEx(long ptr)
450 stdcall GetViewportOrgEx(long ptr)
451 stdcall GetWinMetaFileBits(long long ptr long long)
452 stdcall GetWindowExtEx(long ptr)
453 stdcall GetWindowOrgEx(long ptr)
454 stdcall GetWorldTransform(long ptr)
455 stdcall HT_Get8BPPFormatPalette(ptr long long long) NtGdiHT_Get8BPPFormatPalette
456 stdcall HT_Get8BPPMaskPalette(ptr long long long long long) NtGdiHT_Get8BPPMaskPalette
457 stdcall IntersectClipRect(long long long long long)
458 stdcall InvertRgn(long long)
459 stdcall IsValidEnhMetaRecord(long long)
460 stdcall IsValidEnhMetaRecordOffExt(long long long long)
461 stdcall LPtoDP(long ptr long)
462 stdcall LineDDA(long long long long ptr long)
463 stdcall LineTo(long long long)
464 stdcall MaskBlt(long long long long long long long long long long long long)
465 stdcall MirrorRgn(ptr ptr)
466 stdcall ModifyWorldTransform(long ptr long)
467 stdcall MoveToEx(long long long ptr)
468 stdcall NamedEscape(ptr wstr long long str long str)
469 stdcall OffsetClipRgn(long long long)
470 stdcall OffsetRgn(long long long)
471 stdcall OffsetViewportOrgEx(long long long ptr)
472 stdcall OffsetWindowOrgEx(long long long ptr)
473 stdcall PATHOBJ_bEnum(ptr ptr) NtGdiPATHOBJ_bEnum
474 stdcall PATHOBJ_bEnumClipLines(ptr long ptr) NtGdiPATHOBJ_bEnumClipLines
475 stdcall PATHOBJ_vEnumStart(ptr) NtGdiPATHOBJ_vEnumStart
476 stdcall PATHOBJ_vEnumStartClipLines(ptr ptr ptr ptr) NtGdiPATHOBJ_vEnumStartClipLines
477 stdcall PATHOBJ_vGetBounds(ptr ptr) NtGdiPATHOBJ_vGetBounds
478 stdcall PaintRgn(long long)
479 stdcall PatBlt(long long long long long long)
480 stdcall PathToRegion(long)
481 stdcall Pie(long long long long long long long long long)
482 stdcall PlayEnhMetaFile(long long ptr)
483 stdcall PlayEnhMetaFileRecord(long ptr ptr long)
484 stdcall PlayMetaFile(long long)
485 stdcall PlayMetaFileRecord(long ptr ptr long)
486 stdcall PlgBlt(long ptr long long long long long long long long)
487 stdcall PolyBezier(long ptr long)
488 stdcall PolyBezierTo(long ptr long)
489 stdcall PolyDraw(long ptr ptr long)
490 stdcall PolyPatBlt(ptr long ptr long long)
491 stdcall PolyPolygon(long ptr ptr long)
492 stdcall PolyPolyline(long ptr ptr long)
493 stdcall PolyTextOutA(long ptr long)
494 stdcall PolyTextOutW(long ptr long)
495 stdcall Polygon(long ptr long)
496 stdcall Polyline(long ptr long)
497 stdcall PolylineTo(long ptr long)
498 stdcall PtInRegion(long long long)
499 stdcall PtVisible(long long long) NtGdiPtVisible
500 stdcall QueryFontAssocStatus()
501 stdcall RealizePalette(long)
502 stdcall RectInRegion(long ptr)
503 stdcall RectVisible(long ptr) NtGdiRectVisible
504 stdcall Rectangle(long long long long long)
505 stdcall RemoveFontMemResourceEx(ptr)
506 stdcall RemoveFontResourceA(str)
507 stdcall RemoveFontResourceExA(str long ptr)
508 stdcall RemoveFontResourceExW(wstr long ptr)
509 stdcall RemoveFontResourceTracking(ptr long)
510 stdcall RemoveFontResourceW(wstr)
511 stdcall ResetDCA(long ptr)
512 stdcall ResetDCW(long ptr)
513 stdcall ResizePalette(long long)
514 stdcall RestoreDC(long long)
515 stdcall RoundRect(long long long long long long long)
516 stdcall STROBJ_bEnum(ptr ptr ptr) NtGdiSTROBJ_bEnum
517 stdcall STROBJ_bEnumPositionsOnly(ptr ptr ptr) NtGdiSTROBJ_bEnumPositionsOnly
518 stdcall STROBJ_bGetAdvanceWidths(ptr long long ptr) NtGdiSTROBJ_bGetAdvanceWidths
519 stdcall STROBJ_dwGetCodePage(ptr) NtGdiSTROBJ_dwGetCodePage
520 stdcall STROBJ_vEnumStart(ptr) NtGdiSTROBJ_vEnumStart
521 stdcall SaveDC(long)
522 stdcall ScaleViewportExtEx(long long long long long ptr)
523 stdcall ScaleWindowExtEx(long long long long long ptr)
524 stdcall SelectBrushLocal(ptr ptr)
525 stdcall SelectClipPath(long long)
526 stdcall SelectClipRgn(long long)
527 stdcall SelectFontLocal(ptr ptr)
528 stdcall SelectObject(long long)
529 stdcall SelectPalette(long long long)
530 stdcall SetAbortProc(long ptr)
531 stdcall SetArcDirection(long long)
532 stdcall SetBitmapAttributes(ptr long)
533 stdcall SetBitmapBits(long long ptr) NtGdiSetBitmapBits
534 stdcall SetBitmapDimensionEx(long long long ptr) NtGdiSetBitmapDimension
535 stdcall SetBkColor(long long)
536 stdcall SetBkMode(long long)
537 stdcall SetBoundsRect(long ptr long)
538 stdcall SetBrushAttributes(ptr long)
539 stdcall SetBrushOrgEx(long long long ptr)
540 stdcall SetColorAdjustment(long ptr)
541 stdcall SetColorSpace(long long)
542 stdcall SetDCBrushColor(long long)
543 stdcall SetDCPenColor(long long)
544 stdcall SetDIBColorTable(long long long ptr)
545 stdcall SetDIBits(long long long long ptr ptr long)
546 stdcall SetDIBitsToDevice(long long long long long long long long long ptr ptr long)
547 stdcall SetDeviceGammaRamp(long ptr)
548 stdcall SetEnhMetaFileBits(long ptr)
549 stdcall SetFontEnumeration(ptr) NtGdiSetFontEnumeration
550 stdcall SetGraphicsMode(long long)
551 stdcall SetICMMode(long long)
552 stdcall SetICMProfileA(long str)
553 stdcall SetICMProfileW(long wstr)
554 stdcall SetLayout(long long)
555 stdcall SetLayoutWidth(ptr long long)
556 stdcall SetMagicColors(ptr long long) NtGdiSetMagicColors
557 stdcall SetMapMode(long long)
558 stdcall SetMapperFlags(long long)
559 stdcall SetMetaFileBitsEx(long ptr)
560 stdcall SetMetaRgn(long)
561 stdcall SetMiterLimit(long long ptr)
562 stdcall SetPaletteEntries(long long long ptr)
563 stdcall SetPixel(long long long long)
564 stdcall SetPixelFormat(long long ptr)
565 stdcall SetPixelV(long long long long)
566 stdcall SetPolyFillMode(long long)
567 stdcall SetROP2(long long)
568 stdcall SetRectRgn(long long long long long)
569 stdcall SetRelAbs(long long)
570 stdcall SetStretchBltMode(long long)
571 stdcall SetSystemPaletteUse(long long) NtGdiSetSystemPaletteUse
572 stdcall SetTextAlign(long long)
573 stdcall SetTextCharacterExtra(long long)
574 stdcall SetTextColor(long long)
575 stdcall SetTextJustification(long long long)
576 stdcall SetViewportExtEx(long long long ptr)
577 stdcall SetViewportOrgEx(long long long ptr)
578 stdcall SetVirtualResolution(long long long long long) NtGdiSetVirtualResolution
579 stdcall SetWinMetaFileBits(long ptr long ptr)
580 stdcall SetWindowExtEx(long long long ptr)
581 stdcall SetWindowOrgEx(long long long ptr)
582 stdcall SetWorldTransform(long ptr)
583 stdcall StartDocA(long ptr)
584 stdcall StartDocW(long ptr)
585 stdcall StartFormPage(ptr)
586 stdcall StartPage(long)
587 stdcall StretchBlt(long long long long long long long long long long long)
588 stdcall StretchDIBits(long long long long long long long long long ptr ptr long long)
589 stdcall StrokeAndFillPath(long)
590 stdcall StrokePath(long)
591 stdcall SwapBuffers(long)
592 stdcall TextOutA(long long long str long)
593 stdcall TextOutW(long long long wstr long)
594 stdcall TranslateCharsetInfo(ptr ptr long)
595 stdcall UnloadNetworkFonts(long)
596 stdcall UnrealizeObject(long)
597 stdcall UpdateColors(long)
598 stdcall UpdateICMRegKeyA(long str str long)
599 stdcall UpdateICMRegKeyW(long wstr wstr long)
600 stdcall WidenPath(long)
601 stdcall XFORMOBJ_bApplyXform(ptr long long ptr ptr) NtGdiXFORMOBJ_bApplyXform
602 stdcall XFORMOBJ_iGetXform(ptr ptr) NtGdiXFORMOBJ_iGetXform
603 stdcall XLATEOBJ_cGetPalette(ptr long long ptr) NtGdiXLATEOBJ_cGetPalette
604 stdcall XLATEOBJ_hGetColorTransform(ptr) NtGdiXLATEOBJ_hGetColorTransform
605 stdcall XLATEOBJ_iXlate(ptr long) NtGdiXLATEOBJ_iXlate
606 stdcall XLATEOBJ_piVector(ptr)
607 stdcall bInitSystemAndFontsDirectoriesW(wstr wstr)
608 stdcall bMakePathNameW(wstr wstr wstr long)
609 stdcall cGetTTFFromFOT(long long long long long long long)
610 stdcall gdiPlaySpoolStream(long long long long long long)
