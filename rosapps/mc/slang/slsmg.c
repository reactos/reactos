/* SLang Screen management routines */
/* Copyright (c) 1992, 1995 John E. Davis
 * All rights reserved.
 * 
 * You may distribute under the terms of either the GNU General Public
 * License or the Perl Artistic License.
 */

#include "config.h"

#include <stdio.h>
#include <string.h>

#include "slang.h"
#include "_slang.h"

typedef struct Screen_Type
  {
     int n;                    /* number of chars written last time */
     int flags;                /* line untouched, etc... */
     unsigned short *old, *neew;
#ifndef pc_system
     unsigned long old_hash, new_hash;
#endif
  }
Screen_Type;

#define TOUCHED 0x1
#define TRASHED 0x2

#ifndef pc_system
#define MAX_SCREEN_SIZE 120
#else
#define MAX_SCREEN_SIZE 75
#endif

Screen_Type SL_Screen[MAX_SCREEN_SIZE];
static int Start_Col, Start_Row;
static int Screen_Cols, Screen_Rows;
static int This_Row, This_Col;
static int This_Color;		       /* only the first 8 bits of this
					* are used.  The highest bit is used
					* to indicate an alternate character
					* set.  This leaves 127 userdefineable
					* color combination.
					*/

#ifndef pc_system
#define ALT_CHAR_FLAG 0x80
#else
#define ALT_CHAR_FLAG 0x00
#endif

int SLsmg_Newline_Moves = 0;
int SLsmg_Backspace_Moves = 0;

static void blank_line (unsigned short *p, int n, unsigned char ch)
{
   register unsigned short *pmax = p + n;
   register unsigned short color_ch;

   color_ch = (This_Color << 8) | (unsigned short) ch;
   
   while (p < pmax)
     {
	*p++ = color_ch;
     }
}


static void clear_region (int row, int n)
{
   int i;
   int imax = row + n;
   
   if (imax > Screen_Rows) imax = Screen_Rows;
   for (i = row; i < imax; i++)
     {
	if (i >= 0)
	  {
	     blank_line (SL_Screen[i].neew, Screen_Cols, ' ');
	     SL_Screen[i].flags |= TOUCHED;
	  }
     }
}

void SLsmg_erase_eol (void)
{
   int r, c;
   
   c = This_Col - Start_Col;
   r = This_Row - Start_Row;
   
   if ((r < 0) || (r >= Screen_Rows)) return;
   if (c < 0) c = 0; else if (c >= Screen_Cols) return;
   blank_line (SL_Screen[This_Row].neew + c , Screen_Cols - c, ' ');
   SL_Screen[This_Row].flags |= TOUCHED;
}

static void scroll_up (void)
{
   unsigned int i, imax;
   unsigned short *neew;

   neew = SL_Screen[0].neew;
   imax = Screen_Rows - 1;
   for (i = 0; i < imax; i++)
     {
	SL_Screen[i].neew = SL_Screen[i + 1].neew;
	SL_Screen[i].flags |= TOUCHED;
     }
   SL_Screen[i].neew = neew;
   SL_Screen[i].flags |= TOUCHED;
   blank_line (neew, Screen_Cols, ' ');
   This_Row--;
}

   


void SLsmg_gotorc (int r, int c)
{
   This_Row = r;
   This_Col = c;
}

int SLsmg_get_row (void)
{
   return This_Row;
}

int SLsmg_get_column (void)
{
   return This_Col;
}

void SLsmg_erase_eos (void)
{
   SLsmg_erase_eol ();
   clear_region (This_Row + 1, Screen_Rows);
}

static int This_Alt_Char;

#ifndef pc_system
void SLsmg_set_char_set (int i)
{
   if (SLtt_Use_Blink_For_ACS) return; /* alt chars not used and the alt bit
					* is used to indicate a blink.
					*/
   if (i) This_Alt_Char = ALT_CHAR_FLAG;
   else This_Alt_Char = 0;
   
   This_Color &= 0x7F;
   This_Color |= This_Alt_Char;
}
#endif

void SLsmg_set_color (int color)
{
   if (color < 0) return;
   This_Color = color | This_Alt_Char;
}


