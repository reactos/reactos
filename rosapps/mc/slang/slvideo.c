/* Copyright (c) 1992, 1995 John E. Davis
 * All rights reserved.
 * 
 * You may distribute under the terms of either the GNU General Public
 * License or the Perl Artistic License.
 */

#include "config.h"

#include <stdio.h>
#include <string.h>

#include <stdlib.h>

#ifdef __WIN32__
# include <windows.h>
#endif

#ifdef __GO32__
# undef msdos
#endif

#if defined (msdos)
# include <conio.h>
# include <bios.h>
# include <mem.h>
#endif
#if defined (__WATCOMC__)
# include <graph.h>
# define int86	int386		/* simplify code writing */
#endif

#if defined (__GO32__)
# include <pc.h>
# define GO32_VIDEO
#endif

#if defined (__os2__) && !defined (EMX_VIDEO)
# define INCL_BASE
# define INCL_NOPM
# define INCL_VIO
# define INCL_KBD
# include <os2.h>
#else
# if defined (__EMX__)		/* EMX video does both DOS & OS/2 */
#  ifndef EMX_VIDEO
#   define EMX_VIDEO
#  endif
#  include <sys/video.h>
# endif
#endif

#include <dos.h>
#include "slang.h"

#ifdef GO32_VIDEO
# define HAS_SAVE_SCREEN
#endif

/* ------------------------- global variables ------------------------- */

#ifdef WIN32
extern HANDLE hStdout, hStdin;
extern CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
#endif


int SLtt_Term_Cannot_Insert;
int SLtt_Term_Cannot_Scroll;
int SLtt_Ignore_Beep = 3;
int SLtt_Use_Ansi_Colors;

int SLtt_Screen_Rows = 25;
int SLtt_Screen_Cols = 80;

/* ------------------------- local variables -------------------------- */
static int Attribute_Byte;
static int Scroll_r1 = 0, Scroll_r2 = 25;
static int Cursor_Row = 1, Cursor_Col = 1;
static int Current_Color;
static int IsColor = 1;
static int Blink_Killed;	/* high intensity background enabled */

#define JMAX_COLORS	256
#define JNORMAL_COLOR	0
#define JNO_COLOR	-1

static unsigned char Color_Map [JMAX_COLORS] = 
{
   0x7, 0x70, 0x70, 0x70, 0x70, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7,
   0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7
};


#define JMAX_COLOR_NAMES 16
static char *Color_Names [JMAX_COLOR_NAMES] =
{
   "black", "blue", "green", "cyan",
     "red", "magenta", "brown", "lightgray",
     "gray", "brightblue", "brightgreen", "brightcyan",
     "brightred", "brightmagenta", "yellow", "white"
};

/*
 * set_color_from_attribute (int attribute);
 * define the correspondence of color to attribute
 */
#define set_color_from_attribute(a)\
	SLtt_set_color (\
			JNORMAL_COLOR, NULL,\
			Color_Names[(a) & 0xf],\
			Color_Names[(a) >> 4])
/* this is how to make a space character */
#define mkSpaceChar()	(((Attribute_Byte) << 8) | 0x20)

/* buffer to hold a line of character/attribute pairs */
#define MAXCOLS 256
static unsigned char Line_Buffer [MAXCOLS*2];

/*----------------------------------------------------------------------*\
 * define various ways and means of writing to the screen
\*----------------------------------------------------------------------*/
#if defined (__GO32__) || defined (__WATCOMC__)
# if !defined (GO32_VIDEO)
#  define HAS_LINEAR_SCREEN
# endif
#else	/* __GO32__ or __WATCOMC__ */
# if defined (msdos)
#  define USE_ASM
# endif
#endif	/* __GO32__ or __WATCOMC__ */

/* define for direct to memory screen writes */
#if defined (USE_ASM) || defined (HAS_LINEAR_SCREEN)
static unsigned char *Video_Base;
# define mkScreenPointer(row,col)	((unsigned short *)\
					 (Video_Base +\
					  2 * (SLtt_Screen_Cols * (row)\
					       + (col))))
# if defined (USE_ASM)
int SLtt_Msdos_Cheap_Video = 0;
static int Video_Status_Port;

#  define MONO_STATUS	0x3BA
#  define CGA_STATUS	0x3DA
#  define CGA_SETMODE	0x3D8

#  define SNOW_CHECK \
if (SLtt_Msdos_Cheap_Video)\
{ while ((inp (CGA_STATUS) & 0x08)); while (!(inp (CGA_STATUS) & 0x08)); }
# endif	/* USE_ASM */
#endif	/* USE_ASM or HAS_LINEAR_SCREEN */


/* -------------------------------------------------------------------- */
#if defined (__WATCOMC__)
# define ScreenPrimary	(0xb800 << 4)
# define ScreenSize	(SLtt_Screen_Cols * SLtt_Screen_Rows)
# define ScreenSetCursor (x,y) _settextposition (x+1,y+1)
void ScreenGetCursor (int *x, int *y)
{
   struct rccoord rc = _gettextposition ();
   *x = rc.row - 1;
   *y = rc.col - 1;
}
void ScreenRetrieve (unsigned char *dest)
{
   memcpy (dest, (unsigned char *) ScreenPrimary, 2 * ScreenSize);
}
void ScreenUpdate (unsigned char *src)
{
   memcpy ((unsigned char *) ScreenPrimary, src, 2 * ScreenSize);
}
#endif	/* __WATCOMC__ */

#ifdef HAS_SAVE_SCREEN
static void *Saved_Screen_Buffer;
static int Saved_Cursor_Row;

static void save_screen (void)
{
   int row, col;
   
   if (Saved_Screen_Buffer != NULL)
     {
	SLFREE (Saved_Screen_Buffer);
	Saved_Screen_Buffer = NULL;
     }
#ifdef GO32_VIDEO   
   Saved_Screen_Buffer = SLMALLOC (sizeof (short) * 
				   ScreenCols () * ScreenRows ());

   if (Saved_Screen_Buffer == NULL)
     return;
   
   ScreenRetrieve (Saved_Screen_Buffer);
   ScreenGetCursor (&row, &col);
   Saved_Cursor_Row = row;
#endif
   
}

static void restore_screen (void)
{
   if (Saved_Screen_Buffer == NULL) return;
#ifdef GO32_VIDEO
   ScreenUpdate (Saved_Screen_Buffer);
   SLtt_goto_rc (Saved_Cursor_Row, 0);
#endif
   
}
#endif				       /* HAS_SAVE_SCREEN */
/*----------------------------------------------------------------------*\
 * Function:	void SLtt_write_string (char *str);
 *
 * put string STR to 'stdout'
\*----------------------------------------------------------------------*/
void SLtt_write_string (char *str)
{
#ifdef WIN32
   int bytes;
   
   (void) WriteConsole(hStdout, str, strlen(str), &bytes, NULL);
#else
   fputs (str, stdout);
#endif
}

