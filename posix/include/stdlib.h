/* $Id: stdlib.h,v 1.2 2002/02/20 09:17:55 hyperion Exp $
 */
/*
 * stdlib.h
 *
 * standard library definitions. Conforming to the Single UNIX(r)
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
#ifndef __STDLIB_H_INCLUDED__
#define __STDLIB_H_INCLUDED__

/* INCLUDES */
#include <stddef.h>
#include <limits.h>
#include <math.h>
#include <sys/wait.h>

/* OBJECTS */

/* TYPES */
typedef struct __tagdiv_t
{
 int quot; /* quotient */
 int rem; /* remainder */
} div_t;

typedef struct __tagldiv_t
{
 long int quot; /* quotient */
 long int rem; /* remainder */
} ldiv_t;

/* CONSTANTS */
#define EXIT_FAILURE (-1) /* Unsuccessful termination for exit(), evaluates \
                             to a non-zero value. */
#define EXIT_SUCCESS (0) /* Successful termination for exit(), evaluates to 0. */

/* FIXME */
#define RAND_MAX (32767) /* Maximum value returned by rand(), at least 32,767. */

/* FIXME */
#define MB_CUR_MAX (1) /* Integer expression whose value is the maximum number \
                          of bytes in a character specified by the current \
                          locale. */

/* PROTOTYPES */
long      a64l(const char *);
void      abort(void);
int       abs(int);
int       atexit(void (*)(void));
double    atof(const char *);
int       atoi(const char *);
long int  atol(const char *);
void     *bsearch(const void *, const void *, size_t, size_t,
              int (*)(const void *, const void *));
void     *calloc(size_t, size_t);
div_t     div(int, int);
double    drand48(void);
char     *ecvt(double, int, int *, int *);
double    erand48(unsigned short int[3]);
void      exit(int);
char     *fcvt (double, int, int *, int *);
void      free(void *);
char     *gcvt(double, int, char *);
char     *getenv(const char *);
int       getsubopt(char **, char *const *, char **);
int       grantpt(int);
char     *initstate(unsigned int, char *, size_t);
long int  jrand48(unsigned short int[3]);
char     *l64a(long);
long int  labs(long int);
void      lcong48(unsigned short int[7]);
ldiv_t    ldiv(long int, long int);
long int  lrand48(void);
void     *malloc(size_t);
int       mblen(const char *, size_t);
size_t    mbstowcs(wchar_t *, const char *, size_t);
int       mbtowc(wchar_t *, const char *, size_t);
char     *mktemp(char *);
int       mkstemp(char *);
long int  mrand48(void);
long int  nrand48(unsigned short int [3]);
char     *ptsname(int);
int       putenv(char *);
void      qsort(void *, size_t, size_t, int (*)(const void *,
              const void *));
int       rand(void);
int       rand_r(unsigned int *);
long      random(void);
void     *realloc(void *, size_t);
char     *realpath(const char *, char *);
unsigned  short int    seed48(unsigned short int[3]);
void      setkey(const char *);
char     *setstate(const char *);
void      srand(unsigned int);
void      srand48(long int);
void      srandom(unsigned);
double    strtod(const char *, char **);
long int  strtol(const char *, char **, int);
unsigned long int
          strtoul(const char *, char **, int);
int       system(const char *);
int       ttyslot(void); /* LEGACY */
int       unlockpt(int);
void     *valloc(size_t); /* LEGACY */
size_t    wcstombs(char *, const wchar_t *, size_t);
int       wctomb(char *, wchar_t);

/* MACROS */

#endif /* __STDLIB_H_INCLUDED__ */

/* EOF */

