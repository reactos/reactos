/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/io/iomgr/ramdisk.c
 * PURPOSE:         Allows booting from RAM disk
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#include <initguid.h>
#include <ntddrdsk.h>
#define NDEBUG
#include <debug.h>

/* DATA ***********************************************************************/

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, IopStartRamdisk)
#endif

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
IopStartRamdisk(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    NTSTATUS Status;
    PCHAR CommandLine, Offset, OffsetValue, Length, LengthValue;
    HANDLE DriverHandle;
    RAMDISK_CREATE_INPUT RamdiskCreate;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING GuidString, SymbolicLinkName, ObjectName, DeviceString;
    PLIST_ENTRY ListHead, NextEntry;
    OBJECT_ATTRIBUTES ObjectAttributes;
    WCHAR SourceString[54];
    
    //
    // Scan memory descriptors
    //
    MemoryDescriptor = NULL;
    ListHead = &LoaderBlock->MemoryDescriptorListHead;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        //
        // Get the descriptor
        //
        MemoryDescriptor = CONTAINING_RECORD(NextEntry,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);
        
        //
        // Needs to be a ROM/RAM descriptor
        //
        if (MemoryDescriptor->MemoryType == LoaderXIPRom) break;

        //
        // Keep trying
        //
        NextEntry = NextEntry->Flink;
    }
    
    //
    // Nothing found?
    //
    if (NextEntry == ListHead)
    {
        //
        // Bugcheck -- no data
        //
        KeBugCheckEx(RAMDISK_BOOT_INITIALIZATION_FAILED,
                     RD_NO_XIPROM_DESCRIPTOR,
                     STATUS_INVALID_PARAMETER,
                     0,
                     0);
    }
    
    //
    // Setup the input buffer
    //
    RtlZeroMemory(&RamdiskCreate, sizeof(RamdiskCreate));
    RamdiskCreate.Version = sizeof(RamdiskCreate);
    RamdiskCreate.DiskType = RAMDISK_BOOT_DISK;
    RamdiskCreate.BasePage = MemoryDescriptor->BasePage;
    RamdiskCreate.DiskOffset = 0;
    RamdiskCreate.DiskLength.QuadPart = MemoryDescriptor->PageCount << PAGE_SHIFT;
    RamdiskCreate.DiskGuid = RAMDISK_BOOTDISK_GUID;
    RamdiskCreate.DriveLetter = L'C';
    RamdiskCreate.Options.Fixed = TRUE;
    
    //
    // Check for commandline parameters
    //
    CommandLine = LoaderBlock->LoadOptions;
    if (CommandLine)
    {
        //
        // Make everything upper case
        //
        _strupr(CommandLine);
        
        //
        // Check for offset parameter
        //
        Offset = strstr(CommandLine, "RDIMAGEOFFSET");
        if (Offset)
        {
            //
            // Get to the actual value
            //
            OffsetValue = strstr(Offset, "=");
            if (OffsetValue)
            {
                //
                // Set the offset
                //
                RamdiskCreate.DiskOffset = atol(OffsetValue + 1);
            }
        }
        
        //
        // Reduce the disk length
        //
        RamdiskCreate.DiskLength.QuadPart -= RamdiskCreate.DiskOffset;
        
        //
        // Check for length parameter
        //
        Length = strstr(CommandLine, "RDIMAGELENGTH");
        if (Length)
        {
            //
            // Get to the actual value
            //
            LengthValue = strstr(Length, "=");
            if (LengthValue)
            {
                //
                // Set the offset
                //
                RamdiskCreate.DiskLength.QuadPart = _atoi64(LengthValue + 1);
            }
        }
    }
    
    //
    // Setup object attributes
    //
    RtlInitUnicodeString(&ObjectName, L"\\Device\\Ramdisk");
    InitializeObjectAttributes(&ObjectAttributes,
                               &ObjectName,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    
    //
    // Open a handle to the driver
    //
    Status = ZwOpenFile(&DriverHandle,
                        GENERIC_ALL,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!(NT_SUCCESS(Status)) || !(NT_SUCCESS(IoStatusBlock.Status)))
    {
        //
        // Bugcheck -- no driver
        //
        KeBugCheckEx(RAMDISK_BOOT_INITIALIZATION_FAILED,
                     RD_NO_RAMDISK_DRIVER,
                     IoStatusBlock.Status,
                     0,
                     0);
    }
    
    //
    // Send create command
    //
    Status = ZwDeviceIoControlFile(DriverHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   FSCTL_CREATE_RAM_DISK,
                                   &RamdiskCreate,
                                   sizeof(RamdiskCreate),
                                   NULL,
                                   0);
    ZwClose(DriverHandle);
    if (!(NT_SUCCESS(Status)) || !(NT_SUCCESS(IoStatusBlock.Status)))
    {
        //
        // Bugcheck -- driver failed
        //
        KeBugCheckEx(RAMDISK_BOOT_INITIALIZATION_FAILED,
                     RD_FSCTL_FAILED,
                     IoStatusBlock.Status,
                     0,
                     0);
    }
    
    //
    // Convert the GUID
    //
    Status = RtlStringFromGUID(&RamdiskCreate.DiskGuid, &GuidString);
    if (!NT_SUCCESS(Status))
    {
        //
        // Bugcheck -- GUID convert failed
        //
        KeBugCheckEx(RAMDISK_BOOT_INITIALIZATION_FAILED,
                     RD_GUID_CONVERT_FAILED,
                     Status,
                     0,
                     0);
    }
    
    //
    // Build the symbolic link name and target
    //
    _snwprintf(SourceString,
               sizeof(SourceString),
               L"\\Device\\Ramdisk%wZ",
               &GuidString);
    SymbolicLinkName.Length = 38;
    SymbolicLinkName.MaximumLength = 38 + sizeof(UNICODE_NULL);
    SymbolicLinkName.Buffer = L"\\ArcName\\ramdisk(0)";
    
    //
    // Create the symbolic link
    //
    RtlInitUnicodeString(&DeviceString, SourceString);
    Status = IoCreateSymbolicLink(&SymbolicLinkName, &DeviceString);
    RtlFreeUnicodeString(&GuidString);
    if (!NT_SUCCESS(Status))
    {
        //
        // Bugcheck -- symlink create failed
        //
        KeBugCheckEx(RAMDISK_BOOT_INITIALIZATION_FAILED,
                     RD_SYMLINK_CREATE_FAILED,
                     Status,
                     0,
                     0);
    }
    
    //
    // We made it
    //
    return STATUS_SUCCESS;
}
