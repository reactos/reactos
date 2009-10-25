/*
 * Copyright (C) 2004-2009  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2002  Internet Software Consortium.
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

/* $Id: rbt.h,v 1.71.48.2 2009/01/18 23:47:41 tbox Exp $ */

#ifndef DNS_RBT_H
#define DNS_RBT_H 1

/*! \file dns/rbt.h */

#include <isc/lang.h>
#include <isc/magic.h>
#include <isc/refcount.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

#define DNS_RBT_USEHASH 1

/*@{*/
/*%
 * Option values for dns_rbt_findnode() and dns_rbt_findname().
 * These are used to form a bitmask.
 */
#define DNS_RBTFIND_NOOPTIONS                   0x00
#define DNS_RBTFIND_EMPTYDATA                   0x01
#define DNS_RBTFIND_NOEXACT                     0x02
#define DNS_RBTFIND_NOPREDECESSOR               0x04
/*@}*/

#ifndef DNS_RBT_USEISCREFCOUNT
#ifdef ISC_REFCOUNT_HAVEATOMIC
#define DNS_RBT_USEISCREFCOUNT 1
#endif
#endif

/*
 * These should add up to 30.
 */
#define DNS_RBT_LOCKLENGTH                      10
#define DNS_RBT_REFLENGTH                       20

#define DNS_RBTNODE_MAGIC               ISC_MAGIC('R','B','N','O')
#if DNS_RBT_USEMAGIC
#define DNS_RBTNODE_VALID(n)            ISC_MAGIC_VALID(n, DNS_RBTNODE_MAGIC)
#else
#define DNS_RBTNODE_VALID(n)            ISC_TRUE
#endif

/*%
 * This is the structure that is used for each node in the red/black
 * tree of trees.  NOTE WELL:  the implementation manages this as a variable
 * length structure, with the actual wire-format name and other data
 * appended to this structure.  Allocating a contiguous block of memory for
 * multiple dns_rbtnode structures will not work.
 */
typedef struct dns_rbtnode dns_rbtnode_t;
struct dns_rbtnode {
#if DNS_RBT_USEMAGIC
	unsigned int magic;
#endif
	dns_rbtnode_t *parent;
	dns_rbtnode_t *left;
	dns_rbtnode_t *right;
	dns_rbtnode_t *down;
#ifdef DNS_RBT_USEHASH
	dns_rbtnode_t *hashnext;
#endif

	/*%
	 * Used for LRU cache.  This linked list is used to mark nodes which
	 * have no data any longer, but we cannot unlink at that exact moment
	 * because we did not or could not obtain a write lock on the tree.
	 */
	ISC_LINK(dns_rbtnode_t) deadlink;

	/*@{*/
	/*!
	 * The following bitfields add up to a total bitwidth of 32.
	 * The range of values necessary for each item is indicated,
	 * but in the case of "attributes" the field is wider to accommodate
	 * possible future expansion.  "offsetlen" could be one bit
	 * narrower by always adjusting its value by 1 to find the real
	 * offsetlen, but doing so does not gain anything (except perhaps
	 * another bit for "attributes", which doesn't yet need any more).
	 *
	 * In each case below the "range" indicated is what's _necessary_ for
	 * the bitfield to hold, not what it actually _can_ hold.
	 */
	unsigned int is_root : 1;       /*%< range is 0..1 */
	unsigned int color : 1;         /*%< range is 0..1 */
	unsigned int find_callback : 1; /*%< range is 0..1 */
	unsigned int attributes : 3;    /*%< range is 0..2 */
	unsigned int nsec3 : 1;    	/*%< range is 0..1 */
	unsigned int namelen : 8;       /*%< range is 1..255 */
	unsigned int offsetlen : 8;     /*%< range is 1..128 */
	unsigned int padbytes : 9;      /*%< range is 0..380 */
	/*@}*/

#ifdef DNS_RBT_USEHASH
	unsigned int hashval;
#endif

	/*@{*/
	/*!
	 * These values are used in the RBT DB implementation.  The appropriate
	 * node lock must be held before accessing them.
	 */
	void *data;
	unsigned int dirty:1;
	unsigned int wild:1;
	unsigned int locknum:DNS_RBT_LOCKLENGTH;
#ifndef DNS_RBT_USEISCREFCOUNT
	unsigned int references:DNS_RBT_REFLENGTH;
#else
	isc_refcount_t references; /* note that this is not in the bitfield */
#endif
	/*@}*/
};

typedef isc_result_t (*dns_rbtfindcallback_t)(dns_rbtnode_t *node,
					      dns_name_t *name,
					      void *callback_arg);

