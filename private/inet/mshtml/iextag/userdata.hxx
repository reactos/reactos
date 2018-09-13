//+==================================================================
//
//   UserData.h : declaration of the CPersistUserData Class
//
//+==================================================================

#ifndef __USERDATA_H_
#define __USERDATA_H_

#include "resource.h"     // main symbols

#define URLZONE_INVALID (0xFFFFFFFF)
// Forward declarations.
class CSiteTable;

//+==================================================================
//
//  Class:      CPersistData
//
//  Purpose:    A Trident side translation class that sits between the 
//          users of the persistence model OM and the shell (which provides
//          the back end support and storage objects.
//
//===================================================================

class ATL_NO_VTABLE CPersistUserData : 
        public CComObjectRootEx<CComSingleThreadModel>,
        public CComCoClass<CPersistUserData, &CLSID_CPersistUserData>,
        public IDispatchImpl<IHTMLUserDataOM, &IID_IHTMLUserDataOM, &LIBID_IEXTagLib>,
        public IElementBehavior
{
public:
    CPersistUserData() 
     {
         _pPeerSite = NULL;
         _pPeerSite = NULL;
         _pPeerSiteOM = NULL;
         _pInnerXMLDoc = NULL;
         _pRoot = NULL;
         _ftExpires.dwLowDateTime = 0;
         _ftExpires.dwHighDateTime = 0;
         _fCoCreateTried = false;  
         _dwZone = URLZONE_INVALID;
     }
    ~CPersistUserData();

    DECLARE_REGISTRY_RESOURCEID(IDR_CPERSISTUSERDATA)
    DECLARE_NOT_AGGREGATABLE(CPersistUserData)

    BEGIN_COM_MAP(CPersistUserData)
        COM_INTERFACE_ENTRY(IHTMLUserDataOM)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IElementBehavior)
    END_COM_MAP()

public:
    //
    // IElementBehavior
    //---------------------------------------------
	STDMETHOD(Init)(IElementBehaviorSite *pPeerSite);
    STDMETHOD(Notify)(LONG, VARIANT *);
    STDMETHOD(Detach)() { return S_OK; };


    //IHTMLUserData methods
    //----------------------------------------
    STDMETHOD(get_XMLDocument)(IDispatch **ppDisp);
    STDMETHOD(save)(BSTR strName);
    STDMETHOD(load)(BSTR strName);
    STDMETHOD(getAttribute)(BSTR strAttr, VARIANT * pvar);
    STDMETHOD(setAttribute)(BSTR strAttr, VARIANT var);
    STDMETHOD(removeAttribute)(BSTR strAttr);
    STDMETHOD(put_expires)(BSTR strExpires);
    STDMETHOD(get_expires)(BSTR * pstrEpires);

    static BOOL  GlobalInit();
    static BOOL GlobalUninit();

