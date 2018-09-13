
/*++

    Copyright (c) 1997 Microsoft Corporation

    Module Name:

        regleaks.hxx

    Abstract:

        Common stuff for CS debugger extensions

    Author:

        UShaji (Adapted from rpc extensions, MarioGo, MazharM, JRoberts)

--*/

#ifndef _REGLEAKS_HXX_
#define _REGLEAKS_HXX_


#define MY_DECLARE_API(_x_)                           \
    DECLARE_API( _x_ )                                \
    {                                                 \
    DWORD dwAddr;                                     \
    INIT_DPRINTF();                                   \
    dwAddr = GetExpression(lpArgumentString);         \
    if ( !dwAddr ) {                                  \
        dprintf("Error: Failure to get address\n");   \
        return;                                       \
    }                                                 \
    do_##_x_(dwAddr);                                 \
    return;}                                          \



extern 
WCHAR *
ReadProcessChar(
    unsigned short * Address
    );

extern
PCHAR
MapSymbol(DWORD);


#define READ(addr, buffer, length)                                           \
   b = GetData((addr),(buffer),(length), NULL);                              \
   if (!b) {                                                                 \
           dprintf("Failed to read address %d, %d bytes", (addr), (length)); \
           return;                                                           \
           }                                                                 \

extern HANDLE ProcessHandle;
extern BOOL fKernelDebug;

#undef DECLARE_API

#define DECLARE_API(s)                              \
        VOID                                        \
        s(                                          \
            HANDLE               hCurrentProcess,   \
            HANDLE               hCurrentThread,    \
            DWORD                dwCurrentPc,       \
            PWINDBG_EXTENSION_APIS pExtensionApis,  \
            LPSTR                lpArgumentString   \
            )


//
// types
//

typedef BOOL (*UEnvReadMemory)( HANDLE, const void*, void*, DWORD, DWORD* );
typedef BOOL (*UEnvWriteMemory)( HANDLE, void*, void*, DWORD, DWORD* );

//
// forwards
//

BOOL
ReadMemoryUserMode( HANDLE hProcess, const void* pAddress, void* pBuffer, DWORD dwSize, DWORD* pdwRead );

BOOL
ReadMemoryKernelMode( HANDLE hProcess, const void* pAddress, void* pBuffer, DWORD dwSize, DWORD* pdwRead );

BOOL
WriteMemoryUserMode( HANDLE hProcess, const void* pAddress, void* pBuffer, DWORD dwSize, DWORD* pdwRead );

BOOL
WriteMemoryKernelMode( HANDLE hProcess, const void* pAddress, void* pBuffer, DWORD dwSize, DWORD* pdwRead );

extern UEnvReadMemory          ReadMemoryExt;
extern UEnvReadMemory          WriteMemoryExt;

#define INIT_DPRINTF()    { if (!fKernelDebug) \
                            {                                         \
                                ExtensionApis = *pExtensionApis;      \
                                ReadMemoryExt = ReadMemoryUserMode;   \
                                WriteMemoryExt = WriteMemoryUserMode; \
                            }                                         \
                            ProcessHandle = hCurrentProcess;          \
                          }                                           \


extern WINDBG_EXTENSION_APIS ExtensionApis;

#define MIN(x, y) ((x) < (y)) ? x:y

extern 
BOOL
GetData(IN DWORD dwAddress,  IN LPVOID ptr, IN ULONG size, IN PCSTR type );

#define MAX_SYMBOL_LENGTH (sizeof( IMAGEHLP_SYMBOL ) + 256)

typedef struct _TrackObjectData 
{
    LIST_ENTRY Links;
    HKEY       hKey;
    DWORD      dwStackDepth;
    PVOID*     rgStack;
} TrackObjectData;

enum
{
    LEAK_TRACK_FLAG_NONE = 0,
    LEAK_TRACK_FLAG_USER = 1,
    LEAK_TRACK_FLAG_ALL = 0xFFFFFFFF
};

typedef struct _RegLeakTable
{
    TrackObjectData*       pHead;
    DWORD                  cKeys;
    DWORD                  dwFlags;
    BOOL                   bCriticalSectionInitialized;
    RTL_CRITICAL_SECTION   CriticalSection;

} RegLeakTable;

void TrackObjectDataPrint(TrackObjectData* pKeyData);
void RegLeakTableDump(RegLeakTable* pLeakTable);
NTSTATUS PrintObjectInfo(HANDLE Handle);
/*
void GetSymbolForFrame(
    PVOID  pAddress,
    PUCHAR pchBuffer,
    DWORD_PTR *pDisplacement
    );*/

#endif


