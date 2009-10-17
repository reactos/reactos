# GDI driver

@ cdecl AlphaBlend(ptr long long long long ptr long long long long long) RosDrv_AlphaBlend
@ cdecl Arc(ptr long long long long long long long long) RosDrv_Arc
@ cdecl BitBlt(ptr long long long long ptr long long long) RosDrv_BitBlt
@ cdecl ChoosePixelFormat(ptr ptr) RosDrv_ChoosePixelFormat
@ cdecl Chord(ptr long long long long long long long long) RosDrv_Chord
@ cdecl CreateBitmap(ptr long ptr) RosDrv_CreateBitmap
@ cdecl CreateDC(long ptr wstr wstr wstr ptr) RosDrv_CreateDC
@ cdecl CreateDIBSection(ptr long ptr long) RosDrv_CreateDIBSection
@ cdecl DeleteBitmap(long) RosDrv_DeleteBitmap
@ cdecl DeleteDC(ptr) RosDrv_DeleteDC
@ cdecl DescribePixelFormat(ptr long long ptr) RosDrv_DescribePixelFormat
@ cdecl Ellipse(ptr long long long long) RosDrv_Ellipse
@ cdecl EnumDeviceFonts(ptr ptr ptr long) RosDrv_EnumDeviceFonts
@ cdecl ExtEscape(ptr long long ptr long ptr) RosDrv_ExtEscape
@ cdecl ExtFloodFill(ptr long long long long) RosDrv_ExtFloodFill
@ cdecl ExtTextOut(ptr long long long ptr ptr long ptr) RosDrv_ExtTextOut
@ cdecl GetBitmapBits(long ptr long) RosDrv_GetBitmapBits
@ cdecl GetCharWidth(ptr long long ptr) RosDrv_GetCharWidth
@ cdecl GetDCOrgEx(ptr ptr) RosDrv_GetDCOrgEx
@ cdecl GetDIBits(ptr long long long ptr ptr long) RosDrv_GetDIBits
@ cdecl GetDeviceCaps(ptr long) RosDrv_GetDeviceCaps
@ cdecl GetDeviceGammaRamp(ptr ptr) RosDrv_GetDeviceGammaRamp
@ cdecl GetICMProfile(ptr ptr ptr) RosDrv_GetICMProfile
@ cdecl GetNearestColor(ptr long) RosDrv_GetNearestColor
@ cdecl GetPixel(ptr long long) RosDrv_GetPixel
@ cdecl GetPixelFormat(ptr) RosDrv_GetPixelFormat
@ cdecl GetSystemPaletteEntries(ptr long long ptr) RosDrv_GetSystemPaletteEntries
@ cdecl GetTextExtentExPoint(ptr ptr long long ptr ptr ptr) RosDrv_GetTextExtentExPoint
@ cdecl GetTextMetrics(ptr ptr) RosDrv_GetTextMetrics
@ cdecl LineTo(ptr long long) RosDrv_LineTo
@ cdecl PaintRgn(ptr long) RosDrv_PaintRgn
@ cdecl PatBlt(ptr long long long long long) RosDrv_PatBlt
@ cdecl Pie(ptr long long long long long long long long) RosDrv_Pie
@ cdecl PolyPolygon(ptr ptr ptr long) RosDrv_PolyPolygon
@ cdecl PolyPolyline(ptr ptr ptr long) RosDrv_PolyPolyline
@ cdecl Polygon(ptr ptr long) RosDrv_Polygon
@ cdecl Polyline(ptr ptr long) RosDrv_Polyline
@ cdecl RealizeDefaultPalette(ptr) RosDrv_RealizeDefaultPalette
@ cdecl RealizePalette(ptr long long) RosDrv_RealizePalette
@ cdecl Rectangle(ptr long long long long) RosDrv_Rectangle
@ cdecl RoundRect(ptr long long long long long long) RosDrv_RoundRect
@ cdecl SelectBitmap(ptr long) RosDrv_SelectBitmap
@ cdecl SelectBrush(ptr long) RosDrv_SelectBrush
@ cdecl SelectFont(ptr long long) RosDrv_SelectFont
@ cdecl SelectPen(ptr long) RosDrv_SelectPen
@ cdecl SetBitmapBits(long ptr long) RosDrv_SetBitmapBits
@ cdecl SetBkColor(ptr long) RosDrv_SetBkColor
@ cdecl SetDCBrushColor(ptr long) RosDrv_SetDCBrushColor
@ cdecl SetDCOrg(ptr long long) RosDrv_SetDCOrg
@ cdecl SetDCPenColor(ptr long) RosDrv_SetDCPenColor
@ cdecl SetDIBColorTable(ptr long long ptr) RosDrv_SetDIBColorTable
@ cdecl SetDIBits(ptr long long long ptr ptr long) RosDrv_SetDIBits
@ cdecl SetDIBitsToDevice(ptr long long long long long long long long ptr ptr long) RosDrv_SetDIBitsToDevice
@ cdecl SetDeviceClipping(ptr long long) RosDrv_SetDeviceClipping
@ cdecl SetDeviceGammaRamp(ptr ptr) RosDrv_SetDeviceGammaRamp
@ cdecl SetPixel(ptr long long long) RosDrv_SetPixel
@ cdecl SetPixelFormat(ptr long ptr) RosDrv_SetPixelFormat
@ cdecl SetTextColor(ptr long) RosDrv_SetTextColor
@ cdecl StretchBlt(ptr long long long long ptr long long long long long) RosDrv_StretchBlt
@ cdecl SwapBuffers(ptr) RosDrv_SwapBuffers
@ cdecl UnrealizePalette(long) RosDrv_UnrealizePalette

