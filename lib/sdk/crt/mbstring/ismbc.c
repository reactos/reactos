/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/mbstring/ismbc.c
 * PURPOSE:
 * PROGRAMER:   
 * UPDATE HISTORY:
 *              05/30/08: Samuel Serapion adapted from PROJECT C Library
 *
 */


#include <precomp.h>
#include <mbstring.h>
#include <mbctype.h>

unsigned char _mbctype[257] = {
    0,						     /* EOF */
    0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 , /* 0 */
    0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 , /* 1 */
    B ,P ,P ,P ,P ,P ,P ,P ,P ,P ,P ,P ,P ,P ,P ,P , /* 2 */
    D ,D ,D ,P ,D ,D ,D ,D ,D ,D ,P ,P ,P ,P ,P ,P , /* 3 */
    PT,AT,AT,AT,AT,AT,AT,AT,AT,AT,AT,AT,AT,AT,AT,AT, /* 4 */
    AT,AT,AT,AT,AT,AT,AT,AT,AT,AT,AT,PT,PT,PT,PT,PT, /* 5 */
    PT,AT,AT,AT,AT,AT,AT,AT,AT,AT,AT,AT,AT,AT,AT,AT, /* 6 */
    AT,AT,AT,AT,AT,AT,AT,AT,AT,AT,AT,PT,PT,PT,PT,0 , /* 7 */
    T ,LT,LT,LT,LT,LT,LT,LT,LT,LT,LT,LT,LT,LT,LT,LT, /* 8 */
    LT,LT,LT,LT,LT,LT,LT,LT,LT,LT,LT,LT,LT,LT,LT,LT, /* 9 */
    T ,GT,GT,GT,GT,GT,KT,KT,KT,KT,KT,KT,KT,KT,KT,KT, /* A */
    KT,KT,KT,KT,KT,KT,KT,KT,KT,KT,KT,KT,KT,KT,KT,KT, /* B */
    KT,KT,KT,KT,KT,KT,KT,KT,KT,KT,KT,KT,KT,KT,KT,KT, /* C */
    KT,KT,KT,KT,KT,KT,KT,KT,KT,KT,KT,KT,KT,KT,GT,GT, /* D */
    LT,LT,LT,LT,LT,LT,LT,LT,LT,LT,LT,LT,LT,LT,LT,LT, /* E */
    LT,LT,LT,LT,LT,LT,LT,LT,LT,LT,LT,LT,LT,0 ,0 ,0 ,  /* F */
};

/*
 * @implemented
 */
int _ismbcalnum( unsigned int c )
{
	if ((c & 0xFF00) != 0) {
		// true multibyte character
		return 0;
	}
	else
		return _ismbbalnum(c);

	return 0;
}

/*
 * @implemented
 */
int _ismbcalpha( unsigned int c )
{
	return (_ismbcupper (c) || _ismbclower (c));
}

/*
 * @implemented
 */
int _ismbcdigit( unsigned int c )
{
	return ((c) >= 0x824f && (c) <= 0x8258);
}

/*
 * @implemented
 */
int _ismbcprint( unsigned int c )
{
    return (_MBHMASK (c) ? _ismbclegal (c) : (isprint (c) || _ismbbkana (c)));
}

/*
 * @implemented
 */
int _ismbcsymbol( unsigned int c )
{
	return (c >= 0x8141 && c <= 0x817e) || (c >= 0x8180 && c <= 0x81ac);
}

/*
 * @implemented
 */
int _ismbcspace( unsigned int c )
{
	return ((c) == 0x8140);
}
/*
 * @implemented
 */
int _ismbclegal(unsigned int c)
{
	return (_ismbblead (_MBGETH (c)) && _ismbbtrail (_MBGETL (c)));
}

/*
 * @implemented
 */
int _ismbcl0(unsigned int c)
{
  return (c >= 0x8140 && c <= 0x889e);
}

/*
 * @implemented
 */
int _ismbcl1(unsigned int c)
{
  return (c >= 0x889f && c <= 0x9872);
}

/*
 * @implemented
 */
int _ismbcl2(unsigned int c)
{
  return (c >= 0x989f && c <= 0xea9e);
}

/*
 * @unimplemented
 */
int _ismbcgraph(unsigned int ch)
{
    //wchar_t wch = msvcrt_mbc_to_wc( ch );
    //return (get_char_typeW( wch ) & (C1_UPPER | C1_LOWER | C1_DIGIT | C1_PUNCT | C1_ALPHA));
    return 0;
}

/*
 * @unimplemented
 */
int _ismbcpunct(unsigned int ch)
{
    //wchar_t wch = msvcrt_mbc_to_wc( ch );
    //return (get_char_typeW( wch ) & C1_PUNCT);
    return 0;
}
