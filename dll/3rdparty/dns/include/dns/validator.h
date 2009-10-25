/*
 * Copyright (C) 2004-2009  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2000-2003  Internet Software Consortium.
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

/* $Id: validator.h,v 1.41.48.3 2009/01/18 23:25:17 marka Exp $ */

#ifndef DNS_VALIDATOR_H
#define DNS_VALIDATOR_H 1

/*****
 ***** Module Info
 *****/

/*! \file dns/validator.h
 *
 * \brief
 * DNS Validator
 * This is the BIND 9 validator, the module responsible for validating the
 * rdatasets and negative responses (messages).  It makes use of zones in
 * the view and may fetch RRset to complete trust chains.  It implements
 * DNSSEC as specified in RFC 4033, 4034 and 4035.
 *
 * It can also optionally implement ISC's DNSSEC look-aside validation.
 *
 * Correct operation is critical to preventing spoofed answers from secure
 * zones being accepted.
 *
 * MP:
 *\li	The module ensures appropriate synchronization of data structures it
 *	creates and manipulates.
 *
 * Reliability:
 *\li	No anticipated impact.
 *
 * Resources:
 *\li	TBS
 *
 * Security:
 *\li	No anticipated impact.
 *
 * Standards:
 *\li	RFCs:	1034, 1035, 2181, 4033, 4034, 4035.
 */

#include <isc/lang.h>
#include <isc/event.h>
#include <isc/mutex.h>

#include <dns/fixedname.h>
#include <dns/types.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h> /* for dns_rdata_rrsig_t */

#include <dst/dst.h>

/*%
 * A dns_validatorevent_t is sent when a 'validation' completes.
 * \brief
 * 'name', 'rdataset', 'sigrdataset', and 'message' are the values that were
 * supplied when dns_validator_create() was called.  They are returned to the
 * caller so that they may be freed.
 *
 * If the RESULT is ISC_R_SUCCESS and the answer is secure then
 * proofs[] will contain the names of the NSEC records that hold the
 * various proofs.  Note the same name may appear multiple times.
 */
typedef struct dns_validatorevent {
	ISC_EVENT_COMMON(struct dns_validatorevent);
	dns_validator_t *		validator;
	isc_result_t			result;
	/*
	 * Name and type of the response to be validated.
	 */
	dns_name_t *			name;
	dns_rdatatype_t			type;
	/*
	 * Rdata and RRSIG (if any) for positive responses.
	 */
	dns_rdataset_t *		rdataset;
	dns_rdataset_t *		sigrdataset;
	/*
	 * The full response.  Required for negative responses.
	 * Also required for positive wildcard responses.
	 */
	dns_message_t *			message;
	/*
	 * Proofs to be cached.
	 */
	dns_name_t *			proofs[4];
	/*
	 * Optout proof seen.
	 */
	isc_boolean_t			optout;
} dns_validatorevent_t;

#define DNS_VALIDATOR_NOQNAMEPROOF 0
#define DNS_VALIDATOR_NODATAPROOF 1
#define DNS_VALIDATOR_NOWILDCARDPROOF 2
#define DNS_VALIDATOR_CLOSESTENCLOSER 3

/*%
 * A validator object represents a validation in progress.
 * \brief
 * Clients are strongly discouraged from using this type directly, with
 * the exception of the 'link' field, which may be used directly for
 * whatever purpose the client desires.
 */
