/*******************************************************************************
*                                                                              *
* ACPI Component Architecture Operating System Layer (OSL) for ReactOS         *
*                                                                              *
*******************************************************************************/

/*
 *  Copyright (C) 2000 Andrew Henroid
 *  Copyright (C) 2001 Andrew Grover
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <acpisys.h>

#define NDEBUG
#include <debug.h>

static PKINTERRUPT AcpiInterrupt;
static BOOLEAN AcpiInterruptHandlerRegistered = FALSE;
static OSD_HANDLER AcpiIrqHandler = NULL;
static PVOID AcpiIrqContext = NULL;
static ULONG AcpiIrqNumber = 0;
static KDPC AcpiDpc;
static PVOID IVTVirtualAddress = NULL;
static PVOID BDAVirtualAddress = NULL;


VOID STDCALL
OslDpcStub(
  IN PKDPC Dpc,
  IN PVOID DeferredContext,
  IN PVOID SystemArgument1,
  IN PVOID SystemArgument2)
{
  OSD_EXECUTION_CALLBACK Routine = (OSD_EXECUTION_CALLBACK)SystemArgument1;

  DPRINT("OslDpcStub()\n");

  DPRINT("Calling [%p]([%p])\n", Routine, SystemArgument2);

  (*Routine)(SystemArgument2);
}


ACPI_STATUS
acpi_os_remove_interrupt_handler(
  u32 irq,
  OSD_HANDLER handler);


ACPI_STATUS
acpi_os_initialize(void)
{
  DPRINT("acpi_os_initialize()\n");

  KeInitializeDpc(&AcpiDpc, OslDpcStub, NULL);

	return AE_OK;
}

ACPI_STATUS
acpi_os_terminate(void)
{
  DPRINT("acpi_os_terminate()\n");

  if (AcpiInterruptHandlerRegistered) {
    acpi_os_remove_interrupt_handler(AcpiIrqNumber, AcpiIrqHandler);
  }

  return AE_OK;
}

s32
acpi_os_printf(const NATIVE_CHAR *fmt,...)
{
	LONG Size;
	va_list args;
	va_start(args, fmt);
	Size = acpi_os_vprintf(fmt, args);
	va_end(args);
	return Size;
}

s32
acpi_os_vprintf(const NATIVE_CHAR *fmt, va_list args)
{
	static char Buffer[512];
  LONG Size = vsprintf(Buffer, fmt, args);

	DbgPrint("%s", Buffer);
	return Size;
}

void *
acpi_os_allocate(u32 size)
{
  return ExAllocatePool(NonPagedPool, size);
}

void *
acpi_os_callocate(u32 size)
{
  PVOID ptr = ExAllocatePool(NonPagedPool, size);
  if (ptr)
    memset(ptr, 0, size);
  return ptr;
}

void
acpi_os_free(void *ptr)
{
  if (ptr) {
    /* FIXME: There is at least one bug somewhere that
              results in an attempt to release a null pointer */
    ExFreePool(ptr);
  }
}

ACPI_STATUS
acpi_os_map_memory(ACPI_PHYSICAL_ADDRESS phys, u32 size, void **virt)
{
  PHYSICAL_ADDRESS Address;
  PVOID Virtual;

  DPRINT("acpi_os_map_memory(phys 0x%X  size 0x%X)\n", (ULONG)phys, size);

  if (phys == 0x0) {
    /* Real mode Interrupt Vector Table */
    Virtual = ExAllocatePool(NonPagedPool, size);
    if (NT_SUCCESS(NtVdmControl(0, Virtual))) {
      IVTVirtualAddress = Virtual;
      *virt = Virtual;
      return AE_OK;
    } else {
      return AE_ERROR;
    }
  }

  if ((ULONG)phys >= 0x100000) {
    Address.QuadPart = (ULONG)phys;
    *virt = MmMapIoSpace(Address, size, FALSE);
    if (!*virt)
      return AE_ERROR;
  } else {
    *virt = (PVOID)((ULONG)phys);
  }

  return AE_OK;
}

