#ifndef APITESTS_UNKNOWNBASE_H
#define APITESTS_UNKNOWNBASE_H

template<typename Interface>
class CUnknownBase : public Interface
{
    LONG m_lRef;
    bool m_AutoDelete;
protected:
    virtual const QITAB* GetQITab() = 0;
public:

    CUnknownBase(bool autoDelete, LONG initialRef)
        : m_lRef(initialRef),
        m_AutoDelete(autoDelete)
    {
    }

   ULONG STDMETHODCALLTYPE AddRef ()
   {
       return InterlockedIncrement( &m_lRef );
   }

   ULONG STDMETHODCALLTYPE Release()
   {
       long newref = InterlockedDecrement( &m_lRef );
       if (m_AutoDelete && newref<=0)
       {
           delete this;
       }
       return newref;
   }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv)
    {
        return QISearch(this, GetQITab(), riid, ppv);
    }

    virtual ~CUnknownBase() {}

    LONG GetRef() const
    {
        return m_lRef;
    }
};

#endif // APITESTS_UNKNOWNBASE_H
