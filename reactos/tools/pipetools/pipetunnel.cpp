//
// pipetunnel.cpp
//
// Martin Fuchs, 30.11.2003
//

//
// Invoke as:	"pipetunnel [pipe_name]",
// for example:	"pipetunnel com_2"
//
// Then start up RectOS in VMWare, wait for the serial connect.
// After that you can connect GDB using the command "target remote :9999".
//


#define	WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <winsock.h>

#ifdef _MSC_VER
#pragma comment(lib, "wsock32")
#endif

#include <stdio.h>
#include <errno.h>


 // This definition currently missing in MinGW.
#ifndef	FILE_FLAG_FIRST_PIPE_INSTANCE
#define	FILE_FLAG_FIRST_PIPE_INSTANCE 0x00080000
#endif


static void print_error(DWORD win32_error)
{
	fprintf(stderr, "WIN32 error %lu\n", win32_error);
}


#ifdef _DEBUG

 // critical section wrapper
struct CritSect : public CRITICAL_SECTION
{
	CritSect()
	{
		InitializeCriticalSection(this);
	}

	~CritSect()
	{
		DeleteCriticalSection(this);
	}
};

static void dbg_trace(char mode, const char* buffer, int l)
{
	static char s_mode = '\0';
	static CritSect crit_sect;

	EnterCriticalSection(&crit_sect);

	if (l) {
		for(const char*p=buffer; l--; ++p) {
			if (mode != s_mode) {
				putchar('\n');
				putchar(mode);
				putchar(' ');

				s_mode = mode;
			}

			if (*p=='\n' || !*p /*|| *p=='#'*/) {
				/*if (*p == '#')
					putchar(*p);*/

				s_mode = '\0';
			} else
				putchar(*p);
		}
	}

	LeaveCriticalSection(&crit_sect);
}

#endif


static SOCKET s_srv_socket = (SOCKET)-1;

SOCKET open_tcp_connect()
{
	if (s_srv_socket == (SOCKET)-1) {
		SOCKADDR_IN srv_addr = {0};

		srv_addr.sin_family = AF_INET;
		srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		srv_addr.sin_port = htons(9999);

		s_srv_socket = socket(PF_INET, SOCK_STREAM, 0);
		if (s_srv_socket == (SOCKET)-1) {
			perror("socket()");
			return 0;
		}

		if (bind(s_srv_socket, (struct sockaddr*) &srv_addr, sizeof(srv_addr)) == -1) {
			perror("bind()");
			return 0;
		}

		if (listen(s_srv_socket, 4) == -1) {
			perror("listen()");
			return 0;
		}
	}

	SOCKADDR_IN rem_addr;
	int rem_len = sizeof(rem_addr);

	for(;;) {
		SOCKET sock = accept(s_srv_socket, (struct sockaddr*)&rem_addr, &rem_len);

		if (sock < 0) {
			if (errno == EINTR)
				continue;

			perror("accept()");
			return 0;
		}

		return sock;
	}
}



struct WriterThread
{
	WriterThread(SOCKET sock, HANDLE hPipe)
	 :	_sock(sock),
		_hPipe(hPipe)
	{
		DWORD tid;

		HANDLE hThread = CreateThread(NULL, 0, WriterThreadRoutine, this, 0, &tid);

		if (hThread) {
			SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST);

			CloseHandle(hThread);
		} else
			delete this;
	}

protected:
	SOCKET	_sock;
	HANDLE	_hPipe;

	static DWORD WINAPI WriterThreadRoutine(LPVOID param)
	{
		WriterThread* pThis = (WriterThread*) param;

		DWORD ret = pThis->Run();

		delete pThis;

		return ret;
	}

	DWORD Run()
	{
		char buffer[1024];

		for(;;) {
			int r = recv(_sock, buffer, sizeof(buffer), 0);

			if (r == -1) {
				perror("recv()");
				fprintf(stderr, "debugger connection broken\n");
				_sock = (SOCKET)-1;
				return 1;
			}

			if (r) {
				DWORD wrote;

				if (!WriteFile(_hPipe, buffer, r, &wrote, NULL))
					break;

#ifdef _DEBUG
				dbg_trace('<', buffer, r);
#endif
			}
		}

		return 0;
	}
};


LONG read_pipe(HANDLE hPipe, SOCKET sock)
{
	for(;;) {
		DWORD read;
		char buffer[1024];

		 // wait for input data
		WaitForSingleObject(hPipe, INFINITE);

		if (!ReadFile(hPipe, buffer, sizeof(buffer), &read, NULL)) {
			DWORD error = GetLastError();

			if (error == ERROR_PIPE_LISTENING)
				Sleep(1000);
			else 
				return error;
		}

		if (read) {
#ifdef _DEBUG
			dbg_trace('>', buffer, read);
#endif

			if (!send(sock, buffer, read, 0)) {
				perror("send()");
				return GetLastError();
			}
		}
	}
}


int main(int argc, char** argv)
{
	char path[MAX_PATH];
	const char* pipe_name;

	if (argc > 1)
		pipe_name = *++argv;
	else
		pipe_name = "com_2";

	sprintf(path, "\\\\.\\pipe\\%s", pipe_name);


	 // initialize winsock
	WSADATA wsa_data;

	if (WSAStartup(MAKEWORD(2,2), &wsa_data)) {
		fprintf(stderr, "WSAStartup() failed\n");
		return 0;
	}


	 // increment priority to be faster than the cpu eating VMWare process
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);


	HANDLE hPipe = INVALID_HANDLE_VALUE;

	for(;;) {
		DWORD read;

		if (hPipe == INVALID_HANDLE_VALUE) {
			hPipe = CreateNamedPipe(path, PIPE_ACCESS_DUPLEX|FILE_FLAG_FIRST_PIPE_INSTANCE|FILE_FLAG_OVERLAPPED, PIPE_WAIT|PIPE_TYPE_BYTE, 1, 4096, 4096, 30000, NULL);

			if (hPipe == INVALID_HANDLE_VALUE) {
				print_error(GetLastError());
				return 1;
			}
		}

		 // wait for the client side of the pipe
		while(!ReadFile(hPipe, NULL, 0, &read, NULL) &&
				GetLastError()==ERROR_PIPE_LISTENING)
			Sleep(1000);

		puts("\nnamed pipe connected, now waiting for TCP connection...");

		SOCKET sock = open_tcp_connect();
		if (sock == (SOCKET)-1)
			break;

		puts("TCP connection established.");

		 // launch writer thread
		new WriterThread(sock, hPipe);

		 // launch reader loop
		LONG error = read_pipe(hPipe, sock);


		 // close TCP connectiom
		closesocket(sock);
		sock = (SOCKET)-1;

		 // close named pipe
		CloseHandle(hPipe);
		hPipe = INVALID_HANDLE_VALUE;


		if (error == ERROR_BROKEN_PIPE)
			puts("\nconnection closed.");	// normal connection termination
		else {
			print_error(GetLastError());
			break;
		}
	}

	if (hPipe != INVALID_HANDLE_VALUE)
		if (!CloseHandle(hPipe))
			print_error(GetLastError());

	return 0;
}
