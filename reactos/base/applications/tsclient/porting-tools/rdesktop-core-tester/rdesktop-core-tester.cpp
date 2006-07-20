#include "stdafx.h"

#include <zmouse.h>

#include "rdesktop/rdesktop.h"
#include "rdesktop/proto.h"

template<class T, class U> T aligndown(const T& X, const U& align)
{
	return X & ~(T(align) - 1);		
}

template<class T, class U> T alignup(const T& X, const U& align)
{
	return aligndown(X + (align - 1), align);
}

#ifdef _WIN64
#else

#undef InterlockedExchangePointer

#define InterlockedExchangePointer(Target, Value) \
	LongToPtr(InterlockedExchange((LONG volatile *)(Target), PtrToLong(Value)))

#endif

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
	xmalloc(size_t size)
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
		char *mem = _strdup(s);
		if (mem == NULL)
		{
			perror("strdup");
			exit(1);
		}
		return mem;
	}

	/* realloc; exit if out of memory */
	void *
	xrealloc(void *oldmem, size_t size)
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
	PVOID pBuffer;
	HDC hdcBuffer;
	UINT wmZMouseWheel;

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
		POINT point;
		point.x = x;
		point.y = y;

		ClientToScreen(hwnd, &point);
		SetCursorPos(point.x, point.y);
	}

	HCURSOR hcursor;

	struct Bitmap
	{
		int width;
		int height;
		uint8 data[1];
	};

	static
	HBITMAP
	win32_create_dib(LONG width, LONG height, WORD bitcount, const BYTE * data)
	{
		struct b_
		{
			BITMAPINFO bmi;
			RGBQUAD colormap[256 - ARRAYSIZE(RTL_FIELD_TYPE(BITMAPINFO, bmiColors))];
		}
		b;

		b.bmi.bmiHeader.biSize = sizeof(b.bmi.bmiHeader);
		b.bmi.bmiHeader.biWidth = width;
		b.bmi.bmiHeader.biHeight = height;
		b.bmi.bmiHeader.biPlanes = 1;
		b.bmi.bmiHeader.biBitCount = bitcount;
		b.bmi.bmiHeader.biCompression = BI_RGB;
		b.bmi.bmiHeader.biSizeImage = 0;
		b.bmi.bmiHeader.biXPelsPerMeter = 0;
		b.bmi.bmiHeader.biYPelsPerMeter = 0;

		if(bitcount > 8)
		{
			b.bmi.bmiHeader.biClrUsed = 0;
			b.bmi.bmiHeader.biClrImportant = 0;
		}
		else
		{
			b.bmi.bmiHeader.biClrUsed = 2 << bitcount;
			b.bmi.bmiHeader.biClrImportant = 2 << bitcount;

			// TODO: palette
		}

		HBITMAP hbm = CreateDIBitmap(hdcBuffer, &b.bmi.bmiHeader, CBM_INIT, data, &b.bmi, DIB_RGB_COLORS);

		if(hbm == NULL)
			error("CreateDIBitmap %dx%dx%d failed\n", width, height, bitcount);

		return hbm;
	}

	static
	uint8 *
	win32_convert_scanlines(int width, int height, int bitcount, int fromalign, int toalign, const uint8 * data, uint8 ** buffer)
	{
		assert(width > 0);
		assert(height);
		assert(bitcount && bitcount <= 32);
		assert(fromalign <= toalign);
		assert(data);
		assert(buffer);

		bool flipped = height < 0;

		if(flipped)
			height = - height;

		int bytesperrow = alignup(width * bitcount, 8) / 8;
		int fromstride = alignup(bytesperrow, fromalign);
		int tostride = alignup(bytesperrow, toalign);
		assert(fromstride <= tostride);

		int datasize = tostride * height;

		uint8 * dibits = new(xmalloc(datasize)) uint8;

		const uint8 * src = data;
		uint8 * dest = dibits;

		const int pad = tostride - fromstride;

		assert(pad < 4);
		__assume(pad < 4);

		if(flipped)
		{
			dest += (height - 1) * tostride;
			tostride = - tostride;
		}

		for(int i = 0; i < height; ++ i)
		{
			memcpy(dest, src, fromstride);
			memset(dest + fromstride, 0, pad);
			src += fromstride;
			dest += tostride;
		}

		*buffer = dibits;
		return dibits;
	}

	HBITMAP
	ui_create_bitmap(RDPCLIENT * This, int width, int height, uint8 * data)
	{
		return win32_create_dib(width, height, This->server_depth, data);
	}

	void
	ui_destroy_bitmap(RDPCLIENT * This, HBITMAP bmp)
	{
		DeleteObject(bmp);
	}

	HGLYPH
	ui_create_glyph(RDPCLIENT * This, int width, int height, const uint8 * data)
	{
		uint8 * databuf = NULL;
		uint8 * databits = win32_convert_scanlines(width, height, 1, 1, 2, data, &databuf);
		
		HBITMAP hbm = CreateBitmap(width, height, 1, 1, databits);

		if(databuf)
			xfree(databuf);

		const uint8 * p = data;
		int stride = alignup(alignup(width, 8) / 8, 1);

		printf("glyph %p\n", hbm);

		for(int i = 0; i < height; ++ i, p += stride)
		{
			for(int j = 0; j < width; ++ j)
			{
				int B = p[j / 8];
				int b = 8 - j % 8 - 1;

				if(B & (1 << b))
					fputs("##", stdout);
				else
					fputs("..", stdout);
			}

			fputc('\n', stdout);
		}

		fputc('\n', stdout);

		return hbm;
	}

	void
	ui_destroy_glyph(RDPCLIENT * This, HGLYPH glyph)
	{
		DeleteObject(glyph);
	}

	HCURSOR
	ui_create_cursor(RDPCLIENT * This, unsigned int x, unsigned int y, int width, int height,
			 uint8 * andmask, uint8 * xormask)
	{
		uint8 * andbuf = NULL;
		uint8 * xorbuf = NULL;

		uint8 * andbits = win32_convert_scanlines(width, - height, 1, 2, 4, andmask, &andbuf);
		uint8 * xorbits = win32_convert_scanlines(width, height, 24, 2, 4, xormask, &xorbuf);

		HBITMAP hbmMask = CreateBitmap(width, height, 1, 1, andbits);
		HBITMAP hbmColor = win32_create_dib(width, height, 24, xorbits);

		ICONINFO iconinfo;
		iconinfo.fIcon = FALSE;
		iconinfo.xHotspot = x;
		iconinfo.yHotspot = y;
		iconinfo.hbmMask = hbmMask;
		iconinfo.hbmColor = hbmColor;
		
		HICON icon = CreateIconIndirect(&iconinfo);

		if(icon == NULL)
			error("CreateIconIndirect %dx%d failed\n", width, height);

		if(andbuf)
			xfree(andbuf);

		if(xorbuf)
			xfree(xorbuf);

		DeleteObject(hbmMask);
		DeleteObject(hbmColor);

		return icon;
	}

	void
	ui_set_cursor(RDPCLIENT * This, HCURSOR cursor)
	{
		hcursor = cursor;
	}

	void
	ui_destroy_cursor(RDPCLIENT * This, HCURSOR cursor)
	{
		DestroyIcon(cursor);
	}

	void
	ui_set_null_cursor(RDPCLIENT * This)
	{
		hcursor = NULL;
	}

	HCOLOURMAP
	ui_create_colourmap(RDPCLIENT * This, COLOURMAP * colours)
	{
		// TODO
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
		rcClip.right = x + cx + 1;
		rcClip.bottom = y + cy + 1;

		HRGN hrgn = CreateRectRgnIndirect(&rcClip);
		SelectClipRgn(hdcBuffer, hrgn);
		DeleteObject(hrgn);
	}

	void
	ui_reset_clip(RDPCLIENT * This)
	{
		rcClip.left = 0;
		rcClip.top = 0;
		rcClip.right = This->width + 1;
		rcClip.bottom = This->height + 1;
		SelectClipRgn(hdcBuffer, NULL);
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

#if 0
		HDC hdc = GetDC(hwnd);
		SelectObject(hdc, GetStockObject(NULL_PEN));
		SelectObject(hdc, CreateSolidBrush(RGB(255, 0, 0)));
		Rectangle(hdc, rcDamage.left, rcDamage.top, rcDamage.right + 1, rcDamage.bottom + 1);
		ReleaseDC(hwnd, hdc);
		Sleep(200);
#endif

		InvalidateRect(hwnd, &rcDamage, FALSE);
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

	void
	ui_paint_bitmap(RDPCLIENT * This, int x, int y, int cx, int cy, int width, int height, uint8 * data)
	{
		GdiFlush();

		int Bpp = This->server_depth / 8;
		int fromstride = alignup(width * Bpp, 4);
		int tostride = alignup(This->width * Bpp, 4);
		int sizex = cx * Bpp;

		const uint8 * src = data;
		uint8 * dst = (uint8 *)pBuffer + (This->height - y - cy) * tostride + x * Bpp;

		for(int i = 0; i < cy; ++ i)
		{
			memcpy(dst, src, sizex);
			src += fromstride;
			dst += tostride;
		}

		win32_repaint_area(This, x, y, cx, cy);
	}

	void
	ui_destblt(RDPCLIENT * This, uint8 opcode,
		   /* dest */ int x, int y, int cx, int cy)
	{
		HGDIOBJ holdbrush = SelectObject(hdcBuffer, GetStockObject(BLACK_BRUSH));
		PatBlt(hdcBuffer, x, y, cx, cy, MAKELONG(0, opcode));
		SelectObject(hdcBuffer, holdbrush);		

		win32_repaint_area(This, x, y, cx, cy);
	}

	static
	HBRUSH
	win32_create_brush(RDPCLIENT * This, BRUSH * brush, COLORREF fgcolour)
	{
		switch(brush->style)
		{
		case BS_SOLID:
		case BS_NULL:
		case BS_HATCHED:
		case BS_PATTERN:
		case BS_PATTERN8X8:
			break;

		default:
			return NULL;
		}

		switch(brush->style)
		{
		case BS_SOLID:
			return CreateSolidBrush(fgcolour);

		case BS_HATCHED:
			return CreateHatchBrush(brush->pattern[0], fgcolour);

		case BS_NULL:
			return (HBRUSH)GetStockObject(NULL_BRUSH);

		case BS_PATTERN:
		case BS_PATTERN8X8:
			{
				uint16 pattern[8];

				for(size_t i = 0; i < 8; ++ i)
					pattern[7 - i] = brush->pattern[i];

				HBITMAP hpattern = CreateBitmap(8, 8, 1, 1, pattern);
				HBRUSH hbr = CreatePatternBrush(hpattern);
				DeleteObject(hpattern);
				return hbr;
			}

		DEFAULT_UNREACHABLE;
		}
	}

	void
	ui_patblt(RDPCLIENT * This, uint8 opcode,
		  /* dest */ int x, int y, int cx, int cy,
		  /* brush */ BRUSH * brush, int bgcolour, int fgcolour)
	{
		HBRUSH hbr = win32_create_brush(This, brush, fgcolour);

		int oldbkcolor = SetBkColor(hdcBuffer, bgcolour);
		int oldtextcolor = SetTextColor(hdcBuffer, fgcolour);
		POINT oldbrushorg; SetBrushOrgEx(hdcBuffer, brush->xorigin, brush->yorigin, &oldbrushorg);
		HGDIOBJ holdbrush = SelectObject(hdcBuffer, hbr);		

		PatBlt(hdcBuffer, x, y, cx, cy, MAKELONG(0, opcode));

		SelectObject(hdcBuffer, holdbrush);
		SetBrushOrgEx(hdcBuffer, oldbrushorg.x, oldbrushorg.y, NULL);
		SetTextColor(hdcBuffer, oldtextcolor);
		SetBkColor(hdcBuffer, oldbkcolor);

		DeleteObject(hbr);

		win32_repaint_area(This, x, y, cx, cy);
	}

	void
	ui_screenblt(RDPCLIENT * This, uint8 opcode,
			 /* dest */ int x, int y, int cx, int cy,
			 /* src */ int srcx, int srcy)
	{
		BitBlt(hdcBuffer, x, y, cx, cy, hdcBuffer, srcx, srcy, MAKELONG(0, opcode));
		win32_repaint_area(This, x, y, cx, cy);
	}

	void
	ui_memblt(RDPCLIENT * This, uint8 opcode,
		  /* dest */ int x, int y, int cx, int cy,
		  /* src */ HBITMAP src, int srcx, int srcy)
	{
		HDC hdcSrc = CreateCompatibleDC(hdcBuffer);
		HGDIOBJ hOld = SelectObject(hdcSrc, src);

		BitBlt(hdcBuffer, x, y, cx, cy, hdcSrc, srcx, srcy, MAKELONG(0, opcode));

		SelectObject(hdcSrc, hOld);
		DeleteDC(hdcSrc);

		win32_repaint_area(This, x, y, cx, cy);
	}

	void
	ui_triblt(RDPCLIENT * This, uint8 opcode,
		  /* dest */ int x, int y, int cx, int cy,
		  /* src */ HBITMAP src, int srcx, int srcy,
		  /* brush */ BRUSH * brush, int bgcolour, int fgcolour)
	{
		HDC hdcSrc = CreateCompatibleDC(hdcBuffer);
		HGDIOBJ hOld = SelectObject(hdcSrc, src);

		//SELECT_BRUSH(brush, bgcolour, fgcolour);

		BitBlt(hdcBuffer, x, y, cx, cy, hdcSrc, srcx, srcy, MAKELONG(0, opcode));

		//RESET_BRUSH();

		SelectObject(hdcSrc, hOld);
		DeleteDC(hdcSrc);

		win32_repaint_area(This, x, y, cx, cy);
	}

	void
	ui_line(RDPCLIENT * This, uint8 opcode,
		/* dest */ int startx, int starty, int endx, int endy,
		/* pen */ PEN * pen)
	{
		HPEN hpen = CreatePen(pen->style, pen->width, pen->colour);

		int oldROP2 = SetROP2(hdcBuffer, opcode);
		HGDIOBJ holdpen = SelectObject(hdcBuffer, hpen);
		POINT oldpos; MoveToEx(hdcBuffer, startx, starty, &oldpos);

		LineTo(hdcBuffer, endx, endy);

		MoveToEx(hdcBuffer, oldpos.x, oldpos.y, NULL);
		SelectObject(hdcBuffer, holdpen);
		SetROP2(hdcBuffer, oldROP2);

		DeleteObject(hpen);

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
		HBRUSH hbr = CreateSolidBrush(colour);

		HGDIOBJ holdbrush = SelectObject(hdcBuffer, hbr);
		HGDIOBJ holdpen = SelectObject(hdcBuffer, GetStockObject(NULL_PEN));

		Rectangle(hdcBuffer, x, y, x + cx + 1, y + cy + 1);

		SelectObject(hdcBuffer, holdpen);
		SelectObject(hdcBuffer, holdbrush);
		
		DeleteObject(hbr);

		win32_repaint_area(This, x, y, cx, cy);
	}

	void
	ui_polygon(RDPCLIENT * This, uint8 opcode,
		   /* mode */ uint8 fillmode,
		   /* dest */ POINT * point, int npoints,
		   /* brush */ BRUSH * brush, int bgcolour, int fgcolour)
	{
		HBRUSH hbr = win32_create_brush(This, brush, fgcolour);

		int oldbkcolor = SetBkColor(hdcBuffer, bgcolour);
		int oldtextcolor = SetTextColor(hdcBuffer, fgcolour);
		int oldFillMode = SetPolyFillMode(hdcBuffer, fillmode);
		HGDIOBJ holdbrush = SelectObject(hdcBuffer, hbr);

		Polygon(hdcBuffer, point, npoints);

		SelectObject(hdcBuffer, holdbrush);
		SetPolyFillMode(hdcBuffer, oldFillMode);
		SetTextColor(hdcBuffer, oldtextcolor);
		SetBkColor(hdcBuffer, oldbkcolor);

		win32_repaint_poly(This, point, npoints, 0);
	}

	void
	ui_polyline(RDPCLIENT * This, uint8 opcode,
			/* dest */ POINT * points, int npoints,
			/* pen */ PEN * pen)
	{
		POINT last = points[0];

		for(int i = 1; i < npoints; ++ i)
		{
			points[i].x += last.x;
			points[i].y += last.y;
			last = points[i];
		}

		HPEN hpen = CreatePen(pen->style, pen->width, pen->colour);

		int oldROP2 = SetROP2(hdcBuffer, opcode);
		HGDIOBJ holdpen = SelectObject(hdcBuffer, hpen);

		Polyline(hdcBuffer, points, npoints);

		SelectObject(hdcBuffer, holdpen);
		SetROP2(hdcBuffer, oldROP2);

		DeleteObject(hpen);

		win32_repaint_poly(This, points, npoints, pen->width);
	}

	void
	ui_ellipse(RDPCLIENT * This, uint8 opcode,
		   /* mode */ uint8 fillmode,
		   /* dest */ int x, int y, int cx, int cy,
		   /* brush */ BRUSH * brush, int bgcolour, int fgcolour)
	{
		switch(fillmode)
		{
		case 0: // outline
			{
				HPEN hpen = CreatePen(PS_SOLID, 1, fgcolour);

				int oldROP2 = SetROP2(hdcBuffer, opcode);
				HGDIOBJ holdPen = SelectObject(hdcBuffer, hpen);

				Ellipse(hdcBuffer, x, y, x + cx, y + cy);

				SelectObject(hdcBuffer, holdPen);
				SetROP2(hdcBuffer, oldROP2);

				DeleteObject(hpen);
			}

			break;

		case 1:
			// TODO
			break;
		}

		win32_repaint_area(This, x, y, cx, cy);
	}

	void
	ui_draw_glyph(RDPCLIENT * This, int mixmode,
			  /* dest */ int x, int y, int cx, int cy,
			  /* src */ HGLYPH glyph, int srcx, int srcy,
			  int bgcolour, int fgcolour)
	{
		HBITMAP hbmGlyph = (HBITMAP)glyph;
		HDC hdcGlyph = CreateCompatibleDC(hdcBuffer);
		HGDIOBJ hOld = SelectObject(hdcGlyph, hbmGlyph);

		switch(mixmode)
		{
		case MIX_TRANSPARENT:
			{
				/*
					ROP is DSPDxax:
					 - where the glyph (S) is white, D is set to the foreground color (P)
					 - where the glyph (S) is black, D is left untouched

					This paints a transparent glyph in the specified color
				*/
				HBRUSH hbr = CreateSolidBrush(fgcolour);
				HGDIOBJ holdbrush = SelectObject(hdcBuffer, hbr);
				BitBlt(hdcBuffer, x, y, cx, cy, hdcGlyph, srcx, srcy, MAKELONG(0, 0xe2));
				SelectObject(hdcBuffer, holdbrush);
				DeleteObject(hbr);
			}

			break;

		case MIX_OPAQUE:
			{
				/* Curiously, glyphs are inverted (white-on-black) */
				int oldbkcolor = SetBkColor(hdcBuffer, fgcolour);
				int oldtextcolor = SetTextColor(hdcBuffer, bgcolour);
				BitBlt(hdcBuffer, x, y, cx, cy, hdcGlyph, srcx, srcy, SRCCOPY);
				SetTextColor(hdcBuffer, oldtextcolor);
				SetBkColor(hdcBuffer, oldbkcolor);
			}

			break;
		}

		SelectObject(hdcGlyph, hOld);
		DeleteDC(hdcGlyph);

		win32_repaint_area(This, x, y, cx, cy);
	}

#define DO_GLYPH(ttext,idx) \
{\
  glyph = cache_get_font (This, font, ttext[idx]);\
  if (!(flags & TEXT2_IMPLICIT_X))\
  {\
    xyoffset = ttext[++idx];\
    if ((xyoffset & 0x80))\
    {\
      if (flags & TEXT2_VERTICAL)\
        y += ttext[idx+1] | (ttext[idx+2] << 8);\
      else\
        x += ttext[idx+1] | (ttext[idx+2] << 8);\
      idx += 2;\
    }\
    else\
    {\
      if (flags & TEXT2_VERTICAL)\
        y += xyoffset;\
      else\
        x += xyoffset;\
    }\
  }\
  if (glyph != NULL)\
  {\
      ui_draw_glyph (This, mixmode, x + (short) glyph->offset,\
                     y + (short) glyph->baseline,\
                     glyph->width, glyph->height,\
                     glyph->pixmap, 0, 0, bgcolour, fgcolour);\
    if (flags & TEXT2_IMPLICIT_X)\
      x += glyph->width;\
  }\
}

	void
	ui_draw_text(RDPCLIENT * This, uint8 font, uint8 flags, uint8 opcode, int mixmode, int x, int y,
			 int clipx, int clipy, int clipcx, int clipcy,
			 int boxx, int boxy, int boxcx, int boxcy, BRUSH * brush,
			 int bgcolour, int fgcolour, uint8 * text, uint8 length)
	{
		FONTGLYPH * glyph;
		int i, j, xyoffset;
		DATABLOB *entry;

		HBRUSH hbr = CreateSolidBrush(bgcolour);
		HGDIOBJ holdbrush = SelectObject(hdcBuffer, hbr);
		HGDIOBJ holdpen = SelectObject(hdcBuffer, GetStockObject(NULL_PEN));

		if (boxcx > 1)
			Rectangle(hdcBuffer, boxx, boxy, boxx + boxcx + 1, boxy + boxcy + 1);
		else if (mixmode == MIX_OPAQUE)
			Rectangle(hdcBuffer, clipx, clipy, clipx + clipcx + 1, clipy + clipcy + 1);

		SelectObject(hdcBuffer, holdpen);
		SelectObject(hdcBuffer, holdbrush);
		
		DeleteObject(hbr);

		if(boxcx > 1)
			win32_repaint_area(This, boxx, boxy, boxcx, boxcy);
		else
			win32_repaint_area(This, clipx, clipy, clipcx, clipcy);

		/* Paint text, character by character */
		for (i = 0; i < length;)
		{
			switch (text[i])
			{
				case 0xff:
					/* At least two bytes needs to follow */
					if (i + 3 > length)
					{
						warning("Skipping short 0xff command:");
						for (j = 0; j < length; j++)
							fprintf(stderr, "%02x ", text[j]);
						fprintf(stderr, "\n");
						i = length = 0;
						break;
					}
					cache_put_text(This, text[i + 1], text, text[i + 2]);
					i += 3;
					length -= i;
					/* this will move pointer from start to first character after FF command */
					text = &(text[i]);
					i = 0;
					break;

				case 0xfe:
					/* At least one byte needs to follow */
					if (i + 2 > length)
					{
						warning("Skipping short 0xfe command:");
						for (j = 0; j < length; j++)
							fprintf(stderr, "%02x ", text[j]);
						fprintf(stderr, "\n");
						i = length = 0;
						break;
					}
					entry = cache_get_text(This, text[i + 1]);
					if (entry->data != NULL)
					{
						if ((((uint8 *) (entry->data))[1] == 0)
							&& (!(flags & TEXT2_IMPLICIT_X)) && (i + 2 < length))
						{
							if (flags & TEXT2_VERTICAL)
								y += text[i + 2];
							else
								x += text[i + 2];
						}
						for (j = 0; j < entry->size; j++)
							DO_GLYPH(((uint8 *) (entry->data)), j);
					}
					if (i + 2 < length)
						i += 3;
					else
						i += 2;
					length -= i;
					/* this will move pointer from start to first character after FE command */
					text = &(text[i]);
					i = 0;
					break;

				default:
					DO_GLYPH(text, i);
					i++;
					break;
			}
		}
	}

	void
	ui_desktop_save(RDPCLIENT * This, uint32 offset, int x, int y, int cx, int cy)
	{
		// TODO
	}

	void
	ui_desktop_restore(RDPCLIENT * This, uint32 offset, int x, int y, int cx, int cy)
	{
		// TODO
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
void
mstsc_mousewheel(RDPCLIENT * This, int value, LPARAM lparam)
{
	uint16 button;

	if(value < 0)
		button = MOUSE_FLAG_BUTTON5;
	else
		button = MOUSE_FLAG_BUTTON4;

	if(value < 0)
		value = - value;

	for(int click = 0; click < value; click += WHEEL_DELTA)
		rdp_send_input(This, GetTickCount(), RDP_INPUT_MOUSE, button | MOUSE_FLAG_DOWN, LOWORD(lparam), HIWORD(lparam));
}

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
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;

		// FIXME: temporary
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

		/* Initialization */
	case WM_CREATE:
		This = static_cast<RDPCLIENT *>(reinterpret_cast<LPCREATESTRUCT>(lparam)->lpCreateParams);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, PtrToLong(This));
		break;

		/* Painting */
	case WM_PRINTCLIENT:
		if(wparam == 0)
			break;

	case WM_PAINT:
		{
			HDC hdc = (HDC)wparam;

			// A DC was provided: print the whole client area into it
			if(hdc)
			{
				RECT rc;
				GetClientRect(hwnd, &rc);
				BitBlt(hdc, 0, 0, rc.right, rc.bottom, hdcBuffer, 0, 0, SRCCOPY);
			}
			// Otherwise, we're refreshing to screen
			else
			{
				PAINTSTRUCT ps;
				hdc = BeginPaint(hwnd, &ps);

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
		}

		break;

		/* Keyboard stuff */
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:		
		rdp_send_input(This, GetMessageTime(), RDP_INPUT_SCANCODE, RDP_KEYPRESS | (lparam & 0x1000000 ? KBD_FLAG_EXT : 0), LOBYTE(HIWORD(lparam)), 0);
		break;

	case WM_SYSKEYUP:
	case WM_KEYUP:
		rdp_send_input(This, GetMessageTime(), RDP_INPUT_SCANCODE, RDP_KEYRELEASE | (lparam & 0x1000000 ? KBD_FLAG_EXT : 0), LOBYTE(HIWORD(lparam)), 0);
		break;

		/* Mouse stuff */
		// Cursor shape
	case WM_SETCURSOR:
		if(LOWORD(lparam) == HTCLIENT)
		{
			SetCursor(hcursor);
			return TRUE;
		}

		break;

		// Movement
	case WM_MOUSEMOVE:
		if(This->sendmotion || wparam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON | MK_XBUTTON1 | MK_XBUTTON2))
			rdp_send_input(This, GetMessageTime(), RDP_INPUT_MOUSE, MOUSE_FLAG_MOVE, LOWORD(lparam), HIWORD(lparam));

		break;

		// Buttons
		// TODO: X buttons
	case WM_LBUTTONDOWN:
		rdp_send_input(This, GetMessageTime(), RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON1 | MOUSE_FLAG_DOWN, LOWORD(lparam), HIWORD(lparam));
		break;

	case WM_RBUTTONDOWN:
		rdp_send_input(This, GetMessageTime(), RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON2 | MOUSE_FLAG_DOWN, LOWORD(lparam), HIWORD(lparam));
		break;

	case WM_MBUTTONDOWN:
		rdp_send_input(This, GetMessageTime(), RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON3 | MOUSE_FLAG_DOWN, LOWORD(lparam), HIWORD(lparam));
		break;

	case WM_LBUTTONUP:
		rdp_send_input(This, GetMessageTime(), RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON1, LOWORD(lparam), HIWORD(lparam));
		break;

	case WM_RBUTTONUP:
		rdp_send_input(This, GetMessageTime(), RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON2, LOWORD(lparam), HIWORD(lparam));
		break;

	case WM_MBUTTONUP:
		rdp_send_input(This, GetMessageTime(), RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON3, LOWORD(lparam), HIWORD(lparam));
		break;

		// Wheel
	case WM_MOUSEWHEEL:
		mstsc_mousewheel(This, (SHORT)HIWORD(wparam), lparam);
		break;

	default:
		/* Registered messages */
		// Z-Mouse wheel support - you know, just in case
		if(uMsg == wmZMouseWheel)
		{
			mstsc_mousewheel(This, (int)wparam, lparam);
			break;
		}

		/* Unhandled messages */
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

	strcpy(This->username, "Administrator");

	DWORD dw = sizeof(This->hostname);
	GetComputerNameA(This->hostname, &dw);

	uint32 flags = RDP_LOGON_NORMAL | RDP_LOGON_COMPRESSION | RDP_LOGON_COMPRESSION2;

	//rdp_connect(This, "10.0.0.3", flags, "", "", "", "");
	rdp_connect(This, "192.168.7.232", flags, "", "", "", "");

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

	hbmBuffer = CreateDIBSection(hdcBuffer, &bmi, DIB_RGB_COLORS, &pBuffer, NULL, 0);

	SelectObject(hdcBuffer, hbmBuffer);

	rcClip.left = 0;
	rcClip.top = 0;
	rcClip.right = This->width;
	rcClip.bottom = This->height;

	BOOL deactivated;
	uint32 ext_disc_reason;

	rdp_main_loop(This, &deactivated, &ext_disc_reason);

	SendMessage(hwnd, WM_CLOSE, 0, 0);

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
	This->desktop_save = False; // True;
	This->polygon_ellipse_orders = False; // = True;
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

	hcursor = NULL;

	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(wc));

	wc.lpfnWndProc = mstsc_WndProc;
	wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(HOLLOW_BRUSH));
	wc.lpszClassName = TEXT("MissTosca_Desktop");

	wmZMouseWheel = RegisterWindowMessage(MSH_MOUSEWHEEL);

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
