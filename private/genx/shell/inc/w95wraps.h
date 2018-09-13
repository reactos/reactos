/*****************************************************************************\
*                                                                             *
* w95wraps.h - Unicode wrappers for ANSI functions on Win95                   *
*                                                                             *
* Version 1.0                                                                 *
*                                                                             *
* Copyright (c) 1991-1998, Microsoft Corp.      All rights reserved.          *
*                                                                             *
\*****************************************************************************/

//
//  This file is for internal use only.  Do not put it in the SDK.
//

#ifndef _INC_W95WRAPS
#define _INC_W95WRAPS

#if defined(UNIX)
#define NO_W95WRAPS_UNITHUNK
#define GetSaveFileNameW GetSaveFileNameWrapW
#define GetOpenFileNameW GetOpenFileNameWrapW
#define PrintDlgW PrintDlgWrapW
#define PageSetupDlgW PageSetupDlgWrapW
#define GetMenuItemInfoW GetMenuItemInfoWrapW
#endif
//
// Users of this header may define any number of these constants to avoid
// the definitions of each functional group.
//
//    NO_W95WRAPS_UNITHUNK  Unicode wrapper functions
//    NO_W95WRAPS_TPS       Thread Pool Services
//    NO_W95WRAPS_MLUI      MLUI wrapper functions
//
//  You are expected to have done a #include <shlwapi.h> before including
//  this file.
//
// BUGBUG: you can't include shlwapi.h before this, as ATL templates
// require things like TranlsateAccelerator which, due to munging, get
// turned into TranslateAcceleratorWrapW at compile time.  So w95wraps.h
// pretty much needs to be included FIRST so all the interface definitions
// get munged too.
//#ifndef _INC_SHLWAPI
//#error "You must include shlwapi.h *before* w95wraps.h
//#endif

//=============== Unicode Wrapper Routines ===================================

#ifndef NO_W95WRAPS_UNITHUNK

//
//  If you #include this file, then calls to many UNICODE functions
//  are re-routed through wrapper functions in SHLWAPI which will
//  either call the UNICODE version (on NT) or thunk the parameters
//  to ANSI and call the ANSI version (on 9x).
//
//  Note that these wrapper functions must be used with care, because
//
//  *   They do not account for subtle differences between UNICODE and
//      ANSI versions of the same API.  Examples:
//
//      -   RegisterClassW register a UNICODE window class, whereas
//          RegisterClassA registers an ANSI window class.  Consequently,
//          if you use RegisterClassWrapW, your WNDPROC will receive
//          *ANSI* WM_SETTEXT messages on Windows 9x.
//
//      -   SetWindowLongW(GWL_WNDPROC) and CallWindowProcW behave
//          very differently from their ANSI counterparts.
//
//      -   DialogBoxW will send your dialog procedure UNICODE window
//          messages, whereas DialogBoxA will send ANSI window messages.
//
//      -   Anything that manipulates window messages or MSG structures
//          will be subtly affected by character set discrepancies.
//
//  *   Not all features of the underlying API are always supported,
//      or are supported with restrictions.  Examples:
//
//      -   DialogBoxWrapW does not support named dialog resources.
//
//      -   AppendMenuWrapW does not support bitmap or owner-draw
//          menu items.
//
//      -   FormatMessageWrapW does not support insertions.
//
//      -   If you use RegQueryValueExWrapW with a NULL output buffer to
//          query the size of a buffer, you must also pass *lpcbData = 0.
//
//      -   SendMessageWrapW requires that the window message not conflict
//          with messages used by any of the Windows common controls.
//

#define IsCharAlphaW                IsCharAlphaWrapW
#define IsCharUpperW                IsCharUpperWrapW
#define IsCharLowerW                IsCharLowerWrapW
#define IsCharAlphaNumericW         IsCharAlphaNumericWrapW

