/*
 * @implemented
 *  copy it from wine 0.9.0 with small modifcations do check for NULL
 */
char * _CDECL _ultoa(unsigned long value, char *string, int radix)
{
	char buffer[33];
	char *pos;
	int digit;

	pos = &buffer[32];
	*pos = '\0';

	if (string == NULL)
	{
		return NULL;
	}

	do {
		digit = value % radix;
		value = value / radix;
		if (digit < 10) {
			*--pos = '0' + digit;
		} else {
			*--pos = 'a' + digit - 10;
		}
	} while (value != 0L);

	memcpy(string, pos, &buffer[32] - pos + 1);

	return string;
}
