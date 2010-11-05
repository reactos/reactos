/* lfn.h  -  Functions for handling VFAT long filenames */

/* Written 1998 by Roman Hodek */


#ifndef _LFN_H
#define _LFN_H

void lfn_reset( void );
/* Reset the state of the LFN parser. */

void lfn_add_slot( DIR_ENT *de, loff_t dir_offset );
/* Process a dir slot that is a VFAT LFN entry. */

char *lfn_get( DIR_ENT *de );
/* Retrieve the long name for the proper dir entry. */

void lfn_check_orphaned(void);

#endif
