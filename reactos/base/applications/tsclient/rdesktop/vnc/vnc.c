/*
   rdesktop: A Remote Desktop Protocol client.
   User interface services - VNC target
   Copyright (C) Matthew Chapman 1999-2000
   Copyright (C) 2000 Tim Edmonds
   Copyright (C) 2001 James "Wez" Weatherall
   Copyright (C) 2001 Johannes E. Schindelin
   
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


#include <stdio.h>
#include <time.h>

#ifdef WIN32
#define close closesocket
#define strcasecmp _strcmpi
#else
#include <unistd.h>
#include <sys/time.h>		/* timeval */
#include <sys/socket.h>
#endif

#include "../rdesktop.h"
#undef VERSION

#ifdef WIN32
#define HBITMAP R_HBITMAP
#define HCURSOR R_HCURSOR
#define WORD R_WORD
#endif
#include "vnc.h"
#ifdef WIN32
#undef HBITMAP
#undef HCURSOR
#undef WORD
#endif

#include <errno.h>
#include <sys/socket.h>
extern int ListenOnTCPPort(int port);
extern int rfbClientSocket;

#include <rfb/rfbregion.h>

#define BITSPERBYTES 8
#define TOBYTES(bits) ((bits)/BITSPERBYTES)

extern int g_width;
extern int g_height;
extern int keylayout;
extern BOOL sendmotion;
#ifdef ENABLE_SHADOW
extern int client_counter;
#endif


int rfb_port = 5923;
int defer_time = 5;
int rfbClientSocket = 0;
static rfbScreenInfoPtr server = NULL;
static vncBuffer *frameBuffer = NULL;
static uint8_t reverseByte[0x100];
BOOL g_enable_compose = False;
int g_display = 0;

/* ignored */
BOOL owncolmap = False;
BOOL enable_compose = False;

void
vncHideCursor()
{
	if (server->clientHead)
		rfbUndrawCursor(server);
}

/* -=- mouseLookup
 * Table converting mouse button number (0-2) to flag
 */

int mouseLookup[3] = {
	MOUSE_FLAG_BUTTON1, MOUSE_FLAG_BUTTON3, MOUSE_FLAG_BUTTON2
};

int clipX, clipY, clipW, clipH;

BOOL
vncwinClipRect(int *x, int *y, int *cx, int *cy)
{
	if (*x + *cx > clipX + clipW)
		*cx = clipX + clipW - *x;
	if (*y + *cy > clipY + clipH)
		*cy = clipY + clipH - *y;
	if (*x < clipX)
	{
		*cx -= clipX - *x;
		*x = clipX;
	}
	if (*y < clipY)
	{
		*cy -= clipY - *y;
		*y = clipY;
	}
	if (*cx < 0 || *cy < 0)
		*cx = *cy = 0;
	return (*cx > 0 && *cy > 0 && *x < server->width && *y < server->height);
}

void
xwin_toggle_fullscreen(void)
{
}

static int lastbuttons = 0;

#define FIRST_MODIFIER XK_Shift_L
#define LAST_MODIFIER XK_Hyper_R

static BOOL keystate[LAST_MODIFIER - FIRST_MODIFIER];

void
init_keyboard()
{
	int i;
	for (i = 0; i < LAST_MODIFIER - FIRST_MODIFIER; i++)
		keystate[i] = 0;

	xkeymap_init();
}

BOOL
get_key_state(unsigned int state, uint32 keysym)
{
	if (keysym >= FIRST_MODIFIER && keysym <= LAST_MODIFIER)
		return keystate[keysym - FIRST_MODIFIER];
	return 0;
}

void
vncKey(rfbBool down, rfbKeySym keysym, struct _rfbClientRec *cl)
{
	uint32 ev_time = time(NULL);
	key_translation tr = { 0, 0 };

	if (keysym >= FIRST_MODIFIER && keysym <= LAST_MODIFIER)
	{
		/* TODO: fake local state */
		keystate[keysym - FIRST_MODIFIER] = down;
	}

	if (down)
	{
		/* TODO: fake local state */
		if (handle_special_keys(keysym, 0, ev_time, True))
			return;

		/* TODO: fake local state */
		tr = xkeymap_translate_key(keysym, 0, 0);

		if (tr.scancode == 0)
			return;

		ensure_remote_modifiers(ev_time, tr);

		rdp_send_scancode(ev_time, RDP_KEYPRESS, tr.scancode);
	}
	else
	{
		/* todO: fake local state */
		if (handle_special_keys(keysym, 0, ev_time, False))
			return;

		/* todO: fake local state */
		tr = xkeymap_translate_key(keysym, 0, 0);

		if (tr.scancode == 0)
			return;

		rdp_send_scancode(ev_time, RDP_KEYRELEASE, tr.scancode);
	}
}

