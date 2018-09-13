#include "headers.hxx"

#ifndef X_PDLPARSE_HXX_
#define X_PDLPARSE_HXX_
#include "pdlparse.hxx"
#endif

#ifndef X_DATA_HXX_
#define X_DATA_HXX_
#include "data.hxx"
#endif

#ifndef X_VTABLE_HXX_
#define X_VTABLE_HXX_
#include <vtable.hxx>
#endif

#ifndef X_ASSERT_H_
#define X_ASSERT_H_
#include <assert.h>
#endif

// Win16 needs the old typelibs. Old Typelib can't cope
// with default parameters. So if this flag is set we
// ignore all the default parameters in the PDL files.
// This flag is set based on run-time argument "win16"
BOOL gbWin16Mode = FALSE;

static 
BOOL
GetALine ( FILE *fp, char *szLineBuffer, int nMaxLen, int &nLenRead );


void PDLError (char *szTextError)
{
    printf ( "PDLParse (0) : error PDL0001: /G  %s", szTextError);
}


BOOL TagArray::CompareTag ( INT nIndex, char *szWith )
{
    if ( szValues [ nIndex ] == NULL )
        return FALSE;
    else
        return _stricmp ( szValues [ nIndex ], szWith ) == 0 ? TRUE : FALSE;
}

char *TagArray::GetTagValue ( INT nIndex )
{
    // If the string is null always return an empty string
    if ( szValues [ nIndex ] == NULL )
        return szEmptyString;
    else
        return szValues [ nIndex ];
}

BOOL TagArray::AddTag ( int iTag, LPCSTR szStr, int nLen )
{
    if ( szValues [ iTag ] )
        delete [] ( szValues [ iTag ] );
    if ( nLen == -1 )
        nLen = strlen ( szStr );
    if ( szStr == NULL )
    {
        szValues [ iTag ] = NULL;
        return TRUE;
    }
    szValues [ iTag ] = new char [ nLen + 1 ];
    if ( !szValues [ iTag ] )
        return FALSE;
    strncpy ( szValues [ iTag ], szStr, nLen );
    szValues [ iTag ][ nLen ] = '\0';
    return TRUE;
}


Token::~Token()
{
    if ( _pChildList )
        _pChildList -> Release();
}

void CTokenListWalker::Reset ( void )
{
    _pCurrentToken = NULL;
    if ( _pszFileName )
        _fInCurrentFile = FALSE;
    else
        _fInCurrentFile = TRUE;
    _fAtEnd = FALSE;
}


UINT CString::Lookup ( Associate *pArray, LPCSTR pStr )
{
    UINT uIndex =0;
    while ( pArray->szKey )
    {
        if ( strcmp ( pArray->szKey, pStr ) == 0 )
        {
            strcpy ( szString, pArray->szValue );
            return uIndex;
        }
        pArray++; uIndex++;
    }
    return (UINT)-1;
}

UINT CString::Lookup ( AssociateDataType *pArray, LPCSTR pStr )
{
    UINT uIndex =0;
    while ( pArray->szKey )
    {
        if ( strcmp ( pArray->szKey, pStr ) == 0 )
        {
            strcpy ( szString, pArray->szValue );
            return uIndex;
        }
        pArray++; uIndex++;
    }
    return (UINT)-1;
}


/* static */
char *TagArray::szEmptyString = "";

BOOL
GetStdInLine ( char *szLineBuffer, int nMaxLen )
{
    int nLenRead,i;
    int nTotalRead = 0;

    do
    {
        if ( !GetALine ( NULL, szLineBuffer+nTotalRead, 
            nMaxLen, nLenRead ) )
        {
            return FALSE;
        }
        nTotalRead += nLenRead;
        for ( i = nTotalRead-1 ; i >= 0 ; i-- )
        {
            if ( !isspace ( szLineBuffer [ i ] ) )
            {
                break;
            }
        }
        if ( i > 0 )
        {
            if ( szLineBuffer [ i ] == '\\' )
            {
                szLineBuffer [ i ] = ' ';
                szLineBuffer [ i+1 ] = '\0';
                // we'll go on to the next line
                nTotalRead = i+1; 
                // Don't break, so we append the next line
            }
            else
            {
                // Regular line
                szLineBuffer [ i+1 ] = '\0';
                nTotalRead = i+1;
                // Not continuing - break so we process this line
                break;
            }
        }
        else
        {
            // Completly blank line - ignore it...
            nTotalRead = 0;
        }
    } while ( nTotalRead < nMaxLen );
    return TRUE;
}

static
BOOL
GetALine (  FILE *fp,char *szLineBuffer, int nMaxLen, int &nLenRead )
{
    nLenRead = 0;
    // Keep reading in data until we blow the buffer size, or hit a real
    // EOL. If we have a trailing \ character, go on to the next line
    if ( fp )
    {
        if ( !fgets ( szLineBuffer, nMaxLen, fp )  )
        {
            return FALSE;
        }
    }
    else
    {
        if ( !gets ( szLineBuffer )  )
        {
            return FALSE;
        }
    }

    nLenRead = strlen ( szLineBuffer );
    if ( szLineBuffer [ 0 ] && szLineBuffer [ nLenRead-1 ] == '\n' )
    {
        szLineBuffer [ --nLenRead ] = '\0';
    }
    return TRUE;
}

Token *CTokenList::FindToken ( char *pTokenName, DESCRIPTOR_TYPE nType )  const
{
    Token *pToken;

    for ( pToken = _pFirstToken ;
        pToken != NULL ; pToken = pToken-> GetNextToken() )
    {
        // Token 0 is always the name
        // Case sensitive match
        if ( strcmp ( pToken->GetTagValue ( 0 ), pTokenName ) == 0 )
        {
            return pToken->GetType() ? pToken : NULL;
        }
    }
    return NULL;
}


Token *CTokenList::AddNewToken ( DESCRIPTOR_TYPE nTokenDescriptor )
{
    Token *pNewToken = new Token ( nTokenDescriptor );

    if ( !pNewToken )
        return NULL;

    if ( !_pFirstToken )
    {
        _pFirstToken = pNewToken;
    }
    else
    {
        _pLastToken->SetNextToken ( pNewToken );
    }
    _pLastToken = pNewToken;
    _uTokens++;
    return pNewToken;
}




/*
 Scan the input file and extract the 4 build arguments
    Do this to be compatible with previous version of mkprop
*/