/*****
 *****  Chain Info
 *****/

/*!
 * A chain is used to keep track of the sequence of nodes to reach any given
 * node from the root of the tree.  Originally nodes did not have parent
 * pointers in them (for memory usage reasons) so there was no way to find
 * the path back to the root from any given node.  Now that nodes have parent
 * pointers, chains might be going away in a future release, though the
 * movement functionality would remain.
 *
 * In any event, parent information, whether via parent pointers or chains, is
 * necessary information for iterating through the tree or for basic internal
 * tree maintenance issues (ie, the rotations that are done to rebalance the
 * tree when a node is added).  The obvious implication of this is that for a
 * chain to remain valid, the tree has to be locked down against writes for the
 * duration of the useful life of the chain, because additions or removals can
 * change the path from the root to the node the chain has targeted.
 *
 * The dns_rbtnodechain_ functions _first, _last, _prev and _next all take
 * dns_name_t parameters for the name and the origin, which can be NULL.  If
 * non-NULL, 'name' will end up pointing to the name data and offsets that are
 * stored at the node (and thus it will be read-only), so it should be a
 * regular dns_name_t that has been initialized with dns_name_init.  When
 * 'origin' is non-NULL, it will get the name of the origin stored in it, so it
 * needs to have its own buffer space and offsets, which is most easily
 * accomplished with a dns_fixedname_t.  It is _not_ necessary to reinitialize
 * either 'name' or 'origin' between calls to the chain functions.
 *
 * NOTE WELL: even though the name data at the root of the tree of trees will
 * be absolute (typically just "."), it will will be made into a relative name
 * with an origin of "." -- an empty name when the node is ".".  This is
 * because a common on operation on 'name' and 'origin' is to use
 * dns_name_concatenate() on them to generate the complete name.  An empty name
 * can be detected when dns_name_countlabels == 0, and is printed by
 * dns_name_totext()/dns_name_format() as "@", consistent with RFC1035's
 * definition of "@" as the current origin.
 *
 * dns_rbtnodechain_current is similar to the _first, _last, _prev and _next
 * functions but additionally can provide the node to which the chain points.
 */

/*%
 * The number of level blocks to allocate at a time.  Currently the maximum
 * number of levels is allocated directly in the structure, but future
 * revisions of this code might have a static initial block with dynamic
 * growth.  Allocating space for 256 levels when the tree is almost never that
 * deep is wasteful, but it's not clear that it matters, since the waste is
 * only 2MB for 1000 concurrently active chains on a system with 64-bit
 * pointers.
 */
#define DNS_RBT_LEVELBLOCK 254

typedef struct dns_rbtnodechain {
	unsigned int            magic;
	isc_mem_t *             mctx;
	/*%
	 * The terminal node of the chain.  It is not in levels[].
	 * This is ostensibly private ... but in a pinch it could be
	 * used tell that the chain points nowhere without needing to
	 * call dns_rbtnodechain_current().
	 */
	dns_rbtnode_t *         end;
	/*%
	 * The maximum number of labels in a name is 128; bitstrings mean
	 * a conceptually very large number (which I have not bothered to
	 * compute) of logical levels because splitting can potentially occur
	 * at each bit.  However, DNSSEC restricts the number of "logical"
	 * labels in a name to 255, meaning only 254 pointers are needed
	 * in the worst case.
	 */
	dns_rbtnode_t *         levels[DNS_RBT_LEVELBLOCK];
	/*%
	 * level_count indicates how deep the chain points into the
	 * tree of trees, and is the index into the levels[] array.
	 * Thus, levels[level_count - 1] is the last level node stored.
	 * A chain that points to the top level of the tree of trees has
	 * a level_count of 0, the first level has a level_count of 1, and
	 * so on.
	 */
	unsigned int            level_count;
	/*%
	 * level_matches tells how many levels matched above the node
	 * returned by dns_rbt_findnode().  A match (partial or exact) found
	 * in the first level thus results in level_matches being set to 1.
	 * This is used by the rbtdb to set the start point for a recursive
	 * search of superdomains until the RR it is looking for is found.
	 */
	unsigned int            level_matches;
} dns_rbtnodechain_t;

/*****
 ***** Public interfaces.
 *****/
isc_result_t
dns_rbt_create(isc_mem_t *mctx, void (*deleter)(void *, void *),
	       void *deleter_arg, dns_rbt_t **rbtp);
