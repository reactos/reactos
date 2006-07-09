#include "stdafx.h"

#include "rdesktop/rdesktop.h"
#include "rdesktop/proto.h"

extern "C"
{
	/* ==== BEGIN POOP ==== */
	// Temporary implementations of stuff we totally positively need to make the Real Thing happy
	/* produce a hex dump */
	void
	hexdump(unsigned char *p, unsigned int len)
	{
		unsigned char *line = p;
		int i, thisline;
		unsigned int offset = 0;

		while (offset < len)
		{
			printf("%04x ", offset);
			thisline = len - offset;
			if (thisline > 16)
				thisline = 16;

			for (i = 0; i < thisline; i++)
				printf("%02x ", line[i]);

			for (; i < 16; i++)
				printf("   ");

			for (i = 0; i < thisline; i++)
				printf("%c", (line[i] >= 0x20 && line[i] < 0x7f) ? line[i] : '.');

			printf("\n");
			offset += thisline;
			line += thisline;
		}
	}

	void generate_random(uint8 * random)
	{
		memcpy(random, "12345678901234567890123456789012", 32);
	}

	/* report an error */
	void
	error(char *format, ...)
	{
		va_list ap;

		fprintf(stderr, "ERROR: ");

		va_start(ap, format);
		vfprintf(stderr, format, ap);
		va_end(ap);
	}

	/* report a warning */
	void
	warning(char *format, ...)
	{
		va_list ap;

		fprintf(stderr, "WARNING: ");

		va_start(ap, format);
		vfprintf(stderr, format, ap);
		va_end(ap);
	}

	/* report an unimplemented protocol feature */
	void
	unimpl(char *format, ...)
	{
		va_list ap;

		fprintf(stderr, "NOT IMPLEMENTED: ");

		va_start(ap, format);
		vfprintf(stderr, format, ap);
		va_end(ap);
	}

	/* malloc; exit if out of memory */
	void *
	xmalloc(int size)
	{
		void *mem = malloc(size);
		if (mem == NULL)
		{
			error("xmalloc %d\n", size);
			exit(1);
		}
		return mem;
	}

	/* strdup */
	char *
	xstrdup(const char *s)
	{
		char *mem = strdup(s);
		if (mem == NULL)
		{
			perror("strdup");
			exit(1);
		}
		return mem;
	}

	/* realloc; exit if out of memory */
	void *
	xrealloc(void *oldmem, int size)
	{
		void *mem;

		if (size < 1)
			size = 1;
		mem = realloc(oldmem, size);
		if (mem == NULL)
		{
			error("xrealloc %d\n", size);
			exit(1);
		}
		return mem;
	}

	/* free */
	void
	xfree(void *mem)
	{
		free(mem);
	}

	/* Create the bitmap cache directory */
	BOOL
	rd_pstcache_mkdir(void)
	{
		char *home;
		char bmpcache_dir[256];

		home = getenv("HOME");

		if (home == NULL)
			return False;

		sprintf(bmpcache_dir, "%s/%s", home, ".rdesktop");

		if ((_mkdir(bmpcache_dir) == -1) && errno != EEXIST)
		{
			perror(bmpcache_dir);
			return False;
		}

		sprintf(bmpcache_dir, "%s/%s", home, ".rdesktop/cache");

		if ((_mkdir(bmpcache_dir) == -1) && errno != EEXIST)
		{
			perror(bmpcache_dir);
			return False;
		}

		return True;
	}

	/* open a file in the .rdesktop directory */
	int
	rd_open_file(char *filename)
	{
		char *home;
		char fn[256];
		int fd;

		home = getenv("HOME");
		if (home == NULL)
			return -1;
		sprintf(fn, "%s/.rdesktop/%s", home, filename);
		fd = _open(fn, _O_RDWR | _O_CREAT, 0);
		if (fd == -1)
			perror(fn);
		return fd;
	}

	/* close file */
	void
	rd_close_file(int fd)
	{
		_close(fd);
	}

	/* read from file*/
	int
	rd_read_file(int fd, void *ptr, int len)
	{
		return _read(fd, ptr, len);
	}

	/* write to file */
	int
	rd_write_file(int fd, void *ptr, int len)
	{
		return _write(fd, ptr, len);
	}

	/* move file pointer */
	int
	rd_lseek_file(int fd, int offset)
	{
		return _lseek(fd, offset, SEEK_SET);
	}

	/* do a write lock on a file */
	BOOL
	rd_lock_file(int fd, int start, int len)
	{
		// TODOOO...
		return False;
	}

	int
	load_licence(RDPCLIENT * This, unsigned char **data)
	{
		char *home, *path;
		struct stat st;
		int fd, length;

		home = getenv("HOME");
		if (home == NULL)
			return -1;

		path = (char *) xmalloc(strlen(home) + strlen(This->hostname) + sizeof("/.rdesktop/licence."));
		sprintf(path, "%s/.rdesktop/licence.%s", home, This->hostname);

		fd = _open(path, O_RDONLY);
		if (fd == -1)
			return -1;

		if (fstat(fd, &st))
			return -1;

		*data = (uint8 *) xmalloc(st.st_size);
		length = _read(fd, *data, st.st_size);
		_close(fd);
		xfree(path);
		return length;
	}

	void
	save_licence(RDPCLIENT * This, unsigned char *data, int length)
	{
		char *home, *path, *tmppath;
		int fd;

		home = getenv("HOME");
		if (home == NULL)
			return;

		path = (char *) xmalloc(strlen(home) + strlen(This->hostname) + sizeof("/.rdesktop/licence."));

		sprintf(path, "%s/.rdesktop", home);
		if ((_mkdir(path) == -1) && errno != EEXIST)
		{
			perror(path);
			return;
		}

		/* write licence to licence.hostname.new, then atomically rename to licence.hostname */

		sprintf(path, "%s/.rdesktop/licence.%s", home, This->hostname);
		tmppath = (char *) xmalloc(strlen(path) + sizeof(".new"));
		strcpy(tmppath, path);
		strcat(tmppath, ".new");

		fd = _open(tmppath, O_WRONLY | O_CREAT | O_TRUNC, 0600);
		if (fd == -1)
		{
			perror(tmppath);
			return;
		}

		if (_write(fd, data, length) != length)
		{
			perror(tmppath);
			_unlink(tmppath);
		}
		else if (rename(tmppath, path) == -1)
		{
			perror(path);
			_unlink(tmppath);
		}

		_close(fd);
		xfree(tmppath);
		xfree(path);
	}

	/* ==== END POOP ==== */

	/* ==== UI ==== */
	// Globals are totally teh evil, but cut me some slack here
	HWND hwnd;
	HBITMAP hbmBuffer;
	HDC hdcBuffer;

#if 0
	// NOTE: we don't really need these with rdesktop.c out of the picture
	BOOL
	ui_init(RDPCLIENT * This)
	{
		return 0;
	}

	void
	ui_deinit(RDPCLIENT * This)
	{
	}

	BOOL
	ui_create_window(RDPCLIENT * This)
	{
		return 0;
	}
#endif

	void
	ui_resize_window(RDPCLIENT * This)
	{
		// TODO
	}

#if 0
	void
	ui_destroy_window(RDPCLIENT * This)
	{
	}
#endif

	int
	ui_select(RDPCLIENT * This, int rdp_socket)
	{
		return 1; // TODO: return 0 for user quit. Or just kill this silly function
	}

	void
	ui_move_pointer(RDPCLIENT * This, int x, int y)
	{
		// TODO
	}

	HBITMAP
	ui_create_bitmap(RDPCLIENT * This, int width, int height, uint8 * data)
	{
		void * pBits;

		BITMAPINFOHEADER bmih;
		BITMAPINFO bmi;

		bmih.biSize = sizeof(bmih);
		bmih.biWidth = width;
		bmih.biHeight = - height;
		bmih.biPlanes = 1;
		bmih.biBitCount = This->server_depth;
		bmih.biCompression = BI_RGB;
		bmih.biSizeImage = 0;
		bmih.biXPelsPerMeter = 0;
		bmih.biYPelsPerMeter = 0;
		bmih.biClrUsed = 0;
		bmih.biClrImportant = 0;

		bmi.bmiHeader = bmih;
		memset(bmi.bmiColors, 0, sizeof(bmi.bmiColors));

		return CreateDIBitmap(hdcBuffer, &bmih, CBM_INIT, data, &bmi, DIB_RGB_COLORS);
	}

	void
	ui_destroy_bitmap(RDPCLIENT * This, HBITMAP bmp)
	{
		DeleteObject(bmp);
	}

	HGLYPH
	ui_create_glyph(RDPCLIENT * This, int width, int height, const uint8 * data)
	{
		// TODO: create 2bpp mask
		return 0;
	}

	void
	ui_destroy_glyph(RDPCLIENT * This, HGLYPH glyph)
	{
		// TODO
	}

	HCURSOR
	ui_create_cursor(RDPCLIENT * This, unsigned int x, unsigned int y, int width, int height,
			 uint8 * andmask, uint8 * xormask)
	{
		return CreateCursor(NULL, x, y, width, height, andmask, xormask);
	}

	void
	ui_set_cursor(RDPCLIENT * This, HCURSOR cursor)
	{
		// TODO
	}

	void
	ui_destroy_cursor(RDPCLIENT * This, HCURSOR cursor)
	{
		DestroyCursor(cursor);
	}

	void
	ui_set_null_cursor(RDPCLIENT * This)
	{
		// TODO
	}

	HCOLOURMAP
	ui_create_colourmap(RDPCLIENT * This, COLOURMAP * colours)
	{
		// TODO: kill HCOLOURMAP/COLOURMAP, use HPALETTE/LOGPALETTE
		return 0;
	}

	void
	ui_destroy_colourmap(RDPCLIENT * This, HCOLOURMAP map)
	{
		// TODO: see above
	}

	void
	ui_set_colourmap(RDPCLIENT * This, HCOLOURMAP map)
	{
		// TODO
	}

	RECT rcClip; // TODO: initialize

	void
	ui_set_clip(RDPCLIENT * This, int x, int y, int cx, int cy)
	{
		rcClip.left = x;
		rcClip.top = y;
		rcClip.right = x + cx - 1;
		rcClip.bottom = y + cy - 1;
		SelectObject(hdcBuffer, CreateRectRgnIndirect(&rcClip));
	}

	void
	ui_reset_clip(RDPCLIENT * This)
	{
		rcClip.left = 0;
		rcClip.top = 0;
		rcClip.right = This->width;
		rcClip.bottom = This->height;
		SelectObject(hdcBuffer, CreateRectRgnIndirect(&rcClip));
	}

	void
	ui_bell(RDPCLIENT * This)
	{
		MessageBeep(MB_OK); // TODO? use Beep() on remote sessions?
	}

	static
	void
	win32_repaint_rect(RDPCLIENT * This, const RECT * lprc)
	{
		RECT rcDamage;
		IntersectRect(&rcDamage, lprc, &rcClip);
		InvalidateRect(hwnd, &rcDamage, FALSE);
		//Sleep(100);
	}

	static
	void
	win32_repaint_area(RDPCLIENT * This, int x, int y, int cx, int cy)
	{
		RECT rcDamage;
		rcDamage.left = x;
		rcDamage.top = y;
		rcDamage.right = x + cx;
		rcDamage.bottom = y + cy;
		win32_repaint_rect(This, &rcDamage);
	}

	static
	void
	win32_repaint_poly(RDPCLIENT * This, POINT * point, int npoints, int linewidth)
	{
		RECT rcDamage;

		rcDamage.left = MAXLONG;
		rcDamage.top = MAXLONG;
		rcDamage.right = 0;
		rcDamage.bottom = 0;

		for(int i = 0; i < npoints; ++ i)
		{
			if(point[i].x < rcDamage.left)
				rcDamage.left = point[i].x;

			if(point[i].y < rcDamage.top)
				rcDamage.top = point[i].y;

			if(point[i].x > rcDamage.right)
				rcDamage.right = point[i].x;

			if(point[i].y > rcDamage.bottom)
				rcDamage.bottom = point[i].y;
		}

		InflateRect(&rcDamage, linewidth, linewidth);
		win32_repaint_rect(This, &rcDamage);
	}

	static
	void
	win32_repaint_whole(RDPCLIENT * This)
	{
		InvalidateRgn(hwnd, NULL, FALSE);
	}

	/*
		TODO: all of the following could probably be implemented this way:
		 - perform operation on off-screen buffer
		 - invalidate affected region of the window
	*/
	void
	ui_paint_bitmap(RDPCLIENT * This, int x, int y, int cx, int cy, int width, int height, uint8 * data)
	{
		BITMAPINFO bmi;
		ZeroMemory(&bmi, sizeof(bmi));

		bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
		bmi.bmiHeader.biWidth = width;
		bmi.bmiHeader.biHeight = - height;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = This->server_depth;

		StretchDIBits(hdcBuffer, x, y, cx, cy, 0, 0, width, height, data, &bmi, DIB_RGB_COLORS, SRCCOPY);

		win32_repaint_area(This, x, y, cx, cy);
	}

	void
	ui_destblt(RDPCLIENT * This, uint8 opcode,
		   /* dest */ int x, int y, int cx, int cy)
	{
		RECT rc;
		rc.left = x;
		rc.top = y;
		rc.right = x + cx;
		rc.bottom = y + cy;

		int prev = SetROP2(hdcBuffer, MAKELONG(0, opcode | opcode << 4));

		FillRect(hdcBuffer, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH)); // use Rectangle instead?

		SetROP2(hdcBuffer, prev);

		win32_repaint_area(This, x, y, cx, cy);
	}

	void
	ui_patblt(RDPCLIENT * This, uint8 opcode,
		  /* dest */ int x, int y, int cx, int cy,
		  /* brush */ BRUSH * brush, int bgcolour, int fgcolour)
	{
		// TODO
		win32_repaint_area(This, x, y, cx, cy);
	}

	void
	ui_screenblt(RDPCLIENT * This, uint8 opcode,
			 /* dest */ int x, int y, int cx, int cy,
			 /* src */ int srcx, int srcy)
	{
		BitBlt(hdcBuffer, x, y, cx, cy, hdcBuffer, srcx, srcy, MAKELONG(0, opcode | opcode << 4));
		win32_repaint_area(This, x, y, cx, cy);
	}

	void
	ui_memblt(RDPCLIENT * This, uint8 opcode,
		  /* dest */ int x, int y, int cx, int cy,
		  /* src */ HBITMAP src, int srcx, int srcy)
	{
		HDC hdcSrc = CreateCompatibleDC(hdcBuffer);
		HGDIOBJ hOld = SelectObject(hdcSrc, src);

		BitBlt(hdcBuffer, x, y, cx, cy, hdcSrc, srcx, srcy, MAKELONG(0, opcode | opcode << 4));

		DeleteObject(hOld);
		DeleteDC(hdcSrc);

		win32_repaint_area(This, x, y, cx, cy);
	}

	void
	ui_triblt(RDPCLIENT * This, uint8 opcode,
		  /* dest */ int x, int y, int cx, int cy,
		  /* src */ HBITMAP src, int srcx, int srcy,
		  /* brush */ BRUSH * brush, int bgcolour, int fgcolour)
	{
		// TODO
		win32_repaint_area(This, x, y, cx, cy);
	}

	void
	ui_line(RDPCLIENT * This, uint8 opcode,
		/* dest */ int startx, int starty, int endx, int endy,
		/* pen */ PEN * pen)
	{
		HPEN hpen = CreatePen(pen->style, pen->width, pen->colour);
		HGDIOBJ hOld = SelectObject(hdcBuffer, hpen);

		int prevROP = SetROP2(hdcBuffer, opcode);

		POINT prevPos;
		MoveToEx(hdcBuffer, startx, starty, &prevPos);

		LineTo(hdcBuffer, endx, endy);

		MoveToEx(hdcBuffer, prevPos.x, prevPos.y, NULL);

		SetROP2(hdcBuffer, prevROP);

		SelectObject(hdcBuffer, hOld);

		RECT rcDamage;
		
		if(startx < endx)
		{
			rcDamage.left = startx;
			rcDamage.right = endx;
		}
		else
		{
			rcDamage.left = endx;
			rcDamage.right = startx;
		}

		if(starty < endy)
		{
			rcDamage.top = starty;
			rcDamage.bottom = endy;
		}
		else
		{
			rcDamage.top = endy;
			rcDamage.bottom = starty;
		}

		InflateRect(&rcDamage, pen->width, pen->width);
		win32_repaint_rect(This, &rcDamage);
	}

	void
	ui_rect(RDPCLIENT * This,
			   /* dest */ int x, int y, int cx, int cy,
			   /* brush */ int colour)
	{
		RECT rc;
		rc.left = x;
		rc.top = y;
		rc.right = x + cx;
		rc.bottom = y + cy;

		HBRUSH hbr = CreateSolidBrush(colour);
		FillRect(hdcBuffer, &rc, hbr); // use Rectangle instead?
		DeleteObject(hbr);

		win32_repaint_rect(This, &rc);
	}

	void
	ui_polygon(RDPCLIENT * This, uint8 opcode,
		   /* mode */ uint8 fillmode,
		   /* dest */ POINT * point, int npoints,
		   /* brush */ BRUSH * brush, int bgcolour, int fgcolour)
	{
		// TODO: create brush, set fg&bg

		int oldRop = SetROP2(hdcBuffer, opcode);
		int oldFillMode = SetPolyFillMode(hdcBuffer, fillmode);
		Polygon(hdcBuffer, point, npoints);
		SetPolyFillMode(hdcBuffer, oldFillMode);
		SetROP2(hdcBuffer, oldRop);

		win32_repaint_poly(This, point, npoints, 0);
	}

	void
	ui_polyline(RDPCLIENT * This, uint8 opcode,
			/* dest */ POINT * points, int npoints,
			/* pen */ PEN * pen)
	{
		HPEN hpen = CreatePen(pen->style, pen->width, pen->colour);
		HGDIOBJ hOld = SelectObject(hdcBuffer, hpen);
		int oldRop = SetROP2(hdcBuffer, opcode);
		Polyline(hdcBuffer, points, npoints);
		SetROP2(hdcBuffer, oldRop);
		SelectObject(hdcBuffer, hOld);

		win32_repaint_poly(This, points, npoints, pen->width);
	}

	void
	ui_ellipse(RDPCLIENT * This, uint8 opcode,
		   /* mode */ uint8 fillmode,
		   /* dest */ int x, int y, int cx, int cy,
		   /* brush */ BRUSH * brush, int bgcolour, int fgcolour)
	{
#if 0
		switch(fillmode)
		{
		case 0: // outline // HOW? HOW? WITH WHAT PEN?
		case 1: // filled // TODO
		}
#endif
		win32_repaint_area(This, x, y, cx, cy);
	}

	void
	ui_draw_glyph(RDPCLIENT * This, int mixmode,
			  /* dest */ int x, int y, int cx, int cy,
			  /* src */ HGLYPH glyph, int srcx, int srcy,
			  int bgcolour, int fgcolour)
	{
		// TODO!!!
	}

	void
	ui_draw_text(RDPCLIENT * This, uint8 font, uint8 flags, uint8 opcode, int mixmode, int x, int y,
			 int clipx, int clipy, int clipcx, int clipcy,
			 int boxx, int boxy, int boxcx, int boxcy, BRUSH * brush,
			 int bgcolour, int fgcolour, uint8 * text, uint8 length)
	{
		// TODO!!!
	}

	void
	ui_desktop_save(RDPCLIENT * This, uint32 offset, int x, int y, int cx, int cy)
	{
		// TODO (use GetDIBits)
	}

	void
	ui_desktop_restore(RDPCLIENT * This, uint32 offset, int x, int y, int cx, int cy)
	{
		// TODO (use SetDIBitsToDevice)
		win32_repaint_whole(This);
	}

	void
	ui_begin_update(RDPCLIENT * This)
	{
		// TODO? use a mutex to arbitrate access to the off-screen buffer?
	}

	void
	ui_end_update(RDPCLIENT * This)
	{
		// TODO? use a mutex to arbitrate access to the off-screen buffer?
	}
};

