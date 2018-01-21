#ifndef APPSHIM_APITEST_H
#define APPSHIM_APITEST_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tagHOOKAPI {
    PCSTR LibraryName;
    PCSTR FunctionName;
    PVOID ReplacementFunction;
    PVOID OriginalFunction;
    PVOID Unk1;
    PVOID Unk2;
} HOOKAPI, *PHOOKAPI;

typedef HRESULT (WINAPI* tSDBGETAPPPATCHDIR)(PVOID hsdb, LPWSTR path, DWORD size);
typedef PHOOKAPI (WINAPI* tGETHOOKAPIS)(LPCSTR szCommandLine, LPCWSTR wszShimName, PDWORD pdwHookCount);


/* versionlie.c */
void expect_shim_imp(PHOOKAPI hook, PCSTR library, PCSTR function, PCSTR shim, int* same);
#define expect_shim  (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : expect_shim_imp


BOOL LoadShimDLL(PCWSTR ShimDll, HMODULE* module, tGETHOOKAPIS* ppGetHookAPIs);
tGETHOOKAPIS LoadShimDLL2(PCWSTR ShimDll);



#ifdef __cplusplus
} // extern "C"
#endif

#endif // APPHELP_APITEST_H
