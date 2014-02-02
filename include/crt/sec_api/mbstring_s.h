/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

#ifndef _INC_MBSTRING_S
#define _INC_MBSTRING_S

#include <mbstring.h>

#if defined(MINGW_HAS_SECURE_API)

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _MBSTRING_S_DEFINED
#define _MBSTRING_S_DEFINED

  _CRTIMP
  errno_t
  __cdecl
  _mbscat_s(
    _Inout_updates_z_(_DstSizeInBytes) unsigned char *_Dst,
    _In_ size_t _DstSizeInBytes,
    _In_z_ const unsigned char *_Src);

  _CRTIMP
  errno_t
  __cdecl
  _mbscat_s_l(
    _Inout_updates_z_(_DstSizeInBytes) unsigned char *_Dst,
    _In_ size_t _DstSizeInBytes,
    _In_z_ const unsigned char *_Src,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  errno_t
  __cdecl
  _mbscpy_s(
    _Out_writes_z_(_DstSizeInBytes) unsigned char *_Dst,
    _In_ size_t _DstSizeInBytes,
    _In_z_ const unsigned char *_Src);

  _CRTIMP
  errno_t
  __cdecl
  _mbscpy_s_l(
    _Out_writes_z_(_DstSizeInBytes) unsigned char *_Dst,
    _In_ size_t _DstSizeInBytes,
    _In_z_ const unsigned char *_Src,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  errno_t
  __cdecl
  _mbslwr_s(
    _Inout_updates_opt_z_(_SizeInBytes) unsigned char *_Str,
    _In_ size_t _SizeInBytes);

  _CRTIMP
  errno_t
  __cdecl
  _mbslwr_s_l(
    _Inout_updates_opt_z_(_SizeInBytes) unsigned char *_Str,
    _In_ size_t _SizeInBytes,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  errno_t
  __cdecl
  _mbsnbcat_s(
    _Inout_updates_z_(_DstSizeInBytes) unsigned char *_Dst,
    _In_ size_t _DstSizeInBytes,
    _In_z_ const unsigned char *_Src,
    _In_ size_t _MaxCount);

  _CRTIMP
  errno_t
  __cdecl
  _mbsnbcat_s_l(
    _Inout_updates_z_(_DstSizeInBytes) unsigned char *_Dst,
    _In_ size_t _DstSizeInBytes,
    _In_z_ const unsigned char *_Src,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  errno_t
  __cdecl
  _mbsnbcpy_s(
    _Out_writes_z_(_DstSizeInBytes) unsigned char *_Dst,
    _In_ size_t _DstSizeInBytes,
    _In_z_ const unsigned char *_Src,
    _In_ size_t _MaxCount);

  _CRTIMP
  errno_t
  __cdecl
  _mbsnbcpy_s_l(
    _Out_writes_z_(_DstSizeInBytes) unsigned char *_Dst,
    _In_ size_t _DstSizeInBytes,
    _In_z_ const unsigned char *_Src,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  errno_t
  __cdecl
  _mbsnbset_s(
    _Inout_updates_z_(_DstSizeInBytes) unsigned char *_Dst,
    _In_ size_t _DstSizeInBytes,
    _In_ unsigned int _Ch,
    _In_ size_t _MaxCount);

  _CRTIMP
  errno_t
  __cdecl
  _mbsnbset_s_l(
    _Inout_updates_z_(_DstSizeInBytes) unsigned char *_Dst,
    _In_ size_t _DstSizeInBytes,
    _In_ unsigned int _Ch,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  errno_t
  __cdecl
  _mbsncat_s(
    _Inout_updates_z_(_DstSizeInBytes) unsigned char *_Dst,
    _In_ size_t _DstSizeInBytes,
    _In_z_ const unsigned char *_Src,
    _In_ size_t _MaxCount);

  _CRTIMP
  errno_t
  __cdecl
  _mbsncat_s_l(
    _Inout_updates_z_(_DstSizeInBytes) unsigned char *_Dst,
    _In_ size_t _DstSizeInBytes,
    _In_z_ const unsigned char *_Src,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  errno_t
  __cdecl
  _mbsncpy_s(
    _Out_writes_z_(_DstSizeInBytes) unsigned char *_Dst,
    _In_ size_t _DstSizeInBytes,
    _In_z_ const unsigned char *_Src,
    _In_ size_t _MaxCount);

  _CRTIMP
  errno_t
  __cdecl
  _mbsncpy_s_l(
    _Out_writes_z_(_DstSizeInBytes) unsigned char *_Dst,
    _In_ size_t _DstSizeInBytes,
    _In_z_ const unsigned char *_Src,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  errno_t
  __cdecl
  _mbsnset_s(
    _Inout_updates_z_(_DstSizeInBytes) unsigned char *_Dst,
    _In_ size_t _DstSizeInBytes,
    _In_ unsigned int _Val,
    _In_ size_t _MaxCount);

  _CRTIMP
  errno_t
  __cdecl
  _mbsnset_s_l(
    _Inout_updates_z_(_DstSizeInBytes) unsigned char *_Dst,
    _In_ size_t _DstSizeInBytes,
    _In_ unsigned int _Val,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  errno_t
  __cdecl
  _mbsset_s(
    _Inout_updates_z_(_DstSizeInBytes) unsigned char *_Dst,
    _In_ size_t _DstSizeInBytes,
    _In_ unsigned int _Val);

  _CRTIMP
  errno_t
  __cdecl
  _mbsset_s_l(
    _Inout_updates_z_(_DstSizeInBytes) unsigned char *_Dst,
    _In_ size_t _DstSizeInBytes,
    _In_ unsigned int _Val,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  unsigned char *
  __cdecl
  _mbstok_s(
    _Inout_opt_z_ unsigned char *_Str,
    _In_z_ const unsigned char *_Delim,
    _Inout_ _Deref_prepost_opt_z_ unsigned char **_Context);

  _Check_return_
  _CRTIMP
  unsigned char *
  __cdecl
  _mbstok_s_l(
    _Inout_opt_z_ unsigned char *_Str,
    _In_z_ const unsigned char *_Delim,
    _Inout_ _Deref_prepost_opt_z_ unsigned char **_Context,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  errno_t
  __cdecl
  _mbsupr_s(
    _Inout_updates_z_(_SizeInBytes) unsigned char *_Str,
    _In_ size_t _SizeInBytes);

  _CRTIMP
  errno_t
  __cdecl
  _mbsupr_s_l(
    _Inout_updates_z_(_SizeInBytes) unsigned char *_Str,
    _In_ size_t _SizeInBytes,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  errno_t
  __cdecl
  _mbccpy_s(
    _Out_writes_z_(_DstSizeInBytes) unsigned char *_Dst,
    _In_ size_t _DstSizeInBytes,
    _Out_opt_ int *_PCopied,
    _In_z_ const unsigned char *_Src);

  _CRTIMP
  errno_t
  __cdecl
  _mbccpy_s_l(
    _Out_writes_z_(_DstSizeInBytes) unsigned char *_Dst,
    _In_ size_t _DstSizeInBytes,
    _Out_opt_ int *_PCopied,
    _In_z_ const unsigned char *_Src,
    _In_opt_ _locale_t _Locale);

#endif /* _MBSTRING_S_DEFINED */

#ifdef __cplusplus
}
#endif

#endif /* MINGW_HAS_SECURE_API */

#endif /* _INC_MBSTRING_S */
