//====================================================================
// Debugging memory problems.
//====================================================================

#include "shellprv.h"
#pragma hdrstop

#undef LocalAlloc
#undef LocalReAlloc
#undef LocalFree

#define CBALLOCEXTRA        (sizeof(LPARAM)+sizeof(UINT))
#define HMEM2PTR(hMem,i)    (((BYTE*)hMem)+i)

void _StoreSigniture(HLOCAL hMem, UINT uByte, LPARAM lParam)
{
    if (hMem)
    {
        SIZE_T uSize = LocalSize(hMem);
        ASSERT(uSize>=uByte+CBALLOCEXTRA);
        *(UNALIGNED UINT *)HMEM2PTR(hMem, uSize-sizeof(UINT)) = uByte;
        *(UNALIGNED LPARAM *)HMEM2PTR(hMem, uByte) = lParam;
        _DebugMsg( DM_ALLOC, TEXT("_StoreSig: %x ==> uByte = 0x%x, lParam = 0x%x"), hMem, uByte, lParam );
    }
}

UINT _ValidateLocalMem(HLOCAL hMem, LPARAM lParam, LPCTSTR pszText)
{
    UINT uByte = 0;
    if (hMem)
    {
        SIZE_T uSize = LocalSize(hMem);
        if (uSize)
        {
            LPARAM lParamStored;
            uByte = *(UNALIGNED UINT *)HMEM2PTR(hMem, uSize-sizeof(UINT));
            AssertMsg(uByte+CBALLOCEXTRA <= uSize,
                      TEXT("cm ASSERT! Bogus uByte %x (%x, %x) (%s for %x)"),
                      uByte, uByte+CBALLOCEXTRA, uSize, pszText, hMem);
            lParamStored = *(UNALIGNED LPARAM *)HMEM2PTR(hMem, uByte);
            AssertMsg( lParamStored==lParam,
                       TEXT("cm ASSERT! Bad Signiture %x!=%x (%s for %x)"),
                       lParamStored, lParam, pszText, hMem
                      );
        }
        else
        {
            AssertMsg( uSize!=0,
                       TEXT("cm ASSERT! LocalSize is zero (%s for %x)"),
                       pszText, hMem
                      );
        }
    }
    return uByte;
}

HLOCAL WINAPI DebugLocalAlloc(UINT uFlags, UINT uBytes)
{
    HLOCAL hMem;
    _DebugMsg(DM_ALLOC,TEXT("DbgLocalAlloc( size=0x%x )..."),uBytes);
    hMem = LocalAlloc(uFlags, (uBytes+CBALLOCEXTRA));
    _StoreSigniture(hMem, uBytes, (LPARAM)hMem);
    _DebugMsg(DM_ALLOC,TEXT("DbgLocalAlloc( size=0x%x ) returning %x, size=0x%x"),uBytes,hMem,(uBytes+CBALLOCEXTRA));
    return hMem;
}

HLOCAL WINAPI DebugLocalReAlloc(HLOCAL hMem, UINT uBytes, UINT uFlags)
{
    HLOCAL hMemSave = hMem;
    _DebugMsg(DM_ALLOC,TEXT("DbgLocalReAlloc( hMem=%x, size=0x%x )..."),hMem,uBytes);
    _ValidateLocalMem(hMem, (LPARAM)hMem, TEXT("LocalReAlloc"));
    hMem = LocalReAlloc(hMemSave, uBytes+CBALLOCEXTRA, uFlags);
    _StoreSigniture(hMem, uBytes, (LPARAM)hMem);
    _DebugMsg(DM_ALLOC,TEXT("DbgLocalReAlloc( hMem=%x, size=0x%x ) returning %x, size=0x%x"),hMemSave,uBytes,hMem,(uBytes+CBALLOCEXTRA));
    return hMem;
}

HLOCAL WINAPI DebugLocalFree( HLOCAL hMem )
{
    UINT uBytes;
    _DebugMsg(DM_ALLOC,TEXT("DbgLocalFree( hMem=%x )..."),hMem);
    uBytes = _ValidateLocalMem(hMem, (LPARAM)hMem, TEXT("LocalFree"));
    if (uBytes)
    {
        _StoreSigniture(hMem, uBytes, (LPARAM)0xDEADDEAD);
        _fmemset(hMem, 0xE5, uBytes);
    }
    return LocalFree(hMem);
}


