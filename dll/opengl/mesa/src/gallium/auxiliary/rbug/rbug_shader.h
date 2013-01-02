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

#ifndef _RBUG_PROTO_SHADER_H_
#define _RBUG_PROTO_SHADER_H_

#include "rbug/rbug_proto.h"
#include "rbug/rbug_core.h"

struct rbug_proto_shader_list
{
	struct rbug_header header;
	rbug_context_t context;
};

struct rbug_proto_shader_info
{
	struct rbug_header header;
	rbug_context_t context;
	rbug_shader_t shader;
};

struct rbug_proto_shader_disable
{
	struct rbug_header header;
	rbug_context_t context;
	rbug_shader_t shader;
	uint8_t disable;
};

struct rbug_proto_shader_replace
{
	struct rbug_header header;
	rbug_context_t context;
	rbug_shader_t shader;
	uint32_t *tokens;
	uint32_t tokens_len;
};

struct rbug_proto_shader_list_reply
{
	struct rbug_header header;
	uint32_t serial;
	rbug_shader_t *shaders;
	uint32_t shaders_len;
};

struct rbug_proto_shader_info_reply
{
	struct rbug_header header;
	uint32_t serial;
	uint32_t *original;
	uint32_t original_len;
	uint32_t *replaced;
	uint32_t replaced_len;
	uint8_t disabled;
};

int rbug_send_shader_list(struct rbug_connection *__con,
                          rbug_context_t context,
                          uint32_t *__serial);

int rbug_send_shader_info(struct rbug_connection *__con,
                          rbug_context_t context,
                          rbug_shader_t shader,
                          uint32_t *__serial);

int rbug_send_shader_disable(struct rbug_connection *__con,
                             rbug_context_t context,
                             rbug_shader_t shader,
                             uint8_t disable,
                             uint32_t *__serial);

int rbug_send_shader_replace(struct rbug_connection *__con,
                             rbug_context_t context,
                             rbug_shader_t shader,
                             uint32_t *tokens,
                             uint32_t tokens_len,
                             uint32_t *__serial);

int rbug_send_shader_list_reply(struct rbug_connection *__con,
                                uint32_t serial,
                                rbug_shader_t *shaders,
                                uint32_t shaders_len,
                                uint32_t *__serial);

int rbug_send_shader_info_reply(struct rbug_connection *__con,
                                uint32_t serial,
                                uint32_t *original,
                                uint32_t original_len,
                                uint32_t *replaced,
                                uint32_t replaced_len,
                                uint8_t disabled,
                                uint32_t *__serial);

struct rbug_proto_shader_list * rbug_demarshal_shader_list(struct rbug_proto_header *header);

struct rbug_proto_shader_info * rbug_demarshal_shader_info(struct rbug_proto_header *header);

struct rbug_proto_shader_disable * rbug_demarshal_shader_disable(struct rbug_proto_header *header);

struct rbug_proto_shader_replace * rbug_demarshal_shader_replace(struct rbug_proto_header *header);

struct rbug_proto_shader_list_reply * rbug_demarshal_shader_list_reply(struct rbug_proto_header *header);

struct rbug_proto_shader_info_reply * rbug_demarshal_shader_info_reply(struct rbug_proto_header *header);

#endif
