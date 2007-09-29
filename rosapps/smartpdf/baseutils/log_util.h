/* Written by Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   The author disclaims copyright to this source code. */
#ifndef LOG_UTIL_H_
#define LOG_UTIL_H_

#include "base_util.h"
#include "tstr_util.h"

BOOL    slog_init(void);
void    slog_deinit(void);
BOOL    slog_file_log_start(const TCHAR *fileName);
void    slog_file_log_stop(const TCHAR *fileName);
void    slog_str(const char *txt);
void    slog_str_printf(const char *format, ...);
void    slog_str_nl(const char *txt);

void    slog_last_error(const char *optional_prefix);

void    slog_wstr(const WCHAR *txt);
void    slog_wstr_nl(const WCHAR *txt);

#endif

