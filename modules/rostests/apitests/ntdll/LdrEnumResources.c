/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for LdrEnumResources
 * PROGRAMMER:      Timo Kreuzer
 */

#include "precomp.h"

typedef struct _TEST_RESOURCES
{
    IMAGE_RESOURCE_DIRECTORY TypeDirectory;
    IMAGE_RESOURCE_DIRECTORY_ENTRY TypeEntries[2];
    IMAGE_RESOURCE_DIRECTORY Name1Directory;
    IMAGE_RESOURCE_DIRECTORY_ENTRY Name1Entries[2];
    IMAGE_RESOURCE_DIRECTORY Name2Directory;
    IMAGE_RESOURCE_DIRECTORY_ENTRY Name2Entries[2];
    IMAGE_RESOURCE_DIRECTORY Lang1Directory;
    IMAGE_RESOURCE_DIRECTORY_ENTRY Lang1Entries[2];
    IMAGE_RESOURCE_DIRECTORY Lang2Directory;
    IMAGE_RESOURCE_DIRECTORY_ENTRY Lang2Entries[2];
    IMAGE_RESOURCE_DIRECTORY Lang3Directory;
    IMAGE_RESOURCE_DIRECTORY_ENTRY Lang3Entries[2];
    IMAGE_RESOURCE_DIRECTORY Lang4Directory;
    IMAGE_RESOURCE_DIRECTORY_ENTRY Lang4Entries[2];
    IMAGE_RESOURCE_DATA_ENTRY DataEntries[8];
    IMAGE_RESOURCE_DIRECTORY_STRING Name1String;
    ULONG StringIndex;
    WCHAR StringBuffer[20];
} TEST_RESOURCES, *PTEST_RESOURCES;

typedef struct _TEST_IMAGE
{
    IMAGE_DOS_HEADER DosHeader;
    IMAGE_NT_HEADERS NtHeaders;
    IMAGE_SECTION_HEADER SectionHeaders[1];
    TEST_RESOURCES Resources;
} TEST_IMAGE, *PTEST_IMAGE;

static
VOID
InitializeResourceDirectory(
    PIMAGE_RESOURCE_DIRECTORY ResourceDirectory,
    USHORT NumberOfNamedEntries,
    USHORT NumberOfIdEntries)
{
    ResourceDirectory->Characteristics = 0;
    ResourceDirectory->TimeDateStamp = 0;
    ResourceDirectory->MajorVersion = 0;
    ResourceDirectory->MinorVersion = 0;
    ResourceDirectory->NumberOfNamedEntries = NumberOfNamedEntries;
    ResourceDirectory->NumberOfIdEntries = NumberOfIdEntries;
}

static
VOID
InitializeNamedEntry(
    PTEST_RESOURCES Resource,
    PIMAGE_RESOURCE_DIRECTORY_ENTRY DirEntry,
    PWCHAR Name,
    PVOID Data)
{
    DirEntry->Name = Resource->StringIndex * 2 + FIELD_OFFSET(TEST_RESOURCES, StringBuffer);
    DirEntry->NameIsString = 1;
    DirEntry->OffsetToData = (PUCHAR)Data - (PUCHAR)Resource;
    if (DirEntry < Resource->Lang1Entries)
        DirEntry->DataIsDirectory = 1;
    Resource->StringBuffer[Resource->StringIndex] = (USHORT)wcslen(Name);
    wcscpy(&Resource->StringBuffer[Resource->StringIndex + 1], Name);
    Resource->StringIndex += Resource->StringBuffer[Resource->StringIndex] * 2 + 1;
}

static
VOID
InitializeIdEntry(
    PTEST_RESOURCES Resource,
    PIMAGE_RESOURCE_DIRECTORY_ENTRY DirEntry,
    USHORT Id,
    PVOID Data)
{
    DirEntry->Name = Id;
    DirEntry->NameIsString = 0;
    DirEntry->OffsetToData = (PUCHAR)Data - (PUCHAR)Resource;
    if (DirEntry < Resource->Lang1Entries)
        DirEntry->DataIsDirectory = 1;
}

static
VOID
InitializeDataEntry(
    PVOID ImageBase,
    PIMAGE_RESOURCE_DATA_ENTRY DataEntry,
    PVOID Data,
    ULONG Size)
{

    DataEntry->OffsetToData = (PUCHAR)Data - (PUCHAR)ImageBase;
    DataEntry->Size = Size;
    DataEntry->CodePage = 0;
    DataEntry->Reserved = 0;
}

