/* ----------- dflatmsg.h ------------ */

/*
 * message foundation file
 * make message changes here
 * other source files will adapt
 */

/* -------------- process communication messages ----------- */
DFlatMsg(DFM_START)              /* start message processing     */
DFlatMsg(DFM_STOP)               /* stop message processing      */
DFlatMsg(DFM_COMMAND)            /* send a command to a window   */
/* -------------- window management messages --------------- */
DFlatMsg(CREATE_WINDOW)      /* create a window              */
DFlatMsg(SHOW_WINDOW)        /* show a window                */
DFlatMsg(DFM_HIDE_WINDOW)        /* hide a window                */
DFlatMsg(CLOSE_WINDOW)       /* delete a window              */
DFlatMsg(SETFOCUS)           /* set and clear the focus      */
DFlatMsg(PAINT)              /* paint the window's data space*/
DFlatMsg(BORDER)             /* paint the window's border    */
DFlatMsg(TITLE)              /* display the window's title   */
DFlatMsg(MOVE)               /* move the window              */
DFlatMsg(DFM_SIZE)               /* change the window's size     */
#ifdef INCLUDE_MAXIMIZE
DFlatMsg(MAXIMIZE)           /* maximize the window          */
#endif
#ifdef INCLUDE_MINIMIZE
DFlatMsg(MINIMIZE)           /* minimize the window          */
#endif
DFlatMsg(RESTORE)            /* restore the window           */
DFlatMsg(INSIDE_WINDOW)      /* test x/y inside a window     */
/* ---------------- clock messages ------------------------- */
DFlatMsg(CLOCKTICK)          /* the clock ticked             */
DFlatMsg(CAPTURE_CLOCK)      /* capture clock into a window  */
DFlatMsg(RELEASE_CLOCK)      /* release clock to the system  */
/* -------------- keyboard and screen messages ------------- */
DFlatMsg(KEYBOARD)           /* key was pressed              */
DFlatMsg(CAPTURE_KEYBOARD) /* capture keyboard into a window */
DFlatMsg(RELEASE_KEYBOARD)   /* release keyboard to system   */
DFlatMsg(KEYBOARD_CURSOR)    /* position the keyboard cursor */
DFlatMsg(CURRENT_KEYBOARD_CURSOR) /*read the cursor position */
DFlatMsg(HIDE_CURSOR)        /* hide the keyboard cursor     */
DFlatMsg(SHOW_CURSOR)        /* display the keyboard cursor  */
DFlatMsg(SAVE_CURSOR)      /* save the cursor's configuration*/
DFlatMsg(RESTORE_CURSOR)     /* restore the saved cursor     */
DFlatMsg(SHIFT_CHANGED)      /* the shift status changed     */
DFlatMsg(WAITKEYBOARD)     /* waits for a key to be released */

/* ---------------- mouse messages ------------------------- */
DFlatMsg(MOUSE_TRAVEL)       /* set the mouse travel         */
DFlatMsg(RIGHT_BUTTON)       /* right button pressed         */
DFlatMsg(LEFT_BUTTON)        /* left button pressed          */
DFlatMsg(DFM_DOUBLE_CLICK)       /* left button double-clicked   */
DFlatMsg(DFM_MOUSE_MOVED)        /* mouse changed position       */
DFlatMsg(DFM_BUTTON_RELEASED)    /* mouse button released        */
DFlatMsg(WAITMOUSE)          /* wait until button released   */
DFlatMsg(TESTMOUSE)          /* test any mouse button pressed*/
DFlatMsg(CAPTURE_MOUSE)      /* capture mouse into a window  */
DFlatMsg(RELEASE_MOUSE)      /* release the mouse to system  */

/* ---------------- text box messages ---------------------- */
DFlatMsg(ADDTEXT)            /* append text to the text box  */
DFlatMsg(INSERTTEXT)		 /* insert line of text          */
DFlatMsg(DELETETEXT)         /* delete line of text          */
DFlatMsg(CLEARTEXT)          /* clear the edit box           */
DFlatMsg(SETTEXT)            /* copy text to text buffer     */
DFlatMsg(SCROLL)             /* vertical line scroll         */
DFlatMsg(HORIZSCROLL)        /* horizontal column scroll     */
DFlatMsg(SCROLLPAGE)         /* vertical page scroll         */
DFlatMsg(HORIZPAGE)          /* horizontal page scroll       */
DFlatMsg(SCROLLDOC)          /* scroll to beginning/end      */
/* ---------------- edit box messages ---------------------- */
DFlatMsg(GETTEXT)            /* get text from an edit box    */
DFlatMsg(SETTEXTLENGTH)		 /* set maximum text length      */
/* ---------------- menubar messages ----------------------- */
DFlatMsg(BUILDMENU)          /* build the menu display       */
DFlatMsg(MB_SELECTION)       /* menubar selection            */
/* ---------------- popdown messages ----------------------- */
DFlatMsg(BUILD_SELECTIONS)   /* build the menu display       */
DFlatMsg(CLOSE_POPDOWN)    /* tell parent popdown is closing */
/* ---------------- list box messages ---------------------- */
DFlatMsg(LB_SELECTION)       /* sent to parent on selection  */
DFlatMsg(LB_CHOOSE)          /* sent when user chooses       */
DFlatMsg(LB_CURRENTSELECTION)/* return the current selection */
DFlatMsg(DFM_LB_GETTEXT)         /* return the text of selection */
DFlatMsg(LB_SETSELECTION)    /* sets the listbox selection   */
/* ---------------- dialog box messages -------------------- */
DFlatMsg(INITIATE_DIALOG)    /* begin a dialog               */
DFlatMsg(ENTERFOCUS)         /* tell DB control got focus    */
DFlatMsg(LEAVEFOCUS)         /* tell DB control lost focus   */
DFlatMsg(ENDDIALOG)          /* end a dialog                 */
/* ---------------- help box messages ---------------------- */
DFlatMsg(DISPLAY_HELP)
/* --------------- application window messages ------------- */
DFlatMsg(ADDSTATUS)
/* --------------- picture box messages -------------------- */
DFlatMsg(DRAWVECTOR)
DFlatMsg(DRAWBOX)
DFlatMsg(DRAWBAR)

