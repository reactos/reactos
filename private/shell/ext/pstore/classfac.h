#ifndef CLASSFAC_H
#define CLASSFAC_H

class CClassFactory : public IClassFactory
{
protected:
    LONG m_ObjRefCount;

public:
    CClassFactory();
    ~CClassFactory();

    //
    // IUnknown methods
    //

    STDMETHODIMP QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(DWORD) AddRef();
    STDMETHODIMP_(DWORD) Release();

    //
    // IClassFactory methods
    //

    STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID*);
    STDMETHODIMP LockServer(BOOL);
};

#endif   // CLASSFAC_H
