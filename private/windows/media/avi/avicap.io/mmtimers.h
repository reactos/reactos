/* + mmtimers.h
 *
 * Accurate timers using pentium cpu clock, QueryPerformanceCounter
 * or GetTickCount depending on what system the code is run on
 *
 * Copyright (C) 1995, Microsoft Corporation, all rights reserved
 *
 *-========================================================================*/
 
#if !defined _INC_MMTIMERS_
#define _INC_MMTIMERS_

  typedef struct {
    DWORD dwlo;
    DWORD dwhi;
    } PCTIMER, NEAR * PPCTIMER;

  struct _pctimer_global {
    DWORD    dwRawHz;
    DWORD    dwMicroAdjust;
    union {
      DWORD    dwRawKhz;
      WORD     wRawKhz;
    };
    union {
      DWORD    dwRawMhz;
      WORD     wRawMhz;
    };
    DWORD    dwTimerKhz;
    PCTIMER  base;
    DWORD (WINAPI * DifTicks     )(PCTIMER *);
    DWORD (WINAPI * DifMicrosec  )(PCTIMER *);
    DWORD (WINAPI * DifMillisec  )(PCTIMER *);
    DWORD (WINAPI * DeltaTicks   )(PCTIMER *);
    DWORD (WINAPI * DeltaMicrosec)(PCTIMER *);
    DWORD (WINAPI * DeltaMillisec)(PCTIMER *);
    UINT     uTimerType;
    };
  extern struct _pctimer_global pc;

  extern VOID WINAPI InitPerformanceCounters ();

  #define pcBegin()          pc.DeltaTicks(&pc.base)
  #define pcGetTime()        pc.DifMillisec(&pc.base)
  #define pcGetTicks()       pc.DifMicrosec(&pc.base)
  #define pcGetTickRate()   (pc.dwTimerKhz * 1000)
  #define pcBeginTimer(ppt) (pc.DeltaMicrosec(ppt), 0)
  #define pcDeltaTicks(ppt)  pc.DeltaMicrosec(ppt)

#endif //_INC_MMTIMERS_

// =============================================================================

