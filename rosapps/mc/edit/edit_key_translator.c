/*
    these #defines are probably the ones most people will be interested in.
    You can use these two #defines to hard code the key mappings --- just
    uncomment the one you want. But only if you have trouble with learn
    keys (which is unlikely).
*/

    /* KEY_BACKSPACE is the key learned in the learn keys menu : */
#define OUR_BACKSPACE_KEY KEY_BACKSPACE
    /* ...otherwise ctrl-h : */
/* #define OUR_BACKSPACE_KEY XCTRL ('h') */
    /* ...otherwise 127 or DEL in ascii : */
/* #define OUR_BACKSPACE_KEY 0177 */

    /* KEY_DC is the key learned in the learn keys menu */
#define OUR_DELETE_KEY KEY_DC
    /* ...otherwise ctrl-d : */
/* #define OUR_DELETE_KEY XCTRL ('d') */
    /* ...otherwise 127 or DEL in ascii : */
/* #define OUR_DELETE_KEY 0177 */


/*
   This is #include'd into the function edit_translate_key in edit.c.
   This sequence of code takes 'x_state' and 'x_key' and translates them
   into either 'command' or 'char_for_insertion'. 'x_key' holds one of
   KEY_NPAGE, KEY_HOME etc., and 'x_state' holds a bitwise inclusive OR of
   CONTROL_PRESSED, ALT_PRESSED or SHIFT_PRESSED, although none may
   be supported.
   'command' is one of the editor commands editcmddef.h.

   Almost any C code can go into this file. The code below is an example
   that may by appended or modified by the user.
 */

/* look in this file for the list of commands : */
#include "editcmddef.h"

#define KEY_NUMLOCK ???

