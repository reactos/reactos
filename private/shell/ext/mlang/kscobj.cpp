// ============================================================================
// Internet Character Set Conversion: Input from ISO-2022-KR
// ============================================================================

#include "private.h"
#include "fechrcnv.h"
#include "kscobj.h"
#include "codepage.h"

/******************************************************************************
**************************   C O N S T R U C T O R   **************************
******************************************************************************/

CInccKscIn::CInccKscIn(UINT uCodePage, int nCodeSet) : CINetCodeConverter(uCodePage, nCodeSet)
{
    Reset();    // initialization
    return ;
}

/******************************************************************************
*******************************   R E S E T   *********************************
******************************************************************************/

void CInccKscIn::Reset()
{
	m_pfnConv = ConvMain;
	m_pfnCleanUp = CleanUpMain;
	m_fShift = FALSE;
    // BUGBUG: bug #57570, Korean ISP DACOM only labels one designator in the 
    // conversion of a MIME mail. To decode the other part of MIME correctly, 
    // we need to decode the ISO document or MIME message even there is no
    // designator "esc ) C".
	m_fKorea = TRUE;
	m_nESCBytes = 0 ;
	m_fLeadByte = FALSE ;
    return ;
}

/******************************************************************************
*************************   C O N V E R T   C H A R   *************************
******************************************************************************/

HRESULT CInccKscIn::ConvertChar(UCHAR tc, int cchSrc)
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

BOOL CInccKscIn::CleanUp()
{
	return (this->*m_pfnCleanUp)();
}

/******************************************************************************
****************************   C O N V   M A I N   ****************************
******************************************************************************/

BOOL CInccKscIn::ConvMain(UCHAR tc)
{
	BOOL fDone = TRUE;

	if (tc == ESC) {
		m_pfnConv = ConvEsc;
		m_pfnCleanUp = CleanUpEsc;
		m_nESCBytes++ ;
	} else {
		if (m_fKorea) {
			switch (tc) {
			case SO:
				m_fShift = TRUE;
				break;

			case SI:
				m_fShift = FALSE;
				m_fLeadByte = FALSE ;
				break;

			default:
				if (m_fShift) {
					switch (tc) {
					case ' ':
					case '\t':
					case '\n':
						fDone = Output(tc);
						break;

					default:
						fDone = Output(tc | 0x80);
						m_fLeadByte = ~m_fLeadByte ;
						break;
					}
				} else {
					fDone = Output(tc);
				}
				break;
			}
		} else {
			fDone = Output(tc);
		}
	}
	return fDone;
}

/******************************************************************************
************************   C L E A N   U P   M A I N   ************************
******************************************************************************/

BOOL CInccKscIn::CleanUpMain()
{
	return TRUE;
}

/******************************************************************************
*****************************   C O N V   E S C   *****************************
******************************************************************************/

BOOL CInccKscIn::ConvEsc(UCHAR tc)
{
	if (tc == ISO2022_IN_CHAR) {
		m_pfnConv = ConvIsoIn;
		m_pfnCleanUp = CleanUpIsoIn;
		m_nESCBytes++ ;
		return TRUE;
	} else {
		m_pfnConv = ConvMain;
		m_pfnCleanUp = CleanUpMain;
		m_nESCBytes = 0 ;
		(void)Output(ESC);
        if (SUCCEEDED(ConvertChar(tc)))
            return TRUE;
        else
            return FALSE;
    }
}

/******************************************************************************
*************************   C L E A N   U P   E S C   *************************
******************************************************************************/

BOOL CInccKscIn::CleanUpEsc()
{
	m_pfnConv = ConvMain;
	m_pfnCleanUp = CleanUpMain;
	m_nESCBytes = 0 ;
	return Output(ESC);
}

/******************************************************************************
**************************   C O N V   I S O   I N   **************************
******************************************************************************/

