/*
 * msvcrt.dll errno functions
 *
 * Copyright 2000 Jon Griffiths
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <precomp.h>

char __syserr00[] = "No Error";
char __syserr01[] = "Operation not permitted (EPERM)";
char __syserr02[] = "No such file or directory (ENOENT)";
char __syserr03[] = "No such process (ESRCH)";
char __syserr04[] = "Interrupted system call (EINTR)";
char __syserr05[] = "Input or output error (EIO)";
char __syserr06[] = "No such device or address (ENXIO)";
char __syserr07[] = "Argument list too long (E2BIG)";
char __syserr08[] = "Unable to execute file (ENOEXEC)";
char __syserr09[] = "Bad file descriptor (EBADF)";
char __syserr10[] = "No child processes (ECHILD)";
char __syserr11[] = "Resource temporarily unavailable (EAGAIN)";
char __syserr12[] = "Not enough memory (ENOMEM)";
char __syserr13[] = "Permission denied (EACCES)";
char __syserr14[] = "Bad address (EFAULT)";
char __syserr15[] = "Unknown Error: 15";
char __syserr16[] = "Resource busy (EBUSY)";
char __syserr17[] = "File exists (EEXIST)";
char __syserr18[] = "Improper link (EXDEV)";
char __syserr19[] = "No such device (ENODEV)";
char __syserr20[] = "Not a directory (ENOTDIR)";
char __syserr21[] = "Is a directory (EISDIR)";
char __syserr22[] = "Invalid argument (EINVAL)";
char __syserr23[] = "Too many open files in system (ENFILE)";
char __syserr24[] = "Too many open files (EMFILE)";
char __syserr25[] = "Inappropriate I/O control operation (ENOTTY)";
char __syserr26[] = "Unknown error: 26";
char __syserr27[] = "File too large (EFBIG)";
char __syserr28[] = "No space left on drive (ENOSPC)";
char __syserr29[] = "Invalid seek (ESPIPE)";
char __syserr30[] = "Read-only file system (EROFS)";
char __syserr31[] = "Too many links (EMLINK)";
char __syserr32[] = "Broken pipe (EPIPE)";
char __syserr33[] = "Input to function out of range (EDOM)";
char __syserr34[] = "Output of function out of range (ERANGE)";
char __syserr35[] = "Unknown error: 35";
char __syserr36[] = "Resource deadlock avoided (EDEADLK)";
char __syserr37[] = "Unknown error: 37";
char __syserr38[] = "File name too long (ENAMETOOLONG)";
char __syserr39[] = "No locks available (ENOLCK)";
char __syserr40[] = "Function not implemented (ENOSYS)";
char __syserr41[] = "Directory not empty (ENOTEMPTY)";
char __syserr42[] = "Illegal byte sequence (EILSEQ)";

char *_sys_errlist[] = {
__syserr00, __syserr01, __syserr02, __syserr03, __syserr04,
__syserr05, __syserr06, __syserr07, __syserr08, __syserr09,
__syserr10, __syserr11, __syserr12, __syserr13, __syserr14,
__syserr15, __syserr16, __syserr17, __syserr18, __syserr19,
__syserr20, __syserr21, __syserr22, __syserr23, __syserr24,
__syserr25, __syserr26, __syserr27, __syserr28, __syserr29,
__syserr30, __syserr31, __syserr32, __syserr33, __syserr34,
__syserr35, __syserr36, __syserr37, __syserr38, __syserr39,
__syserr40, __syserr41, __syserr42
};

int _sys_nerr = sizeof(_sys_errlist) / sizeof(_sys_errlist[0]) - 1;

/*********************************************************************
 *		strerror (MSVCRT.@)
 */
char* CDECL strerror(int err)
{
    thread_data_t *data = msvcrt_get_thread_data();

    if (!data->strerror_buffer)
        if (!(data->strerror_buffer = malloc(256))) return NULL;

    if (err < 0 || err > _sys_nerr) err = _sys_nerr;
    strcpy( data->strerror_buffer, _sys_errlist[err] );
    return data->strerror_buffer;
}

/**********************************************************************
 *		strerror_s	(MSVCRT.@)
 */
