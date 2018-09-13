/*
* urlprop.cpp - Implementation for URLProp class.
*/


#include "priv.h"
#include "ishcut.h"

STDAPI_(LPITEMIDLIST) IEILCreate(UINT cbSize);

#define MAX_BUF_INT         (1 + 10 + 1)        // -2147483647

const TCHAR c_szIntshcut[]       = ISHCUT_INISTRING_SECTION;



#ifdef DEBUG

BOOL IsValidPCURLProp(PCURLProp pcurlprop)
{
    return (IS_VALID_READ_PTR(pcurlprop, CURLProp) &&
            (NULL == pcurlprop->m_hstg ||
             IS_VALID_HANDLE(pcurlprop->m_hstg, PROPSTG)));
}


BOOL IsValidPCIntshcutProp(PCIntshcutProp pcisprop)
{
    return (IS_VALID_READ_PTR(pcisprop, CIntshcutProp) &&
            IS_VALID_STRUCT_PTR(pcisprop, CURLProp));
}

BOOL IsValidPCIntsiteProp(PCIntsiteProp pcisprop)
{
    return (IS_VALID_READ_PTR(pcisprop, CIntsiteProp) &&
            IS_VALID_STRUCT_PTR(pcisprop, CURLProp));
}


#endif


BOOL AnyMeatW(LPCWSTR pcsz)
{
    ASSERT(! pcsz || IS_VALID_STRING_PTRW(pcsz, -1));
    
    return(pcsz ? StrSpnW(pcsz, L" \t") < lstrlenW(pcsz) : FALSE);
}


BOOL AnyMeatA(LPCSTR pcsz)
{
    ASSERT(! pcsz || IS_VALID_STRING_PTRA(pcsz, -1));
    
    return(pcsz ? StrSpnA(pcsz, " \t") < lstrlenA(pcsz) : FALSE);
}


/*----------------------------------------------------------
Purpose: Read an arbitrary named string from the .ini file.

Returns: S_OK if the name exists
         S_FALSE if it doesn't

         E_OUTOFMEMORY
*/
HRESULT ReadStringFromFile(IN  LPCTSTR    pszFile, 
                           IN  LPCTSTR    pszSectionName,
                           IN  LPCTSTR    pszName,
                           OUT LPWSTR *   ppwsz,
                           IN  CHAR *     pszBuf)
{
    HRESULT hres = E_OUTOFMEMORY;
    
    ASSERT(IS_VALID_STRING_PTR(pszFile, -1));
    ASSERT(IS_VALID_STRING_PTR(pszName, -1));
    ASSERT(IS_VALID_WRITE_PTR(ppwsz, PWSTR));
    
    *ppwsz = (LPWSTR)LocalAlloc(LPTR, SIZEOF(WCHAR) * INTERNET_MAX_URL_LENGTH);
    if (*ppwsz)
    {
        DWORD cch;
        
        hres = S_OK;

        cch = SHGetIniString(pszSectionName, pszName,
            *ppwsz, INTERNET_MAX_URL_LENGTH, pszFile);
        if (0 == cch)                                
        {
            hres = S_FALSE;
            LocalFree(*ppwsz);
            *ppwsz = NULL;
        }
    }
    
    return hres;
}

/*----------------------------------------------------------
Purpose: Read an arbitrary named string from the .ini file.
         Return a BSTR

Returns: S_OK if the name exists
         S_FALSE if it doesn't

         E_OUTOFMEMORY
*/
HRESULT ReadBStrFromFile(IN  LPCTSTR      pszFile, 
                           IN  LPCTSTR    pszSectionName,
                           IN  LPCTSTR    pszName,
                           OUT BSTR *     pBStr)
{
    CHAR szTempBuf[INTERNET_MAX_URL_LENGTH];
    WCHAR *pwsz;
    HRESULT hres = E_OUTOFMEMORY;
    *pBStr = NULL;
    ASSERT(IS_VALID_STRING_PTR(pszFile, -1));
    ASSERT(IS_VALID_STRING_PTR(pszName, -1));
    ASSERT(IS_VALID_WRITE_PTR(pBStr, PWSTR));

    // (Pass in an empty string so we can determine from the return
    // value whether there is any text associated with this name.)
    hres = ReadStringFromFile(pszFile, pszSectionName, pszName, &pwsz, szTempBuf);
    if (S_OK == hres)                                
    {
        *pBStr = SysAllocString(pwsz);
        LocalFree(pwsz);
    }

    return hres;
}

/*----------------------------------------------------------
Purpose: read an arbitrary named unsigend int from the .ini file. note in order to implement
         ReadSignedFromFile one'll need to use ReadStringFromFile and then StrToIntEx. this is
         because GetPrivateProfileInt can't return a negative.

Returns: S_OK if the name exists
         S_FALSE if it doesn't

         E_OUTOFMEMORY
*/
HRESULT
ReadUnsignedFromFile(
    IN LPCTSTR pszFile,
    IN LPCTSTR pszSectionName,
    IN LPCTSTR pszName,
    IN LPDWORD pdwVal)
{
    HRESULT hr;
    int     iValue;

    ASSERT(IS_VALID_STRING_PTR(pszFile,        -1));
    ASSERT(IS_VALID_STRING_PTR(pszSectionName, -1));
    ASSERT(IS_VALID_STRING_PTR(pszName,        -1));

    if (NULL == pdwVal)
        return E_INVALIDARG;
    *pdwVal = 0;

    hr     = S_OK;
    iValue = GetPrivateProfileInt(pszSectionName, pszName, 1, pszFile);
    if (1 == iValue) {
        iValue = GetPrivateProfileInt(pszSectionName, pszName, 2, pszFile);
        hr     = (2 != iValue) ? S_OK : S_FALSE;
        ASSERT(S_FALSE == hr || 1 == iValue);
    }

    if (S_OK == hr)
        *pdwVal = (DWORD)iValue;

    return hr;
}

/*----------------------------------------------------------
Purpose: Write number to URL (ini) file

*/
HRESULT WriteSignedToFile(IN LPCTSTR  pszFile,
                          IN LPCTSTR  pszSectionName,
                          IN LPCTSTR  pszName,
                          IN int      nVal)
{
    HRESULT hres;
    TCHAR szVal[MAX_BUF_INT];
    int cch;
    
    ASSERT(IS_VALID_STRING_PTR(pszFile, -1));
    ASSERT(IS_VALID_STRING_PTR(pszName, -1));
    
    cch = wnsprintf(szVal, ARRAYSIZE(szVal), TEXT("%d"), nVal);
    ASSERT(cch > 0);
    ASSERT(cch < SIZECHARS(szVal));
    ASSERT(cch == lstrlen(szVal));      // Dude, talk about anal...
    
    hres = WritePrivateProfileString(pszSectionName, pszName, szVal,
        pszFile) ? S_OK : E_FAIL;
    
    return hres;
}


