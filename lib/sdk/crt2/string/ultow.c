/*
 * @implemented
 * from wine cvs 2006-05-21
 */
wchar_t * _CDECL _ultow(unsigned long value, wchar_t *string, int radix)
{
	WCHAR buffer[33];
	PWCHAR pos;
	WCHAR digit;

	pos = &buffer[32];
	*pos = '\0';

	do {
		digit = value % radix;
		value = value / radix;
		if (digit < 10) {
			*--pos = '0' + digit;
		} else {
			*--pos = 'a' + digit - 10;
		}
	} while (value != 0L);

	if (string != NULL) {
		memcpy(string, pos, (&buffer[32] - pos + 1) * sizeof(WCHAR));
	}
	return string;
}
