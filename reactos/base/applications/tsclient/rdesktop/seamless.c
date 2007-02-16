/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Seamless Windows support
   Copyright (C) Peter Astrand <astrand@cendio.se> 2005-2006
   
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
#include <stdarg.h>
#include <assert.h>

/* #define WITH_DEBUG_SEAMLESS */

#ifdef WITH_DEBUG_SEAMLESS
#define DEBUG_SEAMLESS(args) printf args;
#else
#define DEBUG_SEAMLESS(args)
#endif

static char *
seamless_get_token(char **s)
{
	char *comma, *head;
	head = *s;

	if (!head)
		return NULL;

	comma = strchr(head, ',');
	if (comma)
	{
		*comma = '\0';
		*s = comma + 1;
	}
	else
	{
		*s = NULL;
	}

	return head;
}


static BOOL
seamless_process_line(RDPCLIENT * This, const char *line, void *data)
{
	char *p, *l;
	char *tok1, *tok2, *tok3, *tok4, *tok5, *tok6, *tok7, *tok8;
	unsigned long id, flags;
	char *endptr;

	l = xstrdup(line);
	p = l;

	DEBUG_SEAMLESS(("seamlessrdp got:%s\n", p));

	tok1 = seamless_get_token(&p);
	tok2 = seamless_get_token(&p);
	tok3 = seamless_get_token(&p);
	tok4 = seamless_get_token(&p);
	tok5 = seamless_get_token(&p);
	tok6 = seamless_get_token(&p);
	tok7 = seamless_get_token(&p);
	tok8 = seamless_get_token(&p);

	if (!strcmp("CREATE", tok1))
	{
		unsigned long group, parent;
		if (!tok6)
			return False;

		id = strtoul(tok3, &endptr, 0);
		if (*endptr)
			return False;

		group = strtoul(tok4, &endptr, 0);
		if (*endptr)
			return False;

		parent = strtoul(tok5, &endptr, 0);
		if (*endptr)
			return False;

		flags = strtoul(tok6, &endptr, 0);
		if (*endptr)
			return False;

		ui_seamless_create_window(This, id, group, parent, flags);
	}
	else if (!strcmp("DESTROY", tok1))
	{
		if (!tok4)
			return False;

		id = strtoul(tok3, &endptr, 0);
		if (*endptr)
			return False;

		flags = strtoul(tok4, &endptr, 0);
		if (*endptr)
			return False;

		ui_seamless_destroy_window(This, id, flags);

	}
	else if (!strcmp("DESTROYGRP", tok1))
	{
		if (!tok4)
			return False;

		id = strtoul(tok3, &endptr, 0);
		if (*endptr)
			return False;

		flags = strtoul(tok4, &endptr, 0);
		if (*endptr)
			return False;

		ui_seamless_destroy_group(This, id, flags);
	}
	else if (!strcmp("SETICON", tok1))
	{
		unimpl("SeamlessRDP SETICON1\n");
	}
	else if (!strcmp("POSITION", tok1))
	{
		int x, y, width, height;

		if (!tok8)
			return False;

		id = strtoul(tok3, &endptr, 0);
		if (*endptr)
			return False;

		x = strtol(tok4, &endptr, 0);
		if (*endptr)
			return False;
		y = strtol(tok5, &endptr, 0);
		if (*endptr)
			return False;

		width = strtol(tok6, &endptr, 0);
		if (*endptr)
			return False;
		height = strtol(tok7, &endptr, 0);
		if (*endptr)
			return False;

		flags = strtoul(tok8, &endptr, 0);
		if (*endptr)
			return False;

		ui_seamless_move_window(This, id, x, y, width, height, flags);
	}
	else if (!strcmp("ZCHANGE", tok1))
	{
		unsigned long behind;

		id = strtoul(tok3, &endptr, 0);
		if (*endptr)
			return False;

		behind = strtoul(tok4, &endptr, 0);
		if (*endptr)
			return False;

		flags = strtoul(tok5, &endptr, 0);
		if (*endptr)
			return False;

		ui_seamless_restack_window(This, id, behind, flags);
	}
	else if (!strcmp("TITLE", tok1))
	{
		if (!tok5)
			return False;

		id = strtoul(tok3, &endptr, 0);
		if (*endptr)
			return False;

		flags = strtoul(tok5, &endptr, 0);
		if (*endptr)
			return False;

		ui_seamless_settitle(This, id, tok4, flags);
	}
	else if (!strcmp("STATE", tok1))
	{
		unsigned int state;

		if (!tok5)
			return False;

		id = strtoul(tok3, &endptr, 0);
		if (*endptr)
			return False;

		state = strtoul(tok4, &endptr, 0);
		if (*endptr)
			return False;

		flags = strtoul(tok5, &endptr, 0);
		if (*endptr)
			return False;

		ui_seamless_setstate(This, id, state, flags);
	}
	else if (!strcmp("DEBUG", tok1))
	{
		DEBUG_SEAMLESS(("SeamlessRDP:%s\n", line));
	}
	else if (!strcmp("SYNCBEGIN", tok1))
	{
		if (!tok3)
			return False;

		flags = strtoul(tok3, &endptr, 0);
		if (*endptr)
			return False;

		ui_seamless_syncbegin(This, flags);
	}
	else if (!strcmp("SYNCEND", tok1))
	{
		if (!tok3)
			return False;

		flags = strtoul(tok3, &endptr, 0);
		if (*endptr)
			return False;

		/* do nothing, currently */
	}
	else if (!strcmp("HELLO", tok1))
	{
		if (!tok3)
			return False;

		flags = strtoul(tok3, &endptr, 0);
		if (*endptr)
			return False;

		ui_seamless_begin(This, !!(flags & SEAMLESSRDP_HELLO_HIDDEN));
	}
	else if (!strcmp("ACK", tok1))
	{
		unsigned int serial;

		serial = strtoul(tok3, &endptr, 0);
		if (*endptr)
			return False;

		ui_seamless_ack(This, serial);
	}
	else if (!strcmp("HIDE", tok1))
	{
		if (!tok3)
			return False;

		flags = strtoul(tok3, &endptr, 0);
		if (*endptr)
			return False;

		ui_seamless_hide_desktop(This);
	}
	else if (!strcmp("UNHIDE", tok1))
	{
		if (!tok3)
			return False;

		flags = strtoul(tok3, &endptr, 0);
		if (*endptr)
			return False;

		ui_seamless_unhide_desktop(This);
	}


	xfree(l);
	return True;
}


