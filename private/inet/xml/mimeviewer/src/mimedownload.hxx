/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _MIMEDOWNLOAD_HXX
#define _MIMEDOWNLOAD_HXX

#define MIMEASYNC 1

#include <xmlparser.h>

#if MIMEASYNC

class ViewerFactory;

// HANDLE identifying a particular download processed by the worker thread.
// returned from the WindowMgr (see below).
typedef UINT MDHANDLE;

// When the mime thread executes an action, that action can return one of three values,
// specified below
enum MTARESULT 
{
    MTA_SUCCESS,        // action succeeded
    MTA_FAIL,           // action failed
    MTA_CONTINUE        // action still in progress: stick the action back on the queue for further processing
};

// abstract base class for the general execution unit invoked by the download thread
class MimeThreadAction
{
public:
    MimeThreadAction() { }
    virtual ~MimeThreadAction() { }
public:
    // execute the action
    virtual MTARESULT Run(void) { return MTA_SUCCESS;  }

    // perform cleanup on this action
    virtual void Close(void)  { }

    // perform abnormal cleanup on this action
    virtual void Abort(void)  { }

    virtual void StopAsync(void) {}

    virtual MDHANDLE GetDownloadHandle(void) { return 0; }

};


// first derivation for download actions
class MimeDwnThreadAction : public MimeThreadAction
{
public:
    MimeDwnThreadAction(MDHANDLE mdh);
    virtual ~MimeDwnThreadAction();
public:
    // get the download handle for this beastie
    MDHANDLE GetDownloadHandle(void) { return _mdh; }
    virtual void StopAsync() { _fStopped = TRUE; }
protected:
    MDHANDLE _mdh;
    BOOL  _fStopped;
};


// parser action
class MimeDwnParserAction : public MimeDwnThreadAction
{
public:
    MimeDwnParserAction(MDHANDLE mdh, IXMLParser *pParser, ViewerFactory *pFactory, HANDLE evtLoadComplete);
    virtual ~MimeDwnParserAction();
public:
    // execute the action
    virtual MTARESULT Run(void);

    // perform cleanup on this action
    virtual void Close(void);

    // perform abnormal cleanup on this action
    virtual void Abort(void);

    // clean up all resources associated with this action (on main thread)
    void End(void);
private:
    IXMLParser *_pParser;
    ViewerFactory *_pFactory;
    HANDLE _evtLoadComplete;
    BOOL   _fFirstRun;
};


// queue class used by the download thread
// it's not at all general, probably should be a template
class MimeDwnQueue {
public:
    MimeDwnQueue(CRITICAL_SECTION *cs);
    ~MimeDwnQueue();
public:
    // Add, Remove and Clear are atomic operations protected by a critical section

    // add the action to end the queue
    // will return NULL if it could not add it (out of memory etc).
    // otherwise will return the input argument
    // 
    MimeThreadAction *Add(MimeThreadAction *pMTA);

    // remove the action at the front of the queue
    // if none, will return NULL
    // remove will never shrink down the allocated array (arbitrary decision)
    // This is because often the download thread will normally just push it back on after processing a bit of data.
    MimeThreadAction *Remove(void);

    // Just peek at the first action - but don't actually remove it.
    MimeThreadAction *Peek(void);

    // cause front of queue to be atomically moved to end of queue so caller
    // can get a round-robin load balancing effect.
    void RemoveAdd(void);

    // Causes download for give handle to stop on next iteration.
    void StopAsync(MDHANDLE handle);

    // Clear removes all elements from the queue, aborts them and deletes them
    void Clear(void);
private:
    // array of pointers to parsers
    // This probably grows by some predetermined increment as necessary during add calls
    MimeThreadAction **m_prMTA;
    CRITICAL_SECTION *m_pCS;
    UINT              m_nElems;
    UINT              m_nSize;
};




// For each download initiated from GUI thread we create 
// a hidden window which is used to "marshall" the notification 
// from the worker thread to the gui thread that data is available
class MimeDwnWindowMgr {
public:
    MimeDwnWindowMgr(CRITICAL_SECTION *cs);
    ~MimeDwnWindowMgr();
public:
    // Create a GUI Window for the download
    // return a handle to identify the context
    // return NULL handle if failure
    MDHANDLE AddGUIWnd(void);

    // When someone else also wants to keep the window alive.
    HRESULT AddRefGUIWnd(MDHANDLE h);

    // Get the GUI Window corresponding to the specified gui thread
    // return NULL if none
    HWND GetGUIWnd(MDHANDLE handle);

    // Destroy the GUI Window for this download
    HRESULT ReleaseGUIWnd(MDHANDLE handle);

    // Check for hwnd validity
    BOOL IsGUIWndRegistered(HWND h);
private:
    struct WindInfo {
        HWND _hWnd;
        LONG _ulRefs; // refcount for window
    };
    WindInfo* _prMimeWnd;
    CRITICAL_SECTION *m_pCS;
    UINT    m_nSize;
};



// *************************************************

// global variables which perform the threading and sychronization for the
// download thread

// One download thread per process
// As per win32 these variables will be per process

// Hence we can use critical sections rather than mutexes for access control

// We have two synchronization events
// One to process more data, the other to terminate 
#define MDEVT_PROCESS    0
#define MDEVT_TERMINATE  1
#define MDEVT_LIM        2
extern HANDLE g_MimeDwnEvents[MDEVT_LIM];

// The queue used by the download thread 
extern MimeDwnQueue *g_pMimeDwnQueue;

#define WM_MIMECBFIRST    WM_APP + 10

// message to the hidden window on the gui thread so it can send trident OnDataAvail
// stays out of the way of trident at least...the shell?
#define WM_MIMECBDATA     WM_MIMECBFIRST

// message to hidden window on gui thread to terminate the parser
#define WM_MIMECBEND      WM_MIMECBFIRST + 1

#define WM_MIMECBLAST     WM_MIMECBFIRST + 1

// The array of Windows for each download initiated from a GUI thread
extern MimeDwnWindowMgr *g_pMimeDwnWndMgr;
extern CRITICAL_SECTION g_MimeDwnCSWndMgr;

// *************************************************

// Initialize the global variables above.
// If already initialized, just returns S_OK;

// This will be called typically from the viewerfactory once it gets past the 
// prolog and start to download the heart of the document. It is at that
// point where it tries to download the rest on the other thread.

// No harm calling this each time, if already initialized this just
// returns S_OK
extern HRESULT InitializeMimeDwn(void);


// Close down the subsystem above if at all active
// Terminate the thread by sending an event to it
// Wait for it to return, then close all handles and DeleteCriticalSection
extern void TerminateMimeDwn(void);

// Startup and Kill the download thread
extern HRESULT StartMimeDownloadThread(void);
extern void KillMimeDownloadThread(void);

// return TRUE if the currently executing thread is the mime thread
BOOL IsMimeDownloadThread(void);

// helper function for the GUI thread which will process messages
// while waiting for a signal from the worker thread
// returns index of event which was signaled.
// returns WAIT_OBJECT values from MSDN.
DWORD MsgWaitForDownloadObjects(DWORD count, LPHANDLE lpObjHandle, BOOL fWaitAll, DWORD timeout);

#endif

#endif