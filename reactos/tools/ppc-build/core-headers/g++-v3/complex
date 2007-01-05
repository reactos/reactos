// The template and inlines for the -*- C++ -*- complex number classes.

// Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005
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
// ISO C++ 14882: 26.2  Complex Numbers
// Note: this is not a conforming implementation.
// Initially implemented by Ulrich Drepper <drepper@cygnus.com>
// Improved by Gabriel Dos Reis <dosreis@cmla.ens-cachan.fr>
//

/** @file complex
 *  This is a Standard C++ Library header.  You should @c #include this header
 *  in your programs, rather than any of the "st[dl]_*.h" implementation files.
 */

#ifndef _GLIBCXX_COMPLEX
#define _GLIBCXX_COMPLEX 1

#pragma GCC system_header

#include <bits/c++config.h>
#include <bits/cpp_type_traits.h>
#include <cmath>
#include <sstream>

namespace std
{
  // Forward declarations
  template<typename _Tp> class complex;
  template<> class complex<float>;
  template<> class complex<double>;
  template<> class complex<long double>;

  ///  Return magnitude of @a z.
  template<typename _Tp> _Tp abs(const complex<_Tp>&);
  ///  Return phase angle of @a z.
  template<typename _Tp> _Tp arg(const complex<_Tp>&);
  ///  Return @a z magnitude squared.
  template<typename _Tp> _Tp norm(const complex<_Tp>&);

  ///  Return complex conjugate of @a z.
  template<typename _Tp> complex<_Tp> conj(const complex<_Tp>&);
  ///  Return complex with magnitude @a rho and angle @a theta.
  template<typename _Tp> complex<_Tp> polar(const _Tp&, const _Tp& = 0);

  // Transcendentals:
  /// Return complex cosine of @a z.
  template<typename _Tp> complex<_Tp> cos(const complex<_Tp>&);
  /// Return complex hyperbolic cosine of @a z.
  template<typename _Tp> complex<_Tp> cosh(const complex<_Tp>&);
  /// Return complex base e exponential of @a z.
  template<typename _Tp> complex<_Tp> exp(const complex<_Tp>&);
  /// Return complex natural logarithm of @a z.
  template<typename _Tp> complex<_Tp> log(const complex<_Tp>&);
  /// Return complex base 10 logarithm of @a z.
  template<typename _Tp> complex<_Tp> log10(const complex<_Tp>&);
  /// Return complex cosine of @a z.
  template<typename _Tp> complex<_Tp> pow(const complex<_Tp>&, int);
  /// Return @a x to the @a y'th power.
  template<typename _Tp> complex<_Tp> pow(const complex<_Tp>&, const _Tp&);
  /// Return @a x to the @a y'th power.
  template<typename _Tp> complex<_Tp> pow(const complex<_Tp>&, 
					   const complex<_Tp>&);
  /// Return @a x to the @a y'th power.
  template<typename _Tp> complex<_Tp> pow(const _Tp&, const complex<_Tp>&);
  /// Return complex sine of @a z.
  template<typename _Tp> complex<_Tp> sin(const complex<_Tp>&);
  /// Return complex hyperbolic sine of @a z.
  template<typename _Tp> complex<_Tp> sinh(const complex<_Tp>&);
  /// Return complex square root of @a z.
  template<typename _Tp> complex<_Tp> sqrt(const complex<_Tp>&);
  /// Return complex tangent of @a z.
  template<typename _Tp> complex<_Tp> tan(const complex<_Tp>&);
  /// Return complex hyperbolic tangent of @a z.
  template<typename _Tp> complex<_Tp> tanh(const complex<_Tp>&);
  //@}
    
    
  // 26.2.2  Primary template class complex
  /**
   *  Template to represent complex numbers.
   *
   *  Specializations for float, double, and long double are part of the
   *  library.  Results with any other type are not guaranteed.
   *
   *  @param  Tp  Type of real and imaginary values.
  */
  template<typename _Tp>
    class complex
    {
    public:
      /// Value typedef.
      typedef _Tp value_type;
      
      ///  Default constructor.  First parameter is x, second parameter is y.
      ///  Unspecified parameters default to 0.
      complex(const _Tp& = _Tp(), const _Tp & = _Tp());

      // Lets the compiler synthesize the copy constructor   
      // complex (const complex<_Tp>&);
      ///  Copy constructor.
      template<typename _Up>
        complex(const complex<_Up>&);

