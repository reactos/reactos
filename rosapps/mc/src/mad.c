/* The Memory Allocation Debugging system
   Copyright (C) 1994 Janne Kukonlehto.

   To use MAD define HAVE_MAD and include "mad.h" in all the *.c files.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <config.h>
#include "mad.h"
#undef malloc
#undef calloc
#undef realloc
#undef xmalloc
#undef strdup
#undef free
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>	/* For kill() */
#ifdef HAVE_UNISTD_H
#   include <unistd.h>	/* For getpid() */
#endif

/* Here to avoid non empty translation units */
#ifdef HAVE_MAD

/* Maximum number of memory area handles,
   increase this if you run out of handles */
#define MAD_MAX_AREAS 3000
/* Maximum file name length */
#define MAD_MAX_FILE 50
/* Signature for detecting overwrites */
#define MAD_SIGNATURE (('M'<<24)|('a'<<16)|('d'<<8)|('S'))

typedef struct {
    int in_use;
    long *start_sig;
    char file [MAD_MAX_FILE];
    int line;
    void *data;
    long *end_sig;
} mad_mem_area;

static mad_mem_area mem_areas [MAD_MAX_AREAS];
void *watch_free_pointer = 0;

/* This function is only called by the mad_check function */
static void mad_abort (char *message, int area, char *file, int line)
{
    fprintf (stderr, "MAD: %s in area %d.\r\n", message, area);
    fprintf (stderr, "     Allocated in file \"%s\" at line %d.\r\n",
	     mem_areas [area].file, mem_areas [area].line);
    fprintf (stderr, "     Discovered in file \"%s\" at line %d.\r\n",
	     file, line);
    fprintf (stderr, "MAD: Core dumping...\r\n");
    kill (getpid (), 3);
}

/* Checks all the allocated memory areas.
   This is called everytime memory is allocated or freed.
   You can also call it anytime you think memory might be corrupted. */
void mad_check (char *file, int line)
{
    int i;

    for (i = 0; i < MAD_MAX_AREAS; i++){
 	if (! mem_areas [i].in_use)
	    continue;
	if (*(mem_areas [i].start_sig) != MAD_SIGNATURE)
	    mad_abort ("Overwrite error: Bad start signature", i, file, line);
	if (*(mem_areas [i].end_sig) != MAD_SIGNATURE)
	    mad_abort ("Overwrite error: Bad end signature", i, file, line);
    }
}

/* Allocates a memory area. Used instead of malloc and calloc. */
void *mad_alloc (int size, char *file, int line)
{
    int i;
    char *area;

    mad_check (file, line);

    for (i = 0; i < MAD_MAX_AREAS; i++){
	if (! mem_areas [i].in_use)
	    break;
    }
    if (i >= MAD_MAX_AREAS){
	fprintf (stderr, "MAD: Out of memory area handles. Increase the value of MAD_MAX_AREAS.\r\n");
	fprintf (stderr, "     Discovered in file \"%s\" at line %d.\r\n",
		 file, line);
	fprintf (stderr, "MAD: Aborting...\r\n");
	abort ();
    }

    mem_areas [i].in_use = 1;
    size = (size + 3) & (~3); /* Alignment */
    area = (char*) malloc (size + 2 * sizeof (long));
    if (!area){
	fprintf (stderr, "MAD: Out of memory.\r\n");
	fprintf (stderr, "     Discovered in file \"%s\" at line %d.\r\n",
		 file, line);
	fprintf (stderr, "MAD: Aborting...\r\n");
	abort ();
    }

    mem_areas [i].start_sig = (long*) area;
    mem_areas [i].data = (area + sizeof (long));
    mem_areas [i].end_sig = (long*) (area + size + sizeof (long));
    *(mem_areas [i].start_sig) = MAD_SIGNATURE;
    *(mem_areas [i].end_sig) = MAD_SIGNATURE;

    if (strlen (file) >= MAD_MAX_FILE)
	file [MAD_MAX_FILE - 1] = 0;
    strcpy (mem_areas [i].file, file);
    mem_areas [i].line = line;

    return mem_areas [i].data;
}

