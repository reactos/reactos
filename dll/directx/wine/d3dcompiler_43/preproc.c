/*
 * Copyright 1998 Bertho A. Stultiens (BS)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "wpp_private.h"

struct pp_status pp_status;

#define HASHKEY		2039

typedef struct pp_def_state
{
    struct pp_def_state *next;
    pp_entry_t          *defines[HASHKEY];
} pp_def_state_t;

static pp_def_state_t *pp_def_state;

#define MAXIFSTACK	64
static pp_if_state_t if_stack[MAXIFSTACK];
static int if_stack_idx = 0;

void *pp_xmalloc(size_t size)
{
    void *res;

    assert(size > 0);
    res = malloc(size);
    if(res == NULL)
    {
        /* Set the error flag */
        pp_status.state = 1;
    }
    return res;
}

void *pp_xrealloc(void *p, size_t size)
{
    void *res;

    assert(size > 0);
    res = realloc(p, size);
    if(res == NULL)
    {
        /* Set the error flag */
        pp_status.state = 1;
    }
    return res;
}

char *pp_xstrdup(const char *str)
{
	char *s;
	int len;

	assert(str != NULL);
	len = strlen(str)+1;
	s = pp_xmalloc(len);
	if(!s)
		return NULL;
	return memcpy(s, str, len);
}

/* Don't comment on the hash, it's primitive but functional... */
static int pphash(const char *str)
{
	int sum = 0;
	while(*str)
		sum += *str++;
	return sum % HASHKEY;
}

pp_entry_t *pplookup(const char *ident)
{
	int idx;
	pp_entry_t *ppp;

	if(!ident)
		return NULL;
	idx = pphash(ident);
	for(ppp = pp_def_state->defines[idx]; ppp; ppp = ppp->next)
	{
		if(!strcmp(ident, ppp->ident))
			return ppp;
	}
	return NULL;
}

static void free_pp_entry( pp_entry_t *ppp, int idx )
{
	if(ppp->iep)
	{
		if(ppp->iep == pp_includelogiclist)
		{
			pp_includelogiclist = ppp->iep->next;
			if(pp_includelogiclist)
				pp_includelogiclist->prev = NULL;
		}
		else
		{
			ppp->iep->prev->next = ppp->iep->next;
			if(ppp->iep->next)
				ppp->iep->next->prev = ppp->iep->prev;
		}
		free(ppp->iep->filename);
		free(ppp->iep);
	}

	if(pp_def_state->defines[idx] == ppp)
	{
		pp_def_state->defines[idx] = ppp->next;
		if(pp_def_state->defines[idx])
			pp_def_state->defines[idx]->prev = NULL;
	}
	else
	{
		ppp->prev->next = ppp->next;
		if(ppp->next)
			ppp->next->prev = ppp->prev;
	}

	free(ppp);
}

/* push a new (empty) define state */
int pp_push_define_state(void)
{
    pp_def_state_t *state = pp_xmalloc( sizeof(*state) );
    if(!state)
        return 1;

    memset( state->defines, 0, sizeof(state->defines) );
    state->next = pp_def_state;
    pp_def_state = state;
    return 0;
}

/* pop the current define state */
void pp_pop_define_state(void)
{
    int i;
    pp_entry_t *ppp;
    pp_def_state_t *state;

    for (i = 0; i < HASHKEY; i++)
    {
        while ((ppp = pp_def_state->defines[i]) != NULL) pp_del_define( ppp->ident );
    }
    state = pp_def_state;
    pp_def_state = state->next;
    free( state );
}

void pp_del_define(const char *name)
{
	pp_entry_t *ppp;
	int idx = pphash(name);

	if((ppp = pplookup(name)) == NULL)
	{
		if(pp_status.pedantic)
			ppy_warning("%s was not defined", name);
		return;
	}

	free( ppp->ident );
	free( ppp->subst.text );
	free( ppp->filename );
	free_pp_entry( ppp, idx );
}

