#include <windows.h>
#include <msvcrt/stdio.h>
#include <msvcrt/string.h>


char *tmpnam(char *s)
{
  char PathName[MAX_PATH];
  static char static_buf[MAX_PATH];

  GetTempPathA(MAX_PATH, PathName);
  GetTempFileNameA(PathName, "ARI", 007, static_buf);
  strcpy(s, static_buf);

  return s;
}

wchar_t *_wtmpnam(wchar_t *s)
{
  wchar_t PathName[MAX_PATH];
  static wchar_t static_buf[MAX_PATH];

  GetTempPathW(MAX_PATH, PathName);
  GetTempFileNameW(PathName, L"ARI", 007, static_buf);
  wcscpy(s, static_buf);

  return s;
}