      ///  Return real part of complex number.
      _Tp& real(); 
      ///  Return real part of complex number.
      const _Tp& real() const;
      ///  Return imaginary part of complex number.
      _Tp& imag();
      ///  Return imaginary part of complex number.
      const _Tp& imag() const;

      /// Assign this complex number to scalar @a t.
      complex<_Tp>& operator=(const _Tp&);
      /// Add @a t to this complex number.
      complex<_Tp>& operator+=(const _Tp&);
      /// Subtract @a t from this complex number.
      complex<_Tp>& operator-=(const _Tp&);
      /// Multiply this complex number by @a t.
      complex<_Tp>& operator*=(const _Tp&);
      /// Divide this complex number by @a t.
      complex<_Tp>& operator/=(const _Tp&);

      // Lets the compiler synthesize the
      // copy and assignment operator
      // complex<_Tp>& operator= (const complex<_Tp>&);
      /// Assign this complex number to complex @a z.
      template<typename _Up>
        complex<_Tp>& operator=(const complex<_Up>&);
      /// Add @a z to this complex number.
      template<typename _Up>
        complex<_Tp>& operator+=(const complex<_Up>&);
      /// Subtract @a z from this complex number.
      template<typename _Up>
        complex<_Tp>& operator-=(const complex<_Up>&);
      /// Multiply this complex number by @a z.
      template<typename _Up>
        complex<_Tp>& operator*=(const complex<_Up>&);
      /// Divide this complex number by @a z.
      template<typename _Up>
        complex<_Tp>& operator/=(const complex<_Up>&);

    private:
      _Tp _M_real;
      _Tp _M_imag;
    };

  template<typename _Tp>
    inline _Tp&
    complex<_Tp>::real() { return _M_real; }

  template<typename _Tp>
    inline const _Tp&
    complex<_Tp>::real() const { return _M_real; }

  template<typename _Tp>
    inline _Tp&
    complex<_Tp>::imag() { return _M_imag; }

  template<typename _Tp>
    inline const _Tp&
    complex<_Tp>::imag() const { return _M_imag; }

  template<typename _Tp>
    inline 
    complex<_Tp>::complex(const _Tp& __r, const _Tp& __i)
    : _M_real(__r), _M_imag(__i) { }

  template<typename _Tp>
    template<typename _Up>
    inline 
    complex<_Tp>::complex(const complex<_Up>& __z)
    : _M_real(__z.real()), _M_imag(__z.imag()) { }
        
  template<typename _Tp>
    complex<_Tp>&
    complex<_Tp>::operator=(const _Tp& __t)
    {
     _M_real = __t;
     _M_imag = _Tp();
     return *this;
    } 

  // 26.2.5/1
  template<typename _Tp>
    inline complex<_Tp>&
    complex<_Tp>::operator+=(const _Tp& __t)
    {
      _M_real += __t;
      return *this;
    }

  // 26.2.5/3
  template<typename _Tp>
    inline complex<_Tp>&
    complex<_Tp>::operator-=(const _Tp& __t)
    {
      _M_real -= __t;
      return *this;
    }

  // 26.2.5/5
  template<typename _Tp>
    complex<_Tp>&
    complex<_Tp>::operator*=(const _Tp& __t)
    {
      _M_real *= __t;
      _M_imag *= __t;
      return *this;
    }

  // 26.2.5/7
  template<typename _Tp>
    complex<_Tp>&
    complex<_Tp>::operator/=(const _Tp& __t)
    {
      _M_real /= __t;
      _M_imag /= __t;
      return *this;
    }

  template<typename _Tp>
    template<typename _Up>
    complex<_Tp>&
    complex<_Tp>::operator=(const complex<_Up>& __z)
    {
      _M_real = __z.real();
      _M_imag = __z.imag();
      return *this;
    }

  // 26.2.5/9
  template<typename _Tp>
    template<typename _Up>
    complex<_Tp>&
    complex<_Tp>::operator+=(const complex<_Up>& __z)
    {
      _M_real += __z.real();
      _M_imag += __z.imag();
      return *this;
    }

  // 26.2.5/11
  template<typename _Tp>
    template<typename _Up>
    complex<_Tp>&
    complex<_Tp>::operator-=(const complex<_Up>& __z)
    {
      _M_real -= __z.real();
      _M_imag -= __z.imag();
      return *this;
    }

