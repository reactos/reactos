/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Protocol services - Clipboard functions
   Copyright (C) Erik Forsberg <forsberg@cendio.se> 2003
   Copyright (C) Matthew Chapman 2003

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include "rdesktop.h"

/*
  To gain better understanding of this code, one could be assisted by the following documents:
  - Inter-Client Communication Conventions Manual (ICCCM)
    HTML: http://tronche.com/gui/x/icccm/
    PDF:  http://ftp.xfree86.org/pub/XFree86/4.5.0/doc/PDF/icccm.pdf
  - MSDN: Clipboard Formats
    https://web.archive.org/web/20080103082730/http://msdn2.microsoft.com/en-us/library/ms649013.aspx
*/

#ifdef HAVE_ICONV
#ifdef HAVE_LANGINFO_H
#ifdef HAVE_ICONV_H
#include <langinfo.h>
#include <iconv.h>
#define USE_UNICODE_CLIPBOARD
#endif
#endif
#endif

#ifdef USE_UNICODE_CLIPBOARD
#define RDP_CF_TEXT CF_UNICODETEXT
#else
#define RDP_CF_TEXT CF_TEXT
#endif


/* Translate LF to CR-LF. To do this, we must allocate more memory.
   The returned string is null-terminated, as required by CF_TEXT.
   Does not stop on embedded nulls.
   The length is updated. */
static void
crlf2lf(uint8 * data, uint32 * length)
{
	uint8 *dst, *src;
	src = dst = data;
	while (src < data + *length)
	{
		if (*src != '\x0d')
			*dst++ = *src;
		src++;
	}
	*length = dst - data;
}

#ifdef USE_UNICODE_CLIPBOARD
/* Translate LF to CR-LF. To do this, we must allocate more memory.
   The returned string is null-terminated, as required by CF_UNICODETEXT.
   The size is updated. */
static uint8 *
utf16_lf2crlf(uint8 * data, uint32 * size)
{
	uint8 *result;
	uint16 *inptr, *outptr;
	Bool swap_endianess;

	/* Worst case: Every char is LF */
	result = xmalloc((*size * 2) + 2);
	if (result == NULL)
		return NULL;

	inptr = (uint16 *) data;
	outptr = (uint16 *) result;

	/* Check for a reversed BOM */
	swap_endianess = (*inptr == 0xfffe);

	while ((uint8 *) inptr < data + *size)
	{
		uint16 uvalue = *inptr;
		if (swap_endianess)
			uvalue = ((uvalue << 8) & 0xff00) + (uvalue >> 8);
		if (uvalue == 0x0a)
			*outptr++ = swap_endianess ? 0x0d00 : 0x0d;
		*outptr++ = *inptr++;
	}
	*outptr++ = 0;		/* null termination */
	*size = (uint8 *) outptr - result;

	return result;
}
#else
/* Translate LF to CR-LF. To do this, we must allocate more memory.
   The length is updated. */
static uint8 *
lf2crlf(uint8 * data, uint32 * length)
{
	uint8 *result, *p, *o;

	/* Worst case: Every char is LF */
	result = xmalloc(*length * 2);

	p = data;
	o = result;

	while (p < data + *length)
	{
		if (*p == '\x0a')
			*o++ = '\x0d';
		*o++ = *p++;
	}
	*length = o - result;

	/* Convenience */
	*o++ = '\0';

	return result;
}
#endif

static void
xclip_provide_selection(RDPCLIENT * This, XSelectionRequestEvent * req, Atom type, unsigned int format, uint8 * data,
			uint32 length)
{
	XEvent xev;

	DEBUG_CLIPBOARD(("xclip_provide_selection: requestor=0x%08x, target=%s, property=%s, length=%u\n", (unsigned) req->requestor, XGetAtomName(This->display, req->target), XGetAtomName(This->display, req->property), (unsigned) length));

	XChangeProperty(This->display, req->requestor, req->property,
			type, format, PropModeReplace, data, length);

	xev.xselection.type = SelectionNotify;
	xev.xselection.serial = 0;
	xev.xselection.send_event = True;
	xev.xselection.requestor = req->requestor;
	xev.xselection.selection = req->selection;
	xev.xselection.target = req->target;
	xev.xselection.property = req->property;
	xev.xselection.time = req->time;
	XSendEvent(This->display, req->requestor, False, NoEventMask, &xev);
}

/* Replies a clipboard requestor, telling that we're unable to satisfy his request for whatever reason.
   This has the benefit of finalizing the clipboard negotiation and thus not leaving our requestor
   lingering (and, potentially, stuck). */
static void
xclip_refuse_selection(RDPCLIENT * This, XSelectionRequestEvent * req)
{
	XEvent xev;

	DEBUG_CLIPBOARD(("xclip_refuse_selection: requestor=0x%08x, target=%s, property=%s\n",
			 (unsigned) req->requestor, XGetAtomName(This->display, req->target),
			 XGetAtomName(This->display, req->property)));

	xev.xselection.type = SelectionNotify;
	xev.xselection.serial = 0;
	xev.xselection.send_event = True;
	xev.xselection.requestor = req->requestor;
	xev.xselection.selection = req->selection;
	xev.xselection.target = req->target;
	xev.xselection.property = None;
	xev.xselection.time = req->time;
	XSendEvent(This->display, req->requestor, False, NoEventMask, &xev);
}

