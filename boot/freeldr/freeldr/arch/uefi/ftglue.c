/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     FreeType glue for the UEFI loader
 * COPYRIGHT:   Copyright 2026 Ahmed ARIF <arif.ing@outlook.com>
 */

#include <stdio.h>
#include <stdarg.h>
#include <setjmp.h>

#include <uefildr.h>

extern EFI_SYSTEM_TABLE* GlobalSystemTable;

typedef struct _FTGLUE_ALLOC_HEADER
{
    PVOID RawAllocation;
    SIZE_T Size;
} FTGLUE_ALLOC_HEADER, *PFTGLUE_ALLOC_HEADER;

#define FTGLUE_ALLOC_ALIGNMENT 16

static
PVOID
FtGlueAllocatePool(
    _In_ SIZE_T Size)
{
    EFI_STATUS Status;
    PVOID Buffer = NULL;

    if (!GlobalSystemTable || !GlobalSystemTable->BootServices || (Size == 0))
        return NULL;

    Status = GlobalSystemTable->BootServices->AllocatePool(EfiLoaderData,
                                                           Size,
                                                           &Buffer);
    return EFI_ERROR(Status) ? NULL : Buffer;
}

void*
malloc(
    size_t Size)
{
    PUCHAR AlignedObject;
    PFTGLUE_ALLOC_HEADER Header;
    SIZE_T TotalSize;
    PUCHAR RawAllocation;

    if (Size == 0)
        return NULL;

    if (Size > MAXULONG_PTR - sizeof(*Header) - FTGLUE_ALLOC_ALIGNMENT + 1)
        return NULL;

    TotalSize = Size + sizeof(*Header) + FTGLUE_ALLOC_ALIGNMENT - 1;
    RawAllocation = FtGlueAllocatePool(TotalSize);
    if (!RawAllocation)
        return NULL;

    /*
     * FreeType stores internal records with natural x64 alignment
     * expectations, but UEFI pool allocations only guarantee pool
     * alignment. Realign the public pointer explicitly and keep the raw
     * allocation for FreePool().
     */
    AlignedObject = (PUCHAR)(((ULONG_PTR)(RawAllocation + sizeof(*Header) +
                                          FTGLUE_ALLOC_ALIGNMENT - 1)) &
                              ~((ULONG_PTR)FTGLUE_ALLOC_ALIGNMENT - 1));
    Header = (PFTGLUE_ALLOC_HEADER)(AlignedObject - sizeof(*Header));
    Header->RawAllocation = RawAllocation;
    Header->Size = Size;
    return AlignedObject;
}

void
free(
    void* Object)
{
    PFTGLUE_ALLOC_HEADER Header;

    if (!Object || !GlobalSystemTable || !GlobalSystemTable->BootServices)
        return;

    Header = (PFTGLUE_ALLOC_HEADER)((PUCHAR)Object - sizeof(*Header));
    GlobalSystemTable->BootServices->FreePool(Header->RawAllocation);
}

void*
realloc(
    void* Object,
    size_t Size)
{
    PVOID NewObject;
    PFTGLUE_ALLOC_HEADER Header;
    SIZE_T CopySize;

    if (!Object)
        return malloc(Size);

    if (Size == 0)
    {
        free(Object);
        return NULL;
    }

    NewObject = malloc(Size);
    if (!NewObject)
        return NULL;

    Header = (PFTGLUE_ALLOC_HEADER)((PUCHAR)Object - sizeof(*Header));
    CopySize = min(Header->Size, Size);
    RtlCopyMemory(NewObject, Object, CopySize);
    free(Object);
    return NewObject;
}

void*
calloc(
    size_t Count,
    size_t Size)
{
    SIZE_T TotalSize;
    PVOID Buffer;

    if ((Count == 0) || (Size == 0))
        return NULL;

    if (Count > MAXULONG_PTR / Size)
        return NULL;

    TotalSize = Count * Size;
    Buffer = malloc(TotalSize);
    if (!Buffer)
        return NULL;

    RtlZeroMemory(Buffer, TotalSize);
    return Buffer;
}

static
VOID
FtGlueLog(
    _In_ PCSTR Format,
    _In_ va_list Args)
{
    CHAR Buffer[256];

    if (!Format)
        return;

    _vsnprintf(Buffer, sizeof(Buffer), Format, Args);
    Buffer[sizeof(Buffer) - 1] = ANSI_NULL;
    DbgPrint("FreeType: %s", Buffer);
}

void
FT_Message(
    const char* Format,
    ...)
{
    va_list Args;

    va_start(Args, Format);
    FtGlueLog(Format, Args);
    va_end(Args);
}

void
FT_Panic(
    const char* Format,
    ...)
{
    va_list Args;

    va_start(Args, Format);
    FtGlueLog(Format, Args);
    va_end(Args);

    BugCheck("%s", "FreeType panic");
}

#if defined(_M_IX86)
__declspec(noreturn)
void
__longjmp_nounwind(
    const _JUMP_BUFFER* _Buf,
    int _Value);
#elif defined(_M_AMD64)
__declspec(noreturn)
void
__longjmp_noframe(
    const _JUMP_BUFFER* _Buf,
    int _Value);
#endif

__declspec(noreturn)
void
longjmp(
    jmp_buf _Buf,
    int _Value)
{
    const _JUMP_BUFFER* JumpBuffer = (_JUMP_BUFFER*)_Buf;

#if defined(_M_IX86)
    __longjmp_nounwind(JumpBuffer, (_Value == 0) ? 1 : _Value);
#elif defined(_M_AMD64)
    __longjmp_noframe(JumpBuffer, (_Value == 0) ? 1 : _Value);
#else
#error Unsupported UEFI FreeType longjmp target architecture
#endif
}

__declspec(noreturn)
void
ms_longjmp(
    jmp_buf _Buf,
    int _Value)
{
    longjmp(_Buf, _Value);
}

FILE*
fopen(
    const char* FileName,
    const char* Mode)
{
    UNREFERENCED_PARAMETER(FileName);
    UNREFERENCED_PARAMETER(Mode);
    return NULL;
}

int
fseek(
    FILE* Stream,
    long Offset,
    int Origin)
{
    UNREFERENCED_PARAMETER(Stream);
    UNREFERENCED_PARAMETER(Offset);
    UNREFERENCED_PARAMETER(Origin);
    return -1;
}

long
ftell(
    FILE* Stream)
{
    UNREFERENCED_PARAMETER(Stream);
    return -1;
}

size_t
fread(
    void* Buffer,
    size_t Size,
    size_t Count,
    FILE* Stream)
{
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(Size);
    UNREFERENCED_PARAMETER(Count);
    UNREFERENCED_PARAMETER(Stream);
    return 0;
}

int
fclose(
    FILE* Stream)
{
    UNREFERENCED_PARAMETER(Stream);
    return EOF;
}
