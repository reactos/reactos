/* $Id: string.c,v 1.2 2003/07/01 01:03:49 rcampbell Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * PROGRAMMER:      Richard Campbell
 * UPDATE HISTORY:
 *      06-30-2003  CSH  Created
 */

INT STDCALL
wsprintfA( LPCSTR lpOut,
		   LPCSTR lpFmt,
	       ...)
{
	UNIMPLEMENTED;
}

INT STDCALL
wsprintfW( LPCWSTR lpOut,
		   LPCWSTR lpFmt,
		   ... )
{
	UNIMPLEMENTED;
}

INT STDCALL
wvsprintfA( LPSTR lpOutput,
		   LPSTR lpFmt,
		   va_list arglist )
{
	UNIMPLEMENTED;
}

INT STDCALL
wvsprintfW( LPWSTR lpOutput,
		    LPWSTR lpFmt,
			va_list arglist )
{
	UNIMPLEMENTED;
}

INT STDCALL
ToUnicodeEx( UINT wVirtKey,
             UINT wScanCode,
             const PBYTE lpKeyState,
             LPWSTR pwszBuff,
             int cchBuff,
             UINT wFlags,
        	 HKL dwhkl )
{
	UNIMPLEMENTED;
}

INT STDCALL
ToUnicode( UINT wVirtKey,
		   UINT wScanCode,
		   const PBYTE lpKeyState,
		   LPWSTR pwszBuff,
		   int cchBuff, 
		   UINT wFlags )
{
	UNIMPLEMENTED;
}

INT STDCALL
ToAsciiEx( UINT uVirtKey,
   		   UINT uScanCode,
		   PBYTE lpKeyState,
		   LPWORD lpChar,
		   UINT uFlags,
		   HKL dwhkl )
{
	UNIMPLEMENTED;
}

INT STDCALL
ToAscii( UINT uVirtKey,
		 UINT uScanCode,
		 PBYTE lpKeyState,
		 LPWORD lpChar,
		 UINT uFlags )
{
	UNIMPLEMENTED;
}

LONG STDCALL 
TabbedTextOutA( HDC hDC,                         
               int X,
			   int Y,
			   LPCSTR lpString,
			   int nCount,
			   int nTabPositions,
			   CONST LPINT lpnTabStopPositions,
			   int nTabOrigin )
{
	UNIMPLEMENTED;
}

LONG STDCALL 
TabbedTextOutW( HDC hDC,                         
               int X,
			   int Y,
			   LPCWSTR lpString,
			   int nCount,
			   int nTabPositions,
			   CONST LPINT lpnTabStopPositions,
			   int nTabOrigin )
{
	UNIMPLEMENTED;
}
