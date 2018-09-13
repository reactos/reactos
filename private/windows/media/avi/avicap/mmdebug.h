/*
 * johnkn's debug logging and assert macros
 *
 */

#if !defined _INC_MMDEBUG_
#define _INC_MMDEBUG_
//
// prototypes for debug functions.
//
#if !defined(_WIN32) && !defined(LPTSTR)
    #define LPTSTR LPSTR
#endif

    #define SQUAWKNUMZ(num) #num
    #define SQUAWKNUM(num) SQUAWKNUMZ(num)
    #define SQUAWK __FILE__ "(" SQUAWKNUM(__LINE__) ") ----"
    #define DEBUGLINE __FILE__ "(" SQUAWKNUM(__LINE__) ") "

    #if defined DEBUG || defined _DEBUG || defined DEBUG_RETAIL

        int  FAR _cdecl AuxDebugEx(int, LPSTR, ...);
        VOID WINAPI AuxDebugDump (int, LPVOID, int);
        int  WINAPI DebugSetOutputLevel (int);

       #if defined DEBUG_RETAIL
        #define INLINE_BREAK
       #else
        #ifdef _X86_
         #define INLINE_BREAK _asm {int 3}
        #else
         #define INLINE_BREAK DebugBreak()
        #endif
       #endif

       #if 0
        #undef  assert
        #define assert(exp) \
            (void)((exp) ? 0 : AuxDebugEx(-2, DEBUGLINE "assert failed: " #exp "\r\n", (int)__LINE__))

        #undef  assert2
        #define assert2(exp,sz) \
            (void)((exp) ? 0 : AuxDebugEx(-2, DEBUGLINE "assert failed: " sz "\r\n", (int)__LINE__))
       #else
        #undef  assert
        #define assert(exp); {\
            if (!(exp)) {\
                AuxDebugEx(-2, DEBUGLINE "assert failed: " #exp "\r\n", (int)__LINE__); \
                INLINE_BREAK;\
                }\
            }
        #undef  assert2
        #define assert2(exp,sz); {\
            if (!(exp)) {\
                AuxDebugEx(-2, DEBUGLINE "assert failed: " sz "\r\n", (int)__LINE__); \
                INLINE_BREAK;\
                }\
            }
        #undef  assert3
        #define assert3(exp,sz,arg); {\
            if (!(exp)) {\
                AuxDebugEx(-2, DEBUGLINE "assert failed: " sz "\r\n", (int)__LINE__, (arg)); \
                INLINE_BREAK;\
                }\
            }
       #endif

     #ifndef STATICFN
      #define STATICFN
     #endif

    #else // defined(DEBUG)

      #define AuxDebugEx  1 ? (void)0 : (void)
      #define AuxDebugDump(a,b,c)

      #define assert(a)      ((void)0)
      #define assert2(a,b)   ((void)0)
      #define assert3(a,b,c) ((void)0)

      #define INLINE_BREAK
      #define DebugSetOutputLevel(i)
     #ifndef STATICFN
      #define STATICFN static
     #endif

   #endif // defined(DEBUG)

   #define AuxDebug(sz) AuxDebugEx (1, DEBUGLINE sz "\r\n")
   #define AuxDebug2(sz,a) AuxDebugEx (1, DEBUGLINE sz "\r\n", (a))

#endif //_INC_MMDEBUG_

// =============================================================================

//
// include this in only one module in a DLL or APP
//
#if defined DEBUG || defined _DEBUG || defined DEBUG_RETAIL
    #if (defined _INC_MMDEBUG_CODE_) && (_INC_MMDEBUG_CODE_ != FALSE)
    #undef _INC_MMDEBUG_CODE_
    #define _INC_MMDEBUG_CODE_ FALSE

    #include <stdarg.h>

    #if !defined _WIN32 && !defined wvsprintfA
     #define wvsprintfA wvsprintf
    #endif

    int    debug_OutputOn = 0;

    /*+ AuxDebug - create a formatted string and output to debug terminal
     *
     *-=================================================================*/

    int FAR _cdecl AuxDebugEx (
       int    iLevel,
       LPSTR lpFormat,
       ...)
       {
      #ifdef _WIN32
       char     szBuf[1024];
      #else
       static char  szBuf[512];
      #endif
       int      cb;
       va_list  va;
       char FAR * psz;

       if (debug_OutputOn >= iLevel)
          {
          va_start (va, lpFormat);
          cb = wvsprintfA (szBuf, lpFormat, va);
          va_end (va);

          // eat leading ..\..\ which we get from __FILE__ since
          // george's wierd generic makefile stuff.
          //
          psz = szBuf;
          while (psz[0] == '.' && psz[1] == '.' && psz[2] == '\\')
             psz += 3;

          #ifdef MODULE_DEBUG_PREFIX
           if (psz != szBuf)
              OutputDebugStringA (MODULE_DEBUG_PREFIX);
          #endif

          OutputDebugStringA (psz);
          }

       return cb;
       }

    /*+ AuxDebugDump -
     *
     *-=================================================================*/

    VOID WINAPI AuxDebugDump (
       int    iLevel,
       LPVOID lpvData,
       int    nCount)
       {
       LPBYTE   lpData = lpvData;
       char     szBuf[128];
       LPSTR    psz;
       int      cb;
       int      ix;
       BYTE     abRow[8];

       if (debug_OutputOn <= iLevel || nCount <= 0)
          return;

       do {
          cb = wsprintfA(szBuf, "\t%08X: ", lpData);
          psz = szBuf + cb;

          for (ix = 0; ix < 8; ++ix)
             {
             LPBYTE lpb = lpData;

             abRow[ix] = '.';
             if (IsBadReadPtr (lpData + ix, 1))
                lstrcpyA (psz, ".. ");
             else
                {
                wsprintfA (psz, "%02X ", lpData[ix]);
                if (lpData[ix] >= 32 && lpData[ix] < 127)
                    abRow[ix] = lpData[ix];
                }
             psz += 3;
             }
          for (ix = 0; ix < 8; ++ix)
             *psz++ = abRow[ix];

          lstrcpyA (psz, "\r\n");

          OutputDebugStringA (szBuf);

          } while (lpData += 8, (nCount -= 8) > 0);

       return;
       }

    /*+ DebugSetOutputLevel
     *
     *-=================================================================*/

    BOOL  WINAPI DebugSetOutputLevel (
        int nLevel)
        {
        int nOldLevel = debug_OutputOn;
        debug_OutputOn = nLevel;
        return nOldLevel;
        }

    #endif // _INC_MMDEBUG_CODE_
#endif // DEBUG || _DEBUG
