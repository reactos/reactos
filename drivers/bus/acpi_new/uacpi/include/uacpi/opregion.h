#pragma once

#include <uacpi/types.h>
#include <uacpi/status.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UACPI_BAREBONES_MODE

/*
 * Install an address space handler to a device node.
 * The handler is recursively connected to all of the operation regions of
 * type 'space' underneath 'device_node'. Note that this recursion stops as
 * soon as another device node that already has an address space handler of
 * this type installed is encountered.
 */
uacpi_status uacpi_install_address_space_handler(
    uacpi_namespace_node *device_node, enum uacpi_address_space space,
    uacpi_region_handler handler, uacpi_handle handler_context
);

/*
 * Uninstall the handler of type 'space' from a given device node.
 */
uacpi_status uacpi_uninstall_address_space_handler(
    uacpi_namespace_node *device_node,
    enum uacpi_address_space space
);

/*
 * Execute _REG(space, ACPI_REG_CONNECT) for all of the opregions with this
 * address space underneath this device. This should only be called manually
 * if you want to register an early handler that must be available before the
 * call to uacpi_namespace_initialize().
 */
uacpi_status uacpi_reg_all_opregions(
    uacpi_namespace_node *device_node,
    enum uacpi_address_space space
);

#endif // !UACPI_BAREBONES_MODE

#ifdef __cplusplus
}
#endif
