/*
 * Copyright (C) 2004-2009  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: db.h,v 1.93.50.3 2009/01/18 23:25:17 marka Exp $ */

#ifndef DNS_DB_H
#define DNS_DB_H 1

/*****
 ***** Module Info
 *****/

/*! \file dns/db.h
 * \brief
 * The DNS DB interface allows named rdatasets to be stored and retrieved.
 *
 * The dns_db_t type is like a "virtual class".  To actually use
 * DBs, an implementation of the class is required.
 *
 * XXX more XXX
 *
 * MP:
 * \li	The module ensures appropriate synchronization of data structures it
 *	creates and manipulates.
 *
 * Reliability:
 * \li	No anticipated impact.
 *
 * Resources:
 * \li	TBS
 *
 * Security:
 * \li	No anticipated impact.
 *
 * Standards:
 * \li	None.
 */

/*****
 ***** Imports
 *****/

#include <isc/lang.h>
#include <isc/magic.h>
#include <isc/ondestroy.h>
#include <isc/stdtime.h>

#include <dns/name.h>
#include <dns/types.h>

ISC_LANG_BEGINDECLS

/*****
 ***** Types
 *****/

typedef struct dns_dbmethods {
	void		(*attach)(dns_db_t *source, dns_db_t **targetp);
	void		(*detach)(dns_db_t **dbp);
	isc_result_t	(*beginload)(dns_db_t *db, dns_addrdatasetfunc_t *addp,
				     dns_dbload_t **dbloadp);
	isc_result_t	(*endload)(dns_db_t *db, dns_dbload_t **dbloadp);
	isc_result_t	(*dump)(dns_db_t *db, dns_dbversion_t *version,
				const char *filename,
				dns_masterformat_t masterformat);
	void		(*currentversion)(dns_db_t *db,
					  dns_dbversion_t **versionp);
	isc_result_t	(*newversion)(dns_db_t *db,
				      dns_dbversion_t **versionp);
	void		(*attachversion)(dns_db_t *db, dns_dbversion_t *source,
					 dns_dbversion_t **targetp);
	void		(*closeversion)(dns_db_t *db,
					dns_dbversion_t **versionp,
					isc_boolean_t commit);
	isc_result_t	(*findnode)(dns_db_t *db, dns_name_t *name,
				    isc_boolean_t create,
				    dns_dbnode_t **nodep);
	isc_result_t	(*find)(dns_db_t *db, dns_name_t *name,
				dns_dbversion_t *version,
				dns_rdatatype_t type, unsigned int options,
				isc_stdtime_t now,
				dns_dbnode_t **nodep, dns_name_t *foundname,
				dns_rdataset_t *rdataset,
				dns_rdataset_t *sigrdataset);
	isc_result_t	(*findzonecut)(dns_db_t *db, dns_name_t *name,
				       unsigned int options, isc_stdtime_t now,
				       dns_dbnode_t **nodep,
				       dns_name_t *foundname,
				       dns_rdataset_t *rdataset,
				       dns_rdataset_t *sigrdataset);
	void		(*attachnode)(dns_db_t *db,
				      dns_dbnode_t *source,
				      dns_dbnode_t **targetp);
	void		(*detachnode)(dns_db_t *db,
				      dns_dbnode_t **targetp);
	isc_result_t	(*expirenode)(dns_db_t *db, dns_dbnode_t *node,
				      isc_stdtime_t now);
	void		(*printnode)(dns_db_t *db, dns_dbnode_t *node,
				     FILE *out);
	isc_result_t 	(*createiterator)(dns_db_t *db, unsigned int options,
					  dns_dbiterator_t **iteratorp);
	isc_result_t	(*findrdataset)(dns_db_t *db, dns_dbnode_t *node,
					dns_dbversion_t *version,
					dns_rdatatype_t type,
					dns_rdatatype_t covers,
					isc_stdtime_t now,
					dns_rdataset_t *rdataset,
					dns_rdataset_t *sigrdataset);
	isc_result_t	(*allrdatasets)(dns_db_t *db, dns_dbnode_t *node,
					dns_dbversion_t *version,
					isc_stdtime_t now,
					dns_rdatasetiter_t **iteratorp);
	isc_result_t	(*addrdataset)(dns_db_t *db, dns_dbnode_t *node,
				       dns_dbversion_t *version,
				       isc_stdtime_t now,
				       dns_rdataset_t *rdataset,
				       unsigned int options,
				       dns_rdataset_t *addedrdataset);
	isc_result_t	(*subtractrdataset)(dns_db_t *db, dns_dbnode_t *node,
					    dns_dbversion_t *version,
					    dns_rdataset_t *rdataset,
					    unsigned int options,
					    dns_rdataset_t *newrdataset);
	isc_result_t	(*deleterdataset)(dns_db_t *db, dns_dbnode_t *node,
					  dns_dbversion_t *version,
					  dns_rdatatype_t type,
					  dns_rdatatype_t covers);
	isc_boolean_t	(*issecure)(dns_db_t *db);
	unsigned int	(*nodecount)(dns_db_t *db);
	isc_boolean_t	(*ispersistent)(dns_db_t *db);
	void		(*overmem)(dns_db_t *db, isc_boolean_t overmem);
	void		(*settask)(dns_db_t *db, isc_task_t *);
	isc_result_t	(*getoriginnode)(dns_db_t *db, dns_dbnode_t **nodep);
	void		(*transfernode)(dns_db_t *db, dns_dbnode_t **sourcep,
					dns_dbnode_t **targetp);
	isc_result_t    (*getnsec3parameters)(dns_db_t *db,
					      dns_dbversion_t *version,
					      dns_hash_t *hash,
					      isc_uint8_t *flags,
					      isc_uint16_t *iterations,
					      unsigned char *salt,
					      size_t *salt_len);
	isc_result_t    (*findnsec3node)(dns_db_t *db, dns_name_t *name,
					 isc_boolean_t create,
					 dns_dbnode_t **nodep);
	isc_result_t	(*setsigningtime)(dns_db_t *db,
					  dns_rdataset_t *rdataset,
					  isc_stdtime_t resign);
	isc_result_t	(*getsigningtime)(dns_db_t *db,
					  dns_rdataset_t *rdataset,
					  dns_name_t *name);
	void		(*resigned)(dns_db_t *db, dns_rdataset_t *rdataset,
					   dns_dbversion_t *version);
	isc_boolean_t	(*isdnssec)(dns_db_t *db);
	dns_stats_t	*(*getrrsetstats)(dns_db_t *db);
} dns_dbmethods_t;