//
// include this in only one module in a DLL or APP
//   
#if (defined _INC_MMTIMERS_CODE_) && (_INC_MMTIMERS_CODE_ != FALSE)
#undef _INC_MMTIMERS_CODE_
#define _INC_MMTIMERS_CODE_ FALSE

  static DWORD WINAPI tgtDeltaTime (PCTIMER *pctimer)
  {
        DWORD dwTime = timeGetTime();
        DWORD dwDelta = dwTime - pctimer->dwlo;
        pctimer->dwlo = dwTime;
        return dwDelta;
  }

  static DWORD WINAPI tgtDiffTime (PCTIMER *pctimer)
  {
        return timeGetTime() - pctimer->dwlo;
  }

  struct _pctimer_global pc = {1000, 0, 1, 0, 1,
                               0, 0,
                               (LPVOID)tgtDiffTime,
                               (LPVOID)tgtDiffTime,
                               (LPVOID)tgtDiffTime,
                               (LPVOID)tgtDeltaTime,
                               (LPVOID)tgtDeltaTime,
                               (LPVOID)tgtDeltaTime,
                               0,
                               };

  #if defined WIN32 || defined _WIN32

    #if !defined _X86_
      #define Scale(value,scalar) (DWORD)((value).QuadPart / (scalar))
    #else
      //
      // c9 wants to do LARGE_INTEGER division by calling a library
      // routine. We get a link error for projects that are not
      // already using the C-runtime, so to avoid that, we do the division
      // using x86 assembler
      //
      #pragma warning(disable:4704)
      #pragma warning(disable:4035)
      DWORD _inline Scale(
          LARGE_INTEGER value,
          DWORD         scalar)
      {
          _asm {
            mov  ecx, scalar
            mov  eax, value.LowPart
            mov  edx, value.HighPart
            jecxz bail
            cmp  edx, ecx
            jb ok_to_divide
            push eax
            mov  eax, edx
            xor  edx, edx
            div  ecx
            pop  eax
          ok_to_divide:
            div  ecx
          bail:
          }
      }
    #endif

    static VOID WINAPI qpcInitTimer (PCTIMER * pbase)
    {
       QueryPerformanceCounter ((LPVOID)pbase);
    }

    static DWORD WINAPI qpcDiffTicks (PCTIMER * pbase)
    {
       LARGE_INTEGER *plarge = (LPVOID)pbase;
       LARGE_INTEGER ticks;

       QueryPerformanceCounter (&ticks);
       ticks.QuadPart -= plarge->QuadPart;
       return ticks.LowPart;
    }

    static DWORD WINAPI qpcDiffMicrosec (PCTIMER * pbase)
    {
       LARGE_INTEGER *plarge = (LPVOID)pbase;
       LARGE_INTEGER ticks;

       QueryPerformanceCounter (&ticks);
       ticks.QuadPart -= plarge->QuadPart;
       ticks.LowPart = Scale(ticks, pc.dwRawMhz);
       if (pc.dwMicroAdjust)
           return MulDiv (ticks.LowPart, 1000000, pc.dwMicroAdjust);
       return ticks.LowPart;
    }

    static DWORD WINAPI qpcDiffMillisec (PCTIMER * pbase)
    {
       LARGE_INTEGER *plarge = (LPVOID)pbase;
       LARGE_INTEGER ticks;

       QueryPerformanceCounter (&ticks);
       ticks.QuadPart -= plarge->QuadPart;
       return Scale(ticks, pc.dwRawKhz);
    }

    static DWORD WINAPI qpcDeltaTicks (PCTIMER * pbase)
    {
       LARGE_INTEGER *plarge = (LPVOID)pbase;
       LARGE_INTEGER ticks = *plarge;

       QueryPerformanceCounter (plarge);
       ticks.QuadPart = plarge->QuadPart - ticks.QuadPart;
       return ticks.LowPart;
    }

    static DWORD WINAPI qpcDeltaMicrosec (PCTIMER * pbase)
    {
       LARGE_INTEGER *plarge = (LPVOID)pbase;
       LARGE_INTEGER ticks = *plarge;

       QueryPerformanceCounter (plarge);
       ticks.QuadPart = plarge->QuadPart - ticks.QuadPart;
       ticks.LowPart = Scale(ticks, pc.dwRawMhz);
       if (pc.dwMicroAdjust)
           return MulDiv (ticks.LowPart, 1000000, pc.dwMicroAdjust);
       return ticks.LowPart;
    }

    static DWORD WINAPI qpcDeltaMillisec (PCTIMER * pbase)
    {
       LARGE_INTEGER *plarge = (LPVOID)pbase;
       LARGE_INTEGER ticks = *plarge;

       QueryPerformanceCounter (plarge);
       ticks.QuadPart = plarge->QuadPart - ticks.QuadPart;
       return Scale(ticks, pc.dwRawKhz);
    }

    static DWORD WINAPI qpcTimerFreq ()
    {
       LARGE_INTEGER freq;
       if (QueryPerformanceFrequency (&freq))
          return freq.LowPart;
       return 0;
    }

    #ifdef _X86_

      #pragma warning(disable:4704)
      #pragma warning(disable:4035)

      static VOID WINAPI p5InitTimer (PCTIMER * pBase)
      {
         _asm {
            _emit 0x0f
            _emit 0x31

            mov ebx, pBase
            mov [ebx], eax
            mov [ebx+4], edx
         }
      }

      static DWORD WINAPI p5DiffTicks (PCTIMER * pBase)
      {
         _asm {
            _emit 0x0f
            _emit 0x31

            mov ebx, pBase
            sub eax, [ebx]
            sbb edx, [ebx+4]
         }
      }

      static DWORD WINAPI p5DiffMicrosec (PCTIMER * pBase)
      {
         _asm {
            _emit 0x0f
            _emit 0x31

            mov ebx, pBase
            sub eax, [ebx]
            sbb edx, [ebx+4]

            mov  ecx, pc.dwRawMhz
            jecxz bail
            cmp  edx, ecx
            jb ok_to_divide
            push eax
            mov  eax, edx
            xor  edx, edx
            div  ecx
            pop  eax
          ok_to_divide:
            div  ecx
          bail:
         }
      }

      static DWORD WINAPI p5DiffMillisec (PCTIMER * pBase)
      {
         _asm {
            _emit 0x0f
            _emit 0x31

            mov ebx, pBase
            sub eax, [ebx]
            sbb edx, [ebx+4]

            mov  ecx, pc.dwRawKhz
            jecxz bail
            cmp  edx, ecx
            jb ok_to_divide
            push eax
            mov  eax, edx
            xor  edx, edx
            div  ecx
            pop  eax
          ok_to_divide:
            div  ecx
          bail:
         }
      }

      static DWORD WINAPI p5DeltaTicks (PCTIMER * pBase)
      {
         _asm {
            _emit 0x0f
            _emit 0x31

            mov  ebx, pBase
            mov  ecx, eax
            sub  eax, [ebx]
            mov  [ebx], ecx
            mov  ecx, edx
            sbb  edx, [ebx+4]
            mov  [ebx+4], ecx
         }
      }
      static DWORD WINAPI p5DeltaMicrosec (PCTIMER * pBase)
      {
         _asm {
            _emit 0x0f
            _emit 0x31

            mov  ebx, pBase
            mov  ecx, eax
            sub  eax, [ebx]
            mov  [ebx], ecx
            mov  ecx, edx
            sbb  edx, [ebx+4]
            mov  [ebx+4], ecx

            mov  ecx, pc.dwRawMhz
            jecxz bail
            cmp  edx, ecx
            jb ok_to_divide
            push eax
            mov  eax, edx
            xor  edx, edx
            div  ecx
            pop  eax
          ok_to_divide:
            div  ecx
          bail:
         }
      }

      static DWORD WINAPI p5DeltaMillisec (PCTIMER * pBase)
      {
         _asm {
            _emit 0x0f
            _emit 0x31

            mov  ebx, pBase
            mov  ecx, eax
            sub  eax, [ebx]
            mov  [ebx], ecx
            mov  ecx, edx
            sbb  edx, [ebx+4]
            mov  [ebx+4], ecx

            mov  ecx, pc.dwRawKhz
            jecxz bail
            cmp  edx, ecx
            jb ok_to_divide
            push eax
            mov  eax, edx
            xor  edx, edx
            div  ecx
            pop  eax
          ok_to_divide:
            div  ecx
          bail:
         }
      }

      static DWORD WINAPI p5TimerFreq ()
      {
          SYSTEM_INFO si;

          GetSystemInfo(&si);
          if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL &&
              si.wProcessorLevel == 5
             )
          {
             PCTIMER timer;
             LARGE_INTEGER qpc1, qpc2;
             DWORD   dwTime;
             DWORD   dwTicks;
             OSVERSIONINFO osv;
             #define MS_INTERVAL 500

             // pentium timers dont work correctly on NT so
             // dont use them
             //
             {
             osv.dwOSVersionInfoSize = sizeof(osv);
             GetVersionEx (&osv);
             }

             // dont use pentium timers if they take more
             // than about 12 microsec to execute
             //
             p5InitTimer  (&timer);
             if (p5DeltaTicks (&timer) > (60 * 12) &&
                 p5DeltaTicks (&timer) > (60 * 12))
             {
                // pentium timers are too slow to try and use them.
                // just go with QueryPerformanceCounter instead
                //
                return 0;
             }

             // for some reason, if you use timeBeginPeriod
             // on NT.  it decides that my 90mhz pentium is an 88mhz
             // pentium.
             //
             //if (osv.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
             //   timeBeginPeriod (1);

             p5InitTimer (&timer);
             QueryPerformanceCounter (&qpc1);
             Sleep(MS_INTERVAL);
             QueryPerformanceCounter (&qpc2);
             dwTicks = p5DiffTicks(&timer);

             //if (osv.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
             //   timeEndPeriod (1);

             dwTime = (DWORD)(qpc2.QuadPart - qpc1.QuadPart);
             QueryPerformanceFrequency (&qpc1);
             dwTime = MulDiv(dwTime, 1000, qpc1.LowPart);

             if (dwTime < MS_INTERVAL * 9 / 10)
                return 0;

             pc.dwRawMhz = (dwTicks + dwTime * 1000/2) /dwTime /1000;
             pc.dwRawKhz = pc.dwRawMhz * 1000;
             pc.dwRawHz  = pc.dwRawKhz * 1000;
             pc.dwMicroAdjust = 0;
             pc.dwTimerKhz = 1000;

             return pc.dwRawHz;
          }

          return 0;
      }

    #endif

    VOID WINAPI InitPerformanceCounters (void)
    {
        DWORD dwFreq;

       #ifdef _X86_
        if (p5TimerFreq())
        {
            pc.DifTicks      = p5DiffTicks;
            pc.DifMicrosec   = p5DiffMicrosec;
            pc.DifMillisec   = p5DiffMillisec;
            pc.DeltaTicks    = p5DeltaTicks;
            pc.DeltaMicrosec = p5DeltaMicrosec;
            pc.DeltaMillisec = p5DeltaMillisec;
            pc.uTimerType    = 5;
            return;
        }
       #endif

        if (dwFreq = qpcTimerFreq())
        {
            pc.dwRawKhz = dwFreq / 1000;
            pc.dwRawMhz = pc.dwRawKhz / 1000;
            pc.dwMicroAdjust = dwFreq / pc.dwRawMhz;
            if (pc.dwMicroAdjust == 1000000)
                pc.dwMicroAdjust = 0;
            pc.dwTimerKhz = 1000;

            pc.DifTicks      = qpcDiffTicks;
            pc.DifMicrosec   = qpcDiffMicrosec;
            pc.DifMillisec   = qpcDiffMillisec;
            pc.DeltaTicks    = qpcDeltaTicks;
            pc.DeltaMicrosec = qpcDeltaMicrosec;
            pc.DeltaMillisec = qpcDeltaMillisec;
            pc.uTimerType    = 1;
        }
    }

  #else // win16

    #pragma warning(disable:4704)
    #pragma warning(disable:4035)

    static VOID WINAPI p5InitTimer (PCTIMER * pBase)
    {
       _asm {
          _emit 0x0f
          _emit 0x31

          _emit 0x66
          xor bx, bx
          mov bx, pBase
          _emit 0x66
          mov [bx], ax
          _emit 0x66
          mov [bx+4], dx
       }
    }

    static DWORD WINAPI p5DiffTicks (PCTIMER * pBase)
    {
       _asm {
          _emit 0x0f
          _emit 0x31

          _emit 0x66
          xor bx, bx
          mov bx, pBase
          _emit 0x66
          sub ax, [bx]
          _emit 0x66
          sbb dx, [bx+4]
       }
    }

    static DWORD WINAPI p5DiffMicrosec (PCTIMER * pBase)
    {
       _asm {
          _emit 0x0f
          _emit 0x31

          _emit 0x66
          xor bx, bx
          mov bx, pBase
          _emit 0x66
          sub ax, [bx]
          _emit 0x66
          sbb dx, [bx+4]

          //_emit 0x66
          mov  cx, pc.wRawMhz
          _emit 0x66
          jcxz bail
          _emit 0x66
          cmp  dx, cx
          jb ok_to_divide
          _emit 0x66
          push ax
          _emit 0x66
          mov  ax, dx
          _emit 0x66
          xor  dx, dx
          _emit 0x66
          div  cx
          _emit 0x66
          pop  ax
        ok_to_divide:
          _emit 0x66
          div  cx
        bail:
       }
    }

    static DWORD WINAPI p5DiffMillisec (PCTIMER * pBase)
    {
       _asm {
          _emit 0x0f
          _emit 0x31

          _emit 0x66
          xor bx, bx
          mov bx, pBase
          _emit 0x66
          sub ax, [bx]
          _emit 0x66
          sbb dx, [bx+4]

          _emit 0x66
          mov  cx, pc.wRawKhz
          _emit 0x66
          jcxz  bail
          _emit 0x66
          cmp  dx, cx
          jb ok_to_divide
          _emit 0x66
          push ax
          _emit 0x66
          mov  ax, dx
          _emit 0x66
          xor  dx, dx
          _emit 0x66
          div  cx
          _emit 0x66
          pop  ax
        ok_to_divide:
          _emit 0x66
          div  cx
        bail:
       }
    }

    static DWORD WINAPI p5DeltaTicks (PCTIMER * pBase)
    {
       _asm {
          _emit 0x0f
          _emit 0x31

          _emit 0x66
          mov  bx, pBase
          _emit 0x66
          mov  cx, ax
          _emit 0x66
          sub  ax, [bx]
          _emit 0x66
          mov  [bx], cx
          _emit 0x66
          mov  cx, dx
          _emit 0x66
          sbb  dx, [bx+4]
          _emit 0x66
          mov  [bx+4], cx
       }
    }
    static DWORD WINAPI p5DeltaMicrosec (PCTIMER * pBase)
    {
       _asm {
          _emit 0x0f
          _emit 0x31

          _emit 0x66
          mov  bx, pBase
          _emit 0x66
          mov  cx, ax
          _emit 0x66
          sub  ax, [bx]
          _emit 0x66
          mov  [bx], cx
          _emit 0x66
          mov  cx, dx
          _emit 0x66
          sbb  dx, [bx+4]
          _emit 0x66
          mov  [bx+4], cx

          _emit 0x66
          mov  cx, pc.wRawMhz
          _emit 0x66
          jcxz  bail
          _emit 0x66
          cmp  dx, cx
          jb ok_to_divide
          _emit 0x66
          push ax
          _emit 0x66
          mov  ax, dx
          _emit 0x66
          xor  dx, dx
          _emit 0x66
          div  cx
          _emit 0x66
          pop  ax
        ok_to_divide:
          _emit 0x66
          div  cx
        bail:
       }
    }

    static DWORD WINAPI p5DeltaMillisec (PCTIMER * pBase)
    {
       _asm {
          _emit 0x0f
          _emit 0x31

          _emit 0x66
          mov  bx, pBase
          _emit 0x66
          mov  cx, ax
          _emit 0x66
          sub  ax, [bx]
          _emit 0x66
          mov  [bx], cx
          _emit 0x66
          mov  cx, dx
          _emit 0x66
          sbb  dx, [bx+4]
          _emit 0x66
          mov  [bx+4], cx

          //_emit 0x66
          mov  cx, pc.wRawKhz
          _emit 0x66
          jcxz  bail
          _emit 0x66
          cmp  dx, cx
          jb ok_to_divide
          _emit 0x66
          push ax
          _emit 0x66
          mov  ax, dx
          _emit 0x66
          xor  dx, dx
          _emit 0x66
          div  cx
          _emit 0x66
          pop  ax
        ok_to_divide:
          _emit 0x66
          div  cx
        bail:

       }
    }

    // 16 bit code for detecting CPU type so we can decide
    // whether or not it is ok to use the pentium timing stuff
    //
    int WINAPI pcGetCpuID ()
    {
    _asm {
        _emit 0x66
        pushf       ; save eflags

        // check for 486 by attempting to set the 0x40000 bit
        // in eflags.  if we can set it, the processor is 486 or better
        //
        _emit 0x66
        pushf               ; push eflags
        pop   ax            ; move eflags to dx:ax
        pop   dx
        or    dx, 4         ; set 0x40000 bit in eflags
        push  dx            ; put back onto stack
        push  ax
        _emit 0x66
        popf                ; pop modified flags back into eflags
        _emit 0x66
        pushf               ; push eflags back onto stack
        pop   ax            ; move eflags in to dx:bx
        pop   dx

        _emit 0x66
        popf        ; restore origonal eflags

        mov   bx, 3 ; assume 386
        test  dx, 4 ; 486 will preserve 0x40000 bit on push/pop of eflags
        jz    ret_procid
        inc   bx    ; this is a 486 or higher

        // if we get to here it is a 486 or greater

        // check for pentium or higher by attempting to toggle the
        // ID bit (0x200000) in eflags.
        // on a pentium, this bit will toggle, on 486 it will not
        //
        _emit  0x66
        pushf                   ; save eflags
        _emit  0x66
        pushf                   ; get eflags
        pop    ax               ; put eflags into dx:ax
        pop    dx
        xor    dx, 0x20         ; toggle 0x200000 bit in eflags
        push   dx
        push   ax               ; push modified eflags from dx:ax
        _emit  0x66
        popf                    ; load changed eflags
        _emit  0x66
        pushf                   ; get eflags again
        pop    ax               ; discard eflags lo
        pop    ax               ; get eflags hi
        xor    dx, ax           ; did anything change?
        _emit  0x66             ; restore old eflags
        popf

        test   dx, 0x20         ; did we change the 20 bit?
        jz     ret_procid       ; if not, bx already has 4, return that

        // if we get to here, it is a pentium or greater

        // use the pentium CPUID instruction to detect exact processor
        // type
        //
        _emit 0x0F            ; cpuid instruction
        _emit 0xA2
        shr   ax, 8           ; extract family field
        and   ax, 0x0F
        mov   bx, ax          ; 5 is pentium, others are higher

       ret_procid:
        mov   ax, bx
        }
    }

    static DWORD WINAPI p5TimerFreq ()
    {
        if (pcGetCpuID() >= 5)
        {
           DWORD   dw;
           DWORD   dwTicks;
           static PCTIMER timer;

           p5InitTimer (&timer);
           dw = timeGetTime() + 200;
           while (timeGetTime() < dw)
               ;
           dw = timeGetTime() - dw;
           dwTicks = p5DiffTicks(&timer);

           pc.dwRawMhz = (dwTicks + dw * 1000/2) /dw /1000;
           pc.dwRawKhz = pc.dwRawMhz * 1000;
           pc.dwRawHz  = pc.dwRawKhz * 1000;
           pc.dwMicroAdjust = 0;
           pc.dwTimerKhz = 1000;

           return pc.dwRawHz;
        }

        return 0;
    }

    VOID WINAPI InitPerformanceCounters (void)
    {
        if (p5TimerFreq() != 0l)
        {
            pc.DifTicks      = p5DiffTicks;
            pc.DifMicrosec   = p5DiffMicrosec;
            pc.DifMillisec   = p5DiffMillisec;
            pc.DeltaTicks    = p5DeltaTicks;
            pc.DeltaMicrosec = p5DeltaMicrosec;
            pc.DeltaMillisec = p5DeltaMillisec;
            pc.uTimerType    = 5;
            return;
        }
    }

  #endif // WIN32

#endif // _INC_MMTIMERS_CODE_
