/*
 * @implemented
 */
int _CDECL mbtowc(wchar_t *wchar, const char *mbchar, size_t count)
{
	NTSTATUS Status;
	ULONG Size;

	if(wchar == NULL)
		return 0;

	Status = RtlMultiByteToUnicodeN(wchar,
	                                 sizeof(WCHAR),
	                                 &Size,
	                                 mbchar,
	                                 count);
	if (!NT_SUCCESS(Status))
		return -1;

	return (int)Size;
}

