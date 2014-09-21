/*
 * Revision 12: http://theos.com/~deraadt/snprintf.c
 *
 * Copyright (c) 1997 Theo de Raadt
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __VMS
# include <sys/param.h>
#endif
#include <sys/types.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdio.h>
#if __STDC__
#include <stdarg.h>
#include <stdlib.h>
#else
#include <varargs.h>
#endif
#include <setjmp.h>
#include <unistd.h>
#include <string.h>

#ifndef roundup
#define roundup(x, y) ((((x)+((y)-1))/(y))*(y))
#endif

#ifdef __sgi
#define size_t ssize_t
#endif

static int pgsize;
static char *curobj;
static int caught;
static sigjmp_buf bail;

#define EXTRABYTES	2	/* XXX: why 2? you don't want to know */

static char *
msetup(str, n)
	char *str;
	size_t n;
{
	char *e;

	if (n == 0)
		return NULL;
	if (pgsize == 0)
		pgsize = getpagesize();
	curobj = (char *)malloc(n + EXTRABYTES + pgsize * 2);
	if (curobj == NULL)
		return NULL;
	e = curobj + n + EXTRABYTES;
	e = (char *)roundup((unsigned long)e, pgsize);
	if (mprotect(e, pgsize, PROT_NONE) == -1) {
		free(curobj);
		curobj = NULL;
		return NULL;
	}
	e = e - n - EXTRABYTES;
	*e = '\0';
	return (e);
}

static void
  mcatch( int a )
{
	siglongjmp(bail, 1);
}

static void
mcleanup(str, n, p)
	char *str;
	size_t n;
	char *p;
{
	strncpy(str, p, n-1);
	str[n-1] = '\0';
	if (mprotect((caddr_t)(p + n + EXTRABYTES), pgsize,
	    PROT_READ|PROT_WRITE|PROT_EXEC) == -1)
		mprotect((caddr_t)(p + n + EXTRABYTES), pgsize,
		    PROT_READ|PROT_WRITE);
	free(curobj);
}

int
#if __STDC__
vsnprintf(char *str, size_t n, char const *fmt, va_list ap)
#else
vsnprintf(str, n, fmt, ap)
	char *str;
	size_t n;
	char *fmt;
	char *ap;
#endif
{
	struct sigaction osa, nsa;
	char *p;
	int ret = n + 1;	/* if we bail, indicated we overflowed */

	memset(&nsa, 0, sizeof nsa);
	nsa.sa_handler = mcatch;
	sigemptyset(&nsa.sa_mask);

	p = msetup(str, n);
	if (p == NULL) {
		*str = '\0';
		return 0;
	}
	if (sigsetjmp(bail, 1) == 0) {
		if (sigaction(SIGSEGV, &nsa, &osa) == -1) {
			mcleanup(str, n, p);
			return (0);
		}
		ret = vsprintf(p, fmt, ap);
	}
	mcleanup(str, n, p);
	(void) sigaction(SIGSEGV, &osa, NULL);
	return (ret);
}

int
#if __STDC__
snprintf(char *str, size_t n, char const *fmt, ...)
#else
snprintf(str, n, fmt, va_alist)
	char *str;
	size_t n;
	char *fmt;
	va_dcl
#endif
{
	va_list ap;
#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif

	return (vsnprintf(str, n, fmt, ap));
	va_end(ap);
}



