#include <windows.h>
#include <ddk/ntddk.h>

HANDLE StdInput = NULL;
HANDLE StdOutput = NULL;
HANDLE StdError = NULL;




#define FSCTL_GET_CONSOLE_SCREEN_BUFFER_INFO              CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 254, DO_DIRECT_IO, FILE_READ_ACCESS|FILE_WRITE_ACCESS)
#define FSCTL_SET_CONSOLE_SCREEN_BUFFER_INFO              CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 255, DO_DIRECT_IO, FILE_READ_ACCESS|FILE_WRITE_ACCESS)

HANDLE
STDCALL
GetStdHandle(
	     DWORD nStdHandle
	     )
{
	if ( nStdHandle == STD_INPUT_HANDLE )
		return StdInput;
	if ( nStdHandle == STD_OUTPUT_HANDLE )
		return StdOutput;
	if ( nStdHandle == STD_ERROR_HANDLE )
		return StdError;
/*
	if ( nStdHandle == STD_AUX_HANDLE )
		return StdError;
	if ( nStdHandle == STD_PRINT_HANDLE )
		return StdError;
*/
	return NULL;		
	
}

WINBOOL
STDCALL
WriteConsoleA(
    HANDLE hConsoleOutput,
    CONST VOID *lpBuffer,
    DWORD nNumberOfCharsToWrite,
    LPDWORD lpNumberOfCharsWritten,
    LPVOID lpReserved
    )
{
	return WriteFile(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite,lpNumberOfCharsWritten, lpReserved);
}


WINBOOL
STDCALL
ReadConsoleA(
    HANDLE hConsoleInput,
    LPVOID lpBuffer,
    DWORD nNumberOfCharsToRead,
    LPDWORD lpNumberOfCharsRead,
    LPVOID lpReserved
    )
{
	KEY_EVENT_RECORD *k;
	OVERLAPPED Overlapped;
	OVERLAPPED * lpOverlapped;
	int kSize;
	int i,j;

	if ( lpReserved == NULL ) {
		Overlapped.Internal = 0;
		Overlapped.InternalHigh = 0;
		Overlapped.Offset = 0;
		Overlapped.OffsetHigh = 0;
	//	Overlapped.hEvent = CreateEvent(NULL,FALSE,TRUE,NULL);
		lpOverlapped = &Overlapped;
	}
	else
		lpOverlapped = lpReserved;


	kSize = nNumberOfCharsToRead*sizeof(kSize);
	k = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,kSize);
	if ( k == NULL || kSize == 0 )
		return FALSE;
		
	k[0].AsciiChar = 0;
	while(k[0].AsciiChar == 0 )
	{
		ReadFile(hConsoleInput,k,kSize,lpNumberOfCharsRead,lpOverlapped);

	}
	j = 0;
	i = 0;
	//if ( k[i].bKeyDown ) 
	{
		((char *)lpBuffer)[j] = k[i].AsciiChar;
		j++;
	}
	i++;
	while(j < nNumberOfCharsToRead && i < *lpNumberOfCharsRead ) {
		//if ( k[i].bKeyDown ) 
		{
			((char *)lpBuffer)[j] = k[i].AsciiChar;
			j++;
		}
		i++;
	}

	HeapFree(GetProcessHeap(),0,k);
	//if ( lpReserved == NULL ) {
	//	CloseHandle(Overlapped.hEvent);
	//}
	
}

WINBOOL
STDCALL
AllocConsole( VOID )
{
	StdInput = CreateFile("\\Device\\Keyboard",
			       FILE_GENERIC_READ,
			       0,
			       NULL,
			       OPEN_EXISTING,
			       FILE_FLAG_OVERLAPPED,
			       NULL);

	StdOutput = CreateFile("\\Device\\BlueScreen",
			       FILE_GENERIC_WRITE|FILE_GENERIC_READ,
			       0,
			       NULL,
			       OPEN_EXISTING,
			       0,
			       NULL);

	StdError = StdOutput;

	return TRUE;
}


WINBOOL
STDCALL
FreeConsole( VOID )
{
	CloseHandle(StdInput);
	CloseHandle(StdOutput);
	return TRUE;
}


WINBOOL
STDCALL
GetConsoleScreenBufferInfo(
    HANDLE hConsoleOutput,
    PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo
    )
{
	DWORD dwBytesReturned;
	if( !DeviceIoControl(hConsoleOutput,
		FSCTL_GET_CONSOLE_SCREEN_BUFFER_INFO,
		NULL,0,
		lpConsoleScreenBufferInfo,sizeof(CONSOLE_SCREEN_BUFFER_INFO),&dwBytesReturned,NULL ))
		return FALSE;

	return TRUE;
}

WINBOOL
STDCALL
SetConsoleCursorPosition(
    HANDLE hConsoleOutput,
    COORD dwCursorPosition
    )
{
	DWORD dwBytesReturned;
	CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;

	if( !GetConsoleScreenBufferInfo(hConsoleOutput,&ConsoleScreenBufferInfo) )
		return FALSE;
	ConsoleScreenBufferInfo.dwCursorPosition.X = dwCursorPosition.X;
	ConsoleScreenBufferInfo.dwCursorPosition.Y = dwCursorPosition.Y;
	
	if( !DeviceIoControl(
		hConsoleOutput,
			FSCTL_SET_CONSOLE_SCREEN_BUFFER_INFO,
			&ConsoleScreenBufferInfo,
			sizeof(CONSOLE_SCREEN_BUFFER_INFO),
			NULL,
			0,
			&dwBytesReturned,
			NULL ))
		return FALSE;

	return TRUE;
}

WINBOOL
STDCALL
FillConsoleOutputCharacterA(
    HANDLE hConsoleOutput,
    CHAR  cCharacter,
    DWORD  nLength,
    COORD  dwWriteCoord,
    LPDWORD lpNumberOfCharsWritten
    )
{
	return FillConsoleOutputCharacterW(hConsoleOutput,
    (WCHAR)  cCharacter,nLength, dwWriteCoord,lpNumberOfCharsWritten);
}

WINBOOL
STDCALL
FillConsoleOutputCharacterW(
    HANDLE hConsoleOutput,
    WCHAR  cCharacter,
    DWORD  nLength,
    COORD  dwWriteCoord,
    LPDWORD lpNumberOfCharsWritten
    )
{
		
	return FALSE;
}