/*----------------------------------------------------------
Purpose: Write number to URL (ini) file

*/
HRESULT WriteUnsignedToFile(IN LPCTSTR  pszFile,
                            IN  LPCTSTR pszSectionName,
                            IN LPCTSTR  pszName,
                            IN DWORD    nVal)
{
    HRESULT hres;
    TCHAR szVal[MAX_BUF_INT];
    int cch;
    
    ASSERT(IS_VALID_STRING_PTR(pszFile, -1));
    ASSERT(IS_VALID_STRING_PTR(pszName, -1));
    
    cch = wnsprintf(szVal, ARRAYSIZE(szVal), TEXT("%u"), nVal);
    ASSERT(cch > 0);
    ASSERT(cch < SIZECHARS(szVal));
    ASSERT(cch == lstrlen(szVal));      // Dude, talk about anal...
    
    hres = WritePrivateProfileString(pszSectionName, pszName, szVal,
        pszFile) ? S_OK : E_FAIL;
    
    return hres;
}


/*----------------------------------------------------------
Purpose: Write binary data to URL (ini) file

*/
HRESULT WriteBinaryToFile(IN LPCTSTR pszFile,
                          IN  LPCTSTR pszSectionName,
                          IN LPCTSTR pszName,
                          IN LPVOID  pvData,
                          IN DWORD   cbSize)
{
    HRESULT hres;
    
    ASSERT(IS_VALID_STRING_PTR(pszFile, -1));
    ASSERT(IS_VALID_STRING_PTR(pszName, -1));

    hres = (WritePrivateProfileStruct(pszSectionName, pszName, pvData, cbSize, pszFile))
        ? S_OK : E_FAIL;
    
    return hres;
}


/*----------------------------------------------------------
Purpose: Read the hotkey from the URL (ini) file

*/
HRESULT ReadBinaryFromFile(IN LPCTSTR pszFile,
                           IN LPCTSTR pszSectionName,
                           IN LPCTSTR pszName,
                           IN LPVOID  pvData,
                           IN DWORD   cbData)
{
    HRESULT hres = S_FALSE;
    
    ASSERT(IS_VALID_STRING_PTR(pszFile, -1));
    
    memset(pvData, 0, cbData);
    
    if (GetPrivateProfileStruct(pszSectionName, pszName, pvData, cbData, pszFile))
        hres = S_OK;
    
    return hres;
}


/*----------------------------------------------------------
Purpose: Real the URL from the URL (ini) file

*/
HRESULT 
ReadURLFromFile(
    IN  LPCTSTR  pszFile, 
    IN  LPCTSTR pszSectionName,
    OUT LPTSTR * ppsz)
{
    HRESULT hres = E_OUTOFMEMORY;
    
    *ppsz = (LPTSTR)LocalAlloc(LPTR, SIZEOF(TCHAR) * INTERNET_MAX_URL_LENGTH);
    if (*ppsz)
    {
        DWORD cch;

        cch = SHGetIniString(pszSectionName, ISHCUT_INISTRING_URL,
            *ppsz, INTERNET_MAX_URL_LENGTH, pszFile);
        if (0 != cch)
        {
            PathRemoveBlanks(*ppsz);
            hres = S_OK;
        }
        else
        {
            LocalFree(*ppsz);
            *ppsz = NULL;    
            hres = S_FALSE;     
        }
    }
    
    return hres;
}


/*----------------------------------------------------------
Purpose: Read the icon location from the URL (ini) file

Returns: S_OK  value was obtained from file
         S_FALSE value wasn't in file

         E_OUTOFMEMORY
*/
HRESULT 
ReadIconLocation(
    IN  LPCTSTR  pszFile,
    OUT LPWSTR * ppwsz,
    OUT int *    pniIcon,
    IN CHAR *    pszBuf)
{
    HRESULT hres = E_OUTOFMEMORY;
    DWORD cch;
    
    ASSERT(IS_VALID_STRING_PTR(pszFile, -1));
    ASSERT(IS_VALID_WRITE_PTR(ppwsz, PTSTR));
    ASSERT(IS_VALID_WRITE_PTR(pniIcon, INT));
    
    *ppwsz = NULL;
    *pniIcon = 0;
    
    *ppwsz = (LPWSTR)LocalAlloc(LPTR, SIZEOF(WCHAR) * MAX_PATH);
    if (*ppwsz)
    {
        hres = S_FALSE;     // assume no value exists in the file
        
        cch = SHGetIniString(c_szIntshcut,
           ISHCUT_INISTRING_ICONFILE, *ppwsz,
            MAX_PATH, pszFile);
        
        if (0 != cch)
        {
            TCHAR szIndex[MAX_BUF_INT];
            // The icon index is all ASCII so don't need SHGetIniString
            cch = GetPrivateProfileString(c_szIntshcut,
                ISHCUT_INISTRING_ICONINDEX, c_szNULL, 
                szIndex, SIZECHARS(szIndex),
                pszFile);
            if (0 != cch)
            {
                if (StrToIntEx(szIndex, 0, pniIcon))
                    hres = S_OK;
            }
        }
        
        if (S_OK != hres)
        {
            LocalFree(*ppwsz);
            *ppwsz = NULL;    
        }
    }
    
    return hres;
}


/*----------------------------------------------------------
Purpose: Write icon location to URL (ini) file

*/
HRESULT 
    WriteIconFile(
    IN LPCTSTR pszFile,
    IN LPCWSTR pszIconFile)
{
    HRESULT hres = S_OK;
    
    ASSERT(IS_VALID_STRING_PTR(pszFile, -1));
    ASSERT(! pszIconFile ||
        IS_VALID_STRING_PTRW(pszIconFile, -1));
    
    if (*pszFile)
    {
        if (AnyMeatW(pszIconFile))
        {
            hres = SHSetIniString(c_szIntshcut, ISHCUT_INISTRING_ICONFILE, pszIconFile,
                pszFile) ? S_OK : E_FAIL;
        }
        else
        {
            // NOTE: since this function removes both the file and the index
            // values, then this function must be called *after* any call 
            // to WriteIconIndex.  One way to do this is make sure 
            // PID_IS_ICONINDEX < PID_IS_ICONFILE, since the index will
            // be enumerated first.
            
            hres = (SHDeleteIniString(c_szIntshcut, ISHCUT_INISTRING_ICONFILE,
                pszFile) &&
                DeletePrivateProfileString(c_szIntshcut, ISHCUT_INISTRING_ICONINDEX,
                pszFile))
                ? S_OK : E_FAIL;
        }
    }
    
    return hres;
}


/*----------------------------------------------------------
Purpose: Write icon index to URL (ini) file

*/
HRESULT 
WriteIconIndex(
    IN LPCTSTR pszFile,
    IN int     niIcon)
{
    HRESULT hres;
    
    if (*pszFile)
        hres = WriteSignedToFile(pszFile, c_szIntshcut, ISHCUT_INISTRING_ICONINDEX, niIcon);
    else
        hres = S_FALSE;
    
    return hres;
}


/*----------------------------------------------------------
Purpose: Read the hotkey from the URL (ini) file

*/
HRESULT 
ReadHotkey(
    IN LPCTSTR pszFile, 
    IN WORD *  pwHotkey)
{
    HRESULT hres = S_FALSE;
    TCHAR szHotkey[MAX_BUF_INT];
    DWORD cch;
    
    ASSERT(IS_VALID_STRING_PTR(pszFile, -1));
    ASSERT(IS_VALID_WRITE_PTR(pwHotkey, WORD));
    
    *pwHotkey = 0;
    
    cch = GetPrivateProfileString(c_szIntshcut,
        TEXT("Hotkey"), c_szNULL,
        szHotkey, SIZECHARS(szHotkey),
        pszFile);
    if (0 != cch)
    {
        int nVal;
        
        if (StrToIntEx(szHotkey, 0, &nVal))
        {
            *pwHotkey = nVal;
            hres = S_OK;
        }
    }
    
    return hres;
}


