#ifndef _FULL_STREAM_H
#define _FULL_STREAM_H

#include <streambuf>

/*
 * This full_streambuf purpose is to act like a full disk to check the right behavior
 * of the STLport code in such a case.
 */

class full_streambuf : public std::streambuf {
public:
  typedef std::streambuf _Base;

  typedef _Base::int_type int_type;
  typedef _Base::traits_type traits_type;

  full_streambuf(size_t nb_output, bool do_throw = false)
    : _nb_output(nb_output), _do_throw(do_throw)
  {}

  std::string const& str() const
  { return _buf; }

protected:
  int_type overflow(int_type c) {
    if (_nb_output == 0) {
#if !defined (STLPORT) || defined (_STLP_USE_EXCEPTIONS)
      if (_do_throw) {
        throw "streambuf full";
      }
#endif
      return traits_type::eof();
    }
    --_nb_output;
    _buf += traits_type::to_char_type(c);
    return c;
  }

private:
  size_t _nb_output;
  bool _do_throw;
  std::string _buf;
};

#endif //_FULL_STREAM_H
