// ===========================================================================
// File: HOOKS.CXX
//    implements CSetupHook
//


#include <cdlpch.h>
#include "advpkp.h"
#include "advpub.h"

CRunSetupHook g_RunSetupHook;
extern DWORD  g_dwCodeDownloadSetupFlags;

// ---------------------------------------------------------------------------
// %%Function: CSetupHook::CSetupHook
// ---------------------------------------------------------------------------
CSetupHook::CSetupHook(
    CDownload *pdl,
    LPCSTR szHook,
    LPCSTR szInf,
    LPCSTR szInfSection,
    DWORD flags,
    HRESULT *phr)
    :
    m_pdl(pdl),
    m_flags(flags),
    m_state(INSTALL_INIT)
{
    Assert(szInf);

    *phr = S_OK;

    if (szInf) {

        m_szInf= new char [lstrlen(szInf)+1];

        if (m_szInf)
            lstrcpy(m_szInf, szInf);
        else
            *phr = E_OUTOFMEMORY;
    } else {
        m_szInf = NULL;
    }

    if (szInfSection) {

        m_szInfSection = new char [lstrlen(szInfSection)+1];

        if (m_szInfSection)
            lstrcpy(m_szInfSection, szInfSection);
        else
            *phr = E_OUTOFMEMORY;
    } else {
        m_szInfSection = NULL;
    }

    if (szHook) {

        m_szHook = new char [lstrlen(szHook)+1];

        if (m_szHook)
            lstrcpy(m_szHook, szHook);
        else
            *phr = E_OUTOFMEMORY;
    } else {
        m_szHook = NULL;
    }

}  // CSetupHook

// ---------------------------------------------------------------------------
// %%Function: CSetupHook::~CSetupHook
// ---------------------------------------------------------------------------
CSetupHook::~CSetupHook()
{
    if (m_szInf)
        SAFEDELETE(m_szInf);

    if (m_szInfSection)
        SAFEDELETE(m_szInfSection);

    if (m_szHook)
        SAFEDELETE(m_szHook);

}  // ~CSetupHook


// ---------------------------------------------------------------------------
// %%Function: CSetupHook::ExpandVar
// ---------------------------------------------------------------------------
HRESULT
CSetupHook::ExpandVar(
    LPSTR& pchSrc,          // passed by ref!
    LPSTR& pchOut,          // passed by ref!
    DWORD& cbLen,           // passed by ref!
    DWORD cbBuffer,
    const char * szVars[],
    const char * szValues[])
{
    HRESULT hr = S_FALSE;
    int cbvar = 0;

    Assert (*pchSrc == '%');

    for (int i=0; szVars[i] && (cbvar = lstrlen(szVars[i])) ; i++) { // for each variable

        int cbneed = 0;

        if ( (szValues[i] == NULL) || !(cbneed = lstrlen(szValues[i])))
            continue;

        cbneed++;   // add for nul

        if (0 == strncmp(szVars[i], pchSrc, cbvar)) {

            // found something we can expand

                if ((cbLen + cbneed) >= cbBuffer) {
                    // out of buffer space
                    *pchOut = '\0'; // term
                    hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                    goto Exit;
                }

                lstrcpy(pchOut, szValues[i]);
                cbLen += (cbneed -1); //don't count the nul

                pchSrc += cbvar;        // skip past the var in pchSrc
                pchOut += (cbneed -1);  // skip past dir in pchOut

                hr = S_OK;
                goto Exit;

        }
    }

Exit:

    return hr;
    
}

