/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/include/baseheap.h
 * PURPOSE:         Base Heap Structures
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

//
// Some important implementation notes.
//
// Firstly, the Global* APIs in Win32 are largely similar to the Local* APIs,
// but there are a number of small differences (for example, there is no such
// thing as DDE/Shared Local memory, and the re-allocation semantics are also
// simpler, because you cannot force a move).
//
// Note something VERY IMPORTANT: This implementation *depends* on the fact
// that all heap pointers are, at the very least, 8 byte aligned, and that heap
// handles are actually pointers inside our HandleEntry->Object, which happens
// to be at offset 0x4, which means testing with bit 3 tells us if the handle
// is a pointer or a real "handle". On 64-bit, heap pointers should be 16-byte
// aligned, and our offset should be 0x8, so the trick works anyways, but using
// the 4th bit.
//
// Apart from MSDN, a wonderful source of information about how this works is
// available on Raymond's blog, in a 4-parter series starting at:
// http://blogs.msdn.com/oldnewthing/archive/2004/11/04/252258.aspx.
//
// Finally, as Raymond points out, be aware that some applications depend on
// the way this implementation was done, since global memory handles are a
// straight-forward overlay on top of the RTL Handle implementation, and rogue
// applications can easily do the conversion manually without calling the right
// API for it (such as GlobalLock).
//

//
// The Handle Table
//
extern RTL_HANDLE_TABLE BaseGlobalHandleTable;

//
// Tracing Support
// Define _BASE_HANDLE_TRACE for Traces
//
#ifdef _BASE_HANDLE_TRACE_
#define BH_PRINT DbgPrint
#else
#define BH_PRINT DPRINT
#endif
#define BASE_TRACE_ALLOC(x, y)                                              \
    BH_PRINT("[BASE_HEAP] %s : Allocating %lx bytes with flags: %lx\n",     \
             __FUNCTION__, x, y)
#define BASE_TRACE_ALLOC2(x)                                                \
    BH_PRINT("[BASE_HEAP] %s : Allocated %p\n",                             \
             __FUNCTION__, x)
#define BASE_TRACE_PTR(x, y)                                                \
    BH_PRINT("[BASE_HEAP] %s : Using handle: %lx for pointer: %p\n",        \
             __FUNCTION__, x, y)
#define BASE_TRACE_HANDLE(x, y)                                             \
    BH_PRINT("[BASE_HEAP] %s : Using handle: %lx for block: %p\n",          \
             __FUNCTION__, x, y)
#define BASE_TRACE_DEALLOC(x)                                               \
    BH_PRINT("[BASE_HEAP] %s : Freeing %p\n",                               \
             __FUNCTION__, x)

//
// The handle structure for global heap handles.
// Notice that it nicely overlays with RTL_HANDLE_ENTRY.
// KEEP IT THAT WAY! ;-)
//
typedef struct _GLOBAL_HEAP_HANDLE_ENTRY
{
    USHORT Flags;
    USHORT LockCount;
    union
    {
        PVOID Object;
        ULONG OldSize;
    };
} GLOBAL_HEAP_HANDLE_ENTRY, *PGLOBAL_HEAP_HANDLE_ENTRY;

//
// Handle entry flags
// Note that 0x0001 is the shared/generic RTL_HANDLE_VALID
//
#define GLOBAL_HEAP_ENTRY_FLAG_MOVABLE      0x0002
#define GLOBAL_HEAP_ENTRY_FLAG_REUSABLE     0x0004
#define GLOBAL_HEAP_ENTRY_FLAG_REUSE        0x0008
#define GLOBAL_HEAP_ENTRY_FLAG_DDESHARE     0x0010

//
// Easy way to check if the global handle is actually an entry in our table
//
#define GLOBAL_HEAP_IS_HANDLE_ENTRY         \
    (ULONG_PTR)FIELD_OFFSET(GLOBAL_HEAP_HANDLE_ENTRY, Object)

//
// Tags for the entire heap allocation for this global memory.
// They are set part of the User Flags of the RTL Heap.
//
#define GLOBAL_HEAP_FLAG_MOVABLE            HEAP_SETTABLE_USER_FLAG1
#define GLOBAL_HEAP_FLAG_DDESHARE           HEAP_SETTABLE_USER_FLAG2

//
// Internal Handle Functions
//
#define GlobalAllocEntry()                  \
    (PGLOBAL_HEAP_HANDLE_ENTRY)RtlAllocateHandle(&BaseGlobalHandleTable, NULL)

#define GlobalGetEntry(h)                   \
    (PGLOBAL_HEAP_HANDLE_ENTRY)             \
        CONTAINING_RECORD(h,                \
            GLOBAL_HEAP_HANDLE_ENTRY,       \
            Object);

#define GlobalValidateEntry(he)             \
    RtlIsValidHandle(&BaseGlobalHandleTable, (PRTL_HANDLE_TABLE_ENTRY)he)

#define GlobalFreeEntry(he)                 \
    RtlFreeHandle(&BaseGlobalHandleTable, (PRTL_HANDLE_TABLE_ENTRY)he);

