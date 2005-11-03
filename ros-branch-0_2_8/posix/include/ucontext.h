/* $Id: ucontext.h,v 1.4 2002/10/29 04:45:25 rex Exp $
 */
/*
 * ucontext.h
 *
 * user context. Conforming to the Single UNIX(r) Specification Version 2,
 * System Interface & Headers Issue 5
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
#ifndef __UCONTEXT_H_INCLUDED__
#define __UCONTEXT_H_INCLUDED__

/* INCLUDES */
#include <signal.h>

/* OBJECTS */

/* TYPES */
typedef void * mcontext_t;

typedef struct __tagucontext_t ucontext_t;

struct __tagucontext_t
{
 ucontext_t *uc_link;     /* pointer to the context that will be resumed
                             when this context returns */
 sigset_t    uc_sigmask;  /* the set of signals that are blocked when this
                             context is active */
 stack_t     uc_stack;    /* the stack used by this context */
 mcontext_t  uc_mcontext; /* a machine-specific representation of the saved
                             context */
};

/* CONSTANTS */

/* PROTOTYPES */
int  getcontext(ucontext_t *);
int  setcontext(const ucontext_t *);
void makecontext(ucontext_t *, (void *)(), int, ...);
int  swapcontext(ucontext_t *, const ucontext_t *);

/* MACROS */

#endif /* __UCONTEXT_H_INCLUDED__ */

/* EOF */

