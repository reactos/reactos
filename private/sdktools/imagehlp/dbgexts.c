#include "private.h"

#include <ntsdexts.h>


#ifdef __cplusplus
#define CPPMOD extern "C"
#else
#define CPPMOD
#endif

#define DECLARE_API(s)                          \
    CPPMOD VOID                                 \
    s(                                          \
        HANDLE hCurrentProcess,                 \
        HANDLE hCurrentThread,                  \
        DWORD dwCurrentPc,                      \
        PNTSD_EXTENSION_APIS lpExtensionApis,   \
        LPSTR lpArgumentString                  \
     )

#define INIT_API() {                            \
    ExtensionApis = *lpExtensionApis;           \
    ExtensionCurrentProcess = hCurrentProcess;  \
    }

#define dprintf                 (ExtensionApis.lpOutputRoutine)
#define GetExpression           (ExtensionApis.lpGetExpressionRoutine)
#define GetSymbol               (ExtensionApis.lpGetSymbolRoutine)
#define Disassm                 (ExtensionApis.lpDisasmRoutine)
#define CheckControlC           (ExtensionApis.lpCheckControlCRoutine)
#define ReadMemory(a,b,c,d)     ReadProcessMemory( ExtensionCurrentProcess, (LPCVOID)(a), (b), (c), (d) )
#define WriteMemory(a,b,c,d)    WriteProcessMemory( ExtensionCurrentProcess, (LPVOID)(a), (LPVOID)(b), (c), (d) )

NTSD_EXTENSION_APIS ExtensionApis;
HANDLE ExtensionCurrentProcess;

#ifdef IMAGEHLP_HEAP_DEBUG
DECLARE_API( allocs )
{
    PLIST_ENTRY                 Next;
    HEAP_BLOCK                  HeapBlock;
    ULONG                       Address;
    ULONG                       r;
    ULONG                       cb;

    LIST_ENTRY                  LocalHeapHeader;
    ULONG                       LocalTotalAllocs;
    ULONG                       LocalTotalMemory;
    HANDLE                      LocalhHeap;


    INIT_API();

    Address = GetExpression("imagehlp!TotalAllocs");
    r = ReadMemory(Address,
                   &LocalTotalAllocs,
                   sizeof(LocalTotalAllocs),
                   &cb
                   );
    if (!r || cb != sizeof(LocalTotalAllocs)) {
        dprintf("*** TotalAllocs unreadable\n");
        return;
    }

    Address = GetExpression("imagehlp!TotalMemory");
    r = ReadMemory(Address,
                   &LocalTotalMemory,
                   sizeof(LocalTotalMemory),
                   &cb
                   );
    if (!r || cb != sizeof(LocalTotalMemory)) {
        dprintf("*** TotalMemory unreadable\n");
        return;
    }

    Address = GetExpression("imagehlp!hHeap");
    r = ReadMemory(Address,
                   &LocalhHeap,
                   sizeof(LocalhHeap),
                   &cb
                   );
    if (!r || cb != sizeof(LocalhHeap)) {
        dprintf("*** hHeap unreadable\n");
        return;
    }


    Address = GetExpression("imagehlp!HeapHeader");
    r = ReadMemory(Address,
                   &LocalHeapHeader,
                   sizeof(LocalHeapHeader),
                   &cb
                   );
    if (!r || cb != sizeof(LocalHeapHeader)) {
        dprintf("*** HeapHeader unreadable\n");
        return;
    }
    Next = LocalHeapHeader.Flink;
    if (!Next) {
        return;
    }

    dprintf( "-----------------------------------------------------------------------------\n" );
    dprintf( "Memory Allocations for Heap 0x%08x, Allocs=%d, TotalMem=%d\n",
                     LocalhHeap, LocalTotalAllocs, LocalTotalMemory );
    dprintf( "-----------------------------------------------------------------------------\n" );
    dprintf( "*\n" );

    while ((ULONG)Next != Address) {
        r = ReadMemory( CONTAINING_RECORD( Next, HEAP_BLOCK, ListEntry ),
                        &HeapBlock,
                        sizeof(HeapBlock),
                        &cb
                        );
        if (!r || cb != sizeof(HeapBlock)) {
            dprintf("*** list broken\n");
            return;
        }
        Next = HeapBlock.ListEntry.Flink;
        dprintf( "%8d %16s @ %5d\n", HeapBlock.Size, HeapBlock.File, HeapBlock.Line );
    }

    return;

}
#endif
