/*
 * @implemented
 */
int _CDECL _stricmp(const char *s1, const char *s2)
{
	while (toupper(*s1) == toupper(*s2))
	{
		if (*s1 == 0)
			return 0;
		s1++;
		s2++;
	}
	return toupper(*(unsigned const char *)s1) - toupper(*(unsigned const char *)(s2));
}

