#ifdef UNICODE
#undef UNICODE
#endif

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/kapi.h>

BOOL  
STDCALL 
TextOutA(
	HDC  hDC,
	int  XStart,
	int  YStart,
	LPCSTR  String,
	int  Count)
{
        ANSI_STRING StringA;
        UNICODE_STRING StringU;
	BOOL ret;

	if (NULL != String)
	{
		RtlInitAnsiString(&StringA, (LPSTR)String);
		RtlAnsiStringToUnicodeString(&StringU, &StringA, TRUE);
	} else
		StringU.Buffer = NULL;

	ret = TextOutW(hDC, XStart, YStart, StringU.Buffer, Count);
	RtlFreeUnicodeString(&StringU);
	return ret;
}

BOOL
STDCALL
TextOutW(
	HDC  hDC,
	int  XStart,
	int  YStart,
	LPCWSTR  String,
	int  Count)
{
	return W32kTextOut(hDC, XStart, YStart, String, Count);
}
