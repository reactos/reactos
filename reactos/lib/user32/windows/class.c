/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/user32/windows/class.c
 * PURPOSE:     Registers a window class
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/05/99: Created
 */
#include <windows.h>
#include <user32/class.h>
#include <user32/win.h>
#include <user32/dce.h>
#include <user32/heapdup.h>

CLASS *rootClass;

ATOM STDCALL 
RegisterClassA(const WNDCLASS* wc) 
{
  WNDCLASSEX wcex;

  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = wc->style; 
  wcex.lpfnWndProc = wc->lpfnWndProc; 
  wcex.cbClsExtra = wc->cbClsExtra; 
  wcex.cbWndExtra = wc->cbWndExtra;
  wcex.hInstance = wc->hInstance;
  wcex.hIcon = wc->hIcon;
  wcex.hCursor = wc->hCursor;
  wcex.hbrBackground = wc->hbrBackground;
  wcex.lpszMenuName = wc->lpszMenuName; 
  wcex.lpszClassName = 	wc->lpszClassName;
  wcex.hIconSm = NULL;
  return RegisterClassExA(&wcex);
}

ATOM STDCALL 
RegisterClassW(const WNDCLASS* wc) 
{
  WNDCLASSEX wcex;

  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = wc->style; 
  wcex.lpfnWndProc = wc->lpfnWndProc; 
  wcex.cbClsExtra = wc->cbClsExtra; 
  wcex.cbWndExtra = wc->cbWndExtra;
  wcex.hInstance = wc->hInstance;
  wcex.hIcon = wc->hIcon;
  wcex.hCursor = wc->hCursor;
  wcex.hbrBackground = wc->hbrBackground;
  wcex.lpszMenuName = wc->lpszMenuName; 
  wcex.lpszClassName = 	wc->lpszClassName;
  wcex.hIconSm = NULL;
  return RegisterClassExW(&wcex);
}

ATOM STDCALL 
RegisterClassExA(const WNDCLASSEX* wc) 
{
    ATOM atom;
    CLASS *classPtr;
    INT classExtra, winExtra;
    int len;
    
    if (wc == NULL || wc->cbSize != sizeof(WNDCLASSEX)) 
      {
	SetLastError(ERROR_INVALID_DATA);
	return FALSE;
      }
    
    atom = GlobalAddAtomA(wc->lpszClassName);
    if (!atom)
      {
        SetLastError(ERROR_CLASS_ALREADY_EXISTS);
        return FALSE;
      }
    
    classExtra = wc->cbClsExtra;
    if (classExtra < 0) 
      classExtra = 0;
    else if (classExtra > 40)  
      classExtra = 40;
    
    winExtra = wc->cbClsExtra;
    if (winExtra < 0) 
      winExtra = 0;
    else if (winExtra > 40)    
      winExtra = 40;
    
    classPtr = (CLASS *)HeapAlloc( GetProcessHeap(), 0, sizeof(CLASS) +
				   classExtra - sizeof(classPtr->wExtra) );
    
    
    if (classExtra) 
      HEAP_memset( classPtr->wExtra, 0, classExtra );
    
    if (!classPtr) {
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      GlobalDeleteAtom( atom );
      return FALSE;
    }
    classPtr->magic       = CLASS_MAGIC;
    classPtr->cWindows    = 0;  
    classPtr->style       = wc->style;
    classPtr->winproc     = wc->lpfnWndProc; 
    classPtr->cbWndExtra  = winExtra;
    classPtr->cbClsExtra  = classExtra;
    classPtr->hInstance   = wc->hInstance;
    classPtr->atomName	  = atom;
    classPtr->hIcon         = (HICON)wc->hIcon;
    classPtr->hIconSm       = (HICON)wc->hIconSm;
    classPtr->hCursor       = (HCURSOR)wc->hCursor;
    classPtr->hbrBackground = (HBRUSH)wc->hbrBackground;
    classPtr->bUnicode = FALSE;
    
    if (wc->style & CS_CLASSDC)
      classPtr->dce         = DCE_AllocDCE( 0, DCE_CLASS_DC ) ;
    else 
      classPtr->dce = NULL;
    
    
    if ( wc->lpszMenuName != NULL ) {
      len = lstrlenA(wc->lpszMenuName);
      classPtr->menuName = HeapAlloc(GetProcessHeap(),0,len+1);
      lstrcpyA(classPtr->menuName,wc->lpszMenuName);
    }
    else
      classPtr->menuName = NULL;
    
    
    
    len = lstrlenA(wc->lpszClassName);
    classPtr->className = HeapAlloc(GetProcessHeap(),0,len+1);
    lstrcpyA(classPtr->className,wc->lpszClassName);
    
    

    classPtr->next        = rootClass;
    rootClass = classPtr;

    return atom;
}


