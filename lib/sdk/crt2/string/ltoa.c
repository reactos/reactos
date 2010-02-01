/*
 * @implemented
 */
#if 0 // #define _ltoa _itoa
char * _CDECL _ltoa(long value, char *string, int radix)
{
	return _itoa(value, string, radix);
}
#endif