/* Wrapper for cliprdr_send_data which also cleans the request state. */
static void
helper_cliprdr_send_response(RDPCLIENT * This, uint8 * data, uint32 length)
{
	if (This->xclip.rdp_clipboard_request_format != 0)
	{
		cliprdr_send_data(This, data, length);
		This->xclip.rdp_clipboard_request_format = 0;
		if (!This->xclip.rdesktop_is_selection_owner)
			cliprdr_send_simple_native_format_announce(This, RDP_CF_TEXT);
	}
}

/* Last resort, when we have to provide clipboard data but for whatever
   reason couldn't get any.
 */
static void
helper_cliprdr_send_empty_response(RDPCLIENT * This)
{
	helper_cliprdr_send_response(This, NULL, 0);
}

/* Replies with clipboard data to RDP, converting it from the target format
   to the expected RDP format as necessary. Returns true if data was sent.
 */
static Bool
xclip_send_data_with_convert(RDPCLIENT * This, uint8 * source, size_t source_size, Atom target)
{
	DEBUG_CLIPBOARD(("xclip_send_data_with_convert: target=%s, size=%u\n",
			 XGetAtomName(This->display, target), (unsigned) source_size));

#ifdef USE_UNICODE_CLIPBOARD
	if (target == This->xclip.format_string_atom ||
	    target == This->xclip.format_unicode_atom || target == This->xclip.format_utf8_string_atom)
	{
		size_t unicode_buffer_size;
		char *unicode_buffer;
		iconv_t cd;
		size_t unicode_buffer_size_remaining;
		char *unicode_buffer_remaining;
		char *data_remaining;
		size_t data_size_remaining;
		uint32 translated_data_size;
		uint8 *translated_data;

		if (This->xclip.rdp_clipboard_request_format != RDP_CF_TEXT)
			return False;

		/* Make an attempt to convert any string we send to Unicode.
		   We don't know what the RDP server's ANSI Codepage is, or how to convert
		   to it, so using CF_TEXT is not safe (and is unnecessary, since all
		   WinNT versions are Unicode-minded).
		 */
		if (target == This->xclip.format_string_atom)
		{
			char *locale_charset = nl_langinfo(CODESET);
			cd = iconv_open(WINDOWS_CODEPAGE, locale_charset);
			if (cd == (iconv_t) - 1)
			{
				DEBUG_CLIPBOARD(("Locale charset %s not found in iconv. Unable to convert clipboard text.\n", locale_charset));
				return False;
			}
			unicode_buffer_size = source_size * 4;
		}
		else if (target == This->xclip.format_unicode_atom)
		{
			cd = iconv_open(WINDOWS_CODEPAGE, "UCS-2");
			if (cd == (iconv_t) - 1)
			{
				return False;
			}
			unicode_buffer_size = source_size;
		}
		else if (target == This->xclip.format_utf8_string_atom)
		{
			cd = iconv_open(WINDOWS_CODEPAGE, "UTF-8");
			if (cd == (iconv_t) - 1)
			{
				return False;
			}
			/* UTF-8 is guaranteed to be less or equally compact
			   as UTF-16 for all Unicode chars >=2 bytes.
			 */
			unicode_buffer_size = source_size * 2;
		}
		else
		{
			return False;
		}

		unicode_buffer = xmalloc(unicode_buffer_size);
		unicode_buffer_size_remaining = unicode_buffer_size;
		unicode_buffer_remaining = unicode_buffer;
		data_remaining = (char *) source;
		data_size_remaining = source_size;
		iconv(cd, (ICONV_CONST char **) &data_remaining, &data_size_remaining,
		      &unicode_buffer_remaining, &unicode_buffer_size_remaining);
		iconv_close(cd);

		/* translate linebreaks */
		translated_data_size = unicode_buffer_size - unicode_buffer_size_remaining;
		translated_data = utf16_lf2crlf((uint8 *) unicode_buffer, &translated_data_size);
		if (translated_data != NULL)
		{
			DEBUG_CLIPBOARD(("Sending Unicode string of %d bytes\n",
					 translated_data_size));
			helper_cliprdr_send_response(This, translated_data, translated_data_size);
			xfree(translated_data);	/* Not the same thing as XFree! */
		}

		xfree(unicode_buffer);

		return True;
	}
#else
	if (target == This->xclip.format_string_atom)
	{
		uint8 *translated_data;
		uint32 length = source_size;

		if (This->xclip.rdp_clipboard_request_format != RDP_CF_TEXT)
			return False;

		DEBUG_CLIPBOARD(("Translating linebreaks before sending data\n"));
		translated_data = lf2crlf(source, &length);
		if (translated_data != NULL)
		{
			helper_cliprdr_send_response(This, translated_data, length);
			xfree(translated_data);	/* Not the same thing as XFree! */
		}

		return True;
	}
#endif
	else if (target == This->xclip.rdesktop_native_atom)
	{
		helper_cliprdr_send_response(This, source, source_size + 1);

		return True;
	}
	else
	{
		return False;
	}
}

