//
// osfinfo.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines the functions used to control allocation, locking, and freeing of CRT
// file handles.
//
#include <corecrt_internal_lowio.h>



extern "C" __crt_lowio_handle_data* __cdecl __acrt_lowio_create_handle_array()
{
    __crt_unique_heap_ptr<__crt_lowio_handle_data> array(_calloc_crt_t(
        __crt_lowio_handle_data,
        IOINFO_ARRAY_ELTS));

    if (!array)
        return nullptr;

    __crt_lowio_handle_data* const first = array.get();
    __crt_lowio_handle_data* const last  = first + IOINFO_ARRAY_ELTS;
    for (auto it = first; it != last; ++it)
    {
        __acrt_InitializeCriticalSectionEx(&it->lock, _CORECRT_SPINCOUNT, 0);
        it->osfhnd             = reinterpret_cast<intptr_t>(INVALID_HANDLE_VALUE);
        it->startpos           = 0;
        it->osfile             = 0;
        it->textmode           = __crt_lowio_text_mode::ansi;
        it->_pipe_lookahead[0] = LF;
        it->_pipe_lookahead[1] = LF;
        it->_pipe_lookahead[2] = LF;
        it->unicode            = false;
        it->utf8translations   = false;
        it->dbcsBufferUsed     = false;
        for (int i = 0; i < sizeof(it->mbBuffer); ++i)
        {
            it->mbBuffer[i] = '\0';
        }
    }

    return array.detach();
}

extern "C" void __cdecl __acrt_lowio_destroy_handle_array(__crt_lowio_handle_data* const array)
{
    if (!array)
        return;

    __crt_lowio_handle_data* const first = array;
    __crt_lowio_handle_data* const last  = first + IOINFO_ARRAY_ELTS;
    for (auto it = first; it != last; ++it)
    {
        DeleteCriticalSection(&it->lock);
    }

    _free_crt(array);
}

// Ensures that a lowio handle data object has been created for file handle 'fh'.
// The 'fh' must be less than the hard maximum, _NHANDLE_.  If 'fh' is already
// backed by a handle data object, this function has no effect.  Otherwise, this
// function extends the global arrays of handle data objects until 'fh' is backed
// by a handle data object. 
extern "C" errno_t __cdecl __acrt_lowio_ensure_fh_exists(int const fh)
{
    _VALIDATE_RETURN_ERRCODE(static_cast<unsigned>(fh) < _NHANDLE_, EBADF);

    errno_t status = 0;

    __acrt_lock(__acrt_lowio_index_lock);
    __try
    {
        for (size_t i = 0; fh >= _nhandle; ++i)
        {
            if (__pioinfo[i])
            {
                continue;
            }

            __pioinfo[i] = __acrt_lowio_create_handle_array();
            if (!__pioinfo[i])
            {
                status = ENOMEM;
                __leave;
            }

            _nhandle += IOINFO_ARRAY_ELTS;
        }
    }
    __finally
    {
        __acrt_unlock(__acrt_lowio_index_lock);
    }
    __endtry

    return status;
}



// Allocates a CRT file handle.  This function finds the first free entry in
// the arrays of file objects and returns the index of that entry (that index
// is the CRT file handle) to the caller.  The FOPEN flag is set in the new
// entry, to pevent multithreaded race conditions and deadlocks.
//
// Returns the CRT file handle on success; returns -1 on failure (e.g. if no
// more file handles are available or if memory allocation is required but
// fails).
//
// MULTITHREADING NOTE:  If this function is successful and returns a CRT file
// handle, the handle is locked when it is returned and the FOPEN flag has been
// set.  The caller must be sure to release the lock, and if the caller abandons
// the file handle, it must clear the FOPEN flag to free the handle.
extern "C" int __cdecl _alloc_osfhnd()
{
    __acrt_lock(__acrt_lowio_index_lock);
    int result = -1;
    __try
    {
        // Search the arrays of file objects, in order, looking for the first
        // free entry.  The compound index of this free entry is the return
        // value.
        //
        // The compound index of the file object entry *(__pioinfo[i] + j) is
        // k = i * IOINFO_ARRAY_ELTS + j.
        for (int i = 0; i < IOINFO_ARRAYS; ++i)
        {
            // If this __crt_lowio_handle_data array does not yet exist, create a new one:
            if (!__pioinfo[i])
            {
                __pioinfo[i] = __acrt_lowio_create_handle_array();
                if (!__pioinfo[i])
                    __leave;

                _nhandle += IOINFO_ARRAY_ELTS;

                // The first element of the newly allocated array of handle data
                // objects is our first free entry.  Note that since we hold the
                // index lock, no one else can allocate this handle.
                int const fh = i * IOINFO_ARRAY_ELTS;

                __acrt_lowio_lock_fh(fh);
                _osfile(fh) = FOPEN;
                result = fh;
                __leave;
            }

            // Otherwise, this file object array already exists.  Search it looking
            // for the first free entry:
            __crt_lowio_handle_data* const first = __pioinfo[i];
            __crt_lowio_handle_data* const last  = first + IOINFO_ARRAY_ELTS;
            for (__crt_lowio_handle_data* pio = first; pio != last; ++pio)
            {
                if (pio->osfile & FOPEN)
                    continue;

                // Another thread may have grabbed this file handle out from
                // under us while we waited for the lock.  If so, continue on
                // searching through the array.
                //
                // CRT_REFACTOR TODO:  Resolve lowio synchronization issues.
                EnterCriticalSection(&pio->lock);
                if ((pio->osfile & FOPEN) != 0)
                {
                    LeaveCriticalSection(&pio->lock);
                    continue;
                }

                // Otherwise, this entry is ours:  we hold the lock, so we can
                // initialize it and return its handle:
                int const fh = i * IOINFO_ARRAY_ELTS + static_cast<int>(pio - first);
                _osfile(fh) = FOPEN;
                _osfhnd(fh) = reinterpret_cast<intptr_t>(INVALID_HANDLE_VALUE);
                result = fh;
                __leave;
            }
        }

        // All entries are in use if we fall out of the loop.  return -1 in this case (which result is already set to)
    }
    __finally
    {
        __acrt_unlock(__acrt_lowio_index_lock);
    }
    __endtry
    return result;
}



