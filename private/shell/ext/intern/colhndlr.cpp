#include "priv.h"
#include "guids.h"

///////////////
//
// This file contains the implementation of the column handlers
// Called from fstreex's FS_HandleExtendedColumn
//
#define PIDDSI_COMPANY 0x0000000F
STDAPI CExeDllColumnProvider_CreateInstance(IUnknown *punk, REFIID riid, void **pcpOut);

//  FMTID_ExeDllInformation,
//// {0CEF7D53-FA64-11d1-A203-0000F81FEDEE}



static const SHCOLUMNID SCID_CompanyName        =   { //from the Doc Summary information.
    { 0xd5cdd502, 0x2e9c, 0x101b, 0x93, 0x97, 0x08, 0x00, 0x2b, 0x2c, 0xf9, 0xae  }//this one is overloaded.
    , PIDDSI_COMPANY};
static const SHCOLUMNID SCID_FileDescription    =   {
    { 0xcef7d53, 0xfa64, 0x11d1, 0xa2, 0x3, 0x0, 0x0, 0xf8, 0x1f, 0xed, 0xee }
    , PIDSI_FileDescription};
static const SHCOLUMNID SCID_FileVersion        =   { 
    { 0xcef7d53, 0xfa64, 0x11d1, 0xa2, 0x3, 0x0, 0x0, 0xf8, 0x1f, 0xed, 0xee }
    , PIDSI_FileVersion};
static const SHCOLUMNID SCID_InternalName       =   { 
    { 0xcef7d53, 0xfa64, 0x11d1, 0xa2, 0x3, 0x0, 0x0, 0xf8, 0x1f, 0xed, 0xee }
    , PIDSI_InternalName};  
static const SHCOLUMNID SCID_OriginalFileName   =   {    
    { 0xcef7d53, 0xfa64, 0x11d1, 0xa2, 0x3, 0x0, 0x0, 0xf8, 0x1f, 0xed, 0xee }
    , PIDSI_OriginalFileName};
static const SHCOLUMNID SCID_ProductName        =   {
    { 0xcef7d53, 0xfa64, 0x11d1, 0xa2, 0x3, 0x0, 0x0, 0xf8, 0x1f, 0xed, 0xee }
    , PIDSI_ProductName};
static const SHCOLUMNID SCID_ProductVersion     =   {
    { 0xcef7d53, 0xfa64, 0x11d1, 0xa2, 0x3, 0x0, 0x0, 0xf8, 0x1f, 0xed, 0xee }
    , PIDSI_ProductVersion};
// A PROPVARIANT can hold a few more types than a VARIANT can.  We convert the types that are
// only supported by a PROPVARIANT into equivalent VARIANT types.



typedef struct {
    const SHCOLUMNID *pscid;
    DWORD id;
    DWORD dwWid;
    VARTYPE vt;             // Note that the type of a given FMTID/PID pair is a known, fixed value
} COLMAP;



// W because pidl is always converted to widechar filename
const LPCTSTR c_szExeDllExtensions[] = {
    TEXT(".DLL"), TEXT(".EXE"), NULL
};

const COLMAP c_rgExeDllColumns[] = {
    { &SCID_CompanyName,IDS_EXCOL_CompanyName, 20, VT_BSTR },
    { &SCID_FileDescription,IDS_EXCOL_FileDescription, 20, VT_BSTR },
    { &SCID_FileVersion,IDS_EXCOL_FileVersion, 20, VT_BSTR },
    { &SCID_InternalName,IDS_EXCOL_InternalName, 20, VT_BSTR },
    { &SCID_OriginalFileName,IDS_EXCOL_OriginalFileName, 20, VT_BSTR },
    { &SCID_ProductName,IDS_EXCOL_ProductName, 20, VT_BSTR },
    { &SCID_ProductVersion,IDS_EXCOL_ProductVersion, 20, VT_BSTR },
   
};





