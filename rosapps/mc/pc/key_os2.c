/* Keyboard support routines.
	for OS/2 system.

   20. April 97: Alexander Dong (ado)

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

#include <config.h>
#ifndef __os2__
#error This file is for OS/2 systems.
#else

#define INCL_BASE
#define INCL_NOPM
#define INCL_VIO
#define INCL_KBD
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_WININPUT
#include <os2.h>
#include <stdio.h>
#include "../src/mouse.h"
#include "../src/global.h"
#include "../src/main.h"
#include "../src/key.h"
#include "../vfs/vfs.h"
#include "../src/tty.h"

/* Code to read keystrokes in a separate thread */

typedef struct kbdcodes {
  UCHAR ascii;
  UCHAR scan;
  USHORT shift;  /* .ado: change for mc */
} KBDCODES;

/*  Global variables */
int old_esc_mode = 0;
/* HANDLE hConsoleInput;
DWORD  dwSaved_ControlState; */
Gpm_Event evSaved_Event;

/* Unused variables */
int double_click_speed;		/* they are here to keep linker happy */
int mou_auto_repeat;
int use_8th_bit_as_meta = 0;

static int VKtoCurses (int vkcode);

/* -------------------------------------------------------------- */
/* DEFINITIONS:
      Return from SLANG: KeyCode:  0xaaaabbcc

      where: aaaa = Flags
             bb   = Scan code
             cc   = ASCII-code (if available)

   if no flags (CTRL and ALT) is set, cc will be returned.
   If CTRL is pressed, cc is already the XCTRL(code).
   case cc is:
       0xE0: The scan code will be used for the following keys:
              Insert:  0x52,      DEL:       0x53,
              Page_Up: 0x49,      Page_Down: 0x51,
              Pos1:    0x47,      Ende:      0x4F,
              Up:      0x48,      Down:      0x50,
              Left:    0x4B,      Right:     0x4D,

       0x00: The function keys are defined as:
                F1: 3b00, F2: 3c00 ... F10: 4400, F11: 8500, F12: 8600.
           With ALT-bit set:
           ALT(F1): 6800, 6900,... ALT(F10): 7100, ALT(F11): 8b00
                                                   ALT(F12): 8c00

           Mapping for ALT(key_code):
                For Mapping with normal keys, only the scan code can be 
                used. (see struct ALT_table)
        
   Special keys:
        ENTER (number block): 0xaaaaE00D
        + (number block):     0xaaaa4E2B        Normal: 1B2B
        - (number block):     0xaaaa4A2D        Normal: 352D
        * (number block):     0xaaaa372A        Normal: 1B2A
        / (number block):     0xaaaaE02F
*/
/* -------------------------------------------------------------- */
#define RIGHT_SHIFT_PRESSED             1
#define LEFT_SHIFT_PRESSED              2
#define CTRL_PRESSED                    4
#define ALT_PRESSED                     8
#define SCROLL_LOCK_MODE                16
#define NUM_LOCK_MODE                   32
#define CAPS_LOCK_MODE                  64
#define INSERT_MODE                     128
#define LEFT_CTRL_PRESSED               256
#define LEFT_ALT_PRESSED                512
#define RIGHT_CTRL_PRESSED              1024
#define RIGHT_ALT_PRESSED               2048
#define SCROLL_LOCK_PRESSED             4096
#define NUM_LOCK_PRESSED                8192
#define CAPS_LOCK_PRESSED               16384
#define SYSREQ                          32768
/* -------------------------------------------------------------- */

/* Static Tables */
struct {
    int key_code;
    int vkcode;
} fkt_table [] = {
    { KEY_F(1),  0x3B },
    { KEY_F(2),  0x3C },
    { KEY_F(3),  0x3D },
    { KEY_F(4),  0x3E },
    { KEY_F(5),  0x3F },
    { KEY_F(6),  0x40 },
    { KEY_F(7),  0x41 },
    { KEY_F(8),  0x42 },
    { KEY_F(9),  0x43 },
    { KEY_F(10), 0x44 },
    { KEY_F(11), 0x85 },
    { KEY_F(12), 0x86 },
    { 0, 0}
};		