// Sets the Win32 HANDLE associated with the specified CRT file.  Returns 0
// on success; returns -1 and sets errno on failure.
extern "C" int __cdecl __acrt_lowio_set_os_handle(int const fh, intptr_t const value)
{
    if (fh >= 0 &&
        static_cast<unsigned>(fh) < static_cast<unsigned>(_nhandle) &&
        _osfhnd(fh) == reinterpret_cast<intptr_t>(INVALID_HANDLE_VALUE))
    {
        if (_query_app_type() == _crt_console_app)
        {
            HANDLE const handle_value = reinterpret_cast<HANDLE>(value);
            switch (fh)
            {
            case 0: SetStdHandle(STD_INPUT_HANDLE,  handle_value); break;
            case 1: SetStdHandle(STD_OUTPUT_HANDLE, handle_value); break;
            case 2: SetStdHandle(STD_ERROR_HANDLE,  handle_value); break;
            }
        }

        _osfhnd(fh) = value;
        return 0 ;
    }
    else
    {
        errno = EBADF; // Bad handle
        _doserrno = 0; // This is not an OS error
        return -1;
    }
}



// Marks the specified CRT file handle as free and available for allocation.
// Returns 0 on success; returns -1 and sets errno on failure.
extern "C" int __cdecl _free_osfhnd(int const fh)
{
    if (fh >= 0 &&
        static_cast<unsigned>(fh) < static_cast<unsigned>(_nhandle) &&
        (_osfile(fh) & FOPEN) &&
        _osfhnd(fh) != reinterpret_cast<intptr_t>(INVALID_HANDLE_VALUE))
    {
        if (_query_app_type() == _crt_console_app)
        {
            switch (fh)
            {
            case 0: SetStdHandle(STD_INPUT_HANDLE,  nullptr); break;
            case 1: SetStdHandle(STD_OUTPUT_HANDLE, nullptr); break;
            case 2: SetStdHandle(STD_ERROR_HANDLE,  nullptr); break;
            }
        }

        _osfhnd(fh) = reinterpret_cast<intptr_t>(INVALID_HANDLE_VALUE);
        return 0;
    }
    else
    {
        errno = EBADF; // Bad handle
        _doserrno = 0; // This is not an OS error
        return -1;
    }
}



// Gets the Win32 HANDLE with which the given CRT file handle is associated.
// On success, returns the Win32 HANDLE; on failure, returns -1 and sets errno.
extern "C" intptr_t __cdecl _get_osfhandle(int const fh)
{
    _CHECK_FH_CLEAR_OSSERR_RETURN(fh, EBADF, -1 );
    _VALIDATE_CLEAR_OSSERR_RETURN(fh >= 0 && (unsigned)fh < (unsigned)_nhandle, EBADF, -1);
    _VALIDATE_CLEAR_OSSERR_RETURN(_osfile(fh) & FOPEN, EBADF, -1);

    return _osfhnd(fh);
}



// Allocates a free CRT file handle and associates it with the provided Win32
// HANDLE and sets its flags to the given flags.  Returns the CRT file handle
// on success; returns -1 on failure.
extern "C" int __cdecl _open_osfhandle(intptr_t const osfhandle, int const source_flags)
{
    // Copy relevant source_flags from second parameter
    unsigned char file_flags = 0;

    if (source_flags & _O_APPEND)
        file_flags |= FAPPEND;

    if (source_flags & _O_TEXT)
        file_flags |= FTEXT;

    if (source_flags & _O_NOINHERIT)
        file_flags |= FNOINHERIT;

    // Find out what type of file (file/device/pipe):
    DWORD const file_type = GetFileType(reinterpret_cast<HANDLE>(osfhandle));
    if (file_type == FILE_TYPE_UNKNOWN)
    {
        __acrt_errno_map_os_error(GetLastError());
        return -1;
    }

    if (file_type == FILE_TYPE_CHAR)
        file_flags |= FDEV;

    else if (file_type == FILE_TYPE_PIPE)
        file_flags |= FPIPE;

    // Attempt to allocate a CRT file handle:
    int const fh = _alloc_osfhnd();
    if (fh == -1)
    {
        errno = EMFILE; // Too many open files
        _doserrno = 0L; // This is not an OS error
        return -1;
    }

    bool success = false;
    __try
    {
        // The file is open. now set the info in _osfhnd array:
        __acrt_lowio_set_os_handle(fh, osfhandle);

        file_flags |= FOPEN;

        _osfile(fh) = file_flags;
        _textmode(fh) = __crt_lowio_text_mode::ansi;
        _tm_unicode(fh) = false;

        success = true;
    }
    __finally
    {
        if (!success)
        {
            _osfile(fh) &= ~FOPEN;
        }

        __acrt_lowio_unlock_fh(fh);
    }
    __endtry
    return fh;
}



// Acquires the lock associated with the given file handle.
extern "C" void __cdecl __acrt_lowio_lock_fh(int const fh)
{
    EnterCriticalSection(&_pioinfo(fh)->lock);
}



// Releases the lock associated with the given file handle.
extern "C" void __cdecl __acrt_lowio_unlock_fh(int const fh)
{
    LeaveCriticalSection(&_pioinfo(fh)->lock);
}