static void
xclip_clear_target_props(RDPCLIENT * This)
{
	XDeleteProperty(This->display, This->wnd, This->xclip.rdesktop_clipboard_target_atom);
	XDeleteProperty(This->display, This->wnd, This->xclip.rdesktop_primary_timestamp_target_atom);
	XDeleteProperty(This->display, This->wnd, This->xclip.rdesktop_clipboard_timestamp_target_atom);
}

static void
xclip_notify_change(RDPCLIENT * This)
{
	XChangeProperty(This->display, DefaultRootWindow(This->display),
			This->xclip.rdesktop_selection_notify_atom, XA_INTEGER, 32, PropModeReplace, NULL, 0);
}

static void
xclip_probe_selections(RDPCLIENT * This)
{
	Window primary_owner, clipboard_owner;

	if (This->xclip.probing_selections)
	{
		DEBUG_CLIPBOARD(("Already probing selections. Scheduling reprobe.\n"));
		This->xclip.reprobe_selections = True;
		return;
	}

	DEBUG_CLIPBOARD(("Probing selections.\n"));

	This->xclip.probing_selections = True;
	This->xclip.reprobe_selections = False;

	xclip_clear_target_props(This);

	if (This->xclip.auto_mode)
		primary_owner = XGetSelectionOwner(This->display, This->xclip.primary_atom);
	else
		primary_owner = None;

	clipboard_owner = XGetSelectionOwner(This->display, This->xclip.clipboard_atom);

	/* If we own all relevant selections then don't do anything. */
	if (((primary_owner == This->wnd) || !This->xclip.auto_mode) && (clipboard_owner == This->wnd))
		goto end;

	/* Both available */
	if ((primary_owner != None) && (clipboard_owner != None))
	{
		This->xclip.primary_timestamp = 0;
		This->xclip.clipboard_timestamp = 0;
		XConvertSelection(This->display, This->xclip.primary_atom, This->xclip.timestamp_atom,
				  This->xclip.rdesktop_primary_timestamp_target_atom, This->wnd, CurrentTime);
		XConvertSelection(This->display, This->xclip.clipboard_atom, This->xclip.timestamp_atom,
				  This->xclip.rdesktop_clipboard_timestamp_target_atom, This->wnd, CurrentTime);
		return;
	}

	/* Just PRIMARY */
	if (primary_owner != None)
	{
		XConvertSelection(This->display, This->xclip.primary_atom, This->xclip.targets_atom,
				  This->xclip.rdesktop_clipboard_target_atom, This->wnd, CurrentTime);
		return;
	}

	/* Just CLIPBOARD */
	if (clipboard_owner != None)
	{
		XConvertSelection(This->display, This->xclip.clipboard_atom, This->xclip.targets_atom,
				  This->xclip.rdesktop_clipboard_target_atom, This->wnd, CurrentTime);
		return;
	}

	DEBUG_CLIPBOARD(("No owner of any selection.\n"));

	/* FIXME:
	   Without XFIXES, we cannot reliably know the formats offered by an
	   upcoming selection owner, so we just lie about him offering
	   RDP_CF_TEXT. */
	cliprdr_send_simple_native_format_announce(This, RDP_CF_TEXT);

      end:
	This->xclip.probing_selections = False;
}

/* This function is called for SelectionNotify events.
   The SelectionNotify event is sent from the clipboard owner to the requestor
   after his request was satisfied.
   If this function is called, we're the requestor side. */
