/* Keyboard support routines.
	for Windows NT system.

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

  Bugs:
    Have trouble with non-US keyboards, "Alt-gr"+keys (API tells CTRL-ALT is pressed)
   */
#include <config.h>
#ifndef _OS_NT
#error This file is for Win32 systems.
#else

#include <windows.h>
#include <stdio.h>
#include "../src/mouse.h"
#include "../src/global.h"
#include "../src/main.h"
#include "../src/key.h"
#include "../vfs/vfs.h"
#include "../src/tty.h"
#include "trace_nt.h"

/*  Global variables */
int old_esc_mode = 0;
HANDLE hConsoleInput;
DWORD  dwSaved_ControlState;
Gpm_Event evSaved_Event;

/* Unused variables */
int double_click_speed;		/* they are here to keep linker happy */
int mou_auto_repeat;
int use_8th_bit_as_meta = 0;

/* Static Tables */
struct {
    int key_code;
    int vkcode;
} key_table [] = {
    { KEY_F(1),  VK_F1 },
    { KEY_F(2),  VK_F2 },
    { KEY_F(3),  VK_F3 },
    { KEY_F(4),  VK_F4 },
    { KEY_F(5),  VK_F5 },
    { KEY_F(6),  VK_F6 },
    { KEY_F(7),  VK_F7 },
    { KEY_F(8),  VK_F8 },
    { KEY_F(9),  VK_F9 },
    { KEY_F(10), VK_F10 },
    { KEY_F(11), VK_F11 },
    { KEY_F(12), VK_F12 },
    { KEY_F(13), VK_F13 }, 
    { KEY_F(14), VK_F14 },
    { KEY_F(15), VK_F15 },
    { KEY_F(16), VK_F16 },
    { KEY_F(17), VK_F17 },
    { KEY_F(18), VK_F18 },
    { KEY_F(19), VK_F19 },
    { KEY_F(20), VK_F20 },	
    { KEY_IC,    VK_INSERT },		
    { KEY_DC,    VK_DELETE },
    { KEY_BACKSPACE, VK_BACK },

    { KEY_PPAGE, VK_PRIOR },
    { KEY_NPAGE, VK_NEXT },
    { KEY_LEFT,  VK_LEFT },
    { KEY_RIGHT, VK_RIGHT },
    { KEY_UP,    VK_UP },
    { KEY_DOWN,  VK_DOWN },
    { KEY_HOME,  VK_HOME },
    { KEY_END,	 VK_END },

    { ALT('*'),  VK_MULTIPLY },
    { ALT('+'),  VK_ADD },
    { ALT('-'),  VK_SUBTRACT },
    
    { ALT('\t'), VK_PAUSE }, /* Added to make Complete work press Pause */

    { ESC_CHAR, VK_ESCAPE },

    { 0, 0}
};		

/*  init_key  - Called in main.c to initialize ourselves
		Get handle to console input
*/
void init_key (void)
{
    win32APICALL_HANDLE (hConsoleInput, GetStdHandle (STD_INPUT_HANDLE));
}

int ctrl_pressed ()
{
    if(dwSaved_ControlState & RIGHT_ALT_PRESSED) return 0; 
    /* The line above fixes the BUG with the AltGr Keys*/
    return dwSaved_ControlState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED);
}

int shift_pressed ()
{
    if(dwSaved_ControlState & RIGHT_ALT_PRESSED) return 0; 
    /* The line above fixes the BUG with the AltGr Keys*/
    return dwSaved_ControlState & SHIFT_PRESSED;
}

int alt_pressed ()
{
    return dwSaved_ControlState & (/* RIGHT_ALT_PRESSED | */ LEFT_ALT_PRESSED);
}

static int VKtoCurses (int a_vkc)
{
    int i;

    for (i = 0; key_table[i].vkcode != 0; i++) 
	if (a_vkc == key_table[i].vkcode) {
	    return key_table[i].key_code;
	}
    return 0;
}

