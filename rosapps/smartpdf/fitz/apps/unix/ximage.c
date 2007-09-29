/*
 * Blit ARGB images to X with X(Shm)Images
 */

#include <fitz.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>

extern int ffs(int);

typedef void (*ximage_convert_func_t)
	(
	 const unsigned char *src,
	 int srcstride,
	 unsigned char *dst,
	 int dststride,
	 int w,
	 int h
	);

#define POOLSIZE 4
#define WIDTH 256
#define HEIGHT 256

enum {
	ARGB8888,
	BGRA8888,
	RGBA8888,
	ABGR8888,
	RGB888,
	BGR888,
	RGB565,
	RGB565_BR,
	RGB555,
	RGB555_BR,
	BGR233,
	UNKNOWN
};

static char *modename[] = {
	"ARGB8888",
	"BGRA8888",
	"RGBA8888",
	"ABGR8888",
	"RGB888",
	"BGR888",
	"RGB565",
	"RGB565_BR",
	"RGB555",
	"RGB555_BR",
	"BGR233",
	"UNKNOWN"
};

extern ximage_convert_func_t ximage_convert_funcs[];

static struct
{
	Display *display;
	int screen;
	XVisualInfo visual;
	Colormap colormap;

	int bitsperpixel;
	int mode;

	XColor rgbcube[256];

	ximage_convert_func_t convert_func;

	int useshm;
	XImage *pool[POOLSIZE];
	/* MUST exist during the lifetime of the shared ximage according to the
	   xc/doc/hardcopy/Xext/mit-shm.PS.gz */
	XShmSegmentInfo shminfo[POOLSIZE];
	int lastused;
} info;

static XImage *
createximage(Display *dpy, Visual *vis, XShmSegmentInfo *xsi, int depth, int w, int h)
{
	XImage *img;
	Status status;

	if (!XShmQueryExtension(dpy)) goto fallback;

	img = XShmCreateImage(dpy, vis, depth, ZPixmap, nil, xsi, w, h);
	if (!img)
	{
		fprintf(stderr, "warn: could not XShmCreateImage\n");
		goto fallback;
	}

	xsi->shmid = shmget(IPC_PRIVATE,
		img->bytes_per_line * img->height,
		IPC_CREAT | 0777);
	if (xsi->shmid < 0)
	{
		XDestroyImage(img);
		fprintf(stderr, "warn: could not shmget\n");
		goto fallback;
	}

	img->data = xsi->shmaddr = shmat(xsi->shmid, 0, 0);
	if (img->data == (char*)-1)
	{
		XDestroyImage(img);
		fprintf(stderr, "warn: could not shmat\n");
		goto fallback;
	}

	xsi->readOnly = False;
	status = XShmAttach(dpy, xsi);
	if (!status)
	{
		shmdt(xsi->shmaddr);
		XDestroyImage(img);
		fprintf(stderr, "warn: could not XShmAttach\n");
		goto fallback;
	}

	XSync(dpy, False);

	shmctl(xsi->shmid, IPC_RMID, 0);

	return img;

fallback:
	info.useshm = 0;

	img = XCreateImage(dpy, vis, depth, ZPixmap, 0, 0, w, h, 32, 0);
	if (!img)
	{
		fprintf(stderr, "fail: could not XCreateImage");
		abort();
	}

	img->data = malloc(h * img->bytes_per_line);
	if (!img->data)
	{
		fprintf(stderr, "fail: could not malloc");
		abort();
	}

	return img;
}

static void
make_colormap(void)
{
	if (info.visual.class == PseudoColor && info.visual.depth == 8)
	{
		int i, r, g, b;
		i = 0;
		for (b = 0; b < 4; b++) {
			for (g = 0; g < 8; g++) {
				for (r = 0; r < 8; r++) {
					info.rgbcube[i].pixel = i;
					info.rgbcube[i].red = (r * 36) << 8;
					info.rgbcube[i].green = (g * 36) << 8;
					info.rgbcube[i].blue = (b * 85) << 8;
					info.rgbcube[i].flags =
						DoRed | DoGreen | DoBlue;
					i++;
				}
			}
		}
		info.colormap = XCreateColormap(info.display,
					RootWindow(info.display, info.screen),
					info.visual.visual,
					AllocAll);
		XStoreColors(info.display, info.colormap, info.rgbcube, 256);
		return;
	}
	else if (info.visual.class == TrueColor)
	{
		info.colormap = 0;
		return;
	}
	fprintf(stderr, "Cannot handle visual class %d with depth: %d\n",
			info.visual.class, info.visual.depth);
	return;
}

