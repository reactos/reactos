#ifndef _delayimp_h
#define _delayimp_h

#ifdef __GNUC__
/* Hack, for bug in ld.  Will be removed soon.  */
#define __ImageBase _image_base__
#endif

#define DELAYLOAD_VERSION 0x200

typedef DWORD RVA;
typedef IMAGE_THUNK_DATA *PImgThunkData;
typedef const IMAGE_THUNK_DATA *PCImgThunkData;

enum DLAttr
{
	dlattrRva = 0x1,
};

/* Notification codes */
enum
{
	dliStartProcessing,
	dliNotePreLoadLibrary,
	dliNotePreGetProcAddress,
	dliFailLoadLib,
	dliFailGetProc,
	dliNoteEndProcessing,
};

typedef struct ImgDelayDescr
{
	DWORD grAttrs;
	RVA rvaDLLName;
	RVA rvaHmod;
	RVA rvaIAT;
	RVA rvaINT;
	RVA rvaBoundIAT;
	RVA rvaUnloadIAT;
	DWORD dwTimeStamp; 
} ImgDelayDescr, *PImgDelayDescr;
typedef const ImgDelayDescr *PCImgDelayDescr;

typedef struct DelayLoadProc
{
	BOOL fImportByName;
	union
	{
		LPCSTR szProcName;
		DWORD dwOrdinal;
	};
} DelayLoadProc;

typedef struct DelayLoadInfo
{
	DWORD cb;
	PCImgDelayDescr pidd;
	FARPROC *ppfn;
	LPCSTR szDll;
	DelayLoadProc dlp;
	HMODULE hmodCur;
	FARPROC pfnCur;
	DWORD dwLastError;
} DelayLoadInfo, *PDelayLoadInfo;

typedef FARPROC (WINAPI *PfnDliHook)(unsigned, PDelayLoadInfo);

FORCEINLINE
unsigned
IndexFromPImgThunkData(PCImgThunkData pData, PCImgThunkData pBase)
{
	return pData - pBase;
}

extern const IMAGE_DOS_HEADER __ImageBase;

FORCEINLINE
PVOID
PFromRva(RVA rva)
{
	return (PVOID)(((ULONG_PTR)(rva)) + ((ULONG_PTR)&__ImageBase));
}


extern PfnDliHook __pfnDliNotifyHook2;
extern PfnDliHook __pfnDliFailureHook2;

#endif /* not _delayimp_h */
