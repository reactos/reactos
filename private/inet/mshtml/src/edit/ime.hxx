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

#ifndef I__IME_HXX_
#define I__IME_HXX_
#pragma INCMSG("--- Beg 'ime.hxx'")

class CMarkupUndo;
class CCaretTracker;

#ifndef NO_IME

#if defined(IME_RECONVERSION)

//
// WINVER >= 0x0400 define
//

#ifndef WM_IME_REQUEST
  #define WM_IME_REQUEST 0x0288
#endif

//
// WINVER >= 0x040A defines
//

#ifndef IMR_RECONVERTSTRING
  #define IMR_RECONVERTSTRING 0x0004
  #define IMR_CONFIRMRECONVERTSTRING 0x0005
  typedef struct tagRECONVERTSTRING {
      DWORD dwSize;
      DWORD dwVersion;
      DWORD dwStrLen;
      DWORD dwStrOffset;
      DWORD dwCompStrLen;
      DWORD dwCompStrOffset;
      DWORD dwTargetStrLen;
      DWORD dwTargetStrOffset;
  } RECONVERTSTRING, *PRECONVERTSTRING, NEAR *NPRECONVERTSTRING, FAR *LPRECONVERTSTRING;
#endif // IMR_RECONVERTSTRING

#define MAX_RECONVERSION_SIZE 100

#endif // IME_RECONVERSION
  
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

// defines for termination

const DWORD TERMINATE_NORMAL=1;
const DWORD TERMINATE_FORCECANCEL=2;

extern BOOL forceLevel2;    // Force level 2 composition processing if TRUE.

MtExtern(CIme)

UINT ConvertLanguageIDtoCodePage( WORD langid );
UINT GetKeyboardCodePage();

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
    friend class CSelectionManager;
    
    //@access   Protected data
    protected:
    INT       _imeLevel;                            //@cmember IME Level 2 or 3
    BOOL      _fKorean;                             //@cmember In Hangeul mode?

    LONG      _iFormatSave;                         //@cmember  format before we started IME composition mode.

    BOOL      _fHoldNotify;                         //@cmember hold notify until we have result string

    INT       _dxCaret;                             //@cmember current IME caret width
    BOOL      _fIgnoreIMEChar;                      //@cmember Level 2 IME use to eat WM_IME_CHAR message

    public:

    CSelectionManager * _pManager;                  // back pointer
    CMarkupUndo    * _pMarkupUndo;                     //Undo Manager conduit object

    //@access   Public methods
    public:
    BOOL    _fDestroy;                              //@cmember set when object wishes to be deleted.
    INT     _compMessageRefCount;                   //@cmember so as not to delete if recursed.
    BOOL    _fUnderLineMode;                        //@cmember save original Underline mode

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CIme))

    virtual HRESULT StartComposition( ) = 0;
    virtual HRESULT CompositionString( const LPARAM lparam, TRACKER_NOTIFY * peTrackerNotify ) = 0;
    virtual void PostIMEChar() = 0;
    virtual HRESULT IMENotify( const WPARAM wparam, const LPARAM lparam ) = 0;

    void    TerminateIMEComposition(
                DWORD dwMode,           //@cmember  Terminate current IME composition session.
                TRACKER_NOTIFY * peTrackerNotify );

    BOOL    IsKoreanMode() { return _fKorean; }     //@cmember  check for Korean mode

    BOOL    HoldNotify() { return _fHoldNotify; }   //@cmember  check if we want to hold sending change notification
                                                    //@cmember  check if we need to ignore WM_IME_CHAR messages
    BOOL    IgnoreIMECharMsg() { return _fIgnoreIMEChar; }

    INT     GetIMECaretWidth() { return _dxCaret; } //@cmember  return current caret width
    void    SetIMECaretWidth(INT dxCaretWidth)
    {
        _dxCaret = dxCaretWidth;                    //@cmember  setup current caret width
    }

    INT     GetIMELevel ()                          //@cmember  Return the current IME level.
    {
        return _imeLevel;
    }

    BOOL    IsProtected()
    {
        return _imeLevel == IME_PROTECTED;
    }

    //@access   Protected methods
    public:                                        //@cmember  Get composition string, convert to unicode.

    INT GetCompositionStringInfo( DWORD dwIndex, WCHAR *pchCompStr, INT cchUniCompStr, BYTE *pbAttrib, INT cbAttrib, LONG *pcpCursorCp, LONG *pcchAttrib );
    HRESULT CheckInsertResultString( const LPARAM lparam );


    void    SetCompositionFont ( BOOL *pbUnderLineMode ); //@cmember  Setup for level 2 and 3 composition and candidate window's font.
    void    SetCompositionForm ();    //@cmember  Setup for level 2 IME composition window's position.

    CODEPAGE KeyboardCodePage() { return _pManager->KeyboardCodePage(); }
    HIMC     ImmGetContext() { return _pManager->ImmGetContext(); }
    void     ImmReleaseContext( HIMC himc ) { _pManager->ImmReleaseContext( himc ); }
    HRESULT  HandleImeComposition( SelectionMessage *pMessage );
    IHTMLViewServices* GetViewServices() { return _pManager->GetViewServices(); }

    HRESULT  HandleMessage( 
        SelectionMessage *pMessage, 
        DWORD* pdwFollowUpAction, 
        TRACKER_NOTIFY * peTrackerNotify );

    HRESULT AddHighlightSegment( LONG index,
                                 BOOL fLast,
                                 LONG ichMin,
                                 LONG ichMost,
                                 HIGHLIGHT_TYPE HighlightType );
    HRESULT ClearHighlightSegments();
    HRESULT GetCompositionPos( POINT * ppt,
                               RECT * prc,
                               long * plLineHeight );

