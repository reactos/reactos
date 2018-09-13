//***************************************************************************
//
// Typedefs and Function proto for Forced Configuration Edit (FCE)
// support.
// These function are in FCE.C, which is part of SETUPX.DLL
//
//***************************************************************************

#define NOT_VXD
#include <configmg.h>

#define ULONG_AT(x)     (*(LPULONG)(x))

typedef	WORD			FCERET;
typedef	LPVOID			LPREGLOGCONF;
typedef	ULONG		_far	*LPULONG;

#define	FCE_OK			0x00000000
#define	FCE_OK_IS_ALLOC		0x00000001
#define	FCE_OK_IS_NOT_ALLOC	0x00000002
#define	FCE_OK_IS_IN_CONFLICT	0x00000003
#define	FCE_ERROR		0x00000004
#define	FCE_NO_MORE		0x00000005

#define REGSTR_VAL_FORCEDCONFIG "ForcedConfig"		//

typedef union _RESOURCE_POINTER {
    LPBYTE              pRaw;
    LPDWORD             pDword;
    MEM_RESOURCE FAR    *pMem;
    IO_RESOURCE FAR     *pIo;
    DMA_RESOURCE FAR    *pDma;
    IRQ_RESOURCE FAR    *pIrq;
}   RESOURCE_POINTER;

typedef struct _ASSIGN_RESOURCES_DATA {
    LPDEVICE_INFO lpdi;
    UINT EnabledBits;
    UINT AutomaticBits;
}   ASSIGN_RESOURCES_DATA, FAR* LPASSIGN_RESOURCES_DATA;

FCERET WINAPI
FCEInit(DWORD dwFlags);

#define FCE_FLAGS_USECONFIGMG       0x00000001

FCERET WINAPI
FCEGetResDes(LPREGLOGCONF pLogConf, WORD wResNumber, PRESOURCEID PResType);

FCERET WINAPI
FCEGetFirstValue(DEVNODE dnDevNode, LPREGLOGCONF pLogConf, WORD wResNumber, LPULONG pulValue, LPULONG pulLen);

FCERET WINAPI
FCEGetOtherValue(DEVNODE dnDevNode, LPREGLOGCONF pLogConf, WORD wResNumber, BOOL bNext, LPULONG pulValue, LPULONG pulLen);

FCERET WINAPI
FCEGetValidateValue(DEVNODE dnDevNode, LPREGLOGCONF pLogConf, WORD wResNumber, ULONG ulValue, ULONG ulLen, LPULONG pulValue, LPULONG pulLen);

FCERET WINAPI
FCEWriteThisForcedConfigNow(LPREGLOGCONF pLogConf, HKEY hkey);

FCERET WINAPI
FCEAddResDes(LPREGLOGCONF pLogConf, LPBYTE lpResDes, ULONG ulResDesSize, RESOURCEID ResType);

FCERET WINAPI
FCEDeleteResDes(LPREGLOGCONF pLogConf, WORD wResNumber, LPULONG pulNewSize);

FCERET WINAPI
FCEGetAllocValue(LPREGLOGCONF pLogConf, WORD wResNumber, LPULONG pulValue, LPULONG pulLen);

