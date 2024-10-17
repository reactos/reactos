/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

#ifndef _INC_MBSTRING
#define _INC_MBSTRING

#include <corecrt.h>

#pragma pack(push,_CRT_PACKING)

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _FILE_DEFINED
  struct _iobuf {
    char *_ptr;
    int _cnt;
    char *_base;
    int _flag;
    int _file;
    int _charbuf;
    int _bufsiz;
    char *_tmpfname;
  };
  typedef struct _iobuf FILE;
#define _FILE_DEFINED
#endif

#ifndef _MBSTRING_DEFINED
#define _MBSTRING_DEFINED

  _Check_return_
  _CRTIMP
  unsigned char*
  __cdecl
  _mbsdup(
    _In_z_ const unsigned char *_Str);

  _Check_return_
  _CRTIMP
  unsigned int
  __cdecl
  _mbbtombc(
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  unsigned int
  __cdecl
  _mbbtombc_l(
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _mbbtype(
    _In_ unsigned char _Ch,
    _In_ int _CType);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _mbbtype_l(
    _In_ unsigned char _Ch,
    _In_ int _CType,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  unsigned int
  __cdecl
  _mbctombb(
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  unsigned int
  __cdecl
  _mbctombb_l(
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  int
  __cdecl
  _mbsbtype(
    _In_reads_bytes_(_Pos) _Pre_z_ const unsigned char *_Str,
    _In_ size_t _Pos);

  _CRTIMP
  int
  __cdecl
  _mbsbtype_l(
    _In_reads_bytes_(_Pos) _Pre_z_ const unsigned char *_Str,
    _In_ size_t _Pos,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  unsigned char*
  __cdecl
  _mbscat(
    _Inout_updates_z_(_String_length_(_Curr_) + _String_length_(_Source) + 1) unsigned char *_Dest,
    _In_z_ const unsigned char *_Source);

  _CRTIMP
  unsigned char*
  _mbscat_l(
    _Inout_z_ unsigned char *_Dest,
    _In_z_ const unsigned char *_Source,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  _CONST_RETURN
  unsigned char*
  __cdecl
  _mbschr(
    _In_z_ const unsigned char *_Str,
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  _CONST_RETURN
  unsigned char*
  __cdecl
  _mbschr_l(
    _In_z_ const unsigned char *_Str,
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _mbscmp(
    _In_z_ const unsigned char *_Str1,
    _In_z_ const unsigned char *_Str2);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _mbscmp_l(
    _In_z_ const unsigned char *_Str1,
    _In_z_ const unsigned char *_Str2,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _mbscoll(
    _In_z_ const unsigned char *_Str1,
    _In_z_ const unsigned char *_Str2);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _mbscoll_l(
    _In_z_ const unsigned char *_Str1,
    _In_z_ const unsigned char *_Str2,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  unsigned char*
  __cdecl
  _mbscpy(
    _Out_writes_z_(_String_length_(_Source) + 1) unsigned char *_Dest,
    _In_z_ const unsigned char *_Source);

  _CRTIMP
  unsigned char*
  _mbscpy_l(
    _Pre_notnull_ _Post_z_ unsigned char *_Dest,
    _In_z_ const unsigned char *_Source,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  size_t
  __cdecl
  _mbscspn(
    _In_z_ const unsigned char *_Str,
    _In_z_ const unsigned char *_Control);

  _Check_return_
  _CRTIMP
  size_t
  __cdecl
  _mbscspn_l(
    _In_z_ const unsigned char *_Str,
    _In_z_ const unsigned char *_Control,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  unsigned char*
  __cdecl
  _mbsdec(
    _In_reads_z_(_Pos-_Start + 1) const unsigned char *_Start,
    _In_z_ const unsigned char *_Pos);

  _Check_return_
  _CRTIMP
  unsigned char*
  __cdecl
  _mbsdec_l(
    _In_reads_z_(_Pos-_Start + 1) const unsigned char *_Start,
    _In_z_ const unsigned char *_Pos,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _mbsicmp(
    _In_z_ const unsigned char *_Str1,
    _In_z_ const unsigned char *_Str2);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _mbsicmp_l(
    _In_z_ const unsigned char *_Str1,
    _In_z_ const unsigned char *_Str2,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _mbsicoll(
    _In_z_ const unsigned char *_Str1,
    _In_z_ const unsigned char *_Str2);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _mbsicoll_l(
    _In_z_ const unsigned char *_Str1,
    _In_z_ const unsigned char *_Str2,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  unsigned char*
  __cdecl
  _mbsinc(
    _In_z_ const unsigned char *_Ptr);

  _Check_return_
  _CRTIMP
  unsigned char*
  __cdecl
  _mbsinc_l(
    _In_z_ const unsigned char *_Ptr,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  size_t
  __cdecl
  _mbslen(
    _In_z_ const unsigned char *_Str);

  _Check_return_
  _CRTIMP
  size_t
  __cdecl
  _mbslen_l(
    _In_z_ const unsigned char *_Str,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  size_t
  __cdecl
  _mbsnlen(
    _In_z_ const unsigned char *_Str,
    _In_ size_t _MaxCount);

  _Check_return_
  _CRTIMP
  size_t
  __cdecl
  _mbsnlen_l(
    _In_z_ const unsigned char *_Str,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  unsigned char*
  __cdecl
  _mbslwr(
    _Inout_z_ unsigned char *_String);

  _CRTIMP
  unsigned char*
  _mbslwr_l(
    _Inout_z_ unsigned char *_String,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  unsigned char*
  __cdecl
  _mbsnbcat(
    _Inout_z_ unsigned char *_Dest,
    _In_z_ const unsigned char *_Source,
    _In_ size_t _Count);

  _CRTIMP
  unsigned char*
  __cdecl
  _mbsnbcat_l(
    _Inout_z_ unsigned char *_Dest,
    _In_z_ const unsigned char *_Source,
    _In_ size_t _Count,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _mbsnbcmp(
    _In_z_ const unsigned char *_Str1,
    _In_z_ const unsigned char *_Str2,
    _In_ size_t _MaxCount);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _mbsnbcmp_l(
    _In_z_ const unsigned char *_Str1,
    _In_z_ const unsigned char *_Str2,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _mbsnbcoll(
    _In_z_ const unsigned char *_Str1,
    _In_z_ const unsigned char *_Str2,
    _In_ size_t _MaxCount);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _mbsnbcoll_l(
    _In_z_ const unsigned char *_Str1,
    _In_z_ const unsigned char *_Str2,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  size_t
  __cdecl
  _mbsnbcnt(
    _In_reads_bytes_(_MaxCount) _Pre_z_ const unsigned char *_Str,
    _In_ size_t _MaxCount);

  _Check_return_
  _CRTIMP
  size_t
  __cdecl
  _mbsnbcnt_l(
    _In_reads_bytes_(_MaxCount) _Pre_z_ const unsigned char *_Str,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  unsigned char*
  __cdecl
  _mbsnbcpy(
    _Out_writes_(_Count) _Post_maybez_ unsigned char *_Dest,
    _In_z_ const unsigned char *_Source,
    _In_ size_t _Count);

  _CRTIMP
  errno_t
  __cdecl
  _mbsnbcpy_s(
    _Out_writes_z_(_DstSizeInBytes) unsigned char *_Dst,
    _In_ size_t _DstSizeInBytes,
    _In_z_ const unsigned char *_Src,
    _In_ size_t _MaxCount);

  _CRTIMP
  unsigned char*
  __cdecl
  _mbsnbcpy_l(
    _Out_writes_(_Count) _Post_maybez_ unsigned char *_Dest,
    _In_z_ const unsigned char *_Source,
    _In_ size_t _Count,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _mbsnbicmp(
    _In_z_ const unsigned char *_Str1,
    _In_z_ const unsigned char *_Str2,
    _In_ size_t _MaxCount);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _mbsnbicmp_l(
    _In_z_ const unsigned char *_Str1,
    _In_z_ const unsigned char *_Str2,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _mbsnbicoll(
    _In_z_ const unsigned char *_Str1,
    _In_z_ const unsigned char *_Str2,
    _In_ size_t _MaxCount);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _mbsnbicoll_l(
    _In_z_ const unsigned char *_Str1,
    _In_z_ const unsigned char *_Str2,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  unsigned char*
  __cdecl
  _mbsnbset(
    _Inout_updates_z_(_MaxCount) unsigned char *_Str,
    _In_ unsigned int _Ch,
    _In_ size_t _MaxCount);

  _CRTIMP
  unsigned char*
  __cdecl
  _mbsnbset_l(
    _Inout_updates_z_(_MaxCount) unsigned char *_Str,
    _In_ unsigned int _Ch,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  unsigned char*
  __cdecl
  _mbsncat(
    _Inout_z_ unsigned char *_Dest,
    _In_z_ const unsigned char *_Source,
    _In_ size_t _Count);

  _CRTIMP
  unsigned char*
  __cdecl
  _mbsncat_l(
    _Inout_z_ unsigned char *_Dest,
    _In_z_ const unsigned char *_Source,
    _In_ size_t _Count,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  size_t
  __cdecl
  _mbsnccnt(
    _In_reads_bytes_(_MaxCount) _Pre_z_ const unsigned char *_Str,
    _In_ size_t _MaxCount);

  _Check_return_
  _CRTIMP
  size_t
  __cdecl
  _mbsnccnt_l(
    _In_reads_bytes_(_MaxCount) _Pre_z_ const unsigned char *_Str,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _mbsncmp(
    _In_z_ const unsigned char *_Str1,
    _In_z_ const unsigned char *_Str2,
    _In_ size_t _MaxCount);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _mbsncmp_l(
    _In_z_ const unsigned char *_Str1,
    _In_z_ const unsigned char *_Str2,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _mbsncoll(
    _In_z_ const unsigned char *_Str1,
    _In_z_ const unsigned char *_Str2,
    _In_ size_t _MaxCount);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _mbsncoll_l(
    _In_z_ const unsigned char *_Str1,
    _In_z_ const unsigned char *_Str2,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  unsigned char*
  __cdecl
  _mbsncpy(
    _Pre_notnull_ _Out_writes_(2 * _Count) _Post_maybez_ unsigned char *_Dest,
    _In_z_ const unsigned char *_Source,
    _In_ size_t _Count);

  _CRTIMP
  unsigned char*
  __cdecl
  _mbsncpy_l(
    _Out_writes_(_Count) _Post_maybez_ unsigned char *_Dest,
    _In_z_ const unsigned char *_Source,
    _In_ size_t _Count,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  unsigned int
  __cdecl
  _mbsnextc(
    _In_z_ const unsigned char *_Str);

  _Check_return_
  _CRTIMP
  unsigned int
  __cdecl
  _mbsnextc_l(
    _In_z_ const unsigned char *_Str,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _mbsnicmp(
    _In_z_ const unsigned char *_Str1,
    _In_z_ const unsigned char *_Str2,
    _In_ size_t _MaxCount);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _mbsnicmp_l(
    _In_z_ const unsigned char *_Str1,
    _In_z_ const unsigned char *_Str2,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _mbsnicoll(
    _In_z_ const unsigned char *_Str1,
    _In_z_ const unsigned char *_Str2,
    _In_ size_t _MaxCount);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _mbsnicoll_l(
    _In_z_ const unsigned char *_Str1,
    _In_z_ const unsigned char *_Str2,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  unsigned char*
  __cdecl
  _mbsninc(
    _In_reads_bytes_(_Count) _Pre_z_ const unsigned char *_Str,
    _In_ size_t _Count);

  _Check_return_
  _CRTIMP
  unsigned char*
  __cdecl
  _mbsninc_l(
    _In_reads_bytes_(_Count) _Pre_z_ const unsigned char *_Str,
    _In_ size_t _Count,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  unsigned char*
  __cdecl
  _mbsnset(
    _Inout_updates_z_(_MaxCount) unsigned char *_Dst,
    _In_ unsigned int _Val,
    _In_ size_t _MaxCount);

  _CRTIMP
  unsigned char*
  __cdecl
  _mbsnset_l(
    _Inout_updates_z_(_MaxCount) unsigned char *_Dst,
    _In_ unsigned int _Val,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  _CONST_RETURN
  unsigned char*
  __cdecl
  _mbspbrk(
    _In_z_ const unsigned char *_Str,
    _In_z_ const unsigned char *_Control);

  _Check_return_
  _CRTIMP
  _CONST_RETURN
  unsigned char*
  __cdecl
  _mbspbrk_l(
    _In_z_ const unsigned char *_Str,
    _In_z_ const unsigned char *_Control,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  _CONST_RETURN
  unsigned char*
  __cdecl
  _mbsrchr(
    _In_z_ const unsigned char *_Str,
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  _CONST_RETURN
  unsigned char*
  __cdecl
  _mbsrchr_l(
    _In_z_ const unsigned char *_Str,
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  unsigned char*
  __cdecl
  _mbsrev(
    _Inout_z_ unsigned char *_Str);

  _CRTIMP
  unsigned char*
  __cdecl
  _mbsrev_l(
    _Inout_z_ unsigned char *_Str,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  unsigned char*
  __cdecl
  _mbsset(
    _Inout_z_ unsigned char *_Str,
    _In_ unsigned int _Val);

  _CRTIMP
  unsigned char*
  __cdecl
  _mbsset_l(
    _Inout_z_ unsigned char *_Str,
    _In_ unsigned int _Val,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  size_t
  __cdecl
  _mbsspn(
    _In_z_ const unsigned char *_Str,
    _In_z_ const unsigned char *_Control);

  _Check_return_
  _CRTIMP
  size_t
  __cdecl
  _mbsspn_l(
    _In_z_ const unsigned char *_Str,
    _In_z_ const unsigned char *_Control,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  unsigned char*
  __cdecl
  _mbsspnp(
    _In_z_ const unsigned char *_Str1,
    _In_z_ const unsigned char *_Str2);

  _Check_return_
  _CRTIMP
  unsigned char*
  __cdecl
  _mbsspnp_l(
    _In_z_ const unsigned char *_Str1,
    _In_z_ const unsigned char *_Str2,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  _CONST_RETURN
  unsigned char*
  __cdecl
  _mbsstr(
    _In_z_ const unsigned char *_Str,
    _In_z_ const unsigned char *_Substr);

  _Check_return_
  _CRTIMP
  _CONST_RETURN
  unsigned char*
  __cdecl
  _mbsstr_l(
    _In_z_ const unsigned char *_Str,
    _In_z_ const unsigned char *_Substr,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  unsigned char*
  __cdecl
  _mbstok(
    _Inout_opt_z_ unsigned char *_Str,
    _In_z_ const unsigned char *_Delim);

  _Check_return_
  _CRTIMP
  unsigned char*
  __cdecl
  _mbstok_l(
    _Inout_opt_z_ unsigned char *_Str,
    _In_z_ const unsigned char *_Delim,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  unsigned char*
  __cdecl
  _mbsupr(
    _Inout_z_ unsigned char *_String);

  _CRTIMP
  unsigned char*
  _mbsupr_l(
    _Inout_z_ unsigned char *_String,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  size_t
  __cdecl
  _mbclen(
    _In_z_ const unsigned char *_Str);

  _Check_return_
  _CRTIMP
  size_t
  __cdecl
  _mbclen_l(
    _In_z_ const unsigned char *_Str,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  void
  __cdecl
  _mbccpy(
    _Out_writes_bytes_(2) unsigned char *_Dst,
    _In_z_ const unsigned char *_Src);

  _CRTIMP
  void
  __cdecl
  _mbccpy_l(
    _Out_writes_bytes_(2) unsigned char *_Dst,
    _In_z_ const unsigned char *_Src,
    _In_opt_ _locale_t _Locale);

#define _mbccmp(_cpc1,_cpc2) _mbsncmp((_cpc1),(_cpc2),1)

#ifdef __cplusplus
#ifndef _CPP_MBCS_INLINES_DEFINED
#define _CPP_MBCS_INLINES_DEFINED
  extern "C++" {
    static inline unsigned char *__cdecl _mbschr(unsigned char *_String,unsigned int _Char) { return ((unsigned char *)_mbschr((const unsigned char *)_String,_Char)); }
    static inline unsigned char *__cdecl _mbschr_l(unsigned char *_String,unsigned int _Char,_locale_t _Locale) { return ((unsigned char *)_mbschr_l((const unsigned char *)_String,_Char,_Locale)); }
    static inline unsigned char *__cdecl _mbspbrk(unsigned char *_String,const unsigned char *_CharSet) { return ((unsigned char *)_mbspbrk((const unsigned char *)_String,_CharSet)); }
    static inline unsigned char *__cdecl _mbspbrk_l(unsigned char *_String,const unsigned char *_CharSet,_locale_t _Locale) { return ((unsigned char *)_mbspbrk_l((const unsigned char *)_String,_CharSet,_Locale)); }
    static inline unsigned char *__cdecl _mbsrchr(unsigned char *_String,unsigned int _Char) { return ((unsigned char *)_mbsrchr((const unsigned char *)_String,_Char)); }
    static inline unsigned char *__cdecl _mbsrchr_l(unsigned char *_String,unsigned int _Char,_locale_t _Locale) { return ((unsigned char *)_mbsrchr_l((const unsigned char *)_String,_Char,_Locale)); }
    static inline unsigned char *__cdecl _mbsstr(unsigned char *_String,const unsigned char *_Match) { return ((unsigned char *)_mbsstr((const unsigned char *)_String,_Match)); }
    static inline unsigned char *__cdecl _mbsstr_l(unsigned char *_String,const unsigned char *_Match,_locale_t _Locale) { return ((unsigned char *)_mbsstr_l((const unsigned char *)_String,_Match,_Locale)); }
  }
#endif
#endif

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbcalnum(
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbcalnum_l(
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbcalpha(
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbcalpha_l(
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbcdigit(
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbcdigit_l(
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbcgraph(
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbcgraph_l(
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbclegal(
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbclegal_l(
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbclower(
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbclower_l(
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbcprint(
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbcprint_l(
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbcpunct(
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbcpunct_l(
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbcspace(
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbcspace_l(
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbcupper(
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  __cdecl _ismbcupper_l(
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  unsigned int
  __cdecl
  _mbctolower(
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  unsigned int
  __cdecl
  _mbctolower_l(
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  unsigned int
  __cdecl
  _mbctoupper(
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  unsigned int
  __cdecl
  _mbctoupper_l(
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

#endif /* _MBSTRING_DEFINED */

#ifndef _MBLEADTRAIL_DEFINED
#define _MBLEADTRAIL_DEFINED

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbblead(
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbblead_l(
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbbtrail(
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbbtrail_l(
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbslead(
    _In_reads_z_(_Pos - _Str + 1) const unsigned char *_Str,
    _In_z_ const unsigned char *_Pos);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbslead_l(
    _In_reads_z_(_Pos - _Str + 1) const unsigned char *_Str,
    _In_z_ const unsigned char *_Pos,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbstrail(
    _In_reads_z_(_Pos - _Str + 1) const unsigned char *_Str,
    _In_z_ const unsigned char *_Pos);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbstrail_l(
    _In_reads_z_(_Pos - _Str + 1) const unsigned char *_Str,
    _In_z_ const unsigned char *_Pos,
    _In_opt_ _locale_t _Locale);

#endif /* _MBLEADTRAIL_DEFINED */

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbchira(
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbchira_l(
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbckata(
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbckata_l(
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbcsymbol(
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbcsymbol_l(
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbcl0(
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbcl0_l(
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbcl1(
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbcl1_l(
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbcl2(
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbcl2_l(
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  unsigned int
  __cdecl
  _mbcjistojms(
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  unsigned int
  __cdecl
  _mbcjistojms_l(
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  unsigned int
  __cdecl
  _mbcjmstojis(
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  unsigned int
  __cdecl
  _mbcjmstojis_l(
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  unsigned int
  __cdecl
  _mbctohira(
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  unsigned int
  __cdecl
  _mbctohira_l(
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  unsigned int
  __cdecl
  _mbctokata(
    _In_ unsigned int _Ch);

  _Check_return_
  _CRTIMP
  unsigned int
  __cdecl
  _mbctokata_l(
    _In_ unsigned int _Ch,
    _In_opt_ _locale_t _Locale);

#ifdef __cplusplus
}
#endif

#pragma pack(pop)

#include <sec_api/mbstring_s.h>

#endif /* _INC_MBSTRING */
