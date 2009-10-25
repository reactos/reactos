/*
 * Portions Copyright (C) 2005, 2007, 2009  Internet Systems Consortium, Inc. ("ISC")
 * Portions Copyright (C) 1999-2001  Internet Software Consortium.
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

/*
 * Copyright (C) 2002 Stichting NLnet, Netherlands, stichting@nlnet.nl.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND STICHTING NLNET
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 * STICHTING NLNET BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
 * USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * The development of Dynamically Loadable Zones (DLZ) for Bind 9 was
 * conceived and contributed by Rob Butler.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ROB BUTLER
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 * ROB BUTLER BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
 * USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* $Id: dlz.c,v 1.5.332.2 2009/01/18 23:47:35 tbox Exp $ */

/*! \file */

/***
 *** Imports
 ***/

#include <config.h>

#include <dns/fixedname.h>
#include <dns/log.h>
#include <dns/master.h>
#include <dns/dlz.h>


#include <isc/buffer.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/once.h>
#include <isc/rwlock.h>
#include <isc/string.h>
#include <isc/util.h>

/***
 *** Supported DLZ DB Implementations Registry
 ***/

static ISC_LIST(dns_dlzimplementation_t) dlz_implementations;
static isc_rwlock_t dlz_implock;
static isc_once_t once = ISC_ONCE_INIT;

static void
dlz_initialize(void) {
	RUNTIME_CHECK(isc_rwlock_init(&dlz_implock, 0, 0) == ISC_R_SUCCESS);
	ISC_LIST_INIT(dlz_implementations);
}

/*%
 * Searches the dlz_implementations list for a driver matching name.
 */
static inline dns_dlzimplementation_t *
dlz_impfind(const char *name) {
	dns_dlzimplementation_t *imp;

	for (imp = ISC_LIST_HEAD(dlz_implementations);
	     imp != NULL;
	     imp = ISC_LIST_NEXT(imp, link))
		if (strcasecmp(name, imp->name) == 0)
			return (imp);
	return (NULL);
}

/***
 *** Basic DLZ Methods
 ***/

isc_result_t
dns_dlzallowzonexfr(dns_view_t *view, dns_name_t *name,
		    isc_sockaddr_t *clientaddr, dns_db_t **dbp)
{
	isc_result_t result;
	dns_dlzallowzonexfr_t allowzonexfr;
	dns_dlzdb_t *dlzdatabase;

	/*
	 * Performs checks to make sure data is as we expect it to be.
	 */
	REQUIRE(DNS_DLZ_VALID(view->dlzdatabase));
	REQUIRE(name != NULL);
	REQUIRE(dbp != NULL && *dbp == NULL);

	/* ask driver if the zone is supported */
	dlzdatabase = view->dlzdatabase;
	allowzonexfr = dlzdatabase->implementation->methods->allowzonexfr;
	result = (*allowzonexfr)(dlzdatabase->implementation->driverarg,
				 dlzdatabase->dbdata, dlzdatabase->mctx,
				 view->rdclass, name, clientaddr, dbp);

	if (result == ISC_R_NOTIMPLEMENTED)
		return (ISC_R_NOTFOUND);
	return (result);
}

