//
// corecrt_internal_lowio.h
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// This internal header defines internal utilities for working with the lowio
// library.  This header may only be included in C++ translation units.
//
#pragma once

#include <fcntl.h>
#include <io.h>
#include <corecrt_internal_traits.h>
#include <share.h>
#include <stdint.h>
#include <stdlib.h>

_CRT_BEGIN_C_HEADER



#define LF 10           /* line feed */
#define CR 13           /* carriage return */
#define CTRLZ 26        /* ctrl-z means eof for text */

// Real default size for stdio buffers
#define _INTERNAL_BUFSIZ    4096
#define _SMALL_BUFSIZ       512

/* Most significant Bit */
#define _msbit(c) ((c) & 0x80)

/* Independent byte has most significant bit set to 0 */
#define  _utf8_is_independent(c)    (_msbit(c) == 0)

/* Get no of trailing bytes from the lookup table */
//    1 for pattern 110xxxxx - 1 trailbyte
//    2 for pattern 1110xxxx - 2 trailbytes
//    3 for pattern 11110xxx - 3 trailbytes
//    0 for everything else, including invalid patterns.
// We return 0 for invalid patterns because we rely on MultiByteToWideChar to
// do the validations.

extern char _lookuptrailbytes[256];
__inline char _utf8_no_of_trailbytes(const unsigned char c)
{
    return _lookuptrailbytes[c];
}
// It may be faster to just look up the bytes than to use the lookup table.
//__inline char _utf8_no_of_trailbytes(const unsigned char c)
//{
//    // ASCII range is a single character
//    if ((c & 0x80) == 0) return 0;
//    // Trail bytes 10xxxxxx aren't lead bytes
//    if ((c & 0x40) == 0) return 0;
//    // 110xxxxx is a 2 byte sequence (1 trail byte)
//    if ((c & 0x20) == 0) return 1;
//    // 1110xxxx is a 3 byte sequence (2 trail bytes)
//    if ((c & 0x10) == 0) return 2;
//    // 11110xxx is a 4 byte sequence (3 trail bytes)
//    if ((c & 0x08) == 0) return 3;
//    // Anything with 5 or more lead bits is illegal
//    return 0;
//}

/* Any leadbyte will have the patterns 11000xxx 11100xxx or 11110xxx */
#define  _utf8_is_leadbyte(c)       (_utf8_no_of_trailbytes(static_cast<const unsigned char>(c)) != 0)

enum class __crt_lowio_text_mode : char
{
    ansi    = 0, // Regular text
    utf8    = 1, // UTF-8 encoded
    utf16le = 2, // UTF-16LE encoded
};

// osfile flag values
enum : unsigned char
{
    FOPEN      = 0x01, // file handle open
    FEOFLAG    = 0x02, // end of file has been encountered
    FCRLF      = 0x04, // CR-LF across read buffer (in text mode)
    FPIPE      = 0x08, // file handle refers to a pipe
    FNOINHERIT = 0x10, // file handle opened _O_NOINHERIT
    FAPPEND    = 0x20, // file handle opened O_APPEND
    FDEV       = 0x40, // file handle refers to device
    FTEXT      = 0x80, // file handle is in text mode
};

typedef char __crt_lowio_pipe_lookahead[3];

/*
 * Control structure for lowio file handles
 */
struct __crt_lowio_handle_data
{
    CRITICAL_SECTION           lock;
    intptr_t                   osfhnd;          // underlying OS file HANDLE
    __int64                    startpos;        // File position that matches buffer start
    unsigned char              osfile;          // Attributes of file (e.g., open in text mode?)
    __crt_lowio_text_mode      textmode;
    __crt_lowio_pipe_lookahead _pipe_lookahead;

    uint8_t unicode          : 1; // Was the file opened as unicode?
    uint8_t utf8translations : 1; // Buffer contains translations other than CRLF
    uint8_t dbcsBufferUsed   : 1; // Is the dbcsBuffer in use?
    char    mbBuffer[MB_LEN_MAX]; // Buffer for the lead byte of DBCS when converting from DBCS to Unicode
                                  // Or for the first up to 3 bytes of a UTF-8 character
};

// The log-base-2 of the number of elements in each array of lowio file objects
#define IOINFO_L2E          6

// The number of elements in each array of lowio file objects
#define IOINFO_ARRAY_ELTS   (1 << IOINFO_L2E)

// The hard maximum number of arrays of lowio file objects that may be allocated
#define IOINFO_ARRAYS       128

