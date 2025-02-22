/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Setup Library
 * FILE:            base/setup/lib/spapisup.h
 * PURPOSE:         Interfacing with Setup* API support functions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

/* Make setupapi.h to not define the API as exports to the DLL */
#ifdef __REACTOS__
#define _SETUPAPI_
#endif

/* Architecture names to be used for architecture-specific INF sections */
#ifdef _M_IX86
#define INF_ARCH L"x86"
#elif defined(_M_AMD64)
#define INF_ARCH L"amd64"
#elif defined(_M_IA64)
#define INF_ARCH L"ia64"
#elif defined(_M_ARM)
#define INF_ARCH L"arm"
#elif defined(_M_PPC)
#define INF_ARCH L"ppc"
#endif

/* EOF */