/*----------------------------------------------------------
Purpose: Write hotkey to URL (ini) file

*/
HRESULT 
WriteHotkey(
    IN LPCTSTR pszFile, 
    IN WORD    wHotkey)
{
    HRESULT hres = S_FALSE;
    
    ASSERT(IS_VALID_STRING_PTR(pszFile, -1));
    
    if (*pszFile)
    {
        if (wHotkey)
        {
            hres = WriteUnsignedToFile(pszFile, c_szIntshcut, TEXT("Hotkey"), wHotkey);
        }
        else
        {
            hres = DeletePrivateProfileString(c_szIntshcut, TEXT("Hotkey"), pszFile)
                ? S_OK
                : E_FAIL;
        }
    }
    
    return hres;
}


/*----------------------------------------------------------
Purpose: Read the working directory from the URL (ini) file

*/
HRESULT 
ReadWorkingDirectory(
    IN  LPCTSTR  pszFile,
    OUT LPWSTR * ppwsz)
{
    HRESULT hres = E_OUTOFMEMORY;
    TCHAR szPath[MAX_PATH];
    DWORD cch;
    
    ASSERT(IS_VALID_STRING_PTR(pszFile, -1));
    ASSERT(IS_VALID_WRITE_PTR(ppwsz, PWSTR));
    
    *ppwsz = NULL;
    
    *ppwsz = (LPWSTR)LocalAlloc(LPTR, SIZEOF(WCHAR) * MAX_PATH);
    if (*ppwsz)
    {
        hres = S_FALSE;
        
        cch = SHGetIniString(c_szIntshcut,
            ISHCUT_INISTRING_WORKINGDIR,
            szPath, SIZECHARS(szPath), pszFile);
        if (0 != cch)
        {
            TCHAR szFullPath[MAX_PATH];
            PTSTR pszFileName;
            
            if (0 < GetFullPathName(szPath, SIZECHARS(szFullPath), szFullPath,
                &pszFileName))
            {
                SHTCharToUnicode(szFullPath, *ppwsz, MAX_PATH);
                
                hres = S_OK;
            }
        }
        
        if (S_OK != hres)
        {
            LocalFree(*ppwsz);
            *ppwsz = NULL;    
        }
    }
    
    return hres;
}


/*----------------------------------------------------------
Purpose: Write the working directory to the URL (ini) file.

*/
HRESULT 
WriteGenericString(
    IN LPCTSTR pszFile, 
    IN  LPCTSTR pszSectionName,
    IN LPCTSTR pszName,
    IN LPCWSTR pwsz)          OPTIONAL
{
    HRESULT hres = S_FALSE;
    
    ASSERT(IS_VALID_STRING_PTR(pszFile, -1));
    ASSERT(IS_VALID_STRING_PTR(pszName, -1));
    ASSERT(! pwsz || IS_VALID_STRING_PTRW(pwsz, -1));
    
    if (*pszFile)
    {
        if (AnyMeatW(pwsz))
        {
            hres = (SHSetIniString(pszSectionName, pszName, pwsz,
                pszFile)) ? S_OK : E_FAIL;
        }
        else
        {
            hres = (SHDeleteIniString(pszSectionName, pszName, pszFile))
                ? S_OK : E_FAIL;
        }
    }
    
    return hres;
}


/*----------------------------------------------------------
Purpose: Read the show-command flag from the URL (ini) file

*/
HRESULT 
ReadShowCmd(
    IN  LPCTSTR pszFile, 
    OUT PINT    pnShowCmd)
{
    HRESULT hres = S_FALSE;
    TCHAR szT[MAX_BUF_INT];
    DWORD cch;
    
    ASSERT(IS_VALID_STRING_PTR(pszFile, -1));
    ASSERT(IS_VALID_WRITE_PTR(pnShowCmd, INT));
    
    *pnShowCmd = SW_NORMAL;
    
    cch = GetPrivateProfileString(c_szIntshcut,
        TEXT("ShowCommand"), c_szNULL, szT,
        SIZECHARS(szT), pszFile);
    if (0 != cch)
    {
        if (StrToIntEx(szT, 0, pnShowCmd))
        {
            hres = S_OK;
        }
    }
    
    return hres;
}


/*----------------------------------------------------------
Purpose: Write showcmd to URL (ini) file

*/
HRESULT 
WriteShowCmd(
    IN LPCTSTR pszFile, 
    IN int     nShowCmd)
{
    HRESULT hres = S_FALSE;
    
    ASSERT(IS_VALID_STRING_PTR(pszFile, -1));
    
    if (*pszFile)
    {
        if (SW_NORMAL != nShowCmd)
        {
            hres = WriteSignedToFile(pszFile, c_szIntshcut, TEXT("ShowCommand"), nShowCmd);
        }
        else
        {
            hres = DeletePrivateProfileString(c_szIntshcut, TEXT("ShowCommand"), pszFile)
                ? S_OK
                : E_FAIL;
        }
    }
    
    return hres;
}



/*----------------------------------------------------------
Purpose: Read the IDList from the URL (ini) file

*/
HRESULT 
ReadIDList(
    IN  LPCTSTR pszFile, 
    OUT LPITEMIDLIST *ppidl)
{
    HRESULT hres = S_FALSE;
    ULONG cb;

    ASSERT(ppidl);

    // Delete the old one if any.
    if (*ppidl)
    {
        ILFree(*ppidl);
        *ppidl = NULL;
    }

    // Read the size of the IDLIST
    cb = GetPrivateProfileInt(c_szIntshcut, TEXT("ILSize"), 0, pszFile);
    if (cb)
    {
        // Create a IDLIST
        LPITEMIDLIST pidl = IEILCreate(cb);
        if (pidl)
        {
            // Read its contents
            if (GetPrivateProfileStruct(c_szIntshcut, TEXT("IDList"), (LPVOID)pidl, cb, pszFile))
            {
                *ppidl = pidl;
                hres = S_OK;
            }
            else
            {
                ILFree(pidl);
                hres = E_FAIL;
            }
        }
        else
        {
           hres = E_OUTOFMEMORY;
        }
    }
    
    return hres;
}

HRESULT
WriteStream(
    IN LPCTSTR pszFile, 
    IN IStream *pStream,
    IN LPCTSTR pszStreamName,
    IN LPCTSTR pszSizeName)
{
    HRESULT hr = E_FAIL;
    ULARGE_INTEGER li = {0};
    
    if(pStream)
        IStream_Size(pStream, &li);

    if (li.LowPart)
    {
        ASSERT(!li.HighPart);
        LPVOID pv = LocalAlloc(LPTR, li.LowPart);

        if (pv && SUCCEEDED(hr = IStream_Read(pStream, pv, li.LowPart)))
        {
            //  we have loaded the data properly, time to write it out

            if (SUCCEEDED(hr = WriteUnsignedToFile(pszFile, c_szIntshcut, pszSizeName, li.LowPart)))
                hr = WriteBinaryToFile(pszFile, c_szIntshcut, pszStreamName, pv, li.LowPart);
        }

        if (pv)
            LocalFree(pv);
    }
    else
    {
        // delete the keys if
        // 1. pStream is NULL, or
        // 2. pStream in empty (cbPidl == 0).
        if (DeletePrivateProfileString(c_szIntshcut, pszSizeName, pszFile) &&
            DeletePrivateProfileString(c_szIntshcut, pszStreamName, pszFile))
        {
            hr = S_OK;
        }
    }

    return hr;
}

