#ifndef _INC_DSKQUOTA_UNDO_H
#define _INC_DSKQUOTA_UNDO_H
///////////////////////////////////////////////////////////////////////////////
/*  File: undo.h

    Description: Declarations for classes associated with the "undo" feature.



    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/30/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef _INC_DSKQUOTA_H
#   include "dskquota.h"
#endif
#ifndef _INC_DSKQUOTA_DYNARRAY_H
#   include "dynarray.h"
#endif

class UndoList;  // Fwd decl.

//
// Virtual base class for all undo actions.
// UndoList maintains a list of these.
//
class UndoAction
{
    protected:
        PDISKQUOTA_CONTROL m_pQuotaControl; // Ptr to quota control object.
        PDISKQUOTA_USER m_pUser;            // User object affected by action.
        UndoList     *m_pUndoList;          // Containing undo list object.
        LONGLONG      m_llLimit;            // User object's previous quota limit.
        LONGLONG      m_llThreshold;        // Previous quota threshold.

    public:
        UndoAction(PDISKQUOTA_USER pUser, LONGLONG llThreshold, LONGLONG llLimit,
                   PDISKQUOTA_CONTROL pQuotaControl = NULL);
        ~UndoAction(VOID);

        virtual HRESULT Undo(VOID) = 0;

        VOID SetUndoList(UndoList *pUndoList)
            { m_pUndoList = pUndoList; }
};


//
// Class for restoring a deleted record.
//
class UndoDelete : public UndoAction
{
    public:
        UndoDelete(
            PDISKQUOTA_USER pUser,
            LONGLONG llThreshold,
            LONGLONG llLimit
            ) : UndoAction(pUser, llThreshold, llLimit) { }

        HRESULT Undo(VOID);
};


//
// Class for restoring a newly added record (delete it).
//
class UndoAdd : public UndoAction
{
    public:
        UndoAdd(
            PDISKQUOTA_USER pUser,
            PDISKQUOTA_CONTROL pQuotaControl
            ) : UndoAction(pUser, 0, 0, pQuotaControl) { }

        HRESULT Undo(VOID);
};


//
// Class for restoring a record's previous settings.
//
class UndoModify : public UndoAction
{
    public:
        UndoModify(
            PDISKQUOTA_USER pUser,
            LONGLONG llThreshold,
            LONGLONG llLimit
            ) : UndoAction(pUser, llThreshold, llLimit) { }

        HRESULT Undo(VOID);
};


//
// Container for a set of undo actions.
//
class UndoList
{
    private:
        PointerList        m_hList;         // List of undo action object ptrs.
        PointerList       *m_pUserList;     // List of quota user object ptrs.
        HWND               m_hwndListView;  // Listview we'll update.

    public:
        UndoList(PointerList *pUserList, HWND hwndListView)
            : m_pUserList(pUserList),
              m_hwndListView(hwndListView) { }

        ~UndoList(VOID);

        VOID Add(UndoAction *pAction)
            { 
                pAction->SetUndoList(this), 
                m_hList.Append((LPVOID)pAction); 
            }

        HWND GetListViewHwnd(VOID)
            { return m_hwndListView; }

        PointerList *GetUserList(VOID)
            { return m_pUserList; }
        
        VOID Undo(VOID);

        VOID Clear(VOID);

        INT Count(VOID)
            { return m_hList.Count(); }
};



    

#endif // _INC_DSKQUOTA_UNDO_H
