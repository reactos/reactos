#include <stdio.h>
#include <windows.h>
#include <ddk/ntddk.h>
#include "mpu401.h"

int main()
{
//    NTSTATUS s;
//    PHANDLE Handle;
//    PIO_STATUS_BLOCK Status;
    DWORD BytesReturned;
    BYTE Test[3]; // Will store MIDI data
    BYTE Notes[] = {50, 52, 54, 55, 57, 59, 61};
    HANDLE Device;
    UINT Note;
    UINT Junk;

    printf("Test program for MPU401 driver\n");

    Device = CreateFile("\\\\.\\MPU401_Out_0", GENERIC_READ | GENERIC_WRITE,
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

    printf("Device is open, let's play some music...\n");

        Test[0] = 0x90;
        Test[2] = 0x7f;

    for (Note = 0; Note < sizeof(Notes); Note ++)
    {
        Test[1] = Notes[Note];

    DeviceIoControl(
        Device,
        IOCTL_MIDI_PLAY,
        &Test,
        sizeof(Test),
        NULL,
        0,
        &BytesReturned,
        NULL
        );

        for (Junk = 0; Junk < 100000; Junk ++);   // Pause
    }


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
