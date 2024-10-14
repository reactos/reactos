#ifndef ROS_AUDIO_MMEBUDDY_DEBUG_H
#define ROS_AUDIO_MMEBUDDY_DEBUG_H

/*
    Hacky debug macro
*/
// TODO: Use a string-safe function instead of wsprintf().

// FIXME: sdk\lib\...\mmebuddy compilation would fail: wsprintf() and MessageBoxW() are undefined!
#if DBG && !defined(NDEBUG) // #if DBG

    // Helper for SND_ASSERT().
    #define POPUP(...) \
        { \
            WCHAR dbg_popup_msg[1024], dbg_popup_title[256]; \
            wsprintf(dbg_popup_title, L"%hS(%d)", __FILE__, __LINE__); \
            wsprintf(dbg_popup_msg, __VA_ARGS__); \
            MessageBoxW(NULL, dbg_popup_msg, dbg_popup_title, MB_OK | MB_TASKMODAL); \
        }

    #define SND_ERR(...) \
        { \
            WCHAR dbg_popup_msg[1024]; \
            wsprintf(dbg_popup_msg, __VA_ARGS__); \
            OutputDebugStringW(dbg_popup_msg); \
        }

    #define SND_ASSERT(condition) \
        { \
            if ( ! ( condition ) ) \
            { \
                SND_ERR(L"ASSERT FAILED: %hS File %hS Line %u\n", #condition, __FILE__, __LINE__); \
                POPUP(L"ASSERT FAILED: %hS\n", #condition); \
                ExitProcess(1); \
            } \
        }

#else // DBG

    #define SND_ERR(...) do {} while ( 0 )
    #define SND_ASSERT(condition) do {(void)(condition);} while ( 0 )

#endif // DBG

#if DBG && !defined(NDEBUG)

    #define SND_WARN(...) \
        { \
            WCHAR dbg_popup_msg[1024]; \
            wsprintf(dbg_popup_msg, __VA_ARGS__); \
            OutputDebugStringW(dbg_popup_msg); \
        }

    #define SND_TRACE(...) \
        { \
            WCHAR dbg_popup_msg[1024]; \
            wsprintf(dbg_popup_msg, __VA_ARGS__); \
            OutputDebugStringW(dbg_popup_msg); \
        }

    #define DUMP_WAVEHDR_QUEUE(sound_device_instance) \
        { \
            PWAVEHDR CurrDumpHdr = sound_device_instance->HeadWaveHeader; \
            SND_TRACE(L"-- Current wave header list --\n"); \
            while ( CurrDumpHdr ) \
            { \
                SND_TRACE(L"%x | %d bytes | flags: %x\n", CurrDumpHdr, \
                          CurrDumpHdr->dwBufferLength, \
                          CurrDumpHdr->dwFlags); \
                CurrDumpHdr = CurrDumpHdr->lpNext; \
            } \
        }

#else // DBG && !defined(NDEBUG)

    #define SND_WARN(...) do {} while ( 0 )
    #define SND_TRACE(...) do {} while ( 0 )
    #define DUMP_WAVEHDR_QUEUE(sound_device_instance) do {} while ( 0 )

#endif // DBG && !defined(NDEBUG)

#endif /* ROS_AUDIO_MMEBUDDY_DEBUG_H */
