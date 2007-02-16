#define WIN32_NO_STATUS
#include <windows.h>
#include <ndk/umtypes.h>
#include <ndk/rtlfuncs.h>
#include <string.h>

/*
 * @implemented
 */
int mbtowc (wchar_t *wchar, const char *mbchar, size_t count)
{
	NTSTATUS Status;
	ULONG Size;

	if (wchar == NULL)
		return 0;

	Status = RtlMultiByteToUnicodeN (wchar,
	                                 sizeof(WCHAR),
	                                 &Size,
	                                 mbchar,
	                                 count);
	if (!NT_SUCCESS(Status))
		return -1;

	return (int)Size;
}

/*
 * @implemented
 */
size_t mbstowcs (wchar_t *wcstr, const char *mbstr, size_t count)
{
	NTSTATUS Status;
	ULONG Size;
	ULONG Length;

	Length = strlen (mbstr);

	if (wcstr == NULL)
	{
		RtlMultiByteToUnicodeSize (&Size,
		                           mbstr,
		                           Length);

		return (size_t)Size;
	}

	Status = RtlMultiByteToUnicodeN (wcstr,
	                                 count,
	                                 &Size,
	                                 mbstr,
	                                 Length);
	if (!NT_SUCCESS(Status))
		return -1;

	return (size_t)Size;
}

/* EOF */
