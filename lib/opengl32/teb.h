// teb.h

#ifdef _MSC_VER

typedef struct _TEB
{
	char blah[0x7C4];
	PVOID glDispatchTable[0xA3];        /* 7C4h */
	PVOID glReserved1[0xA3];            /* A50h */
	PVOID glReserved2;                  /* BDCh */
	PVOID glSectionInfo;                /* BE0h */
	PVOID glSection;                    /* BE4h */
	PVOID glTable;                      /* BE8h */
	PVOID glCurrentRC;                  /* BECh */
	PVOID glContext;                    /* BF0h */
} TEB, *PTEB;

#pragma warning ( disable : 4035 )
static inline PTEB __declspec(naked) NtCurrentTeb(void)
{
	//struct _TEB * pTeb;
	__asm mov eax, fs:0x18
	//__asm mov pTeb, eax
	//return pTeb;
};
#pragma warning ( default : 4035 )

#else/*_MSC_VER*/

#include <ntos/types.h>
#include <napi/teb.h>

#endif/*_MSC_VER*/
