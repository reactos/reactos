#include <msvcrt/stdlib.h>

size_t mbstowcs( wchar_t *wcstr, const char *mbstr, size_t count )
{
	size_t Size;
	return (size_t)Size;
}

/*
	NTSTATUS Status;
	ULONG Size;
	ULONG Length;

	Length = strlen (mbstr);

	if (wcstr == NULL)
	{
		RtlMultiByteToUnicodeSize (&Size,
		                           (char *)mbstr,
		                           Length);

		return (size_t)Size;
	}

	Status = RtlMultiByteToUnicodeN (wcstr,
	                                 count,
	                                 &Size,
	                                 (char *)mbstr,
	                                 Length);
	if (!NT_SUCCESS(Status))
		return -1;
 */

//int mbtowc( wchar_t *wchar, const char *mbchar, size_t count )
//{
//	return 0;
//}
