// String based streams -*- C++ -*-

// Copyright (C) 1997, 1998, 1999, 2002, 2003, 2004
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

/** @file sstream
 *  This is a Standard C++ Library header.  You should @c #include this header
 *  in your programs, rather than any of the "st[dl]_*.h" implementation files.
 */

#ifndef _GLIBCXX_SSTREAM
#define _GLIBCXX_SSTREAM 1

#pragma GCC system_header

#include <istream>
#include <ostream>

namespace std
{
  // [27.7.1] template class basic_stringbuf
  /**
   *  @brief  The actual work of input and output (for std::string).
   *
   *  This class associates either or both of its input and output sequences
   *  with a sequence of characters, which can be initialized from, or made
   *  available as, a @c std::basic_string.  (Paraphrased from [27.7.1]/1.)
   *
   *  For this class, open modes (of type @c ios_base::openmode) have
   *  @c in set if the input sequence can be read, and @c out set if the
   *  output sequence can be written.
  */
  template<typename _CharT, typename _Traits, typename _Alloc>
    class basic_stringbuf : public basic_streambuf<_CharT, _Traits>
    {
    public:
      // Types:
      typedef _CharT 					char_type;
      typedef _Traits 					traits_type;
      // _GLIBCXX_RESOLVE_LIB_DEFECTS
      // 251. basic_stringbuf missing allocator_type
      typedef _Alloc				       	allocator_type;
      typedef typename traits_type::int_type 		int_type;
      typedef typename traits_type::pos_type 		pos_type;
      typedef typename traits_type::off_type 		off_type;

      //@{
      /**
       *  @if maint
       *  @doctodo
       *  @endif
      */
      typedef basic_streambuf<char_type, traits_type>  	__streambuf_type;
      typedef basic_string<char_type, _Traits, _Alloc> 	__string_type;
      typedef typename __string_type::size_type		__size_type;
      //@}

    protected:
      /**
       *  @if maint
       *  Place to stash in || out || in | out settings for current stringbuf.
       *  @endif
      */
      ios_base::openmode 	_M_mode;

      // Data Members:
      /**
       *  @if maint
       *  @doctodo
       *  @endif
      */
      __string_type 		_M_string;

    public:
      // Constructors:
      /**
       *  @brief  Starts with an empty string buffer.
       *  @param  mode  Whether the buffer can read, or write, or both.
       *
       *  The default constructor initializes the parent class using its
       *  own default ctor.
      */
      explicit
      basic_stringbuf(ios_base::openmode __mode = ios_base::in | ios_base::out)
      : __streambuf_type(), _M_mode(__mode), _M_string()
      { }

      /**
       *  @brief  Starts with an existing string buffer.
       *  @param  str  A string to copy as a starting buffer.
       *  @param  mode  Whether the buffer can read, or write, or both.
       *
       *  This constructor initializes the parent class using its
       *  own default ctor.
      */
      explicit
      basic_stringbuf(const __string_type& __str,
		      ios_base::openmode __mode = ios_base::in | ios_base::out)
      : __streambuf_type(), _M_mode(), _M_string(__str.data(), __str.size())
      { _M_stringbuf_init(__mode); }

      // Get and set:
      /**
       *  @brief  Copying out the string buffer.
       *  @return  A copy of one of the underlying sequences.
       *
       *  "If the buffer is only created in input mode, the underlying
       *  character sequence is equal to the input sequence; otherwise, it
       *  is equal to the output sequence." [27.7.1.2]/1
      */
      __string_type
      str() const
      {
	if (this->pptr())
	  {
	    // The current egptr() may not be the actual string end.
	    if (this->pptr() > this->egptr())
	      return __string_type(this->pbase(), this->pptr());
	    else
 	      return __string_type(this->pbase(), this->egptr());
	  }
	else
	  return _M_string;
      }

