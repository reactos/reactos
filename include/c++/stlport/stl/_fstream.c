/*
 * Copyright (c) 1996,1997
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
#ifndef _STLP_FSTREAM_C
#define _STLP_FSTREAM_C

#ifndef _STLP_INTERNAL_FSTREAM_H
#  include <stl/_fstream.h>
#endif

#ifndef _STLP_INTERNAL_LIMITS
#  include <stl/_limits.h>
#endif

_STLP_BEGIN_NAMESPACE

# if defined ( _STLP_NESTED_TYPE_PARAM_BUG )
// no wchar_t is supported for this mode
# define __BF_int_type__ int
# define __BF_pos_type__ streampos
# define __BF_off_type__ streamoff
# else
# define __BF_int_type__ _STLP_TYPENAME_ON_RETURN_TYPE basic_filebuf<_CharT, _Traits>::int_type
# define __BF_pos_type__ _STLP_TYPENAME_ON_RETURN_TYPE basic_filebuf<_CharT, _Traits>::pos_type
# define __BF_off_type__ _STLP_TYPENAME_ON_RETURN_TYPE basic_filebuf<_CharT, _Traits>::off_type
# endif


//----------------------------------------------------------------------
// Public basic_filebuf<> member functions

template <class _CharT, class _Traits>
basic_filebuf<_CharT, _Traits>::basic_filebuf()
     :  basic_streambuf<_CharT, _Traits>(), _M_base(),
    _M_constant_width(false), _M_always_noconv(false),
    _M_int_buf_dynamic(false),
    _M_in_input_mode(false), _M_in_output_mode(false),
    _M_in_error_mode(false), _M_in_putback_mode(false),
    _M_int_buf(0), _M_int_buf_EOS(0),
    _M_ext_buf(0), _M_ext_buf_EOS(0),
    _M_ext_buf_converted(0), _M_ext_buf_end(0),
    _M_state(_STLP_DEFAULT_CONSTRUCTED(_State_type)),
    _M_end_state(_STLP_DEFAULT_CONSTRUCTED(_State_type)),
    _M_mmap_base(0), _M_mmap_len(0),
    _M_saved_eback(0), _M_saved_gptr(0), _M_saved_egptr(0),
    _M_codecvt(0),
    _M_width(1), _M_max_width(1)
{
  this->_M_setup_codecvt(locale(), false);
}

template <class _CharT, class _Traits>
basic_filebuf<_CharT, _Traits>::~basic_filebuf() {
  this->close();
  _M_deallocate_buffers();
}


template <class _CharT, class _Traits>
_STLP_TYPENAME_ON_RETURN_TYPE basic_filebuf<_CharT, _Traits>::int_type
basic_filebuf<_CharT, _Traits>::underflow() {
  return _Underflow<_CharT, _Traits>::_M_doit(this);
}

template <class _CharT, class _Traits>
basic_filebuf<_CharT, _Traits>*
basic_filebuf<_CharT, _Traits>::close() {
  bool __ok = this->is_open();

  if (_M_in_output_mode) {
    __ok = __ok && !_Traits::eq_int_type(this->overflow(traits_type::eof()),
                                         traits_type::eof());
    __ok = __ok && this->_M_unshift();
  }
  else if (_M_in_input_mode)
      this->_M_exit_input_mode();

  // Note order of arguments.  We close the file even if __ok is false.
  __ok = _M_base._M_close() && __ok;

  // Restore the initial state, except that we don't deallocate the buffer
  // or mess with the cached codecvt information.
  _M_state = _M_end_state = _State_type();
  _M_ext_buf_converted = _M_ext_buf_end = 0;

  _M_mmap_base = 0;
  _M_mmap_len = 0;

  this->setg(0, 0, 0);
  this->setp(0, 0);

  _M_saved_eback = _M_saved_gptr = _M_saved_egptr = 0;

  _M_in_input_mode = _M_in_output_mode = _M_in_error_mode = _M_in_putback_mode
    = false;

  return __ok ? this : 0;
}

// This member function is called whenever we exit input mode.
// It unmaps the memory-mapped file, if any, and sets
// _M_in_input_mode to false.
template <class _CharT, class _Traits>
void basic_filebuf<_CharT, _Traits>::_M_exit_input_mode() {
  if (_M_mmap_base != 0) {
    _M_base._M_unmap(_M_mmap_base, _M_mmap_len);
    _M_mmap_base = 0;
    _M_mmap_len = 0;
  }
  _M_in_input_mode = false;
}


//----------------------------------------------------------------------
// basic_filebuf<> overridden protected virtual member functions

template <class _CharT, class _Traits>
streamsize basic_filebuf<_CharT, _Traits>::showmanyc() {
  // Is there any possibility that reads can succeed?
  if (!this->is_open() || _M_in_output_mode || _M_in_error_mode)
    return -1;
  else if (_M_in_putback_mode)
    return this->egptr() - this->gptr();
  else if (_M_constant_width) {
    streamoff __pos  = _M_base._M_seek(0, ios_base::cur);
    streamoff __size = _M_base._M_file_size();
    return __pos >= 0 && __size > __pos ? __size - __pos : 0;
  }
  else
    return 0;
}


// Make a putback position available, if necessary, by switching to a
// special internal buffer used only for putback.  The buffer is
// [_M_pback_buf, _M_pback_buf + _S_pback_buf_size), but the base
// class only sees a piece of it at a time.  (We want to make sure
// that we don't try to read a character that hasn't been initialized.)
// The end of the putback buffer is always _M_pback_buf + _S_pback_buf_size,
// but the beginning is usually not _M_pback_buf.
template <class _CharT, class _Traits>
__BF_int_type__
basic_filebuf<_CharT, _Traits>::pbackfail(int_type __c) {
  const int_type __eof = traits_type::eof();

  // If we aren't already in input mode, pushback is impossible.
  if (!_M_in_input_mode)
    return __eof;

  // We can use the ordinary get buffer if there's enough space, and
  // if it's a buffer that we're allowed to write to.
  if (this->gptr() != this->eback() &&
      (traits_type::eq_int_type(__c, __eof) ||
       traits_type::eq(traits_type::to_char_type(__c), this->gptr()[-1]) ||
       !_M_mmap_base)) {
    this->gbump(-1);
    if (traits_type::eq_int_type(__c, __eof) ||
        traits_type::eq(traits_type::to_char_type(__c), *this->gptr()))
      return traits_type::to_int_type(*this->gptr());
  }
  else if (!traits_type::eq_int_type(__c, __eof)) {
    // Are we in the putback buffer already?
    _CharT* __pback_end = _M_pback_buf + __STATIC_CAST(int,_S_pback_buf_size);
    if (_M_in_putback_mode) {
      // Do we have more room in the putback buffer?
      if (this->eback() != _M_pback_buf)
        this->setg(this->egptr() - 1, this->egptr() - 1, __pback_end);
      else
        return __eof;           // No more room in the buffer, so fail.
    }
    else {                      // We're not yet in the putback buffer.
      _M_saved_eback = this->eback();
      _M_saved_gptr  = this->gptr();
      _M_saved_egptr = this->egptr();
      this->setg(__pback_end - 1, __pback_end - 1, __pback_end);
      _M_in_putback_mode = true;
    }
  }
  else
    return __eof;

  // We have made a putback position available.  Assign to it, and return.
  *this->gptr() = traits_type::to_char_type(__c);
  return __c;
}

// This member function flushes the put area, and also outputs the
// character __c (unless __c is eof).  Invariant: we always leave room
// in the internal buffer for one character more than the base class knows
// about.  We see the internal buffer as [_M_int_buf, _M_int_buf_EOS), but
// the base class only sees [_M_int_buf, _M_int_buf_EOS - 1).
template <class _CharT, class _Traits>
__BF_int_type__
basic_filebuf<_CharT, _Traits>::overflow(int_type __c) {
  // Switch to output mode, if necessary.
  if (!_M_in_output_mode)
    if (!_M_switch_to_output_mode())
      return traits_type::eof();

  _CharT* __ibegin = this->_M_int_buf;
  _CharT* __iend   = this->pptr();
  this->setp(_M_int_buf, _M_int_buf_EOS - 1);

  // Put __c at the end of the internal buffer.
  if (!traits_type::eq_int_type(__c, traits_type::eof()))
    *__iend++ = _Traits::to_char_type(__c);

  // For variable-width encodings, output may take more than one pass.
  while (__ibegin != __iend) {
    const _CharT* __inext = __ibegin;
    char* __enext         = _M_ext_buf;
    typename _Codecvt::result __status
      = _M_codecvt->out(_M_state, __ibegin, __iend, __inext,
                        _M_ext_buf, _M_ext_buf_EOS, __enext);
    if (__status == _Codecvt::noconv) {
      return _Noconv_output<_Traits>::_M_doit(this, __ibegin, __iend)
        ? traits_type::not_eof(__c)
        : _M_output_error();
    }

    // For a constant-width encoding we know that the external buffer
    // is large enough, so failure to consume the entire internal buffer
    // or to produce the correct number of external characters, is an error.
    // For a variable-width encoding, however, we require only that we
    // consume at least one internal character
    else if (__status != _Codecvt::error &&
             (((__inext == __iend) &&
               (__enext - _M_ext_buf == _M_width * (__iend - __ibegin))) ||
              (!_M_constant_width && __inext != __ibegin))) {
        // We successfully converted part or all of the internal buffer.
      ptrdiff_t __n = __enext - _M_ext_buf;
      if (_M_write(_M_ext_buf, __n))
        __ibegin += __inext - __ibegin;
      else
        return _M_output_error();
    }
    else
      return _M_output_error();
  }

  return traits_type::not_eof(__c);
}

// This member function must be called before any I/O has been
// performed on the stream, otherwise it has no effect.
//
// __buf == 0 && __n == 0 means to make this stream unbuffered.
// __buf != 0 && __n > 0 means to use __buf as the stream's internal
// buffer, rather than the buffer that would otherwise be allocated
// automatically.  __buf must be a pointer to an array of _CharT whose
// size is at least __n.
template <class _CharT, class _Traits>
basic_streambuf<_CharT, _Traits>*
basic_filebuf<_CharT, _Traits>::setbuf(_CharT* __buf, streamsize __n) {
  if (!_M_in_input_mode &&! _M_in_output_mode && !_M_in_error_mode &&
      _M_int_buf == 0) {
    if (__buf == 0 && __n == 0)
      _M_allocate_buffers(0, 1);
    else if (__buf != 0 && __n > 0)
      _M_allocate_buffers(__buf, __n);
  }
  return this;
}

#if defined (_STLP_ASSERTIONS)
// helper class.
template <class _CharT>
struct _Filebuf_Tmp_Buf {
  _CharT* _M_ptr;
  _Filebuf_Tmp_Buf(ptrdiff_t __n) : _M_ptr(0) { _M_ptr = new _CharT[__n]; }
  ~_Filebuf_Tmp_Buf() { delete[] _M_ptr; }
};
#endif

template <class _CharT, class _Traits>
__BF_pos_type__
basic_filebuf<_CharT, _Traits>::seekoff(off_type __off,
                                        ios_base::seekdir __whence,
                                        ios_base::openmode /* dummy */) {
  if (!this->is_open())
    return pos_type(-1);

  if (!_M_constant_width && __off != 0)
    return pos_type(-1);

  if (!_M_seek_init(__off != 0 || __whence != ios_base::cur))
    return pos_type(-1);

  // Seek to beginning or end, regardless of whether we're in input mode.
  if (__whence == ios_base::beg || __whence == ios_base::end)
    return _M_seek_return(_M_base._M_seek(_M_width * __off, __whence),
                          _State_type());

  // Seek relative to current position.  Complicated if we're in input mode.
  _STLP_ASSERT(__whence == ios_base::cur)
  if (!_M_in_input_mode)
    return _M_seek_return(_M_base._M_seek(_M_width * __off, __whence),
                          _State_type());

  if (_M_mmap_base != 0) {
    // __off is relative to gptr().  We need to do a bit of arithmetic
    // to get an offset relative to the external file pointer.
    streamoff __adjust = _M_mmap_len - (this->gptr() - (_CharT*) _M_mmap_base);

    // if __off == 0, we do not need to exit input mode and to shift file pointer
    return __off == 0 ? pos_type(_M_base._M_seek(0, ios_base::cur) - __adjust)
                      : _M_seek_return(_M_base._M_seek(__off - __adjust, ios_base::cur), _State_type());
  }

  if (_M_constant_width) { // Get or set the position.
    streamoff __iadj = _M_width * (this->gptr() - this->eback());

    // Compensate for offset relative to gptr versus offset relative
    // to external pointer.  For a text-oriented stream, where the
    // compensation is more than just pointer arithmetic, we may get
    // but not set the current position.

    if (__iadj <= _M_ext_buf_end - _M_ext_buf) {
      streamoff __eadj =  _M_base._M_get_offset(_M_ext_buf + __STATIC_CAST(ptrdiff_t, __iadj), _M_ext_buf_end);

      return __off == 0 ? pos_type(_M_base._M_seek(0, ios_base::cur) - __eadj)
                        : _M_seek_return(_M_base._M_seek(__off - __eadj, ios_base::cur), _State_type());
    }
  }
  else {                    // Get the position.  Encoding is var width.
    // Get position in internal buffer.
    ptrdiff_t __ipos = this->gptr() - this->eback();

    // Get corresponding position in external buffer.
    _State_type __state = _M_state;
    int __epos = _M_codecvt->length(__state, _M_ext_buf, _M_ext_buf_converted,
                                    __ipos);
#if defined (_STLP_ASSERTIONS)
    // Sanity check (expensive): make sure __epos is the right answer.
    _STLP_ASSERT(__epos >= 0)
    _State_type __tmp_state = _M_state;
    _Filebuf_Tmp_Buf<_CharT> __buf(__ipos);
    _CharT* __ibegin = __buf._M_ptr;
    _CharT* __inext  = __ibegin;
    const char* __dummy;
    typename _Codecvt::result __status
      = _M_codecvt->in(__tmp_state,
                       _M_ext_buf, _M_ext_buf + __epos, __dummy,
                       __ibegin, __ibegin + __ipos, __inext);
    // The result code is necessarily ok because:
    // - noconv: impossible for a variable encoding
    // - error: length method is supposed to count until it reach max value or find an error so we cannot have
    //          an error again when decoding an external buffer up to length return value.
    // - partial: idem error, it is also a reason for length to stop counting.
    _STLP_ASSERT(__status == _Codecvt::ok)
    _STLP_ASSERT(__inext == __ibegin + __ipos)
    _STLP_ASSERT(equal(this->eback(), this->gptr(), __ibegin, _STLP_PRIV _Eq_traits<traits_type>()))
#endif
    // Get the current position (at the end of the external buffer),
    // then adjust it.  Again, it might be a text-oriented stream.
    streamoff __cur = _M_base._M_seek(0, ios_base::cur);
    streamoff __adj = _M_base._M_get_offset(_M_ext_buf, _M_ext_buf + __epos) -
                      _M_base._M_get_offset(_M_ext_buf, _M_ext_buf_end);
    if (__cur != -1 && __cur + __adj >= 0)
      return __off == 0 ? pos_type(__cur + __adj)
                        : _M_seek_return(__cur + __adj, __state);
  }

  return pos_type(-1);
}


