//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       switches.cxx
//
//  Contents:   Runtime switches
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_SWITCHES_HXX_
#define X_SWITCHES_HXX_
#include "switches.hxx"
#endif

#if defined(PRODUCT_PROF)
extern "C" void _stdcall StartCAPAll(void);
extern "C" void _stdcall StopCAPAll(void);
extern "C" void _stdcall SuspendCAPAll(void);
extern "C" void _stdcall ResumeCAPAll(void);
extern "C" void _stdcall StartCAP(void);
extern "C" void _stdcall StopCAP(void);
extern "C" void _stdcall SuspendCAP(void);
extern "C" void _stdcall ResumeCAP(void);
#else
inline void StartCAPAll() { }
inline void StopCAPAll() { }
inline void SuspendCAPAll() { }
inline void ResumeCAPAll() { }
inline void StartCAP() { }
inline void StopCAP() { }
inline void SuspendCAP() { }
inline void ResumeCAP() { }
#endif

PerfDbgTag(tagSwitchSerialize,     "Switches", "Serialize pre-parser and post-parser")
PerfDbgTag(tagSwitchNoBgRecalc,    "Switches", "Disable background recalc")
PerfDbgTag(tagSwitchNoRecalcLines, "Switches", "Disable flow measuring")
PerfDbgTag(tagSwitchNoRenderLines, "Switches", "Disable flow rendering")
PerfDbgTag(tagSwitchNoImageCache,  "Switches", "Disable image caching")

#if defined(PRODUCT_PROF) || defined(USESWITCHES)

BOOL g_fSwitchSerialize     = FALSE;
BOOL g_fSwitchNoBgRecalc    = FALSE;
BOOL g_fSwitchNoRecalcLines = FALSE;
BOOL g_fSwitchNoRenderLines = FALSE;
BOOL g_fSwitchNoImageCache  = FALSE;
BOOL g_fSwitchUseTimers     = FALSE;
BOOL g_fSwitchUseImageTimer = FALSE;
BOOL g_fIsProfiling         = FALSE;

CCriticalSection g_csTimers;
__int64          g_rgtSum[SWITCHES_TIMER_COUNT];
__int64          g_rgtStack[128];
int              g_rgiStack[128];
int              g_iStackDepth;
int              g_iTimerProfile = -1;
char *           g_rgpchNames[] = { "Tokenize", "Parse", "ComputeFormats", "RecalcLines", "DecodeImage", "Paint" };

void
InitRuntimeSwitchesFn()
{
#if defined(PRODUCT_PROF)
    char * pszSection = "profile";
#else
    char * pszSection = "retail";
#endif

    g_fSwitchSerialize     = GetPrivateProfileIntA(pszSection, "serialize",     FALSE, "mshtmdbg.ini");
    g_fSwitchNoBgRecalc    = GetPrivateProfileIntA(pszSection, "nobgrecalc",    FALSE, "mshtmdbg.ini");
    g_fSwitchNoRecalcLines = GetPrivateProfileIntA(pszSection, "norecalclines", FALSE, "mshtmdbg.ini");
    g_fSwitchNoRenderLines = GetPrivateProfileIntA(pszSection, "norenderlines", FALSE, "mshtmdbg.ini");
    g_fSwitchNoImageCache  = GetPrivateProfileIntA(pszSection, "noimagecache",  FALSE, "mshtmdbg.ini");
    g_fSwitchUseTimers     = GetPrivateProfileIntA(pszSection, "usetimers",     FALSE, "mshtmdbg.ini");
    g_fSwitchUseImageTimer = GetPrivateProfileIntA(pszSection, "useimagetimer", FALSE, "mshtmdbg.ini");

#ifdef PRODUCT_PROF
    char ach[256];
    ach[0] = 0;
    GetPrivateProfileStringA(pszSection, "profiletimer", "", ach, ARRAY_SIZE(ach), "mshtmdbg.ini");
    g_fSwitchUseTimers = FALSE;
    for (int i = 0; i < ARRAY_SIZE(g_rgpchNames); ++i)
    {
        if (lstrcmpiA(ach, g_rgpchNames[i]) == 0)
        {
            g_iTimerProfile = i;
            g_fSwitchUseTimers = TRUE;
            break;
        }
    }
#endif
}

