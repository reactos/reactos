//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       schagent.hxx
//
//  Contents:   schedule agent class
//
//  Classes:
//
//  Functions:
//
//  History:    2-20-1997   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------

#ifndef _SCHAGENT_HXX_
#define _SCHAGENT_HXX_

#ifndef unix
#include "DelAgent.hxx"
#include "SortList.hxx"
#else
#include "delagent.hxx"
#include "sortlist.hxx"
#endif /* unix */

inline CFileTime GetCurrentDtTime()
{
   SYSTEMTIME      st;
   CFileTime       curdt;

   GetLocalTime(&st);
   SystemTimeToFileTime(&st, &curdt);
   return curdt;
}


class CSchedListAgent;
class CDelayListAgent;
class CThrottleListAgent;

extern CSchedListAgent          *g_pCSchedListAgent;
extern CDelayListAgent          *g_pCDelayListAgent;
extern CDeliverAgentStart       *g_pDeliverAgent;
extern CThrottleListAgent           *g_pCIdleListAgent;

//internal task trigger types
#define TASK_TIME_TRIGGER_NOW      (TASK_TRIGGER_TYPE)1024
#define TASK_TIME_TRIGGER_DELAYED  (TASK_TRIGGER_TYPE)1025
typedef enum _WAKEUPTYPE
{
     WT_NONE        = 0
    ,WT_DELAY       = 1
    ,WT_SCHED       = 2
    ,WT_SCHEDGROUP
    ,WT_USERIDLE
    ,WT_NEWITEM
    ,WT_NEXTITEM
    ,WT_TIMECHANGE

} WAKEUPTYPE;

typedef enum _tagListMode
{
    LM_LOCALCOPY = 0x00000001
    
} _ListMode;

typedef DWORD LISTMODE;

class CListAgent : public CCourierAgent
{

public:
    virtual HRESULT HandlePackage(CPackage    *pCPackage) = 0;

    virtual HRESULT RevokePackage(PNOTIFICATIONCOOKIE packageCookie,
                                  CPackage          **ppCPackage,
                                  DWORD               dwMode
                                  ) = 0;

    virtual HRESULT OnWakeup(WAKEUPTYPE wt, CPackage *pCPackage = 0) = 0;

    virtual HRESULT FindFirstPackage(PNOTIFICATIONCOOKIE packageCookie,
                                     CPackage          **ppCPackage,
                                     DWORD               dwMode
                                     )
    {
        return E_FAIL;
    }

    virtual HRESULT FindNextPackage(PNOTIFICATIONCOOKIE packageCookie,
                                    CPackage          **ppCPackage,
                                    DWORD               dwMode
                                    )
    {
        return E_FAIL;
    }

    virtual HRESULT FindPackage(PNOTIFICATIONCOOKIE packageCookie,
                                CPackage          **ppCPackage,
                                DWORD               dwMode
                                )
    {
        return E_FAIL;
    }

    virtual HRESULT GetPackageCount(ULONG          *pCount)
    {
        return E_FAIL;
    }

    CListAgent() : CCourierAgent()
    {
        _Timer = 0;
        _pDeliverAgent = g_pDeliverAgent;
    }

    void Uninitialize()
    {
        if (_Timer)
        {
            KillTimer(NULL, _Timer);
            _Timer = 0;
        }
    }
    
    virtual ~CListAgent()
    {}

private:
    virtual HRESULT SetWakeup()
    {
        return E_FAIL;
    }

protected:
    CMutexSem           _mxs;           // single access to the timer
    UINT                _Timer;
    WAKEUPTYPE          _wt;
    CDeliverAgentStart *_pDeliverAgent;
    
}; // CListAgent

//#include "SortXLst.hxx"
#ifndef unix
#include "SortPkgL.hxx"
#else
#include "sortpkgl.hxx"
#endif /* unix */

class CDelayListAgent : public CListAgent
{
public:
    #define SCH_DELAY_INTERVAL 5000         // 5sec

    virtual HRESULT HandlePackage(
                                  CPackage    *pCPackage);

