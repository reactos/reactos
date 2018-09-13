#ifndef __SPRITE_H__
#define __SPRITE_H__

// Draggers are graphical objects that typicaly overlay windows and
// can be shown, hidden, and moved.  A default dragger is simply a 
// dotted rectangle XOR'd over the contents of it's window.
//
// For bitmap draggers, use the CSprite class.
//
// CDragger() creates hidden draggers.
//
// ~CDragger() will hide a dragger before it is destroyed.
//
// Move() and SetSize() will make sure a dragger gets erased from 
// it's old position and if it was visible, it will be drawn in 
// it's new position.
//

class CDragger : public CObject
    {
    public:

    enum STATE 
        {
        hidden,
        shown,
        obscured
        };

             CDragger   () : m_rect(), m_pWnd() { m_state = hidden; };
             CDragger   ( CWnd* pWnd, CRect* pRect = NULL );
    virtual ~CDragger   ();

    virtual void Hide   ();
    virtual void Show   ();
    virtual void Obscure(BOOL bObscure);
    
    virtual void Draw   ();
    virtual void Erase  ();
    virtual void Move   ( const CRect& newRect, BOOL bForceShow = FALSE );
    virtual void Move   ( const CPoint& newTopLeft, BOOL bForceShow = FALSE );
            void MoveBy ( int cx, int cy, BOOL bForceShow = FALSE );
            void SetSize( const CSize& newSize, BOOL bForceShow = FALSE );

    virtual CObList* GetDraggerList() { return NULL; }
    virtual CRect    GetRect() const;
    inline  BOOL     IsShown() const { return m_state == shown; }
    
    CRect m_rect;
    STATE m_state;
    CWnd* m_pWnd;

    #ifdef _DEBUG
    DECLARE_DYNAMIC( CDragger )
    #endif
    };


class CMultiDragger : public CDragger
    {
    public:

             CMultiDragger();
             CMultiDragger(CWnd* pWnd);
    virtual ~CMultiDragger();

    virtual void Hide();
    virtual void Show();
    virtual void Draw();
    virtual void Erase();
    virtual void Move(const CPoint& newTopLeft, BOOL bForceShow = FALSE);
    
    virtual CRect GetRect() const;

    void Add   (CDragger *pDragger);
    void Remove(CDragger *pDragger);

    virtual CObList* GetDraggerList() { return &m_draggerList; }

    CObList m_draggerList;

    #ifdef _DEBUG
    DECLARE_DYNAMIC( CMultiDragger )
    #endif
    };

class CSprite : public CDragger
    {
    public:

    CSprite();
    CSprite(CWnd* pWnd, CRect* pRect = NULL);
    virtual ~CSprite();
    
    virtual void Move(const CRect&, BOOL = FALSE);
    inline  void Move(const CPoint& newTopLeft) 
                { CDragger::Move(newTopLeft); }
    virtual void Draw() = 0;
    virtual void SaveBits();
    virtual void Erase();
    
    CBitmap m_saveBits;

    #ifdef _DEBUG
    DECLARE_DYNAMIC( CSprite )
    #endif
    };

class CHighlight : public CDragger
    {
    public:

     CHighlight();
     CHighlight(CWnd *pWnd, CRect* pRect = NULL, int bdrSize = 2);
    ~CHighlight();

    int m_bdrSize;

    virtual void Draw();
    virtual void Erase();
    };

#endif
