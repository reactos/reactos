#pragma once

#include <uacpi/internal/types.h>
#include <uacpi/acpi.h>
#include <uacpi/io.h>

#ifndef UACPI_BAREBONES_MODE

typedef struct uacpi_mapped_gas {
    uacpi_handle mapping;
    uacpi_u8 access_bit_width;
    uacpi_u8 total_bit_width;
    uacpi_u8 bit_offset;

    uacpi_status (*read)(
        uacpi_handle, uacpi_size offset, uacpi_u8 width, uacpi_u64 *out
    );
    uacpi_status (*write)(
        uacpi_handle, uacpi_size offset, uacpi_u8 width, uacpi_u64 in
    );

    void (*unmap)(uacpi_handle, uacpi_size);
} uacpi_mapped_gas;

uacpi_status uacpi_map_gas_noalloc(
    const struct acpi_gas *gas, uacpi_mapped_gas *out_mapped
);
void uacpi_unmap_gas_nofree(uacpi_mapped_gas *gas);

uacpi_size uacpi_round_up_bits_to_bytes(uacpi_size bit_length);

void uacpi_read_buffer_field(
    const uacpi_buffer_field *field, void *dst
);
void uacpi_write_buffer_field(
    uacpi_buffer_field *field, const void *src, uacpi_size size
);

uacpi_status uacpi_field_unit_get_read_type(
    struct uacpi_field_unit *field, uacpi_object_type *out_type
);

uacpi_status uacpi_field_unit_get_bit_length(
    struct uacpi_field_unit *field, uacpi_size *out_length
);

uacpi_status uacpi_read_field_unit(
    uacpi_field_unit *field, void *dst, uacpi_size size,
    uacpi_data_view *wtr_response
);
uacpi_status uacpi_write_field_unit(
    uacpi_field_unit *field, const void *src, uacpi_size size,
    uacpi_data_view *wtr_response
);

uacpi_status uacpi_system_memory_read(
    void *ptr, uacpi_size offset, uacpi_u8 width, uacpi_u64 *out
);
uacpi_status uacpi_system_memory_write(
    void *ptr, uacpi_size offset, uacpi_u8 width, uacpi_u64 in
);

uacpi_status uacpi_system_io_read(
    uacpi_handle handle, uacpi_size offset, uacpi_u8 width, uacpi_u64 *out
);
uacpi_status uacpi_system_io_write(
    uacpi_handle handle, uacpi_size offset, uacpi_u8 width, uacpi_u64 in
);

uacpi_status uacpi_pci_read(
    uacpi_handle handle, uacpi_size offset, uacpi_u8 width, uacpi_u64 *out
);
uacpi_status uacpi_pci_write(
    uacpi_handle handle, uacpi_size offset, uacpi_u8 width, uacpi_u64 in
);

#endif // !UACPI_BAREBONES_MODE
