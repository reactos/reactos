/*
 * Copyright (C) 2004, 2005, 2007-2009  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1998-2002  Internet Software Consortium.
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

/* $Id: timer.c,v 1.84.58.4 2009/01/23 23:47:21 tbox Exp $ */

/*! \file */

#include <config.h>

#include <isc/condition.h>
#include <isc/heap.h>
#include <isc/log.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/msgs.h>
#include <isc/platform.h>
#include <isc/task.h>
#include <isc/thread.h>
#include <isc/time.h>
#include <isc/timer.h>
#include <isc/util.h>

#ifndef ISC_PLATFORM_USETHREADS
#include "timer_p.h"
#endif /* ISC_PLATFORM_USETHREADS */

#ifdef ISC_TIMER_TRACE
#define XTRACE(s)			fprintf(stderr, "%s\n", (s))
#define XTRACEID(s, t)			fprintf(stderr, "%s %p\n", (s), (t))
#define XTRACETIME(s, d)		fprintf(stderr, "%s %u.%09u\n", (s), \
					       (d).seconds, (d).nanoseconds)
#define XTRACETIME2(s, d, n)		fprintf(stderr, "%s %u.%09u %u.%09u\n", (s), \
					       (d).seconds, (d).nanoseconds, (n).seconds, (n).nanoseconds)
#define XTRACETIMER(s, t, d)		fprintf(stderr, "%s %p %u.%09u\n", (s), (t), \
					       (d).seconds, (d).nanoseconds)
#else
#define XTRACE(s)
#define XTRACEID(s, t)
#define XTRACETIME(s, d)
#define XTRACETIME2(s, d, n)
#define XTRACETIMER(s, t, d)
#endif /* ISC_TIMER_TRACE */

#define TIMER_MAGIC			ISC_MAGIC('T', 'I', 'M', 'R')
#define VALID_TIMER(t)			ISC_MAGIC_VALID(t, TIMER_MAGIC)

struct isc_timer {
	/*! Not locked. */
	unsigned int			magic;
	isc_timermgr_t *		manager;
	isc_mutex_t			lock;
	/*! Locked by timer lock. */
	unsigned int			references;
	isc_time_t			idle;
	/*! Locked by manager lock. */
	isc_timertype_t			type;
	isc_time_t			expires;
	isc_interval_t			interval;
	isc_task_t *			task;
	isc_taskaction_t		action;
	void *				arg;
	unsigned int			index;
	isc_time_t			due;
	LINK(isc_timer_t)		link;
};

#define TIMER_MANAGER_MAGIC		ISC_MAGIC('T', 'I', 'M', 'M')
#define VALID_MANAGER(m)		ISC_MAGIC_VALID(m, TIMER_MANAGER_MAGIC)

struct isc_timermgr {
	/* Not locked. */
	unsigned int			magic;
	isc_mem_t *			mctx;
	isc_mutex_t			lock;
	/* Locked by manager lock. */
	isc_boolean_t			done;
	LIST(isc_timer_t)		timers;
	unsigned int			nscheduled;
	isc_time_t			due;
#ifdef ISC_PLATFORM_USETHREADS
	isc_condition_t			wakeup;
	isc_thread_t			thread;
#else /* ISC_PLATFORM_USETHREADS */
	unsigned int			refs;
#endif /* ISC_PLATFORM_USETHREADS */
	isc_heap_t *			heap;
};

#ifndef ISC_PLATFORM_USETHREADS
/*!
 * If threads are not in use, there can be only one.
 */
static isc_timermgr_t *timermgr = NULL;
#endif /* ISC_PLATFORM_USETHREADS */

