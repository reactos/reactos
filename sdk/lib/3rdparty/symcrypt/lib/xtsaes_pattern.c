//
// xtsaes_pattern.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#if 0
#pragma makedep header
#endif

VOID
SYMCRYPT_CALL
SYMCRYPT_XtsAesXxx(
    _In_                                    PCSYMCRYPT_XTS_AES_EXPANDED_KEY pExpandedKey,
                                            SIZE_T                          cbDataUnit,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PCBYTE                          pbTweak,
    _In_reads_( cbData )                    PCBYTE                          pbSrc,
    _Out_writes_( cbData )                  PBYTE                           pbDst,
                                            SIZE_T                          cbData,
                                            BOOLEAN                         bOverflow )
{
    SYMCRYPT_XTS_AES_LOCALSCRATCH_DEFN;
    // SYMCRYPT_ALIGN BYTE localScratch[N_PARALLEL_TWEAKS * SYMCRYPT_AES_BLOCK_SIZE];
    // or
    // /* Defining localScratch as a buffer of __m128is ensures there is required 16B alignment on x86 */
    // __m128i localScratch[ N_PARALLEL_TWEAKS + 16 ];
    // Note that the extra 16 __m128i space is used for internal scratch space for SymCryptXtsAesEncryptDataUnitXmm
    // This allows modified tweak generation to be performed in scalar registers in parallel with AES in Xmm register
    // which reduces register pressure and increases throughput
    PBYTE   tweakBuf    = (PBYTE) &localScratch[0];
    SIZE_T  i, tweakBytes;
    UINT64  tweakLow64  = SYMCRYPT_LOAD_LSBFIRST64(pbTweak);
    UINT64  tweakHigh64 = SYMCRYPT_LOAD_LSBFIRST64(pbTweak+8);
    UINT64  previousTweakLow64;

    SYMCRYPT_ASSERT( cbData % cbDataUnit == 0 );

    while( cbData >= cbDataUnit )
    {
        //
        // We encrypt the tweaks of many data units in parallel for best performance.
        // In the first loop we build the tweaks and decrement cbData.
        // In the second loop we use up all the tweaks, and update the pointers.
        // Both loops are executed the same number of times.
        //
        tweakBytes = 0;
        previousTweakLow64 = tweakLow64;

        do // do-while because we know we are going to go through at least once.
        {
            SYMCRYPT_STORE_LSBFIRST64(&tweakBuf[tweakBytes    ], tweakLow64);
            SYMCRYPT_STORE_LSBFIRST64(&tweakBuf[tweakBytes + 8], tweakHigh64);
            tweakLow64++;
            cbData -= cbDataUnit;
            tweakBytes += SYMCRYPT_AES_BLOCK_SIZE;
        } while( cbData >= cbDataUnit && tweakBytes < SYMCRYPT_AES_BLOCK_SIZE * N_PARALLEL_TWEAKS );

        if( bOverflow && previousTweakLow64 > tweakLow64 )
        {
            // Very rare fix-up of tweaks if tweakLow64 overflowed, and should have incremented tweakHigh64
            // bOverflow=FALSE allows backwards compatibility with old API which wrapped around at 64-bits
            SYMCRYPT_ASSERT( tweakLow64 < N_PARALLEL_TWEAKS );

            // Increment tweakHigh64 and store new value in high half of the previous tweakLow64 tweaks
            tweakHigh64++;
            for( i=0; i<tweakLow64; i++)
            {
                SYMCRYPT_STORE_LSBFIRST64(&tweakBuf[tweakBytes - (16*i) - 8], tweakHigh64);
            }
        }

        SYMCRYPT_AesEcbEncryptXxx( &pExpandedKey->key2, &tweakBuf[0], &tweakBuf[0], tweakBytes );

        i = 0;
        while( i < tweakBytes )
        {
            SYMCRYPT_XTSAESDATAUNIT_INVOKE;
            // SymCryptXtsAesXxcryptDataUnitXxx( &pExpandedKey->key1, &tweakBuf[i], pbSrc, pbDst, cbDataUnit );
            // or
            // SymCryptXtsAesXxcryptDataUnitXxx( &pExpandedKey->key1, &tweakBuf[i], (PBYTE) &localScratch[N_PARALLEL_TWEAKS], pbSrc, pbDst, cbDataUnit );
            // Note that the scratch space being provided to the DataUnit function is an offset into the localScratch buffer

            pbSrc += cbDataUnit;
            pbDst += cbDataUnit;
            i += SYMCRYPT_AES_BLOCK_SIZE;
        }
    }

    SymCryptWipeKnownSize( localScratch, sizeof( localScratch ) );
}
