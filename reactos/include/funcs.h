/* 
   Functions.h

   Declarations for all the Windows32 API Functions

   Copyright (C) 1996, 1997 Free Software Foundation, Inc.

   Author: Scott Christley <scottc@net-community.com>

   This file is part of the Windows32 API Library.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   If you are interested in a warranty or support for this source code,
   contact Scott Christley <scottc@net-community.com> for more information.
   
   You should have received a copy of the GNU Library General Public
   License along with this library; see the file COPYING.LIB.
   If not, write to the Free Software Foundation, 
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/ 

#ifndef _GNU_H_WINDOWS32_FUNCTIONS
#define _GNU_H_WINDOWS32_FUNCTIONS

#ifndef WIN32_LEAN_AND_MEAN

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef
DWORD (CALLBACK *PPROGRESS_ROUTINE)(
	LARGE_INTEGER TotalFileSize,
	LARGE_INTEGER TotalBytesTransferred,
	LARGE_INTEGER StreamSize,
	LARGE_INTEGER StreamBytesTransferred,
	DWORD StreamNumber,
	DWORD CallbackReason,
	HANDLE SourceFile,
	HANDLE DestinationFile,
	LPVOID UserData);
typedef PPROGRESS_ROUTINE LPPROGRESS_ROUTINE;

/* These functions were a real pain, having to figure out which
   had Unicode/Ascii versions and which did not */

#ifndef UNICODE_ONLY
#include <unicode.h>
#endif /* !UNICODE_ONLY */

#ifndef ANSI_ONLY
#include <ascii.h>
#endif /* !ANSI_ONLY */

/* Define the approprate declaration based upon UNICODE or ASCII */

#ifndef _DISABLE_TIDENTS
/* UNICODE */
#ifdef UNICODE

#define RegConnectRegistry  RegConnectRegistryW
#define RegCreateKey  RegCreateKeyW
#define RegCreateKeyEx  RegCreateKeyExW
#define RegDeleteKey  RegDeleteKeyW
#define RegDeleteValue  RegDeleteValueW
#define RegEnumKey  RegEnumKeyW
#define RegEnumKeyEx  RegEnumKeyExW
#define RegEnumValue  RegEnumValueW
#define RegLoadKey  RegLoadKeyW
#define RegOpenKey  RegOpenKeyW
#define RegOpenKeyEx  RegOpenKeyExW
#define RegQueryInfoKey  RegQueryInfoKeyW
#define RegQueryValue  RegQueryValueW
#define RegQueryMultipleValues  RegQueryMultipleValuesW
#define RegQueryValueEx  RegQueryValueExW
#define RegReplaceKey  RegReplaceKeyW
#define RegRestoreKey  RegRestoreKeyW
#define RegSaveKey  RegSaveKeyW
#define RegSetValue  RegSetValueW
#define ChangeServiceConfig2 ChangeServiceConfig2W
#define EnumServicesStatusEx EnumServicesStatusExW
#define QueryServiceConfig2 QueryServiceConfig2W
#define RegisterServiceCtrlHandlerEx RegisterServiceCtrlHandlerExW
#define RegSetValueEx  RegSetValueExW
#define AbortSystemShutdown  AbortSystemShutdownW
#define InitiateSystemShutdown  InitiateSystemShutdownW
#define RegUnLoadKey  RegUnLoadKeyW
#define SetProp  SetPropW
#define GetProp  GetPropW
#define RemoveProp  RemovePropW
#define EnumPropsEx  EnumPropsExW
#define EnumProps  EnumPropsW
#define SetWindowText  SetWindowTextW
#define GetWindowText  GetWindowTextW
#define GetWindowTextLength  GetWindowTextLengthW
#define MessageBox  MessageBoxW
#define MessageBoxEx  MessageBoxExW
#define MessageBoxIndirect  MessageBoxIndirectW
#define GetWindowLong  GetWindowLongW
#define SetWindowLong  SetWindowLongW
#define GetClassLong  GetClassLongW
#define SetClassLong  SetClassLongW
#define FindWindow  FindWindowW
#define FindWindowEx  FindWindowExW
#define GetClassName  GetClassNameW
#define SetWindowsHookEx  SetWindowsHookExW
#define SetWindowsHook  SetWindowsHookW
#define LoadBitmap  LoadBitmapW
#define LoadCursor  LoadCursorW
#define LoadCursorFromFile  LoadCursorFromFileW
#define LoadIcon  LoadIconW
#define LoadImage  LoadImageW
#define LoadString  LoadStringW
#define IsDialogMessage  IsDialogMessageW
#define DlgDirList  DlgDirListW
#define DlgDirSelectEx  DlgDirSelectExW
#define DlgDirListComboBox  DlgDirListComboBoxW
#define DlgDirSelectComboBoxEx  DlgDirSelectComboBoxExW
#define DefFrameProc  DefFrameProcW
#define DefMDIChildProc  DefMDIChildProcW
#define CreateMDIWindow  CreateMDIWindowW
#define WinHelp  WinHelpW
#define ChangeDisplaySettings  ChangeDisplaySettingsW
#define EnumDisplaySettings  EnumDisplaySettingsW
#define SystemParametersInfo  SystemParametersInfoW
#define AddFontResource  AddFontResourceW
#define CopyMetaFile  CopyMetaFileW
#define CreateDC  CreateDCW
#define CreateFontIndirect  CreateFontIndirectW
#define CreateFont  CreateFontW
#define CreateIC  CreateICW
#define CreateMetaFile  CreateMetaFileW
#define CreateScalableFontResource  CreateScalableFontResourceW
#define DeviceCapabilities  DeviceCapabilitiesW
#define EnumFontFamiliesEx  EnumFontFamiliesExW
#define EnumFontFamilies  EnumFontFamiliesW
#define EnumFonts  EnumFontsW
#define GetCharWidth  GetCharWidthW
#define GetCharWidth32  GetCharWidth32W
#define GetCharWidthFloat  GetCharWidthFloatW
#define GetCharABCWidths  GetCharABCWidthsW
#define GetCharABCWidthsFloat  GetCharABCWidthsFloatW
#define GetGlyphOutline  GetGlyphOutlineW
#define GetMetaFile  GetMetaFileW
#define GetOutlineTextMetrics  GetOutlineTextMetricsW
#define GetTextExtentPoint  GetTextExtentPointW
#define GetTextExtentPoint32  GetTextExtentPoint32W
#define GetTextExtentExPoint  GetTextExtentExPointW
#define GetCharacterPlacement  GetCharacterPlacementW
#define ResetDC  ResetDCW
#define RemoveFontResource  RemoveFontResourceW
#define CopyEnhMetaFile  CopyEnhMetaFileW
#define CreateEnhMetaFile  CreateEnhMetaFileW
#define GetEnhMetaFile  GetEnhMetaFileW
#define GetEnhMetaFileDescription  GetEnhMetaFileDescriptionW
#define GetTextMetrics  GetTextMetricsW
#define StartDoc  StartDocW
#define GetObject  GetObjectW
#define TextOut  TextOutW
#define ExtTextOut  ExtTextOutW
#define PolyTextOut  PolyTextOutW
#define GetTextFace  GetTextFaceW
#define GetKerningPairs  GetKerningPairsW
#define GetLogColorSpace  GetLogColorSpaceW
#define CreateColorSpace  CreateColorSpaceW
#define GetICMProfile  GetICMProfileW
#define SetICMProfile  SetICMProfileW
#define UpdateICMRegKey  UpdateICMRegKeyW
#define EnumICMProfiles  EnumICMProfilesW
#define CreatePropertySheetPage  CreatePropertySheetPageW
#define PropertySheet            PropertySheetW
#define ImageList_LoadImage     ImageList_LoadImageW
#define CreateStatusWindow      CreateStatusWindowW
#define DrawStatusText          DrawStatusTextW
#define GetOpenFileName  GetOpenFileNameW
#define GetSaveFileName  GetSaveFileNameW
#define GetFileTitle  GetFileTitleW
#define ChooseColor  ChooseColorW
#define FindText  FindTextW
#define ReplaceText  ReplaceTextW
#define ChooseFont  ChooseFontW
#define PrintDlg  PrintDlgW
#define PageSetupDlg  PageSetupDlgW
#define DefWindowProc  DefWindowProcW
#define CallWindowProc  CallWindowProcW
#define RegisterClass  RegisterClassW
#define UnregisterClass  UnregisterClassW
#define GetClassInfo  GetClassInfoW
#define RegisterClassEx  RegisterClassExW
#define GetClassInfoEx  GetClassInfoExW
#define CreateWindowEx  CreateWindowExW
#define CreateWindow  CreateWindowW
#define CreateDialogParam  CreateDialogParamW
#define CreateDialogIndirectParam  CreateDialogIndirectParamW
#define CreateDialog  CreateDialogW
#define CreateDialogIndirect  CreateDialogIndirectW
#define DialogBoxParam  DialogBoxParamW
#define DialogBoxIndirectParam  DialogBoxIndirectParamW
#define DialogBox  DialogBoxW
#define DialogBoxIndirect  DialogBoxIndirectW
#define RegisterClipboardFormat  RegisterClipboardFormatW
#define SetDlgItemText  SetDlgItemTextW
#define GetDlgItemText  GetDlgItemTextW
#define SendDlgItemMessage  SendDlgItemMessageW
#define DefDlgProc  DefDlgProcW
#define GetClipboardFormatName  GetClipboardFormatNameW
#define CharToOem  CharToOemW
#define OemToChar  OemToCharW
#define CharToOemBuff  CharToOemBuffW
#define OemToCharBuff  OemToCharBuffW
#define CharUpper  CharUpperW
#define CharUpperBuff  CharUpperBuffW
#define CharLower  CharLowerW
#define CharLowerBuff  CharLowerBuffW
#define CharNext  CharNextW
#define CharPrev  CharPrevW
#define IsCharAlpha  IsCharAlphaW
#define IsCharAlphaNumeric  IsCharAlphaNumericW
#define IsCharUpper  IsCharUpperW
#define IsCharLower  IsCharLowerW
#define GetKeyNameText  GetKeyNameTextW
#define VkKeyScan  VkKeyScanW
#define VkKeyScanEx  VkKeyScanExW
#define MapVirtualKey  MapVirtualKeyW
#define MapVirtualKeyEx  MapVirtualKeyExW
#define FindFirstVolumeMountPoint FindFirstVolumeMountPointW
#define FindNextVolume FindNextVolumeW
#define LoadAccelerators  LoadAcceleratorsW
#define CreateAcceleratorTable  CreateAcceleratorTableW
#define CopyAcceleratorTable  CopyAcceleratorTableW
#define TranslateAccelerator  TranslateAcceleratorW
#define LoadMenu  LoadMenuW
#define LoadMenuIndirect  LoadMenuIndirectW
#define ChangeMenu  ChangeMenuW
#define GetMenuString  GetMenuStringW
#define InsertMenu  InsertMenuW
#define AppendMenu  AppendMenuW
#define ModifyMenu  ModifyMenuW
#define InsertMenuItem  InsertMenuItemW
#define GetMenuItemInfo  GetMenuItemInfoW
#define SetMenuItemInfo  SetMenuItemInfoW
#define DrawText  DrawTextW
#define DrawTextEx  DrawTextExW
#define GrayString  GrayStringW
#define CreateJobObject CreateJobObjectW
#define DrawState  DrawStateW
#define TabbedTextOut  TabbedTextOutW
#define GetTabbedTextExtent  GetTabbedTextExtentW
#define GetVersionEx  GetVersionExW
#define wvsprintf  wvsprintfW
#define wsprintf  wsprintfW
#define LoadKeyboardLayout  LoadKeyboardLayoutW
#define GetKeyboardLayoutName  GetKeyboardLayoutNameW
#define CreateDesktop  CreateDesktopW
#define OpenDesktop  OpenDesktopW
#define EnumDesktops  EnumDesktopsW
#define CreateWindowStation  CreateWindowStationW
#define OpenWindowStation  OpenWindowStationW
#define EnumWindowStations  EnumWindowStationsW
#define IsBadStringPtr  IsBadStringPtrW
#define LookupAccountSid  LookupAccountSidW
#define LookupAccountName  LookupAccountNameW
#define LookupPrivilegeValue  LookupPrivilegeValueW
#define LookupPrivilegeName  LookupPrivilegeNameW
#define LookupPrivilegeDisplayName  LookupPrivilegeDisplayNameW
#define BuildCommDCB  BuildCommDCBW
#define BuildCommDCBAndTimeouts  BuildCommDCBAndTimeoutsW
#define CommConfigDialog  CommConfigDialogW
#define GetDefaultCommConfig  GetDefaultCommConfigW
#define SetDefaultCommConfig  SetDefaultCommConfigW
#define GetComputerName  GetComputerNameW
#define SetComputerName  SetComputerNameW
#define GetUserName  GetUserNameW
#define CreateMailslot  CreateMailslotW
#define FormatMessage  FormatMessageW
#define GetEnvironmentStrings  GetEnvironmentStringsW
#define FreeEnvironmentStrings  FreeEnvironmentStringsW
#define lstrcmp  lstrcmpW
#define GetRawInputDeviceInfo GetRawInputDeviceInfoW
#define BroadcastSystemMessageEx BroadcastSystemMessageExW
#define lstrcmpi  lstrcmpiW
#define lstrcpyn  lstrcpynW
#define lstrcpy  lstrcpyW
#define lstrcat  lstrcatW
#define lstrlen  lstrlenW
#define GetBinaryType  GetBinaryTypeW
#define GetShortPathName  GetShortPathNameW
#define SetFileSecurity  SetFileSecurityW
#define CheckNameLegalDOS8Dot3 CheckNameLegalDOS8Dot3W
#define CreateHardLink CreateHardLinkW
#define GetFileSecurity  GetFileSecurityW
#define FindFirstChangeNotification  FindFirstChangeNotificationW
#define AccessCheckAndAuditAlarm  AccessCheckAndAuditAlarmW
#define ObjectOpenAuditAlarm  ObjectOpenAuditAlarmW
#define ObjectPrivilegeAuditAlarm  ObjectPrivilegeAuditAlarmW
#define ObjectCloseAuditAlarm  ObjectCloseAuditAlarmW
#define PrivilegedServiceAuditAlarm  PrivilegedServiceAuditAlarmW
#define OpenEventLog  OpenEventLogW
#define RegisterEventSource  RegisterEventSourceW
#define OpenBackupEventLog  OpenBackupEventLogW
#define ReadEventLog  ReadEventLogW
#define ReportEvent  ReportEventW
#define CreateProcess  CreateProcessW
#define FatalAppExit  FatalAppExitW
#define CallMessageFilter CallMessageFilterW
#define GetAltTabInfo GetAltTabInfoW
#define GetStartupInfo  GetStartupInfoW
#define GetEnvironmentVariable  GetEnvironmentVariableW
#define GetCommandLine  GetCommandLineW
#define SetEnvironmentVariable  SetEnvironmentVariableW
#define ExpandEnvironmentStrings  ExpandEnvironmentStringsW
#define OutputDebugString  OutputDebugStringW
#define FindResource  FindResourceW
#define FindResourceEx  FindResourceExW
#define EnumResourceTypes  EnumResourceTypesW
#define CreateActCtx CreateActCtxW
#define EnumResourceNames  EnumResourceNamesW
#define EnumResourceLanguages  EnumResourceLanguagesW
#define BeginUpdateResource  BeginUpdateResourceW
#define UpdateResource  UpdateResourceW
#define PrivateExtractIcons  PrivateExtractIconsW
#define EndUpdateResource  EndUpdateResourceW
#define GlobalAddAtom  GlobalAddAtomW
#define GlobalFindAtom  GlobalFindAtomW
#define GlobalGetAtomName  GlobalGetAtomNameW
#define AddAtom  AddAtomW
#define RemoveFontResourceEx RemoveFontResourceExW
#define FindAtom  FindAtomW
#define GetAtomName  GetAtomNameW
#define GetProfileInt  GetProfileIntW
#define GetProfileString  GetProfileStringW
#define WriteProfileString  WriteProfileStringW
#define ReadEventLog ReadEventLogW
#define GetProfileSection  GetProfileSectionW
#define WriteProfileSection  WriteProfileSectionW
#define GetPrivateProfileInt  GetPrivateProfileIntW
#define GetPrivateProfileString  GetPrivateProfileStringW
#define WritePrivateProfileString  WritePrivateProfileStringW
#define GetPrivateProfileSection  GetPrivateProfileSectionW
#define WritePrivateProfileSection  WritePrivateProfileSectionW
#define GetDriveType  GetDriveTypeW
#define GetSystemDirectory  GetSystemDirectoryW
#define FindFirstFileEx FindFirstFileExW
#define FindFirstVolume FindFirstVolumeW
#define GetTempPath  GetTempPathW
#define GetTempFileName  GetTempFileNameW
#define GetWindowsDirectory  GetWindowsDirectoryW
#define SetCurrentDirectory  SetCurrentDirectoryW
#define GetCurrentDirectory  GetCurrentDirectoryW
#define GetDiskFreeSpace  GetDiskFreeSpaceW
#define GetDiskFreeSpaceEx GetDiskFreeSpaceExW
#define CreateDirectory  CreateDirectoryW
#define CreateDirectoryEx  CreateDirectoryExW
#define RemoveDirectory  RemoveDirectoryW
#define GetLongPathName GetLongPathNameW
#define GetModuleHandleEx GetModuleHandleExW
#define GetFullPathName  GetFullPathNameW
#define DefineDosDevice  DefineDosDeviceW
#define QueryDosDevice  QueryDosDeviceW
#define EnumLanguageGroupLocales EnumLanguageGroupLocalesW
#define EnumSystemLanguageGroups EnumSystemLanguageGroupsW
#define CreateFile  CreateFileW
#define SetFileAttributes  SetFileAttributesW
#define GetFileAttributes  GetFileAttributesW
#define EncryptFile EncryptFileW
#define FileEncryptionStatus FileEncryptionStatusW
#define GetFileAttributesEx  GetFileAttributesExW
#define GetCompressedFileSize  GetCompressedFileSizeW
#define DeleteFile  DeleteFileW
#define FindFirstFileEx  FindFirstFileExW
#define SetFileShortName SetFileShortNameW
#define SetFirmwareEnvironmentVariable SetFirmwareEnvironmentVariableW
#define FindFirstFile  FindFirstFileW
#define FindNextFile  FindNextFileW
#define SearchPath  SearchPathW
#define CopyFile  CopyFileW
#define CopyFileEx  CopyFileExW
#define MoveFile  MoveFileW
#define MoveFileEx  MoveFileExW
#define MoveFileWithProgress  MoveFileWithProgressW
#define CreateFontIndirectEx CreateFontIndirectExW
#define CreateNamedPipe  CreateNamedPipeW
#define GetNamedPipeHandleState  GetNamedPipeHandleStateW
#define CallNamedPipe  CallNamedPipeW
#define IMPGetIME IMPGetIMEW
#define WaitNamedPipe  WaitNamedPipeW
#define SetVolumeLabel  SetVolumeLabelW
#define GetVolumeInformation  GetVolumeInformationW
#define ClearEventLog  ClearEventLogW
#define BackupEventLog  BackupEventLogW
#define CreateMutex  CreateMutexW
#define OpenMutex  OpenMutexW
#define CreateEvent  CreateEventW
#define OpenEvent  OpenEventW
#define CreateSemaphore  CreateSemaphoreW
#define OpenSemaphore  OpenSemaphoreW
#define CreateFileMapping  CreateFileMappingW
#define OpenFileMapping  OpenFileMappingW
#define GetLogicalDriveStrings  GetLogicalDriveStringsW
#define LoadLibrary  LoadLibraryW
#define LoadLibraryEx  LoadLibraryExW
#define FindNextVolumeMountPoint FindNextVolumeMountPointW
#define GetComputerNameEx GetComputerNameExW
#define GetModuleFileName  GetModuleFileNameW
#define GetModuleHandle  GetModuleHandleW
#define GetUserObjectInformation  GetUserObjectInformationW
#define SetUserObjectInformation  SetUserObjectInformationW
#define RegisterWindowMessage  RegisterWindowMessageW
#define GetMessage  GetMessageW
#define DispatchMessage  DispatchMessageW
#define SendIMEMessageEx SendIMEMessageExW
#define PeekMessage  PeekMessageW
#define SendMessage  SendMessageW
#define SendMessageTimeout  SendMessageTimeoutW
#define IMPSetIME IMPSetIMEW
#define IMPQueryIME IMPQueryIMEW
#define SendNotifyMessage  SendNotifyMessageW
#define SendMessageCallback  SendMessageCallbackW
#define BackupEventLog BackupEventLogW
#define ClearEventLog ClearEventLogW
#define PostMessage  PostMessageW
#define PostThreadMessage  PostThreadMessageW
#define OpenBackupEventLog OpenBackupEventLogW
#define OpenEncryptedFileRaw OpenEncryptedFileRawW
#define VerFindFile  VerFindFileW
#define VerInstallFile  VerInstallFileW
#define GetFileVersionInfoSize  GetFileVersionInfoSizeW
#define GetStringBitmap GetStringBitmapW
#define GetGlyphIndices GetGlyphIndicesW
#define GetFileVersionInfo  GetFileVersionInfoW
#define VerLanguageName  VerLanguageNameW
#define VerQueryValue  VerQueryValueW
#define CompareString  CompareStringW
#define LCMapString  LCMapStringW
#define GetLocaleInfo  GetLocaleInfoW
#define GetDllDirectory GetDllDirectoryW
#define GetFirmwareEnvironmentVariable GetFirmwareEnvironmentVariableW
#define SetLocaleInfo  SetLocaleInfoW
#define GetTimeFormat  GetTimeFormatW
#define GetDateFormat  GetDateFormatW
#define GetNumberFormat  GetNumberFormatW
#define GetCurrencyFormat  GetCurrencyFormatW
#define EnumCalendarInfo  EnumCalendarInfoW
#define EnumTimeFormats  EnumTimeFormatsW
#define CreateProcessAsUser CreateProcessAsUserW
#define DecryptFile DecryptFileW
#define FoldString  FoldStringW
#define EnumSystemCodePages  EnumSystemCodePagesW
#define EnumSystemLocales  EnumSystemLocalesW
#define GetStringTypeEx  GetStringTypeExW
#define EnumDateFormats  EnumDateFormatsW
#define GetConsoleTitle  GetConsoleTitleW
#define ScrollConsoleScreenBuffer  ScrollConsoleScreenBufferW
#define SetConsoleTitle  SetConsoleTitleW
#define ReadConsole  ReadConsoleW
#define WriteConsole  WriteConsoleW
#define PeekConsoleInput  PeekConsoleInputW
#define ReadConsoleInput  ReadConsoleInputW
#define WriteConsoleInput  WriteConsoleInputW
#define ReadConsoleOutput  ReadConsoleOutputW
#define WriteConsoleOutput  WriteConsoleOutputW
#define ReadConsoleOutputCharacter  ReadConsoleOutputCharacterW
#define WriteConsoleOutputCharacter  WriteConsoleOutputCharacterW
#define FillConsoleOutputCharacter  FillConsoleOutputCharacterW
#define WNetGetProviderName  WNetGetProviderNameW
#define SetCalendarInfo SetCalendarInfoW
#define EnumUILanguages EnumUILanguagesW
#define GetSystemWow64Directory GetSystemWow64DirectoryW
#define GetVolumeNameForVolumeMountPoint GetVolumeNameForVolumeMountPointW
#define WNetGetNetworkInformation  WNetGetNetworkInformationW
#define GetCurrentHwProfile GetCurrentHwProfileW
#define LogonUser LogonUserW
#define WNetGetLastError  WNetGetLastErrorW
#define MultinetGetConnectionPerformance  MultinetGetConnectionPerformanceW
#define WNetConnectionDialog1  WNetConnectionDialog1W
#define WNetDisconnectDialog1  WNetDisconnectDialog1W
#define WNetOpenEnum  WNetOpenEnumW
#define WNetEnumResource  WNetEnumResourceW
#define WNetGetUniversalName  WNetGetUniversalNameW
#define WNetGetUser  WNetGetUserW
#define WNetAddConnection  WNetAddConnectionW
#define WNetAddConnection2  WNetAddConnection2W
#define WNetAddConnection3  WNetAddConnection3W
#define DnsHostnameToComputerName DnsHostnameToComputerNameW
#define FindActCtxSectionString FindActCtxSectionStringW
#define WNetCancelConnection  WNetCancelConnectionW
#define OpenEventLog OpenEventLogW
#define WNetCancelConnection2  WNetCancelConnection2W
#define WNetGetConnection  WNetGetConnectionW
#define WNetUseConnection  WNetUseConnectionW
#define WNetSetConnection  WNetSetConnectionW
#define CreateService  CreateServiceW
#define ChangeServiceConfig  ChangeServiceConfigW
#define LogonUserEx LogonUserExW
#define EnumDependentServices  EnumDependentServicesW
#define EnumServicesStatus  EnumServicesStatusW
#define GetServiceKeyName  GetServiceKeyNameW
#define GetVolumePathName GetVolumePathNameW
#define GetVolumePathNamesForVolumeName GetVolumePathNamesForVolumeNameW
#define GetServiceDisplayName  GetServiceDisplayNameW
#define OpenSCManager  OpenSCManagerW
#define SetVolumeMountPoint SetVolumeMountPointW
#define VerifyVersionInfo VerifyVersionInfoW
#define RegisterDeviceNotification RegisterDeviceNotificationW
#define OpenService  OpenServiceW
#define QueryServiceConfig  QueryServiceConfigW
#define QueryServiceLockStatus  QueryServiceLockStatusW
#define GetGeoInfo GetGeoInfoW
#define EnumDateFormatsEx EnumDateFormatsExW
#define RegisterServiceCtrlHandler  RegisterServiceCtrlHandlerW
#define StartServiceCtrlDispatcher  StartServiceCtrlDispatcherW
#define EnumCalendarInfoEx EnumCalendarInfoExW
#define GetCalendarInfo GetCalendarInfoW
#define StartService  StartServiceW
#define DragQueryFile DragQueryFileW
#define GetCPInfoEx GetCPInfoExW
#define ExtractAssociatedIcon ExtractAssociatedIconW
#define ExtractIcon ExtractIconW
#define FindExecutable FindExecutableW
#define RegSaveKeyEx RegSaveKeyExW
#define InitiateSystemShutdownEx InitiateSystemShutdownExW
#define ShellAbout ShellAboutW
#define DeleteVolumeMountPoint DeleteVolumeMountPointW
#define ShellExecute ShellExecuteW
#define DdeCreateStringHandle DdeCreateStringHandleW
#define DdeInitialize DdeInitializeW
#define DdeQueryString DdeQueryStringW
#define OpenJobObject OpenJobObjectW
#define ReplaceFile ReplaceFileW
#define CreateProcessAsUser CreateProcessAsUserW
#define SHGetFileInfo SHGetFileInfoW
#define SetComputerNameEx SetComputerNameExW
#define SetDllDirectory SetDllDirectoryW
#define SHGetPathFromIDList SHGetPathFromIDListW

/* ASCII */
#else

