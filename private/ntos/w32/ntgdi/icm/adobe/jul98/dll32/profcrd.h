#ifndef PROFCRD_H
#define PROFCRD_H

#define MAXCHANNELS    4
#define PREVIEWCRDGRID 16

typedef enum { DATA_lut=0, DATA_matrix } DATATYPE;

typedef struct tagHOSTCLUT {
    USHORT         size;
    DATATYPE       dataType;
    DWORD          colorSpace;
    DWORD          pcs;
    DWORD          intent;
    float          whitePoint[3];
    float          mediaWP[3];
    unsigned char  inputChan;
    unsigned char  outputChan;
    unsigned char  clutPoints;
    unsigned char  lutBits;
    float          e[9];
    USHORT         inputEnt;
    USHORT         outputEnt;
    MEMPTR         inputArray[MAXCHANNELS];
    MEMPTR         outputArray[MAXCHANNELS];
    MEMPTR         clut;
} HOSTCLUT;
typedef HOSTCLUT __huge *LPHOSTCLUT;

BOOL EXTERN
GetPS2PreviewColorRenderingDictionary (
                    CHANDLE cpDev,
                    CHANDLE cpTarget,
                    DWORD Intent,
                    MEMPTR lpMem,
                    LPDWORD lpcbSize,
                    BOOL AllowBinary);
#endif

