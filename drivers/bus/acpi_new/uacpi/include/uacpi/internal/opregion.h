#pragma once

#include <uacpi/internal/types.h>
#include <uacpi/opregion.h>

#ifndef UACPI_BAREBONES_MODE

uacpi_status uacpi_initialize_opregion(void);
void uacpi_deinitialize_opregion(void);

void uacpi_trace_region_error(
    uacpi_namespace_node *node, uacpi_char *message, uacpi_status ret
);

uacpi_status uacpi_install_address_space_handler_with_flags(
    uacpi_namespace_node *device_node, enum uacpi_address_space space,
    uacpi_region_handler handler, uacpi_handle handler_context,
    uacpi_u16 flags
);

void uacpi_opregion_uninstall_handler(uacpi_namespace_node *node);

uacpi_bool uacpi_address_space_handler_is_default(
    uacpi_address_space_handler *handler
);

uacpi_address_space_handlers *uacpi_node_get_address_space_handlers(
    uacpi_namespace_node *node
);

uacpi_status uacpi_initialize_opregion_node(uacpi_namespace_node *node);

uacpi_status uacpi_opregion_attach(uacpi_namespace_node *node);

void uacpi_install_default_address_space_handlers(void);

uacpi_bool uacpi_is_buffer_access_address_space(uacpi_address_space space);

union uacpi_opregion_io_data {
    uacpi_u64 *integer;
    uacpi_data_view buffer;
};

uacpi_status uacpi_dispatch_opregion_io(
    uacpi_field_unit *field, uacpi_u32 offset,
    uacpi_region_op op, union uacpi_opregion_io_data data
);

#endif // !UACPI_BAREBONES_MODE
