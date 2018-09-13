#ifndef __dataobj_h
#define __dataobj_h

#define DSCF_SHELLIDLIST        0
#define DSCF_SHELLDESCRIPTORS   1
#define DSCF_DSOBJECTNAMES      2
#define DSCF_SHELLIDLOFFSETS    3
#define DSCF_DISPSPECOPTIONS    4
#define DSCF_MAX                5

extern CLIPFORMAT g_clipboardFormats[DSCF_MAX];

#define g_cfShellIDList         g_clipboardFormats[DSCF_SHELLIDLIST]
#define g_cfShellDescriptors    g_clipboardFormats[DSCF_SHELLDESCRIPTORS]
#define g_cfDsObjectNames       g_clipboardFormats[DSCF_DSOBJECTNAMES]
#define g_cfShellIDLOffsets     g_clipboardFormats[DSCF_SHELLIDLOFFSETS]
#define g_cfDsDispSpecOptions   g_clipboardFormats[DSCF_DISPSPECOPTIONS]

void RegisterDsClipboardFormats(void);

//
// data object construction
//

typedef struct
{
    HWND hwnd;
    LPCITEMIDLIST pidlRoot;
    INT cbOffset;
    INT cidl;
    LPCITEMIDLIST* aidl;
    DWORD dwProviderAND;
    DWORD dwProviderXOR;
} DSDATAOBJINIT, * LPDSDATAOBJINIT;

STDAPI CDsDataObject_CreateInstance(LPDSDATAOBJINIT pddoi, REFIID riid, void **ppv);


#endif
