/**
 * @file
 * lwIP internal memory implementations (do not use in application code)
 */

/*
 * Copyright (c) 2018 Swedish Institute of Computer Science.
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
 * Author: Simon Goldschmidt <goldsimon@gmx.de>
 *
 */

#ifndef LWIP_HDR_MEM_PRIV_H
#define LWIP_HDR_MEM_PRIV_H

#include "lwip/opt.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "lwip/mem.h"

#if MEM_OVERFLOW_CHECK || MEMP_OVERFLOW_CHECK
/* if MEM_OVERFLOW_CHECK or MEMP_OVERFLOW_CHECK is turned on, we reserve some
 * bytes at the beginning and at the end of each element, initialize them as
 * 0xcd and check them later.
 * If MEM(P)_OVERFLOW_CHECK is >= 2, on every call to mem(p)_malloc or mem(p)_free,
 * every single element in each pool/heap is checked!
 * This is VERY SLOW but also very helpful.
 * MEM_SANITY_REGION_BEFORE and MEM_SANITY_REGION_AFTER can be overridden in
 * lwipopts.h to change the amount reserved for checking. */
#ifndef MEM_SANITY_REGION_BEFORE
#define MEM_SANITY_REGION_BEFORE  16
#endif /* MEM_SANITY_REGION_BEFORE*/
#if MEM_SANITY_REGION_BEFORE > 0
#define MEM_SANITY_REGION_BEFORE_ALIGNED    LWIP_MEM_ALIGN_SIZE(MEM_SANITY_REGION_BEFORE)
#else
#define MEM_SANITY_REGION_BEFORE_ALIGNED    0
#endif /* MEM_SANITY_REGION_BEFORE*/
#ifndef MEM_SANITY_REGION_AFTER
#define MEM_SANITY_REGION_AFTER   16
#endif /* MEM_SANITY_REGION_AFTER*/
#if MEM_SANITY_REGION_AFTER > 0
#define MEM_SANITY_REGION_AFTER_ALIGNED     LWIP_MEM_ALIGN_SIZE(MEM_SANITY_REGION_AFTER)
#else
#define MEM_SANITY_REGION_AFTER_ALIGNED     0
#endif /* MEM_SANITY_REGION_AFTER*/

void mem_overflow_init_raw(void *p, size_t size);
void mem_overflow_check_raw(void *p, size_t size, const char *descr1, const char *descr2);

#endif /* MEM_OVERFLOW_CHECK || MEMP_OVERFLOW_CHECK */

#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_MEMP_PRIV_H */
