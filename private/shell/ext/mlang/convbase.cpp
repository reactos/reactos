// ============================================================================
// Internet Character Set Conversion: Base Class
// ============================================================================

#include "private.h"
#include "convbase.h"
#include "fechrcnv.h"
#include "codepage.h"

/******************************************************************************
**************************   C O N S T R U C T O R   **************************
******************************************************************************/

CINetCodeConverter::CINetCodeConverter()
{
	m_uCodePage = 0;
	m_nCodeSet = CP_UNDEFINED;
	m_cchOverflow = 0;
}

CINetCodeConverter::CINetCodeConverter(UINT uCodePage, int nCodeSet)
{
	m_uCodePage = uCodePage;
	m_nCodeSet = nCodeSet;
	m_cchOverflow = 0;
}

/******************************************************************************
********************   G E T   S T R I N G   S I Z E   A   ********************
******************************************************************************/

HRESULT CINetCodeConverter::GetStringSizeA(LPCSTR lpSrcStr, int cchSrc, LPINT lpnSize)
{
	m_fOutput = FALSE;

	return WalkString(lpSrcStr, cchSrc, lpnSize);
}

/******************************************************************************
*********************   C O N V E R T   S T R I N G   A   *********************
******************************************************************************/

HRESULT CINetCodeConverter::ConvertStringA(LPCSTR lpSrcStr, int cchSrc, LPSTR lpDestStr, int cchDest, LPINT lpnSize)
{
	m_fOutput = TRUE;
	m_lpDestStr = lpDestStr;
	m_cchDest = cchDest;

	if ( !OutputOverflowBuffer() ) // Output those chars which could not be output at previous time.
		return FALSE;

	return WalkString(lpSrcStr, cchSrc, lpnSize);
}

/******************************************************************************
**************************   W A L K   S T R I N G   **************************
******************************************************************************/

HRESULT CINetCodeConverter::WalkString(LPCSTR lpSrcStr, int cchSrc, LPINT lpnSize)
{
        HRESULT hr = S_OK;

        m_cchOutput = 0;

        if (lpSrcStr) {
            while (cchSrc-- > 0) {
            HRESULT _hr = ConvertChar(*lpSrcStr++, cchSrc);
            if (!SUCCEEDED(_hr))
            {   
                hr = _hr;
                break;
            }
            else
                if (hr == S_OK && _hr == S_FALSE)
                    hr = S_FALSE;
            }
	} else {
        if (!CleanUp())
            hr = E_FAIL;
	}

	if (lpnSize)
		*lpnSize = m_cchOutput;

	return hr;
}

/******************************************************************************
**************************   E N D   O F   D E S T   **************************
******************************************************************************/

BOOL CINetCodeConverter::EndOfDest(UCHAR tc)
{
	if (m_cchOverflow < MAXOVERFLOWCHARS) {
		m_OverflowBuffer[m_cchOverflow++] = tc;
		return TRUE;
	} else {
		return FALSE; // Overflow on Overflow buffer, No way
	}
}

/******************************************************************************
***************   O U T P U T   O V E R F L O W   B U F F E R   ***************
******************************************************************************/

BOOL CINetCodeConverter::OutputOverflowBuffer()
{
	for (int n = 0; n < m_cchOverflow; n++) {
		if (m_cchOutput < m_cchDest) {
			*m_lpDestStr++ = m_OverflowBuffer[n];
			m_cchOutput++;
		} else {
			// Overflow again
			for (int n2 = 0; n < m_cchOverflow; n++, n2++)
				m_OverflowBuffer[n2] = m_OverflowBuffer[n];
			m_cchOverflow = n2;
			return FALSE;
		}
	}
	return TRUE;
}