void
vncMouse(int buttonMask, int x, int y, struct _rfbClientRec *cl)
{
	int b;
	uint32 ev_time = time(NULL);

	rdp_send_input(ev_time, RDP_INPUT_MOUSE, MOUSE_FLAG_MOVE, x, y);

	for (b = 0; b < 3; b++)
	{
		int bb = 1 << (b);
		if (!(lastbuttons & bb) && (buttonMask & bb))
		{
			rdp_send_input(ev_time, RDP_INPUT_MOUSE,
				       (mouseLookup[b]) | MOUSE_FLAG_DOWN, x, y);
		}
		else if ((lastbuttons & bb) && !(buttonMask & bb))
		{
			rdp_send_input(ev_time, RDP_INPUT_MOUSE, (mouseLookup[b]), x, y);
		}
	}
	lastbuttons = buttonMask;

	/* handle cursor */
	rfbDefaultPtrAddEvent(buttonMask, x, y, cl);
}


void
rdp2vnc_connect(char *server, uint32 flags, char *domain, char *password,
		char *shell, char *directory)
{
	struct sockaddr addr;
	fd_set fdset;
	struct timeval tv;
	int rfbListenSock, addrlen = sizeof(addr);

	rfbListenSock = rfbListenOnTCPPort(rfb_port);
	fprintf(stderr, "Listening on VNC port %d\n", rfb_port);
	if (rfbListenSock <= 0)
		error("Cannot listen on port %d", rfb_port);
	else
		while (1)
		{
			FD_ZERO(&fdset);
			FD_SET(rfbListenSock, &fdset);
			tv.tv_sec = 5;
			tv.tv_usec = 0;
			if (select(rfbListenSock + 1, &fdset, NULL, NULL, &tv) > 0)
			{
				rfbClientSocket = accept(rfbListenSock, &addr, &addrlen);
				if (rfbClientSocket < 0)
				{
					error("Error accepting client (%d: %s.\n",
					      errno, strerror(errno));
					continue;
				}
				ui_create_window();
				if (!rdp_connect(server, flags, domain, password, shell, directory))
				{
					error("Error connecting to RDP server.\n");
					continue;
				}
				if (!fork())
				{
					BOOL deactivated;
					uint32_t ext_disc_reason;
					printf("Connection successful.\n");
					rdp_main_loop(&deactivated, &ext_disc_reason);
					printf("Disconnecting...\n");
					rdp_disconnect();
					ui_destroy_window();
					exit(0);
				}
			}
		}
}





extern char g_title[];
BOOL
ui_create_window()
{
	int i;

	for (i = 0; i < 0x100; i++)
		reverseByte[i] =
			(((i >> 7) & 1)) | (((i >> 6) & 1) << 1) | (((i >> 5) & 1) << 2) |
			(((i >> 4) & 1) << 3) | (((i >> 3) & 1) << 4) | (((i >> 2) & 1) << 5) |
			(((i >> 1) & 1) << 6) | (((i >> 0) & 1) << 7);

	server = rfbGetScreen(0, NULL, g_width, g_height, 8, 1, 1);
	server->desktopName = g_title;
	server->frameBuffer = (char *) malloc(g_width * g_height);
	server->ptrAddEvent = vncMouse;
	server->kbdAddEvent = vncKey;
#ifdef ENABLE_SHADOW
	server->httpPort = 6124 + client_counter;
	server->port = 5924 + client_counter;
	rfbInitSockets(server);
	server->alwaysShared = TRUE;
	server->neverShared = FALSE;
#else
	server->port = -1;
	server->alwaysShared = FALSE;
	server->neverShared = FALSE;
#endif
	server->inetdSock = rfbClientSocket;
	server->serverFormat.trueColour = FALSE;	/* activate colour maps */
	server->deferUpdateTime = defer_time;

	frameBuffer = (vncBuffer *) malloc(sizeof(vncBuffer));
	frameBuffer->w = g_width;
	frameBuffer->h = g_height;
	frameBuffer->linew = g_width;
	frameBuffer->data = server->frameBuffer;
	frameBuffer->owner = FALSE;
	frameBuffer->format = &server->serverFormat;

	ui_set_clip(0, 0, g_width, g_height);

	rfbInitServer(server);
#ifndef ENABLE_SHADOW
	server->port = rfb_port;
#else
	fprintf(stderr, "server listening on port %d (socket %d)\n", server->port,
		server->listenSock);
#endif

	init_keyboard();

	return (server != NULL);
}

void
ui_destroy_window()
{
	rfbCloseClient(server->clientHead);
}


int
ui_select(int rdpSocket)
{
	fd_set fds;
	struct timeval tv;
	int n, m = server->maxFd;

	if (rdpSocket > m)
		m = rdpSocket;
	while (1)
	{
		fds = server->allFds;
		FD_SET(rdpSocket, &fds);
		tv.tv_sec = defer_time / 1000;
		tv.tv_usec = (defer_time % 1000) * 1000;
		n = select(m + 1, &fds, NULL, NULL, &tv);
		rfbProcessEvents(server, 0);
		/* if client is gone, close connection */
		if (!server->clientHead)
			close(rdpSocket);
		if (FD_ISSET(rdpSocket, &fds))
			return 1;
	}
	return 0;
}

void
ui_move_pointer(int x, int y)
{
	// TODO: Is there a way to send x,y even if cursor encoding is active?
	rfbUndrawCursor(server);
	server->cursorX = x;
	server->cursorY = y;
}

