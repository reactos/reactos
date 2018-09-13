// defclsf.c

// ref count for this DLL, use this in your object Create/Release functions
extern UINT g_cRefDll;
STDAPI CreateClassFactory(HRESULT (*pfnCreate)(IUnknown *, REFIID, void **), REFIID riidInst, void **ppv);


//
// Helper macro for implemting OLE classes in C
//
#define _IOffset(class, itf)         ((UINT)&(((class *)0)->itf))
#define IToClass(class, itf, pitf)   ((class  *)(((LPSTR)pitf)-_IOffset(class, itf)))


