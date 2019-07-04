/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/bug.c
 * PURPOSE:         Bugcheck Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, KiInitializeBugCheck)
#endif

/* GLOBALS *******************************************************************/

LIST_ENTRY KeBugcheckCallbackListHead;
LIST_ENTRY KeBugcheckReasonCallbackListHead;
KSPIN_LOCK BugCheckCallbackLock;
ULONG KeBugCheckActive, KeBugCheckOwner;
LONG KeBugCheckOwnerRecursionCount;
PMESSAGE_RESOURCE_DATA KiBugCodeMessages;
ULONG KeBugCheckCount = 1;
ULONG KiHardwareTrigger;
PUNICODE_STRING KiBugCheckDriver;
ULONG_PTR KiBugCheckData[5];

PKNMI_HANDLER_CALLBACK KiNmiCallbackListHead = NULL;
KSPIN_LOCK KiNmiCallbackListLock;
#define TAG_KNMI 'IMNK'

/* Bugzilla Reporting */
UNICODE_STRING KeRosProcessorName, KeRosBiosDate, KeRosBiosVersion;
UNICODE_STRING KeRosVideoBiosDate, KeRosVideoBiosVersion;

/* PRIVATE FUNCTIONS *********************************************************/

PVOID
NTAPI
KiPcToFileHeader(IN PVOID Pc,
                 OUT PLDR_DATA_TABLE_ENTRY *LdrEntry,
                 IN BOOLEAN DriversOnly,
                 OUT PBOOLEAN InKernel)
{
    ULONG i = 0;
    PVOID ImageBase, PcBase = NULL;
    PLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY ListHead, NextEntry;

    /* Check which list we should use */
    ListHead = (KeLoaderBlock) ? &KeLoaderBlock->LoadOrderListHead :
                                 &PsLoadedModuleList;

    /* Assume no */
    *InKernel = FALSE;

    /* Set list pointers and make sure it's valid */
    NextEntry = ListHead->Flink;
    if (NextEntry)
    {
        /* Start loop */
        while (NextEntry != ListHead)
        {
            /* Increase entry */
            i++;

            /* Check if this is a kernel entry and we only want drivers */
            if ((i <= 2) && (DriversOnly != FALSE))
            {
                /* Skip it */
                NextEntry = NextEntry->Flink;
                continue;
            }

            /* Get the loader entry */
            Entry = CONTAINING_RECORD(NextEntry,
                                      LDR_DATA_TABLE_ENTRY,
                                      InLoadOrderLinks);

            /* Move to the next entry */
            NextEntry = NextEntry->Flink;
            ImageBase = Entry->DllBase;

            /* Check if this is the right one */
            if (((ULONG_PTR)Pc >= (ULONG_PTR)Entry->DllBase) &&
                ((ULONG_PTR)Pc < ((ULONG_PTR)Entry->DllBase + Entry->SizeOfImage)))
            {
                /* Return this entry */
                *LdrEntry = Entry;
                PcBase = ImageBase;

                /* Check if this was a kernel or HAL entry */
                if (i <= 2) *InKernel = TRUE;
                break;
            }
        }
    }

    /* Return the base address */
    return PcBase;
}

PVOID
NTAPI
KiRosPcToUserFileHeader(IN PVOID Pc,
                        OUT PLDR_DATA_TABLE_ENTRY *LdrEntry)
{
    PVOID ImageBase, PcBase = NULL;
    PLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY ListHead, NextEntry;

    /*
     * We know this is valid because we should only be called after a
     * succesfull address from RtlWalkFrameChain for UserMode, which
     * validates everything for us.
     */
    ListHead = &KeGetCurrentThread()->
               Teb->ProcessEnvironmentBlock->Ldr->InLoadOrderModuleList;

    /* Set list pointers and make sure it's valid */
    NextEntry = ListHead->Flink;
    if (NextEntry)
    {
        /* Start loop */
        while (NextEntry != ListHead)
        {
            /* Get the loader entry */
            Entry = CONTAINING_RECORD(NextEntry,
                                      LDR_DATA_TABLE_ENTRY,
                                      InLoadOrderLinks);

            /* Move to the next entry */
            NextEntry = NextEntry->Flink;
            ImageBase = Entry->DllBase;

            /* Check if this is the right one */
            if (((ULONG_PTR)Pc >= (ULONG_PTR)Entry->DllBase) &&
                ((ULONG_PTR)Pc < ((ULONG_PTR)Entry->DllBase + Entry->SizeOfImage)))
            {
                /* Return this entry */
                *LdrEntry = Entry;
                PcBase = ImageBase;
                break;
            }
        }
    }

    /* Return the base address */
    return PcBase;
}

USHORT
NTAPI
KeRosCaptureUserStackBackTrace(IN ULONG FramesToSkip,
                               IN ULONG FramesToCapture,
                               OUT PVOID *BackTrace,
                               OUT PULONG BackTraceHash OPTIONAL)
{
    PVOID Frames[2 * 64];
    ULONG FrameCount;
    ULONG Hash = 0, i;

    /* Skip a frame for the caller */
    FramesToSkip++;

    /* Don't go past the limit */
    if ((FramesToCapture + FramesToSkip) >= 128) return 0;

    /* Do the back trace */
    FrameCount = RtlWalkFrameChain(Frames, FramesToCapture + FramesToSkip, 1);

    /* Make sure we're not skipping all of them */
    if (FrameCount <= FramesToSkip) return 0;

    /* Loop all the frames */
    for (i = 0; i < FramesToCapture; i++)
    {
        /* Don't go past the limit */
        if ((FramesToSkip + i) >= FrameCount) break;

        /* Save this entry and hash it */
        BackTrace[i] = Frames[FramesToSkip + i];
        Hash += PtrToUlong(BackTrace[i]);
    }

    /* Write the hash */
    if (BackTraceHash) *BackTraceHash = Hash;

    /* Clear the other entries and return count */
    RtlFillMemoryUlong(Frames, 128, 0);
    return (USHORT)i;
}


VOID
FASTCALL
KeRosDumpStackFrameArray(IN PULONG_PTR Frames,
                         IN ULONG FrameCount)
{
    ULONG i;
    ULONG_PTR Addr;
    BOOLEAN InSystem;
    PVOID p;

    /* GCC complaints that it may be used uninitialized */
    PLDR_DATA_TABLE_ENTRY LdrEntry = NULL;

    /* Loop them */
    for (i = 0; i < FrameCount; i++)
    {
        /* Get the EIP */
        Addr = Frames[i];
        if (!Addr)
        {
            break;
        }

        /* Get the base for this file */
        if (Addr > (ULONG_PTR)MmHighestUserAddress)
        {
            /* We are in kernel */
            p = KiPcToFileHeader((PVOID)Addr, &LdrEntry, FALSE, &InSystem);
        }
        else
        {
            /* We are in user land */
            p = KiRosPcToUserFileHeader((PVOID)Addr, &LdrEntry);
        }
        if (p)
        {
#ifdef KDBG
            if (!KdbSymPrintAddress((PVOID)Addr, NULL))
#endif
            {
                CHAR AnsiName[64];

                /* Convert module name to ANSI and print it */
                KeBugCheckUnicodeToAnsi(&LdrEntry->BaseDllName,
                                        AnsiName,
                                        sizeof(AnsiName));
                Addr -= (ULONG_PTR)LdrEntry->DllBase;
                DbgPrint("<%s: %p>", AnsiName, (PVOID)Addr);
            }
        }
        else
        {
            /* Print only the address */
            DbgPrint("<%p>", (PVOID)Addr);
        }

        /* Go to the next frame */
        DbgPrint("\n");
    }
}

VOID
NTAPI
KeRosDumpStackFrames(IN PULONG_PTR Frame OPTIONAL,
                     IN ULONG FrameCount OPTIONAL)
{
    ULONG_PTR Frames[32];
    ULONG RealFrameCount;

    /* If the caller didn't ask, assume 32 frames */
    if (!FrameCount || FrameCount > 32) FrameCount = 32;

    if (Frame)
    {
        /* Dump them */
        KeRosDumpStackFrameArray(Frame, FrameCount);
    }
    else
    {
        /* Get the current frames (skip the two. One for the dumper, one for the caller) */
        RealFrameCount = RtlCaptureStackBackTrace(2, FrameCount, (PVOID*)Frames, NULL);
        DPRINT1("RealFrameCount =%lu\n", RealFrameCount);

        /* Dump them */
        KeRosDumpStackFrameArray(Frames, RealFrameCount);

        /* Count left for user mode? */
        if (FrameCount - RealFrameCount > 0)
        {
            /* Get the current frames */
            RealFrameCount = KeRosCaptureUserStackBackTrace(-1, FrameCount - RealFrameCount, (PVOID*)Frames, NULL);

            /* Dump them */
            KeRosDumpStackFrameArray(Frames, RealFrameCount);
        }
    }
}

