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

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
    http://msdn.microsoft.com/library/en-us/winui/winui/windowsuserinterface/dataexchange/clipboard/clipboardformats.asp
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

#define MAX_TARGETS 8

extern Display *g_display;
extern Window g_wnd;
extern Time g_last_gesturetime;
extern BOOL g_rdpclip;

/* Mode of operation.
   - Auto: Look at both PRIMARY and CLIPBOARD and use the most recent.
   - Non-auto: Look at just CLIPBOARD. */
static BOOL auto_mode = True;
/* Atoms of the two X selections we're dealing with: CLIPBOARD (explicit-copy) and PRIMARY (selection-copy) */
static Atom clipboard_atom, primary_atom;
/* Atom of the TARGETS clipboard target */
static Atom targets_atom;
/* Atom of the TIMESTAMP clipboard target */
static Atom timestamp_atom;
/* Atom _RDESKTOP_CLIPBOARD_TARGET which is used as the 'property' argument in
   XConvertSelection calls: This is the property of our window into which
   XConvertSelection will store the received clipboard data. */
static Atom rdesktop_clipboard_target_atom;
/* Atoms _RDESKTOP_PRIMARY_TIMESTAMP_TARGET and _RDESKTOP_CLIPBOARD_TIMESTAMP_TARGET
   are used to store the timestamps for when a window got ownership of the selections.
   We use these to determine which is more recent and should be used. */
static Atom rdesktop_primary_timestamp_target_atom, rdesktop_clipboard_timestamp_target_atom;
/* Storage for timestamps since we get them in two separate notifications. */
static Time primary_timestamp, clipboard_timestamp;
/* Clipboard target for getting a list of native Windows clipboard formats. The
   presence of this target indicates that the selection owner is another rdesktop. */
static Atom rdesktop_clipboard_formats_atom;
/* The clipboard target (X jargon for "clipboard format") for rdesktop-to-rdesktop
   interchange of Windows native clipboard data. The requestor must supply the
   desired native Windows clipboard format in the associated property. */
static Atom rdesktop_native_atom;
/* Local copy of the list of native Windows clipboard formats. */
static uint8 *formats_data = NULL;
static uint32 formats_data_length = 0;
/* We need to know when another rdesktop process gets or loses ownership of a
   selection. Without XFixes we do this by touching a property on the root window
   which will generate PropertyNotify notifications. */
static Atom rdesktop_selection_notify_atom;
/* State variables that indicate if we're currently probing the targets of the
   selection owner. reprobe_selections indicate that the ownership changed in
   the middle of the current probe so it should be restarted. */
static BOOL probing_selections, reprobe_selections;
/* Atoms _RDESKTOP_PRIMARY_OWNER and _RDESKTOP_CLIPBOARD_OWNER. Used as properties
   on the root window to indicate which selections that are owned by rdesktop. */
static Atom rdesktop_primary_owner_atom, rdesktop_clipboard_owner_atom;
static Atom format_string_atom, format_utf8_string_atom, format_unicode_atom;
/* Atom of the INCR clipboard type (see ICCCM on "INCR Properties") */
static Atom incr_atom;
/* Stores the last "selection request" (= another X client requesting clipboard data from us).
   To satisfy such a request, we request the clipboard data from the RDP server.
   When we receive the response from the RDP server (asynchronously), this variable gives us
   the context to proceed. */
static XSelectionRequestEvent selection_request;
/* Denotes we have a pending selection request. */
static Bool has_selection_request;
/* Stores the clipboard format (CF_TEXT, CF_UNICODETEXT etc.) requested in the last
   CLIPDR_DATA_REQUEST (= the RDP server requesting clipboard data from us).
   When we receive this data from whatever X client offering it, this variable gives us
   the context to proceed.
 */
static int rdp_clipboard_request_format;
/* Array of offered clipboard targets that will be sent to fellow X clients upon a TARGETS request. */
static Atom targets[MAX_TARGETS];
static int num_targets;
/* Denotes that an rdesktop (not this rdesktop) is owning the selection,
   allowing us to interchange Windows native clipboard data directly. */
