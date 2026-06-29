//
// corecrt_internal_stdio.h
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// This internal header defines internal utilities for working with the stdio
// library.
//
#pragma once

#include <corecrt_internal.h>
#include <corecrt_internal_lowio.h>
#include <corecrt_internal_traits.h>
#include <io.h>
#include <mbstring.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push, _CRT_PACKING)



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Stream State Flags
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
enum : long
{
    // Mode bits:  These bits control the stream mode.  A stream may be in one
    // of three modes:  read mode, write mode, or update (read/write) mode.  At
    // least one of these bits will be set for any open stream.
    //
    // If the stream is open in read mode or write mode, then only the _IOREAD
    // or _IOWRITE bit will be set.
    //
    // If the stream is open in update (read/write) mode, then the _IOUPDATE bit
    // will be set.  Further state must also be tracked for update mode streams.
    // Read and write operations cannot be mixed willy-nilly:  in most cases, a
    // flush or reposition must take place in order to transition between reading
    // and writing.  So, for update mode streams, if the next operation must be
    // a read, the _IOREAD bit is set, and if the next operation must be a write,
    // the _IOWRITE bit is set.
    _IOREAD           = 0x0001,
    _IOWRITE          = 0x0002,
    _IOUPDATE         = 0x0004,

    // Stream state bits:  These bits track the state of the stream.  The _IOEOF
    // and _IOERROR flags track the end-of-file and error states, respectively,
    // which are reported by feof() and ferror().  The _IOCTRLZ flag is when the
    // last read ended because a Ctrl+Z was read; it corresponds to the lowio
    // FEOFLAG state.
    _IOEOF            = 0x0008,
    _IOERROR          = 0x0010,
    _IOCTRLZ          = 0x0020,

    // Buffering state bits:  These track the buffering mode of the stream:
    //
    // (*) CRT:      The buffer was allocated by the CRT via the usual mechanism
    //               (typically via __acrt_stdio_allocate_buffer_nolock, and of
    //               size _INTERNAL_BUFSIZ).
    //
    // (*) USER:     The buffer was allocated by the user and was configured via
    //               the setvbuf() function.
    //
    // (*) SETVBUF:  The buffer was set via the setvbuf() function.  This flag
    //               may be combined with either the CRT or USER flag, depending
    //               on who owns the buffer (based on how setvbuf() was called).
    //
    // (*) STBUF:    The buffer was set via a call to the
    //               __acrt_stdio_begin_temporary_buffering_nolock() function,
    //               which provides a temporary buffer for console I/O operations
    //               to improve the performance of bulk read or write operations.
    //
    // (*) NONE:     Buffering is disabled, either because it was explicitly
    //               disabled or because the CRT attempted to allocate a buffer
    //               but allocation failed.  When this flag is set, the internal
    //               two-byte character buffer is used.
    //
    // Note that these flags are related to, but distinct from, the public stdio
    // buffering flags that are used with setvbuf (_IOFBF, _IOLBF, and _IONBF).
    // Specifically, note that those flags are never or'ed into the flags for a
    // stream.
    _IOBUFFER_CRT     = 0x0040,
    _IOBUFFER_USER    = 0x0080,
    _IOBUFFER_SETVBUF = 0x0100,
    _IOBUFFER_STBUF   = 0x0200,
    _IOBUFFER_NONE    = 0x0400,

    // Commit-on-flush state bit:  When this flag is set, every flush operation
    // on the stream also commits the file to disk.
    _IOCOMMIT         = 0x0800,

    // String state bit:  When this flag is set, it indicates that the stream is
    // backed by a string, not a file.  String-backed streams are not exposed to
    // user code; they are created internally to support formatted I/O to string
    // buffers (e.g. the sprintf and sscanf families of functions).  If a stream
    // is backed by a string, its lock is not initialized and no synchronization
    // is required.
    _IOSTRING         = 0x1000,

    // Allocation state bit:  When this flag is set it indicates that the stream
    // is currently allocated and in-use.  If this flag is not set, it indicates
    // that the stream is free and available for use.
    _IOALLOCATED      = 0x2000,
};