static
BOOL
ScanBuildFile ( char *szBuildFileName,
    char *szInputFile, 
    char *szOutputFileRoot, 
    char *szPDLFileName, 
    char *szOutputPath )
{
    FILE   *fp = NULL;
    char    szLineBuffer [ MAX_LINE_LEN+1 ]; 
    BOOL    fReturn = FALSE;
    int     nRead;

    fp = fopen ( szBuildFileName, "r" );
    if ( !fp )
    {
        printf ( "Cannot open %s\n", szBuildFileName );
        goto cleanup;
    }
    if ( !GetALine ( fp, szLineBuffer, sizeof ( szLineBuffer ), nRead ) )
    {
        printf ( "Cannot read %s\n", szBuildFileName );
        goto cleanup;
    }

    // Old input file used to support "-<flags>" - which was never used
    // so I just skip them
    if (sscanf ( szLineBuffer, "- %s %s %s",
                     (LPCSTR)szOutputFileRoot,
                     (LPCSTR)szInputFile,
                     (LPCSTR)szOutputPath ) != 3)
    {
        printf ( "Cannot read params from %s\n", szBuildFileName );
        goto cleanup;
    }

    while ( GetALine ( fp, szLineBuffer, sizeof ( szLineBuffer ), nRead ) )
        printf ( "Build file Line:%s\n",szLineBuffer ); 

    strcpy ( szPDLFileName, szInputFile );

    fReturn = TRUE;
    goto cleanup;

cleanup:
    if ( fp )
        fclose ( fp );
    return fReturn;
}

CTokenList::~CTokenList()
{
    Token *pNextToken;
    Token *pToken;
    // Tidy up the list
    for ( pToken = _pFirstToken ; pToken != NULL ; pToken = pNextToken )
    {
        pNextToken = pToken -> _pNextToken;
        delete pToken;
    }
}


Token *CTokenListWalker::GetNext ( void )
{
    Token *pToken;
    while ( pToken = GetNextToken() )
    {
        if ( _pszFileName )
        {
            if ( _pCurrentToken->nType == TYPE_FILE )
            {
                if ( _fInCurrentFile && _pszFileName )
                {
                    _fInCurrentFile = FALSE;
                }
                else
                {
                    _fInCurrentFile = pToken->CompareTag ( FILE_TAG, _pszFileName );
                }
            }
            else if ( _fInCurrentFile )
            {
                break;
            }
        }
        else
        {
            break;
        }
    }
    return pToken;
}

Token *CTokenListWalker::GetNext ( DESCRIPTOR_TYPE Type, LPCSTR pName )
{
    Token *pToken;
    while ( pToken = GetNext() )
    {
        if ( pToken -> nType == Type && 
            ( pName == NULL || _stricmp ( pName, pToken -> GetTagValue ( NAME_TAG ) ) == 0 ) ) 
        {
            return pToken;
        }
    }
    return NULL;
}



Token *Token::AddChildToken ( DESCRIPTOR_TYPE nType )
{
    Token *pNewToken;

    if ( _pChildList == NULL )
    {
        if ( ( _pChildList = new CTokenList() ) == NULL )
        {
            return NULL;
        }
    }

    // Created the new arg token
    if ( ! ( pNewToken = _pChildList->AddNewToken ( nType ) ) )
    {
        return NULL;
    }
    return pNewToken;
}

void Token::CalculateEnumMask ( Token *pParentToken )
{
    INT nValue;
    if ( IsSet ( EVAL_VALUE ) )
    {
        nValue = atoi ( GetTagValue ( EVAL_VALUE ) );
    }
    else
    {
        char szText [ MAX_LINE_LEN+1 ];
        nValue = pParentToken -> nNextEnumValue;
        // Plug the new value into the tag
        sprintf ( szText, "%d", nValue );
        AddTag ( EVAL_VALUE, szText );

    }
    if ( nValue >=0 && nValue < 32 )
    {
        pParentToken -> uEnumMask |= (1 << (UINT)nValue);
    }
    pParentToken -> nNextEnumValue = ++nValue;
}

UINT Token::GetChildTokenCount ( void )
{
    return _pChildList ? _pChildList -> GetTokenCount() : 0;
}

// Helper for building comma sperarated arg lists
// if the arg is set add the corresponding text to
// the arg string
void Token::AddParam ( CString &szArg, INT nTag, LPCSTR szText )
{
    if ( IsSet ( nTag ) )
    {
        if ( szArg [ 0 ] != '\0' )
            szArg += ", ";

        szArg += szText;
    }
}

// Helper for building comma sperarated arg lists
// if the arg is set add the corresponding text to
// the arg string. With the current arg value replacing the %s
// szArg must contain one and only one %s
void Token::AddParamStr ( CString &szArg, INT nTag, LPCSTR szText )
{
    char szTemp [ MAX_LINE_LEN+1 ];
    if ( IsSet ( nTag ) )
    {
        sprintf ( szTemp, (LPCSTR)szText, (LPCSTR)GetTagValue ( nTag ) );
        if ( szArg [ 0 ] != '\0' )
            szArg += ", ";
        szArg += szTemp;
    }
}

void Token::Clone ( Token *pFrom )
{
    UINT i;
    INT nLen;

    for ( i = 0 ; i < MAX_TAGS ; i++ )
    {
        nLen = pFrom -> TagValues.GetInternalValue ( i ) ?  strlen ( pFrom -> TagValues.GetInternalValue ( i ) ) : 0;
        TagValues.AddTag ( i, pFrom -> TagValues.GetInternalValue ( i ), nLen );
    }
    nType = pFrom -> nType;

    // Now clone the child arg list if there is one
    // Implemented an Addref/Release to simplify the process
    if ( _pChildList )
    {
        _pChildList -> Release();
        _pChildList = NULL;
    }
    if ( pFrom -> _pChildList )
    {
        // Point ourselves at the child list & AddRef it
        _pChildList = pFrom -> _pChildList;
        _pChildList -> AddRef();
    }
}


struct METH_PARAMS  {
        BOOL        fOptional;
        BOOL        fDefaultValue;
        CString     szVTParamType;
        CString     szCParamType;
        VARTYPE     vtParam;
};

#define ATYPE_Undefined     0
#define ATYPE_Method        1
#define ATYPE_GetProperty   2
#define ATYPE_SetProperty   4

struct DispatchHandler {
    CString     szRawString;
    WORD        invokeType;
    CString     szDispName;
    CString     szVTResult;
    CString     szCResult;
    VARTYPE     vtResult;
    METH_PARAMS params[8];
};


#define MAX_HANDLERS    130
#define MAX_IIDS        130

int             _cHandlers;
DispatchHandler _allHandlers[MAX_HANDLERS];
int             _cIID;
LPSTR           _allIIDs[MAX_IIDS];


