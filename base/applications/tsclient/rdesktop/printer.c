/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Copyright (C) Matthew Chapman 1999-2005

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "rdesktop.h"

static PRINTER *
get_printer_data(RDPCLIENT * This, NTHANDLE handle)
{
	int index;

	for (index = 0; index < RDPDR_MAX_DEVICES; index++)
	{
		if (handle == This->rdpdr_device[index].handle)
			return (PRINTER *) This->rdpdr_device[index].pdevice_data;
	}
	return NULL;
}

int
printer_enum_devices(RDPCLIENT * This, uint32 * id, char *optarg)
{
	PRINTER *pprinter_data;

	char *pos = optarg;
	char *pos2;
	int count = 0;
	int already = 0;

	/* we need to know how many printers we've already set up
	   supplied from other -r flags than this one. */
	while (count < *id)
	{
		if (This->rdpdr_device[count].device_type == DEVICE_TYPE_PRINTER)
			already++;
		count++;
	}

	count = 0;

	if (*optarg == ':')
		optarg++;

	while ((pos = next_arg(optarg, ',')) && *id < RDPDR_MAX_DEVICES)
	{
		pprinter_data = (PRINTER *) xmalloc(sizeof(PRINTER));

		strcpy(This->rdpdr_device[*id].name, "PRN");
		strcat(This->rdpdr_device[*id].name, l_to_a(already + count + 1, 10));

		/* first printer is set as default printer */
		if ((already + count) == 0)
			pprinter_data->default_printer = True;
		else
			pprinter_data->default_printer = False;

		pos2 = next_arg(optarg, '=');
		if (*optarg == (char) 0x00)
			pprinter_data->printer = "mydeskjet";	/* set default */
		else
		{
			pprinter_data->printer = xmalloc(strlen(optarg) + 1);
			strcpy(pprinter_data->printer, optarg);
		}

		if (!pos2 || (*pos2 == (char) 0x00))
			pprinter_data->driver = "HP Color LaserJet 8500 PS";	/* no printer driver supplied set default */
		else
		{
			pprinter_data->driver = xmalloc(strlen(pos2) + 1);
			strcpy(pprinter_data->driver, pos2);
		}

		printf("PRINTER %s to %s driver %s\n", This->rdpdr_device[*id].name,
		       pprinter_data->printer, pprinter_data->driver);
		This->rdpdr_device[*id].device_type = DEVICE_TYPE_PRINTER;
		This->rdpdr_device[*id].pdevice_data = (void *) pprinter_data;
		count++;
		(*id)++;

		optarg = pos;
	}
	return count;
}

static NTSTATUS
printer_create(RDPCLIENT * This, uint32 device_id, uint32 access, uint32 share_mode, uint32 disposition, uint32 flags,
	       char *filename, NTHANDLE * handle)
{
	char cmd[256];
	PRINTER *pprinter_data;

	pprinter_data = (PRINTER *) This->rdpdr_device[device_id].pdevice_data;

	/* default printer name use default printer queue as well in unix */
	if (pprinter_data->printer == "mydeskjet")
	{
		pprinter_data->printer_fp = popen("lpr", "w");
	}
	else
	{
		sprintf(cmd, "lpr -P %s", pprinter_data->printer);
		pprinter_data->printer_fp = popen(cmd, "w");
	}

	This->rdpdr_device[device_id].handle = fileno(pprinter_data->printer_fp);
	*handle = This->rdpdr_device[device_id].handle;
	return STATUS_SUCCESS;
}

static NTSTATUS
printer_close(RDPCLIENT * This, NTHANDLE handle)
{
	int i = get_device_index(This, handle);
	if (i >= 0)
	{
		PRINTER *pprinter_data = This->rdpdr_device[i].pdevice_data;
		if (pprinter_data)
			pclose(pprinter_data->printer_fp);
		This->rdpdr_device[i].handle = 0;
	}
	return STATUS_SUCCESS;
}

static NTSTATUS
printer_write(RDPCLIENT * This, NTHANDLE handle, uint8 * data, uint32 length, uint32 offset, uint32 * result)
{
	PRINTER *pprinter_data;

	pprinter_data = get_printer_data(This, handle);
	*result = length * fwrite(data, length, 1, pprinter_data->printer_fp);

	if (ferror(pprinter_data->printer_fp))
	{
		*result = 0;
		return STATUS_INVALID_HANDLE;
	}
	return STATUS_SUCCESS;
}

DEVICE_FNS printer_fns = {
	printer_create,
	printer_close,
	NULL,			/* read */
	printer_write,
	NULL			/* device_control */
};