pp_entry_t *pp_add_define(const char *def, const char *text)
{
	int len;
	char *cptr;
	int idx;
	pp_entry_t *ppp;

	if(!def)
		return NULL;
	idx = pphash(def);
	if((ppp = pplookup(def)) != NULL)
	{
		if(pp_status.pedantic)
			ppy_warning("Redefinition of %s\n\tPrevious definition: %s:%d", def, ppp->filename, ppp->linenumber);
		pp_del_define(def);
	}
	ppp = pp_xmalloc(sizeof(pp_entry_t));
	if(!ppp)
		return NULL;
	memset( ppp, 0, sizeof(*ppp) );
	ppp->ident = pp_xstrdup(def);
	if(!ppp->ident)
		goto error;
	ppp->type = def_define;
	ppp->subst.text = text ? pp_xstrdup(text) : NULL;
	if(text && !ppp->subst.text)
		goto error;
	ppp->filename = pp_xstrdup(pp_status.input ? pp_status.input : "<internal or cmdline>");
	if(!ppp->filename)
		goto error;
	ppp->linenumber = pp_status.input ? pp_status.line_number : 0;
	ppp->next = pp_def_state->defines[idx];
	pp_def_state->defines[idx] = ppp;
	if(ppp->next)
		ppp->next->prev = ppp;
	if(ppp->subst.text)
	{
		/* Strip trailing white space from subst text */
		len = strlen(ppp->subst.text);
		while(len && strchr(" \t\r\n", ppp->subst.text[len-1]))
		{
			ppp->subst.text[--len] = '\0';
		}
		/* Strip leading white space from subst text */
		for(cptr = ppp->subst.text; *cptr && strchr(" \t\r", *cptr); cptr++)
		;
		if(ppp->subst.text != cptr)
			memmove(ppp->subst.text, cptr, strlen(cptr)+1);
	}
	return ppp;

error:
	free(ppp->ident);
	free(ppp->subst.text);
	free(ppp);
	return NULL;
}

pp_entry_t *pp_add_macro(char *id, marg_t *args[], int nargs, mtext_t *exp)
{
	int idx;
	pp_entry_t *ppp;

	if(!id)
		return NULL;
	idx = pphash(id);
	if((ppp = pplookup(id)) != NULL)
	{
		if(pp_status.pedantic)
			ppy_warning("Redefinition of %s\n\tPrevious definition: %s:%d", id, ppp->filename, ppp->linenumber);
		pp_del_define(id);
	}
	ppp = pp_xmalloc(sizeof(pp_entry_t));
	if(!ppp)
		return NULL;
	memset( ppp, 0, sizeof(*ppp) );
	ppp->ident	= id;
	ppp->type	= def_macro;
	ppp->margs	= args;
	ppp->nargs	= nargs;
	ppp->subst.mtext= exp;
	ppp->filename = pp_xstrdup(pp_status.input ? pp_status.input : "<internal or cmdline>");
	if(!ppp->filename)
	{
		free(ppp);
		return NULL;
	}
	ppp->linenumber = pp_status.input ? pp_status.line_number : 0;
	ppp->next	= pp_def_state->defines[idx];
	pp_def_state->defines[idx] = ppp;
	if(ppp->next)
		ppp->next->prev = ppp;
	return ppp;
}


void *pp_open_include(const char *name, int type, const char *parent_name, char **newpath)
{
    char *path;
    void *fp;

    if (!(path = wpp_lookup(name, type, parent_name))) return NULL;
    fp = wpp_open(path, type);

    if (fp)
    {
        if (newpath) *newpath = path;
        else free( path );
        return fp;
    }
    free( path );
    return NULL;
}