class CDefColumnProvider :
    public IPersist, 
    public IColumnProvider
{
    // IUnknown methods
public:
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvOut)
    {
        static const QITAB qit[] = {
            QITABENT(CDefColumnProvider, IColumnProvider),           // IID_IColumnProvider
            QITABENT(CDefColumnProvider, IPersist),           // IID_IColumnProvider
            { 0 },
        };

        return QISearch(this, qit, riid, ppvOut);
    };
    
    virtual STDMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&_cRef);
    };
    virtual STDMETHODIMP_(ULONG) Release()
    {
        if (InterlockedDecrement(&_cRef))
            return _cRef;

        delete this;
        return 0;
    };

    // *** IPersist methods ***
    virtual STDMETHODIMP GetClassID(CLSID *pClassID) { *pClassID = *_pclsid; return S_OK; };

    // IColumnProvider methods
    virtual STDMETHODIMP Initialize(LPCSHCOLUMNINIT psci) {return E_NOTIMPL;};  // Inheriting class is required to impl
    virtual STDMETHODIMP GetColumnInfo(DWORD dwIndex, SHCOLUMNINFO *psci);
    virtual STDMETHODIMP GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData) = 0;

    virtual STDMETHODIMP_(ULONG) GetColumnCount() { return _iCount; };

protected:
    CDefColumnProvider(const CLSID *pclsid, const COLMAP rgColMap[], int iCount, const LPCTSTR rgExts[]) : 
       _cRef(1), _pclsid(pclsid), _rgColumns(rgColMap), _iCount(iCount), _rgExts(rgExts)
       {
           DllAddRef();
       };

    virtual ~CDefColumnProvider() 
    {
        DllRelease();
    };

    // helper fns
    BOOL _IsHandled(LPCTSTR szExtension);

    int _iCount;
    const COLMAP  *_rgColumns;

private:
    // variables
    long _cRef;

    const CLSID * _pclsid;
    const LPCTSTR *_rgExts;
};

// the index is an arbitrary zero based index used for enumeration

STDMETHODIMP CDefColumnProvider::GetColumnInfo(DWORD dwIndex, SHCOLUMNINFO *psci)
{
    ZeroMemory(psci, sizeof(*psci));
    
    if (dwIndex < (UINT) _iCount)
    {
        TCHAR szTitle[MAX_COLUMN_NAME_LEN];
        
        psci->scid = *_rgColumns[dwIndex].pscid;
        psci->cChars = _rgColumns[dwIndex].dwWid;
        psci->vt = _rgColumns[dwIndex].vt;
        psci->fmt = LVCFMT_LEFT;
        psci->csFlags = SHCOLSTATE_TYPE_STR;

        // Not called very often, easier to just load from resource file
        LoadString(HINST_THISDLL, _rgColumns[dwIndex].id, szTitle, ARRAYSIZE(szTitle));
        SHTCharToUnicode(szTitle, psci->wszTitle, ARRAYSIZE(psci->wszTitle));
        SHTCharToUnicode(szTitle, psci->wszDescription, ARRAYSIZE(psci->wszDescription));
        return S_OK;
    }

    return S_FALSE;
}

// TODO: there's probably some way to check this via
// the registry. If the BROWSABLE bit is set or something...
BOOL CDefColumnProvider::_IsHandled(LPCTSTR szExtension)
{
    int i=0;

    while (_rgExts[i])
    {
        if (0 == StrCmpI(szExtension, _rgExts[i]))
            return TRUE;
        i++;
    }
    return FALSE;
}


////////////////////////////////////////////////////////////////////////////////
// Exe and DLL  handler

class CExeDllColumnProvider : 
    public CDefColumnProvider
{
    STDMETHODIMP Initialize(LPCSHCOLUMNINIT psci) {return E_NOTIMPL;};  // We should really do something here.
    STDMETHODIMP GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData);

private:
    // help on initializing base classes: mk:@ivt:vclang/FB/DD/S44B5E.HTM
    CExeDllColumnProvider() : CDefColumnProvider(&CLSID_ExeDllColumnProvider, c_rgExeDllColumns, ARRAYSIZE(c_rgExeDllColumns), c_szExeDllExtensions)
    {
    };
    
    virtual ~CExeDllColumnProvider() 
    {
    };
    // friends
    friend HRESULT CExeDllColumnProvider_CreateInstance(IUnknown *punk, REFIID riid, void **pcpOut);
};