HBITMAP
ui_create_bitmap(int width, int height, uint8 * data)
{
	vncBuffer *buf;

	buf = vncNewBuffer(width, height, 8);
	memcpy(buf->data, data, width * height);

	return (HBITMAP) buf;
}

void
ui_paint_bitmap(int x, int y, int cx, int cy, int width, int height, uint8 * data)
{
	vncBuffer *buf;
	buf = ui_create_bitmap(width, height, data);
	vncCopyBlitFrom(server, x, y, cx, cy, buf, 0, 0);
	vncDeleteBuffer(buf);
}

void
ui_destroy_bitmap(HBITMAP bmp)
{
	vncDeleteBuffer((vncBuffer *) bmp);
}

uint8_t
vncLookupColour(rfbColourMap * colourMap, uint8_t * p)
{
	uint8_t i, i1 = 0;
	uint8_t *cm = colourMap->data.bytes;
	uint32_t m, m1 = abs(cm[0] - p[0]) + abs(cm[1] - p[1]) + abs(cm[2] - p[2]);
	for (i = 1; i < 255; i++)
	{
		m = abs(cm[i * 3] - p[0]) + abs(cm[i * 3 + 1] - p[1]) + abs(cm[i * 3 + 2] - p[2]);
		if (m < m1)
		{
			m1 = m;
			i1 = i;
		}
	}
	return (i1);
}

HCURSOR
ui_create_cursor(unsigned int x, unsigned int y, int width, int height, uint8 * mask, uint8 * data)
{
	int i, j;
	uint8_t *d0, *d1;
	uint8_t *cdata;
	uint8_t white[3] = { 0xff, 0xff, 0xff };
	uint8_t black[3] = { 0, 0, 0 };
	uint8_t *cur;
	rfbCursorPtr cursor;
	rfbColourMap *colourMap = &server->colourMap;

	cdata = xmalloc(sizeof(uint8_t) * width * height);
	d0 = xmalloc(sizeof(uint32_t) * width * height / 4);
	d1 = (uint8_t *) mask;
	for (j = 0; j < height; j++)
		for (i = 0; i < width / 8; i++)
		{
			d0[j * width / 8 + i] = d1[(height - 1 - j) * width / 8 + i] ^ 0xffffffff;
		}
	for (j = 0; j < height; j++)
	{
		for (i = 0; i < width; i++)
		{
			//strange that the pointer is in 24bit depth when everything
			//else is in 8bit palletized.
			cur = data + ((height - 1 - j) * width + i) * 3;
			if (cur[0] > 0x80 || cur[1] > 0x80 || cur[2] > 0x80)
			{
				if (!(d0[(j * width + i) / 8] & (0x80 >> (i & 7))))
				{
					/* text cursor! */
					cdata[j * width + i] = vncLookupColour(colourMap, black);
					d0[(j * width + i) / 8] |= 0x80 >> (i & 7);
				}
				else
					cdata[j * width + i] = vncLookupColour(colourMap, white);
			}
			else
				cdata[j * width + i] = vncLookupColour(colourMap, cur);
		}
	}
	cursor = (rfbCursorPtr) xmalloc(sizeof(rfbCursor));
	cursor->width = width;
	cursor->height = height;
	cursor->xhot = x;
	cursor->yhot = y;
	cursor->mask = (char *) d0;
	cursor->source = 0;
	cursor->richSource = cdata;
	cursor->cleanup = 0;	// workaround: this produces a memleak

	cursor->backRed = cursor->backGreen = cursor->backBlue = 0xffff;
	cursor->foreRed = cursor->foreGreen = cursor->foreBlue = 0;

	return (HCURSOR) cursor;
}

void
ui_set_cursor(HCURSOR cursor)
{
	/* FALSE means: don't delete old cursor */
	rfbSetCursor(server, (rfbCursorPtr) cursor, FALSE);
}

void
ui_destroy_cursor(HCURSOR cursor)
{
	if (cursor)
		rfbFreeCursor((rfbCursorPtr) cursor);
}

void
ui_set_null_cursor(void)
{
	rfbSetCursor(server, 0, FALSE);
}

HGLYPH
ui_create_glyph(int width, int height, uint8 * data)
{
	int x, y;
	vncBuffer *buf;

	buf = vncNewBuffer(width, height, 8);

	//data is padded to multiple of 16bit line lengths 
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			int byte = x / 8 + (y * ((width + 7) / 8));
			byte = rfbEndianTest ? reverseByte[data[byte]] : data[byte];
			byte = (byte >> (x & 7)) & 0x01;
			vncSetPixel(buf, x, y, byte ? 0x7f : 0x00);
		}
	}

	return (HGLYPH) buf;
}

void
ui_destroy_glyph(HGLYPH glyph)
{
	ui_destroy_bitmap((HBITMAP) glyph);
}

HCOLOURMAP
ui_create_colourmap(COLOURMAP * colours)
{
	int i;
	rfbColourMap *map = vncNewColourMap(server, colours->ncolours);
	for (i = 0; i < colours->ncolours; i++)
	{
		vncSetColourMapEntry(map, i, colours->colours[i].red,
				     colours->colours[i].green, colours->colours[i].blue);
	}
	return map;
}

