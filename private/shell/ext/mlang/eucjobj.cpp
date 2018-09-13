// ============================================================================
// Internet Character Set Conversion: Input from EUC-JP
// ============================================================================

#include "private.h"
#include "fechrcnv.h"
#include "eucjobj.h"
#include "codepage.h"

/******************************************************************************
**************************   C O N S T R U C T O R   **************************
******************************************************************************/

CInccEucJIn::CInccEucJIn(UINT uCodePage, int nCodeSet) : CINetCodeConverter(uCodePage, nCodeSet)
{
    Reset();    // initialization
    return ;
}

/******************************************************************************
*******************************   R E S E T   *********************************
******************************************************************************/

void CInccEucJIn::Reset()
{
	m_pfnConv = ConvMain;
	m_pfnCleanUp = CleanUpMain;
	m_tcLeadByte = 0 ;
    return ;
}

/******************************************************************************
*************************   C O N V E R T   C H A R   *************************
******************************************************************************/

HRESULT CInccEucJIn::ConvertChar(UCHAR tc, int cchSrc)
{
	BOOL fDone = (this->*m_pfnConv)(tc);
    if (fDone)
        return S_OK;
    else
        return E_FAIL;
}

/******************************************************************************
*****************************   C L E A N   U P   *****************************
******************************************************************************/

BOOL CInccEucJIn::CleanUp()
{
	return (this->*m_pfnCleanUp)();
}

/******************************************************************************
****************************   C O N V   M A I N   ****************************
******************************************************************************/

BOOL CInccEucJIn::ConvMain(UCHAR tc)
{
	BOOL fDone = TRUE;



	if (tc >= 0xa1 && tc <= 0xfe) {
		m_pfnConv = ConvDoubleByte;
		m_pfnCleanUp = CleanUpDoubleByte;
		m_tcLeadByte = tc;
	} else if (tc == 0x8e) { // Single Byte Katakana
		m_pfnConv = ConvKatakana;
		m_pfnCleanUp = CleanUpKatakana;
	} else {
		fDone = Output(tc);
	}
	return fDone;
}

/******************************************************************************
************************   C L E A N   U P   M A I N   ************************
******************************************************************************/

BOOL CInccEucJIn::CleanUpMain()
{
	return TRUE;
}

/******************************************************************************
*********************   C O N V   D O U B L E   B Y T E   *********************
******************************************************************************/

BOOL CInccEucJIn::ConvDoubleByte(UCHAR tc)
{
	BOOL fRet ;

	m_pfnConv = ConvMain;
	m_pfnCleanUp = CleanUpMain;
	if (m_tcLeadByte <= 0xde) { // && m_tcLeadByte >= 0xa1
		if (m_tcLeadByte % 2) // odd
			(void)Output((m_tcLeadByte - 0xa1) / 2 + 0x81);
		else // even
			(void)Output((m_tcLeadByte - 0xa2) / 2 + 0x81);
	} else { // m_tcLeadByte >= 0xdf && m_tcLeadByte <= 0xfe
		if (m_tcLeadByte % 2) // odd
			(void)Output((m_tcLeadByte - 0xdf) / 2 + 0xe0);
		else // even
			(void)Output((m_tcLeadByte - 0xe0) / 2 + 0xe0);
	}
	if (m_tcLeadByte % 2) { // odd
		if (tc >= 0xa1 && tc <= 0xdf)
			fRet = Output(tc - 0x61);
		else
			fRet = Output(tc - 0x60);
	} else { // even
		fRet = Output(tc - 2);
	}
	m_tcLeadByte = 0 ;
	return fRet ;
}

/******************************************************************************
*****************   C L E A N   U P   D O U B L E   B Y T E   *****************
******************************************************************************/

BOOL CInccEucJIn::CleanUpDoubleByte()
{
	m_pfnConv = ConvMain;
	m_pfnCleanUp = CleanUpMain;
	return TRUE;
}

/******************************************************************************
************************   C O N V   K A T A K A N A   ************************
******************************************************************************/

