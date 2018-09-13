/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WKBMAN.H
 *  WOW32 16-bit Keyboard API support (manually-coded thunks)
 *
 *  History:
 *  Created 27-Jan-1991 by Jeff Parsons (jeffpar)
--*/



/* Keyboard thunks
 */


ULONG FASTCALL WKB32ToAscii(PVDMFRAME pFrame);
ULONG FASTCALL WKB32OemKeyScan(PVDMFRAME pFrame);
ULONG FASTCALL WKB32VkKeyScan(PVDMFRAME pFrame);
ULONG FASTCALL WKB32GetKeyboardType(PVDMFRAME pFrame);
ULONG FASTCALL WKB32MapVirtualKey(PVDMFRAME pFrame);
ULONG FASTCALL WKB32GetKBCodePage(PVDMFRAME pFrame);
ULONG FASTCALL WKB32GetKeyNameText(PVDMFRAME pFrame);
ULONG FASTCALL WKB32AnsiToOemBuff(PVDMFRAME pFrame);
ULONG FASTCALL WKB32OemToAnsiBuff(PVDMFRAME pFrame);
