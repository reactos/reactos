/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Tests for loading images that carry a COM descriptor (.NET / CLR images)
 *              through LdrLoadDll
 * COPYRIGHT:   Copyright 2026 Abdias Joel Moya Perez <abdi.moya@gmail.com>
 */

#include "precomp.h"

#define FILE_ALIGNMENT      0x200
#define TEXT_RVA            0x1000
#define DATA_RVA            0x2000
#define TEST_IMAGE_SIZE     0x3000
#define TEST_IMAGE_BASE     0x5E000000

#define DATA_FIELD_RVA(Field) (DATA_RVA + FIELD_OFFSET(TEST_DATA, Field))

/*
 * The entry point of the test images is a stub that returns TRUE or FALSE. That is enough to
 * tell whether the loader called it, because a DllMain that returns FALSE fails the load.
 */
#if defined(_M_IX86)
#define HAVE_ENTRY_CODE
/* mov eax, 1; ret 12 */
static const UCHAR EntryTrueCode[] = { 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC2, 0x0C, 0x00 };
/* xor eax, eax; ret 12 */
static const UCHAR EntryFalseCode[] = { 0x33, 0xC0, 0xC2, 0x0C, 0x00 };
#elif defined(_M_AMD64)
#define HAVE_ENTRY_CODE
/* mov eax, 1; ret */
static const UCHAR EntryTrueCode[] = { 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3 };
/* xor eax, eax; ret */
static const UCHAR EntryFalseCode[] = { 0x33, 0xC0, 0xC3 };
#endif

typedef enum _ENTRY_KIND
{
    EntryNone,
    EntryReturnsTrue,
    EntryReturnsFalse,
} ENTRY_KIND;

/*
 * The smallest metadata root that a CLR image validator accepts: signature, version 1.1,
 * a version string, no flags and no streams. It is not a usable assembly (a real one has
 * tables and heaps behind the streams). The loader only cares that the descriptor is sound.
 */
typedef struct _COR_METADATA
{
    ULONG Signature;
    USHORT MajorVersion;
    USHORT MinorVersion;
    ULONG Reserved;
    ULONG VersionLength;
    CHAR Version[12];
    USHORT Flags;
    USHORT Streams;
} COR_METADATA;

typedef struct _TEST_HEADERS
{
    IMAGE_DOS_HEADER Dos;
    IMAGE_NT_HEADERS Nt;
    IMAGE_SECTION_HEADER Sections[2];
} TEST_HEADERS;

/* Everything in the data section: one import from kernel32 and, optionally, the CLR data */
typedef struct _TEST_DATA
{
    IMAGE_IMPORT_DESCRIPTOR Imports[2];
    IMAGE_THUNK_DATA Lookup[2];
    IMAGE_THUNK_DATA Iat[2];
    struct
    {
        WORD Hint;
        CHAR Name[20];
    } ImportByName;
    CHAR ImportedDll[16];
    IMAGE_COR20_HEADER CorHeader;
    COR_METADATA Metadata;
} TEST_DATA;

/* The image as it is stored in the file: headers, a code section and a data section */
typedef struct _TEST_FILE
{
    TEST_HEADERS Headers;
    UCHAR HeadersPadding[FILE_ALIGNMENT - sizeof(TEST_HEADERS)];
    UCHAR Code[FILE_ALIGNMENT];
    TEST_DATA Data;
    UCHAR DataPadding[FILE_ALIGNMENT - sizeof(TEST_DATA)];
} TEST_FILE;

C_ASSERT(sizeof(TEST_FILE) == 3 * FILE_ALIGNMENT);
C_ASSERT(sizeof(COR_METADATA) == 32);

typedef VOID (*PBREAK_IMAGE)(_Inout_ TEST_FILE *File);

/* Fills in a well-formed image. HasCor adds the COM descriptor, CorFlags are its flags. */
static
VOID
InitializeImage(
    _Out_ TEST_FILE *File,
    _In_ BOOLEAN HasCor,
    _In_ ULONG CorFlags,
    _In_ ENTRY_KIND Entry)
{
    PIMAGE_OPTIONAL_HEADER Optional = &File->Headers.Nt.OptionalHeader;
    PIMAGE_SECTION_HEADER Text = &File->Headers.Sections[0];
    PIMAGE_SECTION_HEADER Data = &File->Headers.Sections[1];

    RtlZeroMemory(File, sizeof(*File));

    File->Headers.Dos.e_magic = IMAGE_DOS_SIGNATURE;
    File->Headers.Dos.e_lfanew = sizeof(IMAGE_DOS_HEADER);

    File->Headers.Nt.Signature = IMAGE_NT_SIGNATURE;
    File->Headers.Nt.FileHeader.Machine = IMAGE_FILE_MACHINE_NATIVE;
    File->Headers.Nt.FileHeader.NumberOfSections = 2;
    File->Headers.Nt.FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER);
    File->Headers.Nt.FileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_DLL;
