extern BOOL gbWin16Mode;

UINT CPDLParser::CountTags ( TokenDescriptor *tokdesc )
{
    UINT uCount;
    TagDescriptor *pTagDescriptor;
    for ( uCount = 0, pTagDescriptor = tokdesc -> Tags ; pTagDescriptor -> szTag != NULL;
        uCount++, pTagDescriptor++ );
    return uCount;
}

CPDLParser::CPDLParser()
{
    fpHDLFile = NULL;
    fpHeaderFile = NULL;
    fpIDLFile = NULL;
    fpLOGFile = NULL;
    fpHTMFile = NULL;
    fpHTMIndexFile = NULL;
    fpDISPIDFile = NULL;
    fpMaxLenFile = NULL;
    pRuntimeList = new CTokenList;
    pDynamicTypeList = new CTokenList;
    pDynamicEventTypeList = new CTokenList;
    Init();
}
                  

void
CPDLParser::SplitTag ( char *pStr, int nLen, char **pTag, int *pnTagLen,
    char **pValue, int *pnValueLen )
{
    *pTag = pStr;
    for ( *pnTagLen = 0 ; *pnTagLen < nLen ; pStr++, (*pnTagLen)++ )
    {
        if ( *pStr == ':' )
        {
            *pValue = pStr + 1;
            *pnValueLen = nLen - *pnTagLen -1;
            return;
        }
    }
    *pnValueLen = 0;
}