static
VOID
InitializeTestResource(
    PVOID ImageBase,
    PTEST_RESOURCES Resource)
{

    memset(Resource->StringBuffer, 0, sizeof(Resource->StringBuffer));
    Resource->StringIndex = 0;

    InitializeResourceDirectory(&Resource->TypeDirectory, 0, 2);
    InitializeIdEntry(Resource, &Resource->TypeEntries[0], 1, &Resource->Name1Directory);
    InitializeIdEntry(Resource, &Resource->TypeEntries[1], 2, &Resource->Name2Directory);

    InitializeResourceDirectory(&Resource->Name1Directory, 1, 1);
    InitializeNamedEntry(Resource, &Resource->Name1Entries[0], L"TEST", &Resource->Lang1Directory);
    InitializeIdEntry(Resource, &Resource->Name1Entries[1], 7, &Resource->Lang2Directory);

    InitializeResourceDirectory(&Resource->Name2Directory, 2, 0);
    InitializeNamedEntry(Resource, &Resource->Name2Entries[0], L"LOL", &Resource->Lang3Directory);
    InitializeNamedEntry(Resource, &Resource->Name2Entries[1], L"xD", &Resource->Lang4Directory);

    InitializeResourceDirectory(&Resource->Lang1Directory, 0, 2);
    InitializeIdEntry(Resource, &Resource->Lang1Entries[0], 0x409, &Resource->DataEntries[0]);
    InitializeIdEntry(Resource, &Resource->Lang1Entries[1], 0x407, &Resource->DataEntries[1]);

    InitializeResourceDirectory(&Resource->Lang2Directory, 0, 2);
    InitializeIdEntry(Resource, &Resource->Lang2Entries[0], 0x408, &Resource->DataEntries[2]);
    InitializeIdEntry(Resource, &Resource->Lang2Entries[1], 0x406, &Resource->DataEntries[3]);

    InitializeResourceDirectory(&Resource->Lang3Directory, 0, 2);
    InitializeIdEntry(Resource, &Resource->Lang3Entries[0], 0x405, &Resource->DataEntries[4]);
    InitializeIdEntry(Resource, &Resource->Lang3Entries[1], 0x403, &Resource->DataEntries[5]);

    InitializeResourceDirectory(&Resource->Lang4Directory, 0, 2);
    InitializeIdEntry(Resource, &Resource->Lang4Entries[0], 0x402, &Resource->DataEntries[6]);
    InitializeIdEntry(Resource, &Resource->Lang4Entries[1], 0x404, &Resource->DataEntries[7]);

    InitializeDataEntry(ImageBase, &Resource->DataEntries[0], Resource->StringBuffer + 0, 2);
    InitializeDataEntry(ImageBase, &Resource->DataEntries[1], Resource->StringBuffer + 2, 4);
    InitializeDataEntry(ImageBase, &Resource->DataEntries[2], Resource->StringBuffer + 4, 6);
    InitializeDataEntry(ImageBase, &Resource->DataEntries[3], Resource->StringBuffer + 6, 8);
    InitializeDataEntry(ImageBase, &Resource->DataEntries[4], Resource->StringBuffer + 8, 10);
    InitializeDataEntry(ImageBase, &Resource->DataEntries[5], Resource->StringBuffer + 10, 12);
    InitializeDataEntry(ImageBase, &Resource->DataEntries[6], Resource->StringBuffer + 12, 14);
    InitializeDataEntry(ImageBase, &Resource->DataEntries[7], Resource->StringBuffer + 14, 16);

}

