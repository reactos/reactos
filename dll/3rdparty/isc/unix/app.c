/*
 * Copyright (C) 2004, 2005, 2007, 2008  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2003  Internet Software Consortium.
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

/* $Id: app.c,v 1.60 2008/10/15 03:41:17 marka Exp $ */

/*! \file */

#include <config.h>

#include <sys/param.h>	/* Openserver 5.0.6A and FD_SETSIZE */
#include <sys/types.h>

#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#ifdef HAVE_EPOLL
#include <sys/epoll.h>
#endif

#include <isc/app.h>
#include <isc/boolean.h>
#include <isc/condition.h>
#include <isc/msgs.h>
#include <isc/mutex.h>
#include <isc/event.h>
#include <isc/platform.h>
#include <isc/strerror.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/time.h>
#include <isc/util.h>

#ifdef ISC_PLATFORM_USETHREADS
#include <pthread.h>
#else /* ISC_PLATFORM_USETHREADS */
#include "../timer_p.h"
#include "../task_p.h"
#include "socket_p.h"
#endif /* ISC_PLATFORM_USETHREADS */

static isc_eventlist_t		on_run;
static isc_mutex_t		lock;
static isc_boolean_t		shutdown_requested = ISC_FALSE;
static isc_boolean_t		running = ISC_FALSE;
/*!
 * We assume that 'want_shutdown' can be read and written atomically.
 */
static volatile isc_boolean_t	want_shutdown = ISC_FALSE;
/*
 * We assume that 'want_reload' can be read and written atomically.
 */
static volatile isc_boolean_t	want_reload = ISC_FALSE;

static isc_boolean_t		blocked  = ISC_FALSE;
#ifdef ISC_PLATFORM_USETHREADS
static pthread_t		blockedthread;
#endif /* ISC_PLATFORM_USETHREADS */

#ifdef HAVE_LINUXTHREADS
/*!
 * Linux has sigwait(), but it appears to prevent signal handlers from
 * running, even if they're not in the set being waited for.  This makes
 * it impossible to get the default actions for SIGILL, SIGSEGV, etc.
 * Instead of messing with it, we just use sigsuspend() instead.
 */
#undef HAVE_SIGWAIT
/*!
 * We need to remember which thread is the main thread...
 */
static pthread_t		main_thread;
#endif

#ifndef HAVE_SIGWAIT
static void
exit_action(int arg) {
	UNUSED(arg);
	want_shutdown = ISC_TRUE;
}

static void
reload_action(int arg) {
	UNUSED(arg);
	want_reload = ISC_TRUE;
}
#endif