      /**
       *  @brief  Setting a new buffer.
       *  @param  s  The string to use as a new sequence.
       *
       *  Deallocates any previous stored sequence, then copies @a s to
       *  use as a new one.
      */
      void
      str(const __string_type& __s)
      {
	// Cannot use _M_string = __s, since v3 strings are COW.
	_M_string.assign(__s.data(), __s.size());
	_M_stringbuf_init(this->_M_mode);
      }

    protected:
      // Common initialization code goes here.
      /**
       *  @if maint
       *  @doctodo
       *  @endif
      */
      void
      _M_stringbuf_init(ios_base::openmode __mode)
      {
	this->_M_mode = __mode;

	__size_type __len = 0;
	if (this->_M_mode & (ios_base::ate | ios_base::app))
	  __len = _M_string.size();
	_M_sync(const_cast<char_type*>(_M_string.data()), 0, __len);
      }

      // [documentation is inherited]
      virtual int_type
      underflow();

      // [documentation is inherited]
      virtual int_type
      pbackfail(int_type __c = traits_type::eof());

      // [documentation is inherited]
      virtual int_type
      overflow(int_type __c = traits_type::eof());

      /**
       *  @brief  Manipulates the buffer.
       *  @param  s  Pointer to a buffer area.
       *  @param  n  Size of @a s.
       *  @return  @c this
       *
       *  If no buffer has already been created, and both @a s and @a n are
       *  non-zero, then @c s is used as a buffer; see
       *  http://gcc.gnu.org/onlinedocs/libstdc++/27_io/howto.html#2
       *  for more.
      */
      virtual __streambuf_type*
      setbuf(char_type* __s, streamsize __n)
      {
	if (__s && __n >= 0)
	  {
	    // This is implementation-defined behavior, and assumes
	    // that an external char_type array of length __n exists
	    // and has been pre-allocated. If this is not the case,
	    // things will quickly blow up.
	    
	    // Step 1: Destroy the current internal array.
	    _M_string = __string_type(__s, __n);
	    
	    // Step 2: Use the external array.
	    _M_sync(__s, 0, 0);
	  }
	return this;
      }

      // [documentation is inherited]
      virtual pos_type
      seekoff(off_type __off, ios_base::seekdir __way,
	      ios_base::openmode __mode = ios_base::in | ios_base::out);

      // [documentation is inherited]
      virtual pos_type
      seekpos(pos_type __sp,
	      ios_base::openmode __mode = ios_base::in | ios_base::out);

      // Internal function for correctly updating the internal buffer
      // for a particular _M_string, due to initialization or
      // re-sizing of an existing _M_string.
      // Assumes: contents of _M_string and internal buffer match exactly.
      // __i == _M_in_cur - _M_in_beg
      // __o == _M_out_cur - _M_out_beg
      /**
       *  @if maint
       *  @doctodo
       *  @endif
      */
      void
      _M_sync(char_type* __base, __size_type __i, __size_type __o)
      {
	const bool __testin = this->_M_mode & ios_base::in;
	const bool __testout = this->_M_mode & ios_base::out;
	const __size_type __len = _M_string.size();

	if (__testin)
	  this->setg(__base, __base + __i, __base + __len);
	if (__testout)
	  {
	    this->setp(__base, __base + _M_string.capacity());
	    this->pbump(__o);
	    // We need a pointer to the string end anyway, even when
	    // !__testin: in that case, however, for the correct
	    // functioning of the streambuf inlines all the get area
	    // pointers must be identical.
	    if (!__testin)
	      this->setg(__base + __len, __base + __len, __base + __len);
	  }
      }

      // Internal function for correctly updating egptr() to the actual
      // string end.
      void
      _M_update_egptr()
      {
	const bool __testin = this->_M_mode & ios_base::in;

	if (this->pptr() && this->pptr() > this->egptr())
	  if (__testin)
	    this->setg(this->eback(), this->gptr(), this->pptr());
	  else
	    this->setg(this->pptr(), this->pptr(), this->pptr());
      }
    };


