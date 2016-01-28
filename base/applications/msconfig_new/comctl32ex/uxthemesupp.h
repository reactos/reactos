/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig_new/comctl32ex/uxthemesupp.h
 * PURPOSE:     UX Theming helpers.
 * COPYRIGHT:   Copyright 2015 Hermes BELUSCA - MAITO <hermes.belusca@sfr.fr>
 */

#ifndef _UXTHEMESUPP_H_
#define _UXTHEMESUPP_H_

#pragma once

#if defined(_UXTHEME_H) || defined(_UXTHEME_H_) // First one is our headers from Wine/MinGW, second one is MS PSDK
#warning "PSDK header uxtheme.h is already included, you might think about using it instead!"
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
//  Copied from uxtheme.h
//  If you have this new header, then delete these and
//  #include <uxtheme.h> instead!
//

#define ETDT_DISABLE        0x00000001
#define ETDT_ENABLE         0x00000002
#define ETDT_USETABTEXTURE  0x00000004
#define ETDT_ENABLETAB      (ETDT_ENABLE  | ETDT_USETABTEXTURE)

HRESULT
WINAPI
EnableThemeDialogTexture(_In_ HWND  hwnd,
                         _In_ DWORD dwFlags);

HRESULT
WINAPI
SetWindowTheme(_In_ HWND    hwnd,
               _In_ LPCWSTR pszSubAppName,
               _In_ LPCWSTR pszSubIdList);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _UXTHEMESUPP_H_