static
LRESULT
CALLBACK
mstsc_WndProc
(
	HWND hwnd,
	UINT uMsg,
	WPARAM wparam,
	LPARAM lparam
)
{
	// BUGBUG: LongToPtr & PtrToLong will break on Win64

	RDPCLIENT * This = reinterpret_cast<RDPCLIENT *>(LongToPtr(GetWindowLongPtr(hwnd, GWLP_USERDATA)));

	switch(uMsg)
	{
	case WM_CREATE:
		This = static_cast<RDPCLIENT *>(reinterpret_cast<LPCREATESTRUCT>(lparam)->lpCreateParams);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, PtrToLong(This));
		break;

	case WM_PAINT:
		// Obscenely simple code for now...
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);

			BitBlt
			(
				hdc,
				ps.rcPaint.left,
				ps.rcPaint.top,
				ps.rcPaint.right - ps.rcPaint.left,
				ps.rcPaint.bottom - ps.rcPaint.top,
				hdcBuffer,
				ps.rcPaint.left,
				ps.rcPaint.top,
				SRCCOPY
			);

			EndPaint(hwnd, &ps);
		}

		break;

	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:		
		rdp_send_input
		(
			This,
			GetMessageTime(),
			RDP_INPUT_SCANCODE,
			RDP_KEYPRESS | (lparam & 0x1000000 ? KBD_FLAG_EXT : 0),
			LOBYTE(HIWORD(lparam)),
			0
		);

		break;

	case WM_SYSKEYUP:
	case WM_KEYUP:
		rdp_send_input
		(
			This,
			GetMessageTime(),
			RDP_INPUT_SCANCODE,
			RDP_KEYRELEASE | (lparam & 0x1000000 ? KBD_FLAG_EXT : 0),
			LOBYTE(HIWORD(lparam)),
			0
		);

		break;

	default:
		return DefWindowProc(hwnd, uMsg, wparam, lparam);
	}

	return 0;
}

