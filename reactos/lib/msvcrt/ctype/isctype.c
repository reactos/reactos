/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/ctype.h>


extern unsigned short _ctype[];

unsigned short *_pctype = _ctype + 1;
unsigned short *_pwctype = _ctype + 1;


unsigned short **__p__pctype(void)
{
   return &_pctype;
}

unsigned short **__p__pwctype(void)
{
   return &_pwctype;
}

int _isctype(unsigned int c, int ctypeFlags)
{
   return (_pctype[(unsigned char)(c & 0xFF)] & ctypeFlags);
}

int iswctype(wint_t wc, wctype_t wctypeFlags)
{
   return (_pwctype[(unsigned char)(wc & 0xFF)] & wctypeFlags);
}

// obsolete
int is_wctype(wint_t wc, wctype_t wctypeFlags)
{
   return (_pwctype[(unsigned char)(wc & 0xFF)] & wctypeFlags);
}

/* EOF */
