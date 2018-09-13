//
// NT5 tracking is compiled into all NT builds, but only runs on
// NT5 by honoring the g_RunOnNT5 bool.
//

#if defined(WINNT)

#define ENABLE_TRACK 1
#define DM_TRACK 0x0100

#endif
