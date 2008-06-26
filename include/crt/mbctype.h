/*
 * mbctype.h
 *
 * Functions for testing multibyte character types and converting characters.
 *
 * This file is part of the Mingw32 package.
 *
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAIMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef _MBCTYPE_H_
#define _MBCTYPE_H_

/* All the headers include this file. */
#include <_mingw.h>

/* return values for _mbsbtype  and  _mbbtype in mbstring.h */
#define _MBC_SINGLE 0
#define _MBC_LEAD 1
#define _MBC_TRAIL 2
#define _MBC_ILLEGAL (-1)

/*  args for setmbcp (in lieu of actual codepage)  */
#define _MB_CP_SBCS 0
#define _MB_CP_OEM (-2)
#define _MB_CP_ANSI (-3)
#define _MB_CP_LOCALE (-4)

#define _MS     0x01
#define _MP     0x02
#define _M1     0x04
#define _M2     0x08

#define _SBUP   0x10
#define _SBLOW  0x20


#ifndef RC_INVOKED

#ifdef __cplusplus
extern "C" {
#endif

#ifndef	__STRICT_ANSI__

_CRTIMP int __cdecl __MINGW_NOTHROW _setmbcp (int);
_CRTIMP int __cdecl __MINGW_NOTHROW _getmbcp (void);

/* byte classification  */
/* NB: Corresponding _ismbc* functions are in mbstring.h */

_CRTIMP int __cdecl __MINGW_NOTHROW _ismbbalpha (unsigned int);
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbbalnum (unsigned int);
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbbgraph (unsigned int);
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbbprint (unsigned int);
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbbpunct (unsigned int);

_CRTIMP int __cdecl __MINGW_NOTHROW _ismbbkana (unsigned int);
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbbkalnum (unsigned int);
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbbkprint (unsigned int);
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbbkpunct (unsigned int);


/* these are also in mbstring.h */
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbblead (unsigned int);
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbbtrail (unsigned int);
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbslead (const unsigned char*, const unsigned char*);
_CRTIMP int __cdecl __MINGW_NOTHROW _ismbstrail (const unsigned char*, const unsigned char*);

#ifdef __DECLSPEC_SUPPORTED
__MINGW_IMPORT unsigned char _mbctype[];
__MINGW_IMPORT unsigned char _mbcasemap[];
#endif

/* TODO : _MBCS_ mappings go in tchar.h */

#endif	/* Not strict ANSI */

#ifdef __cplusplus
}
#endif

#endif	/* Not RC_INVOKED */

#endif	/* Not _MCTYPE_H_ */


