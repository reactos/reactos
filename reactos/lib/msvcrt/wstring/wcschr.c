/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrti.h>


wchar_t* wcschr(const wchar_t* str, wchar_t ch)
{
	while ((*str)!=0)
	{
		if ((*str)==ch)
		{
			return((wchar_t *)str);
		}
		str++;
	}
	return(NULL);
}