INIT_FUNCTION
VOID
NTAPI
KiInitializeBugCheck(VOID)
{
    PMESSAGE_RESOURCE_DATA BugCheckData;
    LDR_RESOURCE_INFO ResourceInfo;
    PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry;
    NTSTATUS Status;
    PLDR_DATA_TABLE_ENTRY LdrEntry;

    /* Get the kernel entry */
    LdrEntry = CONTAINING_RECORD(KeLoaderBlock->LoadOrderListHead.Flink,
                                 LDR_DATA_TABLE_ENTRY,
                                 InLoadOrderLinks);

    /* Cache the Bugcheck Message Strings. Prepare the Lookup Data */
    ResourceInfo.Type = 11;
    ResourceInfo.Name = 1;
    ResourceInfo.Language = 9;

    /* Do the lookup. */
    Status = LdrFindResource_U(LdrEntry->DllBase,
                               &ResourceInfo,
                               RESOURCE_DATA_LEVEL,
                               &ResourceDataEntry);

    /* Make sure it worked */
    if (NT_SUCCESS(Status))
    {
        /* Now actually get a pointer to it */
        Status = LdrAccessResource(LdrEntry->DllBase,
                                   ResourceDataEntry,
                                   (PVOID*)&BugCheckData,
                                   NULL);
        if (NT_SUCCESS(Status)) KiBugCodeMessages = BugCheckData;
    }
}

