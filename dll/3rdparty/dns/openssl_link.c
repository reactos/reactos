/*
 * Portions Copyright (C) 2004-2009  Internet Systems Consortium, Inc. ("ISC")
 * Portions Copyright (C) 1999-2003  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC AND NETWORK ASSOCIATES DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE
 * FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Portions Copyright (C) 1995-2000 by Network Associates, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC AND NETWORK ASSOCIATES DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE
 * FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Principal Author: Brian Wellington
 * $Id: openssl_link.c,v 1.22.112.3 2009/02/11 03:07:01 jinmei Exp $
 */
#ifdef OPENSSL

#include <config.h>

#include <isc/entropy.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/mutexblock.h>
#include <isc/string.h>
#include <isc/thread.h>
#include <isc/util.h>

#include "dst_internal.h"
#include "dst_openssl.h"

#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/conf.h>
#include <openssl/crypto.h>

#if defined(CRYPTO_LOCK_ENGINE) && (OPENSSL_VERSION_NUMBER >= 0x0090707f)
#define USE_ENGINE 1
#endif

#ifdef USE_ENGINE
#include <openssl/engine.h>

#ifdef ENGINE_ID
const char *engine_id = ENGINE_ID;
#else
const char *engine_id;
#endif
#endif

static RAND_METHOD *rm = NULL;

static isc_mutex_t *locks = NULL;
static int nlocks;

#ifdef USE_ENGINE
static ENGINE *e;
static ENGINE *he;
#endif

#ifdef USE_PKCS11
static isc_result_t
dst__openssl_load_engine(const char *name, const char *engine_id,
			 const char **pre_cmds, int pre_num,
			 const char **post_cmds, int post_num);
#endif

static int
entropy_get(unsigned char *buf, int num) {
	isc_result_t result;
	if (num < 0)
		return (-1);
	result = dst__entropy_getdata(buf, (unsigned int) num, ISC_FALSE);
	return (result == ISC_R_SUCCESS ? num : -1);
}

static int
entropy_status(void) {
	return (dst__entropy_status() > 32);
}

static int
entropy_getpseudo(unsigned char *buf, int num) {
	isc_result_t result;
	if (num < 0)
		return (-1);
	result = dst__entropy_getdata(buf, (unsigned int) num, ISC_TRUE);
	return (result == ISC_R_SUCCESS ? num : -1);
}

static void
entropy_add(const void *buf, int num, double entropy) {
	/*
	 * Do nothing.  The only call to this provides no useful data anyway.
	 */
	UNUSED(buf);
	UNUSED(num);
	UNUSED(entropy);
}

static void
lock_callback(int mode, int type, const char *file, int line) {
	UNUSED(file);
	UNUSED(line);
	if ((mode & CRYPTO_LOCK) != 0)
		LOCK(&locks[type]);
	else
		UNLOCK(&locks[type]);
}

static unsigned long
id_callback(void) {
	return ((unsigned long)isc_thread_self());
}

static void *
mem_alloc(size_t size) {
	INSIST(dst__memory_pool != NULL);
	return (isc_mem_allocate(dst__memory_pool, size));
}

static void
mem_free(void *ptr) {
	INSIST(dst__memory_pool != NULL);
	if (ptr != NULL)
		isc_mem_free(dst__memory_pool, ptr);
}

static void *
mem_realloc(void *ptr, size_t size) {
	INSIST(dst__memory_pool != NULL);
	return (isc_mem_reallocate(dst__memory_pool, ptr, size));
}

