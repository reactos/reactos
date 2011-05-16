/*
 * Copyright (c) 1999
 * Silicon Graphics Computer Systems, Inc.
 *
 * Copyright (c) 1999
 * Boris Fomitchev
 *
 * This material is provided "as is", with absolutely no warranty expressed
 * or implied. Any use is at your own risk.
 *
 * Permission to use or copy this software for any purpose is hereby granted
 * without fee, provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 *
 */

#include "stlport_prefix.h"

#ifdef _STLP_USE_UNIX_IO
# include "details/fstream_unistd.cpp"
#elif defined(_STLP_USE_STDIO_IO)
# include "details/fstream_stdio.cpp"
#elif defined(_STLP_USE_WIN32_IO)
# include "details/fstream_win32io.cpp"
#else
#  error "Can't recognize IO scheme to use"
#endif

_STLP_BEGIN_NAMESPACE

// fbp : let us map 1 MB maximum, just be sure not to trash VM
#define MMAP_CHUNK 0x100000L

_Underflow< char, char_traits<char> >::int_type _STLP_CALL
_Underflow< char, char_traits<char> >::_M_doit(basic_filebuf<char, char_traits<char> >* __this)
{
  typedef char_traits<char> traits_type;
  typedef traits_type::int_type int_type;

  if (!__this->_M_in_input_mode) {
    if (!__this->_M_switch_to_input_mode())
      return traits_type::eof();
  }
  else if (__this->_M_in_putback_mode) {
    __this->_M_exit_putback_mode();
    if (__this->gptr() != __this->egptr()) {
      int_type __c = traits_type::to_int_type(*__this->gptr());
      return __c;
    }
  }

  // If it's a disk file, and if the internal and external character
  // sequences are guaranteed to be identical, then try to use memory
  // mapped I/O.  Otherwise, revert to ordinary read.
  if (__this->_M_base.__regular_file()
      && __this->_M_always_noconv
      && __this->_M_base._M_in_binary_mode()) {
    // If we've mmapped part of the file already, then unmap it.
    if (__this->_M_mmap_base)
      __this->_M_base._M_unmap(__this->_M_mmap_base, __this->_M_mmap_len);

    // Determine the position where we start mapping.  It has to be
    // a multiple of the page size.
    streamoff __cur = __this->_M_base._M_seek(0, ios_base::cur);
    streamoff __size = __this->_M_base._M_file_size();
    if (__size > 0 && __cur >= 0 && __cur < __size) {
      streamoff __offset = (__cur / __this->_M_base.__page_size()) * __this->_M_base.__page_size();
      streamoff __remainder = __cur - __offset;

      __this->_M_mmap_len = __size - __offset;

      if (__this->_M_mmap_len > MMAP_CHUNK)
        __this->_M_mmap_len = MMAP_CHUNK;

      if ((__this->_M_mmap_base = __this->_M_base._M_mmap(__offset, __this->_M_mmap_len)) != 0) {
        __this->setg(__STATIC_CAST(char*, __this->_M_mmap_base),
                     __STATIC_CAST(char*, __this->_M_mmap_base) + __STATIC_CAST(ptrdiff_t, __remainder),
                     __STATIC_CAST(char*, __this->_M_mmap_base) + __STATIC_CAST(ptrdiff_t, __this->_M_mmap_len));
        return traits_type::to_int_type(*__this->gptr());
      }
      else
        __this->_M_mmap_len = 0;
    }
    else {
      __this->_M_mmap_base = 0;
      __this->_M_mmap_len = 0;
    }
  }

  return __this->_M_underflow_aux();
}

//----------------------------------------------------------------------
// Force instantiation of filebuf and fstream classes.
#if !defined(_STLP_NO_FORCE_INSTANTIATE)

template class basic_filebuf<char, char_traits<char> >;
template class basic_ifstream<char, char_traits<char> >;
template class basic_ofstream<char, char_traits<char> >;
template class basic_fstream<char, char_traits<char> >;

#  if !defined (_STLP_NO_WCHAR_T)
template class _Underflow<wchar_t, char_traits<wchar_t> >;
template class basic_filebuf<wchar_t, char_traits<wchar_t> >;
template class basic_ifstream<wchar_t, char_traits<wchar_t> >;
template class basic_ofstream<wchar_t, char_traits<wchar_t> >;
template class basic_fstream<wchar_t, char_traits<wchar_t> >;
#  endif /* _STLP_NO_WCHAR_T */

#endif

_STLP_END_NAMESPACE
