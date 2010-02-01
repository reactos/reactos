/*
 * @implemented
 */
wchar_t * _CDECL _wcsupr(wchar_t *x)
{
	wchar_t  *y = x;

	while (*y) {
		*y = towupper(*y);
		y++;
	}
	return x;
}
