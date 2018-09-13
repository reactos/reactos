#ifndef _CRTBLD
#define _CRTBLD
#endif

#include <windows.h>
#include <shlwapi.h>

/***
* double StrToDbl(const char *str, char **strStop) - convert string to double
*
* Purpose:
*           To convert a string into a double.  This function supports
*           simple double representations like '1.234', '.5678'.  It also support
*           the a killobyte computaion by appending 'k' to the end of the string
*           as in '1.5k' or '.5k'.  The results would then become 1536 and 512.5.
*
* Return:
*           The double representation of the string.
*           strStop points to the character that caused the scan to stop.
*
*******************************************************************************/

double __cdecl StrToDbl(const char *str, char **strStop)
{
    double dbl = 0;
    char *psz;
    int iMult = 1;
    int iKB = 1;
    int iVal = 0;
    BOOL bHaveDot = FALSE;

    psz = (char*)str;
    while(*psz)
    {
        if((*psz >= '0') && (*psz <= '9'))
        {
            iVal = (iVal * 10) + (*psz - '0');
            if(bHaveDot)
                iMult *= 10;
        }
        else if((*psz == '.') && !bHaveDot)
        {
            bHaveDot = TRUE;
        }
        else if((*psz == 'k') || (*psz == 'K'))
        {
            iKB = 1024;
            psz++;
            break;
        }
        else
        {
            break;
        }
        psz++;
    }
    *strStop = psz;

    dbl = (double) (iVal * iKB) / iMult;
    
    return(dbl);
}

