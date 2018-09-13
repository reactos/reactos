//*************************************************************
//  File name: Terminal.h
//
//  Description:  Header file for terminal.c
//
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1991-1996
//  All rights reserved
//
//*************************************************************


PTERMINAL WLCreateTerminal (HANDLE hTerminal, LPWSTR lpTerminalName);
VOID WLDestroyTerminal (PTERMINAL pTerminal);
PWINDOWSTATION WLCreateWinlogonWindowStation (PTERMINAL pTerm, LPWSTR lpWindowStationName);
VOID WLDestroyWindowStation (PTERMINAL pTerm, PWINDOWSTATION pWindowStation);
HDESK WLCreateDesktop (HANDLE hWindowStation, LPWSTR lpDesktopName, DWORD dwSectionSize);

