#include <windows.h>
#include <user32/property.h>
#include <user32/win.h>

/***********************************************************************
 *           GetPropA   (USER.281)
 */
HANDLE STDCALL GetPropA( HWND hWnd, LPCSTR str )
{
    PROPERTY *prop;
    prop = PROPERTY_FindProp(hWnd,STRING2ATOMA(str));
    return prop ? prop->handle : NULL;
  
}


/***********************************************************************
 *           GetPropW   (USER.282)
 */
HANDLE STDCALL GetPropW( HWND hWnd, LPCWSTR str)
{
    PROPERTY *prop;
    prop = PROPERTY_FindProp(hWnd,STRING2ATOMW(str));
    return prop ? prop->handle : NULL;
}





/***********************************************************************
 *           SetPropA   (USER.497)
 */
WINBOOL STDCALL SetPropA( HWND hwnd, LPCSTR str, HANDLE hdata )
{

   return PROPERTY_SetProp( hwnd,STRING2ATOMA(str),hdata);
 
}


/***********************************************************************
 *           SetPropW   (USER.498)
 */
WINBOOL STDCALL SetPropW( HWND hwnd, LPCWSTR str, HANDLE hdata )
{
    return PROPERTY_SetProp(hwnd,STRING2ATOMW(str),hdata);
}




/***********************************************************************
 *           RemovePropA   (USER.442)
 */
HANDLE STDCALL RemovePropA( HWND hwnd, LPCSTR str )
{
    return PROPERTY_RemoveProp(hwnd, STRING2ATOMA(str));
}


/***********************************************************************
 *           RemovePropW   (USER.443)
 */
HANDLE STDCALL RemovePropW( HWND hwnd, LPCWSTR str )
{
   return PROPERTY_RemoveProp(hwnd, STRING2ATOMW(str));
}





#if 0

/***********************************************************************
 *           EnumPropsA   (USER.186)
 */
INT STDCALL EnumPropsA( HWND hwnd, PROPENUMPROCA func )
{
     int size;
    int i;
    INT ret = -1;
    LPWSTR string[MAX_PATH];
    PROPVALUE pv;

    pv = malloc(100*sizeof(PROPVALUE));

    PROPERTY_EnumPropEx(hwnd, pv ,100, &size );
    for (i=0;i<size;i++)
    {
        /* Already get the next in case the callback */
        /* function removes the current property.    */
        next = prop->next;

    	GlobalFindAtomNameA(pv[i]->atom,string,MAX_PATH-1);
        ret = func( hwnd, string, pv[i]->handle);
        if (!ret) 
		break;
    }

    free(pv);
    return ret;
}


/***********************************************************************
 *           EnumPropsW   (USER.189)
 */
INT STDCALL EnumPropsW( HWND hwnd, PROPENUMPROCW func )
{
    int size;
    int i;
    INT ret = -1;
    LPWSTR string[MAX_PATH];
    PROPVALUE *pv;

    pv = malloc(100*sizeof(PROPVALUE));

    PROPERTY_EnumPropEx(hwnd, pv ,100, &size );
    for (i=0;i<size;i++)
    {
        /* Already get the next in case the callback */
        /* function removes the current property.    */
        next = prop->next;

    	GlobalFindAtomNameW(pv[i]->atom,string,MAX_PATH-1);
        ret = func( hwnd, string, pv[i]->handle);
        if (!ret) 
		break;
    }

    free(pv);
    return ret;
}


/***********************************************************************
 *           EnumPropsExA   (USER.187)
 */
INT STDCALL EnumPropsExA(HWND hwnd, PROPENUMPROCEXA func, LPARAM lParam)
{
    int size;
    int i;
    INT ret = -1;
    LPWSTR string[MAX_PATH];
    PROPVALUE *pv;

    pv = malloc(100*sizeof(PROPVALUE));

    PROPERTY_EnumPropEx(hwnd, pv ,100, &size );
    for (i=0;i<size;i++)
    {
        /* Already get the next in case the callback */
        /* function removes the current property.    */
        next = prop->next;

    	GlobalFindAtomNameW(pv[i]->atom,string,MAX_PATH-1);
        ret = func( hwnd, string, pv[i]->handle, lParam);
        if (!ret) 
		break;
    }

    free(pv);
    return ret;
}


/***********************************************************************
 *           EnumPropsExW   (USER.188)
 */
INT STDCALL EnumPropsExW(HWND hwnd, PROPENUMPROCEXW func, LPARAM lParam)
{
    int size;
    int i;
    INT ret = -1;
    LPWSTR string[MAX_PATH];
    PROPVALUE *pv;

    pv = malloc(100*sizeof(PROPVALUE));

    PROPERTY_EnumPropEx(hwnd, pv ,100, &size );
    for (i=0;i<size;i++)
    {
        /* Already get the next in case the callback */
        /* function removes the current property.    */
        next = prop->next;

    	GlobalFindAtomNameW(pv[i]->atom,string,MAX_PATH-1);
        ret = func( hwnd, string, pv[i]->handle, lParam);
        if (!ret) 
		break;
    }

    free(pv);
    return ret;
}


#endif