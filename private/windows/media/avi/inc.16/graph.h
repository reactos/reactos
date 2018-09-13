/***
*graph.h - declare constants, functions, and macros for graphics library
*
*   Copyright (c) 1987 - 1992, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   This file declares the graphics library functions and the
*   structures and manifest constants that are used with them.
*
***************************************************************************/

#ifndef	_WINDOWS
/* Force graphics.lib to be linked in if graph.h used */
#pragma comment(lib,"graphics.lib")
#endif

#ifdef __cplusplus
extern "C" {			/* allow use with C++ */
#endif

#if (_MSC_VER <= 600)
#define	__cdecl	_cdecl
#define	__far	_far
#define	__huge	_huge
#endif

/* force word packing to avoid possible -Zp override */
#pragma pack(2)


/* user-visible declarations for Quick-C Graphics Library */

#ifndef _VIDEOCONFIG_DEFINED
/* structure for _getvideoconfig() as visible to user */
struct _videoconfig {
	short numxpixels;	/* number of pixels on X axis */
	short numypixels;	/* number of pixels on Y axis */
	short numtextcols;	/* number of text columns available */
	short numtextrows;	/* number of text rows available */
	short numcolors;	/* number of actual colors */
	short bitsperpixel;	/* number of bits per pixel */
	short numvideopages;	/* number of available video pages */
	short mode;		/* current video mode */
	short adapter;		/* active display adapter */
	short monitor;		/* active display monitor */
	short memory;		/* adapter video memory in K bytes */
};
#define _VIDEOCONFIG_DEFINED
#endif

#ifndef _XYCOORD_DEFINED
/* return value of _setvieworg(), etc. */
struct _xycoord {
	short xcoord;
	short ycoord;
};
#define _XYCOORD_DEFINED
#endif

/* structure for text position */
#ifndef _RCCOORD_DEFINED
struct _rccoord {
	short row;
	short col;
};
#define _RCCOORD_DEFINED
#endif

#ifndef __STDC__
/* Non-ANSI names for compatibility */
#define videoconfig	_videoconfig
#define xycoord		_xycoord
#define rccoord		_rccoord
#endif


/* ERROR HANDLING */
short __far __cdecl _grstatus(void);

/* Error Status Information returned by _grstatus() */

/* successful */
#define	_GROK                        0

/* errors */
#define _GRERROR                    (-1)
#define	_GRMODENOTSUPPORTED	    (-2)
#define	_GRNOTINPROPERMODE          (-3)
#define _GRINVALIDPARAMETER         (-4)
#define	_GRFONTFILENOTFOUND         (-5)
#define	_GRINVALIDFONTFILE          (-6)
#define _GRCORRUPTEDFONTFILE        (-7)
#define _GRINSUFFICIENTMEMORY       (-8)
#define _GRINVALIDIMAGEBUFFER       (-9)

/* warnings */
#define _GRNOOUTPUT                  1
#define _GRCLIPPED                   2
#define _GRPARAMETERALTERED          3
#define _GRTEXTNOTSUPPORTED          4


/* SETUP AND CONFIGURATION */

short __far __cdecl _setvideomode(short);
short __far __cdecl _setvideomoderows(short,short); /* return rows; 0 if error */

/* arguments to _setvideomode() */
#define _MAXRESMODE	(-3)	/* graphics mode with highest resolution */
#define _MAXCOLORMODE	(-2)	/* graphics mode with most colors */
#define _DEFAULTMODE	(-1)	/* restore screen to original mode */
#define _TEXTBW40	0	/* 40-column text, 16 grey */
#define _TEXTC40	1	/* 40-column text, 16/8 color */
#define _TEXTBW80	2	/* 80-column text, 16 grey */
#define _TEXTC80	3	/* 80-column text, 16/8 color */
#define _MRES4COLOR	4	/* 320 x 200, 4 color */
#define _MRESNOCOLOR	5	/* 320 x 200, 4 grey */
#define _HRESBW		6	/* 640 x 200, BW */
#define _TEXTMONO	7	/* 80-column text, BW */
#define _HERCMONO	8	/* 720 x 348, BW for HGC */
#define _MRES16COLOR	13	/* 320 x 200, 16 color */
#define _HRES16COLOR	14	/* 640 x 200, 16 color */
#define _ERESNOCOLOR	15	/* 640 x 350, BW */
#define _ERESCOLOR	16	/* 640 x 350, 4 or 16 color */
#define _VRES2COLOR	17	/* 640 x 480, BW */
#define _VRES16COLOR	18	/* 640 x 480, 16 color */
#define _MRES256COLOR	19	/* 320 x 200, 256 color */
#define _ORESCOLOR	64	/* 640 x 400, 1 of 16 colors (Olivetti only) */

/* the following 8 modes require VESA SuperVGA BIOS extensions */
#define	_ORES256COLOR	0x0100	/* 640 x 400, 256 color */
#define	_VRES256COLOR	0x0101	/* 640 x 480, 256 color */

/* WARNING: DO NOT attempt to set the following modes without ensuring that
   your monitor can safely handle that resolution.  Otherwise, you may risk
   damaging your display monitor!  Consult your owner's manual for details.
   Note: _MAXRESMODE and _MAXCOLORMODE never select SRES, XRES, or ZRES modes */

/* requires NEC MultiSync 3D or equivalent, or better */
#define	_SRES16COLOR	0x0102	/* 800 x 600, 16 color */
#define	_SRES256COLOR	0x0103	/* 800 x 600, 256 color */

/* requires NEC MultiSync 4D or equivalent, or better */
#define	_XRES16COLOR	0x0104	/* 1024 x 768, 16 color */
#define	_XRES256COLOR	0x0105	/* 1024 x 768, 256 color */

/* requires NEC MultiSync 5D or equivalent, or better */
#define	_ZRES16COLOR	0x0106	/* 1280 x 1024, 16 color */
#define	_ZRES256COLOR	0x0107	/* 1280 x 1024, 256 color */


short __far __cdecl _setactivepage(short);
short __far __cdecl _setvisualpage(short);
short __far __cdecl _getactivepage(void);
short __far __cdecl _getvisualpage(void);

/* _videoconfig adapter values */
/* these manifest constants can be used to determine the type of the active  */
/* adapter, using either simple comparisons or the bitwise-AND operator (&)  */
#define _MDPA		0x0001	/* Monochrome Display Adapter	      (MDPA) */
#define _CGA		0x0002	/* Color Graphics Adapter	      (CGA)  */
#define _EGA		0x0004	/* Enhanced Graphics Adapter	      (EGA)  */
#define _VGA		0x0008	/* Video Graphics Array		      (VGA)  */
#define _MCGA		0x0010	/* MultiColor Graphics Array	      (MCGA) */
#define _HGC		0x0020	/* Hercules Graphics Card	      (HGC)  */
#define _OCGA		0x0042	/* Olivetti Color Graphics Adapter    (OCGA) */
#define _OEGA		0x0044	/* Olivetti Enhanced Graphics Adapter (OEGA) */
#define _OVGA		0x0048	/* Olivetti Video Graphics Array      (OVGA) */
#define _SVGA		0x0088	/* Super VGA with VESA BIOS support   (SVGA) */

/* _videoconfig monitor values */
/* these manifest constants can be used to determine the type of monitor in */
/* use, using either simple comparisons or the bitwise-AND operator (&) */
#define _MONO		0x0001	/* Monochrome */
#define _COLOR		0x0002	/* Color (or Enhanced emulating color) */
#define _ENHCOLOR	0x0004	/* Enhanced Color */
#define _ANALOGMONO	0x0008	/* Analog Monochrome only */
#define _ANALOGCOLOR	0x0010	/* Analog Color only */
#define _ANALOG		0x0018	/* Analog Monochrome and Color modes */

struct _videoconfig __far * __far __cdecl _getvideoconfig(struct _videoconfig __far *);


/* COORDINATE SYSTEMS */

struct _xycoord __far __cdecl _setvieworg(short, short);
#define _setlogorg _setvieworg		/* obsolescent */

struct _xycoord __far __cdecl _getviewcoord(short, short);
#define _getlogcoord _getviewcoord	/* obsolescent */

struct _xycoord __far __cdecl _getphyscoord(short, short);

void __far __cdecl _setcliprgn(short, short, short, short);
void __far __cdecl _setviewport(short, short, short, short);


/* OUTPUT ROUTINES */

/* control parameters for _ellipse, _rectangle, _pie and _polygon */
#define _GBORDER	2	/* draw outline only */
#define _GFILLINTERIOR	3	/* fill using current fill mask */

/* parameters for _clearscreen */
#define _GCLEARSCREEN	0
#define _GVIEWPORT	1
#define _GWINDOW	2

void __far __cdecl _clearscreen(short);

struct _xycoord __far __cdecl _moveto(short, short);
struct _xycoord __far __cdecl _getcurrentposition(void);

short __far __cdecl _lineto(short, short);
short __far __cdecl _rectangle(short, short, short, short, short);
short __far __cdecl _polygon(short, const struct _xycoord __far *, short);
short __far __cdecl _arc(short, short, short, short, short, short, short, short);
short __far __cdecl _ellipse(short, short, short, short, short);
short __far __cdecl _pie(short, short, short, short, short, short, short, short, short);

short __far __cdecl _getarcinfo(struct _xycoord __far *, struct _xycoord __far *, struct _xycoord __far *);

short __far __cdecl _setpixel(short, short);
short __far __cdecl _getpixel(short, short);
short __far __cdecl _floodfill(short, short, short);


/* PEN COLOR, LINE STYLE, WRITE MODE, FILL PATTERN */

short __far __cdecl _setcolor(short);
short __far __cdecl _getcolor(void);

void __far __cdecl _setlinestyle(unsigned short);
unsigned short __far __cdecl _getlinestyle(void);

short __far __cdecl _setwritemode(short);
short __far __cdecl _getwritemode(void);

void __far __cdecl _setfillmask(const unsigned char __far *);
unsigned char __far * __far __cdecl _getfillmask(unsigned char __far *);


/* COLOR SELECTION */

long __far __cdecl _setbkcolor(long);
long __far __cdecl _getbkcolor(void);

long __far __cdecl _remappalette(short, long);
short __far __cdecl _remapallpalette(const long __far *);
short __far __cdecl _selectpalette(short);


/* TEXT */
/* parameters for _displaycursor */
#define _GCURSOROFF	0
#define _GCURSORON	1

/* parameters for _wrapon */
#define _GWRAPOFF	0
#define _GWRAPON	1


/* direction parameters for _scrolltextwindow */
#define _GSCROLLUP	1
#define _GSCROLLDOWN	(-1)

/* request maximum number of rows in _settextrows and _setvideomoderows */
#define _MAXTEXTROWS	(-1)

short __far __cdecl _settextrows(short); /* returns # rows set; 0 if error */
void __far __cdecl _settextwindow(short, short, short, short);
void __far __cdecl _gettextwindow(short __far *, short __far *, short __far *, short __far *);
void __far __cdecl _scrolltextwindow(short);
void __far __cdecl _outmem(const char __far *, short);
void __far __cdecl _outtext(const char __far *);
short __far __cdecl _inchar(void);
short __far __cdecl _wrapon(short);

short __far __cdecl _displaycursor(short);
short __far __cdecl _settextcursor(short);
short __far __cdecl _gettextcursor(void);

struct _rccoord __far __cdecl _settextposition(short, short);
struct _rccoord __far __cdecl _gettextposition(void);

short __far __cdecl _settextcolor(short);
short __far __cdecl _gettextcolor(void);


/* SCREEN IMAGES */

void __far __cdecl _getimage(short, short, short, short, char __huge *);
void __far __cdecl _putimage(short, short, char __huge *, short);
long __far __cdecl _imagesize(short, short, short, short);

/* "action verbs" for _putimage() and _setwritemode() */
#define _GPSET		3
#define _GPRESET	2
#define _GAND		1
#define _GOR		0
#define _GXOR		4


/* Color values are used with _setbkcolor in graphics modes and also by
   _remappalette and _remapallpalette.  Also known as palette colors.
   Not to be confused with color indices (aka. color attributes).  */

/* universal color values (all color modes): */
#define _BLACK		0x000000L
#define _BLUE		0x2a0000L
#define _GREEN		0x002a00L
#define _CYAN		0x2a2a00L
#define _RED		0x00002aL
#define _MAGENTA	0x2a002aL
#define _BROWN		0x00152aL
#define _WHITE		0x2a2a2aL
#define _GRAY		0x151515L
#define _LIGHTBLUE	0x3F1515L
#define _LIGHTGREEN	0x153f15L
#define _LIGHTCYAN	0x3f3f15L
#define _LIGHTRED	0x15153fL
#define _LIGHTMAGENTA	0x3f153fL
#define _YELLOW		0x153f3fL
#define _BRIGHTWHITE	0x3f3f3fL

/* the following is obsolescent and defined only for backward compatibility */
#define _LIGHTYELLOW	_YELLOW

/* mono mode F (_ERESNOCOLOR) color values: */
#define _MODEFOFF	0L
#define _MODEFOFFTOON	1L
#define _MODEFOFFTOHI	2L
#define _MODEFONTOOFF	3L
#define _MODEFON	4L
#define _MODEFONTOHI	5L
#define _MODEFHITOOFF	6L
#define _MODEFHITOON	7L
#define _MODEFHI	8L

/* mono mode 7 (_TEXTMONO) color values: */
#define _MODE7OFF	0L
#define _MODE7ON	1L
#define _MODE7HI	2L


/* Warning:  these '_xy' entrypoints are undocumented.
   They may or may not be supported in future versions. */
struct _xycoord __far __cdecl _moveto_xy(struct _xycoord);
short __far __cdecl _lineto_xy(struct _xycoord);
short __far __cdecl _rectangle_xy(short,struct _xycoord,struct _xycoord);
short __far __cdecl _arc_xy(struct _xycoord, struct _xycoord, struct _xycoord, struct _xycoord);
short __far __cdecl _ellipse_xy(short, struct _xycoord, struct _xycoord);
short __far __cdecl _pie_xy(short, struct _xycoord, struct _xycoord, struct _xycoord, struct _xycoord);
short __far __cdecl _getpixel_xy(struct _xycoord);
short __far __cdecl _setpixel_xy(struct _xycoord);
short __far __cdecl _floodfill_xy(struct _xycoord, short);
void __far __cdecl _getimage_xy(struct _xycoord,struct _xycoord, char __huge *);
long __far __cdecl _imagesize_xy(struct _xycoord,struct _xycoord);
void __far __cdecl _putimage_xy(struct _xycoord, char __huge *, short);


/* WINDOW COORDINATE SYSTEM */

#ifndef _WXYCOORD_DEFINED
/* structure for window coordinate pair */
struct _wxycoord {
	double wx;	/* window x coordinate */
	double wy;	/* window y coordinate */
	};
#define _WXYCOORD_DEFINED
#endif


/* define real coordinate window - returns non-zero if successful */
short __far __cdecl _setwindow(short,double,double,double,double);

/* convert from view to window coordinates */
struct _wxycoord __far __cdecl _getwindowcoord(short,short);
struct _wxycoord __far __cdecl _getwindowcoord_xy(struct _xycoord);

/* convert from window to view coordinates */
struct _xycoord __far __cdecl _getviewcoord_w(double,double);
struct _xycoord __far __cdecl _getviewcoord_wxy(const struct _wxycoord __far *);

/*	return the window coordinates of the current graphics output
	position as an _wxycoord structure. no error return. */
struct _wxycoord __far __cdecl _getcurrentposition_w(void);


/* window coordinate entry points for graphics output routines */

/*	returns nonzero if successful; otherwise 0	*/
short __far __cdecl _arc_w(double, double, double, double, double, double, double, double);
short __far __cdecl _arc_wxy(const struct _wxycoord __far *, const struct _wxycoord __far *, const struct _wxycoord __far *, const struct _wxycoord __far *);

/*	returns nonzero if successful; otherwise 0	*/
short __far __cdecl _ellipse_w(short, double, double, double, double);
short __far __cdecl _ellipse_wxy(short, const struct _wxycoord __far *, const struct _wxycoord __far *);

/*	returns nonzero if successful; otherwise 0	*/
short __far __cdecl _floodfill_w(double, double, short);

/*	returns pixel value at given point; -1 if unsuccessful. */
short __far __cdecl _getpixel_w(double, double);

/*	returns nonzero if successful; otherwise 0	*/
short __far __cdecl _lineto_w(double, double);

/*	returns the view coordinates of the previous output
	position as a _wxycoord structure. no error return */
struct _wxycoord __far __cdecl _moveto_w(double, double);

/*	returns nonzero if successful; otherwise 0	*/
short __far __cdecl _pie_w(short, double, double, double, double, double, double, double, double);
short __far __cdecl _pie_wxy(short, const struct _wxycoord __far *, const struct _wxycoord __far *, const struct _wxycoord __far *, const struct _wxycoord __far *);

/*	returns nonzero if successful; otherwise 0	*/
short __far __cdecl _rectangle_w(short, double, double, double, double);
short __far __cdecl _rectangle_wxy(short, const struct _wxycoord __far *, const struct _wxycoord __far *);

/*	returns nonzero if successful; otherwise 0	*/
short __far __cdecl _polygon_w(short, const double __far *, short);
short __far __cdecl _polygon_wxy(short, const struct _wxycoord __far *, short);

/*	returns previous color; -1 if unsuccessful */
short __far __cdecl _setpixel_w(double, double);


/* window coordinate image routines */

/*	no return value */
void __far __cdecl _getimage_w(double, double, double, double, char __huge *);
void __far __cdecl _getimage_wxy(const struct _wxycoord __far *, const struct _wxycoord __far *, char __huge *);

/*	returns the image's storage size in bytes */
long __far __cdecl _imagesize_w(double, double, double, double);
long __far __cdecl _imagesize_wxy(const struct _wxycoord __far *, const struct _wxycoord __far *);

/*	no return value */
void __far __cdecl _putimage_w(double, double ,char __huge * ,short);


/* FONTS */

#ifndef _FONTINFO_DEFINED
/* structure for _getfontinfo() */
struct _fontinfo {
	int	type;		/* b0 set = vector,clear = bit map	*/
	int	ascent;		/* pix dist from top to baseline	*/
	int	pixwidth;	/* character width in pixels, 0=prop	*/
	int	pixheight;	/* character height in pixels		*/
	int	avgwidth;	/* average character width in pixels	*/
	char	filename[81];	/* file name including path		*/
	char	facename[32];	/* font name				*/
};
#define _FONTINFO_DEFINED
#endif


/* font function prototypes */
short	__far __cdecl	_registerfonts( const char __far *);
void	__far __cdecl	_unregisterfonts( void );
short	__far __cdecl	_setfont( const char __far * );
short	__far __cdecl	_getfontinfo( struct _fontinfo __far * );
void	__far __cdecl	_outgtext( const char __far * );
short	__far __cdecl	_getgtextextent( const char __far * );
struct _xycoord __far __cdecl _setgtextvector( short, short );
struct _xycoord __far __cdecl _getgtextvector(void);


#ifdef _WINDOWS
/* QuickWin graphics extension prototypes */
int __far __cdecl _wgclose( int );
int __far __cdecl _wggetactive( void );
int __far __cdecl _wgopen( char __far * );
int __far __cdecl _wgsetactive( int );
#endif


/* restore default packing */
#pragma pack()

#ifdef __cplusplus
}
#endif