PCHAR
NTAPI
KeGetBugMessageName(IN ULONG MessageId)
{
    /* Generated with https://github.com/feel-the-dz3n/kebughtoc */

    switch (MessageId)
    {
        case        WINDOWS_NT_BANNER: // 0x4000007e
            return "WINDOWS NT BANNER";
        case        WINDOWS_NT_CSD_STRING: // 0x40000087
            return "WINDOWS NT CSD STRING";
        case        WINDOWS_NT_INFO_STRING: // 0x40000088
            return "WINDOWS NT INFO STRING";
        case        WINDOWS_NT_MP_STRING: // 0x40000089
            return "WINDOWS NT MP STRING";
        case        THREAD_TERMINATE_HELD_MUTEX: // 0x4000008a
            return "THREAD TERMINATE HELD MUTEX";
        case        WINDOWS_NT_INFO_STRING_PLURAL: // 0x4000009d
            return "WINDOWS NT INFO STRING PLURAL";
        case        REACTOS_COPYRIGHT_NOTICE: // 0x4000009f
            return "REACTOS COPYRIGHT NOTICE";
        case        BUGCHECK_MESSAGE_INTRO: // 0x8000007f
            return "BUGCHECK MESSAGE INTRO";
        case        BUGCODE_ID_DRIVER: // 0x80000080
            return "BUGCODE ID DRIVER";
        case        PSS_MESSAGE_INTRO: // 0x80000081
            return "PSS MESSAGE INTRO";
        case        BUGCODE_PSS_MESSAGE: // 0x80000082
            return "BUGCODE PSS MESSAGE";
        case        BUGCHECK_TECH_INFO: // 0x80000083
            return "BUGCHECK TECH INFO";
        case        UNDEFINED_BUG_CODE: // 0x0
            return "UNDEFINED BUG CODE";
        case        APC_INDEX_MISMATCH: // 0x1
            return "APC INDEX MISMATCH";
        case        DEVICE_QUEUE_NOT_BUSY: // 0x2
            return "DEVICE QUEUE NOT BUSY";
        case        INVALID_AFFINITY_SET: // 0x3
            return "INVALID AFFINITY SET";
        case        INVALID_DATA_ACCESS_TRAP: // 0x4
            return "INVALID DATA ACCESS TRAP";
        case        INVALID_PROCESS_ATTACH_ATTEMPT: // 0x5
            return "INVALID PROCESS ATTACH ATTEMPT";
        case        INVALID_PROCESS_DETACH_ATTEMPT: // 0x6
            return "INVALID PROCESS DETACH ATTEMPT";
        case        INVALID_SOFTWARE_INTERRUPT: // 0x7
            return "INVALID SOFTWARE INTERRUPT";
        case        IRQL_NOT_DISPATCH_LEVEL: // 0x8
            return "IRQL NOT DISPATCH LEVEL";
        case        IRQL_NOT_GREATER_OR_EQUAL: // 0x9
            return "IRQL NOT GREATER OR EQUAL";
        case        IRQL_NOT_LESS_OR_EQUAL: // 0xa
            return "IRQL NOT LESS OR EQUAL";
        case        NO_EXCEPTION_HANDLING_SUPPORT: // 0xb
            return "NO EXCEPTION HANDLING SUPPORT";
        case        MAXIMUM_WAIT_OBJECTS_EXCEEDED: // 0xc
            return "MAXIMUM WAIT OBJECTS EXCEEDED";
        case        MUTEX_LEVEL_NUMBER_VIOLATION: // 0xd
            return "MUTEX LEVEL NUMBER VIOLATION";
        case        NO_USER_MODE_CONTEXT: // 0xe
            return "NO USER MODE CONTEXT";
        case        SPIN_LOCK_ALREADY_OWNED: // 0xf
            return "SPIN LOCK ALREADY OWNED";
        case        SPIN_LOCK_NOT_OWNED: // 0x10
            return "SPIN LOCK NOT OWNED";
        case        THREAD_NOT_MUTEX_OWNER: // 0x11
            return "THREAD NOT MUTEX OWNER";
        case        TRAP_CAUSE_UNKNOWN: // 0x12
            return "TRAP CAUSE UNKNOWN";
        case        EMPTY_THREAD_REAPER_LIST: // 0x13
            return "EMPTY THREAD REAPER LIST";
        case        CREATE_DELETE_LOCK_NOT_LOCKED: // 0x14
            return "CREATE DELETE LOCK NOT LOCKED";
        case        LAST_CHANCE_CALLED_FROM_KMODE: // 0x15
            return "LAST CHANCE CALLED FROM KMODE";
        case        CID_HANDLE_CREATION: // 0x16
            return "CID HANDLE CREATION";
        case        CID_HANDLE_DELETION: // 0x17
            return "CID HANDLE DELETION";
        case        REFERENCE_BY_POINTER: // 0x18
            return "REFERENCE BY POINTER";
        case        BAD_POOL_HEADER: // 0x19
            return "BAD POOL HEADER";
        case        MEMORY_MANAGEMENT: // 0x1a
            return "MEMORY MANAGEMENT";
        case        PFN_SHARE_COUNT: // 0x1b
            return "PFN SHARE COUNT";
        case        PFN_REFERENCE_COUNT: // 0x1c
            return "PFN REFERENCE COUNT";
        case        NO_SPINLOCK_AVAILABLE: // 0x1d
            return "NO SPINLOCK AVAILABLE";
        case        KMODE_EXCEPTION_NOT_HANDLED: // 0x1e
            return "KMODE EXCEPTION NOT HANDLED";
        case        SHARED_RESOURCE_CONV_ERROR: // 0x1f
            return "SHARED RESOURCE CONV ERROR";
        case        KERNEL_APC_PENDING_DURING_EXIT: // 0x20
            return "KERNEL APC PENDING DURING EXIT";
        case        QUOTA_UNDERFLOW: // 0x21
            return "QUOTA UNDERFLOW";
        case        FILE_SYSTEM: // 0x22
            return "FILE SYSTEM";
        case        FAT_FILE_SYSTEM: // 0x23
            return "FAT FILE SYSTEM";
        case        NTFS_FILE_SYSTEM: // 0x24
            return "NTFS FILE SYSTEM";
        case        NPFS_FILE_SYSTEM: // 0x25
            return "NPFS FILE SYSTEM";
        case        CDFS_FILE_SYSTEM: // 0x26
            return "CDFS FILE SYSTEM";
        case        RDR_FILE_SYSTEM: // 0x27
            return "RDR FILE SYSTEM";
        case        CORRUPT_ACCESS_TOKEN: // 0x28
            return "CORRUPT ACCESS TOKEN";
        case        SECURITY_SYSTEM: // 0x29
            return "SECURITY SYSTEM";
        case        INCONSISTENT_IRP: // 0x2a
            return "INCONSISTENT IRP";
        case        PANIC_STACK_SWITCH: // 0x2b
            return "PANIC STACK SWITCH";
        case        PORT_DRIVER_INTERNAL: // 0x2c
            return "PORT DRIVER INTERNAL";
        case        SCSI_DISK_DRIVER_INTERNAL: // 0x2d
            return "SCSI DISK DRIVER INTERNAL";
        case        DATA_BUS_ERROR: // 0x2e
            return "DATA BUS ERROR";
        case        INSTRUCTION_BUS_ERROR: // 0x2f
            return "INSTRUCTION BUS ERROR";
        case        SET_OF_INVALID_CONTEXT: // 0x30
            return "SET OF INVALID CONTEXT";
        case        PHASE0_INITIALIZATION_FAILED: // 0x31
            return "PHASE0 INITIALIZATION FAILED";
        case        PHASE1_INITIALIZATION_FAILED: // 0x32
            return "PHASE1 INITIALIZATION FAILED";
        case        UNEXPECTED_INITIALIZATION_CALL: // 0x33
            return "UNEXPECTED INITIALIZATION CALL";
        case        CACHE_MANAGER: // 0x34
            return "CACHE MANAGER";
        case        NO_MORE_IRP_STACK_LOCATIONS: // 0x35
            return "NO MORE IRP STACK LOCATIONS";
        case        DEVICE_REFERENCE_COUNT_NOT_ZERO: // 0x36
            return "DEVICE REFERENCE COUNT NOT ZERO";
        case        FLOPPY_INTERNAL_ERROR: // 0x37
            return "FLOPPY INTERNAL ERROR";
        case        SERIAL_DRIVER_INTERNAL: // 0x38
            return "SERIAL DRIVER INTERNAL";
        case        SYSTEM_EXIT_OWNED_MUTEX: // 0x39
            return "SYSTEM EXIT OWNED MUTEX";
        case        MULTIPROCESSOR_CONFIGURATION_NOT_SUPPORTED: // 0x3e
            return "MULTIPROCESSOR CONFIGURATION NOT SUPPORTED";
        case        NO_MORE_SYSTEM_PTES: // 0x3f
            return "NO MORE SYSTEM PTES";
        case        TARGET_MDL_TOO_SMALL: // 0x40
            return "TARGET MDL TOO SMALL";
        case        MUST_SUCCEED_POOL_EMPTY: // 0x41
            return "MUST SUCCEED POOL EMPTY";
        case        ATDISK_DRIVER_INTERNAL: // 0x42
            return "ATDISK DRIVER INTERNAL";
        case        MULTIPLE_IRP_COMPLETE_REQUESTS: // 0x44
            return "MULTIPLE IRP COMPLETE REQUESTS";
        case        INSUFFICIENT_SYSTEM_MAP_REGS: // 0x45
            return "INSUFFICIENT SYSTEM MAP REGS";
        case        CANCEL_STATE_IN_COMPLETED_IRP: // 0x48
            return "CANCEL STATE IN COMPLETED IRP";
        case        PAGE_FAULT_WITH_INTERRUPTS_OFF: // 0x49
            return "PAGE FAULT WITH INTERRUPTS OFF";
        case        IRQL_GT_ZERO_AT_SYSTEM_SERVICE: // 0x4a
            return "IRQL GT ZERO AT SYSTEM SERVICE";
        case        STREAMS_INTERNAL_ERROR: // 0x4b
            return "STREAMS INTERNAL ERROR";
        case        FATAL_UNHANDLED_HARD_ERROR: // 0x4c
            return "FATAL UNHANDLED HARD ERROR";
        case        NO_PAGES_AVAILABLE: // 0x4d
            return "NO PAGES AVAILABLE";
        case        PFN_LIST_CORRUPT: // 0x4e
            return "PFN LIST CORRUPT";
        case        NDIS_INTERNAL_ERROR: // 0x4f
            return "NDIS INTERNAL ERROR";
        case        PAGE_FAULT_IN_NONPAGED_AREA: // 0x50
            return "PAGE FAULT IN NONPAGED AREA";
        case        REGISTRY_ERROR: // 0x51
            return "REGISTRY ERROR";
        case        MAILSLOT_FILE_SYSTEM: // 0x52
            return "MAILSLOT FILE SYSTEM";
        case        NO_BOOT_DEVICE: // 0x53
            return "NO BOOT DEVICE";
        case        LM_SERVER_INTERNAL_ERROR: // 0x54
            return "LM SERVER INTERNAL ERROR";
        case        DATA_COHERENCY_EXCEPTION: // 0x55
            return "DATA COHERENCY EXCEPTION";
        case        INSTRUCTION_COHERENCY_EXCEPTION: // 0x56
            return "INSTRUCTION COHERENCY EXCEPTION";
        case        XNS_INTERNAL_ERROR: // 0x57
            return "XNS INTERNAL ERROR";
        case        FTDISK_INTERNAL_ERROR: // 0x58
            return "FTDISK INTERNAL ERROR";
        case        PINBALL_FILE_SYSTEM: // 0x59
            return "PINBALL FILE SYSTEM";
        case        CRITICAL_SERVICE_FAILED: // 0x5a
            return "CRITICAL SERVICE FAILED";
        case        SET_ENV_VAR_FAILED: // 0x5b
            return "SET ENV VAR FAILED";
        case        HAL_INITIALIZATION_FAILED: // 0x5c
            return "HAL INITIALIZATION FAILED";
        case        UNSUPPORTED_PROCESSOR: // 0x5d
            return "UNSUPPORTED PROCESSOR";
        case        OBJECT_INITIALIZATION_FAILED: // 0x5e
            return "OBJECT INITIALIZATION FAILED";
        case        SECURITY_INITIALIZATION_FAILED: // 0x5f
            return "SECURITY INITIALIZATION FAILED";
        case        PROCESS_INITIALIZATION_FAILED: // 0x60
            return "PROCESS INITIALIZATION FAILED";
        case        HAL1_INITIALIZATION_FAILED: // 0x61
            return "HAL1 INITIALIZATION FAILED";
        case        OBJECT1_INITIALIZATION_FAILED: // 0x62
            return "OBJECT1 INITIALIZATION FAILED";
        case        SECURITY1_INITIALIZATION_FAILED: // 0x63
            return "SECURITY1 INITIALIZATION FAILED";
        case        SYMBOLIC_INITIALIZATION_FAILED: // 0x64
            return "SYMBOLIC INITIALIZATION FAILED";
        case        MEMORY1_INITIALIZATION_FAILED: // 0x65
            return "MEMORY1 INITIALIZATION FAILED";
        case        CACHE_INITIALIZATION_FAILED: // 0x66
            return "CACHE INITIALIZATION FAILED";
        case        CONFIG_INITIALIZATION_FAILED: // 0x67
            return "CONFIG INITIALIZATION FAILED";
        case        FILE_INITIALIZATION_FAILED: // 0x68
            return "FILE INITIALIZATION FAILED";
        case        IO1_INITIALIZATION_FAILED: // 0x69
            return "IO1 INITIALIZATION FAILED";
        case        LPC_INITIALIZATION_FAILED: // 0x6a
            return "LPC INITIALIZATION FAILED";
        case        PROCESS1_INITIALIZATION_FAILED: // 0x6b
            return "PROCESS1 INITIALIZATION FAILED";
        case        REFMON_INITIALIZATION_FAILED: // 0x6c
            return "REFMON INITIALIZATION FAILED";
        case        SESSION1_INITIALIZATION_FAILED: // 0x6d
            return "SESSION1 INITIALIZATION FAILED";
        case        SESSION2_INITIALIZATION_FAILED: // 0x6e
            return "SESSION2 INITIALIZATION FAILED";
        case        SESSION3_INITIALIZATION_FAILED: // 0x6f
            return "SESSION3 INITIALIZATION FAILED";
        case        SESSION4_INITIALIZATION_FAILED: // 0x70
            return "SESSION4 INITIALIZATION FAILED";
        case        SESSION5_INITIALIZATION_FAILED: // 0x71
            return "SESSION5 INITIALIZATION FAILED";
        case        ASSIGN_DRIVE_LETTERS_FAILED: // 0x72
            return "ASSIGN DRIVE LETTERS FAILED";
        case        CONFIG_LIST_FAILED: // 0x73
            return "CONFIG LIST FAILED";
        case        BAD_SYSTEM_CONFIG_INFO: // 0x74
            return "BAD SYSTEM CONFIG INFO";
        case        CANNOT_WRITE_CONFIGURATION: // 0x75
            return "CANNOT WRITE CONFIGURATION";
        case        PROCESS_HAS_LOCKED_PAGES: // 0x76
            return "PROCESS HAS LOCKED PAGES";
        case        KERNEL_STACK_INPAGE_ERROR: // 0x77
            return "KERNEL STACK INPAGE ERROR";
        case        PHASE0_EXCEPTION: // 0x78
            return "PHASE0 EXCEPTION";
        case        MISMATCHED_HAL: // 0x79
            return "MISMATCHED HAL";
        case        KERNEL_DATA_INPAGE_ERROR: // 0x7a
            return "KERNEL DATA INPAGE ERROR";
        case        INACCESSIBLE_BOOT_DEVICE: // 0x7b
            return "INACCESSIBLE BOOT DEVICE";
        case        BUGCODE_NDIS_DRIVER: // 0x7c
            return "BUGCODE NDIS DRIVER";
        case        INSTALL_MORE_MEMORY: // 0x7d
            return "INSTALL MORE MEMORY";
        case        SYSTEM_THREAD_EXCEPTION_NOT_HANDLED: // 0x7e
            return "SYSTEM THREAD EXCEPTION NOT HANDLED";
        case        UNEXPECTED_KERNEL_MODE_TRAP: // 0x7f
            return "UNEXPECTED KERNEL MODE TRAP";
        case        NMI_HARDWARE_FAILURE: // 0x80
            return "NMI HARDWARE FAILURE";
        case        SPIN_LOCK_INIT_FAILURE: // 0x81
            return "SPIN LOCK INIT FAILURE";
        case        KERNEL_MODE_EXCEPTION_NOT_HANDLED: // 0x8e
            return "KERNEL MODE EXCEPTION NOT HANDLED";
        case        PP0_INITIALIZATION_FAILED: // 0x8f
            return "PP0 INITIALIZATION FAILED";
        case        PP1_INITIALIZATION_FAILED: // 0x90
            return "PP1 INITIALIZATION FAILED";
        case        WIN32K_INIT_OR_RIT_FAILURE: // 0x91
            return "WIN32K INIT OR RIT FAILURE";
        case        INVALID_KERNEL_HANDLE: // 0x93
            return "INVALID KERNEL HANDLE";
        case        KERNEL_STACK_LOCKED_AT_EXIT: // 0x94
            return "KERNEL STACK LOCKED AT EXIT";
        case        INVALID_WORK_QUEUE_ITEM: // 0x96
            return "INVALID WORK QUEUE ITEM";
        case        MORAL_EXCEPTION_ERROR: // 0x9a
            return "MORAL EXCEPTION ERROR";
        case        INTERNAL_POWER_ERROR: // 0xa0
            return "INTERNAL POWER ERROR";
        case        PCI_BUS_DRIVER_INTERNAL: // 0xa1
            return "PCI BUS DRIVER INTERNAL";
        case        ACPI_BIOS_ERROR: // 0xa5
            return "ACPI BIOS ERROR";
        case        BOOTING_IN_SAFEMODE_MINIMAL: // 0x400000a8
            return "BOOTING IN SAFEMODE MINIMAL";
        case        BOOTING_IN_SAFEMODE_NETWORK: // 0x400000a9
            return "BOOTING IN SAFEMODE NETWORK";
        case        BOOTING_IN_SAFEMODE_DSREPAIR: // 0x400000aa
            return "BOOTING IN SAFEMODE DSREPAIR";
        case        HAL_MEMORY_ALLOCATION: // 0xac
            return "HAL MEMORY ALLOCATION";
        case        VIDEO_DRIVER_INIT_FAILURE: // 0xb4
            return "VIDEO DRIVER INIT FAILURE";
        case        BOOTLOG_ENABLED: // 0x400000b7
            return "BOOTLOG ENABLED";
        case        ATTEMPTED_SWITCH_FROM_DPC: // 0xb8
            return "ATTEMPTED SWITCH FROM DPC";
        case        ATTEMPTED_WRITE_TO_READONLY_MEMORY: // 0xbe
            return "ATTEMPTED WRITE TO READONLY MEMORY";
        case        SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION: // 0xc1
            return "SPECIAL POOL DETECTED MEMORY CORRUPTION";
        case        BAD_POOL_CALLER: // 0xc2
            return "BAD POOL CALLER";
        case        BUGCODE_PSS_MESSAGE_SIGNATURE: // 0xc3
            return "BUGCODE PSS MESSAGE SIGNATURE";
        case        DRIVER_VERIFIER_DETECTED_VIOLATION: // 0xc4
            return "DRIVER VERIFIER DETECTED VIOLATION";
        case        DRIVER_CORRUPTED_EXPOOL: // 0xc5
            return "DRIVER CORRUPTED EXPOOL";
        case        DRIVER_CAUGHT_MODIFYING_FREED_POOL: // 0xc6
            return "DRIVER CAUGHT MODIFYING FREED POOL";
        case        IRQL_UNEXPECTED_VALUE: // 0xc8
            return "IRQL UNEXPECTED VALUE";
        case        PNP_DETECTED_FATAL_ERROR: // 0xca
            return "PNP DETECTED FATAL ERROR";
        case        DRIVER_LEFT_LOCKED_PAGES_IN_PROCESS: // 0xcb
            return "DRIVER LEFT LOCKED PAGES IN PROCESS";
        case        PAGE_FAULT_IN_FREED_SPECIAL_POOL: // 0xcc
            return "PAGE FAULT IN FREED SPECIAL POOL";
        case        PAGE_FAULT_BEYOND_END_OF_ALLOCATION: // 0xcd
            return "PAGE FAULT BEYOND END OF ALLOCATION";
        case        DRIVER_UNLOADED_WITHOUT_CANCELLING_PENDING_OPERATIONS: // 0xce
            return "DRIVER UNLOADED WITHOUT CANCELLING PENDING OPERATIONS";
        case        DRIVER_CORRUPTED_MMPOOL: // 0xd0
            return "DRIVER CORRUPTED MMPOOL";
        case        DRIVER_IRQL_NOT_LESS_OR_EQUAL: // 0xd1
            return "DRIVER IRQL NOT LESS OR EQUAL";
        case        DRIVER_PORTION_MUST_BE_NONPAGED: // 0xd3
            return "DRIVER PORTION MUST BE NONPAGED";
        case        SYSTEM_SCAN_AT_RAISED_IRQL_CAUGHT_IMPROPER_DRIVER_UNLOAD: // 0xd4
            return "SYSTEM SCAN AT RAISED IRQL CAUGHT IMPROPER DRIVER UNLOAD";
        case        DRIVER_PAGE_FAULT_IN_FREED_SPECIAL_POOL: // 0xd5
            return "DRIVER PAGE FAULT IN FREED SPECIAL POOL";
        case        DRIVER_PAGE_FAULT_BEYOND_END_OF_ALLOCATION: // 0xd6
            return "DRIVER PAGE FAULT BEYOND END OF ALLOCATION";
        case        DRIVER_UNMAPPING_INVALID_VIEW: // 0xd7
            return "DRIVER UNMAPPING INVALID VIEW";
        case        DRIVER_USED_EXCESSIVE_PTES: // 0xd8
            return "DRIVER USED EXCESSIVE PTES";
        case        LOCKED_PAGES_TRACKER_CORRUPTION: // 0xd9
            return "LOCKED PAGES TRACKER CORRUPTION";
        case        SYSTEM_PTE_MISUSE: // 0xda
            return "SYSTEM PTE MISUSE";
        case        DRIVER_CORRUPTED_SYSPTES: // 0xdb
            return "DRIVER CORRUPTED SYSPTES";
        case        DRIVER_INVALID_STACK_ACCESS: // 0xdc
            return "DRIVER INVALID STACK ACCESS";
        case        POOL_CORRUPTION_IN_FILE_AREA: // 0xde
            return "POOL CORRUPTION IN FILE AREA";
        case        IMPERSONATING_WORKER_THREAD: // 0xdf
            return "IMPERSONATING WORKER THREAD";
        case        ACPI_BIOS_FATAL_ERROR: // 0xe0
            return "ACPI BIOS FATAL ERROR";
        case        WORKER_THREAD_RETURNED_AT_BAD_IRQL: // 0xe1
            return "WORKER THREAD RETURNED AT BAD IRQL";
        case        MANUALLY_INITIATED_CRASH: // 0xe2
            return "MANUALLY INITIATED CRASH";
        case        RESOURCE_NOT_OWNED: // 0xe3
            return "RESOURCE NOT OWNED";
        case        WORKER_INVALID: // 0xe4
            return "WORKER INVALID";
        case        POWER_FAILURE_SIMULATE: // 0xe5
            return "POWER FAILURE SIMULATE";
        case        INVALID_CANCEL_OF_FILE_OPEN: // 0xe8
            return "INVALID CANCEL OF FILE OPEN";
        case        ACTIVE_EX_WORKER_THREAD_TERMINATION: // 0xe9
            return "ACTIVE EX WORKER THREAD TERMINATION";
        case        THREAD_STUCK_IN_DEVICE_DRIVER: // 0xea
            return "THREAD STUCK IN DEVICE DRIVER";
        case        CRITICAL_PROCESS_DIED: // 0xef
            return "CRITICAL PROCESS DIED";
        case        CRITICAL_OBJECT_TERMINATION: // 0xf4
            return "CRITICAL OBJECT TERMINATION";
        case        PCI_VERIFIER_DETECTED_VIOLATION: // 0xf6
            return "PCI VERIFIER DETECTED VIOLATION";
        case        DRIVER_OVERRAN_STACK_BUFFER: // 0xf7
            return "DRIVER OVERRAN STACK BUFFER";
        case        RAMDISK_BOOT_INITIALIZATION_FAILED: // 0xf8
            return "RAMDISK BOOT INITIALIZATION FAILED";
        case        DRIVER_RETURNED_STATUS_REPARSE_FOR_VOLUME_OPEN: // 0xf9
            return "DRIVER RETURNED STATUS REPARSE FOR VOLUME OPEN";
        case        HTTP_DRIVER_CORRUPTED: // 0xfa
            return "HTTP DRIVER CORRUPTED";
        case        ATTEMPTED_EXECUTE_OF_NOEXECUTE_MEMORY: // 0xfc
            return "ATTEMPTED EXECUTE OF NOEXECUTE MEMORY";
        case        DIRTY_NOWRITE_PAGES_CONGESTION: // 0xfd
            return "DIRTY NOWRITE PAGES CONGESTION";
        case        BUGCODE_USB_DRIVER: // 0xfe
            return "BUGCODE USB DRIVER";
        case        KERNEL_SECURITY_CHECK_FAILURE: // 0x139
            return "KERNEL SECURITY CHECK FAILURE";

        default:
            return "UNKNOWN CODE NAME";
    }
}

