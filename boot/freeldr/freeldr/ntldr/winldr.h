/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Windows-compatible NT OS Loader.
 * COPYRIGHT:   Copyright 2006-2019 Aleksey Bragin <aleksey@reactos.org>
 */

#pragma once

#include <arc/setupblk.h>

/* Entry-point to kernel */
typedef VOID (NTAPI *KERNEL_ENTRY_POINT) (PLOADER_PARAMETER_BLOCK LoaderBlock);

/* Descriptors */
#define NUM_GDT 128     // Must be 128
#define NUM_IDT 0x100   // Only 16 are used though. Must be 0x100

#if 0

#include <pshpack1.h>
typedef struct  /* Root System Descriptor Pointer */
{
    CHAR             signature [8];          /* contains "RSD PTR " */
    UCHAR            checksum;               /* to make sum of struct == 0 */
    CHAR             oem_id [6];             /* OEM identification */
    UCHAR            revision;               /* Must be 0 for 1.0, 2 for 2.0 */
    ULONG            rsdt_physical_address;  /* 32-bit physical address of RSDT */
    ULONG            length;                 /* XSDT Length in bytes including hdr */
    ULONGLONG        xsdt_physical_address;  /* 64-bit physical address of XSDT */
    UCHAR            extended_checksum;      /* Checksum of entire table */
    CHAR             reserved [3];           /* reserved field must be 0 */
} RSDP_DESCRIPTOR, *PRSDP_DESCRIPTOR;
#include <poppack.h>

typedef struct _ARC_DISK_SIGNATURE_EX
{
    ARC_DISK_SIGNATURE DiskSignature;
    CHAR ArcName[MAX_PATH];
} ARC_DISK_SIGNATURE_EX, *PARC_DISK_SIGNATURE_EX;

#endif

#define MAX_OPTIONS_LENGTH 255

typedef struct _LOADER_SYSTEM_BLOCK
{
    LOADER_PARAMETER_BLOCK LoaderBlock;
    LOADER_PARAMETER_EXTENSION Extension;
    SETUP_LOADER_BLOCK SetupBlock;
#ifdef _M_IX86
    HEADLESS_LOADER_BLOCK HeadlessLoaderBlock;
#endif
    NLS_DATA_BLOCK NlsDataBlock;
    CHAR LoadOptions[MAX_OPTIONS_LENGTH+1];
    CHAR ArcBootDeviceName[MAX_PATH+1];
    // CHAR ArcHalDeviceName[MAX_PATH];
    CHAR NtBootPathName[MAX_PATH+1];
    CHAR NtHalPathName[MAX_PATH+1];
    ARC_DISK_INFORMATION ArcDiskInformation;
} LOADER_SYSTEM_BLOCK, *PLOADER_SYSTEM_BLOCK;

extern PLOADER_SYSTEM_BLOCK WinLdrSystemBlock;
/**/extern PCWSTR BootFileSystem;/**/


// conversion.c
#if 0
PVOID VaToPa(PVOID Va);
PVOID PaToVa(PVOID Pa);
VOID List_PaToVa(_In_ LIST_ENTRY *ListEntry);
#endif
VOID ConvertConfigToVA(PCONFIGURATION_COMPONENT_DATA Start);

// winldr.c
extern BOOLEAN SosEnabled;
#ifdef _M_IX86
extern BOOLEAN PaeModeOn;
#endif

FORCEINLINE
VOID
UiResetForSOS(VOID)
{
#ifdef _M_ARM
    /* Re-initialize the UI */
    UiInitialize(TRUE);
#else
    /* Reset the UI and switch to MiniTui */
    UiVtbl.UnInitialize();
    UiVtbl = MiniTuiVtbl;
    UiVtbl.Initialize();
#endif
    /* Disable the progress bar */
    UiProgressBar.Show = FALSE;
}

VOID
NtLdrOutputLoadMsg(
    _In_ PCSTR FileName,
    _In_opt_ PCSTR Description);

PVOID WinLdrLoadModule(PCSTR ModuleName, PULONG Size,
                       TYPE_OF_MEMORY MemoryType);

// wlmemory.c
BOOLEAN
WinLdrSetupMemoryLayout(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock);

// wlregistry.c
BOOLEAN
WinLdrInitSystemHive(
    IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PCSTR SystemRoot,
    IN BOOLEAN Setup);

BOOLEAN WinLdrScanSystemHive(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                             IN PCSTR SystemRoot);

BOOLEAN
WinLdrLoadNLSData(
    _Inout_ PLOADER_PARAMETER_BLOCK LoaderBlock,
    _In_ PCSTR DirectoryPath,
    _In_ PCUNICODE_STRING AnsiFileName,
    _In_ PCUNICODE_STRING OemFileName,
    _In_ PCUNICODE_STRING LangFileName, // CaseTable
    _In_ PCUNICODE_STRING OemHalFileName);

BOOLEAN
WinLdrAddDriverToList(
    _Inout_ PLIST_ENTRY DriverListHead,
    _In_ BOOLEAN InsertAtHead,
    _In_ PCWSTR DriverName,
    _In_opt_ PCWSTR ImagePath,
    _In_opt_ PCWSTR GroupName,
    _In_ ULONG ErrorControl,
    _In_ ULONG Tag);

// winldr.c
VOID
WinLdrInitializePhase1(PLOADER_PARAMETER_BLOCK LoaderBlock,
                       PCSTR Options,
                       PCSTR SystemPath,
                       PCSTR BootPath,
                       USHORT VersionToBoot);

VOID
WinLdrpDumpMemoryDescriptors(PLOADER_PARAMETER_BLOCK LoaderBlock);

VOID
WinLdrpDumpBootDriver(PLOADER_PARAMETER_BLOCK LoaderBlock);

VOID
WinLdrpDumpArcDisks(PLOADER_PARAMETER_BLOCK LoaderBlock);

ARC_STATUS
LoadAndBootWindowsCommon(
    IN USHORT OperatingSystemVersion,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PCSTR BootOptions,
    IN PCSTR BootPath);

VOID
WinLdrSetupMachineDependent(PLOADER_PARAMETER_BLOCK LoaderBlock);

VOID
WinLdrSetProcessorContext(VOID);

// arch/xxx/winldr.c
BOOLEAN
MempSetupPaging(IN PFN_NUMBER StartPage,
                IN PFN_NUMBER NumberOfPages,
                IN BOOLEAN KernelMapping);

VOID
MempUnmapPage(PFN_NUMBER Page);

VOID
MempDump(VOID);