static void
select_mode(void)
{

	int byteorder;
	int byterev;
	unsigned long rm, gm, bm;
	unsigned long rs, gs, bs;

	byteorder = ImageByteOrder(info.display);
#if BYTE_ORDER == BIG_ENDIAN
	byterev = byteorder != MSBFirst;
#else
	byterev = byteorder != LSBFirst;
#endif

	rm = info.visual.red_mask;
	gm = info.visual.green_mask;
	bm = info.visual.blue_mask;

	rs = ffs(rm) - 1;
	gs = ffs(gm) - 1;
	bs = ffs(bm) - 1;

	printf("ximage: mode %d/%d %08lx %08lx %08lx (%ld,%ld,%ld) %s%s\n",
			info.visual.depth,
			info.bitsperpixel,
			rm, gm, bm, rs, gs, bs,
			byteorder == MSBFirst ? "msb" : "lsb",
			byterev ? " <swap>":"");

	info.mode = UNKNOWN;
	if (info.bitsperpixel == 8) {
		/* Either PseudoColor with BGR233 colormap, or TrueColor */
		info.mode = BGR233;
	}
	else if (info.bitsperpixel == 16) {
		if (rm == 0xF800 && gm == 0x07E0 && bm == 0x001F)
			info.mode = !byterev ? RGB565 : RGB565_BR;
		if (rm == 0x7C00 && gm == 0x03E0 && bm == 0x001F)
			info.mode = !byterev ? RGB555 : RGB555_BR;
	}
	else if (info.bitsperpixel == 24) {
		if (rs == 0 && gs == 8 && bs == 16)
			info.mode = byteorder == MSBFirst ? RGB888 : BGR888;
		if (rs == 16 && gs == 8 && bs == 0)
			info.mode = byteorder == MSBFirst ? BGR888 : RGB888;
	}
	else if (info.bitsperpixel == 32) {
		if (rs ==  0 && gs ==  8 && bs == 16)
			info.mode = byteorder == MSBFirst ? ABGR8888 : RGBA8888;
		if (rs ==  8 && gs == 16 && bs == 24)
			info.mode = byteorder == MSBFirst ? BGRA8888 : ARGB8888;
		if (rs == 16 && gs ==  8 && bs ==  0)
			info.mode = byteorder == MSBFirst ? ARGB8888 : BGRA8888;
		if (rs == 24 && gs == 16 && bs ==  8)
			info.mode = byteorder == MSBFirst ? RGBA8888 : ABGR8888;
	}

	printf("ximage: ARGB8888 to %s\n", modename[info.mode]);

	/* select conversion function */
	info.convert_func = ximage_convert_funcs[info.mode];
}

static int
create_pool(void)
{
	int i;

	info.lastused = 0;

	for (i = 0; i < POOLSIZE; i++) {
		info.pool[i] = nil;
	}

	for (i = 0; i < POOLSIZE; i++) {
		info.pool[i] = createximage(info.display,
					info.visual.visual, &info.shminfo[i], info.visual.depth,
					WIDTH, HEIGHT);
		if (info.pool[i] == nil) {
			return 0;
		}
	}

	return 1;
}

static XImage *
next_pool_image(void)
{
	if (info.lastused + 1 >= POOLSIZE) {
		if (info.useshm)
			XSync(info.display, False);
		else
			XFlush(info.display);
		info.lastused = 0;
	}
	return info.pool[info.lastused ++];
}

