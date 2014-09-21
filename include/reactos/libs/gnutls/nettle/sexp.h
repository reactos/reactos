/* sexp.h
 *
 * Parsing s-expressions.
 */

/* nettle, low-level cryptographics library
 *
 * Copyright (C) 2002 Niels Möller
 *  
 * The nettle library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 * 
 * The nettle library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with the nettle library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02111-1301, USA.
 */

#ifndef NETTLE_SEXP_H_INCLUDED
#define NETTLE_SEXP_H_INCLUDED

#include <stdarg.h>
#include "nettle-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Name mangling */
#define sexp_iterator_first nettle_sexp_iterator_first
#define sexp_transport_iterator_first nettle_sexp_transport_iterator_first
#define sexp_iterator_next nettle_sexp_iterator_next
#define sexp_iterator_enter_list nettle_sexp_iterator_enter_list
#define sexp_iterator_exit_list nettle_sexp_iterator_exit_list
#define sexp_iterator_subexpr nettle_sexp_iterator_subexpr
#define sexp_iterator_get_uint32 nettle_sexp_iterator_get_uint32
#define sexp_iterator_check_type nettle_sexp_iterator_check_type
#define sexp_iterator_check_types nettle_sexp_iterator_check_types
#define sexp_iterator_assoc nettle_sexp_iterator_assoc
#define sexp_format nettle_sexp_format
#define sexp_vformat nettle_sexp_vformat
#define sexp_transport_format nettle_sexp_transport_format
#define sexp_transport_vformat nettle_sexp_transport_vformat
#define sexp_token_chars nettle_sexp_token_chars

	enum sexp_type { SEXP_ATOM, SEXP_LIST, SEXP_END };

	struct sexp_iterator {
		unsigned length;
		const uint8_t *buffer;

		/* Points at the start of the current sub expression. */
		unsigned start;
		/* If type is SEXP_LIST, pos points at the start of the current
		 * element. Otherwise, it points at the end. */
		unsigned pos;
		unsigned level;

		enum sexp_type type;

		unsigned display_length;
		const uint8_t *display;

		unsigned atom_length;
		const uint8_t *atom;
	};


/* All these functions return 1 on success, 0 on failure */

/* Initializes the iterator. */
	int
	 sexp_iterator_first(struct sexp_iterator *iterator,
			     unsigned length, const uint8_t * input);

/* NOTE: Decodes the input string in place */
	int
	 sexp_transport_iterator_first(struct sexp_iterator *iterator,
				       unsigned length, uint8_t * input);

	int
	 sexp_iterator_next(struct sexp_iterator *iterator);

/* Current element must be a list. */
	int
	 sexp_iterator_enter_list(struct sexp_iterator *iterator);

/* Skips the rest of the current list */
	int
	 sexp_iterator_exit_list(struct sexp_iterator *iterator);

#if 0
/* Skips out of as many lists as necessary to get back to the given
 * level. */
	int
	 sexp_iterator_exit_lists(struct sexp_iterator *iterator,
				  unsigned level);
#endif

/* Gets start and length of the current subexpression. Implies
 * sexp_iterator_next. */
	const uint8_t *sexp_iterator_subexpr(struct sexp_iterator
					     *iterator, unsigned *length);

	int
	 sexp_iterator_get_uint32(struct sexp_iterator *iterator,
				  uint32_t * x);


/* Checks the type of the current expression, which should be a list
 *
 *  (<type> ...)
 */
	int
	 sexp_iterator_check_type(struct sexp_iterator *iterator,
				  const uint8_t * type);

	const uint8_t *sexp_iterator_check_types(struct sexp_iterator
						 *iterator,
						 unsigned ntypes,
						 const uint8_t *
						 const *types);

/* Current element must be a list. Looks up element of type
 *
 *   (key rest...)
 *
 * For a matching key, the corresponding iterator is initialized
 * pointing at the start of REST.
 *
 * On success, exits the current list.
 */
	int
	 sexp_iterator_assoc(struct sexp_iterator *iterator,
			     unsigned nkeys,
			     const uint8_t * const *keys,
			     struct sexp_iterator *values);


/* Output functions. What is a reasonable API for this? It seems
 * ugly to have to reimplement string streams. */

/* Declared for real in buffer.h */
	struct nettle_buffer;

/* Returns the number of output characters, or 0 on out of memory. If
 * buffer == NULL, just compute length.
 *
 * Format strings can contained matched parentheses, tokens ("foo" in
 * the format string is formatted as "3:foo"), whitespace (which
 * separates tokens but is otherwise ignored) and the following
 * formatting specifiers:
 *
 *   %s   String represented as unsigned length, const uint8_t *data.
 *
 *   %t   Optional display type, represented as
 *        unsigned display_length, const uint8_t *display,
 *        display == NULL means no display type.
 *
 *   %i   Non-negative small integer, uint32_t.
 *
 *   %b   Non-negative bignum, mpz_t.
 *
 *   %l   Literal string (no length added), typically a balanced
 *        subexpression. Represented as unsigned length, const uint8_t
 *        *data.
 *
 *   %(, %)  Allows insertion of unbalanced parenthesis.
 *
 * Modifiers:
 *
 *   %0   For %s, %t and %l, says that there's no length argument,
 *        instead the string is NUL-terminated, and there's only one
 *        const uint8_t * argument.
 */

	unsigned
	 sexp_format(struct nettle_buffer *buffer,
		     const char *format, ...);

	unsigned
	 sexp_vformat(struct nettle_buffer *buffer,
		      const char *format, va_list args);

	unsigned
	 sexp_transport_format(struct nettle_buffer *buffer,
			       const char *format, ...);

	unsigned
	 sexp_transport_vformat(struct nettle_buffer *buffer,
				const char *format, va_list args);

/* Classification for advanced syntax. */
	extern const char
	 sexp_token_chars[0x80];

#define TOKEN_CHAR(c) ((c) < 0x80 && sexp_token_chars[(c)])

#ifdef __cplusplus
}
#endif
#endif				/* NETTLE_SEXP_H_INCLUDED */
