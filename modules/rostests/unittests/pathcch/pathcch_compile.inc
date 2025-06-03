/*
 * PROJECT:     ReactOS PathCch Library - Unit-tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     C/C++ compilation tests
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 *              Copyright 2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#ifdef __cplusplus
extern "C"
void test_CPP_PathCch(void)
#else
void test_C_PathCch(void)
#endif
{
#ifdef __cplusplus
    LPWSTR psz = NULL;
#endif
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
#ifdef __cplusplus
    PathCchFindExtension(NULL, 0, &psz); // Won't compile in C; is OK in C++
#endif
    PathCchFindExtension(NULL, 0, &pcsz);
    PathCchIsRoot(NULL);
    PathCchRemoveBackslash(NULL, 0);
    PathCchRemoveBackslashEx(NULL, 0, NULL, NULL);
    PathCchRemoveExtension(NULL, 0);
    PathCchRemoveFileSpec(NULL, 0);
    PathCchRenameExtension(NULL, 0, NULL);
#ifdef __cplusplus
    PathCchSkipRoot(psz, &psz); // Won't compile in C; is OK in C++
#endif
    PathCchSkipRoot(pcsz, &pcsz);
    PathCchStripPrefix(NULL, 0);
    PathCchStripToRoot(NULL, 0);
#ifdef __cplusplus
    PathIsUNCEx(psz, &psz); // Won't compile in C; is OK in C++
#endif
    PathIsUNCEx(pcsz, &pcsz);
}

/* EOF */