#ifndef MAKE_PROTO
void
xclip_handle_SelectionNotify(RDPCLIENT * This, XSelectionEvent * event)
{
	unsigned long nitems, bytes_left;
	XWindowAttributes wa;
	Atom type;
	Atom *supported_targets;
	int res, i, format;
	uint8 *data = NULL;

	if (event->property == None)
		goto fail;

	DEBUG_CLIPBOARD(("xclip_handle_SelectionNotify: selection=%s, target=%s, property=%s\n",
			 XGetAtomName(This->display, event->selection),
			 XGetAtomName(This->display, event->target),
			 XGetAtomName(This->display, event->property)));

	if (event->target == This->xclip.timestamp_atom)
	{
		if (event->selection == This->xclip.primary_atom)
		{
			res = XGetWindowProperty(This->display, This->wnd,
						 This->xclip.rdesktop_primary_timestamp_target_atom, 0,
						 XMaxRequestSize(This->display), False, AnyPropertyType,
						 &type, &format, &nitems, &bytes_left, &data);
		}
		else
		{
			res = XGetWindowProperty(This->display, This->wnd,
						 This->xclip.rdesktop_clipboard_timestamp_target_atom, 0,
						 XMaxRequestSize(This->display), False, AnyPropertyType,
						 &type, &format, &nitems, &bytes_left, &data);
		}


		if ((res != Success) || (nitems != 1) || (format != 32))
		{
			DEBUG_CLIPBOARD(("XGetWindowProperty failed!\n"));
			goto fail;
		}

		if (event->selection == This->xclip.primary_atom)
		{
			This->xclip.primary_timestamp = *(Time *) data;
			if (This->xclip.primary_timestamp == 0)
				This->xclip.primary_timestamp++;
			XDeleteProperty(This->display, This->wnd, This->xclip.rdesktop_primary_timestamp_target_atom);
			DEBUG_CLIPBOARD(("Got PRIMARY timestamp: %u\n",
					 (unsigned) This->xclip.primary_timestamp));
		}
		else
		{
			This->xclip.clipboard_timestamp = *(Time *) data;
			if (This->xclip.clipboard_timestamp == 0)
				This->xclip.clipboard_timestamp++;
			XDeleteProperty(This->display, This->wnd, This->xclip.rdesktop_clipboard_timestamp_target_atom);
			DEBUG_CLIPBOARD(("Got CLIPBOARD timestamp: %u\n",
					 (unsigned) This->xclip.clipboard_timestamp));
		}

		XFree(data);

		if (This->xclip.primary_timestamp && This->xclip.clipboard_timestamp)
		{
			if (This->xclip.primary_timestamp > This->xclip.clipboard_timestamp)
			{
				DEBUG_CLIPBOARD(("PRIMARY is most recent selection.\n"));
				XConvertSelection(This->display, This->xclip.primary_atom, This->xclip.targets_atom,
						  This->xclip.rdesktop_clipboard_target_atom, This->wnd,
						  event->time);
			}
			else
			{
				DEBUG_CLIPBOARD(("CLIPBOARD is most recent selection.\n"));
				XConvertSelection(This->display, This->xclip.clipboard_atom, This->xclip.targets_atom,
						  This->xclip.rdesktop_clipboard_target_atom, This->wnd,
						  event->time);
			}
		}

		return;
	}

	if (This->xclip.probing_selections && This->xclip.reprobe_selections)
	{
		This->xclip.probing_selections = False;
		xclip_probe_selections(This);
		return;
	}

	res = XGetWindowProperty(This->display, This->wnd, This->xclip.rdesktop_clipboard_target_atom,
				 0, XMaxRequestSize(This->display), False, AnyPropertyType,
				 &type, &format, &nitems, &bytes_left, &data);

	xclip_clear_target_props(This);

	if (res != Success)
	{
		DEBUG_CLIPBOARD(("XGetWindowProperty failed!\n"));
		goto fail;
	}

	if (type == This->xclip.incr_atom)
	{
		DEBUG_CLIPBOARD(("Received INCR.\n"));

		XGetWindowAttributes(This->display, This->wnd, &wa);
		if ((wa.your_event_mask | PropertyChangeMask) != wa.your_event_mask)
		{
			XSelectInput(This->display, This->wnd, (wa.your_event_mask | PropertyChangeMask));
		}
		XFree(data);
		This->xclip.incr_target = event->target;
		This->xclip.waiting_for_INCR = 1;
		goto end;
	}

	/* Negotiate target format */
	if (event->target == This->xclip.targets_atom)
	{
		/* Determine the best of text This->xclip.targets that we have available:
		   Prefer UTF8_STRING > text/unicode (unspecified encoding) > STRING
		   (ignore TEXT and COMPOUND_TEXT because we don't have code to handle them)
		 */
		int text_target_satisfaction = 0;
		Atom best_text_target = 0;	/* measures how much we're satisfied with what we found */
		if (type != None)
		{
			supported_targets = (Atom *) data;
			for (i = 0; i < nitems; i++)
			{
				DEBUG_CLIPBOARD(("Target %d: %s\n", i,
						 XGetAtomName(This->display, supported_targets[i])));
				if (supported_targets[i] == This->xclip.format_string_atom)
				{
					if (text_target_satisfaction < 1)
					{
						DEBUG_CLIPBOARD(("Other party supports STRING, choosing that as best_target\n"));
						best_text_target = supported_targets[i];
						text_target_satisfaction = 1;
					}
				}
#ifdef USE_UNICODE_CLIPBOARD
				else if (supported_targets[i] == This->xclip.format_unicode_atom)
				{
					if (text_target_satisfaction < 2)
					{
						DEBUG_CLIPBOARD(("Other party supports text/unicode, choosing that as best_target\n"));
						best_text_target = supported_targets[i];
						text_target_satisfaction = 2;
					}
				}
				else if (supported_targets[i] == This->xclip.format_utf8_string_atom)
				{
					if (text_target_satisfaction < 3)
					{
						DEBUG_CLIPBOARD(("Other party supports UTF8_STRING, choosing that as best_target\n"));
						best_text_target = supported_targets[i];
						text_target_satisfaction = 3;
					}
				}
#endif
				else if (supported_targets[i] == This->xclip.rdesktop_clipboard_formats_atom)
				{
					if (This->xclip.probing_selections && (text_target_satisfaction < 4))
					{
						DEBUG_CLIPBOARD(("Other party supports native formats, choosing that as best_target\n"));
						best_text_target = supported_targets[i];
						text_target_satisfaction = 4;
					}
				}
			}
		}

		/* Kickstarting the next step in the process of satisfying RDP's
		   clipboard request -- specifically, requesting the actual clipboard data.
		 */
		if ((best_text_target != 0)
		    && (!This->xclip.probing_selections
			|| (best_text_target == This->xclip.rdesktop_clipboard_formats_atom)))
		{
			XConvertSelection(This->display, event->selection, best_text_target,
					  This->xclip.rdesktop_clipboard_target_atom, This->wnd, event->time);
			goto end;
		}
		else
		{
			DEBUG_CLIPBOARD(("Unable to find a textual target to satisfy RDP clipboard text request\n"));
			goto fail;
		}
	}
	else
	{
		if (This->xclip.probing_selections)
		{
			Window primary_owner, clipboard_owner;

			/* FIXME:
			   Without XFIXES, we must make sure that the other
			   rdesktop owns all relevant selections or we might try
			   to get a native format from non-rdesktop window later
			   on. */

			clipboard_owner = XGetSelectionOwner(This->display, This->xclip.clipboard_atom);

			if (This->xclip.auto_mode)
				primary_owner = XGetSelectionOwner(This->display, This->xclip.primary_atom);
			else
				primary_owner = clipboard_owner;

			if (primary_owner != clipboard_owner)
				goto fail;

			DEBUG_CLIPBOARD(("Got fellow rdesktop formats\n"));
			This->xclip.probing_selections = False;
			This->xclip.rdesktop_is_selection_owner = True;
			cliprdr_send_native_format_announce(This, data, nitems);
		}
		else if (!xclip_send_data_with_convert(This, data, nitems, event->target))
		{
			goto fail;
		}
	}

      end:
	if (data)
		XFree(data);

	return;

      fail:
	xclip_clear_target_props(This);
	if (This->xclip.probing_selections)
	{
		DEBUG_CLIPBOARD(("Unable to find suitable target. Using default text format.\n"));
		This->xclip.probing_selections = False;
		This->xclip.rdesktop_is_selection_owner = False;

		/* FIXME:
		   Without XFIXES, we cannot reliably know the formats offered by an
		   upcoming selection owner, so we just lie about him offering
		   RDP_CF_TEXT. */
		cliprdr_send_simple_native_format_announce(This, RDP_CF_TEXT);
	}
	else
	{
		helper_cliprdr_send_empty_response(This);
	}
	goto end;
}