void SLsmg_reverse_video (void)
{
   SLsmg_set_color (1);
}


void SLsmg_normal_video (void)
{
   This_Color = This_Alt_Char;	       /* reset video but NOT char set. */
}


static int point_visible (int col_too)
{
   return ((This_Row >= Start_Row) && (This_Row < Start_Row + Screen_Rows)
	   && ((col_too == 0)
	       || ((This_Col >= Start_Col) 
		   && (This_Col < Start_Col + Screen_Cols))));
}
   
void SLsmg_printf (char *fmt, ...)
{
   char p[1000];
   va_list ap;

   va_start(ap, fmt);
   (void) vsprintf(p, fmt, ap);
   va_end(ap);
   
   SLsmg_write_string (p);
}

void SLsmg_write_string (char *str)
{
   SLsmg_write_nchars (str, strlen (str));
}

void SLsmg_write_nstring (char *str, int n)
{
   int width;
   char blank = ' ';
   if (str == NULL) width = 0;
   else 
     {
	width = strlen (str);
	if (width > n) width = n;
	SLsmg_write_nchars (str, width);
     }
   while (width++ < n) SLsmg_write_nchars (&blank, 1);
}

void SLsmg_write_wrapped_string (char *s, int r, int c, int dr, int dc, int fill)
{
   register char ch, *p;
   int maxc = dc;
   
   if ((dr == 0) || (dc == 0)) return;
   p = s;
   dc = 0;
   while (1)
     {
	ch = *p++;
	if ((ch == 0) || (ch == '\n'))
	  {
	     int diff;
	     
	     diff = maxc - dc;
	     
	     SLsmg_gotorc (r, c);
	     SLsmg_write_nchars (s, dc);
	     if (fill && (diff > 0))
	       {
		  while (diff--) SLsmg_write_char (' ');
	       }
	     if ((ch == 0) || (dr == 1)) break;
	     
	     r++;
	     dc = 0;
	     dr--;
	     s = p;
	  }
	else if (dc == maxc)
	  {
	     SLsmg_gotorc (r, c);
	     SLsmg_write_nchars (s, dc + 1);
	     if (dr == 1) break;

	     r++;
	     dc = 0;
	     dr--;
	     s = p;
	  }
	else dc++;
     }
}

   

int SLsmg_Tab_Width = 8;

/* Minimum value for which eight bit char is displayed as is. */

#ifndef pc_system
int SLsmg_Display_Eight_Bit = 160;
static unsigned char Alt_Char_Set[129];/* 129th is used as a flag */
#else
int SLsmg_Display_Eight_Bit = 128;
#endif