typedef isc_result_t
(*dns_dbcreatefunc_t)(isc_mem_t *mctx, dns_name_t *name,
		      dns_dbtype_t type, dns_rdataclass_t rdclass,
		      unsigned int argc, char *argv[], void *driverarg,
		      dns_db_t **dbp);

#define DNS_DB_MAGIC		ISC_MAGIC('D','N','S','D')
#define DNS_DB_VALID(db)	ISC_MAGIC_VALID(db, DNS_DB_MAGIC)

/*%
 * This structure is actually just the common prefix of a DNS db
 * implementation's version of a dns_db_t.
 * \brief
 * Direct use of this structure by clients is forbidden.  DB implementations
 * may change the structure.  'magic' must be DNS_DB_MAGIC for any of the
 * dns_db_ routines to work.  DB implementations must maintain all DB
 * invariants.
 */
struct dns_db {
	unsigned int			magic;
	unsigned int			impmagic;
	dns_dbmethods_t *		methods;
	isc_uint16_t			attributes;
	dns_rdataclass_t		rdclass;
	dns_name_t			origin;
	isc_ondestroy_t			ondest;
	isc_mem_t *			mctx;
};

#define DNS_DBATTR_CACHE		0x01
#define DNS_DBATTR_STUB			0x02

/*@{*/
/*%
 * Options that can be specified for dns_db_find().
 */
#define DNS_DBFIND_GLUEOK		0x01
#define DNS_DBFIND_VALIDATEGLUE		0x02
#define DNS_DBFIND_NOWILD		0x04
#define DNS_DBFIND_PENDINGOK		0x08
#define DNS_DBFIND_NOEXACT		0x10
#define DNS_DBFIND_FORCENSEC		0x20
#define DNS_DBFIND_COVERINGNSEC		0x40
#define DNS_DBFIND_FORCENSEC3		0x80
/*@}*/

/*@{*/
/*%
 * Options that can be specified for dns_db_addrdataset().
 */
#define DNS_DBADD_MERGE			0x01
#define DNS_DBADD_FORCE			0x02
#define DNS_DBADD_EXACT			0x04
#define DNS_DBADD_EXACTTTL		0x08
/*@}*/

/*%
 * Options that can be specified for dns_db_subtractrdataset().
 */
#define DNS_DBSUB_EXACT			0x01

/*@{*/
/*%
 * Iterator options
 */
#define DNS_DB_RELATIVENAMES	0x1
#define DNS_DB_NSEC3ONLY	0x2
#define DNS_DB_NONSEC3		0x4
/*@}*/

/*****
 ***** Methods
 *****/

/***
 *** Basic DB Methods
 ***/

isc_result_t
dns_db_create(isc_mem_t *mctx, const char *db_type, dns_name_t *origin,
	      dns_dbtype_t type, dns_rdataclass_t rdclass,
	      unsigned int argc, char *argv[], dns_db_t **dbp);
/*%<
 * Create a new database using implementation 'db_type'.
 *
 * Notes:
 * \li	All names in the database must be subdomains of 'origin' and in class
 *	'rdclass'.  The database makes its own copy of the origin, so the
 *	caller may do whatever they like with 'origin' and its storage once the
 *	call returns.
 *
 * \li	DB implementation-specific parameters are passed using argc and argv.
 *
 * Requires:
 *
 * \li	dbp != NULL and *dbp == NULL
 *
 * \li	'origin' is a valid absolute domain name.
 *
 * \li	mctx is a valid memory context
 *
 * Ensures:
 *
 * \li	A copy of 'origin' has been made for the databases use, and the
 *	caller is free to do whatever they want with the name and storage
 *	associated with 'origin'.
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOMEMORY
 * \li	#ISC_R_NOTFOUND				db_type not found
 *
 * \li	Many other errors are possible, depending on what db_type was
 *	specified.
 */

void
dns_db_attach(dns_db_t *source, dns_db_t **targetp);
/*%<
 * Attach *targetp to source.
 *
 * Requires:
 *
 * \li	'source' is a valid database.
 *
 * \li	'targetp' points to a NULL dns_db_t *.
 *
 * Ensures:
 *
 * \li	*targetp is attached to source.
 */

