// stab.cpp - symbol table for created UDT syms
#include "shinc.hpp"

struct STAB {
public:
    BOOL fFindSym(LPSSTR lpsstr, PFNCMP pfnCmp, SHFLAG fCase, UDTPTR* ppsym, unsigned *piHash);
    BOOL fAddSym(LPSSTR lpsstr, unsigned iHash, UDTPTR* ppsym);
    STAB();
    ~STAB();

private:
    unsigned itMac;
    unsigned itEntries;
    UDTPTR* rgpsym;
    BOOL fCreateNewUDTSym(LPSSTR lpsstr, unsigned iHash, UDTPTR* ppsym);
    BOOL resize();

    unsigned hash(LPB lpName)
    {
        return hashPbCb(lpName + 1, *lpName, itMac);
    }
    unsigned hash(LPSSTR lpsstr)
    {
        return hashPbCb(lpsstr->lpName, lpsstr->cb, itMac);
    }
};

BOOL STABOpen(STAB **ppstab)
{
    *ppstab = new STAB;
    return *ppstab != 0;
}

BOOL STABFindUDTSym(STAB* pstab, LPSSTR lpsstr, PFNCMP pfnCmp, SHFLAG fCase, UDTPTR *ppsym, unsigned *piHash)
{
    *piHash = 0;
    assert(pstab);
    return pstab->fFindSym(lpsstr, pfnCmp, fCase, (UDTPTR *)ppsym, piHash);
}

BOOL STABAddUDTSym(STAB* pstab, LPSSTR lpsstr, unsigned iHash, UDTPTR* ppsym)
{
    assert(pstab);
    return pstab->fAddSym(lpsstr, iHash, (UDTPTR *)ppsym);
}

void STABClose(STAB* pstab)
{
    delete pstab;
}

STAB::STAB()
{
    itEntries = 0;
    itMac = 1024;
    rgpsym = (UDTPTR *) MHAlloc(itMac * sizeof(UDTPTR));
    memset(rgpsym, 0, itMac * sizeof(UDTPTR));
}

STAB::~STAB()
{
    for (unsigned i = 0; i < itMac; i++)
        if (rgpsym[i])
            MHFree(rgpsym[i]);

    MHFree(rgpsym);
    itEntries = 0;
    itMac = 1024;
    rgpsym = 0;

}

BOOL STAB::fFindSym(LPSSTR lpsstr, PFNCMP pfnCmp, SHFLAG fCase, UDTPTR* ppsym, unsigned* piHash)
{
    *ppsym = 0;

    // nothing but S_UDTs here - if were looking for a specific type of sym
    // and its not a S_UDT - don't bother
    if ((lpsstr->searchmask & SSTR_symboltype ) &&
        ( lpsstr->symtype != S_UDT ))
        return FALSE;

    for (*piHash = hash(lpsstr);
        rgpsym[*piHash];
        *piHash = (*piHash < itMac) ? *piHash + 1: 0) {
        // thats right pfnCmp returns 0 if compare succeeds
        if (!(*pfnCmp) ( lpsstr, (SYMPTR)rgpsym[*piHash], (char *)(rgpsym[*piHash]->name), fCase )) {
            // got it  - return the sym
            *ppsym = rgpsym[*piHash];
            return TRUE;
        }
    }

    return FALSE;
}

BOOL STAB::fAddSym(LPSSTR lpsstr, unsigned iHash, UDTPTR* ppsym)
{
    if (!fCreateNewUDTSym(lpsstr, iHash, ppsym))
        return FALSE;

    if ((itEntries >> 1) > itMac) {
        // over half full - double itMac and rehash the table
        if (!resize())
            return FALSE;
    }

    return TRUE;
}

BOOL STAB::fCreateNewUDTSym(LPSSTR lpsstr, unsigned iHash, UDTPTR* ppsym)
{
    *ppsym = rgpsym[iHash] = (UDTPTR) MHAlloc(sizeof(UDTSYM) + lpsstr->cb);

    if (*ppsym) {
        (*ppsym)->reclen = sizeof(UDTSYM) + lpsstr->cb;
        (*ppsym)->rectyp = S_UDT;
        (*ppsym)->typind = 0;
        (*ppsym)->name[0] = lpsstr->cb;
        memcpy((*ppsym)->name + 1, lpsstr->lpName, lpsstr->cb);
        itEntries++;
        return TRUE;
    }

    return FALSE;
}

BOOL STAB::resize()
{
    unsigned itMac_ = itMac;
    itMac *= 2;         // double size of hash table
    UDTPTR *rgpsym_ = rgpsym;
    rgpsym = (UDTPTR *) MHAlloc(itMac * sizeof(UDTPTR));
    if (!rgpsym)  {
        rgpsym = rgpsym_;
        return FALSE;
    }

    memset(rgpsym, 0, itMac * sizeof(UDTPTR));

    for (unsigned i_ = 0; i_ < itMac_; i_++)  {
        if (rgpsym_[i_]) {
            for (unsigned i = hash((LPB)(rgpsym_[i_]->name));
                rgpsym[i];
                i = (i < itMac) ? i + 1: 0);
            rgpsym[i] = rgpsym_[i_];
        }
    }

    MHFree(rgpsym_);
    return TRUE;
}
