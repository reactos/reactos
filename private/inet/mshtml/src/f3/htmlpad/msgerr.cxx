//
//  MSGERR.CPP
//
// implementation file for CLastError which implements the
// GetLastError helper
//
//  
//
// Copyright (C) 1995 Microsoft Corp.
//

//#define STRICT

#include "padhead.hxx"

#ifndef X_PAD_HXX_
#define X_PAD_HXX_
#include "pad.hxx"
#endif

#ifndef X_PADRC_H_
#define X_PADRC_H_
#include "padrc.h"
#endif

#ifndef X_MSGERR_HXX_
#define X_MSGERR_HXX_
#include "msgerr.hxx"
#endif

const int       CchMaxErrorMessage = 256;

extern const char  SzNull[] = "";
char szErrUnknown[] = "Error description is not available";

//
// some stuff to put a "Help" button on the error msgbox
//
#if defined(_WIN32)
char szHelpFile[_MAX_PATH];
VOID CALLBACK ErrorBoxCallBack(LPHELPINFO lpHelpInfo);
#endif //_WIN32

static int iFromHR(HRESULT hr)
{
    switch(GetScode(hr)) {
    case MAPI_E_NOT_ENOUGH_MEMORY:      return IDS_E_OUTOFMEMORY;
    case MAPI_E_INVALID_PARAMETER:      return IDS_INVALID_ARGUMENT;
    case MAPI_E_INVALID_OBJECT:         return IDS_INVALID_OBJECT;
    case MAPI_E_INTERFACE_NOT_SUPPORTED: return IDS_INTERFACE_NOT_SUPPORTED;
    case MAPI_E_NO_ACCESS:              return IDS_ACCESS_DENIED;
    case MAPI_E_NO_SUPPORT:             return IDS_NOT_SUPPORTED;
    case MAPI_E_BAD_CHARWIDTH:          return IDS_INVALID_CHARWIDTH;
    case MAPI_E_NOT_FOUND:              return IDS_NOT_FOUND;
    case MAPI_E_CALL_FAILED:            return IDS_CALL_FAILED;
    case MAPI_E_USER_CANCEL:            return IDS_USER_CANCEL;
    case MAPI_W_ERRORS_RETURNED:        return IDS_ERRORS_RETURNED;
    case MAPI_E_UNKNOWN_FLAGS:          return IDS_UNKNOWN_FLAGS;
    case E_UNEXPECTED:                  return IDS_UNEXPECTED;
    case OLEOBJ_S_CANNOT_DOVERB_NOW:    return IDS_CANTNOW;
    // if it's not in this list you need to add it.
    default:
        //DebugTrace(TEXT ("lasterr: bad arg to FORMScodeFromHR"));
        Assert(FALSE);
        return 0;
    }
}

HRESULT CLastError::Init(LPCTSTR szComponent)
{
    HRESULT hr;
    LONG cch;
    
    _eLastErr = eNoError;
    _hrLast = 0;
    _hrGLE = 0;
    _pmapierr = 0;
    _szComponent = NULL;

    cch = lstrlen(szComponent) + 1;

    hr = THR(MAPIAllocateBuffer(cch, (LPVOID *) &_szComponent));
    if(hr)
        goto Cleanup;

    WideCharToMultiByte(CP_ACP, 0, szComponent, cch, _szComponent, cch + 1, NULL, NULL);

Cleanup:
    RRETURN(hr);
}
    
CLastError::~CLastError()
{
    if (_pmapierr != NULL)
    {
        MAPIFreeBuffer(_pmapierr);
    }
    if(_szComponent != NULL)
    {
        MAPIFreeBuffer(_szComponent);
    }
}

HRESULT CLastError::SetLastError(HRESULT hr)
{
#if defined(DEBUG)
    //
    //  Ensure that the error string exists -- when we set it not when
    //  they ask for it.
    //

    (void) iFromHR(hr);
#endif

    //
    //  Release any previous error
    //

    if (_pmapierr != NULL) {
        MAPIFreeBuffer(_pmapierr);
        _pmapierr = NULL;
    }

    if (hr) {
        _eLastErr = eMAPI;
    }
    else {
        _eLastErr = eNoError;
    }

    return (_hrLast = hr);
}

