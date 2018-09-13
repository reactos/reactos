
#ifndef _DELAGENT_HXX_
#define _DELAGENT_HXX_
#ifndef unix
#include "Courier.hxx"
#else
#include "courier.hxx"
#endif /* unix */
class CCourierAgent;
class CScheduleAgent;
class CDeliverAgent;

class CDeliverAgentThread;
class CDeliverAgentAsync;
class CDeliverAgentProcess;
class CDeliverAgentGlobal;


extern CMutexSem                               g_mxs;
extern CMap<THREADID , THREADID, HWND , HWND>  g_mapThreadIdToHwnd;
extern CMapCookieToCVal                        g_mapCookieToCDest;
extern CMapCookieToCVal                        g_mapClsToCDest;
extern CMapCookieToCVal                        g_mapPIDToCDest;
extern CDeliverAgentThread     *g_pDeliverAgentThread   ;
extern CDeliverAgentAsync      *g_pDeliverAgentAsync    ;
extern CDeliverAgentProcess    *g_pDeliverAgentProcess  ;
extern CDeliverAgentGlobal     *g_pDeliverAgentMachine  ;

typedef struct _tagDeliverAgentData
{
    CMutexSem                               _mxs;
    CMap<THREADID , THREADID, HWND , HWND>  _mapThreadIdToHwnd;
    CMapCookieToCVal                        _mapCookieToCDest;
    CMapCookieToCVal                        _mapClsToCDest;
    CMapCookieToCVal                        _mapPIDToCDest;
} DELIVERAGENTDATA, *PDELIVERAGENTDATA;


#define DF_VERIFY 0x00000001

class CDeliverAgent : public CCourierAgent
{
public:

    virtual HRESULT HandlePackage(CPackage *pCPackage) = 0;

    HRESULT Register(
        LPNOTIFICATIONSINK  pAcceptor,      // can be null - see mode
        LPDESTID            pDestID,
        NOTFSINKMODE        asMode,
        ULONG               cPackages,
        PNOTIFICATIONTYPE   pPackageIDs,
        PNOTIFICATIONCOOKIE pRegisterCookie,
        DWORD               dwReserved);

    HRESULT Unregister(
        PNOTIFICATIONCOOKIE pRegisterCookie
        );

    // let the following methods be overwriteable
    virtual HRESULT FindFirst(REFDESTID rPackageDest, 
                              REFNOTIFICATIONTYPE rNotfType, 
                              INotificationSink*& rValue,
                              NOTFSINKMODE NotfSinkMode = 0,
                              PNOTIFICATIONCOOKIE pCookie = 0,
                              DWORD dwMode = 0
                              );

    virtual HRESULT FindNext (REFDESTID rPackageDest, 
                              REFNOTIFICATIONTYPE rNotfType, 
                              INotificationSink*& rValue,
                              NOTFSINKMODE NotfSinkMode = 0,
                              PNOTIFICATIONCOOKIE pCookie = 0,
                              DWORD dwMode = 0
                              );

    virtual HRESULT CreateDestination(REFDESTID rPackageDest, INotificationSink*& rValue);

    CDeliverAgent() : CCourierAgent()
                        ,_mxs                (g_mxs                 )
                        ,_mapThreadIdToHwnd  (g_mapThreadIdToHwnd   )
                        ,_mapCookieToCDest   (g_mapCookieToCDest    )
                        ,_mapClsToCDest      (g_mapClsToCDest       )
                        ,_mapPIDToCDest      (g_mapPIDToCDest       )
    {
    }

    virtual ~CDeliverAgent()
    {
    }

public:

private:

protected:
    CMutexSem                               &_mxs;           // single access to all methods for now
    CMap<THREADID , THREADID, HWND , HWND>  &_mapThreadIdToHwnd;
    CMapCookieToCVal                        &_mapCookieToCDest;
    CMapCookieToCVal                        &_mapClsToCDest;
    CMapCookieToCVal                        &_mapPIDToCDest;
}; // CDeliverAgent


class CDeliverAgentThread : public virtual CDeliverAgent
{
public:
    virtual HRESULT HandlePackage(CPackage *pCPackage);

    CDeliverAgentThread() : CDeliverAgent()
    {}

    virtual ~CDeliverAgentThread()
    {}

private:
    HRESULT DispatchOnNotification(CPackage *pCPackage, INotificationSink *pNotificationSink);

};

class CDeliverAgentAsync : public virtual CDeliverAgent
{
public:
    virtual HRESULT HandlePackage(CPackage *pCPackage);

    CDeliverAgentAsync() : CDeliverAgent()
    {}

    virtual ~CDeliverAgentAsync()
    {}
};

class CDeliverAgentProcess : public virtual CDeliverAgent
{
public:
    virtual HRESULT HandlePackage(CPackage *pCPackage);

    CDeliverAgentProcess() : CDeliverAgent()
    {}

    virtual ~CDeliverAgentProcess()
    {}
};

class CDeliverAgentGlobal : public virtual CDeliverAgent
{
public:
    virtual HRESULT HandlePackage(CPackage *pCPackage);

    CDeliverAgentGlobal() : CDeliverAgent()
    {}

    virtual ~CDeliverAgentGlobal()
    {}

};

class CDeliverAgentStart :  public CDeliverAgent
{
public:
    static HRESULT Create(CDeliverAgentStart **pDelAgentStart);

    HRESULT HandlePackage(CPackage *pCPackage);

    CDeliverAgentStart() : CDeliverAgent()
    {
        _pDeliverAgentThread    = g_pDeliverAgentThread  ;
        _pDeliverAgentAsync     = g_pDeliverAgentAsync   ;
        _pDeliverAgentProcess   = g_pDeliverAgentProcess ;
        _pDeliverAgentGlobal    = g_pDeliverAgentMachine ;

    }

    virtual ~CDeliverAgentStart()
    {}

    HRESULT VerifyDestinationPort(THREADID threadId = 0);

private:
    HRESULT FindDeliverAgent(
        CPackage           *pCPackage,
        CDeliverAgent     **ppCourierAgent,
        DWORD               dwFlags);

    HRESULT GetRegisteredDestination(CPackage *pCPackage, CDestination **ppCDestination, DWORD dwMode = 0);
    HRESULT VerifyDestination(CDestination *pCDestination, DWORD dwMode);

private:
    CDeliverAgentThread        *_pDeliverAgentThread;
    CDeliverAgentAsync         *_pDeliverAgentAsync;
    CDeliverAgentProcess       *_pDeliverAgentProcess;
    CDeliverAgentGlobal        *_pDeliverAgentGlobal;

};

#endif // _DELAGENT_HXX_

 