template <class _CharT, class _Traits>
__BF_pos_type__
basic_filebuf<_CharT, _Traits>::seekpos(pos_type __pos,
                                        ios_base::openmode /* dummy */) {
  if (this->is_open()) {
    if (!_M_seek_init(true))
      return pos_type(-1);

    streamoff __off = off_type(__pos);
    if (__off != -1 && _M_base._M_seek(__off, ios_base::beg) != -1) {
      _M_state = __pos.state();
      return _M_seek_return(__off, __pos.state());
    }
  }

  return pos_type(-1);
}


template <class _CharT, class _Traits>
int basic_filebuf<_CharT, _Traits>::sync() {
  if (_M_in_output_mode)
    return traits_type::eq_int_type(this->overflow(traits_type::eof()),
                                    traits_type::eof()) ? -1 : 0;
  return 0;
}


// Change the filebuf's locale.  This member function has no effect
// unless it is called before any I/O is performed on the stream.
template <class _CharT, class _Traits>
void basic_filebuf<_CharT, _Traits>::imbue(const locale& __loc) {
  if (!_M_in_input_mode && !_M_in_output_mode && !_M_in_error_mode) {
    this->_M_setup_codecvt(__loc);
  }
}

//----------------------------------------------------------------------
// basic_filebuf<> helper functions.

