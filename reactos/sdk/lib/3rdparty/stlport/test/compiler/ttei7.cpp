/*
 * It is known that this code not compiled by following compilers:
 *
 *   MSVC 6
 *
 * It is known that this code compiled by following compilers:
 *
 *   MSVC 8 Beta
 */

/*
 * This code represent what STLport waits from a compiler which support
 * the rebind member template class technique (!_STLP_DONT_SUPPORT_REBIND_MEMBER_TEMPLATE)
 */

template <typename T1>
struct A
{
  template <typename T2>
  struct B
  {
    typedef A<T2> _Type;
  };
};


template <typename T, typename A>
struct C
{
  typedef typename A:: template B<T>::_Type _ATType;
};