int
ximage_init(Display *display, int screen, Visual *visual)
{
	XVisualInfo template;
	XVisualInfo *visuals;
	int nvisuals;
	XPixmapFormatValues *formats;
	int nformats;
	int ok;
	int i;

	info.display = display;
	info.screen = screen;
	info.colormap = 0;

	/* Get XVisualInfo for this visual */
	template.visualid = XVisualIDFromVisual(visual);
	visuals = XGetVisualInfo(display, VisualIDMask, &template, &nvisuals);
	if (nvisuals != 1) {
		fprintf(stderr, "Visual not found!\n");
		XFree(visuals);
		return 0;
	}
	memcpy(&info.visual, visuals, sizeof (XVisualInfo));
	XFree(visuals);

	/* Get appropriate PixmapFormat for this visual */
	formats = XListPixmapFormats(info.display, &nformats);
	for (i = 0; i < nformats; i++) {
		if (formats[i].depth == info.visual.depth) {
			info.bitsperpixel = formats[i].bits_per_pixel;
			break;
		}
	}
	XFree(formats);
	if (i == nformats) {
		fprintf(stderr, "PixmapFormat not found!\n");
		return 0;
	}

	/* extract mode */
	select_mode();

	/* prepare colormap */
	make_colormap();

	/* prepare pool of XImages */
	info.useshm = 1;
	ok = create_pool();
	if (!ok)
		return 0;

	printf("ximage: %sPutImage\n", info.useshm ? "XShm" : "X");

	return 1;
}

int
ximage_get_depth(void)
{
	return info.visual.depth;
}

Visual *
ximage_get_visual(void)
{
	return info.visual.visual;
}

Colormap
ximage_get_colormap(void)
{
	return info.colormap;
}

void
ximage_blit(Drawable d, GC gc,
		int dstx, int dsty,
		unsigned char *srcdata,
		int srcx, int srcy,
		int srcw, int srch,
		int srcstride)
{
	XImage *image;
	int ax, ay;
	int w, h;
	unsigned char *srcptr;

	for (ay = 0; ay < srch; ay += HEIGHT)
	{
		h = MIN(srch - ay, HEIGHT);
		for (ax = 0; ax < srcw; ax += WIDTH)
		{
			w = MIN(srcw - ax, WIDTH);

			image = next_pool_image();

			srcptr = srcdata +
				(ay + srcy) * srcstride +
				(ax + srcx) * 4;

			info.convert_func(srcptr, srcstride,
					  image->data,
					  image->bytes_per_line, w, h);

			if (info.useshm)
			{
				XShmPutImage(info.display, d, gc, image,
					0, 0, dstx + ax, dsty + ay,
					w, h, False);
			}
			else
			{
				XPutImage(info.display, d, gc, image,
					0, 0,
					dstx + ax,
					dsty + ay,
					w, h);
			}
		}
	}
}

/*
 * Primitive conversion functions
 */

#ifndef restrict
#ifndef _C99
#ifdef __GNUC__
#define restrict __restrict__
#else
#define restrict
#endif
#endif
#endif

#define PARAMS \
	 const unsigned char * restrict src, \
	 int srcstride, \
	 unsigned char * restrict dst, \
	 int dststride, \
	 int w, \
	 int h

/*
 * Convert byte:ARGB8888 to various formats
 */

static void
ximage_convert_argb8888(PARAMS)
{
	int x, y;
	unsigned * restrict s = (unsigned * restrict )src;
	unsigned * restrict d = (unsigned * restrict )dst;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			d[x] = s[x];
		}
		d += dststride>>2;
		s += srcstride>>2;
	}
}

static void
ximage_convert_bgra8888(PARAMS)
{
	int x, y;
	unsigned *s = (unsigned *)src;
	unsigned *d = (unsigned *)dst;
	unsigned val;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			val = s[x];
			d[x] =
				(val >> 24) |
				((val >> 8) & 0xff00) |
				(val << 24) |
				((val << 8) & 0xff0000);
/*
			d[x] =
				(((val >> 24) & 0xff) <<  0) |
				(((val >> 16) & 0xff) <<  8) |
				(((val >>  8) & 0xff) << 16) |
				(((val >>  0) & 0xff) << 24);
*/
		}
		d += dststride>>2;
		s += srcstride>>2;
	}
}

/* following have yet to recieve some MMX love ;-) */

static void
ximage_convert_abgr8888(PARAMS)
{
	int x, y;
	unsigned *s = (unsigned *)src;
	unsigned *d = (unsigned *)dst;
	unsigned val;

	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			val = s[x];
#if 1 /* FZ_MSB */
			d[x] = (val & 0xff00ff00) |
				(((val << 16) | (val >> 16)) & 0x00ff00ff);
#else /* FZ_LSB */
			d[x] = (val << 24) | ((val >> 8) & 0xff);
#endif
		}
		d += dststride>>2;
		s += srcstride>>2;
	}
}