void SLsmg_write_nchars (char *str, int n)
{
   register unsigned short *p, old, neew, color;
   unsigned char ch;
   unsigned int flags;
   int len, start_len, max_len;
   char *str_max;
   int newline_flag;
#ifndef pc_system
   int alt_char_set_flag;
   
   alt_char_set_flag = ((SLtt_Use_Blink_For_ACS == 0) 
			&& (This_Color & ALT_CHAR_FLAG));
#endif

   str_max = str + n;
   color = This_Color << 8;
   
   top:				       /* get here only on newline */
   
   newline_flag = 0;
   start_len = Start_Col;
   
   if (point_visible (0) == 0) return;
   
   len = This_Col;
   max_len = start_len + Screen_Cols;

   p = SL_Screen[This_Row].neew;
   if (len > start_len) p += (len - start_len);
   
   flags = SL_Screen[This_Row].flags;
   while ((len < max_len) && (str < str_max))
     {	
	ch = (unsigned char) *str++;

#ifndef pc_system
	if (alt_char_set_flag)
	  ch = Alt_Char_Set [ch & 0x7F];
#endif
	if (((ch >= ' ') && (ch < 127))
	    || (ch >= (unsigned char) SLsmg_Display_Eight_Bit)
#ifndef pc_system
	    || alt_char_set_flag
#endif
	    )
	  {
	     len += 1;
	     if (len > start_len)
	       {
		  old = *p;
		  neew = color | (unsigned short) ch;
		  if (old != neew)
		    {
		       flags |= TOUCHED;
		       *p = neew;
		    }
		  p++;
	       }
	  }
	
	else if ((ch == '\t') && (SLsmg_Tab_Width > 0))
	  {
	     n = len;
	     n += SLsmg_Tab_Width;
	     n = SLsmg_Tab_Width - (n % SLsmg_Tab_Width);
	     if (len + n > max_len) n = max_len - len;
	     neew = color | (unsigned short) ' ';
	     while (n--)
	       {
		  len += 1;
		  if (len > start_len)
		    {
		       if (*p != neew) 
			 {
			    flags |= TOUCHED;
			    *p = neew;
			 }
		       p++;
		    }
	       }
	  }
	else if (ch == '\n')
	  {
	     newline_flag = 1;
	     break;
	  }
	else if ((ch == 0x8) && SLsmg_Backspace_Moves)
	  {
	     if (len != 0) len--;
	  }
	else
	  {
	     if (ch & 0x80)
	       {
		  neew = color | (unsigned short) '~';
		  len += 1;
		  if (len > start_len)
		    {
		       if (*p != neew)
			 {
			    *p = neew;
			    flags |= TOUCHED;
			 }
		       p++;
		       if (len == max_len) break;
		       ch &= 0x7F;
		    }
	       }
	     
	     len += 1;
	     if (len > start_len)
	       {
		  neew = color | (unsigned short) '^';
		  if (*p != neew)
		    {
		       *p = neew;
		       flags |= TOUCHED;
		    }
		  p++;
		  if (len == max_len) break;
	       }
	     
	     if (ch == 127) ch = '?'; else ch = ch + '@';
	     len++;
	     if (len > start_len)
	       {
		  neew = color | (unsigned short) ch;
		  if (*p != neew)
		    {
		       *p = neew;
		       flags |= TOUCHED;
		    }
		  p++;
	       }
	  }
     }
   
   SL_Screen[This_Row].flags = flags;
   This_Col = len;
   
   if (SLsmg_Newline_Moves == 0)
     return;
   
   if (newline_flag == 0)
     {
	while (str < str_max)
	  {
	     if (*str == '\n') break;
	     str++;
	  }
	if (str == str_max) return;
	str++;
     }
   
   This_Row++;
   This_Col = 0;
   if (This_Row == Start_Row + Screen_Rows)
     {
	if (SLsmg_Newline_Moves > 0) scroll_up ();
     }
   goto top;
}


void SLsmg_write_char (char ch)
{
   SLsmg_write_nchars (&ch, 1);
}

static int Cls_Flag;


void SLsmg_cls (void)
{
   This_Color = 0;
   clear_region (0, Screen_Rows);
   This_Color = This_Alt_Char;
   Cls_Flag = 1;
}
#if 0
static void do_copy (unsigned short *a, unsigned short *b)
{
   unsigned short *amax = a + Screen_Cols;
   
   while (a < amax) *a++ = *b++;
}
#endif

#ifndef pc_system
int SLsmg_Scroll_Hash_Border = 0;
static unsigned long compute_hash (unsigned short *s, int n)
{
   register unsigned long h = 0, g;
   register unsigned long sum = 0;
   register unsigned short *smax, ch;
   int is_blank = 2;
   
   s += SLsmg_Scroll_Hash_Border;
   smax = s + (n - SLsmg_Scroll_Hash_Border);
   while (s < smax) 
     {
	ch = *s++;
	if (is_blank && ((ch & 0xFF) != 32)) is_blank--;
	
	sum += ch;
	
	h = sum + (h << 3);
	if ((g = h & 0xE0000000UL) != 0)
	  {
	     h = h ^ (g >> 24);
	     h = h ^ g;
	  }
     }
   if (is_blank) return 0;
   return h;
}

static unsigned long Blank_Hash;

