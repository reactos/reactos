/* 
   UnicodeFunctions.h

   Declarations for all the Windows32 API Unicode Functions

   Copyright (C) 1996 Free Software Foundation, Inc.

   Author:  Scott Christley <scottc@net-community.com>
   Date: 1996
   
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

#ifndef _GNU_H_WINDOWS32_UNICODEFUNCTIONS
#define _GNU_H_WINDOWS32_UNICODEFUNCTIONS

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

WINBOOL
STDCALL
GetBinaryTypeW(
    LPCWSTR lpApplicationName,
    LPDWORD lpBinaryType
    );

DWORD
STDCALL
GetShortPathNameW(
    LPCWSTR lpszLongPath,
    LPWSTR  lpszShortPath,
    DWORD    cchBuffer
    );

LPWSTR
STDCALL
GetEnvironmentStringsW(
    VOID
    );

WINBOOL
STDCALL
FreeEnvironmentStringsW(
    LPWSTR
    );

DWORD
STDCALL
FormatMessageW(
    DWORD dwFlags,
    LPCVOID lpSource,
    DWORD dwMessageId,
    DWORD dwLanguageId,
    LPWSTR lpBuffer,
    DWORD nSize,
    va_list *Arguments
    );

HANDLE
STDCALL
CreateMailslotW(
    LPCWSTR lpName,
    DWORD nMaxMessageSize,
    DWORD lReadTimeout,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

int
STDCALL
lstrcmpW(
    LPCWSTR lpString1,
    LPCWSTR lpString2
    );

int
STDCALL
lstrcmpiW(
    LPCWSTR lpString1,
    LPCWSTR lpString2
    );

LPWSTR
STDCALL
lstrcpynW(
    LPWSTR lpString1,
    LPCWSTR lpString2,
    int iMaxLength
    );

LPWSTR
STDCALL
lstrcpyW(
    LPWSTR lpString1,
    LPCWSTR lpString2
    );

LPWSTR
STDCALL
lstrcatW(
    LPWSTR lpString1,
    LPCWSTR lpString2
    );

int
STDCALL
lstrlenW(
    LPCWSTR lpString
    );

HANDLE
STDCALL
CreateMutexW(
    LPSECURITY_ATTRIBUTES lpMutexAttributes,
    WINBOOL bInitialOwner,
    LPCWSTR lpName
    );

HANDLE
STDCALL
OpenMutexW(
    DWORD dwDesiredAccess,
    WINBOOL bInheritHandle,
    LPCWSTR lpName
    );

HANDLE
STDCALL
CreateEventW(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    WINBOOL bManualReset,
    WINBOOL bInitialState,
    LPCWSTR lpName
    );

HANDLE
STDCALL
OpenEventW(
    DWORD dwDesiredAccess,
    WINBOOL bInheritHandle,
    LPCWSTR lpName
    );

HANDLE
STDCALL
CreateSemaphoreW(
		 LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
		 LONG lInitialCount,
		 LONG lMaximumCount,
		 LPCWSTR lpName
		 );

HANDLE
STDCALL
OpenSemaphoreW(
    DWORD dwDesiredAccess,
    WINBOOL bInheritHandle,
    LPCWSTR lpName
    );

HANDLE
STDCALL
CreateFileMappingW(
    HANDLE hFile,
    LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    DWORD flProtect,
    DWORD dwMaximumSizeHigh,
    DWORD dwMaximumSizeLow,
    LPCWSTR lpName
    );

HANDLE
STDCALL
OpenFileMappingW(
    DWORD dwDesiredAccess,
    WINBOOL bInheritHandle,
    LPCWSTR lpName
    );

DWORD
STDCALL
GetLogicalDriveStringsW(
    DWORD nBufferLength,
    LPWSTR lpBuffer
    );

HINSTANCE
STDCALL
LoadLibraryW(
    LPCWSTR lpLibFileName
    );

HINSTANCE
STDCALL
LoadLibraryExW(
    LPCWSTR lpLibFileName,
    HANDLE hFile,
    DWORD dwFlags
    );

DWORD
STDCALL
GetModuleFileNameW(
    HINSTANCE hModule,
    LPWSTR lpFilename,
    DWORD nSize
    );

HMODULE
STDCALL
GetModuleHandleW(
    LPCWSTR lpModuleName
    );

VOID
STDCALL
FatalAppExitW(
    UINT uAction,
    LPCWSTR lpMessageText
    );

LPWSTR
STDCALL
GetCommandLineW(
    VOID
    );

DWORD
STDCALL
GetEnvironmentVariableW(
    LPCWSTR lpName,
    LPWSTR lpBuffer,
    DWORD nSize
    );

WINBOOL
STDCALL
SetEnvironmentVariableW(
    LPCWSTR lpName,
    LPCWSTR lpValue
    );

DWORD
STDCALL
ExpandEnvironmentStringsW(
    LPCWSTR lpSrc,
    LPWSTR lpDst,
    DWORD nSize
    );

VOID
STDCALL
OutputDebugStringW(
    LPCWSTR lpOutputString
    );

HRSRC
STDCALL
FindResourceW(
    HINSTANCE hModule,
    LPCWSTR lpName,
    LPCWSTR lpType
    );

HRSRC
STDCALL
FindResourceExW(
    HINSTANCE hModule,
    LPCWSTR lpType,
    LPCWSTR lpName,
    WORD    wLanguage
    );

WINBOOL
STDCALL
EnumResourceTypesW(
    HINSTANCE hModule,
    ENUMRESTYPEPROC lpEnumFunc,
    LONG lParam
    );

WINBOOL
STDCALL
EnumResourceNamesW(
    HINSTANCE hModule,
    LPCWSTR lpType,
    ENUMRESNAMEPROC lpEnumFunc,
    LONG lParam
    );

WINBOOL
STDCALL
EnumResourceLanguagesW(
    HINSTANCE hModule,
    LPCWSTR lpType,
    LPCWSTR lpName,
    ENUMRESLANGPROC lpEnumFunc,
    LONG lParam
    );

HANDLE
STDCALL
BeginUpdateResourceW(
    LPCWSTR pFileName,
    WINBOOL bDeleteExistingResources
    );

WINBOOL
STDCALL
UpdateResourceW(
    HANDLE      hUpdate,
    LPCWSTR     lpType,
    LPCWSTR     lpName,
    WORD        wLanguage,
    LPVOID      lpData,
    DWORD       cbData
    );

WINBOOL
STDCALL
EndUpdateResourceW(
    HANDLE      hUpdate,
    WINBOOL        fDiscard
    );

ATOM
STDCALL
GlobalAddAtomW(
    LPCWSTR lpString
    );

ATOM
STDCALL
GlobalFindAtomW(
    LPCWSTR lpString
    );

UINT
STDCALL
GlobalGetAtomNameW(
    ATOM nAtom,
    LPWSTR lpBuffer,
    int nSize
    );

ATOM
STDCALL
AddAtomW(
    LPCWSTR lpString
    );

ATOM
STDCALL
FindAtomW(
    LPCWSTR lpString
    );

UINT
STDCALL
GetAtomNameW(
    ATOM nAtom,
    LPWSTR lpBuffer,
    int nSize
    );

UINT
STDCALL
GetProfileIntW(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    INT nDefault
    );

DWORD
STDCALL
GetProfileStringW(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    LPCWSTR lpDefault,
    LPWSTR lpReturnedString,
    DWORD nSize
    );

WINBOOL
STDCALL
WriteProfileStringW(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    LPCWSTR lpString
    );

DWORD
STDCALL
GetProfileSectionW(
    LPCWSTR lpAppName,
    LPWSTR lpReturnedString,
    DWORD nSize
    );

WINBOOL
STDCALL
WriteProfileSectionW(
    LPCWSTR lpAppName,
    LPCWSTR lpString
    );

UINT
STDCALL
GetPrivateProfileIntW(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    INT nDefault,
    LPCWSTR lpFileName
    );

DWORD
STDCALL
GetPrivateProfileStringW(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    LPCWSTR lpDefault,
    LPWSTR lpReturnedString,
    DWORD nSize,
    LPCWSTR lpFileName
    );

WINBOOL
STDCALL
WritePrivateProfileStringW(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    LPCWSTR lpString,
    LPCWSTR lpFileName
    );

DWORD
STDCALL
GetPrivateProfileSectionW(
    LPCWSTR lpAppName,
    LPWSTR lpReturnedString,
    DWORD nSize,
    LPCWSTR lpFileName
    );

WINBOOL
STDCALL
WritePrivateProfileSectionW(
    LPCWSTR lpAppName,
    LPCWSTR lpString,
    LPCWSTR lpFileName
    );

UINT
STDCALL
GetDriveTypeW(
    LPCWSTR lpRootPathName
    );

UINT
STDCALL
GetSystemDirectoryW(
    LPWSTR lpBuffer,
    UINT uSize
    );

DWORD
STDCALL
GetTempPathW(
    DWORD nBufferLength,
    LPWSTR lpBuffer
    );

UINT
STDCALL
GetTempFileNameW(
    LPCWSTR lpPathName,
    LPCWSTR lpPrefixString,
    UINT uUnique,
    LPWSTR lpTempFileName
    );

UINT
STDCALL
GetWindowsDirectoryW(
    LPWSTR lpBuffer,
    UINT uSize
    );

WINBOOL
STDCALL
SetCurrentDirectoryW(
    LPCWSTR lpPathName
    );

DWORD
STDCALL
GetCurrentDirectoryW(
    DWORD nBufferLength,
    LPWSTR lpBuffer
    );

WINBOOL
STDCALL
GetDiskFreeSpaceW(
    LPCWSTR lpRootPathName,
    LPDWORD lpSectorsPerCluster,
    LPDWORD lpBytesPerSector,
    LPDWORD lpNumberOfFreeClusters,
    LPDWORD lpTotalNumberOfClusters
    );

WINBOOL
STDCALL
GetDiskFreeSpaceExW(
    LPCWSTR lpDirectoryName,
    PULARGE_INTEGER lpFreeBytesAvailableToCaller,
    PULARGE_INTEGER lpTotalNumberOfBytes,
    PULARGE_INTEGER lpTotalNumberOfFreeBytes
    );

WINBOOL
STDCALL
CreateDirectoryW(
    LPCWSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

WINBOOL
STDCALL
CreateDirectoryExW(
    LPCWSTR lpTemplateDirectory,
    LPCWSTR lpNewDirectory,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

WINBOOL
STDCALL
RemoveDirectoryW(
    LPCWSTR lpPathName
    );

DWORD
STDCALL
GetFullPathNameW(
    LPCWSTR lpFileName,
    DWORD nBufferLength,
    LPWSTR lpBuffer,
    LPWSTR *lpFilePart
    );

WINBOOL
STDCALL
DefineDosDeviceW(
    DWORD dwFlags,
    LPCWSTR lpDeviceName,
    LPCWSTR lpTargetPath
    );

DWORD
STDCALL
QueryDosDeviceW(
    LPCWSTR lpDeviceName,
    LPWSTR lpTargetPath,
    DWORD ucchMax
    );

HANDLE
STDCALL
CreateFileW(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    );

WINBOOL
STDCALL
SetFileAttributesW(
    LPCWSTR lpFileName,
    DWORD dwFileAttributes
    );

DWORD
STDCALL
GetFileAttributesW(
    LPCWSTR lpFileName
    );

DWORD
STDCALL
GetCompressedFileSizeW(
    LPCWSTR lpFileName,
    LPDWORD lpFileSizeHigh
    );

WINBOOL
STDCALL
DeleteFileW(
    LPCWSTR lpFileName
    );

DWORD
STDCALL
SearchPathW(
    LPCWSTR lpPath,
    LPCWSTR lpFileName,
    LPCWSTR lpExtension,
    DWORD nBufferLength,
    LPWSTR lpBuffer,
    LPWSTR *lpFilePart
    );

WINBOOL
STDCALL
CopyFileW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    WINBOOL bFailIfExists
    );

WINBOOL
STDCALL
CopyFileExW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    LPPROGRESS_ROUTINE lpProgressRoutine,
    LPVOID lpData,
    LPBOOL pbCancel,
    DWORD dwCopyFlags
    );

WINBOOL
STDCALL
MoveFileW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName
    );

WINBOOL
STDCALL
MoveFileExW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    DWORD dwFlags
    );

WINBOOL
STDCALL
MoveFileWithProgressW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    LPPROGRESS_ROUTINE lpProgressRoutine,
    LPVOID lpData,
    DWORD dwFlags
    );

HANDLE
STDCALL
CreateNamedPipeW(
    LPCWSTR lpName,
    DWORD dwOpenMode,
    DWORD dwPipeMode,
    DWORD nMaxInstances,
    DWORD nOutBufferSize,
    DWORD nInBufferSize,
    DWORD nDefaultTimeOut,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

WINBOOL
STDCALL
GetNamedPipeHandleStateW(
    HANDLE hNamedPipe,
    LPDWORD lpState,
    LPDWORD lpCurInstances,
    LPDWORD lpMaxCollectionCount,
    LPDWORD lpCollectDataTimeout,
    LPWSTR lpUserName,
    DWORD nMaxUserNameSize
    );

WINBOOL
STDCALL
CallNamedPipeW(
    LPCWSTR lpNamedPipeName,
    LPVOID lpInBuffer,
    DWORD nInBufferSize,
    LPVOID lpOutBuffer,
    DWORD nOutBufferSize,
    LPDWORD lpBytesRead,
    DWORD nTimeOut
    );

WINBOOL
STDCALL
WaitNamedPipeW(
    LPCWSTR lpNamedPipeName,
    DWORD nTimeOut
    );

WINBOOL
STDCALL
SetVolumeLabelW(
    LPCWSTR lpRootPathName,
    LPCWSTR lpVolumeName
    );

WINBOOL
STDCALL
GetVolumeInformationW(
    LPCWSTR lpRootPathName,
    LPWSTR lpVolumeNameBuffer,
    DWORD nVolumeNameSize,
    LPDWORD lpVolumeSerialNumber,
    LPDWORD lpMaximumComponentLength,
    LPDWORD lpFileSystemFlags,
    LPWSTR lpFileSystemNameBuffer,
    DWORD nFileSystemNameSize
    );

WINBOOL
STDCALL
ClearEventLogW (
    HANDLE hEventLog,
    LPCWSTR lpBackupFileName
    );

WINBOOL
STDCALL
BackupEventLogW (
    HANDLE hEventLog,
    LPCWSTR lpBackupFileName
    );

HANDLE
STDCALL
OpenEventLogW (
    LPCWSTR lpUNCServerName,
    LPCWSTR lpSourceName
    );

HANDLE
STDCALL
RegisterEventSourceW (
    LPCWSTR lpUNCServerName,
    LPCWSTR lpSourceName
    );

HANDLE
STDCALL
OpenBackupEventLogW (
    LPCWSTR lpUNCServerName,
    LPCWSTR lpFileName
    );

WINBOOL
STDCALL
ReadEventLogW (
     HANDLE     hEventLog,
     DWORD      dwReadFlags,
     DWORD      dwRecordOffset,
     LPVOID     lpBuffer,
     DWORD      nNumberOfBytesToRead,
     DWORD      *pnBytesRead,
     DWORD      *pnMinNumberOfBytesNeeded
    );

WINBOOL
STDCALL
ReportEventW (
     HANDLE     hEventLog,
     WORD       wType,
     WORD       wCategory,
     DWORD      dwEventID,
     PSID       lpUserSid,
     WORD       wNumStrings,
     DWORD      dwDataSize,
     LPCWSTR   *lpStrings,
     LPVOID     lpRawData
    );

WINBOOL
STDCALL
AccessCheckAndAuditAlarmW (
    LPCWSTR SubsystemName,
    LPVOID HandleId,
    LPWSTR ObjectTypeName,
    LPWSTR ObjectName,
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    DWORD DesiredAccess,
    PGENERIC_MAPPING GenericMapping,
    WINBOOL ObjectCreation,
    LPDWORD GrantedAccess,
    LPBOOL AccessStatus,
    LPBOOL pfGenerateOnClose
    );

WINBOOL
STDCALL
ObjectOpenAuditAlarmW (
    LPCWSTR SubsystemName,
    LPVOID HandleId,
    LPWSTR ObjectTypeName,
    LPWSTR ObjectName,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    HANDLE ClientToken,
    DWORD DesiredAccess,
    DWORD GrantedAccess,
    PPRIVILEGE_SET Privileges,
    WINBOOL ObjectCreation,
    WINBOOL AccessGranted,
    LPBOOL GenerateOnClose
    );

WINBOOL
STDCALL
ObjectPrivilegeAuditAlarmW (
    LPCWSTR SubsystemName,
    LPVOID HandleId,
    HANDLE ClientToken,
    DWORD DesiredAccess,
    PPRIVILEGE_SET Privileges,
    WINBOOL AccessGranted
    );

WINBOOL
STDCALL
ObjectCloseAuditAlarmW (
    LPCWSTR SubsystemName,
    LPVOID HandleId,
    WINBOOL GenerateOnClose
    );

WINBOOL
STDCALL
PrivilegedServiceAuditAlarmW (
    LPCWSTR SubsystemName,
    LPCWSTR ServiceName,
    HANDLE ClientToken,
    PPRIVILEGE_SET Privileges,
    WINBOOL AccessGranted
    );

WINBOOL
STDCALL
SetFileSecurityW (
    LPCWSTR lpFileName,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor
    );

WINBOOL
STDCALL
GetFileSecurityW (
    LPCWSTR lpFileName,
    SECURITY_INFORMATION RequestedInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    DWORD nLength,
    LPDWORD lpnLengthNeeded
    );

HANDLE
STDCALL
FindFirstChangeNotificationW(
    LPCWSTR lpPathName,
    WINBOOL bWatchSubtree,
    DWORD dwNotifyFilter
    );

WINBOOL
STDCALL
IsBadStringPtrW(
    LPCWSTR lpsz,
    UINT ucchMax
    );

WINBOOL
STDCALL
LookupAccountSidW(
    LPCWSTR lpSystemName,
    PSID Sid,
    LPWSTR Name,
    LPDWORD cbName,
    LPWSTR ReferencedDomainName,
    LPDWORD cbReferencedDomainName,
    PSID_NAME_USE peUse
    );

WINBOOL
STDCALL
LookupAccountNameW(
    LPCWSTR lpSystemName,
    LPCWSTR lpAccountName,
    PSID Sid,
    LPDWORD cbSid,
    LPWSTR ReferencedDomainName,
    LPDWORD cbReferencedDomainName,
    PSID_NAME_USE peUse
    );

WINBOOL
STDCALL
LookupPrivilegeValueW(
    LPCWSTR lpSystemName,
    LPCWSTR lpName,
    PLUID   lpLuid
    );

WINBOOL
STDCALL
LookupPrivilegeNameW(
    LPCWSTR lpSystemName,
    PLUID   lpLuid,
    LPWSTR lpName,
    LPDWORD cbName
    );

WINBOOL
STDCALL
LookupPrivilegeDisplayNameW(
    LPCWSTR lpSystemName,
    LPCWSTR lpName,
    LPWSTR lpDisplayName,
    LPDWORD cbDisplayName,
    LPDWORD lpLanguageId
    );

WINBOOL
STDCALL
BuildCommDCBW(
    LPCWSTR lpDef,
    LPDCB lpDCB
    );

WINBOOL
STDCALL
BuildCommDCBAndTimeoutsW(
    LPCWSTR lpDef,
    LPDCB lpDCB,
    LPCOMMTIMEOUTS lpCommTimeouts
    );

WINBOOL
STDCALL
CommConfigDialogW(
    LPCWSTR lpszName,
    HWND hWnd,
    LPCOMMCONFIG lpCC
    );

WINBOOL
STDCALL
GetDefaultCommConfigW(
    LPCWSTR lpszName,
    LPCOMMCONFIG lpCC,
    LPDWORD lpdwSize
    );

WINBOOL
STDCALL
SetDefaultCommConfigW(
    LPCWSTR lpszName,
    LPCOMMCONFIG lpCC,
    DWORD dwSize
    );

WINBOOL
STDCALL
GetComputerNameW (
    LPWSTR lpBuffer,
    LPDWORD nSize
    );

WINBOOL
STDCALL
SetComputerNameW (
    LPCWSTR lpComputerName
    );

WINBOOL
STDCALL
GetUserNameW (
    LPWSTR lpBuffer,
    LPDWORD nSize
    );

int
STDCALL
wvsprintfW(
    LPWSTR,
    LPCWSTR,
    va_list arglist);

int
CDECL
wsprintfW(LPWSTR, LPCWSTR, ...);

HKL
STDCALL
LoadKeyboardLayoutW(
    LPCWSTR pwszKLID,
    UINT Flags);

WINBOOL
STDCALL
GetKeyboardLayoutNameW(
    LPWSTR pwszKLID);

HDESK
STDCALL
CreateDesktopW(
    LPCWSTR lpszDesktop,
    LPCWSTR lpszDevice,
    LPDEVMODEW pDevmode,
    DWORD dwFlags,
    ACCESS_MASK dwDesiredAccess,
    LPSECURITY_ATTRIBUTES lpsa);

HDESK
STDCALL
OpenDesktopW(
    LPWSTR lpszDesktop,
    DWORD dwFlags,
    WINBOOL fInherit,
    DWORD dwDesiredAccess);

WINBOOL
STDCALL
EnumDesktopsW(
    HWINSTA hwinsta,
    DESKTOPENUMPROC lpEnumFunc,
    LPARAM lParam);

HWINSTA
STDCALL
CreateWindowStationW(
    LPWSTR lpwinsta,
    DWORD dwReserved,
    DWORD dwDesiredAccess,
    LPSECURITY_ATTRIBUTES lpsa);

HWINSTA
STDCALL
OpenWindowStationW(
    LPWSTR lpszWinSta,
    WINBOOL fInherit,
    DWORD dwDesiredAccess);

WINBOOL
STDCALL
EnumWindowStationsW(
    ENUMWINDOWSTATIONPROC lpEnumFunc,
    LPARAM lParam);

WINBOOL
STDCALL
GetUserObjectInformationW(
    HANDLE hObj,
    int nIndex,
    PVOID pvInfo,
    DWORD nLength,
    LPDWORD lpnLengthNeeded);

WINBOOL
STDCALL
SetUserObjectInformationW(
    HANDLE hObj,
    int nIndex,
    PVOID pvInfo,
    DWORD nLength);

UINT
STDCALL
RegisterWindowMessageW(
    LPCWSTR lpString);

WINBOOL
STDCALL
GetMessageW(
    LPMSG lpMsg,
    HWND hWnd ,
    UINT wMsgFilterMin,
    UINT wMsgFilterMax);

LRESULT
STDCALL
DispatchMessageW(
    CONST MSG *lpMsg);

WINBOOL
STDCALL
PeekMessageW(
    LPMSG lpMsg,
    HWND hWnd ,
    UINT wMsgFilterMin,
    UINT wMsgFilterMax,
    UINT wRemoveMsg);

LRESULT
STDCALL
SendMessageW(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);

LRESULT
STDCALL
SendMessageTimeoutW(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam,
    UINT fuFlags,
    UINT uTimeout,
    PDWORD_PTR lpdwResult);

WINBOOL
STDCALL
SendNotifyMessageW(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);

WINBOOL
STDCALL
SendMessageCallbackW(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam,
    SENDASYNCPROC lpResultCallBack,
    ULONG_PTR dwData);

WINBOOL
STDCALL
PostMessageW(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);

WINBOOL
STDCALL
PostThreadMessageW(
    DWORD idThread,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);

LRESULT
STDCALL
DefWindowProcW(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);

LRESULT
STDCALL
CallWindowProcW(
    WNDPROC lpPrevWndFunc,
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);

ATOM
STDCALL
RegisterClassW(
    CONST WNDCLASS *lpWndClass);

WINBOOL
STDCALL
UnregisterClassW(
    LPCWSTR lpClassName,
    HINSTANCE hInstance);

WINBOOL
STDCALL
GetClassInfoW(
    HINSTANCE hInstance ,
    LPCWSTR lpClassName,
    LPWNDCLASS lpWndClass);

ATOM
STDCALL
RegisterClassExW(CONST WNDCLASSEX *);

WINBOOL
STDCALL
GetClassInfoExW(HINSTANCE, LPCWSTR, LPWNDCLASSEX);

HWND
STDCALL
CreateWindowExW(
    DWORD dwExStyle,
    LPCWSTR lpClassName,
    LPCWSTR lpWindowName,
    DWORD dwStyle,
    int X,
    int Y,
    int nWidth,
    int nHeight,
    HWND hWndParent ,
    HMENU hMenu,
    HINSTANCE hInstance,
    LPVOID lpParam);

HWND
STDCALL
CreateDialogParamW(
    HINSTANCE hInstance,
    LPCWSTR lpTemplateName,
    HWND hWndParent ,
    DLGPROC lpDialogFunc,
    LPARAM dwInitParam);

HWND
STDCALL
CreateDialogIndirectParamW(
    HINSTANCE hInstance,
    LPCDLGTEMPLATE lpTemplate,
    HWND hWndParent,
    DLGPROC lpDialogFunc,
    LPARAM dwInitParam);

int
STDCALL
DialogBoxParamW(
    HINSTANCE hInstance,
    LPCWSTR lpTemplateName,
    HWND hWndParent ,
    DLGPROC lpDialogFunc,
    LPARAM dwInitParam);

int
STDCALL
DialogBoxIndirectParamW(
    HINSTANCE hInstance,
    LPCDLGTEMPLATE hDialogTemplate,
    HWND hWndParent ,
    DLGPROC lpDialogFunc,
    LPARAM dwInitParam);

WINBOOL
STDCALL
SetDlgItemTextW(
    HWND hDlg,
    int nIDDlgItem,
    LPCWSTR lpString);

UINT
STDCALL
GetDlgItemTextW(
    HWND hDlg,
    int nIDDlgItem,
    LPWSTR lpString,
    int nMaxCount);

LRESULT
STDCALL
SendDlgItemMessageW(
    HWND hDlg,
    int nIDDlgItem,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);

LRESULT
STDCALL
DefDlgProcW(
    HWND hDlg,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);

WINBOOL
STDCALL
CallMsgFilterW(
    LPMSG lpMsg,
    int nCode);

UINT
STDCALL
RegisterClipboardFormatW(
    LPCWSTR lpszFormat);

int
STDCALL
GetClipboardFormatNameW(
    UINT format,
    LPWSTR lpszFormatName,
    int cchMaxCount);

WINBOOL
STDCALL
CharToOemW(
    LPCWSTR lpszSrc,
    LPSTR lpszDst);

WINBOOL
STDCALL
OemToCharW(
    LPCSTR lpszSrc,
    LPWSTR lpszDst);

WINBOOL
STDCALL
CharToOemBuffW(
    LPCWSTR lpszSrc,
    LPSTR lpszDst,
    DWORD cchDstLength);

WINBOOL
STDCALL
OemToCharBuffW(
    LPCSTR lpszSrc,
    LPWSTR lpszDst,
    DWORD cchDstLength);

LPWSTR
STDCALL
CharUpperW(
    LPWSTR lpsz);

DWORD
STDCALL
CharUpperBuffW(
    LPWSTR lpsz,
    DWORD cchLength);

LPWSTR
STDCALL
CharLowerW(
    LPWSTR lpsz);

DWORD
STDCALL
CharLowerBuffW(
    LPWSTR lpsz,
    DWORD cchLength);

LPWSTR
STDCALL
CharNextW(
    LPCWSTR lpsz);

LPWSTR
STDCALL
CharPrevW(
    LPCWSTR lpszStart,
    LPCWSTR lpszCurrent);

WINBOOL
STDCALL
IsCharAlphaW(
    WCHAR ch);

WINBOOL
STDCALL
IsCharAlphaNumericW(
    WCHAR ch);

WINBOOL
STDCALL
IsCharUpperW(
    WCHAR ch);

WINBOOL
STDCALL
IsCharLowerW(
    WCHAR ch);

int
STDCALL
GetKeyNameTextW(
    LONG lParam,
    LPWSTR lpString,
    int nSize
    );

SHORT
STDCALL
VkKeyScanW(
    WCHAR ch);

SHORT
STDCALL VkKeyScanExW(
    WCHAR  ch,
    HKL   dwhkl);

UINT
STDCALL
MapVirtualKeyW(
    UINT uCode,
    UINT uMapType);

UINT
STDCALL
MapVirtualKeyExW(
    UINT uCode,
    UINT uMapType,
    HKL dwhkl);

HACCEL
STDCALL
LoadAcceleratorsW(
    HINSTANCE hInstance,
    LPCWSTR lpTableName);

HACCEL
STDCALL
CreateAcceleratorTableW(
    LPACCEL, int);

int
STDCALL
CopyAcceleratorTableW(
    HACCEL hAccelSrc,
    LPACCEL lpAccelDst,
    int cAccelEntries);

int
STDCALL
TranslateAcceleratorW(
    HWND hWnd,
    HACCEL hAccTable,
    LPMSG lpMsg);

HMENU
STDCALL
LoadMenuW(
    HINSTANCE hInstance,
    LPCWSTR lpMenuName);

HMENU
STDCALL
LoadMenuIndirectW(
    CONST MENUTEMPLATE *lpMenuTemplate);

WINBOOL
STDCALL
ChangeMenuW(
    HMENU hMenu,
    UINT cmd,
    LPCWSTR lpszNewItem,
    UINT cmdInsert,
    UINT flags);

int
STDCALL
GetMenuStringW(
    HMENU hMenu,
    UINT uIDItem,
    LPWSTR lpString,
    int nMaxCount,
    UINT uFlag);

WINBOOL
STDCALL
InsertMenuW(
    HMENU hMenu,
    UINT uPosition,
    UINT uFlags,
    UINT uIDNewItem,
    LPCWSTR lpNewItem
    );

WINBOOL
STDCALL
AppendMenuW(
    HMENU hMenu,
    UINT uFlags,
    UINT uIDNewItem,
    LPCWSTR lpNewItem
    );

WINBOOL
STDCALL
ModifyMenuW(
    HMENU hMnu,
    UINT uPosition,
    UINT uFlags,
    UINT uIDNewItem,
    LPCWSTR lpNewItem
    );

WINBOOL
STDCALL
InsertMenuItemW(
    HMENU,
    UINT,
    WINBOOL,
    LPCMENUITEMINFO
    );

WINBOOL
STDCALL
GetMenuItemInfoW(
    HMENU,
    UINT,
    WINBOOL,
    LPMENUITEMINFO
    );

WINBOOL
STDCALL
SetMenuItemInfoW(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPMENUITEMINFO lpmii);

int
STDCALL
DrawTextW(
    HDC hDC,
    LPCWSTR lpString,
    int nCount,
    LPRECT lpRect,
    UINT uFormat);

int
STDCALL
DrawTextExW(HDC, LPWSTR, int, LPRECT, UINT, LPDRAWTEXTPARAMS);

WINBOOL
STDCALL
GrayStringW(
    HDC hDC,
    HBRUSH hBrush,
    GRAYSTRINGPROC lpOutputFunc,
    LPARAM lpData,
    int nCount,
    int X,
    int Y,
    int nWidth,
    int nHeight);

WINBOOL STDCALL DrawStateW(HDC, HBRUSH, DRAWSTATEPROC, LPARAM, WPARAM, int, int, int, int, UINT);

LONG
STDCALL
TabbedTextOutW(
    HDC hDC,
    int X,
    int Y,
    LPCWSTR lpString,
    int nCount,
    int nTabPositions,
    LPINT lpnTabStopPositions,
    int nTabOrigin);

DWORD
STDCALL
GetTabbedTextExtentW(
    HDC hDC,
    LPCWSTR lpString,
    int nCount,
    int nTabPositions,
    LPINT lpnTabStopPositions);

WINBOOL
STDCALL
SetPropW(
    HWND hWnd,
    LPCWSTR lpString,
    HANDLE hData);

HANDLE
STDCALL
GetPropW(
    HWND hWnd,
    LPCWSTR lpString);

HANDLE
STDCALL
RemovePropW(
    HWND hWnd,
    LPCWSTR lpString);

int
STDCALL
EnumPropsExW(
    HWND hWnd,
    PROPENUMPROCEX lpEnumFunc,
    LPARAM lParam);

int
STDCALL
EnumPropsW(
    HWND hWnd,
    PROPENUMPROC lpEnumFunc);

WINBOOL
STDCALL
SetWindowTextW(
    HWND hWnd,
    LPCWSTR lpString);

int
STDCALL
GetWindowTextW(
    HWND hWnd,
    LPWSTR lpString,
    int nMaxCount);

int
STDCALL
GetWindowTextLengthW(
    HWND hWnd);

int
STDCALL
MessageBoxW(
    HWND hWnd ,
    LPCWSTR lpText,
    LPCWSTR lpCaption,
    UINT uType);

int
STDCALL
MessageBoxExW(
    HWND hWnd ,
    LPCWSTR lpText,
    LPCWSTR lpCaption,
    UINT uType,
    WORD wLanguageId);

int
STDCALL
MessageBoxIndirectW(LPMSGBOXPARAMS);

LONG
STDCALL
GetWindowLongW(
    HWND hWnd,
    int nIndex);

LONG
STDCALL
SetWindowLongW(
    HWND hWnd,
    int nIndex,
    LONG dwNewLong);

DWORD
STDCALL
GetClassLongW(
    HWND hWnd,
    int nIndex);

DWORD
STDCALL
SetClassLongW(
    HWND hWnd,
    int nIndex,
    LONG dwNewLong);

HWND
STDCALL
FindWindowW(
    LPCWSTR lpClassName ,
    LPCWSTR lpWindowName);

HWND
STDCALL
FindWindowExW(HWND, HWND, LPCWSTR, LPCWSTR);

int
STDCALL
GetClassNameW(
    HWND hWnd,
    LPWSTR lpClassName,
    int nMaxCount);

HHOOK
STDCALL
SetWindowsHookExW(
    int idHook,
    HOOKPROC lpfn,
    HINSTANCE hmod,
    DWORD dwThreadId);

HBITMAP
STDCALL
LoadBitmapW(
    HINSTANCE hInstance,
    LPCWSTR lpBitmapName);

HCURSOR
STDCALL
LoadCursorW(
    HINSTANCE hInstance,
    LPCWSTR lpCursorName);

HCURSOR
STDCALL
LoadCursorFromFileW(
    LPCWSTR    lpFileName);

HICON
STDCALL
LoadIconW(
    HINSTANCE hInstance,
    LPCWSTR lpIconName);

HANDLE
STDCALL
LoadImageW(
    HINSTANCE,
    LPCWSTR,
    UINT,
    int,
    int,
    UINT);

int
STDCALL
LoadStringW(
    HINSTANCE hInstance,
    UINT uID,
    LPWSTR lpBuffer,
    int nBufferMax);

WINBOOL
STDCALL
IsDialogMessageW(
    HWND hDlg,
    LPMSG lpMsg);

int
STDCALL
DlgDirListW(
    HWND hDlg,
    LPWSTR lpPathSpec,
    int nIDListBox,
    int nIDStaticPath,
    UINT uFileType);

WINBOOL
STDCALL
DlgDirSelectExW(
    HWND hDlg,
    LPWSTR lpString,
    int nCount,
    int nIDListBox);

int
STDCALL
DlgDirListComboBoxW(
    HWND hDlg,
    LPWSTR lpPathSpec,
    int nIDComboBox,
    int nIDStaticPath,
    UINT uFiletype);

WINBOOL
STDCALL
DlgDirSelectComboBoxExW(
    HWND hDlg,
    LPWSTR lpString,
    int nCount,
    int nIDComboBox);

LRESULT
STDCALL
DefFrameProcW(
    HWND hWnd,
    HWND hWndMDIClient ,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

LRESULT
STDCALL
DefMDIChildProcW(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

HWND
STDCALL
CreateMDIWindowW(
    LPCWSTR lpClassName,
    LPCWSTR lpWindowName,
    DWORD dwStyle,
    int X,
    int Y,
    int nWidth,
    int nHeight,
    HWND hWndParent,
    HINSTANCE hInstance,
    LPARAM lParam
    );

WINBOOL
STDCALL
WinHelpW(
    HWND hWndMain,
    LPCWSTR lpszHelp,
    UINT uCommand,
    DWORD dwData
    );

LONG
STDCALL
ChangeDisplaySettingsW(
    LPDEVMODEW lpDevMode,
    DWORD dwFlags);

WINBOOL
STDCALL
EnumDisplaySettingsW(
    LPCWSTR lpszDeviceName,
    DWORD iModeNum,
    LPDEVMODEW lpDevMode);

WINBOOL
STDCALL
SystemParametersInfoW(
    UINT uiAction,
    UINT uiParam,
    PVOID pvParam,
    UINT fWinIni);

int
STDCALL
AddFontResourceW(LPCWSTR);

HMETAFILE
STDCALL
CopyMetaFileW(HMETAFILE, LPCWSTR);

HFONT
STDCALL
CreateFontIndirectW(CONST LOGFONT *);

HFONT
STDCALL
CreateFontW(int, int, int, int, int, DWORD,
                             DWORD, DWORD, DWORD, DWORD, DWORD,
                             DWORD, DWORD, LPCWSTR);

HDC
STDCALL
CreateICW(LPCWSTR, LPCWSTR , LPCWSTR , CONST DEVMODEW *);

HDC
STDCALL
CreateMetaFileW(LPCWSTR);

WINBOOL
STDCALL
CreateScalableFontResourceW(DWORD, LPCWSTR, LPCWSTR, LPCWSTR);

int
STDCALL
DeviceCapabilitiesW(LPCWSTR, LPCWSTR, WORD,
                                LPWSTR, CONST DEVMODEW *);

int
STDCALL
EnumFontFamiliesExW(HDC, LPLOGFONT, FONTENUMEXPROC, LPARAM, DWORD);

int
STDCALL
EnumFontFamiliesW(HDC, LPCWSTR, FONTENUMPROC, LPARAM);

int
STDCALL
EnumFontsW(HDC, LPCWSTR,  ENUMFONTSPROC, LPARAM);

WINBOOL
STDCALL
GetCharWidthW(HDC, UINT, UINT, LPINT);

WINBOOL
STDCALL 
GetCharWidth32W(HDC, UINT, UINT, LPINT);

WINBOOL
STDCALL
GetCharWidthFloatW(HDC, UINT, UINT, PFLOAT);

WINBOOL
STDCALL
GetCharABCWidthsW(HDC, UINT, UINT, LPABC);

WINBOOL
STDCALL
GetCharABCWidthsFloatW(HDC, UINT, UINT, LPABCFLOAT);

DWORD
STDCALL
GetGlyphOutlineW(HDC, UINT, UINT, LPGLYPHMETRICS, DWORD, LPVOID, CONST MAT2 *);

HMETAFILE
STDCALL
GetMetaFileW(LPCWSTR);

UINT
STDCALL
GetOutlineTextMetricsW(HDC, UINT, LPOUTLINETEXTMETRICW);

WINBOOL
STDCALL GetTextExtentPointW(
                    HDC,
                    LPCWSTR,
                    int,
                    LPSIZE
                    );

WINBOOL
STDCALL
GetTextExtentPoint32W(
                    HDC,
                    LPCWSTR,
                    int,
                    LPSIZE
                    );

WINBOOL
STDCALL
GetTextExtentExPointW(
                    HDC,
                    LPCWSTR,
                    int,
                    int,
                    LPINT,
                    LPINT,
                    LPSIZE
                    );

DWORD
STDCALL
GetCharacterPlacementW(HDC, LPCWSTR, int, int, LPGCP_RESULTS, DWORD);

HDC
STDCALL
ResetDCW(HDC, CONST DEVMODEW *);

WINBOOL
STDCALL
RemoveFontResourceW(LPCWSTR);

HENHMETAFILE
STDCALL
CopyEnhMetaFileW(HENHMETAFILE, LPCWSTR);

HDC
STDCALL
CreateEnhMetaFileW(HDC, LPCWSTR, CONST RECT *, LPCWSTR);

HENHMETAFILE
STDCALL
GetEnhMetaFileW(LPCWSTR);

UINT
STDCALL
GetEnhMetaFileDescriptionW(HENHMETAFILE, UINT, LPWSTR );

WINBOOL
STDCALL
GetTextMetricsW(HDC, LPTEXTMETRICW);

int
STDCALL
StartDocW(HDC, CONST DOCINFO *);

int
STDCALL
GetObjectW(HGDIOBJ, int, LPVOID);

WINBOOL
STDCALL
TextOutW(HDC, int, int, LPCWSTR, int);

WINBOOL
STDCALL
ExtTextOutW(HDC, int, int, UINT, CONST RECT *,LPCWSTR, UINT, CONST INT *);

WINBOOL
STDCALL
PolyTextOutW(HDC, CONST POLYTEXT *, int);

int
STDCALL
GetTextFaceW(HDC, int, LPWSTR);

DWORD
STDCALL
GetKerningPairsW(HDC, DWORD, LPKERNINGPAIR);

WINBOOL
STDCALL
GetLogColorSpaceW(HCOLORSPACE,LPLOGCOLORSPACE,DWORD);

HCOLORSPACE
STDCALL
CreateColorSpaceW(LPLOGCOLORSPACE);

WINBOOL
STDCALL
GetICMProfileW(HDC,DWORD,LPWSTR);

WINBOOL
STDCALL
SetICMProfileW(HDC,LPWSTR);

WINBOOL
STDCALL
UpdateICMRegKeyW(DWORD, DWORD, LPWSTR, UINT);

int
STDCALL
EnumICMProfilesW(HDC,ICMENUMPROC,LPARAM);

HPROPSHEETPAGE
STDCALL
CreatePropertySheetPageW(LPCPROPSHEETPAGE lppsp);

int
STDCALL
PropertySheetW(LPCPROPSHEETHEADER lppsph);

HIMAGELIST
STDCALL
ImageList_LoadImageW(HINSTANCE hi, 
LPCWSTR lpbmp, 
int cx, 
int cGrow, 
COLORREF crMask, 
UINT uType, 
UINT uFlags);

HWND
STDCALL
CreateStatusWindowW(LONG style, LPCWSTR lpszText, HWND hwndParent, UINT wID);

void
STDCALL
DrawStatusTextW(HDC hDC, LPRECT lprc, LPCWSTR pszText, UINT uFlags);

WINBOOL
STDCALL
GetOpenFileNameW(LPOPENFILENAME);

WINBOOL
STDCALL
GetSaveFileNameW(LPOPENFILENAME);

short
STDCALL
GetFileTitleW(LPCWSTR, LPWSTR, WORD);

WINBOOL
STDCALL
ChooseColorW(LPCHOOSECOLOR);

HWND
STDCALL
ReplaceTextW(LPFINDREPLACE);

WINBOOL
STDCALL
ChooseFontW(LPCHOOSEFONT);

HWND
STDCALL
FindTextW(LPFINDREPLACE);

WINBOOL
STDCALL
PrintDlgW(LPPRINTDLG);

WINBOOL
STDCALL
PageSetupDlgW(LPPAGESETUPDLG);

WINBOOL
STDCALL
CreateProcessW(
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    WINBOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    );

VOID
STDCALL
GetStartupInfoW(
    LPSTARTUPINFOW lpStartupInfo
    );

WINBASEAPI
HANDLE
WINAPI
FindFirstFileExW (
	LPCWSTR			lpFileName,
	FINDEX_INFO_LEVELS	fInfoLevelId,
	LPVOID			lpFindFileData,
	FINDEX_SEARCH_OPS	fSearchOp,
	LPVOID			lpSearchFilter,
	DWORD			dwAdditionalFlags
	);

HANDLE
STDCALL
FindFirstFileW(
    LPCWSTR lpFileName,
    LPWIN32_FIND_DATAW lpFindFileData
    );

WINBOOL
STDCALL
FindNextFileW(
    HANDLE hFindFile,
    LPWIN32_FIND_DATAW lpFindFileData
    );

WINBOOL
STDCALL
GetVersionExW(
    LPOSVERSIONINFOW lpVersionInformation
    );

#define CreateWindowW(lpClassName, lpWindowName, dwStyle, x, y,\
nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam)\
CreateWindowExW(0L, lpClassName, lpWindowName, dwStyle, x, y,\
nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam)

#define CreateDialogW(hInstance, lpName, hWndParent, lpDialogFunc) \
CreateDialogParamW(hInstance, lpName, hWndParent, lpDialogFunc, 0L)

#define CreateDialogIndirectW(hInstance, lpTemplate, hWndParent, lpDialogFunc) \
CreateDialogIndirectParamW(hInstance, lpTemplate, hWndParent, lpDialogFunc, 0L)

#define DialogBoxW(hInstance, lpTemplate, hWndParent, lpDialogFunc) \
DialogBoxParamW(hInstance, lpTemplate, hWndParent, lpDialogFunc, 0L)

#define DialogBoxIndirectW(hInstance, lpTemplate, hWndParent, lpDialogFunc) \
DialogBoxIndirectParamW(hInstance, lpTemplate, hWndParent, lpDialogFunc, 0L)

HDC
STDCALL
CreateDCW(LPCWSTR, LPCWSTR , LPCWSTR , CONST DEVMODEW *);

HFONT
STDCALL
CreateFontA(int, int, int, int, int, DWORD,
                             DWORD, DWORD, DWORD, DWORD, DWORD,
                             DWORD, DWORD, LPCSTR);

DWORD
STDCALL
VerInstallFileW(
        DWORD uFlags,
        LPWSTR szSrcFileName,
        LPWSTR szDestFileName,
        LPWSTR szSrcDir,
        LPWSTR szDestDir,
        LPWSTR szCurDir,
        LPWSTR szTmpFile,
        PUINT lpuTmpFileLen
        );

DWORD
STDCALL
GetFileVersionInfoSizeW(
        LPWSTR lptstrFilename,
        LPDWORD lpdwHandle
        );

WINBOOL
STDCALL
GetFileVersionInfoW(
        LPWSTR lptstrFilename,
        DWORD dwHandle,
        DWORD dwLen,
        LPVOID lpData
        );

DWORD
STDCALL
VerLanguageNameW(
        DWORD wLang,
        LPWSTR szLang,
        DWORD nSize
        );

WINBOOL
STDCALL
VerQueryValueW(
        const LPVOID pBlock,
        LPWSTR lpSubBlock,
        LPVOID * lplpBuffer,
        PUINT puLen
        );

DWORD
STDCALL
VerFindFileW(
        DWORD uFlags,
        LPWSTR szFileName,
        LPWSTR szWinDir,
        LPWSTR szAppDir,
        LPWSTR szCurDir,
        PUINT lpuCurDirLen,
        LPWSTR szDestDir,
        PUINT lpuDestDirLen
        );

LONG
STDCALL
RegSetValueExW (
    HKEY hKey,
    LPCWSTR lpValueName,
    DWORD Reserved,
    DWORD dwType,
    CONST BYTE* lpData,
    DWORD cbData
    );

LONG
STDCALL
RegUnLoadKeyW (
    HKEY    hKey,
    LPCWSTR lpSubKey
    );

WINBOOL
STDCALL
InitiateSystemShutdownW(
    LPWSTR lpMachineName,
    LPWSTR lpMessage,
    DWORD dwTimeout,
    WINBOOL bForceAppsClosed,
    WINBOOL bRebootAfterShutdown
    );

WINBOOL
STDCALL
AbortSystemShutdownW(
    LPCWSTR lpMachineName
    );

LONG
STDCALL
RegRestoreKeyW (
    HKEY hKey,
    LPCWSTR lpFile,
    DWORD   dwFlags
    );

LONG
STDCALL
RegSaveKeyW (
    HKEY hKey,
    LPCWSTR lpFile,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

LONG
STDCALL
RegSetValueW (
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD dwType,
    LPCWSTR lpData,
    DWORD cbData
    );

LONG
STDCALL
RegQueryValueW (
    HKEY hKey,
    LPCWSTR lpSubKey,
    LPWSTR lpValue,
    PLONG   lpcbValue
    );

LONG
STDCALL
RegQueryMultipleValuesW (
    HKEY hKey,
    PVALENTW val_list,
    DWORD num_vals,
    LPWSTR lpValueBuf,
    LPDWORD ldwTotsize
    );

LONG
STDCALL
RegQueryValueExW (
    HKEY hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );

LONG
STDCALL
RegReplaceKeyW (
    HKEY     hKey,
    LPCWSTR  lpSubKey,
    LPCWSTR  lpNewFile,
    LPCWSTR  lpOldFile
    );

LONG
STDCALL
RegConnectRegistryW (
    LPCWSTR lpMachineName,
    HKEY hKey,
    PHKEY phkResult
    );

LONG
STDCALL
RegCreateKeyW (
    HKEY hKey,
    LPCWSTR lpSubKey,
    PHKEY phkResult
    );

LONG
STDCALL
RegCreateKeyExW (
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD Reserved,
    LPWSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition
    );

LONG
STDCALL
RegDeleteKeyW (
    HKEY hKey,
    LPCWSTR lpSubKey
    );

LONG
STDCALL
RegDeleteValueW (
    HKEY hKey,
    LPCWSTR lpValueName
    );

LONG
STDCALL
RegEnumKeyW (
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpName,
    DWORD cbName
    );

LONG
STDCALL
RegEnumKeyExW (
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpName,
    LPDWORD lpcbName,
    LPDWORD lpReserved,
    LPWSTR lpClass,
    LPDWORD lpcbClass,
    PFILETIME lpftLastWriteTime
    );

LONG
STDCALL
RegEnumValueW (
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpValueName,
    LPDWORD lpcbValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );

LONG
STDCALL
RegLoadKeyW (
    HKEY    hKey,
    LPCWSTR  lpSubKey,
    LPCWSTR  lpFile
    );

LONG
STDCALL
RegOpenKeyW (
    HKEY hKey,
    LPCWSTR lpSubKey,
    PHKEY phkResult
    );

LONG
STDCALL
RegOpenKeyExW (
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
    );

LONG
STDCALL
RegQueryInfoKeyW (
    HKEY hKey,
    LPWSTR lpClass,
    LPDWORD lpcbClass,
    LPDWORD lpReserved,
    LPDWORD lpcSubKeys,
    LPDWORD lpcbMaxSubKeyLen,
    LPDWORD lpcbMaxClassLen,
    LPDWORD lpcValues,
    LPDWORD lpcbMaxValueNameLen,
    LPDWORD lpcbMaxValueLen,
    LPDWORD lpcbSecurityDescriptor,
    PFILETIME lpftLastWriteTime
    );

int
STDCALL
CompareStringW(
    LCID     Locale,
    DWORD    dwCmpFlags,
    LPCWSTR lpString1,
    int      cchCount1,
    LPCWSTR lpString2,
    int      cchCount2);

int
STDCALL
LCMapStringW(
    LCID     Locale,
    DWORD    dwMapFlags,
    LPCWSTR lpSrcStr,
    int      cchSrc,
    LPWSTR  lpDestStr,
    int      cchDest);


int
STDCALL
GetLocaleInfoW(
    LCID     Locale,
    LCTYPE   LCType,
    LPWSTR  lpLCData,
    int      cchData);

WINBOOL
STDCALL
SetLocaleInfoW(
    LCID     Locale,
    LCTYPE   LCType,
    LPCWSTR lpLCData);

int
STDCALL
GetTimeFormatW(
    LCID     Locale,
    DWORD    dwFlags,
    CONST SYSTEMTIME *lpTime,
    LPCWSTR lpFormat,
    LPWSTR  lpTimeStr,
    int      cchTime);

int
STDCALL
GetDateFormatW(
    LCID     Locale,
    DWORD    dwFlags,
    CONST SYSTEMTIME *lpDate,
    LPCWSTR lpFormat,
    LPWSTR  lpDateStr,
    int      cchDate);

int
STDCALL
GetNumberFormatW(
    LCID     Locale,
    DWORD    dwFlags,
    LPCWSTR lpValue,
    CONST NUMBERFMT *lpFormat,
    LPWSTR  lpNumberStr,
    int      cchNumber);

int
STDCALL
GetCurrencyFormatW(
    LCID     Locale,
    DWORD    dwFlags,
    LPCWSTR lpValue,
    CONST CURRENCYFMT *lpFormat,
    LPWSTR  lpCurrencyStr,
    int      cchCurrency);

WINBOOL
STDCALL
EnumCalendarInfoW(
    CALINFO_ENUMPROC lpCalInfoEnumProc,
    LCID              Locale,
    CALID             Calendar,
    CALTYPE           CalType);

WINBOOL
STDCALL
EnumTimeFormatsW(
    TIMEFMT_ENUMPROC lpTimeFmtEnumProc,
    LCID              Locale,
    DWORD             dwFlags);

WINBOOL
STDCALL
EnumDateFormatsW(
    DATEFMT_ENUMPROC lpDateFmtEnumProc,
    LCID              Locale,
    DWORD             dwFlags);

WINBOOL
STDCALL
GetStringTypeExW(
    LCID     Locale,
    DWORD    dwInfoType,
    LPCWSTR lpSrcStr,
    int      cchSrc,
    LPWORD   lpCharType);

WINBOOL
STDCALL
GetStringTypeW(
    DWORD    dwInfoType,
    LPCWSTR  lpSrcStr,
    int      cchSrc,
    LPWORD   lpCharType);

int
STDCALL
FoldStringW(
    DWORD    dwMapFlags,
    LPCWSTR lpSrcStr,
    int      cchSrc,
    LPWSTR  lpDestStr,
    int      cchDest);

WINBOOL
STDCALL
EnumSystemLocalesW(
    LOCALE_ENUMPROC lpLocaleEnumProc,
    DWORD            dwFlags);

WINBOOL
STDCALL
EnumSystemCodePagesW(
    CODEPAGE_ENUMPROC lpCodePageEnumProc,
    DWORD              dwFlags);

WINBOOL
STDCALL
PeekConsoleInputW(
    HANDLE hConsoleInput,
    PINPUT_RECORD lpBuffer,
    DWORD nLength,
    LPDWORD lpNumberOfEventsRead
    );

WINBOOL
STDCALL
ReadConsoleInputW(
    HANDLE hConsoleInput,
    PINPUT_RECORD lpBuffer,
    DWORD nLength,
    LPDWORD lpNumberOfEventsRead
    );

WINBOOL
STDCALL
WriteConsoleInputW(
    HANDLE hConsoleInput,
    CONST INPUT_RECORD *lpBuffer,
    DWORD nLength,
    LPDWORD lpNumberOfEventsWritten
    );

WINBOOL
STDCALL
ReadConsoleOutputW(
    HANDLE hConsoleOutput,
    PCHAR_INFO lpBuffer,
    COORD dwBufferSize,
    COORD dwBufferCoord,
    PSMALL_RECT lpReadRegion
    );

WINBOOL
STDCALL
WriteConsoleOutputW(
    HANDLE hConsoleOutput,
    CONST CHAR_INFO *lpBuffer,
    COORD dwBufferSize,
    COORD dwBufferCoord,
    PSMALL_RECT lpWriteRegion
    );

WINBOOL
STDCALL
ReadConsoleOutputCharacterW(
    HANDLE hConsoleOutput,
    LPWSTR lpCharacter,
    DWORD nLength,
    COORD dwReadCoord,
    LPDWORD lpNumberOfCharsRead
    );

WINBOOL
STDCALL
WriteConsoleOutputCharacterW(
    HANDLE hConsoleOutput,
    LPCWSTR lpCharacter,
    DWORD nLength,
    COORD dwWriteCoord,
    LPDWORD lpNumberOfCharsWritten
    );

WINBOOL
STDCALL
FillConsoleOutputCharacterW(
    HANDLE hConsoleOutput,
    WCHAR  cCharacter,
    DWORD  nLength,
    COORD  dwWriteCoord,
    LPDWORD lpNumberOfCharsWritten
    );

WINBOOL
STDCALL
ScrollConsoleScreenBufferW(
    HANDLE hConsoleOutput,
    CONST SMALL_RECT *lpScrollRectangle,
    CONST SMALL_RECT *lpClipRectangle,
    COORD dwDestinationOrigin,
    CONST CHAR_INFO *lpFill
    );

DWORD
STDCALL
GetConsoleTitleW(
    LPWSTR lpConsoleTitle,
    DWORD nSize
    );

WINBOOL
STDCALL
SetConsoleTitleW(
    LPCWSTR lpConsoleTitle
    );

WINBOOL
STDCALL
ReadConsoleW(
    HANDLE hConsoleInput,
    LPVOID lpBuffer,
    DWORD nNumberOfCharsToRead,
    LPDWORD lpNumberOfCharsRead,
    LPVOID lpReserved
    );

WINBOOL
STDCALL
WriteConsoleW(
    HANDLE hConsoleOutput,
    CONST VOID *lpBuffer,
    DWORD nNumberOfCharsToWrite,
    LPDWORD lpNumberOfCharsWritten,
    LPVOID lpReserved
    );

DWORD STDCALL
WNetAddConnectionW(
     LPCWSTR   lpRemoteName,
     LPCWSTR   lpPassword,
     LPCWSTR   lpLocalName
    );

DWORD STDCALL
WNetAddConnection2W(
     LPNETRESOURCE lpNetResource,
     LPCWSTR       lpPassword,
     LPCWSTR       lpUserName,
     DWORD          dwFlags
    );

DWORD STDCALL
WNetAddConnection3W(
     HWND           hwndOwner,
     LPNETRESOURCE lpNetResource,
     LPCWSTR       lpPassword,
     LPCWSTR       lpUserName,
     DWORD          dwFlags
    );

DWORD STDCALL
WNetCancelConnectionW(
     LPCWSTR lpName,
     WINBOOL     fForce
    );

DWORD STDCALL
WNetCancelConnection2W(
     LPCWSTR lpName,
     DWORD    dwFlags,
     WINBOOL     fForce
    );

DWORD STDCALL
WNetGetConnectionW(
     LPCWSTR lpLocalName,
     LPWSTR  lpRemoteName,
     LPDWORD  lpnLength
    );

DWORD STDCALL
WNetUseConnectionW(
    HWND            hwndOwner,
    LPNETRESOURCE  lpNetResource,
    LPCWSTR        lpUserID,
    LPCWSTR        lpPassword,
    DWORD           dwFlags,
    LPWSTR         lpAccessName,
    LPDWORD         lpBufferSize,
    LPDWORD         lpResult
    );

DWORD STDCALL
WNetSetConnectionW(
    LPCWSTR    lpName,
    DWORD       dwProperties,
    LPVOID      pvValues
    );

DWORD STDCALL
WNetConnectionDialog1W(
    LPCONNECTDLGSTRUCT lpConnDlgStruct
    );

DWORD STDCALL
WNetDisconnectDialog1W(
    LPDISCDLGSTRUCT lpConnDlgStruct
    );

DWORD STDCALL
WNetOpenEnumW(
     DWORD          dwScope,
     DWORD          dwType,
     DWORD          dwUsage,
     LPNETRESOURCE lpNetResource,
     LPHANDLE       lphEnum
    );

DWORD STDCALL
WNetEnumResourceW(
     HANDLE  hEnum,
     LPDWORD lpcCount,
     LPVOID  lpBuffer,
     LPDWORD lpBufferSize
    );

DWORD STDCALL
WNetGetUniversalNameW(
     LPCWSTR lpLocalPath,
     DWORD    dwInfoLevel,
     LPVOID   lpBuffer,
     LPDWORD  lpBufferSize
     );

DWORD STDCALL
WNetGetUserW(
     LPCWSTR  lpName,
     LPWSTR   lpUserName,
     LPDWORD   lpnLength
    );

DWORD STDCALL
WNetGetProviderNameW(
    DWORD   dwNetType,
    LPWSTR lpProviderName,
    LPDWORD lpBufferSize
    );

DWORD STDCALL
WNetGetNetworkInformationW(
    LPCWSTR          lpProvider,
    LPNETINFOSTRUCT   lpNetInfoStruct
    );

DWORD STDCALL
WNetGetLastErrorW(
     LPDWORD    lpError,
     LPWSTR    lpErrorBuf,
     DWORD      nErrorBufSize,
     LPWSTR    lpNameBuf,
     DWORD      nNameBufSize
    );

DWORD STDCALL
MultinetGetConnectionPerformanceW(
        LPNETRESOURCE lpNetResource,
        LPNETCONNECTINFOSTRUCT lpNetConnectInfoStruct
        );

WINBOOL
STDCALL
ChangeServiceConfigW(
    SC_HANDLE    hService,
    DWORD        dwServiceType,
    DWORD        dwStartType,
    DWORD        dwErrorControl,
    LPCWSTR     lpBinaryPathName,
    LPCWSTR     lpLoadOrderGroup,
    LPDWORD      lpdwTagId,
    LPCWSTR     lpDependencies,
    LPCWSTR     lpServiceStartName,
    LPCWSTR     lpPassword,
    LPCWSTR     lpDisplayName
    );

SC_HANDLE
STDCALL
CreateServiceW(
    SC_HANDLE    hSCManager,
    LPCWSTR     lpServiceName,
    LPCWSTR     lpDisplayName,
    DWORD        dwDesiredAccess,
    DWORD        dwServiceType,
    DWORD        dwStartType,
    DWORD        dwErrorControl,
    LPCWSTR     lpBinaryPathName,
    LPCWSTR     lpLoadOrderGroup,
    LPDWORD      lpdwTagId,
    LPCWSTR     lpDependencies,
    LPCWSTR     lpServiceStartName,
    LPCWSTR     lpPassword
    );

WINBOOL
STDCALL
EnumDependentServicesW(
    SC_HANDLE               hService,
    DWORD                   dwServiceState,
    LPENUM_SERVICE_STATUSW  lpServices,
    DWORD                   cbBufSize,
    LPDWORD                 pcbBytesNeeded,
    LPDWORD                 lpServicesReturned
    );

WINBOOL
STDCALL
EnumServicesStatusW(
    SC_HANDLE               hSCManager,
    DWORD                   dwServiceType,
    DWORD                   dwServiceState,
    LPENUM_SERVICE_STATUSW  lpServices,
    DWORD                   cbBufSize,
    LPDWORD                 pcbBytesNeeded,
    LPDWORD                 lpServicesReturned,
    LPDWORD                 lpResumeHandle
    );

WINBOOL
STDCALL
GetServiceKeyNameW(
    SC_HANDLE               hSCManager,
    LPCWSTR                 lpDisplayName,
    LPWSTR                  lpServiceName,
    LPDWORD                 lpcchBuffer
    );

WINBOOL
STDCALL
GetServiceDisplayNameW(
    SC_HANDLE               hSCManager,
    LPCWSTR                 lpServiceName,
    LPWSTR                  lpDisplayName,
    LPDWORD                 lpcchBuffer
    );

SC_HANDLE
STDCALL
OpenSCManagerW(
    LPCWSTR lpMachineName,
    LPCWSTR lpDatabaseName,
    DWORD   dwDesiredAccess
    );

SC_HANDLE
STDCALL
OpenServiceW(
    SC_HANDLE   hSCManager,
    LPCWSTR     lpServiceName,
    DWORD       dwDesiredAccess
    );

WINBOOL
STDCALL
QueryServiceConfigW(
    SC_HANDLE               hService,
    LPQUERY_SERVICE_CONFIGW lpServiceConfig,
    DWORD                   cbBufSize,
    LPDWORD                 pcbBytesNeeded
    );

WINBOOL
STDCALL
QueryServiceLockStatusW(
    SC_HANDLE                       hSCManager,
    LPQUERY_SERVICE_LOCK_STATUSW    lpLockStatus,
    DWORD                           cbBufSize,
    LPDWORD                         pcbBytesNeeded
    );

SERVICE_STATUS_HANDLE
STDCALL
RegisterServiceCtrlHandlerW(
    LPCWSTR              lpServiceName,
    LPHANDLER_FUNCTION   lpHandlerProc
    );

WINBOOL
STDCALL
StartServiceCtrlDispatcherW(
    LPSERVICE_TABLE_ENTRYW   lpServiceStartTable
    );

WINBOOL
STDCALL
StartServiceW(
    SC_HANDLE            hService,
    DWORD                dwNumServiceArgs,
    LPCWSTR             *lpServiceArgVectors
    );

/* Extensions to OpenGL */

