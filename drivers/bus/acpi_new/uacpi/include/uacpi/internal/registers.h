#pragma once

#include <uacpi/types.h>
#include <uacpi/registers.h>

uacpi_status uacpi_initialize_registers(void);
void uacpi_deinitialize_registers(void);
