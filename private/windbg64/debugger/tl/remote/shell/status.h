
class CWindbgrmFeedback {
protected:
    // Connected, Debuggee, TL, Remote info (todo)
    enum { MAX_NUM_STRINGS = 3 };

    PSTR                m_rgpsz[MAX_NUM_STRINGS];
    SIZE                m_rgsize[MAX_NUM_STRINGS];
    BOOL                m_rgbChanged[MAX_NUM_STRINGS];

    CIndiv_TL_RM_WKSP   m_CTl;
    PSTR                m_pszClientName;

    BOOL                m_bCurrentlyPainting;
    BOOL                m_bCurrentlyUpdating;

    CRITICAL_SECTION    m_cs;


public:
    CWindbgrmFeedback();
    virtual ~CWindbgrmFeedback();

    void UpdateInfo(HWND);
    void PaintConnectionStatus(HWND, HDC);
    
    void SetTL(CIndiv_TL_RM_WKSP * pCTl);
    void SetClientName(PSTR pszClientName);

protected:
    void HelperSetString(HDC hdc, PSTR & pszNewText, 
        PSTR & pszOldText, SIZE & sizeTextExtent, 
        BOOL & bChanged);
};


extern CWindbgrmFeedback g_CWindbgrmFeedback;