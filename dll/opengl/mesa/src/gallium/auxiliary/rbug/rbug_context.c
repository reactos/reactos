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
 * This file holds the function implementation for one of the rbug extensions.
 * Prototypes and declerations of functions and structs is in the same folder
 * in the header file matching this file's name.
 *
 * The functions starting rbug_send_* encodes a call to the write format and
 * sends that to the supplied connection, while functions starting with
 * rbug_demarshal_* demarshal data in the wire protocol.
 *
 * Functions ending with _reply are replies to requests.
 */

#include "rbug_internal.h"
#include "rbug/rbug_context.h"

int rbug_send_context_list(struct rbug_connection *__con,
                           uint32_t *__serial)
{
	uint32_t __len = 0;
	uint32_t __pos = 0;
	uint8_t *__data = NULL;
	int __ret = 0;

	LEN(8); /* header */

	/* align */
	PAD(__len, 8);

	__data = (uint8_t*)MALLOC(__len);
	if (!__data)
		return -ENOMEM;

	WRITE(4, int32_t, ((int32_t)RBUG_OP_CONTEXT_LIST));
	WRITE(4, uint32_t, ((uint32_t)(__len / 4)));

	/* final pad */
	PAD(__pos, 8);

	if (__pos != __len) {
		__ret = -EINVAL;
	} else {
		rbug_connection_send_start(__con, RBUG_OP_CONTEXT_LIST, __len);
		rbug_connection_write(__con, __data, __len);
		__ret = rbug_connection_send_finish(__con, __serial);
	}

	FREE(__data);
	return __ret;
}

int rbug_send_context_info(struct rbug_connection *__con,
                           rbug_context_t context,
                           uint32_t *__serial)
{
	uint32_t __len = 0;
	uint32_t __pos = 0;
	uint8_t *__data = NULL;
	int __ret = 0;

	LEN(8); /* header */
	LEN(8); /* context */

	/* align */
	PAD(__len, 8);

	__data = (uint8_t*)MALLOC(__len);
	if (!__data)
		return -ENOMEM;

	WRITE(4, int32_t, ((int32_t)RBUG_OP_CONTEXT_INFO));
	WRITE(4, uint32_t, ((uint32_t)(__len / 4)));
	WRITE(8, rbug_context_t, context); /* context */

	/* final pad */
	PAD(__pos, 8);

	if (__pos != __len) {
		__ret = -EINVAL;
	} else {
		rbug_connection_send_start(__con, RBUG_OP_CONTEXT_INFO, __len);
		rbug_connection_write(__con, __data, __len);
		__ret = rbug_connection_send_finish(__con, __serial);
	}

	FREE(__data);
	return __ret;
}

int rbug_send_context_draw_block(struct rbug_connection *__con,
                                 rbug_context_t context,
                                 rbug_block_t block,
                                 uint32_t *__serial)
{
	uint32_t __len = 0;
	uint32_t __pos = 0;
	uint8_t *__data = NULL;
	int __ret = 0;

	LEN(8); /* header */
	LEN(8); /* context */
	LEN(4); /* block */

	/* align */
	PAD(__len, 8);

	__data = (uint8_t*)MALLOC(__len);
	if (!__data)
		return -ENOMEM;

	WRITE(4, int32_t, ((int32_t)RBUG_OP_CONTEXT_DRAW_BLOCK));
	WRITE(4, uint32_t, ((uint32_t)(__len / 4)));
	WRITE(8, rbug_context_t, context); /* context */
	WRITE(4, rbug_block_t, block); /* block */

	/* final pad */
	PAD(__pos, 8);

	if (__pos != __len) {
		__ret = -EINVAL;
	} else {
		rbug_connection_send_start(__con, RBUG_OP_CONTEXT_DRAW_BLOCK, __len);
		rbug_connection_write(__con, __data, __len);
		__ret = rbug_connection_send_finish(__con, __serial);
	}

	FREE(__data);
	return __ret;
}