void
ui_destroy_colourmap(HCOLOURMAP map)
{
	vncDeleteColourMap(map);
}

void
ui_set_colourmap(HCOLOURMAP map)
{
	vncSetColourMap(server, map);
}

void
ui_set_clip(int x, int y, int cx, int cy)
{
	clipX = x;
	clipY = y;
	clipW = cx;
	clipH = cy;
}

void
ui_reset_clip()
{
	clipX = 0;
	clipY = 0;
	clipW = 64000;
	clipH = 64000;
}

void
ui_bell()
{
	rfbSendBell(server);
}

void
ui_destblt(uint8 opcode,
	   /* dest */ int x, int y, int cx, int cy)
{
	int i;
	vncBuffer *buf;

	switch (opcode)
	{
		case 0:
		case 15:
			ui_rect(x, y, cx, cy, 0xff);
			break;
		case 5:	// invert
			buf = vncGetRect(server, x, y, cx, cy);
			for (i = 0; i < cx * cy; i++)
				((char *) (buf->data))[i] = !((char *) (buf->data))[i];
			break;
		default:
			unimpl("ui_destblt: opcode=%d %d,%d %dx%d\n", opcode, x, y, cx, cy);
	}
}

void
ui_patblt(uint8 opcode,
	  /* dest */ int x, int y, int cx, int cy,
	  /* brush */ BRUSH * brush, int bgcolour, int fgcolour)
{
	switch (brush->style)
	{
		case 0:	/* Solid */
			switch (opcode)
			{
				case ROP2_XOR:
					{
						int xx, yy;
						vncBuffer *fill = vncNewBuffer(cx, cy, 8);
						for (yy = 0; yy < cy; yy++)
							for (xx = 0; xx < cx; xx++)
								vncSetPixel(fill, xx, yy, fgcolour);
						if (vncwinClipRect(&x, &y, &cx, &cy))
							vncXorBlitFrom(server, x, y, cx, cy, fill,
								       0, 0);
						break;
					}

				default:
					if (vncwinClipRect(&x, &y, &cx, &cy))
						vncSetRect(server, x, y, cx, cy, fgcolour);
			}
			break;

		case 3:	/* Pattern */
			{
				int xx, yy;
				vncBuffer *fill;
				fill = (vncBuffer *) ui_create_glyph(8, 8, brush->pattern);

				for (yy = 0; yy < 8; yy++)
				{
					for (xx = 0; xx < 8; xx++)
					{
						vncSetPixel(fill, xx, yy,
							    vncGetPixel(fill, xx,
									yy) ? fgcolour : bgcolour);
					}
				}

				if (vncwinClipRect(&x, &y, &cx, &cy))
				{
					switch (opcode)
					{
						case ROP2_COPY:
							vncCopyBlitFrom(server, x, y, cx, cy, fill,
									0, 0);
							break;
						case ROP2_XOR:
							vncXorBlitFrom(server, x, y, cx, cy, fill,
								       0, 0);
							break;
						default:
							unimpl("pattern blit (%d,%d) opcode=%d bg=%d fg=%d\n", x, y, opcode, bgcolour, fgcolour);
							vncCopyBlitFrom(server, x, y, cx, cy, fill,
									0, 0);
							break;
					}
				}

				ui_destroy_glyph((HGLYPH) fill);
				break;

			}
		default:
			unimpl("brush %d\n", brush->style);
	}
}

void
ui_screenblt(uint8 opcode,
	     /* dest */ int x, int y, int cx, int cy,
	     /* src */ int srcx, int srcy)
{
	int ox, oy;

	ox = x;
	oy = y;
	if (vncwinClipRect(&x, &y, &cx, &cy))
	{
		//if we clipped top or left, we have to adjust srcx,srcy;
		srcx += x - ox;
		srcy += y - oy;
		vncCopyBlit(server, x, y, cx, cy, srcx, srcy);
	}
}

void
ui_memblt(uint8 opcode,
	  /* dest */ int x, int y, int cx, int cy,
	  /* src */ HBITMAP src, int srcx, int srcy)
{
	int ox, oy;
	ox = x;
	oy = y;

	if (vncwinClipRect(&x, &y, &cx, &cy))
	{
		//if we clipped top or left, we have to adjust srcx,srcy;
		srcx += x - ox;
		srcy += y - oy;
		switch (ROP2_S(opcode))
		{
			case ROP2_OR:
				vncTransBlitFrom(server, x, y, cx, cy, (vncBuffer *) src, srcx,
						 srcy, 0x0);
				break;
			case ROP2_XOR:
				vncXorBlitFrom(server, x, y, cx, cy, (vncBuffer *) src, srcx, srcy);
				break;
			case ROP2_AND:
				vncAndBlitFrom(server, x, y, cx, cy, (vncBuffer *) src, srcx, srcy);
				break;
			case ROP2_COPY:
				vncCopyBlitFrom(server, x, y, cx, cy, (vncBuffer *) src, srcx,
						srcy);
				break;
			default:
				unimpl("ui_memblt: op%d %d,%d %dx%d\n", opcode, x, y, cx, cy);
				vncCopyBlitFrom(server, x, y, cx, cy, (vncBuffer *) src, srcx,
						srcy);
				break;
		}
	}
}

