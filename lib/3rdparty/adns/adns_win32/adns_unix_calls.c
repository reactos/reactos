
/*
* adns_unix_calls.c
* - Simple implementation of requiered UNIX system calls and
*   library functions.
*/
/*
*  This file is
*    Copyright (C) 2000, 2002 Jarle (jgaa) Aase <jgaa@jgaa.com>
*
*  It is part of adns, which is
*    Copyright (C) 1997-2000 Ian Jackson <ian@davenant.greenend.org.uk>
*    Copyright (C) 1999 Tony Finch <dot@dotat.at>
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software Foundation,
*  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/


#include "adns.h"

int adns_writev(int FileDescriptor, const struct iovec * iov, int iovCount)
{
	size_t total_len = 0;
	int i = 0, r = 0;
	char *buf = NULL, *p = NULL;

	for(; i < iovCount; i++)
		total_len += iov[i].iov_len;

	p = buf = (char *)alloca(total_len);

	for(; i < iovCount; i++)
	{
		memcpy(p, iov[i].iov_base, iov[i].iov_len);
		p += iov[i].iov_len;
	}

	ADNS_CLEAR_ERRNO
	r = send(FileDescriptor, buf, total_len, 0);
	ADNS_CAPTURE_ERRNO;
	return r;
}

int adns_inet_aton(const char *cp, struct in_addr *inp)
{
    if (!cp || !*cp || !inp)
    {
        errno = EINVAL;
        return -1; 
    }

    if (!strcmp(cp, "255.255.255.255"))
    {
        // Special case
        inp->s_addr = INADDR_NONE;
        return 0;
    }

	inp->s_addr = inet_addr(cp);
    return (inp->s_addr == INADDR_NONE) ? -1 : 0;
}

int adns_getpid()
{
	return GetCurrentProcessId();
}

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	static __int64 Adjustment;
	__int64 now = 0;
	
	if (!Adjustment)
	{
		SYSTEMTIME st = {1970,1,0,1,0,0,0};
		SystemTimeToFileTime(&st, (LPFILETIME)&Adjustment);
	}
	
	if (tz)
	{
		errno = EINVAL;
		return -1;
	}
	
	GetSystemTimeAsFileTime((LPFILETIME)&now);
	now -= Adjustment;
	
	tv->tv_sec = (long)(now / 10000000);
	tv->tv_usec = (long)((now % 10000000) / 10); 

	return 0;
}

/* Memory allocated in the DLL must be freed in the dll, so
   we provide memory manegement functions. */

#ifdef ADNS_DLL

#undef malloc
#undef realloc
#undef free

void *adns_malloc(const size_t bytes)
{
	return malloc(bytes);
}

void *adns_realloc(void *ptr, const size_t bytes)
{
	return realloc(ptr, bytes);
}

void adns_free(void *ptr)
{
	free(ptr);
}

#endif /* ADNS_DLL */
