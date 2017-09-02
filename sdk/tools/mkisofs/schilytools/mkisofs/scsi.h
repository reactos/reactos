/* @(#)scsi.h	1.4 16/11/14 Copyright 1997-2016 J. Schilling */
/*
 *	Copyright (c) 1997-2016 J. Schilling
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
 * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef	_SCSI_H
#define	_SCSI_H

/*
 * Implemented inside multi.c in case that USE_SCG has not been defined.
 */
extern int	readsecs	__PR((UInt32_t startsecno, void *buffer, int sectorcount));

#ifdef	USE_SCG
extern int	scsidev_open	__PR((char *path));
extern int	scsidev_close	__PR((void));
#endif

#endif	/* _SCSI_H */
