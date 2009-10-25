/*
 * Copyright (C) 2004-2009  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: journal.h,v 1.33.120.2 2009/01/18 23:47:41 tbox Exp $ */

#ifndef DNS_JOURNAL_H
#define DNS_JOURNAL_H 1

/*****
 ***** Module Info
 *****/

/*! \file dns/journal.h
 * \brief
 * Database journaling.
 */

/***
 *** Imports
 ***/

#include <isc/lang.h>
#include <isc/magic.h>

#include <dns/name.h>
#include <dns/diff.h>
#include <dns/rdata.h>
#include <dns/types.h>

/***
 *** Defines.
 ***/
#define DNS_JOURNALOPT_RESIGN	0x00000001

/***
 *** Types
 ***/

/*%
 * A dns_journal_t represents an open journal file.  This is an opaque type.
 *
 * A particular dns_journal_t object may be opened for writing, in which case
 * it can be used for writing transactions to a journal file, or it can be
 * opened for reading, in which case it can be used for reading transactions
 * from (iterating over) a journal file.  A single dns_journal_t object may
 * not be used for both purposes.
 */
typedef struct dns_journal dns_journal_t;


/***
 *** Functions
 ***/

ISC_LANG_BEGINDECLS

/**************************************************************************/

isc_result_t
dns_db_createsoatuple(dns_db_t *db, dns_dbversion_t *ver, isc_mem_t *mctx,
		   dns_diffop_t op, dns_difftuple_t **tp);
/*!< brief
 * Create a diff tuple for the current database SOA.
 * XXX this probably belongs somewhere else.
 */


/*@{*/
#define DNS_SERIAL_GT(a, b) ((int)(((a) - (b)) & 0xFFFFFFFF) > 0)
#define DNS_SERIAL_GE(a, b) ((int)(((a) - (b)) & 0xFFFFFFFF) >= 0)
/*!< brief
 * Compare SOA serial numbers.  DNS_SERIAL_GT(a, b) returns true iff
 * a is "greater than" b where "greater than" is as defined in RFC1982.
 * DNS_SERIAL_GE(a, b) returns true iff a is "greater than or equal to" b.
 */
/*@}*/

/**************************************************************************/
/*
 * Journal object creation and destruction.
 */

isc_result_t
dns_journal_open(isc_mem_t *mctx, const char *filename, isc_boolean_t write,
		 dns_journal_t **journalp);
/*%<
 * Open the journal file 'filename' and create a dns_journal_t object for it.
 *
 * If 'write' is ISC_TRUE, the journal is open for writing.  If it does
 * not exist, it is created.
 *
 * If 'write' is ISC_FALSE, the journal is open for reading.  If it does
 * not exist, ISC_R_NOTFOUND is returned.
 */

void
dns_journal_destroy(dns_journal_t **journalp);
/*%<
 * Destroy a dns_journal_t, closing any open files and freeing its memory.
 */

/**************************************************************************/
/*
 * Writing transactions to journals.
 */

isc_result_t
dns_journal_begin_transaction(dns_journal_t *j);
/*%<
 * Prepare to write a new transaction to the open journal file 'j'.
 *
 * Requires:
 *     \li 'j' is open for writing.
 */

isc_result_t
dns_journal_writediff(dns_journal_t *j, dns_diff_t *diff);
/*%<
 * Write 'diff' to the current transaction of journal file 'j'.
 *
 * Requires:
 * \li     'j' is open for writing and dns_journal_begin_transaction()
 * 	has been called.
 *
 *\li 	'diff' is a full or partial, correctly ordered IXFR
 *      difference sequence.
 */

isc_result_t
dns_journal_commit(dns_journal_t *j);
/*%<
 * Commit the current transaction of journal file 'j'.
 *
 * Requires:
 * \li     'j' is open for writing and dns_journal_begin_transaction()
 * 	has been called.
 *
 *   \li   dns_journal_writediff() has been called one or more times
 * 	to form a complete, correctly ordered IXFR difference
 *      sequence.
 */