/*----------------------------------------------------------------------*\
 * Function:	void SLtt_set_scroll_region (int r1, int r2);
 *
 * define a scroll region of top_row to bottom_row
\*----------------------------------------------------------------------*/
void SLtt_set_scroll_region (int top_row, int bottom_row)
{
   Scroll_r1 = top_row;
   Scroll_r2 = bottom_row;
}

/*----------------------------------------------------------------------*\
 * Function:	void SLtt_reset_scroll_region (void);
 *
 * reset the scrol region to be the entire screen,
 * ie, SLtt_set_scroll_region (0, SLtt_Screen_Rows);
\*----------------------------------------------------------------------*/
void SLtt_reset_scroll_region (void)
{
   Scroll_r1 = 0;
   Scroll_r2 = SLtt_Screen_Rows;
}

/*----------------------------------------------------------------------*\
 * Function:	void SLtt_goto_rc (int row, int col);
 *
 * move the terminal cursor to x,y position COL, ROW and record the
 * position in Cursor_Row, Cursor_Col
\*----------------------------------------------------------------------*/
void SLtt_goto_rc (int row, int col)
{
#ifdef WIN32
   COORD newPosition;
   newPosition.X = col;
   newPosition.Y = row;
#endif
   
#if !defined (USE_ASM)
   if (row > SLtt_Screen_Rows) row = SLtt_Screen_Rows;
   if (col > SLtt_Screen_Cols) col = SLtt_Screen_Cols;
# if defined (EMX_VIDEO)
   v_gotoxy (col, Scroll_r1 + row);
# else /* EMX_VIDEO_ */
#  if defined (__os2__)
   VioSetCurPos (Scroll_r1 + row, col, 0);
#  elif defined(WIN32)
   (void) SetConsoleCursorPosition(hStdout, newPosition);
#  else /* __os2__ */
#   if defined (__GO32__) || defined (__WATCOMC__)
   ScreenSetCursor(Scroll_r1 + row, col);
#   endif	/* __GO32__ or __WATCOMC__ */
#  endif /* __os2__ */
# endif /* EMX_VIDEO_ */
   Cursor_Row = row;
   Cursor_Col = col;
#else	/* USE_ASM */
   /* if (r > SLtt_Screen_Rows - 1) r = SLtt_Screen_Rows - 1; */
   asm  mov ax, row
     asm  mov bx, SLtt_Screen_Rows
     asm  dec bx
     asm  cmp ax, bx
     asm  jle L1
     asm  mov ax, bx
     L1:
   /* if (c > SLtt_Screen_Cols - 1) c = SLtt_Screen_Cols - 1; */
   asm  mov cx, SLtt_Screen_Cols
     asm  dec cx
     asm  mov bx, col
     asm  cmp bx, cx
     asm  jle L2
     asm  mov bx, cx
     L2:
   asm  mov Cursor_Row, ax
     asm  mov Cursor_Col, bx
     asm  add ax, Scroll_r1
     asm  xor dx, dx
     asm  mov dh, al
     asm  mov dl, bl
     asm  xor bx, bx
     asm  mov ax, 0x200
     asm  int 0x10
#endif	/* USE_ASM */
}

/*----------------------------------------------------------------------*\
 * Function:	static void slvid_getxy (void);
 *
 * retrieve the cursor position into Cursor_Row, Cursor_Col
\*----------------------------------------------------------------------*/
static void slvid_getxy (void)
{
#if !defined (USE_ASM)
# if defined (EMX_VIDEO)
   v_getxy (&Cursor_Col, &Cursor_Row);
# else	/* EMX_VIDEO */
#  if defined (__os2__)
   VioGetCurPos ((USHORT*) &Cursor_Row, (USHORT*) &Cursor_Col, 0);
#  elif defined(WIN32)
   CONSOLE_SCREEN_BUFFER_INFO screenInfo;
   if (GetConsoleScreenBufferInfo(hStdout, &screenInfo) == TRUE)
     {
	Cursor_Row = screenInfo.dwCursorPosition.Y;
	Cursor_Col = screenInfo.dwCursorPosition.X;
     }
#  else	/* __os2__ */
#   if defined (__GO32__) || defined (__WATCOMC__)
   ScreenGetCursor (&Cursor_Row, &Cursor_Col);
#   endif	/* __GO32__ or __WATCOMC__ */
#  endif	/* __os2__ */
# endif	/* EMX_VIDEO */
#else	/* USE_ASM */
   asm  mov ah, 3
     asm  mov bh, 0
     asm  int 10h
     asm  xor ax, ax
     asm  mov al, dh
     asm  mov Cursor_Row, ax
     asm  xor ax, ax
     asm  mov al, dl
     asm  mov Cursor_Col, ax
#endif	/* USE_ASM */
}

/*----------------------------------------------------------------------*\
 * static void slvid_deleol (int x);
 *
 * write space characters from column X of row Cursor_Row through to
 * SLtt_Screen_Cols using the current Attribute_Byte
\*----------------------------------------------------------------------*/
#if defined (GO32_VIDEO)
static void slvid_deleol (int x)
{
   while (x < SLtt_Screen_Cols)
     ScreenPutChar (32, Attribute_Byte, x++, Cursor_Row);
}
#endif
#if defined (EMX_VIDEO)
static void slvid_deleol (int x)
{
   unsigned char *p, *pmax;
   int w = mkSpaceChar ();
   int count = SLtt_Screen_Cols - x;
   
   p = Line_Buffer;
   pmax = p + 2 * count;
   
   while (p < pmax)
     {
	*p++ = (unsigned char) w;
	*p++ = (unsigned char) (w >> 8);
     }

   v_putline (Line_Buffer, x, Cursor_Row, count);
}
#endif	/* EMX_VIDEO */

/*----------------------------------------------------------------------*\
 * Function:	void SLtt_begin_insert (void);
 *
 * insert a single space, moving everything right 1 character to make room
\*----------------------------------------------------------------------*/
void SLtt_begin_insert (void)
{
#if !defined (GO32_VIDEO)
# if defined (HAS_LINEAR_SCREEN) || defined (USE_ASM)
   unsigned short *p;
#  if defined (HAS_LINEAR_SCREEN)
   unsigned short *pmin;
#  endif
# endif
   int n;
   slvid_getxy ();
   n = SLtt_Screen_Cols - Cursor_Col;
   /* Msdos_Insert_Mode = 1; */
   
# ifndef WIN32
#  if defined (EMX_VIDEO)
   v_getline (Line_Buffer, Cursor_Col, Cursor_Row, n);
   v_putline (Line_Buffer, Cursor_Col+1, Cursor_Row, n - 1);
#  else	/* EMX_VIDEO */
#   if defined (__os2__)
   n = 2 * (n - 1);
   VioReadCellStr ((PCH)Line_Buffer, (USHORT*) &n, Cursor_Row, Cursor_Col, 0);
   VioWrtCellStr ((PCH)Line_Buffer, n, Cursor_Row, Cursor_Col + 1, 0);
#   else	/* __os2__ */
   p = mkScreenPointer (Cursor_Row, SLtt_Screen_Cols - 1);
   
#    if defined (HAS_LINEAR_SCREEN)
   /* pmin = p - (n-1); */
   pmin = mkScreenPointer (Cursor_Row, Cursor_Col);
   while (p-- > pmin) *(p + 1) = *p;
#    else
   SNOW_CHECK;
   asm  mov ax, ds
     asm  mov bx, di
     asm  mov dx, si
     
     asm  mov cx, n
     asm  les di, p
     asm  lds si, p
     asm  sub si, 2
     asm  std
     asm  rep movsw
     
     asm  mov ds, ax
     asm  mov di, bx
     asm  mov si, dx
#    endif	/* HAS_LINEAR_SCREEN */
#   endif  /* __os2__ */
#  endif	/* EMX_VIDEO */
     
# endif  /* WIN32 */
     
#endif	/* not GO32_VIDEO */
}

/*----------------------------------------------------------------------*\
 * Function:	void SLtt_end_insert (void);
 *
 * any cleanup after insert a blank column
\*----------------------------------------------------------------------*/
void SLtt_end_insert (void)
{
}

/*----------------------------------------------------------------------*\
 * Function:	void SLtt_delete_char (void);
 *
 * delete a single character, moving everything left 1 column to take
 * up the room
\*----------------------------------------------------------------------*/
void SLtt_delete_char (void)
{
#if !defined (GO32_VIDEO)
# if defined (HAS_LINEAR_SCREEN) || defined (USE_ASM)
   unsigned short *p;
#  if defined (HAS_LINEAR_SCREEN)
   register unsigned short *p1;
#  endif
# endif
   int n;
   
   slvid_getxy ();
   n = SLtt_Screen_Cols - Cursor_Col - 1;
   
# ifndef WIN32
   
#  if defined (EMX_VIDEO)
   v_getline (Line_Buffer, Cursor_Col+1, Cursor_Row, n);
   v_putline (Line_Buffer, Cursor_Col, Cursor_Row, n);
#  else	/* EMX_VIDEO */
#   if defined (__os2__)
   n *= 2;
   VioReadCellStr ((PCH)Line_Buffer, (USHORT*)&n, Cursor_Row, Cursor_Col + 1, 0);
   VioWrtCellStr ((PCH)Line_Buffer, n, Cursor_Row, Cursor_Col, 0);
   return;
#   else	/* __os2__ */
   p = mkScreenPointer (Cursor_Row, Cursor_Col);
   
#    if defined (HAS_LINEAR_SCREEN)
   while (n--)
     {
	p1 = p + 1;
	*p = *p1;
	p++;
     }
#    else	/* HAS_LINEAR_SCREEN */
   SNOW_CHECK;
   asm  mov ax, ds
     asm  mov bx, si
     asm  mov dx, di
     
     asm  mov cx, n
     asm  les di, p
     asm  lds si, p
     asm  add si, 2
     asm  cld
     asm  rep movsw
     
     asm  mov ds, ax
     asm  mov si, bx
     asm  mov di, dx
#    endif	/* HAS_LINEAR_SCREEN */
#   endif /* __os2__ */
#  endif /* EMX_VIDEO */
     
# endif /* WIN32 */
     
#endif /* not GO32_VIDEO */
}

/*----------------------------------------------------------------------*\
 * Function:	void SLtt_erase_line (void);
 *
 * This function is *only* called on exit.
 * It sets attribute byte to Black & White
\*----------------------------------------------------------------------*/
void SLtt_erase_line (void)
{
   
#ifndef WIN32
   
# if defined (GO32_VIDEO) || defined (EMX_VIDEO)
   Attribute_Byte = 0x07;
   slvid_deleol (0);
# else	/* GO32_VIDEO or EMX_VIDEO */
#  if defined (__os2__)
   USHORT w;
   Attribute_Byte = 0x07;
   w = mkSpaceChar ();
   VioWrtNCell ((BYTE*)&w, SLtt_Screen_Cols, Cursor_Row, 0, 0);
#  else	/* __os2__ */
   unsigned short w;
   unsigned short *p = mkScreenPointer (Cursor_Row, 0);
#   if defined (HAS_LINEAR_SCREEN)
   register unsigned short *pmax = p + SLtt_Screen_Cols;
   
   Attribute_Byte = 0x07;
   w = mkSpaceChar ();
   while (p < pmax) *p++ = w;
#   else	/* HAS_LINEAR_SCREEN */
   Attribute_Byte = 0x07;
   w = mkSpaceChar ();
   SNOW_CHECK;
   asm  mov dx, di
     asm  mov ax, w
     asm  mov cx, SLtt_Screen_Cols
     asm  les di, p
     asm  cld
     asm  rep stosw
     asm  mov di, dx
#   endif  /* HAS_LINEAR_SCREEN */
#  endif  /* __os2__ */
# endif  /* GO32_VIDEO or EMX_VIDEO */
     Current_Color = JNO_COLOR;		/* since we messed with attribute byte */
   
#endif  /* WIN32 */
   
}

