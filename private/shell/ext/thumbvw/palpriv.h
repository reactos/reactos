#ifndef _PALPRIV_H_
#define _PALPRIV_H_

//#include "memdbg.h"

#define CYC_QUANT_FLR(EV,NCYCV) \
    ((UCHAR)(((USHORT)(EV) * ((NCYCV) - 1)) / 255))
#define CYC_QUANT_MID(EV,NCYCV) \
    ((UCHAR)(((USHORT)(EV) * ((NCYCV) - 1) + 127) / 255))

#define CYC_EXPAND(QV,NCYCV) \
    ((UCHAR)(((USHORT)(QV) * 255 + ((NCYCV) - 1) / 2) / ((NCYCV) - 1)))

#define CYC_PACKSCALEDQUANT(QSR,QSG,QSB) ((QSR) + (QSG) + (QSB))
#define CYC_PACKQUANT(QR,QG,QB,NCYCR,NCYCG) \
    ((QR) + (((QG) + ((QB) * (NCYCG))) * (NCYCR)))


#define SHF_QUANT_FLR(EV,NSHFV) ((UCHAR)((EV) >> (NSHFV)))
#define SHF_QUANT_MID(EV,NSHFV) SHF_QUANT_FLR((EV),(NSHFV))

#define BIT_EXPAND(QV,NBITV) CYC_EXPAND((QV),(1 << (NBITV)))

#define BIT_PACKSCALEDQUANT(QSR,QSG,QSB) ((QSR) | (QSG) | (QSB))
#define BIT_PACKQUANT(QR,QG,QB,NBITR,NBITG) \
    ((QR) | (((QG) | ((QB) << (NBITG))) << (NBITR)))

typedef struct
{
    short r, g, b;
} Err;

#define DIV16(V) ((V) >> 4)
#define MUL7(V) (((V) << 3) - (V))
#define MUL5(V) (((V) << 2) + (V))
#define MUL3(V) (((V) << 1) + (V))

#endif // _PALPRIV_H_
