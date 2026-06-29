//
// stdio.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The C Standard Library <stdio.h> header.
//
#pragma once
#ifndef _INC_STDIO // include guard for 3rd party interop
#define _INC_STDIO

#include <corecrt.h>
#include <corecrt_wstdio.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER

/* Buffered I/O macros */

#define BUFSIZ  512



/*
 * Default number of supported streams. _NFILE is confusing and obsolete, but
 * supported anyway for backwards compatibility.
 */
#define _NFILE      _NSTREAM_

#define _NSTREAM_   512

/*
 * Number of entries in _iob[] (declared below). Note that _NSTREAM_ must be
 * greater than or equal to _IOB_ENTRIES.
 */
#define _IOB_ENTRIES 3

#define EOF    (-1)

#define _IOFBF 0x0000
#define _IOLBF 0x0040
#define _IONBF 0x0004



#define L_tmpnam   260 // _MAX_PATH
#if __STDC_WANT_SECURE_LIB__
    #define L_tmpnam_s L_tmpnam
#endif



/* Seek method constants */

#define SEEK_CUR    1
#define SEEK_END    2
#define SEEK_SET    0


#define FILENAME_MAX    260
#define FOPEN_MAX       20
#define _SYS_OPEN       20
#define TMP_MAX         _CRT_INT_MAX
#if __STDC_WANT_SECURE_LIB__
    #define TMP_MAX_S       TMP_MAX
    #define _TMP_MAX_S      TMP_MAX
#endif


typedef __int64 fpos_t;



#if _CRT_FUNCTIONS_REQUIRED

    _Check_return_opt_
    _ACRTIMP errno_t __cdecl _get_stream_buffer_pointers(
        _In_      FILE*   _Stream,
        _Out_opt_ char*** _Base,
        _Out_opt_ char*** _Pointer,
        _Out_opt_ int**   _Count
        );


    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // Narrow Character Stream I/O Functions
    //
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    #if __STDC_WANT_SECURE_LIB__

        _Check_return_wat_
        _ACRTIMP errno_t __cdecl clearerr_s(
            _Inout_ FILE* _Stream
            );

        _Check_return_wat_
        _Success_(return == 0)
        _ACRTIMP errno_t __cdecl fopen_s(
            _Outptr_result_nullonfailure_ FILE**      _Stream,
            _In_z_                        char const* _FileName,
            _In_z_                        char const* _Mode
            );

        _Check_return_opt_
        _Success_(return != 0)
        _ACRTIMP size_t __cdecl fread_s(
            _Out_writes_bytes_to_(_BufferSize, _ElementSize * _ElementCount)   void*  _Buffer,
            _In_range_(>=, _ElementSize * _ElementCount)                       size_t _BufferSize,
            _In_                                                               size_t _ElementSize,
            _In_                                                               size_t _ElementCount,
            _Inout_                                                            FILE*  _Stream
            );

        _Check_return_wat_
        _ACRTIMP errno_t __cdecl freopen_s(
            _Outptr_result_maybenull_ FILE**      _Stream,
            _In_z_                    char const* _FileName,
            _In_z_                    char const* _Mode,
            _Inout_                   FILE*       _OldStream
            );

        _Success_(return != 0)
        _ACRTIMP char* __cdecl gets_s(
            _Out_writes_z_(_Size) char*   _Buffer,
            _In_                  rsize_t _Size
            );

        _Check_return_wat_
        _ACRTIMP errno_t __cdecl tmpfile_s(
            _Out_opt_ _Deref_post_valid_ FILE** _Stream
            );

        _Success_(return == 0)
        _Check_return_wat_
        _ACRTIMP errno_t __cdecl tmpnam_s(
            _Out_writes_z_(_Size) char*   _Buffer,
            _In_                  rsize_t _Size
            );

    #endif

    _ACRTIMP void __cdecl clearerr(
        _Inout_ FILE* _Stream
        );

    _Success_(return != -1)
    _Check_return_opt_
    _ACRTIMP int __cdecl fclose(
        _Inout_ FILE* _Stream
        );

    _Check_return_opt_
    _ACRTIMP int __cdecl _fcloseall(void);

    _Check_return_
    _ACRTIMP FILE* __cdecl _fdopen(
        _In_   int         _FileHandle,
        _In_z_ char const* _Mode
        );

    _Check_return_
    _ACRTIMP int __cdecl feof(
        _In_ FILE* _Stream
        );

    _Check_return_
    _ACRTIMP int __cdecl ferror(
        _In_ FILE* _Stream
        );

    _Check_return_opt_
    _ACRTIMP int __cdecl fflush(
        _Inout_opt_ FILE* _Stream
        );

    _Success_(return != EOF)
    _Check_return_opt_
    _ACRTIMP int __cdecl fgetc(
        _Inout_ FILE* _Stream
        );

    _Check_return_opt_
    _ACRTIMP int __cdecl _fgetchar(void);

    _Success_(return != EOF)
    _Check_return_opt_
    _ACRTIMP int __cdecl fgetpos(
        _Inout_ FILE*   _Stream,
        _Out_   fpos_t* _Position
        );

    _Success_(return == _Buffer)
    _Check_return_opt_
    _ACRTIMP char* __cdecl fgets(
        _Out_writes_z_(_MaxCount) char* _Buffer,
        _In_                      int   _MaxCount,
        _Inout_                   FILE* _Stream
        );

    _Check_return_
    _ACRTIMP int __cdecl _fileno(
        _In_ FILE* _Stream
        );

    _Check_return_opt_
    _ACRTIMP int __cdecl _flushall(void);

    _Check_return_ _CRT_INSECURE_DEPRECATE(fopen_s)
    _ACRTIMP FILE* __cdecl fopen(
        _In_z_ char const* _FileName,
        _In_z_ char const* _Mode
        );


    _Success_(return != EOF)
    _Check_return_opt_
    _ACRTIMP int __cdecl fputc(
        _In_    int   _Character,
        _Inout_ FILE* _Stream
        );

    _Check_return_opt_
    _ACRTIMP int __cdecl _fputchar(
        _In_ int _Character
        );

    _Success_(return != EOF)
    _Check_return_opt_
    _ACRTIMP int __cdecl fputs(
        _In_z_  char const* _Buffer,
        _Inout_ FILE*       _Stream
        );

    _Check_return_opt_
    _ACRTIMP size_t __cdecl fread(
        _Out_writes_bytes_(_ElementSize * _ElementCount) void*  _Buffer,
        _In_                                             size_t _ElementSize,
        _In_                                             size_t _ElementCount,
        _Inout_                                          FILE*  _Stream
        );

    _Success_(return != 0)
    _Check_return_ _CRT_INSECURE_DEPRECATE(freopen_s)
    _ACRTIMP FILE* __cdecl freopen(
        _In_z_  char const* _FileName,
        _In_z_  char const* _Mode,
        _Inout_ FILE*       _Stream
        );

    _Check_return_
    _ACRTIMP FILE* __cdecl _fsopen(
        _In_z_ char const* _FileName,
        _In_z_ char const* _Mode,
        _In_   int         _ShFlag
        );

    _Success_(return == 0)
    _Check_return_opt_
    _ACRTIMP int __cdecl fsetpos(
        _Inout_ FILE*         _Stream,
        _In_    fpos_t const* _Position
        );

    _Success_(return == 0)
    _Check_return_opt_
    _ACRTIMP int __cdecl fseek(
        _Inout_ FILE* _Stream,
        _In_    long  _Offset,
        _In_    int   _Origin
        );

    _Success_(return == 0)
    _Check_return_opt_
    _ACRTIMP int __cdecl _fseeki64(
        _Inout_ FILE*   _Stream,
        _In_    __int64 _Offset,
        _In_    int     _Origin
        );

    _Success_(return != -1)
    _Check_return_
    _ACRTIMP long __cdecl ftell(
        _Inout_ FILE* _Stream
        );

    _Success_(return != -1)
    _Check_return_
    _ACRTIMP __int64 __cdecl _ftelli64(
        _Inout_ FILE* _Stream
        );

    _Check_return_opt_
    _ACRTIMP size_t __cdecl fwrite(
        _In_reads_bytes_(_ElementSize * _ElementCount) void const* _Buffer,
        _In_                                           size_t      _ElementSize,
        _In_                                           size_t      _ElementCount,
        _Inout_                                        FILE*       _Stream
        );

    _Success_(return != EOF)
    _Check_return_
    _ACRTIMP int __cdecl getc(
        _Inout_ FILE* _Stream
        );

    _Check_return_
    _ACRTIMP int __cdecl getchar(void);

    _Check_return_
    _ACRTIMP int __cdecl _getmaxstdio(void);

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_0(
        char*, gets_s,
        char, _Buffer)

    _Check_return_
    _ACRTIMP int __cdecl _getw(
        _Inout_ FILE* _Stream
        );

    _ACRTIMP void __cdecl perror(
        _In_opt_z_ char const* _ErrorMessage
        );

    #if defined _CRT_USE_WINAPI_FAMILY_DESKTOP_APP

        _Success_(return != -1)
        _Check_return_opt_
        _DCRTIMP int __cdecl _pclose(
            _Inout_ FILE* _Stream
            );

        _Check_return_
        _DCRTIMP FILE* __cdecl _popen(
            _In_z_ char const* _Command,
            _In_z_ char const* _Mode
            );

    #endif

    _Success_(return != EOF)
    _Check_return_opt_
    _ACRTIMP int __cdecl putc(
        _In_    int   _Character,
        _Inout_ FILE* _Stream
        );

    _Check_return_opt_
    _ACRTIMP int __cdecl putchar(
        _In_ int _Character
        );

    _Check_return_opt_
    _ACRTIMP int __cdecl puts(
        _In_z_ char const* _Buffer
        );

    _Success_(return != -1)
    _Check_return_opt_
    _ACRTIMP int __cdecl _putw(
        _In_    int   _Word,
        _Inout_ FILE* _Stream
        );

    _ACRTIMP int __cdecl remove(
        _In_z_ char const* _FileName
        );

    _Check_return_
    _ACRTIMP int __cdecl rename(
        _In_z_ char const* _OldFileName,
        _In_z_ char const* _NewFileName
        );

    _ACRTIMP int __cdecl _unlink(
        _In_z_ char const* _FileName
        );

    #if defined(_CRT_INTERNAL_NONSTDC_NAMES) && _CRT_INTERNAL_NONSTDC_NAMES

        _CRT_NONSTDC_DEPRECATE(_unlink)
        _ACRTIMP int __cdecl unlink(
            _In_z_ char const* _FileName
            );

    #endif

    _ACRTIMP void __cdecl rewind(
        _Inout_ FILE* _Stream
        );

    _Check_return_opt_
    _ACRTIMP int __cdecl _rmtmp(void);

    _CRT_INSECURE_DEPRECATE(setvbuf)
    _ACRTIMP void __cdecl setbuf(
        _Inout_                                             FILE* _Stream,
        _Inout_updates_opt_(BUFSIZ) _Post_readable_size_(0) char* _Buffer
        );

    _Check_return_opt_
    _ACRTIMP int __cdecl _setmaxstdio(
        _In_ int _Maximum
        );

    _Success_(return == 0)
    _Check_return_opt_
    _ACRTIMP int __cdecl setvbuf(
        _Inout_                      FILE*  _Stream,
        _Inout_updates_opt_(_Size)   char*  _Buffer,
        _In_                         int    _Mode,
        _In_                         size_t _Size
        );

    #if defined _DEBUG && defined _CRTDBG_MAP_ALLOC
        #pragma push_macro("_tempnam")
        #undef _tempnam
    #endif

    _Check_return_
    _ACRTIMP _CRTALLOCATOR char* __cdecl _tempnam(
        _In_opt_z_ char const* _DirectoryName,
        _In_opt_z_ char const* _FilePrefix
        );

    #if defined _DEBUG && defined _CRTDBG_MAP_ALLOC
        #pragma pop_macro("_tempnam")
    #endif

    _Check_return_ _CRT_INSECURE_DEPRECATE(tmpfile_s)
    _ACRTIMP FILE* __cdecl tmpfile(void);

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_0(
        _Success_(return == 0)
        errno_t, tmpnam_s,
        _Always_(_Post_z_) char, _Buffer
        )

__DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0(
        _Success_(return != 0)
        char*, __RETURN_POLICY_DST, _ACRTIMP, tmpnam,
        _Pre_maybenull_ _Always_(_Post_z_), char, _Buffer
        )

    _Success_(return != EOF)
    _Check_return_opt_
    _ACRTIMP int __cdecl ungetc(
        _In_    int   _Character,
        _Inout_ FILE* _Stream
        );



    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // I/O Synchronization and _nolock family of I/O functions
    //
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    _ACRTIMP void __cdecl _lock_file(
        _Inout_ FILE* _Stream
        );

    _ACRTIMP void __cdecl _unlock_file(
        _Inout_ FILE* _Stream
        );

    _Success_(return != EOF)
    _Check_return_opt_
    _ACRTIMP int __cdecl _fclose_nolock(
        _Inout_ FILE* _Stream
        );

    _Success_(return != EOF)
    _Check_return_opt_
    _ACRTIMP int __cdecl _fflush_nolock(
        _Inout_opt_ FILE* _Stream
        );

    _Success_(return != EOF)
    _Check_return_opt_
    _ACRTIMP int __cdecl _fgetc_nolock(
        _Inout_ FILE* _Stream
        );

    _Success_(return != EOF)
    _Check_return_opt_
    _ACRTIMP int __cdecl _fputc_nolock(
        _In_    int   _Character,
        _Inout_ FILE* _Stream
        );

    _Check_return_opt_
    _ACRTIMP size_t __cdecl _fread_nolock(
        _Out_writes_bytes_(_ElementSize * _ElementCount) void*  _Buffer,
        _In_                                             size_t _ElementSize,
        _In_                                             size_t _ElementCount,
        _Inout_                                          FILE*  _Stream
        );

    _Check_return_opt_
    _Success_(return != 0)
    _ACRTIMP size_t __cdecl _fread_nolock_s(
        _Out_writes_bytes_to_(_BufferSize, _ElementSize * _ElementCount) void*  _Buffer,
        _In_range_(>=, _ElementSize * _ElementCount)                     size_t _BufferSize,
        _In_                                                             size_t _ElementSize,
        _In_                                                             size_t _ElementCount,
        _Inout_                                                          FILE*  _Stream
        );

    _Check_return_opt_
    _ACRTIMP int __cdecl _fseek_nolock(
        _Inout_ FILE* _Stream,
        _In_    long  _Offset,
        _In_    int   _Origin
        );

    _Check_return_opt_
    _ACRTIMP int __cdecl _fseeki64_nolock(
        _Inout_ FILE*   _Stream,
        _In_    __int64 _Offset,
        _In_    int     _Origin
        );

    _Check_return_
    _ACRTIMP long __cdecl _ftell_nolock(
        _Inout_ FILE* _Stream
        );

    _Check_return_
    _ACRTIMP __int64 __cdecl _ftelli64_nolock(
        _Inout_ FILE* _Stream
        );

    _Check_return_opt_
    _ACRTIMP size_t __cdecl _fwrite_nolock(
        _In_reads_bytes_(_ElementSize * _ElementCount) void const* _Buffer,
        _In_                                           size_t      _ElementSize,
        _In_                                           size_t      _ElementCount,
        _Inout_                                        FILE*       _Stream
        );

    _Check_return_opt_
    _ACRTIMP int __cdecl _getc_nolock(
        _Inout_ FILE* _Stream
        );

    _Check_return_opt_
    _ACRTIMP int __cdecl _putc_nolock(
        _In_    int   _Character,
        _Inout_ FILE* _Stream
        );

    _Check_return_opt_
    _ACRTIMP int __cdecl _ungetc_nolock(
        _In_    int   _Character,
        _Inout_ FILE* _Stream
        );

    #define _getchar_nolock()     _getc_nolock(stdin)
    #define _putchar_nolock(_Ch)  _putc_nolock(_Ch, stdout)
    #define _getwchar_nolock()    _getwc_nolock(stdin)
    #define _putwchar_nolock(_Ch) _putwc_nolock(_Ch, stdout)



    #if defined _CRT_DISABLE_PERFCRIT_LOCKS && !defined _DLL
        #define fclose(_Stream)                                           _fclose_nolock(_Stream)
        #define fflush(_Stream)                                           _fflush_nolock(_Stream)
        #define fgetc(_Stream)                                            _fgetc_nolock(_Stream)
        #define fputc(_Ch, _Stream)                                       _fputc_nolock(_Ch, _Stream)
        #define fread(_DstBuf, _ElementSize, _Count, _Stream)             _fread_nolock(_DstBuf, _ElementSize, _Count, _Stream)
        #define fread_s(_DstBuf, _DstSize, _ElementSize, _Count, _Stream) _fread_nolock_s(_DstBuf, _DstSize, _ElementSize, _Count, _Stream)
        #define fseek(_Stream, _Offset, _Origin)                          _fseek_nolock(_Stream, _Offset, _Origin)
        #define _fseeki64(_Stream, _Offset, _Origin)                      _fseeki64_nolock(_Stream, _Offset, _Origin)
        #define ftell(_Stream)                                            _ftell_nolock(_Stream)
        #define _ftelli64(_Stream)                                        _ftelli64_nolock(_Stream)
        #define fwrite(_SrcBuf, _ElementSize, _Count, _Stream)            _fwrite_nolock(_SrcBuf, _ElementSize, _Count, _Stream)
        #define getc(_Stream)                                             _getc_nolock(_Stream)
        #define putc(_Ch, _Stream)                                        _putc_nolock(_Ch, _Stream)
        #define ungetc(_Ch, _Stream)                                      _ungetc_nolock(_Ch, _Stream)
    #endif



    _ACRTIMP int* __cdecl __p__commode(void);

    #ifdef _CRT_DECLARE_GLOBAL_VARIABLES_DIRECTLY
        extern int _commode;
    #else
        #define _commode (*__p__commode())
    #endif



    // Variadic functions are not supported in managed code under /clr
    #if defined _M_CEE_MIXED
        #pragma managed(push, off)
    #endif

    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // Narrow Character Formatted Output Functions (Stream)
    //
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    _ACRTIMP int __cdecl __stdio_common_vfprintf(
        _In_                                    unsigned __int64 _Options,
        _Inout_                                 FILE*            _Stream,
        _In_z_ _Printf_format_string_params_(2) char const*      _Format,
        _In_opt_                                _locale_t        _Locale,
                                                va_list          _ArgList
        );

    _ACRTIMP int __cdecl __stdio_common_vfprintf_s(
        _In_                                    unsigned __int64 _Options,
        _Inout_                                 FILE*            _Stream,
        _In_z_ _Printf_format_string_params_(2) char const*      _Format,
        _In_opt_                                _locale_t        _Locale,
                                                va_list          _ArgList
        );

    _Success_(return >= 0)
    _ACRTIMP int __cdecl __stdio_common_vfprintf_p(
        _In_                                    unsigned __int64 _Options,
        _Inout_                                 FILE*            _Stream,
        _In_z_ _Printf_format_string_params_(2) char const*      _Format,
        _In_opt_                                _locale_t        _Locale,
                                                va_list          _ArgList
        );

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vfprintf_l(
        _Inout_  FILE*       const _Stream,
        _In_z_   char const* const _Format,
        _In_opt_ _locale_t   const _Locale,
                 va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return __stdio_common_vfprintf(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, _Stream, _Format, _Locale, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL vfprintf(
        _Inout_                       FILE*       const _Stream,
        _In_z_ _Printf_format_string_ char const* const _Format,
                                      va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vfprintf_l(_Stream, _Format, NULL, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vfprintf_s_l(
        _Inout_  FILE*       const _Stream,
        _In_z_   char const* const _Format,
        _In_opt_ _locale_t   const _Locale,
                 va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return __stdio_common_vfprintf_s(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, _Stream, _Format, _Locale, _ArgList);
    }
    #endif

    #if __STDC_WANT_SECURE_LIB__

        _Check_return_opt_
        _CRT_STDIO_INLINE int __CRTDECL vfprintf_s(
            _Inout_                       FILE*       const _Stream,
            _In_z_ _Printf_format_string_ char const* const _Format,
                                          va_list           _ArgList
            )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
        {
            return _vfprintf_s_l(_Stream, _Format, NULL, _ArgList);
        }
    #endif

    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vfprintf_p_l(
        _Inout_  FILE*       const _Stream,
        _In_z_   char const* const _Format,
        _In_opt_ _locale_t   const _Locale,
                 va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return __stdio_common_vfprintf_p(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS, _Stream, _Format, _Locale, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vfprintf_p(
        _Inout_                       FILE*       const _Stream,
        _In_z_ _Printf_format_string_ char const* const _Format,
                                      va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vfprintf_p_l(_Stream, _Format, NULL, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vprintf_l(
        _In_z_ _Printf_format_string_params_(2) char const* const _Format,
        _In_opt_                                _locale_t   const _Locale,
                                                va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vfprintf_l(stdout, _Format, _Locale, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL vprintf(
        _In_z_ _Printf_format_string_ char const* const _Format,
                                      va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vfprintf_l(stdout, _Format, NULL, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vprintf_s_l(
        _In_z_ _Printf_format_string_params_(2) char const* const _Format,
        _In_opt_                                _locale_t   const _Locale,
                                                va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vfprintf_s_l(stdout, _Format, _Locale, _ArgList);
    }
    #endif

    #if __STDC_WANT_SECURE_LIB__

        _Check_return_opt_
        _CRT_STDIO_INLINE int __CRTDECL vprintf_s(
            _In_z_ _Printf_format_string_ char const* const _Format,
                                          va_list           _ArgList
            )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
        {
            return _vfprintf_s_l(stdout, _Format, NULL, _ArgList);
        }
    #endif

    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vprintf_p_l(
        _In_z_ _Printf_format_string_params_(2) char const* const _Format,
        _In_opt_                                _locale_t   const _Locale,
                                                va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vfprintf_p_l(stdout, _Format, _Locale, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vprintf_p(
        _In_z_ _Printf_format_string_ char const* const _Format,
                                      va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vfprintf_p_l(stdout, _Format, NULL, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _fprintf_l(
        _Inout_                                 FILE*       const _Stream,
        _In_z_ _Printf_format_string_params_(0) char const* const _Format,
        _In_opt_                                _locale_t   const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vfprintf_l(_Stream, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL fprintf(
        _Inout_                       FILE*       const _Stream,
        _In_z_ _Printf_format_string_ char const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vfprintf_l(_Stream, _Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _ACRTIMP int __cdecl _set_printf_count_output(
        _In_ int _Value
        );

    _ACRTIMP int __cdecl _get_printf_count_output(void);

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _fprintf_s_l(
        _Inout_                                 FILE*       const _Stream,
        _In_z_ _Printf_format_string_params_(0) char const* const _Format,
        _In_opt_                                _locale_t   const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vfprintf_s_l(_Stream, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    #if __STDC_WANT_SECURE_LIB__

        _Check_return_opt_
        _CRT_STDIO_INLINE int __CRTDECL fprintf_s(
            _Inout_                       FILE*       const _Stream,
            _In_z_ _Printf_format_string_ char const* const _Format,
            ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
        {
            int _Result;
            va_list _ArgList;
            __crt_va_start(_ArgList, _Format);
            _Result = _vfprintf_s_l(_Stream, _Format, NULL, _ArgList);
            __crt_va_end(_ArgList);
            return _Result;
        }
    #endif

    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _fprintf_p_l(
        _Inout_                                 FILE*       const _Stream,
        _In_z_ _Printf_format_string_params_(0) char const* const _Format,
        _In_opt_                                _locale_t   const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vfprintf_p_l(_Stream, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _fprintf_p(
        _Inout_                       FILE*       const _Stream,
        _In_z_ _Printf_format_string_ char const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vfprintf_p_l(_Stream, _Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _printf_l(
        _In_z_ _Printf_format_string_params_(0) char const* const _Format,
        _In_opt_                                _locale_t   const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vfprintf_l(stdout, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL printf(
        _In_z_ _Printf_format_string_ char const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vfprintf_l(stdout, _Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _printf_s_l(
        _In_z_ _Printf_format_string_params_(0) char const* const _Format,
        _In_opt_                                _locale_t   const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vfprintf_s_l(stdout, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    #if __STDC_WANT_SECURE_LIB__

        _Check_return_opt_
        _CRT_STDIO_INLINE int __CRTDECL printf_s(
            _In_z_ _Printf_format_string_ char const* const _Format,
            ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
        {
            int _Result;
            va_list _ArgList;
            __crt_va_start(_ArgList, _Format);
            _Result = _vfprintf_s_l(stdout, _Format, NULL, _ArgList);
            __crt_va_end(_ArgList);
            return _Result;
        }
    #endif

    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _printf_p_l(
        _In_z_ _Printf_format_string_params_(0) char const* const _Format,
        _In_opt_                                _locale_t   const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vfprintf_p_l(stdout, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _printf_p(
        _In_z_ _Printf_format_string_ char const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vfprintf_p_l(stdout, _Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif


    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // Narrow Character Formatted Input Functions (Stream)
    //
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    _ACRTIMP int __cdecl __stdio_common_vfscanf(
        _In_                                   unsigned __int64 _Options,
        _Inout_                                FILE*            _Stream,
        _In_z_ _Scanf_format_string_params_(2) char const*      _Format,
        _In_opt_                               _locale_t        _Locale,
                                               va_list          _Arglist
        );

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vfscanf_l(
        _Inout_                       FILE*       const _Stream,
        _In_z_ _Printf_format_string_ char const* const _Format,
        _In_opt_                      _locale_t   const _Locale,
                                      va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return __stdio_common_vfscanf(
            _CRT_INTERNAL_LOCAL_SCANF_OPTIONS,
            _Stream, _Format, _Locale, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL vfscanf(
        _Inout_                       FILE*       const _Stream,
        _In_z_ _Printf_format_string_ char const* const _Format,
                                      va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vfscanf_l(_Stream, _Format, NULL, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vfscanf_s_l(
        _Inout_                       FILE*       const _Stream,
        _In_z_ _Printf_format_string_ char const* const _Format,
        _In_opt_                      _locale_t   const _Locale,
                                      va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return __stdio_common_vfscanf(
            _CRT_INTERNAL_LOCAL_SCANF_OPTIONS | _CRT_INTERNAL_SCANF_SECURECRT,
            _Stream, _Format, _Locale, _ArgList);
    }
    #endif


    #if __STDC_WANT_SECURE_LIB__

        _Check_return_opt_
        _CRT_STDIO_INLINE int __CRTDECL vfscanf_s(
            _Inout_                       FILE*       const _Stream,
            _In_z_ _Printf_format_string_ char const* const _Format,
                                          va_list           _ArgList
            )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
        {
            return _vfscanf_s_l(_Stream, _Format, NULL, _ArgList);
        }
    #endif

    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vscanf_l(
        _In_z_ _Printf_format_string_ char const* const _Format,
        _In_opt_                      _locale_t   const _Locale,
                                      va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vfscanf_l(stdin, _Format, _Locale, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL vscanf(
        _In_z_ _Printf_format_string_ char const* const _Format,
                                      va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vfscanf_l(stdin, _Format, NULL, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vscanf_s_l(
        _In_z_ _Printf_format_string_ char const* const _Format,
        _In_opt_                      _locale_t   const _Locale,
                                      va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vfscanf_s_l(stdin, _Format, _Locale, _ArgList);
    }
    #endif

    #if __STDC_WANT_SECURE_LIB__

        _Check_return_opt_
        _CRT_STDIO_INLINE int __CRTDECL vscanf_s(
            _In_z_ _Printf_format_string_ char const* const _Format,
                                          va_list           _ArgList
            )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
        {
            return _vfscanf_s_l(stdin, _Format, NULL, _ArgList);
        }
    #endif

    #endif

    _Check_return_opt_ _CRT_INSECURE_DEPRECATE(_fscanf_s_l)
    _CRT_STDIO_INLINE int __CRTDECL _fscanf_l(
        _Inout_                                FILE*       const _Stream,
        _In_z_ _Scanf_format_string_params_(0) char const* const _Format,
        _In_opt_                               _locale_t   const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vfscanf_l(_Stream, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_ _CRT_INSECURE_DEPRECATE(fscanf_s)
    _CRT_STDIO_INLINE int __CRTDECL fscanf(
        _Inout_                      FILE*       const _Stream,
        _In_z_ _Scanf_format_string_ char const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vfscanf_l(_Stream, _Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _fscanf_s_l(
        _Inout_                                  FILE*       const _Stream,
        _In_z_ _Scanf_s_format_string_params_(0) char const* const _Format,
        _In_opt_                                 _locale_t   const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vfscanf_s_l(_Stream, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    #if __STDC_WANT_SECURE_LIB__

        _Check_return_opt_
        _CRT_STDIO_INLINE int __CRTDECL fscanf_s(
            _Inout_                        FILE*       const _Stream,
            _In_z_ _Scanf_s_format_string_ char const* const _Format,
            ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
        {
            int _Result;
            va_list _ArgList;
            __crt_va_start(_ArgList, _Format);
            _Result = _vfscanf_s_l(_Stream, _Format, NULL, _ArgList);
            __crt_va_end(_ArgList);
            return _Result;
        }
    #endif

    #endif

    _Check_return_opt_ _CRT_INSECURE_DEPRECATE(_scanf_s_l)
    _CRT_STDIO_INLINE int __CRTDECL _scanf_l(
        _In_z_ _Scanf_format_string_params_(0) char const* const _Format,
        _In_opt_                               _locale_t   const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vfscanf_l(stdin, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_ _CRT_INSECURE_DEPRECATE(scanf_s)
    _CRT_STDIO_INLINE int __CRTDECL scanf(
        _In_z_ _Scanf_format_string_ char const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vfscanf_l(stdin, _Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _scanf_s_l(
        _In_z_ _Scanf_s_format_string_params_(0) char const* const _Format,
        _In_opt_                                 _locale_t   const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vfscanf_s_l(stdin, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    #if __STDC_WANT_SECURE_LIB__

        _Check_return_opt_
        _CRT_STDIO_INLINE int __CRTDECL scanf_s(
            _In_z_ _Scanf_s_format_string_ char const* const _Format,
            ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
        {
            int _Result;
            va_list _ArgList;
            __crt_va_start(_ArgList, _Format);
            _Result = _vfscanf_s_l(stdin, _Format, NULL, _ArgList);
            __crt_va_end(_ArgList);
            return _Result;
        }
    #endif

    #endif



    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // Narrow Character Formatted Output Functions (String)
    //
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    _Success_(return >= 0)
    _ACRTIMP int __cdecl __stdio_common_vsprintf(
        _In_                                    unsigned __int64 _Options,
        _Out_writes_opt_z_(_BufferCount)        char*            _Buffer,
        _In_                                    size_t           _BufferCount,
        _In_z_ _Printf_format_string_params_(2) char const*      _Format,
        _In_opt_                                _locale_t        _Locale,
                                                va_list          _ArgList
        );

    _Success_(return >= 0)
    _ACRTIMP int __cdecl __stdio_common_vsprintf_s(
        _In_                                    unsigned __int64 _Options,
        _Out_writes_z_(_BufferCount)            char*            _Buffer,
        _In_                                    size_t           _BufferCount,
        _In_z_ _Printf_format_string_params_(2) char const*      _Format,
        _In_opt_                                _locale_t        _Locale,
                                                va_list          _ArgList
        );

    _Success_(return >= 0)
    _ACRTIMP int __cdecl __stdio_common_vsnprintf_s(
        _In_                                    unsigned __int64 _Options,
        _Out_writes_opt_z_(_BufferCount)        char*            _Buffer,
        _In_                                    size_t           _BufferCount,
        _In_                                    size_t           _MaxCount,
        _In_z_ _Printf_format_string_params_(2) char const*      _Format,
        _In_opt_                                _locale_t        _Locale,
                                                va_list          _ArgList
        );

    _Success_(return >= 0)
    _ACRTIMP int __cdecl __stdio_common_vsprintf_p(
        _In_                                    unsigned __int64 _Options,
        _Out_writes_z_(_BufferCount)            char*            _Buffer,
        _In_                                    size_t           _BufferCount,
        _In_z_ _Printf_format_string_params_(2) char const*      _Format,
        _In_opt_                                _locale_t        _Locale,
                                                va_list          _ArgList
        );

    _Success_(return >= 0)
    _Check_return_opt_ _CRT_INSECURE_DEPRECATE(_vsnprintf_s_l)
    _CRT_STDIO_INLINE int __CRTDECL _vsnprintf_l(
        _Out_writes_opt_(_BufferCount) _Post_maybez_ char*       const _Buffer,
        _In_                                         size_t      const _BufferCount,
        _In_z_ _Printf_format_string_params_(2)      char const* const _Format,
        _In_opt_                                     _locale_t   const _Locale,
                                                     va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int const _Result = __stdio_common_vsprintf(
            _CRT_INTERNAL_LOCAL_PRINTF_OPTIONS | _CRT_INTERNAL_PRINTF_LEGACY_VSPRINTF_NULL_TERMINATION,
            _Buffer, _BufferCount, _Format, _Locale, _ArgList);

        return _Result < 0 ? -1 : _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vsnprintf(
        _Out_writes_opt_(_BufferCount) _Post_maybez_ char*       const _Buffer,
        _In_                                        size_t      const _BufferCount,
        _In_z_ _Printf_format_string_               char const* const _Format,
                                                    va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vsnprintf_l(_Buffer, _BufferCount, _Format, NULL, _ArgList);
    }
    #endif

    #if defined vsnprintf
        // This definition of vsnprintf will generate "warning C4005: 'vsnprintf': macro
        // redefinition" with a subsequent line indicating where the previous definition
        // of vsnprintf was.  This makes it easier to find where vsnprintf was defined.
        #pragma warning(push, 1)
        #pragma warning(1: 4005) // macro redefinition
        #define vsnprintf Do not define vsnprintf as a macro
        #pragma warning(pop)
        #error Macro definition of vsnprintf conflicts with Standard Library function declaration
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL vsnprintf(
        _Out_writes_opt_(_BufferCount) _Always_(_Post_z_) char*       const _Buffer,
        _In_                                              size_t      const _BufferCount,
        _In_z_ _Printf_format_string_                     char const* const _Format,
                                                          va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int const _Result = __stdio_common_vsprintf(
            _CRT_INTERNAL_LOCAL_PRINTF_OPTIONS | _CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR,
            _Buffer, _BufferCount, _Format, NULL, _ArgList);

        return _Result < 0 ? -1 : _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_ _CRT_INSECURE_DEPRECATE(_vsprintf_s_l)
    _CRT_STDIO_INLINE int __CRTDECL _vsprintf_l(
        _Pre_notnull_ _Always_(_Post_z_) char*       const _Buffer,
        _In_z_                           char const* const _Format,
        _In_opt_                         _locale_t   const _Locale,
                                         va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vsnprintf_l(_Buffer, (size_t)-1, _Format, _Locale, _ArgList);
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_ _CRT_INSECURE_DEPRECATE(vsprintf_s)
    _CRT_STDIO_INLINE int __CRTDECL vsprintf(
        _Pre_notnull_ _Always_(_Post_z_) char*       const _Buffer,
        _In_z_ _Printf_format_string_    char const* const _Format,
                                         va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vsnprintf_l(_Buffer, (size_t)-1, _Format, NULL, _ArgList);
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vsprintf_s_l(
        _Out_writes_(_BufferCount) _Always_(_Post_z_) char*       const _Buffer,
        _In_                                          size_t      const _BufferCount,
        _In_z_ _Printf_format_string_params_(2)       char const* const _Format,
        _In_opt_                                      _locale_t   const _Locale,
                                                      va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int const _Result = __stdio_common_vsprintf_s(
            _CRT_INTERNAL_LOCAL_PRINTF_OPTIONS,
            _Buffer, _BufferCount, _Format, _Locale, _ArgList);

        return _Result < 0 ? -1 : _Result;
    }
    #endif

    #if __STDC_WANT_SECURE_LIB__

        _Success_(return >= 0)
        _Check_return_opt_
        _CRT_STDIO_INLINE int __CRTDECL vsprintf_s(
            _Out_writes_(_BufferCount) _Always_(_Post_z_) char*       const _Buffer,
            _In_                                          size_t      const _BufferCount,
            _In_z_ _Printf_format_string_                 char const* const _Format,
                                                          va_list           _ArgList
            )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
        {
            return _vsprintf_s_l(_Buffer, _BufferCount, _Format, NULL, _ArgList);
        }
    #endif

        __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(
            _Success_(return >= 0)
            int, vsprintf_s,
            _Always_(_Post_z_)            char,        _Buffer,
            _In_z_ _Printf_format_string_ char const*, _Format,
                                          va_list,     _ArgList
            )

    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vsprintf_p_l(
        _Out_writes_(_BufferCount) _Always_(_Post_z_) char*       const _Buffer,
        _In_                                          size_t      const _BufferCount,
        _In_z_ _Printf_format_string_params_(2)       char const* const _Format,
        _In_opt_                                      _locale_t   const _Locale,
                                                      va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int const _Result = __stdio_common_vsprintf_p(
            _CRT_INTERNAL_LOCAL_PRINTF_OPTIONS,
            _Buffer, _BufferCount, _Format, _Locale, _ArgList);

        return _Result < 0 ? -1 : _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vsprintf_p(
        _Out_writes_(_BufferCount) _Always_(_Post_z_) char*       const _Buffer,
        _In_                                          size_t      const _BufferCount,
        _In_z_ _Printf_format_string_                 char const* const _Format,
                                                      va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vsprintf_p_l(_Buffer, _BufferCount, _Format, NULL, _ArgList);
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vsnprintf_s_l(
        _Out_writes_opt_(_BufferCount) _Always_(_Post_z_) char*       const _Buffer,
        _In_                                              size_t      const _BufferCount,
        _In_                                              size_t      const _MaxCount,
        _In_z_ _Printf_format_string_params_(2)           char const* const _Format,
        _In_opt_                                          _locale_t   const _Locale,
                                                          va_list          _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int const _Result = __stdio_common_vsnprintf_s(
            _CRT_INTERNAL_LOCAL_PRINTF_OPTIONS,
            _Buffer, _BufferCount, _MaxCount, _Format, _Locale, _ArgList);

        return _Result < 0 ? -1 : _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vsnprintf_s(
        _Out_writes_opt_(_BufferCount) _Always_(_Post_z_) char*       const _Buffer,
        _In_                                              size_t      const _BufferCount,
        _In_                                              size_t      const _MaxCount,
        _In_z_ _Printf_format_string_                     char const* const _Format,
                                                          va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vsnprintf_s_l(_Buffer, _BufferCount, _MaxCount, _Format, NULL, _ArgList);
    }
    #endif

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_3(
        _Success_(return >= 0)
        int, _vsnprintf_s,
        _Always_(_Post_z_)            char,        _Buffer,
        _In_                          size_t,      _BufferCount,
        _In_z_ _Printf_format_string_ char const*, _Format,
                                      va_list,     _ArgList
        )

    #if __STDC_WANT_SECURE_LIB__

        _Success_(return >= 0)
        _Check_return_opt_
        _CRT_STDIO_INLINE int __CRTDECL vsnprintf_s(
            _Out_writes_opt_(_BufferCount) _Always_(_Post_z_) char*       const _Buffer,
            _In_                                              size_t      const _BufferCount,
            _In_                                              size_t      const _MaxCount,
            _In_z_ _Printf_format_string_                     char const* const _Format,
                                                              va_list           _ArgList
            )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
        {
            return _vsnprintf_s_l(_Buffer, _BufferCount, _MaxCount, _Format, NULL, _ArgList);
        }
    #endif

        __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_3(
            _Success_(return >= 0)
            int, vsnprintf_s,
            _Always_(_Post_z_)            char,        _Buffer,
            _In_                          size_t,      _BufferCount,
            _In_z_ _Printf_format_string_ char const*, _Format,
                                          va_list,     _ArgList
            )

    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vscprintf_l(
        _In_z_ _Printf_format_string_params_(2) char const* const _Format,
        _In_opt_                                _locale_t   const _Locale,
                                                va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int const _Result = __stdio_common_vsprintf(
            _CRT_INTERNAL_LOCAL_PRINTF_OPTIONS | _CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR,
            NULL, 0, _Format, _Locale, _ArgList);

        return _Result < 0 ? -1 : _Result;
    }
    #endif

    _Check_return_
    _CRT_STDIO_INLINE int __CRTDECL _vscprintf(
        _In_z_ _Printf_format_string_ char const* const _Format,
                                      va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vscprintf_l(_Format, NULL, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vscprintf_p_l(
        _In_z_ _Printf_format_string_params_(2) char const* const _Format,
        _In_opt_                                _locale_t   const _Locale,
                                                va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int const _Result = __stdio_common_vsprintf_p(
            _CRT_INTERNAL_LOCAL_PRINTF_OPTIONS | _CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR,
            NULL, 0, _Format, _Locale, _ArgList);

        return _Result < 0 ? -1 : _Result;
    }
    #endif

    _Check_return_
    _CRT_STDIO_INLINE int __CRTDECL _vscprintf_p(
        _In_z_ _Printf_format_string_ char const* const _Format,
                                      va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vscprintf_p_l(_Format, NULL, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vsnprintf_c_l(
        _Out_writes_opt_(_BufferCount)          char*       const _Buffer,
        _In_                                    size_t      const _BufferCount,
        _In_z_ _Printf_format_string_params_(2) char const* const _Format,
        _In_opt_                                _locale_t   const _Locale,
                                                va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int const _Result = __stdio_common_vsprintf(
            _CRT_INTERNAL_LOCAL_PRINTF_OPTIONS,
            _Buffer, _BufferCount, _Format, _Locale, _ArgList);

        return _Result < 0 ? -1 : _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vsnprintf_c(
        _Out_writes_opt_(_BufferCount) char*       const _Buffer,
        _In_                           size_t      const _BufferCount,
        _In_z_ _Printf_format_string_  char const* const _Format,
                                       va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vsnprintf_c_l(_Buffer, _BufferCount, _Format, NULL, _ArgList);
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_ _CRT_INSECURE_DEPRECATE(_sprintf_s_l)
    _CRT_STDIO_INLINE int __CRTDECL _sprintf_l(
        _Pre_notnull_ _Always_(_Post_z_)        char*       const _Buffer,
        _In_z_ _Printf_format_string_params_(0) char const* const _Format,
        _In_opt_                                _locale_t   const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);

        _Result = _vsprintf_l(_Buffer, _Format, _Locale, _ArgList);

        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL sprintf(
        _Pre_notnull_ _Always_(_Post_z_) char*       const _Buffer,
        _In_z_ _Printf_format_string_    char const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);

        _Result = _vsprintf_l(_Buffer, _Format, NULL, _ArgList);

        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_1_ARGLIST(
        _Success_(return >= 0)
        int, __RETURN_POLICY_SAME, __EMPTY_DECLSPEC, __CRTDECL, sprintf, vsprintf,
        _Pre_notnull_ _Always_(_Post_z_), char,        _Buffer,
        _In_z_ _Printf_format_string_     char const*, _Format
        )

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _sprintf_s_l(
        _Out_writes_(_BufferCount) _Always_(_Post_z_) char*       const _Buffer,
        _In_                                          size_t      const _BufferCount,
        _In_z_ _Printf_format_string_params_(0)       char const* const _Format,
        _In_opt_                                      _locale_t   const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vsprintf_s_l(_Buffer, _BufferCount, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    #if __STDC_WANT_SECURE_LIB__

        _Success_(return >= 0)
        _Check_return_opt_
        _CRT_STDIO_INLINE int __CRTDECL sprintf_s(
            _Out_writes_(_BufferCount) _Always_(_Post_z_) char*       const _Buffer,
            _In_                                          size_t      const _BufferCount,
            _In_z_ _Printf_format_string_                 char const* const _Format,
            ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
        {
            int _Result;
            va_list _ArgList;
            __crt_va_start(_ArgList, _Format);
            _Result = _vsprintf_s_l(_Buffer, _BufferCount, _Format, NULL, _ArgList);
            __crt_va_end(_ArgList);
            return _Result;
        }
    #endif

    #endif

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1_ARGLIST(
        _Success_(return >= 0)
        int, sprintf_s, vsprintf_s,
        _Always_(_Post_z_)            char,        _Buffer,
        _In_z_ _Printf_format_string_ char const*, _Format
        )

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _sprintf_p_l(
        _Out_writes_(_BufferCount) _Always_(_Post_z_) char*       const _Buffer,
        _In_                                          size_t      const _BufferCount,
        _In_z_ _Printf_format_string_params_(0)       char const* const _Format,
        _In_opt_                                      _locale_t   const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vsprintf_p_l(_Buffer, _BufferCount, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _sprintf_p(
        _Out_writes_(_BufferCount) _Always_(_Post_z_) char*       const _Buffer,
        _In_                                          size_t      const _BufferCount,
        _In_z_ _Printf_format_string_                 char const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vsprintf_p_l(_Buffer, _BufferCount, _Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_ _CRT_INSECURE_DEPRECATE(_snprintf_s_l)
    _CRT_STDIO_INLINE int __CRTDECL _snprintf_l(
        _Out_writes_opt_(_BufferCount) _Post_maybez_ char*       const _Buffer,
        _In_                                         size_t      const _BufferCount,
        _In_z_ _Printf_format_string_params_(0)      char const* const _Format,
        _In_opt_                                     _locale_t   const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);

        _Result = _vsnprintf_l(_Buffer, _BufferCount, _Format, _Locale, _ArgList);

        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    #if defined snprintf
        // This definition of snprintf will generate "warning C4005: 'snprintf': macro
        // redefinition" with a subsequent line indicating where the previous definition
        // of snprintf was.  This makes it easier to find where snprintf was defined.
        #pragma warning(push, 1)
        #pragma warning(1: 4005) // macro redefinition
        #define snprintf Do not define snprintf as a macro
        #pragma warning(pop)
        #error Macro definition of snprintf conflicts with Standard Library function declaration
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL snprintf(
        _Out_writes_opt_(_BufferCount) _Always_(_Post_z_) char*       const _Buffer,
        _In_                                              size_t      const _BufferCount,
        _In_z_ _Printf_format_string_                     char const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = vsnprintf(_Buffer, _BufferCount, _Format, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _snprintf(
        _Out_writes_opt_(_BufferCount) _Post_maybez_ char*       const _Buffer,
        _In_                                         size_t      const _BufferCount,
        _In_z_ _Printf_format_string_                char const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vsnprintf(_Buffer, _BufferCount, _Format, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    __DEFINE_CPP_OVERLOAD_STANDARD_NFUNC_0_2_ARGLIST_EX(
        _Success_(return >= 0)
        int, __RETURN_POLICY_SAME, __EMPTY_DECLSPEC, __CRTDECL, _snprintf, _vsnprintf,
        _Pre_notnull_ _Post_maybez_                   char,
        _Out_writes_opt_(_BufferCount) _Post_maybez_, char,        _Buffer,
        _In_                                          size_t,      _BufferCount,
        _In_z_ _Printf_format_string_                 char const*, _Format
        )

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _snprintf_c_l(
        _Out_writes_opt_(_BufferCount)          char*       const _Buffer,
        _In_                                    size_t      const _BufferCount,
        _In_z_ _Printf_format_string_params_(0) char const* const _Format,
        _In_opt_                                _locale_t   const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vsnprintf_c_l(_Buffer, _BufferCount, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _snprintf_c(
        _Out_writes_opt_(_BufferCount) char*       const _Buffer,
        _In_                           size_t      const _BufferCount,
        _In_z_ _Printf_format_string_  char const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vsnprintf_c_l(_Buffer, _BufferCount, _Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _snprintf_s_l(
        _Out_writes_opt_(_BufferCount) _Always_(_Post_z_) char*       const _Buffer,
        _In_                                              size_t      const _BufferCount,
        _In_                                              size_t      const _MaxCount,
        _In_z_ _Printf_format_string_params_(0)           char const* const _Format,
        _In_opt_                                          _locale_t   const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vsnprintf_s_l(_Buffer, _BufferCount, _MaxCount, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Success_(return >= 0)
    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _snprintf_s(
        _Out_writes_opt_(_BufferCount) _Always_(_Post_z_) char*       const _Buffer,
        _In_                                              size_t      const _BufferCount,
        _In_                                              size_t      const _MaxCount,
        _In_z_ _Printf_format_string_                     char const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vsnprintf_s_l(_Buffer, _BufferCount, _MaxCount, _Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2_ARGLIST(
        _Success_(return >= 0)
        int, _snprintf_s, _vsnprintf_s,
        _Always_(_Post_z_)            char,        _Buffer,
        _In_                          size_t,      _BufferCount,
        _In_z_ _Printf_format_string_ char const*, _Format
        )

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _scprintf_l(
        _In_z_ _Printf_format_string_params_(0) char const* const _Format,
        _In_opt_                                _locale_t   const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vscprintf_l(_Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_
    _CRT_STDIO_INLINE int __CRTDECL _scprintf(
        _In_z_ _Printf_format_string_ char const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vscprintf_l(_Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _scprintf_p_l(
        _In_z_ _Printf_format_string_params_(0) char const* const _Format,
        _In_opt_                                _locale_t   const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vscprintf_p_l(_Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_
    _CRT_STDIO_INLINE int __CRTDECL _scprintf_p(
        _In_z_ _Printf_format_string_ char const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vscprintf_p(_Format, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // Narrow Character Formatted Input Functions (String)
    //
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    _ACRTIMP int __cdecl __stdio_common_vsscanf(
        _In_                                   unsigned __int64 _Options,
        _In_reads_(_BufferCount) _Pre_z_       char const*      _Buffer,
        _In_                                   size_t           _BufferCount,
        _In_z_ _Scanf_format_string_params_(2) char const*      _Format,
        _In_opt_                               _locale_t        _Locale,
                                               va_list          _ArgList
        );

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vsscanf_l(
        _In_z_                        char const* const _Buffer,
        _In_z_ _Printf_format_string_ char const* const _Format,
        _In_opt_                      _locale_t   const _Locale,
                                      va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return __stdio_common_vsscanf(
            _CRT_INTERNAL_LOCAL_SCANF_OPTIONS,
            _Buffer, (size_t)-1, _Format, _Locale, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL vsscanf(
        _In_z_                        char const* const _Buffer,
        _In_z_ _Printf_format_string_ char const* const _Format,
                                      va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return _vsscanf_l(_Buffer, _Format, NULL, _ArgList);
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _vsscanf_s_l(
        _In_z_                        char const* const _Buffer,
        _In_z_ _Printf_format_string_ char const* const _Format,
        _In_opt_                      _locale_t   const _Locale,
                                      va_list           _ArgList
        )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
    {
        return __stdio_common_vsscanf(
            _CRT_INTERNAL_LOCAL_SCANF_OPTIONS | _CRT_INTERNAL_SCANF_SECURECRT,
            _Buffer, (size_t)-1, _Format, _Locale, _ArgList);
    }
    #endif

    #if __STDC_WANT_SECURE_LIB__

        #pragma warning(push)
        #pragma warning(disable: 6530) // Unrecognized SAL format string

        _Check_return_opt_
        _CRT_STDIO_INLINE int __CRTDECL vsscanf_s(
            _In_z_                        char const* const _Buffer,
            _In_z_ _Printf_format_string_ char const* const _Format,
                                          va_list           _ArgList
            )
    #if defined _NO_CRT_STDIO_INLINE
    ;
    #else
        {
            return _vsscanf_s_l(_Buffer, _Format, NULL, _ArgList);
        }
    #endif

        __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_2(
            int, vsscanf_s,
            _In_z_                        char const,  _Buffer,
            _In_z_ _Printf_format_string_ char const*, _Format,
                                          va_list,     _ArgList
            )

        #pragma warning(pop)

    #endif

    _Check_return_opt_ _CRT_INSECURE_DEPRECATE(_sscanf_s_l)
    _CRT_STDIO_INLINE int __CRTDECL _sscanf_l(
        _In_z_                                 char const* const _Buffer,
        _In_z_ _Scanf_format_string_params_(0) char const* const _Format,
        _In_opt_                               _locale_t   const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vsscanf_l(_Buffer, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_ _CRT_INSECURE_DEPRECATE(sscanf_s)
    _CRT_STDIO_INLINE int __CRTDECL sscanf(
        _In_z_                       char const* const _Buffer,
        _In_z_ _Scanf_format_string_ char const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);
        _Result = _vsscanf_l(_Buffer, _Format, NULL, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _sscanf_s_l(
        _In_z_                                   char const* const _Buffer,
        _In_z_ _Scanf_s_format_string_params_(0) char const* const _Format,
        _In_opt_                                 _locale_t   const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);
        _Result = _vsscanf_s_l(_Buffer, _Format, _Locale, _ArgList);
        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    #if __STDC_WANT_SECURE_LIB__

        _Check_return_opt_
        _CRT_STDIO_INLINE int __CRTDECL sscanf_s(
            _In_z_                         char const* const _Buffer,
            _In_z_ _Scanf_s_format_string_ char const* const _Format,
            ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
            ;
    #else
        {
            int _Result;
            va_list _ArgList;
            __crt_va_start(_ArgList, _Format);

            _Result = vsscanf_s(_Buffer, _Format, _ArgList);

            __crt_va_end(_ArgList);
            return _Result;
        }
    #endif

    #endif

    #pragma warning(push)
    #pragma warning(disable: 6530) // Unrecognized SAL format string

    _Check_return_opt_ _CRT_INSECURE_DEPRECATE(_snscanf_s_l)
    _CRT_STDIO_INLINE int __CRTDECL _snscanf_l(
        _In_reads_bytes_(_BufferCount) _Pre_z_ char const* const _Buffer,
        _In_                                   size_t      const _BufferCount,
        _In_z_ _Scanf_format_string_params_(0) char const* const _Format,
        _In_opt_                               _locale_t   const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);

        _Result = __stdio_common_vsscanf(
            _CRT_INTERNAL_LOCAL_SCANF_OPTIONS,
            _Buffer, _BufferCount, _Format, _Locale, _ArgList);

        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_opt_ _CRT_INSECURE_DEPRECATE(_snscanf_s)
    _CRT_STDIO_INLINE int __CRTDECL _snscanf(
        _In_reads_bytes_(_BufferCount) _Pre_z_ char const* const _Buffer,
        _In_                                   size_t      const _BufferCount,
        _In_z_ _Scanf_format_string_           char const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);

        _Result = __stdio_common_vsscanf(
            _CRT_INTERNAL_LOCAL_SCANF_OPTIONS,
            _Buffer, _BufferCount, _Format, NULL, _ArgList);

        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif


    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _snscanf_s_l(
        _In_reads_bytes_(_BufferCount) _Pre_z_   char const* const _Buffer,
        _In_                                     size_t      const _BufferCount,
        _In_z_ _Scanf_s_format_string_params_(0) char const* const _Format,
        _In_opt_                                 _locale_t   const _Locale,
        ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Locale);

        _Result = __stdio_common_vsscanf(
            _CRT_INTERNAL_LOCAL_SCANF_OPTIONS | _CRT_INTERNAL_SCANF_SECURECRT,
            _Buffer, _BufferCount, _Format, _Locale, _ArgList);

        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    _Check_return_opt_
    _CRT_STDIO_INLINE int __CRTDECL _snscanf_s(
        _In_reads_bytes_(_BufferCount) _Pre_z_ char const* const _Buffer,
        _In_                                   size_t      const _BufferCount,
        _In_z_ _Scanf_s_format_string_         char const* const _Format,
        ...)
    #if defined _NO_CRT_STDIO_INLINE // SCANF
        ;
    #else
    {
        int _Result;
        va_list _ArgList;
        __crt_va_start(_ArgList, _Format);

        _Result = __stdio_common_vsscanf(
            _CRT_INTERNAL_LOCAL_SCANF_OPTIONS | _CRT_INTERNAL_SCANF_SECURECRT,
            _Buffer, _BufferCount, _Format, NULL, _ArgList);

        __crt_va_end(_ArgList);
        return _Result;
    }
    #endif

    #pragma warning(pop)

    #if defined _M_CEE_MIXED
        #pragma managed(pop)
    #endif



    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // Non-ANSI Names for Compatibility
    //
    //-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    #if defined(_CRT_INTERNAL_NONSTDC_NAMES) && _CRT_INTERNAL_NONSTDC_NAMES

        #define SYS_OPEN  _SYS_OPEN

        #if defined _DEBUG && defined _CRTDBG_MAP_ALLOC
            #pragma push_macro("tempnam")
            #undef tempnam
        #endif

        _CRT_NONSTDC_DEPRECATE(_tempnam)
        _ACRTIMP char* __cdecl tempnam(
            _In_opt_z_ char const* _Directory,
            _In_opt_z_ char const* _FilePrefix
            );

        #if defined _DEBUG && defined _CRTDBG_MAP_ALLOC
            #pragma pop_macro("tempnam")
        #endif

        _Check_return_opt_ _CRT_NONSTDC_DEPRECATE(_fcloseall) _ACRTIMP int   __cdecl fcloseall(void);
        _Check_return_     _CRT_NONSTDC_DEPRECATE(_fdopen)    _ACRTIMP FILE* __cdecl fdopen(_In_ int _FileHandle, _In_z_ char const* _Format);
        _Check_return_opt_ _CRT_NONSTDC_DEPRECATE(_fgetchar)  _ACRTIMP int   __cdecl fgetchar(void);
        _Check_return_     _CRT_NONSTDC_DEPRECATE(_fileno)    _ACRTIMP int   __cdecl fileno(_In_ FILE* _Stream);
        _Check_return_opt_ _CRT_NONSTDC_DEPRECATE(_flushall)  _ACRTIMP int   __cdecl flushall(void);
        _Check_return_opt_ _CRT_NONSTDC_DEPRECATE(_fputchar)  _ACRTIMP int   __cdecl fputchar(_In_ int _Ch);
        _Check_return_     _CRT_NONSTDC_DEPRECATE(_getw)      _ACRTIMP int   __cdecl getw(_Inout_ FILE* _Stream);
        _Check_return_opt_ _CRT_NONSTDC_DEPRECATE(_putw)      _ACRTIMP int   __cdecl putw(_In_ int _Ch, _Inout_ FILE* _Stream);
        _Check_return_     _CRT_NONSTDC_DEPRECATE(_rmtmp)     _ACRTIMP int   __cdecl rmtmp(void);

    #endif // _CRT_INTERNAL_NONSTDC_NAMES
#endif // _CRT_FUNCTIONS_REQUIRED



_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
#endif // _INC_STDIO
