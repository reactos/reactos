//+----------------------------------------------------------------------------
//  File:       basecom.hxx
//
//  Synopsis:
//
//-----------------------------------------------------------------------------


#ifndef _BASECOM_HXX
#define _BASECOM_HXX


class CVoid;
class CUnknown;
class CComponent;


// Types ----------------------------------------------------------------------
#define DEFINE_IUNKNOWN_METHODS                                 \
        STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObj)  \
            { return PublicQueryInterface(riid, ppvObj); }      \
        STDMETHOD_(ULONG, AddRef)()                             \
            { return PublicAddRef(); }                          \
        STDMETHOD_(ULONG, Release)()                            \
            { return PublicRelease(); }

#define OWNING_CLASS(parent, member)    CONTAINING_RECORD(this, parent, member)


// Prototypes -----------------------------------------------------------------
template <class TYPE> TYPE * SAddRef(TYPE * pUnk)
{
    if (pUnk)
    {
        pUnk->AddRef();
    }
    return pUnk;
}
void SClear(IUnknown ** ppUnk);
void SRelease(IUnknown * pUnk);


//+----------------------------------------------------------------------------
//  Class:      CVoid
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
class CVoid
{
public:
};


//+----------------------------------------------------------------------------
//  Class:      CComponent
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
class CComponent : public CVoid
{
    // Private IUnknown
    class CUnknown : public IUnknown
    {
    public:
        STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObj);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();
    };

    friend class CUnknown;

public:
    CComponent(IUnknown * pUnkOuter = NULL)
        {
            _cRefs = 1;
            _pUnkOuter = (pUnkOuter
                            ? pUnkOuter
                            : &_Unk);
            IncrementThreadUsage();
        }
    virtual ~CComponent()
        {
            DecrementThreadUsage();
        }

    // IUnknown helpers
    STDMETHOD(PublicQueryInterface)(REFIID riid, void ** ppvObj);
    STDMETHOD_(ULONG, PublicAddRef)();
    STDMETHOD_(ULONG, PublicRelease)();

    IUnknown *  PrivateUnknown()
        {
        return &_Unk;
        }

protected:
    ULONG       _cRefs;
    IUnknown *  _pUnkOuter;
    CUnknown    _Unk;

    // Private QueryInterface (override to provide additional interfaces)
    virtual HRESULT PrivateQueryInterface(REFIID riid, void ** ppvObj);
};


#endif  // _BASECOM_HXX
