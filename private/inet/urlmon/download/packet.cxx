// ===========================================================================
// File: PACKET.CXX
//
// Packet Manager for Code Downloader
// A Packet is a unit of work that takes time eg. trust verifcation of a piece
// setup of a piece or INF processing of one piece. To be able to have the
// client be responsive with UI and abort capabilty we need to split out work 
// into as small units as possible and queue up these CDLPackets
// CDLPackets get run on a timer per thread.


#include <cdlpch.h>


//+---------------------------------------------------------------------------
//
//  Function:   CDL_PacketProcessProc
//
//  Synopsis:   the timer proc to process packet
//
//  Arguments:  [hWnd] --
//              [WPARAM] --
//              [idEvent] --
//              [dwTime] --
//
//  Returns:
//----------------------------------------------------------------------------
VOID CALLBACK CDL_PacketProcessProc(HWND hWnd, UINT msg, UINT_PTR idEvent, DWORD dwTime)
{
    HRESULT hr = NO_ERROR;
    CUrlMkTls tls(hr); // hr passed by reference!

    Assert(SUCCEEDED(hr));
    Assert(msg == WM_TIMER);
    Assert(idEvent);

    if (SUCCEEDED(hr)) {     // if tls ctor passed above

        Assert(tls->pCDLPacketMgr);

        // give the packet mgr a time slice
        // so a packet can be processed.

        if (tls->pCDLPacketMgr)
            hr = tls->pCDLPacketMgr->TimeSlice();

        Assert(SUCCEEDED(hr));
    }

}

//+---------------------------------------------------------------------------
//
//  Function:   CCDLPacketMgr::CCDLPacketMgr
//----------------------------------------------------------------------------
CCDLPacketMgr::CCDLPacketMgr()
{
    m_Timer = 0;

    m_PacketList.RemoveAll();
}

//+---------------------------------------------------------------------------
//
//  Function:   CCDLPacketMgr::~CCDLPacketMgr
//----------------------------------------------------------------------------
CCDLPacketMgr::~CCDLPacketMgr()
{
    if (m_Timer) {
        KillTimer(NULL, m_Timer);
    }

    m_PacketList.RemoveAll();
}