BOOL GenerateVTableHeader  ( LPCSTR pszOutputPath )
{
    char    chHeaderFileName[255];
    FILE   *fpHeaderFile;
    int     i;

    strcpy(chHeaderFileName, pszOutputPath);
    strcat(chHeaderFileName, FILENAME_SEPARATOR_STR "funcsig.hxx");

    fpHeaderFile = fopen ( chHeaderFileName, "w" );
    if ( !fpHeaderFile )
    {
        return FALSE;
    }

    fprintf ( fpHeaderFile, "BOOL DispNonDualDIID(IID iid);\n" );

    fprintf ( fpHeaderFile, "typedef HRESULT (*CustomHandler)(CBase *pBase,\n" );
    fprintf ( fpHeaderFile, "                                 IServiceProvider *pSrvProvider,\n" );
    fprintf ( fpHeaderFile, "                                 IDispatch *pInstance,\n" );
    fprintf ( fpHeaderFile, "                                 WORD wVTblOffset,\n" );
    fprintf ( fpHeaderFile, "                                 PROPERTYDESC_BASIC_ABSTRACT *pDesc,\n" );
    fprintf ( fpHeaderFile, "                                 WORD wFlags,\n" );
    fprintf ( fpHeaderFile, "                                 DISPPARAMS *pdispparams,\n" );
    fprintf ( fpHeaderFile, "                                 VARIANT *pvarResult);\n\n\n" );

    // Spit out function signatures for handlers.
    for (i = 0; i < _cHandlers; i++)
    {
        fprintf ( fpHeaderFile, "HRESULT %s (CBase *pBase,\n", (LPCSTR)(_allHandlers[i].szRawString) );
        fprintf ( fpHeaderFile, "            IServiceProvider *pSrvProvider,\n" );
        fprintf ( fpHeaderFile, "            IDispatch *pInstance,\n" );
        fprintf ( fpHeaderFile, "            WORD wVTblOffset,\n" );
        fprintf ( fpHeaderFile, "            PROPERTYDESC_BASIC_ABSTRACT *pDesc,\n" );
        fprintf ( fpHeaderFile, "            WORD wFlags,\n" );
        fprintf ( fpHeaderFile, "            DISPPARAMS *pdispparams,\n" );
        fprintf ( fpHeaderFile, "            VARIANT *pvarResult);\n\n" );
    }

    fprintf ( fpHeaderFile, "\n" );

    // Spit out indexes into handler table.
    for (i = 0; i < _cHandlers; i++)
    {
        fprintf ( fpHeaderFile, "#define \tIDX_%s \t%i\n", (LPCSTR)(_allHandlers[i].szRawString), i );
    }

    fclose ( fpHeaderFile );

    return TRUE;
}