/*----------------------------------------------------------------------*\
 * Function:	void SLtt_delete_nlines (int nlines);
 *
 * delete NLINES by scrolling up the region <Scroll_r1, Scroll_r2>
\*----------------------------------------------------------------------*/
void SLtt_delete_nlines (int nlines)
{
   SLtt_normal_video ();
   
#ifndef WIN32
   
# if defined (EMX_VIDEO)
   v_attrib (Attribute_Byte);
   v_scroll (0, Scroll_r1, SLtt_Screen_Cols-1, Scroll_r2, nlines, V_SCROLL_UP);
# else	/* EMX_VIDEO */
#  if defined (__os2__)
     {
	Line_Buffer[0] = ' '; Line_Buffer[1] = Attribute_Byte;
	VioScrollUp (Scroll_r1, 0, Scroll_r2, SLtt_Screen_Cols-1,
		     nlines, (PCH) Line_Buffer, 0);
     }
#  else	/* __os2__ */
#   if defined (USE_ASM)
   /* This has the effect of pulling all lines below it up */
   asm  mov ax, nlines
     asm  mov ah, 6		/* int 6h */
     asm  xor cx, cx
     asm  mov ch, byte ptr Scroll_r1
     asm  mov dx, SLtt_Screen_Cols
     asm  dec dx
     asm  mov dh, byte ptr Scroll_r2
     asm  mov bh, byte ptr Attribute_Byte
     asm  int 10h
#   else	/* USE_ASM */
     {
	union REGS r;
#    if defined (__WATCOMC__)
	r.x.eax = nlines;
	r.x.ecx = 0;
#    else
	r.x.ax = nlines;
	r.x.cx = 0;
#    endif
	r.h.ah = 6;
	r.h.ch = Scroll_r1;
	r.h.dl = SLtt_Screen_Cols - 1;
	r.h.dh = Scroll_r2;
	r.h.bh = Attribute_Byte;
	int86 (0x10, &r, &r);
     }
#   endif	/* USE_ASM */
#  endif /* __os2__ */
# endif	/* EMX_VIDEO */
   
#endif  /* WIN32 */
   
}

/*----------------------------------------------------------------------*\
 * Function:	void SLtt_reverse_index (int nlines);
 *
 * scroll down the region <Scroll_r1, Scroll_r2> by NLINES
\*----------------------------------------------------------------------*/
void SLtt_reverse_index (int nlines)
{
   SLtt_normal_video ();
   
#ifndef WIN32
   
# if defined (EMX_VIDEO)
   v_attrib (Attribute_Byte);
   v_scroll (0, Scroll_r1, SLtt_Screen_Cols-1, Scroll_r2, nlines,
	     V_SCROLL_DOWN);
# else	/* EMX_VIDEO */
#  if defined (__os2__)
     {
	Line_Buffer[0] = ' '; Line_Buffer[1] = Attribute_Byte;
	VioScrollDn (Scroll_r1, 0, Scroll_r2, SLtt_Screen_Cols-1,
		     nlines, (PCH) Line_Buffer, 0);
     }
#  else	/* __os2__ */
#   if defined (USE_ASM)
   asm  xor cx, cx
     asm  mov ch, byte ptr Scroll_r1
     asm  mov dx, SLtt_Screen_Cols
     asm  dec dx
     asm  mov dh, byte ptr Scroll_r2
     asm  mov bh, byte ptr Attribute_Byte
     asm  mov ah, 7
     asm  mov al, byte ptr nlines
     asm  int 10h
#   else	/* USE_ASM */
     {
	union REGS r;
	r.h.al = nlines;
#    if defined (__WATCOMC__)
	r.x.ecx = 0;
#    else
	r.x.cx = 0;
#    endif
	r.h.ah = 7;
	r.h.ch = Scroll_r1;
	r.h.dl = SLtt_Screen_Cols - 1;
	r.h.dh = Scroll_r2;
	r.h.bh = Attribute_Byte;
	int86 (0x10, &r, &r);
     }
#   endif	/* USE_ASM */
#  endif	/* __os2__ */
# endif  /* EMX_VIDEO */
   
#endif  /* WIN32 */
   
}

/*----------------------------------------------------------------------*\
 * Function:	static void slvid_invert_region (int top_row, int bot_row);
 *
 * invert the display in the region,  top_row <= row < bot_row
\*----------------------------------------------------------------------*/
static void slvid_invert_region (int top_row, int bot_row)
{
   
#ifndef WIN32
   
# if defined (EMX_VIDEO)
   int row, col;
   
   for (row = top_row; row < bot_row; row++)
     {
	v_getline (Line_Buffer, 0, row, SLtt_Screen_Cols);
	for (col = 1; col < SLtt_Screen_Cols * 2; col += 2)
	  Line_Buffer [col] ^= 0xff;
	v_putline (Line_Buffer, 0, row, SLtt_Screen_Cols);
     }
# else	/* EMX_VIDEO */
#  ifdef __os2__
   int row, col;
   USHORT length = SLtt_Screen_Cols * 2;
   
   for (row = top_row; row < bot_row; row++)
     {
	VioReadCellStr ((PCH)Line_Buffer, &length, row, 0, 0);
	for (col = 1; col < length; col += 2)
	  Line_Buffer [col] ^= 0xff;
	VioWrtCellStr ((PCH)Line_Buffer, length, row, 0, 0);
     }
#  else	/* __os2__ */
#   if defined (__GO32__) || defined (__WATCOMC__)
   unsigned char buf [2 * 180 * 80];         /* 180 cols x 80 rows */
   unsigned char *b, *bmax;
   
   b    = buf + 1 + 2 * SLtt_Screen_Cols * top_row;
   bmax = buf + 1 + 2 * SLtt_Screen_Cols * bot_row;
   ScreenRetrieve (buf);
   while (b < bmax)
     {
	*b ^= 0xFF;
	b += 2;
     }
   ScreenUpdate (buf);
#   else	/* __GO32__ or __WATCOMC__ */
   register unsigned short ch, sh;
   register unsigned short *pmin = mkScreenPointer (top_row, 0);
   register unsigned short *pmax = mkScreenPointer (bot_row, 0);
   
   while (pmin < pmax)
     {
	sh = *pmin;
	ch = sh;
	ch = ch ^ 0xFF00;
	*pmin = (ch & 0xFF00) | (sh & 0x00FF);
	pmin++;
     }
#   endif	/* __GO32__ or __WATCOMC__ */
#  endif	/* __os2__ */
# endif	/* EMX_VIDEO */
   
#endif  /* WIN32 */
   
}

