//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       htpvpv.hxx
//
//  Contents:   Hash table mapping PVOID to PVOID
//
//              CHtPvPv
//
//----------------------------------------------------------------------------

#ifndef I_HTPVPV_HXX_
#define I_HTPVPV_HXX_
#pragma INCMSG("--- Beg 'htpvpv.hxx'")

MtExtern(CHtPvPv)

class CHtPvPv
{

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtPvPv))

                CHtPvPv();
               ~CHtPvPv();

	void *      Lookup(void * pvKey);
	HRESULT     Insert(void * pvKey, void * pvVal);
	void *      Remove(void * pvKey);
    UINT        GetCount()                      { return(_cEnt); }
    UINT        GetDelCount()                   { return(_cEntDel); }
    UINT        GetMaxCount()                   { return(_cEntMax); }

#if DBG==1
    BOOL        IsPresent(void * pvKey);
#endif

protected:

    struct HTENT
    {
        void *  pvKey;
        void *  pvVal;
    };

    UINT        ComputeProbe(void * pvKey)      { return((UINT)((DWORD_PTR)pvKey % _cEntMax)); }
    UINT        ComputeStride(void * pvKey)     { return((UINT)(((DWORD_PTR)pvKey >> 2) & _cStrideMask) + 1); }
	static UINT ComputeStrideMask(UINT cEntMax);
    void        Rehash(UINT cEntMax);
    HRESULT     Grow();
    void        Shrink();

private:

    HTENT *     _pEnt;              // Pointer to entries vector
    UINT        _cEnt;              // Number of active entries in the table
    UINT        _cEntDel;           // Number of free entries marked deleted
    UINT        _cEntGrow;          // Number of entries when table should be expanded
    UINT        _cEntShrink;        // Number of entries when table should be shrunk
    UINT        _cEntMax;           // Number of entries in entire vector
    UINT        _cStrideMask;       // Mask for computing secondary hash
    HTENT *     _pEntLast;          // Last entry probed during lookup
    HTENT       _EntEmpty;          // Empty hash table entry (initial state)

};

#pragma INCMSG("--- End 'htpvpv.hxx'")
#else
#pragma INCMSG("*** Dup 'htpvpv.hxx'")
#endif
