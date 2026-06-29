//
// corecrt_io.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// This file declares the low-level I/O and file handling functionality.  These
// declarations are split out to support the Windows build.
//
#pragma once

#include <corecrt_share.h>
#include <corecrt_wio.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Types
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#ifdef _USE_32BIT_TIME_T
    #define _finddata_t     _finddata32_t
    #define _finddatai64_t  _finddata32i64_t
#else
    #define _finddata_t     _finddata64i32_t
    #define _finddatai64_t  __finddata64_t
#endif

struct _finddata32_t
{
    unsigned    attrib;
    __time32_t  time_create;    // -1 for FAT file systems
    __time32_t  time_access;    // -1 for FAT file systems
    __time32_t  time_write;
    _fsize_t    size;
    char        name[260];
};

struct _finddata32i64_t
{
    unsigned    attrib;
    __time32_t  time_create;    // -1 for FAT file systems
    __time32_t  time_access;    // -1 for FAT file systems
    __time32_t  time_write;
    __int64     size;
    char        name[260];
};

struct _finddata64i32_t
{
    unsigned    attrib;
    __time64_t  time_create;    // -1 for FAT file systems
    __time64_t  time_access;    // -1 for FAT file systems
    __time64_t  time_write;
    _fsize_t    size;
    char        name[260];
};

struct __finddata64_t
{
    unsigned    attrib;
    __time64_t  time_create;    // -1 for FAT file systems
    __time64_t  time_access;    // -1 for FAT file systems
    __time64_t  time_write;
    __int64     size;
    char        name[260];
};

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Macros
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// File attribute constants for the _findfirst() family of functions
#define _A_NORMAL 0x00 // Normal file - No read/write restrictions
#define _A_RDONLY 0x01 // Read only file
#define _A_HIDDEN 0x02 // Hidden file
#define _A_SYSTEM 0x04 // System file
#define _A_SUBDIR 0x10 // Subdirectory
#define _A_ARCH   0x20 // Archive file



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Functions
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#ifdef _USE_32BIT_TIME_T
    #define _findfirst      _findfirst32
    #define _findnext       _findnext32
    #define _findfirsti64   _findfirst32i64
    #define _findnexti64     _findnext32i64
#else
    #define _findfirst      _findfirst64i32
    #define _findnext       _findnext64i32
    #define _findfirsti64   _findfirst64
    #define _findnexti64    _findnext64
#endif