BOOLEAN
NTAPI
KeGetBugMessageText(IN ULONG BugCheckCode,
                    OUT PANSI_STRING OutputString OPTIONAL)
{
    ULONG i;
    ULONG IdOffset;
    PMESSAGE_RESOURCE_ENTRY MessageEntry;
    PCHAR BugCode;
    USHORT Length;
    BOOLEAN Result = FALSE;

    /* Make sure we're not bugchecking too early */
    if (!KiBugCodeMessages) return Result;

    /*
     * Globally protect in SEH as we are trying to access data in
     * dire situations, and potentially going to patch it (see below).
     */
    _SEH2_TRY
    {

    /*
     * Make the kernel resource section writable, as we are going to manually
     * trim the trailing newlines in the bugcheck resource message in place,
     * when OutputString is NULL and before displaying it on screen.
     */
    MmMakeKernelResourceSectionWritable();

    /* Find the message. This code is based on RtlFindMesssage */
    for (i = 0; i < KiBugCodeMessages->NumberOfBlocks; i++)
    {
        /* Check if the ID matches */
        if ((BugCheckCode >= KiBugCodeMessages->Blocks[i].LowId) &&
            (BugCheckCode <= KiBugCodeMessages->Blocks[i].HighId))
        {
            /* Get offset to entry */
            MessageEntry = (PMESSAGE_RESOURCE_ENTRY)
                ((ULONG_PTR)KiBugCodeMessages + KiBugCodeMessages->Blocks[i].OffsetToEntries);
            IdOffset = BugCheckCode - KiBugCodeMessages->Blocks[i].LowId;

            /* Advance in the entries until finding it */
            while (IdOffset--)
            {
                MessageEntry = (PMESSAGE_RESOURCE_ENTRY)
                    ((ULONG_PTR)MessageEntry + MessageEntry->Length);
            }

            /* Make sure it's not Unicode */
            ASSERT(!(MessageEntry->Flags & MESSAGE_RESOURCE_UNICODE));

            /* Get the final code */
            BugCode = (PCHAR)MessageEntry->Text;
            Length = (USHORT)strlen(BugCode);

            /* Handle trailing newlines */
            while ((Length > 0) && ((BugCode[Length - 1] == '\n') ||
                                    (BugCode[Length - 1] == '\r') ||
                                    (BugCode[Length - 1] == ANSI_NULL)))
            {
                /* Directly trim the newline in place if we don't return the string */
                if (!OutputString) BugCode[Length - 1] = ANSI_NULL;

                /* Skip the trailing newline */
                Length--;
            }

            /* Check if caller wants an output string */
            if (OutputString)
            {
                /* Return it in the OutputString */
                OutputString->Buffer = BugCode;
                OutputString->Length = Length;
                OutputString->MaximumLength = Length;
            }
            else
            {
                /* Direct output to screen */
                InbvDisplayString(BugCode);
                InbvDisplayString("\r");
            }

            /* We're done */
            Result = TRUE;
            break;
        }
    }

    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    /* Return the result */
    return Result;
}

VOID
NTAPI
KiDoBugCheckCallbacks(VOID)
{
    PKBUGCHECK_CALLBACK_RECORD CurrentRecord;
    PLIST_ENTRY ListHead, NextEntry, LastEntry;
    ULONG_PTR Checksum;

    /* First make sure that the list is initialized... it might not be */
    ListHead = &KeBugcheckCallbackListHead;
    if ((!ListHead->Flink) || (!ListHead->Blink))
        return;

    /* Loop the list */
    LastEntry = ListHead;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the reord */
        CurrentRecord = CONTAINING_RECORD(NextEntry,
                                          KBUGCHECK_CALLBACK_RECORD,
                                          Entry);

        /* Validate it */
        // TODO/FIXME: Check whether the memory CurrentRecord points to
        // is still accessible and valid!
        if (CurrentRecord->Entry.Blink != LastEntry) return;
        Checksum = (ULONG_PTR)CurrentRecord->CallbackRoutine;
        Checksum += (ULONG_PTR)CurrentRecord->Buffer;
        Checksum += (ULONG_PTR)CurrentRecord->Length;
        Checksum += (ULONG_PTR)CurrentRecord->Component;

        /* Make sure it's inserted and validated */
        if ((CurrentRecord->State == BufferInserted) &&
            (CurrentRecord->Checksum == Checksum))
        {
            /* Call the routine */
            CurrentRecord->State = BufferStarted;
            _SEH2_TRY
            {
                (CurrentRecord->CallbackRoutine)(CurrentRecord->Buffer,
                                                 CurrentRecord->Length);
                CurrentRecord->State = BufferFinished;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                CurrentRecord->State = BufferIncomplete;
            }
            _SEH2_END;
        }

        /* Go to the next entry */
        LastEntry = NextEntry;
        NextEntry = NextEntry->Flink;
    }
}

