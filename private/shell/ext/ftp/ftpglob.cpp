/**************************************************************\
    FILE: ftpglob.cpp

    DESCRIPTION:
        Refcounted HGLOBAL.
\**************************************************************/

#include "priv.h"
#include "ftpglob.h"


/****************************************************\
    Constructor
\****************************************************/
CFtpGlob::CFtpGlob() : m_cRef(1)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_hglob);

    LEAK_ADDREF(LEAK_CFtpGlob);
}


/****************************************************\
    Destructor
\****************************************************/
CFtpGlob::~CFtpGlob()
{
    if (m_hglob)
        GlobalFree(m_hglob);

    DllRelease();
    LEAK_DELREF(LEAK_CFtpGlob);
}


//===========================
// *** IUnknown Interface ***
ULONG CFtpGlob::AddRef()
{
    m_cRef++;
    return m_cRef;
}

ULONG CFtpGlob::Release()
{
    ASSERT(m_cRef > 0);
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}

HRESULT CFtpGlob::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = SAFECAST(this, IUnknown*);
    }
    else
    {
        TraceMsg(TF_FTPQI, "CFtpGlob::QueryInterface() failed.");
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}



/****************************************************\
    FUNCTION: CFtpGlob_Create
  
    DESCRIPTION:
        This function will create an instance of the
    CFtpGlob object.
\****************************************************/
IUnknown * CFtpGlob_Create(HGLOBAL hglob)
{
    IUnknown * punk = NULL;
    CFtpGlob * pfg = new CFtpGlob();

    if (pfg)
    {
        pfg->m_hglob = hglob;
        pfg->QueryInterface(IID_IUnknown, (LPVOID *)&punk);
        pfg->Release();
    }

    return punk;
}



/****************************************************\
    FUNCTION: CFtpGlob_CreateStr
  
    DESCRIPTION:
        This function will create an instance of the
    CFtpGlob object.
\****************************************************/
CFtpGlob * CFtpGlob_CreateStr(LPCTSTR pszStr)
{
    CFtpGlob * pfg = new CFtpGlob();

    if (EVAL(pfg))
        pfg->m_hglob = (HGLOBAL) pszStr;

    return pfg;
}