//----------------------------------------
// Helper functions for switching between modes.

// This member function is called if we're performing the first I/O
// operation on a filebuf, or if we're performing an input operation
// immediately after a seek.
template <class _CharT, class _Traits>
bool basic_filebuf<_CharT, _Traits>::_M_switch_to_input_mode() {
  if (this->is_open() && (((int)_M_base.__o_mode() & (int)ios_base::in) !=0)
      && (_M_in_output_mode == 0) && (_M_in_error_mode == 0)) {
    if (!_M_int_buf && !_M_allocate_buffers())
      return false;

    _M_ext_buf_converted = _M_ext_buf;
    _M_ext_buf_end       = _M_ext_buf;

    _M_end_state    = _M_state;

    _M_in_input_mode = true;
    return true;
  }

  return false;
}


// This member function is called if we're performing the first I/O
// operation on a filebuf, or if we're performing an output operation
// immediately after a seek.
template <class _CharT, class _Traits>
bool basic_filebuf<_CharT, _Traits>::_M_switch_to_output_mode() {
  if (this->is_open() && (_M_base.__o_mode() & (int)ios_base::out) &&
      _M_in_input_mode == 0 && _M_in_error_mode == 0) {

    if (!_M_int_buf && !_M_allocate_buffers())
      return false;

    // In append mode, every write does an implicit seek to the end
    // of the file.  Whenever leaving output mode, the end of file
    // get put in the initial shift state.
    if (_M_base.__o_mode() & ios_base::app)
      _M_state = _State_type();

    this->setp(_M_int_buf, _M_int_buf_EOS - 1);
    _M_in_output_mode = true;
    return true;
  }

  return false;
}