void
dns_db_detach(dns_db_t **dbp);
/*%<
 * Detach *dbp from its database.
 *
 * Requires:
 *
 * \li	'dbp' points to a valid database.
 *
 * Ensures:
 *
 * \li	*dbp is NULL.
 *
 * \li	If '*dbp' is the last reference to the database,
 *		all resources used by the database will be freed
 */

isc_result_t
dns_db_ondestroy(dns_db_t *db, isc_task_t *task, isc_event_t **eventp);
/*%<
 * Causes 'eventp' to be sent to be sent to 'task' when the database is
 * destroyed.
 *
 * Note; ownership of the eventp is taken from the caller (and *eventp is
 * set to NULL). The sender field of the event is set to 'db' before it is
 * sent to the task.
 */

isc_boolean_t
dns_db_iscache(dns_db_t *db);
/*%<
 * Does 'db' have cache semantics?
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * Returns:
 * \li	#ISC_TRUE	'db' has cache semantics
 * \li	#ISC_FALSE	otherwise
 */

isc_boolean_t
dns_db_iszone(dns_db_t *db);
/*%<
 * Does 'db' have zone semantics?
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * Returns:
 * \li	#ISC_TRUE	'db' has zone semantics
 * \li	#ISC_FALSE	otherwise
 */

isc_boolean_t
dns_db_isstub(dns_db_t *db);
/*%<
 * Does 'db' have stub semantics?
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * Returns:
 * \li	#ISC_TRUE	'db' has zone semantics
 * \li	#ISC_FALSE	otherwise
 */

isc_boolean_t
dns_db_issecure(dns_db_t *db);
/*%<
 * Is 'db' secure?
 *
 * Requires:
 *
 * \li	'db' is a valid database with zone semantics.
 *
 * Returns:
 * \li	#ISC_TRUE	'db' is secure.
 * \li	#ISC_FALSE	'db' is not secure.
 */

isc_boolean_t
dns_db_isdnssec(dns_db_t *db);
/*%<
 * Is 'db' secure or partially secure?
 *
 * Requires:
 *
 * \li	'db' is a valid database with zone semantics.
 *
 * Returns:
 * \li	#ISC_TRUE	'db' is secure or is partially.
 * \li	#ISC_FALSE	'db' is not secure.
 */

dns_name_t *
dns_db_origin(dns_db_t *db);
/*%<
 * The origin of the database.
 *
 * Note: caller must not try to change this name.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * Returns:
 *
 * \li	The origin of the database.
 */

dns_rdataclass_t
dns_db_class(dns_db_t *db);
/*%<
 * The class of the database.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * Returns:
 *
 * \li	The class of the database.
 */

isc_result_t
dns_db_beginload(dns_db_t *db, dns_addrdatasetfunc_t *addp,
		 dns_dbload_t **dbloadp);
/*%<
 * Begin loading 'db'.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	This is the first attempt to load 'db'.
 *
 * \li	addp != NULL && *addp == NULL
 *
 * \li	dbloadp != NULL && *dbloadp == NULL
 *
 * Ensures:
 *
 * \li	On success, *addp will be a valid dns_addrdatasetfunc_t suitable
 *	for loading 'db'.  *dbloadp will be a valid DB load context which
 *	should be used as 'arg' when *addp is called.
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOMEMORY
 *
 * \li	Other results are possible, depending upon the database
 *	implementation used, syntax errors in the master file, etc.
 */

isc_result_t
dns_db_endload(dns_db_t *db, dns_dbload_t **dbloadp);
/*%<
 * Finish loading 'db'.
 *
 * Requires:
 *
 * \li	'db' is a valid database that is being loaded.
 *
 * \li	dbloadp != NULL and *dbloadp is a valid database load context.
 *
 * Ensures:
 *
 * \li	*dbloadp == NULL
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOMEMORY
 *
 * \li	Other results are possible, depending upon the database
 *	implementation used, syntax errors in the master file, etc.
 */

isc_result_t
dns_db_load(dns_db_t *db, const char *filename);

isc_result_t
dns_db_load2(dns_db_t *db, const char *filename, dns_masterformat_t format);
/*%<
 * Load master file 'filename' into 'db'.
 *
 * Notes:
 * \li	This routine is equivalent to calling
 *
 *\code
 *		dns_db_beginload();
 *		dns_master_loadfile();
 *		dns_db_endload();
 *\endcode
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	This is the first attempt to load 'db'.
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOMEMORY
 *
 * \li	Other results are possible, depending upon the database
 *	implementation used, syntax errors in the master file, etc.
 */

isc_result_t
dns_db_dump(dns_db_t *db, dns_dbversion_t *version, const char *filename);

isc_result_t
dns_db_dump2(dns_db_t *db, dns_dbversion_t *version, const char *filename,
	     dns_masterformat_t masterformat);
/*%<
 * Dump version 'version' of 'db' to master file 'filename'.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	'version' is a valid version.
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOMEMORY
 *
 * \li	Other results are possible, depending upon the database
 *	implementation used, OS file errors, etc.
 */

/***
 *** Version Methods
 ***/

void
dns_db_currentversion(dns_db_t *db, dns_dbversion_t **versionp);
/*%<
 * Open the current version for reading.
 *
 * Requires:
 *
 * \li	'db' is a valid database with zone semantics.
 *
 * \li	versionp != NULL && *verisonp == NULL
 *
 * Ensures:
 *
 * \li	On success, '*versionp' is attached to the current version.
 *
 */

