#ifndef __DSPSPRT_H__
#define __DSPSPRT_H__

// get ITypeInfo uuid/lcid out of type library
HRESULT GetTypeInfoFromLibId(LCID lcid, UUID libid, USHORT wVerMajor, USHORT wVerMinor, 
                             UUID uuid, ITypeInfo **ppITypeInfo);

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
        CImpIDispatch(const IID *plibid, USHORT wVerMajor, USHORT wVerMinor, const IID * piid);
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
        const IID *m_plibid;
        USHORT    m_wVerMajor;
        USHORT    m_wVerMinor;

        ITypeInfo *m_pITINeutral; // Cached Type information
};

#endif // __DSPSPRT_H__