#define RegConnectRegistry  RegConnectRegistryA
#define RegCreateKey  RegCreateKeyA
#define InitiateSystemShutdownEx InitiateSystemShutdownExA
#define RegCreateKeyEx  RegCreateKeyExA
#define RegDeleteKey  RegDeleteKeyA
#define RegDeleteValue  RegDeleteValueA
#define EnumSystemLanguageGroups EnumSystemLanguageGroupsA
#define GetGeoInfo GetGeoInfoA
#define RegEnumKey  RegEnumKeyA
#define RegEnumKeyEx  RegEnumKeyExA
#define GetCalendarInfo GetCalendarInfoA
#define LogonUser LogonUserA
#define GetCPInfoEx GetCPInfoExA
#define RegEnumValue  RegEnumValueA
#define RegLoadKey  RegLoadKeyA
#define RegOpenKey  RegOpenKeyA
#define RegOpenKeyEx  RegOpenKeyExA
#define FileEncryptionStatus FileEncryptionStatusA
#define GetCurrentHwProfile GetCurrentHwProfileA
#define LogonUserEx LogonUserExA
#define RegQueryInfoKey  RegQueryInfoKeyA
#define RegQueryValue  RegQueryValueA
#define RegQueryMultipleValues  RegQueryMultipleValuesA
#define RegQueryValueEx  RegQueryValueExA
#define GetFirmwareEnvironmentVariable GetFirmwareEnvironmentVariableA
#define GetLongPathName GetLongPathNameA
#define RegReplaceKey  RegReplaceKeyA
#define RegRestoreKey  RegRestoreKeyA
#define RegSaveKey  RegSaveKeyA
#define RegSetValue  RegSetValueA
#define RegSetValueEx  RegSetValueExA
#define AbortSystemShutdown  AbortSystemShutdownA
#define InitiateSystemShutdown  InitiateSystemShutdownA
#define RegUnLoadKey  RegUnLoadKeyA
#define LoadIcon  LoadIconA
#define LoadImage  LoadImageA
#define LoadString  LoadStringA
#define IsDialogMessage  IsDialogMessageA
#define DlgDirList  DlgDirListA
#define DlgDirSelectEx  DlgDirSelectExA
#define DlgDirListComboBox  DlgDirListComboBoxA
#define DlgDirSelectComboBoxEx  DlgDirSelectComboBoxExA
#define DefFrameProc  DefFrameProcA
#define DefMDIChildProc  DefMDIChildProcA
#define EnumUILanguages EnumUILanguagesA
#define EnumLanguageGroupLocales EnumLanguageGroupLocalesA
#define CreateMDIWindow  CreateMDIWindowA
#define FindNextVolume FindNextVolumeA
#define FindNextVolumeMountPoint FindNextVolumeMountPointA
#define GetComputerNameEx GetComputerNameExA
#define OpenEncryptedFileRaw OpenEncryptedFileRawA
#define OpenEventLog OpenEventLogA
#define VerifyVersionInfo VerifyVersionInfoA
#define SetFirmwareEnvironmentVariable SetFirmwareEnvironmentVariableA
#define SetVolumeMountPoint SetVolumeMountPointA
#define GetModuleHandleEx GetModuleHandleExA
#define GetSystemWow64Directory GetSystemWow64DirectoryA
#define GetVolumeNameForVolumeMountPoint GetVolumeNameForVolumeMountPointA
#define GetVolumePathNamesForVolumeName GetVolumePathNamesForVolumeNameA
#define OpenJobObject OpenJobObjectA
#define GetVolumePathName GetVolumePathNameA
#define GetDllDirectory GetDllDirectoryA
#define WinHelp  WinHelpA
#define ChangeDisplaySettings  ChangeDisplaySettingsA
#define EnumDisplaySettings  EnumDisplaySettingsA
#define SystemParametersInfo  SystemParametersInfoA
#define GetWindowLong  GetWindowLongA
#define SetWindowLong  SetWindowLongA
#define GetClassLong  GetClassLongA
#define SetClassLong  SetClassLongA
#define FindWindow  FindWindowA
#define FindWindowEx  FindWindowExA
#define GetClassName  GetClassNameA
#define SetWindowsHookEx  SetWindowsHookExA
#define SetWindowsHook  SetWindowsHookA
#define LoadBitmap  LoadBitmapA
#define LoadCursor  LoadCursorA
#define LoadCursorFromFile  LoadCursorFromFileA
#define ReadEventLog ReadEventLogA
#define ReplaceFile ReplaceFileA
#define SetDllDirectory SetDllDirectoryA
#define EnumDateFormatsEx EnumDateFormatsExA
#define EnumCalendarInfoEx EnumCalendarInfoExA
#define SetFileShortName SetFileShortNameA
#define SetComputerNameEx SetComputerNameExA
#define SetProp  SetPropA
#define GetProp  GetPropA
#define RemoveProp  RemovePropA
#define EnumPropsEx  EnumPropsExA
#define EnumProps  EnumPropsA
#define CreateHardLink CreateHardLinkA
#define CreateJobObject CreateJobObjectA
#define SetWindowText  SetWindowTextA
#define GetWindowText  GetWindowTextA
#define GetWindowTextLength  GetWindowTextLengthA
#define MessageBox  MessageBoxA
#define MessageBoxEx  MessageBoxExA
#define MessageBoxIndirect  MessageBoxIndirectA
#define AddFontResource  AddFontResourceA
#define CopyMetaFile  CopyMetaFileA
#define CreateDC  CreateDCA
#define CreateFontIndirect  CreateFontIndirectA
#define CreateFont  CreateFontA
#define CreateIC  CreateICA
#define CreateMetaFile  CreateMetaFileA
#define CreateScalableFontResource  CreateScalableFontResourceA
#define DeviceCapabilities  DeviceCapabilitiesA
#define EnumFontFamiliesEx  EnumFontFamiliesExA
#define EnumFontFamilies  EnumFontFamiliesA
#define EnumFonts  EnumFontsA
#define GetCharWidth  GetCharWidthA
#define GetCharWidth32  GetCharWidth32A
#define GetCharWidthFloat  GetCharWidthFloatA
#define FindFirstVolume FindFirstVolumeA
#define FindFirstVolumeMountPoint FindFirstVolumeMountPointA
#define GetCharABCWidths  GetCharABCWidthsA
#define GetCharABCWidthsFloat  GetCharABCWidthsFloatA
#define GetGlyphOutline  GetGlyphOutlineA
#define GetMetaFile  GetMetaFileA
#define GetOutlineTextMetrics  GetOutlineTextMetricsA
#define GetTextExtentPoint  GetTextExtentPointA
#define GetTextExtentPoint32  GetTextExtentPoint32A
#define GetTextExtentExPoint  GetTextExtentExPointA
#define GetCharacterPlacement  GetCharacterPlacementA
#define ResetDC  ResetDCA
#define RemoveFontResource  RemoveFontResourceA
#define CopyEnhMetaFile  CopyEnhMetaFileA
#define CreateEnhMetaFile  CreateEnhMetaFileA
#define GetEnhMetaFile  GetEnhMetaFileA
#define GetEnhMetaFileDescription  GetEnhMetaFileDescriptionA
#define GetTextMetrics  GetTextMetricsA
#define StartDoc  StartDocA
#define GetObject  GetObjectA
#define TextOut  TextOutA
#define ExtTextOut  ExtTextOutA
#define PolyTextOut  PolyTextOutA
#define GetTextFace  GetTextFaceA
#define GetKerningPairs  GetKerningPairsA
#define GetLogColorSpace  GetLogColorSpaceA
#define CreateColorSpace  CreateColorSpaceA
#define GetICMProfile  GetICMProfileA
#define DecryptFile DecryptFileA
#define EncryptFile EncryptFileA
#define SetICMProfile  SetICMProfileA
#define UpdateICMRegKey  UpdateICMRegKeyA
#define EnumICMProfiles  EnumICMProfilesA
#define CreatePropertySheetPage  CreatePropertySheetPageA
#define PropertySheet            PropertySheetA
#define ImageList_LoadImage     ImageList_LoadImageA
#define CreateStatusWindow      CreateStatusWindowA
#define DrawStatusText          DrawStatusTextA
#define GetOpenFileName  GetOpenFileNameA
#define GetSaveFileName  GetSaveFileNameA
#define GetFileTitle  GetFileTitleA
#define IMPSetIME IMPSetIMEA
#define ChooseColor  ChooseColorA
#define FindText  FindTextA
#define ReplaceText  ReplaceTextA
#define ChooseFont  ChooseFontA
#define PrintDlg  PrintDlgA
#define PageSetupDlg  PageSetupDlgA
#define DefWindowProc  DefWindowProcA
#define CallWindowProc  CallWindowProcA
#define GetGlyphIndices GetGlyphIndicesA
#define CreateFontIndirectEx CreateFontIndirectExA
#define RegisterClass  RegisterClassA
#define UnregisterClass  UnregisterClassA
#define GetClassInfo  GetClassInfoA
#define CreateActCtx CreateActCtxA
#define RegisterClassEx  RegisterClassExA
#define GetClassInfoEx  GetClassInfoExA
#define CreateWindowEx  CreateWindowExA
#define CreateWindow  CreateWindowA
#define CreateDialogParam  CreateDialogParamA
#define CreateDialogIndirectParam  CreateDialogIndirectParamA
#define CreateDialog  CreateDialogA
#define CreateDialogIndirect  CreateDialogIndirectA
#define CreateWaitableTimer CreateWaitableTimerA
#define DialogBoxParam  DialogBoxParamA
#define DialogBoxIndirectParam  DialogBoxIndirectParamA
#define DialogBox  DialogBoxA
#define ClearEventLog ClearEventLogA
#define CreateProcessAsUser CreateProcessAsUserA
#define DialogBoxIndirect  DialogBoxIndirectA
#define RegisterClipboardFormat  RegisterClipboardFormatA
#define SetDlgItemText  SetDlgItemTextA
#define GetDlgItemText  GetDlgItemTextA
#define SendDlgItemMessage  SendDlgItemMessageA
#define DefDlgProc  DefDlgProcA
#define GetClipboardFormatName  GetClipboardFormatNameA
#define CharToOem  CharToOemA
#define OemToChar  OemToCharA
#define CharToOemBuff  CharToOemBuffA
#define OemToCharBuff  OemToCharBuffA
#define CharUpper  CharUpperA
#define CharUpperBuff  CharUpperBuffA
#define CharLower  CharLowerA
#define CharLowerBuff  CharLowerBuffA
#define CharNext  CharNextA
#define CharPrev  CharPrevA
#define IsCharAlpha  IsCharAlphaA
#define IsCharAlphaNumeric  IsCharAlphaNumericA
#define IsCharUpper  IsCharUpperA
#define IsCharLower  IsCharLowerA
#define GetKeyNameText  GetKeyNameTextA
#define VkKeyScan  VkKeyScanA
#define RegisterDeviceNotification RegisterDeviceNotificationA
#define GetRawInputDeviceInfo GetRawInputDeviceInfoA
#define VkKeyScanEx  VkKeyScanExA
#define MapVirtualKey  MapVirtualKeyA
#define MapVirtualKeyEx  MapVirtualKeyExA
#define LoadAccelerators  LoadAcceleratorsA
#define CreateAcceleratorTable  CreateAcceleratorTableA
#define CopyAcceleratorTable  CopyAcceleratorTableA
#define TranslateAccelerator  TranslateAcceleratorA
#define LoadMenu  LoadMenuA
#define LoadMenuIndirect  LoadMenuIndirectA
#define ChangeMenu  ChangeMenuA
#define GetMenuString  GetMenuStringA
#define InsertMenu  InsertMenuA
#define AppendMenu  AppendMenuA
#define ModifyMenu  ModifyMenuA
#define InsertMenuItem  InsertMenuItemA
#define GetMenuItemInfo  GetMenuItemInfoA
#define SetMenuItemInfo  SetMenuItemInfoA
#define DrawText  DrawTextA
#define BroadcastSystemMessageEx BroadcastSystemMessageExA
#define DrawTextEx  DrawTextExA
#define GrayString  GrayStringA
#define DrawState  DrawStateA
#define TabbedTextOut  TabbedTextOutA
#define GetTabbedTextExtent  GetTabbedTextExtentA
#define GetVersionEx  GetVersionExA
#define wvsprintf  wvsprintfA
#define wsprintf  wsprintfA
#define FindActCtxSectionString FindActCtxSectionStringA
#define LoadKeyboardLayout  LoadKeyboardLayoutA
#define FindFirstFileEx FindFirstFileExA
#define GetKeyboardLayoutName  GetKeyboardLayoutNameA
#define CreateDesktop  CreateDesktopA
#define OpenDesktop  OpenDesktopA
#define EnumDesktops  EnumDesktopsA
#define CreateWindowStation  CreateWindowStationA
#define OpenWindowStation  OpenWindowStationA
#define EnumWindowStations  EnumWindowStationsA
#define IsBadStringPtr  IsBadStringPtrA
#define LookupAccountSid  LookupAccountSidA
#define LookupAccountName  LookupAccountNameA
#define LookupPrivilegeValue  LookupPrivilegeValueA
#define LookupPrivilegeName  LookupPrivilegeNameA
#define LookupPrivilegeDisplayName  LookupPrivilegeDisplayNameA
#define BuildCommDCB  BuildCommDCBA
#define BuildCommDCBAndTimeouts  BuildCommDCBAndTimeoutsA
#define CommConfigDialog  CommConfigDialogA
#define GetDefaultCommConfig  GetDefaultCommConfigA
#define SetDefaultCommConfig  SetDefaultCommConfigA
#define GetComputerName  GetComputerNameA
#define SetComputerName  SetComputerNameA
#define GetUserName  GetUserNameA
#define CreateMailslot  CreateMailslotA
#define CallMessageFilter CallMessageFilterA
#define GetAltTabInfo GetAltTabInfoA
#define BackupEventLog BackupEventLogA
#define FormatMessage  FormatMessageA
#define CheckNameLegalDOS8Dot3 CheckNameLegalDOS8Dot3A
#define GetEnvironmentStrings  GetEnvironmentStringsA
#define FreeEnvironmentStrings  FreeEnvironmentStringsA
#define lstrcmp  lstrcmpA
#define lstrcmpi  lstrcmpiA
#define lstrcpyn  lstrcpynA
#define lstrcpy  lstrcpyA
#define lstrcat  lstrcatA
#define lstrlen  lstrlenA
#define GetBinaryType  GetBinaryTypeA
#define GetStringBitmap GetStringBitmapA
#define GetShortPathName  GetShortPathNameA
#define SetFileSecurity  SetFileSecurityA
#define GetFileSecurity  GetFileSecurityA
#define FindFirstChangeNotification  FindFirstChangeNotificationA
#define AccessCheckAndAuditAlarm  AccessCheckAndAuditAlarmA
#define ObjectOpenAuditAlarm  ObjectOpenAuditAlarmA
#define ObjectPrivilegeAuditAlarm  ObjectPrivilegeAuditAlarmA
#define ObjectCloseAuditAlarm  ObjectCloseAuditAlarmA
#define PrivilegedServiceAuditAlarm  PrivilegedServiceAuditAlarmA
#define DeleteVolumeMountPoint DeleteVolumeMountPointA
#define DnsHostnameToComputerName DnsHostnameToComputerNameA
#define OpenEventLog  OpenEventLogA
#define RegisterEventSource  RegisterEventSourceA
#define OpenBackupEventLog  OpenBackupEventLogA
#define ReadEventLog  ReadEventLogA
#define ReportEvent  ReportEventA
#define CreateProcess  CreateProcessA
#define FatalAppExit  FatalAppExitA
#define RemoveFontResourceEx RemoveFontResourceExA
#define GetStartupInfo  GetStartupInfoA
#define GetCommandLine  GetCommandLineA
#define GetEnvironmentVariable  GetEnvironmentVariableA
#define SetEnvironmentVariable  SetEnvironmentVariableA
#define ExpandEnvironmentStrings  ExpandEnvironmentStringsA
#define OutputDebugString  OutputDebugStringA
#define FindResource  FindResourceA
#define FindResourceEx  FindResourceExA
#define EnumResourceTypes  EnumResourceTypesA
#define EnumResourceNames  EnumResourceNamesA
#define EnumResourceLanguages  EnumResourceLanguagesA
#define BeginUpdateResource  BeginUpdateResourceA
#define UpdateResource  UpdateResourceA
#define EndUpdateResource  EndUpdateResourceA
#define GlobalAddAtom  GlobalAddAtomA
#define GlobalFindAtom  GlobalFindAtomA
#define GlobalGetAtomName  GlobalGetAtomNameA
#define AddAtom  AddAtomA
#define FindAtom  FindAtomA
#define GetProfileInt  GetProfileIntA
#define GetAtomName  GetAtomNameA
#define GetProfileString  GetProfileStringA
#define WriteProfileString  WriteProfileStringA
#define GetProfileSection  GetProfileSectionA
#define WriteProfileSection  WriteProfileSectionA
#define GetPrivateProfileInt  GetPrivateProfileIntA
#define GetPrivateProfileString  GetPrivateProfileStringA
#define WritePrivateProfileString  WritePrivateProfileStringA
#define GetPrivateProfileSection  GetPrivateProfileSectionA
#define WritePrivateProfileSection  WritePrivateProfileSectionA
#define GetDriveType  GetDriveTypeA
#define GetSystemDirectory  GetSystemDirectoryA
#define GetTempPath  GetTempPathA
#define GetTempFileName  GetTempFileNameA
#define GetWindowsDirectory  GetWindowsDirectoryA
#define SetCurrentDirectory  SetCurrentDirectoryA
#define GetCurrentDirectory  GetCurrentDirectoryA
#define GetDiskFreeSpace  GetDiskFreeSpaceA
#define GetDiskFreeSpaceEx GetDiskFreeSpaceExA
#define CreateDirectory  CreateDirectoryA
#define CreateDirectoryEx  CreateDirectoryExA
#define RemoveDirectory  RemoveDirectoryA
#define GetFullPathName  GetFullPathNameA
#define DefineDosDevice  DefineDosDeviceA
#define QueryDosDevice  QueryDosDeviceA
#define CreateFile  CreateFileA
#define SetFileAttributes  SetFileAttributesA
#define GetFileAttributes  GetFileAttributesA
#define GetFileAttributesEx GetFileAttributesExA
#define GetCompressedFileSize  GetCompressedFileSizeA
#define DeleteFile  DeleteFileA
#define FindFirstFileEx  FindFirstFileExA
#define FindFirstFile  FindFirstFileA
#define FindNextFile  FindNextFileA
#define SearchPath  SearchPathA
#define CopyFile  CopyFileA
#define CopyFileEx  CopyFileExA
#define MoveFile  MoveFileA
#define MoveFileEx  MoveFileExA
#define MoveFileWithProgress  MoveFileWithProgressA
#define CreateNamedPipe  CreateNamedPipeA
#define GetNamedPipeHandleState  GetNamedPipeHandleStateA
#define CallNamedPipe  CallNamedPipeA
#define WaitNamedPipe  WaitNamedPipeA
#define SetVolumeLabel  SetVolumeLabelA
#define GetVolumeInformation  GetVolumeInformationA
#define ClearEventLog  ClearEventLogA
#define BackupEventLog  BackupEventLogA
#define CreateMutex  CreateMutexA
#define OpenMutex  OpenMutexA
#define CreateEvent  CreateEventA
#define OpenEvent  OpenEventA
#define CreateSemaphore  CreateSemaphoreA
#define OpenSemaphore  OpenSemaphoreA
#define CreateFileMapping  CreateFileMappingA
#define OpenFileMapping  OpenFileMappingA
#define GetLogicalDriveStrings  GetLogicalDriveStringsA
#define LoadLibrary  LoadLibraryA
#define LoadLibraryEx  LoadLibraryExA
#define GetModuleFileName  GetModuleFileNameA
#define GetModuleHandle  GetModuleHandleA
#define GetUserObjectInformation  GetUserObjectInformationA
#define PrivateExtractIcons  PrivateExtractIconsA
#define SetUserObjectInformation  SetUserObjectInformationA
#define RegisterWindowMessage  RegisterWindowMessageA
#define GetMessage  GetMessageA
#define DispatchMessage  DispatchMessageA
#define PeekMessage  PeekMessageA
#define SendMessage  SendMessageA
#define SendMessageTimeout  SendMessageTimeoutA
#define SendNotifyMessage  SendNotifyMessageA
#define SendMessageCallback  SendMessageCallbackA
#define PostMessage  PostMessageA
#define PostThreadMessage  PostThreadMessageA
#define VerFindFile  VerFindFileA
#define VerInstallFile  VerInstallFileA
#define GetFileVersionInfoSize  GetFileVersionInfoSizeA
#define GetFileVersionInfo  GetFileVersionInfoA
#define VerLanguageName  VerLanguageNameA
#define VerQueryValue  VerQueryValueA
#define CompareString  CompareStringA
#define LCMapString  LCMapStringA
#define GetLocaleInfo  GetLocaleInfoA
#define SetLocaleInfo  SetLocaleInfoA
#define GetTimeFormat  GetTimeFormatA
#define GetDateFormat  GetDateFormatA
#define GetNumberFormat  GetNumberFormatA
#define GetCurrencyFormat  GetCurrencyFormatA
#define EnumCalendarInfo  EnumCalendarInfoA
#define EnumTimeFormats  EnumTimeFormatsA
#define FoldString  FoldStringA
#define EnumSystemCodePages  EnumSystemCodePagesA
#define EnumSystemLocales  EnumSystemLocalesA
#define GetStringTypeEx  GetStringTypeExA
#define SetCalendarInfo SetCalendarInfoA
#define EnumDateFormats  EnumDateFormatsA
#define GetConsoleTitle  GetConsoleTitleA
#define ScrollConsoleScreenBuffer  ScrollConsoleScreenBufferA
#define SetConsoleTitle  SetConsoleTitleA
#define ReadConsole  ReadConsoleA
#define RegisterServiceCtrlHandlerEx RegisterServiceCtrlHandlerExA
#define QueryServiceConfig2 QueryServiceConfig2A
#define EnumServicesStatusEx EnumServicesStatusExA
#define ChangeServiceConfig2 ChangeServiceConfig2A
#define SendIMEMessageEx SendIMEMessageExA
#define IMPQueryIME IMPQueryIMEA
#define IMPGetIME IMPGetIMEA
#define WriteConsole  WriteConsoleA
#define PeekConsoleInput  PeekConsoleInputA
#define ReadConsoleInput  ReadConsoleInputA
#define WriteConsoleInput  WriteConsoleInputA
#define ReadConsoleOutput  ReadConsoleOutputA
#define WriteConsoleOutput  WriteConsoleOutputA
#define ReadConsoleOutputCharacter  ReadConsoleOutputCharacterA
#define WriteConsoleOutputCharacter  WriteConsoleOutputCharacterA
#define FillConsoleOutputCharacter  FillConsoleOutputCharacterA
#define MultinetGetConnectionPerformance  MultinetGetConnectionPerformanceA
#define WNetGetLastError  WNetGetLastErrorA
#define WNetGetProviderName  WNetGetProviderNameA
#define WNetGetNetworkInformation  WNetGetNetworkInformationA
#define WNetConnectionDialog1  WNetConnectionDialog1A
#define WNetDisconnectDialog1  WNetDisconnectDialog1A
#define WNetOpenEnum  WNetOpenEnumA
#define WNetEnumResource  WNetEnumResourceA
#define WNetGetUniversalName  WNetGetUniversalNameA
#define WNetGetUser  WNetGetUserA
#define WNetAddConnection  WNetAddConnectionA
#define WNetAddConnection2  WNetAddConnection2A
#define WNetAddConnection3  WNetAddConnection3A
#define WNetCancelConnection  WNetCancelConnectionA
#define WNetCancelConnection2  WNetCancelConnection2A
#define WNetGetConnection  WNetGetConnectionA
#define WNetUseConnection  WNetUseConnectionA
#define WNetSetConnection  WNetSetConnectionA
#define OpenService  OpenServiceA
#define QueryServiceConfig  QueryServiceConfigA
#define QueryServiceLockStatus  QueryServiceLockStatusA
#define RegisterServiceCtrlHandler  RegisterServiceCtrlHandlerA
#define StartServiceCtrlDispatcher  StartServiceCtrlDispatcherA
#define StartService  StartServiceA
#define ChangeServiceConfig  ChangeServiceConfigA
#define CreateService  CreateServiceA
#define EnumDependentServices  EnumDependentServicesA
#define EnumServicesStatus  EnumServicesStatusA
#define GetServiceKeyName  GetServiceKeyNameA
#define GetServiceDisplayName  GetServiceDisplayNameA
#define OpenSCManager  OpenSCManagerA
#define DragQueryFile DragQueryFileA
#define ExtractAssociatedIcon ExtractAssociatedIconA
#define RegSaveKeyEx RegSaveKeyExA
#define ExtractIcon ExtractIconA
#define FindFirstVolumeMountPoint FindFirstVolumeMountPointA
#define FindExecutable FindExecutableA
#define ShellAbout ShellAboutA
#define ShellExecute ShellExecuteA
#define DdeCreateStringHandle DdeCreateStringHandleA
#define DdeInitialize DdeInitializeA
#define DdeQueryString DdeQueryStringA
#define CreateProcessAsUser CreateProcessAsUserA
#define OpenBackupEventLog OpenBackupEventLogA
#define SHGetFileInfo SHGetFileInfoA
#define SHGetPathFromIDList SHGetPathFromIDListA

#endif /* UNICODE and ASCII defines */
#endif /* _DISABLE_TIDENTS */

NTSTATUS STDCALL CreateNlsSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,ULONG Size,ULONG AccessMask);

BOOL STDCALL GetConsoleInputExeNameA(ULONG length,LPCSTR name);

BOOL STDCALL GetConsoleInputExeNameW(ULONG length,LPCWSTR name);

BOOL STDCALL IsValidUILanguage(LANGID langid);

VOID STDCALL NlsConvertIntegerToString(ULONG Value,ULONG Base,ULONG strsize, LPWSTR str, ULONG strsize2);

UINT STDCALL SetCPGlobal(UINT CodePage);

BOOL
STDCALL
SetClientTimeZoneInformation(
		       CONST TIME_ZONE_INFORMATION *lpTimeZoneInformation
		       );

NTSTATUS STDCALL DbgUiDebugActiveProcess(HANDLE process);

NTSTATUS STDCALL DbgUiStopDebugging(HANDLE process);

VOID
STDCALL
RtlInitializeSListHead (
    PSLIST_HEADER ListHead
    );
PSLIST_ENTRY
STDCALL
RtlInterlockedFlushSList (
    PSLIST_HEADER ListHead
    );
PSLIST_ENTRY
STDCALL
RtlInterlockedPopEntrySList (
    PSLIST_HEADER ListHead
    );
PSLIST_ENTRY
STDCALL
RtlInterlockedPushEntrySList (
    PSLIST_HEADER ListHead,
    PSLIST_ENTRY ListEntry
    );
USHORT
STDCALL
RtlQueryDepthSList (
    PSLIST_HEADER ListHead
    );
NTSTATUS
STDCALL
RtlCreateTimer(HANDLE TimerQueue,PHANDLE phNewTimer, WAITORTIMERCALLBACK Callback,PVOID Parameter,DWORD DueTime,DWORD Period,ULONG Flags);

NTSTATUS
STDCALL
RtlCreateTimerQueue(PHANDLE TimerQueue);

NTSTATUS
STDCALL
RtlDeleteTimer(HANDLE TimerQueue,HANDLE Timer,HANDLE CompletionEvent);

NTSTATUS
STDCALL
RtlUpdateTimer(HANDLE TimerQueue,HANDLE Timer,ULONG DueTime,ULONG Period);

NTSTATUS
STDCALL
RtlDeleteTimerQueueEx(HANDLE TimerQueue,HANDLE CompletionEvent);

NTSTATUS
STDCALL
RtlDeleteTimerQueue(HANDLE TimerQueue);

NTSTATUS
STDCALL
RtlRegisterWait(PHANDLE hOutput, HANDLE hObject,WAITORTIMERCALLBACK Callback,PVOID Context,ULONG dwMilliseconds,ULONG dwFlags);

NTSTATUS
STDCALL
RtlDeregisterWait(
    HANDLE WaitHandle
    );

NTSTATUS
STDCALL
RtlDeregisterWaitEx(
    HANDLE WaitHandle,
    HANDLE CompletionEvent
    );

NTSTATUS
STDCALL
RtlSetHeapInformation(
    HANDLE HeapHandle,
    HEAP_INFORMATION_CLASS HeapInformationClass,
    PVOID HeapInformation OPTIONAL,
    SIZE_T HeapInformationLength OPTIONAL
    );

NTSTATUS
STDCALL
RtlQueryHeapInformation(
    HANDLE HeapHandle, 
    HEAP_INFORMATION_CLASS HeapInformationClass,
    PVOID HeapInformation OPTIONAL,
    SIZE_T HeapInformationLength OPTIONAL,
    PSIZE_T ReturnLength OPTIONAL
    );

NTSTATUS
STDCALL
NtCreateJobSet(
    ULONG NumJob,
    PJOB_SET_ARRAY UserJobSet,
    ULONG Flags);

NTSTATUS
STDCALL
ZwCreateJobSet(
    ULONG NumJob,
    PJOB_SET_ARRAY UserJobSet,
    ULONG Flags);

VOID
STDCALL
RtlReleaseActivationContext(
    HANDLE hActCtx
    );

NTSTATUS
STDCALL
RtlQueryInformationActivationContext(
    DWORD dwFlags,
    HANDLE hActCtx,
    PVOID pvSubInstance,
    ULONG ulInfoClass,
    PVOID pvBuffer,
    SIZE_T cbBuffer OPTIONAL,
    SIZE_T *pcbWrittenOrRequired OPTIONAL
    );

VOID
STDCALL
RtlAddRefActivationContext(
    HANDLE hActCtx
    );

NTSTATUS
STDCALL
RtlGetActiveActivationContext(
    HANDLE *lphActCtx);

NTSTATUS
STDCALL
RtlZombifyActivationContext(
    HANDLE hActCtx
    );

NTSTATUS
STDCALL
RtlDeactivateActivationContext(
    DWORD dwFlags,
    ULONG_PTR ulCookie
    );

ULONG 
STDCALL
RtlCreateTagHeap(	
	IN HANDLE HeapHandle,
	IN ULONG Flags,
	IN PCWSTR TagName,
	IN PCWSTR TagSubName
	);

PRTL_SPLAY_LINKS
STDCALL
RtlDelete(
	IN PRTL_SPLAY_LINKS Links
	);

BOOLEAN
STDCALL
RtlDeleteElementGenericTable(
	IN PRTL_GENERIC_TABLE Table,
	IN PVOID Buffer
	);

VOID
STDCALL
RtlDeleteNoSplay(
	IN PRTL_SPLAY_LINKS Links,
	OUT PRTL_SPLAY_LINKS *Root
	);

PVOID
STDCALL
RtlEnumerateGenericTable(
	IN PRTL_GENERIC_TABLE Table,
	IN BOOLEAN Restart
	);

PVOID
STDCALL
RtlEnumerateGenericTableWithoutSplaying(
	IN PRTL_GENERIC_TABLE Table,
	IN PVOID *RestartKey
	);

VOID
STDCALL
RtlGetElementGenericTable(
	IN PRTL_GENERIC_TABLE Table,
	IN ULONG I
	);

PVOID
STDCALL
RtlLookupElementGenericTable(
	IN PRTL_GENERIC_TABLE Table,
	IN PVOID Buffer
	);

PVOID
STDCALL
RtlProtectHeap(
	IN HANDLE Heap,
	IN BOOLEAN ReadOnly
	);

PCWSTR 
STDCALL
RtlQueryTagHeap(
	IN HANDLE HeapHandle,
	IN ULONG Flags,
	IN USHORT TagNumber,
	IN BOOLEAN ZeroInternalTagInfo,
	OUT PRTL_HEAP_TAG_INFO HeapTagInfo OPTIONAL
	);

PRTL_SPLAY_LINKS
STDCALL
RtlRealPredecessor(
	IN PRTL_SPLAY_LINKS Links
	);

PRTL_SPLAY_LINKS
STDCALL
RtlRealSuccessor(
	IN PRTL_SPLAY_LINKS Links
	);

PRTL_SPLAY_LINKS
STDCALL
RtlSplay(
	IN PRTL_SPLAY_LINKS Links
	);

NTSTATUS
STDCALL
NtAccessCheckByType(
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSID PrincipalSelfSid,
	IN HANDLE TokenHandle,
	IN ULONG DesiredAccess,
	IN POBJECT_TYPE_LIST ObjectTypeList,
	IN ULONG ObjectTypeListLength,
	IN PGENERIC_MAPPING GenericMapping,
	IN PPRIVILEGE_SET PrivilegeSet,
	IN PULONG PrivilegeSetLength,
	OUT PACCESS_MASK GrantedAccess,
	OUT PULONG AccessStatus
	);

NTSTATUS
STDCALL
NtAccessCheckByTypeAndAuditAlarm(
	IN PUNICODE_STRING SubsystemName,
	IN PVOID HandleId,
	IN PUNICODE_STRING ObjectTypeName,
	IN PUNICODE_STRING ObjectName,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSID PrincipalSelfSid,
	IN ACCESS_MASK DesiredAccess,
	IN AUDIT_EVENT_TYPE AuditType,
	IN ULONG Flags,
	IN POBJECT_TYPE_LIST ObjectTypeList,
	IN ULONG ObjectTypeListLength,
	IN PGENERIC_MAPPING GenericMapping,
	IN BOOLEAN ObjectCreation,
	OUT PACCESS_MASK GrantedAccess,
	OUT PULONG AccessStatus,
	OUT PBOOLEAN GenerateOnClose
	);

NTSTATUS
STDCALL
NtAccessCheckByTypeResultList(
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSID PrincipalSelfSid,
	IN HANDLE TokenHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_TYPE_LIST ObjectTypeList,
	IN ULONG ObjectTypeListLength,
	IN PGENERIC_MAPPING GenericMapping,
	IN PPRIVILEGE_SET PrivilegeSet,
	IN PULONG PrivilegeSetLength,
	OUT PACCESS_MASK GrantedAccessList,
	OUT PULONG AccessStatusList
	);

NTSTATUS
STDCALL
NtAccessCheckByTypeResultListAndAuditAlarm(
	IN PUNICODE_STRING SubsystemName,
	IN PVOID HandleId,
	IN PUNICODE_STRING ObjectTypeName,
	IN PUNICODE_STRING ObjectName,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSID PrincipalSelfSid,
	IN ACCESS_MASK DesiredAccess,
	IN AUDIT_EVENT_TYPE AuditType,
	IN ULONG Flags,
	IN POBJECT_TYPE_LIST ObjectTypeList,
	IN ULONG ObjectTypeListLength,
	IN PGENERIC_MAPPING GenericMapping,
	IN BOOLEAN ObjectCreation,
	OUT PACCESS_MASK GrantedAccessList,
	OUT PULONG AccessStatusList,
	OUT PULONG GenerateOnClose
	);

NTSTATUS
STDCALL
NtAccessCheckByTypeResultListAndAuditAlarmByHandle(
	IN PUNICODE_STRING SubsystemName,
	IN PVOID HandleId,
	IN HANDLE TokenHandle,
	IN PUNICODE_STRING ObjectTypeName,
	IN PUNICODE_STRING ObjectName,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSID PrincipalSelfSid,
	IN ACCESS_MASK DesiredAccess,
	IN AUDIT_EVENT_TYPE AuditType,
	IN ULONG Flags,
	IN POBJECT_TYPE_LIST ObjectTypeList,
	IN ULONG ObjectTypeListLength,
	IN PGENERIC_MAPPING GenericMapping,
	IN BOOLEAN ObjectCreation,
	OUT PACCESS_MASK GrantedAccessList,
	OUT PULONG AccessStatusList,
	OUT PULONG GenerateOnClose
	);

NTSTATUS
STDCALL
NtAllocateUserPhysicalPages(
	IN HANDLE ProcessHandle,
	IN PULONG NumberOfPages,
	OUT PULONG PageFrameNumbers
	);

NTSTATUS
STDCALL
NtAreMappedFilesTheSame(
	IN PVOID Address1,
	IN PVOID Address2
	);

NTSTATUS
STDCALL
NtCreateJobObject(
	OUT PHANDLE JobHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);

NTSTATUS
STDCALL
NtCreateKeyedEvent(
	OUT PHANDLE KeyedEventHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	ULONG Reserved
	);

NTSTATUS
STDCALL
NtFilterToken(
	IN HANDLE ExistingTokenHandle,
	IN ULONG Flags,
	IN PTOKEN_GROUPS SidsToDisable,
	IN PTOKEN_PRIVILEGES PrivilegesToDelete,
	IN PTOKEN_GROUPS SidsToRestricted,
	OUT PHANDLE NewTokenHandle
	);

NTSTATUS
STDCALL
NtFreeUserPhysicalPages(
	IN HANDLE ProcessHandle,
	IN OUT PULONG NumberOfPages,
	IN PULONG PageFrameNumbers
	);

NTSTATUS
STDCALL
NtGetWriteWatch(
	IN HANDLE ProcessHandle,
	IN ULONG Flags,
	IN PVOID BaseAddress,
	IN ULONG RegionSize,
	OUT PULONG Buffer,
	IN OUT PULONG BufferEntries,
	OUT PULONG Granularity
	);

NTSTATUS
STDCALL
NtImpersonateAnonymousToken(
	IN HANDLE ThreadHandle
	);

NTSTATUS
STDCALL
NtIsProcessInJob(
	IN HANDLE JobHandle,
	IN HANDLE ProcessHandle
	);

NTSTATUS
STDCALL
NtMakePermanentObject(
	IN HANDLE Object
	);

NTSTATUS
STDCALL
NtMapUserPhysicalPages(
	IN PVOID BaseAddress,
	IN PULONG NumberOfPages,
	IN PULONG PageFrameNumbers
	);

NTSTATUS
STDCALL
NtMapUserPhysicalPagesScatter(
	IN PVOID *BaseAddresses,
	IN PULONG NumberOfPages,
	IN PULONG PageFrameNumbers
	);

NTSTATUS
STDCALL
NtOpenJobObject(
	OUT PHANDLE JobHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);

NTSTATUS
STDCALL
NtOpenKeyedEvent(
	OUT PHANDLE KeyedEventHandle,
	IN PACCESS_MASK DesiredAccess,
	IN PUNICODE_STRING KeyedEventName
	);

NTSTATUS
STDCALL
NtOpenProcessTokenEx(
	IN HANDLE ProcessHandle,
	IN ACCESS_MASK DesiredAccess,
	IN ULONG HandleAttributes,
	OUT PHANDLE TokenHandle
	);

NTSTATUS
STDCALL
NtOpenThreadTokenEx(
	IN HANDLE ThreadHandle,
	IN ACCESS_MASK DesiredAccess,
	IN BOOLEAN OpenAsSelf,
	IN ULONG HandleAttributes,
	OUT PHANDLE TokenHandle
	);

NTSTATUS
STDCALL
NtQueryDefaultUILanguage(
	OUT PLANGID LanguageId
	);

NTSTATUS
STDCALL
NtQueryInformationJobObject(
	IN HANDLE JobHandle,
	IN JOBOBJECTINFOCLASS JobInformationClass,
	OUT PVOID JobInformation,
	IN ULONG JobInformationLength,
	OUT PULONG ReturnLength OPTIONAL
	);

NTSTATUS
STDCALL
NtQueryInstallUILanguage(
	OUT PLANGID LanguageId
	);

ULONG
STDCALL
NtQueryPortInformationProcess(
	VOID
	);

NTSTATUS
STDCALL
NtReleaseKeyedEvent(
	IN HANDLE KeyedEventHandle,
	IN ULONG Requested,
	IN BOOLEAN Alertable,
	IN PLARGE_INTEGER Timeout OPTIONAL
	);

NTSTATUS
STDCALL
NtReplyWaitReceivePortEx(
	IN HANDLE PortHandle,
	OUT PULONG PortIdentifier OPTIONAL,
	IN PPORT_MESSAGE ReplyMessage OPTIONAL,
	OUT PPORT_MESSAGE Message,
	IN PLARGE_INTEGER Timeout
	);

NTSTATUS
STDCALL
NtResetWriteWatch(
	IN HANDLE ProcessHandle,
	IN PVOID BaseAddress,
	IN ULONG RegionSize
	);

NTSTATUS
STDCALL
NtResumeProcess(
	IN HANDLE Process
	);

NTSTATUS
STDCALL
NtSecureConnectPort(
	OUT PHANDLE PortHandle,
	IN PUNICODE_STRING PortName,
	IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
	IN OUT PPORT_SECTION_WRITE WriteSection OPTIONAL,
	IN PSID ServerSid OPTIONAL,
	IN OUT PPORT_SECTION_READ ReadSection OPTIONAL,
	OUT PULONG MaxMessageSize OPTIONAL,
	IN OUT PVOID ConnectData OPTIONAL,
	IN OUT PULONG ConnectDataLength OPTIONAL
	);

NTSTATUS
STDCALL
NtSetDefaultUILanguage(
	IN LANGID LanguageId
	);

NTSTATUS
STDCALL
NtSetHighWaitLowThread(
	VOID
	);

NTSTATUS
STDCALL
NtSetInformationJobObject(
	IN HANDLE JobHandle,
	IN JOBOBJECTINFOCLASS JobInformationClass,
	IN PVOID JobInformation,
	IN ULONG JobInformationLength
	);

NTSTATUS
STDCALL
NtSetLowWaitHighThread(
	VOID
	);

NTSTATUS
STDCALL
NtSetQuotaInformationFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PFILE_USER_QUOTA_INFORMATION Buffer,
	IN ULONG BufferLength
	);

NTSTATUS
STDCALL
NtSetUuidSeed(
	IN PUCHAR UuidSeed
	);

NTSTATUS
STDCALL
NtSuspendProcess(
	IN HANDLE Process
	);

NTSTATUS
STDCALL
NtTerminateJobObject(
	IN HANDLE JobHandle,
	IN NTSTATUS ExitStatus
	);

NTSTATUS
STDCALL
NtWaitForKeyedEvent(
	IN HANDLE KeyedEventHandle,
	IN ULONG Requested,
	IN BOOLEAN Alertable,
	IN PLARGE_INTEGER Timeout OPTIONAL
	);

BOOLEAN
STDCALL
RtlDllShutdownInProgress(
	VOID
	);

PPEB
STDCALL
RtlGetCurrentPeb(
	VOID
	);

NTSTATUS
STDCALL
RtlGetLastNtStatus(
	VOID
	);

ULONG
STDCALL
RtlGetLastWin32Error(
	VOID
	);

NTSTATUS
STDCALL
RtlGetNativeSystemInformation(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	IN OUT PVOID SystemInformation,
	IN ULONG SystemInformationLength,
	OUT PULONG ReturnLength OPTIONAL
	);

USHORT
STDCALL
RtlLogStackBackTrace(
	VOID
	);

NTSTATUS
STDCALL
RtlQueueWorkItem(
	LPTHREAD_START_ROUTINE Function,
	PVOID Context,
	ULONG Flags
	);

VOID
STDCALL
RtlRestoreLastWin32Error(
	IN ULONG Win32Error
	);

ULONG
STDCALL
RtlSetCriticalSectionSpinCount(
   IN PRTL_CRITICAL_SECTION CriticalSection
   );

typedef
VOID
(CALLBACK *POVERLAPPED_COMPLETION_ROUTINE)(
    DWORD dwErrorCode,
    DWORD dwNumberOfBytesTransfered,
    LPOVERLAPPED lpOverlapped
    );


NTSTATUS
STDCALL
RtlSetIoCompletionCallback(
	IN HANDLE FileHandle,
	IN POVERLAPPED_COMPLETION_ROUTINE CompletionRoutine,
	IN ULONG Flags
	);

VOID
STDCALL
RtlSetLastWin32Error(
	IN ULONG Win32Error
	);

VOID
STDCALL
RtlSetLastWin32ErrorAndNtStatusFromNtStatus(
	IN NTSTATUS NtStatus
	);

NTSTATUS
STDCALL
ZwAccessCheckByType(
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSID PrincipalSelfSid,
	IN HANDLE TokenHandle,
	IN ULONG DesiredAccess,
	IN POBJECT_TYPE_LIST ObjectTypeList,
	IN ULONG ObjectTypeListLength,
	IN PGENERIC_MAPPING GenericMapping,
	IN PPRIVILEGE_SET PrivilegeSet,
	IN PULONG PrivilegeSetLength,
	OUT PACCESS_MASK GrantedAccess,
	OUT PULONG AccessStatus
	);

NTSTATUS
STDCALL
ZwAccessCheckByTypeAndAuditAlarm(
	IN PUNICODE_STRING SubsystemName,
	IN PVOID HandleId,
	IN PUNICODE_STRING ObjectTypeName,
	IN PUNICODE_STRING ObjectName,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSID PrincipalSelfSid,
	IN ACCESS_MASK DesiredAccess,
	IN AUDIT_EVENT_TYPE AuditType,
	IN ULONG Flags,
	IN POBJECT_TYPE_LIST ObjectTypeList,
	IN ULONG ObjectTypeListLength,
	IN PGENERIC_MAPPING GenericMapping,
	IN BOOLEAN ObjectCreation,
	OUT PACCESS_MASK GrantedAccess,
	OUT PULONG AccessStatus,
	OUT PBOOLEAN GenerateOnClose
	);

NTSTATUS
STDCALL
ZwAccessCheckByTypeResultList(
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSID PrincipalSelfSid,
	IN HANDLE TokenHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_TYPE_LIST ObjectTypeList,
	IN ULONG ObjectTypeListLength,
	IN PGENERIC_MAPPING GenericMapping,
	IN PPRIVILEGE_SET PrivilegeSet,
	IN PULONG PrivilegeSetLength,
	OUT PACCESS_MASK GrantedAccessList,
	OUT PULONG AccessStatusList
	);

NTSTATUS
STDCALL
ZwAccessCheckByTypeResultListAndAuditAlarm(
	IN PUNICODE_STRING SubsystemName,
	IN PVOID HandleId,
	IN PUNICODE_STRING ObjectTypeName,
	IN PUNICODE_STRING ObjectName,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSID PrincipalSelfSid,
	IN ACCESS_MASK DesiredAccess,
	IN AUDIT_EVENT_TYPE AuditType,
	IN ULONG Flags,
	IN POBJECT_TYPE_LIST ObjectTypeList,
	IN ULONG ObjectTypeListLength,
	IN PGENERIC_MAPPING GenericMapping,
	IN BOOLEAN ObjectCreation,
	OUT PACCESS_MASK GrantedAccessList,
	OUT PULONG AccessStatusList,
	OUT PULONG GenerateOnClose
	);

NTSTATUS
STDCALL
ZwAccessCheckByTypeResultListAndAuditAlarmByHandle(
	IN PUNICODE_STRING SubsystemName,
	IN PVOID HandleId,
	IN HANDLE TokenHandle,
	IN PUNICODE_STRING ObjectTypeName,
	IN PUNICODE_STRING ObjectName,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSID PrincipalSelfSid,
	IN ACCESS_MASK DesiredAccess,
	IN AUDIT_EVENT_TYPE AuditType,
	IN ULONG Flags,
	IN POBJECT_TYPE_LIST ObjectTypeList,
	IN ULONG ObjectTypeListLength,
	IN PGENERIC_MAPPING GenericMapping,
	IN BOOLEAN ObjectCreation,
	OUT PACCESS_MASK GrantedAccessList,
	OUT PULONG AccessStatusList,
	OUT PULONG GenerateOnClose
	);

NTSTATUS
STDCALL
ZwAllocateUserPhysicalPages(
	IN HANDLE ProcessHandle,
	IN PULONG NumberOfPages,
	OUT PULONG PageFrameNumbers
	);

NTSTATUS
STDCALL
ZwAreMappedFilesTheSame(
	IN PVOID Address1,
	IN PVOID Address2
	);

NTSTATUS
STDCALL
ZwAssignProcessToJobObject(
	IN HANDLE JobHandle,
	IN HANDLE ProcessHandle
	);

NTSTATUS
STDCALL
ZwCreateJobObject(
	OUT PHANDLE JobHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);

