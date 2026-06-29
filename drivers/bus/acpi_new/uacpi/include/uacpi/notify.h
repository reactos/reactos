#pragma once

#include <uacpi/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UACPI_BAREBONES_MODE

/*
 * Install a Notify() handler to a device node.
 * A handler installed to the root node will receive all notifications, even if
 * a device already has a dedicated Notify handler.
 * 'handler_context' is passed to the handler on every invocation.
 */
uacpi_status uacpi_install_notify_handler(
    uacpi_namespace_node *node, uacpi_notify_handler handler,
    uacpi_handle handler_context
);

uacpi_status uacpi_uninstall_notify_handler(
    uacpi_namespace_node *node, uacpi_notify_handler handler
);

#endif // !UACPI_BAREBONES_MODE

#ifdef __cplusplus
}
#endif
