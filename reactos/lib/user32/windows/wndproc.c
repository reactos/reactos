#include <windows.h>
#include <user32/winproc.h>


/**********************************************************************
 *	     CallWindowProc    
 *
 * The CallWindowProc() function invokes the windows procedure _func_,
 * with _hwnd_ as the target window, the message specified by _msg_, and
 * the message parameters _wParam_ and _lParam_.
 *
 * Some kinds of argument conversion may be done, I'm not sure what.
 *
 * CallWindowProc() may be used for windows subclassing. Use
 * SetWindowLong() to set a new windows procedure for windows of the
 * subclass, and handle subclassed messages in the new windows
 * procedure. The new windows procedure may then use CallWindowProc()
 * with _func_ set to the parent class's windows procedure to dispatch
 * the message to the superclass.
 *
 * RETURNS
 *
 *    The return value is message dependent.
 *
 * CONFORMANCE
 *
 *   ECMA-234, Win32 
 */
LRESULT WINAPI CallWindowProcA( 
    WNDPROC func, 
    HWND hwnd,
    UINT msg, 
    WPARAM wParam, 
    LPARAM lParam    
) 
{
}

LRESULT WINAPI CallWindowProcW( 
    WNDPROC func, 
    HWND hwnd,
    UINT msg, 
    WPARAM wParam, 
    LPARAM lParam    
) 
{
}

/***********************************************************************
 * DefWindowProc  Calls default window message handler
 * 
 * Calls default window procedure for messages not processed 
 *  by application.
 *
 *  RETURNS
 *     Return value is dependent upon the message.
*/


//FIXME DefWindowProcW should be fundamental

LRESULT WINAPI DefWindowProcA(
	 HWND hwnd,
	 UINT msg,
	 WPARAM wParam,
         LPARAM lParam )
{

}


LRESULT WINAPI DefWindowProcW( 
    HWND hwnd,      
    UINT msg,       
    WPARAM wParam,  
    LPARAM lParam ) 
{
 
}



