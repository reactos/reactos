#define COLLECT_STATISTICS  0          // Set to 1 to enable statics gathering

extern const char *rgszCcssString[];

#define CCSSF_NONE                 0    // no flags
#define CCSSF_CLEARCACHES          1    // clear the caches when changing
#define CCSSF_REMEASURECONTENTS    2    // need to resize site after changing this
#define CCSSF_SIZECHANGED          4    // notify the parent of a size change.
#define CCSSF_REMEASUREINPARENT    8    // remeasure in parent layout's context
#define CCSSF_CLEARFF              16   // clear the caches when changing
#define CCSSF_REMEASUREALLCONTENTS 32   // resize self & all nested layout elements

struct CCachedAttrArrayInfo
{
    char *szDispId;
    DWORD dwFlags;
};

enum StorageType
{
    STORAGETYPE_OTHER,
    STORAGETYPE_NUMBER,
    STORAGETYPE_STRING,
};


struct AssociateDataType
{
    char *szKey;
    char *szValue;
    char *szMethodFnPrefix;
    StorageType stStorageType;
};

struct Associate
{
    char *szKey;
    char *szValue;
};


// Maximum allowed length of a parse line
#define MAX_LINE_LEN 2048

// Max number of tags per token
#define MAX_TAGS 63
#define MAX_SUBTOKENS 8

class CString
{
    char szString [ MAX_LINE_LEN+1 ];
public:
    CString() { szString [ 0 ] = '\0'; }
    BOOL operator== ( LPCSTR szStr1 )
    {
        return _stricmp ( (const char *)szString, (const char *)szStr1 ) == 0 ? TRUE : FALSE;
    }
#ifndef UNIX
    static BOOL operator== ( const CString& szStr1, LPCSTR szStr2 )
    {
        return _stricmp ( (LPCSTR)szStr1, (LPCSTR)szStr2 ) == 0 ? TRUE : FALSE;
    }
    static BOOL operator== ( LPCSTR szStr1, const CString &szStr2 )
    {
        return _stricmp ( (LPCSTR)szStr1, (LPCSTR)szStr2 ) == 0 ? TRUE : FALSE;
    }
#endif
    BOOL operator!= ( LPCSTR szStr1 )
    {
        return _stricmp ( (const char *)szString, (const char *)szStr1 ) == 0 ? FALSE : TRUE;
    }
#ifndef UNIX
    static BOOL operator!= ( const CString& szStr1, LPCSTR szStr2 )
    {
        return _stricmp ( (LPCSTR)szStr1, (LPCSTR)szStr2 ) == 0 ? FALSE : TRUE;
    }
    static BOOL operator!= ( LPCSTR szStr1, const CString &szStr2 )
    {
        return _stricmp ( (LPCSTR)szStr1, (LPCSTR)szStr2 ) == 0 ? FALSE : TRUE;
    }
#endif 
    const CString &operator= ( LPCSTR pStr )
    {
        strcpy ( szString, pStr );
        return *this;
    }
    const CString &operator+= ( LPCSTR pStr )
    {
        strcat ( szString, pStr );
        return *this;
    }
    const CString &operator+ ( CString szStr )
    {
        strcat ( szString, (LPCSTR)szStr );
        return *this;
    }
    const CString &operator+ ( LPCSTR pStr )
    {
        strcat ( szString, pStr );
        return *this;
    }
    char operator[] ( INT nIndex )
    {
        return szString [ nIndex ];
    }
    operator LPCSTR () const
    {
        return (const char *)szString;
    }
    CString &ToUpper ()
    {
        _strupr ( szString );
        return *this;
    }
    int Length ( void )
    {
        return strlen ( szString );
    }

    char * FindChar ( char ch )
    {
        return strchr(szString, ch);
    }
    
    UINT Lookup ( Associate *pArray, LPCSTR pStr );
    UINT Lookup ( AssociateDataType *pArray, LPCSTR pStr );

};

#ifdef UNIX
    inline BOOL operator== ( const CString& szStr1, LPCSTR szStr2 )
    {
        return _stricmp ( (LPCSTR)szStr1, (LPCSTR)szStr2 ) == 0 ? TRUE : FALSE;
    }
    inline BOOL operator== ( LPCSTR szStr1, const CString &szStr2 )
    {
        return _stricmp ( (LPCSTR)szStr1, (LPCSTR)szStr2 ) == 0 ? TRUE : FALSE;
    }
    inline BOOL operator== ( const CString& szStr1, const CString& szStr2)
    {
        return _stricmp( (LPCSTR)szStr1, (LPCSTR)szStr2 ) == 0 ? TRUE : FALSE;
    }
    inline BOOL operator!= ( const CString& szStr1, LPCSTR szStr2 )
    {
        return _stricmp ( (LPCSTR)szStr1, (LPCSTR)szStr2 ) == 0 ? FALSE : TRUE;
    }
