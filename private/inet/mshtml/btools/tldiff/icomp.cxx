#include <assert.h>
#include <iostream.h>
#include "IComp.Hxx"

extern bool fgMethodAttribute;
extern bool fgMethodParameter;

bool CompareBuffer( char* pBuff1, char* pBuff2, unsigned long nLen);
bool CompareBufferNoCase( char* pBuff1, char* pBuff2, unsigned long nLen);
void WriteLine(HANDLE file, char* pBuff, int nLen);
void TokenizeAttributes( char* pBuf, unsigned long nCnt, CAutoArray<ATTRINFO>* pList );

extern unsigned long g_ulAppRetVal;
extern bool fgParamNames;
extern bool fgParamTypes;
extern bool fgParamNameCase;
extern bool fgParamTypeCase;

CCompareInterface::CCompareInterface( char* pCurBuf, 
                                      char* pRefBuf, 
                                      HANDLE fileDiff, 
                                      char* pszIntName, 
                                      BLOCK_TYPE blockType, 
                                      char* pszMethodAttr)
{
	_pCurBuf = pCurBuf;
	_pRefBuf = pRefBuf;
	_fileDiff = fileDiff;
	_pszIntName = pszIntName;
	_blockType = blockType;
    _pszMethodAttr = pszMethodAttr;

	_pCurList = new CAutoArray<LINEINFO>;
	_pRefList = new CAutoArray<LINEINFO>;


	CreateLineIndex(_pCurList, _pCurBuf);
	CreateLineIndex(_pRefList, _pRefBuf);

	_bFirstTime = true;
}

CCompareInterface::~CCompareInterface()
{
	delete _pCurList;
	delete _pRefList;
}

void CCompareInterface::FindAdditionsAndChanges( )
{
	long lIdx;
	long lTmp;
	LINEINFO lineCur;
	LINEINFO lineRef;

    bool bRetVal = false;
	
	//check for additions and alterations
	for ( lIdx = 0; lIdx< (int)_pCurList->Size(); lIdx++)
	{
		_pCurList->GetAt( lIdx, &lineCur );

		//get the real name of the method or property
		char* pszMethodName = new char[lineCur.ulParamStart-lineCur.ulMethodNameStart+1];

		for (lTmp=lineCur.ulMethodNameStart; lTmp<(long)lineCur.ulParamStart; lTmp++)
		{
			pszMethodName[lTmp-lineCur.ulMethodNameStart] = _pCurBuf[lTmp];
		}
		pszMethodName[lTmp-lineCur.ulMethodNameStart] = 0;

		lstrcpy( _szLogBuff, _pszIntName);
		lstrcat( _szLogBuff, "::" );
		lstrcat( _szLogBuff, pszMethodName );

		for ( lTmp=0; lTmp<_pRefList->Size(); lTmp++ )
		{
			_pRefList->GetAt( lTmp, &lineRef );
			
			//compare the names of two methods to find if they are comparable
			//in respect to other aspects of their declarations.
			if ((!lineRef.fUsed)&&
                ( CompareBuffer( pszMethodName, 
                                    &_pRefBuf[lineRef.ulMethodNameStart], 
                                    max( lineCur.ulParamStart-lineCur.ulMethodNameStart, lineRef.ulParamStart-lineRef.ulMethodNameStart) )))
			{
                // if the names are the same, now compare the return values. If they are not the same, then the method is
                // modified from its original version.
                if ( !CompareBuffer( &_pCurBuf[lineCur.ulNameStart], 
                                     &_pRefBuf[lineRef.ulNameStart], 
                                     max( lineCur.ulMethodNameStart-lineCur.ulNameStart, lineRef.ulMethodNameStart-lineRef.ulNameStart) ))
                {
                    bRetVal = true;
                }

				//compare attribute block 
				if (( fgMethodAttribute ) && 
                    ((lineCur.ulAttrEnd-lineCur.ulAttrStart != lineRef.ulAttrEnd-lineRef.ulAttrStart) || 
					 ( !CompareBuffer(	&_pCurBuf[lineCur.ulAttrStart], &_pRefBuf[lineRef.ulAttrStart], 							
					    				max(lineCur.ulAttrEnd-lineCur.ulAttrStart, lineRef.ulAttrEnd-lineRef.ulAttrStart))))  )
				{
                    // since we know the attributes have changed, analyze the type of change
                    CompareMethodAttributes( &lineRef, &lineCur );
				}

				//compare parameter block 
				if (( fgMethodParameter ) && 
                    ((lineCur.ulParamEnd-lineCur.ulParamStart != lineRef.ulParamEnd-lineRef.ulParamStart) ||
                     ( !CompareBuffer(	&_pCurBuf[lineCur.ulParamStart], &_pRefBuf[lineRef.ulParamStart], 
										max(lineCur.ulParamEnd-lineCur.ulParamStart, lineRef.ulParamEnd-lineRef.ulParamStart)))) )
				{
                    CompareMethodParameters( &lineRef, &lineCur );
				}

				//we have found the method that matches, move on to the next
				//method name on the current block.
				lineRef.fUsed = true;
				_pRefList->Set( lTmp, lineRef );
				break;
			}
		}

		//write the results that were found from this comparison, 
		//if this was a different line
		if ( bRetVal || (lTmp == _pRefList->Size()) )
		{
			char* pszBuff = new char[128];

            EnsureTitle( TRUE );

            if ( bRetVal )
            {
                lstrcpy( pszBuff, _szLogBuff );
                lstrcat( pszBuff, " - Return value or call type has changed " );
                WriteLine( _fileDiff, pszBuff, -1);
                bRetVal = false;
                g_ulAppRetVal |= CHANGE_RETVALCHANGE;
            }
	
			if (lTmp == _pRefList->Size())	//this is a new nethod
			{
				lstrcpy( pszBuff, _szLogBuff );
				lstrcat( pszBuff, " - Is a new method " );
				WriteLine(_fileDiff, pszBuff,-1);

				if ( _blockType== BLK_DISPINT )
					g_ulAppRetVal |= CHANGE_METHODONDISPINT;
				else
					g_ulAppRetVal |= CHANGE_METHODONINT;
			}
			delete [] pszBuff;
		}
		
		delete [] pszMethodName;
	}
}

//----------------------------------------------------------------------------
//  bMode == TRUE  --> Addition / Change
//----------------------------------------------------------------------------
void
CCompareInterface::EnsureTitle( BOOL bAddition )
{
    char szBuff[256];

    if ( _bFirstTime )
    {
        //write the header.
        if ( _blockType== BLK_DISPINT )
            lstrcpy( szBuff, "\nDispinterface " );
        else
            lstrcpy( szBuff, "\nInterface " );
        
        lstrcat( szBuff, _pszIntName );
        lstrcat( szBuff, "\n------------------------------------\n");
        if ( bAddition )
        {
            lstrcat( szBuff, "Additions / Changes:" );
        }
        
        WriteLine( _fileDiff, szBuff, -1);
        
        _bFirstTime = false;
    }
}
//
//	Walk through the unmarked elements of the reference block index. These are the
//	entries that do not exist in the current block.
//
void CCompareInterface::FindRemovals( )
{
	long lIdx;
	long lTmp;
	LINEINFO	lineRef;
	char* szBuff = new char[128];
	bool bFirstRemoval = true;

	for ( lIdx=0; lIdx< (int)_pRefList->Size(); lIdx++ )
	{
		//get the record
		_pRefList->GetAt( lIdx, &lineRef);

		//is the record marked ?
		if (!lineRef.fUsed)
		{
			//get the real name of the interface
			char* pszMethodName = new char[lineRef.ulParamStart-lineRef.ulMethodNameStart+1];
			int nIdx;
			
			for (lTmp=lineRef.ulMethodNameStart, nIdx=0; lTmp<(long)lineRef.ulParamStart; lTmp++, nIdx++)
			{
				pszMethodName[nIdx] = _pRefBuf[lTmp];
			}

            pszMethodName[nIdx] = 0;		//terminate the string

            // if this is the first removal, then add the word Removals
            EnsureTitle( FALSE );

			//write the header.
			if ( _blockType== BLK_DISPINT )
				g_ulAppRetVal |= CHANGE_REMOVEFROMDISPINT;
			else
				g_ulAppRetVal |= CHANGE_REMOVEFROMINT;

			if ( bFirstRemoval)
			{
				WriteLine( _fileDiff, "Removals : ", -1);
				bFirstRemoval = false;
			}

			lstrcpy( szBuff, _pszIntName);
			lstrcat( szBuff, "::");
			lstrcat( szBuff, pszMethodName);
			lstrcat( szBuff, " has been removed.");
			WriteLine( _fileDiff, szBuff, -1);

			delete [] pszMethodName;
		}
	}

	delete [] szBuff;
}

