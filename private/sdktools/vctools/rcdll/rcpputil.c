/************************************************************************/
/*                                                                      */
/* RCPP - Resource Compiler Pre-Processor for NT system                 */
/*                                                                      */
/* RCPPTIL.C - Utility routines for RCPP                                */
/*                                                                      */
/* 27-Nov-90 w-BrianM  Update for NT from PM SDK RCPP                   */
/*                                                                      */
/************************************************************************/

#include "rc.h"

extern void     error(int);
extern CHAR     Msg_Text[];
extern PCHAR    Msg_Temp;

/************************************************************************
 * PSTRDUP - Create a duplicate of string s and return a pointer to it.
 ************************************************************************/
WCHAR *
pstrdup(
    WCHAR *s
    )
{
    return(wcscpy((WCHAR *)MyAlloc((wcslen(s) + 1) * sizeof(WCHAR)), s));
}


/************************************************************************
**  pstrndup : copies n bytes from the string to a newly allocated
**  near memory location.
************************************************************************/
WCHAR *
pstrndup(
    WCHAR *s,
    int n
    )
{
    WCHAR   *r;
    WCHAR   *res;

    r = res = (WCHAR *) MyAlloc((n+1) * sizeof(WCHAR));
    if (res == NULL) {
        strcpy (Msg_Text, GET_MSG (1002));
        error(1002);
        return NULL;
    }

    __try {
        for (; n--; r++, s++) {
            *r = *s;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        n++;
        while (n--) {
            *r++ = L'\0';
        }
    }
    *r = L'\0';
    return(res);
}


/************************************************************************
**      strappend : appends src to the dst,
**  returns a ptr in dst to the null terminator.
************************************************************************/
WCHAR *
strappend(
    register WCHAR *dst,
    register WCHAR *src
    )
{
    while ((*dst++ = *src++) != 0);
    return(--dst);
}
