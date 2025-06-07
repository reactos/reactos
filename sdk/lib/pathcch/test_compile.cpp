#include <windows.h>
#include <pathcch.h>

/// #pragma comment(lib, "pathcch.lib")

extern "C"
void test_CPP_PathCch(void)
{
    LPWSTR psz = NULL;
    LPCWSTR pcsz = NULL;
    PathAllocCanonicalize(NULL, 0, NULL);
    PathAllocCombine(NULL, NULL, 0, NULL);
    PathCchAddBackslash(NULL, 0);
    PathCchAddBackslashEx(NULL, 0, NULL, NULL);
    PathCchAddExtension(NULL, 0, NULL);
    PathCchAppend(NULL, 0, NULL);
    PathCchAppendEx(NULL, 0, NULL, 0);
    PathCchCanonicalize(NULL, 0, NULL);
    PathCchCanonicalizeEx(NULL, 0, NULL, 0);
    PathCchCombine(NULL, 0, NULL, NULL);
    PathCchCombineEx(NULL, 0, NULL, NULL, 0);
    PathCchFindExtension(NULL, 0, &psz); // Won't compile in C; is OK in C++
    PathCchFindExtension(NULL, 0, &pcsz);
    PathCchIsRoot(NULL);
    PathCchRemoveBackslash(NULL, 0);
    PathCchRemoveBackslashEx(NULL, 0, NULL, NULL);
    PathCchRemoveExtension(NULL, 0);
    PathCchRemoveFileSpec(NULL, 0);
    PathCchRenameExtension(NULL, 0, NULL);
    PathCchSkipRoot(psz, &psz); // Won't compile in C; is OK in C++
    PathCchSkipRoot(pcsz, &pcsz);
    PathCchStripPrefix(NULL, 0);
    PathCchStripToRoot(NULL, 0);
    PathIsUNCEx(psz, &psz);
    PathIsUNCEx(pcsz, &pcsz);
}