isc_result_t
dns_journal_write_transaction(dns_journal_t *j, dns_diff_t *diff);
/*%
 * Write a complete transaction at once to a journal file,
 * sorting it if necessary, and commit it.  Equivalent to calling
 * dns_diff_sort(), dns_journal_begin_transaction(),
 * dns_journal_writediff(), and dns_journal_commit().
 *
 * Requires:
 *\li      'j' is open for writing.
 *
 * \li	'diff' contains exactly one SOA deletion, one SOA addition
 *       with a greater serial number, and possibly other changes,
 *       in arbitrary order.
 */

/**************************************************************************/
/*
 * Reading transactions from journals.
 */

isc_uint32_t
dns_journal_first_serial(dns_journal_t *j);
isc_uint32_t
dns_journal_last_serial(dns_journal_t *j);
/*%<
 * Get the first and last addressable serial number in the journal.
 */

isc_result_t
dns_journal_iter_init(dns_journal_t *j,
		      isc_uint32_t begin_serial, isc_uint32_t end_serial);
/*%<
 * Prepare to iterate over the transactions that will bring the database
 * from SOA serial number 'begin_serial' to 'end_serial'.
 *
 * Returns:
 *\li	ISC_R_SUCCESS
 *\li	ISC_R_RANGE	begin_serial is outside the addressable range.
 *\li	ISC_R_NOTFOUND	begin_serial is within the range of addressable
 *			serial numbers covered by the journal, but
 *			this particular serial number does not exist.
 */

/*@{*/
isc_result_t
dns_journal_first_rr(dns_journal_t *j);
isc_result_t
dns_journal_next_rr(dns_journal_t *j);
/*%<
 * Position the iterator at the first/next RR in a journal
 * transaction sequence established using dns_journal_iter_init().
 *
 * Requires:
 *    \li  dns_journal_iter_init() has been called.
 *
 */
/*@}*/

void
dns_journal_current_rr(dns_journal_t *j, dns_name_t **name, isc_uint32_t *ttl,
		       dns_rdata_t **rdata);
/*%<
 * Get the name, ttl, and rdata of the current journal RR.
 *
 * Requires:
 * \li     The last call to dns_journal_first_rr() or dns_journal_next_rr()
 *      returned ISC_R_SUCCESS.
 */

/**************************************************************************/
/*
 * Database roll-forward.
 */

isc_result_t
dns_journal_rollforward(isc_mem_t *mctx, dns_db_t *db, unsigned int options,
			const char *filename);
/*%<
 * Roll forward (play back) the journal file "filename" into the
 * database "db".  This should be called when the server starts
 * after a shutdown or crash.
 *
 * Requires:
 *\li   'mctx' is a valid memory context.
 *\li	'db' is a valid database which does not have a version
 *           open for writing.
 *\li   'filename' is the name of the journal file belonging to 'db'.
 *
 * Returns:
 *\li	DNS_R_NOJOURNAL when journal does not exist.
 *\li	ISC_R_NOTFOUND when current serial in not in journal.
 *\li	ISC_R_RANGE when current serial in not in journals range.
 *\li	ISC_R_SUCCESS journal has been applied successfully to database.
 *	others
 */

isc_result_t
dns_journal_print(isc_mem_t *mctx, const char *filename, FILE *file);
/* For debugging not general use */

isc_result_t
dns_db_diff(isc_mem_t *mctx,
	    dns_db_t *dba, dns_dbversion_t *dbvera,
	    dns_db_t *dbb, dns_dbversion_t *dbverb,
	    const char *journal_filename);
/*%<
 * Compare the databases 'dba' and 'dbb' and generate a journal
 * entry containing the changes to make 'dba' from 'dbb' (note
 * the order).  This journal entry will consist of a single,
 * possibly very large transaction.  Append the journal
 * entry to the journal file specified by 'journal_filename'.
 */

isc_result_t
dns_journal_compact(isc_mem_t *mctx, char *filename, isc_uint32_t serial,
		    isc_uint32_t target_size);
/*%<
 * Attempt to compact the journal if it is greater that 'target_size'.
 * Changes from 'serial' onwards will be preserved.  If the journal
 * exists and is non-empty 'serial' must exist in the journal.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_JOURNAL_H */
