/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:          ReactOS kernel
 * FILE:             ntoskrnl/dbg/kdb_keyboard.c
 * PURPOSE:          Keyboard driver
 * PROGRAMMER:       Victor Kirhenshtein (sauros@iname.com)
 *                   Jason Filby (jasonfilby@yahoo.com)
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <string.h>
#include <ntos/keyboard.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <debug.h>

#if 1

#define KBD_STATUS_REG		0x64	
#define KBD_CNTL_REG		0x64	
#define KBD_DATA_REG		0x60	

#define KBD_STAT_OBF 		0x01	/* Keyboard output buffer full */

#define kbd_read_input() READ_PORT_UCHAR((PUCHAR)KBD_DATA_REG)
#define kbd_read_status() READ_PORT_UCHAR((PUCHAR)KBD_STATUS_REG)

static unsigned char keyb_layout[2][128] =
{
	"\000\0331234567890-=\177\t"			/* 0x00 - 0x0f */
	"qwertyuiop[]\r\000as"				/* 0x10 - 0x1f */
	"dfghjkl;'`\000\\zxcv"				/* 0x20 - 0x2f */
	"bnm,./\000*\000 \000\201\202\203\204\205"	/* 0x30 - 0x3f */
	"\206\207\210\211\212\000\000789-456+1"		/* 0x40 - 0x4f */
	"230\177\000\000\213\214\000\000\000\000\000\000\000\000\000\000" /* 0x50 - 0x5f */
	"\r\000/"					/* 0x60 - 0x6f */
	,
	"\000\033!@#$%^&*()_+\177\t"			/* 0x00 - 0x0f */
	"QWERTYUIOP{}\r\000AS"				/* 0x10 - 0x1f */
	"DFGHJKL:\"`\000\\ZXCV"				/* 0x20 - 0x2f */
	"BNM<>?\000*\000 \000\201\202\203\204\205"	/* 0x30 - 0x3f */
	"\206\207\210\211\212\000\000789-456+1"		/* 0x40 - 0x4f */
	"230\177\000\000\213\214\000\000\000\000\000\000\000\000\000\000" /* 0x50 - 0x5f */
	"\r\000/"					/* 0x60 - 0x6f */
};

typedef BYTE byte_t;

CHAR
KdbTryGetCharKeyboard()
{
    static byte_t last_key = 0;
    static byte_t shift = 0;
    char c;
    while(1) {
	unsigned char status = kbd_read_status();
	while (status & KBD_STAT_OBF) {
	    byte_t scancode;
	    scancode = kbd_read_input();
	    /* check for SHIFT-keys */
	    if (((scancode & 0x7F) == 42) || ((scancode & 0x7F) == 54))
	    {
		shift = !(scancode & 0x80);
		continue;
	    }
	    /* ignore all other RELEASED-codes */
	    if (scancode & 0x80)
		last_key = 0;
	    else if (last_key != scancode)
	    {
		//printf("kbd: %d, %d, %c\n", scancode, last_key, keyb_layout[shift][scancode]);
		last_key = scancode;
		c = keyb_layout[shift][scancode];
		if (c > 0) return c;
	    }
	}
    }
}

#endif

#if 0

/* GLOBALS *******************************************************************/

/*
 * Keyboard I/O ports.
 */
#define K_RDWR 		0x60		/* keyboard data & cmds (read/write) */
#define K_STATUS 	0x64		/* keybd status (read-only) */
#define K_CMD	 	0x64		/* keybd ctlr command (write-only) */

/*
 * Bit definitions for K_STATUS port.
 */
#define K_OBUF_FUL 	0x01		/* output (from keybd) buffer full */
#define K_IBUF_FUL 	0x02		/* input (to keybd) buffer full */
#define K_SYSFLAG	0x04		/* "System Flag" */
#define K_CMD_DATA	0x08		/* 1 = input buf has cmd, 0 = data */
#define K_KBD_INHIBIT	0x10		/* 0 if keyboard inhibited */
#define K_AUX_OBUF_FUL	0x20		/* 1 = obuf holds aux device data */
#define K_TIMEOUT	0x40		/* timout error flag */
#define K_PARITY_ERROR	0x80		/* parity error flag */

/* 
 * Keyboard controller commands (sent to K_CMD port).
 */