isc_result_t
dst__openssl_init() {
	isc_result_t result;
#ifdef USE_ENGINE
	/* const char  *name; */
	ENGINE *re;
#endif

#ifdef  DNS_CRYPTO_LEAKS
	CRYPTO_malloc_debug_init();
	CRYPTO_set_mem_debug_options(V_CRYPTO_MDEBUG_ALL);
	CRYPTO_mem_ctrl(CRYPTO_MEM_CHECK_ON);
#endif
	CRYPTO_set_mem_functions(mem_alloc, mem_realloc, mem_free);
	nlocks = CRYPTO_num_locks();
	locks = mem_alloc(sizeof(isc_mutex_t) * nlocks);
	if (locks == NULL)
		return (ISC_R_NOMEMORY);
	result = isc_mutexblock_init(locks, nlocks);
	if (result != ISC_R_SUCCESS)
		goto cleanup_mutexalloc;
	CRYPTO_set_locking_callback(lock_callback);
	CRYPTO_set_id_callback(id_callback);

	rm = mem_alloc(sizeof(RAND_METHOD));
	if (rm == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup_mutexinit;
	}
	rm->seed = NULL;
	rm->bytes = entropy_get;
	rm->cleanup = NULL;
	rm->add = entropy_add;
	rm->pseudorand = entropy_getpseudo;
	rm->status = entropy_status;
#ifdef USE_ENGINE
	OPENSSL_config(NULL);
#ifdef USE_PKCS11
#ifndef PKCS11_SO_PATH
#define PKCS11_SO_PATH		"/usr/local/lib/engines/engine_pkcs11.so"
#endif
#ifndef PKCS11_MODULE_PATH
#define PKCS11_MODULE_PATH	"/usr/lib/libpkcs11.so"
#endif
	{
		/*
		 * to use this to config the PIN, add in openssl.cnf:
		 *  - at the beginning: "openssl_conf = openssl_def"
		 *  - at any place these sections:
		 * [ openssl_def ]
		 * engines = engine_section
		 * [ engine_section ]
		 * pkcs11 = pkcs11_section
		 * [ pkcs11_section ]
		 * PIN = my___pin
		 */

		const char *pre_cmds[] = {
			"SO_PATH", PKCS11_SO_PATH,
			"LOAD", NULL,
			"MODULE_PATH", PKCS11_MODULE_PATH
		};
		const char *post_cmds[] = {
			/* "PIN", "my___pin" */
		};
		result = dst__openssl_load_engine("pkcs11", "pkcs11",
						  pre_cmds, 0,
						  post_cmds, /*1*/ 0);
		if (result != ISC_R_SUCCESS)
			goto cleanup_rm;
	}
#endif /* USE_PKCS11 */
	if (engine_id != NULL) {
		e = ENGINE_by_id(engine_id);
		if (e == NULL) {
			result = ISC_R_NOTFOUND;
			goto cleanup_rm;
		}
		if (!ENGINE_init(e)) {
			result = ISC_R_FAILURE;
			ENGINE_free(e);
			goto cleanup_rm;
		}
		ENGINE_set_default(e, ENGINE_METHOD_ALL);
		ENGINE_free(e);
	} else {
		ENGINE_register_all_complete();
		for (e = ENGINE_get_first(); e != NULL; e = ENGINE_get_next(e)) {

			/*
			 * Something weird here. If we call ENGINE_finish()
			 * ENGINE_get_default_RAND() will fail.
			 */
			if (ENGINE_init(e)) {
				if (he == NULL)
					he = e;
			}
		}
	}
	re = ENGINE_get_default_RAND();
	if (re == NULL) {
		re = ENGINE_new();
		if (re == NULL) {
			result = ISC_R_NOMEMORY;
			goto cleanup_rm;
		}
		ENGINE_set_RAND(re, rm);
		ENGINE_set_default_RAND(re);
		ENGINE_free(re);
	} else
		ENGINE_finish(re);

#else
	RAND_set_rand_method(rm);
#endif /* USE_ENGINE */
	return (ISC_R_SUCCESS);

#ifdef USE_ENGINE
 cleanup_rm:
	mem_free(rm);
#endif
 cleanup_mutexinit:
	CRYPTO_set_locking_callback(NULL);
	DESTROYMUTEXBLOCK(locks, nlocks);
 cleanup_mutexalloc:
	mem_free(locks);
	return (result);
}