#ifndef _WIN64
    File->Headers.Nt.FileHeader.Characteristics |= IMAGE_FILE_32BIT_MACHINE;
#endif

    Optional->Magic = IMAGE_NT_OPTIONAL_HDR_MAGIC;
    Optional->SizeOfCode = FILE_ALIGNMENT;
    Optional->SizeOfInitializedData = FILE_ALIGNMENT;
    Optional->BaseOfCode = TEXT_RVA;
#ifndef _WIN64
    Optional->BaseOfData = DATA_RVA;
#endif
    Optional->ImageBase = TEST_IMAGE_BASE;
    Optional->SectionAlignment = PAGE_SIZE;
    Optional->FileAlignment = FILE_ALIGNMENT;
    Optional->MajorOperatingSystemVersion = 5;
    Optional->MajorSubsystemVersion = 5;
    Optional->SizeOfImage = TEST_IMAGE_SIZE;
    Optional->SizeOfHeaders = FILE_ALIGNMENT;
    Optional->Subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
    Optional->SizeOfStackReserve = 0x100000;
    Optional->SizeOfStackCommit = 0x1000;
    Optional->SizeOfHeapReserve = 0x100000;
    Optional->SizeOfHeapCommit = 0x1000;
    Optional->NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;

    RtlCopyMemory(Text->Name, ".text", sizeof(".text"));
    Text->Misc.VirtualSize = FILE_ALIGNMENT;
    Text->VirtualAddress = TEXT_RVA;
    Text->SizeOfRawData = FILE_ALIGNMENT;
    Text->PointerToRawData = FILE_ALIGNMENT;
    Text->Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;

    RtlCopyMemory(Data->Name, ".data", sizeof(".data"));
    Data->Misc.VirtualSize = sizeof(TEST_DATA);
    Data->VirtualAddress = DATA_RVA;
    Data->SizeOfRawData = FILE_ALIGNMENT;
    Data->PointerToRawData = 2 * FILE_ALIGNMENT;
    Data->Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA |
                            IMAGE_SCN_MEM_READ |
                            IMAGE_SCN_MEM_WRITE;

#ifdef HAVE_ENTRY_CODE
    if (Entry == EntryReturnsTrue)
    {
        RtlCopyMemory(File->Code, EntryTrueCode, sizeof(EntryTrueCode));
        Optional->AddressOfEntryPoint = TEXT_RVA;
    }
    else if (Entry == EntryReturnsFalse)
    {
        RtlCopyMemory(File->Code, EntryFalseCode, sizeof(EntryFalseCode));
        Optional->AddressOfEntryPoint = TEXT_RVA;
    }
#else
    UNREFERENCED_PARAMETER(Entry);
#endif

    /* One import, which the loader has to resolve. The IAT starts out equal to the lookup table */
    RtlCopyMemory(File->Data.ImportByName.Name,
                  "GetCurrentProcessId",
                  sizeof("GetCurrentProcessId"));
    RtlCopyMemory(File->Data.ImportedDll, "kernel32.dll", sizeof("kernel32.dll"));
    File->Data.Lookup[0].u1.AddressOfData = DATA_FIELD_RVA(ImportByName);
    File->Data.Iat[0].u1.AddressOfData = DATA_FIELD_RVA(ImportByName);
    File->Data.Imports[0].OriginalFirstThunk = DATA_FIELD_RVA(Lookup);
    File->Data.Imports[0].Name = DATA_FIELD_RVA(ImportedDll);
    File->Data.Imports[0].FirstThunk = DATA_FIELD_RVA(Iat);
    Optional->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress =
        DATA_FIELD_RVA(Imports);
    Optional->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = sizeof(File->Data.Imports);

    if (!HasCor)
        return;

    File->Data.Metadata.Signature = 0x424A5342; /* "BSJB" */
    File->Data.Metadata.MajorVersion = 1;
    File->Data.Metadata.MinorVersion = 1;
    File->Data.Metadata.VersionLength = sizeof(File->Data.Metadata.Version);
    RtlCopyMemory(File->Data.Metadata.Version, "v4.0.30319", sizeof("v4.0.30319"));

    File->Data.CorHeader.cb = sizeof(File->Data.CorHeader);
    File->Data.CorHeader.MajorRuntimeVersion = COR_VERSION_MAJOR_V2;
    File->Data.CorHeader.MinorRuntimeVersion = 5;
    File->Data.CorHeader.MetaData.VirtualAddress = DATA_FIELD_RVA(Metadata);
    File->Data.CorHeader.MetaData.Size = sizeof(File->Data.Metadata);
    File->Data.CorHeader.Flags = CorFlags;

    Optional->DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress =
        DATA_FIELD_RVA(CorHeader);
    Optional->DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size =
        sizeof(File->Data.CorHeader);
}