static BOOL rdesktop_is_selection_owner = False;
/* Time when we acquired the selection. */
static Time acquire_time = 0;

/* Denotes that an INCR ("chunked") transfer is in progress. */
static int g_waiting_for_INCR = 0;
/* Denotes the target format of the ongoing INCR ("chunked") transfer. */
static Atom g_incr_target = 0;
/* Buffers an INCR transfer. */
static uint8 *g_clip_buffer = 0;
/* Denotes the size of g_clip_buffer. */
static uint32 g_clip_buflen = 0;

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
xclip_provide_selection(XSelectionRequestEvent * req, Atom type, unsigned int format, uint8 * data,
			uint32 length)
{
	XEvent xev;

	DEBUG_CLIPBOARD(("xclip_provide_selection: requestor=0x%08x, target=%s, property=%s, length=%u\n", (unsigned) req->requestor, XGetAtomName(g_display, req->target), XGetAtomName(g_display, req->property), (unsigned) length));

	XChangeProperty(g_display, req->requestor, req->property,
			type, format, PropModeReplace, data, length);

	xev.xselection.type = SelectionNotify;
	xev.xselection.serial = 0;
	xev.xselection.send_event = True;
	xev.xselection.requestor = req->requestor;
	xev.xselection.selection = req->selection;
	xev.xselection.target = req->target;
	xev.xselection.property = req->property;
	xev.xselection.time = req->time;
	XSendEvent(g_display, req->requestor, False, NoEventMask, &xev);
}

/* Replies a clipboard requestor, telling that we're unable to satisfy his request for whatever reason.
   This has the benefit of finalizing the clipboard negotiation and thus not leaving our requestor
   lingering (and, potentially, stuck). */
static void
xclip_refuse_selection(XSelectionRequestEvent * req)
{
	XEvent xev;

	DEBUG_CLIPBOARD(("xclip_refuse_selection: requestor=0x%08x, target=%s, property=%s\n",
			 (unsigned) req->requestor, XGetAtomName(g_display, req->target),
			 XGetAtomName(g_display, req->property)));

	xev.xselection.type = SelectionNotify;
	xev.xselection.serial = 0;
	xev.xselection.send_event = True;
	xev.xselection.requestor = req->requestor;
	xev.xselection.selection = req->selection;
	xev.xselection.target = req->target;
	xev.xselection.property = None;
	xev.xselection.time = req->time;
	XSendEvent(g_display, req->requestor, False, NoEventMask, &xev);
}

/* Wrapper for cliprdr_send_data which also cleans the request state. */
static void
helper_cliprdr_send_response(uint8 * data, uint32 length)
{
	if (rdp_clipboard_request_format != 0)
	{
		cliprdr_send_data(data, length);
		rdp_clipboard_request_format = 0;
		if (!rdesktop_is_selection_owner)
			cliprdr_send_simple_native_format_announce(RDP_CF_TEXT);
	}
}

/* Last resort, when we have to provide clipboard data but for whatever
   reason couldn't get any.
 */
static void
helper_cliprdr_send_empty_response()
{
	helper_cliprdr_send_response(NULL, 0);
}

/* Replies with clipboard data to RDP, converting it from the target format
   to the expected RDP format as necessary. Returns true if data was sent.
 */
static Bool
xclip_send_data_with_convert(uint8 * source, size_t source_size, Atom target)
{
	DEBUG_CLIPBOARD(("xclip_send_data_with_convert: target=%s, size=%u\n",
			 XGetAtomName(g_display, target), (unsigned) source_size));

#ifdef USE_UNICODE_CLIPBOARD
	if (target == format_string_atom ||
	    target == format_unicode_atom || target == format_utf8_string_atom)
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

		if (rdp_clipboard_request_format != RDP_CF_TEXT)
			return False;

		/* Make an attempt to convert any string we send to Unicode.
		   We don't know what the RDP server's ANSI Codepage is, or how to convert
		   to it, so using CF_TEXT is not safe (and is unnecessary, since all
		   WinNT versions are Unicode-minded).
		 */
		if (target == format_string_atom)
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
		else if (target == format_unicode_atom)
		{
			cd = iconv_open(WINDOWS_CODEPAGE, "UCS-2");
			if (cd == (iconv_t) - 1)
			{
				return False;
			}
			unicode_buffer_size = source_size;
		}
		else if (target == format_utf8_string_atom)
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
			helper_cliprdr_send_response(translated_data, translated_data_size);
			xfree(translated_data);	/* Not the same thing as XFree! */
		}

		xfree(unicode_buffer);

		return True;
	}