isc_result_t
dns_db_newversion(dns_db_t *db, dns_dbversion_t **versionp);
/*%<
 * Open a new version for reading and writing.
 *
 * Requires:
 *
 * \li	'db' is a valid database with zone semantics.
 *
 * \li	versionp != NULL && *verisonp == NULL
 *
 * Ensures:
 *
 * \li	On success, '*versionp' is attached to the current version.
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOMEMORY
 *
 * \li	Other results are possible, depending upon the database
 *	implementation used.
 */

void
dns_db_attachversion(dns_db_t *db, dns_dbversion_t *source,
		     dns_dbversion_t **targetp);
/*%<
 * Attach '*targetp' to 'source'.
 *
 * Requires:
 *
 * \li	'db' is a valid database with zone semantics.
 *
 * \li	source is a valid open version
 *
 * \li	targetp != NULL && *targetp == NULL
 *
 * Ensures:
 *
 * \li	'*targetp' is attached to source.
 */

void
dns_db_closeversion(dns_db_t *db, dns_dbversion_t **versionp,
		    isc_boolean_t commit);
/*%<
 * Close version '*versionp'.
 *
 * Note: if '*versionp' is a read-write version and 'commit' is ISC_TRUE,
 * then all changes made in the version will take effect, otherwise they
 * will be rolled back.  The value if 'commit' is ignored for read-only
 * versions.
 *
 * Requires:
 *
 * \li	'db' is a valid database with zone semantics.
 *
 * \li	'*versionp' refers to a valid version.
 *
 * \li	If committing a writable version, then there must be no other
 *	outstanding references to the version (e.g. an active rdataset
 *	iterator).
 *
 * Ensures:
 *
 * \li	*versionp == NULL
 *
 * \li	If *versionp is a read-write version, and commit is ISC_TRUE, then
 *	the version will become the current version.  If !commit, then all
 *	changes made in the version will be undone, and the version will
 *	not become the current version.
 */

/***
 *** Node Methods
 ***/

isc_result_t
dns_db_findnode(dns_db_t *db, dns_name_t *name, isc_boolean_t create,
		dns_dbnode_t **nodep);
/*%<
 * Find the node with name 'name'.
 *
 * Notes:
 * \li	If 'create' is ISC_TRUE and no node with name 'name' exists, then
 *	such a node will be created.
 *
 * \li	This routine is for finding or creating a node with the specified
 *	name.  There are no partial matches.  It is not suitable for use
 *	in building responses to ordinary DNS queries; clients which wish
 *	to do that should use dns_db_find() instead.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	'name' is a valid, non-empty, absolute name.
 *
 * \li	nodep != NULL && *nodep == NULL
 *
 * Ensures:
 *
 * \li	On success, *nodep is attached to the node with name 'name'.
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOTFOUND			If !create and name not found.
 * \li	#ISC_R_NOMEMORY			Can only happen if create is ISC_TRUE.
 *
 * \li	Other results are possible, depending upon the database
 *	implementation used.
 */

isc_result_t
dns_db_find(dns_db_t *db, dns_name_t *name, dns_dbversion_t *version,
	    dns_rdatatype_t type, unsigned int options, isc_stdtime_t now,
	    dns_dbnode_t **nodep, dns_name_t *foundname,
	    dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset);
