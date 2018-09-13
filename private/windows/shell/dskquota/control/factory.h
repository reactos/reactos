#ifndef _INC_DSKQUOTA_FACTORY_H
#define _INC_DSKQUOTA_FACTORY_H
///////////////////////////////////////////////////////////////////////////////
/*  File: factory.h

    Description: Contains declaration for the class factory object.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
    08/15/96    Added shell extension support.                       BrianAu
    08/20/97    Added IDispatch support.                             BrianAu
*/
///////////////////////////////////////////////////////////////////////////////

class DiskQuotaControlClassFactory : public IClassFactory
{
    private:
        LONG m_cRef;

        HRESULT Create_IDiskQuotaControl(REFIID riid, LPVOID *ppvOut);

        //
        // Prevent copying.
        //
        DiskQuotaControlClassFactory(const DiskQuotaControlClassFactory&);
        void operator = (const DiskQuotaControlClassFactory&);

    public:
        DiskQuotaControlClassFactory(void)
            : m_cRef(0) 
            { DBGTRACE((DM_CONTROL, DL_MID, TEXT("DiskQuotaControlClassFactory::DiskQuotaControlClassFactory"))); }

        //
        // IUnknown methods
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
        // IClassFactory methods
        //
        STDMETHODIMP 
        CreateInstance(
            LPUNKNOWN pUnkOuter, 
            REFIID riid, 
            LPVOID *ppvOut);

        STDMETHODIMP 
        LockServer(
            BOOL fLock);
};



#endif // _INC_DSKQUOTA_FACTORY_H
