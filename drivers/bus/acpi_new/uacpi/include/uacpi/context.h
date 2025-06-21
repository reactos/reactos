#pragma once

#include <uacpi/types.h>
#include <uacpi/log.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Set the minimum log level to be accepted by the logging facilities. Any logs
 * below this level are discarded and not passed to uacpi_kernel_log, etc.
 *
 * 0 is treated as a special value that resets the setting to the default value.
 *
 * E.g. for a log level of UACPI_LOG_INFO:
 *   UACPI_LOG_DEBUG -> discarded
 *   UACPI_LOG_TRACE -> discarded
 *   UACPI_LOG_INFO -> allowed
 *   UACPI_LOG_WARN -> allowed
 *   UACPI_LOG_ERROR -> allowed
 */
void uacpi_context_set_log_level(uacpi_log_level);

/*
 * Enables table checksum validation at installation time instead of first use.
 * Note that this makes uACPI map the entire table at once, which not all
 * hosts are able to handle at early init.
 */
void uacpi_context_set_proactive_table_checksum(uacpi_bool);

#ifndef UACPI_BAREBONES_MODE
/*
 * Set the maximum number of seconds a While loop is allowed to run for before
 * getting timed out.
 *
 * 0 is treated a special value that resets the setting to the default value.
 */
void uacpi_context_set_loop_timeout(uacpi_u32 seconds);

/*
 * Set the maximum call stack depth AML can reach before getting aborted.
 *
 * 0 is treated as a special value that resets the setting to the default value.
 */
void uacpi_context_set_max_call_stack_depth(uacpi_u32 depth);

uacpi_u32 uacpi_context_get_loop_timeout(void);
#endif // !UACPI_BAREBONES_MODE

#ifdef __cplusplus
}
#endif
