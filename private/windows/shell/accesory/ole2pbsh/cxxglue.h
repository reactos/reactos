//
// FILE:    cxxglue.h
// DATE:    2/1/94
//
// NOTES:   functions to make our C++ classes accessable to pbrush C code
//

#ifdef __cplusplus
extern "C" BOOL CreatePBClassFactory(HINSTANCE hinst,BOOL fEmbedded);
#else
BOOL CreatePBClassFactory(HINSTANCE hinst,BOOL fEmbedded);
#endif

#ifdef __cplusplus
extern "C" HRESULT ReleasePBClassFactory(void);
#else
HRESULT ReleasePBClassFactory(void);
#endif

