/*
 *  linux/lib/ctype.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <ctype.h>

#if 0
char _ctmp;
unsigned char _ctype[] = {0x00,			/* EOF */
_C,_C,_C,_C,_C,_C,_C,_C,			/* 0-7 */
_C,_C|_S,_C|_S,_C|_S,_C|_S,_C|_S,_C,_C,		/* 8-15 */
_C,_C,_C,_C,_C,_C,_C,_C,			/* 16-23 */
_C,_C,_C,_C,_C,_C,_C,_C,			/* 24-31 */
_S|_SP,_P,_P,_P,_P,_P,_P,_P,			/* 32-39 */
_P,_P,_P,_P,_P,_P,_P,_P,			/* 40-47 */
_D,_D,_D,_D,_D,_D,_D,_D,			/* 48-55 */
_D,_D,_P,_P,_P,_P,_P,_P,			/* 56-63 */
_P,_U|_X,_U|_X,_U|_X,_U|_X,_U|_X,_U|_X,_U,	/* 64-71 */
_U,_U,_U,_U,_U,_U,_U,_U,			/* 72-79 */
_U,_U,_U,_U,_U,_U,_U,_U,			/* 80-87 */
_U,_U,_U,_P,_P,_P,_P,_P,			/* 88-95 */
_P,_L|_X,_L|_X,_L|_X,_L|_X,_L|_X,_L|_X,_L,	/* 96-103 */
_L,_L,_L,_L,_L,_L,_L,_L,			/* 104-111 */
_L,_L,_L,_L,_L,_L,_L,_L,			/* 112-119 */
_L,_L,_L,_P,_P,_P,_P,_C,			/* 120-127 */
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,		/* 128-143 */
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,		/* 144-159 */
_S|_SP,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,   /* 160-175 */
_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,       /* 176-191 */
_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,       /* 192-207 */
_U,_U,_U,_U,_U,_U,_U,_P,_U,_U,_U,_U,_U,_U,_U,_L,       /* 208-223 */
_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,       /* 224-239 */
_L,_L,_L,_L,_L,_L,_L,_P,_L,_L,_L,_L,_L,_L,_L,_L};      /* 240-255 */
#endif


int isdigit(int c)
{
   return((c >= '0' && c <= '9'));
}

int islower(int c)
{
   return((c >= 'a' && c <= 'z'));
}

int isprint(int c)
{
   return((c >= ' ' && c <= '~'));
}

int isspace(int c)
{
   return((c == ' ' || c == '\t'));
}

int isupper(int c)
{
   return((c >= 'A') && (c <= 'Z'));
}

int isxdigit(int c)
{
   return(('0' <= c && '9' >= c) || ('a' <= 'c' && 'f' >= 'c') ||
	  ('A' <= c && 'Z' >= c));
}

int tolower(int c)
{
   return (c >= 'A' && c <= 'Z')   ? c - ( 'A' - 'a' ) : c;
}

int toupper(int c)
{
   return (c >= 'a' && c <= 'z')   ? c + 'A' - 'a' : c;
}

wchar_t towlower(wchar_t c)
{
   return (c >= 'A' && c <= 'Z')   ? c - ( 'A' - 'a' ) : c;
}

wchar_t towupper(wchar_t c)
{
   return (c >= 'a' && c <= 'z')   ? c + 'A' - 'a' : c;
}
