/*
 * Copyright © 2008 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "main/imports.h"
#include "symbol_table.h"
#include "hash_table.h"

struct symbol {
    /**
     * Link to the next symbol in the table with the same name
     *
     * The linked list of symbols with the same name is ordered by scope
     * from inner-most to outer-most.
     */
    struct symbol *next_with_same_name;


    /**
     * Link to the next symbol in the table with the same scope
     *
     * The linked list of symbols with the same scope is unordered.  Symbols
     * in this list my have unique names.
     */
    struct symbol *next_with_same_scope;


    /**
     * Header information for the list of symbols with the same name.
     */
    struct symbol_header *hdr;


    /**
     * Name space of the symbol
     *
     * Name space are arbitrary user assigned integers.  No two symbols can
     * exist in the same name space at the same scope level.
     */
    int name_space;

    /** Scope depth where this symbol was defined. */
    unsigned depth;

    /**
     * Arbitrary user supplied data.
     */
    void *data;
};


/**
 */
struct symbol_header {
    /** Linkage in list of all headers in a given symbol table. */
    struct symbol_header *next;

    /** Symbol name. */
    char *name;

    /** Linked list of symbols with the same name. */
    struct symbol *symbols;
};


/**
 * Element of the scope stack.
 */
struct scope_level {
    /** Link to next (inner) scope level. */
    struct scope_level *next;
    
    /** Linked list of symbols with the same scope. */
    struct symbol *symbols;
};


/**
 *
 */
struct _mesa_symbol_table {
    /** Hash table containing all symbols in the symbol table. */
    struct hash_table *ht;

    /** Top of scope stack. */
    struct scope_level *current_scope;

    /** List of all symbol headers in the table. */
    struct symbol_header *hdr;

    /** Current scope depth. */
    unsigned depth;
};


struct _mesa_symbol_table_iterator {
    /**
     * Name space of symbols returned by this iterator.
     */
    int name_space;


    /**
     * Currently iterated symbol
     *
     * The next call to \c _mesa_symbol_table_iterator_get will return this
     * value.  It will also update this value to the value that should be
     * returned by the next call.
     */
    struct symbol *curr;
};


static void
check_symbol_table(struct _mesa_symbol_table *table)
{
#if 1
    struct scope_level *scope;

    for (scope = table->current_scope; scope != NULL; scope = scope->next) {
        struct symbol *sym;

        for (sym = scope->symbols
             ; sym != NULL
             ; sym = sym->next_with_same_name) {
            const struct symbol_header *const hdr = sym->hdr;
            struct symbol *sym2;

            for (sym2 = hdr->symbols
                 ; sym2 != NULL
                 ; sym2 = sym2->next_with_same_name) {
                assert(sym2->hdr == hdr);
            }
        }
    }
#endif
}

void
_mesa_symbol_table_pop_scope(struct _mesa_symbol_table *table)
{
    struct scope_level *const scope = table->current_scope;
    struct symbol *sym = scope->symbols;

    table->current_scope = scope->next;
    table->depth--;

    free(scope);

    while (sym != NULL) {
        struct symbol *const next = sym->next_with_same_scope;
        struct symbol_header *const hdr = sym->hdr;

        assert(hdr->symbols == sym);

        hdr->symbols = sym->next_with_same_name;

        free(sym);

        sym = next;
    }

    check_symbol_table(table);
}


void
_mesa_symbol_table_push_scope(struct _mesa_symbol_table *table)
{
    struct scope_level *const scope = calloc(1, sizeof(*scope));
    
    scope->next = table->current_scope;
    table->current_scope = scope;
    table->depth++;
}


static struct symbol_header *
find_symbol(struct _mesa_symbol_table *table, const char *name)
{
    return (struct symbol_header *) hash_table_find(table->ht, name);
}


struct _mesa_symbol_table_iterator *
_mesa_symbol_table_iterator_ctor(struct _mesa_symbol_table *table,
                                 int name_space, const char *name)
{
    struct _mesa_symbol_table_iterator *iter = calloc(1, sizeof(*iter));
    struct symbol_header *const hdr = find_symbol(table, name);
    
    iter->name_space = name_space;

    if (hdr != NULL) {
        struct symbol *sym;

        for (sym = hdr->symbols; sym != NULL; sym = sym->next_with_same_name) {
            assert(sym->hdr == hdr);

            if ((name_space == -1) || (sym->name_space == name_space)) {
                iter->curr = sym;
                break;
            }
        }
    }

    return iter;
}


void
_mesa_symbol_table_iterator_dtor(struct _mesa_symbol_table_iterator *iter)
{
    free(iter);
}


void *
_mesa_symbol_table_iterator_get(struct _mesa_symbol_table_iterator *iter)
{
    return (iter->curr == NULL) ? NULL : iter->curr->data;
}


int
_mesa_symbol_table_iterator_next(struct _mesa_symbol_table_iterator *iter)
{
    struct symbol_header *hdr;

    if (iter->curr == NULL) {
        return 0;
    }

    hdr = iter->curr->hdr;
    iter->curr = iter->curr->next_with_same_name;

    while (iter->curr != NULL) {
        assert(iter->curr->hdr == hdr);
        (void)hdr;

        if ((iter->name_space == -1)
            || (iter->curr->name_space == iter->name_space)) {
            return 1;
        }

        iter->curr = iter->curr->next_with_same_name;
    }

    return 0;
}