/*----------------------------------------------------------
Purpose: Write IDList to URL (ini) file

*/
HRESULT 
WriteIDList(
    IN LPCTSTR pszFile, 
    IN IStream *pStream)
{
    return WriteStream(pszFile, pStream, TEXT("IDList"), TEXT("ILSize"));
}




/********************************** Methods **********************************/



//==========================================================================================
// URLProp class implementation 
//==========================================================================================


#ifdef DEBUG

/*----------------------------------------------------------
Purpose: Dump the properties in this object

*/
STDMETHODIMP_(void) URLProp::Dump(void)
{
    if (IsFlagSet(g_dwDumpFlags, DF_URLPROP))
    {
        PropStg_Dump(m_hstg, 0);
    }
}

#endif


/*----------------------------------------------------------
Purpose: Constructor for URLProp 

*/
URLProp::URLProp(void) : m_cRef(1)
{
    // Don't validate this until after construction.
    
    m_hstg = NULL;
    
    ASSERT(IS_VALID_STRUCT_PTR(this, CURLProp));
    
    return;
}


/*----------------------------------------------------------
Purpose: Destructor for URLProp

*/
URLProp::~URLProp(void)
{
    ASSERT(IS_VALID_STRUCT_PTR(this, CURLProp));
    
    if (m_hstg)
    {
        PropStg_Destroy(m_hstg);
        m_hstg = NULL;
    }
    
    ASSERT(IS_VALID_STRUCT_PTR(this, CURLProp));
    
    return;
}


STDMETHODIMP_(ULONG) URLProp::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) URLProp::Release()
{
    m_cRef--;
    if (m_cRef > 0)
        return m_cRef;
    
    delete this;
    return 0;
}

/*----------------------------------------------------------
Purpose: IUnknown::QueryInterface method for URLProp

*/
STDMETHODIMP URLProp::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IPropertyStorage))
    {
        *ppvObj = SAFECAST(this, IPropertyStorage *);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}

/*----------------------------------------------------------
Purpose: Initialize the object

Returns: S_OK
         E_OUTOFMEMORY
*/
STDMETHODIMP URLProp::Init(void)
{
    HRESULT hres = S_OK;
    
    // Don't stomp on ourselves if this has already been initialized 
    if (NULL == m_hstg)
    {
        hres = PropStg_Create(&m_hstg, PSTGF_DEFAULT);
    }
    
    return hres;
}


/*----------------------------------------------------------
Purpose: Helper function that retrieves the string property

*/
STDMETHODIMP
URLProp::GetProp(
    IN PROPID pid,
    IN LPTSTR pszBuf,
    IN int    cchBuf)
{
    HRESULT hres;
    PROPSPEC propspec;
    PROPVARIANT propvar;
    
    ASSERT(pszBuf);
    
    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = pid;
    
    *pszBuf = TEXT('\0');
    
    hres = ReadMultiple(1, &propspec, &propvar);
    if (SUCCEEDED(hres))
    {
        if (VT_LPWSTR == propvar.vt)
        {
            OleStrToStrN(pszBuf, cchBuf, propvar.pwszVal, -1);
            hres = S_OK;
        }
        else
        {
            if (VT_EMPTY != propvar.vt && VT_ILLEGAL != propvar.vt)
                TraceMsg(TF_WARNING, "URLProp::GetProp: expected propid %#lx to be VT_LPWSTR, but is %s", pid, Dbg_GetVTName(propvar.vt));
            hres = S_FALSE;
        }
        
        PropVariantClear(&propvar);
    }
    
    return hres;
}


/*----------------------------------------------------------
Purpose: Helper function that retrieves the word property

*/
STDMETHODIMP
URLProp::GetProp(
    IN PROPID pid,
    IN int * piVal)
{
    HRESULT hres;
    PROPSPEC propspec;
    PROPVARIANT propvar;
    
    ASSERT(piVal);
    
    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = pid;
    
    *piVal = 0;
    
    hres = ReadMultiple(1, &propspec, &propvar);
    if (SUCCEEDED(hres))
    {
        if (VT_I4 == propvar.vt)
        {
            *piVal = propvar.lVal;
            hres = S_OK;
        }
        else
        {
            if (VT_EMPTY != propvar.vt && VT_ILLEGAL != propvar.vt)
                TraceMsg(TF_WARNING, "URLProp::GetProp: expected propid %#lx to be VT_I4, but is %s", pid, Dbg_GetVTName(propvar.vt));
            hres = S_FALSE;
        }
        
        PropVariantClear(&propvar);
    }
    
    return hres;
}


/*----------------------------------------------------------
Purpose: Helper function that retrieves the word property

*/
STDMETHODIMP
URLProp::GetProp(
    IN PROPID pid,
    IN LPDWORD pdwVal)
{
    HRESULT hres;
    PROPSPEC propspec;
    PROPVARIANT propvar;
    
    ASSERT(pdwVal);
    
    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = pid;
    
    *pdwVal = 0;
    
    hres = ReadMultiple(1, &propspec, &propvar);
    if (SUCCEEDED(hres))
    {
        if (VT_UI4 == propvar.vt)
        {
            *pdwVal = propvar.ulVal;
            hres = S_OK;
        }
        else
        {
            if (VT_EMPTY != propvar.vt && VT_ILLEGAL != propvar.vt)
                TraceMsg(TF_WARNING, "URLProp::GetProp: expected propid %#lx to be VT_UI4, but is %s", pid, Dbg_GetVTName(propvar.vt));
            hres = S_FALSE;
        }
        
        PropVariantClear(&propvar);
    }
    
    return hres;
}


/*----------------------------------------------------------
Purpose: Helper function that retrieves the word property

*/
STDMETHODIMP
URLProp::GetProp(
    IN PROPID pid,
    IN WORD * pwVal)
{
    HRESULT hres;
    PROPSPEC propspec;
    PROPVARIANT propvar;
    
    ASSERT(pwVal);
    
    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = pid;
    
    *pwVal = 0;
    
    hres = ReadMultiple(1, &propspec, &propvar);
    if (SUCCEEDED(hres))
    {
        if (VT_UI2 == propvar.vt)
        {
            *pwVal = propvar.uiVal;
            hres = S_OK;
        }
        else
        {
            if (VT_EMPTY != propvar.vt && VT_ILLEGAL != propvar.vt)
                TraceMsg(TF_WARNING, "URLProp::GetProp: expected propid %#lx to be VT_UI2, but is %s", pid, Dbg_GetVTName(propvar.vt));
            hres = S_FALSE;
        }
        
        PropVariantClear(&propvar);
    }
    
    return hres;
}


