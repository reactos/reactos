#include <windows.h>
#include <msvcrt/stdio.h>
#include <msvcrt/stdlib.h>


char *_tempnam(const char *dir,const char *prefix )
{
  char *TempFileName = malloc(MAX_PATH);
  char *d;

  if (dir == NULL)
    d = getenv("TMP");
  else
    d = (char *)dir;

  if (GetTempFileNameA(d, prefix, 1, TempFileName) == 0)
    {
      free(TempFileName);
      return NULL;
    }

  return TempFileName;
}

wchar_t *_wtempnam(const wchar_t *dir,const wchar_t *prefix)
{
  wchar_t *TempFileName = malloc(MAX_PATH);
  wchar_t *d;

  if (dir == NULL)
    d = _wgetenv(L"TMP");
  else 
    d = (wchar_t *)dir;

  if (GetTempFileNameW(d, prefix, 1, TempFileName) == 0)
    {
      free(TempFileName);
      return NULL;
    }

  return TempFileName;
}
