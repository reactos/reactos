/* Copyright (c) 1992, 1995 John E. Davis
 * All rights reserved.
 * 
 * You may distribute under the terms of either the GNU General Public
 * License or the Perl Artistic License.
 */

/*
 * SLTT_TRANSP_ACS_PATCH (#define/#undef):
 *
 *  The problem: some terminals (e.g. QNX/qansi*) map the whole upper half of
 *  the ASCII table to the lower half, when alt-char-set is activated with
 *  the smacs/as string-sequence. This means, that if 0 <= ch < 128 written
 *  to the terminal, it will be translated to (ch+128) automatically by the
 *  terminal: so not only the line-drawing characters can be written, when
 *  the alt-char-set is activated. It implicitly means, that space, NL, CR,
 *  etc. characters (exactly: anything besides the "standard" line drawing
 *  characters) can not be written directly to the terminal, when the
 *  alt-char-set is activated, because writing these characters doesn't cause
 *  an implicit/temporary switching-back to the standard char-set!
 *  
 *  The original code in SLang assumes that space, NL, CR, etc. can be
 *  printed when alt-char-set is activated. If SLTT_TRANSP_ACS_PATCH is
 *  defined, the modified code will not use this assumption.
 *  [Remark: the patch-code is not the most exact solution, but works...] 
 */
/*#define SLTT_TRANSP_ACS_PATCH 1*/

/*
 * QNX_QANSI_SLANG_COMPAT_ACS (#define/#undef):
 *
 *  A more OS/terminal-specific solution for the problem mentioned above
 *  (->SLTT_TRANSP_ACS_PATCH).
 *
 *  If QNX_QANSI_SLANG_COMPAT_ACS is defined, the default smacs/sa, rmacs/ae,
 *  acsc/ac [and sgr/sa, if it would be used!] command sequences will be
 *  replaced internally with the "old style" (pre-QNX 4.23) sequences in case
 *  of QNX/qansi terminals. Using these optional command sequences the terminal
 *  remains compatible with the original SLang code (without using the
 *  workaround-code enabled by defining SLTT_TRANSP_ACS_PATCH).
 */
/*#define QNX_QANSI_SLANG_COMPAT_ACS 1*/

/* auto-configuration */
#ifdef SLTT_TRANSP_ACS_PATCH
# if defined(__QNX__) && defined(QNX_QANSI_SLANG_COMPAT_ACS)
#  undef SLTT_TRANSP_ACS_PATCH
# endif
#else
# if defined(__QNX__) && !defined(QNX_QANSI_SLANG_COMPAT_ACS)
#  define QNX_QANSI_SLANG_COMPAT_ACS 1
# endif
#endif

#include "sl-feat.h"
#include "config.h"

#include <stdio.h>
#include <string.h>
#ifdef SCO_FLAVOR
#	include <sys/types.h>
#	include <sys/timeb.h> /* for struct timeb, used in time.h */
#endif
#include <time.h>
#include <ctype.h>

#ifndef VMS
# include <sys/time.h>
# ifdef __QNX__
#   include <sys/select.h>
# endif
# include <sys/types.h>
#endif

#ifdef __BEOS__
/* Prototype for select */
# include <net/socket.h>
#endif

#ifdef HAVE_TERMIOS_H
# include <termios.h>
#endif

#ifdef VMS
# include <unixlib.h>
# include <unixio.h>
# include <dvidef.h>
# include <descrip.h>
# include <lib$routines.h>
# include <starlet.h>
#else
# if !defined(sun)
#  include <sys/ioctl.h>
# endif
#endif


#ifdef SYSV
# include <sys/termio.h>
# include <sys/stream.h>
# include <sys/ptem.h>
# include <sys/tty.h>
#endif

#if defined (_AIX) && !defined (FD_SET)
# include <sys/select.h>	/* for FD_ISSET, FD_SET, FD_ZERO */
#endif

#include <errno.h>
#include "slang.h"
#include "_slang.h"

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#if defined(__DECC) && defined(VMS)
/* These get prototypes for write an sleep */
# include <unixio.h>
#endif
#include <signal.h>


/* Colors:  These definitions are used for the display.  However, the 
 * application only uses object handles which get mapped to this
 * internal representation.  The mapping is performed by the Color_Map
 * structure below. */

#define CHAR_MASK	0x000000FF
#define FG_MASK		0x0000FF00
#define BG_MASK		0x00FF0000
#define ATTR_MASK	0x1F000000
#define BGALL_MASK	0x0FFF0000

/* The 0x10000000 bit represents the alternate character set.  BGALL_MASK does
 * not include this attribute.
 */



#define GET_FG(color) ((color & FG_MASK) >> 8)
#define GET_BG(color) ((color & BG_MASK) >> 16)
#define MAKE_COLOR(fg, bg) (((fg) | ((bg) << 8)) << 8)

int SLtt_Screen_Cols;
int SLtt_Screen_Rows;
int SLtt_Term_Cannot_Insert;
int SLtt_Term_Cannot_Scroll;
int SLtt_Use_Ansi_Colors;
int SLtt_Blink_Mode = 1;
int SLtt_Use_Blink_For_ACS = 0;
int SLtt_Newline_Ok = 0;
int SLtt_Has_Alt_Charset = 0;
int SLtt_Force_Keypad_Init = 0;

/* -1 means unknown */
int SLtt_Has_Status_Line = -1;	       /* hs */

static int Automatic_Margins;
/* static int No_Move_In_Standout; */
static int Worthless_Highlight;
#define HP_GLITCH_CODE
#ifdef HP_GLITCH_CODE
/* This glitch is exclusive to HP term.  Basically it means that to clear 
 * attributes, one has to erase to the end of the line.
 */
static int Has_HP_Glitch;
#endif

static char *Reset_Color_String;

static int Linux_Console;

/* It is crucial that JMAX_COLORS must be less than 128 since the high bit
 * is used to indicate a character from the ACS (alt char set).  The exception
 * to this rule is if SLtt_Use_Blink_For_ACS is true.  This means that of 
 * the highbit is set, we interpret that as a blink character.  This is 
 * exploited by DOSemu.
 */
#define JMAX_COLORS 256
#define JNORMAL_COLOR 0

typedef struct 
{
   SLtt_Char_Type fgbg;
   SLtt_Char_Type mono;
   char *custom_esc;
} Ansi_Color_Type;

#define RGB1(r, g, b)   ((r) | ((g) << 1) | ((b) << 2))
#define RGB(r, g, b, br, bg, bb)  ((RGB1(r, g, b) << 8) | (RGB1(br, bg, bb) << 16))

static Ansi_Color_Type Ansi_Color_Map[JMAX_COLORS] = 
{
   {RGB(1, 1, 1, 0, 0, 0), 0x00000000, NULL},   /* white/black */
   {RGB(0, 1, 0, 0, 0, 0), SLTT_REV_MASK, NULL},   /* green/black */
   {RGB(1, 0, 1, 0, 0, 0), SLTT_REV_MASK, NULL},   /* magenta/black */
   {RGB(0, 1, 1, 0, 0, 0), SLTT_REV_MASK, NULL},   /* cyan/black */
   {RGB(1, 0, 0, 0, 0, 0), SLTT_REV_MASK, NULL},
   {RGB(0, 1, 0, 0, 0, 1), SLTT_REV_MASK, NULL},
   {RGB(1, 0, 0, 0, 0, 1), SLTT_REV_MASK, NULL},
   {RGB(1, 0, 0, 0, 1, 0), SLTT_REV_MASK, NULL},
   {RGB(0, 0, 1, 1, 0, 0), SLTT_REV_MASK, NULL},
   {RGB(0, 1, 0, 1, 0, 0), SLTT_REV_MASK, NULL},
   {RGB(0, 1, 1, 1, 1, 1), SLTT_REV_MASK, NULL},
   {RGB(1, 1, 0, 1, 1, 1), SLTT_REV_MASK, NULL},
   {RGB(1, 0, 1, 1, 1, 1), SLTT_REV_MASK, NULL},
   {RGB(0, 0, 0, 0, 1, 1), SLTT_REV_MASK, NULL},
   {RGB(0, 1, 0, 1, 1, 1), SLTT_REV_MASK, NULL},
   {RGB(0, 1, 0, 1, 1, 1), SLTT_REV_MASK, NULL},
   {RGB(0, 1, 0, 1, 1, 1), SLTT_REV_MASK, NULL},
   {RGB(0, 1, 0, 1, 1, 1), SLTT_REV_MASK, NULL}
};


/* This is the string to use to use when outputting color information.
 */
#ifdef M_UNIX
/* work around for sco console bug that can't handle combined sequences */
  static char *Color_Escape_Sequence = "\033[3%dm\033[4%dm";
#else
/* Believe it or not, this is what is in the linux terminfo database.  It
 * produces the same escape sequence but it is much more CPU intensive.  
 * Why not just encode it as "\033[3%p1%dm\033[4%p2%dm" ???
 */
 /* static char *Color_Escape_Sequence = "\033[%p1%{30}%+%dm\033[%p2%{40}%+%dm"; */
