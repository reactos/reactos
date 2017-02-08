
#ifdef _M_ARM

#else

#include <asm.inc>

.code

MACRO(DEFINE_ALIAS, alias, orig)
EXTERN &orig:PROC
ALIAS <&alias> = <&orig>
ENDM

#ifdef _M_IX86
PUBLIC ?NonDelegatedRelease@CBaseUnknown@@UAGKXZ
?NonDelegatedRelease@CBaseUnknown@@UAGKXZ:
    jmp _IIndirectedUnknown_fnRelease@4

DEFINE_ALIAS ??0CBaseUnknown@@QAE@ABU_GUID@@PAUIUnknown@@@Z, @CBaseUnknown_ConstructorWithGUID@16
DEFINE_ALIAS ??0CBaseUnknown@@QAE@PAUIUnknown@@@Z, @CBaseUnknown_Constructor@12
DEFINE_ALIAS ??1CBaseUnknown@@UAE@XZ, @CBaseUnknown_DefaultDestructor@8
DEFINE_ALIAS ??_FCBaseUnknown@@QAEXXZ, @CBaseUnknown_Destructor@12
DEFINE_ALIAS ?IndirectedAddRef@CBaseUnknown@@UAGKXZ, _IIndirectedUnknown_fnAddRef@4
DEFINE_ALIAS ?IndirectedQueryInterface@CBaseUnknown@@UAGJABU_GUID@@PAPAX@Z, _IIndirectedUnknown_fnQueryInterface@12
DEFINE_ALIAS ?IndirectedRelease@CBaseUnknown@@UAGKXZ, _IIndirectedUnknown_fnRelease@4
DEFINE_ALIAS ?NonDelegatedAddRef@CBaseUnknown@@UAGKXZ, _INonDelegatedUnknown_fnAddRef@4
DEFINE_ALIAS ?NonDelegatedQueryInterface@CBaseUnknown@@UAGJABU_GUID@@PAPAX@Z, _INonDelegatedUnknown_fnQueryInterface@12
#else
EXTERN IIndirectedUnknown_fnRelease:PROC
PUBLIC ?NonDelegatedRelease@CBaseUnknown@@UAGKXZ
?NonDelegatedRelease@CBaseUnknown@@UAGKXZ:
    jmp IIndirectedUnknown_fnRelease

DEFINE_ALIAS ??0CBaseUnknown@@QAE@ABU_GUID@@PAUIUnknown@@@Z, CBaseUnknown_ConstructorWithGUID
DEFINE_ALIAS ??0CBaseUnknown@@QAE@PAUIUnknown@@@Z, CBaseUnknown_Constructor
DEFINE_ALIAS ??1CBaseUnknown@@UAE@XZ, CBaseUnknown_DefaultDestructor
DEFINE_ALIAS ??_FCBaseUnknown@@QAEXXZ, CBaseUnknown_Destructor
DEFINE_ALIAS ?IndirectedAddRef@CBaseUnknown@@UAGKXZ, IIndirectedUnknown_fnAddRef
DEFINE_ALIAS ?IndirectedQueryInterface@CBaseUnknown@@UAGJABU_GUID@@PAPAX@Z, IIndirectedUnknown_fnQueryInterface
DEFINE_ALIAS ?IndirectedRelease@CBaseUnknown@@UAGKXZ, IIndirectedUnknown_fnRelease
DEFINE_ALIAS ?NonDelegatedAddRef@CBaseUnknown@@UAGKXZ, INonDelegatedUnknown_fnAddRef
DEFINE_ALIAS ?NonDelegatedQueryInterface@CBaseUnknown@@UAGJABU_GUID@@PAPAX@Z, INonDelegatedUnknown_fnQueryInterface
#endif
#endif

    END