static
PIMAGE_DATA_DIRECTORY
GetComDescriptorDirectory(
    _In_ TEST_FILE *File)
{
    return &File->Headers.Nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR];
}

static
VOID
BreakZeroCorHeader(
    _Inout_ TEST_FILE *File)
{
    RtlZeroMemory(&File->Data.CorHeader, sizeof(File->Data.CorHeader));
}

static
VOID
BreakOldRuntimeVersion(
    _Inout_ TEST_FILE *File)
{
    File->Data.CorHeader.MajorRuntimeVersion = 1;
}

static
VOID
BreakCorHeaderSize(
    _Inout_ TEST_FILE *File)
{
    File->Data.CorHeader.cb = sizeof(IMAGE_COR20_HEADER) - 1;
}

static
VOID
BreakDirectorySize(
    _Inout_ TEST_FILE *File)
{
    GetComDescriptorDirectory(File)->Size = sizeof(IMAGE_COR20_HEADER) - 1;
}

static
VOID
BreakDirectoryOutsideImage(
    _Inout_ TEST_FILE *File)
{
    GetComDescriptorDirectory(File)->VirtualAddress = TEST_IMAGE_SIZE;
}

/* RVA + size is 0x38 as a 32-bit sum, which is inside the image */
static
VOID
BreakDirectoryWrapsAround(
    _Inout_ TEST_FILE *File)
{
    GetComDescriptorDirectory(File)->VirtualAddress = 0xFFFFFFF0;
}

static
VOID
BreakMetadataSignature(
    _Inout_ TEST_FILE *File)
{
    File->Data.Metadata.Signature = 0x4B4A5342;
}

static
VOID
BreakMetadataTooSmall(
    _Inout_ TEST_FILE *File)
{
    File->Data.CorHeader.MetaData.Size = 8;
}

static
VOID
BreakMetadataOutsideImage(
    _Inout_ TEST_FILE *File)
{
    File->Data.CorHeader.MetaData.VirtualAddress = TEST_IMAGE_SIZE - 4;
}

/* RVA + size is 0x10 as a 32-bit sum, which is inside the image */
static
VOID
BreakMetadataWrapsAround(
    _Inout_ TEST_FILE *File)
{
    File->Data.CorHeader.MetaData.VirtualAddress = 0xFFFFFFF0;
    File->Data.CorHeader.MetaData.Size = 0x20;
}

static
VOID
BreakVersionLength(
    _Inout_ TEST_FILE *File)
{
    File->Data.Metadata.VersionLength = 0x1000;
}

static const struct
{
    PCSTR Name;
    PBREAK_IMAGE Break;
} RejectedImages[] =
{
    { "all-zero COR header", BreakZeroCorHeader },
    { "runtime version 1", BreakOldRuntimeVersion },
    { "COR header size too small", BreakCorHeaderSize },
    { "COM descriptor directory too small", BreakDirectorySize },
    { "COM descriptor outside the image", BreakDirectoryOutsideImage },
    { "COM descriptor RVA wraps around", BreakDirectoryWrapsAround },
    { "bad metadata signature", BreakMetadataSignature },
    { "metadata too small", BreakMetadataTooSmall },
    { "metadata outside the image", BreakMetadataOutsideImage },
    { "metadata RVA wraps around", BreakMetadataWrapsAround },
    { "version string longer than the metadata", BreakVersionLength },
};

