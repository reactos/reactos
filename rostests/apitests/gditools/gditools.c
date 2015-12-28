

/* SDK/DDK/NDK Headers. */
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winddi.h>
#include <prntfont.h>

#define NTOS_MODE_USER
#include <ndk/ntndk.h>

/* Public Win32K Headers */
#include <ntgdityp.h>
#include <ntgdi.h>
#include <ntgdihdl.h>

PENTRY
GdiQueryTable(
    VOID)
{
    PTEB pTeb = NtCurrentTeb();
    PPEB pPeb = pTeb->ProcessEnvironmentBlock;
    return pPeb->GdiSharedHandleTable;
}

BOOL
GdiIsHandleValid(
    _In_ HGDIOBJ hobj)
{
    PENTRY pentHmgr = GdiQueryTable();
    USHORT Index = (ULONG_PTR)hobj & 0xFFFF;
    PENTRY pentry = &pentHmgr[Index];

    if ((pentry->einfo.pobj == NULL) ||
        ((LONG_PTR)pentry->einfo.pobj > 0) ||
        (pentry->FullUnique != (USHORT)((ULONG_PTR)hobj >> 16)))
    {
        return FALSE;
    }

    return TRUE;
}

BOOL
GdiIsHandleValidEx(
    _In_ HGDIOBJ hobj,
    _In_ GDILOOBJTYPE ObjectType)
{
    PENTRY pentHmgr = GdiQueryTable();
    USHORT Index = (ULONG_PTR)hobj & 0xFFFF;
    PENTRY pentry = &pentHmgr[Index];

    if ((pentry->einfo.pobj == NULL) ||
        ((LONG_PTR)pentry->einfo.pobj > 0) ||
        (pentry->FullUnique != (USHORT)((ULONG_PTR)hobj >> 16)) ||
        (pentry->Objt != (UCHAR)(ObjectType >> 16)) ||
        (pentry->Flags != (UCHAR)(ObjectType >> 24)))
    {
        return FALSE;
    }

    return TRUE;
}

PVOID
GdiGetHandleUserData(
    _In_ HGDIOBJ hobj)
{
    PENTRY pentHmgr = GdiQueryTable();
    USHORT Index = (ULONG_PTR)hobj;
    PENTRY pentry = &pentHmgr[Index];

    if (!GdiIsHandleValid(hobj))
    {
        return NULL;
    }

    return pentry->pUser;
}

