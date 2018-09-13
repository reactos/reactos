///////////////////////////////////////////////////////////////////////////////
/*  File: undo.cpp

    Description: Definitions for classes associated with the "undo" feature.
        A client first creates an UndoList object.  Whenever an "undoable" 
        action is performed in the quota UI (modification/deletion), an
        undo action object is created and added to the UndoList object.  
        Each type of undo action object knows what is has to do to reverse
        the effects of the original operation.  When the client wants to 
        reverse the effects of all operations on the undo list, it merely
        commands the UndoList object to "Undo".  To clear the undo list,
        a client calls UndoList::Clear().


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/30/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h" // PCH
#pragma hdrstop

#include "undo.h"


///////////////////////////////////////////////////////////////////////////////
/*  Function: UndoAction::UndoAction
    Function: UndoAction::~UndoAction

    Description: Constructor and Destructor

    Arguments: 
        pUser - Address of IDiskQuotaUser interface for user associated
            with this undo action.

        llThreshold - Quota threshold value to be restored if action is 
            undone.

        llLimit - Quota limit value to be restored if action is undone.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/30/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
UndoAction::UndoAction(
    PDISKQUOTA_USER pUser,
    LONGLONG llThreshold,
    LONGLONG llLimit,
    PDISKQUOTA_CONTROL pQuotaControl
    ) : m_pUser(pUser),
        m_pUndoList(NULL),
        m_pQuotaControl(pQuotaControl)
{
    DBGTRACE((DM_UNDO, DL_HIGH, TEXT("UndoAction::UndoAction")));
    DBGPRINT((DM_UNDO, DL_HIGH, TEXT("\tthis = 0x%08X"), this));

    DBGASSERT((NULL != m_pUser));
    m_llThreshold = llThreshold;
    m_llLimit     = llLimit;
}


UndoAction::~UndoAction(
    VOID
    )
{
    DBGTRACE((DM_UNDO, DL_HIGH, TEXT("UndoAction::~UndoAction")));
    DBGPRINT((DM_UNDO, DL_HIGH, TEXT("\tthis = 0x%08X"), this));

    if (NULL != m_pUser)
        m_pUser->Release();  // Release from Undo list.
    if (NULL != m_pQuotaControl)
        m_pQuotaControl->Release();
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: UndoList::~UndoList

    Description: Destructor.  Destroys all undo action objects in the 
        undo list object.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/30/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
UndoList::~UndoList(
    VOID
    )
{
    Clear();
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: UndoList::Clears

    Description: Destroys all undo action objects in the 
        undo list object.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/30/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID UndoList::Clear(
    VOID
    )
{
    UndoAction *pAction = NULL;

    DBGPRINT((DM_UNDO, DL_MID, TEXT("UNDO - Cleared undo list")));
    m_hList.Lock();
    while(m_hList.RemoveFirst((LPVOID *)&pAction))
    {
        DBGASSERT((NULL != pAction));
        delete pAction;
    }
    m_hList.ReleaseLock();
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: UndoList::Undo

    Description: Iterates through all undo action objects and commands each
        to perform it's undo action.  Once the action is performed, the
        undo action object is destroyed.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/30/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID UndoList::Undo(
    VOID
    )
{
    UndoAction *pAction = NULL;

    DBGPRINT((DM_UNDO, DL_MID, TEXT("UNDO - Undoing undo list")));

    //
    // Disable redraw on the listview so that we only update once.
    //
    CAutoSetRedraw autoredraw(m_hwndListView, false);
    m_hList.Lock();
    while(m_hList.RemoveFirst((LPVOID *)&pAction))
    {
        DBGASSERT((NULL != pAction));
        pAction->Undo();
        delete pAction;
    }
    m_hList.ReleaseLock();
    InvalidateRect(m_hwndListView, NULL, FALSE);
}




///////////////////////////////////////////////////////////////////////////////
/*  Function: UndoDelete::Undo

    Description: Reverses the deletion of a user quota record.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/30/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
UndoDelete::Undo(
    VOID
    )
{
    DBGPRINT((DM_UNDO, DL_MID, TEXT("UNDO - Undoing deletion")));

    HRESULT hResult = NO_ERROR;

    //
    // Just restore the quota settings.
    //
    hResult = m_pUser->SetQuotaLimit(m_llLimit, TRUE);
    hResult = m_pUser->SetQuotaThreshold(m_llThreshold, TRUE);

    if (SUCCEEDED(hResult))
    {
        //
        // Add the entry back into the listview.
        //
        HWND hwndListView      = m_pUndoList->GetListViewHwnd();
        PointerList *pUserList = m_pUndoList->GetUserList();

        LV_ITEM item;

        item.mask       = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE;
        item.state      = 0;
        item.stateMask  = 0;
        item.iSubItem   = 0;
        item.pszText    = LPSTR_TEXTCALLBACK;
        item.iImage     = I_IMAGECALLBACK;
        item.iItem      = 0;

        pUserList->Insert((LPVOID)m_pUser);
        if (-1 != ListView_InsertItem(hwndListView, &item))
        {
            m_pUser->AddRef();
        }
        else
            hResult = E_FAIL;
    }

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: UndoAdd::Undo

    Description: Reverses the addition of a user quota record by marking it
        for deletion.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/27/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
UndoAdd::Undo(
    VOID
    )
{
    DBGPRINT((DM_UNDO, DL_MID, TEXT("UNDO - Undoing addition")));

    DBGASSERT((NULL != m_pQuotaControl));
    DBGASSERT((NULL != m_pUser));

    HRESULT hResult = m_pQuotaControl->DeleteUser(m_pUser);

    if (SUCCEEDED(hResult))
    {
        INT iItem;
        LV_FINDINFO fi;
        HWND hwndListView = m_pUndoList->GetListViewHwnd();
        PointerList *pUserList = m_pUndoList->GetUserList();

        fi.flags  = LVFI_PARAM;
        fi.lParam = (LPARAM)m_pUser;

        iItem = ListView_FindItem(hwndListView, -1, &fi);
        if (-1 != iItem)
        {
            PDISKQUOTA_USER pIUserToDelete = NULL;

            //
            // Delete the entry from the list view.
            //
            ListView_DeleteItem(hwndListView, iItem);

            //
            // Delete the entry from the user list.
            //
            pUserList->Remove((LPVOID *)&pIUserToDelete, iItem);


            pIUserToDelete->Release();  // Release from listview.
        }
        else
        {
            DBGERROR((TEXT("UndoAdd::Undo - Didn't find user in listview.")));
        }
    }
    else
    {
        DBGERROR((TEXT("UndoAdd::Undo - Error 0x%08X deleting user 0x%08X"),
                 hResult, m_pUser));
    }
    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: UndoModify::Undo

    Description: Reverses the modification of a user quota record.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/30/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
UndoModify::Undo(
    VOID
    )
{
    DBGPRINT((DM_UNDO, DL_MID, TEXT("UNDO - Undoing modification")));

    HRESULT hResult = NO_ERROR;
    //
    // Restore the user's quota settings.
    //
    hResult = m_pUser->SetQuotaLimit(m_llLimit, TRUE);
    hResult = m_pUser->SetQuotaThreshold(m_llThreshold, TRUE);

    if (SUCCEEDED(hResult))
    {
        //
        // Locate the corresponding listview item and update it.
        //
        HWND hwndListView      = m_pUndoList->GetListViewHwnd();
        PointerList *pUserList = m_pUndoList->GetUserList();

        DBGASSERT((NULL != hwndListView));
        INT iItem = -1;

        if (pUserList->FindIndex((LPVOID)m_pUser, &iItem))
            ListView_Update(hwndListView, iItem);
        else
            hResult = E_FAIL;
    }
    return hResult;
}
        

