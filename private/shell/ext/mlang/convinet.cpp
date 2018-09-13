#include "private.h"
#include "jisobj.h"
#include "eucjobj.h"
#include "hzgbobj.h"
#include "kscobj.h"

#include "utf8obj.h"
#include "utf7obj.h"

#include "fechrcnv.h"

#include "codepage.h"

#include "ichrcnv.h"



HRESULT CICharConverter::KSC5601ToEUCKR(LPCSTR lpSrcStr, LPINT lpnSrcSize, LPSTR lpDestStr, int cchDest, LPINT lpnSize)
{
    int nSize=0;
    int i=0;
    HRESULT hr = S_OK;
    UCHAR szDefaultChar[3] = {0x3f}; // possible DBCS + null    


    if (_lpFallBack && (_dwFlag & MLCONVCHARF_USEDEFCHAR))
    {
        // only take SBCS, no DBCS character
        if ( 1 != WideCharToMultiByte(CP_KOR_5601, 0,
                               (LPCWSTR)_lpFallBack, 1,
                               (LPSTR)szDefaultChar, ARRAYSIZE(szDefaultChar), NULL, NULL ))
            szDefaultChar[0] = 0x3f;
    }


    while(i < *lpnSrcSize)
    {
        // Check space
        if (lpDestStr && (nSize > cchDest))
            break;

        //  DBCS
        if (((UCHAR)lpSrcStr[i] >= 0x81 && (UCHAR)lpSrcStr[i] <= 0xFE) && (i+1 < *lpnSrcSize))
        {

            // UHC 
            if (!((UCHAR)lpSrcStr[i] >= 0xA1 && (UCHAR)lpSrcStr[i] <= 0xFE &&
                  (UCHAR)lpSrcStr[i+1] >= 0xA1 && (UCHAR)lpSrcStr[i+1] <= 0xFE))

            {
                // use NCR if flag specified
                if (_dwFlag & (MLCONVCHARF_NCR_ENTITIZE|MLCONVCHARF_NAME_ENTITIZE))
                {
                    char    szDstStr[10] = {0};
                    WCHAR   szwChar[2];
                    int     cCount;
               
                    if (MultiByteToWideChar(CP_KOR_5601, 0, &lpSrcStr[i], 2, szwChar, ARRAYSIZE(szwChar)))
                    {
                        // Caculate NCR length
                        _ultoa((unsigned long)szwChar[0], (char*)szDstStr, 10);
                        cCount = lstrlenA(szDstStr)+3;
                        // Not enough space for NCR entity
                        if (lpDestStr)
                        {
                            if (nSize+cCount > cchDest)
                                break;
                            // Output NCR entity
                            else
                            {                                    
                                *lpDestStr ++= '&';
                                *lpDestStr ++= '#';
                                for (int j=0; j< cCount-3; j++)
                                    *lpDestStr++=szDstStr[j];
                                *lpDestStr ++= ';';
                            }
                        }
                        nSize += cCount;
                    }
                    else
                    {
                        if (lpDestStr)
                        {
                            if (nSize+1 > cchDest)
                                break;
                            *lpDestStr++=szDefaultChar[0];
                        }
                        nSize++;
                        hr = S_FALSE;
                    }
                }
                // use default char, question mark
                else
                {
                    if (lpDestStr)
                    {
                        if (nSize+1 > cchDest)
                            break;
                        *lpDestStr++=szDefaultChar[0];
                    }
                    nSize++;
                    hr = S_FALSE;
                }
                i += 2;
            }
            // Wansung
            else
            {
                if (lpDestStr)
                {
                    if (nSize+2 > cchDest)
                        break;
                    *lpDestStr++=lpSrcStr[i];
                    *lpDestStr++=lpSrcStr[i+1];
                }
                i+=2;
                nSize += 2;
            }
        }
        // SBCS
        else
        {
            if (lpDestStr)
            {
                if (nSize+1 > cchDest)
                    break; 
                *lpDestStr++=lpSrcStr[i];
            }
            nSize++;
            i++;
        }
    } // End of loop

    if (lpnSize)
        *lpnSize = nSize;

    return hr;
}


