#include <msvcrt/stdlib.h>


//int	mblen		(const char* mbs, size_t sizeString)
//{
//	return 0;
//}

//size_t	mbstowcs	(wchar_t* wcaDest, const char* mbsConvert, size_t size)
//{
//	return 0;
//}

//int mbtowc(wchar_t* wcDest, const char* mbConvert, size_t size)
int mbtowc(wchar_t *wchar, const char *mbchar, size_t count)
{
    *wchar = (wchar_t)*mbchar;
    return 1;

             // WideCharToMultiByte
             // MultiByteToWideChar
/*
int MultiByteToWideChar(
  UINT CodePage,         // code page
  DWORD dwFlags,         // character-type options
  LPCSTR lpMultiByteStr, // string to map
  int cbMultiByte,       // number of bytes in string
  LPWSTR lpWideCharStr,  // wide-character buffer
  int cchWideChar        // size of buffer
);
 */
/*
	NTSTATUS Status;
	ULONG Size;

	if (wchar == NULL)
		return 0;

	Status = RtlMultiByteToUnicodeN (wchar,
	                                 sizeof(WCHAR),
	                                 &Size,
	                                 (char *)mbchar,
	                                 count);
	if (!NT_SUCCESS(Status))
		return -1;

	return (int)Size;
 */
}