#define AppendMenuW                 AppendMenuWrapW
#define CallMsgFilterW              CallMsgFilterWrapW
#define CallWindowProcW             CallWindowProcWrapW
#define CharLowerW                  CharLowerWrapW
#define CharLowerBuffW              CharLowerBuffWrapW
#define CharNextW                   CharNextWrapW
#define CharPrevW                   CharPrevWrapW
#define CharToOemW                  CharToOemWrapW
#define CharUpperW                  CharUpperWrapW
#define CharUpperBuffW              CharUpperBuffWrapW
#define CompareStringW              CompareStringWrapW
#define CopyAcceleratorTableW       CopyAcceleratorTableWrapW
#define CreateAcceleratorTableW     CreateAcceleratorTableWrapW
#define CreateDCW                   CreateDCWrapW
#define CreateDirectoryW            CreateDirectoryWrapW
#define CreateEventW                CreateEventWrapW
#define CreateFileW                 CreateFileWrapW
#define CreateFontW                 CreateFontWrapW
#define CreateFontIndirectW         CreateFontIndirectWrapW
#define CreateMetaFileW             CreateMetaFileWrapW
#define CreateMutexW                CreateMutexWrapW
#define CreateICW                   CreateICWrapW
#define CreateSemaphoreW            CreateSemaphoreWrapW
#define CreateWindowExW             CreateWindowExWrapW
#define GetFileVersionInfoSizeW     GetFileVersionInfoSizeWrapW
#define GetFileVersionInfoW         GetFileVersionInfoWrapW
#define VerQueryValueW              VerQueryValueWrapW

#ifndef NO_W95_ATL_WRAPS_TBS
// #define NO_W95_ATL_WRAPS_TBS if you use ATL.
#define DefWindowProcW              DefWindowProcWrapW
#endif // NO_W95_ATL_WRAPS_TBS

#define DeleteFileW                 DeleteFileWrapW
#define DispatchMessageW            DispatchMessageWrapW
#define DragQueryFileW              DragQueryFileWrapW
#define DrawTextExW                 DrawTextExWrapW
#define DrawTextW                   DrawTextWrapW
#define EnumFontFamiliesW           EnumFontFamiliesWrapW
#define EnumFontFamiliesExW         EnumFontFamiliesExWrapW
#define EnumResourceNamesW          EnumResourceNamesWrapW
#define ExpandEnvironmentStringsW   ExpandEnvironmentStringsWrapW
#define ExtractIconExW              ExtractIconExWrapW
#define ExtTextOutW                 ExtTextOutWrapW
#define FindFirstFileW              FindFirstFileWrapW
#define FindResourceW               FindResourceWrapW
#define FindNextFileW               FindNextFileWrapW
#define FindWindowW                 FindWindowWrapW
#define FindWindowExW               FindWindowExWrapW
#define FormatMessageW              FormatMessageWrapW
#ifndef NO_W95_GETCLASSINFO_WRAPS
// #define NO_W95_GETCLASSINFO_WRAPS if one of the objects uses IProvideClassInfo which has a GetClassInfo method.
#define GetClassInfoW               GetClassInfoWrapW
#define GetClassInfoExW             GetClassInfoExWrapW
#endif  // NO_W95_GETCLASSINFO_WRAPS
#define GetClassLongW               GetClassLongWrapW
#define GetClassNameW               GetClassNameWrapW
#define GetClipboardFormatNameW     GetClipboardFormatNameWrapW
#define GetCurrentDirectoryW        GetCurrentDirectoryWrapW
#define GetDlgItemTextW             GetDlgItemTextWrapW
#define GetFileAttributesW          GetFileAttributesWrapW
#define GetFullPathNameW            GetFullPathNameWrapW
#define GetLocaleInfoW              GetLocaleInfoWrapW
#define GetMenuItemInfoW            GetMenuItemInfoWrapW
#define GetMenuStringW              GetMenuStringWrapW
#define GetMessageW                 GetMessageWrapW
#define GetModuleFileNameW          GetModuleFileNameWrapW
#define GetNumberFormatW            GetNumberFormatWrapW
#define GetSystemDirectoryW         GetSystemDirectoryWrapW
#define GetModuleHandleW            GetModuleHandleWrapW
#define GetObjectW                  GetObjectWrapW
#define GetPrivateProfileIntW       GetPrivateProfileIntWrapW
#define GetPrivateProfileStringW    GetPrivateProfileStringWrapW
#define GetProfileStringW           GetProfileStringWrapW
#define GetPropW                    GetPropWrapW
#define GetShortPathNameW           GetShortPathNameWrapW
#define GetLongPathNameW            GetLongPathNameWrapW
#define GetLongPathNameA            GetLongPathNameWrapA
#define GetStringTypeExW            GetStringTypeExWrapW
#define GetTempFileNameW            GetTempFileNameWrapW
#define GetTempPathW                GetTempPathWrapW
#define GetTextExtentPoint32W       GetTextExtentPoint32WrapW
#define GetTextFaceW                GetTextFaceWrapW
#define GetTextMetricsW             GetTextMetricsWrapW
#define GetTimeFormatW              GetTimeFormatWrapW
#define GetDateFormatW              GetDateFormatWrapW
#define GetUserNameW                GetUserNameWrapW
#define GetWindowLongW              GetWindowLongWrapW
#define GetEnvironmentVariableW     GetEnvironmentVariableWrapW

