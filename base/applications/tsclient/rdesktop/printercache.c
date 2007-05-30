/* -*- c-basic-offset: 8 -*-
 * rdesktop: A Remote Desktop Protocol client.
 * Entrypoint and utility functions
 * Copyright (C) Matthew Chapman 1999-2005
 * Copyright (C) Jeroen Meijer 2003
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* According to the W2K RDP Printer Redirection WhitePaper, a data
 * blob is sent to the client after the configuration of the printer
 * is changed at the server.
 *
 * This data blob is saved to the registry. The client returns this
 * data blob in a new session with the printer announce data.
 * The data is not interpreted by the client.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "rdesktop.h"

static BOOL
printercache_mkdir(char *base, char *printer)
{
	char *path;

	path = (char *) xmalloc(strlen(base) + sizeof("/.rdesktop/rdpdr/") + strlen(printer) + 1);

	sprintf(path, "%s/.rdesktop", base);
	if ((mkdir(path, 0700) == -1) && errno != EEXIST)
	{
		perror(path);
		xfree(path);
		return False;
	}

	strcat(path, "/rdpdr");
	if ((mkdir(path, 0700) == -1) && errno != EEXIST)
	{
		perror(path);
		xfree(path);
		return False;
	}

	strcat(path, "/");
	strcat(path, printer);
	if ((mkdir(path, 0700) == -1) && errno != EEXIST)
	{
		perror(path);
		xfree(path);
		return False;
	}

	xfree(path);
	return True;
}

static BOOL
printercache_unlink_blob(char *printer)
{
	char *path;
	char *home;

	if (printer == NULL)
		return False;

	home = getenv("HOME");
	if (home == NULL)
		return False;

	path = (char *) xmalloc(strlen(home) + sizeof("/.rdesktop/rdpdr/") + strlen(printer) +
				sizeof("/AutoPrinterCacheData") + 1);

	sprintf(path, "%s/.rdesktop/rdpdr/%s/AutoPrinterCacheData", home, printer);

	if (unlink(path) < 0)
	{
		xfree(path);
		return False;
	}

	sprintf(path, "%s/.rdesktop/rdpdr/%s", home, printer);

	if (rmdir(path) < 0)
	{
		xfree(path);
		return False;
	}

	xfree(path);
	return True;
}


static BOOL
printercache_rename_blob(char *printer, char *new_printer)
{
	char *printer_path;
	char *new_printer_path;
	int printer_maxlen;

	char *home;

	if (printer == NULL)
		return False;

	home = getenv("HOME");
	if (home == NULL)
		return False;

	printer_maxlen =
		(strlen(printer) >
		 strlen(new_printer) ? strlen(printer) : strlen(new_printer)) + strlen(home) +
		sizeof("/.rdesktop/rdpdr/") + 1;

	printer_path = (char *) xmalloc(printer_maxlen);
	new_printer_path = (char *) xmalloc(printer_maxlen);

	sprintf(printer_path, "%s/.rdesktop/rdpdr/%s", home, printer);
	sprintf(new_printer_path, "%s/.rdesktop/rdpdr/%s", home, new_printer);

	printf("%s,%s\n", printer_path, new_printer_path);
	if (rename(printer_path, new_printer_path) < 0)
	{
		xfree(printer_path);
		xfree(new_printer_path);
		return False;
	}

	xfree(printer_path);
	xfree(new_printer_path);
	return True;
}


int
printercache_load_blob(char *printer_name, uint8 ** data)
{
	char *home, *path;
	struct stat st;
	int fd, length;

	if (printer_name == NULL)
		return 0;

	*data = NULL;

	home = getenv("HOME");
	if (home == NULL)
		return 0;

	path = (char *) xmalloc(strlen(home) + sizeof("/.rdesktop/rdpdr/") + strlen(printer_name) +
				sizeof("/AutoPrinterCacheData") + 1);
	sprintf(path, "%s/.rdesktop/rdpdr/%s/AutoPrinterCacheData", home, printer_name);

	fd = open(path, O_RDONLY);
	if (fd == -1)
	{
		xfree(path);
		return 0;
	}

	if (fstat(fd, &st))
	{
		xfree(path);
		return 0;
	}

	*data = (uint8 *) xmalloc(st.st_size);
	length = read(fd, *data, st.st_size);
	close(fd);
	xfree(path);
	return length;
}

static void
printercache_save_blob(char *printer_name, uint8 * data, uint32 length)
{
	char *home, *path;
	int fd;

	if (printer_name == NULL)
		return;

	home = getenv("HOME");
	if (home == NULL)
		return;

	if (!printercache_mkdir(home, printer_name))
		return;

	path = (char *) xmalloc(strlen(home) + sizeof("/.rdesktop/rdpdr/") + strlen(printer_name) +
				sizeof("/AutoPrinterCacheData") + 1);
	sprintf(path, "%s/.rdesktop/rdpdr/%s/AutoPrinterCacheData", home, printer_name);

	fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (fd == -1)
	{
		perror(path);
		xfree(path);
		return;
	}

	if (write(fd, data, length) != length)
	{
		perror(path);
		unlink(path);
	}

	close(fd);
	xfree(path);
}

void
printercache_process(RDPCLIENT * This, STREAM s)
{
	uint32 type, printer_length, driver_length, printer_unicode_length, blob_length;
	char device_name[9], printer[256], driver[256];

	in_uint32_le(s, type);
	switch (type)
	{
		case 4:	/* rename item */
			in_uint8(s, printer_length);
			in_uint8s(s, 0x3);	/* padding */
			in_uint8(s, driver_length);
			in_uint8s(s, 0x3);	/* padding */

			/* NOTE - 'driver' doesn't contain driver, it contains the new printer name */

			rdp_in_unistr(This, s, printer, printer_length);
			rdp_in_unistr(This, s, driver, driver_length);

			printercache_rename_blob(printer, driver);
			break;

		case 3:	/* delete item */
			in_uint8(s, printer_unicode_length);
			in_uint8s(s, 0x3);	/* padding */
			printer_length = rdp_in_unistr(This, s, printer, printer_unicode_length);
			printercache_unlink_blob(printer);
			break;

		case 2:	/* save printer data */
			in_uint32_le(s, printer_unicode_length);
			in_uint32_le(s, blob_length);

			if (printer_unicode_length < 2 * 255)
			{
				rdp_in_unistr(This, s, printer, printer_unicode_length);
				printercache_save_blob(printer, s->p, blob_length);
			}
			break;

		case 1:	/* save device data */
			in_uint8a(s, device_name, 5);	/* get LPTx/COMx name */

			/* need to fetch this data so that we can get the length of the packet to store. */
			in_uint8s(s, 0x2);	/* ??? */
			in_uint8s(s, 0x2)	/* pad?? */
				in_uint32_be(s, driver_length);
			in_uint32_be(s, printer_length);
			in_uint8s(s, 0x7)	/* pad?? */
				/* next is driver in unicode */
				/* next is printer in unicode */
				/* TODO: figure out how to use this information when reconnecting */
				/* actually - all we need to store is the driver and printer */
				/* and figure out what the first word is. */
				/* rewind stream so that we can save this blob   */
				/* length is driver_length + printer_length + 19 */
				/* rewind stream */
				s->p = s->p - 19;

			printercache_save_blob(device_name, s->p,
					       driver_length + printer_length + 19);
			break;
		default:

			unimpl("RDPDR Printer Cache Packet Type: %d\n", type);
			break;
	}
}