void CCompareInterface::CreateLineIndex( CAutoArray<LINEINFO>* pList, char* pBuf )
{
	LINEINFO lineinfo = {0};

	unsigned long ulIdx=0;
	unsigned long ulLastSpace = 0;
	char chSearch = '[';				//initially look for the opening attribute char.
	unsigned int uBrCnt = 0;
	unsigned int uParCnt = 0;

	//go until the end of the buffer, it is null terminated.
	while ( pBuf[ulIdx] != 0)
	{
		if ( pBuf[ulIdx] == chSearch )
		{
			//depending on what we were looking for, 
			//we can decide what to look for next.
			switch (chSearch)
			{
				case '[':
					uBrCnt++;
					if ( uBrCnt == 1 )
					{
						lineinfo.ulAttrStart = ulIdx;
						chSearch = ']';
					}
					break;

				case ']':
					uBrCnt --;
					if ( uBrCnt == 0 )
					{
						lineinfo.ulAttrEnd = ulIdx;
						lineinfo.ulNameStart = ulIdx+2;
						chSearch = '(';
					}
					break;

				case '(':
					uParCnt++;
					if (uParCnt==1)
					{
						lineinfo.ulNameEnd = ulIdx-1;
						lineinfo.ulParamStart = ulIdx;
						lineinfo.ulMethodNameStart = ulLastSpace+1;
						chSearch = ')';
					}
					break;

				case ')':
					uParCnt--;
					if ( uParCnt == 0 )
					{
						lineinfo.ulParamEnd = ulIdx;
						chSearch = '[';
						
						//completed the cycle, add this record to the list
						pList->Append(lineinfo);
					}
					break;
			}
		}
		else
		{
			switch ( pBuf[ulIdx] )
			{
				case '(':
					uParCnt++;
					break;

				case ')':
					uParCnt--;
					break;

				case '[':
					uBrCnt++;
					break;

				case ']':
					uBrCnt--;
					break;
	
				case ' ':
					ulLastSpace = ulIdx;
					break;
			}
		}

		ulIdx++;
	}
}

/*----------------------------------------------------------------------------

  ----------------------------------------------------------------------------*/
void 
CCompareInterface::CompareMethodAttributes( LINEINFO* pRef, 
                                            LINEINFO* pCur )
{
    long                    l, 
                            k;
    long                    curBase = (pCur->ulAttrStart)+1;
    long                    refBase = (pRef->ulAttrStart)+1;
    ATTRINFO                attrRef; 
    ATTRINFO                attrCur;

    CAutoArray<ATTRINFO>*   pCurList = new CAutoArray<ATTRINFO>;
	CAutoArray<ATTRINFO>*   pRefList = new CAutoArray<ATTRINFO>;

    TokenizeAttributes( &_pRefBuf[refBase], pRef->ulAttrEnd-refBase, pRefList );
    TokenizeAttributes( &_pCurBuf[curBase], pCur->ulAttrEnd-curBase, pCurList );

    //let's find the ones that are new
    for ( l=0; l < pCurList->Size(); l++ )      
    {
        pCurList->GetAt( l, &attrCur);

        for ( k=0; k < pRefList->Size(); k++ )
        {
            pRefList->GetAt( k, &attrRef );

            if ( (!attrRef.fUsed ) && 
                  CompareBuffer( &_pCurBuf[curBase + attrCur.ulAttrStart], 
                                &_pRefBuf[refBase + attrRef.ulAttrStart],
                                max( attrCur.ulAttrLength, attrRef.ulAttrLength)) )
            {
                // found the same attribute in the reference attributes, it is not a new
                // attribute
                attrRef.fUsed = true;
                pRefList->Set( k, attrRef );

                attrCur.fUsed = true;
                pCurList->Set( l, attrCur );
                break;
            }
        }
        
        if ( k == pRefList->Size() )
        {
            // this is a new attribute. 
            // if we find this attribute name in the list, then we are breaking the compat
            if ( IsAttributeBreaker( _pszMethodAttr,
                                        _pCurBuf+curBase+attrCur.ulAttrStart, 
                                        attrCur.ulAttrLength ) )
            {
                EnsureTitle(TRUE);
                WriteAttrChangeString( _pCurBuf, 
                                        curBase+attrCur.ulAttrStart, 
                                        attrCur.ulAttrLength, 
                                        "' attribute was added"); 
            }
        }
    }

    // Whatever is left in the reference array as not used are removals.
    for ( l=0; l < pRefList->Size(); l++ )      
    {
        pRefList->GetAt( l, &attrRef);

        if ( !attrRef.fUsed )
        {
            // if we find this attribute name in the list, then we are breaking the compat
            if ( IsAttributeBreaker( _pszMethodAttr, 
                                        _pRefBuf+refBase+attrRef.ulAttrStart, 
                                        attrRef.ulAttrLength ) )
            {
                // breaker attribute
                EnsureTitle(TRUE);
                WriteAttrChangeString( _pRefBuf, 
                                        refBase+attrRef.ulAttrStart, 
                                        attrRef.ulAttrLength, 
                                        "' attribute was removed");
            }
        }
    }

    delete pCurList;
    delete pRefList;
}

