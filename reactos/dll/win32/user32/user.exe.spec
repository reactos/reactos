1   pascal -ret16 MessageBox(word str str word) MessageBox16
2   stub OldExitWindows
3   stub EnableOEMLayer
4   stub DisableOEMLayer
5   pascal -ret16 InitApp(word) InitApp16
6   pascal -ret16 PostQuitMessage(word) PostQuitMessage16
7   pascal -ret16 ExitWindows(long word) ExitWindows16
10  pascal -ret16 SetTimer(word word word segptr) SetTimer16
11  pascal -ret16 SetSystemTimer(word word word segptr) SetSystemTimer16 # BEAR11
12  pascal -ret16 KillTimer(word word) KillTimer16
13  pascal   GetTickCount() GetTickCount
14  pascal   GetTimerResolution() GetTimerResolution16
# GetCurrentTime is effectively identical to GetTickCount
15  pascal   GetCurrentTime() GetTickCount
16  pascal -ret16 ClipCursor(ptr) ClipCursor16
17  pascal -ret16 GetCursorPos(ptr) GetCursorPos16
18  pascal -ret16 SetCapture(word) SetCapture16
19  pascal -ret16 ReleaseCapture() ReleaseCapture16
20  pascal -ret16 SetDoubleClickTime(word) SetDoubleClickTime16
21  pascal -ret16 GetDoubleClickTime() GetDoubleClickTime16
22  pascal -ret16 SetFocus(word) SetFocus16
23  pascal -ret16 GetFocus() GetFocus16
24  pascal -ret16 RemoveProp(word ptr) RemoveProp16
25  pascal -ret16 GetProp(word str) GetProp16
26  pascal -ret16 SetProp(word str word) SetProp16
27  pascal -ret16 EnumProps(word segptr) EnumProps16
28  pascal -ret16 ClientToScreen(word ptr) ClientToScreen16
29  pascal -ret16 ScreenToClient(word ptr) ScreenToClient16
30  pascal -ret16 WindowFromPoint(long) WindowFromPoint16
31  pascal -ret16 IsIconic(word) IsIconic16
32  pascal -ret16 GetWindowRect(word ptr) GetWindowRect16
33  pascal -ret16 GetClientRect(word ptr) GetClientRect16
34  pascal -ret16 EnableWindow(word word) EnableWindow16
35  pascal -ret16 IsWindowEnabled(word) IsWindowEnabled16
36  pascal -ret16 GetWindowText(word segptr word) GetWindowText16
37  pascal -ret16 SetWindowText(word segstr) SetWindowText16
38  pascal -ret16 GetWindowTextLength(word) GetWindowTextLength16
39  pascal -ret16 BeginPaint(word ptr) BeginPaint16
40  pascal -ret16 EndPaint(word ptr) EndPaint16
41  pascal -ret16 CreateWindow(str str long s_word s_word s_word s_word word word word segptr) CreateWindow16
42  pascal -ret16 ShowWindow(word word) ShowWindow16
43  pascal -ret16 CloseWindow(word) CloseWindow16
44  pascal -ret16 OpenIcon(word) OpenIcon16
45  pascal -ret16 BringWindowToTop(word) BringWindowToTop16
46  pascal -ret16 GetParent(word) GetParent16
47  pascal -ret16 IsWindow(word) IsWindow16
48  pascal -ret16 IsChild(word word) IsChild16
49  pascal -ret16 IsWindowVisible(word) IsWindowVisible16
50  pascal -ret16 FindWindow(str str) FindWindow16
51  stub BEAR51 # IsTwoByteCharPrefix
52  pascal -ret16 AnyPopup() AnyPopup16
53  pascal -ret16 DestroyWindow(word) DestroyWindow16
54  pascal -ret16 EnumWindows(segptr long) EnumWindows16
55  pascal -ret16 EnumChildWindows(word segptr long) EnumChildWindows16
56  pascal -ret16 MoveWindow(word word word word word word) MoveWindow16
57  pascal -ret16 RegisterClass(ptr) RegisterClass16
58  pascal -ret16 GetClassName(word ptr word) GetClassName16
59  pascal -ret16 SetActiveWindow(word) SetActiveWindow16
60  pascal -ret16 GetActiveWindow() GetActiveWindow16
61  pascal -ret16 ScrollWindow(word s_word s_word ptr ptr) ScrollWindow16
62  pascal -ret16 SetScrollPos(word word s_word word) SetScrollPos16
63  pascal -ret16 GetScrollPos(word word) GetScrollPos16
64  pascal -ret16 SetScrollRange(word word s_word s_word word) SetScrollRange16
65  pascal -ret16 GetScrollRange(word word ptr ptr) GetScrollRange16
66  pascal -ret16 GetDC(word) GetDC16
67  pascal -ret16 GetWindowDC(word) GetWindowDC16
68  pascal -ret16 ReleaseDC(word word) ReleaseDC16
69  pascal -ret16 SetCursor(word) SetCursor16
70  pascal -ret16 SetCursorPos(word word) SetCursorPos16
71  pascal -ret16 ShowCursor(word) ShowCursor16
72  pascal -ret16 SetRect(ptr s_word s_word s_word s_word) SetRect16
73  pascal -ret16 SetRectEmpty(ptr) SetRectEmpty16
74  pascal -ret16 CopyRect(ptr ptr) CopyRect16
75  pascal -ret16 IsRectEmpty(ptr) IsRectEmpty16
76  pascal -ret16 PtInRect(ptr long) PtInRect16
77  pascal -ret16 OffsetRect(ptr s_word s_word) OffsetRect16
78  pascal -ret16 InflateRect(ptr s_word s_word) InflateRect16
79  pascal -ret16 IntersectRect(ptr ptr ptr) IntersectRect16
80  pascal -ret16 UnionRect(ptr ptr ptr) UnionRect16
81  pascal -ret16 FillRect(word ptr word) FillRect16
82  pascal -ret16 InvertRect(word ptr) InvertRect16
83  pascal -ret16 FrameRect(word ptr word) FrameRect16
84  pascal -ret16 DrawIcon(word s_word s_word word) DrawIcon16
85  pascal -ret16 DrawText(word str s_word ptr word) DrawText16
86  pascal   IconSize() IconSize16 # later versions: BEAR86
87  pascal -ret16 DialogBox(word str word segptr) DialogBox16
88  pascal -ret16 EndDialog(word s_word) EndDialog16
89  pascal -ret16 CreateDialog(word str word segptr) CreateDialog16
90  pascal -ret16 IsDialogMessage(word ptr) IsDialogMessage16
91  pascal -ret16 GetDlgItem(word word) GetDlgItem16
92  pascal -ret16 SetDlgItemText(word word segstr) SetDlgItemText16
93  pascal -ret16 GetDlgItemText(word word segptr word) GetDlgItemText16
94  pascal -ret16 SetDlgItemInt(word word word word) SetDlgItemInt16
95  pascal -ret16 GetDlgItemInt(word s_word ptr word) GetDlgItemInt16
96  pascal -ret16 CheckRadioButton(word word word word) CheckRadioButton16
97  pascal -ret16 CheckDlgButton(word word word) CheckDlgButton16
98  pascal -ret16 IsDlgButtonChecked(word word) IsDlgButtonChecked16
99  pascal -ret16 DlgDirSelect(word ptr word) DlgDirSelect16
100 pascal -ret16 DlgDirList(word str word word word) DlgDirList16
101 pascal   SendDlgItemMessage(word word word word long) SendDlgItemMessage16
102 pascal -ret16 AdjustWindowRect(ptr long word) AdjustWindowRect16
103 pascal -ret16 MapDialogRect(word ptr) MapDialogRect16
104 pascal -ret16 MessageBeep(word) MessageBeep16
105 pascal -ret16 FlashWindow(word word) FlashWindow16
106 pascal -ret16 GetKeyState(word) GetKeyState16
107 pascal   DefWindowProc(word word word long) DefWindowProc16
108 pascal -ret16 GetMessage(ptr word word word) GetMessage16
109 pascal -ret16 PeekMessage(ptr word word word word) PeekMessage16
110 pascal -ret16 PostMessage(word word word long) PostMessage16
111 pascal   SendMessage(word word word long) SendMessage16
112 pascal -ret16 WaitMessage() WaitMessage
113 pascal -ret16 TranslateMessage(ptr) TranslateMessage16
114 pascal   DispatchMessage(ptr) DispatchMessage16
115 pascal -ret16 ReplyMessage(long) ReplyMessage16
116 pascal -ret16 PostAppMessage(word word word long) PostAppMessage16
117 pascal -ret16 WindowFromDC(word) WindowFromDC16 # not in W1.1, W2.0
118 pascal -ret16 RegisterWindowMessage(str) RegisterWindowMessageA
119 pascal   GetMessagePos() GetMessagePos
120 pascal   GetMessageTime() GetMessageTime
121 pascal   SetWindowsHook(s_word segptr) SetWindowsHook16
122 pascal   CallWindowProc(segptr word word word long) CallWindowProc16
123 pascal -ret16 CallMsgFilter(ptr s_word) CallMsgFilter16
124 pascal -ret16 UpdateWindow(word) UpdateWindow16
125 pascal -ret16 InvalidateRect(word ptr word) InvalidateRect16
126 pascal -ret16 InvalidateRgn(word word word) InvalidateRgn16
127 pascal -ret16 ValidateRect(word ptr) ValidateRect16
128 pascal -ret16 ValidateRgn(word word) ValidateRgn16
129 pascal -ret16 GetClassWord(word s_word) GetClassWord16
130 pascal -ret16 SetClassWord(word s_word word) SetClassWord16
131 pascal   GetClassLong(word s_word) GetClassLong16
132 pascal   SetClassLong(word s_word long) SetClassLong16
133 pascal -ret16 GetWindowWord(word s_word) GetWindowWord16
134 pascal -ret16 SetWindowWord(word s_word word) SetWindowWord16
135 pascal   GetWindowLong(word s_word) GetWindowLong16
136 pascal   SetWindowLong(word s_word long) SetWindowLong16
137 pascal -ret16 OpenClipboard(word) OpenClipboard16
138 pascal -ret16 CloseClipboard() CloseClipboard16
139 pascal -ret16 EmptyClipboard() EmptyClipboard16
140 pascal -ret16 GetClipboardOwner() GetClipboardOwner16
141 pascal -ret16 SetClipboardData(word word) SetClipboardData16
142 pascal -ret16 GetClipboardData(word) GetClipboardData16
143 pascal -ret16 CountClipboardFormats() CountClipboardFormats16
144 pascal -ret16 EnumClipboardFormats(word) EnumClipboardFormats16
145 pascal -ret16 RegisterClipboardFormat(ptr) RegisterClipboardFormat16
146 pascal -ret16 GetClipboardFormatName(word ptr s_word) GetClipboardFormatName16
147 pascal -ret16 SetClipboardViewer(word) SetClipboardViewer16
148 pascal -ret16 GetClipboardViewer() GetClipboardViewer16
149 pascal -ret16 ChangeClipboardChain(word word) ChangeClipboardChain16
150 pascal -ret16 LoadMenu(word str) LoadMenu16
151 pascal -ret16 CreateMenu() CreateMenu16
152 pascal -ret16 DestroyMenu(word) DestroyMenu16
153 pascal -ret16 ChangeMenu(word word segstr word word) ChangeMenu16
154 pascal -ret16 CheckMenuItem(word word word) CheckMenuItem16
155 pascal -ret16 EnableMenuItem(word word word) EnableMenuItem16
156 pascal -ret16 GetSystemMenu(word word) GetSystemMenu16
157 pascal -ret16 GetMenu(word) GetMenu16
158 pascal -ret16 SetMenu(word word) SetMenu16
159 pascal -ret16 GetSubMenu(word word) GetSubMenu16
160 pascal -ret16 DrawMenuBar(word) DrawMenuBar16
161 pascal -ret16 GetMenuString(word word ptr s_word word) GetMenuString16
162 pascal -ret16 HiliteMenuItem(word word word word) HiliteMenuItem16
163 pascal -ret16 CreateCaret(word word word word) CreateCaret16
164 pascal -ret16 DestroyCaret() DestroyCaret16
165 pascal -ret16 SetCaretPos(word word) SetCaretPos16
166 pascal -ret16 HideCaret(word) HideCaret16
167 pascal -ret16 ShowCaret(word) ShowCaret16
168 pascal -ret16 SetCaretBlinkTime(word) SetCaretBlinkTime16
169 pascal -ret16 GetCaretBlinkTime() GetCaretBlinkTime16
170 pascal -ret16 ArrangeIconicWindows(word) ArrangeIconicWindows16 # W1.1: CREATECONVERTWINDOW, W2.0: nothing !
171 pascal -ret16 WinHelp(word str word long) WinHelp16 # W1.1: SHOWCONVERTWINDOW, W2.0: nothing !
172 pascal -ret16 SwitchToThisWindow(word word) SwitchToThisWindow16 # W1.1: SETCONVERTWINDOWHEIGHT, W2.0: nothing !
173 pascal -ret16 LoadCursor(word str) LoadCursor16
174 pascal -ret16 LoadIcon(word str) LoadIcon16
175 pascal -ret16 LoadBitmap(word str) LoadBitmap16
176 pascal -ret16 LoadString(word word ptr s_word) LoadString16
177 pascal -ret16 LoadAccelerators(word str) LoadAccelerators16
178 pascal -ret16 TranslateAccelerator(word word ptr) TranslateAccelerator16
179 pascal -ret16 GetSystemMetrics(s_word) GetSystemMetrics16
180 pascal   GetSysColor(word) GetSysColor16
181 pascal -ret16 SetSysColors(word ptr ptr) SetSysColors16
182 pascal -ret16 KillSystemTimer(word word) KillSystemTimer16 # BEAR182
183 pascal -ret16 GetCaretPos(ptr) GetCaretPos16
184 stub QuerySendMessage # W1.1, W2.0: SYSHASKANJI
185 pascal -ret16 GrayString(word word segptr segptr s_word s_word s_word s_word s_word) GrayString16
186 pascal -ret16 SwapMouseButton(word) SwapMouseButton16
187 pascal -ret16 EndMenu() EndMenu
188 pascal -ret16 SetSysModalWindow(word) SetSysModalWindow16
189 pascal -ret16 GetSysModalWindow() GetSysModalWindow16
190 pascal -ret16 GetUpdateRect(word ptr word) GetUpdateRect16
191 pascal -ret16 ChildWindowFromPoint(word long) ChildWindowFromPoint16
192 pascal -ret16 InSendMessage() InSendMessage16
193 pascal -ret16 IsClipboardFormatAvailable(word) IsClipboardFormatAvailable16
194 pascal -ret16 DlgDirSelectComboBox(word ptr word) DlgDirSelectComboBox16
195 pascal -ret16 DlgDirListComboBox(word ptr word word word) DlgDirListComboBox16
196 pascal   TabbedTextOut(word s_word s_word ptr s_word s_word ptr s_word) TabbedTextOut16
197 pascal   GetTabbedTextExtent(word ptr word word ptr) GetTabbedTextExtent16
198 pascal -ret16 CascadeChildWindows(word word) CascadeChildWindows16
199 pascal -ret16 TileChildWindows(word word) TileChildWindows16
200 pascal -ret16 OpenComm(str word word) OpenComm16
201 pascal -ret16 SetCommState(ptr) SetCommState16
202 pascal -ret16 GetCommState(word ptr) GetCommState16
203 pascal -ret16 GetCommError(word ptr) GetCommError16
204 pascal -ret16 ReadComm(word ptr word) ReadComm16
205 pascal -ret16 WriteComm(word ptr word) WriteComm16
206 pascal -ret16 TransmitCommChar(word word) TransmitCommChar16
207 pascal -ret16 CloseComm(word) CloseComm16
208 pascal   SetCommEventMask(word word) SetCommEventMask16
209 pascal -ret16 GetCommEventMask(word word) GetCommEventMask16
210 pascal -ret16 SetCommBreak(word) SetCommBreak16
211 pascal -ret16 ClearCommBreak(word) ClearCommBreak16
212 pascal -ret16 UngetCommChar(word word) UngetCommChar16
213 pascal -ret16 BuildCommDCB(ptr ptr) BuildCommDCB16
214 pascal   EscapeCommFunction(word word) EscapeCommFunction16
215 pascal -ret16 FlushComm(word word) FlushComm16
216 pascal   UserSeeUserDo(word word word word) UserSeeUserDo16 # W1.1, W2.0: MYOPENCOMM
#217-299 not in W1.1
217 pascal -ret16 LookupMenuHandle(word s_word) LookupMenuHandle16
218 pascal -ret16 DialogBoxIndirect(word word word segptr) DialogBoxIndirect16
219 pascal -ret16 CreateDialogIndirect(word ptr word segptr) CreateDialogIndirect16
220 pascal -ret16 LoadMenuIndirect(ptr) LoadMenuIndirect16
221 pascal -ret16 ScrollDC(word s_word s_word ptr ptr word ptr) ScrollDC16
222 pascal -ret16 GetKeyboardState(ptr) GetKeyboardState16
223 pascal -ret16 SetKeyboardState(ptr) SetKeyboardState16
224 pascal -ret16 GetWindowTask(word) GetWindowTask16
225 pascal -ret16 EnumTaskWindows(word segptr long) EnumTaskWindows16
226 stub LockInput # not in W2.0
227 pascal -ret16 GetNextDlgGroupItem(word word word) GetNextDlgGroupItem16
228 pascal -ret16 GetNextDlgTabItem(word word word) GetNextDlgTabItem16
229 pascal -ret16 GetTopWindow(word) GetTopWindow16
230 pascal -ret16 GetNextWindow(word word) GetNextWindow16
231 pascal -ret16 GetSystemDebugState() GetSystemDebugState16
232 pascal -ret16 SetWindowPos(word word word word word word word) SetWindowPos16
233 pascal -ret16 SetParent(word word) SetParent16
234 pascal -ret16 UnhookWindowsHook(s_word segptr) UnhookWindowsHook16
235 pascal   DefHookProc(s_word word long ptr) DefHookProc16
236 pascal -ret16 GetCapture() GetCapture16
237 pascal -ret16 GetUpdateRgn(word word word) GetUpdateRgn16
238 pascal -ret16 ExcludeUpdateRgn(word word) ExcludeUpdateRgn16
239 pascal -ret16 DialogBoxParam(word str word segptr long) DialogBoxParam16
240 pascal -ret16 DialogBoxIndirectParam(word word word segptr long) DialogBoxIndirectParam16
241 pascal -ret16 CreateDialogParam(word str word segptr long) CreateDialogParam16
242 pascal -ret16 CreateDialogIndirectParam(word ptr word segptr long) CreateDialogIndirectParam16
243 pascal   GetDialogBaseUnits() GetDialogBaseUnits
244 pascal -ret16 EqualRect(ptr ptr) EqualRect16
245 pascal -ret16 EnableCommNotification(s_word word s_word s_word) EnableCommNotification16
246 pascal -ret16 ExitWindowsExec(str str) ExitWindowsExec16
247 pascal -ret16 GetCursor() GetCursor16
248 pascal -ret16 GetOpenClipboardWindow() GetOpenClipboardWindow16
249 pascal -ret16 GetAsyncKeyState(word) GetAsyncKeyState16
250 pascal -ret16 GetMenuState(word word word) GetMenuState16
251 pascal   SendDriverMessage(word word long long) SendDriverMessage16
252 pascal -ret16 OpenDriver(str str long) OpenDriver16
253 pascal   CloseDriver(word long long) CloseDriver16
254 pascal -ret16 GetDriverModuleHandle(word) GetDriverModuleHandle16
255 pascal   DefDriverProc(long word word long long) DefDriverProc16
256 pascal -ret16 GetDriverInfo(word ptr) GetDriverInfo16
257 pascal -ret16 GetNextDriver(word long) GetNextDriver16
258 pascal -ret16 MapWindowPoints(word word ptr word) MapWindowPoints16
259 pascal -ret16 BeginDeferWindowPos(s_word) BeginDeferWindowPos16
260 pascal -ret16 DeferWindowPos(word word word s_word s_word s_word s_word word) DeferWindowPos16
261 pascal -ret16 EndDeferWindowPos(word) EndDeferWindowPos16
262 pascal -ret16 GetWindow(word word) GetWindow16
263 pascal -ret16 GetMenuItemCount(word) GetMenuItemCount16
264 pascal -ret16 GetMenuItemID(word word) GetMenuItemID16
265 pascal -ret16 ShowOwnedPopups(word word) ShowOwnedPopups16
266 pascal -ret16 SetMessageQueue(word) SetMessageQueue16
267 pascal -ret16 ShowScrollBar(word word word) ShowScrollBar16
268 pascal -ret16 GlobalAddAtom(str) GlobalAddAtom16
269 pascal -ret16 GlobalDeleteAtom(word) GlobalDeleteAtom16
270 pascal -ret16 GlobalFindAtom(str) GlobalFindAtom16
271 pascal -ret16 GlobalGetAtomName(word ptr s_word) GlobalGetAtomName16
272 pascal -ret16 IsZoomed(word) IsZoomed16
273 pascal -ret16 ControlPanelInfo(word word str) ControlPanelInfo16
274 stub GetNextQueueWindow
275 stub RepaintScreen
276 stub LockMyTask
277 pascal -ret16 GetDlgCtrlID(word) GetDlgCtrlID16
278 pascal -ret16 GetDesktopHwnd() GetDesktopHwnd16
279 pascal -ret16 OldSetDeskPattern() SetDeskPattern16
280 pascal -ret16 SetSystemMenu(word word) SetSystemMenu16
281 pascal -ret16 GetSysColorBrush(word) GetSysColorBrush16
282 pascal -ret16 SelectPalette(word word word) SelectPalette16
283 pascal -ret16 RealizePalette(word) RealizePalette16
284 pascal -ret16 GetFreeSystemResources(word) GetFreeSystemResources16
285 pascal -ret16 SetDeskWallPaper(ptr) SetDeskWallPaper16 # BEAR285
286 pascal -ret16 GetDesktopWindow() GetDesktopWindow16
287 pascal -ret16 GetLastActivePopup(word) GetLastActivePopup16
288 pascal   GetMessageExtraInfo() GetMessageExtraInfo
289 pascal -register keybd_event() keybd_event16
290 pascal -ret16 RedrawWindow(word ptr word word) RedrawWindow16
291 pascal   SetWindowsHookEx(s_word segptr word word) SetWindowsHookEx16
292 pascal -ret16 UnhookWindowsHookEx(segptr) UnhookWindowsHookEx16
293 pascal   CallNextHookEx(segptr s_word word long) CallNextHookEx16
294 pascal -ret16 LockWindowUpdate(word) LockWindowUpdate16
299 pascal -register mouse_event() mouse_event16
300 stub UnloadInstalledDrivers # W1.1: USER_FARFRAME
301 stub EDITWNDPROC # BOZOSLIVEHERE :-))
302 stub STATICWNDPROC
303 stub BUTTONWNDPROC
304 stub SBWNDPROC
305 stub DESKTOPWNDPROC # W1.1: ICONWNDPROC
306 stub MENUWNDPROC # BEAR306
307 stub LBOXCTLWNDPROC
308 pascal   DefDlgProc(word word word long) DefDlgProc16 # W1.1, W2.0: DLGWNDPROC
309 pascal -ret16 GetClipCursor(ptr) GetClipCursor16 # W1.1, W2.0: MESSAGEBOXWNDPROC
#310 ContScroll
#311 CaretBlinkProc # W1.1
#312 SendMessage2
#313 PostMessage2
314 pascal -ret16 SignalProc(word word word word word) SignalProc16
#315 XCStoDS
#316 CompUpdateRect
#317 CompUpdateRgn
#318 GetWC2
319 pascal -ret16 ScrollWindowEx(word s_word s_word ptr ptr word ptr word) ScrollWindowEx16 # W1.1, W2.0: SETWC2
320 stub SysErrorBox # W1.1: ICONNAMEWNDPROC, W2.0: nothing !
321 pascal   SetEventHook(segptr) SetEventHook16 # W1.1, W2.0: DESTROYTASKWINDOWS2
322 stub WinOldAppHackOMatic # W1.1, W2.0: POSTSYSERROR
323 stub GetMessage2
324 pascal -ret16 FillWindow(word word word word) FillWindow16
325 pascal -ret16 PaintRect(word word word word ptr) PaintRect16
326 pascal -ret16 GetControlBrush(word word word) GetControlBrush16
#327 KillTimer2
#328 SetTimer2
#329 MenuItemState # W1.1
#330 SetGetKbdState
331 pascal -ret16 EnableHardwareInput(word) EnableHardwareInput16
332 pascal -ret16 UserYield() UserYield16
333 pascal -ret16 IsUserIdle() IsUserIdle16
334 pascal   GetQueueStatus(word) GetQueueStatus16
335 pascal -ret16 GetInputState() GetInputState16
336 pascal -ret16 LoadCursorIconHandler(word word word) LoadCursorIconHandler16
337 pascal   GetMouseEventProc() GetMouseEventProc16
338 stub ECGETDS # W2.0 (only ?)
#340 WinFarFrame
#341 _FFFE_FARFRAME
343 stub GetFilePortName
344 stub COMBOBOXCTLWNDPROC
345 stub BEAR345
#354 TabTheTextOutForWimps
#355 BroadcastMessage
356 pascal -ret16 LoadDIBCursorHandler(word word word) LoadDIBCursorHandler16
357 pascal -ret16 LoadDIBIconHandler(word word word) LoadDIBIconHandler16
358 pascal -ret16 IsMenu(word) IsMenu16
359 pascal -ret16 GetDCEx(word word long) GetDCEx16
362 pascal -ret16 DCHook(word word long long) DCHook16
364 pascal -ret16 LookupIconIdFromDirectoryEx(ptr word word word word) LookupIconIdFromDirectoryEx16
368 pascal -ret16 CopyIcon(word word) CopyIcon16
369 pascal -ret16 CopyCursor(word word) CopyCursor16
370 pascal -ret16 GetWindowPlacement(word ptr) GetWindowPlacement16
371 pascal -ret16 SetWindowPlacement(word ptr) SetWindowPlacement16
372 stub GetInternalIconHeader
373 pascal -ret16 SubtractRect(ptr ptr ptr) SubtractRect16
#374 DllEntryPoint
375 stub DrawTextEx
376 stub SetMessageExtraInfo
378 stub SetPropEx
379 stub GetPropEx
380 stub RemovePropEx
#381 stub UsrMPR_ThunkData16
382 stub SetWindowContextHelpID
383 stub GetWindowContextHelpID
384 pascal -ret16 SetMenuContextHelpId(word word) SetMenuContextHelpId16
385 pascal -ret16 GetMenuContextHelpId(word) GetMenuContextHelpId16
389 pascal   LoadImage(word str word word word word) LoadImage16
390 pascal -ret16 CopyImage(word word word word word) CopyImage16
391 pascal -ret16 SignalProc32(long long long word) UserSignalProc
394 pascal -ret16 DrawIconEx(word word word word word word word word word) DrawIconEx16
395 pascal -ret16 GetIconInfo(word ptr) GetIconInfo16
397 pascal -ret16 RegisterClassEx(ptr) RegisterClassEx16
398 pascal -ret16 GetClassInfoEx(word segstr ptr) GetClassInfoEx16
399 pascal -ret16 ChildWindowFromPointEx(word long word) ChildWindowFromPointEx16
400 pascal -ret16 FinalUserInit() FinalUserInit16
402 pascal -ret16 GetPriorityClipboardFormat(ptr s_word) GetPriorityClipboardFormat16
403 pascal -ret16 UnregisterClass(str word) UnregisterClass16
404 pascal -ret16 GetClassInfo(word segstr ptr) GetClassInfo16
406 pascal -ret16 CreateCursor(word word word word word ptr ptr) CreateCursor16
407 pascal -ret16 CreateIcon(word word word word word ptr ptr) CreateIcon16
408 pascal -ret16 CreateCursorIconIndirect(word ptr ptr ptr) CreateCursorIconIndirect16
409 pascal -ret16 InitThreadInput(word word) InitThreadInput16
410 pascal -ret16 InsertMenu(word word word word segptr) InsertMenu16
411 pascal -ret16 AppendMenu(word word word segptr) AppendMenu16
412 pascal -ret16 RemoveMenu(word word word) RemoveMenu16
413 pascal -ret16 DeleteMenu(word word word) DeleteMenu16
414 pascal -ret16 ModifyMenu(word word word word segptr) ModifyMenu16
415 pascal -ret16 CreatePopupMenu() CreatePopupMenu16
416 pascal -ret16 TrackPopupMenu(word word s_word s_word s_word word ptr) TrackPopupMenu16
417 pascal   GetMenuCheckMarkDimensions() GetMenuCheckMarkDimensions
418 pascal -ret16 SetMenuItemBitmaps(word word word word word) SetMenuItemBitmaps16
420 varargs -ret16 _wsprintf(ptr str) wsprintf16
421 pascal -ret16 wvsprintf(ptr str ptr) wvsprintf16
422 pascal -ret16 DlgDirSelectEx(word ptr word word) DlgDirSelectEx16
423 pascal -ret16 DlgDirSelectComboBoxEx(word ptr word word) DlgDirSelectComboBoxEx16
427 pascal -ret16 FindWindowEx(word word str str) FindWindowEx16
428 stub TileWindows
429 stub CascadeWindows
430 pascal -ret16 lstrcmp(str str) lstrcmp16
431 pascal   AnsiUpper(segstr) AnsiUpper16
432 pascal   AnsiLower(segstr) AnsiLower16
433 pascal -ret16 IsCharAlpha(word) IsCharAlphaA
434 pascal -ret16 IsCharAlphaNumeric(word) IsCharAlphaNumericA
435 pascal -ret16 IsCharUpper(word) IsCharUpperA
436 pascal -ret16 IsCharLower(word) IsCharLowerA
437 pascal -ret16 AnsiUpperBuff(str word) AnsiUpperBuff16
438 pascal -ret16 AnsiLowerBuff(str word) AnsiLowerBuff16
441 pascal -ret16 InsertMenuItem(word word word ptr) InsertMenuItem16
443 stub GetMenuItemInfo
445 pascal   DefFrameProc(word word word word long) DefFrameProc16
446 stub SetMenuItemInfo
447 pascal   DefMDIChildProc(word word word long) DefMDIChildProc16
448 pascal -ret16 DrawAnimatedRects(word word ptr ptr) DrawAnimatedRects16
449 pascal -ret16 DrawState(word word segptr long word s_word s_word s_word s_word word) DrawState16
450 pascal -ret16 CreateIconFromResourceEx(ptr long word long word word word) CreateIconFromResourceEx16
451 pascal -ret16 TranslateMDISysAccel(word ptr) TranslateMDISysAccel16
452 pascal -ret16 CreateWindowEx(long str str long s_word s_word s_word s_word word word word segptr) CreateWindowEx16
454 pascal -ret16 AdjustWindowRectEx(ptr long word long) AdjustWindowRectEx16
455 pascal -ret16 GetIconID(word long) GetIconID16
456 pascal -ret16 LoadIconHandler(word word) LoadIconHandler16
457 pascal -ret16 DestroyIcon(word) DestroyIcon16
458 pascal -ret16 DestroyCursor(word) DestroyCursor16
459 pascal   DumpIcon(segptr ptr ptr ptr) DumpIcon16
460 pascal -ret16 GetInternalWindowPos(word ptr ptr) GetInternalWindowPos16
461 pascal -ret16 SetInternalWindowPos(word word ptr ptr) SetInternalWindowPos16
462 pascal -ret16 CalcChildScroll(word word) CalcChildScroll16
463 pascal -ret16 ScrollChildren(word word word long) ScrollChildren16
464 pascal   DragObject(word word word word word word) DragObject16
465 pascal -ret16 DragDetect(word long) DragDetect16
466 pascal -ret16 DrawFocusRect(word ptr) DrawFocusRect16
470 stub StringFunc
471 pascal -ret16 lstrcmpi(str str) lstrcmpiA
472 pascal   AnsiNext(segptr) AnsiNext16
473 pascal   AnsiPrev(str segptr) AnsiPrev16
475 pascal -ret16 SetScrollInfo(word s_word ptr word) SetScrollInfo16
476 pascal -ret16 GetScrollInfo(word s_word ptr) GetScrollInfo16
477 pascal -ret16 GetKeyboardLayoutName(ptr) GetKeyboardLayoutName16
478 stub LoadKeyboardLayout
479 stub MenuItemFromPoint
480 stub GetUserLocalObjType
#481 HARDWARE_EVENT
482 pascal -ret16 EnableScrollBar(word word word) EnableScrollBar16
483 pascal -ret16 SystemParametersInfo(word word ptr word) SystemParametersInfo16
#484 __GP
# Stubs for Hebrew version
489 pascal -ret16 USER_489() stub_USER_489
490 pascal -ret16 USER_490() stub_USER_490
492 pascal -ret16 USER_492() stub_USER_492
496 pascal -ret16 USER_496() stub_USER_496
498 stub BEAR498
499 pascal -ret16 WNetErrorText(word ptr word) WNetErrorText16
500 stub FARCALLNETDRIVER 			# Undocumented Windows
501 pascal -ret16 WNetOpenJob(ptr ptr word ptr)  WNetOpenJob16
502 pascal -ret16 WNetCloseJob(word ptr ptr) WNetCloseJob16
503 pascal -ret16 WNetAbortJob(ptr word) WNetAbortJob16
504 pascal -ret16 WNetHoldJob(ptr word) WNetHoldJob16
505 pascal -ret16 WNetReleaseJob(ptr word) WNetReleaseJob16
506 pascal -ret16 WNetCancelJob(ptr word) WNetCancelJob16
507 pascal -ret16 WNetSetJobCopies(ptr word word) WNetSetJobCopies16
508 pascal -ret16 WNetWatchQueue(word ptr ptr word) WNetWatchQueue16
509 pascal -ret16 WNetUnwatchQueue(str) WNetUnwatchQueue16
510 pascal -ret16 WNetLockQueueData(ptr ptr ptr) WNetLockQueueData16
511 pascal -ret16 WNetUnlockQueueData(ptr) WNetUnlockQueueData16
512 pascal -ret16 WNetGetConnection(ptr ptr ptr) WNetGetConnection16
513 pascal -ret16 WNetGetCaps(word) WNetGetCaps16
514 pascal -ret16 WNetDeviceMode(word) WNetDeviceMode16
515 pascal -ret16 WNetBrowseDialog(word word ptr) WNetBrowseDialog16
516 pascal -ret16 WNetGetUser(ptr ptr ptr) WNetGetUser16
517 pascal -ret16 WNetAddConnection(str str str) WNetAddConnection16
518 pascal -ret16 WNetCancelConnection(str word) WNetCancelConnection16
519 pascal -ret16 WNetGetError(ptr) WNetGetError16
520 pascal -ret16 WNetGetErrorText(word ptr ptr) WNetGetErrorText16
521 stub WNetEnable
522 stub WNetDisable
523 pascal -ret16 WNetRestoreConnection(word ptr) WNetRestoreConnection16
524 pascal -ret16 WNetWriteJob(word ptr ptr) WNetWriteJob16
525 pascal -ret16 WNetConnectDialog(word word) WNetConnectDialog
526 pascal -ret16 WNetDisconnectDialog(word word) WNetDisconnectDialog16
527 pascal -ret16 WNetConnectionDialog(word word) WNetConnectionDialog16
528 pascal -ret16 WNetViewQueueDialog(word ptr) WNetViewQueueDialog16
529 pascal -ret16 WNetPropertyDialog(word word word str word) WNetPropertyDialog16
530 pascal -ret16 WNetGetDirectoryType(ptr ptr) WNetGetDirectoryType16
531 pascal -ret16 WNetDirectoryNotify(word ptr word) WNetDirectoryNotify16
532 pascal -ret16 WNetGetPropertyText(word word str str word word) WNetGetPropertyText16
533 stub WNetInitialize
#533 stub NOTIFYWOW # ordinal conflict with WNetInitialize !!
534 stub WNetLogon
#534 stub DEFDLGPROCTHUNK # ordinal conflict with WNetLogon !!
535 stub WOWWORDBREAKPROC
537 stub MOUSEEVENT
538 stub KEYBDEVENT
595 stub OLDEXITWINDOWS
600 pascal -ret16 GetShellWindow() GetShellWindow16
601 stub DoHotkeyStuff
602 stub SetCheckCursorTimer
604 stub BroadcastSystemMessage
605 stub HackTaskMonitor
606 pascal -ret16 FormatMessage(long segptr word word ptr word ptr) FormatMessage16
608 pascal -ret16 GetForegroundWindow() GetForegroundWindow16
609 pascal -ret16 SetForegroundWindow(word) SetForegroundWindow16
610 pascal -ret16 DestroyIcon32(word word) DestroyIcon32
620 pascal   ChangeDisplaySettings(ptr long) ChangeDisplaySettings16
621 pascal -ret16 EnumDisplaySettings(str long ptr) EnumDisplaySettings16
640 pascal   MsgWaitForMultipleObjects(long ptr long long long) MsgWaitForMultipleObjects16
650 stub ActivateKeyboardLayout
651 stub GetKeyboardLayout
652 stub GetKeyboardLayoutList
654 stub UnloadKeyboardLayout
655 stub PostPostedMessages
656 pascal -ret16 DrawFrameControl(word ptr word word) DrawFrameControl16
657 pascal -ret16 DrawCaptionTemp(word word ptr word word ptr word) DrawCaptionTemp16
658 stub DispatchInput
659 pascal -ret16 DrawEdge(word ptr word word) DrawEdge16
660 pascal -ret16 DrawCaption(word word ptr word) DrawCaption16
661 stub SetSysColorsTemp
662 stub DrawMenubarTemp
663 stub GetMenuDefaultItem
664 stub SetMenuDefaultItem
665 pascal -ret16 GetMenuItemRect(word word word ptr) GetMenuItemRect16
666 pascal -ret16 CheckMenuRadioItem(word word word word word) CheckMenuRadioItem16
667 stub TrackPopupMenuEx
668 pascal -ret16 SetWindowRgn(word word word) SetWindowRgn16
669 stub GetWindowRgn
800 stub CHOOSEFONT_CALLBACK16
801 stub FINDREPLACE_CALLBACK16
802 stub OPENFILENAME_CALLBACK16
803 stub PRINTDLG_CALLBACK16
804 stub CHOOSECOLOR_CALLBACK16
819 pascal -ret16 PeekMessage32(ptr word word word word word) PeekMessage32_16
820 pascal   GetMessage32(ptr word word word word) GetMessage32_16
821 pascal -ret16 TranslateMessage32(ptr word) TranslateMessage32_16
#821 stub IsDialogMessage32		# FIXME: two ordinal 821???
822 pascal   DispatchMessage32(ptr word) DispatchMessage32_16
823 pascal -ret16 CallMsgFilter32(ptr word word) CallMsgFilter32_16
825 stub PostMessage32
826 stub PostThreadMessage32
827 pascal -ret16 MessageBoxIndirect(ptr) MessageBoxIndirect16
851 stub MsgThkConnectionDataLS
853 stub FT_USRFTHKTHKCONNECTIONDATA
854 stub FT__USRF2THKTHKCONNECTIONDATA
855 stub Usr32ThkConnectionDataSL
890 stub InstallIMT
891 stub UninstallIMT
# API for Hebrew version
902 pascal -ret16 LoadSystemLanguageString(word word ptr word word) LoadSystemLanguageString16
905 pascal -ret16 ChangeDialogTemplate() ChangeDialogTemplate16
906 pascal -ret16 GetNumLanguages() GetNumLanguages16
907 pascal -ret16 GetLanguageName(word word ptr word) GetLanguageName16
909 pascal -ret16 SetWindowTextEx(word str word) SetWindowTextEx16
910 pascal -ret16 BiDiMessageBoxEx() BiDiMessageBoxEx16
911 pascal -ret16 SetDlgItemTextEx(word word str word) SetDlgItemTextEx16
912 pascal   ChangeKeyboardLanguage(word word) ChangeKeyboardLanguage16
913 pascal -ret16 GetCodePageSystemFont(word word) GetCodePageSystemFont16
914 pascal -ret16 QueryCodePage(word word word long) QueryCodePage16
915 pascal   GetAppCodePage(word) GetAppCodePage16
916 pascal -ret16 CreateDialogIndirectParamML(word ptr word ptr long word word str word) CreateDialogIndirectParamML16
918 pascal -ret16 DialogBoxIndirectParamML(word word word ptr long word word str word) DialogBoxIndirectParamML16
919 pascal -ret16 LoadLanguageString(word word word ptr word) LoadLanguageString16
920 pascal   SetAppCodePage(word word word word) SetAppCodePage16
922 pascal   GetBaseCodePage() GetBaseCodePage16
923 pascal -ret16 FindLanguageResource(word str str word) FindLanguageResource16
924 pascal   ChangeKeyboardCodePage(word word) ChangeKeyboardCodePage16
930 pascal -ret16 MessageBoxEx(word str str word word) MessageBoxEx16
1000 pascal -ret16 SetProcessDefaultLayout(long) SetProcessDefaultLayout16
1001 pascal -ret16 GetProcessDefaultLayout(ptr) GetProcessDefaultLayout16

# Wine internal functions
1010 pascal __wine_call_wndproc(word word word long long) __wine_call_wndproc