/*%<
 * Find the best match for 'name' and 'type' in version 'version' of 'db'.
 *
 * Notes:
 *
 * \li	If type == dns_rdataset_any, then rdataset will not be bound.
 *
 * \li	If 'options' does not have #DNS_DBFIND_GLUEOK set, then no glue will
 *	be returned.  For zone databases, glue is as defined in RFC2181.
 *	For cache databases, glue is any rdataset with a trust of
 *	dns_trust_glue.
 *
 * \li	If 'options' does not have #DNS_DBFIND_PENDINGOK set, then no
 *	pending data will be returned.  This option is only meaningful for
 *	cache databases.
 *
 * \li	If the #DNS_DBFIND_NOWILD option is set, then wildcard matching will
 *	be disabled.  This option is only meaningful for zone databases.
 *
 * \li	If the #DNS_DBFIND_FORCENSEC option is set, the database is assumed to
 *	have NSEC records, and these will be returned when appropriate.  This
 *	is only necessary when querying a database that was not secure
 *	when created.
 *
 * \li	If the DNS_DBFIND_COVERINGNSEC option is set, then look for a
 *	NSEC record that potentially covers 'name' if a answer cannot
 *	be found.  Note the returned NSEC needs to be checked to ensure
 *	that it is correct.  This only affects answers returned from the
 *	cache.
 *
 * \li	To respond to a query for SIG records, the caller should create a
 *	rdataset iterator and extract the signatures from each rdataset.
 *
 * \li	Making queries of type ANY with #DNS_DBFIND_GLUEOK is not recommended,
 *	because the burden of determining whether a given rdataset is valid
 *	glue or not falls upon the caller.
 *
 * \li	The 'now' field is ignored if 'db' is a zone database.  If 'db' is a
 *	cache database, an rdataset will not be found unless it expires after
 *	'now'.  Any ANY query will not match unless at least one rdataset at
 *	the node expires after 'now'.  If 'now' is zero, then the current time
 *	will be used.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	'type' is not SIG, or a meta-RR type other than 'ANY' (e.g. 'OPT').
 *
 * \li	'nodep' is NULL, or nodep is a valid pointer and *nodep == NULL.
 *
 * \li	'foundname' is a valid name with a dedicated buffer.
 *
 * \li	'rdataset' is NULL, or is a valid unassociated rdataset.
 *
 * Ensures,
 *	on a non-error completion:
 *
 *	\li	If nodep != NULL, then it is bound to the found node.
 *
 *	\li	If foundname != NULL, then it contains the full name of the
 *		found node.
 *
 *	\li	If rdataset != NULL and type != dns_rdatatype_any, then
 *		rdataset is bound to the found rdataset.
 *
 *	Non-error results are:
 *
 *	\li	#ISC_R_SUCCESS			The desired node and type were
 *						found.
 *
 *	\li	#DNS_R_WILDCARD			The desired node and type were
 *						found after performing
 *						wildcard matching.  This is
 *						only returned if the
 *						#DNS_DBFIND_INDICATEWILD
 *						option is set; otherwise
 *						#ISC_R_SUCCESS is returned.
 *
 *	\li	#DNS_R_GLUE			The desired node and type were
 *						found, but are glue.  This
 *						result can only occur if
 *						the DNS_DBFIND_GLUEOK option
 *						is set.  This result can only
 *						occur if 'db' is a zone
 *						database.  If type ==
 *						dns_rdatatype_any, then the
 *						node returned may contain, or
 *						consist entirely of invalid
 *						glue (i.e. data occluded by a
 *						zone cut).  The caller must
 *						take care not to return invalid
 *						glue to a client.
 *
 *	\li	#DNS_R_DELEGATION		The data requested is beneath
 *						a zone cut.  node, foundname,
 *						and rdataset reference the
 *						NS RRset of the zone cut.
 *						If 'db' is a cache database,
 *						then this is the deepest known
 *						delegation.
 *
 *	\li	#DNS_R_ZONECUT			type == dns_rdatatype_any, and
 *						the desired node is a zonecut.
 *						The caller must take care not
 *						to return inappropriate glue
 *						to a client.  This result can
 *						only occur if 'db' is a zone
 *						database and DNS_DBFIND_GLUEOK
 *						is set.
 *
 *	\li	#DNS_R_DNAME			The data requested is beneath
 *						a DNAME.  node, foundname,
 *						and rdataset reference the
 *						DNAME RRset.
 *
 *	\li	#DNS_R_CNAME			The rdataset requested was not
 *						found, but there is a CNAME
 *						at the desired name.  node,
 *						foundname, and rdataset
 *						reference the CNAME RRset.
 *
 *	\li	#DNS_R_NXDOMAIN			The desired name does not
 *						exist.
 *
 *	\li	#DNS_R_NXRRSET			The desired name exists, but
 *						the desired type does not.
 *
 *	\li	#ISC_R_NOTFOUND			The desired name does not
 *						exist, and no delegation could
 *						be found.  This result can only
 *						occur if 'db' is a cache
 *						database.  The caller should
 *						use its nameserver(s) of last
 *						resort (e.g. root hints).
 *
 *	\li	#DNS_R_NCACHENXDOMAIN		The desired name does not
 *						exist.  'node' is bound to the
 *						cache node with the desired
 *						name, and 'rdataset' contains
 *						the negative caching proof.
 *
 *	\li	#DNS_R_NCACHENXRRSET		The desired type does not
 *						exist.  'node' is bound to the
 *						cache node with the desired
 *						name, and 'rdataset' contains
 *						the negative caching proof.
 *
 *	\li	#DNS_R_EMPTYNAME		The name exists but there is
 *						no data at the name.
 *
 *	\li	#DNS_R_COVERINGNSEC		The returned data is a NSEC
 *						that potentially covers 'name'.
 *
 *	Error results:
 *
 *	\li	#ISC_R_NOMEMORY
 *
 *	\li	#DNS_R_BADDB			Data that is required to be
 *						present in the DB, e.g. an NSEC
 *						record in a secure zone, is not
 *						present.
 *
 *	\li	Other results are possible, and should all be treated as
 *		errors.
 */

isc_result_t
dns_db_findzonecut(dns_db_t *db, dns_name_t *name,
		   unsigned int options, isc_stdtime_t now,
		   dns_dbnode_t **nodep, dns_name_t *foundname,
		   dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset);
/*%<
 * Find the deepest known zonecut which encloses 'name' in 'db'.
 *
 * Notes:
 *
 * \li	If the #DNS_DBFIND_NOEXACT option is set, then the zonecut returned
 *	(if any) will be the deepest known ancestor of 'name'.
 *
 * \li	If 'now' is zero, then the current time will be used.
 *
 * Requires:
 *
 * \li	'db' is a valid database with cache semantics.
 *
 * \li	'nodep' is NULL, or nodep is a valid pointer and *nodep == NULL.
 *
 * \li	'foundname' is a valid name with a dedicated buffer.
 *
 * \li	'rdataset' is NULL, or is a valid unassociated rdataset.
 *
 * Ensures, on a non-error completion:
 *
 * \li	If nodep != NULL, then it is bound to the found node.
 *
 * \li	If foundname != NULL, then it contains the full name of the
 *	found node.
 *
 * \li	If rdataset != NULL and type != dns_rdatatype_any, then
 *	rdataset is bound to the found rdataset.
 *
 * Non-error results are:
 *
 * \li	#ISC_R_SUCCESS
 *
 * \li	#ISC_R_NOTFOUND
 *
 * \li	Other results are possible, and should all be treated as
 *	errors.
 */