NTSTATUS
STDCALL
ZwCreateKeyedEvent(
	OUT PHANDLE KeyedEventHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	ULONG Reserved
	);

NTSTATUS
STDCALL
ZwCreateWaitablePort(
	OUT PHANDLE PortHandle,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN ULONG MaxDataSize,
	IN ULONG MaxMessageSize,
	IN ULONG Reserved
	);

NTSTATUS
STDCALL
ZwFilterToken(
	IN HANDLE ExistingTokenHandle,
	IN ULONG Flags,
	IN PTOKEN_GROUPS SidsToDisable,
	IN PTOKEN_PRIVILEGES PrivilegesToDelete,
	IN PTOKEN_GROUPS SidsToRestricted,
	OUT PHANDLE NewTokenHandle
	);

NTSTATUS
STDCALL
ZwFreeUserPhysicalPages(
	IN HANDLE ProcessHandle,
	IN OUT PULONG NumberOfPages,
	IN PULONG PageFrameNumbers
	);

NTSTATUS
STDCALL
ZwGetWriteWatch(
	IN HANDLE ProcessHandle,
	IN ULONG Flags,
	IN PVOID BaseAddress,
	IN ULONG RegionSize,
	OUT PULONG Buffer,
	IN OUT PULONG BufferEntries,
	OUT PULONG Granularity
	);

NTSTATUS
STDCALL
ZwImpersonateAnonymousToken(
	IN HANDLE ThreadHandle
	);

NTSTATUS
STDCALL
ZwIsProcessInJob(
	IN HANDLE JobHandle,
	IN HANDLE ProcessHandle
	);

NTSTATUS
STDCALL
ZwMakePermanentObject(
	IN HANDLE Object
	);

NTSTATUS
STDCALL
ZwMapUserPhysicalPages(
	IN PVOID BaseAddress,
	IN PULONG NumberOfPages,
	IN PULONG PageFrameNumbers
	);

NTSTATUS
STDCALL
ZwMapUserPhysicalPagesScatter(
	IN PVOID *BaseAddresses,
	IN PULONG NumberOfPages,
	IN PULONG PageFrameNumbers
	);

NTSTATUS
STDCALL
ZwOpenJobObject(
	OUT PHANDLE JobHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);

NTSTATUS
STDCALL
ZwOpenKeyedEvent(
	OUT PHANDLE KeyedEventHandle,
	IN PACCESS_MASK DesiredAccess,
	IN PUNICODE_STRING KeyedEventName
	);

NTSTATUS
STDCALL
ZwOpenProcessTokenEx(
	IN HANDLE ProcessHandle,
	IN ACCESS_MASK DesiredAccess,
	IN ULONG HandleAttributes,
	OUT PHANDLE TokenHandle
	);

NTSTATUS
STDCALL
ZwOpenThreadTokenEx(
	IN HANDLE ThreadHandle,
	IN ACCESS_MASK DesiredAccess,
	IN BOOLEAN OpenAsSelf,
	IN ULONG HandleAttributes,
	OUT PHANDLE TokenHandle
	);

NTSTATUS
STDCALL
ZwQueryDefaultUILanguage(
	OUT PLANGID LanguageId
	);

NTSTATUS
STDCALL
ZwQueryInformationJobObject(
	IN HANDLE JobHandle,
	IN JOBOBJECTINFOCLASS JobInformationClass,
	OUT PVOID JobInformation,
	IN ULONG JobInformationLength,
	OUT PULONG ReturnLength OPTIONAL
	);

NTSTATUS
STDCALL
ZwQueryInstallUILanguage(
	OUT PLANGID LanguageId
	);

ULONG
STDCALL
ZwQueryPortInformationProcess(
	VOID
	);

NTSTATUS
STDCALL
ZwQueryQuotaInformationFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PFILE_USER_QUOTA_INFORMATION Buffer,
	IN ULONG BufferLength,
	IN BOOLEAN ReturnSingleEntry,
	IN PFILE_QUOTA_LIST_INFORMATION QuotaList OPTIONAL,
	IN ULONG QuotaListLength,
	IN PSID ResumeSid OPTIONAL,
	IN BOOLEAN RestartScan
	);

NTSTATUS
STDCALL
ZwReleaseKeyedEvent(
	IN HANDLE KeyedEventHandle,
	IN ULONG Requested,
	IN BOOLEAN Alertable,
	IN PLARGE_INTEGER Timeout OPTIONAL
	);

NTSTATUS
STDCALL
ZwReplyWaitReceivePortEx(
	IN HANDLE PortHandle,
	OUT PULONG PortIdentifier OPTIONAL,
	IN PPORT_MESSAGE ReplyMessage OPTIONAL,
	OUT PPORT_MESSAGE Message,
	IN PLARGE_INTEGER Timeout
	);

NTSTATUS
STDCALL
ZwResetWriteWatch(
	IN HANDLE ProcessHandle,
	IN PVOID BaseAddress,
	IN ULONG RegionSize
	);

NTSTATUS
STDCALL
ZwResumeProcess(
	IN HANDLE Process
	);

NTSTATUS
STDCALL
ZwSecureConnectPort(
	OUT PHANDLE PortHandle,
	IN PUNICODE_STRING PortName,
	IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
	IN OUT PPORT_SECTION_WRITE WriteSection OPTIONAL,
	IN PSID ServerSid OPTIONAL,
	IN OUT PPORT_SECTION_READ ReadSection OPTIONAL,
	OUT PULONG MaxMessageSize OPTIONAL,
	IN OUT PVOID ConnectData OPTIONAL,
	IN OUT PULONG ConnectDataLength OPTIONAL
	);

NTSTATUS
STDCALL
ZwSetDefaultUILanguage(
	IN LANGID LanguageId
	);

NTSTATUS
STDCALL
ZwSetHighWaitLowThread(
	VOID
	);

NTSTATUS
STDCALL
ZwSetInformationJobObject(
	IN HANDLE JobHandle,
	IN JOBOBJECTINFOCLASS JobInformationClass,
	IN PVOID JobInformation,
	IN ULONG JobInformationLength
	);

NTSTATUS
STDCALL
ZwSetLowWaitHighThread(
	VOID
	);

NTSTATUS
STDCALL
ZwSetQuotaInformationFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PFILE_USER_QUOTA_INFORMATION Buffer,
	IN ULONG BufferLength
	);

NTSTATUS
STDCALL
ZwSetUuidSeed(
	IN PUCHAR UuidSeed
	);

NTSTATUS
STDCALL
ZwSuspendProcess(
	IN HANDLE Process
	);

NTSTATUS
STDCALL
ZwTerminateJobObject(
	IN HANDLE JobHandle,
	IN NTSTATUS ExitStatus
	);

NTSTATUS
STDCALL
ZwWaitForKeyedEvent(
	IN HANDLE KeyedEventHandle,
	IN ULONG Requested,
	IN BOOLEAN Alertable,
	IN PLARGE_INTEGER Timeout OPTIONAL
	);
DWORD
STDCALL
GetAppCompatFlags2(HTASK hTask);
VOID STDCALL InitializeLpkHooks(FARPROC *hookfuncs);
BOOL STDCALL EnterReaderModeHelper(HWND hwnd);
BOOL STDCALL CtxInitUser32(VOID);
BOOL STDCALL ReasonCodeNeedsBugID(ULONG reasoncode);
BOOL STDCALL ReasonCodeNeedsComment(ULONG reasoncode);
int
STDCALL
MessageBoxTimeoutW(
    HWND hWnd,
    LPCWSTR lpText,
    LPCWSTR lpCaption,
    UINT uType,
    WORD wLanguageId,
    DWORD dwTime);

int
STDCALL
MessageBoxTimeoutA(
    HWND hWnd,
    LPCSTR lpText,
    LPCSTR lpCaption,
    UINT uType,
    WORD wLanguageId,
    DWORD dwTime);
BOOL STDCALL DisplayExitWindowsWarnings(ULONG flags);
FARPROC STDCALL DelayLoadFailureHook(unsigned int dliNotify, PDelayLoadInfo pdli);
VOID STDCALL WEP();
DWORD STDCALL RtlDeleteSecurityObject(DWORD x1);
DWORD     STDCALL RtlNewSecurityObject(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6);
NTSTATUS  STDCALL RtlWalkHeap( HANDLE heap, PVOID entry_ptr );
NTSTATUS  STDCALL RtlpUnWaitCriticalSection(RTL_CRITICAL_SECTION *crit);
NTSTATUS  STDCALL RtlpWaitForCriticalSection(RTL_CRITICAL_SECTION *crit);
NTSTATUS STDCALL LdrLockLoaderLock(ULONG flags, ULONG *result, ULONG *magic);
NTSTATUS STDCALL LdrUnlockLoaderLock(ULONG flags, ULONG magic);
NTSTATUS  STDCALL RtlAddAccessAllowedAceEx(
	IN OUT PACL pAcl,
	IN DWORD dwAceRevision, 
	IN DWORD AceFlags,
	IN DWORD AccessMask,
	IN PSID pSid);
NTSTATUS  STDCALL RtlAddAccessDeniedAceEx(
	IN OUT PACL pAcl,
	IN DWORD dwAceRevision,
	IN DWORD AceFlags,
	IN DWORD AccessMask,
	IN PSID pSid);
NTSTATUS  STDCALL RtlDuplicateUnicodeString(
    int add_nul,
    const UNICODE_STRING *source,
    UNICODE_STRING *destination);
NTSTATUS  STDCALL RtlFindCharInUnicodeString(
    int flags,
    const UNICODE_STRING *main_str,
    const UNICODE_STRING *search_chars,
    USHORT *pos);
NTSTATUS  STDCALL RtlInitUnicodeStringEx(PUNICODE_STRING target,PCWSTR source);
NTSTATUS  STDCALL RtlInt64ToUnicodeString(ULONGLONG value,ULONG base,UNICODE_STRING *str);

ULONG
__cdecl
DbgPrintEx(
    IN ULONG ComponentId,
    IN ULONG Level,
    IN PCH Format,
    ...
    );
ULONG
__cdecl
DbgPrintReturnControlC(
    PCH Format,
    ...
    );
NTSTATUS
STDCALL
DbgQueryDebugFilterState(
    IN ULONG ComponentId,
    IN ULONG Level
    );
NTSTATUS
STDCALL
DbgSetDebugFilterState(
    IN ULONG ComponentId,
    IN ULONG Level,
    IN BOOL State
    );
NTSTATUS
STDCALL
NtCancelDeviceWakeupRequest(
    IN HANDLE Device
    );
NTSTATUS
STDCALL
NtCompactKeys(
    IN ULONG Count,
    IN HANDLE KeyArray[]
            );
NTSTATUS
STDCALL
NtCompressKey(
    IN HANDLE Key
            );
NTSTATUS
STDCALL
NtGetDevicePowerState(
    IN HANDLE Device,
    OUT DEVICE_POWER_STATE *State
    );
NTSTATUS
STDCALL
NtInitiatePowerAction(
    IN POWER_ACTION SystemAction,
    IN SYSTEM_POWER_STATE MinSystemState,
    IN ULONG Flags,
    IN BOOL Asynchronous
    );
BOOL
STDCALL
NtIsSystemResumeAutomatic(
    VOID
    );
NTSTATUS
STDCALL
NtLockProductActivationKeys(
    ULONG   *pPrivateVer,
    ULONG   *pIsSafeMode
    );
NTSTATUS
STDCALL
NtLockRegistryKey(
    IN HANDLE           KeyHandle
    );
NTSTATUS
STDCALL
NtNotifyChangeMultipleKeys(
    IN HANDLE MasterKeyHandle,  		
    IN ULONG Count,
    IN OBJECT_ATTRIBUTES SlaveObjects[],
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG CompletionFilter,
    IN BOOL WatchTree,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    IN BOOL Asynchronous
    );
NTSTATUS
STDCALL
NtQueryOpenSubKeys(
    IN POBJECT_ATTRIBUTES TargetKey,
    OUT PULONG  HandleCount
    );
NTSTATUS
STDCALL
NtRenameKey(
    IN HANDLE           KeyHandle,
    IN PUNICODE_STRING  NewName
    );
NTSTATUS
STDCALL
NtRequestDeviceWakeup(
    IN HANDLE Device
    );
NTSTATUS
STDCALL
NtRequestWakeupLatency(
    IN LATENCY_TIME latency
    );
NTSTATUS
STDCALL
NtSaveKeyEx(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle,
    IN ULONG  Format
    );
NTSTATUS
STDCALL
NtSaveMergedKeys(
    IN HANDLE HighPrecedenceKeyHandle,
    IN HANDLE LowPrecedenceKeyHandle,
    IN HANDLE FileHandle
    );
NTSTATUS
STDCALL
NtSetThreadExecutionState(
    IN EXECUTION_STATE esFlags,
    OUT EXECUTION_STATE *PreviousFlags
    );
NTSTATUS
STDCALL
NtUnloadKeyEx(
    IN POBJECT_ATTRIBUTES TargetKey,
    IN HANDLE Event OPTIONAL
    );
NTSTATUS
STDCALL
RtlGUIDFromString(
    IN PUNICODE_STRING GuidString,
    OUT GUID* Guid
    );
NTSTATUS
STDCALL
RtlGetVersion(
    OUT PRTL_OSVERSIONINFOW lpVersionInformation
    );
NTSTATUS
STDCALL
RtlHashUnicodeString(
    IN const UNICODE_STRING *String,
    IN BOOL CaseInSensitive,
    IN ULONG HashAlgorithm,
    OUT PULONG HashValue
    );
USHORT
FASTCALL
RtlUshortByteSwap(
    IN USHORT Source
    );
ULONG
FASTCALL
RtlUlongByteSwap(
    IN ULONG Source
    );
ULONGLONG
FASTCALL
RtlUlonglongByteSwap(
    IN ULONGLONG Source
    );
BOOL
STDCALL
RtlValidRelativeSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptorInput,
    IN ULONG SecurityDescriptorLength,
    IN SECURITY_INFORMATION RequiredInformation
    );
NTSTATUS
STDCALL
RtlVerifyVersionInfo(
    IN PRTL_OSVERSIONINFOEXW VersionInfo,
    IN ULONG TypeMask,
    IN ULONGLONG  ConditionMask
    );
ULONG
STDCALL
RtlWalkFrameChain (
    OUT PVOID *Callers,
    IN ULONG Count,
    IN ULONG Flags
    );
ULONG
vDbgPrintEx(
    IN ULONG ComponentId,
    IN ULONG Level,
    IN PCH Format,
    va_list arglist
    );
ULONG
vDbgPrintExWithPrefix(
    IN PCH Prefix,
    IN ULONG ComponentId,
    IN ULONG Level,
    IN PCH Format,
    va_list arglist
    );
NTSTATUS
STDCALL
ZwCancelDeviceWakeupRequest(
    IN HANDLE Device
    );
NTSTATUS
STDCALL
ZwCompactKeys(
    IN ULONG Count,
    IN HANDLE KeyArray[]
            );
NTSTATUS
STDCALL
ZwCompressKey(
    IN HANDLE Key
            );
NTSTATUS
STDCALL
ZwGetDevicePowerState(
    IN HANDLE Device,
    OUT DEVICE_POWER_STATE *State
    );
NTSTATUS
STDCALL
ZwInitiatePowerAction(
    IN POWER_ACTION SystemAction,
    IN SYSTEM_POWER_STATE MinSystemState,
    IN ULONG Flags,
    IN BOOL Asynchronous
    );
BOOL
STDCALL
ZwIsSystemResumeAutomatic(
    VOID
    );
NTSTATUS
STDCALL
ZwLockProductActivationKeys(
    ULONG   *pPrivateVer,
    ULONG   *pIsSafeMode
    );
NTSTATUS
STDCALL
ZwLockRegistryKey(
    IN HANDLE           KeyHandle
    );
NTSTATUS
STDCALL
ZwNotifyChangeMultipleKeys(
    IN HANDLE MasterKeyHandle,  		
    IN ULONG Count,
    IN OBJECT_ATTRIBUTES SlaveObjects[],
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG CompletionFilter,
    IN BOOL WatchTree,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    IN BOOL Asynchronous
    );
NTSTATUS
STDCALL
ZwQueryOpenSubKeys(
    IN POBJECT_ATTRIBUTES TargetKey,
    OUT PULONG  HandleCount
    );
NTSTATUS
STDCALL
ZwRenameKey(
    IN HANDLE           KeyHandle,
    IN PUNICODE_STRING  NewName
    );
NTSTATUS
STDCALL
ZwRequestDeviceWakeup(
    IN HANDLE Device
    );
NTSTATUS
STDCALL
ZwRequestWakeupLatency(
    IN LATENCY_TIME latency
    );
NTSTATUS
STDCALL
ZwSaveKeyEx(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle,
    IN ULONG  Format
    );
NTSTATUS
STDCALL
ZwSaveMergedKeys(
    IN HANDLE HighPrecedenceKeyHandle,
    IN HANDLE LowPrecedenceKeyHandle,
    IN HANDLE FileHandle
    );
NTSTATUS
STDCALL
ZwSetThreadExecutionState(
    IN EXECUTION_STATE esFlags,
    OUT EXECUTION_STATE *PreviousFlags
    );
NTSTATUS
STDCALL
ZwUnloadKeyEx(
    IN POBJECT_ATTRIBUTES TargetKey,
    IN HANDLE Event OPTIONAL
    );
BOOL STDCALL LdrFlushAlternateResourceModules(VOID);
BOOL STDCALL LdrAlternateResourcesEnabled(VOID);

VOID STDCALL MD4Init(PMD4_CONTEXT Context);
VOID STDCALL MD4Update(PMD4_CONTEXT Context, PVOID Buffer, UINT BufferSize);
VOID STDCALL MD4Final(PMD4_CONTEXT Context);

VOID STDCALL MD5Init(PMD5_CONTEXT Context);
VOID STDCALL MD5Update(PMD5_CONTEXT Context, PVOID Buffer, UINT BufferSize);
VOID STDCALL MD5Final(PMD5_CONTEXT Context);

VOID STDCALL A_SHAInit(PSHA_CONTEXT Context);
VOID STDCALL A_SHAUpdate(PSHA_CONTEXT Context, PVOID Buffer, UINT BufferSize);
VOID STDCALL A_SHAFinal(PSHA_CONTEXT Context, PVOID Result);

VOID STDCALL GdiInitializeLanguagePack(DWORD InitParam);
BOOL STDCALL SetConsoleIcon(HICON hicon);
NTSTATUS STDCALL NtPowerInformation(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5);
NTSTATUS STDCALL ZwPowerInformation(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5);
double __cdecl _CIpow(double x,double y);
LPSTR __cdecl _ui64toa(
    ULONGLONG value,
    LPSTR str,
    INT radix);
LPWSTR __cdecl _ui64tow(
    ULONGLONG value,
    LPWSTR str,
    INT radix);
INT __cdecl iswdigit( WCHAR wc );
INT __cdecl iswlower( WCHAR wc );
INT __cdecl iswxdigit( WCHAR wc );
DWORD STDCALL GetHandleContext(HANDLE hnd);
HANDLE STDCALL CreateSocketHandle(VOID);
BOOL STDCALL SetHandleContext(HANDLE hnd,DWORD context);
BOOL STDCALL SetConsoleInputExeNameA(LPCSTR name);
BOOL STDCALL SetConsoleInputExeNameW(LPCWSTR name);
BOOL STDCALL UTRegister( HMODULE hModule, LPSTR lpsz16BITDLL,
                        LPSTR lpszInitName, LPSTR lpszProcName,
                        FARPROC *ppfn32Thunk, FARPROC pfnUT32CallBack,
                        LPVOID lpBuff );
VOID STDCALL UTUnRegister( HMODULE hModule );
BOOL STDCALL SynchronizeWindows31FilesAndWindowsNTRegistry( DWORD x1, DWORD x2, DWORD x3,DWORD x4 );
BOOL STDCALL QueryWindows31FilesMigration( DWORD x1 );

WINBOOL STDCALL GetConsoleKeyboardLayoutNameA(LPSTR name);
WINBOOL STDCALL GetConsoleKeyboardLayoutNameW(LPWSTR name);

DWORD STDCALL SetupChangeFontSize(HANDLE HWindow,
                                  LPCWSTR lpszFontSize);
WINBOOL
STDCALL
AccessCheckByType(
PSECURITY_DESCRIPTOR pSecurityDescriptor,
PSID PrincipalSelfSid,
HANDLE ClientToken,
DWORD DesiredAccess,
POBJECT_TYPE_LIST ObjectTypeList,
DWORD ObjectTypeListLength,
PGENERIC_MAPPING GenericMapping,
PPRIVILEGE_SET PrivilegeSet,
LPDWORD PrivilegeSetLength,
LPDWORD GrantedAccess,
LPBOOL AccessStatus
);

WINBOOL
STDCALL
AccessCheckByTypeResultList(
PSECURITY_DESCRIPTOR pSecurityDescriptor,
PSID PrincipalSelfSid,
HANDLE ClientToken,
DWORD DesiredAccess,
POBJECT_TYPE_LIST ObjectTypeList,
DWORD ObjectTypeListLength,
PGENERIC_MAPPING GenericMapping,
PPRIVILEGE_SET PrivilegeSet,
LPDWORD PrivilegeSetLength,
LPDWORD GrantedAccessList,
LPDWORD AccessStatusList
);

WINBOOL
STDCALL
AccessCheckByTypeAndAuditAlarmA(
LPCSTR SubsystemName,
LPVOID HandleId,
LPCSTR ObjectTypeName,
LPCSTR ObjectName,
PSECURITY_DESCRIPTOR SecurityDescriptor,
PSID PrincipalSelfSid,
DWORD DesiredAccess,
AUDIT_EVENT_TYPE AuditType,
DWORD Flags,
POBJECT_TYPE_LIST ObjectTypeList,
DWORD ObjectTypeListLength,
PGENERIC_MAPPING GenericMapping,
WINBOOL ObjectCreation,
LPDWORD GrantedAccess,
LPBOOL AccessStatus,
LPBOOL pfGenerateOnClose
);

WINBOOL
STDCALL
AccessCheckByTypeAndAuditAlarmW(
LPCWSTR SubsystemName,
LPVOID HandleId,
LPCWSTR ObjectTypeName,
LPCWSTR ObjectName,
PSECURITY_DESCRIPTOR SecurityDescriptor,
PSID PrincipalSelfSid,
DWORD DesiredAccess,
AUDIT_EVENT_TYPE AuditType,
DWORD Flags,
POBJECT_TYPE_LIST ObjectTypeList,
DWORD ObjectTypeListLength,
PGENERIC_MAPPING GenericMapping,
WINBOOL ObjectCreation,
LPDWORD GrantedAccess,
LPBOOL AccessStatus,
LPBOOL pfGenerateOnClose
);

WINBOOL
STDCALL
AccessCheckByTypeResultListAndAuditAlarmA(
LPCSTR SubsystemName,
LPVOID HandleId,
LPCSTR ObjectTypeName,
LPCSTR ObjectName,
PSECURITY_DESCRIPTOR SecurityDescriptor,
PSID PrincipalSelfSid,
DWORD DesiredAccess,
AUDIT_EVENT_TYPE AuditType,
DWORD Flags,
POBJECT_TYPE_LIST ObjectTypeList,
DWORD ObjectTypeListLength,
PGENERIC_MAPPING GenericMapping,
WINBOOL ObjectCreation,
LPDWORD GrantedAccess,
LPDWORD AccessStatusList,
LPBOOL pfGenerateOnClose
);

WINBOOL
STDCALL
AccessCheckByTypeResultListAndAuditAlarmW(
LPCWSTR SubsystemName,
LPVOID HandleId,
LPCWSTR ObjectTypeName,
LPCWSTR ObjectName,
PSECURITY_DESCRIPTOR SecurityDescriptor,
PSID PrincipalSelfSid,
DWORD DesiredAccess,
AUDIT_EVENT_TYPE AuditType,
DWORD Flags,
POBJECT_TYPE_LIST ObjectTypeList,
DWORD ObjectTypeListLength,
PGENERIC_MAPPING GenericMapping,
WINBOOL ObjectCreation,
LPDWORD GrantedAccess,
LPDWORD AccessStatusList,
LPBOOL pfGenerateOnClose
);

WINBOOL
STDCALL
AccessCheckByTypeResultListAndAuditAlarmByHandleA(
LPCSTR SubsystemName,
LPVOID HandleId,
HANDLE ClientToken,
LPCSTR ObjectTypeName,
LPCSTR ObjectName,
PSECURITY_DESCRIPTOR SecurityDescriptor,
PSID PrincipalSelfSid,
DWORD DesiredAccess,
AUDIT_EVENT_TYPE AuditType,
DWORD Flags,
POBJECT_TYPE_LIST ObjectTypeList,
DWORD ObjectTypeListLength,
PGENERIC_MAPPING GenericMapping,
WINBOOL ObjectCreation,
LPDWORD GrantedAccess,
LPDWORD AccessStatusList,
LPBOOL pfGenerateOnClose
);

WINBOOL
STDCALL
AccessCheckByTypeResultListAndAuditAlarmByHandleW(
LPCWSTR SubsystemName,
LPVOID HandleId,
HANDLE ClientToken,
LPCWSTR ObjectTypeName,
LPCWSTR ObjectName,
PSECURITY_DESCRIPTOR SecurityDescriptor,
PSID PrincipalSelfSid,
DWORD DesiredAccess,
AUDIT_EVENT_TYPE AuditType,
DWORD Flags,
POBJECT_TYPE_LIST ObjectTypeList,
DWORD ObjectTypeListLength,
PGENERIC_MAPPING GenericMapping,
WINBOOL ObjectCreation,
LPDWORD GrantedAccess,
LPDWORD AccessStatusList,
LPBOOL pfGenerateOnClose
);

WINBOOL
STDCALL
InitiateSystemShutdownExA(LPSTR lpMachineName,LPSTR lpMessage,DWORD dwTimeout,BOOL bForceAppsClosed,BOOL bRebootAfterShutdown,DWORD dwReason);
WINBOOL
STDCALL
InitiateSystemShutdownExW(LPWSTR lpMachineName,LPWSTR lpMessage,DWORD dwTimeout,BOOL bForceAppsClosed,BOOL bRebootAfterShutdown,DWORD dwReason);
WINBOOL
STDCALL
NotifyBootConfigStatus(WINBOOL BootAcceptable);
LONG
STDCALL
RegDisablePredefinedCache(VOID);
LONG
STDCALL
RegOpenCurrentUser(REGSAM samDesired,PHKEY phkResult);
LONG
STDCALL
RegOpenUserClassesRoot(HANDLE hToken,DWORD  dwOptions,REGSAM samDesired,PHKEY  phkResult);
LONG
STDCALL
RegOverridePredefKey (HKEY hKey,HKEY hNewHKey);
LONG
STDCALL
RegSaveKeyExA (HKEY hKey,LPCSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes,DWORD Flags);
LONG
STDCALL
RegSaveKeyExW (HKEY hKey,LPCWSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes,DWORD Flags);
WINBOOL 
STDCALL
SetServiceBits(SERVICE_STATUS_HANDLE hServiceStatus,DWORD  dwServiceBits,WINBOOL  bSetBitsOn,WINBOOL  bUpdateImmediately);
LONG
STDCALL
Wow64Win32ApiEntry (DWORD dwFuncNumber,DWORD dwFlag,DWORD dwRes);

WINBOOL
STDCALL
AddAccessAllowedAceEx (
 PACL pAcl,
 DWORD dwAceRevision,
 DWORD AceFlags,
 DWORD AccessMask,
 PSID pSid
 );

WINBOOL
STDCALL
AddAccessAllowedObjectAce (
 PACL pAcl,
 DWORD dwAceRevision,
 DWORD AceFlags,
 DWORD AccessMask,
 GUID *ObjectTypeGuid,
 GUID *InheritedObjectTypeGuid,
 PSID pSid
 );

WINBOOL
STDCALL
AddAccessDeniedAceEx (
 PACL pAcl,
 DWORD dwAceRevision,
 DWORD AceFlags,
 DWORD AccessMask,
 PSID pSid
 );

WINBOOL
STDCALL
AddAccessDeniedObjectAce (
 PACL pAcl,
 DWORD dwAceRevision,
 DWORD AceFlags,
 DWORD AccessMask,
 GUID *ObjectTypeGuid,
 GUID *InheritedObjectTypeGuid,
 PSID pSid
 );

WINBOOL
STDCALL
AddAuditAccessAceEx(
 PACL pAcl,
 DWORD dwAceRevision,
 DWORD AceFlags,
 DWORD dwAccessMask,
 PSID pSid,
 WINBOOL bAuditSuccess,
 WINBOOL bAuditFailure
 );

WINBOOL
STDCALL
AddAuditAccessObjectAce (
 PACL pAcl,
 DWORD dwAceRevision,
 DWORD AceFlags,
 DWORD AccessMask,
 GUID *ObjectTypeGuid,
 GUID *InheritedObjectTypeGuid,
 PSID pSid,
 WINBOOL bAuditSuccess,
 WINBOOL bAuditFailure
 );

WINBOOL
APIENTRY
CheckTokenMembership(
 HANDLE TokenHandle,
 PSID SidToCheck,
 PWINBOOL IsMember
 );

VOID
STDCALL
CloseEncryptedFileRaw(
 PVOID pvContext
 );

WINBOOL
STDCALL
CloseEventLog (
 HANDLE hEventLog
 );

WINBOOL
STDCALL
ConvertToAutoInheritPrivateObjectSecurity(
 PSECURITY_DESCRIPTOR ParentDescriptor,
 PSECURITY_DESCRIPTOR CurrentSecurityDescriptor,
 PSECURITY_DESCRIPTOR *NewSecurityDescriptor,
 GUID *ObjectType,
 WINBOOL IsDirectoryObject,
 PGENERIC_MAPPING GenericMapping
 );

WINBOOL
STDCALL
CreatePrivateObjectSecurity (
 PSECURITY_DESCRIPTOR ParentDescriptor,
 PSECURITY_DESCRIPTOR CreatorDescriptor,
 PSECURITY_DESCRIPTOR * NewDescriptor,
 WINBOOL IsDirectoryObject,
 HANDLE Token,
 PGENERIC_MAPPING GenericMapping
 );

WINBOOL
STDCALL
CreatePrivateObjectSecurityEx (
 PSECURITY_DESCRIPTOR ParentDescriptor,
 PSECURITY_DESCRIPTOR CreatorDescriptor,
 PSECURITY_DESCRIPTOR * NewDescriptor,
 GUID *ObjectType,
 WINBOOL IsContainerObject,
 ULONG AutoInheritFlags,
 HANDLE Token,
 PGENERIC_MAPPING GenericMapping
 );

WINBOOL
STDCALL
CreatePrivateObjectSecurityWithMultipleInheritance (
 PSECURITY_DESCRIPTOR ParentDescriptor,
 PSECURITY_DESCRIPTOR CreatorDescriptor,
 PSECURITY_DESCRIPTOR * NewDescriptor,
 GUID **ObjectTypes,
 ULONG GuidCount,
 WINBOOL IsContainerObject,
 ULONG AutoInheritFlags,
 HANDLE Token,
 PGENERIC_MAPPING GenericMapping
 );

WINBOOL
STDCALL
CreateProcessWithLogonW(
 LPCWSTR lpUsername,
 LPCWSTR lpDomain,
 LPCWSTR lpPassword,
 DWORD dwLogonFlags,
 LPCWSTR lpApplicationName,
 LPWSTR lpCommandLine,
 DWORD dwCreationFlags,
 LPVOID lpEnvironment,
 LPCWSTR lpCurrentDirectory,
 LPSTARTUPINFOW lpStartupInfo,
 LPPROCESS_INFORMATION lpProcessInformation
 );

WINBOOL
APIENTRY
CreateRestrictedToken(
 HANDLE ExistingTokenHandle,
 DWORD Flags,
 DWORD DisableSidCount,
 PSID_AND_ATTRIBUTES SidsToDisable,
 DWORD DeletePrivilegeCount,
 PLUID_AND_ATTRIBUTES PrivilegesToDelete,
 DWORD RestrictedSidCount,
 PSID_AND_ATTRIBUTES SidsToRestrict,
 PHANDLE NewTokenHandle
 );

WINBOOL
STDCALL
CreateWellKnownSid(
 WELL_KNOWN_SID_TYPE WellKnownSidType,
 PSID DomainSid ,
 PSID pSid,
 DWORD *cbSid
 );

WINBOOL
STDCALL
DestroyPrivateObjectSecurity (
 PSECURITY_DESCRIPTOR * ObjectDescriptor
 );

WINBOOL
STDCALL
EqualDomainSid(
 PSID pSid1,
 PSID pSid2,
 WINBOOL *pfEqual
 );

WINBOOL
STDCALL
GetEventLogInformation (
 HANDLE hEventLog,
 DWORD dwInfoLevel,
 LPVOID lpBuffer,
 DWORD cbBufSize,
 LPDWORD pcbBytesNeeded
 );

WINBOOL
STDCALL
GetNumberOfEventLogRecords (
 HANDLE hEventLog,
 PDWORD NumberOfRecords
 );

WINBOOL
STDCALL
GetOldestEventLogRecord (
 HANDLE hEventLog,
 PDWORD OldestRecord
 );

WINBOOL
STDCALL
GetPrivateObjectSecurity (
 PSECURITY_DESCRIPTOR ObjectDescriptor,
 SECURITY_INFORMATION SecurityInformation,
 PSECURITY_DESCRIPTOR ResultantDescriptor,
 DWORD DescriptorLength,
 PDWORD ReturnLength
 );

DWORD
STDCALL
GetSecurityDescriptorRMControl(
 PSECURITY_DESCRIPTOR SecurityDescriptor,
 PUCHAR RMControl
 );

WINBOOL
STDCALL
GetWindowsAccountDomainSid(
 PSID pSid,
 PSID ppDomainSid,
 DWORD *cbSid
 );

WINBOOL
APIENTRY
ImpersonateAnonymousToken(
 HANDLE ThreadHandle
 );

WINBOOL
STDCALL
ImpersonateNamedPipeClient(
 HANDLE hNamedPipe
 );

WINBOOL
STDCALL
IsTextUnicode(
 CONST VOID* lpBuffer,
 int cb,
 LPINT lpi
 );

WINBOOL
STDCALL
IsTokenRestricted(
 HANDLE TokenHandle
 );

WINBOOL
STDCALL
IsTokenUntrusted(
 HANDLE TokenHandle
 );

WINBOOL
STDCALL
IsWellKnownSid (
 PSID pSid,
 WELL_KNOWN_SID_TYPE WellKnownSidType
 );

WINBOOL
STDCALL
MakeAbsoluteSD2 (
 PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor,
 LPDWORD lpdwBufferSize
 );

WINBOOL
STDCALL
NotifyChangeEventLog(
 HANDLE hEventLog,
 HANDLE hEvent
 );

DWORD
STDCALL
ReadEncryptedFileRaw(
 PFE_EXPORT_FUNC pfExportCallback,
 PVOID pvCallbackContext,
 PVOID pvContext
 );

DWORD
STDCALL
WriteEncryptedFileRaw(
 PFE_IMPORT_FUNC pfImportCallback,
 PVOID pvCallbackContext,
 PVOID pvContext
 );

WINBOOL
STDCALL
SetPrivateObjectSecurity (
 SECURITY_INFORMATION SecurityInformation,
 PSECURITY_DESCRIPTOR ModificationDescriptor,
 PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
 PGENERIC_MAPPING GenericMapping,
 HANDLE Token
 );

WINBOOL
STDCALL
SetPrivateObjectSecurityEx (
 SECURITY_INFORMATION SecurityInformation,
 PSECURITY_DESCRIPTOR ModificationDescriptor,
 PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
 ULONG AutoInheritFlags,
 PGENERIC_MAPPING GenericMapping,
 HANDLE Token
 );

WINBOOL
STDCALL
SetSecurityDescriptorControl (
 PSECURITY_DESCRIPTOR pSecurityDescriptor,
 SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
 SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet
 );

DWORD
STDCALL
SetSecurityDescriptorRMControl(
 PSECURITY_DESCRIPTOR SecurityDescriptor,
 PUCHAR RMControl
 );

VOID
STDCALL
RtlCaptureContext (
    PCONTEXT ContextRecord
    );

ULONGLONG
STDCALL
VerSetConditionMask(
        ULONGLONG   ConditionMask,
        DWORD   TypeMask,
        BYTE    Condition
        );

INT
STDCALL
GetExpandedNameA(
	LPSTR in,
	LPSTR out
	);
INT
STDCALL
GetExpandedNameW(
	LPWSTR in,
	LPWSTR out
	);

UINT
STDCALL
WINNLSGetIMEHotkey( HWND hwnd);

WINBOOL
STDCALL
WINNLSEnableIME( HWND hwnd, BOOL enable);

WINBOOL
STDCALL
WINNLSGetEnableStatus( HWND hwnd);

LANGID
STDCALL
GetUserDefaultUILanguage(VOID);

LANGID
STDCALL
GetSystemDefaultUILanguage(VOID);

WINBOOL
STDCALL
SetUserGeoID(
    GEOID       GeoId);

GEOID
STDCALL
GetUserGeoID(
    GEOCLASS    GeoClass);

WINBOOL
STDCALL
EnumSystemGeoID(
    GEOCLASS        GeoClass,
    GEOID           ParentGeoId,
    GEO_ENUMPROC    lpGeoEnumProc);

WINBOOL
STDCALL
IsValidLanguageGroup(
    LGRPID  LanguageGroup,
    DWORD   dwFlags);

WINBOOL
STDCALL
ActivateActCtx(
    HANDLE hActCtx,
    ULONG_PTR *lpCookie
    );

