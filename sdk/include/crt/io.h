
/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _IO_H_
#define _IO_H_

#include <corecrt.h>
#include <string.h>

#pragma pack(push,_CRT_PACKING)

#ifndef _POSIX_

#ifdef __cplusplus
extern "C" {
#endif

_Check_return_
_Ret_opt_z_
_CRTIMP
char*
__cdecl
_getcwd(
  _Out_writes_opt_(_SizeInBytes) char *_DstBuf,
  _In_ int _SizeInBytes);

#ifndef _FSIZE_T_DEFINED
  typedef unsigned long _fsize_t;
#define _FSIZE_T_DEFINED
#endif

#ifndef _FINDDATA_T_DEFINED

  struct _finddata_t {
    unsigned attrib;
    time_t time_create;
    time_t time_access;
    time_t time_write;
    _fsize_t size;
    char name[260];
  };

  struct _finddata32_t {
    unsigned attrib;
    __time32_t time_create;
    __time32_t time_access;
    __time32_t time_write;
    _fsize_t size;
    char name[260];
  };

#if _INTEGRAL_MAX_BITS >= 64

  struct _finddatai64_t {
    unsigned attrib;
    time_t time_create;
    time_t time_access;
    time_t time_write;
    __MINGW_EXTENSION __int64 size;
    char name[260];
  };

  struct _finddata32i64_t {
    unsigned attrib;
    __time32_t time_create;
    __time32_t time_access;
    __time32_t time_write;
    __MINGW_EXTENSION __int64 size;
    char name[260];
  };

  struct _finddata64i32_t {
    unsigned attrib;
    __time64_t time_create;
    __time64_t time_access;
    __time64_t time_write;
    _fsize_t size;
    char name[260];
  };

  struct __finddata64_t {
    unsigned attrib;
    __time64_t time_create;
    __time64_t time_access;
    __time64_t time_write;
    __MINGW_EXTENSION __int64 size;
    char name[260];
  };
#endif /* _INTEGRAL_MAX_BITS >= 64 */

#define _FINDDATA_T_DEFINED
#endif

#ifndef _WFINDDATA_T_DEFINED

  struct _wfinddata_t {
    unsigned attrib;
    time_t time_create;
    time_t time_access;
    time_t time_write;
    _fsize_t size;
    wchar_t name[260];
  };

  struct _wfinddata32_t {
    unsigned attrib;
    __time32_t time_create;
    __time32_t time_access;
    __time32_t time_write;
    _fsize_t size;
    wchar_t name[260];
  };

#if _INTEGRAL_MAX_BITS >= 64

  struct _wfinddatai64_t {
    unsigned attrib;
    time_t time_create;
    time_t time_access;
    time_t time_write;
    __MINGW_EXTENSION __int64 size;
    wchar_t name[260];
  };

  struct _wfinddata32i64_t {
    unsigned attrib;
    __time32_t time_create;
    __time32_t time_access;
    __time32_t time_write;
    __MINGW_EXTENSION __int64 size;
    wchar_t name[260];
  };

  struct _wfinddata64i32_t {
    unsigned attrib;
    __time64_t time_create;
    __time64_t time_access;
    __time64_t time_write;
    _fsize_t size;
    wchar_t name[260];
  };

  struct _wfinddata64_t {
    unsigned attrib;
    __time64_t time_create;
    __time64_t time_access;
    __time64_t time_write;
    __MINGW_EXTENSION __int64 size;
    wchar_t name[260];
  };
#endif

#define _WFINDDATA_T_DEFINED
#endif

#define _A_NORMAL 0x00
#define _A_RDONLY 0x01
#define _A_HIDDEN 0x02
#define _A_SYSTEM 0x04
#define _A_SUBDIR 0x10
#define _A_ARCH 0x20

  /* Some defines for _access nAccessMode (MS doesn't define them, but
  * it doesn't seem to hurt to add them). */
#define	F_OK	0	/* Check for file existence */
#define	X_OK	1	/* Check for execute permission. */
#define	W_OK	2	/* Check for write permission */
#define	R_OK	4	/* Check for read permission */

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _access(
    _In_z_ const char *_Filename,
    _In_ int _AccessMode);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _chmod(
    _In_z_ const char *_Filename,
    _In_ int _Mode);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _chsize(
    _In_ int _FileHandle,
    _In_ long _Size);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _close(
    _In_ int _FileHandle);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _commit(
    _In_ int _FileHandle);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _creat(
    _In_z_ const char *_Filename,
    _In_ int _PermissionMode);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _dup(
    _In_ int _FileHandle);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _dup2(
    _In_ int _FileHandleSrc,
    _In_ int _FileHandleDst);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _eof(
    _In_ int _FileHandle);

  _Check_return_
  _CRTIMP
  long
  __cdecl
  _filelength(
    _In_ int _FileHandle);

  _CRTIMP
  intptr_t
  __cdecl
  _findfirst(
    const char *_Filename,
    struct _finddata_t *_FindData);

  _Check_return_
  _CRTIMP
  intptr_t
  __cdecl
  _findfirst32(
    _In_z_ const char *_Filename,
    _Out_ struct _finddata32_t *_FindData);

  _CRTIMP
  int
  __cdecl
  _findnext(
    intptr_t _FindHandle,
    struct _finddata_t *_FindData);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _findnext32(
    _In_ intptr_t _FindHandle,
    _Out_ struct _finddata32_t *_FindData);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _findclose(
    _In_ intptr_t _FindHandle);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _isatty(
    _In_ int _FileHandle);

  _CRTIMP
  int
  __cdecl
  _locking(
    _In_ int _FileHandle,
    _In_ int _LockMode,
    _In_ long _NumOfBytes);

  _Check_return_opt_
  _CRTIMP
  long
  __cdecl
  _lseek(
    _In_ int _FileHandle,
    _In_ long _Offset,
    _In_ int _Origin);

  _Check_return_
  _CRTIMP
  char*
  __cdecl
  _mktemp(
    _Inout_z_ char *_TemplateName);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _pipe(
    _Inout_updates_(2) int *_PtHandles,
    _In_ unsigned int _PipeSize,
    _In_ int _TextMode);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _read(
    _In_ int _FileHandle,
    _Out_writes_bytes_(_MaxCharCount) void *_DstBuf,
    _In_ unsigned int _MaxCharCount);

#ifndef _CRT_DIRECTORY_DEFINED
#define _CRT_DIRECTORY_DEFINED

  _Check_return_
  int
  __cdecl
  remove(
    _In_z_ const char *_Filename);

  _Check_return_
  int
  __cdecl
  rename(
    _In_z_ const char *_OldFilename,
    _In_z_ const char *_NewFilename);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _unlink(
    _In_z_ const char *_Filename);

#ifndef NO_OLDNAMES
  _Check_return_
  _CRTIMP
  int
  __cdecl
  unlink(
    _In_z_ const char *_Filename);
#endif

#endif /* _CRT_DIRECTORY_DEFINED */

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _setmode(
    _In_ int _FileHandle,
    _In_ int _Mode);

  _Check_return_
  _CRTIMP
  long
  __cdecl
  _tell(
    _In_ int _FileHandle);

  _CRTIMP
  int
  __cdecl
  _umask(
    _In_ int _Mode);

  _CRTIMP
  int
  __cdecl
  _write(
    _In_ int _FileHandle,
    _In_reads_bytes_(_MaxCharCount) const void *_Buf,
    _In_ unsigned int _MaxCharCount);

#if _INTEGRAL_MAX_BITS >= 64

  __MINGW_EXTENSION
  _Check_return_
  _CRTIMP
  __int64
  __cdecl
  _filelengthi64(
    _In_ int _FileHandle);

  _Check_return_
  _CRTIMP
  intptr_t
  __cdecl
  _findfirst32i64(
    _In_z_ const char *_Filename,
    _Out_ struct _finddata32i64_t *_FindData);

  _Check_return_
  _CRTIMP
  intptr_t
  __cdecl
  _findfirst64i32(
    _In_z_ const char *_Filename,
    _Out_ struct _finddata64i32_t *_FindData);

  _Check_return_
  _CRTIMP
  intptr_t
  __cdecl
  _findfirst64(
    _In_z_ const char *_Filename,
    _Out_ struct __finddata64_t *_FindData);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _findnext32i64(
    _In_ intptr_t _FindHandle,
    _Out_ struct _finddata32i64_t *_FindData);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _findnext64i32(
    _In_ intptr_t _FindHandle,
    _Out_ struct _finddata64i32_t *_FindData);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _findnext64(
    _In_ intptr_t _FindHandle,
    _Out_ struct __finddata64_t *_FindData);

  __MINGW_EXTENSION
  _Check_return_opt_
  _CRTIMP
  __int64
  __cdecl
  _lseeki64(
    _In_ int _FileHandle,
    _In_ __int64 _Offset,
    _In_ int _Origin);

  __MINGW_EXTENSION
  _Check_return_
  _CRTIMP
  __int64
  __cdecl
  _telli64(
    _In_ int _FileHandle);

#ifdef __cplusplus
#include <string.h>
#endif

  _Check_return_
  __CRT_INLINE
  intptr_t
  __cdecl
  _findfirst64i32(
    const char *_Filename,
    struct _finddata64i32_t *_FindData)
  {
    struct __finddata64_t fd;
    intptr_t ret = _findfirst64(_Filename,&fd);
    _FindData->attrib=fd.attrib;
    _FindData->time_create=fd.time_create;
    _FindData->time_access=fd.time_access;
    _FindData->time_write=fd.time_write;
    _FindData->size=(_fsize_t) fd.size;
    strncpy(_FindData->name,fd.name,260);
    return ret;
  }

  _Check_return_
  __CRT_INLINE
  int
  __cdecl
  _findnext64i32(
    intptr_t _FindHandle,
    struct _finddata64i32_t *_FindData)
  {
    struct __finddata64_t fd;
    int ret = _findnext64(_FindHandle,&fd);
    _FindData->attrib=fd.attrib;
    _FindData->time_create=fd.time_create;
    _FindData->time_access=fd.time_access;
    _FindData->time_write=fd.time_write;
    _FindData->size=(_fsize_t) fd.size;
    strncpy(_FindData->name,fd.name,260);
    return ret;
  }

#endif /* _INTEGRAL_MAX_BITS >= 64 */

#ifndef NO_OLDNAMES
#ifndef _UWIN

  _Check_return_
  _CRTIMP
  int
  __cdecl
  chdir(
    _In_z_ const char *_Path);

  _Check_return_
  _Ret_opt_z_
  _CRTIMP
  char*
  __cdecl
  getcwd(
    _Out_writes_opt_(_SizeInBytes) char *_DstBuf,
    _In_ int _SizeInBytes);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  mkdir(
    _In_z_ const char *_Path);

  _CRTIMP
  char*
  __cdecl
  mktemp(
    _Inout_z_ char *_TemplateName);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  rmdir(
    _In_z_ const char *_Path);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  chmod(
    _In_z_ const char *_Filename,
    _In_ int _AccessMode);

#endif /* _UWIN */
#endif /* Not NO_OLDNAMES */

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _sopen_s(
    _Out_ int *_FileHandle,
    _In_z_ const char *_Filename,
    _In_ int _OpenFlag,
    _In_ int _ShareFlag,
    _In_ int _PermissionMode);

#ifndef __cplusplus
  _CRTIMP int __cdecl _open(const char *_Filename,int _OpenFlag,...);
  _CRTIMP int __cdecl _sopen(const char *_Filename,int _OpenFlag,int _ShareFlag,...);
#else
  extern "C++" _CRTIMP int __cdecl _open(const char *_Filename,int _Openflag,int _PermissionMode = 0);
  extern "C++" _CRTIMP int __cdecl _sopen(const char *_Filename,int _Openflag,int _ShareFlag,int _PermissionMode = 0);
#endif

#ifndef _WIO_DEFINED
#define _WIO_DEFINED

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _waccess(
    _In_z_ const wchar_t *_Filename,
    _In_ int _AccessMode);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wchmod(
    _In_z_ const wchar_t *_Filename,
    _In_ int _Mode);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wcreat(
    _In_z_ const wchar_t *_Filename,
    _In_ int _PermissionMode);

  _Check_return_
  _CRTIMP
  intptr_t
  __cdecl
  _wfindfirst32(
    _In_z_ const wchar_t *_Filename,
    _Out_ struct _wfinddata32_t *_FindData);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wfindnext32(
    _In_ intptr_t _FindHandle,
    _Out_ struct _wfinddata32_t *_FindData);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wunlink(
    _In_z_ const wchar_t *_Filename);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wrename(
    _In_z_ const wchar_t *_NewFilename,
    _In_z_ const wchar_t *_OldFilename);

  _CRTIMP
  wchar_t*
  __cdecl
  _wmktemp(
    _Inout_z_ wchar_t *_TemplateName);

#if _INTEGRAL_MAX_BITS >= 64

  _Check_return_
  _CRTIMP
  intptr_t
  __cdecl
  _wfindfirst32i64(
    _In_z_ const wchar_t *_Filename,
    _Out_ struct _wfinddata32i64_t *_FindData);

  _Check_return_
  _CRTIMP
  intptr_t
  __cdecl
  _wfindfirst64i32(
    _In_z_ const wchar_t *_Filename,
    _Out_ struct _wfinddata64i32_t *_FindData);

  _Check_return_
  _CRTIMP
  intptr_t
  __cdecl
  _wfindfirst64(
    _In_z_ const wchar_t *_Filename,
    _Out_ struct _wfinddata64_t *_FindData);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wfindnext32i64(
    _In_ intptr_t _FindHandle,
    _Out_ struct _wfinddata32i64_t *_FindData);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wfindnext64i32(
    _In_ intptr_t _FindHandle,
    _Out_ struct _wfinddata64i32_t *_FindData);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wfindnext64(
    _In_ intptr_t _FindHandle,
    _Out_ struct _wfinddata64_t *_FindData);

#endif /* _INTEGRAL_MAX_BITS >= 64 */

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wsopen_s(
    _Out_ int *_FileHandle,
    _In_z_ const wchar_t *_Filename,
    _In_ int _OpenFlag,
    _In_ int _ShareFlag,
    _In_ int _PermissionFlag);

#if !defined(__cplusplus) || !(defined(_X86_) && !defined(__x86_64))
  _CRTIMP int __cdecl _wopen(const wchar_t *_Filename,int _OpenFlag,...);
  _CRTIMP int __cdecl _wsopen(const wchar_t *_Filename,int _OpenFlag,int _ShareFlag,...);
#else
  extern "C++" _CRTIMP int __cdecl _wopen(const wchar_t *_Filename,int _OpenFlag,int _PermissionMode = 0);
  extern "C++" _CRTIMP int __cdecl _wsopen(const wchar_t *_Filename,int _OpenFlag,int _ShareFlag,int _PermissionMode = 0);
#endif

#endif /* !_WIO_DEFINED */

  int
  __cdecl
  __lock_fhandle(
    _In_ int _Filehandle);

  void
  __cdecl
  _unlock_fhandle(
    _In_ int _Filehandle);

  _CRTIMP
  intptr_t
  __cdecl
  _get_osfhandle(
    _In_ int _FileHandle);

  _CRTIMP
  int
  __cdecl
  _open_osfhandle(
    _In_ intptr_t _OSFileHandle,
    _In_ int _Flags);

#ifndef NO_OLDNAMES

  _Check_return_
  _CRTIMP
  int
  __cdecl
  access(
    _In_z_ const char *_Filename,
    _In_ int _AccessMode);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  chmod(
    _In_z_ const char *_Filename,
    _In_ int _AccessMode);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  chsize(
    _In_ int _FileHandle,
    _In_ long _Size);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  close(
    _In_ int _FileHandle);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  creat(
    _In_z_ const char *_Filename,
    _In_ int _PermissionMode);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  dup(
    _In_ int _FileHandle);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  dup2(
    _In_ int _FileHandleSrc,
    _In_ int _FileHandleDst);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  __cdecl eof(
    _In_ int _FileHandle);

  _Check_return_
  _CRTIMP
  long
  __cdecl
  filelength(
    _In_ int _FileHandle);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  isatty(
    _In_ int _FileHandle);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  locking(
    _In_ int _FileHandle,
    _In_ int _LockMode,
    _In_ long _NumOfBytes);

  _Check_return_opt_
  _CRTIMP
  long
  __cdecl
  lseek(
    _In_ int _FileHandle,
    _In_ long _Offset,
    _In_ int _Origin);

  _CRTIMP
  char*
  __cdecl
  mktemp(
    _Inout_z_ char *_TemplateName);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  open(
    _In_z_ const char *_Filename,
    _In_ int _OpenFlag,
    ...);

  _CRTIMP
  int
  __cdecl
  read(
    _In_ int _FileHandle,
    _Out_writes_bytes_(_MaxCharCount) void *_DstBuf,
    _In_ unsigned int _MaxCharCount);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  setmode(
    _In_ int _FileHandle,
    _In_ int _Mode);

  _CRTIMP
  int
  __cdecl
  sopen(
    const char *_Filename,
    int _OpenFlag,
    int _ShareFlag,
    ...);

  _Check_return_
  _CRTIMP
  long
  __cdecl
  tell(
    _In_ int _FileHandle);

  _CRTIMP
  int
  __cdecl
  umask(
    _In_ int _Mode);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  write(
    _In_ int _Filehandle,
    _In_reads_bytes_(_MaxCharCount) const void *_Buf,
    _In_ unsigned int _MaxCharCount);

#endif /* NO_OLDNAMES */

#ifdef __cplusplus
}
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Misc stuff */
char *getlogin(void);
#ifdef __USE_MINGW_ALARM
unsigned int alarm(unsigned int seconds);
#endif

#ifdef __USE_MINGW_ACCESS
/*  Old versions of MSVCRT access() just ignored X_OK, while the version
    shipped with Vista, returns an error code.  This will restore the
    old behaviour  */
static inline int __mingw_access (const char *__fname, int __mode) {
  return  _access (__fname, __mode & ~X_OK);
}

#define access(__f,__m)  __mingw_access (__f, __m)
#endif

#ifdef __cplusplus
}
#endif

#pragma pack(pop)

#include <sec_api/io_s.h>

#endif /* End _IO_H_ */

