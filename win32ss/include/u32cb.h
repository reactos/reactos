/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Defining kernel-to-user32 callback entries
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

/* DEFINE_USER32_CALLBACK(id, value, fn) */
DEFINE_USER32_CALLBACK(USER32_CALLBACK_WINDOWPROC,              0, User32CallWindowProcFromKernel)
DEFINE_USER32_CALLBACK(USER32_CALLBACK_SENDASYNCPROC,           1, User32CallSendAsyncProcForKernel)
DEFINE_USER32_CALLBACK(USER32_CALLBACK_LOADSYSMENUTEMPLATE,     2, User32LoadSysMenuTemplateForKernel)
DEFINE_USER32_CALLBACK(USER32_CALLBACK_LOADDEFAULTCURSORS,      3, User32SetupDefaultCursors)
DEFINE_USER32_CALLBACK(USER32_CALLBACK_HOOKPROC,                4, User32CallHookProcFromKernel)
DEFINE_USER32_CALLBACK(USER32_CALLBACK_EVENTPROC,               5, User32CallEventProcFromKernel)
DEFINE_USER32_CALLBACK(USER32_CALLBACK_LOADMENU,                6, User32CallLoadMenuFromKernel)
DEFINE_USER32_CALLBACK(USER32_CALLBACK_CLIENTTHREADSTARTUP,     7, User32CallClientThreadSetupFromKernel)
DEFINE_USER32_CALLBACK(USER32_CALLBACK_CLIENTLOADLIBRARY,       8, User32CallClientLoadLibraryFromKernel)
DEFINE_USER32_CALLBACK(USER32_CALLBACK_GETCHARSETINFO,          9, User32CallGetCharsetInfo)
DEFINE_USER32_CALLBACK(USER32_CALLBACK_COPYIMAGE,              10, User32CallCopyImageFromKernel)
DEFINE_USER32_CALLBACK(USER32_CALLBACK_SETWNDICONS,            11, User32CallSetWndIconsFromKernel)
DEFINE_USER32_CALLBACK(USER32_CALLBACK_DELIVERUSERAPC,         12, User32DeliverUserAPC)
DEFINE_USER32_CALLBACK(USER32_CALLBACK_DDEPOST,                13, User32CallDDEPostFromKernel)
DEFINE_USER32_CALLBACK(USER32_CALLBACK_DDEGET,                 14, User32CallDDEGetFromKernel)
DEFINE_USER32_CALLBACK(USER32_CALLBACK_SETOBM,                 15, User32CallOBMFromKernel)
DEFINE_USER32_CALLBACK(USER32_CALLBACK_LPK,                    16, User32CallLPKFromKernel)
DEFINE_USER32_CALLBACK(USER32_CALLBACK_UMPD,                   17, User32CallUMPDFromKernel)
DEFINE_USER32_CALLBACK(USER32_CALLBACK_IMMPROCESSKEY,          18, User32CallImmProcessKeyFromKernel)
DEFINE_USER32_CALLBACK(USER32_CALLBACK_IMMLOADLAYOUT,          19, User32CallImmLoadLayoutFromKernel)