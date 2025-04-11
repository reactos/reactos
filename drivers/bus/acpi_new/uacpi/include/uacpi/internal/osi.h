#pragma once

#include <uacpi/osi.h>

uacpi_status uacpi_initialize_interfaces(void);
void uacpi_deinitialize_interfaces(void);

uacpi_status uacpi_handle_osi(const uacpi_char *string, uacpi_bool *out_value);