static char *Color_Escape_Sequence = "\033[3%d;4%dm";
#endif

char *SLtt_Graphics_Char_Pairs;	       /* ac termcap string -- def is vt100 */

   
/* 1 if terminal lacks the ability to do into insert mode or into delete
   mode. Currently controlled by S-Lang but later perhaps termcap. */

static char *UnderLine_Vid_Str;
static char *Blink_Vid_Str;
static char *Bold_Vid_Str;
static char *Ins_Mode_Str; /* = "\033[4h"; */   /* ins mode (im) */
static char *Eins_Mode_Str; /* = "\033[4l"; */  /* end ins mode (ei) */
static char *Scroll_R_Str; /* = "\033[%d;%dr"; */ /* scroll region */
static char *Cls_Str; /* = "\033[2J\033[H"; */  /* cl termcap STR  for ansi terminals */
static char *Rev_Vid_Str; /* = "\033[7m"; */    /* mr,so termcap string */
static char *Norm_Vid_Str; /* = "\033[m"; */   /* me,se termcap string */
static char *Del_Eol_Str; /* = "\033[K"; */	       /* ce */
static char *Del_Char_Str; /* = "\033[P"; */   /* dc */
static char *Del_N_Lines_Str; /* = "\033[%dM"; */  /* DL */
static char *Add_N_Lines_Str; /* = "\033[%dL"; */  /* AL */
static char *Rev_Scroll_Str;
static char *Curs_Up_Str;
static char *Curs_F_Str;    /* RI termcap string */
static char *Cursor_Visible_Str;    /* ve termcap string */
static char *Cursor_Invisible_Str;    /* vi termcap string */

static char *Start_Alt_Chars_Str;  /* as */
static char *End_Alt_Chars_Str;   /* ae */
static char *Enable_Alt_Char_Set;  /* eA */

static char *Term_Init_Str;
static char *Keypad_Init_Str;
static char *Term_Reset_Str;
static char *Keypad_Reset_Str;

/* status line functions */
static char *Disable_Status_line_Str;  /* ds */
static char *Return_From_Status_Line_Str;   /* fs */
static char *Goto_Status_Line_Str;     /* ts */
static int Num_Status_Line_Columns;    /* ws */
static int Status_Line_Esc_Ok;	       /* es */

/* static int Len_Curs_F_Str = 5; */

/* cm string has %i%d since termcap numbers columns from 0 */
/* char *CURS_POS_STR = "\033[%d;%df";  ansi-- hor and vert pos */
static char *Curs_Pos_Str; /* = "\033[%i%d;%dH";*/   /* cm termcap string */


/* scrolling region */
static int Scroll_r1 = 0, Scroll_r2 = 23;
static int Cursor_r, Cursor_c;	       /* 0 based */

/* current attributes --- initialized to impossible value */
static SLtt_Char_Type Current_Fgbg = 0xFFFFFFFFU;

static int Cursor_Set;		       /* 1 if cursor position known, 0
					* if not.  -1 if only row is known
					*/


#define MAX_OUTPUT_BUFFER_SIZE 4096

static unsigned char Output_Buffer[MAX_OUTPUT_BUFFER_SIZE];
static unsigned char *Output_Bufferp = Output_Buffer;

unsigned long SLtt_Num_Chars_Output;

#ifdef SLTT_TRANSP_ACS_PATCH
static int SLtt_ACS_Active = 0;
#endif

static int sl_usleep (unsigned long usecs)
{
#ifndef VMS
   struct timeval tv;
   tv.tv_sec = usecs / 1000000;
   tv.tv_usec = usecs % 1000000;
   return select(0, NULL, NULL, NULL, &tv);
#else
   return 0;
#endif
}

int SLtt_flush_output (void)
{
   int nwrite = 0;
   unsigned int total;
   int n = (int) (Output_Bufferp - Output_Buffer);
   
   SLtt_Num_Chars_Output += n;
   
   total = 0;
   while (n > 0)
     {
	nwrite = write (fileno(stdout), (char *) Output_Buffer + total, n);
	if (nwrite == -1)
	  {
	     nwrite = 0;
#ifdef EAGAIN
	     if (errno == EAGAIN) 
	       {
		  sl_usleep (100000);   /* 1/10 sec */
		  continue;
	       }
#endif
#ifdef EWOULDBLOCK
	     if (errno == EWOULDBLOCK)
	       {
		  sl_usleep (100000);
		  continue;
	       }
#endif
#ifdef EINTR
	     if (errno == EINTR) continue;
#endif
	     break;
	  }
	n -= nwrite;
	total += nwrite;
     }
   Output_Bufferp = Output_Buffer;
   return n;
}


int SLtt_Baud_Rate;
static void tt_write(char *str, unsigned int n)
{
   static unsigned long last_time;
   static int total;
   unsigned long now;
   unsigned int ndiff;
   
   if ((str == NULL) || (n == 0)) return;
   total += n;
   
   while (1)
     {
	ndiff = MAX_OUTPUT_BUFFER_SIZE - (int) (Output_Bufferp - Output_Buffer);
	if (ndiff < n)
	  {
	     SLMEMCPY ((char *) Output_Bufferp, (char *) str, ndiff);
	     Output_Bufferp += ndiff;
	     SLtt_flush_output ();
	     n -= ndiff;
	     str += ndiff;
	  }
	else 
	  {
	     SLMEMCPY ((char *) Output_Bufferp, str, n);
	     Output_Bufferp += n;
	     break;
	  }
     }
   
   if (((SLtt_Baud_Rate > 150) && (SLtt_Baud_Rate <= 9600))
       && (10 * total > SLtt_Baud_Rate))
     {
	total = 0;
	if ((now = (unsigned long) time(NULL)) - last_time <= 1)
	  {
	     SLtt_flush_output ();
	     sleep((unsigned) 1);
	  }
	last_time = now;
     }
}


void SLtt_write_string (char *str)
{
   if (str != NULL) tt_write(str, strlen(str));
}


void SLtt_putchar (char ch)
{
#ifdef SLTT_TRANSP_ACS_PATCH
   int restore_acs = 0;
#endif
    
   SLtt_normal_video ();
   if (Cursor_Set == 1)
     {
	if (ch >= ' ') Cursor_c++;
#ifndef SLTT_TRANSP_ACS_PATCH
	else if (ch == '\b') Cursor_c--;
#else 
	if (ch <= ' ' && SLtt_ACS_Active)
	  {
	     SLtt_set_alt_char_set (0);
	     restore_acs = 1;
	  }
	if (ch == '\b') Cursor_c--;
#endif
	else if (ch == '\r') Cursor_c = 0;
	else Cursor_Set = 0;
	
	if ((Cursor_c + 1 == SLtt_Screen_Cols) 
	    && Automatic_Margins) Cursor_Set = 0;
     }
   
   if (Output_Bufferp < Output_Buffer + MAX_OUTPUT_BUFFER_SIZE)
     {
	*Output_Bufferp++ = (unsigned char) ch;
     }
   else tt_write (&ch, 1);
   
#ifdef SLTT_TRANSP_ACS_PATCH
   if (restore_acs)
     {
        SLtt_set_alt_char_set (1);
     }
#endif
}

/* this is supposed to be fast--- also handles 
   termcap: %d, &i, %., %+, %r strings as well as terminfo stuff */
static unsigned int tt_sprintf(char *buf, char *fmt, int x, int y)
{
   register unsigned char *f = (unsigned char *) fmt, *b, ch;
   int offset = 0, tinfo = 0;
   int stack[10];
   int i = 0, z;
   stack[0] = y; stack[1] = x; i = 2;
   
   b = (unsigned char *) buf;
   if (fmt != NULL) while ((ch = *f++) != 0)
     {
	if (ch != '%') *b++ = ch;
	else 
	  {
	     ch = *f++;
	     if (tinfo)
	       {
		  if ((ch <= '3') && (ch >= '0'))
		    {
		       /* map it to termcap.  Since this is terminfo,
			* it must be one of:
			*   %2d, %3d, %02d, %03d
			* 
			* I am assuming that a terminal that understands
			* %2d form will also understand the %02d form.  These
			* only differ by a space padding the field.
			*/
		       
		       /* skip the 'd'-- hope it is there */
		       if (ch == '0') 
			 {
			    ch = *f;
			    f += 2;
			 }
		       else f++;
		    }
	       }
	     
	     switch (ch)
	       {
		case 'p':
		  tinfo = 1;
		  ch = *f++;
		  if (ch == '1') stack[i++] = x; else stack[i++] = y;
		  break;
		  
		case '\'':   /* 'x' */
		  stack[i++] = *f++;
		  f++;
		  break;
		  
		case '{':	       /* literal constant, e.g. {30} */
		  z = 0;
		  while (((ch = *f) <= '9') && (ch >= '0'))
		    {
		       z = z * 10 + (ch - '0');
		       f++;
		    }
		  stack[i++] = z;
		  if (ch == '}') f++;
		  break;
		       
		case 'd':
		case '2':
		case '3':
		  z = stack[--i];
		  z += offset;
		  if (z >= 100)
		    {
		       *b++ = z / 100 + '0';
		       z = z % 100;
		       goto ten;
		    }
		  else if (ch == 3) 
		    {
		       *b++ = '0';
		       ch = '2';
		    }
		  
		  if (z >= 10)
		    {
		       ten:
		       *b++ = z / 10 + '0';
		       z = z % 10;
		    }
		  else if (ch == 2) *b++ = '0';
		  
		  *b++ = z + '0';
		  break;
		  
		case 'i':
		  offset = 1;
		  break;
		  
		case '+':
		  if (tinfo) 
		    {
		       z = stack[--i];
		       stack[i-1] += z;
		    }
		  else
		    {
		       ch = *f++;
		       if ((unsigned char) ch == 128) ch = 0;
		       ch = ch + (unsigned char) stack[--i];
		       if (ch == '\n') ch++;
		       *b++ = ch;
		    }
		  break;
		 
		case 'r':
		  stack[0] = x;
		  stack[1] = y;
		  break;
		  
		case '.':
		case 'c':
		  ch = (unsigned char) stack[--i];
		  if (ch == '\n') ch++;
		  *b++ = ch;
		  break;
		  
		default:
		  *b++ = ch;
	       }
	  }
     }
   *b = 0;
   return((unsigned int) (b - (unsigned char *) buf));
}

