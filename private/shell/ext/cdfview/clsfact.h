//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// clsfact.h 
//
//   Definitions for the cdf viewer class factory..
//
//   History:
//
//       3/16/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Check for previous includes of this file.
//

#ifndef _CLSFACT_H_

#define _CLSFACT_H_

//
// Prototype for function used in class factory to create objects.

typedef HRESULT (*CREATEPROC)(IUnknown** ppIUnknown);

//
// Class definition for the class factory
//

class CCdfClassFactory : public IClassFactory
{
//
// Methods
//

public:

    // Constructor
    CCdfClassFactory(CREATEPROC pfn);

    // IUnknown
    STDMETHODIMP         QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //IClassFactory
    STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID, void **);
    STDMETHODIMP         LockServer(BOOL);

private:
    
    // Destructor
    ~CCdfClassFactory(void);

//
// Members
//

private:

    ULONG       m_cRef;
    CREATEPROC  m_Create;
};


#endif // _CLSFACT_H_
