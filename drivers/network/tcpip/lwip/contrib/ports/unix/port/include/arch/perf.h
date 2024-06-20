/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#ifndef LWIP_ARCH_PERF_H
#define LWIP_ARCH_PERF_H

#include <sys/times.h>

#ifdef PERF
#define PERF_START  { \
                         unsigned long __c1l, __c1h, __c2l, __c2h; \
                         __asm__(".byte 0x0f, 0x31" : "=a" (__c1l), "=d" (__c1h))
#define PERF_STOP(x)   __asm__(".byte 0x0f, 0x31" : "=a" (__c2l), "=d" (__c2h)); \
                       perf_print(__c1l, __c1h, __c2l, __c2h, x);}

/*#define PERF_START do { \
                     struct tms __perf_start, __perf_end; \
                     times(&__perf_start)
#define PERF_STOP(x) times(&__perf_end); \
                     perf_print_times(&__perf_start, &__perf_end, x);\
                     } while(0)*/
#else /* PERF */
#define PERF_START    /* null definition */
#define PERF_STOP(x)  /* null definition */
#endif /* PERF */

void perf_print(unsigned long c1l, unsigned long c1h,
		unsigned long c2l, unsigned long c2h,
		char *key);

void perf_print_times(struct tms *start, struct tms *end, char *key);

void perf_init(char *fname);

#endif /* LWIP_ARCH_PERF_H */
