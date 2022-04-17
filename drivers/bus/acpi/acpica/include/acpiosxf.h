/******************************************************************************
 *
 * Name: acpiosxf.h - All interfaces to the OS Services Layer (OSL). These
 *                    interfaces must be implemented by OSL to interface the
 *                    ACPI components to the host operating system.
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2021, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#ifndef __ACPIOSXF_H__
#define __ACPIOSXF_H__

#include "platform/acenv.h"
#include "actypes.h"


/* Types for AcpiOsExecute */

typedef enum
{
    OSL_GLOBAL_LOCK_HANDLER,
    OSL_NOTIFY_HANDLER,
    OSL_GPE_HANDLER,
    OSL_DEBUGGER_MAIN_THREAD,
    OSL_DEBUGGER_EXEC_THREAD,
    OSL_EC_POLL_HANDLER,
    OSL_EC_BURST_HANDLER

} ACPI_EXECUTE_TYPE;

#define ACPI_NO_UNIT_LIMIT          ((UINT32) -1)
#define ACPI_MUTEX_SEM              1


/* Functions for AcpiOsSignal */

#define ACPI_SIGNAL_FATAL           0
#define ACPI_SIGNAL_BREAKPOINT      1

typedef struct acpi_signal_fatal_info
{
    UINT32                  Type;
    UINT32                  Code;
    UINT32                  Argument;

} ACPI_SIGNAL_FATAL_INFO;


/*
 * OSL Initialization and shutdown primitives
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsInitialize
ACPI_STATUS
AcpiOsInitialize (
    void);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsTerminate
ACPI_STATUS
AcpiOsTerminate (
    void);
#endif


/*
 * ACPI Table interfaces
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetRootPointer
ACPI_PHYSICAL_ADDRESS
AcpiOsGetRootPointer (
    void);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsPredefinedOverride
ACPI_STATUS
AcpiOsPredefinedOverride (
    const ACPI_PREDEFINED_NAMES *InitVal,
    ACPI_STRING                 *NewVal);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsTableOverride
ACPI_STATUS
AcpiOsTableOverride (
    ACPI_TABLE_HEADER       *ExistingTable,
    ACPI_TABLE_HEADER       **NewTable);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsPhysicalTableOverride
ACPI_STATUS
AcpiOsPhysicalTableOverride (
    ACPI_TABLE_HEADER       *ExistingTable,
    ACPI_PHYSICAL_ADDRESS   *NewAddress,
    UINT32                  *NewTableLength);
#endif


/*
 * Spinlock primitives
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsCreateLock
ACPI_STATUS
AcpiOsCreateLock (
    ACPI_SPINLOCK           *OutHandle);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsDeleteLock
void
AcpiOsDeleteLock (
    ACPI_SPINLOCK           Handle);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsAcquireLock
ACPI_CPU_FLAGS
AcpiOsAcquireLock (
    ACPI_SPINLOCK           Handle);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReleaseLock
void
AcpiOsReleaseLock (
    ACPI_SPINLOCK           Handle,
    ACPI_CPU_FLAGS          Flags);
#endif


/*
 * Semaphore primitives
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsCreateSemaphore
ACPI_STATUS
AcpiOsCreateSemaphore (
    UINT32                  MaxUnits,
    UINT32                  InitialUnits,
    ACPI_SEMAPHORE          *OutHandle);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsDeleteSemaphore
ACPI_STATUS
AcpiOsDeleteSemaphore (
    ACPI_SEMAPHORE          Handle);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsWaitSemaphore
ACPI_STATUS
AcpiOsWaitSemaphore (
    ACPI_SEMAPHORE          Handle,
    UINT32                  Units,
    UINT16                  Timeout);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsSignalSemaphore
ACPI_STATUS
AcpiOsSignalSemaphore (
    ACPI_SEMAPHORE          Handle,
    UINT32                  Units);
#endif


/*
 * Mutex primitives. May be configured to use semaphores instead via
 * ACPI_MUTEX_TYPE (see platform/acenv.h)
 */
