/**************************************************/
/*					                              */
/*					                              */
/*	Setting code range		                      */
/*					                              */
/*                                                */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

#include	"stdafx.h"
#include	"eudcedit.h"
#include	"registry.h"
#ifdef BUILD_ON_WINNT
#include    "extfunc.h"
#endif // BUILD_ON_WINNT
#include    "util.h"

#define		S_UNICODE	0xE000
#define		E_UNICODE	0xE0ff

BOOL	SetCountryInfo( UINT LocalCP);
int		SetLeadByteRange(TCHAR * CodeRange,int nCode);
void	SetTrailByteRange();
#ifdef BUILD_ON_WINNT
void    CorrectTrailByteRange(int nIndex);
#endif // BUILD_ON_WINNT
/****************************************/
/*					*/
/*	Set Country Infomation		*/
/*					*/
/****************************************/
BOOL 
SetCountryInfo( 
UINT 	LocalCP)
{
	TCHAR	CodePage[10], szUnicode[] = TEXT("Unicode");
	TCHAR	Coderange[50];
	int nCode = 0;

	SetTrailByteRange(LocalCP);
	
	if (!CountryInfo.bOnlyUnicode){

/* Read EUDC Coderange from Registry */
#ifndef NEWREG
		/* Old Version */
		TCHAR 	CodeTmp[10];
		wsprintf( CodeTmp, TEXT("%d"), LocalCP);
		if( lstrlen( CodeTmp) == 3){
			lstrcpy(CodePage, TEXT("CP00"));
		}else{
			lstrcpy(CodePage, TEXT("CP0"));
		}
		lstrcat(CodePage, CodeTmp);
#else
		/* New Version */
		wsprintf( CodePage, TEXT("%d"), LocalCP);
#endif
		if( !InqCodeRange(CodePage, (BYTE *)Coderange, 50))
		return FALSE;
		
		if ((nCode = SetLeadByteRange ( Coderange, 0)) == -1)
			return FALSE;
	} //!CountryInfo.bOnlyUnicode	

#ifdef UNICODE
	// unicode range will always be the last one.
	lstrcpy(CodePage, szUnicode);
	if( !InqCodeRange(CodePage, (BYTE *)Coderange, 50))
		return FALSE;

	if (SetLeadByteRange (Coderange, nCode) == -1)
		return FALSE;
#else
	//
	//	Ansi version, we have to set end Unicode 
	//	code point to the last ansi range.
	//
	WCHAR RangeTmp[2];
	CHAR AnsiRange[2];
	CountryInfo.nRange = nCode+1;
	CountryInfo.nLeadByte = nCode+1;
	CountryInfo.sRange[nCode] = S_UNICODE;
	
	AnsiRange[0] = HIBYTE(CountryInfo.eRange[nCode-1]);
	AnsiRange[1] = LOBYTE(CountryInfo.eRange[nCode-1]);
	MultiByteToWideChar(CP_ACP, 0, AnsiRange,2,RangeTmp, 1);
	CountryInfo.eRange[nCode] = RangeTmp[0];
	CountryInfo.sLeadByte[nCode] = HIBYTE(CountryInfo.sRange[nCode]);
	CountryInfo.eLeadByte[nCode] = HIBYTE(CountryInfo.eRange[nCode]);
#endif

	
	return TRUE;

}

