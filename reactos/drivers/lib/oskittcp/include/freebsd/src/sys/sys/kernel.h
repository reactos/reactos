/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)kernel.h	8.3 (Berkeley) 1/21/94
 * $\Id: kernel.h,v 1.9 1995/03/20 19:20:26 wollman Exp $
 */

#ifndef _SYS_KERNEL_H_
#define _SYS_KERNEL_H_

/* Global variables for the kernel. */

/* 1.1 */
extern long hostid;
extern char hostname[MAXHOSTNAMELEN];
extern int hostnamelen;
extern char domainname[MAXHOSTNAMELEN];
extern int domainnamelen;
extern char kernelname[MAXPATHLEN];

/* 1.2 */
extern volatile struct timeval mono_time;
extern struct timeval boottime;
extern struct timeval runtime;
extern volatile struct timeval time;
extern struct timezone tz;			/* XXX */

extern int tick;			/* usec per tick (1000000 / hz) */
extern int hz;				/* system clock's frequency */
extern int stathz;			/* statistics clock's frequency */
extern int profhz;			/* profiling clock's frequency */
extern int ticks;
extern int lbolt;			/* once a second sleep address */
extern int tickdelta;
extern long timedelta;

/*
 * The following macros are used to declare global sets of objects, which
 * are collected by the linker into a `struct linker_set' as defined below.
 *
 * NB: the constants defined below must match those defined in
 * ld/ld.h.  Since their calculation requires arithmetic, we
 * can't name them symbolically (e.g., 23 is N_SETT | N_EXT).
 */
#define MAKE_SET(set, sym, type) \
	asm(".stabs \"_" #set "\", " #type ", 0, 0, _" #sym)
#define TEXT_SET(set, sym) MAKE_SET(set, sym, 23)
#define DATA_SET(set, sym) MAKE_SET(set, sym, 25)
#define BSS_SET(set, sym)  MAKE_SET(set, sym, 27)
#define ABS_SET(set, sym)  MAKE_SET(set, sym, 21)

#ifdef PSEUDO_LKM
#include <sys/conf.h>
#include <sys/exec.h>
#include <sys/sysent.h>
#include <sys/lkm.h>

#define PSEUDO_SET(init, name) \
	extern struct linker_set MODVNOPS; \
	MOD_MISC(#name); \
        int name ## _load(struct lkm_table *lkmtp, int cmd) \
		{ init(); return 0; } \
	int name ## _unload(struct lkm_table *lkmtp, int cmd) \
		{ return EINVAL; } \
	int \
	name ## _mod(struct lkm_table *lkmtp, int cmd, int ver) { \
		DISPATCH(lkmtp, cmd, ver, name ## _load, name ## _unload, \
			 nosys); }
#else /* PSEUDO_LKM */

#define PSEUDO_SET(sym, name)	   TEXT_SET(pseudo_set, sym)

#endif /* PSEUDO_LKM */

struct linker_set {
	int ls_length;
	caddr_t ls_items[1];	/* really ls_length of them, trailing NULL */
};

extern const struct linker_set execsw_set;

#endif