static
BOOL
WriteTempImage(
    _Out_writes_(MAX_PATH) PWSTR Path,
    _In_ const TEST_FILE *File)
{
    WCHAR TempDir[MAX_PATH];
    HANDLE Handle;
    DWORD Written;
    BOOL Success;

    if (!GetTempPathW(_countof(TempDir), TempDir) ||
        !GetTempFileNameW(TempDir, L"COR", 0, Path))
    {
        return FALSE;
    }

    Handle = CreateFileW(Path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (Handle == INVALID_HANDLE_VALUE)
    {
        DeleteFileW(Path);
        return FALSE;
    }

    Success = WriteFile(Handle, File, sizeof(*File), &Written, NULL) && Written == sizeof(*File);
    CloseHandle(Handle);
    if (!Success)
        DeleteFileW(Path);
    return Success;
}

/*
 * Counts import thunks that still hold their on-disk (unbound) value.
 * Returns -1 if the image has no import descriptor or no separate lookup table.
 */
static
LONG
CountUnboundImports(
    _In_ PVOID Base)
{
    PIMAGE_IMPORT_DESCRIPTOR Descriptor;
    ULONG Size;
    LONG Unbound = 0;
    ULONG Descriptors = 0;

    Descriptor = RtlImageDirectoryEntryToData(Base, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &Size);
    if (!Descriptor)
        return -1;

    for (; Descriptor->Name != 0; Descriptor++, Descriptors++)
    {
        PIMAGE_THUNK_DATA Lookup, Iat;

        if (Descriptor->OriginalFirstThunk == 0)
            return -1;

        Lookup = (PIMAGE_THUNK_DATA)((PUCHAR)Base + Descriptor->OriginalFirstThunk);
        Iat = (PIMAGE_THUNK_DATA)((PUCHAR)Base + Descriptor->FirstThunk);
        for (; Lookup->u1.AddressOfData != 0; Lookup++, Iat++)
        {
            if (Iat->u1.Function == Lookup->u1.Function)
                Unbound++;
        }
    }

    return Descriptors ? Unbound : -1;
}

static
NTSTATUS
LoadImageFile(
    _In_ PCWSTR Path,
    _Out_ PVOID *BaseAddress)
{
    UNICODE_STRING DllName;

    RtlInitUnicodeString(&DllName, Path);
    *BaseAddress = NULL;
    return LdrLoadDll(NULL, NULL, &DllName, BaseAddress);
}

static
BOOLEAN
IsMscoreeInstalled(VOID)
{
    WCHAR Path[MAX_PATH];
    UINT Length;

    Length = GetSystemDirectoryW(Path, _countof(Path));
    if (Length == 0 || Length + _countof(L"\\mscoree.dll") > _countof(Path))
        return FALSE;

    RtlCopyMemory(Path + Length, L"\\mscoree.dll", sizeof(L"\\mscoree.dll"));
    return GetFileAttributesW(Path) != INVALID_FILE_ATTRIBUTES;
}

/* Unloads an image and checks that it went away completely */
static
VOID
UnloadAndCheck(
    _In_ PCSTR Name,
    _In_ PCWSTR Path,
    _In_ PVOID Base)
{
    MEMORY_BASIC_INFORMATION Info;

    ok_ntstatus(LdrUnloadDll(Base), STATUS_SUCCESS);
    ok(GetModuleHandleW(Path) == NULL,
       "%s: still listed as loaded after the last unload\n", Name);
    ok(VirtualQuery(Base, &Info, sizeof(Info)) == sizeof(Info) &&
       (Info.State == MEM_FREE || Info.AllocationBase != Base),
       "%s: image is still mapped at %p after the last unload\n", Name, Base);
}

/*
 * A plain image, without a COM descriptor. The other tests are only meaningful if this
 * one loads, has its imports resolved and unloads like an image is supposed to.
 */
static
VOID
TestControlImage(VOID)
{
    TEST_FILE File;
    WCHAR Path[MAX_PATH];
    NTSTATUS Status;
    PVOID Base;
    LONG Unbound;

    InitializeImage(&File, FALSE, 0, EntryReturnsTrue);
    if (!WriteTempImage(Path, &File))
    {
        skip("Could not write the control image (%lu)\n", GetLastError());
        return;
    }

    Status = LoadImageFile(Path, &Base);
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status))
    {
        ok(GetModuleHandleW(Path) == Base, "control image not found by GetModuleHandle\n");

        Unbound = CountUnboundImports(Base);
        ok(Unbound >= 0, "control image has no measurable import table\n");
        if (Unbound >= 0)
            ok(Unbound == 0, "control image: %ld unbound imports\n", Unbound);

        UnloadAndCheck("control image", Path, Base);
    }

    ok(DeleteFileW(Path), "DeleteFileW failed with %lu\n", GetLastError());
}

