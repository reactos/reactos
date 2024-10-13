//
// _file.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines the _iob array, the global __piob pointer, the stdio initialization
// and termination code, and the stream locking routines.
//
#include <corecrt_internal_stdio.h>



// FILE descriptors for stdin, stdout, and stderr
extern "C" { __crt_stdio_stream_data _iob[_IOB_ENTRIES] =
{
    // ptr      _base,   _cnt, _flag,                   _file, _charbuf, _bufsiz
    {  nullptr, nullptr, 0,    _IOALLOCATED | _IOREAD,  0,     0,        0}, // stdin
    {  nullptr, nullptr, 0,    _IOALLOCATED | _IOWRITE, 1,     0,        0}, // stdout
    {  nullptr, nullptr, 0,    _IOALLOCATED | _IOWRITE, 2,     0,        0}, // stderr
}; }

extern "C" FILE* __cdecl __acrt_iob_func(unsigned const id)
{
    return &_iob[id]._public_file;
}



// Pointer to the array of FILE objects:
__crt_stdio_stream_data** __piob;



// Number of open streams (set to _NSTREAM by default):
#ifdef CRTDLL
    extern "C" { int _nstream = _NSTREAM_; }
#else
    extern "C" { int _nstream; }
#endif



// Initializer and terminator for the stdio library:
_CRT_LINKER_FORCE_INCLUDE(__acrt_stdio_initializer);
_CRT_LINKER_FORCE_INCLUDE(__acrt_stdio_terminator);



#ifndef CRTDLL
    // This variable is used by the statically linked CRT to ensure that if any
    // stdio functionality is used, the terminate_stdio() function will be
    // registered for call during CRT termination.
    extern "C" { int _cflush = 0; }
#endif



// Initializes the stdio library.  Returns 0 on success; -1 on failure.
extern "C" int __cdecl __acrt_initialize_stdio()
{
    #ifndef CRTDLL
    // If the user has not supplied a definition of _nstream, set it to _NSTREAM_.
    // If the user has supplied a value that is too small, set _nstream to the
    // minimum acceptable value (_IOB_ENTRIES):
    if (_nstream == 0)
    {
        _nstream = _NSTREAM_;
    }
    else if (_nstream < _IOB_ENTRIES)
    {
        _nstream = _IOB_ENTRIES;
    }
    #endif

    // Allocate the __piob array.  Try for _nstream entries first.  If this
    // fails, then reset _nstream to _IOB_ENTRIES an try again.  If it still
    // fails, bail out and cause CRT initialization to fail:
    __piob = _calloc_crt_t(__crt_stdio_stream_data*, _nstream).detach();
    if (!__piob)
    {
        _nstream = _IOB_ENTRIES;

        __piob = _calloc_crt_t(__crt_stdio_stream_data*, _nstream).detach();
        if (!__piob)
        {
            return -1;
        }
    }

    // Initialize the first _IOB_ENTRIES to point to the corresponding entries
    // in _iob[]:
    for (int i = 0; i != _IOB_ENTRIES; ++i)
    {
        __acrt_InitializeCriticalSectionEx(&_iob[i]._lock, _CORECRT_SPINCOUNT, 0);
        __piob[i] = &_iob[i];

        // For stdin, stdout, and stderr, we use _NO_CONSOLE_FILENO to allow
        // callers to distinguish between failure to open a file (-1) and a
        // program run without a console.
        intptr_t const os_handle = _osfhnd(i);
        bool const has_no_console =
            os_handle == reinterpret_cast<intptr_t>(INVALID_HANDLE_VALUE) ||
            os_handle == _NO_CONSOLE_FILENO ||
            os_handle == 0;

        if (has_no_console)
        {
            _iob[i]._file = _NO_CONSOLE_FILENO;
        }
    }

    return 0;
}



// Terminates the stdio library.  All streams are flushed (this is done even if
// we are going to close them since that stream won't do anything to the standard
// streams) and closed.
extern "C" void __cdecl __acrt_uninitialize_stdio()
{
    _flushall();
    _fcloseall();

    for (int i = 0; i != _IOB_ENTRIES; ++i)
    {
        __acrt_stdio_free_buffer_nolock(&__piob[i]->_public_file);
        DeleteCriticalSection(&__piob[i]->_lock);
    }

    _free_crt(__piob);
    __piob = nullptr;
}



// Locks a stdio stream.
extern "C" void __cdecl _lock_file(FILE* const stream)
{
    EnterCriticalSection(&__crt_stdio_stream(stream)->_lock);
}



// Unlocks a stdio stream.
extern "C" void __cdecl _unlock_file(FILE* const stream)
{
    LeaveCriticalSection(&__crt_stdio_stream(stream)->_lock);
}



extern "C" errno_t __cdecl _get_stream_buffer_pointers(
    FILE*   const public_stream,
    char*** const base,
    char*** const ptr,
    int**   const count
    )
{
    _VALIDATE_RETURN_ERRCODE(public_stream != nullptr, EINVAL);

    __crt_stdio_stream const stream(public_stream);
    if (base)
    {
        *base = &stream->_base;
    }

    if (ptr)
    {
        *ptr = &stream->_ptr;
    }

    if (count)
    {
        *count = &stream->_cnt;
    }

    return 0;
}
