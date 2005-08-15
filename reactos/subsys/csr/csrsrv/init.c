/* $Id$
 *
 * subsys/csr/csrsrv/init.c - CSR server - initialization
 * 
 * ReactOS Operating System
 * 
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 */

#include "srv.h"

//#define NDEBUG
#include <debug.h>


typedef enum {
	CSRAT_UNKNOWN=0,
	CSRAT_OBJECT_DIRECTORY,
	CSRAT_SUBSYSTEM_TYPE,
	CSRAT_REQUEST_THREADS, /* ReactOS extension */
	CSRAT_REQUEST_THREADS_MAX,
	CSRAT_PROFILE_CONTROL,
	CSRAT_SHARED_SECTION,
	CSRAT_SERVER_DLL,
	CSRAT_WINDOWS,
	CSRAT_SESSIONS, /* ReactOS extension */
	CSRAT_MAX
} CSR_ARGUMENT_TYPE, *PCSR_ARGUMENT_TYPE;

typedef struct _CSR_ARGUMENT_ITEM
{
	CSR_ARGUMENT_TYPE Type;
	UNICODE_STRING Data;
	union {
		UNICODE_STRING     ObjectDirectory;
		CSR_SUBSYSTEM_TYPE SubSystemType;
		USHORT             RequestThreads;
		USHORT             MaxRequestThreads;
		BOOL               ProfileControl;
		BOOL               Windows;
		BOOL               Sessions;
		CSR_SERVER_DLL     ServerDll;
		struct {
				   USHORT PortSectionSize;                   // 1024k; 128k..?
				   USHORT InteractiveDesktopHeapSize;    // 3072k; 128k..
				   USHORT NonInteractiveDesktopHeapSize; // (InteractiveDesktopHeapSize); 128k..
				   USHORT Reserved; /* unused */
				 } SharedSection;
	} Item;
	
} CSR_ARGUMENT_ITEM, * PCSR_ARGUMENT_ITEM;

/**********************************************************************
 * CsrpStringToBool/3						PRIVATE
 */
static BOOL STDCALL CsrpStringToBool (LPWSTR TestString, LPWSTR TrueString, LPWSTR FalseString)
{
	if((0 == wcscmp(TestString, TrueString)))
	{
		return TRUE;
	}
	if((0 == wcscmp(TestString, FalseString)))
	{
		return FALSE;
	}
	DPRINT1("CSRSRV:%s: replacing invalid value '%S' with '%S'!\n",
			__FUNCTION__, TestString, FalseString);
	return FALSE;
}
/**********************************************************************
 * CsrpSplitServerDll/2						PRIVATE
 *
 * RETURN VALUE
 * 	0: syntax error
 * 	2: ServerDll=="basesrv,1"
 * 	3: ServerDll=="winsrv:UserServerDllInitialization,3"
 */
static INT STDCALL CsrpSplitServerDll (LPWSTR ServerDll, PCSR_ARGUMENT_ITEM pItem)
{
	LPWSTR DllName = NULL;
	LPWSTR DllEntryPoint = NULL;
	LPWSTR DllId = NULL;
	static LPWSTR DefaultDllEntryPoint = L"ServerDllInitialization";
	LPWSTR tmp = NULL;
	INT rc = 0;
	PCSR_SERVER_DLL pCsrServerDll = & pItem->Item.ServerDll;

	if (L'\0' == *ServerDll)
	{
		return 0;
	}
	/*
	 *	DllName	(required)
	 */
	DllName = ServerDll;
	if (NULL == DllName)
	{
		return 0;
	}
	/*
	 *	DllEntryPoint (optional)
	 */
	DllEntryPoint = wcschr (ServerDll, L':');
	if (NULL == DllEntryPoint)
	{
		DllEntryPoint = DefaultDllEntryPoint;
		tmp = ServerDll;
		rc = 2;
	} else {
		tmp = ++DllEntryPoint;
		rc = 3;
	}
	/*
	 *	DllId (required)
	 */	
	DllId = wcschr (tmp, L',');
	if (NULL == DllId)
	{
		return 0;
	}
	*DllId++ = L'\0';
	// OK
	pCsrServerDll->ServerIndex = wcstoul (DllId, NULL, 10);
	pCsrServerDll->Unused = 0;
	RtlInitUnicodeString (& pCsrServerDll->DllName, DllName);
	RtlInitUnicodeString (& pCsrServerDll->DllEntryPoint, DllEntryPoint);
	return rc;
}
/**********************************************************************
 * CsrpSplitSharedSection/2					PRIVATE
 *
 * RETURN VALUE
 * 	0: syntax error
 * 	1: PortSectionSize (required)
 * 	2: PortSection,InteractiveDesktopHeap
 * 	3: PortSection,InteractiveDesktopHeap,NonInteractiveDesktopHeap
 */
