; Functions exported by Win 2K3 SP2
1 stdcall ActivateKeyboardLayout(long long) NtUserActivateKeyboardLayout
2 stdcall AdjustWindowRect(ptr long long)
3 stdcall AdjustWindowRectEx(ptr long long long)
4 stdcall AlignRects(ptr long long long)
5 stdcall AllowForegroundActivation()
6 stdcall AllowSetForegroundWindow(long)
7 stdcall AnimateWindow(long long long)
8 stdcall AnyPopup()
9 stdcall AppendMenuA(long long long ptr)
10 stdcall AppendMenuW(long long long ptr)
11 stdcall ArrangeIconicWindows(long)
12 stdcall AttachThreadInput(long long long) NtUserAttachThreadInput
13 stdcall BeginDeferWindowPos(long)
14 stdcall BeginPaint(long ptr) NtUserBeginPaint
15 stdcall BlockInput(long) NtUserBlockInput
16 stdcall BringWindowToTop(long)
17 stdcall BroadcastSystemMessage(long ptr long long long) BroadcastSystemMessageA
18 stdcall BroadcastSystemMessageA(long ptr long long long)
19 stdcall BroadcastSystemMessageExA(long ptr long long long ptr)
20 stdcall BroadcastSystemMessageExW(long ptr long long long ptr)
21 stdcall BroadcastSystemMessageW(long ptr long long long)
22 stdcall BuildReasonArray(ptr)
23 stdcall CalcMenuBar(long long long long long) NtUserCalcMenuBar
24 stdcall CallMsgFilter(ptr long) CallMsgFilterA
25 stdcall CallMsgFilterA(ptr long)
26 stdcall CallMsgFilterW(ptr long)
27 stdcall CallNextHookEx(long long long long)
28 stdcall CallWindowProcA(ptr long long long long)
29 stdcall CallWindowProcW(ptr long long long long)
30 stdcall CascadeChildWindows(long long)
31 stdcall CascadeWindows(long long ptr long ptr)
32 stdcall ChangeClipboardChain(long long) NtUserChangeClipboardChain
33 stdcall ChangeDisplaySettingsA(ptr long)
34 stdcall ChangeDisplaySettingsExA(str ptr long long ptr)
35 stdcall ChangeDisplaySettingsExW(wstr ptr long long ptr)
36 stdcall ChangeDisplaySettingsW(ptr long)
37 stdcall ChangeMenuA(long long ptr long long)
38 stdcall ChangeMenuW(long long ptr long long)
39 stdcall CharLowerA(str)
40 stdcall CharLowerBuffA(str long)
41 stdcall CharLowerBuffW(wstr long)
42 stdcall CharLowerW(wstr)
43 stdcall CharNextA(str)
44 stdcall CharNextExA(long str long)
45 stdcall CharNextW(wstr)
46 stdcall CharPrevA(str str)
47 stdcall CharPrevExA(long str str long)
48 stdcall CharPrevW(wstr wstr)
49 stdcall CharToOemA(str ptr)
50 stdcall CharToOemBuffA(str ptr long)
51 stdcall CharToOemBuffW(wstr ptr long)
52 stdcall CharToOemW(wstr ptr)
53 stdcall CharUpperA(str)
54 stdcall CharUpperBuffA(str long)
55 stdcall CharUpperBuffW(wstr long)
56 stdcall CharUpperW(wstr)
57 stdcall CheckDlgButton(long long long)
58 stdcall CheckMenuItem(long long long)
59 stdcall CheckMenuRadioItem(long long long long long)
60 stdcall CheckRadioButton(long long long long)
61 stdcall ChildWindowFromPoint(long double)
62 stdcall ChildWindowFromPointEx(long double long) ; Direct call NtUserChildWindowFromPointEx
63 stdcall CliImmSetHotKey(long long long ptr)
64 stdcall ClientThreadSetup()
65 stdcall ClientToScreen(long ptr)
66 stdcall ClipCursor(ptr) NtUserClipCursor
67 stdcall CloseClipboard() NtUserCloseClipboard
68 stdcall CloseDesktop(long) NtUserCloseDesktop
69 stdcall CloseWindow(long)
70 stdcall CloseWindowStation(long) NtUserCloseWindowStation
71 stdcall CopyAcceleratorTableA(long ptr long)
72 stdcall CopyAcceleratorTableW(long ptr long) NtUserCopyAcceleratorTable
73 stdcall CopyIcon(long)
74 stdcall CopyImage(long long long long long)
75 stdcall CopyRect(ptr ptr)
76 stdcall CountClipboardFormats() NtUserCountClipboardFormats
77 stdcall CreateAcceleratorTableA(ptr long)
78 stdcall CreateAcceleratorTableW(ptr long) NtUserCreateAcceleratorTable
79 stdcall CreateCaret(long long long long) NtUserCreateCaret
80 stdcall CreateCursor(long long long long long ptr ptr)
81 stdcall CreateDesktopA(str str ptr long long ptr)
82 stdcall CreateDesktopW(wstr wstr ptr long long ptr)
83 stdcall CreateDialogIndirectParamA(long ptr long ptr long)
84 stdcall CreateDialogIndirectParamAorW(long ptr long ptr long long)
85 stdcall CreateDialogIndirectParamW(long ptr long ptr long)
86 stdcall CreateDialogParamA(long ptr long ptr long)
87 stdcall CreateDialogParamW(long ptr long ptr long)
88 stdcall CreateIcon(long long long long long ptr ptr)
89 stdcall CreateIconFromResource (ptr long long long)
90 stdcall CreateIconFromResourceEx(ptr long long long long long long)
91 stdcall CreateIconIndirect(ptr)
92 stdcall CreateMDIWindowA(ptr ptr long long long long long long long long)
93 stdcall CreateMDIWindowW(ptr ptr long long long long long long long long)
94 stdcall CreateMenu()
95 stdcall CreatePopupMenu()
96 stdcall CreateSystemThreads(long)
97 stdcall CreateWindowExA(long str str long long long long long long long long ptr)
98 stdcall CreateWindowExW(long wstr wstr long long long long long long long long ptr)
99 stdcall CreateWindowStationA(str long long ptr)
100 stdcall CreateWindowStationW(wstr long long ptr)
101 stdcall CsrBroadcastSystemMessageExW(long ptr long long long ptr)
102 stdcall CtxInitUser32()
# DbgWin32HeapFail
# DbgWin32HeapStat
105 stdcall DdeAbandonTransaction(long long long)
106 stdcall DdeAccessData(long ptr)
107 stdcall DdeAddData(long ptr long long)
108 stdcall DdeClientTransaction(ptr long long long long long long ptr)
109 stdcall DdeCmpStringHandles(long long)
110 stdcall DdeConnect(long long long ptr)
111 stdcall DdeConnectList(long long long long ptr)
112 stdcall DdeCreateDataHandle(long ptr long long long long long)
113 stdcall DdeCreateStringHandleA(long str long)
114 stdcall DdeCreateStringHandleW(long wstr long)
115 stdcall DdeDisconnect(long)
116 stdcall DdeDisconnectList(long)
117 stdcall DdeEnableCallback(long long long)
118 stdcall DdeFreeDataHandle(long)
119 stdcall DdeFreeStringHandle(long long)
120 stdcall DdeGetData(long ptr long long)
121 stdcall DdeGetLastError(long)
122 stdcall DdeGetQualityOfService(long long ptr) ; Direct call NtUserDdeGetQualityOfService
123 stdcall DdeImpersonateClient(long)
124 stdcall DdeInitializeA(ptr ptr long long)
125 stdcall DdeInitializeW(ptr ptr long long)
126 stdcall DdeKeepStringHandle(long long)
127 stdcall DdeNameService(long long long long)
128 stdcall DdePostAdvise(long long long)
129 stdcall DdeQueryConvInfo(long long ptr)
130 stdcall DdeQueryNextServer(long long)
131 stdcall DdeQueryStringA(long long ptr long long)
132 stdcall DdeQueryStringW(long long ptr long long)
133 stdcall DdeReconnect(long)
134 stdcall DdeSetQualityOfService(long ptr ptr) ; Direct call NtUserDdeSetQualityOfService
135 stdcall DdeSetUserHandle (long long long)
136 stdcall DdeUnaccessData(long)
137 stdcall DdeUninitialize(long)
138 stdcall DefDlgProcA(long long long long)
139 stdcall DefDlgProcW(long long long long)
140 stdcall DefFrameProcA(long long long long long)
141 stdcall DefFrameProcW(long long long long long)
142 stdcall DefMDIChildProcA(long long long long)
143 stdcall DefMDIChildProcW(long long long long)
144 stdcall DefRawInputProc(ptr long long)
145 stdcall DefWindowProcA(long long long long)
146 stdcall DefWindowProcW(long long long long)
147 stdcall DeferWindowPos(long long long long long long long long) ; Direct call NtUserDeferWindowPos
148 stdcall DeleteMenu(long long long) NtUserDeleteMenu
149 stdcall DeregisterShellHookWindow (long)
150 stdcall DestroyAcceleratorTable(long)
151 stdcall DestroyCaret()
152 stdcall DestroyCursor(long)
153 stdcall DestroyIcon(long)
154 stdcall DestroyMenu(long) NtUserDestroyMenu
155 stdcall DestroyReasons(ptr)
156 stdcall DestroyWindow(long) NtUserDestroyWindow
157 stdcall DeviceEventWorker(long long long long long)
158 stdcall DialogBoxIndirectParamA(long ptr long ptr long)
159 stdcall DialogBoxIndirectParamAorW(long ptr long ptr long long)
160 stdcall DialogBoxIndirectParamW(long ptr long ptr long)
161 stdcall DialogBoxParamA(long str long ptr long)
162 stdcall DialogBoxParamW(long wstr long ptr long)
163 stdcall DisableProcessWindowsGhosting()
164 stdcall DispatchMessageA(ptr)
165 stdcall DispatchMessageW(ptr)
166 stdcall DisplayExitWindowsWarnings(long)
167 stdcall DlgDirListA(long str long long long)
168 stdcall DlgDirListComboBoxA(long ptr long long long)
169 stdcall DlgDirListComboBoxW(long ptr long long long)
170 stdcall DlgDirListW(long wstr long long long)
171 stdcall DlgDirSelectComboBoxExA(long ptr long long)
172 stdcall DlgDirSelectComboBoxExW(long ptr long long)
173 stdcall DlgDirSelectExA(long ptr long long)
174 stdcall DlgDirSelectExW(long ptr long long)
175 stdcall DragDetect(long double) ; Direct call NtUserDragDetect
176 stdcall DragObject(long long long long long) NtUserDragObject
177 stdcall DrawAnimatedRects(long long ptr ptr) NtUserDrawAnimatedRects
178 stdcall DrawCaption(long long ptr long)
179 stdcall DrawCaptionTempA(long long ptr long long str long)
180 stdcall DrawCaptionTempW(long long ptr long long wstr long)
181 stdcall DrawEdge(long ptr long long)
182 stdcall DrawFocusRect(long ptr)
183 stdcall DrawFrame(long ptr long long)
184 stdcall DrawFrameControl(long ptr long long)
185 stdcall DrawIcon(long long long long)
186 stdcall DrawIconEx(long long long long long long long long long)
187 stdcall DrawMenuBar(long)
188 stdcall DrawMenuBarTemp(long long long long long) NtUserDrawMenuBarTemp
189 stdcall DrawStateA(long long ptr long long long long long long long)
190 stdcall DrawStateW(long long ptr long long long long long long long)
191 stdcall DrawTextA(long str long ptr long)
192 stdcall DrawTextExA(long str long ptr long ptr)
193 stdcall DrawTextExW(long wstr long ptr long ptr)
194 stdcall DrawTextW(long wstr long ptr long)
195 stdcall EditWndProc(long long long long) EditWndProcA
196 stdcall EmptyClipboard() NtUserEmptyClipboard
197 stdcall EnableMenuItem(long long long)
198 stdcall EnableScrollBar(long long long)
199 stdcall EnableWindow(long long)
200 stdcall EndDeferWindowPos(long)
201 stdcall EndDialog(long long)
202 stdcall EndMenu() NtUserEndMenu
203 stdcall EndPaint(long ptr) NtUserEndPaint
204 stdcall EndTask(ptr long long)
205 stdcall EnterReaderModeHelper(ptr)
206 stdcall EnumChildWindows(long ptr long)
207 stdcall EnumClipboardFormats(long)
208 stdcall EnumDesktopWindows(long ptr ptr)
209 stdcall EnumDesktopsA(ptr ptr long)
210 stdcall EnumDesktopsW(ptr ptr long)
211 stdcall EnumDisplayDevicesA(ptr long ptr long)
212 stdcall EnumDisplayDevicesW(ptr long ptr long)
213 stdcall EnumDisplayMonitors(long ptr ptr long) ; Direct call NtUserEnumDisplayMonitors
214 stdcall EnumDisplaySettingsA(str long ptr)
215 stdcall EnumDisplaySettingsExA(str long ptr long)
216 stdcall EnumDisplaySettingsExW(wstr long ptr long)
217 stdcall EnumDisplaySettingsW(wstr long ptr )
218 stdcall EnumPropsA(long ptr)
219 stdcall EnumPropsExA(long ptr long)
220 stdcall EnumPropsExW(long ptr long)
221 stdcall EnumPropsW(long ptr)
222 stdcall EnumThreadWindows(long ptr long)
223 stdcall EnumWindowStationsA(ptr long)
224 stdcall EnumWindowStationsW(ptr long)
225 stdcall EnumWindows(ptr long)
226 stdcall EqualRect(ptr ptr)
227 stdcall ExcludeUpdateRgn(long long) NtUserExcludeUpdateRgn
228 stdcall ExitWindowsEx(long long)
229 stdcall FillRect(long ptr long)
230 stdcall FindWindowA(str str)
231 stdcall FindWindowExA(long long str str)
232 stdcall FindWindowExW(long long wstr wstr)
233 stdcall FindWindowW(wstr wstr)
234 stdcall FlashWindow(long long)
235 stdcall FlashWindowEx(ptr) NtUserFlashWindowEx
236 stdcall FrameRect(long ptr long)
237 stdcall FreeDDElParam(long long)
238 stdcall GetActiveWindow()
239 stdcall GetAltTabInfo(long long ptr ptr long) GetAltTabInfoA
240 stdcall GetAltTabInfoA(long long ptr ptr long)
241 stdcall GetAltTabInfoW(long long ptr ptr long)
242 stdcall GetAncestor(long long) ; Direct call NtUserGetAncestor
243 stdcall GetAppCompatFlags(long)
244 stdcall GetAppCompatFlags2(long)
245 stdcall GetAsyncKeyState(long)
246 stdcall GetCapture()
247 stdcall GetCaretBlinkTime() NtUserGetCaretBlinkTime
248 stdcall GetCaretPos(ptr) NtUserGetCaretPos
249 stdcall GetClassInfoA(long str ptr)
250 stdcall GetClassInfoExA(long str ptr)
251 stdcall GetClassInfoExW(long wstr ptr)
252 stdcall GetClassInfoW(long wstr ptr)
253 stdcall GetClassLongA(long long)
@ stdcall -arch=x86_64 GetClassLongPtrA(ptr long)
@ stdcall -arch=x86_64 GetClassLongPtrW(ptr long)
254 stdcall GetClassLongW(long long)
255 stdcall GetClassNameA(long ptr long)
256 stdcall GetClassNameW(long ptr long)
257 stdcall GetClassWord(long long)
258 stdcall GetClientRect(long long)
259 stdcall GetClipCursor(ptr) NtUserGetClipCursor
260 stdcall GetClipboardData(long)
261 stdcall GetClipboardFormatNameA(long ptr long)
262 stdcall GetClipboardFormatNameW(long ptr long)
263 stdcall GetClipboardOwner() NtUserGetClipboardOwner
264 stdcall GetClipboardSequenceNumber() NtUserGetClipboardSequenceNumber
265 stdcall GetClipboardViewer() NtUserGetClipboardViewer
266 stdcall GetComboBoxInfo(long ptr) ; Direct call NtUserGetComboBoxInfo
267 stdcall GetCursor()
268 stdcall GetCursorFrameInfo(long long long long long)
269 stdcall GetCursorInfo(ptr) NtUserGetCursorInfo
270 stdcall GetCursorPos(ptr)
271 stdcall GetDC(long) NtUserGetDC
272 stdcall GetDCEx(long long long) NtUserGetDCEx
# GetDbgTagFlags
274 stdcall GetDesktopWindow()
275 stdcall GetDialogBaseUnits()
276 stdcall GetDlgCtrlID(long)
277 stdcall GetDlgItem(long long)
278 stdcall GetDlgItemInt(long long ptr long)
279 stdcall GetDlgItemTextA(long long ptr long)
280 stdcall GetDlgItemTextW(long long ptr long)
281 stdcall GetDoubleClickTime() NtUserGetDoubleClickTime
282 stdcall GetFocus()
283 stdcall GetForegroundWindow() NtUserGetForegroundWindow
284 stdcall GetGUIThreadInfo(long ptr) NtUserGetGUIThreadInfo
285 stdcall GetGuiResources(long long) NtUserGetGuiResources
286 stdcall GetIconInfo(long ptr)
287 stdcall GetInputDesktop()
288 stdcall GetInputState()
289 stdcall GetInternalWindowPos(long ptr ptr) NtUserGetInternalWindowPos
290 stdcall GetKBCodePage()
291 stdcall GetKeyNameTextA(long ptr long)
292 stdcall GetKeyNameTextW(long ptr long)
293 stdcall GetKeyState(long)
294 stdcall GetKeyboardLayout(long)
295 stdcall GetKeyboardLayoutList(long ptr) NtUserGetKeyboardLayoutList
296 stdcall GetKeyboardLayoutNameA(ptr)
297 stdcall GetKeyboardLayoutNameW(ptr)
298 stdcall GetKeyboardState(ptr) NtUserGetKeyboardState
299 stdcall GetKeyboardType(long)
300 stdcall GetLastActivePopup(long)
301 stdcall GetLastInputInfo(ptr)
302 stdcall GetLayeredWindowAttributes(long ptr ptr ptr) NtUserGetLayeredWindowAttributes
303 stdcall GetListBoxInfo(long) NtUserGetListBoxInfo
304 stdcall GetMenu(long)
305 stdcall GetMenuBarInfo(long long long ptr) NtUserGetMenuBarInfo
306 stdcall GetMenuCheckMarkDimensions()
307 stdcall GetMenuContextHelpId(long)
308 stdcall GetMenuDefaultItem(long long long)
309 stdcall GetMenuInfo(long ptr)
310 stdcall GetMenuItemCount(long)
311 stdcall GetMenuItemID(long long)
312 stdcall GetMenuItemInfoA(long long long ptr)
313 stdcall GetMenuItemInfoW(long long long ptr)
314 stdcall GetMenuItemRect(long long long ptr) NtUserGetMenuItemRect
315 stdcall GetMenuState(long long long)
316 stdcall GetMenuStringA(long long ptr long long)
317 stdcall GetMenuStringW(long long ptr long long)
318 stdcall GetMessageA(ptr long long long)
319 stdcall GetMessageExtraInfo()
320 stdcall GetMessagePos()
321 stdcall GetMessageTime()
322 stdcall GetMessageW(ptr long long long)
323 stdcall GetMonitorInfoA(long ptr)
324 stdcall GetMonitorInfoW(long ptr)
325 stdcall GetMouseMovePointsEx(long ptr ptr long long) NtUserGetMouseMovePointsEx
326 stdcall GetNextDlgGroupItem(long long long)
327 stdcall GetNextDlgTabItem(long long long)
328 stdcall GetOpenClipboardWindow() NtUserGetOpenClipboardWindow
329 stdcall GetParent(long)
330 stdcall GetPriorityClipboardFormat(ptr long) NtUserGetPriorityClipboardFormat
331 stdcall GetProcessDefaultLayout(ptr)
332 stdcall GetProcessWindowStation() NtUserGetProcessWindowStation
333 stdcall GetProgmanWindow ()
334 stdcall GetPropA(long str)
335 stdcall GetPropW(long wstr)
336 stdcall GetQueueStatus(long)
337 stdcall GetRawInputBuffer(ptr ptr long)
338 stdcall GetRawInputData(ptr long ptr ptr long)
339 stdcall GetRawInputDeviceInfoA(ptr long ptr ptr)
340 stdcall GetRawInputDeviceInfoW(ptr long ptr ptr)
341 stdcall GetRawInputDeviceList(ptr ptr long)
342 stdcall GetReasonTitleFromReasonCode(long long long)
343 stdcall GetRegisteredRawInputDevices(ptr ptr long)
# GetRipFlags
345 stdcall GetScrollBarInfo(long long ptr) ; NtUserGetScrollBarInfo
346 stdcall GetScrollInfo(long long ptr)
347 stdcall GetScrollPos(long long)
348 stdcall GetScrollRange(long long ptr ptr)
349 stdcall GetShellWindow()
350 stdcall GetSubMenu(long long)
351 stdcall GetSysColor(long)
352 stdcall GetSysColorBrush(long)
353 stdcall GetSystemMenu(long long) ; NtUserGetSystemMenu
354 stdcall GetSystemMetrics(long)
355 stdcall GetTabbedTextExtentA(long str long long ptr)
356 stdcall GetTabbedTextExtentW(long wstr long long ptr)
357 stdcall GetTaskmanWindow ()
358 stdcall GetThreadDesktop(long)
359 stdcall GetTitleBarInfo(long ptr) NtUserGetTitleBarInfo
360 stdcall GetTopWindow(long)
361 stdcall GetUpdateRect(long ptr long)
362 stdcall GetUpdateRgn(long long long)
363 stdcall GetUserObjectInformationA(long long ptr long ptr)
364 stdcall GetUserObjectInformationW(long long ptr long ptr) NtUserGetObjectInformation
365 stdcall GetUserObjectSecurity (long ptr ptr long ptr)
366 stdcall GetWinStationInfo(ptr)
367 stdcall GetWindow(long long)
368 stdcall GetWindowContextHelpId(long)
369 stdcall GetWindowDC(long) NtUserGetWindowDC
370 stdcall GetWindowInfo(long ptr)
371 stdcall GetWindowLongA(long long)
@ stdcall -arch=x86_64 GetWindowLongPtrA(ptr long)
@ stdcall -arch=x86_64 GetWindowLongPtrW(ptr long)
372 stdcall GetWindowLongW(long long)
373 stdcall GetWindowModuleFileName(long ptr long) GetWindowModuleFileNameA
374 stdcall GetWindowModuleFileNameA(long ptr long)
375 stdcall GetWindowModuleFileNameW(long ptr long)
376 stdcall GetWindowPlacement(long ptr) NtUserGetWindowPlacement
377 stdcall GetWindowRect(long ptr)
378 stdcall GetWindowRgn(long long)
379 stdcall GetWindowRgnBox(long ptr)
380 stdcall GetWindowTextA(long ptr long)
381 stdcall GetWindowTextLengthA(long)
382 stdcall GetWindowTextLengthW(long)
383 stdcall GetWindowTextW(long ptr long)
384 stdcall GetWindowThreadProcessId(long ptr)
385 stdcall GetWindowWord(long long)
386 stdcall GrayStringA(long long ptr long long long long long long)
387 stdcall GrayStringW(long long ptr long long long long long long)
388 stdcall HideCaret(long) NtUserHideCaret
389 stdcall HiliteMenuItem(long long long long) NtUserHiliteMenuItem
390 stdcall IMPGetIMEA(long ptr)
391 stdcall IMPGetIMEW(long ptr)
392 stdcall IMPQueryIMEA(ptr)
393 stdcall IMPQueryIMEW(ptr)
394 stdcall IMPSetIMEA(long ptr)
395 stdcall IMPSetIMEW(long ptr)
396 stdcall ImpersonateDdeClientWindow(long long) ; Direct call NtUserImpersonateDdeClientWindow
397 stdcall InSendMessage()
398 stdcall InSendMessageEx(ptr)
399 stdcall InflateRect(ptr long long)
400 stdcall InitializeLpkHooks(ptr)
401 stdcall InitializeWin32EntryTable(ptr)
402 stdcall InsertMenuA(long long long long ptr)
403 stdcall InsertMenuItemA(long long long ptr)
404 stdcall InsertMenuItemW(long long long ptr)
405 stdcall InsertMenuW(long long long long ptr)
406 stdcall InternalGetWindowText(long long long)
407 stdcall IntersectRect(ptr ptr ptr)
408 stdcall InvalidateRect(long ptr long) NtUserInvalidateRect
409 stdcall InvalidateRgn(long long long) NtUserInvalidateRgn
410 stdcall InvertRect(long ptr)
411 stdcall IsCharAlphaA(long)
412 stdcall IsCharAlphaNumericA(long)
413 stdcall IsCharAlphaNumericW(long)
414 stdcall IsCharAlphaW(long)
415 stdcall IsCharLowerA(long)
416 stdcall IsCharLowerW(long)
417 stdcall IsCharUpperA(long)
418 stdcall IsCharUpperW(long)
419 stdcall IsChild(long long)
420 stdcall IsClipboardFormatAvailable(long) NtUserIsClipboardFormatAvailable
421 stdcall IsDialogMessage(long ptr) IsDialogMessageA
422 stdcall IsDialogMessageA(long ptr)
423 stdcall IsDialogMessageW(long ptr)
424 stdcall IsDlgButtonChecked(long long)
425 stdcall IsGUIThread(long)
426 stdcall IsHungAppWindow(long)
427 stdcall IsIconic(long)
428 stdcall IsMenu(long)
429 stdcall -stub IsProcess16Bit()
430 stdcall IsRectEmpty(ptr)
431 stdcall IsSETEnabled()
432 stdcall IsServerSideWindow(long)
433 stdcall IsWinEventHookInstalled(long)
434 stdcall IsWindow(long)
435 stdcall IsWindowEnabled(long)
436 stdcall IsWindowInDestroy(long)
437 stdcall IsWindowUnicode(long)
438 stdcall IsWindowVisible(long)
439 stdcall -stub IsWow64Message()
440 stdcall IsZoomed(long)
441 stdcall KillSystemTimer(long long)
442 stdcall KillTimer(long long) NtUserKillTimer
443 stdcall LoadAcceleratorsA(long str)
444 stdcall LoadAcceleratorsW(long wstr)
445 stdcall LoadBitmapA(long str)
446 stdcall LoadBitmapW(long wstr)
447 stdcall LoadCursorA(long str)
448 stdcall LoadCursorFromFileA(str)
449 stdcall LoadCursorFromFileW(wstr)
450 stdcall LoadCursorW(long wstr)
451 stdcall LoadIconA(long str)
452 stdcall LoadIconW(long wstr)
453 stdcall LoadImageA(long str long long long long)
454 stdcall LoadImageW(long wstr long long long long)
455 stdcall LoadKeyboardLayoutA(str long)
456 stdcall LoadKeyboardLayoutEx(long ptr long)
457 stdcall LoadKeyboardLayoutW(wstr long)
458 stdcall LoadLocalFonts()
459 stdcall LoadMenuA(long str)
460 stdcall LoadMenuIndirectA(ptr)
461 stdcall LoadMenuIndirectW(ptr)
462 stdcall LoadMenuW(long wstr)
463 stdcall LoadRemoteFonts()
464 stdcall LoadStringA(long long ptr long)
465 stdcall LoadStringW(long long ptr long)
466 stdcall LockSetForegroundWindow (long)
467 stdcall LockWindowStation(long) NtUserLockWindowStation
468 stdcall LockWindowUpdate(long) NtUserLockWindowUpdate
469 stdcall LockWorkStation() NtUserLockWorkStation
470 stdcall LookupIconIdFromDirectory(ptr long)
471 stdcall LookupIconIdFromDirectoryEx(ptr long long long long)
472 stdcall MBToWCSEx(long str long wstr long long)
473 stdcall MB_GetString(ptr)
474 stdcall MapDialogRect(long ptr)
475 stdcall MapVirtualKeyA(long long)
476 stdcall MapVirtualKeyExA(long long long)
477 stdcall MapVirtualKeyExW(long long long)
478 stdcall MapVirtualKeyW(long long)
479 stdcall MapWindowPoints(long long ptr long)
480 stdcall MenuItemFromPoint(long long double) NtUserMenuItemFromPoint
481 stdcall MenuWindowProcA (long ptr long long long)
482 stdcall MenuWindowProcW (long ptr long long long)
483 stdcall MessageBeep(long)
484 stdcall MessageBoxA(long str str long)
485 stdcall MessageBoxExA(long str str long long)
486 stdcall MessageBoxExW(long wstr wstr long long)
487 stdcall MessageBoxIndirectA(ptr)
488 stdcall MessageBoxIndirectW(ptr)
489 stdcall MessageBoxTimeoutA(ptr str str long long long)
490 stdcall MessageBoxTimeoutW(ptr wstr wstr long long long)
491 stdcall MessageBoxW(long wstr wstr long)
492 stdcall ModifyMenuA(long long long long ptr)
493 stdcall ModifyMenuW(long long long long ptr)
494 stdcall MonitorFromPoint(double long)
495 stdcall MonitorFromRect(ptr long)
496 stdcall MonitorFromWindow(long long)
497 stdcall MoveWindow(long long long long long long) NtUserMoveWindow
498 stdcall MsgWaitForMultipleObjects(long ptr long long long)
499 stdcall MsgWaitForMultipleObjectsEx(long ptr long long long)
500 stdcall NotifyWinEvent(long long long long)
501 stdcall OemKeyScan(long)
502 stdcall OemToCharA(ptr ptr)
503 stdcall OemToCharBuffA(ptr ptr long)
504 stdcall OemToCharBuffW(ptr ptr long)
505 stdcall OemToCharW(ptr ptr)
506 stdcall OffsetRect(ptr long long)
507 stdcall OpenClipboard(long)
508 stdcall OpenDesktopA(str long long long)
509 stdcall OpenDesktopW(wstr long long long)
510 stdcall OpenIcon(long)
511 stdcall OpenInputDesktop(long long long) NtUserOpenInputDesktop
512 stdcall OpenWindowStationA(str long long)
513 stdcall OpenWindowStationW(wstr long long)
514 stdcall PackDDElParam(long long long)
515 stdcall PaintDesktop(long) NtUserPaintDesktop
516 stdcall PaintMenuBar(long long long long long long) NtUserPaintMenuBar
517 stdcall PeekMessageA(ptr long long long long)
518 stdcall PeekMessageW(ptr long long long long)
519 stdcall PostMessageA(long long long long)
520 stdcall PostMessageW(long long long long)
521 stdcall PostQuitMessage(long)
522 stdcall PostThreadMessageA(long long long long)
523 stdcall PostThreadMessageW(long long long long)
524 stdcall PrintWindow(ptr ptr long) NtUserPrintWindow
525 stdcall PrivateExtractIconExA(str long ptr ptr long)
526 stdcall PrivateExtractIconExW(wstr long ptr ptr long)
527 stdcall PrivateExtractIconsA(str long long long ptr ptr long long)
528 stdcall PrivateExtractIconsW(wstr long long long ptr ptr long long)
# PrivateSetDbgTag
# PrivateSetRipFlags
531 stdcall PtInRect(ptr double)
532 stdcall QuerySendMessage(ptr) NtUserQuerySendMessage
533 stdcall RealChildWindowFromPoint(long double) ; Direct call NtUserRealChildWindowFromPoint
534 stdcall RealGetWindowClass(long ptr long) RealGetWindowClassA
535 stdcall RealGetWindowClassA(long ptr long)
536 stdcall RealGetWindowClassW(long ptr long)
537 stdcall ReasonCodeNeedsBugID(long)
538 stdcall ReasonCodeNeedsComment(long)
539 stdcall RecordShutdownReason(long)
540 stdcall RedrawWindow(long ptr long long) NtUserRedrawWindow
541 stdcall RegisterClassA(ptr)
542 stdcall RegisterClassExA(ptr)
543 stdcall RegisterClassExW(ptr)
544 stdcall RegisterClassW(ptr)
545 stdcall RegisterClipboardFormatA(str)
546 stdcall RegisterClipboardFormatW(wstr)
547 stdcall RegisterDeviceNotificationA(long ptr long) RegisterDeviceNotificationW
548 stdcall RegisterDeviceNotificationW(long ptr long)
549 stdcall RegisterHotKey(long long long long) NtUserRegisterHotKey
550 stdcall RegisterLogonProcess(long long)
551 stdcall RegisterMessagePumpHook(ptr)
552 stdcall RegisterRawInputDevices(ptr long long)
553 stdcall RegisterServicesProcess(long)
554 stdcall RegisterShellHookWindow(long)
555 stdcall RegisterSystemThread(long long)
556 stdcall RegisterTasklist(long) NtUserRegisterTasklist
557 stdcall RegisterUserApiHook(ptr)
558 stdcall RegisterWindowMessageA(str)
559 stdcall RegisterWindowMessageW(wstr)
560 stdcall ReleaseCapture()
561 stdcall ReleaseDC(long long)
562 stdcall RemoveMenu(long long long) NtUserRemoveMenu
563 stdcall RemovePropA(long str)
564 stdcall RemovePropW(long wstr)
565 stdcall ReplyMessage(long)
# ResolveDesktopForWOW
567 stdcall ReuseDDElParam(long long long long long)
568 stdcall ScreenToClient(long ptr)
569 stdcall ScrollChildren(long long long long)
570 stdcall ScrollDC(long long long ptr ptr long ptr)
571 stdcall ScrollWindow(long long long ptr ptr)
572 stdcall ScrollWindowEx(long long long ptr ptr long ptr long)
573 stdcall SendDlgItemMessageA(long long long long long)
574 stdcall SendDlgItemMessageW(long long long long long)
575 stdcall SendIMEMessageExA(long long)
576 stdcall SendIMEMessageExW(long long)
577 stdcall SendInput(long ptr long) NtUserSendInput
578 stdcall SendMessageA(long long long long)
579 stdcall SendMessageCallbackA(long long long long ptr long)
580 stdcall SendMessageCallbackW(long long long long ptr long)
581 stdcall SendMessageTimeoutA(long long long long long long ptr)
582 stdcall SendMessageTimeoutW(long long long long long long ptr)
583 stdcall SendMessageW(long long long long)
584 stdcall SendNotifyMessageA(long long long long)
585 stdcall SendNotifyMessageW(long long long long)
586 stdcall SetActiveWindow(long) NtUserSetActiveWindow
587 stdcall SetCapture(long) NtUserSetCapture
588 stdcall SetCaretBlinkTime(long)
589 stdcall SetCaretPos(long long)
590 stdcall SetClassLongA(long long long)
@ stdcall -arch=x86_64 SetClassLongPtrA(ptr long ptr)
@ stdcall -arch=x86_64 SetClassLongPtrW(ptr long ptr)
591 stdcall SetClassLongW(long long long)
592 stdcall SetClassWord(long long long) ; Direct call NtUserSetClassWord
593 stdcall SetClipboardData(long long)
594 stdcall SetClipboardViewer(long) NtUserSetClipboardViewer
595 stdcall SetConsoleReserveKeys(long long) NtUserSetConsoleReserveKeys
596 stdcall SetCursor(long) NtUserSetCursor
597 stdcall SetCursorContents(ptr ptr) NtUserSetCursorContents
598 stdcall SetCursorPos(long long)
# SetDbgTag
600 stdcall SetDebugErrorLevel(long)
601 stdcall SetDeskWallpaper(ptr)
602 stdcall SetDlgItemInt(long long long long)
603 stdcall SetDlgItemTextA(long long str)
604 stdcall SetDlgItemTextW(long long wstr)
605 stdcall SetDoubleClickTime(long)
606 stdcall SetFocus(long) NtUserSetFocus
607 stdcall SetForegroundWindow(long)
608 stdcall SetInternalWindowPos(long long ptr ptr) NtUserSetInternalWindowPos
609 stdcall SetKeyboardState(ptr) NtUserSetKeyboardState
610 stdcall SetLastErrorEx(long long)
611 stdcall SetLayeredWindowAttributes(ptr long long long) NtUserSetLayeredWindowAttributes
612 stdcall SetLogonNotifyWindow(long) ; Direct call NtUserSetLogonNotifyWindow
613 stdcall SetMenu(long long)
614 stdcall SetMenuContextHelpId(long long) NtUserSetMenuContextHelpId
615 stdcall SetMenuDefaultItem(long long long) NtUserSetMenuDefaultItem
616 stdcall SetMenuInfo(long ptr)
617 stdcall SetMenuItemBitmaps(long long long long long)
618 stdcall SetMenuItemInfoA(long long long ptr)
619 stdcall SetMenuItemInfoW(long long long ptr)
620 stdcall SetMessageExtraInfo(long)
621 stdcall SetMessageQueue(long)
622 stdcall SetParent(long long) NtUserSetParent
623 stdcall SetProcessDefaultLayout(long)
624 stdcall SetProcessWindowStation(long) NtUserSetProcessWindowStation
625 stdcall SetProgmanWindow (long)
626 stdcall SetPropA(long str long)
627 stdcall SetPropW(long wstr long)
628 stdcall SetRect(ptr long long long long)
629 stdcall SetRectEmpty(ptr)
# SetRipFlags
631 stdcall SetScrollInfo(long long ptr long) ; Direct call NtUserSetScrollInfo
632 stdcall SetScrollPos(long long long long)
633 stdcall SetScrollRange(long long long long long)
634 stdcall SetShellWindow(long)
635 stdcall SetShellWindowEx(long long) NtUserSetShellWindowEx
636 stdcall SetSysColors(long ptr ptr)
637 stdcall SetSysColorsTemp(ptr ptr long)
638 stdcall SetSystemCursor(long long)
639 stdcall SetSystemMenu(long long) ; NtUserSetSystemMenu
640 stdcall SetSystemTimer(long long long ptr) NtUserSetSystemTimer
641 stdcall SetTaskmanWindow (long)
642 stdcall SetThreadDesktop(long) NtUserSetThreadDesktop
643 stdcall SetTimer(long long long ptr) NtUserSetTimer
644 stdcall SetUserObjectInformationA(long long ptr long) NtUserSetObjectInformation
645 stdcall SetUserObjectInformationW(long long ptr long) NtUserSetObjectInformation
646 stdcall SetUserObjectSecurity(long ptr ptr)
647 stdcall SetWinEventHook(long long long ptr long long long)
648 stdcall SetWindowContextHelpId(long long)
649 stdcall SetWindowLongA(long long long)
@ stdcall -arch=x86_64 SetWindowLongPtrA(ptr long ptr)
@ stdcall -arch=x86_64 SetWindowLongPtrW(ptr long ptr)
650 stdcall SetWindowLongW(long long long)
651 stdcall SetWindowPlacement(long ptr) NtUserSetWindowPlacement
652 stdcall SetWindowPos(long long long long long long long) NtUserSetWindowPos
653 stdcall SetWindowRgn(long long long)
654 stdcall SetWindowStationUser(long long long long)
655 stdcall SetWindowTextA(long str)
656 stdcall SetWindowTextW(long wstr)
657 stdcall SetWindowWord(long long long) ; Direct call NtUserSetWindowWord
658 stdcall SetWindowsHookA(long ptr)
659 stdcall SetWindowsHookExA(long long long long)
660 stdcall SetWindowsHookExW(long long long long)
661 stdcall SetWindowsHookW(long ptr)
662 stdcall ShowCaret(long) NtUserShowCaret
663 stdcall ShowCursor(long)
664 stdcall ShowOwnedPopups(long long)
665 stdcall ShowScrollBar(long long long) NtUserShowScrollBar
666 stdcall ShowStartGlass(long)
667 stdcall ShowWindow(long long) NtUserShowWindow
668 stdcall ShowWindowAsync(long long) NtUserShowWindowAsync
669 stdcall SoftModalMessageBox(ptr)
670 stdcall SubtractRect(ptr ptr ptr)
671 stdcall SwapMouseButton(long)
672 stdcall SwitchDesktop(long) NtUserSwitchDesktop
673 stdcall SwitchToThisWindow(long long)
674 stdcall SystemParametersInfoA(long long ptr long)
675 stdcall SystemParametersInfoW(long long ptr long)
676 stdcall TabbedTextOutA(long long long str long long ptr long)
677 stdcall TabbedTextOutW(long long long wstr long long ptr long)
678 stdcall TileChildWindows(long long)
679 stdcall TileWindows(long long ptr long ptr)
680 stdcall ToAscii(long long ptr ptr long)
681 stdcall ToAsciiEx(long long ptr ptr long long)
682 stdcall ToUnicode(long long ptr ptr long long)
683 stdcall ToUnicodeEx(long long ptr ptr long long long)
684 stdcall TrackMouseEvent(ptr) NtUserTrackMouseEvent
685 stdcall TrackPopupMenu(long long long long long long ptr)
686 stdcall TrackPopupMenuEx(long long long long long ptr) NtUserTrackPopupMenuEx
687 stdcall TranslateAccelerator(long long ptr) TranslateAcceleratorA
688 stdcall TranslateAcceleratorA(long long ptr)
689 stdcall TranslateAcceleratorW(long long ptr)
690 stdcall TranslateMDISysAccel(long ptr)
691 stdcall TranslateMessage(ptr)
692 stdcall TranslateMessageEx(ptr long)
693 stdcall UnhookWinEvent(long) NtUserUnhookWinEvent
694 stdcall UnhookWindowsHook(long ptr)
695 stdcall UnhookWindowsHookEx(long) NtUserUnhookWindowsHookEx
696 stdcall UnionRect(ptr ptr ptr)
697 stdcall UnloadKeyboardLayout(long) NtUserUnloadKeyboardLayout
698 stdcall UnlockWindowStation(long) NtUserUnlockWindowStation
699 stdcall UnpackDDElParam(long long ptr ptr)
700 stdcall UnregisterClassA(str long)
701 stdcall UnregisterClassW(wstr long)
702 stdcall UnregisterDeviceNotification(long)
703 stdcall UnregisterHotKey(long long) NtUserUnregisterHotKey
704 stdcall UnregisterMessagePumpHook()
705 stdcall UnregisterUserApiHook() NtUserUnregisterUserApiHook
706 stdcall UpdateLayeredWindow(long long ptr ptr long ptr long ptr long)
707 stdcall UpdateLayeredWindowIndirect(long ptr)
708 stdcall UpdatePerUserSystemParameters(long long)
709 stdcall UpdateWindow(long)
710 stdcall User32InitializeImmEntryTable(ptr)
711 stdcall UserClientDllInitialize(long long ptr) DllMain
712 stdcall UserHandleGrantAccess(ptr ptr long) NtUserUserHandleGrantAccess
713 stdcall UserLpkPSMTextOut(long long long wstr long long)
714 stdcall UserLpkTabbedTextOut(long long long long long long long long long long long long)
715 stdcall UserRealizePalette(long)
716 stdcall UserRegisterWowHandlers(ptr ptr)
# VRipOutput
# VTagOutput
719 stdcall ValidateRect(long ptr) NtUserValidateRect
720 stdcall ValidateRgn(long long)
721 stdcall VkKeyScanA(long)
722 stdcall VkKeyScanExA(long long)
723 stdcall VkKeyScanExW(long long)
724 stdcall VkKeyScanW(long)
725 stdcall WCSToMBEx(long wstr long str long long)
726 stdcall WINNLSEnableIME(long long)
727 stdcall WINNLSGetEnableStatus(long)
728 stdcall WINNLSGetIMEHotkey(long)
729 stdcall WaitForInputIdle(long long)
730 stdcall WaitMessage() NtUserWaitMessage
731 stdcall Win32PoolAllocationStats(long long long long long)
732 stdcall WinHelpA(long str long long)
733 stdcall WinHelpW(long wstr long long)
734 stdcall WindowFromDC(long)
735 stdcall WindowFromPoint(double)
736 stdcall keybd_event(long long long long)
737 stdcall mouse_event(long long long long long)
738 varargs wsprintfA(ptr str)
739 varargs wsprintfW(ptr wstr)
740 stdcall wvsprintfA(ptr str ptr)
741 stdcall wvsprintfW(ptr wstr ptr)
