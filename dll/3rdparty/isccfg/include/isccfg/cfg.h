/*
 * Copyright (C) 2004-2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2000-2002  Internet Software Consortium.
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

/* $Id: cfg.h,v 1.44 2007/10/12 04:17:18 each Exp $ */

#ifndef ISCCFG_CFG_H
#define ISCCFG_CFG_H 1

/*****
 ***** Module Info
 *****/

/*! \file isccfg/cfg.h
 * \brief
 * This is the new, table-driven, YACC-free configuration file parser.
 */

/***
 *** Imports
 ***/

#include <isc/formatcheck.h>
#include <isc/lang.h>
#include <isc/types.h>
#include <isc/list.h>


/***
 *** Types
 ***/

/*%
 * A configuration parser.
 */
typedef struct cfg_parser cfg_parser_t;

/*%
 * A configuration type definition object.  There is a single
 * static cfg_type_t object for each data type supported by
 * the configuration parser.
 */
typedef struct cfg_type cfg_type_t;

/*%
 * A configuration object.  This is the basic building block of the
 * configuration parse tree.  It contains a value (which may be
 * of one of several types) and information identifying the file
 * and line number the value came from, for printing error
 * messages.
 */
typedef struct cfg_obj cfg_obj_t;

/*%
 * A configuration object list element.
 */
typedef struct cfg_listelt cfg_listelt_t;

/*%
 * A callback function to be called when parsing an option 
 * that needs to be interpreted at parsing time, like
 * "directory".
 */
typedef isc_result_t
(*cfg_parsecallback_t)(const char *clausename, const cfg_obj_t *obj, void *arg);

/***
 *** Functions
 ***/

ISC_LANG_BEGINDECLS

isc_result_t
cfg_parser_create(isc_mem_t *mctx, isc_log_t *lctx, cfg_parser_t **ret);
/*%<
 * Create a configuration file parser.  Any warning and error
 * messages will be logged to 'lctx'.
 *
 * The parser object returned can be used for a single call
 * to cfg_parse_file() or cfg_parse_buffer().  It must not
 * be reused for parsing multiple files or buffers.
 */

void
cfg_parser_setcallback(cfg_parser_t *pctx,
		       cfg_parsecallback_t callback,
		       void *arg);
/*%<
 * Make the parser call 'callback' whenever it encounters
 * a configuration clause with the callback attribute,
 * passing it the clause name, the clause value,
 * and 'arg' as arguments.
 *
 * To restore the default of not invoking callbacks, pass
 * callback==NULL and arg==NULL.
 */

isc_result_t
cfg_parse_file(cfg_parser_t *pctx, const char *filename,
	       const cfg_type_t *type, cfg_obj_t **ret);
isc_result_t
cfg_parse_buffer(cfg_parser_t *pctx, isc_buffer_t *buffer,
		 const cfg_type_t *type, cfg_obj_t **ret);
/*%<
 * Read a configuration containing data of type 'type'
 * and make '*ret' point to its parse tree.
 *
 * The configuration is read from the file 'filename'
 * (isc_parse_file()) or the buffer 'buffer'
 * (isc_parse_buffer()).
 *
 * Returns an error if the file does not parse correctly.
 * 
 * Requires:
 *\li 	"filename" is valid.
 *\li 	"mem" is valid.
 *\li	"type" is valid.
 *\li 	"cfg" is non-NULL and "*cfg" is NULL.
 *
 * Returns:
 *     \li #ISC_R_SUCCESS                 - success
 *\li      #ISC_R_NOMEMORY                - no memory available
 *\li      #ISC_R_INVALIDFILE             - file doesn't exist or is unreadable
 *\li      others	                      - file contains errors
 */

void
cfg_parser_destroy(cfg_parser_t **pctxp);
/*%<
 * Destroy a configuration parser.
 */

isc_boolean_t
cfg_obj_isvoid(const cfg_obj_t *obj);
/*%<
 * Return true iff 'obj' is of void type (e.g., an optional 
 * value not specified).
 */

