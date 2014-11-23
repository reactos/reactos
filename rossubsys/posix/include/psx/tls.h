/* $Id: tls.h,v 1.3 2002/10/29 04:45:15 rex Exp $
 */
/*
 * psx/tls.h
 *
 * types and calls for TLS management
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

#ifndef __PSX_TLS_H_INCLUDED__
#define __PSX_TLS_H_INCLUDED__

/* INCLUDES */

/* OBJECTS */

/* TYPES */
typedef unsigned int __tls_index_t;

/* CONSTANTS */

/* PROTOTYPES */
__tls_index_t __tls_alloc();
int           __tls_free(__tls_index_t index);
void *        __tls_get_val(__tls_index_t index);
int           __tls_put_val(__tls_index_t index, void * data);

/* MACROS */

#endif /* __PSX_TLS_H_INCLUDED__ */

/* EOF */