ATOM STDCALL RegisterClassExW( const WNDCLASSEX* wc )
{
    ATOM atom;
    CLASS *classPtr;
    INT classExtra, winExtra;

    int len;
    if ( wc == NULL || wc->cbSize != sizeof(WNDCLASSEX)) {
	SetLastError(ERROR_INVALID_DATA);
	return FALSE;
    }

    if (!(atom = GlobalAddAtomW( (LPWSTR)wc->lpszClassName )))
    {
        SetLastError(ERROR_CLASS_ALREADY_EXISTS);
        return FALSE;
    }

    classExtra = wc->cbClsExtra;
    if (classExtra < 0) 
	classExtra = 0;
    else if (classExtra > 40)  
	classExtra = 40;

    winExtra = wc->cbClsExtra;
    if (winExtra < 0) 
	winExtra = 0;
    else if (winExtra > 40)    
	winExtra = 40;

    classPtr = (CLASS *)HeapAlloc( GetProcessHeap(), 0, sizeof(CLASS) +
                                       classExtra - sizeof(classPtr->wExtra) );


    if (classExtra) 
	HEAP_memset( classPtr->wExtra, 0, classExtra );

    if (!classPtr) {
	SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	GlobalDeleteAtom( atom );
	return FALSE;
    }
    classPtr->magic       = CLASS_MAGIC;
    classPtr->cWindows    = 0;  
    classPtr->style       = wc->style;
    classPtr->winproc     = wc->lpfnWndProc; 
    classPtr->cbWndExtra  = winExtra;
    classPtr->cbClsExtra  = classExtra;
    classPtr->hInstance   = wc->hInstance;
    classPtr->atomName	  = atom;
    classPtr->hIcon         = (HICON)wc->hIcon;
    classPtr->hIconSm       = (HICON)wc->hIconSm;
    classPtr->hCursor       = (HCURSOR)wc->hCursor;
    classPtr->hbrBackground = (HBRUSH)wc->hbrBackground;
    classPtr->bUnicode = FALSE;

    classPtr->dce         = (wc->style & CS_CLASSDC) ?
                                 CreateDC( "DISPLAY", NULL,NULL,NULL ) : NULL;

   if ( wc->lpszMenuName != NULL ) {
	len = lstrlenW(wc->lpszMenuName);
    	classPtr->menuName = HeapAlloc(GetProcessHeap(),0,sizeof(WCHAR)*(len+1));
    	lstrcpyW(classPtr->menuName,wc->lpszMenuName);
    }
    else
	classPtr->menuName = NULL;


    len = lstrlenW((LPWSTR)wc->lpszClassName);
    classPtr->className = HeapAlloc(GetProcessHeap(),0,(len+1)*sizeof(WCHAR));
    lstrcpyW((LPWSTR)classPtr->className,(LPWSTR) wc->lpszClassName );

    classPtr->next        = rootClass;
    rootClass = classPtr;

    return atom;
}

WINBOOL STDCALL UnregisterClassA(LPCSTR  lpClassName,	 HINSTANCE  hInstance )
{
    CLASS *classPtr;
    classPtr = CLASS_FindClassByAtom( STRING2ATOMA(lpClassName), hInstance );
    if ( classPtr == NULL )
		return FALSE;


    if ( CLASS_FreeClass(classPtr) == TRUE )
	    GlobalDeleteAtom( classPtr->atomName );	
    return TRUE;
}




WINBOOL STDCALL UnregisterClassW(LPCWSTR  lpClassName,	 HINSTANCE  hInstance )
{
    CLASS *classPtr;
    classPtr = CLASS_FindClassByAtom( STRING2ATOMW(lpClassName), hInstance );
    if ( classPtr == NULL )
		return FALSE;


    if ( CLASS_FreeClass(classPtr) == TRUE )
	    GlobalDeleteAtom( classPtr->atomName );	
    return TRUE;
}


WINBOOL STDCALL GetClassInfoA( HINSTANCE  hInstance, LPCSTR  lpClassName, LPWNDCLASS  lpWndClass )
{

	CLASS *classPtr;
	ATOM a;

	if ( HIWORD(lpClassName) != 0 )
		a = FindAtomA(lpClassName);
	else
		a = lpClassName;

	classPtr = CLASS_FindClassByAtom(  a,  hInstance );
	if ( classPtr == NULL )
		return FALSE;

		
	lpWndClass->style = classPtr->style;
    	lpWndClass->lpfnWndProc = classPtr->winproc;
	lpWndClass->cbClsExtra =   classPtr->cbWndExtra;
	lpWndClass->cbClsExtra = classPtr->cbClsExtra;
    	lpWndClass->hInstance = classPtr->hInstance;
    	lpWndClass->hIcon = classPtr->hIcon;
    	lpWndClass->hCursor = classPtr->hCursor;
	lpWndClass->hbrBackground = classPtr->hbrBackground;
	return TRUE;
}


