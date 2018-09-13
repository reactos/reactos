/*++

Module Name:

    regleaks.cxx

Abstract:

        Debugger extensions for class store.

Author:

        UShaji (Adapted from  extensions, MarioGo, MazharM, JRoberts)

--*/
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"

#include "windows.h"

// #include "stkwalk.h"
#include <imagehlp.h>
#include "wdbgexts.h"
#include "regexts.hxx"


//
// globals
//
WINDBG_EXTENSION_APIS   ExtensionApis;
USHORT                  SavedMajorVersion = 0;
USHORT                  SavedMinorVersion = 0;
HANDLE                  ProcessHandle = 0;
BOOL                    fKernelDebug = FALSE;
UEnvReadMemory          ReadMemoryExt = ReadMemoryUserMode;
UEnvReadMemory          WriteMemoryExt = ReadMemoryUserMode;

//
// macros
//

/*
#define ExtensionRoutinePrologue()  if (!fKernelDebug) \
                                    { \
                                        ExtensionApis = *lpExtensionApis; \
                                        ReadMemoryExt = ReadMemoryUserMode; \
                                        WriteMemoryExt = WriteMemoryUserMode; \
                                    } \
                                    ULONG_PTR dwAddr = GetExpression(lpArgumentString); \

*/

#define ALLOC_SIZE 500
#define MAX_ARGS 4


// define our own operators new and delete, so that we do not have to include the crt

void * __cdecl
::operator new(unsigned int dwBytes)
{
    void *p;
    p = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwBytes);
    return (p);
}


void __cdecl
::operator delete (void *p)
{
    HeapFree(GetProcessHeap(), 0, p);
}


BOOL
ReadMemoryUserMode( HANDLE hProcess, const void* pAddress, void* pBuffer, DWORD dwSize, DWORD* pdwRead )
{
    return ReadProcessMemory( hProcess, pAddress, pBuffer, dwSize, pdwRead );
}

BOOL
ReadMemoryKernelMode( HANDLE, const void* pAddress, void* pBuffer, DWORD dwSize, DWORD* pdwRead )
{
    return ReadMemory( (ULONG) pAddress, pBuffer, dwSize, pdwRead );
}

BOOL
WriteMemoryUserMode( HANDLE hProcess, const void* pAddress, void* pBuffer, DWORD dwSize, DWORD* pdwRead )
{
    return WriteProcessMemory( hProcess, (void*) pAddress, pBuffer, dwSize, pdwRead );
}

BOOL
WriteMemoryKernelMode( HANDLE, const void* pAddress, void* pBuffer, DWORD dwSize, DWORD* pdwRead )
{
    return WriteMemory( (ULONG) pAddress, pBuffer, dwSize, pdwRead );
}

BOOL
GetData(IN DWORD dwAddress,  IN LPVOID ptr, IN ULONG size, IN PCSTR type )
{
    BOOL b;
    ULONG BytesRead;
    ULONG count;

    if (!fKernelDebug)
        {
        return ReadMemoryExt(ProcessHandle, (LPVOID) dwAddress, ptr, size, 0);
        }
        else {
        }

    while( size > 0 )
        {
        count = MIN( size, 3000 );

        b = ReadMemoryExt(ProcessHandle, (LPVOID)  dwAddress, ptr, count, &BytesRead );

        if (!b || BytesRead != count )
            {
            if (NULL == type)
                {
                type = "unspecified" ;
                }
            dprintf("Couldn't read memory with error %d\n", GetLastError());
             
            return FALSE;
            }

        dwAddress += count;
        size -= count;
        ptr = (LPVOID)((ULONG)ptr + count);
        }

    return TRUE;
}

#define MAX_MESSAGE_BLOCK_SIZE 1024
#define BLOCK_SIZE 2
// could have been bigger but hit the boundary case once.

WCHAR *ReadProcessChar(
    unsigned short * Address
    )
{
    DWORD dwAddr = (DWORD) Address;

    char       block[BLOCK_SIZE];
    WCHAR     *Block  = (WCHAR *)&block;
    char      *string_block = new char[MAX_MESSAGE_BLOCK_SIZE];
    WCHAR     *String = (WCHAR *)string_block;
    int        length = 0;
    int        i      = 0;
    BOOL       b;
    BOOL       end    = FALSE;

    if (dwAddr == NULL) {
        return (L'\0');
    }

    for (length = 0; length < MAX_MESSAGE_BLOCK_SIZE/2; ) {
        b = GetData( dwAddr, &block, BLOCK_SIZE, NULL);
        if (b == FALSE) {
            dprintf("couldn't read address %x\n", dwAddr);
            return (L'\0');
        }
        for (i = 0; i < BLOCK_SIZE/2; i++) {
            if (Block[i] == L'\0') {
                end = TRUE;
            }
            String[length] = Block[i];
            length++;
        }
        if (end == TRUE) {
            break;
        }
        dwAddr += BLOCK_SIZE;
    }

    return (String);
}

