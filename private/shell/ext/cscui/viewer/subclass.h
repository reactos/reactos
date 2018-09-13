#ifndef _INC_CSCVIEW_SUBCLASS_H
#define _INC_CSCVIEW_SUBCLASS_H
//////////////////////////////////////////////////////////////////////////////
/*  File: subclass.h

    Description: Helps with subclassing a window.  All a client needs to do
        is create a class derived from WindowSubclass, providing an
        implementation of HandleMessages().  To activate the subclassing,
        the client calls Initialize, passing in the handle of the window to 
        be subclassed.  The HandleMessages() function will receive all 
        messages bound for the subclassed window.  Messages are automatically
        forwarded to the subclassed window after HandleMessages has returned.
        The subclassing is terminated by either calling the Cancel()
        function or by destroying the subclass object.  This implementation
        uses the window property "WSCPROP_THISPTR" to store the "this"
        pointer for the subclass object.   This allows you to subclass
        system windows (i.e. common controls) without concern for disturbing
        existing window instance data.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    12/16/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef _WINDOWS_
#include <windows.h>
#endif


class WindowSubclass
{
    public:
        WindowSubclass(void);
        ~WindowSubclass(void);

        bool Initialize(HWND hwnd);
        bool Resume(void);
        void Cancel(void);

    protected:
        virtual LRESULT HandleMessages(HWND, UINT, WPARAM, LPARAM) = 0;
        HWND GetWindow(void) const
            { return m_hwnd; }

    private:
        HWND         m_hwnd;         // Handle to subclassed window.
        WNDPROC      m_lpfnWndProc;  // Subclassed window's wnd proc.
        static const TCHAR m_szPropThis[]; // "WSCPROP_THIS"

        static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
};



#endif // _INC_CSCVIEW_SUBCLASS_H