// ---------------------------------------------------------------------------
// %%Function: CSetupHook::TranslateString
// ---------------------------------------------------------------------------
HRESULT
CSetupHook::TranslateString()
{
    Assert(m_pdl);
    Assert(m_szInf);
    Assert(m_szHook);

    HRESULT hr = S_OK;

    DWORD flags = (g_dwCodeDownloadSetupFlags & CDSF_USE_SETUPAPI)?RSC_FLAG_SETUPAPI:0;

    // there's a command line in the run= command.
    // have advpack tranalate it for us.

    DWORD dwBufferSize = 0;
    DWORD dwRequiredSize = 0;

    // get reqd size to hold expanded string
#ifdef WX86
    if (m_pdl->GetCodeDownload()->GetMultiArch()->GetRequiredArch() == PROCESSOR_ARCHITECTURE_INTEL) {
        hr = g_RunSetupHook.TranslateInfStringX86(
                m_pdl->GetCodeDownload()->GetMainInf(), // inf name
                NULL,                                   // use DefaultInstall
                m_szHook,                               // hook section name
                "run",                                  // key name
                NULL,                                   // find out size reqd.
                dwBufferSize,
                &dwRequiredSize,
                (PVOID)flags);
    } else {
        hr = g_RunSetupHook.TranslateInfString(
                m_pdl->GetCodeDownload()->GetMainInf(), // inf name
                NULL,                                   // use DefaultInstall
                m_szHook,                               // hook section name
                "run",                                  // key name
                NULL,                                   // find out size reqd.
                dwBufferSize,
                &dwRequiredSize,
                (PVOID)flags);
    }
#else

    hr = g_RunSetupHook.TranslateInfString(
            m_pdl->GetCodeDownload()->GetMainInf(), // inf name
            NULL,                                   // use DefaultInstall 
            m_szHook,                               // hook section name
            "run",                                  // key name
            NULL,                                   // find out size reqd.
            dwBufferSize,
            &dwRequiredSize,
            (PVOID)flags);
#endif


    if (FAILED(hr))
        goto Exit;

    Assert(dwRequiredSize);

    if (m_szInf)
        SAFEDELETE(m_szInf);

    m_szInf= new char [(dwBufferSize = dwRequiredSize+1)];

    if (!m_szInf) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // the real thing
#ifdef WX86
    if (m_pdl->GetCodeDownload()->GetMultiArch()->GetRequiredArch() == PROCESSOR_ARCHITECTURE_INTEL) {
        hr = g_RunSetupHook.TranslateInfStringX86(
                m_pdl->GetCodeDownload()->GetMainInf(), // inf name
                NULL,                                   // use DefaultInstall
                m_szHook,                               // hook section name
                "run",                                  // key name
                m_szInf,
                dwBufferSize,
                &dwRequiredSize,
                (PVOID)flags);
    } else {
        hr = g_RunSetupHook.TranslateInfString(
                m_pdl->GetCodeDownload()->GetMainInf(), // inf name
                NULL,                                   // use DefaultInstall
                m_szHook,                               // hook section name
                "run",                                  // key name
                m_szInf,
                dwBufferSize,
                &dwRequiredSize,
                (PVOID)flags);
    }
#else
    hr = g_RunSetupHook.TranslateInfString(
            m_pdl->GetCodeDownload()->GetMainInf(), // inf name
            NULL,                                   // use DefaultInstall 
            m_szHook,                               // hook section name
            "run",                                  // key name
            m_szInf,
            dwBufferSize,
            &dwRequiredSize,
            (PVOID)flags);
#endif



Exit:

    return hr;
        
}


