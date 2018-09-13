/*
 *  @doc    INTERNAL
 *
 *  @module _ime.h -- support for IME APIs |
 *
 *  Purpose:
 *      Most everything to do with FE composition string editing passes
 *      through here.
 *
 *  Authors: <nl>
 *      Jon Matousek  <nl>
 *      Justin Voskuhl  <nl>
 *      Hon Wah Chan  <nl>
 *
 *  History: <nl>
 *      10/18/1995      jonmat  Cleaned up level 2 code and converted it into
 *                              a class hierarchy supporting level 3.
 *
 *  Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
 *
 */

#ifndef I__IME_H_
#define I__IME_H_
#pragma INCMSG("--- Beg '_ime.h'")

#ifndef NO_IME
class CFlowLayout;

// defines for some FE Codepages
#define _JAPAN_CP           932
#define _KOREAN_CP          949
#define _CHINESE_TRAD_CP    950
#define _CHINESE_SIM_CP     936

// special virtual keys copied from Japan MSVC ime.h
#define VK_KANA         0x15
#define VK_KANJI        0x19

// defines for IME Level 2 and 3
#define IME_LEVEL_2     2
#define IME_LEVEL_3     3
#define IME_PROTECTED   4


extern BOOL forceLevel2;    // Force level 2 composition processing if TRUE.

MtExtern(CIme)

/*
 *  IME
 *
 *  @class  base class for IME support.
 *
 *  @devnote
 *      For level 2, at caret IMEs, the IME will draw a window directly over the text giving the
 *      impression that the text is being processed by the application--this is called pseudo inline.
 *      All UI is handled by the IME. This mode is currenlty bypassed in favor of level 3 true inline (TI);
 *      however, it would be trivial to allow a user preference to select this mode. Some IMEs may have
 *      a "special" UI, in which case level 3 TI is *NOT* used, necessitating level 2.
 *
 *      For level 2, near caret IMEs, the IME will draw a very small and obvious window near the current
 *      caret position in the document. This currently occurs for PRC(?) and Taiwan.
 *      All UI is handled by the IME.
 *
 *      For level 3, at caret IMEs, the composition string is drawn by the application, which is called
 *      true inline, bypassing the level 2 "composition window".
 *      Currently, we allow the IME to support all remaining UI *except* drawing of the composition string.
 */
class CIme
{
    friend LRESULT OnGetIMECompositionMode ( CFlowLayout &ts );
    friend BOOL IMECheckGetInvertRange(CFlowLayout *ts, LONG *, LONG *);
    friend HRESULT CompositionStringGlue ( const LPARAM lparam, CFlowLayout &ts );
    friend HRESULT EndCompositionGlue ( CFlowLayout &ts );
    friend void CheckDestroyIME ( CFlowLayout &ts );


    //@access   Protected data
    protected:
    INT       _imeLevel;                            //@cmember IME Level 2 or 3
    BOOL      _fKorean;                             //@cmember In Hangeul mode?

    LONG      _iFormatSave;                         //@cmember  format before we started IME composition mode.

    BOOL      _fHoldNotify;                         //@cmember hold notify until we have result string

    INT       _dxCaret;                             //@cmember current IME caret width
    BOOL      _fIgnoreIMEChar;                      //@cmember Level 2 IME use to eat WM_IME_CHAR message

    public:
#ifdef MERGEFUN
    CTxtRange *_prgUncommitted;                     //Current uncommitted range, for the CRenderer
    CTxtRange *_prgInverted;                        //Current inverted range, for the CRenderer
#endif

