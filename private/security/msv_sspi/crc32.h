/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    crc32.h

Abstract:

    CRC-32 alogorithm prototypes and constants

Author:

    MikeSw

Revision History:

    ChandanS  25-Jul-96      Stolen from net\svcdlls\ntlmssp\client\crc32.h
--*/



//////////////////////////////////////////////////////////////
//
// Function prototypes for CRC-32
//
//////////////////////////////////////////////////////////////


#ifdef __cplusplus
extern "C"
{
#endif

void
Crc32(  unsigned long crc,
        unsigned long cbBuffer,
        void * pvBuffer,
        unsigned long * pNewCrc);

#ifdef __cplusplus
}
#endif
