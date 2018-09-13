//+------------------------------------------------------------------------
//
//  File:       unixmodeless.cxx
//
//  Contents:   unix function handling modeless dialog messages
//
//  History:
//
//-------------------------------------------------------------------------

#ifdef NO_MARSHALLING
#include <unixmodeless.hxx>

CUnixModeless g_Modeless;

extern "C"
HRESULT TranslateModelessAccelerator(MSG* pmsg, HWND hWnd)
{
    HWND hTarget = (hWnd ? hWnd : pmsg->hwnd);

    if (hTarget)
    {
        for (CHTMLDlg* pdlg = g_Modeless.GetFirstDlg(); pdlg; ) 
        {
            if (hTarget == pdlg->_hwnd || (hWnd ? 0 : IsChild(pdlg->_hwnd, hTarget)))
            {
                return pdlg->TranslateAccelerator(pmsg);
            } 
            pdlg = g_Modeless.GetNextDlg();
        }
    }
    return S_FALSE;  
}

CUnixModeless::CUnixModeless()
{
    m_iUsed = m_iSize = m_iCurrent = 0;
    m_pHead = NULL;
}

CUnixModeless::~CUnixModeless()
{
    if (m_pHead)
    {
        free(m_pHead);
    }
}

BOOL CUnixModeless::Append(CHTMLDlg* pdlg)
{
    if (m_iUsed==m_iSize)
    {
	    void* p=realloc((void*)m_pHead, sizeof(CHTMLDlg*) * (m_iSize+10));
        if(!p)
        {
            return FALSE;
        } 
	    m_pHead = (CHTMLDlg**)p;
        m_iSize += 10;
    }
    m_pHead[m_iUsed] = pdlg;
    m_iUsed++;
    return TRUE;
} 

BOOL CUnixModeless::Remove(CHTMLDlg* pdlg)
{
    for(int i=0; i<m_iUsed; i++)
    {
        if (pdlg == m_pHead[i])
        {
            for (int j=i; j<m_iUsed-1; j++) // Remove this HWND...
            {
                m_pHead[j] = m_pHead[j+1];
            }
            if (m_iCurrent && m_iCurrent == i)
            {
                m_iCurrent--; // So that GetNext can point to correct HWND
            }
            m_iUsed --;
            return TRUE;
        }
    }
    return FALSE;
}

CHTMLDlg* CUnixModeless::GetFirstDlg()
{
    m_iCurrent = 0;
    return (m_pHead ? m_pHead[0] : NULL);
}

CHTMLDlg* CUnixModeless::GetNextDlg()
{
    if (m_iCurrent + 1 < m_iUsed)
    {
        return m_pHead[ ++m_iCurrent ];
    }
    return NULL;
}            

#endif
