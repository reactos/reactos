/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    pdb.h

Abstract:

    This header file contains typedefs and prototypes
    necessary for accessing pdb files thru the msvc pdb dll.

Revision History:

--*/

#ifdef __cplusplus
extern "C" {
#endif

__inline
unsigned char *
DataSymNameStart(
    DATASYM32 *dataSym
    )
{
    switch (dataSym->rectyp) {
        case S_LDATA32_16t:
        case S_GDATA32_16t:
        case S_PUB32_16t:
            return(&((DATASYM32_16t *)dataSym)->name[1]);

        case S_LDATA32:
        case S_GDATA32:
        case S_PUB32:
        default:
            return(&((DATASYM32 *)dataSym)->name[1]);
    }
}


__inline
unsigned char
DataSymNameLength(
    DATASYM32 *dataSym
    )
{
    switch (dataSym->rectyp) {
        case S_LDATA32_16t:
        case S_GDATA32_16t:
        case S_PUB32_16t:
            return(((DATASYM32_16t *)dataSym)->name[0]);

        case S_LDATA32:
        case S_GDATA32:
        case S_PUB32:
        default:
            return(((DATASYM32 *)dataSym)->name[0]);
    }
}


__inline
unsigned short
DataSymSeg(
    DATASYM32 *dataSym
    )
{
    switch (dataSym->rectyp) {
        case S_LDATA32_16t:
        case S_GDATA32_16t:
        case S_PUB32_16t:
            return(((DATASYM32_16t *)dataSym)->seg);

        case S_LDATA32:
        case S_GDATA32:
        case S_PUB32:
        default:
            return(((DATASYM32 *)dataSym)->seg);
    }
}


__inline
unsigned long
DataSymOffset(
    DATASYM32 *dataSym
    )
{
    switch (dataSym->rectyp) {
        case S_LDATA32_16t:
        case S_GDATA32_16t:
        case S_PUB32_16t:
            return(((DATASYM32_16t *)dataSym)->off);

        case S_LDATA32:
        case S_GDATA32:
        case S_PUB32:
        default:
            return(((DATASYM32 *)dataSym)->off);
    }
}

#ifdef __cplusplus
}
#endif
