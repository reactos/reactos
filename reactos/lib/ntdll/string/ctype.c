/* $Id: ctype.c,v 1.5 1999/12/29 17:13:14 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/rtl/ctype.c
 * PURPOSE:         Character type and conversion functions
 * PROGRAMMERS:     ???
 *                  Eric Kohl
 * HISTORY:         ???: Created
 *                  29/12/1999: Added missing functions and changed
 *                              all functions to use ctype table
 */

#include <ctype.h>

#define upalpha ('A' - 'a')

#define _GRAPH 0x0200
#define _PRINT 0x0400


unsigned short _pctype_dll[] = {
   _CONTROL,					/* CTRL+@, 0x00 */
   _CONTROL,					/* CTRL+A, 0x01 */
   _CONTROL,					/* CTRL+B, 0x02 */
   _CONTROL,					/* CTRL+C, 0x03 */
   _CONTROL,					/* CTRL+D, 0x04 */
   _CONTROL,					/* CTRL+E, 0x05 */
   _CONTROL,					/* CTRL+F, 0x06 */
   _CONTROL,					/* CTRL+G, 0x07 */
   _CONTROL,					/* CTRL+H, 0x08 */
   _CONTROL | _SPACE,				/* CTRL+I, 0x09 */
   _CONTROL | _SPACE,				/* CTRL+J, 0x0a */
   _CONTROL | _SPACE,				/* CTRL+K, 0x0b */
   _CONTROL | _SPACE,				/* CTRL+L, 0x0c */
   _CONTROL | _SPACE,				/* CTRL+M, 0x0d */
   _CONTROL,					/* CTRL+N, 0x0e */
   _CONTROL,					/* CTRL+O, 0x0f */
   _CONTROL,					/* CTRL+P, 0x10 */
   _CONTROL,					/* CTRL+Q, 0x11 */
   _CONTROL,					/* CTRL+R, 0x12 */
   _CONTROL,					/* CTRL+S, 0x13 */
   _CONTROL,					/* CTRL+T, 0x14 */
   _CONTROL,					/* CTRL+U, 0x15 */
   _CONTROL,					/* CTRL+V, 0x16 */
   _CONTROL,					/* CTRL+W, 0x17 */
   _CONTROL,					/* CTRL+X, 0x18 */
   _CONTROL,					/* CTRL+Y, 0x19 */
   _CONTROL,					/* CTRL+Z, 0x1a */
   _CONTROL,					/* CTRL+[, 0x1b */
   _CONTROL,					/* CTRL+\, 0x1c */
   _CONTROL,					/* CTRL+], 0x1d */
   _CONTROL,					/* CTRL+^, 0x1e */
   _CONTROL,					/* CTRL+_, 0x1f */
   _PRINT | _SPACE,				/* ` ', 0x20 */
   _GRAPH | _PRINT | _PUNCT,			/* `!', 0x21 */
   _GRAPH | _PRINT | _PUNCT,			/* 0x22 */
   _GRAPH | _PRINT | _PUNCT,			/* `#', 0x23 */
   _GRAPH | _PRINT | _PUNCT,			/* `$', 0x24 */
   _GRAPH | _PRINT | _PUNCT,			/* `%', 0x25 */
   _GRAPH | _PRINT | _PUNCT,			/* `&', 0x26 */
   _GRAPH | _PRINT | _PUNCT,			/* 0x27 */
   _GRAPH | _PRINT | _PUNCT,			/* `(', 0x28 */
   _GRAPH | _PRINT | _PUNCT,			/* `)', 0x29 */
   _GRAPH | _PRINT | _PUNCT,			/* `*', 0x2a */
   _GRAPH | _PRINT | _PUNCT,			/* `+', 0x2b */
   _GRAPH | _PRINT | _PUNCT,			/* `,', 0x2c */
   _GRAPH | _PRINT | _PUNCT,			/* `-', 0x2d */
   _GRAPH | _PRINT | _PUNCT,			/* `.', 0x2e */
   _GRAPH | _PRINT | _PUNCT,			/* `/', 0x2f */
   _DIGIT | _GRAPH | _PRINT | _HEX,		/* `0', 0x30 */
   _DIGIT | _GRAPH | _PRINT | _HEX,		/* `1', 0x31 */
   _DIGIT | _GRAPH | _PRINT | _HEX,		/* `2', 0x32 */
   _DIGIT | _GRAPH | _PRINT | _HEX,		/* `3', 0x33 */
   _DIGIT | _GRAPH | _PRINT | _HEX,		/* `4', 0x34 */
   _DIGIT | _GRAPH | _PRINT | _HEX,		/* `5', 0x35 */
   _DIGIT | _GRAPH | _PRINT | _HEX,		/* `6', 0x36 */
   _DIGIT | _GRAPH | _PRINT | _HEX,		/* `7', 0x37 */
   _DIGIT | _GRAPH | _PRINT | _HEX,		/* `8', 0x38 */
   _DIGIT | _GRAPH | _PRINT | _HEX,		/* `9', 0x39 */
   _GRAPH | _PRINT | _PUNCT,			/* `:', 0x3a */
   _GRAPH | _PRINT | _PUNCT,			/* `;', 0x3b */
   _GRAPH | _PRINT | _PUNCT,			/* `<', 0x3c */
   _GRAPH | _PRINT | _PUNCT,			/* `=', 0x3d */
   _GRAPH | _PRINT | _PUNCT,			/* `>', 0x3e */
   _GRAPH | _PRINT | _PUNCT,			/* `?', 0x3f */
   _GRAPH | _PRINT | _PUNCT,			/* `@', 0x40 */
   _ALPHA | _GRAPH | _PRINT | _UPPER | _HEX,	/* `A', 0x41 */
   _ALPHA | _GRAPH | _PRINT | _UPPER | _HEX,	/* `B', 0x42 */
   _ALPHA | _GRAPH | _PRINT | _UPPER | _HEX,	/* `C', 0x43 */
   _ALPHA | _GRAPH | _PRINT | _UPPER | _HEX,	/* `D', 0x44 */
   _ALPHA | _GRAPH | _PRINT | _UPPER | _HEX,	/* `E', 0x45 */
   _ALPHA | _GRAPH | _PRINT | _UPPER | _HEX,	/* `F', 0x46 */
   _ALPHA | _GRAPH | _PRINT | _UPPER,		/* `G', 0x47 */
   _ALPHA | _GRAPH | _PRINT | _UPPER,		/* `H', 0x48 */
   _ALPHA | _GRAPH | _PRINT | _UPPER,		/* `I', 0x49 */
   _ALPHA | _GRAPH | _PRINT | _UPPER,		/* `J', 0x4a */
   _ALPHA | _GRAPH | _PRINT | _UPPER,		/* `K', 0x4b */
   _ALPHA | _GRAPH | _PRINT | _UPPER,		/* `L', 0x4c */
   _ALPHA | _GRAPH | _PRINT | _UPPER,		/* `M', 0x4d */
   _ALPHA | _GRAPH | _PRINT | _UPPER,		/* `N', 0x4e */
   _ALPHA | _GRAPH | _PRINT | _UPPER,		/* `O', 0x4f */
   _ALPHA | _GRAPH | _PRINT | _UPPER,		/* `P', 0x50 */
   _ALPHA | _GRAPH | _PRINT | _UPPER,		/* `Q', 0x51 */
   _ALPHA | _GRAPH | _PRINT | _UPPER,		/* `R', 0x52 */
   _ALPHA | _GRAPH | _PRINT | _UPPER,		/* `S', 0x53 */
   _ALPHA | _GRAPH | _PRINT | _UPPER,		/* `T', 0x54 */
   _ALPHA | _GRAPH | _PRINT | _UPPER,		/* `U', 0x55 */
   _ALPHA | _GRAPH | _PRINT | _UPPER,		/* `V', 0x56 */
   _ALPHA | _GRAPH | _PRINT | _UPPER,		/* `W', 0x57 */
   _ALPHA | _GRAPH | _PRINT | _UPPER,		/* `X', 0x58 */
   _ALPHA | _GRAPH | _PRINT | _UPPER,		/* `Y', 0x59 */
   _ALPHA | _GRAPH | _PRINT | _UPPER,		/* `Z', 0x5a */
   _GRAPH | _PRINT | _PUNCT,			/* `[', 0x5b */
   _GRAPH | _PRINT | _PUNCT,			/* 0x5c */
   _GRAPH | _PRINT | _PUNCT,			/* `]', 0x5d */
   _GRAPH | _PRINT | _PUNCT,			/* `^', 0x5e */
   _GRAPH | _PRINT | _PUNCT,			/* `_', 0x5f */
   _GRAPH | _PRINT | _PUNCT,			/* 0x60 */
   _ALPHA | _GRAPH | _LOWER | _PRINT | _HEX,	/* `a', 0x61 */
   _ALPHA | _GRAPH | _LOWER | _PRINT | _HEX,	/* `b', 0x62 */
   _ALPHA | _GRAPH | _LOWER | _PRINT | _HEX,	/* `c', 0x63 */
   _ALPHA | _GRAPH | _LOWER | _PRINT | _HEX,	/* `d', 0x64 */
   _ALPHA | _GRAPH | _LOWER | _PRINT | _HEX,	/* `e', 0x65 */
   _ALPHA | _GRAPH | _LOWER | _PRINT | _HEX,	/* `f', 0x66 */
   _ALPHA | _GRAPH | _LOWER | _PRINT,		/* `g', 0x67 */
   _ALPHA | _GRAPH | _LOWER | _PRINT,		/* `h', 0x68 */
   _ALPHA | _GRAPH | _LOWER | _PRINT,		/* `i', 0x69 */
   _ALPHA | _GRAPH | _LOWER | _PRINT,		/* `j', 0x6a */
   _ALPHA | _GRAPH | _LOWER | _PRINT,		/* `k', 0x6b */
   _ALPHA | _GRAPH | _LOWER | _PRINT,		/* `l', 0x6c */
   _ALPHA | _GRAPH | _LOWER | _PRINT,		/* `m', 0x6d */
   _ALPHA | _GRAPH | _LOWER | _PRINT,		/* `n', 0x6e */
   _ALPHA | _GRAPH | _LOWER | _PRINT,		/* `o', 0x6f */
   _ALPHA | _GRAPH | _LOWER | _PRINT,		/* `p', 0x70 */
   _ALPHA | _GRAPH | _LOWER | _PRINT,		/* `q', 0x71 */
   _ALPHA | _GRAPH | _LOWER | _PRINT,		/* `r', 0x72 */
   _ALPHA | _GRAPH | _LOWER | _PRINT,		/* `s', 0x73 */
   _ALPHA | _GRAPH | _LOWER | _PRINT,		/* `t', 0x74 */
   _ALPHA | _GRAPH | _LOWER | _PRINT,		/* `u', 0x75 */
   _ALPHA | _GRAPH | _LOWER | _PRINT,		/* `v', 0x76 */
   _ALPHA | _GRAPH | _LOWER | _PRINT,		/* `w', 0x77 */
   _ALPHA | _GRAPH | _LOWER | _PRINT,		/* `x', 0x78 */
   _ALPHA | _GRAPH | _LOWER | _PRINT,		/* `y', 0x79 */
   _ALPHA | _GRAPH | _LOWER | _PRINT,		/* `z', 0x7a */
   _GRAPH | _PRINT | _PUNCT,			/* `{', 0x7b */
   _GRAPH | _PRINT | _PUNCT,			/* `|', 0x7c */
   _GRAPH | _PRINT | _PUNCT,			/* `}', 0x7d */
   _GRAPH | _PRINT | _PUNCT,			/* `~', 0x7e */
   _CONTROL,					/* 0x7f */
   0,						/* 0x80 */
   0,						/* 0x81 */
   0,						/* 0x82 */
   0,						/* 0x83 */
   0,						/* 0x84 */
   0,						/* 0x85 */
   0,						/* 0x86 */
   0,						/* 0x87 */
   0,						/* 0x88 */
   0,						/* 0x89 */
   0,						/* 0x8a */
   0,						/* 0x8b */
   0,						/* 0x8c */
   0,						/* 0x8d */
   0,						/* 0x8e */
   0,						/* 0x8f */
   0,						/* 0x90 */
   0,						/* 0x91 */
   0,						/* 0x92 */
   0,						/* 0x93 */
   0,						/* 0x94 */
   0,						/* 0x95 */
   0,						/* 0x96 */
   0,						/* 0x97 */
   0,						/* 0x98 */
   0,						/* 0x99 */
   0,						/* 0x9a */
   0,						/* 0x9b */
   0,						/* 0x9c */
   0,						/* 0x9d */
   0,						/* 0x9e */
   0,						/* 0x9f */
   0,						/* 0xa0 */
   0,						/* 0xa1 */
   0,						/* 0xa2 */
   0,						/* 0xa3 */
   0,						/* 0xa4 */
   0,						/* 0xa5 */
   0,						/* 0xa6 */
   0,						/* 0xa7 */
   0,						/* 0xa8 */
   0,						/* 0xa9 */
   0,						/* 0xaa */
   0,						/* 0xab */
   0,						/* 0xac */
   0,						/* 0xad */
   0,						/* 0xae */
   0,						/* 0xaf */
   0,						/* 0xb0 */
   0,						/* 0xb1 */
   0,						/* 0xb2 */
   0,						/* 0xb3 */
   0,						/* 0xb4 */
   0,						/* 0xb5 */
   0,						/* 0xb6 */
   0,						/* 0xb7 */
   0,						/* 0xb8 */
   0,						/* 0xb9 */
   0,						/* 0xba */
   0,						/* 0xbb */
   0,						/* 0xbc */
   0,						/* 0xbd */
   0,						/* 0xbe */
   0,						/* 0xbf */
   0,						/* 0xc0 */
   0,						/* 0xc1 */
   0,						/* 0xc2 */
   0,						/* 0xc3 */
   0,						/* 0xc4 */
   0,						/* 0xc5 */
   0,						/* 0xc6 */
   0,						/* 0xc7 */
   0,						/* 0xc8 */
   0,						/* 0xc9 */
   0,						/* 0xca */
   0,						/* 0xcb */
   0,						/* 0xcc */
   0,						/* 0xcd */
   0,						/* 0xce */
   0,						/* 0xcf */
   0,						/* 0xd0 */
   0,						/* 0xd1 */
   0,						/* 0xd2 */
   0,						/* 0xd3 */
   0,						/* 0xd4 */
   0,						/* 0xd5 */
   0,						/* 0xd6 */
   0,						/* 0xd7 */
   0,						/* 0xd8 */
   0,						/* 0xd9 */
   0,						/* 0xda */
   0,						/* 0xdb */
   0,						/* 0xdc */
   0,						/* 0xdd */
   0,						/* 0xde */
   0,						/* 0xdf */
   0,						/* 0xe0 */
   0,						/* 0xe1 */
   0,						/* 0xe2 */
   0,						/* 0xe3 */
   0,						/* 0xe4 */
   0,						/* 0xe5 */
   0,						/* 0xe6 */
   0,						/* 0xe7 */
   0,						/* 0xe8 */
   0,						/* 0xe9 */
   0,						/* 0xea */
   0,						/* 0xeb */
   0,						/* 0xec */
   0,						/* 0xed */
   0,						/* 0xee */
   0,						/* 0xef */
   0,						/* 0xf0 */
   0,						/* 0xf1 */
   0,						/* 0xf2 */
   0,						/* 0xf3 */
   0,						/* 0xf4 */
   0,						/* 0xf5 */
   0,						/* 0xf6 */
   0,						/* 0xf7 */
   0,						/* 0xf8 */
   0,						/* 0xf9 */
   0,						/* 0xfa */
   0,						/* 0xfb */
   0,						/* 0xfc */
   0,						/* 0xfd */
   0,						/* 0xfe */
   0,						/* 0xff */
};

