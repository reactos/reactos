
/*****************************************************************************

                            S T R T O K   H E A D E R

    Name:       strtok.h
    Date:       21-Jan-1994
    Creator:    John Fu

    Description:
        This is the header file for strtok.c

*****************************************************************************/



BOOL IsInAlphaA(
    char    ch);


BOOL IsInAlphaW(
    WCHAR   ch);


LPSTR strtokA(
    LPSTR   lpchStart,
    LPCSTR  lpchDelimiters);


LPWSTR strtokW(
    LPWSTR  lpchStart,
    LPCWSTR lpchDelimiters);


TCHAR *ltoa(
    long        l,
    TCHAR       *ptch,
    unsigned    uRadix);
