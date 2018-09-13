#ifndef _CAGGUNK_H
#define _CAGGUNK_H

class CAggregatedUnknown  : public IUnknown
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    
protected:
    virtual ~CAggregatedUnknown(); // so that subclasses get deleted right
    CAggregatedUnknown(IUnknown *punkAgg);

    // This is the IUnknown that subclasses returns from their CreateInstance func
    IUnknown* _GetInner() { return &_unkInner; }

    // A couple helper functions for subclasses to cache their aggregator's
    // (or their own) interfaces.
    void _ReleaseOuterInterface(IUnknown** ppunk);
    HRESULT _QueryOuterInterface(REFIID riid, void ** ppvOut);

    // Do non-cached QIs off this IUnknown
    IUnknown* _GetOuter() { return _punkAgg; }

    // Allow "delayed aggregation"
    void _SetOuter(IUnknown* punk) { _punkAgg = punk; }

    // This is the QueryInterface the aggregator implements
    virtual HRESULT v_InternalQueryInterface(REFIID riid, void **ppvObj) = 0;

    virtual BOOL v_HandleDelete(PLONG pcRef) { return FALSE; };
    
private:

    // Get a non-refcounted pointer to the canonical IUnknown of the
    // controlling unknown.  Used by _QueryOuterInterface and
    // _ReleaseOuterInterface.
    IUnknown *_GetCanonicalOuter(void);

    // Embed default IUnknown handler
    class CUnkInner : public IUnknown
    {
    public:
        virtual STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
        virtual STDMETHODIMP_(ULONG) AddRef(void) ;
        virtual STDMETHODIMP_(ULONG) Release(void);

        CUnkInner() { _cRef = 1; }
    private:
        LONG _cRef;
    };
    friend class CUnkInner;
    CUnkInner _unkInner;
    IUnknown* _punkAgg; // points to _unkInner or aggregating IUnknown

};

#define RELEASEOUTERINTERFACE(p) _ReleaseOuterInterface((IUnknown**)((void **)&p))

#endif // _CAGGUNK_H