WINBOOL STDCALL GetClassInfoW( HINSTANCE  hInstance, LPCWSTR  lpClassName, LPWNDCLASS  lpWndClass )
{
	CLASS *classPtr;
	ATOM a;

	if ( HIWORD(lpClassName) != 0 )
		a = FindAtomW(lpClassName);
	else
		a = lpClassName;

	classPtr = CLASS_FindClassByAtom(  a,  hInstance );
	if ( classPtr == NULL )
		return FALSE;

		
	lpWndClass->style = classPtr->style;
    	lpWndClass->lpfnWndProc = classPtr->winproc;
	lpWndClass->cbClsExtra =   classPtr->cbWndExtra;
	lpWndClass->cbClsExtra = classPtr->cbClsExtra;
    	lpWndClass->hInstance = classPtr->hInstance;
    	lpWndClass->hIcon = classPtr->hIcon;
    	lpWndClass->hCursor = classPtr->hCursor;
	lpWndClass->hbrBackground = classPtr->hbrBackground;
	return TRUE;
	
}

WINBOOL STDCALL GetClassInfoExA( HINSTANCE  hInstance, LPCSTR  lpClassName, LPWNDCLASSEX  lpWndClassEx )
{

	CLASS *classPtr;
	ATOM a;

	if ( HIWORD(lpClassName) != 0 )
		a = FindAtomA(lpClassName);
	else
		a = (ATOM)lpClassName;

	classPtr = CLASS_FindClassByAtom(  a, hInstance );
	if ( classPtr == NULL )
		return FALSE;

	
	if ( lpWndClassEx ->cbSize != sizeof(WNDCLASSEX) ) 
		return FALSE;
		
	
	lpWndClassEx->style = classPtr->style;
    	lpWndClassEx->lpfnWndProc = classPtr->winproc;
	lpWndClassEx->cbClsExtra =   classPtr->cbWndExtra;
	lpWndClassEx->cbClsExtra = classPtr->cbClsExtra;
    	lpWndClassEx->hInstance = classPtr->hInstance;
    	lpWndClassEx->hIcon = classPtr->hIcon;
    	lpWndClassEx->hCursor = classPtr->hCursor;
	lpWndClassEx->hbrBackground = classPtr->hbrBackground;
		
	
	return TRUE;
}

WINBOOL STDCALL GetClassInfoExW( HINSTANCE  hInstance, LPCWSTR  lpClassName, LPWNDCLASSEX  lpWndClassEx )
{

	CLASS *classPtr;
	ATOM a;

	if ( HIWORD(lpClassName) != 0 )
		a = FindAtomW(lpClassName);
	else
		a = (ATOM)lpClassName;

	classPtr = CLASS_FindClassByAtom(  a, hInstance );
	if ( classPtr == NULL )
		return FALSE;

	
	if ( lpWndClassEx ->cbSize != sizeof(WNDCLASSEX) ) 
		return FALSE;
		
	
	lpWndClassEx->style = classPtr->style;
    	lpWndClassEx->lpfnWndProc = classPtr->winproc;
	lpWndClassEx->cbClsExtra =   classPtr->cbWndExtra;
	lpWndClassEx->cbClsExtra = classPtr->cbClsExtra;
    	lpWndClassEx->hInstance = classPtr->hInstance;
    	lpWndClassEx->hIcon = classPtr->hIcon;
    	lpWndClassEx->hCursor = classPtr->hCursor;
	lpWndClassEx->hbrBackground = classPtr->hbrBackground;
		
	
	return TRUE;
}

int STDCALL  GetClassNameA(HWND  hWnd, LPSTR  lpClassName, int  nMaxCount )
{
	WND *wndPtr = WIN_FindWndPtr(hWnd);

	if ( wndPtr == NULL )
		return 0;

	if ( wndPtr->class->bUnicode == TRUE )
		return 0;

	return lpstrncpyA(lpClassName,wndPtr->class->className, nMaxCount);
	
}

int STDCALL GetClassNameW(HWND  hWnd, LPWSTR  lpClassName, int  nMaxCount )
{
	WND *wndPtr = WIN_FindWndPtr(hWnd);

	if ( wndPtr == NULL )
		return 0;

	if ( wndPtr->class->bUnicode == FALSE )
		return 0;

	return lpstrncpyW(lpClassName,wndPtr->class->className, nMaxCount);


}

