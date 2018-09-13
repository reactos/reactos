//****************************************************************************
//
//  Module:     RNAUI.DLL
//  File:       rnahelp.h
//  Content:    This file contains the context-sensitive help header.
//  History:
//      Sun 03-Jul-1994 15:22:54  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1991-1994
//
//****************************************************************************

#ifdef SCRPT_HELP_ENABLED
// RNA Scripting ID's

#define IDH_SCRIPT_FILENAME		4302
#define IDH_SCRIPT_BROWSE		4303
#define IDH_SCRIPT_EDIT			4304
#define IDH_SCRIPT_STEPTHROUGH		4305
#define IDH_SCRIPT_STARTTERMINAL	4306
#define IDH_SCRIPT_HELP                 4307

#define CREATE_SCRIPT_MAIN              4313
#endif // SCRPT_HELP_ENABLED

//****************************************************************************
// Context-sentive help/control mapping arrays
//****************************************************************************

extern DWORD const gaABEntry[];
extern DWORD const gaABMLEntry[];
extern DWORD const gaSubEntry[];
extern DWORD const gaEditSub[];
extern DWORD const gaSettings[];
extern DWORD const gaScripter[];
extern DWORD const gaConfirm[];

void NEAR PASCAL ContextHelp (DWORD const *aHelp, UINT uMsg, WPARAM wParam,
                              LPARAM lParam);
void NEAR PASCAL WhatNextHelp (HWND hWnd);
