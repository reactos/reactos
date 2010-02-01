/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */

/*
 * @implemented
 */
int _CDECL atoi(const char *str)
{
	char *s = (char *)str;
	int acc = 0;
	char neg = 0;

	if(!str)
		return 0;

	while(isspace((int)*s))
		s++;
	if(*s == '-')
	{
		neg = 1;
		s++;
	}
	else if(*s == '+')
		s++;

	while(isdigit((int)*s))
	{
		acc = 10 * acc + ((int)*s - '0');
		s++;
	}

	if (neg)
		acc = -acc;
	return acc;
}
