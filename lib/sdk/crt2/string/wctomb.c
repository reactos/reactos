/*
 * @implemented
 */
int _CDECL wctomb(char *mbchar, wchar_t wchar)
{
	NTSTATUS Status;
	ULONG Size;

	if (mbchar == NULL)
		return 0;

	Status = RtlUnicodeToMultiByteN (mbchar,
	                                 1,
	                                 &Size,
	                                 &wchar,
	                                 sizeof(WCHAR));
	if (!NT_SUCCESS(Status))
		return -1;

	return (int)Size;
}

