
/*****************************************************************************

                                S T R T O K

    Name:       strtok.c
    Date:       21-Jan-1994
    Creator:    Unknown

    Description:
        This file contains functions for string manipulations.

    History:
        21-Jan-1994     John Fu, cleanup and reformat

*****************************************************************************/



#include <windows.h>
#include "clipbook.h"
#include "strtok.h"




static LPCSTR   lpchAlphaDelimiters;
static LPCWSTR  lpwchAlphaDelimiters;



/*
 *      IsInAlphaA
 */

BOOL IsInAlphaA(
    char    ch)
{
LPCSTR lpchDel = lpchAlphaDelimiters;

    if (ch)
        {
        while (*lpchDel)
            {
            if (ch == *lpchDel++)
                {
                return TRUE;
                }
            }
        }
    else
        {
        return TRUE;
        }

    return FALSE;

}





/*
 *      IsInAlphaW
 */

BOOL IsInAlphaW(
    WCHAR   ch)
{
LPCWSTR lpchDel = lpwchAlphaDelimiters;

    if (ch)
        {
        while (*lpchDel)
            {
            if (ch == *lpchDel++)
                {
                return TRUE;
                }
            }
        }
    else
        {
        return TRUE;
        }

    return FALSE;

}





/*
 *      strtokA
 */

LPSTR strtokA(
    LPSTR   lpchStart,
    LPCSTR  lpchDelimiters)
{
static LPSTR lpchEnd;



    // PINFO("sTRTOK\r\n");

    if (NULL == lpchStart)
        {
        if (lpchEnd)
            {
            lpchStart = lpchEnd + 1;
            }
        else
            {
            return NULL;
            }
        }


    // PINFO("sTRING: %s\r\n", lpchStart);

    lpchAlphaDelimiters = lpchDelimiters;

    if (*lpchStart)
        {
        while (IsInAlphaA(*lpchStart))
            {
            lpchStart++;
            }

        // PINFO("Token: %s\r\n", lpchStart);

        lpchEnd = lpchStart;
        while (*lpchEnd && !IsInAlphaA(*lpchEnd))
            {
            lpchEnd++;
            }

        if (*lpchEnd)
            {
            // PINFO("Found tab\r\n");
            *lpchEnd = '\0';
            }
        else
            {
            // PINFO("Found null\r\n");
            lpchEnd = NULL;
            }
        }
    else
        {
        lpchEnd = NULL;
        return NULL;
        }

    // PINFO("Returning %s\r\n", lpchStart);

    return lpchStart;

}








/*
 *      strtokW
 */

LPWSTR strtokW(
    LPWSTR  lpchStart,
    LPCWSTR lpchDelimiters)
{
static LPWSTR lpchEnd;

    if (NULL == lpchStart)
        {
        if (lpchEnd)
            {
            lpchStart = lpchEnd + 1;
            }
        else
            {
            return NULL;
            }
        }

    lpwchAlphaDelimiters = lpchDelimiters;

    if (*lpchStart)
        {
        while (IsInAlphaW(*lpchStart))
            {
            lpchStart++;
            }

        lpchEnd = lpchStart;
        while (*lpchEnd && !IsInAlphaW(*lpchEnd))
            {
            lpchEnd++;
            }

        if (*lpchEnd)
            {
            *lpchEnd = '\0';
            }
        else
            {
            lpchEnd = NULL;
            }
        }
    else
        {
        lpchEnd = NULL;
        return NULL;
        }

    return lpchStart;
}






/*
 *      ltoa
 *
 *  Purpose: Look, it's ltoa, OK? GO READ K&R.
 *
 *  Parameters: GO READ K&R, YOU SCUM!
 *
 *  Returns: READ K&R! READ K&R! READ K&R! Oh, okay... Returns ptch.
 */

TCHAR *_ltoa(
    long        l,
    TCHAR       *ptch,
    unsigned    uRadix)
{
TCHAR   rgtchDigits[]=TEXT("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");



    if (uRadix < 37)
        {
        unsigned long ul;
        
        if (10 == uRadix && l < 0)
            {
            *ptch++ = TEXT('-');
            ul = (unsigned long)(l = -l); // bugbug: l==0x8000000000000....
            }
        else
            {
            ul = (unsigned long)l;
            }

        // For non-decimal numbers, print all digits.
        if (10 != uRadix)
            {
            l = ((~0L)>>1);
            }

        while (l > 0)
            {
            l /= uRadix;
            ptch++;
            }
        *ptch-- = TEXT('\0');

        do
            {
            *ptch-- = rgtchDigits[ul % uRadix];
            ul /= uRadix;
            } while (ul > 0);

        }
    else
        {
        *ptch = TEXT('\0');
        }


    return(ptch);

}