//----------------------------------------------------------------------------
// The attribute list contains the buffer that is read from the INI file.
// Each attribute name is a string that is terminated by a NULL character. At 
// the very end, after the last attribute, there is an additional NULL.
//----------------------------------------------------------------------------
BOOL 
CCompareInterface::IsAttributeBreaker( char * pszAttrList, char * pszAttr, unsigned long ulAttrLen )
{
    unsigned long   ulStrLen;
    unsigned long   ulIdx = 0;    // index to the big buffer.

    // until we reach the very end.
    // if we can not get into the loop below, it means that there are no 
    // attributes that are considered breaking
    while (pszAttrList[ulIdx] != NULL)
    {
        ulStrLen = lstrlen(&pszAttrList[ulIdx]);

        // if the lengths and the contents are the same, then this attribute
        // is a breaker attribute
        if ((ulStrLen == ulAttrLen) && 
             (CompareBuffer(&pszAttrList[ulIdx], pszAttr, ulStrLen)))
        {
            return TRUE;
        }

        // increment the index, to point to the next string in the buffer
        ulIdx += ulStrLen + 1;
    }

    // if we reach here, it means that we could not find the attribute in the list
    // this is NOT a breaker attribute
    return FALSE;
}

void 
CCompareInterface::WriteAttrChangeString(char* pBuf,
                                         unsigned long ulAttrStart, 
                                         unsigned long ulAttrLength, 
                                         char* szChangeType)
{
    unsigned long   k;
    char            szBuff[256];
    char *          pszAttrName = new char[ulAttrLength+1];

    //copy the attribute name into the buffer
    for (k = 0; k < ulAttrLength; k++)
        pszAttrName[k] = *(pBuf + ulAttrStart + k);

    pszAttrName[k] = 0; //terminate

    lstrcpy(szBuff, _szLogBuff);
    lstrcat(szBuff, " - '");
    lstrcat(szBuff, pszAttrName);
    lstrcat(szBuff, szChangeType);

    WriteLine(_fileDiff, szBuff, -1);
    g_ulAppRetVal |= CHANGE_ATTRCHANGE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void 
CCompareInterface::CompareMethodParameters( LINEINFO* pRef, LINEINFO* pCur)
{
    long                    l;
    long                    k;
    long                    curBase = (pCur->ulParamStart)+1;
    long                    refBase = (pRef->ulParamStart)+1;

    PARAMINFO               paramRef;
    PARAMINFO               paramCur;

    CAutoArray<PARAMINFO>*  pCurList = NULL; 
	CAutoArray<PARAMINFO>*  pRefList = NULL; 

    bool                    bNameChange, 
                            bTypeChange,
                            bReplaced;
    char                    szBuff[512] = {0};
    char                    szType[64] = {0};
    char                    szName[64] = {0};


    pCurList = new CAutoArray<PARAMINFO>;
    pRefList = new CAutoArray<PARAMINFO>;

    assert( pCurList );
    assert( pRefList );

    // start by tokenizing the parameters
    TokenizeParameters( &_pRefBuf[refBase], pRef->ulParamEnd-refBase, pRefList);
    TokenizeParameters( &_pCurBuf[curBase], pCur->ulParamEnd-curBase, pCurList);

    // parameters have to match one to one.
    for ( l=0; l<pRefList->Size(); l++ )
    {
        bTypeChange = bNameChange = bReplaced = false;

        pCurList->GetAt( l, &paramCur);
        pRefList->GetAt( l, &paramRef);

        // is this parameter touched before, because of a replacement catch?
        // if that is the case we should only check if the reference parameter was replaced
        if ( paramCur.fUsed )
        {
            bTypeChange = bNameChange = true;
            goto ReplaceCheck;
        }

        // compare the types
        if ( fgParamTypes )
        {
            // if the lengths are different bail out immediately, without text comparison.
            if ( paramRef.ulTypeLength == paramCur.ulTypeLength ) 
            {
                //compare the contents, check if we want case sensitive or not.            
                if ( fgParamTypeCase )
                    bTypeChange = !CompareBuffer(   _pRefBuf+refBase+paramRef.ulTypeStart, 
                                                    _pCurBuf+curBase+paramCur.ulTypeStart,
                                                    max( paramRef.ulTypeLength, paramCur.ulTypeLength) );
                else
                    bTypeChange = !CompareBufferNoCase( _pRefBuf+refBase+paramRef.ulTypeStart, 
                                                        _pCurBuf+curBase+paramCur.ulTypeStart,
                                                        max( paramRef.ulTypeLength, paramCur.ulTypeLength) );
            }
            else 
                bTypeChange = true;
        }


        if ( fgParamNames )
        {
            // if the lengths are different bail out immediately, without text comparison.
            if ( paramRef.ulNameLength == paramCur.ulNameLength ) 
            {
                //compare the contents, check if we want case sensitive or not.            
                if ( fgParamNameCase )
                    bNameChange = !CompareBuffer(   _pRefBuf+refBase+paramRef.ulNameStart, 
                                                    _pCurBuf+curBase+paramCur.ulNameStart,
                                                    max( paramRef.ulNameLength, paramCur.ulNameLength) );
                else
                    bNameChange = !CompareBuffer(   _pRefBuf+refBase+paramRef.ulNameStart, 
                                                    _pCurBuf+curBase+paramCur.ulNameStart,
                                                    max( paramRef.ulNameLength, paramCur.ulNameLength) );
            }
            else 
                bNameChange = true;
        }

ReplaceCheck:
        // if there was a change in the parameter, find out if this parameter is moved to another location
        // in the parameter list. We look for an exact match in this case, since this is only additional 
        // information
        if ( bNameChange || bTypeChange )
        {
            PARAMINFO   paramTmp;

            for ( k=0; k< pCurList->Size(); k++ )
            {
                pCurList->GetAt( k, &paramTmp );

                if ( ( !paramTmp.fUsed ) && 
                     ( paramTmp.ulParamLength == paramCur.ulParamLength ) &&
                     ( CompareBuffer( _pRefBuf+refBase+paramRef.ulTypeStart, 
                                        _pCurBuf+curBase+paramTmp.ulTypeStart, 
                                            paramTmp.ulParamLength) ) )
                {
                    // we have found the parameter at another location.
                    bReplaced = true;

                    // we will only report the replacement, to simplify
                    bTypeChange = false;
                    bNameChange = false;

                    // mark the parameter in the current list as touched, so that 
                    // whatever parameter we check in the reference list does not get
                    // processed against this. ( perf. )
                    paramTmp.fUsed = true;
                    pCurList->Set( k, paramTmp );
                }
            }
        }
    
        // if we found the parameter  at the same location 
        if ( bReplaced || bNameChange || bTypeChange )
        {
            EnsureTitle(TRUE);

            g_ulAppRetVal |= CHANGE_PARAMCHANGE;

            // we copy the type and the name. The lengths are +1 since the function requires
            // us to calculate the NULL character too.

            lstrcpyn( szType, _pRefBuf+refBase+paramRef.ulTypeStart, paramRef.ulTypeLength+1 );

            if ( paramRef.ulNameStart )
                lstrcpyn( szName, _pRefBuf+refBase+paramRef.ulNameStart, paramRef.ulNameLength+1 );

            // fill the string with ' - Parameter xx', so that we can add the change type
            lstrcpy( szBuff, _szLogBuff);
            lstrcat( szBuff, " - Parameter " );
            lstrcat( szBuff, szType );
            lstrcat( szBuff, " ");
            lstrcat( szBuff, szName );

            // if replaced, then name and type change flags are false.
            if ( bReplaced  )
            {
                // output replacement information
                lstrcat( szBuff, " has been replaced" );
            }
            else 
            {
                // was this parameter removed
                if ( bNameChange && bTypeChange )
                {
                    // output information that shows the name change
                    lstrcat( szBuff, " has been removed");
                }
                else
                {
                    if ( bNameChange )
                    {
                        // output information that shows the name change
                        lstrcat( szBuff, " name has been modified");

                        // for name only changes, mark the parameter as used.
                        paramCur.fUsed = true;
                        pCurList->Set( l, paramCur );
                    }
            
                    if ( bTypeChange )
                    {
                        // output information that shows the type change.
                        lstrcat( szBuff, " type has been modified");
                    }
                }
            }

            WriteLine(_fileDiff, szBuff, -1);
        }
        else
        {
            // mark the parameter, everything is OK, move on.
            paramCur.fUsed = true;
            pCurList->Set( l, paramCur );
        }
    }

    // find the parameters that were added.
    for ( l=0; l<pCurList->Size(); l++ )
    {
        pCurList->GetAt( l, &paramCur);

        // if this parameter was not used, then it means that it was added.
        if ( !paramCur.fUsed )
        {
            EnsureTitle(TRUE);

            g_ulAppRetVal |= CHANGE_PARAMCHANGE;

            // copy the parameter name as a whole
            lstrcpyn( szName, _pCurBuf+curBase+paramCur.ulTypeStart, paramCur.ulParamLength+1 );

            // fill the string with ' - Parameter xx', so that we can add the change type
            lstrcpy( szBuff, _szLogBuff);
            lstrcat( szBuff, " - Parameter " );
            lstrcat( szBuff, szName );
            lstrcat( szBuff, " was added");
            WriteLine( _fileDiff, szBuff, -1);

        }
    }

    delete pCurList;
    delete pRefList;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void 
CCompareInterface::TokenizeParameters(  char* pBuf, 
                                        unsigned long nCnt, 
                                        CAutoArray<PARAMINFO>* pList )
{
    unsigned long   i,j;
    PARAMINFO       paramInfo;
    bool            bInBracket = false;

    paramInfo.ulTypeStart = 0;

    for( i=0; i<=nCnt ; i++ )
    {
        // since we are coming from left, and going right, we will first see the 
        // opening and then the closing bracket
        if ( pBuf[i] == '[' ) 
            bInBracket = true;
        if ( pBuf[i] == ']' )
            bInBracket = false;

        // if we reached a comma that was not inside a bracket, or reached the end
        // and the end is an opening parenthesis
        if ( ((pBuf[i] == ',') && !bInBracket ) || ( i == nCnt ) )
        {
            paramInfo.ulParamLength = i - paramInfo.ulTypeStart;
            paramInfo.fUsed = false;

            // digest the type and name here ! ! !
            for ( j = paramInfo.ulTypeStart+paramInfo.ulParamLength-1; j > 0 ; j-- )
            {
                // go from the end of the parameter, towards the beginning, 
                // searching for a space character, or the beginning of the parameter block
                if ( *(pBuf + j) == ' ') 
                {
                    paramInfo.ulNameStart = j + 1;
                    paramInfo.ulTypeLength = j - paramInfo.ulTypeStart;
                    paramInfo.ulNameLength = paramInfo.ulTypeStart + paramInfo.ulParamLength - paramInfo.ulNameStart;
                    break;
                }
            }

            // we could not find a parameter when we parsed through, it means a void..
            // double check for void 
            if (( j==0 ) && ( *pBuf == 'v' ) && (*(pBuf+1) == 'o'))
            {
                paramInfo.ulTypeStart = 0;
                paramInfo.ulTypeLength = 4;
                paramInfo.ulParamLength = 4;
                paramInfo.ulNameStart = 0;
                paramInfo.ulNameLength = 0;

                pList->Append( paramInfo );
            }
            else
            {
                // we should never ever reach zero.
                if ( j==0 )
                    assert( false );

                pList->Append( paramInfo );
    
                // skip over the comma
                i++;

                // the name starts next to the space
                paramInfo.ulTypeStart = i+1;
            }
        }
    }
}