static inline isc_result_t
schedule(isc_timer_t *timer, isc_time_t *now, isc_boolean_t signal_ok) {
	isc_result_t result;
	isc_timermgr_t *manager;
	isc_time_t due;
	int cmp;
#ifdef ISC_PLATFORM_USETHREADS
	isc_boolean_t timedwait;
#endif

	/*!
	 * Note: the caller must ensure locking.
	 */

	REQUIRE(timer->type != isc_timertype_inactive);

#ifndef ISC_PLATFORM_USETHREADS
	UNUSED(signal_ok);
#endif /* ISC_PLATFORM_USETHREADS */

	manager = timer->manager;

#ifdef ISC_PLATFORM_USETHREADS
	/*!
	 * If the manager was timed wait, we may need to signal the
	 * manager to force a wakeup.
	 */
	timedwait = ISC_TF(manager->nscheduled > 0 &&
			   isc_time_seconds(&manager->due) != 0);
#endif

	/*
	 * Compute the new due time.
	 */
	if (timer->type != isc_timertype_once) {
		result = isc_time_add(now, &timer->interval, &due);
		if (result != ISC_R_SUCCESS)
			return (result);
		if (timer->type == isc_timertype_limited &&
		    isc_time_compare(&timer->expires, &due) < 0)
			due = timer->expires;
	} else {
		if (isc_time_isepoch(&timer->idle))
			due = timer->expires;
		else if (isc_time_isepoch(&timer->expires))
			due = timer->idle;
		else if (isc_time_compare(&timer->idle, &timer->expires) < 0)
			due = timer->idle;
		else
			due = timer->expires;
	}

	/*
	 * Schedule the timer.
	 */

	if (timer->index > 0) {
		/*
		 * Already scheduled.
		 */
		cmp = isc_time_compare(&due, &timer->due);
		timer->due = due;
		switch (cmp) {
		case -1:
			isc_heap_increased(manager->heap, timer->index);
			break;
		case 1:
			isc_heap_decreased(manager->heap, timer->index);
			break;
		case 0:
			/* Nothing to do. */
			break;
		}
	} else {
		timer->due = due;
		result = isc_heap_insert(manager->heap, timer);
		if (result != ISC_R_SUCCESS) {
			INSIST(result == ISC_R_NOMEMORY);
			return (ISC_R_NOMEMORY);
		}
		manager->nscheduled++;
	}

	XTRACETIMER(isc_msgcat_get(isc_msgcat, ISC_MSGSET_TIMER,
				   ISC_MSG_SCHEDULE, "schedule"), timer, due);

	/*
	 * If this timer is at the head of the queue, we need to ensure
	 * that we won't miss it if it has a more recent due time than
	 * the current "next" timer.  We do this either by waking up the
	 * run thread, or explicitly setting the value in the manager.
	 */
#ifdef ISC_PLATFORM_USETHREADS

	/*
	 * This is a temporary (probably) hack to fix a bug on tru64 5.1
	 * and 5.1a.  Sometimes, pthread_cond_timedwait() doesn't actually
	 * return when the time expires, so here, we check to see if
	 * we're 15 seconds or more behind, and if we are, we signal
	 * the dispatcher.  This isn't such a bad idea as a general purpose
	 * watchdog, so perhaps we should just leave it in here.
	 */
	if (signal_ok && timedwait) {
		isc_interval_t fifteen;
		isc_time_t then;

		isc_interval_set(&fifteen, 15, 0);
		result = isc_time_add(&manager->due, &fifteen, &then);

		if (result == ISC_R_SUCCESS &&
		    isc_time_compare(&then, now) < 0) {
			SIGNAL(&manager->wakeup);
			signal_ok = ISC_FALSE;
			isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
				      ISC_LOGMODULE_TIMER, ISC_LOG_WARNING,
				      "*** POKED TIMER ***");
		}
	}

	if (timer->index == 1 && signal_ok) {
		XTRACE(isc_msgcat_get(isc_msgcat, ISC_MSGSET_TIMER,
				      ISC_MSG_SIGNALSCHED,
				      "signal (schedule)"));
		SIGNAL(&manager->wakeup);
	}
#else /* ISC_PLATFORM_USETHREADS */
	if (timer->index == 1 &&
	    isc_time_compare(&timer->due, &manager->due) < 0)
		manager->due = timer->due;
#endif /* ISC_PLATFORM_USETHREADS */

	return (ISC_R_SUCCESS);
}

static inline void
deschedule(isc_timer_t *timer) {
	isc_boolean_t need_wakeup = ISC_FALSE;
	isc_timermgr_t *manager;

	/*
	 * The caller must ensure locking.
	 */

	manager = timer->manager;
	if (timer->index > 0) {
		if (timer->index == 1)
			need_wakeup = ISC_TRUE;
		isc_heap_delete(manager->heap, timer->index);
		timer->index = 0;
		INSIST(manager->nscheduled > 0);
		manager->nscheduled--;
#ifdef ISC_PLATFORM_USETHREADS
		if (need_wakeup) {
			XTRACE(isc_msgcat_get(isc_msgcat, ISC_MSGSET_TIMER,
					      ISC_MSG_SIGNALDESCHED,
					      "signal (deschedule)"));
			SIGNAL(&manager->wakeup);
		}
#endif /* ISC_PLATFORM_USETHREADS */
	}
}

