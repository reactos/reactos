
#ifndef _WIATWCMP_H_
#define _WIATWCMP_H_

#define WiaItemTypeTwainCapabilityPassThrough       0x00020000
#define ESC_TWAIN_CAPABILITY                        2001
#define ESC_TWAIN_PRIVATE_SUPPORTED_CAPS            2002

typedef struct _TWAIN_CAPABILITY
{
    LONG  lSize;
    LONG  lMSG;
    LONG  lCapID;
    LONG  lConType;
    LONG  lRC;
    LONG  lCC;
    LONG  lDataSize;
    BYTE  Data[1];
} TWAIN_CAPABILITY,*PTWAIN_CAPABILITY;

#endif


