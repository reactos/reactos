; Functions exported by Win 2K3 SP2
@ stdcall ActivateKeyboardLayout(long long) NtUserActivateKeyboardLayout
@ stdcall AdjustWindowRect(ptr long long)
@ stdcall AdjustWindowRectEx(ptr long long long)
@ stdcall AlignRects(ptr long long long)
@ stdcall AllowForegroundActivation()
@ stdcall AllowSetForegroundWindow(long)
@ stdcall AnimateWindow(long long long)
@ stdcall AnyPopup()
@ stdcall AppendMenuA(long long long ptr)
@ stdcall AppendMenuW(long long long ptr)
@ stdcall ArrangeIconicWindows(long)
@ stdcall AttachThreadInput(long long long) NtUserAttachThreadInput
@ stdcall BeginDeferWindowPos(long)
@ stdcall BeginPaint(long ptr) NtUserBeginPaint
@ stdcall BlockInput(long) NtUserBlockInput
@ stdcall BringWindowToTop(long)
@ stdcall BroadcastSystemMessage(long ptr long long long) BroadcastSystemMessageA
@ stdcall BroadcastSystemMessageA(long ptr long long long)
@ stdcall BroadcastSystemMessageExA(long ptr long long long ptr)
@ stdcall BroadcastSystemMessageExW(long ptr long long long ptr)
@ stdcall BroadcastSystemMessageW(long ptr long long long)
@ stdcall BuildReasonArray(ptr)
@ stdcall CalcMenuBar(long long long long long) NtUserCalcMenuBar
@ stdcall CallMsgFilter(ptr long) CallMsgFilterA
@ stdcall CallMsgFilterA(ptr long)
@ stdcall CallMsgFilterW(ptr long)
@ stdcall CallNextHookEx(long long long long)
@ stdcall CallWindowProcA(ptr long long long long)
@ stdcall CallWindowProcW(ptr long long long long)
@ stdcall CascadeChildWindows(long long)
@ stdcall CascadeWindows(long long ptr long ptr)
@ stdcall ChangeClipboardChain(long long) NtUserChangeClipboardChain
@ stdcall ChangeDisplaySettingsA(ptr long)
@ stdcall ChangeDisplaySettingsExA(str ptr long long ptr)
@ stdcall ChangeDisplaySettingsExW(wstr ptr long long ptr)
@ stdcall ChangeDisplaySettingsW(ptr long)
@ stdcall ChangeMenuA(long long ptr long long)
@ stdcall ChangeMenuW(long long ptr long long)
@ stdcall CharLowerA(str)
@ stdcall CharLowerBuffA(str long)
@ stdcall CharLowerBuffW(wstr long)
@ stdcall CharLowerW(wstr)
@ stdcall CharNextA(str)
@ stdcall CharNextExA(long str long)
@ stdcall CharNextW(wstr)
@ stdcall CharPrevA(str str)
@ stdcall CharPrevExA(long str str long)
@ stdcall CharPrevW(wstr wstr)
@ stdcall CharToOemA(str ptr)
@ stdcall CharToOemBuffA(str ptr long)
@ stdcall CharToOemBuffW(wstr ptr long)
@ stdcall CharToOemW(wstr ptr)
@ stdcall CharUpperA(str)
@ stdcall CharUpperBuffA(str long)
@ stdcall CharUpperBuffW(wstr long)
@ stdcall CharUpperW(wstr)
@ stdcall CheckDlgButton(long long long)
@ stdcall CheckMenuItem(long long long)
@ stdcall CheckMenuRadioItem(long long long long long)
@ stdcall CheckRadioButton(long long long long)
@ stdcall ChildWindowFromPoint(long double)
@ stdcall ChildWindowFromPointEx(long double long) ; Direct call NtUserChildWindowFromPointEx
@ stdcall CliImmSetHotKey(long long long ptr)
@ stdcall ClientThreadSetup()
@ stdcall ClientToScreen(long ptr)
@ stdcall ClipCursor(ptr) NtUserClipCursor
@ stdcall CloseClipboard() NtUserCloseClipboard
@ stdcall CloseDesktop(long) NtUserCloseDesktop
@ stdcall CloseWindow(long)
@ stdcall CloseWindowStation(long) NtUserCloseWindowStation
@ stdcall CopyAcceleratorTableA(long ptr long)
@ stdcall CopyAcceleratorTableW(long ptr long) NtUserCopyAcceleratorTable
@ stdcall CopyIcon(long)
@ stdcall CopyImage(long long long long long)
@ stdcall CopyRect(ptr ptr)
@ stdcall CountClipboardFormats() NtUserCountClipboardFormats
@ stdcall CreateAcceleratorTableA(ptr long)
@ stdcall CreateAcceleratorTableW(ptr long) NtUserCreateAcceleratorTable
@ stdcall CreateCaret(long long long long) NtUserCreateCaret
@ stdcall CreateCursor(long long long long long ptr ptr)
@ stdcall CreateDesktopA(str str ptr long long ptr)
@ stdcall CreateDesktopW(wstr wstr ptr long long ptr)
@ stdcall CreateDialogIndirectParamA(long ptr long ptr long)
@ stdcall CreateDialogIndirectParamAorW(long ptr long ptr long long)
@ stdcall CreateDialogIndirectParamW(long ptr long ptr long)
@ stdcall CreateDialogParamA(long ptr long ptr long)
@ stdcall CreateDialogParamW(long ptr long ptr long)
@ stdcall CreateIcon(long long long long long ptr ptr)
@ stdcall CreateIconFromResource (ptr long long long)
@ stdcall CreateIconFromResourceEx(ptr long long long long long long)
@ stdcall CreateIconIndirect(ptr)
@ stdcall CreateMDIWindowA(ptr ptr long long long long long long long long)
@ stdcall CreateMDIWindowW(ptr ptr long long long long long long long long)
@ stdcall CreateMenu()
@ stdcall CreatePopupMenu()
@ stdcall CreateSystemThreads(long)
@ stdcall CreateWindowExA(long str str long long long long long long long long ptr)
@ stdcall CreateWindowExW(long wstr wstr long long long long long long long long ptr)
@ stdcall CreateWindowStationA(str long long ptr)
@ stdcall CreateWindowStationW(wstr long long ptr)
@ stdcall CsrBroadcastSystemMessageExW(long ptr long long long ptr)
@ stdcall CtxInitUser32()
# DbgWin32HeapFail
# DbgWin32HeapStat
@ stdcall DdeAbandonTransaction(long long long)
@ stdcall DdeAccessData(long ptr)
@ stdcall DdeAddData(long ptr long long)
@ stdcall DdeClientTransaction(ptr long long long long long long ptr)
@ stdcall DdeCmpStringHandles(long long)
@ stdcall DdeConnect(long long long ptr)
@ stdcall DdeConnectList(long long long long ptr)
@ stdcall DdeCreateDataHandle(long ptr long long long long long)
@ stdcall DdeCreateStringHandleA(long str long)
@ stdcall DdeCreateStringHandleW(long wstr long)
@ stdcall DdeDisconnect(long)
@ stdcall DdeDisconnectList(long)
@ stdcall DdeEnableCallback(long long long)
@ stdcall DdeFreeDataHandle(long)
@ stdcall DdeFreeStringHandle(long long)
@ stdcall DdeGetData(long ptr long long)
@ stdcall DdeGetLastError(long)
@ stdcall DdeGetQualityOfService(long long ptr) ; Direct call NtUserDdeGetQualityOfService
@ stdcall DdeImpersonateClient(long)
@ stdcall DdeInitializeA(ptr ptr long long)
@ stdcall DdeInitializeW(ptr ptr long long)
@ stdcall DdeKeepStringHandle(long long)
@ stdcall DdeNameService(long long long long)
@ stdcall DdePostAdvise(long long long)
@ stdcall DdeQueryConvInfo(long long ptr)
@ stdcall DdeQueryNextServer(long long)
@ stdcall DdeQueryStringA(long long ptr long long)
@ stdcall DdeQueryStringW(long long ptr long long)
@ stdcall DdeReconnect(long)
@ stdcall DdeSetQualityOfService(long ptr ptr) ; Direct call NtUserDdeSetQualityOfService
@ stdcall DdeSetUserHandle (long long long)
@ stdcall DdeUnaccessData(long)
@ stdcall DdeUninitialize(long)
@ stdcall DefDlgProcA(long long long long)
@ stdcall DefDlgProcW(long long long long)
@ stdcall DefFrameProcA(long long long long long)
@ stdcall DefFrameProcW(long long long long long)
@ stdcall DefMDIChildProcA(long long long long)
@ stdcall DefMDIChildProcW(long long long long)
@ stdcall DefRawInputProc(ptr long long)
@ stdcall DefWindowProcA(long long long long)
@ stdcall DefWindowProcW(long long long long)
@ stdcall DeferWindowPos(long long long long long long long long) ; Direct call NtUserDeferWindowPos
@ stdcall DeleteMenu(long long long) NtUserDeleteMenu
@ stdcall DeregisterShellHookWindow (long)
@ stdcall DestroyAcceleratorTable(long)
@ stdcall DestroyCaret()
@ stdcall DestroyCursor(long)
@ stdcall DestroyIcon(long)
@ stdcall DestroyMenu(long) NtUserDestroyMenu
@ stdcall DestroyReasons(ptr)
@ stdcall DestroyWindow(long) NtUserDestroyWindow
@ stdcall DeviceEventWorker(long long long long long)
@ stdcall DialogBoxIndirectParamA(long ptr long ptr long)
@ stdcall DialogBoxIndirectParamAorW(long ptr long ptr long long)
@ stdcall DialogBoxIndirectParamW(long ptr long ptr long)
@ stdcall DialogBoxParamA(long str long ptr long)
@ stdcall DialogBoxParamW(long wstr long ptr long)
@ stdcall DisableProcessWindowsGhosting()
@ stdcall DispatchMessageA(ptr)
@ stdcall DispatchMessageW(ptr)
@ stdcall DisplayExitWindowsWarnings(long)
@ stdcall DlgDirListA(long str long long long)
@ stdcall DlgDirListComboBoxA(long ptr long long long)
@ stdcall DlgDirListComboBoxW(long ptr long long long)
@ stdcall DlgDirListW(long wstr long long long)
@ stdcall DlgDirSelectComboBoxExA(long ptr long long)
@ stdcall DlgDirSelectComboBoxExW(long ptr long long)
@ stdcall DlgDirSelectExA(long ptr long long)
@ stdcall DlgDirSelectExW(long ptr long long)
@ stdcall DragDetect(long double) ; Direct call NtUserDragDetect
@ stdcall DragObject(long long long long long) NtUserDragObject
@ stdcall DrawAnimatedRects(long long ptr ptr) NtUserDrawAnimatedRects
@ stdcall DrawCaption(long long ptr long)
@ stdcall DrawCaptionTempA(long long ptr long long str long)
@ stdcall DrawCaptionTempW(long long ptr long long wstr long)
@ stdcall DrawEdge(long ptr long long)
@ stdcall DrawFocusRect(long ptr)
@ stdcall DrawFrame(long ptr long long)
@ stdcall DrawFrameControl(long ptr long long)
@ stdcall DrawIcon(long long long long)
@ stdcall DrawIconEx(long long long long long long long long long)
@ stdcall DrawMenuBar(long)
@ stdcall DrawMenuBarTemp(long long long long long) NtUserDrawMenuBarTemp
@ stdcall DrawStateA(long long ptr long long long long long long long)
@ stdcall DrawStateW(long long ptr long long long long long long long)
@ stdcall DrawTextA(long str long ptr long)
@ stdcall DrawTextExA(long str long ptr long ptr)
@ stdcall DrawTextExW(long wstr long ptr long ptr)
@ stdcall DrawTextW(long wstr long ptr long)
@ stdcall EditWndProc(long long long long) EditWndProcA
@ stdcall EmptyClipboard() NtUserEmptyClipboard
@ stdcall EnableMenuItem(long long long)
@ stdcall EnableScrollBar(long long long)
@ stdcall EnableWindow(long long)
@ stdcall EndDeferWindowPos(long)
@ stdcall EndDialog(long long)
@ stdcall EndMenu() NtUserEndMenu
@ stdcall EndPaint(long ptr) NtUserEndPaint
@ stdcall EndTask(ptr long long)
@ stdcall EnterReaderModeHelper(ptr)
@ stdcall EnumChildWindows(long ptr long)
@ stdcall EnumClipboardFormats(long)
@ stdcall EnumDesktopWindows(long ptr ptr)
@ stdcall EnumDesktopsA(ptr ptr long)
@ stdcall EnumDesktopsW(ptr ptr long)
@ stdcall EnumDisplayDevicesA(ptr long ptr long)
@ stdcall EnumDisplayDevicesW(ptr long ptr long)
@ stdcall EnumDisplayMonitors(long ptr ptr long) ; Direct call NtUserEnumDisplayMonitors
@ stdcall EnumDisplaySettingsA(str long ptr)
@ stdcall EnumDisplaySettingsExA(str long ptr long)
@ stdcall EnumDisplaySettingsExW(wstr long ptr long)
@ stdcall EnumDisplaySettingsW(wstr long ptr )
@ stdcall EnumPropsA(long ptr)
@ stdcall EnumPropsExA(long ptr long)
@ stdcall EnumPropsExW(long ptr long)
@ stdcall EnumPropsW(long ptr)
@ stdcall EnumThreadWindows(long ptr long)
@ stdcall EnumWindowStationsA(ptr long)
@ stdcall EnumWindowStationsW(ptr long)
@ stdcall EnumWindows(ptr long)
@ stdcall EqualRect(ptr ptr)
@ stdcall ExcludeUpdateRgn(long long) NtUserExcludeUpdateRgn
@ stdcall ExitWindowsEx(long long)
@ stdcall FillRect(long ptr long)
@ stdcall FindWindowA(str str)
@ stdcall FindWindowExA(long long str str)
@ stdcall FindWindowExW(long long wstr wstr)
@ stdcall FindWindowW(wstr wstr)
@ stdcall FlashWindow(long long)
@ stdcall FlashWindowEx(ptr) NtUserFlashWindowEx
@ stdcall FrameRect(long ptr long)
@ stdcall FreeDDElParam(long long)
@ stdcall GetActiveWindow()
@ stdcall GetAltTabInfo(long long ptr ptr long) GetAltTabInfoA
@ stdcall GetAltTabInfoA(long long ptr ptr long)
@ stdcall GetAltTabInfoW(long long ptr ptr long)
@ stdcall GetAncestor(long long) ; Direct call NtUserGetAncestor
@ stdcall GetAppCompatFlags(long)
@ stdcall GetAppCompatFlags2(long)
@ stdcall GetAsyncKeyState(long)
@ stdcall GetCapture()
@ stdcall GetCaretBlinkTime() NtUserGetCaretBlinkTime
@ stdcall GetCaretPos(ptr) NtUserGetCaretPos
@ stdcall GetClassInfoA(long str ptr)
@ stdcall GetClassInfoExA(long str ptr)
@ stdcall GetClassInfoExW(long wstr ptr)
@ stdcall GetClassInfoW(long wstr ptr)
@ stdcall GetClassLongA(long long)
@ stdcall -arch=x86_64,arm64 GetClassLongPtrA(ptr long)
@ stdcall -arch=x86_64,arm64 GetClassLongPtrW(ptr long)
@ stdcall GetClassLongW(long long)
@ stdcall GetClassNameA(long ptr long)
@ stdcall GetClassNameW(long ptr long)
@ stdcall GetClassWord(long long)
@ stdcall GetClientRect(long long)
@ stdcall GetClipCursor(ptr) NtUserGetClipCursor
@ stdcall GetClipboardData(long)
@ stdcall GetClipboardFormatNameA(long ptr long)
@ stdcall GetClipboardFormatNameW(long ptr long)
@ stdcall GetClipboardOwner() NtUserGetClipboardOwner
@ stdcall GetClipboardSequenceNumber() NtUserGetClipboardSequenceNumber
@ stdcall GetClipboardViewer() NtUserGetClipboardViewer
@ stdcall GetComboBoxInfo(long ptr) ; Direct call NtUserGetComboBoxInfo
@ stdcall GetCursor()
@ stdcall GetCursorFrameInfo(long long long long long)
@ stdcall GetCursorInfo(ptr) NtUserGetCursorInfo
@ stdcall GetCursorPos(ptr)
@ stdcall GetDC(long) NtUserGetDC
@ stdcall GetDCEx(long long long) NtUserGetDCEx
# GetDbgTagFlags
@ stdcall GetDesktopWindow()
@ stdcall GetDialogBaseUnits()
@ stdcall GetDlgCtrlID(long)
@ stdcall GetDlgItem(long long)
@ stdcall GetDlgItemInt(long long ptr long)
@ stdcall GetDlgItemTextA(long long ptr long)
@ stdcall GetDlgItemTextW(long long ptr long)
@ stdcall GetDoubleClickTime() NtUserGetDoubleClickTime
@ stdcall -version=0xA00+ GetDpiForSystem()
@ stdcall -version=0xA00+ GetDpiForWindow(ptr)
@ stdcall GetFocus()
@ stdcall GetForegroundWindow() NtUserGetForegroundWindow
@ stdcall GetGUIThreadInfo(long ptr) NtUserGetGUIThreadInfo
@ stdcall GetGuiResources(long long) NtUserGetGuiResources
@ stdcall GetIconInfo(long ptr)
@ stdcall GetInputDesktop()
@ stdcall GetInputState()
@ stdcall GetInternalWindowPos(long ptr ptr) NtUserGetInternalWindowPos
@ stdcall GetKBCodePage()
@ stdcall GetKeyNameTextA(long ptr long)
@ stdcall GetKeyNameTextW(long ptr long)
@ stdcall GetKeyState(long)
@ stdcall GetKeyboardLayout(long)
@ stdcall GetKeyboardLayoutList(long ptr) NtUserGetKeyboardLayoutList
@ stdcall GetKeyboardLayoutNameA(ptr)
@ stdcall GetKeyboardLayoutNameW(ptr)
@ stdcall GetKeyboardState(ptr) NtUserGetKeyboardState
@ stdcall GetKeyboardType(long)
@ stdcall GetLastActivePopup(long)
@ stdcall GetLastInputInfo(ptr)
@ stdcall GetLayeredWindowAttributes(long ptr ptr ptr) NtUserGetLayeredWindowAttributes
@ stdcall GetListBoxInfo(long) NtUserGetListBoxInfo
@ stdcall GetMenu(long)
@ stdcall GetMenuBarInfo(long long long ptr) NtUserGetMenuBarInfo
@ stdcall GetMenuCheckMarkDimensions()
@ stdcall GetMenuContextHelpId(long)
@ stdcall GetMenuDefaultItem(long long long)
@ stdcall GetMenuInfo(long ptr)
@ stdcall GetMenuItemCount(long)
@ stdcall GetMenuItemID(long long)
@ stdcall GetMenuItemInfoA(long long long ptr)
@ stdcall GetMenuItemInfoW(long long long ptr)
@ stdcall GetMenuItemRect(long long long ptr) NtUserGetMenuItemRect
@ stdcall GetMenuState(long long long)
@ stdcall GetMenuStringA(long long ptr long long)
@ stdcall GetMenuStringW(long long ptr long long)
@ stdcall GetMessageA(ptr long long long)
@ stdcall GetMessageExtraInfo()
@ stdcall GetMessagePos()
@ stdcall GetMessageTime()
@ stdcall GetMessageW(ptr long long long)
@ stdcall GetMonitorInfoA(long ptr)
@ stdcall GetMonitorInfoW(long ptr)
@ stdcall GetMouseMovePointsEx(long ptr ptr long long) NtUserGetMouseMovePointsEx
@ stdcall GetNextDlgGroupItem(long long long)
@ stdcall GetNextDlgTabItem(long long long)
@ stdcall GetOpenClipboardWindow() NtUserGetOpenClipboardWindow
@ stdcall GetParent(long)
@ stdcall GetPriorityClipboardFormat(ptr long) NtUserGetPriorityClipboardFormat
@ stdcall GetProcessDefaultLayout(ptr)
@ stdcall GetProcessWindowStation() NtUserGetProcessWindowStation
@ stdcall GetProgmanWindow ()
@ stdcall GetPropA(long str)
@ stdcall GetPropW(long wstr)
@ stdcall GetQueueStatus(long)
@ stdcall GetRawInputBuffer(ptr ptr long)
@ stdcall GetRawInputData(ptr long ptr ptr long)
@ stdcall GetRawInputDeviceInfoA(ptr long ptr ptr)
@ stdcall GetRawInputDeviceInfoW(ptr long ptr ptr)
@ stdcall GetRawInputDeviceList(ptr ptr long)
@ stdcall GetReasonTitleFromReasonCode(long long long)
@ stdcall GetRegisteredRawInputDevices(ptr ptr long)
# GetRipFlags
@ stdcall GetScrollBarInfo(long long ptr) ; NtUserGetScrollBarInfo
@ stdcall GetScrollInfo(long long ptr)
@ stdcall GetScrollPos(long long)
@ stdcall GetScrollRange(long long ptr ptr)
@ stdcall GetShellWindow()
@ stdcall GetSubMenu(long long)
@ stdcall GetSysColor(long)
@ stdcall GetSysColorBrush(long)
@ stdcall GetSystemMenu(long long) ; NtUserGetSystemMenu
@ stdcall GetSystemMetrics(long)
@ stdcall GetTabbedTextExtentA(long str long long ptr)
@ stdcall GetTabbedTextExtentW(long wstr long long ptr)
@ stdcall GetTaskmanWindow ()
@ stdcall GetThreadDesktop(long)
@ stdcall GetTitleBarInfo(long ptr) NtUserGetTitleBarInfo
@ stdcall GetTopWindow(long)
@ stdcall GetUpdateRect(long ptr long)
@ stdcall GetUpdateRgn(long long long)
@ stdcall GetUserObjectInformationA(long long ptr long ptr)
@ stdcall GetUserObjectInformationW(long long ptr long ptr) NtUserGetObjectInformation
@ stdcall GetUserObjectSecurity (long ptr ptr long ptr)
@ stdcall GetWinStationInfo(ptr)
@ stdcall GetWindow(long long)
@ stdcall GetWindowContextHelpId(long)
@ stdcall GetWindowDC(long) NtUserGetWindowDC
@ stdcall GetWindowInfo(long ptr)
@ stdcall GetWindowLongA(long long)
@ stdcall -arch=x86_64,arm64 GetWindowLongPtrA(ptr long)
@ stdcall -arch=x86_64,arm64 GetWindowLongPtrW(ptr long)
@ stdcall GetWindowLongW(long long)
@ stdcall GetWindowModuleFileName(long ptr long) GetWindowModuleFileNameA
@ stdcall GetWindowModuleFileNameA(long ptr long)
@ stdcall GetWindowModuleFileNameW(long ptr long)
@ stdcall GetWindowPlacement(long ptr) NtUserGetWindowPlacement
@ stdcall GetWindowRect(long ptr)
@ stdcall GetWindowRgn(long long)
@ stdcall GetWindowRgnBox(long ptr)
@ stdcall GetWindowTextA(long ptr long)
@ stdcall GetWindowTextLengthA(long)
@ stdcall GetWindowTextLengthW(long)
@ stdcall GetWindowTextW(long ptr long)
@ stdcall GetWindowThreadProcessId(long ptr)
@ stdcall GetWindowWord(long long)
@ stdcall GrayStringA(long long ptr long long long long long long)
@ stdcall GrayStringW(long long ptr long long long long long long)
@ stdcall HideCaret(long) NtUserHideCaret
@ stdcall HiliteMenuItem(long long long long) NtUserHiliteMenuItem
@ stdcall IMPGetIMEA(long ptr)
@ stdcall IMPGetIMEW(long ptr)
@ stdcall IMPQueryIMEA(ptr)
@ stdcall IMPQueryIMEW(ptr)
@ stdcall IMPSetIMEA(long ptr)
@ stdcall IMPSetIMEW(long ptr)
@ stdcall ImpersonateDdeClientWindow(long long) ; Direct call NtUserImpersonateDdeClientWindow
@ stdcall InSendMessage()
@ stdcall InSendMessageEx(ptr)
@ stdcall InflateRect(ptr long long)
@ stdcall InitializeLpkHooks(ptr)
@ stdcall InitializeWin32EntryTable(ptr)
@ stdcall InsertMenuA(long long long long ptr)
@ stdcall InsertMenuItemA(long long long ptr)
@ stdcall InsertMenuItemW(long long long ptr)
@ stdcall InsertMenuW(long long long long ptr)
@ stdcall InternalGetWindowText(long long long)
@ stdcall IntersectRect(ptr ptr ptr)
@ stdcall InvalidateRect(long ptr long) NtUserInvalidateRect
@ stdcall InvalidateRgn(long long long) NtUserInvalidateRgn
@ stdcall InvertRect(long ptr)
@ stdcall IsCharAlphaA(long)
@ stdcall IsCharAlphaNumericA(long)
@ stdcall IsCharAlphaNumericW(long)
@ stdcall IsCharAlphaW(long)
@ stdcall IsCharLowerA(long)
@ stdcall IsCharLowerW(long)
@ stdcall IsCharUpperA(long)
@ stdcall IsCharUpperW(long)
@ stdcall IsChild(long long)
@ stdcall IsClipboardFormatAvailable(long) NtUserIsClipboardFormatAvailable
@ stdcall IsDialogMessage(long ptr) IsDialogMessageA
@ stdcall IsDialogMessageA(long ptr)
@ stdcall IsDialogMessageW(long ptr)
@ stdcall IsDlgButtonChecked(long long)
@ stdcall IsGUIThread(long)
@ stdcall IsHungAppWindow(long)
@ stdcall IsIconic(long)
@ stdcall IsMenu(long)
@ stdcall -stub IsProcess16Bit()
@ stdcall IsRectEmpty(ptr)
@ stdcall IsSETEnabled()
@ stdcall IsServerSideWindow(long)
@ stdcall IsWinEventHookInstalled(long)
@ stdcall IsWindow(long)
@ stdcall IsWindowEnabled(long)
@ stdcall IsWindowInDestroy(long)
@ stdcall IsWindowUnicode(long)
@ stdcall IsWindowVisible(long)
@ stdcall -stub IsWow64Message()
@ stdcall IsZoomed(long)
@ stdcall KillSystemTimer(long long)
@ stdcall KillTimer(long long) NtUserKillTimer
@ stdcall LoadAcceleratorsA(long str)
@ stdcall LoadAcceleratorsW(long wstr)
@ stdcall LoadBitmapA(long str)
@ stdcall LoadBitmapW(long wstr)
@ stdcall LoadCursorA(long str)
@ stdcall LoadCursorFromFileA(str)
@ stdcall LoadCursorFromFileW(wstr)
@ stdcall LoadCursorW(long wstr)
@ stdcall LoadIconA(long str)
@ stdcall LoadIconW(long wstr)
@ stdcall LoadImageA(long str long long long long)
@ stdcall LoadImageW(long wstr long long long long)
@ stdcall LoadKeyboardLayoutA(str long)
@ stdcall LoadKeyboardLayoutEx(long ptr long)
@ stdcall LoadKeyboardLayoutW(wstr long)
@ stdcall LoadLocalFonts()
@ stdcall LoadMenuA(long str)
@ stdcall LoadMenuIndirectA(ptr)
@ stdcall LoadMenuIndirectW(ptr)
@ stdcall LoadMenuW(long wstr)
@ stdcall LoadRemoteFonts()
@ stdcall LoadStringA(long long ptr long)
@ stdcall LoadStringW(long long ptr long)
@ stdcall LockSetForegroundWindow (long)
@ stdcall LockWindowStation(long) NtUserLockWindowStation
@ stdcall LockWindowUpdate(long) NtUserLockWindowUpdate
@ stdcall LockWorkStation() NtUserLockWorkStation
@ stdcall LookupIconIdFromDirectory(ptr long)
@ stdcall LookupIconIdFromDirectoryEx(ptr long long long long)
@ stdcall MBToWCSEx(long str long wstr long long)
@ stdcall MB_GetString(ptr)
@ stdcall MapDialogRect(long ptr)
@ stdcall MapVirtualKeyA(long long)
@ stdcall MapVirtualKeyExA(long long long)
@ stdcall MapVirtualKeyExW(long long long)
@ stdcall MapVirtualKeyW(long long)
@ stdcall MapWindowPoints(long long ptr long)
@ stdcall MenuItemFromPoint(long long double) NtUserMenuItemFromPoint
@ stdcall MenuWindowProcA (long ptr long long long)
@ stdcall MenuWindowProcW (long ptr long long long)
@ stdcall MessageBeep(long)
@ stdcall MessageBoxA(long str str long)
@ stdcall MessageBoxExA(long str str long long)
@ stdcall MessageBoxExW(long wstr wstr long long)
@ stdcall MessageBoxIndirectA(ptr)
@ stdcall MessageBoxIndirectW(ptr)
@ stdcall MessageBoxTimeoutA(ptr str str long long long)
@ stdcall MessageBoxTimeoutW(ptr wstr wstr long long long)
@ stdcall MessageBoxW(long wstr wstr long)
@ stdcall ModifyMenuA(long long long long ptr)
@ stdcall ModifyMenuW(long long long long ptr)
@ stdcall MonitorFromPoint(double long)
@ stdcall MonitorFromRect(ptr long)
@ stdcall MonitorFromWindow(long long)
@ stdcall MoveWindow(long long long long long long) NtUserMoveWindow
@ stdcall MsgWaitForMultipleObjects(long ptr long long long)
@ stdcall MsgWaitForMultipleObjectsEx(long ptr long long long)
@ stdcall NotifyWinEvent(long long long long)
@ stdcall OemKeyScan(long)
@ stdcall OemToCharA(ptr ptr)
@ stdcall OemToCharBuffA(ptr ptr long)
@ stdcall OemToCharBuffW(ptr ptr long)
@ stdcall OemToCharW(ptr ptr)
@ stdcall OffsetRect(ptr long long)
@ stdcall OpenClipboard(long)
@ stdcall OpenDesktopA(str long long long)
@ stdcall OpenDesktopW(wstr long long long)
@ stdcall OpenIcon(long)
@ stdcall OpenInputDesktop(long long long) NtUserOpenInputDesktop
@ stdcall OpenWindowStationA(str long long)
@ stdcall OpenWindowStationW(wstr long long)
@ stdcall PackDDElParam(long long long)
@ stdcall PaintDesktop(long) NtUserPaintDesktop
@ stdcall PaintMenuBar(long long long long long long) NtUserPaintMenuBar
@ stdcall PeekMessageA(ptr long long long long)
@ stdcall PeekMessageW(ptr long long long long)
@ stdcall PostMessageA(long long long long)
@ stdcall PostMessageW(long long long long)
@ stdcall PostQuitMessage(long)
@ stdcall PostThreadMessageA(long long long long)
@ stdcall PostThreadMessageW(long long long long)
@ stdcall PrintWindow(ptr ptr long) NtUserPrintWindow
@ stdcall PrivateExtractIconExA(str long ptr ptr long)
@ stdcall PrivateExtractIconExW(wstr long ptr ptr long)
@ stdcall PrivateExtractIconsA(str long long long ptr ptr long long)
@ stdcall PrivateExtractIconsW(wstr long long long ptr ptr long long)
# PrivateSetDbgTag
# PrivateSetRipFlags
@ stdcall PtInRect(ptr double)
@ stdcall QuerySendMessage(ptr) NtUserQuerySendMessage
@ stdcall RealChildWindowFromPoint(long double) ; Direct call NtUserRealChildWindowFromPoint
@ stdcall RealGetWindowClass(long ptr long) RealGetWindowClassA
@ stdcall RealGetWindowClassA(long ptr long)
@ stdcall RealGetWindowClassW(long ptr long)
@ stdcall ReasonCodeNeedsBugID(long)
@ stdcall ReasonCodeNeedsComment(long)
@ stdcall RecordShutdownReason(long)
@ stdcall RedrawWindow(long ptr long long) NtUserRedrawWindow
@ stdcall RegisterClassA(ptr)
@ stdcall RegisterClassExA(ptr)
@ stdcall RegisterClassExW(ptr)
@ stdcall RegisterClassW(ptr)
@ stdcall RegisterClipboardFormatA(str)
@ stdcall RegisterClipboardFormatW(wstr)
@ stdcall RegisterDeviceNotificationA(long ptr long) RegisterDeviceNotificationW
@ stdcall RegisterDeviceNotificationW(long ptr long)
@ stdcall RegisterHotKey(long long long long) NtUserRegisterHotKey
@ stdcall RegisterLogonProcess(long long)
@ stdcall RegisterMessagePumpHook(ptr)
@ stdcall RegisterRawInputDevices(ptr long long)
@ stdcall RegisterServicesProcess(long)
@ stdcall RegisterShellHookWindow(long)
@ stdcall RegisterSystemThread(long long)
@ stdcall RegisterTasklist(long) NtUserRegisterTasklist
@ stdcall RegisterUserApiHook(ptr)
@ stdcall RegisterWindowMessageA(str)
@ stdcall RegisterWindowMessageW(wstr)
@ stdcall ReleaseCapture()
@ stdcall ReleaseDC(long long)
@ stdcall RemoveMenu(long long long) NtUserRemoveMenu
@ stdcall RemovePropA(long str)
@ stdcall RemovePropW(long wstr)
@ stdcall ReplyMessage(long)
# ResolveDesktopForWOW
@ stdcall ReuseDDElParam(long long long long long)
@ stdcall ScreenToClient(long ptr)
@ stdcall ScrollChildren(long long long long)
@ stdcall ScrollDC(long long long ptr ptr long ptr)
@ stdcall ScrollWindow(long long long ptr ptr)
@ stdcall ScrollWindowEx(long long long ptr ptr long ptr long)
@ stdcall SendDlgItemMessageA(long long long long long)
@ stdcall SendDlgItemMessageW(long long long long long)
@ stdcall SendIMEMessageExA(long long)
@ stdcall SendIMEMessageExW(long long)
@ stdcall SendInput(long ptr long) NtUserSendInput
@ stdcall SendMessageA(long long long long)
@ stdcall SendMessageCallbackA(long long long long ptr long)
@ stdcall SendMessageCallbackW(long long long long ptr long)
@ stdcall SendMessageTimeoutA(long long long long long long ptr)
@ stdcall SendMessageTimeoutW(long long long long long long ptr)
@ stdcall SendMessageW(long long long long)
@ stdcall SendNotifyMessageA(long long long long)
@ stdcall SendNotifyMessageW(long long long long)
@ stdcall SetActiveWindow(long) NtUserSetActiveWindow
@ stdcall SetCapture(long) NtUserSetCapture
@ stdcall SetCaretBlinkTime(long)
@ stdcall SetCaretPos(long long)
@ stdcall SetClassLongA(long long long)
@ stdcall -arch=x86_64 SetClassLongPtrA(ptr long ptr)
@ stdcall -arch=x86_64 SetClassLongPtrW(ptr long ptr)
@ stdcall SetClassLongW(long long long)
@ stdcall SetClassWord(long long long) ; Direct call NtUserSetClassWord
@ stdcall SetClipboardData(long long)
@ stdcall SetClipboardViewer(long) NtUserSetClipboardViewer
@ stdcall SetConsoleReserveKeys(long long) NtUserSetConsoleReserveKeys
@ stdcall SetCursor(long) NtUserSetCursor
@ stdcall SetCursorContents(ptr ptr) NtUserSetCursorContents
@ stdcall SetCursorPos(long long)
# SetDbgTag
@ stdcall SetDebugErrorLevel(long)
@ stdcall SetDeskWallpaper(ptr)
@ stdcall SetDlgItemInt(long long long long)
@ stdcall SetDlgItemTextA(long long str)
@ stdcall SetDlgItemTextW(long long wstr)
@ stdcall SetDoubleClickTime(long)
@ stdcall SetFocus(long) NtUserSetFocus
@ stdcall SetForegroundWindow(long)
@ stdcall SetInternalWindowPos(long long ptr ptr) NtUserSetInternalWindowPos
@ stdcall SetKeyboardState(ptr) NtUserSetKeyboardState
@ stdcall SetLastErrorEx(long long)
@ stdcall SetLayeredWindowAttributes(ptr long long long) NtUserSetLayeredWindowAttributes
@ stdcall SetLogonNotifyWindow(long) ; Direct call NtUserSetLogonNotifyWindow
@ stdcall SetMenu(long long)
@ stdcall SetMenuContextHelpId(long long) NtUserSetMenuContextHelpId
@ stdcall SetMenuDefaultItem(long long long) NtUserSetMenuDefaultItem
@ stdcall SetMenuInfo(long ptr)
@ stdcall SetMenuItemBitmaps(long long long long long)
@ stdcall SetMenuItemInfoA(long long long ptr)
@ stdcall SetMenuItemInfoW(long long long ptr)
@ stdcall SetMessageExtraInfo(long)
@ stdcall SetMessageQueue(long)
@ stdcall SetParent(long long) NtUserSetParent
@ stdcall SetProcessDefaultLayout(long)
@ stdcall SetProcessWindowStation(long) NtUserSetProcessWindowStation
@ stdcall SetProgmanWindow (long)
@ stdcall SetPropA(long str long)
@ stdcall SetPropW(long wstr long)
@ stdcall SetRect(ptr long long long long)
@ stdcall SetRectEmpty(ptr)
# SetRipFlags
@ stdcall SetScrollInfo(long long ptr long) ; Direct call NtUserSetScrollInfo
@ stdcall SetScrollPos(long long long long)
@ stdcall SetScrollRange(long long long long long)
@ stdcall SetShellWindow(long)
@ stdcall SetShellWindowEx(long long) NtUserSetShellWindowEx
@ stdcall SetSysColors(long ptr ptr)
@ stdcall SetSysColorsTemp(ptr ptr long)
@ stdcall SetSystemCursor(long long)
@ stdcall SetSystemMenu(long long) ; NtUserSetSystemMenu
@ stdcall SetSystemTimer(long long long ptr) NtUserSetSystemTimer
@ stdcall SetTaskmanWindow (long)
@ stdcall SetThreadDesktop(long) NtUserSetThreadDesktop
@ stdcall SetTimer(long long long ptr) NtUserSetTimer
@ stdcall SetUserObjectInformationA(long long ptr long) NtUserSetObjectInformation
@ stdcall SetUserObjectInformationW(long long ptr long) NtUserSetObjectInformation
@ stdcall SetUserObjectSecurity(long ptr ptr)
@ stdcall SetWinEventHook(long long long ptr long long long)
@ stdcall SetWindowContextHelpId(long long)
@ stdcall SetWindowLongA(long long long)
@ stdcall -arch=x86_64,arm64 SetWindowLongPtrA(ptr long ptr)
@ stdcall -arch=x86_64,arm64 SetWindowLongPtrW(ptr long ptr)
@ stdcall SetWindowLongW(long long long)
@ stdcall SetWindowPlacement(long ptr) NtUserSetWindowPlacement
@ stdcall SetWindowPos(long long long long long long long) NtUserSetWindowPos
@ stdcall SetWindowRgn(long long long)
@ stdcall SetWindowStationUser(long long long long)
@ stdcall SetWindowTextA(long str)
@ stdcall SetWindowTextW(long wstr)
@ stdcall SetWindowWord(long long long) ; Direct call NtUserSetWindowWord
@ stdcall SetWindowsHookA(long ptr)
@ stdcall SetWindowsHookExA(long long long long)
@ stdcall SetWindowsHookExW(long long long long)
@ stdcall SetWindowsHookW(long ptr)
@ stdcall ShowCaret(long) NtUserShowCaret
@ stdcall ShowCursor(long)
@ stdcall ShowOwnedPopups(long long)
@ stdcall ShowScrollBar(long long long) NtUserShowScrollBar
@ stdcall ShowStartGlass(long)
@ stdcall ShowWindow(long long) NtUserShowWindow
@ stdcall ShowWindowAsync(long long) NtUserShowWindowAsync
@ stdcall SoftModalMessageBox(ptr)
@ stdcall SubtractRect(ptr ptr ptr)
@ stdcall SwapMouseButton(long)
@ stdcall SwitchDesktop(long) NtUserSwitchDesktop
@ stdcall SwitchToThisWindow(long long)
@ stdcall SystemParametersInfoA(long long ptr long)
@ stdcall SystemParametersInfoW(long long ptr long)
@ stdcall TabbedTextOutA(long long long str long long ptr long)
@ stdcall TabbedTextOutW(long long long wstr long long ptr long)
@ stdcall TileChildWindows(long long)
@ stdcall TileWindows(long long ptr long ptr)
@ stdcall ToAscii(long long ptr ptr long)
@ stdcall ToAsciiEx(long long ptr ptr long long)
@ stdcall ToUnicode(long long ptr ptr long long)
@ stdcall ToUnicodeEx(long long ptr ptr long long long)
@ stdcall TrackMouseEvent(ptr) NtUserTrackMouseEvent
@ stdcall TrackPopupMenu(long long long long long long ptr)
@ stdcall TrackPopupMenuEx(long long long long long ptr) NtUserTrackPopupMenuEx
@ stdcall TranslateAccelerator(long long ptr) TranslateAcceleratorA
@ stdcall TranslateAcceleratorA(long long ptr)
@ stdcall TranslateAcceleratorW(long long ptr)
@ stdcall TranslateMDISysAccel(long ptr)
@ stdcall TranslateMessage(ptr)
@ stdcall TranslateMessageEx(ptr long)
@ stdcall UnhookWinEvent(long) NtUserUnhookWinEvent
@ stdcall UnhookWindowsHook(long ptr)
@ stdcall UnhookWindowsHookEx(long) NtUserUnhookWindowsHookEx
@ stdcall UnionRect(ptr ptr ptr)
@ stdcall UnloadKeyboardLayout(ptr)
@ stdcall UnlockWindowStation(long) NtUserUnlockWindowStation
@ stdcall UnpackDDElParam(long long ptr ptr)
@ stdcall UnregisterClassA(str long)
@ stdcall UnregisterClassW(wstr long)
@ stdcall UnregisterDeviceNotification(long)
@ stdcall UnregisterHotKey(long long) NtUserUnregisterHotKey
@ stdcall UnregisterMessagePumpHook()
@ stdcall UnregisterUserApiHook() NtUserUnregisterUserApiHook
@ stdcall UpdateLayeredWindow(long long ptr ptr long ptr long ptr long)
@ stdcall UpdateLayeredWindowIndirect(long ptr)
@ stdcall UpdatePerUserSystemParameters(long long)
@ stdcall UpdateWindow(long)
@ stdcall User32InitializeImmEntryTable(ptr)
@ stdcall UserClientDllInitialize(ptr long ptr) DllMain
@ stdcall UserHandleGrantAccess(ptr ptr long) NtUserUserHandleGrantAccess
@ stdcall UserLpkPSMTextOut(long long long wstr long long)
@ stdcall UserLpkTabbedTextOut(long long long long long long long long long long long long)
@ stdcall UserRealizePalette(long)
@ stdcall UserRegisterWowHandlers(ptr ptr)
# VRipOutput
# VTagOutput
@ stdcall ValidateRect(long ptr) NtUserValidateRect
@ stdcall ValidateRgn(long long)
@ stdcall VkKeyScanA(long)
@ stdcall VkKeyScanExA(long long)
@ stdcall VkKeyScanExW(long long)
@ stdcall VkKeyScanW(long)
@ stdcall WCSToMBEx(long wstr long str long long)
@ stdcall WINNLSEnableIME(long long)
@ stdcall WINNLSGetEnableStatus(long)
@ stdcall WINNLSGetIMEHotkey(long)
@ stdcall WaitForInputIdle(long long)
@ stdcall WaitMessage() NtUserWaitMessage
@ stdcall Win32PoolAllocationStats(long long long long long)
@ stdcall WinHelpA(long str long long)
@ stdcall WinHelpW(long wstr long long)
@ stdcall WindowFromDC(long)
@ stdcall WindowFromPoint(double)
@ stdcall keybd_event(long long long long)
@ stdcall mouse_event(long long long long long)
@ varargs wsprintfA(ptr str)
@ varargs wsprintfW(ptr wstr)
@ stdcall wvsprintfA(ptr str ptr)
@ stdcall wvsprintfW(ptr wstr ptr)
