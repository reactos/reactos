#include <stdio.h>
#include <windows.h>
#include <winioctl.h>

int main(int argc, char *argv[ ])
{

    BOOL b;
    DWORD FsFlags;
    LPSTR lp;
    HANDLE hMod;
    UINT w;
    CHAR FileName[MAX_PATH];
    HANDLE hFile;
    DWORD Nbytes;
    DWORD FileSize;
    WORD State;
    DWORD Length;
    DWORD wrap;


    b = GetVolumeInformation(NULL,NULL,0,NULL,NULL,&FsFlags,NULL,0);

    if ( !b ) {
        printf("compstrs: Failure getting volumeinformation %d\n",GetLastError());
        return 0;
        }

    if ( !(FsFlags & FS_FILE_COMPRESSION) ) {
        printf("compstrs: File system does not support per-file compression %x\n",FsFlags);
        return 0;
        }

    //
    // Get a temp file
    //

    w = GetTempFileName(".","cstr",0,FileName);
    if ( !w ) {
        printf("compstrs: unable to get temp file name\n");
        return 0;
        }

    //
    // Create the tempfile
    //

    hFile = CreateFile(
                FileName,
                GENERIC_WRITE | GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

    if ( hFile == INVALID_HANDLE_VALUE ) {
        printf("compstrs: failure creating %s %d\n",FileName,GetLastError());
        return 0;
        }

    //
    // Write the file that we want to compress. It is a copy of kernel32 and ntdll
    //

    hMod = GetModuleHandle("kernel32");
    if ( !hMod ) {
        printf("compstrs: failure getting handle to kernel32.dll\n");
        CloseHandle(hFile);
        DeleteFile(FileName);
        return 0;
        }

    lp = (LPSTR)hMod;

    b = TRUE;
    FileSize = 0;
    while(b) {
        b = WriteFile(hFile,lp,512, &Nbytes, NULL);
        if ( b ) {
            FileSize += Nbytes;
            lp += Nbytes;
            }
        }

    hMod = GetModuleHandle("ntdll");
    if ( !hMod ) {
        printf("compstrs: failure getting handle to ntdll\n");
        CloseHandle(hFile);
        DeleteFile(FileName);
        return 0;
        }

    lp = (LPSTR)hMod;

    b = TRUE;
    while(b) {
        b = WriteFile(hFile,lp,512, &Nbytes, NULL);
        if ( b ) {
            FileSize += Nbytes;
            lp += Nbytes;
            }
        }

    wrap = 0;
    while(1) {

        //
        // compress and de-compress this file forever
        //

        State = 1;

        b = DeviceIoControl(
                hFile,
                FSCTL_SET_COMPRESSION,
                &State,
                sizeof(WORD),
                NULL,
                0,
                &Length,
                NULL
                );
        if ( !b ) {
            printf("compstrs: compress failed %d\n",GetLastError());
            wrap = 0;
            }
        else {
            FlushFileBuffers(hFile);
            printf("C");
            wrap++;
            }

        Sleep(500);

        //
        // Decompress
        //

        State = 0;

        b = DeviceIoControl(
                hFile,
                FSCTL_SET_COMPRESSION,
                &State,
                sizeof(WORD),
                NULL,
                0,
                &Length,
                NULL
                );
        if ( !b ) {
            printf("compstrs: uncompress failed %d\n",GetLastError());
            wrap = 0;
            }
        else {
            FlushFileBuffers(hFile);
            printf("U");
            wrap++;
            }

        if ( wrap > 50 ) {
            printf("\n");
            wrap = 0;
            }
    }

    CloseHandle(hFile);
    DeleteFile(FileName);

}