  // [27.7.2] Template class basic_istringstream
  /**
   *  @brief  Controlling input for std::string.
   *
   *  This class supports reading from objects of type std::basic_string,
   *  using the inherited functions from std::basic_istream.  To control
   *  the associated sequence, an instance of std::basic_stringbuf is used,
   *  which this page refers to as @c sb.
  */
  template<typename _CharT, typename _Traits, typename _Alloc>
    class basic_istringstream : public basic_istream<_CharT, _Traits>
    {
    public:
      // Types:
      typedef _CharT 					char_type;
      typedef _Traits 					traits_type;
      // _GLIBCXX_RESOLVE_LIB_DEFECTS
      // 251. basic_stringbuf missing allocator_type
      typedef _Alloc				       	allocator_type;
      typedef typename traits_type::int_type 		int_type;
      typedef typename traits_type::pos_type 		pos_type;
      typedef typename traits_type::off_type 		off_type;

      // Non-standard types:
      typedef basic_string<_CharT, _Traits, _Alloc> 	__string_type;
      typedef basic_stringbuf<_CharT, _Traits, _Alloc> 	__stringbuf_type;
      typedef basic_istream<char_type, traits_type>	__istream_type;

    private:
      /**
       *  @if maint
       *  @doctodo
       *  @endif
      */
      __stringbuf_type	_M_stringbuf;

    public:
      // Constructors:
      /**
       *  @brief  Default constructor starts with an empty string buffer.
       *  @param  mode  Whether the buffer can read, or write, or both.
       *
       *  @c ios_base::in is automatically included in @a mode.
       *
       *  Initializes @c sb using @c mode|in, and passes @c &sb to the base
       *  class initializer.  Does not allocate any buffer.
       *
       *  @if maint
       *  That's a lie.  We initialize the base class with NULL, because the
       *  string class does its own memory management.
       *  @endif
      */
      explicit
      basic_istringstream(ios_base::openmode __mode = ios_base::in)
      : __istream_type(), _M_stringbuf(__mode | ios_base::in)
      { this->init(&_M_stringbuf); }

      /**
       *  @brief  Starts with an existing string buffer.
       *  @param  str  A string to copy as a starting buffer.
       *  @param  mode  Whether the buffer can read, or write, or both.
       *
       *  @c ios_base::in is automatically included in @a mode.
       *
       *  Initializes @c sb using @a str and @c mode|in, and passes @c &sb
       *  to the base class initializer.
       *
       *  @if maint
       *  That's a lie.  We initialize the base class with NULL, because the
       *  string class does its own memory management.
       *  @endif
      */
      explicit
      basic_istringstream(const __string_type& __str,
			  ios_base::openmode __mode = ios_base::in)
      : __istream_type(), _M_stringbuf(__str, __mode | ios_base::in)
      { this->init(&_M_stringbuf); }

      /**
       *  @brief  The destructor does nothing.
       *
       *  The buffer is deallocated by the stringbuf object, not the
       *  formatting stream.
      */
      ~basic_istringstream()
      { }

      // Members:
      /**
       *  @brief  Accessing the underlying buffer.
       *  @return  The current basic_stringbuf buffer.
       *
       *  This hides both signatures of std::basic_ios::rdbuf().
      */
      __stringbuf_type*
      rdbuf() const
      { return const_cast<__stringbuf_type*>(&_M_stringbuf); }

      /**
       *  @brief  Copying out the string buffer.
       *  @return  @c rdbuf()->str()
      */
      __string_type
      str() const
      { return _M_stringbuf.str(); }

      /**
       *  @brief  Setting a new buffer.
       *  @param  s  The string to use as a new sequence.
       *
       *  Calls @c rdbuf()->str(s).
      */
      void
      str(const __string_type& __s)
      { _M_stringbuf.str(__s); }
    };


