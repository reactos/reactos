//
// chsize.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _chsize() and _chsize_s(), which change the size of a file.
//
#include <corecrt_internal_lowio.h>
#include <corecrt_internal_ptd_propagation.h>


// Changes the size of the given file, either extending or truncating it.  The
// file must have been opened with write permissions or this will fail.  Returns
// 0 on success; returns an errno error code on failure,
static errno_t __cdecl _chsize_s_internal(int const fh, __int64 const size, __crt_cached_ptd_host& ptd)
{
    _UCRT_CHECK_FH_CLEAR_OSSERR_RETURN_ERRCODE(ptd, fh, EBADF);
    _UCRT_VALIDATE_CLEAR_OSSERR_RETURN_ERRCODE(ptd, (fh >= 0 && (unsigned)fh < (unsigned)_nhandle), EBADF);
    _UCRT_VALIDATE_CLEAR_OSSERR_RETURN_ERRCODE(ptd, (_osfile(fh) & FOPEN), EBADF);
    _UCRT_VALIDATE_CLEAR_OSSERR_RETURN_ERRCODE(ptd, (size >= 0), EINVAL);

    return __acrt_lowio_lock_fh_and_call(fh, [&]()
    {
        if (_osfile(fh) & FOPEN)
        {
            return _chsize_nolock_internal(fh, size, ptd);
        }
        else
        {
            _ASSERTE(("Invalid file descriptor. File possibly closed by a different thread", 0));
            return ptd.get_errno().set(EBADF);
        }
    });
}

extern "C" errno_t __cdecl _chsize_s(int const fh, __int64 const size)
{
    __crt_cached_ptd_host ptd;
    return _chsize_s_internal(fh, size, ptd);
}

struct __crt_seek_guard
{

    __crt_seek_guard(int const fh, __int64 const size)
        : place(_lseeki64_nolock(fh, 0, SEEK_CUR)),
        end(_lseeki64_nolock(fh, 0, SEEK_END)),
        extend(size - end),
        fhh(fh)
    {
    }

    ~__crt_seek_guard()
    {
        _lseeki64_nolock(fhh, place, SEEK_SET);
    }

    __crt_seek_guard(__crt_seek_guard const &) = delete;
    __crt_seek_guard& operator=(__crt_seek_guard const &) = delete;

    __int64 place;
    __int64 end;
    __int64 extend;
    int fhh;
};

extern "C" errno_t __cdecl _chsize_nolock_internal(int const fh, __int64 const size, __crt_cached_ptd_host& ptd)
{
    // Get current file position and seek to end
    __crt_seek_guard seek_guard(fh, size);

    if (seek_guard.place == -1 || seek_guard.end == -1)
    {
        // EBADF if earlier lseek (in __crt_seek_guard) failed
        // EINVAL otherwise (ex: too large of a offset)
        return ptd.get_errno().value_or(EINVAL);
    }

    // Grow or shrink the file as necessary:
    if (seek_guard.extend > 0)
    {
        // Extend the file by filling the new area with zeroes:
        __crt_unique_heap_ptr<char> const zero_buffer(_calloc_crt_t(char, _INTERNAL_BUFSIZ));
        if (!zero_buffer)
        {
            return ptd.get_errno().set(ENOMEM);
        }

        int const old_mode = _setmode_nolock(fh, _O_BINARY);

        do
        {
            int const bytes_to_write = seek_guard.extend >= static_cast<__int64>(_INTERNAL_BUFSIZ)
                ? _INTERNAL_BUFSIZ
                : static_cast<int>(seek_guard.extend);

            int const bytes_written = _write_nolock(fh, zero_buffer.get(), bytes_to_write, ptd);
            if (bytes_written == -1)
            {
                // Error on write:
                if (ptd.get_doserrno().check(ERROR_ACCESS_DENIED))
                {
                    ptd.get_errno().set(EACCES);
                }

                return ptd.get_errno().value_or(0);
            }

            seek_guard.extend -= bytes_written;
        } while (seek_guard.extend > 0);

#pragma warning(suppress:6031) // return value ignored
        _setmode_nolock(fh, old_mode);
    }
    else if (seek_guard.extend < 0)
    {
        // Shorten the file by truncating it:
        __int64 const new_end = _lseeki64_nolock(fh, size, SEEK_SET);
        if (new_end == -1)
        {
            return ptd.get_errno().value_or(0);
        }

        if (!SetEndOfFile(reinterpret_cast<HANDLE>(_get_osfhandle(fh))))
        {
            ptd.get_doserrno().set(GetLastError());
            return ptd.get_errno().set(EACCES);
        }
    }

    return 0;
}

extern "C" errno_t __cdecl _chsize_nolock(int const fh, __int64 const size)
{   // TODO: _chsize_nolock is already internal-only.
    // Once PTD is propagated everywhere, rename _chsize_nolock_internal to _chsize_nolock.
    __crt_cached_ptd_host ptd;
    return _chsize_nolock_internal(fh, size, ptd);
}


// Changes the size of the given file, either extending or truncating it.  The
// file must have been opened with write permissions or this will fail.  Returns
// 0 on success; returns -1 and sets errno on failure.
extern "C" int __cdecl _chsize(int const fh, long const size)
{
    return _chsize_s(fh, size) == 0 ? 0 : -1;
}
