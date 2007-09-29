#include "fitz.h"
#include "mupdf.h"
#include "pdfapp.h"

#include "gs_l.xbm"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

extern int ximage_init(Display *display, int screen, Visual *visual);
extern int ximage_get_depth(void);
extern Visual *ximage_get_visual(void);
extern Colormap ximage_get_colormap(void);
extern void ximage_blit(Drawable d, GC gc, int dstx, int dsty,
	unsigned char *srcdata,
	int srcx, int srcy, int srcw, int srch, int srcstride);

static Display *xdpy;
static Atom XA_TARGETS;
static Atom XA_TIMESTAMP;
static Atom XA_UTF8_STRING;
static int xscr;
static Window xwin;
static GC xgc;
static XEvent xevt;
static int mapped = 0;
static Cursor xcarrow, xchand, xcwait;
static int justcopied = 0;
static int dirty = 0;
static char *password = "";
static XColor xbgcolor;
static XColor xshcolor;
static int reqw = 0;
static int reqh = 0;
static char copylatin1[1024 * 16] = "";
static char copyutf8[1024 * 48] = "";
static Time copytime;

static pdfapp_t gapp;

/*
 * Dialog boxes
 */

void winwarn(pdfapp_t *app, char *msg)
{
	fprintf(stderr, "apparition: %s\n", msg);
}

void winerror(pdfapp_t *app, char *msg)
{
	fprintf(stderr, "apparition: %s\n", msg);
	exit(1);
}

char *winpassword(pdfapp_t *app, char *filename)
{
	char *r = password;
	password = NULL;
	return r;
}

/*
 * X11 magic
 */

void winopen(void)
{
	XWMHints *hints;

	xdpy = XOpenDisplay(nil);
	if (!xdpy)
		winerror(&gapp, "could not open display.");

	XA_TARGETS = XInternAtom(xdpy, "TARGETS", False);
	XA_TIMESTAMP = XInternAtom(xdpy, "TIMESTAMP", False);
	XA_UTF8_STRING = XInternAtom(xdpy, "UTF8_STRING", False);

	xscr = DefaultScreen(xdpy);

	ximage_init(xdpy, xscr, DefaultVisual(xdpy, xscr));

	xcarrow = XCreateFontCursor(xdpy, XC_left_ptr);
	xchand = XCreateFontCursor(xdpy, XC_hand2);
	xcwait = XCreateFontCursor(xdpy, XC_watch);

	xbgcolor.red = 0x7000;
	xbgcolor.green = 0x7000;
	xbgcolor.blue = 0x7000;

	xshcolor.red = 0x4000;
	xshcolor.green = 0x4000;
	xshcolor.blue = 0x4000;

	XAllocColor(xdpy, DefaultColormap(xdpy, xscr), &xbgcolor);
	XAllocColor(xdpy, DefaultColormap(xdpy, xscr), &xshcolor);

	xwin = XCreateWindow(xdpy, DefaultRootWindow(xdpy),
			10, 10, 200, 100, 1,
			ximage_get_depth(),
			InputOutput,
			ximage_get_visual(),
			0,
			nil);

	XSetWindowColormap(xdpy, xwin, ximage_get_colormap());
	XSelectInput(xdpy, xwin,
			StructureNotifyMask | ExposureMask | KeyPressMask |
			PointerMotionMask | ButtonPressMask | ButtonReleaseMask);

	mapped = 0;

	xgc = XCreateGC(xdpy, xwin, 0, nil);

	XDefineCursor(xdpy, xwin, xcarrow);

	hints = XAllocWMHints();
	if (hints)
	{
		hints->flags = IconPixmapHint;
		hints->icon_pixmap = XCreateBitmapFromData(xdpy, xwin,
				gs_l_xbm_bits, gs_l_xbm_width, gs_l_xbm_height);
		if (hints->icon_pixmap)
		{
			XSetWMHints(xdpy, xwin, hints);
		}
		XFree(hints);
	}
}

void wincursor(pdfapp_t *app, int curs)
{
	if (curs == ARROW)
		XDefineCursor(xdpy, xwin, xcarrow);
	if (curs == HAND)
		XDefineCursor(xdpy, xwin, xchand);
	if (curs == WAIT)
		XDefineCursor(xdpy, xwin, xcwait);
	XFlush(xdpy);
}

void wintitle(pdfapp_t *app, char *s)
{
#ifdef X_HAVE_UTF8_STRING
	Xutf8SetWMProperties(xdpy, xwin, s, s, 0, 0, 0, 0, 0);
#else
	XmbSetWMProperties(xdpy, xwin, s, s, 0, 0, 0, 0, 0);
#endif
}

void winconvert(pdfapp_t *app, fz_pixmap *image)
{
	// never mind
}

