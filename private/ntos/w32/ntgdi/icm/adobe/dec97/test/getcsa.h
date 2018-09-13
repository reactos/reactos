#ifndef GETCSA_H
#define GETCSA_H
BOOL    EXTERN  GetPS2ColorSpaceArray(CHANDLE cp, DWORD InputIntent,
        DWORD InpDrvClrSp, MEMPTR lpBuffer, LPDWORD lpcbSize, BOOL AllowBinary);
BOOL    GetPS2CSA_DEFG_Intent(CHANDLE cp, MEMPTR lpBuffer, LPDWORD lpcbSize,
        DWORD InpDrvClrSp, CSIG Intent, int Type, BOOL AllowBinary);
BOOL    GetPS2CSA_ABC( CHANDLE cp, MEMPTR lpMem, LPDWORD lpcbSize, 
        CSIG InputIntent, DWORD InpDrvClrSp, BOOL AllowBinary);
BOOL    GetPS2CSA_ABC_LAB( CHANDLE cp, MEMPTR lpMem, LPDWORD lpcbSize, 
        CSIG InputIntent, DWORD InpDrvClrSp, BOOL AllowBinary);
SINT    GetPublicArrayName(CHANDLE cp, CSIG IntentSig, MEMPTR PublicArrayName);
SINT    CreateInputArray(MEMPTR lpMem, SINT nInputCh, SINT nInputTable ,
        MEMPTR Intent, CSIG Tag, MEMPTR  Buff, BOOL AllowBinary);
SINT    CreateOutputArray(MEMPTR lpMem, SINT nOutputCh, SINT nOutputTable,
        SINT Offset, MEMPTR Intent, CSIG Tag, MEMPTR  Buff, BOOL AllowBinary);

#endif
