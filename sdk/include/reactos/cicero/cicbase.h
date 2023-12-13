/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero base
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

static inline LPVOID cicMemAllocClear(SIZE_T size)
{
    return LocalAlloc(LMEM_ZEROINIT, size);
}

static inline void cicMemFree(LPVOID ptr)
{
    if (ptr)
        LocalFree(ptr);
}

#ifdef __cplusplus
inline void* __cdecl operator new(size_t size) noexcept
{
    return cicMemAllocClear(size);
}

inline void __cdecl operator delete(void* ptr) noexcept
{
    cicMemFree(ptr);
}

inline void __cdecl operator delete(void* ptr, size_t size) noexcept
{
    cicMemFree(ptr);
}
#endif // __cplusplus

// FIXME: Use msutb.dll and header
static inline void ClosePopupTipbar(void)
{
}

// FIXME: Use msutb.dll and header
static inline void GetPopupTipbar(HWND hwnd, BOOL fWinLogon)
{
}
