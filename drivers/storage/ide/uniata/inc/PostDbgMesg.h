#ifndef __DBG_DUMP_TOOLS__H__
#define __DBG_DUMP_TOOLS__H__

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

BOOLEAN
_cdecl
DbgDump_Printf(
    PCHAR Format,
    ...
    );

BOOLEAN
__stdcall
DbgDump_Print(
    PCHAR Msg
    );

BOOLEAN
__stdcall
DbgDump_Printn(
    PCHAR Msg,
    ULONG Length
    );

BOOLEAN
__stdcall
DbgDump_Reconnect(VOID);

VOID
__stdcall
DbgDump_Disconnect(VOID);

VOID
__stdcall
DbgDump_SetAutoReconnect(
    BOOLEAN AutoReconnect
    );

#ifdef __cplusplus
};
#endif //__cplusplus

#endif //__DBG_DUMP_TOOLS__H__