isc_boolean_t
cfg_obj_ismap(const cfg_obj_t *obj);
/*%<
 * Return true iff 'obj' is of a map type.
 */

isc_result_t
cfg_map_get(const cfg_obj_t *mapobj, const char* name, const cfg_obj_t **obj);
/*%<
 * Extract an element from a configuration object, which
 * must be of a map type.
 *
 * Requires:
 * \li     'mapobj' points to a valid configuration object of a map type.
 * \li     'name' points to a null-terminated string.
 * \li	'obj' is non-NULL and '*obj' is NULL.
 *
 * Returns:
 * \li     #ISC_R_SUCCESS                  - success
 * \li     #ISC_R_NOTFOUND                 - name not found in map
 */

const cfg_obj_t *
cfg_map_getname(const cfg_obj_t *mapobj);
/*%<
 * Get the name of a named map object, like a server "key" clause.
 *
 * Requires:
 *    \li  'mapobj' points to a valid configuration object of a map type.
 *
 * Returns:
 * \li     A pointer to a configuration object naming the map object,
 *	or NULL if the map object does not have a name.
 */

isc_boolean_t
cfg_obj_istuple(const cfg_obj_t *obj);
/*%<
 * Return true iff 'obj' is of a map type.
 */

const cfg_obj_t *
cfg_tuple_get(const cfg_obj_t *tupleobj, const char *name);
/*%<
 * Extract an element from a configuration object, which
 * must be of a tuple type.
 *
 * Requires:
 * \li     'tupleobj' points to a valid configuration object of a tuple type.
 * \li     'name' points to a null-terminated string naming one of the
 *\li	fields of said tuple type.
 */

isc_boolean_t
cfg_obj_isuint32(const cfg_obj_t *obj);
/*%<
 * Return true iff 'obj' is of integer type.
 */

isc_uint32_t
cfg_obj_asuint32(const cfg_obj_t *obj);
/*%<
 * Returns the value of a configuration object of 32-bit integer type.
 *
 * Requires:
 * \li     'obj' points to a valid configuration object of 32-bit integer type.
 *
 * Returns:
 * \li     A 32-bit unsigned integer.
 */

isc_boolean_t
cfg_obj_isuint64(const cfg_obj_t *obj);
/*%<
 * Return true iff 'obj' is of integer type.
 */

isc_uint64_t
cfg_obj_asuint64(const cfg_obj_t *obj);
/*%<
 * Returns the value of a configuration object of 64-bit integer type.
 *
 * Requires:
 * \li     'obj' points to a valid configuration object of 64-bit integer type.
 *
 * Returns:
 * \li     A 64-bit unsigned integer.
 */

isc_boolean_t
cfg_obj_isstring(const cfg_obj_t *obj);
/*%<
 * Return true iff 'obj' is of string type.
 */

const char *
cfg_obj_asstring(const cfg_obj_t *obj);
/*%<
 * Returns the value of a configuration object of a string type
 * as a null-terminated string.
 *
 * Requires:
 * \li     'obj' points to a valid configuration object of a string type.
 *
 * Returns:
 * \li     A pointer to a null terminated string.
 */

isc_boolean_t
cfg_obj_isboolean(const cfg_obj_t *obj);
/*%<
 * Return true iff 'obj' is of a boolean type.
 */

isc_boolean_t
cfg_obj_asboolean(const cfg_obj_t *obj);
/*%<
 * Returns the value of a configuration object of a boolean type.
 *
 * Requires:
 * \li     'obj' points to a valid configuration object of a boolean type.
 *
 * Returns:
 * \li     A boolean value.
 */

isc_boolean_t
cfg_obj_issockaddr(const cfg_obj_t *obj);
/*%<
 * Return true iff 'obj' is a socket address.
 */

const isc_sockaddr_t *
cfg_obj_assockaddr(const cfg_obj_t *obj);
/*%<
 * Returns the value of a configuration object representing a socket address.
 *
 * Requires:
 * \li     'obj' points to a valid configuration object of a socket address type.
 *
 * Returns:
 * \li     A pointer to a sockaddr.  The sockaddr must be copied by the caller
 *      if necessary.
 */