int CDECL strerror_s(char *buffer, size_t numberOfElements, int errnum)
{
    char *ptr;

    if (!buffer || !numberOfElements)
    {
        *_errno() = EINVAL;
        return EINVAL;
    }

    if (errnum < 0 || errnum > _sys_nerr)
        errnum = _sys_nerr;

    ptr = _sys_errlist[errnum];
    while (*ptr && numberOfElements > 1)
    {
        *buffer++ = *ptr++;
        numberOfElements--;
    }

    *buffer = '\0';
    return 0;
}

/**********************************************************************
 *		_strerror	(MSVCRT.@)
 */
char* CDECL _strerror(const char* str)
{
    thread_data_t *data = msvcrt_get_thread_data();
    int err;

    if (!data->strerror_buffer)
        if (!(data->strerror_buffer = malloc(256))) return NULL;

    err = data->thread_errno;
    if (err < 0 || err > _sys_nerr) err = _sys_nerr;

    if (str && *str)
        sprintf( data->strerror_buffer, "%s: %s\n", str, _sys_errlist[err] );
    else
        sprintf( data->strerror_buffer, "%s\n", _sys_errlist[err] );

    return data->strerror_buffer;
}

/*********************************************************************
 *		perror (MSVCRT.@)
 */
void CDECL perror(const char* str)
{
    int err = *_errno();
    if (err < 0 || err > _sys_nerr) err = _sys_nerr;

    if (str && *str)
    {
        _write( 2, str, (unsigned int)strlen(str) );
        _write( 2, ": ", 2 );
    }
    _write( 2, _sys_errlist[err], (unsigned int)strlen(_sys_errlist[err]) );
    _write( 2, "\n", 1 );
}

/*********************************************************************
 *		_wcserror_s (MSVCRT.@)
 */
int CDECL _wcserror_s(wchar_t* buffer, size_t nc, int err)
{
    if (!MSVCRT_CHECK_PMT(buffer != NULL) || !MSVCRT_CHECK_PMT(nc > 0))
    {
        _set_errno(EINVAL);
        return EINVAL;
    }
    if (err < 0 || err > _sys_nerr) err = _sys_nerr;
    MultiByteToWideChar(CP_ACP, 0, _sys_errlist[err], -1, buffer, (int)nc);
    return 0;
}

/*********************************************************************
 *		_wcserror (MSVCRT.@)
 */
wchar_t* CDECL _wcserror(int err)
{
    thread_data_t *data = msvcrt_get_thread_data();

    if (!data->wcserror_buffer)
        if (!(data->wcserror_buffer = malloc(256 * sizeof(wchar_t)))) return NULL;
    _wcserror_s(data->wcserror_buffer, 256, err);
    return data->wcserror_buffer;
}

/**********************************************************************
 *		__wcserror_s	(MSVCRT.@)
 */
int CDECL __wcserror_s(wchar_t* buffer, size_t nc, const wchar_t* str)
{
    int err;
    static const WCHAR colonW[] = {':', ' ', '\0'};
    static const WCHAR nlW[] = {'\n', '\0'};
    size_t len;

    err = *_errno();
    if (err < 0 || err > _sys_nerr) err = _sys_nerr;

    len = MultiByteToWideChar(CP_ACP, 0, _sys_errlist[err], -1, NULL, 0) + 1 /* \n */;
    if (str && *str) len += lstrlenW(str) + 2 /* ': ' */;
    if (len > nc)
    {
        MSVCRT_INVALID_PMT("buffer[nc] is too small", ERANGE);
        return ERANGE;
    }
    if (str && *str)
    {
        lstrcpyW(buffer, str);
        lstrcatW(buffer, colonW);
    }
    else buffer[0] = '\0';
    len = lstrlenW(buffer);
    MultiByteToWideChar(CP_ACP, 0, _sys_errlist[err], -1, buffer + len, (int)(256 - len));
    lstrcatW(buffer, nlW);

    return 0;
}

/**********************************************************************
 *		__wcserror	(MSVCRT.@)
 */
wchar_t* CDECL __wcserror(const wchar_t* str)
{
    thread_data_t *data = msvcrt_get_thread_data();
    int err;

    if (!data->wcserror_buffer)
        if (!(data->wcserror_buffer = malloc(256 * sizeof(wchar_t)))) return NULL;

    err = __wcserror_s(data->wcserror_buffer, 256, str);
    if (err) FIXME("bad wcserror call (%d)\n", err);

    return data->wcserror_buffer;
}
