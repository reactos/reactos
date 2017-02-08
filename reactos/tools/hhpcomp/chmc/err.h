/*

  Copyright (C) 2010 Alex Andreotti <alex.andreotti@gmail.com>

  This file is part of chmc.

  chmc is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  chmc is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with chmc.  If not, see <http://www.gnu.org/licenses/>.

*/
#ifndef CHMC_ERR_H
#define CHMC_ERR_H

#include <stdio.h>
#define chmcerr_printf(fmt, ...) fprintf (stderr, fmt , ## __VA_ARGS__)

#include <stdlib.h>
#define BUG_ON(fmt, ...)	  \
	do { \
		fprintf (stderr, "%s:%d: ", __FILE__, __LINE__); \
		fprintf (stderr, fmt , ## __VA_ARGS__); \
		abort (); \
	} while (0)

#define CHMC_ERRMAXLEN (1023)

#include <errno.h>

#define CHMC_NOERR (0)
#define CHMC_ENOMEM (ENOMEM)
#define CHMC_EINVAL (EINVAL)

void chmcerr_set(int code, const char *fmt, ...);
void chmcerr_clean(void);
int chmcerr_code(void);
const char *chmcerr_message(void);

#define chmc_error(fmt, ...) fprintf (stdout, fmt , ## __VA_ARGS__)

#define chmcerr_return_msg(fmt,...)	  \
	do { \
		chmcerr_printf ( "%s: %d: ", __FILE__, __LINE__ ); \
		chmcerr_printf ( "error %d: ", chmcerr_code () ); \
		chmcerr_printf ( fmt , ## __VA_ARGS__ ); \
		chmcerr_printf ( ": %s\n", chmcerr_message () ); \
		return chmcerr_code (); \
	} while (0)

#define chmcerr_msg(fmt,...)	  \
	do { \
		chmcerr_printf ("%s: %d: ", __FILE__, __LINE__); \
		chmcerr_printf ("error %d: ", chmcerr_code ()); \
		chmcerr_printf (fmt , ## __VA_ARGS__ ); \
		chmcerr_printf (": %s\n", chmcerr_message ()); \
	} while (0)

#define chmcerr_set_return(code,fmt,...)	  \
	do { \
		chmcerr_set ( (code), (fmt), ## __VA_ARGS__ ); \
		return (code); \
	} while (0)

#endif /* CHMC_ERR_H */