static void
ximage_convert_rgba8888(PARAMS)
{
	int x, y;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			dst[x * 4 + 0] = src[x * 4 + 1];
			dst[x * 4 + 1] = src[x * 4 + 2];
			dst[x * 4 + 2] = src[x * 4 + 3];
			dst[x * 4 + 3] = src[x * 4 + 0];
		}
		dst += dststride;
		src += srcstride;
	}
}

static void
ximage_convert_bgr888(PARAMS)
{
	int x, y;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			dst[3*x + 0] = src[4*x + 3];
			dst[3*x + 1] = src[4*x + 2];
			dst[3*x + 2] = src[4*x + 1];
		}
		src += srcstride;
		dst += dststride;
	}
}

static void
ximage_convert_rgb888(PARAMS)
{
	int x, y;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			dst[3*x + 0] = src[4*x + 1];
			dst[3*x + 1] = src[4*x + 2];
			dst[3*x + 2] = src[4*x + 3];
		}
		src += srcstride;
		dst += dststride;
	}
}

static void
ximage_convert_rgb565(PARAMS)
{
	unsigned char r, g, b;
	int x, y;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			r = src[4*x + 1];
			g = src[4*x + 2];
			b = src[4*x + 3];
			((unsigned short *)dst)[x] =
				((r & 0xF8) << 8) |
				((g & 0xFC) << 3) |
				(b >> 3);
		}
		src += srcstride;
		dst += dststride;
	}
}

static void
ximage_convert_rgb565_br(PARAMS)
{
	unsigned char r, g, b;
	int x, y;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			r = src[4*x + 1];
			g = src[4*x + 2];
			b = src[4*x + 3];
			/* final word is:
			   g4 g3 g2 b7 b6 b5 b4 b3  r7 r6 r5 r4 r3 g7 g6 g5
			 */
			((unsigned short *)dst)[x] =
				(r & 0xF8) |
				((g & 0xE0) >> 5) |
				((g & 0x1C) << 11) |
				((b & 0xF8) << 5);
		}
		src += srcstride;
		dst += dststride;
	}
}

static void
ximage_convert_rgb555(PARAMS)
{
	unsigned char r, g, b;
	int x, y;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			r = src[4*x + 1];
			g = src[4*x + 2];
			b = src[4*x + 3];
			((unsigned short *)dst)[x] =
				((r & 0xF8) << 7) |
				((g & 0xF8) << 2) |
				(b >> 3);
		}
		src += srcstride;
		dst += dststride;
	}
}

static void
ximage_convert_rgb555_br(PARAMS)
{
	unsigned char r, g, b;
	int x, y;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			r = src[4*x + 1];
			g = src[4*x + 2];
			b = src[4*x + 3];
			/* final word is:
			   g5 g4 g3 b7 b6 b5 b4 b3  0 r7 r6 r5 r4 r3 g7 g6
			 */
			((unsigned short *)dst)[x] =
				((r & 0xF8) >> 1) |
				((g & 0xC0) >> 6) |
				((g & 0x38) << 10) |
				((b & 0xF8) << 5);
		}
		src += srcstride;
		dst += dststride;
	}
}

static void
ximage_convert_bgr233(PARAMS)
{
	unsigned char r, g, b;
	int x,y;
	for(y = 0; y < h; y++) {
		for(x = 0; x < w; x++) {
			r = src[4*x + 1];
			g = src[4*x + 2];
			b = src[4*x + 3];
			/* format: b7 b6 g7 g6 g5 r7 r6 r5 */
			dst[x] = (b&0xC0) | ((g>>2)&0x38) | ((r>>5)&0x7);
		}
		src += srcstride;
		dst += dststride;
	}
}

ximage_convert_func_t ximage_convert_funcs[] = {
	ximage_convert_argb8888,
	ximage_convert_bgra8888,
	ximage_convert_rgba8888,
	ximage_convert_abgr8888,
	ximage_convert_rgb888,
	ximage_convert_bgr888,
	ximage_convert_rgb565,
	ximage_convert_rgb565_br,
	ximage_convert_rgb555,
	ximage_convert_rgb555_br,
	ximage_convert_bgr233,
};