unsigned short *_pwctype_dll = _pctype_dll;


int _isctype (int c, int ctypeFlags)
{
   return((_pctype_dll[(unsigned char)(c & 0xFF)]&ctypeFlags) == ctypeFlags);
}

int iswctype(wint_t wc, wctype_t wctypeFlags)
{
   return((_pwctype_dll[(unsigned char)(wc & 0xFF)]&wctypeFlags) == wctypeFlags);
}

int isalpha(int c)
{
   return(_isctype(c, _ALPHA));
}

int isalnum(int c)
{
   return(isalpha(c)||isdigit(c));
}

int __isascii(int c)
{
   return(!((c)&(~0x7f)));
}

int iscntrl(int c)
{
   return(_isctype(c, _CONTROL));
}

int __iscsym(int c)
{
   return(isalnum(c)||(c == '_'));
}

int __iscsymf(int c)
{
   return(isalpha(c)||(c == '_'));
}

int isdigit(int c)
{
   return(_isctype(c, _DIGIT));
}

int isgraph(int c)
{
   return(_isctype(c, _GRAPH));
}

int islower(int c)
{
   return(_isctype(c, _LOWER));
}

int isprint(int c)
{
   return(_isctype(c, _PRINT));
}

int ispunct(int c)
{
   return(_isctype(c, _PUNCT));
}

int isspace(int c)
{
   return(_isctype(c, _SPACE));
}

int isupper(int c)
{
   return(_isctype(c, _UPPER));
}

int iswalpha(wint_t c)
{
   return(iswctype(c, _ALPHA));
}

int isxdigit(int c)
{
   return(_isctype(c, _HEX));
}

int __toascii(int c)
{
   return((unsigned)(c) & 0x7f);
}

int _tolower(int c)
{
   if (_isctype (c, _UPPER))
       return (c - upalpha);
   return(c);
}

int _toupper(int c)
{
   if (_isctype (c, _LOWER))
      return (c + upalpha);
   return(c);
}

int tolower(int c)
{
   if (_isctype (c, _UPPER))
       return (c - upalpha);
   return(c);
}

int toupper(int c)
{
   if (_isctype (c, _LOWER))
      return (c + upalpha);
   return(c);
}

wchar_t towlower(wchar_t c)
{
   if (iswctype (c, _UPPER))
       return (c - upalpha);
   return(c);
}

wchar_t towupper(wchar_t c)
{
   if (iswctype (c, _LOWER))
      return (c + upalpha);
   return(c);
}

/* EOF */
