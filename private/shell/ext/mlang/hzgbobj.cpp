// ============================================================================
// Internet Character Set Conversion: Input from HZ-GB-2312
// ============================================================================

#include "private.h"
#include "fechrcnv.h"
#include "hzgbobj.h"
#include "codepage.h"

/******************************************************************************
**************************   C O N S T R U C T O R   **************************
******************************************************************************/

CInccHzGbIn::CInccHzGbIn(UINT uCodePage, int nCodeSet) : CINetCodeConverter(uCodePage, nCodeSet)
{
    Reset();    // initialization
    return ;
}

/******************************************************************************
*******************************   R E S E T   *********************************
******************************************************************************/

void CInccHzGbIn::Reset()
{
	m_pfnConv = ConvMain;
	m_pfnCleanUp = CleanUpMain;
	m_fGBMode = FALSE;
	m_tcLeadByte = 0 ;
	m_nESCBytes = 0 ;  
    return ;
}

/******************************************************************************
*************************   C O N V E R T   C H A R   *************************
******************************************************************************/

HRESULT CInccHzGbIn::ConvertChar(UCHAR tc, int cchSrc)
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

BOOL CInccHzGbIn::CleanUp()
{
	return (this->*m_pfnCleanUp)();
}

/******************************************************************************
****************************   C O N V   M A I N   ****************************
******************************************************************************/

BOOL CInccHzGbIn::ConvMain(UCHAR tc)
{
	BOOL fDone = TRUE;

	if (!m_fGBMode) {
		if (tc == '~') {
			m_pfnConv = ConvTilde;
			m_pfnCleanUp = CleanUpTilde;
			m_nESCBytes = 1 ;  
		} else {
			fDone = Output(tc);
		}
	} else {
		if (tc >= 0x20 && tc <= 0x7e) {
			m_pfnConv = ConvDoubleByte;
			m_pfnCleanUp = CleanUpDoubleByte;
			m_tcLeadByte = tc;
		} else {
			fDone = Output(tc);
		}
	}
	return fDone;
}

/******************************************************************************
************************   C L E A N   U P   M A I N   ************************
******************************************************************************/

BOOL CInccHzGbIn::CleanUpMain()
{
	return TRUE;
}

/******************************************************************************
***************************   C O N V   T I L D E   ***************************
******************************************************************************/

BOOL CInccHzGbIn::ConvTilde(UCHAR tc)
{
	m_pfnConv = ConvMain;
	m_pfnCleanUp = CleanUpMain;

	m_nESCBytes = 0 ;  

	switch (tc) {
	case '~':
		return Output('~');

	case '{':
		m_fGBMode = TRUE;
		return TRUE;

	case '\n':
		return TRUE; // Just eat it

	default:
		(void)Output('~');
		if (SUCCEEDED(ConvertChar(tc)))
            return TRUE;
        else
            return FALSE;
	}
}

/******************************************************************************
***********************   C L E A N   U P   T I L D E   ***********************
******************************************************************************/

BOOL CInccHzGbIn::CleanUpTilde()
{
	m_pfnConv = ConvMain;
	m_pfnCleanUp = CleanUpMain;

	return Output('~');
}

/******************************************************************************
*********************   C O N V   D O U B L E   B Y T E   *********************
******************************************************************************/

BOOL CInccHzGbIn::ConvDoubleByte(UCHAR tc)
{
	BOOL fRet ;
	m_pfnConv = ConvMain;
	m_pfnCleanUp = CleanUpMain;

	if (m_tcLeadByte >= 0x21 && m_tcLeadByte <= 0x77 && tc >= 0x21 && tc <= 0x7e) { // Check if GB char
		(void)Output(m_tcLeadByte | 0x80);
		fRet = Output(tc | 0x80);
	} else if (m_tcLeadByte == '~' && tc == '}') { // 0x7e7d
		m_fGBMode = FALSE;
		fRet = TRUE;
	} else if (m_tcLeadByte >= 0x78 && m_tcLeadByte <= 0x7d && tc >= 0x21 && tc <= 0x7e) { // Check if non standard extended code
		(void)Output((UCHAR)0xa1); // Output blank box symbol
		fRet = Output((UCHAR)0xf5);
	} else if (m_tcLeadByte == '~') {
		(void)Output('~'); // Output blank box symbol
		fRet = Output(tc);
	} else if (m_tcLeadByte == ' ') {
		fRet = Output(tc);
	} else if (tc == ' ') {
		(void)Output((UCHAR)0xa1); // Output space symbol
		fRet = Output((UCHAR)0xa1);
	} else {
		(void)Output(m_tcLeadByte);
		fRet = Output(tc);
	}
	m_tcLeadByte = 0 ;
	return fRet ;
}

/******************************************************************************
*****************   C L E A N   U P   D O U B L E   B Y T E   *****************
******************************************************************************/

