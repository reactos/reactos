/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992   Microsoft Corporation

Module Name:

    prflibva.c

Abstract:

    Virtual address space counter evaluation routines

    computes the process and image virtual address space usage for return
    via Perfmon API

Author:

    Stolen from the "internal" PVIEW SDK program and adapted for Perfmon by:

    a-robw (Bob Watson) 11/29/92

Revision History:

--*/
//
//  define routine's "personality"
//
#define UNICODE 1
//
//  Include files
//

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winperf.h>
#include <perfutil.h>
#include "perfsprc.h"

#define DEFAULT_INCR (64*1024)
#define STOP_AT ((PVOID)(0xFFFFFFFF80000000))

// Function Prototypes

PPROCESS_VA_INFO
GetProcessVaData (
    IN PSYSTEM_PROCESS_INFORMATION
);

PMODINFO
GetModuleVaData (
    PLDR_DATA_TABLE_ENTRY,  // module information structure
    PPROCESS_VA_INFO        // process data structure
);

BOOL
FreeProcessVaData (
    IN PPROCESS_VA_INFO
);

BOOL
FreeModuleVaData (
    IN PMODINFO
);


PMODINFO
LocateModInfo(
    IN PMODINFO,
    IN PVOID,
    IN SIZE_T
    );

DWORD
ProtectionToIndex(
    IN ULONG
    );

DWORD   dwProcessCount;
DWORD   dwModuleCount;


PPROCESS_VA_INFO
GetSystemVaData (
    IN PSYSTEM_PROCESS_INFORMATION pFirstProcess
)
/*++

GetSystemVaData

    Obtains the Process and Image Virtual Address information for all
    processes running on the system. (note that the routines called by
    this function allocate data structures consequently the corresponding
    FreeSystemVaData must be called to prevent memory "leaks")

Arguments

    IN PSYSTEM_PROCESS_INFORMATION
        pFirstProcess
            Pointer to first process in list of process structures returned
            by NtQuerySystemInformation service

Return Value

    Pointer to first process in list of processes
    or NULL if unable to obtain data

--*/
{
    PSYSTEM_PROCESS_INFORMATION     pThisProcess;
    PPROCESS_VA_INFO                pReturnValue = NULL;
    PPROCESS_VA_INFO                pLastProcess;
    PPROCESS_VA_INFO                pNewProcess;
    DWORD                           dwStartTime;

    dwProcessCount = 0;
    dwModuleCount = 0;

    if (pFirstProcess != NULL) {
        pThisProcess = pFirstProcess;
        pLastProcess = NULL;

        while (TRUE) {  // loop exit is at bottom of loop
            dwStartTime = GetTickCount ();
            pNewProcess = GetProcessVaData(
                    pThisProcess);  // pointer to process Info structure
            if (pNewProcess) { // process data found OK
                pNewProcess->LookUpTime = GetTickCount() - dwStartTime;
                dwProcessCount++;
                if (!pLastProcess) {    // this is the first process returned
                    pReturnValue = pNewProcess; // save return value here
                } else {
                    pLastProcess->pNextProcess = pNewProcess;
                }
                pLastProcess = pNewProcess;
            }
            if ( pThisProcess->NextEntryOffset == 0 ) {
                break; // this is the last entry
            } else {   // point to the next process info structure
				pThisProcess = (PSYSTEM_PROCESS_INFORMATION)
					((PBYTE)pThisProcess + pThisProcess->NextEntryOffset);
            }
        }
        return pReturnValue;    // return pointer to first list entry
    } else {
        return NULL;
    }
}