PCHAR
MapSymbol(DWORD dwAddr)
{
    static CHAR Name[256];
    DWORD Displacement;

    GetSymbol((LPVOID)dwAddr, (UCHAR *)Name, &Displacement);
    strcat(Name, "+");
    PCHAR p = strchr(Name, '\0');
    _ltoa(Displacement, p, 16);
    return(Name);
}


DECLARE_API( help )
{
    INIT_DPRINTF();

    if (lpArgumentString[0] == '\0') {
        dprintf("\n"
                "regexts help:\n\n"
                "\n"
                "!keys    - Dumps stack for all open reg handles \n"
                "!version  - Dumps the version numbers \n"
                );
    }
}



BOOL   ChkTarget;            // is debuggee a CHK build?
#define VER_PRODUCTBUILD 10
EXT_API_VERSION ApiVersion = { 3, 5, EXT_API_VERSION_NUMBER, 0 };

VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    fKernelDebug = TRUE;
    ReadMemoryExt = ReadMemoryKernelMode;
    WriteMemoryExt = WriteMemoryKernelMode;

    ExtensionApis = *lpExtensionApis ;
    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;
    ChkTarget = SavedMajorVersion == 0x0c ? TRUE : FALSE;
}

DECLARE_API( version )
{
#if    DBG
    PCSTR kind = "Checked";
#else
    PCSTR kind = "Free";
#endif

    dprintf(
        "%s SMB Extension dll for Build %d debugging %s kernel for Build %d\n",
        kind,
        VER_PRODUCTBUILD,
        SavedMajorVersion == 0x0c ? "Checked" : "Free",
        SavedMinorVersion
    );
}

VOID
CheckVersion(
    VOID
    )
{
}

LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}

DECLARE_API( keys )
{
    RegLeakTable* pLeakTable;

    INIT_DPRINTF();


    if (*lpArgumentString) {
        dprintf("Dump keys for table at %s\n", lpArgumentString);
        sscanf(lpArgumentString, "%lx", &pLeakTable);
    } else {
        dprintf("Dump keys for advapi32!gLeakTable\n");
        pLeakTable = (RegLeakTable*) GetExpression( "advapi32!gLeakTable" );

        if (!pLeakTable) {
            dprintf("Unable to resolve advapi32!gLeakTable\n"
                    "Please fix symbols or specify the address of a leak table"
                    "to !keys\n");
            return;
        } 
        
        dprintf("Dump keys for table at 0x%x\n", pLeakTable);
    }

    RegLeakTableDump(pLeakTable);
}

void RegLeakTableDump(RegLeakTable* pLeakTable)
{
    TrackObjectData* pData;
    DWORD            ListHead;
    DWORD            cKeys;
    DWORD            KeysAddress;
    DWORD            dwFlags;
    DWORD            FlagsAddress;

    KeysAddress = ((DWORD) pLeakTable) + 4;
    FlagsAddress = ((DWORD) pLeakTable) + 8;

    if (!GetData(KeysAddress,
                 &cKeys,
                 sizeof(pLeakTable->cKeys),
                 NULL)) {
        dprintf("Error reading key count at 0x%x\n", KeysAddress);
        return;
    }

    dprintf("\tKeys = 0x%x\n", cKeys);

    if (!GetData(FlagsAddress,
                 &dwFlags,
                 sizeof(pLeakTable->pHead),
                 NULL)) {
        dprintf("Error reading list head at 0x%x\n", pLeakTable);
        return;
    }

    dprintf("\tFlags = 0x%x", dwFlags);

    switch (dwFlags) 
    {
    case LEAK_TRACK_FLAG_NONE:
        dprintf("\tNo tracking\n");
        return;

    case LEAK_TRACK_FLAG_USER:
        dprintf("\tOnly subkeys of HKEY_USERS\n");
        break;

    case LEAK_TRACK_FLAG_ALL:
        dprintf("\tAll keys\n");
        break;

    default:
        dprintf("\tInvalid flag -- table corrupt\n");
        return;
    }

    if (!GetData((DWORD)pLeakTable,
                 &ListHead,
                 sizeof(pLeakTable->pHead),
                 NULL)) {
        dprintf("Error reading list head at 0x%x\n", pLeakTable);
        return;
    }

    dprintf("\tList starts at 0x%x\n", ListHead);

    TrackObjectData* NextData;

    int ikey = 0;

    for (pData = (TrackObjectData*) ListHead;
         pData != NULL;
         pData = NextData)
    {
        dprintf("\tObject at 0x%x", pData);

        TrackObjectDataPrint(pData);

        if (!GetData((DWORD) pData, &NextData, sizeof(NextData), NULL)) {
            dprintf("Error reading next link for object at 0x%x\n", pData);
            return;
        }
    }
}


