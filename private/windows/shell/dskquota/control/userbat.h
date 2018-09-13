#ifndef _INC_DSKQUOTA_USERBAT_H
#define _INC_DSKQUOTA_USERBAT_H
///////////////////////////////////////////////////////////////////////////////
/*  File: userbat.h

    Description: Provides declaration for class DiskQuotaUserBatch.
        This class is provided to allow batch updates of quota user information.



    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/07/96    Initial creation.                                    BrianAu
    09/03/96    Added exception handling.                            BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef _INC_DSKQUOTA_H
#   include "dskquota.h"
#endif
#ifndef _INC_DSKQUOTA_FSOBJECT_H
#   include "fsobject.h"
#endif
#ifndef _INC_DSKQUOTA_EXCEPT_H
#   include "except.h"
#endif

class DiskQuotaUserBatch : public IDiskQuotaUserBatch
{
    private:
        LONG              m_cRef;            // Ref counter.
        FSObject         *m_pFSObject;       // Ptr to file sys object.
        CArray<PDISKQUOTA_USER> m_UserList;  // List of users to batch process.
        
        HRESULT
        RemoveUser(
            PDISKQUOTA_USER pUser);

        STDMETHODIMP
        RemoveAllUsers(VOID);

        VOID
        Destroy(
            VOID);

        //
        // Prevent copying.
        //
        DiskQuotaUserBatch(const DiskQuotaUserBatch& );
        DiskQuotaUserBatch& operator = (const DiskQuotaUserBatch& );

    public:
//
// BUGBUG: Find out why compiler doesn't like this exception decl.
//
//        DiskQuotaUserBatch(FSObject *pFSObject) throw(OutOfMemory, SyncObjErrorCreate);
        DiskQuotaUserBatch(FSObject *pFSObject);
        ~DiskQuotaUserBatch(VOID);

        //
        // IUnknown interface.
        //
        STDMETHODIMP         
        QueryInterface(
            REFIID, 
            LPVOID *);

        STDMETHODIMP_(ULONG) 
        AddRef(
            VOID);

        STDMETHODIMP_(ULONG) 
        Release(
            VOID);

        //
        // IDiskQuotaUserBatch interface.
        //
        STDMETHODIMP
        Add(
            PDISKQUOTA_USER);

        STDMETHODIMP
        Remove(
            PDISKQUOTA_USER);

        STDMETHODIMP
        RemoveAll(
            VOID);

        STDMETHODIMP
        FlushToDisk(
            VOID);
};

#endif // _INC_DSKQUOTA_USERBAT_H