/*%<
 * Initialize a red-black tree of trees.
 *
 * Notes:
 *\li   The deleter argument, if non-null, points to a function that is
 *      responsible for cleaning up any memory associated with the data
 *      pointer of a node when the node is deleted.  It is passed the
 *      deleted node's data pointer as its first argument and deleter_arg
 *      as its second argument.
 *
 * Requires:
 * \li  mctx is a pointer to a valid memory context.
 *\li   rbtp != NULL && *rbtp == NULL
 *\li   arg == NULL iff deleter == NULL
 *
 * Ensures:
 *\li   If result is ISC_R_SUCCESS:
 *              *rbtp points to a valid red-black tree manager
 *
 *\li   If result is failure:
 *              *rbtp does not point to a valid red-black tree manager.
 *
 * Returns:
 *\li   #ISC_R_SUCCESS  Success
 *\li   #ISC_R_NOMEMORY Resource limit: Out of Memory
 */

isc_result_t
dns_rbt_addname(dns_rbt_t *rbt, dns_name_t *name, void *data);
/*%<
 * Add 'name' to the tree of trees, associated with 'data'.
 *
 * Notes:
 *\li   'data' is never required to be non-NULL, but specifying it
 *      when the name is added is faster than searching for 'name'
 *      again and then setting the data pointer.  The lack of a data pointer
 *      for a node also has other ramifications regarding whether
 *      dns_rbt_findname considers a node to exist, or dns_rbt_deletename
 *      joins nodes.
 *
 * Requires:
 *\li   rbt is a valid rbt manager.
 *\li   dns_name_isabsolute(name) == TRUE
 *
 * Ensures:
 *\li   'name' is not altered in any way.
 *
 *\li   Any external references to nodes in the tree are unaffected by
 *      node splits that are necessary to insert the new name.
 *
 *\li   If result is #ISC_R_SUCCESS:
 *              'name' is findable in the red/black tree of trees in O(log N).
 *              The data pointer of the node for 'name' is set to 'data'.
 *
 *\li   If result is #ISC_R_EXISTS or #ISC_R_NOSPACE:
 *              The tree of trees is unaltered.
 *
 *\li   If result is #ISC_R_NOMEMORY:
 *              No guarantees.
 *
 * Returns:
 *\li   #ISC_R_SUCCESS  Success
 *\li   #ISC_R_EXISTS   The name already exists with associated data.
 *\li   #ISC_R_NOSPACE  The name had more logical labels than are allowed.
 *\li   #ISC_R_NOMEMORY Resource Limit: Out of Memory
 */

isc_result_t
dns_rbt_addnode(dns_rbt_t *rbt, dns_name_t *name, dns_rbtnode_t **nodep);

/*%<
 * Just like dns_rbt_addname, but returns the address of the node.
 *
 * Requires:
 *\li   rbt is a valid rbt structure.
 *\li   dns_name_isabsolute(name) == TRUE
 *\li   nodep != NULL && *nodep == NULL
 *
 * Ensures:
 *\li   'name' is not altered in any way.
 *
 *\li   Any external references to nodes in the tree are unaffected by
 *      node splits that are necessary to insert the new name.
 *
 *\li   If result is ISC_R_SUCCESS:
 *              'name' is findable in the red/black tree of trees in O(log N).
 *              *nodep is the node that was added for 'name'.
 *
 *\li   If result is ISC_R_EXISTS:
 *              The tree of trees is unaltered.
 *              *nodep is the existing node for 'name'.
 *
 *\li   If result is ISC_R_NOMEMORY:
 *              No guarantees.
 *
 * Returns:
 *\li   #ISC_R_SUCCESS  Success
 *\li   #ISC_R_EXISTS   The name already exists, possibly without data.
 *\li   #ISC_R_NOMEMORY Resource Limit: Out of Memory
 */

isc_result_t
dns_rbt_findname(dns_rbt_t *rbt, dns_name_t *name, unsigned int options,
		 dns_name_t *foundname, void **data);
/*%<
 * Get the data pointer associated with 'name'.
 *
 * Notes:
 *\li   When #DNS_RBTFIND_NOEXACT is set, the closest matching superdomain is
 *      returned (also subject to #DNS_RBTFIND_EMPTYDATA), even when there is
 *      an exact match in the tree.
 *
 *\li   A node that has no data is considered not to exist for this function,
 *      unless the #DNS_RBTFIND_EMPTYDATA option is set.
 *
 * Requires:
 *\li   rbt is a valid rbt manager.
 *\li   dns_name_isabsolute(name) == TRUE
 *\li   data != NULL && *data == NULL
 *
 * Ensures:
 *\li   'name' and the tree are not altered in any way.
 *
 *\li   If result is ISC_R_SUCCESS:
 *              *data is the data associated with 'name'.
 *
 *\li   If result is DNS_R_PARTIALMATCH:
 *              *data is the data associated with the deepest superdomain
 *              of 'name' which has data.
 *
 *\li   If result is ISC_R_NOTFOUND:
 *              Neither the name nor a superdomain was found with data.
 *
 * Returns:
 *\li   #ISC_R_SUCCESS          Success
 *\li   #DNS_R_PARTIALMATCH     Superdomain found with data
 *\li   #ISC_R_NOTFOUND         No match
 *\li   #ISC_R_NOSPACE          Concatenating nodes to form foundname failed
 */

