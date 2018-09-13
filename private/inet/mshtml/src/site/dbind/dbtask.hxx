//+----------------------------------------------------------------------------
// Classes:
//          CDataBindTask,
//
// Copyright: (c) 1996-1996, Microsoft Corporation
// All rights Reserved.
// Information contained herein is Proprietary and Confidential.

#ifndef I_DBTASK_HXX_
#define I_DBTASK_HXX_
#pragma INCMSG("--- Beg 'dbtask.hxx'")

#ifndef X_TASKMAN_HXX_
#define X_TASKMAN_HXX_
#include "taskman.hxx"
#endif

// forward references
class CDataSourceBinder;
class CCurrentRecordInstance;
class CDoc;
class CSimpleDataConverter;
interface IProgSink;

MtExtern(CDataBindTask);
MtExtern(CDataBindTask_aryCRI_pv);

//+----------------------------------------------------------------------------
//
//  Class CDataBindTask
//
//  Purpose:
//      Manage deferred binding
//
//  Created by sambent
//
//  Hungarian: dbt, pdbt
//

class CDataBindTask: public CTask
{
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDataBindTask))

    // construction/destruction
    CDataBindTask(CDoc *pDoc):
        _aryCRI(Mt(CDataBindTask_aryCRI_pv)),
        CTask(TRUE), _pDoc(pDoc)
        { _pDoc->SubAddRef(); }

    // Databinding service
    HRESULT AddDeferredBinding(CDataSourceBinder *pdsb, BOOL fSetWait=TRUE);
    HRESULT RemoveDeferredBinding(CDataSourceBinder *pdsb);
    HRESULT InitCurrentRecord(CCurrentRecordInstance *pCRI);
    void    DecideToRun();
    void    Stop();
    ISimpleDataConverter* GetSimpleDataConverter();
    
    // CTask methods
    void    OnRun(DWORD dwTimeout);
    void    OnTerminate();

    // state
    BOOL    SetEnabled(BOOL fEnabled);
    void    SetWaiting() { _fWorkWaiting = TRUE;  DecideToRun(); }
    
private:
    void    RemoveBindingFromList(CDataSourceBinder *pdsb,
                                        CDataSourceBinder **ppdsbListHead,
                                        DWORD *pdwProgCookie);

    CDoc *              _pDoc;          // document that owns me
    CDataSourceBinder * _pdsbWaiting;   // list of waiting binders
    CDataSourceBinder * _pdsbInProcess; // list of in-process binders
    ULONG               _cWaiting;      // number of waiting binders
    ULONG               _cInProcess;    // number of in-process binders
    DWORD               _dwProgCookieWait;  // Progress cookie for waiting list
    DWORD               _dwProgCookieActive; // Progress cookie for active list
    CSimpleDataConverter * _pSDC;       // for dataFormatAs = localized-text

    CPtrAry<CCurrentRecordInstance*>    _aryCRI;    // CRI's that need init
    
    unsigned            _fWorkWaiting:1;   // do the wait list at next opportunity
    unsigned            _fEnabled:1;    // is binding enabled
    unsigned            _fBinding:1;    // binding is happening
};

#pragma INCMSG("--- End 'dbtask.hxx'")
#else
#pragma INCMSG("*** Dup 'dbtask.hxx'")
#endif
