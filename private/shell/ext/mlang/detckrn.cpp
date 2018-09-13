// =================================================================================
// Internet Character Set Detection: For Korean
// =================================================================================

#include "private.h"
#include "detcbase.h"
#include "detckrn.h"
#include "fechrcnv.h"
#include "codepage.h"

CIncdKorean::CIncdKorean()
{
    m_nEscMode = NO_ESC ;
	m_bFindDesigator = FALSE ;
	m_nCharCount = 0 ;
}

BOOL CIncdKorean::DetectChar(UCHAR tc)
{
	switch ( tc ) 
    {
	    case ESC:
	   	    if ( m_nEscMode == NO_ESC )
        	    m_nEscMode = sESC ;
            else
        	    m_nEscMode = NO_ESC ;
		    break;
        case ISO2022_IN_CHAR :      // 	'$'
	   	    if ( m_nEscMode == sESC )
        	    m_nEscMode = sESC_1 ;
            else
        	    m_nEscMode = NO_ESC ;
		    break;
        case ISO2022_IN_KR_CHAR_1 : //	')'
	   	    if ( m_nEscMode == sESC_1 )
        	    m_nEscMode = sESC_2 ;
            else
        	    m_nEscMode = NO_ESC ;
		    break;
        case ISO2022_IN_KR_CHAR_2 : //	'C'
	   	    if ( m_nEscMode == sESC_2 )
            {
                m_bFindDesigator = TRUE ;
            	return TRUE ;
            }
		    break;
        default:
       	    m_nEscMode = NO_ESC ;
		    break;
    }

    // only look ahead a max 1024 char
    if ( ++m_nCharCount > 1024 )
	    return TRUE;

	return FALSE;
}

int CIncdKorean::GetDetectedCodeSet()
{
    if ( m_bFindDesigator )
		return CP_ISO_2022_KR ; // ISO
    else
		return CP_KOR_5601 ; // Korean Windows 
}