// ---------------------------------------------------------------------------
// %%Function: CSetupHook::ExpandCommandLine
// ---------------------------------------------------------------------------
HRESULT
CSetupHook::ExpandCommandLine(
    LPSTR szSrc,
    LPSTR szBuf,
    DWORD cbBuffer,
    const char * szVars[],
    const char * szValues[])
{
    Assert(cbBuffer);


    HRESULT hr = S_FALSE;

    LPSTR pchSrc = szSrc;     // start parsing at begining of cmdline

    LPSTR pchOut = szBuf;       // set at begin of out buffer
    DWORD cbLen = 0;

    while (*pchSrc) {

        // look for match of any of our env vars
        if (*pchSrc == '%') {

            HRESULT hr1 = ExpandVar(pchSrc, pchOut, cbLen, // all passed by ref!
                cbBuffer, szVars, szValues);  

            if (FAILED(hr1)) {
                hr = hr1;
                goto Exit;
            }


            if (hr1 == S_OK) {    // expand var expanded this
                hr = hr1;
                continue;
            }
        }
            
        // copy till the next % or nul
        if ((cbLen + 1) < cbBuffer) {

            *pchOut++ = *pchSrc++;
            cbLen++;

        } else {

            // out of buffer space
            *pchOut = '\0'; // term
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            goto Exit;

        }


    }

    *pchOut = '\0'; // term


Exit:

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CSetupHook::Run
// ---------------------------------------------------------------------------
HRESULT
CSetupHook::Run()
{
    Assert(m_pdl);
    CCodeDownload *pcdl = m_pdl->GetCodeDownload();
    Assert(pcdl);
    HWND hWnd = pcdl->GetClientBinding()->GetHWND();

    HANDLE hExe = INVALID_HANDLE_VALUE;
    HRESULT hr = S_OK;
    DWORD flags = m_flags;

#define SIZE_CMD_LINE   2048

    char szBuf[SIZE_CMD_LINE];  // enough for commandline

    DWORD nStatusText = 0;
    WCHAR szStatusText[SIZE_CMD_LINE];

    Assert(m_szInf);

    // BEGIN NOTE: add vars and values in matching order
    // add a var by adding a new define VAR_NEW_VAR = NUM_VARS++
    const char *szVars[] = {

#define VAR_EXTRACT_DIR     0       // dir hook CABs are expanded to temporarily
        "%EXTRACT_DIR%",

#define VAR_OBJECT_DIR      1       // dir the main object will be installed to
        "%OBJECT_DIR%",             // usually activex cache dir, sometimes
                                    // a conflict dir or dir of prev version
                                    // of object

#define VAR_SRC_URL         2       // CODEBASE where we got this from
        "%SRC_URL%",                
                                    
                                    

#define NUM_VARS            3

        ""
    };

    char      szSrcURL[INTERNET_MAX_URL_LENGTH];
    LPCWSTR   wzSrcURL = NULL;

    szSrcURL[0] = '\0';
    if ((wzSrcURL = GetSrcURL()) != NULL) {
        WideCharToMultiByte(CP_ACP, 0, wzSrcURL, -1, szSrcURL, MAX_PATH, 0, 0);
    }

    const char *szValues[NUM_VARS + 1];
    szValues[VAR_EXTRACT_DIR] = GetHookDir();
    szValues[VAR_OBJECT_DIR] = GetObjectDir();
    szValues[VAR_SRC_URL] = szSrcURL;
    szValues[NUM_VARS] = NULL;
    // END NOTE: add vars and values in matching order


    if (m_flags & RSC_FLAG_INF) {

        flags |= RSC_FLAG_QUIET;

        if (g_dwCodeDownloadSetupFlags & CDSF_USE_SETUPAPI)
            flags |= RSC_FLAG_SETUPAPI;

        // get fully qualified name for INF

        if (GetHookDir()) {

            if (!catDirAndFile(szBuf, SIZE_CMD_LINE, GetHookDir(),
                               m_szInf)) {
                hr = E_UNEXPECTED;
                goto Exit;
            }
        } else {

            // no hook dir
            Assert(m_pdl->GetExtn() != FILEXTN_CAB);

            lstrcpy(szBuf, m_pdl->GetFileName());
        }

    } else {

        // cmd line

        // have ADVPACK expand out custom LDID vars in run= cmdline
        // then we will process for our own vars
        // this allows the user to specify custom dirs thru the INF
        // and use it in the cmd line off registry keys

        hr = TranslateString();
        if (FAILED(hr))
            goto Exit;

        // look for and substitute variables like %EXTRACT_DIR%
        // and expand out the command line

        hr = ExpandCommandLine(m_szInf, szBuf, SIZE_CMD_LINE, szVars, szValues);

        if (FAILED(hr))
            goto Exit;

        hr = S_OK;  // reset

    }

    nStatusText = MultiByteToWideChar(CP_ACP, 0, szBuf,
        -1, szStatusText, SIZE_CMD_LINE);


#ifdef WX86
    if (m_pdl->GetCodeDownload()->GetMultiArch()->GetRequiredArch() == PROCESSOR_ARCHITECTURE_INTEL) {
        hr = g_RunSetupHook.RunSetupCommandX86(hWnd,
            szBuf,
            m_szInfSection,
            // extracted files in this dir
            (char *)((GetHookDir())?GetHookDir():m_pdl->GetCodeDownload()->GetCacheDir()),
            NULL, /*title*/
            &hExe,                          // handle to wait on for EXE
            flags,
            NULL);
    } else {
        hr = g_RunSetupHook.RunSetupCommand(hWnd,
            szBuf,
            m_szInfSection,
            // extracted files in this dir
            (char *)((GetHookDir())?GetHookDir():m_pdl->GetCodeDownload()->GetCacheDir()),
            NULL, /*title*/
            &hExe,                          // handle to wait on for EXE
            flags,
            NULL);
    }
#else
    hr = g_RunSetupHook.RunSetupCommand(hWnd,
        szBuf,
        m_szInfSection,
        // extracted files in this dir
        (char *)((GetHookDir())?GetHookDir():m_pdl->GetCodeDownload()->GetCacheDir()), 
        NULL, /*title*/
        &hExe,                          // handle to wait on for EXE
        flags,
        NULL);
#endif


    if (SUCCEEDED(hr) && (m_flags & RSC_FLAG_INF)) {

        // pass a notification to reboot
        if (hr == ERROR_SUCCESS_REBOOT_REQUIRED) {

            ICodeInstall* pCodeInstall = pcdl->GetICodeInstall();
            if (pCodeInstall) {
                pCodeInstall->OnCodeInstallProblem(
                                    CIP_NEED_REBOOT,
                                    NULL, szStatusText, 0);
            }

            pcdl->SetRebootRequired();
            hr = S_OK;
        }
    }

    // if we launched an EXE then we would have to mark as waiting for it
    if (hExe != INVALID_HANDLE_VALUE) {
        
        pcdl->SetWaitingForEXE(szBuf, FALSE /*don't delete EXE */);
        pcdl->SetWaitingForEXEHandle(hExe);
    }

Exit:

    if (FAILED(hr)) {
        pcdl->CodeDownloadDebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_RUNSETUPHOOK_FAILED, hr, m_szInf);
    } else {
        pcdl->CodeDownloadDebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_HOOK_COMPLETE, m_szHook);

    }


    SetState(INSTALL_DONE);

    return hr;
}
