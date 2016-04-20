/*
 * It is known that this code not compiled by following compilers:
 *
 *   MSVC 6
 *
 * It is known that this code compiled by following compilers:
 *
 *   MSVC 8
 *   gcc 4.1.1
 */

/*
 * This code represent what STLport waits from a compiler which support
 * the partial template function ordering (!_STLP_NO_FUNCTION_TMPL_PARTIAL_ORDER)
 */

template <class T1>
struct template_struct {};

template <class T1>
int func(T1 p1);

template <class T1>
int func(template_struct<T1>);


int foo()
{
  int tmp1 = 0;
  template_struct<int> tmp2;
  func(tmp1);
  func(tmp2);
  return 0;
}
