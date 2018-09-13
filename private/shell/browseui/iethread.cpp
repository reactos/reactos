#include "priv.h"
#include <iethread.h>
#include "hnfblock.h"

#ifdef UNIX
#include "unixstuff.h"
#endif

BOOL _GetToken(LPCWSTR *ppszCmdLine, LPWSTR szToken, UINT cchMax)
{
    LPCWSTR pszCmdLine = *ppszCmdLine;

    WCHAR chTerm = L' ';
    if (*pszCmdLine == L'"') {
        chTerm = L'"';
        pszCmdLine++;
    }

    UINT ichToken = 0;
    WCHAR ch;
    while((ch=*pszCmdLine) && (ch != chTerm)) {
        if (ichToken < cchMax-1) {
            szToken[ichToken++] = ch;
        }
        pszCmdLine++;
    }

    szToken[ichToken] = L'\0';

    if (chTerm == L'"' && ch == L'"') {
        pszCmdLine++;
    }

    // skip trailing spaces
    while(*pszCmdLine == L' ')
        pszCmdLine++;

    *ppszCmdLine = pszCmdLine;

    TraceMsgW(TF_SHDAUTO, "_GetToken returning %s (+%s)", szToken, pszCmdLine);

    return szToken[0];
}

BOOL _CheckForOptionOnCmdLine(LPCWSTR *ppszCmdLine, LPCWSTR pszOption)
{
    LPCWSTR pszCmdLine = *ppszCmdLine;
    int cch = lstrlenW(pszOption);

    if (0 == StrCmpNIW(pszCmdLine, pszOption, cch))
    {
        pszCmdLine+= cch;
        while(*pszCmdLine == L' ')
            pszCmdLine++;

        *ppszCmdLine = pszCmdLine;
        return TRUE;
    }
    return FALSE;
}

BOOL IsCalleeIEAK()
{
    // BUGBUG: this is hack so as to allow IEAK CD install to continue without
    // any security restrictions. If the IEAK CD install window name changes
    // the name change should also reflect here.
    return (FindWindow(TEXT("IECD"), NULL) != NULL);
}

BOOL SHParseIECommandLine(LPCWSTR *ppwszCmdLine, IETHREADPARAM * piei)
{
    ASSERT(ppwszCmdLine);
    ASSERT(*ppwszCmdLine);
    LPCWSTR pszCmdLine = *ppwszCmdLine;

#ifdef UNIX
    if( CheckForInvalidOptions( *ppwszCmdLine ) == FALSE )
    {
        piei->fShouldStart = FALSE;
        return FALSE;
    }

    // Options valid.
    piei->fShouldStart = TRUE;
#endif

    TraceMsg(TF_SHDAUTO, "ParseIECommandLine called with %s", pszCmdLine);

    BOOL fDontLookForPidl = FALSE; // A flag option is set, so don't go looking for an open window 
                                   // with the same pidl
                                   // BUGBUG: (dli) what if there is a window opened with the same flags?
    while (*pszCmdLine == L'-')
    {
        fDontLookForPidl = TRUE;
        
        //Note: (dli)These flags are supposed to be set to FALSE at initialization
        // check if -nohome was passed in!
        //
        if (_CheckForOptionOnCmdLine(&pszCmdLine, L"-slf") && !IsOS(OS_NT5) && IsCalleeIEAK())
            piei->fNoLocalFileWarning = TRUE;
        else if (_CheckForOptionOnCmdLine(&pszCmdLine, L"-nohome"))
            piei->fDontUseHomePage = TRUE;
        else if (_CheckForOptionOnCmdLine(&pszCmdLine, L"-k"))
        {
            piei->fFullScreen = TRUE;
            piei->fNoDragDrop = TRUE;
        }
        else if (_CheckForOptionOnCmdLine(&pszCmdLine, L"-embedding"))
        {
            piei->fAutomation = TRUE;
            // if we're started as an embedding, we don't want to go to our start page
            piei->fDontUseHomePage = TRUE;
        } 
#ifndef UNIX
        else if (_CheckForOptionOnCmdLine(&pszCmdLine, L"-channelband"))
        {
            piei->fDesktopChannel = TRUE;
        } 
        else if (_CheckForOptionOnCmdLine(&pszCmdLine, L"-e")) 
        {
            piei->uFlags |= COF_EXPLORE;

        } 
#else
        else if (_CheckForOptionOnCmdLine(&pszCmdLine, L"-help"))
        {
            piei->fShouldStart = FALSE;
            PrintIEHelp();
            break;
        } 
        else if  (_CheckForOptionOnCmdLine(&pszCmdLine, L"-v") || 
                  _CheckForOptionOnCmdLine(&pszCmdLine, L"-version")) 
        {
            piei->fShouldStart = FALSE;
            PrintIEVersion();
            break;
        } 
#endif
        else if (_CheckForOptionOnCmdLine(&pszCmdLine, L"-root")) 
        {
            ASSERT(piei->pidlRoot==NULL);
            WCHAR szRoot[MAX_PATH];
            if (_GetToken(&pszCmdLine, szRoot, ARRAYSIZE(szRoot))) 
            {
                CLSID clsid, *pclsid = NULL;
                
                TraceMsgW(TF_SHDAUTO, "ParseIECommandLine got token for /root %s", szRoot);

                if (GUIDFromString(szRoot, &clsid))
                {
                    pclsid = &clsid;
                    _GetToken(&pszCmdLine, szRoot, ARRAYSIZE(szRoot));
                }

                if (szRoot[0]) 
                {
                    LPITEMIDLIST pidlRoot = ILCreateFromPathW(szRoot);
                    if (pidlRoot) 
                    {
                        piei->pidl = ILRootedCreateIDList(pclsid, pidlRoot);
                        ILFree(pidlRoot);
                    } 
                }
            }
        }
        else
        {
#ifdef UNIX
            piei->fShouldStart = FALSE;
#endif
            // unknown option..
            fDontLookForPidl = FALSE;
            break;
        }
    }

    *ppwszCmdLine = pszCmdLine;
    
    return fDontLookForPidl;
}