VOID
STDCALL
AddRefActCtx(
    HANDLE hActCtx
    );

WINBOOL
STDCALL
AllocateUserPhysicalPages(
    HANDLE hProcess,
    PULONG_PTR NumberOfPages,
    PULONG_PTR PageArray
    );

WINBOOL
STDCALL
AssignProcessToJobObject(
    HANDLE hJob,
    HANDLE hProcess
    );

WINBOOL
STDCALL
BindIoCompletionCallback (
    HANDLE FileHandle,
    LPOVERLAPPED_COMPLETION_ROUTINE Function,
    ULONG Flags
    );

WINBOOL
STDCALL
CancelDeviceWakeupRequest(
    HANDLE hDevice
    );

WINBOOL
STDCALL
CancelTimerQueueTimer(
    HANDLE TimerQueue,
    HANDLE Timer
    );

WINBOOL
STDCALL
ChangeTimerQueueTimer(
    HANDLE TimerQueue,
    HANDLE Timer,
    ULONG DueTime,
    ULONG Period
    );

WINBOOL
STDCALL
CheckRemoteDebuggerPresent(
    HANDLE hProcess,
    PWINBOOL pbDebuggerPresent
    );

WINBOOL
STDCALL
ConvertFiberToThread(
    VOID
    );

LONG
STDCALL
CopyLZFile(
	INT a,
	INT b
	);

HANDLE
STDCALL
CreateActCtxA(
    PCACTCTXA pActCtx
    );


HANDLE
STDCALL
CreateActCtxW(
    PCACTCTXW pActCtx
    );

LPVOID
STDCALL
CreateFiberEx(
    SIZE_T dwStackCommitSize,
    SIZE_T dwStackReserveSize,
    DWORD dwFlags,
    LPFIBER_START_ROUTINE lpStartAddress,
    LPVOID lpParameter
    );

WINBOOL
STDCALL
CreateJobSet (
    ULONG NumJob,
    PJOB_SET_ARRAY UserJobSet,
    ULONG Flags);

HANDLE
STDCALL
CreateMemoryResourceNotification(
    MEMORY_RESOURCE_NOTIFICATION_TYPE NotificationType
    );

HANDLE
STDCALL
CreateTimerQueue(
    VOID
    );

WINBOOL
STDCALL
CreateTimerQueueTimer(
    PHANDLE phNewTimer,
    HANDLE TimerQueue,
    WAITORTIMERCALLBACK Callback,
    PVOID Parameter,
    DWORD DueTime,
    DWORD Period,
    ULONG Flags
    ) ;

WINBOOL
STDCALL
DeactivateActCtx(
    DWORD dwFlags,
    ULONG_PTR ulCookie
    );

WINBOOL
STDCALL
DebugActiveProcessStop(
    DWORD dwProcessId
    );

WINBOOL
STDCALL
DebugBreakProcess (
    HANDLE Process
    );

WINBOOL
STDCALL
DebugSetProcessKillOnExit(
    WINBOOL KillOnExit
    );

WINBOOL
STDCALL
DeleteTimerQueue(
    HANDLE TimerQueue
    );

WINBOOL
STDCALL
DeleteTimerQueueEx(
    HANDLE TimerQueue,
    HANDLE CompletionEvent
    );

WINBOOL
STDCALL
DeleteTimerQueueTimer(
    HANDLE TimerQueue,
    HANDLE Timer,
    HANDLE CompletionEvent
    );

WINBOOL
STDCALL
FindActCtxSectionGuid(
    DWORD dwFlags,
    const GUID *lpExtensionGuid,
    ULONG ulSectionId,
    const GUID *lpGuidToFind,
    PACTCTX_SECTION_KEYED_DATA ReturnedData
    );

WINBOOL
STDCALL
FindVolumeClose(
    HANDLE hFindVolume
    );

WINBOOL
STDCALL
FindVolumeMountPointClose(
    HANDLE hFindVolumeMountPoint
    );

WINBOOL
STDCALL
FreeUserPhysicalPages(
    HANDLE hProcess,
    PULONG_PTR NumberOfPages,
    PULONG_PTR PageArray
    );

WINBOOL
STDCALL
GetCurrentActCtx(
    HANDLE *lphActCtx);

WINBOOL
STDCALL
GetDevicePowerState(
    HANDLE hDevice,
    WINBOOL *pfOn
    );

WINBOOL
STDCALL
GetFileSizeEx(
    HANDLE hFile,
    PLARGE_INTEGER lpFileSize
    );

VOID
STDCALL
GetNativeSystemInfo(
    LPSYSTEM_INFO lpSystemInfo
    );

WINBOOL
STDCALL
GetNumaHighestNodeNumber(
    PULONG HighestNodeNumber
    );

WINBOOL
STDCALL
GetNumaNodeProcessorMask(
    UCHAR Node,
    PULONGLONG ProcessorMask
    );

WINBOOL
STDCALL
GetNumaProcessorNode(
    UCHAR Processor,
    PUCHAR NodeNumber
    );

WINBOOL
STDCALL
GetProcessHandleCount(
    HANDLE hProcess,
    PDWORD pdwHandleCount
    );

DWORD
STDCALL
GetProcessId(
    HANDLE Process
    );

WINBOOL
STDCALL
GetProcessIoCounters(
    HANDLE hProcess,
    PIO_COUNTERS lpIoCounters
    );

WINBOOL
STDCALL
GetProcessPriorityBoost(
    HANDLE hProcess,
    PWINBOOL pDisablePriorityBoost
    );

WINBOOL
STDCALL
GetSystemRegistryQuota(
    PDWORD pdwQuotaAllowed,
    PDWORD pdwQuotaUsed
    );

WINBOOL
STDCALL
GetSystemTimes(
    LPFILETIME lpIdleTime,
    LPFILETIME lpKernelTime,
    LPFILETIME lpUserTime
    );

WINBOOL
STDCALL
GetThreadIOPendingFlag(
    HANDLE hThread,
    PWINBOOL lpIOIsPending
    );

UINT
STDCALL
GetWriteWatch(
    DWORD  dwFlags,
    PVOID  lpBaseAddress,
    SIZE_T dwRegionSize,
    PVOID *lpAddresses,
    PULONG_PTR lpdwCount,
    PULONG lpdwGranularity
    );

WINBOOL
STDCALL
GlobalMemoryStatusEx(
    LPMEMORYSTATUSEX lpBuffer
    );

WINBOOL
STDCALL
HeapQueryInformation (
    HANDLE HeapHandle, 
    HEAP_INFORMATION_CLASS HeapInformationClass,
    PVOID HeapInformation OPTIONAL,
    SIZE_T HeapInformationLength OPTIONAL,
    PSIZE_T ReturnLength OPTIONAL
    );

WINBOOL
STDCALL
HeapSetInformation (
    HANDLE HeapHandle,
    HEAP_INFORMATION_CLASS HeapInformationClass,
    PVOID HeapInformation OPTIONAL,
    SIZE_T HeapInformationLength OPTIONAL
    );

WINBOOL
STDCALL
InitializeCriticalSectionAndSpinCount(
    LPCRITICAL_SECTION lpCriticalSection,
    DWORD dwSpinCount
    );

VOID
STDCALL
InitializeSListHead (
    PSLIST_HEADER ListHead
    );

PSLIST_ENTRY
STDCALL
InterlockedFlushSList (
    PSLIST_HEADER ListHead
    );

#ifndef __NTDRIVER__ /* needed to avoid conflicts in win32k */
PSLIST_ENTRY
STDCALL
InterlockedPopEntrySList (
    PSLIST_HEADER ListHead
    );

PSLIST_ENTRY
STDCALL
InterlockedPushEntrySList (
    PSLIST_HEADER ListHead,
    PSLIST_ENTRY ListEntry
    );

#endif

WINBOOL
STDCALL
IsProcessInJob (
    HANDLE ProcessHandle,
    HANDLE JobHandle,
    PWINBOOL Result
    );

WINBOOL
STDCALL
IsSystemResumeAutomatic(
    VOID
    );

WINBOOL
STDCALL
IsWow64Process(
    HANDLE hProcess,
    PWINBOOL Wow64Process
    );

WINBOOL
STDCALL
MapUserPhysicalPages(
    PVOID VirtualAddress,
    ULONG_PTR NumberOfPages,
    PULONG_PTR PageArray OPTIONAL
    );

WINBOOL
STDCALL
MapUserPhysicalPagesScatter(
    PVOID *VirtualAddresses,
    ULONG_PTR NumberOfPages,
    PULONG_PTR PageArray OPTIONAL
    );

HANDLE
STDCALL
OpenThread(
    DWORD dwDesiredAccess,
    WINBOOL bInheritHandle,
    DWORD dwThreadId
    );

WINBOOL
STDCALL
QueryActCtxW(
    DWORD dwFlags,
    HANDLE hActCtx,
    PVOID pvSubInstance,
    ULONG ulInfoClass,
    PVOID pvBuffer,
    SIZE_T cbBuffer OPTIONAL,
    SIZE_T *pcbWrittenOrRequired OPTIONAL
    );

USHORT
STDCALL
QueryDepthSList (
    PSLIST_HEADER ListHead
    );

WINBOOL
STDCALL
QueryInformationJobObject(
    HANDLE hJob,
    JOBOBJECTINFOCLASS JobObjectInformationClass,
    LPVOID lpJobObjectInformation,
    DWORD cbJobObjectInformationLength,
    LPDWORD lpReturnLength
    );

WINBOOL
STDCALL
QueryMemoryResourceNotification(
     HANDLE ResourceNotificationHandle,
    PWINBOOL  ResourceState
    );

DWORD
STDCALL
QueueUserAPC(
    PAPCFUNC pfnAPC,
    HANDLE hThread,
    ULONG_PTR dwData
    );

NTSTATUS
STDCALL
RtlQueueWorkItem(
    LPTHREAD_START_ROUTINE Function,
    PVOID Context,
    ULONG Flags
    );

WINBOOL
STDCALL
ReadDirectoryChangesW(
    HANDLE hDirectory,
    LPVOID lpBuffer,
    DWORD nBufferLength,
    WINBOOL bWatchSubtree,
    DWORD dwNotifyFilter,
    LPDWORD lpBytesReturned,
    LPOVERLAPPED lpOverlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    );

WINBOOL
STDCALL
ReadFileScatter(
    HANDLE hFile,
    FILE_SEGMENT_ELEMENT aSegmentArray[],
    DWORD nNumberOfBytesToRead,
    LPDWORD lpReserved,
    LPOVERLAPPED lpOverlapped
    );

WINBOOL
STDCALL
RegisterWaitForSingleObject(
    PHANDLE phNewWaitObject,
    HANDLE hObject,
    WAITORTIMERCALLBACK Callback,
    PVOID Context,
    ULONG dwMilliseconds,
    ULONG dwFlags
    );

HANDLE
STDCALL
RegisterWaitForSingleObjectEx(
    HANDLE hObject,
    WAITORTIMERCALLBACK Callback,
    PVOID Context,
    ULONG dwMilliseconds,
    ULONG dwFlags
    );

VOID
STDCALL
ReleaseActCtx(
    HANDLE hActCtx
    );

ULONG
STDCALL
RemoveVectoredExceptionHandler(
    PVOID VectoredHandlerHandle
    );

WINBOOL
STDCALL
RequestDeviceWakeup(
    HANDLE hDevice
    );

WINBOOL
STDCALL
RequestWakeupLatency(
    LATENCY_TIME latency
    );

UINT
STDCALL
ResetWriteWatch(
    LPVOID lpBaseAddress,
    SIZE_T dwRegionSize
    );

VOID
STDCALL
RestoreLastError(
    DWORD dwErrCode
    );

DWORD
STDCALL
SetCriticalSectionSpinCount(
    LPCRITICAL_SECTION lpCriticalSection,
    DWORD dwSpinCount
    );

WINBOOL
STDCALL
SetFilePointerEx(
    HANDLE hFile,
    LARGE_INTEGER liDistanceToMove,
    PLARGE_INTEGER lpNewFilePointer,
    DWORD dwMoveMethod
    );

WINBOOL
STDCALL
SetFileValidData(
    HANDLE hFile,
    LONGLONG ValidDataLength
    );

WINBOOL
STDCALL
SetInformationJobObject(
    HANDLE hJob,
    JOBOBJECTINFOCLASS JobObjectInformationClass,
    LPVOID lpJobObjectInformation,
    DWORD cbJobObjectInformationLength
    );

WINBOOL
STDCALL
SetMessageWaitingIndicator(
    HANDLE hMsgIndicator,
    ULONG ulMsgCount
    );

WINBOOL
STDCALL
SetProcessPriorityBoost(
    HANDLE hProcess,
    WINBOOL bDisablePriorityBoost
    );

EXECUTION_STATE
STDCALL
SetThreadExecutionState(
    EXECUTION_STATE esFlags
    );

HANDLE
STDCALL
SetTimerQueueTimer(
    HANDLE TimerQueue,
    WAITORTIMERCALLBACK Callback,
    PVOID Parameter,
    DWORD DueTime,
    DWORD Period,
    WINBOOL PreferIo
    );

DWORD
STDCALL
SignalObjectAndWait(
    HANDLE hObjectToSignal,
    HANDLE hObjectToWaitOn,
    DWORD dwMilliseconds,
    WINBOOL bAlertable
    );

WINBOOL
STDCALL
TerminateJobObject(
    HANDLE hJob,
    UINT uExitCode
    );

WINBOOL
STDCALL
TzSpecificLocalTimeToSystemTime(
    LPTIME_ZONE_INFORMATION lpTimeZoneInformation,
    LPSYSTEMTIME lpLocalTime,
    LPSYSTEMTIME lpUniversalTime
    );

WINBOOL
STDCALL
UnregisterWait(
    HANDLE WaitHandle
    );

WINBOOL
STDCALL
UnregisterWaitEx(
    HANDLE WaitHandle,
    HANDLE CompletionEvent
    );

WINBOOL
STDCALL
WriteFileGather(
    HANDLE hFile,
    FILE_SEGMENT_ELEMENT aSegmentArray[],
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpReserved,
    LPOVERLAPPED lpOverlapped
    );

DWORD
STDCALL
WTSGetActiveConsoleSessionId(VOID);

WINBOOL
STDCALL
ZombifyActCtx(
    HANDLE hActCtx
    );

PVOID
STDCALL
AddVectoredExceptionHandler(
    ULONG FirstHandler,
    PVECTORED_EXCEPTION_HANDLER VectoredHandler
    );

HANDLE 
STDCALL 
AddFontMemResourceEx(
	PVOID pbFont,
	DWORD cbFont,
	PVOID pdv,
	DWORD *pcFonts
);

int 
STDCALL 
AddFontResourceTracking(
	LPCSTR lpString,
	int unknown
);

WINBOOL 
STDCALL 
AnyLinkedFonts(VOID);

HBITMAP 
STDCALL
ClearBitmapAttributes(HBITMAP hbm, DWORD dwFlags);

HBRUSH 
STDCALL
ClearBrushAttributes(HBRUSH hbm, DWORD dwFlags);

WINBOOL 
STDCALL
ColorCorrectPalette(HDC hDC,HPALETTE hPalette,DWORD dwFirstEntry,DWORD dwNumOfEntries);

WINBOOL 
STDCALL
EnableEUDC(WINBOOL enable);

int
STDCALL
EndFormPage(HDC hdc);

WINBOOL
STDCALL
EudcLoadLinkW(LPCWSTR pBaseFaceName,LPCWSTR pEudcFontPath,INT iPriority,INT iFontLinkType);

WINBOOL 
STDCALL
EudcUnloadLinkW(LPCWSTR pBaseFaceName,LPCWSTR pEudcFontPath);

WINBOOL 
STDCALL
FontIsLinked(HDC hdc);

int 
STDCALL
GdiAddFontResourceW(LPCWSTR filename,FLONG f,DESIGNVECTOR *pdv);

DWORD 
STDCALL
GdiAddGlsBounds(HDC hdc,LPRECT prc);

WINBOOL 
STDCALL
GdiAlphaBlend(HDC hdcDst,LONG DstX,LONG DstY,LONG DstCx,LONG DstCy,HDC hdcSrc,LONG SrcX,LONG SrcY,LONG SrcCx,LONG SrcCy,BLENDFUNCTION BlendFunction);

WINBOOL 
STDCALL
GdiArtificialDecrementDriver(LPWSTR pDriverName,WINBOOL unknown);

WINBOOL
STDCALL
GdiCleanCacheDC(HDC hdc);

WINBOOL 
STDCALL
GdiConsoleTextOut(HDC hdc, POLYTEXTW *lpto,UINT nStrings, RECTL *prclBounds);

HDC
STDCALL
GdiConvertAndCheckDC(HDC hdc);

HBITMAP 
STDCALL
GdiConvertBitmap(HBITMAP hbm);

HBRUSH
STDCALL
GdiConvertBrush(HBRUSH hbr);

HDC 
STDCALL
GdiConvertDC(HDC hdc);

HFONT 
STDCALL
GdiConvertFont(HFONT hfont);

HPALETTE 
STDCALL
GdiConvertPalette(HPALETTE hpal);

HRGN
STDCALL
GdiConvertRegion(HRGN hregion);

HENHMETAFILE 
STDCALL
GdiConvertEnhMetaFile(HENHMETAFILE hmf);

WINBOOL
STDCALL
GdiDeleteLocalDC(HDC hdc);

int 
STDCALL
GdiDescribePixelFormat(HDC hdc,int ipfd,UINT cjpfd,PPIXELFORMATDESCRIPTOR ppfd);

WINBOOL
STDCALL
GdiDrawStream(HDC dc, ULONG l, VOID *v);

HGDIOBJ
STDCALL
GdiFixUpHandle(HGDIOBJ hobj);

NTSTATUS
STDCALL
GdiFullscreenControl(FULLSCREENCONTROL FullscreenCommand, PVOID FullscreenInput,
					DWORD FullscreenInputLength, PVOID FullscreenOutput,
					PULONG FullscreenOutputLength);

DWORD 
STDCALL
GdiGetCodePage(HDC hdc);

HBRUSH 
STDCALL
GdiGetLocalBrush(HBRUSH hbr);

HDC 
STDCALL
GdiGetLocalDC(HDC hdc);

HFONT 
STDCALL
GdiGetLocalFont(HFONT hfont);

WINBOOL
STDCALL
GdiGradientFill(HDC hdc,PTRIVERTEX pVertex,ULONG uVertex,PVOID pMesh,ULONG uMesh,ULONG ulMode);

WINBOOL
STDCALL
GdiIsMetaFileDC(HDC hdc);

WINBOOL
STDCALL
GdiIsMetaPrintDC(HDC hdc);

WINBOOL
STDCALL
GdiIsPlayMetafileDC(HDC hdc);

INT 
STDCALL
GdiQueryFonts(PUNIVERSAL_FONT_ID pufiFontList,ULONG nBufferSize,PLARGE_INTEGER pTimeStamp );

WINBOOL 
STDCALL
GdiRealizationInfo(HDC hdc, PREALIZATION_INFO pri);

WINBOOL
STDCALL
GdiReleaseDC(HDC hdc);

WINBOOL
STDCALL
GdiReleaseLocalDC(HDC hdc);

WINBOOL
STDCALL
GdiSetAttrs(HDC hdc);

VOID
STDCALL
GdiSetLastError(DWORD dwErrCode);

WINBOOL
STDCALL
GdiSetPixelFormat(HDC hdc,int ipfd);

WINBOOL
STDCALL
GdiTransparentBlt(HDC hdcDst, int xDst, int yDst, int cxDst, int cyDst,HDC hdcSrc, int xSrc, int ySrc, int cxSrc, int cySrc,COLORREF TransColor);

WINBOOL
STDCALL
GdiValidateHandle(HGDIOBJ hobj);

WINBOOL
STDCALL
GdiSwapBuffers(HDC hdc);

VOID 
STDCALL
GdiSetServerAttr(HDC hdc,DWORD attr);

DWORD 
STDCALL
GetBitmapAttributes(HBITMAP hbm);

DWORD 
STDCALL
GetBrushAttributes(HBRUSH hbr);

WINBOOL 
STDCALL
GetCharABCWidthsI(
	HDC hdc,
	UINT giFirst,
	UINT cgi,
	LPWORD pgi,
	LPABC lpabc
);

WINBOOL 
STDCALL
GetCharWidthI(
	HDC hdc,
	UINT giFirst,
	UINT cgi,
	LPWORD pgi,
	LPINT lpBuffer
);

WINBOOL 
STDCALL
GetCharWidthInfo(HDC hdc,PCHWIDTHINFO pChWidthInfo);

COLORREF 
STDCALL
GetDCBrushColor(
	HDC hdc
);

COLORREF 
STDCALL
GetDCPenColor(
	HDC hdc
);

DWORD 
STDCALL
GetFontUnicodeRanges(
	HDC hdc,
	LPGLYPHSET lpgs
);

WINBOOL 
STDCALL
GetETM(HDC hdc,EXTTEXTMETRIC *petm);

ULONG 
STDCALL
GetEUDCTimeStamp(VOID);

DWORD 
STDCALL
GetEUDCTimeStampExW(LPCWSTR str);

ULONG 
STDCALL
GetFontAssocStatus(HDC hdc);

HFONT 
STDCALL
GetHFONT(HDC dc);

DWORD 
STDCALL
GetLayout(
	HDC hdc
);

WINBOOL
STDCALL
GetTextExtentExPointWPri(HDC hdc,LPWSTR lpwsz,ULONG cwc,ULONG dxMax,ULONG *pcCh,PULONG pdxOut,LPSIZE psize);

WINBOOL 
STDCALL
GetTextExtentPointI(
	HDC hdc,
	LPWORD pgiIn,
	int cgi,
	LPSIZE lpSize
);

int 
STDCALL
GetTextFaceAliasW(HDC hdc,int cChar,LPWSTR pszOut);

WINBOOL 
STDCALL
GetTransform(HDC hdc, DWORD iXform, LPXFORM pxf);

LONG 
STDCALL
HT_Get8BPPFormatPalette(LPPALETTEENTRY pPaletteEntry, USHORT RedGamma,USHORT GreenGamma, USHORT BlueGamma);

LONG 
STDCALL
HT_Get8BPPMaskPalette(LPPALETTEENTRY pPaletteEntry, WINBOOL Use8BPPMaskPal,BYTE CMYMask, USHORT RedGamma, USHORT GreenGamma, USHORT BlueGamma);

WINBOOL 
STDCALL
MirrorRgn(HWND hwnd,HRGN hrgn);

int 
STDCALL
NamedEscape(HDC hdc,PWCHAR pDriver,int nDriver,int iEsc,int cjIn,LPSTR pjIn,int cjOut,LPSTR pjOut);

DWORD 
STDCALL
QueryFontAssocStatus(VOID);

WINBOOL 
STDCALL
RemoveFontMemResourceEx(
	HANDLE fh
);

int 
STDCALL
RemoveFontResourceTracking(LPCSTR lpString,int unknown);

WINBOOL 
STDCALL
GetTextExtentExPointI(
	HDC hdc,
	LPWORD pgiIn,
	int cgi,
	int nMaxExtent,
	LPINT lpnFit,
	LPINT alpDx,
	LPSIZE lpSize
);

HBITMAP 
STDCALL
SetBitmapAttributes(HBITMAP hbm, DWORD dwFlags);

HBRUSH 
STDCALL
SetBrushAttributes(HBRUSH hbm, DWORD dwFlags);

COLORREF 
STDCALL
SetDCBrushColor(
	HDC hdc,
	COLORREF crColor
);

COLORREF 
STDCALL
SetDCPenColor(
	HDC hdc,
	COLORREF crColor
);

DWORD 
STDCALL
SetLayout(
	HDC hdc,
	DWORD dwLayout
);

DWORD 
STDCALL
SetLayoutWidth(HDC hdc,LONG wox,DWORD dwLayout);

WINBOOL 
STDCALL
SetMagicColors(HDC hdc,PALETTEENTRY peMagic,ULONG Index);

WINBOOL
STDCALL
SetVirtualResolution(HDC hdc, int cxVirtualDevicePixel,int cyVirtualDevicePixel,int cxVirtualDeviceMm, int cyVirtualDeviceMm);

int 
STDCALL
StartFormPage(HDC hdc);

VOID 
STDCALL
UnloadNetworkFonts(DWORD unknown);

WINBOOL 
STDCALL
bInitSystemAndFontsDirectoriesW(LPWSTR *SystemDir,LPWSTR *FontsDir);

WINBOOL 
STDCALL
bMakePathNameW(LPWSTR lpBuffer,LPCWSTR lpFileName,LPWSTR *lpFilePart,DWORD unknown);

WINBOOL
STDCALL
GdiAddGlsRecord(HDC hdc,DWORD unknown1,LPCSTR unknown2,LPRECT unknown3);

HANDLE
STDCALL
GdiConvertMetaFilePict(HGLOBAL hMem);

DEVMODEW *
STDCALL
GdiConvertToDevmodeW(DEVMODEA *dm);

HENHMETAFILE
STDCALL
GdiCreateLocalEnhMetaFile(HENHMETAFILE hmo);

METAFILEPICT *
STDCALL
GdiCreateLocalMetaFilePict(HENHMETAFILE hmo);

DWORD
STDCALL
GdiGetCharDimensions(HDC hdc,LPTEXTMETRICW lptm,BOOL unk);

PSHAREDHANDLETABLE
STDCALL
GdiQueryTable(VOID);

HANDLE 
STDCALL
GdiGetSpoolFileHandle(
	LPWSTR		pwszPrinterName,
	LPDEVMODEW	pDevmode,
	LPWSTR		pwszDocName);

WINBOOL
STDCALL
GdiDeleteSpoolFileHandle(
	HANDLE	SpoolFileHandle);

DWORD 
STDCALL
GdiGetPageCount(
	HANDLE	SpoolFileHandle);

HDC
STDCALL
GdiGetDC(
	HANDLE	SpoolFileHandle);

HANDLE 
STDCALL
GdiGetPageHandle(
	HANDLE	SpoolFileHandle,
	DWORD	Page,
	LPDWORD	pdwPageType);

WINBOOL
STDCALL
GdiStartDocEMF(
	HANDLE		SpoolFileHandle,
	DOCINFOW	*pDocInfo);

WINBOOL
STDCALL
GdiStartPageEMF(
	HANDLE	SpoolFileHandle);

WINBOOL
STDCALL
GdiPlayPageEMF(
	HANDLE	SpoolFileHandle,
	HANDLE	hemf,
	RECT	*prectDocument,
	RECT	*prectBorder,
	RECT	*prectClip);

WINBOOL
STDCALL
GdiEndPageEMF(
	HANDLE	SpoolFileHandle,
	DWORD	dwOptimization);

WINBOOL
STDCALL
GdiEndDocEMF(
	HANDLE	SpoolFileHandle);

WINBOOL
STDCALL
GdiGetDevmodeForPage(
	HANDLE		SpoolFileHandle,
	DWORD		dwPageNumber,
	PDEVMODEW	*pCurrDM,
	PDEVMODEW	*pLastDM);

WINBOOL
STDCALL
GdiResetDCEMF(
	HANDLE		SpoolFileHandle,
	PDEVMODEW	pCurrDM);

WINBOOL 
STDCALL
GdiPlayEMF
(
	LPWSTR     pwszPrinterName,
	LPDEVMODEW pDevmode,
	LPWSTR     pwszDocName,
	EMFPLAYPROC pfnEMFPlayFn,
	HANDLE     hPageQuery
);

WINBOOL
STDCALL
GdiInitSpool(VOID);

WINBOOL
STDCALL
GdiPlayPrivatePageEMF
(
	HANDLE	SpoolFileHandle,
	DWORD	unknown,
	RECT	*prectDocument
);

WINBOOL 
STDCALL
AlignRects(LPRECT rect, DWORD b, DWORD c, DWORD d);

LRESULT
STDCALL
DefRawInputProc(
    PRAWINPUT* paRawInput,
    INT nInput,
    UINT cbSizeHeader);

VOID
STDCALL 
DisableProcessWindowsGhosting(VOID);

WINBOOL
STDCALL
GetLayeredWindowAttributes(
    HWND hwnd,
    COLORREF *pcrKey,
    BYTE *pbAlpha,
    DWORD *pdwFlags);

UINT
STDCALL
GetRawInputBuffer(
    PRAWINPUT   pData,
    PUINT    pcbSize,
    UINT         cbSizeHeader);

UINT
STDCALL
GetRawInputData(
    HRAWINPUT    hRawInput,
    UINT         uiCommand,
    LPVOID      pData,
    PUINT    pcbSize,
    UINT         cbSizeHeader);

UINT
STDCALL
GetRawInputDeviceList(
    PRAWINPUTDEVICELIST pRawInputDeviceList,
    PUINT puiNumDevices,
    UINT cbSize);

UINT
STDCALL
GetRegisteredRawInputDevices(
    PRAWINPUTDEVICE pRawInputDevices,
    PUINT puiNumDevices,
    UINT cbSize);

int
STDCALL
GetWindowRgnBox(
    HWND hWnd,
    LPRECT lprc);

WINBOOL
STDCALL
IsGUIThread(
    WINBOOL bConvert);

WINBOOL
STDCALL
IsWinEventHookInstalled(
    DWORD event);

WINBOOL
STDCALL
PrintWindow(
    HWND hwnd,
    HDC hdcBlt,
    UINT nFlags);

WINBOOL
STDCALL
RegisterRawInputDevices(
    PCRAWINPUTDEVICE pRawInputDevices,
    UINT uiNumDevices,
    UINT cbSize);

DWORD
STDCALL 
SetSysColorsTemp(
		 const COLORREF *pPens,
		 const HBRUSH *pBrushes,
		 DWORD n
		 );

INT
STDCALL
UserClientDllInitialize(
			HINSTANCE inst,
			DWORD reason,
			LPVOID reserved
			);

DWORD
STDCALL
DrawMenuBarTemp(
		HWND hwnd,
		HDC hDC,
		LPRECT lprect,
		HMENU hMenu,
		HFONT hFont
		);

UINT
STDCALL
UserRealizePalette ( HDC hDC );

WINBOOL
STDCALL
DdeFreeStringHandle ( DWORD idInst, HSZ hsz );

WINBOOL
STDCALL
DeregisterShellHookWindow ( HWND hWnd );

WINBOOL
STDCALL
RegisterShellHookWindow ( HWND hWnd );

WINBOOL
STDCALL
EndTask(
	HWND hWnd,
	WINBOOL fShutDown,
	WINBOOL fForce
	);

DWORD
STDCALL
InternalGetWindowText(
		      HWND hWnd,
		      LPWSTR lpString,
		      int nMaxCount
		      );

WINBOOL
STDCALL
IsHungAppWindow ( HWND hwnd );

VOID
STDCALL
NotifyWinEvent(
	       DWORD event,
	       HWND hwnd,
	       LONG idObject,
	       LONG idChild
	       );

HWINEVENTHOOK
STDCALL
SetWinEventHook(
		DWORD eventMin,
		DWORD eventMax,
		HMODULE hmodWinEventProc,
		WINEVENTPROC pfnWinEventProc,
		DWORD idProcess,
		DWORD idThread,
		DWORD dwFlags
		);

VOID
STDCALL
SwitchToThisWindow(
		   HWND hwnd,
		   WINBOOL fUnknown );

WINBOOL
STDCALL
UnhookWinEvent ( HWINEVENTHOOK hWinEventHook );

LRESULT
STDCALL
EditWndProc(
	    HWND hWnd,
	    UINT Msg,
	    WPARAM wParam,
	    LPARAM lParam
	    );

WINBOOL
STDCALL
SetDeskWallpaper ( LPCSTR filename );

DWORD
STDCALL
GetAppCompatFlags ( HTASK hTask );

HDESK
STDCALL
GetInputDesktop ( VOID );

UINT
STDCALL
GetInternalWindowPos(
		     HWND hwnd,
		     LPRECT rectWnd,
		     LPPOINT ptIcon
		     );

HWND
STDCALL
GetProgmanWindow ( VOID );

HWND
STDCALL
GetTaskmanWindow ( VOID );

DWORD
STDCALL
SetWindowStationUser(
		       DWORD Unknown1,
		       DWORD Unknown2,
		       DWORD Unknown3,
		       DWORD Unknown4
		     );

UINT
STDCALL
SetSystemTimer(
	      HWND hWnd,
	      UINT_PTR IDEvent,
	      UINT Period,
	      TIMERPROC TimerFunc
	      );

WINBOOL
STDCALL
SetSystemMenu(
	      HWND hwnd,
	      HMENU hMenu
	      );

HWND
STDCALL
SetTaskmanWindow ( HWND x );

HWND
STDCALL
SetProgmanWindow ( HWND x );

VOID
STDCALL
ScrollChildren(
	       HWND hWnd,
	       UINT uMsg,
	       WPARAM wParam,
	       LPARAM lParam
	       );

VOID
STDCALL
LoadLocalFonts ( VOID );

VOID
STDCALL
LoadRemoteFonts ( VOID );

VOID
STDCALL
SetInternalWindowPos(
		     HWND hwnd,
		     UINT showCmd,
		     LPRECT rect,
		     LPPOINT pt
		     );

VOID
STDCALL
RegisterSystemThread(
		     DWORD flags,
		     DWORD reserved
		     );

DWORD
STDCALL
RegisterLogonProcess(
		     HANDLE hprocess,
		     BOOL x
		     );

WINBOOL
STDCALL
KillSystemTimer(
		     HWND hWnd,
		     UINT_PTR IDEvent
		     );

DWORD
STDCALL
RegisterTasklist ( DWORD x );

DWORD
STDCALL
SetLogonNotifyWindow ( HWINSTA hwinsta, HWND hwnd );

WORD
STDCALL
CascadeChildWindows ( HWND hWndParent, WORD wFlags );

WORD
STDCALL
TileChildWindows ( HWND hWndParent, WORD wFlags );

WINBOOL
STDCALL
TranslateMessageEx ( CONST MSG *lpMsg, DWORD unk );

WINBOOL
STDCALL
DrawFrame(
	  HDC hDc,
	  RECT *r,
	  DWORD width,
	  DWORD type
	  );

WINBOOL
STDCALL
LockWindowStation ( HWINSTA hWinSta );

WINBOOL
STDCALL
UnlockWindowStation ( HWINSTA hWinSta );

WINBOOL
STDCALL
GetAccCursorInfo ( PCURSORINFO pci );

LPWSTR
STDCALL
MB_GetString ( DWORD string );

WINBOOL
STDCALL
ClientThreadSetup ( VOID );

WINBOOL
STDCALL
PolyPatBlt(
	   HDC hDC,
	   DWORD dwRop,
	   PPATRECT pRects,
	   int cRects,
	   ULONG Reserved
	   );

DWORD
STDCALL
WCSToMBEx(
	  WORD CodePage,
	  LPWSTR UnicodeString,
	  LONG UnicodeSize,
	  LPSTR *MBString,
	  LONG MBSize,
	  BOOL Allocate
	  );

DWORD
STDCALL
MBToWCSEx(
	  WORD CodePage,
	  LPSTR MBString,
	  LONG MBSize,
	  LPWSTR *UnicodeString,
	  LONG UnicodeSize,
	  BOOL Allocate
	  );

HWND
STDCALL
GetAncestor ( HWND hWnd, UINT GaFlags );

WINBOOL
STDCALL
AbnormalTermination ( VOID );

int
STDCALL
AbortDoc ( HDC );

WINBOOL
STDCALL
AbortPath ( HDC );

WINBOOL
STDCALL
AbortPrinter ( HANDLE );

WINBOOL
CALLBACK
AbortProc ( HDC, int );

WINBOOL
STDCALL
AccessCheck(
	    PSECURITY_DESCRIPTOR  pSecurityDescriptor,
	    HANDLE  ClientToken,
	    DWORD  DesiredAccess,
	    PGENERIC_MAPPING  GenericMapping,
	    PPRIVILEGE_SET  PrivilegeSet,
	    LPDWORD  PrivilegeSetLength,
	    LPDWORD  GrantedAccess,
	    LPBOOL  AccessStatus
	    );

#ifndef __NTDRIVER__
LONG
STDCALL
InterlockedIncrement(
		     LPLONG lpAddend
		     );

LONG
STDCALL
InterlockedDecrement(
		     LPLONG lpAddend
		     );

LONG
STDCALL
InterlockedExchange(
		    LPLONG Target,
		    LONG Value
		    );

PVOID
STDCALL
InterlockedCompareExchange(
	    PVOID *Destination,
	    PVOID Exchange,
	    PVOID Comperand
	    );
#endif

WINBOOL
STDCALL
FreeResource(
	     HGLOBAL hResData
	     );

LPVOID
STDCALL
LockResource(
	     HGLOBAL hResData
	     );

int
#ifdef __GNUC__
STDCALL
#else
__stdcall
#endif
WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nShowCmd
	);

WINBOOL
STDCALL
FreeLibrary(
	    HINSTANCE hLibModule
	    );


VOID
STDCALL
FreeLibraryAndExitThread(
			 HMODULE hLibModule,
			 DWORD dwExitCode
			 );

WINBOOL
STDCALL
DisableThreadLibraryCalls(
			  HMODULE hLibModule
			  );

FARPROC
STDCALL
GetProcAddress(
	       HINSTANCE hModule,
	       LPCSTR lpProcName
	       );

DWORD
STDCALL
GetVersion( VOID );

HGLOBAL
STDCALL
GlobalAlloc(
	    UINT uFlags,
	    DWORD dwBytes
	    );


HGLOBAL
STDCALL
GlobalReAlloc(
	      HGLOBAL hMem,
	      DWORD dwBytes,
	      UINT uFlags
	      );

