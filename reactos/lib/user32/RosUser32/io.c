/* $Id: io.c,v 1.1 2003/06/30 22:17:37 rcampbell Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * PROGRAMMER:      Richard Campbell
 * UPDATE HISTORY:
 *      06-30-2003  CSH  Created
 */


VOID STDCALL
mouse_event( DWORD dwFlags,
			 DWORD dx,
			 DWORD dy,
			 DWORD dwData,
			 ULONG_PTR dwExtraInfo )
{
	UNIMPLEMENTED;
}

VOID STDCALL
keybd_event( BYTE bVk,
			 BYTE bScan,
			 DWORD dwFlags,
			 PTR dwExtraInfo )
{
	UNIMPLEMENTED;
}

SHORT STDCALL
VkKeyScanA( TCHAR ch )
{
	UNIMPLEMENTED;
}

SHORT STDCALL 
VkKeyScanW( WCHAR ch )
{
	UNIMPLEMENTED;
}

SHORT STDCALL
VkKeyScanExA( char ch,
              HKL dwhkl )
{
	UNIMPLEMENTED;
}

SHORT STDCALL
VkKeyScanExW( WCHAR ch,
    	      HKL dwhkl )
{
	UNIMPLEMENTED;
}

BOOL STDCALL
UnloadKeyboardLayout( HKL hkl )
{
	UNIMPLEMENTED;
}
