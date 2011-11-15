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
#include <ios>
#include <locale>
#include <ostream> // for __get_ostreambuf definition

#include "aligned_buffer.h"

_STLP_BEGIN_NAMESPACE

//----------------------------------------------------------------------
// ios_base members

// class ios_base::failure, a subclass of exception.  It's used solely
// for reporting errors.

ios_base::failure::failure(const string& s)
  : __Named_exception(s)
{}

ios_base::failure::~failure() _STLP_NOTHROW_INHERENTLY {}

#if !defined (_STLP_STATIC_CONST_INIT_BUG) && !defined (_STLP_NO_STATIC_CONST_DEFINITION)
// Definitions of ios_base's formatting flags.
const ios_base::fmtflags ios_base::left;
const ios_base::fmtflags ios_base::right;
const ios_base::fmtflags ios_base::internal;
const ios_base::fmtflags ios_base::dec;
const ios_base::fmtflags ios_base::hex;
const ios_base::fmtflags ios_base::oct;
const ios_base::fmtflags ios_base::fixed;
const ios_base::fmtflags ios_base::scientific;
const ios_base::fmtflags ios_base::boolalpha;
const ios_base::fmtflags ios_base::showbase;
const ios_base::fmtflags ios_base::showpoint;
const ios_base::fmtflags ios_base::showpos;
const ios_base::fmtflags ios_base::skipws;
const ios_base::fmtflags ios_base::unitbuf;
const ios_base::fmtflags ios_base::uppercase;
const ios_base::fmtflags ios_base::adjustfield;
const ios_base::fmtflags ios_base::basefield;
const ios_base::fmtflags ios_base::floatfield;

// Definitions of ios_base's state flags.
const ios_base::iostate ios_base::goodbit;
const ios_base::iostate ios_base::badbit;
const ios_base::iostate ios_base::eofbit;
const ios_base::iostate ios_base::failbit;

// Definitions of ios_base's openmode flags.
const ios_base::openmode ios_base::app;
const ios_base::openmode ios_base::ate;
const ios_base::openmode ios_base::binary;
const ios_base::openmode ios_base::in;
const ios_base::openmode ios_base::out;
const ios_base::openmode ios_base::trunc;

// Definitions of ios_base's seekdir flags.
const ios_base::seekdir ios_base::beg;
const ios_base::seekdir ios_base::cur;
const ios_base::seekdir ios_base::end;

#endif

// Internal functions used for managing exponentially-growing arrays of
// POD types.

// array is a pointer to N elements of type PODType.  Expands the array,
// if necessary, so that array[index] is meaningful.  All new elements are
// initialized to zero.  Returns a pointer to the new array, and the new
// size.

template <class PODType>
static pair<PODType*, size_t>
_Stl_expand_array(PODType* __array, size_t N, int index) {
  if ((int)N < index + 1) {
    size_t new_N = (max)(2 * N, size_t(index + 1));
    PODType* new_array
      = __STATIC_CAST(PODType*,realloc(__array, new_N * sizeof(PODType)));
    if (new_array) {
      fill(new_array + N, new_array + new_N, PODType());
      return pair<PODType*, size_t>(new_array, new_N);
    }
    else
      return pair<PODType*, size_t>(__STATIC_CAST(PODType*,0), 0);
  }
  else
    return pair<PODType*, size_t>(__array, N);
}

// array is a pointer to N elements of type PODType.  Allocate a new
// array of N elements, copying the values from the old array to the new.
// Return a pointer to the new array.  It is assumed that array is non-null
// and N is nonzero.
template <class PODType>
static PODType* _Stl_copy_array(const PODType* __array, size_t N) {
  PODType* result = __STATIC_CAST(PODType*,malloc(N * sizeof(PODType)));
  if (result)
    copy(__array, __array + N, result);
  return result;
}

locale ios_base::imbue(const locale& loc) {
  if (loc != _M_locale) {
    locale previous = _M_locale;
    _M_locale = loc;
    _M_invoke_callbacks(imbue_event);
    return previous;
  }
  else {
    _M_invoke_callbacks(imbue_event);
    return _M_locale;
  }
}

int _STLP_CALL ios_base::xalloc() {
#if defined (_STLP_THREADS) && \
    defined (_STLP_WIN32THREADS) && defined (_STLP_NEW_PLATFORM_SDK)
  static volatile __stl_atomic_t _S_index = 0;
  return _STLP_ATOMIC_INCREMENT(&_S_index);
#else
  static int _S_index = 0;
  static _STLP_STATIC_MUTEX __lock _STLP_MUTEX_INITIALIZER;
  _STLP_auto_lock sentry(__lock);
  return _S_index++;
#endif
}

long& ios_base::iword(int index) {
  static long dummy = 0;

  pair<long*, size_t> tmp = _Stl_expand_array(_M_iwords, _M_num_iwords, index);
  if (tmp.first) {              // The allocation, if any, succeeded.
    _M_iwords = tmp.first;
    _M_num_iwords = tmp.second;
    return _M_iwords[index];
  }
  else {
    _M_setstate_nothrow(badbit);
    _M_check_exception_mask();
    return dummy;
  }
}


