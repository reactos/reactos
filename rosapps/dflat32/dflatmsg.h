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
DFlatMsg(DFM_CREATE_WINDOW)      /* create a window              */
DFlatMsg(DFM_SHOW_WINDOW)        /* show a window                */
DFlatMsg(DFM_HIDE_WINDOW)        /* hide a window                */
DFlatMsg(DFM_CLOSE_WINDOW)       /* delete a window              */
DFlatMsg(DFM_SETFOCUS)           /* set and clear the focus      */
DFlatMsg(DFM_PAINT)              /* paint the window's data space*/
DFlatMsg(DFM_BORDER)             /* paint the window's border    */
DFlatMsg(DFM_TITLE)              /* display the window's title   */
DFlatMsg(DFM_MOVE)               /* move the window              */
DFlatMsg(DFM_DFM_SIZE)               /* change the window's size     */
#ifdef INCLUDE_MAXIMIZE
DFlatMsg(DFM_MAXIMIZE)           /* maximize the window          */
#endif
#ifdef INCLUDE_MINIMIZE
DFlatMsg(DFM_MINIMIZE)           /* minimize the window          */
#endif
DFlatMsg(DFM_RESTORE)            /* restore the window           */
DFlatMsg(DFM_INSIDE_WINDOW)      /* test x/y inside a window     */
/* ---------------- clock messages ------------------------- */
DFlatMsg(DFM_CLOCKTICK)          /* the clock ticked             */
DFlatMsg(DFM_CAPTURE_CLOCK)      /* capture clock into a window  */
DFlatMsg(DFM_RELEASE_CLOCK)      /* release clock to the system  */
/* -------------- keyboard and screen messages ------------- */
DFlatMsg(DFM_KEYBOARD)           /* key was pressed              */
DFlatMsg(DFM_CAPTURE_KEYBOARD) /* capture keyboard into a window */
DFlatMsg(DFM_RELEASE_KEYBOARD)   /* release keyboard to system   */
DFlatMsg(DFM_KEYBOARD_CURSOR)    /* position the keyboard DfCursor */
DFlatMsg(DFM_CURRENT_KEYBOARD_CURSOR) /*read the DfCursor position */
DFlatMsg(DFM_HIDE_CURSOR)        /* hide the keyboard DfCursor     */
DFlatMsg(DFM_SHOW_CURSOR)        /* display the keyboard DfCursor  */
DFlatMsg(DFM_SAVE_CURSOR)      /* save the DfCursor's configuration*/
DFlatMsg(DFM_RESTORE_CURSOR)     /* restore the saved DfCursor     */
DFlatMsg(DFM_SHIFT_CHANGED)      /* the shift status changed     */
DFlatMsg(DFM_WAITKEYBOARD)     /* waits for a key to be released */

/* ---------------- mouse messages ------------------------- */
DFlatMsg(DFM_MOUSE_TRAVEL)       /* set the mouse travel         */
DFlatMsg(DFM_RIGHT_BUTTON)       /* right button pressed         */
DFlatMsg(DFM_LEFT_BUTTON)        /* left button pressed          */
DFlatMsg(DFM_DOUBLE_CLICK)       /* left button double-clicked   */
DFlatMsg(DFM_MOUSE_MOVED)        /* mouse changed position       */
DFlatMsg(DFM_BUTTON_RELEASED)    /* mouse button released        */
DFlatMsg(DFM_WAITMOUSE)          /* wait until button released   */
DFlatMsg(DFM_TESTMOUSE)          /* test any mouse button pressed*/
DFlatMsg(DFM_CAPTURE_MOUSE)      /* capture mouse into a window  */
DFlatMsg(DFM_RELEASE_MOUSE)      /* release the mouse to system  */

/* ---------------- text box messages ---------------------- */
DFlatMsg(DFM_ADDTEXT)            /* append text to the text box  */
DFlatMsg(DFM_INSERTTEXT)		 /* insert line of text          */
DFlatMsg(DFM_DELETETEXT)         /* delete line of text          */
DFlatMsg(DFM_CLEARTEXT)          /* clear the edit box           */
DFlatMsg(DFM_SETTEXT)            /* copy text to text buffer     */
DFlatMsg(DFM_SCROLL)             /* vertical line scroll         */
DFlatMsg(DFM_HORIZSCROLL)        /* horizontal column scroll     */
DFlatMsg(DFM_SCROLLPAGE)         /* vertical page scroll         */
DFlatMsg(DFM_HORIZPAGE)          /* horizontal page scroll       */
DFlatMsg(DFM_SCROLLDOC)          /* scroll to beginning/end      */
/* ---------------- edit box messages ---------------------- */
DFlatMsg(DFM_GETTEXT)            /* get text from an edit box    */
DFlatMsg(DFM_SETTEXTLENGTH)		 /* set maximum text length      */
/* ---------------- menubar messages ----------------------- */
DFlatMsg(DFM_BUILDMENU)          /* build the menu display       */
DFlatMsg(DFM_MB_SELECTION)       /* menubar selection            */
/* ---------------- popdown messages ----------------------- */
DFlatMsg(DFM_BUILD_SELECTIONS)   /* build the menu display       */
DFlatMsg(DFM_CLOSE_POPDOWN)    /* tell parent popdown is closing */
/* ---------------- list box messages ---------------------- */
DFlatMsg(DFM_LB_SELECTION)       /* sent to parent on selection  */
DFlatMsg(DFM_LB_CHOOSE)          /* sent when user chooses       */
DFlatMsg(DFM_LB_CURRENTSELECTION)/* return the current selection */
DFlatMsg(DFM_LB_GETTEXT)         /* return the text of selection */
DFlatMsg(DFM_LB_SETSELECTION)    /* sets the listbox selection   */
/* ---------------- dialog box messages -------------------- */
DFlatMsg(DFM_INITIATE_DIALOG)    /* begin a dialog               */
DFlatMsg(DFM_ENTERFOCUS)         /* tell DB control got focus    */
DFlatMsg(DFM_LEAVEFOCUS)         /* tell DB control lost focus   */
DFlatMsg(DFM_ENDDIALOG)          /* end a dialog                 */
/* ---------------- help box messages ---------------------- */
DFlatMsg(DFM_DISPLAY_HELP)
/* --------------- application window messages ------------- */
DFlatMsg(DFM_ADDSTATUS)
/* --------------- picture box messages -------------------- */
DFlatMsg(DFM_DRAWVECTOR)
DFlatMsg(DFM_DRAWBOX)
DFlatMsg(DFM_DRAWBAR)

