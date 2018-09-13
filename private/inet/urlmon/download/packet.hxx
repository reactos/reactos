#ifndef _PACKET_H_
#define _PACKET_H_

#ifdef __cplusplus

//  Packet Manager for Code Downloader

// A Packet is a unit of work that takes time eg. trust verifcation of a piece
// setup of a piece or INF processing of one piece. To be able to have the
// client be responsive with UI and abort capabilty we need to split out work 
// into as small units as possible and queue up these CDLPackets
// CDLPackets get run on a timer per thread.


class CCodeDownload;
class CDownload;

#define CPP_SIGNATURE               0x050395
#define PROCESS_PACKET_INTERVAL     200     // 1/5 of a second

typedef enum {
    PACKET_PRIORITY_NORMAL      = 0,
    PACKET_PRIORITY_HIGH        = 1
} PACKET_PRIORITY;


// use 0xffff0000 for storing msg types for different objs
// if you want to add a msg for a new obj, make a new msg type for that
// obj and then add the obj into the PACKETOBJ union and 
// write a packet constrcutor for that obj
#define MSG_CDOWNLOAD_OBJ       0x00000000
#define MSG_CCODEDOWNLOAD_OBJ   0x00010000

#define GETMSGTYPE(msg)     (msg & 0xffff0000)
#define MAKEMSG(mtype,n)    (mtype|n)

// msgs for CDOWNLOAD
#define CODE_DOWNLOAD_TRUST_PIECE       MAKEMSG(MSG_CDOWNLOAD_OBJ, 1)
#define CODE_DOWNLOAD_PROCESS_PIECE     MAKEMSG(MSG_CDOWNLOAD_OBJ, 2)

// msgs for CCODEDOWNLOAD
#define CODE_DOWNLOAD_PROCESS_INF       MAKEMSG(MSG_CCODEDOWNLOAD_OBJ, 1)
#define CODE_DOWNLOAD_SETUP             MAKEMSG(MSG_CCODEDOWNLOAD_OBJ, 2)
#define CODE_DOWNLOAD_WAIT_FOR_EXE      MAKEMSG(MSG_CCODEDOWNLOAD_OBJ, 3)

typedef union {
    CCodeDownload*  pcdl;
    CDownload*      pdl;
} PACKETOBJ;

class CCDLPacket;

class CCDLPacketMgr {

    public:

    CCDLPacketMgr();
    ~CCDLPacketMgr();

    HRESULT TimeSlice();

    UINT_PTR GetTimer() const {return m_Timer;}

    HRESULT Post(CCDLPacket *pPkt, ULONG pri = PACKET_PRIORITY_NORMAL);

    HRESULT AbortPackets(CDownload *pdl);

    HRESULT Kill(CCDLPacket *pPkt);

    private:

    UINT_PTR            m_Timer;

    //BUGBUG: if we want priority classes then we soudl really have
    // multiple lists!

    CList<CCDLPacket *,CCDLPacket *>
                        m_PacketList;       // linked list of pointers to
                                            // CCDLPacket objects ongoing on
                                            // this thread.A CDLPacket is a unit
                                            // of work that takes time eg.
                                            // trust verifcation of a piece
                                            // setup of a piece or INF
                                            // processing of one piece.
                                            // To be able to have the
                                            // client be responsive with UI
                                            // and abort capabilty we need
                                            // to split out work into as 
                                            // small units as possible
                                            // and queue up these CDLPackets
                                            // CDLPackets get run on a timer per
                                            // thread.
    
};


class CCDLPacket {

    public:

    CCDLPacket(DWORD type, CCodeDownload *pcdl, DWORD_PTR param);
    CCDLPacket(DWORD type, CDownload *pcdl, DWORD_PTR param);

    ~CCDLPacket();

    HRESULT Post(ULONG pri = PACKET_PRIORITY_NORMAL);
    HRESULT Kill();
    HRESULT Process();

    CDownload* GetDownload()
    {
        if (GETMSGTYPE(m_type) == MSG_CDOWNLOAD_OBJ)
            return m_obj.pdl;
        else 
            return NULL;
    }

    CCodeDownload* GetCodeDownload()
    {
        if (GETMSGTYPE(m_type) == MSG_CCODEDOWNLOAD_OBJ)
            return m_obj.pcdl;
        else 
            return NULL;
    }

    private:

    DWORD                   m_signature;
    DWORD_PTR               m_param;
    DWORD                   m_type;

    PACKETOBJ               m_obj;
};

#endif
#endif // _PACKET_H_
