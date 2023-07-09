#ifndef ENUM_H_
#define ENUM_H_

typedef struct {
    HDSA hdsa;
    DWORD grfInvalid;
} SFEnumCacheData;

#define ENUMGRFFLAGS (SHCONTF_INCLUDEHIDDEN | SHCONTF_FOLDERS | SHCONTF_NONFOLDERS)
#define EnumCanCache(grfFlags) (!(grfFlags & ~(ENUMGRFFLAGS)))

STDAPI SFEnumCache_Create(IEnumIDList* penum, DWORD grfFlags, SFEnumCacheData* pcache, IShellFolder* psf, IEnumIDList **ppenumUnknown);
STDAPI_(void) SFEnumCache_Invalidate(SFEnumCacheData *pcache, DWORD grfFlags);
STDAPI_(void) SFEnumCache_Terminate(SFEnumCacheData *pcache);

#endif // ENUM_H_