/*----------------------------------------------------------------------*\
 * Function:	void SLtt_beep (void);
 *
 * signal error by a "bell" condition, the type of signal is governed
 * by the value of SLtt_Ignore_Beep:
 *
 * 0	silent bell
 * 1	audible bell
 * 2	visual bell
 * 4	special visual bell (only flash the bottom status line)
 *
 * these may be combined:
 * 	eg, 3 = audible visual bell.
 * but if both the visual bell and the "special" visual bell are specified,
 * only the special bell is used.
\*----------------------------------------------------------------------*/
void SLtt_beep (void)
{
   int audible;			/* audible bell */
   int special = 0;		/* first row to invert */
   int visual = 0;		/* final row to invert */
   if (!SLtt_Ignore_Beep) return;
   
   audible = (SLtt_Ignore_Beep & 1);
   if ( (SLtt_Ignore_Beep & 4) )
     {
	special = SLtt_Screen_Rows - 1;
	visual = special--;	/* only invert bottom status line */
     }
   else if ( (SLtt_Ignore_Beep & 2) )
     {
	visual = SLtt_Screen_Rows;
     }
   
   if (visual) slvid_invert_region (special, visual);
#if defined (EMX_VIDEO)
   if (audible) /*sound (1500)*/; _sleep2 (100); if (audible) /* nosound () */;
#else
# ifdef __os2__
   if (audible) DosBeep (1500, 100); else DosSleep (100);
   
# elif defined(WIN32)
   
# else
   if (audible) sound (1500); delay (100); if (audible) nosound ();
# endif
#endif
   if (visual) slvid_invert_region (special, visual);
}

/*----------------------------------------------------------------------*\
 * Function:	void SLtt_del_eol (void);
 *
 * delete from the current cursor position to the end of the row
\*----------------------------------------------------------------------*/
void SLtt_del_eol (void)
{
   
#ifndef WIN32
   
# if defined (GO32_VIDEO) || defined (EMX_VIDEO)
   if (Current_Color != JNO_COLOR) SLtt_normal_video ();
   slvid_deleol (Cursor_Col);
# else	/* GO32_VIDEO or EMX_VIDEO */
#  ifdef __os2__
   USHORT w;
   if (Current_Color != JNO_COLOR) SLtt_normal_video ();
   w = mkSpaceChar ();
   VioWrtNCell ((BYTE*)&w, (SLtt_Screen_Cols - Cursor_Col),
		Cursor_Row, Cursor_Col, 0);
#  else	/* __os2__ */
   unsigned short *p = mkScreenPointer (Cursor_Row, Cursor_Col);
   int n = SLtt_Screen_Cols - Cursor_Col;
   unsigned short w;
#   if defined (HAS_LINEAR_SCREEN)
   unsigned short *pmax = p + n;
   
   if (Current_Color != JNO_COLOR) SLtt_normal_video ();
   w = mkSpaceChar ();
   while (p < pmax) *p++ = w;
#   else	/* HAS_LINEAR_SCREEN */
   if (Current_Color != JNO_COLOR) SLtt_normal_video ();
   w = mkSpaceChar ();
   SNOW_CHECK;
   asm  mov dx, di
     asm  les di, p
     asm  mov ax, w
     asm  mov cx, n
     asm  cld
     asm  rep stosw
     
     asm  mov di, dx
#   endif	/* HAS_LINEAR_SCREEN */
#  endif	/* __os2__ */
# endif	/* GO32_VIDEO or EMX_VIDEO */
     
#endif  /* WIN32 */
     
}

/*----------------------------------------------------------------------*\
 * Function:	void SLtt_reverse_video (int color);
 *
 * set Attribute_Byte corresponding to COLOR.
 * Use Current_Color to remember the color which was set.
 * convert from the COLOR number to the attribute value.
\*----------------------------------------------------------------------*/
void SLtt_reverse_video (int color)
{
   Attribute_Byte = Color_Map [color];
   Current_Color = color;
}

/*----------------------------------------------------------------------*\
 * Function:	void SLtt_normal_video (void);
 *
 * reset the attributes for normal video
\*----------------------------------------------------------------------*/
void SLtt_normal_video (void)
{
   SLtt_reverse_video (JNORMAL_COLOR);
}

#if defined (USE_ASM)
/*----------------------------------------------------------------------*\
 * Function:	static unsigned short *video_write (register unsigned char *pp,
 *						    register unsigned char *p,
 *						    register unsigned short *pos)
 *
 * write out (P - PP) characters from the array pointed to by PP
 * at position (POS, Cursor_Row) in the current Attribute_Byte
 *
 * increment POS to reflect the number of characters sent and
 * return the it as a pointer
\*----------------------------------------------------------------------*/
static unsigned short *video_write (register unsigned char *pp,
				    register unsigned char *p,
				    register unsigned short *pos)
{
   int n = (int) (p - pp);	/* num of characters of PP to write */
   
   asm  push si
     asm  push ds
     asm  push di
     
   /* set up register for BOTH fast and slow */
     asm  mov bx, SLtt_Msdos_Cheap_Video
     
   /* These are the registers needed for both fast AND slow */
     asm  mov ah, byte ptr Attribute_Byte
     asm  mov cx, n
     asm  lds si, dword ptr pp
     asm  les di, dword ptr pos
     asm  cld
     
     asm  cmp bx, 0		       /* cheap video test */
     asm  je L_fast
     asm  mov bx, ax
     asm  mov dx, CGA_STATUS
     asm  jg L_slow_blank
     
   /* slow video */
     asm  cli
     
   /* wait for retrace */
     L_slow:
   asm  in al, dx
     asm  test al, 1
     asm  jnz L_slow
     
     L_slow1:
   asm  in al, dx
     asm  test al, 1
     asm  jz L_slow1
     
   /* move a character out */
     asm  mov ah, bh
     asm  lodsb
     asm  stosw
     asm  loop L_slow
     
     asm  sti
     asm  jmp done
     
/* -------------- slow video, vertical retace and pump --------------*/
     L_slow_blank:
   L_slow_blank_loop:
   asm  in al, dx
     asm  test al, 8
     asm  jnz L_slow_blank_loop
     
     L_slow_blank1:
   asm  in al, dx
     asm  test al, 8
     asm  jz L_slow_blank1
   /* write line */
     asm  mov ah, bh
     L_slow_blank2:
   asm  lodsb
     asm  stosw
     asm  loop L_slow_blank2
     
     asm jmp done
/*-------------- Fast video --------------*/
     
     L_fast:
   asm  lodsb
     asm  stosw
     asm  loop L_fast
     done:
   asm  pop di
     asm  pop ds
     asm  pop si
     return (pos + n);
}
#endif  /* USE_ASM */

