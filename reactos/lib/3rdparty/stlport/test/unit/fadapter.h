#ifndef _fadapter_h_
#define _fadapter_h_

#include <functional>

// used as adaptor's return/argument type,
// to allow binders/composers usage
struct __void_tag {};

#if !defined (STLPORT) || defined (_STLP_USE_NAMESPACES)
using std::unary_function;
#endif

template <class Result>
class pointer_to_void_function {
protected:
  Result (*ptr)();
public:
  explicit pointer_to_void_function(Result (*x)()) : ptr(x) {}
  Result operator()() const { return ptr(); }
  Result operator()(__void_tag) const { return ptr(); }
};

// to feed composers
template <class Arg1>
struct projectvoid : public unary_function<Arg1,__void_tag> {
  __void_tag operator()(const Arg1& x) const { return __void_tag(); }
};

#if !defined (_STLP_MEMBER_POINTER_PARAM_BUG)

template <class Result>
pointer_to_void_function<Result> ptr_fun(Result (*x)()) {
  return pointer_to_void_function<Result>(x);
}

// alternate name
template <class Result>
pointer_to_void_function<Result> ptr_gen(Result (*x)()) {
  return pointer_to_void_function<Result>(x);
}

#endif /*  !defined (_STLP_MEMBER_POINTER_PARAM_BUG) */

template <class Arg>
class pointer_to_unary_procedure /* :public unary_function<Arg, __void_tag> */ {
protected:
  typedef void (*fun_type)(Arg);
  fun_type ptr;
public:
  typedef Arg argument_type;
  pointer_to_unary_procedure() {}
  pointer_to_unary_procedure(fun_type x) : ptr(x) {}
  void operator() (Arg x) const { ptr(x); }
};

template <class Arg>
inline pointer_to_unary_procedure<Arg> ptr_proc(void (*x)(Arg)) {
  return pointer_to_unary_procedure<Arg>(x);
}

template <class Arg1, class Arg2>
class pointer_to_binary_procedure /* : public unary_function<Arg1, Arg2, __void_tag> */ {
protected:
  typedef void (*fun_type)(Arg1, Arg2);
  fun_type ptr;
public:
  typedef Arg1 first_argument_type;
  typedef Arg2 second_argument_type;
  pointer_to_binary_procedure() {}
  pointer_to_binary_procedure(fun_type x) : ptr(x) {}
  void operator() (Arg1 x, Arg2 y) const { ptr(x, y); }
};

template <class Arg1, class Arg2>
inline pointer_to_binary_procedure<Arg1, Arg2> ptr_proc(void (*x)(Arg1, Arg2)) {
  return pointer_to_binary_procedure<Arg1, Arg2>(x);
}

#endif