PPROCESS_VA_INFO
GetProcessVaData (
    IN PSYSTEM_PROCESS_INFORMATION     pProcess
)
/*++

GetProcessVaData

    Gets the Virtual Memory usage details for the process passed in the
    argument list. Collects the data for all images in use by the process.

    Note that this routine allocates data structures that must be freed
    (using the FreeProcessVaData routine) when finished with them.


Arguments

    IN HANDLE hProcess
        handle to the process to collect data for

Return Value

    Pointer to completed Process VA info structure or
    NULL if unable to collect data
--*/
{
    NTSTATUS                Status;
    HANDLE                  hProcess;
    PPROCESS_VA_INFO        pThisProcess;
    PPEB                    pPeb;
    PPEB_LDR_DATA           Ldr;
    PLIST_ENTRY             LdrHead, LdrNext;
    LDR_DATA_TABLE_ENTRY    LdrEntryData, *pLdrEntry;
    PMODINFO                pNewModule, pLastModule;
    PVOID                   pBaseAddress;
    MEMORY_BASIC_INFORMATION VaBasicInfo;
    DWORD                   dwProtection;
    PMODINFO                pMod;
    SIZE_T                  dwRegionSize;
    OBJECT_ATTRIBUTES       obProcess;
    CLIENT_ID               ClientId;
    PUNICODE_STRING         pProcessNameBuffer;

    // get handle to process

    ClientId.UniqueThread = (HANDLE)NULL;
    ClientId.UniqueProcess = pProcess->UniqueProcessId;

    InitializeObjectAttributes(
        &obProcess,
        NULL,
        0,
        NULL,
        NULL
        );

    Status = NtOpenProcess(
        &hProcess,
        (ACCESS_MASK)PROCESS_ALL_ACCESS,
        &obProcess,
        &ClientId);

    if (! NT_SUCCESS(Status)){
        // unable to open the process, but still want to
        // create pThisProcess so we will not screw up
        // the process sequence.
        hProcess = 0;
//        return NULL;    // unable to open process
    }

    // allocate structure

    pThisProcess = ALLOCMEM (
        hLibHeap,
        HEAP_ZERO_MEMORY,
        sizeof (PROCESS_VA_INFO));

    if (pThisProcess) { // allocation successful
        // initialize fields

        pThisProcess->BasicInfo =  ALLOCMEM (
            hLibHeap,
            HEAP_ZERO_MEMORY,
            sizeof (PROCESS_BASIC_INFORMATION));

        if (!pThisProcess->BasicInfo) {
            // Bailout if unable to allocate memory
            goto PBailOut;
        }

        // zero process counters
        pThisProcess->MappedGuard = 0;
        pThisProcess->PrivateGuard = 0;
        pThisProcess->ImageReservedBytes = 0;
        pThisProcess->ImageFreeBytes = 0;
        pThisProcess->ReservedBytes = 0;
        pThisProcess->FreeBytes = 0;

        // get process short name from Process Info Structure

        // alloc a new buffer since GetProcessShortName reuses the name
        // buffer
        pThisProcess->pProcessName = ALLOCMEM (hLibHeap,
            HEAP_ZERO_MEMORY, (sizeof(UNICODE_STRING) + MAX_PROCESS_NAME_LENGTH));

        if (pThisProcess->pProcessName != NULL) {
            pThisProcess->pProcessName->Length = 0;
            pThisProcess->pProcessName->MaximumLength = MAX_PROCESS_NAME_LENGTH;
            pThisProcess->pProcessName->Buffer = (PWSTR)(&pThisProcess->pProcessName[1]);

            if (lProcessNameCollectionMethod == PNCM_MODULE_FILE) {
                pProcessNameBuffer = GetProcessSlowName (pProcess);
            } else {
               pProcessNameBuffer = GetProcessShortName (pProcess);
            }
            RtlCopyUnicodeString (pThisProcess->pProcessName,
                pProcessNameBuffer);
        } else {
            pThisProcess->pProcessName = NULL;
        }

        pThisProcess->dwProcessId = HandleToUlong(pProcess->UniqueProcessId);
        pThisProcess->hProcess = hProcess;

        // zero list pointers
        pThisProcess->pMemBlockInfo = NULL;
        pThisProcess->pNextProcess = NULL;

        if (hProcess) {

            Status = NtQueryInformationProcess (
                hProcess,
                ProcessBasicInformation,
                pThisProcess->BasicInfo,
                sizeof (PROCESS_BASIC_INFORMATION),
                NULL);

            if (!NT_SUCCESS(Status)){
                // if error reading data, then bail out
                goto SuccessExit;
            }

            // get pointer to the Process Environment Block

            pPeb = pThisProcess->BasicInfo->PebBaseAddress;

            // read address of loader information structure

            Status = NtReadVirtualMemory (
                hProcess,
                &pPeb->Ldr,
                &Ldr,
                sizeof (Ldr),
                NULL);

            // bail out if unable to read information

            if (!NT_SUCCESS(Status)){
                // if error reading data, then bail out
                goto SuccessExit;
            }

            //
            // get head pointer to linked list of memory modules used by
            // this process
            //

            LdrHead = &Ldr->InMemoryOrderModuleList;

            // Get address of next list entry

            Status = NtReadVirtualMemory (
                hProcess,
                &LdrHead->Flink,
                &LdrNext,
                sizeof (LdrNext),
                NULL);

            // bail out if unable to read information

            if (!NT_SUCCESS(Status)){
                // if error reading data, then bail out
                goto SuccessExit;
            }

            pLastModule = NULL;

            // walk down the list of modules until back at the top.
            // to list all the images in use by this process

            while ( LdrNext != LdrHead ) {
                // get record attached to list entry
	            pLdrEntry = CONTAINING_RECORD(LdrNext,
                                            LDR_DATA_TABLE_ENTRY,
                                            InMemoryOrderLinks);

                Status = NtReadVirtualMemory(
                            hProcess,
                            pLdrEntry,
                            &LdrEntryData,
                            sizeof(LdrEntryData),
                            NULL
                            );
                // if unable to read memory, then give up rest of search
                // and return what we have already.
                if ( !NT_SUCCESS(Status) ) {
                    goto SuccessExit;
                }


                pNewModule = GetModuleVaData (
                    &LdrEntryData,
                    pThisProcess);
                if (pNewModule) {   // if structure returned...
                    dwModuleCount++;
                    if (!pLastModule) { // if this is the first module...
                        // then set list head pointer
                        pThisProcess->pMemBlockInfo = pNewModule;
                    } else {
                        // otherwise link to list
                        pLastModule->pNextModule = pNewModule;
                    }
                    pLastModule = pNewModule;
                }
                LdrNext = LdrEntryData.InMemoryOrderLinks.Flink;
            } // end while not at end of list


            // now that we have a list of all images, query the process'
            // virtual memory for the list of memory blocks in use by this
            // process and assign them to the appropriate category of memory

            pBaseAddress = NULL;    // start at 0 and go to end of User VA space

            while (pBaseAddress < STOP_AT) { // truncate to 32-bit if necessary

                Status = NtQueryVirtualMemory (
                    hProcess,
                    pBaseAddress,
                    MemoryBasicInformation,
                    &VaBasicInfo,
                    sizeof(VaBasicInfo),
                    NULL);

                if (!NT_SUCCESS(Status)) {
                    goto SuccessExit;
                } else {
                    // get protection type for index into counter array
                    dwRegionSize = VaBasicInfo.RegionSize;
                    switch (VaBasicInfo.State) {
                        case MEM_COMMIT:
                            // if the memory is for an IMAGE, then search the image list
                            // for the corresponding image to update
                            dwProtection = ProtectionToIndex(VaBasicInfo.Protect);
                            if (VaBasicInfo.Type == MEM_IMAGE) {
                                // update process total
                                pThisProcess->MemTotals.CommitVector[dwProtection] += dwRegionSize;
                                pMod = LocateModInfo (pThisProcess->pMemBlockInfo, pBaseAddress, dwRegionSize);
                                if (pMod) { // if matching image found, then update
                                    pMod->CommitVector[dwProtection] += dwRegionSize;
                                    pMod->TotalCommit += dwRegionSize;
                                } else { // otherwise update orphan total
                                    pThisProcess->OrphanTotals.CommitVector[dwProtection] += dwRegionSize;
                                }
                            } else {
                                // if not assigned to an image, then update the process
                                // counters
                                if (VaBasicInfo.Type == MEM_MAPPED) {
                                    pThisProcess->MappedCommit[dwProtection] += dwRegionSize;
                                } else {
                                    pThisProcess->PrivateCommit[dwProtection] += dwRegionSize;
                                }
                            }
                            break;

                        case MEM_RESERVE:
                            if (VaBasicInfo.Type == MEM_IMAGE) {
                                pThisProcess->ImageReservedBytes += dwRegionSize;
                            } else {
                                pThisProcess->ReservedBytes += dwRegionSize;
                            }
                            break;

                        case MEM_FREE:
                            if (VaBasicInfo.Type == MEM_IMAGE) {
                                pThisProcess->ImageFreeBytes += dwRegionSize;
                            } else {
                                pThisProcess->FreeBytes += dwRegionSize;
                            }
                            break;

                        default:
                            break;
                    } // end switch (VaBasicInfo.State)
                } // endif QueryVM ok

                // go to next memory block

                pBaseAddress = (PVOID)((ULONG_PTR)pBaseAddress + dwRegionSize);

            } // end whil not at the end of  memory
        } // endif hProcess not NULL
    } // endif pThisProcess not NULL

SuccessExit:

    if (hProcess) CloseHandle(hProcess);

    return pThisProcess;

//
//  error recovery section, called when the routine is unable to
//  complete successfully to clean up before leaving
//

PBailOut:
    if (pThisProcess->BasicInfo) {
        FREEMEM (
            hLibHeap,
            0,
            pThisProcess->BasicInfo);
    }
    FREEMEM (
        hLibHeap,
        0,
        pThisProcess);
    if (hProcess) CloseHandle(hProcess);
    return NULL;
}

