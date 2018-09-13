#ifndef COM_H_INCLUDED
#define COM_H_INCLUDED

#include "userinfo.h"
#include "grpinfo.h"
#include "unpage.h"
#include "grppage.h"

class CUnknownImpl
{
public:
    // IUnknown ref counting
    ULONG BaseAddRef()
    {return InterlockedIncrement((long*) &m_cRef);}

    ULONG BaseRelease()
    {
        if (InterlockedDecrement((long*) &m_cRef) == 0)
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }

    CUnknownImpl(): m_cRef(1) {InterlockedIncrement((long*) &g_cLocks);}

    virtual ~CUnknownImpl() {InterlockedDecrement((long*) &g_cLocks);}

private:
    ULONG m_cRef;
};

class CUserPropertyPages: public CUnknownImpl, public IShellExtInit, public IShellPropSheetExt
{
public:
    // IUnknown
    HRESULT __stdcall QueryInterface(REFIID iid, LPVOID* ppvOut);
    ULONG __stdcall AddRef() {return BaseAddRef();}
    ULONG __stdcall Release() {return BaseRelease();}

    // IShellExtInit
    HRESULT __stdcall Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, HKEY hkeyProgID);

    // IShellPropSheetExt
    HRESULT __stdcall AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
    HRESULT __stdcall ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam);

    CUserPropertyPages(): 
        CUnknownImpl(), m_pUserInfo(NULL), m_pUsernamePage(NULL), m_pGroupPage(NULL) {}
    
    ~CUserPropertyPages();

private:
    // The user for the property sheet
    CUserInfo* m_pUserInfo;

    // Basic info page, only shown for local users
    CUsernamePropertyPage* m_pUsernamePage;

    // The group page, which is common to both local and domain users
    CGroupPropertyPage* m_pGroupPage;

    // The group list, used by the group page
    CGroupInfoList m_GroupList;
};

class CUserPropertyPagesFactory: public CUnknownImpl, public IClassFactory
{
public:
    // IUnknown
    HRESULT __stdcall QueryInterface(REFIID iid, LPVOID* ppvOut);
    ULONG __stdcall AddRef() {return BaseAddRef();}
    ULONG __stdcall Release() {return BaseRelease();}

    // IClassFactory
    HRESULT __stdcall CreateInstance(IUnknown * pUnkOuter, REFIID riid, void ** ppvObject);
    HRESULT __stdcall LockServer(BOOL fLock);
    CUserPropertyPagesFactory(): CUnknownImpl() {}
};

class CUserSidDataObject: public CUnknownImpl, public IDataObject
{
public:
    // IUnknown
    HRESULT __stdcall QueryInterface(REFIID iid, LPVOID* ppvOut);
    ULONG __stdcall AddRef() {return BaseAddRef();}
    ULONG __stdcall Release() {return BaseRelease();}

    // IDataObject
    HRESULT __stdcall GetData(FORMATETC* pFormatEtc, STGMEDIUM* pMedium);
    HRESULT __stdcall GetDataHere(FORMATETC* pFormatEtc, STGMEDIUM* pMedium);
    HRESULT __stdcall QueryGetData(FORMATETC* pFormatEtc);
    HRESULT __stdcall GetCanonicalFormatEtc(FORMATETC* pFormatetcIn, FORMATETC* pFormatetcOut);
    HRESULT __stdcall SetData(FORMATETC* pFormatetc, STGMEDIUM* pmedium, BOOL fRelease);
    HRESULT __stdcall EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC ** ppenumFormatetc);
    HRESULT __stdcall DAdvise(FORMATETC* pFormatetc, DWORD advf, IAdviseSink* pAdvSink, DWORD * pdwConnection);
    HRESULT __stdcall DUnadvise(DWORD dwConnection);
    HRESULT __stdcall EnumDAdvise(IEnumSTATDATA ** ppenumAdvise);

    CUserSidDataObject(): CUnknownImpl(), m_psid(NULL) {}
    HRESULT SetSid(PSID psid);
    ~CUserSidDataObject();

private:
    PSID m_psid;
};

#endif // !COM_H_INCLUDED