#ifdef __unix__
#include <string.h>
#include <stdlib.h>
#include <wchar.h>

typedef unsigned short UINT;
typedef unsigned int DWORD;
typedef const unsigned char* LPCSTR;
typedef wchar_t* LPWSTR;
typedef unsigned char* LPSTR;
typedef const wchar_t* LPCWSTR;
typedef unsigned int* LPBOOL;

int
stricmp(const char* s1, const char* s2)
{
  return(strcasecmp(s1, s2));
}

int
WideCharToMultiByte(
    UINT     CodePage,
    DWORD    dwFlags,
    LPCWSTR  lpWideCharStr,
    int      cchWideChar,
    LPSTR    lpMultiByteStr,
    int      cchMultiByte,
    LPCSTR   lpDefaultChar,
    LPBOOL   lpUsedDefaultChar)
{
  unsigned int i = 0;
  if (cchWideChar == -1)
    {
      while(*lpWideCharStr != 0)
	{
	  wctomb(lpMultiByteStr, *lpWideCharStr);
	  lpMultiByteStr++;
	  lpWideCharStr++;
	  i++;
	}
    }
  else
    {
      while(i < cchWideChar)
	{
	  wctomb(lpMultiByteStr, *lpWideCharStr);
	  lpMultiByteStr++;
	  lpWideCharStr++;
	  i++;
	}
    }
  return(i);
}

int
MultiByteToWideChar(
    UINT     CodePage,
    DWORD    dwFlags,
    LPCSTR   lpMultiByteStr,
    int      cchMultiByte,
    LPWSTR   lpWideCharStr,
    int      cchWideChar)
{
  int i, j;
  
  i = j = 0;
  while (i < cchMultiByte && lpMultiByteStr[i] != 0)
    {
      i = i + mbtowc(&lpWideCharStr[j], &lpMultiByteStr[i], 
		     cchMultiByte - i);
      j++;
    }
  lpWideCharStr[j] = 0;
  j++;
  return(j);
}
#endif
