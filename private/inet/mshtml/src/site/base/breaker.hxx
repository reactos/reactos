#ifndef I_BREAKER_HXX_
#define I_BREAKER_HXX_
#pragma INCMSG("--- Beg 'breaker.hxx'")

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#define BREAK_NONE            0x0
#define BREAK_BLOCK_BREAK     0x1
#define BREAK_SITE_BREAK      0x2
#define BREAK_SITE_END        0x4

// The following are not returned by CLineBreakCompat
// but are included for completeness (the saver uses them)
#define BREAK_LINE_BREAK      0x8
#define BREAK_WORD_BREAK      0x10
#define BREAK_NOSCOPE         0x20

class CLineBreakCompat
{
public:

    CLineBreakCompat ( );

    HRESULT QueryBreaks ( CTreePosGap * ptpg, DWORD * pdwBreaks );
    
    HRESULT QueryBreaks ( CMarkupPointer * pPointer, DWORD * pdwBreaks );

    void SetWantPendingBreak ( BOOL );

private:

    HRESULT Init ( CTreePos * ptp );

    HRESULT ComputeNextBreaks ( );

    HRESULT HandleText ( CTreePos * ptpRunNow, long cchInRun );

    //
    // The markup we are currently line breaking.  Also we have a copy
    // of the version number when we started line breaking.
    //

    CMarkup * _pMarkup;

    BOOL _fWantEndingBreak;

    long _lMarkupContentsVersion;
    
    struct CRunEvent
    {
        CTreePos  * ptp;
        CTreeNode * pNode;
        BOOL        fEnd;
    };

    CStackDataAry < CRunEvent, 64 > _aryRunEvents;

    //
    // _ptpWalk is where we have currently walked the line breaking
    // machine up to.
    //
    
    CTreePos * _ptpWalk;

    //
    // The memebers _ptpPrevBreak and _ptpNextBreak define the range over which
    // breaks have been computed.  To be within the range, a pos needs to be
    // after (not equal to) _ptpPrevBreak and before or equal to _ptpNextBreak.
    //
    // If in the range, _aryBreaks will contain the pos's corresponding to
    // the breaks.  Each entry in the array indicated a pos before which the
    // break occurs.
    //

    CTreePos * _ptpPrevBreak;   // After this to be in range
    CTreePos * _ptpNextBreak;   // Before or equal to this to be in range

    struct BreakEntry
    {
        CTreePos * ptp;
        DWORD      bt;
    };

    CStackDataAry < BreakEntry, 8 > _aryBreaks;

    //
    // The following is the state of the line breaking machine
    //
    
    struct CBlockScope
    {
        CTreeNode * pNodeBlock;
        BOOL        fVirgin;
    };

    CStackDataAry < CBlockScope, 32 > _aryStackBS;
    
    CBlockScope & TopScope ( )
    {
        Assert( _aryStackBS.Size() > 0 );
        return _aryStackBS [ _aryStackBS.Size() - 1 ];
    }

    void RemoveScope ( CElement * pElement, CBlockScope & bsRemoved );
    
    enum TextInScope
    {
        TIS_NONE,   // No text in scope
        TIS_REAL,   // Real characters in scope
        TIS_FAKE,   // Break on empty bits and list item fakes text
        TIS_SIMU,   // Simulated text for tables, marquee, etc.
    };

    TextInScope _textInScope;
    
    long _nNextTableBreakId;
    
    BOOL _fInTable;

    CElement * _pElementLastBlockBreak;

    CStackPtrAry < LONG_PTR, 4 > _aryTableBreakCharIds;

    struct BlockBreak
    {
        BlockBreak ( ) : ptpBreak ( NULL ) { }

        void Clear ( ) { ptpBreak = NULL; }

        BOOL IsSet ( ) { return ptpBreak != NULL; }

        //
        // This is the type of break to insert
        //
        
        DWORD btBreak;

        //
        // Put the break before this tree pos
        //

        CTreePos * ptpBreak;

        //
        // 
        //

        long nBreakId;
    };

    BlockBreak _BreakPending;
    
    HRESULT SetPendingBreak ( DWORD btBreak, CTreePos * ptpBreak );

    HRESULT FlushPendingBreak ( );

    HRESULT SetBreak ( DWORD btBreak, CTreePos * ptpBreak );
    
    void ClearPendingBlockBreak ( );
};

#pragma INCMSG("--- End 'breaker.hxx'")
#else
#pragma INCMSG("*** Dup 'breaker.hxx'")
#endif
