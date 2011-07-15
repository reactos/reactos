/* This test purpose is simply to check Standard header independancy that
 * is to say that the header can be included alone without any previous
 * include.
 * Additionnaly, for C Standard headers that STLport expose, it can also be
 * used to check that files included by those headers are compatible with
 * pure C compilers.
 */
#include <typeinfo>

#if 0 /* !defined (_STLP_NO_RTTI) && !defined (_STLP_NO_TYPEINFO) */
/* SourceForge: STLport bug report 1721844
 * type_info is not a member of stlp_std
 */
class A {};

void type_info_header_test()
{
  const std::type_info& ti = typeid(A);
}
#endif
