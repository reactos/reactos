/*
 * dos.h
 *
 * Definitions for MS-DOS interface routines
 *
 * This header file is meant for use with CRTDLL.DLL as included with
 * Windows 95(tm) and Windows NT(tm). In conjunction with other versions
 * of the standard C library things may or may not work so well.
 *
 * Contributors:
 *  Created by J.J. van der Heijden <J.J.vanderHeijden@student.utwente.nl>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef __STRICT_ANSI__

#ifndef _DOS_H_
#define _DOS_H_

#define __need_wchar_t
#include <stddef.h>

/* For DOS file attributes */
#include <dir.h>

#ifdef __cplusplus
extern "C" {
#endif

extern char**		__imp__pgmptr_dll;
#define _pgmptr		(*__imp__pgmptr_dll)

/* Wide character equivalent */
extern wchar_t**	__imp__wpgmptr_dll;
#define _wpgmptr	(*__imp__wpgmptr_dll)

/* These are obsolete, but some may find them useful */
extern unsigned int*	__imp__basemajor_dll;
extern unsigned int*	__imp__baseminor_dll;
extern unsigned int*	__imp__baseversion_dll;
extern unsigned int*	__imp__osmajor_dll;
extern unsigned int*	__imp__osminor_dll;
extern unsigned int*	__imp__osversion_dll;

#define _basemajor	(*__imp__basemajor_dll)
#define _baseminor	(*__imp__baseminor_dll)
#define _baseversion	(*__imp__baseversion_dll)
#define _osmajor	(*__imp__osmajor_dll)
#define _osminor	(*__imp__osminor_dll)
#define _osversion	(*__imp__osversion_dll)

#ifdef __cplusplus
}
#endif

#endif /* _DOS_H_ */

#endif  /* Not __STRICT_ANSI__ */