    //@access   Public methods
    public:
    BOOL    _fDestroy;                              //@cmember set when object wishes to be deleted.
    INT     _compMessageRefCount;                   //@cmember so as not to delete if recursed.
    BOOL    _fUnderLineMode;                        //@cmember save original Underline mode

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CIme))

                                                    //@cmember  Handle WM_IME_STARTCOMPOSITION
    virtual HRESULT StartComposition ( CFlowLayout &ts ) = 0;
                                                    //@cmember  Handle WM_IME_COMPOSITION and WM_IME_ENDCOMPOSITION
    virtual HRESULT CompositionString ( const LPARAM lparam, CFlowLayout &ts ) = 0;
                                                    //@cmember  Handle post WM_IME_CHAR to update comp window.
    virtual void PostIMEChar( CFlowLayout &ts ) = 0;

                                                    //@cmember  Handle WM_IME_NOTIFY
    virtual HRESULT IMENotify (const WPARAM wparam, const LPARAM lparam, CFlowLayout &ts ) = 0;

    enum TerminateMode
    {
            TERMINATE_NORMAL = 1,
            TERMINATE_FORCECANCEL = 2
    };

    void    TerminateIMEComposition(CFlowLayout &ts,
                CIme::TerminateMode mode);          //@cmember  Terminate current IME composition session.

    BOOL    IsKoreanMode() { return _fKorean; }     //@cmember  check for Korean mode

    BOOL    HoldNotify() { return _fHoldNotify; }   //@cmember  check if we want to hold sending change notification
                                                    //@cmember  check if we need to ignore WM_IME_CHAR messages
    BOOL    IgnoreIMECharMsg() { return _fIgnoreIMEChar; }

    INT     GetIMECaretWidth() { return _dxCaret; } //@cmember  return current caret width
    void    SetIMECaretWidth(INT dxCaretWidth)
    {
        _dxCaret = dxCaretWidth;                    //@cmember  setup current caret width
    }
    static  void    CheckKeyboardFontMatching ( UINT cp, CFlowLayout &ts, LONG *iFormat ); //@cmember  Check current font/keyboard matching.

    INT     GetIMELevel ()                          //@cmember  Return the current IME level.
    {
        return _imeLevel;
    }

    BOOL    IsProtected()
    {
        return _imeLevel == IME_PROTECTED;
    }

    //@access   Protected methods
    protected:                                      //@cmember  Get composition string, convert to unicode.

    static INT GetCompositionStringInfo( HIMC hIMC, DWORD dwIndex, WCHAR *uniCompStr, INT cchUniCompStr, BYTE *attrib, INT cbAttrib, LONG *cursorCP, LONG *cchAttrib );
    HRESULT CheckInsertResultString ( const LPARAM lparam, CFlowLayout &ts );


    void    SetCompositionFont ( CFlowLayout &ts, BOOL *pbUnderLineMode ); //@cmember  Setup for level 2 and 3 composition and candidate window's font.
    void    SetCompositionForm ( CFlowLayout &ts );    //@cmember  Setup for level 2 IME composition window's position.

public:
    CIme( CFlowLayout &ts );
    CIme(); // for CIme_Protected;
    ~CIme();
};

/*
 *  IME_Lev2
 *
 *  @class  Level 2 IME support.
 *
 */
class CIme_Lev2 : public CIme
{

    //@access   Public methods
    public:                                         //@cmember  Handle level 2 WM_IME_STARTCOMPOSITION
    virtual HRESULT StartComposition ( CFlowLayout &ts );
                                                    //@cmember  Handle level 2 WM_IME_COMPOSITION
    virtual HRESULT CompositionString ( const LPARAM lparam, CFlowLayout &ts );
                                                    //@cmember  Handle post WM_IME_CHAR to update comp window.
    virtual void PostIMEChar( CFlowLayout &ts );
                                                    //@cmember  Handle level 2 WM_IME_NOTIFY
    virtual HRESULT IMENotify (const WPARAM wparam, const LPARAM lparam, CFlowLayout &ts );

    CIme_Lev2( CFlowLayout &ts );
    ~CIme_Lev2();
};

/*
 *  IME_PROTECTED
 *
 *  @class  IME_PROTECTED
 *
 */
class CIme_Protected : public CIme
{
    //@access   Public methods
    public:                                         //@cmember  Handle level 2 WM_IME_STARTCOMPOSITION
    virtual HRESULT StartComposition ( CFlowLayout &ts )
        {_imeLevel  = IME_PROTECTED; return S_OK;}
                                                    //@cmember  Handle level 2 WM_IME_COMPOSITION
    virtual HRESULT CompositionString ( const LPARAM lparam, CFlowLayout &ts );
                                                    //@cmember  Handle post WM_IME_CHAR to update comp window.
    virtual void PostIMEChar( CFlowLayout &ts )
        {}
                                                    //@cmember  Handle level 2 WM_IME_NOTIFY
    virtual HRESULT IMENotify (const WPARAM wparam, const LPARAM lparam, CFlowLayout &ts )
        {return S_FALSE;}
};