int rbug_send_context_draw_step(struct rbug_connection *__con,
                                rbug_context_t context,
                                rbug_block_t step,
                                uint32_t *__serial)
{
	uint32_t __len = 0;
	uint32_t __pos = 0;
	uint8_t *__data = NULL;
	int __ret = 0;

	LEN(8); /* header */
	LEN(8); /* context */
	LEN(4); /* step */

	/* align */
	PAD(__len, 8);

	__data = (uint8_t*)MALLOC(__len);
	if (!__data)
		return -ENOMEM;

	WRITE(4, int32_t, ((int32_t)RBUG_OP_CONTEXT_DRAW_STEP));
	WRITE(4, uint32_t, ((uint32_t)(__len / 4)));
	WRITE(8, rbug_context_t, context); /* context */
	WRITE(4, rbug_block_t, step); /* step */

	/* final pad */
	PAD(__pos, 8);

	if (__pos != __len) {
		__ret = -EINVAL;
	} else {
		rbug_connection_send_start(__con, RBUG_OP_CONTEXT_DRAW_STEP, __len);
		rbug_connection_write(__con, __data, __len);
		__ret = rbug_connection_send_finish(__con, __serial);
	}

	FREE(__data);
	return __ret;
}

int rbug_send_context_draw_unblock(struct rbug_connection *__con,
                                   rbug_context_t context,
                                   rbug_block_t unblock,
                                   uint32_t *__serial)
{
	uint32_t __len = 0;
	uint32_t __pos = 0;
	uint8_t *__data = NULL;
	int __ret = 0;

	LEN(8); /* header */
	LEN(8); /* context */
	LEN(4); /* unblock */

	/* align */
	PAD(__len, 8);

	__data = (uint8_t*)MALLOC(__len);
	if (!__data)
		return -ENOMEM;

	WRITE(4, int32_t, ((int32_t)RBUG_OP_CONTEXT_DRAW_UNBLOCK));
	WRITE(4, uint32_t, ((uint32_t)(__len / 4)));
	WRITE(8, rbug_context_t, context); /* context */
	WRITE(4, rbug_block_t, unblock); /* unblock */

	/* final pad */
	PAD(__pos, 8);

	if (__pos != __len) {
		__ret = -EINVAL;
	} else {
		rbug_connection_send_start(__con, RBUG_OP_CONTEXT_DRAW_UNBLOCK, __len);
		rbug_connection_write(__con, __data, __len);
		__ret = rbug_connection_send_finish(__con, __serial);
	}

	FREE(__data);
	return __ret;
}

int rbug_send_context_draw_rule(struct rbug_connection *__con,
                                rbug_context_t context,
                                rbug_shader_t vertex,
                                rbug_shader_t fragment,
                                rbug_texture_t texture,
                                rbug_texture_t surface,
                                rbug_block_t block,
                                uint32_t *__serial)
{
	uint32_t __len = 0;
	uint32_t __pos = 0;
	uint8_t *__data = NULL;
	int __ret = 0;

	LEN(8); /* header */
	LEN(8); /* context */
	LEN(8); /* vertex */
	LEN(8); /* fragment */
	LEN(8); /* texture */
	LEN(8); /* surface */
	LEN(4); /* block */

	/* align */
	PAD(__len, 8);

	__data = (uint8_t*)MALLOC(__len);
	if (!__data)
		return -ENOMEM;

	WRITE(4, int32_t, ((int32_t)RBUG_OP_CONTEXT_DRAW_RULE));
	WRITE(4, uint32_t, ((uint32_t)(__len / 4)));
	WRITE(8, rbug_context_t, context); /* context */
	WRITE(8, rbug_shader_t, vertex); /* vertex */
	WRITE(8, rbug_shader_t, fragment); /* fragment */
	WRITE(8, rbug_texture_t, texture); /* texture */
	WRITE(8, rbug_texture_t, surface); /* surface */
	WRITE(4, rbug_block_t, block); /* block */

	/* final pad */
	PAD(__pos, 8);

	if (__pos != __len) {
		__ret = -EINVAL;
	} else {
		rbug_connection_send_start(__con, RBUG_OP_CONTEXT_DRAW_RULE, __len);
		rbug_connection_write(__con, __data, __len);
		__ret = rbug_connection_send_finish(__con, __serial);
	}

	FREE(__data);
	return __ret;
}

int rbug_send_context_flush(struct rbug_connection *__con,
                            rbug_context_t context,
                            uint32_t *__serial)
{
	uint32_t __len = 0;
	uint32_t __pos = 0;
	uint8_t *__data = NULL;
	int __ret = 0;

	LEN(8); /* header */
	LEN(8); /* context */

	/* align */
	PAD(__len, 8);

	__data = (uint8_t*)MALLOC(__len);
	if (!__data)
		return -ENOMEM;

	WRITE(4, int32_t, ((int32_t)RBUG_OP_CONTEXT_FLUSH));
	WRITE(4, uint32_t, ((uint32_t)(__len / 4)));
	WRITE(8, rbug_context_t, context); /* context */

	/* final pad */
	PAD(__pos, 8);

	if (__pos != __len) {
		__ret = -EINVAL;
	} else {
		rbug_connection_send_start(__con, RBUG_OP_CONTEXT_FLUSH, __len);
		rbug_connection_write(__con, __data, __len);
		__ret = rbug_connection_send_finish(__con, __serial);
	}

	FREE(__data);
	return __ret;
}

