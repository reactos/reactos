
#pragma once

#include <vcruntime.h>

_CRT_BEGIN_C_HEADER

void*
__cdecl
memset(
    _Out_writes_bytes_all_(_Size) void *_Dst,
    _In_ int _Val,
    _In_ size_t _Size);

_CRT_INSECURE_DEPRECATE_MEMORY(memcpy_s)
_Post_equal_to_(_Dst)
_At_buffer_((unsigned char*)_Dst, _Iter_, _Size,
  _Post_satisfies_(((unsigned char*)_Dst)[_Iter_] == ((unsigned char*)_Src)[_Iter_]))
void*
__cdecl
memcpy(
    _Out_writes_bytes_all_(_Size) void *_Dst,
    _In_reads_bytes_(_Size) void const *_Src,
    _In_ size_t _Size);

_CRT_INSECURE_DEPRECATE_MEMORY(memmove_s)
_VCRTIMP
void*
__cdecl
memmove(
    _Out_writes_bytes_all_opt_(_Size) void *_Dst,
    _In_reads_bytes_opt_(_Size) void const *_Src,
    _In_ size_t _Size);

_NODISCARD
_Check_return_
_VCRTIMP
char _CONST_RETURN*
__cdecl
strchr(
    _In_z_ const char *_Str,
    _In_ int _Val);

_NODISCARD
_Check_return_
_When_(return != NULL, _Ret_range_(_Str, _Str + _String_length_(_Str) - 1))
_VCRTIMP
wchar_t _CONST_RETURN*
__cdecl
wcschr(
    _In_z_ const wchar_t *_Str,
    _In_ wchar_t _Ch);

_NODISCARD
_Check_return_
_VCRTIMP
char _CONST_RETURN*
__cdecl strrchr(
    _In_z_ const char *_Str,
    _In_ int _Ch);

_NODISCARD
_Check_return_
_Ret_maybenull_
_VCRTIMP
char _CONST_RETURN*
__cdecl
strstr(
    _In_z_ const char *_Str,
    _In_z_ const char *_SubStr);

_NODISCARD
_Check_return_
int
__cdecl
memcmp(
    _In_reads_bytes_(_Size) const void *_Buf1,
    _In_reads_bytes_(_Size) const void *_Buf2,
    _In_ size_t _Size);

_NODISCARD
_Check_return_
_Ret_maybenull_
_When_(return != NULL, _Ret_range_(_Str, _Str + _String_length_(_Str) - 1))
_VCRTIMP
wchar_t _CONST_RETURN*
__cdecl
wcsstr(
    _In_z_ const wchar_t *_Str,
    _In_z_ const wchar_t *_SubStr);

_NODISCARD
_Check_return_
_VCRTIMP
void _CONST_RETURN*
__cdecl
memchr(
    _In_reads_bytes_opt_(_MaxCount) const void *_Buf,
    _In_ int _Val,
    _In_ size_t _MaxCount);

_NODISCARD
_Check_return_
_VCRTIMP
wchar_t _CONST_RETURN*
__cdecl
wcsrchr(
    _In_z_ const wchar_t *_Str,
    _In_ wchar_t _Ch);

_CRT_END_C_HEADER
