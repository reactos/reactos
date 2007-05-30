#include <stdio.h>
#include <windows.h>
#include <ntddk.h>

int main()
{
//    NTSTATUS s;
//    PHANDLE Handle;
//    PIO_STATUS_BLOCK Status;

    HANDLE Device;
    DWORD BytesReturned;

    printf("SB Test\n");

    Device = CreateFile("\\\\.\\SndBlst", GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_NO_BUFFERING,
                        NULL);

    if (Device == INVALID_HANDLE_VALUE)
    {
        printf("Device is busy or could not be found.\n");
        return -1;
    }

//    DeviceIoControl(
//        Device,
//        IOCTL_FILE_DISK_OPEN_FILE,
//        OpenFileInformation,
//        sizeof(OPEN_FILE_INFORMATION) + OpenFileInformation->FileNameLength - 1,
//        NULL
//        0,
//        &BytesReturned,
//        NULL
//        )


/*    s = IoCreateFile(Handle, GENERIC_READ | GENERIC_WRITE,
                     OBJ_KERNEL_HANDLE,
                     Status,
                     0,
                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                     FILE_OPEN,
                     FILE_NON_DIRECTORY_FILE,
                     NULL,
                     0,
                     CreateFileTypeNone,
                     NULL,
                     0);
*/
}
