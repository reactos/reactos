#include <string.h>
#undef __MINGW_IMPORT
#define __MINGW_IMPORT
#include <ctype.h>

#ifndef _LIBCNT_
#include <precomp.h>
#endif

#undef _pctype
#undef _pwctype

/* MS's CRT header defines all that, and we actually implement that */
#undef iswalnum
#undef __isascii
#undef iswascii
#undef __iscsym
#undef __iscsymf
#undef iswalpha
#undef iswcntrl
#undef iswdigit
#undef iswgraph
#undef iswprint
#undef iswpunct
#undef iswlower
#undef iswupper
#undef iswspace
#undef iswxdigit
#undef __toascii
#undef _tolower
#undef _toupper

#define upalpha ('A' - 'a')


const unsigned short _ctype[] = {
	0,			/* <EOF>, 0xFFFF */
	_CONTROL,		/* CTRL+@, 0x00 */
	_CONTROL,		/* CTRL+A, 0x01 */
	_CONTROL,		/* CTRL+B, 0x02 */
	_CONTROL,		/* CTRL+C, 0x03 */
	_CONTROL,		/* CTRL+D, 0x04 */
	_CONTROL,		/* CTRL+E, 0x05 */
	_CONTROL,		/* CTRL+F, 0x06 */
	_CONTROL,		/* CTRL+G, 0x07 */
	_CONTROL,		/* CTRL+H, 0x08 */
	_CONTROL | _SPACE,	/* CTRL+I, 0x09 */
	_CONTROL | _SPACE,	/* CTRL+J, 0x0a */
	_CONTROL | _SPACE,	/* CTRL+K, 0x0b */
	_CONTROL | _SPACE,	/* CTRL+L, 0x0c */
	_CONTROL | _SPACE,	/* CTRL+M, 0x0d */
	_CONTROL,		/* CTRL+N, 0x0e */
	_CONTROL,		/* CTRL+O, 0x0f */
	_CONTROL,		/* CTRL+P, 0x10 */
	_CONTROL,		/* CTRL+Q, 0x11 */
	_CONTROL,		/* CTRL+R, 0x12 */
	_CONTROL,		/* CTRL+S, 0x13 */
	_CONTROL,		/* CTRL+T, 0x14 */
	_CONTROL,		/* CTRL+U, 0x15 */
	_CONTROL,		/* CTRL+V, 0x16 */
	_CONTROL,		/* CTRL+W, 0x17 */
	_CONTROL,		/* CTRL+X, 0x18 */
	_CONTROL,		/* CTRL+Y, 0x19 */
	_CONTROL,		/* CTRL+Z, 0x1a */
	_CONTROL,		/* CTRL+[, 0x1b */
	_CONTROL,		/* CTRL+\, 0x1c */
	_CONTROL,		/* CTRL+], 0x1d */
	_CONTROL,		/* CTRL+^, 0x1e */
	_CONTROL,		/* CTRL+_, 0x1f */
	_SPACE | _BLANK,	/* ` ', 0x20 */
	_PUNCT,			/* `!', 0x21 */
	_PUNCT,			/* 0x22 */
	_PUNCT,			/* `#', 0x23 */
	_PUNCT,			/* `$', 0x24 */
	_PUNCT,			/* `%', 0x25 */
	_PUNCT,			/* `&', 0x26 */
	_PUNCT,			/* 0x27 */
	_PUNCT,			/* `(', 0x28 */
	_PUNCT,			/* `)', 0x29 */
	_PUNCT,			/* `*', 0x2a */
	_PUNCT,			/* `+', 0x2b */
	_PUNCT,			/* `,', 0x2c */
	_PUNCT,			/* `-', 0x2d */
	_PUNCT,			/* `.', 0x2e */
	_PUNCT,			/* `/', 0x2f */
	_DIGIT | _HEX,		/* `0', 0x30 */
	_DIGIT | _HEX,		/* `1', 0x31 */
	_DIGIT | _HEX,		/* `2', 0x32 */
	_DIGIT | _HEX,		/* `3', 0x33 */
	_DIGIT | _HEX,		/* `4', 0x34 */
	_DIGIT | _HEX,		/* `5', 0x35 */
	_DIGIT | _HEX,		/* `6', 0x36 */
	_DIGIT | _HEX,		/* `7', 0x37 */
	_DIGIT | _HEX,		/* `8', 0x38 */
	_DIGIT | _HEX,		/* `9', 0x39 */
	_PUNCT,			/* `:', 0x3a */
	_PUNCT,			/* `;', 0x3b */
	_PUNCT,			/* `<', 0x3c */
	_PUNCT,			/* `=', 0x3d */
	_PUNCT,			/* `>', 0x3e */
	_PUNCT,			/* `?', 0x3f */
	_PUNCT,			/* `@', 0x40 */
	_UPPER | _HEX,		/* `A', 0x41 */
	_UPPER | _HEX,		/* `B', 0x42 */
	_UPPER | _HEX,		/* `C', 0x43 */
	_UPPER | _HEX,		/* `D', 0x44 */
	_UPPER | _HEX,		/* `E', 0x45 */
	_UPPER | _HEX,		/* `F', 0x46 */
	_UPPER,			/* `G', 0x47 */
	_UPPER,			/* `H', 0x48 */
	_UPPER,			/* `I', 0x49 */
	_UPPER,			/* `J', 0x4a */
	_UPPER,			/* `K', 0x4b */
	_UPPER,			/* `L', 0x4c */
	_UPPER,			/* `M', 0x4d */
	_UPPER,			/* `N', 0x4e */
	_UPPER,			/* `O', 0x4f */
	_UPPER,			/* `P', 0x50 */
	_UPPER,			/* `Q', 0x51 */
	_UPPER,			/* `R', 0x52 */
	_UPPER,			/* `S', 0x53 */
	_UPPER,			/* `T', 0x54 */
	_UPPER,			/* `U', 0x55 */
	_UPPER,			/* `V', 0x56 */
	_UPPER,			/* `W', 0x57 */
	_UPPER,			/* `X', 0x58 */
	_UPPER,			/* `Y', 0x59 */
	_UPPER,			/* `Z', 0x5a */
	_PUNCT,			/* `[', 0x5b */
	_PUNCT,			/* 0x5c */
	_PUNCT,			/* `]', 0x5d */
	_PUNCT,			/* `^', 0x5e */
	_PUNCT,			/* `_', 0x5f */
	_PUNCT,			/* 0x60 */
	_LOWER | _HEX,		/* `a', 0x61 */
	_LOWER | _HEX,		/* `b', 0x62 */
	_LOWER | _HEX,		/* `c', 0x63 */
	_LOWER | _HEX,		/* `d', 0x64 */
	_LOWER | _HEX,		/* `e', 0x65 */
	_LOWER | _HEX,		/* `f', 0x66 */
	_LOWER,			/* `g', 0x67 */
	_LOWER,			/* `h', 0x68 */
	_LOWER,			/* `i', 0x69 */
	_LOWER,			/* `j', 0x6a */
	_LOWER,			/* `k', 0x6b */
	_LOWER,			/* `l', 0x6c */
	_LOWER,			/* `m', 0x6d */
	_LOWER,			/* `n', 0x6e */
	_LOWER,			/* `o', 0x6f */
	_LOWER,			/* `p', 0x70 */
	_LOWER,			/* `q', 0x71 */
	_LOWER,			/* `r', 0x72 */
	_LOWER,			/* `s', 0x73 */
	_LOWER,			/* `t', 0x74 */
	_LOWER,			/* `u', 0x75 */
	_LOWER,			/* `v', 0x76 */
	_LOWER,			/* `w', 0x77 */
	_LOWER,			/* `x', 0x78 */
	_LOWER,			/* `y', 0x79 */
	_LOWER,			/* `z', 0x7a */
	_PUNCT,			/* `{', 0x7b */
	_PUNCT,			/* `|', 0x7c */
	_PUNCT,			/* `}', 0x7d */
	_PUNCT,			/* `~', 0x7e */
	_CONTROL,		/* 0x7f */
	0,			/* 0x80 */
	0,			/* 0x81 */
	0,			/* 0x82 */
	0,			/* 0x83 */
	0,			/* 0x84 */
	0,			/* 0x85 */
	0,			/* 0x86 */
	0,			/* 0x87 */
	0,			/* 0x88 */
	0,			/* 0x89 */
	0,			/* 0x8a */
	0,			/* 0x8b */
	0,			/* 0x8c */
	0,			/* 0x8d */
	0,			/* 0x8e */
	0,			/* 0x8f */
	0,			/* 0x90 */
	0,			/* 0x91 */
	0,			/* 0x92 */
	0,			/* 0x93 */
	0,			/* 0x94 */
	0,			/* 0x95 */
	0,			/* 0x96 */
	0,			/* 0x97 */
	0,			/* 0x98 */
	0,			/* 0x99 */
	0,			/* 0x9a */
	0,			/* 0x9b */
	0,			/* 0x9c */
	0,			/* 0x9d */
	0,			/* 0x9e */
	0,			/* 0x9f */
	0,			/* 0xa0 */
	0,			/* 0xa1 */
	0,			/* 0xa2 */
	0,			/* 0xa3 */
	0,			/* 0xa4 */
	0,			/* 0xa5 */
	0,			/* 0xa6 */
	0,			/* 0xa7 */
	0,			/* 0xa8 */
	0,			/* 0xa9 */
	0,			/* 0xaa */
	0,			/* 0xab */
	0,			/* 0xac */
	0,			/* 0xad */
	0,			/* 0xae */
	0,			/* 0xaf */
	0,			/* 0xb0 */
	0,			/* 0xb1 */
	0,			/* 0xb2 */
	0,			/* 0xb3 */
	0,			/* 0xb4 */
	0,			/* 0xb5 */
	0,			/* 0xb6 */
	0,			/* 0xb7 */
	0,			/* 0xb8 */
	0,			/* 0xb9 */
	0,			/* 0xba */
	0,			/* 0xbb */
	0,			/* 0xbc */
	0,			/* 0xbd */
	0,			/* 0xbe */
	0,			/* 0xbf */
	0,			/* 0xc0 */
	0,			/* 0xc1 */
	0,			/* 0xc2 */
	0,			/* 0xc3 */
	0,			/* 0xc4 */
	0,			/* 0xc5 */
	0,			/* 0xc6 */
	0,			/* 0xc7 */
	0,			/* 0xc8 */
	0,			/* 0xc9 */
	0,			/* 0xca */
	0,			/* 0xcb */
	0,			/* 0xcc */
	0,			/* 0xcd */
	0,			/* 0xce */
	0,			/* 0xcf */
	0,			/* 0xd0 */
	0,			/* 0xd1 */
	0,			/* 0xd2 */
	0,			/* 0xd3 */
	0,			/* 0xd4 */
	0,			/* 0xd5 */
	0,			/* 0xd6 */
	0,			/* 0xd7 */
	0,			/* 0xd8 */
	0,			/* 0xd9 */
	0,			/* 0xda */
	0,			/* 0xdb */
	0,			/* 0xdc */
	0,			/* 0xdd */
	0,			/* 0xde */
	0,			/* 0xdf */
	0,			/* 0xe0 */
	0,			/* 0xe1 */
	0,			/* 0xe2 */
	0,			/* 0xe3 */
	0,			/* 0xe4 */
	0,			/* 0xe5 */
	0,			/* 0xe6 */
	0,			/* 0xe7 */
	0,			/* 0xe8 */
	0,			/* 0xe9 */
	0,			/* 0xea */
	0,			/* 0xeb */
	0,			/* 0xec */
	0,			/* 0xed */
	0,			/* 0xee */
	0,			/* 0xef */
	0,			/* 0xf0 */
	0,			/* 0xf1 */
	0,			/* 0xf2 */
	0,			/* 0xf3 */
	0,			/* 0xf4 */
	0,			/* 0xf5 */
	0,			/* 0xf6 */
	0,			/* 0xf7 */
	0,			/* 0xf8 */
	0,			/* 0xf9 */
	0,			/* 0xfa */
	0,			/* 0xfb */
	0,			/* 0xfc */
	0,			/* 0xfd */
	0,			/* 0xfe */
	0			/* 0xff */
};