  // [27.7.3] Template class basic_ostringstream
  /**
   *  @brief  Controlling output for std::string.
   *
   *  This class supports writing to objects of type std::basic_string,
   *  using the inherited functions from std::basic_ostream.  To control
   *  the associated sequence, an instance of std::basic_stringbuf is used,
   *  which this page refers to as @c sb.
  */
  template <typename _CharT, typename _Traits, typename _Alloc>
    class basic_ostringstream : public basic_ostream<_CharT, _Traits>
    {
    public:
      // Types:
      typedef _CharT 					char_type;
      typedef _Traits 					traits_type;
      // _GLIBCXX_RESOLVE_LIB_DEFECTS
      // 251. basic_stringbuf missing allocator_type
      typedef _Alloc				       	allocator_type;
      typedef typename traits_type::int_type 		int_type;
      typedef typename traits_type::pos_type 		pos_type;
      typedef typename traits_type::off_type 		off_type;

      // Non-standard types:
      typedef basic_string<_CharT, _Traits, _Alloc> 	__string_type;
      typedef basic_stringbuf<_CharT, _Traits, _Alloc> 	__stringbuf_type;
      typedef basic_ostream<char_type, traits_type>	__ostream_type;

    private:
      /**
       *  @if maint
       *  @doctodo
       *  @endif
      */
      __stringbuf_type	_M_stringbuf;

    public:
      // Constructors/destructor:
      /**
       *  @brief  Default constructor starts with an empty string buffer.
       *  @param  mode  Whether the buffer can read, or write, or both.
       *
       *  @c ios_base::out is automatically included in @a mode.
       *
       *  Initializes @c sb using @c mode|out, and passes @c &sb to the base
       *  class initializer.  Does not allocate any buffer.
       *
       *  @if maint
       *  That's a lie.  We initialize the base class with NULL, because the
       *  string class does its own memory management.
       *  @endif
      */
      explicit
      basic_ostringstream(ios_base::openmode __mode = ios_base::out)
      : __ostream_type(), _M_stringbuf(__mode | ios_base::out)
      { this->init(&_M_stringbuf); }

      /**
       *  @brief  Starts with an existing string buffer.
       *  @param  str  A string to copy as a starting buffer.
       *  @param  mode  Whether the buffer can read, or write, or both.
       *
       *  @c ios_base::out is automatically included in @a mode.
       *
       *  Initializes @c sb using @a str and @c mode|out, and passes @c &sb
       *  to the base class initializer.
       *
       *  @if maint
       *  That's a lie.  We initialize the base class with NULL, because the
       *  string class does its own memory management.
       *  @endif
      */
      explicit
      basic_ostringstream(const __string_type& __str,
			  ios_base::openmode __mode = ios_base::out)
      : __ostream_type(), _M_stringbuf(__str, __mode | ios_base::out)
      { this->init(&_M_stringbuf); }

      /**
       *  @brief  The destructor does nothing.
       *
       *  The buffer is deallocated by the stringbuf object, not the
       *  formatting stream.
      */
      ~basic_ostringstream()
      { }

      // Members:
      /**
       *  @brief  Accessing the underlying buffer.
       *  @return  The current basic_stringbuf buffer.
       *
       *  This hides both signatures of std::basic_ios::rdbuf().
      */
      __stringbuf_type*
      rdbuf() const
      { return const_cast<__stringbuf_type*>(&_M_stringbuf); }

      /**
       *  @brief  Copying out the string buffer.
       *  @return  @c rdbuf()->str()
      */
      __string_type
      str() const
      { return _M_stringbuf.str(); }

      /**
       *  @brief  Setting a new buffer.
       *  @param  s  The string to use as a new sequence.
       *
       *  Calls @c rdbuf()->str(s).
      */
      void
      str(const __string_type& __s)
      { _M_stringbuf.str(__s); }
    };