struct dns_validator {
	/* Unlocked. */
	unsigned int			magic;
	isc_mutex_t			lock;
	dns_view_t *			view;
	/* Locked by lock. */
	unsigned int			options;
	unsigned int			attributes;
	dns_validatorevent_t *		event;
	dns_fetch_t *			fetch;
	dns_validator_t *		subvalidator;
	dns_validator_t *		parent;
	dns_keytable_t *		keytable;
	dns_keynode_t *			keynode;
	dst_key_t *			key;
	dns_rdata_rrsig_t *		siginfo;
	isc_task_t *			task;
	isc_taskaction_t		action;
	void *				arg;
	unsigned int			labels;
	dns_rdataset_t *		currentset;
	isc_boolean_t			seensig;
	dns_rdataset_t *		keyset;
	dns_rdataset_t *		dsset;
	dns_rdataset_t *		soaset;
	dns_rdataset_t *		nsecset;
	dns_rdataset_t *		nsec3set;
	dns_name_t *			soaname;
	dns_rdataset_t			frdataset;
	dns_rdataset_t			fsigrdataset;
	dns_fixedname_t			fname;
	dns_fixedname_t			wild;
	dns_fixedname_t			nearest;
	dns_fixedname_t			closest;
	ISC_LINK(dns_validator_t)	link;
	dns_rdataset_t 			dlv;
	dns_fixedname_t			dlvsep;
	isc_boolean_t			havedlvsep;
	isc_boolean_t			mustbesecure;
	unsigned int			dlvlabels;
	unsigned int			depth;
};

/*%
 * dns_validator_create() options.
 */
#define DNS_VALIDATOR_DLV 1U
#define DNS_VALIDATOR_DEFER 2U

ISC_LANG_BEGINDECLS

isc_result_t
dns_validator_create(dns_view_t *view, dns_name_t *name, dns_rdatatype_t type,
		     dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset,
		     dns_message_t *message, unsigned int options,
		     isc_task_t *task, isc_taskaction_t action, void *arg,
		     dns_validator_t **validatorp);
/*%<
 * Start a DNSSEC validation.
 *
 * This validates a response to the question given by
 * 'name' and 'type'.
 *
 * To validate a positive response, the response data is
 * given by 'rdataset' and 'sigrdataset'.  If 'sigrdataset'
 * is NULL, the data is presumed insecure and an attempt
 * is made to prove its insecurity by finding the appropriate
 * null key.
 *
 * The complete response message may be given in 'message',
 * to make available any authority section NSECs that may be
 * needed for validation of a response resulting from a
 * wildcard expansion (though no such wildcard validation
 * is implemented yet).  If the complete response message
 * is not available, 'message' is NULL.
 *
 * To validate a negative response, the complete negative response
 * message is given in 'message'.  The 'rdataset', and
 * 'sigrdataset' arguments must be NULL, but the 'name' and 'type'
 * arguments must be provided.
 *
 * The validation is performed in the context of 'view'.
 *
 * When the validation finishes, a dns_validatorevent_t with
 * the given 'action' and 'arg' are sent to 'task'.
 * Its 'result' field will be ISC_R_SUCCESS iff the
 * response was successfully proven to be either secure or
 * part of a known insecure domain.
 *
 * options:
 * If DNS_VALIDATOR_DLV is set the caller knows there is not a
 * trusted key and the validator should immediately attempt to validate
 * the answer by looking for an appropriate DLV RRset.
 */

void
dns_validator_send(dns_validator_t *validator);
/*%<
 * Send a deferred validation request
 *
 * Requires:
 *	'validator' to points to a valid DNSSEC validator.
 */

void
dns_validator_cancel(dns_validator_t *validator);
/*%<
 * Cancel a DNSSEC validation in progress.
 *
 * Requires:
 *\li	'validator' points to a valid DNSSEC validator, which
 *	may or may not already have completed.
 *
 * Ensures:
 *\li	It the validator has not already sent its completion
 *	event, it will send it with result code ISC_R_CANCELED.
 */

void
dns_validator_destroy(dns_validator_t **validatorp);
/*%<
 * Destroy a DNSSEC validator.
 *
 * Requires:
 *\li	'*validatorp' points to a valid DNSSEC validator.
 * \li	The validator must have completed and sent its completion
 * 	event.
 *
 * Ensures:
 *\li	All resources used by the validator are freed.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_VALIDATOR_H */
