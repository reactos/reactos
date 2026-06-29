/***
*strspn.c - find length of initial substring of chars from a control string
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines strspn() - finds the length of the initial substring of
*       a string consisting entirely of characters from a control string.
*
*       defines strcspn()- finds the length of the initial substring of
*       a string consisting entirely of characters not in a control string.
*
*       defines strpbrk()- finds the index of the first character in a string
*       that is not in a control string
*
*******************************************************************************/

#include <string.h>

#pragma warning(disable:__WARNING_POTENTIAL_BUFFER_OVERFLOW_NULLTERMINATED) // 26018 Prefast doesn't understand reading past buffer but staying on same page.
#pragma warning(disable:__WARNING_RETURNING_BAD_RESULT) // 28196

/* Determine which routine we're compiling for (default to STRSPN) */
#define _STRSPN         1
#define _STRCSPN        2
#define _STRPBRK        3

#if defined (SSTRCSPN)
    #define ROUTINE _STRCSPN
#elif defined (SSTRPBRK)
    #define ROUTINE _STRPBRK
#else
    #define ROUTINE _STRSPN
#endif

/***
*int strspn(string, control) - find init substring of control chars
*
*Purpose:
*       Finds the index of the first character in string that does belong
*       to the set of characters specified by control.  This is
*       equivalent to the length of the initial substring of string that
*       consists entirely of characters from control.  The '\0' character
*       that terminates control is not considered in the matching process.
*
*Entry:
*       char *string - string to search
*       char *control - string containing characters not to search for
*
*Exit:
*       returns index of first char in string not in control
*
*Exceptions:
*
*******************************************************************************/

/***
*int strcspn(string, control) - search for init substring w/o control chars
*
*Purpose:
*       returns the index of the first character in string that belongs
*       to the set of characters specified by control.  This is equivalent
*       to the length of the length of the initial substring of string
*       composed entirely of characters not in control.  Null chars not
*       considered.
*
*Entry:
*       char *string - string to search
*       char *control - set of characters not allowed in init substring
*
*Exit:
*       returns the index of the first char in string
*       that is in the set of characters specified by control.
*
*Exceptions:
*
*******************************************************************************/

/***
*char *strpbrk(string, control) - scans string for a character from control
*
*Purpose:
*       Finds the first occurence in string of any character from
*       the control string.
*
*Entry:
*       char *string - string to search in
*       char *control - string containing characters to search for
*
*Exit:
*       returns a pointer to the first character from control found
*       in string.
*       returns NULL if string and control have no characters in common.
*
*Exceptions:
*
*******************************************************************************/



/* Routine prototype */
#if ROUTINE == _STRSPN
size_t __cdecl strspn (
#elif ROUTINE == _STRCSPN
size_t __cdecl strcspn (
#else  /* ROUTINE == _STRCSPN */
char * __cdecl strpbrk (
#endif  /* ROUTINE == _STRCSPN */
        const char * string,
        const char * control
        )
{
        const unsigned char *str = (unsigned char const*)string;
        const unsigned char *ctrl = (unsigned char const*)control;

        unsigned char map[32];
        int count;

        /* Clear out bit map */
        for (count=0; count<32; count++)
                map[count] = 0;

        /* Set bits in control map */
        while (*ctrl)
        {
                map[*ctrl >> 3] |= (1 << (*ctrl & 7));
                ctrl++;
        }

#if ROUTINE == _STRSPN

        /* 1st char NOT in control map stops search */
        if (*str)
        {
                count=0;
                while (map[*str >> 3] & (1 << (*str & 7)))
                {
                        count++;
                        str++;
                }
                return(count);
        }
        return(0);

#elif ROUTINE == _STRCSPN

        /* 1st char in control map stops search */
        count=0;
        map[0] |= 1;    /* null chars not considered */
        while (!(map[*str >> 3] & (1 << (*str & 7))))
        {
                count++;
                str++;
        }
        return(count);

#else  /* ROUTINE == _STRCSPN */

        /* 1st char in control map stops search */
        while (*str)
        {
                if (map[*str >> 3] & (1 << (*str & 7)))
                        return((char *)str);
                str++;
        }
        return(NULL);

#endif  /* ROUTINE == _STRCSPN */

}