#if 0 // apogee can't tell the difference between these two.
    inline BOOL operator!= ( LPCSTR szStr1, const CString &szStr2 )
    {
        return _stricmp ( (LPCSTR)szStr1, (LPCSTR)szStr2 ) == 0 ? FALSE : TRUE;
    }
#endif
#endif
struct TagDescriptor
{
    char *szTag;
    BOOL fIsFlag;
    BOOL fIsRequired;
    BOOL fRefersToClass;
};

// This enum must match the order that the corresponding structs appear in AllDescriptors
enum DESCRIPTOR_TYPE;

class TagArray
{
    char *szValues [ MAX_TAGS ];
    static char *szEmptyString;
public:
    BOOL AddTag ( int iTag, LPCSTR szStr, int nLen );
    TagArray()
    {
        INT i;
        for ( i = 0 ; i < MAX_TAGS ; i++ )
        {
            szValues [ i ] = NULL;
        }
    }
    ~TagArray()
    {
        INT i;
        for ( i = 0 ; i < MAX_TAGS ; i++ )
        {
            if ( szValues [ i ] )
                delete szValues [ i ];
        }
    }
    BOOL CompareTag ( INT nIndex, char *szWith );

    char *GetTagValue ( INT nIndex );

    BOOL IsSet ( INT nIndex )
    {
        return szValues [ nIndex ] == NULL ? FALSE : TRUE;
    }
    // Only use this method if you must - use GetTagValue normaly
    char *GetInternalValue ( INT nIndex )
    {
        return szValues [ nIndex ];
    }
};



struct TokenDescriptor;

class CTokenList;

class Token
{
    friend class CRuntimeTokenList;
    friend class CPDLParser;
    friend class CTokenList;
    friend class CTokenListWalker;
protected:
    DESCRIPTOR_TYPE nType;
    // Calculate bitwise enum vaklidation mask
    UINT uEnumMask;
    UINT nNextEnumValue;
    //
    Token *_pNextToken;
    CTokenList *_pChildList;
public:
    Token ( DESCRIPTOR_TYPE nDescriptorType )
    {
        nType = nDescriptorType;
        uEnumMask = 0; nNextEnumValue = 0;
        _pNextToken = NULL;
        _pChildList = NULL;
    }
    void Clone ( Token *pFrom );
    ~Token();
    TagArray TagValues; // Array of values size == nTag
    BOOL CompareTag ( INT nIndex, char *szWith )
    {
        return TagValues.CompareTag ( nIndex, szWith );
    }
    char *GetTagValue ( INT nIndex )
    {
        return TagValues.GetTagValue ( nIndex );
    }
    BOOL AddTag ( int iTag, LPCSTR szStr, int nLen = -1 )
    {
        if ( nLen == -1 )
            nLen = strlen ( szStr );
        return TagValues.AddTag ( iTag, szStr, nLen );
    }    
    BOOL IsSet ( INT nIndex )
    {
        return TagValues.IsSet ( nIndex );
    }
    // Sets a flag value to TRUE
    BOOL Set ( INT nIndex )
    {
        return TagValues.AddTag ( nIndex, "", 1 );
    }
    void AddParam ( CString &szArg, INT nTag, LPCSTR szText );
    void AddParamStr ( CString &szArg, INT nTag, LPCSTR szText );
    void GetTagValueOrDefault ( CString &szArg, INT nTag,
        LPCSTR szDefault )
    {
        if ( IsSet ( nTag ) )
            szArg = (LPCSTR)GetTagValue ( nTag );
        else
            szArg = (LPCSTR)szDefault;
    }
    DESCRIPTOR_TYPE GetType ( void ) const { return nType; }
    void SetNextToken ( Token *pToken ) { _pNextToken = pToken; }
    Token *GetNextToken ( void ) const { return _pNextToken; }
    Token *AddChildToken ( DESCRIPTOR_TYPE nType );
    void CalculateEnumMask ( Token *pParentToken );
    UINT GetChildTokenCount ( void );
}; 


