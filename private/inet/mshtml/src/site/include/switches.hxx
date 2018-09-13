//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       switches.hxx
//
//  Contents:   Runtime switches
//
//----------------------------------------------------------------------------

#ifndef I_SWITCHES_HXX_
#define I_SWITCHES_HXX_
//#pragma INCMSG("--- Beg 'switches.hxx'")

#if DBG==1

PerfDbgExtern(tagSwitchSerialize)
PerfDbgExtern(tagSwitchNoBgRecalc)
PerfDbgExtern(tagSwitchNoRecalcLines)
PerfDbgExtern(tagSwitchNoRenderLines)
PerfDbgExtern(tagSwitchNoImageCache)

#define IsSwitchSerialize()         IsPerfDbgEnabled(tagSwitchSerialize)
#define IsSwitchNoBgRecalc()        IsPerfDbgEnabled(tagSwitchNoBgRecalc)
#define IsSwitchNoRecalcLines()     IsPerfDbgEnabled(tagSwitchNoRecalcLines)
#define IsSwitchNoRenderLines()     IsPerfDbgEnabled(tagSwitchNoRenderLines)
#define IsSwitchNoImageCache()      IsPerfDbgEnabled(tagSwitchNoImageCache)
#define InitRuntimeSwitches()
#define SWITCHES_ENABLED

#elif defined(PRODUCT_PROF) || defined(USESWITCHES)

void InitRuntimeSwitchesFn();

extern BOOL g_fSwitchSerialize;
extern BOOL g_fSwitchNoBgRecalc;
extern BOOL g_fSwitchNoRecalcLines;
extern BOOL g_fSwitchNoRenderLines;
extern BOOL g_fSwitchNoImageCache;

#define IsSwitchSerialize()         g_fSwitchSerialize
#define IsSwitchNoBgRecalc()        g_fSwitchNoBgRecalc
#define IsSwitchNoRecalcLines()     g_fSwitchNoRecalcLines
#define IsSwitchNoRenderLines()     g_fSwitchNoRenderLines
#define IsSwitchNoImageCache()      g_fSwitchNoImageCache
#define InitRuntimeSwitches()       InitRuntimeSwitchesFn()
#define SWITCHES_ENABLED

#define SWITCHES_TIMER_TOKENIZER        0
#define SWITCHES_TIMER_PARSER           1
#define SWITCHES_TIMER_COMPUTEFORMATS   2
#define SWITCHES_TIMER_RECALCLINES      3
#define SWITCHES_TIMER_DECODEIMAGE      4
#define SWITCHES_TIMER_PAINT            5
#define SWITCHES_TIMER_COUNT            6

void    SwitchesBegTimer(int iTimer);
void    SwitchesEndTimer(int iTimer);
void    SwitchesGetTimers(char * pchBuf);
#define SWITCHTIMERS_ENABLED

#endif

#ifndef SWITCHTIMERS_ENABLED
#define SwitchesBegTimer(iTimer)
#define SwitchesEndTimer(iTimer)
#endif

//#pragma INCMSG("--- End 'switches.hxx'")
#else
//#pragma INCMSG("*** Dup 'switches.hxx'")
#endif
