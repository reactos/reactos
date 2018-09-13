/*
 * util.c - Utility routines.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop


/****************************** Public Functions *****************************/


#ifdef WINNT

//
//  These are some helper functions for handling Unicode strings
//

/*----------------------------------------------------------
Purpose: This function converts a wide-char string to a multi-byte
         string.

         If pszBuf is non-NULL and the converted string can fit in 
         pszBuf, then *ppszAnsi will point to the given buffer.
         Otherwise, this function will allocate a buffer that can
         hold the converted string.

         If pszWide is NULL, then *ppszAnsi will be freed.  Note
         that pszBuf must be the same pointer between the call
         that converted the string and the call that frees the 
         string.

Returns: TRUE 
         FALSE (if out of memory)

Cond:    --
*/
PRIVATE_CODE 
BOOL 
AnsiFromUnicode(
    LPSTR * ppszAnsi,
    LPCWSTR pwszWide,        // NULL to clean up
    LPSTR pszBuf,
    int cchBuf)
    {
    BOOL bRet;

    // Convert the string?
    if (pwszWide)
        {
        // Yes; determine the converted string length
        int cch;
        LPSTR psz;

        cch = WideCharToMultiByte(CP_ACP, 0, pwszWide, -1, NULL, 0, NULL, NULL);

        // String too big, or is there no buffer?
        if (cch > cchBuf || NULL == pszBuf)
            {
            // Yes; allocate space
            cchBuf = cch + 1;
            psz = (LPSTR)LocalAlloc(LPTR, CbFromCchA(cchBuf));
            }
        else
            {
            // No; use the provided buffer
            ASSERT(pszBuf);
            psz = pszBuf;
            }

        if (psz)
            {
            // Convert the string
            cch = WideCharToMultiByte(CP_ACP, 0, pwszWide, -1, psz, cchBuf, NULL, NULL);
            bRet = (0 < cch);
            }
        else
            {
            bRet = FALSE;
            }

        *ppszAnsi = psz;
        }
    else
        {
        // No; was this buffer allocated?
        if (*ppszAnsi && pszBuf != *ppszAnsi)
            {
            // Yes; clean up
            LocalFree((HLOCAL)*ppszAnsi);
            *ppszAnsi = NULL;
            }
        bRet = TRUE;
        }

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Wide-char wrapper for StrToIntExA.

Returns: see StrToIntExA
Cond:    --
*/
PUBLIC_CODE
BOOL 
StrToIntExW(
    LPCWSTR   pwszString,
    DWORD     dwFlags,          // STIF_ bitfield 
    int FAR * piRet)
    {
    CHAR szBuf[MAX_BUF];
    LPSTR pszString;
    BOOL bRet = AnsiFromUnicode(&pszString, pwszString, szBuf, SIZECHARS(szBuf));

    if (bRet)
        {
        bRet = StrToIntExA(pszString, dwFlags, piRet);
        AnsiFromUnicode(&pszString, NULL, szBuf, 0);
        }
    return bRet;
    }


/*----------------------------------------------------------
Purpose: Returns an integer value specifying the length of
         the substring in psz that consists entirely of 
         characters in pszSet.  If psz begins with a character
         not in pszSet, then this function returns 0.

         This is a DBCS-safe version of the CRT strspn().  

Returns: see above
Cond:    --
*/
PUBLIC_CODE
int
StrSpnW(
    LPCWSTR psz,
    LPCWSTR pszSet)
    {
    LPCWSTR pszT;
    LPCWSTR pszSetT;

    ASSERT(psz);
    ASSERT(pszSet);

    // Go thru the string to be inspected 

    for (pszT = psz; *pszT; pszT++)
        {
        // Go thru the char set

        for (pszSetT = pszSet; *pszSetT != *pszT; pszSetT++)
            {
            if (0 == *pszSetT)
                {
                // Reached end of char set without finding a match
                return (int)(pszT - psz);
                }
            }
        }

    return (int)(pszT - psz);
    }


/*----------------------------------------------------------
Purpose: Returns a pointer to the first occurrence of a character
         in psz that belongs to the set of characters in pszSet.
         The search does not include the null terminator.

Returns: see above
Cond:    --
*/
PUBLIC_CODE
LPWSTR
StrPBrkW(
    IN LPCWSTR psz,
    IN LPCWSTR pszSet)
    {
    LPCWSTR pszSetT;

    ASSERT(psz);
    ASSERT(pszSet);

    // Go thru the string to be inspected 

    while (*psz)
        {
        // Go thru the char set

        for (pszSetT = pszSet; *pszSetT; pszSetT++)
            {
            if (*psz == *pszSetT)
                {
                // Found first character that matches
                return (LPWSTR)psz;     // Const -> non-const
                }
            }
        psz++;
        }

    return NULL;
    }

#endif // WINNT


/*----------------------------------------------------------
Purpose: Special verion of atoi.  Supports hexadecimal too.

         If this function returns FALSE, *piRet is set to 0.

Returns: TRUE if the string is a number, or contains a partial number
         FALSE if the string is not a number

Cond:    --
*/
PUBLIC_CODE
BOOL 
StrToIntExA(
    LPCSTR    pszString,
    DWORD     dwFlags,          // STIF_ bitfield 
    int FAR * piRet)
    {
    #define IS_DIGIT(ch)    InRange(ch, '0', '9')

    BOOL bRet;
    int n;
    BOOL bNeg = FALSE;
    LPCSTR psz;
    LPCSTR pszAdj;

    // Skip leading whitespace
    //
    for (psz = pszString; *psz == ' ' || *psz == '\n' || *psz == '\t'; psz = CharNextA(psz))
        ;
      
    // Determine possible explicit signage
    //  
    if (*psz == '+' || *psz == '-')
        {
        bNeg = (*psz == '+') ? FALSE : TRUE;
        psz++;
        }

    // Or is this hexadecimal?
    //
    pszAdj = CharNextA(psz);
    if ((STIF_SUPPORT_HEX & dwFlags) &&
        *psz == '0' && (*pszAdj == 'x' || *pszAdj == 'X'))
        {
        // Yes

        // (Never allow negative sign with hexadecimal numbers)
        bNeg = FALSE;   
        psz = CharNextA(pszAdj);

        pszAdj = psz;

        // Do the conversion
        //
        for (n = 0; ; psz = CharNextA(psz))
            {
            if (IS_DIGIT(*psz))
                n = 0x10 * n + *psz - '0';
            else
                {
                CHAR ch = *psz;
                int n2;

                if (ch >= 'a')
                    ch -= 'a' - 'A';

                n2 = ch - 'A' + 0xA;
                if (n2 >= 0xA && n2 <= 0xF)
                    n = 0x10 * n + n2;
                else
                    break;
                }
            }

        // Return TRUE if there was at least one digit
        bRet = (psz != pszAdj);
        }
    else
        {
        // No
        pszAdj = psz;

        // Do the conversion
        for (n = 0; IS_DIGIT(*psz); psz = CharNextA(psz))
            n = 10 * n + *psz - '0';

        // Return TRUE if there was at least one digit
        bRet = (psz != pszAdj);
        }

    *piRet = bNeg ? -n : n;

    return bRet;
    }    


/*----------------------------------------------------------
Purpose: Returns an integer value specifying the length of
         the substring in psz that consists entirely of 
         characters in pszSet.  If psz begins with a character
         not in pszSet, then this function returns 0.

         This is a DBCS-safe version of the CRT strspn().  

Returns: see above
Cond:    --
*/
PUBLIC_CODE
int
StrSpnA(
    LPCSTR psz,
    LPCSTR pszSet)
    {
    LPCSTR pszT;
    LPCSTR pszSetT;

    // Go thru the string to be inspected 

    for (pszT = psz; *pszT; pszT = CharNextA(pszT))
        {
        // Go thru the char set

        for (pszSetT = pszSet; *pszSetT; pszSetT = CharNextA(pszSetT))
            {
            if (*pszSetT == *pszT)
                {
                if ( !IsDBCSLeadByte(*pszSetT) )
                    {
                    break;      // Chars match
                    }
                else if (pszSetT[1] == pszT[1])
                    {
                    break;      // Chars match
                    }
                }
            }

        // End of char set?
        if (0 == *pszSetT)
            {
            break;      // Yes, no match on this inspected char
            }
        }

    return (int)(pszT - psz);
    }


/*----------------------------------------------------------
Purpose: Returns a pointer to the first occurrence of a character
         in psz that belongs to the set of characters in pszSet.
         The search does not include the null terminator.

         If psz contains no characters that are in the set of
         characters in pszSet, this function returns NULL.

         This function is DBCS-safe.

Returns: see above
Cond:    --
*/
PUBLIC_CODE
LPSTR
StrPBrkA(
    LPCSTR psz,
    LPCSTR pszSet)
    {
    LPCSTR pszSetT;

    ASSERT(psz);
    ASSERT(pszSet);

    while (*psz)
        {
        for (pszSetT = pszSet; *pszSetT; pszSetT = CharNextA(pszSetT))
            {
            if (*psz == *pszSetT)
                {
                // Found first character that matches
                return (LPSTR)psz;      // Const -> non-const
                }
            }
        psz = CharNextA(psz);
        }

    return NULL;
    }





PUBLIC_CODE BOOL IsPathDirectory(PCSTR pcszPath)
{
   DWORD dwAttr;

   ASSERT(IS_VALID_STRING_PTR(pcszPath, CSTR));

   dwAttr = GetFileAttributes(pcszPath);

   return(dwAttr != -1 &&
          IS_FLAG_SET(dwAttr, FILE_ATTRIBUTE_DIRECTORY));
}


PUBLIC_CODE BOOL KeyExists(HKEY hkeyRoot, PCSTR pcszSubKey)
{
   BOOL bExists;
   HKEY hkey;

   ASSERT(IS_VALID_HANDLE(hkeyRoot, KEY));
   ASSERT(IS_VALID_STRING_PTR(pcszSubKey, CSTR));

   bExists = (RegOpenKey(hkeyRoot, pcszSubKey, &hkey) == ERROR_SUCCESS);

   if (bExists)
      EVAL(RegCloseKey(hkey) == ERROR_SUCCESS);

   return(bExists);
}


#ifdef DEBUG

PUBLIC_CODE BOOL IsStringContained(PCSTR pcszBigger, PCSTR pcszSuffix)
{
   ASSERT(IS_VALID_STRING_PTR(pcszBigger, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszSuffix, CSTR));

   return(pcszSuffix >= pcszBigger &&
          pcszSuffix <= pcszBigger + lstrlen(pcszBigger));
}

#endif
