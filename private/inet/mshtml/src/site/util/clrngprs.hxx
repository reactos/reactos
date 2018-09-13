#ifndef I_CLRNGPRS_HXX_
#define I_CLRNGPRS_HXX_
#pragma INCMSG("--- Beg 'clrngprs.hxx'")


MtExtern(CCellRangeParser)


//+------------------------------------------------------------
//
//  Class : CCellRangeParser
//
//-------------------------------------------------------------

class CCellRangeParser
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CCellRangeParser))

    CCellRangeParser(LPCTSTR szRange);

    void    GetRangeRect(RECT *pRect) {*pRect = _RangeRect;}
    BOOL    Failed(void) { return _fFailed;}
    LPCTSTR GetNormalizedRangeName()  {return _strNormString;}
private:
    RECT     _RangeRect;
    BOOL     _fFailed;
    BOOL     _fOneValue;
    CStr     _strNormString;

    void Normalize(LPCTSTR);
    BOOL GetColumn(int *pnCurIndex, long *pnCol);
    BOOL GetNumber(int *pnCurIndex, long *pnRow);
};

#pragma INCMSG("--- End 'clrngprs.hxx'")
#else
#pragma INCMSG("*** Dup 'clrngprs.hxx'")
#endif