void GenerateFunctionBody ( FILE * fpCXXFile, DispatchHandler *pHandler)
{
    if ((pHandler->invokeType & ATYPE_GetProperty) || (pHandler->invokeType & ATYPE_SetProperty))
    {
        BOOL    fObjectRet = FALSE;
        BOOL    fBSTRParam = FALSE;
        BOOL    fGetAndSet = (pHandler->invokeType & ATYPE_GetProperty) &&
                             (pHandler->invokeType & ATYPE_SetProperty);
        BOOL    fGS_BSTR    = pHandler->szRawString == "GS_BSTR";
        BOOL    fGS_VARIANT = pHandler->szRawString == "GS_VARIANT";
        BOOL    fGS_PropEnum = pHandler->szRawString == "GS_PropEnum";
        char    chIdent[16];

        chIdent[0] = '\0';

        // Does the property muck with an object?
        fObjectRet = ((pHandler->vtResult & VT_TYPEMASK) == VT_UNKNOWN) ||
                      ((pHandler->vtResult & VT_TYPEMASK) == VT_DISPATCH);
        fBSTRParam = ((pHandler->vtResult & VT_TYPEMASK) == VT_BSTR);

        if (pHandler->invokeType & ATYPE_GetProperty)
        {
            fprintf ( fpCXXFile, "    typedef HRESULT (STDMETHODCALLTYPE *OLEVTbl%sPropFunc)(IDispatch *, %s *);\n", fGetAndSet ? "Get" : "", (LPCSTR)pHandler->szCResult );
        }
        if (pHandler->invokeType & ATYPE_SetProperty)
        {
            fprintf ( fpCXXFile, "    typedef HRESULT (STDMETHODCALLTYPE *OLEVTbl%sPropFunc)(IDispatch *, %s);\n", fGetAndSet ? "Set" : "", (LPCSTR)pHandler->szCResult );
        }
        fprintf ( fpCXXFile, "\n" );

        fprintf ( fpCXXFile, "    HRESULT         hr;\n" );

        // Any IDispatch* or IUnknown* arguments?  If so then possible
        // creation of BSTR/object needs to be handled.
        if ((pHandler->invokeType & ATYPE_SetProperty) && (fObjectRet || fBSTRParam || fGS_VARIANT))
        {
            fprintf ( fpCXXFile, "    ULONG   ulAlloc = 0;\n" );
        }

        if (pHandler->invokeType & ATYPE_GetProperty)
        {
            fprintf ( fpCXXFile, "    VTABLE_ENTRY *pVTbl;\n");
        }
        else if (pHandler->invokeType & ATYPE_SetProperty)
        {
            fprintf ( fpCXXFile, "    VTABLE_ENTRY *pVTbl;\n");
        }
        fprintf ( fpCXXFile, "    VARTYPE         argTypes[] = { %s };\n", (LPCSTR)pHandler->szVTResult );

        if (pHandler->invokeType & ATYPE_SetProperty && pHandler->szCResult.Length() > 0)
        {
            fprintf ( fpCXXFile, "    %s    param1;\n", (LPCSTR)pHandler->szCResult );
        }

        fprintf ( fpCXXFile, "\n    Assert(pInstance || pDesc || pdispparams);\n\n" );

        if (pHandler->invokeType & ATYPE_GetProperty)
        {
            fprintf ( fpCXXFile, "    pVTbl = (VTABLE_ENTRY *)(((BYTE *)(*(DWORD_PTR *)pInstance)) + (wVTblOffset*sizeof(VTABLE_ENTRY)/sizeof(DWORD_PTR) + FIRST_VTABLE_OFFSET));\n\n");
        }
        else if (pHandler->invokeType & ATYPE_SetProperty)
        {
            fprintf ( fpCXXFile, "    pVTbl = (VTABLE_ENTRY *)(((BYTE *)(*(DWORD_PTR *)pInstance)) + (wVTblOffset*sizeof(VTABLE_ENTRY)/sizeof(DWORD_PTR) + FIRST_VTABLE_OFFSET));\n\n");
        }

        if ((pHandler->invokeType & ATYPE_GetProperty) && (pHandler->invokeType & ATYPE_SetProperty))
        {
            fprintf ( fpCXXFile, "    if (wFlags & INVOKE_PROPERTYGET)\n" );
            fprintf ( fpCXXFile, "    {\n" );
            strcpy( chIdent, "    " );
        }

        if (pHandler->invokeType & ATYPE_GetProperty)
        {
            if ((pHandler->szCResult.Length() > 0) && (pHandler->vtResult != VT_VARIANT))
            {
                fprintf ( fpCXXFile, "%s    hr = (*(OLEVTbl%sPropFunc)VTBL_PFN(pVTbl))((IDispatch*) VTBL_THIS(pVTbl,pInstance), (%s *)&pvarResult->iVal);\n", chIdent, fGetAndSet ? "Get" : "", (LPCSTR)pHandler->szCResult);
                fprintf ( fpCXXFile, "%s    if (!hr)\n", chIdent );
                fprintf ( fpCXXFile, "%s        V_VT(pvarResult) = argTypes[0];\n", chIdent );
            }
            else
            {
                fprintf ( fpCXXFile, "%s    hr = (*(OLEVTbl%sPropFunc)VTBL_PFN(pVTbl))((IDispatch*)VTBL_THIS(pVTbl, pInstance), pvarResult);\n", chIdent, fGetAndSet ? "Get" : "");
            }
        }

        if ((pHandler->invokeType & ATYPE_GetProperty) && (pHandler->invokeType & ATYPE_SetProperty))
        {
            fprintf ( fpCXXFile, "    }\n" );
            fprintf ( fpCXXFile, "    else\n" );
            fprintf ( fpCXXFile, "    {\n" );
        }

        if (pHandler->invokeType & ATYPE_SetProperty)
        {
            if (pHandler->szCResult.Length() > 0)
            {
                fprintf ( fpCXXFile, "%s     // Convert dispatch params to C params.\n", chIdent );
                fprintf ( fpCXXFile, "%s     hr = DispParamsToCParams(pSrvProvider, pdispparams, ", chIdent);
                if (fObjectRet || fBSTRParam || fGS_VARIANT)
                    fprintf ( fpCXXFile, "&ulAlloc, " );    // Keep track of all allocation.
                else
                    fprintf ( fpCXXFile, "0L, " );

                if (fGS_BSTR || fGS_VARIANT || fGS_PropEnum)
                    fprintf ( fpCXXFile, "((PROPERTYDESC_BASIC_ABSTRACT *)pDesc)->b.wMaxstrlen, " );
                else
                    fprintf ( fpCXXFile, "0, " );
                
                fprintf ( fpCXXFile, "argTypes, &param1, -1);\n" );

                fprintf ( fpCXXFile, "%s     if (hr)\n", chIdent );
                fprintf ( fpCXXFile, "%s         pBase->SetErrorInfo(hr);\n", chIdent );
                fprintf ( fpCXXFile, "%s     else\n", chIdent );
                if (fGetAndSet)
                {
                    fprintf ( fpCXXFile, "%s         hr = (*(OLEVTblSetPropFunc)VTBL_PFN(pVTbl))((IDispatch*)VTBL_THIS(pVTbl,pInstance), (%s)param1);\n", chIdent, (LPCSTR)pHandler->szCResult);
                }
                else
                {
                    fprintf ( fpCXXFile, "%s         hr = (*(OLEVTblPropFunc)VTBL_PFN(pVTbl))((IDispatch*)VTBL_THIS(pVTbl,pInstance), (%s)param1);\n", chIdent, (LPCSTR)pHandler->szCResult);
                }
            }
            else
            {
                fprintf ( fpCXXFile, "%s     hr = (*(OLEVTblPropFunc)VTBL_PFN(pVTbl))((IDispatch*)VTBL_THIS(pVTbl,pInstance), pdispparams[0]);\n", chIdent );
            }
        }

        if ((pHandler->invokeType & ATYPE_GetProperty) && (pHandler->invokeType & ATYPE_SetProperty))
        {
            fprintf ( fpCXXFile, "    }\n" );
        }

        if ((pHandler->invokeType & ATYPE_SetProperty) && (fObjectRet || fBSTRParam || fGS_VARIANT))
        {
            if (fGS_VARIANT)
                fprintf ( fpCXXFile, "    if (ulAlloc)\n" );
            else 
                fprintf ( fpCXXFile, "    if (ulAlloc && param1)\n" );

            if (fObjectRet)
            {
                fprintf ( fpCXXFile, "        param1->Release();\n");
            }
            else if (fBSTRParam)
            {
                fprintf ( fpCXXFile, "        SysFreeString(param1);\n");
            }
            else if (fGS_VARIANT)
            {
                fprintf ( fpCXXFile, "    {\n" );
                fprintf ( fpCXXFile, "        Assert(V_VT(&param1) == VT_BSTR);\n" );
                fprintf ( fpCXXFile, "        SysFreeString(V_BSTR(&param1));\n" );
                fprintf ( fpCXXFile, "    }\n" );
            }
        }

        fprintf ( fpCXXFile, "\n    return hr;\n" );
    }
    else if ( pHandler->invokeType & ATYPE_Method )
    {
        // We've got a method to spit out.
        char    chVARMacro[16];
        int     i;
        BOOL    fParamConvert;
        BOOL    fBSTRParam = FALSE;
        BOOL    fObjectParam = FALSE;
        BOOL    fVariantParam = FALSE;
        int     cArgs;

        if (pHandler->vtResult & VT_BYREF)
        {
            strcpy(chVARMacro, "V_BYREF");
        }
        else
        {
            switch (pHandler->vtResult & VT_TYPEMASK) {
            case VT_EMPTY:
                strcpy(chVARMacro, "");
                break;
            case VT_I2:
                strcpy(chVARMacro, "V_I2");
                break;
            case VT_I4:
                strcpy(chVARMacro, "V_I4");
                break;
            case VT_R4:
                strcpy(chVARMacro, "V_R4");
                break;
            case VT_R8:
                strcpy(chVARMacro, "V_R8");
                break;
            case VT_CY:
                strcpy(chVARMacro, "V_CY");
                break;
            case VT_BSTR:
                strcpy(chVARMacro, "V_BSTR");
                break;
            case VT_DISPATCH:
            case VT_VARIANT:
            case VT_UNKNOWN:
                strcpy(chVARMacro, "V_BYREF");
                break;
            case VT_BOOL:
                strcpy(chVARMacro, "V_BOOL");
                break;
            default:
                strcpy(chVARMacro, "Unsupported");
            }
        }

        fprintf ( fpCXXFile, "    typedef HRESULT (STDMETHODCALLTYPE *MethodOLEVTblFunc)(IDispatch *" );

        i = 0;
        cArgs = 0;
        while ( pHandler->params[i].szVTParamType.Length() )
        {
            fprintf ( fpCXXFile, ", %s", (LPCSTR)pHandler->params[i].szCParamType );
            i++;
            cArgs++;
        }

        if ( pHandler->vtResult != VT_EMPTY )
        {
            fprintf ( fpCXXFile, ", %s", (LPCSTR)pHandler->szCResult );
        }

        fprintf ( fpCXXFile, ");\n\n" );

        fprintf ( fpCXXFile, "    HRESULT               hr;\n" );
        fprintf ( fpCXXFile, "    VTABLE_ENTRY     *pVTbl;\n" );

        fParamConvert = cArgs;

        // Any paramters?
        if (fParamConvert)
        {
            int     iDefIndex;

            fprintf ( fpCXXFile, "    VARTYPE               argTypes[] = {\n" );

            i = 0;
            while ( pHandler->params[i].szVTParamType.Length() )
            {
                fprintf ( fpCXXFile, "                                %s,\n",
                          (LPCSTR)pHandler->params[i].szVTParamType );
                i++;
            }

            fprintf ( fpCXXFile, "                          };\n" );

            i = 0;
            iDefIndex = 0;
            while ( pHandler->params[i].szVTParamType.Length() )
            {
                fBSTRParam |= ((pHandler->params[i].vtParam & VT_TYPEMASK) == VT_BSTR);
                fVariantParam |= ((pHandler->params[i].vtParam & VT_TYPEMASK) == VT_VARIANT);
                fObjectParam |= ((pHandler->params[i].vtParam & VT_TYPEMASK) == VT_UNKNOWN) ||
                                ((pHandler->params[i].vtParam & VT_TYPEMASK) == VT_DISPATCH);

                if (pHandler->params[i].fOptional)
                {
                    if (pHandler->params[i].vtParam == (VT_VARIANT | VT_BYREF))
                    {
                        fprintf ( fpCXXFile, "    CVariant   param%iopt(VT_ERROR);      // optional variant\n", i + 1 );
                        fprintf ( fpCXXFile, "    VARIANT   *param%i = &param%iopt;      // optional arg.\n", i + 1, i + 1 );
                    }
                    else if (pHandler->params[i].vtParam == VT_VARIANT)
                    {
                        fprintf ( fpCXXFile, "    VARIANT    param%i;    // optional arg.\n", i + 1 );
                    }
                    else
                    {
                        // error, only VARIANT and VARIANT * can be optional.
                        PDLError("optional only allowed for VARIANT and VARIANT *");
                        return;
                    }
                }
                else if (pHandler->params[i].fDefaultValue)
                {
                    if (pHandler->params[i].vtParam == VT_BSTR)
                    {
                        fprintf ( fpCXXFile, "    %s    paramDef%i = SysAllocString((TCHAR *)(((PROPERTYDESC_METHOD *)pDesc)->c->defargs[%i]));\n",
                                  (LPCSTR)pHandler->params[i].szCParamType,
                                  i + 1,
                                  iDefIndex );
                        fprintf ( fpCXXFile, "    %s    param%i = paramDef%i;\n",
                                  (LPCSTR)pHandler->params[i].szCParamType,
                                  i + 1,
                                  i + 1);
                    }
                    else
                    {
                        fprintf ( fpCXXFile, "    %s    param%i = (%s)(((PROPERTYDESC_METHOD *)pDesc)->c->defargs[%i]);\n",
                                  (LPCSTR)pHandler->params[i].szCParamType,
                                  i + 1,
                                  (LPCSTR)pHandler->params[i].szCParamType,
                                  iDefIndex );
                    }

                    iDefIndex++;
                }
                else
                {
                    // Non optional variants need conversion if they contain a string that it longer than maxlen
                    fprintf ( fpCXXFile, "    %s    param%i;\n", (LPCSTR)pHandler->params[i].szCParamType, i + 1 );
                }
                i++;
            }

            // Any BSTR or IDispatch* or IUnknown* arguments?  If so then possible
            // creation of BSTR/object needs to be handled.
            if (fBSTRParam || fObjectParam || fVariantParam)
            {
                fprintf ( fpCXXFile, "    ULONG   ulAlloc = 0;\n" );
            }

            fprintf ( fpCXXFile, "\n" );
        }

        i = 0;
        while ( pHandler->params[i].szVTParamType.Length() )
        {
            if ( pHandler->params[i].fOptional &&
                 pHandler->params[i].vtParam == VT_VARIANT )
            {
                fprintf ( fpCXXFile, "    V_VT(&param%i) = VT_ERROR;\n", i + 1 );
            }

            i++;
        };
        fprintf ( fpCXXFile, "\n" );

        fprintf ( fpCXXFile, "    Assert(pInstance || pDesc || pdispparams);\n\n" );
        fprintf ( fpCXXFile, "    pVTbl = (VTABLE_ENTRY*)(((BYTE *)(*(DWORD_PTR *)pInstance)) + wVTblOffset * sizeof(VTABLE_ENTRY)/sizeof(DWORD_PTR) + FIRST_VTABLE_OFFSET);\n\n" );

        if ( pHandler->params[0].vtParam == VT_ARRAY )
        {
            fprintf ( fpCXXFile, "    // Convert dispatch params to safearray.\n" );
            fprintf ( fpCXXFile, "    param1 = DispParamsToSAFEARRAY(pdispparams);\n" );
        }
        else if ( fParamConvert )
        {
            // Any parameters?
            fprintf ( fpCXXFile, "    // Convert dispatch params to C params.\n" );
            fprintf ( fpCXXFile, "    hr = DispParamsToCParams(pSrvProvider, pdispparams, " );
            if (fBSTRParam || fObjectParam || fVariantParam)
                fprintf ( fpCXXFile, "&ulAlloc, " );
            else
                fprintf ( fpCXXFile, "NULL, " );
            
            if (fBSTRParam || fVariantParam)
                fprintf ( fpCXXFile, "((PROPERTYDESC_BASIC_ABSTRACT *)pDesc)->b.wMaxstrlen" );
            else
                fprintf ( fpCXXFile, "0" );

            fprintf ( fpCXXFile, ", argTypes" );
          
            i = 0;
            while ( pHandler->params[i].szVTParamType.Length() )
            {
                fprintf ( fpCXXFile, ", &param%i", i + 1 );
                i++;
            }

            fprintf ( fpCXXFile, ", -1L);\n" );

            fprintf ( fpCXXFile, "    if (hr)\n" );
            fprintf ( fpCXXFile, "    {\n" );
            fprintf ( fpCXXFile, "        pBase->SetErrorInfo(hr);\n" );
            fprintf ( fpCXXFile, "        goto Cleanup;\n" );
            fprintf ( fpCXXFile, "    }\n\n" );
        }

        fprintf ( fpCXXFile, "    hr = (*(MethodOLEVTblFunc)VTBL_PFN(pVTbl))((IDispatch*)VTBL_THIS(pVTbl,pInstance)", chVARMacro );

        i = 0;
        while ( pHandler->params[i].szVTParamType.Length() )
        {
            fprintf ( fpCXXFile, ", param%i", i + 1 );
            i++;
        }

        // Do we have a result type?
        if ( pHandler->vtResult != VT_EMPTY )
        {
            if (((pHandler->vtResult & VT_BYREF) == VT_BYREF) && ((pHandler->vtResult & VT_TYPEMASK) != VT_VARIANT))
            {
                fprintf ( fpCXXFile, ", (%s)(&(pvarResult->pdispVal))", (LPCSTR)pHandler->szCResult );
            }
            else
            {
                fprintf ( fpCXXFile, ", pvarResult" );
            }
        }

        fprintf ( fpCXXFile, ");\n" );

        if (((pHandler->vtResult & VT_BYREF) == VT_BYREF) && ((pHandler->vtResult & VT_TYPEMASK) != VT_VARIANT))
        {
            fprintf ( fpCXXFile, "    if (!hr)\n");
            fprintf ( fpCXXFile, "        V_VT(pvarResult) = (%s) & ~VT_BYREF;\n", (LPCSTR)pHandler->szVTResult );
        }

        if ( pHandler->params[0].vtParam == VT_ARRAY )
        {
            fprintf ( fpCXXFile, "    if (param1)\n" );
            fprintf ( fpCXXFile, "    {\n" );
            fprintf ( fpCXXFile, "        HRESULT hr1 = SafeArrayDestroy(param1);\n" );
            fprintf ( fpCXXFile, "        if (hr1)\n");
            fprintf ( fpCXXFile, "            hr = hr1;\n");
            fprintf ( fpCXXFile, "    }\n" );
        }
        else if ( fParamConvert )
        {
            // Any parameters then we'll need a cleanup routine.
            fprintf ( fpCXXFile, "\nCleanup:\n" );
        }


        // Deallocate any default BSTR parameters.
        i = 0;
        while ( pHandler->params[i].szVTParamType.Length() )
        {
            if (pHandler->params[i].fDefaultValue && pHandler->params[i].vtParam == VT_BSTR)
            {
                fprintf ( fpCXXFile, "    SysFreeString(paramDef%i);\n", i + 1);
            }
            i++;
        }

        if (fBSTRParam || fObjectParam || fVariantParam)
        {
            i = 0;
            while ( pHandler->params[i].szVTParamType.Length() )
            {
                // Variants don't need conversion.
                if ((pHandler->params[i].vtParam & VT_TYPEMASK) == VT_BSTR)
                {
                    fprintf ( fpCXXFile, "    if (ulAlloc & %i)\n",  (1 << i) );
                    fprintf ( fpCXXFile, "        SysFreeString(param%i);\n", i + 1);
                }
                if ((pHandler->params[i].vtParam & VT_TYPEMASK) == VT_UNKNOWN ||
                    (pHandler->params[i].vtParam & VT_TYPEMASK) == VT_DISPATCH)
                {
                    if (!(pHandler->params[i].vtParam & VT_BYREF))
                    {
                        fprintf ( fpCXXFile, "    if ((ulAlloc & %i) && param%i)\n",  (1 << i), i + 1 );
                        fprintf ( fpCXXFile, "        param%i->Release();\n", i + 1);
                    }
                    else
                    {
                        fprintf ( fpCXXFile, "    if ((ulAlloc & %i) && *param%i)\n",  (1 << i), i + 1 );
                        fprintf ( fpCXXFile, "        (*param%i)->Release();\n", i + 1);
                    }
                }
                if ((pHandler->params[i].vtParam & VT_TYPEMASK) == VT_VARIANT)
                {
                    fprintf ( fpCXXFile, "    if (ulAlloc & %i)\n",  (1 << i) );
                    fprintf ( fpCXXFile, "    {\n" );
                    fprintf ( fpCXXFile, "        Assert(V_VT(%sparam%i) == VT_BSTR);\n",
                                        pHandler->params[i].vtParam & VT_BYREF ? "" : "&", i + 1);
                    fprintf ( fpCXXFile, "        SysFreeString(V_BSTR(%sparam%i));\n", 
                                        pHandler->params[i].vtParam & VT_BYREF ? "" : "&", i + 1);
                    fprintf ( fpCXXFile, "    }\n" );
                }
                i++;
            }
        }

        fprintf ( fpCXXFile, "    return hr;\n" );
    }
}