    virtual HRESULT RevokePackage(
                                  PNOTIFICATIONCOOKIE packageCookie,
                                  CPackage          **ppCPackage,
                                  DWORD               dwMode
                                  );

    virtual HRESULT OnWakeup(WAKEUPTYPE wt, CPackage *pCPackage = 0);

    CDelayListAgent() : CListAgent(), _cElements(0)
    {
        _wt = WT_DELAY;
    }
    virtual ~CDelayListAgent();

private:
    virtual HRESULT SetWakeup();
    static VOID CALLBACK WakeupProc(HWND hWnd, UINT msg, UINT dwWhat, DWORD dwTime);

private:
    CSortList<DWORD, CPackage *, CPackage *>    _XSortList;
    CRefCount                                   _cElements;
};  // CDelayListAgent

class CSchedListAgent : public CListAgent
{
public:
    virtual HRESULT HandlePackage(
                                  CPackage    *pCPackage);

    virtual HRESULT RevokePackage(
                                  PNOTIFICATIONCOOKIE packageCookie,
                                  CPackage          **ppCPackage,
                                  DWORD               dwMode
                                  );

    virtual HRESULT OnWakeup(WAKEUPTYPE wt, CPackage *pCPackage = 0);

    virtual HRESULT FindFirstPackage(
                                     PNOTIFICATIONCOOKIE packageCookie,
                                     CPackage          **ppCPackage,
                                     DWORD               dwMode
                                     );

    virtual HRESULT FindNextPackage(
                                    PNOTIFICATIONCOOKIE packageCookie,
                                    CPackage          **ppCPackage,
                                    DWORD               dwMode
                                    );

    virtual HRESULT FindPackage(
                                PNOTIFICATIONCOOKIE packageCookie,
                                CPackage          **ppCPackage,
                                DWORD               dwMode
                                );


    // finding group packages
    virtual HRESULT FindFirstGroupPackage(
                                     PNOTIFICATIONCOOKIE packageCookie,
                                     CPackage          **ppCPackage,
                                     DWORD               dwMode
                                     );

    virtual HRESULT FindNextGroupPackage(
                                    PNOTIFICATIONCOOKIE packageCookie,
                                    CPackage          **ppCPackage,
                                    DWORD               dwMode
                                    );

    virtual HRESULT FindGroupPackage(
                                PNOTIFICATIONCOOKIE packageCookie,
                                CPackage          **ppCPackage,
                                DWORD               dwMode
                                );


    virtual HRESULT GetPackageCount(
                                   ULONG          *pCount
                                   );

    virtual HRESULT SetWakeup();

    STDMETHODIMP Load(
                      PNOTIFICATIONCOOKIE packageCookie,
                      IStorage *pStg,
                      DWORD     dwMode
                      );

    STDMETHODIMP Save(
                      PNOTIFICATIONCOOKIE packageCookie,
                      IStorage *pStg,
                      DWORD     dwMode
                      );

    STDMETHODIMP GetSizeMax(ULARGE_INTEGER *pcbSize);

    STDMETHODIMP Resync();
    
    CListAgent *GetListAgent(DWORD grfFlags, BOOL fAddRef = FALSE)
    {
        CListAgent *pCPRet = 0;
        pCPRet = &_XSortList;
        
        if (fAddRef && pCPRet)
        {
            pCPRet->AddRef();
        }

        return pCPRet;
    }

    CSchedListAgent()
        : CListAgent()
        , _XSortList(*g_pGlobalNotfMgr, c_pszRegKey)
    {
        _wt = WT_SCHED;
        _dateNextRun = MAX_FILETIME;
    }
    virtual ~CSchedListAgent();

private:
    static VOID CALLBACK WakeupProc(HWND hWnd, UINT msg, UINT dwWhat, DWORD dwTime);

    HRESULT HandleTimeChange(DWORD dwMode = 0);


protected:
    CSortPkgList    _XSortList;
    CFileTime       _dateNextRun;
}; //CSchedListAgent


class CScheduleAgent : public CCourierAgent
{
public:
    virtual HRESULT HandlePackage(CPackage    *pCPackage);