IETHREADPARAM* SHCreateIETHREADPARAM(LPCWSTR pszCmdLineIn, int nCmdShowIn, ITravelLog *ptlIn, IEFreeThreadedHandShake* piehsIn)
{
    IETHREADPARAM *piei = (IETHREADPARAM *)LocalAlloc(LPTR, sizeof(IETHREADPARAM));
    if (piei)
    {
        piei->pszCmdLine = pszCmdLineIn;    // careful, aliased pointer
        piei->nCmdShow = nCmdShowIn;
        piei->ptl = ptlIn;
        piei->piehs = piehsIn;

        if (piehsIn)
            piehsIn->AddRef();

        if (ptlIn)
            ptlIn->AddRef();
#ifdef UNIX
        piei->fShouldStart = TRUE;
#endif
#ifdef NO_MARSHALLING
        piei->fOnIEThread = TRUE;
#endif 
    }

    return piei;
}

IETHREADPARAM* SHCloneIETHREADPARAM(IETHREADPARAM* pieiIn)
{
    IETHREADPARAM *piei = (IETHREADPARAM *)LocalAlloc(LPTR, sizeof(IETHREADPARAM));
    if (piei)
    {
        *piei = *pieiIn;

        // convert aliased pointers into refs

        if (piei->pidl)
            piei->pidl = ILClone(piei->pidl);
    
        if (piei->pidlSelect)
            piei->pidlSelect = ILClone(piei->pidlSelect);
    
        if (piei->pidlRoot)
            piei->pidlRoot = ILClone(piei->pidlRoot);
    
        if (piei->psbCaller)
            piei->psbCaller->AddRef();
    
        if (piei->ptl)
            piei->ptl->Clone(&piei->ptl);
    }
    return piei;

}

void SHDestroyIETHREADPARAM(IETHREADPARAM* piei)
{
    if (piei)
    {
        if (piei->pidl)
            ILFree(piei->pidl);
    
        if (piei->pidlSelect)
            ILFree(piei->pidlSelect);
    
        if (((piei->uFlags & COF_HASHMONITOR) == 0) && piei->pidlRoot)
            ILFree(piei->pidlRoot);
    
        if (piei->piehs)
            piei->piehs->Release();   // note, this is not a COM object, don't ATOMICRELEASE();

        ATOMICRELEASE(piei->psbCaller);
        ATOMICRELEASE(piei->pSplash);
        ATOMICRELEASE(piei->ptl);
        ATOMICRELEASE(piei->punkRefProcess);

        LocalFree(piei);
    }
}