DWORD
STDCALL
GlobalSize(
	   HGLOBAL hMem
	   );

UINT
STDCALL
GlobalFlags(
	    HGLOBAL hMem
	    );


LPVOID
STDCALL
GlobalLock(
	   HGLOBAL hMem
	   );

HGLOBAL
STDCALL
GlobalHandle(
	     LPCVOID pMem
	     );


WINBOOL
STDCALL
GlobalUnlock(
	     HGLOBAL hMem
	     );


HGLOBAL
STDCALL
GlobalFree(
	   HGLOBAL hMem
	   );

UINT
STDCALL
GlobalCompact(
	      DWORD dwMinFree
	      );


VOID
STDCALL
GlobalFix(
	  HGLOBAL hMem
	  );


VOID
STDCALL
GlobalUnfix(
	    HGLOBAL hMem
	    );


LPVOID
STDCALL
GlobalWire(
	   HGLOBAL hMem
	   );


WINBOOL
STDCALL
GlobalUnWire(
	     HGLOBAL hMem
	     );


VOID
STDCALL
GlobalMemoryStatus(
		   LPMEMORYSTATUS lpBuffer
		   );


HLOCAL
STDCALL
LocalAlloc(
	   UINT uFlags,
	   UINT uBytes
	   );

HLOCAL
LocalDiscard(
	     HLOCAL hlocMem
	     );

HLOCAL
STDCALL
LocalReAlloc(
	     HLOCAL hMem,
	     UINT uBytes,
	     UINT uFlags
	     );


LPVOID
STDCALL
LocalLock(
	  HLOCAL hMem
	  );


HLOCAL
STDCALL
LocalHandle(
	    LPCVOID pMem
	    );


WINBOOL
STDCALL
LocalUnlock(
	    HLOCAL hMem
	    );


UINT
STDCALL
LocalSize(
	  HLOCAL hMem
	  );


UINT
STDCALL
LocalFlags(
	   HLOCAL hMem
	   );


HLOCAL
STDCALL
LocalFree(
	  HLOCAL hMem
	  );


UINT
STDCALL
LocalShrink(
	    HLOCAL hMem,
	    UINT cbNewSize
	    );


UINT
STDCALL
LocalCompact(
	     UINT uMinFree
	     );


WINBOOL
STDCALL
FlushInstructionCache(
		      HANDLE hProcess,
		      LPCVOID lpBaseAddress,
		      DWORD dwSize
		      );


LPVOID
STDCALL
VirtualAlloc(
	     LPVOID lpAddress,
	     DWORD dwSize,
	     DWORD flAllocationType,
	     DWORD flProtect
	     );


WINBOOL
STDCALL
VirtualFree(
	    LPVOID lpAddress,
	    DWORD dwSize,
	    DWORD dwFreeType
	    );


WINBOOL
STDCALL
VirtualProtect(
	       LPVOID lpAddress,
	       DWORD dwSize,
	       DWORD flNewProtect,
	       PDWORD lpflOldProtect
	       );


DWORD
STDCALL
VirtualQuery(
	     LPCVOID lpAddress,
	     PMEMORY_BASIC_INFORMATION lpBuffer,
	     DWORD dwLength
	     );


WINBOOL
STDCALL
VirtualProtectEx(
		 HANDLE hProcess,
		 LPVOID lpAddress,
		 DWORD dwSize,
		 DWORD flNewProtect,
		 PDWORD lpflOldProtect
		 );


DWORD
STDCALL
VirtualQueryEx(
	       HANDLE hProcess,
	       LPCVOID lpAddress,
	       PMEMORY_BASIC_INFORMATION lpBuffer,
	       DWORD dwLength
	       );


HANDLE
STDCALL
HeapCreate(
	   DWORD flOptions,
	   DWORD dwInitialSize,
	   DWORD dwMaximumSize
	   );

WINBOOL
STDCALL
HeapDestroy(
	    HANDLE hHeap
	    );

LPVOID
STDCALL
HeapAlloc(
	  HANDLE hHeap,
	  DWORD dwFlags,
	  DWORD dwBytes
	  );

LPVOID
STDCALL
HeapReAlloc(
	    HANDLE hHeap,
	    DWORD dwFlags,
	    LPVOID lpMem,
	    DWORD dwBytes
	    );

WINBOOL
STDCALL
HeapFree(
	 HANDLE hHeap,
	 DWORD dwFlags,
	 LPVOID lpMem
	 );

DWORD
STDCALL
HeapSize(
	 HANDLE hHeap,
	 DWORD dwFlags,
	 LPCVOID lpMem
	 );

WINBOOL
STDCALL
HeapValidate(
	     HANDLE hHeap,
	     DWORD dwFlags,
	     LPCVOID lpMem
	     );

UINT
STDCALL
HeapCompact(
	    HANDLE hHeap,
	    DWORD dwFlags
	    );

HANDLE
STDCALL
GetProcessHeap( VOID );

DWORD
STDCALL
GetProcessHeaps(
		DWORD NumberOfHeaps,
		PHANDLE ProcessHeaps
		);

DWORD
STDCALL
GetProcessVersion (
		DWORD	ProcessId
		);

WINBOOL
STDCALL
HeapLock(
	 HANDLE hHeap
	 );

WINBOOL
STDCALL
HeapUnlock(
	   HANDLE hHeap
	   );

WINBOOL
STDCALL
HeapWalk(
	 HANDLE hHeap,
	 LPPROCESS_HEAP_ENTRY lpEntry
	 );

WINBOOL
STDCALL
GetProcessAffinityMask(
		       HANDLE hProcess,
		       LPDWORD lpProcessAffinityMask,
		       LPDWORD lpSystemAffinityMask
		       );

WINBOOL
STDCALL
GetProcessTimes(
		HANDLE hProcess,
		LPFILETIME lpCreationTime,
		LPFILETIME lpExitTime,
		LPFILETIME lpKernelTime,
		LPFILETIME lpUserTime
		);

WINBOOL
STDCALL
GetProcessWorkingSetSize(
			 HANDLE hProcess,
			 LPDWORD lpMinimumWorkingSetSize,
			 LPDWORD lpMaximumWorkingSetSize
			 );

WINBOOL
STDCALL
SetProcessWorkingSetSize(
			 HANDLE hProcess,
			 DWORD dwMinimumWorkingSetSize,
			 DWORD dwMaximumWorkingSetSize
			 );

HANDLE
STDCALL
OpenProcess(
	    DWORD dwDesiredAccess,
	    WINBOOL bInheritHandle,
	    DWORD dwProcessId
	    );

HANDLE
STDCALL
GetCurrentProcess(
		  VOID
		  );

DWORD
STDCALL
GetCurrentProcessId(
		    VOID
		    );

VOID
STDCALL
ExitProcess(
	    UINT uExitCode
	    ) __attribute__ ((noreturn));

WINBOOL
STDCALL
TerminateProcess(
		 HANDLE hProcess,
		 UINT uExitCode
		 );

WINBOOL
STDCALL
GetExitCodeProcess(
		   HANDLE hProcess,
		   LPDWORD lpExitCode
		   );

VOID
STDCALL
FatalExit(
	  int ExitCode
	  );

VOID
STDCALL
RaiseException(
	       DWORD dwExceptionCode,
	       DWORD dwExceptionFlags,
	       DWORD nNumberOfArguments,
	       CONST DWORD *lpArguments
	       );

LONG
STDCALL
UnhandledExceptionFilter(
			 struct _EXCEPTION_POINTERS *ExceptionInfo
			 );

/*
 TODO: what is TOP_LEVEL_EXCEPTION_FILTER?
LPTOP_LEVEL_EXCEPTION_FILTER
STDCALL
SetUnhandledExceptionFilter(
    LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter
    );
*/


HANDLE
STDCALL
CreateThread(
	     LPSECURITY_ATTRIBUTES lpThreadAttributes,
	     DWORD dwStackSize,
	     LPTHREAD_START_ROUTINE lpStartAddress,
	     LPVOID lpParameter,
	     DWORD dwCreationFlags,
	     LPDWORD lpThreadId
	     );


HANDLE
STDCALL
CreateRemoteThread(
		   HANDLE hProcess,
		   LPSECURITY_ATTRIBUTES lpThreadAttributes,
		   DWORD dwStackSize,
		   LPTHREAD_START_ROUTINE lpStartAddress,
		   LPVOID lpParameter,
		   DWORD dwCreationFlags,
		   LPDWORD lpThreadId
		   );


HANDLE
STDCALL
GetCurrentThread(
		 VOID
		 );


DWORD
STDCALL
GetCurrentThreadId(
		   VOID
		   );


DWORD
STDCALL
SetThreadAffinityMask(
		      HANDLE hThread,
		      DWORD dwThreadAffinityMask
		      );


WINBOOL
STDCALL
SetThreadPriority(
		  HANDLE hThread,
		  int nPriority
		  );


int
STDCALL
GetThreadPriority(
		  HANDLE hThread
		  );


WINBOOL
STDCALL
GetThreadTimes(
	       HANDLE hThread,
	       LPFILETIME lpCreationTime,
	       LPFILETIME lpExitTime,
	       LPFILETIME lpKernelTime,
	       LPFILETIME lpUserTime
	       );


__declspec(noreturn)
VOID
STDCALL
ExitThread(
	   DWORD dwExitCode
	   );


WINBOOL
STDCALL
TerminateThread(
		HANDLE hThread,
		DWORD dwExitCode
		);


WINBOOL
STDCALL
GetExitCodeThread(
		  HANDLE hThread,
		  LPDWORD lpExitCode
		  );

WINBOOL
STDCALL
GetThreadSelectorEntry(
		       HANDLE hThread,
		       DWORD dwSelector,
		       LPLDT_ENTRY lpSelectorEntry
		       );


DWORD
STDCALL
GetLastError(
	     VOID
	     );


VOID
STDCALL
SetLastError(
	     DWORD dwErrCode
	     );


WINBOOL
STDCALL
GetOverlappedResult(
		    HANDLE hFile,
		    LPOVERLAPPED lpOverlapped,
		    LPDWORD lpNumberOfBytesTransferred,
		    WINBOOL bWait
		    );


HANDLE
STDCALL
CreateIoCompletionPort(
		       HANDLE FileHandle,
		       HANDLE ExistingCompletionPort,
		       DWORD CompletionKey,
		       DWORD NumberOfConcurrentThreads
		       );


WINBOOL
STDCALL
GetQueuedCompletionStatus(
			  HANDLE CompletionPort,
			  LPDWORD lpNumberOfBytesTransferred,
			  LPDWORD lpCompletionKey,
			  LPOVERLAPPED *lpOverlapped,
			  DWORD dwMilliseconds
			  );

UINT
STDCALL
SetErrorMode(
	     UINT uMode
	     );


WINBOOL
STDCALL
ReadProcessMemory(
		  HANDLE hProcess,
		  LPCVOID lpBaseAddress,
		  LPVOID lpBuffer,
		  DWORD nSize,
		  LPDWORD lpNumberOfBytesRead
		  );


WINBOOL
STDCALL
WriteProcessMemory(
		   HANDLE hProcess,
		   LPVOID lpBaseAddress,
		   LPVOID lpBuffer,
		   DWORD nSize,
		   LPDWORD lpNumberOfBytesWritten
		   );


WINBOOL
STDCALL
GetThreadContext(
		 HANDLE hThread,
		 LPCONTEXT lpContext
		 );


WINBOOL
STDCALL
SetThreadContext(
		 HANDLE hThread,
		 CONST CONTEXT *lpContext
		 );


DWORD
STDCALL
SuspendThread(
	      HANDLE hThread
	      );


DWORD
STDCALL
ResumeThread(
	     HANDLE hThread
	     );


VOID
STDCALL
DebugBreak(
	   VOID
	   );


WINBOOL
STDCALL
WaitForDebugEvent(
		  LPDEBUG_EVENT lpDebugEvent,
		  DWORD dwMilliseconds
		  );


WINBOOL
STDCALL
ContinueDebugEvent(
		   DWORD dwProcessId,
		   DWORD dwThreadId,
		   DWORD dwContinueStatus
		   );


WINBOOL
STDCALL
DebugActiveProcess(
		   DWORD dwProcessId
		   );


VOID
STDCALL
InitializeCriticalSection(
			  LPCRITICAL_SECTION lpCriticalSection
			  );


VOID
STDCALL
EnterCriticalSection(
		     LPCRITICAL_SECTION lpCriticalSection
		     );


VOID
STDCALL
LeaveCriticalSection(
		     LPCRITICAL_SECTION lpCriticalSection
		     );


VOID
STDCALL
DeleteCriticalSection(
		      LPCRITICAL_SECTION lpCriticalSection
		      );


WINBOOL
STDCALL
SetEvent(
	 HANDLE hEvent
	 );


WINBOOL
STDCALL
ResetEvent(
	   HANDLE hEvent
	   );


WINBOOL
STDCALL
PulseEvent(
	   HANDLE hEvent
	   );


WINBOOL
STDCALL
ReleaseSemaphore(
		 HANDLE hSemaphore,
		 LONG lReleaseCount,
		 LPLONG lpPreviousCount
		 );


WINBOOL
STDCALL
ReleaseMutex(
	     HANDLE hMutex
	     );


DWORD
STDCALL
WaitForSingleObject(
		    HANDLE hHandle,
		    DWORD dwMilliseconds
		    );


DWORD
STDCALL
WaitForMultipleObjects(
		       DWORD nCount,
		       CONST HANDLE *lpHandles,
		       WINBOOL bWaitAll,
		       DWORD dwMilliseconds
		       );


VOID
STDCALL
Sleep(
      DWORD dwMilliseconds
      );


HGLOBAL
STDCALL
LoadResource(
	     HINSTANCE hModule,
	     HRSRC hResInfo
	     );


DWORD
STDCALL
SizeofResource(
	       HINSTANCE hModule,
	       HRSRC hResInfo
	       );



ATOM
STDCALL
GlobalDeleteAtom(
		 ATOM nAtom
		 );


WINBOOL
STDCALL
InitAtomTable(
	      DWORD nSize
	      );


ATOM
STDCALL
DeleteAtom(
	   ATOM nAtom
	   );


UINT
STDCALL
SetHandleCount(
	       UINT uNumber
	       );


DWORD
STDCALL
GetLogicalDrives(
		 VOID
		 );


WINBOOL
STDCALL
LockFile(
	 HANDLE hFile,
	 DWORD dwFileOffsetLow,
	 DWORD dwFileOffsetHigh,
	 DWORD nNumberOfBytesToLockLow,
	 DWORD nNumberOfBytesToLockHigh
	 );


WINBOOL
STDCALL
UnlockFile(
	   HANDLE hFile,
	   DWORD dwFileOffsetLow,
	   DWORD dwFileOffsetHigh,
	   DWORD nNumberOfBytesToUnlockLow,
	   DWORD nNumberOfBytesToUnlockHigh
	   );


WINBOOL
STDCALL
LockFileEx(
	   HANDLE hFile,
	   DWORD dwFlags,
	   DWORD dwReserved,
	   DWORD nNumberOfBytesToLockLow,
	   DWORD nNumberOfBytesToLockHigh,
	   LPOVERLAPPED lpOverlapped
	   );

WINBOOL
STDCALL
UnlockFileEx(
	     HANDLE hFile,
	     DWORD dwReserved,
	     DWORD nNumberOfBytesToUnlockLow,
	     DWORD nNumberOfBytesToUnlockHigh,
	     LPOVERLAPPED lpOverlapped
	     );

WINBOOL
STDCALL
GetFileInformationByHandle(
			   HANDLE hFile,
			   LPBY_HANDLE_FILE_INFORMATION lpFileInformation
			   );


DWORD
STDCALL
GetFileType(
	    HANDLE hFile
	    );


DWORD
STDCALL
GetFileSize(
	    HANDLE hFile,
	    LPDWORD lpFileSizeHigh
	    );


HANDLE
STDCALL
GetStdHandle(
	     DWORD nStdHandle
	     );


WINBOOL
STDCALL
SetStdHandle(
	     DWORD nStdHandle,
	     HANDLE hHandle
	     );


WINBOOL
STDCALL
WriteFile(
	  HANDLE hFile,
	  LPCVOID lpBuffer,
	  DWORD nNumberOfBytesToWrite,
	  LPDWORD lpNumberOfBytesWritten,
	  LPOVERLAPPED lpOverlapped
	  );


WINBOOL
STDCALL
ReadFile(
	 HANDLE hFile,
	 LPVOID lpBuffer,
	 DWORD nNumberOfBytesToRead,
	 LPDWORD lpNumberOfBytesRead,
	 LPOVERLAPPED lpOverlapped
	 );


WINBOOL
STDCALL
FlushFileBuffers(
		 HANDLE hFile
		 );


WINBOOL
STDCALL
DeviceIoControl(
		HANDLE hDevice,
		DWORD dwIoControlCode,
		LPVOID lpInBuffer,
		DWORD nInBufferSize,
		LPVOID lpOutBuffer,
		DWORD nOutBufferSize,
		LPDWORD lpBytesReturned,
		LPOVERLAPPED lpOverlapped
		);


WINBOOL
STDCALL
SetEndOfFile(
	     HANDLE hFile
	     );


DWORD
STDCALL
SetFilePointer(
	       HANDLE hFile,
	       LONG lDistanceToMove,
	       PLONG lpDistanceToMoveHigh,
	       DWORD dwMoveMethod
	       );


WINBOOL
STDCALL
FindClose(
	  HANDLE hFindFile
	  );


WINBOOL
STDCALL
GetFileTime(
	    HANDLE hFile,
	    LPFILETIME lpCreationTime,
	    LPFILETIME lpLastAccessTime,
	    LPFILETIME lpLastWriteTime
	    );


WINBOOL
STDCALL
SetFileTime(
	    HANDLE hFile,
	    CONST FILETIME *lpCreationTime,
	    CONST FILETIME *lpLastAccessTime,
	    CONST FILETIME *lpLastWriteTime
	    );


WINBOOL
STDCALL
CloseHandle(
	    HANDLE hObject
	    );


WINBOOL
STDCALL
DuplicateHandle(
		HANDLE hSourceProcessHandle,
		HANDLE hSourceHandle,
		HANDLE hTargetProcessHandle,
		LPHANDLE lpTargetHandle,
		DWORD dwDesiredAccess,
		WINBOOL bInheritHandle,
		DWORD dwOptions
		);


WINBOOL
STDCALL
GetHandleInformation(
		     HANDLE hObject,
		     LPDWORD lpdwFlags
		     );


WINBOOL
STDCALL
SetHandleInformation(
		     HANDLE hObject,
		     DWORD dwMask,
		     DWORD dwFlags
		     );

DWORD
STDCALL
LoadModule(
	   LPCSTR lpModuleName,
	   LPVOID lpParameterBlock
	   );


UINT
STDCALL
WinExec(
	LPCSTR lpCmdLine,
	UINT uCmdShow
	);


WINBOOL
STDCALL
ClearCommBreak(
	       HANDLE hFile
	       );


WINBOOL
STDCALL
ClearCommError(
	       HANDLE hFile,
	       LPDWORD lpErrors,
	       LPCOMSTAT lpStat
	       );


WINBOOL
STDCALL
SetupComm(
	  HANDLE hFile,
	  DWORD dwInQueue,
	  DWORD dwOutQueue
	  );


WINBOOL
STDCALL
EscapeCommFunction(
		   HANDLE hFile,
		   DWORD dwFunc
		   );


WINBOOL
STDCALL
GetCommConfig(
	      HANDLE hCommDev,
	      LPCOMMCONFIG lpCC,
	      LPDWORD lpdwSize
	      );


WINBOOL
STDCALL
GetCommMask(
	    HANDLE hFile,
	    LPDWORD lpEvtMask
	    );


WINBOOL
STDCALL
GetCommProperties(
		  HANDLE hFile,
		  LPCOMMPROP lpCommProp
		  );


WINBOOL
STDCALL
GetCommModemStatus(
		   HANDLE hFile,
		   LPDWORD lpModemStat
		   );


WINBOOL
STDCALL
GetCommState(
	     HANDLE hFile,
	     LPDCB lpDCB
	     );


WINBOOL
STDCALL
GetCommTimeouts(
		HANDLE hFile,
		LPCOMMTIMEOUTS lpCommTimeouts
		);


WINBOOL
STDCALL
PurgeComm(
	  HANDLE hFile,
	  DWORD dwFlags
	  );


WINBOOL
STDCALL
SetCommBreak(
	     HANDLE hFile
	     );


WINBOOL
STDCALL
SetCommConfig(
	      HANDLE hCommDev,
	      LPCOMMCONFIG lpCC,
	      DWORD dwSize
	      );


WINBOOL
STDCALL
SetCommMask(
	    HANDLE hFile,
	    DWORD dwEvtMask
	    );


WINBOOL
STDCALL
SetCommState(
	     HANDLE hFile,
	     LPDCB lpDCB
	     );


WINBOOL
STDCALL
SetCommTimeouts(
		HANDLE hFile,
		LPCOMMTIMEOUTS lpCommTimeouts
		);


WINBOOL
STDCALL
TransmitCommChar(
		 HANDLE hFile,
		 char cChar
		 );


WINBOOL
STDCALL
WaitCommEvent(
	      HANDLE hFile,
	      LPDWORD lpEvtMask,
	      LPOVERLAPPED lpOverlapped
	      );



DWORD
STDCALL
SetTapePosition(
		HANDLE hDevice,
		DWORD dwPositionMethod,
		DWORD dwPartition,
		DWORD dwOffsetLow,
		DWORD dwOffsetHigh,
		WINBOOL bImmediate
		);


DWORD
STDCALL
GetTapePosition(
		HANDLE hDevice,
		DWORD dwPositionType,
		LPDWORD lpdwPartition,
		LPDWORD lpdwOffsetLow,
		LPDWORD lpdwOffsetHigh
		);


DWORD
STDCALL
PrepareTape(
	    HANDLE hDevice,
	    DWORD dwOperation,
	    WINBOOL bImmediate
	    );

BOOL
STDCALL
ProcessIdToSessionId(
	DWORD dwProcessId,
	DWORD* pSessionId
	);

DWORD
STDCALL
EraseTape(
	  HANDLE hDevice,
	  DWORD dwEraseType,
	  WINBOOL bImmediate
	  );


DWORD
STDCALL
CreateTapePartition(
		    HANDLE hDevice,
		    DWORD dwPartitionMethod,
		    DWORD dwCount,
		    DWORD dwSize
		    );


DWORD
STDCALL
WriteTapemark(
	      HANDLE hDevice,
	      DWORD dwTapemarkType,
	      DWORD dwTapemarkCount,
	      WINBOOL bImmediate
	      );


DWORD
STDCALL
GetTapeStatus(
	      HANDLE hDevice
	      );


DWORD
STDCALL
GetTapeParameters(
		  HANDLE hDevice,
		  DWORD dwOperation,
		  LPDWORD lpdwSize,
		  LPVOID lpTapeInformation
		  );

DWORD
STDCALL
SetTapeParameters(
		  HANDLE hDevice,
		  DWORD dwOperation,
		  LPVOID lpTapeInformation
		  );

WINBOOL
STDCALL
Beep(
     DWORD dwFreq,
     DWORD dwDuration
     );


VOID
STDCALL
OpenSound(
	  VOID
	  );


VOID
STDCALL
CloseSound(
	   VOID
	   );


VOID
STDCALL
StartSound(
	   VOID
	   );


VOID
STDCALL
StopSound(
	  VOID
	  );


DWORD
STDCALL
WaitSoundState(
	       DWORD nState
	       );


DWORD
STDCALL
SyncAllVoices(
	      VOID
	      );


DWORD
STDCALL
CountVoiceNotes(
		DWORD nVoice
		);


LPDWORD
STDCALL
GetThresholdEvent(
		  VOID
		  );


DWORD
STDCALL
GetThresholdStatus(
		   VOID
		   );


DWORD
STDCALL
SetSoundNoise(
	      DWORD nSource,
	      DWORD nDuration
	      );


DWORD
STDCALL
SetVoiceAccent(
	       DWORD nVoice,
	       DWORD nTempo,
	       DWORD nVolume,
	       DWORD nMode,
	       DWORD nPitch
	       );


DWORD
STDCALL
SetVoiceEnvelope(
		 DWORD nVoice,
		 DWORD nShape,
		 DWORD nRepeat
		 );


DWORD
STDCALL
SetVoiceNote(
	     DWORD nVoice,
	     DWORD nValue,
	     DWORD nLength,
	     DWORD nCdots
	     );


DWORD
STDCALL
SetVoiceQueueSize(
		  DWORD nVoice,
		  DWORD nBytes
		  );


DWORD
STDCALL
SetVoiceSound(
	      DWORD nVoice,
	      DWORD Frequency,
	      DWORD nDuration
	      );


DWORD
STDCALL
SetVoiceThreshold(
		  DWORD nVoice,
		  DWORD nNotes
		  );


int
STDCALL
MulDiv(
       int nNumber,
       int nNumerator,
       int nDenominator
       );


VOID
STDCALL
GetSystemTime(
	      LPSYSTEMTIME lpSystemTime
	      );

VOID
STDCALL
GetSystemTimeAsFileTime (
	LPFILETIME	lpSystemTimeAsFileTime 
	);

WINBOOL
STDCALL
SetSystemTime(
	      CONST SYSTEMTIME *lpSystemTime
	      );


VOID
STDCALL
GetLocalTime(
	     LPSYSTEMTIME lpSystemTime
	     );


WINBOOL
STDCALL
SetLocalTime(
	     CONST SYSTEMTIME *lpSystemTime
	     );


VOID
STDCALL
GetSystemInfo(
	      LPSYSTEM_INFO lpSystemInfo
	      );

WINBOOL
STDCALL
SystemTimeToTzSpecificLocalTime(
				LPTIME_ZONE_INFORMATION lpTimeZoneInformation,
				LPSYSTEMTIME lpUniversalTime,
				LPSYSTEMTIME lpLocalTime
				);


DWORD
STDCALL
GetTimeZoneInformation(
		       LPTIME_ZONE_INFORMATION lpTimeZoneInformation
		       );


WINBOOL
STDCALL
SetTimeZoneInformation(
		       CONST TIME_ZONE_INFORMATION *lpTimeZoneInformation
		       );

WINBOOL
STDCALL
SystemTimeToFileTime(
		     CONST SYSTEMTIME *lpSystemTime,
		     LPFILETIME lpFileTime
		     );


WINBOOL
STDCALL
FileTimeToLocalFileTime(
			CONST FILETIME *lpFileTime,
			LPFILETIME lpLocalFileTime
			);


WINBOOL
STDCALL
LocalFileTimeToFileTime(
			CONST FILETIME *lpLocalFileTime,
			LPFILETIME lpFileTime
			);


WINBOOL
STDCALL
FileTimeToSystemTime(
		     CONST FILETIME *lpFileTime,
		     LPSYSTEMTIME lpSystemTime
		     );


LONG
STDCALL
CompareFileTime(
		CONST FILETIME *lpFileTime1,
		CONST FILETIME *lpFileTime2
		);


WINBOOL
STDCALL
FileTimeToDosDateTime(
		      CONST FILETIME *lpFileTime,
		      LPWORD lpFatDate,
		      LPWORD lpFatTime
		      );


WINBOOL
STDCALL
DosDateTimeToFileTime(
		      WORD wFatDate,
		      WORD wFatTime,
		      LPFILETIME lpFileTime
		      );


DWORD
STDCALL
GetTickCount(
	     VOID
	     );


WINBOOL
STDCALL
SetSystemTimeAdjustment(
			DWORD dwTimeAdjustment,
			WINBOOL  bTimeAdjustmentDisabled
			);


WINBOOL
STDCALL
GetSystemTimeAdjustment(
			PDWORD lpTimeAdjustment,
			PDWORD lpTimeIncrement,
			PWINBOOL  lpTimeAdjustmentDisabled
			);


WINBOOL
STDCALL
CreatePipe(
	   PHANDLE hReadPipe,
	   PHANDLE hWritePipe,
	   LPSECURITY_ATTRIBUTES lpPipeAttributes,
	   DWORD nSize
	   );


WINBOOL
STDCALL
ConnectNamedPipe(
		 HANDLE hNamedPipe,
		 LPOVERLAPPED lpOverlapped
		 );


WINBOOL
STDCALL
DisconnectNamedPipe(
		    HANDLE hNamedPipe
		    );


WINBOOL
STDCALL
SetNamedPipeHandleState(
			HANDLE hNamedPipe,
			LPDWORD lpMode,
			LPDWORD lpMaxCollectionCount,
			LPDWORD lpCollectDataTimeout
			);


WINBOOL
STDCALL
GetNamedPipeInfo(
		 HANDLE hNamedPipe,
		 LPDWORD lpFlags,
		 LPDWORD lpOutBufferSize,
		 LPDWORD lpInBufferSize,
		 LPDWORD lpMaxInstances
		 );


WINBOOL
STDCALL
PeekNamedPipe(
	      HANDLE hNamedPipe,
	      LPVOID lpBuffer,
	      DWORD nBufferSize,
	      LPDWORD lpBytesRead,
	      LPDWORD lpTotalBytesAvail,
	      LPDWORD lpBytesLeftThisMessage
	      );


WINBOOL
STDCALL
TransactNamedPipe(
		  HANDLE hNamedPipe,
		  LPVOID lpInBuffer,
		  DWORD nInBufferSize,
		  LPVOID lpOutBuffer,
		  DWORD nOutBufferSize,
		  LPDWORD lpBytesRead,
		  LPOVERLAPPED lpOverlapped
		  );



WINBOOL
STDCALL
GetMailslotInfo(
		HANDLE hMailslot,
		LPDWORD lpMaxMessageSize,
		LPDWORD lpNextSize,
		LPDWORD lpMessageCount,
		LPDWORD lpReadTimeout
		);


WINBOOL
STDCALL
SetMailslotInfo(
		HANDLE hMailslot,
		DWORD lReadTimeout
		);


LPVOID
STDCALL
MapViewOfFile(
	      HANDLE hFileMappingObject,
	      DWORD dwDesiredAccess,
	      DWORD dwFileOffsetHigh,
	      DWORD dwFileOffsetLow,
	      DWORD dwNumberOfBytesToMap
	      );


WINBOOL
STDCALL
FlushViewOfFile(
		LPCVOID lpBaseAddress,
		DWORD dwNumberOfBytesToFlush
		);


WINBOOL
STDCALL
UnmapViewOfFile(
		LPVOID lpBaseAddress
		);

HFILE
STDCALL
OpenFile(
	 LPCSTR lpFileName,
	 LPOFSTRUCT lpReOpenBuff,
	 UINT uStyle
	 );


HFILE
STDCALL
_lopen(
       LPCSTR lpPathName,
       int iReadWrite
       );


HFILE
STDCALL
_lcreat(
	LPCSTR lpPathName,
	int  iAttribute
	);


UINT
STDCALL
_lread(
       HFILE hFile,
       LPVOID lpBuffer,
       UINT uBytes
       );


UINT
STDCALL
_lwrite(
	HFILE hFile,
	LPCSTR lpBuffer,
	UINT uBytes
	);


long
STDCALL
_hread(
       HFILE hFile,
       LPVOID lpBuffer,
       long lBytes
       );


long
STDCALL
_hwrite(
	HFILE hFile,
	LPCSTR lpBuffer,
	long lBytes
	);


HFILE
STDCALL
_lclose(
	HFILE hFile
	);


LONG
STDCALL
_llseek(
	HFILE hFile,
	LONG lOffset,
	int iOrigin
	);


DWORD
STDCALL
TlsAlloc(
	 VOID
	 );

LPVOID
STDCALL
TlsGetValue(
	    DWORD dwTlsIndex
	    );


WINBOOL
STDCALL
TlsSetValue(
	    DWORD dwTlsIndex,
	    LPVOID lpTlsValue
	    );


WINBOOL
STDCALL
TlsFree(
	DWORD dwTlsIndex
	);

typedef VOID STDCALL_FUNC (*PFLS_CALLBACK_FUNCTION)(PVOID lpFlsData);

DWORD STDCALL FlsAlloc(PFLS_CALLBACK_FUNCTION lpCallback);

BOOL STDCALL FlsFree(DWORD dwFlsIndex);

PVOID STDCALL FlsGetValue(DWORD dwFlsIndex);

BOOL STDCALL FlsSetValue(DWORD dwFlsIndex, PVOID lpFlsData);

DWORD
STDCALL
SleepEx(
	DWORD dwMilliseconds,
	WINBOOL bAlertable
	);


DWORD
STDCALL
WaitForSingleObjectEx(
		      HANDLE hHandle,
		      DWORD dwMilliseconds,
		      WINBOOL bAlertable
		      );


DWORD
STDCALL
WaitForMultipleObjectsEx(
			 DWORD nCount,
			 CONST HANDLE *lpHandles,
			 WINBOOL bWaitAll,
			 DWORD dwMilliseconds,
			 WINBOOL bAlertable
			 );


WINBOOL
STDCALL
ReadFileEx(
	   HANDLE hFile,
	   LPVOID lpBuffer,
	   DWORD nNumberOfBytesToRead,
	   LPOVERLAPPED lpOverlapped,
	   LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	   );


WINBOOL
STDCALL
WriteFileEx(
	    HANDLE hFile,
	    LPCVOID lpBuffer,
	    DWORD nNumberOfBytesToWrite,
	    LPOVERLAPPED lpOverlapped,
	    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	    );


WINBOOL
STDCALL
BackupRead(
	   HANDLE hFile,
	   LPBYTE lpBuffer,
	   DWORD nNumberOfBytesToRead,
	   LPDWORD lpNumberOfBytesRead,
	   WINBOOL bAbort,
	   WINBOOL bProcessSecurity,
	   LPVOID *lpContext
	   );


WINBOOL
STDCALL
BackupSeek(
	   HANDLE hFile,
	   DWORD  dwLowBytesToSeek,
	   DWORD  dwHighBytesToSeek,
	   LPDWORD lpdwLowByteSeeked,
	   LPDWORD lpdwHighByteSeeked,
	   LPVOID *lpContext
	   );


WINBOOL
STDCALL
BackupWrite(
	    HANDLE hFile,
	    LPBYTE lpBuffer,
	    DWORD nNumberOfBytesToWrite,
	    LPDWORD lpNumberOfBytesWritten,
	    WINBOOL bAbort,
	    WINBOOL bProcessSecurity,
	    LPVOID *lpContext
	    );

WINBOOL
STDCALL
SetProcessShutdownParameters(
			     DWORD dwLevel,
			     DWORD dwFlags
			     );


WINBOOL
STDCALL
GetProcessShutdownParameters(
			     LPDWORD lpdwLevel,
			     LPDWORD lpdwFlags
			     );


VOID
STDCALL
SetFileApisToOEM( VOID );


VOID
STDCALL
SetFileApisToANSI( VOID );


WINBOOL
STDCALL
AreFileApisANSI( VOID );

WINBOOL
STDCALL
CloseEventLog (
	       HANDLE hEventLog
		);


WINBOOL
STDCALL
DeregisterEventSource (
		       HANDLE hEventLog
			);


WINBOOL
STDCALL
NotifyChangeEventLog (
		      HANDLE hEventLog,
		      HANDLE hEvent
		       );


WINBOOL
STDCALL
GetNumberOfEventLogRecords (
			    HANDLE hEventLog,
			    PDWORD NumberOfRecords
			     );


WINBOOL
STDCALL
GetOldestEventLogRecord (
			 HANDLE hEventLog,
			 PDWORD OldestRecord
			  );

WINBOOL
STDCALL
DuplicateToken(
	       HANDLE ExistingTokenHandle,
	       SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
	       PHANDLE DuplicateTokenHandle
	       );


WINBOOL
STDCALL
GetKernelObjectSecurity (
			 HANDLE Handle,
			 SECURITY_INFORMATION RequestedInformation,
			 PSECURITY_DESCRIPTOR pSecurityDescriptor,
			 DWORD nLength,
			 LPDWORD lpnLengthNeeded
			  );


WINBOOL
STDCALL
ImpersonateNamedPipeClient(
			   HANDLE hNamedPipe
			   );


WINBOOL
STDCALL
ImpersonateLoggedOnUser(
			   HANDLE hToken
			   );


WINBOOL
STDCALL
ImpersonateSelf(
		SECURITY_IMPERSONATION_LEVEL ImpersonationLevel
		);



WINBOOL
STDCALL
RevertToSelf (
	      VOID
	       );


WINBOOL
STDCALL
SetThreadToken (
		PHANDLE Thread,
		HANDLE Token
		 );


WINBOOL
STDCALL
AccessCheck (
	     PSECURITY_DESCRIPTOR pSecurityDescriptor,
	     HANDLE ClientToken,
	     DWORD DesiredAccess,
	     PGENERIC_MAPPING GenericMapping,
	     PPRIVILEGE_SET PrivilegeSet,
	     LPDWORD PrivilegeSetLength,
	     LPDWORD GrantedAccess,
	     LPBOOL AccessStatus
	      );



WINBOOL
STDCALL
OpenProcessToken (
		  HANDLE ProcessHandle,
		  DWORD DesiredAccess,
		  PHANDLE TokenHandle
		   );



WINBOOL
STDCALL
OpenThreadToken (
		 HANDLE ThreadHandle,
		 DWORD DesiredAccess,
		 WINBOOL OpenAsSelf,
		 PHANDLE TokenHandle
		  );



WINBOOL
STDCALL
GetTokenInformation (
		     HANDLE TokenHandle,
		     TOKEN_INFORMATION_CLASS TokenInformationClass,
		     LPVOID TokenInformation,
		     DWORD TokenInformationLength,
		     PDWORD ReturnLength
		      );