void winresize(pdfapp_t *app, int w, int h)
{
	XWindowChanges values;
	int mask;

	mask = CWWidth | CWHeight;
	values.width = w;
	values.height = h;
	XConfigureWindow(xdpy, xwin, mask, &values);

	reqw = w;
	reqh = h;

	if (!mapped)
	{
		gapp.winw = w;
		gapp.winh = h;

		XMapWindow(xdpy, xwin);
		XFlush(xdpy);

		while (1)
		{
			XNextEvent(xdpy, &xevt);
			if (xevt.type == MapNotify)
				break;
		}

		XSetForeground(xdpy, xgc, WhitePixel(xdpy, xscr));
		XFillRectangle(xdpy, xwin, xgc, 0, 0, gapp.image->w, gapp.image->h);
		XFlush(xdpy);

		mapped = 1;
	}
}

static void fillrect(int x, int y, int w, int h)
{
	if (w > 0 && h > 0)
		XFillRectangle(xdpy, xwin, xgc, x, y, w, h);
}

static void invertcopyrect()
{
	unsigned t, *p;
	int x, y;

	int x0 = gapp.selr.x0 - gapp.panx;
	int x1 = gapp.selr.x1 - gapp.panx;
	int y0 = gapp.selr.y0 - gapp.pany;
	int y1 = gapp.selr.y1 - gapp.pany;

	x0 = CLAMP(x0, 0, gapp.image->w - 1);
	x1 = CLAMP(x1, 0, gapp.image->w - 1);
	y0 = CLAMP(y0, 0, gapp.image->h - 1);
	y1 = CLAMP(y1, 0, gapp.image->h - 1);

	for (y = y0; y < y1; y++)
	{
		p = (unsigned *)(gapp.image->samples + (y * gapp.image->w + x0) * 4);
		for (x = x0; x < x1; x++)
		{
                        *p = ~0 - *p;
			p ++;
		}
	}

	justcopied = 1;
}

void winblit(pdfapp_t *app)
{
	int x0 = gapp.panx;
	int y0 = gapp.pany;
	int x1 = gapp.panx + gapp.image->w;
	int y1 = gapp.pany + gapp.image->h;

	XSetForeground(xdpy, xgc, xbgcolor.pixel);
	fillrect(0, 0, x0, gapp.winh);
	fillrect(x1, 0, gapp.winw - x1, gapp.winh);
	fillrect(0, 0, gapp.winw, y0);
	fillrect(0, y1, gapp.winw, gapp.winh - y1);

	XSetForeground(xdpy, xgc, xshcolor.pixel);
	fillrect(x0+2, y1, gapp.image->w, 2);
	fillrect(x1, y0+2, 2, gapp.image->h);

	if (gapp.iscopying || justcopied)
		invertcopyrect();

	ximage_blit(xwin, xgc,
			x0, y0,
			gapp.image->samples,
			0, 0,
			gapp.image->w,
			gapp.image->h,
			gapp.image->w * gapp.image->n);

	if (gapp.iscopying || justcopied)
		invertcopyrect();
}

void winrepaint(pdfapp_t *app)
{
	dirty = 1;
}

void windocopy(pdfapp_t *app)
{
	unsigned short copyucs2[16 * 1024];
	char *latin1 = copylatin1;
	char *utf8 = copyutf8;
	unsigned short *ucs2;
	int ucs;

	pdfapp_oncopy(&gapp, copyucs2, 16 * 1024);

	for (ucs2 = copyucs2; ucs2[0] != 0; ucs2++)
	{
		ucs = ucs2[0];

		utf8 += runetochar(utf8, &ucs);

		if (ucs < 256)
			*latin1++ = ucs;
		else
			*latin1++ = '?';
	}

	*utf8 = 0;
	*latin1 = 0;

printf("oncopy utf8=%d latin1=%d\n", strlen(copyutf8), strlen(copylatin1));

	XSetSelectionOwner(xdpy, XA_PRIMARY, xwin, copytime);

	justcopied = 1;
}