static
DWORD
WINAPI
mstsc_ProtocolIOThread
(
	LPVOID lpArgument
)
{
	RDPCLIENT * This = static_cast<RDPCLIENT *>(lpArgument);

	strcpy(This->username, "Hyperion");

	DWORD dw = sizeof(This->hostname);
	GetComputerNameA(This->hostname, &dw);

	uint32 flags = RDP_LOGON_NORMAL | RDP_LOGON_COMPRESSION | RDP_LOGON_COMPRESSION2;

	rdp_connect(This, "10.0.0.3", flags, "", "", "", "");

	hdcBuffer = CreateCompatibleDC(NULL);

	BITMAPINFO bmi;
	bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth = This->width;
	bmi.bmiHeader.biHeight = This->height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = This->server_depth;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = 0;
	bmi.bmiHeader.biXPelsPerMeter = 0;
	bmi.bmiHeader.biYPelsPerMeter = 0;
	bmi.bmiHeader.biClrUsed = 0; // TODO! palette displays
	bmi.bmiHeader.biClrImportant = 0; // TODO! palette displays

	void * p;

	hbmBuffer = CreateDIBSection(hdcBuffer, &bmi, DIB_RGB_COLORS, &p, NULL, 0);

	SelectObject(hdcBuffer, hbmBuffer);

	rcClip.left = 0;
	rcClip.top = 0;
	rcClip.right = This->width;
	rcClip.bottom = This->height;

	BOOL deactivated;
	uint32 ext_disc_reason;

	rdp_main_loop(This, &deactivated, &ext_disc_reason);

	return 0;
}