int rbug_send_context_list_reply(struct rbug_connection *__con,
                                 uint32_t serial,
                                 rbug_context_t *contexts,
                                 uint32_t contexts_len,
                                 uint32_t *__serial)
{
	uint32_t __len = 0;
	uint32_t __pos = 0;
	uint8_t *__data = NULL;
	int __ret = 0;

	LEN(8); /* header */
	LEN(4); /* serial */
	LEN_ARRAY(8, contexts); /* contexts */

	/* align */
	PAD(__len, 8);

	__data = (uint8_t*)MALLOC(__len);
	if (!__data)
		return -ENOMEM;

	WRITE(4, int32_t, ((int32_t)RBUG_OP_CONTEXT_LIST_REPLY));
	WRITE(4, uint32_t, ((uint32_t)(__len / 4)));
	WRITE(4, uint32_t, serial); /* serial */
	WRITE_ARRAY(8, rbug_context_t, contexts); /* contexts */

	/* final pad */
	PAD(__pos, 8);

	if (__pos != __len) {
		__ret = -EINVAL;
	} else {
		rbug_connection_send_start(__con, RBUG_OP_CONTEXT_LIST_REPLY, __len);
		rbug_connection_write(__con, __data, __len);
		__ret = rbug_connection_send_finish(__con, __serial);
	}

	FREE(__data);
	return __ret;
}

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
                                 uint32_t *__serial)
{
	uint32_t __len = 0;
	uint32_t __pos = 0;
	uint8_t *__data = NULL;
	int __ret = 0;

	LEN(8); /* header */
	LEN(4); /* serial */
	LEN(8); /* vertex */
	LEN(8); /* fragment */
	LEN_ARRAY(8, texs); /* texs */
	LEN_ARRAY(8, cbufs); /* cbufs */
	LEN(8); /* zsbuf */
	LEN(4); /* blocker */
	LEN(4); /* blocked */

	/* align */
	PAD(__len, 8);

	__data = (uint8_t*)MALLOC(__len);
	if (!__data)
		return -ENOMEM;

	WRITE(4, int32_t, ((int32_t)RBUG_OP_CONTEXT_INFO_REPLY));
	WRITE(4, uint32_t, ((uint32_t)(__len / 4)));
	WRITE(4, uint32_t, serial); /* serial */
	WRITE(8, rbug_shader_t, vertex); /* vertex */
	WRITE(8, rbug_shader_t, fragment); /* fragment */
	WRITE_ARRAY(8, rbug_texture_t, texs); /* texs */
	WRITE_ARRAY(8, rbug_texture_t, cbufs); /* cbufs */
	WRITE(8, rbug_texture_t, zsbuf); /* zsbuf */
	WRITE(4, rbug_block_t, blocker); /* blocker */
	WRITE(4, rbug_block_t, blocked); /* blocked */

	/* final pad */
	PAD(__pos, 8);

	if (__pos != __len) {
		__ret = -EINVAL;
	} else {
		rbug_connection_send_start(__con, RBUG_OP_CONTEXT_INFO_REPLY, __len);
		rbug_connection_write(__con, __data, __len);
		__ret = rbug_connection_send_finish(__con, __serial);
	}

	FREE(__data);
	return __ret;
}

int rbug_send_context_draw_blocked(struct rbug_connection *__con,
                                   rbug_context_t context,
                                   rbug_block_t block,
                                   uint32_t *__serial)
{
	uint32_t __len = 0;
	uint32_t __pos = 0;
	uint8_t *__data = NULL;
	int __ret = 0;

	LEN(8); /* header */
	LEN(8); /* context */
	LEN(4); /* block */

	/* align */
	PAD(__len, 8);

	__data = (uint8_t*)MALLOC(__len);
	if (!__data)
		return -ENOMEM;

	WRITE(4, int32_t, ((int32_t)RBUG_OP_CONTEXT_DRAW_BLOCKED));
	WRITE(4, uint32_t, ((uint32_t)(__len / 4)));
	WRITE(8, rbug_context_t, context); /* context */
	WRITE(4, rbug_block_t, block); /* block */

	/* final pad */
	PAD(__pos, 8);

	if (__pos != __len) {
		__ret = -EINVAL;
	} else {
		rbug_connection_send_start(__con, RBUG_OP_CONTEXT_DRAW_BLOCKED, __len);
		rbug_connection_write(__con, __data, __len);
		__ret = rbug_connection_send_finish(__con, __serial);
	}

	FREE(__data);
	return __ret;
}