BOOL GenerateVTableCXX  ( LPCSTR pszOutputPath )
{
    char    chCXXFileName[255];
    FILE   *fpCXXFile;
    int     i;

    strcpy(chCXXFileName, pszOutputPath);
    strcat(chCXXFileName, FILENAME_SEPARATOR_STR "funcsig.cxx");

    fpCXXFile = fopen ( chCXXFileName, "w" );
    if ( !fpCXXFile )
    {
        return FALSE;
    }

    //
    // Generate CXX file.
    //

    // Spit out handler table.
    fprintf ( fpCXXFile, "\n\nstatic const CustomHandler  _HandlerTable[] = {\n" );

    for (i = 0; i < _cHandlers; i++)
    {
        fprintf ( fpCXXFile, "   %s,\n", (LPCSTR)(_allHandlers[i].szRawString));
    }

    fprintf ( fpCXXFile, "   NULL\n" );
    fprintf ( fpCXXFile, "};\n\n" );

    // Spit out function signatures for handlers.
    for (i = 0; i < _cHandlers; i++)
    {
        fprintf ( fpCXXFile, "HRESULT %s (CBase *pBase,\n", (LPCSTR)(_allHandlers[i].szRawString) );
        fprintf ( fpCXXFile, "            IServiceProvider *pSrvProvider,\n" );
        fprintf ( fpCXXFile, "            IDispatch *pInstance,\n" );
        fprintf ( fpCXXFile, "            WORD wVTblOffset,\n" );
        fprintf ( fpCXXFile, "            PROPERTYDESC_BASIC_ABSTRACT *pDesc,\n" );
        fprintf ( fpCXXFile, "            WORD wFlags,\n" );
        fprintf ( fpCXXFile, "            DISPPARAMS *pdispparams,\n" );
        fprintf ( fpCXXFile, "            VARIANT *pvarResult)\n" );
        fprintf ( fpCXXFile, "{\n" );

        GenerateFunctionBody ( fpCXXFile, &(_allHandlers[i]) );

        fprintf ( fpCXXFile, "}\n" );
    }

    fprintf ( fpCXXFile, "\n" );

    for (i = 0; i < _cIID; i++)
    {
        fprintf ( fpCXXFile, "EXTERN_C const IID IID_%s;\n", _allIIDs[i] );
    }

    fprintf ( fpCXXFile, "\n\n" );

    fprintf ( fpCXXFile, "#define MAX_IIDS %i\n", _cIID + 1);   // The null entry at index 0.

    fprintf ( fpCXXFile, "static const IID * _IIDTable[MAX_IIDS] = {\n" );
    fprintf ( fpCXXFile, "\tNULL,\n" );     // Index 0 is reserved for primary dispatch interface.
    // Spit out indexes into IID table.
    for (i = 0; i < _cIID; i++)
    {
        fprintf ( fpCXXFile, "\t&IID_%s,\n", _allIIDs[i] );
    }
    fprintf ( fpCXXFile, "};\n");

    fprintf ( fpCXXFile, "\n" );

    // Helper function for mapping DISP_IHTMLxxxxx
    fprintf ( fpCXXFile, "#define DIID_DispBase   0x3050f500\n" );
    fprintf ( fpCXXFile, "#define DIID_DispMax    0x3050f5a0\n" );
    fprintf ( fpCXXFile, "\n" );
    fprintf ( fpCXXFile, "const GUID DIID_Low12Bytes = { 0x00000000, 0x98b5, 0x11cf, { 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b } };\n" );
    fprintf ( fpCXXFile, "\n\n" );
    fprintf ( fpCXXFile, "BOOL DispNonDualDIID(IID iid)\n" );
    fprintf ( fpCXXFile, "{\n" );
    fprintf ( fpCXXFile, "\tBOOL    fRetVal = FALSE;\n" );
    fprintf ( fpCXXFile, "\n" );
    fprintf ( fpCXXFile, "\tif (iid.Data1 >= DIID_DispBase && iid.Data1 <= DIID_DispMax)\n" );
    fprintf ( fpCXXFile, "\t{\n" );
    fprintf ( fpCXXFile, "\t\tfRetVal = memcmp(&iid.Data2,\n" );
    fprintf ( fpCXXFile, "\t\t\t\t&DIID_Low12Bytes.Data2,\n" );
    fprintf ( fpCXXFile, "\t\t\t\tsizeof(IID) - sizeof(DWORD)) == 0;\n" );
    fprintf ( fpCXXFile, "\t}\n" );
    fprintf ( fpCXXFile, "\n" );
    fprintf ( fpCXXFile, "\treturn fRetVal;\n" );
    fprintf ( fpCXXFile, "}\n\n" );

    fclose ( fpCXXFile );

    return TRUE;
}


