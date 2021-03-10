/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxMacros.h

Abstract:

    This file contains macros used by Mx files

Author:



Revision History:



--*/

#pragma once

#if (FX_CORE_MODE==FX_CORE_USER_MODE)
#define CHECK_RETURN_IF_USER_MODE _Must_inspect_result_
#else
#define CHECK_RETURN_IF_USER_MODE
#endif

#ifdef __REACTOS__
# ifndef STDCALL
#  define STDCALL __stdcall
# endif
#endif
