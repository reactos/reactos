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
#include "rbug/rbug_texture.h"

int rbug_send_texture_list(struct rbug_connection *__con,
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

	WRITE(4, int32_t, ((int32_t)RBUG_OP_TEXTURE_LIST));
	WRITE(4, uint32_t, ((uint32_t)(__len / 4)));

	/* final pad */
	PAD(__pos, 8);

	if (__pos != __len) {
		__ret = -EINVAL;
	} else {
		rbug_connection_send_start(__con, RBUG_OP_TEXTURE_LIST, __len);
		rbug_connection_write(__con, __data, __len);
		__ret = rbug_connection_send_finish(__con, __serial);
	}

	FREE(__data);
	return __ret;
}

int rbug_send_texture_info(struct rbug_connection *__con,
                           rbug_texture_t texture,
                           uint32_t *__serial)
{
	uint32_t __len = 0;
	uint32_t __pos = 0;
	uint8_t *__data = NULL;
	int __ret = 0;

	LEN(8); /* header */
	LEN(8); /* texture */

	/* align */
	PAD(__len, 8);

	__data = (uint8_t*)MALLOC(__len);
	if (!__data)
		return -ENOMEM;

	WRITE(4, int32_t, ((int32_t)RBUG_OP_TEXTURE_INFO));
	WRITE(4, uint32_t, ((uint32_t)(__len / 4)));
	WRITE(8, rbug_texture_t, texture); /* texture */

	/* final pad */
	PAD(__pos, 8);

	if (__pos != __len) {
		__ret = -EINVAL;
	} else {
		rbug_connection_send_start(__con, RBUG_OP_TEXTURE_INFO, __len);
		rbug_connection_write(__con, __data, __len);
		__ret = rbug_connection_send_finish(__con, __serial);
	}

	FREE(__data);
	return __ret;
}

int rbug_send_texture_write(struct rbug_connection *__con,
                            rbug_texture_t texture,
                            uint32_t face,
                            uint32_t level,
                            uint32_t zslice,
                            uint32_t x,
                            uint32_t y,
                            uint32_t w,
                            uint32_t h,
                            uint8_t *data,
                            uint32_t data_len,
                            uint32_t stride,
                            uint32_t *__serial)
{
	uint32_t __len = 0;
	uint32_t __pos = 0;
	uint8_t *__data = NULL;
	int __ret = 0;

	LEN(8); /* header */
	LEN(8); /* texture */
	LEN(4); /* face */
	LEN(4); /* level */
	LEN(4); /* zslice */
	LEN(4); /* x */
	LEN(4); /* y */
	LEN(4); /* w */
	LEN(4); /* h */
	LEN_ARRAY(1, data); /* data */
	LEN(4); /* stride */

	/* align */
	PAD(__len, 8);

	__data = (uint8_t*)MALLOC(__len);
	if (!__data)
		return -ENOMEM;

	WRITE(4, int32_t, ((int32_t)RBUG_OP_TEXTURE_WRITE));
	WRITE(4, uint32_t, ((uint32_t)(__len / 4)));
	WRITE(8, rbug_texture_t, texture); /* texture */
	WRITE(4, uint32_t, face); /* face */
	WRITE(4, uint32_t, level); /* level */
	WRITE(4, uint32_t, zslice); /* zslice */
	WRITE(4, uint32_t, x); /* x */
	WRITE(4, uint32_t, y); /* y */
	WRITE(4, uint32_t, w); /* w */
	WRITE(4, uint32_t, h); /* h */
	WRITE_ARRAY(1, uint8_t, data); /* data */
	WRITE(4, uint32_t, stride); /* stride */

	/* final pad */
	PAD(__pos, 8);

	if (__pos != __len) {
		__ret = -EINVAL;
	} else {
		rbug_connection_send_start(__con, RBUG_OP_TEXTURE_WRITE, __len);
		rbug_connection_write(__con, __data, __len);
		__ret = rbug_connection_send_finish(__con, __serial);
	}

	FREE(__data);
	return __ret;
}