WINBOOL
STDCALL
SetTokenInformation (
		     HANDLE TokenHandle,
		     TOKEN_INFORMATION_CLASS TokenInformationClass,
		     LPVOID TokenInformation,
		     DWORD TokenInformationLength
		      );



WINBOOL
STDCALL
AdjustTokenPrivileges (
		       HANDLE TokenHandle,
		       WINBOOL DisableAllPrivileges,
		       PTOKEN_PRIVILEGES NewState,
		       DWORD BufferLength,
		       PTOKEN_PRIVILEGES PreviousState,
		       PDWORD ReturnLength
			);



WINBOOL
STDCALL
AdjustTokenGroups (
		   HANDLE TokenHandle,
		   WINBOOL ResetToDefault,
		   PTOKEN_GROUPS NewState,
		   DWORD BufferLength,
		   PTOKEN_GROUPS PreviousState,
		   PDWORD ReturnLength
		    );



WINBOOL
STDCALL
PrivilegeCheck (
		HANDLE ClientToken,
		PPRIVILEGE_SET RequiredPrivileges,
		LPBOOL pfResult
		 );



WINBOOL
STDCALL
IsValidSid (
	    PSID pSid
	     );



WINBOOL
STDCALL
EqualSid (
	  PSID pSid1,
	  PSID pSid2
	   );



WINBOOL
STDCALL
EqualPrefixSid (
		PSID pSid1,
		PSID pSid2
		 );



DWORD
STDCALL
GetSidLengthRequired (
		      UCHAR nSubAuthorityCount
		       );



WINBOOL
STDCALL
AllocateAndInitializeSid (
			  PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
			  BYTE nSubAuthorityCount,
			  DWORD nSubAuthority0,
			  DWORD nSubAuthority1,
			  DWORD nSubAuthority2,
			  DWORD nSubAuthority3,
			  DWORD nSubAuthority4,
			  DWORD nSubAuthority5,
			  DWORD nSubAuthority6,
			  DWORD nSubAuthority7,
			  PSID *pSid
			   );


PVOID
STDCALL
FreeSid(
	PSID pSid
	);


WINBOOL
STDCALL
InitializeSid (
	       PSID Sid,
	       PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
	       BYTE nSubAuthorityCount
		);



PSID_IDENTIFIER_AUTHORITY
STDCALL
GetSidIdentifierAuthority (
			   PSID pSid
			    );



PDWORD
STDCALL
GetSidSubAuthority (
		    PSID pSid,
		    DWORD nSubAuthority
		     );



PUCHAR
STDCALL
GetSidSubAuthorityCount (
			 PSID pSid
			  );



DWORD
STDCALL
GetLengthSid (
	      PSID pSid
	       );



WINBOOL
STDCALL
CopySid (
	 DWORD nDestinationSidLength,
	 PSID pDestinationSid,
	 PSID pSourceSid
	  );



WINBOOL
STDCALL
AreAllAccessesGranted (
		       DWORD GrantedAccess,
		       DWORD DesiredAccess
			);



WINBOOL
STDCALL
AreAnyAccessesGranted (
		       DWORD GrantedAccess,
		       DWORD DesiredAccess
			);



VOID
STDCALL
MapGenericMask (
		PDWORD AccessMask,
		PGENERIC_MAPPING GenericMapping
		 );



WINBOOL
STDCALL
IsValidAcl (
	    PACL pAcl
	     );



WINBOOL
STDCALL
InitializeAcl (
	       PACL pAcl,
	       DWORD nAclLength,
	       DWORD dwAclRevision
		);



WINBOOL
STDCALL
GetAclInformation (
		   PACL pAcl,
		   LPVOID pAclInformation,
		   DWORD nAclInformationLength,
		   ACL_INFORMATION_CLASS dwAclInformationClass
		    );



WINBOOL
STDCALL
SetAclInformation (
		   PACL pAcl,
		   LPVOID pAclInformation,
		   DWORD nAclInformationLength,
		   ACL_INFORMATION_CLASS dwAclInformationClass
		    );



WINBOOL
STDCALL
AddAce (
	PACL pAcl,
	DWORD dwAceRevision,
	DWORD dwStartingAceIndex,
	LPVOID pAceList,
	DWORD nAceListLength
	 );



WINBOOL
STDCALL
DeleteAce (
	   PACL pAcl,
	   DWORD dwAceIndex
	    );



WINBOOL
STDCALL
GetAce (
	PACL pAcl,
	DWORD dwAceIndex,
	LPVOID *pAce
	 );



WINBOOL
STDCALL
AddAccessAllowedAce (
		     PACL pAcl,
		     DWORD dwAceRevision,
		     DWORD AccessMask,
		     PSID pSid
		      );



WINBOOL
STDCALL
AddAccessDeniedAce (
		    PACL pAcl,
		    DWORD dwAceRevision,
		    DWORD AccessMask,
		    PSID pSid
		     );



WINBOOL
STDCALL
AddAuditAccessAce(
		  PACL pAcl,
		  DWORD dwAceRevision,
		  DWORD dwAccessMask,
		  PSID pSid,
		  WINBOOL bAuditSuccess,
		  WINBOOL bAuditFailure
		  );



WINBOOL
STDCALL
FindFirstFreeAce (
		  PACL pAcl,
		  LPVOID *pAce
		   );



WINBOOL
STDCALL
InitializeSecurityDescriptor (
			      PSECURITY_DESCRIPTOR pSecurityDescriptor,
			      DWORD dwRevision
			       );



WINBOOL
STDCALL
IsValidSecurityDescriptor (
			   PSECURITY_DESCRIPTOR pSecurityDescriptor
			    );



DWORD
STDCALL
GetSecurityDescriptorLength (
			     PSECURITY_DESCRIPTOR pSecurityDescriptor
			      );



WINBOOL
STDCALL
GetSecurityDescriptorControl (
			      PSECURITY_DESCRIPTOR pSecurityDescriptor,
			      PSECURITY_DESCRIPTOR_CONTROL pControl,
			      LPDWORD lpdwRevision
			       );



WINBOOL
STDCALL
SetSecurityDescriptorDacl (
			   PSECURITY_DESCRIPTOR pSecurityDescriptor,
			   WINBOOL bDaclPresent,
			   PACL pDacl,
			   WINBOOL bDaclDefaulted
			    );



WINBOOL
STDCALL
GetSecurityDescriptorDacl (
			   PSECURITY_DESCRIPTOR pSecurityDescriptor,
			   LPBOOL lpbDaclPresent,
			   PACL *pDacl,
			   LPBOOL lpbDaclDefaulted
			    );



WINBOOL
STDCALL
SetSecurityDescriptorSacl (
			   PSECURITY_DESCRIPTOR pSecurityDescriptor,
			   WINBOOL bSaclPresent,
			   PACL pSacl,
			   WINBOOL bSaclDefaulted
			    );



WINBOOL
STDCALL
GetSecurityDescriptorSacl (
			   PSECURITY_DESCRIPTOR pSecurityDescriptor,
			   LPBOOL lpbSaclPresent,
			   PACL *pSacl,
			   LPBOOL lpbSaclDefaulted
			    );



WINBOOL
STDCALL
SetSecurityDescriptorOwner (
			    PSECURITY_DESCRIPTOR pSecurityDescriptor,
			    PSID pOwner,
			    WINBOOL bOwnerDefaulted
			     );



WINBOOL
STDCALL
GetSecurityDescriptorOwner (
			    PSECURITY_DESCRIPTOR pSecurityDescriptor,
			    PSID *pOwner,
			    LPBOOL lpbOwnerDefaulted
			     );



WINBOOL
STDCALL
SetSecurityDescriptorGroup (
			    PSECURITY_DESCRIPTOR pSecurityDescriptor,
			    PSID pGroup,
			    WINBOOL bGroupDefaulted
			     );



WINBOOL
STDCALL
GetSecurityDescriptorGroup (
			    PSECURITY_DESCRIPTOR pSecurityDescriptor,
			    PSID *pGroup,
			    LPBOOL lpbGroupDefaulted
			     );



WINBOOL
STDCALL
CreatePrivateObjectSecurity (
			     PSECURITY_DESCRIPTOR ParentDescriptor,
			     PSECURITY_DESCRIPTOR CreatorDescriptor,
			     PSECURITY_DESCRIPTOR * NewDescriptor,
			     WINBOOL IsDirectoryObject,
			     HANDLE Token,
			     PGENERIC_MAPPING GenericMapping
			      );



WINBOOL
STDCALL
SetPrivateObjectSecurity (
			  SECURITY_INFORMATION SecurityInformation,
			  PSECURITY_DESCRIPTOR ModificationDescriptor,
			  PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
			  PGENERIC_MAPPING GenericMapping,
			  HANDLE Token
			   );



WINBOOL
STDCALL
GetPrivateObjectSecurity (
			  PSECURITY_DESCRIPTOR ObjectDescriptor,
			  SECURITY_INFORMATION SecurityInformation,
			  PSECURITY_DESCRIPTOR ResultantDescriptor,
			  DWORD DescriptorLength,
			  PDWORD ReturnLength
			   );



WINBOOL
STDCALL
DestroyPrivateObjectSecurity (
			      PSECURITY_DESCRIPTOR * ObjectDescriptor
			       );



WINBOOL
STDCALL
MakeSelfRelativeSD (
		    PSECURITY_DESCRIPTOR pAbsoluteSecurityDescriptor,
		    PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor,
		    LPDWORD lpdwBufferLength
		     );



WINBOOL
STDCALL
MakeAbsoluteSD (
		PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor,
		PSECURITY_DESCRIPTOR pAbsoluteSecurityDescriptor,
		LPDWORD lpdwAbsoluteSecurityDescriptorSize,
		PACL pDacl,
		LPDWORD lpdwDaclSize,
		PACL pSacl,
		LPDWORD lpdwSaclSize,
		PSID pOwner,
		LPDWORD lpdwOwnerSize,
		PSID pPrimaryGroup,
		LPDWORD lpdwPrimaryGroupSize
		 );



WINBOOL
STDCALL
SetKernelObjectSecurity (
			 HANDLE Handle,
			 SECURITY_INFORMATION SecurityInformation,
			 PSECURITY_DESCRIPTOR SecurityDescriptor
			  );


WINBOOL
STDCALL
FindNextChangeNotification(
			   HANDLE hChangeHandle
			   );


WINBOOL
STDCALL
FindCloseChangeNotification(
			    HANDLE hChangeHandle
			    );


WINBOOL
STDCALL
VirtualLock(
	    LPVOID lpAddress,
	    DWORD dwSize
	    );


WINBOOL
STDCALL
VirtualUnlock(
	      LPVOID lpAddress,
	      DWORD dwSize
	      );


LPVOID
STDCALL
MapViewOfFileEx(
		HANDLE hFileMappingObject,
		DWORD dwDesiredAccess,
		DWORD dwFileOffsetHigh,
		DWORD dwFileOffsetLow,
		DWORD dwNumberOfBytesToMap,
		LPVOID lpBaseAddress
		);


WINBOOL
STDCALL
SetPriorityClass(
		 HANDLE hProcess,
		 DWORD dwPriorityClass
		 );


DWORD
STDCALL
GetPriorityClass(
		 HANDLE hProcess
		 );


WINBOOL
STDCALL
IsBadReadPtr(
	     CONST VOID *lp,
	     UINT ucb
	     );


WINBOOL
STDCALL
IsBadWritePtr(
	      LPVOID lp,
	      UINT ucb
	      );


WINBOOL
STDCALL
IsBadHugeReadPtr(
		 CONST VOID *lp,
		 UINT ucb
		 );


WINBOOL
STDCALL
IsBadHugeWritePtr(
		  LPVOID lp,
		  UINT ucb
		  );


WINBOOL
STDCALL
IsBadCodePtr(
	     FARPROC lpfn
	     );

WINBOOL
STDCALL
AllocateLocallyUniqueId(
			PLUID Luid
			);


WINBOOL
STDCALL
QueryPerformanceCounter(
			LARGE_INTEGER *lpPerformanceCount
			);


WINBOOL
STDCALL
QueryPerformanceFrequency(
			  LARGE_INTEGER *lpFrequency
			  );

VOID
STDCALL
MoveMemory (
	    PVOID Destination,
	    CONST VOID *Source,
	    DWORD Length
	     );

VOID
STDCALL
FillMemory (
	    PVOID Destination,
	    DWORD Length,
	    BYTE  Fill
	     );

VOID
STDCALL
ZeroMemory (
	    PVOID Destination,
	    DWORD Length
	     );

/* The memory functions don't seem to be defined in the libraries, so
   define macro versions as well.  */
#define MoveMemory(t, s, c) memmove ((t), (s), (c))
#define FillMemory(p, c, v) memset ((p), (v), (c))
#define ZeroMemory(p, c) memset ((p), 0, (c))

#ifdef WINNT351
WINBOOL
STDCALL
ActivateKeyboardLayout(
		       HKL hkl,
		       UINT Flags);
#else
HKL
STDCALL
ActivateKeyboardLayout(
		       HKL hkl,
		       UINT Flags);
#endif /* WIN95 */

 
int
STDCALL
ToUnicodeEx(
	    UINT wVirtKey,
	    UINT wScanCode,
	    PBYTE lpKeyState,
	    LPWSTR pwszBuff,
	    int cchBuff,
	    UINT wFlags,
	    HKL dwhkl);

 
WINBOOL
STDCALL
UnloadKeyboardLayout(
		     HKL hkl);

 
UINT
STDCALL
GetKeyboardLayoutList(
		      int nBuff,
		      HKL *lpList);

 
HKL
STDCALL
GetKeyboardLayout(
		  DWORD dwLayout
		  );

 
HDESK
STDCALL
OpenInputDesktop(
		 DWORD dwFlags,
		 WINBOOL fInherit,
		 DWORD dwDesiredAccess);

WINBOOL
STDCALL
EnumDesktopWindows(
		   HDESK hDesktop,
		   ENUMWINDOWSPROC lpfn,
		   LPARAM lParam);

 
WINBOOL
STDCALL
SwitchDesktop(
	      HDESK hDesktop);

 
WINBOOL
STDCALL
SetThreadDesktop(
		 HDESK hDesktop);

 
WINBOOL
STDCALL
CloseDesktop(
	     HDESK hDesktop);

 
HDESK
STDCALL
GetThreadDesktop(
		 DWORD dwThreadId);

 
WINBOOL
STDCALL
CloseWindowStation(
		   HWINSTA hWinSta);

 
WINBOOL
STDCALL
SetProcessWindowStation(
			HWINSTA hWinSta);

 
HWINSTA
STDCALL
GetProcessWindowStation(
			VOID);

 
WINBOOL
STDCALL
SetUserObjectSecurity(
		      HANDLE hObj,
		      PSECURITY_INFORMATION pSIRequested,
		      PSECURITY_DESCRIPTOR pSID);

 
WINBOOL
STDCALL
GetUserObjectSecurity(
		      HANDLE hObj,
		      PSECURITY_INFORMATION pSIRequested,
		      PSECURITY_DESCRIPTOR pSID,
		      DWORD nLength,
		      LPDWORD lpnLengthNeeded);

 
WINBOOL
STDCALL
TranslateMessage(
		 CONST MSG *lpMsg);

WINBOOL
STDCALL
SetMessageQueue(
		int cMessagesMax);

WINBOOL
STDCALL
RegisterHotKey(
	       HWND hWnd ,
	       int anID,
	       UINT fsModifiers,
	       UINT vk);

 
WINBOOL
STDCALL
UnregisterHotKey(
		 HWND hWnd,
		 int anID);

 
WINBOOL
STDCALL
ExitWindowsEx(
	      UINT uFlags,
	      DWORD dwReserved);

 
WINBOOL
STDCALL
SwapMouseButton(
		WINBOOL fSwap);

 
DWORD
STDCALL
GetMessagePos(
	      VOID);

 
LONG
STDCALL
GetMessageTime(
	       VOID);

 
LPARAM
STDCALL
GetMessageExtraInfo(
		    VOID);

 
LPARAM
STDCALL
SetMessageExtraInfo(
		    LPARAM lParam);

 
long  
STDCALL  
BroadcastSystemMessage(
		       DWORD, 
		       LPDWORD, 
		       UINT, 
		       WPARAM, 
		       LPARAM);

WINBOOL
STDCALL
AttachThreadInput(
		  DWORD idAttach,
		  DWORD idAttachTo,
		  WINBOOL fAttach);

 
WINBOOL
STDCALL
ReplyMessage(
	     LRESULT lResult);

 
WINBOOL
STDCALL
WaitMessage(
	    VOID);

 
DWORD
STDCALL
WaitForInputIdle(
		 HANDLE hProcess,
		 DWORD dwMilliseconds);

 
VOID
STDCALL
PostQuitMessage(
		int nExitCode);

WINBOOL
STDCALL
InSendMessage(
	      VOID);

 
UINT
STDCALL
GetDoubleClickTime(
		   VOID);

 
WINBOOL
STDCALL
SetDoubleClickTime(
		   UINT);

 
WINBOOL
STDCALL
IsWindow(
	 HWND hWnd);

 
WINBOOL
STDCALL
IsMenu(
       HMENU hMenu);

 
WINBOOL
STDCALL
IsChild(
	HWND hWndParent,
	HWND hWnd);

 
WINBOOL
STDCALL
DestroyWindow(
	      HWND hWnd);

 
WINBOOL
STDCALL
ShowWindow(
	   HWND hWnd,
	   int nCmdShow);

 
WINBOOL
STDCALL
ShowWindowAsync(
		HWND hWnd,
		int nCmdShow);

 
WINBOOL
STDCALL
FlashWindow(
	    HWND hWnd,
	    WINBOOL bInvert);

 
WINBOOL
STDCALL
ShowOwnedPopups(
		HWND hWnd,
		WINBOOL fShow);

 
WINBOOL
STDCALL
OpenIcon(
	 HWND hWnd);

 
WINBOOL
STDCALL
CloseWindow(
	    HWND hWnd);

 
WINBOOL
STDCALL
MoveWindow(
	   HWND hWnd,
	   int X,
	   int Y,
	   int nWidth,
	   int nHeight,
	   WINBOOL bRepaint);

 
WINBOOL
STDCALL
SetWindowPos(
	     HWND hWnd,
	     HWND hWndInsertAfter ,
	     int X,
	     int Y,
	     int cx,
	     int cy,
	     UINT uFlags);

 
WINBOOL
STDCALL
GetWindowPlacement(
		   HWND hWnd,
		   WINDOWPLACEMENT *lpwndpl);

 
WINBOOL
STDCALL
SetWindowPlacement(
		   HWND hWnd,
		   CONST WINDOWPLACEMENT *lpwndpl);

 
HDWP
STDCALL
BeginDeferWindowPos(
		    int nNumWindows);

 
HDWP
STDCALL
DeferWindowPos(
	       HDWP hWinPosInfo,
	       HWND hWnd,
	       HWND hWndInsertAfter,
	       int x,
	       int y,
	       int cx,
	       int cy,
	       UINT uFlags);

 
WINBOOL
STDCALL
EndDeferWindowPos(
		  HDWP hWinPosInfo);

 
WINBOOL
STDCALL
IsWindowVisible(
		HWND hWnd);

 
WINBOOL
STDCALL
IsIconic(
	 HWND hWnd);

 
WINBOOL
STDCALL
AnyPopup(
	 VOID);

 
WINBOOL
STDCALL
BringWindowToTop(
		 HWND hWnd);

 
WINBOOL
STDCALL
IsZoomed(
	 HWND hWnd);

 
WINBOOL
STDCALL
EndDialog(
	  HWND hDlg,
	  int nResult);

 
HWND
STDCALL
GetDlgItem(
	   HWND hDlg,
	   int nIDDlgItem);

 
WINBOOL
STDCALL
SetDlgItemInt(
	      HWND hDlg,
	      int nIDDlgItem,
	      UINT uValue,
	      WINBOOL bSigned);

 
UINT
STDCALL
GetDlgItemInt(
	      HWND hDlg,
	      int nIDDlgItem,
	      WINBOOL *lpTranslated,
	      WINBOOL bSigned);

 
WINBOOL
STDCALL
CheckDlgButton(
	       HWND hDlg,
	       int nIDButton,
	       UINT uCheck);

 
WINBOOL
STDCALL
CheckRadioButton(
		 HWND hDlg,
		 int nIDFirstButton,
		 int nIDLastButton,
		 int nIDCheckButton);

 
UINT
STDCALL
IsDlgButtonChecked(
		   HWND hDlg,
		   int nIDButton);

 
HWND
STDCALL
GetNextDlgGroupItem(
		    HWND hDlg,
		    HWND hCtl,
		    WINBOOL bPrevious);

 
HWND
STDCALL
GetNextDlgTabItem(
		  HWND hDlg,
		  HWND hCtl,
		  WINBOOL bPrevious);

 
int
STDCALL
GetDlgCtrlID(
	     HWND hWnd);

 
LONG
STDCALL
GetDialogBaseUnits(VOID);

WINBOOL
STDCALL
OpenClipboard(
	      HWND hWndNewOwner);

 
WINBOOL
STDCALL
CloseClipboard(
	       VOID);

 
HWND
STDCALL
GetClipboardOwner(
		  VOID);

 
HWND
STDCALL
SetClipboardViewer(
		   HWND hWndNewViewer);

 
HWND
STDCALL
GetClipboardViewer(
		   VOID);

 
WINBOOL
STDCALL
ChangeClipboardChain(
		     HWND hWndRemove,
		     HWND hWndNewNext);

 
HANDLE
STDCALL
SetClipboardData(
		 UINT uFormat,
		 HANDLE hMem);

 
HANDLE
STDCALL
GetClipboardData(
		 UINT uFormat);

 
 
int
STDCALL
CountClipboardFormats(
		      VOID);

 
UINT
STDCALL
EnumClipboardFormats(
		     UINT format);

 
WINBOOL
STDCALL
EmptyClipboard(
	       VOID);

 
WINBOOL
STDCALL
IsClipboardFormatAvailable(
			   UINT format);

 
int
STDCALL
GetPriorityClipboardFormat(
			   UINT *paFormatPriorityList,
			   int cFormats);

 
HWND
STDCALL
GetOpenClipboardWindow(
		       VOID);

 
/* Despite the A these are ASCII functions! */
LPSTR
STDCALL
CharNextExA(
	    WORD CodePage,
	    LPCSTR lpCurrentChar,
	    DWORD dwFlags);

 
LPSTR
STDCALL
CharPrevExA(
	    WORD CodePage,
	    LPCSTR lpStart,
	    LPCSTR lpCurrentChar,
	    DWORD dwFlags);

 
HWND
STDCALL
SetFocus(
	 HWND hWnd);

 
HWND
STDCALL
GetActiveWindow(
		VOID);

 
HWND
STDCALL
GetFocus(
	 VOID);

 
UINT
STDCALL
GetKBCodePage(
	      VOID);

 
SHORT
STDCALL
GetKeyState(
	    int nVirtKey);

 
SHORT
STDCALL
GetAsyncKeyState(
		 int vKey);

 
WINBOOL
STDCALL
GetKeyboardState(
		 PBYTE lpKeyState);

 
WINBOOL
STDCALL
SetKeyboardState(
		 LPBYTE lpKeyState);

 
int
STDCALL
GetKeyboardType(
		int nTypeFlag);

 
int
STDCALL
ToAscii(
	UINT uVirtKey,
	UINT uScanCode,
	PBYTE lpKeyState,
	LPWORD lpChar,
	UINT uFlags);

 
int
STDCALL
ToAsciiEx(
	  UINT uVirtKey,
	  UINT uScanCode,
	  PBYTE lpKeyState,
	  LPWORD lpChar,
	  UINT uFlags,
	  HKL dwhkl);

 
int
STDCALL
ToUnicode(UINT wVirtKey,
	  UINT wScanCode,
	  PBYTE lpKeyState,
	  LPWSTR pwszBuff,
	  int cchBuff,
	  UINT wFlags);

 
DWORD
STDCALL
OemKeyScan(
	   WORD wOemChar);

 
VOID
STDCALL
keybd_event(
	    BYTE bVk,
	    BYTE bScan,
	    DWORD dwFlags,
	    DWORD dwExtraInfo);

 
VOID
STDCALL
mouse_event(
	    DWORD dwFlags,
	    DWORD dx,
	    DWORD dy,
	    DWORD cButtons,
	    DWORD dwExtraInfo);

WINBOOL
STDCALL
GetInputState(
	      VOID);

 
DWORD
STDCALL
GetQueueStatus(
	       UINT flags);

 
HWND
STDCALL
GetCapture(
	   VOID);

 
HWND
STDCALL
SetCapture(
	   HWND hWnd);

 
WINBOOL
STDCALL
ReleaseCapture(
	       VOID);

 
DWORD
STDCALL
MsgWaitForMultipleObjects(
			  DWORD nCount,
			  LPHANDLE pHandles,
			  WINBOOL fWaitAll,
			  DWORD dwMilliseconds,
			  DWORD dwWakeMask);

 
UINT
STDCALL
SetTimer(
	 HWND hWnd ,
	 UINT nIDEvent,
	 UINT uElapse,
	 TIMERPROC lpTimerFunc);


WINBOOL
STDCALL
SetWaitableTimer(HANDLE hTimer,
		 const LARGE_INTEGER *pDueTime,
		 LONG lPeriod,
		 PTIMERAPCROUTINE pfnCompletionRoutine,
		 LPVOID lpArgToCompletionRoutine,
		 WINBOOL fResume);


WINBOOL
STDCALL
KillTimer(
	  HWND hWnd,
	  UINT uIDEvent);

 
WINBOOL
STDCALL
IsWindowUnicode(
		HWND hWnd);

 
WINBOOL
STDCALL
EnableWindow(
	     HWND hWnd,
	     WINBOOL bEnable);

 
WINBOOL
STDCALL
IsWindowEnabled(
		HWND hWnd);

 
WINBOOL
STDCALL
DestroyAcceleratorTable(
			HACCEL hAccel);

int
STDCALL
GetSystemMetrics(
		 int nIndex);

HMENU
STDCALL
GetMenu(
	HWND hWnd);

 
WINBOOL
STDCALL
SetMenu(
	HWND hWnd,
	HMENU hMenu);

 
WINBOOL
STDCALL
HiliteMenuItem(
	       HWND hWnd,
	       HMENU hMenu,
	       UINT uIDHiliteItem,
	       UINT uHilite);

 
UINT
STDCALL
GetMenuState(
	     HMENU hMenu,
	     UINT uId,
	     UINT uFlags);

 
WINBOOL
STDCALL
DrawMenuBar(
	    HWND hWnd);

 
HMENU
STDCALL
GetSystemMenu(
	      HWND hWnd,
	      WINBOOL bRevert);

 
HMENU
STDCALL
CreateMenu(
	   VOID);

 
HMENU
STDCALL
CreatePopupMenu(
		VOID);

 
WINBOOL
STDCALL
DestroyMenu(
	    HMENU hMenu);

 
DWORD
STDCALL
CheckMenuItem(
	      HMENU hMenu,
	      UINT uIDCheckItem,
	      UINT uCheck);

 
UINT
STDCALL
EnableMenuItem(
	       HMENU hMenu,
	       UINT uIDEnableItem,
	       UINT uEnable);

 
HMENU
STDCALL
GetSubMenu(
	   HMENU hMenu,
	   int nPos);

 
UINT
STDCALL
GetMenuItemID(
	      HMENU hMenu,
	      int nPos);

 
int
STDCALL
GetMenuItemCount(
		 HMENU hMenu);

WINBOOL
STDCALL RemoveMenu(
		   HMENU hMenu,
		   UINT uPosition,
		   UINT uFlags);

 
WINBOOL
STDCALL
DeleteMenu(
	   HMENU hMenu,
	   UINT uPosition,
	   UINT uFlags);

 
WINBOOL
STDCALL
SetMenuItemBitmaps(
		   HMENU hMenu,
		   UINT uPosition,
		   UINT uFlags,
		   HBITMAP hBitmapUnchecked,
		   HBITMAP hBitmapChecked);

 
LONG
STDCALL
GetMenuCheckMarkDimensions(
			   VOID);

 
WINBOOL
STDCALL
TrackPopupMenu(
	       HMENU hMenu,
	       UINT uFlags,
	       int x,
	       int y,
	       int nReserved,
	       HWND hWnd,
	       CONST RECT *prcRect);

UINT
STDCALL
GetMenuDefaultItem(
		   HMENU hMenu, 
		   UINT fByPos, 
		   UINT gmdiFlags);

WINBOOL
STDCALL
SetMenuDefaultItem(
		   HMENU hMenu, 
		   UINT uItem, 
		   UINT fByPos);

WINBOOL
STDCALL
GetMenuItemRect(HWND hWnd, 
		HMENU hMenu, 
		UINT uItem, 
		LPRECT lprcItem);

int
STDCALL
MenuItemFromPoint(HWND hWnd, 
		  HMENU hMenu, 
		  POINT ptScreen);

 
DWORD
STDCALL
DragObject(HWND, HWND, UINT, DWORD, HCURSOR);

 
WINBOOL
STDCALL
DragDetect(HWND hwnd, 
	   POINT pt);

 
WINBOOL
STDCALL
DrawIcon(
	 HDC hDC,
	 int X,
	 int Y,
	 HICON hIcon);

WINBOOL
STDCALL
UpdateWindow(
	     HWND hWnd);

 
HWND
STDCALL
SetActiveWindow(
		HWND hWnd);

 
HWND
STDCALL
GetForegroundWindow(
		    VOID);

WINBOOL
STDCALL
PaintDesktop(HDC hdc);

 
WINBOOL
STDCALL
SetForegroundWindow(
		    HWND hWnd);

 
HWND
STDCALL
WindowFromDC(
	     HDC hDC);

 
HDC
STDCALL
GetDC(
      HWND hWnd);

 
HDC
STDCALL
GetDCEx(
	HWND hWnd ,
	HRGN hrgnClip,
	DWORD flags);

 
HDC
STDCALL
GetWindowDC(
	    HWND hWnd);

 
int
STDCALL
ReleaseDC(
	  HWND hWnd,
	  HDC hDC);

 
HDC
STDCALL
BeginPaint(
	   HWND hWnd,
	   LPPAINTSTRUCT lpPaint);

 
WINBOOL
STDCALL
EndPaint(
	 HWND hWnd,
	 CONST PAINTSTRUCT *lpPaint);

 
WINBOOL
STDCALL
GetUpdateRect(
	      HWND hWnd,
	      LPRECT lpRect,
	      WINBOOL bErase);

 
int
STDCALL
GetUpdateRgn(
	     HWND hWnd,
	     HRGN hRgn,
	     WINBOOL bErase);

 
int
STDCALL
SetWindowRgn(
	     HWND hWnd,
	     HRGN hRgn,
	     WINBOOL bRedraw);

 
int
STDCALL
GetWindowRgn(
	     HWND hWnd,
	     HRGN hRgn);

 
int
STDCALL
ExcludeUpdateRgn(
		 HDC hDC,
		 HWND hWnd);

 
WINBOOL
STDCALL
InvalidateRect(
	       HWND hWnd ,
	       CONST RECT *lpRect,
	       WINBOOL bErase);

 
WINBOOL
STDCALL
ValidateRect(
	     HWND hWnd ,
	     CONST RECT *lpRect);

 
WINBOOL
STDCALL
InvalidateRgn(
	      HWND hWnd,
	      HRGN hRgn,
	      WINBOOL bErase);

 
WINBOOL
STDCALL
ValidateRgn(
	    HWND hWnd,
	    HRGN hRgn);

 
WINBOOL
STDCALL
RedrawWindow(
	     HWND hWnd,
	     CONST RECT *lprcUpdate,
	     HRGN hrgnUpdate,
	     UINT flags);

 
WINBOOL
STDCALL
LockWindowUpdate(
		 HWND hWndLock);

 
WINBOOL
STDCALL
ScrollWindow(
	     HWND hWnd,
	     int XAmount,
	     int YAmount,
	     CONST RECT *lpRect,
	     CONST RECT *lpClipRect);

 
WINBOOL
STDCALL
ScrollDC(
	 HDC hDC,
	 int dx,
	 int dy,
	 CONST RECT *lprcScroll,
	 CONST RECT *lprcClip ,
	 HRGN hrgnUpdate,
	 LPRECT lprcUpdate);

 
int
STDCALL
ScrollWindowEx(
	       HWND hWnd,
	       int dx,
	       int dy,
	       CONST RECT *prcScroll,
	       CONST RECT *prcClip ,
	       HRGN hrgnUpdate,
	       LPRECT prcUpdate,
	       UINT flags);

 
int
STDCALL
SetScrollPos(
	     HWND hWnd,
	     int nBar,
	     int nPos,
	     WINBOOL bRedraw);

 
int
STDCALL
GetScrollPos(
	     HWND hWnd,
	     int nBar);

 
WINBOOL
STDCALL
SetScrollRange(
	       HWND hWnd,
	       int nBar,
	       int nMinPos,
	       int nMaxPos,
	       WINBOOL bRedraw);

 
WINBOOL
STDCALL
GetScrollRange(
	       HWND hWnd,
	       int nBar,
	       LPINT lpMinPos,
	       LPINT lpMaxPos);

WINBOOL
STDCALL
ShowScrollBar(
	      HWND hWnd,
	      int wBar,
	      WINBOOL bShow);

WINBOOL
STDCALL
EnableScrollBar(
		HWND hWnd,
		UINT wSBflags,
		UINT wArrows);

BOOL
STDCALL
GetScrollBarInfo(
		 HWND hwnd,
		 LONG idObject,
		 PSCROLLBARINFO psbi);

WINBOOL
STDCALL
GetClientRect(
	      HWND hWnd,
	      LPRECT lpRect);

WINBOOL
STDCALL
GetWindowRect(
	      HWND hWnd,
	      LPRECT lpRect);

WINBOOL
STDCALL
AdjustWindowRect(
		 LPRECT lpRect,
		 DWORD dwStyle,
		 WINBOOL bMenu);

WINBOOL
STDCALL
AdjustWindowRectEx(
		   LPRECT lpRect,
		   DWORD dwStyle,
		   WINBOOL bMenu,
		   DWORD dwExStyle);

WINBOOL
STDCALL
SetWindowContextHelpId(
		       HWND hwnd,
		       DWORD dwContextHelpId);

DWORD
STDCALL
GetWindowContextHelpId(
		       HWND hwnd);

WINBOOL
STDCALL
SetMenuContextHelpId(
		     HMENU hmenu,
		     DWORD dwContextHelpId);

DWORD
STDCALL
GetMenuContextHelpId(
		     HMENU hmenu);

WINBOOL
STDCALL
MessageBeep(
	    UINT uType);

HCURSOR
STDCALL
CopyCursor(HCURSOR pcur);

int
STDCALL
ShowCursor(
	   WINBOOL bShow);

WINBOOL
STDCALL
SetCursorPos(
	     int X,
	     int Y);

 
HCURSOR
STDCALL
SetCursor(
	  HCURSOR hCursor);

 
WINBOOL
STDCALL
GetCursorPos(
	     LPPOINT lpPoint);

 
WINBOOL
STDCALL
ClipCursor(
	   CONST RECT *lpRect);

 
WINBOOL
STDCALL
GetClipCursor(
	      LPRECT lpRect);

 
HCURSOR
STDCALL
GetCursor(
	  VOID);

 
WINBOOL
STDCALL
CreateCaret(
	    HWND hWnd,
	    HBITMAP hBitmap ,
	    int nWidth,
	    int nHeight);

 
UINT
STDCALL
GetCaretBlinkTime(
		  VOID);

 
WINBOOL
STDCALL
SetCaretBlinkTime(
		  UINT uMSeconds);

 
WINBOOL
STDCALL
DestroyCaret(
	     VOID);

 
WINBOOL
STDCALL
HideCaret(
	  HWND hWnd);

 
WINBOOL
STDCALL
ShowCaret(
	  HWND hWnd);

 
WINBOOL
STDCALL
SetCaretPos(
	    int X,
	    int Y);

 
WINBOOL
STDCALL
GetCaretPos(
	    LPPOINT lpPoint);

 
WINBOOL
STDCALL
ClientToScreen(
	       HWND hWnd,
	       LPPOINT lpPoint);

 
WINBOOL
STDCALL
ScreenToClient(
	       HWND hWnd,
	       LPPOINT lpPoint);

 
int
STDCALL
MapWindowPoints(
		HWND hWndFrom,
		HWND hWndTo,
		LPPOINT lpPoints,
		UINT cPoints);

 
HWND
STDCALL
WindowFromPoint(
		POINT Point);

 
HWND
STDCALL
ChildWindowFromPoint(
		     HWND hWndParent,
		     POINT Point);

 
DWORD
STDCALL
GetSysColor(
	    int nIndex);

 
HBRUSH
STDCALL
GetSysColorBrush(
		 int nIndex);

 
WINBOOL
STDCALL
SetSysColors(
	     int cElements,
	     CONST INT * lpaElements,
	     CONST COLORREF * lpaRgbValues);

 
WINBOOL
STDCALL
DrawFocusRect(
	      HDC hDC,
	      CONST RECT * lprc);

 
int
STDCALL
FillRect(
	 HDC hDC,
	 CONST RECT *lprc,
	 HBRUSH hbr);

 
int
STDCALL
FrameRect(
	  HDC hDC,
	  CONST RECT *lprc,
	  HBRUSH hbr);

 
WINBOOL
STDCALL
InvertRect(
	   HDC hDC,
	   CONST RECT *lprc);

 
WINBOOL
STDCALL
SetRect(
	LPRECT lprc,
	int xLeft,
	int yTop,
	int xRight,
	int yBottom);

 