isc_result_t
dns_rbt_findnode(dns_rbt_t *rbt, dns_name_t *name, dns_name_t *foundname,
		 dns_rbtnode_t **node, dns_rbtnodechain_t *chain,
		 unsigned int options, dns_rbtfindcallback_t callback,
		 void *callback_arg);
/*%<
 * Find the node for 'name'.
 *
 * Notes:
 *\li   A node that has no data is considered not to exist for this function,
 *      unless the DNS_RBTFIND_EMPTYDATA option is set.  This applies to both
 *      exact matches and partial matches.
 *
 *\li   If the chain parameter is non-NULL, then the path through the tree
 *      to the DNSSEC predecessor of the searched for name is maintained,
 *      unless the DNS_RBTFIND_NOPREDECESSOR or DNS_RBTFIND_NOEXACT option
 *      is used. (For more details on those options, see below.)
 *
 *\li   If there is no predecessor, then the chain will point to nowhere, as
 *      indicated by chain->end being NULL or dns_rbtnodechain_current
 *      returning ISC_R_NOTFOUND.  Note that in a normal Internet DNS RBT
 *      there will always be a predecessor for all names except the root
 *      name, because '.' will exist and '.' is the predecessor of
 *      everything.  But you can certainly construct a trivial tree and a
 *      search for it that has no predecessor.
 *
 *\li   Within the chain structure, the 'levels' member of the structure holds
 *      the root node of each level except the first.
 *
 *\li   The 'level_count' of the chain indicates how deep the chain to the
 *      predecessor name is, as an index into the 'levels[]' array.  It does
 *      not count name elements, per se, but only levels of the tree of trees,
 *      the distinction arising because multiple labels from a name can be
 *      stored on only one level.  It is also does not include the level
 *      that has the node, since that level is not stored in levels[].
 *
 *\li   The chain's 'level_matches' is not directly related to the predecessor.
 *      It is the number of levels above the level of the found 'node',
 *      regardless of whether it was a partial match or exact match.  When
 *      the node is found in the top level tree, or no node is found at all,
 *      level_matches is 0.
 *
 *\li   When DNS_RBTFIND_NOEXACT is set, the closest matching superdomain is
 *      returned (also subject to DNS_RBTFIND_EMPTYDATA), even when
 *      there is an exact match in the tree.  In this case, the chain
 *      will not point to the DNSSEC predecessor, but will instead point
 *      to the exact match, if there was any.  Thus the preceding paragraphs
 *      should have "exact match" substituted for "predecessor" to describe
 *      how the various elements of the chain are set.  This was done to
 *      ensure that the chain's state was sane, and to prevent problems that
 *      occurred when running the predecessor location code under conditions
 *      it was not designed for.  It is not clear *where* the chain should
 *      point when DNS_RBTFIND_NOEXACT is set, so if you end up using a chain
 *      with this option because you want a particular node, let us know
 *      where you want the chain pointed, so this can be made more firm.
 *
 * Requires:
 *\li   rbt is a valid rbt manager.
 *\li   dns_name_isabsolute(name) == TRUE.
 *\li   node != NULL && *node == NULL.
 *\li   #DNS_RBTFIND_NOEXACT and DNS_RBTFIND_NOPREDECESSOR are mutually
 *              exclusive.
 *
 * Ensures:
 *\li   'name' and the tree are not altered in any way.
 *
 *\li   If result is ISC_R_SUCCESS:
 *\verbatim
 *              *node is the terminal node for 'name'.

 *              'foundname' and 'name' represent the same name (though not
 *              the same memory).

 *              'chain' points to the DNSSEC predecessor, if any, of 'name'.
 *
 *              chain->level_matches and chain->level_count are equal.
 *\endverbatim
 *
 *      If result is DNS_R_PARTIALMATCH:
 *\verbatim
 *              *node is the data associated with the deepest superdomain
 *              of 'name' which has data.
 *
 *              'foundname' is the name of deepest superdomain (which has
 *              data, unless the DNS_RBTFIND_EMPTYDATA option is set).
 *
 *              'chain' points to the DNSSEC predecessor, if any, of 'name'.
 *\endverbatim
 *
 *\li   If result is ISC_R_NOTFOUND:
 *\verbatim
 *              Neither the name nor a superdomain was found.  *node is NULL.
 *
 *              'chain' points to the DNSSEC predecessor, if any, of 'name'.
 *
 *              chain->level_matches is 0.
 *\endverbatim
 *
 * Returns:
 *\li   #ISC_R_SUCCESS          Success
 *\li   #DNS_R_PARTIALMATCH     Superdomain found with data
 *\li   #ISC_R_NOTFOUND         No match, or superdomain with no data
 *\li   #ISC_R_NOSPACE Concatenating nodes to form foundname failed
 */