BOOL CInccEucJIn::ConvKatakana(UCHAR tc)
{
	m_pfnConv = ConvMain;
	m_pfnCleanUp = CleanUpMain;
	return Output(tc);
}

/******************************************************************************
********************   C L E A N   U P   K A T A K A N A   ********************
******************************************************************************/

BOOL CInccEucJIn::CleanUpKatakana()
{
	m_pfnConv = ConvMain;
	m_pfnCleanUp = CleanUpMain;
	return TRUE;
}

int CInccEucJIn::GetUnconvertBytes()
{
    if (m_tcLeadByte || m_pfnConv == ConvKatakana)
    	return 1;
    else
    	return 0;
}

DWORD CInccEucJIn::GetConvertMode()
{
    // 0xCADC -> 51932 EUC-JP (codepage)
 	return 0xCADC0000 ;
}

void CInccEucJIn::SetConvertMode(DWORD mode)
{
    Reset();
 	return ;
}


// ============================================================================
// Internet Character Set Conversion: Output to EUC-JP
// ============================================================================

/******************************************************************************
**************************   C O N S T R U C T O R   **************************
******************************************************************************/

CInccEucJOut::CInccEucJOut(UINT uCodePage, int nCodeSet, DWORD dwFlag, WCHAR *lpFallBack) : CINetCodeConverter(uCodePage, nCodeSet)
{
    Reset();    // initialization
    _dwFlag = dwFlag;
    _lpFallBack = lpFallBack;
    return ;
}

/******************************************************************************
*******************************   R E S E T   *********************************
******************************************************************************/
void CInccEucJOut::Reset()
{
	m_fDoubleByte = FALSE;
	m_tcLeadByte = 0 ;
    return ;
}

/******************************************************************************
*************************   C O N V E R T   C H A R   *************************
******************************************************************************/

