/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/ctype.h>


extern unsigned short _ctype[];

unsigned short *_pctype_dll = _ctype + 1;
unsigned short *_pwctype_dll = _ctype + 1;


/*
 * @implemented
 *
 * this function is now forwarded to MSVCRT._isctype to reduce code duplication
 */
#if 0
int _isctype(unsigned int c, int ctypeFlags)
{
	return (_pctype_dll[(unsigned char)(c & 0xFF)] & ctypeFlags);
}
#endif

/*
 * @implemented
 *
 * this function is now forwarded to MSVCRT.iswctype to reduce code duplication
 */
#if 0
int iswctype(wint_t wc, wctype_t wctypeFlags)
{
	return (_pwctype_dll[(unsigned char)(wc & 0xFF)] & wctypeFlags);
}
#endif

// obsolete
/*
 * @implemented
 *
 * this function is now forwarded to MSVCRT.is_wctype to reduce code duplication
 */
#if 0
int	is_wctype(wint_t wc, wctype_t wctypeFlags)
{
	return (_pwctype_dll[(unsigned char)(wc & 0xFF)] & wctypeFlags);
}
#endif

/* EOF */