/**
 * Determine the scope "distance" of a symbol from the current scope
 *
 * \return
 * A non-negative number for the number of scopes between the current scope
 * and the scope where a symbol was defined.  A value of zero means the current
 * scope.  A negative number if the symbol does not exist.
 */
int
_mesa_symbol_table_symbol_scope(struct _mesa_symbol_table *table,
				int name_space, const char *name)
{
    struct symbol_header *const hdr = find_symbol(table, name);
    struct symbol *sym;

    if (hdr != NULL) {
       for (sym = hdr->symbols; sym != NULL; sym = sym->next_with_same_name) {
	  assert(sym->hdr == hdr);

	  if ((name_space == -1) || (sym->name_space == name_space)) {
	     assert(sym->depth <= table->depth);
	     return sym->depth - table->depth;
	  }
       }
    }

    return -1;
}


void *
_mesa_symbol_table_find_symbol(struct _mesa_symbol_table *table,
                               int name_space, const char *name)
{
    struct symbol_header *const hdr = find_symbol(table, name);

    if (hdr != NULL) {
        struct symbol *sym;


        for (sym = hdr->symbols; sym != NULL; sym = sym->next_with_same_name) {
            assert(sym->hdr == hdr);

            if ((name_space == -1) || (sym->name_space == name_space)) {
                return sym->data;
            }
        }
    }

    return NULL;
}


int
_mesa_symbol_table_add_symbol(struct _mesa_symbol_table *table,
                              int name_space, const char *name,
                              void *declaration)
{
    struct symbol_header *hdr;
    struct symbol *sym;

    check_symbol_table(table);

    hdr = find_symbol(table, name);

    check_symbol_table(table);

    if (hdr == NULL) {
       hdr = calloc(1, sizeof(*hdr));
       hdr->name = strdup(name);

       hash_table_insert(table->ht, hdr, hdr->name);
       hdr->next = table->hdr;
       table->hdr = hdr;
    }

    check_symbol_table(table);

    /* If the symbol already exists in this namespace at this scope, it cannot
     * be added to the table.
     */
    for (sym = hdr->symbols
	 ; (sym != NULL) && (sym->name_space != name_space)
	 ; sym = sym->next_with_same_name) {
       /* empty */
    }

    if (sym && (sym->depth == table->depth))
       return -1;

    sym = calloc(1, sizeof(*sym));
    sym->next_with_same_name = hdr->symbols;
    sym->next_with_same_scope = table->current_scope->symbols;
    sym->hdr = hdr;
    sym->name_space = name_space;
    sym->data = declaration;
    sym->depth = table->depth;

    assert(sym->hdr == hdr);

    hdr->symbols = sym;
    table->current_scope->symbols = sym;

    check_symbol_table(table);
    return 0;
}


int
_mesa_symbol_table_add_global_symbol(struct _mesa_symbol_table *table,
				     int name_space, const char *name,
				     void *declaration)
{
    struct symbol_header *hdr;
    struct symbol *sym;
    struct symbol *curr;
    struct scope_level *top_scope;

    check_symbol_table(table);

    hdr = find_symbol(table, name);

    check_symbol_table(table);

    if (hdr == NULL) {
        hdr = calloc(1, sizeof(*hdr));
        hdr->name = strdup(name);

        hash_table_insert(table->ht, hdr, hdr->name);
        hdr->next = table->hdr;
        table->hdr = hdr;
    }

    check_symbol_table(table);

    /* If the symbol already exists in this namespace at this scope, it cannot
     * be added to the table.
     */
    for (sym = hdr->symbols
	 ; (sym != NULL) && (sym->name_space != name_space)
	 ; sym = sym->next_with_same_name) {
       /* empty */
    }

    if (sym && sym->depth == 0)
       return -1;

    /* Find the top-level scope */
    for (top_scope = table->current_scope
	 ; top_scope->next != NULL
	 ; top_scope = top_scope->next) {
       /* empty */
    }

    sym = calloc(1, sizeof(*sym));
    sym->next_with_same_scope = top_scope->symbols;
    sym->hdr = hdr;
    sym->name_space = name_space;
    sym->data = declaration;

    assert(sym->hdr == hdr);

    /* Since next_with_same_name is ordered by scope, we need to append the
     * new symbol to the _end_ of the list.
     */
    if (hdr->symbols == NULL) {
       hdr->symbols = sym;
    } else {
       for (curr = hdr->symbols
	    ; curr->next_with_same_name != NULL
	    ; curr = curr->next_with_same_name) {
	  /* empty */
       }
       curr->next_with_same_name = sym;
    }
    top_scope->symbols = sym;

    check_symbol_table(table);
    return 0;
}



struct _mesa_symbol_table *
_mesa_symbol_table_ctor(void)
{
    struct _mesa_symbol_table *table = calloc(1, sizeof(*table));

    if (table != NULL) {
       table->ht = hash_table_ctor(32, hash_table_string_hash,
				   hash_table_string_compare);

       _mesa_symbol_table_push_scope(table);
    }

    return table;
}


void
_mesa_symbol_table_dtor(struct _mesa_symbol_table *table)
{
   struct symbol_header *hdr;
   struct symbol_header *next;

   while (table->current_scope != NULL) {
      _mesa_symbol_table_pop_scope(table);
   }

   for (hdr = table->hdr; hdr != NULL; hdr = next) {
       next = hdr->next;
       free(hdr->name);
       free(hdr);
   }

   hash_table_dtor(table->ht);
   free(table);
}