#else
	if (target == format_string_atom)
	{
		uint8 *translated_data;
		uint32 length = source_size;

		if (rdp_clipboard_request_format != RDP_CF_TEXT)
			return False;

		DEBUG_CLIPBOARD(("Translating linebreaks before sending data\n"));
		translated_data = lf2crlf(source, &length);
		if (translated_data != NULL)
		{
			helper_cliprdr_send_response(translated_data, length);
			xfree(translated_data);	/* Not the same thing as XFree! */
		}

		return True;
	}
#endif
	else if (target == rdesktop_native_atom)
	{
		helper_cliprdr_send_response(source, source_size + 1);

		return True;
	}
	else
	{
		return False;
	}
}

static void
xclip_clear_target_props()
{
	XDeleteProperty(g_display, g_wnd, rdesktop_clipboard_target_atom);
	XDeleteProperty(g_display, g_wnd, rdesktop_primary_timestamp_target_atom);
	XDeleteProperty(g_display, g_wnd, rdesktop_clipboard_timestamp_target_atom);
}

static void
xclip_notify_change()
{
	XChangeProperty(g_display, DefaultRootWindow(g_display),
			rdesktop_selection_notify_atom, XA_INTEGER, 32, PropModeReplace, NULL, 0);
}

static void
xclip_probe_selections()
{
	Window primary_owner, clipboard_owner;

	if (probing_selections)
	{
		DEBUG_CLIPBOARD(("Already probing selections. Scheduling reprobe.\n"));
		reprobe_selections = True;
		return;
	}

	DEBUG_CLIPBOARD(("Probing selections.\n"));

	probing_selections = True;
	reprobe_selections = False;

	xclip_clear_target_props();

	if (auto_mode)
		primary_owner = XGetSelectionOwner(g_display, primary_atom);
	else
		primary_owner = None;

	clipboard_owner = XGetSelectionOwner(g_display, clipboard_atom);

	/* If we own all relevant selections then don't do anything. */
	if (((primary_owner == g_wnd) || !auto_mode) && (clipboard_owner == g_wnd))
		goto end;

	/* Both available */
	if ((primary_owner != None) && (clipboard_owner != None))
	{
		primary_timestamp = 0;
		clipboard_timestamp = 0;
		XConvertSelection(g_display, primary_atom, timestamp_atom,
				  rdesktop_primary_timestamp_target_atom, g_wnd, CurrentTime);
		XConvertSelection(g_display, clipboard_atom, timestamp_atom,
				  rdesktop_clipboard_timestamp_target_atom, g_wnd, CurrentTime);
		return;
	}

	/* Just PRIMARY */
	if (primary_owner != None)
	{
		XConvertSelection(g_display, primary_atom, targets_atom,
				  rdesktop_clipboard_target_atom, g_wnd, CurrentTime);
		return;
	}

	/* Just CLIPBOARD */
	if (clipboard_owner != None)
	{
		XConvertSelection(g_display, clipboard_atom, targets_atom,
				  rdesktop_clipboard_target_atom, g_wnd, CurrentTime);
		return;
	}

	DEBUG_CLIPBOARD(("No owner of any selection.\n"));

	/* FIXME:
	   Without XFIXES, we cannot reliably know the formats offered by an
	   upcoming selection owner, so we just lie about him offering
	   RDP_CF_TEXT. */
	cliprdr_send_simple_native_format_announce(RDP_CF_TEXT);

      end:
	probing_selections = False;
}

/* This function is called for SelectionNotify events.
   The SelectionNotify event is sent from the clipboard owner to the requestor
   after his request was satisfied.
   If this function is called, we're the requestor side. */
