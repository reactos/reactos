#include <msvcrti.h>


int wcsncmp(const wchar_t * cs,const wchar_t * ct,size_t count)
{
  while ((*cs) == (*ct) && count > 0)
  {
    if (*cs == 0)
      return 0;
    cs++;
    ct++;
    count--;
  }
  return (*cs) - (*ct);
	
}

