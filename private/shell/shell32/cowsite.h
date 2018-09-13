#ifndef _COWSITE_H_
#define _COWSITE_H_

class CObjectWithSite : public IObjectWithSite
{
public:
#if DEBUG
    CObjectWithSite()  {ASSERT(_punkSite == NULL);}
#endif
    ~CObjectWithSite() {ATOMICRELEASE(_punkSite);}

    //*** IUnknown ****
    // (client must provide!)

    //*** IObjectWithSite ***
    STDMETHOD(SetSite)(IUnknown *punkSite);
    STDMETHOD(GetSite)(REFIID riid, void **ppvSite);

protected:
    IUnknown*   _punkSite;
};

#endif
