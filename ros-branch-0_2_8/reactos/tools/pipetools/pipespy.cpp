/*
 * PipeSpy
 * Copyright (c) 2004, Skywing (skywing@valhallalegends.com)
 * Released under the GNU GPL for the ReactOS project
 *
 * This program can be used to spy on a named pipe in Windows NT(+)
 * or ReactOS.  It is particularly intended to be used for
 * understanding the remote kernel debugging protocol used
 * by windows kd.
 *
 * USE:
 *
 * PipeSpy \\.\pipe\debug_server \\.\pipe\debug_client > PipeSpy.log
 *
 * VMware: act as client, connect to \\.\pipe\debug_server, pipe is connected
 * to application
 *
 * WinDbg -k com:pipe,port=\\.\pipe\debug_client,resets=0
 *
 * NOTE:
 *
 * This program hasn't really been tested against ReactOS, and has only
 * been built with Visual Studio .NET 2003.
 */
#include <process.h>
#include <windows.h>
#include <stdio.h>

HANDLE g_DebugServerPipe, g_DebugClientPipe;
CRITICAL_SECTION g_OutputLock;

void dumphex(const char *buf, int len, int pos)
{
	int i, j;
	for(j = 0; j < len; j += 16) {
		for(i = 0; i < 16; i++) {
			if(i + j < len)
				printf("%02x%c", (unsigned char)buf[i + j], j + i + 1 == pos ? '*' : ' ');
			else 
				printf("   ");
		}
		for(i = 0; i < 16; i++) {
			if(i + j < len)
				printf("%c", buf[i + j] >= ' ' ? buf[i + j] : '.');
			else 
				printf(" ");
		}
		printf("\n");
	}
}

enum {
	PIPEBUF_INITIAL_SIZE			=	4096,
	PIPEBUF_MAX_SIZE				=	16384
};

typedef struct _READ_OVERLAPPED {
	OVERLAPPED ReadOverlapped;
	char* Buffer;
	HANDLE Pipe, OtherPipe;
	bool Server;
	bool Connected;
} READ_OVERLAPPED;

VOID WINAPI WritePipeCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransferred, LPOVERLAPPED lpOverlapped)
{
	if(dwErrorCode) {
		EnterCriticalSection(&g_OutputLock);
		printf(lpOverlapped->hEvent ? "Server> Error %u writing to client pipe\n" : "Client> Error %u writing to server pipe\n", dwErrorCode);
		LeaveCriticalSection(&g_OutputLock);
	}

	free((void*)lpOverlapped);
}

VOID WINAPI ReadPipeCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransferred, LPOVERLAPPED lpOverlapped)
{
	READ_OVERLAPPED* ReadOverlapped = (READ_OVERLAPPED*)lpOverlapped;
	LPOVERLAPPED WriteOverlapped;

	EnterCriticalSection(&g_OutputLock);

	if(dwErrorCode) {
		printf(ReadOverlapped->Server ? "Server> Error %u reading from pipe\n" : "Client> Error %u reading from pipe\n", dwErrorCode);
		ReadOverlapped->Connected = false;
	} else {
		SYSTEMTIME SystemTime;

		GetLocalTime(&SystemTime);
		printf(ReadOverlapped->Server ? "Server> [%02u:%02u:%02u.%03u] Received %u byte message:\n" : "Client> [%02u:%02u:%02u.%03u] Received %u byte message:\n",
			SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, SystemTime.wMilliseconds, dwNumberOfBytesTransferred);
		dumphex(ReadOverlapped->Buffer, (int)dwNumberOfBytesTransferred, -1);
		printf("\n");
	}

	LeaveCriticalSection(&g_OutputLock);

	WriteOverlapped = (LPOVERLAPPED)malloc(sizeof(OVERLAPPED)+dwNumberOfBytesTransferred);

	if(!WriteOverlapped)
		printf(ReadOverlapped->Server ? "Server> Out of memory\n" : "Client> Out of memory\n");
	else {
		ZeroMemory(WriteOverlapped, sizeof(OVERLAPPED));
		WriteOverlapped->hEvent = (HANDLE)ReadOverlapped->Server;
		memcpy(((char*)WriteOverlapped)+sizeof(OVERLAPPED), ReadOverlapped->Buffer, dwNumberOfBytesTransferred);
		WriteFileEx(ReadOverlapped->OtherPipe, ((char*)WriteOverlapped)+sizeof(OVERLAPPED), dwNumberOfBytesTransferred, WriteOverlapped,
			WritePipeCompletion);
	}

	if(!dwErrorCode) {
		ZeroMemory(ReadOverlapped, sizeof(OVERLAPPED));
		ReadFileEx(ReadOverlapped->Pipe, ReadOverlapped->Buffer, PIPEBUF_INITIAL_SIZE, &ReadOverlapped->ReadOverlapped, ReadPipeCompletion);
	}
}

