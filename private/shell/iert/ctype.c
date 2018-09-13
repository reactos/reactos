/***
*ctype.c - _ctype definition file
*
*       Copyright (c) 1989-1994, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       _ctype definition file of character classification data.  This file
*       initializes the array used by the character classification macros
*       in ctype.h.
*
*******************************************************************************/

#include <cruntime.h>
#include <ctype.h>

unsigned short *_pctype = _ctype+1;     /* pointer to table for char's      */
unsigned short *_pwctype = _ctype+1;    /* pointer to table for wchar_t's   */

unsigned short _ctype[257] = {
        0,                      /* -1 EOF   */
        _CONTROL,               /* 00 (NUL) */
        _CONTROL,               /* 01 (SOH) */
        _CONTROL,               /* 02 (STX) */
        _CONTROL,               /* 03 (ETX) */
        _CONTROL,               /* 04 (EOT) */
        _CONTROL,               /* 05 (ENQ) */
        _CONTROL,               /* 06 (ACK) */
        _CONTROL,               /* 07 (BEL) */
        _CONTROL,               /* 08 (BS)  */
        _SPACE+_CONTROL,        /* 09 (HT)  */
        _SPACE+_CONTROL,        /* 0A (LF)  */
        _SPACE+_CONTROL,        /* 0B (VT)  */
        _SPACE+_CONTROL,        /* 0C (FF)  */
        _SPACE+_CONTROL,        /* 0D (CR)  */
        _CONTROL,               /* 0E (SI)  */
        _CONTROL,               /* 0F (SO)  */
        _CONTROL,               /* 10 (DLE) */
        _CONTROL,               /* 11 (DC1) */
        _CONTROL,               /* 12 (DC2) */
        _CONTROL,               /* 13 (DC3) */
        _CONTROL,               /* 14 (DC4) */
        _CONTROL,               /* 15 (NAK) */
        _CONTROL,               /* 16 (SYN) */
        _CONTROL,               /* 17 (ETB) */
        _CONTROL,               /* 18 (CAN) */
        _CONTROL,               /* 19 (EM)  */
        _CONTROL,               /* 1A (SUB) */
        _CONTROL,               /* 1B (ESC) */
        _CONTROL,               /* 1C (FS)  */
        _CONTROL,               /* 1D (GS)  */
        _CONTROL,               /* 1E (RS)  */
        _CONTROL,               /* 1F (US)  */
        _SPACE+_BLANK,          /* 20 SPACE */
        _PUNCT,                 /* 21 !     */
        _PUNCT,                 /* 22 "     */
        _PUNCT,                 /* 23 #     */
        _PUNCT,                 /* 24 $     */
        _PUNCT,                 /* 25 %     */
        _PUNCT,                 /* 26 &     */
        _PUNCT,                 /* 27 '     */
        _PUNCT,                 /* 28 (     */
        _PUNCT,                 /* 29 )     */
        _PUNCT,                 /* 2A *     */
        _PUNCT,                 /* 2B +     */
        _PUNCT,                 /* 2C ,     */
        _PUNCT,                 /* 2D -     */
        _PUNCT,                 /* 2E .     */
        _PUNCT,                 /* 2F /     */
        _DIGIT+_HEX,            /* 30 0     */
        _DIGIT+_HEX,            /* 31 1     */
        _DIGIT+_HEX,            /* 32 2     */
        _DIGIT+_HEX,            /* 33 3     */
        _DIGIT+_HEX,            /* 34 4     */
        _DIGIT+_HEX,            /* 35 5     */
        _DIGIT+_HEX,            /* 36 6     */
        _DIGIT+_HEX,            /* 37 7     */
        _DIGIT+_HEX,            /* 38 8     */
        _DIGIT+_HEX,            /* 39 9     */
        _PUNCT,                 /* 3A :     */
        _PUNCT,                 /* 3B ;     */
        _PUNCT,                 /* 3C <     */
        _PUNCT,                 /* 3D =     */
        _PUNCT,                 /* 3E >     */
        _PUNCT,                 /* 3F ?     */
        _PUNCT,                 /* 40 @     */
        _UPPER+_HEX,            /* 41 A     */
        _UPPER+_HEX,            /* 42 B     */
        _UPPER+_HEX,            /* 43 C     */
        _UPPER+_HEX,            /* 44 D     */
        _UPPER+_HEX,            /* 45 E     */
        _UPPER+_HEX,            /* 46 F     */
        _UPPER,                 /* 47 G     */
        _UPPER,                 /* 48 H     */
        _UPPER,                 /* 49 I     */
        _UPPER,                 /* 4A J     */
        _UPPER,                 /* 4B K     */
        _UPPER,                 /* 4C L     */
        _UPPER,                 /* 4D M     */
        _UPPER,                 /* 4E N     */
        _UPPER,                 /* 4F O     */
        _UPPER,                 /* 50 P     */
        _UPPER,                 /* 51 Q     */
        _UPPER,                 /* 52 R     */
        _UPPER,                 /* 53 S     */
        _UPPER,                 /* 54 T     */
        _UPPER,                 /* 55 U     */
        _UPPER,                 /* 56 V     */
        _UPPER,                 /* 57 W     */
        _UPPER,                 /* 58 X     */
        _UPPER,                 /* 59 Y     */
        _UPPER,                 /* 5A Z     */
        _PUNCT,                 /* 5B [     */
        _PUNCT,                 /* 5C \     */
        _PUNCT,                 /* 5D ]     */
        _PUNCT,                 /* 5E ^     */
        _PUNCT,                 /* 5F _     */
        _PUNCT,                 /* 60 `     */
        _LOWER+_HEX,            /* 61 a     */
        _LOWER+_HEX,            /* 62 b     */
        _LOWER+_HEX,            /* 63 c     */
        _LOWER+_HEX,            /* 64 d     */
        _LOWER+_HEX,            /* 65 e     */
        _LOWER+_HEX,            /* 66 f     */
        _LOWER,                 /* 67 g     */
        _LOWER,                 /* 68 h     */
        _LOWER,                 /* 69 i     */
        _LOWER,                 /* 6A j     */
        _LOWER,                 /* 6B k     */
        _LOWER,                 /* 6C l     */
        _LOWER,                 /* 6D m     */
        _LOWER,                 /* 6E n     */
        _LOWER,                 /* 6F o     */
        _LOWER,                 /* 70 p     */
        _LOWER,                 /* 71 q     */
        _LOWER,                 /* 72 r     */
        _LOWER,                 /* 73 s     */
        _LOWER,                 /* 74 t     */
        _LOWER,                 /* 75 u     */
        _LOWER,                 /* 76 v     */
        _LOWER,                 /* 77 w     */
        _LOWER,                 /* 78 x     */
        _LOWER,                 /* 79 y     */
        _LOWER,                 /* 7A z     */
        _PUNCT,                 /* 7B {     */
        _PUNCT,                 /* 7C |     */
        _PUNCT,                 /* 7D }     */
        _PUNCT,                 /* 7E ~     */
        _CONTROL,               /* 7F (DEL) */
        /* and the rest are 0... */
};