/*----------------------------------------------------------------------*\
 * Function:	static void write_attributes (unsigned short *src,
 *					      int count);
 *
 * Copy COUNT character/color pairs from the array pointed to by
 * SRC to the screen at position (0,Cursor_Row).
 * NB: SRC contains character/color pairs -- the color must be converted to
 * an ansi attribute.
 *
 * Write out
 *	1) a combination of string/attributes
 *	2) each string of continuous colour
 *
 * approach 2) is used for assembler output, while 1) is used when a higher
 * level API is available or direct to memory writing is possible: emx video
 * routines, os/2, go32, watcom.
\*----------------------------------------------------------------------*/
static void write_attributes (unsigned short *src, int count)
{
   register unsigned char *p = Line_Buffer;
   register unsigned short pair;
#ifdef WIN32
   register unsigned char * org_src = src;
   COORD coord;
   long bytes;
#endif
#if !defined (USE_ASM)
# if defined (HAS_LINEAR_SCREEN)
   register unsigned short *pos = mkScreenPointer (Cursor_Row, 0);
# endif
   int n = count;
   
   /* write into a character/attribute pair */
   while (n-- > 0)
     {
	pair   = *(src++);		/* character/color pair */
	SLtt_reverse_video (pair >> 8);	/* color change */
# if defined (HAS_LINEAR_SCREEN)
	*(pos++) = ((unsigned short) Attribute_Byte << 8) | pair & 0xff;
# else
#  if defined(EMX_VIDEO) || !defined(WIN32)
	*(p++) = pair & 0xff;		/* character byte */
	*(p++) = Attribute_Byte;	/* attribute byte */
#  else
	/* WIN32 for now... */
	*(p++) = pair & 0xff;
#  endif
# endif
     }
   
# if !defined (HAS_LINEAR_SCREEN)
#  if defined (EMX_VIDEO)
   v_putline (Line_Buffer, Cursor_Col, Cursor_Row, count);
#  else	/* EMX_VIDEO */
#   if defined (__os2__)
   VioWrtCellStr ((PCH)Line_Buffer, (USHORT)(2 * count),
		  (USHORT)Cursor_Row, (USHORT)Cursor_Col, 0);
#   elif defined(WIN32)
  /* do color attributes later */
   p = Line_Buffer;
   coord.X = Cursor_Col;
   coord.Y = Cursor_Row;
   WriteConsoleOutputCharacter(hStdout, p, count, coord, &bytes);
   
  /* write color attributes */
   p = Line_Buffer;
   n = count;
   src = org_src; /* restart the src pointer */
   
  /* write into attributes only */
   while (n-- > 0)
     {
	pair   = *(src++);		/* character/color pair */
	SLtt_reverse_video (pair >> 8);	/* color change */
	*(p++) = Attribute_Byte; /* attribute byte */
	*(p++) = 0; /* what's this for? */
     }
   
   WriteConsoleOutputAttribute(hStdout, Line_Buffer, count, coord, &bytes);
#   else	/* __os2__ */
   /* ScreenUpdateLine (void *virtual_screen_line, int row); */
   p = Line_Buffer;
   n = Cursor_Col;
   while (count-- > 0)
     {
	ScreenPutChar ((int)p[0], (int)p[1], n++, Cursor_Row);
	p += 2;
     }
#   endif	/* EMX_VIDEO */
#  endif	/* __os2__ */
# endif	/* HAS_LINEAR_SCREEN */
#else	/* not USE_ASM */
   unsigned char ch, color;
   register unsigned short *pos = mkScreenPointer (Cursor_Row, 0);
   
   while (count--)
     {
	pair = *(src++);	/* character/color pair */
	ch = pair & 0xff;	/* character value */
	color = pair >> 8;	/* color value */
	if (color != Current_Color)	/* need a new color */
	  {
	     if (p != Line_Buffer)
	       {
		  pos = video_write (Line_Buffer, p, pos);
		  p = Line_Buffer;
	       }
	     SLtt_reverse_video (color);	/* change color */
	  }
	*(p++) = ch;
     }
   pos = video_write (Line_Buffer, p, pos);
#endif	/* not USE_ASM */
}

/*----------------------------------------------------------------------*\
 * Function:	void SLtt_smart_puts (unsigned short *new_string,
 *				      unsigned short *old_string,
 *				      int len, int row);
 *
 * puts NEW_STRING, which has length LEN, at row ROW.  NEW_STRING contains
 * characters/colors packed in the form value = ((color << 8) | (ch));
 *
 * the puts tries to avoid overwriting the same characters/colors
 *
 * OLD_STRING is not used, maintained for compatibility with other systems
\*----------------------------------------------------------------------*/
void SLtt_smart_puts (unsigned short *new_string,
		      unsigned short *old_string,
		      int len, int row)
{
   (void) old_string;
   Cursor_Row = row;
   Cursor_Col = 0;
   write_attributes (new_string, len);
}

/*----------------------------------------------------------------------*\
 * Function:	void SLtt_reset_video (void);
\*----------------------------------------------------------------------*/
void SLtt_reset_video (void)
{
   SLtt_goto_rc (SLtt_Screen_Rows - 1, 0);
#ifdef HAS_SAVE_SCREEN
   restore_screen ();
#endif
   Attribute_Byte = 0x07;
   Current_Color = JNO_COLOR;
   SLtt_del_eol ();
}

#if 0
void wide_width (void)
{
}

void narrow_width (void)
{
}
#endif

/*----------------------------------------------------------------------*\
 * Function:	void SLtt_cls (void);
\*----------------------------------------------------------------------*/
void SLtt_cls (void)
{
#ifdef WIN32
   long bytes;
   COORD coord;
   char ch;
#endif
   SLtt_normal_video ();
#if defined (__GO32__) || defined (__WATCOMC__) || defined (EMX_VIDEO)
   SLtt_reset_scroll_region ();
   SLtt_goto_rc (0, 0);
   SLtt_delete_nlines (SLtt_Screen_Rows);
#else	/* __GO32__ or __WATCOMC__ or EMX_VIDEO */
# ifdef __os2__
     {
	Line_Buffer [0] = ' '; Line_Buffer [1] = Attribute_Byte;
	VioScrollUp (0, 0, -1, -1, -1, (PCH)Line_Buffer, 0);
     }
# elif defined(WIN32)
     /* clear the WIN32 screen in one shot */
   coord.X = 0;
   coord.Y = 0;
   
   ch = ' ';
   
   (void) FillConsoleOutputCharacter(hStdout,
				     ch,
				     csbiInfo.dwMaximumWindowSize.Y * csbiInfo.dwMaximumWindowSize.X,
				     coord,
				     &bytes);
   
     /* now set screen to the current attribute */
   ch = Attribute_Byte;
   (void) FillConsoleOutputAttribute(hStdout,
				     ch,
				     csbiInfo.dwMaximumWindowSize.Y * csbiInfo.dwMaximumWindowSize.X,
				     coord,
				     &bytes);
# else	/* __os2__ */
   asm  mov dx, SLtt_Screen_Cols
     asm  dec dx
     asm  mov ax, SLtt_Screen_Rows
     asm  dec ax
     asm  mov dh, al
     asm  xor cx, cx
     asm  xor ax, ax
     asm  mov ah, 7
     asm  mov bh, byte ptr Attribute_Byte
     asm  int 10h
# endif	/* __os2__ */
#endif	/* __GO32__ or __WATCOMC__ or EMX_VIDEO */
}