static void try_scroll (void)
{
   int i, j, di, r1, r2, rmin, rmax;
   unsigned long hash;
   int color, did_scroll = 0;
   unsigned short *tmp;
   int ignore;
   
   /* find region limits. */
   
   for (rmax = Screen_Rows - 1; rmax > 0; rmax--)
     {
	if (SL_Screen[rmax].new_hash != SL_Screen[rmax].old_hash)
	  {
	     r1 = rmax - 1;
	     if ((r1 == 0)
		 || (SL_Screen[r1].new_hash != SL_Screen[r1].old_hash))
	       break;
	     
	     rmax = r1;
	  }
     }
   
   for (rmin = 0; rmin < rmax; rmin++)
     {
	if (SL_Screen[rmin].new_hash != SL_Screen[rmin].old_hash)
	  {
	     r1 = rmin + 1;
	     if ((r1 == rmax)
		 || (SL_Screen[r1].new_hash != SL_Screen[r1].old_hash))
	       break;
	     
	     rmin = r1;
	  }
     }

   
   for (i = rmax; i > rmin; i--)
     {
	hash = SL_Screen[i].new_hash;
	if (hash == Blank_Hash) continue;
	
	if ((hash == SL_Screen[i].old_hash) 
	    || ((i + 1 < Screen_Rows) && (hash == SL_Screen[i + 1].old_hash))
	    || ((i - 1 > rmin) && (SL_Screen[i].old_hash == SL_Screen[i - 1].new_hash)))
	  continue;
	
	for (j = i - 1; j >= rmin; j--)
	  {
	     if (hash == SL_Screen[j].old_hash) break;
	  }
	if (j < rmin) continue;
	
	r2 = i;			       /* end scroll region */
	
	di = i - j;
	j--;
	ignore = 0;
	while ((j >= rmin) && (SL_Screen[j].old_hash == SL_Screen[j + di].new_hash))
	  {
	     if (SL_Screen[j].old_hash == Blank_Hash) ignore++;
	     j--;
	  }
	r1 = j + 1;
	
	/* If this scroll only scrolls this line into place, don't do it.
	 */
	if ((di > 1) && (r1 + di + ignore == r2)) continue;
	
	/* If there is anything in the scrolling region that is ok, abort the 
	 * scroll.
	 */

	for (j = r1; j <= r2; j++)
	  {
	     if ((SL_Screen[j].old_hash != Blank_Hash)
		 && (SL_Screen[j].old_hash == SL_Screen[j].new_hash))
	       {
		  /* See if the scroll is happens to scroll this one into place. */
		  if ((j + di > r2) || (SL_Screen[j].old_hash != SL_Screen[j + di].new_hash))
		    break;
	       }
	  }
	if (j <= r2) continue;
	
	color = This_Color;  This_Color = 0;
	did_scroll = 1;
	SLtt_normal_video ();
	SLtt_set_scroll_region (r1, r2);
	SLtt_goto_rc (0, 0);
	SLtt_reverse_index (di);
	SLtt_reset_scroll_region ();
	/* Now we have a hole in the screen.  Make the virtual screen look 
	 * like it.
	 */
	for (j = r1; j <= r2; j++) SL_Screen[j].flags = TOUCHED;
	
	while (di--)
	  {
	     tmp = SL_Screen[r2].old;
	     for (j = r2; j > r1; j--)
	       {
		  SL_Screen[j].old = SL_Screen[j - 1].old;
		  SL_Screen[j].old_hash = SL_Screen[j - 1].old_hash;
	       }
	     SL_Screen[r1].old = tmp;
	     blank_line (SL_Screen[r1].old, Screen_Cols, ' ');
	     SL_Screen[r1].old_hash = Blank_Hash;
	     r1++;
	  }
	This_Color = color;
     }
   if (did_scroll) return;
   
   /* Try other direction */

   for (i = rmin; i < rmax; i++)
     {
	hash = SL_Screen[i].new_hash;
	if (hash == Blank_Hash) continue;
	if (hash == SL_Screen[i].old_hash) continue;
	
	/* find a match further down screen */
	for (j = i + 1; j <= rmax; j++)
	  {
	     if (hash == SL_Screen[j].old_hash) break;
	  }
	if (j > rmax) continue;
	
	r1 = i;			       /* beg scroll region */
	di = j - i;		       /* number of lines to scroll */
	j++;			       /* since we know this is a match */
	
	/* find end of scroll region */
	ignore = 0;
	while ((j <= rmax) && (SL_Screen[j].old_hash == SL_Screen[j - di].new_hash))
	  {
	     if (SL_Screen[j].old_hash == Blank_Hash) ignore++;
	     j++;
	  }
	r2 = j - 1;		       /* end of scroll region */
	
	/* If this scroll only scrolls this line into place, don't do it.
	 */
	if ((di > 1) && (r1 + di + ignore == r2)) continue;

	/* If there is anything in the scrolling region that is ok, abort the 
	 * scroll.
	 */
	
	for (j = r1; j <= r2; j++)
	  {
	     if ((SL_Screen[j].old_hash != Blank_Hash)
		 && (SL_Screen[j].old_hash == SL_Screen[j].new_hash))
	       {
		  if ((j - di < r1) || (SL_Screen[j].old_hash != SL_Screen[j - di].new_hash))
		    break;
	       }
	     
	  }
	if (j <= r2) continue;
	
	color = This_Color;  This_Color = 0;
	SLtt_normal_video ();
	SLtt_set_scroll_region (r1, r2);
	SLtt_goto_rc (0, 0);	       /* relative to scroll region */
	SLtt_delete_nlines (di);
	SLtt_reset_scroll_region ();
	/* Now we have a hole in the screen.  Make the virtual screen look 
	 * like it.
	 */
	for (j = r1; j <= r2; j++) SL_Screen[j].flags = TOUCHED;
	
	while (di--)
	  {
	     tmp = SL_Screen[r1].old;
	     for (j = r1; j < r2; j++)
	       {
		  SL_Screen[j].old = SL_Screen[j + 1].old;
		  SL_Screen[j].old_hash = SL_Screen[j + 1].old_hash;
	       }
	     SL_Screen[r2].old = tmp;
	     blank_line (SL_Screen[r2].old, Screen_Cols, ' ');
	     SL_Screen[r2].old_hash = Blank_Hash;
	     r2--;
	  }
	This_Color = color;
     }
}

