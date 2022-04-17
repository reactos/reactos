/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/hw/keyboard.c
 * PURPOSE:         LLB Keyboard Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

#define E0_KPENTER      96
#define E0_RCTRL        97
#define E0_KPSLASH      98
#define E0_PRSCR        99
#define E0_RALT         100
#define E0_BREAK        101
#define E0_HOME         102
#define E0_UP           103
#define E0_PGUP         104
#define E0_LEFT         105
#define E0_RIGHT        106
#define E0_END          107
#define E0_DOWN         108
#define E0_PGDN         109
#define E0_INS          110
#define E0_DEL          111
#define E1_PAUSE        119
#define E0_MACRO        112
#define E0_F13          113
#define E0_F14          114
#define E0_HELP         115
#define E0_DO           116
#define E0_F17          117
#define E0_KPMINPLUS    118
#define E0_OK           124
#define E0_MSLW         125
#define E0_MSRW         126
#define E0_MSTM         127

/* US 101 KEYBOARD LAYOUT *****************************************************/

CHAR LlbHwScanCodeToAsciiTable[58][2] =
{
    {   0,0   } ,
    {  27, 27 } ,
    { '1','!' } ,
    { '2','@' } ,
    { '3','#' } ,
    { '4','$' } ,
    { '5','%' } ,
    { '6','^' } ,
    { '7','&' } ,
    { '8','*' } ,
    { '9','(' } ,
    { '0',')' } ,
    { '-','_' } ,
    { '=','+' } ,
    {   8,8   } ,
    {   9,9   } ,
    { 'q','Q' } ,
    { 'w','W' } ,
    { 'e','E' } ,
    { 'r','R' } ,
    { 't','T' } ,
    { 'y','Y' } ,
    { 'u','U' } ,
    { 'i','I' } ,
    { 'o','O' } ,
    { 'p','P' } ,
    { '[','{' } ,
    { ']','}' } ,
    {  13,13  } ,
    {   0,0   } ,
    { 'a','A' } ,
    { 's','S' } ,
    { 'd','D' } ,
    { 'f','F' } ,
    { 'g','G' } ,
    { 'h','H' } ,
    { 'j','J' } ,
    { 'k','K' } ,
    { 'l','L' } ,
    { ';',':' } ,
    {  39,34  } ,
    { '`','~' } ,
    {   0,0   } ,
    { '\\','|'} ,
    { 'z','Z' } ,
    { 'x','X' } ,
    { 'c','C' } ,
    { 'v','V' } ,
    { 'b','B' } ,
    { 'n','N' } ,
    { 'm','M' } ,
    { ',','<' } ,
    { '.','>' } ,
    { '/','?' } ,
    {   0,0   } ,
    {   0,0   } ,
    {   0,0   } ,
    { ' ',' ' } ,
};

/* EXTENDED KEY TABLE *********************************************************/

UCHAR LlbHwExtendedScanCodeTable[128] =
{
	0,		0,		0,		0,
    0,		0,		0,		0,
 	0,		0,		0,		0,
 	0,		0,		0,		0,
 	0,		0,		0,		0,
 	0,		0,		0,		0,
 	0,		0,		0,		0,
 	E0_KPENTER,	E0_RCTRL,	0,		0,
 	0,		0,		0,		0,
 	0,		0,		0,		0,
 	0,		0,		0,		0,
 	0,		0,		0,		0,
 	0,		0,		0,		0,
 	0,		E0_KPSLASH,	0,		E0_PRSCR,
 	E0_RALT,	0,		0,		0,
 	0,		E0_F13,		E0_F14,		E0_HELP,
 	E0_DO,		E0_F17,		0,		0,
 	0,		0,		E0_BREAK,	E0_HOME,
 	E0_UP,		E0_PGUP,	0,		E0_LEFT,
 	E0_OK,		E0_RIGHT,	E0_KPMINPLUS,	E0_END,
 	E0_DOWN,	E0_PGDN,	E0_INS,		E0_DEL,
 	0,		0,		0,		0,
 	0,		0,		0,		E0_MSLW,
 	E0_MSRW,	E0_MSTM,	0,		0,
 	0,		0,		0,		0,
 	0,		0,		0,		0,
 	0,		0,		0,		0,
 	0,		0,		0,		E0_MACRO,
 	0,		0,		0,		0,
 	0,		0,		0,		0,
 	0,		0,		0,		0,
 	0,		0,		0,		0
 };


/* FUNCTIONS ******************************************************************/

USHORT LlbKbdLastScanCode;

UCHAR
NTAPI
LlbKbdTranslateScanCode(IN USHORT ScanCode,
                        IN PUCHAR KeyCode)
{
    ULONG LastScanCode;

    /* Check for extended scan codes */
    if ((ScanCode == 0xE0) || (ScanCode == 0xE1))
 	{
 	    /* We'll get these on the next scan */
 		LlbKbdLastScanCode = ScanCode;
 		return 0;
 	}

    /* Check for bogus scan codes */
 	if ((ScanCode == 0x00) || (ScanCode == 0xFF))
 	{
 	    /* Reset */
 		LlbKbdLastScanCode = 0;
 		return 0;
 	}

 	/* Only act on the break, not the make */
    if (ScanCode > 0x80) return 0;

    /* Keep only simple scan codes */
 	ScanCode &= 0x7F;

    /* Check if this was part of an extended sequence */
 	if (LlbKbdLastScanCode)
 	{
 	    /* Save the last scan code and clear it, since we've consumed it now */
        LastScanCode = LlbKbdLastScanCode;
        LlbKbdLastScanCode = 0;
 		switch (LastScanCode)
 		{
 		    /* E0 extended codes */
 		    case 0xE0:

 		        /* Skip bogus codes */
 			    if ((ScanCode == 0x2A) || (ScanCode == 0x36)) return 0;

 			    /* Lookup the code for it */
                if (!LlbHwExtendedScanCodeTable[ScanCode]) return 0;
 				*KeyCode = LlbHwExtendedScanCodeTable[ScanCode];
 			    break;

            /* E1 extended codes */
 		    case 0xE1:

 		        /* Only recognize one (the SYSREQ/PAUSE sequence) */
                if (ScanCode != 0x1D) return 0;
 			    LlbKbdLastScanCode = 0x100;
 			    break;

            /* PAUSE sequence */
 		    case 0x100:

 		        /* Make sure it's the one */
                if (ScanCode != 0x45) return 0;
 				*KeyCode = E1_PAUSE;
 			    break;
 		    }
 	}
 	else
 	{
 	    /* Otherwise, the scancode is the key code */
        LlbKbdLastScanCode = 0;
 		*KeyCode = ScanCode;
	}

	/* Translation success */
 	return 1;
 }

CHAR
NTAPI
LlbKeyboardGetChar(VOID)
{
    UCHAR ScanCode, KeyCode;

    do
    {
        /* Read the scan code and convert it to a virtual key code */
        ScanCode = LlbHwKbdRead();
    } while (!LlbKbdTranslateScanCode(ScanCode, &KeyCode));

    /* Is this ASCII? */
    if (KeyCode > 96) return ScanCode;

    /* Return the ASCII character */
    return LlbHwScanCodeToAsciiTable[KeyCode][0];
}

/* EOF */
