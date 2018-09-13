#ifndef __cache_h
#define __cache_h

//
// Class cache exports
//

#define CLASSCACHE_PROPPAGES            0x00000001  // = 1 => get property page list
#define CLASSCACHE_CONTEXTMENUS         0x00000002  // = 1 => get context menu table
#define CLASSCACHE_ICONS                0x00000004  // = 1 => get icon locations
#define CLASSCACHE_CONTAINER            0x00000008  // = 1 => get container flag for class
#define CLASSCACHE_FRIENDLYNAME         0x00000010  // = 1 => get friendly name of the class
#define CLASSCACHE_ATTRIBUTENAMES       0x00000020  // = 1 => get attribute names of class
#define CLASSCACHE_TREATASLEAF          0x00000040  // = 1 => get the treat as leaf flags
#define CLASSCACHE_WIZARDDIALOG         0x00000080  // = 1 => get the creation wizard dialog CLSID
#define CLASSCACHE_WIZARDPRIMARYPAGE    0x00000100  // = 1 => get the creation wizard primary CLSID
#define CLASSCACHE_WIZARDEXTN           0x00000200  // = 1 => get the creation wizard extension CLSIDs
#define CLASSCACHE_CREATIONINFO         (CLASSCACHE_WIZARDDIALOG|CLASSCACHE_WIZARDPRIMARYPAGE|CLASSCACHE_WIZARDEXTN)

#define CLASSCACHE_IMAGEMASK            0x000f0000  // mask + shift for getting image mask
#define CLASSCACHE_IMAGEMASK_BIT        16      

#define CLASSCACHE_DSAVAILABLE          0x40000000  // = 1 => set GDSIF_DSAVAILABLE when calling GetDisplaySpecifierEx 
#define CLASSCACHE_SIMPLEAUTHENTICATE   0x80000000  // = 1 => perform simple authentication (eg ~ADS_SECURE_AUTHENTICATION)

typedef struct
{
    LPWSTR pPageReference;                          // page reference in strange URL form
} DSPROPERTYPAGE, * LPDSPROPERTYPAGE;

typedef struct
{
    LPWSTR pMenuReference;                          // menu reference stored in strange URL reference
} DSMENUHANDLER, * LPDSMENUHANDLER;

typedef struct
{
    LPWSTR pName;                                   // UNICODE atribute name
    LPWSTR pDisplayName;                            // UNICODE display name
    ADSTYPE dwADsType;                              // ADsType for the property
    DWORD dwFlags;                                  // flags for attribute (from display specifier)
} ATTRIBUTENAME, * LPATTRIBUTENAME;

typedef struct
{    
    HANDLE          hLock;                          // handle containing the lock value
    LPWSTR          pKey;                           // key string    
    DWORD           dwFlags;                        // cache entries we tried to cache
    DWORD           dwCached;                       // fields which have been cached
    LPWSTR          pObjectClass;                   // class name to find display specifier under    
    LPWSTR          pServer;                        // server name / ==  NULL if none specified
    LPWSTR          pFriendlyClassName;             // friendly class name
    HDSA            hdsaPropertyPages;              // property page list
    HDSA            hdsaMenuHandlers;               // list of menu handlers to bring in
    LPWSTR          pIconName[DSGIF_ISMASK];        // icon names for the various states
    BOOL            fIsContainer:1;                 // class is a conatiner
    BOOL            fTreatAsLeaf:1;                 // treat this class as a leaf 
    HDPA            hdpaAttributeNames;             // DPA containing the attribute names
    CLSID           clsidWizardDialog;              // CLSID of iface for creation dialog
    CLSID           clsidWizardPrimaryPage;         // CLSID of iface for wizards primary page
    HDSA            hdsaWizardExtn;                 // DSA containing wizard extension pages
} CLASSCACHEENTRY, * LPCLASSCACHEENTRY;

typedef struct
{
    DWORD        dwFlags;                           // Cache attributes interested in
    LPWSTR       pPath;                             // ADsPath of a reference object
    LPWSTR       pObjectClass;                      // Object Class of the class we need information on
    LPWSTR       pAttributePrefix;                  // Attribute prefixed used when reading display specifiers
    LPWSTR       pServer;                           // Server name == NULL then we assume this is serverless
    LPWSTR       pServerConfigPath;                 // Server config path == NULL then determine from the server name
    LPWSTR       pUserName;                         // pUserName / pPassword passed to ADsOpenObject as credential information
    LPWSTR       pPassword;
    LANGID       langid;                              // lang ID to be used
    IDataObject* pDataObject;                       // IDataObject containing extra information
} CLASSCACHEGETINFO, * LPCLASSCACHEGETINFO;


//
// Cache helper functions
//

INT _CompareAttributeNameCB(LPVOID p1, LPVOID p2, LPARAM lParam);

VOID ClassCache_Init(VOID);
HRESULT ClassCache_GetClassInfo(LPCLASSCACHEGETINFO pCacheInfo, LPCLASSCACHEENTRY* ppCacheEntry);
VOID ClassCache_ReleaseClassInfo(LPCLASSCACHEENTRY* ppCacheEntry);
VOID ClassCache_Discard(VOID);
ADSTYPE ClassCache_GetADsTypeFromAttribute(LPCLASSCACHEGETINFO pccgi, LPCWSTR pAttributeName);

HRESULT GetRootDSE(LPCWSTR pszUserName, LPCWSTR pszPassword, LPCWSTR pszServer, BOOL fSecure, IADs **ppads);
HRESULT GetCacheInfoRootDSE(LPCLASSCACHEGETINFO pccgi, IADs **ppads);
HRESULT GetDisplaySpecifier(LPCLASSCACHEGETINFO pccgi, REFIID riid, LPVOID* ppvObject);
HRESULT GetServerAndCredentails(LPCLASSCACHEGETINFO pccgi);
HRESULT GetAttributePrefix(LPWSTR* ppAttributePrefix, IDataObject* pDataObject);

BOOL _IsClassContainer(LPCLASSCACHEENTRY pClassCacheEntry, BOOL fIgnoreTreatAsLeaf);
HRESULT _GetIconLocation(LPCLASSCACHEENTRY pCacheEntry, DWORD dwFlags, LPWSTR pBuffer, INT cchBuffer, INT* piIndex);


#endif