void
ui_triblt(uint8 opcode,
	  /* dest */ int x, int y, int cx, int cy,
	  /* src */ HBITMAP src, int srcx, int srcy,
	  /* brush */ BRUSH * brush, int bgcolour, int fgcolour)
{
	/* This is potentially difficult to do in general. Until someone
	   comes up with a more efficient way of doing it I am using cases. */

	switch (opcode)
	{
		case 0x69:	/* PDSxxn */
			ui_memblt(ROP2_XOR, x, y, cx, cy, src, srcx, srcy);
			ui_patblt(ROP2_NXOR, x, y, cx, cy, brush, bgcolour, fgcolour);
			break;

		case 0xb8:	/* PSDPxax */
			ui_patblt(ROP2_XOR, x, y, cx, cy, brush, bgcolour, fgcolour);
			ui_memblt(ROP2_AND, x, y, cx, cy, src, srcx, srcy);
			ui_patblt(ROP2_XOR, x, y, cx, cy, brush, bgcolour, fgcolour);
			break;

		default:
			unimpl("ui_triblt 1x%x\n", opcode);
			ui_memblt(ROP2_COPY, x, y, cx, cy, src, srcx, srcy);
	}

}

void
ui_line(uint8 opcode,
	/* dest */ int startx, int starty, int endx, int endy,
	/* pen */ PEN * pen)
{
	//vncSetRect(server,startx,starty,1+endx-startx,endy-starty,pen->colour);
	//unimpl("drawline: pen colour=%d\n",pen->colour);
	/* TODO: implement opcodes */
	rfbDrawLine(server, startx, starty, endx, endy, pen->colour);
}

void
ui_rect(
	       /* dest */ int x, int y, int cx, int cy,
	       /* brush */ int colour)
{
	if (vncwinClipRect(&x, &y, &cx, &cy))
	{
		vncSetRect(server, x, y, cx, cy, colour);
	}
}

void
ui_draw_glyph(int mixmode,
	      /* dest */ int x, int y, int cx, int cy,
	      /* src */ HGLYPH glyph, int srcx, int srcy,
	      /* colours */ int bgcolour, int fgcolour)
{
	int xx, yy;
	int ox, oy;
	vncBuffer *buf = vncDupBuffer(glyph);

	x &= 0xffff;
	y &= 0xffff;

	/* yes, sometimes same fgcolour and bgcolour are sent, but because
	 * of transparency, we have to change that! */
	if (mixmode == MIX_TRANSPARENT && fgcolour == bgcolour)
		bgcolour = fgcolour ^ 0xff;

	ox = x;
	oy = y;

	for (yy = srcy; yy < srcy + cy; yy++)
	{
		for (xx = srcx; xx < srcx + cx; xx++)
		{
			vncSetPixel(buf, xx, yy, vncGetPixel(buf, xx, yy) ? fgcolour : bgcolour);
		}
	}

	switch (mixmode)
	{
		case MIX_TRANSPARENT:
			if (vncwinClipRect(&x, &y, &cx, &cy))
			{
				//if we clipped top or left, we have to adjust srcx,srcy;
				srcx += x - ox;
				srcy += y - oy;
				vncTransBlitFrom(server, x, y, cx, cy, buf, srcx, srcy, bgcolour);
			}
			break;
		case MIX_OPAQUE:
			if (vncwinClipRect(&x, &y, &cx, &cy))
			{
				//if we clipped top or left, we have to adjust srcx,srcy;
				srcx += x - ox;
				srcy += y - oy;
				vncCopyBlitFrom(server, x, y, cx, cy, buf, srcx, srcy);
			}
			break;

		default:
			unimpl("mix %d\n", mixmode);
	}
	vncDeleteBuffer(buf);
}

#define DO_GLYPH(ttext,idx) \
{\
  glyph = cache_get_font (font, ttext[idx]);\
  if (!(flags & TEXT2_IMPLICIT_X))\
    {\
      offset = ttext[++idx];\
      if ((offset & 0x80))\
        offset = ((offset & 0x7f) << 8) | ttext[++idx];\
      if (flags & TEXT2_VERTICAL)\
            y += offset;\
          else\
            x += offset;\
    }\
  if (glyph != NULL)\
    {\
      ui_draw_glyph (mixmode, x + (short) glyph->offset,\
                     y + (short) glyph->baseline,\
                     glyph->width, glyph->height,\
                     glyph->pixmap, 0, 0, bgcolour, fgcolour);\
      if (flags & TEXT2_IMPLICIT_X)\
        x += glyph->width;\
    }\
}


