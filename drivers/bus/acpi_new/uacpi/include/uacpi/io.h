#pragma once

#include <uacpi/types.h>
#include <uacpi/acpi.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UACPI_BAREBONES_MODE

uacpi_status uacpi_gas_read(const struct acpi_gas *gas, uacpi_u64 *value);
uacpi_status uacpi_gas_write(const struct acpi_gas *gas, uacpi_u64 value);

typedef struct uacpi_mapped_gas uacpi_mapped_gas;

/*
 * Map a GAS for faster access in the future. The handle returned via
 * 'out_mapped' must be freed & unmapped using uacpi_unmap_gas() when
 * no longer needed.
 */
uacpi_status uacpi_map_gas(const struct acpi_gas *gas, uacpi_mapped_gas **out_mapped);
void uacpi_unmap_gas(uacpi_mapped_gas*);

/*
 * Same as uacpi_gas_{read,write} but operates on a pre-mapped handle for faster
 * access and/or ability to use in critical sections/irq contexts.
 */
uacpi_status uacpi_gas_read_mapped(const uacpi_mapped_gas *gas, uacpi_u64 *value);
uacpi_status uacpi_gas_write_mapped(const uacpi_mapped_gas *gas, uacpi_u64 value);

#endif // !UACPI_BAREBONES_MODE

#ifdef __cplusplus
}
#endif