void TrackObjectDataPrint(TrackObjectData* pKeyData)
{
    NTSTATUS           Status;
    DWORD              dwStackDepth;
    DWORD              StackAddress;
    HKEY               hKey;
    DWORD              hKeyAddress;
    DWORD              StackDepthAddress;
    PVOID*             rgStack;
    DWORD              pStack;

    hKeyAddress = ((DWORD) pKeyData) + 8;
    StackDepthAddress = ((DWORD) pKeyData) + 12;

    rgStack = NULL;

    if (!GetData(hKeyAddress, &hKey, sizeof(hKey), NULL)) {
        dprintf("Error reading hkey for object at 0x%x\n", pKeyData);
        return;
    }

    dprintf("Tracked key data for object 0x%x\n", hKey);

    if (!fKernelDebug) 
        (void) PrintObjectInfo(hKey);
    else
        dprintf("!!!!!!Broken into kd. do '!handle 0x%x f' for details of the handle\n", hKey);

    if (!GetData(StackDepthAddress, &dwStackDepth, sizeof(dwStackDepth), NULL)) {
        dprintf("Error reading key object at 0x%x\n", pKeyData);
        return;
    }

    if (!dwStackDepth) {
        dprintf("\t\tNo stack data\n");
        return;
    }

    dprintf("\t\tStack depth 0x%x\n", dwStackDepth);

    StackAddress = ((DWORD) (pKeyData)) + 16;

    if (!GetData(StackAddress,
                 &pStack,
                 sizeof(PVOID),
                 NULL)) {
        dprintf("Error reading stack frames at 0x%x\n", StackAddress);
        return;
    } 
    dprintf("\t\tStack frames at 0x%x\n", pStack);

    rgStack = (PVOID*) RtlAllocateHeap(
        RtlProcessHeap(),
        0,
        sizeof(*rgStack) * dwStackDepth);

    if (!rgStack) {
        return;
    }

    if (!GetData(pStack,
                 rgStack,
                 sizeof(*rgStack) * dwStackDepth,
                 NULL)) {
        dprintf("Error reading stack frames at 0x%x\n", StackAddress);
        RtlFreeHeap(RtlProcessHeap(), 0, rgStack);
        return;
    } 

    for (int iFrame = 0; iFrame < dwStackDepth; iFrame++)
    {
        UCHAR Symbol[MAX_SYMBOL_LENGTH];
        DWORD_PTR Displacement;

        *Symbol = L'\0';

        GetSymbol(
            rgStack[iFrame],
            Symbol,
            &Displacement);

        dprintf("\t\t0x%x", rgStack[iFrame]);

        if (*Symbol) {
            dprintf("\t %s", Symbol);
            
            if (Displacement) {
                dprintf("+0x%x", Displacement);
            }
        } else {
            dprintf("\t ????????");
        }

        dprintf("\n");
    }

    if (rgStack) {
        RtlFreeHeap(RtlProcessHeap(), 0, rgStack);
    }

    dprintf("\n");
}


NTSTATUS PrintObjectInfo(HANDLE Handle)
{

    POBJECT_NAME_INFORMATION pNameInfo;
    BYTE     rgNameInfoBuf[512];
    NTSTATUS Status;
    HKEY     hkDup;
    DWORD    dwRequired;

    Status = NtDuplicateObject(
        ProcessHandle,
        Handle,
        NtCurrentProcess(),
        (PHANDLE) &hkDup,
        0,
        FALSE,
        DUPLICATE_SAME_ACCESS);

    if (!NT_SUCCESS(Status)) {
        dprintf("Unable to duplicate handle 0x%x from process handle 0x%x, error 0x%x\n", 
                Handle,
                ProcessHandle,
                Status);
        return Status;
    }
        
    pNameInfo = (POBJECT_NAME_INFORMATION) rgNameInfoBuf;

    Status = NtQueryObject(
        hkDup,
        ObjectNameInformation,
        pNameInfo,
        sizeof(pNameInfo),
        &dwRequired);

    if (!NT_SUCCESS(Status)) {

        if (STATUS_INFO_LENGTH_MISMATCH == Status) {

            Status = STATUS_NO_MEMORY;

            pNameInfo = (POBJECT_NAME_INFORMATION) RtlAllocateHeap(
                RtlProcessHeap(),
                0,
                dwRequired);
            
            if (pNameInfo) {

                Status = NtQueryObject(
                    hkDup,
                    ObjectNameInformation,
                    pNameInfo,
                    dwRequired,
                    &dwRequired);
            }

        }
    }


    if (!NT_SUCCESS(Status)) {
        dprintf("Unable to query object information for object error 0x%x\n",
                Status);
    } else {
        dprintf("Object 0x%x\n\tName: %S\n", Handle, pNameInfo->Name.Buffer);
    }

    NtClose(hkDup);

    if ((PBYTE) pNameInfo != rgNameInfoBuf) {
        RtlFreeHeap(RtlProcessHeap(), 0, pNameInfo);
    }

    return STATUS_SUCCESS;
}





