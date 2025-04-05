#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum uacpi_log_level {
    /*
     * Super verbose logging, every op & uop being processed is logged.
     * Mostly useful for tracking down hangs/lockups.
     */
    UACPI_LOG_DEBUG = 5,

    /*
     * A little verbose, every operation region access is traced with a bit of
     * extra information on top.
     */
    UACPI_LOG_TRACE = 4,

    /*
     * Only logs the bare minimum information about state changes and/or
     * initialization progress.
     */
    UACPI_LOG_INFO = 3,

    /*
     * Logs recoverable errors and/or non-important aborts.
     */
    UACPI_LOG_WARN = 2,

    /*
     * Logs only critical errors that might affect the ability to initialize or
     * prevent stable runtime.
     */
    UACPI_LOG_ERROR = 1,
} uacpi_log_level;

#ifdef __cplusplus
}
#endif