HRESULT CInccEucJOut::ConvertChar(UCHAR tc, int cchSrc)
{
	BOOL fDone = TRUE;
    HRESULT hr = S_OK;

	if (!m_fDoubleByte) {
		if ((tc >= 0x81 && tc <= 0x9f) || (tc >= 0xe0 && tc <= 0xfc )) { // Double Byte Code
			m_fDoubleByte = TRUE;
			m_tcLeadByte = tc;
		} else if (tc >= 0xa1 && tc <= 0xdf) { // Single Byte Katakana Code
			(void) Output( (UCHAR) 0x8e);
			fDone = Output(tc);
		} else {
			fDone = Output(tc);
		}
	} else {

        // map extended char (0xfa40-0xfc4b) to a special range
        if (m_tcLeadByte >= 0xfa && m_tcLeadByte <= 0xfc && tc >= 0x40 )
        {
            WCHAR  wcDBCS ;

            wcDBCS = ((WCHAR) m_tcLeadByte ) << 8 | tc ;

            if ( wcDBCS >= 0xfa40 && wcDBCS <= 0xfa5b )
            {
                if ( wcDBCS <= 0xfa49 )
                    wcDBCS = wcDBCS - 0x0b51 ;
                else if ( wcDBCS >= 0xfa4a && wcDBCS <= 0xfa53 )
                    wcDBCS = wcDBCS - 0x072f6 ;
                else if ( wcDBCS >= 0xfa54 && wcDBCS <= 0xfa57 )
                    wcDBCS = wcDBCS - 0x0b5b ;
                else if ( wcDBCS == 0xfa58 )
                    wcDBCS = 0x878a ;
                else if ( wcDBCS == 0xfa59 )
                    wcDBCS = 0x8782 ;
                else if ( wcDBCS == 0xfa5a )
                    wcDBCS = 0x8784 ;
                else if ( wcDBCS == 0xfa5b )
                    wcDBCS = 0x879a ;
            }
            else if ( wcDBCS >= 0xfa5c && wcDBCS <= 0xfc4b )
            {
                if ( tc < 0x5c )
                    wcDBCS = wcDBCS - 0x0d5f;
                else if ( tc >= 0x80 && tc <= 0x9B )
                    wcDBCS = wcDBCS - 0x0d1d;
                else
                    wcDBCS = wcDBCS - 0x0d1c;
            }
            tc = (UCHAR) wcDBCS ;
            m_tcLeadByte = (UCHAR) ( wcDBCS >> 8 ) ;
        }

        // Do conversion
		if (m_tcLeadByte <= 0xef) {
			if (m_tcLeadByte <= 0x9f) { // && m_tcLeadByte >= 0x81
				if (tc <= 0x9e)
					(void)Output((m_tcLeadByte - 0x81) * 2 + 0xa1);
				else
					(void)Output((m_tcLeadByte - 0x81) * 2 + 0xa2);
			} else { // m_tcLeadByte >= 0xe0 && m_tcLeadByte <= 0xef
				if (tc <= 0x9e)
					(void)Output((m_tcLeadByte - 0xe0) * 2 + 0xdf);
				else
					(void)Output((m_tcLeadByte - 0xe0) * 2 + 0xe0);
			}
			if (tc >= 0x40 && tc <= 0x7e)
				fDone = Output(tc + 0x61);
			else if (tc >= 0x80 && tc <= 0x9e)
				fDone = Output(tc + 0x60);
			else
				fDone = Output(tc + 0x02);
		} else if (m_tcLeadByte >= 0xfa) { // && m_tcLeadByte <= 0xfc; IBM Extended Char
            UCHAR szDefaultChar[3] = {0x3f}; // possible DBCS + null    


            if (_lpFallBack && (_dwFlag & MLCONVCHARF_USEDEFCHAR))
            {
                // only take SBCS, no DBCS character
                if ( 1 != WideCharToMultiByte(CP_JPN_SJ, 0,
                               (LPCWSTR)_lpFallBack, 1,
                               (LPSTR)szDefaultChar, ARRAYSIZE(szDefaultChar), NULL, NULL ))
                    szDefaultChar[0] = 0x3f;
            }

            if (_dwFlag & (MLCONVCHARF_NCR_ENTITIZE|MLCONVCHARF_NAME_ENTITIZE))
            {
                char    szChar[3] = {0};
                char    szDstStr[10] = {0};
                WCHAR   szwChar[2];
                int     cCount;

                szChar[0] = m_tcLeadByte;
                szChar[1] = tc;
                
                if (MultiByteToWideChar(CP_JPN_SJ, 0, szChar, -1, szwChar, ARRAYSIZE(szwChar)))
                {

                    // Output NCR entity
                    Output('&');
                    Output('#');
                    _ultoa((unsigned long)szwChar[0], (char*)szDstStr, 10);
                    cCount = lstrlenA(szDstStr);
                    for (int i=0; i< cCount; i++)
                    {
                        Output(szDstStr[i]);
                    }
                    fDone = Output(';');
                }
                else
                {
    		        fDone = Output(szDefaultChar[0]);
                    hr = S_FALSE;
                }
            }
            else
            {
    		        fDone = Output(szDefaultChar[0]);
                    hr = S_FALSE;
            }
		} else {
			(void)Output(m_tcLeadByte);
			fDone = Output(tc);
		}
		m_fDoubleByte = FALSE;
		m_tcLeadByte = 0 ;
	}

    if (!fDone)
        hr = E_FAIL;

    return hr;
}

/******************************************************************************
*****************************   C L E A N   U P   *****************************
******************************************************************************/

BOOL CInccEucJOut::CleanUp()
{
	m_fDoubleByte = FALSE;
	return TRUE;
}

int CInccEucJOut::GetUnconvertBytes()
{
    if (m_tcLeadByte)
    	return 1;
    else
    	return 0;
}

DWORD CInccEucJOut::GetConvertMode()
{
 	return 0 ;
}

void CInccEucJOut::SetConvertMode(DWORD mode)
{
    Reset();
 	return ;
}
