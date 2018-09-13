// --------------------------------------------------------------------------------
// EncodeQP.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
//
// Stolen from Athena's source base by JulianJ, January 21st, 1997
//
// --------------------------------------------------------------------------------

#include "private.h"

//
// Porting macros so the code can be copied without changes
//
#define MIMEOLEAPI
#define AssertSz ASSERT_MSG
#define TrapError(hr) ((EVAL(SUCCEEDED(hr))), (hr))
#define CCHMAX_ENCODE_BUFFER 4096
#define CHECKHR(hrExp) \
    if (FAILED (hrExp)) { \
    ASSERT_MSG(SUCCEEDED(hr), "%x", (hr)); \
    goto exit; \
    } else



// --------------------------------------------------------------------------------
// QP Encoder
// --------------------------------------------------------------------------------
static char rgchHex[] = "0123456789ABCDEF";
#define ENCODEQPBUF 4096
#define CCHMAX_QPLINE 72

// --------------------------------------------------------------------------------
// MimeOleEncodeStreamQP
// --------------------------------------------------------------------------------
MIMEOLEAPI HRESULT MimeOleEncodeStreamQP(IStream *pstmIn, IStream *pstmOut)
{
    // Locals
    HRESULT         hr=S_OK;
    BOOL            fSeenCR=FALSE,
                    fEOLN=FALSE;
    CHAR            szBuffer[CCHMAX_ENCODE_BUFFER];
    ULONG           cbRead=0, 
                    cbCurr=1, 
                    cbAttLast=0,
                    cbBffLast=0, 
                    cbBuffer=0;
    UCHAR           chThis=0, 
                    chPrev=0,
                    buf[ENCODEQPBUF];

    // check params
    if (NULL == pstmIn || NULL == pstmOut)
        return TrapError(E_INVALIDARG);

    // Read from pstmIn and encode into pstmOut
    while (1)
    {    
        // Reads one buffer full from the attachment
        if (cbCurr >= cbRead)
        {
            // Moves buffer from the last white space to front
            cbCurr = 0;
            while (cbAttLast < cbRead)
                buf[cbCurr++] = buf[cbAttLast++];

            // RAID-33342 - Reset cbAttLast - buff[0] is now equal to cbAttLast !!!
            cbAttLast = 0;

            // Read into buf
            CHECKHR(hr = pstmIn->Read(buf+cbCurr, ENCODEQPBUF-cbCurr, &cbRead));

            // No more ?
            if (cbRead == 0)
                break;

            // Adjusts buffer length
            cbRead += cbCurr;
        }

        // Gets the next character
        chThis = buf[cbCurr++]; 

        // Tests for end of line
        if (chThis == '\n' && fSeenCR == TRUE)
            fEOLN = TRUE;
        
        // Tests for an isolated CR
        else if (fSeenCR == TRUE)
        {
            szBuffer[cbBuffer++] = '=';
            szBuffer[cbBuffer++] = '0';
            szBuffer[cbBuffer++] = 'D';
            chPrev = 0xFF;
        }

        // CR has been taken care of
        fSeenCR = FALSE;

        // Tests for trailing white space if end of line
        if (fEOLN == TRUE)
        {
            if (chPrev == ' ' || chPrev == '\t')
            {
                cbBuffer--;
                szBuffer[cbBuffer++] = '=';
                szBuffer[cbBuffer++] = rgchHex[chPrev >>    4];
                szBuffer[cbBuffer++] = rgchHex[chPrev &  0x0F];
                chPrev = 0xFF;
                cbAttLast = cbCurr;
                cbBffLast = cbBuffer;
            }
        }

        // Tests for a must quote character
        else if (((chThis < 32) && (chThis != '\r' && chThis != '\t')) || (chThis > 126 ) || (chThis == '='))
        {
            szBuffer[cbBuffer++] = '=';
            szBuffer[cbBuffer++] = rgchHex[chThis >>    4];
            szBuffer[cbBuffer++] = rgchHex[chThis &  0x0F];
            chPrev = 0xFF;
        }

        // Tests for possible end of line
        else if (chThis == '\r')
        {
            fSeenCR = TRUE;
        }

        // Other characters (includes ' ' and '\t')
        else
        {
            // Stuffs leading '.'
            if (chThis == '.' && cbBuffer == 0)
                szBuffer[cbBuffer++] = '.';

            szBuffer[cbBuffer++] = chThis;
            chPrev = chThis;

            // Tests for white space and saves location
            if (chThis == ' ' || chThis == '\t')
            {
                cbAttLast = cbCurr;
                cbBffLast = cbBuffer;
            }
        }

        // Tests for line break
        if (cbBuffer > 72 || fEOLN == TRUE || chThis == '\n')
        {
            // Backtracks to last whitespace
            if (cbBuffer > 72 && cbBffLast > 0)
            {
                // RAID-33342
                AssertSz(cbAttLast <= cbCurr, "Were about to eat some text.");
                cbCurr = cbAttLast;
                cbBuffer = cbBffLast;
            }
            else
            {
                cbAttLast = cbCurr;
                cbBffLast = cbBuffer;
            }

            // Adds soft line break, if necessary
            if (fEOLN == FALSE)
                szBuffer[cbBuffer++] = '=';

            // Ends line and writes to storage
            szBuffer[cbBuffer++] = '\r';
            szBuffer[cbBuffer++] = '\n';

            // Write the buffer
            CHECKHR(hr = pstmOut->Write(szBuffer, cbBuffer, NULL));

            // Resets counters
            fEOLN = FALSE;
            cbBuffer = 0;
            chPrev = 0xFF;
            cbBffLast = 0;
        }
    }

    // Writes last line to storage
    if (cbBuffer > 0)
    {
        // Write the line
        CHECKHR(hr = pstmOut->Write(szBuffer, cbBuffer, NULL));
    }

exit:
    // Done
    return hr;
}