/* ordinary translations. (Some of this may be redundant.) Note that keys listed
   first take priority when a key is assigned to more than one command */
    static long *key_map;
    static long cooledit_key_map[] =
    {OUR_BACKSPACE_KEY, CK_BackSpace, OUR_DELETE_KEY, CK_Delete,
     XCTRL ('d'), CK_Delete, '\n', CK_Enter,
     KEY_PPAGE, CK_Page_Up, KEY_NPAGE, CK_Page_Down, KEY_LEFT, CK_Left,
     KEY_RIGHT, CK_Right, KEY_UP, CK_Up, KEY_DOWN, CK_Down, ALT ('\t'), CK_Return, ALT ('\n'), CK_Return,
     KEY_HOME, CK_Home, KEY_END, CK_End, '\t', CK_Tab, XCTRL ('u'), CK_Undo, KEY_IC, CK_Toggle_Insert,
     XCTRL ('o'), CK_Load, KEY_F (3), CK_Mark, KEY_F (5), CK_Copy,
     KEY_F (6), CK_Move, KEY_F (8), CK_Remove, KEY_F (12), CK_Save_As,
     KEY_F (2), CK_Save, XCTRL ('n'), CK_New,
     XCTRL ('l'), CK_Refresh, ESC_CHAR, CK_Exit, KEY_F (10), CK_Exit,
     KEY_F (19), /*C formatter */ CK_Pipe_Block (0),
     XCTRL ('p'), /*spell check */ CK_Pipe_Block (1),
     KEY_F (15), CK_Insert_File,
     XCTRL ('f'), CK_Save_Block, KEY_F (1), CK_Help,
     ALT ('t'), CK_Sort, ALT ('m'), CK_Mail,
     XCTRL ('z'), CK_Word_Left, XCTRL ('x'), CK_Word_Right,
     KEY_F (4), CK_Replace, KEY_F (7), CK_Find, KEY_F (14), CK_Replace_Again,
     XCTRL ('h'), CK_BackSpace, ALT ('l'), CK_Goto, ALT ('L'), CK_Goto, XCTRL ('y'), CK_Delete_Line,
     KEY_F (17), CK_Find_Again, ALT ('p'), CK_Paragraph_Format, 0};

    static long emacs_key_map[] =
    {OUR_BACKSPACE_KEY, CK_BackSpace, OUR_DELETE_KEY, CK_Delete, '\n', CK_Enter,
     KEY_PPAGE, CK_Page_Up, KEY_NPAGE, CK_Page_Down, KEY_LEFT, CK_Left,
     KEY_RIGHT, CK_Right, KEY_UP, CK_Up, KEY_DOWN, CK_Down, ALT ('\t'), CK_Return, ALT ('\n'), CK_Return,
     KEY_HOME, CK_Home, KEY_END, CK_End, '\t', CK_Tab, XCTRL ('u'), CK_Undo, KEY_IC, CK_Toggle_Insert,
     XCTRL ('o'), CK_Load, KEY_F (3), CK_Mark, KEY_F (5), CK_Copy,
     KEY_F (6), CK_Move, KEY_F (8), CK_Remove, KEY_F (12), CK_Save_As,
     KEY_F (2), CK_Save, ALT ('p'), CK_Paragraph_Format,

     ALT ('t'), CK_Sort,

     XCTRL ('a'), CK_Home, XCTRL ('e'), CK_End,
     XCTRL ('b'), CK_Left, XCTRL ('f'), CK_Right,
     XCTRL ('n'), CK_Down, XCTRL ('p'), CK_Up,
     XCTRL ('d'), CK_Delete,
     XCTRL ('v'), CK_Page_Down, ALT ('v'), CK_Page_Up,
     XCTRL ('@'), CK_Mark,
     XCTRL ('k'), CK_Delete_To_Line_End,
     XCTRL ('s'), CK_Find,

     ALT ('b'), CK_Word_Left, ALT ('f'), CK_Word_Right,
     XCTRL ('w'), CK_XCut,
     XCTRL ('y'), CK_XPaste,
     ALT ('w'), CK_XStore,

     XCTRL ('l'), CK_Refresh, ESC_CHAR, CK_Exit, KEY_F (10), CK_Exit,
     KEY_F (19), /*C formatter */ CK_Pipe_Block (0),
     ALT ('$'), /*spell check */ CK_Pipe_Block (1),
     KEY_F (15), CK_Insert_File,
     KEY_F (1), CK_Help,

     KEY_F (4), CK_Replace, KEY_F (7), CK_Find, KEY_F (14), CK_Replace_Again,
     XCTRL ('h'), CK_BackSpace, ALT ('l'), CK_Goto, ALT ('L'), CK_Goto,
     KEY_F (17), CK_Find_Again,
     ALT ('<'), CK_Beginning_Of_Text,
     ALT ('>'), CK_End_Of_Text,
     
     0, 0};

    static long key_pad_map[10] =
    {XCTRL ('o'), KEY_END, KEY_DOWN, KEY_NPAGE, KEY_LEFT,
     KEY_DOWN, KEY_RIGHT, KEY_HOME, KEY_UP, KEY_PPAGE};


#define DEFAULT_NUM_LOCK        0

    static int num_lock = DEFAULT_NUM_LOCK;
    int i = 0;

    switch (edit_key_emulation) {
    case EDIT_KEY_EMULATION_NORMAL:
	key_map = cooledit_key_map;
	break;
    case EDIT_KEY_EMULATION_EMACS:
	key_map = emacs_key_map;
	if (x_key == XCTRL ('x')) {
	    int ext_key;
	    ext_key = edit_raw_key_query (" Ctrl-X ", _(" Emacs key: "), 0);
	    switch (ext_key) {
	    case 's':
		command = CK_Save;
		goto fin;
	    case 'x':
		command = CK_Exit;
		goto fin;
	    case 'k':
		command = CK_New;
		goto fin;
	    case 'e':
		command = CK_Macro (edit_raw_key_query (_(" Execute Macro "), _(" Press macro hotkey: "), 1));
		if (command == CK_Macro (0))
		    command = -1;
		goto fin;
	    }
	    goto fin;
	}
	break;
    }

    if (x_key == XCTRL ('q')) {
	char_for_insertion = edit_raw_key_query (_(" Insert Literal "), _(" Press any key: "), 0);
	goto fin;
    }
    if (x_key == XCTRL ('a') && edit_key_emulation != EDIT_KEY_EMULATION_EMACS) {
	command = CK_Macro (edit_raw_key_query (" Execute Macro ", " Press macro hotkey: ", 1));
	if (command == CK_Macro (0))
	    command = -1;
	goto fin;
    }