// The maximum number of lowio file objects that may be allocated at any one time
#define _NHANDLE_           (IOINFO_ARRAYS * IOINFO_ARRAY_ELTS)



#define STDIO_HANDLES_COUNT       3
/*
 * Access macros for getting at an __crt_lowio_handle_data struct and its fields from a
 * file handle
 */
#define _pioinfo(i)          (__pioinfo[(i) >> IOINFO_L2E] + ((i) & (IOINFO_ARRAY_ELTS - 1)))
#define _osfhnd(i)           (_pioinfo(i)->osfhnd)
#define _osfile(i)           (_pioinfo(i)->osfile)
#define _pipe_lookahead(i)   (_pioinfo(i)->_pipe_lookahead)
#define _textmode(i)         (_pioinfo(i)->textmode)
#define _tm_unicode(i)       (_pioinfo(i)->unicode)
#define _startpos(i)         (_pioinfo(i)->startpos)
#define _utf8translations(i) (_pioinfo(i)->utf8translations)
#define _mbBuffer(i)         (_pioinfo(i)->mbBuffer)
#define _dbcsBuffer(i)       (_pioinfo(i)->mbBuffer[0])
#define _dbcsBufferUsed(i)   (_pioinfo(i)->dbcsBufferUsed)

/*
 * Safer versions of the above macros. Currently, only _osfile_safe is
 * used.
 */
#define _pioinfo_safe(i)    ((((i) != -1) && ((i) != -2)) ? _pioinfo(i) : &__badioinfo)
#define _osfile_safe(i)     (_pioinfo_safe(i)->osfile)
#define _textmode_safe(i)   (_pioinfo_safe(i)->textmode)
#define _tm_unicode_safe(i) (_pioinfo_safe(i)->unicode)

typedef __crt_lowio_handle_data* __crt_lowio_handle_data_array[IOINFO_ARRAYS];

// Special, static lowio file object used only for more graceful handling
// of a C file handle value of -1 (results from common errors at the stdio
// level).
extern __crt_lowio_handle_data __badioinfo;

// The umask value
extern int _umaskval;

// Global array of pointers to the arrays of lowio file objects.
extern __crt_lowio_handle_data_array __pioinfo;

// The number of handles for which file objects have been allocated.  This
// number is such that for any fh in [0, _nhandle), _pioinfo(fh) is well-
// formed.
extern int _nhandle;



int __cdecl _alloc_osfhnd(void);
int __cdecl _free_osfhnd(int);
int __cdecl __acrt_lowio_set_os_handle(int, intptr_t);

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Internal lowio functions
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

_Success_(return == 0)
errno_t __cdecl _sopen_nolock(
    _Out_  int*        _UnlockFlag,
    _Out_  int*        _FileHandle,
    _In_z_ char const* _FileName,
    _In_   int         _OpenFlag,
    _In_   int         _ShareFlag,
    _In_   int         _PermissionFlag,
    _In_   int         _SecureFlag
    );

_Success_(return == 0)
errno_t __cdecl _wsopen_nolock(
    _Out_  int*           _UnlockFlag,
    _Out_  int*           _FileHandle,
    _In_z_ wchar_t const* _FileName,
    _In_   int            _OpenFlag,
    _In_   int            _ShareFlag,
    _In_   int            _PermissionFlag,
    _In_   int            _SecureFlag
    );


_Check_return_
__crt_lowio_handle_data* __cdecl __acrt_lowio_create_handle_array();

void __cdecl __acrt_lowio_destroy_handle_array(
    _Pre_maybenull_ _Post_invalid_ _In_reads_opt_(IOINFO_ARRAY_ELTS) __crt_lowio_handle_data* _Array
    );

_Check_return_opt_
errno_t __cdecl __acrt_lowio_ensure_fh_exists(
    _In_ int _FileHandle
    );

void __cdecl __acrt_lowio_lock_fh  (_In_ int _FileHandle);
void __cdecl __acrt_lowio_unlock_fh(_In_ int _FileHandle);

extern "C++"
{
    template <typename Action>
    auto __acrt_lowio_lock_fh_and_call(int const fh, Action&& action) throw()
        -> decltype(action())
    {
        return __crt_seh_guarded_call<decltype(action())>()(
            [fh]() { __acrt_lowio_lock_fh(fh); },
            action,
            [fh]() { __acrt_lowio_unlock_fh(fh); });
    }
}

// console_invalid_handle indicates that CONOUT$ or CONIN$ could not be created
// console_uninitialized_handle indicates that the handle has not yet been initialized
const HANDLE _console_invalid_handle = reinterpret_cast<HANDLE>(-1);
const HANDLE _console_uninitialized_handle = reinterpret_cast<HANDLE>(-2);

