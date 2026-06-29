//
// sys/stat.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The _stat() and _fstat() families of functions.
//
#pragma once

#include <corecrt.h>
#include <sys/types.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Types
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
struct _stat32
{
    _dev_t         st_dev;
    _ino_t         st_ino;
    unsigned short st_mode;
    short          st_nlink;
    short          st_uid;
    short          st_gid;
    _dev_t         st_rdev;
    _off_t         st_size;
    __time32_t     st_atime;
    __time32_t     st_mtime;
    __time32_t     st_ctime;
};

struct _stat32i64
{
    _dev_t         st_dev;
    _ino_t         st_ino;
    unsigned short st_mode;
    short          st_nlink;
    short          st_uid;
    short          st_gid;
    _dev_t         st_rdev;
    __int64        st_size;
    __time32_t     st_atime;
    __time32_t     st_mtime;
    __time32_t     st_ctime;
};

struct _stat64i32
{
    _dev_t         st_dev;
    _ino_t         st_ino;
    unsigned short st_mode;
    short          st_nlink;
    short          st_uid;
    short          st_gid;
    _dev_t         st_rdev;
    _off_t         st_size;
    __time64_t     st_atime;
    __time64_t     st_mtime;
    __time64_t     st_ctime;
};

struct _stat64
{
    _dev_t         st_dev;
    _ino_t         st_ino;
    unsigned short st_mode;
    short          st_nlink;
    short          st_uid;
    short          st_gid;
    _dev_t         st_rdev;
    __int64        st_size;
    __time64_t     st_atime;
    __time64_t     st_mtime;
    __time64_t     st_ctime;
};

#define __stat64 _stat64 // For legacy compatibility

#if defined(_CRT_INTERNAL_NONSTDC_NAMES) && _CRT_INTERNAL_NONSTDC_NAMES && !defined _CRT_NO_TIME_T
    struct stat
    {
        _dev_t         st_dev;
        _ino_t         st_ino;
        unsigned short st_mode;
        short          st_nlink;
        short          st_uid;
        short          st_gid;
        _dev_t         st_rdev;
        _off_t         st_size;
        time_t         st_atime;
        time_t         st_mtime;
        time_t         st_ctime;
    };
#endif



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Flags
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#define _S_IFMT   0xF000 // File type mask
#define _S_IFDIR  0x4000 // Directory
#define _S_IFCHR  0x2000 // Character special
#define _S_IFIFO  0x1000 // Pipe
#define _S_IFREG  0x8000 // Regular
#define _S_IREAD  0x0100 // Read permission, owner
#define _S_IWRITE 0x0080 // Write permission, owner
#define _S_IEXEC  0x0040 // Execute/search permission, owner

#if defined(_CRT_INTERNAL_NONSTDC_NAMES) && _CRT_INTERNAL_NONSTDC_NAMES
    #define S_IFMT   _S_IFMT
    #define S_IFDIR  _S_IFDIR
    #define S_IFCHR  _S_IFCHR
    #define S_IFREG  _S_IFREG
    #define S_IREAD  _S_IREAD
    #define S_IWRITE _S_IWRITE
    #define S_IEXEC  _S_IEXEC
#endif



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Functions
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#ifdef _USE_32BIT_TIME_T
    #define _fstat      _fstat32
    #define _fstati64   _fstat32i64
    #define _stat       _stat32
    #define _stati64    _stat32i64
    #define _wstat      _wstat32
    #define _wstati64   _wstat32i64
#else
    #define _fstat      _fstat64i32
    #define _fstati64   _fstat64
    #define _stat       _stat64i32
    #define _stati64    _stat64
    #define _wstat      _wstat64i32
    #define _wstati64   _wstat64
#endif



_ACRTIMP int __cdecl _fstat32(
    _In_  int             _FileHandle,
    _Out_ struct _stat32* _Stat
    );

_ACRTIMP int __cdecl _fstat32i64(
    _In_  int                _FileHandle,
    _Out_ struct _stat32i64* _Stat
    );

_ACRTIMP int __cdecl _fstat64i32(
    _In_  int                _FileHandle,
    _Out_ struct _stat64i32* _Stat
    );

_ACRTIMP int __cdecl _fstat64(
    _In_  int             _FileHandle,
    _Out_ struct _stat64* _Stat
    );

_ACRTIMP int __cdecl _stat32(
    _In_z_ char const*     _FileName,
    _Out_  struct _stat32* _Stat
    );

_ACRTIMP int __cdecl _stat32i64(
    _In_z_ char const*        _FileName,
    _Out_  struct _stat32i64* _Stat
    );

_ACRTIMP int __cdecl _stat64i32(
    _In_z_ char const*        _FileName,
    _Out_  struct _stat64i32* _Stat
    );

_ACRTIMP int __cdecl _stat64(
    _In_z_ char const*     _FileName,
    _Out_  struct _stat64* _Stat
    );

_ACRTIMP int __cdecl _wstat32(
    _In_z_ wchar_t const*  _FileName,
    _Out_  struct _stat32* _Stat
    );

_ACRTIMP int __cdecl _wstat32i64(
    _In_z_ wchar_t const*     _FileName,
    _Out_  struct _stat32i64* _Stat
    );

_ACRTIMP int __cdecl _wstat64i32(
    _In_z_ wchar_t const*     _FileName,
    _Out_  struct _stat64i32* _Stat
    );

_ACRTIMP int __cdecl _wstat64(
    _In_z_ wchar_t const*  _FileName,
    _Out_  struct _stat64* _Stat
    );



#if !defined RC_INVOKED && !defined __midl && defined(_CRT_INTERNAL_NONSTDC_NAMES) && _CRT_INTERNAL_NONSTDC_NAMES && !defined _CRT_NO_TIME_T
    #ifdef _USE_32BIT_TIME_T

        static __inline int __CRTDECL fstat(int const _FileHandle, struct stat* const _Stat)
        {
            _STATIC_ASSERT(sizeof(struct stat) == sizeof(struct _stat32));
            return _fstat32(_FileHandle, (struct _stat32*)_Stat);
        }

        static __inline int __CRTDECL stat(char const* const _FileName, struct stat* const _Stat)
        {
            _STATIC_ASSERT(sizeof(struct stat) == sizeof(struct _stat32));
            return _stat32(_FileName, (struct _stat32*)_Stat);
        }

    #else

        static __inline int __CRTDECL fstat(int const _FileHandle, struct stat* const _Stat)
        {
            _STATIC_ASSERT(sizeof(struct stat) == sizeof(struct _stat64i32));
            return _fstat64i32(_FileHandle, (struct _stat64i32*)_Stat);
        }
        static __inline int __CRTDECL stat(char const* const _FileName, struct stat* const _Stat)
        {
            _STATIC_ASSERT(sizeof(struct stat) == sizeof(struct _stat64i32));
            return _stat64i32(_FileName, (struct _stat64i32*)_Stat);
        }

    #endif
#endif

_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
