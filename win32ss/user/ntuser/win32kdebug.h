#pragma once

/*
    When a process is created, DbgInitDebugChannels will locate DEBUGCHANNEL
    environment variable and extract information about debug channels.
    This information includes which of the win32k debug channels will be
    enabled for the current process and which level of a channel will be active.
    This information will be stored in ppi->DbgChannelLevel.
    In this way user mode can control how win32k debugging will work when
    the following macros are used: ERR, FIXME, WARN, TRACE

    By default only the ERR channel will be active. Remember that other
    debug channels can be activated for applications that are executed with a DEBUGCHANNEL

    Valid syntax for DEBUGCHANNEL is the following:
    +UserProcess,info+UserWnd,err+UserWndpos,-listview
    warn+UserMsgQ,-UserMsgGet,+shell

    Note the following:
    The debug level is not required
    The operation to enable/disable (+/-) and the name of the channel is required
    Channels are devided by commas
    No spaces are allowed
    The syntax is case sensitive. Levels must be lowercase and
    the names of the channels must be exactly like they are defined in DBG_DEFAULT_CHANNEL
    This syntax can be mixed with wine debug channels without problems

*/

#if DBG

    #if !defined(__RELFILE__)
        #define __RELFILE__ __FILE__
    #endif

    typedef struct
    {
        PWCHAR Name;
        ULONG Id;
    } DBG_CHANNEL;

    /* Note: The following values don't need to be sorted */
    enum _DEBUGCHANNELS
    {
        DbgChEngBlt,
        DbgChEngBrush,
        DbgChEngClip,
        DbgChEngCursor,
        DbgChEngDev,
        DbgChEngErr,
        DbgChEngEvent,
        DbgChEngGrad,
        DbgChEngLDev,
        DbgChEngLine,
        DbgChEngMapping,
        DbgChEngMDev,
        DbgChEngPDev,
        DbgChEngSurface,
        DbgChEngWnd,
        DbgChEngXlate,
        DbgChGdiBitmap,
        DbgChGdiBlt,
        DbgChGdiBrush,
        DbgChGdiClipRgn,
        DbgChGdiCoord,
        DbgChGdiDC,
        DbgChGdiDCAttr,
        DbgChGdiDCState,
        DbgChGdiDev,
        DbgChGdiDib,
        DbgChGdiFont,
        DbgChGdiLine,
        DbgChGdiObj,
        DbgChGdiPalette,
        DbgChGdiPath,
        DbgChGdiPen,
        DbgChGdiPool,
        DbgChGdiRgn,
        DbgChGdiText,
        DbgChGdiXFormObj,
        DbgChUserAccel,
        DbgChUserCallback,
        DbgChUserCallProc,
        DbgChUserCaret,
        DbgChUserClass,
        DbgChUserClipbrd,
        DbgChUserCsr,
        DbgChUserDce,
        DbgChUserDefwnd,
        DbgChUserDesktop,
        DbgChUserDisplay,
        DbgChUserEvent,
        DbgChUserFocus,
        DbgChUserHook,
        DbgChUserHotkey,
        DbgChUserIcon,
        DbgChUserInput,
        DbgChUserKbd,
        DbgChUserKbdLayout,
        DbgChUserMenu,
        DbgChUserMetric,
        DbgChUserMisc,
        DbgChUserMonitor,
        DbgChUserMsg,
        DbgChUserMsgQ,
        DbgChUserObj,
        DbgChUserPainting,
        DbgChUserProcess,
        DbgChUserProp,
        DbgChUserScrollbar,
        DbgChUserSecurity,
        DbgChUserShutdown,
        DbgChUserSysparams,
        DbgChUserThread,
        DbgChUserTimer,
        DbgChUserWinsta,
        DbgChUserWnd,
        DbgChUserWinpos,
        DbgChCount
    };

    #define DISABLED_LEVEL 0x0
    #define ERR_LEVEL      0x1
    #define FIXME_LEVEL    0x2
    #define WARN_LEVEL     0x4
    #define TRACE_LEVEL    0x8

    #define MAX_LEVEL ERR_LEVEL | FIXME_LEVEL | WARN_LEVEL | TRACE_LEVEL

    #define DBG_GET_PPI ((PPROCESSINFO)PsGetCurrentProcessWin32Process())
    #define DBG_DEFAULT_CHANNEL(x) static int DbgDefaultChannel = DbgCh##x;

    #define DBG_ENABLE_CHANNEL(ppi,ch,level) ((ppi)->DbgChannelLevel[ch] |= level)
    #define DBG_DISABLE_CHANNEL(ppi,ch,level) ((ppi)->DbgChannelLevel[ch] &= ~level)
    #define DBG_IS_CHANNEL_ENABLED(ppi,ch,level) (((ppi)->DbgChannelLevel[ch] & level) == level)

    #define DBG_PRINT(ppi,ch,level,fmt, ...)    \
    do {    \
    if ((level == ERR_LEVEL) || (ppi && DBG_IS_CHANNEL_ENABLED(ppi,ch,level)))  \
        DbgPrint("(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__);         \
    } while (0)

    #define ERR(fmt, ...)     DBG_PRINT(DBG_GET_PPI, DbgDefaultChannel, ERR_LEVEL,"err: " fmt, ##__VA_ARGS__)
    #define FIXME(fmt, ...)   DBG_PRINT(DBG_GET_PPI, DbgDefaultChannel, FIXME_LEVEL,"fixme: " fmt, ##__VA_ARGS__)
    #define WARN(fmt, ...)    DBG_PRINT(DBG_GET_PPI, DbgDefaultChannel, WARN_LEVEL,"warn: " fmt, ##__VA_ARGS__)
    #define TRACE(fmt, ...)   DBG_PRINT(DBG_GET_PPI, DbgDefaultChannel, TRACE_LEVEL,"trace: " fmt, ##__VA_ARGS__)

    #define ERR_CH(ch,fmt, ...)        DBG_PRINT(DBG_GET_PPI, DbgCh##ch, ERR_LEVEL, "err: " fmt, ##__VA_ARGS__)
    #define FIXME_CH(ch,fmt, ...)      DBG_PRINT(DBG_GET_PPI, DbgCh##ch, FIXME_LEVEL, "fixme: " fmt, ##__VA_ARGS__)
    #define WARN_CH(ch,fmt, ...)       DBG_PRINT(DBG_GET_PPI, DbgCh##ch, WARN_LEVEL, "warn: " fmt, ##__VA_ARGS__)
    #define TRACE_CH(ch,fmt, ...)      DBG_PRINT(DBG_GET_PPI, DbgCh##ch, TRACE_LEVEL, "trace: " fmt, ##__VA_ARGS__)

    #define ERR_PPI(ppi,ch,fmt, ...)   DBG_PRINT(ppi, DbgCh##ch, ERR_LEVEL,"err: " fmt, ##__VA_ARGS__)
    #define FIXME_PPI(ppi,ch,fmt, ...) DBG_PRINT(ppi, DbgCh##ch, FIXME_LEVEL,"fixme: " fmt, ##__VA_ARGS__)
    #define WARN_PPI(ppi,ch,fmt, ...)  DBG_PRINT(ppi, DbgCh##ch, WARN_LEVEL,"warn: " fmt, ##__VA_ARGS__)
    #define TRACE_PPI(ppi,ch,fmt, ...) DBG_PRINT(ppi, DbgCh##ch, TRACE_LEVEL,"trace: " fmt, ##__VA_ARGS__)

    #define STUB         DbgPrint("WARNING:  %s at %s:%d is UNIMPLEMENTED!\n",__FUNCTION__,__RELFILE__,__LINE__);

#else
    #define DBG_GET_PPI
    #define DBG_DEFAULT_CHANNEL(x)

    #define DBG_ENABLE_CHANNEL(ppi,ch,level)
    #define DBG_DISABLE_CHANNEL(ppi,ch,level)
    #define DBG_IS_CHANNEL_ENABLED(ppi,ch,level)

    #define DBG_PRINT(ppi,ch,level)

    #define ERR(fmt, ...)
    #define FIXME(fmt, ...)
    #define WARN(fmt, ...)
    #define TRACE(fmt, ...)

    #define ERR_CH(ch,fmt, ...)
    #define FIXME_CH(ch,fmt, ...)
    #define WARN_CH(ch,fmt, ...)
    #define TRACE_CH(ch,fmt, ...)

    #define ERR_PPI(ppi,ch,fmt, ...)
    #define FIXME_PPI(ppi,ch,fmt, ...)
    #define WARN_PPI(ppi,ch,fmt, ...)
    #define TRACE_PPI(ppi,ch,fmt, ...)

    #define STUB

#endif

BOOL DbgInitDebugChannels();
