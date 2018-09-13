
#ifndef OBJWSITE_H_
#define OBJWSITE_H_

class CObjWithSite : public IObjectWithSite
{
public:
    CObjWithSite();
    ~CObjWithSite();

    // IUnknown (we multiply inherit from IUnknown, disambiguate here)
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)() PURE;
    STDMETHOD_(ULONG, Release)() PURE;
    
    // IObjectWithSite
    STDMETHOD(SetSite)(IUnknown *punkSite);
    STDMETHOD(GetSite)(REFIID riid, void **ppvSite);
            
protected:
    IUnknown *_punkSite;
};
   
#endif // OBJWSITE_H_
