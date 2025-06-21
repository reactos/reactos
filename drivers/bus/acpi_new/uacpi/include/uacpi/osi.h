#pragma once

#include <uacpi/types.h>
#include <uacpi/status.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UACPI_BAREBONES_MODE

typedef enum uacpi_vendor_interface {
    UACPI_VENDOR_INTERFACE_NONE = 0,
    UACPI_VENDOR_INTERFACE_WINDOWS_2000,
    UACPI_VENDOR_INTERFACE_WINDOWS_XP,
    UACPI_VENDOR_INTERFACE_WINDOWS_XP_SP1,
    UACPI_VENDOR_INTERFACE_WINDOWS_SERVER_2003,
    UACPI_VENDOR_INTERFACE_WINDOWS_XP_SP2,
    UACPI_VENDOR_INTERFACE_WINDOWS_SERVER_2003_SP1,
    UACPI_VENDOR_INTERFACE_WINDOWS_VISTA,
    UACPI_VENDOR_INTERFACE_WINDOWS_SERVER_2008,
    UACPI_VENDOR_INTERFACE_WINDOWS_VISTA_SP1,
    UACPI_VENDOR_INTERFACE_WINDOWS_VISTA_SP2,
    UACPI_VENDOR_INTERFACE_WINDOWS_7,
    UACPI_VENDOR_INTERFACE_WINDOWS_8,
    UACPI_VENDOR_INTERFACE_WINDOWS_8_1,
    UACPI_VENDOR_INTERFACE_WINDOWS_10,
    UACPI_VENDOR_INTERFACE_WINDOWS_10_RS1,
    UACPI_VENDOR_INTERFACE_WINDOWS_10_RS2,
    UACPI_VENDOR_INTERFACE_WINDOWS_10_RS3,
    UACPI_VENDOR_INTERFACE_WINDOWS_10_RS4,
    UACPI_VENDOR_INTERFACE_WINDOWS_10_RS5,
    UACPI_VENDOR_INTERFACE_WINDOWS_10_19H1,
    UACPI_VENDOR_INTERFACE_WINDOWS_10_20H1,
    UACPI_VENDOR_INTERFACE_WINDOWS_11,
    UACPI_VENDOR_INTERFACE_WINDOWS_11_22H2,
} uacpi_vendor_interface;

/*
 * Returns the "latest" AML-queried _OSI vendor interface.
 *
 * E.g. for the following AML code:
 *     _OSI("Windows 2021")
 *     _OSI("Windows 2000")
 *
 * This function will return UACPI_VENDOR_INTERFACE_WINDOWS_11, since this is
 * the latest version of the interface the code queried, even though the
 * "Windows 2000" query came after "Windows 2021".
 */
uacpi_vendor_interface uacpi_latest_queried_vendor_interface(void);

typedef enum uacpi_interface_kind {
    UACPI_INTERFACE_KIND_VENDOR = (1 << 0),
    UACPI_INTERFACE_KIND_FEATURE = (1 << 1),
    UACPI_INTERFACE_KIND_ALL = UACPI_INTERFACE_KIND_VENDOR |
                               UACPI_INTERFACE_KIND_FEATURE,
} uacpi_interface_kind;

/*
 * Install or uninstall an interface.
 *
 * The interface kind is used for matching during interface enumeration in
 * uacpi_bulk_configure_interfaces().
 *
 * After installing an interface, all _OSI queries report it as supported.
 */
uacpi_status uacpi_install_interface(
    const uacpi_char *name, uacpi_interface_kind
);
uacpi_status uacpi_uninstall_interface(const uacpi_char *name);

typedef enum uacpi_host_interface {
    UACPI_HOST_INTERFACE_MODULE_DEVICE = 1,
    UACPI_HOST_INTERFACE_PROCESSOR_DEVICE,
    UACPI_HOST_INTERFACE_3_0_THERMAL_MODEL,
    UACPI_HOST_INTERFACE_3_0_SCP_EXTENSIONS,
    UACPI_HOST_INTERFACE_PROCESSOR_AGGREGATOR_DEVICE,
} uacpi_host_interface;

/*
 * Same as install/uninstall interface, but comes with an enum of known
 * interfaces defined by the ACPI specification. These are disabled by default
 * as they depend on the host kernel support.
 */
uacpi_status uacpi_enable_host_interface(uacpi_host_interface);
uacpi_status uacpi_disable_host_interface(uacpi_host_interface);

typedef uacpi_bool (*uacpi_interface_handler)
    (const uacpi_char *name, uacpi_bool supported);

/*
 * Set a custom interface query (_OSI) handler.
 *
 * This callback will be invoked for each _OSI query with the value
 * passed in the _OSI, as well as whether the interface was detected as
 * supported. The callback is able to override the return value dynamically
 * or leave it untouched if desired (e.g. if it simply wants to log something or
 * do internal bookkeeping of some kind).
 */
uacpi_status uacpi_set_interface_query_handler(uacpi_interface_handler);

typedef enum uacpi_interface_action {
    UACPI_INTERFACE_ACTION_DISABLE = 0,
    UACPI_INTERFACE_ACTION_ENABLE,
} uacpi_interface_action;

/*
 * Bulk interface configuration, used to disable or enable all interfaces that
 * match 'kind'.
 *
 * This is generally only needed to work around buggy hardware, for example if
 * requested from the kernel command line.
 *
 * By default, all vendor strings (like "Windows 2000") are enabled, and all
 * host features (like "3.0 Thermal Model") are disabled.
 */
uacpi_status uacpi_bulk_configure_interfaces(
    uacpi_interface_action action, uacpi_interface_kind kind
);

#endif // !UACPI_BAREBONES_MODE

#ifdef __cplusplus
}
#endif