struct TypeMap {
    char    chType[32];
    char    chAutomationType[32];
    char    chCType[32];
    VARTYPE vtResult;
};

static TypeMap  _mapper[] = {
    { "IDispatchp"          ,   "VT_DISPATCH"               , "IDispatch *"     ,   VT_DISPATCH             },  // Entry 0 is VT_DISPATCH
    { "IDispatchpp"         ,   "VT_DISPATCH | VT_BYREF"    , "IDispatch **"    ,   VT_DISPATCH | VT_BYREF  },  // Entry 1 is VT_DISPATCH | VT_BYREF
    { "DWORD"               ,   "VT_I4"                     , "DWORD"           ,   VT_I4                   },
    { "long"                ,   "VT_I4"                     , "LONG"            ,   VT_I4                   },
    { "LONG"                ,   "VT_I4"                     , "LONG"            ,   VT_I4                   },
    { "short"               ,   "VT_I2"                     , "SHORT"           ,   VT_I2                   },
    { "VARIANT"             ,   "VT_VARIANT"                , "VARIANT"         ,   VT_VARIANT              },
    { "BSTR"                ,   "VT_BSTR"                   , "BSTR"            ,   VT_BSTR                 },
    { "BOOL"                ,   "VT_BOOL"                   , "VARIANT_BOOL"    ,   VT_BOOL                 },
    { "BOOLp"               ,   "VT_BOOL | VT_BYREF"        , "VARIANT_BOOL *"  ,   VT_BOOL | VT_BYREF      },
    { "VARIANTp"            ,   "VT_VARIANT | VT_BYREF"     , "VARIANT *"       ,   VT_VARIANT | VT_BYREF   },
    { "IUnknownp"           ,   "VT_UNKNOWN"                , "IUnknown *"      ,   VT_UNKNOWN              },
    { "IUnknownpp"          ,   "VT_UNKNOWN | VT_BYREF"     , "IUnknown **"     ,   VT_UNKNOWN | VT_BYREF   },
    { "float"               ,   "VT_R4"                     , "float"           ,   VT_R4                   },
    { "longp"               ,   "VT_I4 | VT_BYREF"          , "LONG *"          ,   VT_I4 | VT_BYREF        },
    { "BSTRp"               ,   "VT_BSTR | VT_BYREF"        , "BSTR *"          ,   VT_BSTR | VT_BYREF      },
    { "int"                 ,   "VT_I4"                     , "LONG"            ,   VT_I4                   },
    { "VARIANTBOOL"         ,   "VT_BOOL"                   , "VARIANT_BOOL"    ,   VT_BOOL                 },
    { "VARIANTBOOLp"        ,   "VT_BOOL | VT_BYREF"        , "VARIANT_BOOL *"  ,   VT_BOOL | VT_BYREF      },
    { "SAFEARRAYPVARIANTP"  ,   "VT_ARRAY"                  , "SAFEARRAY *"     ,   VT_ARRAY                },
    { "void"                ,   ""                          , ""                ,   VT_EMPTY                },
    { "PropEnum"            ,   "VT_BSTR"                   , "BSTR"            ,   VT_BSTR                 },  // BUGBUG: Remove and fix.
    { "IHTMLControlElementp",   "VT_DISPATCH"               , "IDispatch *"     ,   VT_DISPATCH             },  // BUGBUG: Not converted to IDispatch
    { "IHTMLElementpp"      ,   "VT_DISPATCH | VT_BYREF"    , "IDispatch **"    ,   VT_DISPATCH | VT_BYREF  },  // BUGBUG: Not converted to IDispatch
    { "IHTMLElementCollectionpp"      ,   "VT_DISPATCH | VT_BYREF"    , "IDispatch **"    ,   VT_DISPATCH | VT_BYREF  },  // BUGBUG: Not converted to IDispatch
    { ""                    ,   ""                          , ""                ,   VT_EMPTY                }
};

