/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

#ifndef _INC_IO_S
#define _INC_IO_S

#include <io.h>

#if defined(MINGW_HAS_SECURE_API)

#ifdef __cplusplus
extern "C" {
#endif

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _access_s(
    _In_z_ const char *_Filename,
    _In_ int _AccessMode);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _chsize_s(
    _In_ int _FileHandle,
    _In_ __int64 _Size);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _mktemp_s(
    _Inout_updates_z_(_Size) char *_TemplateName,
    _In_ size_t _Size);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _umask_s(
    _In_ int _NewMode,
    _Out_ int *_OldMode);

#ifndef _WIO_S_DEFINED
#define _WIO_S_DEFINED

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _waccess_s(
    _In_z_ const wchar_t *_Filename,
    _In_ int _AccessMode);

  _CRTIMP
  errno_t
  __cdecl
  _wmktemp_s(
    _Inout_updates_z_(_SizeInWords) wchar_t *_TemplateName,
    _In_ size_t _SizeInWords);

#endif /* _WIO_S_DEFINED */

#ifdef __cplusplus
}
#endif

#endif /* MINGW_HAS_SECURE_API */

#endif /* _INC_IO_S */
