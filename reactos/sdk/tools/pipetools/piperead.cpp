//
// piperead.cpp
//
// Martin Fuchs, 30.11.2003
//
// Jan Roeloffzen, 26.1.2010
// Pipe client, based on msdn example


#define WIN32_LEAN_AND_MEAN
#include <errno.h>
#include <windows.h>
#include <stdio.h>

#define PIPEREAD_VERSION    "0.3"
#define PIPEREAD_NOPIPE     (-101)

 // This definition currently missing in MinGW.
#ifndef FILE_FLAG_FIRST_PIPE_INSTANCE
#define FILE_FLAG_FIRST_PIPE_INSTANCE 0x00080000
#endif


#define BUFSIZE 1024

static void print_error(DWORD win32_error)
{
    fprintf(stderr, "WIN32 error %lu\n", win32_error);
}

static int pipeServer(char *path)
{
    HANDLE hPipe = CreateNamedPipe(path, PIPE_ACCESS_DUPLEX|FILE_FLAG_FIRST_PIPE_INSTANCE, PIPE_WAIT|PIPE_TYPE_BYTE, 1, 4096, 4096, 30000, NULL);

    if (hPipe == INVALID_HANDLE_VALUE) {
        print_error(GetLastError());
        return 1;
    }

    for(;;) {
        DWORD read;
        BYTE buffer[BUFSIZE];

        if (!ReadFile(hPipe, buffer, sizeof(buffer), &read, NULL)) {
            DWORD error = GetLastError();

            if (error == ERROR_PIPE_LISTENING) {
                Sleep(1000);
            } else if (error == ERROR_BROKEN_PIPE) {
                CloseHandle(hPipe);

                hPipe = CreateNamedPipe(path, PIPE_ACCESS_DUPLEX|FILE_FLAG_FIRST_PIPE_INSTANCE, PIPE_WAIT|PIPE_TYPE_BYTE, 1, 4096, 4096, 30000, NULL);

                if (hPipe == INVALID_HANDLE_VALUE) {
                    fprintf(stderr,"INVALID_HANDLE_VALUE\n");
                    print_error(GetLastError());
                    return 1;
                }
            } else {
                fprintf(stderr,"error %lu\n",error);
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


static int pipeClient(char *path)
{
    HANDLE hPipe=INVALID_HANDLE_VALUE;
    TCHAR  chBuf[BUFSIZE];
    BOOL   fSuccess = FALSE;
    DWORD  cbRead;
    DWORD  Err;
    int res = 0;

    setvbuf(stdout, NULL, _IONBF, 0);
    while (1) {
        hPipe = CreateFile(path,           // pipe name
                           GENERIC_READ,
                           0,              // no sharing
                           NULL,           // default security attributes
                           OPEN_EXISTING,  // opens existing pipe
                           0,              // default attributes
                           NULL);          // no template file

        // Break if the pipe handle is valid.
        if (hPipe != INVALID_HANDLE_VALUE)
            break;

        // Exit if an error other than ERROR_PIPE_BUSY occurs.
        Err = GetLastError();
        if (Err != ERROR_PIPE_BUSY) {
            if (ERROR_FILE_NOT_FOUND == Err)
            {
                res = PIPEREAD_NOPIPE;
                return res;
            }
            else
            {
                fprintf(stderr,"Could not open pipe %s. Error=%lu\n", path, Err );
                res = -1;
            }
            break;
        }

        // All pipe instances are busy, so wait for 20 seconds.
        if ( ! WaitNamedPipe(path, 20000)) {
            fprintf(stderr,"Could not open pipe: 20 second wait timed out.");
            res = -2;
            break;
        }
    }

    if (!res) do {
        fSuccess = ReadFile(hPipe,    // pipe handle
                            chBuf,    // buffer to receive reply
                            BUFSIZE,  // size of buffer
                            &cbRead,  // number of bytes read
                            NULL);    // not overlapped

        if ( ! fSuccess ) {
            Err = GetLastError();
            if ( Err == ERROR_MORE_DATA ) {
                fSuccess = TRUE;
            } else {
                fprintf(stderr, "ReadFile: Error %lu \n", Err );
                res = -9;
                break;
            }
        }

        fwrite(chBuf,1,cbRead,stdout);
    } while ( fSuccess);

    if ( ! fSuccess) {
        fprintf(stderr, "ReadFile from pipe failed. Error=%lu\n", GetLastError() );
    }

    if (hPipe != INVALID_HANDLE_VALUE)
        CloseHandle(hPipe);

    return res;

}

static int fileClient(const char *path)
{
    int res = 0;
    FILE *fin;
    int c;

    setvbuf(stdout, NULL, _IONBF, 0);
    if (!(fin = fopen(path, "r"))) {
        fprintf(stderr,"Could not fopen %s (%s)\n", path, strerror(errno) );
        return -1;
    }

    while ((c = fgetc(fin)) != EOF) {
        fputc(c, stdout);
    }

    fclose(fin);
    return res;
}

void usage(void)
{
    fprintf(stderr, "piperead " PIPEREAD_VERSION "\n\n");
    fprintf(stderr, "Usage: piperead [-c] <named pipe>\n");
    fprintf(stderr, "-c means Client mode\n");
    fprintf(stderr, "Example: piperead -c \\\\.\\pipe\\kdbg | log2lines -c\n\n");
}

int main(int argc, char** argv)
{
    char path[MAX_PATH];
    const char* pipe_name;
    const char* clientMode;
    int res = 0;

    pipe_name = "com_1";
    clientMode = NULL;
    switch (argc) {
    case 3:
        clientMode = *++argv;
        if (strcmp(clientMode,"-c") != 0) {
            fprintf(stderr,"Invalid option: %s\n", clientMode);
            clientMode = NULL;
            res = -6;
        }
        //fall through
    case 2:
        pipe_name = *++argv;
        if (strcmp(pipe_name,"-h") == 0) {
            res = -7;
        }
        break;
    default:
        res = -8;
        break;
    }
    if (res) {
        usage();
        return res;
    }

    if ( pipe_name[0] == '\\' ) {
        //assume caller specified full path
        sprintf(path, "%s", pipe_name);
    } else {
        sprintf(path, "\\\\.\\pipe\\%s", pipe_name);
    }

    if ( clientMode ) {
        res = pipeClient(path);
        if (res == PIPEREAD_NOPIPE) {
            res = fileClient(pipe_name);
        }
    } else {
        res = pipeServer(path);
    }

    return res;
}
