/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1994, Microsoft Corporation
 *
 *  WWMMAN.H
 *  WOW32 16-bit WifeMan API support (manually-coded thunks)
 *
 *  History:
 *  Created 17-May-1994 by hiroh
--*/

ULONG FASTCALL	WWM32MiscGetEUDCLeadByteRange(PVDMFRAME pFrame);
unsigned char far * PASCAL SkipWhite(unsigned char far *lpch);
unsigned char far * PASCAL StrtoNum(unsigned char  far *lpch,
                                    unsigned short far *lpus);
unsigned short PASCAL CharToNum(unsigned char ch);