#endif   /* NOT pc_system */
        
	
	
static int Smg_Inited;

void SLsmg_refresh (void)
{
   int i;
   
   if (Smg_Inited == 0) return;
#ifndef pc_system
   for (i = 0; i < Screen_Rows; i++)
     {
	if (SL_Screen[i].flags == 0) continue;
	SL_Screen[i].new_hash = compute_hash (SL_Screen[i].neew, Screen_Cols);
     }
#endif
   
   if (Cls_Flag) 
     {
	SLtt_normal_video ();  SLtt_cls ();
     }
#ifndef pc_system
   else if (SLtt_Term_Cannot_Scroll == 0) try_scroll ();
#endif

   for (i = 0; i < Screen_Rows; i++)
     {
	int trashed;
	
	if (SL_Screen[i].flags == 0) continue;
	
	if (SL_Screen[i].flags & TRASHED)
	  {
	     SLtt_goto_rc (i, -1); /* Force cursor to move */
	     SLtt_goto_rc (i, 0);
	     if (Cls_Flag == 0) SLtt_del_eol ();
	     trashed = 1;
	  }
	else trashed = 0;
	
	if (Cls_Flag || trashed) 
	  {
	     int color = This_Color;
	     This_Color = 0;
	     blank_line (SL_Screen[i].old, Screen_Cols, ' ');
	     This_Color = color;
	  }
	
	SL_Screen[i].old[Screen_Cols] = 0;
	SL_Screen[i].neew[Screen_Cols] = 0;
	
	SLtt_smart_puts (SL_Screen[i].neew, SL_Screen[i].old, Screen_Cols, i);

	SLMEMCPY ((char *) SL_Screen[i].old, (char *) SL_Screen[i].neew, 
		  Screen_Cols * sizeof (short));

	SL_Screen[i].flags = 0;
#ifndef pc_system
	SL_Screen[i].old_hash = SL_Screen[i].new_hash;
#endif
     }
   
   if (point_visible (1)) SLtt_goto_rc (This_Row - Start_Row, This_Col - Start_Col);
   SLtt_flush_output ();
   Cls_Flag = 0;
}

static int compute_clip (int row, int n, int box_start, int box_end,
			 int *rmin, int *rmax)
{
   int row_max;
   
   if (n < 0) return 0;
   if (row >= box_end) return 0;
   row_max = row + n;
   if (row_max <= box_start) return 0;
   
   if (row < box_start) row = box_start;
   if (row_max >= box_end) row_max = box_end;
   *rmin = row;
   *rmax = row_max;
   return 1;
}

void SLsmg_touch_lines (int row, int n)
{
   int i;
   int r1, r2;
   
   if (0 == compute_clip (row, n, Start_Row, Start_Row + Screen_Rows, &r1, &r2))
     return;
   
   r1 -= Start_Row;
   r2 -= Start_Row;
   for (i = r1; i < r2; i++)
     {
	SL_Screen[i].flags |= TRASHED;
     }
}

#ifndef pc_system
static char Fake_Alt_Char_Pairs [] = "a:j+k+l+m+q-t+u+v+w+x|";

static void init_alt_char_set (void)
{
   int i;
   unsigned char *p, *pmax, ch;
   
   if (Alt_Char_Set[128] == 128) return;

   i = 32;
   memset ((char *)Alt_Char_Set, ' ', i);
   while (i <= 128) 
     {
	Alt_Char_Set [i] = i;
	i++;
     }
   
   /* Map to VT100 */
   if (SLtt_Has_Alt_Charset)
     {
	p = (unsigned char *) SLtt_Graphics_Char_Pairs;
	if (p == NULL) return;
     }
   else	p = (unsigned char *) Fake_Alt_Char_Pairs;
   pmax = p + strlen ((char *) p);
   
   /* Some systems have messed up entries for this */
   while (p < pmax)
     {
	ch = *p++;
	ch &= 0x7F;		       /* should be unnecessary */
	Alt_Char_Set [ch] = *p;
	p++;
     }
}
#endif

#ifndef pc_system
# define BLOCK_SIGNALS SLsig_block_signals ();
# define UNBLOCK_SIGNALS SLsig_unblock_signals ();
#else
# define BLOCK_SIGNALS
# define UNBLOCK_SIGNALS
#endif

static int Smg_Suspended;
void SLsmg_suspend_smg (void)
{
   BLOCK_SIGNALS

   if (Smg_Suspended == 0)
     {
	SLtt_reset_video ();
	Smg_Suspended = 1;
     }
   
   UNBLOCK_SIGNALS
}

void SLsmg_resume_smg (void)
{
   int i;
   BLOCK_SIGNALS

   if (Smg_Suspended == 0) 
     {
	UNBLOCK_SIGNALS
	return;
     }
   
   Smg_Suspended = 0;
   SLtt_init_video ();
   Cls_Flag = 1;
   for (i = 0; i < Screen_Rows; i++)
     SL_Screen[i].flags |= TRASHED;
   SLsmg_refresh ();
   
   UNBLOCK_SIGNALS
}

int SLsmg_init_smg (void)
{
   int i, len;
   unsigned short *old, *neew;
   BLOCK_SIGNALS
     
   if (Smg_Inited) SLsmg_reset_smg ();
   SLtt_init_video ();
   Screen_Cols = SLtt_Screen_Cols;
   Screen_Rows = SLtt_Screen_Rows;
   This_Col = This_Row = Start_Col = Start_Row = 0;

   This_Color = 0;
   This_Alt_Char = 0;
   Cls_Flag = 1;
#ifndef pc_system
   init_alt_char_set ();
#endif
   len = Screen_Cols + 3;
   for (i = 0; i < Screen_Rows; i++)
     {
	if ((NULL == (old = (unsigned short *) SLMALLOC (sizeof(short) * len)))
	    || ((NULL == (neew = (unsigned short *) SLMALLOC (sizeof(short) * len)))))
	  {
	     SLang_Error = SL_MALLOC_ERROR;
	     UNBLOCK_SIGNALS
	     return 0;
	  }
	blank_line (old, len, ' ');
	blank_line (neew, len, ' ');
	SL_Screen[i].old = old;
	SL_Screen[i].neew = neew;
	SL_Screen[i].flags = 0;
#ifndef pc_system
	Blank_Hash = compute_hash (old, Screen_Cols);
	SL_Screen[i].new_hash = SL_Screen[i].old_hash =  Blank_Hash;
#endif
     }
   Smg_Inited = 1;
   UNBLOCK_SIGNALS
   return 1;
}