VOID
InitializeTestImage(
    PTEST_IMAGE TestImage)
{
    PIMAGE_DATA_DIRECTORY ResourceDirectory;

    TestImage->DosHeader.e_magic = IMAGE_DOS_SIGNATURE;
    TestImage->DosHeader.e_lfanew = sizeof(IMAGE_DOS_HEADER);

    TestImage->NtHeaders.Signature = IMAGE_NT_SIGNATURE;

    TestImage->NtHeaders.FileHeader.Machine = IMAGE_FILE_MACHINE_I386;
    TestImage->NtHeaders.FileHeader.NumberOfSections = 1;
    TestImage->NtHeaders.FileHeader.TimeDateStamp = 0;
    TestImage->NtHeaders.FileHeader.PointerToSymbolTable = 0;
    TestImage->NtHeaders.FileHeader.NumberOfSymbols = 0;
    TestImage->NtHeaders.FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER);
    TestImage->NtHeaders.FileHeader.Characteristics = 0;

    TestImage->NtHeaders.OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    TestImage->NtHeaders.OptionalHeader.ImageBase = (DWORD_PTR)TestImage;
    TestImage->NtHeaders.OptionalHeader.SizeOfImage = sizeof(TEST_IMAGE);
    TestImage->NtHeaders.OptionalHeader.SizeOfHeaders = sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_NT_HEADERS);

    ResourceDirectory = &TestImage->NtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE];
    ResourceDirectory->VirtualAddress = FIELD_OFFSET(TEST_IMAGE, Resources);
    ResourceDirectory->Size = sizeof(TEST_RESOURCES);

    strcpy((char*)TestImage->SectionHeaders[0].Name, ".rsrc");
    TestImage->SectionHeaders[0].Misc.VirtualSize = sizeof(TEST_IMAGE);
    TestImage->SectionHeaders[0].VirtualAddress = FIELD_OFFSET(TEST_IMAGE, Resources);
    TestImage->SectionHeaders[0].SizeOfRawData = sizeof(TEST_IMAGE);
    TestImage->SectionHeaders[0].PointerToRawData = FIELD_OFFSET(TEST_IMAGE, Resources);
    TestImage->SectionHeaders[0].PointerToRelocations = 0;
    TestImage->SectionHeaders[0].PointerToLinenumbers = 0;
    TestImage->SectionHeaders[0].NumberOfRelocations = 0;
    TestImage->SectionHeaders[0].NumberOfLinenumbers = 0;
    TestImage->SectionHeaders[0].Characteristics = 0;

    InitializeTestResource(TestImage, &TestImage->Resources);
}

#define ok_nwstr(str1, str2, count) \
    ok(wcsncmp((PWCHAR)str1, (PWCHAR)str2, count) == 0, \
       "string is wrong, expected: '%S', got '%S'\n", str1, str2); \

#define ok_enumres(_Res, _Type, _Name, _Lang, _Data, _Size) \
    ok_dec((_Res)->Type, _Type); \
    if ((ULONG_PTR)(_Name) > 0xFFFF) \
    { \
        ok_dec(*(WORD*)((_Res)->Name), wcslen((PWCHAR)(_Name))); \
        ok_nwstr((PWCHAR)((_Res)->Name + 2), (PWCHAR)_Name, *(WORD*)((_Res)->Name)); \
    } \
    else \
    { \
        ok_dec((_Res)->Name, (ULONG_PTR)_Name); \
    } \
    ok_hex((_Res)->Language, _Lang); \
    ok_ptr((PVOID)(_Res)->Data, _Data); \
    ok_dec((_Res)->Size, _Size); \
    ok_dec((_Res)->Reserved, 0);

static
void
Test_Data(PTEST_IMAGE TestImage)
{
    LDR_RESOURCE_INFO ResourceInfo;
    LDR_ENUM_RESOURCE_INFO EnumRes[8];
    ULONG ResourceCount;
    NTSTATUS Status;

    InitializeTestImage(TestImage);

    memset(EnumRes, 0xcc, sizeof(EnumRes));

    ResourceInfo.Type = 1;
    ResourceInfo.Name = 1;
    ResourceInfo.Language = 0x408;
    ResourceCount = 8;
    Status = LdrEnumResources(TestImage, &ResourceInfo, 0, &ResourceCount, EnumRes);
    ok_hex(Status, STATUS_SUCCESS);
    ok_dec(ResourceCount, 8);

    ok_ptr((PVOID)EnumRes[0].Data, TestImage->Resources.StringBuffer);
    ok_dec(EnumRes[0].Size, 2);

    ok_enumres(&EnumRes[0], 1, L"TEST", 0x409, TestImage->Resources.StringBuffer, 2)
    ok_enumres(&EnumRes[1], 1, L"TEST", 0x407, TestImage->Resources.StringBuffer + 2, 4)
    ok_enumres(&EnumRes[2], 1, 7, 0x408, TestImage->Resources.StringBuffer + 4, 6)
    ok_enumres(&EnumRes[3], 1, 7, 0x406, TestImage->Resources.StringBuffer + 6, 8)
    ok_enumres(&EnumRes[4], 2, L"LOL", 0x405, TestImage->Resources.StringBuffer + 8, 10)
    ok_enumres(&EnumRes[5], 2, L"LOL", 0x403, TestImage->Resources.StringBuffer + 10, 12)
    ok_enumres(&EnumRes[6], 2, L"xD", 0x402, TestImage->Resources.StringBuffer + 12, 14)
    ok_enumres(&EnumRes[7], 2, L"xD", 0x404, TestImage->Resources.StringBuffer + 14, 16)

    Status = LdrEnumResources(TestImage, &ResourceInfo, 1, &ResourceCount, EnumRes);
    ok_hex(Status, STATUS_SUCCESS);
    ok_dec(ResourceCount, 4);

}


