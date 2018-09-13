class CFolder
{
    friend class CSnapIn;
    friend class CComponentData;

public:
    // UNINITIALIZED is an invalid memory address and is a good cookie initializer
    CFolder()
    {
        m_cookie = 0;
        m_enumed = FALSE;
        m_pScopeItem = NULL;
        m_extend = FALSE;
        m_pszName = NULL;
    };

    ~CFolder() { delete m_pScopeItem; CoTaskMemFree(m_pszName); };

// Interface
public:
    BOOL IsEnumerated() { return  m_enumed; };
    void Set(BOOL state) { m_enumed = state; };
    void SetCookie(MMC_COOKIE cookie) { m_cookie = cookie; }
    BOOL GetType() { return m_extend; };
    BOOL operator == (const CFolder& rhs) const { return rhs.m_cookie == m_cookie; };
    BOOL operator == (long cookie) const { return cookie == m_cookie; };

// Implementation
private:
    void Create(LPWSTR szName, int nImage, int nOpenImage, BOOL bExtension);

// Attributes
private:
    LPSCOPEDATAITEM m_pScopeItem;
    MMC_COOKIE      m_cookie;
    BOOL            m_enumed;
    BOOL            m_extend;
    LPOLESTR        m_pszName;
};