WINBOOL STDCALL
wglUseFontBitmapsW(HDC, DWORD, DWORD, DWORD);

WINBOOL STDCALL
wglUseFontOutlinesW(HDC, DWORD, DWORD, DWORD, FLOAT,
		    FLOAT, int, LPGLYPHMETRICSFLOAT);

/* ------------------------------------- */
/* From shellapi.h in old Cygnus headers */

unsigned int WINAPI
DragQueryFileW(HDROP, unsigned int, LPCWSTR, unsigned int);

HICON WINAPI
ExtractAssociatedIconW (HINSTANCE, LPCWSTR, WORD *);

HICON WINAPI
ExtractIconW (HINSTANCE, const LPCWSTR, unsigned int);

HINSTANCE WINAPI
FindExecutableW (const LPCWSTR, const LPCWSTR, LPCWSTR);

int WINAPI
ShellAboutW (HWND, const LPCWSTR, const LPCWSTR, HICON);

HINSTANCE WINAPI
ShellExecuteW (HWND, const LPCWSTR, const LPCWSTR, LPCWSTR, const LPCWSTR, int);

/* end of stuff from shellapi.h in old Cygnus headers */
/* -------------------------------------------------- */
/* From ddeml.h in old Cygnus headers */

HSZ
WINAPI
DdeCreateStringHandleW(
  DWORD idInst,
  LPWSTR psz,
  int iCodePage);

UINT WINAPI
DdeInitializeW (DWORD *, CALLB, DWORD, DWORD);

DWORD
WINAPI
DdeQueryStringW(
  DWORD idInst,
  HSZ hsz,
  LPWSTR psz,
  DWORD cchMax,
  int iCodePage);

/* end of stuff from ddeml.h in old Cygnus headers */
/* ----------------------------------------------- */

WINBOOL STDCALL LogonUserW (LPWSTR, LPWSTR, LPWSTR, DWORD, DWORD, HANDLE *);
WINBOOL STDCALL CreateProcessAsUserW (HANDLE, LPCWSTR, LPWSTR,
			SECURITY_ATTRIBUTES*, SECURITY_ATTRIBUTES*, WINBOOL,
                        DWORD, LPVOID, LPCWSTR, STARTUPINFOW*,
			PROCESS_INFORMATION*);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GNU_H_WINDOWS32_UNICODEFUNCTIONS */
