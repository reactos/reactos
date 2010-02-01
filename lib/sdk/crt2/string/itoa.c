/*
 * @implemented
 */
char * _CDECL _itoa(int value, char *string, int radix)
{
	unsigned long val;
	int negative;
	char buffer[33];
	char *pos;
	int digit;

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

	memcpy(string, pos, &buffer[32] - pos + 1);
	return string;
}
