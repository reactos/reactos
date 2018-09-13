//+---------------------------------------------------------------------
//
//   File:       pbfact.cxx
//
//   Contents:   Paintbrush Class Factory
//
//   Classes:    PBFactory
//
//------------------------------------------------------------------------

#include <stdlib.h>

#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>      // common dialog boxes

#include <ole2.h>
#include <o2base.hxx>     // the base classes and utilities

#include <initguid.h>
#include "pbs.hxx"

    PBFactory *gpPBFactory = NULL;
    DWORD gdwRegisterClass = 0;

    extern LPPBCTRL gpCtrlThis; // the This pointer for our single, global object


extern "C" BOOL
CreatePBClassFactory(HINSTANCE hinst,BOOL fEmbedded)
{
    BOOL fRet = FALSE;
    if(PBFactory::Create(hinst))
    {
        gpPBFactory->AddRef();
        if(fEmbedded)
        {
            DOUT(L"PBrush: Registering PBFactory\r\n");

            HRESULT hr = CoRegisterClassObject(CLSID_PBRUSH32,
                    (LPUNKNOWN)(LPCLASSFACTORY)gpPBFactory,
                    CLSCTX_LOCAL_SERVER,
                    REGCLS_SINGLEUSE,
                    &gdwRegisterClass);
            if(OK(hr))
            {
                CoLockObjectExternal((LPUNKNOWN)(LPCLASSFACTORY)gpPBFactory,
                    TRUE, TRUE);
                fRet = TRUE;
            }
#if DBG
            else
            {
                OLECHAR achBuffer[256];
                wsprintf(achBuffer,
                        L"PBrush: CoRegisterClassObject (%lx)\r\n",
                        (long)hr);
                DOUT(achBuffer);
            }
#endif
        }
        else
        {
            fRet = TRUE;
        }
    }

    return fRet;
}

extern "C" HRESULT
ReleasePBClassFactory(void)
{
    HRESULT hr = NOERROR;
    if(gdwRegisterClass)
    {
        DOUT(L"PBrush: Revoking PBFactory\r\n");

        hr = CoRevokeClassObject(gdwRegisterClass);
        gdwRegisterClass = 0;
        CoLockObjectExternal((LPUNKNOWN)(LPCLASSFACTORY)gpPBFactory,
             FALSE, TRUE);
        gpPBFactory->Release();
    }
    return hr;
}

BOOL
PBFactory::Create(HINSTANCE hinst)
{
    gpPBFactory = new PBFactory;
    //
    // initialize all the classes
    //
    if (gpPBFactory == NULL
             || !gpPBFactory->Init(hinst)
             || !PBCtrl::ClassInit(gpPBFactory->_pClass)
             || !PBDV::ClassInit(gpPBFactory->_pClass)
             || !PBInPlace::ClassInit(gpPBFactory->_pClass))
    {
        return FALSE;
    }
    return TRUE;
}

//+---------------------------------------------------------------
//
//  Member:     PBFactory:::Init
//
//  Synopsis:   Initializes the class factory
//
//  Arguments:  [hinst] -- instance handle of the module with
//                         class descriptor resources
//
//  Returns:    TRUE iff the factory was successfully initialized
//
//----------------------------------------------------------------

extern "C" {
extern TCHAR pgmName[7];    //BUGBUG hacked in reference
}

BOOL
PBFactory::Init(HINSTANCE hinst)
{
    //
    // Register the standard OLE clipboard formats.
    // These are available in the OleClipFormat array.
    //
    DOUT(L"PBFactory::Init Registering OLE Clipboard Formats\r\n");
    RegisterOleClipFormats();
    if((_pClass = new ClassDescriptor) == NULL)
        return FALSE;

    BOOL fRet = _pClass->Init(hinst, IDS_CLASSID);
    //
    //BUGBUG need to patch _pClass->_haccel with a reload of
    //       resource for InPlace due to mismatch between
    //       base class resource scheme and legacy code...
    if(fRet)
    {
        if((_pClass->_haccel = LoadAccelerators(hinst, pgmName)) == NULL)
            fRet = FALSE;
    }
    return fRet;
}

STDMETHODIMP
PBFactory::LockServer(BOOL fLock)
{
    return CoLockObjectExternal((LPUNKNOWN)gpPBFactory, fLock, TRUE);
}

//+---------------------------------------------------------------
//
//  Member:     PBFactory:::CreateInstance
//
//  Synopsis:   Member of IClassFactory interface
//
//----------------------------------------------------------------

STDMETHODIMP
PBFactory::CreateInstance(LPUNKNOWN pUnkOuter, REFIID iid, LPVOID FAR* ppv)
{
    DOUT(L"PBFactory::CreateInstance\r\n");

    HRESULT hr;
    LPUNKNOWN pUnk;
    LPPBCTRL pTemp;
    //
    // create an object, then query for the appropriate interface
    //
    if (OK(hr = PBCtrl::Create(pUnkOuter, _pClass, &pUnk, &pTemp)))
    {
        hr = pUnk->QueryInterface(iid, ppv);
        pUnk->Release();    // on failure this will release obj, otherwise
                            // it will ensure an object ref count of 1
    }
    return hr;
}


BOOL
CreateStandaloneObject(void)
{
    DOUT(L"PBrush CreateStandaloneObject\r\n");

    //
    //Ensure a unique filename in gachLinkFilename so we can create valid
    //FileMonikers...
    //
    if(gachLinkFilename[0] == L'\0')
        BuildUniqueLinkName();

    BOOL fSucess = FALSE;
    LPVOID pvUnk;
    HRESULT hr = gpPBFactory->CreateInstance(NULL, IID_IUnknown, (LPVOID FAR*)&pvUnk);
    if(hr == NOERROR)
    {
        //CoLockObjectExternal((LPUNKNOWN)(LPOLEOBJECT)gpCtrlThis, TRUE, TRUE);
        fSucess = TRUE;
    }
    else
    {
        DOUT(L"PBrush CreateStandaloneObject FAILED!\r\n");
        fSucess = FALSE;
    }
    return fSucess;
}