/*----------------------------------------------------------
Purpose: Helper function that retrieves the IStream property

*/
STDMETHODIMP
URLProp::GetProp(
    IN PROPID pid,
    IN IStream **ppStream)
{
    HRESULT hres;
    PROPSPEC propspec;
    PROPVARIANT propvar;
    
    ASSERT(ppStream);
    
    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = pid;
    
    *ppStream = 0;
    
    hres = ReadMultiple(1, &propspec, &propvar);
    if (SUCCEEDED(hres))
    {
        if (VT_STREAM == propvar.vt)
        {
            *ppStream = propvar.pStream;
            hres = S_OK;
        }
        else
        {
            if (VT_EMPTY != propvar.vt && VT_ILLEGAL != propvar.vt && propvar.lVal != 0)
                TraceMsg(TF_WARNING, "URLProp::GetProp: expected propid %#lx to be VT_STREAM, but is %s", pid, Dbg_GetVTName(propvar.vt));
            hres = S_FALSE;
        }
        
        // Do not PropVariantClear(&propvar), because it will call pStream->Release().
    }
    
    return hres;
}


/*----------------------------------------------------------
Purpose: Helper function that sets the string property

*/
STDMETHODIMP
URLProp::SetProp(
    IN PROPID  pid,
    IN LPCTSTR psz)         OPTIONAL
{
    HRESULT hres;
    PROPSPEC propspec;
    PROPVARIANT propvar;

    // WARNING:: this function gets called as part of ShellExecute which can be
    // called by 16 bit apps so don't put mondo strings on stack...
    WCHAR *pwsz = NULL;
    
    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = pid;
    
    if (psz && *psz)
    {
        SHStrDup(psz, &pwsz);
        propvar.vt = VT_LPWSTR;
        propvar.pwszVal = pwsz;
    }
    else
        propvar.vt = VT_EMPTY;
    
    hres = WriteMultiple(1, &propspec, &propvar, 0);

    if (pwsz)
        CoTaskMemFree(pwsz);

    return hres;
}


/*----------------------------------------------------------
Purpose: Helper function that sets the int property

*/
STDMETHODIMP
URLProp::SetProp(
    IN PROPID  pid,
    IN int     iVal)
{
    PROPSPEC propspec;
    PROPVARIANT propvar;
    
    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = pid;
    
    propvar.vt = VT_I4;
    propvar.lVal = iVal;
    
    return WriteMultiple(1, &propspec, &propvar, 0);
}


/*----------------------------------------------------------
Purpose: Helper function that sets the dword property

*/
STDMETHODIMP
URLProp::SetProp(
    IN PROPID  pid,
    IN DWORD   dwVal)
{
    HRESULT hres;
    PROPSPEC propspec;
    PROPVARIANT propvar;
    
    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = pid;
    
    propvar.vt = VT_UI4;
    propvar.ulVal = dwVal;
    
    hres = WriteMultiple(1, &propspec, &propvar, 0);
    
    return hres;
}


/*----------------------------------------------------------
Purpose: Helper function that sets the word property

*/
STDMETHODIMP
URLProp::SetProp(
    IN PROPID  pid,
    IN WORD    wVal)
{
    HRESULT hres;
    PROPSPEC propspec;
    PROPVARIANT propvar;
    
    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = pid;
    
    propvar.vt = VT_UI2;
    propvar.uiVal = wVal;
    
    hres = WriteMultiple(1, &propspec, &propvar, 0);
    
    return hres;
}


/*----------------------------------------------------------
Purpose: Helper function that sets the IStream* property

*/
STDMETHODIMP
URLProp::SetProp(
    IN PROPID  pid,
    IN IStream *pStream)
{
    HRESULT hres;
    PROPSPEC propspec;
    PROPVARIANT propvar;
    
    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = pid;
    
    propvar.vt = VT_STREAM;
    propvar.pStream = pStream;
    
    hres = WriteMultiple(1, &propspec, &propvar, 0);
    
    return hres;
}


STDMETHODIMP URLProp::IsDirty(void)
{
    return PropStg_IsDirty(m_hstg);
}


STDMETHODIMP URLProp::ReadMultiple(IN ULONG         cpspec,
                                   IN const PROPSPEC rgpropspec[],
                                   IN PROPVARIANT   rgpropvar[])
{
    HRESULT hres = PropStg_ReadMultiple(m_hstg, cpspec, rgpropspec, rgpropvar);
    
    if (SUCCEEDED(hres))
    {
        // Set the accessed time
        SYSTEMTIME st;
        
        GetSystemTime(&st);
        SystemTimeToFileTime(&st, &m_ftAccessed);
    }
    
    return hres;
}


STDMETHODIMP URLProp::WriteMultiple(IN ULONG         cpspec,
                                    IN const PROPSPEC rgpropspec[],
                                    IN const PROPVARIANT rgpropvar[],
                                    IN PROPID        propidFirst)
{
    HRESULT hres = PropStg_WriteMultiple(m_hstg, cpspec, rgpropspec, 
        rgpropvar, propidFirst);
    
    if (SUCCEEDED(hres))
    {
        // Set the modified time
        SYSTEMTIME st;
        
        GetSystemTime(&st);
        SystemTimeToFileTime(&st, &m_ftModified);
    }
    
    return hres;
}

STDMETHODIMP URLProp::DeleteMultiple(ULONG cpspec, const PROPSPEC rgpropspec[])
{
    return PropStg_DeleteMultiple(m_hstg, cpspec, rgpropspec);
}


STDMETHODIMP URLProp::ReadPropertyNames(ULONG cpropid, const PROPID rgpropid[], LPWSTR rgpwszName[])
{
    return E_NOTIMPL;
}

STDMETHODIMP URLProp::WritePropertyNames(ULONG cpropid, const PROPID rgpropid[], const LPWSTR rgpwszName[])
{
    return E_NOTIMPL;
}


/*----------------------------------------------------------
Purpose: IPropertyStorage::DeletePropertyNames method for URLProp

*/
STDMETHODIMP
URLProp::DeletePropertyNames(
    IN ULONG    cpropid,
    IN const PROPID rgpropid[])
{
    return E_NOTIMPL;
}


/*----------------------------------------------------------
Purpose: IPropertyStorage::SetClass method for URLProp

*/
STDMETHODIMP
URLProp::SetClass(
    IN REFCLSID rclsid)
{
    CopyMemory(&m_clsid, &rclsid, SIZEOF(m_clsid));
    
    return S_OK;
}


/*----------------------------------------------------------
Purpose: IPropertyStorage::Commit method for URLProp

*/
STDMETHODIMP
URLProp::Commit(
    IN DWORD dwFlags)
{
    return E_NOTIMPL;
}


/*----------------------------------------------------------
Purpose: IPropertyStorage::Revert method for URLProp

*/
STDMETHODIMP URLProp::Revert(void)
{
#ifdef DEBUG
    Dump();
#endif
    return E_NOTIMPL;
}


/*----------------------------------------------------------
Purpose: IPropertyStorage::Enum method for URLProp

*/
STDMETHODIMP URLProp::Enum(IEnumSTATPROPSTG ** ppenum)
{
    *ppenum = NULL;
    return E_NOTIMPL;
}


/*----------------------------------------------------------
Purpose: IPropertyStorage::Stat method for URLProp

*/
STDMETHODIMP
URLProp::Stat(
    IN STATPROPSETSTG * pstat)
{
    HRESULT hres = STG_E_INVALIDPARAMETER;

    if (IS_VALID_WRITE_PTR(pstat, STATPROPSETSTG))
    {
        pstat->fmtid = m_fmtid;
        pstat->clsid = m_clsid;
        pstat->grfFlags = m_grfFlags;
        pstat->mtime = m_ftModified;
        pstat->ctime = m_ftCreated;
        pstat->atime = m_ftAccessed;

        hres = S_OK;
    }
    return hres;
}


