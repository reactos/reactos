/*
 * propstg.h - Property storage ADT
 */

#ifndef _PROPSTG_H_
#define _PROPSTG_H_

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_HANDLE(HPROPSTG);

HRESULT
WINAPI
PropStg_Create(
    OUT HPROPSTG * phstg,
    IN  DWORD      dwFlags);        // PSTGF_*

// Flags for PropStg_Create
#define PSTGF_DEFAULT       0x00000000

HRESULT
WINAPI
PropStg_Destroy(
    IN HPROPSTG hstg);

HRESULT
WINAPI
PropStg_ReadMultiple(
    IN HPROPSTG      hstg,
    IN ULONG         cpspec,
    IN const PROPSPEC * rgpropspec,
    IN PROPVARIANT * rgpropvar);

HRESULT
WINAPI
PropStg_WriteMultiple(
    IN HPROPSTG      hstg,
    IN ULONG         cpspec,
    IN const PROPSPEC * rgpropspec,
    IN const PROPVARIANT * rgpropvar,
    IN PROPID        propidFirst);     OPTIONAL


typedef HRESULT (CALLBACK *PFNPROPVARMASSAGE)(PROPID propid, const PROPVARIANT * ppropvar, LPARAM lParam);

HRESULT
WINAPI
PropStg_WriteMultipleEx(
    IN HPROPSTG      hstg,
    IN ULONG         cpspec,
    IN const PROPSPEC * rgpropspec,
    IN const PROPVARIANT * rgpropvar,
    IN PROPID        propidFirst,      OPTIONAL
    IN PFNPROPVARMASSAGE pfn,          OPTIONAL
    IN LPARAM        lParam);          OPTIONAL

HRESULT
WINAPI
PropStg_DeleteMultiple(
    IN HPROPSTG      hstg,
    IN ULONG         cpspec,
    IN const PROPSPEC * rgpropspec);


HRESULT
WINAPI
PropStg_DirtyMultiple(
    IN HPROPSTG    hstg,
    IN ULONG       cpspec,
    IN const PROPSPEC * rgpropspec,
    IN BOOL        bDirty);

HRESULT
WINAPI
PropStg_DirtyAll(
    IN HPROPSTG    hstg,
    IN BOOL        bDirty);

HRESULT
WINAPI
PropStg_IsDirty(
    IN HPROPSTG hstg);


typedef HRESULT (CALLBACK *PFNPROPSTGENUM)(PROPID propid, PROPVARIANT * ppropvar, LPARAM lParam);

HRESULT
WINAPI
PropStg_Enum(
    IN HPROPSTG       hstg,
    IN DWORD          dwFlags,      // One of PSTGEF_ 
    IN PFNPROPSTGENUM pfnEnum,
    IN LPARAM         lParam);      OPTIONAL

// Filter flags for PropStg_Enum
#define PSTGEF_DEFAULT      0x00000000
#define PSTGEF_DIRTY        0x00000001

#ifdef DEBUG
HRESULT
WINAPI
PropStg_Dump(
    IN HPROPSTG       hstg,
    IN DWORD          dwFlags);     // One of PSTGDF_ 
#endif


#ifdef DEBUG

BOOL
IsValidPPROPSPEC(
    PROPSPEC * ppropspec);

BOOL
IsValidHPROPSTG(
    HPROPSTG hstg);

#endif


#ifdef __cplusplus
};
#endif

#endif  // _PROPSTG_H_