static void tt_printf(char *fmt, int x, int y)
{
   char buf[256];
   unsigned int n;
   if (fmt == NULL) return;
   n = tt_sprintf(buf, fmt, x, y);
   tt_write(buf, n);
}


void SLtt_set_scroll_region (int r1, int r2)
{
   Scroll_r1 = r1;
   Scroll_r2 = r2;
   tt_printf (Scroll_R_Str, Scroll_r1, Scroll_r2);
   Cursor_Set = 0;
}

void SLtt_reset_scroll_region (void)
{
    SLtt_set_scroll_region(0, SLtt_Screen_Rows - 1);
}

int SLtt_set_cursor_visibility (int show)
{
   if ((Cursor_Visible_Str == NULL) || (Cursor_Invisible_Str == NULL))
     return -1;
   
   SLtt_write_string (show ? Cursor_Visible_Str : Cursor_Invisible_Str);
   return 0;
}

   
/* the goto_rc function moves to row relative to scrolling region */
void SLtt_goto_rc(int r, int c)
{
   char *s = NULL;
   int n;
   char buf[6];
#ifdef SLTT_TRANSP_ACS_PATCH
   int check_alt_acs = 0;
#endif
   
   if (c < 0)
     {
	c = -c - 1;
	Cursor_Set = 0;
     }
   
   /* if (No_Move_In_Standout && Current_Fgbg) SLtt_normal_video (); */
   r += Scroll_r1;
   
   if ((Cursor_Set > 0) || ((Cursor_Set < 0) && !Automatic_Margins))
     {
	n = r - Cursor_r;
	if ((n == -1) && (Cursor_Set > 0) && (Cursor_c == c) 
	    && (Curs_Up_Str != NULL))
	  {
	     s = Curs_Up_Str;
	  }
	else if ((n >= 0) && (n <= 4))
	  {
	     if ((n == 0) && (Cursor_Set == 1) 
		 && ((c > 1) || (c == Cursor_c)))
	       {
		  if (Cursor_c == c) return;
		  if (Cursor_c == c + 1) 
		    {
		       s = buf;
		       *s++ = '\b'; *s = 0;
		       s = buf;
#ifdef SLTT_TRANSP_ACS_PATCH
		       check_alt_acs = 1;
#endif
		    }
	       }
	     else if (c == 0)
	       {
		  s = buf;
		  if ((Cursor_Set != 1) || (Cursor_c != 0)) *s++ = '\r';
		  while (n--) *s++ = '\n';
#ifdef VMS
		  /* Need to add this after \n to start a new record.  Sheesh. */
		  *s++ = '\r';
#endif
		  *s = 0;
		  s = buf;
#ifdef SLTT_TRANSP_ACS_PATCH
		  check_alt_acs = 1;
#endif
	       }
	     /* Will fail on VMS */
#ifndef VMS
	     else if (SLtt_Newline_Ok && (Cursor_Set == 1) && 
		      (Cursor_c >= c) && (c + 3 > Cursor_c))
	       {
		  s = buf;
		  while (n--) *s++ = '\n';
		  n = Cursor_c - c;
		  while (n--) *s++ = '\b';
		  *s = 0;
		  s = buf;
#ifdef SLTT_TRANSP_ACS_PATCH
		  check_alt_acs = 1;
#endif
	       }
#endif
	  }
     }
#ifndef SLTT_TRANSP_ACS_PATCH
   if (s != NULL) SLtt_write_string(s);
#else
   if (s != NULL)
     {
        if (check_alt_acs && SLtt_ACS_Active)
          {
	     SLtt_set_alt_char_set (0);
	     SLtt_write_string(s);
	     SLtt_set_alt_char_set (1);
          }
        else SLtt_write_string(s);
     }
#endif
   else tt_printf(Curs_Pos_Str, r, c);
   Cursor_c = c; Cursor_r = r;
   Cursor_Set = 1;
}

void SLtt_begin_insert (void)
{
   SLtt_write_string(Ins_Mode_Str);
}

void SLtt_end_insert (void)
{
   SLtt_write_string(Eins_Mode_Str);
}

void SLtt_delete_char (void)
{
   SLtt_normal_video ();
   SLtt_write_string(Del_Char_Str);
}

void SLtt_erase_line (void)
{
   SLtt_write_string("\r");
   Cursor_Set = 1; Cursor_c = 0;
   SLtt_del_eol();
}

void SLtt_delete_nlines (int n)
{
   int r1, curs;
   char buf[132];
#ifdef SLTT_TRANSP_ACS_PATCH
   int restore_acs = 0;
#endif
   
   if (n <= 0) return;
   SLtt_normal_video ();
   if (Del_N_Lines_Str != NULL) tt_printf(Del_N_Lines_Str,n, 0);
   else
   /* get a new terminal */
     {
	r1 = Scroll_r1;
	curs = Cursor_r;
	SLtt_set_scroll_region(curs, Scroll_r2);
	SLtt_goto_rc(Scroll_r2 - Scroll_r1, 0);
#ifdef SLTT_TRANSP_ACS_PATCH
	if (SLtt_ACS_Active)
	  {
	     SLtt_set_alt_char_set (0);
	     restore_acs = 1;
	  }
#endif
	SLMEMSET(buf, '\n', (unsigned int) n);
	tt_write(buf, (unsigned int) n);
#ifdef SLTT_TRANSP_ACS_PATCH
	if (restore_acs) SLtt_set_alt_char_set (1);
#endif
	/* while (n--) tt_putchar('\n'); */
	SLtt_set_scroll_region(r1, Scroll_r2);
	SLtt_goto_rc(curs, 0);
     }
}

void SLtt_cls (void)
{
   SLtt_normal_video();
   SLtt_reset_scroll_region ();
   SLtt_write_string(Cls_Str);
}

void SLtt_reverse_index (int n)
{
   if (!n) return;
   
   SLtt_normal_video();
   if (Add_N_Lines_Str != NULL) tt_printf(Add_N_Lines_Str,n, 0);
   else
     {
	while(n--) SLtt_write_string(Rev_Scroll_Str);
     }
}


int SLtt_Ignore_Beep = 1;
static char *Visible_Bell_Str;

void SLtt_beep (void)
{
   if (SLtt_Ignore_Beep & 0x1) SLtt_putchar('\007');
   
   if (SLtt_Ignore_Beep & 0x2)
     {
	if (Visible_Bell_Str != NULL) SLtt_write_string (Visible_Bell_Str);
#ifdef __linux__
	else if (Linux_Console)
	  {
	     SLtt_write_string ("\033[?5h");
	     SLtt_flush_output ();
	     sl_usleep (50000);
	     SLtt_write_string ("\033[?5l");
	  }
#endif
     }
   SLtt_flush_output ();
}

void SLtt_del_eol (void)
{
   if (Current_Fgbg != 0xFFFFFFFFU) SLtt_normal_video ();
   SLtt_write_string(Del_Eol_Str);
}

typedef struct
{
   char *name;
   SLtt_Char_Type color;
}
Color_Def_Type;

