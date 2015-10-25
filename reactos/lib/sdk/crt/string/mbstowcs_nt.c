#include <ndk/umtypes.h>
#include <ndk/rtlfuncs.h>
#include <string.h>

#undef MB_CUR_MAX
#define MB_CUR_MAX 2

/*
 * @implemented
 */
int mbtowc (wchar_t *wchar, const char *mbchar, size_t count)
{
	UCHAR mbarr[MB_CUR_MAX] = { 0 };
	PUCHAR mbs = mbarr;
	WCHAR wc;

	if (mbchar == NULL)
		return 0;

	if (wchar == NULL)
		return 0;

	memcpy(mbarr, mbchar, min(count, sizeof mbarr));

	wc = RtlAnsiCharToUnicodeChar(&mbs);

	if (wc == L' ' && mbarr[0] != ' ')
		return -1;

	*wchar = wc;

	return (int)(mbs - mbarr);
}

/*
 * @implemented
 */
size_t mbstowcs (wchar_t *wcstr, const char *mbstr, size_t count)
{
	NTSTATUS Status;
	ULONG Size;
	ULONG Length;

	Length = (ULONG)strlen (mbstr);

	if (wcstr == NULL)
	{
		RtlMultiByteToUnicodeSize (&Size,
		                           mbstr,
		                           Length);

		return (size_t)(Size / sizeof(wchar_t));
	}

	Status = RtlMultiByteToUnicodeN (wcstr,
	                                 (ULONG)count * sizeof(WCHAR),
	                                 &Size,
	                                 mbstr,
	                                 Length);
	if (!NT_SUCCESS(Status))
		return -1;

	return (size_t)(Size / sizeof(wchar_t));;
}

/* EOF */