/* edit is a pointer to the widget */
    if (edit)
	if (x_key == XCTRL ('r')) {
	    command = edit->macro_i < 0 ? CK_Begin_Record_Macro : CK_End_Record_Macro;
	    goto fin;
	}
/*    if (x_key == KEY_NUMLOCK) {
   num_lock = 1 - num_lock;
   return 1;
   }
 */

/* first translate the key-pad */
    if (num_lock) {
	if (x_key >= '0' && x_key <= '9') {
	    x_key = key_pad_map[x_key - '0'];
	}
	if (x_key == '.') {
	    x_key = KEY_DC;
	}
    }
    if ((x_state & SHIFT_PRESSED) && (x_state & CONTROL_PRESSED)) {
	switch (x_key) {
	case KEY_PPAGE:
	    command = CK_Beginning_Of_Text_Highlight;
	    goto fin;
	case KEY_NPAGE:
	    command = CK_End_Of_Text_Highlight;
	    goto fin;
	case KEY_LEFT:
	    command = CK_Word_Left_Highlight;
	    goto fin;
	case KEY_RIGHT:
	    command = CK_Word_Right_Highlight;
	    goto fin;
	}
    }
    if ((x_state & SHIFT_PRESSED) && !(x_state & CONTROL_PRESSED)) {
	switch (x_key) {
	case KEY_PPAGE:
	    command = CK_Page_Up_Highlight;
	    goto fin;
	case KEY_NPAGE:
	    command = CK_Page_Down_Highlight;
	    goto fin;
	case KEY_LEFT:
	    command = CK_Left_Highlight;
	    goto fin;
	case KEY_RIGHT:
	    command = CK_Right_Highlight;
	    goto fin;
	case KEY_UP:
	    command = CK_Up_Highlight;
	    goto fin;
	case KEY_DOWN:
	    command = CK_Down_Highlight;
	    goto fin;
	case KEY_HOME:
	    command = CK_Home_Highlight;
	    goto fin;
	case KEY_END:
	    command = CK_End_Highlight;
	    goto fin;
	case KEY_IC:
	    command = CK_XPaste;
	    goto fin;
	case KEY_DC:
	    command = CK_XCut;
	    goto fin;
	}
    }
/* things that need a control key */
    if (x_state & CONTROL_PRESSED) {
	switch (x_key) {
	case KEY_F (2):
	    command = CK_Save_As;
	    goto fin;
	case KEY_F (4):
	    command = CK_Replace_Again;
	    goto fin;
	case KEY_F (7):
	    command = CK_Find_Again;
	    goto fin;
	case KEY_BACKSPACE:
	    command = CK_Undo;
	    goto fin;
	case KEY_PPAGE:
	    command = CK_Beginning_Of_Text;
	    goto fin;
	case KEY_NPAGE:
	    command = CK_End_Of_Text;
	    goto fin;
	case KEY_UP:
	    command = CK_Scroll_Up;
	    goto fin;
	case KEY_DOWN:
	    command = CK_Scroll_Down;
	    goto fin;
	case KEY_LEFT:
	    command = CK_Word_Left;
	    goto fin;
	case KEY_RIGHT:
	    command = CK_Word_Right;
	    goto fin;
	case KEY_IC:
	    command = CK_XStore;
	    goto fin;
	case KEY_DC:
	    command = CK_Remove;
	    goto fin;
	}
    }
/* an ordinary insertable character */
    if (x_key < 256 && is_printable (x_key)) {
	char_for_insertion = x_key;
	goto fin;
    }
/* other commands */
    i = 0;
    while (key_map[i] != x_key && (key_map[i] || key_map[i + 1]))
	i += 2;
    command = key_map[i + 1];
    if (command)
	goto fin;

/* Function still not found for this key, so try macro's */
/* This allows the same macro to be 
   enabled by either eg "ALT('f')" or "XCTRL('f')" or "XCTRL('a'), 'f'" */

/* key.h: #define ALT(x) (0x200 | (x)) */
    if (x_key & ALT (0)) {	/* is an alt key ? */
	command = CK_Macro (x_key - ALT (0));
	goto fin;
    }
/* key.h: #define XCTRL(x) ((x) & 31) */
    if (x_key < ' ') {		/* is a ctrl key ? */
	command = CK_Macro (x_key);
	goto fin;
    }
  fin:


