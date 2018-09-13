#ifndef __T_POLY_H__
#define __T_POLY_H__

#include "imgtools.h"

class CPolygonTool : public CClosedFormTool
    {
    DECLARE_DYNAMIC( CPolygonTool )

    protected:

    CObArray m_cObArrayPoints;
    CRect    m_cRectBounding;
    CImgWnd* m_pImgWnd;
    MTI      m_MTI;

    void DeleteArrayContents ( void );
    void AdjustBoundingRect  ( void );
    BOOL CopyPointsToMemArray( CPoint **pcPoint, int *piNumElements );
    void AddPoint            ( POINT ptNewPoint );
    void SetCurrentPoint     ( POINT ptNewPoint );

    virtual void RenderInProgress         ( CDC* pDC );
    virtual void RenderFinal              ( CDC* pDC );
    virtual BOOL SetupPenBrush            ( HDC hDC, BOOL bLeftButton, BOOL bSetup, BOOL bCtrlDown );
    virtual void AdjustPointsForConstraint( MTI *pmti );
    virtual void PreProcessPoints         ( MTI *pmti );

    public:

    CPolygonTool();
    ~CPolygonTool();

    virtual void Render        ( CDC* pDC, CRect& rect, BOOL bDraw, BOOL bCommit, BOOL bCtrlDown );
    virtual void OnEnter       ( CImgWnd* pImgWnd, MTI* pmti );
    virtual void OnLeave       ( CImgWnd* pImgWnd, MTI* pmti );
    virtual void OnActivate    ( BOOL bActivate );
    virtual void OnStartDrag   ( CImgWnd* pImgWnd, MTI* pmti );
    virtual void OnEndDrag     ( CImgWnd* pImgWnd, MTI* pmti );
    virtual void OnDrag        ( CImgWnd* pImgWnd, MTI* pmti );
    virtual void OnCancel      ( CImgWnd* pImgWnd );
    virtual void OnUpdateColors( CImgWnd* pImgWnd );

    virtual void EndMultiptOperation   ( BOOL bAbort = FALSE );
    virtual BOOL CanEndMultiptOperation( MTI* pmti );


    friend class CImgWnd;
    };

#endif // __T_POLY_H__

