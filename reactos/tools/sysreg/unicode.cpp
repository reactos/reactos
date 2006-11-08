#include "unicode.h"

#include <stdlib.h>
#include <stdio.h>

namespace System_
{
//---------------------------------------------------------------------------------------
	bool UnicodeConverter::ansi2Unicode(char * abuf, wchar_t *outbuf, size_t length)
	{
		size_t i = 0;
		int conv;

		while((conv = mbtowc(&outbuf[i], &abuf[i], length - i)))
		{
			i += conv;
			if (i == length)
				break;
		}
		outbuf[i] = L'\0';

		if (i)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
} // end of namespace System_