#ifndef NO_W95_ATL_WRAPS_TBS
// #define NO_W95_ATL_WRAPS_TBS if you use ATL.
#define GetWindowTextW              GetWindowTextWrapW
#endif // NO_W95_ATL_WRAPS_TBS

#define GetWindowTextLengthW        GetWindowTextLengthWrapW
#define GetWindowsDirectoryW        GetWindowsDirectoryWrapW
#define InsertMenuW                 InsertMenuWrapW
#define InsertMenuItemW             InsertMenuItemWrapW
#define IsBadStringPtrW             IsBadStringPtrWrapW
#define IsDialogMessageW            IsDialogMessageWrapW
#define LoadAcceleratorsW           LoadAcceleratorsWrapW
#define LoadBitmapW                 LoadBitmapWrapW
#define LoadCursorW                 LoadCursorWrapW
#define LoadIconW                   LoadIconWrapW
#define LoadImageW                  LoadImageWrapW
#define LoadLibraryW                LoadLibraryWrapW
#define LoadLibraryExW              LoadLibraryExWrapW
#define LoadMenuW                   LoadMenuWrapW
#define LoadStringW                 LoadStringWrapW
#define MessageBoxIndirectW         MessageBoxIndirectWrapW
#define MessageBoxW                 MessageBoxWrapW
#define ModifyMenuW                 ModifyMenuWrapW
#define GetCharWidth32W             GetCharWidth32WrapW
#define GetCharacterPlacementW      GetCharacterPlacementWrapW
#define CopyFileW                   CopyFileWrapW
#define MoveFileW                   MoveFileWrapW
#define OemToCharW                  OemToCharWrapW
#define OpenEventW                  OpenEventWrapW
#define OutputDebugStringW          OutputDebugStringWrapW
#define PeekMessageW                PeekMessageWrapW
#define PostMessageW                PostMessageWrapW
#define PostThreadMessageW          PostThreadMessageWrapW
#define RegCreateKeyW               RegCreateKeyWrapW
#define RegCreateKeyExW             RegCreateKeyExWrapW
#define RegDeleteKeyW               RegDeleteKeyWrapW
#define RegDeleteValueW             RegDeleteValueWrapW
#define RegEnumKeyW                 RegEnumKeyWrapW
#define RegEnumKeyExW               RegEnumKeyExWrapW
#define RegOpenKeyW                 RegOpenKeyWrapW
#define RegOpenKeyExW               RegOpenKeyExWrapW
#define RegQueryInfoKeyW            RegQueryInfoKeyWrapW
#define RegQueryValueW              RegQueryValueWrapW
#define RegQueryValueExW            RegQueryValueExWrapW
#define RegSetValueW                RegSetValueWrapW
#define RegSetValueExW              RegSetValueExWrapW
#define RegisterClassW              RegisterClassWrapW
#define RegisterClassExW            RegisterClassExWrapW
#define RegisterClipboardFormatW    RegisterClipboardFormatWrapW
#define RegisterWindowMessageW      RegisterWindowMessageWrapW
#define RemoveDirectoryW            RemoveDirectoryWrapW
#define RemovePropW                 RemovePropWrapW
#define SearchPathW                 SearchPathWrapW
#define SendDlgItemMessageW         SendDlgItemMessageWrapW
#define SendMessageW                SendMessageWrapW
#define SendMessageTimeoutW         SendMessageTimeoutWrapW
#define SetCurrentDirectoryW        SetCurrentDirectoryWrapW
#define SetDlgItemTextW             SetDlgItemTextWrapW
#define SetMenuItemInfoW            SetMenuItemInfoWrapW
#define SetPropW                    SetPropWrapW
#define SetFileAttributesW          SetFileAttributesWrapW
#define SetWindowLongW              SetWindowLongWrapW
#define SHGetFileInfoW              SHGetFileInfoWrapW
#define SHBrowseForFolderW          SHBrowseForFolderWrapW
#define ShellExecuteExW             ShellExecuteExWrapW
#define SHFileOperationW            SHFileOperationWrapW
#define SHGetNewLinkInfoW           SHGetNewLinkInfoWrapW
#define SHDefExtractIconW           SHDefExtractIconWrapW
#define GetUserNameW                GetUserNameWrapW
#define RegEnumValueW               RegEnumValueWrapW
#define WritePrivateProfileStructW  WritePrivateProfileStructWrapW
#define GetPrivateProfileStructW    GetPrivateProfileStructWrapW
#define CreateProcessW              CreateProcessWrapW
#define ExtractIconW                ExtractIconWrapW
#define DdeInitializeW              DdeInitializeWrapW
#define DdeCreateStringHandleW      DdeCreateStringHandleWrapW
#define DdeQueryStringW             DdeQueryStringWrapW
#define GetSaveFileNameW            GetSaveFileNameWrapW
#define GetOpenFileNameW            GetOpenFileNameWrapW
#define SHChangeNotify              SHChangeNotifyWrap
#define SHFlushSFCache              SHFlushSFCacheWrap
#define PageSetupDlgW               PageSetupDlgWrapW
#define PrintDlgW                   PrintDlgWrapW
#define SHGetPathFromIDListW        SHGetPathFromIDListWrapW
#define SetWindowsHookExW           SetWindowsHookExWrapW
#define SetWindowTextW              SetWindowTextWrapW
#define StartDocW                   StartDocWrapW
#define SystemParametersInfoW       SystemParametersInfoWrapW

