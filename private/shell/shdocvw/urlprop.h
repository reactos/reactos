/*
 * urlprop.h - URL properties class implementation description.
 */

#ifndef _URLPROP_H_
#define _URLPROP_H_

#include "propstg.h"

#ifdef __cplusplus

// URL Property object

class URLProp : public IPropertyStorage
    {
private:
    ULONG       m_cRef;
    CLSID       m_clsid;
    FMTID       m_fmtid;
    DWORD       m_grfFlags;

    // private methods

protected:
    HPROPSTG    m_hstg;
    FILETIME    m_ftModified;
    FILETIME    m_ftCreated;
    FILETIME    m_ftAccessed;

public:
    URLProp(void);
    virtual ~URLProp(void);

    // IUnknown methods
    
    virtual STDMETHODIMP  QueryInterface(REFIID riid, PVOID *ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    
    // IPropertyStorage methods

    virtual STDMETHODIMP ReadMultiple(ULONG cpspec, const PROPSPEC rgpspec[], PROPVARIANT rgvar[]);
    virtual STDMETHODIMP WriteMultiple(ULONG cpspec, const PROPSPEC rgpspec[], const PROPVARIANT rgvar[], PROPID propidNameFirst);
    virtual STDMETHODIMP DeleteMultiple(ULONG cpspec, const PROPSPEC rgpspec[]);
    virtual STDMETHODIMP ReadPropertyNames(ULONG cpropid, const PROPID rgpropid[], LPOLESTR rglpwstrName[]);
    virtual STDMETHODIMP WritePropertyNames(ULONG cpropid, const PROPID rgpropid[], const LPOLESTR rglpwstrName[]);
    virtual STDMETHODIMP DeletePropertyNames(ULONG cpropid, const PROPID rgpropid[]);
    virtual STDMETHODIMP SetClass(REFCLSID clsid);
    virtual STDMETHODIMP Commit(DWORD grfCommitFlags);
    virtual STDMETHODIMP Revert(void);
    virtual STDMETHODIMP Enum(IEnumSTATPROPSTG** ppenm);
    virtual STDMETHODIMP Stat(STATPROPSETSTG* pstatpsstg);
    virtual STDMETHODIMP SetTimes(const FILETIME* pmtime, const FILETIME* pctime, const FILETIME* patime);

    // other methods
    
    virtual STDMETHODIMP Init(void);

    STDMETHODIMP GetProp(PROPID pid, LPTSTR pszBuf, int cchBuf);
    STDMETHODIMP GetProp(PROPID pid, int * piVal);
    STDMETHODIMP GetProp(PROPID pid, LPDWORD pdwVal);
    STDMETHODIMP GetProp(PROPID pid, WORD * pwVal);
    STDMETHODIMP GetProp(PROPID pid, IStream **ppStream);
    STDMETHODIMP SetProp(PROPID pid, LPCTSTR psz);
    STDMETHODIMP SetProp(PROPID pid, int iVal);
    STDMETHODIMP SetProp(PROPID pid, DWORD dwVal);
    STDMETHODIMP SetProp(PROPID pid, WORD wVal);
    STDMETHODIMP SetProp(PROPID pid, IStream *pStream);

    STDMETHODIMP IsDirty(void);

#ifdef DEBUG
    virtual STDMETHODIMP_(void) Dump(void);
    friend BOOL IsValidPCURLProp(const URLProp *pcurlprop);
#endif

    };

typedef URLProp * PURLProp;
typedef const URLProp CURLProp;
typedef const URLProp * PCURLProp;


// Internet Shortcut Property object

class IntshcutProp : public URLProp
    {

    typedef URLProp super;

private:
    TCHAR       m_szFile[MAX_PATH];

    // private methods

    STDMETHODIMP LoadFromFile(LPCTSTR pszFile);

public:
    IntshcutProp(void);
    ~IntshcutProp(void);

    // IPropertyStorage methods

    virtual STDMETHODIMP Commit(DWORD grfCommitFlags);

    // other methods
    
    STDMETHODIMP Init(void);
    STDMETHODIMP InitFromFile(LPCTSTR pszFile);

    STDMETHODIMP SetFileName(LPCTSTR pszFile);
    STDMETHODIMP SetURLProp(LPCTSTR pszURL, DWORD dwFlags);
    STDMETHODIMP SetIDListProp(LPCITEMIDLIST pcidl);

    STDMETHODIMP SetProp(PROPID pid, LPCTSTR psz);
    STDMETHODIMP SetProp(PROPID pid, int iVal)          { return super::SetProp(pid, iVal); }
    STDMETHODIMP SetProp(PROPID pid, DWORD dwVal)       { return super::SetProp(pid, dwVal); }
    STDMETHODIMP SetProp(PROPID pid, WORD wVal)         { return super::SetProp(pid, wVal); }
    STDMETHODIMP SetProp(PROPID pid, IStream *pStream)  { return super::SetProp(pid, pStream); }

#ifdef DEBUG
    virtual STDMETHODIMP_(void) Dump(void);
    friend BOOL IsValidPCIntshcutProp(const IntshcutProp *pcisprop);
#endif

    };

typedef IntshcutProp * PIntshcutProp;
typedef const IntshcutProp CIntshcutProp;
typedef const IntshcutProp * PCIntshcutProp;

class Intshcut;

// Internet Site Property object

class IntsiteProp : public URLProp
    {
private:
    TCHAR       m_szURL[INTERNET_MAX_URL_LENGTH];
    Intshcut *  m_pintshcut;
    BOOL        m_fPrivate;

    // private methods

    STDMETHODIMP LoadFromDB(LPCTSTR pszURL);

public:
    IntsiteProp(void);
    ~IntsiteProp(void);

    // IPropertyStorage methods

    virtual STDMETHODIMP Commit(DWORD grfCommitFlags);

    // other methods
    
    STDMETHODIMP Init(void);
    STDMETHODIMP InitFromDB(LPCTSTR pszURL, Intshcut * pintshcut, BOOL fPrivObj);

#ifdef DEBUG
    virtual STDMETHODIMP_(void) Dump(void);
    friend BOOL IsValidPCIntsiteProp(const IntsiteProp *pcisprop);
#endif

    };

typedef IntsiteProp * PIntsiteProp;
typedef const IntsiteProp CIntsiteProp;
typedef const IntsiteProp * PCIntsiteProp;


DWORD
SchemeTypeFromURL(
   LPCTSTR pszURL);

#endif  // __cplusplus


//
// Prototypes for all modules
//

#ifdef __cplusplus
extern "C" {
#endif

typedef const PARSEDURL CPARSEDURL;
typedef const PARSEDURL * PCPARSEDURL;

STDAPI
CIntshcutProp_CreateInstance(
   IN  LPUNKNOWN punkOuter, 
   IN  REFIID    riid, 
   OUT LPVOID *  ppvOut);

STDAPI
CIntsiteProp_CreateInstance(
   IN  LPUNKNOWN punkOuter, 
   IN  REFIID    riid, 
   OUT LPVOID *  ppvOut);


// Worker routines for updating the ini file corresponding to a shortcut

HRESULT 
ReadStringFromFile(IN  LPCTSTR    pszFile, 
                   IN  LPCTSTR    pszSectionName,
                   IN  LPCTSTR    pszName,
                   OUT LPWSTR *   ppwsz,
                   IN  CHAR *     pszBuf);

HRESULT 
ReadBStrFromFile(IN  LPCTSTR      pszFile, 
                 IN  LPCTSTR      pszSectionName,
                 IN  LPCTSTR      pszName,
                 OUT BSTR *       pBstr);

HRESULT
ReadUnsignedFromFile(
    IN  LPCTSTR pszFile,
    IN  LPCTSTR pszSectionName,
    IN  LPCTSTR pszName,
    OUT LPDWORD pdwVal);

HRESULT 
WriteGenericString(
    IN LPCTSTR pszFile, 
    IN LPCTSTR pszSectionName,
    IN LPCTSTR pszName,
    IN LPCWSTR pwsz);

HRESULT 
WriteSignedToFile(
    IN LPCTSTR  pszFile,
    IN LPCTSTR  pszSectionName,
    IN LPCTSTR  pszName,
    IN int      nVal);

HRESULT 
WriteUnsignedToFile(
    IN LPCTSTR  pszFile,
    IN LPCTSTR  pszSectionName,
    IN LPCTSTR  pszName,
    IN DWORD    nVal);

HRESULT 
ReadURLFromFile(
    IN  LPCTSTR  pszFile, 
    IN  LPCTSTR pszSectionName,
    OUT LPTSTR * ppsz);

HRESULT 
ReadBinaryFromFile(
   IN LPCTSTR pszFile,
   IN LPCTSTR pszSectionName,
   IN LPCTSTR pszName,
   IN LPVOID  pvData,
   IN DWORD   cbData);

HRESULT 
WriteBinaryToFile(
  IN LPCTSTR pszFile,
  IN  LPCTSTR pszSectionName,
  IN LPCTSTR pszName,
  IN LPVOID  pvData,
  IN DWORD   cbSize);

#define DeletePrivateProfileString(pszSection, pszKey, pszFile) \
WritePrivateProfileString(pszSection, pszKey, NULL, pszFile)
#define SHDeleteIniString(pszSection, pszKey, pszFile) \
           SHSetIniString(pszSection, pszKey, NULL, pszFile)
#ifdef __cplusplus
};  // extern "C"
#endif

#endif  // _URLPROP_H_
