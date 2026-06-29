#pragma once

#include <uacpi/internal/types.h>
#include <uacpi/notify.h>

#ifndef UACPI_BAREBONES_MODE

uacpi_status uacpi_initialize_notify(void);
void uacpi_deinitialize_notify(void);

uacpi_status uacpi_notify_all(uacpi_namespace_node *node, uacpi_u64 value);

#endif // !UACPI_BAREBONES_MODE
