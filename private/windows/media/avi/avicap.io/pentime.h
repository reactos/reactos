/*+ pentime.h
 *
 * pentium specific high precision timer functions for 16 or 32 bit apps
 * (16 bit also needs pentime.asm)
 *
 *-======================================================================*/

#ifndef PENTIME_H
#define PENTIME_H

typedef struct {
    DWORD dwlo;
    DWORD dwhi;
    } PENTIMER, NEAR * PPENTIMER;

void FAR PASCAL pentimeInitTimer (
    PPENTIMER pptimer);

DWORD FAR PASCAL pentimeGetMicrosecs (
    PPENTIMER pptimer);

DWORD FAR PASCAL pentimeGetMicrosecDelta (
    PPENTIMER pptimer);

DWORD FAR PASCAL pentimeGetMillisecs (
    PPENTIMER pptimer);

struct _pentime_global {
    DWORD    dwTimerKhz;
    BOOL     bActive;
    PENTIMER base;
    DWORD    dwCpuMhz;
    DWORD    dwCpuKhz;
    };
extern struct _pentime_global pentime;

//
// macros to make whether to use pentium timers or not a runtime option
//
#ifdef _X86_

  #define pentimeGetTime()       pentime.bActive ? pentimeGetMillisecs(&pentime.base) : timeGetTime()
  #define pentimeGetTicks()      pentime.bActive ? pentimeGetMicrosecs(&pentime.base) : timeGetTime()
  #define pentimeBegin()         pentime.bActive ? (pentimeInitTimer(&pentime.base), 0l) : (void)(pentime.base.dwlo = timeGetTime())
  #define pentimeGetTickRate()   (pentime.bActive ? (pentime.dwTimerKhz * 1000) : 1000l)
  #define pentimeGetDeltaTicks(ppt) pentime.bActive ? pentimeGetMicrosecDelta(ppt) : \
    ((ppt)->dwhi = (ppt)->dwlo, (ppt)->dwlo = timeGetTime(), (ppt)->dwlo - (ppt)->dwhi)

#else

  #define pentimeGetTime()       timeGetTime()
  #define pentimeGetTicks()      timeGetTime()
  #define pentimeBegin()         (pentime.base.dwlo = timeGetTime())
  #define pentimeGetTickRate()   (1000l)
  #define pentimeGetDeltaTicks(ppt) \
    ((ppt)->dwhi = (ppt)->dwlo, (ppt)->dwlo = timeGetTime(), (ppt)->dwlo - (ppt)->dwhi)

#endif

#if (defined _INC_PENTIME_CODE_) && (_INC_PENTIME_CODE_ != FALSE)
    #undef _INC_PENTIME_CODE_
    #define _INC_PENTIME_CODE_ FALSE

    struct _pentime_global pentime = {1, 0};

   #ifdef _WIN32
     #ifdef _X86_
      static BYTE opGetP5Ticks[] = {
          0x0f, 0x31,                   // rtdsc
          0xc3                          // ret
          };

      static void (WINAPI * GetP5Ticks)() = (LPVOID)opGetP5Ticks;

      #pragma warning(disable:4704)
      #pragma warning(disable:4035)

      void FAR PASCAL pentimeInitTimer (
          PPENTIMER pptimer)
      {
          GetP5Ticks();
          _asm {
              mov  ebx, pptimer
              mov  [ebx], eax
              mov  [ebx+4], edx
          };
      }

      DWORD FAR PASCAL pentimeGetCpuTicks (
          PPENTIMER pptimer)
      {
          GetP5Ticks();
          _asm {
              mov  ebx, pptimer
              sub  eax, [ebx]
              sbb  edx, [ebx+4]
          };
      }

      DWORD FAR PASCAL pentimeGetMicrosecs (
          PPENTIMER pptimer)
      {
          GetP5Ticks();
          _asm {
              mov  ebx, pptimer
              sub  eax, [ebx]
              sbb  edx, [ebx+4]
              and  edx, 31               // to prevent overflow
              mov  ecx, pentime.dwCpuMhz
              div  ecx
          };
      }

      DWORD WINAPI pentimeGetMicrosecDelta (
          PPENTIMER pptimer)
      {
          GetP5Ticks();
          _asm {
              mov  ebx, pptimer
              mov  ecx, eax
              sub  eax, [ebx]
              mov  [ebx], ecx
              mov  ecx, edx
              sbb  edx, [ebx+4]
              mov  [ebx+4], ecx
              and  edx, 31
              mov  ecx, pentime.dwCpuMhz
              div  ecx
          };
      }

      DWORD FAR PASCAL pentimeGetMillisecs (
          PPENTIMER pptimer)
      {
          GetP5Ticks();
          _asm {
              mov  ebx, pptimer
              sub  eax, [ebx]
              sbb  edx, [ebx+4]
              and  edx, 0x7fff           // to prevent overflow
              mov  ecx, pentime.dwCpuKhz
              div  ecx
          };
      }
     #endif

      void FAR PASCAL pentimeSetMhz (
          DWORD dwCpuMhz)
      {
          pentime.dwCpuMhz = dwCpuMhz;
          pentime.dwCpuKhz = dwCpuMhz * 1000;
      }
   #else // 16 bit - set mhz is in ASM file

    void FAR PASCAL pentimeSetMhz (
        DWORD dwCpuMhz);

   #endif

    void FAR PASCAL pentimeInit (
        BOOL  bIsPentium,
        DWORD dwCpuMhz)
    {
        if (pentime.bActive = bIsPentium)
        {
            pentimeSetMhz (dwCpuMhz);
            pentime.dwTimerKhz = 1000;
        }
        else
            pentime.dwTimerKhz = 1;

        pentimeBegin();
    }

   #ifdef _WIN32
    VOID WINAPI pentimeDetectCPU ()
    {
        SYSTEM_INFO si;
        static DWORD MS_INTERVAL = 500; // measure pentium cpu clock for this
                                        // many millisec.  the larger this number
                                        // the more accurate our Mhz measurement.
                                        // numbers less than 100 are unlikely
                                        // to be reliable because of the slop
                                        // in GetTickCount

       #ifdef _X86_
        GetSystemInfo(&si);
        if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL &&
            si.wProcessorLevel == 5
           )
        {
            DWORD     dw;
            PENTIMER  qwTicks;
            DWORD     dwTicks;

            pentime.bActive = TRUE;
            pentime.dwTimerKhz = 1000;

            timeBeginPeriod(1);
            dw = timeGetTime ();
            pentimeInitTimer (&qwTicks);

            Sleep(MS_INTERVAL);

            dw = timeGetTime() - dw;
            dwTicks = pentimeGetCpuTicks (&qwTicks);
            timeEndPeriod(1);

            // calculate the CPU Mhz value and Khz value
            // to use as millisec and microsec divisors
            //
            pentime.dwCpuMhz = (dwTicks + dw*500)/dw/1000;
            pentime.dwCpuKhz = pentime.dwCpuMhz * 1000;
        }
        else
       #endif
        {
            pentime.bActive = FALSE;
            pentime.dwTimerKhz = 1;
        }
    }
   #else // win16
    VOID WINAPI pentimeDetectCPU ()
    {
        pentimeInit (FALSE, 33);
    }
   #endif


#endif // _INC_PENTIME_CODE_
#endif // PENTIME_H
