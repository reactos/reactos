//
// piperead.cpp
//
// Martin Fuchs, 30.11.2003
//

//
// Invoke as:	"piperead [pipe_name]",
// for example:	"piperead com_1"
//


#define	WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>


 // This definition currently missing in MinGW.
#ifndef	FILE_FLAG_FIRST_PIPE_INSTANCE
#define	FILE_FLAG_FIRST_PIPE_INSTANCE 0x00080000
#endif


static void print_error(DWORD win32_error)
{
	fprintf(stderr, "WIN32 error %lu\n", win32_error);
}


int main(int argc, char** argv)
{
	char path[MAX_PATH];
	const char* pipe_name;

	if (argc > 1)
		pipe_name = *++argv;
	else
		pipe_name = "com_1";

	sprintf(path, "\\\\.\\pipe\\%s", pipe_name);

	HANDLE hPipe = CreateNamedPipe(path, PIPE_ACCESS_DUPLEX|FILE_FLAG_FIRST_PIPE_INSTANCE, PIPE_WAIT|PIPE_TYPE_BYTE, 1, 4096, 4096, 30000, NULL);

	if (hPipe == INVALID_HANDLE_VALUE) {
		print_error(GetLastError());
		return 1;
	}

	for(;;) {
		DWORD read;
		BYTE buffer[1024];

		if (!ReadFile(hPipe, buffer, sizeof(buffer), &read, NULL)) {
			DWORD error = GetLastError();

			if (error == ERROR_PIPE_LISTENING)
				Sleep(1000);
			else if (error == ERROR_BROKEN_PIPE) {
				CloseHandle(hPipe);

				hPipe = CreateNamedPipe(path, PIPE_ACCESS_DUPLEX|FILE_FLAG_FIRST_PIPE_INSTANCE, PIPE_WAIT|PIPE_TYPE_BYTE, 1, 4096, 4096, 30000, NULL);

				if (hPipe == INVALID_HANDLE_VALUE) {
					print_error(GetLastError());
					return 1;
				}
			} else {
				print_error(error);
				break;
			}
		}

		if (read)
			fwrite(buffer, read, 1, stdout);
	}

	if (!CloseHandle(hPipe))
		print_error(GetLastError());

	return 0;
}
