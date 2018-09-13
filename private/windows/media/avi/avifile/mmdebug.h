/*
 * johnkn's debug logging and assert macros
 *
 */
 
#ifdef __cplusplus
extern "C" {            // Assume C declarations for C++
#endif  // __cplusplus

#if !defined _INC_MMDEBUG_
#define _INC_MMDEBUG_

//
// prototypes for debug functions.
//
    #define SQUAWKNUMZ(num) #num
    #define SQUAWKNUM(num) SQUAWKNUMZ(num)
    #define SQUAWK __FILE__ "(" SQUAWKNUM(__LINE__) ") ----"
    #define DEBUGLINE __FILE__ "(" SQUAWKNUM(__LINE__) ") "
        
    #if defined DEBUG || defined _DEBUG || defined DEBUG_RETAIL

        int  WINAPI AuxDebugEx(int, LPTSTR, ...);
        VOID WINAPI AuxDebugDump (int, LPVOID, int);
        int  WINAPI DebugSetOutputLevel (int);
        LPCTSTR WINAPI AuxMMErrText (DWORD  mmr);

       #if defined DEBUG_RETAIL
        #define INLINE_BREAK
       #else
        #define INLINE_BREAK _asm {int 3}
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

       #define AuxMMR(api,mmr) (mmr) ? AuxDebugEx(1, DEBUGLINE #api " error %d '%s'\r\n", mmr, AuxMMErrText(mmr)) : (int)0


    #else // defined(DEBUG)
                      
      #define AuxDebugEx  1 ? (void)0 :
      #define AuxDebugDump(a,b,c)
      
      #define assert(a)      ((void)0)
      #define assert2(a,b)   ((void)0)
      #define assert3(a,b,c) ((void)0)

      #define INLINE_BREAK
      #define DebugSetOutputLevel(i)
      #define AuxMMErrText(mmr)
      #define AuxMMR(api,mmr)


   #endif // defined(DEBUG)
   
   #define AuxDebug(sz) AuxDebugEx (1, DEBUGLINE sz "\r\n")
   #define AuxDebug2(sz,a) AuxDebugEx (1, DEBUGLINE sz "\r\n", (a))
   

#ifdef __cplusplus
}             // Assume C declarations for C++
#endif  // __cplusplus

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

    int    debug_OutputOn = 0;

    /*+ AuxDebug - create a formatted string and output to debug terminal
     *
     *-=================================================================*/
    
    int WINAPI AuxDebugEx (
       int    iLevel,
       LPTSTR lpFormat,
       ...)
       {
       char     szBuf[1024];
       int      cb;
       va_list  va;
                
       if (debug_OutputOn >= iLevel)
          {
          va_start (va, lpFormat);
          cb = wvsprintfA (szBuf, lpFormat, va);
          va_end (va);
          OutputDebugString (szBuf);
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
       char *   psz;
       int      cb;
       int      ix;
       BYTE     abRow[8];
                
       if (debug_OutputOn <= iLevel || nCount <= 0)
          return;

       do {
          cb = wsprintf (szBuf, "\t%08X: ", lpData);
          psz = szBuf + cb;

          for (ix = 0; ix < 8; ++ix)
             {
             LPBYTE lpb = lpData;

             abRow[ix] = '.';
             if (IsBadReadPtr (lpData + ix, 1))
                lstrcpy (psz, ".. ");
             else
                {
                wsprintf (psz, "%02X ", lpData[ix]);
                if (lpData[ix] >= 32 && lpData[ix] < 127)
                    abRow[ix] = lpData[ix];
                }
             psz += 3;
             }
          for (ix = 0; ix < 8; ++ix)
             *psz++ = abRow[ix];

          lstrcpy (psz, "\r\n");

          OutputDebugString (szBuf);

          } while (lpData += 8, (nCount -= 8) > 0);

       return;
       }

    /*+ AuxMMErrText
     *
     *-=================================================================*/
    
   LPCTSTR WINAPI AuxMMErrText (
      DWORD  mmr)
   {
      static struct _mmerrors {
         DWORD    mmr;
         LPCTSTR  psz;
         } aMMErr[] = {
            MMSYSERR_NOERROR      ,"Success",
            MMSYSERR_ERROR        ,"unspecified error",
            MMSYSERR_BADDEVICEID  ,"device ID out of range",
            MMSYSERR_NOTENABLED   ,"driver failed enable",
            MMSYSERR_ALLOCATED    ,"device already allocated",
            MMSYSERR_INVALHANDLE  ,"device handle is invalid",
            MMSYSERR_NODRIVER     ,"no device driver present",
            MMSYSERR_NOMEM        ,"memory allocation error",
            MMSYSERR_NOTSUPPORTED ,"function isn't supported",
            MMSYSERR_BADERRNUM    ,"error value out of range",
            MMSYSERR_INVALFLAG    ,"invalid flag passed",
            MMSYSERR_INVALPARAM   ,"invalid parameter passed",
           #if (WINVER >= 0x0400)
            MMSYSERR_HANDLEBUSY   ,"handle in use by another thread",
            MMSYSERR_INVALIDALIAS ,"specified alias not found",
            MMSYSERR_BADDB        ,"bad registry database",
            MMSYSERR_KEYNOTFOUND  ,"registry key not found",
            MMSYSERR_READERROR    ,"registry read error",
            MMSYSERR_WRITEERROR   ,"registry write error",
            MMSYSERR_DELETEERROR  ,"registry delete error",
            MMSYSERR_VALNOTFOUND  ,"registry value not found",
           #endif

            WAVERR_BADFORMAT      ,"wave:unsupported wave format",
            WAVERR_STILLPLAYING   ,"wave:still something playing",
            WAVERR_UNPREPARED     ,"wave:header not prepared",
            WAVERR_SYNC           ,"wave:device is synchronous",

            MIDIERR_UNPREPARED    ,"midi:header not prepared",
            MIDIERR_STILLPLAYING  ,"midi:still something playing",
            //MIDIERR_NOMAP         ,"midi:no configured instruments",
            MIDIERR_NOTREADY      ,"midi:hardware is still busy",
            MIDIERR_NODEVICE      ,"midi:port no longer connected",
            MIDIERR_INVALIDSETUP  ,"midi:invalid MIF",
            MIDIERR_BADOPENMODE   ,"midi:operation unsupported w/ open mode",

            TIMERR_NOCANDO        ,"timer: request not completed",
            JOYERR_PARMS          ,"joy:bad parameters",
            JOYERR_NOCANDO        ,"joy:request not completed",
            JOYERR_UNPLUGGED      ,"joystick is unplugged",

            MCIERR_INVALID_DEVICE_ID        ,"MCIERR_INVALID_DEVICE_ID",
            MCIERR_UNRECOGNIZED_KEYWORD     ,"MCIERR_UNRECOGNIZED_KEYWORD",
            MCIERR_UNRECOGNIZED_COMMAND     ,"MCIERR_UNRECOGNIZED_COMMAND",
            MCIERR_HARDWARE                 ,"MCIERR_HARDWARE",
            MCIERR_INVALID_DEVICE_NAME      ,"MCIERR_INVALID_DEVICE_NAME",
            MCIERR_OUT_OF_MEMORY            ,"MCIERR_OUT_OF_MEMORY",
            MCIERR_DEVICE_OPEN              ,"MCIERR_DEVICE_OPEN",
            MCIERR_CANNOT_LOAD_DRIVER       ,"MCIERR_CANNOT_LOAD_DRIVER",
            MCIERR_MISSING_COMMAND_STRING   ,"MCIERR_MISSING_COMMAND_STRING",
            MCIERR_PARAM_OVERFLOW           ,"MCIERR_PARAM_OVERFLOW",
            MCIERR_MISSING_STRING_ARGUMENT  ,"MCIERR_MISSING_STRING_ARGUMENT",
            MCIERR_BAD_INTEGER              ,"MCIERR_BAD_INTEGER",
            MCIERR_PARSER_INTERNAL          ,"MCIERR_PARSER_INTERNAL",
            MCIERR_DRIVER_INTERNAL          ,"MCIERR_DRIVER_INTERNAL",
            MCIERR_MISSING_PARAMETER        ,"MCIERR_MISSING_PARAMETER",
            MCIERR_UNSUPPORTED_FUNCTION     ,"MCIERR_UNSUPPORTED_FUNCTION",
            MCIERR_FILE_NOT_FOUND           ,"MCIERR_FILE_NOT_FOUND",
            MCIERR_DEVICE_NOT_READY         ,"MCIERR_DEVICE_NOT_READY",
            MCIERR_INTERNAL                 ,"MCIERR_INTERNAL",
            MCIERR_DRIVER                   ,"MCIERR_DRIVER",
            MCIERR_CANNOT_USE_ALL           ,"MCIERR_CANNOT_USE_ALL",
            MCIERR_MULTIPLE                 ,"MCIERR_MULTIPLE",
            MCIERR_EXTENSION_NOT_FOUND      ,"MCIERR_EXTENSION_NOT_FOUND",
            MCIERR_OUTOFRANGE               ,"MCIERR_OUTOFRANGE",
            MCIERR_FLAGS_NOT_COMPATIBLE     ,"MCIERR_FLAGS_NOT_COMPATIBLE",
            MCIERR_FILE_NOT_SAVED           ,"MCIERR_FILE_NOT_SAVED",
            MCIERR_DEVICE_TYPE_REQUIRED     ,"MCIERR_DEVICE_TYPE_REQUIRED",
            MCIERR_DEVICE_LOCKED            ,"MCIERR_DEVICE_LOCKED",
            MCIERR_DUPLICATE_ALIAS          ,"MCIERR_DUPLICATE_ALIAS",
            MCIERR_BAD_CONSTANT             ,"MCIERR_BAD_CONSTANT",
            MCIERR_MUST_USE_SHAREABLE       ,"MCIERR_MUST_USE_SHAREABLE",
            MCIERR_MISSING_DEVICE_NAME      ,"MCIERR_MISSING_DEVICE_NAME",
            MCIERR_BAD_TIME_FORMAT          ,"MCIERR_BAD_TIME_FORMAT",
            MCIERR_NO_CLOSING_QUOTE         ,"MCIERR_NO_CLOSING_QUOTE",
            MCIERR_DUPLICATE_FLAGS          ,"MCIERR_DUPLICATE_FLAGS",
            MCIERR_INVALID_FILE             ,"MCIERR_INVALID_FILE",
            MCIERR_NULL_PARAMETER_BLOCK     ,"MCIERR_NULL_PARAMETER_BLOCK",
            MCIERR_UNNAMED_RESOURCE         ,"MCIERR_UNNAMED_RESOURCE",
            MCIERR_NEW_REQUIRES_ALIAS       ,"MCIERR_NEW_REQUIRES_ALIAS",
            MCIERR_NOTIFY_ON_AUTO_OPEN      ,"MCIERR_NOTIFY_ON_AUTO_OPEN",
            MCIERR_NO_ELEMENT_ALLOWED       ,"MCIERR_NO_ELEMENT_ALLOWED",
            MCIERR_NONAPPLICABLE_FUNCTION   ,"MCIERR_NONAPPLICABLE_FUNCTION",
            MCIERR_ILLEGAL_FOR_AUTO_OPEN    ,"MCIERR_ILLEGAL_FOR_AUTO_OPEN",
            MCIERR_FILENAME_REQUIRED        ,"MCIERR_FILENAME_REQUIRED",
            MCIERR_EXTRA_CHARACTERS         ,"MCIERR_EXTRA_CHARACTERS",
            MCIERR_DEVICE_NOT_INSTALLED     ,"MCIERR_DEVICE_NOT_INSTALLED",
            MCIERR_GET_CD                   ,"MCIERR_GET_CD",
            MCIERR_SET_CD                   ,"MCIERR_SET_CD",
            MCIERR_SET_DRIVE                ,"MCIERR_SET_DRIVE",
            MCIERR_DEVICE_LENGTH            ,"MCIERR_DEVICE_LENGTH",
            MCIERR_DEVICE_ORD_LENGTH        ,"MCIERR_DEVICE_ORD_LENGTH",
            MCIERR_NO_INTEGER               ,"MCIERR_NO_INTEGER",
            MCIERR_WAVE_OUTPUTSINUSE        ,"MCIERR_WAVE_OUTPUTSINUSE",
            MCIERR_WAVE_SETOUTPUTINUSE      ,"MCIERR_WAVE_SETOUTPUTINUSE",
            MCIERR_WAVE_INPUTSINUSE         ,"MCIERR_WAVE_INPUTSINUSE",
            MCIERR_WAVE_SETINPUTINUSE       ,"MCIERR_WAVE_SETINPUTINUSE",
            MCIERR_WAVE_OUTPUTUNSPECIFIED   ,"MCIERR_WAVE_OUTPUTUNSPECIFIED",
            MCIERR_WAVE_INPUTUNSPECIFIED    ,"MCIERR_WAVE_INPUTUNSPECIFIED",
            MCIERR_WAVE_OUTPUTSUNSUITABLE   ,"MCIERR_WAVE_OUTPUTSUNSUITABLE",
            MCIERR_WAVE_SETOUTPUTUNSUITABLE ,"MCIERR_WAVE_SETOUTPUTUNSUITABLE",
            MCIERR_WAVE_INPUTSUNSUITABLE    ,"MCIERR_WAVE_INPUTSUNSUITABLE",
            MCIERR_WAVE_SETINPUTUNSUITABLE  ,"MCIERR_WAVE_SETINPUTUNSUITABLE",
            MCIERR_SEQ_DIV_INCOMPATIBLE     ,"MCIERR_SEQ_DIV_INCOMPATIBLE",
            MCIERR_SEQ_PORT_INUSE           ,"MCIERR_SEQ_PORT_INUSE",
            MCIERR_SEQ_PORT_NONEXISTENT     ,"MCIERR_SEQ_PORT_NONEXISTENT",
            MCIERR_SEQ_PORT_MAPNODEVICE     ,"MCIERR_SEQ_PORT_MAPNODEVICE",
            MCIERR_SEQ_PORT_MISCERROR       ,"MCIERR_SEQ_PORT_MISCERROR",
            MCIERR_SEQ_TIMER                ,"MCIERR_SEQ_TIMER",
            MCIERR_SEQ_PORTUNSPECIFIED      ,"MCIERR_SEQ_PORTUNSPECIFIED",
            MCIERR_SEQ_NOMIDIPRESENT        ,"MCIERR_SEQ_NOMIDIPRESENT",
            MCIERR_NO_WINDOW                ,"MCIERR_NO_WINDOW",
            MCIERR_CREATEWINDOW             ,"MCIERR_CREATEWINDOW",
            MCIERR_FILE_READ                ,"MCIERR_FILE_READ",
            MCIERR_FILE_WRITE               ,"MCIERR_FILE_WRITE",
            MCIERR_NO_IDENTITY              ,"MCIERR_NO_IDENTITY",

            MIXERR_INVALLINE            ,"Invalid Mixer Line",
            MIXERR_INVALCONTROL         ,"Invalid Mixer Control",
            MIXERR_INVALVALUE           ,"Invalid Mixer Value",

            MIXERR_INVALVALUE+1         , "unknown error %d"
            };

      UINT uRemain = sizeof(aMMErr)/sizeof(aMMErr[0]);
      UINT uUpper  = uRemain-1;
      UINT uLower  = 0;
      static char szTemp[50];

      if (mmr <= aMMErr[uUpper].mmr)
      {
         // binary search for mmr match, if match
         // return string pointer
         //
         while (--uRemain)
         {
            UINT ii = (uLower + uUpper) >> 1;

            if (aMMErr[ii].mmr < mmr)
            {
               if (uLower == ii)
                  break;
               uLower = ii;
            }
            else if (aMMErr[ii].mmr > mmr)
            {
               if (uUpper == ii)
                  break;
               uUpper = ii;
            }
            else
            {
               return aMMErr[ii].psz;
               break;
            }
         }

         // we can only get to here if no match was found for
         // the error id.
         //
         if ( ! uRemain)
         {
            int ix;

            INLINE_BREAK;

            for (ix = 0; ix < sizeof(aMMErr)/sizeof(aMMErr[0])-1; ++ix)
            {
                assert (aMMErr[ix].mmr < aMMErr[ix+1].mmr);
            }
            lstrcpy (szTemp, "#### Fatal Error! error table not sorted!");
            return szTemp;
         }
      }

      wsprintf (szTemp, aMMErr[uUpper].psz, mmr);
      return szTemp;
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

#ifdef __cplusplus
}             // Assume C declarations for C++
#endif  // __cplusplus