void
dns_db_attachnode(dns_db_t *db, dns_dbnode_t *source, dns_dbnode_t **targetp);
/*%<
 * Attach *targetp to source.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	'source' is a valid node.
 *
 * \li	'targetp' points to a NULL dns_dbnode_t *.
 *
 * Ensures:
 *
 * \li	*targetp is attached to source.
 */

void
dns_db_detachnode(dns_db_t *db, dns_dbnode_t **nodep);
/*%<
 * Detach *nodep from its node.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	'nodep' points to a valid node.
 *
 * Ensures:
 *
 * \li	*nodep is NULL.
 */

void
dns_db_transfernode(dns_db_t *db, dns_dbnode_t **sourcep,
		    dns_dbnode_t **targetp);
/*%<
 * Transfer a node between pointer.
 *
 * This is equivalent to calling dns_db_attachnode() then dns_db_detachnode().
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	'*sourcep' is a valid node.
 *
 * \li	'targetp' points to a NULL dns_dbnode_t *.
 *
 * Ensures:
 *
 * \li	'*sourcep' is NULL.
 */

isc_result_t
dns_db_expirenode(dns_db_t *db, dns_dbnode_t *node, isc_stdtime_t now);
/*%<
 * Mark as stale all records at 'node' which expire at or before 'now'.
 *
 * Note: if 'now' is zero, then the current time will be used.
 *
 * Requires:
 *
 * \li	'db' is a valid cache database.
 *
 * \li	'node' is a valid node.
 */

void
dns_db_printnode(dns_db_t *db, dns_dbnode_t *node, FILE *out);
/*%<
 * Print a textual representation of the contents of the node to
 * 'out'.
 *
 * Note: this function is intended for debugging, not general use.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	'node' is a valid node.
 */

/***
 *** DB Iterator Creation
 ***/

isc_result_t
dns_db_createiterator(dns_db_t *db, unsigned int options,
		      dns_dbiterator_t **iteratorp);
/*%<
 * Create an iterator for version 'version' of 'db'.
 *
 * Notes:
 *
 * \li	One or more of the following options can be set.
 *	#DNS_DB_RELATIVENAMES
 *	#DNS_DB_NSEC3ONLY
 *	#DNS_DB_NONSEC3
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	iteratorp != NULL && *iteratorp == NULL
 *
 * Ensures:
 *
 * \li	On success, *iteratorp will be a valid database iterator.
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOMEMORY
 */

/***
 *** Rdataset Methods
 ***/

/*
 * XXXRTH  Should we check for glue and pending data in dns_db_findrdataset()?
 */

isc_result_t
dns_db_findrdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
		    dns_rdatatype_t type, dns_rdatatype_t covers,
		    isc_stdtime_t now, dns_rdataset_t *rdataset,
		    dns_rdataset_t *sigrdataset);
/*%<
 * Search for an rdataset of type 'type' at 'node' that are in version
 * 'version' of 'db'.  If found, make 'rdataset' refer to it.
 *
 * Notes:
 *
 * \li	If 'version' is NULL, then the current version will be used.
 *
 * \li	Care must be used when using this routine to build a DNS response:
 *	'node' should have been found with dns_db_find(), not
 *	dns_db_findnode().  No glue checking is done.  No checking for
 *	pending data is done.
 *
 * \li	The 'now' field is ignored if 'db' is a zone database.  If 'db' is a
 *	cache database, an rdataset will not be found unless it expires after
 *	'now'.  If 'now' is zero, then the current time will be used.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	'node' is a valid node.
 *
 * \li	'rdataset' is a valid, disassociated rdataset.
 *
 * \li	'sigrdataset' is a valid, disassociated rdataset, or it is NULL.
 *
 * \li	If 'covers' != 0, 'type' must be SIG.
 *
 * \li	'type' is not a meta-RR type such as 'ANY' or 'OPT'.
 *
 * Ensures:
 *
 * \li	On success, 'rdataset' is associated with the found rdataset.
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOTFOUND
 *
 * \li	Other results are possible, depending upon the database
 *	implementation used.
 */

isc_result_t
dns_db_allrdatasets(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
		    isc_stdtime_t now, dns_rdatasetiter_t **iteratorp);
/*%<
 * Make '*iteratorp' an rdataset iterator for all rdatasets at 'node' in
 * version 'version' of 'db'.
 *
 * Notes:
 *
 * \li	If 'version' is NULL, then the current version will be used.
 *
 * \li	The 'now' field is ignored if 'db' is a zone database.  If 'db' is a
 *	cache database, an rdataset will not be found unless it expires after
 *	'now'.  Any ANY query will not match unless at least one rdataset at
 *	the node expires after 'now'.  If 'now' is zero, then the current time
 *	will be used.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	'node' is a valid node.
 *
 * \li	iteratorp != NULL && *iteratorp == NULL
 *
 * Ensures:
 *
 * \li	On success, '*iteratorp' is a valid rdataset iterator.
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOTFOUND
 *
 * \li	Other results are possible, depending upon the database
 *	implementation used.
 */

isc_result_t
dns_db_addrdataset(dns_db_t *db, dns_dbnode_t *node, dns_dbversion_t *version,
		   isc_stdtime_t now, dns_rdataset_t *rdataset,
		   unsigned int options, dns_rdataset_t *addedrdataset);
