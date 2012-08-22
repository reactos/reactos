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

#ifndef _RBUG_PROTO_CONTEXT_H_
#define _RBUG_PROTO_CONTEXT_H_

#include "rbug/rbug_proto.h"
#include "rbug/rbug_core.h"

typedef enum
{
	RBUG_BLOCK_BEFORE = 1,
	RBUG_BLOCK_AFTER = 2,
	RBUG_BLOCK_RULE = 4,
	RBUG_BLOCK_MASK = 7
} rbug_block_t;

struct rbug_proto_context_list
{
	struct rbug_header header;
};

struct rbug_proto_context_info
{
	struct rbug_header header;
	rbug_context_t context;
};

struct rbug_proto_context_draw_block
{
	struct rbug_header header;
	rbug_context_t context;
	rbug_block_t block;
};

struct rbug_proto_context_draw_step
{
	struct rbug_header header;
	rbug_context_t context;
	rbug_block_t step;
};

struct rbug_proto_context_draw_unblock
{
	struct rbug_header header;
	rbug_context_t context;
	rbug_block_t unblock;
};

struct rbug_proto_context_draw_rule
{
	struct rbug_header header;
	rbug_context_t context;
	rbug_shader_t vertex;
	rbug_shader_t fragment;
	rbug_texture_t texture;
	rbug_texture_t surface;
	rbug_block_t block;
};

struct rbug_proto_context_flush
{
	struct rbug_header header;
	rbug_context_t context;
};

struct rbug_proto_context_list_reply
{
	struct rbug_header header;
	uint32_t serial;
	rbug_context_t *contexts;
	uint32_t contexts_len;
};

struct rbug_proto_context_info_reply
{
	struct rbug_header header;
	uint32_t serial;
	rbug_shader_t vertex;
	rbug_shader_t fragment;
	rbug_texture_t *texs;
	uint32_t texs_len;
	rbug_texture_t *cbufs;
	uint32_t cbufs_len;
	rbug_texture_t zsbuf;
	rbug_block_t blocker;
	rbug_block_t blocked;
};

struct rbug_proto_context_draw_blocked
{
	struct rbug_header header;
	rbug_context_t context;
	rbug_block_t block;
};

int rbug_send_context_list(struct rbug_connection *__con,
                           uint32_t *__serial);

int rbug_send_context_info(struct rbug_connection *__con,
                           rbug_context_t context,
                           uint32_t *__serial);

int rbug_send_context_draw_block(struct rbug_connection *__con,
                                 rbug_context_t context,
                                 rbug_block_t block,
                                 uint32_t *__serial);

int rbug_send_context_draw_step(struct rbug_connection *__con,
                                rbug_context_t context,
                                rbug_block_t step,
                                uint32_t *__serial);

int rbug_send_context_draw_unblock(struct rbug_connection *__con,
                                   rbug_context_t context,
                                   rbug_block_t unblock,
                                   uint32_t *__serial);

int rbug_send_context_draw_rule(struct rbug_connection *__con,
                                rbug_context_t context,
                                rbug_shader_t vertex,
                                rbug_shader_t fragment,
                                rbug_texture_t texture,
                                rbug_texture_t surface,
                                rbug_block_t block,
                                uint32_t *__serial);

int rbug_send_context_flush(struct rbug_connection *__con,
                            rbug_context_t context,
                            uint32_t *__serial);

int rbug_send_context_list_reply(struct rbug_connection *__con,
                                 uint32_t serial,
                                 rbug_context_t *contexts,
                                 uint32_t contexts_len,
                                 uint32_t *__serial);

int rbug_send_context_info_reply(struct rbug_connection *__con,
                                 uint32_t serial,
                                 rbug_shader_t vertex,
                                 rbug_shader_t fragment,
                                 rbug_texture_t *texs,
                                 uint32_t texs_len,
                                 rbug_texture_t *cbufs,
                                 uint32_t cbufs_len,
                                 rbug_texture_t zsbuf,
                                 rbug_block_t blocker,
                                 rbug_block_t blocked,
                                 uint32_t *__serial);

int rbug_send_context_draw_blocked(struct rbug_connection *__con,
                                   rbug_context_t context,
                                   rbug_block_t block,
                                   uint32_t *__serial);

struct rbug_proto_context_list * rbug_demarshal_context_list(struct rbug_proto_header *header);

struct rbug_proto_context_info * rbug_demarshal_context_info(struct rbug_proto_header *header);

struct rbug_proto_context_draw_block * rbug_demarshal_context_draw_block(struct rbug_proto_header *header);

struct rbug_proto_context_draw_step * rbug_demarshal_context_draw_step(struct rbug_proto_header *header);

struct rbug_proto_context_draw_unblock * rbug_demarshal_context_draw_unblock(struct rbug_proto_header *header);

struct rbug_proto_context_draw_rule * rbug_demarshal_context_draw_rule(struct rbug_proto_header *header);

struct rbug_proto_context_flush * rbug_demarshal_context_flush(struct rbug_proto_header *header);

struct rbug_proto_context_list_reply * rbug_demarshal_context_list_reply(struct rbug_proto_header *header);

struct rbug_proto_context_info_reply * rbug_demarshal_context_info_reply(struct rbug_proto_header *header);

struct rbug_proto_context_draw_blocked * rbug_demarshal_context_draw_blocked(struct rbug_proto_header *header);

#endif
