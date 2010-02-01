/*
 * @implemented
 * copy _i64toa from wine cvs 2006 month 05 day 21
 */
char * _CDECL _ui64toa(unsigned __int64 value, char *string, int radix)
{
    char buffer[65];
    char *pos;
    int digit;

    pos = &buffer[64];
    *pos = '\0';

    do {
	digit = value % radix;
	value = value / radix;
	if (digit < 10) {
	    *--pos = '0' + digit;
	} else {
	    *--pos = 'a' + digit - 10;
	} /* if */
    } while (value != 0L);

    memcpy(string, pos, &buffer[64] - pos + 1);
    return string;
}