/* This function is called for SelectionRequest events.
   The SelectionRequest event is sent from the requestor to the clipboard owner
   to request clipboard data.
 */
void
xclip_handle_SelectionRequest(RDPCLIENT * This, XSelectionRequestEvent * event)
{
	unsigned long nitems, bytes_left;
	unsigned char *prop_return;
	int format, res;
	Atom type;

	DEBUG_CLIPBOARD(("xclip_handle_SelectionRequest: selection=%s, target=%s, property=%s\n",
			 XGetAtomName(This->display, event->selection),
			 XGetAtomName(This->display, event->target),
			 XGetAtomName(This->display, event->property)));

	if (event->target == This->xclip.targets_atom)
	{
		xclip_provide_selection(This, event, XA_ATOM, 32, (uint8 *) & This->xclip.targets, This->xclip.num_targets);
		return;
	}
	else if (event->target == This->xclip.timestamp_atom)
	{
		xclip_provide_selection(This, event, XA_INTEGER, 32, (uint8 *) & This->xclip.acquire_time, 1);
		return;
	}
	else if (event->target == This->xclip.rdesktop_clipboard_formats_atom)
	{
		xclip_provide_selection(This, event, XA_STRING, 8, This->xclip.formats_data, This->xclip.formats_data_length);
	}
	else
	{
		/* All the following This->xclip.targets require an async operation with the RDP server
		   and currently we don't do X clipboard request queueing so we can only
		   handle one such request at a time. */
		if (This->xclip.has_selection_request)
		{
			DEBUG_CLIPBOARD(("Error: Another clipboard request was already sent to the RDP server and not yet responded. Refusing this request.\n"));
			xclip_refuse_selection(This, event);
			return;
		}
		if (event->target == This->xclip.rdesktop_native_atom)
		{
			/* Before the requestor makes a request for the _RDESKTOP_NATIVE target,
			   he should declare requestor[property] = CF_SOMETHING. */
			res = XGetWindowProperty(This->display, event->requestor,
						 event->property, 0, 1, True,
						 XA_INTEGER, &type, &format, &nitems, &bytes_left,
						 &prop_return);
			if (res != Success)
			{
				DEBUG_CLIPBOARD(("Requested native format but didn't specifiy which.\n"));
				xclip_refuse_selection(This, event);
				return;
			}

			format = *(uint32 *) prop_return;
			XFree(prop_return);
		}
		else if (event->target == This->xclip.format_string_atom || event->target == XA_STRING)
		{
			/* STRING and XA_STRING are defined to be ISO8859-1 */
			format = CF_TEXT;
		}
		else if (event->target == This->xclip.format_utf8_string_atom)
		{
#ifdef USE_UNICODE_CLIPBOARD
			format = CF_UNICODETEXT;
#else
			DEBUG_CLIPBOARD(("Requested target unavailable due to lack of Unicode support. (It was not in TARGETS, so why did you ask for it?!)\n"));
			xclip_refuse_selection(This, event);
			return;
#endif
		}
		else if (event->target == This->xclip.format_unicode_atom)
		{
			/* Assuming text/unicode to be UTF-16 */
			format = CF_UNICODETEXT;
		}
		else
		{
			DEBUG_CLIPBOARD(("Requested target unavailable. (It was not in TARGETS, so why did you ask for it?!)\n"));
			xclip_refuse_selection(This, event);
			return;
		}

		cliprdr_send_data_request(This, format);
		This->xclip.selection_request = *event;
		This->xclip.has_selection_request = True;
		return;		/* wait for data */
	}
}