int wmain()
{
	WSADATA wsd;
	WSAStartup(MAKEWORD(2, 2), &wsd);

	static RDPCLIENT This_; // NOTE: this is HUGE and would overflow the stack!
	ZeroMemory(&This_, sizeof(This_));

	RDPCLIENT * This = &This_;

	/*
		Threading model for MissTosca:
		 - main thread is the GUI thread. Message loop maintained by caller
		 - protocol I/O is handled in an I/O thread (or thread pool)
		 - extra threads maintained by virtual channel handlers. Virtual channel writes are thread-neutral

		How we handle drawing: at the moment just an off-screen buffer we dump on-screen when asked to.
		Still considering how to draw on-screen directly and *then* buffering off-screen (for example,
		when running inside another remote session)
	*/

	// FIXME: keyboard mess
	This->keylayout = 0x409;
	This->keyboard_type = 0x4;
	This->keyboard_subtype = 0x0;
	This->keyboard_functionkeys = 0xc;
	This->width = 1024;
	This->height = 768;
	This->server_depth = 24;
	This->bitmap_compression = True;
	This->sendmotion = True;
	This->bitmap_cache = True;
	This->bitmap_cache_persist_enable = False;
	This->bitmap_cache_precache = True;
	This->encryption = True;
	This->packet_encryption = True;
	This->desktop_save = True;
	This->polygon_ellipse_orders = True;
	This->fullscreen = False;
	This->grab_keyboard = True;
	This->hide_decorations = False;
	This->use_rdp5 = True;
	This->rdpclip = True;
	This->console_session = False;
	This->numlock_sync = False;
	This->seamless_rdp = False;
	This->rdp5_performanceflags = RDP5_NO_WALLPAPER | RDP5_NO_FULLWINDOWDRAG | RDP5_NO_MENUANIMATIONS;
	This->tcp_port_rdp = TCP_PORT_RDP;

#define NOT_SET -1
	This->cache.bmpcache_lru[0] = NOT_SET;
	This->cache.bmpcache_lru[1] = NOT_SET;
	This->cache.bmpcache_lru[2] = NOT_SET;
	This->cache.bmpcache_mru[0] = NOT_SET;
	This->cache.bmpcache_mru[1] = NOT_SET;
	This->cache.bmpcache_mru[2] = NOT_SET;

	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(wc));

	wc.lpfnWndProc = mstsc_WndProc;
	wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(HOLLOW_BRUSH));
	wc.lpszClassName = TEXT("MissTosca_Desktop");

	ATOM a = RegisterClass(&wc);

	hwnd = CreateWindow
	(
		MAKEINTATOM(a),
		NULL,
		WS_POPUP | WS_VISIBLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		This->width,
		This->height,
		NULL,
		NULL,
		NULL,
		This
	);

	// The righ time to start the protocol thread
	DWORD dwThreadId;
	HANDLE hThread = CreateThread(NULL, 0, mstsc_ProtocolIOThread, This, 0, &dwThreadId);

	// Your standard, garden variety message loop
	MSG msg;

	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

// EOF
