/*
 * Copyright (C) 2007 Ben Skeggs.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/* GL_ARB_occlusion_query support for NV20/30/40 */

#include "mtypes.h"

#include "nouveau_fifo.h"
#include "nouveau_msg.h"
#include "nouveau_object.h"
#include "nouveau_reg.h"
#include "nouveau_sync.h"
#include "nouveau_query.h"

static struct gl_query_object *
nouveauNewQueryObject(GLcontext *ctx, GLuint id)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	nouveau_query_object *nq;
	int i;

	for (i=0; i<nmesa->query_object_max; i++)
		if (nmesa->query_alloc[i] == GL_FALSE)
			break;
	if (i==nmesa->query_object_max)
		return NULL;

	nq = CALLOC_STRUCT(nouveau_query_object_t);
	if (nq) {
		nq->notifier_id	= i;

		nq->mesa.Id     = id;
		nq->mesa.Result = 0;
		nq->mesa.Active = GL_FALSE;
		nq->mesa.Ready  = GL_TRUE;
	}

	return (struct gl_query_object *)nq;
}

static void
nouveauBeginQuery(GLcontext *ctx, GLenum target, struct gl_query_object *q)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	nouveau_query_object *nq = (nouveau_query_object *)q;

	nouveau_notifier_reset(nmesa->queryNotifier, nq->notifier_id);

	switch (nmesa->screen->card->type) {
	case NV_20:
		BEGIN_RING_CACHE(NvSub3D, 0x17c8, 1);
		OUT_RING_CACHE  (1);
		BEGIN_RING_CACHE(NvSub3D, 0x17cc, 1);
		OUT_RING_CACHE  (1);
		break;
	case NV_30:
	case NV_40:
	case NV_44:
		/* I don't think this is OCC_QUERY enable, but it *is* needed to make
		 * the SET_OBJECT7 notifier block work with STORE_RESULT.
		 *
		 * Also, this appears to reset the pixel pass counter */
		BEGIN_RING_SIZE(NvSub3D,
				 NV30_TCL_PRIMITIVE_3D_OCC_QUERY_OR_COLOR_BUFF_ENABLE,
				 1);
		OUT_RING        (1);
		/* Probably OCC_QUERY_ENABLE */
		BEGIN_RING_CACHE(NvSub3D, 0x17cc, 1);
		OUT_RING_CACHE  (1);
		break;
	default:
		WARN_ONCE("no support for this card\n");
		break;
	}
}

static void
nouveauUpdateQuery(GLcontext *ctx, GLenum target, struct gl_query_object *q)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	nouveau_query_object *nq = (nouveau_query_object *)q;
	int status;

	status = nouveau_notifier_status(nmesa->queryNotifier,
					 nq->notifier_id);

	q->Ready = (status == NV_NOTIFY_STATE_STATUS_COMPLETED);
	if (q->Ready)
		q->Result = nouveau_notifier_return_val(nmesa->queryNotifier,
							nq->notifier_id);
}

static void
nouveauWaitQueryResult(GLcontext *ctx, GLenum target, struct gl_query_object *q)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	nouveau_query_object *nq = (nouveau_query_object *)q;

	nouveau_notifier_wait_status(nmesa->queryNotifier, nq->notifier_id,
				     NV_NOTIFY_STATE_STATUS_COMPLETED, 0);
	nouveauUpdateQuery(ctx, target, q);
}

static void
nouveauEndQuery(GLcontext *ctx, GLenum target, struct gl_query_object *q)
{
	nouveau_query_object *nq = (nouveau_query_object *)q;
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	switch (nmesa->screen->card->type) {
	case NV_20:
		BEGIN_RING_SIZE(NvSub3D, 0x17d0, 1);
		OUT_RING       (0x01000000 | nq->notifier_id*32);
		break;
	case NV_30:
	case NV_40:
	case NV_44:
		BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_STORE_RESULT, 1);
		OUT_RING       (0x01000000 | nq->notifier_id*32);
		break;
	default:
		WARN_ONCE("no support for this card\n");
		break;
	}
	FIRE_RING();

	/*XXX: wait for query to complete, mesa doesn't give the driver
	 *     an interface to query the status of a query object so 
	 *     this has to stall the channel.
	 */
	nouveauWaitQueryResult(ctx, target, q);

	BEGIN_RING_CACHE(NvSub3D, 0x17cc, 1);
	OUT_RING_CACHE  (0);
}

void
nouveauQueryInitFuncs(GLcontext *ctx)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	if (nmesa->screen->card->type < NV_20)
		return;

	nmesa->query_object_max = (0x4000 / 32);
	nmesa->queryNotifier =
		nouveau_notifier_new(ctx, NvQueryNotify,
					  nmesa->query_object_max);
	nmesa->query_alloc = calloc(nmesa->query_object_max, sizeof(GLboolean));

	switch (nmesa->screen->card->type) {
	case NV_20:
		BEGIN_RING_CACHE(NvSub3D, NV20_TCL_PRIMITIVE_3D_SET_OBJECT8, 1);
		OUT_RING_CACHE  (NvQueryNotify);
		break;
	case NV_30:
	case NV_40:
	case NV_44:
		BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_SET_OBJECT7, 1);
		OUT_RING_CACHE  (NvQueryNotify);
		break;
	default:
		break;
	};

	ctx->Driver.NewQueryObject	= nouveauNewQueryObject;
	ctx->Driver.BeginQuery		= nouveauBeginQuery;
	ctx->Driver.EndQuery		= nouveauEndQuery;
#if 0
	ctx->Driver.UpdateQuery		= nouveauUpdateQuery;
	ctx->Driver.WaitQueryResult	= nouveauWaitQueryResult;
#endif
}