void __cdecl pipeserver(void* param)
{
	READ_OVERLAPPED ReadOverlapped;

	ReadOverlapped.Pipe = (HANDLE)param, ReadOverlapped.OtherPipe = ReadOverlapped.Pipe == g_DebugServerPipe ? g_DebugClientPipe : g_DebugServerPipe;
	ReadOverlapped.Server = ReadOverlapped.Pipe == g_DebugServerPipe;
	ReadOverlapped.Buffer = (char*)malloc(PIPEBUF_INITIAL_SIZE);

	for(;;) {
		if(!ConnectNamedPipe(ReadOverlapped.Pipe, 0) && GetLastError() != ERROR_PIPE_CONNECTED) {
			printf(ReadOverlapped.Server ? "Server> Error %u accepting pipe connection\n" : "Client> Error %u accepting pipe connection\n",
				GetLastError());
			break;
		}

		ReadOverlapped.Connected = true;
		printf(ReadOverlapped.Server ? "Server> Connected\n" : "Client> Connected\n");

		ZeroMemory(&ReadOverlapped.ReadOverlapped, sizeof(OVERLAPPED));
		ReadFileEx(ReadOverlapped.Pipe, ReadOverlapped.Buffer, PIPEBUF_INITIAL_SIZE, &ReadOverlapped.ReadOverlapped, ReadPipeCompletion);

		do {
			SleepEx(INFINITE, TRUE);
		} while(ReadOverlapped.Connected) ;

		DisconnectNamedPipe(ReadOverlapped.Pipe);
		printf(ReadOverlapped.Server ? "Server> Disconnected\n" : "Client> Disconnected\n");
	}

	printf(ReadOverlapped.Server ? "Server> Shutting down\n" : "Client> Shutting down\n");

	free(ReadOverlapped.Buffer);
	CloseHandle(ReadOverlapped.Pipe);
	SleepEx(0, TRUE);
}

int main(int ac, char **av)
{
	if(ac != 3) {
		printf("syntax: pipespy serverpipe clientpipe\n");
		return 0;
	}

	g_DebugServerPipe = CreateNamedPipe(av[1], PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
		1, 4096, 4096, NMPWAIT_WAIT_FOREVER, 0);

	if(g_DebugServerPipe == INVALID_HANDLE_VALUE) {
		printf("Error %u creating server pipe\n", GetLastError());
		return 0;
	}

	g_DebugClientPipe = CreateNamedPipe(av[2], PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
		1, 4096, 4096, NMPWAIT_WAIT_FOREVER, 0);

	if(g_DebugClientPipe == INVALID_HANDLE_VALUE) {
		printf("Error %u creating client pipe\n", GetLastError());
		CloseHandle(g_DebugServerPipe);
		return 0;
	}

	InitializeCriticalSection(&g_OutputLock);
	_beginthread(pipeserver, 0, (void*)g_DebugServerPipe);
	pipeserver((void*)g_DebugClientPipe);
	DeleteCriticalSection(&g_OutputLock);
	return 0;
}


/*
		while(!Error) {
			SleepEx(0, TRUE);
			BufferPos = 0;

			if(BufferSize > PIPEBUF_MAX_SIZE) {
				char* NewBuf = (char*)realloc(Buffer, PIPEBUF_MAX_SIZE);

				if(NewBuf) {
					Buffer = NewBuf;
					BufferSize = PIPEBUF_MAX_SIZE;
				}
			}

			for(;;) {
				char* NewBuf;

				if(ReadFile(Pipe, Buffer+BufferPos, BufferSize-BufferPos, &Read, 0)) {
					BufferPos += Read;
					Error = 0;
					break;
				}

				Error = GetLastError();

				printf("Error=%u read=%u\n", Error, Read);

				if(Error != ERROR_MORE_DATA) {
					printf(Server ? "Server> Error %u reading from pipe\n" : "Client> Error %u reading from pipe\n", GetLastError());
					break;
				}

				NewBuf = (char*)realloc(Buffer, BufferSize << 1);

				if(!NewBuf) {
					printf(Server ? "Server> Out of memory\n" : "Client> Out of memory\n");
					break;
				}

				BufferSize <<= 1;
				BufferPos += Read;
				Error = 0;
			}

			if(Error)
				break;

			WriteOverlapped = (LPOVERLAPPED)malloc(sizeof(OVERLAPPED)+BufferPos);

			if(!WriteOverlapped)
				printf(Server ? "Server> Out of memory\n" : "Client> Out of memory\n");
			else {
				ZeroMemory(WriteOverlapped, sizeof(OVERLAPPED));
				memcpy(((char*)WriteOverlapped)+sizeof(OVERLAPPED), Buffer, BufferPos);
				WriteFileEx(OtherPipe, ((char*)WriteOverlapped)+sizeof(OVERLAPPED), BufferPos, WriteOverlapped, WritePipeCompletion);
			}

			EnterCriticalSection(&g_OutputLock);
			printf(Server ? "Server> Received %u byte message:\n" : "Client> Received %u byte message:\n", BufferPos);
			dumphex(Buffer, (int)BufferPos, -1);
			printf("\n");
			LeaveCriticalSection(&g_OutputLock);
		}

		DisconnectNamedPipe(Pipe);
		printf(Server ? "Server> Disconnected\n" : "Client> Disconnected\n");

 */