VOID
NTAPI
KiBugCheckDebugBreak(IN ULONG StatusCode)
{
    /*
     * Wrap this in SEH so we don't crash if
     * there is no debugger or if it disconnected
     */
DoBreak:
    _SEH2_TRY
    {
        /* Breakpoint */
        DbgBreakPointWithStatus(StatusCode);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* No debugger, halt the CPU */
        HalHaltSystem();
    }
    _SEH2_END;

    /* Break again if this wasn't first try */
    if (StatusCode != DBG_STATUS_BUGCHECK_FIRST) goto DoBreak;
}

PCHAR
NTAPI
KeBugCheckUnicodeToAnsi(IN PUNICODE_STRING Unicode,
                        OUT PCHAR Ansi,
                        IN ULONG Length)
{
    PCHAR p;
    PWCHAR pw;
    ULONG i;

    /* Set length and normalize it */
    i = Unicode->Length / sizeof(WCHAR);
    i = min(i, Length - 1);

    /* Set source and destination, and copy */
    pw = Unicode->Buffer;
    p = Ansi;
    while (i--) *p++ = (CHAR)*pw++;

    /* Null terminate and return */
    *p = ANSI_NULL;
    return Ansi;
}

VOID
NTAPI
KiDumpParameterImages(IN PCHAR Message,
                      IN PULONG_PTR Parameters,
                      IN ULONG ParameterCount,
                      IN PKE_BUGCHECK_UNICODE_TO_ANSI ConversionRoutine)
{
    ULONG i;
    BOOLEAN InSystem;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PVOID ImageBase;
    PUNICODE_STRING DriverName;
    CHAR AnsiName[32];
    PIMAGE_NT_HEADERS NtHeader;
    ULONG TimeStamp;
    BOOLEAN FirstRun = TRUE;

    /* Loop parameters */
    for (i = 0; i < ParameterCount; i++)
    {
        /* Get the base for this parameter */
        ImageBase = KiPcToFileHeader((PVOID)Parameters[i],
                                     &LdrEntry,
                                     FALSE,
                                     &InSystem);
        if (!ImageBase)
        {
            /* FIXME: Add code to check for unloaded drivers */
            DPRINT1("Potentially unloaded driver!\n");
            continue;
        }
        else
        {
            /* Get the NT Headers and Timestamp */
            NtHeader = RtlImageNtHeader(LdrEntry->DllBase);
            TimeStamp = NtHeader->FileHeader.TimeDateStamp;

            /* Convert the driver name */
            DriverName = &LdrEntry->BaseDllName;
            ConversionRoutine(&LdrEntry->BaseDllName,
                              AnsiName,
                              sizeof(AnsiName));
        }

        /* Format driver name */
        sprintf(Message,
                "%s**  %12s - Address %p base at %p, DateStamp %08lx\r\n",
                FirstRun ? "\r\n*":"*",
                AnsiName,
                (PVOID)Parameters[i],
                ImageBase,
                TimeStamp);

        /* Check if we only had one parameter */
        if (ParameterCount <= 1)
        {
            /* Then just save the name */
            KiBugCheckDriver = DriverName;
        }
        else
        {
            /* Otherwise, display the message */
            InbvDisplayString(Message);
        }

        /* Loop again */
        FirstRun = FALSE;
    }
}

