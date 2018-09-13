/*++
Copyright (c) 1997  Microsoft Corporation

Module Name:  instcon.hxx

Abstract:

    Installed container class derived from URL_CONTAINER
    
Author:
    Adriaan Canter (adriaanc) 04-10-97
    
--*/
#ifndef _INSTCON_HXX
#define _INSTCON_HXX

class CInstCon : public URL_CONTAINER
{
private:

    CHAR    *_szPrefixMap;
    CHAR    *_szVolumeLabel;
    CHAR    *_szVolumeTitle;
    DWORD    _cbMaxFileSize;
	DWORD    _cbPrefixMap;

    DWORD    GetEntry(LPCSTR, LPCACHE_ENTRY_INFO, LPDWORD, DWORD);
    DWORD    GetEntryFromCD(LPCSTR, LPCACHE_ENTRY_INFO, LPDWORD);
    DWORD    AddEntryToIndex(LPCACHE_ENTRY_INFO, LPDWORD);
    VOID     MapUrlToAbsPath(LPSTR, LPSTR, LPDWORD);
    
public:

    CInstCon(LPSTR CacheName, LPSTR VolumeName, LPSTR VolumeTitle, 
             LPSTR CachePath, LPSTR CachePrefix, LPSTR PrefixMap, 
             LONGLONG CacheLimit, DWORD dwOptions);

    ~CInstCon();  
   
    // URL_CONTAINER virtual overrides
    // Note only public functions declared
    // virtual in URL_CONTAINER.
        
    DWORD AddUrl(AddUrlArg* args);              // Sets cache entry type to INSTALLED_CACHE_ENTRY.
                                                
    DWORD RetrieveUrl(                          // Calls GetEntry.
        LPCSTR  UrlName,
        LPCACHE_ENTRY_INFO EntryInfo,
        LPDWORD EntryInfoSize,
        DWORD dwLookupFlags,
        DWORD dwRetrievalFlags);
    
    DWORD GetUrlInfo(                           // Calls GetEntry, allows for zero buffer.
        LPCSTR UrlName,
        LPCACHE_ENTRY_INFO UrlInfo,
        LPDWORD UrlInfoLength,
        DWORD dwLookupFlags,
        DWORD dwEntryFlags);

    LPSTR GetPrefixMap();
    LPSTR GetVolumeLabel();
    LPSTR GetVolumeTitle();

};

#endif // _INSTCON_HXX
