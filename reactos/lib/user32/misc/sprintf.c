/* $Id: sprintf.c,v 1.4 2001/02/19 15:04:51 dwelch Exp $
 *
 * user32.dll
 *
 * wsprintf functions
 *
 * Copyright 1996 Alexandre Julliard
 *
 * 1999-05-01 (Emanuele Aliberti)
 * 	Adapted from Wine to ReactOS
 */

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>

#define WPRINTF_LEFTALIGN   0x0001  /* Align output on the left ('-' prefix) */
#define WPRINTF_PREFIX_HEX  0x0002  /* Prefix hex with 0x ('#' prefix) */
#define WPRINTF_ZEROPAD     0x0004  /* Pad with zeros ('0' prefix) */
#define WPRINTF_LONG        0x0008  /* Long arg ('l' prefix) */
#define WPRINTF_SHORT       0x0010  /* Short arg ('h' prefix) */
#define WPRINTF_UPPER_HEX   0x0020  /* Upper-case hex ('X' specifier) */
#define WPRINTF_WIDE        0x0040  /* Wide arg ('w' prefix) */

typedef enum
{
	WPR_UNKNOWN,
	WPR_CHAR,
	WPR_WCHAR,
	WPR_STRING,
	WPR_WSTRING,
	WPR_SIGNED,
	WPR_UNSIGNED,
	WPR_HEXA
		
} WPRINTF_TYPE;


typedef struct
{
	UINT		flags;
	UINT		width;
	UINT		precision;
	WPRINTF_TYPE	type;
	
} WPRINTF_FORMAT;


static const LPSTR	null_stringA = "(null)";
static const LPWSTR	null_stringW = L"(null)";


/* === COMMON === */


/***********************************************************************
 * NAME								PRIVATE
 *	WPRINTF_GetLen
 *
 * DESCRIPTION
 *	?
 *
 * ARGUMENTS
 *	format
 *		?
 *	arg
 *		?
 *	number
 *		?
 *	maxlen
 *		?
 *
 * RETURN VALUE
 *	?
 */
static
UINT
WPRINTF_GetLen(
	WPRINTF_FORMAT	*format,
	LPCVOID		arg,
	LPSTR		number,
	UINT		maxlen
	)
{
	UINT len;

	if (format->flags & WPRINTF_LEFTALIGN)
	{
		format->flags &= ~WPRINTF_ZEROPAD;
	}
	if (format->width > maxlen)
	{
		format->width = maxlen;
	}
	switch(format->type)
	{
	case WPR_CHAR:
	case WPR_WCHAR:
        	return (format->precision = 1);
		
	case WPR_STRING:
		if (!*(LPCSTR *)arg)
		{
			*(LPCSTR *)arg = null_stringA;
		}
		for (	len = 0;
			(!format->precision || (len < format->precision));
			len++
			)
		{
			if (!*(*(LPCSTR *)arg + len))
			{
				break;
			}
		}
		if (len > maxlen)
		{
			len = maxlen;
		}
		return (format->precision = len);

	case WPR_WSTRING:
		if (!*(LPCWSTR *)arg)
		{
			*(LPCWSTR *)arg = null_stringW;
		}
		for (	len = 0;
			(!format->precision || (len < format->precision));
			len++
			)
		{
	            if (!*(*(LPCWSTR *)arg + len))
			    break;
		}
		if (len > maxlen)
		{
			len = maxlen;
		}
		return (format->precision = len);
		
	case WPR_SIGNED:
        	len = sprintf(
			number,
			"%d",
			*(INT *) arg
			);
		break;
		
	case WPR_UNSIGNED:
		len = sprintf(
			number,
			"%u",
			*(UINT *) arg
			);
		break;
		
	case WPR_HEXA:
        	len = sprintf(
			number,
                        ((format->flags & WPRINTF_UPPER_HEX)
				? "%X"
				: "%x"),
                        *(UINT *) arg
			);
	        if (format->flags & WPRINTF_PREFIX_HEX)
		{
			len += 2;
		}
	        break;
	
	default:
		return 0;
	}
	if (len > maxlen)
	{
		len = maxlen;
	}
	if (format->precision < len)
	{
		format->precision = len;
	}
	if (format->precision > maxlen)
	{
		format->precision = maxlen;
	}
	if (	(format->flags & WPRINTF_ZEROPAD)
		&& (format->width > format->precision)
		)
	{
		format->precision = format->width;
	}
	return len;
}


/* === ANSI VERSION === */


/***********************************************************************
 * NAME								PRIVATE
 *	WPRINTF_ParseFormatA
 *
 * DESCRIPTION
 *	Parse a format specification. A format specification has the
 *	form:
 *
 *	[-][#][0][width][.precision]type
 *
 * ARGUMENTS
 *	format
 *		?
 *	res
 *		?
 *
 * RETURN VALUE
 * 	The length of the format specification in characters.
 */