struct TokenDescriptor
{
    char *szTokenName;
    BOOL fIsParentToken;
    TagDescriptor Tags [MAX_TAGS];
    //
    BOOL LookupTagName ( char *szTag, int nTagLen, INT *pnIndex )
    {
        for ( *pnIndex = 0 ; Tags [ *pnIndex ].szTag != NULL ; (*pnIndex)++ )
        {
            if ( _strnicmp ( Tags [ *pnIndex ].szTag, szTag, nTagLen ) == 0 )
            {
                // Descriptors never have an entry for the "name", so
                // the actual tag array index is always 1 more than the
                // corresponding descriptor index
                (*pnIndex)++;
                return TRUE;
            }
        }
        return FALSE;
    }
};

class CTokenList
{
private:
    // Linked list of all tokens
    Token *_pFirstToken; 
    // Pointer to last item in token list
    Token *_pLastToken;
    // Count of number of tokens in list
    UINT _uTokens;
    ULONG ulRefs;
    // protect out destructor so people don't delete us, use Release() instead
    ~CTokenList();
public:
    CTokenList()
    {
        _pFirstToken = NULL;
        _pLastToken = NULL;
        _uTokens = 0;
        ulRefs = 1;
    }
    Token *AddNewToken ( DESCRIPTOR_TYPE nTokenDescriptor );
    Token *FindToken  ( char *pTokenName, DESCRIPTOR_TYPE nType ) const;
    Token *GetFirst ( void ) const { return _pFirstToken; } 
    UINT GetTokenCount ( void ) const { return _uTokens; } 
    // We reference count to simplify Cloning of arg lists
    // for ref properties
    void AddRef ( void ) { ulRefs++; };
    void Release ( void ) {  if ( --ulRefs == 0 ) delete this; } 
};



class CTokenListWalker
{
private:
    CTokenList *_pList;
    Token *_pCurrentToken;
    char *_pszFileName;
    BOOL _fInCurrentFile;
    BOOL _fAtEnd;
private:
    Token *GetNextToken ( void )
    {
        if ( _fAtEnd || _pList == NULL )
        {
            return NULL;
        }
        if ( _pCurrentToken == NULL )
            _pCurrentToken = _pList -> GetFirst ();
        else
        {
            _pCurrentToken = _pCurrentToken -> GetNextToken();
            if ( _pCurrentToken == NULL )
                _fAtEnd = TRUE;
        }
        return _pCurrentToken;
    }
public:
    Token *CurrentToken ( void )
    {
        return _pCurrentToken;
    }
    void Reset ( void );

    // Generic walker
    CTokenListWalker ( CTokenList *pList )
    {
        _pList = pList;
        _pszFileName = NULL;
        Reset();
    }
    // Walker that will just walk definitions in a given pdl file
    CTokenListWalker ( CTokenList *pList, char *pszFileName )
    {
        _pList = pList;
        _pszFileName = pszFileName;
        Reset();
    }
    // Child token list walker
    CTokenListWalker ( Token *pParentToken )
    {
        if ( pParentToken )
            _pList = pParentToken -> _pChildList;
        else
            _pList = NULL;
        _pszFileName = NULL;
        Reset();
    }
    Token *GetNext ( void );
    Token *GetNext ( DESCRIPTOR_TYPE Type, LPCSTR pName = NULL );
    UINT GetTokenCount ( void )
    {
        return _pList ? _pList -> GetTokenCount() : 0;
    }

    BOOL IsGenericWalker ( void )
    {
        return _pszFileName == 0;
    }
};

struct PropdescInfo{
    LPCSTR _szClass;
    LPCSTR _szPropName;
    BOOL   _fAppendA;
    BOOL   _fProperty;      // TRUE if property FALSE if method
    UINT   _uVTblIndex;     // VTable Index of method or first Property (Get or Set)
    LPCSTR _szAttrName;
    LPCSTR _szSortKey;

    void Set(LPCSTR szClass, LPCSTR szPropName, BOOL fAppendA, LPCSTR szAttrName, UINT uVTblIndex = 0, BOOL fProperty = TRUE)
    {
        _szClass    = szClass;
        _szPropName = szPropName;
        _fAppendA   = fAppendA;
        _szAttrName = szAttrName;
        // If szAttribute specified, use that to override property name for sorting.
        _szSortKey  = strlen(szAttrName) ? szAttrName : szPropName;
        _uVTblIndex = uVTblIndex;
        _fProperty = fProperty;
    }

