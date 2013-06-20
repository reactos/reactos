/* First the call stub */
DWORD WINAPI
NtUserCountClipboardFormats(VOID)
{
	DWORD p;
	return Syscall(L"NtUserCountClipboardFormats", 0, &p);
}

INT
Test_NtUserCountClipboardFormats(PTESTINFO pti)
{
	RTEST(NtUserCountClipboardFormats() < 1000);
	return APISTATUS_NORMAL;
}

