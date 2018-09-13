//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       debugpaint.hxx
//
//  Contents:   Utility class to debug painting code.
//
//----------------------------------------------------------------------------

#ifndef I_DEBUGPAINT_HXX_
#define I_DEBUGPAINT_HXX_
#pragma INCMSG("--- Beg 'debugpaint.hxx'")

#if DBG==1
PerfDbgExtern(tagPaintShow);
PerfDbgExtern(tagPaintPause);
PerfDbgExtern(tagPaintWait);
#endif

class CRegion;

//+---------------------------------------------------------------------------
//
//  Class:      CDebugPaint
//
//  Synopsis:   Utility class to debug painting code.
//
//----------------------------------------------------------------------------

class CDebugPaint
{
public:
                            CDebugPaint() {}
                            ~CDebugPaint() {}
    
    static BOOL             UseDisplayTree() {return TRUE;}
    static BOOL             NoOffScreen();
    
#if DBG==1
    static void             ShowPaint(
                                const RECT* prcPaint,
                                HRGN rgnPaint,
                                HDC hdc,
                                TRACETAG tagShow,
                                TRACETAG tagPause,
                                TRACETAG tagWait,
                                BOOL fHatch);
    
    static void             PausePaint(TRACETAG tagWait);
    
    void                    StartTimer();
    void                    StopTimer(TRACETAG tag, char* message, BOOL fMicrosec);
    
    __int64                 _timeStart;
    

#else // !DBG
    static void             ShowPaint(
                                const RECT* prcPaint,
                                HRGN rgnPaint,
                                HDC hdc,
                                TRACETAG tagShow,
                                TRACETAG tagPause,
                                TRACETAG tagWait,
                                BOOL fHatch) {}
    
    static void             PausePaint(TRACETAG tagWait) {}
    
    void                    StartTimer() {}
    void                    StopTimer(TRACETAG tag, char* message, BOOL fMicrosec) {}
#endif
};

#if DBG==1
class CRegionRects
{
public:
    CRegionRects(HRGN hrgn)
    {
        GetRegionData(hrgn, sizeof(_rd), (RGNDATA *)&_rd);
        _count = _rd.rdh.nCount;
    }
    CRegionRects(const CRegion& rgn);
    
    struct {
        RGNDATAHEADER rdh;
        RECT arc[8];
    } _rd;
    long _count;
};
#endif

#pragma INCMSG("--- End 'debugpaint.hxx'")
#else
#pragma INCMSG("*** Dup 'debugpaint.hxx'")
#endif

