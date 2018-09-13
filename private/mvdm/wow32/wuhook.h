/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WUHOOK.H
 *  WOW32 16-bit User API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
--*/



ULONG FASTCALL   WU32SetWindowsHookInternal(PVDMFRAME pFrame);
ULONG FASTCALL   WU32UnhookWindowsHook(PVDMFRAME pFrame);
ULONG FASTCALL   WU32CallNextHookEx(PVDMFRAME pFrame);
ULONG FASTCALL   WU32SetWindowsHookEx(PVDMFRAME pFrame);
ULONG FASTCALL   WU32UnhookWindowsHookEx(PVDMFRAME pFrame);


#define HOOK_ID      0x4B48             // dumps as 'H' 'K'
#define MAKEHHOOK(index)           (MAKELONG(index,HOOK_ID))
#define GETHHOOKINDEX(hook)        (LOWORD(hook))
#define ISVALIDHHOOK(hook)         (HIWORD(hook) == HOOK_ID)