#ifndef _M_CEE

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Internal stdio functions with PTD propagation
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
_Check_return_opt_
extern "C" int __cdecl _putch_nolock_internal(
    _In_    int                    _Ch,
    _Inout_ __crt_cached_ptd_host& _Ptd
    );

_Check_return_opt_
extern "C" wint_t __cdecl _putwch_nolock_internal(
    _In_    wchar_t                _Ch,
    _Inout_ __crt_cached_ptd_host& _Ptd
    );

_Check_return_opt_
extern "C" wint_t __cdecl _fputwc_nolock_internal(
    _In_    wchar_t                _Character,
    _Inout_ FILE*                  _Stream,
    _Inout_ __crt_cached_ptd_host& _Ptd
    );

_Success_(return != EOF)
_Check_return_opt_
extern "C" int __cdecl _fputc_nolock_internal(
    _In_    int                    _Character,
    _Inout_ FILE*                  _Stream,
    _Inout_ __crt_cached_ptd_host& _Ptd
    );

_Check_return_opt_
extern "C" size_t __cdecl _fwrite_nolock_internal(
    _In_reads_bytes_(_ElementSize * _ElementCount) void const*            _Buffer,
    _In_                                           size_t                 _ElementSize,
    _In_                                           size_t                 _ElementCount,
    _Inout_                                        FILE*                  _Stream,
    _Inout_                                        __crt_cached_ptd_host& _Ptd
    );

_Check_return_
extern "C" __int64 __cdecl _ftelli64_nolock_internal(
    _Inout_ FILE*                  _Stream,
    _Inout_ __crt_cached_ptd_host& _Ptd
    );

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Internal Stream Types (__crt_stdio_stream and friends)
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
struct __crt_stdio_stream_data
{
    union
    {
        FILE  _public_file;
        char* _ptr;
    };

    char*            _base;
    int              _cnt;
    long             _flags;
    long             _file;
    int              _charbuf;
    int              _bufsiz;
    char*            _tmpfname;
    CRITICAL_SECTION _lock;
};

// Ensure that __crt_stdio_stream_data* and FILE* pointers are freely convertible:
static_assert(
    offsetof(__crt_stdio_stream_data, _public_file) == 0,
    "FILE member of __crt_stdio_stream_data is not at offset zero."
    );

static_assert(
    sizeof(FILE) == sizeof(void*),
    "FILE structure has unexpected size."
    );


class __crt_stdio_stream
{
public:

    __crt_stdio_stream() throw()
        : _stream(nullptr)
    {
    }

    explicit __crt_stdio_stream(FILE* const stream) throw()
        : _stream(reinterpret_cast<__crt_stdio_stream_data*>(stream))
    {
    }

    explicit __crt_stdio_stream(__crt_stdio_stream_data* const stream) throw()
        : _stream(stream)
    {
    }

    bool  valid()         const throw() { return _stream != nullptr;     }
    FILE* public_stream() const throw() { return &_stream->_public_file; }



    // Tests whether this stream is allocated.  Returns true if the stream is
    // currently in use; returns false if the stream is free for allocation.
    bool is_in_use() const throw()
    {
        return (get_flags() & _IOALLOCATED) != 0;
    }

    // Attempts to allocate this stream for use.  Returns true if this stream was
    // free and has been allocated for the caller.  Returns false if the stream
    // was in-use and could not be allocated for the caller.  If it returns true,
    // the caller gains ownership of the stream and is responsible for deallocating
    // it.
    bool try_allocate() throw()
    {
        return (_InterlockedOr(&_stream->_flags, _IOALLOCATED) & _IOALLOCATED) == 0;
    }

    // Deallocates the stream, freeing it for use by another client.  It is
    // assumed that the caller owns the stream before calling this function.
    void deallocate() throw()
    {
        // Note:  We clear all flags intentionally, so that the stream object
        // is "clean" the next time it is allocated.
        _InterlockedExchange(&_stream->_flags, 0);
    }



