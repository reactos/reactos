/*  File: \shared\emulator\keydef.h (Created: 12/19/95)
 *
 *  Copyright 1995 by Hilgraeve Inc. -- Monroe, MI
 *  All rights reserved
 *
 *  Description:
 *		This header file defines structures and macros for
 *		handling keyboard imput in a plateform independent
 *		fashion.
 *
 *  $Revision: 1 $
 *  $Date: 10/05/98 12:27p $
 */
#if !defined(KEYDEF_INCLUDED)
#define KEYDEF_INCLUDED

// Here it is!  The fame and fabled KEYDEF typedef.  This type is used
// throughout our entire code base to represent a "key".  A key is any
// character data coming from the user thru a local input device (fancy
// way of saying a keyboard).  It contains reserved areas as follows:
//
// bits 00-15   : character or virtual key code.
// bits 16-19   : key state information (ALT, CTRL, SHIFT, EXTENDED)
// bit  23		: controls how bits 00-15 are interpreted (Virtual or char)
//
// This means a KEYDEF has a minimum size of 24 bits.  In practical terms
// KEYDEF should be defined in such way to be at least 32 bits.
//
// Interesting thought:  There is a world standard being proposed that
// use 32 bits to represent a character.  If this ever happens, we would
// have to have a 40 bit value to represent a key.  I suspect that when
// that happens, we'll all be using 64 bit architectures anyways.
//
// Interesting thought two:  We tried to represent a key as a bit-field
// structure but ran into difficulties.  The disadvantages were; needed
// a function to compare KEYDEF values since bit-fields are set in a
// plateform specific way; could not easily create constant KEYDEF values
// that could be used in switch statements.  Using a simply integer type
// makes manipulating and comparing KEYDEF values much easier.
//
typedef unsigned int KEYDEF;   // minimum size is 32 bits.

// Keys are interpreted as follows:
//
// If the VIRTUAL_KEY flag is clear, then the lower word of the value is the
// displayable (usually ASCII) code for the character
//
// If the VIRTUAL_KEY flag is set, then the lower word is the
// HVK key code for the key that was pressed.  In addition, the flags
// for ALT_KEY, CTRL_KEY, SHIFT_KEY, and EXTENDED_KEY are set to the
// correct values.
//
// mrw:3/4/96 - Added the HVIRTUAL_KEY flag.  Needed to do this for
// windows because many of the WM_KEYDOWN sequences in Windows look
// like our HVK_? values.  The VIRTUAL_KEY flag is stilled OR'ed in
// to the HVK_? values to maintain compatibility with our old code.
//
#define HVIRTUAL_KEY		0x01000000
#define VIRTUAL_KEY			0x00800000
#define ALT_KEY				0x00010000
#define CTRL_KEY			0x00020000
#define SHIFT_KEY			0x00040000
#define EXTENDED_KEY		0x00080000