void
ui_draw_text(uint8 font, uint8 flags, int mixmode, int x, int y,
	     int clipx, int clipy, int clipcx, int clipcy,
	     int boxx, int boxy, int boxcx, int boxcy,
	     int bgcolour, int fgcolour, uint8 * text, uint8 length)
{
	FONTGLYPH *glyph;
	int i, j, offset;
	DATABLOB *entry;

	if (boxcx > 1)
	{
		ui_rect(boxx, boxy, boxcx, boxcy, bgcolour);
	}
	else if (mixmode == MIX_OPAQUE)
	{
		ui_rect(clipx, clipy, clipcx, clipcy, bgcolour);
	}

	/* Paint text, character by character */
	for (i = 0; i < length;)
	{
		switch (text[i])
		{
			case 0xff:
				if (i + 2 < length)
					cache_put_text(text[i + 1], &(text[i - text[i + 2]]),
						       text[i + 2]);
				else
				{
					error("this shouldn't be happening\n");
					break;
				}
				/* this will move pointer from start to first character after FF command */
				length -= i + 3;
				text = &(text[i + 3]);
				i = 0;
				break;

			case 0xfe:
				entry = cache_get_text(text[i + 1]);
				if (entry != NULL)
				{
					if ((((uint8 *) (entry->data))[1] == 0)
					    && (!(flags & TEXT2_IMPLICIT_X)))
					{
						if (flags & 0x04)	/* vertical text */
							y += text[i + 2];
						else
							x += text[i + 2];
					}
					if (i + 2 < length)
						i += 3;
					else
						i += 2;
					length -= i;
					/* this will move pointer from start to first character after FE command */
					text = &(text[i]);
					i = 0;
					for (j = 0; j < entry->size; j++)
						DO_GLYPH(((uint8 *) (entry->data)), j);
				}
				break;
			default:
				DO_GLYPH(text, i);
				i++;
				break;
		}
	}
}

void
ui_desktop_save(uint32 offset, int x, int y, int cx, int cy)
{
	vncBuffer *buf;

	buf = vncGetRect(server, x, y, cx, cy);
	offset *= TOBYTES(server->serverFormat.bitsPerPixel);
	cache_put_desktop(offset, cx, cy, cx, TOBYTES(server->serverFormat.bitsPerPixel),
			  (buf->data));
}

void
ui_desktop_restore(uint32 offset, int x, int y, int cx, int cy)
{
	uint8 *data;
	vncBuffer *buf;
	int ox, oy, srcx, srcy;

	srcx = srcy = 0;
	ox = x;
	oy = y;

	offset *= TOBYTES(server->serverFormat.bitsPerPixel);
	data = cache_get_desktop(offset, cx, cy, TOBYTES(server->serverFormat.bitsPerPixel));
	if (data == NULL)
		return;

	buf = vncNewBuffer(cx, cy, 8);
	memcpy(buf->data, data, cx * cy * 1);

	if (vncwinClipRect(&x, &y, &cx, &cy))
	{
		srcx += x - ox;
		srcy += y - oy;
		vncCopyBlitFrom(server, x, y, cx, cy, buf, srcx, srcy);
	}
	vncDeleteBuffer(buf);
}

rfbPixelFormat vnc_formats[] = {
	/* bpp, depth,BE,TC, rmax, gmax, bmax, rsh, gsh, bsh  */
	{8, 8, 1, 0, 7, 7, 3, 0, 3, 6}
	,
	{16, 16, 1, 1, 31, 63, 31, 0, 5, 10}
	,
	{32, 24, 1, 1, 255, 255, 255, 0, 8, 16}
	,			//non-existant
	{32, 32, 1, 1, 2047, 2047, 1023, 0, 11, 22}
};

rfbPixelFormat *
vncNewFormat(int depth)
{
	return &(vnc_formats[(depth + 1) / 8 - 1]);
}

vncBuffer *
vncNewBuffer(int w, int h, int depth)
{
	vncBuffer *b = (vncBuffer *) xmalloc(sizeof(vncBuffer));
	b->format = vncNewFormat(depth);
	b->data = (void *) xmalloc(w * h * (b->format->bitsPerPixel / 8));
	b->owner = 1;
	b->w = w;
	b->h = h;
	b->linew = w;
	return b;
}

vncBuffer *
vncDupBuffer(vncBuffer * b)
{
	vncBuffer *buf = vncNewBuffer(b->w, b->h, b->format->depth);
	memcpy(buf->data, b->data, b->linew * b->h * b->format->bitsPerPixel / 8);
	return buf;
}

void
vncPrintStats()
{
	if (server && server->clientHead)
		rfbPrintStats(server->clientHead);
}

/* blit */

#define GETPIXEL(buf,x,y) \
     (((uint8_t*)(buf->data))[(x)+((y)*buf->linew)])
#define SETPIXEL(buf,x,y,p) \
     (((uint8_t*)(buf->data))[(x)+((y)*buf->linew)] = (uint8_t)p)