/*----------------------------------------------------------
Purpose: IPropertyStorage::SetTimes method for URLProp

*/
STDMETHODIMP
URLProp::SetTimes(
    IN const FILETIME * pftModified,        OPTIONAL
    IN const FILETIME * pftCreated,         OPTIONAL
    IN const FILETIME * pftAccessed)        OPTIONAL
{
    HRESULT hres;
    
    if (pftModified && !IS_VALID_READ_PTR(pftModified, FILETIME) ||
        pftCreated && !IS_VALID_READ_PTR(pftCreated, FILETIME) ||
        pftAccessed && !IS_VALID_READ_PTR(pftAccessed, FILETIME))
    {
        hres = STG_E_INVALIDPARAMETER;
    }
    else
    {
        if (pftModified)
            m_ftModified = *pftModified;
        
        if (pftCreated)
            m_ftCreated = *pftCreated;
        
        if (pftAccessed)
            m_ftAccessed = *pftAccessed;
        
        hres = S_OK;
    }
    
    return hres;
}

#ifdef DEBUG

STDMETHODIMP_(void) IntshcutProp::Dump(void)
{
    if (IsFlagSet(g_dwDumpFlags, DF_URLPROP))
    {
        TraceMsg(TF_ALWAYS, "  IntshcutProp obj: %s", m_szFile);
        URLProp::Dump();
    }
}

#endif


IntshcutProp::IntshcutProp(void)
{
    // Don't validate this until after construction.
    
    *m_szFile = 0;
    
    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcutProp));
}

IntshcutProp::~IntshcutProp(void)
{
    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcutProp));

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcutProp));
}


// (These are not related to PID_IS_*) 
#define IPROP_ICONINDEX     0 
#define IPROP_ICONFILE      1
#define IPROP_HOTKEY        2 
#define IPROP_WORKINGDIR    3
#define IPROP_SHOWCMD       4
#define IPROP_WHATSNEW      5     
#define IPROP_AUTHOR        6 
#define IPROP_DESC          7 
#define IPROP_COMMENT       8
#define IPROP_URL           9       // these two must be the last 
#define IPROP_SCHEME        10      //  in this list.  See LoadFromFile.
#define CPROP_INTSHCUT      11      // Count of properties 

// (we don't write the URL or the scheme in the massive write sweep)
#define CPROP_INTSHCUT_WRITE    (CPROP_INTSHCUT - 2)      

/*----------------------------------------------------------
Purpose: Load the basic property info like URL.

Returns: 
Cond:    --
*/
STDMETHODIMP IntshcutProp::LoadFromFile(LPCTSTR pszFile)
{
    HRESULT hres;
    LPWSTR pwszBuf;
    LPTSTR pszBuf;
    CHAR *pszTempBuf;
    static const PROPSPEC rgpropspec[CPROP_INTSHCUT] = 
    {
        // This must be initialized in the same order as how the
        // IPROP_* values were defined.
        { PRSPEC_PROPID, PID_IS_ICONINDEX },
        { PRSPEC_PROPID, PID_IS_ICONFILE },
        { PRSPEC_PROPID, PID_IS_HOTKEY },
        { PRSPEC_PROPID, PID_IS_WORKINGDIR },
        { PRSPEC_PROPID, PID_IS_SHOWCMD },
        { PRSPEC_PROPID, PID_IS_WHATSNEW },
        { PRSPEC_PROPID, PID_IS_AUTHOR },
        { PRSPEC_PROPID, PID_IS_DESCRIPTION },
        { PRSPEC_PROPID, PID_IS_COMMENT },
        { PRSPEC_PROPID, PID_IS_URL },
        { PRSPEC_PROPID, PID_IS_SCHEME },
    };
    PROPVARIANT rgpropvar[CPROP_INTSHCUT] = { 0 };
    
    ASSERT(pszFile);

    // try to allocate a temporary buffer, don't put on stack as this may be called
    // by 16 bit apps through the shellexecute thunk
    pszTempBuf = (CHAR*)LocalAlloc(LMEM_FIXED, INTERNET_MAX_URL_LENGTH * sizeof(CHAR));
    if (!pszTempBuf)
        return E_OUTOFMEMORY;

    if (!g_fRunningOnNT)
    {
        // Flush the cache first to encourage Win95 kernel to zero-out
        // its buffer.  Kernel GP-faults with hundreds of writes made to
        // ini files.
        WritePrivateProfileString(NULL, NULL, NULL, pszFile);
    }
    
    // Get the URL 
    hres = ReadURLFromFile(pszFile, c_szIntshcut, &pszBuf);
    if (S_OK == hres)
    {
        // Call this method because it does more work before
        // setting the property 
        SetURLProp(pszBuf, (IURL_SETURL_FL_GUESS_PROTOCOL | IURL_SETURL_FL_USE_DEFAULT_PROTOCOL));
        
        LocalFree(pszBuf);
    }
    
    // Get the IDList
    LPITEMIDLIST pidl = NULL;
    hres = ReadIDList(pszFile, &pidl);
    if (S_OK == hres)
    {
        // Call this method because it does more work before
        // setting the property 
        SetIDListProp(pidl);
        
        ILFree(pidl);
    }

#ifndef UNIX

    // Get icon location
    int nVal;
    hres = ReadIconLocation(pszFile, &pwszBuf, &nVal, pszTempBuf);
    if (S_OK == hres)
    {
        rgpropvar[IPROP_ICONFILE].vt = VT_LPWSTR;
        rgpropvar[IPROP_ICONFILE].pwszVal = pwszBuf;
        
        rgpropvar[IPROP_ICONINDEX].vt = VT_I4;
        rgpropvar[IPROP_ICONINDEX].lVal = nVal;
    }
    
    // Get the hotkey 
    WORD wHotkey;
    hres = ReadHotkey(pszFile, &wHotkey);
    if (S_OK == hres)
    {
        rgpropvar[IPROP_HOTKEY].vt = VT_UI2;
        rgpropvar[IPROP_HOTKEY].uiVal = wHotkey;
    }
    
    // Get the working directory 
    hres = ReadWorkingDirectory(pszFile, &pwszBuf);
    if (S_OK == hres)
    {
        rgpropvar[IPROP_WORKINGDIR].vt = VT_LPWSTR;
        rgpropvar[IPROP_WORKINGDIR].pwszVal = pwszBuf;
    }
    
    // Get the showcmd flag 
    hres = ReadShowCmd(pszFile, &nVal);
    rgpropvar[IPROP_SHOWCMD].vt = VT_I4;
    if (S_OK == hres)
        rgpropvar[IPROP_SHOWCMD].lVal = nVal;
    else
        rgpropvar[IPROP_SHOWCMD].lVal = SW_NORMAL;
    
    
    // Get the What's New bulletin 
    hres = ReadStringFromFile(pszFile, c_szIntshcut, ISHCUT_INISTRING_WHATSNEW, &pwszBuf, pszTempBuf);
    if (S_OK == hres)
    {
        rgpropvar[IPROP_WHATSNEW].vt = VT_LPWSTR;
        rgpropvar[IPROP_WHATSNEW].pwszVal = pwszBuf;
    }
    
    // Get the Author 
    hres = ReadStringFromFile(pszFile, c_szIntshcut, ISHCUT_INISTRING_AUTHOR, &pwszBuf, pszTempBuf);
    if (S_OK == hres)
    {
        rgpropvar[IPROP_AUTHOR].vt = VT_LPWSTR;
        rgpropvar[IPROP_AUTHOR].pwszVal = pwszBuf;
    }
    
    // Get the Description 
    hres = ReadStringFromFile(pszFile, c_szIntshcut, ISHCUT_INISTRING_DESC, &pwszBuf, pszTempBuf);
    if (S_OK == hres)
    {
        rgpropvar[IPROP_DESC].vt = VT_LPWSTR;
        rgpropvar[IPROP_DESC].pwszVal = pwszBuf;
    }
    
    // Get the Comment
    hres = ReadStringFromFile(pszFile, c_szIntshcut, ISHCUT_INISTRING_COMMENT, &pwszBuf, pszTempBuf);
    if (S_OK == hres)
    {
        rgpropvar[IPROP_COMMENT].vt = VT_LPWSTR;
        rgpropvar[IPROP_COMMENT].pwszVal = pwszBuf;
    }

#endif /* !UNIX */
    
    // Write it all out to our in-memory storage.  Note we're using 
    // CPROP_INTSHCUT_WRITE, which should be the size of the array minus the
    // url and scheme propids, since they were written separately 
    // above.
    hres = WriteMultiple(CPROP_INTSHCUT_WRITE, (PROPSPEC *)rgpropspec, rgpropvar, 0);
    if (SUCCEEDED(hres))
    {
        // Unmark *all* these properties, since we're initializing from
        // the file
        PropStg_DirtyMultiple(m_hstg, ARRAYSIZE(rgpropspec), rgpropspec, FALSE);
    }
    
    // Get the times.  We don't support the Accessed time for internet
    // shortcuts updating this field would cause the shortcut to be
    // constantly written to disk to record the Accessed time simply
    // when a property is read.  A huge perf hit!

    ZeroMemory(&m_ftAccessed, sizeof(m_ftAccessed));
    
    DWORD cbData = SIZEOF(m_ftModified);
    ReadBinaryFromFile(pszFile, c_szIntshcut, ISHCUT_INISTRING_MODIFIED, &m_ftModified, cbData);
    
    // Free up the buffers that we allocated 
    int cprops;
    PROPVARIANT * ppropvar;
    for (cprops = ARRAYSIZE(rgpropvar), ppropvar = rgpropvar; 0 < cprops; cprops--)
    {
        if (VT_LPWSTR == ppropvar->vt)
        {
            ASSERT(ppropvar->pwszVal);
            LocalFree(ppropvar->pwszVal);
        }
        ppropvar++;
    }

    LocalFree((HLOCAL)pszTempBuf);
    
    return hres;
}