#define MAX_COLOR_NAMES 17
static Color_Def_Type Color_Defs [MAX_COLOR_NAMES] =
{
   {"black",		SLSMG_COLOR_BLACK},
   {"red",		SLSMG_COLOR_RED},
   {"green",		SLSMG_COLOR_GREEN},
   {"brown",		SLSMG_COLOR_BROWN},
   {"blue",		SLSMG_COLOR_BLUE},
   {"magenta",		SLSMG_COLOR_MAGENTA},
   {"cyan",		SLSMG_COLOR_CYAN},
   {"lightgray",	SLSMG_COLOR_LGRAY},
   {"gray",		SLSMG_COLOR_GRAY},
   {"brightred",	SLSMG_COLOR_BRIGHT_RED},
   {"brightgreen",	SLSMG_COLOR_BRIGHT_GREEN},
   {"yellow",		SLSMG_COLOR_BRIGHT_BROWN},
   {"brightblue",	SLSMG_COLOR_BRIGHT_BLUE},
   {"brightmagenta",	SLSMG_COLOR_BRIGHT_CYAN},
   {"brightcyan",	SLSMG_COLOR_BRIGHT_MAGENTA},
   {"white",		SLSMG_COLOR_BRIGHT_WHITE},
   {"default",		25}
};


void SLtt_set_mono (int obj, char *what, SLtt_Char_Type mask)
{
   (void) what;
   if ((obj < 0) || (obj >= JMAX_COLORS))
     {
	return;
     }   
   Ansi_Color_Map[obj].mono = mask & ATTR_MASK;
}

static char *check_color_for_digit_form (char *color)
{
   unsigned int i, ich;
   char *s = color;
   
   i = 0;
   while ((ich = (int) *s) != 0)
     {
	if ((ich < '0') || (ich > '9'))
	  return color;
	
	i = i * 10 + (ich - '0');
	s++;
     }
   
   if (i < MAX_COLOR_NAMES)
     color = Color_Defs[i].name;
   
   return color;
}

static int get_default_colors (char **fgp, char **bgp)
{
   static char fg_buf[16], bg_buf[16], *bg, *fg;
   static int already_parsed;
   char *p, *pmax;
   
   if (already_parsed == -1)
     return -1;

   if (already_parsed)
     {
	*fgp = fg;
	*bgp = bg;
	return 0;
     }
   
   already_parsed = -1;
   
   bg = getenv ("COLORFGBG");

   if (bg == NULL)
     {
	bg = getenv ("DEFAULT_COLORS");
	if (bg == NULL)
	  return -1;
     }
   
   p = fg_buf;
   pmax = p + (sizeof (fg_buf) - 1);
	
   while ((*bg != 0) && (*bg != ';'))
     {
	if (p < pmax) *p++ = *bg;
	bg++;
     }
   *p = 0;
	
   if (*bg) bg++;
	
   p = bg_buf;
   pmax = p + (sizeof (bg_buf) - 1);
   
   /* Mark suggested allowing for extra spplication specific stuff following
    * the background color.  That is what the check for the semi-colon is for.
    */
   while ((*bg != 0) && (*bg != ';'))
     {
	if (p < pmax) *p++ = *bg;
	bg++;
     }
   *p = 0;

   if (!strcmp (fg_buf, "default") || !strcmp(bg_buf, "default"))
     {
	*fgp = *bgp = fg = bg = "default";
     }
   else
     {
	*fgp = fg = check_color_for_digit_form (fg_buf);
	*bgp = bg = check_color_for_digit_form (bg_buf);
     }
   already_parsed = 1;
   return 0;
}


static unsigned char FgBg_Stats[JMAX_COLORS];

static int Color_0_Modified = 0;

void SLtt_set_color_object (int obj, SLtt_Char_Type attr)
{
   char *cust_esc;
   
   if ((obj < 0) || (obj >= JMAX_COLORS)) return;
     
   cust_esc = Ansi_Color_Map[obj].custom_esc;
   if (cust_esc != NULL) 
     {
	SLFREE (cust_esc);
	FgBg_Stats[(Ansi_Color_Map[obj].fgbg >> 8) & 0x7F] -= 1;
	Ansi_Color_Map[obj].custom_esc = NULL;
     }
   
   Ansi_Color_Map[obj].fgbg = attr;
   if (obj == 0) Color_0_Modified = 1;
}

SLtt_Char_Type SLtt_get_color_object (int obj)
{
   if ((obj < 0) || (obj >= JMAX_COLORS)) return 0;
   return Ansi_Color_Map[obj].fgbg;
}


void SLtt_add_color_attribute (int obj, SLtt_Char_Type attr)
{
   if ((obj < 0) || (obj >= JMAX_COLORS)) return;
     
   Ansi_Color_Map[obj].fgbg |= (attr & ATTR_MASK);
   if (obj == 0) Color_0_Modified = 1;
}

static SLtt_Char_Type fb_to_fgbg (SLtt_Char_Type f, SLtt_Char_Type b)
{   
   SLtt_Char_Type attr = 0;

   if ((f & 0xF0) == 0)
     {
	if (f & 0x8) attr = SLTT_BOLD_MASK;
	f &= 0x7;
     }
   else f = 9;
   
   if ((b & 0xF0) == 0)
     {
	if (b & 0x8) attr |= SLTT_BLINK_MASK;
	b &= 0x7;
     }
   else b = 9;
   
   return ((f << 8) | (b << 16) | attr);
}

static int make_color_fgbg (char *fg, char *bg, SLtt_Char_Type *fgbg)
{
   SLtt_Char_Type f = 0xFFFFFFFFU, b = 0xFFFFFFFFU;
   char *dfg, *dbg;
   unsigned int i;
   
   if ((fg != NULL) && (*fg == 0)) fg = NULL;
   if ((bg != NULL) && (*bg == 0)) bg = NULL;
   
   if ((fg == NULL) || (bg == NULL))
     {
	if (-1 == get_default_colors (&dfg, &dbg))
	  return -1;
	
	if (fg == NULL) fg = dfg;
	if (bg == NULL) bg = dbg;
     }
   
   for (i = 0; i < MAX_COLOR_NAMES; i++)
     {
	if (strcmp(fg, Color_Defs[i].name)) continue;
	f = Color_Defs[i].color;
	break;
     }

   for (i = 0; i < MAX_COLOR_NAMES; i++)
     {
	if (strcmp(bg, Color_Defs[i].name)) continue;
	b = Color_Defs[i].color;
	break;
     }
   
   if ((f == 0xFFFFFFFFU) || (b == 0xFFFFFFFFU))
     return -1;
   
   *fgbg = fb_to_fgbg (f, b);
   return 0;
}

void SLtt_set_color (int obj, char *what, char *fg, char *bg)
{
   SLtt_Char_Type fgbg;

   (void) what;
   if ((obj < 0) || (obj >= JMAX_COLORS))
     return;

   if (-1 != make_color_fgbg (fg, bg, &fgbg))
     SLtt_set_color_object (obj, fgbg);
}

void SLtt_set_color_fgbg (int obj, SLtt_Char_Type f, SLtt_Char_Type b)
{
   SLtt_set_color_object (obj, fb_to_fgbg (f, b));
}

void SLtt_set_color_esc (int obj, char *esc)
{
   char *cust_esc;
   SLtt_Char_Type fgbg = 0;
   int i;
   
   if ((obj < 0) || (obj >= JMAX_COLORS))
     {
	return;
     }
   
   cust_esc = Ansi_Color_Map[obj].custom_esc;
   if (cust_esc != NULL) 
     {
	SLFREE (cust_esc);
	FgBg_Stats[(Ansi_Color_Map[obj].fgbg >> 8) & 0x7F] -= 1;
     }
   
   cust_esc = (char *) SLMALLOC (strlen(esc) + 1);
   if (cust_esc != NULL) strcpy (cust_esc, esc);
   
   Ansi_Color_Map[obj].custom_esc = cust_esc;
   if (cust_esc == NULL) fgbg = 0;
   else
     {
	/* The whole point of this is to generate a unique fgbg */
	for (i = 0; i < JMAX_COLORS; i++)
	  {
	     if (FgBg_Stats[i] == 0) fgbg = i;
	     
	     if (obj == i) continue;
	     if ((Ansi_Color_Map[i].custom_esc) == NULL) continue;
	     if (!strcmp (Ansi_Color_Map[i].custom_esc, cust_esc))
	       {
		  fgbg = (Ansi_Color_Map[i].fgbg >> 8) & 0x7F;
		  break;
	       }
	  }
	FgBg_Stats[fgbg] += 1;
     }
   
   fgbg |= 0x80;
   Ansi_Color_Map[obj].fgbg = (fgbg | (fgbg << 8)) << 8;
   if (obj == 0) Color_0_Modified = 1;
}

void SLtt_set_alt_char_set (int i)
{
#ifndef SLTT_TRANSP_ACS_PATCH
   static int last_i;
#else
#define last_i SLtt_ACS_Active
#endif
   if (SLtt_Has_Alt_Charset == 0) return;
   if (i == last_i) return;
   SLtt_write_string (i ? Start_Alt_Chars_Str : End_Alt_Chars_Str );
   /* if (i) Current_Fgbg |= SLTT_ALTC_MASK; 
    else Current_Fgbg &= ~SLTT_ALTC_MASK; */
   last_i = i;
#ifdef SLTT_TRANSP_ACS_PATCH
#undef last_i
#endif
}

