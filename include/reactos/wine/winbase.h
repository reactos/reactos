#ifndef __WINE_WINBASE_H
#define __WINE_WINBASE_H

#include_next <winbase.h>

/* undocumented functions */

typedef struct tagSYSLEVEL
{
    CRITICAL_SECTION crst;
    INT              level;
} SYSLEVEL;


#endif /* __WINE_WINBASE_H */