static isc_result_t
handle_signal(int sig, void (*handler)(int)) {
	struct sigaction sa;
	char strbuf[ISC_STRERRORSIZE];

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;

	if (sigfillset(&sa.sa_mask) != 0 ||
	    sigaction(sig, &sa, NULL) < 0) {
		isc__strerror(errno, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 isc_msgcat_get(isc_msgcat, ISC_MSGSET_APP,
					       ISC_MSG_SIGNALSETUP,
					       "handle_signal() %d setup: %s"),
				 sig, strbuf);
		return (ISC_R_UNEXPECTED);
	}

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_app_start(void) {
	isc_result_t result;
	int presult;
	sigset_t sset;
	char strbuf[ISC_STRERRORSIZE];

	/*
	 * Start an ISC library application.
	 */

#ifdef NEED_PTHREAD_INIT
	/*
	 * BSDI 3.1 seg faults in pthread_sigmask() if we don't do this.
	 */
	presult = pthread_init();
	if (presult != 0) {
		isc__strerror(presult, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_app_start() pthread_init: %s", strbuf);
		return (ISC_R_UNEXPECTED);
	}
#endif

#ifdef HAVE_LINUXTHREADS
	main_thread = pthread_self();
#endif

	result = isc_mutex_init(&lock);
	if (result != ISC_R_SUCCESS)
		return (result);

#ifndef HAVE_SIGWAIT
	/*
	 * Install do-nothing handlers for SIGINT and SIGTERM.
	 *
	 * We install them now because BSDI 3.1 won't block
	 * the default actions, regardless of what we do with
	 * pthread_sigmask().
	 */
	result = handle_signal(SIGINT, exit_action);
	if (result != ISC_R_SUCCESS)
		return (result);
	result = handle_signal(SIGTERM, exit_action);
	if (result != ISC_R_SUCCESS)
		return (result);
#endif

	/*
	 * Always ignore SIGPIPE.
	 */
	result = handle_signal(SIGPIPE, SIG_IGN);
	if (result != ISC_R_SUCCESS)
		return (result);

	/*
	 * On Solaris 2, delivery of a signal whose action is SIG_IGN
	 * will not cause sigwait() to return. We may have inherited
	 * unexpected actions for SIGHUP, SIGINT, and SIGTERM from our parent
	 * process (e.g, Solaris cron).  Set an action of SIG_DFL to make
	 * sure sigwait() works as expected.  Only do this for SIGTERM and
	 * SIGINT if we don't have sigwait(), since a different handler is
	 * installed above.
	 */
	result = handle_signal(SIGHUP, SIG_DFL);
	if (result != ISC_R_SUCCESS)
		return (result);

#ifdef HAVE_SIGWAIT
	result = handle_signal(SIGTERM, SIG_DFL);
	if (result != ISC_R_SUCCESS)
		return (result);
	result = handle_signal(SIGINT, SIG_DFL);
	if (result != ISC_R_SUCCESS)
		return (result);
#endif

#ifdef ISC_PLATFORM_USETHREADS
	/*
	 * Block SIGHUP, SIGINT, SIGTERM.
	 *
	 * If isc_app_start() is called from the main thread before any other
	 * threads have been created, then the pthread_sigmask() call below
	 * will result in all threads having SIGHUP, SIGINT and SIGTERM
	 * blocked by default, ensuring that only the thread that calls
	 * sigwait() for them will get those signals.
	 */
	if (sigemptyset(&sset) != 0 ||
	    sigaddset(&sset, SIGHUP) != 0 ||
	    sigaddset(&sset, SIGINT) != 0 ||
	    sigaddset(&sset, SIGTERM) != 0) {
		isc__strerror(errno, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_app_start() sigsetops: %s", strbuf);
		return (ISC_R_UNEXPECTED);
	}
	presult = pthread_sigmask(SIG_BLOCK, &sset, NULL);
	if (presult != 0) {
		isc__strerror(presult, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_app_start() pthread_sigmask: %s",
				 strbuf);
		return (ISC_R_UNEXPECTED);
	}
#else /* ISC_PLATFORM_USETHREADS */
	/*
	 * Unblock SIGHUP, SIGINT, SIGTERM.
	 *
	 * If we're not using threads, we need to make sure that SIGHUP,
	 * SIGINT and SIGTERM are not inherited as blocked from the parent
	 * process.
	 */
	if (sigemptyset(&sset) != 0 ||
	    sigaddset(&sset, SIGHUP) != 0 ||
	    sigaddset(&sset, SIGINT) != 0 ||
	    sigaddset(&sset, SIGTERM) != 0) {
		isc__strerror(errno, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_app_start() sigsetops: %s", strbuf);
		return (ISC_R_UNEXPECTED);
	}
	presult = sigprocmask(SIG_UNBLOCK, &sset, NULL);
	if (presult != 0) {
		isc__strerror(presult, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_app_start() sigprocmask: %s", strbuf);
		return (ISC_R_UNEXPECTED);
	}
#endif /* ISC_PLATFORM_USETHREADS */

	ISC_LIST_INIT(on_run);

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_app_onrun(isc_mem_t *mctx, isc_task_t *task, isc_taskaction_t action,
	      void *arg)
{
	isc_event_t *event;
	isc_task_t *cloned_task = NULL;
	isc_result_t result;

	LOCK(&lock);

	if (running) {
		result = ISC_R_ALREADYRUNNING;
		goto unlock;
	}

	/*
	 * Note that we store the task to which we're going to send the event
	 * in the event's "sender" field.
	 */
	isc_task_attach(task, &cloned_task);
	event = isc_event_allocate(mctx, cloned_task, ISC_APPEVENT_SHUTDOWN,
				   action, arg, sizeof(*event));
	if (event == NULL) {
		result = ISC_R_NOMEMORY;
		goto unlock;
	}

	ISC_LIST_APPEND(on_run, event, ev_link);

	result = ISC_R_SUCCESS;

 unlock:
	UNLOCK(&lock);

	return (result);
}

#ifndef ISC_PLATFORM_USETHREADS
/*!
 * Event loop for nonthreaded programs.
 */
static isc_result_t
evloop(void) {
	isc_result_t result;
	while (!want_shutdown) {
		int n;
		isc_time_t when, now;
		struct timeval tv, *tvp;
		isc_socketwait_t *swait;
		isc_boolean_t readytasks;
		isc_boolean_t call_timer_dispatch = ISC_FALSE;

		readytasks = isc__taskmgr_ready();
		if (readytasks) {
			tv.tv_sec = 0;
			tv.tv_usec = 0;
			tvp = &tv;
			call_timer_dispatch = ISC_TRUE;
		} else {
			result = isc__timermgr_nextevent(&when);
			if (result != ISC_R_SUCCESS)
				tvp = NULL;
			else {
				isc_uint64_t us;

				TIME_NOW(&now);
				us = isc_time_microdiff(&when, &now);
				if (us == 0)
					call_timer_dispatch = ISC_TRUE;
				tv.tv_sec = us / 1000000;
				tv.tv_usec = us % 1000000;
				tvp = &tv;
			}
		}

		swait = NULL;
		n = isc__socketmgr_waitevents(tvp, &swait);

		if (n == 0 || call_timer_dispatch) {
			/*
			 * We call isc__timermgr_dispatch() only when
			 * necessary, in order to reduce overhead.  If the
			 * select() call indicates a timeout, we need the
			 * dispatch.  Even if not, if we set the 0-timeout
			 * for the select() call, we need to check the timer
			 * events.  In the 'readytasks' case, there may be no
			 * timeout event actually, but there is no other way
			 * to reduce the overhead.
			 * Note that we do not have to worry about the case
			 * where a new timer is inserted during the select()
			 * call, since this loop only runs in the non-thread
			 * mode.
			 */
			isc__timermgr_dispatch();
		}
		if (n > 0)
			(void)isc__socketmgr_dispatch(swait);
		(void)isc__taskmgr_dispatch();

		if (want_reload) {
			want_reload = ISC_FALSE;
			return (ISC_R_RELOAD);
		}
	}
	return (ISC_R_SUCCESS);
}

/*
 * This is a gross hack to support waiting for condition
 * variables in nonthreaded programs in a limited way;
 * see lib/isc/nothreads/include/isc/condition.h.
 * We implement isc_condition_wait() by entering the
 * event loop recursively until the want_shutdown flag
 * is set by isc_condition_signal().
 */

/*!
 * \brief True if we are currently executing in the recursive
 * event loop.
 */
static isc_boolean_t in_recursive_evloop = ISC_FALSE;

/*!
 * \brief True if we are exiting the event loop as the result of
 * a call to isc_condition_signal() rather than a shutdown
 * or reload.
 */
static isc_boolean_t signalled = ISC_FALSE;

isc_result_t
isc__nothread_wait_hack(isc_condition_t *cp, isc_mutex_t *mp) {
	isc_result_t result;

	UNUSED(cp);
	UNUSED(mp);

	INSIST(!in_recursive_evloop);
	in_recursive_evloop = ISC_TRUE;

	INSIST(*mp == 1); /* Mutex must be locked on entry. */
	--*mp;

	result = evloop();
	if (result == ISC_R_RELOAD)
		want_reload = ISC_TRUE;
	if (signalled) {
		want_shutdown = ISC_FALSE;
		signalled = ISC_FALSE;
	}

	++*mp;
	in_recursive_evloop = ISC_FALSE;
	return (ISC_R_SUCCESS);
}

isc_result_t
isc__nothread_signal_hack(isc_condition_t *cp) {

	UNUSED(cp);

	INSIST(in_recursive_evloop);

	want_shutdown = ISC_TRUE;
	signalled = ISC_TRUE;
	return (ISC_R_SUCCESS);
}

#endif /* ISC_PLATFORM_USETHREADS */

isc_result_t
isc_app_run(void) {
	int result;
	isc_event_t *event, *next_event;
	isc_task_t *task;
#ifdef ISC_PLATFORM_USETHREADS
	sigset_t sset;
	char strbuf[ISC_STRERRORSIZE];
#ifdef HAVE_SIGWAIT
	int sig;
#endif
#endif /* ISC_PLATFORM_USETHREADS */

#ifdef HAVE_LINUXTHREADS
	REQUIRE(main_thread == pthread_self());
#endif

	LOCK(&lock);

	if (!running) {
		running = ISC_TRUE;

		/*
		 * Post any on-run events (in FIFO order).
		 */
		for (event = ISC_LIST_HEAD(on_run);
		     event != NULL;
		     event = next_event) {
			next_event = ISC_LIST_NEXT(event, ev_link);
			ISC_LIST_UNLINK(on_run, event, ev_link);
			task = event->ev_sender;
			event->ev_sender = NULL;
			isc_task_sendanddetach(&task, &event);
		}

	}

	UNLOCK(&lock);

#ifndef HAVE_SIGWAIT
	/*
	 * Catch SIGHUP.
	 *
	 * We do this here to ensure that the signal handler is installed
	 * (i.e. that it wasn't a "one-shot" handler).
	 */
	result = handle_signal(SIGHUP, reload_action);
	if (result != ISC_R_SUCCESS)
		return (ISC_R_SUCCESS);
#endif

#ifdef ISC_PLATFORM_USETHREADS
	/*
	 * There is no danger if isc_app_shutdown() is called before we wait
	 * for signals.  Signals are blocked, so any such signal will simply
	 * be made pending and we will get it when we call sigwait().
	 */

	while (!want_shutdown) {
#ifdef HAVE_SIGWAIT
		/*
		 * Wait for SIGHUP, SIGINT, or SIGTERM.
		 */
		if (sigemptyset(&sset) != 0 ||
		    sigaddset(&sset, SIGHUP) != 0 ||
		    sigaddset(&sset, SIGINT) != 0 ||
		    sigaddset(&sset, SIGTERM) != 0) {
			isc__strerror(errno, strbuf, sizeof(strbuf));
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "isc_app_run() sigsetops: %s", strbuf);
			return (ISC_R_UNEXPECTED);
		}

#ifndef HAVE_UNIXWARE_SIGWAIT
		result = sigwait(&sset, &sig);
		if (result == 0) {
			if (sig == SIGINT ||
			    sig == SIGTERM)
				want_shutdown = ISC_TRUE;
			else if (sig == SIGHUP)
				want_reload = ISC_TRUE;
		}

#else /* Using UnixWare sigwait semantics. */
		sig = sigwait(&sset);
		if (sig >= 0) {
			if (sig == SIGINT ||
			    sig == SIGTERM)
				want_shutdown = ISC_TRUE;
			else if (sig == SIGHUP)
				want_reload = ISC_TRUE;
		}

#endif /* HAVE_UNIXWARE_SIGWAIT */
#else  /* Don't have sigwait(). */
		/*
		 * Listen for all signals.
		 */
		if (sigemptyset(&sset) != 0) {
			isc__strerror(errno, strbuf, sizeof(strbuf));
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "isc_app_run() sigsetops: %s", strbuf);
			return (ISC_R_UNEXPECTED);
		}
		result = sigsuspend(&sset);
#endif /* HAVE_SIGWAIT */

		if (want_reload) {
			want_reload = ISC_FALSE;
			return (ISC_R_RELOAD);
		}

		if (want_shutdown && blocked)
			exit(1);
	}

#else /* ISC_PLATFORM_USETHREADS */

	(void)isc__taskmgr_dispatch();

	result = evloop();
	if (result != ISC_R_SUCCESS)
		return (result);

#endif /* ISC_PLATFORM_USETHREADS */

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_app_shutdown(void) {
	isc_boolean_t want_kill = ISC_TRUE;
	char strbuf[ISC_STRERRORSIZE];

	LOCK(&lock);

	REQUIRE(running);

	if (shutdown_requested)
		want_kill = ISC_FALSE;
	else
		shutdown_requested = ISC_TRUE;

	UNLOCK(&lock);

	if (want_kill) {
#ifdef HAVE_LINUXTHREADS
		int result;

		result = pthread_kill(main_thread, SIGTERM);
		if (result != 0) {
			isc__strerror(result, strbuf, sizeof(strbuf));
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "isc_app_shutdown() pthread_kill: %s",
					 strbuf);
			return (ISC_R_UNEXPECTED);
		}
#else
		if (kill(getpid(), SIGTERM) < 0) {
			isc__strerror(errno, strbuf, sizeof(strbuf));
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "isc_app_shutdown() kill: %s", strbuf);
			return (ISC_R_UNEXPECTED);
		}
#endif
	}

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_app_reload(void) {
	isc_boolean_t want_kill = ISC_TRUE;
	char strbuf[ISC_STRERRORSIZE];

	LOCK(&lock);

	REQUIRE(running);

	/*
	 * Don't send the reload signal if we're shutting down.
	 */
	if (shutdown_requested)
		want_kill = ISC_FALSE;

	UNLOCK(&lock);

	if (want_kill) {
#ifdef HAVE_LINUXTHREADS
		int result;

		result = pthread_kill(main_thread, SIGHUP);
		if (result != 0) {
			isc__strerror(result, strbuf, sizeof(strbuf));
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "isc_app_reload() pthread_kill: %s",
					 strbuf);
			return (ISC_R_UNEXPECTED);
		}
#else
		if (kill(getpid(), SIGHUP) < 0) {
			isc__strerror(errno, strbuf, sizeof(strbuf));
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "isc_app_reload() kill: %s", strbuf);
			return (ISC_R_UNEXPECTED);
		}
#endif
	}

	return (ISC_R_SUCCESS);
}

void
isc_app_finish(void) {
	DESTROYLOCK(&lock);
}

void
isc_app_block(void) {
#ifdef ISC_PLATFORM_USETHREADS
	sigset_t sset;
#endif /* ISC_PLATFORM_USETHREADS */
	REQUIRE(running);
	REQUIRE(!blocked);

	blocked = ISC_TRUE;
#ifdef ISC_PLATFORM_USETHREADS
	blockedthread = pthread_self();
	RUNTIME_CHECK(sigemptyset(&sset) == 0 &&
		      sigaddset(&sset, SIGINT) == 0 &&
		      sigaddset(&sset, SIGTERM) == 0);
	RUNTIME_CHECK(pthread_sigmask(SIG_UNBLOCK, &sset, NULL) == 0);
#endif /* ISC_PLATFORM_USETHREADS */
}

void
isc_app_unblock(void) {
#ifdef ISC_PLATFORM_USETHREADS
	sigset_t sset;
#endif /* ISC_PLATFORM_USETHREADS */

	REQUIRE(running);
	REQUIRE(blocked);

	blocked = ISC_FALSE;

#ifdef ISC_PLATFORM_USETHREADS
	REQUIRE(blockedthread == pthread_self());

	RUNTIME_CHECK(sigemptyset(&sset) == 0 &&
		      sigaddset(&sset, SIGINT) == 0 &&
		      sigaddset(&sset, SIGTERM) == 0);
	RUNTIME_CHECK(pthread_sigmask(SIG_BLOCK, &sset, NULL) == 0);
#endif /* ISC_PLATFORM_USETHREADS */
}