static void write_attributes (SLtt_Char_Type fgbg)
{
   int bg0, fg0;

   if (Worthless_Highlight) return;
   if (fgbg == Current_Fgbg) return;
   
   /* Before spitting out colors, fix attributes */
   if ((fgbg & ATTR_MASK) != (Current_Fgbg & ATTR_MASK))
     {
	if (Current_Fgbg & ATTR_MASK)
	  {
	     SLtt_write_string(Norm_Vid_Str);
	     /* In case normal video turns off ALL attributes: */
	     if (fgbg & SLTT_ALTC_MASK) 
	       Current_Fgbg &= ~SLTT_ALTC_MASK;
	     SLtt_set_alt_char_set (0);
	  }
	
	if ((fgbg & SLTT_ALTC_MASK) 
	    != (Current_Fgbg & SLTT_ALTC_MASK))
	  {
	     SLtt_set_alt_char_set ((int) (fgbg & SLTT_ALTC_MASK));
	  }
	
	if (fgbg & SLTT_ULINE_MASK) SLtt_write_string (UnderLine_Vid_Str);
	if (fgbg & SLTT_BOLD_MASK) SLtt_bold_video ();
	if (fgbg & SLTT_REV_MASK) SLtt_write_string (Rev_Vid_Str);
	if (fgbg & SLTT_BLINK_MASK)
	  {
	     /* Someday Linux will have a blink mode that set high intensity
	      * background.  Lets be prepared.
	      */
	     if (SLtt_Blink_Mode) SLtt_write_string (Blink_Vid_Str);
	  }
     }
   
   if (SLtt_Use_Ansi_Colors)
     {
	fg0 = (int) GET_FG(fgbg);
	bg0 = (int) GET_BG(fgbg);
	tt_printf(Color_Escape_Sequence, fg0, bg0);
     }
   Current_Fgbg = fgbg;
}

static int Video_Initialized;

void SLtt_reverse_video (int color)
{
   SLtt_Char_Type fgbg;
   char *esc;
   
   if (Worthless_Highlight) return;
   if ((color < 0) || (color >= JMAX_COLORS)) return;
   
   if (Video_Initialized == 0)
     {
	if (color == JNORMAL_COLOR)
	  {
	     SLtt_write_string (Norm_Vid_Str);
	  }
	else SLtt_write_string (Rev_Vid_Str);
	Current_Fgbg = 0xFFFFFFFFU;
	return;
     }
   
   if (SLtt_Use_Ansi_Colors) 
     {
	fgbg = Ansi_Color_Map[color].fgbg;
	if ((esc = Ansi_Color_Map[color].custom_esc) != NULL)
	  {
	     if (fgbg != Current_Fgbg)
	       {
		  Current_Fgbg = fgbg;
		  SLtt_write_string (esc);
		  return;
	       }
	  }
     }
   else fgbg = Ansi_Color_Map[color].mono;

   if (fgbg == Current_Fgbg) return;
   write_attributes (fgbg);
}




void SLtt_normal_video (void)
{
   SLtt_reverse_video(JNORMAL_COLOR);
}

void SLtt_narrow_width (void)
{
    SLtt_write_string("\033[?3l");
}

void SLtt_wide_width (void)
{
    SLtt_write_string("\033[?3h");
}

/* Highest bit represents the character set. */
#define COLOR_MASK 0x7F00

#define COLOR_OF(x) (((unsigned int)(x) & COLOR_MASK) >> 8)
/*
#define COLOR_EQS(a, b) \
   (Ansi_Color_Map[COLOR_OF(a)].fgbg == Ansi_Color_Map[COLOR_OF(b)].fgbg)
*/

#define COLOR_EQS(a, b) \
   (SLtt_Use_Ansi_Colors \
    ? (Ansi_Color_Map[COLOR_OF(a)].fgbg == Ansi_Color_Map[COLOR_OF(b)].fgbg)\
    :  (Ansi_Color_Map[COLOR_OF(a)].mono == Ansi_Color_Map[COLOR_OF(b)].mono))


#define CHAR_EQS(a, b) (((a) == (b))\
			|| ((((a) & ~COLOR_MASK) == ((b) & ~COLOR_MASK))\
			    && COLOR_EQS((a), (b))))


/* The whole point of this routine is to prevent writing to the last column
 * and last row on terminals with automatic margins.
 */
static void write_string_with_care (char *str)
{
   unsigned int len;
   
   if (str == NULL) return;
   
   len = strlen (str);
   if (Automatic_Margins && (Cursor_r + 1 == SLtt_Screen_Rows))
     {
	if (len + (unsigned int) Cursor_c >= (unsigned int) SLtt_Screen_Cols)
	  {
	     /* For now, just do not write there.  Later, something more
	      * sophisticated will be implemented.
	      */
	     if (SLtt_Screen_Cols > Cursor_c)
	       len = SLtt_Screen_Cols - Cursor_c - 1;
	     else len = 0;
	  }
     }
   tt_write (str, len);
}

static void send_attr_str (unsigned short *s)
{
   unsigned char out[256], ch, *p;
   register SLtt_Char_Type attr;
   register unsigned short sh;
   int color, last_color = -1;
   
   p = out;
   while (0 != (sh = *s++))
     {
	ch = sh & 0xFF;
	color = ((int) sh & 0xFF00) >> 8;
#ifdef SLTT_TRANSP_ACS_PATCH
	if (ch <= ' ' && (color & 0x80)) color &= ~0x80;
#endif
	if (color != last_color)
	  {
	     if (SLtt_Use_Ansi_Colors) attr = Ansi_Color_Map[color & 0x7F].fgbg;
	     else attr = Ansi_Color_Map[color & 0x7F].mono;
	     
	     /* sh => color */
	     if (color & 0x80) /* alternate char set */
	       {
		  if (SLtt_Use_Blink_For_ACS)
		    {
		       if (SLtt_Blink_Mode) attr |= SLTT_BLINK_MASK;
		    }
		  else attr |= SLTT_ALTC_MASK;   
	       }
	     
	     
	     if (attr != Current_Fgbg)
	       {
#ifndef SLTT_TRANSP_ACS_PATCH
		  if ((ch != ' ') ||
		      /* it is a space so only consider it different if it
		       * has different attributes.
		       */
		      (attr & BGALL_MASK) != (Current_Fgbg & BGALL_MASK))
#endif
		    {
		       if (p != out)
			 {
			    *p = 0;
			    write_string_with_care ((char *) out);
			    Cursor_c += (int) (p - out);
			    p = out;
			 }

		       if (SLtt_Use_Ansi_Colors && (NULL != Ansi_Color_Map[color & 0x7F].custom_esc))
			 {
			    SLtt_write_string (Ansi_Color_Map[color & 0x7F].custom_esc);
			    /* Just in case the custom escape sequence screwed up
			     * the alt character set state...
			     */
	                    if ((attr & SLTT_ALTC_MASK) != (Current_Fgbg & SLTT_ALTC_MASK))
			      SLtt_set_alt_char_set ((int) (attr & SLTT_ALTC_MASK));
			    Current_Fgbg = attr;
			 }
		       else write_attributes (attr);
		       
		       last_color = color;
		    }
	       }
	  }
	*p++ = ch;
     }
   *p = 0;
   if (p != out) write_string_with_care ((char *) out);
   Cursor_c += (int) (p - out);
}

static void forward_cursor (unsigned int n, int row)
{
   char buf[30];
   
   
   if (n <= 4) 
     {
	SLtt_normal_video ();
	SLMEMSET (buf, ' ', n);
	buf[n] = 0;
	write_string_with_care (buf);
	Cursor_c += n;
     }
   else if (Curs_F_Str != NULL)
     {
	Cursor_c += n;
	n = tt_sprintf(buf, Curs_F_Str, (int) n, 0);
	tt_write(buf, n);
     }
   else SLtt_goto_rc (row, (int) (Cursor_c + n));
}


#define SPACE_CHAR (0x20 | (JNORMAL_COLOR << 8))                        