isc_result_t
dns_rbt_deletename(dns_rbt_t *rbt, dns_name_t *name, isc_boolean_t recurse);
/*%<
 * Delete 'name' from the tree of trees.
 *
 * Notes:
 *\li   When 'name' is removed, if recurse is ISC_TRUE then all of its
 *      subnames are removed too.
 *
 * Requires:
 *\li   rbt is a valid rbt manager.
 *\li   dns_name_isabsolute(name) == TRUE
 *
 * Ensures:
 *\li   'name' is not altered in any way.
 *
 *\li   Does NOT ensure that any external references to nodes in the tree
 *      are unaffected by node joins.
 *
 *\li   If result is ISC_R_SUCCESS:
 *              'name' does not appear in the tree with data; however,
 *              the node for the name might still exist which can be
 *              found with dns_rbt_findnode (but not dns_rbt_findname).
 *
 *\li   If result is ISC_R_NOTFOUND:
 *              'name' does not appear in the tree with data, because
 *              it did not appear in the tree before the function was called.
 *
 *\li   If result is something else:
 *              See result codes for dns_rbt_findnode (if it fails, the
 *              node is not deleted) or dns_rbt_deletenode (if it fails,
 *              the node is deleted, but the tree is not optimized when
 *              it could have been).
 *
 * Returns:
 *\li   #ISC_R_SUCCESS  Success
 *\li   #ISC_R_NOTFOUND No match
 *\li   something_else  Any return code from dns_rbt_findnode except
 *                      DNS_R_PARTIALMATCH (which causes ISC_R_NOTFOUND
 *                      to be returned instead), and any code from
 *                      dns_rbt_deletenode.
 */

isc_result_t
dns_rbt_deletenode(dns_rbt_t *rbt, dns_rbtnode_t *node, isc_boolean_t recurse);
/*%<
 * Delete 'node' from the tree of trees.
 *
 * Notes:
 *\li   When 'node' is removed, if recurse is ISC_TRUE then all nodes
 *      in levels down from it are removed too.
 *
 * Requires:
 *\li   rbt is a valid rbt manager.
 *\li   node != NULL.
 *
 * Ensures:
 *\li   Does NOT ensure that any external references to nodes in the tree
 *      are unaffected by node joins.
 *
 *\li   If result is ISC_R_SUCCESS:
 *              'node' does not appear in the tree with data; however,
 *              the node might still exist if it serves as a pointer to
 *              a lower tree level as long as 'recurse' was false, hence
 *              the node could can be found with dns_rbt_findnode when
 *              that function's empty_data_ok parameter is true.
 *
 *\li   If result is ISC_R_NOMEMORY or ISC_R_NOSPACE:
 *              The node was deleted, but the tree structure was not
 *              optimized.
 *
 * Returns:
 *\li   #ISC_R_SUCCESS  Success
 *\li   #ISC_R_NOMEMORY Resource Limit: Out of Memory when joining nodes.
 *\li   #ISC_R_NOSPACE  dns_name_concatenate failed when joining nodes.
 */

void
dns_rbt_namefromnode(dns_rbtnode_t *node, dns_name_t *name);
/*%<
 * Convert the sequence of labels stored at 'node' into a 'name'.
 *
 * Notes:
 *\li   This function does not return the full name, from the root, but
 *      just the labels at the indicated node.
 *
 *\li   The name data pointed to by 'name' is the information stored
 *      in the node, not a copy.  Altering the data at this pointer
 *      will likely cause grief.
 *
 * Requires:
 * \li  name->offsets == NULL
 *
 * Ensures:
 * \li  'name' is DNS_NAMEATTR_READONLY.
 *
 * \li  'name' will point directly to the labels stored after the
 *      dns_rbtnode_t struct.
 *
 * \li  'name' will have offsets that also point to the information stored
 *      as part of the node.
 */

