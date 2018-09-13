/*
 * CRefCountedObj
 *
 */

#pragma once

#if !defined(_CRefCountedObj_h)
#define _CRefCountedObj_h

#if defined(_DEBUG)
#define Debug(x)    x
#else
#define Debug(x)
#endif

// handles ref counted garbage collection and dyncasting

class CRefCountedObj {
private:
    unsigned    _cUses;

public:

    CRefCountedObj() {
        _cUses = 0;
        }

    // copy ctor, does not copy usage count of object being copied from
    CRefCountedObj ( const CRefCountedObj & ) {
        _cUses = 0;
        }

    // virtual dtor should cause all descendents of CRefCountedObj have to have a virtual dtor
    virtual ~CRefCountedObj() { }

    CRefCountedObj & operator= ( const CRefCountedObj & ) {
        return *this;
        }

    unsigned CUses() {
        return _cUses;
        }

    void Use() {
        _cUses++;
        }

    bool FUnUse() {
        return !(--_cUses);
        }
    };

#endif