WINBOOL
STDCALL
SetRectEmpty(
	     LPRECT lprc);

 
WINBOOL
STDCALL
CopyRect(
	 LPRECT lprcDst,
	 CONST RECT *lprcSrc);

 
WINBOOL
STDCALL
InflateRect(
	    LPRECT lprc,
	    int dx,
	    int dy);

 
WINBOOL
STDCALL
IntersectRect(
	      LPRECT lprcDst,
	      CONST RECT *lprcSrc1,
	      CONST RECT *lprcSrc2);

 
WINBOOL
STDCALL
UnionRect(
	  LPRECT lprcDst,
	  CONST RECT *lprcSrc1,
	  CONST RECT *lprcSrc2);

 
WINBOOL
STDCALL
SubtractRect(
	     LPRECT lprcDst,
	     CONST RECT *lprcSrc1,
	     CONST RECT *lprcSrc2);

 
WINBOOL
STDCALL
OffsetRect(
	   LPRECT lprc,
	   int dx,
	   int dy);

 
WINBOOL
STDCALL
IsRectEmpty(
	    CONST RECT *lprc);

 
WINBOOL
STDCALL
EqualRect(
	  CONST RECT *lprc1,
	  CONST RECT *lprc2);

 
WINBOOL
STDCALL
PtInRect(
	 CONST RECT *lprc,
	 POINT pt);

 
WORD
STDCALL
GetWindowWord(
	      HWND hWnd,
	      int nIndex);

 
WORD
STDCALL
SetWindowWord(
	      HWND hWnd,
	      int nIndex,
	      WORD wNewWord);

 
WORD
STDCALL
GetClassWord(
	     HWND hWnd,
	     int nIndex);

 
WORD
STDCALL
SetClassWord(
	     HWND hWnd,
	     int nIndex,
	     WORD wNewWord);

HWND
STDCALL
GetDesktopWindow(
		 VOID);

 
HWND
STDCALL
GetShellWindow(VOID);


HWND
STDCALL
GetParent(
	  HWND hWnd);

 
HWND
STDCALL
SetParent(
	  HWND hWndChild,
	  HWND hWndNewParent);

 
WINBOOL
STDCALL
EnumChildWindows(
		 HWND hWndParent,
		 ENUMWINDOWSPROC lpEnumFunc,
		 LPARAM lParam);

 
WINBOOL
STDCALL
EnumWindows(
	    ENUMWINDOWSPROC lpEnumFunc,
	    LPARAM lParam);

 
WINBOOL
STDCALL
EnumThreadWindows(
		  DWORD dwThreadId,
		  ENUMWINDOWSPROC lpfn,
		  LPARAM lParam);

HWND
STDCALL
GetTopWindow(
	     HWND hWnd);

 
DWORD
STDCALL
GetWindowThreadProcessId(
			 HWND hWnd,
			 LPDWORD lpdwProcessId);


DWORD
STDCALL
GetGuiResources(
		   HANDLE hProcess,
		   DWORD uiFlags);


WINBOOL
STDCALL
GetGUIThreadInfo(
		   DWORD idThread,
		   LPGUITHREADINFO lpgui);

 
HWND
STDCALL
GetLastActivePopup(
		   HWND hWnd);

 
HWND
STDCALL
GetWindow(
	  HWND hWnd,
	  UINT uCmd);

WINBOOL
STDCALL
UnhookWindowsHook(
		  int nCode,
		  HOOKPROC pfnFilterProc);

WINBOOL
STDCALL
UnhookWindowsHookEx(
		    HHOOK hhk);

 
LRESULT
STDCALL
CallNextHookEx(
	       HHOOK hhk,
	       int nCode,
	       WPARAM wParam,
	       LPARAM lParam);

 
WINBOOL
STDCALL
CheckMenuRadioItem(HMENU, UINT, UINT, UINT, UINT);

HCURSOR
STDCALL
CreateCursor(
	     HINSTANCE hInst,
	     int xHotSpot,
	     int yHotSpot,
	     int nWidth,
	     int nHeight,
	     CONST VOID *pvANDPlane,
	     CONST VOID *pvXORPlane);

 
WINBOOL
STDCALL
DestroyCursor(
	      HCURSOR hCursor);

 
WINBOOL
STDCALL
SetSystemCursor(
		HCURSOR hcur,
		DWORD   anID);

 
HICON
STDCALL
CreateIcon(
	   HINSTANCE hInstance,
	   int nWidth,
	   int nHeight,
	   BYTE cPlanes,
	   BYTE cBitsPixel,
	   CONST BYTE *lpbANDbits,
	   CONST BYTE *lpbXORbits);

 
WINBOOL
STDCALL
DestroyIcon(
	    HICON hIcon);

 
int
STDCALL
LookupIconIdFromDirectory(
			  PBYTE presbits,
			  WINBOOL fIcon);

 
int
STDCALL
LookupIconIdFromDirectoryEx(
			    PBYTE presbits,
			    WINBOOL  fIcon,
			    int   cxDesired,
			    int   cyDesired,
			    UINT  Flags);

 
HICON
STDCALL
CreateIconFromResource(
		       PBYTE presbits,
		       DWORD dwResSize,
		       WINBOOL fIcon,
		       DWORD dwVer);

 
HICON
STDCALL
CreateIconFromResourceEx(
			 PBYTE presbits,
			 DWORD dwResSize,
			 WINBOOL  fIcon,
			 DWORD dwVer,
			 int   cxDesired,
			 int   cyDesired,
			 UINT  Flags);

 
HICON
STDCALL
CopyImage(
	  HANDLE,
	  UINT,
	  int,
	  int,
	  UINT);

 
HICON
STDCALL
CreateIconIndirect(
		   PICONINFO piconinfo);

 
HICON
STDCALL
CopyIcon(
	 HICON hIcon);

 
WINBOOL
STDCALL
GetIconInfo(
	    HICON hIcon,
	    PICONINFO piconinfo);

 
WINBOOL
STDCALL
MapDialogRect(
	      HWND hDlg,
	      LPRECT lpRect);

int
STDCALL
SetScrollInfo(HWND, int, LPCSCROLLINFO, WINBOOL);

WINBOOL
STDCALL
GetScrollInfo(HWND, int, LPSCROLLINFO);

WINBOOL
STDCALL
TranslateMDISysAccel(
		     HWND hWndClient,
		     LPMSG lpMsg);

 
UINT
STDCALL
ArrangeIconicWindows(
		     HWND hWnd);

WORD
STDCALL
TileWindows(HWND hwndParent, UINT wHow, CONST RECT * lpRect, UINT cKids, const HWND *lpKids);

WORD
STDCALL
CascadeWindows(HWND hwndParent, UINT wHow, CONST RECT * lpRect, UINT cKids,  const HWND *lpKids);

 
VOID
STDCALL
SetLastErrorEx(
	       DWORD dwErrCode,
	       DWORD dwType
	       );

 
VOID
STDCALL
SetDebugErrorLevel(
		   DWORD dwLevel
		   );

WINBOOL
STDCALL
DrawEdge(HDC hdc, LPRECT qrc, UINT edge, UINT grfFlags);

WINBOOL
STDCALL
DrawFrameControl(HDC, LPRECT, UINT, UINT);

WINBOOL
STDCALL
DrawCaption(
  HWND hwnd,
  HDC hdc,
  LPRECT lprc,
  UINT uFlags);

WINBOOL
STDCALL
DrawAnimatedRects(HWND hwnd, int idAni, CONST RECT * lprcFrom, CONST RECT * lprcTo);

WINBOOL
STDCALL
TrackPopupMenuEx(HMENU, UINT, int, int, HWND, LPTPMPARAMS);

HWND
STDCALL
ChildWindowFromPointEx(HWND, POINT, UINT);

WINBOOL
STDCALL
DrawIconEx(HDC hdc, int xLeft, int yTop,
	   HICON hIcon, int cxWidth, int cyWidth,
	   UINT istepIfAniCur, HBRUSH hbrFlickerFreeDraw, UINT diFlags);

WINBOOL
STDCALL
AnimatePalette(HPALETTE, UINT, UINT, CONST PALETTEENTRY *);

WINBOOL
STDCALL
Arc(HDC, int, int, int, int, int, int, int, int);

WINBOOL
STDCALL
BitBlt(HDC, int, int, int, int, HDC, int, int, DWORD);

WINBOOL
STDCALL
CancelDC(HDC);

WINBOOL
STDCALL
Chord(HDC, int, int, int, int, int, int, int, int);

HMETAFILE
STDCALL
CloseMetaFile(HDC);

int
STDCALL
CombineRgn(HRGN, HRGN, HRGN, int);

HBITMAP
STDCALL
CreateBitmap(int, int, UINT, UINT, CONST VOID *);

HBITMAP
STDCALL
CreateBitmapIndirect(CONST BITMAP *);

HBRUSH
STDCALL
CreateBrushIndirect(CONST LOGBRUSH *);

HBITMAP
STDCALL
CreateCompatibleBitmap(HDC, int, int);

HBITMAP
STDCALL
CreateDiscardableBitmap(HDC, int, int);

HDC
STDCALL
CreateCompatibleDC(HDC);

HBITMAP
STDCALL
CreateDIBitmap(HDC, CONST BITMAPINFOHEADER *, DWORD, CONST VOID *, CONST BITMAPINFO *, UINT);

HBRUSH
STDCALL
CreateDIBPatternBrush(HGLOBAL, UINT);

HBRUSH
STDCALL
CreateDIBPatternBrushPt(CONST VOID *, UINT);

HRGN
STDCALL
CreateEllipticRgn(int, int, int, int);

HRGN
STDCALL
CreateEllipticRgnIndirect(CONST RECT *);

HBRUSH
STDCALL
CreateHatchBrush(int, COLORREF);

HPALETTE
STDCALL
CreatePalette(CONST LOGPALETTE *);

HPEN
STDCALL
CreatePen(int, int, COLORREF);

HPEN
STDCALL
CreatePenIndirect(CONST LOGPEN *);

HRGN
STDCALL
CreatePolyPolygonRgn(CONST POINT *, CONST INT *, int, int);

HBRUSH
STDCALL
CreatePatternBrush(HBITMAP);

HRGN
STDCALL
CreateRectRgn(int, int, int, int);

HRGN
STDCALL
CreateRectRgnIndirect(CONST RECT *);

HRGN
STDCALL
CreateRoundRectRgn(int, int, int, int, int, int);

HBRUSH
STDCALL
CreateSolidBrush(COLORREF);

WINBOOL
STDCALL
DeleteDC(HDC);

WINBOOL
STDCALL
DeleteMetaFile(HMETAFILE);

WINBOOL
STDCALL
DeleteObject(HGDIOBJ);

int
STDCALL
DrawEscape(HDC, int, int, LPCSTR);

WINBOOL
STDCALL
Ellipse(HDC, int, int, int, int);

int
STDCALL
EnumObjects(HDC, int, ENUMOBJECTSPROC, LPARAM);

WINBOOL
STDCALL
EqualRgn(HRGN, HRGN);

int
STDCALL
Escape(HDC, int, int, LPCSTR, LPVOID);

int
STDCALL
ExtEscape(HDC, int, int, LPCSTR, int, LPSTR);

int
STDCALL
ExcludeClipRect(HDC, int, int, int, int);

HRGN
STDCALL
ExtCreateRegion(CONST XFORM *, DWORD, CONST RGNDATA *);

WINBOOL
STDCALL
ExtFloodFill(HDC, int, int, COLORREF, UINT);

WINBOOL
STDCALL
FillRgn(HDC, HRGN, HBRUSH);

WINBOOL
STDCALL
FloodFill(HDC, int, int, COLORREF);

WINBOOL
STDCALL
FrameRgn(HDC, HRGN, HBRUSH, int, int);

WINBOOL
STDCALL
GdiDllInitialize (HANDLE, DWORD, LPVOID);

VOID
STDCALL
GdiProcessSetup (VOID);

int
STDCALL
GetROP2(HDC);

WINBOOL
STDCALL
GetAspectRatioFilterEx(HDC, LPSIZE);

COLORREF
STDCALL
GetBkColor(HDC);

int
STDCALL
GetBkMode(HDC);

LONG
STDCALL
GetBitmapBits(HBITMAP, LONG, LPVOID);

WINBOOL
STDCALL
GetBitmapDimensionEx(HBITMAP, LPSIZE);

UINT
STDCALL
GetBoundsRect(HDC, LPRECT, UINT);

WINBOOL
STDCALL
GetBrushOrgEx(HDC, LPPOINT);

int
STDCALL
GetClipBox(HDC, LPRECT);

int
STDCALL
GetClipRgn(HDC, HRGN);

int
STDCALL
GetMetaRgn(HDC, HRGN);

HGDIOBJ
STDCALL
GetCurrentObject(HDC, UINT);

WINBOOL
STDCALL
GetCurrentPositionEx(HDC, LPPOINT);

int
STDCALL
GetDeviceCaps(HDC, int);

int
STDCALL
GetDIBits(HDC, HBITMAP, UINT, UINT, LPVOID, LPBITMAPINFO, UINT);

DWORD
STDCALL
GetFontData(HDC, DWORD, DWORD, LPVOID, DWORD);

int
STDCALL
GetGraphicsMode(HDC);

int
STDCALL
GetMapMode(HDC);

UINT
STDCALL
GetMetaFileBitsEx(HMETAFILE, UINT, LPVOID);

COLORREF
STDCALL
GetNearestColor(HDC, COLORREF);

UINT
STDCALL
GetNearestPaletteIndex(HPALETTE, COLORREF);

DWORD
STDCALL
GetObjectType(HGDIOBJ h);

UINT
STDCALL
GetPaletteEntries(HPALETTE, UINT, UINT, LPPALETTEENTRY);

COLORREF
STDCALL
GetPixel(HDC, int, int);

int
STDCALL
GetPixelFormat(HDC);

int
STDCALL
GetPolyFillMode(HDC);

WINBOOL
STDCALL
GetRasterizerCaps(LPRASTERIZER_STATUS, UINT);

DWORD
STDCALL
GetRegionData(HRGN, DWORD, LPRGNDATA);

int
STDCALL
GetRgnBox(HRGN, LPRECT);

HGDIOBJ
STDCALL
GetStockObject(int);

int
STDCALL
GetStretchBltMode(HDC);

UINT
STDCALL
GetSystemPaletteEntries(HDC, UINT, UINT, LPPALETTEENTRY);

UINT
STDCALL
GetSystemPaletteUse(HDC);

int
STDCALL
GetTextCharacterExtra(HDC);

UINT
STDCALL
GetTextAlign(HDC);

COLORREF
STDCALL
GetTextColor(HDC);

int
STDCALL
GetTextCharset(HDC hdc);

int
STDCALL
GetTextCharsetInfo(HDC hdc, LPFONTSIGNATURE lpSig, DWORD dwFlags);

WINBOOL
STDCALL
TranslateCharsetInfo( DWORD *lpSrc, LPCHARSETINFO lpCs, DWORD dwFlags);

DWORD
STDCALL
GetFontLanguageInfo( HDC );

WINBOOL
STDCALL
GetViewportExtEx(HDC, LPSIZE);

WINBOOL
STDCALL
GetViewportOrgEx(HDC, LPPOINT);

WINBOOL
STDCALL
GetWindowExtEx(HDC, LPSIZE);

WINBOOL
STDCALL
GetWindowOrgEx(HDC, LPPOINT);

int
STDCALL
IntersectClipRect(HDC, int, int, int, int);

WINBOOL
STDCALL
InvertRgn(HDC, HRGN);

WINBOOL
STDCALL
LineDDA(int, int, int, int, LINEDDAPROC, LPARAM);

WINBOOL
STDCALL
LineTo(HDC, int, int);

WINBOOL
STDCALL
MaskBlt(HDC, int, int, int, int,
	HDC, int, int, HBITMAP, int, int, DWORD);

WINBOOL
STDCALL
PlgBlt(HDC, CONST POINT *, HDC, int, int, int,
       int, HBITMAP, int, int);

int
STDCALL
OffsetClipRgn(HDC, int, int);

int
STDCALL
OffsetRgn(HRGN, int, int);

WINBOOL
STDCALL 
PatBlt(HDC, int, int, int, int, DWORD);

WINBOOL
STDCALL
Pie(HDC, int, int, int, int, int, int, int, int);

WINBOOL
STDCALL
PlayMetaFile(HDC, HMETAFILE);

WINBOOL
STDCALL
PaintRgn(HDC, HRGN);

WINBOOL
STDCALL
PolyPolygon(HDC, CONST POINT *, CONST INT *, int);

WINBOOL
STDCALL
PtInRegion(HRGN, int, int);

WINBOOL
STDCALL
PtVisible(HDC, int, int);

WINBOOL
STDCALL
RectInRegion(HRGN, CONST RECT *);

WINBOOL
STDCALL
RectVisible(HDC, CONST RECT *);

WINBOOL
STDCALL
Rectangle(HDC, int, int, int, int);

WINBOOL
STDCALL
RestoreDC(HDC, int);

UINT
STDCALL
RealizePalette(HDC);

WINBOOL
STDCALL
RoundRect(HDC, int, int, int, int, int, int);

WINBOOL
STDCALL
ResizePalette(HPALETTE, UINT);

int
STDCALL
SaveDC(HDC);

int
STDCALL
SelectClipRgn(HDC, HRGN);

int
STDCALL
ExtSelectClipRgn(HDC, HRGN, int);

int
STDCALL
SetMetaRgn(HDC);

HGDIOBJ
STDCALL
SelectObject(HDC, HGDIOBJ);

HPALETTE
STDCALL
SelectPalette(HDC, HPALETTE, WINBOOL);

COLORREF
STDCALL
SetBkColor(HDC, COLORREF);

int
STDCALL
SetBkMode(HDC, int);

LONG
STDCALL
SetBitmapBits(HBITMAP, DWORD, CONST VOID *);

UINT
STDCALL
SetBoundsRect(HDC, CONST RECT *, UINT);

int
STDCALL
SetDIBits(HDC, HBITMAP, UINT, UINT, CONST VOID *, CONST BITMAPINFO *, UINT);

int
STDCALL
SetDIBitsToDevice(HDC, int, int, DWORD, DWORD, int,
		  int, UINT, UINT, CONST VOID *, CONST BITMAPINFO *, UINT);

DWORD
STDCALL
SetMapperFlags(HDC, DWORD);

int
STDCALL
SetGraphicsMode(HDC hdc, int iMode);

int
STDCALL
SetMapMode(HDC, int);

HMETAFILE
STDCALL
SetMetaFileBitsEx(UINT, CONST BYTE *);

UINT
STDCALL
SetPaletteEntries(HPALETTE, UINT, UINT, CONST PALETTEENTRY *);

COLORREF
STDCALL
SetPixel(HDC, int, int, COLORREF);

WINBOOL
STDCALL
SetPixelV(HDC, int, int, COLORREF);

int
STDCALL
SetPolyFillMode(HDC, int);

WINBOOL
STDCALL
StretchBlt(HDC, int, int, int, int, HDC, int, int, int, int, DWORD);

WINBOOL
STDCALL
SetRectRgn(HRGN, int, int, int, int);
int
STDCALL
StretchDIBits(HDC, int, int, int, int, int, int, int, int, CONST
	      VOID *, CONST BITMAPINFO *, UINT, DWORD);

int
STDCALL
SetROP2(HDC, int);

int
STDCALL
SetStretchBltMode(HDC, int);

UINT
STDCALL
SetSystemPaletteUse(HDC, UINT);

int
STDCALL
SetTextCharacterExtra(HDC, int);

COLORREF
STDCALL
SetTextColor(HDC, COLORREF);

UINT
STDCALL
SetTextAlign(HDC, UINT);

WINBOOL
STDCALL
SetTextJustification(HDC, int, int);

WINBOOL
STDCALL
UpdateColors(HDC);

WINBOOL
STDCALL
PlayMetaFileRecord(HDC, LPHANDLETABLE, LPMETARECORD, UINT);

WINBOOL
STDCALL
EnumMetaFile(HDC, HMETAFILE, ENUMMETAFILEPROC, LPARAM);

HENHMETAFILE
STDCALL
CloseEnhMetaFile(HDC);

WINBOOL
STDCALL
DeleteEnhMetaFile(HENHMETAFILE);

WINBOOL
STDCALL
EnumEnhMetaFile(HDC, HENHMETAFILE, ENHMETAFILEPROC,
		LPVOID, CONST RECT *);

UINT
STDCALL
GetEnhMetaFileHeader(HENHMETAFILE, UINT, LPENHMETAHEADER );

UINT
STDCALL
GetEnhMetaFilePaletteEntries(HENHMETAFILE, UINT, LPPALETTEENTRY );

UINT
STDCALL
GetWinMetaFileBits(HENHMETAFILE, UINT, LPBYTE, INT, HDC);

WINBOOL
STDCALL
PlayEnhMetaFile(HDC, HENHMETAFILE, CONST RECT *);

WINBOOL
STDCALL
PlayEnhMetaFileRecord(HDC, LPHANDLETABLE, CONST ENHMETARECORD *, UINT);

HENHMETAFILE
STDCALL
SetEnhMetaFileBits(UINT, CONST BYTE *);

#if 0
HENHMETAFILE
STDCALL
SetWinMetaFileBits(UINT, CONST BYTE *, HDC, CONST METAFILEPICT *);
#endif

WINBOOL
STDCALL
GdiComment(HDC, UINT, CONST BYTE *);

WINBOOL
STDCALL
AngleArc(HDC, int, int, DWORD, FLOAT, FLOAT);

WINBOOL
STDCALL
PolyPolyline(HDC, CONST POINT *, CONST DWORD *, DWORD);

WINBOOL
STDCALL
GetWorldTransform(HDC, LPXFORM);

WINBOOL
STDCALL
SetWorldTransform(HDC, CONST XFORM *);

WINBOOL
STDCALL
ModifyWorldTransform(HDC, CONST XFORM *, DWORD);

WINBOOL
STDCALL
CombineTransform(LPXFORM, CONST XFORM *, CONST XFORM *);

HBITMAP
STDCALL
CreateDIBSection(HDC, CONST BITMAPINFO *, UINT, VOID *, HANDLE, DWORD);

UINT
STDCALL
GetDIBColorTable(HDC, UINT, UINT, RGBQUAD *);

UINT
STDCALL
SetDIBColorTable(HDC, UINT, UINT, CONST RGBQUAD *);

WINBOOL
STDCALL
SetColorAdjustment(HDC, CONST COLORADJUSTMENT *);

WINBOOL
STDCALL
GetColorAdjustment(HDC, LPCOLORADJUSTMENT);

HPALETTE
STDCALL 
CreateHalftonePalette(HDC);

int
STDCALL
EndDoc(HDC);

int
STDCALL
StartPage(HDC);

int
STDCALL
EndPage(HDC);

int
STDCALL
AbortDoc(HDC);

int
STDCALL
SetAbortProc(HDC, ABORTPROC);

WINBOOL
STDCALL
AbortPath(HDC);

WINBOOL
STDCALL
ArcTo(HDC, int, int, int, int, int, int,int, int);

WINBOOL
STDCALL
BeginPath(HDC);

WINBOOL
STDCALL
CloseFigure(HDC);

WINBOOL
STDCALL
EndPath(HDC);

WINBOOL
STDCALL
FillPath(HDC);

WINBOOL
STDCALL
FlattenPath(HDC);

int
STDCALL
GetPath(HDC, LPPOINT, LPBYTE, int);

HRGN
STDCALL
PathToRegion(HDC);

WINBOOL
STDCALL
PolyDraw(HDC, CONST POINT *, CONST BYTE *, int);

WINBOOL
STDCALL
SelectClipPath(HDC, int);

int
STDCALL
SetArcDirection(HDC, int);

WINBOOL
STDCALL
SetMiterLimit(HDC, FLOAT, PFLOAT);

WINBOOL
STDCALL
StrokeAndFillPath(HDC);

WINBOOL
STDCALL
StrokePath(HDC);

WINBOOL
STDCALL
WidenPath(HDC);

HPEN
STDCALL
ExtCreatePen(DWORD, DWORD, CONST LOGBRUSH *, DWORD, CONST DWORD *);

WINBOOL
STDCALL
GetMiterLimit(HDC, PFLOAT);

int
STDCALL
GetArcDirection(HDC);

WINBOOL
STDCALL
MoveToEx(HDC, int, int, LPPOINT);

HRGN
STDCALL
CreatePolygonRgn(CONST POINT *, int, int);

WINBOOL
STDCALL
DPtoLP(HDC, LPPOINT, int);

WINBOOL
STDCALL
LPtoDP(HDC, LPPOINT, int);
 
WINBOOL
STDCALL
Polygon(HDC, CONST POINT *, int);

WINBOOL
STDCALL
Polyline(HDC, CONST POINT *, int);

WINBOOL
STDCALL
PolyBezier(HDC, CONST POINT *, DWORD);

WINBOOL
STDCALL
PolyBezierTo(HDC, CONST POINT *, DWORD);

WINBOOL
STDCALL
PolylineTo(HDC, CONST POINT *, DWORD);

WINBOOL
STDCALL
SetViewportExtEx(HDC, int, int, LPSIZE);

WINBOOL
STDCALL
SetViewportOrgEx(HDC, int, int, LPPOINT);

WINBOOL
STDCALL
SetWindowExtEx(HDC, int, int, LPSIZE);

WINBOOL
STDCALL
SetWindowOrgEx(HDC, int, int, LPPOINT);

WINBOOL
STDCALL
OffsetViewportOrgEx(HDC, int, int, LPPOINT);

WINBOOL
STDCALL
OffsetWindowOrgEx(HDC, int, int, LPPOINT);

WINBOOL
STDCALL
ScaleViewportExtEx(HDC, int, int, int, int, LPSIZE);

WINBOOL
STDCALL
ScaleWindowExtEx(HDC, int, int, int, int, LPSIZE);

WINBOOL
STDCALL
SetBitmapDimensionEx(HBITMAP, int, int, LPSIZE);

WINBOOL
STDCALL
SetBrushOrgEx(HDC, int, int, LPPOINT);

WINBOOL
STDCALL
GetDCOrgEx(HDC,LPPOINT);

WINBOOL
STDCALL
FixBrushOrgEx(HDC,int,int,LPPOINT);

WINBOOL
STDCALL
UnrealizeObject(HGDIOBJ);

WINBOOL
STDCALL
GdiFlush(void);

DWORD
STDCALL
GdiSetBatchLimit(DWORD);

DWORD
STDCALL
GdiGetBatchLimit(void);

int
STDCALL
SetICMMode(HDC, int);

WINBOOL
STDCALL
CheckColorsInGamut(HDC,LPVOID,LPVOID,DWORD);

HANDLE
STDCALL
GetColorSpace(HDC);

WINBOOL
STDCALL
SetColorSpace(HDC,HCOLORSPACE);

WINBOOL
STDCALL
DeleteColorSpace(HCOLORSPACE);

WINBOOL
STDCALL
GetDeviceGammaRamp(HDC,LPVOID);

WINBOOL
STDCALL
SetDeviceGammaRamp(HDC,LPVOID);

WINBOOL
STDCALL
ColorMatchToTarget(HDC,HDC,DWORD);

HPROPSHEETPAGE
STDCALL
CreatePropertySheetPageA(LPCPROPSHEETPAGEA lppsp);

WINBOOL
STDCALL
DestroyPropertySheetPage(HPROPSHEETPAGE hPSPage);

void
STDCALL
InitCommonControls(void);

#define  ImageList_AddIcon(himl, hicon) ImageList_ReplaceIcon(himl, -1, hicon)

HIMAGELIST
STDCALL
ImageList_Create(int cx, int cy, UINT flags, 
		 int cInitial, int cGrow);

WINBOOL
STDCALL
ImageList_Destroy(HIMAGELIST himl);

int
STDCALL
ImageList_GetImageCount(HIMAGELIST himl);

int
STDCALL
ImageList_Add(HIMAGELIST himl, HBITMAP hbmImage, 
	      HBITMAP hbmMask);

int
STDCALL
ImageList_ReplaceIcon(HIMAGELIST himl, int i, HICON hicon);

COLORREF
STDCALL
ImageList_SetBkColor(HIMAGELIST himl, COLORREF clrBk);

COLORREF
STDCALL
ImageList_GetBkColor(HIMAGELIST himl);

WINBOOL
STDCALL
ImageList_SetOverlayImage(HIMAGELIST himl, int iImage, 
			  int iOverlay);

WINBOOL
STDCALL
ImageList_Draw(HIMAGELIST himl, int i, HDC hdcDst, int x, int y, UINT fStyle);

WINBOOL
STDCALL
ImageList_Replace(HIMAGELIST himl, int i, HBITMAP hbmImage, HBITMAP hbmMask);

int
STDCALL
ImageList_AddMasked(HIMAGELIST himl, HBITMAP hbmImage, COLORREF crMask);

WINBOOL
STDCALL
ImageList_DrawEx(HIMAGELIST himl, int i, HDC hdcDst, int x, int y, int dx, int dy, COLORREF rgbBk, COLORREF rgbFg, UINT fStyle);

WINBOOL
STDCALL
ImageList_Remove(HIMAGELIST himl, int i);

HICON
STDCALL
ImageList_GetIcon(HIMAGELIST himl, int i, UINT flags);

WINBOOL
STDCALL
ImageList_BeginDrag(HIMAGELIST himlTrack, int iTrack, int dxHotspot, int dyHotspot);

void
STDCALL
ImageList_EndDrag(void);

WINBOOL
STDCALL
ImageList_DragEnter(HWND hwndLock, int x, int y);

WINBOOL
STDCALL
ImageList_DragLeave(HWND hwndLock);

WINBOOL
STDCALL
ImageList_DragMove(int x, int y);

WINBOOL
STDCALL
ImageList_SetDragCursorImage(HIMAGELIST himlDrag, int iDrag, int dxHotspot, int dyHotspot);

WINBOOL
STDCALL
ImageList_DragShowNolock(WINBOOL fShow);

HIMAGELIST
STDCALL
ImageList_GetDragImage(POINT * ppt,POINT * pptHotspot);

WINBOOL
STDCALL
ImageList_GetIconSize(HIMAGELIST himl, int *cx, int *cy);

WINBOOL
STDCALL
ImageList_SetIconSize(HIMAGELIST himl, int cx, int cy);

WINBOOL
STDCALL
ImageList_GetImageInfo(HIMAGELIST himl, int i, IMAGEINFO * pImageInfo);

HIMAGELIST
STDCALL
ImageList_Merge(HIMAGELIST himl1, int i1, HIMAGELIST himl2, int i2, int dx, int dy);

HWND
STDCALL
CreateToolbarEx(HWND hwnd, DWORD ws, UINT wID, int nBitmaps,
		HINSTANCE hBMInst, UINT wBMID, LPCTBBUTTON lpButtons,
		int iNumButtons, int dxButton, int dyButton,
		int dxBitmap, int dyBitmap, UINT uStructSize);

HBITMAP
STDCALL
CreateMappedBitmap(HINSTANCE hInstance, int idBitmap,
		   UINT wFlags, LPCOLORMAP lpColorMap,
		   int iNumMaps);


void
STDCALL
MenuHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, HMENU hMainMenu, HINSTANCE hInst, HWND hwndStatus, UINT *lpwIDs);

WINBOOL
STDCALL
ShowHideMenuCtl(HWND hWnd, UINT uFlags, LPINT lpInfo);

void
STDCALL
GetEffectiveClientRect(HWND hWnd, LPRECT lprc, LPINT lpInfo);

WINBOOL
STDCALL
MakeDragList(HWND hLB);

void
STDCALL
DrawInsert(HWND handParent, HWND hLB, int nItem);

int
STDCALL
LBItemFromPt(HWND hLB, POINT pt, WINBOOL bAutoScroll);

HWND
STDCALL
CreateUpDownControl(DWORD dwStyle, int x, int y, int cx, int cy,
		    HWND hParent, int nID, HINSTANCE hInst,
		    HWND hBuddy,
		    int nUpper, int nLower, int nPos);

DWORD
STDCALL
CommDlgExtendedError(VOID);

/* Animation controls */

#define Animate_Create(hwndP, id, dwStyle, hInstance)   CreateWindow(ANIMATE_CLASS, NULL, dwStyle, 0, 0, 0, 0, hwndP, (HMENU)(id), hInstance, NULL)

#define Animate_Open(hwnd, szName) SendMessage(hwnd, ACM_OPEN, 0, (LPARAM)(LPTSTR)(szName))

#define Animate_Play(hwnd, from, to, rep) SendMessage(hwnd, ACM_PLAY, (WPARAM)(UINT)(rep), (LPARAM)MAKELONG(from, to))

#define Animate_Stop(hwnd) SendMessage(hwnd, ACM_STOP, 0, 0)

#define Animate_Close(hwnd) Animate_Open(hwnd, NULL)

#define Animate_Seek(hwnd, frame) Animate_Play(hwnd, frame, frame, 1)

/* Property sheet macros */

#define PropSheet_AddPage(hPropSheetDlg, hpage) SendMessage(hPropSheetDlg, PSM_ADDPAGE, 0, (LPARAM)(HPROPSHEETPAGE)hpage)

#define PropSheet_Apply(hPropSheetDlg) SendMessage(hPropSheetDlg, PSM_APPLY, 0, 0)

#define PropSheet_CancelToClose(hPropSheetDlg) SendMessage(hPropSheetDlg, PSM_CANCELTOCLOSE, 0, 0)

#define PropSheet_Changed(hPropSheetDlg, hwndPage) SendMessage(hPropSheetDlg, PSM_CHANGED, (WPARAM)(HWND)hwndPage, 0)

#define PropSheet_GetCurrentPageHwnd(hDlg) SendMessage(hDlg, PSM_GETCURRENTPAGEHWND, 0, 0)

#define PropSheet_GetTabControl(hPropSheetDlg) SendMessage(hPropSheetDlg, PSM_GETTABCONTROL, 0, 0)

#define PropSheet_IsDialogMessage(hDlg, pMsg) SendMessage(hDlg, PSM_ISDIALOGMESSAGE, 0, (LPARAM)pMsg)

#define PropSheet_PressButton(hPropSheetDlg, iButton) SendMessage(hPropSheetDlg, PSM_PRESSBUTTON, (WPARAM)(int)iButton, 0)

#define PropSheet_QuerySiblings(hPropSheetDlg, param1, param2) SendMessage(hPropSheetDlg, PSM_QUERYSIBLINGS, (WPARAM)param1, (LPARAM)param2)

#define PropSheet_RebootSystem(hPropSheetDlg) SendMessage(hPropSheetDlg, PSM_REBOOTSYSTEM, 0, 0)

#define PropSheet_RemovePage(hPropSheetDlg, index, hpage) SendMessage(hPropSheetDlg, PSM_REMOVEPAGE, (WPARAM)(int)index, (LPARAM)(HPROPSHEETPAGE)hpage)

#define PropSheet_RestartWindows(hPropSheetDlg) SendMessage(hPropSheetDlg, PSM_RESTARTWINDOWS, 0, 0)

#define PropSheet_SetCurSel(hPropSheetDlg, hpage, index) SendMessage(hPropSheetDlg, PSM_SETCURSEL, (WPARAM)(int)index, (LPARAM)(HPROPSHEETPAGE)hpage)

#define PropSheet_SetCurSelByID(hPropSheetDlg, id) SendMessage(hPropSheetDlg, PSM_SETCURSELID, 0, (LPARAM)(int)id)

#define PropSheet_SetFinishText(hPropSheetDlg, lpszText) SendMessage(hPropSheetDlg, PSM_SETFINISHTEXT, 0, (LPARAM)(LPTSTR)lpszText)

#define PropSheet_SetTitle(hPropSheetDlg, dwStyle, lpszText) SendMessage(hPropSheetDlg, PSM_SETTITLE, (WPARAM)(DWORD)dwStyle, (LPARAM)(LPCTSTR)lpszText)

#define PropSheet_SetWizButtons(hPropSheetDlg, dwFlags) SendMessage(hPropSheetDlg, PSM_SETWIZBUTTONS, 0, (LPARAM)(DWORD)dwFlags)

#define PropSheet_UnChanged(hPropSheetDlg, hwndPage) SendMessage(hPropSheetDlg, PSM_UNCHANGED, (WPARAM)(HWND)hwndPage, 0)

/* Header control */
#define Header_DeleteItem(hwndHD, index)     (BOOL)SendMessage((hwndHD), HDM_DELETEITEM, (WPARAM)(int)(index), 0L)  

#define Header_GetItem(hwndHD, index, phdi)      (BOOL)SendMessage((hwndHD), HDM_GETITEM,   (WPARAM)(int)(index), (LPARAM)(HD_ITEM FAR*)(phdi))
 
#define Header_GetItemCount(hwndHD)   (int)SendMessage((hwndHD), HDM_GETITEMCOUNT, 0, 0L)

#define Header_InsertItem(hwndHD, index, phdi) (int)SendMessage((hwndHD), HDM_INSERTITEM, (WPARAM)(int)(index), (LPARAM)(const HD_ITEM FAR*)(phdi))
  
#define Header_Layout(hwndHD, playout) (BOOL)SendMessage((hwndHD), HDM_LAYOUT, 0, (LPARAM)(HD_LAYOUT FAR*)(playout))
  
#define Header_SetItem(hwndHD, index, phdi) (BOOL)SendMessage((hwndHD), HDM_SETITEM,  (WPARAM)(int)(index), (LPARAM)(const HD_ITEM FAR*)(phdi))

/* List View */
#define ListView_Arrange(hwndLV, code) SendMessage((hwndLV), LVM_ARRANGE, (WPARAM)(UINT)(code), 0)