//----------------------------------------
// Helper functions for input

// This member function is called if there is an error during input.
// It puts the filebuf in error mode, clear the get area buffer, and
// returns eof.
// returns eof.  Error mode is sticky; it is cleared only by close or
// seek.

template <class _CharT, class _Traits>
__BF_int_type__
basic_filebuf<_CharT, _Traits>::_M_input_error() {
   this->_M_exit_input_mode();
  _M_in_output_mode = false;
  _M_in_error_mode = true;
  this->setg(0, 0, 0);
  return traits_type::eof();
}

template <class _CharT, class _Traits>
__BF_int_type__
basic_filebuf<_CharT, _Traits>::_M_underflow_aux() {
  // We have the state and file position from the end of the internal
  // buffer.  This round, they become the beginning of the internal buffer.
  _M_state    = _M_end_state;

  // Fill the external buffer.  Start with any leftover characters that
  // didn't get converted last time.
  if (_M_ext_buf_end > _M_ext_buf_converted)

    _M_ext_buf_end = _STLP_STD::copy(_M_ext_buf_converted, _M_ext_buf_end, _M_ext_buf);
    // boris : copy_backward did not work
    //_M_ext_buf_end = copy_backward(_M_ext_buf_converted, _M_ext_buf_end,
    //_M_ext_buf+ (_M_ext_buf_end - _M_ext_buf_converted));
  else
    _M_ext_buf_end = _M_ext_buf;

  // Now fill the external buffer with characters from the file.  This is
  // a loop because occasionally we don't get enough external characters
  // to make progress.
  for (;;) {
    ptrdiff_t __n = _M_base._M_read(_M_ext_buf_end, _M_ext_buf_EOS - _M_ext_buf_end);
    if (__n < 0) {
      // Read failed, maybe we should set err bit on associated stream...
      this->setg(0, 0, 0);
      return traits_type::eof();
    }

    _M_ext_buf_end += __n;

    // If external buffer is empty there is nothing to do. 
    if (_M_ext_buf == _M_ext_buf_end) {
      this->setg(0, 0, 0);
      return traits_type::eof();
    }

    // Convert the external buffer to internal characters.
    const char* __enext;
    _CharT* __inext;

    typename _Codecvt::result __status
      = _M_codecvt->in(_M_end_state,
                       _M_ext_buf, _M_ext_buf_end, __enext,
                       _M_int_buf, _M_int_buf_EOS, __inext);

    /* Error conditions:
     * (1) Return value of error.
     * (2) Producing internal characters without consuming external characters.
     * (3) In fixed-width encodings, producing an internal sequence whose length
     * is inconsistent with that of the internal sequence.
     * (4) Failure to produce any characters if we have enough characters in
     * the external buffer, where "enough" means the largest possible width
     * of a single character. */
    if (__status == _Codecvt::noconv)
      return _Noconv_input<_Traits>::_M_doit(this);
    else if (__status == _Codecvt::error ||
            (__inext != _M_int_buf && __enext == _M_ext_buf) ||
            (_M_constant_width && (__inext - _M_int_buf) *  _M_width != (__enext - _M_ext_buf)) ||
            (__inext == _M_int_buf && __enext - _M_ext_buf >= _M_max_width))
      return _M_input_error();
    else if (__inext != _M_int_buf) {
      _M_ext_buf_converted = _M_ext_buf + (__enext - _M_ext_buf);
      this->setg(_M_int_buf, _M_int_buf, __inext);
      return traits_type::to_int_type(*_M_int_buf);
    }
    /* We need to go around the loop again to get more external characters.
     * But if the previous read failed then don't try again for now.
     * Don't enter error mode for a failed read. Error mode is sticky,
     * and we might succeed if we try again. */
    if (__n <= 0) {
      this->setg(0, 0, 0);
      return traits_type::eof();
    }
  }
}