isc_result_t
dns_rbt_fullnamefromnode(dns_rbtnode_t *node, dns_name_t *name);
/*%<
 * Like dns_rbt_namefromnode, but returns the full name from the root.
 *
 * Notes:
 * \li  Unlike dns_rbt_namefromnode, the name will not point directly
 *      to node data.  Rather, dns_name_concatenate will be used to copy
 *      the name data from each node into the 'name' argument.
 *
 * Requires:
 * \li  name != NULL
 * \li  name has a dedicated buffer.
 *
 * Returns:
 * \li  ISC_R_SUCCESS
 * \li  ISC_R_NOSPACE           (possible via dns_name_concatenate)
 * \li  DNS_R_NAMETOOLONG       (possible via dns_name_concatenate)
 */

char *
dns_rbt_formatnodename(dns_rbtnode_t *node, char *printname,
		       unsigned int size);
/*%<
 * Format the full name of a node for printing, using dns_name_format().
 *
 * Notes:
 * \li  'size' is the length of the printname buffer.  This should be
 *      DNS_NAME_FORMATSIZE or larger.
 *
 * Requires:
 * \li  node and printname are not NULL.
 *
 * Returns:
 * \li  The 'printname' pointer.
 */

unsigned int
dns_rbt_nodecount(dns_rbt_t *rbt);
/*%<
 * Obtain the number of nodes in the tree of trees.
 *
 * Requires:
 * \li  rbt is a valid rbt manager.
 */

void
dns_rbt_destroy(dns_rbt_t **rbtp);
isc_result_t
dns_rbt_destroy2(dns_rbt_t **rbtp, unsigned int quantum);
/*%<
 * Stop working with a red-black tree of trees.
 * If 'quantum' is zero then the entire tree will be destroyed.
 * If 'quantum' is non zero then up to 'quantum' nodes will be destroyed
 * allowing the rbt to be incrementally destroyed by repeated calls to
 * dns_rbt_destroy2().  Once dns_rbt_destroy2() has been called no other
 * operations than dns_rbt_destroy()/dns_rbt_destroy2() should be
 * performed on the tree of trees.
 *
 * Requires:
 * \li  *rbt is a valid rbt manager.
 *
 * Ensures on ISC_R_SUCCESS:
 * \li  All space allocated by the RBT library has been returned.
 *
 * \li  *rbt is invalidated as an rbt manager.
 *
 * Returns:
 * \li  ISC_R_SUCCESS
 * \li  ISC_R_QUOTA if 'quantum' nodes have been destroyed.
 */

void
dns_rbt_printall(dns_rbt_t *rbt);
/*%<
 * Print an ASCII representation of the internal structure of the red-black
 * tree of trees.
 *
 * Notes:
 * \li  The name stored at each node, along with the node's color, is printed.
 *      Then the down pointer, left and right pointers are displayed
 *      recursively in turn.  NULL down pointers are silently omitted;
 *      NULL left and right pointers are printed.
 */

/*****
 ***** Chain Functions
 *****/

void
dns_rbtnodechain_init(dns_rbtnodechain_t *chain, isc_mem_t *mctx);
/*%<
 * Initialize 'chain'.
 *
 * Requires:
 *\li   'chain' is a valid pointer.
 *
 *\li   'mctx' is a valid memory context.
 *
 * Ensures:
 *\li   'chain' is suitable for use.
 */

void
dns_rbtnodechain_reset(dns_rbtnodechain_t *chain);
/*%<
 * Free any dynamic storage associated with 'chain', and then reinitialize
 * 'chain'.
 *
 * Requires:
 *\li   'chain' is a valid pointer.
 *
 * Ensures:
 *\li   'chain' is suitable for use, and uses no dynamic storage.
 */

void
dns_rbtnodechain_invalidate(dns_rbtnodechain_t *chain);
/*%<
 * Free any dynamic storage associated with 'chain', and then invalidates it.
 *
 * Notes:
 *\li   Future calls to any dns_rbtnodechain_ function will need to call
 *      dns_rbtnodechain_init on the chain first (except, of course,
 *      dns_rbtnodechain_init itself).
 *
 * Requires:
 *\li   'chain' is a valid chain.
 *
 * Ensures:
 *\li   'chain' is no longer suitable for use, and uses no dynamic storage.
 */

isc_result_t
dns_rbtnodechain_current(dns_rbtnodechain_t *chain, dns_name_t *name,
			 dns_name_t *origin, dns_rbtnode_t **node);