BOOL MapTypeToAutomationType (char *pTypeStr, char **ppAutomationType, char **ppCType, VARTYPE *pVTType)
{
    int     i = 0;
    VARTYPE vtTemp;

    if (!pVTType)
        pVTType = &vtTemp;

    while (_mapper[i].chType[0])
    {
        if (strcmp(_mapper[i].chType, pTypeStr) == 0)
        {
            *ppAutomationType = _mapper[i].chAutomationType;
            *ppCType = _mapper[i].chCType;
            *pVTType = _mapper[i].vtResult;
            return TRUE;
        }

        i++;
    }
    return FALSE;
}


BOOL ProcessDatFile ( LPCSTR pszOutputPath )
{
    char   *buffer;
    FILE   *fpDatFile;
    char    chDatFileName[255];
    char    szTextError[MAX_LINE_LEN+1];
    char   *pATypeStr;
    char   *pCTypeStr;
    int     cLines;

    if ( !pszOutputPath )
        return FALSE;

    strcpy(chDatFileName, pszOutputPath);
    strcat(chDatFileName, FILENAME_SEPARATOR_STR "funcsig.dat");

    fpDatFile = fopen ( chDatFileName, "rb");

    if ( !fpDatFile )
    {
        return FALSE;
    }

    if ( fseek( fpDatFile, 0, SEEK_END ) == 0 )
    {
        fpos_t  pos;

        if ( fgetpos( fpDatFile, &pos ) == 0 )
        {
            int     i = 0;

            buffer = new char[pos];
            if (buffer == NULL)
                return FALSE;

            fseek( fpDatFile, 0, SEEK_SET );

            if ( fread ( buffer, 1, pos, fpDatFile ) != pos )
                return FALSE;

            // Create the header file from the .dat file.
            _cHandlers = buffer[0];     // Number of signatures
            _cIID = buffer[1];          // Number of IIDs

            if (_cHandlers >= MAX_HANDLERS)
            {
                PDLError("To many handlers increase MAX_HANDLERS\n");
                goto Error;
            }

            if (_cIID >= MAX_IIDS)
            {
                PDLError("To many handlers increase MAX_IIDS\n");
                goto Error;
            }

            i = 4;                      // Skip 4 byte header.
            cLines = 0;
            while ( cLines < _cHandlers )
            {
                char   *pStrWork;
                char   *pStrWork2;
                int     cStr = strlen ( buffer + i );

                // Copy raw function signature.
                _allHandlers[cLines].szRawString = (buffer + i);

                pStrWork = buffer + i;

                // Get type
                pStrWork2 = strchr(pStrWork, '_');
                if (!pStrWork2)
                    goto Error;
                *pStrWork2 = '\0';

                _allHandlers[cLines].invokeType = ATYPE_Undefined;
                if (strchr(pStrWork, 'G'))
                    _allHandlers[cLines].invokeType |= ATYPE_GetProperty;
                if (strchr(pStrWork, 'S'))
                    _allHandlers[cLines].invokeType |= ATYPE_SetProperty;
                if (strchr(pStrWork, 'M'))
                    _allHandlers[cLines].invokeType = ATYPE_Method;

                // Point pass the G, S, GS or Method
                pStrWork = pStrWork2 + 1;

                if (_allHandlers[cLines].invokeType == ATYPE_Method)
                {
                    // Do method parsing:
                    int     iArg;

                    pStrWork2 = strchr(pStrWork, '_');
                    if (!pStrWork2)
                    {
                        PDLError("bad method result");
                        return FALSE;
                    }

                    *pStrWork2 = '\0';

                    // Get result type
                    if (MapTypeToAutomationType(pStrWork, &pATypeStr, &pCTypeStr, &(_allHandlers[cLines].vtResult)))
                    {
                        _allHandlers[cLines].szVTResult = pATypeStr;
                        _allHandlers[cLines].szCResult = pCTypeStr;
                    }
                    else
                    {
                        sprintf(szTextError, "result type '%s' not found.", pStrWork);
                        PDLError(szTextError);
                        return FALSE;
                    }

                    pStrWork = pStrWork2 + 1;

                    // arg parsing:
                    iArg = 0;
                    while (*pStrWork)
                    {
                        pStrWork2 = strchr(pStrWork, '_');
                        if (pStrWork2)
                        {
                            *pStrWork2 = '\0';
                        }

                        // optional argument?
                        _allHandlers[cLines].params[iArg].fOptional = 
                                            (pStrWork[0] == 'o' &&
                                             pStrWork[1] == '0' &&
                                             pStrWork[2] == 'o');
                        _allHandlers[cLines].params[iArg].fDefaultValue = 
                                            (pStrWork[0] == 'o' &&
                                             pStrWork[1] == 'D' &&
                                             pStrWork[2] == 'o');

                        if (_allHandlers[cLines].params[iArg].fOptional ||
                            _allHandlers[cLines].params[iArg].fDefaultValue)
                        {
                            pStrWork += 3;  // Skip o0o or oDo which signals optional (litle o zero little o) or defaultValue.
                        }

                        if (MapTypeToAutomationType(pStrWork, &pATypeStr, &pCTypeStr, &(_allHandlers[cLines].params[iArg].vtParam)))
                        {
                            _allHandlers[cLines].params[iArg].szVTParamType = pATypeStr;
                            _allHandlers[cLines].params[iArg].szCParamType = pCTypeStr;
                        }
                        else
                        {
                            sprintf(szTextError, "argument type '%s' not found.", pStrWork);
                            PDLError(szTextError);
                            return FALSE;
                        }

                        pStrWork += strlen(pStrWork);
                        // More args to parse?
                        if (pStrWork2)
                            pStrWork++;         // Point at next arg.

                        iArg++;
                    }
                }
                else
                {
                    // Do property parsing
                    if (MapTypeToAutomationType(pStrWork, &pATypeStr, &pCTypeStr, &(_allHandlers[cLines].vtResult)))
                    {
                        _allHandlers[cLines].szVTResult = pATypeStr;
                        _allHandlers[cLines].szCResult = pCTypeStr;
                    }
                    else
                    {
                        sprintf(szTextError, "type '%s' not found.", pStrWork);
                        PDLError(szTextError);
                        return FALSE;
                    }
                }

                cLines++;
                i += cStr + 1;
            }

            // Load up the IIDs.
            cLines = 0;
            while ( cLines < _cIID )
            {
                int     cStr = strlen ( buffer + i );

                _allIIDs[cLines] = buffer + i;

                cLines++;
                i += cStr + 1;
            }

            fclose ( fpDatFile );

            return TRUE;
        }
    }

Error:
    return FALSE;
}