static
INT
WPRINTF_ParseFormatA(
	LPCSTR		format,
	WPRINTF_FORMAT	*res
	)
{
	LPCSTR p = format;

	res->flags = 0;
	res->width = 0;
	res->precision = 0;
	if (*p == '-')
	{
		res->flags |= WPRINTF_LEFTALIGN;
		p++;
	}
	if (*p == '#')
	{
		res->flags |= WPRINTF_PREFIX_HEX;
		p++;
	}
	if (*p == '0')
	{
		res->flags |= WPRINTF_ZEROPAD;
		p++;
	}
	while ((*p >= '0') && (*p <= '9'))  /* width field */
	{
		res->width =
			(res->width * 10)
			+ (*p - '0');
		p++;
	}
	if (*p == '.')  /* precision field */
	{
		p++;
		while ((*p >= '0') && (*p <= '9'))
		{
			res->precision =
				(res->precision * 10)
				+ (*p - '0');
			p++;
		}
	}
	if (*p == 'l')
	{
		res->flags |= WPRINTF_LONG;
		p++;
	}
	else if (*p == 'h')
	{
		res->flags |= WPRINTF_SHORT;
		p++;
	}
	else if (*p == 'w')
	{
		res->flags |= WPRINTF_WIDE;
		p++;
	}
	
	switch (*p)
	{
	case 'c':
        	res->type =
			(res->flags & WPRINTF_LONG)
				? WPR_WCHAR
				: WPR_CHAR;
		break;
		
	case 'C':
        	res->type =
			(res->flags & WPRINTF_SHORT)
				? WPR_CHAR
				: WPR_WCHAR;
		break;
		
	case 'd':
	case 'i':
        	res->type = WPR_SIGNED;
		break;
		
	case 's':
		res->type =
			(res->flags & (WPRINTF_LONG |WPRINTF_WIDE)) 
				? WPR_WSTRING
				: WPR_STRING;
		break;
		
	case 'S':
		res->type =
			(res->flags & (WPRINTF_SHORT|WPRINTF_WIDE))
				? WPR_STRING
				: WPR_WSTRING;
		break;
		
	case 'u':
		res->type = WPR_UNSIGNED;
		break;
		
	case 'X':
		res->flags |= WPRINTF_UPPER_HEX;
		/* fall through */
		
	case 'x':
		res->type = WPR_HEXA;
		break;
		
	default: /* unknown format char */
		res->type = WPR_UNKNOWN;
		p--;  /* print format as normal char */
		break;

	} /* switch */
	
	return (INT) (p - format) + 1;
}


/***********************************************************************
 * NAME								PRIVATE
 *	wvsnprintfA   (Not a Windows API)
 */
static
INT
STDCALL
wvsnprintfA(
	LPSTR	buffer,
	UINT	maxlen,
	LPCSTR	spec,
	va_list	args
	)
{
	WPRINTF_FORMAT	format;
	LPSTR		p = buffer;
	UINT		i;
	UINT		len;
	CHAR		number [20];

	while (*spec && (maxlen > 1))
	{
		if (*spec != '%')
		{
			*p++ = *spec++;
			maxlen--;
			continue;
		}
		spec++;
		if (*spec == '%')
		{
			*p++ = *spec++;
			maxlen--;
			continue;
		}
		spec += WPRINTF_ParseFormatA(
				spec,
				& format
				);
		len = WPRINTF_GetLen(
				& format,
				args,
				number,
				(maxlen - 1)
				);
		if (!(format.flags & WPRINTF_LEFTALIGN))
		{
			for (	i = format.precision;
				(i < format.width);
				i++, maxlen--
				)
			{
				*p++ = ' ';
			}
		}
		switch (format.type)
		{
		case WPR_WCHAR:
			if ((*p = (CHAR) (WCHAR) va_arg( args, int)))
			{
				p++;
			}
			else if (format.width > 1)
			{
				*p++ = ' ';
			}
			else
			{
				len = 0;
			}
			break;
			
		case WPR_CHAR:
			if ((*p = (CHAR) va_arg( args, int )))
			{
				p++;
			}
			else if (format.width > 1)
			{
				*p++ = ' ';
			}
			else
			{
				len = 0;
			}
			break;
			
		case WPR_STRING:
			memcpy(
				p,
				va_arg( args, LPCSTR ),
				len
				);
			p += len;
			break;
			
		case WPR_WSTRING:
		{
			LPCWSTR ptr = va_arg( args, LPCWSTR );
			
			for (	i = 0;
				(i < len);
				i++
				)
			{
				*p++ = (CHAR) *ptr++;
			}
		}
			break;
		
		case WPR_HEXA:
			if (	(format.flags & WPRINTF_PREFIX_HEX)
				&& (maxlen > 3)
				)
			{
				*p++ = '0';
				*p++ = (format.flags & WPRINTF_UPPER_HEX)
					? 'X'
					: 'x';
				maxlen -= 2;
				len -= 2;
				format.precision -= 2;
				format.width -= 2;
			}
			/* fall through */
			
		case WPR_SIGNED:
		case WPR_UNSIGNED:
			for (	i = len;
				(i < format.precision);
				i++, maxlen--
				)
			{
				*p++ = '0';
			}
			memcpy(
				p,
				number,
				len
				);
			p += len;
			(void) va_arg( args, INT ); /* Go to the next arg */
			break;
			
		case WPR_UNKNOWN:
			continue;
		} /* switch */
		
		if (format.flags & WPRINTF_LEFTALIGN)
		{
			for (	i = format.precision;
				(i < format.width);
				i++, maxlen--
				)
			{
				*p++ = ' ';
			}
		}
		maxlen -= len;
	} /* while */
	
	*p = '\0';
	return (maxlen > 1)
		? (INT) (p - buffer)
		: -1;
}


