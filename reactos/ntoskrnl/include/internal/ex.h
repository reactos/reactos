#ifndef __NTOSKRNL_INCLUDE_INTERNAL_EXECUTIVE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_EXECUTIVE_H

/* GLOBAL VARIABLES *********************************************************/

extern TIME_ZONE_INFORMATION ExpTimeZoneInfo;
extern LARGE_INTEGER ExpTimeZoneBias;
extern ULONG ExpTimeZoneId;
extern POBJECT_TYPE ExEventPairObjectType;

/* INITIALIZATION FUNCTIONS *************************************************/

VOID
ExpWin32kInit(VOID);

VOID
ExInit2(VOID);

VOID
ExInit3(VOID);

VOID
ExpInitTimeZoneInfo(VOID);

VOID
ExpInitializeWorkerThreads(VOID);

VOID
ExpInitLookasideLists(VOID);

VOID
ExpInitializeCallbacks(VOID);

VOID
ExpInitUuids(VOID);

VOID
STDCALL
ExpInitializeExecutive(VOID);

VOID
ExpInitializeEventImplementation(VOID);

VOID
ExpInitializeEventImplementation(VOID);

VOID
ExpInitializeEventPairImplementation(VOID);

VOID
ExpInitializeSemaphoreImplementation(VOID);

VOID
ExpInitializeMutantImplementation(VOID);

VOID
ExpInitializeTimerImplementation(VOID);

VOID
ExpInitializeProfileImplementation(VOID);

/* HANDLE TABLE FUNCTIONS ***************************************************/

#define EX_HANDLE_ENTRY_LOCKED (1 << ((sizeof(PVOID) * 8) - 1))
#define EX_HANDLE_ENTRY_PROTECTFROMCLOSE (1 << 0)
#define EX_HANDLE_ENTRY_INHERITABLE (1 << 1)
#define EX_HANDLE_ENTRY_AUDITONCLOSE (1 << 2)

#define EX_HANDLE_TABLE_CLOSING 0x1

#define EX_INVALID_HANDLE (~0)

#define EX_HANDLE_ENTRY_FLAGSMASK (EX_HANDLE_ENTRY_LOCKED |                    \
                                   EX_HANDLE_ENTRY_PROTECTFROMCLOSE |          \
                                   EX_HANDLE_ENTRY_INHERITABLE |               \
                                   EX_HANDLE_ENTRY_AUDITONCLOSE)

typedef VOID (STDCALL PEX_DESTROY_HANDLE_CALLBACK)(
    PHANDLE_TABLE HandleTable, 
    PVOID Object, 
    ULONG GrantedAccess, 
    PVOID Context
);

typedef BOOLEAN (STDCALL PEX_DUPLICATE_HANDLE_CALLBACK)(
    PHANDLE_TABLE HandleTable, 
    PHANDLE_TABLE_ENTRY HandleTableEntry, 
    PVOID Context
);

typedef BOOLEAN (STDCALL PEX_CHANGE_HANDLE_CALLBACK)(
    PHANDLE_TABLE HandleTable, 
    PHANDLE_TABLE_ENTRY HandleTableEntry, 
    PVOID Context
);

VOID
ExpInitializeHandleTables(VOID);

PHANDLE_TABLE
ExCreateHandleTable(IN PEPROCESS QuotaProcess  OPTIONAL);

VOID
ExDestroyHandleTable(
    IN PHANDLE_TABLE HandleTable,
    IN PEX_DESTROY_HANDLE_CALLBACK DestroyHandleCallback  OPTIONAL,
    IN PVOID Context  OPTIONAL
);

PHANDLE_TABLE
ExDupHandleTable(
    IN PEPROCESS QuotaProcess  OPTIONAL,
    IN PEX_DUPLICATE_HANDLE_CALLBACK DuplicateHandleCallback  OPTIONAL,
    IN PVOID Context  OPTIONAL,
    IN PHANDLE_TABLE SourceHandleTable
);

BOOLEAN
ExLockHandleTableEntry(
    IN PHANDLE_TABLE HandleTable,
    IN PHANDLE_TABLE_ENTRY Entry
);

VOID
ExUnlockHandleTableEntry(
    IN PHANDLE_TABLE HandleTable,
    IN PHANDLE_TABLE_ENTRY Entry
);

LONG
ExCreateHandle(
    IN PHANDLE_TABLE HandleTable,
    IN PHANDLE_TABLE_ENTRY Entry
);

BOOLEAN
ExDestroyHandle(
    IN PHANDLE_TABLE HandleTable,
    IN LONG Handle
);

VOID
ExDestroyHandleByEntry(
    IN PHANDLE_TABLE HandleTable,
    IN PHANDLE_TABLE_ENTRY Entry,
    IN LONG Handle
);

PHANDLE_TABLE_ENTRY
ExMapHandleToPointer(
    IN PHANDLE_TABLE HandleTable,
    IN LONG Handle
);

BOOLEAN
ExChangeHandle(
    IN PHANDLE_TABLE HandleTable,
    IN LONG Handle,
    IN PEX_CHANGE_HANDLE_CALLBACK ChangeHandleCallback,
    IN PVOID Context
);

/* PSEH EXCEPTION HANDLING **************************************************/

LONG
STDCALL
ExSystemExceptionFilter(VOID);

static __inline _SEH_FILTER(_SEH_ExSystemExceptionFilter)
{
    return ExSystemExceptionFilter();
}

/* OTHER FUNCTIONS **********************************************************/

LONGLONG
FASTCALL
ExfpInterlockedExchange64(
    LONGLONG volatile * Destination,
    PLONGLONG Exchange
);

NTSTATUS
ExpSetTimeZoneInformation(PTIME_ZONE_INFORMATION TimeZoneInformation);

NTSTATUS
ExpAllocateLocallyUniqueId(OUT LUID *LocallyUniqueId);

VOID
STDCALL
ExTimerRundown(VOID);

#define InterlockedDecrementUL(Addend) \
   (ULONG)InterlockedDecrement((PLONG)(Addend))

#define InterlockedIncrementUL(Addend) \
   (ULONG)InterlockedIncrement((PLONG)(Addend))

#define InterlockedExchangeUL(Target, Value) \
   (ULONG)InterlockedExchange((PLONG)(Target), (LONG)(Value))

#define InterlockedExchangeAddUL(Addend, Value) \
   (ULONG)InterlockedExchangeAdd((PLONG)(Addend), (LONG)(Value))

#define InterlockedCompareExchangeUL(Destination, Exchange, Comperand) \
   (ULONG)InterlockedCompareExchange((PLONG)(Destination), (LONG)(Exchange), (LONG)(Comperand))

#define ExfInterlockedCompareExchange64UL(Destination, Exchange, Comperand) \
   (ULONGLONG)ExfInterlockedCompareExchange64((PLONGLONG)(Destination), (PLONGLONG)(Exchange), (PLONGLONG)(Comperand))

#define ExfpInterlockedExchange64UL(Target, Value) \
   (ULONGLONG)ExfpInterlockedExchange64((PLONGLONG)(Target), (PLONGLONG)(Value))

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_EXECUTIVE_H */