#endif

#ifdef SWITCHTIMERS_ENABLED

void
SwitchesBegTimer(int iTimer)
{
    if (!g_fSwitchUseTimers)
        return;

    if (iTimer == SWITCHES_TIMER_DECODEIMAGE && !g_fSwitchUseImageTimer)
        return;

    SuspendCAP();

    g_csTimers.Enter();

    int i = g_iStackDepth;

    if (i < ARRAY_SIZE(g_rgiStack))
    {
        QueryPerformanceCounter((LARGE_INTEGER *)&g_rgtStack[i]);
        g_rgiStack[i] = iTimer;
        g_iStackDepth += 1;

#ifdef PRODUCT_PROF
        if (g_iTimerProfile >= 0)
        {
            if (g_fIsProfiling && iTimer != g_iTimerProfile)
            {
                StopCAP();
                g_fIsProfiling = FALSE;
            }
            else if (!g_fIsProfiling && iTimer == g_iTimerProfile)
            {
                StartCAP();
                g_fIsProfiling = TRUE;
            }
        }
#endif
    }

    g_csTimers.Leave();

    ResumeCAP();
}

void
SwitchesEndTimer(int iTimer)
{
    if (!g_fSwitchUseTimers)
        return;

    if (iTimer == SWITCHES_TIMER_DECODEIMAGE && !g_fSwitchUseImageTimer)
        return;

    SuspendCAP();

    g_csTimers.Enter();

    int i = g_iStackDepth;
    int j;

    if (i > 0)
    {
        i -= 1;
        __int64 * ptBeg = &g_rgtStack[i];
        int * piTimer = &g_rgiStack[i];
        __int64 tBeg, tEnd;

        for (j = i; j >= 0; --j, --ptBeg, --piTimer)
        {
            if (*piTimer == iTimer)
                break;
        }

        if (j >= 0)
        {
            tBeg = *ptBeg;

            if (j < i)
            {
                tEnd = *(ptBeg + 1);
                memmove(ptBeg, ptBeg + 1, (i - j) * sizeof(__int64));
                memmove(piTimer, piTimer + 1, (i - j) * sizeof(int));
            }
            else
            {
                QueryPerformanceCounter((LARGE_INTEGER *)&tEnd);
            }

            g_iStackDepth -= 1;

            tEnd -= tBeg;

            g_rgtSum[iTimer] += tEnd;

            for (--j, --ptBeg; j >= 0; --j, --ptBeg)
                *ptBeg += tEnd;
        }
    }

#ifdef PRODUCT_PROF
    if (g_iTimerProfile >= 0)
    {
        if (g_fIsProfiling && (g_iStackDepth == 0 || g_rgiStack[g_iStackDepth - 1] != g_iTimerProfile))
        {
            StopCAP();
            g_fIsProfiling = FALSE;
        }
        else if (!g_fIsProfiling && g_iStackDepth > 0 && g_rgiStack[g_iStackDepth - 1] == g_iTimerProfile)
        {
            StartCAP();
            g_fIsProfiling = TRUE;
        }
    }
#endif

    g_csTimers.Leave();

    ResumeCAP();
}

void
SwitchesGetTimers(char * pchBuf)
{
    char * pch = pchBuf;

    if (pch)
    {
        *pch = 0;

        if (!g_fSwitchUseTimers)
            return;

#if !defined(PRODUCT_PROF)
        __int64 tFreq;
        QueryPerformanceFrequency((LARGE_INTEGER *)&tFreq);

        for (int i = 0; i < SWITCHES_TIMER_COUNT; ++i)
        {
            if (g_rgtSum[i] == 0)
                continue;

            wsprintfA(pch, "%s=%ld ", g_rgpchNames[i], ((LONG)(g_rgtSum[i] * 1000L / tFreq)));
            pch += lstrlenA(pch);
        }
#endif
    }

    memset(g_rgtSum, 0, sizeof(g_rgtSum));
}

#endif