  // 26.2.5/13
  // XXX: This is a grammar school implementation.
  template<typename _Tp>
    template<typename _Up>
    complex<_Tp>&
    complex<_Tp>::operator*=(const complex<_Up>& __z)
    {
      const _Tp __r = _M_real * __z.real() - _M_imag * __z.imag();
      _M_imag = _M_real * __z.imag() + _M_imag * __z.real();
      _M_real = __r;
      return *this;
    }

  // 26.2.5/15
  // XXX: This is a grammar school implementation.
  template<typename _Tp>
    template<typename _Up>
    complex<_Tp>&
    complex<_Tp>::operator/=(const complex<_Up>& __z)
    {
      const _Tp __r =  _M_real * __z.real() + _M_imag * __z.imag();
      const _Tp __n = std::norm(__z);
      _M_imag = (_M_imag * __z.real() - _M_real * __z.imag()) / __n;
      _M_real = __r / __n;
      return *this;
    }
    
  // Operators:
  //@{
  ///  Return new complex value @a x plus @a y.
  template<typename _Tp>
    inline complex<_Tp>
    operator+(const complex<_Tp>& __x, const complex<_Tp>& __y)
    {
      complex<_Tp> __r = __x;
      __r += __y;
      return __r;
    }

  template<typename _Tp>
    inline complex<_Tp>
    operator+(const complex<_Tp>& __x, const _Tp& __y)
    {
      complex<_Tp> __r = __x;
      __r.real() += __y;
      return __r;
    }

  template<typename _Tp>
    inline complex<_Tp>
    operator+(const _Tp& __x, const complex<_Tp>& __y)
    {
      complex<_Tp> __r = __y;
      __r.real() += __x;
      return __r;
    }
  //@}

  //@{
  ///  Return new complex value @a x minus @a y.
  template<typename _Tp>
    inline complex<_Tp>
    operator-(const complex<_Tp>& __x, const complex<_Tp>& __y)
    {
      complex<_Tp> __r = __x;
      __r -= __y;
      return __r;
    }
    
  template<typename _Tp>
    inline complex<_Tp>
    operator-(const complex<_Tp>& __x, const _Tp& __y)
    {
      complex<_Tp> __r = __x;
      __r.real() -= __y;
      return __r;
    }

  template<typename _Tp>
    inline complex<_Tp>
    operator-(const _Tp& __x, const complex<_Tp>& __y)
    {
      complex<_Tp> __r(__x, -__y.imag());
      __r.real() -= __y.real();
      return __r;
    }
  //@}

  //@{
  ///  Return new complex value @a x times @a y.
  template<typename _Tp>
    inline complex<_Tp>
    operator*(const complex<_Tp>& __x, const complex<_Tp>& __y)
    {
      complex<_Tp> __r = __x;
      __r *= __y;
      return __r;
    }

  template<typename _Tp>
    inline complex<_Tp>
    operator*(const complex<_Tp>& __x, const _Tp& __y)
    {
      complex<_Tp> __r = __x;
      __r *= __y;
      return __r;
    }

  template<typename _Tp>
    inline complex<_Tp>
    operator*(const _Tp& __x, const complex<_Tp>& __y)
    {
      complex<_Tp> __r = __y;
      __r *= __x;
      return __r;
    }
  //@}

  //@{
  ///  Return new complex value @a x divided by @a y.
  template<typename _Tp>
    inline complex<_Tp>
    operator/(const complex<_Tp>& __x, const complex<_Tp>& __y)
    {
      complex<_Tp> __r = __x;
      __r /= __y;
      return __r;
    }
    
  template<typename _Tp>
    inline complex<_Tp>
    operator/(const complex<_Tp>& __x, const _Tp& __y)
    {
      complex<_Tp> __r = __x;
      __r /= __y;
      return __r;
    }

  template<typename _Tp>
    inline complex<_Tp>
    operator/(const _Tp& __x, const complex<_Tp>& __y)
    {
      complex<_Tp> __r = __x;
      __r /= __y;
      return __r;
    }
  //@}

  ///  Return @a x.
  template<typename _Tp>
    inline complex<_Tp>
    operator+(const complex<_Tp>& __x)
    { return __x; }

  ///  Return complex negation of @a x.
  template<typename _Tp>
    inline complex<_Tp>
    operator-(const complex<_Tp>& __x)
    {  return complex<_Tp>(-__x.real(), -__x.imag()); }

