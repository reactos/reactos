/*
 * @implemented
 */
size_t _CDECL mbstowcs (wchar_t *wcstr, const char *mbstr, size_t count)
{
	NTSTATUS Status;
	ULONG Size;
	ULONG Length;

	Length = strlen (mbstr);

	if(wcstr == NULL)
	{
		RtlMultiByteToUnicodeSize(&Size, mbstr, Length);
		return (size_t)Size;
	}

	Status = RtlMultiByteToUnicodeN(wcstr, count, &Size, mbstr, Length);
	if(!NT_SUCCESS(Status))
		return -1;

	return (size_t)Size;
}
