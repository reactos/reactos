// String based streams -*- C++ -*-

// Copyright (C) 1997, 1998, 1999, 2001, 2002, 2003
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
// ISO C++ 14882: 27.7  String-based streams
//

#ifndef _SSTREAM_TCC
#define _SSTREAM_TCC 1

#pragma GCC system_header

#include <sstream>

namespace std
{
  template <class _CharT, class _Traits, class _Alloc>
    typename basic_stringbuf<_CharT, _Traits, _Alloc>::int_type
    basic_stringbuf<_CharT, _Traits, _Alloc>::
    pbackfail(int_type __c)
    {
      int_type __ret = traits_type::eof();
      const bool __testeof = traits_type::eq_int_type(__c, __ret);

      if (this->eback() < this->gptr())
	{
	  const bool __testeq = traits_type::eq(traits_type::to_char_type(__c),
						this->gptr()[-1]);
	  this->gbump(-1);

	  // Try to put back __c into input sequence in one of three ways.
	  // Order these tests done in is unspecified by the standard.
	  if (!__testeof && __testeq)
	    __ret = __c;
	  else if (__testeof)
	    __ret = traits_type::not_eof(__c);
	  else
	    {
	      *this->gptr() = traits_type::to_char_type(__c);
	      __ret = __c;
	    }
	}
      return __ret;
    }

  template <class _CharT, class _Traits, class _Alloc>
    typename basic_stringbuf<_CharT, _Traits, _Alloc>::int_type
    basic_stringbuf<_CharT, _Traits, _Alloc>::
    overflow(int_type __c)
    {
      const bool __testout = this->_M_mode & ios_base::out;
      if (__builtin_expect(!__testout, false))
	return traits_type::eof();

      const bool __testeof = traits_type::eq_int_type(__c, traits_type::eof());
      if (__builtin_expect(__testeof, false))
	return traits_type::not_eof(__c);

      const __size_type __capacity = _M_string.capacity();
      const __size_type __max_size = _M_string.max_size();
      const bool __testput = this->pptr() < this->epptr();
      if (__builtin_expect(!__testput && __capacity == __max_size, false))
	return traits_type::eof();

      // Try to append __c into output sequence in one of two ways.
      // Order these tests done in is unspecified by the standard.
      if (!__testput)
	{
	  // NB: Start ostringstream buffers at 512 chars. This is an
	  // experimental value (pronounced "arbitrary" in some of the
	  // hipper english-speaking countries), and can be changed to
	  // suit particular needs.
	  // Then, in virtue of DR 169 (TC) we are allowed to grow more
	  // than one char.
	  const __size_type __opt_len = std::max(__size_type(2 * __capacity),
						 __size_type(512));
	  const __size_type __len = std::min(__opt_len, __max_size);
	  __string_type __tmp;
	  __tmp.reserve(__len);
	  __tmp.assign(_M_string.data(), this->epptr() - this->pbase());
	  _M_string.swap(__tmp);
	  _M_sync(const_cast<char_type*>(_M_string.data()),
		  this->gptr() - this->eback(), this->pptr() - this->pbase());
	}
      return this->sputc(traits_type::to_char_type(__c));
    }

  template <class _CharT, class _Traits, class _Alloc>
    typename basic_stringbuf<_CharT, _Traits, _Alloc>::int_type
    basic_stringbuf<_CharT, _Traits, _Alloc>::
    underflow()
    {
      int_type __ret = traits_type::eof();
      const bool __testin = this->_M_mode & ios_base::in;
      if (__testin)
	{
	  // Update egptr() to match the actual string end.
	  _M_update_egptr();

	  if (this->gptr() < this->egptr())
	    __ret = traits_type::to_int_type(*this->gptr());
	}
      return __ret;
    }

  template <class _CharT, class _Traits, class _Alloc>
    typename basic_stringbuf<_CharT, _Traits, _Alloc>::pos_type
    basic_stringbuf<_CharT, _Traits, _Alloc>::
    seekoff(off_type __off, ios_base::seekdir __way, ios_base::openmode __mode)
    {
      pos_type __ret =  pos_type(off_type(-1));
      bool __testin = (ios_base::in & this->_M_mode & __mode) != 0;
      bool __testout = (ios_base::out & this->_M_mode & __mode) != 0;
      const bool __testboth = __testin && __testout && __way != ios_base::cur;
      __testin &= !(__mode & ios_base::out);
      __testout &= !(__mode & ios_base::in);

      // _GLIBCXX_RESOLVE_LIB_DEFECTS
      // 453. basic_stringbuf::seekoff need not always fail for an empty stream.
      const char_type* __beg = __testin ? this->eback() : this->pbase();
      if ((__beg || !__off) && (__testin || __testout || __testboth))
	{
	  _M_update_egptr();

	  off_type __newoffi = 0;
	  off_type __newoffo = 0;
	  if (__way == ios_base::cur)
	    {
	      __newoffi = this->gptr() - __beg;
	      __newoffo = this->pptr() - __beg;
	    }
	  else if (__way == ios_base::end)
	    __newoffo = __newoffi = this->egptr() - __beg;

	  if ((__testin || __testboth)
	      && __newoffi + __off >= 0
	      && this->egptr() - __beg >= __newoffi + __off)
	    {
	      this->gbump((__beg + __newoffi + __off) - this->gptr());
	      __ret = pos_type(__newoffi);
	    }
	  if ((__testout || __testboth)
	      && __newoffo + __off >= 0
	      && this->egptr() - __beg >= __newoffo + __off)
	    {
	      this->pbump((__beg + __newoffo + __off) - this->pptr());
	      __ret = pos_type(__newoffo);
	    }
	}
      return __ret;
    }

  template <class _CharT, class _Traits, class _Alloc>
    typename basic_stringbuf<_CharT, _Traits, _Alloc>::pos_type
    basic_stringbuf<_CharT, _Traits, _Alloc>::
    seekpos(pos_type __sp, ios_base::openmode __mode)
    {
      pos_type __ret =  pos_type(off_type(-1));
      const bool __testin = (ios_base::in & this->_M_mode & __mode) != 0;
      const bool __testout = (ios_base::out & this->_M_mode & __mode) != 0;

      const char_type* __beg = __testin ? this->eback() : this->pbase();
      if (__beg)
	{
	  _M_update_egptr();

	  off_type __pos(__sp);
	  const bool __testpos = 0 <= __pos
	                         && __pos <=  this->egptr() - __beg;
	  if ((__testin || __testout) && __testpos)
	    {
	      if (__testin)
		this->gbump((__beg + __pos) - this->gptr());
	      if (__testout)
                this->pbump((__beg + __pos) - this->pptr());
	      __ret = __sp;
	    }
	}
      return __ret;
    }

  // Inhibit implicit instantiations for required instantiations,
  // which are defined via explicit instantiations elsewhere.
  // NB:  This syntax is a GNU extension.
#if _GLIBCXX_EXTERN_TEMPLATE
  extern template class basic_stringbuf<char>;
  extern template class basic_istringstream<char>;
  extern template class basic_ostringstream<char>;
  extern template class basic_stringstream<char>;

#ifdef _GLIBCXX_USE_WCHAR_T
  extern template class basic_stringbuf<wchar_t>;
  extern template class basic_istringstream<wchar_t>;
  extern template class basic_ostringstream<wchar_t>;
  extern template class basic_stringstream<wchar_t>;
#endif
#endif
} // namespace std

#endif
