/* $Id: dlfcn.h,v 1.2 2002/02/20 09:17:55 hyperion Exp $
 */
/*
 * psx/dlfcn.h
 *
 * internal dlfcn.h
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
#ifndef __PSX_DLFCN_H_INCLUDED__
#define __PSX_DLFCN_H_INCLUDED__

/* INCLUDES */
#include <psx/errno.h>

/* OBJECTS */

/* TYPES */
/* internal representation for loaded DLLs */
/* TODO: get rid of this. The handle should be enough, with a proper PE loader */
struct __dlobj
{
 int   global; /* if non-zero, all the other fields have no meaning */
 void *handle; /* pointer to the module mapping */
};

/* CONSTANTS */

/* PROTOTYPES */
void __dl_set_last_error(int);

/* MACROS */
#define __dl_get_reloc_flag(m) ((m) & (RTLD_LAZY | RTLD_NOW))
#define __dl_get_scope_flag(m) ((m) & (RTLD_GLOBAL | RTLD_LOCAL))

#endif /* __PSX_DLFCN_H_INCLUDED__ */

/* EOF */