isc_result_t
dns_dlzcreate(isc_mem_t *mctx, const char *dlzname, const char *drivername,
	      unsigned int argc, char *argv[], dns_dlzdb_t **dbp)
{
	dns_dlzimplementation_t *impinfo;
	isc_result_t result;

	/*
	 * initialize the dlz_implementations list, this is guaranteed
	 * to only really happen once.
	 */
	RUNTIME_CHECK(isc_once_do(&once, dlz_initialize) == ISC_R_SUCCESS);

	/*
	 * Performs checks to make sure data is as we expect it to be.
	 */
	REQUIRE(dbp != NULL && *dbp == NULL);
	REQUIRE(dlzname != NULL);
	REQUIRE(drivername != NULL);
	REQUIRE(mctx != NULL);

	/* write log message */
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
		      DNS_LOGMODULE_DLZ, ISC_LOG_INFO,
		      "Loading '%s' using driver %s", dlzname, drivername);

	/* lock the dlz_implementations list so we can search it. */
	RWLOCK(&dlz_implock, isc_rwlocktype_read);

	/* search for the driver implementation	 */
	impinfo = dlz_impfind(drivername);
	if (impinfo == NULL) {
		RWUNLOCK(&dlz_implock, isc_rwlocktype_read);

		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_DLZ, ISC_LOG_ERROR,
			      "unsupported DLZ database driver '%s'."
			      "  %s not loaded.",
			      drivername, dlzname);

		return (ISC_R_NOTFOUND);
	}

	/* Allocate memory to hold the DLZ database driver */
	(*dbp) = isc_mem_get(mctx, sizeof(dns_dlzdb_t));
	if ((*dbp) == NULL) {
		RWUNLOCK(&dlz_implock, isc_rwlocktype_read);
		return (ISC_R_NOMEMORY);
	}

	/* Make sure memory region is set to all 0's */
	memset((*dbp), 0, sizeof(dns_dlzdb_t));

	(*dbp)->implementation = impinfo;

	/* Create a new database using implementation 'drivername'. */
	result = ((impinfo->methods->create)(mctx, dlzname, argc, argv,
					     impinfo->driverarg,
					     &(*dbp)->dbdata));

	/* mark the DLZ driver as valid */
	if (result == ISC_R_SUCCESS) {
		RWUNLOCK(&dlz_implock, isc_rwlocktype_read);
		(*dbp)->magic = DNS_DLZ_MAGIC;
		isc_mem_attach(mctx, &(*dbp)->mctx);
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_DLZ, ISC_LOG_DEBUG(2),
			      "DLZ driver loaded successfully.");
		return (ISC_R_SUCCESS);
	} else {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_DLZ, ISC_LOG_ERROR,
			      "DLZ driver failed to load.");
	}

	/* impinfo->methods->create failed. */
	RWUNLOCK(&dlz_implock, isc_rwlocktype_read);
	isc_mem_put(mctx, (*dbp), sizeof(dns_dlzdb_t));
	return (result);
}

void
dns_dlzdestroy(dns_dlzdb_t **dbp) {
	isc_mem_t *mctx;
	dns_dlzdestroy_t destroy;

	/* Write debugging message to log */
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
		      DNS_LOGMODULE_DLZ, ISC_LOG_DEBUG(2),
		      "Unloading DLZ driver.");

	/*
	 * Perform checks to make sure data is as we expect it to be.
	 */
	REQUIRE(dbp != NULL && DNS_DLZ_VALID(*dbp));

	/* call the drivers destroy method */
	if ((*dbp) != NULL) {
		mctx = (*dbp)->mctx;
		destroy = (*dbp)->implementation->methods->destroy;
		(*destroy)((*dbp)->implementation->driverarg,(*dbp)->dbdata);
		/* return memory */
		isc_mem_put(mctx, (*dbp), sizeof(dns_dlzdb_t));
		isc_mem_detach(&mctx);
	}

	*dbp = NULL;
}