static INT STDCALL CsrpSplitSharedSection (LPWSTR SharedSection, PCSR_ARGUMENT_ITEM pItem)
{
	LPWSTR PortSectionSize = NULL;
	LPWSTR InteractiveDesktopHeapSize = NULL;
	LPWSTR NonInteractiveDesktopHeapSize = NULL;
	INT rc = 1;

	DPRINT("CSRSRV:%s(%S) called\n", __FUNCTION__, SharedSection);

	if(L'\0' == *SharedSection)
	{
		DPRINT("CSRSRV:%s(%S): *SharedSection == L'\\0'\n", __FUNCTION__, SharedSection);
		return 0;
	}

	// PortSectionSize (required)
	PortSectionSize = SharedSection;
	// InteractiveDesktopHeapSize (optional)
	InteractiveDesktopHeapSize = wcschr (PortSectionSize, L',');
	if (NULL == InteractiveDesktopHeapSize)
	{
		// Default value is 128k
		InteractiveDesktopHeapSize = L"128";
	} else {
		rc = 2;
	}
	// NonInteractiveDesktopHeapSize (optional)
	NonInteractiveDesktopHeapSize = wcschr (InteractiveDesktopHeapSize, L',');
	if (NULL == NonInteractiveDesktopHeapSize)
	{
		// Default value equals interactive one
		NonInteractiveDesktopHeapSize = InteractiveDesktopHeapSize;
	} else {
		rc = 3;
	}
	// OK - normalization
	pItem->Item.SharedSection.PortSectionSize = wcstoul (PortSectionSize, NULL, 10);
	if (pItem->Item.SharedSection.PortSectionSize < 64)
	{
		pItem->Item.SharedSection.PortSectionSize = 64;
	}
	pItem->Item.SharedSection.InteractiveDesktopHeapSize = wcstoul (InteractiveDesktopHeapSize, NULL, 10);
	if (pItem->Item.SharedSection.InteractiveDesktopHeapSize < 128)
	{
		pItem->Item.SharedSection.InteractiveDesktopHeapSize = 128;
	}
	pItem->Item.SharedSection.NonInteractiveDesktopHeapSize = wcstoul (NonInteractiveDesktopHeapSize, NULL, 10);
	if (pItem->Item.SharedSection.NonInteractiveDesktopHeapSize < 128)
	{
		pItem->Item.SharedSection.NonInteractiveDesktopHeapSize = 128;
	}
	// done
	return rc;
}
/**********************************************************************
 * CsrpParseArgumentItem/1					PRIVATE
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 * 	Argument: argument to decode;
 *
 * RETURN VALUE
 * 	STATUS_SUCCESS; otherwise, STATUS_UNSUCCESSFUL and
 * 	pItem->Type = CSRAT_UNKNOWN.
 *
 * NOTE
 * 	The command line could be as complex as the following one,
 * 	which is the original command line for the Win32 subsystem
 * 	in NT 5.1:
 * 	
 *	%SystemRoot%\system32\csrss.exe
 *		ObjectDirectory=\Windows
 *		SharedSection=1024,3072,512
 *		Windows=On
 *		SubSystemType=Windows
 *		ServerDll=basesrv,1
 *		ServerDll=winsrv:UserServerDllInitialization,3
 *		ServerDll=winsrv:ConServerDllInitialization,2
 *		ProfileControl=Off
 *		MaxRequestThreads=16
 */
