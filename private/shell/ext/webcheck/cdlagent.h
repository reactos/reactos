#ifndef _CDL_AGENT_HXX_INCLUDED
#define _CDL_AGENT_HXX_INCLUDED

class CDLAgentBSC;

class CCDLAgent : public CDeliveryAgent,
                  public CRunDeliveryAgentSink
{
    private:
        virtual ~CCDLAgent();
    
    public:
        CCDLAgent();

    // virtual functions overriding CDeliveryAgent

    public:
        void        CleanUp();
        HRESULT     AgentAbort(DWORD dwFlags);
        HRESULT     AgentPause(DWORD dwFlags);
        HRESULT     AgentResume(DWORD dwFlags);
   
    protected:
        HRESULT     StartOperation();
        HRESULT     StartDownload();
        HRESULT     ModifyUpdateEnd(ISubscriptionItem *pEndItem, UINT *puiRes);

    public:
        void        SetEndStatus(HRESULT hr) { CDeliveryAgent::SetEndStatus(hr); }
        void        SetErrorEndText(LPCWSTR szErrorText);
        LPWSTR      GetErrorMessage(HRESULT hr);

        HRESULT     StartNextDownload(LPWSTR szCodeBase, DWORD dwSize);
        HRESULT     OnAgentEnd(const SUBSCRIPTIONCOOKIE *, long, HRESULT, LPCWSTR, BOOL);

    private:

        IXMLElement*     m_pSoftDistElement;
        union {
            LPWSTR           m_szCDF;
            LPWSTR           m_szURL;
        };
        LPWSTR           m_szDistUnit;
        SOFTDISTINFO     m_sdi;
        LPWSTR           m_szErrorText;
        DWORD            m_dwVersionMS;
        DWORD            m_dwVersionLS;

        CDLAgentBSC     *m_pCCDLAgentBSC;
        ISoftDistExt    *m_pSoftDistExt;
        
        BOOL             m_bAcceptSoftware;
        BOOL             m_bSendEmail;
        BOOL             m_bSilentMode;

        DWORD            m_dwChannelFlags;
        DWORD            m_dwAgentFlags;
        DWORD            m_dwMaxSizeKB;
        DWORD            m_dwCurSize;
        
        CRunDeliveryAgent *m_pAgent;
};
         
#endif