VOID
NTAPI
KiInitializeBlueScreen()
{
    PVOID bmpBugcheck = NULL;

    /* Check if bootvid is installed */
    if (InbvIsBootDriverInstalled())
    {
        /* Acquire ownership and reset the display */
        InbvAcquireDisplayOwnership();
        InbvResetDisplay();
    }

    /* Load bugcheck bitmap */
    bmpBugcheck = InbvGetResourceAddress(IDB_BUGCHECK);

    /* If loaded */
    if (bmpBugcheck)
    {
        /* Display it */
        InbvBitBlt(bmpBugcheck, 0, 0);
    }

    /* Display blue screen */
    InbvSolidColorFill(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, 1);

    /* Set up text printing */
    InbvSetTextColor(BV_COLOR_WHITE);
    InbvInstallDisplayStringFilter(NULL);
    InbvEnableDisplayString(TRUE);
    InbvSetScrollRegion(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
}

VOID
NTAPI
KiDisplayBlueScreen(IN ULONG MessageId,
                    IN BOOLEAN IsHardError,
                    IN PCHAR HardErrCaption OPTIONAL,
                    IN PCHAR HardErrMessage OPTIONAL,
                    IN PCHAR Message)
{
    CHAR AnsiName[75];
    ANSI_STRING GenericMsgStr;

    /* If MessageId is generic message, then bugcode is in data */
    ULONG ActualBugcode = MessageId == BUGCODE_PSS_MESSAGE
                          ? KiBugCheckData[0]
                          : MessageId;

    /* Initialize graphical part */
    KiInitializeBlueScreen();

    /* Check if this is a hard error */
    if (IsHardError)
    {
        /* Display caption and message */
        if (HardErrCaption) InbvDisplayString(HardErrCaption);
        if (HardErrMessage) InbvDisplayString(HardErrMessage);
    }

    /* Print out initial message */
    KeGetBugMessageText(BUGCHECK_MESSAGE_INTRO, NULL);
    InbvDisplayString("\r\n\r\n");

    /* Print stop code */
    InbvSetTextColor(8);
    InbvDisplayString("Stop code: ");
    InbvSetTextColor(15);
    InbvDisplayString(KeGetBugMessageName(ActualBugcode));

    /* Print additional information */
    InbvSetTextColor(8);
    InbvDisplayString(" (");
    sprintf(AnsiName, "0x%08lX", MessageId);
    InbvDisplayString(AnsiName);

    /* Check if we have a driver */
    if (KiBugCheckDriver)
    {
        /* Convert and print out driver name */
        KeBugCheckUnicodeToAnsi(KiBugCheckDriver, AnsiName, sizeof(AnsiName));
        InbvDisplayString(", ");
        InbvDisplayString(AnsiName);
    }

    /* Finish printing basic technical information */
    InbvDisplayString(")\r\n\r\n");
    InbvSetTextColor(15);

    /* Check if this is the generic message */
    if (MessageId == BUGCODE_PSS_MESSAGE)
    {
        /* It is, so get the bug code string as well */
        KeGetBugMessageText((ULONG)KiBugCheckData[0], NULL);
        InbvDisplayString("\r\n\r\n");
    }
    else
    {
        /* If message is not generic print bug code message */
        KeGetBugMessageText(MessageId, NULL);
        InbvDisplayString("\r\n\r\n");
    }

    /* Let's print generic message. Initialize GenericMsgStr */
    RtlInitAnsiString(&GenericMsgStr, "");

    /* Put generic message there */
    KeGetBugMessageText(BUGCODE_PSS_MESSAGE, &GenericMsgStr);

    /* Format it */
    sprintf(GenericMsgStr.Buffer, GenericMsgStr.Buffer, ActualBugcode);

    /* Display */
    InbvDisplayString(GenericMsgStr.Buffer);
    InbvDisplayString("\r\n\r\n");

    /* Print message for technical information */
    InbvSetTextColor(8);
    KeGetBugMessageText(BUGCHECK_TECH_INFO, NULL);

    /* Show the technical Data */
    sprintf(AnsiName,
            "\r\n\r\n*** STOP: 0x%08lX (0x%p,0x%p,0x%p,0x%p)\r\n\r\n",
            (ULONG)KiBugCheckData[0],
            (PVOID)KiBugCheckData[1],
            (PVOID)KiBugCheckData[2],
            (PVOID)KiBugCheckData[3],
            (PVOID)KiBugCheckData[4]);
    InbvDisplayString(AnsiName);

    /* Check if we have a driver*/
    if (KiBugCheckDriver)
    {
        /* Display technical driver data */
        InbvDisplayString(Message);
    }
    else
    {
        /* Dump parameter information */
        KiDumpParameterImages(Message,
                              (PVOID)&KiBugCheckData[1],
                              4,
                              KeBugCheckUnicodeToAnsi);
    }

    /* Print ReactOS version at the end */
    InbvDisplayString(NtBuildLab);
}

DECLSPEC_NORETURN
VOID
NTAPI
KeBugCheckWithTf(IN ULONG BugCheckCode,
                 IN ULONG_PTR BugCheckParameter1,
                 IN ULONG_PTR BugCheckParameter2,
                 IN ULONG_PTR BugCheckParameter3,
                 IN ULONG_PTR BugCheckParameter4,
                 IN PKTRAP_FRAME TrapFrame)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    CONTEXT Context;
    ULONG MessageId;
    CHAR AnsiName[128];
    BOOLEAN IsSystem, IsHardError = FALSE, Reboot = FALSE;
    PCHAR HardErrCaption = NULL, HardErrMessage = NULL;
    PVOID Pc = NULL, Memory;
    PVOID DriverBase;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PULONG_PTR HardErrorParameters;
    KIRQL OldIrql;
#ifdef CONFIG_SMP
    LONG i = 0;
#endif

    /* Set active bugcheck */
    KeBugCheckActive = TRUE;
    KiBugCheckDriver = NULL;

    /* Check if this is power failure simulation */
    if (BugCheckCode == POWER_FAILURE_SIMULATE)
    {
        /* Call the Callbacks and reboot */
        KiDoBugCheckCallbacks();
        HalReturnToFirmware(HalRebootRoutine);
    }

    /* Save the IRQL and set hardware trigger */
    Prcb->DebuggerSavedIRQL = KeGetCurrentIrql();
    InterlockedIncrement((PLONG)&KiHardwareTrigger);

    /* Capture the CPU Context */
    RtlCaptureContext(&Prcb->ProcessorState.ContextFrame);
    KiSaveProcessorControlState(&Prcb->ProcessorState);
    Context = Prcb->ProcessorState.ContextFrame;

    /* FIXME: Call the Watchdog if it's registered */

    /* Check which bugcode this is */
    switch (BugCheckCode)
    {
        /* These bug checks already have detailed messages, keep them */
        case UNEXPECTED_KERNEL_MODE_TRAP:
        case DRIVER_CORRUPTED_EXPOOL:
        case ACPI_BIOS_ERROR:
        case ACPI_BIOS_FATAL_ERROR:
        case THREAD_STUCK_IN_DEVICE_DRIVER:
        case DATA_BUS_ERROR:
        case FAT_FILE_SYSTEM:
        case NO_MORE_SYSTEM_PTES:
        case INACCESSIBLE_BOOT_DEVICE:

            /* Keep the same code */
            MessageId = BugCheckCode;
            break;

        /* Check if this is a kernel-mode exception */
        case KERNEL_MODE_EXCEPTION_NOT_HANDLED:
        case SYSTEM_THREAD_EXCEPTION_NOT_HANDLED:
        case KMODE_EXCEPTION_NOT_HANDLED:

            /* Use the generic text message */
            MessageId = KMODE_EXCEPTION_NOT_HANDLED;
            break;

        /* File-system errors */
        case NTFS_FILE_SYSTEM:

            /* Use the generic message for FAT */
            MessageId = FAT_FILE_SYSTEM;
            break;

        /* Check if this is a coruption of the Mm's Pool */
        case DRIVER_CORRUPTED_MMPOOL:

            /* Use generic corruption message */
            MessageId = DRIVER_CORRUPTED_EXPOOL;
            break;

        /* Check if this is a signature check failure */
        case STATUS_SYSTEM_IMAGE_BAD_SIGNATURE:

            /* Use the generic corruption message */
            MessageId = BUGCODE_PSS_MESSAGE_SIGNATURE;
            break;

        /* All other codes */
        default:

            /* Use the default bugcheck message */
            MessageId = BUGCODE_PSS_MESSAGE;
            break;
    }

    /* Save bugcheck data */
    KiBugCheckData[0] = BugCheckCode;
    KiBugCheckData[1] = BugCheckParameter1;
    KiBugCheckData[2] = BugCheckParameter2;
    KiBugCheckData[3] = BugCheckParameter3;
    KiBugCheckData[4] = BugCheckParameter4;

    /* Now check what bugcheck this is */
    switch (BugCheckCode)
    {
        /* Invalid access to R/O memory or Unhandled KM Exception */
        case KERNEL_MODE_EXCEPTION_NOT_HANDLED:
        case ATTEMPTED_WRITE_TO_READONLY_MEMORY:
        case ATTEMPTED_EXECUTE_OF_NOEXECUTE_MEMORY:
        {
            /* Check if we have a trap frame */
            if (!TrapFrame)
            {
                /* Use parameter 3 as a trap frame, if it exists */
                if (BugCheckParameter3) TrapFrame = (PVOID)BugCheckParameter3;
            }

            /* Check if we got one now and if we need to get the Program Counter */
            if ((TrapFrame) &&
                (BugCheckCode != KERNEL_MODE_EXCEPTION_NOT_HANDLED))
            {
                /* Get the Program Counter */
                Pc = (PVOID)KeGetTrapFramePc(TrapFrame);
            }
            break;
        }

        /* Wrong IRQL */
        case IRQL_NOT_LESS_OR_EQUAL:
        {
            /*
             * The NT kernel has 3 special sections:
             * MISYSPTE, POOLMI and POOLCODE. The bug check code can
             * determine in which of these sections this bugcode happened
             * and provide a more detailed analysis. For now, we don't.
             */

            /* Program Counter is in parameter 4 */
            Pc = (PVOID)BugCheckParameter4;

            /* Get the driver base */
            DriverBase = KiPcToFileHeader(Pc,
                                          &LdrEntry,
                                          FALSE,
                                          &IsSystem);
            if (IsSystem)
            {
                /*
                 * The error happened inside the kernel or HAL.
                 * Get the memory address that was being referenced.
                 */
                Memory = (PVOID)BugCheckParameter1;

                /* Find to which driver it belongs */
                DriverBase = KiPcToFileHeader(Memory,
                                              &LdrEntry,
                                              TRUE,
                                              &IsSystem);
                if (DriverBase)
                {
                    /* Get the driver name and update the bug code */
                    KiBugCheckDriver = &LdrEntry->BaseDllName;
                    KiBugCheckData[0] = DRIVER_PORTION_MUST_BE_NONPAGED;
                }
                else
                {
                    /* Find the driver that unloaded at this address */
                    KiBugCheckDriver = NULL; // FIXME: ROS can't locate

                    /* Check if the cause was an unloaded driver */
                    if (KiBugCheckDriver)
                    {
                        /* Update bug check code */
                        KiBugCheckData[0] =
                            SYSTEM_SCAN_AT_RAISED_IRQL_CAUGHT_IMPROPER_DRIVER_UNLOAD;
                    }
                }
            }
            else
            {
                /* Update the bug check code */
                KiBugCheckData[0] = DRIVER_IRQL_NOT_LESS_OR_EQUAL;
            }

            /* Clear Pc so we don't look it up later */
            Pc = NULL;
            break;
        }

        /* Hard error */
        case FATAL_UNHANDLED_HARD_ERROR:
        {
            /* Copy bug check data from hard error */
            HardErrorParameters = (PULONG_PTR)BugCheckParameter2;
            KiBugCheckData[0] = BugCheckParameter1;
            KiBugCheckData[1] = HardErrorParameters[0];
            KiBugCheckData[2] = HardErrorParameters[1];
            KiBugCheckData[3] = HardErrorParameters[2];
            KiBugCheckData[4] = HardErrorParameters[3];

            /* Remember that this is hard error and set the caption/message */
            IsHardError = TRUE;
            HardErrCaption = (PCHAR)BugCheckParameter3;
            HardErrMessage = (PCHAR)BugCheckParameter4;
            break;
        }

        /* Page fault */
        case PAGE_FAULT_IN_NONPAGED_AREA:
        {
            /* Assume no driver */
            DriverBase = NULL;

            /* Check if we have a trap frame */
            if (!TrapFrame)
            {
                /* We don't, use parameter 3 if possible */
                if (BugCheckParameter3) TrapFrame = (PVOID)BugCheckParameter3;
            }

            /* Check if we have a frame now */
            if (TrapFrame)
            {
                /* Get the Program Counter */
                Pc = (PVOID)KeGetTrapFramePc(TrapFrame);
                KiBugCheckData[3] = (ULONG_PTR)Pc;

                /* Find out if was in the kernel or drivers */
                DriverBase = KiPcToFileHeader(Pc,
                                              &LdrEntry,
                                              FALSE,
                                              &IsSystem);
            }
            else
            {
                /* Can't blame a driver, assume system */
                IsSystem = TRUE;
            }

            /* FIXME: Check for session pool in addition to special pool */

            /* Special pool has its own bug check codes */
            if (MmIsSpecialPoolAddress((PVOID)BugCheckParameter1))
            {
                if (MmIsSpecialPoolAddressFree((PVOID)BugCheckParameter1))
                {
                    KiBugCheckData[0] = IsSystem
                        ? PAGE_FAULT_IN_FREED_SPECIAL_POOL
                        : DRIVER_PAGE_FAULT_IN_FREED_SPECIAL_POOL;
                }
                else
                {
                    KiBugCheckData[0] = IsSystem
                        ? PAGE_FAULT_BEYOND_END_OF_ALLOCATION
                        : DRIVER_PAGE_FAULT_BEYOND_END_OF_ALLOCATION;
                }
            }
            else if (!DriverBase)
            {
                /* Find the driver that unloaded at this address */
                KiBugCheckDriver = NULL; // FIXME: ROS can't locate

                /* Check if the cause was an unloaded driver */
                if (KiBugCheckDriver)
                {
                    KiBugCheckData[0] =
                        DRIVER_UNLOADED_WITHOUT_CANCELLING_PENDING_OPERATIONS;
                }
            }
            break;
        }

        /* Check if the driver forgot to unlock pages */
        case DRIVER_LEFT_LOCKED_PAGES_IN_PROCESS:

            /* Program Counter is in parameter 1 */
            Pc = (PVOID)BugCheckParameter1;
            break;

        /* Check if the driver consumed too many PTEs */
        case DRIVER_USED_EXCESSIVE_PTES:

            /* Loader entry is in parameter 1 */
            LdrEntry = (PVOID)BugCheckParameter1;
            KiBugCheckDriver = &LdrEntry->BaseDllName;
            break;

        /* Check if the driver has a stuck thread */
        case THREAD_STUCK_IN_DEVICE_DRIVER:

            /* The name is in Parameter 3 */
            KiBugCheckDriver = (PVOID)BugCheckParameter3;
            break;

        /* Anything else */
        default:
            break;
    }

    /* Do we have a driver name? */
    if (KiBugCheckDriver)
    {
        /* Convert it to ANSI */
        KeBugCheckUnicodeToAnsi(KiBugCheckDriver, AnsiName, sizeof(AnsiName));
    }
    else
    {
        /* Do we have a Program Counter? */
        if (Pc)
        {
            /* Dump image name */
            KiDumpParameterImages(AnsiName,
                                  (PULONG_PTR)&Pc,
                                  1,
                                  KeBugCheckUnicodeToAnsi);
        }
    }

    /* Check if we need to save the context for KD */
    if (!KdPitchDebugger) KdDebuggerDataBlock.SavedContext = (ULONG_PTR)&Context;

    /* Check if a debugger is connected */
    if ((BugCheckCode != MANUALLY_INITIATED_CRASH) && (KdDebuggerEnabled))
    {
        /* Crash on the debugger console */
        DbgPrint("\n*** Fatal System Error: 0x%08lx\n"
                 "                       (0x%p,0x%p,0x%p,0x%p)\n\n",
                 KiBugCheckData[0],
                 KiBugCheckData[1],
                 KiBugCheckData[2],
                 KiBugCheckData[3],
                 KiBugCheckData[4]);

        /* Check if the debugger isn't currently connected */
        if (!KdDebuggerNotPresent)
        {
            /* Check if we have a driver to blame */
            if (KiBugCheckDriver)
            {
                /* Dump it */
                DbgPrint("Driver at fault: %s.\n", AnsiName);
            }

            /* Check if this was a hard error */
            if (IsHardError)
            {
                /* Print caption and message */
                if (HardErrCaption) DbgPrint(HardErrCaption);
                if (HardErrMessage) DbgPrint(HardErrMessage);
            }

            /* Break in the debugger */
            KiBugCheckDebugBreak(DBG_STATUS_BUGCHECK_FIRST);
        }
    }

    /* Raise IRQL to HIGH_LEVEL */
    _disable();
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    /* Avoid recursion */
    if (!InterlockedDecrement((PLONG)&KeBugCheckCount))
    {
#ifdef CONFIG_SMP
        /* Set CPU that is bug checking now */
        KeBugCheckOwner = Prcb->Number;

        /* Freeze the other CPUs */
        for (i = 0; i < KeNumberProcessors; i++)
        {
            if (i != (LONG)KeGetCurrentProcessorNumber())
            {
                /* Send the IPI and give them one second to catch up */
                KiIpiSend(1 << i, IPI_FREEZE);
                KeStallExecutionProcessor(1000000);
            }
        }
#endif

        /* Display the BSOD */
        KiDisplayBlueScreen(MessageId,
                            IsHardError,
                            HardErrCaption,
                            HardErrMessage,
                            AnsiName);

        // TODO/FIXME: Run the registered reason-callbacks from
        // the KeBugcheckReasonCallbackListHead list with the
        // KbCallbackReserved1 reason.

        /* Check if the debugger is disabled but we can enable it */
        if (!(KdDebuggerEnabled) && !(KdPitchDebugger))
        {
            /* Enable it */
            KdEnableDebuggerWithLock(FALSE);
        }
        else
        {
            /* Otherwise, print the last line */
            InbvDisplayString("\r\n");
        }

        /* Save the context */
        Prcb->ProcessorState.ContextFrame = Context;

        /* FIXME: Support Triage Dump */

        /* FIXME: Write the crash dump */
    }
    else
    {
        /* Increase recursion count */
        KeBugCheckOwnerRecursionCount++;
        if (KeBugCheckOwnerRecursionCount == 2)
        {
            /* Break in the debugger */
            KiBugCheckDebugBreak(DBG_STATUS_BUGCHECK_SECOND);
        }
        else if (KeBugCheckOwnerRecursionCount > 2)
        {
            /* Halt execution */
            while (TRUE);
        }
    }

    /* Call the Callbacks */
    KiDoBugCheckCallbacks();

    /* FIXME: Call Watchdog if enabled */

    /* Check if we have to reboot */
    if (Reboot)
    {
        /* Unload symbols */
        DbgUnLoadImageSymbols(NULL, (PVOID)MAXULONG_PTR, 0);
        HalReturnToFirmware(HalRebootRoutine);
    }

    /* Attempt to break in the debugger (otherwise halt CPU) */
    KiBugCheckDebugBreak(DBG_STATUS_BUGCHECK_SECOND);

    /* Shouldn't get here */
    ASSERT(FALSE);
    while (TRUE);
}