void
acpi_os_unmap_memory(void *virt, u32 size)
{
  DPRINT("acpi_os_unmap_memory()\n");

  if (virt == IVTVirtualAddress) {
    /* Real mode Interrupt Vector Table */
    ExFreePool(IVTVirtualAddress);
    IVTVirtualAddress = NULL;
    return;
  }
  /* FIXME: Causes "Memory area is NULL" bugcheck in marea.c */
  //if ((ULONG)virt >= 0x100000)
    //MmUnmapIoSpace(virt, size);
}

ACPI_STATUS
acpi_os_get_physical_address(void *virt, ACPI_PHYSICAL_ADDRESS *phys)
{
  PHYSICAL_ADDRESS Address;

  DPRINT("acpi_os_get_physical_address()\n");

  if (!phys || !virt)
    return AE_BAD_PARAMETER;

  Address = MmGetPhysicalAddress(virt);

  *phys = (ULONG)Address.QuadPart;

  return AE_OK;
}

BOOLEAN STDCALL
OslIsrStub(
  PKINTERRUPT Interrupt,
  PVOID ServiceContext)
{
  INT32 Status;

  Status = (*AcpiIrqHandler)(AcpiIrqContext);

  if (Status == INTERRUPT_HANDLED)
    return TRUE;
  else
    return FALSE;
}

ACPI_STATUS
acpi_os_install_interrupt_handler(u32 irq, OSD_HANDLER handler, void *context)
{
  ULONG Vector;
  KIRQL DIrql;
  KAFFINITY Affinity;
  NTSTATUS Status;

  DPRINT("acpi_os_install_interrupt_handler()\n");

  Vector = HalGetInterruptVector(
    Internal,
    0,
    0,
    irq,
    &DIrql,
    &Affinity);

  Status = IoConnectInterrupt(
    &AcpiInterrupt,
    OslIsrStub,
    NULL,
    NULL,
    Vector,
    DIrql,
    DIrql,
    LevelSensitive, /* FIXME: LevelSensitive or Latched? */
    FALSE,
    Affinity,
    FALSE);
  if (!NT_SUCCESS(Status)) {
	  DPRINT("Could not connect to interrupt %d\n", Vector);
    return AE_ERROR;
  }

  AcpiIrqNumber = irq;
  AcpiIrqHandler = handler;
  AcpiIrqContext = context;
  AcpiInterruptHandlerRegistered = TRUE;

	return AE_OK;
}

ACPI_STATUS
acpi_os_remove_interrupt_handler(u32 irq, OSD_HANDLER handler)
{
  DPRINT("acpi_os_remove_interrupt_handler()\n");

  if (AcpiInterruptHandlerRegistered) {
    IoDisconnectInterrupt(AcpiInterrupt);
    AcpiInterrupt = NULL;
    AcpiInterruptHandlerRegistered = FALSE;
  }

  return AE_OK;
}

void
acpi_os_sleep(u32 sec, u32 ms)
{
  /* FIXME: Wait */
}

void
acpi_os_sleep_usec(u32 us)
{
  KeStallExecutionProcessor(us);
}

u8
acpi_os_in8(ACPI_IO_ADDRESS port)
{
  return READ_PORT_UCHAR((PUCHAR)port);
}

u16
acpi_os_in16(ACPI_IO_ADDRESS port)
{
  return READ_PORT_USHORT((PUSHORT)port);
}

u32
acpi_os_in32(ACPI_IO_ADDRESS port)
{
  return READ_PORT_ULONG((PULONG)port);
}

void
acpi_os_out8(ACPI_IO_ADDRESS port, u8 val)
{
  WRITE_PORT_UCHAR((PUCHAR)port, val);
}