STDMETHODIMP CExeDllColumnProvider::GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData)
{
    TCHAR szPath[MAX_PATH];

    HRESULT hr = E_OUTOFMEMORY;

    // should we match against a list of known extensions, or always try to open?
    StrCpyN(szPath, TEXT("C:\\"), ARRAYSIZE(szPath));

    // clear our output
    if (!_IsHandled(PathFindExtension(szPath))) //check to see if we should be handling this file.
        return S_FALSE;


    //start stuff specific to the DLL and EXE versioning.

    DWORD   versionISize; //A Dword to receive the size of the whole version structure
    DWORD   dwVestigial;  //A Dword to be ignored.
    LPVOID  pvAllTheInfo; //A pointer to the whole version data structure
    LPTSTR  pszVersionInfo = NULL; //A pointer to the specific version info I am looking for
    UINT    uInfoSize=0;        //the size of the info returned.
    //stolen from Relja's code
    TCHAR   szVersionKey[60];   //a string to hold all the format string for VerQueryValue
    TCHAR   szField[40];        //a temporary variable to get the field-specific formatting info

     struct _VERXLATE 
    {
        WORD wLanguage;
        WORD wCodePage;
    } *lpXlate;                     /* ptr to translations data */



    versionISize=GetFileVersionInfoSize(szPath, &dwVestigial); //gets the version info size.
    pvAllTheInfo= new BYTE[versionISize];

    if (!(pvAllTheInfo)) 
        return E_OUTOFMEMORY; // error, out of memory.

    if (!(GetFileVersionInfo(szPath, dwVestigial, versionISize, pvAllTheInfo)))//assign actual version stuff in TheInfo
    {
        delete (BYTE*) pvAllTheInfo;
        return E_FAIL;// fubar use GetLastError for more info
    }
    //AllTheInfo is now initialized.

    //setup the field name we are looking for based on the SCID passed in
    switch (pscid->pid){
        case PIDDSI_COMPANY: strcpy(szField,TEXT("CompanyName")); break;
        case PIDSI_FileDescription: strcpy(szField,TEXT("FileDescription")); break;
        case PIDSI_FileVersion: strcpy(szField,TEXT("FileVersion")); break;
        case PIDSI_InternalName: strcpy(szField,TEXT("InternalName")); break;
        case PIDSI_OriginalFileName: strcpy(szField,TEXT("OriginalFileName")); break;
        case PIDSI_ProductName: strcpy(szField,TEXT("ProductName")); break;
        case PIDSI_ProductVersion: strcpy(szField,TEXT("ProductVersion")); break;
        default: delete (BYTE*)pvAllTheInfo; return E_FAIL; break;
        };
    //look for the intended language in the examined object.

    //this is a fallthrough set of if statements.
    //on a failure, it just tries the next one, until it runs out of tries.
    if (VerQueryValue(pvAllTheInfo, TEXT("\\VarFileInfo\\Translation"), (void **)&lpXlate, &uInfoSize))
    {
        wnsprintf(szVersionKey, ARRAYSIZE(szVersionKey), TEXT("\\StringFileInfo\\%04X%04X\\%s"),
                                            lpXlate[0].wLanguage, lpXlate[0].wCodePage, szField);
        if (!VerQueryValue(pvAllTheInfo, szVersionKey, (LPVOID*) &pszVersionInfo, &uInfoSize))
        {
#ifdef UNICODE
        wnsprintf(szVersionKey, ARRAYSIZE(szVersionKey), TEXT("\\StringFileInfo\\040904B0\\%s"),szField);
        if (!VerQueryValue(pvAllTheInfo, szVersionKey,(LPVOID*) &pszVersionInfo, &uInfoSize))
#endif
        {
            wnsprintf(szVersionKey, ARRAYSIZE(szVersionKey), TEXT("\\StringFileInfo\\040904E4\\%s"),szField);
            if (!VerQueryValue(pvAllTheInfo, szVersionKey, (LPVOID*) &pszVersionInfo, &uInfoSize))
            {
                wnsprintf(szVersionKey, ARRAYSIZE(szVersionKey), TEXT("\\StringFileInfo\\04090000\\%s"),szField);
                if (!VerQueryValue(pvAllTheInfo, szVersionKey, (LPVOID*) &pszVersionInfo, &uInfoSize))
                {
                    pszVersionInfo=NULL;
                }
            
            }
        }
        }
    };
    
    //getting here is success, prepare the output, clean up memory and exit.
    if (pszVersionInfo)
    {
        pvarData->vt = VT_BSTR;
        pvarData->bstrVal = TCharSysAllocString(pszVersionInfo);
        hr = S_OK;
    }
    else
        hr=E_FAIL;

    delete (BYTE*)pvAllTheInfo;
    return hr;
};


STDAPI CExeDllColumnProvider_CreateInstance(IUnknown *punk, REFIID riid, void **pcpOut)
{
    HRESULT hres;
    CExeDllColumnProvider *pdocp = new CExeDllColumnProvider;
    if (pdocp)
    {
        hres = pdocp->QueryInterface(riid, pcpOut);
        pdocp->Release();
    }
    else
    {
        *pcpOut = NULL;
        hres = E_OUTOFMEMORY;
    };

    return hres;
};
