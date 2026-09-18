// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_text
//      $Keywords:
//
//  $Description:
//      Simple instrumentation to investigate performance of code fragments,
//      based on QueryPerformance* calls.
//
//  $ENDTAG
//
//  Classes:
//      CPerfAcc
//      CPerfMeasure
//
//  Macros:
//      DeclarePerfAcc MeasurePerf
//
//------------------------------------------------------------------------------



//  Performance measurement is disabled by default,
//  so the macro calls can be kept in code without any effect.
//  To enable, uncomment following line and rebuild. 
//#define ENABLE_SIMPLE_PERF

#ifdef ENABLE_SIMPLE_PERF

#define SIMPLE_PERF_SAMPLES_NUM 256
//+-----------------------------------------------------------------------------
//
//  Usage pattern:
//      //file foo.cpp
//
//      DeclarePerfAcc(someName);   <------------------------------
//
//      void Foo(int width, int height)
//      {
//          UINT uItemsCount = width*height;
//          { // start of the block to measure
//              MeasurePerf(someName, uItemsCount);  <---------------
//              for (UINT i = 0; i < uItemsCount; i++)
//              {
//                  // do something
//              }
//          } // end of the block to measure
//      }
//
//      After application shut down the result is available in text file
//      c:\perfDump.txt.
//
//------------------------------------------------------------------------------

class CPerfAcc
{
    UINT m_uCallsCount;
    UINT m_uItemsCount;
    UINT64 m_uTicksCount;
    UINT m_uMinTicks;
    UINT m_uMaxTicks;
    UINT m_uMinTicksPerItem;
    UINT m_uMaxTicksPerItem;
    UINT m_uMinItems;
    UINT m_uMaxItems;
    char const* m_pTitle;
    CPerfAcc* m_pNext;
public:
    CPerfAcc(char const* pTitle)
    {
        m_pTitle = pTitle;
        m_uCallsCount = 0;
        m_uItemsCount = 0;
        m_uTicksCount = 0;
        m_uMinTicks = 0xFFFFFFFF;
        m_uMaxTicks = 0;
        m_uMinTicksPerItem = 0xFFFFFFFF;
        m_uMaxTicksPerItem = 0;
        m_uMinItems = 0xFFFFFFFF;
        m_uMaxItems = 0;
        m_dumper.Add(this);
    }
    void Update(UINT uItemsCount, UINT64 u64Ticks)
    {
        UINT uTicks = static_cast<UINT>(u64Ticks);
        m_uItemsCount += uItemsCount;
        m_uTicksCount += u64Ticks;

        if (m_uMinTicks > uTicks) m_uMinTicks = uTicks;
        if (m_uMaxTicks < uTicks) m_uMaxTicks = uTicks;

        if (m_uMaxItems < uItemsCount) m_uMaxItems = uItemsCount;
        if (m_uMinItems > uItemsCount) m_uMinItems = uItemsCount;

        uTicks /= uItemsCount;
        if (m_uMinTicksPerItem > uTicks) m_uMinTicksPerItem = uTicks;
        if (m_uMaxTicksPerItem < uTicks) m_uMaxTicksPerItem = uTicks;

        m_uCallsCount++;
    }

    class CDumper
    {
        CPerfAcc* m_pChain;
    public:
        CDumper()
        {
            m_pChain = NULL;
        }
        void Add(CPerfAcc* pAcc)
        {
            pAcc->m_pNext = m_pChain;
            m_pChain = pAcc;
        }
        ~CDumper() {Dump();}
        // static object destructors may not work in milcore.dll
        // so we are going through explicit call to Dump()
        void Dump()
        {
            FILE* pFile = fopen("c:\\perfDump.txt", "wt");
            if (pFile)
            {
                for (CPerfAcc* pAcc = m_pChain; pAcc; pAcc = pAcc->m_pNext)
                {
                    pAcc->Dump(pFile);
                }
                fclose(pFile);
            }
        }
    };

    static CDumper m_dumper;


private:
    void Dump(FILE* pFile)
    {
        if (m_uCallsCount == 0)
            return;

        fprintf(pFile, "\n%s\n", m_pTitle);
        fprintf(pFile, "called, times: %d\n", m_uCallsCount);
        fprintf(pFile, "items handled: %d\n", m_uItemsCount);
        fprintf(pFile, "total time, 1000000 ticks: %10.6f\n", double(m_uTicksCount)*(1./1000000));
        fprintf(pFile, "time per call, 1000 ticks: %f\n", double(m_uTicksCount)*(1./1000)/double(m_uCallsCount));
        fprintf(pFile, "average time per item, ticks: %f\n", double(m_uTicksCount)/double(m_uItemsCount));
        fprintf(pFile, "Min time per call, ticks: %d\n", m_uMinTicks);
        fprintf(pFile, "Max time per call, ticks: %d\n", m_uMaxTicks);
        fprintf(pFile, "Min time per item, ticks: %d\n", m_uMinTicksPerItem);
        fprintf(pFile, "Max time per item, ticks: %d\n", m_uMaxTicksPerItem);
        fprintf(pFile, "Min items per call: %d\n", m_uMinItems);
        fprintf(pFile, "Max items per call: %d\n", m_uMaxItems);
    }
};

__forceinline static UINT64 ReadTicksCounter()
{
    __asm rdtsc;
}

class CPerfMeasure
{
public:
    CPerfMeasure(CPerfAcc& acc, UINT uItemsCount) : m_acc(acc)
    {
        m_uItemsCount = uItemsCount;
        m_startTime = ReadTicksCounter();
    }
    ~CPerfMeasure()
    {
        UINT64 endTime = ReadTicksCounter();
        m_acc.Update(m_uItemsCount, endTime - m_startTime);
    }
private:
    CPerfAcc& m_acc;
    UINT m_uItemsCount;
    UINT64 m_startTime;
};


#define DeclarePerfAcc(title) CPerfAcc g_PerfAcc_##title(#title)
#define MeasurePerf(title, uItemsCount) CPerfMeasure o(g_PerfAcc_##title, uItemsCount)

#else //#ifndef ENABLE_SIMPLE_PERF

#define DeclarePerfAcc(title)
#define MeasurePerf(title, uItemsCount)

#endif //ENABLE_SIMPLE_PERF


