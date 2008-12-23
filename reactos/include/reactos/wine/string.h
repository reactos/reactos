#ifndef __WINE_STRING_H
#define __WINE_STRING_H

#include_next <string.h>

#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define stricmp _stricmp
#define wcsicmp _wcsicmp


#endif /* !__WINE_STRING_H */
