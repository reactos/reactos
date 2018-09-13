//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       flags.hxx
//
//  Contents:   Classes to encapsulate expandable bit flags.
//
//----------------------------------------------------------------------------

#ifndef I_FLAGS_HXX_
#define I_FLAGS_HXX_
#pragma INCMSG("--- Beg 'flags.hxx'")

//----------------------------------------------------------------------------
//
//  This file defines several classes to simplify usage of bit flags.  In
//  an ideal world, the interface would be defined in an abstract base class,
//  perhaps "CFlags", and then collections of different sizes would be
//  created as subclasses of CFlags, like CFlags32 and CFlags64.  However,
//  that would require methods to be virtual, and that's much more expensive
//  than we're willing to accept.  Therefore, the different size collections
//  are unrelated classes with similar interfaces.
//  
//----------------------------------------------------------------------------


//+---------------------------------------------------------------------------
//
//  Class:      CFlags32
//
//  Synopsis:   32 bit-flags.
//
//----------------------------------------------------------------------------

class CFlags32
{
public:
                            CFlags32() {}
                            CFlags32(const CFlags32& f) {_flags = f._flags;}
                            CFlags32(LONG value) {_flags = value;}
                            ~CFlags32() {}
    
    void                    Set(const CFlags32& f) {_flags |= f._flags;}
    void                    SetMasked(
                                const CFlags32& f,
                                const CFlags32& mask)
                                    {_flags = 
                                        (_flags & ~mask._flags) |
                                        (f._flags & mask._flags);}
    void                    Clear(const CFlags32& f) {_flags &= ~f._flags;}
    void                    Mask(const CFlags32& mask) {_flags &= mask._flags;}
    
    void                    operator += (const CFlags32& f) {Set(f);}
    void                    operator -= (const CFlags32& f) {Clear(f);}

    BOOL                    operator == (const CFlags32& f) const
                                    {return _flags == f._flags;}
    BOOL                    operator != (const CFlags32& f) const
                                    {return _flags != f._flags;}
    
    BOOL                    IsSet(const CFlags32& f) const
                                    {return (_flags & f._flags) != 0;}
    BOOL                    AllSet(const CFlags32& f) const
                                    {return (_flags & f._flags) == f._flags;}
    BOOL                    IsAnySet() const
                                    {return _flags != 0;}
    BOOL                    MaskedEquals(
                                const CFlags32& mask, const CFlags32& value) const
                                    {return (_flags & mask._flags) == value._flags;}
    void                    SetBoolean(const CFlags32& f, BOOL fOn)
                                    {if (fOn) Set(f); else Clear(f);}
    
    // for dealing with multi-bit values stored in flags
    LONG                    GetValue(const CFlags32& mask, int shiftAmount) const
                                    {return (LONG)
                                    (((DWORD)(_flags&mask._flags))>>shiftAmount);}
    void                    SetValue(LONG value, const CFlags32& mask,
                                     int shiftAmount)
                                    {_flags = (_flags & ~mask._flags) |
                                        (value << shiftAmount);}
    
    LONG    _flags;
};

inline CFlags32 operator+(const CFlags32& a, const CFlags32& b)
        {return CFlags32(a._flags | b._flags);}

inline CFlags32 operator-(const CFlags32& a, const CFlags32& b)
        {return CFlags32(a._flags & ~b._flags);}


//+---------------------------------------------------------------------------
//
//  Class:      CFlags64
//
//  Synopsis:   64 bit-flags.
//
//----------------------------------------------------------------------------

class CFlags64
{
public:
                            CFlags64() {}
                            CFlags64(const CFlags64& f)
                                    {_flags1 = f._flags1; _flags2 = f._flags2;}
                            CFlags64(LONG value1, LONG value2)
                                    {_flags1 = value1; _flags2 = value2;}
                            CFlags64(LONG value1)
                                    {_flags1 = value1; _flags2 = 0;}
                            ~CFlags64() {}
    
    void                    Set(const CFlags64& f)
                                    {_flags1 |= f._flags1; _flags2 |= f._flags2;}
    void                    SetMasked(
                                const CFlags64& f,
                                const CFlags64& mask)
                                    {_flags1 = (_flags1 & ~mask._flags1) |
                                        (f._flags1 & mask._flags1);
                                     _flags2 = (_flags2 & ~mask._flags2) |
                                        (f._flags2 & mask._flags2);}
    void                    Clear(const CFlags64& f)
                                    {_flags1 &= ~f._flags1;
                                     _flags2 &= ~f._flags2;}
    void                    Mask(const CFlags64& mask)
                                    {_flags1 &= mask._flags1;
                                     _flags2 &= mask._flags2;}
    
    void                    operator += (const CFlags64& f) {Set(f);}
    void                    operator -= (const CFlags64& f) {Clear(f);}

    
    BOOL                    IsSet(const CFlags64& f) const
                                    {return
                                        (_flags1 & f._flags1) != 0 &&
                                        (_flags2 & f._flags2) != 0;}
    BOOL                    AllSet(const CFlags64& f) const
                                    {return 
                                        (_flags1 & f._flags1) == f._flags1 &&
                                        (_flags2 & f._flags2) == f._flags2;}
    BOOL                    IsAnySet() const
                                    {return (_flags1 | _flags2) != 0;}
    CFlags64                Subset(const CFlags64& f) const
                                    {return CFlags64(_flags1 & f._flags1,
                                                     _flags2 & f._flags2);}
    BOOL                    MaskedEquals(
                                const CFlags64& mask, const CFlags64& value) const
                                    {return
                                        (_flags1&mask._flags1) == value._flags1 &&
                                        (_flags2&mask._flags2) == value._flags2;}
    void                    SetBoolean(const CFlags64& f, BOOL fOn)
                                    {if (fOn) Set(f); else Clear(f);}
    
    LONG    _flags1;
    LONG    _flags2;
};

inline CFlags64 operator+(const CFlags64& a, const CFlags64& b)
        {return CFlags64(a._flags1 | b._flags1, a._flags2 | b._flags2);}

inline CFlags64 operator-(const CFlags64& a, const CFlags64& b)
        {return CFlags64(a._flags1 & ~b._flags1, a._flags2 & ~b._flags2);}


#pragma INCMSG("--- End 'flags.hxx'")
#else
#pragma INCMSG("*** Dup 'flags.hxx'")
#endif