int rbug_send_texture_read(struct rbug_connection *__con,
                           rbug_texture_t texture,
                           uint32_t face,
                           uint32_t level,
                           uint32_t zslice,
                           uint32_t x,
                           uint32_t y,
                           uint32_t w,
                           uint32_t h,
                           uint32_t *__serial)
{
	uint32_t __len = 0;
	uint32_t __pos = 0;
	uint8_t *__data = NULL;
	int __ret = 0;

	LEN(8); /* header */
	LEN(8); /* texture */
	LEN(4); /* face */
	LEN(4); /* level */
	LEN(4); /* zslice */
	LEN(4); /* x */
	LEN(4); /* y */
	LEN(4); /* w */
	LEN(4); /* h */

	/* align */
	PAD(__len, 8);

	__data = (uint8_t*)MALLOC(__len);
	if (!__data)
		return -ENOMEM;

	WRITE(4, int32_t, ((int32_t)RBUG_OP_TEXTURE_READ));
	WRITE(4, uint32_t, ((uint32_t)(__len / 4)));
	WRITE(8, rbug_texture_t, texture); /* texture */
	WRITE(4, uint32_t, face); /* face */
	WRITE(4, uint32_t, level); /* level */
	WRITE(4, uint32_t, zslice); /* zslice */
	WRITE(4, uint32_t, x); /* x */
	WRITE(4, uint32_t, y); /* y */
	WRITE(4, uint32_t, w); /* w */
	WRITE(4, uint32_t, h); /* h */

	/* final pad */
	PAD(__pos, 8);

	if (__pos != __len) {
		__ret = -EINVAL;
	} else {
		rbug_connection_send_start(__con, RBUG_OP_TEXTURE_READ, __len);
		rbug_connection_write(__con, __data, __len);
		__ret = rbug_connection_send_finish(__con, __serial);
	}

	FREE(__data);
	return __ret;
}

int rbug_send_texture_list_reply(struct rbug_connection *__con,
                                 uint32_t serial,
                                 rbug_texture_t *textures,
                                 uint32_t textures_len,
                                 uint32_t *__serial)
{
	uint32_t __len = 0;
	uint32_t __pos = 0;
	uint8_t *__data = NULL;
	int __ret = 0;

	LEN(8); /* header */
	LEN(4); /* serial */
	LEN_ARRAY(8, textures); /* textures */

	/* align */
	PAD(__len, 8);

	__data = (uint8_t*)MALLOC(__len);
	if (!__data)
		return -ENOMEM;

	WRITE(4, int32_t, ((int32_t)RBUG_OP_TEXTURE_LIST_REPLY));
	WRITE(4, uint32_t, ((uint32_t)(__len / 4)));
	WRITE(4, uint32_t, serial); /* serial */
	WRITE_ARRAY(8, rbug_texture_t, textures); /* textures */

	/* final pad */
	PAD(__pos, 8);

	if (__pos != __len) {
		__ret = -EINVAL;
	} else {
		rbug_connection_send_start(__con, RBUG_OP_TEXTURE_LIST_REPLY, __len);
		rbug_connection_write(__con, __data, __len);
		__ret = rbug_connection_send_finish(__con, __serial);
	}

	FREE(__data);
	return __ret;
}

int rbug_send_texture_info_reply(struct rbug_connection *__con,
                                 uint32_t serial,
                                 uint32_t target,
                                 uint32_t format,
                                 uint32_t *width,
                                 uint32_t width_len,
                                 uint32_t *height,
                                 uint32_t height_len,
                                 uint32_t *depth,
                                 uint32_t depth_len,
                                 uint32_t blockw,
                                 uint32_t blockh,
                                 uint32_t blocksize,
                                 uint32_t last_level,
                                 uint32_t nr_samples,
                                 uint32_t tex_usage,
                                 uint32_t *__serial)
{
	uint32_t __len = 0;
	uint32_t __pos = 0;
	uint8_t *__data = NULL;
	int __ret = 0;

	LEN(8); /* header */
	LEN(4); /* serial */
	LEN(4); /* target */
	LEN(4); /* format */
	LEN_ARRAY(4, width); /* width */
	LEN_ARRAY(4, height); /* height */
	LEN_ARRAY(4, depth); /* depth */
	LEN(4); /* blockw */
	LEN(4); /* blockh */
	LEN(4); /* blocksize */
	LEN(4); /* last_level */
	LEN(4); /* nr_samples */
	LEN(4); /* tex_usage */

	/* align */
	PAD(__len, 8);

	__data = (uint8_t*)MALLOC(__len);
	if (!__data)
		return -ENOMEM;

	WRITE(4, int32_t, ((int32_t)RBUG_OP_TEXTURE_INFO_REPLY));
	WRITE(4, uint32_t, ((uint32_t)(__len / 4)));
	WRITE(4, uint32_t, serial); /* serial */
	WRITE(4, uint32_t, target); /* target */
	WRITE(4, uint32_t, format); /* format */
	WRITE_ARRAY(4, uint32_t, width); /* width */
	WRITE_ARRAY(4, uint32_t, height); /* height */
	WRITE_ARRAY(4, uint32_t, depth); /* depth */
	WRITE(4, uint32_t, blockw); /* blockw */
	WRITE(4, uint32_t, blockh); /* blockh */
	WRITE(4, uint32_t, blocksize); /* blocksize */
	WRITE(4, uint32_t, last_level); /* last_level */
	WRITE(4, uint32_t, nr_samples); /* nr_samples */
	WRITE(4, uint32_t, tex_usage); /* tex_usage */

	/* final pad */
	PAD(__pos, 8);

	if (__pos != __len) {
		__ret = -EINVAL;
	} else {
		rbug_connection_send_start(__con, RBUG_OP_TEXTURE_INFO_REPLY, __len);
		rbug_connection_write(__con, __data, __len);
		__ret = rbug_connection_send_finish(__con, __serial);
	}

	FREE(__data);
	return __ret;
}

