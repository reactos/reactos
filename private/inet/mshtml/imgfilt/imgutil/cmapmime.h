HRESULT InitDefaultMappings();
void CleanupDefaultMappings();

class CMapStringToCLSID
{
public:
   CMapStringToCLSID();  // Default constructor
   ~CMapStringToCLSID();  // Destructor

   const CLSID& GetCLSID() const;
   DWORD GetMapMode() const;
   LPCTSTR GetString() const;
   HRESULT InitFromKey( HKEY hKey, LPCTSTR pszKeyName );
   void SetCLSID( REFGUID clsid );
   void SetMapMode( DWORD dwMapMode );
   HRESULT SetString( LPCTSTR pszString );

protected:
   char m_achSignature[4];

public:
   CMapStringToCLSID* m_pNext;

protected:
   LPTSTR m_pszString;
   CLSID m_clsid;
   DWORD m_dwMapMode;
};

class CMapMIMEToCLSID :
   public IMapMIMEToCLSID,
   public CComObjectRoot,
   public CComCoClass< CMapMIMEToCLSID, &CLSID_CoMapMIMEToCLSID >
{
public:
   CMapMIMEToCLSID();  // Default constructor
   ~CMapMIMEToCLSID();  // Destructor

   BEGIN_COM_MAP( CMapMIMEToCLSID )
      COM_INTERFACE_ENTRY( IMapMIMEToCLSID )
   END_COM_MAP()


   DECLARE_REGISTRY( CMapMIMEToCLSID, _T( "ImgUtil.CoMapMIMEToCLSID.1" ),
      _T( "ImgUtil.CoMapMIMEToCLSID" ), IDS_COMAPMIMETOCLSID_DESC,
      THREADFLAGS_BOTH )


//   DECLARE_NO_REGISTRY()

// IMapMIMEToCLSID
public:
   STDMETHOD( EnableDefaultMappings )( BOOL bEnable );
   STDMETHOD( MapMIMEToCLSID )( LPCOLESTR pszMIMEType, GUID* pCLSID );
   STDMETHOD( SetMapping )( LPCOLESTR pszMIMEType, DWORD dwMapMode, 
      REFGUID clsid );

protected:
   CMapStringToCLSID* AddMapping( LPCTSTR pszMIMEType );
   void DeleteMapping( LPCTSTR pszMIMEType );
   CMapStringToCLSID* FindMapping( LPCTSTR pszMIMEType );

protected:
   ULONG m_nMappings;
   BOOL m_bEnableDefaultMappings;
   CMapStringToCLSID* m_pMappings;
};