  //@{
  ///  Return true if @a x is equal to @a y.
  template<typename _Tp>
    inline bool
    operator==(const complex<_Tp>& __x, const complex<_Tp>& __y)
    { return __x.real() == __y.real() && __x.imag() == __y.imag(); }

  template<typename _Tp>
    inline bool
    operator==(const complex<_Tp>& __x, const _Tp& __y)
    { return __x.real() == __y && __x.imag() == _Tp(); }

  template<typename _Tp>
    inline bool
    operator==(const _Tp& __x, const complex<_Tp>& __y)
    { return __x == __y.real() && _Tp() == __y.imag(); }
  //@}

  //@{
  ///  Return false if @a x is equal to @a y.
  template<typename _Tp>
    inline bool
    operator!=(const complex<_Tp>& __x, const complex<_Tp>& __y)
    { return __x.real() != __y.real() || __x.imag() != __y.imag(); }

  template<typename _Tp>
    inline bool
    operator!=(const complex<_Tp>& __x, const _Tp& __y)
    { return __x.real() != __y || __x.imag() != _Tp(); }

  template<typename _Tp>
    inline bool
    operator!=(const _Tp& __x, const complex<_Tp>& __y)
    { return __x != __y.real() || _Tp() != __y.imag(); }
  //@}

  ///  Extraction operator for complex values.
  template<typename _Tp, typename _CharT, class _Traits>
    basic_istream<_CharT, _Traits>&
    operator>>(basic_istream<_CharT, _Traits>& __is, complex<_Tp>& __x)
    {
      _Tp __re_x, __im_x;
      _CharT __ch;
      __is >> __ch;
      if (__ch == '(') 
	{
	  __is >> __re_x >> __ch;
	  if (__ch == ',') 
	    {
	      __is >> __im_x >> __ch;
	      if (__ch == ')') 
		__x = complex<_Tp>(__re_x, __im_x);
	      else
		__is.setstate(ios_base::failbit);
	    }
	  else if (__ch == ')') 
	    __x = __re_x;
	  else
	    __is.setstate(ios_base::failbit);
	}
      else 
	{
	  __is.putback(__ch);
	  __is >> __re_x;
	  __x = __re_x;
	}
      return __is;
    }

  ///  Insertion operator for complex values.
  template<typename _Tp, typename _CharT, class _Traits>
    basic_ostream<_CharT, _Traits>&
    operator<<(basic_ostream<_CharT, _Traits>& __os, const complex<_Tp>& __x)
    {
      basic_ostringstream<_CharT, _Traits> __s;
      __s.flags(__os.flags());
      __s.imbue(__os.getloc());
      __s.precision(__os.precision());
      __s << '(' << __x.real() << ',' << __x.imag() << ')';
      return __os << __s.str();
    }

  // Values
  template<typename _Tp>
    inline _Tp&
    real(complex<_Tp>& __z)
    { return __z.real(); }
    
  template<typename _Tp>
    inline const _Tp&
    real(const complex<_Tp>& __z)
    { return __z.real(); }
    
  template<typename _Tp>
    inline _Tp&
    imag(complex<_Tp>& __z)
    { return __z.imag(); }
    
  template<typename _Tp>
    inline const _Tp&
    imag(const complex<_Tp>& __z)
    { return __z.imag(); }

  template<typename _Tp>
    inline _Tp
    abs(const complex<_Tp>& __z)
    {
      _Tp __x = __z.real();
      _Tp __y = __z.imag();
      const _Tp __s = std::max(abs(__x), abs(__y));
      if (__s == _Tp())  // well ...
        return __s;
      __x /= __s; 
      __y /= __s;
      return __s * sqrt(__x * __x + __y * __y);
    }

  template<typename _Tp>
    inline _Tp
    arg(const complex<_Tp>& __z)
    { return atan2(__z.imag(), __z.real()); }

  // 26.2.7/5: norm(__z) returns the squared magintude of __z.
  //     As defined, norm() is -not- a norm is the common mathematical
  //     sens used in numerics.  The helper class _Norm_helper<> tries to
  //     distinguish between builtin floating point and the rest, so as
  //     to deliver an answer as close as possible to the real value.
  template<bool>
    struct _Norm_helper
    {
      template<typename _Tp>
        static inline _Tp _S_do_it(const complex<_Tp>& __z)
        {
          const _Tp __x = __z.real();
          const _Tp __y = __z.imag();
          return __x * __x + __y * __y;
        }
    };