struct rbug_proto_context_list * rbug_demarshal_context_list(struct rbug_proto_header *header)
{
	struct rbug_proto_context_list *ret;

	if (!header)
		return NULL;
	if (header->opcode != (int32_t)RBUG_OP_CONTEXT_LIST)
		return NULL;

	ret = MALLOC(sizeof(*ret));
	if (!ret)
		return NULL;

	ret->header.__message = header;
	ret->header.opcode = header->opcode;

	return ret;
}

struct rbug_proto_context_info * rbug_demarshal_context_info(struct rbug_proto_header *header)
{
	uint32_t len = 0;
	uint32_t pos = 0;
	uint8_t *data =  NULL;
	struct rbug_proto_context_info *ret;

	if (!header)
		return NULL;
	if (header->opcode != (int32_t)RBUG_OP_CONTEXT_INFO)
		return NULL;

	pos = 0;
	len = header->length * 4;
	data = (uint8_t*)&header[1];
	ret = MALLOC(sizeof(*ret));
	if (!ret)
		return NULL;

	ret->header.__message = header;
	ret->header.opcode = header->opcode;

	READ(8, rbug_context_t, context); /* context */

	return ret;
}

struct rbug_proto_context_draw_block * rbug_demarshal_context_draw_block(struct rbug_proto_header *header)
{
	uint32_t len = 0;
	uint32_t pos = 0;
	uint8_t *data =  NULL;
	struct rbug_proto_context_draw_block *ret;

	if (!header)
		return NULL;
	if (header->opcode != (int32_t)RBUG_OP_CONTEXT_DRAW_BLOCK)
		return NULL;

	pos = 0;
	len = header->length * 4;
	data = (uint8_t*)&header[1];
	ret = MALLOC(sizeof(*ret));
	if (!ret)
		return NULL;

	ret->header.__message = header;
	ret->header.opcode = header->opcode;

	READ(8, rbug_context_t, context); /* context */
	READ(4, rbug_block_t, block); /* block */

	return ret;
}

struct rbug_proto_context_draw_step * rbug_demarshal_context_draw_step(struct rbug_proto_header *header)
{
	uint32_t len = 0;
	uint32_t pos = 0;
	uint8_t *data =  NULL;
	struct rbug_proto_context_draw_step *ret;

	if (!header)
		return NULL;
	if (header->opcode != (int32_t)RBUG_OP_CONTEXT_DRAW_STEP)
		return NULL;

	pos = 0;
	len = header->length * 4;
	data = (uint8_t*)&header[1];
	ret = MALLOC(sizeof(*ret));
	if (!ret)
		return NULL;

	ret->header.__message = header;
	ret->header.opcode = header->opcode;

	READ(8, rbug_context_t, context); /* context */
	READ(4, rbug_block_t, step); /* step */

	return ret;
}

struct rbug_proto_context_draw_unblock * rbug_demarshal_context_draw_unblock(struct rbug_proto_header *header)
{
	uint32_t len = 0;
	uint32_t pos = 0;
	uint8_t *data =  NULL;
	struct rbug_proto_context_draw_unblock *ret;

	if (!header)
		return NULL;
	if (header->opcode != (int32_t)RBUG_OP_CONTEXT_DRAW_UNBLOCK)
		return NULL;

	pos = 0;
	len = header->length * 4;
	data = (uint8_t*)&header[1];
	ret = MALLOC(sizeof(*ret));
	if (!ret)
		return NULL;

	ret->header.__message = header;
	ret->header.opcode = header->opcode;

	READ(8, rbug_context_t, context); /* context */
	READ(4, rbug_block_t, unblock); /* unblock */

	return ret;
}

struct rbug_proto_context_draw_rule * rbug_demarshal_context_draw_rule(struct rbug_proto_header *header)
{
	uint32_t len = 0;
	uint32_t pos = 0;
	uint8_t *data =  NULL;
	struct rbug_proto_context_draw_rule *ret;