    void lock()   const throw() { _lock_file  (public_stream()); }
    void unlock() const throw() { _unlock_file(public_stream()); }

    bool has_any_of(long const flags) const throw() { return (get_flags() & flags) != 0;     }
    bool has_all_of(long const flags) const throw() { return (get_flags() & flags) == flags; }

    bool set_flags  (long const flags) const throw() { return (_InterlockedOr(&_stream->_flags,   flags) & flags) != 0; }
    bool unset_flags(long const flags) const throw() { return (_InterlockedAnd(&_stream->_flags, ~flags) & flags) != 0; }

    bool eof()    const throw() { return has_any_of(_IOEOF);   }
    bool error()  const throw() { return has_any_of(_IOERROR); }
    bool ctrl_z() const throw() { return has_any_of(_IOCTRLZ); }

    bool has_crt_buffer()       const throw() { return has_any_of(_IOBUFFER_CRT);                                   }
    bool has_user_buffer()      const throw() { return has_any_of(_IOBUFFER_USER);                                  }
    bool has_temporary_buffer() const throw() { return has_any_of(_IOBUFFER_STBUF);                                 }
    bool has_setvbuf_buffer()   const throw() { return has_any_of(_IOBUFFER_SETVBUF);                               }
    bool has_big_buffer()       const throw() { return has_any_of(_IOBUFFER_CRT | _IOBUFFER_USER);                  }
    bool has_any_buffer()       const throw() { return has_any_of(_IOBUFFER_CRT | _IOBUFFER_USER | _IOBUFFER_NONE); }



    int lowio_handle() const throw() { return __crt_interlocked_read(&_stream->_file); }

    bool is_string_backed() const throw() { return (get_flags() & _IOSTRING) != 0; }

    __crt_stdio_stream_data* operator->() const throw() { return _stream; }

    long get_flags() const throw()
    {
        return __crt_interlocked_read(&_stream->_flags);
    }

private:

    __crt_stdio_stream_data* _stream;
};

// These cannot have C linkage because they use __crt_stdio_stream, which has
// a destructor.
__crt_stdio_stream __cdecl __acrt_stdio_allocate_stream() throw();
void __cdecl __acrt_stdio_free_stream(__crt_stdio_stream _Stream) throw();



template <typename Action>
auto __acrt_lock_stream_and_call(FILE* const stream, Action&& action) throw()
    -> decltype(action())
{
    return __crt_seh_guarded_call<decltype(action())>()(
        [stream]() { _lock_file(stream); },
        action,
        [stream]() { _unlock_file(stream); });
}



/*
 * Number of entries supported in the array pointed to by __piob[]. That is,
 * the number of stdio-level files which may be open simultaneously. This
 * is normally set to _NSTREAM_ by the stdio initialization code.
 */
extern "C" int _nstream;

/*
 * Pointer to the array of pointers to FILE structures that are used
 * to manage stdio-level files.
 */
extern "C" __crt_stdio_stream_data** __piob;