static int translate_key_code(int asc, int scan)
{
    int c;
    switch(scan){
    case 106: /* KP_MULT*/
 	return ALT('*');
    case 107: /* KP_PLUS*/
 	return ALT('+');
    case 109: /* KP_MINUS*/
 	return ALT('-');
    }
    c = VKtoCurses (scan);
    if (!asc && !c)
	return 0;
    if (asc && c)
	return c;
    if (!asc || asc=='\t' )
    {
	if (shift_pressed() && (c >= KEY_F(1)) && (c <= KEY_F(10)))
	    c += 10;
	if (alt_pressed() && (c >= KEY_F(1)) && (c <= KEY_F(2)))
	    c += 10;
	if (alt_pressed() && (c == KEY_F(7)))
	    c = ALT('?');
  	if (asc == '\t'){
	    if(ctrl_pressed())c = ALT('\t');
  	    else c=asc;
  	}
	return c;
    }
    if (ctrl_pressed())
	return XCTRL(asc);
    if (alt_pressed())
	return ALT(asc);
    if (asc == 13)
	return 10;
    return asc;
}

int get_key_code (int no_delay)
{
    INPUT_RECORD ir;				/* Input record */
    DWORD		 dw;				/* number of records actually read */
    int			 ch, vkcode, j;

    if (no_delay) {
        /* Check if any input pending, otherwise return */
	nodelay (stdscr, TRUE);
	win32APICALL(PeekConsoleInput(hConsoleInput, &ir, 1, &dw));
	if (!dw)
	    return 0;
    }
 
    do {
	win32APICALL(ReadConsoleInput(hConsoleInput, &ir, 1, &dw));
	switch (ir.EventType) {
 	    case KEY_EVENT:
		if (!ir.Event.KeyEvent.bKeyDown)		/* Process key just once: when pressed */
		    break;

		vkcode = ir.Event.KeyEvent.wVirtualKeyCode;
//#ifndef __MINGW32__
		ch = ir.Event.KeyEvent.uChar.AsciiChar;
//#else
//		ch = ir.Event.KeyEvent.AsciiChar;
//#endif
		dwSaved_ControlState = ir.Event.KeyEvent.dwControlKeyState;
		j = translate_key_code (ch, vkcode);
		if (j)
		    return j;
		break;

	case MOUSE_EVENT:
		/* Save event as a GPM-like event */
		evSaved_Event.x = ir.Event.MouseEvent.dwMousePosition.X;
		evSaved_Event.y = ir.Event.MouseEvent.dwMousePosition.Y+1;
		evSaved_Event.buttons = ir.Event.MouseEvent.dwButtonState;
		switch (ir.Event.MouseEvent.dwEventFlags) {
		    case 0:
			evSaved_Event.type = GPM_DOWN | GPM_SINGLE;
			break;
		    case MOUSE_MOVED:
			evSaved_Event.type = GPM_MOVE;
			break;
		    case DOUBLE_CLICK:
			evSaved_Event.type = GPM_DOWN | GPM_DOUBLE;
			break;
		};
		return 0;	
	}
    } while (!no_delay);
    return 0;
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

/* Returns a character read from stdin with appropriate interpretation */
int get_event (Gpm_Event *event, int redo_event, int block)
{
    int c;
    static int flag;			/* Return value from select */
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
        return EV_NONE; /* FIXME: mouse not supported */
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
    DWORD dw;
    if (GetNumberOfConsoleInputEvents (hConsoleInput, &dw))
	if (dw > 15)
 	    return 0;
    return 1;
}

/* get_modifier  */
int get_modifier()
{
    int  retval = 0;

    if (dwSaved_ControlState & LEFT_ALT_PRESSED)        /* code is not clean, because we return Linux-like bitcodes*/
	retval |= ALTL_PRESSED;
    if (dwSaved_ControlState & RIGHT_ALT_PRESSED)
	retval |= ALTR_PRESSED;

    if (dwSaved_ControlState & RIGHT_CTRL_PRESSED ||
	dwSaved_ControlState & LEFT_CTRL_PRESSED)
	retval |= CONTROL_PRESSED;

    if (dwSaved_ControlState & SHIFT_PRESSED)
	retval |= SHIFT_PRESSED;

    return retval;
}

/* void functions for UNIX compatibility */
void define_sequence (int code, char* vkcode, int action) {}
void channels_up() {}
void channels_down() {}
void init_key_input_fd (void) {}
void numeric_keypad_mode (void) {}
void application_keypad_mode (void) {}

/* mouse is not yet supported, sorry */
void init_mouse (void) {}
void shut_mouse (void) {}

#endif /* _OS_NT */