BOOL CInccKscIn::ConvIsoIn(UCHAR tc)
{
	if (tc == ISO2022_IN_KR_CHAR_1) {
		m_pfnConv = ConvIsoInKr;
		m_pfnCleanUp = CleanUpIsoInKr;
		m_nESCBytes++ ;
		return TRUE;
	} else {
		m_pfnConv = ConvMain;
		m_pfnCleanUp = CleanUpMain;
		m_nESCBytes = 0 ;
		(void)Output(ESC);
		(void)ConvertChar(ISO2022_IN_CHAR);
        if (SUCCEEDED(ConvertChar(tc)))
            return TRUE;
        else
            return FALSE;
	}
}

/******************************************************************************
**********************   C L E A N   U P   I S O   I N   **********************
******************************************************************************/

BOOL CInccKscIn::CleanUpIsoIn()
{
	m_pfnConv = ConvMain;
	m_pfnCleanUp = CleanUpMain;
   	m_nESCBytes = 0 ;

	(void)Output(ESC);
	(void)ConvertChar(ISO2022_IN_CHAR);
	return CleanUp();
}

/******************************************************************************
***********************   C O N V   I S O   I N   K R   ***********************
******************************************************************************/

BOOL CInccKscIn::ConvIsoInKr(UCHAR tc)
{
	m_pfnConv = ConvMain;
	m_pfnCleanUp = CleanUpMain;
   	m_nESCBytes = 0 ;

	if (tc == ISO2022_IN_KR_CHAR_2) {
		m_fKorea = TRUE;
		return TRUE;
	} else {
		(void)Output(ESC);
		(void)ConvertChar(ISO2022_IN_CHAR);
		(void)ConvertChar(ISO2022_IN_KR_CHAR_1);
        if (SUCCEEDED(ConvertChar(tc)))
            return TRUE;
        else
            return FALSE;
	}
}

/******************************************************************************
*******************   C L E A N   U P   I S O   I N   K R   *******************
******************************************************************************/

BOOL CInccKscIn::CleanUpIsoInKr()
{
	m_pfnConv = ConvMain;
	m_pfnCleanUp = CleanUpMain;
   	m_nESCBytes = 0 ;

	(void)Output(ESC);
	(void)ConvertChar(ISO2022_IN_CHAR);
	(void)ConvertChar(ISO2022_IN_KR_CHAR_1);
	return CleanUp();
}

int CInccKscIn::GetUnconvertBytes()
{
    if ( m_fLeadByte )
        return 1 ;
    else if ( m_nESCBytes )
        return m_nESCBytes < 4 ? m_nESCBytes : 3 ;
    else
        return 0 ;
}

DWORD CInccKscIn::GetConvertMode()
{
    // 0xC431 -> 50225 ISO-2022-KR
    return ( m_fKorea ? 1 : 0 ) + ( m_fShift ? 2 : 0 ) | 0xC4310000 ;
}

void CInccKscIn::SetConvertMode(DWORD mode)
{
    Reset();    // initialization

    if ( mode & 0x00000001 )
        m_fKorea = TRUE ;
    if ( mode & 0x00000002 ) 
        m_fShift = TRUE ;
    return ;
}

// ============================================================================
// Internet Character Set Conversion: Output to ISO-2022-KSC
// ============================================================================

/******************************************************************************
**************************   C O N S T R U C T O R   **************************
******************************************************************************/

CInccKscOut::CInccKscOut(UINT uCodePage, int nCodeSet, DWORD dwFlag, WCHAR *lpFallBack) : CINetCodeConverter(uCodePage, nCodeSet)
{
    Reset();    // initialization
    _dwFlag = dwFlag;
    _lpFallBack = lpFallBack;
    return ;
}

/******************************************************************************
*******************************   R E S E T   *********************************
******************************************************************************/

void CInccKscOut::Reset()
{
	m_fDoubleByte = FALSE;
	m_fShift = FALSE;
	m_fKorea = FALSE;
	m_tcLeadByte = 0 ;
    return ;
}

/******************************************************************************
*************************   C O N V E R T   C H A R   *************************
******************************************************************************/

