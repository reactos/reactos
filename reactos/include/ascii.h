/* 
   ASCIIFunctions.h

   Declarations for all the Win32 ASCII Functions

   Copyright (C) 1996 Free Software Foundation, Inc.

   Author:  Scott Christley <scottc@net-community.com>

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

#ifndef _GNU_H_WINDOWS32_ASCIIFUNCTIONS
#define _GNU_H_WINDOWS32_ASCIIFUNCTIONS

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

WINBOOL
STDCALL
GetBinaryTypeA(
	       LPCSTR lpApplicationName,
	       LPDWORD lpBinaryType
	       );

DWORD
STDCALL
GetShortPathNameA(
		  LPCSTR lpszLongPath,
		  LPSTR  lpszShortPath,
		  DWORD    cchBuffer
		  );

LPSTR
STDCALL
GetEnvironmentStringsA(
		       VOID
		       );

WINBOOL
STDCALL
FreeEnvironmentStringsA(
			LPSTR
			);

DWORD
STDCALL
FormatMessageA(
	       DWORD dwFlags,
	       LPCVOID lpSource,
	       DWORD dwMessageId,
	       DWORD dwLanguageId,
	       LPSTR lpBuffer,
	       DWORD nSize,
	       va_list *Arguments
	       );

HANDLE
STDCALL
CreateMailslotA(
		LPCSTR lpName,
		DWORD nMaxMessageSize,
		DWORD lReadTimeout,
		LPSECURITY_ATTRIBUTES lpSecurityAttributes
		);

int
STDCALL
lstrcmpA(
	 LPCSTR lpString1,
	 LPCSTR lpString2
	 );

int
STDCALL
lstrcmpiA(
	  LPCSTR lpString1,
	  LPCSTR lpString2
	  );

LPSTR
STDCALL
lstrcpynA(
	  LPSTR lpString1,
	  LPCSTR lpString2,
	  int iMaxLength
	  );

LPSTR
STDCALL
lstrcpyA(
	 LPSTR lpString1,
	 LPCSTR lpString2
	 );

LPSTR
STDCALL
lstrcatA(
	 LPSTR lpString1,
	 LPCSTR lpString2
	 );

int
STDCALL
lstrlenA(
	 LPCSTR lpString
	 );

HANDLE
STDCALL
CreateMutexA(
	     LPSECURITY_ATTRIBUTES lpMutexAttributes,
	     WINBOOL bInitialOwner,
	     LPCSTR lpName
	     );

HANDLE
STDCALL
OpenMutexA(
	   DWORD dwDesiredAccess,
	   WINBOOL bInheritHandle,
	   LPCSTR lpName
	   );

HANDLE
STDCALL
CreateEventA(
	     LPSECURITY_ATTRIBUTES lpEventAttributes,
	     WINBOOL bManualReset,
	     WINBOOL bInitialState,
	     LPCSTR lpName
	     );

HANDLE
STDCALL
OpenEventA(
	   DWORD dwDesiredAccess,
	   WINBOOL bInheritHandle,
	   LPCSTR lpName
	   );

HANDLE
STDCALL
CreateSemaphoreA(
		 LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
		 LONG lInitialCount,
		 LONG lMaximumCount,
		 LPCSTR lpName
		 );

HANDLE
STDCALL
OpenSemaphoreA(
	       DWORD dwDesiredAccess,
	       WINBOOL bInheritHandle,
	       LPCSTR lpName
	       );

HANDLE
STDCALL
CreateFileMappingA(
		   HANDLE hFile,
		   LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
		   DWORD flProtect,
		   DWORD dwMaximumSizeHigh,
		   DWORD dwMaximumSizeLow,
		   LPCSTR lpName
		   );

HANDLE
STDCALL
OpenFileMappingA(
		 DWORD dwDesiredAccess,
		 WINBOOL bInheritHandle,
		 LPCSTR lpName
		 );

DWORD
STDCALL
GetLogicalDriveStringsA(
			DWORD nBufferLength,
			LPSTR lpBuffer
			);

HINSTANCE
STDCALL
LoadLibraryA(
	     LPCSTR lpLibFileName
	     );

HINSTANCE
STDCALL
LoadLibraryExA(
	       LPCSTR lpLibFileName,
	       HANDLE hFile,
	       DWORD dwFlags
	       );

DWORD
STDCALL
GetModuleFileNameA(
		   HINSTANCE hModule,
		   LPSTR lpFilename,
		   DWORD nSize
		   );

HMODULE
STDCALL
GetModuleHandleA(
		 LPCSTR lpModuleName
		 );

VOID
STDCALL
FatalAppExitA(
	      UINT uAction,
	      LPCSTR lpMessageText
	      );

LPSTR
STDCALL
GetCommandLineA(
		VOID
		);

DWORD
STDCALL
GetEnvironmentVariableA(
			LPCSTR lpName,
			LPSTR lpBuffer,
			DWORD nSize
			);

WINBOOL
STDCALL
SetEnvironmentVariableA(
			LPCSTR lpName,
			LPCSTR lpValue
			);

DWORD
STDCALL
ExpandEnvironmentStringsA(
			  LPCSTR lpSrc,
			  LPSTR lpDst,
			  DWORD nSize
			  );

VOID
STDCALL
OutputDebugStringA(
		   LPCSTR lpOutputString
		   );

HRSRC
STDCALL
FindResourceA(
	      HINSTANCE hModule,
	      LPCSTR lpName,
	      LPCSTR lpType
	      );

HRSRC
STDCALL
FindResourceExA(
		HINSTANCE hModule,
		LPCSTR lpType,
		LPCSTR lpName,
		WORD    wLanguage
		);

WINBOOL
STDCALL
EnumResourceTypesA(
		   HINSTANCE hModule,
		   ENUMRESTYPEPROC lpEnumFunc,
		   LONG lParam
		   );

WINBOOL
STDCALL
EnumResourceNamesA(
		   HINSTANCE hModule,
		   LPCSTR lpType,
		   ENUMRESNAMEPROC lpEnumFunc,
		   LONG lParam
		   );

WINBOOL
STDCALL
EnumResourceLanguagesA(
		       HINSTANCE hModule,
		       LPCSTR lpType,
		       LPCSTR lpName,
		       ENUMRESLANGPROC lpEnumFunc,
		       LONG lParam
		       );

HANDLE
STDCALL
BeginUpdateResourceA(
		     LPCSTR pFileName,
		     WINBOOL bDeleteExistingResources
		     );

WINBOOL
STDCALL
UpdateResourceA(
		HANDLE      hUpdate,
		LPCSTR     lpType,
		LPCSTR     lpName,
		WORD        wLanguage,
		LPVOID      lpData,
		DWORD       cbData
		);

WINBOOL
STDCALL
EndUpdateResourceA(
		   HANDLE      hUpdate,
		   WINBOOL        fDiscard
		   );

ATOM
STDCALL
GlobalAddAtomA(
	       LPCSTR lpString
	       );

ATOM
STDCALL
GlobalFindAtomA(
		LPCSTR lpString
		);

UINT
STDCALL
GlobalGetAtomNameA(
		   ATOM nAtom,
		   LPSTR lpBuffer,
		   int nSize
		   );

ATOM
STDCALL
AddAtomA(
	 LPCSTR lpString
	 );

ATOM
STDCALL
FindAtomA(
	  LPCSTR lpString
	  );

UINT
STDCALL
GetAtomNameA(
	     ATOM nAtom,
	     LPSTR lpBuffer,
	     int nSize
	     );

UINT
STDCALL
GetProfileIntA(
	       LPCSTR lpAppName,
	       LPCSTR lpKeyName,
	       INT nDefault
	       );

DWORD
STDCALL
GetProfileStringA(
		  LPCSTR lpAppName,
		  LPCSTR lpKeyName,
		  LPCSTR lpDefault,
		  LPSTR lpReturnedString,
		  DWORD nSize
		  );

WINBOOL
STDCALL
WriteProfileStringA(
    LPCSTR lpAppName,
    LPCSTR lpKeyName,
    LPCSTR lpString
    );

DWORD
STDCALL
GetProfileSectionA(
    LPCSTR lpAppName,
    LPSTR lpReturnedString,
    DWORD nSize
    );

WINBOOL
STDCALL
WriteProfileSectionA(
    LPCSTR lpAppName,
    LPCSTR lpString
    );

UINT
STDCALL
GetPrivateProfileIntA(
    LPCSTR lpAppName,
    LPCSTR lpKeyName,
    INT nDefault,
    LPCSTR lpFileName
    );

DWORD
STDCALL
GetPrivateProfileStringA(
    LPCSTR lpAppName,
    LPCSTR lpKeyName,
    LPCSTR lpDefault,
    LPSTR lpReturnedString,
    DWORD nSize,
    LPCSTR lpFileName
    );

WINBOOL
STDCALL
WritePrivateProfileStringA(
    LPCSTR lpAppName,
    LPCSTR lpKeyName,
    LPCSTR lpString,
    LPCSTR lpFileName
    );

DWORD
STDCALL
GetPrivateProfileSectionA(
    LPCSTR lpAppName,
    LPSTR lpReturnedString,
    DWORD nSize,
    LPCSTR lpFileName
    );

WINBOOL
STDCALL
WritePrivateProfileSectionA(
    LPCSTR lpAppName,
    LPCSTR lpString,
    LPCSTR lpFileName
    );

UINT
STDCALL
GetDriveTypeA(
    LPCSTR lpRootPathName
    );

UINT
STDCALL
GetSystemDirectoryA(
    LPSTR lpBuffer,
    UINT uSize
    );

DWORD
STDCALL
GetTempPathA(
    DWORD nBufferLength,
    LPSTR lpBuffer
    );

UINT
STDCALL
GetTempFileNameA(
    LPCSTR lpPathName,
    LPCSTR lpPrefixString,
    UINT uUnique,
    LPSTR lpTempFileName
    );

UINT
STDCALL
GetWindowsDirectoryA(
    LPSTR lpBuffer,
    UINT uSize
    );

WINBOOL
STDCALL
SetCurrentDirectoryA(
    LPCSTR lpPathName
    );

DWORD
STDCALL
GetCurrentDirectoryA(
    DWORD nBufferLength,
    LPSTR lpBuffer
    );

WINBOOL
STDCALL
GetDiskFreeSpaceA(
    LPCSTR lpRootPathName,
    LPDWORD lpSectorsPerCluster,
    LPDWORD lpBytesPerSector,
    LPDWORD lpNumberOfFreeClusters,
    LPDWORD lpTotalNumberOfClusters
    );

WINBOOL
STDCALL
GetDiskFreeSpaceExA(
    LPCSTR lpDirectoryName,
    PULARGE_INTEGER lpFreeBytesAvailableToCaller,
    PULARGE_INTEGER lpTotalNumberOfBytes,
    PULARGE_INTEGER lpTotalNumberOfFreeBytes
    );

WINBOOL
STDCALL
CreateDirectoryA(
    LPCSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

WINBOOL
STDCALL
CreateDirectoryExA(
    LPCSTR lpTemplateDirectory,
    LPCSTR lpNewDirectory,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

WINBOOL
STDCALL
RemoveDirectoryA(
    LPCSTR lpPathName
    );

DWORD
STDCALL
GetFullPathNameA(
    LPCSTR lpFileName,
    DWORD nBufferLength,
    LPSTR lpBuffer,
    LPSTR *lpFilePart
    );

WINBOOL
STDCALL
DefineDosDeviceA(
    DWORD dwFlags,
    LPCSTR lpDeviceName,
    LPCSTR lpTargetPath
    );

DWORD
STDCALL
QueryDosDeviceA(
    LPCSTR lpDeviceName,
    LPSTR lpTargetPath,
    DWORD ucchMax
    );

HANDLE
STDCALL
CreateFileA(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    );

WINBOOL
STDCALL
SetFileAttributesA(
    LPCSTR lpFileName,
    DWORD dwFileAttributes
    );

DWORD
STDCALL
GetFileAttributesA(
    LPCSTR lpFileName
    );

DWORD
STDCALL
GetCompressedFileSizeA(
    LPCSTR lpFileName,
    LPDWORD lpFileSizeHigh
    );

WINBOOL
STDCALL
DeleteFileA(
    LPCSTR lpFileName
    );

DWORD
STDCALL
SearchPathA(
	    LPCSTR lpPath,
    LPCSTR lpFileName,
    LPCSTR lpExtension,
    DWORD nBufferLength,
    LPSTR lpBuffer,
    LPSTR *lpFilePart
    );

WINBOOL
STDCALL
CopyFileA(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName,
    WINBOOL bFailIfExists
    );

WINBOOL
STDCALL
MoveFileA(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName
    );

WINBOOL
STDCALL
MoveFileExA(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName,
    DWORD dwFlags
    );

HANDLE
STDCALL
CreateNamedPipeA(
    LPCSTR lpName,
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
GetNamedPipeHandleStateA(
    HANDLE hNamedPipe,
    LPDWORD lpState,
    LPDWORD lpCurInstances,
    LPDWORD lpMaxCollectionCount,
    LPDWORD lpCollectDataTimeout,
    LPSTR lpUserName,
    DWORD nMaxUserNameSize
    );

WINBOOL
STDCALL
CallNamedPipeA(
    LPCSTR lpNamedPipeName,
    LPVOID lpInBuffer,
    DWORD nInBufferSize,
    LPVOID lpOutBuffer,
    DWORD nOutBufferSize,
    LPDWORD lpBytesRead,
    DWORD nTimeOut
    );

WINBOOL
STDCALL
WaitNamedPipeA(
    LPCSTR lpNamedPipeName,
    DWORD nTimeOut
    );

WINBOOL
STDCALL
SetVolumeLabelA(
    LPCSTR lpRootPathName,
    LPCSTR lpVolumeName
    );

WINBOOL
STDCALL
GetVolumeInformationA(
    LPCSTR lpRootPathName,
    LPSTR lpVolumeNameBuffer,
    DWORD nVolumeNameSize,
    LPDWORD lpVolumeSerialNumber,
    LPDWORD lpMaximumComponentLength,
    LPDWORD lpFileSystemFlags,
    LPSTR lpFileSystemNameBuffer,
    DWORD nFileSystemNameSize
    );

WINBOOL
STDCALL
ClearEventLogA (
    HANDLE hEventLog,
    LPCSTR lpBackupFileName
    );

WINBOOL
STDCALL
BackupEventLogA (
    HANDLE hEventLog,
    LPCSTR lpBackupFileName
    );

HANDLE
STDCALL
OpenEventLogA (
    LPCSTR lpUNCServerName,
    LPCSTR lpSourceName
    );

HANDLE
STDCALL
RegisterEventSourceA (
    LPCSTR lpUNCServerName,
    LPCSTR lpSourceName
    );

HANDLE
STDCALL
OpenBackupEventLogA (
    LPCSTR lpUNCServerName,
    LPCSTR lpFileName
    );

WINBOOL
STDCALL
ReadEventLogA (
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
ReportEventA (
     HANDLE     hEventLog,
     WORD       wType,
     WORD       wCategory,
     DWORD      dwEventID,
     PSID       lpUserSid,
     WORD       wNumStrings,
     DWORD      dwDataSize,
     LPCSTR   *lpStrings,
     LPVOID     lpRawData
    );

WINBOOL
STDCALL
AccessCheckAndAuditAlarmA (
    LPCSTR SubsystemName,
    LPVOID HandleId,
    LPSTR ObjectTypeName,
    LPSTR ObjectName,
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
ObjectOpenAuditAlarmA (
    LPCSTR SubsystemName,
    LPVOID HandleId,
    LPSTR ObjectTypeName,
    LPSTR ObjectName,
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
ObjectPrivilegeAuditAlarmA (
    LPCSTR SubsystemName,
    LPVOID HandleId,
    HANDLE ClientToken,
    DWORD DesiredAccess,
    PPRIVILEGE_SET Privileges,
    WINBOOL AccessGranted
    );

WINBOOL
STDCALL
ObjectCloseAuditAlarmA (
    LPCSTR SubsystemName,
    LPVOID HandleId,
    WINBOOL GenerateOnClose
    );

WINBOOL
STDCALL
PrivilegedServiceAuditAlarmA (
    LPCSTR SubsystemName,
    LPCSTR ServiceName,
    HANDLE ClientToken,
    PPRIVILEGE_SET Privileges,
    WINBOOL AccessGranted
    );

WINBOOL
STDCALL
SetFileSecurityA (
    LPCSTR lpFileName,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor
    );

WINBOOL
STDCALL
GetFileSecurityA (
    LPCSTR lpFileName,
    SECURITY_INFORMATION RequestedInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    DWORD nLength,
    LPDWORD lpnLengthNeeded
    );

HANDLE
STDCALL
FindFirstChangeNotificationA(
    LPCSTR lpPathName,
    WINBOOL bWatchSubtree,
    DWORD dwNotifyFilter
    );

WINBOOL
STDCALL
IsBadStringPtrA(
    LPCSTR lpsz,
    UINT ucchMax
    );

WINBOOL
STDCALL
LookupAccountSidA(
    LPCSTR lpSystemName,
    PSID Sid,
    LPSTR Name,
    LPDWORD cbName,
    LPSTR ReferencedDomainName,
    LPDWORD cbReferencedDomainName,
    PSID_NAME_USE peUse
    );

WINBOOL
STDCALL
LookupAccountNameA(
    LPCSTR lpSystemName,
    LPCSTR lpAccountName,
    PSID Sid,
    LPDWORD cbSid,
    LPSTR ReferencedDomainName,
    LPDWORD cbReferencedDomainName,
    PSID_NAME_USE peUse
    );

WINBOOL
STDCALL
LookupPrivilegeValueA(
    LPCSTR lpSystemName,
    LPCSTR lpName,
    PLUID   lpLuid
    );

WINBOOL
STDCALL
LookupPrivilegeNameA(
    LPCSTR lpSystemName,
    PLUID   lpLuid,
    LPSTR lpName,
    LPDWORD cbName
    );

WINBOOL
STDCALL
LookupPrivilegeDisplayNameA(
    LPCSTR lpSystemName,
    LPCSTR lpName,
    LPSTR lpDisplayName,
    LPDWORD cbDisplayName,
    LPDWORD lpLanguageId
    );

WINBOOL
STDCALL
BuildCommDCBA(
    LPCSTR lpDef,
    LPDCB lpDCB
    );

WINBOOL
STDCALL
BuildCommDCBAndTimeoutsA(
    LPCSTR lpDef,
    LPDCB lpDCB,
    LPCOMMTIMEOUTS lpCommTimeouts
    );

WINBOOL
STDCALL
CommConfigDialogA(
    LPCSTR lpszName,
    HWND hWnd,
    LPCOMMCONFIG lpCC
    );

WINBOOL
STDCALL
GetDefaultCommConfigA(
    LPCSTR lpszName,
    LPCOMMCONFIG lpCC,
    LPDWORD lpdwSize
    );

WINBOOL
STDCALL
SetDefaultCommConfigA(
    LPCSTR lpszName,
    LPCOMMCONFIG lpCC,
    DWORD dwSize
    );

WINBOOL
STDCALL
GetComputerNameA (
    LPSTR lpBuffer,
    LPDWORD nSize
    );

WINBOOL
STDCALL
SetComputerNameA (
    LPCSTR lpComputerName
    );

WINBOOL
STDCALL
GetUserNameA (
    LPSTR lpBuffer,
    LPDWORD nSize
    );

int
STDCALL
wvsprintfA(
    LPSTR,
    LPCSTR,
    va_list arglist);
 
int
CDECL
wsprintfA(LPSTR, LPCSTR, ...);

HKL
STDCALL
LoadKeyboardLayoutA(
    LPCSTR pwszKLID,
    UINT Flags);
 
WINBOOL
STDCALL
GetKeyboardLayoutNameA(
    LPSTR pwszKLID);
 
HDESK
STDCALL
CreateDesktopA(
    LPCSTR lpszDesktop,
    LPCSTR lpszDevice,
    LPDEVMODE pDevmode,
    DWORD dwFlags,
    ACCESS_MASK dwDesiredAccess,
    LPSECURITY_ATTRIBUTES lpsa);
 
HDESK
STDCALL
OpenDesktopA(
    LPSTR lpszDesktop,
    DWORD dwFlags,
    WINBOOL fInherit,
    DWORD dwDesiredAccess);
 
WINBOOL
STDCALL
EnumDesktopsA(
    HWINSTA hwinsta,
    DESKTOPENUMPROC lpEnumFunc,
    LPARAM lParam);
 
HWINSTA
STDCALL
CreateWindowStationA(
    LPSTR lpwinsta,
    DWORD dwReserved,
    DWORD dwDesiredAccess,
    LPSECURITY_ATTRIBUTES lpsa);

HANDLE STDCALL CreateWaitableTimerA( LPSECURITY_ATTRIBUTES Attributes,
				     BOOL ManualReset,
				     LPCTSTR Name );
  
HWINSTA
STDCALL
OpenWindowStationA(
    LPSTR lpszWinSta,
    WINBOOL fInherit,
    DWORD dwDesiredAccess);
 
WINBOOL
STDCALL
EnumWindowStationsA(
    ENUMWINDOWSTATIONPROC lpEnumFunc,
    LPARAM lParam);
 
WINBOOL
STDCALL
GetUserObjectInformationA(
    HANDLE hObj,
    int nIndex,
    PVOID pvInfo,
    DWORD nLength,
    LPDWORD lpnLengthNeeded);
 
WINBOOL
STDCALL
SetUserObjectInformationA(
    HANDLE hObj,
    int nIndex,
    PVOID pvInfo,
    DWORD nLength);
 
UINT
STDCALL
RegisterWindowMessageA(
    LPCSTR lpString);
 
WINBOOL
STDCALL
GetMessageA(
    LPMSG lpMsg,
    HWND hWnd ,
    UINT wMsgFilterMin,
    UINT wMsgFilterMax);
 
LRESULT
STDCALL
DispatchMessageA(
    CONST MSG *lpMsg);
 
WINBOOL
STDCALL
PeekMessageA(
    LPMSG lpMsg,
    HWND hWnd ,
    UINT wMsgFilterMin,
    UINT wMsgFilterMax,
    UINT wRemoveMsg);
 
LRESULT
STDCALL
SendMessageA(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);
 
LRESULT
STDCALL
SendMessageTimeoutA(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam,
    UINT fuFlags,
    UINT uTimeout,
    PDWORD_PTR lpdwResult);
 
WINBOOL
STDCALL
SendNotifyMessageA(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);
 
WINBOOL
STDCALL
SendMessageCallbackA(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam,
    SENDASYNCPROC lpResultCallBack,
    ULONG_PTR dwData);
 
WINBOOL
STDCALL
PostMessageA(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);
 
WINBOOL
STDCALL
PostThreadMessageA(
    DWORD idThread,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);
 
LRESULT
STDCALL
DefWindowProcA(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);
 
LRESULT
STDCALL
CallWindowProcA(
    WNDPROC lpPrevWndFunc,
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);
 
ATOM
STDCALL
RegisterClassA(
    CONST WNDCLASS *lpWndClass);
 
WINBOOL
STDCALL
UnregisterClassA(
    LPCSTR lpClassName,
    HINSTANCE hInstance);
 
WINBOOL
STDCALL
GetClassInfoA(
    HINSTANCE hInstance ,
    LPCSTR lpClassName,
    LPWNDCLASS lpWndClass);
 
ATOM
STDCALL
RegisterClassExA(CONST WNDCLASSEX *);
 
WINBOOL
STDCALL
GetClassInfoExA(HINSTANCE, LPCSTR, LPWNDCLASSEX);
 
HWND
STDCALL
CreateWindowExA(
    DWORD dwExStyle,
    LPCSTR lpClassName,
    LPCSTR lpWindowName,
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
CreateDialogParamA(
    HINSTANCE hInstance,
    LPCSTR lpTemplateName,
    HWND hWndParent ,
    DLGPROC lpDialogFunc,
    LPARAM dwInitParam);
 
HWND
STDCALL
CreateDialogIndirectParamA(
    HINSTANCE hInstance,
    LPCDLGTEMPLATE lpTemplate,
    HWND hWndParent,
    DLGPROC lpDialogFunc,
    LPARAM dwInitParam);
 
int
STDCALL
DialogBoxParamA(
    HINSTANCE hInstance,
    LPCSTR lpTemplateName,
    HWND hWndParent ,
    DLGPROC lpDialogFunc,
    LPARAM dwInitParam);
 
int
STDCALL
DialogBoxIndirectParamA(
    HINSTANCE hInstance,
    LPCDLGTEMPLATE hDialogTemplate,
    HWND hWndParent ,
    DLGPROC lpDialogFunc,
    LPARAM dwInitParam);
 
WINBOOL
STDCALL
SetDlgItemTextA(
    HWND hDlg,
    int nIDDlgItem,
    LPCSTR lpString);
 
UINT
STDCALL
GetDlgItemTextA(
    HWND hDlg,
    int nIDDlgItem,
    LPSTR lpString,
    int nMaxCount);
 
LRESULT
STDCALL
SendDlgItemMessageA(
    HWND hDlg,
    int nIDDlgItem,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);
 
LRESULT
STDCALL
DefDlgProcA(
    HWND hDlg,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);
 
WINBOOL
STDCALL
CallMsgFilterA(
    LPMSG lpMsg,
    int nCode);
 
UINT
STDCALL
RegisterClipboardFormatA(
    LPCSTR lpszFormat);
 
int
STDCALL
GetClipboardFormatNameA(
    UINT format,
    LPSTR lpszFormatName,
    int cchMaxCount);
 
WINBOOL
STDCALL
CharToOemA(
    LPCSTR lpszSrc,
    LPSTR lpszDst);
 
WINBOOL
STDCALL
OemToCharA(
    LPCSTR lpszSrc,
    LPSTR lpszDst);
 
WINBOOL
STDCALL
CharToOemBuffA(
    LPCSTR lpszSrc,
    LPSTR lpszDst,
    DWORD cchDstLength);
 
WINBOOL
STDCALL
OemToCharBuffA(
    LPCSTR lpszSrc,
    LPSTR lpszDst,
    DWORD cchDstLength);
 
LPSTR
STDCALL
CharUpperA(
    LPSTR lpsz);
 
DWORD
STDCALL
CharUpperBuffA(
    LPSTR lpsz,
    DWORD cchLength);
 
LPSTR
STDCALL
CharLowerA(
    LPSTR lpsz);
 
DWORD
STDCALL
CharLowerBuffA(
    LPSTR lpsz,
    DWORD cchLength);
 
LPSTR
STDCALL
CharNextA(
    LPCSTR lpsz);
 
LPSTR
STDCALL
CharPrevA(
    LPCSTR lpszStart,
    LPCSTR lpszCurrent);
 
WINBOOL
STDCALL
IsCharAlphaA(
    CHAR ch);
 
WINBOOL
STDCALL
IsCharAlphaNumericA(
    CHAR ch);
 
WINBOOL
STDCALL
IsCharUpperA(
    CHAR ch);
 
WINBOOL
STDCALL
IsCharLowerA(
    CHAR ch);
 
int
STDCALL
GetKeyNameTextA(
    LONG lParam,
    LPSTR lpString,
    int nSize
    );
 
SHORT
STDCALL
VkKeyScanA(
    CHAR ch);
 
SHORT
STDCALL VkKeyScanExA(
    CHAR  ch,
    HKL   dwhkl);
 
UINT
STDCALL
MapVirtualKeyA(
    UINT uCode,
    UINT uMapType);
 
UINT
STDCALL
MapVirtualKeyExA(
    UINT uCode,
    UINT uMapType,
    HKL dwhkl);
 
HACCEL
STDCALL
LoadAcceleratorsA(
    HINSTANCE hInstance,
    LPCSTR lpTableName);
 
HACCEL
STDCALL
CreateAcceleratorTableA(
    LPACCEL, int);
 
int
STDCALL
CopyAcceleratorTableA(
    HACCEL hAccelSrc,
    LPACCEL lpAccelDst,
    int cAccelEntries);
 
int
STDCALL
TranslateAcceleratorA(
    HWND hWnd,
    HACCEL hAccTable,
    LPMSG lpMsg);
 
HMENU
STDCALL
LoadMenuA(
    HINSTANCE hInstance,
    LPCSTR lpMenuName);
 
HMENU
STDCALL
LoadMenuIndirectA(
    CONST MENUTEMPLATE *lpMenuTemplate);
 
WINBOOL
STDCALL
ChangeMenuA(
    HMENU hMenu,
    UINT cmd,
    LPCSTR lpszNewItem,
    UINT cmdInsert,
    UINT flags);
 
int
STDCALL
GetMenuStringA(
    HMENU hMenu,
    UINT uIDItem,
    LPSTR lpString,
    int nMaxCount,
    UINT uFlag);
 
WINBOOL
STDCALL
InsertMenuA(
    HMENU hMenu,
    UINT uPosition,
    UINT uFlags,
    UINT uIDNewItem,
    LPCSTR lpNewItem
    );
 
WINBOOL
STDCALL
AppendMenuA(
    HMENU hMenu,
    UINT uFlags,
    UINT uIDNewItem,
    LPCSTR lpNewItem
    );
 
WINBOOL
STDCALL
ModifyMenuA(
    HMENU hMnu,
    UINT uPosition,
    UINT uFlags,
    UINT uIDNewItem,
    LPCSTR lpNewItem
    );
 
WINBOOL
STDCALL
InsertMenuItemA(
    HMENU,
    UINT,
    WINBOOL,
    LPCMENUITEMINFO
    );
 
WINBOOL
STDCALL
GetMenuItemInfoA(
    HMENU,
    UINT,
    WINBOOL,
    LPMENUITEMINFO
    );
 
WINBOOL
STDCALL
SetMenuItemInfoA(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPMENUITEMINFO lpmii);
 
int
STDCALL
DrawTextA(
    HDC hDC,
    LPCSTR lpString,
    int nCount,
    LPRECT lpRect,
    UINT uFormat);
 
int
STDCALL
DrawTextExA(HDC, LPSTR, int, LPRECT, UINT, LPDRAWTEXTPARAMS);
 
WINBOOL
STDCALL
GrayStringA(
    HDC hDC,
    HBRUSH hBrush,
    GRAYSTRINGPROC lpOutputFunc,
    LPARAM lpData,
    int nCount,
    int X,
    int Y,
    int nWidth,
    int nHeight);
 
WINBOOL
STDCALL
DrawStateA(HDC, HBRUSH, DRAWSTATEPROC, LPARAM, WPARAM, int, int, int, int, UINT);

LONG
STDCALL
TabbedTextOutA(
    HDC hDC,
    int X,
    int Y,
    LPCSTR lpString,
    int nCount,
    int nTabPositions,
    LPINT lpnTabStopPositions,
    int nTabOrigin);
 
DWORD
STDCALL
GetTabbedTextExtentA(
    HDC hDC,
    LPCSTR lpString,
    int nCount,
    int nTabPositions,
    LPINT lpnTabStopPositions);
 
WINBOOL
STDCALL
SetPropA(
    HWND hWnd,
    LPCSTR lpString,
    HANDLE hData);
 
HANDLE
STDCALL
GetPropA(
    HWND hWnd,
    LPCSTR lpString);
 
HANDLE
STDCALL
RemovePropA(
    HWND hWnd,
    LPCSTR lpString);
 
int
STDCALL
EnumPropsExA(
    HWND hWnd,
    PROPENUMPROCEX lpEnumFunc,
    LPARAM lParam);
 
int
STDCALL
EnumPropsA(
    HWND hWnd,
    PROPENUMPROC lpEnumFunc);
 
WINBOOL
STDCALL
SetWindowTextA(
    HWND hWnd,
    LPCSTR lpString);
 
int
STDCALL
GetWindowTextA(
    HWND hWnd,
    LPSTR lpString,
    int nMaxCount);
 
int
STDCALL
GetWindowTextLengthA(
    HWND hWnd);
 
int
STDCALL
MessageBoxA(
    HWND hWnd ,
    LPCSTR lpText,
    LPCSTR lpCaption,
    UINT uType);
 
int
STDCALL
MessageBoxExA(
    HWND hWnd ,
    LPCSTR lpText,
    LPCSTR lpCaption,
    UINT uType,
    WORD wLanguageId);
 
int
STDCALL
MessageBoxIndirectA(LPMSGBOXPARAMS);

LONG
STDCALL
GetWindowLongA(
    HWND hWnd,
    int nIndex);
 
LONG
STDCALL
SetWindowLongA(
    HWND hWnd,
    int nIndex,
    LONG dwNewLong);
 
DWORD
STDCALL
GetClassLongA(
    HWND hWnd,
    int nIndex);
 
DWORD
STDCALL
SetClassLongA(
    HWND hWnd,
    int nIndex,
    LONG dwNewLong);
 
HWND
STDCALL
FindWindowA(
    LPCSTR lpClassName ,
    LPCSTR lpWindowName);
 
HWND
STDCALL
FindWindowExA(HWND, HWND, LPCSTR, LPCSTR);

int
STDCALL
GetClassNameA(
    HWND hWnd,
    LPSTR lpClassName,
    int nMaxCount);
 
HHOOK
STDCALL
SetWindowsHookExA(
    int idHook,
    HOOKPROC lpfn,
    HINSTANCE hmod,
    DWORD dwThreadId);
 
HBITMAP
STDCALL
LoadBitmapA(
    HINSTANCE hInstance,
    LPCSTR lpBitmapName);
 
HCURSOR
STDCALL
LoadCursorA(
    HINSTANCE hInstance,
    LPCSTR lpCursorName);
 
HCURSOR
STDCALL
LoadCursorFromFileA(
    LPCSTR    lpFileName);
 
HICON
STDCALL
LoadIconA(
    HINSTANCE hInstance,
    LPCSTR lpIconName);
 
HANDLE
STDCALL
LoadImageA(
    HINSTANCE,
    LPCSTR,
    UINT,
    int,
    int,
    UINT);
 
int
STDCALL
LoadStringA(
    HINSTANCE hInstance,
    UINT uID,
    LPSTR lpBuffer,
    int nBufferMax);
 
int
STDCALL
DlgDirListA(
    HWND hDlg,
    LPSTR lpPathSpec,
    int nIDListBox,
    int nIDStaticPath,
    UINT uFileType);
 
WINBOOL
STDCALL
DlgDirSelectExA(
    HWND hDlg,
    LPSTR lpString,
    int nCount,
    int nIDListBox);
 
int
STDCALL
DlgDirListComboBoxA(
    HWND hDlg,
    LPSTR lpPathSpec,
    int nIDComboBox,
    int nIDStaticPath,
    UINT uFiletype);
 
WINBOOL
STDCALL
DlgDirSelectComboBoxExA(
    HWND hDlg,
    LPSTR lpString,
    int nCount,
    int nIDComboBox);
 
LRESULT
STDCALL
DefFrameProcA(
    HWND hWnd,
    HWND hWndMDIClient ,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);
 
LRESULT
STDCALL
DefMDIChildProcA(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);
 
HWND
STDCALL
CreateMDIWindowA(
    LPCSTR lpClassName,
    LPCSTR lpWindowName,
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
WinHelpA(
    HWND hWndMain,
    LPCSTR lpszHelp,
    UINT uCommand,
    DWORD dwData
    );
 
LONG
STDCALL
ChangeDisplaySettingsA(
    LPDEVMODE lpDevMode,
    DWORD dwFlags);
 
WINBOOL
STDCALL
EnumDisplaySettingsA(
    LPCSTR lpszDeviceName,
    DWORD iModeNum,
    LPDEVMODE lpDevMode);
 
WINBOOL
STDCALL
SystemParametersInfoA(
    UINT uiAction,
    UINT uiParam,
    PVOID pvParam,
    UINT fWinIni);
 
int
STDCALL
AddFontResourceA(LPCSTR);

HMETAFILE
STDCALL
CopyMetaFileA(HMETAFILE, LPCSTR);

HFONT
STDCALL
CreateFontIndirectA(CONST LOGFONT *);

HDC
STDCALL
CreateICA(LPCSTR, LPCSTR , LPCSTR , CONST DEVMODE *);

HDC
STDCALL
CreateMetaFileA(LPCSTR);

WINBOOL
STDCALL
CreateScalableFontResourceA(DWORD, LPCSTR, LPCSTR, LPCSTR);

int
STDCALL
DeviceCapabilitiesA(LPCSTR, LPCSTR, WORD,
                                LPSTR, CONST DEVMODE *);

int
STDCALL
EnumFontFamiliesExA(HDC, LPLOGFONT, FONTENUMEXPROC, LPARAM,DWORD);

int
STDCALL
EnumFontFamiliesA(HDC, LPCSTR, FONTENUMPROC, LPARAM);

int
STDCALL
EnumFontsA(HDC, LPCSTR,  ENUMFONTSPROC, LPARAM);

WINBOOL
STDCALL
GetCharWidthA(HDC, UINT, UINT, LPINT);

WINBOOL
STDCALL
GetCharWidth32A(HDC, UINT, UINT, LPINT);

WINBOOL
STDCALL
GetCharWidthFloatA(HDC, UINT, UINT, PFLOAT);

WINBOOL
STDCALL
GetCharABCWidthsA(HDC, UINT, UINT, LPABC);

WINBOOL
STDCALL
GetCharABCWidthsFloatA(HDC, UINT, UINT, LPABCFLOAT);
DWORD
STDCALL
GetGlyphOutlineA(HDC, UINT, UINT, LPGLYPHMETRICS, DWORD, LPVOID, CONST MAT2 *);

HMETAFILE
STDCALL
GetMetaFileA(LPCSTR);

UINT
STDCALL
GetOutlineTextMetricsA(HDC, UINT, LPOUTLINETEXTMETRIC);

WINBOOL
STDCALL
GetTextExtentPointA(
                    HDC,
                    LPCSTR,
                    int,
                    LPSIZE
                    );

WINBOOL
STDCALL
GetTextExtentPoint32A(
                    HDC,
                    LPCSTR,
                    int,
                    LPSIZE
                    );

WINBOOL
STDCALL
GetTextExtentExPointA(
                    HDC,
                    LPCSTR,
                    int,
                    int,
                    LPINT,
                    LPINT,
                    LPSIZE
                    );

DWORD
STDCALL
GetCharacterPlacementA(HDC, LPCSTR, int, int, LPGCP_RESULTS, DWORD);

HDC
STDCALL
ResetDCA(HDC, CONST DEVMODE *);

WINBOOL
STDCALL
RemoveFontResourceA(LPCSTR);

HENHMETAFILE
STDCALL
CopyEnhMetaFileA(HENHMETAFILE, LPCSTR);

HDC
STDCALL
CreateEnhMetaFileA(HDC, LPCSTR, CONST RECT *, LPCSTR);

HENHMETAFILE
STDCALL
GetEnhMetaFileA(LPCSTR);

UINT
STDCALL
GetEnhMetaFileDescriptionA(HENHMETAFILE, UINT, LPSTR );

WINBOOL
STDCALL
GetTextMetricsA(HDC, LPTEXTMETRIC);

int
STDCALL
StartDocA(HDC, CONST DOCINFO *);

int
STDCALL
GetObjectA(HGDIOBJ, int, LPVOID);

WINBOOL
STDCALL
TextOutA(HDC, int, int, LPCSTR, int);

WINBOOL
STDCALL
ExtTextOutA(HDC, int, int, UINT, CONST RECT *,LPCSTR, UINT, CONST INT *);

WINBOOL
STDCALL
PolyTextOutA(HDC, CONST POLYTEXT *, int); 

int
STDCALL
GetTextFaceA(HDC, int, LPSTR);

DWORD
STDCALL
GetKerningPairsA(HDC, DWORD, LPKERNINGPAIR);

HCOLORSPACE
STDCALL
CreateColorSpaceA(LPLOGCOLORSPACE);

WINBOOL
STDCALL
GetLogColorSpaceA(HCOLORSPACE,LPLOGCOLORSPACE,DWORD);

WINBOOL
STDCALL
GetICMProfileA(HDC,DWORD,LPSTR);

WINBOOL
STDCALL
SetICMProfileA(HDC,LPSTR);

WINBOOL
STDCALL
UpdateICMRegKeyA(DWORD, DWORD, LPSTR, UINT);

int
STDCALL
EnumICMProfilesA(HDC,ICMENUMPROC,LPARAM);

int
STDCALL
PropertySheetA(LPCPROPSHEETHEADER lppsph);

HIMAGELIST
STDCALL
ImageList_LoadImageA(HINSTANCE hi, LPCSTR lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags);

HWND
STDCALL
CreateStatusWindowA(LONG style, LPCSTR lpszText, HWND hwndParent, UINT wID);

void
STDCALL
DrawStatusTextA(HDC hDC, LPRECT lprc, LPCSTR pszText, UINT uFlags);

WINBOOL
STDCALL
GetOpenFileNameA(LPOPENFILENAME);

WINBOOL
STDCALL
GetSaveFileNameA(LPOPENFILENAME);

short
STDCALL
GetFileTitleA(LPCSTR, LPSTR, WORD);

WINBOOL
STDCALL
ChooseColorA(LPCHOOSECOLOR);

HWND
STDCALL
FindTextA(LPFINDREPLACE);

HWND
STDCALL
ReplaceTextA(LPFINDREPLACE);

WINBOOL
STDCALL
ChooseFontA(LPCHOOSEFONT);

WINBOOL
STDCALL
PrintDlgA(LPPRINTDLG);

WINBOOL
STDCALL
PageSetupDlgA( LPPAGESETUPDLG );

WINBOOL
STDCALL
CreateProcessA(
    LPCSTR lpApplicationName,
    LPSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    WINBOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCSTR lpCurrentDirectory,
    LPSTARTUPINFOA lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    );

VOID
STDCALL
GetStartupInfoA(
    LPSTARTUPINFOA lpStartupInfo
    );

WINBASEAPI
HANDLE
WINAPI
FindFirstFileExA (
	LPCSTR			lpFileName,
	FINDEX_INFO_LEVELS	fInfoLevelId,
	LPVOID			lpFindFileData,
	FINDEX_SEARCH_OPS	fSearchOp,
	LPVOID			lpSearchFilter,
	DWORD			dwAdditionalFlags
	);

HANDLE
STDCALL
FindFirstFileA(
    LPCSTR lpFileName,
    LPWIN32_FIND_DATA lpFindFileData
    );

WINBOOL
STDCALL
FindNextFileA(
    HANDLE hFindFile,
    LPWIN32_FIND_DATA lpFindFileData
    );

WINBOOL
STDCALL
GetVersionExA(
    LPOSVERSIONINFO lpVersionInformation
    );

#define CreateWindowA(lpClassName, lpWindowName, dwStyle, x, y,\
nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam)\
CreateWindowExA(0L, lpClassName, lpWindowName, dwStyle, x, y,\
nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam)

#define CreateDialogA(hInstance, lpName, hWndParent, lpDialogFunc) \
CreateDialogParamA(hInstance, lpName, hWndParent, lpDialogFunc, 0L)

#define CreateDialogIndirectA(hInstance, lpTemplate, hWndParent, lpDialogFunc) \
CreateDialogIndirectParamA(hInstance, lpTemplate, hWndParent, lpDialogFunc, 0L)

#define DialogBoxA(hInstance, lpTemplate, hWndParent, lpDialogFunc) \
DialogBoxParamA(hInstance, lpTemplate, hWndParent, lpDialogFunc, 0L)

#define DialogBoxIndirectA(hInstance, lpTemplate, hWndParent, lpDialogFunc) \
DialogBoxIndirectParamA(hInstance, lpTemplate, hWndParent, lpDialogFunc, 0L)

HDC
STDCALL
CreateDCA(LPCSTR, LPCSTR , LPCSTR , CONST DEVMODE *);

DWORD
STDCALL
VerInstallFileA(
        DWORD uFlags,
        LPSTR szSrcFileName,
        LPSTR szDestFileName,
        LPSTR szSrcDir,
        LPSTR szDestDir,
        LPSTR szCurDir,
        LPSTR szTmpFile,
        PUINT lpuTmpFileLen
        );

DWORD
STDCALL
GetFileVersionInfoSizeA(
        LPSTR lptstrFilename,
        LPDWORD lpdwHandle
        );

WINBOOL
STDCALL
GetFileVersionInfoA(
        LPSTR lptstrFilename,
        DWORD dwHandle,
        DWORD dwLen,
        LPVOID lpData
        );

DWORD
STDCALL
VerLanguageNameA(
        DWORD wLang,
        LPSTR szLang,
        DWORD nSize
        );

WINBOOL
STDCALL
VerQueryValueA(
        const LPVOID pBlock,
        LPSTR lpSubBlock,
        LPVOID * lplpBuffer,
        PUINT puLen
        );

DWORD
STDCALL
VerFindFileA(
        DWORD uFlags,
        LPSTR szFileName,
        LPSTR szWinDir,
        LPSTR szAppDir,
        LPSTR szCurDir,
        PUINT lpuCurDirLen,
        LPSTR szDestDir,
        PUINT lpuDestDirLen
        );

LONG
STDCALL
RegConnectRegistryA (
    LPSTR lpMachineName,
    HKEY hKey,
    PHKEY phkResult
    );

LONG
STDCALL
RegCreateKeyA (
    HKEY hKey,
    LPCSTR lpSubKey,
    PHKEY phkResult
    );

LONG
STDCALL
RegCreateKeyExA (
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD Reserved,
    LPSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition
    );

LONG
STDCALL
RegDeleteKeyA (
    HKEY hKey,
    LPCSTR lpSubKey
    );

LONG
STDCALL
RegDeleteValueA (
    HKEY hKey,
    LPCSTR lpValueName
    );

LONG
STDCALL
RegEnumKeyA (
    HKEY hKey,
    DWORD dwIndex,
    LPSTR lpName,
    DWORD cbName
    );

LONG
STDCALL
RegEnumKeyExA (
    HKEY hKey,
    DWORD dwIndex,
    LPSTR lpName,
    LPDWORD lpcbName,
    LPDWORD lpReserved,
    LPSTR lpClass,
    LPDWORD lpcbClass,
    PFILETIME lpftLastWriteTime
    );

LONG
STDCALL
RegEnumValueA (
    HKEY hKey,
    DWORD dwIndex,
    LPSTR lpValueName,
    LPDWORD lpcbValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );

LONG
STDCALL
RegLoadKeyA (
    HKEY    hKey,
    LPCSTR  lpSubKey,
    LPCSTR  lpFile
    );

LONG
STDCALL
RegOpenKeyA (
    HKEY hKey,
    LPCSTR lpSubKey,
    PHKEY phkResult
    );

LONG
STDCALL
RegOpenKeyExA (
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
    );

LONG
STDCALL
RegQueryInfoKeyA (
    HKEY hKey,
    LPSTR lpClass,
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

LONG
STDCALL
RegQueryValueA (
    HKEY hKey,
    LPCSTR lpSubKey,
    LPSTR lpValue,
    PLONG   lpcbValue
    );

LONG
STDCALL
RegQueryMultipleValuesA (
    HKEY hKey,
    PVALENT val_list,
    DWORD num_vals,
    LPSTR lpValueBuf,
    LPDWORD ldwTotsize
    );

LONG
STDCALL
RegQueryValueExA (
    HKEY hKey,
    LPSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );

LONG
STDCALL
RegReplaceKeyA (
    HKEY     hKey,
    LPCSTR  lpSubKey,
    LPCSTR  lpNewFile,
    LPCSTR  lpOldFile
    );

LONG
STDCALL
RegRestoreKeyA (
    HKEY hKey,
    LPCSTR lpFile,
    DWORD   dwFlags
    );

LONG
STDCALL
RegSaveKeyA (
    HKEY hKey,
    LPCSTR lpFile,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

LONG
STDCALL
RegSetValueA (
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD dwType,
    LPCSTR lpData,
    DWORD cbData
    );

LONG
STDCALL
RegSetValueExA (
    HKEY hKey,
    LPCSTR lpValueName,
    DWORD Reserved,
    DWORD dwType,
    CONST BYTE* lpData,
    DWORD cbData
    );

LONG
STDCALL
RegUnLoadKeyA (
    HKEY    hKey,
    LPCSTR lpSubKey
    );

WINBOOL
STDCALL
InitiateSystemShutdownA(
    LPSTR lpMachineName,
    LPSTR lpMessage,
    DWORD dwTimeout,
    WINBOOL bForceAppsClosed,
    WINBOOL bRebootAfterShutdown
    );

WINBOOL
STDCALL
AbortSystemShutdownA(
    LPSTR lpMachineName
    );

int
STDCALL
CompareStringA(
    LCID     Locale,
    DWORD    dwCmpFlags,
    LPCSTR lpString1,
    int      cchCount1,
    LPCSTR lpString2,
    int      cchCount2);

int
STDCALL
LCMapStringA(
    LCID     Locale,
    DWORD    dwMapFlags,
    LPCSTR lpSrcStr,
    int      cchSrc,
    LPSTR  lpDestStr,
    int      cchDest);

int
STDCALL
GetLocaleInfoA(
    LCID     Locale,
    LCTYPE   LCType,
    LPSTR  lpLCData,
    int      cchData);

WINBOOL
STDCALL
SetLocaleInfoA(
    LCID     Locale,
    LCTYPE   LCType,
    LPCSTR lpLCData);

int
STDCALL
GetTimeFormatA(
    LCID     Locale,
    DWORD    dwFlags,
    CONST SYSTEMTIME *lpTime,
    LPCSTR lpFormat,
    LPSTR  lpTimeStr,
    int      cchTime);

int
STDCALL
GetDateFormatA(
    LCID     Locale,
    DWORD    dwFlags,
    CONST SYSTEMTIME *lpDate,
    LPCSTR lpFormat,
    LPSTR  lpDateStr,
    int      cchDate);

int
STDCALL
GetNumberFormatA(
    LCID     Locale,
    DWORD    dwFlags,
    LPCSTR lpValue,
    CONST NUMBERFMT *lpFormat,
    LPSTR  lpNumberStr,
    int      cchNumber);

int
STDCALL
GetCurrencyFormatA(
    LCID     Locale,
    DWORD    dwFlags,
    LPCSTR lpValue,
    CONST CURRENCYFMT *lpFormat,
    LPSTR  lpCurrencyStr,
    int      cchCurrency);

WINBOOL
STDCALL
EnumCalendarInfoA(
    CALINFO_ENUMPROC lpCalInfoEnumProc,
    LCID              Locale,
    CALID             Calendar,
    CALTYPE           CalType);

WINBOOL
STDCALL
EnumTimeFormatsA(
    TIMEFMT_ENUMPROC lpTimeFmtEnumProc,
    LCID              Locale,
    DWORD             dwFlags);

WINBOOL
STDCALL
EnumDateFormatsA(
    DATEFMT_ENUMPROC lpDateFmtEnumProc,
    LCID              Locale,
    DWORD             dwFlags);

WINBOOL
STDCALL
GetStringTypeExA(
    LCID     Locale,
    DWORD    dwInfoType,
    LPCSTR lpSrcStr,
    int      cchSrc,
    LPWORD   lpCharType);

WINBOOL
STDCALL
GetStringTypeA(
    LCID     Locale,
    DWORD    dwInfoType,
    LPCSTR   lpSrcStr,
    int      cchSrc,
    LPWORD   lpCharType);


int
STDCALL
FoldStringA(
    DWORD    dwMapFlags,
    LPCSTR lpSrcStr,
    int      cchSrc,
    LPSTR  lpDestStr,
    int      cchDest);

WINBOOL
STDCALL
EnumSystemLocalesA(
    LOCALE_ENUMPROC lpLocaleEnumProc,
    DWORD            dwFlags);

WINBOOL
STDCALL
EnumSystemCodePagesA(
    CODEPAGE_ENUMPROC lpCodePageEnumProc,
    DWORD              dwFlags);

WINBOOL
STDCALL
PeekConsoleInputA(
    HANDLE hConsoleInput,
    PINPUT_RECORD lpBuffer,
    DWORD nLength,
    LPDWORD lpNumberOfEventsRead
    );

WINBOOL
STDCALL
ReadConsoleInputA(
    HANDLE hConsoleInput,
    PINPUT_RECORD lpBuffer,
    DWORD nLength,
    LPDWORD lpNumberOfEventsRead
    );

WINBOOL
STDCALL
WriteConsoleInputA(
    HANDLE hConsoleInput,
    CONST INPUT_RECORD *lpBuffer,
    DWORD nLength,
    LPDWORD lpNumberOfEventsWritten
    );

WINBOOL
STDCALL
ReadConsoleOutputA(
    HANDLE hConsoleOutput,
    PCHAR_INFO lpBuffer,
    COORD dwBufferSize,
    COORD dwBufferCoord,
    PSMALL_RECT lpReadRegion
    );

WINBOOL
STDCALL
WriteConsoleOutputA(
    HANDLE hConsoleOutput,
    CONST CHAR_INFO *lpBuffer,
    COORD dwBufferSize,
    COORD dwBufferCoord,
    PSMALL_RECT lpWriteRegion
    );

WINBOOL
STDCALL
ReadConsoleOutputCharacterA(
    HANDLE hConsoleOutput,
    LPSTR lpCharacter,
    DWORD nLength,
    COORD dwReadCoord,
    LPDWORD lpNumberOfCharsRead
    );

WINBOOL
STDCALL
WriteConsoleOutputCharacterA(
    HANDLE hConsoleOutput,
    LPCSTR lpCharacter,
    DWORD nLength,
    COORD dwWriteCoord,
    LPDWORD lpNumberOfCharsWritten
    );

WINBOOL
STDCALL
FillConsoleOutputCharacterA(
    HANDLE hConsoleOutput,
    CHAR  cCharacter,
    DWORD  nLength,
    COORD  dwWriteCoord,
    LPDWORD lpNumberOfCharsWritten
    );

WINBOOL
STDCALL
ScrollConsoleScreenBufferA(
    HANDLE hConsoleOutput,
    CONST SMALL_RECT *lpScrollRectangle,
    CONST SMALL_RECT *lpClipRectangle,
    COORD dwDestinationOrigin,
    CONST CHAR_INFO *lpFill
    );

DWORD
STDCALL
GetConsoleTitleA(
    LPSTR lpConsoleTitle,
    DWORD nSize
    );

WINBOOL
STDCALL
SetConsoleTitleA(
    LPCSTR lpConsoleTitle
    );

WINBOOL
STDCALL
ReadConsoleA(
    HANDLE hConsoleInput,
    LPVOID lpBuffer,
    DWORD nNumberOfCharsToRead,
    LPDWORD lpNumberOfCharsRead,
    LPVOID lpReserved
    );

WINBOOL
STDCALL
WriteConsoleA(
    HANDLE hConsoleOutput,
    CONST VOID *lpBuffer,
    DWORD nNumberOfCharsToWrite,
    LPDWORD lpNumberOfCharsWritten,
    LPVOID lpReserved
    );

DWORD STDCALL
WNetAddConnectionA(
     LPCSTR   lpRemoteName,
     LPCSTR   lpPassword,
     LPCSTR   lpLocalName
    );

DWORD STDCALL
WNetAddConnection2A(
     LPNETRESOURCE lpNetResource,
     LPCSTR       lpPassword,
     LPCSTR       lpUserName,
     DWORD          dwFlags
    );

DWORD STDCALL
WNetAddConnection3A(
     HWND           hwndOwner,
     LPNETRESOURCE lpNetResource,
     LPCSTR       lpPassword,
     LPCSTR       lpUserName,
     DWORD          dwFlags
    );

DWORD STDCALL
WNetCancelConnectionA(
     LPCSTR lpName,
     WINBOOL     fForce
    );

DWORD STDCALL
WNetCancelConnection2A(
     LPCSTR lpName,
     DWORD    dwFlags,
     WINBOOL     fForce
    );

DWORD STDCALL
WNetGetConnectionA(
     LPCSTR lpLocalName,
     LPSTR  lpRemoteName,
     LPDWORD  lpnLength
    );

DWORD STDCALL
WNetUseConnectionA(
    HWND            hwndOwner,
    LPNETRESOURCE  lpNetResource,
    LPCSTR        lpUserID,
    LPCSTR        lpPassword,
    DWORD           dwFlags,
    LPSTR         lpAccessName,
    LPDWORD         lpBufferSize,
    LPDWORD         lpResult
    );

DWORD STDCALL
WNetSetConnectionA(
    LPCSTR    lpName,
    DWORD       dwProperties,
    LPVOID      pvValues
    );

DWORD STDCALL
WNetConnectionDialog1A(
    LPCONNECTDLGSTRUCT lpConnDlgStruct
    );

DWORD STDCALL
WNetDisconnectDialog1A(
    LPDISCDLGSTRUCT lpConnDlgStruct
    );

DWORD STDCALL
WNetOpenEnumA(
     DWORD          dwScope,
     DWORD          dwType,
     DWORD          dwUsage,
     LPNETRESOURCE lpNetResource,
     LPHANDLE       lphEnum
    );

DWORD STDCALL
WNetEnumResourceA(
     HANDLE  hEnum,
     LPDWORD lpcCount,
     LPVOID  lpBuffer,
     LPDWORD lpBufferSize
    );

DWORD STDCALL
WNetGetUniversalNameA(
     LPCSTR lpLocalPath,
     DWORD    dwInfoLevel,
     LPVOID   lpBuffer,
     LPDWORD  lpBufferSize
     );

DWORD STDCALL
WNetGetUserA(
     LPCSTR  lpName,
     LPSTR   lpUserName,
     LPDWORD   lpnLength
    );

DWORD STDCALL
WNetGetProviderNameA(
    DWORD   dwNetType,
    LPSTR lpProviderName,
    LPDWORD lpBufferSize
    );

DWORD STDCALL
WNetGetNetworkInformationA(
    LPCSTR          lpProvider,
    LPNETINFOSTRUCT   lpNetInfoStruct
    );

DWORD STDCALL
WNetGetLastErrorA(
     LPDWORD    lpError,
     LPSTR    lpErrorBuf,
     DWORD      nErrorBufSize,
     LPSTR    lpNameBuf,
     DWORD      nNameBufSize
    );

DWORD STDCALL
MultinetGetConnectionPerformanceA(
        LPNETRESOURCE lpNetResource,
        LPNETCONNECTINFOSTRUCT lpNetConnectInfoStruct
        );

WINBOOL
STDCALL
ChangeServiceConfigA(
    SC_HANDLE    hService,
    DWORD        dwServiceType,
    DWORD        dwStartType,
    DWORD        dwErrorControl,
    LPCSTR     lpBinaryPathName,
    LPCSTR     lpLoadOrderGroup,
    LPDWORD      lpdwTagId,
    LPCSTR     lpDependencies,
    LPCSTR     lpServiceStartName,
    LPCSTR     lpPassword,
    LPCSTR     lpDisplayName
    );

SC_HANDLE
STDCALL
CreateServiceA(
    SC_HANDLE    hSCManager,
    LPCSTR     lpServiceName,
    LPCSTR     lpDisplayName,
    DWORD        dwDesiredAccess,
    DWORD        dwServiceType,
    DWORD        dwStartType,
    DWORD        dwErrorControl,
    LPCSTR     lpBinaryPathName,
    LPCSTR     lpLoadOrderGroup,
    LPDWORD      lpdwTagId,
    LPCSTR     lpDependencies,
    LPCSTR     lpServiceStartName,
    LPCSTR     lpPassword
    );

WINBOOL
STDCALL
EnumDependentServicesA(
    SC_HANDLE               hService,
    DWORD                   dwServiceState,
    LPENUM_SERVICE_STATUSA  lpServices,
    DWORD                   cbBufSize,
    LPDWORD                 pcbBytesNeeded,
    LPDWORD                 lpServicesReturned
    );

WINBOOL
STDCALL
EnumServicesStatusA(
    SC_HANDLE               hSCManager,
    DWORD                   dwServiceType,
    DWORD                   dwServiceState,
    LPENUM_SERVICE_STATUSA  lpServices,
    DWORD                   cbBufSize,
    LPDWORD                 pcbBytesNeeded,
    LPDWORD                 lpServicesReturned,
    LPDWORD                 lpResumeHandle
    );

WINBOOL
STDCALL
GetServiceKeyNameA(
    SC_HANDLE               hSCManager,
    LPCSTR                lpDisplayName,
    LPSTR                 lpServiceName,
    LPDWORD                 lpcchBuffer
    );

WINBOOL
STDCALL
GetServiceDisplayNameA(
    SC_HANDLE               hSCManager,
    LPCSTR                lpServiceName,
    LPSTR                 lpDisplayName,
    LPDWORD                 lpcchBuffer
    );

SC_HANDLE
STDCALL
OpenSCManagerA(
    LPCSTR lpMachineName,
    LPCSTR lpDatabaseName,
    DWORD   dwDesiredAccess
    );

SC_HANDLE
STDCALL
OpenServiceA(
    SC_HANDLE   hSCManager,
    LPCSTR    lpServiceName,
    DWORD       dwDesiredAccess
    );

WINBOOL
STDCALL
QueryServiceConfigA(
    SC_HANDLE               hService,
    LPQUERY_SERVICE_CONFIG lpServiceConfig,
    DWORD                   cbBufSize,
    LPDWORD                 pcbBytesNeeded
    );

WINBOOL
STDCALL
QueryServiceLockStatusA(
    SC_HANDLE                       hSCManager,
    LPQUERY_SERVICE_LOCK_STATUSA    lpLockStatus,
    DWORD                           cbBufSize,
    LPDWORD                         pcbBytesNeeded
    );

SERVICE_STATUS_HANDLE
STDCALL
RegisterServiceCtrlHandlerA(
    LPCSTR             lpServiceName,
    LPHANDLER_FUNCTION   lpHandlerProc
    );

WINBOOL
STDCALL
StartServiceCtrlDispatcherA(
                            LPSERVICE_TABLE_ENTRYA   lpServiceStartTable
			    );

WINBOOL
STDCALL
StartServiceA(
	      SC_HANDLE            hService,
	      DWORD                dwNumServiceArgs,
	      LPCSTR             *lpServiceArgVectors
	      );

/* Extensions to OpenGL */

