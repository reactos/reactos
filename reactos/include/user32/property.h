#ifndef PROPERTY_H
#define PROPERTY_H

#include <user32/win.h>

typedef struct tagPROPERTY
{
    struct tagPROPERTY *next;     /* Next property in window list */
    HANDLE            handle;   /* User's data */
    LPSTR             string;   /* Property string (or atom) */
} PROPERTY;




HANDLE PROPERTY_FindProp( HWND hwnd, ATOM Atom );

void PROPERTY_RemoveWindowProps( WND *pWnd );

#endif