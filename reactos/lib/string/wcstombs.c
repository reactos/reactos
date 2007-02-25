#define WIN32_NO_STATUS
#include <windows.h>
#include <ndk/umtypes.h>
#include <ndk/rtlfuncs.h>

/*
 * @implemented
 */
int wctomb (char *mbchar, wchar_t wchar)
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

/*
 * @implemented
 */
size_t wcstombs (char *mbstr, const wchar_t *wcstr, size_t count)
{
	NTSTATUS Status;
	ULONG Size;
	ULONG Length;

	Length = wcslen (wcstr);

	if (mbstr == NULL)
	{
		RtlUnicodeToMultiByteSize (&Size,
		                           (wchar_t*)((size_t)wcstr),
		                           Length * sizeof(WCHAR));

		return (size_t)Size;
	}

	Status = RtlUnicodeToMultiByteN (mbstr,
	                                 count,
	                                 &Size,
	                                 (wchar_t*)((size_t)wcstr),
	                                 Length * sizeof(WCHAR));
	if (!NT_SUCCESS(Status))
		return -1;

	return (size_t)Size;
}

/* EOF */