int __cdecl
main  ( int argc, char *argv[] )
{
    int nReturnCode;
    char szInputFile [ MAX_PATH+1 ];
    char szOutputFileRoot [ MAX_PATH+1 ];
    char szPDLFileName [ MAX_PATH+1 ];
    char szOutputPath [ MAX_PATH+1 ];
    BOOL fDebugging = FALSE;

    {
    CPDLParser Parser;

    // argv[1] is the name of a file containing build args
    // arg1 of this file is the full path/filename of the input file
    // arg2 is the full name of the output file, minus the file extension
    // arg3 is the 8.3 pdl file name
    // arg4 is a log file name
    if ( argc == 5 )
    {
        strcpy ( szInputFile, argv [ 1 ] );
        strcpy ( szOutputFileRoot, argv [ 2 ] );
        strcpy ( szPDLFileName, argv [ 3 ] );
        strcpy ( szOutputPath, argv [ 4 ] );
        fDebugging = TRUE;
    }
    else if ( argc == 3 && _stricmp( argv[1], "/g") == 0 ||
                           _stricmp( argv[1], "-g") == 0 )
    {
        // Process the funcsig.dat file and produce custsig.hxx file:
        if (!ProcessDatFile ( argv [ 2 ] ))
        {
            nReturnCode = 4;
            goto Cleanup;
        }

        if (!GenerateVTableHeader ( argv [ 2 ] ))
        {
            nReturnCode = 4;
            goto Cleanup;
        }

        if (!GenerateVTableCXX ( argv [ 2 ] ))
        {
            nReturnCode = 4;
            goto Cleanup;
        }

        FILE *fpMaxLenFile = NULL;

        strcpy(szOutputPath, argv [ 2 ]);
        strcat(szOutputPath, FILENAME_SEPARATOR_STR "maxlen.txt");

        fpMaxLenFile = fopen(szOutputPath, "r+");
        if (fpMaxLenFile)
        {
            char chMarker[6];
            strcpy(chMarker, "Const");
            fwrite(chMarker, sizeof(char), 5, fpMaxLenFile);
            fclose(fpMaxLenFile);
        }

        nReturnCode = 0;
        goto Cleanup;
    }
    else if ( argc > 1 )
    {
        if ( !ScanBuildFile ( argv[ 1 ],
                              szInputFile,
                              szOutputFileRoot,
                              szPDLFileName,
                              szOutputPath ) )
        {
            printf ( "Cant scan build file\n" );
            nReturnCode = 2;
            goto Cleanup;
        }
    }
    else
    {
        printf ( "Invalid command line params\n" );
        nReturnCode = 3;
        goto Cleanup;
    }

    nReturnCode = Parser.Parse ( szInputFile, szOutputFileRoot, 
        szPDLFileName, szOutputPath, fDebugging  );

    }

Cleanup:
    if ( nReturnCode != 0 )
        printf ( "Error %d building PDL file\n", nReturnCode );
    exit ( nReturnCode );
    return nReturnCode;
}

#ifndef X_DATA_CXX_
#define X_DATA_CXX_
#include "data.cxx"
#endif

#ifndef X_PARSER_CXX_
#define X_PARSER_CXX_
#include "parser.cxx"
#endif