#define KC_CMD_READ	0x20		/* read controller command byte */
#define KC_CMD_WRITE	0x60		/* write controller command byte */
#define KC_CMD_DIS_AUX	0xa7		/* disable auxiliary device */
#define KC_CMD_ENB_AUX	0xa8		/* enable auxiliary device */
#define KC_CMD_TEST_AUX	0xa9		/* test auxiliary device interface */
#define KC_CMD_SELFTEST	0xaa		/* keyboard controller self-test */
#define KC_CMD_TEST	0xab		/* test keyboard interface */
#define KC_CMD_DUMP	0xac		/* diagnostic dump */
#define KC_CMD_DISABLE	0xad		/* disable keyboard */
#define KC_CMD_ENABLE	0xae		/* enable keyboard */
#define KC_CMD_RDKBD	0xc4		/* read keyboard ID */
#define KC_CMD_WIN	0xd0		/* read  output port */
#define KC_CMD_WOUT	0xd1		/* write output port */
#define KC_CMD_ECHO	0xee		/* used for diagnostic testing */
#define KC_CMD_PULSE	0xff		/* pulse bits 3-0 based on low nybble */

/* 
 * Keyboard commands (send to K_RDWR).
 */
#define K_CMD_LEDS	0xed		/* set status LEDs (caps lock, etc.) */
#define K_CMD_TYPEMATIC	0xf3		/* set key repeat and delay */

/* 
 * Bit definitions for controller command byte (sent following 
 * KC_CMD_WRITE command).
 *
 * Bits 0x02 and 0x80 unused, always set to 0.
 */
#define K_CB_ENBLIRQ	0x01		/* enable data-ready intrpt */
#define K_CB_SETSYSF	0x04		/* Set System Flag */
#define K_CB_INHBOVR	0x08		/* Inhibit Override */
#define K_CB_DISBLE	0x10		/* disable keyboard */
#define K_CB_IGNPARITY	0x20		/* ignore parity from keyboard */
#define K_CB_SCAN	0x40		/* standard scan conversion */

/* 
 * Bit definitions for "Indicator Status Byte" (sent after a 
 * K_CMD_LEDS command).  If the bit is on, the LED is on.  Undefined 
 * bit positions must be 0.
 */
#define K_LED_SCRLLK	0x1		/* scroll lock */
#define K_LED_NUMLK	0x2		/* num lock */
#define K_LED_CAPSLK	0x4		/* caps lock */

/* 
 * Bit definitions for "Miscellaneous port B" (K_PORTB).
 */
/* read/write */
#define K_ENABLETMR2	0x01		/* enable output from timer 2 */
#define K_SPKRDATA	0x02		/* direct input to speaker */
#define K_ENABLEPRTB	0x04		/* "enable" port B */
#define K_EIOPRTB	0x08		/* enable NMI on parity error */
/* read-only */
#define K_REFRESHB	0x10		/* refresh flag from INLTCONT PAL */
#define K_OUT2B		0x20		/* timer 2 output */
#define K_ICKB		0x40		/* I/O channel check (parity error) */

/*
 * Bit definitions for the keyboard controller's output port.
 */
#define KO_SYSRESET	0x01		/* processor reset */
#define KO_GATE20	0x02		/* A20 address line enable */
#define KO_AUX_DATA_OUT	0x04		/* output data to auxiliary device */
#define KO_AUX_CLOCK	0x08		/* auxiliary device clock */
#define KO_OBUF_FUL	0x10		/* keyboard output buffer full */
#define KO_AUX_OBUF_FUL	0x20		/* aux device output buffer full */
#define KO_CLOCK	0x40		/* keyboard clock */
#define KO_DATA_OUT	0x80		/* output data to keyboard */

/*
 * Keyboard return codes.
 */
#define K_RET_RESET_DONE	0xaa		/* BAT complete */
#define K_RET_ECHO		0xee		/* echo after echo command */
#define K_RET_ACK		0xfa		/* ack */
#define K_RET_RESET_FAIL	0xfc		/* BAT error */
#define K_RET_RESEND		0xfe		/* resend request */

#define SHIFT	-1
#define CTRL	-2
#define META	-3

