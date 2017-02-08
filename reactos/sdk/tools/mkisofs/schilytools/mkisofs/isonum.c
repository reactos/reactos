/* @(#)isonum.c	1.9 10/12/19 Copyright 2006-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)isonum.c	1.9 10/12/19 Copyright 2006-2010 J. Schilling";
#endif
/*
 *	Copyright (c) 2006-2010 J. Schilling
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; see the file COPYING.  If not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "mkisofs.h"


EXPORT	void	set_721		__PR((void *vp, UInt32_t i));
EXPORT	void	set_722		__PR((void *vp, UInt32_t i));
EXPORT	void	set_723		__PR((void *vp, UInt32_t i));
EXPORT	void	set_731		__PR((void *vp, UInt32_t i));
EXPORT	void	set_732		__PR((void *vp, UInt32_t i));
EXPORT	void	set_733		__PR((void *vp, UInt32_t i));
EXPORT	UInt32_t get_711	__PR((void *vp));
EXPORT	UInt32_t get_721	__PR((void *vp));
EXPORT	UInt32_t get_723	__PR((void *vp));
EXPORT	UInt32_t get_731	__PR((void *vp));
EXPORT	UInt32_t get_732	__PR((void *vp));
EXPORT	UInt32_t get_733	__PR((void *vp));

/*
 * ISO-9660 7.2.1
 * Set 16 bit unsigned int, store Least significant byte first
 */
EXPORT void
set_721(vp, i)
	void		*vp;
	UInt32_t	i;
{
	Uchar	*p = vp;

	p[0] = i & 0xff;
	p[1] = (i >> 8) & 0xff;
}

/*
 * ISO-9660 7.2.2
 * Set 16 bit unsigned int, store Most significant byte first
 */
EXPORT void
set_722(vp, i)
	void		*vp;
	UInt32_t	i;
{
	Uchar	*p = vp;

	p[0] = (i >> 8) & 0xff;
	p[1] = i & 0xff;
}

/*
 * ISO-9660 7.2.3
 * Set 16 bit unsigned int, store Both Byte orders
 */
EXPORT void
set_723(vp, i)
	void		*vp;
	UInt32_t	i;
{
	Uchar	*p = vp;

	p[3] = p[0] = i & 0xff;
	p[2] = p[1] = (i >> 8) & 0xff;
}

/*
 * ISO-9660 7.3.1
 * Set 32 bit unsigned int, store Least significant byte first
 */
EXPORT void
set_731(vp, i)
	void		*vp;
	UInt32_t	i;
{
	Uchar	*p = vp;

	p[0] = i & 0xff;
	p[1] = (i >> 8) & 0xff;
	p[2] = (i >> 16) & 0xff;
	p[3] = (i >> 24) & 0xff;
}

/*
 * ISO-9660 7.3.2
 * Set 32 bit unsigned int, store Most significant byte first
 */
EXPORT void
set_732(vp, i)
	void		*vp;
	UInt32_t	i;
{
	Uchar	*p = vp;

	p[3] = i & 0xff;
	p[2] = (i >> 8) & 0xff;
	p[1] = (i >> 16) & 0xff;
	p[0] = (i >> 24) & 0xff;
}

/*
 * ISO-9660 7.3.3
 * Set 32 bit unsigned int, store Both Byte orders
 */
EXPORT void
set_733(vp, i)
	void		*vp;
	UInt32_t	i;
{
	Uchar	*p = vp;

	p[7] = p[0] = i & 0xff;
	p[6] = p[1] = (i >> 8) & 0xff;
	p[5] = p[2] = (i >> 16) & 0xff;
	p[4] = p[3] = (i >> 24) & 0xff;
}

/*
 * ISO-9660 7.1.1
 * Get 8 bit unsigned int
 */
EXPORT UInt32_t
get_711(vp)
	void	*vp;
{
	Uchar	*p = vp;

	return (*p & 0xff);
}

/*
 * ISO-9660 7.2.1
 * Get 16 bit unsigned int, stored with Least significant byte first
 */
EXPORT UInt32_t
get_721(vp)
	void	*vp;
{
	Uchar	*p = vp;

	return ((p[0] & 0xff) | ((p[1] & 0xff) << 8));
}


/*
 * ISO-9660 7.2.3
 * Get 16 bit unsigned int, stored with Both Byte orders
 */
EXPORT UInt32_t
get_723(vp)
	void	*vp;
{
	Uchar	*p = vp;
#if 0
	if (p[0] != p[3] || p[1] != p[2]) {
		comerrno(EX_BAD, _("Invalid format 7.2.3 number\n"));
	}
#endif
	return (get_721(p));
}


/*
 * ISO-9660 7.3.1
 * Get 32 bit unsigned int, stored with Least significant byte first
 */
EXPORT UInt32_t
get_731(vp)
	void	*vp;
{
	Uchar	*p = vp;

	return ((p[0] & 0xff)
		| ((p[1] & 0xff) << 8)
		| ((p[2] & 0xff) << 16)
		| ((p[3] & 0xff) << 24));
}

/*
 * ISO-9660 7.3.2
 * Get 32 bit unsigned int, stored with Most significant byte first
 */
EXPORT UInt32_t
get_732(vp)
	void	*vp;
{
	Uchar	*p = vp;

	return ((p[3] & 0xff)
		| ((p[2] & 0xff) << 8)
		| ((p[1] & 0xff) << 16)
		| ((p[0] & 0xff) << 24));
}

/*
 * ISO-9660 7.3.3
 * Get 32 bit unsigned int, stored with Both Byte orders
 */
EXPORT UInt32_t
get_733(vp)
	void	*vp;
{
	Uchar	*p = vp;

	return ((p[0] & 0xff)
		| ((p[1] & 0xff) << 8)
		| ((p[2] & 0xff) << 16)
		| ((p[3] & 0xff) << 24));
}
