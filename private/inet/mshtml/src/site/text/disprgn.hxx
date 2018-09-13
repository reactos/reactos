//+----------------------------------------------------------------------------
//
// File:        Disprgn.hxx
//
// Contents:    Contains class CDispRegion declaration
//
//-----------------------------------------------------------------------------

#ifndef I_DISPRGN_HXX_
#define I_DISPRGN_HXX_
#pragma INCMSG("--- Beg 'disprgn.hxx'")

class CDispNode;

class CDispRegion
{
public:
    CDispRegion();
    ~CDispRegion() { };

    void Init(long iLine, long cp, long yOffset, RECT * prc);

    union
    {
        WORD   _wFlags;
        
        struct
        {
            unsigned    _fBgAndBorder   : 1;
            unsigned    _fTextFlow      : 1;
        };
    };

    WORD        _nLines;        // no of lines in this region
    LONG        _iLine;         // first line of this region
    LONG        _cp;            // start cp

    LONG        _yOffset;       // BUGBUG (srinib) - temporarily storing
    RECT        _rc;            // yOffset & rc here until DisplayTree implementaion
                                // is complete

    CDispNode * _pDispNode;     // display node associated with this region in the
                                // display tree.
};

#pragma INCMSG("--- End 'disprgn.hxx'")
#else
#pragma INCMSG("*** Dup 'disprgn.hxx'")
#endif