//----------------------------------------
// Helper functions for output

// This member function is called if there is an error during output.
// It puts the filebuf in error mode, clear the put area buffer, and
// returns eof.  Error mode is sticky; it is cleared only by close or
// seek.
template <class _CharT, class _Traits>
__BF_int_type__
basic_filebuf<_CharT, _Traits>::_M_output_error() {
  _M_in_output_mode = false;
  _M_in_input_mode = false;
  _M_in_error_mode = true;
  this->setp(0, 0);
  return traits_type::eof();
}


// Write whatever sequence of characters is necessary to get back to
// the initial shift state.  This function overwrites the external
// buffer, changes the external file position, and changes the state.
// Precondition: the internal buffer is empty.
template <class _CharT, class _Traits>
bool basic_filebuf<_CharT, _Traits>::_M_unshift() {
  if (_M_in_output_mode && !_M_constant_width) {
    typename _Codecvt::result __status;
    do {
      char* __enext = _M_ext_buf;
      __status = _M_codecvt->unshift(_M_state,
                                     _M_ext_buf, _M_ext_buf_EOS, __enext);
      if (__status == _Codecvt::noconv ||
          (__enext == _M_ext_buf && __status == _Codecvt::ok))
        return true;
      else if (__status == _Codecvt::error)
        return false;
      else if (!_M_write(_M_ext_buf, __enext - _M_ext_buf))
        return false;
    } while (__status == _Codecvt::partial);
  }

  return true;
}