// __acrt_stdio_is_initialized cannot be with the rest of
// stdio initialization logic since referencing those symbols
// pulls in the stdio initializers.
inline bool __acrt_stdio_is_initialized() {
    return __piob != 0;
}

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Deprecated stdio functionality
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
extern "C" {
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0(
    _Success_(return != 0) char*, __RETURN_POLICY_SAME, _ACRTIMP, gets,
    _Pre_notnull_ _Post_z_ _Out_writes_z_(((size_t)-1)), char, _Buffer
    )

// string[0] must contain the maximum length of the string.  The number of
// characters written is stored in string[1].  The return value is a pointer to
// string[2] on success; nullptr on failure.
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0_CGETS(
    _Success_(return != 0) char*, _DCRTIMP, _cgets,
    _At_(_Buffer, _Pre_notnull_ _In_reads_(1))
    _At_(_Buffer + 1, _Pre_notnull_ _Out_writes_(1))
    _At_(_Buffer + 2, _Pre_notnull_ _Post_z_ _Out_writes_to_(_Buffer[0], _Buffer[1])),
    char, _Buffer
    )

__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0(
    _Success_(return != 0)
    wchar_t*, __RETURN_POLICY_SAME, _ACRTIMP, _getws,
    _Pre_notnull_ _Post_z_, wchar_t, _Buffer
    )

// string[0] must contain the maximum length of the string.  The number of
// characters written is stored in string[1].  The return value is a pointer to
// string[2] on success; nullptr on failure.
__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0_CGETS(
    _Post_satisfies_(return == 0 || return == _Buffer + 2)
    _Success_(return != 0) wchar_t*, _DCRTIMP, _cgetws,
    _At_(_Buffer, _In_reads_(1))
    _At_(_Buffer + 1, _Out_writes_(1))
    _At_(_Buffer + 2, _Post_z_ _Out_writes_to_(_Buffer[0], _Buffer[1])),
    wchar_t, _Buffer
    )

} // extern "C"
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Internal stdio functionality
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
extern "C" {


_Check_return_
FILE* __cdecl _openfile(
    _In_z_ char const* file_name,
    _In_z_ char const* mode,
    _In_   int         share_flag,
    _Out_  FILE*       stream
    );

_Check_return_
FILE* __cdecl _wopenfile(
    _In_z_ wchar_t const* file_name,
    _In_z_ wchar_t const* mode,
    _In_   int            share_flag,
    _Out_  FILE*          stream
    );

_Check_return_
int __cdecl __acrt_stdio_refill_and_read_narrow_nolock(
    _Inout_ FILE* stream
    );

_Check_return_
int __cdecl __acrt_stdio_refill_and_read_wide_nolock(
    _Inout_ FILE* stream
    );

_Check_return_opt_
int __cdecl __acrt_stdio_flush_and_write_narrow_nolock(
    _In_    int                    c,
    _Inout_ FILE*                  stream,
    _Inout_ __crt_cached_ptd_host& ptd
    );

_Check_return_opt_
int __cdecl __acrt_stdio_flush_and_write_wide_nolock(
    _In_    int                    c,
    _Inout_ FILE*                  stream,
    _Inout_ __crt_cached_ptd_host& ptd
    );

void __cdecl __acrt_stdio_allocate_buffer_nolock(
    _Out_ FILE* stream
    );

void __cdecl __acrt_stdio_free_buffer_nolock(
    _Inout_ FILE* stream
    );

bool __cdecl __acrt_stdio_begin_temporary_buffering_nolock(
    _Inout_ FILE* stream
    );

bool __cdecl __acrt_should_use_temporary_buffer(
    _In_ FILE* stream
);

void __cdecl __acrt_stdio_end_temporary_buffering_nolock(
    _In_    bool                 flag,
    _Inout_ FILE*                  stream,
    _Inout_ __crt_cached_ptd_host& ptd
    );

int  __cdecl __acrt_stdio_flush_nolock(
    _Inout_ FILE*                  stream,
    _Inout_ __crt_cached_ptd_host& ptd
    );

void __cdecl __acrt_stdio_free_tmpfile_name_buffers_nolock();

#ifndef CRTDLL
    extern int _cflush;
#endif

extern unsigned int _tempoff;
extern unsigned int _old_pfxlen;

} // extern "C"



class __acrt_stdio_temporary_buffering_guard
{
public:

    explicit __acrt_stdio_temporary_buffering_guard(FILE* const stream, __crt_cached_ptd_host& ptd) throw()
        : _stream(stream), _ptd(ptd)
    {
        _flag = __acrt_stdio_begin_temporary_buffering_nolock(stream);
    }

    __acrt_stdio_temporary_buffering_guard(__acrt_stdio_temporary_buffering_guard const&) throw() = delete;
    void operator=(__acrt_stdio_temporary_buffering_guard const&) throw() = delete;