struct {
    int key_code;
    int vkcode;
} ALT_table [] = {
    { ALT('a'),  0x1E },
    { ALT('b'),  0x30 },
    { ALT('c'),  0x2E },
    { ALT('d'),  0x20 },
    { ALT('e'),  0x12 },
    { ALT('f'),  0x21 },
    { ALT('g'),  0x22 },
    { ALT('h'),  0x23 },
    { ALT('i'),  0x17 },
    { ALT('j'),  0x24 },
    { ALT('k'),  0x25 },
    { ALT('l'),  0x26 },
    { ALT('m'),  0x32 },
    { ALT('n'),  0x31 },
    { ALT('o'),  0x18 },
    { ALT('p'),  0x19 },
    { ALT('q'),  0x10 },
    { ALT('r'),  0x13 },
    { ALT('s'),  0x1F },
    { ALT('t'),  0x14 },
    { ALT('u'),  0x16 },
    { ALT('v'),  0x2F },
    { ALT('w'),  0x11 },
    { ALT('x'),  0x2D },
    { ALT('y'),  0x15 },
    { ALT('z'),  0x2C },
    { ALT('\n'),  0x1c },
    { ALT('\n'),  0xA6 },
    { ALT(KEY_F(1)),  0x68 },
    { ALT(KEY_F(2)),  0x69 },
    { ALT(KEY_F(3)),  0x6A },
    { ALT(KEY_F(4)),  0x6B },
    { ALT(KEY_F(5)),  0x6C },
    { ALT(KEY_F(6)),  0x6D },
    { ALT(KEY_F(7)),  0x6E },
    { ALT(KEY_F(8)),  0x6F },
    { ALT(KEY_F(9)),  0x70 },
    { ALT(KEY_F(10)),  0x71 },
    { ALT(KEY_F(11)),  0x8B },
    { ALT(KEY_F(12)),  0x8C },
    { 0, 0}
};		


struct {
    int key_code;
    int vkcode;
} movement [] = {
    { KEY_IC,    0x52 },
    { KEY_DC,    0x53 },
    { KEY_PPAGE, 0x49 },
    { KEY_NPAGE, 0x51 },
    { KEY_LEFT,  0x4B },
    { KEY_RIGHT, 0x4D },
    { KEY_UP,    0x48 },
    { KEY_DOWN,  0x50 },
    { KEY_HOME,  0x47 },
    { KEY_END,	 0x4F },
    { 0, 0}
};		


/*  init_key -- to make linker happy */
void init_key (void)
{
   return;
}


/* The maximum sequence length (32 + null terminator) */
static int seq_buffer[33];
static int *seq_append = 0;

static int push_char (int c)
{
    if (!seq_append)
	seq_append = seq_buffer;
    
    if (seq_append == &(seq_buffer [sizeof (seq_buffer)-2]))
	return 0;
    *(seq_append++) = c;
    *seq_append = 0;
    return 1;
}

int get_key_code (int no_delay)
{
    unsigned int        inp_ch;

    if (no_delay) {
        /* Check if any input pending, otherwise return */
	nodelay (stdscr, TRUE);
        inp_ch = SLang_input_pending(0);
        if (inp_ch == 0) {
           return 0;
        }
    } 

    if (no_delay) {
       return (VKtoCurses(inp_ch));
    }

    do {
       inp_ch = SLang_getkey();
       if (!inp_ch)
	  inp_ch = (SLang_getkey() << 8);
       if (inp_ch) return (VKtoCurses(inp_ch));
    } while (!no_delay);
    return 0;
}

