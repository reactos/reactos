#include <windows.h>
#include <user32/property.h>
#include <user32/win.h>
#include <user32/debug.h>

//TODO user Wide Character functions as basis 


/*
 * Window properties
 *
 * Copyright 1995, 1996 Alexandre Julliard
 */

/***********************************************************************
 *           PROP_FindProp
 */
static PROPERTY *PROP_FindProp( HWND hwnd, LPCSTR str )
{
    ATOM atom;
    PROPERTY *prop;
    WND *pWnd = WIN_FindWndPtr( hwnd );

    if (!pWnd) return NULL;
    if (HIWORD(str))
    {
        atom = GlobalFindAtomA( str );
        for (prop = pWnd->pProp; prop; prop = prop->next)
        {
            if (HIWORD(prop->string))
            {
                if (!lstrcmpiA( prop->string, str )) goto END;
            }
            else if (LOWORD(prop->string) == atom) goto END;
        }
    }
    else  /* atom */
    {
        atom = LOWORD(str);
        for (prop = pWnd->pProp; (prop); prop = prop->next)
        {
            if (HIWORD(prop->string))
            {
                if (GlobalFindAtomA( prop->string ) == atom) goto END;
            }
            else if (LOWORD(prop->string) == atom) goto END;
        }
    }
    prop = NULL;
END:
    WIN_ReleaseWndPtr(pWnd);
    return prop;
}


/***********************************************************************
 *           GetProp32A   (USER32.281)
 */
HANDLE WINAPI GetPropA( HWND hwnd, LPCSTR str )
{
    PROPERTY *prop = PROP_FindProp( hwnd, str );

    if (HIWORD(str))
        DPRINT("(%08x,'%s'): returning %08x\n",
                      hwnd, str, prop ? prop->handle : 0 );
    else
        DPRINT("(%08x,#%04x): returning %08x\n",
                      hwnd, LOWORD(str), prop ? prop->handle : 0 );

    return prop ? prop->handle : 0;
}


/***********************************************************************
 *           GetProp32W   (USER32.282)
 */
HANDLE WINAPI GetPropW( HWND hwnd, LPCWSTR str )
{
    LPSTR strA;
    HANDLE ret;

    if (!HIWORD(str)) return GetPropA( hwnd, (LPCSTR)(UINT)LOWORD(str) );
    strA = HEAP_strdupWtoA( GetProcessHeap(), 0, str );
    ret = GetPropA( hwnd, strA );
    HeapFree( GetProcessHeap(), 0, strA );
    return ret;
}



/***********************************************************************
 *           SetProp32A   (USER32.497)
 */
BOOL WINAPI SetPropA( HWND hwnd, LPCSTR str, HANDLE handle )
{
    PROPERTY *prop;

    if (HIWORD(str))
        DPRINT("%04x '%s' %08x\n", hwnd, str, handle );
    else
        DPRINT("%04x #%04x %08x\n",
                      hwnd, LOWORD(str), handle );

    if (!(prop = PROP_FindProp( hwnd, str )))
    {
        /* We need to create it */
        WND *pWnd = WIN_FindWndPtr( hwnd );
        if (!pWnd) return FALSE;
        if (!(prop = HeapAlloc( GetProcessHeap(), 0, sizeof(*prop) )))
        {
            WIN_ReleaseWndPtr(pWnd);
            return FALSE;
        }
        if (!(prop->string = HEAP_strdupA(GetProcessHeap(),0,str)))
        {
            HeapFree( GetProcessHeap(), 0, prop );
            WIN_ReleaseWndPtr(pWnd);
            return FALSE;

        }
        prop->next  = pWnd->pProp;
        pWnd->pProp = prop;
        WIN_ReleaseWndPtr(pWnd);
    }
    prop->handle = handle;
    return TRUE;
}


/***********************************************************************
 *           SetProp32W   (USER32.498)
 */
BOOL WINAPI SetPropW( HWND hwnd, LPCWSTR str, HANDLE handle )
{
    BOOL ret;
    LPSTR strA;

    if (!HIWORD(str))
        return SetPropA( hwnd, (LPCSTR)(UINT)LOWORD(str), handle );
    strA = HEAP_strdupWtoA( GetProcessHeap(), 0, str );
    ret = SetPropA( hwnd, strA, handle );
    HeapFree( GetProcessHeap(), 0, strA );
    return ret;
}




/***********************************************************************
 *           RemoveProp32A   (USER32.442)
 */
