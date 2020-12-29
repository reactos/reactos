#ifndef _delayimp_h
#define _delayimp_h

#ifdef __GNUC__
/* Hack, for bug in ld.  Will be removed soon.  */
#define __ImageBase __MINGW_LSYMBOL(_image_base__)
#endif

#if defined(__cplusplus)
#define ExternC extern "C"
#else
#define ExternC extern
#endif

#ifndef FACILITY_VISUALCPP
#define FACILITY_VISUALCPP  ((LONG)0x6d)
#endif
#define VcppException(sev,err)  ((sev) | (FACILITY_VISUALCPP<<16) | err)

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


ExternC PfnDliHook __pfnDliNotifyHook2;
ExternC PfnDliHook __pfnDliFailureHook2;

#endif /* not _delayimp_h */
