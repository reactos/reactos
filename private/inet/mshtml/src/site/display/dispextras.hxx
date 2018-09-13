//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispextras.hxx
//
//  Contents:   Extra information that can be added to CDispXXXPlus nodes.
//
//----------------------------------------------------------------------------

#ifndef I_DISPEXTRAS_HXX_
#define I_DISPEXTRAS_HXX_
#pragma INCMSG("--- Beg 'dispextras.hxx'")

#ifndef X_FLAGS_HXX_
#define X_FLAGS_HXX_
#include "flags.hxx"
#endif

class CDispInfo;

//+---------------------------------------------------------------------------
//
//  Class:      CDispExtras
//
//  Synopsis:   
//
//----------------------------------------------------------------------------

class CDispExtras
{
private:
                            // CDispExtras is never explicitly constructed
                            // or destructed
                            CDispExtras() {Assert(FALSE);}
                            ~CDispExtras() {Assert(FALSE);}
    
public:
    static size_t           GetExtrasSize(
                                BOOL fHasExtraCookie,
                                BOOL fHasUserClip,
                                BOOL fHasInset,
                                DISPNODEBORDER borderType);
    static size_t           GetExtrasSize(const CDispExtras* pExtras);
    
    void                    operator= (const CDispExtras& extras)
                                    {memcpy((void*) this, (void*) &extras,
                                        GetExtrasSize(
                                            extras.HasExtraCookie(),
                                            extras.HasUserClip(),
                                            extras.HasInset(),
                                            extras.GetBorderType()));}
    
    
    BOOL                    HasExtraCookie() const
                                    {return _extraFlags.IsSet(s_fHasExtraCookie);}
    void                    SetHasExtraCookie(BOOL fHasExtraCookie)
                                    {_extraFlags.SetBoolean(
                                        s_fHasExtraCookie, fHasExtraCookie);}
    void*                   GetExtraCookie() const;
    void                    SetExtraCookie(void* cookie);
    
    BOOL                    HasUserClip() const
                                    {return _extraFlags.IsSet(s_fHasUserClip);}
    void                    SetHasUserClip(BOOL fHasUserClip)
                                    {_extraFlags.SetBoolean(
                                        s_fHasUserClip, fHasUserClip);}
    const RECT&             GetUserClip() const;
    void                    SetUserClip(const RECT& rcUserClip);
    
    BOOL                    HasInset() const
                                    {return _extraFlags.IsSet(s_fHasInset);}
    void                    SetHasInset(BOOL fHasInset)
                                    {_extraFlags.SetBoolean(
                                        s_fHasInset, fHasInset);}
    const SIZE&             GetInset() const;
    void                    SetInset(const SIZE& sizeInset);
    
    DISPNODEBORDER          GetBorderType() const
                                    {return (DISPNODEBORDER)
                                        _extraFlags.GetValue(
                                            s_borderType, s_borderTypeShift);}
    void                    SetBorderType(DISPNODEBORDER borderType)
                                    {_extraFlags.SetValue(
                                        borderType, s_borderType,
                                        s_borderTypeShift);}
    void                    GetBorderWidths(CRect* prcBorderWidths) const;
    void                    SetBorderWidths(LONG simpleWidth);
    void                    SetBorderWidths(const RECT& rcBorderWidths);
    
    void                    GetExtraInfo(CDispInfo* pInfo) const;
    
private:
    LONG                    GetExtrasCode() const
                                    {return _extraFlags.GetValue(
                                        s_extrasMask, s_extrasShift);}
    LONG                    GetExtrasCodeWithCookie() const
                                    {return _extraFlags.GetValue(
                                        s_extrasWithCookieMask,
                                        s_extrasWithCookieShift);}
    
    typedef CFlags32 CFlags;
    
    static const CFlags     s_fHasUserClip;
    static const CFlags     s_fHasInset;
    static const CFlags     s_borderType;
    static const int        s_borderTypeShift;
    static const CFlags     s_fHasExtraCookie;
    
    static const CFlags     s_extrasMask;
    static const int        s_extrasShift;
    static const CFlags     s_extrasWithCookieMask;
    static const int        s_extrasWithCookieShift;
    
    // data size is 4-44 bytes
    CFlags  _extraFlags;
    