void
dst__openssl_destroy() {

	/*
	 * Sequence taken from apps_shutdown() in <apps/apps.h>.
	 */
#if (OPENSSL_VERSION_NUMBER >= 0x00907000L)
	CONF_modules_unload(1);
#endif
	EVP_cleanup();
#if defined(USE_ENGINE)
	if (e != NULL) {
		ENGINE_finish(e);
		e = NULL;
	}
#if defined(USE_ENGINE) && OPENSSL_VERSION_NUMBER >= 0x00907000L
	ENGINE_cleanup();
#endif
#endif
#if (OPENSSL_VERSION_NUMBER >= 0x00907000L)
	CRYPTO_cleanup_all_ex_data();
#endif
	ERR_clear_error();
	ERR_free_strings();
	ERR_remove_state(0);

#ifdef  DNS_CRYPTO_LEAKS
	CRYPTO_mem_leaks_fp(stderr);
#endif

	if (rm != NULL) {
#if OPENSSL_VERSION_NUMBER >= 0x00907000L
		RAND_cleanup();
#endif
		mem_free(rm);
	}
	if (locks != NULL) {
		CRYPTO_set_locking_callback(NULL);
		DESTROYMUTEXBLOCK(locks, nlocks);
		mem_free(locks);
	}
}

isc_result_t
dst__openssl_toresult(isc_result_t fallback) {
	isc_result_t result = fallback;
	int err = ERR_get_error();

	switch (ERR_GET_REASON(err)) {
	case ERR_R_MALLOC_FAILURE:
		result = ISC_R_NOMEMORY;
		break;
	default:
		break;
	}
	ERR_clear_error();
	return (result);
}

ENGINE *
dst__openssl_getengine(const char *name) {

	UNUSED(name);


#if defined(USE_ENGINE)
	return (he);
#else
	return (NULL);
#endif
}

isc_result_t
dst__openssl_setdefault(const char *name) {

	UNUSED(name);

#if defined(USE_ENGINE)
	ENGINE_set_default(e, ENGINE_METHOD_ALL);
#endif
	/*
	 * XXXMPA If the engine does not have a default RAND method
	 * restore our method.
	 */
	return (ISC_R_SUCCESS);
}

#ifdef USE_PKCS11
/*
 * 'name' is the name the engine is known by to the dst library.
 * This may or may not match the name the engine is known by to
 * openssl.  It is the name that is stored in the private key file.
 *
 * 'engine_id' is the openssl engine name.
 *
 * pre_cmds and post_cmds a sequence if command argument pairs
 * pre_num and post_num are a count of those pairs.
 *
 * "SO_PATH", PKCS11_SO_PATH ("/usr/local/lib/engines/engine_pkcs11.so")
 * "LOAD", NULL
 * "MODULE_PATH", PKCS11_MODULE_PATH ("/usr/lib/libpkcs11.so")
 */
static isc_result_t
dst__openssl_load_engine(const char *name, const char *engine_id,
			 const char **pre_cmds, int pre_num,
			 const char **post_cmds, int post_num)
{
	ENGINE *e;

	UNUSED(name);

	if (!strcasecmp(engine_id, "dynamic"))
		ENGINE_load_dynamic();
	e = ENGINE_by_id(engine_id);
	if (e == NULL)
		return (ISC_R_NOTFOUND);
	while (pre_num--) {
		if (!ENGINE_ctrl_cmd_string(e, pre_cmds[0], pre_cmds[1], 0)) {
			ENGINE_free(e);
			return (ISC_R_FAILURE);
		}
		pre_cmds += 2;
	}
	if (!ENGINE_init(e)) {
		ENGINE_free(e);
		return (ISC_R_FAILURE);
	}
	/*
	 * ENGINE_init() returned a functional reference, so free the
	 * structural reference from ENGINE_by_id().
	 */
	ENGINE_free(e);
	while (post_num--) {
		if (!ENGINE_ctrl_cmd_string(e, post_cmds[0], post_cmds[1], 0)) {
			ENGINE_free(e);
			return (ISC_R_FAILURE);
		}
		post_cmds += 2;
	}
	if (he != NULL)
		ENGINE_finish(he);
	he = e;
	return (ISC_R_SUCCESS);
}
#endif /* USE_PKCS11 */

#else /* OPENSSL */

#include <isc/util.h>

EMPTY_TRANSLATION_UNIT

#endif /* OPENSSL */
/*! \file */