BOOL CInccHzGbIn::CleanUpDoubleByte()
{
	m_pfnConv = ConvMain;
	m_pfnCleanUp = CleanUpMain;

	return Output(m_tcLeadByte);
}

int CInccHzGbIn::GetUnconvertBytes()
{
    if (m_tcLeadByte)
        return 1;
    else if ( m_nESCBytes )
        return 1;
    else
	    return 0;
}

DWORD CInccHzGbIn::GetConvertMode()
{
    return ( m_fGBMode ? 1 : 0 ) ;
}

void CInccHzGbIn::SetConvertMode(DWORD mode)
{
    Reset();    // initialization
    if ( mode & 0x01 )
        m_fGBMode = TRUE ;
    else
        m_fGBMode = FALSE ;

 	return ;
}

// ============================================================================
// Internet Character Set Conversion: Output to HZ-GB-2312
// ============================================================================

/******************************************************************************
**************************   C O N S T R U C T O R   **************************
******************************************************************************/

CInccHzGbOut::CInccHzGbOut(UINT uCodePage, int nCodeSet, DWORD dwFlag, WCHAR * lpFallBack) : CINetCodeConverter(uCodePage, nCodeSet)
{
    Reset();    // initialization
    _dwFlag = dwFlag;
    _lpFallBack = lpFallBack;
    return ;
}

/******************************************************************************
*******************************   R E S E T   *********************************
******************************************************************************/
void CInccHzGbOut::Reset()
{
	m_fDoubleByte = FALSE;
	m_fGBMode = FALSE;
	m_tcLeadByte = 0 ;
    return ;
}

/******************************************************************************
*************************   C O N V E R T   C H A R   *************************
******************************************************************************/

HRESULT CInccHzGbOut::ConvertChar(UCHAR tc, int cchSrc)
{
	BOOL fDone = TRUE;
	HRESULT hr = S_OK;


    if (!m_fDoubleByte) 
    {
        if (tc & 0x80) 
        {
            m_fDoubleByte = TRUE;
            m_tcLeadByte = tc;
        }
        else 
        {
            if (m_fGBMode) 
            {
                Output('~');
                fDone = Output('}');
                m_fGBMode = FALSE;
            }
            fDone = Output(tc);
        }
    } 
    else 
    {
        UCHAR szDefaultChar[3] = {0x3f}; // possible DBCS + null    


        if (_lpFallBack && (_dwFlag & MLCONVCHARF_USEDEFCHAR))
        {
            // only take SBCS, no DBCS character
            if ( 1 != WideCharToMultiByte(CP_KOR_5601, 0,
                               (LPCWSTR)_lpFallBack, 1,
                               (LPSTR)szDefaultChar, ARRAYSIZE(szDefaultChar), NULL, NULL ))
                szDefaultChar[0] = 0x3f;
        }

        m_fDoubleByte = FALSE;
// a-ehuang: Bug# 31726, send all out of range code to convert to NCR
//			 RFC 1843 => valid HZ code range: leading byte 0x21 - 0x77, 2nd byte 0x21 - 0x7e
        if ( (m_tcLeadByte < 0xa1 || m_tcLeadByte > 0xf7) || (tc < 0xa1 || tc > 0xfe) )
// end-31726
        {
            // End Escape sequence for NCR entity output
            if (m_fGBMode)
            {
                Output('~');
                Output('}');
                m_fGBMode = FALSE;
            }

            if (_dwFlag & (MLCONVCHARF_NCR_ENTITIZE|MLCONVCHARF_NAME_ENTITIZE))
            {
                char    szChar[3] = {0};
                char    szDstStr[10] = {0};
                WCHAR   szwChar[2];
                int     cCount;

                szChar[0] = m_tcLeadByte;
                szChar[1] = tc;
                
                if (MultiByteToWideChar(CP_CHN_GB, 0, szChar, -1, szwChar, ARRAYSIZE(szwChar)))
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
        }
        else
        {
            if (!m_fGBMode) 
            {
                Output('~');
                Output('{');
                m_fGBMode = TRUE;
            }

            Output(m_tcLeadByte & 0x7f);
            fDone = Output(tc & 0x7f);
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

BOOL CInccHzGbOut::CleanUp()
{
	if (!m_fGBMode) {
		return TRUE;
	} else {
		m_fGBMode = FALSE ;
		(void)Output('~');
		return Output('}');
	}
}

int CInccHzGbOut::GetUnconvertBytes()
{
    if (m_tcLeadByte)
        return 1;
    else
	    return 0;
}

DWORD CInccHzGbOut::GetConvertMode()
{
 	return 0 ;
}

void CInccHzGbOut::SetConvertMode(DWORD mode)
{
    Reset();    // initialization
 	return ;
}

