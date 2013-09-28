/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         See COPYING in the top level directory
 * PURPOSE:         Test for NtCreateFile
 * PROGRAMMER:      Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#define WIN32_NO_STATUS
#include <wine/test.h>
//#include <ndk/iotypes.h>
#include <ndk/iofuncs.h>
//#include <ndk/obtypes.h>
//#include <ndk/obfuncs.h>

START_TEST(NtCreateFile)
{
    NTSTATUS Status;
    HANDLE FileHandle;
    IO_STATUS_BLOCK StatusBlock;

    Status = NtCreateFile(&FileHandle,
                          FILE_READ_DATA,
                          (POBJECT_ATTRIBUTES)0xCCCCCCCC,
                          &StatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ,
                          FILE_OPEN_IF,
                          FILE_NON_DIRECTORY_FILE,
                          NULL,
                          0);

    ok_hex(Status, STATUS_ACCESS_VIOLATION);
}
