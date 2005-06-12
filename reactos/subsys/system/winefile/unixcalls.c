/*
 * Winefile
 *
 * Copyright 2004 Martin Fuchs
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef __WINE__

#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>


void call_getcwd(char* buffer, size_t len)
{
	getcwd(buffer, len);
}


#ifndef _NO_EXTENSIONS

/* proxy functions to call UNIX readdir() */

void* call_opendir(const char* path)
{
	DIR* pdir = opendir(path);

	return pdir;
}

int call_readdir(void* pdir, char* name, unsigned* inode)
{
	struct dirent* ent = readdir((DIR*)pdir);

	if (!ent)
		return 0;

	strcpy(name, ent->d_name);
	*inode = ent->d_ino;

	return 1;
}

void call_closedir(void* pdir)
{
	closedir((DIR*)pdir);
}


/* proxy function to call UNIX stat() */
int call_stat(
	const char* path, int* pis_dir,
	unsigned long* psize_low, unsigned long* psize_high,
	time_t* patime, time_t* pmtime,
	unsigned long* plinks
)
{
	struct stat st;

	if (stat(path, &st))
		return 1;

	*pis_dir = S_ISDIR(st.st_mode);
	*psize_low = st.st_size & 0xFFFFFFFF;
	*psize_high = 0; /*st.st_size >> 32;*/
	*patime = st.st_atime;
	*pmtime = st.st_mtime;

	return 0;
}

#endif /* _NO_EXTENSIONS */

#endif /* __WINE__ */