BOOL
CPDLParser::LookupToken ( LPCSTR pTokenName, int nTokenLen,
    TokenDescriptor **ppTokenDescriptor, DESCRIPTOR_TYPE *pnTokenDes )
{
    INT i;
    for ( i = 0 ;
        i < NUM_DESCRIPTOR_TYPES ; i++ )
    {
        if (strlen(AllDescriptors[i]->szTokenName) == (UINT)nTokenLen &&
            !_strnicmp(AllDescriptors[i]->szTokenName, pTokenName, nTokenLen))
        {
            *ppTokenDescriptor = AllDescriptors [ i ];
            *pnTokenDes = (DESCRIPTOR_TYPE)i;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL CPDLParser::GetElem ( char **pStr,
                          char **pElem,
                          int *pnLen,
                          BOOL fBreakOnOpenParenthesis, /* = FALSE */
                          BOOL fBreakOnCloseParenthesis, /* = FALSE */
                          BOOL fBreakOnCommas /* = FALSE */ )
{
    *pnLen = 0;

    while ( isspace ( **pStr ) )
    {
        (*pStr)++;
    }
    *pElem = *pStr;
    if (( fBreakOnOpenParenthesis && **pStr == ')' ) ||
        ( fBreakOnCloseParenthesis &&  **pStr == '('  ) ||
        (fBreakOnCommas &&  **pStr == ',' ))
    {
        (*pnLen)++;
        (*pStr)++;
        goto Cleanup;
    }
    while ( **pStr && !isspace ( **pStr ))
    {
        if (( fBreakOnOpenParenthesis && **pStr == ')' ) ||
            ( fBreakOnCloseParenthesis &&  **pStr == '('  ) ||
            (fBreakOnCommas &&  **pStr == ',' ))
        {
            // Break but leave the pStr pointing at the bracket - we'll pick it up in the
            // next call
            break;
        }

        // Convert curly braces to parens

        if (**pStr == '{')
            **pStr = '(';
        else if (**pStr == '}')
            **pStr = ')';

        (*pnLen)++;
        (*pStr)++;
    }
Cleanup:
    return *pnLen == 0 ? FALSE : TRUE;
}

BOOL CPDLParser::ParseInputFile ( BOOL fDebugging )
{
    BOOL fReturn = TRUE;
    DESCRIPTOR_TYPE nThisDescriptor;
    char szLineBuffer [ MAX_LINE_LEN+1 ];
    char szErrorText [ MAX_LINE_LEN+1 ];
    char *pStr = szLineBuffer;
    char *pElem; int nElemLen;
    TokenDescriptor *pThisDescriptor;
    Token *pNewToken;
    Token *pParentToken = NULL;
    BOOL fGotParentToken = FALSE;
    CString szType;

    fprintf ( fpLOGFile, "Parsing input file\n" );

    for(;;)
    {
        if ( !GetStdInLine ( szLineBuffer, sizeof ( szLineBuffer  ) ) )
            break;

        pStr = szLineBuffer;

        fprintf ( fpLOGFile, "Read Line:%s\n", szLineBuffer );
        // Get the type e..g. enum, eval etc.
        if ( !GetElem ( &pStr, &pElem, &nElemLen ) )
        {
            // Couldn't get the name
            fprintf ( fpLOGFile, "Skipping\n" );
            continue;
        }


        if ( !LookupToken ( pElem, nElemLen, &pThisDescriptor, &nThisDescriptor ) )
        {
            fprintf ( fpLOGFile, "Unknown token\n" );
            continue;
        }

        fprintf ( fpLOGFile, "Processing a %s declaration\n",
            (LPCSTR)AllDescriptors [ nThisDescriptor ] -> szTokenName );

        // If it's a child token and we haven't got a parent
        if ( !pThisDescriptor->fIsParentToken &&
            !fGotParentToken )
        {
            ReportError ( "Child Token Without Parent\n" );
            goto error;
        }

        if ( pThisDescriptor->fIsParentToken )
        {
            fGotParentToken = TRUE;
        }


        INT nTag;
        char *pTag; char *pValue;
        int nTagLen; int nValueLen;
        BOOL fVarArg = FALSE;

        if ( nThisDescriptor == TYPE_METHOD )
        {
            // Look for optional vararg first

            if ( !GetElem ( &pStr, &pElem, &nElemLen ) )
            {
                // Couldn't get the return type
                continue;
            }

            if ( ! ( pNewToken = pParentToken -> AddChildToken ( nThisDescriptor ) ) )
            {
                ReportError ( "Memory Allocation Error\n" );
                goto error;
            }

            if ( !pNewToken->TagValues.AddTag ( METHOD_RETURNTYPE, pElem, nElemLen ) )
            {
                ReportError ( "Memory Allocation Error\n" );
                goto error;
            }

            // Name is next
            if ( !GetElem ( &pStr, &pElem, &nElemLen ) )
            {
                // Couldn't get the name
                continue;
            }

            if ( !pNewToken->TagValues.AddTag ( METHOD_NAME, pElem, nElemLen ) )
            {
                ReportError ( "Memory Allocation Error\n" );
                goto error;
            }

            // Methods need special handling due to the parameter list
            // Put all tokens before the "(" in the TagValues
            // Treat each arg in the param list as a unique token and add
            // to the pArgList for this token
            // Put all the tokens after the ")" in the TagValues
            // We allow either a comma-seperated arg list, but if we
            // don't get commas we break the arg intelligently
            //
            UINT bInParams = FALSE;
            BOOL bCreatedArgToken = FALSE;

            Token *pCurrentToken;

            TokenDescriptor *pDescriptor = pThisDescriptor;
            pCurrentToken = pNewToken;
            fVarArg = FALSE;

            // Set the fBreakOnParenthesis Flag to make parenthis stop GetElem
            while ( GetElem ( &pStr, &pElem, &nElemLen, TRUE, TRUE, TRUE ) )
            {
                if ( nElemLen == 1 && *pElem == '(' )
                {
                    if ( bInParams )
                    {
                        sprintf ( szErrorText, "Syntax Error %s On %s\n", pStr, szLineBuffer );
                        ReportError ( szErrorText );
                        goto error;
                    }
                    bInParams = TRUE;
                    // Switch to method arg descriptor
                    pDescriptor = &MethodArgDescriptor;
                }
                else if ( nElemLen == 1 && *pElem == ')' )
                {
                    if ( !bInParams )
                    {
                        sprintf ( szErrorText, "Syntax Error %s On %s\n", pStr, szLineBuffer );
                        ReportError ( szErrorText );
                        goto error;
                    }
                    bInParams = FALSE;
                    // Switch back to method descriptor
                    pDescriptor = pThisDescriptor;
                    pCurrentToken = pNewToken;
                }
                else if ( nElemLen == 1 && *pElem == ',' )
                {
                    // Reset flag so new arg token gets created for next arg
                    bCreatedArgToken = FALSE;
                }
                else
                {

                    // Split out the prefix:value
                    SplitTag ( pElem, nElemLen, &pTag, &nTagLen,
                        &pValue, &nValueLen );

                    // Match the tag
                    if ( !pDescriptor->LookupTagName ( pTag, nTagLen, &nTag ) )
                    {
                        pTag [ nTagLen ] = '\0';
                        sprintf ( szErrorText, "Unknown tag: %s On %s\n", pTag, szLineBuffer );
                        ReportError ( szErrorText );
                        goto error;
                    }

                    // If we've already got an entry for this tag, and we've seen
                    // at least the arg tag, start a new arg
                    if ( bInParams && pCurrentToken -> IsSet ( METHODARG_ARGNAME ) &&
                        ( nTag == METHODARG_IN || nTag == METHODARG_OUT ) &&
                        ( pCurrentToken -> IsSet ( METHODARG_IN ) ||
                        pCurrentToken -> IsSet ( METHODARG_IN ) ) )
                    {
                        // Start a new arg
                        bCreatedArgToken = FALSE;
                    }
                    if ( bInParams && bCreatedArgToken == FALSE )
                    {
                        // Create the arg list if needed
                        pCurrentToken = pNewToken -> AddChildToken ( TYPE_METHOD_ARG );
                        if ( pCurrentToken == NULL )
                        {
                            ReportError ( "Memory allocation error\n" );
                            goto error;
                        }
                        bCreatedArgToken = TRUE;
                    }

                    // Add the tag either to the main method token array or to the current
                    // arg 's array
                    if ( !pCurrentToken->TagValues.AddTag ( nTag, pValue, nValueLen ) )
                    {
                        ReportError ( "Memory allocation error\n" );
                        goto error;
                    }

                    // Last argument a SAFEARRAY, if so then vararg
                    fVarArg = (strncmp(pValue, "SAFEARRAY(VARIANT)", 18) == 0);
                }

                // method is a vararg because last parameter is a safearray.
                if ( fVarArg && !pNewToken->TagValues.AddTag ( METHOD_VARARG, "vararg", 6 ) )
                {
                    ReportError ( "Memory allocation error\n" );
                    goto error;
                }
            }
        }
        else if ( nThisDescriptor == TYPE_REFPROP ||  nThisDescriptor == TYPE_REFMETHOD )
        {
            // Now get the Class::Name & split it out
            if ( !GetElem ( &pStr, &pElem, &nElemLen ) )
            {
                // Couldn't get the name
                continue;
            }

            // Split out the prefix:value
            SplitTag ( pElem, nElemLen, &pTag, &nTagLen,
                &pValue, &nValueLen );

            pNewToken = pParentToken -> AddChildToken ( nThisDescriptor );

            if ( pNewToken == NULL )
            {
                ReportError ( "Memory Allocation Error\n" );
                goto error;
            }

            if ( !pNewToken->TagValues.AddTag (
                (nThisDescriptor == TYPE_REFPROP) ? (INT)REFPROP_CLASSNAME : (INT)REFMETHOD_CLASSNAME,
                pTag, nTagLen ) )
            {
                ReportError ( "Memory Allocation Error\n" );
                goto error;
            }
            if ( !pNewToken->TagValues.AddTag (
                nThisDescriptor == TYPE_REFPROP ? (INT)REFPROP_PROPERTYNAME : (INT)REFMETHOD_METHODNAME,
                pValue, nValueLen ) )
            {
                ReportError ( "Memory Allocation Error\n" );
                goto error;
            }

        }
        else
        {
            // Now get the name
            if ( !GetElem ( &pStr, &pElem, &nElemLen ) )
            {
                // Couldn't get the name
                continue;
            }

            // If it's a child token, add it to the runtime list, else add it as a child
            // of the current parent
            if ( pThisDescriptor->fIsParentToken )
            {
                pNewToken = pRuntimeList -> AddNewToken ( nThisDescriptor );
                pParentToken = pNewToken;
            }
            else
            {
                pNewToken = pParentToken -> AddChildToken ( nThisDescriptor );
            }

            if ( pNewToken == NULL )
            {
                ReportError ( "Memory Allocation Error\n" );
                goto error;
            }

            // First tag is always the name
            if ( !pNewToken->TagValues.AddTag ( NAME_TAG, pElem, nElemLen ) )
            {
                ReportError ( "Memory Allocation Error\n" );
                goto error;
            }

            // Split out all the token:value pairs
            while ( GetElem ( &pStr, &pElem, &nElemLen ) )
            {
                // Split out the prefix:value
                SplitTag ( pElem, nElemLen, &pTag, &nTagLen,
                    &pValue, &nValueLen );

                // Match the tag
                if ( !pThisDescriptor->LookupTagName ( pTag, nTagLen, &nTag ) )
                {
                    pTag [ nTagLen ] = '\0';
                    sprintf ( szErrorText, "Unknown tag: %s On %s\n", pTag, szLineBuffer );
                    ReportError ( szErrorText );
                    goto error;
                }

                if ( !pNewToken->TagValues.AddTag ( nTag, pValue, nValueLen ) )
                {
                    ReportError ( "Memory Allocation Error\n" );
                    goto error;
                }
            }
        }
        // Perform any cleanup
        switch ( nThisDescriptor )
        {
        case TYPE_EVAL:
            // For enums we calculate and store in the token the enum mask
            // We also number the enum values sequentialy
            pNewToken -> CalculateEnumMask ( pParentToken );
            break;

        case TYPE_ENUM:
            // Add data types to the dynamic type array
            szType = pNewToken -> GetTagValue ( ENUM_NAME );
            AddType ( szType, "Enum" );
            AddEventType ( szType, "VTS_I4" );
            szType += "*";
            AddEventType ( szType, "VTS_PI4" );
            break;

        case TYPE_CLASS:
        case TYPE_INTERFACE:
        case TYPE_EVENT:
            szType = pNewToken -> GetTagValue ( NAME_TAG );
            szType += "*";
            AddType ( szType, "object" );
            AddEventType ( szType, "VTS_DISPATCH" );
            szType += "*";
            AddType ( szType, "object" );
            AddEventType ( szType, "VTS_DISPATCH" );
            break;
            
        }
    }

    // Add an IUnknown to make implicit ref's match up
    pNewToken = pRuntimeList -> AddNewToken ( TYPE_INTERFACE );
    pNewToken->TagValues.AddTag ( INTERFACE_NAME, "IUnknown", -1 );

    // Add an IDispatch to make implicit ref's match up
    pNewToken = pRuntimeList -> AddNewToken ( TYPE_INTERFACE );
    pNewToken->TagValues.AddTag ( INTERFACE_NAME, "IDispatch", -1 );

    pNewToken = pRuntimeList -> AddNewToken ( TYPE_EVENT );
    pNewToken->TagValues.AddTag ( EVENT_NAME, "IDispatch", -1 );

    // Patch up the refprops & refmethods
    if ( !PatchInterfaceRefTypes() )
    {
        goto error;
    }

    // Patch Properties that are of object type
    if ( !PatchPropertyTypes() )
    {
        goto error;
    }

    if ( !PatchInterfaces() )
    {
        goto error;
    }

    goto cleanup;

error:
    fReturn = FALSE;

cleanup:
    return fReturn;
}


BOOL CPDLParser::PatchInterfaceRefTypes ( void )
{
    CTokenListWalker WholeTree ( pRuntimeList );
    Token *pChildToken;
    Token *pInterfaceToken;
    CString szClassName;

    // For each RefProp or Refmethod, we replace the arg list with a copy of the referenced property/method
    // and change the type of the ref'd item appropriatly
    while ( pInterfaceToken = WholeTree.GetNext( TYPE_INTERFACE ) )
    {
        CTokenListWalker ChildList ( pInterfaceToken );
        while ( pChildToken = ChildList.GetNext() )
        {
            if ( pChildToken -> GetType() == TYPE_REFPROP )
            {
                szClassName = pChildToken -> GetTagValue ( REFPROP_CLASSNAME );

                if ( !CloneToken ( pChildToken, TYPE_PROPERTY,
                    REFPROP_CLASSNAME, REFPROP_PROPERTYNAME ) )
                {
                    char szErrorText [ MAX_LINE_LEN+1 ];

                    sprintf ( szErrorText, "Interface %s Invalid RefProp %s:%s\n" ,
                        pInterfaceToken -> GetTagValue ( INTERFACE_NAME ),
                        pChildToken -> GetTagValue ( REFPROP_CLASSNAME ),
                        pChildToken -> GetTagValue ( REFPROP_PROPERTYNAME ) );
                    ReportError ( szErrorText );

                    return FALSE;
                }

                // Remember the class that was refprop'd.
                pChildToken -> AddTag ( PROPERTY_REFDTOCLASS, (LPCSTR)szClassName );
            }
            else if ( pChildToken -> GetType() == TYPE_REFMETHOD )
            {
                szClassName = pChildToken -> GetTagValue ( REFMETHOD_CLASSNAME );

                if ( !CloneToken ( pChildToken, TYPE_METHOD,
                    REFMETHOD_CLASSNAME, REFMETHOD_METHODNAME ) )
                {
                    char szErrorText [ MAX_LINE_LEN+1 ];

                    sprintf ( szErrorText, "Interface %s Invalid RefMethod %s:%s\n" ,
                        pInterfaceToken -> GetTagValue ( INTERFACE_NAME ),
                        pChildToken -> GetTagValue ( REFMETHOD_CLASSNAME ),
                        pChildToken -> GetTagValue ( REFMETHOD_METHODNAME ) );
                    ReportError ( szErrorText );

                    return FALSE;
                }

                // Remember the class that was refprop'd.
                pChildToken -> AddTag ( METHOD_REFDTOCLASS, (LPCSTR)szClassName );
            }
        }
    }
    return TRUE;
}


BOOL CPDLParser::CloneToken ( Token *pChildToken, DESCRIPTOR_TYPE Type, INT nClassName, INT nTagName )
{
    CTokenListWalker SubTree ( pRuntimeList );
    Token *pRefdClassToken;
    Token *pRefdChild;

    pRefdClassToken = SubTree.GetNext( TYPE_CLASS, pChildToken -> GetTagValue ( nClassName ) );
    if ( pRefdClassToken == NULL )
    {
        // Couldn't find the refd class
        return FALSE;
    }
    // Found the refd class, find the refd property
    CTokenListWalker RefdChildList ( pRefdClassToken );
    pRefdChild = RefdChildList.GetNext ( Type,
        pChildToken -> GetTagValue ( nTagName ) );
    if ( pRefdChild == NULL )
    {
        // Couldn't find the refd property
        return FALSE;
    }
    else
    {
        pChildToken -> Clone ( pRefdChild );
    }
    return TRUE;
}



BOOL CPDLParser::GetTypeDetails ( char *szTypeName, CString& szHandler, CString &szFnPrefix,
    StorageType *pStorageType /* = NULL */ )
{
    if ( !LookupType ( szTypeName, szHandler, szFnPrefix, pStorageType ) )
        return FALSE;
    return TRUE;
}

BOOL CPDLParser::IsSpecialProperty(Token *pClassToken)
{
    CString szInterf;
    Token *pInterf;

    szInterf = pClassToken->GetTagValue(CLASS_INTERFACE);
    pInterf = FindInterface(szInterf);
    if (pInterf)
        return (PrimaryTearoff(pInterf) || pInterf->IsSet(INTERFACE_ABSTRACT));
    else
        return TRUE;
}

BOOL CPDLParser::GeneratePropMethodImplementation ( void )
{
    Token *pClassToken;
    Token *pChildToken;
    CString szFnPrefix;
    CString szHandler;
    CString szOffsetOf;
    CString szAType;
    CString szHandlerArgs;
    char szErrorText [ MAX_LINE_LEN+1 ];

    // Only generate def's for this file
    CTokenListWalker TokenList ( pRuntimeList, _pszPDLFileName );

    // Generate propdescs for every property token in every class ( in this file )
    while ( pClassToken = TokenList.GetNext( TYPE_CLASS ) )
    {
        if (!IsSpecialProperty(pClassToken))
        {
            fprintf ( fpHDLFile, "\n//    Property get/set method implementation for class %s\n", (LPCSTR)pClassToken->GetTagValue ( CLASS_NAME ) );
            fprintf ( fpHDLFile, "\n" );

            CTokenListWalker ChildList ( pClassToken );
            while ( pChildToken = ChildList.GetNext() )
            {
                if ( pChildToken->nType == TYPE_METHOD ||
                    pChildToken->nType == TYPE_IMPLEMENTS ||
                    pChildToken -> IsSet ( PROPERTY_ABSTRACT ) ||
                    pChildToken -> IsSet ( PROPERTY_BASEIMPLEMENTATION ) )
                {
                    continue;
                }

                // If propdesc for nameOnly then we wont need a handler for this property
                // another property that matches this will do the handling.
                if ( _stricmp(pChildToken->GetTagValue(PROPERTY_NOPROPDESC), "nameonly") == 0 )
                    continue;

                if ( !GetTypeDetails ( pChildToken->GetTagValue ( PROPERTY_TYPE ),
                    szHandler, szFnPrefix ) )
                {
                    sprintf ( szErrorText ,"Invalid Type:%s in Class:%s Property:%s\n",
                        (LPCSTR)pChildToken->GetTagValue ( PROPERTY_TYPE ),
                        (LPCSTR)pClassToken->GetTagValue(CLASS_NAME),
                        (LPCSTR)pChildToken->GetTagValue ( PROPERTY_NAME ) );
                    ReportError ( szErrorText );
                    return FALSE;
                }

                if ( pChildToken -> IsSet ( PROPERTY_CAA ) )
                {
                    szOffsetOf = "(GetAttrArray())";
                }
                else
                {
                    szOffsetOf = "(this)";
                }
                szAType = pChildToken->GetTagValue ( PROPERTY_ATYPE );
                // Generate set implementation
                if ( pChildToken -> IsSet ( PROPERTY_SET ) )
                {
                    szHandlerArgs = "HANDLEPROP_SET | HANDLEPROP_AUTOMATION | (PROPTYPE_VARIANT << 16)";
                    GenerateMethodImp ( pClassToken, pChildToken, TRUE,
                        szHandler, szHandlerArgs, szOffsetOf, szAType );
                }
                // Generate Get implementation
                if ( pChildToken -> IsSet ( PROPERTY_GET )  )
                {
                    szHandlerArgs = "HANDLEPROP_AUTOMATION | (PROPTYPE_VARIANT << 16)";
                    GenerateMethodImp ( pClassToken, pChildToken, FALSE,
                        szHandler, szHandlerArgs, szOffsetOf, szAType );
                }
            }
        }

        fprintf ( fpHDLFile, "\n" );

        // Whip thru all tearoffs.
        Token  *pLastTearoff = NULL;
        BOOL    fMostDerivedOnly;
        while ((pLastTearoff = NextTearoff((LPCSTR)pClassToken->GetTagValue(CLASS_NAME), pLastTearoff)))
        {
            LPSTR       szInterface = (LPSTR)pLastTearoff->GetTagValue(TEAROFF_INTERFACE);

            // If the tearoff is the primary interface of the coclass then we'll
            // not stack the derived interfaces but generate separate 
            fMostDerivedOnly = _stricmp(szInterface,
                                        (LPSTR)pClassToken->GetTagValue(CLASS_INTERFACE)) == 0;

            if ( !GenerateTearoffTable(pClassToken, pLastTearoff, szInterface, fMostDerivedOnly))
                return FALSE;
        }
    }
    return TRUE;
}


BOOL CPDLParser::FindTearoffMethod(Token *pTearoff, LPCSTR pszTearoffMethod, LPSTR pszUseTearoff)
{
    Token *pChildToken;

    CTokenListWalker ChildList(pTearoff);
    while (pChildToken = ChildList.GetNext())
    {
        if (pChildToken->GetType() == TYPE_TEAROFFMETHOD)
        {
            if (strcmp((LPCSTR)pChildToken->GetTagValue(TEAROFFMETHOD_NAME),
                       pszTearoffMethod) == 0)
            {
				// Check if there is a class name in the method, if it is then
				// extract it explicity.
              	strcpy(pszUseTearoff, (LPCSTR)pChildToken->GetTagValue(TEAROFFMETHOD_MAPTO));

                return TRUE;
            }
        }
    }

    // Call the super:: implementation except for the primary tearoff the base
    // implementation of each method if not specified is the current class.
    strcpy(pszUseTearoff, pTearoff->IsSet(TEAROFF_BASEIMPL) ?
                            (LPCSTR)pTearoff->GetTagValue(TEAROFF_BASEIMPL) : "");
    strcat(pszUseTearoff, pszTearoffMethod);

    return FALSE;
}

// We exploit the Linker's Case insensitive feature to do Tearoffs in Win16.
// This function takes a Tearoff Method and generates a name that is different
// in case from the Real Tearoff Method. We have to follow certain rules so that
// the compiler doesn't complain about undefined symbols and the linker puts in
// the correct address anyhow.
void GenerateWin16TearoffName(LPSTR szWin16Name, LPSTR pszTearoffMethod, LPSTR szClassName = NULL)
{
    strcpy(szWin16Name, pszTearoffMethod);
    // if the Tearoff has a put_ or get_ prefix then we capitalize the PUT_
    // or GET_
    if ( !strncmp(szWin16Name, "put_", 4) )
    {
        strncpy(szWin16Name, "PUT_", 4);
        return;
    }
    if ( !strncmp(szWin16Name, "get_", 4) )
    {
        strncpy(szWin16Name, "GET_", 4);
        return;
    }

    // Check if there is a class name in the method, if it is super
    // then let it be else extract that and copy it to szClassName.
    char *p = strstr(szWin16Name, "::");

    if ( p && strncmp(szWin16Name, "super::",7) )
    {
        strcpy(pszTearoffMethod, p+2);
        if ( szClassName )
        {
            *p = '\0';
            strcpy(szClassName, szWin16Name);
        }
        strcpy(szWin16Name, pszTearoffMethod);
    }

    // lower case the name.
    _strlwr(szWin16Name);
    // make sure the generated name is different from the original one.
    if ( !strcmp( pszTearoffMethod, szWin16Name) )
    {
        char *p = strstr(szWin16Name, "::");

        // Ok, the Method name is all lower case, so we upper case the
        // name after the First char. We also need to skip past the Class Name
        // if any. Upper casing the whole name gave some other problems.
        if ( p != NULL )
            _strupr(p+2);
        else
            _strupr(szWin16Name+1);
    }

}

BOOL CPDLParser::GenerateTearoffTable ( Token *pClassToken, Token *pTearoff, LPCSTR pszInterface, BOOL fMostDerived )
{
    fprintf ( fpHDLFile, "//    Tear-off table for class %s\n",
        pClassToken -> GetTagValue ( CLASS_NAME ) );

    if (IsSpecialTearoff(pTearoff))
    {
        fprintf ( fpHDLFile, "BEGIN_TEAROFF_TABLE_PROPDESC(%s, %s)\n",
            pClassToken -> GetTagValue ( CLASS_NAME ),
            pszInterface );
    }
    else
    {
        fprintf ( fpHDLFile, "BEGIN_TEAROFF_TABLE(%s, %s)\n",
            pClassToken -> GetTagValue ( CLASS_NAME ),
            pszInterface );
    }

    // Walk the interface heirarchy, starting at this classes primary
    // interface, generate a fn table for each interface encountered
    // started with IDispatch methods above. Generate methods in interface order - deepest
    // first

    if ( !GenerateTearOffMethods ( pClassToken -> GetTagValue ( CLASS_NAME ), pTearoff, pszInterface, fMostDerived) )
        return FALSE;

    fprintf ( fpHDLFile, "END_TEAROFF_TABLE()\n\n" );

    return TRUE;
}


Token *CPDLParser::GetSuperClassTokenPtr ( Token *pClassToken )
{
    Token *pSuperToken = NULL;
    if ( pClassToken -> IsSet ( CLASS_SUPER ) )
    {
        CTokenListWalker WholeList ( pRuntimeList );
        pSuperToken = WholeList.GetNext ( TYPE_CLASS,
            pClassToken -> GetTagValue ( CLASS_SUPER ) );
    }
    return pSuperToken;
}

BOOL CPDLParser::HasClassGotProperties ( Token *pClassToken )
{
    Token *pSuperClass;
    CTokenListWalker PropList ( pClassToken );

    if ( PropList.GetNext ( TYPE_PROPERTY ) )
        return TRUE;
    else
    {
        pSuperClass = GetSuperClassTokenPtr ( pClassToken );
        if ( pSuperClass )
            return HasClassGotProperties ( pSuperClass );
        else
            return FALSE;
    }
}


BOOL CPDLParser::PrimaryTearoff (Token *pInterface)
{
    return !pInterface->IsSet(INTERFACE_NOPRIMARYTEAROFF);
}


Token * CPDLParser::NextTearoff (LPCSTR szClassname, Token *pLastTearoff /*= NULL*/)
{
    CTokenListWalker    ThisFilesList(pRuntimeList, _pszPDLFileName);
    Token              *pTearoffToken;
    BOOL                fNextOne = FALSE;

    while (pTearoffToken = ThisFilesList.GetNext(TYPE_TEAROFF))
    {
        if (_stricmp(szClassname, (LPSTR)pTearoffToken->GetTagValue(TEAROFF_NAME)) == 0)
        {
            // The correct class.
            if (pLastTearoff)
            {
                // Return this one.
                if (!fNextOne)
                {
                    fNextOne = pLastTearoff == pTearoffToken;
                    continue;                   // Get the next and then stop.
                }
            }

            break;
        }
    }

    return pTearoffToken;
}


Token* CPDLParser::FindTearoff (LPCSTR szClassname, LPCSTR szInterface)
{
    Token  *pLastTearoff = NULL;

    while (pLastTearoff = NextTearoff(szClassname, pLastTearoff))
    {
        if (!_stricmp(szInterface, (LPSTR)pLastTearoff->GetTagValue(TEAROFF_INTERFACE)))
        {
            break;
        }
    }

    return pLastTearoff;
}


BOOL CPDLParser::GenerateClassIncludes (void)
{
    Token              *pClassToken;
    CTokenListWalker    ThisFilesList(pRuntimeList, _pszPDLFileName);

    // Walk the class statments for this file only
    fprintf(fpLOGFile, "*** Looking for \"%s\"\n",_pszPDLFileName);

    while (pClassToken = ThisFilesList.GetNext(TYPE_CLASS))
    {
        LPCSTR   szClassname = (LPCSTR)pClassToken->GetTagValue(CLASS_NAME);

        fprintf(fpHDLFile, "#ifdef _%s_\n\n", szClassname);

        // Write out inline cached get helpers
        fprintf(fpHDLFile,
                "\n//  Cascaded Property get method prototypes for class %s\n\npublic:\n",
                szClassname);

        GenerateGetAAXPrototypes(pClassToken);

        fprintf(fpHDLFile,
                "\n//    Property get/set method declarations for class %s\n",
                szClassname);

        // Always add a propdesc declaration for non-abstract classes, remember
        // abstract class haven't got a propertydesc array.
        if (!pClassToken->IsSet(CLASS_ABSTRACT))
        {
            fprintf(fpHDLFile,
                    "\npublic:\n    static const PROPERTYDESC * const %s::s_ppropdescs [];\n",
                    szClassname);
            fprintf(fpHDLFile,
                    "    static const VTABLEDESC %s::s_apVTableInterf [];\n",
                    szClassname);
            fprintf(fpHDLFile, "    static HDLDESC %s::s_apHdlDescs;\n",
                    szClassname);
        }

        CString szInterface;
        Token *pInterface;
        BOOL fOk;

        Token *pTearoff = NULL;
        while ((pTearoff = NextTearoff(szClassname, pTearoff)))
        {
            szInterface = pTearoff->GetTagValue(TEAROFF_INTERFACE);
            pInterface = FindInterface(szInterface);

            if (pInterface)
            {
                CTokenListWalker    ChildList(pInterface);
                Token *pChildToken = ChildList.GetNext();
                if (pChildToken)
                {
                    fOk = FALSE;

                    do
                    {
                        if (pChildToken->GetType() == TYPE_PROPERTY && !pChildToken->IsSet(PROPERTY_ABSTRACT) && !pChildToken->IsSet(PROPERTY_BASEIMPLEMENTATION))
                        {
                            fOk = TRUE;
                            break;
                        }
                    } while (pChildToken = ChildList.GetNext());

                    if (fOk)
                    {
                        fprintf(fpHDLFile, "    static const PROPERTYDESC * const %s::s_ppropdescsInVtblOrder%s [];\n",
                            szClassname, (LPSTR)pTearoff->GetTagValue(TEAROFF_INTERFACE));
                    }
                }
            }
        }

        // Generate a CPC if, we have an eventset && it is unique to us
        if ( IsUniqueCPC ( pClassToken ) )
        {
            fprintf(fpHDLFile,
                    "    static const CONNECTION_POINT_INFO %s::s_acpi[];\n",
                    szClassname);
        }

        GeneratePropMethodDecl(pClassToken);

        if (pClassToken->IsSet(CLASS_EVENTS))
        {
            if (!GenerateEventFireDecl(pClassToken))
            {
                return FALSE;
            }
        }

        // Whip thru all tearoffs.
        Token  *pLastTearoff = NULL;
        while ((pLastTearoff = NextTearoff(szClassname, pLastTearoff)))
        {
            // Need static tearoff table decl
            if (IsSpecialTearoff(pLastTearoff))
            {
                fprintf(fpHDLFile, "    DECLARE_TEAROFF_TABLE_PROPDESC(%s)\n",
                        pLastTearoff->GetTagValue(TEAROFF_INTERFACE));
            }
            else
            {
                fprintf(fpHDLFile, "    DECLARE_TEAROFF_TABLE(%s)\n",
                        pLastTearoff->GetTagValue(TEAROFF_INTERFACE));
            }
        }

        GenerateThunkContext(pClassToken);

        fprintf(fpHDLFile, "\n#endif // _%s_\n\n", szClassname);
        fprintf(fpHDLFile, "#undef _%s_\n\n", szClassname);
    }

    return TRUE;
}

// Work out if this class has a unique connection point info structure - 
// or can we use its super ??
BOOL CPDLParser::IsUniqueCPC ( Token *pClassToken )
{
    BOOL                fDoIt = FALSE;
    Token               *pClass;
    CString             szSuperClass, szThisEvents, szThatEvents;

    if ( !pClassToken->IsSet(CLASS_NOCPC) && pClassToken->IsSet ( CLASS_EVENTS ) )
    {
        szThisEvents = pClassToken->GetTagValue(CLASS_EVENTS);
        fDoIt = TRUE;
        for ( pClass = pClassToken ; pClass ; )
        {
            if ( pClass->IsSet ( CLASS_SUPER ) )
            {
                szSuperClass = pClass -> GetTagValue ( CLASS_SUPER );
                pClass = FindClass ( szSuperClass );
                if ( pClass && pClass->IsSet ( CLASS_EVENTS ) && !pClass->IsSet(CLASS_NOCPC) )
                {
                    szThatEvents = pClass->GetTagValue(CLASS_EVENTS);
                    if ( szThatEvents == szThisEvents )
                    {
                        // Do we have non-primary events #1 and is the super non-primary events
                        // the same as ours?  If not, then we are unique.
                        if (pClassToken->IsSet(CLASS_NONPRIMARYEVENTS1))
                        {
                            szThisEvents = pClassToken->GetTagValue(CLASS_NONPRIMARYEVENTS1);
                            szThatEvents = pClass->GetTagValue(CLASS_NONPRIMARYEVENTS1);
                            if (szThatEvents != szThisEvents)
                            {
                                // We're unique.
                                break;
                            }
                            // Do we have non-primary events #2 and is the super non-primary events
                            // the same as ours?  If not, then we are unique.
                            if (pClassToken->IsSet(CLASS_NONPRIMARYEVENTS2))
                            {
                                szThisEvents = pClassToken->GetTagValue(CLASS_NONPRIMARYEVENTS2);
                                szThatEvents = pClass->GetTagValue(CLASS_NONPRIMARYEVENTS2);
                                if (szThatEvents != szThisEvents)
                                {
                                    // We're unique.
                                    break;
                                }
                                // Do we have non-primary events #3 and is the super non-primary events
                                // the same as ours?  If not, then we are unique.
                                if (pClassToken->IsSet(CLASS_NONPRIMARYEVENTS3))
                                {
                                    szThisEvents = pClassToken->GetTagValue(CLASS_NONPRIMARYEVENTS3);
                                    szThatEvents = pClass->GetTagValue(CLASS_NONPRIMARYEVENTS3);
                                    if (szThatEvents != szThisEvents)
                                    {
                                        // We're unique.
                                        break;
                                    }
                                    // Do we have non-primary events #4 and is the super non-primary events
                                    // the same as ours?  If not, then we are unique.
                                    if (pClassToken->IsSet(CLASS_NONPRIMARYEVENTS4))
                                    {
                                        szThisEvents = pClassToken->GetTagValue(CLASS_NONPRIMARYEVENTS4);
                                        szThatEvents = pClass->GetTagValue(CLASS_NONPRIMARYEVENTS4);
                                        if (szThatEvents != szThisEvents)
                                        {
                                            // We're unique.
                                            break;
                                        }
                                    }
                                }
                            }
                            // else fall through to below which set's fDoIt to FALSE.
                        }                        
                        
                        fDoIt = FALSE;
                        break;
                    }
                    break;
                }
            }
            else
                break;
        }
    }
    return fDoIt;
}

BOOL CPDLParser::GenerateEventFireDecl ( Token *pClassToken )
{
    Token *pEventToken;

    fprintf ( fpHDLFile, "//    Event fire method declarations for events %s\n",
        pClassToken -> GetTagValue ( CLASS_EVENTS ) );

    // Find the event declaration
    CTokenListWalker WholeList ( pRuntimeList ) ;
    pEventToken = WholeList.GetNext ( TYPE_EVENT, pClassToken -> GetTagValue ( CLASS_EVENTS ) );
    if ( pEventToken == NULL )
    {
        return FALSE;
    }

    return GenerateEventDecl ( pClassToken, pEventToken );
}

BOOL CPDLParser::GenerateEventDecl ( Token *pClassToken, Token *pEventToken )
{
/* TLL: Don't spit out any arguments.  Currently the only argument for event is eventObject which is computed
    Token *pArgToken;
*/
    Token *pChildToken;
    char szErrorText [ MAX_LINE_LEN+1 ];
    CTokenListWalker ChildList ( pEventToken );
    CString szNameUpper;
    CString szNameLower;

    while ( pChildToken = ChildList.GetNext() )
    {
        if ( pChildToken -> GetType() == TYPE_METHOD &&
            !pChildToken -> IsSet ( METHOD_ABSTRACT ) )
        {
            szNameUpper = pChildToken -> GetTagValue ( METHOD_NAME );
            szNameLower = szNameUpper;
            szNameUpper.ToUpper();

            // Method
            
            fprintf ( fpHDLFile, "    %s Fire_%s(",
                pChildToken->IsSet(METHOD_CANCELABLE) ? "BOOL" : "void",
                pChildToken -> GetTagValue ( METHOD_NAME ) );

            CTokenListWalker ArgListWalker ( pChildToken );
            BOOL fFirst = TRUE;

/* TLL: Don't spit out any arguments.  Currently the only argument for event is eventObject which is computed
            while ( pArgToken = ArgListWalker.GetNext() )
            {
                if ( !fFirst )
                    fprintf ( fpHDLFile, "," );
                fprintf ( fpHDLFile, "%s %s",
                    (LPCSTR)pArgToken -> GetTagValue ( METHODARG_TYPE ),
                    (LPCSTR)pArgToken -> GetTagValue ( METHODARG_ARGNAME ));
                fFirst = FALSE;
            }
*/
            if ( pChildToken->IsSet(METHOD_BUBBLING) )
            {
                if ( !fFirst )
                    fprintf ( fpHDLFile, "," );
                fprintf ( fpHDLFile, "CTreeNode * pNodeContext = NULL");
                fprintf ( fpHDLFile, ", long lSubDivision = -1");
            }

            fprintf ( fpHDLFile, ")\n    {\n        " );


            if ( pChildToken->IsSet(METHOD_CANCELABLE) )
            {
                fprintf ( fpHDLFile, "return " );
            }

            if ( pChildToken->IsSet(METHOD_BUBBLING) )
            {
                fprintf ( fpHDLFile, "Bubble" );
            }
            else
            {
                fprintf ( fpHDLFile, "Fire" );
            }

            if ( pChildToken->IsSet(METHOD_CANCELABLE) )
            {
                fprintf ( fpHDLFile, "Cancelable" );
            }

            fprintf ( fpHDLFile, "Event(%s%sDISPID_EVMETH_%s, DISPID_EVPROP_%s, _T(\"%s\"), (BYTE *)",
                pChildToken->IsSet(METHOD_BUBBLING)
                    ? "pNodeContext, " : "",
                pChildToken->IsSet(METHOD_BUBBLING)
                    ? "lSubDivision, " : "",
                (LPCSTR)szNameUpper,
                (LPCSTR)szNameUpper,
                (LPCSTR)szNameLower+2);  // all event names begin with on*

/* TLL: Don't spit out any arguments.  Currently the only argument for event is eventObject which is computed
            ArgListWalker.Reset();
            if ( ArgListWalker.GetTokenCount() > 0 )
            {
                while ( pArgToken = ArgListWalker.GetNext() )
                {
                    // Locate the vts type for this type
                    CString szVTSType;

                    if ( !LookupEventType ( szVTSType, pArgToken -> GetTagValue ( METHODARG_TYPE ) ) )
                    {
                        sprintf ( szErrorText, "Unknown Type %s in %s::Fire%s event declaration\n",
                            (LPCSTR) pArgToken -> GetTagValue ( METHODARG_TYPE ),
                            pEventToken -> GetTagValue ( EVENT_NAME ),
                            pChildToken -> GetTagValue ( METHOD_NAME ) );
                        ReportError ( szErrorText );
                        return FALSE;
                    }
                    fprintf ( fpHDLFile, " %s", (LPCSTR)szVTSType );
                    fFirst = FALSE;
                }
                ArgListWalker.Reset();
                while ( pArgToken = ArgListWalker.GetNext() )
                {
                    fprintf ( fpHDLFile, ", %s",
                        pArgToken -> GetTagValue ( METHODARG_ARGNAME ) );
                }
            }
            else
*/
            {
                fprintf ( fpHDLFile, " VTS_NONE" );
            }
            fprintf ( fpHDLFile, ");\n    }\n" );
        }
        else if ( pChildToken -> GetType() == TYPE_PROPERTY )
        {
            // Property in event set - should never happen if we
            // check the child/parent relationships at parse time
            sprintf ( szErrorText, "events %s - invalid to have a property in event set\n",
                (LPCSTR)pClassToken -> GetTagValue ( EVENT_NAME ) );
            ReportError ( szErrorText );
            return FALSE;
        }
    }
    return TRUE;
}


LPCSTR CPDLParser::ConvertType(LPCSTR szType)
{
    if (_stricmp(szType, "SAFEARRAY(VARIANT)") == 0)
        return("SAFEARRAY*");
    else
        return(szType);
}

void CPDLParser::GeneratePropMethodDecl ( Token *pClassToken )
{
    CTokenListWalker ChildList ( pClassToken );

    CString szProp;
    Token *pArgToken;
    Token *pChildToken;
    char szContextThunk[] = "ContextThunk_";
    LPCSTR   pCtxt;

    while ( pChildToken = ChildList.GetNext() )
    {
        char    szTemp[256];
        LPSTR   pMethodName;
        BOOL    fNameOnly;

        fNameOnly = _stricmp(pChildToken->GetTagValue(METHOD_NOPROPDESC), "nameonly") == 0;

        if ( pChildToken -> GetType() == TYPE_METHOD  && !fNameOnly )
        {
            // Method
            fprintf ( fpHDLFile, "    " );
            if ( pChildToken -> IsSet ( METHOD_VIRTUAL ) )
            {
                //fprintf ( fpHDLFile, "virtual " );
                fprintf ( fpHDLFile, "DECLARE_TEAROFF_METHOD_(" );
            }
            else
                fprintf ( fpHDLFile, "NV_DECLARE_TEAROFF_METHOD_(" );

           	pMethodName = pChildToken->IsSet(METHOD_SZINTERFACEEXPOSE) ?
							(LPSTR)pChildToken->GetTagValue(METHOD_SZINTERFACEEXPOSE) :
							(LPSTR)pChildToken->GetTagValue(METHOD_NAME);

            GenerateWin16TearoffName(szTemp, pMethodName);

            pCtxt = pChildToken->IsSet(METHOD_THUNKCONTEXT) ||
                    pChildToken->IsSet(METHOD_THUNKNODECONTEXT)
                      ? szContextThunk : "";

            //fprintf ( fpHDLFile, "%s STDMETHODCALLTYPE %s(",
            fprintf ( fpHDLFile, "%s, %s%s, %s, (",
                pChildToken -> GetTagValue ( METHOD_RETURNTYPE ),
                pCtxt, pMethodName, szTemp );

            CTokenListWalker ArgListWalker ( pChildToken );
            BOOL fFirst = TRUE;
            while ( pArgToken = ArgListWalker.GetNext() )
            {
                if ( !fFirst )
                    fprintf ( fpHDLFile, "," );
                fprintf ( fpHDLFile, "%s %s",
                    ConvertType((LPCSTR)pArgToken -> GetTagValue ( METHODARG_TYPE )),
                    (LPCSTR)pArgToken -> GetTagValue ( METHODARG_ARGNAME ) );
                fFirst = FALSE;
            }
            fprintf ( fpHDLFile, "));\n" );
        }
        else
        {
            fNameOnly = _stricmp(pChildToken->GetTagValue(PROPERTY_NOPROPDESC), "nameonly") == 0;

            // Base has provided prototoype
            if ( pChildToken -> IsSet ( PROPERTY_BASEIMPLEMENTATION )
                || (!pChildToken->IsSet(PROPERTY_ABSTRACT) && IsSpecialProperty(pClassToken))
                )
            {
                continue;
            }
            // Property
            szProp = "";
            // Through the index/indextype & index1/indextype1 pdl tags
            // you can provide up to two additional args for the property definition
            if ( pChildToken -> IsSet ( PROPERTY_INDEX ) )
            {
                szProp += pChildToken -> GetTagValue ( PROPERTY_INDEXTYPE );
                szProp += " ";
                szProp += pChildToken -> GetTagValue ( PROPERTY_INDEX );
            }
            if ( pChildToken -> IsSet ( PROPERTY_INDEX1 ) )
            {
                if ( szProp [ 0 ] != '\0' )
                    szProp += ",";
                szProp += pChildToken -> GetTagValue ( PROPERTY_INDEXTYPE1 );
                szProp += " ";
                szProp += pChildToken -> GetTagValue ( PROPERTY_INDEX1 );
            }
            if ( szProp [ 0 ] != '\0' )
                szProp += ",";

           	pMethodName = pChildToken->IsSet(PROPERTY_SZINTERFACEEXPOSE) ?
							(LPSTR)pChildToken->GetTagValue(PROPERTY_SZINTERFACEEXPOSE) :
							(LPSTR)pChildToken->GetTagValue(PROPERTY_NAME);

            pCtxt = pChildToken->IsSet(PROPERTY_THUNKCONTEXT) ||
                    pChildToken->IsSet(PROPERTY_THUNKNODECONTEXT) 
                    ? szContextThunk : "";

            if ( pChildToken -> IsSet ( PROPERTY_SET ) && !fNameOnly )
            {
                fprintf ( fpHDLFile, "    " );
                if ( pChildToken -> IsSet ( PROPERTY_VIRTUAL ) )
                {
                    fprintf ( fpHDLFile, "DECLARE_TEAROFF_METHOD(%sput_%s, PUT_%s, (%s%s v));\n",
                        pCtxt,
                        pMethodName,
                        pMethodName,
                        (LPCSTR)szProp,
                        (LPCSTR)pChildToken -> GetTagValue ( PROPERTY_ATYPE ));
                }
                else
                {
                    fprintf ( fpHDLFile, "NV_DECLARE_TEAROFF_METHOD(%sput_%s, PUT_%s, (%s%s v));\n",
                        pCtxt,
                        pMethodName,
                        pMethodName,
                        (LPCSTR)szProp,
                        (LPCSTR)pChildToken -> GetTagValue ( PROPERTY_ATYPE ));
                }

            }
            if ( pChildToken -> IsSet ( PROPERTY_GET ) && !fNameOnly)
            {
                fprintf ( fpHDLFile, "    " );
                if ( pChildToken -> IsSet ( PROPERTY_VIRTUAL ) )
                {
                    fprintf ( fpHDLFile, "DECLARE_TEAROFF_METHOD(%sget_%s, GET_%s, (%s%s*p));\n",
                        pCtxt,
                        pMethodName,
                        pMethodName,
                        (LPCSTR)szProp,
                        (LPCSTR)pChildToken -> GetTagValue ( PROPERTY_ATYPE ));
                }
                else
                {
                    fprintf ( fpHDLFile, "NV_DECLARE_TEAROFF_METHOD(%sget_%s, GET_%s, (%s%s*p));\n",
                        pCtxt,
                        pMethodName,
                        pMethodName,
                        (LPCSTR)szProp,
                        (LPCSTR)pChildToken -> GetTagValue ( PROPERTY_ATYPE ));
                }
            }
        }
    }

}

BOOL CPDLParser::IsStoredAsString( Token *pChildToken )
{
    CString szJunk1, szJunk2;
    StorageType stHowStored;

    GetTypeDetails ( pChildToken->GetTagValue ( PROPERTY_TYPE ),
        szJunk1, szJunk2, &stHowStored );

    return stHowStored == STORAGETYPE_STRING ? TRUE : FALSE;
}


void CPDLParser::GenerateGetAAXPrototypes ( Token *pClassToken )
{
    CTokenListWalker ChildList ( pClassToken );

    Token *pChildToken;

    if (  pClassToken -> IsSet ( CLASS_NOAAMETHODS ) )
        return;

    while ( pChildToken = ChildList.GetNext() )
    {
        // Generate a GetAA prototype even if the automation Get method is not generated
        if ( pChildToken -> GetType() != TYPE_METHOD &&
             pChildToken -> IsSet ( PROPERTY_CAA ) &&
             _stricmp(pChildToken->GetTagValue(PROPERTY_NOPROPDESC), "nameonly"))
        {
            BOOL fIsString = IsStoredAsString(pChildToken);

            //setaahr method
            if (pChildToken -> IsSet ( PROPERTY_SETAAHR ) )
            {
                fprintf ( fpHDLFile, "    HRESULT SetAA%s(%s);\n",
                    (LPCSTR)pChildToken -> GetTagValue ( PROPERTY_NAME ),
                    (LPCSTR)(fIsString?"LPCTSTR": (LPCSTR)pChildToken -> GetTagValue ( PROPERTY_TYPE ) )
                    );
            }

            //get method. If the property refers to a cascaded format - don't generate a GetAA - this would just
            // encourage mis-use. Always want to force use of GetCascadedXX variant
            if ( !pChildToken -> IsSet ( PROPERTY_CASCADED ) ||
                pChildToken -> IsSet ( PROPERTY_GETAA ) )
            {
                fprintf ( fpHDLFile, "    %s GetAA%s() const;\n",
                    (LPCSTR)(fIsString?"LPCTSTR":(LPCSTR)pChildToken -> GetTagValue ( PROPERTY_TYPE )),
                    (LPCSTR)(pChildToken->GetTagValue ( PROPERTY_NAME )) );
            }
        }
    }
}


void CPDLParser::GenerateGetAAXImplementations( Token *pClassToken )
{
    CTokenListWalker ChildList ( pClassToken );

    Token *pChildToken;

    if (  pClassToken -> IsSet ( CLASS_NOAAMETHODS ) )
        return;

    while ( pChildToken = ChildList.GetNext() )
    {
        // Generate a GetAA prototype even if the automation Get method is not generated
        if ( pChildToken -> GetType() != TYPE_METHOD &&
             pChildToken -> IsSet ( PROPERTY_CAA ) &&
             _stricmp(pChildToken->GetTagValue(PROPERTY_NOPROPDESC), "nameonly"))
        {
            BOOL fIsString = IsStoredAsString(pChildToken);

            //setaahr method
            if (pChildToken -> IsSet ( PROPERTY_SETAAHR ) )
            {
                if (!strcmp((LPCSTR)pChildToken ->GetTagValue(PROPERTY_TYPE), "VARIANT_BOOL"))
                {
                    fprintf ( fpHDLFile, "HRESULT %s::SetAA%s(VARIANT_BOOL pv)\n{\n    DWORD  dwTemp = pv;\n    RRETURN( THR( CAttrArray::SetSimple(GetAttrArray(), &s_propdesc%s%s.a, dwTemp) ) );\n}\n",
                        (LPCSTR)pClassToken -> GetTagValue ( CLASS_NAME ),
                        (LPCSTR)pChildToken -> GetTagValue ( PROPERTY_NAME ),
                        (LPCSTR)pClassToken -> GetTagValue ( CLASS_NAME ),
                        (LPCSTR)pChildToken -> GetTagValue ( PROPERTY_NAME )
                        );
                }
                else
                {
                    fprintf ( fpHDLFile, "HRESULT %s::SetAA%s(%s pv)\n{\n    RRETURN( THR( CAttrArray::Set%s(GetAttrArray(), &s_propdesc%s%s.a, %spv) ) );\n}\n",
                        (LPCSTR)pClassToken -> GetTagValue ( CLASS_NAME ),
                        (LPCSTR)pChildToken -> GetTagValue ( PROPERTY_NAME ),
                        (LPCSTR)( fIsString?"LPCTSTR":(LPCSTR)pChildToken ->GetTagValue(PROPERTY_TYPE) ),
                        (LPCSTR)(fIsString?"String":"Simple"),
                        (LPCSTR)pClassToken -> GetTagValue ( CLASS_NAME ),
                        (LPCSTR)pChildToken -> GetTagValue ( PROPERTY_NAME ),
                        (LPCSTR)(fIsString?"":"*(DWORD*) &")
                        );
                }
            }

            //get method. Don't generate a GetAA - they already have a GetCascadedX
            if ( !pChildToken -> IsSet ( PROPERTY_CASCADED ) ||
                pChildToken -> IsSet ( PROPERTY_GETAA ) )
            {
                Token      *pExposeToken = NULL;
                CString     szClass;

                if (pChildToken -> IsSet ( PROPERTY_NOPROPDESC ))
                {
                    pExposeToken = FindMatchingEntryWOPropDesc(pClassToken, pChildToken);
                }

                if (!pExposeToken)
                    pExposeToken = pChildToken;

                if (strlen( pExposeToken -> GetTagValue ( PROPERTY_REFDTOCLASS )))
                    szClass = pExposeToken -> GetTagValue ( PROPERTY_REFDTOCLASS );
                else
                    szClass = (LPCSTR)pClassToken -> GetTagValue ( CLASS_NAME );

#ifdef UNIX // IEUNIX: On Unix, converting DWORD* to short* would cause value changed. So we cast it from DWORD to short. 
                if (!fIsString)
                {
                    LPCSTR pPropType = (LPCSTR)pChildToken->GetTagValue( PROPERTY_TYPE);
                    if ( !_stricmp(pPropType , "short") || !_stricmp(pPropType, "WORD") || !_stricmp(pPropType, "BYTE") )
                    {
                        fprintf ( fpHDLFile, "%s %s::GetAA%s() const \n{\n    DWORD v;\n    CAttrArray::FindSimple( *GetAttrArray(), &s_propdesc%s%s.a, &v);\n    return (%s)v;\n}\n",
                            pPropType,
                            (LPCSTR)pClassToken -> GetTagValue ( CLASS_NAME ),
                            (LPCSTR)(pChildToken->GetTagValue ( PROPERTY_NAME )),
                            (LPCSTR)szClass,
                            (LPCSTR)pChildToken -> GetTagValue ( PROPERTY_NAME ),
                            pPropType );
                        continue;
                    }
                }
#endif
                fprintf ( fpHDLFile, "%s %s::GetAA%s() const \n{\n    %s v;\n    CAttrArray::Find%s( *GetAttrArray(), &s_propdesc%s%s.a, &v);\n    return *(%s*)&v;\n}\n",
                    (LPCSTR)(fIsString?"LPCTSTR":(LPCSTR)pChildToken -> GetTagValue ( PROPERTY_TYPE )),
                    (LPCSTR)pClassToken -> GetTagValue ( CLASS_NAME ),
                    (LPCSTR)(pChildToken->GetTagValue ( PROPERTY_NAME )) ,
                    (LPCSTR)(fIsString?"LPCTSTR":"DWORD" ),
                    (LPCSTR)(fIsString?"String":"Simple"),
                    (LPCSTR)szClass,
                    (LPCSTR)pChildToken -> GetTagValue ( PROPERTY_NAME ),
                    (LPCSTR)(fIsString?"LPCTSTR":(LPCSTR)pChildToken -> GetTagValue ( PROPERTY_TYPE ) ) );
            }
        }
    }
}

BOOL CPDLParser::GenerateIDispatchTearoff ( LPCSTR szClassName, Token *pTearoff, LPCSTR pszInterface, BOOL fMostDerived)
{
    char    szTearoffMethod[128];
    char    szTemp[128];

    fprintf ( fpHDLFile, "    //  IDispatch methods\n");

    FindTearoffMethod(pTearoff, "GetTypeInfoCount", szTearoffMethod);
    GenerateWin16TearoffName(szTemp, szTearoffMethod);
    fprintf ( fpHDLFile, "    TEAROFF_METHOD(%s, %s, %s, (unsigned int *))\n", 
        szClassName, szTearoffMethod, szTemp);

    FindTearoffMethod(pTearoff, "GetTypeInfo", szTearoffMethod);
    GenerateWin16TearoffName(szTemp, szTearoffMethod);
    fprintf ( fpHDLFile, "    TEAROFF_METHOD(%s, %s, %s, (unsigned int, unsigned long, ITypeInfo **))\n", 
        szClassName, szTearoffMethod, szTemp);

    FindTearoffMethod(pTearoff, "GetIDsOfNames", szTearoffMethod);
    GenerateWin16TearoffName(szTemp, szTearoffMethod);
    fprintf ( fpHDLFile, "    TEAROFF_METHOD(%s, %s, %s, (REFIID, LPOLESTR *, unsigned int, LCID, DISPID *))\n", 
        szClassName, szTearoffMethod, szTemp);

    {
        CString szIHTMLElement, szInterface;
        szIHTMLElement = "IHTMLElement";
        szInterface = pszInterface;

        if(!FindTearoffMethod(pTearoff, "Invoke", szTearoffMethod) &&
           IsSuperInterface(szIHTMLElement, FindInterface(szInterface)))
        {
            strcpy(szTearoffMethod, pTearoff->IsSet(TEAROFF_BASEIMPL) ?
                            (LPCSTR)pTearoff->GetTagValue(TEAROFF_BASEIMPL) : "");
            strcat(szTearoffMethod, "ContextThunk_Invoke");
        }
    }

    GenerateWin16TearoffName(szTemp, szTearoffMethod);
    fprintf ( fpHDLFile, "    TEAROFF_METHOD(%s, %s, %s, (DISPID, REFIID, LCID, WORD, DISPPARAMS *, VARIANT *, EXCEPINFO *, unsigned int *))\n", 
        szClassName, szTearoffMethod, szTemp);

    return TRUE;

}

BOOL CPDLParser::IsSpecialTearoff(Token *pTearoff)
{
    CString szInterface;
    Token *pInterface;
    Token *pChildToken;

    szInterface = pTearoff->GetTagValue(TEAROFF_INTERFACE);
    pInterface = FindInterface(szInterface);
    
    if (!pInterface || pInterface->IsSet(INTERFACE_ABSTRACT))
        return FALSE;

    CTokenListWalker    ChildList(pInterface);
    pChildToken = ChildList.GetNext();

    if (pChildToken)
    {
        do
        {
            if (pChildToken->GetType() == TYPE_PROPERTY && !pChildToken->IsSet(PROPERTY_ABSTRACT) && !pChildToken->IsSet(PROPERTY_BASEIMPLEMENTATION))
            {
                return TRUE;
            }
        } while (pChildToken = ChildList.GetNext());
    }

    return FALSE;
}

BOOL CPDLParser::FindTearoffProperty(Token *pPropertyToken, LPSTR szTearoffMethod, 
                                     LPSTR szTearOffClassName, LPSTR szPropArgs, BOOL fPropGet)
{
    if (pPropertyToken->IsSet(PROPERTY_ABSTRACT) || pPropertyToken->IsSet(PROPERTY_BASEIMPLEMENTATION))
        return FALSE;

    strcpy(szTearoffMethod, fPropGet ? "get_" : "put_");
    strcpy(szPropArgs, fPropGet ? "void" : (LPCSTR)pPropertyToken->GetTagValue(PROPERTY_ATYPE));
    strcpy(szTearOffClassName, "CBase");

    if (!strcmp(pPropertyToken->GetTagValue(PROPERTY_ATYPE), "VARIANT"))
	{
		if (fPropGet)
		{
			strcat(szTearoffMethod, "Property");
		}
		else if (pPropertyToken->IsSet(PROPERTY_DATAEVENT))
	        strcat(szTearoffMethod, "DataEvent");
		else
	        strcat(szTearoffMethod, "Variant");
	}
    else if (!strcmp(pPropertyToken->GetTagValue(PROPERTY_ATYPE), "BSTR"))
    {
        if (!strcmp(pPropertyToken->GetTagValue(PROPERTY_TYPE), "url"))
        {
            strcat(szTearoffMethod, "Url");
            strcpy(szPropArgs, (LPCSTR)pPropertyToken->GetTagValue(PROPERTY_ATYPE));
        }
        else if (!strcmp(pPropertyToken->GetTagValue(PROPERTY_TYPE), "CStyleComponent"))
        {
            strcat(szTearoffMethod, "StyleComponent");
            strcpy(szPropArgs, (LPCSTR)pPropertyToken->GetTagValue(PROPERTY_ATYPE));
        }
        else
            strcat(szTearoffMethod, (fPropGet ? "Property" : "String"));
    }
    else if (!strcmp(pPropertyToken->GetTagValue(PROPERTY_ATYPE), "VARIANT_BOOL"))
        strcat(szTearoffMethod, (fPropGet ? "Property" : "Bool"));
    else if (!strcmp(pPropertyToken->GetTagValue(PROPERTY_ATYPE), "long"))
        strcat(szTearoffMethod, fPropGet ? "Property" : "Long");
    else if (!strcmp(pPropertyToken->GetTagValue(PROPERTY_ATYPE), "short"))
        strcat(szTearoffMethod, fPropGet ? "Property" : "Short");
    else
    {
        char szError[124];
        sprintf(szError, "%s: This property of new type needs a function prototype (in cdbase.hxx) and a body defn. in (baseprop.cxx)",
                (LPCSTR)pPropertyToken->GetTagValue(PROPERTY_NAME));
        ReportError (szError);
        return FALSE;
    }

    return TRUE;
}

BOOL CPDLParser::GeneratePropDescsInVtblOrder(Token *pClassToken, int *pNumVtblPropDescs)
{
    CString szPrimaryInterf;
    CString szInterface;
    LPCSTR  szClassName = pClassToken->GetTagValue(CLASS_NAME);
    BOOL    fOk;
    BOOL    fNameOnly;
    int i = 0, j;

    szPrimaryInterf = pClassToken->GetTagValue(CLASS_INTERFACE);
    Token *pPrimaryInterf = FindInterface(szPrimaryInterf);
    Token *pInterface;
    Token *pChildToken;
    Token *apInterface[5];
    
    *pNumVtblPropDescs = 0;
    Token *pTearoff = NULL;

    while ((pTearoff = NextTearoff(szClassName, pTearoff)))
    {
        szInterface = pTearoff->GetTagValue(TEAROFF_INTERFACE);
        pInterface = FindInterface(szInterface);
        
        if (!pInterface || pInterface->IsSet(INTERFACE_ABSTRACT))
            continue;

        i = 0;
        apInterface[i++] = pInterface;
        while (i < 5 && apInterface[i-1]->IsSet(INTERFACE_SUPER) && 
               _stricmp(apInterface[i-1]->GetTagValue(INTERFACE_SUPER), "IDispatch") &&
               _stricmp(apInterface[i-1]->GetTagValue(INTERFACE_SUPER), "IUnknown"))
        {
            szInterface = apInterface[i-1]->GetTagValue(INTERFACE_SUPER);
            apInterface[i++] = FindInterface(szInterface);
        }

        if (i >= 5)
        {
            ReportError("Super Chain More than 5, Need to increase limit in PdlParser");
            return FALSE;
        }

        fOk = FALSE;

        CTokenListWalker    ChildList(pInterface);
        pChildToken = ChildList.GetNext();

        if (pChildToken)
        {
            do
            {
                if (pChildToken->GetType() == TYPE_PROPERTY && !pChildToken->IsSet(PROPERTY_ABSTRACT) && !pChildToken->IsSet(PROPERTY_BASEIMPLEMENTATION))
                {
                    fOk = TRUE;
                    break;
                }
            } while (pChildToken = ChildList.GetNext());

            if (fOk)
            {
                fprintf(fpHDLFile, "\nconst PROPERTYDESC * const %s::s_ppropdescsInVtblOrder%s[] = {\n", szClassName,
                        (LPSTR)pTearoff->GetTagValue(TEAROFF_INTERFACE));

                for (j = i-1; j >= 0; j--)
                {
                    pInterface = apInterface[j];
                    CTokenListWalker    ChildLst(pInterface);
                    pChildToken = ChildLst.GetNext();

                    do
                    {
                        char    szErrorText[MAX_LINE_LEN + 1];
                        Token  *pExposeToken = NULL;

                        if (pChildToken->GetType() == TYPE_METHOD)
                        {
                            if (pChildToken->IsSet(METHOD_NOPROPDESC))
                            {
                                fNameOnly = _stricmp(pChildToken->GetTagValue(METHOD_NOPROPDESC), "nameonly") == 0;

                                pExposeToken = FindMatchingEntryWOPropDesc(pClassToken, pChildToken, fNameOnly);
                                if (!pExposeToken)
                                {
                                    sprintf(szErrorText,
                                            "Function member marked as nopropdesc can not find exact signature match in new interface:%s in Class:%s\n",
                                            (LPCSTR)pChildToken->GetTagValue(METHOD_NAME),
                                            szClassName);
                                    ReportError(szErrorText);
                                    return FALSE;
                                }
                            }
                            else
                                pExposeToken = pChildToken;

                            fprintf(fpHDLFile, "    (PROPERTYDESC *)&s_methdesc%s%s,\n", pExposeToken->GetTagValue((int)METHOD_REFDTOCLASS), pExposeToken->GetTagValue(METHOD_NAME));
                            if (pPrimaryInterf == pInterface)
                                (*pNumVtblPropDescs)++;
                        }
                        else
                        {
                            if (pChildToken->IsSet(PROPERTY_SET) || pChildToken->IsSet(PROPERTY_GET))
                            {
                                BOOL    fDoAgain = TRUE;

    DoAgain:
                                if (pChildToken->IsSet(PROPERTY_NOPROPDESC))
                                {
                                    fNameOnly = _stricmp(pChildToken->GetTagValue(PROPERTY_NOPROPDESC), "nameonly") == 0;

                                    pExposeToken = FindMatchingEntryWOPropDesc(pClassToken, pChildToken, fNameOnly);
                                    if (!pExposeToken)
                                    {
                                        sprintf(szErrorText,
                                                "Function member marked as nopropdesc can not find exact signature match in new interface:%s in Class:%s\n",
                                                (LPCSTR)pChildToken->GetTagValue(PROPERTY_NAME),
                                                szClassName);
                                        ReportError(szErrorText);
                                        return FALSE;
                                    }
                                }
                                else
                                    pExposeToken = pChildToken;

                                pExposeToken = pExposeToken ? pExposeToken : pChildToken;

                                fprintf(fpHDLFile, "    (PROPERTYDESC *)&s_propdesc%s%s,\n",
                                        pExposeToken->GetTagValue((int)PROPERTY_REFDTOCLASS),
                                        pExposeToken->GetTagValue(PROPERTY_NAME));
                                if (pPrimaryInterf == pInterface)
                                    (*pNumVtblPropDescs)++;

                                // If property is getter and setter both then generate another propdesc (but just once).
                                if (fDoAgain && pChildToken->IsSet(PROPERTY_SET) && pChildToken->IsSet(PROPERTY_GET))
                                {
                                    fDoAgain = FALSE;
                                    goto DoAgain;
                                }
                            }
                        }

                    } while (pChildToken = ChildLst.GetNext());
                }

                fprintf(fpHDLFile, "};\n\n");
            }
        }
    }

    return TRUE;
}

BOOL CPDLParser::GenerateTearOffMethods (LPCSTR szClassName, Token *pTearoff, LPCSTR szInterfaceName, BOOL fMostDerived)
{
    Token              *pInterfaceToken;
    Token              *pChildToken;
    char                szText[MAX_LINE_LEN+1];
    CTokenListWalker    WholeList(pRuntimeList);
#if 0
if (strcmp(_pszPDLFileName, "div.pdl") == 0 || strcmp(szInterfaceName, "IHTMLDivElement") == 0)
__asm { int 3 };
#endif

    // Find the interface decl
    do
    {
        pInterfaceToken = WholeList.GetNext(TYPE_INTERFACE, szInterfaceName);

        if (pInterfaceToken == NULL)
        {
            // if the deepest is IDispatch output that tearoff
            if (_stricmp(szInterfaceName, "IDispatch"))
            {
                // but IUnknowns are OK so return true
                if (_stricmp(szInterfaceName, "IUnknown"))
                {
                    // its something we don't recognise so output error msg
                    sprintf(szText, "interface:%s unknown\n", (LPCSTR)szInterfaceName);
                    ReportError(szText);
                    return FALSE;
                }
                else
                    return TRUE;
            }
            else
            {
                return GenerateIDispatchTearoff(szClassName, pTearoff, szInterfaceName, fMostDerived);
            }
       }
    } while (!pInterfaceToken->IsSet(INTERFACE_GUID));

    // If we only want most derived then we're done otherwise continue recursing.
    if (!fMostDerived)
    {
        // Generate the interfaces super tear off methods first
        if (pInterfaceToken->IsSet(INTERFACE_SUPER))
        {
            GenerateTearOffMethods(szClassName, pTearoff, pInterfaceToken->GetTagValue(INTERFACE_SUPER));
        }
    }
    else
    {
        // fMostDerived is only set for the primary interface, which we don't want stacked ontop of its
        // derivations. so all we want to do is dump in the dispatch and go on.
        GenerateIDispatchTearoff(szClassName, pTearoff, szInterfaceName, fMostDerived);
    }

       // generate the fn prototypes cast to generic fn pts in the
    // tearoff table
    CTokenListWalker    ChildList(pInterfaceToken);
    CString             szProp;
    Token              *pArgToken;

    fprintf ( fpHDLFile, "    //  %s methods\n", szInterfaceName);

    while (pChildToken = ChildList.GetNext())
    {
        char szTemp[256];
        char szTearOffClassName[256];
        char szContextThunk[] = "ContextThunk_";
        LPCSTR   pCtxt;

        if (pChildToken->GetType() == TYPE_METHOD)
        {
            CTokenListWalker    ArgListWalker(pChildToken);
            BOOL                fFirstArg = TRUE;
            char                szTearoffMethod[128];

            FindTearoffMethod(pTearoff,
                              (LPCSTR)pChildToken->GetTagValue(METHOD_NAME),
                              szTearoffMethod);

            strcpy(szTearOffClassName, szClassName);
            GenerateWin16TearoffName(szTemp, szTearoffMethod, szTearOffClassName);


            pCtxt = pChildToken->IsSet(METHOD_THUNKCONTEXT) ||
                    pChildToken->IsSet(METHOD_THUNKNODECONTEXT)
                      ? szContextThunk : "";


            // Method
            fprintf(fpHDLFile, "    TEAROFF_METHOD(%s, %s%s, %s, (", szTearOffClassName, pCtxt, szTearoffMethod, szTemp);

            // All automated methods MUST have an HRESULT for the return type.
            if (_stricmp("HRESULT", (LPCSTR)pChildToken->GetTagValue(METHOD_RETURNTYPE)))
            {
                ReportError("Automated method must have HRESULT for return value\n");
                return FALSE;
            }

            // Output each argument.
            while (pArgToken = ArgListWalker.GetNext())
            {
                if (!fFirstArg)
                    fprintf(fpHDLFile, ",");

                fprintf(fpHDLFile, "%s", ConvertType((LPCSTR)pArgToken->GetTagValue(METHODARG_TYPE)));

                fFirstArg = FALSE;
            }

            fprintf(fpHDLFile, "))\n");
        }
        else
        {
            char    szPropName[128];
            char    szTearoffMethod[128];
            char    szPropArgs[64];
            LPCSTR  pMethodName;

            // Property
            szProp = "";

		    pMethodName = pChildToken->IsSet(PROPERTY_SZINTERFACEEXPOSE) ?
							(LPSTR)pChildToken->GetTagValue(PROPERTY_SZINTERFACEEXPOSE) :
							(LPSTR)pChildToken->GetTagValue(PROPERTY_NAME);

            // Through the index/indextype & index1/indextype1 pdl tags
            // you can provide up to two additional args for the property definition
            pChildToken->AddParam(szProp, PROPERTY_INDEX, pChildToken->GetTagValue(PROPERTY_INDEXTYPE));
            pChildToken->AddParam(szProp, PROPERTY_INDEX1, pChildToken->GetTagValue(PROPERTY_INDEXTYPE1));
            if (szProp[0] != '\0')
                szProp += ",";

            pCtxt = pChildToken->IsSet(PROPERTY_THUNKCONTEXT) ||
                    pChildToken->IsSet(PROPERTY_THUNKNODECONTEXT) 
                    ? szContextThunk : "";

            if ( pChildToken->IsSet(PROPERTY_SET))
            {
                strcpy(szPropName, "put_");
                strcat(szPropName, pMethodName);

                if (FindTearoffMethod(pTearoff, szPropName, szTearoffMethod))
                {
                    strcpy(szPropArgs, (LPCSTR)pChildToken->GetTagValue(PROPERTY_ATYPE));
                    strcpy(szTearOffClassName, szClassName);
                }
                else if (!FindTearoffProperty(pChildToken, szTearoffMethod, szTearOffClassName, szPropArgs, FALSE))
                {
                    FindTearoffMethod(pTearoff, szPropName, szTearoffMethod);
                    strcpy(szPropArgs, (LPCSTR)pChildToken->GetTagValue(PROPERTY_ATYPE));
                    strcpy(szTearOffClassName, szClassName);
                }

                GenerateWin16TearoffName(szTemp, szTearoffMethod, szTearOffClassName);

                fprintf(fpHDLFile, "    TEAROFF_METHOD(%s, %s%s, %s, (%s%s))    // property set_%s\n",
                        szTearOffClassName,
                        pCtxt,
                        szTearoffMethod,
                        szTemp,
                        (LPCSTR)szProp,
                        (LPCSTR)szPropArgs,
                        (LPCSTR)pChildToken->GetTagValue(PROPERTY_NAME));
            }
            if (pChildToken->IsSet(PROPERTY_GET))
            {
                strcpy(szPropName, "get_");
                strcat(szPropName, pMethodName);
                if (FindTearoffMethod(pTearoff, szPropName, szTearoffMethod))
                {
                    strcpy(szPropArgs, (LPCSTR)pChildToken->GetTagValue(PROPERTY_ATYPE));
                    strcpy(szTearOffClassName, szClassName);
                }
                else if (!FindTearoffProperty(pChildToken, szTearoffMethod, szTearOffClassName, szPropArgs, TRUE))
                {
                    FindTearoffMethod(pTearoff, szPropName, szTearoffMethod);
                    strcpy(szPropArgs, (LPCSTR)pChildToken->GetTagValue(PROPERTY_ATYPE));
                    strcpy(szTearOffClassName, szClassName);
                }

                GenerateWin16TearoffName(szTemp, szTearoffMethod, szTearOffClassName);
                fprintf(fpHDLFile, "    TEAROFF_METHOD(%s, %s%s, %s, (%s%s *))    // property get_%s\n",
                        szTearOffClassName,
                        pCtxt,
                        szTearoffMethod,
                        szTemp,
                        (LPCSTR)szProp,
                        (LPCSTR)szPropArgs,
                        (LPCSTR)pChildToken->GetTagValue(PROPERTY_NAME));
            }
        }
    }

    return TRUE;
}

void CPDLParser::GenerateMethodImp ( Token *pClassToken,
    Token *pChildToken, BOOL fIsSet, CString &szHandler,
    CString &szHandlerArgs, CString &szOffsetOf, CString &szAType )
{
#if COLLECT_STATISTICS==1
    // Collect statistics on total property code turds.
    CollectStatistic(NUM_PROPTURDS, GetStatistic(NUM_PROPTURDS) + 1);
#endif

    char *szHandlerMethodPrefix;
    char *szAutomationMethodPrefix;
    if ( fIsSet )
    {
        szHandlerMethodPrefix = "Set";
        szAutomationMethodPrefix = "put_";
    }
    else
    {
        szHandlerMethodPrefix = "Get";
        szAutomationMethodPrefix = "get_";
    }


    // Allow enums with an ATYPE of BSTR to be automated with the string handler
    if ( szHandler == "Enum" && szAType == "BSTR" )
    {
        szHandler = "EnumString";

#if COLLECT_STATISTICS==1
    // Collect statistics on number of enum code turds.
    CollectStatistic(NUM_EMUMTURDS, GetStatistic(NUM_EMUMTURDS) + 1);
#endif
    }

    fprintf ( fpHDLFile,
        "STDMETHODIMP %s::%s%s(%s%s)\n{\n",
        (LPCSTR)pClassToken->GetTagValue ( CLASS_NAME ),
        (LPCSTR)szAutomationMethodPrefix,
        (LPCSTR)pChildToken->GetTagValue ( PROPERTY_NAME ),
        (LPCSTR)szAType,
        fIsSet ? " v" : " * p" );


    if ( pChildToken -> IsSet ( PROPERTY_PRECALLFUNCTION ) )
    {
        CString szfn;
        szfn = pChildToken -> GetTagValue ( PROPERTY_PRECALLFUNCTION ) ;
        // Give the super a chance to reject the call
        fprintf ( fpHDLFile, "    HRESULT hr;\n" );
        if ( szfn == "super" )
        {
            fprintf ( fpHDLFile, "    hr = super::%s%s(%s);\n",
                szAutomationMethodPrefix,
                (LPCSTR)pChildToken->GetTagValue ( PROPERTY_PRECALLFUNCTION ),
                fIsSet ? "v" : "p" );
        }
        else
        {
            fprintf ( fpHDLFile, "    hr = %s%s(%s);\n",
                szAutomationMethodPrefix,
                (LPCSTR)szfn,
                fIsSet ? "v" : "p" );
        }
        fprintf ( fpHDLFile, "    if ( hr )\n        return hr;\n" );
    }

    if ( fIsSet && pChildToken -> IsSet ( PROPERTY_SETDESIGNMODE ) )
    {
        fprintf ( fpHDLFile, "    if ( !IsDesignMode() )\n        return SetErrorInfo(CTL_E_SETNOTSUPPORTEDATRUNTIME);\n" );
    }

    if ( szAType == "VARIANT" )
    {
        fprintf ( fpHDLFile, "    return SetErrorInfo(s_propdesc%s%s.a.Handle%sProperty(%s, %s, this, CVOID_CAST%s));\n}\n",
            (LPCSTR)pClassToken->GetTagValue ( CLASS_NAME ),
            (LPCSTR)pChildToken->GetTagValue ( PROPERTY_NAME ),
            (LPCSTR)szHandler,
            (LPCSTR)szHandlerArgs,
            fIsSet ? "&v" : "p",
            (LPCSTR)szOffsetOf );
    }
    else if ( pChildToken -> IsSet ( PROPERTY_SUBOBJECT ) )
    {
        fprintf ( fpHDLFile, "    return %s::CreateSubObject ( GetElementPtr(), (PROPERTYDESC *)&s_propdesc%s%s,\n        ",
            (LPCSTR)pChildToken->GetTagValue ( PROPERTY_SUBOBJECT ),
            (LPCSTR)pClassToken->GetTagValue ( CLASS_NAME ),
            (LPCSTR)pChildToken->GetTagValue ( PROPERTY_NAME ));
        if ( pChildToken -> IsSet ( PROPERTY_PARAM1 ) )
        {
            fprintf ( fpHDLFile, "%s, ",
                (LPCSTR)pChildToken->GetTagValue ( PROPERTY_PARAM1 ) );
        }

        fprintf ( fpHDLFile, "p );\n}\n" );
    }
    else
    {
        if ( szHandler == "Num" || szHandler == "Enum" )
        {
            fprintf ( fpHDLFile,"    return s_propdesc%s%s.b.%sNumberProperty(%s, this, CVOID_CAST%s);\n}\n",
                (LPCSTR)pClassToken->GetTagValue ( CLASS_NAME ),
                (LPCSTR)pChildToken->GetTagValue ( PROPERTY_NAME ),
                (LPCSTR)szHandlerMethodPrefix,
                fIsSet ? "v" : "p",
                (LPCSTR)szOffsetOf );
        }
        else
        {
            fprintf ( fpHDLFile, "    return s_propdesc%s%s.b.%s%sProperty(%s, this, CVOID_CAST%s);\n}\n",
                (LPCSTR)pClassToken->GetTagValue ( CLASS_NAME ),
                (LPCSTR)pChildToken->GetTagValue ( PROPERTY_NAME ),
                (LPCSTR)szHandlerMethodPrefix,
                (LPCSTR)szHandler,
                fIsSet ? "v" : "p",
                (LPCSTR)szOffsetOf );
        }
    }
}


void CPDLParser::GenerateCPPEnumDefs ( void )
{
    Token *pToken;
    char *pEValText;

    // Only generate def's for this file
    CTokenListWalker TokenList ( pRuntimeList, _pszPDLFileName );

    fprintf ( fpHDLFile, "\n#ifndef _PROPDESCS_EXTERNAL\n\n" );

    // Generate propdescs for every property token in every class ( in this file )
    while ( pToken = TokenList.GetNext( TYPE_ENUM ) )
    {
        fprintf ( fpHDLFile, "EXTERN_C const ENUMDESC s_enumdesc%s = \n{ %u, %u, {\n",
            pToken->GetTagValue ( ENUM_NAME ) ,
            pToken->GetChildTokenCount(),
            pToken->uEnumMask );

        CTokenListWalker ChildList ( pToken );

        while ( pToken = ChildList.GetNext() )
        {
            // If a string is specified use it
            if ( pToken->IsSet ( EVAL_STRING ) )
            {
                pEValText = pToken->GetTagValue ( EVAL_STRING );
            }
            else
            {
                pEValText = pToken->GetTagValue ( EVAL_NAME );
            }
            fprintf ( fpHDLFile, "    { _T(\"%s\"),%s},\n",
                pEValText,
                pToken->GetTagValue ( EVAL_VALUE ) );
        }
        fprintf ( fpHDLFile, "} };\n\n" );
    }

    fprintf ( fpHDLFile, "#endif     // _PROPDESCS_EXTERNAL\n" );
}

void CPDLParser::GenerateInterfaceDISPIDs ( void )
{
    Token *pInterfaceToken;
    Token *pChildToken;
    CString szName;

    // Generate DISPID's for all interface methods that are not
    // ref'd to a class
    // Only generate def's for this file
    CTokenListWalker TokenList ( pRuntimeList, _pszPDLFileName );
    while ( pInterfaceToken = TokenList.GetNext( TYPE_INTERFACE ) )
    {
        szName = pInterfaceToken->GetTagValue ( INTERFACE_NAME );
        if ( szName == "IDispatch" )
            continue;
        fprintf ( fpHDLFile, "//    DISPIDs for class%s\n\n",
            (LPCSTR)pInterfaceToken->GetTagValue ( INTERFACE_NAME ) );
        CTokenListWalker ChildList ( pInterfaceToken );
        while ( pChildToken = ChildList.GetNext() )
        {
            if ( pChildToken -> GetType() == TYPE_METHOD &&
                !strlen( pChildToken -> GetTagValue ( METHOD_REFDTOCLASS ) ) &&
                pChildToken -> IsSet ( METHOD_DISPID ) )
            {
                Token   *pChildMatch = NULL;
                CString  szClassName;
                Token   *pClass;

                szClassName = pInterfaceToken -> GetTagValue( METHOD_REFDTOCLASS );
                pClass = FindClass ( szClassName );

                if (pClass)
                {
                    if (pChildToken->IsSet(METHOD_NOPROPDESC))
                    {
                        pChildMatch = FindMatchingEntryWOPropDesc(pClass, pChildToken);
                    }
                }

                if (!pChildMatch)
                    pChildMatch = pChildToken;

                fprintf ( fpHDLFile, "#define DISPID_%s_%s     %s\n",
                    (LPCSTR)pInterfaceToken->GetTagValue ( INTERFACE_NAME ),
                    (LPCSTR)pChildMatch->GetTagValue ( METHOD_NAME ),
                    (LPCSTR)pChildMatch->GetTagValue ( METHOD_DISPID ) );
            }
        }
    }

}

void CPDLParser::GenerateEventDISPIDs ( FILE *fp, BOOL fPutDIID )
{
    Token *pEventToken;
    Token *pChildToken;
    CString szName,szDISPName;
    int i,nLength;
    BOOL fPutComment;

    // Generate DISPID's for all interface methods that are not
    // ref'd to a class
    // Only generate def's for this file
    CTokenListWalker TokenList ( pRuntimeList, _pszPDLFileName );
    while ( pEventToken = TokenList.GetNext( TYPE_EVENT ) )
    {
        szName = pEventToken->GetTagValue ( EVENT_NAME );
        if ( szName == "IDispatch" || szName == "IUnknown" )
            continue;
        szName.ToUpper();

        fPutComment = FALSE;

        CTokenListWalker ChildList ( pEventToken );
        while ( pChildToken = ChildList.GetNext() )
        {
            if ( pChildToken -> GetType() == TYPE_METHOD &&
                !strlen( pChildToken -> GetTagValue ( METHOD_REFDTOCLASS ) ) &&
                pChildToken -> IsSet ( METHOD_DISPID ) )
            {
                if ( !fPutComment )
                {
                    fprintf ( fp, "//    DISPIDs for event set %s\n\n",
                        (LPCSTR)pEventToken->GetTagValue ( EVENT_NAME ) );
                    fPutComment = TRUE;
                }
                szDISPName = pChildToken->GetTagValue ( METHOD_NAME );
                szDISPName.ToUpper();
                fprintf ( fp, "#define DISPID_%s_%s ",
                    (LPCSTR)szName,
                    (LPCSTR)szDISPName );

                for ( i = 0, nLength = max ( 0, 49-szDISPName.Length()-szName.Length() ); i < nLength ; i++ )
                {
                    fprintf ( fp, " " );
                }

                fprintf ( fp, "%s\n",
                    (LPCSTR)pChildToken->GetTagValue ( METHOD_DISPID ) );
            }
        }
        if ( fPutDIID )
        {
            fprintf ( fp, "\nEXTERN_C const GUID DIID_%s;\n",
                pEventToken->GetTagValue ( EVENT_NAME ));
        }
        if ( fPutComment )
           fprintf ( fp, "\n" );
    }
}

void CPDLParser::GenerateExternalInterfaceDISPIDs ( void )
{
    Token *pInterfaceToken;
    Token *pChildToken;
    CString szName,szDISPName;
    int i,nLength;
    BOOL fPutComment;

    // Generate DISPID's for all interface methods that are not
    // ref'd to a class
    // Only generate def's for this file
    CTokenListWalker TokenList ( pRuntimeList, _pszPDLFileName );
    while ( pInterfaceToken = TokenList.GetNext( TYPE_INTERFACE ) )
    {
        szName = pInterfaceToken->GetTagValue ( INTERFACE_NAME );
        if ( szName == "IDispatch" || szName == "IUnknown" )
            continue;
        szName.ToUpper();

        fPutComment = FALSE;

        CTokenListWalker ChildList ( pInterfaceToken );
        while ( pChildToken = ChildList.GetNext() )
        {
            Token      *pChildMatch = NULL;
            CString     szClassName;
            Token      *pClass;

            if ( !fPutComment )
            {
                fprintf ( fpDISPIDFile, "//    DISPIDs for interface %s\n\n",
                    (LPCSTR)pInterfaceToken->GetTagValue ( INTERFACE_NAME ) );
                fPutComment = TRUE;
            }
            if ( pChildToken -> GetType() == TYPE_METHOD )
            {
                szDISPName = pChildToken->GetTagValue ( METHOD_NAME );

                szClassName = pChildToken -> GetTagValue( METHOD_REFDTOCLASS );
                pClass = FindClass ( szClassName );
                if (pClass)
                {
                    if (pChildToken->IsSet(METHOD_NOPROPDESC))
                    {
                        if (pChildToken->IsSet(METHOD_SZINTERFACEEXPOSE))
                            szDISPName = pChildToken->GetTagValue(METHOD_SZINTERFACEEXPOSE);

                        pChildMatch = FindMatchingEntryWOPropDesc(pClass, pChildToken);
                    }
                }

                if (!pChildMatch)
                    pChildMatch = pChildToken;
            }
            else
            {
                // Property

                szDISPName = pChildToken->GetTagValue ( PROPERTY_NAME );

                szClassName = pChildToken -> GetTagValue( PROPERTY_REFDTOCLASS );
                pClass = FindClass ( szClassName );
                if (pClass)
                {
                    if (pChildToken->IsSet(PROPERTY_NOPROPDESC))
                    {
                        if (pChildToken->IsSet(PROPERTY_SZINTERFACEEXPOSE))
                            szDISPName = pChildToken->GetTagValue(PROPERTY_SZINTERFACEEXPOSE);

                        pChildMatch = FindMatchingEntryWOPropDesc(pClass, pChildToken);
                    }
                }

                if (!pChildMatch)
                    pChildMatch = pChildToken;
            }
            szDISPName.ToUpper();
            
            fprintf ( fpDISPIDFile, "#define DISPID_%s_%s ",
                (LPCSTR)szName,
                (LPCSTR)szDISPName );

            for ( i = 0, nLength = max ( 0, 49-szDISPName.Length()-szName.Length() ); i < nLength ; i++ )
            {
                fprintf ( fpDISPIDFile, " " );
            }

            fprintf ( fpDISPIDFile, "%s\n",
                (LPCSTR)pChildMatch->GetTagValue ( PROPERTY_DISPID ) );
        }
        if ( fPutComment )
            fprintf ( fpDISPIDFile, "\n" );
    }
}



void CPDLParser::GenerateClassDISPIDs ( void )
{
    Token *pClassToken;
    Token *pChildToken;
    CString szCoClassName;

    // Only generate def's for this file
    CTokenListWalker TokenList ( pRuntimeList, _pszPDLFileName );

    // Generate propdescs for every property token in every class ( in this file )
    while ( pClassToken = TokenList.GetNext( TYPE_CLASS ) )
    {
        fprintf ( fpHDLFile, "//    DISPIDs for class %s\n\n", (LPCSTR)pClassToken->GetTagValue ( CLASS_NAME ) );

        if ( pClassToken -> IsSet ( CLASS_GUID ) )
        {
            pClassToken -> GetTagValueOrDefault ( szCoClassName,
                CLASS_COCLASSNAME, pClassToken -> GetTagValue ( CLASS_NAME ) );
            fprintf ( fpHDLFile, "EXTERN_C const GUID CLSID_%s;\n",
                (LPCSTR)szCoClassName );
        }
        CTokenListWalker ChildList ( pClassToken );
        while ( pChildToken = ChildList.GetNext() )
        {
            Token *pChildMatch = pChildToken;

            if ( pChildMatch->nType == TYPE_PROPERTY && pChildMatch->IsSet ( PROPERTY_DISPID ) )
            {
                if (pChildMatch->IsSet(PROPERTY_NOPROPDESC))
                {
                    pChildMatch = FindMatchingEntryWOPropDesc(pClassToken, pChildMatch);
                    if (!pChildMatch)
                        pChildMatch = pChildToken;
                }

                fprintf ( fpHDLFile, "#define DISPID_%s_%s     %s\n",
                    (LPCSTR)pClassToken->GetTagValue ( CLASS_NAME ),
                    (LPCSTR)pChildMatch->GetTagValue ( PROPERTY_NAME ),
                    (LPCSTR)pChildMatch->GetTagValue ( PROPERTY_DISPID ) );

            }
            else if ( pChildMatch->nType == TYPE_METHOD && pChildMatch->IsSet ( METHOD_DISPID ) )
            {
                if (pChildMatch->IsSet(METHOD_NOPROPDESC))
                {
                    pChildMatch = FindMatchingEntryWOPropDesc(pClassToken, pChildMatch);
                    if (!pChildMatch)
                        pChildMatch = pChildToken;
                }

                fprintf ( fpHDLFile, "#define DISPID_%s_%s     %s\n",
                    (LPCSTR)pClassToken->GetTagValue ( CLASS_NAME ),
                    (LPCSTR)pChildMatch->GetTagValue ( METHOD_NAME ),
                    (LPCSTR)pChildMatch->GetTagValue ( METHOD_DISPID ) );
            }
        }
    }
}

void CPDLParser::GeneratePropdescExtern ( Token *pClassToken, BOOL fRecurse )
{
    Token *pChild;
    Token *pSuperClassToken;
    CTokenListWalker ChildList ( pClassToken );

    // Add the supers definitions first.  Recurse to the super of this super!!
    // Notice that the fRecurse if used by abstract classes to only extern the
    // PropertyDesc which are generated for the actual class.
    pSuperClassToken = fRecurse ? GetSuperClassTokenPtr ( pClassToken ) : NULL;
    if ( pSuperClassToken )
        GeneratePropdescExtern ( pSuperClassToken );

    // Don't generate any EXTERN_C for abstract class reference.  The EXTERN_C
    // is only in the hdl that contains both the PROPERTYDESC the EXTERN_C for
    // the abstract class (e.g., bodyroot.hdl).
    if ( fRecurse && pClassToken -> IsSet ( CLASS_ABSTRACT ) )
        return;

    fprintf ( fpHeaderFile, "\n#ifndef _%s_PROPDESCS_\n", pClassToken -> GetTagValue ( CLASS_NAME ) );
    // Walk the super class propdescs looking for properties

    while ( pChild = ChildList.GetNext() )
    {
        if ( pChild->GetType() == TYPE_PROPERTY )
        {
            StorageType stHowStored;
            CString szHandler;
            CString szFnPrefix;


            szHandler = "";
            szFnPrefix = "";
            // Check for missing handler later
            GetTypeDetails ( pChild->GetTagValue ( PROPERTY_TYPE ),
                szHandler, szFnPrefix, &stHowStored );

            if ( !ComputePROPDESC_STRUCT ( fpHeaderFile, pClassToken, pChild, szHandler, szFnPrefix ) )
                return;       // Bad error reported...leave

            fprintf ( fpHeaderFile, " s_propdesc%s%s;\n",
                pClassToken -> GetTagValue ( CLASS_NAME ),
                pChild -> GetTagValue ( PROPERTY_NAME ) );
        }
        else if ( pChild->GetType() == TYPE_METHOD )
        {
            fprintf ( fpHeaderFile, "EXTERN_C const PROPERTYDESC_METHOD s_methdesc%s%s;\n",
                pClassToken -> GetTagValue ( CLASS_NAME ),
                pChild -> GetTagValue ( METHOD_NAME ) );
        }
    }

    fprintf ( fpHeaderFile, "\n#endif\n" );
}

BOOL CPDLParser::GeneratePropdescReference ( Token *pClassToken, BOOL fDerivedClass, PropdescInfo *pPI, int *pnPI )
{
    Token          *pChild;
    char           szErrorText [ MAX_LINE_LEN+1 ];


    if (!pClassToken)
        return TRUE;

    CTokenListWalker ChildList ( pClassToken );

    // Add the supers definitions first
    // Recurse to the super of this super!!
    if ( !GeneratePropdescReference ( GetSuperClassTokenPtr ( pClassToken ), FALSE, pPI, pnPI ) )
    {
        return FALSE;
    }

    if (fDerivedClass)
    {
        ChildList.Reset();
    }

    // Walk the super class propdescs looking for properties
    while ( pChild = ChildList.GetNext() )
    {
        if ( pChild->GetType() == TYPE_PROPERTY )
        {
            if ( !pChild->IsSet ( PROPERTY_INTERNAL ) && !pChild->IsSet ( PROPERTY_ABSTRACT ) &&
                     !pChild->IsSet( PROPERTY_NOPROPDESC ) &&
                    (!pChild->IsSet ( PROPERTY_NOPERSIST ) || 
                        ( pChild->IsSet ( PROPERTY_NOPERSIST ) && pClassToken->IsSet( CLASS_KEEPNOPERSIST ) ) ) )
            {
                pPI[*pnPI].Set(pClassToken -> GetTagValue ( CLASS_NAME ),            
                               pChild -> GetTagValue ( PROPERTY_NAME )  ,
                               fDerivedClass,
                               pChild->GetTagValue ( PROPERTY_SZATTRIBUTE ) );
                *pnPI += 1;
            }
        }
    }

    // Print the array
    if (fDerivedClass)
    {
        int     x;

        SortPropDescInfo(pPI, *pnPI);

        //print out all properties.
        
        for (x=0;x<*pnPI;++x)
        {
            // Error if two properties have the same name
            if ( x > 0 && _strcmpi ( pPI[x]._szSortKey, pPI[x-1]._szSortKey ) == 0 )
            {
                sprintf ( szErrorText,
                    "Duplicate property name %s::%s\n",
                    pClassToken -> GetTagValue ( CLASS_NAME ),
                    pPI[x]._szSortKey );
                ReportError ( szErrorText );
                return FALSE;
            }
            // Property in the propertydesc array
            fprintf ( fpHDLFile, "    (PROPERTYDESC *)&s_propdesc%s%s%s, // %s\n",
                pPI[x]._szClass,
                pPI[x]._szPropName,
                pPI[x]._fAppendA ? ".a" : "",
                pPI[x]._szSortKey );
        }
    }
    return TRUE;
}

BOOL CPDLParser::GeneratePROPDESCArray ( Token *pThisClassToken, int *pNumPropDescs )
{
    PropdescInfo rgPI[120];
    int nPI;

    *pNumPropDescs = 0;

    // Abstract classes have no PROPERTYDESC array, this is to support classes
    // like C2DBodyElement and CBodyElement which want the PROPRTYDESC from
    // CBodyRoot but nothing else.
    if ( !pThisClassToken -> IsSet ( CLASS_ABSTRACT ) )
    {
        fprintf ( fpHDLFile, "\nconst PROPERTYDESC * const %s::s_ppropdescs[] = {\n",
           pThisClassToken -> GetTagValue ( CLASS_NAME ) );

        nPI = 0;

        if ( !GeneratePropdescReference ( pThisClassToken, TRUE, rgPI, &nPI ) )
        {
            return FALSE;
        }

        fprintf ( fpHDLFile, "    NULL\n};\n" );

        // Total number of propdesc generated.
        *pNumPropDescs = nPI;
    }
    if ( IsUniqueCPC ( pThisClassToken ) )
    {
        fprintf ( fpHDLFile, "\nconst CONNECTION_POINT_INFO %s::s_acpi[] = {\n",
           pThisClassToken -> GetTagValue ( CLASS_NAME ) );

        fprintf ( fpHDLFile, "    CPI_ENTRY(IID_IPropertyNotifySink, DISPID_A_PROPNOTIFYSINK)\n" );
        fprintf ( fpHDLFile, "    CPI_ENTRY(DIID_%s, DISPID_A_EVENTSINK)\n",
            pThisClassToken->GetTagValue( CLASS_EVENTS ) );
        if (pThisClassToken->IsSet(CLASS_NONPRIMARYEVENTS1))
        {
            fprintf(fpHDLFile, "    CPI_ENTRY(DIID_%s, DISPID_A_EVENTSINK)\n",
                    pThisClassToken->GetTagValue(CLASS_NONPRIMARYEVENTS1));
        }
        if (pThisClassToken->IsSet(CLASS_NONPRIMARYEVENTS2))
        {
            fprintf(fpHDLFile, "    CPI_ENTRY(DIID_%s, DISPID_A_EVENTSINK)\n",
                    pThisClassToken->GetTagValue(CLASS_NONPRIMARYEVENTS2));
        }
        if (pThisClassToken->IsSet(CLASS_NONPRIMARYEVENTS3))
        {
            fprintf(fpHDLFile, "    CPI_ENTRY(DIID_%s, DISPID_A_EVENTSINK)\n",
                    pThisClassToken->GetTagValue(CLASS_NONPRIMARYEVENTS3));
        }
        if (pThisClassToken->IsSet(CLASS_NONPRIMARYEVENTS4))
        {
            fprintf(fpHDLFile, "    CPI_ENTRY(DIID_%s, DISPID_A_EVENTSINK)\n",
                    pThisClassToken->GetTagValue(CLASS_NONPRIMARYEVENTS4));
        }
        fprintf ( fpHDLFile, "    CPI_ENTRY(IID_ITridentEventSink, DISPID_A_EVENTSINK)\n" );
        fprintf ( fpHDLFile, "    CPI_ENTRY(IID_IDispatch, DISPID_A_EVENTSINK)\n" );
        fprintf ( fpHDLFile, "    CPI_ENTRY_NULL\n};\n" );
    }

    return TRUE;
}

void CPDLParser::GenerateVTableArray ( Token *pThisClassToken, int *pNumVTblEntries )
{
    Token          *pDerivedClass;
    PropdescInfo    rgPI[400];          // Max. number of method/prop for interface.
    int             nPI = 0;
    UINT            uVTblIndex = 0;
    CString         szInterf;
    Token          *pInterf;

    *pNumVTblEntries = 0;

    // Abstract classes don't need the vtable.
    if ( pThisClassToken->IsSet ( CLASS_ABSTRACT ) )
        return;

    pDerivedClass = pThisClassToken;

GetInterf:
    szInterf = pDerivedClass->GetTagValue(CLASS_INTERFACE);
    // If no interface then use the super to find an interface.
    if (!szInterf.Length())
    {
        CString     szClass;

        szClass = pDerivedClass -> GetTagValue ( CLASS_SUPER );
        pDerivedClass = FindClass(szClass);
        if (!pDerivedClass)
            ReportError ( "Unknown class\n" );

        goto GetInterf;
    }

    pInterf = FindInterface(szInterf);
    if (pInterf || strcmp((LPCSTR)szInterf, "IDispatch") == 0)
    {
        fprintf ( fpHDLFile, "\nconst VTABLEDESC %s::s_apVTableInterf[] = {\n",
           pThisClassToken -> GetTagValue ( CLASS_NAME ) );

        if (pInterf)
            ComputeVTable( pThisClassToken, pInterf, FALSE, rgPI, &nPI, &uVTblIndex );

        fprintf ( fpHDLFile, "    {NULL, 0}\n};\n" );

        // Number of entries in vtable array.
        *pNumVTblEntries = nPI;
    }
    else
    {
        char szErrorText [ MAX_LINE_LEN+1 ];

        sprintf( szErrorText, "Unknown interface %s\n", (LPCSTR) szInterf );
        ReportError ( szErrorText );
    }
}


void
CPDLParser::SortPropDescInfo (PropdescInfo *pPI, int cPDI)
{
    int             x, y, cc;        
    PropdescInfo    pd;

    //sort em
    for (x = 0; x < cPDI - 1; ++x)
    {
        for (y = x; y < cPDI; ++y)
        {
            cc = _stricmp( pPI[x]._szSortKey, pPI[y]._szSortKey );  // API caveat
            if (cc > 0)
            {
                memcpy(&pd, pPI + x, sizeof(pd));
                memcpy(pPI + x, pPI + y, sizeof(pd));
                memcpy(pPI + y, &pd, sizeof(pd));
            }
        }
    }
}


BOOL
CPDLParser::ComputeVTable ( Token *pClass, Token *pInterface, BOOL fDerived, PropdescInfo *pPI, int *piPI, UINT *pUVTblIdx, BOOL fPrimaryTearoff/*= FALSE*/ )
{
    CString         szSuperInterf;
    Token          *pChildToken;
    int             cFuncs = 0;
    BOOL            fProperty = FALSE;
    int             idxIID = -1;

    // Compute if this interface on the class is a primary interface tearoff.
    if (pClass)
    {
        Token      *pTearoff;

        pTearoff = FindTearoff(pClass->GetTagValue(CLASS_NAME), pInterface->GetTagValue(INTERFACE_NAME));

        if (pTearoff)
        {
            CString     szInterface;
            Token      *pInterfaceToken;

            szInterface = pTearoff->GetTagValue(TEAROFF_INTERFACE);
            pInterfaceToken = FindInterface(szInterface);

            // If primary interface is not derived from IDispatch then this
            // interface and all derived interfaces (vtable layout) are separated
            // not concatenated.
            LPSTR szSuperPrimaryInterf = pInterfaceToken->GetTagValue(INTERFACE_SUPER);

            if (szSuperPrimaryInterf && *szSuperPrimaryInterf)
                fPrimaryTearoff = (_stricmp(szSuperPrimaryInterf, "IDispatch"));
            else
			{
                fPrimaryTearoff = FALSE;
			}
        }
    }

    if (fPrimaryTearoff)
    {
        CString     szInterface;

        szInterface = pInterface->GetTagValue(INTERFACE_NAME);
        idxIID = FindAndAddIIDs(szInterface);
        if (idxIID == -1)
        {
            ReportError("Problem with IID adding.\n");
            return FALSE;
        }
    }


    szSuperInterf = pInterface->GetTagValue(INTERFACE_SUPER);
    if (szSuperInterf && *szSuperInterf && szSuperInterf != "IDispatch")
    {
        Token  *pSuperInterf;

        pSuperInterf = FindInterface(szSuperInterf);
        if (!pSuperInterf)
            return FALSE;

        if (!ComputeVTable(NULL, pSuperInterf, TRUE, pPI, piPI, pUVTblIdx, fPrimaryTearoff))
            return FALSE;
    }

    CTokenListWalker ChildList ( pInterface );
    while ( pChildToken = ChildList.GetNext() )
    {
        UINT    uIIDnVTbl;

        cFuncs = 0;

        if ( pChildToken -> GetType() == TYPE_PROPERTY )
        {
            fProperty = TRUE;
            cFuncs = pChildToken -> IsSet ( PROPERTY_GET ) ? 1 : 0;
            cFuncs += (pChildToken -> IsSet ( PROPERTY_SET ) ? 1 : 0);
        
            if (pChildToken->IsSet(PROPERTY_NOPROPDESC))
                goto AddInVTable;
        }
        else if ( pChildToken -> GetType() == TYPE_METHOD )
        {
            fProperty = FALSE;
            cFuncs = 1;

            if (pChildToken->IsSet(METHOD_NOPROPDESC))
                goto AddInVTable;
        }
        else
        {
            ReportError ( "Unknown token type in ComputeVTable.\n" );
            return FALSE;
        }

        // Compute vtable offset.  If primary interface is a tearoff then set
        // the offset and the idx into the IID table.  Note, all indexes start
        // at 1 (zero is reserved to imply classdesc primary interface).
        uIIDnVTbl = fPrimaryTearoff ? (((idxIID + 1) << 8) | *pUVTblIdx) : *pUVTblIdx;

        pPI[*piPI].SetVTable(pChildToken -> GetTagValue ( fProperty ? (int)PROPERTY_REFDTOCLASS : (int)METHOD_REFDTOCLASS ),            
                       pChildToken -> GetTagValue ( fProperty ? (int)PROPERTY_NAME : (int)METHOD_NAME ),
                       FALSE,     // Unused.
                       pChildToken -> GetTagValue ( PROPERTY_SZATTRIBUTE ),
                       uIIDnVTbl,
                       fProperty);

        (*piPI)++;

AddInVTable:
        (*pUVTblIdx) += cFuncs;
    }

    // Each derived interface of the primary tearoff is separated not a straight
    // derivation.
    if (fPrimaryTearoff)
        *pUVTblIdx = 0;

    // Top most interface?
    if ( !fDerived )
    {
        // Now check for any other interfaces mentioned in the as implements in
        // the class it is exposed in the coclass, that interface is a
        // separately supported interface.

        CTokenListWalker    ChildWalker(pClass);
        Token              *pChildToken;
        CString             szInterface;
        Token              *pInterfToken;

        while (pChildToken = ChildWalker.GetNext())
        {
            if (pChildToken->GetType() == TYPE_IMPLEMENTS)
            {
                *pUVTblIdx = 0;         // implements always starts at 0 vtable offset.

                szInterface = pChildToken->GetTagValue(IMPLEMENTS_NAME);
                if (_stricmp(pInterface->GetTagValue(INTERFACE_NAME), szInterface) == 0)
                    continue;

                pInterfToken = FindInterface(szInterface);
                if (pInterfToken)
                {
                    if (!ComputeVTable(NULL, pInterfToken, TRUE, pPI, piPI, pUVTblIdx, TRUE))
                        return FALSE;
                }
            }
        }

        // Yes, we've got the entire inheritence chain now sort and spit.
        SortPropDescInfo (pPI, *piPI);

        // Output the vtable interface sorted by name.
        for (int iVTbl = 0; iVTbl < *piPI; iVTbl++)
        {
            fprintf( fpHDLFile, "    {(PROPERTYDESC *)&s_%sdesc%s%s, 0x%x},        // %s %s\n",
                     pPI[iVTbl]._fProperty ? "prop" : "meth",
                     pPI[iVTbl]._szClass,
                     pPI[iVTbl]._szPropName,
                     pPI[iVTbl]._uVTblIndex,
                     pPI[iVTbl]._fProperty ? "Property" : "Method",
                     pPI[iVTbl]._szSortKey);
        }
    }

    return TRUE;
}


BOOL
CPDLParser::ComputePROPDESC_STRUCT ( FILE *fp, Token *pClassToken, Token *pChild, CString & szHandler, CString & szFnPrefix )
{
    char szErrorText [ MAX_LINE_LEN+1 ];

    fprintf ( fp, "EXTERN_C const " );

    if ( !pChild->IsSet ( PROPERTY_ABSTRACT ) && szHandler == "Num" || szHandler == "Enum"  || szHandler == "UnitValue" )
    {
        // Numeric Handlers
        if ( pChild->IsSet ( PROPERTY_GETSETMETHODS ) )
        {
             fprintf ( fp, "PROPERTYDESC_NUMPROP_GETSET" );
        }
        else if ( pChild->IsSet ( PROPERTY_ABSTRACT ) )
        {
            fprintf ( fp, "PROPERTYDESC_NUMPROP_ABSTRACT" );
        }
        else if ( pChild->IsSet ( PROPERTY_ENUMREF ) )
        {
            fprintf ( fp, "PROPERTYDESC_NUMPROP_ENUMREF" );
        }
        else
        {
            fprintf ( fp, "PROPERTYDESC_NUMPROP" );
        }
    }
    else
    {
        // BASIC PROP PARAM Structure
        if ( pChild->IsSet ( PROPERTY_GETSETMETHODS ) )
        {
            if ( szFnPrefix == "" )
            {
                sprintf ( szErrorText, "Invalid Type:%s in Class:%s Property:%s\n",
                    (LPCSTR)pChild->GetTagValue ( PROPERTY_TYPE ),
                    (LPCSTR)pClassToken->GetTagValue(CLASS_NAME),
                    (LPCSTR)pChild->GetTagValue ( PROPERTY_NAME ) );
                ReportError ( szErrorText );
                return FALSE;
            }
            fprintf ( fp, "PROPERTYDESC_%s_GETSET", (LPCSTR)szFnPrefix );
        }
        else if ( pChild->IsSet ( PROPERTY_ABSTRACT ) )
        {
            fprintf ( fp, "PROPERTYDESC_BASIC_ABSTRACT" );
        }
        else
        {
            fprintf ( fp, "PROPERTYDESC_BASIC" );
        }
    }

    return TRUE;
}


Token * CPDLParser::FindEnum ( Token *pChild )
{
    // Get the enum mask from the enum named by  PROPERTY_TYPE
    CTokenListWalker WholeList ( pRuntimeList );

    return WholeList.GetNext ( TYPE_ENUM, pChild->GetTagValue ( PROPERTY_TYPE ) );
}


char * CPDLParser::MapTypeToIDispatch ( CString & szType )
{
    CString szHandler;
    CString szFnPrefix;
    char    szStrType[255];

    strcpy(szStrType, (LPCSTR)szType);
    if (GetTypeDetails (szStrType, szHandler, szFnPrefix, NULL) && (szHandler == "object"))
    {
        int cSz = szType.Length();

        if (szType[cSz - 1] == '*' && szType[cSz - 2] == '*')
        {
            // is it an IUnknown or an IDispatch?
            if (_strnicmp(szType, "IUnknown", cSz-2)==0)
                return "IUnknownpp";
            else
                return "IDispatchpp";
        }
        else if (szType[cSz - 1] == '*')
        {
            // is it an IUnknown or an IDispatch?
            if (_strnicmp(szType, "IUnknown",cSz-1)==0)
                return "IUnknownp";
            else
                return "IDispatchp";
        }
    }

    return NULL;
}


BOOL CPDLParser::ComputeProperty ( Token *pClassToken, Token *pChild )
{
    CString                     szClass;
    CString                     szHandler;
    CString                     szFnPrefix;
    CString                     szHTMLName;
    char                        szExposedName [ MAX_LINE_LEN+1 ];
    CString                     szdwFlags;
    CString                     szNotPresentDefault;
    CString                     szNotAssignedDefault;
    CString                     szUpperName;
    CString                     szDispid;
    char                        szErrorText [ MAX_LINE_LEN+1 ];
    StorageType                 stHowStored;
    const CCachedAttrArrayInfo *pCAAI = NULL;
    BOOL                        fNumericHandler;
    CString                     szVTDef;
    CString                     szPropSignature;
    CString                     szPropVT;
    Token                      *pEnumType;
    char                        chCustomInvokeIdx[128];
    char                       *pDispatchType;
    CString szPreText,szPostText;
    CString szMemberDesc;
    
    szPreText = "_T(";
    szPostText = ")";

    pEnumType = FindEnum ( pChild );
    if ( pEnumType )
    {
        // Generic enumerator property handler, the propDesc has the enumerator
        // type.
        szPropVT = "PropEnum";
    }
    else
    {
        char *pWS;

        szPropVT = pChild->GetTagValue ( PROPERTY_ATYPE );

        // Remove any underscores in the type name.
        while ((pWS = szPropVT.FindChar('_')))
        {
            while (*pWS = *(pWS + 1))
                pWS++;
            *pWS = '\0';
        }
    }

    // Should this type be mapped to IDispatch?
    pDispatchType = MapTypeToIDispatch ( szPropVT );
    if (pDispatchType)
    {
        szPropVT = pDispatchType;
    }

    szClass = pClassToken->GetTagValue ( CLASS_NAME );

    szUpperName = pChild->GetTagValue ( PROPERTY_NAME );
    szUpperName.ToUpper();

    szHandler = "";
    szFnPrefix = "";
    // Check for missing handler later
    GetTypeDetails ( pChild->GetTagValue ( PROPERTY_TYPE ),
        szHandler, szFnPrefix, &stHowStored );

    CString szPropParamDesc;

    szdwFlags = pChild->GetTagValue ( PROPERTY_DWFLAGS );

    if ( pChild->IsSet ( PROPERTY_GETSETMETHODS ) )
    {
        szPropParamDesc = "PROPPARAM_GETMFHandler | PROPPARAM_SETMFHandler";
        szPropSignature = "GS";
    }
    else if ( pChild->IsSet ( PROPERTY_MEMBER ) )
    {
        szPropParamDesc = "PROPPARAM_MEMBER";
    }
    else
    {
        szPropParamDesc = "";
    }

    if ( pChild->IsSet ( PROPERTY_GET ) )
    {
        szPropSignature = "G";
        if ( szPropParamDesc [ 0 ] )
            szPropParamDesc += " | ";
        szPropParamDesc += "PROPPARAM_INVOKEGet";
    }
    if ( pChild->IsSet ( PROPERTY_SET ) )
    {
        szPropSignature += "S";
        if ( szPropParamDesc [ 0 ] )
            szPropParamDesc += " | ";
        szPropParamDesc += "PROPPARAM_INVOKESet";
    }

    if (!szPropSignature[0])
    {
        szPropSignature = "GS";
        if ( szPropParamDesc [ 0 ] )
            szPropParamDesc += " | ";
        szPropParamDesc += "PROPPARAM_INVOKEGet | PROPPARAM_INVOKESet";
    }

    // Write out the function signature for this property.
    if ( !FindAndAddSignature ( szPropSignature, szPropVT, &chCustomInvokeIdx[0] ) )
        return FALSE;

    if ( pChild->IsSet ( PROPERTY_PPFLAGS ))
    {
        szPropParamDesc += " | ";
        szPropParamDesc += pChild->GetTagValue ( PROPERTY_PPFLAGS );
    }
    if ( pChild->IsSet ( PROPERTY_NOPERSIST ) )
    {
        szPropParamDesc += " | PROPPARAM_NOPERSIST";
    }
    if ( pChild->IsSet ( PROPERTY_INVALIDASNOASSIGN ) )
    {
        szPropParamDesc += " | PROPPARAM_INVALIDASNOASSIGN";
    }
    if ( pChild->IsSet ( PROPERTY_NOTPRESENTASDEFAULT ) )
    {
        szPropParamDesc += " | PROPPARAM_NOTPRESENTASDEFAULT";
    }
    if ( pChild->IsSet ( PROPERTY_HIDDEN ) )
    {
        szPropParamDesc += " | PROPPARAM_HIDDEN";
    }
    if ( pChild->IsSet ( PROPERTY_RESTRICTED ) )
    {
        szPropParamDesc += " | PROPPARAM_RESTRICTED";
    }
    if ( pChild->IsSet ( PROPERTY_CAA ) )
    {
        szDispid = pChild->GetTagValue( PROPERTY_DISPID );
        pCAAI = GetCachedAttrArrayInfo(szDispid);

        szPropParamDesc += " | PROPPARAM_ATTRARRAY ";
#if 0
        if (pCAAI->szPPFlags)
        {
            szPropParamDesc += " | ";
            szPropParamDesc += pCAAI->szPPFlags;
        }
        if (pCAAI->szLMinBitMask)
        {
            pChild->AddTag( PROPERTY_MIN , pCAAI->szLMinBitMask );
        }
#endif
    }

    if ( pChild->IsSet ( PROPERTY_MINOUT ) )
    {
        szPropParamDesc += " | PROPPARAM_MINOUT";
    }

    if ( pChild->IsSet ( PROPERTY_SETDESIGNMODE ) )
    {
        szPropParamDesc += " | PROPPARAM_READONLYATRUNTIME";
    }

    // If we're processing a property that applies to a CF/PF/SF/FF, mark it here
    // this helps us optimize the apply process. Only properties with a DISPID that
    // matches our apply table, can be applied
    if ( pCAAI && pCAAI->szDispId != NULL )
    {
        szPropParamDesc += " | PROPPARAM_STYLISTIC_PROPERTY";
    }

    if ( pChild -> IsSet ( PROPERTY_SCRIPTLET ) )
    {
        szPropParamDesc += " | PROPPARAM_SCRIPTLET";
    }

    szMemberDesc = "";

    if ( pChild->IsSet ( PROPERTY_CAA ) )
    {
        if ( pCAAI->dwFlags & CCSSF_CLEARCACHES )
        {
            // Always add dwFlags:ELEMCHNG_CLEARCACHES for these properties
            if ( szdwFlags [0] )
            {
                szdwFlags += "|";
            }
            else
            {
                szdwFlags = "";
            }
            szdwFlags+="ELEMCHNG_CLEARCACHES";
        }

        if ( pCAAI->dwFlags & CCSSF_CLEARFF )
        {
            // Always add dwFlags:ELEMCHNG_CLEARFF for these properties
            if ( szdwFlags [0] )
            {
                szdwFlags += "|";
            }
            else
            {
                szdwFlags = "";
            }
            szdwFlags+="ELEMCHNG_CLEARFF";
        }
        
        if ( pCAAI->dwFlags & CCSSF_REMEASURECONTENTS )
        {
            // Always add dwFlags:ELEMCHNG_REMEASURECONTENTS for these properties
            if ( szdwFlags [0] )
            {
                szdwFlags += "|";
            }
            else
            {
                szdwFlags = "";
            }
            szdwFlags+="ELEMCHNG_REMEASURECONTENTS";
        }

        if ( pCAAI->dwFlags & CCSSF_REMEASUREALLCONTENTS )
        {
            // Always add dwFlags:ELEMCHNG_REMEASURECONTENTS for these properties
            if ( szdwFlags [0] )
            {
                szdwFlags += "|";
            }
            else
            {
                szdwFlags = "";
            }
            szdwFlags+="ELEMCHNG_REMEASUREALLCONTENTS";
        }

        if ( pCAAI->dwFlags & CCSSF_REMEASUREINPARENT )
        {
            // Always add dwFlags:ELEMCHNG_REMEASUREINPARENT for these properties
            if ( szdwFlags [0] )
            {
                szdwFlags += "|";
            }
            else
            {
                szdwFlags = "";
            }
            szdwFlags+="ELEMCHNG_REMEASUREINPARENT";
        }

        if ( pCAAI->dwFlags & CCSSF_SIZECHANGED )
        {
            // Always add dwFlags:ELEMCHNG_SIZECHANGED for these properties
            if ( szdwFlags [0] )
            {
                szdwFlags += "|";
            }
            else
            {
                szdwFlags = "";
            }
            szdwFlags+="ELEMCHNG_SIZECHANGED";
        }

    }
    else if ( pChild->IsSet ( PROPERTY_MEMBER ) )
    {
        // On the object
        szMemberDesc = szClass;
        szMemberDesc += ", ";
        szMemberDesc += pChild->GetTagValue ( PROPERTY_MEMBER );
    }
    
    if ( pChild -> IsSet ( PROPERTY_ACCESSIBILITYSTATE) )
    {
        // we should only allow this on properties that can be set,
        if ( ! ( pChild->IsSet ( PROPERTY_SET ) ) )
        {
            ReportError ( "Accessibility State can only be applied to r/w properties\n" );
            return FALSE;
        }
        
        if ( szdwFlags [ 0 ] )
        {
            szdwFlags += "|";
        }
        szdwFlags += "ELEMCHNG_ACCESSIBILITY";
    }

    if ( pChild -> IsSet ( PROPERTY_UPDATECOLLECTION ) )
    {
        if ( szdwFlags [ 0 ] )
        {
            szdwFlags += "|";
        }
        szdwFlags += "ELEMCHNG_UPDATECOLLECTION";
    }
    if ( pChild -> IsSet ( PROPERTY_CLEARCACHES ) )
    {
        if ( szdwFlags [ 0 ] )
        {
            szdwFlags += "|";
        }
        szdwFlags += "ELEMCHNG_CLEARCACHES";
    }
    if ( pChild -> IsSet ( PROPERTY_STYLEPROP ) )
    {
        if ( szPropParamDesc [ 0 ] )
            szPropParamDesc += " | ";
        szPropParamDesc += "PROPPARAM_STYLESHEET_PROPERTY";
    }
    if ( pChild -> IsSet ( PROPERTY_DONTUSENOTASSIGN ) )
    {
        if ( szPropParamDesc [ 0 ] )
            szPropParamDesc += " | ";
        szPropParamDesc += "PROPPARAM_DONTUSENOTASSIGNED";
    }
    if ( pChild -> IsSet ( PROPERTY_RESIZE ) )
    {
        if ( szdwFlags [ 0 ] )
        {
            szdwFlags += "|";
        }
        szdwFlags += "ELEMCHNG_SIZECHANGED";
    }
    if ( pChild -> IsSet ( PROPERTY_REMEASURE ) )
    {
        if ( szdwFlags [ 0 ] )
        {
            szdwFlags += "|";
        }
        szdwFlags += "ELEMCHNG_REMEASURECONTENTS";
    }
    if ( pChild -> IsSet ( PROPERTY_REMEASUREALL ) )
    {
        if ( szdwFlags [ 0 ] )
        {
            szdwFlags += "|";
        }
        szdwFlags += "ELEMCHNG_REMEASUREALLCONTENTS";
    }
    if ( pChild -> IsSet ( PROPERTY_SITEREDRAW ) )
    {
        if ( szdwFlags [ 0 ] )
        {
            szdwFlags += "|";
        }
        szdwFlags += "ELEMCHNG_SITEREDRAW";
    }

    char szPropertyDesc [ MAX_LINE_LEN+1 ] ;

    pChild -> GetTagValueOrDefault ( szNotPresentDefault, PROPERTY_NOTPRESENTDEFAULT, "0" );
    if ( pChild->IsSet( PROPERTY_NOTPRESENTDEFAULT ) && szHandler == "String")
    {
        szNotPresentDefault = szPreText + szNotPresentDefault;
        szNotPresentDefault += szPostText;
    }
    else if ( szHandler == "Color" &&  szNotPresentDefault == "0" )
    {
        // This is a nasty little hack to make for colors
        szNotPresentDefault = "-1";
    }
    // If there's a not assigned default, use it, else use the not present default
    pChild -> GetTagValueOrDefault ( szNotAssignedDefault, PROPERTY_NOTSETDEFAULT,
        (LPCSTR)szNotPresentDefault );
    if ( pChild->IsSet( PROPERTY_NOTSETDEFAULT ) && szHandler == "String")
    {
        szNotAssignedDefault = szPreText + szNotAssignedDefault;
        szNotAssignedDefault += szPostText;
    }
    else if ( szHandler == "Color" &&  szNotAssignedDefault == "0" )
    {
        szNotAssignedDefault = "-1";
    }

    // szAttribute spevcifies the html name of the property, if not specified the
    // property name itself is used.
    if ( pChild -> IsSet ( PROPERTY_SZATTRIBUTE ) )
    {
        szHTMLName = pChild -> GetTagValue ( PROPERTY_SZATTRIBUTE );
        sprintf ( (LPSTR) szExposedName,
                  "_T(\"%s\")", 
                  pChild->GetTagValue ( PROPERTY_NAME ) );
    }
    else
    {
        szHTMLName = pChild -> GetTagValue ( PROPERTY_NAME );
        strcpy(szExposedName, "NULL");
    }

    // If the propdesc has a member specified generate a full propdesc
    // By setting abstract: AND member: the propdesc can be used for validation
    // ( e.g. UnitMeasurement sub-object )
    if ( pChild->IsSet ( PROPERTY_ABSTRACT ) && !pChild->IsSet ( PROPERTY_MEMBER )  )
    {
        // Generate a minimal propdesc, this is how IDispatchEx traverses
        // attributes and properties.
        sprintf ( (LPSTR) szPropertyDesc,
            " s_propdesc%s%s = \n{\n    NULL, _T(\"%s\"), %s, (ULONG_PTR)%s, (ULONG_PTR)%s,\n    {",
            (LPCSTR)szClass,
            (LPCSTR)pChild->GetTagValue ( PROPERTY_NAME ),
            (LPCSTR)szHTMLName,
            (LPCSTR)szExposedName,
            (LPCSTR)szNotPresentDefault,
            (LPCSTR)szNotAssignedDefault);
    }
    else
    {
        // If you're gonna set up a handler, better be a valid one
        if ( szHandler == "" )
        {
            sprintf ( szErrorText, "Invalid Type:%s in Class:%s Property:%s\n",
                (LPCSTR)pChild->GetTagValue ( PROPERTY_TYPE ),
                (LPCSTR)pClassToken->GetTagValue(CLASS_NAME),
                (LPCSTR)pChild->GetTagValue ( PROPERTY_NAME ) );
            ReportError ( szErrorText );
            return FALSE;
        }
        char szTempBuf[MAX_LINE_LEN+1];
        sprintf ( (LPSTR) szTempBuf, " s_propdesc%s%s =\n{\n#ifdef WIN16\n    (PFN_HANDLEPROPERTY)&PROPERTYDESC::handle%sproperty, _T(\"%s\"), %s, (ULONG_PTR)%s, (ULONG_PTR)%s,\n#else\n",
            (LPCSTR)szClass,
            (LPCSTR)pChild->GetTagValue ( PROPERTY_NAME ),
            (LPCSTR)szHandler,
            (LPCSTR)szHTMLName,
            (LPCSTR)szExposedName,
            (LPCSTR)szNotPresentDefault,
            (LPCSTR)szNotAssignedDefault);
        sprintf ( (LPSTR) szPropertyDesc, "%s    PROPERTYDESC::Handle%sProperty, _T(\"%s\"), %s, (ULONG_PTR)%s,(ULONG_PTR)%s,\n#endif\n    {", szTempBuf,
            (LPCSTR)szHandler,
            (LPCSTR)szHTMLName,
            (LPCSTR)szExposedName,
            (LPCSTR)szNotPresentDefault,
            (LPCSTR)szNotAssignedDefault);
    }

    if ( !ComputePROPDESC_STRUCT ( fpHDLFile, pClassToken, pChild, szHandler, szFnPrefix) )
        return FALSE;       // Bad error reported...leave

    if ( !szdwFlags [ 0 ] )
        szdwFlags = "0";

    fNumericHandler = FALSE;
    if ( !pChild->IsSet ( PROPERTY_ABSTRACT ) && szHandler == "Num" || szHandler == "Enum"  || szHandler == "UnitValue" )
    {
        // Numeric Handlers
        fNumericHandler = TRUE;

        fprintf ( fpHDLFile, "%s\n        {", szPropertyDesc );

        if ( szHandler == "Enum" )
        {
            szPropParamDesc += " | PROPPARAM_ENUM";
        }
        else if ( pChild -> IsSet ( PROPERTY_ENUMREF ) )
        {
            // A number with one or more enums
            szPropParamDesc += " | PROPPARAM_ENUM | PROPPARAM_ANUMBER";
        }

        pChild -> GetTagValueOrDefault ( szDispid, PROPERTY_DISPID, "0" );

        if ((strcmp(pChild->GetTagValue(PROPERTY_ATYPE), "BSTR") == 0 || strcmp(pChild->GetTagValue(PROPERTY_ATYPE), "VARIANT") == 0) && 
            ((pChild->IsSet(PROPERTY_CAA) || pChild->IsSet(PROPERTY_SET)) && !pChild->IsSet(PROPERTY_INTERNAL)) ||
            (!pChild->IsSet(PROPERTY_CAA) && !pChild->IsSet(PROPERTY_SET) && !pChild->IsSet(PROPERTY_GET)))
        {
            if (!pChild->IsSet(PROPERTY_MAXSTRLEN))
            {
                char szErrorText [ MAX_LINE_LEN+1 ];

                // Dispid not specified this is an error all methods should be accessible
                // from automation.
                sprintf ( szErrorText, "maxstrlen required for property: %s::%s in %s.\n",
                          (LPCSTR)pClassToken->GetTagValue ( CLASS_NAME ),
                          (LPCSTR)pChild->GetTagValue ( PROPERTY_NAME ),
                          _pszPDLFileName  );
                ReportError ( szErrorText );
                return FALSE;
            }
        }
        else if (pChild->IsSet(PROPERTY_MAXSTRLEN))
        {
            char szErrorText [ MAX_LINE_LEN+1 ];

            // Dispid not specified this is an error all methods should be accessible
            // from automation.
            sprintf ( szErrorText, "maxstrlen NOT required for property: %s::%s in %s.\n",
                      (LPCSTR)pClassToken->GetTagValue ( CLASS_NAME ),
                      (LPCSTR)pChild->GetTagValue ( PROPERTY_NAME ),
                      _pszPDLFileName  );
            ReportError ( szErrorText );
            return FALSE;
        }

        fprintf ( fpHDLFile, "\n            %s, %s, %s, %s, %s \n        },",
            (LPCSTR)szPropParamDesc,
            (LPCSTR)szDispid,
            (LPCSTR)szdwFlags,
            (LPCSTR)&chCustomInvokeIdx[0],
            pChild->IsSet(PROPERTY_MAXSTRLEN) ? (LPCSTR)pChild->GetTagValue(PROPERTY_MAXSTRLEN) : "0");

        if (fpMaxLenFile && pChild->IsSet(PROPERTY_MAXSTRLEN))
        {
            fprintf ( fpMaxLenFile, "%s::%s    %s\n",
                      (LPCSTR)pClassToken->GetTagValue ( CLASS_NAME ),
                      (LPCSTR)pChild->GetTagValue ( PROPERTY_NAME ),
                      (LPCSTR)pChild->GetTagValue(PROPERTY_MAXSTRLEN) );
        }

        if ( pChild->IsSet ( PROPERTY_ABSTRACT ) )
        {
            fprintf ( fpHDLFile, "\n        0, 0" );
        }
        else
        {
            if ( !pChild->IsSet ( PROPERTY_VT ) )
            {
                CString szAType;
                szAType = pChild->GetTagValue ( PROPERTY_ATYPE );
                if ( szAType == "short" || szAType == "VARIANT_BOOL" )
                    szVTDef = "VT_I2";
                else
                    szVTDef = "VT_I4";
            }
            else
            {
                szVTDef = pChild->GetTagValue ( PROPERTY_VT );
            }

            if ( pChild -> IsSet ( PROPERTY_GETSETMETHODS ) )
            {
                fprintf ( fpHDLFile, "\n        %s, 0",
                    (LPCSTR)szVTDef );
            }
            else if ( pChild -> IsSet ( PROPERTY_CAA ) )
            {
                fprintf ( fpHDLFile, "\n        %s, sizeof(DWORD)",
                    (LPCSTR)szVTDef );
            }
            else
            {
                fprintf ( fpHDLFile, "\n        %s, SIZE_OF(%s)",
                    (LPCSTR)szVTDef, (LPCSTR)szMemberDesc );
            }
        }

        // Fill in the min/max values
        // If it's an enum, the min value is a ptr to the enum desc structure
        if ( szHandler == "Enum" )
        {
            if ( pChild->IsSet ( PROPERTY_ABSTRACT ) )
            {
                fprintf ( fpHDLFile, ", 0, 0,\n" );
            }
            else
            {
                // Get the enum mask from the enum named by  PROPERTY_TYPE
                CTokenListWalker WholeList ( pRuntimeList );
                Token *pEnumToken = WholeList.GetNext ( TYPE_ENUM, pChild->GetTagValue ( PROPERTY_TYPE ) );
                CString szMin;
                pChild->GetTagValueOrDefault ( szMin, PROPERTY_MIN, "0" );

                if ( pEnumToken == NULL )
                {
                    sprintf ( szErrorText,
                        "unknown enum type %s\n",pChild->GetTagValue ( PROPERTY_TYPE ) );
                    ReportError ( szErrorText );
                    return FALSE;
                }
                fprintf ( fpHDLFile, ", %s, (LONG_PTR)&s_enumdesc%s,\n",
                    (LPCSTR)szMin, pChild->GetTagValue ( PROPERTY_TYPE ) );
            }
        }
        else
        {
            CString szMax;
            CString szMin;
            pChild->GetTagValueOrDefault ( szMin, PROPERTY_MIN, "LONG_MIN" );
            pChild->GetTagValueOrDefault ( szMax, PROPERTY_MAX, "LONG_MAX" );
            fprintf ( fpHDLFile, ", %s, %s,\n", (LPCSTR)szMin, (LPCSTR)szMax );
        }
    }
    else
    {
        fprintf ( fpHDLFile, "%s\n    ", szPropertyDesc );
        pChild -> GetTagValueOrDefault ( szDispid, PROPERTY_DISPID, "0" );

        if ((strcmp(pChild->GetTagValue(PROPERTY_ATYPE), "BSTR") == 0 || strcmp(pChild->GetTagValue(PROPERTY_ATYPE), "VARIANT") == 0) && 
            ((pChild->IsSet(PROPERTY_CAA) || pChild->IsSet(PROPERTY_SET)) && !pChild->IsSet(PROPERTY_INTERNAL)) ||
            (!pChild->IsSet(PROPERTY_CAA) && !pChild->IsSet(PROPERTY_SET) && !pChild->IsSet(PROPERTY_GET)))
        {
            if (!pChild->IsSet(PROPERTY_MAXSTRLEN))
            {
                char szErrorText [ MAX_LINE_LEN+1 ];

                // Dispid not specified this is an error all methods should be accessible
                // from automation.
                sprintf ( szErrorText, "maxstrlen required for property: %s::%s in %s.\n",
                          (LPCSTR)pClassToken->GetTagValue ( CLASS_NAME ),
                          (LPCSTR)pChild->GetTagValue ( PROPERTY_NAME ),
                          _pszPDLFileName  );
                ReportError ( szErrorText );
                return FALSE;
            }
        }
        else if (pChild->IsSet(PROPERTY_MAXSTRLEN))
        {
            char szErrorText [ MAX_LINE_LEN+1 ];

            // Dispid not specified this is an error all methods should be accessible
            // from automation.
            sprintf ( szErrorText, "maxstrlen NOT required for property: %s::%s in %s.\n",
                      (LPCSTR)pClassToken->GetTagValue ( CLASS_NAME ),
                      (LPCSTR)pChild->GetTagValue ( PROPERTY_NAME ),
                      _pszPDLFileName  );
            ReportError ( szErrorText );
            return FALSE;
        }

        fprintf ( fpHDLFile, "    %s, %s, %s, %s, %s \n",
            (LPCSTR)szPropParamDesc,
            (LPCSTR)szDispid,
            (LPCSTR)szdwFlags,
            (LPCSTR)&chCustomInvokeIdx[0],
            pChild->IsSet(PROPERTY_MAXSTRLEN) ? (LPCSTR)pChild->GetTagValue(PROPERTY_MAXSTRLEN) : "0");

        if (fpMaxLenFile && pChild->IsSet(PROPERTY_MAXSTRLEN))
        {
            fprintf ( fpMaxLenFile, "%s::%s    %s\n",
                      (LPCSTR)pClassToken->GetTagValue ( CLASS_NAME ),
                      (LPCSTR)pChild->GetTagValue ( PROPERTY_NAME ),
                      (LPCSTR)pChild->GetTagValue(PROPERTY_MAXSTRLEN) );
        }

    }
    fprintf ( fpHDLFile, "    }," );
    if ( pChild->IsSet ( PROPERTY_GETSETMETHODS ) )
    {
        fprintf ( fpHDLFile,  "\n    PROPERTY_METHOD(%s, GET, %s, Get%s, GET%s)," ,
            (LPCSTR)szFnPrefix, 
            (LPCSTR)szClass, (LPCSTR)pChild->GetTagValue ( PROPERTY_GETSETMETHODS ),
            (LPCSTR)pChild->GetTagValue ( PROPERTY_GETSETMETHODS ));
        fprintf ( fpHDLFile,  "\n    PROPERTY_METHOD(%s, SET, %s, Set%s, SET%s)" ,
            (LPCSTR)szFnPrefix, 
            (LPCSTR)szClass, (LPCSTR)pChild->GetTagValue ( PROPERTY_GETSETMETHODS ),
            (LPCSTR)pChild->GetTagValue ( PROPERTY_GETSETMETHODS ));
    }
    else if ( pChild->IsSet ( PROPERTY_ABSTRACT ) )
    {
    }
    else if ( szMemberDesc [ 0 ] )
    {
        fprintf ( fpHDLFile, "\n    offsetof(%s)",
            (LPCSTR)szMemberDesc );
    }

    if ( fNumericHandler && pChild->IsSet ( PROPERTY_ENUMREF ) &&
        !pChild->IsSet ( PROPERTY_ABSTRACT ) )
    {
        CTokenListWalker WholeList ( pRuntimeList );
        Token *pEnumToken = WholeList.GetNext ( TYPE_ENUM,
            pChild->GetTagValue ( PROPERTY_ENUMREF ) );
        if ( pEnumToken == NULL )
        {
            sprintf ( szErrorText,
                "unknown enum type %s\n",pChild->GetTagValue ( PROPERTY_ENUMREF ) );
            ReportError ( szErrorText );
            return FALSE;
        }
        if ( !szMemberDesc [ 0 ] )
        {
            fprintf ( fpHDLFile, "\n    0, &s_enumdesc%s",
                pChild->GetTagValue ( PROPERTY_ENUMREF ) );
        }
        else
        {
            fprintf ( fpHDLFile, "\n    &s_enumdesc%s",
                pChild->GetTagValue ( PROPERTY_ENUMREF ) );
        }
    }

    fprintf ( fpHDLFile, "\n};\n\n" );

    return TRUE;
}


BOOL CPDLParser::BuildMethodSignature(Token *pChild,
                                      CString &szTypesSig,
                                      CString &szArgsType,
                                      BOOL &fBSTRArg,
                                      BOOL &fVARIANTArg,
                                      int  &cArgs,
                                      int  &cRequiredArgs,
                                      char *pDefaultParams[MAX_ARGS],
                                      char *pDefaultStrParams[MAX_ARGS])
{
    Token              *pArgToken;
    CTokenListWalker    ArgListWalker(pChild);
    char               *pDispatchType;
    char                szErrorText [ MAX_LINE_LEN+1 ];

    cArgs = 0;
    cRequiredArgs = 0;
    fBSTRArg = FALSE;
    fVARIANTArg = FALSE;

    // Loop thru all arguments.
    while ( (pArgToken = ArgListWalker.GetNext()) != NULL &&
            pArgToken -> GetType () == TYPE_METHOD_ARG )
    {
        fBSTRArg |= (strcmp(pArgToken->GetTagValue(METHODARG_TYPE), "BSTR") == 0);
        fVARIANTArg |= (!pArgToken->IsSet(METHODARG_OUT) && (strcmp(pArgToken->GetTagValue(METHODARG_TYPE), "VARIANT") == 0 || strcmp(pArgToken->GetTagValue(METHODARG_TYPE), "VARIANT*") == 0));

        // Looking for a return value.
        if ( pArgToken -> IsSet ( METHODARG_RETURNVALUE ) )
        {
            char   *pWS;

            szTypesSig = pArgToken -> GetTagValue ( METHODARG_TYPE );

            // Remove any underscores in the type name.
            while ((pWS = szTypesSig.FindChar('_')))
            {
                while (*pWS = *(pWS + 1))
                    pWS++;
                *pWS = '\0';
            }

            // Should this type be mapped to IDispatch?
            pDispatchType = MapTypeToIDispatch ( szTypesSig );
            if (pDispatchType)
            {
                szTypesSig = pDispatchType;
            }
        }
        else
        {
            CString     szArg;
            char        *pWS;

            if ( pArgToken->IsSet ( METHODARG_OPTIONAL ) )
            {
                // little o + zero + little o, prepended to the type signals
                // the type is optional.
                szArg = "o0o";
            }

            if ( pArgToken -> IsSet ( METHODARG_DEFAULTVALUE ) )
            {
                // Signal default.
                szArg = "oDo";

                if (cArgs >= MAX_ARGS)
                {
                    sprintf ( szErrorText, "PDL parser can only handle upto 8 parameters (increase MAX_ARGS) %s in file %s.\n",
                                (LPCSTR)pChild->GetTagValue ( METHOD_NAME ),
                                _pszPDLFileName  );
                    ReportError ( szErrorText );
                    return FALSE;
                }

                // Currently I only handle 2 types of default values numbers and
                // strings.  If any other appear then I'll need to add those as
                // well.
                if ( strcmp (pArgToken -> GetTagValue ( METHODARG_TYPE ), "BSTR") == 0)
                {
                    if (pDefaultStrParams)
                        pDefaultStrParams[cArgs] = pArgToken -> GetTagValue ( METHODARG_DEFAULTVALUE );
                }
                else if (pDefaultParams)
                {
                    pDefaultParams[cArgs] = pArgToken -> GetTagValue ( METHODARG_DEFAULTVALUE );
                }
            }
            else
            {
                // If not optional and not defaultValue then the argument is
                // required and no defaults could have appeared in between.
                if ( !pArgToken->IsSet ( METHODARG_OPTIONAL ) )
                {
                    // Insure that once a default argument is hit all arguments from
                    // that point to the last argument im the function are either
                    // defaultValue or optional.
                    if (cRequiredArgs != cArgs)
                    {
                        sprintf ( szErrorText, "Default arguments must be contiguous to last argument %s in file %s.\n",
                                    (LPCSTR)pChild->GetTagValue ( METHOD_NAME ),
                                    _pszPDLFileName  );
                        ReportError ( szErrorText );
                        return FALSE;
                    }

                    // Arguments without a defaultValue/optional are required args.

                    // Unless the argument is a safe array than any number of
                    // arguments can be specified 0 to n arguments
                    if (strcmp((LPCSTR)(pArgToken -> GetTagValue(METHODARG_TYPE)), "SAFEARRAY(VARIANT)") != 0)
                        // Not a safearray so it's a required argument.
                        cRequiredArgs++;
                }
            }

            szArg += pArgToken -> GetTagValue ( METHODARG_TYPE );
            if (szArg == "oDoVARIANT")
            {
                sprintf ( szErrorText, "Default arguments cannot be VARIANT %s in file %s.\n",
                          (LPCSTR)pChild->GetTagValue ( METHOD_NAME ),
                          _pszPDLFileName  );
                ReportError ( szErrorText );
                return FALSE;
            }

            if ( pArgToken->IsSet ( METHODARG_OPTIONAL ) &&
                 !(szArg == "o0oVARIANT" || szArg == "o0oVARIANT*") )
            {
                sprintf ( szErrorText, "Optional arguments can ONLY be VARIANT or VARIANT* %s in file %s.\n",
                          (LPCSTR)pChild->GetTagValue ( METHOD_NAME ),
                          _pszPDLFileName  );
                ReportError ( szErrorText );
                return FALSE;
            }

            // Should this type be mapped to IDispatch?
            pDispatchType = MapTypeToIDispatch ( szArg );
            if (pDispatchType)
            {
                // Remap to IDispatch* or IDispatch**
                szArg = pDispatchType;
            }

            cArgs++;

            // Remove any underscores in the type name.
            while ((pWS = szArg.FindChar('_')))
            {
                while (*pWS = *(pWS + 1))
                    pWS++;
                *pWS = '\0';
            }

            szArgsType += "_";
            szArgsType += (LPCSTR)szArg;
        }
    }

    return TRUE;
}




BOOL CPDLParser::ComputeMethod ( Token *pClassToken, Token *pChild )
{
    CString             szTypesSig;
    CString             szArgsType;
    CString             szDispid;
    CTokenListWalker    ArgListWalker ( pChild );
    char                chCustomInvokeIdx[128];
    int                 cArgs = 0;
    int                 cRequiredArgs = 0;
    char                szErrorText [ MAX_LINE_LEN+1 ];
    char               *pDefaultParams[MAX_ARGS]    = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    char               *pDefaultStrParams[MAX_ARGS] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    BOOL                fBSTRArg;
    BOOL                fVARIANTArg;

    if (!BuildMethodSignature(pChild,
                              szTypesSig,
                              szArgsType,
                              fBSTRArg,
                              fVARIANTArg,
                              cArgs,
                              cRequiredArgs,
                              pDefaultParams,
                              pDefaultStrParams))
        return FALSE;

    // Any default values?
    if (cArgs != cRequiredArgs)
    {
        int     i;

        // Spit out any default string constants.
        for ( i = cRequiredArgs; i < cArgs; i++)
        {
            if ( pDefaultStrParams[i] )
            {
                fprintf ( fpHDLFile, "const TCHAR s_strDef%s%s%i[] = _T(%s);\n",
                          (LPCSTR)pClassToken->GetTagValue ( CLASS_NAME ),
                          (LPCSTR)pChild->GetTagValue ( METHOD_NAME ),
                          i - cRequiredArgs,
                          pDefaultStrParams[i] );
                             
            }
        }

        fprintf ( fpHDLFile, "const DEFAULTARGDESC s_defArg%s%s =\n{\n",
                  (LPCSTR)pClassToken->GetTagValue ( CLASS_NAME ),
                  (LPCSTR)pChild->GetTagValue ( METHOD_NAME ) );

        // Spit out the all of all default constants
        for ( i = cRequiredArgs; i < cArgs; i++)
        {
            if ( pDefaultStrParams[i] )
            {
                fprintf ( fpHDLFile, "    (DWORD_PTR)s_strDef%s%s%i,\n",
                          (LPCSTR)pClassToken->GetTagValue ( CLASS_NAME ),
                          (LPCSTR)pChild->GetTagValue ( METHOD_NAME ),
                          i - cRequiredArgs );
            }
            else
            {
                fprintf ( fpHDLFile, "    (DWORD_PTR)%s,\n", pDefaultParams[i] ? pDefaultParams[i] : "\"\""  );
            }
        }

        fprintf ( fpHDLFile, "};\n" );
    }

    // If no return value then set the retVal to void.
    if (!szTypesSig[0])
    {
        szTypesSig = "void";
    }

    // If no arguments then set the argList to void.
    if (!szArgsType[0])
    {
        szArgsType = "_void";
    }

    szTypesSig += szArgsType;

    szDispid = pChild->GetTagValue(METHOD_DISPID);
    if (szDispid[0])
    {
        // Write out the function signature for this method
        // only do this if a dispid exists.  Methods without dispid
        // are not accessible through automation.
        if ( !FindAndAddSignature ( "Method", szTypesSig, &chCustomInvokeIdx[0] ) )
            return FALSE;
    }
    
    fprintf ( fpHDLFile, "EXTERN_C const PROPERTYDESC_METHOD s_methdesc%s%s = \n{\n",
              (LPCSTR)pClassToken->GetTagValue ( CLASS_NAME ),
              (LPCSTR)pChild->GetTagValue ( METHOD_NAME ));

    fprintf ( fpHDLFile, "    NULL, NULL, _T(\"%s\"), (ULONG)0, (ULONG)0,\n    {\n",
              (LPCSTR)pChild->GetTagValue ( METHOD_NAME ));

    if (!szDispid[0])
    {
        // Dispid not specified this is an error all methods should be accessible
        // from automation.
        sprintf ( szErrorText, "DISPID required for method: %s::%s in %s.\n",
                  (LPCSTR)pClassToken->GetTagValue ( CLASS_NAME ),
                  (LPCSTR)pChild->GetTagValue ( METHOD_NAME ),
                  _pszPDLFileName  );
        ReportError ( szErrorText );
        return FALSE;
    }

    if (fBSTRArg || fVARIANTArg)
    {
        if (!pChild->IsSet(METHOD_MAXSTRLEN))
        {
            // Dispid not specified this is an error all methods should be accessible
            // from automation.
            sprintf ( szErrorText, "maxstrlen required for method: %s::%s in %s.\n",
                      (LPCSTR)pClassToken->GetTagValue ( CLASS_NAME ),
                      (LPCSTR)pChild->GetTagValue ( METHOD_NAME ),
                      _pszPDLFileName  );
            ReportError ( szErrorText );
            return FALSE;
        }
    }
    else if (pChild->IsSet(METHOD_MAXSTRLEN))
    {
        // Dispid not specified this is an error all methods should be accessible
        // from automation.
        sprintf ( szErrorText, "maxstrlen NOT required for method: %s::%s in %s.\n",
                  (LPCSTR)pClassToken->GetTagValue ( CLASS_NAME ),
                  (LPCSTR)pChild->GetTagValue ( METHOD_NAME ),
                  _pszPDLFileName  );
        ReportError ( szErrorText );
        return FALSE;
    }

    fprintf ( fpHDLFile, "        %s, %s, 0, %s, %s\n    },\n    ",
              pChild->IsSet(METHOD_RESTRICTED) ? "PROPPARAM_RESTRICTED" : "0",
              (LPCSTR)szDispid,
              (LPCSTR)&chCustomInvokeIdx[0],
              pChild->IsSet(METHOD_MAXSTRLEN) ? (LPCSTR)pChild->GetTagValue(METHOD_MAXSTRLEN) : "0");
    
    if (fpMaxLenFile && pChild->IsSet(METHOD_MAXSTRLEN))
    {
        fprintf ( fpMaxLenFile, "%s::%s    %s\n",
                  (LPCSTR)pClassToken->GetTagValue(CLASS_NAME),
                  (LPCSTR)pChild->GetTagValue(METHOD_NAME),
                  (LPCSTR)pChild->GetTagValue(METHOD_MAXSTRLEN) );
    }

    if ( cArgs != cRequiredArgs )
    {
        fprintf ( fpHDLFile, "&s_defArg%s%s, ", 
                  (LPCSTR)pClassToken->GetTagValue ( CLASS_NAME ),
                  (LPCSTR)pChild->GetTagValue ( METHOD_NAME ) );
    }
    else
    {
        fprintf ( fpHDLFile, "NULL, " );
    }
    
    fprintf ( fpHDLFile, "%i, %i\n};\n\n",
              cArgs,
              cRequiredArgs );

    return TRUE;
}


BOOL CPDLParser::GeneratePROPDESCs ( void )
{
    Token *pClassToken;
    Token *pChild;

    // Only generate def's for this file
    CTokenListWalker TokenList ( pRuntimeList, _pszPDLFileName );

    // Generate propdescs for every property token in every class ( in this file )
    while ( pClassToken = TokenList.GetNext( TYPE_CLASS ) )
    {
        fprintf ( fpHDLFile, "\n" );
        CTokenListWalker ChildList ( pClassToken );

//        if ( pClassToken -> IsSet ( CLASS_ABSTRACT ) )
//        {
//            fprintf ( fpHDLFile, "\n#ifndef _PROPDESCS_EXTERNAL\n" );
//        }

        fprintf ( fpHDLFile, "\n#define _%s_PROPDESCS_\n", pClassToken -> GetTagValue ( CLASS_NAME ) );

        // Walk the super class propdescs looking for properties
        while ( pChild = ChildList.GetNext() )
        {
            if ( pChild->nType == TYPE_PROPERTY &&
                 _stricmp(pChild->GetTagValue(PROPERTY_NOPROPDESC), "nameonly") != 0 )
            {
                if ( !ComputeProperty ( pClassToken, pChild ) )
                    return FALSE;
            }
            else if ( pChild->nType == TYPE_METHOD && !pChild->IsSet(METHOD_NOPROPDESC))
            {
                if (!ComputeMethod(pClassToken, pChild) )
                    return FALSE;
            }
            else
            {
                continue;
            }
        }

        if (fpMaxLenFile)
            fprintf(fpMaxLenFile, "\n");
    }
    return TRUE;
}

void CPDLParser::GenerateThunkContext ( Token *pClassToken )
{
    CTokenListWalker ChildList (pClassToken);

    Token *pChildToken;

    while (pChildToken = ChildList.GetNext())
    {
        if( (pChildToken->GetType() == TYPE_METHOD && pChildToken->IsSet(METHOD_THUNKCONTEXT)) || 
            (pChildToken->GetType() == TYPE_PROPERTY && pChildToken->IsSet(PROPERTY_THUNKCONTEXT)) )
        {
            GenerateSingleThunkContextPrototype(pClassToken, pChildToken, FALSE);
        }
        if( (pChildToken->GetType() == TYPE_METHOD && pChildToken->IsSet(METHOD_THUNKNODECONTEXT)) || 
            (pChildToken->GetType() == TYPE_PROPERTY && pChildToken->IsSet(PROPERTY_THUNKNODECONTEXT)) )
        {
            GenerateSingleThunkContextPrototype(pClassToken, pChildToken, TRUE);
        }
    }

}

void CPDLParser::GenerateSingleThunkContextPrototype ( Token *pClassToken, Token * pChildToken, BOOL fNodeContext )
{
    CString szProp;
    Token *pArgToken;

    if (pChildToken->GetType() == TYPE_METHOD)
    {
        CTokenListWalker ArgListWalker(pChildToken);
        BOOL fFirst = TRUE;

        fprintf(fpHDLFile, "    STDMETHODIMP %s(", (LPCSTR)pChildToken->GetTagValue(METHOD_NAME));
        while (pArgToken = ArgListWalker.GetNext())
        {
            if (!fFirst)
                fprintf(fpHDLFile, ",");
            fprintf(fpHDLFile, "%s %s",
                ConvertType((LPCSTR)pArgToken->GetTagValue ( METHODARG_TYPE )),
                (LPCSTR)pArgToken -> GetTagValue ( METHODARG_ARGNAME ) );
            fFirst = FALSE;
        }

        if(fNodeContext)
        {
            fprintf(fpHDLFile, "%sCTreeNode *pNode);\n", fFirst?"":",");
        }
        else
        {
            fprintf(fpHDLFile, "%sTEAROFF_THUNK*ptt);\n", fFirst?"":",");
        }
    }
    else
    {
        if (pChildToken->IsSet(PROPERTY_SET))
        {
            fprintf(fpHDLFile, "    STDMETHODIMP set_%s(%s v, %s);\n",
                    (LPCSTR)pChildToken->GetTagValue(PROPERTY_NAME),
                    (LPCSTR)pChildToken -> GetTagValue(PROPERTY_ATYPE),
                    fNodeContext?"CTreeNode *pNode":"TEAROFF_THUNK *ptt");
        }
        if (pChildToken->IsSet(PROPERTY_GET))
        {
            fprintf(fpHDLFile, "    STDMETHODIMP get_%s(%s *p, %s);\n",
                    (LPCSTR)pChildToken->GetTagValue(PROPERTY_NAME),
                    (LPCSTR)pChildToken -> GetTagValue(PROPERTY_ATYPE),
                    fNodeContext?"CTreeNode *pNode":"TEAROFF_THUNK *ptt");
        }
    }
}

void CPDLParser::GenerateThunkContextImplemenation ( Token *pClassToken )
{
    CTokenListWalker ChildList (pClassToken);

    Token *pChildToken;

    while (pChildToken = ChildList.GetNext())
    {
        if( (pChildToken->GetType() == TYPE_METHOD && pChildToken->IsSet(METHOD_THUNKCONTEXT)) || 
            (pChildToken->GetType() == TYPE_PROPERTY && pChildToken->IsSet(PROPERTY_THUNKCONTEXT)) )
        {
            GenerateSingleThunkContextImplementation(pClassToken, pChildToken, FALSE);
        }
        if( (pChildToken->GetType() == TYPE_METHOD && pChildToken->IsSet(METHOD_THUNKNODECONTEXT)) || 
            (pChildToken->GetType() == TYPE_PROPERTY && pChildToken->IsSet(PROPERTY_THUNKNODECONTEXT)) )
        {
            GenerateSingleThunkContextImplementation(pClassToken, pChildToken, TRUE);
        }
    }
}

void CPDLParser::GenerateSingleThunkContextImplementation ( Token *pClassToken, Token * pChildToken, BOOL fNodeContext )
{
    CString szProp;
    Token *pArgToken;

    fprintf(fpHDLFile, "#ifdef USE_STACK_SPEW\n#pragma check_stack(off)\n#endif\n");

    if (pChildToken->GetType() == TYPE_METHOD)
    {
        CTokenListWalker ArgListWalker(pChildToken);
        BOOL fFirst = TRUE;

        fprintf(fpHDLFile, "STDMETHODIMP %s::ContextThunk_%s(", 
                (LPCSTR)pClassToken->GetTagValue(CLASS_NAME),
                (LPCSTR)pChildToken->GetTagValue(METHOD_NAME));

        while (pArgToken = ArgListWalker.GetNext())
        {
            if (!fFirst)
                fprintf(fpHDLFile, ",");
            fprintf(fpHDLFile, "%s %s",
                ConvertType((LPCSTR)pArgToken->GetTagValue ( METHODARG_TYPE )),
                (LPCSTR)pArgToken -> GetTagValue ( METHODARG_ARGNAME ) );
            fFirst = FALSE;
        }

        if(fNodeContext)
        {
            fprintf(fpHDLFile, ")\n"
                               "{\n"
                               "    CTreeNode* pNode;\n"
                               "    CONTEXTTHUNK_SETTREENODE\n"
                               "    return %s(", (LPCSTR)pChildToken->GetTagValue(METHOD_NAME));
        }
        else
        {
            fprintf(fpHDLFile, ")\n"
                               "{\n"
                               "    TEAROFF_THUNK* ptt;\n"
                               "    CONTEXTTHUNK_SETTEAROFFTHUNK\n"
                               "    return %s(", (LPCSTR)pChildToken->GetTagValue(METHOD_NAME));
        }

        fFirst = TRUE;
        ArgListWalker.Reset();
        while (pArgToken = ArgListWalker.GetNext())
        {
            if (!fFirst)
                fprintf(fpHDLFile, ",");
            fprintf(fpHDLFile, "%s",
                (LPCSTR)pArgToken -> GetTagValue(METHODARG_ARGNAME));
            fFirst = FALSE;
        }
        fprintf(fpHDLFile, "%s%s);\n}\n", fFirst?"":",", fNodeContext?"pNode":"ptt");
    }
    else
    {
        if (pChildToken->IsSet(PROPERTY_SET))
        {
            fprintf(fpHDLFile, 
                    "STDMETHODIMP %s::ContextThunk_set_%s(%s v)\n"
                    "{\n%s    return set_%s(v,%s);\n}\n",
                    (LPCSTR)pClassToken->GetTagValue(CLASS_NAME),
                    (LPCSTR)pChildToken->GetTagValue(PROPERTY_NAME),
                    (LPCSTR)pChildToken->GetTagValue(PROPERTY_ATYPE),
                    fNodeContext 
                      ? "    CTreeNode* pNode;\n"
                        "    CONTEXTTHUNK_SETTREENODE\n"
                      : "    TEAROFF_THUNK* ptt;\n"
                        "    CONTEXTTHUNK_SETCONTEXTTHUNK\n",
                    (LPCSTR)pChildToken->GetTagValue(PROPERTY_NAME),
                    fNodeContext ? "pNode" : "ptt");
        }
        if (pChildToken->IsSet(PROPERTY_GET))
        {
            fprintf(fpHDLFile, 
                    "STDMETHODIMP %s::ContextThunk_get_%s(%s *p)\n"
                    "{\n%s    return get_%s(p,%s);\n}\n",
                    (LPCSTR)pClassToken->GetTagValue(CLASS_NAME),
                    (LPCSTR)pChildToken->GetTagValue(PROPERTY_NAME),
                    (LPCSTR)pChildToken->GetTagValue(PROPERTY_ATYPE),
                    fNodeContext 
                      ? "    CTreeNode* pNode;\n"
                        "    CONTEXTTHUNK_SETTREENODE\n"
                      : "    TEAROFF_THUNK* ptt;\n"
                        "    CONTEXTTHUNK_SETCONTEXTTHUNK\n",
                    (LPCSTR)pChildToken->GetTagValue(PROPERTY_NAME),
                    fNodeContext ? "pNode" : "ptt");
        }
    }
    fprintf(fpHDLFile, "#ifdef USE_STACK_SPEW\n#pragma check_stack(on)\n#endif\n");
}


BOOL CPDLParser::GenerateHDLFile ( void )
{
    Token  *pClassToken;
    int     numPropDescs;
    int     numVTblEntries;
    int     numVTblPropDescs;

    CTokenListWalker ThisFilesList ( pRuntimeList, _pszPDLFileName );

    fprintf ( fpHDLFile, "\n" );
    fprintf ( fpHDLFile, "// %s.hdl\n",  _pszOutputFileRoot );
    fprintf ( fpHDLFile, "\n" );
    fprintf ( fpHDLFile, "#ifdef _hxx_\n" );
    fprintf ( fpHDLFile, "\n" );
    fprintf ( fpHDLFile, "#include \"%s.h\"\n",  _pszOutputFileRoot );
    fprintf ( fpHDLFile, "\n" );

    // Generate the DISPID's, one for each member of each class

    fprintf ( fpLOGFile, "Generating DISPID's...\n" );
    GenerateClassDISPIDs();
    GenerateInterfaceDISPIDs();
    GenerateEventDISPIDs( fpHDLFile, TRUE );

    fprintf ( fpHDLFile, "\n" );
    fprintf ( fpHDLFile, "#endif _hxx_\n" );
    fprintf ( fpHDLFile, "\n" );
    fprintf ( fpHDLFile, "#undef _hxx_\n" );
    fprintf ( fpHDLFile, "\n" );

    fprintf ( fpHDLFile, "#ifdef _cxx_\n" );
    fprintf ( fpHDLFile, "\n" );

    // Generate the enum definitions
    fprintf ( fpLOGFile, "Generating CPP Enum Defs...\n" );
    GenerateCPPEnumDefs();

    // Generate the property descriptors
    fprintf ( fpLOGFile, "Generating PROPDESC's...\n" );
    if ( !GeneratePROPDESCs() )
        goto Error;

    // For each TYPE_CLASS in this file generate a propdesc array and vtable
    // array for the class.
    while ( pClassToken = ThisFilesList.GetNext( TYPE_CLASS ) )
    {
        // Generate propdescs for every property token in every class ( in this file )
        fprintf ( fpLOGFile, "Generating PROPDESC Arrays...\n" );

        if ( !GeneratePROPDESCArray(pClassToken, &numPropDescs) )
            goto Error;

        // Generate the vtable array for classes in this file
        fprintf ( fpLOGFile, "Generating VTable Arrays...\n" );

        GenerateVTableArray(pClassToken, &numVTblEntries);

        // Generate the propdesc array in vtable order for classes in this file
        fprintf ( fpLOGFile, "Generating propdesc Arrays in vtbl order...\n" );

        GeneratePropDescsInVtblOrder(pClassToken, &numVTblPropDescs);

        // Abstract classes don't have propdesc arrays or vtable arrays.
        if ( !pClassToken -> IsSet ( CLASS_ABSTRACT ) )
        {
            fprintf ( fpHDLFile, "\nHDLDESC %s::s_apHdlDescs = { ",
                      pClassToken -> GetTagValue ( CLASS_NAME ) );

            // Store the Mondo DISPID int he HDLDesc so we can find it for GetTypeInfo
            if ( HasMondoDispInterface ( pClassToken ) )
            {
                CString szDispName;

                // Map to mondo dispinterface as default dispatch interface.
                szDispName = (pClassToken->IsSet(CLASS_COCLASSNAME)) ?
                                pClassToken->GetTagValue (CLASS_COCLASSNAME) :
                                pClassToken->GetTagValue(CLASS_NAME);
                fprintf ( fpHDLFile, " &DIID_Disp%s,", (LPCSTR)szDispName );
            }
            else
            {
                fprintf ( fpHDLFile, " NULL," );
            }

            if ( numPropDescs )
            {
                fprintf ( fpHDLFile, "%s::s_ppropdescs, %u, ",
                          pClassToken -> GetTagValue ( CLASS_NAME ),
                          numPropDescs );
            }
            else
            {
                fprintf ( fpHDLFile, "NULL, 0, " );
            }

            if ( numVTblEntries )
            {
                fprintf ( fpHDLFile, "%s::s_apVTableInterf, %u, ",
                          pClassToken -> GetTagValue ( CLASS_NAME ),
                          numVTblEntries );
            }
            else
            {
                fprintf ( fpHDLFile, "NULL, 0, " );
            }

            if ( numVTblPropDescs )
            {
                fprintf ( fpHDLFile, "%s::s_ppropdescsInVtblOrder%s};\n",
                          pClassToken->GetTagValue(CLASS_NAME), pClassToken->GetTagValue(CLASS_INTERFACE) );
            }
            else
            {
                fprintf ( fpHDLFile, "NULL};\n" );
            }
        }
    }

    fprintf ( fpLOGFile, "Generating Property Methods...\n" );
    if ( !GeneratePropMethodImplementation() )
        goto Error;

    fprintf ( fpLOGFile, "Generating Cascaded Property Method Implementations...\n" );

    fprintf ( fpHDLFile, "//    Cascaded Property get method implementations\n\n" );

    ThisFilesList.Reset();
    while ( pClassToken = ThisFilesList.GetNext( TYPE_CLASS ) )
    {
        GenerateGetAAXImplementations(pClassToken);
        GenerateThunkContextImplemenation(pClassToken);
    }

    fprintf ( fpHDLFile, "\n" );
    fprintf ( fpHDLFile, "#endif _cxx_\n" );
    fprintf ( fpHDLFile, "\n" );
    fprintf ( fpHDLFile, "#undef _cxx_\n" );
    fprintf ( fpHDLFile, "\n" );

    fprintf ( fpLOGFile, "Generating Class Includes...\n" );

    if ( !GenerateClassIncludes() )
        goto Error;

    return TRUE;

Error:
    return FALSE;
}

FILE *OpenMaxlengthFile(LPCSTR pszPDLFileName, LPCSTR pszOutputPath)
{
    char  chMaxLenFileName[255];
    FILE *fpMaxLenFile = NULL;
    BOOL  fOpenNew = TRUE;

    strcpy(chMaxLenFileName, pszOutputPath);
    strcat(chMaxLenFileName, FILENAME_SEPARATOR_STR "maxlen.txt");

    fpMaxLenFile = fopen(chMaxLenFileName, "r");
    if (fpMaxLenFile)
    {
        char chMarker[6];
        if (fread(chMarker, sizeof(char), 5, fpMaxLenFile))
        {
            if (!_stricmp(chMarker, "XXXXX"))
            {
                fOpenNew = FALSE;
            }
        }

        fclose(fpMaxLenFile);
    }

    if (fOpenNew)
    {
        fpMaxLenFile = fopen(chMaxLenFileName, "w");
        if (fpMaxLenFile)
        {
            fprintf(fpMaxLenFile, "XXXXX Key Value Glossary:\n");
            fprintf(fpMaxLenFile, "-------------------------------\n");
            fprintf(fpMaxLenFile, "pdlUrlLen = 4096         // url strings\n");
            fprintf(fpMaxLenFile, "pdlToken = 128           // strings that really are some form of a token\n"); 
            fprintf(fpMaxLenFile, "pdlLength = 128          // strings that really are numeric lengths\n");
            fprintf(fpMaxLenFile, "pdlColor = 128           // strings that really are color values\n");
            fprintf(fpMaxLenFile, "pdlNoLimit = 0xFFFF      // strings that have no limit on their max lengths\n");
            fprintf(fpMaxLenFile, "pdlEvent = pdlNoLimit    // strings that could be assigned to onfoo event properties\n\n");
            fprintf(fpMaxLenFile, "MAX LENGTH CONSTANTS FOR OM PROPERTIES AND METHODS\n");
            fprintf(fpMaxLenFile, "-----------------------------------------------------------------------------------\n\n");
        }
    }
    else
    {
        fpMaxLenFile = fopen(chMaxLenFileName, "a");
    }

    return fpMaxLenFile;
}

int
CPDLParser::Parse ( char *szInputFile,
    char *szOutputFileRoot,
    char *szPDLFileName,
    char *szOutputPath,
    BOOL fDebugging )
{
#if 0
if (strcmp(szPDLFileName, "window.pdl") == 0)
    assert(0);
#endif

    int nReturnCode = 0;

    char szFileName [ MAX_PATH+1 ];
    char szErrorText [ MAX_LINE_LEN+1 ];

    _pszPDLFileName = szPDLFileName;
    _pszOutputFileRoot = szOutputFileRoot;
    _pszInputFile = szInputFile;
    _pszOutputPath = szOutputPath;

    fpMaxLenFile = OpenMaxlengthFile(szPDLFileName, szOutputPath);

    // Read the input file a line at a time, for each line tokenise and
    // parse
    strcpy ( szFileName, szOutputPath );
    strcat ( szFileName, "LOG" );

    fpLOGFile = fopen ( szFileName, "w" );
    if ( !fpLOGFile )
    {
        printf ( szErrorText, "Can't open log file %s\n", szFileName );
        ReportError ( szErrorText );

        goto error;
    }

    fprintf ( fpLOGFile, "InputBuffer = %s\n", szInputFile );
    fprintf ( fpLOGFile, "OuputFileRoot = %s\n", szOutputFileRoot );
    fprintf ( fpLOGFile, "PDLFileName = %s\n", szPDLFileName );
    fprintf ( fpLOGFile, "LogFileName =% s\n", szFileName );

    // All files open and raring to go....
    if ( !ParseInputFile ( fDebugging ) )
    {
        goto error;
    }

    // Create the HDL File
    strcpy ( szFileName, szOutputFileRoot );
    strcat ( szFileName, ".hdl" );

    fpHDLFile = fopen ( szFileName, "w" );
    if ( !fpHDLFile )
    {
        printf ( szErrorText, "Can't open HDL output file %s\n", szFileName );
        ReportError ( szErrorText );

        goto error;
    }

    // Create the IDL File
    strcpy ( szFileName, szOutputFileRoot );
    strcat ( szFileName, ".idl" );

    fpIDLFile = fopen ( szFileName, "w" );
    if ( !fpIDLFile )
    {
        printf ( szErrorText, "Can't open IDL output file %s\n", szFileName );
        ReportError ( szErrorText );
        goto error;
    }

    // Create the external Header file for the SDK users
    // Create the HDL File
    strcpy ( szFileName, szOutputFileRoot );
    strcat ( szFileName, ".h" );    // For now

    fpHeaderFile = fopen ( szFileName, "w" );
    if ( !fpHeaderFile )
    {
        printf ( szErrorText, "Can't open Header output file %s\n", szFileName );
        ReportError ( szErrorText );
        goto error;
    }

    // Create the external DISPIDs file for the SDK users
    strcpy ( szFileName, szOutputFileRoot );
    strcat ( szFileName, ".dsp" );    // For now

    fpDISPIDFile = fopen ( szFileName, "w" );
    if ( !fpDISPIDFile )
    {
        printf ( szErrorText, "Can't open DISPID output file %s\n", szFileName );
        ReportError ( szErrorText );
        goto error;
    }

    // Create function signatures required for the custom OLEAutomation invoke.
    if ( !LoadSignatures (szOutputPath) )
    {
        ReportError ( "Signature file missing" );
        goto error;
    }

#if COLLECT_STATISTICS==1
    LoadStatistics (szOutputPath);
#endif

    // Parsed Successfully - generate HDL file
    if ( !GenerateHDLFile () )
    {
        printf ( szErrorText, "Can't create HDL output file %s%s.hdl\n",
                 szOutputFileRoot, szFileName );
        ReportError ( szErrorText );

        goto error;
    }

    strcpy ( szFileName, szOutputFileRoot );
    strcat ( szFileName, ".idl" ); // For now hxx
    if ( !GenerateIDLFile( szFileName ) )
    {
        printf ( szErrorText, "Can't create IDL file %s\n", szFileName );
        ReportError ( szErrorText );

        goto error;
    }

    // Generate the .H file with just enums & interface decls in
    GenerateHeaderFile();

    // Generate the external DISPID's file
    GenerateExternalInterfaceDISPIDs();
    GenerateEventDISPIDs ( fpDISPIDFile, FALSE );

    // Create/Open the HTML index file
    strcpy ( szFileName, szOutputPath );
    strcat ( szFileName, FILENAME_SEPARATOR_STR "AllIndex.htm" );
    fpHTMIndexFile = fopen ( szFileName, "a+" );

/* rgardner - commented out for now - not very up-to-date or useful any more
    // Create the HTM File
    strcpy ( szFileName, szOutputFileRoot );
    strcat ( szFileName, ".htm" );

    fpHTMFile = fopen ( szFileName, "w" );

    if ( !GenerateHTMFile( ) )
    {
        printf ( szErrorText, "Can't create HTM file %s\n", szFileName );
        ReportError ( szErrorText );

        goto error;
    }
*/

    // Update the signature file is any changes.
    if (!SaveSignatures( szOutputPath ))
    {
        ReportError ( "Signature file save problem." );
        goto error;
    }

#if COLLECT_STATISTICS==1
    SaveStatistics (szOutputPath);
#endif

    goto cleanup;

error:
    if ( nReturnCode == 0 )
        nReturnCode = 1;

cleanup:
    return nReturnCode;

}


#if COLLECT_STATISTICS==1

void CPDLParser::LoadStatistics ( char *pszOutputPath )
{
    BOOL        bRetVal;
    char       *buffer = NULL;
    CString     szFileName;
    FILE       *fpStatFile = NULL;

    szFileName = pszOutputPath;
    szFileName += FILENAME_SEPARATOR_STR "stats.dat";

    fpStatFile = fopen ( szFileName, "r" );
    if ( !fpStatFile )
    {
        char    chInitFuncSig[2] = { '\0', '\0' };

        fpStatFile = fopen ( szFileName, "w+");
        if ( !fpStatFile )
        {
            ReportError ( "Can't create statistics file" );
            goto error;
        }

        fwrite ( chInitFuncSig, 1, sizeof(chInitFuncSig), fpStatFile );
    }

    if ( fseek( fpStatFile, 0, SEEK_END ) == 0 )
    {
        fpos_t  pos;

        if ( fgetpos( fpStatFile, &pos ) == 0 )
        {
            int     i = 0;
            int     cLines;
            
            buffer = new char[pos];
            if (buffer == NULL)
                goto error;

            fseek( fpStatFile, 0, SEEK_SET );

            if ( fread ( buffer, 1, pos, fpStatFile ) != pos )
                goto error;

            // Intialize to 0.
            for (i = 0; i < MAX_STATS; i++)
                rgcStats[i] = 0;

            // Populate the statics array.
            i = 0;
            cLines = 0;
            while ( buffer[i] || buffer[i + 1] )
            {
                int     cStr = strlen(buffer + i);

                rgcStats[cLines] = atol(buffer + i);

                cLines ++;

                i += cStr + 1;
            }
        }
    }

    bRetVal = TRUE;

cleanup:
    delete buffer;

    fclose ( fpStatFile );

    return;

error:
    bRetVal = FALSE;
    goto cleanup;
}


void CPDLParser::SaveStatistics ( char *pszOutputPath )
{
//DebugBreak();
    BOOL        bRetVal = TRUE;
    if (rgcStats)
    {
        int         i;
        CString     szFileName;
        FILE       *fpStatFile = NULL;
        char        buffer[32];

        szFileName = pszOutputPath;
        szFileName += FILENAME_SEPARATOR_STR "stats.dat";

        fpStatFile = fopen ( szFileName, "w" );

        // Write array to file.
        if ( fseek( fpStatFile, 0, SEEK_SET ) != 0 )
            return;

        i = 0;
        while (i < MAX_STATS)
        {
            sprintf( buffer, "%i", rgcStats[i] );
            fwrite ( buffer, 1, strlen(buffer) + 1, fpStatFile );

            i++;
        }

        // Double NULL at the end.
        buffer[0] = '\0'; 
        buffer[1] = '\0'; 
        fwrite ( &buffer, 1, 2, fpStatFile );

        fclose(fpStatFile);
    }

    return;
}

#endif


void CPDLParser::RemoveSignatures()
{
    if (rgszSignatures)
    {
        int     i = cSignatures;

        while (i--)
            delete [] rgszSignatures[i];

        delete [] rgszSignatures;

        rgszSignatures = NULL;
    }

    if (rgszIIDs)
    {
        int     i = cIIDs;

        while (i--)
            delete [] rgszIIDs[i];

        delete [] rgszIIDs;

        rgszIIDs = NULL;
    }
}


BOOL CPDLParser::LoadSignatures ( char *pszOutputPath )
{
    BOOL        bRetVal;
    char       *buffer = NULL;
    CString     szFileName;
    FILE       *fpSigFile = NULL;

    rgszSignatures = NULL;
    cOnFileSignatures = 0;
    cOnFileIIDs = 0;
    cSignatures = 0;
    cIIDs = 0;

    szFileName = pszOutputPath;
    szFileName += FILENAME_SEPARATOR_STR "funcsig.dat";

/*
if (strcmp(_pszPDLFileName, "select.pdl") == 0 || strcmp(_pszPDLFileName, "header.pdl") == 0)
    __asm { int 3 };
*/

    fpSigFile = fopen ( szFileName, "rb" );
    if ( !fpSigFile )
    {
        char    chInitFuncSig[6] = { '\0', '\0', '\0', '\0', '\0', '\0' };

        fpSigFile = fopen ( szFileName, "w+");
        if ( !fpSigFile )
        {
            ReportError ( "Can't create function signature file" );
            goto error;
        }

        fwrite ( chInitFuncSig, 1, sizeof(chInitFuncSig), fpSigFile );
    }

    if ( fseek( fpSigFile, 0, SEEK_END ) == 0 )
    {
        fpos_t  pos;

        if ( fgetpos( fpSigFile, &pos ) == 0 )
        {
            int     i = 0;
            int     cLines;
            
            buffer = new char[pos];
            if (buffer == NULL)
                goto error;

            fseek( fpSigFile, 0, SEEK_SET );

            fread ( buffer, 1, pos, fpSigFile );

            // Number of entries for each signatures and IIDs
            cOnFileSignatures = buffer[0];      // First byte is # signature
            cOnFileIIDs = buffer[1];            // 2nd byte is # IIDs

            // Pre-allocate the signature array.
            rgszSignatures = new char *[cOnFileSignatures + 1];
            if ( !rgszSignatures )
                goto error;

            // Pre-allocate the IIDs array.
            rgszIIDs = new char *[cOnFileIIDs + 1];
            if ( !rgszIIDs )
                goto error;

            // Intialize to NULL for error handling.
            for (i = 0; i <= cOnFileSignatures; i++)
                rgszSignatures[i] = NULL;

            cSignatures = cOnFileSignatures;

            for (i = 0; i <= cOnFileIIDs; i++)
                rgszIIDs[i] = NULL;

            cIIDs = cOnFileIIDs;

            // Populate the signature array.
            i = 4;
            cLines = 0;
            while ( cLines != cSignatures )
            {
                int     cStr = strlen(buffer + i);

                rgszSignatures[cLines] = new char[cStr + 1];
                if (!rgszSignatures[cLines])
                    goto error;

                strcpy(rgszSignatures[cLines], buffer + i);

                cLines ++;

                i += cStr + 1;
            }

            // Populate the IIDs array.
            cLines = 0;
            while ( cLines != cIIDs )
            {
                int     cStr = strlen(buffer + i);

                rgszIIDs[cLines] = new char[cStr + 1];
                if (!rgszIIDs[cLines])
                    goto error;

                strcpy(rgszIIDs[cLines], buffer + i);

                cLines ++;

                i += cStr + 1;
            }
        }
    }

    bRetVal = TRUE;

cleanup:
    fclose ( fpSigFile );

    return bRetVal;

error:
    delete [] buffer;

    RemoveSignatures();

    bRetVal = FALSE;
    goto cleanup;
}


BOOL CPDLParser::SaveSignatures ( char *pszOutputPath )
{
    if (rgszSignatures || rgszIIDs)
    {
        if (cOnFileSignatures != cSignatures || cOnFileIIDs != cIIDs)
        {
            int         i;
            CString     szFileName;
            FILE       *fpSigFile = NULL;
            char        cHeaderFuncSig[4] = { '\0', '\0', '\0', '\0' };

            szFileName = pszOutputPath;
            szFileName += FILENAME_SEPARATOR_STR "funcsig.dat";

            fpSigFile = fopen ( szFileName, "wb" );

            // Write array to file.
            if ( fseek( fpSigFile, 0, SEEK_SET ) != 0 )
                return FALSE;

            // Write out the header on count of each type.
            cHeaderFuncSig[0] = (char)cSignatures;
            cHeaderFuncSig[1] = (char)cIIDs;
            fwrite ( cHeaderFuncSig, 1, sizeof(cHeaderFuncSig), fpSigFile );

            i = 0;
            while (i < cSignatures)
            {
                if ( rgszSignatures[i] )
                {
                    fwrite ( rgszSignatures[i], 1, strlen(rgszSignatures[i]) + 1, fpSigFile );
                }

                i++;
            }

            i = 0;
            while (i < cIIDs)
            {
                if ( rgszIIDs[i] )
                {
                    fwrite ( rgszIIDs[i], 1, strlen(rgszIIDs[i]) + 1, fpSigFile );
                }

                i++;
            }

            // Write out extra zero to mark end of file.
            cHeaderFuncSig[0] = '\0';
            fwrite ( cHeaderFuncSig, 1, 1, fpSigFile );

            fclose(fpSigFile);
        }

        // Dispose of signature array (IIDs is disposed of too).
        RemoveSignatures();
    }

    return TRUE;
}


BOOL CPDLParser::FindAndAddSignature ( LPCSTR szType, LPCSTR szSignature, LPSTR pszInvokeMethod )
{
    BOOL        bRetVal = NULL;
    CString     szLookup;
    char      **rgNewArray = NULL;

    if (pszInvokeMethod && rgszSignatures)
    {
        char    *szWork;

        strcpy( pszInvokeMethod,  "ERROR in PDLParse: No custom invoke method");

        // Any empty string for type or signature is an error.
        if (!szType[0] || !szSignature[0])
        {
            return FALSE;
        }

        // Replace all * with p (e.g., BSTR * is BSTRP and long * is longP).
        while ((szWork = strchr(szSignature, '*')) != NULL)
            *szWork = 'p';        

        // Replace all ( with P.
        while ((szWork = strchr(szSignature, '(')) != NULL)
            *szWork = 'P';        

        // Replace all ) with P.
        while ((szWork = strchr(szSignature, ')')) != NULL)
            *szWork = 'P';        

        szLookup = szType;
        szLookup += "_";
        szLookup += szSignature;

        // Look for the signature
        for (int i = 0; i < cSignatures; i++)
        {
            if ( strcmp ( rgszSignatures[i], szLookup ) == 0 )
            {
                goto success;
            }
        }

        // If not found then add this signature.
        cSignatures++;

        rgNewArray = new char *[cSignatures + 1];   // Take into account the NULL at end.
        if (!rgNewArray)
            goto cleanup;

        memcpy(rgNewArray, rgszSignatures, sizeof(char *) * cSignatures);

        rgNewArray[cSignatures - 1] = new char[szLookup.Length() + 1];

        strcpy(rgNewArray[cSignatures - 1], szLookup);

        rgNewArray[cSignatures] = NULL;

        delete rgszSignatures;

        rgszSignatures = rgNewArray;

success:
        strcpy(pszInvokeMethod, "IDX_");
        strcat(pszInvokeMethod, (LPCSTR)szLookup);
        bRetVal = TRUE;
    }

cleanup:
    return bRetVal;
}


int CPDLParser::FindAndAddIIDs ( CString szInterface )
{
    char      **rgNewArray = NULL;

    // Any empty string interface name is an error.
    if (szInterface.Length() == 0)
        return -1;          // Error.

    // Look for the signature
    for (int i = 0; i < cIIDs; i++)
    {
        if ( strcmp ( rgszIIDs[i], (LPCSTR)szInterface ) == 0 )
        {
            goto success;
        }
    }

    // If not found then add this IID.
    cIIDs++;

    rgNewArray = new char *[cIIDs + 1];   // Take into account the NULL at end.
    if (!rgNewArray)
        return -1;

    memcpy(rgNewArray, rgszIIDs, sizeof(char *) * cIIDs);

    rgNewArray[cIIDs - 1] = new char[szInterface.Length() + 1];

    strcpy(rgNewArray[cIIDs - 1], (LPCSTR)szInterface);

    rgNewArray[cIIDs] = NULL;

    delete rgszIIDs;

    rgszIIDs = rgNewArray;

success:
    return i;
}


BOOL CPDLParser::GenerateIDLFile ( char *szFileName )
{
    CTokenListWalker ThisFileList ( pRuntimeList, _pszPDLFileName );
    Token * pInterfaceToken;
    Token * pClassToken;
    Token * pEnumToken;
    Token * pStructToken;
    
    ThisFileList.Reset();
    while ( pInterfaceToken = ThisFileList.GetNext ( TYPE_EVENT ) )
    {
        if ( !pInterfaceToken -> IsSet ( EVENT_ABSTRACT ) &&
            pInterfaceToken -> IsSet ( EVENT_GUID ) )
        {
            GenerateIDLInterfaceDecl ( pInterfaceToken,
                pInterfaceToken -> GetTagValue ( EVENT_GUID ),
                pInterfaceToken -> GetTagValue ( EVENT_SUPER ) );
        }
    }

    ThisFileList.Reset();
    while (pEnumToken = ThisFileList.GetNext(TYPE_ENUM))
    {
        GenerateIncludeEnum(pEnumToken, FALSE, fpIDLFile);
    }
    
    //
    // Generate all the structs
    //

    ThisFileList.Reset();
    while (pStructToken = ThisFileList.GetNext(TYPE_STRUCT))
    {
        GenerateStruct(pStructToken, fpIDLFile);
    }
    
    ThisFileList.Reset();
    while ( pInterfaceToken = ThisFileList.GetNext ( TYPE_INTERFACE ) )
    {
        if ( !pInterfaceToken -> IsSet ( INTERFACE_ABSTRACT ) &&
            pInterfaceToken -> IsSet ( INTERFACE_GUID ) )
        {
            GenerateIDLInterfaceDecl ( pInterfaceToken,
                pInterfaceToken -> GetTagValue ( INTERFACE_GUID ),
                pInterfaceToken -> GetTagValue ( INTERFACE_SUPER ) );
        }
        else  if ( !pInterfaceToken -> IsSet ( INTERFACE_ABSTRACT ) &&
            !pInterfaceToken -> IsSet ( INTERFACE_GUID ) )
        {
            // Generate a forward declare
            CString szInterfaceName;
            
            szInterfaceName = pInterfaceToken -> GetTagValue ( INTERFACE_NAME );
            if ( szInterfaceName != "IDispatch" &&
                szInterfaceName != "IUnknown" )
            {
                fprintf ( fpIDLFile, "interface %s;\n",
                    pInterfaceToken -> GetTagValue ( NAME_TAG ) );
            }
        }
    }

    ThisFileList.Reset();
    while (pClassToken = ThisFileList.GetNext(TYPE_CLASS))
    {
        CTokenListWalker    ChildWalker(pClassToken);
        Token              *pChildToken;
        int                 cImplements = 0;

        // Find out how many implements are in the class.
        while (pChildToken = ChildWalker.GetNext())
        {
            if (pChildToken->GetType() == TYPE_IMPLEMENTS)
            {
                cImplements++;
            }
        }

        // Any class with more than one implements needs a mondodispid to be specified.
        if (pClassToken->IsSet(CLASS_GUID) && cImplements && !pClassToken->IsSet(CLASS_MONDOGUID))
        {
            char szErrorText [ MAX_LINE_LEN+1 ];

            sprintf(szErrorText,
                    "class: %s needs a mondoguid when implements are specified for a coclass.\n",
                    pClassToken->GetTagValue(CLASS_NAME));
            ReportError(szErrorText);
            return FALSE;
        }

        // Generate non-dual dispinterface?  This is determined by the mondoguid
        // keyword being used int the class.  If this guid is specified then
        // we'll generate the mondodisp interface is use it as the default
        // dispatch interface for the coclass.
        if (pClassToken->IsSet(CLASS_MONDOGUID))
        {
            LPSTR   pMondoGUID;
            CString szInterface;

            szInterface = pClassToken->GetTagValue(CLASS_INTERFACE);
            pInterfaceToken = FindInterface(szInterface);
            pMondoGUID = pClassToken->GetTagValue(CLASS_MONDOGUID);

            // If we have a GUID then the dispinterface GUID must be in the
            // range 0x3050f500 to 0x3050f5a0
            if (pMondoGUID[0] != '3' ||
                pMondoGUID[1] != '0' ||
                pMondoGUID[2] != '5' ||
                pMondoGUID[3] != '0' ||
                pMondoGUID[4] != 'f' ||
                pMondoGUID[5] != '5' ||
                !((pMondoGUID[6] >= '0' &&
                   pMondoGUID[6] <= '9') ||
                   pMondoGUID[6] == 'a') ||
                !((pMondoGUID[7] >= '0' &&
                   pMondoGUID[7] <= '9') ||
                   (pMondoGUID[7] >= 'a' &&
                    pMondoGUID[7] <= 'f')))
            {
                char szErrorText [ MAX_LINE_LEN+1 ];

                sprintf(szErrorText,
                        "The mondoguid must be in the range 0x3050f500 to 0x3050f5a0 for class: %s\n",
                        pClassToken->GetTagValue(CLASS_NAME));
                ReportError(szErrorText);
                return FALSE;
            }

            // Generate the default dispinterface
            GenerateIDLInterfaceDecl(pInterfaceToken,
                                     pMondoGUID,
                                     pInterfaceToken->GetTagValue(INTERFACE_SUPER),
                                     TRUE,
                                     pClassToken);
        }

        if ( pClassToken-> IsSet(CLASS_GUID) &&
            pClassToken->IsSet(CLASS_INTERFACE))
        {
            GenerateCoClassDecl(pClassToken);
        }

    }

    return TRUE;
}


void CPDLParser::GenerateIDLInterfaceDecl (Token   *pInterfaceToken,
                                           char    *pszGUID,
                                           char    *pszSuper,
                                           BOOL     fDispInterface/*= FALSE*/,
                                           Token   *pClassToken/*= NULL*/)
{
    BOOL fImplements = FALSE;

    if ( pInterfaceToken -> GetType() == TYPE_EVENT )
    {
        fprintf ( fpIDLFile, "[\n    hidden,\n" );
        fprintf ( fpIDLFile, "    uuid(%s)\n]\ndispinterface %s\n{",
            pszGUID, pInterfaceToken -> GetTagValue ( NAME_TAG ) );
        fprintf ( fpIDLFile, "\nproperties:\nmethods:\n" );
    }
    else
    {
        if (fDispInterface)
        {
            fprintf ( fpIDLFile, "[\n    hidden,\n" );
            fprintf ( fpIDLFile, "    uuid(%s)\n]\ndispinterface Disp%s\n{\nproperties:\nmethods:\n",
                pszGUID,
                pClassToken->IsSet(CLASS_COCLASSNAME) ?
                        pClassToken->GetTagValue(CLASS_COCLASSNAME) :
                        pClassToken->GetTagValue(CLASS_NAME));
        }
        else
        {
            if (!PrimaryTearoff(pInterfaceToken) && (!pszSuper || !*pszSuper))
                ReportError ( "Interfaces w/o tearoff need super:IDispatch\n" );

            if (pInterfaceToken->IsSet(INTERFACE_CUSTOM) && 
                pszSuper && 
                *pszSuper)
            {
                fprintf(fpIDLFile, "[\n    object,\n    pointer_default(unique),\n" );
                fprintf ( fpIDLFile, "    uuid(%s)\n]\ninterface %s : %s\n{\n",
                    pszGUID,
                    pInterfaceToken -> GetTagValue ( NAME_TAG ),
                    pszSuper);
            }
            else
            {
                fprintf ( fpIDLFile, "[\n    odl,\n    oleautomation,\n    dual,\n" );
                fprintf ( fpIDLFile, "    uuid(%s)\n]\ninterface %s : %s\n{\n",
                    pszGUID,
                    pInterfaceToken -> GetTagValue ( NAME_TAG ),
                    PrimaryTearoff(pInterfaceToken) ? "IDispatch" : pszSuper);
            }
        }
    }

    // Any implements in the class if so then we want the order of the mongo dispinterface to be decided
    // by the order of the implements and not the super chain.
    if (fDispInterface)
    {
        CTokenListWalker    ChildWalker(pClassToken);
        Token              *pChildToken;

        while (pChildToken = ChildWalker.GetNext())
        {
            fImplements = pChildToken->GetType() == TYPE_IMPLEMENTS;
            if (fImplements)
                break;
        }
    }

    // Use the super chain for mongo dispinterface?
    if (!fImplements)
        // Yes.
        GenerateMkTypelibDecl(pInterfaceToken, fDispInterface, pClassToken);

    // Any other interfaces exposed in the coclass which are not part of the
    // primary interface chain?  Look for implements keyword.
    if (fDispInterface)
    {
        CTokenListWalker    ChildWalker(pClassToken);
        Token              *pChildToken;
        CString             szInterface;
        Token              *pInterfToken;

        while (pChildToken = ChildWalker.GetNext())
        {
            if (pChildToken->GetType() == TYPE_IMPLEMENTS)
            {
                szInterface = pChildToken->GetTagValue(IMPLEMENTS_NAME);
                pInterfToken = FindInterface(szInterface);
                if (pInterfToken)
                {
                    GenerateMkTypelibDecl(pInterfToken, fDispInterface, pClassToken);
                }
            }
        }
    }

    fprintf(fpIDLFile, "};\n");
}


void CPDLParser::ComputePropType ( Token *pPropertyToken,
                                   CString &szProp, BOOL fComment )
{
    char szText [ MAX_LINE_LEN+1 ];

    szProp = "";

    // Through the index/indextype & index1/indextype1 pdl tags
    // you can provide up to two additional args for the property definition
    sprintf ( szText, fComment ? "/* [in] */ %s %s" : "[in] %s %s",
        pPropertyToken -> GetTagValue ( PROPERTY_INDEXTYPE ),
        pPropertyToken -> GetTagValue ( PROPERTY_INDEX ) );
    pPropertyToken -> AddParam ( szProp, PROPERTY_INDEX, szText );

    sprintf ( szText, fComment ? "/* [in] */ %s %s" : "[in] %s %s",
        pPropertyToken -> GetTagValue ( PROPERTY_INDEXTYPE1 ),
        pPropertyToken -> GetTagValue ( PROPERTY_INDEX1 ));
    pPropertyToken -> AddParam ( szProp, PROPERTY_INDEX1, szText );

    if ( szProp [ 0 ] != '\0' )
        szProp += ",";
}


void CPDLParser::GenerateMkTypelibDecl ( Token *pInterfaceToken, BOOL fDispInterface/* = FALSE*/, Token *pClass /* =NULL */)
{
    Token *pChildToken;
    Token *pArgToken;
    CString szArg;
    CString szProp;
    CString szAutomationType;
    BOOL fFirst;
    CString szInterfaceName,szMethodName,szPropertyName;

    CTokenListWalker ChildWalker ( pInterfaceToken );

    if ( pInterfaceToken -> GetType() == TYPE_EVENT &&
        pInterfaceToken -> IsSet ( EVENT_SUPER ) )
    {
        CTokenListWalker WholeList(pRuntimeList);

        Token *pSuperEvent = WholeList.GetNext ( TYPE_EVENT,
            pInterfaceToken -> GetTagValue ( EVENT_SUPER ) );
        if ( pSuperEvent )
            GenerateMkTypelibDecl ( pSuperEvent );
    }

    // Special non-dual dispinterface for the default when the primary interface
    // is a tearoff.
    if (fDispInterface)
    {
        Token  *pSuperIntf;
        CString szInterface;

        szInterface = pInterfaceToken->GetTagValue(INTERFACE_SUPER);
        pSuperIntf = FindInterface(szInterface);
        if (pSuperIntf)
            GenerateMkTypelibDecl(pSuperIntf, fDispInterface, pClass);
    }

    szInterfaceName = pInterfaceToken->GetTagValue ( INTERFACE_NAME );
    szInterfaceName.ToUpper();


    while ( pChildToken = ChildWalker.GetNext() )
    {
        if ( pChildToken -> GetType() == TYPE_METHOD )
        {
            // if nopropdesc is set, then this method doesn't
            // participate in the typelib/mondo interface. this happens
            // when the method exists in a base class/interface as well.
            if ( pChildToken->IsSet(METHOD_NOPROPDESC) && fDispInterface)
                continue;

            // Does the property name exist in another interface then the primary
            // interface had better have the override.  Otherwise, it's an error
            // MIDL will not allow overloading names.
            if (fDispInterface && pClass)
            {
                CString szPrimaryInterface;
                Token   *pPriInterf;
                Token   *pExclusiveMember;

                szPrimaryInterface = pClass->GetTagValue(CLASS_INTERFACE);
                pPriInterf = FindInterface(szPrimaryInterface);

                // If working on non-primary interface make sure the method isn't overloaded on
                // the primary, if so it better be marked exclusive.
                if (_strcmpi(szInterfaceName, szPrimaryInterface))
                {
                    pExclusiveMember = FindMethodInInterfaceWOPropDesc(pPriInterf, pChildToken, TRUE);
                    if (pExclusiveMember)
                    {
                        char szErrorText [ MAX_LINE_LEN+1 ];

                        if (pExclusiveMember->IsSet(METHOD_EXCLUSIVETOSCRIPT))
                            continue;

                        // Overloaded method -- illegal.
                        sprintf(szErrorText, "method %s:%s is overloaded - illegal.\n",
                                (LPCSTR)pClass->GetTagValue(CLASS_NAME),
                                (LPCSTR)pExclusiveMember->GetTagValue(METHOD_NAME));
                        ReportError(szErrorText);
                        return;
                    }
                }
            }

            if ( pChildToken -> IsSet ( METHOD_VARARG) )
            {
                fprintf ( fpIDLFile, "    [vararg");
            }
            else
            {
                fprintf ( fpIDLFile, "    [");
            }

            szMethodName = pChildToken -> GetTagValue ( METHOD_NAME );
            if (pChildToken->IsSet(METHOD_NOPROPDESC))
            {
                if (pChildToken->IsSet(METHOD_SZINTERFACEEXPOSE))
                    szMethodName = pChildToken->GetTagValue(METHOD_SZINTERFACEEXPOSE);
			}
            szMethodName.ToUpper();

            if ( pChildToken -> IsSet ( METHOD_DISPID ) )
            {
                fprintf ( fpIDLFile, "%sid(DISPID_%s_%s)",
                    pChildToken -> IsSet ( METHOD_VARARG ) ? "," : "",
                    (LPCSTR)szInterfaceName, (LPCSTR)szMethodName );
            }

            CTokenListWalker ArgListWalker ( pChildToken );

            if (pChildToken->IsSet(METHOD_NOPROPDESC) &&
                pChildToken->IsSet(METHOD_SZINTERFACEEXPOSE))
            {
                szMethodName = pChildToken->GetTagValue(METHOD_SZINTERFACEEXPOSE);
            }
            else
            {
                szMethodName = pChildToken->GetTagValue(METHOD_NAME);
            }

            if (fDispInterface)
            {
                szAutomationType = "void";
                while ( pArgToken = ArgListWalker.GetNext() )
                {
                    if (pArgToken->IsSet(METHODARG_RETURNVALUE))
                    {
                        szAutomationType = pArgToken->IsSet(METHODARG_ATYPE ) ?
                            pArgToken->GetTagValue(METHODARG_ATYPE) :
                            pArgToken->GetTagValue(METHODARG_TYPE);
                    }

                    // If the last character is a pointer then the pointer
                    // should be removed because that is for dual C++ style
                    // interface.  DispInterface doesn't need the retval
                    // specified as a parameter hence the need for
                    // HRESULT funcName(BOOL*) instead of BOOL funcName ().
                    int iSzLength = szAutomationType.Length();
                    if (iSzLength && szAutomationType[iSzLength - 1] == '*')
                    {
                        char    szTypeNoPtr[MAX_LINE_LEN+1];

                        strncpy(szTypeNoPtr, szAutomationType, iSzLength - 1);
                        szTypeNoPtr[iSzLength - 1] = '\0';
                        szAutomationType = szTypeNoPtr;
                    }
                }

                fprintf ( fpIDLFile, "] %s %s(",
                    (LPCSTR)szAutomationType,
                    (LPCSTR)szMethodName );

                ArgListWalker.Reset();
            }
            else
            {
                fprintf ( fpIDLFile, "] %s %s(",
                    pChildToken -> GetTagValue ( METHOD_RETURNTYPE ),
                    (LPCSTR)szMethodName );
            }

            fFirst = TRUE;
            while ( pArgToken = ArgListWalker.GetNext() )
            {
                if (!(fDispInterface && pArgToken->IsSet(METHODARG_RETURNVALUE)))
                {
                    szArg = "";
                    if ( !fFirst )
                        fprintf ( fpIDLFile, "," );

#ifndef WIN16_PARSER
                    pArgToken -> AddParamStr ( szArg, METHODARG_DEFAULTVALUE, "defaultvalue(%s)" );
#endif              
                    pArgToken -> AddParam ( szArg, METHODARG_OPTIONAL, "optional" );
                    pArgToken -> AddParam ( szArg, METHODARG_RETURNVALUE, "retval" );
                    pArgToken -> AddParam ( szArg, METHODARG_IN, "in" );
                    pArgToken -> AddParam ( szArg, METHODARG_OUT, "out" );
                    fprintf ( fpIDLFile, "[%s] %s %s",
                        (LPCSTR)szArg,
                        // Fixing a bug in the old code
                        // Should really get the atype - allow the type if atype not set
                        pArgToken -> IsSet ( METHODARG_ATYPE ) ?
                            pArgToken -> GetTagValue ( METHODARG_ATYPE ) :
                            pArgToken -> GetTagValue ( METHODARG_TYPE ),
                        pArgToken -> GetTagValue ( METHODARG_ARGNAME ) );
                    fFirst = FALSE;
                }
            }
            fprintf ( fpIDLFile, ");\n" );
        }
        else // Property
        {
            ComputePropType ( pChildToken, szProp, FALSE );
            szAutomationType = pChildToken -> GetTagValue ( PROPERTY_ATYPE );

            // if nopropdesc is set, then this property doesn't
            // participate in the typelib/mondo interface. this happens
            // when the property exists in a base class/interface as well.
            if (pChildToken->IsSet(PROPERTY_NOPROPDESC) && fDispInterface)
                continue;

            // Does the property name exist in another interface then the primary
            // interface had better have the override.  Otherwise, it's an error
            // MIDL will not allow overloading names.
            if (fDispInterface && pClass)
            {
                CString szPrimaryInterface;
                Token   *pPriInterf;
                Token   *pExclusiveMember;

                szPrimaryInterface = pClass->GetTagValue(CLASS_INTERFACE);
                pPriInterf = FindInterface(szPrimaryInterface);

                // If working on non-primary interface make sure the property isn't overloaded on
                // the primary, if so it better be marked exclusive.
                if (_strcmpi(szInterfaceName, szPrimaryInterface))
                {
                    pExclusiveMember = FindMethodInInterfaceWOPropDesc(pPriInterf, pChildToken, TRUE);
                    if (pExclusiveMember)
                    {
                        char szErrorText [ MAX_LINE_LEN+1 ];

                        if (pExclusiveMember->IsSet(PROPERTY_EXCLUSIVETOSCRIPT))
                            continue;

                        // Overloaded method -- illegal.
                        sprintf(szErrorText, "property %s:%s is overloaded - illegal.\n",
                                (LPCSTR)pClass->GetTagValue(CLASS_NAME),
                                (LPCSTR)pExclusiveMember->GetTagValue(PROPERTY_NAME));
                        ReportError(szErrorText);
                        return;
                    }
                }
            }

            if ( pChildToken -> IsSet ( PROPERTY_SET ) )
            {
#if COLLECT_STATISTICS==1
    // Collect statistics on total number of property sets.
    CollectStatistic(NUM_SETPROPERTY, GetStatistic(NUM_SETPROPERTY) + 1);
    if (FindEnum ( pChildToken ))
        CollectStatistic(NUM_SETENUMS, GetStatistic(NUM_SETENUMS) + 1);
#endif
                // If it's an object valued property, generate a propputref,
                // otherwise generate a propput
                //
                if ( pChildToken -> IsSet ( PROPERTY_OBJECT ) )
                {
                    szArg = "    [propputref";
                }
                else
                {
                    szArg = "    [propput";
                }
                if ( pChildToken -> IsSet ( PROPERTY_DISPID ) )
                {
                    szArg += ", id(DISPID_";

                    szPropertyName = pChildToken->GetTagValue(PROPERTY_NAME);
		            if (pChildToken->IsSet(PROPERTY_NOPROPDESC))
		            {
		                if (pChildToken->IsSet(PROPERTY_SZINTERFACEEXPOSE))
		                    szPropertyName = pChildToken->GetTagValue(PROPERTY_SZINTERFACEEXPOSE);
					}
                    szPropertyName.ToUpper();

                    szArg += (LPCSTR)szInterfaceName;
                    szArg += "_";
                    szArg += (LPCSTR)szPropertyName;
                    szArg += ")";
                }

                pChildToken -> AddParam ( szArg, PROPERTY_DISPLAYBIND, "displaybind" );
                pChildToken -> AddParam ( szArg, PROPERTY_BINDABLE, "bindable" );
                pChildToken -> AddParam ( szArg, PROPERTY_HIDDEN, "hidden" );
                pChildToken -> AddParam ( szArg, PROPERTY_RESTRICTED, "restricted" );
#ifndef WIN16_PARSER
                pChildToken -> AddParam ( szArg, PROPERTY_NONBROWSABLE, "nonbrowsable" );
#endif                
                pChildToken -> AddParam ( szArg, PROPERTY_SOURCE, "source" );

                if (pChildToken->IsSet(PROPERTY_NOPROPDESC) &&
                    pChildToken->IsSet(PROPERTY_SZINTERFACEEXPOSE))
                {
                    szPropertyName = pChildToken->GetTagValue(PROPERTY_SZINTERFACEEXPOSE);
                }
                else
                {
                    szPropertyName = pChildToken->GetTagValue(PROPERTY_NAME);
                }

                if (fDispInterface)
                {
                    fprintf ( fpIDLFile, "%s] void %s(%s%s v);\n",
                        (LPCSTR)szArg, (LPCSTR)szPropertyName,
                        (LPCSTR)szProp, (LPCSTR)szAutomationType );
                }
                else
                {
                    fprintf ( fpIDLFile, "%s] HRESULT %s(%s[in] %s v);\n",
                        (LPCSTR)szArg, (LPCSTR)szPropertyName,
                        (LPCSTR)szProp, (LPCSTR)szAutomationType );
                }
            }

            if ( pChildToken -> IsSet ( PROPERTY_GET ) )
            {
#if COLLECT_STATISTICS==1
    // Collect statistics on total number of property sets.
    CollectStatistic(NUM_GETPROPERTY, GetStatistic(NUM_GETPROPERTY) + 1);
    if (FindEnum ( pChildToken ))
        CollectStatistic(NUM_GETENUMS, GetStatistic(NUM_GETENUMS) + 1);
#endif
                szArg = "    [propget";
                    szArg += ", id(DISPID_";

                    szPropertyName = pChildToken->GetTagValue(PROPERTY_NAME);
		            if (pChildToken->IsSet(PROPERTY_NOPROPDESC))
		            {
		                if (pChildToken->IsSet(PROPERTY_SZINTERFACEEXPOSE))
		                    szPropertyName = pChildToken->GetTagValue(PROPERTY_SZINTERFACEEXPOSE);
					}
                    szPropertyName.ToUpper();

                    szArg += (LPCSTR)szInterfaceName;
                    szArg += "_";
                    szArg += (LPCSTR)szPropertyName;
                    szArg += ")";
                pChildToken -> AddParam ( szArg, PROPERTY_DISPLAYBIND, "displaybind" );
                pChildToken -> AddParam ( szArg, PROPERTY_BINDABLE, "bindable" );
                pChildToken -> AddParam ( szArg, PROPERTY_HIDDEN, "hidden" );
                pChildToken -> AddParam ( szArg, PROPERTY_RESTRICTED, "restricted" );
#ifndef WIN16_PARSER
                pChildToken -> AddParam ( szArg, PROPERTY_NONBROWSABLE, "nonbrowsable" );
#endif                
                pChildToken -> AddParam ( szArg, PROPERTY_SOURCE, "source" );

                if (pChildToken->IsSet(PROPERTY_NOPROPDESC) &&
                    pChildToken->IsSet(PROPERTY_SZINTERFACEEXPOSE))
                {
                    szPropertyName = pChildToken->GetTagValue(PROPERTY_SZINTERFACEEXPOSE);
                }
                else
                {
                    szPropertyName = pChildToken->GetTagValue(PROPERTY_NAME);
                }

                if (fDispInterface)
                {
                    fprintf ( fpIDLFile, "%s] %s %s();\n",
                        (LPCSTR)szArg,
                        (LPCSTR)szAutomationType,
                        (LPCSTR)szPropertyName);
                }
                else
                {
                    fprintf ( fpIDLFile, "%s] HRESULT %s(%s[retval, out] %s * p);\n",
                        (LPCSTR)szArg, (LPCSTR)szPropertyName,
                        (LPCSTR)szProp, (LPCSTR)szAutomationType );
                }
            }

        }
    }
}


void CPDLParser::GenerateMidlInterfaceDecl ( Token *pInterfaceToken, char *pszGUID,
    char *pszSuper )
{
    Token *pChildToken;
    Token *pArgToken;
    CString szArg;
    CString szProp;
    BOOL fFirst;

    if ( pInterfaceToken -> GetType() == TYPE_EVENT )
        return;

    fprintf ( fpIDLFile,  "[\n    local,\n    object,\n    pointer_default(unique),\n"  );
    fprintf ( fpIDLFile,  "    uuid(%s)\n]\ninterface %s : %s\n{\n",
        pInterfaceToken -> GetTagValue ( INTERFACE_GUID ),
        pInterfaceToken -> GetTagValue ( INTERFACE_NAME ),
        pInterfaceToken -> GetTagValue ( INTERFACE_SUPER ) );

    CTokenListWalker ChildWalker ( pInterfaceToken );

    while ( pChildToken = ChildWalker.GetNext() )
    {
        if ( pChildToken -> GetType() == TYPE_METHOD )
        {
            fprintf ( fpIDLFile, "    %s %s(",
                pChildToken -> GetTagValue ( METHOD_RETURNTYPE ),
                pChildToken -> GetTagValue ( METHOD_NAME ) );

            fFirst = TRUE;

            CTokenListWalker ArgListWalker ( pChildToken );
            while ( pArgToken = ArgListWalker.GetNext() )
            {
                szArg = "";
                if ( !fFirst )
                    fprintf ( fpIDLFile, "," );
                pArgToken -> AddParam ( szArg, METHODARG_IN, "in" );
                pArgToken -> AddParam ( szArg, METHODARG_OUT, "out" );
                fprintf ( fpIDLFile, "[%s] %s %s", (LPCSTR)szArg,
                    // Fixing a bug in the old code
                    // Should realy get the atype - allow the type if atype not set
                    pArgToken -> IsSet ( METHODARG_ATYPE ) ?
                        pArgToken -> GetTagValue ( METHODARG_ATYPE ) :
                        pArgToken -> GetTagValue ( METHODARG_TYPE ),
                    pArgToken -> GetTagValue ( METHODARG_ARGNAME ) );
                fFirst = FALSE;
            }
            fprintf ( fpIDLFile, ");\n" );
        }
        else
        {
            // Property
            ComputePropType ( pChildToken, szProp, FALSE );

            if ( pChildToken -> IsSet ( PROPERTY_SET ))
            {
                fprintf ( fpIDLFile, "    HRESULT put_%s(%s [in] %s v);\n",
                    (LPCSTR)pChildToken -> GetTagValue ( PROPERTY_NAME ),
                    (LPCSTR)szProp,
                    pChildToken -> GetTagValue ( PROPERTY_ATYPE) );
            }

            if ( pChildToken -> IsSet ( PROPERTY_GET ))
            {
                fprintf ( fpIDLFile,
                    "    HRESULT get_%s(%s[out] %s * p);\n",
                        pChildToken -> GetTagValue ( PROPERTY_NAME ),
                        (LPCSTR)szProp,
                        pChildToken -> GetTagValue ( PROPERTY_ATYPE));
            }
        }
    }
    fprintf ( fpIDLFile, "}\n" );
}

BOOL CPDLParser::HasMondoDispInterface ( Token *pClassToken )
{
    CTokenListWalker    ChildWalker(pClassToken);
    Token              *pChildToken;
    int                 cImplements = 0;

    // Find out how many implements are in the class.
    while (pChildToken = ChildWalker.GetNext())
    {
        if (pChildToken->GetType() == TYPE_IMPLEMENTS)
        {
            cImplements++;
        }
    }

    // Any class with more than one implements a guid and a mondoguid will have
    // a mondo dispinterface.
    return (pClassToken->IsSet(CLASS_GUID) && cImplements && pClassToken->IsSet(CLASS_MONDOGUID));
}


void CPDLParser::GenerateCoClassDecl ( Token *pClassToken )
{
    CString     szName;
    CString     szDispName;
    CString     szInterfSuper;
    BOOL        fHasMondoDispInterface;
    BOOL        fElement = FALSE;

    if (pClassToken->IsSet(CLASS_COCLASSNAME))
        szName = pClassToken->GetTagValue(CLASS_COCLASSNAME);
    else
        szName = pClassToken->GetTagValue(CLASS_NAME);

    fprintf(fpIDLFile, "[\n    %suuid(%s)\n]\n",
            pClassToken->IsSet(CLASS_CONTROL) ? "control,\n    " : "",
            pClassToken->GetTagValue(CLASS_GUID));
    fprintf(fpIDLFile, "coclass %s\n{\n", (LPCSTR)szName );

    fHasMondoDispInterface = HasMondoDispInterface(pClassToken);

    if (fHasMondoDispInterface)
    {
        // Map to mondo dispinterface as default dispatch interface.
        szDispName = (pClassToken->IsSet(CLASS_COCLASSNAME)) ?
                        pClassToken->GetTagValue (CLASS_COCLASSNAME) :
                        pClassToken->GetTagValue(CLASS_NAME);
    }
    else
    {
        // Map to the primary interface as default dispatch interface.
        szDispName = pClassToken->GetTagValue(CLASS_INTERFACE);
    }

    fprintf(fpIDLFile, "    [default]           %sinterface %s%s;\n",
            fHasMondoDispInterface ? "disp" : "",
            fHasMondoDispInterface ? "Disp" : "",
            (LPCSTR)szDispName);

    if (pClassToken->IsSet(CLASS_EVENTS))
    {
        fprintf(fpIDLFile, "    [source, default]   dispinterface %s;\n",
                pClassToken->GetTagValue(CLASS_EVENTS));
    }

    if (pClassToken->IsSet(CLASS_NONPRIMARYEVENTS1))
    {
        fprintf(fpIDLFile, "    [source]            dispinterface %s;\n",
                pClassToken->GetTagValue(CLASS_NONPRIMARYEVENTS1));
    }
    if (pClassToken->IsSet(CLASS_NONPRIMARYEVENTS2))
    {
        fprintf(fpIDLFile, "    [source]            dispinterface %s;\n",
                pClassToken->GetTagValue(CLASS_NONPRIMARYEVENTS2));
    }
    if (pClassToken->IsSet(CLASS_NONPRIMARYEVENTS3))
    {
        fprintf(fpIDLFile, "    [source]            dispinterface %s;\n",
                pClassToken->GetTagValue(CLASS_NONPRIMARYEVENTS3));
    }
    if (pClassToken->IsSet(CLASS_NONPRIMARYEVENTS4))
    {
        fprintf(fpIDLFile, "    [source]            dispinterface %s;\n",
                pClassToken->GetTagValue(CLASS_NONPRIMARYEVENTS4));
    }

    // Any other interface to expose in the coclass which is part of the primary
    // interface?
    CTokenListWalker    ChildWalker(pClassToken);
    Token              *pChildToken;

    while (pChildToken = ChildWalker.GetNext())
    {
        if (pChildToken->GetType() == TYPE_IMPLEMENTS)
        {
            Token      *pInterf;
            CString     szInterface;

            if (!fElement)
                fElement = !_stricmp((LPSTR)pChildToken->GetTagValue(IMPLEMENTS_NAME), "IHTMLElement");

            szInterface = pChildToken->GetTagValue(IMPLEMENTS_NAME);
            pInterf = FindInterface(szInterface);
            if (pInterf)
            {
                // Is the interface a local one if not then don't check, we
				// only need to check where interfaces are actually used.
		        if (FindInterfaceLocally(szInterface))
                {
                    // If the super isn't specified and it's not a primary interface
                    // then error the super is required for non-primary interfaces.
                    if (_stricmp((LPSTR)pClassToken->GetTagValue(CLASS_INTERFACE),
                                 (LPSTR)pInterf->GetTagValue(INTERFACE_NAME)) &&
                        !pInterf->IsSet(INTERFACE_SUPER) &&
                        !IsPrimaryInterface(szInterface))
                    {
                        char szErrorText [ MAX_LINE_LEN+1 ];

                        sprintf(szErrorText, "Interface %s missing super key.\n",
                                (LPSTR)pInterf->GetTagValue(INTERFACE_NAME));
                        ReportError(szErrorText);

                        return;
                    }
                }

                fprintf(fpIDLFile, "                        interface %s;\n",
                        (LPCSTR)pInterf->GetTagValue(INTERFACE_NAME));
            }
        }
    }

    fprintf(fpIDLFile, "};\n");

    fprintf(fpIDLFile, "cpp_quote(\"EXTERN_C const GUID CLSID_%s;\")\n",
            pClassToken->GetTagValue(CLASS_NAME));

    if (!pClassToken->IsSet(CLASS_EVENTS) && fElement)
    {
        char szErrorText [ MAX_LINE_LEN+1 ];
        sprintf(szErrorText, "Class %s missing events key.\n", pClassToken->GetTagValue(CLASS_NAME));
        ReportError(szErrorText);
        return;
    }
}



void CPDLParser::GenerateEnumDescIDL ( Token *pEnumToken )
{
    Token *pEvalToken;
    CString szName;

    fprintf ( fpIDLFile, "\ntypedef [uuid(%s)] enum _%s {\n" ,
        pEnumToken -> GetTagValue ( ENUM_GUID ),
        pEnumToken -> GetTagValue ( ENUM_NAME ) );

    CTokenListWalker EvalChildList ( pEnumToken );

    while ( pEvalToken = EvalChildList.GetNext() )
    {
        if ( pEvalToken -> IsSet ( EVAL_ODLNAME ) )
        {
            szName = pEvalToken -> GetTagValue ( EVAL_ODLNAME );
        }
        else
        {
            szName = pEnumToken -> GetTagValue ( ENUM_NAME );
            szName += pEvalToken -> GetTagValue ( EVAL_NAME );
        }
        fprintf ( fpIDLFile, "    [helpstring(\"%s\")] %s = %s,\n",
            pEvalToken -> IsSet ( EVAL_STRING ) ?
                pEvalToken -> GetTagValue ( EVAL_STRING ) :
                pEvalToken -> GetTagValue ( EVAL_NAME ),
            (LPCSTR)szName,
            pEvalToken -> GetTagValue ( EVAL_VALUE ) );
    }
    fprintf ( fpIDLFile, "}%s;\n",
        pEnumToken -> GetTagValue ( ENUM_NAME ) );

}


void CPDLParser::ReportError ( LPCSTR szErrorString )
{
    printf ( "%s(0) : error PDL0000: %s", _pszPDLFileName, szErrorString);
    fprintf ( fpLOGFile, "%s(0) : error PDL0000: %s", _pszPDLFileName, szErrorString );
}

void CPDLParser::GenerateHeaderFile ( void )
{
    CTokenListWalker ThisFileList ( pRuntimeList, _pszPDLFileName );
    Token *pImportToken;
    Token *pInterfaceToken;
    Token *pClassToken;
    Token *pEnumToken;
    Token *pStructToken;
    char *pStr;
    CString szCoClassName;
    char szName [ MAX_LINE_LEN+1 ];
    CString szInterfaceName;

    strcpy ( szName, _pszPDLFileName );
    pStr = strstr ( szName, "." );
    if ( pStr )
        *pStr = '\0';
    _strlwr ( szName );


    fprintf ( fpHeaderFile, "#ifndef __%s_h__\n", szName );
    fprintf ( fpHeaderFile, "#define __%s_h__\n\n", szName );

    fprintf ( fpHeaderFile, "/* Forward Declarations */\n" );

    fprintf ( fpHeaderFile, "\nstruct ENUMDESC;\n" );
    // For each import, generate a .h include
    while ( pImportToken = ThisFileList.GetNext ( TYPE_IMPORT ) )
    {
        fprintf ( fpHeaderFile, "\n/* header files for imported files */\n" );
        GenerateIncludeStatement ( pImportToken );
    }

    ThisFileList.Reset();

    // Forward define all the interfaces so we can have defined them in any order
    while ( pInterfaceToken = ThisFileList.GetNext ( TYPE_INTERFACE ) )
    {
        szInterfaceName = pInterfaceToken -> GetTagValue ( INTERFACE_NAME );
        if ( szInterfaceName != "IDispatch" &&
            szInterfaceName != "IUnknown" )
        {
            fprintf ( fpHeaderFile, "\n#ifndef __%s_FWD_DEFINED__\n",
                pInterfaceToken -> GetTagValue ( INTERFACE_NAME ) );
            fprintf ( fpHeaderFile, "#define __%s_FWD_DEFINED__\n",
                pInterfaceToken -> GetTagValue ( INTERFACE_NAME ) );
            fprintf ( fpHeaderFile, "typedef interface %s %s;\n",
                pInterfaceToken -> GetTagValue ( INTERFACE_NAME ),
                pInterfaceToken -> GetTagValue ( INTERFACE_NAME ) ) ;
            fprintf ( fpHeaderFile, "#endif     /* __%s_FWD_DEFINED__ */\n",
                pInterfaceToken -> GetTagValue ( INTERFACE_NAME ) );
        }
    }   

    //
    // Generate all the enums
    //
    
    ThisFileList.Reset();
    while ( pEnumToken = ThisFileList.GetNext ( TYPE_ENUM ) )
    {
        GenerateIncludeEnum(pEnumToken, TRUE);
    }

    //
    // Generate all the structs
    //

    ThisFileList.Reset();
    while (pStructToken = ThisFileList.GetNext(TYPE_STRUCT))
    {
        GenerateStruct(pStructToken, fpHeaderFile);
    }


    ThisFileList.Reset();

    // For each interface generate an extern for the GUID &
    // an interface decl
    while ( pInterfaceToken = ThisFileList.GetNext ( TYPE_INTERFACE ) )
    {
        szInterfaceName = pInterfaceToken -> GetTagValue ( INTERFACE_NAME );
        if ( szInterfaceName != "IDispatch" &&
            szInterfaceName != "IUnknown" &&
            !pInterfaceToken -> IsSet ( INTERFACE_ABSTRACT ) && 
            pInterfaceToken -> IsSet ( INTERFACE_GUID ) )
        {
            GenerateIncludeInterface ( pInterfaceToken );
        }
    }

    ThisFileList.Reset();

    // For each class with a GUID, generate a GUID ref
    while ( pClassToken = ThisFileList.GetNext ( TYPE_CLASS ) )
    {
        if ( pClassToken -> IsSet ( CLASS_GUID ) &&
            !pClassToken -> IsSet ( CLASS_ABSTRACT ) )
        {
            pClassToken -> GetTagValueOrDefault ( szCoClassName,
                CLASS_COCLASSNAME, pClassToken -> GetTagValue ( CLASS_NAME ) );
            fprintf ( fpHeaderFile, "\n\nEXTERN_C const GUID GUID_%s;\n\n",
                pClassToken -> GetTagValue ( CLASS_COCLASSNAME ) );

            if ( HasMondoDispInterface ( pClassToken ) )
            {
                CString szDispName;

                // Map to mondo dispinterface as default dispatch interface.
                szDispName = (pClassToken->IsSet(CLASS_COCLASSNAME)) ?
                                pClassToken->GetTagValue (CLASS_COCLASSNAME) :
                                pClassToken->GetTagValue(CLASS_NAME);
                fprintf(fpHeaderFile, "\n\nEXTERN_C const GUID DIID_Disp%s;\n\n",
                        (LPCSTR)szDispName);
            }
        }
        // Also generate extern definitions for all propdescs in this file
        GeneratePropdescExtern ( pClassToken, FALSE /* Don't recurse */ );

    }



    fprintf ( fpHeaderFile, "\n\n#endif /*__%s_h__*/\n\n", szName );

}

void  CPDLParser::GenerateIncludeStatement ( Token *pImportToken )
{
    char szText [ MAX_LINE_LEN+1 ];
    char *pStr;

    strcpy ( szText, pImportToken -> GetTagValue ( IMPORT_NAME ) );

    if ( pStr = strstr ( szText, "." ) )
    {
        strcpy ( pStr, ".h" );
    }
    else
    {
        strcat ( szText, ".h" );
    }
    fprintf ( fpHeaderFile, "#include \"%s\"\n", szText );
}

void  CPDLParser::GenerateIncludeInterface ( Token *pInterfaceToken )
{
    // Only generate the C++ Form
    Token *pChildToken;
    Token *pArgToken;
    CString szArg;
    CString szProp;
    BOOL fFirst;
    CString szSuper;
    
    if ( pInterfaceToken -> GetType() == TYPE_EVENT )
        return;


    fprintf ( fpHeaderFile, "\n#ifndef __%s_INTERFACE_DEFINED__\n",
        pInterfaceToken -> GetTagValue ( INTERFACE_NAME ) );
    fprintf ( fpHeaderFile, "\n#define __%s_INTERFACE_DEFINED__\n",
        pInterfaceToken -> GetTagValue ( INTERFACE_NAME ) );


    fprintf ( fpHeaderFile,  "\nEXTERN_C const IID IID_%s;\n\n",
        pInterfaceToken -> GetTagValue ( INTERFACE_NAME ) );

    if (pInterfaceToken->IsSet(INTERFACE_CUSTOM) ||
        !PrimaryTearoff(pInterfaceToken))
    {
        szSuper = pInterfaceToken->GetTagValue(INTERFACE_SUPER);
    }
    
    fprintf(fpHeaderFile, "\nMIDL_INTERFACE(\"%s\")\n%s : public %s\n{\npublic:\n",
        pInterfaceToken->GetTagValue(INTERFACE_GUID),
        pInterfaceToken->GetTagValue(INTERFACE_NAME),
        ((LPCSTR)szSuper && *(LPCSTR)szSuper) ? (LPCSTR)szSuper : "IDispatch");

    CTokenListWalker ChildWalker ( pInterfaceToken );

    while ( pChildToken = ChildWalker.GetNext() )
    {
        if ( pChildToken -> GetType() == TYPE_METHOD )
        {
            fprintf ( fpHeaderFile, "    virtual %s STDMETHODCALLTYPE %s(\n            ",
                pChildToken -> GetTagValue ( METHOD_RETURNTYPE ),
                   pChildToken->IsSet(METHOD_SZINTERFACEEXPOSE) ?
                    pChildToken->GetTagValue(METHOD_SZINTERFACEEXPOSE) :
                    pChildToken->GetTagValue(METHOD_NAME));

            fFirst = TRUE;

            CTokenListWalker ArgListWalker ( pChildToken );
            while ( pArgToken = ArgListWalker.GetNext() )
            {
                if ( !fFirst )
                    fprintf ( fpHeaderFile, "," );
                szArg = "";
                pArgToken -> AddParam ( szArg, METHODARG_IN, "in" );
                pArgToken -> AddParam ( szArg, METHODARG_OUT, "out" );

                fprintf ( fpHeaderFile, "/* [%s] */ %s %s", (LPCSTR)szArg,
                    // Fixing a bug in the old code
                    // Should realy get the atype - allow the type if atype not set
                    ConvertType(pArgToken -> IsSet ( METHODARG_ATYPE ) ?
                        pArgToken -> GetTagValue ( METHODARG_ATYPE ) :
                        pArgToken -> GetTagValue ( METHODARG_TYPE )),
                    pArgToken -> GetTagValue ( METHODARG_ARGNAME ) );
                fFirst = FALSE;
            }
            fprintf ( fpHeaderFile, ") = 0;\n\n" );
        }
        else
        {
            // Property
            ComputePropType(pChildToken, szProp, TRUE);

            if ( pChildToken -> IsSet ( PROPERTY_SET ))
            {
                fprintf ( fpHeaderFile, "    virtual HRESULT STDMETHODCALLTYPE put_%s(\n        %s /* [in] */ %s v) = 0;\n\n",
                   	pChildToken->IsSet(PROPERTY_SZINTERFACEEXPOSE) ?
                            (LPSTR)pChildToken->GetTagValue(PROPERTY_SZINTERFACEEXPOSE) :
                            (LPSTR)pChildToken->GetTagValue(PROPERTY_NAME),
                    (LPCSTR)szProp,
                    pChildToken -> GetTagValue ( PROPERTY_ATYPE));

            }

            if ( pChildToken -> IsSet ( PROPERTY_GET ))
            {
                fprintf ( fpHeaderFile,
                    "    virtual HRESULT STDMETHODCALLTYPE get_%s(\n        %s /* [out] */ %s * p) = 0;\n\n",
                           pChildToken->IsSet(PROPERTY_SZINTERFACEEXPOSE) ?
                            (LPSTR)pChildToken->GetTagValue(PROPERTY_SZINTERFACEEXPOSE) :
                            (LPSTR)pChildToken->GetTagValue(PROPERTY_NAME),
                        (LPCSTR)szProp,
                        pChildToken -> GetTagValue ( PROPERTY_ATYPE) );
            }
        }
    }
    fprintf ( fpHeaderFile, "};\n\n" );
    fprintf ( fpHeaderFile, "#endif     /* __%s_INTERFACE_DEFINED__ */\n\n",
        pInterfaceToken -> GetTagValue ( INTERFACE_NAME ) );
}

void CPDLParser::GenerateIncludeEnum(
    Token *pEnumToken, BOOL fSpitExtern, FILE *pFile)
{
    Token *pEvalToken;

    if (!pFile)
    {
        pFile = fpHeaderFile;
    }
    
    fprintf(pFile, "typedef enum _%s\n{\n" ,
        pEnumToken -> GetTagValue ( ENUM_NAME ) );

    CTokenListWalker EvalChildList ( pEnumToken );

    while ( pEvalToken = EvalChildList.GetNext() )
    {
        fprintf(pFile, "    %s%s = %s,\n",
            pEnumToken -> GetTagValue(ENUM_PREFIX),
            pEvalToken -> GetTagValue(EVAL_NAME),
            pEvalToken -> GetTagValue(EVAL_VALUE));
    }
    //  Add an _Max for the MAC build - apparently the mac needs
    // this to indicate that it's an integer
    fprintf(pFile, "    %s_Max = 2147483647L\n",
        pEnumToken -> GetTagValue ( ENUM_NAME ) );
    fprintf(pFile, "} %s;\n\n",
        pEnumToken -> GetTagValue ( ENUM_NAME ) );

    if (fSpitExtern)
    {
        // Generate an EXTERN from the enum descriptor so other hdls only have to include
        // the .h file.
        fprintf(pFile, "\nEXTERN_C const ENUMDESC s_enumdesc%s;\n\n",
            pEnumToken->GetTagValue ( ENUM_NAME ) );
    }
}


void 
CPDLParser::GenerateStruct(Token *pStructToken, FILE *pFile)
{
    Token *pMemberToken;

    fprintf(pFile, "typedef struct _%s\n{\n" ,
        pStructToken->GetTagValue(STRUCT_NAME));

    CTokenListWalker ChildList(pStructToken);

    while (pMemberToken = ChildList.GetNext())
    {
        fprintf(pFile, "    %s %s;\n",
            pMemberToken->GetTagValue(STRUCTMEMBER_TYPE),
            pMemberToken->GetTagValue(STRUCTMEMBER_NAME));
    }
    fprintf(pFile, "} %s;\n\n",
        pStructToken->GetTagValue(STRUCT_NAME));
}


BOOL CPDLParser::AddType ( LPCSTR szTypeName, LPCSTR szHandler )
{
    Token *pTypeToken = pDynamicTypeList->AddNewToken ( (DESCRIPTOR_TYPE)TYPE_DATATYPE );
    if ( pTypeToken == NULL )
        return FALSE;
    if ( !pTypeToken->AddTag ( DATATYPE_NAME, szTypeName ) )
        return FALSE;
    return pTypeToken->AddTag ( DATATYPE_HANDLER, szHandler );
}

BOOL CPDLParser::AddEventType ( LPCSTR szTypeName, LPCSTR szVTSType )
{
    Token *pTypeToken = pDynamicEventTypeList->AddNewToken ( (DESCRIPTOR_TYPE)TYPE_DATATYPE );
    if ( pTypeToken == NULL )
        return FALSE;
    if ( !pTypeToken->AddTag ( DATATYPE_NAME, szTypeName ) )
        return FALSE;
    return pTypeToken->AddTag ( DATATYPE_HANDLER, szVTSType );
}


BOOL CPDLParser::LookupType ( LPCSTR szTypeName, CString &szIntoString,
    CString &szFnPrefix, StorageType *pStorageType /* = NULL */ )
{
    Token *pTypeToken;
    UINT uIndex;
    // Look in the static array first
    if ( ( uIndex = szIntoString.Lookup ( DataTypes, szTypeName ) ) != (UINT)-1 )
    {
        if ( pStorageType )
        {
            *pStorageType = DataTypes [ uIndex ].stStorageType;
            szFnPrefix = DataTypes [ uIndex ].szMethodFnPrefix;
        }
        return TRUE;
    }

    // Look finaly in the dynamic array
    CTokenListWalker TypeList ( pDynamicTypeList );
    if ( pTypeToken = TypeList.GetNext ( (DESCRIPTOR_TYPE)TYPE_DATATYPE, szTypeName ) )
    {
        szIntoString = pTypeToken->GetTagValue ( DATATYPE_HANDLER );
        if ( pStorageType )
        {
            // BUGBUG In the dynamic array are either enums iface ptrs or class ptrs
            // All can be stored in a DWORD, so ...
            *pStorageType = STORAGETYPE_NUMBER;
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL CPDLParser::LookupEventType ( CString &szIntoString, LPCSTR szTypeName )
{
    Token *pTypeToken;
    // Look in the static array first
    if ( szIntoString.Lookup ( vt, szTypeName ) != (UINT)-1 )
    {
        return TRUE;
    }
    // Look finaly in the dynamic array
    CTokenListWalker TypeList ( pDynamicEventTypeList );
    if ( pTypeToken = TypeList.GetNext ( (DESCRIPTOR_TYPE)TYPE_DATATYPE, szTypeName ) )
    {
        szIntoString = pTypeToken->GetTagValue ( DATATYPE_HANDLER );
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

UINT uProps [] =
{
    PROPERTY_MEMBER,
    PROPERTY_ABSTRACT,
    PROPERTY_GETSETMETHODS,
    PROPERTY_CAA,
    PROPERTY_SUBOBJECT
};

// Legal combinations of properties
static struct
{
    UINT uID1;
    UINT uID2;
    UINT uID3;
    UINT uMask;
} PropertyCheck [] =
{
    { PROPERTY_MEMBER, (UINT)-1,(UINT)-1 },
    { PROPERTY_SUBOBJECT, (UINT)-1 ,(UINT)-1 },
    { PROPERTY_SUBOBJECT, PROPERTY_MEMBER,(UINT)-1 },
    { PROPERTY_GETSETMETHODS, (UINT)-1,(UINT)-1 },
    { PROPERTY_ABSTRACT,  (UINT)-1,(UINT)-1},
    { PROPERTY_CAA, (UINT)-1,(UINT)-1 },
    { PROPERTY_SUBOBJECT, PROPERTY_CAA,(UINT)-1 },
};


void CPDLParser::Init ( void )
{
    UINT i,j;
    for ( i = 0 ; i < ARRAY_SIZE ( PropertyCheck ) ; i++ )
    {
        PropertyCheck [ i ].uMask = 0;
        for ( j = 0 ; j < ARRAY_SIZE ( uProps ) ; j++ )
        {
            if ( PropertyCheck [ i ].uID1 == uProps [ j ] )
            {
                PropertyCheck [ i ].uMask |= 1<<j;
            }
            else if ( PropertyCheck [ i ].uID2 == uProps [ j ] )
            {
                PropertyCheck [ i ].uMask |= 1<<j;
            }
            else if ( PropertyCheck [ i ].uID3 == uProps [ j ] )
            {
                PropertyCheck [ i ].uMask |= 1<<j;
            }
        }
    }
}

BOOL CPDLParser::PatchPropertyTypes ( void )
{
    // For each property declaration, set the object flag if the
    // Handler for the type is object
    CTokenListWalker WholeList ( pRuntimeList );
    Token *pToken;
    Token *pChildToken;
    CString szHandler;
    CString szFnPrefix;
    CString szAType;
    CString szName;
    BOOL fMatched;
    char szErrorText [ MAX_LINE_LEN+1 ];
    UINT i;
    UINT uMask;

    while ( pToken = WholeList.GetNext() )
    {
        if ( pToken -> GetType() == TYPE_EVENT ||
            pToken -> GetType() == TYPE_CLASS ||
            pToken -> GetType() == TYPE_INTERFACE )
        {
            if ( pToken -> GetType() == TYPE_CLASS )
            {
                // Has it got a super, if so is it referenced
                if ( pToken -> IsSet ( CLASS_SUPER ) )
                {
                    CTokenListWalker AllList ( pRuntimeList );
                    if ( !AllList.GetNext ( TYPE_CLASS, pToken -> GetTagValue ( CLASS_SUPER ) ) )
                    {
                        sprintf ( szErrorText, "Class %s References unknown super:%s\n",
                            (LPCSTR)pToken->GetTagValue ( CLASS_NAME ),
                            (LPCSTR)pToken->GetTagValue ( CLASS_SUPER ) );
                        ReportError ( szErrorText );
                        return FALSE;
                    }
                }
                if ( pToken -> IsSet ( CLASS_INTERFACE ) )
                {
                    CTokenListWalker AllList ( pRuntimeList );
                    if ( !AllList.GetNext ( TYPE_INTERFACE, pToken -> GetTagValue ( CLASS_INTERFACE ) ) )
                    {
                        sprintf ( szErrorText, "Class %s References unknown interface:%s\n",
                            (LPCSTR)pToken->GetTagValue ( CLASS_NAME ),
                            (LPCSTR)pToken->GetTagValue ( CLASS_INTERFACE ) );
                        ReportError ( szErrorText );
                        return FALSE;
                    }
                }
                if ( pToken -> IsSet ( CLASS_EVENTS ) )
                {
                    CTokenListWalker AllList ( pRuntimeList );
                    if ( !AllList.GetNext ( TYPE_EVENT, pToken -> GetTagValue ( CLASS_EVENTS ) ) )
                    {
                        sprintf ( szErrorText, "Class %s References unknown events:%s\n",
                            (LPCSTR)pToken->GetTagValue ( CLASS_NAME ),
                            (LPCSTR)pToken->GetTagValue ( CLASS_EVENTS ) );
                        ReportError ( szErrorText );
                        return FALSE;
                    }
                }
                // If you have an event set , you must have a coclass
                if ( pToken -> IsSet ( CLASS_EVENTS ) && !pToken -> IsSet ( CLASS_ABSTRACT ) &&
                    !pToken -> IsSet ( CLASS_GUID ) )
                {
                    sprintf ( szErrorText, "Non abstract class %s has an event set but no GUID\n",
                        (LPCSTR)pToken->GetTagValue ( CLASS_NAME ) );
                    ReportError ( szErrorText );
                    return FALSE;
                }
            }
            else if ( pToken -> GetType() == TYPE_INTERFACE )
            {
                if ( pToken -> IsSet ( INTERFACE_SUPER ) )
                {
                    CTokenListWalker AllList ( pRuntimeList );
                    if ( !AllList.GetNext ( TYPE_INTERFACE, pToken -> GetTagValue ( INTERFACE_SUPER ) ) )
                    {
                        sprintf ( szErrorText, "Interface %s References unknown super:%s\n",
                            (LPCSTR)pToken->GetTagValue ( INTERFACE_NAME ),
                            (LPCSTR)pToken->GetTagValue ( INTERFACE_SUPER ) );
                        ReportError ( szErrorText );
                        return FALSE;
                    }
                }
            }
            else if ( pToken -> GetType() == TYPE_EVENT )
            {
                if ( pToken -> IsSet ( EVENT_SUPER ) )
                {
                    CTokenListWalker AllList ( pRuntimeList );
                    if ( !AllList.GetNext ( TYPE_EVENT, pToken -> GetTagValue ( EVENT_SUPER ) ) )
                    {
                        sprintf ( szErrorText, "Events %s References unknown super:%s\n",
                            (LPCSTR)pToken->GetTagValue ( EVENT_NAME ),
                            (LPCSTR)pToken->GetTagValue ( EVENT_SUPER ) );
                        ReportError ( szErrorText );
                        return FALSE;
                    }
                }
            }
            CTokenListWalker ChildList ( pToken );
            while ( pChildToken = ChildList.GetNext() )
            {
                if ( pChildToken -> GetType() == TYPE_PROPERTY )
                {
                    // If the Type field is not set, which it won't be for many
                    // abstract properties, set it to the ATYPE. We still
                    // need the type to determine if the property is an object value
                    // property
                    if ( !pChildToken -> IsSet ( PROPERTY_TYPE ) )
                    {
                        pChildToken -> AddTag ( PROPERTY_TYPE,
                            pChildToken -> GetTagValue ( PROPERTY_ATYPE ) );
                    }
                    if ( !LookupType ( pChildToken -> GetTagValue ( PROPERTY_TYPE ),
                        szHandler, szFnPrefix ) )
                    {
                        // Seeing as we don't use the handler for abstract types, we
                        // allow the lookup to fail
                        if ( !pChildToken -> IsSet ( PROPERTY_ABSTRACT ) )
                        {
                            sprintf ( szErrorText, "Invalid Type:%s in %s Property:%s\n",
                                (LPCSTR)pChildToken->GetTagValue ( PROPERTY_TYPE ),
                                (LPCSTR)pToken->GetTagValue ( NAME_TAG ),
                                (LPCSTR)pChildToken->GetTagValue ( PROPERTY_NAME ) );
                            ReportError ( szErrorText );
                            return FALSE;
                        }
                    }

                    // Currently we insist that you automate enums as BSTR's
                    szAType = pChildToken->GetTagValue ( PROPERTY_ATYPE );

                    if ( szHandler == "Enum" && szAType != "BSTR" )
                    {
                        sprintf ( szErrorText, "You must set atype:BSTR for an enum property %s : Property:%s\n",
                            (LPCSTR)pToken->GetTagValue ( NAME_TAG ),
                            (LPCSTR)pChildToken->GetTagValue ( PROPERTY_NAME ) );
                        ReportError ( szErrorText );
                        return FALSE;
                    }

                    if ( szHandler == "Color" && szAType != "VARIANT" )
                    {
                        sprintf ( szErrorText, "You must set atype:VARIANT for an type:CColorValue property %s : Property:%s\n",
                            (LPCSTR)pToken->GetTagValue ( NAME_TAG ),
                            (LPCSTR)pChildToken->GetTagValue ( PROPERTY_NAME ) );
                        ReportError ( szErrorText );
                        return FALSE;
                    }

                    if ( szHandler == "object" &&
                        !pChildToken -> IsSet ( PROPERTY_OBJECT ) )
                    {
                        pChildToken -> Set ( PROPERTY_OBJECT );
                    }
                    // Object valued properties must always be abstract because we don't have
                    // the notion of an "ObjectHandler"
                    if ( pChildToken -> IsSet ( PROPERTY_OBJECT ) &&
                        !pChildToken -> IsSet ( PROPERTY_ABSTRACT ) )
                    {
                        sprintf ( szErrorText, "Object Type Property %s:%s MUST be abstract\n",
                            (LPCSTR)pToken->GetTagValue ( NAME_TAG ),
                            (LPCSTR)pChildToken->GetTagValue ( PROPERTY_NAME ) );
                        ReportError ( szErrorText );
                        return FALSE;
                    }

                    for ( i = 0, uMask = 0 ; i < ARRAY_SIZE ( uProps ) ; i++ )
                    {
                        if ( pChildToken -> IsSet ( uProps [ i ] ) )
                        {
                            uMask |= 1<<i;
                        }
                    }

                    for ( i = 0, fMatched = FALSE;
                        i < ARRAY_SIZE ( PropertyCheck ); i++ )
                    {
                        if ( PropertyCheck [ i ].uMask == uMask )
                        {
                            fMatched = TRUE;
                        }
                    }

                    if ( !fMatched )
                    {
                        sprintf ( szErrorText, "Invalid combination of member/method/abstract/caa on %s:%s\n",
                            (LPCSTR)pToken->GetTagValue ( NAME_TAG ),
                            (LPCSTR)pChildToken->GetTagValue ( PROPERTY_NAME ) );
                        ReportError ( szErrorText );
                        return FALSE;
                    }
                    // Subobject MUST have a GET and MUST NOT have a SET
                    if ( pChildToken -> IsSet ( PROPERTY_SUBOBJECT ) &&
                        ( !pChildToken -> IsSet ( PROPERTY_GET ) || pChildToken -> IsSet ( PROPERTY_SET ) ) )
                    {
                        sprintf ( szErrorText, "Invalid combination of subobject/get/set on %s:%s\n",
                            (LPCSTR)pToken->GetTagValue ( NAME_TAG ),
                            (LPCSTR)pChildToken->GetTagValue ( PROPERTY_NAME ) );
                        ReportError ( szErrorText );
                        return FALSE;
                    }

                    // DISPLAYBIND Always implies BINDABLE - so always set it
                    if ( pChildToken -> IsSet ( PROPERTY_DISPLAYBIND ) &&
                        !pChildToken -> IsSet ( PROPERTY_BINDABLE ) )
                    {
                        pChildToken -> Set ( PROPERTY_BINDABLE );
                    }

                    // For now we limit the enum: type to atypre:VARIANT,
                    if ( pChildToken -> IsSet ( PROPERTY_ENUMREF ) )
                    {
                        if ( szAType != "VARIANT" )
                        {
                            sprintf ( szErrorText, "Invalid combination of atype/enum on %s:%s\n",
                                (LPCSTR)pToken->GetTagValue ( NAME_TAG ),
                                (LPCSTR)pChildToken->GetTagValue ( PROPERTY_NAME ) );
                            ReportError ( szErrorText );
                            return FALSE;
                        }
                    }
                    // Set an internal flag if the property cascades - saves looking this up later
                    if ( pChildToken -> IsSet ( PROPERTY_CAA ) )
                    {
                        const CCachedAttrArrayInfo *pCCAAI = GetCachedAttrArrayInfo( (LPCSTR)pChildToken -> GetTagValue ( PROPERTY_DISPID ) );
                        if ( pCCAAI->szDispId != NULL )
                        {
                            pChildToken -> Set ( PROPERTY_CASCADED );
                        }
                    }
                    if ( !pChildToken -> IsSet ( PROPERTY_DISPID ) )
                    {
                        sprintf ( szErrorText, "Missing compulsory attribute 'dispid' on %s:%s\n",
                            (LPCSTR)pToken->GetTagValue ( NAME_TAG ),
                            (LPCSTR)pChildToken->GetTagValue ( PROPERTY_NAME ) );
                        ReportError ( szErrorText );
                    }
                    // SETATDESIGNTIME augments regular set
                    if ( pChildToken -> IsSet ( PROPERTY_SETDESIGNMODE ) )
                    {
                        pChildToken -> Set ( PROPERTY_SET );
                    }

                }
                else if ( pChildToken -> GetType() == TYPE_METHOD )
                {
                    Token *pArgToken;
                    // For each method arg check
                    // that type is VARIANT if a optional tag is specified
                    CTokenListWalker ArgListWalker ( pChildToken );
                    while ( pArgToken = ArgListWalker.GetNext() )
                    {
                        szAType = pArgToken->GetTagValue ( METHODARG_TYPE );
                        if ( pArgToken->IsSet ( METHODARG_OPTIONAL ) &&
                            !(szAType == "VARIANT" || szAType == "VARIANT*") )
                        {
                            // MIDL will let this through but you'd never be able to
                            // set the default
                            sprintf ( szErrorText, "Method arg type must be VARIANT with optional: tag on %s:%s\n",
                                (LPCSTR)pToken->GetTagValue ( NAME_TAG ),
                                (LPCSTR)pChildToken->GetTagValue ( METHOD_NAME ) );
                            ReportError ( szErrorText );
                            return FALSE;
                        }
                        if ( LookupType ( pArgToken -> GetTagValue ( METHODARG_TYPE ),
                            szHandler, szFnPrefix ) )
                        {
                            if (szHandler == "Enum" && 
                                !(pToken->GetType() == TYPE_INTERFACE && 
                                    pToken->IsSet(INTERFACE_CUSTOM)))
                            {
                                sprintf ( szErrorText, "You must set type:BSTR for an enum %s Method:%s arg:%s\n",
                                    (LPCSTR)pToken->GetTagValue ( NAME_TAG ),
                                    (LPCSTR)pChildToken->GetTagValue ( METHOD_NAME ),
                                    (LPCSTR)pArgToken->GetTagValue ( METHODARG_ARGNAME ) );
                                ReportError ( szErrorText );
                                return FALSE;
                            }
                        }
                    }
                }
            }
        }
    }
    return TRUE;
}


//
// Patch up all interfaces which are not a primary default interfaces tearoff
// and mark as such (NOPRIMARY).  Classes marked as NOPRIMARY will be derived
// from the inheritance chain instead of being derived from IDispatch.
//
BOOL
CPDLParser::PatchInterfaces ()
{
    CTokenListWalker    ThisFilesList(pRuntimeList, _pszPDLFileName);
    Token              *pClassToken;
    Token              *pInterf;

    while (pClassToken = ThisFilesList.GetNext(TYPE_CLASS))
    {
        CString     szInterface;

        szInterface = pClassToken->GetTagValue(CLASS_INTERFACE);

        if (!FindTearoff(pClassToken->GetTagValue(CLASS_NAME),
                         (LPCSTR)szInterface))
        {
            CString szInterfSuper;

            pInterf = FindInterface(szInterface);
            while (pInterf)
            {
                pInterf->Set(INTERFACE_NOPRIMARYTEAROFF);

                szInterfSuper = pInterf->GetTagValue(INTERFACE_SUPER);
                pInterf = FindInterface(szInterfSuper);
            }
        }
    }

    return TRUE;
}


const CCachedAttrArrayInfo*
CPDLParser::GetCachedAttrArrayInfo( LPCSTR szDispId )
{
    CCachedAttrArrayInfo *pCCAAI = rgCachedAttrArrayInfo;

    while (pCCAAI->szDispId)
    {
        if (0==strcmp(szDispId, pCCAAI->szDispId))
            return pCCAAI;
        ++pCCAAI;
    }
    //This is means it is not applied to a XF structure
    return pCCAAI;
}


BOOL CPDLParser::GenerateHTMFile ()
{
    Token *pToken;
    CTokenListWalker WholeList ( pRuntimeList );
    CTokenListWalker ThisFilesList ( pRuntimeList, _pszPDLFileName );

    fprintf ( fpHTMFile, "<HTML>\n" );
    fprintf ( fpHTMFile, "<HEAD>\n" );
    fprintf ( fpHTMFile, "<TITLE>Interface Documentation from %s</TITLE>\n", _pszPDLFileName );
    fprintf ( fpHTMFile, "</HEAD>\n" );
    fprintf ( fpHTMFile, "<BODY>\n" );
    fprintf ( fpHTMFile, "<P>\n" );

    while ( pToken = ThisFilesList.GetNext ( TYPE_ENUM ) )
    {
        if ( !pToken -> IsSet ( ENUM_HIDDEN ))
        {
            fprintf ( fpHTMFile, "<B><U>Enumerations:</U></B> %s\n",
                      pToken->GetTagValue ( ENUM_NAME ) );
            fprintf ( fpHTMFile, "<P>\n" );

            fprintf ( fpHTMFile, "<TABLE WIDTH=70%%>\n" );
            fprintf ( fpHTMFile, "<TR>\n" );
            fprintf ( fpHTMFile, "<TH WIDTH=50%% ALIGN=\"Left\">Name</TH>\n" );
            fprintf ( fpHTMFile, "<TH WIDTH=25%% ALIGN=\"Center\">String</TH>\n" );
            fprintf ( fpHTMFile, "</TR>\n" );

            GenerateEnumHTM ( pToken,
                              pToken -> IsSet ( ENUM_PREFIX ) ?
                                        pToken->GetTagValue ( ENUM_PREFIX ) :
                                        pToken->GetTagValue ( ENUM_NAME ) );  // BUGBUG: sort enums

            fprintf ( fpHTMFile, "</TABLE>\n" );
            fprintf ( fpHTMFile, "<P>\n" );
        }
    }

    // Output interface documentation

    ThisFilesList.Reset();
    while ( (pToken = ThisFilesList.GetNext ( TYPE_INTERFACE )) )
    {
        if (_stricmp("IUnknown", pToken->GetTagValue ( INTERFACE_NAME )) &&
            _stricmp("IDispatch", pToken->GetTagValue ( INTERFACE_NAME )) )
        {
            char achFilePrefix[100];
          
            strcpy (achFilePrefix, _pszPDLFileName);
            char *pDot = strchr(achFilePrefix, '.');
            if (pDot) {
                *pDot = '\0';
            }
            else {
                strcpy(achFilePrefix, "badpdl");
            }

            fprintf ( fpHTMIndexFile, "<A NAME=\"Index_Interface_%s\">Interface: <A HREF=\"%s.htm#Interface_%s\">%s</A><BR>\n",
                        pToken->GetTagValue ( INTERFACE_NAME ),
                        achFilePrefix, pToken->GetTagValue ( INTERFACE_NAME ),
                        pToken->GetTagValue ( INTERFACE_NAME ));
      
            fprintf ( fpHTMFile, "<P>\n" );
            fprintf ( fpHTMFile, "<A NAME=\"Interface_%s\">\n", pToken->GetTagValue ( INTERFACE_NAME ) );
            fprintf ( fpHTMFile, "<TABLE>\n" );
            fprintf ( fpHTMFile, "<TR><TD><H1>Interface</H1></TD>"
                                 "<TR><TD><B>%s</B></TD><TD> GUID: </TD><TD>%s</TD></TR>"
                                 "<TR><TD>inherits from interface</TD></TR>"
                                 "<TR><TD><B><A HREF=\"AllIndex.htm#Index_Interface_%s\">%s</A></B></TD></TR>\n"
                                 "</TABLE>\n",
                                  pToken->GetTagValue ( INTERFACE_NAME ),
                                  pToken->GetTagValue ( INTERFACE_GUID ),
                                  pToken->GetTagValue ( INTERFACE_SUPER ),
                                  pToken->GetTagValue ( INTERFACE_SUPER ) );
            fprintf ( fpHTMFile, "<P>\n" );

            fprintf ( fpHTMFile, "<HR>\n" );

            fprintf ( fpHTMFile, "<H2>Properties</H2>\n" );
            fprintf ( fpHTMFile, "<TABLE>\n" );
            fprintf ( fpHTMFile, "<TR>" );
            fprintf ( fpHTMFile, "<TH>Name</TH>" );
            fprintf ( fpHTMFile, "<TH>AType</TH>" );
            fprintf ( fpHTMFile, "<TH>DISPID</TH>" );
            fprintf ( fpHTMFile, "<TH>G</TH>" );
            fprintf ( fpHTMFile, "<TH>S</TH>" );
            fprintf ( fpHTMFile, "<TH>DT</TH>" );
            fprintf ( fpHTMFile, "<TH>Default</TH>" );
            fprintf ( fpHTMFile, "<TH>Min</TH>" );
            fprintf ( fpHTMFile, "<TH>Max</TH>" );
            fprintf ( fpHTMFile, "</TR>\n" );
            GenerateInterfacePropertiesHTM ( pToken );     // BUGBUG: sort attributes
            fprintf ( fpHTMFile, "</TABLE>" );

            fprintf ( fpHTMFile, "<H2>Methods</H2>\n" );
            fprintf ( fpHTMFile, "<TABLE BORDER=1>\n" );
            fprintf ( fpHTMFile, "<TR>" );
            fprintf ( fpHTMFile, "<TH>Ret. Name</TH>" );
            fprintf ( fpHTMFile, "<TH>Name</TH>" );
            fprintf ( fpHTMFile, "<TH>Param Dir</TH>" );
            fprintf ( fpHTMFile, "<TH>Param Type</TH>" );
            fprintf ( fpHTMFile, "<TH>Param Name</TH>" );
            fprintf ( fpHTMFile, "<TH>Default Value</TH>" );
            fprintf ( fpHTMFile, "<TH>Optional</TH>" );
            fprintf ( fpHTMFile, "<TH>Ret. Type</TH>" );
            fprintf ( fpHTMFile, "</TR>\n" );
            GenerateInterfaceMethodHTM ( pToken );     // BUGBUG: sort attributes
            fprintf ( fpHTMFile, "</TABLE>" );
        }
    }

    // Output eventset documentation
    ThisFilesList.Reset();
    while ( (pToken = ThisFilesList.GetNext ( TYPE_EVENT )) )
    {
        if (_stricmp("IDispatch", pToken->GetTagValue ( INTERFACE_NAME )) )
        {
            char achFilePrefix[100];
          
            strcpy (achFilePrefix, _pszPDLFileName);
            char *pDot = strchr(achFilePrefix, '.');
            if (pDot) {
                *pDot = '\0';
            }
            else {
                strcpy(achFilePrefix, "badpdl");
            }

            fprintf ( fpHTMIndexFile, "<A NAME=\"Index_Eventset_%s\">Eventset: <A HREF=\"%s.htm#Eventset_%s\">%s</A><BR>\n",
                        pToken->GetTagValue ( EVENT_NAME ),
                        achFilePrefix, pToken->GetTagValue ( EVENT_NAME ),
                        pToken->GetTagValue ( EVENT_NAME ));
      
            fprintf ( fpHTMFile, "<BR><BR>\n" );
            fprintf ( fpHTMFile, "<A NAME=\"Eventset_%s\">\n", pToken->GetTagValue ( EVENT_NAME ) );
            fprintf ( fpHTMFile, "<TABLE>\n" );
            fprintf ( fpHTMFile, "<TR><TD><H1>Event Set</H1></TD>"
                                 "<TR><TD><B>%s</B></TD><TD> GUID: </TD><TD>%s</TD></TR>"
                                 "<TR><TD>inherits from event set</TD></TR>"
                                 "<TR><TD><B><A HREF=\"AllIndex.htm#Index_Eventset_%s\">%s</A></B></TD></TR>\n"
                                 "</TABLE>\n",
                                  pToken->GetTagValue ( EVENT_NAME ),
                                  pToken->GetTagValue ( EVENT_GUID ),
                                  pToken->GetTagValue ( EVENT_SUPER ),
                                  pToken->GetTagValue ( EVENT_SUPER ) );
            fprintf ( fpHTMFile, "<HR>\n" );
            fprintf ( fpHTMFile, "<H2>Methods</H2>\n" );
            fprintf ( fpHTMFile, "<TABLE BORDER=1>\n" );
            fprintf ( fpHTMFile, "<TR>" );
            fprintf ( fpHTMFile, "<TH>Ret. Name</TH>" );
            fprintf ( fpHTMFile, "<TH>Name</TH>" );
            fprintf ( fpHTMFile, "<TH>DISPID</TH>" );
            fprintf ( fpHTMFile, "<TH>Cancelable</TH>" );
            fprintf ( fpHTMFile, "<TH>Bubbling</TH>" );
            fprintf ( fpHTMFile, "<TH>Param Dir</TH>" );
            fprintf ( fpHTMFile, "<TH>Param Type</TH>" );
            fprintf ( fpHTMFile, "<TH>Param Name</TH>" );
            fprintf ( fpHTMFile, "<TH>Default Value</TH>" );
            fprintf ( fpHTMFile, "<TH>Optional</TH>" );
            fprintf ( fpHTMFile, "<TH>Ret. Type</TH>" );
            fprintf ( fpHTMFile, "</TR>\n" );
            GenerateEventMethodHTM ( pToken );     // BUGBUG: sort attributes
            fprintf ( fpHTMFile, "</TABLE>" );
        }
    }

    fprintf ( fpHTMFile, "</BODY>\n");
    fprintf ( fpHTMFile, "</HTML>\n");

    return TRUE;
}


void CPDLParser::GenerateArg ( Token *pArgToken )
{
    if ( ! pArgToken -> IsSet ( METHODARG_RETURNVALUE ) )
    {
        fprintf ( fpHTMFile, "&nbsp[%s] %s %s",
                  pArgToken -> IsSet ( METHODARG_OUT ) ? "out" : "in",
                  pArgToken -> GetTagValue ( METHODARG_TYPE ),
                  pArgToken -> GetTagValue ( METHODARG_ARGNAME ) );

        if ( pArgToken -> IsSet ( METHODARG_DEFAULTVALUE ) )
        {
            fprintf ( fpHTMFile, "=%s",
                      pArgToken -> GetTagValue ( METHODARG_DEFAULTVALUE ) );
        }

        if ( pArgToken -> IsSet ( METHODARG_OPTIONAL ) )
        {
            fprintf ( fpHTMFile, " [optional]" );
        }
    }
}

void CPDLParser::GenerateInterfaceArg ( Token *pArgToken )
{
    if ( ! pArgToken -> IsSet ( METHODARG_RETURNVALUE ) )
    {
        fprintf ( fpHTMFile, "<TD>%s</TD><TD>%s</TD><TD>%s</TD>",
                  pArgToken -> IsSet ( METHODARG_OUT ) ? "out" : "in",
                  pArgToken -> GetTagValue ( METHODARG_TYPE ),
                  pArgToken -> GetTagValue ( METHODARG_ARGNAME ) );

        fprintf ( fpHTMFile, "<TD>%s</TD><TD>%s</TD>",
                  pArgToken -> IsSet ( METHODARG_DEFAULTVALUE ) ?
                    (pArgToken -> GetTagValue ( METHODARG_DEFAULTVALUE )) : "",
                  pArgToken -> IsSet ( METHODARG_OPTIONAL) ?
                    "Y" : "N" );
    
    }
}

void CPDLParser::GenerateMethodHTM ( Token *pIntfToken )
{
    CTokenListWalker   *pChildList;
    Token              *pChildToken;
    Token              *pArgToken;
    int                 cArgs;
    CString             szSuper;
    CTokenListWalker   *ptlw = NULL;

    pChildList = new CTokenListWalker ( pIntfToken );

    szSuper = pIntfToken->GetTagValue ( INTERFACE_SUPER );

    do
    {
        while ( pChildToken = pChildList -> GetNext() )
        {
            CString             szRetValArg;
            CString             szRetValType;
            CTokenListWalker    ArgListWalker ( pChildToken );

            cArgs = 0;

            // Loop thru all arguments.
            while ( (pArgToken = ArgListWalker.GetNext()) != NULL &&
                    pArgToken -> GetType () == TYPE_METHOD_ARG )
            {
                // Looking for a return value.
                if ( pArgToken -> IsSet ( METHODARG_RETURNVALUE ) )
                {
                    // If a return value exist then get the argument name.
                    szRetValArg = pArgToken -> GetTagValue (
                        pArgToken -> IsSet ( METHODARG_ARGNAME ) ?
                            METHODARG_ARGNAME : METHODARG_RETURNVALUE );

                    szRetValType = pArgToken -> GetTagValue ( METHODARG_TYPE );
                }
                else
                {
                    cArgs++;
                }
            }

            if ( strlen ( szRetValArg ) )
            {
                fprintf ( fpHTMFile, "<TR><TD>%s = <I>object.</I><B>%s</B>",
                          (LPCSTR)szRetValArg,
                          pChildToken -> GetTagValue ( METHOD_NAME ) );
            }
            else
            {
                fprintf ( fpHTMFile, "<TR><TD><I>object.</I><B>%s</B>",
                          pChildToken -> GetTagValue ( METHOD_NAME ) );
            }

            int cArgIndex = 0;
            ArgListWalker.Reset ( );
            while ( ( pArgToken = ArgListWalker.GetNext ( ) ) != NULL &&
                      pArgToken -> GetType () == TYPE_METHOD_ARG )
            {
                if ( ! pArgToken -> IsSet ( METHODARG_RETURNVALUE ) )
                {
                    if (cArgIndex == 0)
                    {
                        switch (cArgs)
                        {
                        case 0:
                            fprintf ( fpHTMFile, "<TD>( )</TD>\n" );
                            fprintf ( fpHTMFile, "<TD ALIGN=\"Center\">%s</TD>\n",
                                      (LPCSTR)szRetValType );
                            fprintf ( fpHTMFile, "</TR>\n" );
                           break;
                        case 1:
                            fprintf ( fpHTMFile, "<TD>(" );

                            GenerateArg( pArgToken );

                            fprintf ( fpHTMFile, "&nbsp)</TD>\n" );
                            fprintf ( fpHTMFile, "<TD ALIGN=\"Center\">%s</TD>\n",
                                      (LPCSTR)szRetValType );
                            fprintf ( fpHTMFile, "</TR>\n" );
                            break;
                        default:
                            fprintf ( fpHTMFile, "<TD>(" );

                            GenerateArg( pArgToken );

                            fprintf ( fpHTMFile, ",</TD>\n" );
                            fprintf ( fpHTMFile, "<TD ALIGN=\"Center\">%s</TD>\n",
                                      (LPCSTR)szRetValType );
                            fprintf ( fpHTMFile, "</TR>\n" );
                            break;
                        }
                    }
                    else
                    {
                        if (cArgIndex > 1)
                        {
                            fprintf ( fpHTMFile, ",</TD></TR>\n" );
                        }
                        fprintf ( fpHTMFile, "<TR><TD></TD><TD>&nbsp" );
                        GenerateArg( pArgToken );
                    }

                    cArgIndex++;
                }
            }

            if ( cArgIndex == 0 )
            {
                fprintf ( fpHTMFile, "<TD>( )</TD>\n" );
                fprintf ( fpHTMFile, "<TD ALIGN=\"Center\">%s</TD>\n",
                          (LPCSTR)szRetValType );
                fprintf ( fpHTMFile, "</TR>\n" );
            }
            else if ( cArgIndex > 1 )
            {
                fprintf ( fpHTMFile, ")</TD></TR>\n" );
            }

            fprintf ( fpHTMFile, "<TR>&nbsp</TR>" );
        }

        // Get inherited interface.
        if ( strlen ( szSuper ) )
        {
            if (ptlw == NULL)
            {
                ptlw = new CTokenListWalker ( pRuntimeList, _pszPDLFileName );
            }
            else
            {
                ptlw->Reset();
            }

TryAgain:
            pIntfToken = ptlw->GetNext ( TYPE_INTERFACE, szSuper );

            if (pIntfToken)
            {
                szSuper = pIntfToken->GetTagValue ( INTERFACE_SUPER );

                delete pChildList;
                pChildList = new CTokenListWalker ( pIntfToken );
            }
            else
            {
                if ( !ptlw->IsGenericWalker ( ) )
                {
                    delete ptlw;
                    ptlw = new CTokenListWalker ( pRuntimeList );
                    goto TryAgain;
                }
            }
        }
        else
        {
            break;
        }
    } while (pIntfToken);

    delete pChildList;
    delete ptlw;
}

void CPDLParser::GenerateInterfaceMethodHTM ( Token *pIntfToken )
{
    CTokenListWalker   *pChildList;
    Token              *pChildToken;
    Token              *pArgToken;
    int                 cArgs;
    CString             szSuper;

    pChildList = new CTokenListWalker ( pIntfToken );

    szSuper = pIntfToken->GetTagValue ( INTERFACE_SUPER );

    while ( pChildToken = pChildList -> GetNext() )
    {
        CString             szRetValArg;
        CString             szRetValType;
        CTokenListWalker    ArgListWalker ( pChildToken );

        cArgs = 0;

        // Loop thru all arguments.
        while ( (pArgToken = ArgListWalker.GetNext()) != NULL &&
                pArgToken -> GetType () == TYPE_METHOD_ARG )
        {
            // Looking for a return value.
            if ( pArgToken -> IsSet ( METHODARG_RETURNVALUE ) )
            {
                // If a return value exist then get the argument name.
                szRetValArg = pArgToken -> GetTagValue (
                    pArgToken -> IsSet ( METHODARG_ARGNAME ) ?
                        METHODARG_ARGNAME : METHODARG_RETURNVALUE );

                szRetValType = pArgToken -> GetTagValue ( METHODARG_TYPE );
            }
            else
            {
                cArgs++;
            }
        }

        if ( strlen ( szRetValArg ) )
        {
            fprintf ( fpHTMFile, "<TR><TD>%s</TD><TD><I>object.</I><B>%s</B></TD>",
                      (LPCSTR)szRetValArg,
                      pChildToken -> GetTagValue ( METHOD_NAME ) );
        }
        else
        {
            fprintf ( fpHTMFile, "<TR><TD></TD><TD><I>object.</I><B>%s</B></TD>",
                      pChildToken -> GetTagValue ( METHOD_NAME ) );
        }

        int cArgIndex = 0;
        ArgListWalker.Reset ( );
        while ( ( pArgToken = ArgListWalker.GetNext ( ) ) != NULL &&
                  pArgToken -> GetType () == TYPE_METHOD_ARG )
        {
            if ( ! pArgToken -> IsSet ( METHODARG_RETURNVALUE ) )
            {
                if (cArgIndex == 0)
                {
                    switch (cArgs)
                    {
                    case 0:
                        fprintf ( fpHTMFile, "<TD></TD><TD></TD><TD></TD><TD></TD><TD></TD> <!-- 1 -->" ); 
                        fprintf ( fpHTMFile, "<TD ALIGN=\"Center\">%s</TD>\n",
                                  (LPCSTR)szRetValType );
                        fprintf ( fpHTMFile, "</TR>\n" );
                       break;
                    default:
                        GenerateInterfaceArg( pArgToken );
                        fprintf ( fpHTMFile, "<TD ALIGN=\"Center\">%s</TD>\n",
                                  (LPCSTR)szRetValType );
                        fprintf ( fpHTMFile, "</TR>\n" );
                        break;
                    }
                }
                else
                {
                    if (cArgIndex > 1)
                    {
                        fprintf ( fpHTMFile, "</TR> <!-- Hello -->\n" );
                    }
                    fprintf ( fpHTMFile, "<TR><TD></TD><TD></TD>" );
                    GenerateInterfaceArg( pArgToken );
                }

                cArgIndex++;
            }
        }

        if ( cArgIndex == 0 )
        {
            fprintf ( fpHTMFile, "<TD></TD><TD></TD><TD></TD><TD></TD><TD></TD> <!-- 2 -->" );
            fprintf ( fpHTMFile, "<TD ALIGN=\"Center\">%s</TD>\n",
                      (LPCSTR)szRetValType );
            fprintf ( fpHTMFile, "</TR>\n" );
        }
        else if ( cArgIndex > 1 )
        {
            fprintf ( fpHTMFile, "</TR> <!-- 3 -->\n" );
        }

        fprintf ( fpHTMFile, "<TR></TR>" );
    }

    delete pChildList;
}

void CPDLParser::GenerateEventMethodHTM ( Token *pIntfToken )
{
    CTokenListWalker   *pChildList;
    Token              *pChildToken;
    Token              *pArgToken;
    int                 cArgs;
    CString             szSuper;

    pChildList = new CTokenListWalker ( pIntfToken );

    szSuper = pIntfToken->GetTagValue ( INTERFACE_SUPER );

    while ( pChildToken = pChildList -> GetNext() )
    {
        CString             szRetValArg;
        CString             szRetValType;
        CTokenListWalker    ArgListWalker ( pChildToken );

        cArgs = 0;

        // Loop thru all arguments.
        while ( (pArgToken = ArgListWalker.GetNext()) != NULL &&
                pArgToken -> GetType () == TYPE_METHOD_ARG )
        {
            // Looking for a return value.
            if ( pArgToken -> IsSet ( METHODARG_RETURNVALUE ) )
            {
                // If a return value exist then get the argument name.
                szRetValArg = pArgToken -> GetTagValue (
                    pArgToken -> IsSet ( METHODARG_ARGNAME ) ?
                        METHODARG_ARGNAME : METHODARG_RETURNVALUE );

                szRetValType = pArgToken -> GetTagValue ( METHODARG_TYPE );
            }
            else
            {
                cArgs++;
            }
        }

        if ( strlen ( szRetValArg ) )
        {
            fprintf ( fpHTMFile, "<TR><TD>%s</TD><TD><I>object.</I><B>%s</B></TD>",
                      (LPCSTR)szRetValArg,
                      pChildToken -> GetTagValue ( METHOD_NAME ) );
        }
        else
        {
            fprintf ( fpHTMFile, "<TR><TD></TD><TD><I>object.</I><B>%s</B></TD>",
                      pChildToken -> GetTagValue ( METHOD_NAME ) );
        }
        
        fprintf ( fpHTMFile, "<TD>%s</TD><TD>%s</TD><TD>%s</TD>",
                  pChildToken->IsSet(METHOD_DISPID) ? pChildToken->GetTagValue ( METHOD_DISPID ) : "",
                  pChildToken->IsSet(METHOD_CANCELABLE) ? "Y" : "N",
                  pChildToken->IsSet(METHOD_BUBBLING) ? "Y" : "N" );

        int cArgIndex = 0;
        ArgListWalker.Reset ( );
        while ( ( pArgToken = ArgListWalker.GetNext ( ) ) != NULL &&
                  pArgToken -> GetType () == TYPE_METHOD_ARG )
        {
            if ( ! pArgToken -> IsSet ( METHODARG_RETURNVALUE ) )
            {
                if (cArgIndex == 0)
                {
                    switch (cArgs)
                    {
                    case 0:
                        fprintf ( fpHTMFile, "<TD></TD><TD></TD><TD></TD><TD></TD><TD></TD> <!-- 1 -->" ); 
                        fprintf ( fpHTMFile, "<TD ALIGN=\"Center\">%s</TD>\n",
                                  (LPCSTR)szRetValType );
                        fprintf ( fpHTMFile, "</TR>\n" );
                       break;
                    default:
                        GenerateInterfaceArg( pArgToken );
                        fprintf ( fpHTMFile, "<TD ALIGN=\"Center\">%s</TD>\n",
                                  (LPCSTR)szRetValType );
                        fprintf ( fpHTMFile, "</TR>\n" );
                        break;
                    }
                }
                else
                {
                    if (cArgIndex > 1)
                    {
                        fprintf ( fpHTMFile, "</TR> <!-- Hello -->\n" );
                    }
                    fprintf ( fpHTMFile, "<TR><TD></TD><TD></TD><TD></TD><TD></TD><TD></TD>" );
                    GenerateInterfaceArg( pArgToken );
                }

                cArgIndex++;
            }
        }

        if ( cArgIndex == 0 )
        {
            fprintf ( fpHTMFile, "<TD></TD><TD></TD><TD></TD><TD></TD><TD></TD> <!-- 2 -->" );
            fprintf ( fpHTMFile, "<TD ALIGN=\"Center\">%s</TD>\n",
                      (LPCSTR)szRetValType );
            fprintf ( fpHTMFile, "</TR>\n" );
        }
        else if ( cArgIndex > 1 )
        {
            fprintf ( fpHTMFile, "</TR> <!-- 3 -->\n" );
        }

        fprintf ( fpHTMFile, "<TR></TR>" );
    }

    delete pChildList;
}


void CPDLParser::GenerateEnumHTM ( Token *pClassToken, char *pEnumPrefix )
{
    CTokenListWalker    ChildList ( pClassToken );
    Token              *pChildToken;
    CString             szStringValue;

    while ( pChildToken = ChildList.GetNext() )
    {
        if ( pChildToken -> GetType() == TYPE_EVAL )
        {

            fprintf ( fpHTMFile, "<TR>\n<TD>%s%s</TD><TD ALIGN=\"Center\">\"%s\"</TD>",
                      pEnumPrefix,
                      pChildToken -> GetTagValue ( EVAL_NAME ),
                      pChildToken ->  IsSet ( EVAL_STRING ) ?
                        pChildToken -> GetTagValue ( EVAL_STRING ) :
                        pChildToken -> GetTagValue ( EVAL_NAME ) );

            fprintf ( fpHTMFile, "</TR>\n");
        }
    }
}


void CPDLParser::GeneratePropertiesHTM ( Token *pIntfToken, BOOL fAttributes )
{
    CTokenListWalker   *pChildList;
    Token              *pChildToken;
    CString             szSuper;
    CTokenListWalker   *ptlw = NULL;

    pChildList = new CTokenListWalker ( pIntfToken );

    szSuper = pIntfToken->GetTagValue ( INTERFACE_SUPER );

    do
    {
        while ( pChildToken = pChildList->GetNext() )
        {
            // Attributes are only properties which are not abstract and the
            // ppflags is not PROPPARAM_NOTHTML.
            if ( pChildToken -> GetType () == TYPE_PROPERTY     &&
                 fAttributes == (
                      !pChildToken -> IsSet ( PROPERTY_ABSTRACT )             &&
                      pChildToken -> IsSet ( PROPERTY_NOPERSIST ) ) )
            {

                if (fAttributes)
                {
                    fprintf ( fpHTMFile, "<TR><TD>%s</TD><TD>%s</TD><TD>%s</TD>",
                              pChildToken -> GetTagValue (
                                    pChildToken -> IsSet ( PROPERTY_SZATTRIBUTE ) ?
                                        PROPERTY_SZATTRIBUTE : PROPERTY_NAME ),
                              pChildToken -> GetTagValue ( PROPERTY_NAME ),
                              pChildToken -> GetTagValue ( PROPERTY_ATYPE ) );
                }
                else
                {
                    fprintf ( fpHTMFile, "<TR><TD>%s</TD><TD>%s</TD>",
                              pChildToken -> GetTagValue ( PROPERTY_NAME ),
                              pChildToken -> GetTagValue ( PROPERTY_ATYPE ) );
                }

                fprintf ( fpHTMFile, "<TD>%s</TD>",
                          pChildToken -> IsSet ( PROPERTY_NOTPRESENTDEFAULT ) ?
                            pChildToken -> GetTagValue ( PROPERTY_NOTPRESENTDEFAULT ) :
                            "" );

                fprintf ( fpHTMFile, "<TD>%s</TD>",
                          pChildToken -> IsSet ( PROPERTY_MIN ) ?
                            pChildToken -> GetTagValue ( PROPERTY_MIN ) :
                            "" );

                fprintf ( fpHTMFile, "<TD>%s</TD>",
                          pChildToken -> IsSet ( PROPERTY_MAX ) ?
                            pChildToken -> GetTagValue ( PROPERTY_MAX ) :
                            "" );

                if ( pChildToken -> IsSet ( PROPERTY_SET )  &&
                     pChildToken -> IsSet ( PROPERTY_GET ) )
                {
                    fprintf ( fpHTMFile, "<TD>R/W</TD>" );
                }
                else if ( pChildToken -> IsSet ( PROPERTY_GET ) )
                {
                    fprintf ( fpHTMFile, "<TD>R/O</TD>" );
                }
                else
                {
                    fprintf ( fpHTMFile, "<TD>W/O</TD>" );
                }

                fprintf ( fpHTMFile,
                          pChildToken -> IsSet ( PROPERTY_SETDESIGNMODE ) ?
                            "<TD>R/W</TD>" : "<TD>R/O</TD>" );

                fprintf ( fpHTMFile, "</TR>\n" );
            }
        }

        // Get inherited interface.
        if ( strlen ( szSuper ) )
        {
            if (ptlw == NULL)
            {
                ptlw = new CTokenListWalker ( pRuntimeList, _pszPDLFileName );
            }
            else
            {
                ptlw->Reset();
            }

TryAgain:
            pIntfToken = ptlw->GetNext ( TYPE_INTERFACE, szSuper );

            if (pIntfToken)
            {
                szSuper = pIntfToken->GetTagValue ( INTERFACE_SUPER );

                delete pChildList;
                pChildList = new CTokenListWalker ( pIntfToken );
            }
            else
            {
                if ( !ptlw->IsGenericWalker ( ) )
                {
                    delete ptlw;
                    ptlw = new CTokenListWalker ( pRuntimeList );
                    goto TryAgain;
                }
            }
        }
        else
        {
            break;
        }
    } while (pIntfToken);

    delete pChildList;
    delete ptlw;
}

#define GET_TAG_VALUE(pToken, value) (pToken->IsSet(value) ? pToken->GetTagValue(value) : "")

void CPDLParser::GenerateInterfacePropertiesHTM ( Token *pIntfToken )
{
    CTokenListWalker   *pChildList;
    Token              *pChildToken;
    CString             szSuper;

    pChildList = new CTokenListWalker ( pIntfToken );

    szSuper = pIntfToken->GetTagValue ( INTERFACE_SUPER );

    while ( pChildToken = pChildList->GetNext() )
    {
        if ( pChildToken -> GetType () == TYPE_PROPERTY )
        {
            fprintf ( fpHTMFile, "<TR>" );
            fprintf ( fpHTMFile, "<TD>%s</TD>", GET_TAG_VALUE(pChildToken, PROPERTY_NAME) );
            fprintf ( fpHTMFile, "<TD>%s</TD>", GET_TAG_VALUE(pChildToken, PROPERTY_ATYPE) );
            fprintf ( fpHTMFile, "<TD>%s</TD>", GET_TAG_VALUE(pChildToken, PROPERTY_DISPID) );
            fprintf ( fpHTMFile, "<TD>%s</TD>", pChildToken->IsSet(PROPERTY_GET) ? "Y" : "N" );
            fprintf ( fpHTMFile, "<TD>%s</TD>", pChildToken->IsSet(PROPERTY_SET) ? "Y" : "N" );
            fprintf ( fpHTMFile, "<TD>%s</TD>", pChildToken->IsSet(PROPERTY_SETDESIGNMODE) ? "Y" : "N" );
            fprintf ( fpHTMFile, "<TD>%s</TD>", pChildToken->IsSet(PROPERTY_NOTPRESENTDEFAULT) ? pChildToken->GetTagValue(PROPERTY_NOTPRESENTDEFAULT) : "" );
            fprintf ( fpHTMFile, "<TD>%s</TD>", pChildToken->IsSet(PROPERTY_MIN) ? pChildToken->GetTagValue(PROPERTY_MIN) : "" );
            fprintf ( fpHTMFile, "<TD>%s</TD>", pChildToken->IsSet(PROPERTY_MAX) ? pChildToken->GetTagValue(PROPERTY_MAX) : "" );
            fprintf ( fpHTMFile, "</TR>\n" );
        }
    }
    delete pChildList;
}

#undef GET_TAG_VALUE

Token* CPDLParser::FindInterface (CString szInterfaceMatch)
{
    CTokenListWalker    WholeTree ( pRuntimeList );
    Token              *pInterfaceToken;

    WholeTree.Reset();
    while ( pInterfaceToken = WholeTree.GetNext ( TYPE_INTERFACE ) )
    {
        CString     szInterface;

        if ( !pInterfaceToken -> IsSet ( INTERFACE_ABSTRACT ) &&
            pInterfaceToken -> IsSet ( INTERFACE_GUID ) )
        {
            szInterface = pInterfaceToken -> GetTagValue ( INTERFACE_NAME );
            if (szInterface == szInterfaceMatch)
            {
                return pInterfaceToken;
            }
        }
    }

    return NULL;
}


// Only interfaces defined in this file.
Token* CPDLParser::FindInterfaceLocally (CString szInterfaceMatch)
{
    CTokenListWalker    TokenList ( pRuntimeList, _pszPDLFileName );
    Token              *pInterfaceToken;

    TokenList.Reset();
    while ( pInterfaceToken = TokenList.GetNext ( TYPE_INTERFACE ) )
    {
        CString     szInterface;

        if ( !pInterfaceToken -> IsSet ( INTERFACE_ABSTRACT ) &&
            pInterfaceToken -> IsSet ( INTERFACE_GUID ) )
        {
            szInterface = pInterfaceToken -> GetTagValue ( INTERFACE_NAME );
            if (szInterface == szInterfaceMatch)
            {
                return pInterfaceToken;
            }
        }
    }

    return NULL;
}


BOOL CPDLParser::IsPrimaryInterface(CString szInterface)
{
    CTokenListWalker    TokenList ( pRuntimeList, _pszPDLFileName );
    Token              *pClassToken;

    TokenList.Reset();
    while ( pClassToken = TokenList.GetNext ( TYPE_CLASS ) )
    {
        if (!_stricmp((LPSTR)pClassToken->GetTagValue(CLASS_INTERFACE),
                      szInterface))
            return TRUE;
    }

    return FALSE;
}


Token* CPDLParser::FindClass (CString szClassMatch)
{
    CTokenListWalker    WholeTree ( pRuntimeList );
    Token              *pClassToken;

    WholeTree.Reset();
    while ( pClassToken = WholeTree.GetNext ( TYPE_CLASS ) )
    {
        CString     szClass;

        szClass = pClassToken -> GetTagValue ( CLASS_NAME );
        if (szClass == szClassMatch)
        {
            return pClassToken;
        }
    }

    return NULL;
}

Token * CPDLParser::IsSuperInterface( CString szSuper, Token * pInterface )
{
    Token              *pInterfaceToken;
    CString             szInterface;

    if(!pInterface)
        return NULL;

    szInterface = pInterface->GetTagValue(INTERFACE_NAME);
    
    pInterfaceToken = pInterface;

    while ( pInterfaceToken )
    {
        if (szSuper == szInterface)
          return pInterfaceToken;

        szInterface = pInterfaceToken->GetTagValue(INTERFACE_SUPER);
        pInterfaceToken = FindInterface(szInterface);
    }

    return NULL;

}


Token *
CPDLParser::FindMatchingEntryWOPropDesc(Token *pClass, Token *pToFindToken, BOOL fNameMatchOnly)
{
    CTokenListWalker    ChildWalker(pClass);
    Token              *pChildToken;
    CString             szInterface;
    Token              *pInterfToken;
    Token              *pFuncToken = NULL;

    while (pChildToken = ChildWalker.GetNext())
    {
        if (pChildToken->GetType() == TYPE_IMPLEMENTS)
        {
            szInterface = pChildToken->GetTagValue(IMPLEMENTS_NAME);
            pInterfToken = FindInterface(szInterface);
            if (pInterfToken)
            {
                pFuncToken = FindMethodInInterfaceWOPropDesc(pInterfToken, pToFindToken, fNameMatchOnly);
                if (pFuncToken)
                    break;      // Found a match...
            }
        }
    }

    return pFuncToken;
}


Token*
CPDLParser::FindMethodInInterfaceWOPropDesc(Token *pInterface, Token *pToFindToken, BOOL fNameMatchOnly)
{
    CTokenListWalker    ChildList(pInterface);
    Token              *pChildToken = NULL;

    while (pChildToken = ChildList.GetNext())
    {
        if (pChildToken->GetType() == pToFindToken->GetType())
        {
            if (pChildToken->GetType() == TYPE_METHOD && !pChildToken->IsSet(METHOD_NOPROPDESC))
            {
                CString szMethodName;

                if (pToFindToken->IsSet(METHOD_NOPROPDESC) &&
                    pToFindToken->IsSet(METHOD_SZINTERFACEEXPOSE))
                {
                    szMethodName = pToFindToken->GetTagValue(METHOD_SZINTERFACEEXPOSE);
                }
                else
                {
                    szMethodName = pToFindToken->GetTagValue(METHOD_NAME);
                }

                if (!strcmp((LPCSTR)szMethodName,
                            (LPCSTR)pChildToken->GetTagValue(METHOD_NAME)))
                {
                   
                    CString     szTypesSig;
                    CString     szArgsType;
                    CString     szFindTypesSig;
                    CString     szFindArgsType;
                    BOOL        fIgnore;
                    int         cIgnore;


                    // If we only need a name match then we're done.
                    if (fNameMatchOnly)
                        break;

                    if (!BuildMethodSignature(pChildToken,
                                              szTypesSig,
                                              szArgsType,
                                              fIgnore,
                                              fIgnore,
                                              cIgnore,
                                              cIgnore))
                        return NULL;

                    if (!BuildMethodSignature(pToFindToken,
                                              szFindTypesSig,
                                              szFindArgsType,
                                              fIgnore,
                                              fIgnore,
                                              cIgnore,
                                              cIgnore))
                        return NULL;

                    // Exact signature match then use it.
                    if (!strcmp(szTypesSig, szFindTypesSig) &&
                        !strcmp(szArgsType, szFindArgsType))
                        break;
                }
            }
            else if (pChildToken->GetType() == TYPE_PROPERTY && !pChildToken->IsSet(PROPERTY_NOPROPDESC))
            {
                CString szPropertyName;

                if (pToFindToken->IsSet(PROPERTY_NOPROPDESC) &&
                    pToFindToken->IsSet(PROPERTY_SZINTERFACEEXPOSE))
                {
                    szPropertyName = pToFindToken->GetTagValue(PROPERTY_SZINTERFACEEXPOSE);
                }
                else
                {
                    szPropertyName = pToFindToken->GetTagValue(PROPERTY_NAME);
                }

                if (!strcmp((LPCSTR)szPropertyName,
                            (LPCSTR)pChildToken->GetTagValue(PROPERTY_NAME)))
                {
                    // If we only need a name match then we're done.
                    if (fNameMatchOnly)
                        break;

                    // If both are properties and the types are similar or an
                    // enum then the signature is the same we found a match.
                    if ((pChildToken->IsSet(PROPERTY_GETSETMETHODS) == pToFindToken->IsSet(PROPERTY_GETSETMETHODS)) &&
                        (pChildToken->IsSet(PROPERTY_MEMBER) == pToFindToken->IsSet(PROPERTY_MEMBER)) &&
                        (pChildToken->IsSet(PROPERTY_GET) == pToFindToken->IsSet(PROPERTY_GET)) &&
                        (pChildToken->IsSet(PROPERTY_SET) == pToFindToken->IsSet(PROPERTY_SET)))
                    {
                        if (!strcmp((LPCSTR)pChildToken->GetTagValue(PROPERTY_ATYPE),
                                    (LPCSTR)pToFindToken->GetTagValue(PROPERTY_ATYPE)))
                            break;

                        if (FindEnum(pChildToken) && FindEnum(pToFindToken))
                            break;
                    }
                }
            }
        }
    }

    return pChildToken;
}
