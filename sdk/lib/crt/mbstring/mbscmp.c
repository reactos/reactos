#include <mbstring.h>
#include <string.h>
#include <precomp.h>

/*
 * @implemented
 */
int _mbscmp(const unsigned char *str1, const unsigned char *str2)
{
    if (!MSVCRT_CHECK_PMT(str1 && str2))
        return _NLSCMPERROR;

  return strcmp((const char*)str1, (char*)str2);
}
