#pragma once

#include <uacpi/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UACPI_BAREBONES_MODE

/**
 * Install a Notify() handler to a device node.
 * A handler installed to the root node will receive all notifications, even if
 * a device already has a dedicated Notify handler.
 * 'handler_context' is passed to the handler on every invocation.
 */
uacpi_status uacpi_install_notify_handler(
    uacpi_namespace_node *node, uacpi_notify_handler handler,
    uacpi_handle handler_context
);

/**
 * Uninstall a Notify() handler previously installed to a device node.
 * The handler is guaranteed to not be running, and to never be invoked again
 * once this function returns.
 *
 * NOTE: this waits for all of the in-flight notifications to complete, and
 *       therefore must not be called from a notify handler, any other work
 *       scheduled via uacpi_kernel_schedule_work, an address space handler,
 *       or with the global lock held.
 */
uacpi_status uacpi_uninstall_notify_handler(
    uacpi_namespace_node *node, uacpi_notify_handler handler
);

#endif // !UACPI_BAREBONES_MODE

#ifdef __cplusplus
}
#endif