    void SetVTable(LPCSTR szClass, LPCSTR szPropName, BOOL fAppendA, LPCSTR szAttrName, UINT uVTblIndex = 0, BOOL fProperty = TRUE)
    {
        _szClass    = szClass;
        _szPropName = szPropName;
        _fAppendA   = fAppendA;
        _szAttrName = szAttrName;
        // If szPropName use that it's the exported name else the szAttrName is the usd.
        _szSortKey  = strlen(szPropName) ? szPropName : szAttrName;
        _uVTblIndex = uVTblIndex;
        _fProperty = fProperty;
    }
};

#define MAX_ARGS    8       // Maximum # of args (w/ default parameters PDL handles)

class CPDLParser
{
private:
    char *_pszPDLFileName;
    char *_pszOutputFileRoot;
    char *_pszInputFile;
    char *_pszOutputPath;
    CTokenList *pRuntimeList;

    FILE *fpHDLFile;
    FILE *fpIDLFile;
    FILE *fpHeaderFile;
    FILE *fpLOGFile;
    FILE *fpHTMFile;
    FILE *fpHTMIndexFile;
    FILE *fpDISPIDFile;
    FILE *fpMaxLenFile;

    int   cOnFileSignatures;
    int   cOnFileIIDs;
    char **rgszSignatures;
    char **rgszIIDs;
    int   cSignatures;
    int   cIIDs;
    int   _numPropDescs;
    int   _numVTblEntries;

    void RemoveSignatures ( void );
    BOOL LoadSignatures ( char *pszOutputPath );
    BOOL SaveSignatures ( char *pszOutputPath );
    BOOL FindAndAddSignature ( LPCSTR szType, LPCSTR szSignature, LPSTR pszInvokeMethod );
    int FindAndAddIIDs ( CString szInterface );

#if COLLECT_STATISTICS==1
    #define MAX_STATS   20
    #define NUM_PROPTURDS       0           // Number of code turds for all properties
    #define NUM_EMUMTURDS       1           // Number of code turds for enum properties
    #define NUM_GETENUMS        2           // Number of unique enum property gets per interface
    #define NUM_SETENUMS        3           // Number of unique enum property gets per interface
    #define NUM_GETPROPERTY     4           // Number of unique property sets per interface
    #define NUM_SETPROPERTY     5           // Number of unique property sets per interface

    long    rgcStats[MAX_STATS];

    void LoadStatistics ( char *pszOutputPath );
    void SaveStatistics ( char *pszOutputPath );

    void CollectStatistic (long lStatisticIdx, long lValue)
        { rgcStats[lStatisticIdx] = lValue; }
    long GetStatistic (long lStatisticIdx)
        { return rgcStats[lStatisticIdx]; }
#endif

    // Used to use this fn, leaving it for posterity
    UINT CountTags ( TokenDescriptor *tokdesc );
    //

    Token * IsSuperInterface( CString szSuper, Token * pInterface );
    Token * FindMatchingEntryWOPropDesc(Token *pClass, Token *pToFindToken, BOOL fNameMatchOnly = FALSE);
    Token * FindMethodInInterfaceWOPropDesc(Token *pInterface, Token *pToFindToken, BOOL fNameMatchOnly = FALSE);

