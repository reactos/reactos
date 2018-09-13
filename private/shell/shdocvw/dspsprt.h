#ifndef __DSPSPRT_H__
#define __DSPSPRT_H__

// get ITypeInfo uuid/lcid out of shdocvw's type library
HRESULT SHDOCVWGetTypeInfo(LCID lcid, UUID uuid, ITypeInfo **ppITypeInfo);

//
// Helper C++ class used to share code for the IDispatch implementations
//
// Inherit from this class passing this IDispatch's IID to the ctor
// 
class CImpIDispatch
{
    public:

        // We need access to the virtual QI -- define it PURE here
        virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj) PURE;

    protected:
        CImpIDispatch(const IID * piid);
        ~CImpIDispatch(void);

        // For raising exceptions
        void Exception(WORD);

        // IDispatch members
        STDMETHODIMP GetTypeInfoCount(UINT *);
        STDMETHODIMP GetTypeInfo(UINT, LCID, ITypeInfo **);
        STDMETHODIMP GetIDsOfNames(REFIID, OLECHAR **, UINT, LCID, DISPID *);
        STDMETHODIMP Invoke(DISPID, REFIID, LCID, WORD, DISPPARAMS *, VARIANT *, EXCEPINFO *, UINT *);

    private:
        const IID *m_piid;
        IDispatch *m_pdisp;

        ITypeInfo *m_pITINeutral; // Cached Type information
};

#endif // __DSPSPRT_H__