#define TrackPopupMenu              TrackPopupMenuWrap
#define TrackPopupMenuEx            TrackPopupMenuExWrap

#ifndef NO_W95_TRANSACCEL_WRAPS_TBS
// #define NO_W95_TRANSACCEL_WRAPS_TBS if one of the objects uses IOleInPlaceActiveObject which has a TranslateAccelerator method.
#define TranslateAcceleratorW       TranslateAcceleratorWrapW
#endif // NO_W95_TRANSACCEL_WRAPS_TBS

#define UnregisterClassW            UnregisterClassWrapW
#define VkKeyScanW                  VkKeyScanWrapW
#define WinHelpW                    WinHelpWrapW
#define WritePrivateProfileStringW  WritePrivateProfileStringWrapW
#define wvsprintfW                  wvsprintfWrapW
#define WNetRestoreConnectionW      WNetRestoreConnectionWrapW
#define WNetGetLastErrorW           WNetGetLastErrorWrapW

#endif // NO_W95WRAPS_UNITHUNK

#if !defined(NO_W95WRAPS_UNITHUNK) || !defined(NO_W95WRAPS_MLUI)

#define CreateDialogIndirectParamW  CreateDialogIndirectParamWrapW
#define CreateDialogParamW          CreateDialogParamWrapW
#define DialogBoxIndirectParamW     DialogBoxIndirectParamWrapW                 // UNICODE, ML
//#ifdef DialogBoxIndirectW
//#undef DialogBoxIndirectW
//#endif
//#define DialogBoxIndirectW(i,h,w,f) DialogBoxIndirectParamWrapW(i,h,w,f,d,0)    // UNICODE, ML
#define DialogBoxParamW             DialogBoxParamWrapW                         // UNICODE, ML
//#ifdef DialogBoxW
//#undef DialogBoxW
//#endif
//#define DialogBoxW(i,t,w,f)         DialogBoxParamWrapW(i,t,w,f,0)              // UNICODE, ML
#define ShellMessageBoxW            ShellMessageBoxWrapW

#define DeleteMenu                  DeleteMenuWrap
#define DestroyMenu                 DestroyMenuWrap

#endif // !defined(NO_W95WRAPS_UNITHUNK) || !defined(NO_W95WRAPS_MLUI)


#if !defined(NO_OLE32_WRAPS)
#define CLSIDFromString             CLSIDFromStringWrap
#define CLSIDFromProgID             CLSIDFromProgIDWrap
#endif

#endif