isc_result_t
dns_dlzfindzone(dns_view_t *view, dns_name_t *name, unsigned int minlabels,
		dns_db_t **dbp)
{
	dns_fixedname_t fname;
	dns_name_t *zonename;
	unsigned int namelabels;
	unsigned int i;
	isc_result_t result;
	dns_dlzfindzone_t findzone;
	dns_dlzdb_t *dlzdatabase;

	/*
	 * Performs checks to make sure data is as we expect it to be.
	 */
	REQUIRE(DNS_DLZ_VALID(view->dlzdatabase));
	REQUIRE(name != NULL);
	REQUIRE(dbp != NULL && *dbp == NULL);

	/* setup a "fixed" dns name */
	dns_fixedname_init(&fname);
	zonename = dns_fixedname_name(&fname);

	/* count the number of labels in the name */
	namelabels = dns_name_countlabels(name);

	/*
	 * loop through starting with the longest domain name and
	 * trying shorter names portions of the name until we find a
	 * match, have an error, or are below the 'minlabels'
	 * threshold.  minlabels is 0, if the standard database didn't
	 * have a zone name match.  Otherwise minlabels is the number
	 * of labels in that name.  We need to beat that for a
	 * "better" match for the DLZ database to be authoritative
	 * instead of the standard database.
	 */
	for (i = namelabels; i > minlabels && i > 1; i--) {
		if (i == namelabels) {
			result = dns_name_copy(name, zonename, NULL);
			if (result != ISC_R_SUCCESS)
				return (result);
		} else
			dns_name_split(name, i, NULL, zonename);

		/* ask SDLZ driver if the zone is supported */
		dlzdatabase = view->dlzdatabase;
		findzone = dlzdatabase->implementation->methods->findzone;
		result = (*findzone)(dlzdatabase->implementation->driverarg,
				     dlzdatabase->dbdata, dlzdatabase->mctx,
				     view->rdclass, zonename, dbp);
		if (result != ISC_R_NOTFOUND)
			return (result);
	}
	return (ISC_R_NOTFOUND);
}

/*%
 * Registers a DLZ driver.  This basically just adds the dlz
 * driver to the list of available drivers in the dlz_implementations list.
 */
isc_result_t
dns_dlzregister(const char *drivername, const dns_dlzmethods_t *methods,
		void *driverarg, isc_mem_t *mctx,
		dns_dlzimplementation_t **dlzimp)
{

	dns_dlzimplementation_t *dlz_imp;

	/* Write debugging message to log */
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
		      DNS_LOGMODULE_DLZ, ISC_LOG_DEBUG(2),
		      "Registering DLZ driver '%s'", drivername);

	/*
	 * Performs checks to make sure data is as we expect it to be.
	 */
	REQUIRE(drivername != NULL);
	REQUIRE(methods != NULL);
	REQUIRE(methods->create != NULL);
	REQUIRE(methods->destroy != NULL);
	REQUIRE(methods->findzone != NULL);
	REQUIRE(mctx != NULL);
	REQUIRE(dlzimp != NULL && *dlzimp == NULL);

	/*
	 * initialize the dlz_implementations list, this is guaranteed
	 * to only really happen once.
	 */
	RUNTIME_CHECK(isc_once_do(&once, dlz_initialize) == ISC_R_SUCCESS);

	/* lock the dlz_implementations list so we can modify it. */
	RWLOCK(&dlz_implock, isc_rwlocktype_write);

	/*
	 * check that another already registered driver isn't using
	 * the same name
	 */
	dlz_imp = dlz_impfind(drivername);
	if (dlz_imp != NULL) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
			      DNS_LOGMODULE_DLZ, ISC_LOG_DEBUG(2),
			      "DLZ Driver '%s' already registered",
			      drivername);
		RWUNLOCK(&dlz_implock, isc_rwlocktype_write);
		return (ISC_R_EXISTS);
	}

	/*
	 * Allocate memory for a dlz_implementation object.  Error if
	 * we cannot.
	 */
	dlz_imp = isc_mem_get(mctx, sizeof(dns_dlzimplementation_t));
	if (dlz_imp == NULL) {
		RWUNLOCK(&dlz_implock, isc_rwlocktype_write);
		return (ISC_R_NOMEMORY);
	}

	/* Make sure memory region is set to all 0's */
	memset(dlz_imp, 0, sizeof(dns_dlzimplementation_t));

	/* Store the data passed into this method */
	dlz_imp->name = drivername;
	dlz_imp->methods = methods;
	dlz_imp->mctx = NULL;
	dlz_imp->driverarg = driverarg;

	/* attach the new dlz_implementation object to a memory context */
	isc_mem_attach(mctx, &dlz_imp->mctx);

	/*
	 * prepare the dlz_implementation object to be put in a list,
	 * and append it to the list
	 */
	ISC_LINK_INIT(dlz_imp, link);
	ISC_LIST_APPEND(dlz_implementations, dlz_imp, link);

	/* Unlock the dlz_implementations list.	 */
	RWUNLOCK(&dlz_implock, isc_rwlocktype_write);

	/* Pass back the dlz_implementation that we created. */
	*dlzimp = dlz_imp;

	return (ISC_R_SUCCESS);
}