static
void
Test_Parameters(PTEST_IMAGE TestImage)
{
    LDR_RESOURCE_INFO ResourceInfo;
    LDR_ENUM_RESOURCE_INFO Resources[8];
    ULONG ResourceCount;
    NTSTATUS Status;

    InitializeTestImage(TestImage);

    ResourceInfo.Type = 6;
    ResourceInfo.Name = 1;
    ResourceInfo.Language = 0x409;

    ResourceCount = 8;
    Status = LdrEnumResources(TestImage, &ResourceInfo, 3, &ResourceCount, Resources);
    ok_hex(Status, STATUS_SUCCESS);
    ok_dec(ResourceCount, 0);

    ResourceInfo.Type = 1;
    ResourceInfo.Name = 7;
    ResourceInfo.Language = 0x406;
    ResourceCount = 8;
    Status = LdrEnumResources(TestImage, &ResourceInfo, 0, &ResourceCount, Resources);
    ok_hex(Status,  STATUS_SUCCESS);
    ok_dec(ResourceCount, 8);

    ResourceCount = 8;
    Status = LdrEnumResources(TestImage, &ResourceInfo, 1, &ResourceCount, Resources);
    ok_hex(Status, STATUS_SUCCESS);
    ok_dec(ResourceCount, 4);

    ResourceCount = 8;
    Status = LdrEnumResources(TestImage, &ResourceInfo, 2, &ResourceCount, Resources);
    ok_hex(Status, STATUS_SUCCESS);
    ok_dec(ResourceCount, 2);

    ResourceCount = 8;
    Status = LdrEnumResources(TestImage, &ResourceInfo, 3, &ResourceCount, Resources);
    ok_hex(Status, STATUS_SUCCESS);
    ok_dec(ResourceCount, 1);

    ResourceCount = 8;
    Status = LdrEnumResources(TestImage, &ResourceInfo, 99, &ResourceCount, Resources);
    ok_hex(Status, STATUS_SUCCESS);
    ok_dec(ResourceCount, 1);

    ResourceCount = 0;
    Status = LdrEnumResources(TestImage, &ResourceInfo, 0, &ResourceCount, Resources);
    ok_hex(Status,  STATUS_INFO_LENGTH_MISMATCH);
    ok_dec(ResourceCount, 8);

    ResourceCount = 7;
    Status = LdrEnumResources(TestImage, &ResourceInfo, 0, &ResourceCount, Resources);
    ok_hex(Status,  STATUS_INFO_LENGTH_MISMATCH);
    ok_dec(ResourceCount, 8);

    ResourceCount = 8;
    Status = LdrEnumResources(NULL, &ResourceInfo, 1, &ResourceCount, Resources);
    ok_hex(Status, STATUS_RESOURCE_DATA_NOT_FOUND);
    ok_dec(ResourceCount, 0);

    ResourceCount = 8;
    Status = LdrEnumResources(TestImage, &ResourceInfo, 1, &ResourceCount, NULL);
    ok_hex(Status, STATUS_INFO_LENGTH_MISMATCH);
    ok_dec(ResourceCount, 4);

    ResourceCount = 8;
    _SEH2_TRY
    {
        Status = LdrEnumResources(NULL, NULL, 0, NULL, NULL);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = 0xDeadC0de;
    }
    _SEH2_END
    ok_hex(Status, 0xDeadC0de);

    ResourceCount = 42;
    _SEH2_TRY
    {
        Status = LdrEnumResources(TestImage, &ResourceInfo, 1, NULL, Resources);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = 0xDeadC0de;
    }
    _SEH2_END
    ok_hex(Status, 0xDeadC0de);
    ok_dec(ResourceCount, 42);

    ResourceCount = 8;
    Status = LdrEnumResources(TestImage + 2, &ResourceInfo, 1, &ResourceCount, NULL);
    ok_hex(Status,  STATUS_RESOURCE_DATA_NOT_FOUND);
    ok_dec(ResourceCount, 0);

    TestImage->Resources.TypeEntries[0].DataIsDirectory = 0;
    Status = LdrEnumResources(TestImage, &ResourceInfo, 1, &ResourceCount, NULL);
    ok_hex(Status,  STATUS_INVALID_IMAGE_FORMAT);
    ok_dec(ResourceCount, 0);

}

START_TEST(LdrEnumResources)
{
    TEST_IMAGE TestImage;

    Test_Parameters(&TestImage);
    Test_Data(&TestImage);

}