/* Reallocates a memory area. Used instead of realloc. */
void *mad_realloc (void *ptr, int newsize, char *file, int line)
{
    int i;
    char *area;

    if (!ptr)
        return (mad_alloc (newsize, file, line));

    mad_check (file, line);

    for (i = 0; i < MAD_MAX_AREAS; i++){
 	if (! mem_areas [i].in_use)
	    continue;
	if (mem_areas [i].data == ptr)
	    break;
    }
    if (i >= MAD_MAX_AREAS){
	fprintf (stderr, "MAD: Attempted to realloc unallocated pointer: %p.\r\n", ptr);
	fprintf (stderr, "     Discovered in file \"%s\" at line %d.\r\n",
		 file, line);
	fprintf (stderr, "MAD: Aborting...\r\n");
	abort ();
    }

    newsize = (newsize + 3) & (~3); /* Alignment */
    area = (char*) realloc (mem_areas [i].start_sig, newsize + 2 * sizeof (long));
    if (!area){
	fprintf (stderr, "MAD: Out of memory.\r\n");
	fprintf (stderr, "     Discovered in file \"%s\" at line %d.\r\n",
		 file, line);
	fprintf (stderr, "MAD: Aborting...\r\n");
	abort ();
    }

    mem_areas [i].start_sig = (long*) area;
    mem_areas [i].data = (area + sizeof (long));
    mem_areas [i].end_sig = (long*) (area + newsize + sizeof (long));
    *(mem_areas [i].start_sig) = MAD_SIGNATURE;
    *(mem_areas [i].end_sig) = MAD_SIGNATURE;

    if (strlen (file) >= MAD_MAX_FILE)
	file [MAD_MAX_FILE - 1] = 0;
    strcpy (mem_areas [i].file, file);
    mem_areas [i].line = line;

    return mem_areas [i].data;
}

/* Duplicates a character string. Used instead of strdup. */
char *mad_strdup (const char *s, char *file, int line)
{
    char *t;

    t = (char *) mad_alloc (strlen (s) + 1, file, line);
    strcpy (t, s);
    return t;
}

/* Frees a memory area. Used instead of free. */
void mad_free (void *ptr, char *file, int line)
{
    int i;
    
    mad_check (file, line);

    if (watch_free_pointer && ptr == watch_free_pointer){
	printf ("watch free pointer found\n");
    }
    
    if (ptr == NULL){
	fprintf (stderr, "MAD: Attempted to free a NULL pointer in file \"%s\" at line %d.\n",
		 file, line);
	return;
    }

    for (i = 0; i < MAD_MAX_AREAS; i++){
 	if (! mem_areas [i].in_use)
	    continue;
	if (mem_areas [i].data == ptr)
	    break;
    }
    if (i >= MAD_MAX_AREAS){
	fprintf (stderr, "MAD: Attempted to free an unallocated pointer: %p.\r\n", ptr);
	fprintf (stderr, "     Discovered in file \"%s\" at line %d.\r\n",
		 file, line);
	fprintf (stderr, "MAD: Aborting...\r\n");
	abort ();
    }

    free (mem_areas [i].start_sig);
    mem_areas [i].in_use = 0;
}

/* Outputs a list of unfreed memory areas,
   to be called as a last thing before exiting */
void mad_finalize (char *file, int line)
{
    int i;
    
    mad_check (file, line);

    /* Following can be commented out if you don't want to see the
       memory leaks of the Midnight Commander */
#if 1
    for (i = 0; i < MAD_MAX_AREAS; i++){
 	if (! mem_areas [i].in_use)
	    continue;
	fprintf (stderr, "MAD: Unfreed pointer: %p.\n", mem_areas [i].data);
	fprintf (stderr, "     Allocated in file \"%s\" at line %d.\r\n",
		 mem_areas [i].file, mem_areas [i].line);
	fprintf (stderr, "     Discovered in file \"%s\" at line %d.\r\n",
		 file, line);
    }
#endif
}

#endif /* HAVE_MAD */