int rbug_send_texture_read_reply(struct rbug_connection *__con,
                                 uint32_t serial,
                                 uint32_t format,
                                 uint32_t blockw,
                                 uint32_t blockh,
                                 uint32_t blocksize,
                                 uint8_t *data,
                                 uint32_t data_len,
                                 uint32_t stride,
                                 uint32_t *__serial)
{
	uint32_t __len = 0;
	uint32_t __pos = 0;
	uint8_t *__data = NULL;
	int __ret = 0;

	LEN(8); /* header */
	LEN(4); /* serial */
	LEN(4); /* format */
	LEN(4); /* blockw */
	LEN(4); /* blockh */
	LEN(4); /* blocksize */
	LEN_ARRAY(1, data); /* data */
	LEN(4); /* stride */

	/* align */
	PAD(__len, 8);

	__data = (uint8_t*)MALLOC(__len);
	if (!__data)
		return -ENOMEM;

	WRITE(4, int32_t, ((int32_t)RBUG_OP_TEXTURE_READ_REPLY));
	WRITE(4, uint32_t, ((uint32_t)(__len / 4)));
	WRITE(4, uint32_t, serial); /* serial */
	WRITE(4, uint32_t, format); /* format */
	WRITE(4, uint32_t, blockw); /* blockw */
	WRITE(4, uint32_t, blockh); /* blockh */
	WRITE(4, uint32_t, blocksize); /* blocksize */
	WRITE_ARRAY(1, uint8_t, data); /* data */
	WRITE(4, uint32_t, stride); /* stride */

	/* final pad */
	PAD(__pos, 8);

	if (__pos != __len) {
		__ret = -EINVAL;
	} else {
		rbug_connection_send_start(__con, RBUG_OP_TEXTURE_READ_REPLY, __len);
		rbug_connection_write(__con, __data, __len);
		__ret = rbug_connection_send_finish(__con, __serial);
	}

	FREE(__data);
	return __ret;
}

struct rbug_proto_texture_list * rbug_demarshal_texture_list(struct rbug_proto_header *header)
{
	struct rbug_proto_texture_list *ret;

	if (!header)
		return NULL;
	if (header->opcode != (int32_t)RBUG_OP_TEXTURE_LIST)
		return NULL;

	ret = MALLOC(sizeof(*ret));
	if (!ret)
		return NULL;

	ret->header.__message = header;
	ret->header.opcode = header->opcode;

	return ret;
}

struct rbug_proto_texture_info * rbug_demarshal_texture_info(struct rbug_proto_header *header)
{
	uint32_t len = 0;
	uint32_t pos = 0;
	uint8_t *data =  NULL;
	struct rbug_proto_texture_info *ret;

	if (!header)
		return NULL;
	if (header->opcode != (int32_t)RBUG_OP_TEXTURE_INFO)
		return NULL;

	pos = 0;
	len = header->length * 4;
	data = (uint8_t*)&header[1];
	ret = MALLOC(sizeof(*ret));
	if (!ret)
		return NULL;

	ret->header.__message = header;
	ret->header.opcode = header->opcode;

	READ(8, rbug_texture_t, texture); /* texture */

	return ret;
}

struct rbug_proto_texture_write * rbug_demarshal_texture_write(struct rbug_proto_header *header)
{
	uint32_t len = 0;
	uint32_t pos = 0;
	uint8_t *data =  NULL;
	struct rbug_proto_texture_write *ret;

