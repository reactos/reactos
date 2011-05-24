/*
	stringbuf: mimicking a bit of C++ to more safely handle strings

	copyright 2006-8 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis
*/

#include "mpg123lib_intern.h"
#include "config.h"
#include "mpg123.h"
#include "compat.h"
#include <string.h>
#include "debug.h"

void attribute_align_arg mpg123_init_string(mpg123_string* sb)
{
	sb->p = NULL;
	sb->size = 0;
	sb->fill = 0;
}

void attribute_align_arg mpg123_free_string(mpg123_string* sb)
{
	if(sb->p != NULL) free(sb->p);
	mpg123_init_string(sb);
}

int attribute_align_arg mpg123_grow_string(mpg123_string* sb, size_t new)
{
	if(sb->size < new) return mpg123_resize_string(sb, new);
	else return 1;
}

int attribute_align_arg mpg123_resize_string(mpg123_string* sb, size_t new)
{
	debug3("resizing string pointer %p from %lu to %lu", (void*) sb->p, (unsigned long)sb->size, (unsigned long)new);
	if(new == 0)
	{
		if(sb->size && sb->p != NULL) free(sb->p);
		mpg123_init_string(sb);
		return 1;
	}
	if(sb->size != new)
	{
		char* t;
		debug("really!");
		t = (char*) safe_realloc(sb->p, new*sizeof(char));
		debug1("safe_realloc returned %p", (void*) t); 
		if(t != NULL)
		{
			sb->p = t;
			sb->size = new;
			return 1;
		}
		else return 0;
	}
	else return 1; /* success */
}

int attribute_align_arg mpg123_copy_string(mpg123_string* from, mpg123_string* to)
{
	size_t fill;
	char  *text;
	if(to == NULL) return -1;

	debug2("called copy_string with %p -> %p", (void*)from, (void*)to);
	if(from == NULL)
	{
		fill = 0;
		text = NULL;
	}
	else
	{
		fill = from->fill;
		text = from->p;
	}

	if(mpg123_resize_string(to, fill))
	{
		memcpy(to->p, text, fill);
		to->fill = fill;
		return 1;
	}
	else return 0;
}

int attribute_align_arg mpg123_add_string(mpg123_string* sb, const char* stuff)
{
	debug1("adding %s", stuff);
	return mpg123_add_substring(sb, stuff, 0, strlen(stuff));
}

int attribute_align_arg mpg123_add_substring(mpg123_string *sb, const char *stuff, size_t from, size_t count)
{
	debug("adding a substring");
	if(sb->fill) /* includes zero byte... */
	{
		if( (SIZE_MAX - sb->fill >= count) /* Avoid overflow. */
		    && (sb->size >= sb->fill+count || mpg123_grow_string(sb, sb->fill+count)) )
		{
			memcpy(sb->p+sb->fill-1, stuff+from, count);
			sb->fill += count;
			sb->p[sb->fill-1] = 0; /* Terminate! */
		}
		else return 0;
	}
	else
	{
		if( count < SIZE_MAX && mpg123_grow_string(sb, count+1) )
		{
			memcpy(sb->p, stuff+from, count);
			sb->fill = count+1;
			sb->p[sb->fill-1] = 0; /* Terminate! */
		}
		else return 0;
	}
	return 1;
}

int attribute_align_arg mpg123_set_substring(mpg123_string* sb, const char* stuff, size_t from, size_t count)
{
	sb->fill = 0;
	return mpg123_add_substring(sb, stuff, from, count);
}

int attribute_align_arg mpg123_set_string(mpg123_string* sb, const char* stuff)
{
	sb->fill = 0;
	return mpg123_add_string(sb, stuff);
}