void SLtt_smart_puts(unsigned short *neww, unsigned short *oldd, int len, int row)
{
   register unsigned short *p, *q, *qmax, *pmax, *buf;
   unsigned short buffer[256];
   unsigned int n_spaces;
   unsigned short *space_match, *last_buffered_match;
#ifdef HP_GLITCH_CODE
   int handle_hp_glitch = 0;
#endif
   
   q = oldd; p = neww;
   qmax = oldd + len;
   pmax = p + len;
   
   /* Find out where to begin --- while they match, we are ok */
   while (1)
     {
	if (q == qmax) return;
#if SLANG_HAS_KANJI_SUPPORT
	if (*p & 0x80) 
	  { /* new is kanji */
	     if ((*q & 0x80) && ((q + 1) < qmax))
	       { /* old is also kanji */
		  if (((0xFF & *q) != (0xFF & *p))
		      || ((0xFF & q[1]) != (0xFF & p[1])))
		    break; /* both kanji, but not match */
		  
		  else 
		    { /* kanji match ! */
		       if (!COLOR_EQS(*q, *p)) break;
		       q++; p++;
		       if (!COLOR_EQS(*q, *p)) break;
		       /* really match! */
		       q++; p++;
		       continue;
		    }
	       }
	     else break; /* old is not kanji */
	  }
	else 
	  { /* new is not kanji */
	     if (*q & 0x80) break; /* old is kanji */
	  }
#endif
	if (!CHAR_EQS(*q, *p)) break;
	q++; p++;
     }

   /*position the cursor */
   SLtt_goto_rc (row, (int) (p - neww));

#ifdef HP_GLITCH_CODE
   if (Has_HP_Glitch)
     {
	unsigned short *qq = q;
	while (qq < qmax)
	  {
	     if (*qq & 0xFF00)
	       {
		  SLtt_normal_video ();
		  SLtt_del_eol ();
		  qmax = q;
		  handle_hp_glitch = 1;
		  break;
	       }
	     qq++;
	  }
     }
#endif
   /* Find where the last non-blank character on old/new screen is */
   
   while (qmax > q)
     {
	qmax--;
	if (!CHAR_EQS(*qmax, SPACE_CHAR)) 
	  {
	     qmax++;
	     break;
	  }
     }
   
   while (pmax > p)
     {
	pmax--;
	if (!CHAR_EQS(*pmax, SPACE_CHAR))
	  {
	     pmax++;
	     break;
	  }
     }
   
   last_buffered_match = buf = buffer;		       /* buffer is empty */

#ifdef HP_GLITCH_CODE
   if (handle_hp_glitch)
     {
	while (p < pmax)
	  {
	     *buf++ = *p++;
	  }
     }
#endif

   /* loop using overwrite then skip algorithm until done */
   while (1)
     {
	/* while they do not match and we do not hit a space, buffer them up */
	n_spaces = 0;
	while (p < pmax)
	  {
	     if (CHAR_EQS(*q,SPACE_CHAR) && CHAR_EQS(*p, SPACE_CHAR))
	       {
		  /* If *q is not a space, we would have to overwrite it.  
		   * However, if *q is a space, then while *p is also one, 
		   * we only need to skip over the blank field.
		   */
		  space_match = p;
		  p++; q++;
		  while ((p < pmax) 
			 && CHAR_EQS(*q,SPACE_CHAR) 
			 && CHAR_EQS(*p, SPACE_CHAR))
		    {
		       p++;
		       q++;
		    }
		  n_spaces = (unsigned int) (p - space_match);
		  break;
	       }
#if SLANG_HAS_KANJI_SUPPORT
	     if ((*p & 0x80) && ((p + 1) < pmax))
	       { /* new is kanji */
		  if (*q & 0x80) 
		    { /* old is also kanji */
 			  if (((0xFF & *q) != (0xFF & *p)) 
			      || ((0xFF & q[1]) != (0xFF & p[1])))
			 {
			    /* both kanji, but not match */
			    *buf++ = *p++;
			    *buf++ = *p++;
			    q += 2;
			    continue;
			 }
		       else 
			 { /* kanji match ? */
			    if (!COLOR_EQS(*q, *p) || !COLOR_EQS(*(q+1), *(p+1))) 
			      {
				 /* code is match ,but color is diff */
				 *buf++ = *p++;
				 *buf++ = *p++;
				 continue;
			      }
			    /* really match ! */
			    break;
			 }
		    }
 		  else 
		    { /* old is not kanji */
		       *buf++ = *p++;
		       *buf++ = *p++;
		       q += 2;
		       continue;
		    }
	       }
	     else 
	       { /* new is not kanji */
 		  if (*q & 0x80) 
		    { /* old is kanji */
		       *buf++ = *p++;
		       q++;
		       continue;
		    }
	       }
#endif

	     if (CHAR_EQS(*q, *p)) break;
	     *buf++ = *p++;
	     q++;
	  }
	*buf = 0;
	
	if (buf != buffer) send_attr_str (buffer);
	buf = buffer;
	
	if (n_spaces && (p < pmax)) 
	  {
	     forward_cursor (n_spaces, row);
	  }
	
	/* Now we overwrote what we could and cursor is placed at position 
	 * of a possible match of new and old.  If this is the case, skip 
	 * some more.
	 */
#if !SLANG_HAS_KANJI_SUPPORT	
	while ((p < pmax) && CHAR_EQS(*p, *q))
	  {
	     *buf++ = *p++;
	     q++;
	  }
#else
	/* Kanji */
	while (p < pmax) 
	  {
	     if ((*p & 0x80) && ((p + 1) < pmax))
	       { /* new is kanji */
		  if (*q & 0x80) 
		    { /* old is also kanji */
		       if (((0xFF & *q) == (0xFF & *p)) 
			   && ((0xFF & q[1]) == (0xFF & p[1])))
			 {
			    /* kanji match ? */
			    if (!COLOR_EQS(*q, *p) 
				|| !COLOR_EQS(q[1], p[1]))
			      break;
			    
			    *buf++ = *p++;
			    q++;
			    if (p >= pmax) 
			      {
				 *buf++ = SPACE_CHAR;
				 p++;
				 break;
			      }
			    else 
			      {
				 *buf++ = *p++;
				 q++;
				 continue;
			      }
			 }
		       else break; /* both kanji, but not match */
		    }
		  else break; /* old is not kanji */
	       }
	     else 
	       {  /* new is not kanji */
		  if (*q & 0x80) break; /* old is kanji */
		  if (!CHAR_EQS(*q, *p)) break;
		  *buf++ = *p++;
		  q++;
	       }
	  }
#endif
	last_buffered_match = buf;
	if (p >= pmax) break;
	
	/* jump to new position is it is greater than 5 otherwise
	 * let it sit in the buffer and output it later.
	 */
	if ((int) (buf - buffer) >= 5) 
	  {
	     forward_cursor ((unsigned int) (buf - buffer), row);
	     last_buffered_match = buf = buffer;
	  }
     }

   if (buf != buffer)
     {
	if (q < qmax)
	  {
	     if ((buf == last_buffered_match) 
		 && ((int) (buf - buffer) >= 5))
	       {
		  forward_cursor ((unsigned int) (buf - buffer), row);
	       }
	     else
	       {
		  *buf = 0;
		  send_attr_str (buffer);
	       }
	  }
     }
   if (q < qmax) SLtt_del_eol ();
   if (Automatic_Margins && (Cursor_c + 1 >= SLtt_Screen_Cols)) Cursor_Set = 0;
}  


static void get_color_info (void)
{
   char *fg, *bg;
   
   SLtt_Use_Ansi_Colors = (NULL != getenv ("COLORTERM"));
   
   if (-1 == get_default_colors (&fg, &bg))
     return;
     
   /* Check to see if application has already set them. */
   if (Color_0_Modified)
     return;
   
   SLtt_set_color (0, NULL, fg, bg);
   SLtt_set_color (1, NULL, bg, fg);
}


/* termcap stuff */

#ifdef __unix__

#ifndef USE_TERMCAP
static char *Tbuf;
static char *Tstr_Buf;

#define tgetstr SLtt_tigetstr
#define tgetent SLtt_tigetent
#define TGETNUM(x) SLtt_tigetnum((x), &Tbuf)
#define TGETFLAG(x) SLtt_tigetflag((x), &Tbuf)
#else

extern char *tgetstr(char *, char **);
extern int tgetent(char *, char *);
extern int tgetnum(char *);
extern int tgetflag(char *);
static char Tstr_Buf[1024];
static char Tbuf[4096];
#define TGETNUM tgetnum
#define TGETFLAG tgetflag
#endif

static char *my_tgetstr(char *what, char **p)
{
   register char *w, *w1;
   char *wsave;
   what = tgetstr(what, p);
   if (what != NULL)
     {
	/* Check for AIX brain-damage */
	if (*what == '@')
	  return NULL;
	
	/* lose pad info --- with today's technology, term is a loser if
	   it is really needed */
	while ((*what == '.') || 
	       ((*what >= '0') && (*what <= '9'))) what++;
	if (*what == '*') what++;	
	
	/* lose terminfo padding--- looks like $<...> */
        w = what;
	while (*w) if ((*w++ == '$') && (*w == '<'))
	  {
	     w1 = w - 1;
	     while (*w && (*w != '>')) w++;
	     if (*w == 0) break;
	     w++;
	     wsave = w1;
	     while ((*w1++ = *w++) != 0);
	     w = wsave;
	  }
	if (*what == 0) what = NULL; 
     }
   return(what);
}

char *SLtt_tgetstr (char *s)
{
#ifdef USE_TERMCAP
   static
#endif
     char *p = Tstr_Buf;
   return my_tgetstr (s, &p);
}

int SLtt_tgetnum (char *s)
{
   return TGETNUM (s);
}
int SLtt_tgetflag (char *s)
{
   return TGETFLAG (s);
}


static int Vt100_Like = 0;