# USER driver

@ cdecl ActivateKeyboardLayout(long long) RosDrv_ActivateKeyboardLayout
@ cdecl Beep() RosDrv_Beep
@ cdecl GetAsyncKeyState(long) RosDrv_GetAsyncKeyState
@ cdecl GetKeyNameText(long ptr long) RosDrv_GetKeyNameText
@ cdecl GetKeyboardLayout(long) RosDrv_GetKeyboardLayout
@ cdecl GetKeyboardLayoutName(ptr) RosDrv_GetKeyboardLayoutName
@ cdecl LoadKeyboardLayout(wstr long) RosDrv_LoadKeyboardLayout
@ cdecl MapVirtualKeyEx(long long long) RosDrv_MapVirtualKeyEx
@ cdecl SendInput(long ptr long) RosDrv_SendInput
@ cdecl ToUnicodeEx(long long ptr ptr long long long) RosDrv_ToUnicodeEx
@ cdecl UnloadKeyboardLayout(long) RosDrv_UnloadKeyboardLayout
@ cdecl VkKeyScanEx(long long) RosDrv_VkKeyScanEx
@ cdecl SetCursor(ptr) RosDrv_SetCursor
@ cdecl GetCursorPos(ptr) RosDrv_GetCursorPos
@ cdecl SetCursorPos(long long) RosDrv_SetCursorPos
@ cdecl ClipCursor(ptr) RosDrv_ClipCursor
@ cdecl GetScreenSaveActive() RosDrv_GetScreenSaveActive
@ cdecl SetScreenSaveActive(long) RosDrv_SetScreenSaveActive
@ cdecl ChangeDisplaySettingsEx(ptr ptr long long long) RosDrv_ChangeDisplaySettingsEx
@ cdecl EnumDisplayMonitors(long ptr ptr long) RosDrv_EnumDisplayMonitors
@ cdecl EnumDisplaySettingsEx(ptr long ptr long) RosDrv_EnumDisplaySettingsEx
@ cdecl GetMonitorInfo(long ptr) RosDrv_GetMonitorInfo
@ cdecl AcquireClipboard(long) RosDrv_AcquireClipboard
@ cdecl CountClipboardFormats() RosDrv_CountClipboardFormats
@ cdecl CreateDesktopWindow(long) RosDrv_CreateDesktopWindow
@ cdecl CreateWindow(long) RosDrv_CreateWindow
@ cdecl DestroyWindow(long) RosDrv_DestroyWindow
@ cdecl EmptyClipboard(long) RosDrv_EmptyClipboard
@ cdecl EndClipboardUpdate() RosDrv_EndClipboardUpdate
@ cdecl EnumClipboardFormats(long) RosDrv_EnumClipboardFormats
@ cdecl GetClipboardData(long ptr ptr) RosDrv_GetClipboardData
@ cdecl GetClipboardFormatName(long ptr long) RosDrv_GetClipboardFormatName
@ cdecl GetDC(long long long ptr ptr long) RosDrv_GetDC
@ cdecl IsClipboardFormatAvailable(long) RosDrv_IsClipboardFormatAvailable
@ cdecl MsgWaitForMultipleObjectsEx(long ptr long long long) RosDrv_MsgWaitForMultipleObjectsEx
@ cdecl RegisterClipboardFormat(wstr) RosDrv_RegisterClipboardFormat
@ cdecl ReleaseDC(long long) RosDrv_ReleaseDC
@ cdecl ScrollDC(long long long ptr ptr long ptr) RosDrv_ScrollDC
@ cdecl SetClipboardData(long long long long) RosDrv_SetClipboardData
@ cdecl SetCapture(long long) RosDrv_SetCapture
@ cdecl SetFocus(long) RosDrv_SetFocus
@ cdecl SetLayeredWindowAttributes(long long long long) RosDrv_SetLayeredWindowAttributes
@ cdecl SetParent(long long long) RosDrv_SetParent
@ cdecl SetWindowIcon(long long long) RosDrv_SetWindowIcon
@ cdecl SetWindowRgn(long long long) RosDrv_SetWindowRgn
@ cdecl SetWindowStyle(ptr long ptr) RosDrv_SetWindowStyle
@ cdecl SetWindowText(long wstr) RosDrv_SetWindowText
@ cdecl ShowWindow(long long ptr long) RosDrv_ShowWindow
@ cdecl SysCommand(long long long) RosDrv_SysCommand
@ cdecl WindowMessage(long long long long) RosDrv_WindowMessage
@ cdecl WindowPosChanging(long long long ptr ptr ptr) RosDrv_WindowPosChanging
@ cdecl WindowPosChanged(long long long ptr ptr ptr ptr) RosDrv_WindowPosChanged