const unsigned short _wctype[] = {
	0,						/* <EOF>, 0xFFFF */
	_CONTROL,				/* CTRL+@, 0x00 */
	_CONTROL,				/* CTRL+A, 0x01 */
	_CONTROL,				/* CTRL+B, 0x02 */
	_CONTROL,				/* CTRL+C, 0x03 */
	_CONTROL,				/* CTRL+D, 0x04 */
	_CONTROL,				/* CTRL+E, 0x05 */
	_CONTROL,				/* CTRL+F, 0x06 */
	_CONTROL,				/* CTRL+G, 0x07 */
	_CONTROL,				/* CTRL+H, 0x08 */
	_BLANK | _CONTROL | _SPACE,	/* CTRL+I, 0x09 */
	_CONTROL | _SPACE,		/* CTRL+J, 0x0a */
	_CONTROL | _SPACE,		/* CTRL+K, 0x0b */
	_CONTROL | _SPACE,		/* CTRL+L, 0x0c */
	_CONTROL | _SPACE,		/* CTRL+M, 0x0d */
	_CONTROL,				/* CTRL+N, 0x0e */
	_CONTROL,				/* CTRL+O, 0x0f */
	_CONTROL,				/* CTRL+P, 0x10 */
	_CONTROL,				/* CTRL+Q, 0x11 */
	_CONTROL,				/* CTRL+R, 0x12 */
	_CONTROL,				/* CTRL+S, 0x13 */
	_CONTROL,				/* CTRL+T, 0x14 */
	_CONTROL,				/* CTRL+U, 0x15 */
	_CONTROL,				/* CTRL+V, 0x16 */
	_CONTROL,				/* CTRL+W, 0x17 */
	_CONTROL,				/* CTRL+X, 0x18 */
	_CONTROL,				/* CTRL+Y, 0x19 */
	_CONTROL,				/* CTRL+Z, 0x1a */
	_CONTROL,				/* CTRL+[, 0x1b */
	_CONTROL,				/* CTRL+\, 0x1c */
	_CONTROL,				/* CTRL+], 0x1d */
	_CONTROL,				/* CTRL+^, 0x1e */
	_CONTROL,				/* CTRL+_, 0x1f */
	_SPACE | _BLANK,		/* ` ', 0x20 */
	_PUNCT,					/* `!', 0x21 */
	_PUNCT,					/* 0x22 */
	_PUNCT,					/* `#', 0x23 */
	_PUNCT,					/* `$', 0x24 */
	_PUNCT,					/* `%', 0x25 */
	_PUNCT,					/* `&', 0x26 */
	_PUNCT,					/* 0x27 */
	_PUNCT,					/* `(', 0x28 */
	_PUNCT,					/* `)', 0x29 */
	_PUNCT,					/* `*', 0x2a */
	_PUNCT,					/* `+', 0x2b */
	_PUNCT,					/* `,', 0x2c */
	_PUNCT,					/* `-', 0x2d */
	_PUNCT,					/* `.', 0x2e */
	_PUNCT,					/* `/', 0x2f */
	_DIGIT | _HEX,			/* `0', 0x30 */
	_DIGIT | _HEX,			/* `1', 0x31 */
	_DIGIT | _HEX,			/* `2', 0x32 */
	_DIGIT | _HEX,			/* `3', 0x33 */
	_DIGIT | _HEX,			/* `4', 0x34 */
	_DIGIT | _HEX,			/* `5', 0x35 */
	_DIGIT | _HEX,			/* `6', 0x36 */
	_DIGIT | _HEX,			/* `7', 0x37 */
	_DIGIT | _HEX,			/* `8', 0x38 */
	_DIGIT | _HEX,			/* `9', 0x39 */
	_PUNCT,					/* `:', 0x3a */
	_PUNCT,					/* `;', 0x3b */
	_PUNCT,					/* `<', 0x3c */
	_PUNCT,					/* `=', 0x3d */
	_PUNCT,					/* `>', 0x3e */
	_PUNCT,					/* `?', 0x3f */
	_PUNCT,					/* `@', 0x40 */
	0x0100 | _UPPER | _HEX,	/* `A', 0x41 */
	0x0100 | _UPPER | _HEX,	/* `B', 0x42 */
	0x0100 | _UPPER | _HEX,	/* `C', 0x43 */
	0x0100 | _UPPER | _HEX,	/* `D', 0x44 */
	0x0100 | _UPPER | _HEX,	/* `E', 0x45 */
	0x0100 | _UPPER | _HEX,	/* `F', 0x46 */
	0x0100 | _UPPER,		/* `G', 0x47 */
	0x0100 | _UPPER,		/* `H', 0x48 */
	0x0100 | _UPPER,		/* `I', 0x49 */
	0x0100 | _UPPER,		/* `J', 0x4a */
	0x0100 | _UPPER,		/* `K', 0x4b */
	0x0100 | _UPPER,		/* `L', 0x4c */
	0x0100 | _UPPER,		/* `M', 0x4d */
	0x0100 | _UPPER,		/* `N', 0x4e */
	0x0100 | _UPPER,		/* `O', 0x4f */
	0x0100 | _UPPER,		/* `P', 0x50 */
	0x0100 | _UPPER,		/* `Q', 0x51 */
	0x0100 | _UPPER,		/* `R', 0x52 */
	0x0100 | _UPPER,		/* `S', 0x53 */
	0x0100 | _UPPER,		/* `T', 0x54 */
	0x0100 | _UPPER,		/* `U', 0x55 */
	0x0100 | _UPPER,		/* `V', 0x56 */
	0x0100 | _UPPER,		/* `W', 0x57 */
	0x0100 | _UPPER,		/* `X', 0x58 */
	0x0100 | _UPPER,		/* `Y', 0x59 */
	0x0100 | _UPPER,		/* `Z', 0x5a */
	_PUNCT,					/* `[', 0x5b */
	_PUNCT,					/* 0x5c */
	_PUNCT,					/* `]', 0x5d */
	_PUNCT,					/* `^', 0x5e */
	_PUNCT,					/* `_', 0x5f */
	_PUNCT,					/* 0x60 */
	0x0100 | _LOWER | _HEX,	/* `a', 0x61 */
	0x0100 | _LOWER | _HEX,	/* `b', 0x62 */
	0x0100 | _LOWER | _HEX,	/* `c', 0x63 */
	0x0100 | _LOWER | _HEX,	/* `d', 0x64 */
	0x0100 | _LOWER | _HEX,	/* `e', 0x65 */
	0x0100 | _LOWER | _HEX,	/* `f', 0x66 */
	0x0100 | _LOWER,		/* `g', 0x67 */
	0x0100 | _LOWER,		/* `h', 0x68 */
	0x0100 | _LOWER,		/* `i', 0x69 */
	0x0100 | _LOWER,		/* `j', 0x6a */
	0x0100 | _LOWER,		/* `k', 0x6b */
	0x0100 | _LOWER,		/* `l', 0x6c */
	0x0100 | _LOWER,		/* `m', 0x6d */
	0x0100 | _LOWER,		/* `n', 0x6e */
	0x0100 | _LOWER,		/* `o', 0x6f */
	0x0100 | _LOWER,		/* `p', 0x70 */
	0x0100 | _LOWER,		/* `q', 0x71 */
	0x0100 | _LOWER,		/* `r', 0x72 */
	0x0100 | _LOWER,		/* `s', 0x73 */
	0x0100 | _LOWER,		/* `t', 0x74 */
	0x0100 | _LOWER,		/* `u', 0x75 */
	0x0100 | _LOWER,		/* `v', 0x76 */
	0x0100 | _LOWER,		/* `w', 0x77 */
	0x0100 | _LOWER,		/* `x', 0x78 */
	0x0100 | _LOWER,		/* `y', 0x79 */
	0x0100 | _LOWER,		/* `z', 0x7a */
	_PUNCT,					/* `{', 0x7b */
	_PUNCT,					/* `|', 0x7c */
	_PUNCT,					/* `}', 0x7d */
	_PUNCT,				    /* `~', 0x7e */
	_CONTROL,				/* 0x7f */
	_CONTROL,				/* 0x80 */
	_CONTROL,				/* 0x81 */
	_CONTROL,				/* 0x82 */
	_CONTROL,				/* 0x83 */
	_CONTROL,				/* 0x84 */
	_CONTROL,				/* 0x85 */
	_CONTROL,				/* 0x86 */
	_CONTROL,				/* 0x87 */
	_CONTROL,				/* 0x88 */
	_CONTROL,				/* 0x89 */
	_CONTROL,				/* 0x8a */
	_CONTROL,				/* 0x8b */
	_CONTROL,				/* 0x8c */
	_CONTROL,				/* 0x8d */
	_CONTROL,				/* 0x8e */
	_CONTROL,				/* 0x8f */
	_CONTROL,				/* 0x90 */
	_CONTROL,				/* 0x91 */
	_CONTROL,				/* 0x92 */
	_CONTROL,				/* 0x93 */
	_CONTROL,				/* 0x94 */
	_CONTROL,				/* 0x95 */
	_CONTROL,				/* 0x96 */
	_CONTROL,				/* 0x97 */
	_CONTROL,				/* 0x98 */
	_CONTROL,				/* 0x99 */
	_CONTROL,				/* 0x9a */
	_CONTROL,				/* 0x9b */
	_CONTROL,				/* 0x9c */
	_CONTROL,				/* 0x9d */
	_CONTROL,				/* 0x9e */
	_CONTROL,				/* 0x9f */
	_BLANK | _SPACE,		/* 0xa0 */
	_PUNCT,					/* 0xa1 */
	_PUNCT,					/* 0xa2 */
	_PUNCT,					/* 0xa3 */
	_PUNCT,					/* 0xa4 */
	_PUNCT,					/* 0xa5 */
	_PUNCT,					/* 0xa6 */
	_PUNCT,					/* 0xa7 */
	_PUNCT,					/* 0xa8 */
	_PUNCT,					/* 0xa9 */
	_PUNCT,					/* 0xaa */
	_PUNCT,					/* 0xab */
	_PUNCT,					/* 0xac */
	_PUNCT,					/* 0xad */
	_PUNCT,					/* 0xae */
	_PUNCT,					/* 0xaf */
	_PUNCT,					/* 0xb0 */
	_PUNCT,					/* 0xb1 */
	_PUNCT | _DIGIT,		/* 0xb2 */
	_PUNCT | _DIGIT,		/* 0xb3 */
	_PUNCT,					/* 0xb4 */
	_PUNCT,					/* 0xb5 */
	_PUNCT,					/* 0xb6 */
	_PUNCT,					/* 0xb7 */
	_PUNCT,					/* 0xb8 */
	_PUNCT | _DIGIT,		/* 0xb9 */
	_PUNCT,					/* 0xba */
	_PUNCT,					/* 0xbb */
	_PUNCT,					/* 0xbc */
	_PUNCT,					/* 0xbd */
	_PUNCT,					/* 0xbe */
	_PUNCT,					/* 0xbf */
	0x0100 | _UPPER,		/* 0xc0 */
	0x0100 | _UPPER,		/* 0xc1 */
	0x0100 | _UPPER,		/* 0xc2 */
	0x0100 | _UPPER,		/* 0xc3 */
	0x0100 | _UPPER,		/* 0xc4 */
	0x0100 | _UPPER,		/* 0xc5 */
	0x0100 | _UPPER,		/* 0xc6 */
	0x0100 | _UPPER,		/* 0xc7 */
	0x0100 | _UPPER,		/* 0xc8 */
	0x0100 | _UPPER,		/* 0xc9 */
	0x0100 | _UPPER,		/* 0xca */
	0x0100 | _UPPER,		/* 0xcb */
	0x0100 | _UPPER,		/* 0xcc */
	0x0100 | _UPPER,		/* 0xcd */
	0x0100 | _UPPER,		/* 0xce */
	0x0100 | _UPPER,		/* 0xcf */
	0x0100 | _UPPER,		/* 0xd0 */
	0x0100 | _UPPER,		/* 0xd1 */
	0x0100 | _UPPER,		/* 0xd2 */
	0x0100 | _UPPER,		/* 0xd3 */
	0x0100 | _UPPER,		/* 0xd4 */
	0x0100 | _UPPER,		/* 0xd5 */
	0x0100 | _UPPER,		/* 0xd6 */
	_PUNCT,					/* 0xd7 */
	0x0100 | _UPPER,		/* 0xd8 */
	0x0100 | _UPPER,		/* 0xd9 */
	0x0100 | _UPPER,		/* 0xda */
	0x0100 | _UPPER,		/* 0xdb */
	0x0100 | _UPPER,		/* 0xdc */
	0x0100 | _UPPER,		/* 0xdd */
	0x0100 | _UPPER,		/* 0xde */
	0x0100 | _LOWER,		/* 0xdf */
	0x0100 | _LOWER,		/* 0xe0 */
	0x0100 | _LOWER,		/* 0xe1 */
	0x0100 | _LOWER,		/* 0xe2 */
	0x0100 | _LOWER,		/* 0xe3 */
	0x0100 | _LOWER,		/* 0xe4 */
	0x0100 | _LOWER,		/* 0xe5 */
	0x0100 | _LOWER,		/* 0xe6 */
	0x0100 | _LOWER,		/* 0xe7 */
	0x0100 | _LOWER,		/* 0xe8 */
	0x0100 | _LOWER,		/* 0xe9 */
	0x0100 | _LOWER,		/* 0xea */
	0x0100 | _LOWER,		/* 0xeb */
	0x0100 | _LOWER,		/* 0xec */
	0x0100 | _LOWER,		/* 0xed */
	0x0100 | _LOWER,		/* 0xee */
	0x0100 | _LOWER,		/* 0xef */
	0x0100 | _LOWER,		/* 0xf0 */
	0x0100 | _LOWER,		/* 0xf1 */
	0x0100 | _LOWER,		/* 0xf2 */
	0x0100 | _LOWER,		/* 0xf3 */
	0x0100 | _LOWER,		/* 0xf4 */
	0x0100 | _LOWER,		/* 0xf5 */
	0x0100 | _LOWER,		/* 0xf6 */
	_PUNCT,					/* 0xf7 */
	0x0100 | _LOWER,		/* 0xf8 */
	0x0100 | _LOWER,		/* 0xf9 */
	0x0100 | _LOWER,		/* 0xfa */
	0x0100 | _LOWER,		/* 0xfb */
	0x0100 | _LOWER,		/* 0xfc */
	0x0100 | _LOWER,		/* 0xfd */
	0x0100 | _LOWER,		/* 0xfe */
	0x0100 | _LOWER			/* 0xff */
};