static NTSTATUS FASTCALL CsrpParseArgumentItem (IN OUT PCSR_ARGUMENT_ITEM pItem)
{
	NTSTATUS Status = STATUS_SUCCESS;
	LPWSTR   ParameterName = NULL;
	LPWSTR   ParameterValue = NULL;

	pItem->Type = CSRAT_UNKNOWN;

	if(0 == pItem->Data.Length)
	{
		DPRINT1("CSRSRV:%s: (0 == Data.Length)!\n", __FUNCTION__);
		return STATUS_INVALID_PARAMETER;
	}
	//--- Seek '=' to split name and value
	ParameterName = pItem->Data.Buffer;
	ParameterValue = wcschr (ParameterName, L'=');
	if (NULL == ParameterValue)
	{
		DPRINT1("CSRSRV:%s: (NULL == ParameterValue)!\n", __FUNCTION__);
		return STATUS_INVALID_PARAMETER;
	}
	*ParameterValue++ = L'\0';
	DPRINT("Name=%S, Value=%S\n", ParameterName, ParameterValue);
	//---
	if(0 == wcscmp(ParameterName, L"ObjectDirectory"))
	{
		RtlInitUnicodeString (& pItem->Item.ObjectDirectory, ParameterValue);
		pItem->Type = CSRAT_OBJECT_DIRECTORY;
	}
	else if(0 == wcscmp(ParameterName, L"SubSystemType"))
	{
		pItem->Type = CSRAT_SUBSYSTEM_TYPE;
		pItem->Item.Windows = CsrpStringToBool (ParameterValue, L"Windows", L"Text");
	}
	else if(0 == wcscmp(ParameterName, L"MaxRequestThreads"))
	{
		pItem->Item.MaxRequestThreads = (USHORT) wcstoul (ParameterValue, NULL, 10);
		pItem->Type = CSRAT_REQUEST_THREADS_MAX;
	}
	else if(0 == wcscmp(ParameterName, L"RequestThreads"))
	{
		// ROS Extension
		pItem->Item.RequestThreads = (USHORT) wcstoul (ParameterValue, NULL, 10);
		pItem->Type = CSRAT_REQUEST_THREADS;
	}
	else if(0 == wcscmp(ParameterName, L"ProfileControl"))
	{
		pItem->Item.ProfileControl = CsrpStringToBool (ParameterValue, L"On", L"Off");
		pItem->Type = CSRAT_PROFILE_CONTROL;
	}
	else if(0 == wcscmp(ParameterName, L"SharedSection"))
	{
		if (0 != CsrpSplitSharedSection(ParameterValue, pItem))
		{
			pItem->Type = CSRAT_SHARED_SECTION;
		} else {
			pItem->Type = CSRAT_UNKNOWN;
			return STATUS_INVALID_PARAMETER;
		}
	}
	else if(0 == wcscmp(ParameterName, L"ServerDll"))
	{
		if (0 != CsrpSplitServerDll(ParameterValue, pItem))
		{
			pItem->Type = CSRAT_SERVER_DLL;
		} else {
			pItem->Type = CSRAT_UNKNOWN;
			return STATUS_INVALID_PARAMETER;
		}
	}
	else if(0 == wcscmp(ParameterName, L"Windows"))
	{
		pItem->Item.Windows = CsrpStringToBool (ParameterValue, L"On", L"Off");
		pItem->Type = CSRAT_WINDOWS;
	}
	else if(0 == wcscmp(ParameterName, L"Sessions"))
	{
		// ROS Extension
		pItem->Item.Sessions = CsrpStringToBool (ParameterValue, L"On", L"Off");
		pItem->Type = CSRAT_SESSIONS;
	}
	else
	{
		DPRINT1("CSRSRV:%s: unknown parameter '%S'!\n", __FUNCTION__, ParameterName);
		pItem->Type = CSRAT_UNKNOWN;
		Status = STATUS_INVALID_PARAMETER;
        }
	return Status;
}
/**********************************************************************
 * CsrServerInitialization/2
 *
 * DESCRIPTION
 * 	Every environment subsystem implicitly starts where this 
 * 	routines stops. This routine is called by CSR on startup
 * 	and then it calls the entry points in the following server
 * 	DLLs, as per command line.
 *
 * ARGUMENTS
 *	ArgumentCount:
 *	Argument:
 *
 * RETURN VALUE
 * 	STATUS_SUCCESS if it succeeds. Otherwise a status code.
 *
 * NOTE
 * 	This is the only function explicitly called by csr.exe.
 */