static BOOL
seamless_line_handler(RDPCLIENT * This, const char *line, void *data)
{
	if (!seamless_process_line(This, line, data))
	{
		warning("SeamlessRDP: Invalid request:%s\n", line);
	}
	return True;
}


static void
seamless_process(RDPCLIENT * This, STREAM s)
{
	unsigned int pkglen;
	static char *rest = NULL;
	char *buf;

	pkglen = s->end - s->p;
	/* str_handle_lines requires null terminated strings */
	buf = xmalloc(pkglen + 1);
	STRNCPY(buf, (char *) s->p, pkglen + 1);
#if 0
	printf("seamless recv:\n");
	hexdump(s->p, pkglen);
#endif

	str_handle_lines(This, buf, &rest, seamless_line_handler, NULL);

	xfree(buf);
}


BOOL
seamless_init(RDPCLIENT * This)
{
	if (!This->seamless_rdp)
		return False;

	This->seamless.serial = 0;

	This->seamless.channel =
		channel_register(This, "seamrdp", CHANNEL_OPTION_INITIALIZED | CHANNEL_OPTION_ENCRYPT_RDP,
				 seamless_process);
	return (This->seamless.channel != NULL);
}


static unsigned int
seamless_send(RDPCLIENT * This, const char *command, const char *format, ...)
{
	STREAM s;
	size_t len;
	va_list argp;
	char buf[1025];

	len = snprintf(buf, sizeof(buf) - 1, "%s,%u,", command, This->seamless.serial);

	assert(len < (sizeof(buf) - 1));

	va_start(argp, format);
	len += vsnprintf(buf + len, sizeof(buf) - len - 1, format, argp);
	va_end(argp);

	assert(len < (sizeof(buf) - 1));

	buf[len] = '\n';
	buf[len + 1] = '\0';

	len++;

	s = channel_init(This, This->seamless.channel, len);
	out_uint8p(s, buf, len) s_mark_end(s);

	DEBUG_SEAMLESS(("SeamlessRDP sending:%s", buf));

#if 0
	printf("seamless send:\n");
	hexdump(s->channel_hdr + 8, s->end - s->channel_hdr - 8);
#endif

	channel_send(This, s, This->seamless.channel);

	return This->seamless.serial++;
}


unsigned int
seamless_send_sync(RDPCLIENT * This)
{
	if (!This->seamless_rdp)
		return (unsigned int) -1;

	return seamless_send(This, "SYNC", "");
}


unsigned int
seamless_send_state(RDPCLIENT * This, unsigned long id, unsigned int state, unsigned long flags)
{
	if (!This->seamless_rdp)
		return (unsigned int) -1;

	return seamless_send(This, "STATE", "0x%08lx,0x%x,0x%lx", id, state, flags);
}


unsigned int
seamless_send_position(RDPCLIENT * This, unsigned long id, int x, int y, int width, int height, unsigned long flags)
{
	return seamless_send(This, "POSITION", "0x%08lx,%d,%d,%d,%d,0x%lx", id, x, y, width, height,
			     flags);
}


/* Update select timeout */
void
seamless_select_timeout(RDPCLIENT * This, struct timeval *tv)
{
	struct timeval ourtimeout = { 0, SEAMLESSRDP_POSITION_TIMER };

	if (This->seamless_rdp)
	{
		if (timercmp(&ourtimeout, tv, <))
		{
			tv->tv_sec = ourtimeout.tv_sec;
			tv->tv_usec = ourtimeout.tv_usec;
		}
	}
}


unsigned int
seamless_send_zchange(RDPCLIENT * This, unsigned long id, unsigned long below, unsigned long flags)
{
	if (!This->seamless_rdp)
		return (unsigned int) -1;

	return seamless_send(This, "ZCHANGE", "0x%08lx,0x%08lx,0x%lx", id, below, flags);
}



unsigned int
seamless_send_focus(RDPCLIENT * This, unsigned long id, unsigned long flags)
{
	if (!This->seamless_rdp)
		return (unsigned int) -1;

	return seamless_send(This, "FOCUS", "0x%08lx,0x%lx", id, flags);
}