    virtual HRESULT RevokePackage(PNOTIFICATIONCOOKIE packageCookie,
                                  CPackage          **ppCPackage,
                                  DWORD               dwMode
                                  );

    virtual HRESULT FindFirstPackage(PNOTIFICATIONCOOKIE packageCookie,
                                     CPackage          **ppCPackage,
                                     DWORD               dwMode
                                     )
    {
        return _pSchedListAgent->FindFirstPackage(packageCookie, ppCPackage, dwMode);
    }

    virtual HRESULT FindNextPackage(PNOTIFICATIONCOOKIE packageCookie,
                                    CPackage          **ppCPackage,
                                    DWORD               dwMode
                                    )
    {
        return _pSchedListAgent->FindNextPackage(packageCookie, ppCPackage, dwMode);
    }

    virtual HRESULT FindPackage(PNOTIFICATIONCOOKIE packageCookie,
                                CPackage          **ppCPackage,
                                DWORD               dwMode
                                )
    {
        return _pSchedListAgent->FindPackage(packageCookie, ppCPackage, dwMode);
    }

    virtual HRESULT GetPackageCount(ULONG          *pCount)
    {
       return _pSchedListAgent->GetPackageCount(pCount);
    }

    STDMETHODIMP Load(PNOTIFICATIONCOOKIE packageCookie,
                      IStorage *pStg,
                      DWORD     dwMode
                      )
    {
        return _pSchedListAgent->Load(packageCookie,pStg,dwMode);
    }
    
    STDMETHODIMP Save(PNOTIFICATIONCOOKIE packageCookie,
                      IStorage *pStg,
                      DWORD     dwMode
                      )
    {
        return _pSchedListAgent->Save(packageCookie,pStg,dwMode);
    }
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER *pcbSize)
    {
        return _pSchedListAgent->GetSizeMax(pcbSize);
    }

    virtual HRESULT OnWakeup(WAKEUPTYPE wt, CPackage *pCPackage = 0);

    CSchedListAgent *GetSchedListAgent(BOOL fAddRef = FALSE)
    {

        if (fAddRef && _pSchedListAgent)
        {
            _pSchedListAgent->AddRef();
        }

        return _pSchedListAgent;
    }
    
    CListAgent *GetListAgent(DWORD grfFlags, BOOL fAddRef = FALSE)
    {
        return _pSchedListAgent->GetListAgent(grfFlags, fAddRef);
    }

    CScheduleAgent() : CCourierAgent()
    {
        _pSchedListAgent = g_pCSchedListAgent;
        _pDelayListAgent = g_pCDelayListAgent;
        _pIdleListAgent  = g_pCIdleListAgent;
        //NotfAssert(( _pIdleListAgent && _pDelayListAgent && _pSchedListAgent));
        NotfAssert(( _pDelayListAgent && _pSchedListAgent));
    }
    virtual ~CScheduleAgent()
    {}

private:
    CMutexSem           _mxs;           // single access to the timer

    CDelayListAgent *_pDelayListAgent;
    CSchedListAgent *_pSchedListAgent;
    CThrottleListAgent  *_pIdleListAgent;
}; // CScheduleAgent


// helper functions
#ifdef unix
// If we did not have this here then the urlmon.map file will complain that
// it cant export GetRunTimes as in urlmon.def.
EXTERN_C
#endif /* unix */
HRESULT CheckNextRunDate(PTASK_TRIGGER pTaskTrigger, PTASK_DATA pTaskData,CFileTime *pDate);


HRESULT ValidateTrigger(TASK_TRIGGER *ptt);
HRESULT ValidateTaskData(TASK_DATA *ptd);

#define RANDOM_TIME_START       0       // 12am (in minutes)
#define RANDOM_TIME_END         300     // 5am (in minutes)
#define RANDOM_TIME_INC         30      // 30min increment

DWORD GetRandTime(DWORD wStartMins, DWORD wEndMins, DWORD wInc);


#endif // _SCHAGENT_HXX_

 