NTSTATUS STDCALL CsrServerInitialization (ULONG ArgumentCount,
					  LPWSTR *Argument)
{
	NTSTATUS           Status = STATUS_SUCCESS;
	ULONG              ArgumentIndex = 0;
	CSR_ARGUMENT_ITEM  ArgumentItem = {CSRAT_UNKNOWN,};

	// get registry bootstrap options
	for (ArgumentIndex = 0; ArgumentIndex < ArgumentCount; ArgumentIndex++)
	{
		RtlInitUnicodeString (& ArgumentItem.Data, Argument[ArgumentIndex]);
		Status = CsrpParseArgumentItem (& ArgumentItem);
		if (NT_SUCCESS(Status))
		{
			switch (ArgumentItem.Type)
			{
			case CSRAT_UNKNOWN:
				// ignore unknown parameters
				DPRINT1("CSRSRV: ignoring param '%s'\n", Argument[ArgumentIndex]);
				break;
			case CSRAT_OBJECT_DIRECTORY:
				RtlDuplicateUnicodeString (1, & ArgumentItem.Item.ObjectDirectory, & CsrSrvOption.NameSpace.Root);
				DPRINT("ObjectDirectory: '%S'\n", CsrSrvOption.NameSpace.Root.Buffer);
				break;
			case CSRAT_SUBSYSTEM_TYPE:
				CsrSrvOption.SubSystemType = ArgumentItem.Item.SubSystemType;
				DPRINT("SubSystemType: %u\n", CsrSrvOption.SubSystemType);
				break;
			case CSRAT_REQUEST_THREADS:
				CsrSrvOption.Threads.RequestCount = ArgumentItem.Item.RequestThreads;
				DPRINT("RequestThreads: %u\n", CsrSrvOption.Threads.RequestCount);
				break;
			case CSRAT_REQUEST_THREADS_MAX:
				CsrSrvOption.Threads.MaxRequestCount = ArgumentItem.Item.MaxRequestThreads;
				DPRINT("MaxRequestThreads: %u\n", CsrSrvOption.Threads.MaxRequestCount);
				break;
			case CSRAT_PROFILE_CONTROL:
				CsrSrvOption.Flag.ProfileControl = ArgumentItem.Item.ProfileControl;
				DPRINT("ProfileControl: %u \n", CsrSrvOption.Flag.ProfileControl);
				break;
			case CSRAT_SHARED_SECTION:
				CsrSrvOption.PortSharedSectionSize              = ArgumentItem.Item.SharedSection.PortSectionSize;
				CsrSrvOption.Heap.InteractiveDesktopHeapSize    = ArgumentItem.Item.SharedSection.InteractiveDesktopHeapSize;
				CsrSrvOption.Heap.NonInteractiveDesktopHeapSize = ArgumentItem.Item.SharedSection.NonInteractiveDesktopHeapSize;
				DPRINT("SharedSection: %u-%u-%u\n",
						CsrSrvOption.PortSharedSectionSize,
						CsrSrvOption.Heap.InteractiveDesktopHeapSize,
						CsrSrvOption.Heap.NonInteractiveDesktopHeapSize);
				break;
			case CSRAT_SERVER_DLL:
				Status = CsrSrvRegisterServerDll (& ArgumentItem.Item.ServerDll);
				if(!NT_SUCCESS(Status))
				{
					DPRINT1("CSRSRV: CsrSrvRegisterServerDll(%S) failed!\n",
						Argument[ArgumentIndex]);
				} else {
					DPRINT("ServerDll: DLL='%S' Entrypoint='%S' ID=%u\n",
						ArgumentItem.Item.ServerDll.DllName.Buffer,
						ArgumentItem.Item.ServerDll.DllEntryPoint.Buffer,
						ArgumentItem.Item.ServerDll.ServerIndex);
				}
				break;
			case CSRAT_WINDOWS:
				CsrSrvOption.Flag.Windows = ArgumentItem.Item.Windows;
				DPRINT("Windows: %d\n", CsrSrvOption.Flag.Windows);
				break;
			case CSRAT_SESSIONS:
				CsrSrvOption.Flag.Sessions = ArgumentItem.Item.Sessions;
				DPRINT("Sessions: %d\n", CsrSrvOption.Flag.Sessions);
				break;
			default:
				DPRINT("CSRSRV: unknown ArgumentItem->Type=%ld!\n", ArgumentItem.Type);
			}
		} else {
			DPRINT1("CSRSRV:%s: CsrpParseArgumentItem(%S) failed with Status = %08lx\n",
					__FUNCTION__, Argument[ArgumentIndex], Status);
		}
	}
	// TODO: verify required 
	Status = CsrSrvBootstrap ();
	return Status;
}
/* EOF */
