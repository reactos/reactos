#ifndef SUPPORT_H
#define SUPPORT_H 
#include "windows.h"
#include "icc.h"
#include "icc_i386.h"

#ifdef  __cplusplus
extern "C"
{
#endif

#define  TYPE_CIEBASEDDEF   1
#define  TYPE_CIEBASEDDEFG  2
#define  TempBfSize         128 
#define  CIEXYZRange        1.99997
#define  MAX_LINELENG       240

/*------------------------------------------------------------------------*/
//               Foreword to the functions format.
//  All functions return TRUE if successful
//  and FALSE if not. Exact reason for the FALSE return can be determined
//  by calling GetLastCPError() function.
//  This allows us to use the "C" standard left-to-right evaluation order
//  for the logical expression and the requirement that logical AND
//  operation is performed until FALSE condition is met. That way we can use
//  expression like:
//        if( LoadCP() && ValidateCP() &&
//            DoesCPTagExist() && GetCPTagSig() &&
//            ValidateCPElement())
//        {   // Profile element is OK
//            Process CP Element........
//        }else
//        {     //Something is wrong
//            Err=GetLastCPError();
//        }

// Implementation-specific representation of the handle to the ColorProfile

typedef     MEMPTR      CHANDLE,        // For the convinience and speed
            __far       *LPCHANDLE;     // let's use the pointer to the
                                        // memory block of the profile
                                        // as the profile handle

BOOL    EXTERN LoadCP(LPCSTR filename, HGLOBAL FAR *phMem, LPCHANDLE lphCP);
BOOL    EXTERN LoadCP32(LPCSTR filename, HGLOBAL *phMem, LPCHANDLE lpCP);

BOOL    EXTERN FreeCP(HGLOBAL hMem);

/* Checks if the profile has all required fields  for
    this specific type of the color profile             */
BOOL    EXTERN ValidateCP(CHANDLE hCP);

BOOL    EXTERN DoesCPTagExist(CHANDLE hCP, CSIG CPTag);
BOOL    EXTERN GetCPTagIndex(CHANDLE hCP, CSIG CPTag, LPSINT lpIndex);

BOOL    EXTERN GetCPElementCount(CHANDLE hCP, LPSINT lpIndex);
BOOL    EXTERN ValidateCPElement(CHANDLE hCP, SINT Index);
BOOL    EXTERN GetCPTagSig(CHANDLE hCP, SINT Index, LPCSIG lpCPTag);
BOOL    EXTERN GetCPElementType(CHANDLE hCP, SINT Index, LPCSIG lpCSig);

BOOL    EXTERN GetCPElementSize(CHANDLE hCP, SINT Index, LPSINT lpSize);
BOOL    EXTERN GetCPElementDataSize(CHANDLE hCP, SINT Index, LPSINT lpSize);
BOOL    EXTERN GetCPElement(CHANDLE hCP, SINT Index, MEMPTR lpData, SINT Size);
BOOL    EXTERN GetCPElementData(CHANDLE hCP, SINT Index, MEMPTR lpData, SINT Size);
BOOL    EXTERN GetCPElementDataType(CHANDLE CP, SINT Index, long far *lpDataType);

// Functions that get all information from the Color Profile Header
BOOL    EXTERN GetCPSize(CHANDLE hCP, LPSINT lpSize);
BOOL    EXTERN GetCPCMMType(CHANDLE hCP, LPCSIG lpType);
BOOL    EXTERN GetCPVersion(CHANDLE hCP, LPSINT lpVers);
BOOL    EXTERN GetCPClass(CHANDLE hCP, LPCSIG lpClass);
BOOL    EXTERN GetCPDevSpace(CHANDLE hCP, LPCSIG lpDevSpace);
BOOL    EXTERN GetCPConnSpace(CHANDLE hCP, LPCSIG lpConnSpace);
BOOL    EXTERN GetCPTarget(CHANDLE hCP, LPCSIG lpTarget);
BOOL    EXTERN GetCPManufacturer(CHANDLE hCP, LPCSIG lpManuf);
BOOL    EXTERN GetCPModel(CHANDLE hCP, LPCSIG lpModel);
BOOL    EXTERN GetCPFlags(CHANDLE hCP, LPSINT lpFlags);
BOOL    EXTERN GetCPAttributes(CHANDLE hCP, LPATTRIB lpAttributes);
BOOL    EXTERN GetCPWhitePoint(CHANDLE CP,  LPSFLOAT lpWP);
BOOL    EXTERN GetCPMediaWhitePoint(CHANDLE CP,  LPSFLOAT lpMediaWP);
BOOL    EXTERN GetCPRenderIntent(CHANDLE CP, LPSINT lpIntent);
BOOL    EXTERN GetPS2ColorRenderingIntent(CHANDLE cp, DWORD Intent,
               MEMPTR lpMem, LPDWORD Size);

SINT    EXTERN GetCPLastCPError();
BOOL    EXTERN SetCPLastCPError(SINT cpError);
BOOL    EXTERN SetCPLastError(SINT LastError);
#ifndef ICMDLL
BOOL    EXTERN ValidColorSpace(LPPDEVICE lppd, LPICMINFO lpICMI);
#endif

BOOL    EXTERN GetCPWhitePoint(CHANDLE CP,  LPSFLOAT lpWP);
SINT    WriteInt(MEMPTR lpMem, SINT Number);
SINT    WriteHex(MEMPTR lpMem, SINT Number);
SINT    WriteObject(MEMPTR lpMem, MEMPTR Obj);
SINT    WriteObjectN(MEMPTR lpMem, MEMPTR Obj, SINT n);
SINT    WriteHexBuffer(MEMPTR lpMem, MEMPTR lpBuff, MEMPTR lpLineStart, DWORD dwBytes);
SINT    WriteFloat(MEMPTR lpMem, double dFloat);
SINT    WriteStringToken(MEMPTR lpMem, BYTE Token, SINT sNum);
SINT    WriteByteString(MEMPTR lpMem, MEMPTR lpBuff, SINT sBytes);
SINT    WriteInt2ByteString(MEMPTR lpMem, MEMPTR lpBuff, SINT sBytes);
SINT    WriteIntStringU2S(MEMPTR lpMem, MEMPTR lpBuff, SINT sNum);
SINT    WriteIntStringU2S_L(MEMPTR lpMem, MEMPTR lpBuff, SINT sNum);
SINT    WriteHNAToken(MEMPTR lpMem, BYTE Token, SINT sNum);
SINT    WriteAscii85(MEMPTR lpDest, unsigned long inword, SINT nBytes);
SINT    ConvertBinaryData2Ascii(MEMPTR lpMem, SINT DataSize, SINT BufSize);
SINT    Convert2Ascii(CHANDLE CP, SINT Index,
		MEMPTR lpData, SINT BufSize, SINT DataSize, BOOL AllowBinary);
#ifdef ICMDLL
SINT    MemCopy(MEMPTR Dest, MEMPTR Source, SINT Length);
#endif
BOOL    EXTERN MemAlloc(SINT Size, HGLOBAL FAR *hMemory, LPMEMPTR lpMH);
BOOL    EXTERN MemFree(HGLOBAL hMem);
DWORD FIXED_2DOT30(float);
DWORD FIXED_16DOT16(float);

#ifdef  __cplusplus
}
#endif

#endif  //  __SUPPORT_H