//----------------------------------------
// Helper functions for buffer allocation and deallocation

// This member function is called when we're initializing a filebuf's
// internal and external buffers.  The argument is the size of the
// internal buffer; the external buffer is sized using the character
// width in the current encoding.  Preconditions: the buffers are currently
// null.  __n >= 1.  __buf is either a null pointer or a pointer to an
// array show size is at least __n.

// We need __n >= 1 for two different reasons.  For input, the base
// class always needs a buffer because of the semantics of underflow().
// For output, we want to have an internal buffer that's larger by one
// element than the buffer that the base class knows about.  (See
// basic_filebuf<>::overflow() for the reason.)
template <class _CharT, class _Traits>
bool basic_filebuf<_CharT, _Traits>::_M_allocate_buffers(_CharT* __buf, streamsize __n) {
  //The major hypothesis in the following implementation is that size_t is unsigned.
  //We also need streamsize byte representation to be larger or equal to the int
  //representation to correctly store the encoding information.
  _STLP_STATIC_ASSERT(!numeric_limits<size_t>::is_signed &&
                      sizeof(streamsize) >= sizeof(int))

  if (__buf == 0) {
    streamsize __bufsize = __n * sizeof(_CharT);
    //We first check that the streamsize representation can't overflow a size_t one.
    //If it can, we check that __bufsize is not higher than the size_t max value.
    if ((sizeof(streamsize) > sizeof(size_t)) &&
        (__bufsize > __STATIC_CAST(streamsize, (numeric_limits<size_t>::max)())))
      return false;
    _M_int_buf = __STATIC_CAST(_CharT*, malloc(__STATIC_CAST(size_t, __bufsize)));
    if (!_M_int_buf)
      return false;
    _M_int_buf_dynamic = true;
  }
  else {
    _M_int_buf = __buf;
    _M_int_buf_dynamic = false;
  }

  streamsize __ebufsiz = (max)(__n * __STATIC_CAST(streamsize, _M_width),
                               __STATIC_CAST(streamsize, _M_codecvt->max_length()));

  _M_ext_buf = 0;
  if ((sizeof(streamsize) < sizeof(size_t)) ||
      ((sizeof(streamsize) == sizeof(size_t)) && numeric_limits<streamsize>::is_signed) ||
      (__ebufsiz <= __STATIC_CAST(streamsize, (numeric_limits<size_t>::max)()))) {
    _M_ext_buf = __STATIC_CAST(char*, malloc(__STATIC_CAST(size_t, __ebufsiz)));
  }

  if (!_M_ext_buf) {
    _M_deallocate_buffers();
    return false;
  }

  _M_int_buf_EOS = _M_int_buf + __STATIC_CAST(ptrdiff_t, __n);
  _M_ext_buf_EOS = _M_ext_buf + __STATIC_CAST(ptrdiff_t, __ebufsiz);
  return true;
}