    ~__acrt_stdio_temporary_buffering_guard() throw()
    {
        __acrt_stdio_end_temporary_buffering_nolock(_flag, _stream, _ptd);
    }

private:
    FILE*                  _stream;
    __crt_cached_ptd_host& _ptd;
    bool                   _flag;
};



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Character Traits
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
template <typename Character>
struct __acrt_stdio_char_traits;

template <>
struct __acrt_stdio_char_traits<char> : __crt_char_traits<char>
{
    static int_type const eof = EOF;

    static bool validate_stream_is_ansi_if_required(FILE* const stream) throw()
    {
        _VALIDATE_STREAM_ANSI_RETURN(stream, EINVAL, false);
        return true;
    }
};

template <>
struct __acrt_stdio_char_traits<wchar_t> : __crt_char_traits<wchar_t>
{
    static int_type const eof = WEOF;

    static bool validate_stream_is_ansi_if_required(FILE* const stream) throw()
    {
        UNREFERENCED_PARAMETER(stream);

        return true; // This validation is only for ANSI functions.
    }
};



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// fopen mode string parser
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// Represents a {lowio, stdio} mode pair.  This is the result of parsing a
// stdio mode string using the parser, defined below.
struct __acrt_stdio_stream_mode
{
    int  _lowio_mode;
    int  _stdio_mode;
    bool _success;
};



// This function and the following functions support __acrt_stdio_parse_mode.
// They handle individual parts of the parsing.
inline bool __acrt_stdio_parse_mode_plus(__acrt_stdio_stream_mode& result, bool& seen_plus) throw()
{
    if (seen_plus) {
        return false;
    }

    seen_plus = true;

    if (result._lowio_mode & _O_RDWR) {
        return false;
    }

    result._lowio_mode |= _O_RDWR;
    result._lowio_mode &= ~(_O_RDONLY | _O_WRONLY);
    result._stdio_mode |= _IOUPDATE;
    result._stdio_mode &= ~(_IOREAD | _IOWRITE);
    return true;
}

inline bool __acrt_stdio_parse_mode_b(__acrt_stdio_stream_mode& result) throw()
{
    if (result._lowio_mode & (_O_TEXT | _O_BINARY)) {
        return false;
    }

    result._lowio_mode |= _O_BINARY;
    return true;
}

inline bool __acrt_stdio_parse_mode_t(__acrt_stdio_stream_mode& result) throw()
{
    if (result._lowio_mode & (_O_TEXT | _O_BINARY)) {
        return false;
    }

    result._lowio_mode |= _O_TEXT;
    return true;
}

inline bool __acrt_stdio_parse_mode_c(__acrt_stdio_stream_mode& result, bool& seen_commit_mode) throw()
{
    if (seen_commit_mode) {
        return false;
    }

    seen_commit_mode = true;
    result._stdio_mode |= _IOCOMMIT;
    return true;
}

inline bool __acrt_stdio_parse_mode_n(__acrt_stdio_stream_mode& result, bool& seen_commit_mode) throw()
{
    if (seen_commit_mode) {
        return false;
    }

    seen_commit_mode = true;
    result._stdio_mode &= ~_IOCOMMIT;
    return true;
}

inline bool __acrt_stdio_parse_mode_S(__acrt_stdio_stream_mode& result, bool& seen_scan_mode) throw()
{
    if (seen_scan_mode) {
        return false;
    }

    seen_scan_mode = true;
    result._lowio_mode |= _O_SEQUENTIAL;
    return true;
}

inline bool __acrt_stdio_parse_mode_R(__acrt_stdio_stream_mode& result, bool& seen_scan_mode) throw()
{
    if (seen_scan_mode) {
        return false;
    }

    seen_scan_mode = true;
    result._lowio_mode |= _O_RANDOM;
    return true;
}

inline bool __acrt_stdio_parse_mode_T(__acrt_stdio_stream_mode& result) throw()
{
    if (result._lowio_mode & _O_SHORT_LIVED) {
        return false;
    }

    result._lowio_mode |= _O_SHORT_LIVED;
    return true;
}