#if _CRT_FUNCTIONS_REQUIRED

    _Check_return_
    _ACRTIMP int __cdecl _access(
        _In_z_ char const* _FileName,
        _In_   int         _AccessMode
        );

    _Check_return_wat_
    _ACRTIMP errno_t __cdecl _access_s(
        _In_z_ char const* _FileName,
        _In_   int         _AccessMode
        );

    _Check_return_
    _ACRTIMP int __cdecl _chmod(
        _In_z_ char const* _FileName,
        _In_   int         _Mode
        );

    _Check_return_
    _ACRTIMP int __cdecl _chsize(
        _In_ int  _FileHandle,
        _In_ long _Size
        );

    _Check_return_wat_
    _ACRTIMP errno_t __cdecl _chsize_s(
        _In_ int     _FileHandle,
        _In_ __int64 _Size
        );

    _Check_return_opt_
    _ACRTIMP int __cdecl _close(
        _In_ int _FileHandle
        );

    _Check_return_opt_
    _ACRTIMP int __cdecl _commit(
        _In_ int _FileHandle
        );

    _Check_return_ _CRT_INSECURE_DEPRECATE(_sopen_s)
    _ACRTIMP int __cdecl _creat(
        _In_z_ char const* _FileName,
        _In_   int         _PermissionMode
        );

    _Check_return_
    _ACRTIMP int __cdecl _dup(
        _In_ int _FileHandle
        );

    _Check_return_
    _ACRTIMP int __cdecl _dup2(
        _In_ int _FileHandleSrc,
        _In_ int _FileHandleDst
        );

    _Check_return_
    _ACRTIMP int __cdecl _eof(
        _In_ int _FileHandle
        );

    _Check_return_
    _ACRTIMP long __cdecl _filelength(
        _In_ int _FileHandle
        );

    _Success_(return != -1)
    _Check_return_
    _ACRTIMP intptr_t __cdecl _findfirst32(
        _In_z_ char const*           _FileName,
        _Out_  struct _finddata32_t* _FindData
        );

    _Success_(return != -1)
    _Check_return_
    _ACRTIMP int __cdecl _findnext32(
        _In_  intptr_t              _FindHandle,
        _Out_ struct _finddata32_t* _FindData
        );

    _Check_return_opt_
    _ACRTIMP int __cdecl _findclose(
        _In_ intptr_t _FindHandle
        );

    _ACRTIMP intptr_t __cdecl _get_osfhandle(
            _In_ int _FileHandle
            );

    _Check_return_
    _ACRTIMP int __cdecl _isatty(
        _In_ int _FileHandle
        );

    _ACRTIMP int __cdecl _locking(
            _In_ int  _FileHandle,
            _In_ int  _LockMode,
            _In_ long _NumOfBytes
            );

    _Check_return_opt_
    _ACRTIMP long __cdecl _lseek(
        _In_ int  _FileHandle,
        _In_ long _Offset,
        _In_ int  _Origin
        );

    _Success_(return == 0)
    _Check_return_wat_
    _ACRTIMP errno_t __cdecl _mktemp_s(
        _Inout_updates_z_(_Size) char*  _TemplateName,
        _In_                     size_t _Size
        );

    __DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_0(
        errno_t, _mktemp_s,
        _Prepost_z_ char, _TemplateName
        )

    _Success_(return != 0)
    _Check_return_ __DEFINE_CPP_OVERLOAD_STANDARD_FUNC_0_0(
        char *, __RETURN_POLICY_DST, _ACRTIMP, _mktemp,
        _Inout_z_, char, _TemplateName
        )

    _ACRTIMP int __cdecl _open_osfhandle(
        _In_ intptr_t _OSFileHandle,
        _In_ int      _Flags
        );

    #ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP
        _Success_(return != -1)
        _Check_return_
        _DCRTIMP int __cdecl _pipe(
            _Out_writes_(2)    int*         _PtHandles,
            _In_               unsigned int _PipeSize,
            _In_               int          _TextMode
            );
    #endif

    _Success_(return != -1)
    _Check_return_
    _ACRTIMP int __cdecl _read(
        _In_                              int          _FileHandle,
        _Out_writes_bytes_(_MaxCharCount) void*        _DstBuf,
        _In_                              unsigned int _MaxCharCount
        );

    _ACRTIMP int __cdecl remove(
        _In_z_ char const* _FileName
        );

    _Check_return_
    _ACRTIMP int __cdecl rename(
        _In_z_ char const* _OldFilename,
        _In_z_ char const* _NewFilename
        );

    _ACRTIMP int __cdecl _unlink(
        _In_z_ char const* _FileName
        );

    _Check_return_
    _ACRTIMP int __cdecl _setmode(
        _In_ int _FileHandle,
        _In_ int _Mode
        );

    _Check_return_
    _ACRTIMP long __cdecl _tell(
        _In_ int _FileHandle
        );

    _CRT_INSECURE_DEPRECATE(_umask_s)
    _ACRTIMP int __cdecl _umask(
        _In_ int _Mode
        );

    _Check_return_wat_
    _ACRTIMP errno_t __cdecl _umask_s(
        _In_  int  _NewMode,
        _Out_ int* _OldMode
        );

    _ACRTIMP int __cdecl _write(
        _In_                            int          _FileHandle,
        _In_reads_bytes_(_MaxCharCount) void const*  _Buf,
        _In_                            unsigned int _MaxCharCount
        );

    _Check_return_
    _ACRTIMP __int64 __cdecl _filelengthi64(
        _In_ int _FileHandle
        );

    _Success_(return != -1)
    _Check_return_
    _ACRTIMP intptr_t __cdecl _findfirst32i64(
        _In_z_ char const*              _FileName,
        _Out_  struct _finddata32i64_t* _FindData
        );

    _Success_(return != -1)
    _Check_return_
    _ACRTIMP intptr_t __cdecl _findfirst64i32(
        _In_z_ char const*              _FileName,
        _Out_  struct _finddata64i32_t* _FindData
        );

    _Success_(return != -1)
    _Check_return_
    _ACRTIMP intptr_t __cdecl _findfirst64(
        _In_z_ char const*            _FileName,
        _Out_  struct __finddata64_t* _FindData
        );

    _Success_(return != -1)
    _Check_return_
    _ACRTIMP int __cdecl _findnext32i64(
        _In_  intptr_t                 _FindHandle,
        _Out_ struct _finddata32i64_t* _FindData
        );

    _Success_(return != -1)
    _Check_return_
    _ACRTIMP int __cdecl _findnext64i32(
        _In_  intptr_t                 _FindHandle,
        _Out_ struct _finddata64i32_t* _FindData
        );

    _Success_(return != -1)
    _Check_return_
    _ACRTIMP int __cdecl _findnext64(
        _In_  intptr_t               _FindHandle,
        _Out_ struct __finddata64_t* _FindData
        );

    _Check_return_opt_
    _ACRTIMP __int64 __cdecl _lseeki64(
        _In_ int     _FileHandle,
        _In_ __int64 _Offset,
        _In_ int     _Origin
        );

    _Check_return_
    _ACRTIMP __int64 __cdecl _telli64(
        _In_ int _FileHandle
        );

    _Check_return_wat_
    _ACRTIMP errno_t __cdecl _sopen_s(
        _Out_  int*        _FileHandle,
        _In_z_ char const* _FileName,
        _In_   int         _OpenFlag,
        _In_   int         _ShareFlag,
        _In_   int         _PermissionMode
        );

    _Check_return_
    _ACRTIMP errno_t __cdecl _sopen_s_nolock(
        _Out_  int*        _FileHandle,
        _In_z_ char const* _FileName,
        _In_   int         _OpenFlag,
        _In_   int         _ShareFlag,
        _In_   int         _PermissionMode
        );

    _ACRTIMP errno_t __cdecl _sopen_dispatch(
        _In_z_ char const* _FileName,
        _In_   int         _OFlag,
        _In_   int         _ShFlag,
        _In_   int         _PMode,
        _Out_  int*        _PFileHandle,
        _In_   int         _BSecure
        );



    #ifdef __cplusplus

        // These function do not validate pmode; use _sopen_s instead.
        extern "C++" _Check_return_ _CRT_INSECURE_DEPRECATE(_sopen_s)
        inline int __CRTDECL _open(
            _In_z_ char const* const _FileName,
            _In_   int         const _OFlag,
            _In_   int         const _PMode = 0
            )
        {
            int _FileHandle;
            // Last parameter passed as 0 because we don't want to validate pmode from _open
            errno_t const _Result = _sopen_dispatch(_FileName, _OFlag, _SH_DENYNO, _PMode, &_FileHandle, 0);
            return _Result ? -1 : _FileHandle;
        }

        extern "C++" _Check_return_ _CRT_INSECURE_DEPRECATE(_sopen_s)
        inline int __CRTDECL _sopen(
            _In_z_ char const* const _FileName,
            _In_   int         const _OFlag,
            _In_   int         const _ShFlag,
            _In_   int         const _PMode = 0
            )
        {
            int _FileHandle;
            // Last parameter passed as 0 because we don't want to validate pmode from _sopen
            errno_t const _Result = _sopen_dispatch(_FileName, _OFlag, _ShFlag, _PMode, &_FileHandle, 0);
            return _Result ? -1 : _FileHandle;
        }

    #else

        _Check_return_ _CRT_INSECURE_DEPRECATE(_sopen_s)
        _ACRTIMP int __cdecl _open(
            _In_z_ char const* _FileName,
            _In_   int         _OpenFlag,
            ...);

        _Check_return_ _CRT_INSECURE_DEPRECATE(_sopen_s)
        _ACRTIMP int __cdecl _sopen(
            _In_z_ char const* _FileName,
            _In_   int         _OpenFlag,
            _In_   int         _ShareFlag,
            ...);

    #endif



    #if defined(_CRT_INTERNAL_NONSTDC_NAMES) && _CRT_INTERNAL_NONSTDC_NAMES
        // Suppress warnings about double deprecation
        #pragma warning(push)
        #pragma warning(disable: 4141)

        _Check_return_ _CRT_NONSTDC_DEPRECATE(_access)
        _ACRTIMP int __cdecl access(
            _In_z_ char const* _FileName,
            _In_   int         _AccessMode
            );

        _Check_return_ _CRT_NONSTDC_DEPRECATE(_chmod)
        _ACRTIMP int __cdecl chmod(
            _In_z_ char const* _FileName,
            _In_   int         _AccessMode
            );

        _Check_return_ _CRT_NONSTDC_DEPRECATE(_chsize)
        _ACRTIMP int __cdecl chsize(
            _In_ int  _FileHandle,
            _In_ long _Size
            );

        _Check_return_opt_ _CRT_NONSTDC_DEPRECATE(_close)
        _ACRTIMP int __cdecl close(
            _In_ int _FileHandle
        );

        _Check_return_ _CRT_NONSTDC_DEPRECATE(_creat) _CRT_INSECURE_DEPRECATE(_sopen_s)
        _ACRTIMP int __cdecl creat(
            _In_z_ char const* _FileName,
            _In_   int         _PermissionMode
            );

        _Check_return_ _CRT_NONSTDC_DEPRECATE(_dup)
        _ACRTIMP int __cdecl dup(
            _In_ int _FileHandle
            );

        _Check_return_ _CRT_NONSTDC_DEPRECATE(_dup2)
        _ACRTIMP int __cdecl dup2(
            _In_ int _FileHandleSrc,
            _In_ int _FileHandleDst
            );

        _Check_return_ _CRT_NONSTDC_DEPRECATE(_eof)
        _ACRTIMP int __cdecl eof(
            _In_ int _FileHandle
            );

        _Check_return_ _CRT_NONSTDC_DEPRECATE(_filelength)
        _ACRTIMP long __cdecl filelength(
            _In_ int _FileHandle
            );

        _Check_return_ _CRT_NONSTDC_DEPRECATE(_isatty)
        _ACRTIMP int __cdecl isatty(
            _In_ int _FileHandle
            );

        _Check_return_ _CRT_NONSTDC_DEPRECATE(_locking)
        _ACRTIMP int __cdecl locking(
            _In_ int  _FileHandle,
            _In_ int  _LockMode,
            _In_ long _NumOfBytes
            );

        _Check_return_opt_ _CRT_NONSTDC_DEPRECATE(_lseek)
        _ACRTIMP long __cdecl lseek(
            _In_ int  _FileHandle,
            _In_ long _Offset,
            _In_ int  _Origin
            );

        _Success_(return != 0)
        _CRT_NONSTDC_DEPRECATE(_mktemp) _CRT_INSECURE_DEPRECATE(_mktemp_s)
        _ACRTIMP char * __cdecl mktemp(
            _Inout_z_ char* _TemplateName
            );

        _Check_return_ _CRT_NONSTDC_DEPRECATE(_open) _CRT_INSECURE_DEPRECATE(_sopen_s)
        _ACRTIMP int __cdecl open(
            _In_z_ char const* _FileName,
            _In_   int         _OpenFlag,
            ...);

        _Success_(return != -1)
        _CRT_NONSTDC_DEPRECATE(_read)
        _ACRTIMP int __cdecl read(
            _In_                              int          _FileHandle,
            _Out_writes_bytes_(_MaxCharCount) void*        _DstBuf,
            _In_                              unsigned int _MaxCharCount
            );

        _Check_return_ _CRT_NONSTDC_DEPRECATE(_setmode)
        _ACRTIMP int __cdecl setmode(
            _In_ int _FileHandle,
            _In_ int _Mode
            );

        _CRT_NONSTDC_DEPRECATE(_sopen) _CRT_INSECURE_DEPRECATE(_sopen_s)
        _ACRTIMP int __cdecl sopen(
            _In_ char const* _FileName,
            _In_ int         _OpenFlag,
            _In_ int         _ShareFlag,
            ...);

        _Check_return_ _CRT_NONSTDC_DEPRECATE(_tell)
        _ACRTIMP long __cdecl tell(
            _In_ int _FileHandle
            );

        _CRT_NONSTDC_DEPRECATE(_umask) _CRT_INSECURE_DEPRECATE(_umask_s)
        _ACRTIMP int __cdecl umask(
            _In_ int _Mode
            );

        _CRT_NONSTDC_DEPRECATE(_unlink)
        _ACRTIMP int __cdecl unlink(
            _In_z_ char const* _FileName
            );

        _Check_return_ _CRT_NONSTDC_DEPRECATE(_write)
        _ACRTIMP int __cdecl write(
            _In_                            int          _FileHandle,
            _In_reads_bytes_(_MaxCharCount) void const*  _Buf,
            _In_                            unsigned int _MaxCharCount
            );

        #pragma warning(pop)
    #endif // _CRT_INTERNAL_NONSTDC_NAMES
#endif // _CRT_FUNCTIONS_REQUIRED

_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
