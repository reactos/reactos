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
    02/04/98    Added creation of IComponent.                        BrianAu
*/
///////////////////////////////////////////////////////////////////////////////

class DiskQuotaUIClassFactory : public IClassFactory
{
    public:
        DiskQuotaUIClassFactory(void)
            : m_cRef(0) { }

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

    private:
        LONG m_cRef;

        //
        // Prevent copying.
        //
        DiskQuotaUIClassFactory(const DiskQuotaUIClassFactory& rhs);
        DiskQuotaUIClassFactory& operator = (const DiskQuotaUIClassFactory& rhs);
};

//
// Since dskquoui.dll can offer up two somewhat-related objects (a shell extension
// and an MMC snapin extension), we create this intermediary class in case someone
// calls IClassFactory::CreateInstance asking for IUnknown (MMC does this).  When
// this happens, we don't know what object the client wants.  The caller then can 
// query this proxy object for a specific interface and the proxy generates the 
// appropriate object.
//
class InstanceProxy : public IUnknown
{
    public:
        InstanceProxy(void)
            : m_cRef(0) { }

        ~InstanceProxy(void) { }
        //
        // IUnknown methods
        //
        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(VOID);
        STDMETHODIMP_(ULONG) Release(VOID);

    private:
        LONG m_cRef;
};




#endif // _INC_DSKQUOTA_FACTORY_H
