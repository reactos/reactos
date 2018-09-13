#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

DWORD Size;
LPSTR WriteData = "Hello World\n";
HANDLE ReadHandle, WriteHandle;

VOID
WriterThread(
    LPVOID ThreadParameter
    )
{
    DWORD n;
    Sleep(10000);
    printf("Writing...\n");
    WriteFile(WriteHandle,WriteData,Size, &n, NULL);
    assert(n==Size);
    printf("Done Writing...\n");
    Sleep(10000);
    printf("Done Sleeping...\n");
}


int
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    BOOL b;
    DWORD n;
    LPSTR l;
    HANDLE Thread;
    DWORD ThreadId;
DebugBreak();
    b = CreatePipe(&ReadHandle, &WriteHandle,NULL,0);
    assert(b);

    Size = strlen(WriteData)+1;
    l = LocalAlloc(LMEM_ZEROINIT,Size);
    assert(l);

    Thread = CreateThread(NULL,0L,WriterThread,(LPVOID)99,0,&ThreadId);
    assert(Thread);
    printf("Reading\n");
    ReadFile(ReadHandle,l,Size, &n, NULL);
    assert(n==Size);
    printf("Reading Done %s\n",l);
}
