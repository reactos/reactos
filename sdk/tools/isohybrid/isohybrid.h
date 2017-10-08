/*
 * isohybrid.h: header file for isohybrid.c: Post process an ISO 9660 image
 * generated with mkisofs or genisoimage to allow - hybrid booting - as a
 * CD-ROM or as a hard disk.
 *
 * Copyright (C) 2010 Geert Stappers <stappers@stappers.nl>
 *
 * isohybrid is a free software; you can redistribute it and/or modify it
 * under the terms of GNU General Public License as published by Free Software
 * Foundation; either version 2 of the license, or (at your option) any later
 * version.
 *
 * isohybrid is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with isohybrid; if not, see: <http://www.gnu.org/licenses>.
 *
 */

#define VERSION     "0.12"
#define BUFSIZE     2048
#define MBRSIZE      432

/* End of header file */