/***********************************************************************
 * NAME								PUBLIC
 *	wsprintfA   (USER32.585)
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 */
INT
CDECL
wsprintfA(
	LPSTR	buffer,
	LPCSTR	spec,
	...
	)
{
	va_list	valist;
	INT	res;

	va_start( valist, spec );
	res = wvsnprintfA(
			buffer,
			0xffffffff,
			spec,
			valist
			);
	va_end( valist );
	return res;
}


/***********************************************************************
 * NAME								PUBLIC
 *	wvsprintfA   (USER32.587)
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 */
INT
STDCALL
wvsprintfA(
	LPSTR	buffer,
	LPCSTR	spec,
	va_list	args
	)
{
	return wvsnprintfA(
			buffer,
			0xffffffff,
			spec,
			args
			);
}


/* === UNICODE VERSION === */


/***********************************************************************
 * NAME								PRIVATE
 *	WPRINTF_ParseFormatW
 *
 * DESCRIPTION
 *	Parse a format specification. A format specification has
 *	the form:
 *
 *	[-][#][0][width][.precision]type
 *
 * ARGUMENTS
 *	format
 *		?
 *	res
 *		?
 *
 * RETURN VALUE
 *	The length of the format specification in characters.
 */
static
INT
WPRINTF_ParseFormatW(
	LPCWSTR		format,
	WPRINTF_FORMAT	*res
	)
{
	LPCWSTR p = format;

	res->flags = 0;
	res->width = 0;
	res->precision = 0;
	if (*p == L'-')
	{
		res->flags |= WPRINTF_LEFTALIGN;
		p++;
	}
	if (*p == L'#')
	{
		res->flags |= WPRINTF_PREFIX_HEX;
		p++;
	}
	if (*p == L'0')
	{
		res->flags |= WPRINTF_ZEROPAD;
		p++;
	}
	
	while ((*p >= L'0') && (*p <= L'9'))  /* width field */
	{
		res->width = 
			(res->width * 10)
			+ (*p - L'0');
		p++;
	}

	if (*p == L'.')  /* precision field */
	{
		p++;
		while ((*p >= L'0') && (*p <= L'9'))
		{
			res->precision =
				(res->precision * 10)
				+ (*p - '0');
			p++;
		}
	}
	if (*p == L'l')
	{
		res->flags |= WPRINTF_LONG;
		p++;
	}
	else if (*p == L'h')
	{
		res->flags |= WPRINTF_SHORT;
		p++;
	}
	else if (*p == L'w')
	{
		res->flags |= WPRINTF_WIDE;
		p++;
	}
	
	switch ((CHAR)*p)
	{
	case L'c':
		res->type =
			(res->flags & WPRINTF_SHORT)
				? WPR_CHAR
				: WPR_WCHAR;
		break;
		
	case L'C':
        	res->type =
			(res->flags & WPRINTF_LONG)
				? WPR_WCHAR
				: WPR_CHAR;
		break;
		
	case L'd':
	case L'i':
		res->type = WPR_SIGNED;
		break;
		
	case L's':
        	res->type =
			((res->flags & WPRINTF_SHORT)
			 && !(res->flags & WPRINTF_WIDE)
			)
				? WPR_STRING
				: WPR_WSTRING;
		break;
		
	case L'S':
		res->type =
			(res->flags & (WPRINTF_LONG | WPRINTF_WIDE))
				? WPR_WSTRING
				: WPR_STRING;
		break;
		
	case L'u':
		res->type = WPR_UNSIGNED;
		break;
		
	case L'X':
		res->flags |= WPRINTF_UPPER_HEX;
		/* fall through */
		
	case L'x':
		res->type = WPR_HEXA;
		break;
		
	default:
		res->type = WPR_UNKNOWN;
		p--;  /* print format as normal char */
		break;
	} /* switch */
	
	return (INT) (p - format) + 1;
}