  // [27.7.4] Template class basic_stringstream
  /**
   *  @brief  Controlling input and output for std::string.
   *
   *  This class supports reading from and writing to objects of type
   *  std::basic_string, using the inherited functions from
   *  std::basic_iostream.  To control the associated sequence, an instance
   *  of std::basic_stringbuf is used, which this page refers to as @c sb.
  */
  template <typename _CharT, typename _Traits, typename _Alloc>
    class basic_stringstream : public basic_iostream<_CharT, _Traits>
    {
    public:
      // Types:
      typedef _CharT 					char_type;
      typedef _Traits 					traits_type;
      // _GLIBCXX_RESOLVE_LIB_DEFECTS
      // 251. basic_stringbuf missing allocator_type
      typedef _Alloc				       	allocator_type;
      typedef typename traits_type::int_type 		int_type;
      typedef typename traits_type::pos_type 		pos_type;
      typedef typename traits_type::off_type 		off_type;

      // Non-standard Types:
      typedef basic_string<_CharT, _Traits, _Alloc> 	__string_type;
      typedef basic_stringbuf<_CharT, _Traits, _Alloc> 	__stringbuf_type;
      typedef basic_iostream<char_type, traits_type>	__iostream_type;

    private:
      /**
       *  @if maint
       *  @doctodo
       *  @endif
      */
      __stringbuf_type	_M_stringbuf;

    public:
      // Constructors/destructors
      /**
       *  @brief  Default constructor starts with an empty string buffer.
       *  @param  mode  Whether the buffer can read, or write, or both.
       *
       *  Initializes @c sb using @c mode, and passes @c &sb to the base
       *  class initializer.  Does not allocate any buffer.
       *
       *  @if maint
       *  That's a lie.  We initialize the base class with NULL, because the
       *  string class does its own memory management.
       *  @endif
      */
      explicit
      basic_stringstream(ios_base::openmode __m = ios_base::out | ios_base::in)
      : __iostream_type(), _M_stringbuf(__m)
      { this->init(&_M_stringbuf); }

      /**
       *  @brief  Starts with an existing string buffer.
       *  @param  str  A string to copy as a starting buffer.
       *  @param  mode  Whether the buffer can read, or write, or both.
       *
       *  Initializes @c sb using @a str and @c mode, and passes @c &sb
       *  to the base class initializer.
       *
       *  @if maint
       *  That's a lie.  We initialize the base class with NULL, because the
       *  string class does its own memory management.
       *  @endif
      */
      explicit
      basic_stringstream(const __string_type& __str,
			 ios_base::openmode __m = ios_base::out | ios_base::in)
      : __iostream_type(), _M_stringbuf(__str, __m)
      { this->init(&_M_stringbuf); }

      /**
       *  @brief  The destructor does nothing.
       *
       *  The buffer is deallocated by the stringbuf object, not the
       *  formatting stream.
      */
      ~basic_stringstream()
      { }

      // Members:
      /**
       *  @brief  Accessing the underlying buffer.
       *  @return  The current basic_stringbuf buffer.
       *
       *  This hides both signatures of std::basic_ios::rdbuf().
      */
      __stringbuf_type*
      rdbuf() const
      { return const_cast<__stringbuf_type*>(&_M_stringbuf); }

      /**
       *  @brief  Copying out the string buffer.
       *  @return  @c rdbuf()->str()
      */
      __string_type
      str() const
      { return _M_stringbuf.str(); }

      /**
       *  @brief  Setting a new buffer.
       *  @param  s  The string to use as a new sequence.
       *
       *  Calls @c rdbuf()->str(s).
      */
      void
      str(const __string_type& __s)
      { _M_stringbuf.str(__s); }
    };
} // namespace std

#ifndef _GLIBCXX_EXPORT_TEMPLATE
# include <bits/sstream.tcc>
#endif

#endif /* _GLIBCXX_SSTREAM */