void
acpi_os_out16(ACPI_IO_ADDRESS port, u16 val)
{
  WRITE_PORT_USHORT((PUSHORT)port, val);
}

void
acpi_os_out32(ACPI_IO_ADDRESS port, u32 val)
{
  WRITE_PORT_ULONG((PULONG)port, val);
}

UINT8
acpi_os_mem_in8 (ACPI_PHYSICAL_ADDRESS phys_addr)
{
  return (*(PUCHAR)(ULONG)phys_addr);
}

UINT16
acpi_os_mem_in16 (ACPI_PHYSICAL_ADDRESS phys_addr)
{
  return (*(PUSHORT)(ULONG)phys_addr);
}

UINT32
acpi_os_mem_in32 (ACPI_PHYSICAL_ADDRESS phys_addr)
{
  return (*(PULONG)(ULONG)phys_addr);
}

void
acpi_os_mem_out8 (ACPI_PHYSICAL_ADDRESS phys_addr, UINT8 value)
{
  *(PUCHAR)(ULONG)phys_addr = value;
}

void
acpi_os_mem_out16 (ACPI_PHYSICAL_ADDRESS phys_addr, UINT16 value)
{
  *(PUSHORT)(ULONG)phys_addr = value;
}

void
acpi_os_mem_out32 (ACPI_PHYSICAL_ADDRESS phys_addr, UINT32 value)
{
  *(PULONG)(ULONG)phys_addr = value;
}

ACPI_STATUS
acpi_os_read_pci_cfg_byte(
	u32 bus,
	u32 func,
	u32 addr,
	u8 * val)
{
  /* FIXME: What do we do here? */

  DPRINT("acpi_os_read_pci_cfg_byte is not implemented");

  return AE_ERROR;
}

ACPI_STATUS
acpi_os_read_pci_cfg_word(
	u32 bus,
	u32 func,
	u32 addr,
	u16 * val)
{
  /* FIXME: What do we do here? */

  DPRINT("acpi_os_read_pci_cfg_word is not implemented");

  return AE_ERROR;
}

ACPI_STATUS
acpi_os_read_pci_cfg_dword(
	u32 bus,
	u32 func,
	u32 addr,
	u32 * val)
{
  /* FIXME: What do we do here? */

  DPRINT("acpi_os_read_pci_cfg_dword is not implemented");

  return AE_ERROR;
}

ACPI_STATUS
acpi_os_write_pci_cfg_byte(
	u32 bus,
	u32 func,
	u32 addr,
	u8 val)
{
  /* FIXME: What do we do here? */

  DPRINT("acpi_os_write_pci_cfg_byte is not implemented");

  return AE_ERROR;
}

ACPI_STATUS
acpi_os_write_pci_cfg_word(
	u32 bus,
	u32 func,
	u32 addr,
	u16 val)
{
  /* FIXME: What do we do here? */

  DPRINT("acpi_os_write_pci_cfg_word is not implemented");

  return AE_ERROR;
}

ACPI_STATUS
acpi_os_write_pci_cfg_dword(
	u32 bus,
	u32 func,
	u32 addr,
	u32 val)
{
  /* FIXME: What do we do here? */

  DPRINT("acpi_os_write_pci_cfg_dword is not implemented");

  return AE_ERROR;
}

ACPI_STATUS
acpi_os_load_module (
	char *module_name)
{
  DPRINT("acpi_os_load_module()\n");

  if (!module_name)
    return AE_BAD_PARAMETER;

  return AE_OK;
}

ACPI_STATUS
acpi_os_unload_module (
	char *module_name)
{
  DPRINT("acpi_os_unload_module()\n");

  if (!module_name)
    return AE_BAD_PARAMETER;

  return AE_OK;
}