WINBOOL STDCALL
wglUseFontBitmapsA(HDC, DWORD, DWORD, DWORD);

WINBOOL STDCALL
wglUseFontOutlinesA(HDC, DWORD, DWORD, DWORD, FLOAT,
		    FLOAT, int, LPGLYPHMETRICSFLOAT);

/* ------------------------------------- */
/* From shellapi.h in old Cygnus headers */

unsigned int WINAPI
DragQueryFileA(HDROP, unsigned int, char *, unsigned int);

HICON WINAPI
ExtractAssociatedIconA (HINSTANCE, char *, WORD *);

HICON WINAPI
ExtractIconA (HINSTANCE, const char *, unsigned int);

HINSTANCE WINAPI
FindExecutableA (const char *, const char *, char *);

int WINAPI
ShellAboutA (HWND, const char *, const char *, HICON);

HINSTANCE WINAPI
ShellExecuteA (HWND, const char *, const char *, char *, const char *, int);

/* end of stuff from shellapi.h in old Cygnus headers */
/* -------------------------------------------------- */
/* From ddeml.h in old Cygnus headers */

HSZ WINAPI
DdeCreateStringHandleA (DWORD, char *, int);

UINT WINAPI
DdeInitializeA (DWORD *, CALLB, DWORD, DWORD);

DWORD WINAPI
DdeQueryStringA (DWORD, HSZ, char *, DWORD, int);

/* end of stuff from ddeml.h in old Cygnus headers */
/* ----------------------------------------------- */

WINBOOL STDCALL LogonUserA (LPSTR, LPSTR, LPSTR, DWORD, DWORD, HANDLE *);
WINBOOL STDCALL CreateProcessAsUserA (HANDLE, LPCTSTR, LPTSTR,
	SECURITY_ATTRIBUTES*, SECURITY_ATTRIBUTES*, WINBOOL, DWORD, LPVOID,
        LPCTSTR, STARTUPINFOA*, PROCESS_INFORMATION*);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GNU_H_WINDOWS32_ASCIIFUNCTIONS */