    // The following union is used to store various permutations of extra node
    // information, which can include an optional user clip rect, an optional inset
    // size, an optional simple border (a LONG value) or a complex border
    // (a RECT-sized structure of border widths), and an optional cookie value
    union
    {
        // no cookie, no user clip, no inset, simple border
        struct
        {
            LONG    _borderWidth;
        } _ck0uc0in0bs;
        
        // no cookie, no user clip, no inset, complex border
        struct
        {
            RECT    _rcBorderWidths;
        } _ck0uc0in0bc;
        
        // no cookie, no user clip, inset, no border
        struct
        {
            SIZE    _sizeInset;
        } _ck0uc0in1b0;
        
        // no cookie, no user clip, inset, simple border
        struct
        {
            SIZE    _sizeInset;
            LONG    _borderWidth;
        } _ck0uc0in1bs;
        
        // no cookie, no user clip, inset, complex border
        struct
        {
            SIZE    _sizeInset;
            RECT    _rcBorderWidths;
        } _ck0uc0in1bc;
        
        // no cookie, user clip, no inset, no border
        struct
        {
            RECT    _rcUserClip;
        } _ck0uc1in0b0;
        
        // no cookie, user clip, no inset, simple border
        struct
        {
            RECT    _rcUserClip;
            LONG    _borderWidth;
        } _ck0uc1in0bs;
        
        // no cookie, user clip, no inset, complex border
        struct
        {
            RECT    _rcUserClip;
            RECT    _rcBorderWidths;
        } _ck0uc1in0bc;
        
        // no cookie, user clip, inset, no border
        struct
        {
            RECT    _rcUserClip;
            SIZE    _sizeInset;
        } _ck0uc1in1b0;
        
        // no cookie, user clip, inset, simple border
        struct
        {
            RECT    _rcUserClip;
            SIZE    _sizeInset;
            LONG    _borderWidth;
        } _ck0uc1in1bs;
        
        // no cookie, user clip, inset, complex border
        struct
        {
            RECT    _rcUserClip;
            SIZE    _sizeInset;
            RECT    _rcBorderWidths;
        } _ck0uc1in1bc;

        // cookie, no user clip, no inset, no border
        struct
        {
            void*   _cookie;
        } _ck1uc0in0b0;
        
        // cookie, no user clip, no inset, simple border
        struct
        {
            void*   _cookie;
            LONG    _borderWidth;
        } _ck1uc0in0bs;
        
        // cookie, no user clip, no inset, complex border
        struct
        {
            void*   _cookie;
            RECT    _rcBorderWidths;
        } _ck1uc0in0bc;
        
        // cookie, no user clip, inset, no border
        struct
        {
            void*   _cookie;
            SIZE    _sizeInset;
        } _ck1uc0in1b0;
        
        // cookie, no user clip, inset, simple border
        struct
        {
            void*   _cookie;
            SIZE    _sizeInset;
            LONG    _borderWidth;
        } _ck1uc0in1bs;
        
        // cookie, no user clip, inset, complex border
        struct
        {
            void*   _cookie;
            SIZE    _sizeInset;
            RECT    _rcBorderWidths;
        } _ck1uc0in1bc;
        
        // cookie, user clip, no inset, no border
        struct
        {
            void*   _cookie;
            RECT    _rcUserClip;
        } _ck1uc1in0b0;
        
        // cookie, user clip, no inset, simple border
        struct
        {
            void*   _cookie;
            RECT    _rcUserClip;
            LONG    _borderWidth;
        } _ck1uc1in0bs;
        
        // cookie, user clip, no inset, complex border
        struct
        {
            void*   _cookie;
            RECT    _rcUserClip;
            RECT    _rcBorderWidths;
        } _ck1uc1in0bc;
        
        // cookie, user clip, inset, no border
        struct
        {
            void*   _cookie;
            RECT    _rcUserClip;
            SIZE    _sizeInset;
        } _ck1uc1in1b0;
        
        // cookie, user clip, inset, simple border
        struct
        {
            void*   _cookie;
            RECT    _rcUserClip;
            SIZE    _sizeInset;
            LONG    _borderWidth;
        } _ck1uc1in1bs;
        
        // cookie, user clip, inset, complex border
        struct
        {
            void*   _cookie;
            RECT    _rcUserClip;
            SIZE    _sizeInset;
            RECT    _rcBorderWidths;
        } _ck1uc1in1bc;
    };
};


#pragma INCMSG("--- End 'dispextras.hxx'")
#else
#pragma INCMSG("*** Dup 'dispextras.hxx'")
#endif
