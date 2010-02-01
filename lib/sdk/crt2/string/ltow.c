/*
 * @implemented
 * from wine cvs 2006-05-21
 */
#if 0 // @define _ltow _itow
wchar_t * _CDECL _ltow(long value, wchar_t *string, int radix)
{
	return _itow(value, string, radix);
}
#endif

