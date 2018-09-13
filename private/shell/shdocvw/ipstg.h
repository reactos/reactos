#ifndef __IPSTG_H__
#define __IPSTG_H__

//
// CImpIPersistStorage works very well along-side an IPersistStreamInit
// implementation.
//
// IE30's CShellEmbedding implemented this interface because it was
// an embedding must-have. But none of our objects were marked as
// embeddable, so we really didn't need it.
//
// I pulled the implementation to a new class that can easily be
// included into any object that needs an IPersistStorange implementation
// that delegates to the object's IPersistStreamInit implementation.
//
class CImpIPersistStorage : public IPersistStorage
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj) PURE;
    virtual STDMETHODIMP_(ULONG) AddRef(void) PURE;
    virtual STDMETHODIMP_(ULONG) Release(void) PURE;

    // *** IPersist ***
    virtual STDMETHODIMP GetClassID(CLSID *pClassID) PURE;

    // *** IPersistStorage ***
    virtual STDMETHODIMP IsDirty(void) PURE; // matches IPersistStreamInit
    virtual STDMETHODIMP InitNew(IStorage *pStg);
    virtual STDMETHODIMP Load(IStorage *pStg);
    virtual STDMETHODIMP Save(IStorage *pStgSave, BOOL fSameAsLoad);
    virtual STDMETHODIMP SaveCompleted(IStorage *pStgNew);
    virtual STDMETHODIMP HandsOffStorage(void);

    // These happen to match IPersistStreamInit methods.
    // They should update the dirty state of the object as
    // returned from IsDirty().
    //
    virtual STDMETHODIMP Load(IStream *pStm) PURE;
    virtual STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty) PURE;
    virtual STDMETHODIMP InitNew(void) PURE;
};

#endif // __IPSTG_H__

