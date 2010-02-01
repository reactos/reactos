/*
 * @implemented
 */
wchar_t * _CDECL _ui64tow(unsigned __int64 value, wchar_t *string, int radix)
{
	WCHAR buffer[65];
	PWCHAR pos;
	WCHAR digit;

	pos = &buffer[64];
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
		memcpy(string, pos, (&buffer[64] - pos + 1) * sizeof(WCHAR));
	}
	return string;
}