/*
 *  IME_Lev3
 *
 *  @class  Level 3 IME support.
 *
 */
class CIme_Lev3 : public CIme_Lev2
{
    //@access   Private data
    private:

    //@access   Protected data
    protected:

    //@access   Public methods
    public:                                         //@cmember  Handle level 3 WM_IME_STARTCOMPOSITION
    virtual HRESULT StartComposition ( CFlowLayout &ts );
                                                    //@cmember  Handle level 3 WM_IME_COMPOSITION
    virtual HRESULT CompositionString ( const LPARAM lparam, CFlowLayout &ts );
                                                    //@cmember  Handle level 3 WM_IME_NOTIFY
    virtual HRESULT IMENotify (const WPARAM wparam, const LPARAM lparam, CFlowLayout &ts );

    BOOL            SetCompositionStyle (   CFlowLayout &ts, CCharFormat &CF, UINT attribute );

    CIme_Lev3( CFlowLayout &ts ) : CIme_Lev2 ( ts ) {};

};

/*
 *  Special IME_Lev3 for Korean Hangeul -> Hanja conversion
 *
 *  @class  Hangual IME support.
 *
 */
class CIme_HangeulToHanja : public CIme_Lev3
{
    //@access   Private data
    private:
    LONG    _xWidth;                                //@cmember width of Korean Hangeul char

    public:
    CIme_HangeulToHanja( CFlowLayout &ts, LONG xWdith );
                                                    //@cmember  Handle Hangeul WM_IME_STARTCOMPOSITION
    virtual HRESULT StartComposition ( CFlowLayout &ts );
                                                    //@cmember  Handle Hangeul WM_IME_COMPOSITION
    virtual HRESULT CompositionString ( const LPARAM lparam, CFlowLayout &ts );
};

// CImeDummy class, exists solely to handle the case where we get a compsition string
// and we have no CIme-derived class instantiated.

class CImeDummy : public CIme
{
public:
    virtual HRESULT StartComposition( CFlowLayout &ts ) { RRETURN(E_FAIL); }
    virtual HRESULT CompositionString( const LPARAM lparam,
                                       CFlowLayout &ts ) { RRETURN(E_FAIL); }
    virtual void    PostIMEChar( CFlowLayout &ts ) {};
    virtual HRESULT IMENotify (const WPARAM wparam,
                               const LPARAM lparam,
                               CFlowLayout &ts ) { RRETURN(E_FAIL); }

    CImeDummy( CFlowLayout &ts ) : CIme( ts ) {};
};

// Glue functions to call the respective methods of an IME object stored in the ts.
HRESULT StartCompositionGlue ( CFlowLayout &ts, BOOL IsProtected );
HRESULT CompositionStringGlue ( const LPARAM lparam, CFlowLayout &ts );
HRESULT EndCompositionGlue ( CFlowLayout &ts );
void    PostIMECharGlue ( CFlowLayout &ts );
HRESULT IMENotifyGlue ( const WPARAM wparam, const LPARAM lparam, CFlowLayout &ts ); // @parm the containing text edit.

// IME helper functions.
void    IMECompositionFull ( CFlowLayout &ts );
LRESULT OnGetIMECompositionMode ( CFlowLayout &ts );
BOOL    IMECheckGetInvertRange(CFlowLayout *ts, LONG *, LONG *, LONG *, LONG *);
void    CheckDestroyIME ( CFlowLayout &ts );
BOOL    IMEHangeulToHanja ( CFlowLayout &ts );

/*
 *  IgnoreIMEInput()
 *
 *  @devnote
 *      This is to ignore the IME character.  By translating
 *      message with result from pImmGetVirtualKey, we
 *      will not receive START_COMPOSITION message.  However,
 *      if the host has already called TranslateMessage, then,
 *      we will let START_COMPOSITION message to come in and
 *      let IME_PROTECTED class to do the work.
 */
HRESULT IgnoreIMEInput( HWND hwnd, CFlowLayout &ts, DWORD lParam );
#endif // !NO_IME

#pragma INCMSG("--- End '_ime.h'")
#else
#pragma INCMSG("*** Dup '_ime.h'")
#endif
