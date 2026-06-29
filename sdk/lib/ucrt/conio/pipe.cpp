//
// pipe.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _pipe(), which creates a pipe.
//
#include <corecrt_internal_lowio.h>
#include <stdlib.h>



// Creates a pipe.  The phandles pointer must be a pointer to an array of two
// int objects.  Upon successful return, phandles[0] is the read handle, and
// phandles[1] is the write handle for the pipe.  On success, 0 is returned; on
// failure, -1 is returned and errno is set.
//
// The psize is the amount of memory, in bytes, to ask the OS to reserve for the
// pipe.  The textmode is the text mode that will be given to the pipe.
extern "C" int __cdecl _pipe(int* const phandles, unsigned const psize, int const textmode)
{
    _VALIDATE_CLEAR_OSSERR_RETURN(phandles != nullptr, EINVAL, -1);
    phandles[0] = phandles[1] = -1;

    _VALIDATE_CLEAR_OSSERR_RETURN((textmode & ~(_O_NOINHERIT | _O_BINARY | _O_TEXT)) == 0, EINVAL, -1);
    _VALIDATE_CLEAR_OSSERR_RETURN((textmode & (_O_BINARY | _O_TEXT)) != (_O_BINARY | _O_TEXT), EINVAL, -1);


    SECURITY_ATTRIBUTES security_attributes;
    security_attributes.nLength = sizeof(security_attributes);
    security_attributes.lpSecurityDescriptor = nullptr;
    security_attributes.bInheritHandle = (textmode & _O_NOINHERIT) == 0;


    // Create the pipe:
    HANDLE read_handle, write_handle;
    if (!CreatePipe(&read_handle, &write_handle, &security_attributes, psize))
    {
        __acrt_errno_map_os_error(GetLastError());
        return -1;
    }

    // Create the CRT read handle for the pipe:
    int const crt_read_handle = _alloc_osfhnd();
    if (crt_read_handle == -1)
    {
        errno = EMFILE;
        _doserrno = 0;
        CloseHandle(read_handle);
        CloseHandle(write_handle);
        return -1;
    }

    __try
    {
        _osfile(crt_read_handle)     = FOPEN | FPIPE | FTEXT;
        _textmode(crt_read_handle)   = __crt_lowio_text_mode::ansi;
        _tm_unicode(crt_read_handle) = false;
    }
    __finally
    {
        __acrt_lowio_unlock_fh(crt_read_handle);
    }
    __endtry

    // Create the CRT write handle for the pipe:
    int const crt_write_handle = _alloc_osfhnd();
    if (crt_write_handle == -1)
    {
        _osfile(crt_read_handle) = 0;
        errno = EMFILE;
        _doserrno = 0;
        CloseHandle(read_handle);
        CloseHandle(write_handle);
        return -1;
    }

    __try
    {
        _osfile(crt_write_handle)     = FOPEN | FPIPE | FTEXT;
        _textmode(crt_write_handle)   = __crt_lowio_text_mode::ansi;
        _tm_unicode(crt_write_handle) = false;
    }
    __finally
    {
        __acrt_lowio_unlock_fh(crt_write_handle);
    }
    __endtry

    // Figure out which textmode the file gets:
    int fmode = 0;
    _ERRCHECK(_get_fmode(&fmode));

    // The pipe gets binary mode if (a) binary was requested or (b) the
    // global default fmode was changed to binary.
    if ((textmode & _O_BINARY) || ((textmode & _O_TEXT) == 0 && fmode == _O_BINARY))
    {
        _osfile(crt_read_handle)  &= ~FTEXT;
        _osfile(crt_write_handle) &= ~FTEXT;
    }

    if (textmode & _O_NOINHERIT)
    {
        _osfile(crt_read_handle)  |= FNOINHERIT;
        _osfile(crt_write_handle) |= FNOINHERIT;
    }

    __acrt_lowio_set_os_handle(crt_read_handle,  reinterpret_cast<intptr_t>(read_handle));
    __acrt_lowio_set_os_handle(crt_write_handle, reinterpret_cast<intptr_t>(write_handle));

    phandles[0] = crt_read_handle;
    phandles[1] = crt_write_handle;

    return 0;
}