/* While this rdesktop holds ownership over the clipboard, it means the clipboard data
   is offered by the RDP server (and when it is pasted inside RDP, there's no network
   roundtrip).

   This event (SelectionClear) symbolizes this rdesktop lost onwership of the clipboard
   to some other X client. We should find out what clipboard formats this other
   client offers and announce that to RDP. */
void
xclip_handle_SelectionClear(RDPCLIENT * This)
{
	DEBUG_CLIPBOARD(("xclip_handle_SelectionClear\n"));
	xclip_notify_change(This);
	xclip_probe_selections(This);
}

/* Called when any property changes in our window or the root window. */
void
xclip_handle_PropertyNotify(RDPCLIENT * This, XPropertyEvent * event)
{
	unsigned long nitems;
	unsigned long offset = 0;
	unsigned long bytes_left = 1;
	int format;
	XWindowAttributes wa;
	uint8 *data;
	Atom type;

	if (event->state == PropertyNewValue && This->xclip.waiting_for_INCR)
	{
		DEBUG_CLIPBOARD(("x_clip_handle_PropertyNotify: This->xclip.waiting_for_INCR != 0\n"));

		while (bytes_left > 0)
		{
			/* Unlike the specification, we don't set the 'delete' arugment to True
			   since we slurp the INCR's chunks in even-smaller chunks of 4096 bytes. */
			if ((XGetWindowProperty
			     (This->display, This->wnd, This->xclip.rdesktop_clipboard_target_atom, offset, 4096L,
			      False, AnyPropertyType, &type, &format, &nitems, &bytes_left,
			      &data) != Success))
			{
				XFree(data);
				return;
			}

			if (nitems == 0)
			{
				/* INCR transfer finished */
				XGetWindowAttributes(This->display, This->wnd, &wa);
				XSelectInput(This->display, This->wnd,
					     (wa.your_event_mask ^ PropertyChangeMask));
				XFree(data);
				This->xclip.waiting_for_INCR = 0;

				if (This->xclip.clip_buflen > 0)
				{
					if (!xclip_send_data_with_convert
					    (This, This->xclip.clip_buffer, This->xclip.clip_buflen, This->xclip.incr_target))
					{
						helper_cliprdr_send_empty_response(This);
					}
					xfree(This->xclip.clip_buffer);
					This->xclip.clip_buffer = NULL;
					This->xclip.clip_buflen = 0;
				}
			}
			else
			{
				/* Another chunk in the INCR transfer */
				offset += (nitems / 4);	/* offset at which to begin the next slurp */
				This->xclip.clip_buffer = xrealloc(This->xclip.clip_buffer, This->xclip.clip_buflen + nitems);
				memcpy(This->xclip.clip_buffer + This->xclip.clip_buflen, data, nitems);
				This->xclip.clip_buflen += nitems;

				XFree(data);
			}
		}
		XDeleteProperty(This->display, This->wnd, This->xclip.rdesktop_clipboard_target_atom);
		return;
	}

	if ((event->atom == This->xclip.rdesktop_selection_notify_atom) &&
	    (event->window == DefaultRootWindow(This->display)))
		xclip_probe_selections(This);
}
#endif


/* Called when the RDP server announces new clipboard data formats.
   In response, we:
   - take ownership over the clipboard
   - declare those formats in their Windows native form
     to other rdesktop instances on this X server */
void
ui_clip_format_announce(RDPCLIENT * This, uint8 * data, uint32 length)
{
	This->xclip.acquire_time = This->last_gesturetime;

	XSetSelectionOwner(This->display, This->xclip.primary_atom, This->wnd, This->xclip.acquire_time);
	if (XGetSelectionOwner(This->display, This->xclip.primary_atom) != This->wnd)
		warning("Failed to aquire ownership of PRIMARY clipboard\n");

	XSetSelectionOwner(This->display, This->xclip.clipboard_atom, This->wnd, This->xclip.acquire_time);
	if (XGetSelectionOwner(This->display, This->xclip.clipboard_atom) != This->wnd)
		warning("Failed to aquire ownership of CLIPBOARD clipboard\n");

	if (This->xclip.formats_data)
		xfree(This->xclip.formats_data);
	This->xclip.formats_data = xmalloc(length);
	memcpy(This->xclip.formats_data, data, length);
	This->xclip.formats_data_length = length;

	xclip_notify_change(This);
}

