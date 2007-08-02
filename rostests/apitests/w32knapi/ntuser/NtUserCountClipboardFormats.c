#include "../w32knapi.h"

/* First the call stub */
DWORD STDCALL
NtUserCountClipboardFormats(VOID)
{
	DWORD p;
	return Syscall(L"NtUserCountClipboardFormats", 0, &p);
}

BOOL
Test_NtUserCountClipboardFormats(PTESTINFO pti)
{
	TEST(NtUserCountClipboardFormats() < 1000);
	TEST(TRUE);
	return TRUE;
}

