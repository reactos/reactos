// =================================================================================
// Internet Character Set Detection: For Japanese
// =================================================================================

#include "private.h"
#include "detcbase.h"
#include "detcjpn.h"
#include "fechrcnv.h"
#include "codepage.h"

CIncdJapanese::CIncdJapanese()
{
	m_nScoreJis = 0;
	m_nScoreEuc = 0;
	m_nScoreSJis = 0;

	m_nISOMode = NONE ;
	m_nJISMode = REGULAR;
	m_nEucMode = REGULAR;
	m_fDoubleByteSJis = FALSE;
}

BOOL CIncdJapanese::CheckISOChar(UCHAR tc)
{
    switch (m_nISOMode) {
    case NONE:
        if ( tc == ESC )
            m_nISOMode = ISO_ESC ;
        break;
    case ISO_ESC:
        if ( tc == ISO2022_IN_CHAR )        // '$'
            m_nISOMode = ISO_ESC_IN ;
        else if ( tc == ISO2022_OUT_CHAR )
            m_nISOMode = ISO_ESC_OUT ;      // '('
        else
            m_nISOMode = NONE ;
        break;
    case ISO_ESC_IN:    // esc '$'
        m_nISOMode = NONE ;
        if ( tc == ISO2022_IN_JP_CHAR1 ||       // 'B'
                tc == ISO2022_IN_JP_CHAR2 )     // '@'
        {
            m_nJISMode = DOUBLEBYTE ;
            return TRUE ;
        }
        break;
    case ISO_ESC_OUT:   // esc '('
        m_nISOMode = NONE ;
        if ( tc == ISO2022_OUT_JP_CHAR1 ||      //	'B'
                tc == ISO2022_OUT_JP_CHAR2 )    //	'J'
        {
            m_nJISMode = REGULAR ;
            return TRUE ;
        }
        else if ( tc == ISO2022_OUT_JP_CHAR3 )   //	'I'
        {
            m_nJISMode = KATAKANA;
            return TRUE ;
        }
        break;
    }
    return FALSE;
}

BOOL CIncdJapanese::DetectChar(UCHAR tc)
{
	// JIS
	if ( CheckISOChar(tc) )
	    return FALSE;   // JIS mode change, don't need to check other type

	switch (m_nJISMode) {
	case REGULAR:
	    if (tc < 0x80)
	        m_nScoreJis += SCORE_MAJOR;
	    break;
	case DOUBLEBYTE:
	case KATAKANA:
	    m_nScoreJis += SCORE_MAJOR;
	    return FALSE;   // In JIS mode for sure, don't need to check other type
	}

	// EUC-J
	switch (m_nEucMode) {
	case REGULAR:
		if (tc >= 0xa1 && tc <= 0xfe) // Double Byte
			m_nEucMode = DOUBLEBYTE;
		else if (tc == 0x8e) // Single Byte Katakana
			m_nEucMode = KATAKANA;
		else if (tc < 0x80)
			m_nScoreEuc += SCORE_MAJOR;
		break;
	case DOUBLEBYTE:
		if (tc >= 0xa1 && tc <= 0xfe)
			m_nScoreEuc += SCORE_MAJOR * 2;
		m_nEucMode = REGULAR;
		break;
	case KATAKANA:
		if (tc >= 0xa1 && tc <= 0xdf) // Katakana range
			m_nScoreEuc += SCORE_MAJOR * 2;
		m_nEucMode = REGULAR;
		break;
	}

	// Shift-JIS
	if (!m_fDoubleByteSJis) {
		if ((tc >= 0x81 && tc <= 0x9f) || (tc >= 0xe0 && tc <= 0xfc)) // Double Byte
			m_fDoubleByteSJis = TRUE;
		else if (tc <= 0x7e || (tc >= 0xa1 && tc <= 0xdf))
			m_nScoreSJis += SCORE_MAJOR;
	} else {
		if (tc >= 0x40 && tc <= 0xfc && tc != 0x7f) // Trail Byte range
			m_nScoreSJis += SCORE_MAJOR * 2;
		m_fDoubleByteSJis = FALSE;
	}

	return FALSE;
}

int CIncdJapanese::GetDetectedCodeSet()
{
	int nMaxScore = m_nScoreSJis;
	int nCodeSet = CP_JPN_SJ;

	if (m_nScoreEuc > nMaxScore) {
		nMaxScore = m_nScoreEuc;
		nCodeSet = CP_EUC_JP ; // EUC
	} else if (m_nScoreEuc == nMaxScore && m_nScoreEuc > MIN_JPN_DETECTLEN * SCORE_MAJOR) {
        // If the given string is not long enough, we should rather choose SJIS
        // This helps fix the bug when we are just given Window Title
        // at Shell HyperText view.
		nCodeSet = CP_EUC_JP ; // EUC
	}

	if (m_nScoreJis > nMaxScore) 
		nCodeSet = CP_ISO_2022_JP ; // JIS
	else if (m_nScoreJis == nMaxScore) // Even score means all 7bits chars
		nCodeSet = 0 ;    // in this case, it maybe just pure ANSI data, we return it is ambiguous.
	return nCodeSet;
}