void 
SetTrailByteRange(
UINT LocalCP)
{
	WORD	UCode[MAX_CODE];
	BYTE	SCode[MAX_CODE], sTral, cTral;
	int	nTral = 0;
	
	if (!CountryInfo.bUnicodeMode){
		// calculate trailbyte range.
		UCode[0] = S_UNICODE;
		UCode[1] = '\0';
		WideCharToMultiByte( LocalCP, 0L, (const unsigned short *)UCode,
			-1, (char *)SCode, sizeof(SCode), NULL, NULL);
		sTral = cTral = SCode[1];
		CountryInfo.sTralByte[nTral] = sTral;

		for( WORD Cnt = S_UNICODE + 1; Cnt <= E_UNICODE; Cnt++){
			UCode[0] = Cnt;
			UCode[1] = '\0';

			WideCharToMultiByte( LocalCP, 0L, (const unsigned short *)UCode,
		 		-1, (char *)SCode, sizeof(SCode), NULL, NULL);

			if( cTral + 1 != SCode[1]){
				CountryInfo.eTralByte[nTral] = cTral;
				nTral++;
				if( sTral != SCode[1]){
					CountryInfo.sTralByte[nTral] = SCode[1];
				}
			}
			cTral = SCode[1];
			if( sTral == cTral)
				break;
		}
		CountryInfo.nTralByte = nTral;

		/* For Extend Wansung (test) */
		if( CountryInfo.LangID == EUDC_KRW){
			CountryInfo.nTralByte = 3;
			CountryInfo.sTralByte[0] = 0x41;
			CountryInfo.eTralByte[0] = 0x5a;
			CountryInfo.sTralByte[1] = 0x61;
			CountryInfo.eTralByte[1] = 0x7a;
			CountryInfo.sTralByte[2] = 0x81;
			CountryInfo.eTralByte[2] = 0xfe;
		}

		/* For CHS  we have to remember the original trail byte range and calculate
		 	 trail byte range dynamically 
		*/
		if( CountryInfo.LangID == EUDC_CHS){
			CountryInfo.nOrigTralByte = 2;
			CountryInfo.sOrigTralByte[0] = 0x40;
			CountryInfo.eOrigTralByte[0] = 0x7e;
			CountryInfo.sOrigTralByte[1] = 0x80;
			CountryInfo.eOrigTralByte[1] = 0xfe;

		//To start with, calculate trailbyte range for the default EUDC selection range.
			CorrectTrailByteRange(0); 

		}else{
			CountryInfo.nOrigTralByte = 0;
		}

	}else { //!CountryInfo.bUnicodeMode
		CountryInfo.nTralByte = 1;
		CountryInfo.sTralByte[0] = 0x00;
		CountryInfo.eTralByte[0] = 0xff;
	} //!CountryInfo.bUnicodeMode
}


int 
SetLeadByteRange( 
TCHAR * Coderange,
int nCode)
{
	// Calculate LeadByte Range 
	TCHAR	*pStr1, *pStr2;
	WORD	wLow, wHigh;
	
	pStr1 = pStr2 = Coderange;
	while(1){
		if(( pStr2 = Mytcschr( pStr1, '-')) != NULL){
			*pStr2 = '\0';
			wLow = (WORD)Mytcstol( pStr1, (TCHAR **)0, 16);
			CountryInfo.sRange[nCode] = wLow;
			CountryInfo.sLeadByte[nCode] = HIBYTE( wLow);	
			pStr2++;
			pStr1 = pStr2;
		}else	return -1;

		if(( pStr2 = Mytcschr( pStr1, ',')) != NULL){
			*pStr2 = '\0';
			wHigh = (WORD)Mytcstol( pStr1, (TCHAR **)0, 16);
			CountryInfo.eRange[nCode] = (unsigned short)wHigh;
			CountryInfo.eLeadByte[nCode] = HIBYTE( wHigh);	
			pStr2++;
			pStr1 = pStr2;
		}else{
			wHigh = (WORD)Mytcstol( pStr1, (TCHAR **)0, 16);
			CountryInfo.eRange[nCode] = (unsigned short)wHigh;
			CountryInfo.eLeadByte[nCode] = HIBYTE( wHigh);	
			break;
		}
		nCode++;
	}

	CountryInfo.nLeadByte = ++nCode;
	CountryInfo.nRange = nCode;
	return nCode;
}

#ifdef BUILD_ON_WINNT
/**************************************************************************\
 * CorrectTralByteRange                                                   * 
 * Correct trailbyte range of EUDC range with each of original trail byte *
 * ranges.  It is used by countries where EUDC trail byte range changes   *
 * with selection of different EUDC range, for example CHS.               *
\**************************************************************************/
void
CorrectTrailByteRange(
int nIndex)
{
    COUNTRYINFO *Info;
    int i, Unique=0;
    
	if (CountryInfo.bUnicodeMode)
		return;

    Info=&CountryInfo;
    for (i=0; i< Info->nOrigTralByte; i++){
        //take the smaller of the two ranges.
        Info->sTralByte[Unique] = max(LOBYTE(Info->sRange[nIndex]),
                                      Info->sOrigTralByte[i]);
        Info->eTralByte[Unique] = min(LOBYTE(Info->eRange[nIndex]),
                                      Info->eOrigTralByte[i]);

        //we keep valid ranges and overwrite invalid one with next loop 
        if (Info->eTralByte[Unique] >= Info->sTralByte[Unique])
            Unique +=1;
    }
    Info->nTralByte=Unique;
}
#endif // BUILD_ON_WINNT
