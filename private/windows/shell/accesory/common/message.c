#include "windows.h"
#include <port1632.h>

VOID    FAR InitMerge();
INT     FAR AlertBox();

BOOL  FAR MergeStrings(CHAR    *szSrc,
                       CHAR    *szMerge,
                       CHAR    *szDst);

WORD wMerge;

/* ** Post a message box */
INT FAR AlertBox(hwndParent, szCaption, szText1, szText2, style)
HWND    hwndParent;
CHAR    *szCaption;
CHAR    *szText1;
CHAR    *szText2;
WORD style;
{
    CHAR    szMessage[256];

    MergeStrings(szText1, szText2, szMessage);

    return (MessageBox(hwndParent, (LPSTR)szMessage, (LPSTR)szCaption, style));
}


/* ** Scan sz1 for merge spec.  If found, insert string sz2 at that point.
      Then append rest of sz1 NOTE! Merge spec guaranteed to be two chars.
      returns TRUE if it does a merge, false otherwise. */
BOOL  FAR MergeStrings(szSrc, szMerge, szDst)
CHAR    *szSrc;
CHAR    *szMerge;
CHAR    *szDst;
{
    register    CHAR *pchSrc;
    register    CHAR *pchDst;

    pchSrc = szSrc;
    pchDst = szDst;

#ifndef UNICODE
    /* Find merge spec if there is one. */
    while (*(WORD *)pchSrc != wMerge) {
        if( IsDBCSLeadByte( *pchSrc ) )
            *pchDst++ = *pchSrc++;
        *pchDst++ = *pchSrc;

        /* If we reach end of string before merge spec, just return. */
        if(!*pchSrc++)
            return FALSE;
    }
#else
    /* Find merge spec if there is one. */
    while (*(WORD *)pchSrc != wMerge) {
        *pchDst++ = *pchSrc;

        /* If we reach end of string before merge spec, just return. */
        if (!*pchSrc++)
            return FALSE;
    }
#endif

    /* If merge spec found, insert sz2 there. (check for null merge string */
    if (szMerge) {
        while (*szMerge)
            *pchDst++ = *szMerge++;

    }

    /* Jump over merge spec */
    pchSrc++,pchSrc++;


    /* Now append rest of Src String */
    while (*pchDst++ = *pchSrc++);
    return TRUE;

}


VOID MergeInit(sz)
CHAR    *sz;
{
    wMerge = *(WORD *)sz;
}