/*%<
 * Add 'rdataset' to 'node' in version 'version' of 'db'.
 *
 * Notes:
 *
 * \li	If the database has zone semantics, the #DNS_DBADD_MERGE option is set,
 *	and an rdataset of the same type as 'rdataset' already exists at
 *	'node' then the contents of 'rdataset' will be merged with the existing
 *	rdataset.  If the option is not set, then rdataset will replace any
 *	existing rdataset of the same type.  If not merging and the
 *	#DNS_DBADD_FORCE option is set, then the data will update the database
 *	without regard to trust levels.  If not forcing the data, then the
 *	rdataset will only be added if its trust level is >= the trust level of
 *	any existing rdataset.  Forcing is only meaningful for cache databases.
 *	If #DNS_DBADD_EXACT is set then there must be no rdata in common between
 *	the old and new rdata sets.  If #DNS_DBADD_EXACTTTL is set then both
 *	the old and new rdata sets must have the same ttl.
 *
 * \li	The 'now' field is ignored if 'db' is a zone database.  If 'db' is
 *	a cache database, then the added rdataset will expire no later than
 *	now + rdataset->ttl.
 *
 * \li	If 'addedrdataset' is not NULL, then it will be attached to the
 *	resulting new rdataset in the database, or to the existing data if
 *	the existing data was better.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	'node' is a valid node.
 *
 * \li	'rdataset' is a valid, associated rdataset with the same class
 *	as 'db'.
 *
 * \li	'addedrdataset' is NULL, or a valid, unassociated rdataset.
 *
 * \li	The database has zone semantics and 'version' is a valid
 *	read-write version, or the database has cache semantics
 *	and version is NULL.
 *
 * \li	If the database has cache semantics, the #DNS_DBADD_MERGE option must
 *	not be set.
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#DNS_R_UNCHANGED			The operation did not change anything.
 * \li	#ISC_R_NOMEMORY
 * \li	#DNS_R_NOTEXACT
 *
 * \li	Other results are possible, depending upon the database
 *	implementation used.
 */

isc_result_t
dns_db_subtractrdataset(dns_db_t *db, dns_dbnode_t *node,
			dns_dbversion_t *version, dns_rdataset_t *rdataset,
			unsigned int options, dns_rdataset_t *newrdataset);
/*%<
 * Remove any rdata in 'rdataset' from 'node' in version 'version' of
 * 'db'.
 *
 * Notes:
 *
 * \li	If 'newrdataset' is not NULL, then it will be attached to the
 *	resulting new rdataset in the database, unless the rdataset has
 *	become nonexistent.  If DNS_DBSUB_EXACT is set then all elements
 *	of 'rdataset' must exist at 'node'.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	'node' is a valid node.
 *
 * \li	'rdataset' is a valid, associated rdataset with the same class
 *	as 'db'.
 *
 * \li	'newrdataset' is NULL, or a valid, unassociated rdataset.
 *
 * \li	The database has zone semantics and 'version' is a valid
 *	read-write version.
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#DNS_R_UNCHANGED			The operation did not change anything.
 * \li	#DNS_R_NXRRSET			All rdata of the same type as those
 *					in 'rdataset' have been deleted.
 * \li	#DNS_R_NOTEXACT			Some part of 'rdataset' did not
 *					exist and DNS_DBSUB_EXACT was set.
 *
 * \li	Other results are possible, depending upon the database
 *	implementation used.
 */

isc_result_t
dns_db_deleterdataset(dns_db_t *db, dns_dbnode_t *node,
		      dns_dbversion_t *version, dns_rdatatype_t type,
		      dns_rdatatype_t covers);
/*%<
 * Make it so that no rdataset of type 'type' exists at 'node' in version
 * version 'version' of 'db'.
 *
 * Notes:
 *
 * \li	If 'type' is dns_rdatatype_any, then no rdatasets will exist in
 *	'version' (provided that the dns_db_deleterdataset() isn't followed
 *	by one or more dns_db_addrdataset() calls).
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	'node' is a valid node.
 *
 * \li	The database has zone semantics and 'version' is a valid
 *	read-write version, or the database has cache semantics
 *	and version is NULL.
 *
 * \li	'type' is not a meta-RR type, except for dns_rdatatype_any, which is
 *	allowed.
 *
 * \li	If 'covers' != 0, 'type' must be SIG.
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#DNS_R_UNCHANGED			No rdatasets of 'type' existed before
 *					the operation was attempted.
 *
 * \li	Other results are possible, depending upon the database
 *	implementation used.
 */

isc_result_t
dns_db_getsoaserial(dns_db_t *db, dns_dbversion_t *ver, isc_uint32_t *serialp);
/*%<
 * Get the current SOA serial number from a zone database.
 *
 * Requires:
 * \li	'db' is a valid database with zone semantics.
 * \li	'ver' is a valid version.
 */

void
dns_db_overmem(dns_db_t *db, isc_boolean_t overmem);
/*%<
 * Enable / disable aggressive cache cleaning.
 */

unsigned int
dns_db_nodecount(dns_db_t *db);
/*%<
 * Count the number of nodes in 'db'.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * Returns:
 * \li	The number of nodes in the database
 */

void
dns_db_settask(dns_db_t *db, isc_task_t *task);
/*%<
 * If task is set then the final detach maybe performed asynchronously.
 *
 * Requires:
 * \li	'db' is a valid database.
 * \li	'task' to be valid or NULL.
 */