void onselreq(Window requestor, Atom selection, Atom target, Atom property, Time time)
{
	XEvent nevt;

printf("onselreq\n");

	if (property == None)
		property = target;

	nevt.xselection.type = SelectionNotify;
	nevt.xselection.send_event = True;
	nevt.xselection.display = xdpy;
	nevt.xselection.requestor = requestor;
	nevt.xselection.selection = selection;
	nevt.xselection.target = target;
	nevt.xselection.property = property;
	nevt.xselection.time = time;

	if (target == XA_TARGETS)
	{
		Atom atomlist[4];
		atomlist[0] = XA_TARGETS;
		atomlist[1] = XA_TIMESTAMP;
		atomlist[2] = XA_STRING;
		atomlist[3] = XA_UTF8_STRING;
printf(" -> targets\n");
		XChangeProperty(xdpy, requestor, property, target,
				32, PropModeReplace,
				(unsigned char *)atomlist, sizeof(atomlist)/sizeof(Atom));
	}

	else if (target == XA_STRING)
	{
printf(" -> string %d\n", strlen(copylatin1));
		XChangeProperty(xdpy, requestor, property, target,
				8, PropModeReplace,
				(unsigned char *)copylatin1, strlen(copylatin1));
	}

	else if (target == XA_UTF8_STRING)
	{
printf(" -> utf8string\n");
		XChangeProperty(xdpy, requestor, property, target,
				8, PropModeReplace,
				(unsigned char *)copyutf8, strlen(copyutf8));
	}

	else
	{
printf(" -> unknown\n");
		nevt.xselection.property = None;
	}

	XSendEvent(xdpy, requestor, False, SelectionNotify, &nevt);
}

void winopenuri(pdfapp_t *app, char *buf)
{
	char cmd[2048];
	if (getenv("BROWSER"))
		sprintf(cmd, "$BROWSER %s &", buf);
	else
		sprintf(cmd, "open %s", buf);
	system(cmd);
}

void onkey(int c)
{
	if (justcopied)
	{
		justcopied = 0;
		winrepaint(&gapp);
	}

	if (c == 'q')
		exit(0);

	pdfapp_onkey(&gapp, c);
}

void onmouse(int x, int y, int btn, int modifiers, int state)
{
	if (state != 0 && justcopied)
	{
		justcopied = 0;
		winrepaint(&gapp);
	}

	pdfapp_onmouse(&gapp, x, y, btn, modifiers, state);
}

void usage(void)
{
	fprintf(stderr, "usage: apparition [-d password] [-z zoom] [-p pagenumber] file.pdf\n");
	exit(1);
}

int main(int argc, char **argv)
{
	char *filename;
	int c;
	int len;
	unsigned char buf[128];
	KeySym keysym;
	int oldx = 0;
	int oldy = 0;
	double zoom = 1.0;
	int pageno = 1;

	while ((c = getopt(argc, argv, "d:z:p:")) != -1)
	{
		switch (c)
		{
		case 'd': password = optarg; break;
		case 'z': zoom = atof(optarg); break;
		case 'p': pageno = atoi(optarg); break;
		default: usage();
		}
	}

	if (argc - optind == 0)
		usage();

	filename = argv[optind++];

	fz_cpudetect();
	fz_accelerate();

	winopen();

	pdfapp_init(&gapp);
	gapp.scrw = DisplayWidth(xdpy, xscr);
	gapp.scrh = DisplayHeight(xdpy, xscr);
	gapp.zoom = zoom;
	gapp.pageno = pageno;

	pdfapp_open(&gapp, filename);

	while (1)
	{
		do
		{
			XNextEvent(xdpy, &xevt);

			switch (xevt.type)
			{
			case Expose:
				dirty = 1;
				break;

			case ConfigureNotify:
				if (gapp.image)
				{
					if (xevt.xconfigure.width != reqw ||
						xevt.xconfigure.height != reqh)
						gapp.shrinkwrap = 0;
				}
				pdfapp_onresize(&gapp,
						xevt.xconfigure.width,
						xevt.xconfigure.height);
				break;

			case KeyPress:
				len = XLookupString(&xevt.xkey, buf, sizeof buf, &keysym, 0);
				if (len)
					onkey(buf[0]);
				onmouse(oldx, oldy, 0, 0, 0);

				if (dirty)
				{
					winblit(&gapp);
					dirty = 0;
				}

				break;

			case MotionNotify:
				oldx = xevt.xbutton.x;
				oldy = xevt.xbutton.y;
				onmouse(xevt.xbutton.x, xevt.xbutton.y, xevt.xbutton.button, xevt.xbutton.state, 0);
				break;

			case ButtonPress:
				onmouse(xevt.xbutton.x, xevt.xbutton.y, xevt.xbutton.button, xevt.xbutton.state, 1);
				break;

			case ButtonRelease:
				copytime = xevt.xbutton.time;
				onmouse(xevt.xbutton.x, xevt.xbutton.y, xevt.xbutton.button, xevt.xbutton.state, -1);
				break;

			case SelectionRequest:
				onselreq(xevt.xselectionrequest.requestor,
						xevt.xselectionrequest.selection,
						xevt.xselectionrequest.target,
						xevt.xselectionrequest.property,
						xevt.xselectionrequest.time);
				break;
			}
		}
		while (XPending(xdpy));

		if (dirty)
		{
			winblit(&gapp);
			dirty = 0;
		}
	}

	pdfapp_close(&gapp);

	return 0;
}