ACPI_STATUS
acpi_os_queue_for_execution(
	u32                     priority,
	OSD_EXECUTION_CALLBACK  function,
	void                    *context)
{
  ACPI_STATUS Status = AE_OK;

  DPRINT("acpi_os_queue_for_execution()\n");

  if (!function)
    return AE_BAD_PARAMETER;

  DPRINT("Scheduling task [%p](%p) for execution.\n", function, context);

#if 0
  switch (priority) {
  case OSD_PRIORITY_MED:
    KeSetImportanceDpc(&AcpiDpc, MediumImportance);
  case OSD_PRIORITY_LO:
    KeSetImportanceDpc(&AcpiDpc, LowImportance);
  case OSD_PRIORITY_HIGH:
  default:
    KeSetImportanceDpc(&AcpiDpc, HighImportance);
  }
#endif

  KeInsertQueueDpc(&AcpiDpc, (PVOID)function, (PVOID)context);

  return Status;
}

ACPI_STATUS
acpi_os_create_semaphore(
	u32		max_units,
	u32		initial_units,
	ACPI_HANDLE	*handle)
{
  PFAST_MUTEX Mutex;

  Mutex = ExAllocatePool(NonPagedPool, sizeof(FAST_MUTEX));
  if (!Mutex)
    return AE_NO_MEMORY;

  DPRINT("acpi_os_create_semaphore() at 0x%X\n", Mutex);

  ExInitializeFastMutex(Mutex);

  *handle = Mutex;
  return AE_OK;
}

ACPI_STATUS
acpi_os_delete_semaphore(
	ACPI_HANDLE handle)
{
  PFAST_MUTEX Mutex = (PFAST_MUTEX)handle;

  DPRINT("acpi_os_delete_semaphore(handle 0x%X)\n", handle);

  if (!Mutex) 
    return AE_BAD_PARAMETER;

  ExFreePool(Mutex);

  return AE_OK;
}

ACPI_STATUS
acpi_os_wait_semaphore(
	ACPI_HANDLE	handle,
	u32                     units,
	u32                     timeout)
{
  ACPI_STATUS Status = AE_OK;
  PFAST_MUTEX Mutex = (PFAST_MUTEX)handle;

  if (!Mutex || (units < 1)) {
    DPRINT("acpi_os_wait_semaphore(handle 0x%X, units %d) Bad parameters\n",
      handle, units);
    return AE_BAD_PARAMETER;
  }

  DPRINT("Waiting for semaphore[%p|%d|%d]\n", handle, units, timeout);

  //ExAcquireFastMutex(Mutex);

  return AE_OK;
}

ACPI_STATUS
acpi_os_signal_semaphore(
    ACPI_HANDLE             handle, 
    u32                     units)
{
  PFAST_MUTEX Mutex = (PFAST_MUTEX)handle;

  if (!Mutex || (units < 1)) {
    DPRINT("acpi_os_signal_semaphore(handle 0x%X) Bad parameter\n", handle);
    return AE_BAD_PARAMETER;
  }

  DPRINT("Signaling semaphore[%p|%d]\n", handle, units);

  //ExReleaseFastMutex(Mutex);

  return AE_OK;
}

ACPI_STATUS
acpi_os_breakpoint(NATIVE_CHAR *msg)
{
	DPRINT1("BREAKPOINT: %s", msg);
	return AE_OK;
}

void
acpi_os_dbg_trap(char *msg)

{
  DPRINT1("TRAP: %s", msg);
}

void
acpi_os_dbg_assert(void *failure, void *file, u32 line, NATIVE_CHAR *msg)
{
  DPRINT1("ASSERT: %s\n", msg);
}

u32
acpi_os_get_line(NATIVE_CHAR *buffer)
{
	return 0;
}

u8
acpi_os_readable(void *ptr, u32 len)
{
  /* Always readable */
	return TRUE;
}

u8
acpi_os_writable(void *ptr, u32 len)
{
  /* Always writable */
	return TRUE;
}

u32
acpi_os_get_thread_id (void)
{
  return (ULONG)PsGetCurrentThreadId();
}
