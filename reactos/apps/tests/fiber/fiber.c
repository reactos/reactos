/* $Id: fiber.c,v 1.1 2004/03/07 03:15:20 hyperion Exp $
*/

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <tchar.h>
#include <windows.h>

#ifndef InitializeListHead
#define InitializeListHead(PLH__) ((PLH__)->Flink = (PLH__)->Blink = (PLH__))
#endif

#ifndef IsListEmpty
#define IsListEmpty(PLH__) ((PLH__)->Flink == (PVOID)(PLH__))
#endif

#ifndef RemoveEntryList

#define RemoveEntryList(PLE__) \
{ \
 PLIST_ENTRY pleBack__ = (PLIST_ENTRY)((PLE__)->Blink); \
 PLIST_ENTRY pleForward__ = (PLIST_ENTRY)((PLE__)->Flink); \
 \
 pleBack__->Flink = pleForward__; \
 pleForward__->Blink = pleBack__; \
}

#endif

#ifndef InsertTailList

#define InsertTailList(PLH__, PLE__) \
{ \
 PLIST_ENTRY pleListHead__ = (PLH__); \
 PLIST_ENTRY pleBlink__ = (PLIST_ENTRY)((PLH__)->Blink); \
 \
 (PLE__)->Flink = pleListHead__; \
 (PLE__)->Blink = pleBlink__; \
 pleBlink__->Flink = (PLE__); \
 pleListHead__->Blink = (PLE__); \
}

#endif

#ifndef RemoveHeadList

#define RemoveHeadList(PLH__) \
 (PLIST_ENTRY)((PLH__)->Flink); \
 RemoveEntryList((PLIST_ENTRY)((PLH__)->Flink));

#endif

#define FIBERTEST_COUNT 500

struct FiberData
{
 unsigned nMagic;
 unsigned nId;
 unsigned nPrio;
 unsigned nRealPrio;
 PVOID pFiber;
 LIST_ENTRY leQueue;
 int nTickQueued;
 int nBoost;
 struct FiberData * pfdPrev;
 int bExitPrev;
};

static LIST_ENTRY a_leQueues[32];
static unsigned nTick = 0;
static struct FiberData * pfdLastStarveScan = NULL;

void Fbt_Create(int);
void Fbt_Exit(void);
void Fbt_Yield(void);

struct FiberData * Fbt_GetCurrent(void);
unsigned Fbt_GetCurrentId(void);
VOID CALLBACK Fbt_Startup(PVOID);
void Fbt_Dispatch(struct FiberData *, int);
void Fbt_AfterSwitch(struct FiberData *);

void DoStuff(void);

struct FiberData * Fbt_GetCurrent(VOID)
{
 return GetFiberData();
}

unsigned Fbt_GetCurrentId(VOID)
{
 return Fbt_GetCurrent()->nId;
}

void Fbt_Yield(VOID)
{
 struct FiberData * pfdCur;

 pfdCur = Fbt_GetCurrent();

 if(pfdCur->nBoost > 1)
 {
  -- pfdCur->nBoost;

  if(!pfdCur->nBoost)
   pfdCur->nPrio = pfdCur->nRealPrio;
 }
 else if((rand() % 100) > 50 - (45 * pfdCur->nPrio) / 32)
  Fbt_Dispatch(pfdCur, 0);
}

void Fbt_AfterSwitch(struct FiberData * pfdCur)
{
 struct FiberData * pfdPrev;

 pfdPrev = pfdCur->pfdPrev;

 if(pfdPrev)
 {
  if(pfdCur->bExitPrev)
  {
   if(pfdLastStarveScan == pfdPrev)
    pfdLastStarveScan = 0;

   DeleteFiber(pfdPrev->pFiber);
   free(pfdPrev);
  }
  else
  {
   pfdPrev->nTickQueued = nTick;

   if(pfdPrev->nBoost)
   {
    pfdPrev->nBoost = 0;
    pfdPrev->nPrio = pfdPrev->nRealPrio;
   }

   InsertTailList
   (
    &a_leQueues[pfdPrev->nPrio],
    &pfdPrev->leQueue
   );
  }
 }
}

VOID CALLBACK Fbt_Startup(PVOID pParam)
{
 assert(pParam == GetFiberData());
 Fbt_AfterSwitch(pParam);
 DoStuff();
 Fbt_Exit();
}