BOOLEAN
NTAPI
KiHandleNmi(VOID)
{
    BOOLEAN Handled = FALSE;
    PKNMI_HANDLER_CALLBACK NmiData;

    /* Parse the list of callbacks */
    NmiData = KiNmiCallbackListHead;
    while (NmiData)
    {
        /* Save if this callback has handled it -- all it takes is one */
        Handled |= NmiData->Callback(NmiData->Context, Handled);
        NmiData = NmiData->Next;
    }

    /* Has anyone handled this? */
    return Handled;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
KeInitializeCrashDumpHeader(IN ULONG Type,
                            IN ULONG Flags,
                            OUT PVOID Buffer,
                            IN ULONG BufferSize,
                            OUT ULONG BufferNeeded OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeDeregisterBugCheckCallback(IN PKBUGCHECK_CALLBACK_RECORD CallbackRecord)
{
    KIRQL OldIrql;
    BOOLEAN Status = FALSE;

    /* Raise IRQL to High */
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    /* Check the Current State */
    if (CallbackRecord->State == BufferInserted)
    {
        /* Reset state and remove from list */
        CallbackRecord->State = BufferEmpty;
        RemoveEntryList(&CallbackRecord->Entry);
        Status = TRUE;
    }

    /* Lower IRQL and return */
    KeLowerIrql(OldIrql);
    return Status;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeDeregisterBugCheckReasonCallback(
    IN PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord)
{
    KIRQL OldIrql;
    BOOLEAN Status = FALSE;

    /* Raise IRQL to High */
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    /* Check the Current State */
    if (CallbackRecord->State == BufferInserted)
    {
        /* Reset state and remove from list */
        CallbackRecord->State = BufferEmpty;
        RemoveEntryList(&CallbackRecord->Entry);
        Status = TRUE;
    }

    /* Lower IRQL and return */
    KeLowerIrql(OldIrql);
    return Status;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeRegisterBugCheckCallback(IN PKBUGCHECK_CALLBACK_RECORD CallbackRecord,
                           IN PKBUGCHECK_CALLBACK_ROUTINE CallbackRoutine,
                           IN PVOID Buffer,
                           IN ULONG Length,
                           IN PUCHAR Component)
{
    KIRQL OldIrql;
    BOOLEAN Status = FALSE;

    /* Raise IRQL to High */
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    /* Check the Current State first so we don't double-register */
    if (CallbackRecord->State == BufferEmpty)
    {
        /* Set the Callback Settings and insert into the list */
        CallbackRecord->Length = Length;
        CallbackRecord->Buffer = Buffer;
        CallbackRecord->Component = Component;
        CallbackRecord->CallbackRoutine = CallbackRoutine;
        CallbackRecord->State = BufferInserted;
        InsertTailList(&KeBugcheckCallbackListHead, &CallbackRecord->Entry);
        Status = TRUE;
    }

    /* Lower IRQL and return */
    KeLowerIrql(OldIrql);
    return Status;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeRegisterBugCheckReasonCallback(
    IN PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord,
    IN PKBUGCHECK_REASON_CALLBACK_ROUTINE CallbackRoutine,
    IN KBUGCHECK_CALLBACK_REASON Reason,
    IN PUCHAR Component)
{
    KIRQL OldIrql;
    BOOLEAN Status = FALSE;

    /* Raise IRQL to High */
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    /* Check the Current State first so we don't double-register */
    if (CallbackRecord->State == BufferEmpty)
    {
        /* Set the Callback Settings and insert into the list */
        CallbackRecord->Component = Component;
        CallbackRecord->CallbackRoutine = CallbackRoutine;
        CallbackRecord->State = BufferInserted;
        CallbackRecord->Reason = Reason;
        InsertTailList(&KeBugcheckReasonCallbackListHead,
                       &CallbackRecord->Entry);
        Status = TRUE;
    }

    /* Lower IRQL and return */
    KeLowerIrql(OldIrql);
    return Status;
}

/*
 * @implemented
 */
PVOID
NTAPI
KeRegisterNmiCallback(IN PNMI_CALLBACK CallbackRoutine,
                      IN PVOID Context)
{
    KIRQL OldIrql;
    PKNMI_HANDLER_CALLBACK NmiData, Next;
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Allocate NMI callback data */
    NmiData = ExAllocatePoolWithTag(NonPagedPool, sizeof(*NmiData), TAG_KNMI);
    if (!NmiData) return NULL;

    /* Fill in the information */
    NmiData->Callback = CallbackRoutine;
    NmiData->Context = Context;
    NmiData->Handle = NmiData;

    /* Insert it into NMI callback list */
    KiAcquireNmiListLock(&OldIrql);
    NmiData->Next = KiNmiCallbackListHead;
    Next = InterlockedCompareExchangePointer((PVOID*)&KiNmiCallbackListHead,
                                             NmiData,
                                             NmiData->Next);
    ASSERT(Next == NmiData->Next);
    KiReleaseNmiListLock(OldIrql);

    /* Return the opaque "handle" */
    return NmiData->Handle;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
KeDeregisterNmiCallback(IN PVOID Handle)
{
    KIRQL OldIrql;
    PKNMI_HANDLER_CALLBACK NmiData;
    PKNMI_HANDLER_CALLBACK* Previous;
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Find in the list the NMI callback corresponding to the handle */
    KiAcquireNmiListLock(&OldIrql);
    Previous = &KiNmiCallbackListHead;
    NmiData = *Previous;
    while (NmiData)
    {
        if (NmiData->Handle == Handle)
        {
            /* The handle is the pointer to the callback itself */
            ASSERT(Handle == NmiData);

            /* Found it, remove from the list */
            *Previous = NmiData->Next;
            break;
        }

        /* Not found; try again */
        Previous = &NmiData->Next;
        NmiData = *Previous;
    }
    KiReleaseNmiListLock(OldIrql);

    /* If we have found the entry, free it */
    if (NmiData)
    {
        ExFreePoolWithTag(NmiData, TAG_KNMI);
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_HANDLE;
}

/*
 * @implemented
 */
DECLSPEC_NORETURN
VOID
NTAPI
KeBugCheckEx(IN ULONG BugCheckCode,
             IN ULONG_PTR BugCheckParameter1,
             IN ULONG_PTR BugCheckParameter2,
             IN ULONG_PTR BugCheckParameter3,
             IN ULONG_PTR BugCheckParameter4)
{
    /* Call the internal API */
    KeBugCheckWithTf(BugCheckCode,
                     BugCheckParameter1,
                     BugCheckParameter2,
                     BugCheckParameter3,
                     BugCheckParameter4,
                     NULL);
}

/*
 * @implemented
 */
DECLSPEC_NORETURN
VOID
NTAPI
KeBugCheck(ULONG BugCheckCode)
{
    /* Call the internal API */
    KeBugCheckWithTf(BugCheckCode, 0, 0, 0, 0, NULL);
}

/*
 * @implemented
 */
VOID
NTAPI
KeEnterKernelDebugger(VOID)
{
    /* Disable interrupts */
    KiHardwareTrigger = 1;
    _disable();

    /* Check the bugcheck count */
    if (!InterlockedDecrement((PLONG)&KeBugCheckCount))
    {
        /* There was only one, is the debugger disabled? */
        if (!(KdDebuggerEnabled) && !(KdPitchDebugger))
        {
            /* Enable the debugger */
            KdInitSystem(0, NULL);
        }
    }

    /* Break in the debugger */
    KiBugCheckDebugBreak(DBG_STATUS_FATAL);
}

/* EOF */
