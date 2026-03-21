#define WIN32_NO_STATUS
#include <windows.h>
#include <ndk/exfuncs.h>
#include <stdio.h>

int main(void)
{
    NTSTATUS Status;
    ULONG ReturnLength = 0;
    ULONG BufferSize = 4096;
    PSYSTEM_VERIFIER_INFORMATION Info;
    PSYSTEM_VERIFIER_INFORMATION Entry;

    Info = HeapAlloc(GetProcessHeap(), 0, BufferSize);
    if (!Info)
    {
        printf("Out of memory\n");
        return 1;
    }

    Status = NtQuerySystemInformation(SystemVerifierInformation,
                                      Info,
                                      BufferSize,
                                      &ReturnLength);

    if (Status == STATUS_NOT_IMPLEMENTED)
    {
        printf("Verifier not enabled\n");
        return 1;
    }

    if (Status == STATUS_INFO_LENGTH_MISMATCH)
    {
        printf("Buffer too small, need %lu bytes\n", ReturnLength);
        return 1;
    }

    if (!NT_SUCCESS(Status))
    {
        printf("NtQuerySystemInformation failed: 0x%08lx\n", Status);
        return 1;
    }

    Entry = Info;
    while (TRUE)
    {
        PWSTR Name = (PWSTR)((PUCHAR)Entry + sizeof(SYSTEM_VERIFIER_INFORMATION));
        printf("Driver: %.*ws\n", Entry->DriverName.Length / sizeof(WCHAR), Name);
        printf("  Level:                          0x%lx\n", Entry->Level);
        printf("  AllocationsAttempted:           %lu\n", Entry->AllocationsAttempted);
        printf("  AllocationsSucceeded:           %lu\n", Entry->AllocationsSucceeded);
        printf("  AllocationsSucceededSpecialPool:%lu\n", Entry->AllocationsSucceededSpecialPool);
        printf("  AllocationsFailed:              %lu\n", Entry->AllocationsFailed);
        printf("  AllocationsWithNoTag:           %lu\n", Entry->AllocationsWithNoTag);
        printf("  CurrentNonPagedPoolAllocations: %lu\n", Entry->CurrentNonPagedPoolAllocations);
        printf("  CurrentPagedPoolAllocations:    %lu\n", Entry->CurrentPagedPoolAllocations);
        printf("  NonPagedPoolUsageInBytes:       %lu\n", (ULONG)Entry->NonPagedPoolUsageInBytes);
        printf("  Loads:                          %lu\n", Entry->Loads);
        printf("  Unloads:                        %lu\n", Entry->Unloads);
        printf("\n");

        if (Entry->NextEntryOffset == 0)
            break;
        Entry = (PSYSTEM_VERIFIER_INFORMATION)((PUCHAR)Entry + Entry->NextEntryOffset);
    }

    HeapFree(GetProcessHeap(), 0, Info);
    return 0;
}