/*----------------------------------------------------------------------*\
 * Function:	void SLtt_putchar (char ch);
 *
 * put CH on the screen in the current position.
 * this function is called assuming that cursor is in correct position
\*----------------------------------------------------------------------*/

void SLtt_putchar (char ch)
{
#if !defined (GO32_VIDEO) && !defined (EMX_VIDEO)
   unsigned short p, *pp;
# if defined(WIN32)
   long bytes;
# endif
#endif
   
   if (Current_Color) SLtt_normal_video ();
   slvid_getxy ();		/* get current position */
   switch (ch)
     {
      case 7:			/* ^G - break */
	SLtt_beep (); break;
      case 8:			/* ^H - backspace */
	SLtt_goto_rc (Cursor_Row, Cursor_Col - 1); break;
      case 13:			/* ^M - carriage return */
	SLtt_goto_rc (Cursor_Row, 0); break;
      default:			/* write character to screen */
#if defined (EMX_VIDEO)
	v_putn (ch, 1);
#else	/* EMX_VIDEO */
# ifdef __os2__
 	VioWrtCharStrAtt (&ch, 1, Cursor_Row, Cursor_Col,
 			  (BYTE*)&Attribute_Byte, 0);
# elif defined(WIN32)
 	WriteConsole(hStdout, &ch, 1, &bytes, NULL);
# else	/* __os2__ */
#  ifdef GO32_VIDEO
	ScreenPutChar ((int) ch, Attribute_Byte, Cursor_Col, Cursor_Row);
#  else	/* GO32_VIDEO */
	pp = mkScreenPointer (Cursor_Row, Cursor_Col);
	p = (Attribute_Byte << 8) | (unsigned char) ch;
	
#   ifdef USE_ASM
	SNOW_CHECK;
#   endif
	*pp = p;
#  endif	/* GO32_VIDEO */
# endif	/* __os2__ */
#endif	/* EMX_VIDEO */
	SLtt_goto_rc (Cursor_Row, Cursor_Col + 1);
     }
}

/*----------------------------------------------------------------------*\
 * Function:	void SLtt_set_color (int obj, char *what, char *fg, char *bg);
 *
 * set foreground and background colors of OBJ to the attributes which
 * correspond to the names FG and BG, respectively.
 *
 * WHAT is the name corresponding to the object OBJ, but is not used in
 * this routine.
\*----------------------------------------------------------------------*/
void SLtt_set_color (int obj, char *what, char *fg, char *bg)
{
   int i, b = -1, f = -1;
#ifdef WIN32
   int newcolor;
#endif
   
   (void) what;
   
   if ( !IsColor || (obj < 0) || (obj >= JMAX_COLORS))
     return;
   
   for (i = 0; i < JMAX_COLOR_NAMES; i++ )
     {
	if (!strcmp (fg, Color_Names [i]))
	  {
	     f = i;
	     break;
	  }
     }
   
   for (i = 0; i < JMAX_COLOR_NAMES; i++)
     {
	if (!strcmp (bg, Color_Names [i]))
	  {
	     if (Blink_Killed) b = i; else b = i & 0x7;
	     break;
	  }
     }
   if ((f == -1) || (b == -1) || (f == b)) return;
#if 1
   Color_Map [obj] = (b << 4) | f;
#else
   
   /*
     0        1       2        3
   "black", "blue", "green", "cyan",
     4        5         6         7
   "red", "magenta", "brown", "lightgray",
      8        9               10            11
   "gray", "brightblue", "brightgreen", "brightcyan",
        12            13            14       15
   "brightred", "brightmagenta", "yellow", "white"
   */
   
   /* these aren't all right yet */
   switch (f) 
     {
      case 0: newcolor = 0; break;
      case 1: newcolor = FOREGROUND_BLUE; break;
      case 2: newcolor = FOREGROUND_GREEN; break;
      case 3: newcolor = FOREGROUND_GREEN | FOREGROUND_BLUE; break;
	
      case 4: newcolor = FOREGROUND_RED; break;
      case 5: newcolor = FOREGROUND_RED | FOREGROUND_BLUE; break;
      case 6: newcolor = FOREGROUND_GREEN | FOREGROUND_RED; break;
      case 7: newcolor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
	
      case 8: newcolor = FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN; break;
      case 9: newcolor = FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
      case 10: newcolor = FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
      case 11: newcolor = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
	
      case 12: newcolor = FOREGROUND_RED | FOREGROUND_INTENSITY; break;
      case 13: newcolor = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
      case 14: newcolor = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
      case 15: newcolor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
     }
   // switch
   
   /*
     0        1       2        3
   "black", "blue", "green", "cyan",
     4        5         6         7
   "red", "magenta", "brown", "lightgray",
      8        9               10            11
   "gray", "brightblue", "brightgreen", "brightcyan",
        12            13            14       15
   "brightred", "brightmagenta", "yellow", "white"
   */
   
   switch (b) 
     {
      case 0: newcolor |= 0; break;
      case 1: newcolor |= BACKGROUND_BLUE; break;
      case 2: newcolor |= BACKGROUND_GREEN; break;
      case 3: newcolor |= BACKGROUND_GREEN | BACKGROUND_BLUE; break;
	
      case 4: newcolor |= BACKGROUND_RED; break;
      case 5: newcolor |= BACKGROUND_RED | BACKGROUND_BLUE; break;
      case 6: newcolor |= BACKGROUND_GREEN | BACKGROUND_RED; break;
      case 7: newcolor |= BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY; break;
	
      case 8: newcolor |= BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE; break;
      case 9: newcolor |= BACKGROUND_BLUE | BACKGROUND_INTENSITY; break;
      case 10: newcolor |= BACKGROUND_GREEN | BACKGROUND_INTENSITY; break;
      case 11: newcolor |= BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY; break;
	
      case 12: newcolor |= BACKGROUND_RED | BACKGROUND_INTENSITY; break;
      case 13: newcolor |= BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_INTENSITY; break;
      case 14: newcolor |= BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY; break;
      case 15: newcolor |= BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY; break;
     }
   // switch
   
   Color_Map [obj] = newcolor;
   
#endif
    /* if we're setting the normal color, and the attribute byte hasn't
     been set yet, set it to the new color */
   if ((obj == 0) && (Attribute_Byte == 0)) 
     SLtt_reverse_video (0);
}