HRESULT CInccKscOut::ConvertChar(UCHAR tc, int cchSrc)
{
	BOOL fDone = TRUE;
    HRESULT hr = S_OK;

    // put designator to the top of the document
	if (!m_fKorea) {
		(void)Output(ESC);
		(void)Output(ISO2022_IN_CHAR);
		(void)Output(ISO2022_IN_KR_CHAR_1);
		(void)Output(ISO2022_IN_KR_CHAR_2);
		m_fKorea = TRUE;
	}

	if (!m_fDoubleByte) {
		if (tc > 0x80) {
			m_fDoubleByte = TRUE;
			m_tcLeadByte = tc;
		} else {
			if (m_fKorea && m_fShift) {
				(void)Output(SI);
				m_fShift = FALSE;
			}
			fDone = Output(tc);
		}
	} else {
		m_fDoubleByte = FALSE;
		if (tc > 0x40) { // Check if trail byte indicates Hangeul
			if (m_tcLeadByte > 0xa0 && tc > 0xa0) { // Check if it's a Wansung
			    if (!m_fShift) {
				    if (!m_fKorea) {
					    (void)Output(ESC);
					    (void)Output(ISO2022_IN_CHAR);
					    (void)Output(ISO2022_IN_KR_CHAR_1);
					    (void)Output(ISO2022_IN_KR_CHAR_2);
					    m_fKorea = TRUE;
				    }
				    (void)Output(SO);
				    m_fShift = TRUE;
			    }                
				(void)Output(m_tcLeadByte & 0x7f);
				fDone = Output(tc & 0x7f);
			} else {
                UCHAR szDefaultChar[3] = {0x3f}; // possible DBCS + null    


                if (_lpFallBack && (_dwFlag & MLCONVCHARF_USEDEFCHAR))
                {
                    // only take SBCS, no DBCS character
                    if ( 1 != WideCharToMultiByte(CP_KOR_5601, 0,
                               (LPCWSTR)_lpFallBack, 1,
                               (LPSTR)szDefaultChar, ARRAYSIZE(szDefaultChar), NULL, NULL ))
                        szDefaultChar[0] = 0x3f;
                }

			    // shift out if we're in DBCS mode
                if (m_fKorea && m_fShift) {
				    (void)Output(SI);
				    m_fShift = FALSE;
			    }

                if (_dwFlag & (MLCONVCHARF_NCR_ENTITIZE|MLCONVCHARF_NAME_ENTITIZE))
                {
                    char    szChar[3] = {0};
                    char    szDstStr[10] = {0};
                    WCHAR   szwChar[2];
                    int     cCount;

                    szChar[0] = m_tcLeadByte;
                    szChar[1] = tc;
                
                    if (MultiByteToWideChar(CP_KOR_5601, 0, szChar, -1, szwChar, ARRAYSIZE(szwChar)))
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
				        fDone = Output(szDefaultChar[0]); // use default char
                        hr = S_FALSE;
                    }
                }
                else
                {
				    fDone = Output(szDefaultChar[0]); // use default char
                    hr = S_FALSE;
                }
			}
		} else {
			if (m_fKorea && m_fShift) {
				(void)Output(SI);
				m_fShift = FALSE;
			}
			(void)Output(m_tcLeadByte);
			fDone = Output(tc);
		}
		m_tcLeadByte = 0 ;
	}

    if (!fDone)
        hr = E_FAIL;

	return hr;
}

/******************************************************************************
*****************************   C L E A N   U P   *****************************
******************************************************************************/

BOOL CInccKscOut::CleanUp()
{
    BOOL fDone = TRUE;

    if ( m_fShift) 
    {
        fDone = Output(SI);
        m_fShift = FALSE;
    }
    return fDone ;
}

int CInccKscOut::GetUnconvertBytes()
{
    if (m_tcLeadByte)
        return 1 ;
    else
        return 0 ;
}

DWORD CInccKscOut::GetConvertMode()
{
    // for output, we don't need write back code page. 0xC431 -> 50225 ISO-2022-KR
    return ( m_fKorea ? 1 : 0 ) +  ( m_fShift ? 2 : 0 ) ;
}

void CInccKscOut::SetConvertMode(DWORD mode)
{
    Reset();    // initialization

    if ( mode & 0x00000001 ) 
        m_fKorea = TRUE ;
    if ( mode & 0x00000002 ) 
        m_fShift = TRUE ;
    return ;
}
