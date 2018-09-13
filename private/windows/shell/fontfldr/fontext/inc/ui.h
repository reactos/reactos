//------------------------------------------------------------------------
// UI.H
//
// Commonly used UI routines, classes, etc.
//------------------------------------------------------------------------

#if !defined(__UI_H__)
#define __UI_H__


#define vUIPStatusShow(a,b)

VOID FAR PASCAL vUIMsgInit( );
int FAR PASCAL iUIErrMemDlg( );

/*  The wait cursor is used to cause an hourglass for the duration of a
 *  routine.  The constructor sets the cursor to the hourglass, holding
 *  the previous. The destructor recovers the original cursor (or arrow if
 *  none).
 */

class WaitCursor
{
    public :
        WaitCursor () { m_Cursor = SetCursor(LoadCursor(NULL, IDC_WAIT));};
        ~WaitCursor () { if (m_Cursor != NULL)
                            SetCursor (m_Cursor);
                         else
                            SetCursor(LoadCursor(NULL, IDC_ARROW)); };
    private :
        HCURSOR    m_Cursor;
};

#endif // __UI_H__
