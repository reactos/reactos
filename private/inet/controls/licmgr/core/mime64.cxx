//+----------------------------------------------------------------------------
//  File:       mime64.cxx
//
//  Synopsis:
//
//-----------------------------------------------------------------------------


// Includes -------------------------------------------------------------------
#include <core.hxx>


// Constants ------------------------------------------------------------------
const LARGE_INTEGER LIB_ZERO = { 0, 0 };
const ULONG BUFFER_SIZE      = 256;
const UCHAR INVALID_CHAR     = (UCHAR)-2;
const UCHAR IGNORE_CHAR      = (UCHAR)-1;
const UCHAR CH_TERMINATION   = '=';
const char  achAlpha[]       = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                               "abcdefghijklmnopqrstuvwxyz"
                               "0123456789+/";


// Globals --------------------------------------------------------------------
UCHAR   g_anBinary[256];


// Prototypes -----------------------------------------------------------------
inline ULONG BinaryFromASCII(UCHAR ch)  { return g_anBinary[ch]; }


//+----------------------------------------------------------------------------
//  Function:   ProcessAttachMIME64
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
HRESULT
ProcessAttachMIME64()
{
    UCHAR   ubin;
    UCHAR   ch;
    int     i;

    // BUGBUG: Hard code this table
    for (i=0; i < ARRAY_SIZE(g_anBinary); i++)
    {
        ch = (UCHAR)i;

        switch (ch)
        {
            case ' ' :
            case '\t':
            case '\n':
            case '\r':
                ubin = IGNORE_CHAR;
                break;

            default:
                if ((ch >= 'A') && (ch <= 'Z'))
                    ubin = (UCHAR)(ch - 'A');
                else if ((ch >= 'a') && (ch <= 'z'))
                    ubin = (UCHAR)(26 + (ch - 'a'));
                else if ((ch >= '0') && (ch <= '9'))
                    ubin = (UCHAR)(52 + (ch - '0'));
                else
                    ubin = INVALID_CHAR;
                break;

            case '+':
                ubin = 62;
                break;

            case '/':
                ubin = 63;
                break;
        }

        g_anBinary[i] = ubin;
    }
    return S_OK;
}


//+----------------------------------------------------------------------------
//  Function:   EncodeMIME64
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
HRESULT
EncodeMIME64(
    BYTE *      pbSrc,
    UINT        cbSrc,
    IStream *   pstmDest,
    ULONG *     pcbWritten)
{
    UCHAR   achOut[(2 * sizeof(UCHAR)) + CB_NEWLINE];
    ULONG   ichOut = 0;
    ULONG   cbWritten;
    ULONG   cbTotalWritten;
    ULONG   bAccum = 0;
    ULONG   cShift = 0;
    HRESULT hr = S_OK;

    Assert(pbSrc);
    Assert(pstmDest);

    if (!pcbWritten)
    {
        pcbWritten = &cbTotalWritten;
    }

    // Convert the source string, 6-bits at a time, to ASCII characters
    while (cbSrc)
    {
        bAccum <<= 8;
        cShift += 8;
        bAccum |= *pbSrc++;
        cbSrc--;

        while (cShift >= 6)
        {
            cShift -= 6;
            hr = pstmDest->Write(&achAlpha[(bAccum >> cShift) & 0x3FL], 1, &cbWritten);
            *pcbWritten += cbWritten;
            if (hr)
                goto Cleanup;
        }
    }

    // If there are bits not yet written, pad with zeros and write the resulting character
    if (cShift)
    {
        bAccum <<= 6 - cShift;
        achOut[ichOut++] = achAlpha[(bAccum >> cShift) & 0x3FL];
    }

    // Add a termination character and newline
    achOut[ichOut++] = CH_TERMINATION;
    ::memcpy(achOut+ichOut, SZ_NEWLINE, CB_NEWLINE);
    ichOut += CB_NEWLINE;

    hr = pstmDest->Write(achOut, ichOut, &cbWritten);
    *pcbWritten += cbWritten;
    if (hr)
        goto Cleanup;

Cleanup:
    return hr;
}


//+----------------------------------------------------------------------------
//  Function:   DecodeMIME64
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
HRESULT
DecodeMIME64(
    IStream *   pstmSrc,
    IStream *   pstmDest,
    ULONG *     pcbWritten)
{
    UCHAR   ch;
    UCHAR   achOut[BUFFER_SIZE];
    ULONG   ichOut = 0;
    ULONG   cbRead;
    ULONG   cbWritten;
    ULONG   cbTotalWritten = 0;
    ULONG   bAccum = 0;
    ULONG   cShift = 0;
    ULONG   bValue;
    HRESULT hr;

    if (!pcbWritten)
    {
        pcbWritten = &cbTotalWritten;
    }

    // As long as characters remain, convert them to binary
    // (This loop skips "whitespace" and stops when it encounters an out-of-range value)
    for (;;)
    {
        hr = pstmSrc->Read(&ch, sizeof(ch), &cbRead);
        if (hr)
            goto Cleanup;

        // Stop when no more characters remain
        if (!cbRead)
            break;

        bValue = BinaryFromASCII(ch);

        // Convert known characters back to binary
        if (bValue < 64)
        {
            bAccum <<= 6;
            cShift += 6;
            bAccum |= bValue;

            if (cShift >= 8)
            {
                cShift -= 8;
                achOut[ichOut++] = (UCHAR)((bAccum >> cShift) & 0xFF);

                if (ichOut >= ARRAY_SIZE(achOut))
                {
                    hr = pstmDest->Write(achOut, ichOut, &cbWritten);
                    *pcbWritten += cbWritten;
                    if (hr)
                        goto Cleanup;
                    ichOut = 0;
                }
            }
        }

        // Skip "whitespace"
        else if (bValue == IGNORE_CHAR)
            ;

        // Stop if anything else is encountered
        else
            break;
    }

    // If characters remain to be written, write them now
    if (ichOut)
    {
        hr = pstmDest->Write(achOut, ichOut, &cbWritten);
        *pcbWritten += cbWritten;
        if (hr)
            goto Cleanup;
    }

Cleanup:
    return hr;
}
