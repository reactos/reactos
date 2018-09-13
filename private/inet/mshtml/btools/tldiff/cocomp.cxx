#include <assert.h>
#include <iostream.h>
#include "CoComp.Hxx"

bool CompareBuffer( char* pBuff1, char* pBuff2, unsigned long nLen);
void WriteLine(HANDLE file, char* pBuff, int nLen);
extern unsigned long g_ulAppRetVal;

CCompareCoClass::CCompareCoClass(char* pCurBuf, char* pRefBuf, HANDLE fileDiff, char* pszClassName )
{
	_pCurBuf = pCurBuf;
	_pRefBuf = pRefBuf;
	_fileDiff = fileDiff;
	_pszClassName = pszClassName;

	_pCurList = new CAutoArray<LINEINFO>;
	_pRefList = new CAutoArray<LINEINFO>;

	CreateLineIndex(_pCurList, _pCurBuf);
	CreateLineIndex(_pRefList, _pRefBuf);

	_bFirstTime = true;
}

CCompareCoClass::~CCompareCoClass()
{
	delete _pCurList;
	delete _pRefList;
}

void CCompareCoClass::FindAdditionsAndChanges()
{
	long lIdx;
	long lTmp;
	LINEINFO lineCur;
	LINEINFO lineRef;

	char* szBuff = new char[128];
	
	//check for additions and alterations
	for ( lIdx = 0; lIdx< (int)_pCurList->Size(); lIdx++ )
	{
		_pCurList->GetAt( lIdx, &lineCur );

		for ( lTmp=0; lTmp<_pRefList->Size(); lTmp++ )
		{
			_pRefList->GetAt( lTmp, &lineRef );
			
			//compare the names of two methods to find if they are comparable
			//in respect to other aspects of their declarations.
			if (( !lineRef.fUsed ) && 
				( CompareBuffer(&_pCurBuf[lineCur.ulNameStart], 
								&_pRefBuf[lineRef.ulNameStart], 
								max(lineCur.ulNameEnd-lineCur.ulNameStart, 
								lineRef.ulNameEnd-lineRef.ulNameStart)+ 1)))
			{
				//we have found a match. Mark it so that we don't compare anymore
				lineRef.fUsed = true;
				_pRefList->Set( lTmp, lineRef );
				break;
			}

			lineRef.fUsed = false;
		}

		//did we find a match.
		if ( lTmp == _pRefList->Size() )
		{
			char* pszName = new char[lineCur.ulNameEnd-lineCur.ulNameStart+1];
			for (lTmp=lineCur.ulNameStart; lTmp<(long)lineCur.ulNameEnd; lTmp++)
			{
				pszName[lTmp-lineCur.ulNameStart] = _pCurBuf[lTmp];
			}
			pszName[lTmp-lineCur.ulNameStart] = 0;

			//if it is the first time, put the banner in.
			if ( _bFirstTime )
			{
				lstrcpy( szBuff, "\ncoclass ");
				lstrcat( szBuff, _pszClassName);
				lstrcat( szBuff, "\n-----------------------------\n" );
				lstrcat( szBuff, "Additions and changes:" );
				WriteLine( _fileDiff, szBuff, -1 );
				_bFirstTime = false;
			}

			//this either has changed, or it's a new line.
			lstrcpy( szBuff, _pszClassName);
			lstrcat( szBuff, " : " );
			lstrcat( szBuff, pszName );
			lstrcat( szBuff, " added");

			WriteLine( _fileDiff, szBuff, -1);

			g_ulAppRetVal |= CHANGE_ADDTOCOCLASS;

			delete pszName;
		}
	}

	delete [] szBuff;
}

void CCompareCoClass::FindRemovals()
{
	long lIdx;
	long lTmp;
	LINEINFO	lineRef;
	char* szBuff = new char[128];
	bool bFirstRemoval= true;

	for ( lIdx=0; lIdx< (int)_pRefList->Size(); lIdx++ )
	{
		//get the record
		_pRefList->GetAt( lIdx, &lineRef);

		//is the record marked ?
		if (!lineRef.fUsed)
		{
			//get the real name of the interface
			char* pszName = new char[lineRef.ulNameEnd-lineRef.ulNameStart+1];
			
			for (lTmp=lineRef.ulNameStart; lTmp<(long)lineRef.ulNameEnd; lTmp++)
			{
				pszName[lTmp-lineRef.ulNameStart] = _pRefBuf[lTmp];
			}
			pszName[lTmp-lineRef.ulNameStart] = 0;		//terminate the string

			//if it is the first time, put the banner in.
			if ( _bFirstTime )
			{
				lstrcpy( szBuff, "coclass ");
				lstrcat( szBuff, _pszClassName);
				lstrcat( szBuff, "\n-----------------------------" );
				WriteLine( _fileDiff, szBuff, -1 );
				_bFirstTime = false;
			}

			if ( bFirstRemoval )
			{
				WriteLine( _fileDiff, "Removals:", -1);
				bFirstRemoval = false;
			}

			lstrcpy( szBuff, _pszClassName );
			lstrcat( szBuff, " : ");
			lstrcat( szBuff, pszName );
			lstrcat( szBuff, " has been removed." );
			WriteLine( _fileDiff, szBuff, -1);

			g_ulAppRetVal |= CHANGE_REMOVEFROMCOCLASS;

			delete [] pszName;

			WriteLine( _fileDiff, " ", -1 );
		}
	}

	delete [] szBuff;
}

//
//
//
void CCompareCoClass::CreateLineIndex( CAutoArray<LINEINFO>* pList, char* pBuf)
{
	LINEINFO lineinfo = {0};

	unsigned long ulIdx = 0;
	unsigned long ulLastSpace = 0;	
	char chSearch = '{';			//look for the start of the declaration
	
	while ( pBuf[ulIdx] != 0 )
	{
		if ( pBuf[ulIdx] == chSearch )
		{
			switch (chSearch)
			{
				case '{':
					ulIdx+=2;		//skip over the CR/LF

					//these values are not used and there is no need to use 
					//CPU time to set them.
					//					lineinfo.ulAttrStart = 0;
					//					lineinfo.ulAttrEnd = 0;
					//					lineinfo.ulParamStart=0;
					//					lineinfo.ulParamEnd=0;

					lineinfo.ulNameStart = ulIdx+1;
					chSearch = ';';
					break;

				case ';':
					//finish logging this line
					lineinfo.ulNameEnd = ulIdx;
					pList->Append(lineinfo);

					//prepare for the next line
					ulIdx+=2;
					lineinfo.ulNameStart = ulIdx+1;
					break;
			}
		}
		ulIdx++;
	}
}