/* An image with a COM descriptor that is not sound must not load, and must not stay mapped */
static
VOID
TestRejectedImage(
    _In_ PCSTR Name,
    _In_ PBREAK_IMAGE Break)
{
    TEST_FILE File;
    WCHAR Path[MAX_PATH];
    NTSTATUS Status;
    PVOID Base;

    InitializeImage(&File, TRUE, 0, EntryReturnsTrue);
    Break(&File);
    if (!WriteTempImage(Path, &File))
    {
        skip("%s: could not write the test image (%lu)\n", Name, GetLastError());
        return;
    }

    Status = LoadImageFile(Path, &Base);
    ok_ntstatus(Status, STATUS_INVALID_IMAGE_FORMAT);
    if (NT_SUCCESS(Status))
    {
        trace("%s: image was loaded at %p\n", Name, Base);
        LdrUnloadDll(Base);
    }
    else
    {
        ok(Base == NULL, "%s: BaseAddress = %p after failure\n", Name, Base);
        ok(GetModuleHandleW(Path) == NULL, "%s: rejected image is still loaded\n", Name);
    }

    /* A rejected load must not leave the file mapped or open */
    ok(DeleteFileW(Path), "%s: DeleteFileW failed with %lu\n", Name, GetLastError());
}

/*
 * A mixed-mode image is a native DLL that also carries CLR data. It has to load like any other
 * DLL: its native imports are resolved and unloading unmaps it.
 */
static
VOID
TestMixedModeImage(VOID)
{
    TEST_FILE File;
    WCHAR Path[MAX_PATH];
    NTSTATUS Status;
    PVOID Base;
    LONG Unbound;

    InitializeImage(&File, TRUE, 0, EntryReturnsTrue);
    if (!WriteTempImage(Path, &File))
    {
        skip("Could not write the mixed-mode image (%lu)\n", GetLastError());
        return;
    }

    Status = LoadImageFile(Path, &Base);
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status))
    {
        ok(GetModuleHandleW(Path) == Base, "mixed-mode image not found by GetModuleHandle\n");

        Unbound = CountUnboundImports(Base);
        ok(Unbound == 0, "mixed-mode image: %ld unbound imports\n", Unbound);

        UnloadAndCheck("mixed-mode image", Path, Base);
    }

    /* An image left mapped keeps its file open */
    ok(DeleteFileW(Path), "DeleteFileW failed with %lu\n", GetLastError());
}

/*
 * The entry point of a native image is called with DLL_PROCESS_ATTACH, and the load fails if it
 * returns FALSE. A mixed-mode image is a native image, so its own entry point has to be called
 * as well, and not one that the CLR substitutes.
 */
static
VOID
TestNativeEntryPoint(
    _In_ PCSTR Name,
    _In_ BOOLEAN HasCor)
{
#ifdef HAVE_ENTRY_CODE
    TEST_FILE File;
    WCHAR Path[MAX_PATH];
    NTSTATUS Status;
    PVOID Base;

    InitializeImage(&File, HasCor, 0, EntryReturnsFalse);
    if (!WriteTempImage(Path, &File))
    {
        skip("%s: could not write the test image (%lu)\n", Name, GetLastError());
        return;
    }

    Status = LoadImageFile(Path, &Base);
    ok_ntstatus(Status, STATUS_DLL_INIT_FAILED);
    if (NT_SUCCESS(Status))
        LdrUnloadDll(Base);

    ok(DeleteFileW(Path), "%s: DeleteFileW failed with %lu\n", Name, GetLastError());
#else
    UNREFERENCED_PARAMETER(HasCor);
    skip("%s: no entry point code for this architecture\n", Name);
#endif
}

/*
 * An IL-only image has no native code. The CLR provides its entry point, so the native entry
 * point that the image has is not called, and the image loads and unloads. That needs mscoree.
 */
static
VOID
TestILOnlyImage(VOID)
{
    TEST_FILE File;
    WCHAR Path[MAX_PATH];
    NTSTATUS Status;
    PVOID Base;

    if (!IsMscoreeInstalled())
    {
        skip("mscoree.dll is not installed\n");
        return;
    }

    /* An entry point that would fail the load, if the loader called it */
    InitializeImage(&File, TRUE, COMIMAGE_FLAGS_ILONLY, EntryReturnsFalse);
    if (!WriteTempImage(Path, &File))
    {
        skip("Could not write the IL-only image (%lu)\n", GetLastError());
        return;
    }

    Status = LoadImageFile(Path, &Base);
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status))
    {
        ok(GetModuleHandleW(Path) == Base, "IL-only image not found by GetModuleHandle\n");
        UnloadAndCheck("IL-only image", Path, Base);
    }

    ok(DeleteFileW(Path), "DeleteFileW failed with %lu\n", GetLastError());
}

START_TEST(LdrCorImage)
{
    ULONG i;

    TestControlImage();

    for (i = 0; i < _countof(RejectedImages); i++)
        TestRejectedImage(RejectedImages[i].Name, RejectedImages[i].Break);

    TestMixedModeImage();
    TestNativeEntryPoint("control image", FALSE);
    TestNativeEntryPoint("mixed-mode image", TRUE);
    TestILOnlyImage();
}
