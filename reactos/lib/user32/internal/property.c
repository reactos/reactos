       /*
 * Window properties
 *
 * Copyright 1995, 1996 Alexandre Julliard
 */

#include <windows.h>
#include <user32/win.h>
#include <user32/property.h>


/***********************************************************************
 *           PROP_FindProp
 */
HANDLE PROPERTY_FindProp( HWND hwnd, ATOM atom )
{

    PROPERTY *prop;
    WND *pWnd = WIN_FindWndPtr( hwnd );

    if (!pWnd) 
	return NULL;
    
    
    for (prop = pWnd->pProp; (prop); prop = prop->next)
    {
          
                if ( prop->atom  == atom) 
			return prop->handle;
    }
    return NULL;
}

WINBOOL PROPERTY_SetProp(HANDLE hwnd,ATOM atom,HANDLE hData)
{
    PROPERTY *prop;
    if (!(prop = PROPERTY_FindProp( hwnd, atom )))
    {
        /* We need to create it */
        WND *pWnd = WIN_FindWndPtr( hwnd );
        if (!pWnd) 
		return FALSE;
        if (!(prop = HeapAlloc( GetProcessHeap(), 0, sizeof(PROPERTY) ))) 
		return FALSE;
   
        prop->next  = pWnd->pProp;
        pWnd->pProp = prop;
        prop->atom = atom;
        prop->handle = hData;
    }
    else {
        prop->handle = hData;
    }
    return TRUE;

}



HANDLE PROPERTY_RemoveProp( HANDLE hwnd , ATOM atom )
{
    HANDLE handle = NULL;
    PROPERTY **pprop, *prop;
    WND *pWnd = WIN_FindWndPtr( hwnd );


    if (!pWnd) 
	return (HANDLE)0;
    
    for (pprop=(PROPERTY**)&pWnd->pProp; (*pprop); pprop = &(*pprop)->next)
       {
        
               if ( (*pprop)->atom  == atom ) { 
			prop   = *pprop;
			handle = prop->handle;
    			*pprop = prop->next;
    			HeapFree( GetProcessHeap(), 0, prop );
			break;
			
		}
       }
    
  
    return handle;
}

/***********************************************************************
 *           PROPERTY_RemoveWindowProps
 *
 * Remove all properties of a window.
 */
void PROPERTY_RemoveWindowProps( HANDLE hwnd  )
{

    PROPERTY *prop, *next;
    WND *pWnd = WIN_FindWndPtr( hwnd );


    if (!pWnd) 
	return (HANDLE)0;
    


    for (prop = pWnd->pProp; (prop); prop = next)
    {
        next = prop->next;
        HeapFree(GetProcessHeap(),0, prop );
	
    }
    pWnd->pProp = NULL;
}


WINBOOL PROPERTY_EnumPropEx(HWND hwnd, PROPVALUE **pv , int maxsize, int *size )
{
    PROPERTY *prop, *next;
    WND *pWnd;
    INT ret = -1;
    int i;
 
    if (!(pWnd = WIN_FindWndPtr( hwnd )))
	return FALSE;
    for (prop = pWnd->pProp; (prop); prop = next)
    {
        /* Already get the next in case the callback */
        /* function removes the current property.    */
        next = prop->next;
        pv[i]->hwnd = hwnd;
	pv[i]->handle = prop->handle;
	pv[i]->atom = prop->atom;
	i++;
	if ( i > maxsize )
		return FALSE;
	
    }
    return TRUE;
}
 
