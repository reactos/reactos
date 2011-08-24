/*
 * Copyright (c) 1998
 * Mark of the Unicorn, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Mark of the Unicorn, Inc. makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 */
#if defined( _STLP_USE_MSIPL ) && !defined( _STLP_MSL_STRING_H_ )
#define _STLP_MSL_STRING_H_

//# define char_traits __msl_char_traits
# define basic_string __msl_basic_string
# define b_str_ref __msl_b_str_ref
# define basic_istream __msl_basic_istream
# define basic_ostream __msl_basic_ostream
# define string __msl_string
# define wstring __msl_wstring
# define iterator_traits __msl_iterator_traits

namespace std
{
  template<class charT, class traits> class basic_istream;
  template<class charT, class traits> class basic_ostream;
}

#if defined (_STLP_HAS_INCLUDE_NEXT)
#  include_next <string>
#else
#  include _STLP_NATIVE_HEADER(string)
#endif

// # undef char_traits
# undef basic_string
# undef b_str_ref
# undef basic_istream
# undef basic_ostream
# undef string
# undef wstring
# undef iterator_traits

#endif