isc_boolean_t
cfg_obj_isnetprefix(const cfg_obj_t *obj);
/*%<
 * Return true iff 'obj' is a network prefix.
 */

void
cfg_obj_asnetprefix(const cfg_obj_t *obj, isc_netaddr_t *netaddr,
		    unsigned int *prefixlen);
/*%<
 * Gets the value of a configuration object representing a network
 * prefix.  The network address is returned through 'netaddr' and the
 * prefix length in bits through 'prefixlen'.
 *
 * Requires:
 * \li     'obj' points to a valid configuration object of network prefix type.
 *\li	'netaddr' and 'prefixlen' are non-NULL.
 */

isc_boolean_t
cfg_obj_islist(const cfg_obj_t *obj);
/*%<
 * Return true iff 'obj' is of list type.
 */

const cfg_listelt_t *
cfg_list_first(const cfg_obj_t *obj);
/*%<
 * Returns the first list element in a configuration object of a list type.
 *
 * Requires:
 * \li     'obj' points to a valid configuration object of a list type or NULL.
 *
 * Returns:
 *   \li   A pointer to a cfg_listelt_t representing the first list element,
 * 	or NULL if the list is empty or nonexistent.
 */

const cfg_listelt_t *
cfg_list_next(const cfg_listelt_t *elt);
/*%<
 * Returns the next element of a list of configuration objects.
 *
 * Requires:
 * \li     'elt' points to cfg_listelt_t obtained from cfg_list_first() or
 *	a previous call to cfg_list_next().
 *
 * Returns:
 * \li     A pointer to a cfg_listelt_t representing the next element,
 * 	or NULL if there are no more elements.
 */

unsigned int
cfg_list_length(const cfg_obj_t *obj, isc_boolean_t recurse);
/*%<
 * Returns the length of a list of configure objects.  If obj is
 * not a list, returns 0.  If recurse is true, add in the length of
 * all contained lists.
 */

const cfg_obj_t *
cfg_listelt_value(const cfg_listelt_t *elt);
/*%<
 * Returns the configuration object associated with cfg_listelt_t.
 *
 * Requires:
 * \li     'elt' points to cfg_listelt_t obtained from cfg_list_first() or
 *	cfg_list_next().
 *
 * Returns:
 * \li     A non-NULL pointer to a configuration object.
 */

void
cfg_print(const cfg_obj_t *obj,
	  void (*f)(void *closure, const char *text, int textlen),
	  void *closure);
/*%<
 * Print the configuration object 'obj' by repeatedly calling the
 * function 'f', passing 'closure' and a region of text starting
 * at 'text' and comprising 'textlen' characters.
 */

void
cfg_print_grammar(const cfg_type_t *type,
	  void (*f)(void *closure, const char *text, int textlen),
	  void *closure);
/*%<
 * Print a summary of the grammar of the configuration type 'type'.
 */

isc_boolean_t
cfg_obj_istype(const cfg_obj_t *obj, const cfg_type_t *type);
/*%<
 * Return true iff 'obj' is of type 'type'. 
 */

void cfg_obj_destroy(cfg_parser_t *pctx, cfg_obj_t **obj);
/*%<
 * Destroy a configuration object.
 */

void
cfg_obj_log(const cfg_obj_t *obj, isc_log_t *lctx, int level,
            const char *fmt, ...)
	ISC_FORMAT_PRINTF(4, 5);
/*%<
 * Log a message concerning configuration object 'obj' to the logging
 * channel of 'pctx', at log level 'level'.  The message will be prefixed
 * with the file name(s) and line number where 'obj' was defined.
 */

const char *
cfg_obj_file(const cfg_obj_t *obj);
/*%<
 * Return the file that defined this object.
 */

unsigned int
cfg_obj_line(const cfg_obj_t *obj);
/*%<
 * Return the line in file where this object was defined.
 */


ISC_LANG_ENDDECLS

#endif /* ISCCFG_CFG_H */