BOOL __cdecl __dcrt_lowio_ensure_console_input_initialized(void);

BOOL __cdecl __dcrt_read_console_input(
    _Out_ PINPUT_RECORD lpBuffer,
    _In_  DWORD         nLength,
    _Out_ LPDWORD       lpNumberOfEventsRead
    );

BOOL __cdecl __dcrt_read_console(
    _Out_ LPVOID  lpBuffer,
    _In_  DWORD   nNumberOfCharsToRead,
    _Out_ LPDWORD lpNumberOfCharsRead
    );

BOOL __cdecl __dcrt_get_number_of_console_input_events(
    _Out_ LPDWORD lpcNumberOfEvents
    );

BOOL __cdecl __dcrt_peek_console_input_a(
    _Out_ PINPUT_RECORD lpBuffer,
    _In_  DWORD         nLength,
    _Out_ LPDWORD       lpNumberOfEventsRead
    );

BOOL __cdecl __dcrt_get_input_console_mode(
    _Out_ LPDWORD lpMode
    );

BOOL __cdecl __dcrt_set_input_console_mode(
    _In_ DWORD dwMode
    );

BOOL __cdecl __dcrt_lowio_ensure_console_output_initialized(void);

BOOL __cdecl __dcrt_write_console(
    _In_  void const * lpBuffer,
    _In_  DWORD        nNumberOfCharsToWrite,
    _Out_ LPDWORD      lpNumberOfCharsWritten
    );

_Check_return_ int __cdecl _chsize_nolock(_In_ int _FileHandle,_In_ __int64 _Size);
_Check_return_opt_ int __cdecl _close_nolock(_In_ int _FileHandle);
_Check_return_opt_ long __cdecl _lseek_nolock(_In_ int _FileHandle, _In_ long _Offset, _In_ int _Origin);
_Check_return_ int __cdecl _setmode_nolock(_In_ int _FileHandle, _In_ int _Mode);
_Check_return_ _Success_(return >= 0 && return <= _MaxCharCount) int __cdecl _read_nolock(_In_ int _FileHandle, _Out_writes_bytes_(_MaxCharCount) void * _DstBuf, _In_ unsigned int _MaxCharCount);
_Check_return_ int __cdecl _write_nolock(_In_ int _FileHandle, _In_reads_bytes_(_MaxCharCount) const void * _Buf, _In_ unsigned int _MaxCharCount, __crt_cached_ptd_host& _Ptd);
_Check_return_opt_ __int64 __cdecl _lseeki64_nolock(_In_ int _FileHandle, _In_ __int64 _Offset, _In_ int _Origin);

// Temporary until non-PTD propagating versions can be replaced:
_Check_return_ int __cdecl _chsize_nolock_internal(_In_ int _FileHandle, _In_ __int64 _Size, _Inout_ __crt_cached_ptd_host& _Ptd);
_Check_return_opt_ __int64 __cdecl _lseeki64_nolock_internal(_In_ int _FileHandle, _In_ __int64 _Offset, _In_ int _Origin, _Inout_ __crt_cached_ptd_host& _Ptd);
_Check_return_opt_ int __cdecl _close_nolock_internal(_In_ int _FileHandle, _Inout_ __crt_cached_ptd_host& _Ptd);

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Internal stdio functions with PTD propagation
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

_Check_return_opt_
int __cdecl _close_internal(
    _In_    int                    _FileHandle,
    _Inout_ __crt_cached_ptd_host& _Ptd
    );

_Check_return_opt_
long __cdecl _lseek_internal(
    _In_    int                    _FileHandle,
    _In_    long                   _Offset,
    _In_    int                    _Origin,
    _Inout_ __crt_cached_ptd_host& _Ptd
    );

_Check_return_opt_
__int64 __cdecl _lseeki64_internal(
    _In_    int                    _FileHandle,
    _In_    __int64                _Offset,
    _In_    int                    _Origin,
    _Inout_ __crt_cached_ptd_host& _Ptd
    );

int __cdecl _write_internal(
    _In_                            int                    _FileHandle,
    _In_reads_bytes_(_MaxCharCount) void const*            _Buf,
    _In_                            unsigned int           _MaxCharCount,
    _Inout_                         __crt_cached_ptd_host& _Ptd
    );

// fileno for stdout, stdin & stderr when there is no console
#define _NO_CONSOLE_FILENO ((intptr_t)-2)


_CRT_END_C_HEADER
