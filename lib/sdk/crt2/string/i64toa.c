/*
 * @implemented
 * copy _i64toa from wine cvs 2006 month 05 day 21
 */
char * _CDECL _i64toa(__int64 value, char *string, int radix)
{
	ULONGLONG  val;
	int negative;
	char buffer[65];
	char *pos;
	int digit;

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

	memcpy(string, pos, &buffer[64] - pos + 1);
	return string;
}

