/* This is an implementation file which is intended to be included
 * multiple times with different _STLP_TEMPLATE_CONTAINER settings.
 */

#if defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)

_STLP_TEMPLATE_HEADER
inline void _STLP_CALL
swap(_STLP_TEMPLATE_CONTAINER& __hm1, _STLP_TEMPLATE_CONTAINER& __hm2) {
  __hm1.swap(__hm2);
}

#endif /* _STLP_FUNCTION_TMPL_PARTIAL_ORDER */
