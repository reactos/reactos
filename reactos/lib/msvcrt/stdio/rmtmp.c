/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/crtdll/stdio/rmtmp.c
 * PURPOSE:         remove temporary files in current directory
 * PROGRAMMER:      Boudewijn ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 19/01/99
 * NOTE		    Not tested.
 */

#include <msvcrt/stdio.h>
#include <msvcrt/string.h>
#include <msvcrt/internal/file.h>

#ifndef F_OK
 #define F_OK	0x01
#endif
#ifndef R_OK
 #define R_OK	0x02
#endif
#ifndef W_OK
 #define W_OK	0x04
#endif
#ifndef X_OK
 #define X_OK	0x08
#endif
#ifndef D_OK
 #define D_OK	0x10
#endif

// should be replace by a closure of the tmp files
extern __file_rec *__file_rec_list;

int _rmtmp( void )
{
/*
loop files and check for name_to_remove 
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
      if (fr->files[i]->_name_to_remove != NULL) {
		if ( _access(fr->files[i]->_name_to_remove,W_OK) ) {
			strcpy(temp_name,fr->files[i]->_name_to_remove);
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
