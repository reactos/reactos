#ifndef WITHOUT_STLPORT 
#include <stl/config/user_config.h>

#ifdef _STLP_USE_BOOST_SUPPORT

#include <boost/config.hpp>

#endif // _STLP_USE_BOOST_SUPPORT

#else

#if 0 // Problem 1:
/* *******************************
../../../stlport/functional:63: error: 'boost::mem_fn' has not been declared
../../../stlport/functional:64: error: 'boost::bind' has not been declared
../../../stlport/functional:67: error: '::_1' has not been declared
../../../stlport/functional:68: error: '::_2' has not been declared
../../../stlport/functional:69: error: '::_3' has not been declared
../../../stlport/functional:70: error: '::_4' has not been declared
../../../stlport/functional:71: error: '::_5' has not been declared
../../../stlport/functional:72: error: '::_6' has not been declared
../../../stlport/functional:73: error: '::_7' has not been declared
../../../stlport/functional:74: error: '::_8' has not been declared
../../../stlport/functional:75: error: '::_9' has not been declared
   ******************************* */

#include <boost/bind.hpp>

#endif // Problem 1

#if 0 // Problem 2

#include <boost/function.hpp>

#endif // Problem 2

#if 0 // Problem 3

#include <boost/function/function_base.hpp>

#endif // Problem 3

#if 0 // Problem 4

#include <boost/function/function1.hpp>

#endif // Problem 4

#endif
