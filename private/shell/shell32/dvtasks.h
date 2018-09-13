#ifndef _DVTASKS_H
#define _DVTASKS_H

#include <runtask.h>

class CDefView;
class CDVGetIconTask;

HRESULT CDVbkgrndEnumTask_CreateInstance( CDefView * pdsv, IEnumIDList *peunk,
    HDPA hdpaNew, BOOL fRefresh, LPRUNNABLETASK * ppTask );
    
HRESULT CDVGetIconTask_CreateInstance( CDefView * pdsv, LPCITEMIDLIST pdl, LPRUNNABLETASK *ppTask, CDVGetIconTask ** ppObject );

/////////////////////////////////////////////////////////////////////////////////////
// task used to perform the background enumeration
class CDVBkgrndEnumTask : public CRunnableTask
{
    public:
        virtual STDMETHODIMP RunInitRT( void );
        virtual STDMETHODIMP InternalResumeRT( void );

    protected:
        CDVBkgrndEnumTask( HRESULT * pHr, 
                     CDefView * pdsv, 
                     IEnumIDList * peunk, 
                     HDPA hdpaNew, 
                     BOOL fRefresh );
        ~CDVBkgrndEnumTask();

        CDefView * _pdsv;
        IEnumIDList * _peunk;
        HDPA _hdpaNew;
        BOOL _fRefresh;

        friend HRESULT CDVBkgrndEnumTask_CreateInstance( CDefView * pdsv, IEnumIDList * peunk,
            HDPA hdpaNew, BOOL fRefresh, LPRUNNABLETASK * ppTask );
};

// task used to do a background icon extraction......
class CDVGetIconTask : public CRunnableTask
{
    public: // data 
        
        LPITEMIDLIST    _pidl;
        int             _iIcon;
        BITBOOL         _fUsed : 1;
        
    public:
        STDMETHODIMP RunInitRT( void );

    protected:
        CDVGetIconTask(HRESULT * pHr, LPCITEMIDLIST pidl, CDefView * pdsv);
        ~CDVGetIconTask();

        CDefView * _pdsv;

        friend HRESULT CDVGetIconTask_CreateInstance( CDefView * pdsv, LPCITEMIDLIST pdl, LPRUNNABLETASK *ppTask, CDVGetIconTask ** ppObject );
};

class CDVExtendedColumnTask : public CRunnableTask
{
    public:
        STDMETHODIMP RunInitRT(void);

    protected:
        CDVExtendedColumnTask(CDefView * pdsv, LPCITEMIDLIST pidl, int fmt, UINT uiColumn);
        ~CDVExtendedColumnTask();

        CDefView *              _pdsv;
              LPCITEMIDLIST     _pidl;
        const int               _fmt;
        const UINT              _uiCol;

        friend HRESULT CDVExtendedColumnTask_CreateInstance(CDefView * pdsv, LPCITEMIDLIST pidl, int fmt, UINT uiColumn, IRunnableTask **ppTask);
};

// task used to do a background icon overlay look up and extraction......
class CDVIconOverlayTask : public CRunnableTask
{
    public:
        STDMETHODIMP RunInitRT( void );

    protected:

        LPITEMIDLIST    _pidl;
        int _iList;
        
        CDVIconOverlayTask(HRESULT * pHr, LPCITEMIDLIST pidl, int iList, CDefView * pdsv);
        ~CDVIconOverlayTask();

        CDefView * _pdsv;
        friend HRESULT CDVIconOverlayTask_CreateInstance( CDefView * pdsv, LPCITEMIDLIST pdl, int iList, LPRUNNABLETASK *ppTask );
};

#endif

