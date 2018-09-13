/*----------------------------------------------------------------------------
/ Title;
/   command.cpp
/
/ Authors;
/   Rick Turner (ricktu)
/
/ Notes;
/   Implements IOleCommandTarget for My Documents code.
/----------------------------------------------------------------------------*/
#include "precomp.hxx"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ CMyDocsCommand
/   This is the My Documents IOleCommandTarget implementation.
/----------------------------------------------------------------------------*/

CMyDocsCommand::CMyDocsCommand( )
{
    TraceEnter(TRACE_COMMAND, "CMyDocsCommand::CMyDocsCommand");

    TraceLeave();
}

CMyDocsCommand::~CMyDocsCommand()
{
    TraceEnter(TRACE_COMMAND, "CMyDocsCommand::~CMyDocsCommand");

    TraceLeave();
}


#undef CLASS_NAME
#define CLASS_NAME CMyDocsCommand
#include "unknown.inc"

STDMETHODIMP
CMyDocsCommand::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[] =
    {
        &IID_IOleCommandTarget, (IOleCommandTarget *)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}


/*-----------------------------------------------------------------------------
/ IOleCommandTarget
/----------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsCommand::QueryStatus( const GUID *pguidCmdGroup,
                             ULONG cCmds,
                             OLECMD prgCmds[],
                             OLECMDTEXT *pCmdText
                            )
{
    HRESULT hr = OLECMDERR_E_UNKNOWNGROUP;

    TraceEnter(TRACE_COMMAND, "CMyDocsCommand::QueryStatus");

    if (IsEqualGUID(*pguidCmdGroup, CGID_ShellServiceObject))
    {
        // We like Shell Service Object notifications...
        hr = S_OK;
    }

    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsCommand::Exec( const GUID *pguidCmdGroup,
                      DWORD nCmdID,
                      DWORD nCmdexecopt,
                      VARIANTARG *pvaIn,
                      VARIANTARG *pvaOut
                     )
{
    HRESULT hr = E_NOTIMPL;

    TraceEnter(TRACE_COMMAND, "CMyDocsCommand::Exec");

    if (IsEqualGUID(*pguidCmdGroup, CGID_ShellServiceObject))
    {
        // Handle Shell Service Object notifications here.
        switch (nCmdID)
        {
            case SSOCMDID_OPEN:
                Trace(TEXT("Called w/OPEN"));
                hr = _CheckPerUserSettings();
                break;

            case SSOCMDID_CLOSE:
                Trace(TEXT("Called w/CLOSE"));
                hr = _RemovePerUserSettings();
                break;

            default:
                hr = S_OK;
                break;
        }
    }

    TraceLeaveResult( hr );
}