const unsigned short *_pctype = _ctype + 1;
const unsigned short *_pwctype = _wctype + 1;

extern const unsigned short wine_wctype_table[];

/*
 * @implemented
 */
const unsigned short **__p__pwctype(void)
{
   return &_pwctype;
}

const unsigned short* __cdecl __pwctype_func(void)
{
   return _pwctype;
}

#ifdef _LIBCNT_
int __cdecl _isctype (int c, int ctypeFlags)
{
  return (_pctype[(unsigned char)(c & 0xFF)] & ctypeFlags);
}
#else
/*********************************************************************
 *         _isctype_l (MSVCRT.@)
 */
int __cdecl _isctype_l(int c, int type, _locale_t locale)
{
  MSVCRT_pthreadlocinfo locinfo;

  if(!locale)
    locinfo = get_locinfo();
  else
    locinfo = ((MSVCRT__locale_t)locale)->locinfo;

  if (c >= -1 && c <= 255)
    return locinfo->pctype[c] & type;

  if (locinfo->mb_cur_max != 1 && c > 0)
  {
    /* FIXME: Is there a faster way to do this? */
    WORD typeInfo;
    char convert[3], *pconv = convert;

    if (locinfo->pctype[(UINT)c >> 8] & _LEADBYTE)
      *pconv++ = (UINT)c >> 8;
    *pconv++ = c & 0xff;
    *pconv = 0;

    if (GetStringTypeExA(locinfo->lc_handle[LC_CTYPE],
                CT_CTYPE1, convert, convert[1] ? 2 : 1, &typeInfo))
      return typeInfo & type;
  }
  return 0;
}

