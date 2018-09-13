/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    chunk.hxx

Abstract:

    Contains a generic chunked transfer implimentation

Author:

    Arthur L Bierer (arthurbi) 03-May-1997

Revision History:

    03-May-1997 arthurbi
        Created

--*/

//
// defines
//

#define BASE_HEX 16

//
// structures/types
//

//
// CHUNK_STATE - enumerated set of states that our chunked-transfer can use.
//



typedef enum {
    CHUNK_STATE_START,
    CHUNK_STATE_SIZE_PARSE, 
    CHUNK_STATE_EXT_PARSE,
    CHUNK_STATE_SIZE_CRLF,
    CHUNK_STATE_DATA_PARSE,
    CHUNK_STATE_DATA_CRLF,
    CHUNK_STATE_ZERO_FOOTER,
    CHUNK_STATE_ZERO_FOOTER_NAME,
    CHUNK_STATE_ZERO_FOOTER_VALUE,
    CHUNK_STATE_ZERO_FOOTER_CRLF,
    CHUNK_STATE_ZERO_FOOTER_FINAL_CRLF,
    CHUNK_STATE_FINISHED
} CHUNK_STATE, * LPCHUNK_STATE;


//
// CHUNK_TOKEN - enumerated set of tokens that can be parsed off of chunk stream
//

typedef enum {
    CHUNK_TOKEN_DIGIT,
    CHUNK_TOKEN_DATA, 
    CHUNK_TOKEN_COLON,
    CHUNK_TOKEN_CR,
    CHUNK_TOKEN_LF,
    CHUNK_TOKEN_INVALID
} CHUNK_TOKEN, * LPCHUNK_TOKEN;



class CHUNK_TRANSFER {

private:

    //
    // _csState - Current State that we are in for parsing chunked encoding
    //

    CHUNK_STATE _csState;


    // 
    // CHUNK_STATE_SIZE_PARSE: below are vars used for this state
    //

    //
    // _dwCalculatedChunkSize - current value used in processing the chunk size
    //   number.
    //

    DWORD _dwCalculatedChunkSize;

    //
    // CHUNK_STATE_SIZE_CRLF, CHUNK_STATE_DATA_CRLF, CHUNK_STATE_ZERO_SIZE_CRLF, 
    //     CHUNK_STATE_ZERO_FOOTER_CRLF: below are vars used to track the CRLF pair
    //     when in the above states.
    //

    //
    // _dwCr & _dwLf - Count of CR & LF bytes
    //

    DWORD _dwCr;
    DWORD _dwLf;

    inline
    VOID
    ClearCrLf(
        VOID
        )
    {
        _dwCr = 0;
        _dwLf = 0;
    }

    inline 
    BOOL
    IsCrLf(
        VOID
        )
    {
        if ( _dwCr == 1 && _dwLf == 1 )
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }


    //
    // CHUNK_STATE_DATA_PARSE: below are vars used to track parsing of this data
    //

    //
    // _dwChunkDataSize - size of current chunk that we are reading data from
    //

    DWORD _dwChunkDataSize;

    //
    // _dwChunkDataOffset - Count of bytes that have already been read from this chunk
    //

    DWORD _dwChunkDataRead;

    //
    // CHUNK_STATE_ZERO_FOOTER: below are vars used to track footer parsing
    // 
    inline
    VOID    
    SetState(
        CHUNK_STATE csState 
        )
    {
        _csState = csState;
    }

    VOID
    ResetSubStateInfo(
        VOID
        )
    {
        _dwCalculatedChunkSize  = 0;
        _dwCr                   = 0;
        _dwLf                   = 0;
        _dwChunkDataSize        = 0;
        _dwChunkDataRead        = 0;
    }




public:

    CHUNK_TRANSFER(
        VOID
        )
    {
        ResetSubStateInfo();

        _csState = CHUNK_STATE_START;
    }


    ~CHUNK_TRANSFER(
        VOID
        )
    {
    }

    DWORD
    ParseChunkInput(
        IN LPSTR lpInputBuffer,
        IN DWORD dwInputBufferSize,
        OUT LPSTR *lplpInputBufferNew,
        OUT LPDWORD lpdwInputBufferNewSize
        );

    inline
    CHUNK_TOKEN
    GetToken(
        IN OUT LPSTR *lplpInputBuffer,
        IN LPSTR     lpEndOfInputBuffer,
        OUT LPDWORD  lpdwValue,
        IN DWORD     dwExpectedTokenSize,
        OUT LPDWORD  lpdwBytesTokenized
        );



    DWORD
    BytesRemaining(
        VOID
        );

    VOID
    Reset(
        VOID
        )
    {
        ResetSubStateInfo();
        _csState = CHUNK_STATE_START;
    }

    BOOL
    IsFinished(
        VOID
        )
    {
        return (( _csState == CHUNK_STATE_FINISHED ) ? TRUE : FALSE );
    }

};
