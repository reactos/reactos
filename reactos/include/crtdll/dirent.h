/*
 * DIRENT.H (formerly DIRLIB.H)
 *
 * by M. J. Weinstein   Released to public domain 1-Jan-89
 *
 * Because I have heard that this feature (opendir, readdir, closedir)
 * it so useful for programmers coming from UNIX or attempting to port
 * UNIX code, and because it is reasonably light weight, I have included
 * it in the Mingw32 package.
 *   - Colin Peters <colin@bird.fu.is.saga-u.ac.jp>
 *
 *  This code is distributed in the hope that is will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includeds but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.2 $
 * $Author: rex $
 * $Date: 1999/03/19 05:55:07 $
 *
 */

#ifndef	_STRICT_ANSI

#ifndef _DIRENT_H_
#define _DIRENT_H_

#include <dir.h>

#ifdef __cplusplus
extern "C" {
#endif

struct dirent
{
	long		d_ino;
	unsigned short	d_reclen;
	unsigned short	d_namlen;
	char		d_name[FILENAME_MAX+1];
};

typedef struct
{
	struct _finddata_t	dd_dta;		/* disk transfer area for this dir */
	struct dirent		dd_dir;		/* dirent struct to return from dir */
	long			dd_handle;	/* _findnext handle */
	short			dd_stat;	/* status return from last lookup */
	char			dd_name[1];	/* full name of file (struct is extended */
} DIR;

DIR*		opendir (const char* szPath);
struct dirent*	readdir (DIR* dir);
int		closedir (DIR* dir);

#ifdef	__cplusplus
}
#endif

#endif _DIRENT_H_

#endif /* Not _STRICT_ANSI */