//+---------------------------------------------------------------------------
//
//  Function:   CCDLPacketMgr::AbortPackets(CDownload *pdl)
//
//  Aborts all packets on the thread that are to do with pdl or
//  its parent codedownload (pdl->GetCodeDownload())
//----------------------------------------------------------------------------
HRESULT 
CCDLPacketMgr::AbortPackets(CDownload *pdl)
{

    HRESULT hr = S_FALSE; //assume none found to be killed
    int iNumPkts;
    LISTPOSITION pos;

    if (!pdl) {
        return  E_INVALIDARG;
    }


    iNumPkts = m_PacketList.GetCount();
    pos = m_PacketList.GetHeadPosition();

    for (int i=0; i < iNumPkts; i++) {

        CCDLPacket *pPkt = m_PacketList.GetNext(pos); // pass ref!

        if ( (pdl == pPkt->GetDownload()) ||
             (pdl->GetCodeDownload() == pPkt->GetCodeDownload()) ) {

            // AbortPackekts is only called from DoSetup. There should
            // normally be no packets left to kill.
            // Assert that this is a NOP.

            UrlMkDebugOut((DEB_CODEDL, "CODE DL:AbortPackets URL:(%ws)\n", pdl->GetURL()));

            Assert(pPkt == NULL);

            Kill(pPkt);

            hr = S_OK;  // indicate killed atleast one

        }
    }

    // here is no more packets in this thread that match CDownload* pdl

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CCDLPacketMgr::Kill(CCDLPacket *pPkt)
//
//  kills packet (removes it from the thread list and deletes it
//----------------------------------------------------------------------------
HRESULT 
CCDLPacketMgr::Kill(CCDLPacket *pPkt)
{

    LISTPOSITION pos = m_PacketList.Find(pPkt);

    m_PacketList.RemoveAt(pos);

    delete pPkt;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   CCDLPacketMgr::Post(CCDLPacket *pPkt, ULONG pri)
//
//  Adds the packet to the list. The packet will get processed
//  on a subsequent timer in the order it appears on the list
//  Also kicks off a timer if none exists already
//----------------------------------------------------------------------------
HRESULT
CCDLPacketMgr::Post(CCDLPacket *pPkt, ULONG pri)
{
    HRESULT hr = S_OK;
    UINT_PTR idEvent = (UINT_PTR) this;

    if (!m_Timer) {
        m_Timer = SetTimer(NULL, idEvent,
                        PROCESS_PACKET_INTERVAL, CDL_PacketProcessProc);

        if (!m_Timer) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
    }

    //BUGBUG: if we want priority classes then we soudl really have
    // multiple lists! This will also affect the order if
    // any sequencing is involved
    if (pri == PACKET_PRIORITY_HIGH) {
        
        m_PacketList.AddHead(pPkt);

    } else {
        
        m_PacketList.AddTail(pPkt);

    }


Exit:

    return hr;

}

//+---------------------------------------------------------------------------
//
//  Function:   CCDLPacketMgr::TimeSlice()
//
//  called from the timer proc.
//  This is like a simple light weight thread machinery that executes/processes
//  one packet per timer msg.
//  Kills the timer if there are no other code downloads on thread.
//----------------------------------------------------------------------------
HRESULT 
CCDLPacketMgr::TimeSlice()
{
    HRESULT hr = S_OK;

    int iNumPkts = m_PacketList.GetCount();

    if (!iNumPkts) {

        // nothing to do. This may happen when processing the previous 
        // packet yields and so we re-enter the timer proc without
        // ever completing the first.

        return hr;
    }

    // have work to do!

    CCDLPacket* pPkt = m_PacketList.RemoveHead();

    Assert(pPkt);

    hr = pPkt->Process();

    // need to refresh this value as the procesing of current pkt
    // may have posted other packets!

    iNumPkts = m_PacketList.GetCount();

    if (!iNumPkts && 
        (CCodeDownload::AnyCodeDownloadsInThread() == S_FALSE)) {

        if (m_Timer) {

            KillTimer(NULL, m_Timer);
            m_Timer = 0;
        }
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   CCDLPacket::CCDLPacket(DWORD type, CDownload *pdl, DWORD param)
//
//  twin constructors that take either CDownload or CCodeDownload obj
//----------------------------------------------------------------------------
CCDLPacket::CCDLPacket(DWORD type, CDownload *pdl, DWORD_PTR param)
{
    m_signature = CPP_SIGNATURE;

    m_type = type;

    m_param = param;

    Assert ((GETMSGTYPE(m_type) == MSG_CDOWNLOAD_OBJ));
    Assert(pdl);

    m_obj.pdl = pdl;
};

//+---------------------------------------------------------------------------
//
//  Function:CCDLPacket::CCDLPacket(DWORD type, CCodeDownload *pcdl,DWORD param)
//----------------------------------------------------------------------------
CCDLPacket::CCDLPacket(DWORD type, CCodeDownload *pcdl, DWORD_PTR param)
{
    m_signature = CPP_SIGNATURE;

    m_type = type;

    m_param = param;

    Assert ((GETMSGTYPE(m_type) == MSG_CCODEDOWNLOAD_OBJ));
    Assert(pcdl);

    m_obj.pcdl = pcdl;
};

//+---------------------------------------------------------------------------
//
//  Function:   CCDLPacket::~CCDLPacket()
//----------------------------------------------------------------------------
CCDLPacket::~CCDLPacket()
{
    m_signature = 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   CCDLPacket::Post(ULONG pri)
//
//  just punts the posting work to the packet mgr.
//----------------------------------------------------------------------------
HRESULT
CCDLPacket::Post(ULONG pri)
{
    HRESULT hr = S_OK;
    CUrlMkTls tls(hr); // hr passed by reference!

    if (FAILED(hr)) {     // if tls ctor failed above
        goto Exit;
    }

    Assert(m_obj.pcdl);

    Assert(tls->pCDLPacketMgr);

    if (tls->pCDLPacketMgr)
        hr = tls->pCDLPacketMgr->Post(this, pri);

Exit:

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CCDLPacket::Process()
//
//  Called from the packet manager's TimeSlice to process packet.
//----------------------------------------------------------------------------
HRESULT
CCDLPacket::Process()
{
    HRESULT hr = S_OK;

    // Code Download messages, arrive in this order
    // First a binding completes and needs to be trusted
    // Next the trusted piece gets processed in ProcessPiece
    // Next if the piece arrived was a CAB and had an INF in it
    // the INF gets processed piece by piece
    // initiating new downloads if necessary or extracting out of
    // already arrived CAB
    // After all pieces are processed and all bindings completed
    // we enter the setup phase, setting up one piece per message.
    // The reason we break it up into some many messages is to make the
    // browser be as responsive during code download and installation as
    // possible and keep the clouds animation smooth.


    Assert(m_signature == CPP_SIGNATURE);

    switch(m_type) {

    case CODE_DOWNLOAD_TRUST_PIECE:
        {
            CDownload *pdl =  m_obj.pdl;

            if (pdl)
                pdl->VerifyTrust();
        }
        break;

    case CODE_DOWNLOAD_PROCESS_PIECE:
        {
            CDownload *pdl =  m_obj.pdl;

            if (pdl)
                pdl->ProcessPiece();
        }
        break;

    case CODE_DOWNLOAD_PROCESS_INF:
        {
            CCodeDownload *pcdl =  m_obj.pcdl;

            if (pcdl)
                pcdl->ProcessInf( (CDownload *) m_param);
        }
        break;

    case CODE_DOWNLOAD_SETUP:
        {
            CCodeDownload *pcdl =  m_obj.pcdl;

            if (pcdl)
                pcdl->DoSetup();
        }
        break;

    case CODE_DOWNLOAD_WAIT_FOR_EXE:
        {
            CCodeDownload *pcdl =  m_obj.pcdl;

            if (pcdl) {
                hr = pcdl->SelfRegEXETimeout();
                Assert(SUCCEEDED(hr));
            }
        }
        break;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   CCDLPacket::Kill()
//----------------------------------------------------------------------------
HRESULT
CCDLPacket::Kill()
{
    HRESULT hr = S_OK;
    CUrlMkTls tls(hr); // hr passed by reference!

    if (FAILED(hr)) {     // if tls ctor failed above
        goto Exit;
    }

    Assert(tls->pCDLPacketMgr);

    if (tls->pCDLPacketMgr)
        hr = tls->pCDLPacketMgr->Kill(this);
Exit:

    return S_OK;
}