# WinTab32
#@ cdecl AttachEventQueueToTablet(long) RosDrv_AttachEventQueueToTablet
#@ cdecl GetCurrentPacket(ptr) RosDrv_GetCurrentPacket
#@ cdecl LoadTabletInfo(long) RosDrv_LoadTabletInfo
#@ cdecl WTInfoW(long long ptr) RosDrv_WTInfoW

# X11 locks
@ cdecl -norelay wine_tsx11_lock()
@ cdecl -norelay wine_tsx11_unlock()

# Desktop
@ cdecl wine_create_desktop(long long) RosDrv_create_desktop

# System tray
@ cdecl wine_notify_icon(long ptr)

# OpenGL
#@ cdecl wglCopyContext(long long long) RosDrv_wglCopyContext
#@ cdecl wglCreateContext(ptr) RosDrv_wglCreateContext
#@ cdecl wglDeleteContext(long) RosDrv_wglDeleteContext
#@ cdecl wglGetProcAddress(str) RosDrv_wglGetProcAddress
#@ cdecl wglGetPbufferDCARB(ptr ptr) RosDrv_wglGetPbufferDCARB
#@ cdecl wglMakeContextCurrentARB(ptr ptr long) RosDrv_wglMakeContextCurrentARB
#@ cdecl wglMakeCurrent(ptr long) RosDrv_wglMakeCurrent
#@ cdecl wglSetPixelFormatWINE(ptr long ptr) RosDrv_wglSetPixelFormatWINE
#@ cdecl wglShareLists(long long) RosDrv_wglShareLists
#@ cdecl wglUseFontBitmapsA(ptr long long long) RosDrv_wglUseFontBitmapsA
#@ cdecl wglUseFontBitmapsW(ptr long long long) RosDrv_wglUseFontBitmapsW

#IME Interface
#@ stdcall ImeInquire(ptr wstr wstr)
#@ stdcall ImeConfigure(long long long ptr)
#@ stdcall ImeDestroy(long)
#@ stdcall ImeEscape(long long ptr)
#@ stdcall ImeSelect(long long)
#@ stdcall ImeSetActiveContext(long long)
#@ stdcall ImeToAsciiEx(long long ptr ptr long long)
#@ stdcall NotifyIME(long long long long)
#@ stdcall ImeRegisterWord(wstr long wstr)
#@ stdcall ImeUnregisterWord(wstr long wstr)
#@ stdcall ImeEnumRegisterWord(ptr wstr long wstr ptr)
#@ stdcall ImeSetCompositionString(long long ptr long ptr long)
#@ stdcall ImeConversionList(long wstr ptr long long)
#@ stdcall ImeProcessKey(long long long ptr)
#@ stdcall ImeGetRegisterWordStyle(long ptr)
#@ stdcall ImeGetImeMenuItems(long long long ptr ptr long)