  template<>
    struct _Norm_helper<true>
    {
      template<typename _Tp>
        static inline _Tp _S_do_it(const complex<_Tp>& __z)
        {
          _Tp __res = std::abs(__z);
          return __res * __res;
        }
    };
  
  template<typename _Tp>
    inline _Tp
    norm(const complex<_Tp>& __z)
    {
      return _Norm_helper<__is_floating<_Tp>::_M_type && !_GLIBCXX_FAST_MATH>::_S_do_it(__z);
    }

  template<typename _Tp>
    inline complex<_Tp>
    polar(const _Tp& __rho, const _Tp& __theta)
    { return complex<_Tp>(__rho * cos(__theta), __rho * sin(__theta)); }

  template<typename _Tp>
    inline complex<_Tp>
    conj(const complex<_Tp>& __z)
    { return complex<_Tp>(__z.real(), -__z.imag()); }
  
  // Transcendentals
  template<typename _Tp>
    inline complex<_Tp>
    cos(const complex<_Tp>& __z)
    {
      const _Tp __x = __z.real();
      const _Tp __y = __z.imag();
      return complex<_Tp>(cos(__x) * cosh(__y), -sin(__x) * sinh(__y));
    }

  template<typename _Tp>
    inline complex<_Tp>
    cosh(const complex<_Tp>& __z)
    {
      const _Tp __x = __z.real();
      const _Tp __y = __z.imag();
      return complex<_Tp>(cosh(__x) * cos(__y), sinh(__x) * sin(__y));
    }

  template<typename _Tp>
    inline complex<_Tp>
    exp(const complex<_Tp>& __z)
    { return std::polar(exp(__z.real()), __z.imag()); }

  template<typename _Tp>
    inline complex<_Tp>
    log(const complex<_Tp>& __z)
    { return complex<_Tp>(log(std::abs(__z)), std::arg(__z)); }

  template<typename _Tp>
    inline complex<_Tp>
    log10(const complex<_Tp>& __z)
    { return std::log(__z) / log(_Tp(10.0)); }

  template<typename _Tp>
    inline complex<_Tp>
    sin(const complex<_Tp>& __z)
    {
      const _Tp __x = __z.real();
      const _Tp __y = __z.imag();
      return complex<_Tp>(sin(__x) * cosh(__y), cos(__x) * sinh(__y)); 
    }

  template<typename _Tp>
    inline complex<_Tp>
    sinh(const complex<_Tp>& __z)
    {
      const _Tp __x = __z.real();
      const _Tp  __y = __z.imag();
      return complex<_Tp>(sinh(__x) * cos(__y), cosh(__x) * sin(__y));
    }

  template<typename _Tp>
    complex<_Tp>
    sqrt(const complex<_Tp>& __z)
    {
      _Tp __x = __z.real();
      _Tp __y = __z.imag();

      if (__x == _Tp())
        {
          _Tp __t = sqrt(abs(__y) / 2);
          return complex<_Tp>(__t, __y < _Tp() ? -__t : __t);
        }
      else
        {
          _Tp __t = sqrt(2 * (std::abs(__z) + abs(__x)));
          _Tp __u = __t / 2;
          return __x > _Tp()
            ? complex<_Tp>(__u, __y / __t)
            : complex<_Tp>(abs(__y) / __t, __y < _Tp() ? -__u : __u);
        }
    }

  template<typename _Tp>
    inline complex<_Tp>
    tan(const complex<_Tp>& __z)
    {
      return std::sin(__z) / std::cos(__z);
    }

  template<typename _Tp>
    inline complex<_Tp>
    tanh(const complex<_Tp>& __z)
    {
      return std::sinh(__z) / std::cosh(__z);
    }

  template<typename _Tp>
    inline complex<_Tp>
    pow(const complex<_Tp>& __z, int __n)
    {
      return std::__pow_helper(__z, __n);
    }

  template<typename _Tp>
    complex<_Tp>
    pow(const complex<_Tp>& __x, const _Tp& __y)
    {
      if (__x.imag() == _Tp() && __x.real() > _Tp())
        return pow(__x.real(), __y);

      complex<_Tp> __t = std::log(__x);
      return std::polar(exp(__y * __t.real()), __y * __t.imag());
    }

