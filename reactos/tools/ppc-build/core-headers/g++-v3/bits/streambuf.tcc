// Stream buffer classes -*- C++ -*-

// Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002, 2003
// Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING.  If not, write to the Free
// Software Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307,
// USA.

// As a special exception, you may use this file as part of a free software
// library without restriction.  Specifically, if other files instantiate
// templates or use macros or inline functions from this file, or you compile
// this file and link it with other files to produce an executable, this
// file does not by itself cause the resulting executable to be covered by
// the GNU General Public License.  This exception does not however
// invalidate any other reasons why the executable file might be covered by
// the GNU General Public License.

//
// ISO C++ 14882: 27.5  Stream buffers
//

#ifndef _STREAMBUF_TCC
#define _STREAMBUF_TCC 1

#pragma GCC system_header

namespace std
{
  template<typename _CharT, typename _Traits>
    streamsize
    basic_streambuf<_CharT, _Traits>::
    xsgetn(char_type* __s, streamsize __n)
    {
      streamsize __ret = 0;
      while (__ret < __n)
	{
	  const size_t __buf_len = this->egptr() - this->gptr();
	  if (__buf_len)
	    {
	      const size_t __remaining = __n - __ret;
	      const size_t __len = std::min(__buf_len, __remaining);
	      traits_type::copy(__s, this->gptr(), __len);
	      __ret += __len;
	      __s += __len;
	      this->gbump(__len);
	    }

	  if (__ret < __n)
	    {
	      const int_type __c = this->uflow();
	      if (!traits_type::eq_int_type(__c, traits_type::eof()))
		{
		  traits_type::assign(*__s++, traits_type::to_char_type(__c));
		  ++__ret;
		}
	      else
		break;
	    }
	}
      return __ret;
    }

  template<typename _CharT, typename _Traits>
    streamsize
    basic_streambuf<_CharT, _Traits>::
    xsputn(const char_type* __s, streamsize __n)
    {
      streamsize __ret = 0;
      while (__ret < __n)
	{
	  const size_t __buf_len = this->epptr() - this->pptr();
	  if (__buf_len)
	    {
	      const size_t __remaining = __n - __ret;
	      const size_t __len = std::min(__buf_len, __remaining);
	      traits_type::copy(this->pptr(), __s, __len);
	      __ret += __len;
	      __s += __len;
	      this->pbump(__len);
	    }

	  if (__ret < __n)
	    {
	      int_type __c = this->overflow(traits_type::to_int_type(*__s));
	      if (!traits_type::eq_int_type(__c, traits_type::eof()))
		{
		  ++__ret;
		  ++__s;
		}
	      else
		break;
	    }
	}
      return __ret;
    }

  // Conceivably, this could be used to implement buffer-to-buffer
  // copies, if this was ever desired in an un-ambiguous way by the
  // standard. If so, then checks for __ios being zero would be
  // necessary.
  template<typename _CharT, typename _Traits>
    streamsize
    __copy_streambufs(basic_streambuf<_CharT, _Traits>* __sbin,
		      basic_streambuf<_CharT, _Traits>* __sbout)
    {
      streamsize __ret = 0;
      typename _Traits::int_type __c = __sbin->sgetc();
      while (!_Traits::eq_int_type(__c, _Traits::eof()))
	{
	  const size_t __n = __sbin->egptr() - __sbin->gptr();
	  if (__n > 1)
	    {
	      const size_t __wrote = __sbout->sputn(__sbin->gptr(), __n);
	      __sbin->gbump(__wrote);
	      __ret += __wrote;
	      if (__wrote < __n)
		break;
	      __c = __sbin->underflow();
	    }
	  else
	    {
	      __c = __sbout->sputc(_Traits::to_char_type(__c));
	      if (_Traits::eq_int_type(__c, _Traits::eof()))
		break;
	      ++__ret;
	      __c = __sbin->snextc();
	    }
	}
      return __ret;
    }

  // Inhibit implicit instantiations for required instantiations,
  // which are defined via explicit instantiations elsewhere.
  // NB:  This syntax is a GNU extension.
#if _GLIBCXX_EXTERN_TEMPLATE
  extern template class basic_streambuf<char>;
  extern template
    streamsize
    __copy_streambufs(basic_streambuf<char>*, basic_streambuf<char>*);

#ifdef _GLIBCXX_USE_WCHAR_T
  extern template class basic_streambuf<wchar_t>;
  extern template
    streamsize
    __copy_streambufs(basic_streambuf<wchar_t>*, basic_streambuf<wchar_t>*);
#endif
#endif
} // namespace std

#endif
