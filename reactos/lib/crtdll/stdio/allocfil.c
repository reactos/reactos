/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdio.h>
#include <crtdll/string.h>
#include <crtdll/stdlib.h>
#include <crtdll/internal/file.h>


FILE *	__alloc_file(void);


/* A FILE* is considered "free" if its flag is zero. */

FILE *__alloc_file(void)
{
  __file_rec *fr = __file_rec_list;
  __file_rec **last_fr = &__file_rec_list;
  FILE *rv=0;
  int i;
   
  /* Try to find an empty slot */
  while (fr)
  {
    last_fr = &(fr->next);

    /* If one of the existing slots is available, return it */
    for (i=0; i<fr->count; i++)
       {
	  if (fr->files[i]->_flag == 0)
	    {
	       return fr->files[i];
	    }
       }

    /* If this one is full, go to the next */
    if (fr->count == __FILE_REC_MAX)
      fr = fr->next;
    else
      /* it isn't full, we can add to it */
      break;
  }
  if (!fr)
  {
    /* add another one to the end, make it empty */
    fr = *last_fr = (__file_rec *)malloc(sizeof(__file_rec));
    if (fr == 0)
      return 0;
    fr->next = 0;
    fr->count = 0;
  }
  /* fr is a pointer to a rec with empty slots in it */
  rv = fr->files[fr->count] = (FILE *)malloc(sizeof(FILE));
  if (rv == 0)
    return 0;
  memset(rv, 0, sizeof(FILE));
  fr->count ++;
  return rv;
}


int _fcloseall( void )
{
   __file_rec *fr = __file_rec_list;
   __file_rec **last_fr = &__file_rec_list;
  
   int total_closed = 0;
   int i = 0;

   /* Try to find an empty slot */
   while (fr)
     {
	last_fr = &(fr->next);
	
	/* If one of the existing slots is available, return it */
	for (i=0; i<fr->count; i++)
	  if (fr->files[i]->_flag != 0) {
	     fclose(fr->files[i]);
	     total_closed++;
	  }
	
	/* If this one is full, go to the next */
	if (fr->count == __FILE_REC_MAX)
	  fr = fr->next;
	else
	  /* it isn't full, we can add to it */
	  break;
     }
   return total_closed; 
}
