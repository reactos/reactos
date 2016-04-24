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

#include <algorithm>
#include <locale>
#include <functional>

#include "c_locale.h"

_STLP_BEGIN_NAMESPACE

//----------------------------------------------------------------------
// ctype<char>

// The classic table: static data members.

#if !defined (_STLP_STATIC_CONST_INIT_BUG) && !defined (_STLP_NO_STATIC_CONST_DEFINITION)
//*TY 02/25/2000 - added workaround for MPW compilers; they confuse on in-class static const
const size_t ctype<char>::table_size;
#endif

// This macro is specifically for platforms where isprint() relies
// on separate flag

const ctype_base::mask*
ctype<char>::classic_table() _STLP_NOTHROW {
  /* Ctype table for the ASCII character set. */
  static const ctype_base::mask _S_classic_table[table_size] = {
    cntrl /* null */,
    cntrl /* ^A */,
    cntrl /* ^B */,
    cntrl /* ^C */,
    cntrl /* ^D */,
    cntrl /* ^E */,
    cntrl /* ^F */,
    cntrl /* ^G */,
    cntrl /* ^H */,
    ctype_base::mask(space | cntrl) /* tab */,
    ctype_base::mask(space | cntrl) /* LF */,
    ctype_base::mask(space | cntrl) /* ^K */,
    ctype_base::mask(space | cntrl) /* FF */,
    ctype_base::mask(space | cntrl) /* ^M */,
    cntrl /* ^N */,
    cntrl /* ^O */,
    cntrl /* ^P */,
    cntrl /* ^Q */,
    cntrl /* ^R */,
    cntrl /* ^S */,
    cntrl /* ^T */,
    cntrl /* ^U */,
    cntrl /* ^V */,
    cntrl /* ^W */,
    cntrl /* ^X */,
    cntrl /* ^Y */,
    cntrl /* ^Z */,
    cntrl /* esc */,
    cntrl /* ^\ */,
    cntrl /* ^] */,
    cntrl /* ^^ */,
    cntrl /* ^_ */,
    ctype_base::mask(space | print) /*  */,
    ctype_base::mask(punct | print) /* ! */,
    ctype_base::mask(punct | print) /* " */,
    ctype_base::mask(punct | print) /* # */,
    ctype_base::mask(punct | print) /* $ */,
    ctype_base::mask(punct | print) /* % */,
    ctype_base::mask(punct | print) /* & */,
    ctype_base::mask(punct | print) /* ' */,
    ctype_base::mask(punct | print) /* ( */,
    ctype_base::mask(punct | print) /* ) */,
    ctype_base::mask(punct | print) /* * */,
    ctype_base::mask(punct | print) /* + */,
    ctype_base::mask(punct | print) /* , */,
    ctype_base::mask(punct | print) /* - */,
    ctype_base::mask(punct | print) /* . */,
    ctype_base::mask(punct | print) /* / */,
    ctype_base::mask(digit | print | xdigit) /* 0 */,
    ctype_base::mask(digit | print | xdigit) /* 1 */,
    ctype_base::mask(digit | print | xdigit) /* 2 */,
    ctype_base::mask(digit | print | xdigit) /* 3 */,
    ctype_base::mask(digit | print | xdigit) /* 4 */,
    ctype_base::mask(digit | print | xdigit) /* 5 */,
    ctype_base::mask(digit | print | xdigit) /* 6 */,
    ctype_base::mask(digit | print | xdigit) /* 7 */,
    ctype_base::mask(digit | print | xdigit) /* 8 */,
    ctype_base::mask(digit | print | xdigit) /* 9 */,
    ctype_base::mask(punct | print) /* : */,
    ctype_base::mask(punct | print) /* ; */,
    ctype_base::mask(punct | print) /* < */,
    ctype_base::mask(punct | print) /* = */,
    ctype_base::mask(punct | print) /* > */,
    ctype_base::mask(punct | print) /* ? */,
    ctype_base::mask(punct | print) /* ! */,
    ctype_base::mask(alpha | print | upper | xdigit) /* A */,
    ctype_base::mask(alpha | print | upper | xdigit) /* B */,
    ctype_base::mask(alpha | print | upper | xdigit) /* C */,
    ctype_base::mask(alpha | print | upper | xdigit) /* D */,
    ctype_base::mask(alpha | print | upper | xdigit) /* E */,
    ctype_base::mask(alpha | print | upper | xdigit) /* F */,
    ctype_base::mask(alpha | print | upper) /* G */,
    ctype_base::mask(alpha | print | upper) /* H */,
    ctype_base::mask(alpha | print | upper) /* I */,
    ctype_base::mask(alpha | print | upper) /* J */,
    ctype_base::mask(alpha | print | upper) /* K */,
    ctype_base::mask(alpha | print | upper) /* L */,
    ctype_base::mask(alpha | print | upper) /* M */,
    ctype_base::mask(alpha | print | upper) /* N */,
    ctype_base::mask(alpha | print | upper) /* O */,
    ctype_base::mask(alpha | print | upper) /* P */,
    ctype_base::mask(alpha | print | upper) /* Q */,
    ctype_base::mask(alpha | print | upper) /* R */,
    ctype_base::mask(alpha | print | upper) /* S */,
    ctype_base::mask(alpha | print | upper) /* T */,
    ctype_base::mask(alpha | print | upper) /* U */,
    ctype_base::mask(alpha | print | upper) /* V */,
    ctype_base::mask(alpha | print | upper) /* W */,
    ctype_base::mask(alpha | print | upper) /* X */,
    ctype_base::mask(alpha | print | upper) /* Y */,
    ctype_base::mask(alpha | print | upper) /* Z */,
    ctype_base::mask(punct | print) /* [ */,
    ctype_base::mask(punct | print) /* \ */,
    ctype_base::mask(punct | print) /* ] */,
    ctype_base::mask(punct | print) /* ^ */,
    ctype_base::mask(punct | print) /* _ */,
    ctype_base::mask(punct | print) /* ` */,
    ctype_base::mask(alpha | print | lower | xdigit) /* a */,
    ctype_base::mask(alpha | print | lower | xdigit) /* b */,
    ctype_base::mask(alpha | print | lower | xdigit) /* c */,
    ctype_base::mask(alpha | print | lower | xdigit) /* d */,
    ctype_base::mask(alpha | print | lower | xdigit) /* e */,
    ctype_base::mask(alpha | print | lower | xdigit) /* f */,
    ctype_base::mask(alpha | print | lower) /* g */,
    ctype_base::mask(alpha | print | lower) /* h */,
    ctype_base::mask(alpha | print | lower) /* i */,
    ctype_base::mask(alpha | print | lower) /* j */,
    ctype_base::mask(alpha | print | lower) /* k */,
    ctype_base::mask(alpha | print | lower) /* l */,
    ctype_base::mask(alpha | print | lower) /* m */,
    ctype_base::mask(alpha | print | lower) /* n */,
    ctype_base::mask(alpha | print | lower) /* o */,
    ctype_base::mask(alpha | print | lower) /* p */,
    ctype_base::mask(alpha | print | lower) /* q */,
    ctype_base::mask(alpha | print | lower) /* r */,
    ctype_base::mask(alpha | print | lower) /* s */,
    ctype_base::mask(alpha | print | lower) /* t */,
    ctype_base::mask(alpha | print | lower) /* u */,
    ctype_base::mask(alpha | print | lower) /* v */,
    ctype_base::mask(alpha | print | lower) /* w */,
    ctype_base::mask(alpha | print | lower) /* x */,
    ctype_base::mask(alpha | print | lower) /* y */,
    ctype_base::mask(alpha | print | lower) /* z */,
    ctype_base::mask(punct | print) /* { */,
    ctype_base::mask(punct | print) /* | */,
    ctype_base::mask(punct | print) /* } */,
    ctype_base::mask(punct | print) /* ~ */,
    cntrl /* del (0x7f)*/,
    /* ASCII is a 7-bit code, so everything else is non-ASCII */
    ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0),  ctype_base::mask(0),
    ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0),  ctype_base::mask(0),
    ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0),  ctype_base::mask(0),
    ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0),  ctype_base::mask(0),
    ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0),  ctype_base::mask(0),
    ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0),  ctype_base::mask(0),
    ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0),  ctype_base::mask(0),
    ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0),  ctype_base::mask(0),
    ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0),  ctype_base::mask(0),
    ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0),  ctype_base::mask(0),
    ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0),  ctype_base::mask(0),
    ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0),  ctype_base::mask(0),
    ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0),  ctype_base::mask(0),
    ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0),  ctype_base::mask(0),
    ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0),  ctype_base::mask(0),
    ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0), ctype_base::mask(0),  ctype_base::mask(0)
  };
  return _S_classic_table;
}

