/*
 * Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2001  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* $Id: taskpool.c,v 1.18 2007/06/18 23:47:44 tbox Exp $ */

/*! \file */

#include <config.h>

#include <isc/mem.h>
#include <isc/taskpool.h>
#include <isc/util.h>

/***
 *** Types.
 ***/

struct isc_taskpool {
	isc_mem_t *			mctx;
	unsigned int			ntasks;
	isc_task_t **			tasks;
};
/***
 *** Functions.
 ***/

isc_result_t
isc_taskpool_create(isc_taskmgr_t *tmgr, isc_mem_t *mctx,
		    unsigned int ntasks, unsigned int quantum,
		    isc_taskpool_t **poolp)
{
	unsigned int i;
	isc_taskpool_t *pool;
	isc_result_t result;

	INSIST(ntasks > 0);
	pool = isc_mem_get(mctx, sizeof(*pool));
	if (pool == NULL)
		return (ISC_R_NOMEMORY);
	pool->mctx = mctx;
	pool->ntasks = ntasks;
	pool->tasks = isc_mem_get(mctx, ntasks * sizeof(isc_task_t *));
	if (pool->tasks == NULL) {
		isc_mem_put(mctx, pool, sizeof(*pool));
		return (ISC_R_NOMEMORY);
	}
	for (i = 0; i < ntasks; i++)
		pool->tasks[i] = NULL;
	for (i = 0; i < ntasks; i++) {
		result = isc_task_create(tmgr, quantum, &pool->tasks[i]);
		if (result != ISC_R_SUCCESS) {
			isc_taskpool_destroy(&pool);
			return (result);
		}
		isc_task_setname(pool->tasks[i], "taskpool", NULL);
	}
	*poolp = pool;
	return (ISC_R_SUCCESS);
}

void isc_taskpool_gettask(isc_taskpool_t *pool, unsigned int hash,
			  isc_task_t **targetp)
{
	isc_task_attach(pool->tasks[hash % pool->ntasks], targetp);
}

void
isc_taskpool_destroy(isc_taskpool_t **poolp) {
	unsigned int i;
	isc_taskpool_t *pool = *poolp;
	for (i = 0; i < pool->ntasks; i++) {
		if (pool->tasks[i] != NULL) {
			isc_task_detach(&pool->tasks[i]);
		}
	}
	isc_mem_put(pool->mctx, pool->tasks,
		    pool->ntasks * sizeof(isc_task_t *));
	isc_mem_put(pool->mctx, pool, sizeof(*pool));
	*poolp = NULL;
}