static void
destroy(isc_timer_t *timer) {
	isc_timermgr_t *manager = timer->manager;

	/*
	 * The caller must ensure it is safe to destroy the timer.
	 */

	LOCK(&manager->lock);

	(void)isc_task_purgerange(timer->task,
				  timer,
				  ISC_TIMEREVENT_FIRSTEVENT,
				  ISC_TIMEREVENT_LASTEVENT,
				  NULL);
	deschedule(timer);
	UNLINK(manager->timers, timer, link);

	UNLOCK(&manager->lock);

	isc_task_detach(&timer->task);
	DESTROYLOCK(&timer->lock);
	timer->magic = 0;
	isc_mem_put(manager->mctx, timer, sizeof(*timer));
}

isc_result_t
isc_timer_create(isc_timermgr_t *manager, isc_timertype_t type,
		 isc_time_t *expires, isc_interval_t *interval,
		 isc_task_t *task, isc_taskaction_t action, const void *arg,
		 isc_timer_t **timerp)
{
	isc_timer_t *timer;
	isc_result_t result;
	isc_time_t now;

	/*
	 * Create a new 'type' timer managed by 'manager'.  The timers
	 * parameters are specified by 'expires' and 'interval'.  Events
	 * will be posted to 'task' and when dispatched 'action' will be
	 * called with 'arg' as the arg value.  The new timer is returned
	 * in 'timerp'.
	 */

	REQUIRE(VALID_MANAGER(manager));
	REQUIRE(task != NULL);
	REQUIRE(action != NULL);
	if (expires == NULL)
		expires = isc_time_epoch;
	if (interval == NULL)
		interval = isc_interval_zero;
	REQUIRE(type == isc_timertype_inactive ||
		!(isc_time_isepoch(expires) && isc_interval_iszero(interval)));
	REQUIRE(timerp != NULL && *timerp == NULL);
	REQUIRE(type != isc_timertype_limited ||
		!(isc_time_isepoch(expires) || isc_interval_iszero(interval)));

	/*
	 * Get current time.
	 */
	if (type != isc_timertype_inactive) {
		TIME_NOW(&now);
	} else {
		/*
		 * We don't have to do this, but it keeps the compiler from
		 * complaining about "now" possibly being used without being
		 * set, even though it will never actually happen.
		 */
		isc_time_settoepoch(&now);
	}


	timer = isc_mem_get(manager->mctx, sizeof(*timer));
	if (timer == NULL)
		return (ISC_R_NOMEMORY);

	timer->manager = manager;
	timer->references = 1;

	if (type == isc_timertype_once && !isc_interval_iszero(interval)) {
		result = isc_time_add(&now, interval, &timer->idle);
		if (result != ISC_R_SUCCESS) {
			isc_mem_put(manager->mctx, timer, sizeof(*timer));
			return (result);
		}
	} else
		isc_time_settoepoch(&timer->idle);

	timer->type = type;
	timer->expires = *expires;
	timer->interval = *interval;
	timer->task = NULL;
	isc_task_attach(task, &timer->task);
	timer->action = action;
	/*
	 * Removing the const attribute from "arg" is the best of two
	 * evils here.  If the timer->arg member is made const, then
	 * it affects a great many recipients of the timer event
	 * which did not pass in an "arg" that was truly const.
	 * Changing isc_timer_create() to not have "arg" prototyped as const,
	 * though, can cause compilers warnings for calls that *do*
	 * have a truly const arg.  The caller will have to carefully
	 * keep track of whether arg started as a true const.
	 */
	DE_CONST(arg, timer->arg);
	timer->index = 0;
	result = isc_mutex_init(&timer->lock);
	if (result != ISC_R_SUCCESS) {
		isc_task_detach(&timer->task);
		isc_mem_put(manager->mctx, timer, sizeof(*timer));
		return (result);
	}
	ISC_LINK_INIT(timer, link);
	timer->magic = TIMER_MAGIC;

	LOCK(&manager->lock);

	/*
	 * Note we don't have to lock the timer like we normally would because
	 * there are no external references to it yet.
	 */

	if (type != isc_timertype_inactive)
		result = schedule(timer, &now, ISC_TRUE);
	else
		result = ISC_R_SUCCESS;
	if (result == ISC_R_SUCCESS)
		APPEND(manager->timers, timer, link);

	UNLOCK(&manager->lock);

	if (result != ISC_R_SUCCESS) {
		timer->magic = 0;
		DESTROYLOCK(&timer->lock);
		isc_task_detach(&timer->task);
		isc_mem_put(manager->mctx, timer, sizeof(*timer));
		return (result);
	}

	*timerp = timer;

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_timer_reset(isc_timer_t *timer, isc_timertype_t type,
		isc_time_t *expires, isc_interval_t *interval,
		isc_boolean_t purge)
{
	isc_time_t now;
	isc_timermgr_t *manager;
	isc_result_t result;

	/*
	 * Change the timer's type, expires, and interval values to the given
	 * values.  If 'purge' is ISC_TRUE, any pending events from this timer
	 * are purged from its task's event queue.
	 */

	REQUIRE(VALID_TIMER(timer));
	manager = timer->manager;
	REQUIRE(VALID_MANAGER(manager));
	if (expires == NULL)
		expires = isc_time_epoch;
	if (interval == NULL)
		interval = isc_interval_zero;
	REQUIRE(type == isc_timertype_inactive ||
		!(isc_time_isepoch(expires) && isc_interval_iszero(interval)));
	REQUIRE(type != isc_timertype_limited ||
		!(isc_time_isepoch(expires) || isc_interval_iszero(interval)));

	/*
	 * Get current time.
	 */
	if (type != isc_timertype_inactive) {
		TIME_NOW(&now);
	} else {
		/*
		 * We don't have to do this, but it keeps the compiler from
		 * complaining about "now" possibly being used without being
		 * set, even though it will never actually happen.
		 */
		isc_time_settoepoch(&now);
	}

	manager = timer->manager;

	LOCK(&manager->lock);
	LOCK(&timer->lock);

	if (purge)
		(void)isc_task_purgerange(timer->task,
					  timer,
					  ISC_TIMEREVENT_FIRSTEVENT,
					  ISC_TIMEREVENT_LASTEVENT,
					  NULL);
	timer->type = type;
	timer->expires = *expires;
	timer->interval = *interval;
	if (type == isc_timertype_once && !isc_interval_iszero(interval)) {
		result = isc_time_add(&now, interval, &timer->idle);
	} else {
		isc_time_settoepoch(&timer->idle);
		result = ISC_R_SUCCESS;
	}

	if (result == ISC_R_SUCCESS) {
		if (type == isc_timertype_inactive) {
			deschedule(timer);
			result = ISC_R_SUCCESS;
		} else
			result = schedule(timer, &now, ISC_TRUE);
	}

	UNLOCK(&timer->lock);
	UNLOCK(&manager->lock);

	return (result);
}

isc_timertype_t
isc_timer_gettype(isc_timer_t *timer) {
	isc_timertype_t t;

	REQUIRE(VALID_TIMER(timer));

	LOCK(&timer->lock);
	t = timer->type;
	UNLOCK(&timer->lock);

	return (t);
}

isc_result_t
isc_timer_touch(isc_timer_t *timer) {
	isc_result_t result;
	isc_time_t now;

	/*
	 * Set the last-touched time of 'timer' to the current time.
	 */

	REQUIRE(VALID_TIMER(timer));

	LOCK(&timer->lock);

	/*
	 * We'd like to
	 *
	 *	REQUIRE(timer->type == isc_timertype_once);
	 *
	 * but we cannot without locking the manager lock too, which we
	 * don't want to do.
	 */

	TIME_NOW(&now);
	result = isc_time_add(&now, &timer->interval, &timer->idle);

	UNLOCK(&timer->lock);

	return (result);
}

void
isc_timer_attach(isc_timer_t *timer, isc_timer_t **timerp) {
	/*
	 * Attach *timerp to timer.
	 */

	REQUIRE(VALID_TIMER(timer));
	REQUIRE(timerp != NULL && *timerp == NULL);

	LOCK(&timer->lock);
	timer->references++;
	UNLOCK(&timer->lock);

	*timerp = timer;
}

void
isc_timer_detach(isc_timer_t **timerp) {
	isc_timer_t *timer;
	isc_boolean_t free_timer = ISC_FALSE;

	/*
	 * Detach *timerp from its timer.
	 */

	REQUIRE(timerp != NULL);
	timer = *timerp;
	REQUIRE(VALID_TIMER(timer));

	LOCK(&timer->lock);
	REQUIRE(timer->references > 0);
	timer->references--;
	if (timer->references == 0)
		free_timer = ISC_TRUE;
	UNLOCK(&timer->lock);

	if (free_timer)
		destroy(timer);

	*timerp = NULL;
}

static void
dispatch(isc_timermgr_t *manager, isc_time_t *now) {
	isc_boolean_t done = ISC_FALSE, post_event, need_schedule;
	isc_timerevent_t *event;
	isc_eventtype_t type = 0;
	isc_timer_t *timer;
	isc_result_t result;
	isc_boolean_t idle;

	/*!
	 * The caller must be holding the manager lock.
	 */

	while (manager->nscheduled > 0 && !done) {
		timer = isc_heap_element(manager->heap, 1);
		INSIST(timer->type != isc_timertype_inactive);
		if (isc_time_compare(now, &timer->due) >= 0) {
			if (timer->type == isc_timertype_ticker) {
				type = ISC_TIMEREVENT_TICK;
				post_event = ISC_TRUE;
				need_schedule = ISC_TRUE;
			} else if (timer->type == isc_timertype_limited) {
				int cmp;
				cmp = isc_time_compare(now, &timer->expires);
				if (cmp >= 0) {
					type = ISC_TIMEREVENT_LIFE;
					post_event = ISC_TRUE;
					need_schedule = ISC_FALSE;
				} else {
					type = ISC_TIMEREVENT_TICK;
					post_event = ISC_TRUE;
					need_schedule = ISC_TRUE;
				}
			} else if (!isc_time_isepoch(&timer->expires) &&
				   isc_time_compare(now,
						    &timer->expires) >= 0) {
				type = ISC_TIMEREVENT_LIFE;
				post_event = ISC_TRUE;
				need_schedule = ISC_FALSE;
			} else {
				idle = ISC_FALSE;

				LOCK(&timer->lock);
				if (!isc_time_isepoch(&timer->idle) &&
				    isc_time_compare(now,
						     &timer->idle) >= 0) {
					idle = ISC_TRUE;
				}
				UNLOCK(&timer->lock);
				if (idle) {
					type = ISC_TIMEREVENT_IDLE;
					post_event = ISC_TRUE;
					need_schedule = ISC_FALSE;
				} else {
					/*
					 * Idle timer has been touched;
					 * reschedule.
					 */
					XTRACEID(isc_msgcat_get(isc_msgcat,
								ISC_MSGSET_TIMER,
								ISC_MSG_IDLERESCHED,
								"idle reschedule"),
						 timer);
					post_event = ISC_FALSE;
					need_schedule = ISC_TRUE;
				}
			}

			if (post_event) {
				XTRACEID(isc_msgcat_get(isc_msgcat,
							ISC_MSGSET_TIMER,
							ISC_MSG_POSTING,
							"posting"), timer);
				/*
				 * XXX We could preallocate this event.
				 */
				event = (isc_timerevent_t *)isc_event_allocate(manager->mctx,
							   timer,
							   type,
							   timer->action,
							   timer->arg,
							   sizeof(*event));

				if (event != NULL) {
					event->due = timer->due;
					isc_task_send(timer->task,
						      ISC_EVENT_PTR(&event));
				} else
					UNEXPECTED_ERROR(__FILE__, __LINE__, "%s",
						 isc_msgcat_get(isc_msgcat,
							 ISC_MSGSET_TIMER,
							 ISC_MSG_EVENTNOTALLOC,
							 "couldn't "
							 "allocate event"));
			}

			timer->index = 0;
			isc_heap_delete(manager->heap, 1);
			manager->nscheduled--;

			if (need_schedule) {
				result = schedule(timer, now, ISC_FALSE);
				if (result != ISC_R_SUCCESS)
					UNEXPECTED_ERROR(__FILE__, __LINE__,
							 "%s: %u",
						isc_msgcat_get(isc_msgcat,
							ISC_MSGSET_TIMER,
							ISC_MSG_SCHEDFAIL,
							"couldn't schedule "
							"timer"),
							 result);
			}
		} else {
			manager->due = timer->due;
			done = ISC_TRUE;
		}
	}
}

#ifdef ISC_PLATFORM_USETHREADS
static isc_threadresult_t
#ifdef _WIN32			/* XXXDCL */
WINAPI
#endif
run(void *uap) {
	isc_timermgr_t *manager = uap;
	isc_time_t now;
	isc_result_t result;

	LOCK(&manager->lock);
	while (!manager->done) {
		TIME_NOW(&now);

		XTRACETIME(isc_msgcat_get(isc_msgcat, ISC_MSGSET_GENERAL,
					  ISC_MSG_RUNNING,
					  "running"), now);

		dispatch(manager, &now);

		if (manager->nscheduled > 0) {
			XTRACETIME2(isc_msgcat_get(isc_msgcat,
						   ISC_MSGSET_GENERAL,
						   ISC_MSG_WAITUNTIL,
						   "waituntil"),
				    manager->due, now);
			result = WAITUNTIL(&manager->wakeup, &manager->lock, &manager->due);
			INSIST(result == ISC_R_SUCCESS ||
			       result == ISC_R_TIMEDOUT);
		} else {
			XTRACETIME(isc_msgcat_get(isc_msgcat, ISC_MSGSET_GENERAL,
						  ISC_MSG_WAIT, "wait"), now);
			WAIT(&manager->wakeup, &manager->lock);
		}
		XTRACE(isc_msgcat_get(isc_msgcat, ISC_MSGSET_TIMER,
				      ISC_MSG_WAKEUP, "wakeup"));
	}
	UNLOCK(&manager->lock);

	return ((isc_threadresult_t)0);
}
#endif /* ISC_PLATFORM_USETHREADS */

static isc_boolean_t
sooner(void *v1, void *v2) {
	isc_timer_t *t1, *t2;

	t1 = v1;
	t2 = v2;
	REQUIRE(VALID_TIMER(t1));
	REQUIRE(VALID_TIMER(t2));

	if (isc_time_compare(&t1->due, &t2->due) < 0)
		return (ISC_TRUE);
	return (ISC_FALSE);
}

static void
set_index(void *what, unsigned int index) {
	isc_timer_t *timer;

	timer = what;
	REQUIRE(VALID_TIMER(timer));

	timer->index = index;
}

isc_result_t
isc_timermgr_create(isc_mem_t *mctx, isc_timermgr_t **managerp) {
	isc_timermgr_t *manager;
	isc_result_t result;

	/*
	 * Create a timer manager.
	 */

	REQUIRE(managerp != NULL && *managerp == NULL);

#ifndef ISC_PLATFORM_USETHREADS
	if (timermgr != NULL) {
		timermgr->refs++;
		*managerp = timermgr;
		return (ISC_R_SUCCESS);
	}
#endif /* ISC_PLATFORM_USETHREADS */

	manager = isc_mem_get(mctx, sizeof(*manager));
	if (manager == NULL)
		return (ISC_R_NOMEMORY);

	manager->magic = TIMER_MANAGER_MAGIC;
	manager->mctx = NULL;
	manager->done = ISC_FALSE;
	INIT_LIST(manager->timers);
	manager->nscheduled = 0;
	isc_time_settoepoch(&manager->due);
	manager->heap = NULL;
	result = isc_heap_create(mctx, sooner, set_index, 0, &manager->heap);
	if (result != ISC_R_SUCCESS) {
		INSIST(result == ISC_R_NOMEMORY);
		isc_mem_put(mctx, manager, sizeof(*manager));
		return (ISC_R_NOMEMORY);
	}
	result = isc_mutex_init(&manager->lock);
	if (result != ISC_R_SUCCESS) {
		isc_heap_destroy(&manager->heap);
		isc_mem_put(mctx, manager, sizeof(*manager));
		return (result);
	}
	isc_mem_attach(mctx, &manager->mctx);
#ifdef ISC_PLATFORM_USETHREADS
	if (isc_condition_init(&manager->wakeup) != ISC_R_SUCCESS) {
		isc_mem_detach(&manager->mctx);
		DESTROYLOCK(&manager->lock);
		isc_heap_destroy(&manager->heap);
		isc_mem_put(mctx, manager, sizeof(*manager));
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_condition_init() %s",
				 isc_msgcat_get(isc_msgcat, ISC_MSGSET_GENERAL,
						ISC_MSG_FAILED, "failed"));
		return (ISC_R_UNEXPECTED);
	}
	if (isc_thread_create(run, manager, &manager->thread) !=
	    ISC_R_SUCCESS) {
		isc_mem_detach(&manager->mctx);
		(void)isc_condition_destroy(&manager->wakeup);
		DESTROYLOCK(&manager->lock);
		isc_heap_destroy(&manager->heap);
		isc_mem_put(mctx, manager, sizeof(*manager));
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_thread_create() %s",
				 isc_msgcat_get(isc_msgcat, ISC_MSGSET_GENERAL,
						ISC_MSG_FAILED, "failed"));
		return (ISC_R_UNEXPECTED);
	}
