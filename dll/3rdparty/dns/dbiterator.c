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

/* $Id: dbiterator.c,v 1.18 2007/06/19 23:47:16 tbox Exp $ */

/*! \file */

#include <config.h>

#include <isc/util.h>

#include <dns/dbiterator.h>
#include <dns/name.h>

void
dns_dbiterator_destroy(dns_dbiterator_t **iteratorp) {
	/*
	 * Destroy '*iteratorp'.
	 */

	REQUIRE(iteratorp != NULL);
	REQUIRE(DNS_DBITERATOR_VALID(*iteratorp));

	(*iteratorp)->methods->destroy(iteratorp);

	ENSURE(*iteratorp == NULL);
}

isc_result_t
dns_dbiterator_first(dns_dbiterator_t *iterator) {
	/*
	 * Move the node cursor to the first node in the database (if any).
	 */

	REQUIRE(DNS_DBITERATOR_VALID(iterator));

	return (iterator->methods->first(iterator));
}

isc_result_t
dns_dbiterator_last(dns_dbiterator_t *iterator) {
	/*
	 * Move the node cursor to the first node in the database (if any).
	 */

	REQUIRE(DNS_DBITERATOR_VALID(iterator));

	return (iterator->methods->last(iterator));
}

isc_result_t
dns_dbiterator_seek(dns_dbiterator_t *iterator, dns_name_t *name) {
	/*
	 * Move the node cursor to the node with name 'name'.
	 */

	REQUIRE(DNS_DBITERATOR_VALID(iterator));

	return (iterator->methods->seek(iterator, name));
}

isc_result_t
dns_dbiterator_prev(dns_dbiterator_t *iterator) {
	/*
	 * Move the node cursor to the previous node in the database (if any).
	 */

	REQUIRE(DNS_DBITERATOR_VALID(iterator));

	return (iterator->methods->prev(iterator));
}

isc_result_t
dns_dbiterator_next(dns_dbiterator_t *iterator) {
	/*
	 * Move the node cursor to the next node in the database (if any).
	 */

	REQUIRE(DNS_DBITERATOR_VALID(iterator));

	return (iterator->methods->next(iterator));
}

isc_result_t
dns_dbiterator_current(dns_dbiterator_t *iterator, dns_dbnode_t **nodep,
		       dns_name_t *name)
{
	/*
	 * Return the current node.
	 */

	REQUIRE(DNS_DBITERATOR_VALID(iterator));
	REQUIRE(nodep != NULL && *nodep == NULL);
	REQUIRE(name == NULL || dns_name_hasbuffer(name));

	return (iterator->methods->current(iterator, nodep, name));
}

isc_result_t
dns_dbiterator_pause(dns_dbiterator_t *iterator) {
	/*
	 * Pause iteration.
	 */

	REQUIRE(DNS_DBITERATOR_VALID(iterator));

	return (iterator->methods->pause(iterator));
}

isc_result_t
dns_dbiterator_origin(dns_dbiterator_t *iterator, dns_name_t *name) {

	/*
	 * Return the origin to which returned node names are relative.
	 */

	REQUIRE(DNS_DBITERATOR_VALID(iterator));
	REQUIRE(iterator->relative_names);
	REQUIRE(dns_name_hasbuffer(name));

	return (iterator->methods->origin(iterator, name));
}

void
dns_dbiterator_setcleanmode(dns_dbiterator_t *iterator, isc_boolean_t mode) {
	REQUIRE(DNS_DBITERATOR_VALID(iterator));

	iterator->cleaning = mode;
}
