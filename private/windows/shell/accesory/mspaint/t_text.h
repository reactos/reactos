#ifndef __T_TEXT_H__
#define __T_TEXT_H__

#include "imgtools.h"

#define MAX_MOVE_DIST_FOR_PLACE 10 // min pixels to move before not considered a place operation

class CTextTool : public CSelectTool
    {
    DECLARE_DYNAMIC( CTextTool )

    protected:

    class CTedit* m_pCTedit;

    void CreateTextEditObject( CImgWnd* pImgWnd, MTI* pmti );
    void PlaceTextOnBitmap   ( CImgWnd* pImgWnd );

    public:

    CTextTool();
    ~CTextTool();

    virtual void OnUpdateColors( CImgWnd* pImgWnd );
    virtual void OnActivate    ( BOOL bActivate );
    virtual void OnCancel      ( CImgWnd* pImgWnd );
    virtual void OnStartDrag   ( CImgWnd* pImgWnd, MTI* pmti );
    virtual void OnEndDrag     ( CImgWnd* pImgWnd, MTI* pmti );
    virtual void OnDrag        ( CImgWnd* pImgWnd, MTI* pmti );
    virtual void OnClickOptions( CImgToolWnd* pWnd, const CRect& optionsRect,
                                                    const CPoint& clickPoint );
    virtual void OnShowControlBars(BOOL bShow);

    BOOL    IsSlectionVisible () { return ( m_pCTedit != NULL ); }
    CTedit* GetTextEditField  () { return m_pCTedit; }
    BOOL    FontPaletteVisible();
    void    ToggleFontPalette ();
    void    CloseTextTool     ( CImgWnd* pImgWnd );
    };

#endif // __T_TEXT_H__