/*%<
 * Provide the name, origin and node to which the chain is currently pointed.
 *
 * Notes:
 *\li   The tree need not have be locked against additions for the chain
 *      to remain valid, however there are no guarantees if any deletion
 *      has been made since the chain was established.
 *
 * Requires:
 *\li   'chain' is a valid chain.
 *
 * Ensures:
 *\li   'node', if non-NULL, is the node to which the chain was pointed
 *      by dns_rbt_findnode, dns_rbtnodechain_first or dns_rbtnodechain_last.
 *      If none were called for the chain since it was initialized or reset,
 *      or if the was no predecessor to the name searched for with
 *      dns_rbt_findnode, then '*node' is NULL and ISC_R_NOTFOUND is returned.
 *
 *\li   'name', if non-NULL, is the name stored at the terminal level of
 *      the chain.  This is typically a single label, like the "www" of
 *      "www.isc.org", but need not be so.  At the root of the tree of trees,
 *      if the node is "." then 'name' is ".", otherwise it is relative to ".".
 *      (Minimalist and atypical case:  if the tree has just the name
 *      "isc.org." then the root node's stored name is "isc.org." but 'name'
 *      will be "isc.org".)
 *
 *\li   'origin', if non-NULL, is the sequence of labels in the levels
 *      above the terminal level, such as "isc.org." in the above example.
 *      'origin' is always "." for the root node.
 *
 *
 * Returns:
 *\li   #ISC_R_SUCCESS          name, origin & node were successfully set.
 *\li   #ISC_R_NOTFOUND         The chain does not point to any node.
 *\li   &lt;something_else>     Any error return from dns_name_concatenate.
 */

isc_result_t
dns_rbtnodechain_first(dns_rbtnodechain_t *chain, dns_rbt_t *rbt,
		       dns_name_t *name, dns_name_t *origin);
/*%<
 * Set the chain to the lexically first node in the tree of trees.
 *
 * Notes:
 *\li   By the definition of ordering for DNS names, the root of the tree of
 *      trees is the very first node, since everything else in the megatree
 *      uses it as a common suffix.
 *
 * Requires:
 *\li   'chain' is a valid chain.
 *\li   'rbt' is a valid rbt manager.
 *
 * Ensures:
 *\li   The chain points to the very first node of the tree.
 *
 *\li   'name' and 'origin', if non-NULL, are set as described for
 *      dns_rbtnodechain_current.  Thus 'origin' will always be ".".
 *
 * Returns:
 *\li   #DNS_R_NEWORIGIN                The name & origin were successfully set.
 *\li   &lt;something_else>     Any error result from dns_rbtnodechain_current.
 */

isc_result_t
dns_rbtnodechain_last(dns_rbtnodechain_t *chain, dns_rbt_t *rbt,
		       dns_name_t *name, dns_name_t *origin);
/*%<
 * Set the chain to the lexically last node in the tree of trees.
 *
 * Requires:
 *\li   'chain' is a valid chain.
 *\li   'rbt' is a valid rbt manager.
 *
 * Ensures:
 *\li   The chain points to the very last node of the tree.
 *
 *\li   'name' and 'origin', if non-NULL, are set as described for
 *      dns_rbtnodechain_current.
 *
 * Returns:
 *\li   #DNS_R_NEWORIGIN                The name & origin were successfully set.
 *\li   #ISC_R_NOMEMORY         Resource Limit: Out of Memory building chain.
 *\li   &lt;something_else>     Any error result from dns_name_concatenate.
 */

isc_result_t
dns_rbtnodechain_prev(dns_rbtnodechain_t *chain, dns_name_t *name,
		      dns_name_t *origin);
/*%<
 * Adjusts chain to point the DNSSEC predecessor of the name to which it
 * is currently pointed.
 *
 * Requires:
 *\li   'chain' is a valid chain.
 *\li   'chain' has been pointed somewhere in the tree with dns_rbt_findnode,
 *      dns_rbtnodechain_first or dns_rbtnodechain_last -- and remember that
 *      dns_rbt_findnode is not guaranteed to point the chain somewhere,
 *      since there may have been no predecessor to the searched for name.
 *
 * Ensures:
 *\li   The chain is pointed to the predecessor of its current target.
 *
 *\li   'name' and 'origin', if non-NULL, are set as described for
 *      dns_rbtnodechain_current.
 *
 *\li   'origin' is only if a new origin was found.
 *
 * Returns:
 *\li   #ISC_R_SUCCESS          The predecessor was found and 'name' was set.
 *\li   #DNS_R_NEWORIGIN                The predecessor was found with a different
 *                              origin and 'name' and 'origin' were set.
 *\li   #ISC_R_NOMORE           There was no predecessor.
 *\li   &lt;something_else>     Any error result from dns_rbtnodechain_current.
 */

