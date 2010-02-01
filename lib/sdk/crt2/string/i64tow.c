/*
 * @implemented
 * from wine cvs 2006-05-21
 */
wchar_t * _CDECL _i64tow(__int64 value, wchar_t *string, int radix)
{
	ULONGLONG val;
	int negative;
	WCHAR buffer[65];
	PWCHAR pos;
	WCHAR digit;

	if (value < 0 && radix == 10) {
		negative = 1;
		val = -value;
	} else {
		negative = 0;
		val = value;
	}

	pos = &buffer[64];
	*pos = '\0';

	do {
		digit = val % radix;
		val = val / radix;
		if (digit < 10) {
			*--pos = '0' + digit;
		} else {
			*--pos = 'a' + digit - 10;
		}
	} while (val != 0L);

	if (negative) {
		*--pos = '-';
	}

	if (string != NULL) {
		memcpy(string, pos, (&buffer[64] - pos + 1) * sizeof(WCHAR));
	}
	return string;
}
