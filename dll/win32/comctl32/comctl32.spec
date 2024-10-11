# Functions exported by the Win95 comctl32.dll
# (these need to have these exact ordinals, because some win95 dlls
#  import comctl32.dll by ordinal)
#   This list was created from a comctl32.dll v5.81 (IE5.01).

  2 stdcall MenuHelp(long long long long long long ptr)
  3 stdcall ShowHideMenuCtl(long long ptr)
  4 stdcall GetEffectiveClientRect(long ptr ptr)
  5 stdcall DrawStatusTextA(long ptr str long)
  6 stdcall CreateStatusWindowA(long str long long)
  7 stdcall CreateToolbar(long long long long long long ptr long)
  8 stdcall CreateMappedBitmap(long long long ptr long)
  9 stdcall -noname DPA_LoadStream(ptr ptr ptr ptr)
 10 stdcall -noname DPA_SaveStream(ptr ptr ptr ptr)
 11 stdcall -noname DPA_Merge(ptr ptr long ptr ptr long)
#12 stub Cctl1632_ThunkData32
 13 stdcall MakeDragList(long)
 14 stdcall LBItemFromPt(long int64 long)
 15 stdcall DrawInsert(long long long)
 16 stdcall CreateUpDownControl(long long long long long long long long long long long long)
 17 stdcall InitCommonControls()
 71 stdcall -noname Alloc(long)
 72 stdcall -noname ReAlloc(ptr long)
 73 stdcall -noname Free(ptr)
 74 stdcall -noname GetSize(ptr)
