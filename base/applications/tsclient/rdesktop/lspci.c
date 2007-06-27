/*  -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Support for the Matrox "lspci" channel
   Copyright (C) 2005 Matrox Graphics Inc. 

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
#include <sys/types.h>
#include <unistd.h>

static VCHANNEL *lspci_channel;

typedef struct _pci_device
{
	uint16 klass;
	uint16 vendor;
	uint16 device;
	uint16 subvendor;
	uint16 subdevice;
	uint8 revision;
	uint8 progif;
} pci_device;

static pci_device current_device;

static void lspci_send(RDPCLIENT * This, const char *output);


/* Handle one line of output from the lspci subprocess */
static BOOL
handle_child_line(RDPCLIENT * This, const char *line, void *data)
{
	const char *val;
	char buf[1024];

	if (str_startswith(line, "Class:"))
	{
		val = line + sizeof("Class:");
		/* Skip whitespace and second Class: occurance */
		val += strspn(val, " \t") + sizeof("Class");
		current_device.klass = strtol(val, NULL, 16);
	}
	else if (str_startswith(line, "Vendor:"))
	{
		val = line + sizeof("Vendor:");
		current_device.vendor = strtol(val, NULL, 16);
	}
	else if (str_startswith(line, "Device:"))
	{
		val = line + sizeof("Device:");
		/* Sigh, there are *two* lines tagged as Device:. We
		   are not interested in the domain/bus/slot/func */
		if (!strchr(val, ':'))
			current_device.device = strtol(val, NULL, 16);
	}
	else if (str_startswith(line, "SVendor:"))
	{
		val = line + sizeof("SVendor:");
		current_device.subvendor = strtol(val, NULL, 16);
	}
	else if (str_startswith(line, "SDevice:"))
	{
		val = line + sizeof("SDevice:");
		current_device.subdevice = strtol(val, NULL, 16);
	}
	else if (str_startswith(line, "Rev:"))
	{
		val = line + sizeof("Rev:");
		current_device.revision = strtol(val, NULL, 16);
	}
	else if (str_startswith(line, "ProgIf:"))
	{
		val = line + sizeof("ProgIf:");
		current_device.progif = strtol(val, NULL, 16);
	}
	else if (strspn(line, " \t") == strlen(line))
	{
		/* Blank line. Send collected information over channel */
		snprintf(buf, sizeof(buf), "%04x,%04x,%04x,%04x,%04x,%02x,%02x\n",
			 current_device.klass, current_device.vendor,
			 current_device.device, current_device.subvendor,
			 current_device.subdevice, current_device.revision, current_device.progif);
		lspci_send(This, buf);
		memset(&current_device, 0, sizeof(current_device));
	}
	else
	{
		warning("lspci: Unrecoqnized line '%s'\n", line);
	}
	return True;
}


/* Process one line of input from virtual channel */
static BOOL
lspci_process_line(RDPCLIENT * This, const char *line, void *data)
{
	char *lspci_command[5] = { "lspci", "-m", "-n", "-v", NULL };

	if (!strcmp(line, "LSPCI"))
	{
		memset(&current_device, 0, sizeof(current_device));
		subprocess(This, lspci_command, handle_child_line, NULL);
		/* Send single dot to indicate end of enumeration */
		lspci_send(This, ".\n");
	}
	else
	{
		error("lspci protocol error: Invalid line '%s'\n", line);
	}
	return True;
}


/* Process new data from the virtual channel */
static void
lspci_process(RDPCLIENT * This, STREAM s)
{
	unsigned int pkglen;
	static char *rest = NULL;
	char *buf;

	pkglen = s->end - s->p;
	/* str_handle_lines requires null terminated strings */
	buf = xmalloc(pkglen + 1);
	STRNCPY(buf, (char *) s->p, pkglen + 1);
#if 0
	printf("lspci recv:\n");
	hexdump(s->p, pkglen);
#endif

	str_handle_lines(This, buf, &rest, lspci_process_line, NULL);
	xfree(buf);
}

/* Initialize this module: Register the lspci channel */
BOOL
lspci_init(RDPCLIENT * This)
{
	lspci_channel =
		channel_register(This, "lspci", CHANNEL_OPTION_INITIALIZED | CHANNEL_OPTION_ENCRYPT_RDP,
				 lspci_process);
	return (lspci_channel != NULL);
}

/* Send data to channel */
static void
lspci_send(RDPCLIENT * This, const char *output)
{
	STREAM s;
	size_t len;

	len = strlen(output);
	s = channel_init(This, lspci_channel, len);
	out_uint8p(s, output, len) s_mark_end(s);

#if 0
	printf("lspci send:\n");
	hexdump(s->channel_hdr + 8, s->end - s->channel_hdr - 8);
#endif

	channel_send(This, s, lspci_channel);
}
