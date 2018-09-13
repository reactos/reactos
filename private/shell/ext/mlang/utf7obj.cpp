// ============================================================================
// Internet Character Set Conversion: Input from UTF-7
// ============================================================================

#include "private.h"
#include "fechrcnv.h"
#include "utf7obj.h"

//+-----------------------------------------------------------------------
//
//  Function:   IsBase64
//
//  Synopsis:   We use the following table to quickly determine if we have
//              a valid base64 character.
//
//------------------------------------------------------------------------

static UCHAR g_aBase64[256] =
{
  /*            0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   A,   B,   C,   D,   E,   F, */

  /* 00-0f */ 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  /* 10-1f */ 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  /* 20-2f */ 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  62, 255, 255, 255,  63,
  /* 30-3f */  52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 255, 255, 255, 255, 255, 255,
  /* 40-4f */ 255,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
  /* 50-5f */  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25, 255, 255, 255, 255, 255,
  /* 60-6f */ 255,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,
  /* 70-7f */  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51, 255, 255, 255, 255, 255,
  /* 80-8f */  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  /* 90-9f */ 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  /* a0-af */ 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  /* b0-bf */ 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  /* c0-cf */ 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  /* d0-df */ 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  /* e0-ef */ 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  /* f0-ff */ 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
};


// Direct encoded ASCII table
static UCHAR g_aDirectChar[128] =
{
  /*            0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   A,   B,   C,   D,   E,   F, */

  /* 00-0f */ 255, 255, 255, 255, 255, 255, 255, 255, 255,  72,  73, 255, 255,  74, 255, 255,
  /* 10-1f */ 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  /* 20-2f */  71, 255, 255, 255, 255, 255, 255,  62,  63,  64, 255, 255,  65,  66,  67,  68,
  /* 30-3f */  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  69, 255, 255, 255, 255,  70,
  /* 40-4f */ 255,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
  /* 50-5f */  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25, 255, 255, 255, 255, 255,
  /* 60-6f */ 255,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,
  /* 70-7f */  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51, 255, 255, 255, 255, 255,
};

// Base64 byte value table
static UCHAR g_aInvBase64[] = { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=" };

static inline BOOL
IsBase64(UCHAR t )
{
    return g_aBase64[t] < 64;
}

/******************************************************************************
**************************   C O N S T R U C T O R   **************************
******************************************************************************/

CInccUTF7In::CInccUTF7In(UINT uCodePage, int nCodeSet) : CINetCodeConverter(uCodePage, nCodeSet)
{
    Reset();    // initialization
    return ;
}

/******************************************************************************
*******************************   R E S E T   *********************************
******************************************************************************/

void CInccUTF7In::Reset()
{
    m_pfnConv = ConvMain;
    m_pfnCleanUp = CleanUpMain;
    m_fUTF7Mode = FALSE ;
    m_nBitCount = 0 ;
    m_tcUnicode = 0 ;
    m_nOutCount = 0 ;
    return ;
}


/******************************************************************************
*************************   C O N V E R T   C H A R   *************************
******************************************************************************/

HRESULT CInccUTF7In::ConvertChar(UCHAR tc, int cchSrc)
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

BOOL CInccUTF7In::CleanUp()
{
    return (this->*m_pfnCleanUp)();
}

/******************************************************************************
****************************   C O N V   M A I N   ****************************
******************************************************************************/

BOOL CInccUTF7In::ConvMain(UCHAR tc)
{
    BOOL fDone = TRUE;

    // are we in UTF-7 mode ?
    if (m_fUTF7Mode )
    {
        if ( IsBase64(tc) )
        {
            UCHAR t64, outc ;
            LONG tcUnicode ;

            // save the Base64 value and update bit count
            t64 = g_aBase64[tc] ;
            m_tcUnicode = m_tcUnicode << 6 | t64 ;
            m_nBitCount += 6 ;

            // see if we accumulate enough bits
            if ( m_nBitCount >= 16 )
            {
                // get higher 16 bits data from buffer
                tcUnicode = m_tcUnicode >> ( m_nBitCount - 16 ) ;
                // output one Unicode char
                outc = (UCHAR) tcUnicode ;
                Output( outc );
                outc = (UCHAR) ( tcUnicode >> 8 ) ;
                fDone = Output( outc );

                // update output char count
                m_nOutCount ++ ;
                m_nBitCount -= 16 ;
            }
        }
        // not a Base64 char, reset UTF-7 mode
        else
        {
            // special case +- decodes to +
            if ( tc == '-' && m_nOutCount == 0 && m_nBitCount == 0 )
            {
                Output('+');
                fDone=Output(0);
            }
            // absorb shiht-out char '-', otherwise output char
            else if ( tc != '-')
            {
                Output(tc);
                fDone=Output(0);
            }
            // reset variables and UTF7Mode
            m_fUTF7Mode = FALSE ;
            m_nBitCount = 0 ;
            m_tcUnicode = 0 ;
            m_nOutCount = 0 ;
        }
    }
    // is it a UTF-7 shift-in char ?
    else if ( tc == '+' )
    {
        m_fUTF7Mode = TRUE ;
        m_nBitCount = 0 ;
        m_tcUnicode = 0 ;
        m_nOutCount = 0 ;
    }
    else
    // convert ASCII directly to Unicode if it is not in UFT-7 mode
    {
        Output(tc);
        fDone = Output(0);
    }

    return fDone;
}

/******************************************************************************
************************   C L E A N   U P   M A I N   ************************
******************************************************************************/

BOOL CInccUTF7In::CleanUpMain()
{
    return TRUE;
}

int CInccUTF7In::GetUnconvertBytes()
{
    return  0 ;
}

DWORD CInccUTF7In::GetConvertMode()
{
    DWORD dwMode ;

    if ( m_fUTF7Mode )
    {
        dwMode = ( m_tcUnicode & 0xffff ) | ( m_nBitCount << 16 ) ;
        if ( dwMode == 0 )
            dwMode = 1L ; // it is ok, since bitcount is 0
    }
    else
        dwMode = 0 ;

    return dwMode;
}

void CInccUTF7In::SetConvertMode(DWORD mode)
{
    Reset();    // initialization
    if (mode)
    {
        m_fUTF7Mode = TRUE ;
        m_tcUnicode = ( mode & 0x7fff );
        m_nBitCount = ( mode >> 16 ) & 0xffff ;
    }
    else
        m_fUTF7Mode = FALSE ;
}

// ============================================================================
// Internet Character Set Conversion: Output to UTF-7
// ============================================================================

/******************************************************************************
**************************   C O N S T R U C T O R   **************************
******************************************************************************/

CInccUTF7Out::CInccUTF7Out(UINT uCodePage, int nCodeSet) : CINetCodeConverter(uCodePage, nCodeSet)

{
    Reset();    // initialization
    return ;
}

/******************************************************************************
*******************************   R E S E T   *********************************
******************************************************************************/

void CInccUTF7Out::Reset()
{
    m_fDoubleByte = FALSE;
    m_fUTF7Mode = FALSE ;
    m_nBitCount = 0 ;
    m_tcUnicode = 0 ;
    return;
}

HRESULT CInccUTF7Out::ConvertChar(UCHAR tc, int cchSrc)
{
    BOOL fDone = TRUE;
    WORD uc ;

    // 2nd byte of Unicode
    if (m_fDoubleByte )
    {
        BOOL bNeedShift ;

        // compose the 16 bits char
        uc = ( (WORD) tc << 8 | m_tcFirstByte  ) ;

        // check whether the char can be direct encoded ?
        bNeedShift = uc > 0x7f ? TRUE : g_aDirectChar[(UCHAR)uc] == 255 ;

        if ( bNeedShift && m_fUTF7Mode == FALSE)
        {
            // output Shift-in char to change to UTF-7 Mode
            fDone = Output('+');

            // handle special case '+-'
            if ( uc == '+' ) // single byte "+"
            {
                fDone=Output('-');
            }
            else
                m_fUTF7Mode = TRUE ;
        }

        if (m_fUTF7Mode)
        {
            LONG tcUnicode ;
            UCHAR t64 ;
            int pad_bits ;

            // either write the char to the bit buffer 
            // or pad bit buffer out to a full base64 char
            if (bNeedShift)
            {
                m_tcUnicode = m_tcUnicode << 16 | uc ;
                m_nBitCount += 16 ;
            }
            // pad bit buffer out to a full base64 char
            else if (m_nBitCount % 6 )  
            {
                pad_bits = 6 - (m_nBitCount % 6 ) ;
                // get to next 6 multiple, pad these bits with 0
                m_tcUnicode = m_tcUnicode << pad_bits ;
                m_nBitCount += pad_bits ;
            }

            // flush out as many full base64 char as possible
            while ( m_nBitCount >= 6 && fDone )
            {
                tcUnicode = ( m_tcUnicode >> ( m_nBitCount - 6 ) );
                t64 = (UCHAR) ( tcUnicode & 0x3f ) ;
                fDone = Output(g_aInvBase64[t64]);
                m_nBitCount -= 6 ;
            }

            if (!bNeedShift)
            {
                // output Shift-out char
                fDone = Output('-');

                m_fUTF7Mode = FALSE ;
                m_nBitCount = 0 ;
                m_tcUnicode = 0 ;
            }
        }

        // the character can be directly encoded as ASCII
        if (!bNeedShift)
        {
            fDone = Output(m_tcFirstByte);
        }

        m_fDoubleByte = FALSE ;
    }
    // 1st byte of Unicode
    else
    {
        m_tcFirstByte = tc ;
        m_fDoubleByte = TRUE ;
    }
    
    if (fDone)
        return S_OK;
    else
        return E_FAIL;
}

/******************************************************************************
*****************************   C L E A N   U P   *****************************
******************************************************************************/

BOOL CInccUTF7Out::CleanUp()
{
    BOOL fDone = TRUE;

    if (m_fUTF7Mode)
    {
        UCHAR t64 ;
        LONG tcUnicode ;
        int pad_bits ;

        // pad bit buffer out to a full base64 char
        if (m_nBitCount % 6 )  
        {
            pad_bits = 6 - (m_nBitCount % 6 ) ;
            // get to next 6 multiple, pad these bits with 0
            m_tcUnicode = m_tcUnicode << pad_bits ;
            m_nBitCount += pad_bits ;
        }

        // flush out as many full base64 char as possible
        while ( m_nBitCount >= 6 && fDone )
        {
            tcUnicode = ( m_tcUnicode >> ( m_nBitCount - 6 ) );
            t64 = (UCHAR) ( tcUnicode & 0x3f ) ;
            fDone = Output(g_aInvBase64[t64]);
            m_nBitCount -= 6 ;
        }

        {
            // output Shift-out char
            fDone = Output('-');

            m_fUTF7Mode = FALSE ;
            m_nBitCount = 0 ;
            m_tcUnicode = 0 ;
        }
    }
    return fDone;
}

int CInccUTF7Out::GetUnconvertBytes()
{
    return  m_fDoubleByte ? 1 : 0 ;
}

DWORD CInccUTF7Out::GetConvertMode()
{
    return 0 ;
}

void CInccUTF7Out::SetConvertMode(DWORD mode)
{
    Reset();    // initialization
    return ;
}
