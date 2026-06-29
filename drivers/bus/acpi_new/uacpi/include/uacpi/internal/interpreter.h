#pragma once

#include <uacpi/types.h>
#include <uacpi/status.h>
#include <uacpi/internal/namespace.h>

#ifndef UACPI_BAREBONES_MODE

enum uacpi_table_load_cause {
    UACPI_TABLE_LOAD_CAUSE_LOAD_OP,
    UACPI_TABLE_LOAD_CAUSE_LOAD_TABLE_OP,
    UACPI_TABLE_LOAD_CAUSE_INIT,
    UACPI_TABLE_LOAD_CAUSE_HOST,
};

uacpi_status uacpi_execute_table(void*, enum uacpi_table_load_cause cause);
uacpi_status uacpi_osi(uacpi_handle handle, uacpi_object *retval);

uacpi_status uacpi_execute_control_method(
    uacpi_namespace_node *scope, uacpi_control_method *method,
    const uacpi_object_array *args, uacpi_object **ret
);

#endif // !UACPI_BAREBONES_MODE
