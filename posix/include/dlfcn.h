/*
 * dlfcn.h
 *
 * dynamic linking. Based on the Single UNIX(r) Specification, Version 2
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
#ifndef __DLFCN_H_INCLUDED__
#define __DLFCN_H_INCLUDED__

/* INCLUDES */

/* TYPES */

/* CONSTANTS */
#define RTLD_LAZY   (0x00000000) /* Relocations are performed at an
                                    implementation-dependent time. */
#define RTLD_NOW    (0x00000001) /* Relocations are performed when
                                    the object is loaded. */

#define RTLD_GLOBAL (0x00000010) /* All symbols are available for
                                    relocation processing of other
                                    modules. */
#define RTLD_LOCAL  (0x00000020) /* All symbols are not made available
                                    for relocation processing by other
                                    modules. */

#define RTLD_NEXT ((void *)(-1))

/* PROTOTYPES */
void  *dlopen(const char *, int);
void  *dlsym(void *, const char *);
int    dlclose(void *);
char  *dlerror(void);

/* MACROS */

#endif /* __DLFCN_H_INCLUDED__ */

/* EOF */