public:
    CIme( CSelectionManager * pManager );
    ~CIme();
    HRESULT Init();
    void Deinit();
    HRESULT ReplaceRange( TCHAR * pch,
                          LONG cch,
                          BOOL fShowCaret = TRUE,
                          LONG ichCaret   = -1,
                          BOOL fMoveIP    = FALSE );
    HRESULT AdjustUncommittedRangeAroundInsertionPoint();
    HRESULT ClingUncommittedRangeToText();
    CSpringLoader * GetSpringLoader();
    HRESULT SetCaretVisible( BOOL fVisible );
    HRESULT SetCaretVisible( IHTMLDocument2* pIDoc, BOOL fVisible );
    HRESULT SetNotAtBOL(BOOL fNotAtBOL) { _fNotAtBOL = fNotAtBOL; return S_OK; }
    HRESULT GetCaretTracker( CCaretTracker ** ppCaretTracker );

private:
    CUndoUnit _undoUnit;
    IMarkupPointer * _pmpInsertionPoint;
    IMarkupPointer * _pmpStartUncommitted;
    IMarkupPointer * _pmpEndUncommitted;
    ISelectionRenderingServices * _pSelRenSvc; 
    BOOL _fNotAtBOL;
    BOOL _fCaretVisible;
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
    public:
    virtual HRESULT StartComposition ();
    virtual HRESULT CompositionString( const LPARAM lparam, TRACKER_NOTIFY * peTrackerNotify );
    virtual void PostIMEChar();
    virtual HRESULT IMENotify( const WPARAM wparam, const LPARAM lparam );

    CIme_Lev2( CSelectionManager * pManager );
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
    public:
    virtual HRESULT StartComposition() {_imeLevel  = IME_PROTECTED; return S_OK;}
    virtual HRESULT CompositionString( const LPARAM lparam, TRACKER_NOTIFY * peTrackerNotify );
    virtual void PostIMEChar() {}
    virtual HRESULT IMENotify( const WPARAM wparam, const LPARAM lparam ) {return S_FALSE;}

    CIme_Protected(CSelectionManager * pManager) : CIme(pManager) {};
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
    public:

    virtual HRESULT StartComposition( );
    virtual HRESULT CompositionString( const LPARAM lparam, TRACKER_NOTIFY * peTrackerNotify );
    virtual HRESULT IMENotify( const WPARAM wparam, const LPARAM lparam );

    BOOL            SetCompositionStyle ( CCharFormat &CF, UINT attribute );

    CIme_Lev3( CSelectionManager * pManager ) : CIme_Lev2 ( pManager ) {};

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
    CIme_HangeulToHanja( CSelectionManager * pManager, LONG xWdith );

    virtual HRESULT StartComposition ();
    virtual HRESULT CompositionString ( const LPARAM lparam, TRACKER_NOTIFY * peTrackerNotify );
};

// CImeDummy class, exists solely to handle the case where we get a compsition string
// and we have no CIme-derived class instantiated.

class CImeDummy : public CIme
{
public:
    virtual HRESULT StartComposition() { RRETURN(E_FAIL); }
    virtual HRESULT CompositionString( const LPARAM lparam, TRACKER_NOTIFY * peTrackerNotify ) { RRETURN(E_FAIL); }
    virtual void    PostIMEChar() {};
    virtual HRESULT IMENotify( const WPARAM wparam, const LPARAM lparam ) { RRETURN(E_FAIL); }

    CImeDummy(CSelectionManager * pManager ) : CIme( pManager ) {};
};

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
HRESULT IgnoreIMEInput( HWND hwnd, DWORD lParam );
#endif // !NO_IME

#pragma INCMSG("--- End 'ime.hxx'")
#else
#pragma INCMSG("*** Dup 'ime.hxx'")
#endif