  template<typename _Tp>
    inline complex<_Tp>
    pow(const complex<_Tp>& __x, const complex<_Tp>& __y)
    {
      return __x == _Tp() ? _Tp() : std::exp(__y * std::log(__x));
    }

  template<typename _Tp>
    inline complex<_Tp>
    pow(const _Tp& __x, const complex<_Tp>& __y)
    {
      return __x > _Tp() ? std::polar(pow(__x, __y.real()),
				      __y.imag() * log(__x))
	                 : std::pow(complex<_Tp>(__x, _Tp()), __y);
    }

  // 26.2.3  complex specializations
  // complex<float> specialization
  template<> class complex<float>
  {
  public:
    typedef float value_type;
    
    complex(float = 0.0f, float = 0.0f);

    explicit complex(const complex<double>&);
    explicit complex(const complex<long double>&);

    float& real();
    const float& real() const;
    float& imag();
    const float& imag() const;

    complex<float>& operator=(float);
    complex<float>& operator+=(float);
    complex<float>& operator-=(float);
    complex<float>& operator*=(float);
    complex<float>& operator/=(float);
        
    // Let's the compiler synthetize the copy and assignment
    // operator.  It always does a pretty good job.
    // complex& operator= (const complex&);
    template<typename _Tp>
      complex<float>&operator=(const complex<_Tp>&);
    template<typename _Tp>
      complex<float>& operator+=(const complex<_Tp>&);
    template<class _Tp>
      complex<float>& operator-=(const complex<_Tp>&);
    template<class _Tp>
      complex<float>& operator*=(const complex<_Tp>&);
    template<class _Tp>
      complex<float>&operator/=(const complex<_Tp>&);

  private:
    typedef __complex__ float _ComplexT;
    _ComplexT _M_value;

    complex(_ComplexT __z) : _M_value(__z) { }
        
    friend class complex<double>;
    friend class complex<long double>;
  };

  inline float&
  complex<float>::real()
  { return __real__ _M_value; }

  inline const float&
  complex<float>::real() const
  { return __real__ _M_value; }

  inline float&
  complex<float>::imag()
  { return __imag__ _M_value; }

  inline const float&
  complex<float>::imag() const
  { return __imag__ _M_value; }

  inline
  complex<float>::complex(float r, float i)
  {
    __real__ _M_value = r;
    __imag__ _M_value = i;
  }

  inline complex<float>&
  complex<float>::operator=(float __f)
  {
    __real__ _M_value = __f;
    __imag__ _M_value = 0.0f;
    return *this;
  }

  inline complex<float>&
  complex<float>::operator+=(float __f)
  {
    __real__ _M_value += __f;
    return *this;
  }

  inline complex<float>&
  complex<float>::operator-=(float __f)
  {
    __real__ _M_value -= __f;
    return *this;
  }

  inline complex<float>&
  complex<float>::operator*=(float __f)
  {
    _M_value *= __f;
    return *this;
  }

  inline complex<float>&
  complex<float>::operator/=(float __f)
  {
    _M_value /= __f;
    return *this;
  }

  template<typename _Tp>
  inline complex<float>&
  complex<float>::operator=(const complex<_Tp>& __z)
  {
    __real__ _M_value = __z.real();
    __imag__ _M_value = __z.imag();
    return *this;
  }

  template<typename _Tp>
  inline complex<float>&
  complex<float>::operator+=(const complex<_Tp>& __z)
  {
    __real__ _M_value += __z.real();
    __imag__ _M_value += __z.imag();
    return *this;
  }
    
  template<typename _Tp>
    inline complex<float>&
    complex<float>::operator-=(const complex<_Tp>& __z)
    {
     __real__ _M_value -= __z.real();
     __imag__ _M_value -= __z.imag();
     return *this;
    } 

  template<typename _Tp>
    inline complex<float>&
    complex<float>::operator*=(const complex<_Tp>& __z)
    {
      _ComplexT __t;
      __real__ __t = __z.real();
      __imag__ __t = __z.imag();
      _M_value *= __t;
      return *this;
    }

  template<typename _Tp>
    inline complex<float>&
    complex<float>::operator/=(const complex<_Tp>& __z)
    {
      _ComplexT __t;
      __real__ __t = __z.real();
      __imag__ __t = __z.imag();
      _M_value /= __t;
      return *this;
    }