int __cdecl _isctype (int c, int ctypeFlags)
{
  return _isctype_l(c, ctypeFlags, NULL);
}

/*********************************************************************
 *		_isleadbyte_l (MSVCRT.@)
 */
int __cdecl _isleadbyte_l(int c, _locale_t locale)
{
  return _isctype_l( c, _LEADBYTE, locale );
}

#endif /* _LIBCNT_ */

/*
 * @implemented
 */
int __cdecl isalpha(int c)
{
   return(_isctype(c, _ALPHA));
}

/*
 * @implemented
 */
int __cdecl isalnum(int c)
{
   return(_isctype(c, _ALPHA | _DIGIT));
}

/*
 * @implemented
 */
int __cdecl iswalnum(wint_t c)
{
    return iswctype(c, _ALPHA | _DIGIT);
}

/*
 * @implemented
 */
int __cdecl __isascii(int c)
{
   return ((unsigned char)c <= 0x7f);
}

/*
 * @implemented
 */
int __cdecl iswascii(wint_t c)
{
    return __isascii(c);
}

/*
 * @implemented
 */
int __cdecl iscntrl(int c)
{
   return(_isctype(c, _CONTROL));
}

/*
 * @implemented
 */
int __cdecl __iscsym(int c)
{
   return (c < 127 && (isalnum(c) || (c == '_')));
}