#ifndef MAKE_PROTO
void
xclip_handle_SelectionNotify(XSelectionEvent * event)
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
			 XGetAtomName(g_display, event->selection),
			 XGetAtomName(g_display, event->target),
			 XGetAtomName(g_display, event->property)));

	if (event->target == timestamp_atom)
	{
		if (event->selection == primary_atom)
		{
			res = XGetWindowProperty(g_display, g_wnd,
						 rdesktop_primary_timestamp_target_atom, 0,
						 XMaxRequestSize(g_display), False, AnyPropertyType,
						 &type, &format, &nitems, &bytes_left, &data);
		}
		else
		{
			res = XGetWindowProperty(g_display, g_wnd,
						 rdesktop_clipboard_timestamp_target_atom, 0,
						 XMaxRequestSize(g_display), False, AnyPropertyType,
						 &type, &format, &nitems, &bytes_left, &data);
		}


		if ((res != Success) || (nitems != 1) || (format != 32))
		{
			DEBUG_CLIPBOARD(("XGetWindowProperty failed!\n"));
			goto fail;
		}

		if (event->selection == primary_atom)
		{
			primary_timestamp = *(Time *) data;
			if (primary_timestamp == 0)
				primary_timestamp++;
			XDeleteProperty(g_display, g_wnd, rdesktop_primary_timestamp_target_atom);
			DEBUG_CLIPBOARD(("Got PRIMARY timestamp: %u\n",
					 (unsigned) primary_timestamp));
		}
		else
		{
			clipboard_timestamp = *(Time *) data;
			if (clipboard_timestamp == 0)
				clipboard_timestamp++;
			XDeleteProperty(g_display, g_wnd, rdesktop_clipboard_timestamp_target_atom);
			DEBUG_CLIPBOARD(("Got CLIPBOARD timestamp: %u\n",
					 (unsigned) clipboard_timestamp));
		}

		XFree(data);

		if (primary_timestamp && clipboard_timestamp)
		{
			if (primary_timestamp > clipboard_timestamp)
			{
				DEBUG_CLIPBOARD(("PRIMARY is most recent selection.\n"));
				XConvertSelection(g_display, primary_atom, targets_atom,
						  rdesktop_clipboard_target_atom, g_wnd,
						  event->time);
			}
			else
			{
				DEBUG_CLIPBOARD(("CLIPBOARD is most recent selection.\n"));
				XConvertSelection(g_display, clipboard_atom, targets_atom,
						  rdesktop_clipboard_target_atom, g_wnd,
						  event->time);
			}
		}

		return;
	}

	if (probing_selections && reprobe_selections)
	{
		probing_selections = False;
		xclip_probe_selections();
		return;
	}

	res = XGetWindowProperty(g_display, g_wnd, rdesktop_clipboard_target_atom,
				 0, XMaxRequestSize(g_display), False, AnyPropertyType,
				 &type, &format, &nitems, &bytes_left, &data);

	xclip_clear_target_props();

	if (res != Success)
	{
		DEBUG_CLIPBOARD(("XGetWindowProperty failed!\n"));
		goto fail;
	}

	if (type == incr_atom)
	{
		DEBUG_CLIPBOARD(("Received INCR.\n"));

		XGetWindowAttributes(g_display, g_wnd, &wa);
		if ((wa.your_event_mask | PropertyChangeMask) != wa.your_event_mask)
		{
			XSelectInput(g_display, g_wnd, (wa.your_event_mask | PropertyChangeMask));
		}
		XFree(data);
		g_incr_target = event->target;
		g_waiting_for_INCR = 1;
		goto end;
	}

	/* Negotiate target format */
	if (event->target == targets_atom)
	{
		/* Determine the best of text targets that we have available:
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
						 XGetAtomName(g_display, supported_targets[i])));
				if (supported_targets[i] == format_string_atom)
				{
					if (text_target_satisfaction < 1)
					{
						DEBUG_CLIPBOARD(("Other party supports STRING, choosing that as best_target\n"));
						best_text_target = supported_targets[i];
						text_target_satisfaction = 1;
					}
				}
#ifdef USE_UNICODE_CLIPBOARD
				else if (supported_targets[i] == format_unicode_atom)
				{
					if (text_target_satisfaction < 2)
					{
						DEBUG_CLIPBOARD(("Other party supports text/unicode, choosing that as best_target\n"));
						best_text_target = supported_targets[i];
						text_target_satisfaction = 2;
					}
				}
				else if (supported_targets[i] == format_utf8_string_atom)
				{
					if (text_target_satisfaction < 3)
					{
						DEBUG_CLIPBOARD(("Other party supports UTF8_STRING, choosing that as best_target\n"));
						best_text_target = supported_targets[i];
						text_target_satisfaction = 3;
					}
				}
#endif
				else if (supported_targets[i] == rdesktop_clipboard_formats_atom)
				{
					if (probing_selections && (text_target_satisfaction < 4))
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
		    && (!probing_selections
			|| (best_text_target == rdesktop_clipboard_formats_atom)))
		{
			XConvertSelection(g_display, event->selection, best_text_target,
					  rdesktop_clipboard_target_atom, g_wnd, event->time);
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
		if (probing_selections)
		{
			Window primary_owner, clipboard_owner;

			/* FIXME:
			   Without XFIXES, we must make sure that the other
			   rdesktop owns all relevant selections or we might try
			   to get a native format from non-rdesktop window later
			   on. */

			clipboard_owner = XGetSelectionOwner(g_display, clipboard_atom);

			if (auto_mode)
				primary_owner = XGetSelectionOwner(g_display, primary_atom);
			else
				primary_owner = clipboard_owner;

			if (primary_owner != clipboard_owner)
				goto fail;

			DEBUG_CLIPBOARD(("Got fellow rdesktop formats\n"));
			probing_selections = False;
			rdesktop_is_selection_owner = True;
			cliprdr_send_native_format_announce(data, nitems);
		}
		else if (!xclip_send_data_with_convert(data, nitems, event->target))
		{
			goto fail;
		}
	}

      end:
	if (data)
		XFree(data);

	return;

      fail:
	xclip_clear_target_props();
	if (probing_selections)
	{
		DEBUG_CLIPBOARD(("Unable to find suitable target. Using default text format.\n"));
		probing_selections = False;
		rdesktop_is_selection_owner = False;

		/* FIXME:
		   Without XFIXES, we cannot reliably know the formats offered by an
		   upcoming selection owner, so we just lie about him offering
		   RDP_CF_TEXT. */
		cliprdr_send_simple_native_format_announce(RDP_CF_TEXT);
	}
	else
	{
		helper_cliprdr_send_empty_response();
	}
	goto end;
}

