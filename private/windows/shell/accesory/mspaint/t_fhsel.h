#ifndef __T_FHSEL_H__
#define __T_FHSEL_H__

#include "imgtools.h"
#include "t_poly.h"

class CFreehandSelectTool : public CPolygonTool
    {
    DECLARE_DYNAMIC(CFreehandSelectTool)

    protected:

    CRgn *m_pcRgnPoly;
    CRgn *m_pcRgnPolyBorder;

    int  m_iNumPoints;

    void AdjustPointsForZoom( int iZoom );
    BOOL CreatePolyRegion   ( int iZoom );
    BOOL CreatePolyRegion   ( int iZoom, LPPOINT lpPoints, int iPoints );

    virtual BOOL SetupPenBrush( HDC hDC, BOOL bLeftButton, BOOL bSetup, BOOL bCtrlDown );
    virtual void AdjustPointsForConstraint( MTI *pmti );
    virtual void PreProcessPoints( MTI *pmti );

    virtual BOOL IsToolModal(void);

    public:

    CFreehandSelectTool();
    ~CFreehandSelectTool();

    BOOL ExpandPolyRegion( int iNewSizeX, int iNewSizeY );

    virtual void OnPaintOptions ( CDC* pDC, const CRect& paintRect,
                                  const CRect& optionsRect );
    virtual void OnClickOptions ( CImgToolWnd* pWnd, const CRect& optionsRect,
                                                     const CPoint& clickPoint );



    virtual void OnStartDrag( CImgWnd* pImgWnd, MTI* pmti );
    virtual void OnEndDrag  ( CImgWnd* pImgWnd, MTI* pmti );
    virtual void OnDrag     ( CImgWnd* pImgWnd, MTI* pmti );
    virtual void OnCancel   ( CImgWnd* pImgWnd );
    virtual void OnActivate ( BOOL bActivate );

    virtual BOOL CanEndMultiptOperation( MTI* pmti );

    friend class CImgWnd;
    };


#endif // __T_FHSEL_H__