  // 26.2.3  complex specializations
  // complex<double> specialization
  template<> class complex<double>
  {
  public:
    typedef double value_type;

    complex(double = 0.0, double = 0.0);

    complex(const complex<float>&);
    explicit complex(const complex<long double>&);

    double& real();
    const double& real() const;
    double& imag();
    const double& imag() const;
        
    complex<double>& operator=(double);
    complex<double>& operator+=(double);
    complex<double>& operator-=(double);
    complex<double>& operator*=(double);
    complex<double>& operator/=(double);

    // The compiler will synthetize this, efficiently.
    // complex& operator= (const complex&);
    template<typename _Tp>
      complex<double>& operator=(const complex<_Tp>&);
    template<typename _Tp>
      complex<double>& operator+=(const complex<_Tp>&);
    template<typename _Tp>
      complex<double>& operator-=(const complex<_Tp>&);
    template<typename _Tp>
      complex<double>& operator*=(const complex<_Tp>&);
    template<typename _Tp>
      complex<double>& operator/=(const complex<_Tp>&);

  private:
    typedef __complex__ double _ComplexT;
    _ComplexT _M_value;

    complex(_ComplexT __z) : _M_value(__z) { }
        
    friend class complex<float>;
    friend class complex<long double>;
  };

  inline double&
  complex<double>::real()
  { return __real__ _M_value; }

  inline const double&
  complex<double>::real() const
  { return __real__ _M_value; }

  inline double&
  complex<double>::imag()
  { return __imag__ _M_value; }

  inline const double&
  complex<double>::imag() const
  { return __imag__ _M_value; }

  inline
  complex<double>::complex(double __r, double __i)
  {
    __real__ _M_value = __r;
    __imag__ _M_value = __i;
  }

  inline complex<double>&
  complex<double>::operator=(double __d)
  {
    __real__ _M_value = __d;
    __imag__ _M_value = 0.0;
    return *this;
  }

  inline complex<double>&
  complex<double>::operator+=(double __d)
  {
    __real__ _M_value += __d;
    return *this;
  }

  inline complex<double>&
  complex<double>::operator-=(double __d)
  {
    __real__ _M_value -= __d;
    return *this;
  }

  inline complex<double>&
  complex<double>::operator*=(double __d)
  {
    _M_value *= __d;
    return *this;
  }

  inline complex<double>&
  complex<double>::operator/=(double __d)
  {
    _M_value /= __d;
    return *this;
  }

  template<typename _Tp>
    inline complex<double>&
    complex<double>::operator=(const complex<_Tp>& __z)
    {
      __real__ _M_value = __z.real();
      __imag__ _M_value = __z.imag();
      return *this;
    }
    
  template<typename _Tp>
    inline complex<double>&
    complex<double>::operator+=(const complex<_Tp>& __z)
    {
      __real__ _M_value += __z.real();
      __imag__ _M_value += __z.imag();
      return *this;
    }

  template<typename _Tp>
    inline complex<double>&
    complex<double>::operator-=(const complex<_Tp>& __z)
    {
      __real__ _M_value -= __z.real();
      __imag__ _M_value -= __z.imag();
      return *this;
    }

  template<typename _Tp>
    inline complex<double>&
    complex<double>::operator*=(const complex<_Tp>& __z)
    {
      _ComplexT __t;
      __real__ __t = __z.real();
      __imag__ __t = __z.imag();
      _M_value *= __t;
      return *this;
    }

  template<typename _Tp>
    inline complex<double>&
    complex<double>::operator/=(const complex<_Tp>& __z)
    {
      _ComplexT __t;
      __real__ __t = __z.real();
      __imag__ __t = __z.imag();
      _M_value /= __t;
      return *this;
    }