/*%
 * Helper function for dns_dlzstrtoargv().
 * Pardon the gratuitous recursion.
 */
static isc_result_t
dns_dlzstrtoargvsub(isc_mem_t *mctx, char *s, unsigned int *argcp,
		    char ***argvp, unsigned int n)
{
	isc_result_t result;

 restart:
	/* Discard leading whitespace. */
	while (*s == ' ' || *s == '\t')
		s++;

	if (*s == '\0') {
		/* We have reached the end of the string. */
		*argcp = n;
		*argvp = isc_mem_get(mctx, n * sizeof(char *));
		if (*argvp == NULL)
			return (ISC_R_NOMEMORY);
	} else {
		char *p = s;
		while (*p != ' ' && *p != '\t' && *p != '\0' && *p != '{') {
			if (*p == '\n') {
				*p = ' ';
				goto restart;
			}
			p++;
		}

		/* do "grouping", items between { and } are one arg */
		if (*p == '{') {
			char *t = p;
			/*
			 * shift all characters to left by 1 to get rid of '{'
			 */
			while (*t != '\0') {
				t++;
				*(t-1) = *t;
			}
			while (*p != '\0' && *p != '}') {
				p++;
			}
			/* get rid of '}' character */
			if (*p == '}') {
				*p = '\0';
				p++;
			}
			/* normal case, no "grouping" */
		} else if (*p != '\0')
			*p++ = '\0';

		result = dns_dlzstrtoargvsub(mctx, p, argcp, argvp, n + 1);
		if (result != ISC_R_SUCCESS)
			return (result);
		(*argvp)[n] = s;
	}
	return (ISC_R_SUCCESS);
}

/*%
 * Tokenize the string "s" into whitespace-separated words,
 * return the number of words in '*argcp' and an array
 * of pointers to the words in '*argvp'.  The caller
 * must free the array using isc_mem_put().  The string
 * is modified in-place.
 */
isc_result_t
dns_dlzstrtoargv(isc_mem_t *mctx, char *s,
		 unsigned int *argcp, char ***argvp)
{
	return(dns_dlzstrtoargvsub(mctx, s, argcp, argvp, 0));
}

/*%
 * Unregisters a DLZ driver.  This basically just removes the dlz
 * driver from the list of available drivers in the dlz_implementations list.
 */
void
dns_dlzunregister(dns_dlzimplementation_t **dlzimp) {
	dns_dlzimplementation_t *dlz_imp;
	isc_mem_t *mctx;

	/* Write debugging message to log */
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
		      DNS_LOGMODULE_DLZ, ISC_LOG_DEBUG(2),
		      "Unregistering DLZ driver.");

	/*
	 * Performs checks to make sure data is as we expect it to be.
	 */
	REQUIRE(dlzimp != NULL && *dlzimp != NULL);

	/*
	 * initialize the dlz_implementations list, this is guaranteed
	 * to only really happen once.
	 */
	RUNTIME_CHECK(isc_once_do(&once, dlz_initialize) == ISC_R_SUCCESS);

	dlz_imp = *dlzimp;

	/* lock the dlz_implementations list so we can modify it. */
	RWLOCK(&dlz_implock, isc_rwlocktype_write);

	/* remove the dlz_implementation object from the list */
	ISC_LIST_UNLINK(dlz_implementations, dlz_imp, link);
	mctx = dlz_imp->mctx;

	/*
	 * return the memory back to the available memory pool and
	 * remove it from the memory context.
	 */
	isc_mem_put(mctx, dlz_imp, sizeof(dns_dlzimplementation_t));
	isc_mem_detach(&mctx);

	/* Unlock the dlz_implementations list. */
	RWUNLOCK(&dlz_implock, isc_rwlocktype_write);
}