STDMETHODIMP IntshcutProp::Init(void)
{
    return URLProp::Init();
}

STDMETHODIMP IntshcutProp::InitFromFile(LPCTSTR pszFile)
{
    // Initialize the in-memory property storage from the file
    // and database
    HRESULT hres = Init();
    if (SUCCEEDED(hres) && pszFile)
    {
        StrCpyN(m_szFile, pszFile, SIZECHARS(m_szFile));
        hres = LoadFromFile(m_szFile);
    }
    else
        m_szFile[0] = 0;
    
    return hres;
}


typedef struct
{
    LPTSTR pszFile;
} COMMITISDATA;

/*----------------------------------------------------------
Purpose: Commit the values for any known properties to the file

         Note this callback is called only for dirty values.

Returns: S_OK if alright
         S_FALSE to skip this value
         error to stop
  
*/
STDAPI CommitISProp(
    IN PROPID        propid,
    IN PROPVARIANT * ppropvar,
    IN LPARAM        lParam)
{
    HRESULT hres = S_OK;
    COMMITISDATA * pcd = (COMMITISDATA *)lParam;
    
    ASSERT(ppropvar);
    ASSERT(pcd);
    
    LPWSTR pwsz;
    USHORT uiVal;
    LONG lVal;
    IStream *pStream;
    
    switch (propid)
    {
    case PID_IS_URL:
    case PID_IS_ICONFILE:
    case PID_IS_WORKINGDIR:
    case PID_IS_WHATSNEW:
    case PID_IS_AUTHOR:
    case PID_IS_DESCRIPTION:
    case PID_IS_COMMENT:
        if (VT_LPWSTR == ppropvar->vt)
            pwsz = ppropvar->pwszVal;
        else
            pwsz = NULL;
        
        switch (propid)
        {
        case PID_IS_URL:
            hres = WriteGenericString(pcd->pszFile, c_szIntshcut, ISHCUT_INISTRING_URL, pwsz);
            break;
            
        case PID_IS_ICONFILE:
            hres = WriteIconFile(pcd->pszFile, pwsz);
            break;
            
        case PID_IS_WORKINGDIR:
            hres = WriteGenericString(pcd->pszFile, c_szIntshcut, ISHCUT_INISTRING_WORKINGDIR, pwsz);
            break;
            
        case PID_IS_WHATSNEW:
            hres = WriteGenericString(pcd->pszFile, c_szIntshcut, ISHCUT_INISTRING_WHATSNEW, pwsz);
            break;
            
        case PID_IS_AUTHOR:
            hres = WriteGenericString(pcd->pszFile, c_szIntshcut, ISHCUT_INISTRING_AUTHOR, pwsz);
            break;
            
        case PID_IS_DESCRIPTION:
            hres = WriteGenericString(pcd->pszFile, c_szIntshcut, ISHCUT_INISTRING_DESC, pwsz);
            break;
            
        case PID_IS_COMMENT:
            hres = WriteGenericString(pcd->pszFile, c_szIntshcut, ISHCUT_INISTRING_COMMENT, pwsz);
            break;
            
        default:
            ASSERT(0);      // should never get here
            break;
        }
        break;
        
        case PID_IS_ICONINDEX:
            if (VT_I4 == ppropvar->vt)
                hres = WriteIconIndex(pcd->pszFile, ppropvar->lVal);
            break;
            
        case PID_IS_HOTKEY:
            if (VT_UI2 == ppropvar->vt)
                uiVal = ppropvar->uiVal;
            else
                uiVal = 0;
            
            hres = WriteHotkey(pcd->pszFile, uiVal);
            break;
            
        case PID_IS_SHOWCMD:
            if (VT_I4 == ppropvar->vt)
                lVal = ppropvar->lVal;
            else
                lVal = SW_NORMAL;
            
            hres = WriteShowCmd(pcd->pszFile, lVal);
            break;
            
        case PID_IS_SCHEME:
            // Don't write this one out
            break;
            
        case PID_IS_IDLIST:
            if (VT_STREAM == ppropvar->vt)
                pStream = ppropvar->pStream;
            else
                pStream = NULL;
                
            hres = WriteIDList(pcd->pszFile, pStream);
            break;
                  
                  
        default:
            TraceMsg(TF_WARNING, "Don't know how to commit url property (%#lx)", propid);
            ASSERT(0);
            break;
    }
    
#ifdef DEBUG
    if (FAILED(hres))
        TraceMsg(TF_WARNING, "Failed to save url property (%#lx) to file %s", propid, pcd->pszFile);
#endif
  
    return hres;
}