static int VKtoCurses (int a_vkc)
{
   int ctrlState = 0;
   int altState  = 0;
   
   int  fsState;
   char scanCode;
   char asciiCode;
   register int i;
   int rtnCode = 0;

   fsState = (a_vkc & 0xFFFF0000) >> 16;
   fsState &= (~INSERT_MODE);  /* Ignore Insertion mode */

   scanCode = (char) ((a_vkc & 0x0000FFFF) >> 8);
   asciiCode = (char) (a_vkc & 0x000000FF);
   
   ctrlState  = (fsState & CTRL_PRESSED);
   altState   = (fsState & ALT_PRESSED);

   rtnCode = asciiCode;

   if (ctrlState) {
      /* CTRL pressed */
      rtnCode = XCTRL(asciiCode);
   }

   if (altState) {
      /* ALT pressed
       * rtnCode = ALT(asciiCode);
       *
       * With German keyboards, the Values between 7B -> 7D
       * and 5b, 5d, 40, fd, fc and e6 are only reachable with the AltGr
       * key. If such a combination is used, asciiCode will not be zero.
       * With the normal ALT key, the asciiCode will always be zero.
       */
      if (asciiCode) {
         return asciiCode;
      }
   }

   /* Scan Movement codes */
   if (asciiCode == 0) {
      /* Replace key code with that in table */
      for (i=0;  movement[i].vkcode != 0 || movement[i].key_code != 0; i++) 
	if (scanCode == movement[i].vkcode) 
  	     return (movement[i].key_code);
   }

   if (asciiCode == 0) {
      /* Function-key detected */
      for (i=0;  fkt_table[i].vkcode != 0 || fkt_table[i].key_code != 0; i++) 
	if (scanCode == fkt_table[i].vkcode) 
  	     return (fkt_table[i].key_code);
      /* ALT - KEY */
      /* if (altState) */ {
         for (i=0;  ALT_table[i].vkcode != 0 || ALT_table[i].key_code != 0; i++) 
                if (scanCode == ALT_table[i].vkcode) 
                 	     return (ALT_table[i].key_code);
      }
   }

   if (asciiCode == 0x0d) {
      return '\n';
   }

   return rtnCode;
}


static int getch_with_delay (void)
{
    int c;

    while (1) {
	/* Try to get a character */
	c = get_key_code (0);
	if (c != ERR)
	    break;
    }
    /* Success -> return the character */
    return c;
}

int get_event (Gpm_Event *event, int redo_event, int block)
{
    int c;
    static int dirty = 3;

    if ((dirty == 1) || is_idle ()){
	refresh ();
	doupdate ();
	dirty = 1;
    } else
	dirty++;

    vfs_timeout_handler ();
    
    c = block ? getch_with_delay () : get_key_code (1);
    if (!c) {
	/* Code is 0, so this is a Control key or mouse event */
	*event = evSaved_Event;
        return EV_NONE; /* FIXME: when should we return EV_MOUSE ? */
    }

    return c;
}

/* Returns a key press, mouse events are discarded */
int mi_getch ()
{
    Gpm_Event ev;
    int       key;
    
    while ((key = get_event (&ev, 0, 1)) == 0)
	;
    return key;
}


/* 
   is_idle -    A function to check if we're idle.
		It checks for any waiting event  (that can be a Key, Mouse event, 
   		and other internal events like focus or menu) 
*/
int is_idle (void)
{
    return 1;
}

/* get_modifier  */
int get_modifier()
{
   return 0;
}

int ctrl_pressed ()
{
    return 0;
}


/* void functions for UNIX copatibility */
void define_sequence (int code, char* vkcode, int action) {}
void channels_up() {}
void channels_down() {}
void init_key_input_fd (void) {}
void numeric_keypad_mode (void) {}
void application_keypad_mode (void) {}

/* mouse is not yet supported, sorry */
void init_mouse (void) {}
void shut_mouse (void) {}

#endif /* __os2__ */