/*
 * @implemented
 */
int __cdecl __iscsymf(int c)
{
   return (c < 127 && (isalpha(c) || (c == '_')));
}

/*
 * @implemented
 */
int __cdecl isdigit(int c)
{
   return(_isctype(c, _DIGIT));
}

/*
 * @implemented
 */
int __cdecl isgraph(int c)
{
   return (_isctype (c, _PUNCT | _ALPHA | _DIGIT));
}

/*
 * @implemented
 */
int __cdecl islower(int c)
{
   return (_isctype (c, _LOWER));
}

/*
 * @implemented
 */
int __cdecl isprint(int c)
{
   return (_isctype (c, _BLANK | _PUNCT | _ALPHA | _DIGIT));
}

/*
 * @implemented
 */
int __cdecl ispunct(int c)
{
   return (_isctype (c, _PUNCT));
}

/*
 * @implemented
 */
int __cdecl isspace(int c)
{
   return (_isctype (c, _SPACE));
}

/*
 * @implemented
 */
int __cdecl isupper(int c)
{
   return (_isctype (c, _UPPER));
}

/*
 * @implemented
 */
int __cdecl isxdigit(int c)
{
   return (_isctype (c, _HEX));
}


/*
 * @implemented
 */
int __cdecl iswalpha(wint_t c)
{
   return (iswctype (c, _ALPHA));
}