inline bool __acrt_stdio_parse_mode_D(__acrt_stdio_stream_mode& result) throw()
{
    if (result._lowio_mode & _O_TEMPORARY) {
        return false;
    }

    result._lowio_mode |= _O_TEMPORARY;
    return true;
}

inline bool __acrt_stdio_parse_mode_N(__acrt_stdio_stream_mode& result) throw()
{
    result._lowio_mode |= _O_NOINHERIT;
    return true;
}

inline bool __acrt_stdio_parse_mode_x(__acrt_stdio_stream_mode& result) throw()
{
    if (!(result._lowio_mode & _O_TRUNC)) {
        // 'x' only permitted with 'w'
        return false;
    }

    result._lowio_mode |= _O_EXCL;
    return true;
}



// Parses a stdio mode string, returning the corresponding pair of lowio and
// stdio flags.  On success, sets the success flag in the result to true; on
// failure, sets that flag to false.  All failures are logic errors.
template <typename Character>
__acrt_stdio_stream_mode __cdecl __acrt_stdio_parse_mode(
    Character const* const mode
    ) throw()
{
    typedef __acrt_stdio_char_traits<Character> stdio_traits;

    // Note that we value initialize the result, so the success flag is false
    // by default.  This ensures that any premature return will return failure.
    __acrt_stdio_stream_mode result = __acrt_stdio_stream_mode();
    result._stdio_mode = _commode;

    // Advance past any leading spaces:
    Character const* it = mode;
    while (*it == ' ')
        ++it;

    // Read the first character.  It must be one of 'r', 'w' , or 'a':
    switch (*it)
    {
    case 'r':
        result._lowio_mode = _O_RDONLY;
        result._stdio_mode = _IOREAD;
        break;

    case 'w':
        result._lowio_mode = _O_WRONLY | _O_CREAT | _O_TRUNC;
        result._stdio_mode = _IOWRITE;
        break;

    case 'a':
        result._lowio_mode = _O_WRONLY | _O_CREAT | _O_APPEND;
        result._stdio_mode = _IOWRITE;
        break;

    default:
        _VALIDATE_RETURN(("Invalid file open mode", 0), EINVAL, result);
    }

    // Advance past the first character:
    ++it;

    // There can be up to seven more optional mode characters:
    // [1] A single '+' character
    // [2] One of 't' or 'b' (indicating text or binary, respectively)
    // [3] One of 'c' or 'n' (enable or disable auto-commit to disk on flush)
    // [4] One of 'S' or 'R' (optimize for sequential or random access)
    // [5] 'T' (indicating the file is short-lived)
    // [6] 'D' (indicating the file is temporary)
    // [7] 'N' (indicating the file should not be inherited by child processes)
    // [8] 'x' (indicating the file must be created and it is an error if it already exists)
    bool seen_commit_mode   = false;
    bool seen_plus          = false;
    bool seen_scan_mode     = false;
    bool seen_encoding_flag = false;
    for (bool continue_loop = true; continue_loop && *it != '\0'; it += (continue_loop ? 1 : 0))
    {
        switch (*it)
        {
        case '+': continue_loop = __acrt_stdio_parse_mode_plus(result, seen_plus);        break;
        case 'b': continue_loop = __acrt_stdio_parse_mode_b   (result);                   break;
        case 't': continue_loop = __acrt_stdio_parse_mode_t   (result);                   break;
        case 'c': continue_loop = __acrt_stdio_parse_mode_c   (result, seen_commit_mode); break;
        case 'n': continue_loop = __acrt_stdio_parse_mode_n   (result, seen_commit_mode); break;
        case 'S': continue_loop = __acrt_stdio_parse_mode_S   (result, seen_scan_mode  ); break;
        case 'R': continue_loop = __acrt_stdio_parse_mode_R   (result, seen_scan_mode  ); break;
        case 'T': continue_loop = __acrt_stdio_parse_mode_T   (result);                   break;
        case 'D': continue_loop = __acrt_stdio_parse_mode_D   (result);                   break;
        case 'N': continue_loop = __acrt_stdio_parse_mode_N   (result);                   break;
        case 'x': continue_loop = __acrt_stdio_parse_mode_x   (result);                   break;

        // If we encounter any spaces, skip them:
        case ' ':
            break;

        // If we encounter a comma, it begins the encoding specification; we
        // break out of the loop immediately and parse the encoding flag next:
        case ',':
            seen_encoding_flag = true;
            continue_loop = false;
            break;

        default:
            _VALIDATE_RETURN(("Invalid file open mode", 0), EINVAL, result);
        }
    }

    // Advance past the comma that terminated the loop:
    if (seen_encoding_flag)
        ++it;

    while (*it == ' ')
        ++it;

    // If we did not encounter the encoding introducer (a comma), make sure we
    // actually reached the end of the mode string.  We are done:
    if (!seen_encoding_flag)
    {
        _VALIDATE_RETURN(*it == '\0', EINVAL, result);
        result._success = true;
        return result;
    }

    // Otherwise, we saw the beginning of an encoding; parse it:
    static Character const ccs[]              = { 'c', 'c', 's' };
    static Character const utf8_encoding[]    = { 'U', 'T', 'F', '-', '8' };
    static Character const utf16_encoding[]   = { 'U', 'T', 'F', '-', '1', '6', 'L', 'E' };
    static Character const unicode_encoding[] = { 'U', 'N', 'I', 'C', 'O', 'D', 'E' };

    // Make sure it begins with "ccs" (all lowercase)...
    if (stdio_traits::tcsncmp(it, ccs, _countof(ccs)) != 0)
        _VALIDATE_RETURN(("Invalid file open mode", 0), EINVAL, result);

    it += _countof(ccs); // Advance past the "ccs"

    while (*it == ' ')
        ++it;

    if (*it != '=')
        _VALIDATE_RETURN(("Invalid file open mode", 0), EINVAL, result);

    ++it; // Advance past the "="

    while (*it == ' ')
        ++it;

    if (stdio_traits::tcsnicmp(it, utf8_encoding, _countof(utf8_encoding)) == 0)
    {
        it += _countof(utf8_encoding);
        result._lowio_mode |= _O_U8TEXT;
    }
    else if (stdio_traits::tcsnicmp(it, utf16_encoding, _countof(utf16_encoding)) == 0)
    {
        it += _countof(utf16_encoding);
        result._lowio_mode |= _O_U16TEXT;
    }
    else if (stdio_traits::tcsnicmp(it, unicode_encoding, _countof(unicode_encoding)) == 0)
    {
        it += _countof(unicode_encoding);
        result._lowio_mode |= _O_WTEXT;
    }
    else
    {
        _VALIDATE_RETURN(("Invalid file open mode", 0), EINVAL, result);
    }

    // Finally, skip any trailing spaces...
    while (*it == ' ')
        ++it;

    // ...and ensure there are no characters left:
    _VALIDATE_RETURN(*it == '\0', EINVAL, result);

    result._success = true;
    return result;
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// __acrt_common_path_requires_backslash()
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
inline bool __cdecl __crt_stdio_path_requires_backslash(char const* const first) throw()
{
    char const* const last = first + strlen(first);
    if (first == last)
        return false;

    if (*(last - 1) == '\\')
    {
        return reinterpret_cast<unsigned char const*>(last - 1)
            != _mbsrchr(reinterpret_cast<unsigned char const*>(first), '\\');
    }

    if (*(last - 1) == '/')
        return false;

    return true;
}

inline bool __cdecl __crt_stdio_path_requires_backslash(wchar_t const* const first) throw()
{
    wchar_t const* const last = first + wcslen(first);
    if (first == last)
        return false;

    if (*(last - 1) == L'\\')
        return false;

    if (*(last - 1) == L'/')
        return false;

    return true;
}

// Reset the file buffer to be empty and ready for reuse
inline void __cdecl __acrt_stdio_reset_buffer(__crt_stdio_stream const stream) throw()
{
    stream->_ptr = stream->_base;
    stream->_cnt = 0;
}

#endif // !_M_CEE

#pragma pack(pop)
