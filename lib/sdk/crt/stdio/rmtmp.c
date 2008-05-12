/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/msvcrt/stdio/rmtmp.c
 * PURPOSE:         remove temporary files in current directory
 * PROGRAMMER:      Ariadne
 * UPDATE HISTORY:
 *                  Created 19/01/99
 * NOTE		    Not tested.
 */
#include <precomp.h>

// should be replace by a closure of the tmp files
extern __file_rec *__file_rec_list;

int _rmtmp( void )
{
/*
loop files and check for _tmpfname
*/
  __file_rec *fr = __file_rec_list;
  __file_rec **last_fr = &__file_rec_list;

  int total_closed = 0;
  int i = 0;
  char temp_name[260];

  /* Try to find an empty slot */
  while (fr)
  {
    last_fr = &(fr->next);

    /* If one of the existing slots is available, return it */
    for (i=0; i<fr->count; i++) {
      if (fr->files[i]->_tmpfname  != NULL) {
      if ( _access(fr->files[i]->_tmpfname  ,W_OK) ) {
         strcpy(temp_name,fr->files[i]->_tmpfname  );
			fclose(fr->files[i]);
			remove(temp_name);
			total_closed++;
		}
	  }
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
