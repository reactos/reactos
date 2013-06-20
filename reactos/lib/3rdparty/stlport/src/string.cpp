#include "stlport_prefix.h"

#include <string>

_STLP_BEGIN_NAMESPACE

#if defined(_STLP_USE_WIDE_INTERFACE)
_STLP_MOVE_TO_PRIV_NAMESPACE

wstring __ASCIIToWide(const char *ascii) {
    size_t size = strlen(ascii);
    wchar_t* buff = new wchar_t[size+1];
    mbstowcs(buff, ascii, size);
    buff[size] = 0x00;
    wstring ret(buff);
    delete[] buff;
    return ret;
}
string __WideToASCII(const wchar_t *wide) {
    size_t size = wcslen(wide);
    char* buff = new char[size+1];
    wcstombs(buff, wide, size);
    buff[size] = 0;
    string ret(buff);
    delete[] buff;
    return ret;
}
_STLP_MOVE_TO_STD_NAMESPACE
#endif

#if !defined (_STLP_NO_FORCE_INSTANTIATE)

template class _STLP_CLASS_DECLSPEC allocator<char>;

_STLP_MOVE_TO_PRIV_NAMESPACE

template class _STLP_CLASS_DECLSPEC _STLP_alloc_proxy<char*, char, allocator<char> >;
template class _STLP_CLASS_DECLSPEC _String_base<char, allocator<char> >;

#  if defined (_STLP_DEBUG) && !defined (__SUNPRO_CC) && !defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND)
#    define basic_string _STLP_NON_DBG_NAME(str)

template class _STLP_CLASS_DECLSPEC basic_string<char, char_traits<char>, allocator<char> >;
template class _STLP_CLASS_DECLSPEC __construct_checker<basic_string<char, char_traits<char>, allocator<char> > >;

#    undef basic_string
#  endif

#  if defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND)
#    define basic_string _STLP_NO_MEM_T_NAME(str)
#  else
_STLP_MOVE_TO_STD_NAMESPACE
#  endif

template class _STLP_CLASS_DECLSPEC basic_string<char, char_traits<char>, allocator<char> >;

#  if defined (basic_string)
_STLP_MOVE_TO_STD_NAMESPACE
#    undef basic_string
#  endif

#  if !defined (_STLP_NO_WCHAR_T)
template class _STLP_CLASS_DECLSPEC allocator<wchar_t>;

_STLP_MOVE_TO_PRIV_NAMESPACE

template class _STLP_CLASS_DECLSPEC _String_base<wchar_t, allocator<wchar_t> >;

#    if defined (_STLP_DEBUG) && !defined (__SUNPRO_CC) && !defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND)
#      define basic_string _STLP_NON_DBG_NAME(str)

template class _STLP_CLASS_DECLSPEC basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >;
template class _STLP_CLASS_DECLSPEC __construct_checker<basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> > >;

#      undef basic_string
#    endif

#    if defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND)
#      define basic_string _STLP_NO_MEM_T_NAME(str)
#    else
_STLP_MOVE_TO_STD_NAMESPACE
#    endif

template class _STLP_CLASS_DECLSPEC basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >;

#    if defined (basic_string)
_STLP_MOVE_TO_STD_NAMESPACE
#      undef basic_string
#    endif
#  endif
#endif

_STLP_END_NAMESPACE
