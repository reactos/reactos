/* $Id: stubs.c,v 1.2 1999/07/17 23:10:20 ea Exp $
 * KERNEL32.DLL stubs (unimplemented functions)
 * Remove from this file, if you implement them.
 */
#include <windows.h>

ATOM
STDCALL
AddAtomA (
	LPCSTR	lpString
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


ATOM
STDCALL
AddAtomW (
	LPCWSTR	lpString
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

BOOL
STDCALL
AddConsoleAliasA (
	DWORD a0,
	DWORD a1,
	DWORD a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

BOOL
STDCALL
AddConsoleAliasW (
	DWORD a0,
	DWORD a1,
	DWORD a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
BackupRead (
	HANDLE	hFile,
	LPBYTE	lpBuffer,
	DWORD	nNumberOfBytesToRead,
	LPDWORD	lpNumberOfBytesRead,
	WINBOOL	bAbort,
	WINBOOL	bProcessSecurity,
	LPVOID	* lpContext
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
BackupSeek (
	HANDLE	hFile,
	DWORD	dwLowBytesToSeek,
	DWORD	dwHighBytesToSeek,
	LPDWORD	lpdwLowByteSeeked,
	LPDWORD	lpdwHighByteSeeked,
	LPVOID	* lpContext
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
BackupWrite (
	HANDLE	hFile,
	LPBYTE	lpBuffer,
	DWORD	nNumberOfBytesToWrite,
	LPDWORD	lpNumberOfBytesWritten,
	WINBOOL	bAbort,
	WINBOOL	bProcessSecurity,
	LPVOID	* lpContext
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
STDCALL
BaseAttachCompleteThunk (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


HANDLE
STDCALL
BeginUpdateResourceW (
	LPCWSTR	pFileName,
	WINBOOL	bDeleteExistingResources
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


HANDLE
STDCALL
BeginUpdateResourceA (
	LPCSTR	pFileName,
	WINBOOL	bDeleteExistingResources
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
BuildCommDCBA (
	LPCSTR	lpDef,
	LPDCB	lpDCB
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
BuildCommDCBW (
	LPCWSTR	lpDef,
	LPDCB	lpDCB
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
BuildCommDCBAndTimeoutsA (
	LPCSTR		lpDef,
	LPDCB		lpDCB,
	LPCOMMTIMEOUTS	lpCommTimeouts
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
BuildCommDCBAndTimeoutsW (
	LPCWSTR		lpDef,
	LPDCB		lpDCB,
	LPCOMMTIMEOUTS	lpCommTimeouts
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
CallNamedPipeA (
	LPCSTR	lpNamedPipeName,
	LPVOID	lpInBuffer,
	DWORD	nInBufferSize,
	LPVOID	lpOutBuffer,
	DWORD	nOutBufferSize,
	LPDWORD	lpBytesRead,
	DWORD	nTimeOut
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
CallNamedPipeW (
	LPCWSTR	lpNamedPipeName,
	LPVOID	lpInBuffer,
	DWORD	nInBufferSize,
	LPVOID	lpOutBuffer,
	DWORD	nOutBufferSize,
	LPDWORD	lpBytesRead,
	DWORD	nTimeOut
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
ClearCommBreak (
	HANDLE	hFile
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
ClearCommError (
	HANDLE		hFile,
	LPDWORD		lpErrors,
	LPCOMSTAT	lpStat
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
STDCALL
CloseConsoleHandle (
	HANDLE	hConsole
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

BOOL
STDCALL
CloseProfileUserMapping ( VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
STDCALL
CmdBatNotification (
	DWORD	Unknown
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
CommConfigDialogA (
	LPCSTR		lpszName,
	HWND		hWnd,
	LPCOMMCONFIG	lpCC
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
CommConfigDialogW (
	LPCWSTR		lpszName,
	HWND		hWnd,
	LPCOMMCONFIG	lpCC
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


int
STDCALL
CompareStringA (
	LCID	Locale,
	DWORD	dwCmpFlags,
	LPCSTR	lpString1,
	int	cchCount1,
	LPCSTR	lpString2,
	int	cchCount2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
CompareStringW (
	LCID	Locale,
	DWORD	dwCmpFlags,
	LPCWSTR	lpString1,
	int	cchCount1,
	LPCWSTR	lpString2,
	int	cchCount2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
ConnectNamedPipe (
	HANDLE		hNamedPipe,
	LPOVERLAPPED	lpOverlapped
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
STDCALL
ConsoleMenuControl (
	HANDLE	hConsole,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
ContinueDebugEvent (
	DWORD	dwProcessId,
	DWORD	dwThreadId,
	DWORD	dwContinueStatus
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


LCID
STDCALL
ConvertDefaultLocale (
	LCID	Locale
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HANDLE
STDCALL
CreateFileMappingA (
	HANDLE			hFile,
	LPSECURITY_ATTRIBUTES	lpFileMappingAttributes,
	DWORD			flProtect,
	DWORD			dwMaximumSizeHigh,
	DWORD			dwMaximumSizeLow,
	LPCSTR			lpName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return INVALID_HANDLE_VALUE;
}


HANDLE
STDCALL
CreateFileMappingW (
	HANDLE			hFile,
	LPSECURITY_ATTRIBUTES	lpFileMappingAttributes,
	DWORD			flProtect,
	DWORD			dwMaximumSizeHigh,
	DWORD			dwMaximumSizeLow,
	LPCWSTR			lpName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return INVALID_HANDLE_VALUE;
}


HANDLE
STDCALL
CreateMailslotA (
	LPCSTR			lpName,
	DWORD			nMaxMessageSize,
	DWORD			lReadTimeout,
	LPSECURITY_ATTRIBUTES	lpSecurityAttributes
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return INVALID_HANDLE_VALUE;
}


HANDLE
STDCALL
CreateMailslotW (
	LPCWSTR			lpName,
	DWORD			nMaxMessageSize,
	DWORD			lReadTimeout,
	LPSECURITY_ATTRIBUTES	lpSecurityAttributes
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return INVALID_HANDLE_VALUE;
}


HANDLE
STDCALL
CreateNamedPipeW (
	LPCWSTR			lpName,
	DWORD			dwOpenMode,
	DWORD			dwPipeMode,
	DWORD			nMaxInstances,
	DWORD			nOutBufferSize,
	DWORD			nInBufferSize,
	DWORD			nDefaultTimeOut,
	LPSECURITY_ATTRIBUTES	lpSecurityAttributes
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return INVALID_HANDLE_VALUE;
}


HANDLE
STDCALL
CreateNamedPipeA (
	LPCSTR			lpName,
	DWORD			dwOpenMode,
	DWORD			dwPipeMode,
	DWORD			nMaxInstances,
	DWORD			nOutBufferSize,
	DWORD			nInBufferSize,
	DWORD			nDefaultTimeOut,
	LPSECURITY_ATTRIBUTES	lpSecurityAttributes
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return INVALID_HANDLE_VALUE;
}


DWORD
STDCALL
CreateTapePartition (
	HANDLE	hDevice,
	DWORD	dwPartitionMethod,
	DWORD	dwCount,
	DWORD	dwSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
CreateVirtualBuffer (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
DebugActiveProcess (
	DWORD	dwProcessId
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


VOID
STDCALL
DebugBreak (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}


ATOM
STDCALL
DeleteAtom (
	ATOM	nAtom
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
DisableThreadLibraryCalls (
	HMODULE	hLibModule
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
DisconnectNamedPipe (
	HANDLE	hNamedPipe
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
STDCALL
DuplicateConsoleHandle (
	HANDLE	hConsole,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
EndUpdateResourceW (
	HANDLE	hUpdate,
	WINBOOL	fDiscard
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
EndUpdateResourceA (
	HANDLE	hUpdate,
	WINBOOL	fDiscard
	)
{
	return EndUpdateResourceW(
			hUpdate,
			fDiscard
			);
}


WINBOOL
STDCALL
EnumCalendarInfoW (
    CALINFO_ENUMPROC lpCalInfoEnumProc,
    LCID              Locale,
    CALID             Calendar,
    CALTYPE           CalType
    )
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



WINBOOL
STDCALL
EnumCalendarInfoA (
	CALINFO_ENUMPROC	lpCalInfoEnumProc,
	LCID			Locale,
	CALID			Calendar,
	CALTYPE			CalType
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
EnumDateFormatsW (
	DATEFMT_ENUMPROC	lpDateFmtEnumProc,
	LCID			Locale,
	DWORD			dwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
EnumDateFormatsA (
	DATEFMT_ENUMPROC	lpDateFmtEnumProc,
	LCID			Locale,
	DWORD			dwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
EnumResourceLanguagesW (
	HINSTANCE	hModule,
	LPCWSTR		lpType,
	LPCWSTR		lpName,
	ENUMRESLANGPROC	lpEnumFunc,
	LONG		lParam
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
EnumResourceLanguagesA (
	HINSTANCE	hModule,
	LPCSTR		lpType,
	LPCSTR		lpName,
	ENUMRESLANGPROC	lpEnumFunc,
	LONG		lParam
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
EnumResourceNamesW (
	HINSTANCE	hModule,
	LPCWSTR		lpType,
	ENUMRESNAMEPROC	lpEnumFunc,
	LONG		lParam
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
EnumResourceNamesA (
	HINSTANCE	hModule,
	LPCSTR		lpType,
	ENUMRESNAMEPROC	lpEnumFunc,
	LONG		lParam
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
EnumResourceTypesW (
	HINSTANCE	hModule,
	ENUMRESTYPEPROC	lpEnumFunc,
	LONG		lParam
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



WINBOOL
STDCALL
EnumResourceTypesA (
	HINSTANCE	hModule,
	ENUMRESTYPEPROC	lpEnumFunc,
	LONG		lParam
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
EnumSystemCodePagesW (
	CODEPAGE_ENUMPROC	lpCodePageEnumProc,
	DWORD			dwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
EnumSystemCodePagesA (
	CODEPAGE_ENUMPROC	lpCodePageEnumProc,
	DWORD			dwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
EnumSystemLocalesW (
	LOCALE_ENUMPROC	lpLocaleEnumProc,
	DWORD		dwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
EnumSystemLocalesA (
	LOCALE_ENUMPROC	lpLocaleEnumProc,
	DWORD		dwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
EnumTimeFormatsW (
	TIMEFMT_ENUMPROC	lpTimeFmtEnumProc,
	LCID			Locale,
	DWORD			dwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
EnumTimeFormatsA (
	TIMEFMT_ENUMPROC	lpTimeFmtEnumProc,
	LCID			Locale,
	DWORD			dwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


DWORD
STDCALL
EraseTape (
	HANDLE	hDevice,
	DWORD	dwEraseType,
	WINBOOL	bImmediate
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
EscapeCommFunction (
	HANDLE	hFile,
	DWORD	dwFunc
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


DWORD
STDCALL
ExitVDM (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
ExpandEnvironmentStringsW (
	LPCWSTR	lpSrc,
	LPWSTR	lpDst,
	DWORD	nSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
ExpandEnvironmentStringsA (
	LPCSTR	lpSrc,
	LPSTR	lpDst,
	DWORD	nSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
ExpungeConsoleCommandHistoryW (
	DWORD	Unknown0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
ExpungeConsoleCommandHistoryA (
	DWORD	Unknown0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


BOOL
STDCALL
ExtendVirtualBuffer (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


VOID
STDCALL
FatalExit (
	  int ExitCode
	  )
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}


ATOM
STDCALL
FindAtomW (
	LPCWSTR lpString
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


ATOM
STDCALL
FindAtomA (
	LPCSTR lpString
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
FindCloseChangeNotification (
	HANDLE	hChangeHandle
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


HANDLE
STDCALL
FindFirstChangeNotificationW (
	LPCWSTR	lpPathName,
	WINBOOL	bWatchSubtree,
	DWORD	dwNotifyFilter
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return INVALID_HANDLE_VALUE;
}


HANDLE
STDCALL
FindFirstChangeNotificationA (
	LPCSTR	lpPathName,
	WINBOOL	bWatchSubtree,
	DWORD	dwNotifyFilter
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return INVALID_HANDLE_VALUE;
}


WINBOOL
STDCALL
FindNextChangeNotification (
	HANDLE	hChangeHandle
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


HRSRC
STDCALL
FindResourceW (
	HINSTANCE	hModule,
	LPCWSTR		lpName,
	LPCWSTR		lpType
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return INVALID_HANDLE_VALUE;
}


HRSRC
STDCALL
FindResourceA (
	HINSTANCE	hModule,
	LPCSTR		lpName,
	LPCSTR		lpType
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return INVALID_HANDLE_VALUE;
}


HRSRC
STDCALL
FindResourceExW (
	HINSTANCE	hModule,
	LPCWSTR		lpType,
	LPCWSTR		lpName,
	WORD		wLanguage
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return INVALID_HANDLE_VALUE;
}


HRSRC
STDCALL
FindResourceExA (
	HINSTANCE	hModule,
	LPCSTR		lpType,
	LPCSTR		lpName,
	WORD		wLanguage
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return INVALID_HANDLE_VALUE;
}



WINBOOL
STDCALL
FlushViewOfFile (
	LPCVOID	lpBaseAddress,
	DWORD	dwNumberOfBytesToFlush
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


int
STDCALL
FoldStringW (
	DWORD	dwMapFlags,
	LPCWSTR	lpSrcStr,
	int	cchSrc,
	LPWSTR	lpDestStr,
	int	cchDest
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



int
STDCALL
FoldStringA (
	DWORD	dwMapFlags,
	LPCSTR	lpSrcStr,
	int	cchSrc,
	LPSTR	lpDestStr,
	int	cchDest
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


   
DWORD
STDCALL
FormatMessageW (
	DWORD	dwFlags,
	LPCVOID	lpSource,
	DWORD	dwMessageId,
	DWORD	dwLanguageId,
	LPWSTR	lpBuffer,
	DWORD	nSize,
	va_list	* Arguments
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
FormatMessageA (
	DWORD	dwFlags,
	LPCVOID	lpSource,
	DWORD	dwMessageId,
	DWORD	dwLanguageId,
	LPSTR	lpBuffer,
	DWORD	nSize,
	va_list	* Arguments
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


VOID
STDCALL
FreeLibraryAndExitThread (
	HMODULE	hLibModule,
	DWORD	dwExitCode
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}


WINBOOL
STDCALL
FreeResource (
	HGLOBAL	hResData
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
STDCALL
FreeVirtualBuffer (
	HANDLE	hVirtualBuffer
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


UINT
STDCALL
GetACP (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



UINT
STDCALL
GetAtomNameW (
	ATOM	nAtom,
	LPWSTR	lpBuffer,
	int	nSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


UINT
STDCALL
GetAtomNameA (
	ATOM	nAtom,
	LPSTR	lpBuffer,
	int	nSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
GetBinaryTypeW (
	LPCWSTR	lpApplicationName,
	LPDWORD	lpBinaryType
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
GetBinaryTypeA (
	LPCSTR	lpApplicationName,
	LPDWORD	lpBinaryType
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
GetCPInfo (
	UINT		a0,
	LPCPINFO	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
GetCommConfig (
	HANDLE		hCommDev,
	LPCOMMCONFIG	lpCC,
	LPDWORD		lpdwSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
GetCommMask (
	HANDLE	hFile,
	LPDWORD	lpEvtMask
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



WINBOOL
STDCALL
GetCommModemStatus (
	HANDLE	hFile,
	LPDWORD	lpModemStat
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
GetCommProperties (
	HANDLE		hFile,
	LPCOMMPROP	lpCommProp
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
GetCommState (
	HANDLE hFile,
	LPDCB lpDCB
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
GetCommTimeouts (
	HANDLE		hFile,
	LPCOMMTIMEOUTS	lpCommTimeouts
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
GetComputerNameW (
	LPWSTR lpBuffer,
	LPDWORD nSize
	)
{
	WCHAR	Name [MAX_COMPUTERNAME_LENGTH + 1];
	DWORD	Size = 0;
	
	/*
	 * FIXME: get the computer's name from
	 * the registry.
	 */
	lstrcpyW( Name, L"ROSHost" ); /* <-- FIXME -- */
	Size = lstrlenW(Name) + 1;
	if (Size > *nSize)
	{
		*nSize = Size;
		SetLastError(ERROR_BUFFER_OVERFLOW);
		return FALSE;
	}
	lstrcpyW( lpBuffer, Name );
	return TRUE;
}


WINBOOL
STDCALL
GetComputerNameA (
	LPSTR	lpBuffer,
	LPDWORD	nSize
	)
{
	WCHAR	Name [MAX_COMPUTERNAME_LENGTH + 1];
	int i;

	if (FALSE == GetComputerNameW(
			Name,
			nSize
			))
	{
		return FALSE;
	}
/* FIXME --> */
/* Use UNICODE to ANSI */
	for ( i=0; Name[i]; ++i )
	{
		lpBuffer[i] = (CHAR) Name[i];
	}
	lpBuffer[i] = '\0';
/* FIXME <-- */
	return TRUE;
}


DWORD
STDCALL
GetConsoleAliasW (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetConsoleAliasA (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



DWORD
STDCALL
GetConsoleAliasExesW (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



DWORD
STDCALL
GetConsoleAliasExesA (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



DWORD
STDCALL
GetConsoleAliasExesLengthA (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



DWORD
STDCALL
GetConsoleAliasExesLengthW (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



DWORD
STDCALL
GetConsoleAliasesW (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



DWORD
STDCALL
GetConsoleAliasesA (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



DWORD
STDCALL
GetConsoleAliasesLengthW (
	DWORD Unknown0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



DWORD
STDCALL
GetConsoleAliasesLengthA (
	DWORD Unknown0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetConsoleCommandHistoryW (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetConsoleCommandHistoryA (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetConsoleCommandHistoryLengthW (
	DWORD	Unknown0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetConsoleCommandHistoryLengthA (
	DWORD	Unknown0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetConsoleDisplayMode (
	DWORD	Unknown0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetConsoleFontInfo (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetConsoleFontSize (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetConsoleHardwareState (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetConsoleInputWaitHandle (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


int
STDCALL
GetCurrencyFormatW (
	LCID			Locale,
	DWORD			dwFlags,
	LPCWSTR			lpValue,
	CONST CURRENCYFMT	* lpFormat,
	LPWSTR			lpCurrencyStr,
	int			cchCurrency
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
GetCurrencyFormatA (
	LCID			Locale,
	DWORD			dwFlags,
	LPCSTR			lpValue,
	CONST CURRENCYFMT	* lpFormat,
	LPSTR			lpCurrencyStr,
	int			cchCurrency
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetCurrentConsoleFont (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
GetDateFormatW (
	LCID			Locale,
	DWORD			dwFlags,
	CONST SYSTEMTIME	* lpDate,
	LPCWSTR			lpFormat,
	LPWSTR			lpDateStr,
	int			cchDate
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
GetDateFormatA (
	LCID			Locale,
	DWORD			dwFlags,
	CONST SYSTEMTIME	* lpDate,
	LPCSTR			lpFormat,
	LPSTR			lpDateStr,
	int			cchDate
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
GetDefaultCommConfigW (
	LPCWSTR		lpszName,
	LPCOMMCONFIG	lpCC,
	LPDWORD		lpdwSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
GetDefaultCommConfigA (
	LPCSTR		lpszName,
	LPCOMMCONFIG	lpCC,
	LPDWORD		lpdwSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


int
STDCALL
GetLocaleInfoW (
	LCID	Locale,
	LCTYPE	LCType,
	LPWSTR	lpLCData,
	int	cchData
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
GetLocaleInfoA (
	LCID	Locale,
	LCTYPE	LCType,
	LPSTR	lpLCData,
	int	cchData
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
GetMailslotInfo (
	HANDLE	hMailslot,
	LPDWORD	lpMaxMessageSize,
	LPDWORD	lpNextSize,
	LPDWORD	lpMessageCount,
	LPDWORD	lpReadTimeout
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


DWORD
STDCALL
GetModuleFileNameW (
	HINSTANCE	hModule,
	LPWSTR		lpFilename,
	DWORD		nSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetModuleFileNameA (
	HINSTANCE	hModule,
	LPSTR		lpFilename,
	DWORD		nSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HMODULE
STDCALL
GetModuleHandleW (
	LPCWSTR	lpModuleName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return INVALID_HANDLE_VALUE;
}


HMODULE
STDCALL
GetModuleHandleA (
	LPCSTR	lpModuleName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return INVALID_HANDLE_VALUE;
}


WINBOOL
STDCALL
GetNamedPipeHandleStateW (
	HANDLE	hNamedPipe,
	LPDWORD	lpState,
	LPDWORD	lpCurInstances,
	LPDWORD	lpMaxCollectionCount,
	LPDWORD	lpCollectDataTimeout,
	LPWSTR	lpUserName,
	DWORD	nMaxUserNameSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
GetNamedPipeHandleStateA (
	HANDLE	hNamedPipe,
	LPDWORD	lpState,
	LPDWORD	lpCurInstances,
	LPDWORD	lpMaxCollectionCount,
	LPDWORD	lpCollectDataTimeout,
	LPSTR	lpUserName,
	DWORD	nMaxUserNameSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
GetNamedPipeInfo (
	HANDLE	hNamedPipe,
	LPDWORD	lpFlags,
	LPDWORD	lpOutBufferSize,
	LPDWORD	lpInBufferSize,
	LPDWORD	lpMaxInstances
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


DWORD
STDCALL
GetNextVDMCommand (
	DWORD	Unknown0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
GetNumberFormatW (
	LCID		Locale,
	DWORD		dwFlags,
	LPCWSTR		lpValue,
	CONST NUMBERFMT	* lpFormat,
	LPWSTR		lpNumberStr,
	int		cchNumber
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
GetNumberFormatA (
	LCID		Locale,
	DWORD		dwFlags,
	LPCSTR		lpValue,
	CONST NUMBERFMT	* lpFormat,
	LPSTR		lpNumberStr,
	int		cchNumber
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
GetNumberOfConsoleFonts (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 1; /* FIXME: call csrss.exe */
}


UINT
STDCALL
GetOEMCP (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 437; /* FIXME: call csrss.exe */
}


DWORD
STDCALL
GetPriorityClass (
	HANDLE	hProcess
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return NORMAL_PRIORITY_CLASS; /* FIXME: call NTOSKRNL.Ps */
}



UINT
STDCALL
GetPrivateProfileIntW (
	LPCWSTR	lpAppName,
	LPCWSTR	lpKeyName,
	INT	nDefault,
	LPCWSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


UINT
STDCALL
GetPrivateProfileIntA (
	LPCSTR	lpAppName,
	LPCSTR	lpKeyName,
	INT	nDefault,
	LPCSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetPrivateProfileSectionW (
	LPCWSTR	lpAppName,
	LPWSTR	lpReturnedString,
	DWORD	nSize,
	LPCWSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetPrivateProfileSectionA (
	LPCSTR	lpAppName,
	LPSTR	lpReturnedString,
	DWORD	nSize,
	LPCSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetPrivateProfileSectionNamesW (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetPrivateProfileSectionNamesA (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetPrivateProfileStringW (
	LPCWSTR lpAppName,
	LPCWSTR lpKeyName,
	LPCWSTR lpDefault,
	LPWSTR	lpReturnedString,
	DWORD	nSize,
	LPCWSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetPrivateProfileStringA (
	LPCSTR	lpAppName,
	LPCSTR	lpKeyName,
	LPCSTR	lpDefault,
	LPSTR	lpReturnedString,
	DWORD	nSize,
	LPCSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetPrivateProfileStructW (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetPrivateProfileStructA (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
GetProcessAffinityMask (
	HANDLE	hProcess,
	LPDWORD	lpProcessAffinityMask,
	LPDWORD lpSystemAffinityMask
	)
{
	if (	(NULL == lpProcessAffinityMask)
		|| (NULL == lpSystemAffinityMask)
		)
	{
		SetLastError(ERROR_BAD_ARGUMENTS);
		return FALSE;
	}
	/* FIXME: check hProcess is actually a process */
	/* FIXME: query the kernel process object */
	*lpProcessAffinityMask = 0x00000001;
	*lpSystemAffinityMask  = 0x00000001;
	return TRUE;
}


WINBOOL
STDCALL
GetProcessShutdownParameters (
	LPDWORD	lpdwLevel,
	LPDWORD	lpdwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


DWORD
STDCALL
GetProcessVersion (
	DWORD	Unknown0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
GetProcessWorkingSetSize (
	HANDLE	hProcess,
	LPDWORD	lpMinimumWorkingSetSize,
	LPDWORD	lpMaximumWorkingSetSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


UINT
STDCALL
GetProfileIntW (
	LPCWSTR	lpAppName,
	LPCWSTR	lpKeyName,
	INT	nDefault
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


UINT
STDCALL
GetProfileIntA (
	LPCSTR	lpAppName,
	LPCSTR	lpKeyName,
	INT	nDefault
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetProfileSectionW (
	LPCWSTR	lpAppName,
	LPWSTR	lpReturnedString,
	DWORD	nSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetProfileSectionA (
	LPCSTR	lpAppName,
	LPSTR	lpReturnedString,
	DWORD	nSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetProfileStringW (
	LPCWSTR	lpAppName,
	LPCWSTR	lpKeyName,
	LPCWSTR	lpDefault,
	LPWSTR	lpReturnedString,
	DWORD	nSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetProfileStringA (
	LPCSTR	lpAppName,
	LPCSTR	lpKeyName,
	LPCSTR	lpDefault,
	LPSTR	lpReturnedString,
	DWORD	nSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
GetStringTypeExW (
	LCID	Locale,
	DWORD	dwInfoType,
	LPCWSTR	lpSrcStr,
	int	cchSrc,
	LPWORD	lpCharType
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
GetStringTypeExA (
	LCID	Locale,
	DWORD	dwInfoType,
	LPCSTR	lpSrcStr,
	int	cchSrc,
	LPWORD	lpCharType
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
GetStringTypeW (
	DWORD	dwInfoType,
	LPCWSTR	lpSrcStr,
	int	cchSrc,
	LPWORD	lpCharType
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
GetStringTypeA (
	LCID	Locale,
	DWORD	dwInfoType,
	LPCSTR	lpSrcStr,
	int	cchSrc,
	LPWORD	lpCharType
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


LCID
STDCALL
GetSystemDefaultLCID (VOID)
{
	/* FIXME: ??? */
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return MAKELCID(
		LANG_ENGLISH,
		SORT_DEFAULT
		);
}


LANGID
STDCALL
GetSystemDefaultLangID (VOID)
{
	 /* FIXME: ??? */
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return MAKELANGID(
		LANG_ENGLISH,
		SUBLANG_ENGLISH_US
		);
}


VOID
STDCALL
GetSystemInfo (
	LPSYSTEM_INFO	lpSystemInfo
	)
{
	/* FIXME: ??? */
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

	lpSystemInfo->u.s.wProcessorArchitecture = 
		PROCESSOR_ARCHITECTURE_INTEL;
	lpSystemInfo->u.s.wReserved = 
		0;
	lpSystemInfo->dwPageSize = 
		4096; /* FIXME: x86 default; may be 4Mb  */
	lpSystemInfo->lpMinimumApplicationAddress = 
		(PVOID) 0x00010000; /* ? */
	lpSystemInfo->lpMaximumApplicationAddress = 
		(PVOID) 0x00010000; /* ? */
	lpSystemInfo->dwActiveProcessorMask = 
		0x00000001;
	lpSystemInfo->dwNumberOfProcessors = 
		1; /* ReactOS is UP for now! */
	lpSystemInfo->dwProcessorType = 
		PROCESSOR_INTEL_PENTIUM;
	lpSystemInfo->dwAllocationGranularity = 
		65536; /* hard coded on Intel? */
	lpSystemInfo->wProcessorLevel = 
		5; /* from cpuid on Intel? */
	lpSystemInfo->wProcessorRevision = 
		0x0000; /* from cpuid on Intel? */
}


DWORD
STDCALL
GetSystemPowerStatus (
	DWORD	Unknown0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetTapeParameters (
	HANDLE	hDevice,
	DWORD	dwOperation,
	LPDWORD	lpdwSize,
	LPVOID	lpTapeInformation
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetTapeStatus (
	HANDLE	hDevice
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


LCID
STDCALL
GetThreadLocale (VOID)
{
	/* FIXME: ??? */
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return MAKELCID(
		LANG_ENGLISH,
		SORT_DEFAULT
		);
}


WINBOOL
STDCALL
GetThreadSelectorEntry (
	HANDLE		hThread,
	DWORD		dwSelector,
	LPLDT_ENTRY	lpSelectorEntry
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


int
STDCALL
GetTimeFormatW (
	LCID			Locale,
	DWORD			dwFlags,
	CONST SYSTEMTIME	* lpTime,
	LPCWSTR			lpFormat,
	LPWSTR			lpTimeStr,
	int			cchTime
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
GetTimeFormatA (
	LCID			Locale,
	DWORD			dwFlags,
	CONST SYSTEMTIME	* lpTime,
	LPCSTR			lpFormat,
	LPSTR			lpTimeStr,
	int			cchTime
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


LCID
STDCALL
GetUserDefaultLCID (VOID)
{
	/* FIXME: ??? */
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return MAKELCID(
		LANG_ENGLISH,
		SORT_DEFAULT
		);
}


LANGID
STDCALL
GetUserDefaultLangID (VOID)
{
	 /* FIXME: ??? */
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return MAKELANGID(
		LANG_ENGLISH,
		SUBLANG_ENGLISH_US
		);
}


DWORD
STDCALL
GetVDMCurrentDirectories (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


ATOM
STDCALL
GlobalAddAtomW (
	LPCWSTR	lpString
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


ATOM
STDCALL
GlobalAddAtomA (
	LPCSTR	lpString
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HGLOBAL
STDCALL
GlobalAlloc (
	UINT	uFlags,
	DWORD	dwBytes
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


UINT
STDCALL
GlobalCompact (
	DWORD	dwMinFree
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


ATOM
STDCALL
GlobalDeleteAtom (
	ATOM	nAtom
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


ATOM
STDCALL
GlobalFindAtomW (
	LPCWSTR	lpString
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


ATOM
STDCALL
GlobalFindAtomA (
	LPCSTR	lpString
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


VOID
STDCALL
GlobalFix (
	HGLOBAL	hMem
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}


UINT
STDCALL
GlobalFlags (
	HGLOBAL	hMem
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HGLOBAL
STDCALL
GlobalFree (
	HGLOBAL	hMem
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return hMem;
}


UINT
STDCALL
GlobalGetAtomNameA (
	ATOM	nAtom,
	LPSTR	lpBuffer,
	int	nSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


UINT
STDCALL
GlobalGetAtomNameW (
	ATOM	nAtom,
	LPWSTR	lpBuffer,
	int	nSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HGLOBAL
STDCALL
GlobalHandle (
	LPCVOID	pMem
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


LPVOID
STDCALL
GlobalLock (
	HGLOBAL hMem
	)
{
	/* In Win32 GlobalAlloc returns LPVOID? */
	return hMem;
}


VOID
STDCALL
GlobalMemoryStatus (
	LPMEMORYSTATUS	lpBuffer
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}


HGLOBAL
STDCALL
GlobalReAlloc (
	HGLOBAL	hMem,
	DWORD	dwBytes,
	UINT	uFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GlobalSize (
	HGLOBAL	hMem
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
GlobalUnWire (
	HGLOBAL	hMem
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


VOID
STDCALL
GlobalUnfix (
	HGLOBAL	hMem
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}


WINBOOL
STDCALL
GlobalUnlock (
	HGLOBAL	hMem
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


LPVOID
STDCALL
GlobalWire (
	HGLOBAL	hMem
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return NULL; /* ? */
}


DWORD
STDCALL
HeapCreateTagsW (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
HeapExtend (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
HeapQueryTagW (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
HeapSummary (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
HeapUsage (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
HeapWalk (
	HANDLE			hHeap,
	LPPROCESS_HEAP_ENTRY	lpEntry
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
InitAtomTable (
	DWORD nSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


LONG
STDCALL
InterlockedDecrement (
	LPLONG	lpAddend
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


LONG
STDCALL
InterlockedExchange (
	LPLONG	Target,
	LONG	Value
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


LONG
STDCALL
InterlockedIncrement (
	LPLONG	lpAddend
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
InvalidateConsoleDIBits (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
IsBadCodePtr (
	FARPROC	lpfn
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
IsBadHugeReadPtr (
	CONST VOID	* lp,
	UINT		ucb
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
IsBadHugeWritePtr (
	LPVOID	lp,
	UINT	ucb
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
IsBadReadPtr (
	CONST VOID	* lp,
	UINT		ucb
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
IsBadStringPtrA (
	LPCSTR	lpsz,
	UINT	ucchMax
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
IsBadStringPtrW (
	LPCWSTR	lpsz,
	UINT	ucchMax
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
IsBadWritePtr (
	LPVOID	lp,
	UINT	ucb
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
IsDBCSLeadByte (
	BYTE	TestChar
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
IsDBCSLeadByteEx (
	UINT	CodePage,
	BYTE	TestChar
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
STDCALL
IsDebuggerPresent (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE; /* FIXME: */
}


WINBOOL
STDCALL
IsValidCodePage (
	UINT	CodePage
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
IsValidLocale (
	LCID	Locale,
	DWORD	dwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


int
STDCALL
LCMapStringA (
	LCID	Locale,
	DWORD	dwMapFlags,
	LPCSTR	lpSrcStr,
	int	cchSrc,
	LPSTR	lpDestStr,
	int	cchDest
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
LCMapStringW (
	LCID	Locale,
	DWORD	dwMapFlags,
	LPCWSTR	lpSrcStr,
	int	cchSrc,
	LPWSTR	lpDestStr,
	int	cchDest
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HINSTANCE
STDCALL
LoadLibraryExA (
	LPCSTR	lpLibFileName,
	HANDLE	hFile,
	DWORD	dwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HINSTANCE
STDCALL
LoadLibraryExW (
	LPCWSTR	lpLibFileName,
	HANDLE	hFile,
	DWORD	dwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
LoadModule (
	LPCSTR	lpModuleName,
	LPVOID	lpParameterBlock
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HGLOBAL
STDCALL
LoadResource (
	HINSTANCE	hModule,
	HRSRC		hResInfo
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HLOCAL
STDCALL
LocalAlloc (
	UINT	uFlags,
	UINT	uBytes
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


UINT
STDCALL
LocalCompact (
	UINT	uMinFree
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


UINT
STDCALL
LocalFlags (
	HLOCAL	hMem
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HLOCAL
STDCALL
LocalFree (
	HLOCAL	hMem
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return hMem;
}


HLOCAL
STDCALL
LocalHandle (
	LPCVOID	pMem
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


LPVOID
STDCALL
LocalLock (
	HLOCAL	hMem
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return NULL;
}


HLOCAL
STDCALL
LocalReAlloc (
	HLOCAL	hMem,
	UINT	uBytes,
	UINT	uFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


UINT
STDCALL
LocalShrink (
	HLOCAL	hMem,
	UINT	cbNewSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


UINT
STDCALL
LocalSize (
	HLOCAL	hMem
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
LocalUnlock (
	HLOCAL	hMem
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


LPVOID
STDCALL
LockResource (
	HGLOBAL	hResData
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return NULL;
}


LPVOID
STDCALL
MapViewOfFile (
	HANDLE	hFileMappingObject,
	DWORD	dwDesiredAccess,
	DWORD	dwFileOffsetHigh,
	DWORD	dwFileOffsetLow,
	DWORD	dwNumberOfBytesToMap
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return NULL;
}


LPVOID
STDCALL
MapViewOfFileEx (
	HANDLE	hFileMappingObject,
	DWORD	dwDesiredAccess,
	DWORD	dwFileOffsetHigh,
	DWORD	dwFileOffsetLow,
	DWORD	dwNumberOfBytesToMap,
	LPVOID	lpBaseAddress
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return NULL;
}


int
STDCALL
MulDiv (
	int	nNumber,
	int	nNumerator,
	int	nDenominator
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
MultiByteToWideChar (
	UINT	CodePage,
	DWORD	dwFlags,
	LPCSTR	lpMultiByteStr,
	int	cchMultiByte,
	LPWSTR	lpWideCharStr,
	int	cchWideChar
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
OpenConsoleW (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HANDLE
STDCALL
OpenFileMappingA (
	DWORD	dwDesiredAccess,
	WINBOOL	bInheritHandle,
	LPCSTR	lpName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return INVALID_HANDLE_VALUE;
}


HANDLE
STDCALL
OpenFileMappingW (
	DWORD	dwDesiredAccess,
	WINBOOL	bInheritHandle,
	LPCWSTR	lpName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return INVALID_HANDLE_VALUE;
}


HANDLE
STDCALL
OpenMutexA (
	DWORD	dwDesiredAccess,
	WINBOOL	bInheritHandle,
	LPCSTR	lpName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return INVALID_HANDLE_VALUE;
}


HANDLE
STDCALL
OpenMutexW (
	DWORD	dwDesiredAccess,
	WINBOOL	bInheritHandle,
	LPCWSTR	lpName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return INVALID_HANDLE_VALUE;
}


DWORD
STDCALL
OpenProfileUserMapping (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HANDLE
STDCALL
OpenSemaphoreA (
	DWORD	dwDesiredAccess,
	WINBOOL	bInheritHandle,
	LPCSTR	lpName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return INVALID_HANDLE_VALUE;
}


HANDLE
STDCALL
OpenSemaphoreW (
	DWORD	dwDesiredAccess,
	WINBOOL	bInheritHandle,
	LPCWSTR	lpName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return INVALID_HANDLE_VALUE;
}


WINBOOL
STDCALL
PeekNamedPipe (
	HANDLE	hNamedPipe,
	LPVOID	lpBuffer,
	DWORD	nBufferSize,
	LPDWORD	lpBytesRead,
	LPDWORD	lpTotalBytesAvail,
	LPDWORD	lpBytesLeftThisMessage
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


DWORD
STDCALL
PrepareTape (
	HANDLE	hDevice,
	DWORD	dwOperation,
	WINBOOL	bImmediate
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
PurgeComm (
	HANDLE	hFile,
	DWORD	dwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
QueryPerformanceCounter (
	LARGE_INTEGER	* lpPerformanceCount
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
QueryPerformanceFrequency (
	LARGE_INTEGER	* lpFrequency
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
QueryWin31IniFilesMappedToRegistry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


VOID
STDCALL
RaiseException (
	DWORD		dwExceptionCode,
	DWORD		dwExceptionFlags,
	DWORD		nNumberOfArguments,
	CONST DWORD	* lpArguments
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}


WINBOOL
STDCALL
ReadProcessMemory (
	HANDLE	hProcess,
	LPCVOID	lpBaseAddress,
	LPVOID	lpBuffer,
	DWORD	nSize,
	LPDWORD	lpNumberOfBytesRead
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
RegisterConsoleVDM (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5,
	DWORD	Unknown6,
	DWORD	Unknown7,
	DWORD	Unknown8,
	DWORD	Unknown9,
	DWORD	Unknown10
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
RegisterWaitForInputIdle (
	DWORD	Unknown0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
RegisterWowBaseHandlers (
	DWORD	Unknown0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
RegisterWowExec (
	DWORD	Unknown0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
ReleaseMutex (
	HANDLE	hMutex
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
ReleaseSemaphore (
	HANDLE	hSemaphore,
	LONG	lReleaseCount,
	LPLONG	lpPreviousCount
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/* FIXME: KERNEL32.RtlFillMemory == NTDLL.RtlFillMemory */
DWORD
STDCALL
RtlFillMemory (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	return 0;
}


/* FIXME: KERNEL32.RtlMoveMemory == NTDLL.RtlMoveMemory */
DWORD
STDCALL
RtlMoveMemory (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	return 0;
}


/* FIXME: KERNEL32.RtlUnwind == NTDLL.RtlUnwind */
DWORD
STDCALL
RtlUnwind (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
	return 0;
}


/* FIXME: KERNEL32.RtlZeroMemory == NTDLL.RtlZeroMemory */
DWORD
STDCALL
RtlZeroMemory (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	return 0;
}


WINBOOL
STDCALL
SetCommBreak (
	HANDLE	hFile
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetCommConfig (
	HANDLE		hCommDev,
	LPCOMMCONFIG	lpCC,
	DWORD		dwSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetCommMask (
	HANDLE	hFile,
	DWORD	dwEvtMask
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetCommState (
	HANDLE	hFile,
	LPDCB	lpDCB
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetCommTimeouts (
	HANDLE		hFile,
	LPCOMMTIMEOUTS	lpCommTimeouts
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetComputerNameA (
	LPCSTR	lpComputerName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetComputerNameW (
    LPCWSTR lpComputerName
    )
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetConsoleCommandHistoryMode (
	DWORD	dwMode
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetConsoleCursor (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetConsoleDisplayMode (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetConsoleFont (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetConsoleHardwareState (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetConsoleKeyShortcuts (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetConsoleMaximumWindowSize (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetConsoleMenuClose (
	DWORD	Unknown0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetConsoleNumberOfCommandsA (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetConsoleNumberOfCommandsW (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetConsolePalette (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetDefaultCommConfigA (
	LPCSTR		lpszName,
	LPCOMMCONFIG	lpCC,
	DWORD		dwSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetDefaultCommConfigW (
	LPCWSTR		lpszName,
	LPCOMMCONFIG	lpCC,
	DWORD		dwSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetLastConsoleEventActive (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetLocaleInfoA (
	LCID	Locale,
	LCTYPE	LCType,
	LPCSTR	lpLCData
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetLocaleInfoW (
	LCID	Locale,
	LCTYPE	LCType,
	LPCWSTR	lpLCData
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetMailslotInfo (
	HANDLE	hMailslot,
	DWORD	lReadTimeout
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetNamedPipeHandleState (
	HANDLE	hNamedPipe,
	LPDWORD	lpMode,
	LPDWORD	lpMaxCollectionCount,
	LPDWORD	lpCollectDataTimeout
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetPriorityClass (
	HANDLE	hProcess,
	DWORD	dwPriorityClass
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetProcessShutdownParameters (
	DWORD	dwLevel,
	DWORD	dwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetProcessWorkingSetSize (
	HANDLE	hProcess,
	DWORD	dwMinimumWorkingSetSize,
	DWORD	dwMaximumWorkingSetSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetSystemPowerState (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


DWORD
STDCALL
SetTapeParameters (
	HANDLE	hDevice,
	DWORD	dwOperation,
	LPVOID	lpTapeInformation
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
SetTapePosition (
	HANDLE	hDevice,
	DWORD	dwPositionMethod,
	DWORD	dwPartition,
	DWORD	dwOffsetLow,
	DWORD	dwOffsetHigh,
	WINBOOL	bImmediate
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetThreadLocale (
	LCID	Locale
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetVDMCurrentDirectories (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetupComm (
	HANDLE	hFile,
	DWORD	dwInQueue,
	DWORD	dwOutQueue
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


DWORD
STDCALL
ShowConsoleCursor (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
SizeofResource (
	HINSTANCE	hModule,
	HRSRC		hResInfo
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
TerminateThread (
	HANDLE	hThread,
	DWORD	dwExitCode
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


LPVOID
STDCALL
TlsGetValue (
	DWORD	dwTlsIndex
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return NULL;
}


WINBOOL
STDCALL
TransactNamedPipe (
	HANDLE		hNamedPipe,
	LPVOID		lpInBuffer,
	DWORD		nInBufferSize,
	LPVOID		lpOutBuffer,
	DWORD		nOutBufferSize,
	LPDWORD		lpBytesRead,
	LPOVERLAPPED	lpOverlapped
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
TransmitCommChar (
	HANDLE	hFile,
	char	cChar
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


DWORD
STDCALL
TrimVirtualBuffer (
	DWORD	Unknown0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
UnmapViewOfFile (
	LPVOID	lpBaseAddress
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
UpdateResourceA (
	HANDLE	hUpdate,
	LPCSTR	lpType,
	LPCSTR	lpName,
	WORD	wLanguage,
	LPVOID	lpData,
	DWORD	cbData
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
UpdateResourceW (
	HANDLE	hUpdate,
	LPCWSTR	lpType,
	LPCWSTR	lpName,
	WORD	wLanguage,
	LPVOID	lpData,
	DWORD	cbData
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


DWORD
STDCALL
VDMConsoleOperation (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
VDMOperationStarted (
	DWORD	Unknown0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
VerLanguageNameA (
	DWORD	wLang,
	LPSTR	szLang,
	DWORD	nSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
VerLanguageNameW (
	DWORD	wLang,
	LPWSTR	szLang,
	DWORD	nSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
VerifyConsoleIoHandle (
	DWORD	Unknown0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
VirtualBufferExceptionHandler (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
VirtualLock (
	LPVOID	lpAddress,
	DWORD	dwSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


DWORD
STDCALL
VirtualQuery (
	LPCVOID				lpAddress,
	PMEMORY_BASIC_INFORMATION	lpBuffer,
	DWORD				dwLength
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
VirtualQueryEx (
	HANDLE				hProcess,
	LPCVOID				lpAddress,
	PMEMORY_BASIC_INFORMATION	lpBuffer,
	DWORD				dwLength
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
VirtualUnlock (
	LPVOID	lpAddress,
	DWORD	dwSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
WaitCommEvent (
	HANDLE		hFile,
	LPDWORD		lpEvtMask,
	LPOVERLAPPED	lpOverlapped
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
WaitForDebugEvent (
	LPDEBUG_EVENT	lpDebugEvent,
	DWORD		dwMilliseconds
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
WaitNamedPipeA (
	LPCSTR	lpNamedPipeName,
	DWORD	nTimeOut
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
WaitNamedPipeW (
	LPCWSTR	lpNamedPipeName,
	DWORD	nTimeOut
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


int
STDCALL
WideCharToMultiByte (
	UINT	CodePage,
	DWORD	dwFlags,
	LPCWSTR	lpWideCharStr,
	int	cchWideChar,
	LPSTR	lpMultiByteStr,
	int	cchMultiByte,
	LPCSTR	lpDefaultChar,
	LPBOOL	lpUsedDefaultChar
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


DWORD
STDCALL
WriteConsoleInputVDMA (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
WriteConsoleInputVDMW (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
WriteFileEx (
	HANDLE				hFile,
	LPCVOID				lpBuffer,
	DWORD				nNumberOfBytesToWrite,
	LPOVERLAPPED			lpOverlapped,
	LPOVERLAPPED_COMPLETION_ROUTINE	lpCompletionRoutine
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
WritePrivateProfileSectionA (
	LPCSTR	lpAppName,
	LPCSTR	lpString,
	LPCSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
WritePrivateProfileSectionW (
	LPCWSTR	lpAppName,
	LPCWSTR	lpString,
	LPCWSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
WritePrivateProfileStringA (
	LPCSTR	lpAppName,
	LPCSTR	lpKeyName,
	LPCSTR	lpString,
	LPCSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
WritePrivateProfileStringW (
	LPCWSTR	lpAppName,
	LPCWSTR	lpKeyName,
	LPCWSTR	lpString,
	LPCWSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
WritePrivateProfileStructA (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
WritePrivateProfileStructW (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
WriteProcessMemory (
	HANDLE	hProcess,
	LPVOID	lpBaseAddress,
	LPVOID	lpBuffer,
	DWORD	nSize,
	LPDWORD	lpNumberOfBytesWritten
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
WriteProfileSectionA (
	LPCSTR	lpAppName,
	LPCSTR	lpString
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
WriteProfileSectionW (
	LPCWSTR	lpAppName,
	LPCWSTR	lpString
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
WriteProfileStringA (
	LPCSTR	lpAppName,
	LPCSTR	lpKeyName,
	LPCSTR	lpString
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
WriteProfileStringW (
	LPCWSTR	lpAppName,
	LPCWSTR	lpKeyName,
	LPCWSTR	lpString
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


DWORD
STDCALL
WriteTapemark (
	HANDLE	hDevice,
	DWORD	dwTapemarkType,
	DWORD	dwTapemarkCount,
	WINBOOL	bImmediate
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

DWORD
STDCALL
GetTapePosition (
	HANDLE	hDevice,
	DWORD	dwPositionType,
	LPDWORD	lpdwPartition,
	LPDWORD	lpdwOffsetLow,
	LPDWORD	lpdwOffsetHigh
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



/* EOF */