151 stdcall -noname CreateMRUListA(ptr)
152 stdcall -noname FreeMRUList(long)
153 stdcall -noname AddMRUStringA(long str)
154 stdcall -noname EnumMRUListA(long long ptr long)
155 stdcall -noname FindMRUStringA(long str ptr)
156 stdcall -noname DelMRUString(long long)
157 stdcall -noname CreateMRUListLazyA(ptr long long long)
163 stub -noname CreatePage
164 stub -noname CreateProxyPage
167 stdcall -noname AddMRUData(long ptr long)
169 stdcall -noname FindMRUData(long ptr long ptr)
233 stdcall -noname Str_GetPtrA(str str long)
234 stdcall -noname Str_SetPtrA(ptr str)
235 stdcall -noname Str_GetPtrW(wstr wstr long)
236 stdcall -noname Str_SetPtrW(ptr wstr)
320 stdcall -ordinal DSA_Create(long long)
321 stdcall -ordinal DSA_Destroy(ptr)
322 stdcall -ordinal DSA_GetItem(ptr long ptr)
323 stdcall -ordinal DSA_GetItemPtr(ptr long)
324 stdcall -ordinal DSA_InsertItem(ptr long ptr)
325 stdcall -ordinal DSA_SetItem (ptr long ptr)
326 stdcall -ordinal DSA_DeleteItem(ptr long)
327 stdcall -ordinal DSA_DeleteAllItems(ptr)
328 stdcall -ordinal DPA_Create(long)
329 stdcall -ordinal DPA_Destroy(ptr)
330 stdcall -ordinal DPA_Grow(ptr long)
331 stdcall -ordinal DPA_Clone(ptr ptr)
332 stdcall -ordinal DPA_GetPtr(ptr long)
333 stdcall -ordinal DPA_GetPtrIndex(ptr ptr)
334 stdcall -ordinal DPA_InsertPtr(ptr long ptr)
335 stdcall -ordinal DPA_SetPtr(ptr long ptr)
336 stdcall -ordinal DPA_DeletePtr(ptr long)
337 stdcall -ordinal DPA_DeleteAllPtrs(ptr)
338 stdcall -ordinal DPA_Sort(ptr ptr long)
339 stdcall -ordinal DPA_Search(ptr ptr long ptr long long)
340 stdcall -ordinal DPA_CreateEx(long long)
341 stdcall -noname SendNotify(long long long ptr)
342 stdcall -noname SendNotifyEx(long long long ptr long)
344 stdcall -ordinal TaskDialog(long long wstr wstr wstr long wstr ptr)
345 stdcall -ordinal TaskDialogIndirect(ptr ptr ptr ptr)
350 stdcall -noname -private StrChrA(str long) kernelbase.StrChrA
351 stdcall -noname -private StrRChrA(str str long) kernelbase.StrRChrA
352 stdcall -noname -private StrCmpNA(str str long) kernelbase.StrCmpNA
353 stdcall -noname -private StrCmpNIA(str str long) kernelbase.StrCmpNIA
354 stdcall -noname -private StrStrA(str str) kernelbase.StrStrA
355 stdcall -noname -private StrStrIA(str str) kernelbase.StrStrIA
356 stdcall -noname -private StrCSpnA(str str) kernelbase.StrCSpnA
357 stdcall -noname -private StrToIntA(str) kernelbase.StrToIntA
358 stdcall -noname -private StrChrW(wstr long) kernelbase.StrChrW
359 stdcall -noname -private StrRChrW(wstr wstr long) kernelbase.StrRChrW
360 stdcall -noname -private StrCmpNW(wstr wstr long) kernelbase.StrCmpNW
361 stdcall -noname -private StrCmpNIW(wstr wstr long) kernelbase.StrCmpNIW
362 stdcall -noname -private StrStrW(wstr wstr) kernelbase.StrStrW
363 stdcall -noname -private StrStrIW(wstr wstr) kernelbase.StrStrIW
364 stdcall -noname -private StrCSpnW(wstr wstr) kernelbase.StrCSpnW
365 stdcall -noname -private StrToIntW(wstr) kernelbase.StrToIntW
366 stdcall -noname -private StrChrIA(str long) kernelbase.StrChrIA
367 stdcall -noname -private StrChrIW(wstr long) kernelbase.StrChrIW
368 stdcall -noname -private StrRChrIA(str str long) kernelbase.StrRChrIA
369 stdcall -noname -private StrRChrIW(wstr wstr long) kernelbase.StrRChrIW
372 stdcall -noname -private StrRStrIA(str str str) kernelbase.StrRStrIA
373 stdcall -noname -private StrRStrIW(wstr wstr wstr) kernelbase.StrRStrIW
374 stdcall -noname -private StrCSpnIA(str str) kernelbase.StrCSpnIA
375 stdcall -noname -private StrCSpnIW(wstr wstr) kernelbase.StrCSpnIW
376 stdcall -noname -private IntlStrEqWorkerA(long str str long)
377 stdcall -noname -private IntlStrEqWorkerW(long wstr wstr long)
380 stdcall -ordinal LoadIconMetric(ptr wstr long ptr)
381 stdcall -ordinal LoadIconWithScaleDown(ptr wstr long long ptr)
382 stdcall -noname SmoothScrollWindow(ptr)
383 stub -noname DoReaderMode
384 stdcall -noname SetPathWordBreakProc(ptr long)
385 stdcall -ordinal DPA_EnumCallback(ptr ptr ptr)
386 stdcall -ordinal DPA_DestroyCallback(ptr ptr ptr)
387 stdcall -ordinal DSA_EnumCallback(ptr ptr ptr)
388 stdcall -ordinal DSA_DestroyCallback(ptr ptr ptr)
389 stub -noname SHGetProcessDword
390 stdcall -noname ImageList_SetColorTable(ptr long long ptr)
400 stdcall -ordinal CreateMRUListW(ptr)
401 stdcall -ordinal AddMRUStringW(long wstr)
402 stdcall -noname FindMRUStringW(long wstr ptr)
403 stdcall -ordinal EnumMRUListW(long long ptr long)
404 stdcall -noname CreateMRUListLazyW(ptr long long long)
410 stdcall -ordinal SetWindowSubclass(long ptr long long)
411 stdcall -ordinal GetWindowSubclass(long ptr long ptr)
412 stdcall -ordinal RemoveWindowSubclass(long ptr long)
413 stdcall -ordinal DefSubclassProc(long long long long)
414 stdcall -noname MirrorIcon(ptr ptr)
415 stdcall -noname DrawTextWrap(long wstr long ptr long) user32.DrawTextW
416 stdcall -noname DrawTextExPrivWrap(long wstr long ptr long ptr) user32.DrawTextExW
417 stdcall -noname ExtTextOutWrap(long long long long ptr wstr long ptr) gdi32.ExtTextOutW
418 stdcall -noname GetCharWidthWrap(long long long long) gdi32.GetCharWidthW
419 stdcall -noname GetTextExtentPointWrap(long wstr long ptr) gdi32.GetTextExtentPointW
420 stdcall -noname GetTextExtentPoint32Wrap(long wstr long ptr) gdi32.GetTextExtentPoint32W
421 stdcall -noname TextOutWrap(long long long wstr long) gdi32.TextOutW

# Functions imported by name