void*& ios_base::pword(int index) {
  static void* dummy = 0;

  pair<void**, size_t> tmp = _Stl_expand_array(_M_pwords, _M_num_pwords, index);
  if (tmp.first) {              // The allocation, if any, succeeded.
    _M_pwords = tmp.first;
    _M_num_pwords = tmp.second;
    return _M_pwords[index];
  }
  else {
    _M_setstate_nothrow(badbit);
    _M_check_exception_mask();
    return dummy;
  }
}

void ios_base::register_callback(event_callback __fn, int index) {
  pair<pair<event_callback, int>*, size_t> tmp
    = _Stl_expand_array(_M_callbacks, _M_num_callbacks, (int)_M_callback_index /* fbp: index ??? */ );
  if (tmp.first) {
    _M_callbacks = tmp.first;
    _M_num_callbacks = tmp.second;
    _M_callbacks[_M_callback_index++] = make_pair(__fn, index);
  }
  else {
    _M_setstate_nothrow(badbit);
    _M_check_exception_mask();
  }
}

// Invokes all currently registered callbacks for a particular event.
// Behaves correctly even if one of the callbacks adds a new callback.
void ios_base::_M_invoke_callbacks(event E) {
  for (size_t i = _M_callback_index; i > 0; --i) {
    event_callback f = _M_callbacks[i-1].first;
    int n = _M_callbacks[i-1].second;
    f(E, *this, n);
  }
}

// This function is called if the state, rdstate(), has a bit set
// that is also set in the exception mask exceptions().
void ios_base::_M_throw_failure() {
  const char* arg ;
# if 0
  char buffer[256];
  char* ptr;
  strcpy(buffer, "ios failure: rdstate = 0x");
  ptr = __write_integer(buffer+strlen(buffer), ios_base::hex, __STATIC_CAST(unsigned long,_M_iostate));
  strcpy(ptr, " mask = 0x");
  ptr = __write_integer(buffer+strlen(buffer), ios_base::hex, __STATIC_CAST(unsigned long,_M_exception_mask));
  *ptr = 0;
  arg = buffer;
# else
  arg = "ios failure";
# endif

# ifndef _STLP_USE_EXCEPTIONS
  fputs(arg, stderr);
# else
  throw failure(arg);
# endif
}

// Copy x's state to *this.  This member function is used in the
// implementation of basic_ios::copyfmt.  Does not copy _M_exception_mask
// or _M_iostate.
void ios_base::_M_copy_state(const ios_base& x) {
  _M_fmtflags  = x._M_fmtflags; // Copy the flags, except for _M_iostate
  _M_openmode  = x._M_openmode; // and _M_exception_mask.
  _M_seekdir   = x._M_seekdir;
  _M_precision = x._M_precision;
  _M_width     = x._M_width;
  _M_locale    = x._M_locale;

  if (x._M_callbacks) {
    pair<event_callback, int>* tmp = _Stl_copy_array(x._M_callbacks, x._M_callback_index);
    if (tmp) {
      free(_M_callbacks);
      _M_callbacks = tmp;
      _M_num_callbacks = _M_callback_index = x._M_callback_index;
    }
    else {
      _M_setstate_nothrow(badbit);
      _M_check_exception_mask();
    }
  }

  if (x._M_iwords) {
    long* tmp = _Stl_copy_array(x._M_iwords, x._M_num_iwords);
    if (tmp) {
      free(_M_iwords);
      _M_iwords = tmp;
      _M_num_iwords = x._M_num_iwords;
    }
    else {
      _M_setstate_nothrow(badbit);
      _M_check_exception_mask();
    }
  }

  if (x._M_pwords) {
    void** tmp = _Stl_copy_array(x._M_pwords, x._M_num_pwords);
    if (tmp) {
      free(_M_pwords);
      _M_pwords = tmp;
      _M_num_pwords = x._M_num_pwords;
    }
    else {
      _M_setstate_nothrow(badbit);
      _M_check_exception_mask();
    }
  }
}

// ios's (protected) default constructor.  The standard says that all
// fields have indeterminate values; we initialize them to zero for
// simplicity.  The only thing that really matters is that the arrays
// are all initially null pointers, and the array element counts are all
// initially zero.
ios_base::ios_base()
  : _M_fmtflags(0), _M_iostate(0), _M_openmode(0), _M_seekdir(0),
    _M_exception_mask(0),
    _M_precision(0), _M_width(0),
    _M_locale(),
    _M_callbacks(0), _M_num_callbacks(0), _M_callback_index(0),
    _M_iwords(0), _M_num_iwords(0),
    _M_pwords(0),
    _M_num_pwords(0)
{}

// ios's destructor.
ios_base::~ios_base() {
  _M_invoke_callbacks(erase_event);
  free(_M_callbacks);
  free(_M_iwords);
  free(_M_pwords);
}

//----------------------------------------------------------------------
// Force instantiation of basic_ios
// For DLL exports, they are already instantiated.
#if !defined(_STLP_NO_FORCE_INSTANTIATE)
template class _STLP_CLASS_DECLSPEC basic_ios<char, char_traits<char> >;
#  if !defined (_STLP_NO_WCHAR_T)
template class _STLP_CLASS_DECLSPEC basic_ios<wchar_t, char_traits<wchar_t> >;
#  endif /* _STLP_NO_WCHAR_T */
#endif

_STLP_END_NAMESPACE

// Local Variables:
// mode:C++
// End:
