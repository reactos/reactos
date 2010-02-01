/*
 * @implemented
 */
long _CDECL atol(const char *str)
{
	return (long)_atoi64(str);
}