PMODINFO
GetModuleVaData (
    PLDR_DATA_TABLE_ENTRY ModuleListEntry,  // module information structure
    PPROCESS_VA_INFO    pProcess            // process data structure
)
/*++

GetModuleVaData

    Gets the Virtual Memory usage details for the module pointed to by the
    Process Memory Module List Entry argument in the argument list

    Note that this routine allocates data structures that must be freed
    (using the FreeModuleVaData routine) when finished with them.

Arguments

    IN HANDLE ModuleListEntry

Return Value

    Pointer to completed Module VA info structure or
    NULL if unable to collect data

--*/
{
    PMODINFO    pThisModule = NULL;    // module structure that is returned
    PUNICODE_STRING pusInstanceName = NULL;    // process->image
    PUNICODE_STRING pusLongInstanceName = NULL;    // process->fullimagepath
    UNICODE_STRING  usImageFileName = {0,0, NULL};	// image file name
    UNICODE_STRING  usExeFileName = {0,0, NULL};    // short name
    UNICODE_STRING  usNtFileName = {0,0, NULL};     // full Nt File Name

    PWCHAR          p,p1;
    PWCHAR      ImageNameBuffer = NULL;
    NTSTATUS    Status;
    HANDLE      hFile;
    HANDLE      hMappedFile;
    WORD        wStringSize;

    PVOID       MappedAddress;
    PVOID       MapBase;
    SIZE_T      dwMappedSize;

    PIMAGE_DOS_HEADER   DosHeader;
    PIMAGE_NT_HEADERS   FileHeader;

    LARGE_INTEGER       liSectionSize;
    PLARGE_INTEGER       pliSectionSize;
    LARGE_INTEGER       liSectionOffset;
    POBJECT_ATTRIBUTES  pobSection;
    OBJECT_ATTRIBUTES   obFile;
    IO_STATUS_BLOCK     IoStatusBlock;
    BOOL                bRetCode;
	WORD				wBufOffset;
	WORD				wDiffSize;

    // allocate this item's memory

    pThisModule = ALLOCMEM (
        hLibHeap,
        HEAP_ZERO_MEMORY,
        sizeof (MODINFO));

    if (!pThisModule) {
        return NULL;
    }

    // allocate this items Instance Name Buffer

    wStringSize = (WORD)(ModuleListEntry->BaseDllName.MaximumLength +
        sizeof (UNICODE_NULL));

    pusInstanceName = ALLOCMEM (
        hLibHeap,
        HEAP_ZERO_MEMORY,
        wStringSize + sizeof(UNICODE_STRING))  ;

    if (!pusInstanceName) {
        goto MBailOut;
    }

    pusInstanceName->Length = 0;
    pusInstanceName->MaximumLength = wStringSize;
    pusInstanceName->Buffer = (PWCHAR)&pusInstanceName[1];

    // save instance name using full file path

    wStringSize = (WORD)(ModuleListEntry->FullDllName.MaximumLength +
        sizeof (UNICODE_NULL));

    pusLongInstanceName = ALLOCMEM (
        hLibHeap,
        HEAP_ZERO_MEMORY,
        wStringSize + sizeof (UNICODE_STRING));

    if (!pusLongInstanceName) {
        goto MBailOut;
    }

    pusLongInstanceName->Length = 0;
    pusLongInstanceName->MaximumLength = wStringSize;
    pusLongInstanceName->Buffer = (PWCHAR)&pusLongInstanceName[1];

    // allocate temporary buffer for image name

    usImageFileName.Length = ModuleListEntry->FullDllName.Length;
    usImageFileName.MaximumLength = ModuleListEntry->FullDllName.MaximumLength;
    ImageNameBuffer = usImageFileName.Buffer = ALLOCMEM(
        hLibHeap,
        HEAP_ZERO_MEMORY,
        usImageFileName.MaximumLength);
    if ( !usImageFileName.Buffer ) {
        goto MBailOut;
    }

    // allocate temporary buffer for exe name

    usExeFileName.Length = ModuleListEntry->BaseDllName.Length;
    usExeFileName.MaximumLength = ModuleListEntry->BaseDllName.MaximumLength;
    usExeFileName.Buffer = ALLOCMEM(
        hLibHeap,
        HEAP_ZERO_MEMORY,
        usExeFileName.MaximumLength);
    if ( !usExeFileName.Buffer ) {
        goto MBailOut;
    }

    // read base .exe/.dll name of image

    Status = NtReadVirtualMemory(
            pProcess->hProcess,
        	ModuleListEntry->BaseDllName.Buffer,
        	usExeFileName.Buffer,
        	usExeFileName.MaximumLength,
            NULL
            );
    if ( !NT_SUCCESS(Status) ) {
        goto MBailOut;
    }

    // read full name of image

    Status = NtReadVirtualMemory(
            pProcess->hProcess,
        	ModuleListEntry->FullDllName.Buffer,
        	usImageFileName.Buffer,
        	usImageFileName.MaximumLength,
            NULL
            );

    if ( !NT_SUCCESS(Status) ) {
        goto MBailOut;
    }

    // make a DOS filename to convert to NT again

	wDiffSize = wBufOffset = 0;
    p = p1 = usImageFileName.Buffer;
    while (*p != (WCHAR)0){
        if (*p == L':'){
            p1 = p;
			wDiffSize = wBufOffset;
        }
		wBufOffset += sizeof(WCHAR);
        p++;
    }
    if (p1 != usImageFileName.Buffer) {
		// move pointer
        usImageFileName.Buffer = --p1;
		// adjust length fields
		wDiffSize -= (WORD)(sizeof(WCHAR));
		usImageFileName.Length -= wDiffSize;
		usImageFileName.MaximumLength -= wDiffSize;
    }

    // Create/copy a NT filename for Nt file operation

    bRetCode = RtlDosPathNameToNtPathName_U (
        usImageFileName.Buffer,
        &usNtFileName,
        NULL,
        NULL);

    if ( !bRetCode ) {
        goto MBailOut;
    }

    // get handle to file

    InitializeObjectAttributes(
        &obFile,
        &usNtFileName,
        FILE_ATTRIBUTE_NORMAL | OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtCreateFile (
        &hFile,
        (ACCESS_MASK)GENERIC_READ | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
        &obFile,
        &IoStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL & FILE_ATTRIBUTE_VALID_FLAGS,
        FILE_SHARE_READ,
        FILE_OPEN,
        0,
        NULL,
        0);

    if (!NT_SUCCESS(Status)) {
        goto MBailOut;
    }

    pliSectionSize = &liSectionSize;
    liSectionSize.HighPart = 0;
    liSectionSize.LowPart = 0;

    InitializeObjectAttributes (
        &obFile,
        NULL,
        0,
        NULL,
        NULL);

    Status = NtCreateSection (
        &hMappedFile,
        SECTION_QUERY | SECTION_MAP_READ,
        &obFile,
        pliSectionSize,
        PAGE_READONLY,
        SEC_COMMIT,
        hFile);

    if ( ! NT_SUCCESS(Status)) {
        CloseHandle(hFile);
        goto MBailOut;
        }

    // get pointer to mapped memory
    MappedAddress = MapBase = NULL;
    dwMappedSize = 0;

    liSectionOffset.LowPart = 0;
    liSectionOffset.HighPart = 0;

    Status = NtMapViewOfSection (
        hMappedFile,
        NtCurrentProcess(),
        &MapBase,
        0L,
        0L,
        &liSectionOffset,
        &dwMappedSize,
        ViewShare,
        0L,
        PAGE_READONLY);

    CloseHandle(hMappedFile);

    if (NT_SUCCESS(Status)) {
        MappedAddress = MapBase;
    } else {
        CloseHandle(hFile);
        goto MBailOut;
    }

    // check for dos image signature (if a dos file)

    DosHeader = (PIMAGE_DOS_HEADER)MappedAddress;

    if ( DosHeader->e_magic != IMAGE_DOS_SIGNATURE ) {
        UnmapViewOfFile(MappedAddress);
        CloseHandle(hFile);
        goto MBailOut;
        }

    FileHeader = (PIMAGE_NT_HEADERS)((UINT_PTR)DosHeader + DosHeader->e_lfanew);

    if ( FileHeader->Signature != IMAGE_NT_SIGNATURE ) {
        UnmapViewOfFile(MappedAddress);
        CloseHandle(hFile);
        goto MBailOut;
        }

    // get base address for this module and save in local data structure

    pThisModule->BaseAddress = ModuleListEntry->DllBase;

    // get image name

    RtlCopyUnicodeString (
        pusInstanceName,
        &usExeFileName);

    RtlCopyUnicodeString (
        pusLongInstanceName,
        &usImageFileName);

    pThisModule->InstanceName = pusInstanceName;
    pThisModule->LongInstanceName = pusLongInstanceName;
    pThisModule->pNextModule = NULL;
    pThisModule->TotalCommit = 0;

    memset (
        &pThisModule->CommitVector[0], 0,
        sizeof (pThisModule->CommitVector));

    pThisModule->VirtualSize = FileHeader->OptionalHeader.SizeOfImage;

    // close file handles

    UnmapViewOfFile(MappedAddress);
    CloseHandle(hFile);

    // free local memory
    FREEMEM (
        RtlProcessHeap(),   // this is allocated by an RTL function
        0,
        usNtFileName.Buffer);

//    FREEMEM (
//        hLibHeap,
//        0,
//        RelativeName.RelativeName.Buffer);

    FREEMEM (
        hLibHeap,
        0,
        ImageNameBuffer);

    FREEMEM (
        hLibHeap,
        0,
        usExeFileName.Buffer);

    return (pThisModule);   // return pointer to completed module structure
//
//  Module bail out point, called when the routine is unable to continue
//  for some reason. This cleans up any allocated memory, etc.
//
MBailOut:

    if (pThisModule) {
        FREEMEM (
            hLibHeap,
            0,
            pThisModule);
    }

    if (usNtFileName.Buffer) {
        FREEMEM (
            RtlProcessHeap(),   // this is allocated by an RTL function
            0,
            usNtFileName.Buffer);
    }

//    if (RelativeName.RelativeName.Buffer) {
//        FREEMEM (
//            hLibHeap,
//            0,
//            RelativeName.RelativeName.Buffer);
//    }

    if (pusInstanceName) {
        FREEMEM (
            hLibHeap,
            0,
            pusInstanceName);

        }

    if (pusLongInstanceName) {
        FREEMEM (
            hLibHeap,
            0,
            pusLongInstanceName);

        }

    if (ImageNameBuffer) {
        FREEMEM (
            hLibHeap,
            0,
            ImageNameBuffer);
        }

    if (usExeFileName.Buffer){
        FREEMEM (
            hLibHeap,
            0,
            usExeFileName.Buffer);
        }

    return NULL;
}

PMODINFO
LocateModInfo(
    IN PMODINFO    pFirstMod,
    IN PVOID    pAddress,
    IN SIZE_T   dwExtent
    )
/*++

LocateModInfo

    Locates the images associated with the address passed in the argument list

Arguments

    IN PMODINFO pFirstMod,
        first module entry  in process list

    IN PVOID Address
        Address to search for in list

Return Value

    Pointer to matching image or
    NULL if no match found

--*/
{
    PMODINFO    pThisMod;

    pThisMod = pFirstMod;

    while (pThisMod)  { // go to end of list or match is found

        // match criteria are:
        //  address >= Module BaseAddress  and
        //  address+extent between base and base+image_extent

        if (pAddress >= pThisMod->BaseAddress) {
            if ((PVOID)((PDWORD)pAddress + dwExtent) <=
                (PVOID)((ULONG_PTR)pThisMod->BaseAddress+pThisMod->VirtualSize)) {
                return (pThisMod);
            }
        }

        pThisMod = pThisMod->pNextModule;

    }

    return NULL;
}

DWORD
ProtectionToIndex(
    IN ULONG Protection
    )
/*++

ProtectionToIndex

    Determine the memory access protection type and return local code

Arguments

   IN ULONG
        Protection

        Process memory protection mask

Return Value

    Local value of protection type

--*/
{
    Protection &= (PAGE_NOACCESS |
                    PAGE_READONLY |
                    PAGE_READWRITE |
                    PAGE_WRITECOPY |
                    PAGE_EXECUTE |
                    PAGE_EXECUTE_READ |
                    PAGE_EXECUTE_READWRITE |
                    PAGE_EXECUTE_WRITECOPY);

    switch ( Protection ) {

        case PAGE_NOACCESS:
                return NOACCESS;

        case PAGE_READONLY:
                return READONLY;

        case PAGE_READWRITE:
                return READWRITE;

        case PAGE_WRITECOPY:
                return WRITECOPY;

        case PAGE_EXECUTE:
                return EXECUTE;

        case PAGE_EXECUTE_READ:
                return EXECUTEREAD;

        case PAGE_EXECUTE_READWRITE:
                return EXECUTEREADWRITE;

        case PAGE_EXECUTE_WRITECOPY:
                return EXECUTEWRITECOPY;
        default:
                return 0xFFFFFFFF;
        }
}

BOOL
FreeSystemVaData (
    IN PPROCESS_VA_INFO pFirstProcess
)
{
    PPROCESS_VA_INFO pThisProcess, pNextProcess;

    pThisProcess = pFirstProcess;
    while (pThisProcess) {
        pNextProcess = pThisProcess->pNextProcess;  // save pointer to next
        FreeProcessVaData (pThisProcess);
        pThisProcess = pNextProcess;    // do next until NULL pointer
    }
    return (FALSE);
}

BOOL
FreeProcessVaData (
    IN PPROCESS_VA_INFO pProcess
)
{
    PMODINFO    pThisModule, pNextModule;

    if (pProcess) {
        if (pProcess->pProcessName) {
            FREEMEM (
                hLibHeap,
                0,
                pProcess->pProcessName);
            pProcess->pProcessName = NULL;
        }
        if (pProcess->BasicInfo) {
            FREEMEM (
                hLibHeap,
                0,
                pProcess->BasicInfo);
            pProcess->BasicInfo = NULL;
        }


        pThisModule = pProcess->pMemBlockInfo;
        while (pThisModule) {
            pNextModule = pThisModule->pNextModule;
            FreeModuleVaData (pThisModule);
            pThisModule = pNextModule;
        }
        //
        //  and finally throw ourselves away
        //
        FREEMEM (
            hLibHeap,
            0,
            pProcess);
    }
    return FALSE;
}

BOOL
FreeModuleVaData (
    IN PMODINFO pModule
)
{
    if (pModule) {
        if (pModule->InstanceName) {
            FREEMEM(
                hLibHeap,
                0,
                pModule->InstanceName);
            pModule->InstanceName = NULL;
        }
        if (pModule->LongInstanceName) {
            FREEMEM(
                hLibHeap,
                0,
                pModule->LongInstanceName);
            pModule->LongInstanceName = NULL;
        }
        FREEMEM (
            hLibHeap,
            0,
            pModule);
    }

    return FALSE;
}