    void GenerateThunkContext ( Token *pClassToken );
    void GenerateThunkContextImplemenation ( Token *pClassToken );
    void GenerateSingleThunkContextPrototype ( Token *pClassToken, Token * pChildToken, BOOL fNodeContext );
    void GenerateSingleThunkContextImplementation ( Token *pClassToken, Token * pChildToken, BOOL fNodeContext );
    void GenerateClassDISPIDs ( void );
    void GenerateInterfaceDISPIDs ( void );
    void GenerateExternalInterfaceDISPIDs ( void );
    void GenerateEventDISPIDs ( FILE *fp, BOOL fPutDIID );
    BOOL ComputePROPDESC_STRUCT ( FILE *fp, Token *pClassToken, Token *pChild, CString & szHandler, CString & szFnPrefix );
    Token * FindEnum ( Token *pChild );
    char * MapTypeToIDispatch ( CString & szType );
    BOOL ComputeProperty ( Token *pClassToken, Token *pChild );
    BOOL ComputeMethod ( Token *pClassToken, Token *pChild );
    BOOL BuildMethodSignature(Token *pChild,
                              CString &szTypesSig,
                              CString &szArgsType,
                              BOOL &fBSTRArg,
                              BOOL &fVARIANTArg,
                              int  &cArgs,
                              int  &cRequiredArgs,
                              char *pDefaultParams[MAX_ARGS] = NULL,
                              char *pDefaultStrParams[MAX_ARGS] = NULL);
    BOOL GeneratePROPDESCs ( void );
    BOOL GeneratePROPDESCArray ( Token *pThisClassToken, int *pNumVTblEntries );
    void GenerateVTableArray ( Token *pThisClassToken, int *pNumVTblEntries );
    void SortPropDescInfo ( PropdescInfo *pPI, int cPDI );
    BOOL ComputeVTable ( Token *pClass, Token *pInterface, BOOL fDerived, PropdescInfo *pPI, int *piPI, UINT *pUVTblIdx, BOOL fPrimaryTearoff = FALSE );
    BOOL GeneratePropMethodImplementation ( void );
    void SplitTag ( char *pStr, int nLen, char **pTag, int *pnTagLen,
        char **pValue, int *pnValueLen );
    BOOL IsStoredAsString( Token *pClassToken );
    BOOL LookupToken ( LPCSTR pTokenName, int nTokenLen, 
        TokenDescriptor **ppTokenDescriptor, DESCRIPTOR_TYPE *pnTokenDes );
    BOOL GetElem ( char **pStr, char **pElem, int *pnLen, BOOL fBreakOnOpenParenthesis = FALSE, 
        BOOL fBreakOnCloseParenthesis = FALSE, BOOL fBreakOnComma=FALSE );
    LPCSTR ConvertType ( LPCSTR szType );
    BOOL ParseInputFile ( BOOL fDebugging );
    BOOL GenerateHDLFile ( void );
    void GenerateCPPEnumDefs ( void );
    BOOL GetTypeDetails ( char *szTypeName, CString& szHandler, CString &szFnPrefix, StorageType *pStorageType = NULL );
    void GenerateMethodImp ( Token *pClassToken,
        Token *pChildToken, BOOL fIsSet, CString &szHandler,
        CString &szHandlerArgs, CString &szOffsetOf, CString &szAType );
    BOOL FindTearoffMethod ( Token *pTearoff, LPCSTR pszTearoffMethod, LPSTR pszUseTearoff);

    BOOL GenerateTearoffTable ( Token *pClassToken, Token *pTearoff, LPCSTR pszInterface, BOOL fMostDerived );
    BOOL GenerateTearOffMethods ( LPCSTR szClassName, Token *pTearoff, LPCSTR szInterfaceName, BOOL fMostDerived = FALSE );
    BOOL GenerateIDispatchTearoff(LPCSTR szClassName, Token *pTearoff, LPCSTR pszInterfaceName, BOOL fMostDerived = FALSE );
    BOOL HasMondoDispInterface ( Token *pClassToken );
    BOOL PatchInterfaceRefTypes ( void );
    BOOL CloneToken ( Token *pChildToken, DESCRIPTOR_TYPE Type, INT nClassName, INT nTagName );
    void GeneratePropMethodDecl ( Token *pClassToken );
    void GenerateGetAAXImplementations ( Token *pClassToken );
    void GenerateGetAAXPrototypes ( Token *pClassToken );
    BOOL PrimaryTearoff (Token *pInterface);
    Token* NextTearoff (LPCSTR szClassname, Token *pLastTearoff = NULL);
    Token* FindTearoff (LPCSTR szClassname, LPCSTR szInterface);
    BOOL GenerateClassIncludes ( void );
    BOOL GenerateEventFireDecl ( Token *pToken );
    BOOL GenerateEventDecl ( Token *pClassToken, Token *pEventToken );
    BOOL GenerateIDLFile ( char *szFileName );
    void GenerateIDLInterfaceDecl ( Token *pInterfaceToken,
                                    char *pszGUID,
                                    char *pszSuper,
                                    BOOL fDispInterface = FALSE,
                                    Token *pClassToken = NULL );
    void ComputePropType ( Token *pPropertyToken, CString &szProp, BOOL fComment );
    void GenerateMkTypelibDecl ( Token *pInterfaceToken, BOOL fDispInterface = FALSE, Token *pClass =NULL );
    void GenerateMidlInterfaceDecl ( Token *pInterfaceToken, char *pszGUID, char *pszSuper );
    void GenerateCoClassDecl ( Token *pClassToken );
    void GenerateEnumDescIDL ( Token *pEnumToken );
    void ReportError ( LPCSTR szErrorString );
    void GenerateHeaderFile ( void );
    void GenerateIncludeStatement ( Token *pImportToken );
    void GenerateIncludeInterface ( Token *pInterfaceToken );
    void GenerateIncludeEnum(Token *pEnumToken, BOOL fSpitExtern, FILE *pFile = NULL);
    BOOL GenerateHTMFile ( void );
    void GenerateArg ( Token *pArgToken );
    void GenerateInterfaceArg ( Token *pArgToken );
    void GenerateMethodHTM ( Token *pClassToken );
    void GenerateInterfaceMethodHTM ( Token *pClassToken );
    void GenerateEventMethodHTM ( Token *pClassToken );
    void GenerateEnumHTM ( Token *pClassToken, char *pEnumPrefix );
    void GeneratePropertiesHTM ( Token *pClassToken, BOOL fAttributes );
    void GenerateInterfacePropertiesHTM ( Token *pIntfToken );
    void GenerateStruct(Token *pStructToken, FILE *pFile);
    