/*
 *-------------------------------------------------------------------------
 * #if, #ifdef, #ifndef, #else, #elif and #endif state management
 *
 * #if state transitions are made on basis of the current TOS and the next
 * required state. The state transitions are required to housekeep because
 * #if:s can be nested. The ignore case is activated to prevent output from
 * within a false clause.
 * Some special cases come from the fact that the #elif cases are not
 * binary, but three-state. The problem is that all other elif-cases must
 * be false when one true one has been found. A second problem is that the
 * #else clause is a final clause. No extra #else:s may follow.
 *
 * The states mean:
 * if_true	Process input to output
 * if_false	Process input but no output
 * if_ignore	Process input but no output
 * if_elif	Process input but no output
 * if_elsefalse	Process input but no output
 * if_elsettrue	Process input to output
 *
 * The possible state-sequences are [state(stack depth)] (rest can be deduced):
 *	TOS		#if 1		#else			#endif
 *	if_true(n)	if_true(n+1)	if_elsefalse(n+1)
 *	if_false(n)	if_ignore(n+1)	if_ignore(n+1)
 *	if_elsetrue(n)	if_true(n+1)	if_elsefalse(n+1)
 *	if_elsefalse(n)	if_ignore(n+1)	if_ignore(n+1)
 *	if_elif(n)	if_ignore(n+1)	if_ignore(n+1)
 *	if_ignore(n)	if_ignore(n+1)	if_ignore(n+1)
 *
 *	TOS		#if 1		#elif 0		#else		#endif
 *	if_true(n)	if_true(n+1)	if_elif(n+1)	if_elif(n+1)
 *	if_false(n)	if_ignore(n+1)	if_ignore(n+1)	if_ignore(n+1)
 *	if_elsetrue(n)	if_true(n+1)	if_elif(n+1)	if_elif(n+1)
 *	if_elsefalse(n)	if_ignore(n+1)	if_ignore(n+1)	if_ignore(n+1)
 *	if_elif(n)	if_ignore(n+1)	if_ignore(n+1)	if_ignore(n+1)
 *	if_ignore(n)	if_ignore(n+1)	if_ignore(n+1)	if_ignore(n+1)
 *
 *	TOS		#if 0		#elif 1		#else		#endif
 *	if_true(n)	if_false(n+1)	if_true(n+1)	if_elsefalse(n+1)
 *	if_false(n)	if_ignore(n+1)	if_ignore(n+1)	if_ignore(n+1)
 *	if_elsetrue(n)	if_false(n+1)	if_true(n+1)	if_elsefalse(n+1)
 *	if_elsefalse(n)	if_ignore(n+1)	if_ignore(n+1)	if_ignore(n+1)
 *	if_elif(n)	if_ignore(n+1)	if_ignore(n+1)	if_ignore(n+1)
 *	if_ignore(n)	if_ignore(n+1)	if_ignore(n+1)	if_ignore(n+1)
 *
 *-------------------------------------------------------------------------
 */

void pp_push_if(pp_if_state_t s)
{
	if(if_stack_idx >= MAXIFSTACK)
		pp_internal_error(__FILE__, __LINE__, "#if-stack overflow; #{if,ifdef,ifndef} nested too deeply (> %d)", MAXIFSTACK);

	if_stack[if_stack_idx++] = s;

	switch(s)
	{
	case if_true:
	case if_elsetrue:
		break;
	case if_false:
	case if_elsefalse:
	case if_elif:
	case if_ignore:
		pp_push_ignore_state();
		break;
	default:
		pp_internal_error(__FILE__, __LINE__, "Invalid pp_if_state (%d)", (int)pp_if_state());
	}
}

pp_if_state_t pp_pop_if(void)
{
	if(if_stack_idx <= 0)
	{
		ppy_error("#{endif,else,elif} without #{if,ifdef,ifndef} (#if-stack underflow)");
		return if_error;
	}

	switch(pp_if_state())
	{
	case if_true:
	case if_elsetrue:
		break;
	case if_false:
	case if_elsefalse:
	case if_elif:
	case if_ignore:
		pp_pop_ignore_state();
		break;
	default:
		pp_internal_error(__FILE__, __LINE__, "Invalid pp_if_state (%d)", (int)pp_if_state());
	}
	return if_stack[--if_stack_idx];
}

pp_if_state_t pp_if_state(void)
{
	if(!if_stack_idx)
		return if_true;
	else
		return if_stack[if_stack_idx-1];
}


void pp_next_if_state(int i)
{
	switch(pp_if_state())
	{
	case if_true:
	case if_elsetrue:
		pp_push_if(i ? if_true : if_false);
		break;
	case if_false:
	case if_elsefalse:
	case if_elif:
	case if_ignore:
		pp_push_if(if_ignore);
		break;
	default:
		pp_internal_error(__FILE__, __LINE__, "Invalid pp_if_state (%d) in #{if,ifdef,ifndef} directive", (int)pp_if_state());
	}
}

int pp_get_if_depth(void)
{
	return if_stack_idx;
}

void WINAPIV pp_internal_error(const char *file, int line, const char *s, ...)
{
	__ms_va_list ap;
	__ms_va_start(ap, s);
	fprintf(stderr, "Internal error (please report) %s %d: ", file, line);
	vfprintf(stderr, s, ap);
	fprintf(stderr, "\n");
	__ms_va_end(ap);
	exit(3);
}
