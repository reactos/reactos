/*
 * Copyright (c) 1996-1998 University of Utah and the Flux Group.
 * All rights reserved.
 * 
 * This file is part of the Flux OSKit.  The OSKit is free software, also known
 * as "open source;" you can redistribute it and/or modify it under the terms
 * of the GNU General Public License (GPL), version 2, as published by the Free
 * Software Foundation (FSF).  To explore alternate licensing terms, contact
 * the University of Utah at csl-dist@cs.utah.edu or +1-801-585-3271.
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */
/*
 * Routines necessary for the bsdnet code.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <vm/vm.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/domain.h>
#include "net/netisr.h"

int oskit_cpl;		/* for machine/spl.h */
int bootverbose;	/* networking code wants to know whether booting 
			   is to be verbose */
int securelevel = 3;	/* used in ip_fw.c:ip_fw_ctl -- make it > 2 */

vm_map_t	mb_map; /* this is passed in kmem_alloc, but ignored there */

struct  proc proc0;
struct  proc *curproc = &proc0;

struct domain localdomain;	/* see uipc_domain.c ADDDOMAIN macro */

/* ---------------------------------------------------------------------- */

/*
 * find a process by pid
 */
struct proc *
pfind(pid_t pid)
{
	printf("%s called, pid=%d, returning x%p\n", 
	       __FUNCTION__, (int)pid, (void*)&proc0);
	return &proc0;
}

/*
 * signal a process 
 */
void    
psignal (struct proc *p, int sig)
{
    printf("%s called, proc=x%p sig=%d\n", __FUNCTION__, p, sig);
}

/*
 * signal a process group
 */
void    
gsignal (int pgid, int sig)
{
	printf("%s called, pgid=%d sig=%d\n", __FUNCTION__, pgid, sig);
}

/* ---------------------------------------------------------------------- */

/*
 * copy in from userspace
 */
int     
copyin (void *udaddr, void *kaddr, u_int len)
{
	memcpy(kaddr, udaddr, len);
	return 0;
}

/*
 * copy out to userspace
 */
int     
copyout (void *kaddr, void *udaddr, u_int len)
{
	memcpy(udaddr, kaddr, len);
	return 0;
}

/*
 * even though these functions have an odd signature, 
 * they only copy one byte
 */
int subyte          (void *base, int byte)
{
    return (int)(base = (char *)byte);
}

int suibyte         (void *base, int byte)
{
	return (int)(base = (char *)byte);
}

/* ---------------------------------------------------------------------- */

#ifndef __REACTOS__
/*
 * log some information
 */
void        
log (int level, const char *format, ...)
{
	extern int vprintf(const char *, va_list);
	va_list args;
	va_start(args, format);
	printf("__FUNCTION__(%d):", level);
	vprintf(format, args);
	va_end(args);
}
#endif

/* ---------------------------------------------------------------------- */

/* 
 * do we have super user credentials? 
 */
/* ARGSUSED */
int 
suser(struct ucred *ucred, u_short *acflag)
{
	/* of course. */
	return 0;
}

/* ---------------------------------------------------------------------- */
/* 
 * stuff stolen from kern/kern_sysctl.c
 */
/*              
 * Validate parameters and get old / set new parameters
 * for an integer-valued sysctl function.
 */
int     
sysctl_int(oldp, oldlenp, newp, newlen, valp)
        void *oldp;
        size_t *oldlenp;
        void *newp;
        size_t newlen; 
        int *valp;
{       
        int error = 0; 
        
        if (oldp && *oldlenp < sizeof(int))
                return (ENOMEM);
        if (newp && newlen != sizeof(int))
                return (EINVAL);
        *oldlenp = sizeof(int);
        if (oldp)
                error = copyout(valp, oldp, sizeof(int));
        if (error == 0 && newp)
                error = copyin(newp, valp, sizeof(int));
        return (error);            
}               

/* 
 * Validate parameters and get old parameters
 * for a structure oriented sysctl function.
 */
int
sysctl_rdstruct(oldp, oldlenp, newp, sp, len)
        void *oldp;
        size_t *oldlenp;
        void *newp, *sp;
        int len;
{ 
        int error = 0;
 
        if (oldp && *oldlenp < len)
                return (ENOMEM);
        if (newp)
                return (EPERM);
        *oldlenp = len;
        if (oldp)
                error = copyout(sp, oldp, len);
        return (error);
}

/* ---------------------------------------------------------------------- */
/* 
 * normally, this is a builtin function in gcc
 * net/if.c doesn't seem to get it, though
 */
static int
memcmp(const void *s1v, const void *s2v, size_t size)
{
        register const char *s1 = s1v, *s2 = s2v;
        register unsigned int a, b;

        while (size-- > 0) {
                if ((a = *s1++) != (b = *s2++))
                        return (a-b);
        }

        return 0;
}

int bcmp(const void *b1, const void *b2, size_t len)
{
	return memcmp(b1, b2, len);
}

