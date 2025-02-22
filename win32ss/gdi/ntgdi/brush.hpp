/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS win32 subsystem
 * PURPOSE:           BRUSH class definition
 * PROGRAMER:         Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#pragma once

#include <win32k.h>
#include "baseobj.hpp"

__prefast_operator_new_null

class BRUSH : public BASEOBJECT, protected _BRUSHBODY
{

public:
    _Analysis_mode_(_Analysis_operator_new_null_)

    inline
    void*
    __cdecl
    operator new(
        _In_ size_t cjSize) throw()
    {
        return ExAllocatePoolWithTag(PagedPool, cjSize, GDITAG_HMGR_BRUSH_TYPE);
        //return BASEOBJECT::pvAllocate(GDIObjType_BRUSH_TYPE, cjSize);
    }

    inline
    void
    operator delete(
        void *pvObject)
    {
        /// HACK! better would be to extract the exact object type's tag
        ExFreePool(pvObject);
        //ExFreePoolWithTag(pvObject, GDITAG_HMGR_BRUSH_TYPE);
        //BASEOBJECT::pvFree(GDIObjType_BRUSH_TYPE, cjSize);
    }

    BRUSH(
        _In_ FLONG flAttrs,
        _In_ COLORREF crColor,
        _In_ ULONG iHatch,
        _In_opt_ HBITMAP hbmPattern,
        _In_opt_ PVOID pvClient,
        _In_ GDILOOBJTYPE objtype);

    ~BRUSH(
        VOID);

    static
    VOID
    vDeleteObject(
        _In_ PVOID pvObject);

    BOOL
    bAllocateBrushAttr(
        VOID);

    _Check_return_
    _Ret_opt_bytecount_(sizeof(BRUSH))
    static
    inline
    PBRUSH
    LockForRead(
        _In_ HBRUSH hbr)
    {
        return static_cast<PBRUSH>(
            BASEOBJECT::LockShared(hbr,
                                   GDILoObjType_LO_BRUSH_TYPE,
                                   BASEOBJECT::OWNER::PUBLIC));
    }

    _Check_return_
    _Ret_opt_bytecount_(sizeof(BRUSH))
    static
    inline
    PBRUSH
    LockForWrite(
        _In_ HBRUSH hbr)
    {
        return static_cast<PBRUSH>(
            BASEOBJECT::LockShared(hbr,
                                   GDILoObjType_LO_BRUSH_TYPE,
                                   BASEOBJECT::OWNER::POWNED));
    }

    _Check_return_
    _Ret_opt_bytecap_(sizeof(BRUSH))
    static
    inline
    PBRUSH
    LockAny(
        _In_ HBRUSH hbr)
    {
        return static_cast<PBRUSH>(
            BASEOBJECT::LockShared(hbr,
                                   GDILoObjType_LO_BRUSH_TYPE,
                                   BASEOBJECT::OWNER::NONE));
    }

    UINT
    cjGetObject(
        _In_ UINT cjBuffer,
        _Out_bytecap_(cjBuffer) PLOGBRUSH plbBuffer) const;

    HBITMAP
    hbmGetBitmapHandle(
        _Out_ PUINT puUsage) const;

    VOID
    vSetSolidColor(
        _In_ COLORREF crColor);

    VOID vReleaseAttribute(VOID);
};

/* HACK! */
extern "C"
PGDI_POOL
GetBrushAttrPool(VOID);
