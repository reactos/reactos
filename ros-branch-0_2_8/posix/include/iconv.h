/* $Id: iconv.h,v 1.4 2002/10/29 04:45:10 rex Exp $
 */
/*
 * iconv.h
 *
 * codeset conversion facility. Conforming to the Single UNIX(r)
 * Specification Version 2, System Interface & Headers Issue 5
 *
 * This file is part of the ReactOS Operating System.
 *
 * Contributors:
 *  Created by KJK::Hyperion <noog@libero.it>
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
 *
 */
#ifndef __ICONV_H_INCLUDED__
#define __ICONV_H_INCLUDED__

/* INCLUDES */

/* OBJECTS */

/* TYPES */
typedef (void *) iconv_t;

/* CONSTANTS */

/* PROTOTYPES */
iconv_t iconv_open(const char *, const char *);
size_t  iconv(iconv_t, char **, size_t *, char **, size_t *);
int     iconv_close(iconv_t);

/* MACROS */

#endif /* __ICONV_H_INCLUDED__ */

/* EOF */

