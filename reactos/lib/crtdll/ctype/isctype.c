/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <ctype.h>


#define __dj_ISUPPER	_UPPER
#define __dj_ISLOWER	_LOWER
#define __dj_ISDIGIT	_DIGIT	
#define __dj_ISSPACE	_SPACE
#define __dj_ISPUNCT	_PUNCT
#define __dj_ISCNTRL	_CONTROL
#define __dj_ISBLANK	_BLANK	
#define __dj_ISXDIGIT	_HEX
#define __dj_ISALPHA	_ALPHA
#define __dj_ISPRINT	_PRINT
#define __dj_ISALNUM 	( _ALPHA | _DIGIT )
#define __dj_ISGRAPH	_GRAPH

int __mb_cur_max = 2;

// removed the first value

unsigned short _pctype_dll[] = {
  __dj_ISCNTRL,											/* CTRL+@, 0x00 */
  __dj_ISCNTRL,											/* CTRL+A, 0x01 */
  __dj_ISCNTRL,											/* CTRL+B, 0x02 */
  __dj_ISCNTRL,											/* CTRL+C, 0x03 */
  __dj_ISCNTRL,											/* CTRL+D, 0x04 */
  __dj_ISCNTRL,											/* CTRL+E, 0x05 */
  __dj_ISCNTRL,											/* CTRL+F, 0x06 */
  __dj_ISCNTRL,											/* CTRL+G, 0x07 */
  __dj_ISCNTRL,											/* CTRL+H, 0x08 */
  __dj_ISCNTRL | __dj_ISSPACE,									/* CTRL+I, 0x09 */
  __dj_ISCNTRL | __dj_ISSPACE,									/* CTRL+J, 0x0a */
  __dj_ISCNTRL | __dj_ISSPACE,									/* CTRL+K, 0x0b */
  __dj_ISCNTRL | __dj_ISSPACE,									/* CTRL+L, 0x0c */
  __dj_ISCNTRL | __dj_ISSPACE,									/* CTRL+M, 0x0d */
  __dj_ISCNTRL,											/* CTRL+N, 0x0e */
  __dj_ISCNTRL,											/* CTRL+O, 0x0f */
  __dj_ISCNTRL,											/* CTRL+P, 0x10 */
  __dj_ISCNTRL,											/* CTRL+Q, 0x11 */
  __dj_ISCNTRL,											/* CTRL+R, 0x12 */
  __dj_ISCNTRL,											/* CTRL+S, 0x13 */
  __dj_ISCNTRL,											/* CTRL+T, 0x14 */
  __dj_ISCNTRL,											/* CTRL+U, 0x15 */
  __dj_ISCNTRL,											/* CTRL+V, 0x16 */
  __dj_ISCNTRL,											/* CTRL+W, 0x17 */
  __dj_ISCNTRL,											/* CTRL+X, 0x18 */
  __dj_ISCNTRL,											/* CTRL+Y, 0x19 */
  __dj_ISCNTRL,											/* CTRL+Z, 0x1a */
  __dj_ISCNTRL,											/* CTRL+[, 0x1b */
  __dj_ISCNTRL,											/* CTRL+\, 0x1c */
  __dj_ISCNTRL,											/* CTRL+], 0x1d */
  __dj_ISCNTRL,											/* CTRL+^, 0x1e */
  __dj_ISCNTRL,											/* CTRL+_, 0x1f */
  __dj_ISPRINT | __dj_ISSPACE,									/* ` ', 0x20 */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `!', 0x21 */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* 0x22 */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `#', 0x23 */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `$', 0x24 */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `%', 0x25 */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `&', 0x26 */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* 0x27 */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `(', 0x28 */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `)', 0x29 */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `*', 0x2a */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `+', 0x2b */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `,', 0x2c */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `-', 0x2d */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `.', 0x2e */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `/', 0x2f */
  __dj_ISALNUM | __dj_ISDIGIT | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISXDIGIT,			/* `0', 0x30 */
  __dj_ISALNUM | __dj_ISDIGIT | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISXDIGIT,			/* `1', 0x31 */
  __dj_ISALNUM | __dj_ISDIGIT | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISXDIGIT,			/* `2', 0x32 */
  __dj_ISALNUM | __dj_ISDIGIT | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISXDIGIT,			/* `3', 0x33 */
  __dj_ISALNUM | __dj_ISDIGIT | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISXDIGIT,			/* `4', 0x34 */
  __dj_ISALNUM | __dj_ISDIGIT | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISXDIGIT,			/* `5', 0x35 */
  __dj_ISALNUM | __dj_ISDIGIT | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISXDIGIT,			/* `6', 0x36 */
  __dj_ISALNUM | __dj_ISDIGIT | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISXDIGIT,			/* `7', 0x37 */
  __dj_ISALNUM | __dj_ISDIGIT | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISXDIGIT,			/* `8', 0x38 */
  __dj_ISALNUM | __dj_ISDIGIT | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISXDIGIT,			/* `9', 0x39 */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `:', 0x3a */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `;', 0x3b */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `<', 0x3c */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `=', 0x3d */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `>', 0x3e */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `?', 0x3f */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `@', 0x40 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISUPPER | __dj_ISXDIGIT,		/* `A', 0x41 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISUPPER | __dj_ISXDIGIT,		/* `B', 0x42 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISUPPER | __dj_ISXDIGIT,		/* `C', 0x43 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISUPPER | __dj_ISXDIGIT,		/* `D', 0x44 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISUPPER | __dj_ISXDIGIT,		/* `E', 0x45 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISUPPER | __dj_ISXDIGIT,		/* `F', 0x46 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISUPPER,			/* `G', 0x47 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISUPPER,			/* `H', 0x48 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISUPPER,			/* `I', 0x49 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISUPPER,			/* `J', 0x4a */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISUPPER,			/* `K', 0x4b */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISUPPER,			/* `L', 0x4c */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISUPPER,			/* `M', 0x4d */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISUPPER,			/* `N', 0x4e */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISUPPER,			/* `O', 0x4f */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISUPPER,			/* `P', 0x50 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISUPPER,			/* `Q', 0x51 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISUPPER,			/* `R', 0x52 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISUPPER,			/* `S', 0x53 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISUPPER,			/* `T', 0x54 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISUPPER,			/* `U', 0x55 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISUPPER,			/* `V', 0x56 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISUPPER,			/* `W', 0x57 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISUPPER,			/* `X', 0x58 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISUPPER,			/* `Y', 0x59 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISPRINT | __dj_ISUPPER,			/* `Z', 0x5a */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `[', 0x5b */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* 0x5c */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `]', 0x5d */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `^', 0x5e */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `_', 0x5f */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* 0x60 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISLOWER | __dj_ISPRINT | __dj_ISXDIGIT,		/* `a', 0x61 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISLOWER | __dj_ISPRINT | __dj_ISXDIGIT,		/* `b', 0x62 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISLOWER | __dj_ISPRINT | __dj_ISXDIGIT,		/* `c', 0x63 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISLOWER | __dj_ISPRINT | __dj_ISXDIGIT,		/* `d', 0x64 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISLOWER | __dj_ISPRINT | __dj_ISXDIGIT,		/* `e', 0x65 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISLOWER | __dj_ISPRINT | __dj_ISXDIGIT,		/* `f', 0x66 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISLOWER | __dj_ISPRINT,			/* `g', 0x67 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISLOWER | __dj_ISPRINT,			/* `h', 0x68 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISLOWER | __dj_ISPRINT,			/* `i', 0x69 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISLOWER | __dj_ISPRINT,			/* `j', 0x6a */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISLOWER | __dj_ISPRINT,			/* `k', 0x6b */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISLOWER | __dj_ISPRINT,			/* `l', 0x6c */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISLOWER | __dj_ISPRINT,			/* `m', 0x6d */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISLOWER | __dj_ISPRINT,			/* `n', 0x6e */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISLOWER | __dj_ISPRINT,			/* `o', 0x6f */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISLOWER | __dj_ISPRINT,			/* `p', 0x70 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISLOWER | __dj_ISPRINT,			/* `q', 0x71 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISLOWER | __dj_ISPRINT,			/* `r', 0x72 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISLOWER | __dj_ISPRINT,			/* `s', 0x73 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISLOWER | __dj_ISPRINT,			/* `t', 0x74 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISLOWER | __dj_ISPRINT,			/* `u', 0x75 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISLOWER | __dj_ISPRINT,			/* `v', 0x76 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISLOWER | __dj_ISPRINT,			/* `w', 0x77 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISLOWER | __dj_ISPRINT,			/* `x', 0x78 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISLOWER | __dj_ISPRINT,			/* `y', 0x79 */
  __dj_ISALNUM | __dj_ISALPHA | __dj_ISGRAPH | __dj_ISLOWER | __dj_ISPRINT,			/* `z', 0x7a */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `{', 0x7b */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `|', 0x7c */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `}', 0x7d */
  __dj_ISGRAPH | __dj_ISPRINT | __dj_ISPUNCT,							/* `~', 0x7e */
  __dj_ISCNTRL,											/* 0x7f */
  0,												/* 0x80 */
  0,												/* 0x81 */
  0,												/* 0x82 */
  0,												/* 0x83 */
  0,												/* 0x84 */
  0,												/* 0x85 */
  0,												/* 0x86 */
  0,												/* 0x87 */
  0,												/* 0x88 */
  0,												/* 0x89 */
  0,												/* 0x8a */
  0,												/* 0x8b */
  0,												/* 0x8c */
  0,												/* 0x8d */
  0,												/* 0x8e */
  0,												/* 0x8f */
  0,												/* 0x90 */
  0,												/* 0x91 */
  0,												/* 0x92 */
  0,												/* 0x93 */
  0,												/* 0x94 */
  0,												/* 0x95 */
  0,												/* 0x96 */
  0,												/* 0x97 */
  0,												/* 0x98 */
  0,												/* 0x99 */
  0,												/* 0x9a */
  0,												/* 0x9b */
  0,												/* 0x9c */
  0,												/* 0x9d */
  0,												/* 0x9e */
  0,												/* 0x9f */
  0,												/* 0xa0 */
  0,												/* 0xa1 */
  0,												/* 0xa2 */
  0,												/* 0xa3 */
  0,												/* 0xa4 */
  0,												/* 0xa5 */
  0,												/* 0xa6 */
  0,												/* 0xa7 */
  0,												/* 0xa8 */
  0,												/* 0xa9 */
  0,												/* 0xaa */
  0,												/* 0xab */
  0,												/* 0xac */
  0,												/* 0xad */
  0,												/* 0xae */
  0,												/* 0xaf */
  0,												/* 0xb0 */
  0,												/* 0xb1 */
  0,												/* 0xb2 */
  0,												/* 0xb3 */
  0,												/* 0xb4 */
  0,												/* 0xb5 */
  0,												/* 0xb6 */
  0,												/* 0xb7 */
  0,												/* 0xb8 */
  0,												/* 0xb9 */
  0,												/* 0xba */
  0,												/* 0xbb */
  0,												/* 0xbc */
  0,												/* 0xbd */
  0,												/* 0xbe */
  0,												/* 0xbf */
  0,												/* 0xc0 */
  0,												/* 0xc1 */
  0,												/* 0xc2 */
  0,												/* 0xc3 */
  0,												/* 0xc4 */
  0,												/* 0xc5 */
  0,												/* 0xc6 */
  0,												/* 0xc7 */
  0,												/* 0xc8 */
  0,												/* 0xc9 */
  0,												/* 0xca */
  0,												/* 0xcb */
  0,												/* 0xcc */
  0,												/* 0xcd */
  0,												/* 0xce */
  0,												/* 0xcf */
  0,												/* 0xd0 */
  0,												/* 0xd1 */
  0,												/* 0xd2 */
  0,												/* 0xd3 */
  0,												/* 0xd4 */
  0,												/* 0xd5 */
  0,												/* 0xd6 */
  0,												/* 0xd7 */
  0,												/* 0xd8 */
  0,												/* 0xd9 */
  0,												/* 0xda */
  0,												/* 0xdb */
  0,												/* 0xdc */
  0,												/* 0xdd */
  0,												/* 0xde */
  0,												/* 0xdf */
  0,												/* 0xe0 */
  0,												/* 0xe1 */
  0,												/* 0xe2 */
  0,												/* 0xe3 */
  0,												/* 0xe4 */
  0,												/* 0xe5 */
  0,												/* 0xe6 */
  0,												/* 0xe7 */
  0,												/* 0xe8 */
  0,												/* 0xe9 */
  0,												/* 0xea */
  0,												/* 0xeb */
  0,												/* 0xec */
  0,												/* 0xed */
  0,												/* 0xee */
  0,												/* 0xef */
  0,												/* 0xf0 */
  0,												/* 0xf1 */
  0,												/* 0xf2 */
  0,												/* 0xf3 */
  0,												/* 0xf4 */
  0,												/* 0xf5 */
  0,												/* 0xf6 */
  0,												/* 0xf7 */
  0,												/* 0xf8 */
  0,												/* 0xf9 */
  0,												/* 0xfa */
  0,												/* 0xfb */
  0,												/* 0xfc */
  0,												/* 0xfd */
  0,												/* 0xfe */
  0,												/* 0xff */
};

unsigned short *_ctype = _pctype_dll -1; // unused
unsigned short *_pwctype_dll = _pctype_dll;


int _isctype(unsigned char c, int t)
{		

	return ((_pctype_dll[(c & 0xFF)]&t) == t );
}

int iswctype(unsigned short c, int t)
{		

	return ((_pwctype_dll[(c & 0xFF)]&t) == t );
}

// obsolete
int is_wctype(unsigned short c, int t)
{		

	return ((_pctype_dll[(c & 0xFF)]&t) == t );
}


