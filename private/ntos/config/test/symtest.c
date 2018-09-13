#include "ntos.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
main(
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Attr1;
    OBJECT_ATTRIBUTES Attr2;
    UNICODE_STRING  Name1;
    UNICODE_STRING  Name2;
    HANDLE Handle1;
    HANDLE Handle2;
    IO_STATUS_BLOCK IoStatusBlock;


    RtlInitUnicodeString(&Name1, L"\\DosDevices");
    RtlInitUnicodeString(&Name2, L"C:\\Nt");
    InitializeObjectAttributes(&Attr1,
                               &Name1,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenDirectoryObject(&Handle1,
                                   DIRECTORY_QUERY,
                                   &Attr1);
    if (!NT_SUCCESS(Status)) {
        printf("NtOpenDirectoryObject failed %08lx\n",Status);
        exit(1);
    }

    InitializeObjectAttributes(&Attr2,
                               &Name2,
                               OBJ_CASE_INSENSITIVE,
                               Handle1,
                               NULL);
    Status = NtOpenFile(&Handle2,
                        FILE_LIST_DIRECTORY,
                        &Attr2,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        0);
    if (!NT_SUCCESS(Status)) {
        printf("NtOpenFile failed %08lx\n",Status);
        exit(1);
    }

    printf("success\n");


}