/*----------------------------------------------------------
Purpose: IPropertyStorage::Commit method for URLProp

*/
STDMETHODIMP
IntshcutProp::Commit(
    IN DWORD dwFlags)
{
    HRESULT hres;
    COMMITISDATA cd;
    
    TraceMsg(TF_INTSHCUT, "Writing properties to \"%s\"", m_szFile);

    cd.pszFile = m_szFile;
    
    // Enumerate thru the dirty property values that get saved to the
    // file
    hres = PropStg_Enum(m_hstg, PSTGEF_DIRTY, CommitISProp, (LPARAM)&cd);
    
    if (SUCCEEDED(hres))
    {
        // Now mark everything clean 
        PropStg_DirtyAll(m_hstg, FALSE);

        // Save the times.  Don't write out the Accessed time for perf.
        // See LoadFromFile.
        EVAL(SUCCEEDED(WriteBinaryToFile(m_szFile, c_szIntshcut, ISHCUT_INISTRING_MODIFIED, &m_ftModified, 
                                         SIZEOF(m_ftModified))));
    }
    
#ifdef DEBUG
    Dump();
#endif
    
    return hres;
}


/*----------------------------------------------------------
Purpose: Helper function to set the file name.

*/
STDMETHODIMP 
IntshcutProp::SetFileName(
    IN LPCTSTR pszFile)
{
    if(pszFile)
    {
        ASSERT(IS_VALID_STRING_PTR(pszFile, -1));
        StrCpyN(m_szFile, pszFile, SIZECHARS(m_szFile));
    }
    else
    {
        *m_szFile = TEXT('\0');;
    }

    return S_OK;
}



/*----------------------------------------------------------
Purpose: Helper function that sets the URL.

*/
STDMETHODIMP
IntshcutProp::SetIDListProp(
    LPCITEMIDLIST pcidl)
{
    HRESULT hres;
    IStream *pstmPidl;
    
    if (pcidl)
    {
        // ???
        // PERF: This loads OLE. Is this OK?
        
        hres = CreateStreamOnHGlobal(NULL, TRUE, &pstmPidl);
        if (SUCCEEDED(hres))
        {
            hres = ILSaveToStream(pstmPidl, pcidl);
            
            if (SUCCEEDED(hres))
                hres = SetProp(PID_IS_IDLIST, pstmPidl);

            pstmPidl->Release();
        }
    }
    else
    {
        hres = SetProp(PID_IS_IDLIST, NULL); 
    }
    
    return hres;
}


/*----------------------------------------------------------
Purpose: Helper function that sets the URL.  This function
         optionally canonicalizes the string as well.

*/
STDMETHODIMP
IntshcutProp::SetURLProp(
    IN LPCTSTR pszURL,              OPTIONAL
    IN DWORD   dwFlags)
{
    HRESULT hres;

    // Warning this function can be called as part of shellexecute which can be
    // thunked up to by a 16 bit app, so be carefull what you put on stack...
    
    BOOL bChanged;

    struct tbufs
    {
        TCHAR szUrl[INTERNET_MAX_URL_LENGTH];
        TCHAR szUrlT[INTERNET_MAX_URL_LENGTH];
    };

    struct tbufs *ptbufs;

    ptbufs = (struct tbufs *)LocalAlloc(LMEM_FIXED, sizeof(struct tbufs));
    if (!ptbufs)
        return E_OUTOFMEMORY;
    
    hres = GetProp(PID_IS_URL, ptbufs->szUrl, INTERNET_MAX_URL_LENGTH);
    
    bChanged = !(( !pszURL && S_OK != hres) ||
        (pszURL && S_OK == hres && 0 == StrCmp(pszURL, ptbufs->szUrl)));
    
    hres = S_OK;
    if (bChanged)
    {
        if (NULL == pszURL)
        {
            hres = SetProp(PID_IS_URL, pszURL);
            if (S_OK == hres)
                hres = SetProp(PID_IS_SCHEME, URL_SCHEME_UNKNOWN);
        }
        else
        {
            DWORD dwFlagsT = UQF_CANONICALIZE;
            
            // Translate the URL 
            
            if (IsFlagSet(dwFlags, IURL_SETURL_FL_GUESS_PROTOCOL))
                SetFlag(dwFlagsT, UQF_GUESS_PROTOCOL);
            
            if (IsFlagSet(dwFlags, IURL_SETURL_FL_USE_DEFAULT_PROTOCOL))
                SetFlag(dwFlagsT, UQF_USE_DEFAULT_PROTOCOL);
            
            // Translate the URL 
            hres = IURLQualify(pszURL, dwFlagsT, ptbufs->szUrlT, NULL, NULL);
            
            if (SUCCEEDED(hres))
            {
                // Is the URL different after being translated? 
                bChanged = (0 != StrCmp(ptbufs->szUrlT, ptbufs->szUrl));
                
                hres = S_OK;
                if (bChanged)
                {
                    // Yes; validate and get the scheme
                    PARSEDURL pu;
                    
                    pu.cbSize = SIZEOF(pu);
                    hres = ParseURL(ptbufs->szUrlT, &pu);
                    
                    if (S_OK == hres)
                        hres = SetProp(PID_IS_URL, ptbufs->szUrlT);
                    
                    if (S_OK == hres)
                        hres = SetProp(PID_IS_SCHEME, (DWORD)pu.nScheme);
                }
            }
        }
    }

    LocalFree((HLOCAL)ptbufs);
    
    return hres;
}

/*----------------------------------------------------------
Purpose: Helper function that sets the string property

*/
STDMETHODIMP
IntshcutProp::SetProp(
    IN PROPID  pid,
    IN LPCTSTR psz)         OPTIONAL
{
    HRESULT hr;

    // WARNING:: this function gets called as part of ShellExecute which can be
    // called by 16 bit apps so don't put mondo strings on stack...
    LPCWSTR pszUrl = psz;
    LPWSTR pszTemp = NULL;

    // For URLs, we need to check for security spoofs
    if (PID_IS_URL == pid && psz && IsSpecialUrl((LPWSTR)psz)) //bugbug: remove cast
    {
        SHStrDup(psz, &pszTemp);

        if (NULL != pszTemp)
        {
            // Unescape the url and look for a security context delimitor
            hr = WrapSpecialUrlFlat(pszTemp, lstrlen(pszTemp)+1);
            if (E_ACCESSDENIED == hr)
            {
                // Security delimitor found, so wack it off
                SHRemoveURLTurd(pszTemp);
                pszUrl = pszTemp;
            }
        }
        else
        {
            return E_OUTOFMEMORY;
        }
    }

    hr = super::SetProp(pid, pszUrl);

    if (pszTemp)
    {
        CoTaskMemFree(pszTemp);
    }
    return hr;
}

    
STDAPI CIntshcutProp_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvOut)
{
    HRESULT hres;
    
    *ppvOut = NULL;
    
    if (punkOuter)
    {
        // No
        hres = CLASS_E_NOAGGREGATION;
    }
    else
    {
        IUnknown * piunk = (IUnknown *)(IPropertyStorage *)new IntshcutProp;
        if ( !piunk ) 
        {
            hres = E_OUTOFMEMORY;
        }
        else
        {
            hres = piunk->QueryInterface(riid, ppvOut);
            piunk->Release();
        }
    }
    
    return hres;        // S_OK or E_NOINTERFACE
}