@ stdcall CreatePropertySheetPage(ptr) CreatePropertySheetPageA
@ stdcall CreatePropertySheetPageA(ptr)
@ stdcall CreatePropertySheetPageW(ptr)
@ stdcall CreateStatusWindow(long str long long) CreateStatusWindowA
@ stdcall CreateStatusWindowW(long wstr long long)
@ stdcall CreateToolbarEx(long long long long long long ptr long long long long long long)
@ stdcall DestroyPropertySheetPage(long)
@ stdcall -private DllGetVersion(ptr)
@ stdcall -private DllInstall(long wstr)
@ stdcall -ret64 DPA_GetSize(ptr)
@ stdcall DrawShadowText(long wstr long ptr long long long long long)
@ stdcall DrawStatusText(long ptr ptr long) DrawStatusTextA
@ stdcall DrawStatusTextW(long ptr wstr long)
@ stdcall DSA_Clone(ptr)
@ stdcall -ret64 DSA_GetSize(ptr)
@ stdcall FlatSB_EnableScrollBar (long long long)
@ stdcall FlatSB_GetScrollInfo (long long ptr)
@ stdcall FlatSB_GetScrollPos (long long)
@ stdcall FlatSB_GetScrollProp (long long ptr)
@ stdcall FlatSB_GetScrollRange (long long ptr ptr)
@ stdcall FlatSB_SetScrollInfo (long long ptr long)
@ stdcall FlatSB_SetScrollPos (long long long long)
@ stdcall FlatSB_SetScrollProp (long long long long)
@ stdcall FlatSB_SetScrollRange (long long long long long)
@ stdcall FlatSB_ShowScrollBar (long long long)
@ stdcall GetMUILanguage()
@ stdcall HIMAGELIST_QueryInterface(ptr ptr ptr)
@ stdcall ImageList_Add(ptr long long)
@ stdcall ImageList_AddIcon(ptr long)
@ stdcall ImageList_AddMasked(ptr long long)
@ stdcall ImageList_BeginDrag(ptr long long long)
@ stdcall ImageList_CoCreateInstance(ptr ptr ptr ptr)
@ stdcall ImageList_Copy(ptr long ptr long long)
@ stdcall ImageList_Create(long long long long long)
@ stdcall ImageList_Destroy(ptr)
@ stdcall ImageList_DragEnter(long long long)
@ stdcall ImageList_DragLeave(long)
@ stdcall ImageList_DragMove(long long)
@ stdcall ImageList_DragShowNolock(long)
@ stdcall ImageList_Draw(ptr long long long long long)
@ stdcall ImageList_DrawEx(ptr long long long long long long long long long)
@ stdcall ImageList_DrawIndirect(ptr)
@ stdcall ImageList_Duplicate(ptr)
@ stdcall ImageList_EndDrag()
@ stdcall ImageList_GetBkColor(ptr)
@ stdcall ImageList_GetDragImage(ptr ptr)
@ stdcall ImageList_GetFlags(ptr)
@ stdcall ImageList_GetIcon(ptr long long)
@ stdcall ImageList_GetIconSize(ptr ptr ptr)
@ stdcall ImageList_GetImageCount(ptr)
@ stdcall ImageList_GetImageInfo(ptr long ptr)
@ stdcall ImageList_GetImageRect(ptr long ptr)
@ stdcall ImageList_LoadImage(long str long long long long long) ImageList_LoadImageA
@ stdcall ImageList_LoadImageA(long str long long long long long)
@ stdcall ImageList_LoadImageW(long wstr long long long long long)
@ stdcall ImageList_Merge(ptr long ptr long long long)
@ stdcall ImageList_Read(ptr)
@ stdcall ImageList_Remove(ptr long)
@ stdcall ImageList_Replace(ptr long long long)
@ stdcall ImageList_ReplaceIcon(ptr long long)
@ stdcall ImageList_SetBkColor(ptr long)
@ stdcall ImageList_SetDragCursorImage(ptr long long long)
@ stdcall ImageList_SetFilter(ptr long long)
@ stdcall ImageList_SetFlags(ptr long)
@ stdcall ImageList_SetIconSize(ptr long long)
@ stdcall ImageList_SetImageCount(ptr long)
@ stdcall ImageList_SetOverlayImage(ptr long long)
@ stdcall ImageList_Write(ptr ptr)
@ stdcall ImageList_WriteEx(ptr long ptr)
@ stdcall InitCommonControlsEx(ptr)
@ stdcall InitMUILanguage(long)
@ stdcall InitializeFlatSB(long)
@ stdcall PropertySheet(ptr) PropertySheetA
@ stdcall PropertySheetA(ptr)
@ stdcall PropertySheetW(ptr)
@ stdcall RegisterClassNameW(wstr)
@ stdcall UninitializeFlatSB(long)
@ stdcall _TrackMouseEvent(ptr)