// So just what is an HVK key code?  Virtual keys are representations
// for keys that are independent of the position on the keyboard.  For
// instance, our program deals with the concept of Page-Up.  We don't
// really care that it is one code in OS/2 and another code in Windows.
// Both OS/2 and Windows generate Virtual key codes but they are
// values (and sometimes symbolic names).  To keep our code independent
// of these differences, we translate the system specific virtual key
// code to a HVK key code.  Our code then deals only in HVK virtual key
// codes.  There of course must be function to provide a translation
// layer which is always defined on the project side.
//
// HVK constants are our definitions for virtual keys.  They are plateform
// independent.  Anyone translating keyboard input will need a function to
// map the system specific virtual key code to a HVK key code.
//
#define HVK_BUTTON1			(0x01 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_BUTTON2			(0x02 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_BUTTON3			(0x03 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_BREAK			(0x04 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_BACKSPACE		(0x05 | VIRTUAL_KEY | HVIRTUAL_KEY)   // can't be used in shared code - mrw
#define HVK_TAB				(0x06 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_BACKTAB			(0x07 | VIRTUAL_KEY | HVIRTUAL_KEY)   // can't be used in shared code - mrw
#define HVK_NEWLINE			(0x08 | VIRTUAL_KEY | HVIRTUAL_KEY)   // can't be used in shared code - mrw
#define HVK_SHIFT			(0x09 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_CTRL			(0x0A | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_ALT				(0x0B | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_ALTGRAF			(0x0C | VIRTUAL_KEY | HVIRTUAL_KEY)   // can't be used in shared code - mrw
#define HVK_PAUSE			(0x0D | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_CAPSLOCK		(0x0E | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_ESC				(0x0F | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_SPACE			(0x10 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_PAGEUP			(0x11 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_PAGEDOWN		(0x12 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_END				(0x13 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_HOME			(VK_HOME | VIRTUAL_KEY)
#define HVK_LEFT			(VK_LEFT | VIRTUAL_KEY)
#define HVK_UP				(VK_UP | VIRTUAL_KEY)
#if FALSE
#define HVK_HOME			(0x14 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_LEFT			(0x15 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_UP				(0x16 | VIRTUAL_KEY | HVIRTUAL_KEY)
#endif
#define HVK_RIGHT			(0x17 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_DOWN			(0x18 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_PRINTSCRN		(0x19 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_INSERT			(0x1A | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_DELETE			(0x1B | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_SCRLLOCK		(0x1C | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_NUMLOCK			(0x1D | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_ENTER			(0x1E | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_SYSRQ			(0x1F | VIRTUAL_KEY | HVIRTUAL_KEY)   // can't be used in shared code - mrw
#define HVK_F1				(0x20 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_F2				(0x21 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_F3				(0x22 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_F4				(0x23 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_F5				(0x24 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_F6				(VK_F6 | VIRTUAL_KEY)
#define HVK_F7				(VK_F7 | VIRTUAL_KEY)
#define HVK_F8				(VK_F8 | VIRTUAL_KEY)
#define HVK_F9				(VK_F9 | VIRTUAL_KEY)
#if FALSE
#define HVK_F6				(0x25 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_F7				(0x26 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_F8				(0x27 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_F9				(0x28 | VIRTUAL_KEY | HVIRTUAL_KEY)
#endif
#define HVK_F10				(0x29 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_F11				(0x2A | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_F12				(0x2B | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_F13				(0x2C | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_F14				(0x2D | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_F15				(0x2E | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_F16				(0x2F | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_F17				(0x30 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_F18				(0x31 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_F19				(0x32 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_F20				(0x33 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_F21				(0x34 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_F22				(0x35 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_F23				(0x36 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_F24				(0x37 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_ENDDRAG			(0x38 | VIRTUAL_KEY | HVIRTUAL_KEY)   // can't be used in shared code - mrw
#define HVK_EREOF			(0x3A | VIRTUAL_KEY | HVIRTUAL_KEY)   // can't be used in shared code - mrw
#define HVK_PA1				(0x3B | VIRTUAL_KEY | HVIRTUAL_KEY)

#define HVK_ADD				(0x3D | VIRTUAL_KEY | HVIRTUAL_KEY)   // Identifies key on Numeric Keypad only.
#define HVK_SUBTRACT		(0x3E | VIRTUAL_KEY | HVIRTUAL_KEY)   // Identifies key on Numeric Keypad only.

// These constants represent the keys on the numeric keypad, when
// the Num Lock key is on.  Again, when the Num Lock key is On.
//
#define HVK_NUMPAD0			(0x45 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_NUMPAD1			(0x46 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_NUMPAD2			(0x47 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_NUMPAD3			(0x48 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_NUMPAD4			(0x49 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_NUMPAD5			(0x64 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_NUMPAD6			(0x4A | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_NUMPAD7			(0x4B | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_NUMPAD8			(0x4C | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_NUMPAD9			(0x4D | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_NUMPADPERIOD	(0x53 | VIRTUAL_KEY | HVIRTUAL_KEY)

// These constants represent some of the keys on the numeric keypad, only.
//
#define HVK_DECIMAL			(0x4E | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_RETURN			(0x4F | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_FSLASH			(0x50 | VIRTUAL_KEY | HVIRTUAL_KEY)
#define HVK_MULTIPLY		(0x51 | VIRTUAL_KEY | HVIRTUAL_KEY)

// This constant represents the 5 on the numeric keypad, or the center
// key on the edit pad.  If it's from the edit pad, the extended bit
// will be set.
//
#define HVK_CENTER			(0x52 | VIRTUAL_KEY | HVIRTUAL_KEY)

#endif