DWORD STDCALL GetClassLongA(HWND  hWnd,	int  nIndex )
{
    WND * wndPtr;
    
    if (!(wndPtr = WIN_FindWndPtr( hWnd ))) return 0;
    if (nIndex >= 0)
    {
        if (nIndex <= wndPtr->class->cbClsExtra - sizeof(LONG))
            return (DWORD)((char *)wndPtr->class->wExtra) + nIndex;
    }
    switch(nIndex)
    {
        case GCL_STYLE:      return (LONG)wndPtr->class->style;
        case GCL_CBWNDEXTRA: return (LONG)wndPtr->class->cbWndExtra;
        case GCL_CBCLSEXTRA: return (LONG)wndPtr->class->cbClsExtra;
        case GCL_HMODULE:    return (LONG)wndPtr->class->hInstance;
        case GCL_WNDPROC:    return (LONG)wndPtr->class->winproc;
        case GCL_MENUNAME:   return (LONG)wndPtr->class->menuName;
        case GCW_ATOM:
        case GCL_HBRBACKGROUND:
        case GCL_HCURSOR:
        case GCL_HICON:
        case GCL_HICONSM:
            return GetClassWord( hWnd, nIndex );
	default:
	    return -1;
    }
    
    return 0;
}

DWORD STDCALL GetClassLongW(HWND  hWnd,	int  nIndex )
{
    WND * wndPtr;
    
    if (!(wndPtr = WIN_FindWndPtr( hWnd ))) return 0;
    if (nIndex >= 0)
    {
        if (nIndex <= wndPtr->class->cbClsExtra - sizeof(LONG))
            return (DWORD)((char *)wndPtr->class->wExtra) + nIndex;
    }
    switch(nIndex)
    {
        case GCL_STYLE:      return (LONG)wndPtr->class->style;
        case GCL_CBWNDEXTRA: return (LONG)wndPtr->class->cbWndExtra;
        case GCL_CBCLSEXTRA: return (LONG)wndPtr->class->cbClsExtra;
        case GCL_HMODULE:    return (LONG)wndPtr->class->hInstance;
        case GCL_WNDPROC:    return (LONG)wndPtr->class->winproc;
        case GCL_MENUNAME:   return (LONG)wndPtr->class->menuName;
        case GCW_ATOM:
        case GCL_HBRBACKGROUND:
        case GCL_HCURSOR:
        case GCL_HICON:
        case GCL_HICONSM:
            return GetClassWord( hWnd, nIndex );
	default:
	    return -1;
    }
    
    return 0;
}



WORD STDCALL GetClassWord( HWND hWnd, INT nIndex )
{
    WND * wndPtr;
    
    if (!(wndPtr = WIN_FindWndPtr( hWnd ))) return 0;
    if (nIndex >= 0)
    {
        if (nIndex <= wndPtr->class->cbClsExtra - sizeof(WORD))
            return (WORD)(wndPtr->class->wExtra + nIndex);
    }
    else switch(nIndex)
    {
        //case GCW_HBRBACKGROUND: return wndPtr->class->hbrBackground;
        //case GCW_HCURSOR:       return wndPtr->class->hCursor;
        //case GCW_HICON:         return wndPtr->class->hIcon;
    //    case GCW_HICONSM:       return wndPtr->class->hIconSm;
        case GCW_ATOM:          return wndPtr->class->atomName;
        //case GCW_STYLE:
        //case GCW_CBWNDEXTRA:
        //case GCW_CBCLSEXTRA:
        //case GCW_HMODULE:
            return (WORD)GetClassLongA( hWnd, nIndex );
	default:
	    return -1;
    }

    
    return 0;
}

WORD
STDCALL
SetClassWord(
	     HWND hWnd,
	     int nIndex,
	     WORD wNewWord)
{
	return 0;
}

DWORD
STDCALL
SetClassLongA(
    HWND hWnd,
    int nIndex,
    LONG dwNewLong)
{
	return 0;
}

DWORD
STDCALL
SetClassLongW(
    HWND hWnd,
    int nIndex,
    LONG dwNewLong)
{
	return 0;
}

CLASS *CLASS_FindClassByAtom( ATOM classAtom, HINSTANCE hInstance )
{
	CLASS *p = rootClass;
	while(p != NULL ) {
		if ( p->atomName == classAtom )
			return p;
		p = p->next;
	}
	return NULL;
}


WINBOOL CLASS_FreeClass(CLASS *classPtr)
{
    CLASS *p = rootClass;
    if( classPtr->cWindows > 0 )
	return FALSE;

    if (classPtr->dce) 
	DeleteDC( classPtr->dce );
    if (classPtr->hbrBackground) 
	DeleteObject( classPtr->hbrBackground );
    

    classPtr->atomName = 0;
    HeapFree(GetProcessHeap(),0,classPtr->className);
    	
    while(p != NULL && p->next != classPtr ) 
	p = p->next;
 
    if ( p != NULL )
	p->next = classPtr->next;

    HeapFree(GetProcessHeap(),0,classPtr);
    return TRUE;
}
		