/* Called when the RDP server responds with clipboard data (after we've requested it). */
void
ui_clip_handle_data(RDPCLIENT * This, uint8 * data, uint32 length)
{
	BOOL free_data = False;

	if (length == 0)
	{
		xclip_refuse_selection(This, &This->xclip.selection_request);
		This->xclip.has_selection_request = False;
		return;
	}

	if (This->xclip.selection_request.target == This->xclip.format_string_atom || This->xclip.selection_request.target == XA_STRING)
	{
		/* We're expecting a CF_TEXT response */
		uint8 *firstnull;

		/* translate linebreaks */
		crlf2lf(data, &length);

		/* Only send data up to null byte, if any */
		firstnull = (uint8 *) strchr((char *) data, '\0');
		if (firstnull)
		{
			length = firstnull - data + 1;
		}
	}
#ifdef USE_UNICODE_CLIPBOARD
	else if (This->xclip.selection_request.target == This->xclip.format_utf8_string_atom)
	{
		/* We're expecting a CF_UNICODETEXT response */
		iconv_t cd = iconv_open("UTF-8", WINDOWS_CODEPAGE);
		if (cd != (iconv_t) - 1)
		{
			size_t utf8_length = length * 2;
			char *utf8_data = malloc(utf8_length);
			size_t utf8_length_remaining = utf8_length;
			char *utf8_data_remaining = utf8_data;
			char *data_remaining = (char *) data;
			size_t length_remaining = (size_t) length;
			if (utf8_data == NULL)
			{
				iconv_close(cd);
				return;
			}
			iconv(cd, (ICONV_CONST char **) &data_remaining, &length_remaining,
			      &utf8_data_remaining, &utf8_length_remaining);
			iconv_close(cd);
			free_data = True;
			data = (uint8 *) utf8_data;
			length = utf8_length - utf8_length_remaining;
		}
	}
	else if (This->xclip.selection_request.target == This->xclip.format_unicode_atom)
	{
		/* We're expecting a CF_UNICODETEXT response, so what we're
		   receiving matches our requirements and there's no need
		   for further conversions. */
	}
#endif
	else if (This->xclip.selection_request.target == This->xclip.rdesktop_native_atom)
	{
		/* Pass as-is */
	}
	else
	{
		DEBUG_CLIPBOARD(("ui_clip_handle_data: BUG! I don't know how to convert selection target %s!\n", XGetAtomName(This->display, This->xclip.selection_request.target)));
		xclip_refuse_selection(This, &This->xclip.selection_request);
		This->xclip.has_selection_request = False;
		return;
	}

	xclip_provide_selection(This, &This->xclip.selection_request, This->xclip.selection_request.target, 8, data, length - 1);
	This->xclip.has_selection_request = False;

	if (free_data)
		free(data);
}

void
ui_clip_request_failed(RDPCLIENT * This)
{
	xclip_refuse_selection(This, &This->xclip.selection_request);
	This->xclip.has_selection_request = False;
}

void
ui_clip_request_data(RDPCLIENT * This, uint32 format)
{
	Window primary_owner, clipboard_owner;

	DEBUG_CLIPBOARD(("Request from server for format %d\n", format));
	This->xclip.rdp_clipboard_request_format = format;

	if (This->xclip.probing_selections)
	{
		DEBUG_CLIPBOARD(("ui_clip_request_data: Selection probe in progress. Cannot handle request.\n"));
		helper_cliprdr_send_empty_response(This);
		return;
	}

	xclip_clear_target_props(This);

	if (This->xclip.rdesktop_is_selection_owner)
	{
		XChangeProperty(This->display, This->wnd, This->xclip.rdesktop_clipboard_target_atom,
				XA_INTEGER, 32, PropModeReplace, (unsigned char *) &format, 1);

		XConvertSelection(This->display, This->xclip.primary_atom, This->xclip.rdesktop_native_atom,
				  This->xclip.rdesktop_clipboard_target_atom, This->wnd, CurrentTime);
		return;
	}

	if (This->xclip.auto_mode)
		primary_owner = XGetSelectionOwner(This->display, This->xclip.primary_atom);
	else
		primary_owner = None;

	clipboard_owner = XGetSelectionOwner(This->display, This->xclip.clipboard_atom);

	/* Both available */
	if ((primary_owner != None) && (clipboard_owner != None))
	{
		This->xclip.primary_timestamp = 0;
		This->xclip.clipboard_timestamp = 0;
		XConvertSelection(This->display, This->xclip.primary_atom, This->xclip.timestamp_atom,
				  This->xclip.rdesktop_primary_timestamp_target_atom, This->wnd, CurrentTime);
		XConvertSelection(This->display, This->xclip.clipboard_atom, This->xclip.timestamp_atom,
				  This->xclip.rdesktop_clipboard_timestamp_target_atom, This->wnd, CurrentTime);
		return;
	}

	/* Just PRIMARY */
	if (primary_owner != None)
	{
		XConvertSelection(This->display, This->xclip.primary_atom, This->xclip.targets_atom,
				  This->xclip.rdesktop_clipboard_target_atom, This->wnd, CurrentTime);
		return;
	}

	/* Just CLIPBOARD */
	if (clipboard_owner != None)
	{
		XConvertSelection(This->display, This->xclip.clipboard_atom, This->xclip.targets_atom,
				  This->xclip.rdesktop_clipboard_target_atom, This->wnd, CurrentTime);
		return;
	}

	/* No data available */
	helper_cliprdr_send_empty_response(This);
}

