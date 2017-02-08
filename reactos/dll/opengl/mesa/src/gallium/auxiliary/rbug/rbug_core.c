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
#include "rbug/rbug_core.h"

int rbug_send_noop(struct rbug_connection *__con,
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

	WRITE(4, int32_t, ((int32_t)RBUG_OP_NOOP));
	WRITE(4, uint32_t, ((uint32_t)(__len / 4)));

	/* final pad */
	PAD(__pos, 8);

	if (__pos != __len) {
		__ret = -EINVAL;
	} else {
		rbug_connection_send_start(__con, RBUG_OP_NOOP, __len);
		rbug_connection_write(__con, __data, __len);
		__ret = rbug_connection_send_finish(__con, __serial);
	}

	FREE(__data);
	return __ret;
}

int rbug_send_ping(struct rbug_connection *__con,
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

	WRITE(4, int32_t, ((int32_t)RBUG_OP_PING));
	WRITE(4, uint32_t, ((uint32_t)(__len / 4)));

	/* final pad */
	PAD(__pos, 8);

	if (__pos != __len) {
		__ret = -EINVAL;
	} else {
		rbug_connection_send_start(__con, RBUG_OP_PING, __len);
		rbug_connection_write(__con, __data, __len);
		__ret = rbug_connection_send_finish(__con, __serial);
	}

	FREE(__data);
	return __ret;
}

int rbug_send_error(struct rbug_connection *__con,
                    uint32_t error,
                    uint32_t *__serial)
{
	uint32_t __len = 0;
	uint32_t __pos = 0;
	uint8_t *__data = NULL;
	int __ret = 0;

	LEN(8); /* header */
	LEN(4); /* error */

	/* align */
	PAD(__len, 8);

	__data = (uint8_t*)MALLOC(__len);
	if (!__data)
		return -ENOMEM;

	WRITE(4, int32_t, ((int32_t)RBUG_OP_ERROR));
	WRITE(4, uint32_t, ((uint32_t)(__len / 4)));
	WRITE(4, uint32_t, error); /* error */

	/* final pad */
	PAD(__pos, 8);

	if (__pos != __len) {
		__ret = -EINVAL;
	} else {
		rbug_connection_send_start(__con, RBUG_OP_ERROR, __len);
		rbug_connection_write(__con, __data, __len);
		__ret = rbug_connection_send_finish(__con, __serial);
	}

	FREE(__data);
	return __ret;
}

int rbug_send_ping_reply(struct rbug_connection *__con,
                         uint32_t serial,
                         uint32_t *__serial)
{
	uint32_t __len = 0;
	uint32_t __pos = 0;
	uint8_t *__data = NULL;
	int __ret = 0;

	LEN(8); /* header */
	LEN(4); /* serial */

	/* align */
	PAD(__len, 8);

	__data = (uint8_t*)MALLOC(__len);
	if (!__data)
		return -ENOMEM;

	WRITE(4, int32_t, ((int32_t)RBUG_OP_PING_REPLY));
	WRITE(4, uint32_t, ((uint32_t)(__len / 4)));
	WRITE(4, uint32_t, serial); /* serial */

	/* final pad */
	PAD(__pos, 8);

	if (__pos != __len) {
		__ret = -EINVAL;
	} else {
		rbug_connection_send_start(__con, RBUG_OP_PING_REPLY, __len);
		rbug_connection_write(__con, __data, __len);
		__ret = rbug_connection_send_finish(__con, __serial);
	}

	FREE(__data);
	return __ret;
}

int rbug_send_error_reply(struct rbug_connection *__con,
                          uint32_t serial,
                          uint32_t error,
                          uint32_t *__serial)
{
	uint32_t __len = 0;
	uint32_t __pos = 0;
	uint8_t *__data = NULL;
	int __ret = 0;

	LEN(8); /* header */
	LEN(4); /* serial */
	LEN(4); /* error */

	/* align */
	PAD(__len, 8);

	__data = (uint8_t*)MALLOC(__len);
	if (!__data)
		return -ENOMEM;

	WRITE(4, int32_t, ((int32_t)RBUG_OP_ERROR_REPLY));
	WRITE(4, uint32_t, ((uint32_t)(__len / 4)));
	WRITE(4, uint32_t, serial); /* serial */
	WRITE(4, uint32_t, error); /* error */

	/* final pad */
	PAD(__pos, 8);

	if (__pos != __len) {
		__ret = -EINVAL;
	} else {
		rbug_connection_send_start(__con, RBUG_OP_ERROR_REPLY, __len);
		rbug_connection_write(__con, __data, __len);
		__ret = rbug_connection_send_finish(__con, __serial);
	}

	FREE(__data);
	return __ret;
}

struct rbug_proto_noop * rbug_demarshal_noop(struct rbug_proto_header *header)
{
	struct rbug_proto_noop *ret;

	if (!header)
		return NULL;
	if (header->opcode != (int32_t)RBUG_OP_NOOP)
		return NULL;

	ret = MALLOC(sizeof(*ret));
	if (!ret)
		return NULL;

	ret->header.__message = header;
	ret->header.opcode = header->opcode;

	return ret;
}

struct rbug_proto_ping * rbug_demarshal_ping(struct rbug_proto_header *header)
{
	struct rbug_proto_ping *ret;

	if (!header)
		return NULL;
	if (header->opcode != (int32_t)RBUG_OP_PING)
		return NULL;

	ret = MALLOC(sizeof(*ret));
	if (!ret)
		return NULL;

	ret->header.__message = header;
	ret->header.opcode = header->opcode;

	return ret;
}

struct rbug_proto_error * rbug_demarshal_error(struct rbug_proto_header *header)
{
	uint32_t len = 0;
	uint32_t pos = 0;
	uint8_t *data =  NULL;
	struct rbug_proto_error *ret;

	if (!header)
		return NULL;
	if (header->opcode != (int32_t)RBUG_OP_ERROR)
		return NULL;

	pos = 0;
	len = header->length * 4;
	data = (uint8_t*)&header[1];
	ret = MALLOC(sizeof(*ret));
	if (!ret)
		return NULL;

	ret->header.__message = header;
	ret->header.opcode = header->opcode;

	READ(4, uint32_t, error); /* error */

	return ret;
}

struct rbug_proto_ping_reply * rbug_demarshal_ping_reply(struct rbug_proto_header *header)
{
	uint32_t len = 0;
	uint32_t pos = 0;
	uint8_t *data =  NULL;
	struct rbug_proto_ping_reply *ret;

	if (!header)
		return NULL;
	if (header->opcode != (int32_t)RBUG_OP_PING_REPLY)
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

	return ret;
}

struct rbug_proto_error_reply * rbug_demarshal_error_reply(struct rbug_proto_header *header)
{
	uint32_t len = 0;
	uint32_t pos = 0;
	uint8_t *data =  NULL;
	struct rbug_proto_error_reply *ret;

	if (!header)
		return NULL;
	if (header->opcode != (int32_t)RBUG_OP_ERROR_REPLY)
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
	READ(4, uint32_t, error); /* error */

	return ret;
}