void
vncCopyBlitFromNoEncode(rfbScreenInfoPtr s, int x, int y, int w, int h,
			vncBuffer * src, int srcx, int srcy)
{
	int xx, yy;

	vncHideCursor();

	if (s->serverFormat.bitsPerPixel == src->format->bitsPerPixel
	    && srcx + w <= src->w && srcy + h <= src->h)
	{
		//simple copy
		uint8_t *srcdata, *dstdata;
		srcdata = src->data + (srcy * src->linew + srcx);
		dstdata = s->frameBuffer + (y * s->paddedWidthInBytes + x);
		for (yy = 0; yy < h; yy++)
		{
			memcpy(dstdata, srcdata, w);
			dstdata += s->paddedWidthInBytes;
			srcdata += src->linew;
		}
	}
	else
	{
		// xsrc,ysrc provide tiling copy support.
		for (yy = y; yy < y + h; yy++)
		{
			int ysrc = srcy + yy - y;
			while (ysrc >= src->h)
				ysrc -= src->h;
			for (xx = x; xx < x + w; xx++)
			{
				vncPixel p;
				int xsrc = srcx + xx - x;
				while (xsrc >= src->linew)
					xsrc -= src->linew;
				p = GETPIXEL(src, xsrc, ysrc);
				SETPIXEL(frameBuffer, xx, yy, p);
			}
		}
	}
}

void
vncCopyBlit(rfbScreenInfoPtr s, int x, int y, int w, int h, int srcx, int srcy)
{
	/* LibVNCServer already knows how to copy the data. */
	rfbDoCopyRect(s, x, y, x + w, y + h, x - srcx, y - srcy);
}

void
vncCopyBlitFrom(rfbScreenInfoPtr s, int x, int y, int w, int h, vncBuffer * src, int srcx, int srcy)
{
	vncCopyBlitFromNoEncode(s, x, y, w, h, src, srcx, srcy);
	rfbMarkRectAsModified(s, x, y, x + w, y + h);
}

void
vncTransBlitFrom(rfbScreenInfoPtr s, int x, int y, int w, int h,
		 vncBuffer * src, int srcx, int srcy, int bg)
{
	int xx, yy;

	vncHideCursor();

	// xsrc,ysrc provide tiling copy support.
	for (yy = y; yy < y + h; yy++)
	{
		int ysrc = srcy + yy - y;
		while (ysrc >= src->h)
			ysrc -= src->h;
		for (xx = x; xx < x + w; xx++)
		{
			vncPixel p;
			int xsrc = srcx + xx - x;
			while (xsrc >= src->linew)
				xsrc -= src->linew;
			p = GETPIXEL(src, xsrc, ysrc);
			// transparent blit!
			if (p != bg)
				SETPIXEL(frameBuffer, xx, yy, p);
		}
	}

	rfbMarkRectAsModified(s, x, y, x + w, y + h);
}

void
vncXorBlitFrom(rfbScreenInfoPtr s, int x, int y, int w, int h, vncBuffer * src, int srcx, int srcy)
{
	int xx, yy;

	vncHideCursor();

	// xsrc,ysrc provide tiling copy support.
	for (yy = y; yy < y + h; yy++)
	{
		int ysrc = srcy + yy - y;
		while (ysrc >= src->h)
			ysrc -= src->h;
		for (xx = x; xx < x + w; xx++)
		{
			vncPixel p, pp;
			int xsrc = srcx + xx - x;
			while (xsrc >= src->linew)
				xsrc -= src->linew;
			p = GETPIXEL(src, xsrc, ysrc);
			pp = GETPIXEL(frameBuffer, xx, yy);
			// xor blit!
			SETPIXEL(frameBuffer, xx, yy, p ^ pp);
		}
	}

	rfbMarkRectAsModified(s, x, y, x + w, y + h);
}

void
vncAndBlitFrom(rfbScreenInfoPtr s, int x, int y, int w, int h, vncBuffer * src, int srcx, int srcy)
{
	int xx, yy;

	vncHideCursor();

	// xsrc,ysrc provide tiling copy support.
	for (yy = y; yy < y + h; yy++)
	{
		int ysrc = srcy + yy - y;
		while (ysrc >= src->h)
			ysrc -= src->h;
		for (xx = x; xx < x + w; xx++)
		{
			vncPixel p, pp;
			int xsrc = srcx + xx - x;
			while (xsrc >= src->linew)
				xsrc -= src->linew;
			p = GETPIXEL(src, xsrc, ysrc);
			pp = GETPIXEL(frameBuffer, xx, yy);
			// and blit!
			SETPIXEL(frameBuffer, xx, yy, p & pp);
		}
	}

	rfbMarkRectAsModified(s, x, y, x + w, y + h);
}

void
vncDeleteBuffer(vncBuffer * b)
{
	if (b->owner)
		xfree(b->data);
	xfree(b);
}

/* cursor */
rfbCursorPtr
vncNewCursor(vncBuffer * mask, vncBuffer * pointer, int hotx, int hoty)
{
	int i, j, w = (mask->w + 7) / 8, mask_size = w * mask->h,
		pointer_size = pointer->w * pointer->h;
	rfbCursorPtr c = (rfbCursorPtr) xmalloc(sizeof(rfbCursor));

	if (mask->w != pointer->w || mask->h != pointer->h)
		error("ERROR! Mask is %dx%d, Pointer is %dx%d\n",
		      mask->w, mask->h, pointer->w, pointer->h);

	c->xhot = hotx;
	c->yhot = hoty;
	c->width = mask->w;
	c->height = mask->h;

	c->mask = (char *) xmalloc(mask_size);
	for (j = 0; j < c->height; j++)
		for (i = 0; i < w; i++)
			c->mask[j * w + i] =
				reverseByte[((unsigned char *) mask->data)[(j) * w + i]];
	vncDeleteBuffer(mask);

	c->source = 0;
	c->richSource = (char *) xmalloc(pointer_size);
	memcpy(c->richSource, pointer->data, pointer_size);
	vncDeleteBuffer(pointer);

	return c;
}

