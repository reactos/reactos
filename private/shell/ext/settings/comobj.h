#ifndef __COM_OBJECT_H
#define __COM_OBJECT_H

class ComObject : public IUnknown
{
    private:
        LONG m_cRef;

    public:
        ComObject(VOID)
            : m_cRef(0) { }

        virtual ~ComObject(VOID) { }

        //
        // IUnknown methods.
        //
        STDMETHODIMP         
        QueryInterface(
            REFIID riid, 
            LPVOID *ppvOut);

        STDMETHODIMP_(ULONG) 
        AddRef(
            VOID);

        STDMETHODIMP_(ULONG) 
        Release(
            VOID);

};


#endif // __COM_OBJECT_H
