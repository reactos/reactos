#include <crtdll/wchar.h>

int _wcsnicmp(const wchar_t * cs,const wchar_t * ct,size_t count)
{
  wchar_t *save = (wchar_t *)cs;
  while (towlower(*cs) == towlower(*ct) && (int)(cs - save) < count)
  {
    if (*cs == 0)
      return 0;
    cs++;
    ct++;
  }
  return towlower(*cs) - towlower(*ct);
}