#else /* ISC_PLATFORM_USETHREADS */
	manager->refs = 1;
	timermgr = manager;
#endif /* ISC_PLATFORM_USETHREADS */

	*managerp = manager;

	return (ISC_R_SUCCESS);
}

void
isc_timermgr_poke(isc_timermgr_t *manager) {
#ifdef ISC_PLATFORM_USETHREADS
	REQUIRE(VALID_MANAGER(manager));

	SIGNAL(&manager->wakeup);
#else
	UNUSED(manager);
#endif
}

void
isc_timermgr_destroy(isc_timermgr_t **managerp) {
	isc_timermgr_t *manager;
	isc_mem_t *mctx;

	/*
	 * Destroy a timer manager.
	 */

	REQUIRE(managerp != NULL);
	manager = *managerp;
	REQUIRE(VALID_MANAGER(manager));

	LOCK(&manager->lock);

#ifndef ISC_PLATFORM_USETHREADS
	if (manager->refs > 1) {
		manager->refs--;
		UNLOCK(&manager->lock);
		*managerp = NULL;
		return;
	}

	isc__timermgr_dispatch();
#endif /* ISC_PLATFORM_USETHREADS */

	REQUIRE(EMPTY(manager->timers));
	manager->done = ISC_TRUE;

#ifdef ISC_PLATFORM_USETHREADS
	XTRACE(isc_msgcat_get(isc_msgcat, ISC_MSGSET_TIMER,
			      ISC_MSG_SIGNALDESTROY, "signal (destroy)"));
	SIGNAL(&manager->wakeup);
#endif /* ISC_PLATFORM_USETHREADS */

	UNLOCK(&manager->lock);

#ifdef ISC_PLATFORM_USETHREADS
	/*
	 * Wait for thread to exit.
	 */
	if (isc_thread_join(manager->thread, NULL) != ISC_R_SUCCESS)
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_thread_join() %s",
				 isc_msgcat_get(isc_msgcat, ISC_MSGSET_GENERAL,
						ISC_MSG_FAILED, "failed"));
#endif /* ISC_PLATFORM_USETHREADS */

	/*
	 * Clean up.
	 */
#ifdef ISC_PLATFORM_USETHREADS
	(void)isc_condition_destroy(&manager->wakeup);
#endif /* ISC_PLATFORM_USETHREADS */
	DESTROYLOCK(&manager->lock);
	isc_heap_destroy(&manager->heap);
	manager->magic = 0;
	mctx = manager->mctx;
	isc_mem_put(mctx, manager, sizeof(*manager));
	isc_mem_detach(&mctx);

	*managerp = NULL;
}

#ifndef ISC_PLATFORM_USETHREADS
isc_result_t
isc__timermgr_nextevent(isc_time_t *when) {
	if (timermgr == NULL || timermgr->nscheduled == 0)
		return (ISC_R_NOTFOUND);
	*when = timermgr->due;
	return (ISC_R_SUCCESS);
}

void
isc__timermgr_dispatch(void) {
	isc_time_t now;
	if (timermgr == NULL)
		return;
	TIME_NOW(&now);
	dispatch(timermgr, &now);
}
#endif /* ISC_PLATFORM_USETHREADS */
