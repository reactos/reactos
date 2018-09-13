#ifndef GETCRD_H
#define GETCRD_H
BOOL EXTERN  GetPS2ColorRenderingDictionary(
    CHANDLE     cp,
    DWORD       Intent, 
    MEMPTR      lpMem, 
    LPDWORD     lpcbSize,
    BOOL        AllowBinary);
SINT SendCRDLMN(MEMPTR lpMem, CSIG Intent, 
    LPSFLOAT whitePoint, LPSFLOAT mediaWP, CSIG pcs);
SINT SendCRDPQR(MEMPTR lpMem, CSIG Intent, LPSFLOAT whitePoint);
SINT SendCRDABC(MEMPTR lpMem, MEMPTR PublicArrayName, CSIG pcs, SINT nInputCh,
    MEMPTR Buff, LPSFLOAT e, CSIG LutTag, BOOL bAllowBinary);
SINT SendCRDBWPoint(MEMPTR lpMem, LPSFLOAT whitePoint);
SINT SendCRDOutputTable(MEMPTR lpMem, MEMPTR PublicArrayName, 
    SINT nOutputCh, CSIG LutTag, BOOL bHost, BOOL bAllowBinary);
#endif
