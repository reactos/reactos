//  utxcpt1.c - user mode structured exception handling test 1

#include <ntos.h>

main()
{
    LONG i, j;
    PULONG p4, p3, p2, p1;
    ULONG Size1, Size2, Size3;
    NTSTATUS status;
    HANDLE CurrentProcessHandle;
    MEMORY_BASIC_INFORMATION MemInfo;
    ULONG OldProtect;
    STRING Name3;
    HANDLE Section1;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG ViewSize, Offset;

    CurrentProcessHandle = NtCurrentProcess();

    for(i=0;i<3;i++){
        DbgPrint("Hello World...\n\n");
    }

    DbgPrint("allocating virtual memory\n");

    p1 = (PULONG)NULL;
    Size1 = 5*4096;

    status = NtAllocateVirtualMemory (CurrentProcessHandle, (PVOID)&p1,
                        0, &Size1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    DbgPrint("created vm status %X start %lx size %lx\n",
            status, (ULONG)p1, Size1);

    p2 = p1;

    *p2 = 99;
    Size2 = 4;

    status = NtProtectVirtualMemory (CurrentProcessHandle, (PVOID)&p2,
                        &Size2, PAGE_GUARD | PAGE_READONLY, &OldProtect);

    DbgPrint("protected VM status %X, base %lx, size %lx, old protect %lx\n",
                    status, p2, Size2, OldProtect);

    p3 = p1 + 1024;

    *p3 =91;
    Size2 = 4;

    status = NtProtectVirtualMemory (CurrentProcessHandle, (PVOID)&p3,
                        &Size2, PAGE_NOACCESS, &OldProtect);

    DbgPrint("protected VM status %X, base %lx, size %lx, old protect %lx\n",
                    status, p3, Size2, OldProtect);
    try {
        *p2 = 94;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        DbgPrint("got an exception of %X\n",status);
    }

    try {
        i = *p2;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        DbgPrint("got an exception of %X\n",status);
    }

    DbgPrint("value of p2 should be 94 is %ld\n",*p2);

    try {
        *p3 = 94;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        DbgPrint("got an exception of %X\n",status);
    }

    return 0;
}