HRESULT CLastError::SetLastError(HRESULT hr, IUnknown* punk)
{
    Assert(punk && hr);     // we have to have an object and an error.

    _eLastErr = eObject;
    _hrLast = hr;

    IMAPIProp* pmprp = (IMAPIProp*)punk;  // I hate this cast but c'est la vie.

    MAPIFreeBuffer(_pmapierr);     // clean up previous error.
    _pmapierr = NULL;

    _hrGLE = pmprp->GetLastError(hr, 0, &_pmapierr);
    if (_hrGLE == S_OK) {
        if (_pmapierr == NULL) {
            if (MAPIAllocateBuffer(sizeof(MAPIERROR), (void **) &_pmapierr)) {
                _hrGLE = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
            }
            else {
                memset(_pmapierr, 0, sizeof(MAPIERROR));
                _pmapierr->ulVersion = MAPI_ERROR_VERSION;
                _pmapierr->lpszError = (LPTSTR)szErrUnknown;
                _pmapierr->lpszComponent =  (LPTSTR)SzNull;
            }
        }
        else if (_pmapierr->lpszError == NULL) {
            _pmapierr->lpszError = (LPTSTR)SzNull;
        }
        else if (_pmapierr->lpszComponent == NULL) {
            _pmapierr->lpszComponent = (LPTSTR)SzNull;
        }
    }
    else {
        if (_pmapierr != NULL) {
            MAPIFreeBuffer(_pmapierr);
            _pmapierr = NULL;
        }
    }
    return _hrLast;
}