void
ui_clip_sync(RDPCLIENT * This)
{
	xclip_probe_selections(This);
}

void
ui_clip_set_mode(RDPCLIENT * This, const char *optarg)
{
	This->rdpclip = True;

	if (str_startswith(optarg, "PRIMARYCLIPBOARD"))
		This->xclip.auto_mode = True;
	else if (str_startswith(optarg, "CLIPBOARD"))
		This->xclip.auto_mode = False;
	else
	{
		warning("Invalid clipboard mode '%s'.\n", optarg);
		This->rdpclip = False;
	}
}

void
xclip_init(RDPCLIENT * This)
{
	if (!This->rdpclip)
		return;

	if (!cliprdr_init(This))
		return;

	This->xclip.primary_atom = XInternAtom(This->display, "PRIMARY", False);
	This->xclip.clipboard_atom = XInternAtom(This->display, "CLIPBOARD", False);
	This->xclip.targets_atom = XInternAtom(This->display, "TARGETS", False);
	This->xclip.timestamp_atom = XInternAtom(This->display, "TIMESTAMP", False);
	This->xclip.rdesktop_clipboard_target_atom =
		XInternAtom(This->display, "_RDESKTOP_CLIPBOARD_TARGET", False);
	This->xclip.rdesktop_primary_timestamp_target_atom =
		XInternAtom(This->display, "_RDESKTOP_PRIMARY_TIMESTAMP_TARGET", False);
	This->xclip.rdesktop_clipboard_timestamp_target_atom =
		XInternAtom(This->display, "_RDESKTOP_CLIPBOARD_TIMESTAMP_TARGET", False);
	This->xclip.incr_atom = XInternAtom(This->display, "INCR", False);
	This->xclip.format_string_atom = XInternAtom(This->display, "STRING", False);
	This->xclip.format_utf8_string_atom = XInternAtom(This->display, "UTF8_STRING", False);
	This->xclip.format_unicode_atom = XInternAtom(This->display, "text/unicode", False);

	/* rdesktop sets _RDESKTOP_SELECTION_NOTIFY on the root window when acquiring the clipboard.
	   Other interested rdesktops can use this to notify their server of the available formats. */
	This->xclip.rdesktop_selection_notify_atom =
		XInternAtom(This->display, "_RDESKTOP_SELECTION_NOTIFY", False);
	XSelectInput(This->display, DefaultRootWindow(This->display), PropertyChangeMask);
	This->xclip.probing_selections = False;

	This->xclip.rdesktop_native_atom = XInternAtom(This->display, "_RDESKTOP_NATIVE", False);
	This->xclip.rdesktop_clipboard_formats_atom =
		XInternAtom(This->display, "_RDESKTOP_CLIPBOARD_FORMATS", False);
	This->xclip.rdesktop_primary_owner_atom = XInternAtom(This->display, "_RDESKTOP_PRIMARY_OWNER", False);
	This->xclip.rdesktop_clipboard_owner_atom = XInternAtom(This->display, "_RDESKTOP_CLIPBOARD_OWNER", False);

	This->xclip.num_targets = 0;
	This->xclip.targets[This->xclip.num_targets++] = This->xclip.targets_atom;
	This->xclip.targets[This->xclip.num_targets++] = This->xclip.timestamp_atom;
	This->xclip.targets[This->xclip.num_targets++] = This->xclip.rdesktop_native_atom;
	This->xclip.targets[This->xclip.num_targets++] = This->xclip.rdesktop_clipboard_formats_atom;
#ifdef USE_UNICODE_CLIPBOARD
	This->xclip.targets[This->xclip.num_targets++] = This->xclip.format_utf8_string_atom;
#endif
	This->xclip.targets[This->xclip.num_targets++] = This->xclip.format_unicode_atom;
	This->xclip.targets[This->xclip.num_targets++] = This->xclip.format_string_atom;
	This->xclip.targets[This->xclip.num_targets++] = XA_STRING;
}

void
xclip_deinit(RDPCLIENT * This)
{
	if (XGetSelectionOwner(This->display, This->xclip.primary_atom) == This->wnd)
		XSetSelectionOwner(This->display, This->xclip.primary_atom, None, This->xclip.acquire_time);
	if (XGetSelectionOwner(This->display, This->xclip.clipboard_atom) == This->wnd)
		XSetSelectionOwner(This->display, This->xclip.clipboard_atom, None, This->xclip.acquire_time);
	xclip_notify_change(This);
}
