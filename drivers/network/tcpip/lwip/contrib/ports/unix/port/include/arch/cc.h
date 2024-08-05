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
#ifndef LWIP_ARCH_CC_H
#define LWIP_ARCH_CC_H

/* see https://sourceforge.net/p/predef/wiki/OperatingSystems/ */
#if defined __ANDROID__
#define LWIP_UNIX_ANDROID
#elif defined __linux__
#define LWIP_UNIX_LINUX
#elif defined __APPLE__
#define LWIP_UNIX_MACH
#elif defined __OpenBSD__
#define LWIP_UNIX_OPENBSD
#elif defined __FreeBSD_kernel__ && __GLIBC__
#define LWIP_UNIX_KFREEBSD
#elif defined __CYGWIN__
#define LWIP_UNIX_CYGWIN
#elif defined __GNU__
#define LWIP_UNIX_HURD
#endif

#define LWIP_TIMEVAL_PRIVATE 0
#include <sys/time.h>

#define LWIP_ERRNO_INCLUDE <errno.h>

#if defined(LWIP_UNIX_LINUX) || defined(LWIP_UNIX_HURD) || defined(LWIP_UNIX_KFREEBSD)
#define LWIP_ERRNO_STDINCLUDE	1
#endif

extern unsigned int lwip_port_rand(void);
#define LWIP_RAND() (lwip_port_rand())

/* different handling for unit test, normally not needed */
#ifdef LWIP_NOASSERT_ON_ERROR
#define LWIP_ERROR(message, expression, handler) do { if (!(expression)) { \
  handler;}} while(0)
#endif

#if defined(LWIP_UNIX_ANDROID) && defined(FD_SET)
typedef __kernel_fd_set fd_set;
#endif

#if defined(LWIP_UNIX_MACH)
/* sys/types.h and signal.h bring in Darwin byte order macros. pull the
   header here and disable LwIP's version so that apps still can get
   the macros via LwIP headers and use system headers */
#include <sys/types.h>
#define LWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS
#endif

struct sio_status_s;
typedef struct sio_status_s sio_status_t;
#define sio_fd_t sio_status_t*
#define __sio_fd_t_defined

typedef unsigned int sys_prot_t;

#endif /* LWIP_ARCH_CC_H */
