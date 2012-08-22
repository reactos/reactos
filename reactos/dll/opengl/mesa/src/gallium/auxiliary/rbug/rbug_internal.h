/*
 * Copyright 2009 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VMWARE AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * This file is internal to the rbug protocol code, and contains asorted
 * features needed by the code.
 */

#ifndef _RBUG_INTERNAL_H_
#define _RBUG_INTERNAL_H_

#include "rbug/rbug_proto.h"

#include "util/u_memory.h"
#include "util/u_debug.h"
#include <errno.h>

int rbug_connection_send_start(struct rbug_connection *con, enum rbug_opcode opcode, uint32_t length);
int rbug_connection_write(struct rbug_connection *con, void *data, uint32_t size);
int rbug_connection_send_finish(struct rbug_connection *con, uint32_t *c);

/**
 * Only works with multiples of 2
 */
#define PAD(from, to)                       \
do {                                        \
	from = (from + to - 1) & ~(to - 1); \
} while(0)

#define LEN(size)         \
do {                      \
	PAD(__len, size); \
	__len += size;    \
} while(0)

#define LEN_ARRAY(size, name)       \
do {                                \
	LEN(4);                     \
	PAD(__len, size);           \
	__len += size * name##_len; \
} while(0)

#define WRITE(size, type, name)             \
do {                                        \
	PAD(__pos, size);                   \
	*((type *)(&__data[__pos])) = name; \
	__pos += size;                      \
} while(0)

#define WRITE_ARRAY(size, type, name)                    \
do {                                                     \
	WRITE(4, uint32_t, name##_len);                  \
	PAD(__pos, size);                                \
	memcpy(&__data[__pos], name, size * name##_len); \
	__pos += size * name##_len;                      \
} while(0)

#define READ(size, type, name)                      \
do {                                                \
	PAD(pos, size);                             \
	pos += size;                                \
	if (pos > len)                              \
		break;                              \
	ret->name = *((type *)(&data[pos - size])); \
} while(0)

#define READ_ARRAY(size, type, name)                             \
do {                                                             \
	READ(4, uint32_t, name##_len);                           \
	if (pos > len)                                           \
		break;                                           \
	PAD(pos, size);                                          \
	pos += size * ret->name##_len;                           \
	if (pos > len)                                           \
		break;                                           \
	ret->name = (type *)&data[pos - size * ret->name##_len]; \
} while(0)

#endif