/***********************************************************************
 * NAME								PRIVATE
 *           wvsnprintfW   (Not a Windows API)
 */
static
INT
wvsnprintfW(
	LPWSTR	buffer,
	UINT	maxlen,
	LPCWSTR	spec,
	va_list	args
	)
{
	WPRINTF_FORMAT	format;
	LPWSTR		p = buffer;
	UINT		i;
	UINT		len;
	CHAR		number [20];

	while (*spec && (maxlen > 1))
	{
        	if (*spec != L'%')
		{
			*p++ = *spec++;
			maxlen--;
			continue;
		}
		spec++;
		if (*spec == L'%')
		{
			*p++ = *spec++;
			maxlen--;
			continue;
		}
		spec += WPRINTF_ParseFormatW(
				spec,
				& format
				);
		len = WPRINTF_GetLen(
				& format,
				args,
				number,
				(maxlen - 1)
				);
		if (!(format.flags & WPRINTF_LEFTALIGN))
		{
			for (	i = format.precision;
				(i < format.width);
				i++, maxlen--
				)
			{
				*p++ = L' ';
			}
		}
		switch (format.type)
		{
		case WPR_WCHAR:
			if ((*p = (WCHAR) va_arg( args, int)))
			{
				p++;
			}
			else if (format.width > 1)
			{
				*p++ = L' ';
			}
			else
			{
				len = 0;
			}
			break;
			
		case WPR_CHAR:
			if ((*p = (WCHAR)(CHAR) va_arg( args, int )))
			{
				p++;
			}
			else if (format.width > 1)
			{
				*p++ = L' ';
			}
			else
			{
				len = 0;
			}
			break;
			
		case WPR_STRING:
		{
			LPCSTR ptr = va_arg( args, LPCSTR );
			
			for (	i = 0;
				(i < len);
				i++
				)
			{
				*p++ = (WCHAR) *ptr++;
			}
		}
			break;
			
		case WPR_WSTRING:
			if (len)
			{
				memcpy(
					p,
					va_arg( args, LPCWSTR ),
					(len * sizeof (WCHAR))
					);
			}
			p += len;
			break;
			
		case WPR_HEXA:
			if (	(format.flags & WPRINTF_PREFIX_HEX)
				&& (maxlen > 3)
				)
			{
				*p++ = L'0';
				*p++ = (format.flags & WPRINTF_UPPER_HEX)
					? L'X'
					: L'x';
				maxlen -= 2;
				len -= 2;
				format.precision -= 2;
				format.width -= 2;
			}
			/* fall through */
			
		case WPR_SIGNED:
		case WPR_UNSIGNED:
			for (	i = len;
				(i < format.precision);
				i++, maxlen--
				)
			{
				*p++ = L'0';
			}
			for (	i = 0;
				(i < len);
				i++
				)
			{
				*p++ = (WCHAR) number[i];
			}
			(void) va_arg( args, INT ); /* Go to the next arg */
			break;
			
		case WPR_UNKNOWN:
			continue;
		} /* switch */

		if (format.flags & WPRINTF_LEFTALIGN)
		{
			for (	i = format.precision;
				(i < format.width);
				i++, maxlen--
				)
			{
				*p++ = L' ';
			}
		}
		maxlen -= len;
	} /* while */
	
	*p = L'\0';
	return (maxlen > 1)
		? (INT) (p - buffer)
		: -1;
}


/***********************************************************************
 * NAME								PUBLIC
 *	wsprintfW   (USER32.586)
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 */
INT
CDECL
wsprintfW(
	LPWSTR	buffer,
	LPCWSTR	spec,
	...
	)
{
	va_list	valist;
	INT	res;

	va_start( valist, spec );
	res = wvsnprintfW(
			buffer,
			0xffffffff,
			spec,
			valist
			);
	va_end( valist );
	return res;
}


/***********************************************************************
 * NAME								PUBLIC
 *           wvsprintfW   (USER32.588)
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 */
INT
STDCALL
wvsprintfW(
	LPWSTR	buffer,
	LPCWSTR	spec,
	va_list	args
	)
{
	return wvsnprintfW(
			buffer,
			0xffffffff,
			spec,
			args
			);
}


/* EOF */
