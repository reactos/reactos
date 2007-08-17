/*
  PROJECT:    ReactOS
  LICENSE:    GPL v2 or any later version
  FILE:       include/reactos/typedefs64.h
  PURPOSE:    Type definitions for host tools, which are built on 64-bit systems
  COPYRIGHT:  Copyright 2007 Colin Finck <mail@colinfinck.de>
*/

#ifndef _TYPEDEFS64_H
#define _TYPEDEFS64_H

#ifndef DWORD_DEFINED
#define DWORD_DEFINED
	typedef unsigned int DWORD;
#endif

#ifndef LONG_DEFINED
#define LONG_DEFINED
	typedef int LONG;
	typedef unsigned int ULONG,*PULONG;
#endif

#ifndef LONG_PTR_DEFINED
#define LONG_PTR_DEFINED
	typedef int LONG_PTR, *PLONG_PTR;
	typedef unsigned int ULONG_PTR, *PULONG_PTR;
#endif

#ifndef HANDLE_PTR_DEFINED
#define HANDLE_PTR_DEFINED
	typedef unsigned int HANDLE_PTR;
#endif

#ifndef _ERROR_STATUS_T_DEFINED
#define _ERROR_STATUS_T_DEFINED
	typedef unsigned int error_status_t;
#endif

#endif