isc_result_t
dns_rbtnodechain_next(dns_rbtnodechain_t *chain, dns_name_t *name,
		      dns_name_t *origin);
/*%<
 * Adjusts chain to point the DNSSEC successor of the name to which it
 * is currently pointed.
 *
 * Requires:
 *\li   'chain' is a valid chain.
 *\li   'chain' has been pointed somewhere in the tree with dns_rbt_findnode,
 *      dns_rbtnodechain_first or dns_rbtnodechain_last -- and remember that
 *      dns_rbt_findnode is not guaranteed to point the chain somewhere,
 *      since there may have been no predecessor to the searched for name.
 *
 * Ensures:
 *\li   The chain is pointed to the successor of its current target.
 *
 *\li   'name' and 'origin', if non-NULL, are set as described for
 *      dns_rbtnodechain_current.
 *
 *\li   'origin' is only if a new origin was found.
 *
 * Returns:
 *\li   #ISC_R_SUCCESS          The successor was found and 'name' was set.
 *\li   #DNS_R_NEWORIGIN                The successor was found with a different
 *                              origin and 'name' and 'origin' were set.
 *\li   #ISC_R_NOMORE           There was no successor.
 *\li   &lt;something_else>     Any error result from dns_name_concatenate.
 */

isc_result_t
dns_rbtnodechain_down(dns_rbtnodechain_t *chain, dns_name_t *name,
		      dns_name_t *origin);
/*%<
 * Descend down if possible.
 */

isc_result_t
dns_rbtnodechain_nextflat(dns_rbtnodechain_t *chain, dns_name_t *name);
/*%<
 * Find the next node at the current depth in DNSSEC order.
 */

/*
 * Wrapper macros for manipulating the rbtnode reference counter:
 *   Since we selectively use isc_refcount_t for the reference counter of
 *   a rbtnode, operations on the counter depend on the actual type of it.
 *   The following macros provide a common interface to these operations,
 *   hiding the back-end.  The usage is the same as that of isc_refcount_xxx().
 */
#ifdef DNS_RBT_USEISCREFCOUNT
#define dns_rbtnode_refinit(node, n)                            \
	do {                                                    \
		isc_refcount_init(&(node)->references, (n));    \
	} while (0)
#define dns_rbtnode_refdestroy(node)                            \
	do {                                                    \
		isc_refcount_destroy(&(node)->references);      \
	} while (0)
#define dns_rbtnode_refcurrent(node)                            \
	isc_refcount_current(&(node)->references)
#define dns_rbtnode_refincrement0(node, refs)                   \
	do {                                                    \
		isc_refcount_increment0(&(node)->references, (refs)); \
	} while (0)
#define dns_rbtnode_refincrement(node, refs)                    \
	do {                                                    \
		isc_refcount_increment(&(node)->references, (refs)); \
	} while (0)
#define dns_rbtnode_refdecrement(node, refs)                    \
	do {                                                    \
		isc_refcount_decrement(&(node)->references, (refs)); \
	} while (0)
#else  /* DNS_RBT_USEISCREFCOUNT */
#define dns_rbtnode_refinit(node, n)    ((node)->references = (n))
#define dns_rbtnode_refdestroy(node)    (REQUIRE((node)->references == 0))
#define dns_rbtnode_refcurrent(node)    ((node)->references)
#define dns_rbtnode_refincrement0(node, refs)                   \
	do {                                                    \
		unsigned int *_tmp = (unsigned int *)(refs);    \
		(node)->references++;                           \
		if ((_tmp) != NULL)                             \
			(*_tmp) = (node)->references;           \
	} while (0)
#define dns_rbtnode_refincrement(node, refs)                    \
	do {                                                    \
		REQUIRE((node)->references > 0);                \
		(node)->references++;                           \
		if ((refs) != NULL)                             \
			(*refs) = (node)->references;           \
	} while (0)
#define dns_rbtnode_refdecrement(node, refs)                    \
	do {                                                    \
		REQUIRE((node)->references > 0);                \
		(node)->references--;                           \
		if ((refs) != NULL)                             \
			(*refs) = (node)->references;           \
	} while (0)
#endif /* DNS_RBT_USEISCREFCOUNT */

ISC_LANG_ENDDECLS

#endif /* DNS_RBT_H */
