// 
// Pei-Hwa Lin (peiwhal), Feb 3, 1997
//
#include "msxml.h"

#ifndef TRACKCACHE_H_
#define TRACKCACHE_H_

#define MAX_CONTENT_LENGTH 2048

#define TRK_BOTH    0x00000000
#define TRK_OFFLINE 0x00000001
#define TRK_ONLINE  0x00000002

// default internet cache entry info size
#define MY_CACHE_FILE_ENTRY_SIZE     2048

#define MY_WEBCHECK_POST_REG \
        WEBCHECK_REGKEY TEXT("\\Log")

const TCHAR c_szEmptyLog[] = TEXT(" ");
#define c_ccEmptyLog (ARRAYSIZE(c_szEmptyLog) - 1)

LPTSTR ReadTrackingPrefix(void);

HRESULT CreateLogCacheEntry(LPCTSTR lpPfxUrl, FILETIME ftExpire, FILETIME ftModified, DWORD dwPurgeTime);

//////////////////////////////////////////////////////////////////////////
//
// Utility Class
//
//////////////////////////////////////////////////////////////////////////
class CChannelAgent;
class CTrackingCacheClassFactory;

class CUrlTrackingCache
{

protected:
    ULONG           m_cRef;         // OLE ref count

public:
     CUrlTrackingCache(ISubscriptionItem* pCDFStartItem,
                       LPCWSTR pwszURL);
    ~CUrlTrackingCache();

    HRESULT              OnProcessDone(void);
    HRESULT              ProcessTrackingInItem(IXMLElement *pTracking, LPCWSTR pwszUrl, BOOL fForceLog);
    HRESULT              ProcessTrackingInLog (IXMLElement *pTracking);

    LPCWSTR              get_PostURL(void) { return _pwszPostUrl; }

protected:
    HRESULT              RunChildElement(IXMLElement* pElement);
    HRESULT              ProcessItemInEnum(LPCWSTR pwszTagName, IXMLElement *pItem);
    HRESULT              ReadAttribute(IXMLElement* pItem, LPCWSTR pwszAttributeName, LPWSTR* pwszAttributeValue);
    //HRESULT              DoLogEventAttribute(IXMLElement* pItem, LPCWSTR pwszAttributeName);

    void                 Init(LPCWSTR pwszURL);

    void                 DoBaseURL(LPCWSTR pwszURL);
    BOOL                 IsValidURL(LPCTSTR lpszURL);             
    BOOL                 ConvertToPrefixedUrl(LPCTSTR lpszUrl, LPTSTR * lplpPrefixedUrl);
    LPINTERNET_CACHE_ENTRY_INFO  RetrieveUrlCacheEntry(LPCTSTR lpUrl);
    HRESULT              CreatePrefixedCacheEntry(LPCTSTR lpUrl);
    
    HRESULT              AddToTrackingCacheEntry(LPCWSTR pwszUrl);

    HRESULT              SchedulePostAgent(void);

private:
    // channel properties
    //data
    GROUPID                 _groupId;

    LPWSTR                  _pwszPostUrl;
    LPWSTR                  _pwszEncodingMethod;
    LPWSTR                  _pwszPurgeTime;
    BOOL                    _bTrackIt;

    LPTSTR                  _pszChannelUrlSite;
    LPTSTR                  _pszPostUrlSite;
    LPTSTR                  _lpPfx;
    DWORD                   _dwPurgeTime;            // in Day
    DWORD                   _ConnectionScope;

    ISubscriptionItem*      _pCDFStartItem;
};

#endif TRACKCACHE_H_
