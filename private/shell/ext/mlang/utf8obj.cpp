// ============================================================================
// Internet Character Set Conversion: Input from UTF-8
// ============================================================================

#include "private.h"
#include "fechrcnv.h"
#include "utf8obj.h"

/******************************************************************************
**************************   C O N S T R U C T O R   **************************
******************************************************************************/

CInccUTF8In::CInccUTF8In(UINT uCodePage, int nCodeSet) : CINetCodeConverter(uCodePage, nCodeSet)
{
    Reset();    // initialization
    return ;
}

/******************************************************************************
*******************************   R E S E T   *********************************
******************************************************************************/

void CInccUTF8In::Reset()
{
	m_pfnConv = ConvMain;
	m_pfnCleanUp = CleanUpMain;
	m_nByteFollow = 0 ;
	m_tcUnicode = 0 ;
	m_tcSurrogateUnicode = 0 ;
	m_nBytesUsed = 0 ;
	m_fSurrogatesPairs = FALSE;
    return ;
}

/******************************************************************************
*************************   C O N V E R T   C H A R   *************************
******************************************************************************/

HRESULT CInccUTF8In::ConvertChar(UCHAR tc, int cchSrc)
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

BOOL CInccUTF8In::CleanUp()
{
	return (this->*m_pfnCleanUp)();
}

/******************************************************************************
****************************   C O N V   M A I N   ****************************
******************************************************************************/

BOOL CInccUTF8In::ConvMain(UCHAR tc)
{
	BOOL fDone = TRUE;

    if( ( 0x80 & tc ) == 0 )                    // BIT7 == 0 ASCII
    {
        Output(tc);
        fDone = Output(0);
        m_nBytesUsed = 0 ; 
    }
    else if( (0x40 & tc) == 0 )                 // BIT6 == 0 a trail byte
    {
        if( m_nByteFollow )                
        {
            if (m_fSurrogatesPairs)
            {
                m_nByteFollow--;
                m_tcSurrogateUnicode <<= 6;             // Make room for trail byte
                m_tcSurrogateUnicode |= ( 0x3F & tc );  // LOWER_6BIT add trail byte value

                if( m_nByteFollow == 0)                 // End of sequence, advance output ptr
                {
                    m_tcUnicode = (WCHAR)(((m_tcSurrogateUnicode - 0x10000) >> 10) + HIGHT_SURROGATE_START);
                    tc = (UCHAR)m_tcUnicode ;
                    if ( fDone = Output(tc) )
                    {
                        tc = (UCHAR) ( m_tcUnicode >> 8 ) ; 
                        fDone = Output(tc);
                    }
                    m_tcUnicode = (WCHAR)((m_tcSurrogateUnicode - 0x10000)%0x400 + LOW_SURROGATE_START);
                    tc = (UCHAR)m_tcUnicode ;
                    if ( fDone = Output(tc) )
                    {
                        tc = (UCHAR) ( m_tcUnicode >> 8 ) ; 
                        fDone = Output(tc);
                    }
                    m_fSurrogatesPairs = 0;
                    m_nBytesUsed = 0 ; 
                }	
                else
                    m_nBytesUsed++ ; 
            }
            else
            {
                m_nByteFollow--;
                m_tcUnicode <<= 6;                  // make room for trail byte
                m_tcUnicode |= ( 0x3F & tc );       // LOWER_6BIT add trail byte value

                if( m_nByteFollow == 0)             // end of sequence, advance output ptr
                {
                    tc = (UCHAR)m_tcUnicode ;
                    if ( fDone = Output(tc) )
                    {
                        tc = (UCHAR) ( m_tcUnicode >> 8 ) ; 
                        fDone = Output(tc);
                    }
                    m_nBytesUsed = 0 ; 
                }	
                else
                    m_nBytesUsed++ ; 
            }
        }
        else                                    // error - ignor and rest
        {
            m_nBytesUsed = 0 ; 
            m_nByteFollow = 0 ;
        }
    }
    else                                        // a lead byte
    {
        if( m_nByteFollow > 0 )                 // error, previous sequence not finished
        {
            m_nByteFollow = 0;
            Output(' ');
            fDone = Output(0);
            m_nBytesUsed = 0 ; 
        }
        else                                    // calculate # bytes to follow
        {
            while( (0x80 & tc) != 0)            // BIT7 until first 0 encountered from left to right
            {
                tc <<= 1;
                m_nByteFollow++;
            }

            if (m_nByteFollow == 4)
            {
                m_fSurrogatesPairs = TRUE;
                m_tcSurrogateUnicode = tc >> m_nByteFollow;

            }
            else
            {
                m_tcUnicode = ( tc >> m_nByteFollow ) ;
                m_nBytesUsed = 1 ;               // # bytes used
            }
            m_nByteFollow--;                     // # bytes to follow
        }
    }

	return fDone;
}

/******************************************************************************
************************   C L E A N   U P   M A I N   ************************
******************************************************************************/

BOOL CInccUTF8In::CleanUpMain()
{
	return TRUE;
}

int CInccUTF8In::GetUnconvertBytes()
{
    return  m_nBytesUsed < 4 ? m_nBytesUsed : 3 ; 
}

DWORD CInccUTF8In::GetConvertMode()
{
    // UTF8 does not use mode esc sequence
 	return 0 ;
}

void CInccUTF8In::SetConvertMode(DWORD mode)
{
    Reset();    // initialization
    // UTF8 does not use mode esc sequence
 	return ;
}

// ============================================================================
// Internet Character Set Conversion: Output to UTF-8
// ============================================================================

/******************************************************************************
**************************   C O N S T R U C T O R   **************************
******************************************************************************/

CInccUTF8Out::CInccUTF8Out(UINT uCodePage, int nCodeSet) : CINetCodeConverter(uCodePage, nCodeSet)
{
    Reset();    // initialization
    return ;
}

/******************************************************************************
*******************************   R E S E T   *********************************
******************************************************************************/

void CInccUTF8Out::Reset()
{
    m_fDoubleByte = FALSE;
    m_wchSurrogateHigh = 0;
    return ;
}

HRESULT CInccUTF8Out::ConvertChar(UCHAR tc, int cchSrc)
{
    BOOL fDone = TRUE;
    WORD uc ;
    UCHAR UTF8[4] ;

    if (m_fDoubleByte )
    {
        uc = (	(WORD) tc << 8 | m_tcLeadByte  ) ;

        if (uc >= HIGHT_SURROGATE_START && uc <= HIGHT_SURROGATE_END && cchSrc >= sizeof(WCHAR))
        {
            if (m_wchSurrogateHigh)
            {
                UTF8[0] = 0xe0 | ( m_wchSurrogateHigh >> 12 );              // 4 bits in first byte
                UTF8[1] = 0x80 | ( ( m_wchSurrogateHigh >> 6 ) & 0x3f );    // 6 bits in second
                UTF8[2] = 0x80 | ( 0x3f & m_wchSurrogateHigh);              // 6 bits in third
                Output(UTF8[0]);
                Output(UTF8[1]);
                fDone = Output(UTF8[2]);
            }
            m_wchSurrogateHigh = uc;
            m_fDoubleByte = FALSE ;
            goto CONVERT_DONE;
        }

        if (m_wchSurrogateHigh)
        {
            if (uc >= LOW_SURROGATE_START && uc <= LOW_SURROGATE_END)       // We find a surrogate pairs
            {

                DWORD dwSurrogateChar = ((m_wchSurrogateHigh-0xD800) << 10) + uc - 0xDC00 + 0x10000;
                UTF8[0] = 0xF0 | (unsigned char)( dwSurrogateChar >> 18 );                 // 3 bits in first byte
                UTF8[1] = 0x80 | (unsigned char)( ( dwSurrogateChar >> 12 ) & 0x3f );      // 6 bits in second
                UTF8[2] = 0x80 | (unsigned char)( ( dwSurrogateChar >> 6 ) & 0x3f );       // 6 bits in third
                UTF8[3] = 0x80 | (unsigned char)( 0x3f & dwSurrogateChar);                 // 6 bits in forth
                Output(UTF8[0]);
                Output(UTF8[1]);
                Output(UTF8[2]);
                fDone = Output(UTF8[3]);                
                m_fDoubleByte = FALSE ;
                m_wchSurrogateHigh = 0;
                goto CONVERT_DONE;
            }
            else                                                            // Not a surrogate pairs, error
            {
                UTF8[0] = 0xe0 | ( m_wchSurrogateHigh >> 12 );              // 4 bits in first byte
                UTF8[1] = 0x80 | ( ( m_wchSurrogateHigh >> 6 ) & 0x3f );    // 6 bits in second
                UTF8[2] = 0x80 | ( 0x3f & m_wchSurrogateHigh);              // 6 bits in third
                Output(UTF8[0]);
                Output(UTF8[1]);
                fDone = Output(UTF8[2]);
                m_wchSurrogateHigh = 0;
            }
        }


        if( ( uc & 0xff80 ) == 0 ) // ASCII
        {
            UTF8[0] = (UCHAR) uc;
            fDone = Output(UTF8[0]);
        }
        else if( ( uc & 0xf800 ) == 0 ) 			// UTF8_2_MAX 2-byte sequence if < 07ff (11 bits)
        {
            UTF8[0] = 0xC0 | (uc >> 6);             // 5 bits in first byte
            UTF8[1] = 0x80 | ( 0x3f & uc);       // 6 bits in second
            Output(UTF8[0]);
            fDone = Output(UTF8[1]);
        }
        else                                             // 3-byte sequence
        {
            UTF8[0] = 0xe0 | ( uc >> 12 );                // 4 bits in first byte
            UTF8[1] = 0x80 | ( ( uc >> 6 ) & 0x3f );      // 6 bits in second
            UTF8[2] = 0x80 | ( 0x3f & uc);                // 6 bits in third
            Output(UTF8[0]);
            Output(UTF8[1]);
            fDone = Output(UTF8[2]);
        }
        m_fDoubleByte = FALSE ;
    }
    else
    {
        m_tcLeadByte = tc ;
        m_fDoubleByte = TRUE ;
    }

CONVERT_DONE:
    if (fDone)
        return S_OK;
    else
        return E_FAIL;
}

/******************************************************************************
*****************************   C L E A N   U P   *****************************
******************************************************************************/

BOOL CInccUTF8Out::CleanUp()
{
	BOOL fDone = TRUE;

	return fDone;
}

int CInccUTF8Out::GetUnconvertBytes()
{
    return  m_fDoubleByte ? 1 : 0 ;
}

DWORD CInccUTF8Out::GetConvertMode()
{
    // UTF8 does not use mode esc sequence
 	return 0 ;
}

void CInccUTF8Out::SetConvertMode(DWORD mode)
{
    Reset();    // initialization
    // UTF8 does not use mode esc sequence
 	return ;
}