void SLtt_get_terminfo (void)
{
   char *term, *t, ch;
   int is_xterm;
   int almost_vtxxx;
   
   get_color_info ();
   
   if (NULL == (term = (char *) getenv("TERM")))
     {
	SLang_exit_error("TERM environment variable needs set.");
     }
   
   Linux_Console = (!strncmp (term, "linux", 5)
#ifdef linux
		    || !strncmp(term, "con", 3)
#endif
		    );
      
   t = term;
   
   if (strcmp(t, "vt52") && (*t++ == 'v') && (*t++ == 't')
       && (ch = *t, (ch >= '1') && (ch <= '9'))) Vt100_Like = 1; 

   is_xterm = !strncmp (term, "xterm", 5);
   almost_vtxxx = (Vt100_Like
		   || Linux_Console
		   || is_xterm
		   || !strcmp (term, "screen"));
     
#ifndef USE_TERMCAP
   if (NULL == (Tbuf = tgetent (term))) 
     {
	char err_buf[512];
	if (almost_vtxxx) /* Special cases. */
	  {
	     int vt102 = 1;
	     if (!strcmp (term, "vt100")) vt102 = 0;
   	     SLtt_set_term_vtxxx (&vt102);
	     return;
	  }
	sprintf (err_buf, "Unknown terminal: %s\n\
Check the TERM environment variable.\n\
Also make sure that the terminal is defined in the terminfo database.\n\
Alternatively, set the TERMCAP environment variable to the desired\n\
termcap entry.", term);
	SLang_exit_error(err_buf);
     }
   Tstr_Buf = Tbuf;
#else				       /* USE_TERMCAP */
   if (1 != tgetent(Tbuf, term)) SLang_exit_error("Unknown terminal.");
#endif				       /* NOT USE_TERMCAP */
   
   if ((NULL == (Cls_Str = SLtt_tgetstr("cl"))) 
       || (NULL == (Curs_Pos_Str = SLtt_tgetstr("cm"))))
     {
	SLang_exit_error("Terminal not powerful enough for SLang.");
     }
   
   if ((NULL == (Ins_Mode_Str = SLtt_tgetstr("im")))
       || ( NULL == (Eins_Mode_Str = SLtt_tgetstr("ei")))
       || ( NULL == (Del_Char_Str = SLtt_tgetstr("dc"))))
     SLtt_Term_Cannot_Insert = 1;
   
   Visible_Bell_Str = SLtt_tgetstr ("vb");
   Curs_Up_Str = SLtt_tgetstr ("up");
   Rev_Scroll_Str = SLtt_tgetstr("sr");
   Del_N_Lines_Str = SLtt_tgetstr("DL");
   Add_N_Lines_Str = SLtt_tgetstr("AL");
   
   /* Actually these are used to initialize terminals that use cursor 
    * addressing.  Hard to believe.
    */
   Term_Init_Str = SLtt_tgetstr ("ti");
   Term_Reset_Str = SLtt_tgetstr ("te");

   /* If I do this for vtxxx terminals, arrow keys start sending ESC O A, 
    * which I do not want.  This is mainly for HP terminals.
    */
   if ((almost_vtxxx == 0) || SLtt_Force_Keypad_Init)
     {
	Keypad_Init_Str = SLtt_tgetstr ("ks");
	Keypad_Reset_Str = SLtt_tgetstr ("ke");
     }

   /* Make up for defective termcap/terminfo databases */
   if ((Vt100_Like && (term[2] != '1'))
       || Linux_Console
       || is_xterm
       )
     {
	if (Del_N_Lines_Str == NULL) Del_N_Lines_Str = "\033[%dM";
	if (Add_N_Lines_Str == NULL) Add_N_Lines_Str = "\033[%dL";
     }
   
   Scroll_R_Str = SLtt_tgetstr("cs");
   
   SLtt_get_screen_size ();

   if ((Scroll_R_Str == NULL) 
       || (((NULL == Del_N_Lines_Str) || (NULL == Add_N_Lines_Str))
	   && (NULL == Rev_Scroll_Str)))
     {
	if (is_xterm
	    || Linux_Console
	    )
	  {
	     /* Defective termcap mode!!!! */
	     SLtt_set_term_vtxxx (NULL);
	  }
	else SLtt_Term_Cannot_Scroll = 1;
     }
   
   Del_Eol_Str = SLtt_tgetstr("ce");

   Rev_Vid_Str = SLtt_tgetstr("mr");
   if (Rev_Vid_Str == NULL) Rev_Vid_Str = SLtt_tgetstr("so");
   
   Bold_Vid_Str = SLtt_tgetstr("md");

   /* Although xterm cannot blink, it does display the blinking characters
    * as bold ones.  Some Rxvt will display the background as high intensity.
    */
   if ((NULL == (Blink_Vid_Str = SLtt_tgetstr("mb")))
       && is_xterm)
     Blink_Vid_Str = "\033[5m";
   
   UnderLine_Vid_Str = SLtt_tgetstr("us");
   
   Start_Alt_Chars_Str = SLtt_tgetstr ("as");   /* smacs */
   End_Alt_Chars_Str = SLtt_tgetstr ("ae");   /* rmacs */
   Enable_Alt_Char_Set = SLtt_tgetstr ("eA");   /* enacs */
   SLtt_Graphics_Char_Pairs = SLtt_tgetstr ("ac");

#ifndef NCURSES_BRAIN_DAMAGE_CONTROL
# define NCURSES_BRAIN_DAMAGE_CONTROL 0
#endif
   
#if NCURSES_BRAIN_DAMAGE_CONTROL
   if (Linux_Console)
     {
# if 0
	char *lgcp = "l\332m\300k\277j\331u\264t\303v\301w\302q\304x\263n\053o\176s\137`\004a\260f\370g\361~\011,\020+\021.\031-\030h\261i\0250\333";

	SLtt_Graphics_Char_Pairs = lgcp;
	Start_Alt_Chars_Str = "\033(B\033)U\016";
	End_Alt_Chars_Str = "\033(B\033)0\017";
	Enable_Alt_Char_Set = NULL;
# else
	char *lgcp = "`\004a\261f\370g\361h\260j\331k\277l\332m\300n\305o\302q\304r\362s_t\303u\264v\301w\302x\263y\371z\372{\373|\374}\375~";
	
	SLtt_Graphics_Char_Pairs = lgcp;
	Start_Alt_Chars_Str = "\033[11m";
	End_Alt_Chars_Str = "\033[10m";
	Enable_Alt_Char_Set = NULL;
# endif
     }
#endif
   
   if (NULL == SLtt_Graphics_Char_Pairs)
     {
	/* make up for defective termcap/terminfo */
	if (Vt100_Like)
	  {
	     Start_Alt_Chars_Str = "\016";
	     End_Alt_Chars_Str = "\017";
	     Enable_Alt_Char_Set = "\033)0";
	  }
     }
   
    /* aixterm added by willi */
   if (is_xterm || !strncmp (term, "aixterm", 7))
     {
	Start_Alt_Chars_Str = "\016";
	End_Alt_Chars_Str = "\017";
	Enable_Alt_Char_Set = "\033(B\033)0";
     }
   
   if ((SLtt_Graphics_Char_Pairs == NULL) && 
       ((Start_Alt_Chars_Str == NULL) || (End_Alt_Chars_Str == NULL)))
     {
	SLtt_Has_Alt_Charset = 0;
	Enable_Alt_Char_Set = NULL;
     }
   else SLtt_Has_Alt_Charset = 1;
   

   /* status line capabilities */
   if ((SLtt_Has_Status_Line == -1)
       && (0 != (SLtt_Has_Status_Line = TGETFLAG ("hs"))))
     {
	Disable_Status_line_Str = SLtt_tgetstr ("ds");
	Return_From_Status_Line_Str = SLtt_tgetstr ("fs");
	Goto_Status_Line_Str = SLtt_tgetstr ("ts");
	Status_Line_Esc_Ok = TGETFLAG("es");
	Num_Status_Line_Columns = TGETNUM("ws");
	if (Num_Status_Line_Columns < 0) Num_Status_Line_Columns = 0;
     }
   
   if (NULL == (Norm_Vid_Str = SLtt_tgetstr("me"))) 
     {
	Norm_Vid_Str = SLtt_tgetstr("se");
     }
   
   Cursor_Invisible_Str = SLtt_tgetstr("vi");
   Cursor_Visible_Str = SLtt_tgetstr("ve");
   
   Curs_F_Str = SLtt_tgetstr("RI");
   
#if 0
   if (NULL != Curs_F_Str)
     {
	 Len_Curs_F_Str = strlen(Curs_F_Str);
     }
   else Len_Curs_F_Str = strlen(Curs_Pos_Str);
#endif
   
   Automatic_Margins = TGETFLAG ("am");
   /* No_Move_In_Standout = !TGETFLAG ("ms"); */
#ifdef HP_GLITCH_CODE
   Has_HP_Glitch = TGETFLAG ("xs");
#else
   Worthless_Highlight = TGETFLAG ("xs");
#endif

   if (Worthless_Highlight == 0)
     {				       /* Magic cookie glitch */
	Worthless_Highlight = (TGETNUM ("sg") > 0);
     }

   if (Worthless_Highlight)
     SLtt_Has_Alt_Charset = 0;
   
   /* Check for color information in the termcap.  A program should not
    * rely on this information being accurate.
    */
   if (SLtt_Use_Ansi_Colors == 0)
     {
	Reset_Color_String = SLtt_tgetstr ("op");
	
	SLtt_Use_Ansi_Colors = ((NULL != Reset_Color_String)
				|| (NULL != SLtt_tgetstr ("Sf"))
				|| (NULL != SLtt_tgetstr ("Sb"))
				|| (NULL != SLtt_tgetstr ("AF"))
				|| (NULL != SLtt_tgetstr ("AB"))
				|| (-1 != SLtt_tgetnum ("Co"))
				|| (-1 != SLtt_tgetnum ("pa")));
	
     }
   
#if defined(__QNX__) && defined(QNX_QANSI_SLANG_COMPAT_ACS)
   /*
    * Override the alt-char-set handling string in case of a
    * QNX/qansi terminal: use the "old style" strings in order
    * to be compatible with S-Lang without the SLTT_TRANSP_ACS_PATCH
    * code...
    */
   if (SLtt_Has_Alt_Charset &&
	    strncmp(term, "qansi", 5) == 0 &&
	    Start_Alt_Chars_Str[0] != '\016')
     {
        Start_Alt_Chars_Str = "\016"; /* smacs/as (^N) */
        End_Alt_Chars_Str = "\017";   /* rmacs/ae (^O) */
        SLtt_Graphics_Char_Pairs =    /* acsc/ac */
	    "``aaffggjjkkllmmnnooppqqrrssttuuvvwwxxyyzz{{||}}~~O\141";
	
	/* 
	 * it would be required to modify the sgr/sa entry also, if it
	 * would be used (->embedded as/ae sequences)...
	 */
     }
#endif /* __QNX__ && QNX_QANSI_SLANG_COMPAT_ACS */

}