	if (!header)
		return NULL;
	if (header->opcode != (int32_t)RBUG_OP_TEXTURE_WRITE)
		return NULL;

	pos = 0;
	len = header->length * 4;
	data = (uint8_t*)&header[1];
	ret = MALLOC(sizeof(*ret));
	if (!ret)
		return NULL;

	ret->header.__message = header;
	ret->header.opcode = header->opcode;

	READ(8, rbug_texture_t, texture); /* texture */
	READ(4, uint32_t, face); /* face */
	READ(4, uint32_t, level); /* level */
	READ(4, uint32_t, zslice); /* zslice */
	READ(4, uint32_t, x); /* x */
	READ(4, uint32_t, y); /* y */
	READ(4, uint32_t, w); /* w */
	READ(4, uint32_t, h); /* h */
	READ_ARRAY(1, uint8_t, data); /* data */
	READ(4, uint32_t, stride); /* stride */

	return ret;
}

struct rbug_proto_texture_read * rbug_demarshal_texture_read(struct rbug_proto_header *header)
{
	uint32_t len = 0;
	uint32_t pos = 0;
	uint8_t *data =  NULL;
	struct rbug_proto_texture_read *ret;

	if (!header)
		return NULL;
	if (header->opcode != (int32_t)RBUG_OP_TEXTURE_READ)
		return NULL;

	pos = 0;
	len = header->length * 4;
	data = (uint8_t*)&header[1];
	ret = MALLOC(sizeof(*ret));
	if (!ret)
		return NULL;

	ret->header.__message = header;
	ret->header.opcode = header->opcode;

	READ(8, rbug_texture_t, texture); /* texture */
	READ(4, uint32_t, face); /* face */
	READ(4, uint32_t, level); /* level */
	READ(4, uint32_t, zslice); /* zslice */
	READ(4, uint32_t, x); /* x */
	READ(4, uint32_t, y); /* y */
	READ(4, uint32_t, w); /* w */
	READ(4, uint32_t, h); /* h */

	return ret;
}

struct rbug_proto_texture_list_reply * rbug_demarshal_texture_list_reply(struct rbug_proto_header *header)
{
	uint32_t len = 0;
	uint32_t pos = 0;
	uint8_t *data =  NULL;
	struct rbug_proto_texture_list_reply *ret;

	if (!header)
		return NULL;
	if (header->opcode != (int32_t)RBUG_OP_TEXTURE_LIST_REPLY)
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
	READ_ARRAY(8, rbug_texture_t, textures); /* textures */

	return ret;
}

struct rbug_proto_texture_info_reply * rbug_demarshal_texture_info_reply(struct rbug_proto_header *header)
{
	uint32_t len = 0;
	uint32_t pos = 0;
	uint8_t *data =  NULL;
	struct rbug_proto_texture_info_reply *ret;

	if (!header)
		return NULL;
	if (header->opcode != (int32_t)RBUG_OP_TEXTURE_INFO_REPLY)
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
	READ(4, uint32_t, target); /* target */
	READ(4, uint32_t, format); /* format */
	READ_ARRAY(4, uint32_t, width); /* width */
	READ_ARRAY(4, uint32_t, height); /* height */
	READ_ARRAY(4, uint32_t, depth); /* depth */
	READ(4, uint32_t, blockw); /* blockw */
	READ(4, uint32_t, blockh); /* blockh */
	READ(4, uint32_t, blocksize); /* blocksize */
	READ(4, uint32_t, last_level); /* last_level */
	READ(4, uint32_t, nr_samples); /* nr_samples */
	READ(4, uint32_t, tex_usage); /* tex_usage */

	return ret;
}

struct rbug_proto_texture_read_reply * rbug_demarshal_texture_read_reply(struct rbug_proto_header *header)
{
	uint32_t len = 0;
	uint32_t pos = 0;
	uint8_t *data =  NULL;
	struct rbug_proto_texture_read_reply *ret;

	if (!header)
		return NULL;
	if (header->opcode != (int32_t)RBUG_OP_TEXTURE_READ_REPLY)
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
	READ(4, uint32_t, format); /* format */
	READ(4, uint32_t, blockw); /* blockw */
	READ(4, uint32_t, blockh); /* blockh */
	READ(4, uint32_t, blocksize); /* blocksize */
	READ_ARRAY(1, uint8_t, data); /* data */
	READ(4, uint32_t, stride); /* stride */

	return ret;
}