    Token* FindInterface (CString szInterfaceMatch);
    Token* FindInterfaceLocally (CString szInterfaceMatch);
    BOOL   IsPrimaryInterface(CString szInterface);
    Token* FindClass (CString szClassMatch);
    BOOL IsUniqueCPC ( Token *pClassToken );

    BOOL FindTearoffProperty ( Token *pPropertyToken, LPSTR szTearoffMethod, 
                               LPSTR szTearOffClassName, LPSTR szPropArgs, BOOL fPropGet );
    BOOL GeneratePropDescsInVtblOrder(Token *pClassToken, int *pNumVtblPropDescs);
    BOOL IsSpecialProperty(Token *pClassToken);
    BOOL IsSpecialTearoff(Token *pTearoff);

    const CCachedAttrArrayInfo* GetCachedAttrArrayInfo( LPCSTR szDispId );

    // Manage a dynamic list of types
    CTokenList *pDynamicTypeList;
    CTokenList *pDynamicEventTypeList;
    enum
    {
        DATATYPE_NAME,
        DATATYPE_HANDLER,
    };
    enum 
    {
        TYPE_DATATYPE=-1
    };
    BOOL LookupType ( LPCSTR szTypeName, CString &szIntoString,  
        CString &szFnPrefix, StorageType *pStorageType  = NULL  );
    BOOL AddType ( LPCSTR szTypeName, LPCSTR szHandler );
    BOOL LookupEventType ( CString &szIntoString, LPCSTR szTypeName );
    BOOL AddEventType ( LPCSTR szTypeName, LPCSTR szVTSType );
    BOOL PatchPropertyTypes ( void );
    BOOL PatchInterfaces ();
    BOOL HasClassGotProperties ( Token *pClassToken );
    Token *GetSuperClassTokenPtr ( Token *pClassToken );
    Token *GetPropAbstractClassTokenPtr ( Token *pClassToken );
    BOOL GeneratePropdescReference ( Token *pClassToken, BOOL fDerivedClass, PropdescInfo *pPI, int *pnPI );
    void GeneratePropdescExtern ( Token *pClassToken, BOOL fRecurse = TRUE );
    void Init ( void );


public:
    CPDLParser ();
    ~CPDLParser ()
    {
        if (fpMaxLenFile)
            fclose(fpMaxLenFile);
        if ( fpHDLFile )
            fclose ( fpHDLFile );
        if ( fpHeaderFile )
            fclose ( fpHeaderFile );
        if ( fpIDLFile )
            fclose ( fpIDLFile );
        if ( fpLOGFile )
            fclose ( fpLOGFile );
        if ( fpDISPIDFile )
            fclose ( fpDISPIDFile );
        if ( fpHTMFile )
            fclose ( fpHTMFile );
        if ( fpHTMIndexFile )
            fclose ( fpHTMIndexFile );

        pRuntimeList -> Release();
        pDynamicTypeList -> Release();
        pDynamicEventTypeList -> Release();
    }
    int Parse ( char *szInputFile, 
        char *szOutputFileRoot, 
        char *szPDLFileName, 
        char *szOutputPath,
        BOOL fDebugging );
};


BOOL
GetStdInLine ( char *szLineBuffer, int nMaxLen );


// Every Token array has a name as the first item
#define NAME_TAG 0

// Always have this as the last element in the descriptor array
#define END_OF_ARG_ARRAY { NULL, FALSE, FALSE }

#define ARRAY_SIZE(ar) (sizeof(ar)/sizeof (ar[0]))
