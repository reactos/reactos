#ifndef _COWSITE_H_
#define _COWSITE_H_

// this is a virtual class
// (since pretty much everyone overrides SetSite)

class CObjectWithSite : public IObjectWithSite
{
public:
    //*** IUnknown ****
    // (client must provide!)

    //*** IObjectWithSite ***
    virtual STDMETHODIMP SetSite(IUnknown *punkSite);
    virtual STDMETHODIMP GetSite(REFIID riid, void **ppvSite);

    ~CObjectWithSite() { ASSERT(!_punkSite); }
protected:
    IUnknown *   _punkSite;
};

#endif
