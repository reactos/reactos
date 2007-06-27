
#ifndef _DV_H_
#define _DV_H_

typedef struct Tag_DVAudInfo
{
    BYTE    bAudStyle[2];
    BYTE    bAudQu[2];
    BYTE    bNumAudPin;
    WORD    wAvgSamplesPerPinPerFrm[2];
    WORD    wBlkMode;
    WORD    wDIFMode;
    WORD    wBlkDiv;
    
} DVAudInfo;

#define DV_AUDIOMODE                0x00000f00
#define DV_AUDIOQU                  0x07000000
#define DV_AUDIOSMP                 0x38000000
#define DV_CAP_AUD16Bits            0x00
#define DV_CAP_AUD12Bits            0x01
#define DV_HD                       0x01
#define DV_DVSD_NTSC_FRAMESIZE      120000
#define DV_DVSD_PAL_FRAMESIZE       144000
#define DV_NTSC                     0
#define DV_NTSCPAL                  0x00200000
#define DV_PAL                      1
#define DV_SMCHN                    0x0000e000
#define DV_SD                       0x00
#define DV_SL                       0x02
#define DV_STYPE                    0x001f0000

#define SIZE_DVINFO                 0x20

#endif
