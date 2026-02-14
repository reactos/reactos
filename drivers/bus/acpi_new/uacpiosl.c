#include "precomp.h"

//#define NDEBUG
#include <debug.h>

UINT32
ACPIInitUACPI(void)
{
    uacpi_status status = uacpi_initialize(0);
    if (uacpi_unlikely_error(status))
    {
        DPRINT1("uacpi_initialize error: %s\n", uacpi_status_to_string(status));
    }
    return status;
}

#ifndef UACPI_FORMATTED_LOGGING
void uacpi_kernel_log(uacpi_log_level Level, const uacpi_char* Char)
{

}
#else
UACPI_PRINTF_DECL(2, 3)
void uacpi_kernel_log(uacpi_log_level Level, const uacpi_char* Char, ...)
{

}
void uacpi_kernel_vlog(uacpi_log_level Level, const uacpi_char* Char, uacpi_va_list list)
{
  
}
#endif

uacpi_u64
uacpi_kernel_get_nanoseconds_since_boot(void)
{
    UNIMPLEMENTED_DBGBREAK();
    return 0;
}

void
uacpi_kernel_stall(uacpi_u8 usec)
{
    UNIMPLEMENTED_DBGBREAK();
}

void uacpi_kernel_sleep(uacpi_u64 msec)
{
    UNIMPLEMENTED_DBGBREAK();
}

uacpi_handle
uacpi_kernel_create_event(void)
{
    UNIMPLEMENTED_DBGBREAK();
    return NULL;
}

void
uacpi_kernel_free_event(uacpi_handle Handle)
{
    UNIMPLEMENTED_DBGBREAK();
}

uacpi_bool
uacpi_kernel_wait_for_event(uacpi_handle Handle, uacpi_u16 Timeout)
{
    UNIMPLEMENTED_DBGBREAK();
    return 1;
}

void
uacpi_kernel_signal_event(uacpi_handle Handle)
{
    UNIMPLEMENTED_DBGBREAK();
}

void 
uacpi_kernel_reset_event(uacpi_handle Handle)
{
    UNIMPLEMENTED_DBGBREAK();
}

uacpi_handle
uacpi_kernel_create_spinlock(void)
{
    UNIMPLEMENTED_DBGBREAK();
    return NULL;
}

void
uacpi_kernel_free_spinlock(uacpi_handle Handle)
{
    UNIMPLEMENTED_DBGBREAK();
}

uacpi_cpu_flags
uacpi_kernel_lock_spinlock(uacpi_handle Handle)
{
    UNIMPLEMENTED_DBGBREAK();
    return 1;
}

void
uacpi_kernel_unlock_spinlock(uacpi_handle Handle, uacpi_cpu_flags Flags)
{
    UNIMPLEMENTED_DBGBREAK();
}

void*
uacpi_kernel_alloc(uacpi_size size)
{
    UNIMPLEMENTED_DBGBREAK();
    return NULL;
}

void *
uacpi_kernel_calloc(uacpi_size count, uacpi_size size)
{
    UNIMPLEMENTED_DBGBREAK();
    return NULL;
}

#ifndef UACPI_SIZED_FREES
void
uacpi_kernel_free(void *mem)
{
    UNIMPLEMENTED_DBGBREAK();
}
#else
void
uacpi_kernel_free(void *mem, uacpi_size size_hint)
{
    UNIMPLEMENTED_DBGBREAK();
}
#endif

uacpi_handle
uacpi_kernel_create_mutex(void)
{
    UNIMPLEMENTED_DBGBREAK();
    return NULL;
}

void
uacpi_kernel_free_mutex(uacpi_handle handle)
{
    UNIMPLEMENTED_DBGBREAK();
}

uacpi_status
uacpi_kernel_acquire_mutex(uacpi_handle Handle, uacpi_u16 Timeout)
{
    UNIMPLEMENTED_DBGBREAK();
    return 1;
}

void
uacpi_kernel_release_mutex(uacpi_handle Handle)
{
    UNIMPLEMENTED_DBGBREAK();
}

uacpi_status
uacpi_kernel_io_map(uacpi_io_addr base, uacpi_size len, uacpi_handle *out_handle)
{
    UNIMPLEMENTED_DBGBREAK();
    return 1;
}

void uacpi_kernel_io_unmap(uacpi_handle handle)
{
    UNIMPLEMENTED_DBGBREAK(); 
}

uacpi_status
uacpi_kernel_handle_firmware_request(uacpi_firmware_request* Req)
{
    UNIMPLEMENTED_DBGBREAK();
    return 1;
}