#define ListView_CreateDragImage(hwnd, i, lpptUpLeft) SendMessage((hwnd), LVM_CREATEDRAGIMAGE, (WPARAM)(int)(i), (LPARAM)(LPPOINT)(lpptUpLeft))

#define ListView_DeleteAllItems(hwnd) SendMessage(hwnd, LVM_DELETEALLITEMS, 0, 0)

#define ListView_DeleteColumn(hwnd, iCol) SendMessage((hwnd), LVM_DELETECOLUMN, (WPARAM)(int)(iCol), 0)

#define ListView_DeleteItem(hwnd, iItem) SendMessage(hwnd, LVM_DELETEITEM, (WPARAM)(int)iItem, 0)

#define ListView_EditLabel(hwndLV, i) SendMessage((hwndLV), LVM_EDITLABEL, (WPARAM)(int)i, 0)

#define ListView_EnsureVisible(hwndLV, i, fPartialOK) SendMessage(hwndLV, LVM_ENSUREVISIBLE, (WPARAM)(int)i, MAKELPARAM((fPartialOK), 0))

#define ListView_FindItem(hwnd, iStart, plvfi) SendMessage(hwnd, LVM_FINDITEM, (WPARAM)(int)iStart, (LPARAM)(const LV_FINDINFO *)plvfi)

#define ListView_GetBkColor(hwnd) SendMessage((HWND)hwnd, LVM_GETBKCOLOR, 0, 0)

#define ListView_GetCallbackMask(hwnd) SendMessage(hwnd, LVM_GETCALLBACKMASK, 0, 0)

#define ListView_GetColumn(hwnd, iCol, pcol) SendMessage((hwnd), LVM_GETCOLUMN, (WPARAM)(int)(iCol), (LPARAM)(LV_COLUMN *)(pcol))

#define ListView_GetColumnWidth(hwnd, iCol) SendMessage((hwnd), LVM_GETCOLUMNWIDTH, (WPARAM)(int)(iCol), 0)

#define ListView_GetCountPerPage(hwndLV) SendMessage((hwndLV), LVM_GETCOUNTPERPAGE, 0, 0)

#define ListView_GetEditControl(hwndLV) SendMessage((hwndLV), LVM_GETEDITCONTROL, 0, 0)

#define ListView_GetImageList(hwnd, iImageList) SendMessage(hwnd, LVM_GETIMAGELIST, (WPARAM)(INT)iImageList, 0)

#define ListView_GetISearchString(hwndLV, lpsz) SendMessage((hwndLV), LVM_GETISEARCHSTRING, 0, (LPARAM)(LPTSTR)lpsz)

#define ListView_GetItem(hwnd, pitem) SendMessage(hwnd, LVM_GETITEM, 0, (LPARAM)(LV_ITEM *)(pitem))

#define ListView_GetItemCount(hwnd) SendMessage(hwnd, LVM_GETITEMCOUNT, 0, 0)

#define ListView_GetItemPosition(hwndLV, i, ppt) SendMessage(hwndLV, LVM_GETITEMPOSITION, (WPARAM)(int)i, (LPARAM)(POINT *)ppt)

#define ListView_GetItemRect(hwnd, i, prc, code) SendMessage(hwnd, LVM_GETITEMRECT, (WPARAM)(int)i, ((prc) ? (((RECT *)(prc))->left = (code), (LPARAM)(RECT *)(prc)) : (LPARAM)(RECT *)NULL))

#define ListView_GetItemSpacing(hwndLV, fSmall) SendMessage((hwndLV), LVM_GETITEMSPACING, fSmall, 0)

#define ListView_GetItemState(hwndLV, i, mask) SendMessage((hwndLV), LVM_GETITEMSTATE, (WPARAM)i, (LPARAM)mask)

#define ListView_GetItemText(hwndLV, i, iSubItem_, pszText_, cchTextMax_) { LV_ITEM _gnu_lvi;_gnu_lvi.iSubItem = iSubItem_;_gnu_lvi.cchTextMax = cchTextMax_;_gnu_lvi.pszText = pszText_;SendMessage((hwndLV), LVM_GETITEMTEXT, (WPARAM)i, (LPARAM)(LV_ITEM *)&_gnu_lvi);}

#define ListView_GetNextItem(hwnd, iStart, flags) SendMessage(hwnd, LVM_GETNEXTITEM, (WPARAM)(int)iStart, (LPARAM)flags)

#define ListView_GetOrigin(hwndLV, ppt) SendMessage((hwndLV), LVM_GETORIGIN, (WPARAM)0, (LPARAM)(POINT *)(ppt))

#define ListView_GetSelectedCount(hwndLV) SendMessage((hwndLV), LVM_GETSELECTEDCOUNT, 0, 0)

#define ListView_GetStringWidth(hwndLV, psz) SendMessage(hwndLV, LVM_GETSTRINGWIDTH, 0, (LPARAM)(LPCTSTR)psz)

#define ListView_GetTextBkColor(hwnd)  SendMessage((hwnd), LVM_GETTEXTBKCOLOR, 0, 0)

#define ListView_GetTextColor(hwnd)  SendMessage((hwnd), LVM_GETTEXTCOLOR, 0, 0)

#define ListView_GetTopIndex(hwndLV) SendMessage((hwndLV), LVM_GETTOPINDEX, 0, 0)

#define ListView_GetViewRect(hwnd, prc) SendMessage((hwnd), LVM_GETVIEWRECT, 0, (LPARAM)(RECT *)(prc))

#define ListView_HitTest(hwndLV, pinfo) SendMessage(hwndLV, LVM_HITTEST, 0, (LPARAM)(LV_HITTESTINFO *)pinfo)

#define ListView_InsertColumn(hwnd, iCol, pcol) SendMessage((hwnd), LVM_INSERTCOLUMN, (WPARAM)(int)(iCol), (LPARAM)(const LV_COLUMN *)(pcol))

#define ListView_InsertItem(hwnd, pitem) SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM)(const LV_ITEM *)pitem)

#define ListView_RedrawItems(hwndLV, iFirst, iLast) SendMessage((hwndLV), LVM_REDRAWITEMS, (WPARAM)(int)iFirst, (LPARAM)(int)iLast)

#define ListView_Scroll(hwndLV, dx, dy) SendMessage((hwndLV), LVM_SCROLL, (WPARAM)(int)dx, (LPARAM)(int)dy)

#define ListView_SetBkColor(hwnd, clrBk) SendMessage(hwnd, LVM_SETBKCOLOR, 0, (LPARAM)(COLORREF)clrBk)

#define ListView_SetCallbackMask(hwnd, mask) SendMessage(hwnd, LVM_SETCALLBACKMASK, (WPARAM)(UINT)(mask), 0)

#define ListView_SetColumn(hwnd, iCol, pcol) SendMessage((hwnd), LVM_SETCOLUMN, (WPARAM)(int)(iCol), (LPARAM)(const LV_COLUMN *)(pcol))

#define ListView_SetColumnWidth(hwnd, iCol, cx) SendMessage((hwnd), LVM_SETCOLUMNWIDTH, (WPARAM)(int)(iCol),  MAKELPARAM((cx), 0))


#define ListView_SetImageList(hwnd, himl, iImageList) SendMessage(hwnd, LVM_SETIMAGELIST, (WPARAM)(iImageList), 	    (LPARAM)(UINT)(HIMAGELIST)(himl))

#define ListView_SetItem(hwnd, pitem) SendMessage(hwnd, LVM_SETITEM, 0, (LPARAM)(const LV_ITEM *)pitem)

#define ListView_SetItemCount(hwndLV, cItems) SendMessage((hwndLV), LVM_SETITEMCOUNT, (WPARAM)cItems, 0)

#define ListView_SetItemPosition(hwndLV, i, x, y) SendMessage(hwndLV, LVM_SETITEMPOSITION, (WPARAM)(int)(i), 	    MAKELPARAM((x), (y)))

#define ListView_SetItemPosition32(hwndLV, i, x, y) { POINT ptNewPos = {x,y};     SendMessage((hwndLV), LVM_SETITEMPOSITION32, (WPARAM)(int)(i), 		(LPARAM)&ptNewPos); }

#define ListView_SetItemState(hwndLV, i, data, mask) { LV_ITEM _gnu_lvi;  _gnu_lvi.stateMask = mask;  _gnu_lvi.state = data;  SendMessage((hwndLV), LVM_SETITEMSTATE, (WPARAM)i, 	      (LPARAM)(LV_ITEM *)&_gnu_lvi);}

#define ListView_SetItemText(hwndLV, i, iSubItem_, pszText_) { LV_ITEM _gnu_lvi;  _gnu_lvi.iSubItem = iSubItem_;  _gnu_lvi.pszText = pszText_;  SendMessage((hwndLV), LVM_SETITEMTEXT, (WPARAM)i, 	      (LPARAM)(LV_ITEM *)&_gnu_lvi);}

#define ListView_SetTextBkColor(hwnd, clrTextBk) SendMessage((hwnd), LVM_SETTEXTBKCOLOR, 0, (LPARAM)(COLORREF)(clrTextBk))

#define ListView_SetTextColor(hwnd, clrText) SendMessage((hwnd), LVM_SETTEXTCOLOR, 0, (LPARAM)(COLORREF)(clrText))

#define ListView_SortItems(hwndLV, _pfnCompare, _lPrm) SendMessage((hwndLV), LVM_SORTITEMS, (WPARAM)(LPARAM)_lPrm, 	    (LPARAM)(PFNLVCOMPARE)_pfnCompare)

#define ListView_Update(hwndLV, i) SendMessage((hwndLV), LVM_UPDATE, (WPARAM)i, 0)

/* Tree View */
#define TreeView_InsertItem(hwnd, lpis) SendMessage((hwnd), TVM_INSERTITEM, 0, (LPARAM)(LPTV_INSERTSTRUCT)(lpis))

#define TreeView_DeleteItem(hwnd, hitem) SendMessage((hwnd), TVM_DELETEITEM, 0, (LPARAM)(HTREEITEM)(hitem))

#define TreeView_DeleteAllItems(hwnd) SendMessage((hwnd), TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT)

#define TreeView_Expand(hwnd, hitem, code) SendMessage((hwnd), TVM_EXPAND, (WPARAM)code, (LPARAM)(HTREEITEM)(hitem))

#define TreeView_GetItemRect(hwnd, hitem, prc, code) SendMessage((hwnd), TVM_GETITEMRECT, (WPARAM)(code), (LPARAM)(RECT *)(prc)))

#define TreeView_GetCount(hwnd) SendMessage((hwnd), TVM_GETCOUNT, 0, 0)

#define TreeView_GetIndent(hwnd) SendMessage((hwnd), TVM_GETINDENT, 0, 0)

#define TreeView_SetIndent(hwnd, indent) SendMessage((hwnd), TVM_SETINDENT, (WPARAM)indent, 0)

#define TreeView_GetImageList(hwnd, iImage) SendMessage((hwnd), TVM_GETIMAGELIST, iImage, 0)

#define TreeView_SetImageList(hwnd, himl, iImage) SendMessage((hwnd), TVM_SETIMAGELIST, iImage, (LPARAM)(UINT)(HIMAGELIST)(himl))

#define TreeView_GetNextItem(hwnd, hitem, code) SendMessage((hwnd), TVM_GETNEXTITEM, (WPARAM)code, (LPARAM)(HTREEITEM)(hitem))

#define TreeView_GetChild(hwnd, hitem) TreeView_GetNextItem(hwnd, hitem, TVGN_CHILD)

#define TreeView_GetNextSibling(hwnd, hitem) TreeView_GetNextItem(hwnd, hitem, TVGN_NEXT)

#define TreeView_GetPrevSibling(hwnd, hitem) TreeView_GetNextItem(hwnd, hitem, TVGN_PREVIOUS)

#define TreeView_GetParent(hwnd, hitem) TreeView_GetNextItem(hwnd, hitem, TVGN_PARENT)

#define TreeView_GetFirstVisible(hwnd) TreeView_GetNextItem(hwnd, NULL,  TVGN_FIRSTVISIBLE)

#define TreeView_GetNextVisible(hwnd, hitem) TreeView_GetNextItem(hwnd, hitem, TVGN_NEXTVISIBLE)

#define TreeView_GetPrevVisible(hwnd, hitem) TreeView_GetNextItem(hwnd, hitem, TVGN_PREVIOUSVISIBLE)

#define TreeView_GetSelection(hwnd) TreeView_GetNextItem(hwnd, NULL,  TVGN_CARET)

#define TreeView_GetDropHilight(hwnd) TreeView_GetNextItem(hwnd, NULL,  TVGN_DROPHILITE)

#define TreeView_GetRoot(hwnd) TreeView_GetNextItem(hwnd, NULL,  TVGN_ROOT)

#define TreeView_Select(hwnd, hitem, code) SendMessage((hwnd), TVM_SELECTITEM, (WPARAM)code, (LPARAM)(HTREEITEM)(hitem))


#define TreeView_SelectItem(hwnd, hitem) TreeView_Select(hwnd, hitem, TVGN_CARET)

#define TreeView_SelectDropTarget(hwnd, hitem) TreeView_Select(hwnd, hitem, TVGN_DROPHILITE)

#define TreeView_SelectSetFirstVisible(hwnd, hitem) TreeView_Select(hwnd, hitem, TVGN_FIRSTVISIBLE)

#define TreeView_GetItem(hwnd, pitem) SendMessage((hwnd), TVM_GETITEM, 0, (LPARAM)(TV_ITEM *)(pitem))

#define TreeView_SetItem(hwnd, pitem) SendMessage((hwnd), TVM_SETITEM, 0, (LPARAM)(const TV_ITEM *)(pitem))

#define TreeView_EditLabel(hwnd, hitem) SendMessage((hwnd), TVM_EDITLABEL, 0, (LPARAM)(HTREEITEM)(hitem))

#define TreeView_GetEditControl(hwnd) SendMessage((hwnd), TVM_GETEDITCONTROL, 0, 0)

#define TreeView_GetVisibleCount(hwnd) SendMessage((hwnd), TVM_GETVISIBLECOUNT, 0, 0)

#define TreeView_HitTest(hwnd, lpht) SendMessage((hwnd), TVM_HITTEST, 0, (LPARAM)(LPTV_HITTESTINFO)(lpht))

#define TreeView_CreateDragImage(hwnd, hitem) SendMessage((hwnd), TVM_CREATEDRAGIMAGE, 0, (LPARAM)(HTREEITEM)(hitem))

#define TreeView_SortChildren(hwnd, hitem, recurse) SendMessage((hwnd), TVM_SORTCHILDREN, (WPARAM)recurse,             (LPARAM)(HTREEITEM)(hitem))

#define TreeView_EnsureVisible(hwnd, hitem) SendMessage((hwnd), TVM_ENSUREVISIBLE, 0, (LPARAM)(HTREEITEM)(hitem))

#define TreeView_SortChildrenCB(hwnd, psort, recurse) SendMessage((hwnd), TVM_SORTCHILDRENCB, (WPARAM)recurse,             (LPARAM)(LPTV_SORTCB)(psort))

#define TreeView_EndEditLabelNow(hwnd, fCancel) SendMessage((hwnd), TVM_ENDEDITLABELNOW, (WPARAM)fCancel, 0)

#define TreeView_GetISearchString(hwndTV, lpsz) SendMessage((hwndTV), TVM_GETISEARCHSTRING, 0, (LPARAM)(LPTSTR)lpsz)


/* Tab control */
#define TabCtrl_GetImageList(hwnd) SendMessage((hwnd), TCM_GETIMAGELIST, 0, 0)

#define TabCtrl_SetImageList(hwnd, himl) SendMessage((hwnd), TCM_SETIMAGELIST, 0, (LPARAM)(UINT)(HIMAGELIST)(himl))

#define TabCtrl_GetItemCount(hwnd) SendMessage((hwnd), TCM_GETITEMCOUNT, 0, 0)

#define TabCtrl_GetItem(hwnd, iItem, pitem) SendMessage((hwnd), TCM_GETITEM, (WPARAM)(int)iItem,             (LPARAM)(TC_ITEM *)(pitem))

#define TabCtrl_SetItem(hwnd, iItem, pitem) SendMessage((hwnd), TCM_SETITEM, (WPARAM)(int)iItem,             (LPARAM)(TC_ITEM *)(pitem))

#define TabCtrl_InsertItem(hwnd, iItem, pitem)   SendMessage((hwnd), TCM_INSERTITEM, (WPARAM)(int)iItem,             (LPARAM)(const TC_ITEM *)(pitem))

#define TabCtrl_DeleteItem(hwnd, i) SendMessage((hwnd), TCM_DELETEITEM, (WPARAM)(int)(i), 0)

#define TabCtrl_DeleteAllItems(hwnd) SendMessage((hwnd), TCM_DELETEALLITEMS, 0, 0)

#define TabCtrl_GetItemRect(hwnd, i, prc) SendMessage((hwnd), TCM_GETITEMRECT, (WPARAM)(int)(i), (LPARAM)(RECT *)(prc))

#define TabCtrl_GetCurSel(hwnd) SendMessage((hwnd), TCM_GETCURSEL, 0, 0)

#define TabCtrl_SetCurSel(hwnd, i) SendMessage((hwnd), TCM_SETCURSEL, (WPARAM)i, 0)

#define TabCtrl_HitTest(hwndTC, pinfo) SendMessage((hwndTC), TCM_HITTEST, 0, (LPARAM)(TC_HITTESTINFO *)(pinfo))

#define TabCtrl_SetItemExtra(hwndTC, cb) SendMessage((hwndTC), TCM_SETITEMEXTRA, (WPARAM)(cb), 0)

#define TabCtrl_AdjustRect(hwnd, bLarger, prc) SendMessage(hwnd, TCM_ADJUSTRECT, (WPARAM)(WINBOOL)bLarger,             (LPARAM)(RECT *)prc)

#define TabCtrl_SetItemSize(hwnd, x, y) SendMessage((hwnd), TCM_SETITEMSIZE, 0, MAKELPARAM(x,y))

#define TabCtrl_RemoveImage(hwnd, i) SendMessage((hwnd), TCM_REMOVEIMAGE, i, 0)

#define TabCtrl_SetPadding(hwnd,  cx, cy) SendMessage((hwnd), TCM_SETPADDING, 0, MAKELPARAM(cx, cy))

#define TabCtrl_GetRowCount(hwnd) SendMessage((hwnd), TCM_GETROWCOUNT, 0, 0)

#define TabCtrl_GetToolTips(hwnd) SendMessage((hwnd), TCM_GETTOOLTIPS, 0, 0)

#define TabCtrl_SetToolTips(hwnd, hwndTT) SendMessage((hwnd), TCM_SETTOOLTIPS, (WPARAM)hwndTT, 0)

#define TabCtrl_GetCurFocus(hwnd) SendMessage((hwnd), TCM_GETCURFOCUS, 0, 0)

#define TabCtrl_SetCurFocus(hwnd, i) SendMessage((hwnd),TCM_SETCURFOCUS, i, 0)

#define CommDlg_OpenSave_GetSpecA(_hdlg, _psz, _cbmax) SNDMSG(_hdlg, CDM_GETSPEC, (WPARAM)_cbmax, (LPARAM)(LPSTR)_psz)

#define CommDlg_OpenSave_GetSpecW(_hdlg, _psz, _cbmax) SNDMSG(_hdlg, CDM_GETSPEC, (WPARAM)_cbmax, (LPARAM)(LPWSTR)_psz)

#ifndef _DISABLE_TIDENTS
#ifdef UNICODE
#define CommDlg_OpenSave_GetSpec  CommDlg_OpenSave_GetSpecW
#else
#define CommDlg_OpenSave_GetSpec  CommDlg_OpenSave_GetSpecA
#endif /* !UNICODE */
#endif /* _DISABLE_TIDENTS */

#define CommDlg_OpenSave_GetFilePathA(_hdlg, _psz, _cbmax) SNDMSG(_hdlg, CDM_GETFILEPATH, (WPARAM)_cbmax, (LPARAM)(LPSTR)_psz)

#define CommDlg_OpenSave_GetFilePathW(_hdlg, _psz, _cbmax) SNDMSG(_hdlg, CDM_GETFILEPATH, (WPARAM)_cbmax, (LPARAM)(LPWSTR)_psz)

#ifndef _DISABLE_TIDENTS
#ifdef UNICODE
#define CommDlg_OpenSave_GetFilePath  CommDlg_OpenSave_GetFilePathW
#else
#define CommDlg_OpenSave_GetFilePath  CommDlg_OpenSave_GetFilePathA
#endif /* !UNICODE */
#endif /* _DISABLE_TIDENTS */

#define CommDlg_OpenSave_GetFolderPathA(_hdlg, _psz, _cbmax) SNDMSG(_hdlg, CDM_GETFOLDERPATH, (WPARAM)_cbmax, (LPARAM)(LPSTR)_psz)

#define CommDlg_OpenSave_GetFolderPathW(_hdlg, _psz, _cbmax) SNDMSG(_hdlg, CDM_GETFOLDERPATH, (WPARAM)_cbmax, (LPARAM)(LPWSTR)_psz)

#ifndef _DISABLE_TIDENTS
#ifdef UNICODE
#define CommDlg_OpenSave_GetFolderPath  CommDlg_OpenSave_GetFolderPathW
#else
#define CommDlg_OpenSave_GetFolderPath  CommDlg_OpenSave_GetFolderPathA
#endif /* !UNICODE */
#endif /* _DISABLE_TIDENTS */

#define CommDlg_OpenSave_GetFolderIDList(_hdlg, _pidl, _cbmax) SNDMSG(_hdlg, CDM_GETFOLDERIDLIST, (WPARAM)_cbmax, (LPARAM)(LPVOID)_pidl)

#define CommDlg_OpenSave_SetControlText(_hdlg, _id, _text) SNDMSG(_hdlg, CDM_SETCONTROLTEXT, (WPARAM)_id, (LPARAM)(LPSTR)_text)

#define CommDlg_OpenSave_HideControl(_hdlg, _id) SNDMSG(_hdlg, CDM_HIDECONTROL, (WPARAM)_id, 0)

#define CommDlg_OpenSave_SetDefExt(_hdlg, _pszext) SNDMSG(_hdlg, CDM_SETDEFEXT, 0, (LPARAM)(LPSTR)_pszext)

LONG
STDCALL
RegCloseKey (
    HKEY hKey
    );

LONG
STDCALL
RegSetKeySecurity (
    HKEY hKey,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor
    );

LONG
STDCALL
RegFlushKey (
    HKEY hKey
    );

LONG
STDCALL
RegGetKeySecurity (
    HKEY hKey,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    LPDWORD lpcbSecurityDescriptor
    );

LONG
STDCALL
RegNotifyChangeKeyValue (
    HKEY hKey,
    WINBOOL bWatchSubtree,
    DWORD dwNotifyFilter,
    HANDLE hEvent,
    WINBOOL fAsynchronus
    );

WINBOOL
STDCALL
IsValidCodePage(
    UINT  CodePage);


UINT
STDCALL
GetACP(void);


UINT
STDCALL
GetOEMCP(void);


WINBOOL
STDCALL
GetCPInfo(UINT, LPCPINFO);


WINBOOL
STDCALL
IsDBCSLeadByte(
    BYTE  TestChar);


WINBOOL
STDCALL
IsDBCSLeadByteEx(
    UINT  CodePage,
    BYTE  TestChar);


int
STDCALL
MultiByteToWideChar(
    UINT     CodePage,
    DWORD    dwFlags,
    LPCSTR   lpMultiByteStr,
    int      cchMultiByte,
    LPWSTR   lpWideCharStr,
    int      cchWideChar);


int
STDCALL
WideCharToMultiByte(
    UINT     CodePage,
    DWORD    dwFlags,
    LPCWSTR  lpWideCharStr,
    int      cchWideChar,
    LPSTR    lpMultiByteStr,
    int      cchMultiByte,
    LPCSTR   lpDefaultChar,
    LPBOOL   lpUsedDefaultChar);

WINBOOL
STDCALL
IsValidLocale(
    LCID   Locale,
    DWORD  dwFlags);


LCID
STDCALL
ConvertDefaultLocale(
    LCID   Locale);


LCID
STDCALL
GetThreadLocale(void);


WINBOOL
STDCALL
SetThreadLocale(
    LCID  Locale
    );


LANGID
STDCALL
GetSystemDefaultLangID(void);


LANGID
STDCALL
GetUserDefaultLangID(void);


LCID
STDCALL
GetSystemDefaultLCID(void);


LCID
STDCALL
GetUserDefaultLCID(void);


WINBOOL
STDCALL
ReadConsoleOutputAttribute(
    HANDLE hConsoleOutput,
    LPWORD lpAttribute,
    DWORD nLength,
    COORD dwReadCoord,
    LPDWORD lpNumberOfAttrsRead
    );


WINBOOL
STDCALL
WriteConsoleOutputAttribute(
    HANDLE hConsoleOutput,
    CONST WORD *lpAttribute,
    DWORD nLength,
    COORD dwWriteCoord,
    LPDWORD lpNumberOfAttrsWritten
    );


WINBOOL
STDCALL
FillConsoleOutputAttribute(
    HANDLE hConsoleOutput,
    WORD   wAttribute,
    DWORD  nLength,
    COORD  dwWriteCoord,
    LPDWORD lpNumberOfAttrsWritten
    );


WINBOOL
STDCALL
GetConsoleMode(
    HANDLE hConsoleHandle,
    LPDWORD lpMode
    );


WINBOOL
STDCALL
GetNumberOfConsoleInputEvents(
    HANDLE hConsoleInput,
    LPDWORD lpNumberOfEvents
    );


WINBOOL
STDCALL
GetConsoleScreenBufferInfo(
    HANDLE hConsoleOutput,
    PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo
    );


COORD
STDCALL
GetLargestConsoleWindowSize(
    HANDLE hConsoleOutput
    );


WINBOOL
STDCALL
GetConsoleCursorInfo(
    HANDLE hConsoleOutput,
    PCONSOLE_CURSOR_INFO lpConsoleCursorInfo
    );


WINBOOL
STDCALL
GetNumberOfConsoleMouseButtons(
    LPDWORD lpNumberOfMouseButtons
    );


WINBOOL
STDCALL
SetConsoleMode(
    HANDLE hConsoleHandle,
    DWORD dwMode
    );


WINBOOL
STDCALL
SetConsoleActiveScreenBuffer(
    HANDLE hConsoleOutput
    );


WINBOOL
STDCALL
FlushConsoleInputBuffer(
    HANDLE hConsoleInput
    );


WINBOOL
STDCALL
SetConsoleScreenBufferSize(
    HANDLE hConsoleOutput,
    COORD dwSize
    );


WINBOOL
STDCALL
SetConsoleCursorPosition(
    HANDLE hConsoleOutput,
    COORD dwCursorPosition
    );


WINBOOL
STDCALL
SetConsoleCursorInfo(
    HANDLE hConsoleOutput,
    CONST CONSOLE_CURSOR_INFO *lpConsoleCursorInfo
    );

WINBOOL
STDCALL
SetConsoleWindowInfo(
    HANDLE hConsoleOutput,
    WINBOOL bAbsolute,
    CONST SMALL_RECT *lpConsoleWindow
    );


WINBOOL
STDCALL
SetConsoleTextAttribute(
    HANDLE hConsoleOutput,
    WORD wAttributes
    );


WINBOOL
STDCALL
SetConsoleCtrlHandler(
    PHANDLER_ROUTINE HandlerRoutine,
    WINBOOL Add
    );


WINBOOL
STDCALL
GenerateConsoleCtrlEvent(
    DWORD dwCtrlEvent,
    DWORD dwProcessGroupId
    );


WINBOOL
STDCALL
AllocConsole( VOID );


WINBOOL
STDCALL
FreeConsole( VOID );



HANDLE
STDCALL
CreateConsoleScreenBuffer(
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    CONST SECURITY_ATTRIBUTES *lpSecurityAttributes,
    DWORD dwFlags,
    LPVOID lpScreenBufferData
    );


UINT
STDCALL
GetConsoleCP( VOID );


WINBOOL
STDCALL
SetConsoleCP(
    UINT wCodePageID
    );


UINT
STDCALL
GetConsoleOutputCP( VOID );


WINBOOL
STDCALL
SetConsoleOutputCP(
    UINT wCodePageID
    );

BOOL
STDCALL
VerifyConsoleIoHandle(
    HANDLE Handle
    );

DWORD STDCALL
WNetConnectionDialog(
    HWND  hwnd,
    DWORD dwType
    );

DWORD STDCALL
WNetDisconnectDialog(
    HWND  hwnd,
    DWORD dwType
    );

DWORD STDCALL
WNetCloseEnum(
    HANDLE   hEnum
    );

WINBOOL
STDCALL
CloseServiceHandle(
    SC_HANDLE   hSCObject
    );


WINBOOL
STDCALL
ControlService(
    SC_HANDLE           hService,
    DWORD               dwControl,
    LPSERVICE_STATUS    lpServiceStatus
    );

WINBOOL
STDCALL
DeleteService(
    SC_HANDLE   hService
    );

SC_LOCK
STDCALL
LockServiceDatabase(
    SC_HANDLE   hSCManager
    );


WINBOOL
STDCALL
NotifyBootConfigStatus(
    WINBOOL     BootAcceptable
    );

WINBOOL
STDCALL
QueryServiceObjectSecurity(
    SC_HANDLE               hService,
    SECURITY_INFORMATION    dwSecurityInformation,
    PSECURITY_DESCRIPTOR    lpSecurityDescriptor,
    DWORD                   cbBufSize,
    LPDWORD                 pcbBytesNeeded
    );


WINBOOL
STDCALL
QueryServiceStatus(
    SC_HANDLE           hService,
    LPSERVICE_STATUS    lpServiceStatus
    );

WINBOOL
STDCALL
SetServiceObjectSecurity(
    SC_HANDLE               hService,
    SECURITY_INFORMATION    dwSecurityInformation,
    PSECURITY_DESCRIPTOR    lpSecurityDescriptor
    );


WINBOOL
STDCALL
SetServiceStatus(
    SERVICE_STATUS_HANDLE   hServiceStatus,
    LPSERVICE_STATUS        lpServiceStatus
    );

WINBOOL
STDCALL
UnlockServiceDatabase(
    SC_LOCK     ScLock
    );

WINBOOL
STDCALL
QueryServiceStatusEx(
    SC_HANDLE           hService,
    SC_STATUS_TYPE      InfoLevel,
    LPBYTE              lpBuffer,
    DWORD               cbBufSize,
    LPDWORD             pcbBytesNeeded
    );

/* Extensions to OpenGL */

int STDCALL
ChoosePixelFormat(HDC, CONST PIXELFORMATDESCRIPTOR *);	

int STDCALL
DescribePixelFormat(HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);

UINT STDCALL
GetEnhMetaFilePixelFormat(HENHMETAFILE, DWORD,
			  CONST PIXELFORMATDESCRIPTOR *);

int STDCALL
GetPixelFormat(HDC);

WINBOOL STDCALL
SetPixelFormat(HDC, int, CONST PIXELFORMATDESCRIPTOR *);

WINBOOL STDCALL
SwapBuffers(HDC);

HGLRC STDCALL
wglCreateContext(HDC);

HGLRC STDCALL
wglCreateLayerContext(HDC, int);

WINBOOL STDCALL
wglCopyContext(HGLRC, HGLRC, UINT);

WINBOOL STDCALL
wglDeleteContext(HGLRC);

WINBOOL STDCALL
wglDescribeLayerPlane(HDC, int, int, UINT, LPLAYERPLANEDESCRIPTOR);

HGLRC STDCALL
wglGetCurrentContext(VOID);

HDC STDCALL
wglGetCurrentDC(VOID);

int STDCALL
wglGetLayerPaletteEntries(HDC, int, int, int, CONST COLORREF *);

PROC STDCALL
wglGetProcAddress(LPCSTR);

WINBOOL STDCALL
wglMakeCurrent(HDC, HGLRC);

WINBOOL STDCALL
wglRealizeLayerPalette(HDC, int, WINBOOL);

int STDCALL
wglSetLayerPaletteEntries(HDC, int, int, int, CONST COLORREF *);

WINBOOL STDCALL
wglShareLists(HGLRC, HGLRC);

WINBOOL STDCALL
wglSwapLayerBuffers(HDC, UINT);

/*
  Why are these different between ANSI and UNICODE?
  There doesn't seem to be any difference.
  */

#ifndef _DISABLE_TIDENTS
#ifdef UNICODE
#define wglUseFontBitmaps  wglUseFontBitmapsW
#define wglUseFontOutlines  wglUseFontOutlinesW
#else
#define wglUseFontBitmaps  wglUseFontBitmapsA
#define wglUseFontOutlines  wglUseFontOutlinesA
#endif /* !UNICODE */
#endif /* _DISABLE_TIDENTS */

/* ------------------------------------- */
/* From shellapi.h in old Cygnus headers */

WINBOOL STDCALL
DragQueryPoint (HDROP, LPPOINT);

void STDCALL
DragFinish (HDROP);

void STDCALL
DragAcceptFiles (HWND, WINBOOL);

HICON STDCALL
DuplicateIcon (HINSTANCE, HICON);

/* end of stuff from shellapi.h in old Cygnus headers */
/* -------------------------------------------------- */
/* From ddeml.h in old Cygnus headers */

HCONV STDCALL	DdeConnect (DWORD, HSZ, HSZ, CONVCONTEXT *);
WINBOOL STDCALL	DdeDisconnect (HCONV);
WINBOOL STDCALL	DdeFreeDataHandle (HDDEDATA);
DWORD STDCALL	DdeGetData (HDDEDATA, BYTE *, DWORD, DWORD);
UINT STDCALL	DdeGetLastError (DWORD);
HDDEDATA STDCALL	DdeNameService (DWORD, HSZ, HSZ, UINT);
WINBOOL STDCALL	DdePostAdvise (DWORD, HSZ, HSZ);
HCONV STDCALL	DdeReconnect (HCONV);
WINBOOL STDCALL	DdeUninitialize (DWORD);
int STDCALL	DdeCmpStringHandles (HSZ, HSZ);
HDDEDATA STDCALL	DdeCreateDataHandle (DWORD, LPBYTE, DWORD, DWORD, HSZ,
				UINT, UINT);

/* end of stuff from ddeml.h in old Cygnus headers */
/* ----------------------------------------------- */

DWORD STDCALL NetUserEnum (LPWSTR, DWORD, DWORD, LPBYTE*, DWORD, LPDWORD,
				LPDWORD, LPDWORD);
DWORD STDCALL NetApiBufferFree (LPVOID);
DWORD STDCALL NetUserGetInfo (LPWSTR, LPWSTR, DWORD, LPBYTE);
DWORD STDCALL NetGetDCName (LPWSTR, LPWSTR, LPBYTE*);
DWORD STDCALL NetGroupEnum (LPWSTR, DWORD, LPBYTE*, DWORD, LPDWORD,
				LPDWORD, LPDWORD);
DWORD STDCALL NetLocalGroupEnum (LPWSTR, DWORD, LPBYTE*, DWORD, LPDWORD,
				LPDWORD, LPDWORD);


VOID CopyMemory(PVOID Destination, CONST VOID* Source, DWORD Length);

DWORD STDCALL GetCurrentTime(VOID);

void STDCALL
SHAddToRecentDocs (UINT, LPCVOID);

LPITEMIDLIST STDCALL
SHBrowseForFolder (LPBROWSEINFO);

void STDCALL
SHChangeNotify (LONG, UINT, LPCVOID, LPCVOID);

int STDCALL
SHFileOperationA (LPSHFILEOPSTRUCTA);

int STDCALL
SHFileOperationW (LPSHFILEOPSTRUCTW);

void STDCALL
SHFreeNameMappings (HANDLE);

/* Define when SHELLFOLDER is defined.
HRESULT STDCALL
SHGetDataFromIDList (LPSHELLFOLDER, LPCITEMIDLIST, int, PVOID, int);

HRESULT STDCALL
SHGetDesktopFolder (LPSHELLFOLDER);
*/

/* Define when IUnknown is defined.
HRESULT STDCALL
SHGetInstanceExplorer (IUnknown **);
*/

/* Define when MALLOC is defined.
HRESULT STDCALL
SHGetMalloc (LPMALLOC *);
*/

HRESULT STDCALL
SHGetSpecialFolderLocation (HWND, int, LPITEMIDLIST *);

/* Define when REFCLSID is defined.
HRESULT STDCALL
SHLoadInProc (REFCLSID);
*/

/* Win32 Fibers */

#define FIBER_FLAG_FLOAT_SWITCH (1)

BOOL STDCALL ConvertFiberToThread(void);

LPVOID STDCALL ConvertThreadToFiber(LPVOID lpParameter);

LPVOID STDCALL ConvertThreadToFiberEx(LPVOID lpParameter, DWORD dwFlags);

LPVOID STDCALL CreateFiber
(
 SIZE_T dwStackSize,
 LPFIBER_START_ROUTINE lpStartAddress,
 LPVOID lpParameter
);

LPVOID STDCALL CreateFiberEx
(
 SIZE_T dwStackCommitSize,
 SIZE_T dwStackReserveSize,
 DWORD dwFlags,
 LPFIBER_START_ROUTINE lpStartAddress,
 LPVOID lpParameter
);

void STDCALL DeleteFiber(LPVOID lpFiber);

void STDCALL SwitchToFiber(LPVOID lpFiber);

#define GetFiberData() *(LPVOID *)(((PNT_TIB)NtCurrentTeb())->Fib.FiberData)

WINBOOL STDCALL
RegisterServicesProcess(DWORD ServicesProcessId);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* WIN32_LEAN_AND_MEAN */

#endif /* _GNU_H_WINDOWS32_FUNCTIONS */