HRESULT CLastError::GetLastError(HRESULT hr, DWORD dwFlags,
                                   LPMAPIERROR FAR * lppMAPIError)
{
    //
    //  Start with parameter validation
    //

    if (IsBadWritePtr(lppMAPIError, sizeof(LPMAPIERROR))) {
        return SetLastError(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (MAPI_UNICODE == (dwFlags & MAPI_UNICODE)) {
        return SetLastError(ResultFromScode(MAPI_E_BAD_CHARWIDTH));
    }

    //
    //  Is the error asked for the last error registered with us?
    //

    if (hr != _hrLast) {
        *lppMAPIError = NULL;
        return S_OK;
    }

    int         cch;
    int         cb;
    int         idsError;
    char*       szMessage = 0;
    char*       szComponent = 0;
    char        szErrorString[CchMaxErrorMessage];
    LPMAPIERROR pmapierr = NULL;

    //
    //  Based on the type of the last error, construct the appropriate
    //  return object
    //

    switch (_eLastErr) {
    case eMAPI:
        //
        //  The last error registered was a MAPI error code.  For mapi
        //      error codes we map the MAPI error code into a resource
        //      id and return the appropriate string.
        //
        // as to spec, we allocate a single buffer for message and
        //      component.  no one will notice that we aren't doing
        //      MAPIAllocateMore for component.
        //
        //   We make an assumption as to the maximum possible length
        //      of the two strings combined.
        //

        Assert(_pmapierr == NULL);
        if (MAPIAllocateBuffer(CchMaxErrorMessage + sizeof(MAPIERROR),
                               (void**)&pmapierr)) {
            return ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        }

        //
        //  Set the version number
        //

        pmapierr->ulVersion = MAPI_ERROR_VERSION;
        pmapierr->ulLowLevelError = 0;

        //
        //   do the maping from the MAPI error code into a FORM string
        //      value.  The FORM eror code code will be set as the low
        //      level error value.
        //

        idsError = iFromHR(_hrLast);
        pmapierr->ulContext = idsError;

        //
        //  Set the error string pointer to the appropriate location
        //      in the error buffer and load the error string.
        //

        pmapierr->lpszError = (LPTSTR) (sizeof(MAPIERROR) +
                                          (BYTE *) pmapierr);

        LoadStringA(g_hInstResource, idsError, szErrorString, 
                    CchMaxErrorMessage);
        
        lstrcpyA((LPSTR)pmapierr->lpszError, szErrorString);
        cch = lstrlenA(szErrorString);
       
        
        //
        // Set the componment string pointer to the appropriate location
        //      in the error buffer and load the component string.
        //

        pmapierr->lpszComponent = pmapierr->lpszError + cch + 1;
        cch = CchMaxErrorMessage - cch - 1;

        lstrcpyA((LPSTR)pmapierr->lpszComponent,
                        _szComponent ? _szComponent : SzNull);
        cch = lstrlen(pmapierr->lpszComponent);
        
        if (cch == 0) {
            *(pmapierr->lpszComponent) = 0;
        }

        break;


    case eObject:
        //
        //  The last regisered error message came from an object.  If we
        //      could not get the last error from the object, just return
        //      the error it returned and we are done.
        //

        if (_hrGLE != NOERROR) {
            Assert( _pmapierr == NULL );
            *lppMAPIError = NULL;
            return _hrGLE;
        }

    case eExtended:
        //
        //  The last error was an extended error.  The error is in the
        //      structure, we need to copy this structure and return
        //      it back to the user
        //

        Assert( _pmapierr != NULL );
        cb = (lstrlen(_pmapierr->lpszError) + lstrlen(_pmapierr->lpszComponent) + 2);

        if (MAPIAllocateBuffer(cb + sizeof(MAPIERROR),
                               (void **) &pmapierr)) {
            return ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        }

        *pmapierr = *_pmapierr;
        pmapierr->lpszError = (LPTSTR) (sizeof(MAPIERROR) + (BYTE *) pmapierr);
        lstrcpy(pmapierr->lpszError, _pmapierr->lpszError);
        pmapierr->lpszComponent = pmapierr->lpszError +
          lstrlen(pmapierr->lpszError) + 1;
        lstrcpy(pmapierr->lpszComponent, _pmapierr->lpszComponent);

        break;

    case eNoError:
        break;

    default:
        Assert(0);
        return NOERROR;
    }

    *lppMAPIError = pmapierr;
    return ResultFromScode(S_OK);
}



int CLastError::ShowError(HWND hWnd)
{
    char szMessage[512];
    char szbuf[256];

    if(_eLastErr != eObject || NULL == _pmapierr) return 0;

    wsprintfA(szMessage, "%s\n%s\nLowLevelError: 0x%08lx context: %ld ",
                        ((LPSTR)_pmapierr->lpszError ? (LPSTR)_pmapierr->lpszError : ""),
                        (LPSTR)_pmapierr->lpszComponent ? (LPSTR)_pmapierr->lpszComponent : "",
                        (LPSTR)_pmapierr->ulLowLevelError, (LPSTR)_pmapierr->ulContext);
   
    wsprintfA(szbuf, "\nReturn Code: 0x%08lx", SCODE(_hrLast));
    lstrcatA (szMessage, szbuf);
    
#if defined (_WIN32)
    *szHelpFile = '\0';

    int iret = 0;
    BOOL fCanHelp;

    if(_pmapierr->lpszError  &&  _pmapierr->ulContext)
        fCanHelp = TRUE;
    else
        fCanHelp = FALSE;
        
    if(fCanHelp)
    {
        DWORD dw = GetPrivateProfileStringA("Help File Mappings", (LPSTR)_pmapierr->lpszComponent,
                           "", szHelpFile, _MAX_PATH, "mapisvc.inf");
        if(0 == dw)
            fCanHelp = FALSE;

        if(fCanHelp)
        {
            MSGBOXPARAMSA mbp = {0};

            mbp.cbSize = sizeof(MSGBOXPARAMS);
            mbp.hwndOwner = hWnd;
            mbp.hInstance = NULL;
            mbp.lpszText = szMessage;
            mbp.lpszCaption = _szComponent ? _szComponent : "Error!";
            mbp.dwStyle = MB_ICONSTOP | MB_OK | MB_HELP;
            mbp.lpszIcon = NULL;
            mbp.dwContextHelpId = _pmapierr->ulContext;
            mbp.lpfnMsgBoxCallback = ErrorBoxCallBack;
            mbp.dwLanguageId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

            iret = MessageBoxIndirectA(&mbp);
        }
        
    }

    
    if(!fCanHelp)
        iret = MessageBoxA (hWnd, szMessage,
                    _szComponent ? _szComponent : "Error!",
                        MB_ICONSTOP | MB_OK );

    *szHelpFile = '\0';

    return iret;

#else
    return MessageBox (hWnd, szMessage,
                     _szComponent ? _szComponent : "Error!",
                         MB_ICONSTOP | MB_OK );
#endif
}

#if defined(_WIN32)
VOID CALLBACK ErrorBoxCallBack(LPHELPINFO lpHelpInfo)
{
    Assert(*szHelpFile != '\0');

    WinHelpA(NULL, szHelpFile, HELP_CONTEXT,
            lpHelpInfo->dwContextId);
}
#endif //_WIN32