/* This function is called for SelectionRequest events.
   The SelectionRequest event is sent from the requestor to the clipboard owner
   to request clipboard data.
 */
void
xclip_handle_SelectionRequest(XSelectionRequestEvent * event)
{
	unsigned long nitems, bytes_left;
	unsigned char *prop_return;
	int format, res;
	Atom type;

	DEBUG_CLIPBOARD(("xclip_handle_SelectionRequest: selection=%s, target=%s, property=%s\n",
			 XGetAtomName(g_display, event->selection),
			 XGetAtomName(g_display, event->target),
			 XGetAtomName(g_display, event->property)));

	if (event->target == targets_atom)
	{
		xclip_provide_selection(event, XA_ATOM, 32, (uint8 *) & targets, num_targets);
		return;
	}
	else if (event->target == timestamp_atom)
	{
		xclip_provide_selection(event, XA_INTEGER, 32, (uint8 *) & acquire_time, 1);
		return;
	}
	else if (event->target == rdesktop_clipboard_formats_atom)
	{
		xclip_provide_selection(event, XA_STRING, 8, formats_data, formats_data_length);
	}
	else
	{
		/* All the following targets require an async operation with the RDP server
		   and currently we don't do X clipboard request queueing so we can only
		   handle one such request at a time. */
		if (has_selection_request)
		{
			DEBUG_CLIPBOARD(("Error: Another clipboard request was already sent to the RDP server and not yet responded. Refusing this request.\n"));
			xclip_refuse_selection(event);
			return;
		}
		if (event->target == rdesktop_native_atom)
		{
			/* Before the requestor makes a request for the _RDESKTOP_NATIVE target,
			   he should declare requestor[property] = CF_SOMETHING. */
			res = XGetWindowProperty(g_display, event->requestor,
						 event->property, 0, 1, True,
						 XA_INTEGER, &type, &format, &nitems, &bytes_left,
						 &prop_return);
			if (res != Success)
			{
				DEBUG_CLIPBOARD(("Requested native format but didn't specifiy which.\n"));
				xclip_refuse_selection(event);
				return;
			}

			format = *(uint32 *) prop_return;
			XFree(prop_return);
		}
		else if (event->target == format_string_atom || event->target == XA_STRING)
		{
			/* STRING and XA_STRING are defined to be ISO8859-1 */
			format = CF_TEXT;
		}
		else if (event->target == format_utf8_string_atom)
		{
#ifdef USE_UNICODE_CLIPBOARD
			format = CF_UNICODETEXT;
#else
			DEBUG_CLIPBOARD(("Requested target unavailable due to lack of Unicode support. (It was not in TARGETS, so why did you ask for it?!)\n"));
			xclip_refuse_selection(event);
			return;
#endif
		}
		else if (event->target == format_unicode_atom)
		{
			/* Assuming text/unicode to be UTF-16 */
			format = CF_UNICODETEXT;
		}
		else
		{
			DEBUG_CLIPBOARD(("Requested target unavailable. (It was not in TARGETS, so why did you ask for it?!)\n"));
			xclip_refuse_selection(event);
			return;
		}

		cliprdr_send_data_request(format);
		selection_request = *event;
		has_selection_request = True;
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
xclip_handle_SelectionClear(void)
{
	DEBUG_CLIPBOARD(("xclip_handle_SelectionClear\n"));
	xclip_notify_change();
	xclip_probe_selections();
}

/* Called when any property changes in our window or the root window. */
void
xclip_handle_PropertyNotify(XPropertyEvent * event)
{
	unsigned long nitems;
	unsigned long offset = 0;
	unsigned long bytes_left = 1;
	int format;
	XWindowAttributes wa;
	uint8 *data;
	Atom type;

	if (event->state == PropertyNewValue && g_waiting_for_INCR)
	{
		DEBUG_CLIPBOARD(("x_clip_handle_PropertyNotify: g_waiting_for_INCR != 0\n"));

		while (bytes_left > 0)
		{
			/* Unlike the specification, we don't set the 'delete' arugment to True
			   since we slurp the INCR's chunks in even-smaller chunks of 4096 bytes. */
			if ((XGetWindowProperty
			     (g_display, g_wnd, rdesktop_clipboard_target_atom, offset, 4096L,
			      False, AnyPropertyType, &type, &format, &nitems, &bytes_left,
			      &data) != Success))
			{
				XFree(data);
				return;
			}

			if (nitems == 0)
			{
				/* INCR transfer finished */
				XGetWindowAttributes(g_display, g_wnd, &wa);
				XSelectInput(g_display, g_wnd,
					     (wa.your_event_mask ^ PropertyChangeMask));
				XFree(data);
				g_waiting_for_INCR = 0;

				if (g_clip_buflen > 0)
				{
					if (!xclip_send_data_with_convert
					    (g_clip_buffer, g_clip_buflen, g_incr_target))
					{
						helper_cliprdr_send_empty_response();
					}
					xfree(g_clip_buffer);
					g_clip_buffer = NULL;
					g_clip_buflen = 0;
				}
			}
			else
			{
				/* Another chunk in the INCR transfer */
				offset += (nitems / 4);	/* offset at which to begin the next slurp */
				g_clip_buffer = xrealloc(g_clip_buffer, g_clip_buflen + nitems);
				memcpy(g_clip_buffer + g_clip_buflen, data, nitems);
				g_clip_buflen += nitems;

				XFree(data);
			}
		}
		XDeleteProperty(g_display, g_wnd, rdesktop_clipboard_target_atom);
		return;
	}

	if ((event->atom == rdesktop_selection_notify_atom) &&
	    (event->window == DefaultRootWindow(g_display)))
		xclip_probe_selections();
}
#endif


/* Called when the RDP server announces new clipboard data formats.
   In response, we:
   - take ownership over the clipboard
   - declare those formats in their Windows native form
     to other rdesktop instances on this X server */
void
ui_clip_format_announce(uint8 * data, uint32 length)
{
	acquire_time = g_last_gesturetime;

	XSetSelectionOwner(g_display, primary_atom, g_wnd, acquire_time);
	if (XGetSelectionOwner(g_display, primary_atom) != g_wnd)
		warning("Failed to aquire ownership of PRIMARY clipboard\n");

	XSetSelectionOwner(g_display, clipboard_atom, g_wnd, acquire_time);
	if (XGetSelectionOwner(g_display, clipboard_atom) != g_wnd)
		warning("Failed to aquire ownership of CLIPBOARD clipboard\n");

	if (formats_data)
		xfree(formats_data);
	formats_data = xmalloc(length);
	memcpy(formats_data, data, length);
	formats_data_length = length;

	xclip_notify_change();
}

/* Called when the RDP server responds with clipboard data (after we've requested it). */
void
ui_clip_handle_data(uint8 * data, uint32 length)
{
	BOOL free_data = False;

	if (length == 0)
	{
		xclip_refuse_selection(&selection_request);
		has_selection_request = False;
		return;
	}

	if (selection_request.target == format_string_atom || selection_request.target == XA_STRING)
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
	else if (selection_request.target == format_utf8_string_atom)
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
	else if (selection_request.target == format_unicode_atom)
	{
		/* We're expecting a CF_UNICODETEXT response, so what we're
		   receiving matches our requirements and there's no need
		   for further conversions. */
	}
#endif
	else if (selection_request.target == rdesktop_native_atom)
	{
		/* Pass as-is */
	}
	else
	{
		DEBUG_CLIPBOARD(("ui_clip_handle_data: BUG! I don't know how to convert selection target %s!\n", XGetAtomName(g_display, selection_request.target)));
		xclip_refuse_selection(&selection_request);
		has_selection_request = False;
		return;
	}

	xclip_provide_selection(&selection_request, selection_request.target, 8, data, length - 1);
	has_selection_request = False;

	if (free_data)
		free(data);
}

void
ui_clip_request_failed()
{
	xclip_refuse_selection(&selection_request);
	has_selection_request = False;
}

void
ui_clip_request_data(uint32 format)
{
	Window primary_owner, clipboard_owner;

	DEBUG_CLIPBOARD(("Request from server for format %d\n", format));
	rdp_clipboard_request_format = format;

	if (probing_selections)
	{
		DEBUG_CLIPBOARD(("ui_clip_request_data: Selection probe in progress. Cannot handle request.\n"));
		helper_cliprdr_send_empty_response();
		return;
	}

	xclip_clear_target_props();

	if (rdesktop_is_selection_owner)
	{
		XChangeProperty(g_display, g_wnd, rdesktop_clipboard_target_atom,
				XA_INTEGER, 32, PropModeReplace, (unsigned char *) &format, 1);

		XConvertSelection(g_display, primary_atom, rdesktop_native_atom,
				  rdesktop_clipboard_target_atom, g_wnd, CurrentTime);
		return;
	}

	if (auto_mode)
		primary_owner = XGetSelectionOwner(g_display, primary_atom);
	else
		primary_owner = None;

	clipboard_owner = XGetSelectionOwner(g_display, clipboard_atom);

	/* Both available */
	if ((primary_owner != None) && (clipboard_owner != None))
	{
		primary_timestamp = 0;
		clipboard_timestamp = 0;
		XConvertSelection(g_display, primary_atom, timestamp_atom,
				  rdesktop_primary_timestamp_target_atom, g_wnd, CurrentTime);
		XConvertSelection(g_display, clipboard_atom, timestamp_atom,
				  rdesktop_clipboard_timestamp_target_atom, g_wnd, CurrentTime);
		return;
	}

	/* Just PRIMARY */
	if (primary_owner != None)
	{
		XConvertSelection(g_display, primary_atom, targets_atom,
				  rdesktop_clipboard_target_atom, g_wnd, CurrentTime);
		return;
	}

	/* Just CLIPBOARD */
	if (clipboard_owner != None)
	{
		XConvertSelection(g_display, clipboard_atom, targets_atom,
				  rdesktop_clipboard_target_atom, g_wnd, CurrentTime);
		return;
	}

	/* No data available */
	helper_cliprdr_send_empty_response();
}

void
ui_clip_sync(void)
{
	xclip_probe_selections();
}

void
ui_clip_set_mode(const char *optarg)
{
	g_rdpclip = True;

	if (str_startswith(optarg, "PRIMARYCLIPBOARD"))
		auto_mode = True;
	else if (str_startswith(optarg, "CLIPBOARD"))
		auto_mode = False;
	else
	{
		warning("Invalid clipboard mode '%s'.\n", optarg);
		g_rdpclip = False;
	}
}

void
xclip_init(void)
{
	if (!g_rdpclip)
		return;

	if (!cliprdr_init())
		return;

	primary_atom = XInternAtom(g_display, "PRIMARY", False);
	clipboard_atom = XInternAtom(g_display, "CLIPBOARD", False);
	targets_atom = XInternAtom(g_display, "TARGETS", False);
	timestamp_atom = XInternAtom(g_display, "TIMESTAMP", False);
	rdesktop_clipboard_target_atom =
		XInternAtom(g_display, "_RDESKTOP_CLIPBOARD_TARGET", False);
	rdesktop_primary_timestamp_target_atom =
		XInternAtom(g_display, "_RDESKTOP_PRIMARY_TIMESTAMP_TARGET", False);
	rdesktop_clipboard_timestamp_target_atom =
		XInternAtom(g_display, "_RDESKTOP_CLIPBOARD_TIMESTAMP_TARGET", False);
	incr_atom = XInternAtom(g_display, "INCR", False);
	format_string_atom = XInternAtom(g_display, "STRING", False);
	format_utf8_string_atom = XInternAtom(g_display, "UTF8_STRING", False);
	format_unicode_atom = XInternAtom(g_display, "text/unicode", False);

	/* rdesktop sets _RDESKTOP_SELECTION_NOTIFY on the root window when acquiring the clipboard.
	   Other interested rdesktops can use this to notify their server of the available formats. */
	rdesktop_selection_notify_atom =
		XInternAtom(g_display, "_RDESKTOP_SELECTION_NOTIFY", False);
	XSelectInput(g_display, DefaultRootWindow(g_display), PropertyChangeMask);
	probing_selections = False;

	rdesktop_native_atom = XInternAtom(g_display, "_RDESKTOP_NATIVE", False);
	rdesktop_clipboard_formats_atom =
		XInternAtom(g_display, "_RDESKTOP_CLIPBOARD_FORMATS", False);
	rdesktop_primary_owner_atom = XInternAtom(g_display, "_RDESKTOP_PRIMARY_OWNER", False);
	rdesktop_clipboard_owner_atom = XInternAtom(g_display, "_RDESKTOP_CLIPBOARD_OWNER", False);

	num_targets = 0;
	targets[num_targets++] = targets_atom;
	targets[num_targets++] = timestamp_atom;
	targets[num_targets++] = rdesktop_native_atom;
	targets[num_targets++] = rdesktop_clipboard_formats_atom;
#ifdef USE_UNICODE_CLIPBOARD
	targets[num_targets++] = format_utf8_string_atom;
#endif
	targets[num_targets++] = format_unicode_atom;
	targets[num_targets++] = format_string_atom;
	targets[num_targets++] = XA_STRING;
}

void
xclip_deinit(void)
{
	if (XGetSelectionOwner(g_display, primary_atom) == g_wnd)
		XSetSelectionOwner(g_display, primary_atom, None, acquire_time);
	if (XGetSelectionOwner(g_display, clipboard_atom) == g_wnd)
		XSetSelectionOwner(g_display, clipboard_atom, None, acquire_time);
	xclip_notify_change();
}