/*
 * @implemented
 */
int __cdecl iswcntrl(wint_t c)
{
    return iswctype(c, _CONTROL);
}

/*
 * @implemented
 */
int __cdecl iswdigit(wint_t c)
{
   return (iswctype (c, _DIGIT));
}

/*
 * @implemented
 */
int __cdecl iswgraph(wint_t c)
{
  return iswctype(c,_PUNCT | _ALPHA | _DIGIT);
}

/*
 * @implemented
 */
int __cdecl iswprint(wint_t c)
{
  return iswctype((unsigned short)c,_BLANK | _PUNCT | _ALPHA | _DIGIT);
}


/*
 * @implemented
 */
int __cdecl iswpunct(wint_t c)
{
    return iswctype(c, _PUNCT);
}

/*
 * @implemented
 */
int __cdecl iswlower(wint_t c)
{
   return (iswctype (c, _LOWER));
}

/*
 * @implemented
 */
int __cdecl iswupper(wint_t c)
{
    return iswctype(c, _UPPER);
}


/*
 * @implemented
 */
int __cdecl iswspace(wint_t c)
{
   return (iswctype (c, _SPACE));
}

/*
 * @implemented
 */
int __cdecl iswxdigit(wint_t c)
{
   return (iswctype (c, _HEX));
}


/*
 * @implemented
 */
int __cdecl __toascii(int c)
{
   return((unsigned int)(c) & 0x7f);
}

/*
 * @implemented
 */
int __cdecl _tolower(int c)
{
   if (_isctype (c, _UPPER))
       return (c - upalpha);
   return(c);
}

/*
 * @implemented
 */
int __cdecl _toupper(int c)
{
   if (_isctype (c, _LOWER))
      return (c + upalpha);
   return(c);
}

/*
 * @implemented
 */
int __cdecl tolower(int c)
{
   if (_isctype (c, _UPPER))
       return (c - upalpha);
   return(c);
}

/*
 * @implemented
 */
int __cdecl toupper(int c)
{
   if (_isctype (c, _LOWER))
      return (c + upalpha);
   return(c);
}

/*
 * @implemented
 */
wint_t __cdecl towlower(wint_t c)
{
   if (iswctype (c, _UPPER))
       return (c - upalpha);
   return(c);
}

/*
 * @implemented
 */
wint_t __cdecl towupper(wint_t c)
{
   if (iswctype (c, _LOWER))
      return (c + upalpha);
   return(c);
}

/* EOF */