void SLsmg_reset_smg (void)
{
   int i;
   BLOCK_SIGNALS
     
   if (Smg_Inited == 0) 
     {
	UNBLOCK_SIGNALS
	return;
     }
   for (i = 0; i < Screen_Rows; i++)
     {
	if (SL_Screen[i].old != NULL) SLFREE (SL_Screen[i].old);
	if (SL_Screen[i].neew != NULL) SLFREE (SL_Screen[i].neew);
	SL_Screen[i].old = SL_Screen[i].neew = NULL;
     }
   SLtt_reset_video ();
   This_Alt_Char = This_Color = 0;
   Smg_Inited = 0;
   
   UNBLOCK_SIGNALS
}


unsigned short SLsmg_char_at (void)
{
   if (point_visible (1))
     {
	return SL_Screen[This_Row - Start_Row].neew[This_Col - Start_Col];
     }
   return 0;
}


void SLsmg_vprintf (char *fmt, va_list ap)
{
   char p[1000];
   
   (void) vsprintf(p, fmt, ap);
   
   SLsmg_write_string (p);
}

void SLsmg_set_screen_start (int *r, int *c)
{
   int or = Start_Row, oc = Start_Col;
   
   if (c == NULL) Start_Col = 0;
   else
     {
	Start_Col = *c;
	*c = oc;
     }
   if (r == NULL) Start_Row = 0;
   else
     {
	Start_Row = *r;
	*r = or;
     }
}

void SLsmg_draw_object (int r, int c, unsigned char object)
{
   This_Row = r;  This_Col = c;
   
   if (point_visible (1))
     {
	int color = This_Color;
	This_Color |= ALT_CHAR_FLAG;
	SLsmg_write_char (object);
	This_Color = color;
     }

   This_Col = c + 1;
}

void SLsmg_draw_hline (int n)
{
   static unsigned char hbuf[16];
   int count;
   int cmin, cmax;
   int final_col = This_Col + n;
   int save_color;
   
   if ((This_Row < Start_Row) || (This_Row >= Start_Row + Screen_Rows) 
       || (0 == compute_clip (This_Col, n, Start_Col, Start_Col + Screen_Cols,
			      &cmin, &cmax)))
     {
	This_Col = final_col;
	return;
     }
   
   if (hbuf[0] == 0)
     {
	SLMEMSET ((char *) hbuf, SLSMG_HLINE_CHAR, 16);
     }
   
   n = cmax - cmin;
   count = n / 16;
   
   save_color = This_Color;
   This_Color |= ALT_CHAR_FLAG;
   This_Col = cmin;
   
   SLsmg_write_nchars ((char *) hbuf, n % 16);
   while (count-- > 0)
     {
	SLsmg_write_nchars ((char *) hbuf, 16);
     }
   
   This_Color = save_color;
   This_Col = final_col;
}

void SLsmg_draw_vline (int n)
{
   unsigned char ch = SLSMG_VLINE_CHAR;
   int c = This_Col, rmin, rmax;
   int final_row = This_Row + n;
   int save_color;
   
   if (((c < Start_Col) || (c >= Start_Col + Screen_Cols)) ||
       (0 == compute_clip (This_Row, n, Start_Row, Start_Row + Screen_Rows,
			  &rmin, &rmax)))
     {
	This_Row = final_row;
	return;
     }
   
   save_color = This_Color;
   This_Color |= ALT_CHAR_FLAG;
    
   for (This_Row = rmin; This_Row < rmax; This_Row++)
     {
	This_Col = c;
	SLsmg_write_nchars ((char *) &ch, 1);
     }
   
   This_Col = c;  This_Row = final_row;
   This_Color = save_color;
}

