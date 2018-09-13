//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       unknownp.hxx
//
//  Contents:   CUnknownPairList
//
//----------------------------------------------------------------------------

#ifndef I_UNKNOWNP_HXX_
#define I_UNKNOWNP_HXX_
#pragma INCMSG("--- Beg 'unknownp.hxx'")

struct CUnknownPair {
    CStr _cstrA;
    CStr _cstrB;
};

class CUnknownPairList{
private:
    CDataAry<CUnknownPair> *_paryUnknown;
public:
    void Free();
	inline void Detach(){_paryUnknown=NULL;}

    CUnknownPairList(){_paryUnknown=NULL;}
    ~CUnknownPairList(){Free();}

    HRESULT Duplicate(CUnknownPairList &upl) const;
    BOOL    Compare(const CUnknownPairList *pup) const;
    WORD    ComputeCrc() const;
    HRESULT AddUnknownPair(const TCHAR *pchA, const size_t cchA,
                           const TCHAR *pchB, const size_t cchB);
    size_t  Length() const;
    CUnknownPair* Get(size_t i);
};

#pragma INCMSG("--- End 'unknownp.hxx'")
#else
#pragma INCMSG("*** Dup 'unknownp.hxx'")
#endif
