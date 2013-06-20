/*
 * It is known that this code not compiled by following compilers:
 *
 * It is known that this code compiled by following compilers:
 *
 *   MSVC 6
 *   MSVC 8 Beta
 */

/*
 * This code represent what STLport waits from a compiler which support
 * member template classes (!_STLP_NO_MEMBER_TEMPLATE_CLASSES)
 */

template <typename T1>
struct A
{
  template <typename T2>
  struct B
  {
    typedef T2 _Type;
  };
};


template <typename T1, typename T2>
struct C
{
  typedef typename A<T1>:: template B<T2>::_Type ABType;
};
