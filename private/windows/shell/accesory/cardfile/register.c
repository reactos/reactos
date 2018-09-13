/************************************************************************/
/*                                                                      */
/*  Windows Cardfile - Written by Mark Cliggett                         */
/*  (c) Copyright Microsoft Corp. 1985, 1994 - All Rights Reserved      */
/*                                                                      */
/************************************************************************/

/* Register.C - This file contains all the Registration Database calls.
 */
#include "precomp.h"

void GetClassId(HWND hwnd, LPTSTR lpstrClass) 
{
    DWORD dwSize = KEYNAMESIZE;
    TCHAR szName[KEYNAMESIZE];

    if (!RegQueryValue(HKEY_CLASSES_ROOT, lpstrClass, (LPTSTR)szName, (LONG FAR *)&dwSize))
        SetWindowText(hwnd, szName);
    else
        SetWindowText(hwnd, lpstrClass);
}

int MakeFilterSpec(LPTSTR lpstrClass, LPTSTR lpstrExt, LPTSTR lpstrFilterSpec) 
{
    /* Note:  The registration keys are guaranteed to be ASCII. */
    LONG dwSize;
    TCHAR szClass[KEYNAMESIZE];
    TCHAR szName[KEYNAMESIZE];
    TCHAR szString[KEYNAMESIZE];
    unsigned int i;
    int  idWhich = 0;
    int  idFilterIndex = 0;

    for (i = 0; !RegEnumKey(HKEY_CLASSES_ROOT, i++, szName, KEYNAMESIZE); ) {
        if (*szName == TEXT('.')              /* Default Extension... */
            /* ... so, get the class name */
             && (dwSize = KEYNAMESIZE)
             && !RegQueryValue(HKEY_CLASSES_ROOT, szName, szClass, &dwSize)
            /* ... and if the class name matches */
             && !lstrcmpi(lpstrClass, szClass)
            /* ... get the class name string */
             && (dwSize = KEYNAMESIZE)
             && !RegQueryValue(HKEY_CLASSES_ROOT, szClass, szString, &dwSize))
         {
            idWhich++;        /* Which item of the combo box is it? */
            /* If the extension matches, save the filter index */
            if (!lstrcmpi(lpstrExt, szName))
                idFilterIndex = idWhich;

            /* Copy over "<Class Name String> (*<Default Extension>)"
             * e.g. "Server Picture (*.PIC)"
             */
            lstrcpy(lpstrFilterSpec, szString);
            lstrcat(lpstrFilterSpec, TEXT(" (*"));
            lstrcat(lpstrFilterSpec, szName);
            lstrcat(lpstrFilterSpec, TEXT(")"));
            lpstrFilterSpec += lstrlen(lpstrFilterSpec) + 1;

            /* Copy over "*<Default Extension>" (e.g. "*.PIC") */
            lstrcpy(lpstrFilterSpec, TEXT("*"));
            lstrcat(lpstrFilterSpec, szName);
            lpstrFilterSpec += lstrlen(lpstrFilterSpec) + 1;
        }
    }

    /* Add another NULL at the end of the spec */
    *lpstrFilterSpec = 0;

    return idFilterIndex;
}
