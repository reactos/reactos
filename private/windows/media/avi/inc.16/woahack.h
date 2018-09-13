/* Defines for WinOldAppHackoMatic flags which win386 oldapp can send to us. 
 * These are kept in user's global variable winOldAppHackoMaticFlags
 */

/*
 * WOAHACK_CHECKALTKEYSTATE
 *
 *  This is sent from WinOldAp to USER when the user has just pressed
 *  Alt+Space.  When the user does that, keyboard focus shifts from
 *  the DOS box to Windows.  This is particularly weird because the
 *  focus is changing while the user has a key down.  The message is
 *  sent to USER to let it know that the Alt key may be in a funky
 *  state right now.
 *
 *  The value of this must be a power of two because it is used
 *  in the winOldAppHackoMaticFlags variable as a bitmask.
 *
 * WOAHACK_IGNOREALTKEYDOWN
 *
 *  This is used internally by USER to keep track of the Alt key
 *  state, triggered by the WOAHACK_CHECKALTKEYSTATE message.
 *
 *  The value of this must be a power of two because it is used
 *  in the winOldAppHackoMaticFlags variable as a bitmask.
 *
 * WOAHACK_DISABLEREPAINTSCREEN
 *
 *  This is sent from WinOldAp to USER when the user has initiated
 *  an Alt+Tab sequence from a fullscreen VM.  This causes USER to
 *  defer the RepaintScreen that will be sent by the display driver.
 *  This speeds up Alt+Tab'ing.  When the Alt+Tab is complete,
 *  if the user selected another fullscreen DOS box, USER will ignore
 *  the RepaintScreen altogether, since it's about to lose the
 *  display focus anyway.  In all other cases, the RepaintScreen will
 *  be performed once the Alt+Tab is complete.
 *
 * WOAHACK_LOSINGDISPLAYFOCUS
 * WOAHACK_GAININGDISPLAYFOCUS
 *
 *  These messages are sent from WinOldAp to USER when the display
 *  focus is changing.  The WOAHACK_LOSINGDISPLAYFOCUS message is
 *  sent *before* Windows is about to lose focus, and the
 *  WOAHACK_GAININGDISPLAYFOCUS is sent *after* Windows has regained
 *  the display focus.
 *
 *  WinOldAp sends these messages whenever it is about to set the
 *  VM focus.  It doesn't check if the message is a repeat of a
 *  previous message.  (So, for example, USER may receive three
 *  copies of WOAHACK_GAININGDISPLAYFOCUS.)  USER maintains an
 *  internal variable to keep track of whether each receipt of the
 *  message is a change in the focus or just a redundant notification.
 *
 *  USER uses these messages to turn off the timer that is normally
 *  used to trigger the mouse drawing code in the display driver
 *  while Windows does not have the display focus, and to turn the
 *  timer back on once Windows gets the display back.
 *
 *  This is needed to make sure the display driver is otherwise
 *  quiet when the int 2F is dispatched from the VDD to notify the
 *  display driver that the focus has changed.  Not doing this
 *  opens the possibility of re-entrancy in the display driver.
 *
 *  WOAHACK_IMAWINOLDAPSORTOFGUY
 *
 *  WinOldAp needs to call this API once with this flag set so that
 *  USER can mark winoldap's queue as being winoldap.  USER needs
 *  to know that a particular queue is winoldap for several reasons
 *  having to do with task switching and priorities.
 */
#define WOAHACK_CHECKALTKEYSTATE 1
#define WOAHACK_IGNOREALTKEYDOWN 2
#define WOAHACK_DISABLEREPAINTSCREEN  3
#define WOAHACK_LOSINGDISPLAYFOCUS    4
#define WOAHACK_GAININGDISPLAYFOCUS   5
#define WOAHACK_IAMAWINOLDAPSORTOFGUY 6


/* ------ After this point comes information that can be publicized ------- */

/* WinOldApp related flags and Macros */

/*
 *  These property bits are stored in the flWinOldAp property of
 *  WinOldAp's main window.  They are provided so that USER and other
 *  applications can query the state of the DOS box.
 *
 *  These properties are read-only.  Changing them will cause Windows
 *  to get confused.
 *
 *  woapropIsWinOldAp
 *
 *	This bit is always set.
 *
 *  woapropFullscreen
 *
 *	Set if this a fullscreen DOS box rather than a windowed DOS box.
 *	Note that this bit is set even if WinOldAp does not have focus.
 *	(E.g., when iconic.)  Its purpose is to indicate what the state
 *	of the DOS box would be *if* it were to be activated.
 *
 *  woapropActive
 *
 *	Set if WinOldAp is active.  Note that one cannot merely
 *	check IsIconic(hwnd) because fullscreen DOS boxes are always
 *	iconic.
 *
 *  None of the other bits are used, although names for some of them
 *  have been chosen.  This does not represent any commitment to use
 *  them for the purpose the name suggests.
 *
 *  Other properties we may think of adding...
 *
 *	hvm
 *
 *	    This extended property contains the 32-bit VM handle.
 *
 *	hprop
 *
 *	    This property contains the property handle being used
 *	    by the DOS box.  (For utilities which want to be able
 *	    to modify the properties of a running DOS box, perhaps.)
 *
 */

#define WINOLDAP_PROP_NAME	"flWinOldAp"

extern ATOM atmWinOldAp;
#define WinOldApFlag(hwnd, flag) ((UINT)GetPropEx(hwnd, MAKEINTATOM(atmWinOldAp)) & woaprop##flag)

#define woapropIsWinOldAp	1
#define woapropFullscreen	2
#define woapropActive		4
#define woapropIdle		8
#define woapropClosable		64

#define IsWinOldApHwnd(hwnd) WinOldApFlag(hwnd, IsWinOldAp)
#define IsFullscreen(hwnd) WinOldApFlag(hwnd, Fullscreen)