  // 26.2.3  complex specializations
  // complex<long double> specialization
  template<> class complex<long double>
  {
  public:
    typedef long double value_type;

    complex(long double = 0.0L, long double = 0.0L);

    complex(const complex<float>&);
    complex(const complex<double>&);

    long double& real();
    const long double& real() const;
    long double& imag();
    const long double& imag() const;

    complex<long double>& operator= (long double);
    complex<long double>& operator+= (long double);
    complex<long double>& operator-= (long double);
    complex<long double>& operator*= (long double);
    complex<long double>& operator/= (long double);

    // The compiler knows how to do this efficiently
    // complex& operator= (const complex&);
    template<typename _Tp>
      complex<long double>& operator=(const complex<_Tp>&);
    template<typename _Tp>
      complex<long double>& operator+=(const complex<_Tp>&);
    template<typename _Tp>
      complex<long double>& operator-=(const complex<_Tp>&);
    template<typename _Tp>
      complex<long double>& operator*=(const complex<_Tp>&);
    template<typename _Tp>
      complex<long double>& operator/=(const complex<_Tp>&);

  private:
    typedef __complex__ long double _ComplexT;
    _ComplexT _M_value;

    complex(_ComplexT __z) : _M_value(__z) { }

    friend class complex<float>;
    friend class complex<double>;
  };

  inline
  complex<long double>::complex(long double __r, long double __i)
  {
    __real__ _M_value = __r;
    __imag__ _M_value = __i;
  }

  inline long double&
  complex<long double>::real()
  { return __real__ _M_value; }

  inline const long double&
  complex<long double>::real() const
  { return __real__ _M_value; }

  inline long double&
  complex<long double>::imag()
  { return __imag__ _M_value; }

  inline const long double&
  complex<long double>::imag() const
  { return __imag__ _M_value; }

  inline complex<long double>&   
  complex<long double>::operator=(long double __r)
  {
    __real__ _M_value = __r;
    __imag__ _M_value = 0.0L;
    return *this;
  }

  inline complex<long double>&
  complex<long double>::operator+=(long double __r)
  {
    __real__ _M_value += __r;
    return *this;
  }

  inline complex<long double>&
  complex<long double>::operator-=(long double __r)
  {
    __real__ _M_value -= __r;
    return *this;
  }

  inline complex<long double>&
  complex<long double>::operator*=(long double __r)
  {
    _M_value *= __r;
    return *this;
  }

  inline complex<long double>&
  complex<long double>::operator/=(long double __r)
  {
    _M_value /= __r;
    return *this;
  }

  template<typename _Tp>
    inline complex<long double>&
    complex<long double>::operator=(const complex<_Tp>& __z)
    {
      __real__ _M_value = __z.real();
      __imag__ _M_value = __z.imag();
      return *this;
    }

  template<typename _Tp>
    inline complex<long double>&
    complex<long double>::operator+=(const complex<_Tp>& __z)
    {
      __real__ _M_value += __z.real();
      __imag__ _M_value += __z.imag();
      return *this;
    }

  template<typename _Tp>
    inline complex<long double>&
    complex<long double>::operator-=(const complex<_Tp>& __z)
    {
      __real__ _M_value -= __z.real();
      __imag__ _M_value -= __z.imag();
      return *this;
    }
    
  template<typename _Tp>
    inline complex<long double>&
    complex<long double>::operator*=(const complex<_Tp>& __z)
    {
      _ComplexT __t;
      __real__ __t = __z.real();
      __imag__ __t = __z.imag();
      _M_value *= __t;
      return *this;
    }

  template<typename _Tp>
    inline complex<long double>&
    complex<long double>::operator/=(const complex<_Tp>& __z)
    {
      _ComplexT __t;
      __real__ __t = __z.real();
      __imag__ __t = __z.imag();
      _M_value /= __t;
      return *this;
    }

  // These bits have to be at the end of this file, so that the
  // specializations have all been defined.
  // ??? No, they have to be there because of compiler limitation at
  // inlining.  It suffices that class specializations be defined.
  inline
  complex<float>::complex(const complex<double>& __z)
  : _M_value(_ComplexT(__z._M_value)) { }

  inline
  complex<float>::complex(const complex<long double>& __z)
  : _M_value(_ComplexT(__z._M_value)) { }

  inline
  complex<double>::complex(const complex<float>& __z) 
  : _M_value(_ComplexT(__z._M_value)) { }

  inline
  complex<double>::complex(const complex<long double>& __z)
  {
    __real__ _M_value = __z.real();
    __imag__ _M_value = __z.imag();
  }

  inline
  complex<long double>::complex(const complex<float>& __z)
  : _M_value(_ComplexT(__z._M_value)) { }

  inline
  complex<long double>::complex(const complex<double>& __z)
  : _M_value(_ComplexT(__z._M_value)) { }
} // namespace std

#endif	/* _GLIBCXX_COMPLEX */