/*----------------------------------------------------------------------*\
 * Function:	void SLtt_get_terminfo (void)
\*----------------------------------------------------------------------*/
void SLtt_get_terminfo (void)
{
#ifdef WIN32
   SLtt_Screen_Rows = csbiInfo.dwMaximumWindowSize.Y;
   SLtt_Screen_Cols = csbiInfo.dwMaximumWindowSize.X;
#endif
#ifdef GO32_VIDEO
   SLtt_Screen_Rows = ScreenRows ();
   SLtt_Screen_Cols = ScreenCols ();
#endif
}

/*----------------------------------------------------------------------*\
 * Function:	void SLtt_init_video (void);
\*----------------------------------------------------------------------*/
void SLtt_init_video (void)
{
#if defined (EMX_VIDEO)
   int OldCol, OldRow;
#endif
   
#ifdef HAS_SAVE_SCREEN
   save_screen ();
#endif

   Cursor_Row = Cursor_Col = 0;
   
#if defined (EMX_VIDEO)
   
   v_init ();
   if ( v_hardware () != V_MONOCHROME ) IsColor = 1; else IsColor = 0;

   v_getxy(&OldCol,&OldRow);

   v_gotoxy (0, 0);
   if (IsColor)
     {
	if (_osmode == OS2_MODE)
	  {
# if 0
	     /* Enable high-intensity background colors */
	     VIOINTENSITY RequestBlock;
	     RequestBlock.cb = sizeof (RequestBlock);
	     RequestBlock.type = 2; RequestBlock.fs = 1;
	     VioSetState (&RequestBlock, 0);	/* nop if !fullscreen */
# endif
	     Blink_Killed = 1;
	  }
	else
	  {
	     Blink_Killed = 1;	/* seems to work */
	  }
     }
   
   if (!Attribute_Byte)
     {
	/* find the attribute currently under the cursor */
	v_getline (Line_Buffer, OldCol, OldRow, 1);
	Attribute_Byte = Line_Buffer[1];
	set_color_from_attribute (Attribute_Byte);
     }
   
   v_attrib (Attribute_Byte);
   /*   SLtt_Term_Cannot_Insert = 1; */
#else	/* EMX_VIDEO */
# ifdef __os2__
   IsColor = 1;			/* is it really? */
     {
	/* Enable high-intensity background colors */
	VIOINTENSITY RequestBlock;
	RequestBlock.cb = sizeof (RequestBlock);
	RequestBlock.type = 2; RequestBlock.fs = 1;
	VioSetState (&RequestBlock, 0);	/* nop if !fullscreen */
	Blink_Killed = 1;
     }
   
   if (!Attribute_Byte)
     {
	/* find the attribute currently under the cursor */
	USHORT Length = 2, Row, Col;
	VioGetCurPos (&Row, &Col, 0);
	VioReadCellStr ((PCH)Line_Buffer, &Length, Row, Col, 0);
	Attribute_Byte = Line_Buffer[1];
	set_color_from_attribute (Attribute_Byte);
     }
# elif defined(WIN32)
   /* initialize the WIN32 console */
   IsColor = 1; /* yes, the WIN32 console can do color (on a color monitor) */
# else
#  if defined (__GO32__) || defined (__WATCOMC__)
#   ifdef GO32_VIDEO
   SLtt_Term_Cannot_Insert = 1;
#   else
   Video_Base = (unsigned char *) ScreenPrimary;
#   endif
   if (!Attribute_Byte) Attribute_Byte = 0x17;
   IsColor = 1;			/* is it really? */
   
   if (IsColor)
     {
	union REGS r;
#   ifdef __WATCOMC__
	r.x.eax = 0x1003; r.x.ebx = 0;
#   else
	r.x.ax = 0x1003; r.x.bx = 0;
#   endif
	int86 (0x10, &r, &r);
	Blink_Killed = 1;
     }
#  else	/* (__GO32__ or __WATCOMC__ */
     {
	unsigned char *p = (unsigned char far *) 0x00400049L;
	if (*p == 7)
	  {
	     Video_Status_Port = MONO_STATUS;
	     Video_Base = (unsigned char *) MK_FP (0xb000,0000);
	     IsColor = 0;
	  }
	else
	  {
	     Video_Status_Port = CGA_STATUS;
	     Video_Base = (unsigned char *) MK_FP (0xb800,0000);
	     IsColor = 1;
	  }
     }
   
   /* test for video adapter type.  Of primary interest is whether there is
    * snow or not.  Assume snow if the card is color and not EGA or greater.
    */
   
   /* Use Ralf Brown test for EGA or greater */
   asm  mov ah, 0x12
     asm  mov bl, 0x10
     asm  mov bh, 0xFF
     asm  int 10h
     asm  cmp bh, 0xFF
     asm  je L1
     
   /* (V)EGA */
     asm  xor bx, bx
     asm  mov SLtt_Msdos_Cheap_Video, bx
     asm  mov ax, Attribute_Byte
     asm  cmp ax, bx
     asm  jne L2
     asm  mov ax, 0x17
     asm  mov Attribute_Byte, ax
     asm  jmp L2
     
     L1:
   /* Not (V)EGA */
   asm  mov ah, 0x0F
     asm  int 10h
     asm  cmp al, 7
     asm  je L3
     asm  mov ax, 1
     asm  mov SLtt_Msdos_Cheap_Video, ax
     L3:
   asm  mov ax, Attribute_Byte
     asm  cmp ax, 0
     asm  jne L2
     asm  mov ax, 0x07
     asm  mov Attribute_Byte, ax
     L2:
   /* toggle the blink bit so we can use hi intensity background */
   if (IsColor && !SLtt_Msdos_Cheap_Video)
     {
	asm  mov ax, 0x1003
	  asm  mov bx, 0
	  asm  int 0x10
	  Blink_Killed = 1;
     }
#  endif	/* __GO32__ or __WATCOMC__ */
# endif	/* __os2__ */
#endif	/* EMX_VIDEO */
   SLtt_set_scroll_region (0, SLtt_Screen_Rows);
   SLtt_Use_Ansi_Colors = IsColor;
}

/*----------------------------------------------------------------------*\
 * Function:	int SLtt_flush_output (void);
\*----------------------------------------------------------------------*/
int SLtt_flush_output (void)
{
   fflush (stdout);
   return -1;
}

int SLtt_set_cursor_visibility (int show)
{
   (void) show;
   return -1;
}

void SLtt_set_mono (int obj_unused, char *unused, SLtt_Char_Type c_unused)
{
   (void) obj_unused;
   (void) unused;
   (void) c_unused;
}

/* /////////////////////// end of file (c source) ///////////////////// */