private:
    // Helper methods
    //----------------------------------------------
    HRESULT initXMLCache(BOOL fResetOnly = false);
    void    ClearOMInterfaces();
    HRESULT GetDirPath(LPOLESTR * ppDirPath);
    BOOL    IsSchemeAllowed(LPCOLESTR pDirPath);
    BOOL    SecureDomainAndPath( LPOLESTR pstrFileName,
                                 LPOLESTR *ppstrStore,
                                 LPOLESTR *ppstrPath,
                                 LPOLESTR *ppstrName);
    void SetZone(DWORD dwZone) { _dwZone = dwZone; }
    DWORD GetZone() const { return _dwZone; }

    // Helper methods to save data to the wininet store.
    HRESULT LoadUserData(LPCOLESTR pwzDirPath, LPCOLESTR pwzName, LPVOID *ppData, LPDWORD pdwSize, DWORD dwFlags);
    HRESULT SaveUserData(LPCOLESTR pwzDirPath, LPCOLESTR pwzName, LPVOID pData, DWORD dwSize, FILETIME ftExpire, DWORD dwFlags);
    HRESULT EnsureCacheContainer() ;
    static HRESULT CreateSiteTable();
    static HRESULT DeleteSiteTable();

    static HRESULT ModifySitesSpaceUsage(LPCTSTR pszSite, int iSizeDelta);
    HRESULT EnforceStorageLimits(LPCOLESTR pwzDirPath, LPCTSTR pszCacheUrl,
                    DWORD dwSize, DWORD *pdwOldSize, BOOL bModify);

    static HRESULT GetCacheUrlFromDirPath(LPCOLESTR pwzDirPath, LPCOLESTR pwzFileName, LPTSTR * ppszCacheUrl);
    static BOOL    GetCacheUrlPrefix(LPCTSTR *ppszCacheUrlPrefix, DWORD *pdcchCacheUrlPrefix);
    static HRESULT GetSiteFromCacheUrl(LPCTSTR pszCacheUrl, LPTSTR *pszSite, DWORD *pcch);

    static TCHAR s_rgchUserData[];
    static DWORD s_rgdwDomainLimit[URLZONE_UNTRUSTED + 1];
    static DWORD s_dwUnkZoneDomainLimit;
    static DWORD s_rgdwDocLimit[URLZONE_UNTRUSTED + 1];
    static DWORD s_dwUnkZoneDocLimit;

    static TCHAR s_rgchCacheUrlPrefix[];    // What is the upper limit for user name??
    static DWORD s_cchCacheUrlPrefix;
    static BOOL  s_bCheckedUserName;
    static BOOL  s_bCacheUrlPrefixRet;

    static HRESULT s_hrCacheContainer;
    static BOOL  s_bCheckedCacheContainer;

    static CRITICAL_SECTION s_csCacheUrlPrefix;
    static CRITICAL_SECTION s_csCacheContainer;
    static CRITICAL_SECTION s_csSiteTable;

    static CSiteTable *s_pSiteTable;    // Used to remember the amount of memory each site has 
                                        // allocated already.

    // Helper function to figure out actual disk usage.
    static DWORD      RealDiskUsage(DWORD dwFileSize)
    {   return ((dwFileSize + s_dwClusterSizeMinusOne) & s_dwClusterSizeMask); }
    
    static DWORD      s_dwClusterSizeMinusOne;
    static DWORD      s_dwClusterSizeMask;

    typedef enum 
    {
        UDF_DEFAULT      =      0x00000000,
        UDF_NOLIMITS     =      0x00000001,          // No limit checks.
        UDF_BSTR      =      0x00000002,
    } UDF_FLAGS;       
    
    //
    // Member data
    //----------------------------------------------
    IElementBehaviorSite   * _pPeerSite;
    IElementBehaviorSiteOM * _pPeerSiteOM;

    IXMLDOMDocument        * _pInnerXMLDoc;  // XML object
    IXMLDOMElement         * _pRoot;         // root of xml documnt
    FILETIME                 _ftExpires;     // the expirey time for this 
    BOOL                     _fCoCreateTried;
    DWORD                    _dwZone;
};

// Helper class used to keep the hash table of string
// corresponding to the site and the total diskspace allocated
// so far. This is essentially a self-growing hash table. 

class CSiteTable 
{
protected:
    // Each individual entry in the hash table is stored in a CAssoc
    struct CAssoc 
    {
        CAssoc *pNext;
        LPTSTR pStr;
        DWORD   dwValue;
    };

public:
    // Default # of entries in the hashtable.
    // Large enough so we don't have to do grow the hash table
    // size often.
    CSiteTable(int nTableSize = 128);
    ~CSiteTable( );

    // Attributes.
    int GetCount() const { return m_nCount; };
    int IsEmpty() const  { return m_nCount == 0 ; }

    // Lookup
    BOOL Lookup(LPCTSTR key, DWORD& rValue) const;

    // Operations,
    HRESULT SetAt(LPCTSTR key, DWORD dwValue);

    // remove existing (key, ?) pair, 
    // Not implemented since we don't need it yet. Should be easy to add though.
    //  BOOL RemoveKey(LPCTSTR key); 

    void RemoveAll();

protected:
    // Overridable - in case a derived class wants to
    //. change the hash function..
    UINT HashKey(LPCTSTR key) const;

    HRESULT InitHashTable();
    HRESULT ChangeHashTableSize(int nTableSize);

    CAssoc ** m_pHashTable;
    UINT m_nHashTableSize;
    UINT m_nCount;

    CAssoc * GetAssocAt(LPCTSTR, UINT&) const;
};

#endif // __userdata_h_