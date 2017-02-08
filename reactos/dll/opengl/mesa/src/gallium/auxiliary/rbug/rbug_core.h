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
 * This file holds structs decelerations and function prototypes for one of
 * the rbug extensions. Implementation of the functions is in the same folder
 * in the c file matching this file's name.
 *
 * The structs what is returned from the demarshal functions. The functions
 * starting rbug_send_* encodes a call to the write format and sends that to
 * the supplied connection, while functions starting with rbug_demarshal_*
 * demarshal data from the wire protocol.
 *
 * Structs and functions ending with _reply are replies to requests.
 */

#ifndef _RBUG_PROTO_CORE_H_
#define _RBUG_PROTO_CORE_H_

#include "rbug/rbug_proto.h"

typedef uint64_t rbug_shader_t;
typedef uint64_t rbug_context_t;
typedef uint64_t rbug_texture_t;

struct rbug_proto_noop
{
	struct rbug_header header;
};

struct rbug_proto_ping
{
	struct rbug_header header;
};

struct rbug_proto_error
{
	struct rbug_header header;
	uint32_t error;
};

struct rbug_proto_ping_reply
{
	struct rbug_header header;
	uint32_t serial;
};

struct rbug_proto_error_reply
{
	struct rbug_header header;
	uint32_t serial;
	uint32_t error;
};

int rbug_send_noop(struct rbug_connection *__con,
                   uint32_t *__serial);

int rbug_send_ping(struct rbug_connection *__con,
                   uint32_t *__serial);

int rbug_send_error(struct rbug_connection *__con,
                    uint32_t error,
                    uint32_t *__serial);

int rbug_send_ping_reply(struct rbug_connection *__con,
                         uint32_t serial,
                         uint32_t *__serial);

int rbug_send_error_reply(struct rbug_connection *__con,
                          uint32_t serial,
                          uint32_t error,
                          uint32_t *__serial);

struct rbug_proto_noop * rbug_demarshal_noop(struct rbug_proto_header *header);

struct rbug_proto_ping * rbug_demarshal_ping(struct rbug_proto_header *header);

struct rbug_proto_error * rbug_demarshal_error(struct rbug_proto_header *header);

struct rbug_proto_ping_reply * rbug_demarshal_ping_reply(struct rbug_proto_header *header);

struct rbug_proto_error_reply * rbug_demarshal_error_reply(struct rbug_proto_header *header);

#endif
