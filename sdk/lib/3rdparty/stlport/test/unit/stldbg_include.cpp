/*
 * This is compile-time test for situation below not happen.
 * STLport use many defines and auxilary structures, namespaces and templates
 * that included via _prolog.h. After preprocessor phase we may see:
 *
 * extern "C" {
 *
 * namespace std { }
 *
 *
 * This is bad, but acceptable. But in STLPORT_DEBUG mode we can see
 *
 * extern "C" {
 *
 * namespace std {
 * namespace private {
 *
 * template <class _Dummy>
 * class __stl_debug_engine {
 *
 * 
 * This lead to compile-time error.
 * [This due to sys/types.h contains
 *
 *  __BEGIN_DECLS
 *  #include <bits/types.h>
 *
 * 
 * i.e. include other headers within extern "C" { scope. Bad, but this is fact.]
 *
 * Origin of problem: STLport provide proxy-headers as for C++ headers, as for C
 * headers. For C headers, we shouldn't expose C++ constructions, because system
 * headers may include each other by unexpected way (from STLport point of view).
 *
 *           - ptr, 2007-04-05
 */

#ifdef __unix
# include <sys/types.h>
#endif