#if (ACPI_MUTEX_TYPE != ACPI_BINARY_SEMAPHORE)

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsCreateMutex
ACPI_STATUS
AcpiOsCreateMutex (
    ACPI_MUTEX              *OutHandle);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsDeleteMutex
void
AcpiOsDeleteMutex (
    ACPI_MUTEX              Handle);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsAcquireMutex
ACPI_STATUS
AcpiOsAcquireMutex (
    ACPI_MUTEX              Handle,
    UINT16                  Timeout);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReleaseMutex
void
AcpiOsReleaseMutex (
    ACPI_MUTEX              Handle);
#endif

#endif


/*
 * Memory allocation and mapping
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsAllocate
void *
AcpiOsAllocate (
    ACPI_SIZE               Size);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsAllocateZeroed
void *
AcpiOsAllocateZeroed (
    ACPI_SIZE               Size);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsFree
void
AcpiOsFree (
    void *                  Memory);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsMapMemory
void *
AcpiOsMapMemory (
    ACPI_PHYSICAL_ADDRESS   Where,
    ACPI_SIZE               Length);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsUnmapMemory
void
AcpiOsUnmapMemory (
    void                    *LogicalAddress,
    ACPI_SIZE               Size);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetPhysicalAddress
ACPI_STATUS
AcpiOsGetPhysicalAddress (
    void                    *LogicalAddress,
    ACPI_PHYSICAL_ADDRESS   *PhysicalAddress);
#endif


/*
 * Memory/Object Cache
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsCreateCache
ACPI_STATUS
AcpiOsCreateCache (
    char                    *CacheName,
    UINT16                  ObjectSize,
    UINT16                  MaxDepth,
    ACPI_CACHE_T            **ReturnCache);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsDeleteCache
ACPI_STATUS
AcpiOsDeleteCache (
    ACPI_CACHE_T            *Cache);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsPurgeCache
ACPI_STATUS
AcpiOsPurgeCache (
    ACPI_CACHE_T            *Cache);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsAcquireObject
void *
AcpiOsAcquireObject (
    ACPI_CACHE_T            *Cache);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReleaseObject
ACPI_STATUS
AcpiOsReleaseObject (
    ACPI_CACHE_T            *Cache,
    void                    *Object);
#endif


/*
 * Interrupt handlers
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsInstallInterruptHandler
ACPI_STATUS
AcpiOsInstallInterruptHandler (
    UINT32                  InterruptNumber,
    ACPI_OSD_HANDLER        ServiceRoutine,
    void                    *Context);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsRemoveInterruptHandler
ACPI_STATUS
AcpiOsRemoveInterruptHandler (
    UINT32                  InterruptNumber,
    ACPI_OSD_HANDLER        ServiceRoutine);
#endif


/*
 * Threads and Scheduling
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetThreadId
ACPI_THREAD_ID
AcpiOsGetThreadId (
    void);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsExecute
ACPI_STATUS
AcpiOsExecute (
    ACPI_EXECUTE_TYPE       Type,
    ACPI_OSD_EXEC_CALLBACK  Function,
    void                    *Context);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsWaitEventsComplete
void
AcpiOsWaitEventsComplete (
    void);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsSleep
void
AcpiOsSleep (
    UINT64                  Milliseconds);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsStall
void
AcpiOsStall (
    UINT32                  Microseconds);
#endif


/*
 * Platform and hardware-independent I/O interfaces
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReadPort
ACPI_STATUS
AcpiOsReadPort (
    ACPI_IO_ADDRESS         Address,
    UINT32                  *Value,
    UINT32                  Width);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsWritePort
ACPI_STATUS
AcpiOsWritePort (
    ACPI_IO_ADDRESS         Address,
    UINT32                  Value,
    UINT32                  Width);
#endif


/*
 * Platform and hardware-independent physical memory interfaces
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReadMemory
ACPI_STATUS
AcpiOsReadMemory (
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT64                  *Value,
    UINT32                  Width);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsWriteMemory
ACPI_STATUS
AcpiOsWriteMemory (
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT64                  Value,
    UINT32                  Width);
#endif


/*
 * Platform and hardware-independent PCI configuration space access
 * Note: Can't use "Register" as a parameter, changed to "Reg" --
 * certain compilers complain.
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReadPciConfiguration
ACPI_STATUS
AcpiOsReadPciConfiguration (
    ACPI_PCI_ID             *PciId,
    UINT32                  Reg,
    UINT64                  *Value,
    UINT32                  Width);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsWritePciConfiguration
ACPI_STATUS
AcpiOsWritePciConfiguration (
    ACPI_PCI_ID             *PciId,
    UINT32                  Reg,
    UINT64                  Value,
    UINT32                  Width);
#endif


/*
 * Miscellaneous
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReadable
BOOLEAN
AcpiOsReadable (
    void                    *Pointer,
    ACPI_SIZE               Length);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsWritable
BOOLEAN
AcpiOsWritable (
    void                    *Pointer,
    ACPI_SIZE               Length);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetTimer
UINT64
AcpiOsGetTimer (
    void);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsSignal
ACPI_STATUS
AcpiOsSignal (
    UINT32                  Function,
    void                    *Info);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsEnterSleep
ACPI_STATUS
AcpiOsEnterSleep (
    UINT8                   SleepState,
    UINT32                  RegaValue,
    UINT32                  RegbValue);
#endif


/*
 * Debug print routines
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsPrintf
ACPI_PRINTF_LIKE (1)
void ACPI_INTERNAL_VAR_XFACE
AcpiOsPrintf (
    const char              *Format,
    ...);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsVprintf
void
AcpiOsVprintf (
    const char              *Format,
    va_list                 Args);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsRedirectOutput
void
AcpiOsRedirectOutput (
    void                    *Destination);
#endif


/*
 * Debug IO
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetLine
ACPI_STATUS
AcpiOsGetLine (
    char                    *Buffer,
    UINT32                  BufferLength,
    UINT32                  *BytesRead);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsInitializeDebugger
ACPI_STATUS
AcpiOsInitializeDebugger (
    void);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsTerminateDebugger
void
AcpiOsTerminateDebugger (
    void);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsWaitCommandReady
ACPI_STATUS
AcpiOsWaitCommandReady (
    void);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsNotifyCommandComplete
ACPI_STATUS
AcpiOsNotifyCommandComplete (
    void);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsTracePoint
void
AcpiOsTracePoint (
    ACPI_TRACE_EVENT_TYPE   Type,
    BOOLEAN                 Begin,
    UINT8                   *Aml,
    char                    *Pathname);
#endif


/*
 * Obtain ACPI table(s)
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetTableByName
ACPI_STATUS
AcpiOsGetTableByName (
    char                    *Signature,
    UINT32                  Instance,
    ACPI_TABLE_HEADER       **Table,
    ACPI_PHYSICAL_ADDRESS   *Address);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetTableByIndex
ACPI_STATUS
AcpiOsGetTableByIndex (
    UINT32                  Index,
    ACPI_TABLE_HEADER       **Table,
    UINT32                  *Instance,
    ACPI_PHYSICAL_ADDRESS   *Address);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetTableByAddress
ACPI_STATUS
AcpiOsGetTableByAddress (
    ACPI_PHYSICAL_ADDRESS   Address,
    ACPI_TABLE_HEADER       **Table);
#endif


/*
 * Directory manipulation
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsOpenDirectory
void *
AcpiOsOpenDirectory (
    char                    *Pathname,
    char                    *WildcardSpec,
    char                    RequestedFileType);
#endif

/* RequesteFileType values */

#define REQUEST_FILE_ONLY                   0
#define REQUEST_DIR_ONLY                    1


#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetNextFilename
char *
AcpiOsGetNextFilename (
    void                    *DirHandle);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsCloseDirectory
void
AcpiOsCloseDirectory (
    void                    *DirHandle);
#endif


#endif /* __ACPIOSXF_H__ */