	if (!header)
		return NULL;
	if (header->opcode != (int32_t)RBUG_OP_CONTEXT_DRAW_RULE)
		return NULL;

	pos = 0;
	len = header->length * 4;
	data = (uint8_t*)&header[1];
	ret = MALLOC(sizeof(*ret));
	if (!ret)
		return NULL;

	ret->header.__message = header;
	ret->header.opcode = header->opcode;

	READ(8, rbug_context_t, context); /* context */
	READ(8, rbug_shader_t, vertex); /* vertex */
	READ(8, rbug_shader_t, fragment); /* fragment */
	READ(8, rbug_texture_t, texture); /* texture */
	READ(8, rbug_texture_t, surface); /* surface */
	READ(4, rbug_block_t, block); /* block */

	return ret;
}

struct rbug_proto_context_flush * rbug_demarshal_context_flush(struct rbug_proto_header *header)
{
	uint32_t len = 0;
	uint32_t pos = 0;
	uint8_t *data =  NULL;
	struct rbug_proto_context_flush *ret;

	if (!header)
		return NULL;
	if (header->opcode != (int32_t)RBUG_OP_CONTEXT_FLUSH)
		return NULL;

	pos = 0;
	len = header->length * 4;
	data = (uint8_t*)&header[1];
	ret = MALLOC(sizeof(*ret));
	if (!ret)
		return NULL;

	ret->header.__message = header;
	ret->header.opcode = header->opcode;

	READ(8, rbug_context_t, context); /* context */

	return ret;
}

struct rbug_proto_context_list_reply * rbug_demarshal_context_list_reply(struct rbug_proto_header *header)
{
	uint32_t len = 0;
	uint32_t pos = 0;
	uint8_t *data =  NULL;
	struct rbug_proto_context_list_reply *ret;

	if (!header)
		return NULL;
	if (header->opcode != (int32_t)RBUG_OP_CONTEXT_LIST_REPLY)
		return NULL;

	pos = 0;
	len = header->length * 4;
	data = (uint8_t*)&header[1];
	ret = MALLOC(sizeof(*ret));
	if (!ret)
		return NULL;

	ret->header.__message = header;
	ret->header.opcode = header->opcode;

	READ(4, uint32_t, serial); /* serial */
	READ_ARRAY(8, rbug_context_t, contexts); /* contexts */

	return ret;
}

struct rbug_proto_context_info_reply * rbug_demarshal_context_info_reply(struct rbug_proto_header *header)
{
	uint32_t len = 0;
	uint32_t pos = 0;
	uint8_t *data =  NULL;
	struct rbug_proto_context_info_reply *ret;

	if (!header)
		return NULL;
	if (header->opcode != (int32_t)RBUG_OP_CONTEXT_INFO_REPLY)
		return NULL;

	pos = 0;
	len = header->length * 4;
	data = (uint8_t*)&header[1];
	ret = MALLOC(sizeof(*ret));
	if (!ret)
		return NULL;

	ret->header.__message = header;
	ret->header.opcode = header->opcode;

	READ(4, uint32_t, serial); /* serial */
	READ(8, rbug_shader_t, vertex); /* vertex */
	READ(8, rbug_shader_t, fragment); /* fragment */
	READ_ARRAY(8, rbug_texture_t, texs); /* texs */
	READ_ARRAY(8, rbug_texture_t, cbufs); /* cbufs */
	READ(8, rbug_texture_t, zsbuf); /* zsbuf */
	READ(4, rbug_block_t, blocker); /* blocker */
	READ(4, rbug_block_t, blocked); /* blocked */

	return ret;
}

struct rbug_proto_context_draw_blocked * rbug_demarshal_context_draw_blocked(struct rbug_proto_header *header)
{
	uint32_t len = 0;
	uint32_t pos = 0;
	uint8_t *data =  NULL;
	struct rbug_proto_context_draw_blocked *ret;

	if (!header)
		return NULL;
	if (header->opcode != (int32_t)RBUG_OP_CONTEXT_DRAW_BLOCKED)
		return NULL;

	pos = 0;
	len = header->length * 4;
	data = (uint8_t*)&header[1];
	ret = MALLOC(sizeof(*ret));
	if (!ret)
		return NULL;

	ret->header.__message = header;
	ret->header.opcode = header->opcode;

	READ(8, rbug_context_t, context); /* context */
	READ(4, rbug_block_t, block); /* block */

	return ret;
}