static char keymap[128][2] = {
	{0},			/* 0 */
	{27,	27},		/* 1 - ESC */
	{'1',	'!'},		/* 2 */
	{'2',	'@'},
	{'3',	'#'},
	{'4',	'$'},
	{'5',	'%'},
	{'6',	'^'},
	{'7',	'&'},
	{'8',	'*'},
	{'9',	'('},
	{'0',	')'},
	{'-',	'_'},
	{'=',	'+'},
	{8,	8},		/* 14 - Backspace */
	{'\t',	'\t'},		/* 15 */
	{'q',	'Q'},
	{'w',	'W'},
	{'e',	'E'},
	{'r',	'R'},
	{'t',	'T'},
	{'y',	'Y'},
	{'u',	'U'},
	{'i',	'I'},
	{'o',	'O'},
	{'p',	'P'},
	{'[',	'{'},
	{']',	'}'},		/* 27 */
	{'\r',	'\r'},		/* 28 - Enter */
	{CTRL,	CTRL},		/* 29 - Ctrl */
	{'a',	'A'},		/* 30 */
	{'s',	'S'},
	{'d',	'D'},
	{'f',	'F'},
	{'g',	'G'},
	{'h',	'H'},
	{'j',	'J'},
	{'k',	'K'},
	{'l',	'L'},
	{';',	':'},
	{'\'',	'"'},		/* 40 */
	{'`',	'~'},		/* 41 */
	{SHIFT,	SHIFT},		/* 42 - Left Shift */
	{'\\',	'|'},		/* 43 */
	{'z',	'Z'},		/* 44 */
	{'x',	'X'},
	{'c',	'C'},
	{'v',	'V'},
	{'b',	'B'},
	{'n',	'N'},
	{'m',	'M'},
	{',',	'<'},
	{'.',	'>'},
	{'/',	'?'},		/* 53 */
	{SHIFT,	SHIFT},		/* 54 - Right Shift */
	{0,	0},		/* 55 - Print Screen */
	{META,	META},		/* 56 - Alt */
	{' ',	' '},		/* 57 - Space bar */
	{0,	0},		/* 58 - Caps Lock */
	{0,	0},		/* 59 - F1 */
	{0,	0},		/* 60 - F2 */
	{0,	0},		/* 61 - F3 */
	{0,	0},		/* 62 - F4 */
	{0,	0},		/* 63 - F5 */
	{0,	0},		/* 64 - F6 */
	{0,	0},		/* 65 - F7 */
	{0,	0},		/* 66 - F8 */
	{0,	0},		/* 67 - F9 */
	{0,	0},		/* 68 - F10 */
	{0,	0},		/* 69 - Num Lock */
	{0,	0},		/* 70 - Scroll Lock */
	{'7',	'7'},		/* 71 - Numeric keypad 7 */
	{'8',	'8'},		/* 72 - Numeric keypad 8 */
	{'9',	'9'},		/* 73 - Numeric keypad 9 */
	{'-',	'-'},		/* 74 - Numeric keypad '-' */
	{'4',	'4'},		/* 75 - Numeric keypad 4 */
	{'5',	'5'},		/* 76 - Numeric keypad 5 */
	{'6',	'6'},		/* 77 - Numeric keypad 6 */
	{'+',	'+'},		/* 78 - Numeric keypad '+' */
	{'1',	'1'},		/* 79 - Numeric keypad 1 */
	{'2',	'2'},		/* 80 - Numeric keypad 2 */
	{'3',	'3'},		/* 81 - Numeric keypad 3 */
	{'0',	'0'},		/* 82 - Numeric keypad 0 */
	{'.',	'.'},		/* 83 - Numeric keypad '.' */
};

/* FUNCTIONS *****************************************************************/

/*
 * Quick poll for a pending input character.
 * Returns a character if available, -1 otherwise.  This routine can return
 * false negatives in the following cases:
 *
 *	- a valid character is in transit from the keyboard when called
 *	- a key release is received (from a previous key press)
 *	- a SHIFT key press is received (shift state is recorded however)
 *	- a key press for a multi-character sequence is received
 *
 * Yes, this is horrible.
 */
ULONG
KdbTryGetCharKeyboard(VOID)
{
	static unsigned shift_state, ctrl_state, meta_state;
	unsigned scan_code, ch;

	/* See if a scan code is ready, returning if none. */
	if ((READ_PORT_UCHAR((PUCHAR)K_STATUS) & K_OBUF_FUL) == 0) {
		return -1;
	}
	scan_code = READ_PORT_UCHAR((PUCHAR)K_RDWR);

	/* Handle key releases - only release of SHIFT is important. */
	if (scan_code & 0x80) {
		scan_code &= 0x7f;
		if (keymap[scan_code][0] == SHIFT)
			shift_state = 0;
		else if (keymap[scan_code][0] == CTRL)
			ctrl_state = 0;
		else if (keymap[scan_code][0] == META)
			meta_state = 0;
		ch = -1;
	} else {
		/* Translate the character through the keymap. */
		ch = keymap[scan_code][shift_state] | meta_state;
		if (ch == SHIFT) {
			shift_state = 1;
			ch = -1;
		} else if (ch == CTRL) {
			ctrl_state = 1;
			ch = -1;
		} else if (ch == META) {
			meta_state = 0200;
			ch = -1;
		} else if (ch == 0)
			ch = -1;
		else if (ctrl_state)
			ch = (keymap[scan_code][1] - '@') | meta_state;
	}

	return ch;
}

#endif