isc_boolean_t
dns_db_ispersistent(dns_db_t *db);
/*%<
 * Is 'db' persistent?  A persistent database does not need to be loaded
 * from disk or written to disk.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * Returns:
 * \li	#ISC_TRUE	'db' is persistent.
 * \li	#ISC_FALSE	'db' is not persistent.
 */

isc_result_t
dns_db_register(const char *name, dns_dbcreatefunc_t create, void *driverarg,
		isc_mem_t *mctx, dns_dbimplementation_t **dbimp);

/*%<
 * Register a new database implementation and add it to the list of
 * supported implementations.
 *
 * Requires:
 *
 * \li 	'name' is not NULL
 * \li	'order' is a valid function pointer
 * \li	'mctx' is a valid memory context
 * \li	dbimp != NULL && *dbimp == NULL
 *
 * Returns:
 * \li	#ISC_R_SUCCESS	The registration succeeded
 * \li	#ISC_R_NOMEMORY	Out of memory
 * \li	#ISC_R_EXISTS	A database implementation with the same name exists
 *
 * Ensures:
 *
 * \li	*dbimp points to an opaque structure which must be passed to
 *	dns_db_unregister().
 */

void
dns_db_unregister(dns_dbimplementation_t **dbimp);
/*%<
 * Remove a database implementation from the list of supported
 * implementations.  No databases of this type can be active when this
 * is called.
 *
 * Requires:
 * \li 	dbimp != NULL && *dbimp == NULL
 *
 * Ensures:
 *
 * \li	Any memory allocated in *dbimp will be freed.
 */

isc_result_t
dns_db_getoriginnode(dns_db_t *db, dns_dbnode_t **nodep);
/*%<
 * Get the origin DB node corresponding to the DB's zone.  This function
 * should typically succeed unless the underlying DB implementation doesn't
 * support the feature.
 *
 * Requires:
 *
 * \li	'db' is a valid zone database.
 * \li	'nodep' != NULL && '*nodep' == NULL
 *
 * Ensures:
 * \li	On success, '*nodep' will point to the DB node of the zone's origin.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOTFOUND - the DB implementation does not support this feature.
 */

isc_result_t
dns_db_getnsec3parameters(dns_db_t *db, dns_dbversion_t *version,
			  dns_hash_t *hash, isc_uint8_t *flags,
			  isc_uint16_t *interations,
			  unsigned char *salt, size_t *salt_length);
/*%<
 * Get the NSEC3 parameters that are associated with this zone.
 *
 * Requires:
 * \li	'db' is a valid zone database.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOTFOUND - the DB implementation does not support this feature
 *			  or this zone does not have NSEC3 records.
 */

isc_result_t
dns_db_findnsec3node(dns_db_t *db, dns_name_t *name,
		     isc_boolean_t create, dns_dbnode_t **nodep);
/*%<
 * Find the NSEC3 node with name 'name'.
 *
 * Notes:
 * \li	If 'create' is ISC_TRUE and no node with name 'name' exists, then
 *	such a node will be created.
 *
 * Requires:
 *
 * \li	'db' is a valid database.
 *
 * \li	'name' is a valid, non-empty, absolute name.
 *
 * \li	nodep != NULL && *nodep == NULL
 *
 * Ensures:
 *
 * \li	On success, *nodep is attached to the node with name 'name'.
 *
 * Returns:
 *
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOTFOUND			If !create and name not found.
 * \li	#ISC_R_NOMEMORY			Can only happen if create is ISC_TRUE.
 *
 * \li	Other results are possible, depending upon the database
 *	implementation used.
 */

isc_result_t
dns_db_setsigningtime(dns_db_t *db, dns_rdataset_t *rdataset,
		      isc_stdtime_t resign);
/*%<
 * Sets the re-signing time associated with 'rdataset' to 'resign'.
 *
 * Requires:
 * \li	'db' is a valid zone database.
 * \li	'rdataset' to be associated with 'db'.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOMEMORY
 * \li	#ISC_R_NOTIMPLEMENTED - Not supported by this DB implementation.
 */

isc_result_t
dns_db_getsigningtime(dns_db_t *db, dns_rdataset_t *rdataset, dns_name_t *name);
/*%<
 * Return the rdataset with the earliest signing time in the zone.
 * Note: the rdataset is version agnostic.
 *
 * Requires:
 * \li	'db' is a valid zone database.
 * \li	'rdataset' to be initialized but not associated.
 * \li	'name' to be NULL or have a buffer associated with it.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOTFOUND - No dataset exists.
 */

void
dns_db_resigned(dns_db_t *db, dns_rdataset_t *rdataset,
		dns_dbversion_t *version);
/*%<
 * Mark 'rdataset' as not being available to be returned by
 * dns_db_getsigningtime().  If the changes associated with 'version'
 * are committed this will be permanent.  If the version is not committed
 * this change will be rolled back when the version is closed.
 *
 * Requires:
 * \li	'db' is a valid zone database.
 * \li	'rdataset' to be associated with 'db'.
 * \li	'version' to be open for writing.
 */

dns_stats_t *
dns_db_getrrsetstats(dns_db_t *db);
/*%<
 * Get statistics information counting RRsets stored in the DB, when available.
 * The statistics may not be available depending on the DB implementation.
 *
 * Requires:
 *
 * \li	'db' is a valid database (zone or cache).
 *
 * Returns:
 * \li	when available, a pointer to a statistics object created by
 *	dns_rdatasetstats_create(); otherwise NULL.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_DB_H */
