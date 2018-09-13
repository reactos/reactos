#include "main.h"

void CFolder::Create(LPWSTR szName, int nImage, int nOpenImage, BOOL bExtension)
{

    // Two-stage construction
    m_pScopeItem = new SCOPEDATAITEM;
    memset(m_pScopeItem, 0, sizeof(SCOPEDATAITEM));

    // Set folder type 
    m_extend = bExtension;

    // Add node name
    if (szName != NULL)
    {
        m_pScopeItem->mask = SDI_STR;
        m_pScopeItem->displayname = (unsigned short*)(-1);
        
        UINT uiByteLen = (lstrlen(szName) + 1) * sizeof(OLECHAR);
        LPOLESTR psz = (LPOLESTR)::CoTaskMemAlloc(uiByteLen);
    
        if (psz != NULL)
        {
            lstrcpy(psz, szName);
            m_pszName = psz;
        }
    }

    // Add close image
    if (nImage != -1)
    {
        m_pScopeItem->mask |= SDI_IMAGE;
        m_pScopeItem->nImage = nImage;
    }

    // Add open image
    if (nOpenImage != -1)
    {
        m_pScopeItem->mask |= SDI_OPENIMAGE;
        m_pScopeItem->nOpenImage = nOpenImage;
    }
}