// For every c in the range 0 <= c < 256, _S_upper[c] is the
// uppercased version of c and _S_lower[c] is the lowercased
// version.  As before, these two tables assume the ASCII character
// set.

const unsigned char _S_upper[ctype<char>::table_size] =
{
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
  0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
  0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
  0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
  0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
  0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
  0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
  0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
  0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
  0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
  0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
  0x58, 0x59, 0x5a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
  0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
  0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
  0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
  0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
  0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
  0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
  0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
  0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
  0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
  0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
  0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
  0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
  0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
  0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
  0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

const unsigned char _S_lower[ctype<char>::table_size] =
{
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
  0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
  0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
  0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
  0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
  0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
  0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
  0x78, 0x79, 0x7a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
  0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
  0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
  0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
  0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
  0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
  0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
  0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
  0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
  0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
  0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
  0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
  0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
  0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
  0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
  0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
  0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
  0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
  0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
  0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

//An helper struct to check wchar_t index without generating warnings
//under some compilers (gcc) because of a limited range of value
//(when wchar_t is unsigned)
template <bool _IsSigned>
struct _WCharIndexT;

#if !(defined (__BORLANDC__) && !defined(__linux__)) && \
    !(defined (__GNUC__) && (defined (__MINGW32__) || defined (__CYGWIN__))) && \
    !defined (__ICL)
_STLP_TEMPLATE_NULL
struct _WCharIndexT<true> {
  static bool in_range(wchar_t c, size_t upperBound) {
    return c >= 0 && size_t(c) < upperBound;
  }
};
#endif

_STLP_TEMPLATE_NULL
struct _WCharIndexT<false> {
  static bool in_range(wchar_t c, size_t upperBound) {
    return size_t(c) < upperBound;
  }
};

typedef _WCharIndexT<wchar_t(-1) < 0> _WCharIndex;

// Some helper functions used in ctype<>::scan_is and scan_is_not.

struct _Ctype_is_mask : public unary_function<char, bool> {
  ctype_base::mask _Mask;
  const ctype_base::mask* _M_table;

  _Ctype_is_mask(ctype_base::mask __m, const ctype_base::mask* __t) : _Mask(__m), _M_table(__t) {}
  bool operator()(char __c) const { return (_M_table[(unsigned char) __c] & _Mask) != 0; }
};

struct _Ctype_not_mask : public unary_function<char, bool> {
  ctype_base::mask _Mask;
  const ctype_base::mask* _M_table;

  _Ctype_not_mask(ctype_base::mask __m, const ctype_base::mask* __t) : _Mask(__m), _M_table(__t) {}
  bool operator()(char __c) const { return (_M_table[(unsigned char) __c] & _Mask) == 0; }
};

ctype<char>::ctype(const ctype_base::mask * __tab, bool __del, size_t __refs) :
  locale::facet(__refs),
  _M_ctype_table(__tab ? __tab : classic_table()),
  _M_delete(__tab && __del)
{}

ctype<char>::~ctype() {
  if (_M_delete)
    delete[] __CONST_CAST(ctype_base::mask *, _M_ctype_table);
}

const char*
#if defined (__DMC__)
_STLP_DECLSPEC
#endif
ctype<char>::scan_is(ctype_base::mask  __m, const char* __low, const char* __high) const
{ return _STLP_STD::find_if(__low, __high, _Ctype_is_mask(__m, _M_ctype_table)); }

const char*
#if defined (__DMC__)
_STLP_DECLSPEC
#endif
ctype<char>::scan_not(ctype_base::mask  __m, const char* __low, const char* __high) const
{ return _STLP_STD::find_if(__low, __high, _Ctype_not_mask(__m, _M_ctype_table)); }

char ctype<char>::do_toupper(char __c) const
{ return (char) _S_upper[(unsigned char) __c]; }
char ctype<char>::do_tolower(char __c) const
{ return (char) _S_lower[(unsigned char) __c]; }

const char* ctype<char>::do_toupper(char* __low, const char* __high) const {
  for ( ; __low < __high; ++__low)
    *__low = (char) _S_upper[(unsigned char) *__low];
  return __high;
}
const char* ctype<char>::do_tolower(char* __low, const char* __high) const {
  for ( ; __low < __high; ++__low)
    *__low = (char) _S_lower[(unsigned char) *__low];
  return __high;
}

char
ctype<char>::do_widen(char __c) const { return __c; }

const char*
ctype<char>::do_widen(const char* __low, const char* __high,
                      char* __to) const {
  _STLP_PRIV __copy_trivial(__low, __high, __to);
  return __high;
}
char
ctype<char>::do_narrow(char __c, char /* dfault */ ) const { return __c; }
const char*
ctype<char>::do_narrow(const char* __low, const char* __high,
                       char /* dfault */, char* __to) const {
  _STLP_PRIV __copy_trivial(__low, __high, __to);
  return __high;
}


#if !defined (_STLP_NO_WCHAR_T)

struct _Ctype_w_is_mask : public unary_function<wchar_t, bool> {
  ctype_base::mask M;
  const ctype_base::mask* table;

  _Ctype_w_is_mask(ctype_base::mask m, const ctype_base::mask* t)
    : M(m), table(t) {}
  bool operator()(wchar_t c) const
  { return _WCharIndex::in_range(c, ctype<char>::table_size) && (table[c] & M); }
};

//----------------------------------------------------------------------
// ctype<wchar_t>

ctype<wchar_t>::~ctype() {}


bool ctype<wchar_t>::do_is(ctype_base::mask  m, wchar_t c) const {
  const ctype_base::mask * table = ctype<char>::classic_table();
  return _WCharIndex::in_range(c, ctype<char>::table_size) && (m & table[c]);
}

const wchar_t* ctype<wchar_t>::do_is(const wchar_t* low, const wchar_t* high,
                                     ctype_base::mask * vec) const {
  // boris : not clear if this is the right thing to do...
  const ctype_base::mask * table = ctype<char>::classic_table();
  wchar_t c;
  for ( ; low < high; ++low, ++vec) {
    c = *low;
    *vec = _WCharIndex::in_range(c, ctype<char>::table_size) ? table[c] : ctype_base::mask(0);
  }
  return high;
}

const wchar_t*
ctype<wchar_t>::do_scan_is(ctype_base::mask  m,
                           const wchar_t* low, const wchar_t* high) const {
  return find_if(low, high, _Ctype_w_is_mask(m, ctype<char>::classic_table()));
}


const wchar_t*
ctype<wchar_t>::do_scan_not(ctype_base::mask  m,
                            const wchar_t* low, const wchar_t* high) const {
  return find_if(low, high, not1(_Ctype_w_is_mask(m, ctype<char>::classic_table())));
}

wchar_t ctype<wchar_t>::do_toupper(wchar_t c) const {
  return _WCharIndex::in_range(c, ctype<char>::table_size) ? (wchar_t)_S_upper[c]
                                                           : c;
}

const wchar_t*
ctype<wchar_t>::do_toupper(wchar_t* low, const wchar_t* high) const {
  for ( ; low < high; ++low) {
    wchar_t c = *low;
    *low = _WCharIndex::in_range(c, ctype<char>::table_size) ? (wchar_t)_S_upper[c]
                                                             : c;
  }
  return high;
}

wchar_t ctype<wchar_t>::do_tolower(wchar_t c) const {
  return _WCharIndex::in_range(c, ctype<char>::table_size) ? (wchar_t)_S_lower[c]
                                                           : c;
}

const wchar_t*
ctype<wchar_t>::do_tolower(wchar_t* low, const wchar_t* high) const {
  for ( ; low < high; ++low) {
    wchar_t c = *low;
    *low = _WCharIndex::in_range(c, ctype<char>::table_size) ? (wchar_t)_S_lower[c]
                                                             : c;
  }
  return high;
}

wchar_t ctype<wchar_t>::do_widen(char c) const {
  return (wchar_t)(unsigned char)c;
}

const char*
ctype<wchar_t>::do_widen(const char* low, const char* high,
                         wchar_t* dest) const {
  while (low != high)
    *dest++ = (wchar_t)(unsigned char)*low++;
  return high;
}

char ctype<wchar_t>::do_narrow(wchar_t c, char dfault) const
{ return (unsigned char)c == c ? (char)c : dfault; }

const wchar_t* ctype<wchar_t>::do_narrow(const wchar_t* low,
                                         const wchar_t* high,
                                         char dfault, char* dest) const {
  while (low != high) {
    wchar_t c = *low++;
    *dest++ = (unsigned char)c == c ? (char)c : dfault;
  }

  return high;
}

# endif
_STLP_END_NAMESPACE

// Local Variables:
// mode:C++
// End:

