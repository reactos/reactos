/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    chunk.cxx

Abstract:

    Contains a generic chunked transfer implimentation

Author:

    Arthur L Bierer (arthurbi) 03-May-1997

Revision History:

    03-May-1997 arthurbi
        Created

--*/


#include <wininetp.h>


inline
CHUNK_TOKEN
CHUNK_TRANSFER::GetToken(
    IN OUT LPSTR *lplpInputBuffer,
    IN LPSTR     lpEndOfInputBuffer,
    OUT LPDWORD  lpdwValue,
    IN DWORD     dwExpectedTokenSize,
    OUT LPDWORD  lpdwBytesTokenized
    )

/*++

Routine Description:

    Lexes through a byte stream, seperating data into tokens.  Data is special cased for efficency.


Arguments:

    lplpInputBuffer - Pointer to Pointer of Buffer that should be lexed, on return contains
        an offset where the next character to lex is.

    lpEndofInputBuffer - Pointer to last character to passed in Buffer.

    lpdwValue   - On return, MAY contain numerical conversion of a text number (digit) token

    dwExpectedTokenSize - Expected size of token, in all cases except for data should be 1

    lpdwBytesTokenized - On return, contains size of token


Return Value:

    CHUNK_TOKEN
    Success - The Correct token.

    Failure - CHUNK_TOKEN_INVALID

--*/

{
    BOOL fFirstIteration = TRUE;
    CHUNK_TOKEN ctToken = CHUNK_TOKEN_INVALID;

    DEBUG_ENTER((DBG_HTTP,
                Dword,
                "CHUNK_TRANSFER::GetToken",
                "%x [%x, %.10q], %x, %x, %u, %x",
                lplpInputBuffer,
                *lplpInputBuffer,
                *lplpInputBuffer,
                lpEndOfInputBuffer,
                lpdwValue,
                dwExpectedTokenSize,
                lpdwBytesTokenized
                ));

    *lpdwBytesTokenized = 0;

    while ( *lplpInputBuffer    < lpEndOfInputBuffer
            && *lpdwBytesTokenized < dwExpectedTokenSize )
    {
        //
        // Set Default Token type
        //

        ctToken = CHUNK_TOKEN_DATA;

        //
        // Handle Other, "special" tokens, only if asked for by the parser.
        //

        if ( dwExpectedTokenSize == 1 )
        {

            if ( **lplpInputBuffer == '\r' )
            {
                ctToken = CHUNK_TOKEN_CR;
                goto quit;
            }

            if ( **lplpInputBuffer == '\n' )
            {
                ctToken = CHUNK_TOKEN_LF;
                goto quit;
            }

            if ( **lplpInputBuffer == ':' )
            {
                ctToken = CHUNK_TOKEN_COLON;
                goto quit;
            }

            if ( **lplpInputBuffer >= '0' && **lplpInputBuffer <= '9' )
            {
                *lpdwValue = (DWORD) (**lplpInputBuffer - '0');
                ctToken = CHUNK_TOKEN_DIGIT;
                goto quit;
            }


            if ( **lplpInputBuffer >= 'A' && **lplpInputBuffer <= 'F' )
            {
                *lpdwValue = (DWORD) (**lplpInputBuffer - 'A') + 10;
                ctToken = CHUNK_TOKEN_DIGIT;
                goto quit;
            }

            if ( **lplpInputBuffer >= 'a' && **lplpInputBuffer <= 'f' )
            {
                *lpdwValue = (DWORD) (**lplpInputBuffer - 'a') + 10;
                ctToken = CHUNK_TOKEN_DIGIT;
                goto quit;
            }
        }

        fFirstIteration = FALSE;
        (*lplpInputBuffer)++;
        (*lpdwBytesTokenized)++;
    }


quit:

    if (ctToken != CHUNK_TOKEN_DATA && ctToken != CHUNK_TOKEN_INVALID)
    {
        if ( !fFirstIteration)
        {
            ctToken = CHUNK_TOKEN_DATA;
        }
        else
        {
            //
            // Advance past this token, since we've only
            //  lexed one token
            //

            (*lplpInputBuffer)++;
            (*lpdwBytesTokenized)++;
        }
    }

    DEBUG_PRINT(HTTP,
                INFO,
                ("GetToken: %q, expected=%u, actual=%u\n",
                InternetMapChunkToken(ctToken),
                dwExpectedTokenSize,
                *lpdwBytesTokenized
                ));

    DEBUG_LEAVE((DWORD)ctToken);

    return ctToken;
}



DWORD
CHUNK_TRANSFER::ParseChunkInput(
    LPSTR lpInputBuffer,
    DWORD dwInputBufferSize,
    LPSTR *lplpInputBufferNew,
    LPDWORD lpdwInputBufferNewSize
    )

/*++

Routine Description:

    Parses a buffer of an assumed chunked encoding byte stream.  Seperates out data from header information.


Arguments:

    lpInputBuffer - Pointer to buffer containing a stream of bytes to parse

    dwInputBufferSize - size of byte lpInputBuffer

    lplpInputBufferNew - Offset into passed in lpInputBuffer, (not used, yet)

    lpdwInputBufferNewSize - On Return, cotains the size of lpInputBuffer ( data is compressed )

Return Value:

    DWORD
    Success - ERROR_SUCCESS

    Failure -

--*/


{
    CHUNK_TOKEN ctToken;
    DWORD dwValue;
    DWORD dwExpectedTokenSize = 1;
    DWORD dwActualTokenSize   = 0;
    LPSTR lpszEndOfInputBuffer = (LPSTR) (lpInputBuffer + dwInputBufferSize);
    LPSTR lpszStartOfFirstDataBuffer = NULL;
    DWORD dwFirstDataBufferSize = 0;
    LPSTR lpszStartOfNextDataBuffer = NULL;
    DWORD error = ERROR_SUCCESS;

    DEBUG_ENTER((DBG_HTTP,
                Dword,
                "CHUNK_TRANSFER::ParseChunkInput",
                "%x [%.10q], %u, %x, %x",
                lpInputBuffer,
                lpInputBuffer,
                dwInputBufferSize,
                lplpInputBufferNew,
                lpdwInputBufferNewSize
                ));

    *lplpInputBufferNew = lpInputBuffer;

    while ( *lplpInputBufferNew < lpszEndOfInputBuffer)
    {

        //
        // Calculate the max size of the token can be, only
        //  relevant for data, since all other tokens are assumed
        //  to be size of 1.
        //

        if (_csState == CHUNK_STATE_DATA_PARSE)
        {
            dwExpectedTokenSize = (_dwChunkDataSize - _dwChunkDataRead);
            lpszStartOfNextDataBuffer = *lplpInputBufferNew;
        }
        else
        {
            dwExpectedTokenSize = 1;
        }

        DEBUG_PRINT(HTTP,
                    INFO,
                    ("ParseChunk: %q, %u/%u\n",
                    InternetMapChunkState(_csState),
                    _dwChunkDataRead,
                    _dwChunkDataSize
                    ));


        //
        //  Lex through the byte stream looking for our next token.
        //

        ctToken = GetToken( lplpInputBufferNew,
                            lpszEndOfInputBuffer,
                            &dwValue,
                            dwExpectedTokenSize,
                            &dwActualTokenSize
                            );

        if ( ctToken == CHUNK_TOKEN_INVALID )
        {
            //
            // Need more data to parse...
            //

            error = ERROR_SUCCESS;
            goto quit;
        }

        //
        // Based on our current state, evalulate the token,
        //  and figure out what to do next.
        //

        switch ( _csState )
        {
            case CHUNK_STATE_START:

                ResetSubStateInfo();

                if ( ctToken != CHUNK_TOKEN_DIGIT )
                {
                    DEBUG_PRINT(HTTP,
                                INFO,
                                ("-->CHUNK err: Got %q, while looking for NOT %q\n",
                                InternetMapChunkToken(ctToken),
                                InternetMapChunkToken(CHUNK_TOKEN_DIGIT)
                                ));

                    error = ERROR_INTERNET_INTERNAL_ERROR;
                    goto quit;
                }

                SetState(CHUNK_STATE_SIZE_PARSE);
                // otherwise fall through

            case CHUNK_STATE_SIZE_PARSE:

                switch ( ctToken )
                {
                    case CHUNK_TOKEN_DIGIT:
                        _dwCalculatedChunkSize *= BASE_HEX;
                        _dwCalculatedChunkSize += dwValue;
                        break;

                    case CHUNK_TOKEN_CR:
                        _dwCr++;
                        // fall through

                    case CHUNK_TOKEN_DATA:
                    case CHUNK_TOKEN_COLON:


                        _dwChunkDataSize = _dwCalculatedChunkSize;
                        _dwChunkDataRead = 0;


                        DEBUG_PRINT(HTTP,
                                    INFO,
                                    ("ChunkParse-GotChunksize: size=%u, %x\n",
                                    _dwCalculatedChunkSize,
                                    _dwCalculatedChunkSize
                                    ));

                        if (ctToken == CHUNK_TOKEN_CR)
                        {
                            SetState(CHUNK_STATE_SIZE_CRLF);
                        }
                        else
                        {
                            SetState(CHUNK_STATE_EXT_PARSE);
                        }

                        break;

                    default:
                        // ERROR
                        error = ERROR_INTERNET_INTERNAL_ERROR;
                        goto quit;
                }

                break;

            case CHUNK_STATE_EXT_PARSE:

                switch ( ctToken )
                {
                    case CHUNK_TOKEN_CR:
                        _dwCr++;
                        SetState(CHUNK_STATE_SIZE_CRLF);
                        break;

                    case CHUNK_TOKEN_DIGIT:
                    case CHUNK_TOKEN_DATA:
                    case CHUNK_TOKEN_COLON:

                        break;

                    default:
                        // ERROR
                        error = ERROR_INTERNET_INTERNAL_ERROR;
                        goto quit;
                }

                break;

            case CHUNK_STATE_SIZE_CRLF:

                switch ( ctToken )
                {
                    case CHUNK_TOKEN_LF:

                        _dwLf++;

                        if ( IsCrLf() )
                        {
                            ClearCrLf();
                            if ( _dwCalculatedChunkSize == 0 )
                            {
                                SetState(CHUNK_STATE_ZERO_FOOTER);
                            }
                            else
                            {
                                SetState(CHUNK_STATE_DATA_PARSE);
                            }
                        }
                        else
                        {
                            DEBUG_PRINT(HTTP,
                                        INFO,
                                        ("-->CHUNK err: Got %q, But CRLF not matched, CR=%u, LF=%u\n",
                                        InternetMapChunkToken(ctToken),
                                        _dwCr,
                                        _dwLf
                                        ));

                            error = ERROR_INTERNET_INTERNAL_ERROR;
                            goto quit;
                        }

                        break;

                    default:

                        DEBUG_PRINT(HTTP,
                                    INFO,
                                    ("-->CHUNK err: Got %q, while looking for %q\n",
                                    InternetMapChunkToken(ctToken),
                                    InternetMapChunkToken(CHUNK_TOKEN_LF)
                                    ));

                        error = ERROR_INTERNET_INTERNAL_ERROR;
                        goto quit;
                }

                break;

            case CHUNK_STATE_DATA_PARSE:

                switch ( ctToken )
                {
                    case CHUNK_TOKEN_CR:
                    case CHUNK_TOKEN_LF:
                    case CHUNK_TOKEN_DATA:
                    case CHUNK_TOKEN_DIGIT:
                    case CHUNK_TOKEN_COLON:

                        //
                        // If this is the first piece of data we receive
                        //  than save it off, so we know the start of the
                        //  buffer we are returning as data.
                        //

                        if ( lpszStartOfFirstDataBuffer == NULL )
                        {
                            lpszStartOfFirstDataBuffer = lpInputBuffer;
                        }

                        //
                        // If this is not the first block of data in the passed in buffer,
                        //  we must move the block of data OVER any Chunked-tranfer header
                        //  information.
                        //

                        if ( (lpszStartOfFirstDataBuffer+dwFirstDataBufferSize) != lpszStartOfNextDataBuffer )
                        {
                            MoveMemory((LPVOID) (lpszStartOfFirstDataBuffer+dwFirstDataBufferSize), // Dest
                                       (LPVOID) lpszStartOfNextDataBuffer,                          // Source
                                       dwActualTokenSize                                            // size
                                       );
                        }

                        //
                        // Update the size of data we've parsed out, and
                        //  check to see if we've completely received all data.
                        //

                        dwFirstDataBufferSize += dwActualTokenSize;
                        _dwChunkDataRead += dwActualTokenSize;

                        if ( _dwChunkDataRead == _dwChunkDataSize )
                        {
                            SetState(CHUNK_STATE_DATA_CRLF);
                        }

                        INET_ASSERT(_dwChunkDataRead <= _dwChunkDataSize);

                        break;

                    default:

                        DEBUG_PRINT(HTTP,
                                    INFO,
                                    ("-->CHUNK err: Got %q, while looking for CR,LF,DATA,DIGIT,COLON\n",
                                    InternetMapChunkToken(ctToken)
                                    ));

                        error = ERROR_INTERNET_INTERNAL_ERROR;
                        goto quit;

                }

                break;

            case CHUNK_STATE_DATA_CRLF:

                switch (ctToken)
                {
                    case CHUNK_TOKEN_CR:
                        _dwCr++;
                        break;

                    case CHUNK_TOKEN_LF:
                        _dwLf++;

                        if ( IsCrLf() )
                        {
                            ClearCrLf();
                            SetState(CHUNK_STATE_START);
                        }
                        else
                        {
                            DEBUG_PRINT(HTTP,
                                        INFO,
                                        ("-->CHUNK err: Got %q, BUT CRLF not matched, CR=%u, LF=%u\n",
                                        InternetMapChunkToken(ctToken),
                                        _dwCr,
                                        _dwLf
                                        ));

                            error = ERROR_INTERNET_INTERNAL_ERROR;
                            goto quit;
                        }

                        break;

                    default:
                        DEBUG_PRINT(HTTP,
                                    INFO,
                                    ("-->CHUNK err: Got %q, while looking for CR or LF (CHUNK DATA SIZE INCORRECT??)\n",
                                    InternetMapChunkToken(ctToken)
                                    ));

                        error = ERROR_INTERNET_INTERNAL_ERROR;
                        goto quit;
                }

                break;

            case CHUNK_STATE_ZERO_FOOTER:

                switch (ctToken)
                {
                    case CHUNK_TOKEN_CR:

                        _dwCr++;
                        SetState(CHUNK_STATE_ZERO_FOOTER_FINAL_CRLF);
                        break;

                    case CHUNK_TOKEN_DATA:
                    case CHUNK_TOKEN_DIGIT:
                    case CHUNK_TOKEN_COLON:

                        SetState(CHUNK_STATE_ZERO_FOOTER_NAME);
                        break;

                    default:

                        DEBUG_PRINT(HTTP,
                                    INFO,
                                    ("-->CHUNK err: Got %q, while looking for DATA or CR\n",
                                    InternetMapChunkToken(ctToken)
                                    ));

                        error = ERROR_INTERNET_INTERNAL_ERROR;
                        goto quit;
                }

                break;

            case CHUNK_STATE_ZERO_FOOTER_NAME:

                switch (ctToken)
                {
                    case CHUNK_TOKEN_DATA:
                    case CHUNK_TOKEN_DIGIT:
                        break;

                    case CHUNK_TOKEN_COLON:
                        SetState(CHUNK_STATE_ZERO_FOOTER_VALUE);
                        break;

                    default:

                        DEBUG_PRINT(HTTP,
                                    INFO,
                                    ("-->CHUNK err: Got %q, while looking for DATA, DIGIT, COLON\n",
                                    InternetMapChunkToken(ctToken)
                                    ));

                        error = ERROR_INTERNET_INTERNAL_ERROR;
                        goto quit;
                }

                break;

            case CHUNK_STATE_ZERO_FOOTER_VALUE:
                switch (ctToken)
                {
                    case CHUNK_TOKEN_DATA:
                    case CHUNK_TOKEN_DIGIT:
                        break;

                    case CHUNK_TOKEN_CR:
                        _dwCr++;
                        SetState(CHUNK_STATE_ZERO_FOOTER_CRLF);
                        break;

                    default:

                        DEBUG_PRINT(HTTP,
                                    INFO,
                                    ("-->CHUNK err: Got %q, while looking for DATA, DIGIT, CR\n",
                                    InternetMapChunkToken(ctToken)
                                    ));

                        error = ERROR_INTERNET_INTERNAL_ERROR;
                        goto quit;
                }

                break;

            case CHUNK_STATE_ZERO_FOOTER_CRLF:
            case CHUNK_STATE_ZERO_FOOTER_FINAL_CRLF:

                switch ( ctToken )
                {
                    case CHUNK_TOKEN_LF:

                        _dwLf++;

                        if ( IsCrLf() )
                        {
                            ClearCrLf();

                            if ( _csState == CHUNK_STATE_ZERO_FOOTER_CRLF)
                            {
                                SetState(CHUNK_STATE_ZERO_FOOTER);
                            }
                            else
                            {
                                INET_ASSERT( _csState == CHUNK_STATE_ZERO_FOOTER_FINAL_CRLF);
                                //Done?
                                SetState(CHUNK_STATE_FINISHED);

                                DEBUG_PRINT(HTTP,
                                            INFO,
                                            ("EOF chunk\n"
                                            ));

                                error = ERROR_SUCCESS;
                                goto quit;
                            }
                        }
                        else
                        {
                            DEBUG_PRINT(HTTP,
                                        INFO,
                                        ("-->CHUNK err: Got %q, But CRLF not matched, CR=%u, LF=%u\n",
                                        InternetMapChunkToken(ctToken),
                                        _dwCr,
                                        _dwLf
                                        ));

                            error = ERROR_INTERNET_INTERNAL_ERROR;
                            goto quit;
                        }

                        break;

                    default:

                        DEBUG_PRINT(HTTP,
                                    INFO,
                                    ("-->CHUNK err: Got %q, while looking for LF\n",
                                    InternetMapChunkToken(ctToken)
                                    ));

                        error = ERROR_INTERNET_INTERNAL_ERROR;
                        goto quit;
                }

                break;

            case CHUNK_STATE_FINISHED:
                INET_ASSERT(FALSE);
                error = ERROR_SUCCESS;
                goto quit;

            default:
                INET_ASSERT(FALSE);
                error = ERROR_INTERNET_INTERNAL_ERROR;
                goto quit;

        }
    }

quit:

    if ( error == ERROR_SUCCESS)
    {
        if ( dwFirstDataBufferSize > 0 )
        {
            INET_ASSERT(lpInputBuffer == lpszStartOfFirstDataBuffer);
        }

        *lplpInputBufferNew     = lpInputBuffer;
        *lpdwInputBufferNewSize = dwFirstDataBufferSize;
    }


    DEBUG_LEAVE(error);

    return error;
}