uacpi_thread_id
uacpi_kernel_get_thread_id(void)
{
    UNIMPLEMENTED_DBGBREAK();
    return (uacpi_thread_id)1;
}

void *
uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len)
{
    UNIMPLEMENTED_DBGBREAK();
    return NULL;
}

void
uacpi_kernel_unmap(void *addr, uacpi_size len)
{
    UNIMPLEMENTED_DBGBREAK();
}

uacpi_status
uacpi_kernel_get_rsdp(uacpi_phys_addr *out_rdsp_address)
{
    UNIMPLEMENTED_DBGBREAK();
    return 1;
}


uacpi_status
uacpi_kernel_install_interrupt_handler(
    uacpi_u32 irq, uacpi_interrupt_handler handler, uacpi_handle ctx,
    uacpi_handle *out_irq_handle)
{
    UNIMPLEMENTED_DBGBREAK();
    return 1;
}

uacpi_status
uacpi_kernel_uninstall_interrupt_handler(uacpi_interrupt_handler handler, uacpi_handle irq_handle)
{
    UNIMPLEMENTED_DBGBREAK();
    return 1;
}

uacpi_status
uacpi_kernel_schedule_work(uacpi_work_type type, uacpi_work_handler Handler, uacpi_handle ctx)
{
    UNIMPLEMENTED_DBGBREAK();
    return 1;
}

uacpi_status
uacpi_kernel_wait_for_work_completion(void)
{
    DPRINT("uacpi_kernel_wait_for_work_completion: Enter\n");
    return 1;
}

uacpi_status 
uacpi_kernel_pci_device_open(
    uacpi_pci_address address, uacpi_handle *out_handle
)
{
    UNIMPLEMENTED_DBGBREAK();
    return 1;
}

void
uacpi_kernel_pci_device_close(uacpi_handle Handle)
{
    UNIMPLEMENTED_DBGBREAK();
}

uacpi_status
uacpi_kernel_pci_read8(uacpi_handle device, uacpi_size offset, uacpi_u8 *Value)
{
    UNIMPLEMENTED_DBGBREAK();
    return 1;
}

uacpi_status
uacpi_kernel_pci_read16(uacpi_handle device, uacpi_size offset, uacpi_u16 *value)
{
    UNIMPLEMENTED_DBGBREAK();
    return 1;
}

uacpi_status
uacpi_kernel_pci_read32(uacpi_handle device, uacpi_size offset, uacpi_u32 *value)
{
    UNIMPLEMENTED_DBGBREAK();
    return 1;
}

uacpi_status
uacpi_kernel_pci_write8(uacpi_handle device, uacpi_size offset, uacpi_u8 value)
{
    UNIMPLEMENTED_DBGBREAK();
    return 1;
}

uacpi_status
uacpi_kernel_pci_write16(uacpi_handle device, uacpi_size offset, uacpi_u16 value)
{
    UNIMPLEMENTED_DBGBREAK();
    return 1;
}

uacpi_status
uacpi_kernel_pci_write32(uacpi_handle device, uacpi_size offset, uacpi_u32 va_list)
{
    UNIMPLEMENTED_DBGBREAK();
    return 1;
}

uacpi_status
uacpi_kernel_io_read8(uacpi_handle handle, uacpi_size offset, uacpi_u8 *out_value)
{
    UNIMPLEMENTED_DBGBREAK();
    return 1;
}

uacpi_status
uacpi_kernel_io_read16(uacpi_handle handle, uacpi_size offset, uacpi_u16 *out_value)
{
    UNIMPLEMENTED_DBGBREAK();
    return 1;
}

uacpi_status
uacpi_kernel_io_read32(uacpi_handle handle, uacpi_size offset, uacpi_u32 *out_value)
{
    UNIMPLEMENTED_DBGBREAK();
    return 1;
}

uacpi_status
uacpi_kernel_io_write8(uacpi_handle handle, uacpi_size offset, uacpi_u8 in_value)
{
    UNIMPLEMENTED_DBGBREAK();
    return 1;
}

uacpi_status
uacpi_kernel_io_write16(uacpi_handle handle, uacpi_size offset, uacpi_u16 in_value)
{
    UNIMPLEMENTED_DBGBREAK();
    return 1;
}

uacpi_status
uacpi_kernel_io_write32(uacpi_handle handle, uacpi_size offset, uacpi_u32 in_value)
{
    UNIMPLEMENTED_DBGBREAK();
    return 1;
}