/******************************************************************************
******************   C O N V E R T   I N E T   S T R I N G   ******************
******************************************************************************/
HRESULT CICharConverter::CreateINetString(BOOL fInbound, UINT uCodePage, int nCodeSet)
{
    if (_hcins)
    {
        delete _hcins ;
        _hcins = NULL ;
    }

	if (fInbound) { // Inbound
		if (uCodePage == CP_JPN_SJ && ( nCodeSet == CP_ISO_2022_JP ||
		    nCodeSet == CP_ISO_2022_JP_ESC || nCodeSet == CP_ISO_2022_JP_SIO ))
		    // JIS
			_hcins = new CInccJisIn(uCodePage, nCodeSet);
		else if (uCodePage == CP_JPN_SJ && nCodeSet == CP_EUC_JP ) // EUC
			_hcins = new CInccEucJIn(uCodePage, nCodeSet);
		else if (uCodePage == CP_CHN_GB && nCodeSet == CP_CHN_HZ ) // HZ-GB
			_hcins = new CInccHzGbIn(uCodePage, nCodeSet);
		else if (uCodePage == CP_KOR_5601 && nCodeSet == CP_ISO_2022_KR )
			_hcins = new CInccKscIn(uCodePage, nCodeSet);
		else if (uCodePage == CP_UCS_2 && nCodeSet == CP_UTF_8 )
			_hcins = new CInccUTF8In(uCodePage, nCodeSet);
		else if (uCodePage == CP_UCS_2 && nCodeSet == CP_UTF_7 )
			_hcins = new CInccUTF7In(uCodePage, nCodeSet);

	} else { // Outbound
		if (uCodePage == CP_JPN_SJ && ( nCodeSet == CP_ISO_2022_JP ||
		    nCodeSet == CP_ISO_2022_JP_ESC || nCodeSet == CP_ISO_2022_JP_SIO ))
            // JIS
			_hcins = new CInccJisOut(uCodePage, nCodeSet);
		else if (uCodePage == CP_JPN_SJ && nCodeSet == CP_EUC_JP ) // EUC
			_hcins = new CInccEucJOut(uCodePage, nCodeSet, _dwFlag, _lpFallBack);
		else if (uCodePage == CP_CHN_GB && nCodeSet == CP_CHN_HZ ) // HZ-GB
			_hcins = new CInccHzGbOut(uCodePage, nCodeSet, _dwFlag, _lpFallBack);
		else if (uCodePage == CP_KOR_5601 && nCodeSet == CP_ISO_2022_KR )
			_hcins = new CInccKscOut(uCodePage, nCodeSet, _dwFlag, _lpFallBack);
		else if (uCodePage == CP_UCS_2 && nCodeSet == CP_UTF_8 )
			_hcins = new CInccUTF8Out(uCodePage, nCodeSet);
		else if (uCodePage == CP_UCS_2 && nCodeSet == CP_UTF_7 )
			_hcins = new CInccUTF7Out(uCodePage, nCodeSet);

	}

    // recode the dst codepage
    if ( _hcins )
        _hcins_dst =  nCodeSet ;

    return S_OK ;
}

HRESULT CICharConverter::DoConvertINetString(LPDWORD lpdwMode, BOOL fInbound, UINT uCodePage, int nCodeSet,
      LPCSTR lpSrcStr, LPINT lpnSrcSize, LPSTR lpDestStr, int cchDest, LPINT lpnSize)
{
    HRESULT hr = S_OK;
	HCINS hcins = NULL;
	int nSize = 0 ;
    int cchSrc = *lpnSrcSize ;

	if (!lpnSize)
		lpnSize = &nSize;

	if (!uCodePage) // Get default code page if nothing speicified
		uCodePage = GetACP();

	if (!lpSrcStr && cchSrc < 0) // Get length of lpSrcStr if not given, assuming lpSrcStr is a zero terminate string.
		cchSrc = lstrlenA(lpSrcStr) + 1;

	if (!_hcins || ( nCodeSet != _hcins_dst ) )
        CreateINetString(fInbound,uCodePage,nCodeSet);

	if (_hcins ) { // Context created, it means DBCS
		int nTempSize = 0 ;
        
        // restore previous mode SO/SI ESC etc.
        ((CINetCodeConverter*)_hcins)->SetConvertMode(*lpdwMode);

		// if it is a JIS output set Kana mode
		if (!fInbound && uCodePage == CP_JPN_SJ && ( nCodeSet == CP_ISO_2022_JP ||
		    nCodeSet == CP_ISO_2022_JP_ESC || nCodeSet == CP_ISO_2022_JP_SIO ))
		    // JIS
		    ((CInccJisOut*)_hcins)->SetKanaMode(nCodeSet);

		if (!lpDestStr || !cchDest) // Get the converted size
		{
			hr = ((CINetCodeConverter*)_hcins)->GetStringSizeA(lpSrcStr, cchSrc, lpnSize);
			if (0 == fInbound) 
			    hr = ((CINetCodeConverter*)_hcins)->GetStringSizeA(NULL, 0, &nTempSize);
		}
		else // Perform actual converting
		{
			hr = ((CINetCodeConverter*)_hcins)->ConvertStringA(lpSrcStr, cchSrc, lpDestStr, cchDest, lpnSize);
			if (0 == fInbound) 
            {
                HRESULT _hr = ((CINetCodeConverter*)_hcins)->ConvertStringA(NULL, 0, lpDestStr+*lpnSize, cchDest-*lpnSize, &nTempSize);
			    if (S_OK != _hr)
                    hr = _hr;
            }
		}

		*lpnSize += nTempSize;

        // get number of unconvetable bytes 
        if ( lpnSrcSize && ((CINetCodeConverter*)_hcins)->GetUnconvertBytes() )
            *lpnSrcSize = cchSrc -((CINetCodeConverter*)_hcins)->GetUnconvertBytes();

        // only save current mode SO/SI ESC if we are perform actual converting
        // we need this if statement because for two stages plus conversion.
        // It will inquire the size first then convert from IWUU or UUWI.

		if (lpDestStr && lpdwMode )
            *lpdwMode = ((CINetCodeConverter*)_hcins)->GetConvertMode();

//        delete hcins;
	} else { 
        // Internet encodings that have same encoding scheme as their family encodings
        switch (nCodeSet)
        {
            case CP_EUC_KR:
                hr = KSC5601ToEUCKR(lpSrcStr, lpnSrcSize, lpDestStr, cchDest, lpnSize);
                break;

            default:
                if (!lpDestStr || !cchDest) // Get the converted size
                   *lpnSize = cchSrc ;
                else
                {
                   *lpnSize = min(cchSrc, cchDest);
                   if (*lpnSize)
                      MoveMemory(lpDestStr, lpSrcStr, *lpnSize);
                }
        }
	}

    return hr;
}