void Fbt_Dispatch(struct FiberData * pfdCur, int bExit)
{
 UCHAR i;
 UCHAR n;
 struct FiberData * pfdNext;

 assert(pfdCur == GetFiberData());

 ++ nTick;

 if(nTick % 10 == 0)
 {
  int j;
  int k;
  int b;
  int bResume;
  PLIST_ENTRY ple;

  bResume = 0;
  i = 0;

  if(pfdLastStarveScan)
  {
   unsigned nPrio;

   nPrio = pfdLastStarveScan->nPrio;

   if(IsListEmpty(&pfdLastStarveScan->leQueue))
    i = nPrio;
   else if(pfdLastStarveScan->leQueue.Flink == &a_leQueues[nPrio])
    i = nPrio + 1;
   else
   {
    ple = pfdLastStarveScan->leQueue.Flink;
    bResume = 1;
   }

   if(i >= 15)
    i = 0;
  }

  for(j = 0, k = 0, b = 0; j < 16 && k < 15 && b < 10; ++ j)
  {
   unsigned nDiff;

   if(!bResume)
   {
    int nQueue;
    
    nQueue = (k + i) % 15;

    if(IsListEmpty(&a_leQueues[nQueue]))
    {
     ++ k;
     continue;
    }
 
    ple = (PLIST_ENTRY)a_leQueues[nQueue].Flink;
   }
   else
    bResume = 0;

   pfdLastStarveScan = CONTAINING_RECORD(ple, struct FiberData, leQueue);
   assert(pfdLastStarveScan->nMagic == 0x12345678);
   assert(pfdLastStarveScan != pfdCur);

   if(nTick > pfdLastStarveScan->nTickQueued)
    nDiff = nTick - pfdLastStarveScan->nTickQueued;
   else
    nDiff = pfdLastStarveScan->nTickQueued - nTick;

   if(nDiff > 30)
   {
    ++ b;

    pfdLastStarveScan->nBoost = 2;
    pfdLastStarveScan->nRealPrio = pfdLastStarveScan->nPrio;
    pfdLastStarveScan->nPrio = 15;

    RemoveEntryList(&pfdLastStarveScan->leQueue);
    InsertTailList(&a_leQueues[15], &pfdLastStarveScan->leQueue);
   }
  }
 }

 pfdNext = NULL;

 if(bExit)
  n = 1;
 else
  n = pfdCur->nPrio + 1;

 for(i = 32; i >= n; -- i)
 {
  PLIST_ENTRY pleNext;

  if(IsListEmpty(&a_leQueues[i - 1]))
   continue;

  pleNext = RemoveHeadList(&a_leQueues[i - 1]);
  InitializeListHead(pleNext);

  pfdNext = CONTAINING_RECORD(pleNext, struct FiberData, leQueue);
  assert(pfdNext->pFiber != GetCurrentFiber());
  assert(pfdNext->nMagic == 0x12345678);
  break;
 }

 if(pfdNext)
 {
  pfdNext->pfdPrev = pfdCur;
  pfdNext->bExitPrev = bExit;
 
  SwitchToFiber(pfdNext->pFiber);
 
  Fbt_AfterSwitch(pfdCur);
 }
 else if(bExit)
 {
  PVOID pCurFiber;

  if(pfdLastStarveScan == pfdCur)
   pfdLastStarveScan = NULL;

  pCurFiber = pfdCur->pFiber;
  free(pfdCur);
  DeleteFiber(pCurFiber);
 }
}

void Fbt_Exit(VOID)
{
 Fbt_Dispatch(GetFiberData(), 1);
}

void Fbt_CreateFiber(int bInitial)
{
 PVOID pFiber;
 struct FiberData * pData;
 static int s_bFiberPrioSeeded = 0;
 static LONG s_nFiberIdSeed = 0;

 pData = malloc(sizeof(struct FiberData));

 assert(pData);

 if(bInitial)
  pFiber = ConvertThreadToFiber(pData);
 else
  pFiber = CreateFiber(0, Fbt_Startup, pData);

 if(!s_bFiberPrioSeeded)
 {
  unsigned nFiberPrioSeed;
  time_t tCurTime;

  tCurTime = time(NULL);
  memcpy(&nFiberPrioSeed, &tCurTime, sizeof(nFiberPrioSeed));
  srand(nFiberPrioSeed);
  s_bFiberPrioSeeded = 1;
 }

 assert(pFiber);

 pData->nMagic = 0x12345678;
 pData->nId = InterlockedIncrement(&s_nFiberIdSeed);
 pData->nPrio = rand() % 32;
 pData->pFiber = pFiber;
 pData->nTickQueued = 0;
 pData->nBoost = 0;
 pData->nRealPrio = pData->nPrio;
 pData->pfdPrev = NULL;
 pData->bExitPrev = 0;

 if(bInitial)
 {
  InitializeListHead(&pData->leQueue);
 }
 else
 {
  InsertTailList
  (
   &a_leQueues[pData->nPrio],
   &pData->leQueue
  );
 }
}

void DoStuff(void)
{
 unsigned i;
 unsigned n;
 unsigned nId;

 n = rand() % 1000;
 nId = Fbt_GetCurrentId();

 _ftprintf(stderr, _T("[%u] BEGIN\n"), nId);

 for(i = 0; i < n; ++ i)
 {
  unsigned j;
  unsigned m;

  _ftprintf(stderr, _T("[%u] [%u/%u]\n"), nId, i + 1, n);

  m = rand() % 1000;

  for(j = 0; j < m; ++ j)
   Sleep(0);

  Fbt_Yield();
 }

 _ftprintf(stderr, _T("[%u] END\n"), nId);
}

int _tmain(int argc, _TCHAR const * const * argv)
{
 unsigned i;
 unsigned nFibers;

 if(argc > 1)
  nFibers = _tcstoul(argv[1], 0, NULL);
 else
  nFibers = FIBERTEST_COUNT;

 for(i = 0; i < 32; ++ i)
 {
  InitializeListHead(&a_leQueues[i]);
 }

 for(i = 0; i < nFibers; ++ i)
  Fbt_CreateFiber(i == 0);

 Fbt_Startup(GetFiberData());

 return 0;
}

/* EOF */
