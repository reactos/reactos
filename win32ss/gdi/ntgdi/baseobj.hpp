
#pragma once

#define GDIOBJ_POOL_TAG(type) ('00hG' + (((type) & 0x1f) << 24))

#define BASEOBJECT CBASEOBJECT

class BASEOBJECT : private _BASEOBJECT
{
public:

    enum OWNER
    {
        POWNED = GDI_OBJ_HMGR_POWNED,
        PUBLIC = GDI_OBJ_HMGR_PUBLIC,
        NONE = GDI_OBJ_HMGR_NONE
    };

protected:

    BASEOBJECT(
        _In_ GDILOOBJTYPE loobjtype)
    {
        /* Initialize the object */
        _BASEOBJECT::hHmgr = (HGDIOBJ)(ULONG_PTR)loobjtype;
        this->cExclusiveLock = 0;
        this->ulShareCount = 1;
        this->BaseFlags = 0;//fl & 0xffff;
        DBG_INITLOG(&this->slhLog);
        DBG_LOGEVENT(&this->slhLog, EVENT_ALLOCATE, 0);
    #if DBG_ENABLE_GDIOBJ_BACKTRACES
        DbgCaptureStackBackTace(this->apvBackTrace, 1, GDI_OBJECT_STACK_LEVELS);
    #endif /* GDI_DEBUG */
    }

    static
    BASEOBJECT*
    LockExclusive(
        HGDIOBJ hobj,
        GDIOBJTYPE objtype);

    static
    BASEOBJECT*
    LockExclusive(
        HGDIOBJ hobj,
        GDILOOBJTYPE loobjtype);

    static
    BASEOBJECT*
    LockShared(
        HGDIOBJ hobj,
        GDILOOBJTYPE loobjtype,
        OWNER owner)
    {
        return 0;
    }

    VOID
    vSetObjectAttr(
        _In_opt_ PVOID pvUserAttr)
    {
        GDIOBJ_vSetObjectAttr((POBJ)this, pvUserAttr);
    }


public:

    static
    inline
    PVOID
    pvAllocate(
        _In_ GDIOBJTYPE objtype,
        _In_ SIZE_T cjSize)
    {
        return ExAllocatePoolWithTag(PagedPool, cjSize, GDIOBJ_POOL_TAG(objtype));
    }

    VOID
    vUnlock(
        VOID)
    {
        if (this->cExclusiveLock > 0)
        {
            GDIOBJ_vUnlockObject(this);
        }
        else
        {
            GDIOBJ_vDereferenceObject(this);
        }
    }

    inline
    HGDIOBJ
    hHmgr(
        VOID)
    {
        return _BASEOBJECT::hHmgr;
    }

    HGDIOBJ
    hInsertObject(
        OWNER owner)
    {
        return GDIOBJ_hInsertObject(this, owner);
    }

};