void SLsmg_draw_box (int r, int c, int dr, int dc)
{
   if (!dr || !dc) return; 
   This_Row = r;  This_Col = c;
   dr--; dc--;
   SLsmg_draw_hline (dc);  
   SLsmg_draw_vline (dr);
   This_Row = r;  This_Col = c;
   SLsmg_draw_vline (dr);
   SLsmg_draw_hline (dc);   
   SLsmg_draw_object (r, c, SLSMG_ULCORN_CHAR);
   SLsmg_draw_object (r, c + dc, SLSMG_URCORN_CHAR);
   SLsmg_draw_object (r + dr, c, SLSMG_LLCORN_CHAR);
   SLsmg_draw_object (r + dr, c + dc, SLSMG_LRCORN_CHAR);
   This_Row = r; This_Col = c;
}
   
void SLsmg_fill_region (int r, int c, int dr, int dc, unsigned char ch)
{
   static unsigned char hbuf[16];
   int count;
   int dcmax, rmax;
   
   
   if ((dc < 0) || (dr < 0)) return;
   
   SLsmg_gotorc (r, c);
   r = This_Row; c = This_Col;
   
   dcmax = Screen_Cols - This_Col;
   if (dc > dcmax) dc = dcmax;
   
   rmax = This_Row + dr;
   if (rmax > Screen_Rows) rmax = Screen_Rows;

#if 0
   ch = Alt_Char_Set[ch];
#endif
   if (ch != hbuf[0]) SLMEMSET ((char *) hbuf, (char) ch, 16);
   
   for (This_Row = r; This_Row < rmax; This_Row++)
     {
	This_Col = c;
	count = dc / 16;
	SLsmg_write_nchars ((char *) hbuf, dc % 16);
	while (count-- > 0)
	  {
	     SLsmg_write_nchars ((char *) hbuf, 16);
	  }
     }
   
   This_Row = r;
}

void SLsmg_forward (int n)
{
   This_Col += n;
}

void SLsmg_write_color_chars (unsigned short *s, unsigned int len)
{
   unsigned short *smax, sh;
   char buf[32], *b, *bmax;
   int color, save_color;
   
   smax = s + len;
   b = buf;
   bmax = b + sizeof (buf);
   
   save_color = This_Color;

   while (s < smax)
     {
	sh = *s++;
	
	color = sh >> 8;
	if ((color != This_Color) || (b == bmax))
	  {
	     if (b != buf) 
	       {
		  SLsmg_write_nchars (buf, (int) (b - buf));
		  b = buf;
	       }
	     This_Color = color;
	  }
	*b++ = (char) (sh & 0xFF);
     }
   
   if (b != buf)
     SLsmg_write_nchars (buf, (int) (b - buf));
	
   This_Color = save_color;
}

unsigned int SLsmg_read_raw (unsigned short *buf, unsigned int len)
{
   unsigned int r, c;

   if (0 == point_visible (1)) return 0;
   
   r = (unsigned int) (This_Row - Start_Row);
   c = (unsigned int) (This_Col - Start_Col);
   
   if (c + len > (unsigned int) Screen_Cols)
     len = (unsigned int) Screen_Cols - c;
   
   memcpy ((char *) buf, (char *) (SL_Screen[r].neew + c), len * sizeof (short));
   return len;
}

unsigned int SLsmg_write_raw (unsigned short *buf, unsigned int len)
{
   unsigned int r, c;
   unsigned short *dest;
   
   if (0 == point_visible (1)) return 0;
   
   r = (unsigned int) (This_Row - Start_Row);
   c = (unsigned int) (This_Col - Start_Col);
   
   if (c + len > (unsigned int) Screen_Cols)
     len = (unsigned int) Screen_Cols - c;
   
   dest = SL_Screen[r].neew + c;
   
   if (0 != memcmp ((char *) dest, (char *) buf, len * sizeof (short)))
     {	
	memcpy ((char *) dest, (char *) buf, len * sizeof (short));
	SL_Screen[r].flags |= TOUCHED;
     }
   return len;
}