HANDLE WINAPI RemovePropA( HWND hwnd, LPCSTR str )
{
    ATOM atom;
    HANDLE handle;
    PROPERTY **pprop, *prop;
    WND *pWnd = WIN_FindWndPtr( hwnd );

    if (HIWORD(str))
      DPRINT("%04x '%s'\n", hwnd, str );
    else
      DPRINT("%04x #%04x\n", hwnd, LOWORD(str));


    if (!pWnd) return (HANDLE)0;
    if (HIWORD(str))
    {
        atom = GlobalFindAtomA( str );
        for (pprop=(PROPERTY**)&pWnd->pProp; (*pprop); pprop = &(*pprop)->next)
        {
            if (HIWORD((*pprop)->string))
            {
                if (!lstrcmpiA( (*pprop)->string, str )) break;
            }
            else if (LOWORD((*pprop)->string) == atom) break;
        }
    }
    else  /* atom */
    {
        atom = LOWORD(str);
        for (pprop=(PROPERTY**)&pWnd->pProp; (*pprop); pprop = &(*pprop)->next)
        {
            if (HIWORD((*pprop)->string))
            {
                if (GlobalFindAtomA( (*pprop)->string ) == atom) break;
            }
            else if (LOWORD((*pprop)->string) == atom) break;
        }
    }
    WIN_ReleaseWndPtr(pWnd);
    if (!*pprop) return 0;
    prop   = *pprop;
    handle = prop->handle;
    *pprop = prop->next;
    HeapFree( GetProcessHeap(), 0, prop->string);
    HeapFree( GetProcessHeap(), 0, prop );
    return handle;
}


/***********************************************************************
 *           RemoveProp32W   (USER32.443)
 */
HANDLE WINAPI RemovePropW( HWND hwnd, LPCWSTR str )
{
    LPSTR strA;
    HANDLE ret;

    if (!HIWORD(str))
        return RemovePropA( hwnd, (LPCSTR)(UINT)LOWORD(str) );
    strA = HEAP_strdupWtoA( GetProcessHeap(), 0, str );
    ret = RemovePropA( hwnd, strA );
    HeapFree( GetProcessHeap(), 0, strA );
    return ret;
}


/***********************************************************************
 *           PROPERTY_RemoveWindowProps
 *
 * Remove all properties of a window.
 */
void PROPERTY_RemoveWindowProps( WND *pWnd )
{
    PROPERTY *prop, *next;

    for (prop = pWnd->pProp; (prop); prop = next)
    {
        next = prop->next;
        HeapFree( GetProcessHeap(), 0, prop->string );
        HeapFree( GetProcessHeap(), 0, prop );
    }
    pWnd->pProp = NULL;
}




/***********************************************************************
 *           EnumProps32A   (USER32.186)
 */
INT WINAPI EnumPropsA( HWND hwnd, PROPENUMPROC func )
{
    return EnumPropsExA( hwnd, (PROPENUMPROCEX)func, 0 );
}


/***********************************************************************
 *           EnumProps32W   (USER32.189)
 */
INT WINAPI EnumPropsW( HWND hwnd, PROPENUMPROC func )
{
    return EnumPropsExW( hwnd, (PROPENUMPROCEX)func, 0 );
}


/***********************************************************************
 *           EnumPropsEx32A   (USER32.187)
 */
INT WINAPI EnumPropsExA(HWND hwnd, PROPENUMPROCEX func, LPARAM lParam)
{
    PROPERTY *prop, *next;
    WND *pWnd;
    INT ret = -1;

    DPRINT("%04x %08x %08lx\n",
                  hwnd, (UINT)func, lParam );
    if (!(pWnd = WIN_FindWndPtr( hwnd ))) return -1;
    for (prop = pWnd->pProp; (prop); prop = next)
    {
        /* Already get the next in case the callback */
        /* function removes the current property.    */
        next = prop->next;

        DPRINT("  Callback: handle=%08x str='%s'\n",
                      prop->handle, prop->string );
        ret = func( hwnd, prop->string, prop->handle, lParam );
        if (!ret) break;
    }
    WIN_ReleaseWndPtr(pWnd);
    return ret;
}


/***********************************************************************
 *           EnumPropsEx32W   (USER32.188)
 */
INT WINAPI EnumPropsExW(HWND hwnd, PROPENUMPROCEX func, LPARAM lParam)
{
    PROPERTY *prop, *next;
    WND *pWnd;
    INT ret = -1;

    DPRINT("%04x %08x %08lx\n",
                  hwnd, (UINT)func, lParam );
    if (!(pWnd = WIN_FindWndPtr( hwnd ))) return -1;
    for (prop = pWnd->pProp; (prop); prop = next)
    {
        /* Already get the next in case the callback */
        /* function removes the current property.    */
        next = prop->next;

        DPRINT("  Callback: handle=%08x str='%s'\n",
                      prop->handle, prop->string );
        if (HIWORD(prop->string))
        {
            LPWSTR str = HEAP_strdupAtoW( GetProcessHeap(), 0, prop->string );
            ret = func( hwnd, str, prop->handle, lParam );
            HeapFree( GetProcessHeap(), 0, str );
        }
        else
            ret = func( hwnd, (LPCWSTR)(UINT)LOWORD( prop->string ),
                        prop->handle, lParam );
        if (!ret) break;
    }
    WIN_ReleaseWndPtr(pWnd);
    return ret;
}
