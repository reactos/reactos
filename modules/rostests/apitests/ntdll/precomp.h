#ifndef _NTDLL_APITEST_PRECOMP_H_
#define _NTDLL_APITEST_PRECOMP_H_

#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <apitest.h>
#include <ndk/ntndk.h>
#include <strsafe.h>

BOOL
Is64BitSystem(VOID)
{
#ifdef _WIN64
    return TRUE;
#else
    NTSTATUS Status;
    ULONG_PTR IsWow64;

    Status = NtQueryInformationProcess(NtCurrentProcess(),
                                       ProcessWow64Information,
                                       &IsWow64,
                                       sizeof(IsWow64),
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        return IsWow64 != 0;
    }

    return FALSE;
#endif
}

ULONG
SizeOfMdl(VOID)
{
    return Is64BitSystem() ? 48 : 28;
}

PVOID
AllocateGuarded(
    _In_ SIZE_T SizeRequested)
{
    NTSTATUS Status;
    SIZE_T Size = PAGE_ROUND_UP(SizeRequested + PAGE_SIZE);
    PVOID VirtualMemory = NULL;
    PCHAR StartOfBuffer;

    Status = NtAllocateVirtualMemory(NtCurrentProcess(), &VirtualMemory, 0, &Size, MEM_RESERVE, PAGE_NOACCESS);

    if (!NT_SUCCESS(Status))
        return NULL;

    Size -= PAGE_SIZE;
    if (Size)
    {
        Status = NtAllocateVirtualMemory(NtCurrentProcess(), &VirtualMemory, 0, &Size, MEM_COMMIT, PAGE_READWRITE);
        if (!NT_SUCCESS(Status))
        {
            Size = 0;
            Status = NtFreeVirtualMemory(NtCurrentProcess(), &VirtualMemory, &Size, MEM_RELEASE);
            ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
            return NULL;
        }
    }

    StartOfBuffer = VirtualMemory;
    StartOfBuffer += Size - SizeRequested;

    return StartOfBuffer;
}

VOID
FreeGuarded(
    _In_ PVOID Pointer)
{
    NTSTATUS Status;
    PVOID VirtualMemory = (PVOID)PAGE_ROUND_DOWN((SIZE_T)Pointer);
    SIZE_T Size = 0;

    Status = NtFreeVirtualMemory(NtCurrentProcess(), &VirtualMemory, &Size, MEM_RELEASE);
    ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
}

PACL
MakeAcl(
    _In_ ULONG AceCount,
    ...)
{
    PACL Acl;
    PACE_HEADER AceHeader;
    ULONG AclSize;
    ULONG AceSizes[10];
    ULONG i;
    va_list Args;

    ASSERT(AceCount <= RTL_NUMBER_OF(AceSizes));
    AclSize = sizeof(ACL);
    va_start(Args, AceCount);
    for (i = 0; i < AceCount; i++)
    {
        AceSizes[i] = va_arg(Args, int);
        AclSize += AceSizes[i];
    }
    va_end(Args);

    Acl = AllocateGuarded(AclSize);
    if (!Acl)
    {
        skip("Failed to allocate %lu bytes\n", AclSize);
        return NULL;
    }

    Acl->AclRevision = ACL_REVISION;
    Acl->Sbz1 = 0;
    Acl->AclSize = AclSize;
    Acl->AceCount = AceCount;
    Acl->Sbz2 = 0;

    AceHeader = (PACE_HEADER)(Acl + 1);
    for (i = 0; i < AceCount; i++)
    {
        AceHeader->AceType = 0;
        AceHeader->AceFlags = 0;
        AceHeader->AceSize = AceSizes[i];
        AceHeader = (PACE_HEADER)((PCHAR)AceHeader + AceHeader->AceSize);
    }

    return Acl;
}

BOOLEAN
CheckStringBuffer(
    PCWSTR Buffer,
    SIZE_T Length,
    SIZE_T MaximumLength,
    PCWSTR Expected)
{
    SIZE_T ExpectedLength = wcslen(Expected) * sizeof(WCHAR);
    SIZE_T EqualLength;
    BOOLEAN Result = TRUE;
    SIZE_T i;

    if (Length != ExpectedLength)
    {
        ok(0, "String length is %lu, expected %lu\n", (ULONG)Length, (ULONG)ExpectedLength);
        Result = FALSE;
    }

    EqualLength = RtlCompareMemory(Buffer, Expected, Length);
    if (EqualLength != Length)
    {
        ok(0, "String is '%S', expected '%S'\n", Buffer, Expected);
        Result = FALSE;
    }

    if (Buffer[Length / sizeof(WCHAR)] != UNICODE_NULL)
    {
        ok(0, "Not null terminated\n");
        Result = FALSE;
    }

    /* The function nulls the rest of the buffer! */
    for (i = Length + sizeof(UNICODE_NULL); i < MaximumLength; i++)
    {
        UCHAR Char = ((PUCHAR)Buffer)[i];
        if (Char != 0)
        {
            ok(0, "Found 0x%x at offset %lu, expected 0x%x\n", Char, (ULONG)i, 0);
            /* Don't count this as a failure unless the string was actually wrong */
            //Result = FALSE;
            /* Don't flood the log */
            break;
        }
    }

    return Result;
}

BOOLEAN
CheckBuffer(
    PVOID Buffer,
    SIZE_T Size,
    UCHAR Value)
{
    PUCHAR Array = Buffer;
    SIZE_T i;

    for (i = 0; i < Size; i++)
    {
        if (Array[i] != Value)
        {
            trace("Expected %x, found %x at offset %lu\n", Value, Array[i], (ULONG)i);
            return FALSE;
        }
    }
    return TRUE;
}

typedef enum
{
    PrefixNone,
    PrefixCurrentDrive,
    PrefixCurrentPath,
    PrefixCurrentPathWithoutLastPart
} PREFIX_TYPE;

#endif /* _NTDLL_APITEST_PRECOMP_H_ */