/* No FreeCursor, because the cursors are buffered. We only get a "HANDLE" */
void
vncSetCursor(rfbScreenInfoPtr s, rfbCursorPtr c)
{
	rfbSetCursor(s, c, FALSE);
}

/* these functions work even if vncBuffer's pixel format is not 1 byte/pixel */
vncPixel
vncGetPixel(vncBuffer * b, int x, int y)
{
	unsigned long offset = (x + (y * (b->linew))) * (b->format->bitsPerPixel >> 3);
	return ((uint8_t *) (b->data))[offset];
}

void
vncSetPixel(vncBuffer * b, int x, int y, vncPixel c)
{
	unsigned long offset = (x + (y * (b->linew))) * (b->format->bitsPerPixel >> 3);
	((uint8_t *) (b->data))[offset] = c;
}

void
vncSetRect(rfbScreenInfoPtr s, int x, int y, int w, int h, vncPixel c)
{
	int xx, yy;

	if (x + w > s->width)
		w = s->width - x;
	if (y + h > s->height)
		h = s->height - y;
	if (w <= 0 || h <= 0)
		return;

	vncHideCursor();

	// - Fill the rect in the local framebuffer
	if (s->serverFormat.bitsPerPixel == 8)
	{
		// - Simple 8-bit fill
		uint8_t *dstdata;
		dstdata = s->frameBuffer + (y * s->paddedWidthInBytes + x);
		for (yy = 0; yy < h; yy++)
		{
			memset(dstdata, c, w);
			dstdata += s->paddedWidthInBytes;
		}
	}
	else
	{
		for (yy = y; yy < y + h; yy++)
		{
			for (xx = x; xx < x + w; xx++)
			{
				SETPIXEL(frameBuffer, xx, yy, c);
			}
		}
	}

	rfbMarkRectAsModified(s, x, y, x + w, y + h);
}

vncBuffer *
vncGetRect(rfbScreenInfoPtr s, int x, int y, int w, int h)
{
	int xx, yy;
	vncBuffer *b = vncNewBuffer(w, h, s->serverFormat.depth);

	vncHideCursor();

	if (s->serverFormat.bitsPerPixel == 8)
	{
		//simple copy
		int srcstep, dststep;
		char *srcdata, *dstdata;
		srcstep = s->paddedWidthInBytes * s->serverFormat.bitsPerPixel / 8;
		dststep = w * s->serverFormat.bitsPerPixel / 8;
		dstdata = b->data;
		srcdata = s->frameBuffer + (y * srcstep + x * s->serverFormat.bitsPerPixel / 8);
		for (yy = 0; yy < h; yy++)
		{
			memcpy(dstdata, srcdata, dststep);
			dstdata += dststep;
			srcdata += srcstep;
		}
	}
	else
	{
		for (yy = y; yy < y + h; yy++)
		{
			for (xx = x; xx < x + w; xx++)
			{
				SETPIXEL(b, xx - x, yy - y, GETPIXEL(frameBuffer, xx, yy));
			}
		}
	}

	return b;
}

/* colourmap */

rfbColourMap *
vncNewColourMap(rfbScreenInfoPtr s, int n)
{
	rfbColourMap *m = (rfbColourMap *) xmalloc(sizeof(rfbColourMap));
	m->is16 = FALSE;
	m->count = n;
	m->data.bytes = (uint8_t *) xmalloc(n * 3);
	return m;
}

void
vncSetColourMapEntry(rfbColourMap * m, int i, vncPixel r, vncPixel g, vncPixel b)
{
	if (i < m->count)
	{
		m->data.bytes[3 * i + 0] = r;
		m->data.bytes[3 * i + 1] = g;
		m->data.bytes[3 * i + 2] = b;
	}
}

void
vncDeleteColourMap(rfbColourMap * m)
{
	if (m->data.bytes)
		free(m->data.bytes);
	m->count = 0;
}

void
vncSetColourMap(rfbScreenInfoPtr s, rfbColourMap * m)
{
	vncDeleteColourMap(&s->colourMap);
	s->colourMap = *m;
	rfbSetClientColourMaps(s, 0, 0);
}

void
ui_begin_update()
{
}

void
ui_end_update()
{
}

void
ui_resize_window()
{
	rfbClientIteratorPtr iter;
	rfbClientPtr cl;

	server->width = g_width;
	server->height = g_height;
	server->frameBuffer = (char *) realloc(server->frameBuffer, g_width * g_height);
	server->paddedWidthInBytes = g_width;

	iter = rfbGetClientIterator(server);
	while ((cl = rfbClientIteratorNext(iter)))
		if (cl->useNewFBSize)
			cl->newFBSizePending = TRUE;
		else
			rfbLog("Warning: Client %s does not support NewFBSize!\n ", cl->host);
	rfbReleaseClientIterator(iter);
}
