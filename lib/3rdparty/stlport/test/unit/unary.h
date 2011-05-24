#ifndef _unary_h
#define _unary_h
#include <cmath>
#include <cfloat>
#include <functional>    //*TY 12/26/1998 - added to get unary_function

#if !defined (STLPORT) || defined (_STLP_USE_NAMESPACES)
using std::unary_function;
#endif

struct odd : public unary_function<int, bool> {
  bool operator()(int n_) const { return(n_ % 2) == 1; }
};

struct positive : public unary_function<int, bool> {
  bool operator()(int n_) const { return n_ >= 0; }
};

struct square_root : public unary_function<double, double> {
  double operator()(double x_) const
  { return ::sqrt(x_); }
};
#endif // _unary_h
