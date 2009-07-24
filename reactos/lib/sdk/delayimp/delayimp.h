#ifndef _DELAYIMP_H_
#define _DELAYIMP_H_

typedef void *RVA;

typedef IMAGE_THUNK_DATA *PImgThunkData;
typedef const IMAGE_THUNK_DATA *PCImgThunkData;

enum
{
	dlattrRva
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

extern PfnDliHook __pfnDliNotifyHook2;
extern PfnDliHook __pfnDliFailureHook2;

#endif /* not _DELAYIMP_H_ */
