/* Written by Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   The author disclaims copyright to this source code. */
#include "log_util.h"
#include "str_util.h"

/* Simple logging. 'slog' stands for "simple logging". I figured that 'log'
   is a very common name so used something else, but still short as a prefix
   for API names */

/* TODO: slogfmt(const char *fmt, ...) */

/* TODO: extend to more than one file, by keeping a list of file names */
const TCHAR * g_cur_fileName = NULL;

/* initialize logging system, should be called before any logging calls */
BOOL slog_init(void)
{
    /* do nothing yet */
    return TRUE;
}

/* deinitialize logging system. Should be called before the program quits */
void slog_deinit(void)
{
    slog_file_log_stop(NULL);
}

/* start logging to a file 'fileName'. From now on until slog_file_log_stop()
   all slog* logging will also go to a file. If a file of that name already
   exists, it'll overwrite it. */
BOOL slog_file_log_start(const TCHAR *fileName)
{
    if (!fileName) return FALSE;
    g_cur_fileName = tstr_dup(fileName);
    DeleteFile(fileName);
    return TRUE;
}

/* like 'slog_file_log_start' but will create a unique file based on 'fileName'.
   If a 'fileName' has extension, it'll try the first available
   '$file-$NNN.$ext' file (e.g. "my-app-log-000.txt") if 'fileName' is "my-app-log.txt"
   If there is no extension, it'll be '$file-$NNN' */
int slog_file_log_unique_start(const TCHAR *fileName)
{
    assert(0); /* not implemented */
    return FALSE;
}

void slog_file_log_stop(const TCHAR *fileName)
{
    /* 'fileName' is currently unused. The idea is that it should match the
        name given to slog_file_log_start */
    if (g_cur_fileName) {
        free((void*)g_cur_fileName);
        g_cur_fileName = NULL;
    }
}

/* log 'txt' to all currently enabled loggers */
void slog_str(const char *txt)
{
    DWORD   to_write_cb;
    DWORD   written_cb;
    int     f_ok;
    HANDLE  fh;

    if (!txt) return;

    if (!g_cur_fileName) return;

    /* we're using this inefficient way of re-opening the file for each
       log so that we can also watch this file life using tail-like program */
    fh = CreateFile(g_cur_fileName, GENERIC_WRITE, FILE_SHARE_READ, NULL,
            OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,  NULL);
    if (INVALID_HANDLE_VALUE == fh)
        return;
    SetFilePointer(fh, 0, NULL, FILE_END);
    to_write_cb = (DWORD)strlen(txt);
    f_ok = WriteFile(fh, (void*)txt, to_write_cb, &written_cb, NULL);
    assert(f_ok && (written_cb == to_write_cb));
    CloseHandle(fh);
}

void slog_str_printf(const char *format, ...)
{
    char *      tmp;
    va_list     args;

    va_start(args, format);
    tmp = str_printf_args(format, args);
    va_end(args);
    if (!tmp) return;
    slog_str(tmp);
    free(tmp);
}

static WCHAR* last_error_as_wstr(void)
{
    WCHAR *msgBuf = NULL;
    WCHAR *copy;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &msgBuf, 0, NULL);
    if (!msgBuf) return NULL;
    copy = wstr_dup(msgBuf);
    LocalFree(msgBuf);
    return copy;
}

void slog_last_error(const char *optional_prefix)
{
    WCHAR *txt = last_error_as_wstr();
    if (!txt) return;
    slog_str(optional_prefix);
    slog_wstr_nl(txt);
    free(txt);
}

/* TODO: converting by casting isn't always correct but here we don't care much */
char *wstr_to_str(const WCHAR *txt)
{
    char *txt_copy, *tmp;

    if (!txt) return NULL;

    txt_copy = (char*)malloc(tstr_len(txt) + 1);
    if (!txt_copy) return NULL;

    tmp = txt_copy;
    while (*txt) {
        *tmp++ = (char)*txt++;
    }
    *tmp = 0;
    return txt_copy;
}

void slog_wstr(const WCHAR *txt)
{
    char *txt_copy;

    txt_copy = wstr_to_str(txt);
    if (!txt_copy) return;
    slog_str(txt_copy);
    free(txt_copy);
}

/* log 'txt' to all currently enabled loggers and add newline */
void slog_str_nl(const char *txt)
{
    /* TODO: given the 'reopen the file each time' implementation of
       slgotxt, this should be optimized */
    slog_str(txt);
    slog_str("\n");
}

void slog_wstr_nl(const WCHAR *txt)
{
    slog_wstr(txt);
    slog_wstr(_T("\n"));
}
