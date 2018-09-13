#ifndef _INC_DSKQUOTA_OADISP_H
#define _INC_DSKQUOTA_OADISP_H
///////////////////////////////////////////////////////////////////////////////
/*  File: oadisp.h

    Description: Provides reusable implementation of IDispatch.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef _OLEAUTO_H_
#   include <oleauto.h>
#endif

#ifndef _INC_DSKQUOTA_STRCLASS_H
#   include "strclass.h"
#endif

class OleAutoDispatch
{
    public:
        OleAutoDispatch(VOID);

        OleAutoDispatch(IDispatch *pObject,
                        REFIID riidTypeLib,
                        REFIID riidDispInterface,
                        LPCTSTR pszTypeLib);

        ~OleAutoDispatch(VOID);

        HRESULT 
        Initialize(
            IDispatch *pObject,
            REFIID riidTypeLib,
            REFIID riidDispInterface,
            LPCTSTR pszTypeLib);

        HRESULT 
        GetIDsOfNames(
            REFIID riid,  
            OLECHAR ** rgszNames,  
            UINT cNames,  
            LCID lcid,  
            DISPID *rgDispId);
        
        HRESULT 
        GetTypeInfo(
            UINT iTInfo,  
            LCID lcid,  
            ITypeInfo **ppTInfo);

        HRESULT 
        GetTypeInfoCount(
            UINT *pctinfo);

        HRESULT 
        Invoke(
            DISPID dispIdMember,  
            REFIID riid,  
            LCID lcid,  
            WORD wFlags,  
            DISPPARAMS *pDispParams,  
            VARIANT *pVarResult,  
            EXCEPINFO *pExcepInfo,  
            UINT *puArgErr);

    private:
        IDispatch *m_pObject;
        GUID       m_idTypeLib;
        GUID       m_idDispInterface;
        ITypeInfo *m_pTypeInfo;
        CString    m_strTypeLib;

        //
        // Prevent copy.
        //
        OleAutoDispatch(const OleAutoDispatch& rhs);
        OleAutoDispatch& operator = (const OleAutoDispatch& rhs);
};

#endif //_INC_DSKQUOTA_OADISP_H
