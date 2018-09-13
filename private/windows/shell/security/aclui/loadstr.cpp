#include "aclpriv.h"

#include "loadstr.h"

//*************************************************************
//
//  SizeofStringResource
//
//  Purpose:    Find the length (in chars) of a string resource
//
//  Parameters: HINSTANCE hInstance - module containing the string
//              UINT idStr - ID of string
//
//
//  Return:     UINT - # of chars in string, not including NULL
//
//  Notes:      Based on code from user32.
//
//*************************************************************
UINT
SizeofStringResource(HINSTANCE hInstance,
                     UINT idStr)
{
    UINT cch = 0;
    HRSRC hRes = FindResource(hInstance, (LPTSTR)((LONG)(((USHORT)idStr >> 4) + 1)), RT_STRING);
    if (NULL != hRes)
    {
        HGLOBAL hStringSeg = LoadResource(hInstance, hRes);
        if (NULL != hStringSeg)
        {
            LPWSTR psz = (LPWSTR)LockResource(hStringSeg);
            if (NULL != psz)
            {
                idStr &= 0x0F;
                for(;;)
                {
                    cch = *psz++;
                    if (idStr-- == 0)
                        break;
                    psz += cch;
                }
            }
        }
    }
    return cch;
}
