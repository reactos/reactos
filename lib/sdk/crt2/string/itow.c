/*
 * @implemented
 * from wine cvs 2006-05-21
 */
wchar_t * _CDECL _itow(int value, wchar_t *string, int radix)
{
	unsigned long val;
	int negative;
	WCHAR buffer[33];
	PWCHAR pos;
	WCHAR digit;

	if (value < 0 && radix == 10) {
		negative = 1;
		val = -value;
	} else {
		negative = 0;
		val = value;
	}

	pos = &buffer[32];
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
		memcpy(string, pos, (&buffer[32] - pos + 1) * sizeof(WCHAR));
	}
	return string;
}