#endif
/* Unix */

/* specific to vtxxx only */
void SLtt_enable_cursor_keys (void)
{
#ifdef __unix__
   if (Vt100_Like)
#endif
   SLtt_write_string("\033=\033[?1l");
}

#ifdef VMS
void SLtt_get_terminfo ()
{
   int zero = 0;
   
   get_color_info ();

   SLtt_set_term_vtxxx(&zero);
   Start_Alt_Chars_Str = "\016";
   End_Alt_Chars_Str = "\017";
   SLtt_Has_Alt_Charset = 1;
   SLtt_Graphics_Char_Pairs = "aaffgghhjjkkllmmnnooqqssttuuvvwwxx";
   Enable_Alt_Char_Set = "\033(B\033)0"; 
   SLtt_get_screen_size ();
}
#endif

/* This sets term for vt102 terminals it parameter vt100 is 0.  If vt100
 * is non-zero, set terminal appropriate for a only vt100  
 * (no add line capability). */
							   
void SLtt_set_term_vtxxx(int *vt100)
{
   Norm_Vid_Str = "\033[m";
   
   Scroll_R_Str = "\033[%i%d;%dr"; 
   Cls_Str = "\033[2J\033[H";
   Rev_Vid_Str = "\033[7m";
   Bold_Vid_Str = "\033[1m";
   Blink_Vid_Str = "\033[5m";
   UnderLine_Vid_Str = "\033[4m";
   Del_Eol_Str = "\033[K";
   Rev_Scroll_Str = "\033M";
   Curs_F_Str = "\033[%dC";
   /* Len_Curs_F_Str = 5; */
   Curs_Pos_Str = "\033[%i%d;%dH";
   if ((vt100 == NULL) || (*vt100 == 0))
     {
	Ins_Mode_Str = "\033[4h";
	Eins_Mode_Str = "\033[4l";
	Del_Char_Str =  "\033[P";
	Del_N_Lines_Str = "\033[%dM";
	Add_N_Lines_Str = "\033[%dL";
	SLtt_Term_Cannot_Insert = 0;
     }
   else
     {
	Del_N_Lines_Str = NULL;
	Add_N_Lines_Str = NULL;
	SLtt_Term_Cannot_Insert = 1;
     }
   SLtt_Term_Cannot_Scroll = 0;
   /* No_Move_In_Standout = 0; */
}

void SLtt_init_video (void)
{
   /*   send_string_to_term("\033[?6h"); */
   /* relative origin mode */
   SLtt_write_string (Term_Init_Str);
   SLtt_write_string (Keypad_Init_Str);
   SLtt_reset_scroll_region();
   SLtt_end_insert();
   SLtt_write_string (Enable_Alt_Char_Set);
   Video_Initialized = 1;
}


void SLtt_reset_video (void)
{
   SLtt_goto_rc (SLtt_Screen_Rows - 1, 0);
   Cursor_Set = 0;
   SLtt_normal_video ();	       /* MSKermit requires this  */
   SLtt_write_string(Norm_Vid_Str);

   Current_Fgbg = 0xFFFFFFFFU;
   SLtt_set_alt_char_set (0);
   if (SLtt_Use_Ansi_Colors) 
     {
	if (Reset_Color_String == NULL)
	  {
	     SLtt_Char_Type attr;
	     if (-1 != make_color_fgbg (NULL, NULL, &attr))
	       write_attributes (attr);
	     else SLtt_write_string ("\033[0m\033[m");
	  }
	else SLtt_write_string (Reset_Color_String);
	Current_Fgbg = 0xFFFFFFFFU;
     }
   SLtt_erase_line ();
   SLtt_write_string (Keypad_Reset_Str);
   SLtt_write_string (Term_Reset_Str);
   SLtt_flush_output ();
   Video_Initialized = 0;
}

void SLtt_bold_video (void)
{
   SLtt_write_string (Bold_Vid_Str);
}

int SLtt_set_mouse_mode (int mode, int force)
{
   char *term;
   
   if (force == 0)
     {
	if (NULL == (term = (char *) getenv("TERM"))) return -1;
	if (strncmp ("xterm", term, 5))
	  return -1;
     }

   if (mode)
     SLtt_write_string ("\033[?9h");
   else
     SLtt_write_string ("\033[?9l");
   
   return 0;
}


void SLtt_disable_status_line (void)
{
   if (SLtt_Has_Status_Line > 0)
     SLtt_write_string (Disable_Status_line_Str);
}

int SLtt_write_to_status_line (char *s, int col)
{
   if ((SLtt_Has_Status_Line <= 0)
       || (Goto_Status_Line_Str == NULL)
       || (Return_From_Status_Line_Str == NULL))
     return -1;
   
   tt_printf (Goto_Status_Line_Str, col, 0);
   SLtt_write_string (s);
   SLtt_write_string (Return_From_Status_Line_Str);
   return 0;
}

   
void SLtt_get_screen_size (void)
{   
#ifdef VMS
   int status, code;
   unsigned short chan;
   $DESCRIPTOR(dev_dsc, "SYS$INPUT:");
#endif
#ifdef __os2__
   VIOMODEINFO vioModeInfo;
#endif
   int r = 0, c = 0;
   
#if defined(TIOCGWINSZ) && !defined(SCO_FLAVOR)
   struct winsize wind_struct;
   
   do
     {
	if ((ioctl(1,TIOCGWINSZ,&wind_struct) == 0)
	    || (ioctl(0, TIOCGWINSZ, &wind_struct) == 0)
	    || (ioctl(2, TIOCGWINSZ, &wind_struct) == 0))
	  {
	     c = (int) wind_struct.ws_col;
	     r = (int) wind_struct.ws_row;
	     break;
	  }
     }
   while (errno == EINTR);
   
#endif

#ifdef VMS
   status = sys$assign(&dev_dsc,&chan,0,0,0);
   if (status & 1)
     {
	code = DVI$_DEVBUFSIZ;
	status = lib$getdvi(&code, &chan,0, &c, 0,0);
	if (!(status & 1))
	  c = 80;
	code = DVI$_TT_PAGE;
	status = lib$getdvi(&code, &chan,0, &r, 0,0);
	if (!(status & 1))
	  r = 24;
	sys$dassgn(chan);
     }
#endif

#ifdef __os2__
   vioModeInfo.cb = sizeof(vioModeInfo);
   VioGetMode (&vioModeInfo, 0);
   c = vioModeInfo.col;
   r = vioModeInfo.row;   
#endif
   
   if (r <= 0)
     {
	char *s = getenv ("LINES");
	if (s != NULL) r = atoi (s);
     }
   
   if (c <= 0)
     {
	char *s = getenv ("COLUMNS");
	if (s != NULL) c = atoi (s);
     }
   
   if ((r <= 0) || (r > 200)) r = 24;
   if ((c <= 0) || (c > 250)) c = 80;
   SLtt_Screen_Rows = r;
   SLtt_Screen_Cols = c;
}