// Abbreviation for the most common case.
template <class _CharT, class _Traits>
bool basic_filebuf<_CharT, _Traits>::_M_allocate_buffers() {
  // Choose a buffer that's at least 4096 characters long and that's a
  // multiple of the page size.
  streamsize __default_bufsiz =
    ((_M_base.__page_size() + 4095UL) / _M_base.__page_size()) * _M_base.__page_size();
  return _M_allocate_buffers(0, __default_bufsiz);
}

template <class _CharT, class _Traits>
void basic_filebuf<_CharT, _Traits>::_M_deallocate_buffers() {
  if (_M_int_buf_dynamic)
    free(_M_int_buf);
  free(_M_ext_buf);
  _M_int_buf     = 0;
  _M_int_buf_EOS = 0;
  _M_ext_buf     = 0;
  _M_ext_buf_EOS = 0;
}


//----------------------------------------
// Helper functiosn for seek and imbue

template <class _CharT, class _Traits>
bool basic_filebuf<_CharT, _Traits>::_M_seek_init(bool __do_unshift) {
  // If we're in error mode, leave it.
   _M_in_error_mode = false;

  // Flush the output buffer if we're in output mode, and (conditionally)
  // emit an unshift sequence.
  if (_M_in_output_mode) {
    bool __ok = !traits_type::eq_int_type(this->overflow(traits_type::eof()),
                                          traits_type::eof());
    if (__do_unshift)
      __ok = __ok && this->_M_unshift();
    if (!__ok) {
      _M_in_output_mode = false;
      _M_in_error_mode = true;
      this->setp(0, 0);
      return false;
    }
  }

  // Discard putback characters, if any.
  if (_M_in_input_mode && _M_in_putback_mode)
    _M_exit_putback_mode();

  return true;
}


/* Change the filebuf's locale.  This member function has no effect
 * unless it is called before any I/O is performed on the stream.
 * This function is called on construction and on an imbue call. In the
 * case of the construction the codecvt facet might be a custom one if
 * the basic_filebuf user has instanciate it with a custom char_traits.
 * The user will have to call imbue before any I/O operation.
 */
template <class _CharT, class _Traits>
void basic_filebuf<_CharT, _Traits>::_M_setup_codecvt(const locale& __loc, bool __on_imbue) {
  if (has_facet<_Codecvt>(__loc)) {
    _M_codecvt = &use_facet<_Codecvt>(__loc) ;
    int __encoding    = _M_codecvt->encoding();

    _M_width          = (max)(__encoding, 1);
    _M_max_width      = _M_codecvt->max_length();
    _M_constant_width = __encoding > 0;
    _M_always_noconv  = _M_codecvt->always_noconv();
  }
  else {
    _M_codecvt = 0;
    _M_width = _M_max_width = 1;
    _M_constant_width = _M_always_noconv  = false;
    if (__on_imbue) {
      //This call will generate an exception reporting the problem.
      use_facet<_Codecvt>(__loc);
    }
  }
}

_STLP_END_NAMESPACE

# undef __BF_int_type__
# undef __BF_pos_type__
# undef __BF_off_type__

#endif /* _STLP_FSTREAM_C */

// Local Variables:
// mode:C++
// End:
