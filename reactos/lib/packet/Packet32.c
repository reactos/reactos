/*
 * Copyright (c) 1999, 2000
 *	Politecnico di Torino.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the Politecnico
 * di Torino, and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#define UNICODE 1

#include <windows.h>
#include <net/ntddndis.h>

#include <packet32.h>
#include "trace.h"


/****** KERNEL Macro APIs ******************************************************/

#define GetInstanceModule(hInst)                (HMODULE)(hInst)
#define GlobalPtrHandle(lp)                     ((HGLOBAL)GlobalHandle(lp))
#define GlobalLockPtr(lp)                       ((BOOL)GlobalLock(GlobalPtrHandle(lp)))
#define GlobalUnlockPtr(lp)                     GlobalUnlock(GlobalPtrHandle(lp))
#define GlobalAllocPtr(flags, cb)               (GlobalLock(GlobalAlloc((flags), (cb))))
#define GlobalReAllocPtr(lp, cbNew, flags)      (GlobalUnlockPtr(lp), GlobalLock(GlobalReAlloc(GlobalPtrHandle(lp) , (cbNew), (flags))))
#define GlobalFreePtr(lp)                       (GlobalUnlockPtr(lp), (BOOL)(ULONG_PTR)GlobalFree(GlobalPtrHandle(lp)))

#undef GMEM_MOVEABLE
#define GMEM_MOVEABLE 0



/// Title of error windows
TCHAR   szWindowTitle[] = TEXT("PACKET.DLL");

#if DBG
#define ODS(_x) OutputDebugString(TEXT(_x))
//#define ODSEx(_x, _y)
#define ODSEx TRACE
#else
#ifdef _DEBUG_TO_FILE
#include <stdio.h>
/*! 
  \brief Macro to print a debug string. The behavior differs depending on the debug level
*/
#define ODS(_x) { \
	FILE *f; \
	f = fopen("winpcap_debug.txt", "a"); \
	fprintf(f, "%s", _x); \
	fclose(f); \
}
/*! 
  \brief Macro to print debug data with the printf convention. The behavior differs depending on 
  the debug level
*/
#define ODSEx(_x, _y) { \
	FILE *f; \
	f = fopen("winpcap_debug.txt", "a"); \
	fprintf(f, _x, _y); \
	fclose(f); \
}

LONG PacketDumpRegistryKey(PCHAR KeyName, PCHAR FileName);
#else
#define ODS(_x)		
#define ODSEx(_x, _y)
#endif
#endif

//service handles
SC_HANDLE scmHandle = NULL;
SC_HANDLE srvHandle = NULL;
LPCTSTR NPFServiceName = TEXT("NPF");
LPCTSTR NPFServiceDesc = TEXT("Netgroup Packet Filter");
LPCTSTR NPFDriverName = TEXT("\\npf.sys");
LPCTSTR NPFRegistryLocation = TEXT("SYSTEM\\ControlSet001\\Services\\NPF");


//---------------------------------------------------------------------------

/*! 
  \brief The main dll function.
*/

BOOL APIENTRY DllMain (HANDLE DllHandle,DWORD Reason,LPVOID lpReserved)
{
    BOOLEAN Status=TRUE;

    switch ( Reason )
    {
	case DLL_PROCESS_ATTACH:
		
		ODS("\n************Packet32: DllMain************\n");
		
#ifdef _DEBUG_TO_FILE
		// dump a bunch of registry keys useful for debug to file
		PacketDumpRegistryKey("HKEY_LOCAL_MACHINE\\SYSTEM\\ControlSet001\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}",
			"adapters.reg");
		PacketDumpRegistryKey("HKEY_LOCAL_MACHINE\\SYSTEM\\ControlSet001\\Services\\Tcpip",
			"tcpip.reg");
		PacketDumpRegistryKey("HKEY_LOCAL_MACHINE\\SYSTEM\\ControlSet001\\Services\\NPF",
			"npf.reg");
		PacketDumpRegistryKey("HKEY_LOCAL_MACHINE\\SYSTEM\\ControlSet001\\Services",
			"services.reg");
#endif
		break;

        case DLL_PROCESS_DETACH:
			break;

		default:
            break;
    }

    return Status;
}

/*! 
  \brief Converts an ASCII string to UNICODE. Uses the MultiByteToWideChar() system function.
  \param string The string to convert.
  \return The converted string.
*/

WCHAR* SChar2WChar(char* string)
{
	WCHAR* TmpStr;
	TmpStr=(WCHAR*) malloc ((strlen(string)+2)*sizeof(WCHAR));

	MultiByteToWideChar(CP_ACP, 0, string, -1, TmpStr, (strlen(string)+2));

	return TmpStr;
}

/*! 
  \brief Sets the maximum possible lookahead buffer for the driver's Packet_tap() function.
  \param AdapterObject Handle to the service control manager.
  \return If the function succeeds, the return value is nonzero.

  The lookahead buffer is the portion of packet that Packet_tap() can access from the NIC driver's memory
  without performing a copy. This function tries to increase the size of that buffer.
*/

BOOLEAN PacketSetMaxLookaheadsize (LPADAPTER AdapterObject)
{
    BOOLEAN    Status;
    ULONG      IoCtlBufferLength=(sizeof(PACKET_OID_DATA)+sizeof(ULONG)-1);
    PPACKET_OID_DATA  OidData;

    OidData=GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,IoCtlBufferLength);
    if (OidData == NULL) {
        ODS("PacketSetMaxLookaheadsize failed\n");
        return FALSE;
    }

	//set the size of the lookahead buffer to the maximum available by the the NIC driver
    OidData->Oid=OID_GEN_MAXIMUM_LOOKAHEAD;
    OidData->Length=sizeof(ULONG);
    Status=PacketRequest(AdapterObject,FALSE,OidData);
    OidData->Oid=OID_GEN_CURRENT_LOOKAHEAD;
    Status=PacketRequest(AdapterObject,TRUE,OidData);
    GlobalFreePtr(OidData);
    return Status;
}

/*! 
  \brief Retrieves the event associated in the driver with a capture instance and stores it in an 
   _ADAPTER structure.
  \param AdapterObject Handle to the service control manager.
  \return If the function succeeds, the return value is nonzero.

  This function is used by PacketOpenAdapter() to retrieve the read event from the driver by means of an ioctl
  call and set it in the _ADAPTER structure pointed by AdapterObject.
*/

BOOLEAN PacketSetReadEvt(LPADAPTER AdapterObject)
{
	DWORD BytesReturned;
	WCHAR EventName[39];

	// this tells the terminal service to retrieve the event from the global namespace
	wcsncpy(EventName,L"Global\\",sizeof(L"Global\\"));

	// retrieve the name of the shared event from the driver
	if(DeviceIoControl(AdapterObject->hFile,pBIOCEVNAME,NULL,0,EventName+7,13*sizeof(TCHAR),&BytesReturned,NULL)==FALSE) return FALSE;

	EventName[20]=0; // terminate the string

	// open the shared event
	AdapterObject->ReadEvent=CreateEventW(NULL,
										 TRUE,
										 FALSE,
										 EventName);

	// in NT4 "Global\" is not automatically ignored: try to use simply the event name
	if(GetLastError()!=ERROR_ALREADY_EXISTS){
		if(AdapterObject->ReadEvent != NULL)
			CloseHandle(AdapterObject->ReadEvent);
		
		// open the shared event
		AdapterObject->ReadEvent=CreateEventW(NULL,
			TRUE,
			FALSE,
			EventName+7);
	}	

	if(AdapterObject->ReadEvent==NULL || GetLastError()!=ERROR_ALREADY_EXISTS){
        ODS("PacketSetReadEvt: error retrieving the event from the kernel\n");
		return FALSE;
	}

	AdapterObject->ReadTimeOut=0;

	return TRUE;

}

/*! 
  \brief Installs the NPF device driver.
  \param ascmHandle Handle to the service control manager.
  \param ascmHandle A pointer to a handle that will receive the pointer to the driver's service.
  \param driverPath The full path of the .sys file to load.
  \return If the function succeeds, the return value is nonzero.

  This function installs the driver's service in the system using the CreateService function.
*/

BOOL PacketInstallDriver(SC_HANDLE ascmHandle, SC_HANDLE* srvHandle, TCHAR* driverPath)
{
	BOOL result = FALSE;
	ULONG err;
	
	ODS("installdriver\n");
	
	if (GetFileAttributes(driverPath) != 0xffffffff) {
		*srvHandle = CreateService(ascmHandle, 
			NPFServiceName,
			NPFServiceDesc,
			SERVICE_ALL_ACCESS,
			SERVICE_KERNEL_DRIVER,
			SERVICE_DEMAND_START,
			SERVICE_ERROR_NORMAL,
			driverPath,
			NULL, NULL, NULL, NULL, NULL);
		if (*srvHandle == NULL) {
			if (GetLastError() == ERROR_SERVICE_EXISTS) {
				//npf.sys already existed
				result = TRUE;
			}
		} else {
			//Created service for npf.sys
			result = TRUE;
		}
	}
	if (result == TRUE) {
		if (*srvHandle != NULL) {
			CloseServiceHandle(*srvHandle);
		}
	}

	if (result == FALSE){
		err = GetLastError();
		if (err != 2) {
			ODSEx("PacketInstallDriver failed, Error=%d\n",err);
		}
	}
	return result;
	
}

/*! 
  \brief Convert a Unicode dotted-quad to a 32-bit IP address.
  \param cp A string containing the address.
  \return the converted 32-bit numeric address.

   Doesn't check to make sure the address is valid.
*/


ULONG inet_addrU(const WCHAR *cp)
{
	ULONG val, part;
	WCHAR c;
	int i;

	val = 0;
	for (i = 0; i < 4; i++) {
		part = 0;
		while ((c = *cp++) != '\0' && c != '.') {
			if (c < '0' || c > '9')
				return -1;
			part = part*10 + (c - '0');
		}
		if (part > 255)
			return -1;	
		val = val | (part << i*8);
		if (i == 3) {
			if (c != '\0')
				return -1;	// extra gunk at end of string 
		} else {
			if (c == '\0')
				return -1;	// string ends early 
		}
	}
	return val;
}

/*! 
  \brief Dumps a registry key to disk in text format. Uses regedit.
  \param KeyName Name of the ket to dump. All its subkeys will be saved recursively.
  \param FileName Name of the file that will contain the dump.
  \return If the function succeeds, the return value is nonzero.

  For debugging purposes, we use this function to obtain some registry keys from the user's machine.
*/

#ifdef _DEBUG_TO_FILE

LONG PacketDumpRegistryKey(PCHAR KeyName, PCHAR FileName)
{
	CHAR Command[256];

	strcpy(Command, "regedit /e ");
	strcat(Command, FileName);
	strcat(Command, " ");
	strcat(Command, KeyName);

	/// Let regedit do the dirt work for us
	system(Command);

	return TRUE;
}
#endif

//---------------------------------------------------------------------------
// PUBLIC API
//---------------------------------------------------------------------------

/** @ingroup packetapi
 *  @{
 */

/** @defgroup packet32 Packet.dll exported functions and variables
 *  @{
 */

/// Current packet.dll Version. It can be retrieved directly or through the PacketGetVersion() function.
char PacketLibraryVersion[] = "2.3"; 

/*! 
  \brief Returns a string with the dll version.
  \return A char pointer to the version of the library.
*/
PCHAR PacketGetVersion(){
	return PacketLibraryVersion;
}

/*! 
  \brief Returns information about the MAC type of an adapter.
  \param AdapterObject The adapter on which information is needed.
  \param type Pointer to a NetType structure that will be filled by the function.
  \return If the function succeeds, the return value is nonzero, otherwise the return value is zero.

  This function return the link layer technology and the speed (in bps) of an opened adapter.
  The LinkType field of the type parameter can have one of the following values:

  - NdisMedium802_3: Ethernet (802.3) 
  - NdisMediumWan: WAN 
  - NdisMedium802_5: Token Ring (802.5) 
  - NdisMediumFddi: FDDI 
  - NdisMediumAtm: ATM 
  - NdisMediumArcnet878_2: ARCNET (878.2) 
*/
BOOLEAN PacketGetNetType (LPADAPTER AdapterObject,NetType *type)
{
    BOOLEAN    Status;
    ULONG      IoCtlBufferLength=(sizeof(PACKET_OID_DATA)+sizeof(ULONG)-1);
    PPACKET_OID_DATA  OidData;

    OidData=GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,IoCtlBufferLength);
    if (OidData == NULL) {
        ODS("PacketGetNetType failed\n");
        return FALSE;
    }
	//get the link-layer type
    OidData->Oid = OID_GEN_MEDIA_IN_USE;
    OidData->Length = sizeof (ULONG);
    Status = PacketRequest(AdapterObject,FALSE,OidData);
    type->LinkType=*((UINT*)OidData->Data);

	//get the link-layer speed
    OidData->Oid = OID_GEN_LINK_SPEED;
    OidData->Length = sizeof (ULONG);
    Status = PacketRequest(AdapterObject,FALSE,OidData);
	type->LinkSpeed=*((UINT*)OidData->Data)*100;
    GlobalFreePtr (OidData);

	ODSEx("Media:%d ",type->LinkType);
	ODSEx("Speed=%d\n",type->LinkSpeed);

    return Status;
}

/*! 
  \brief Stops and unloads the WinPcap device driver.
  \return If the function succeeds, the return value is nonzero, otherwise it is zero.

  This function can be used to unload the driver from memory when the application no more needs it.
  Note that the driver is physically stopped and unloaded only when all the files on its devices 
  are closed, i.e. when all the applications that use WinPcap close all their adapters.
*/
BOOL PacketStopDriver()
{
	SC_HANDLE		scmHandle;
    SC_HANDLE       schService;
    BOOL            ret;
    SERVICE_STATUS  serviceStatus;

	scmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	
	if(scmHandle != NULL){
		
		schService = OpenService (scmHandle,
			NPFServiceName,
			SERVICE_ALL_ACCESS
			);
		
		if (schService != NULL)
		{
			
			ret = ControlService (schService,
				SERVICE_CONTROL_STOP,
				&serviceStatus
				);
			if (!ret)
			{
			}
			
			CloseServiceHandle (schService);
			
			CloseServiceHandle(scmHandle);
			
			return ret;
		}
	}
	
	return FALSE;

}

/*! 
  \brief Opens an adapter.
  \param AdapterName A string containing the name of the device to open. 
   Use the PacketGetAdapterNames() function to retrieve the list of available devices.
  \return If the function succeeds, the return value is the pointer to a properly initialized ADAPTER object,
   otherwise the return value is NULL.

  This function tries to load and start the packet driver at the first invocation. In this way, 
  the management of the driver is transparent to the application, that simply needs to open an adapter to start
  WinPcap.
  
  \note the Windows 95 version of the NPF driver works with the ASCII string format, while the Windows NT 
  version works with UNICODE. Therefore, AdapterName \b should be an ASCII string in Windows 95, and a UNICODE 
  string in Windows NT. This difference is not a problem if the string pointed by AdapterName was obtained 
  through the PacketGetAdapterNames function, because it returns the names of the adapters in the proper format.
  Problems can arise in Windows NT when the string is obtained from ANSI C functions like scanf, because they 
  use the ASCII format. Since this could be a relevant problem during the porting of command-line applications 
  from UNIX, we included in the Windows NT version of PacketOpenAdapter the ability to detect ASCII strings and
  convert them to UNICODE before sending them to the device driver. Therefore PacketOpenAdapter in Windows NT 
  accepts both the ASCII and the UNICODE format. If a ASCII string is received, it is converted to UNICODE 
  by PACKET.DLL before being passed to the driver.
*/
LPADAPTER PacketOpenAdapter(LPTSTR AdapterName)
{
    LPADAPTER lpAdapter;
    BOOLEAN Result;
	char *AdapterNameA;
	WCHAR *AdapterNameU;
	DWORD error;
	SC_HANDLE svcHandle = NULL;
	TCHAR driverPath[512];
	TCHAR WinPath[256];
	LONG KeyRes;
	HKEY PathKey;
	SERVICE_STATUS SStat;
	BOOLEAN QuerySStat;
	WCHAR SymbolicLink[128];

    ODSEx("PacketOpenAdapter: trying to open the adapter=%S\n",AdapterName);

	scmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if(scmHandle == NULL){
		error = GetLastError();
		ODSEx("OpenSCManager failed! Error=%d\n", error);
	} else {
		*driverPath = 0;
		GetCurrentDirectory(512, driverPath);
		wsprintf(driverPath + wcslen(driverPath), NPFDriverName);
		
		// check if the NPF registry key is already present
		// this means that the driver is already installed and that we don't need to call PacketInstallDriver
		KeyRes=RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			NPFRegistryLocation,
			0,
			KEY_READ,
			&PathKey);
		
		if(KeyRes != ERROR_SUCCESS){
			Result = PacketInstallDriver(scmHandle,&svcHandle,driverPath);
		} else {
			Result = TRUE;
			RegCloseKey(PathKey);
		}
		
		if (Result) {
			
			srvHandle = OpenService(scmHandle, NPFServiceName, SERVICE_START | SERVICE_QUERY_STATUS );
			if (srvHandle != NULL){
				
				QuerySStat = QueryServiceStatus(srvHandle, &SStat);
				ODSEx("The status of the driver is:%d\n",SStat.dwCurrentState);
				
				if (!QuerySStat || SStat.dwCurrentState != SERVICE_RUNNING){
					ODS("Calling startservice\n");
					if (StartService(srvHandle, 0, NULL)==0){ 
						error = GetLastError();
						if (error!=ERROR_SERVICE_ALREADY_RUNNING && error!=ERROR_ALREADY_EXISTS){
							SetLastError(error);
							if (scmHandle != NULL) CloseServiceHandle(scmHandle);
							error = GetLastError();
							ODSEx("PacketOpenAdapter: StartService failed, Error=%d\n",error);
							return NULL;
						}
					}				
				}
			} else {
				error = GetLastError();
				ODSEx("OpenService failed! Error=%d", error);
			}
		} else {
			if (GetSystemDirectory(WinPath, sizeof(WinPath)/sizeof(TCHAR)) == 0) {
				return FALSE;
			}
			wsprintf(driverPath, TEXT("%s\\drivers%s"), WinPath, NPFDriverName);
			
			if (KeyRes != ERROR_SUCCESS) {
				Result = PacketInstallDriver(scmHandle,&svcHandle,driverPath);
			} else {
				Result = TRUE;
			}
			if (Result) {
				srvHandle = OpenService(scmHandle,NPFServiceName,SERVICE_START);
				if (srvHandle != NULL) {
					QuerySStat = QueryServiceStatus(srvHandle, &SStat);
					ODSEx("The status of the driver is:%d\n",SStat.dwCurrentState);
					if (!QuerySStat || SStat.dwCurrentState != SERVICE_RUNNING) {
						ODS("Calling startservice\n");
						if (StartService(srvHandle, 0, NULL) == 0) { 
							error = GetLastError();
							if (error != ERROR_SERVICE_ALREADY_RUNNING && error!=ERROR_ALREADY_EXISTS) {
								SetLastError(error);
								if (scmHandle != NULL) CloseServiceHandle(scmHandle);
								ODSEx("PacketOpenAdapter: StartService failed, Error=%d\n",error);
								return NULL;
							}
						}
					}
				} else {
					error = GetLastError();
					ODSEx("OpenService failed! Error=%d", error);
				}
			}
		}
	}
    if (scmHandle != NULL) CloseServiceHandle(scmHandle);

	AdapterNameA = (char*)AdapterName;
	if (AdapterNameA[1] != 0) {  // ASCII
		AdapterNameU = SChar2WChar(AdapterNameA);
		AdapterName = AdapterNameU;
	} else {			         // Unicode
		AdapterNameU = NULL;
	}
	
	lpAdapter = (LPADAPTER)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,sizeof(ADAPTER));
	if (lpAdapter == NULL) {
		ODS("PacketOpenAdapter: GlobalAlloc Failed\n");
		error = GetLastError();
		if (AdapterNameU != NULL) free(AdapterNameU);
		//set the error to the one on which we failed
		SetLastError(error);
	    ODS("PacketOpenAdapter: Failed to allocate the adapter structure\n");
		return NULL;
	}
	lpAdapter->NumWrites = 1;

	wsprintf(SymbolicLink,TEXT("\\\\.\\%s%s"), DOSNAMEPREFIX, &AdapterName[8]);
	
	// Copy  only the bytes that fit in the adapter structure.
	// Note that lpAdapter->SymbolicLink is present for backward compatibility but will
	// never be used by the apps
	memcpy(lpAdapter->SymbolicLink, (PCHAR)SymbolicLink, MAX_LINK_NAME_LENGTH);

	//try if it is possible to open the adapter immediately
	lpAdapter->hFile = CreateFile(SymbolicLink,GENERIC_WRITE | GENERIC_READ,0,NULL,OPEN_EXISTING,0,0);
	
	if (lpAdapter->hFile != INVALID_HANDLE_VALUE) {
	    ODSEx("PacketOpenAdapter: CreateFile(%S) successfull\n", SymbolicLink);
		if (PacketSetReadEvt(lpAdapter) == FALSE) {
			error = GetLastError();
			ODS("PacketOpenAdapter: Unable to open the read event\n");
			if (AdapterNameU != NULL)
				free(AdapterNameU);
			GlobalFreePtr(lpAdapter);
			//set the error to the one on which we failed
			SetLastError(error);
		    ODSEx("PacketOpenAdapter: PacketSetReadEvt failed, Error=%d\n",error);
			return NULL;
		}		
		
		PacketSetMaxLookaheadsize(lpAdapter);
		if (AdapterNameU != NULL)
		    free(AdapterNameU);
		return lpAdapter;
	}
	//this is probably the first request on the packet driver. 
	//We must create the dos device and set the access rights on it
	else {
		Result = DefineDosDevice(DDD_RAW_TARGET_PATH,
                                 &SymbolicLink[4],
                                 AdapterName);
		if (Result) {

		    ODSEx("PacketOpenAdapter: calling CreateFile(%S)\n", SymbolicLink);
			
			lpAdapter->hFile = CreateFile(
                                 SymbolicLink,
                                 GENERIC_WRITE | GENERIC_READ,0,NULL,OPEN_EXISTING,0,0);
			if (lpAdapter->hFile != INVALID_HANDLE_VALUE) {		
				if (PacketSetReadEvt(lpAdapter) == FALSE) {
					error = GetLastError();
					ODS("PacketOpenAdapter: Unable to open the read event\n");
					if (AdapterNameU != NULL)
						free(AdapterNameU);
					GlobalFreePtr(lpAdapter);
					//set the error to the one on which we failed
					SetLastError(error);
				    ODSEx("PacketOpenAdapter: PacketSetReadEvt failed, Error=1,%d\n",error);
					return NULL;					
				}
				PacketSetMaxLookaheadsize(lpAdapter);
				if (AdapterNameU != NULL)
				    free(AdapterNameU);
				return lpAdapter;
			} else {
        	    ODS("PacketOpenAdapter: CreateFile failed\n");
			}
		} else {
    	    ODSEx("PacketOpenAdapter: DefineDosDevice(%S) failed\n", &SymbolicLink[4]);
		}
	}
	error = GetLastError();
	if (AdapterNameU != NULL)
	    free(AdapterNameU);
	GlobalFreePtr(lpAdapter);
	//set the error to the one on which we failed
	SetLastError(error);
    ODSEx("PacketOpenAdapter: CreateFile failed, Error=2,%d\n",error);
	return NULL;

}

/*! 
  \brief Closes an adapter.
  \param lpAdapter the pointer to the adapter to close. 

  PacketCloseAdapter closes the given adapter and frees the associated ADAPTER structure
*/
VOID PacketCloseAdapter(LPADAPTER lpAdapter)
{
    CloseHandle(lpAdapter->hFile);
	SetEvent(lpAdapter->ReadEvent);
    CloseHandle(lpAdapter->ReadEvent);
    GlobalFreePtr(lpAdapter);
}

/*! 
  \brief Allocates a _PACKET structure.
  \return On succeess, the return value is the pointer to a _PACKET structure otherwise the 
   return value is NULL.

  The structure returned will be passed to the PacketReceivePacket() function to receive the
  packets from the driver.

  \warning The Buffer field of the _PACKET structure is not set by this function. 
  The buffer \b must be allocated by the application, and associated to the PACKET structure 
  with a call to PacketInitPacket.
*/
LPPACKET PacketAllocatePacket(void)
{

    LPPACKET lpPacket;
    lpPacket = (LPPACKET)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,sizeof(PACKET));
    if (lpPacket == NULL) {
        ODS("PacketAllocatePacket: GlobalAlloc Failed\n");
        return NULL;
    }
    return lpPacket;
}

/*! 
  \brief Frees a _PACKET structure.
  \param lpPacket The structure to free. 

  \warning the user-allocated buffer associated with the _PACKET structure is not deallocated 
  by this function and \b must be explicitly deallocated by the programmer.

*/
VOID PacketFreePacket(LPPACKET lpPacket)
{
    GlobalFreePtr(lpPacket);
}

/*! 
  \brief Initializes a _PACKET structure.
  \param lpPacket The structure to initialize. 
  \param Buffer A pointer to a user-allocated buffer that will contain the captured data.
  \param Length the length of the buffer. This is the maximum buffer size that will be 
   transferred from the driver to the application using a single read.

  \note the size of the buffer associated with the PACKET structure is a parameter that can sensibly 
  influence the performance of the capture process, since this buffer will contain the packets received
  from the the driver. The driver is able to return several packets using a single read call 
  (see the PacketReceivePacket() function for details), and the number of packets transferable to the 
  application in a call is limited only by the size of the buffer associated with the PACKET structure
  passed to PacketReceivePacket(). Therefore setting a big buffer with PacketInitPacket can noticeably 
  decrease the number of system calls, reducing the impcat of the capture process on the processor.
*/

VOID PacketInitPacket(LPPACKET lpPacket,PVOID Buffer,UINT Length)
{
    lpPacket->Buffer = Buffer;
    lpPacket->Length = Length;
	lpPacket->ulBytesReceived = 0;
	lpPacket->bIoComplete = FALSE;
}

/*! 
  \brief Read data (packets or statistics) from the NPF driver.
  \param AdapterObject Pointer to an _ADAPTER structure identifying the network adapter from which 
   the data is received.
  \param lpPacket Pointer to a PACKET structure that will contain the data.
  \param Sync This parameter is deprecated and will be ignored. It is present for compatibility with 
   older applications.
  \return If the function succeeds, the return value is nonzero.

  The data received with this function can be a group of packets or a static on the network traffic, 
  depending on the working mode of the driver. The working mode can be set with the PacketSetMode() 
  function. Give a look at that function if you are interested in the format used to return statistics 
  values, here only the normal capture mode will be described.

  The number of packets received with this function is variable. It depends on the number of packets 
  currently stored in the driver’s buffer, on the size of these packets and on the size of the buffer 
  associated to the lpPacket parameter. The following figure shows the format used by the driver to pass 
  packets to the application. 

  \image html encoding.gif "method used to encode the packets"

  Packets are stored in the buffer associated with the lpPacket _PACKET structure. The Length field of
  that structure is updated with the amount of data copied in the buffer. Each packet has a header
  consisting in a bpf_hdr structure that defines its length and contains its timestamp. A padding field 
  is used to word-align the data in the buffer (to speed up the access to the packets). The bh_datalen 
  and bh_hdrlen fields of the bpf_hdr structures should be used to extract the packets from the buffer. 
  
  Examples can be seen either in the TestApp sample application (see the \ref packetsamps page) provided
  in the developer's pack, or in the pcap_read() function of wpcap.
*/
BOOLEAN PacketReceivePacket(LPADAPTER AdapterObject,LPPACKET lpPacket,BOOLEAN Sync)
{
	BOOLEAN res;
	if ((int)AdapterObject->ReadTimeOut != -1)
		WaitForSingleObject(AdapterObject->ReadEvent, (AdapterObject->ReadTimeOut==0)?INFINITE:AdapterObject->ReadTimeOut);
    res = ReadFile(AdapterObject->hFile, lpPacket->Buffer, lpPacket->Length, &lpPacket->ulBytesReceived,NULL);
	return res;
}

/*! 
  \brief Sends one (or more) copies of a packet to the network.
  \param AdapterObject Pointer to an _ADAPTER structure identifying the network adapter that will 
   send the packets.
  \param lpPacket Pointer to a PACKET structure with the packet to send.
  \param Sync This parameter is deprecated and will be ignored. It is present for compatibility with 
   older applications.
  \return If the function succeeds, the return value is nonzero.

  This function is used to send a raw packet to the network. 'Raw packet' means that the programmer 
  will have to include the protocol headers, since the packet is sent to the network 'as is'. 
  The CRC needs not to be calculated and put at the end of the packet, because it will be transparently 
  added by the network interface.

  The behavior of this function is influenced by the PacketSetNumWrites() function. With PacketSetNumWrites(),
  it is possible to change the number of times a single write must be repeated. The default is 1, 
  i.e. every call to PacketSendPacket() will correspond to one packet sent to the network. If this number is
  greater than 1, for example 1000, every raw packet written by the application will be sent 1000 times on 
  the network. This feature mitigates the overhead of the context switches and therefore can be used to generate 
  high speed traffic. It is particularly useful for tools that test networks, routers, and servers and need 
  to obtain high network loads.
  The optimized sending process is still limited to one packet at a time: for the moment it cannot be used 
  to send a buffer with multiple packets.

  \note The ability to write multiple packets is currently present only in the Windows NTx version of the 
  packet driver. In Windows 95/98/ME it is emulated at user level in packet.dll. This means that an application
  that uses the multiple write method will run in Windows 9x as well, but its performance will be very low 
  compared to the one of WindowsNTx.
*/
BOOLEAN PacketSendPacket(LPADAPTER AdapterObject,LPPACKET lpPacket,BOOLEAN Sync)
{
    DWORD BytesTransfered;
    return WriteFile(AdapterObject->hFile,lpPacket->Buffer,lpPacket->Length,&BytesTransfered,NULL);
}


/*! 
  \brief Sends a buffer of packets to the network.
  \param AdapterObject Pointer to an _ADAPTER structure identifying the network adapter that will 
   send the packets.
  \param PacketBuff Pointer to buffer with the packets to send.
  \param Size Size of the buffer pointed by the PacketBuff argument.
  \param Sync if TRUE, the packets are sent respecting the timestamps. If FALSE, the packets are sent as
         fast as possible
  \return The amount of bytes actually sent. If the return value is smaller than the Size parameter, an
          error occurred during the send. The error can be caused by a driver/adapter problem or by an
		  inconsistent/bogus packet buffer.

  This function is used to send a buffer of raw packets to the network. The buffer can contain an arbitrary
  number of raw packets, each of which preceded by a dump_bpf_hdr structure. The dump_bpf_hdr is the same used
  by WinPcap and libpcap to store the packets in a file, therefore sending a capture file is straightforward.
  'Raw packets' means that the sending application will have to include the protocol headers, since every packet 
  is sent to the network 'as is'. The CRC of the packets needs not to be calculated, because it will be 
  transparently added by the network interface.

  \note Using this function if more efficient than issuing a series of PacketSendPacket(), because the packets are
  buffered in the kernel driver, so the number of context switches is reduced.

  \note When Sync is set to TRUE, the packets are synchronized in the kerenl with a high precision timestamp.
  This requires a remarkable amount of CPU, but allows to send the packets with a precision of some microseconds
  (depending on the precision of the performance counter of the machine). Such a precision cannot be reached 
  sending the packets separately with PacketSendPacket().
*/
INT PacketSendPackets(LPADAPTER AdapterObject, PVOID PacketBuff, ULONG Size, BOOLEAN Sync)
{
    BOOLEAN			Res;
    DWORD			BytesTransfered, TotBytesTransfered=0;
	struct timeval	BufStartTime;
	LARGE_INTEGER	StartTicks, CurTicks, TargetTicks, TimeFreq;


	ODS("PacketSendPackets");

	// Obtain starting timestamp of the buffer
	BufStartTime.tv_sec = ((struct timeval*)(PacketBuff))->tv_sec;
	BufStartTime.tv_usec = ((struct timeval*)(PacketBuff))->tv_usec;

	// Retrieve the reference time counters
	QueryPerformanceCounter(&StartTicks);
	QueryPerformanceFrequency(&TimeFreq);

	CurTicks.QuadPart = StartTicks.QuadPart;

	do{
		// Send the data to the driver
		Res = DeviceIoControl(AdapterObject->hFile,
			(Sync)?pBIOCSENDPACKETSSYNC:pBIOCSENDPACKETSNOSYNC,
			(PCHAR)PacketBuff + TotBytesTransfered,
			Size - TotBytesTransfered,
			NULL,
			0,
			&BytesTransfered,
			NULL);

		TotBytesTransfered += BytesTransfered;

		// calculate the time interval to wait before sending the next packet
		TargetTicks.QuadPart = StartTicks.QuadPart +
		(LONGLONG)
		((((struct timeval*)((PCHAR)PacketBuff + TotBytesTransfered))->tv_sec - BufStartTime.tv_sec) * 1000000 +
		(((struct timeval*)((PCHAR)PacketBuff + TotBytesTransfered))->tv_usec - BufStartTime.tv_usec)) *
		(TimeFreq.QuadPart) / 1000000;

		// Exit from the loop on termination or error
		if(TotBytesTransfered >= Size || Res != TRUE)
			break;
		
		// Wait until the time interval has elapsed
		while( CurTicks.QuadPart <= TargetTicks.QuadPart )
			QueryPerformanceCounter(&CurTicks);

	}
	while(TRUE);

	return TotBytesTransfered;
}

/*! 
  \brief Defines the minimum amount of data that will be received in a read.
  \param AdapterObject Pointer to an _ADAPTER structure
  \param nbytes the minimum amount of data in the kernel buffer that will cause the driver to
   release a read on this adapter.
  \return If the function succeeds, the return value is nonzero.

  In presence of a large value for nbytes, the kernel waits for the arrival of several packets before 
  copying the data to the user. This guarantees a low number of system calls, i.e. lower processor usage, 
  i.e. better performance, which is a good setting for applications like sniffers. Vice versa, a small value 
  means that the kernel will copy the packets as soon as the application is ready to receive them. This is 
  suggested for real time applications (like, for example, a bridge) that need the better responsiveness from 
  the kernel.

  \b note: this function has effect only in Windows NTx. The driver for Windows 9x doesn't offer 
  this possibility, therefore PacketSetMinToCopy is implemented under these systems only for compatibility.
*/

BOOLEAN PacketSetMinToCopy(LPADAPTER AdapterObject,int nbytes)
{
	DWORD BytesReturned;
    return DeviceIoControl(AdapterObject->hFile,pBIOCSMINTOCOPY,&nbytes,4,NULL,0,&BytesReturned,NULL);
}

/*!
  \brief Sets the working mode of an adapter.
  \param AdapterObject Pointer to an _ADAPTER structure.
  \param mode The new working mode of the adapter.
  \return If the function succeeds, the return value is nonzero.

  The device driver of WinPcap has 4 working modes:
  - Capture mode (mode = PACKET_MODE_CAPT): normal capture mode. The packets transiting on the wire are copied
   to the application when PacketReceivePacket() is called. This is the default working mode of an adapter.
  - Statistical mode (mode = PACKET_MODE_STAT): programmable statistical mode. PacketReceivePacket() returns, at
   precise intervals, statics values on the network traffic. The interval between the statistic samples is 
   by default 1 second but it can be set to any other value (with a 1 ms precision) with the 
   PacketSetReadTimeout() function. The data returned by PacketReceivePacket() when the adapter is in statistical
   mode is shown in the following figure:<p>
   	 \image html stats.gif "data structure returned by statistical mode"
   Two 64-bit counters are provided: the number of packets and the amount of bytes that satisfy a filter 
   previously set with PacketSetBPF(). If no filter has been set, all the packets are counted. The counters are 
   encapsulated in a bpf_hdr structure, so that they will be parsed correctly by wpcap. Statistical mode has a 
   very low impact on system performance compared to capture mode. 
  - Dump mode (mode = PACKET_MODE_DUMP): the packets are dumped to disk by the driver, in libpcap format. This
   method is much faster than saving the packets after having captured them. No data is returned 
   by PacketReceivePacket(). If the application sets a filter with PacketSetBPF(), only the packets that satisfy
   this filter are dumped to disk.
  - Statitical Dump mode (mode = PACKET_MODE_STAT_DUMP): the packets are dumped to disk by the driver, in libpcap 
   format, like in dump mode. PacketReceivePacket() returns, at precise intervals, statics values on the 
   network traffic and on the amount of data saved to file, in a way similar to statistical mode.
   The data returned by PacketReceivePacket() when the adapter is in statistical dump mode is shown in 
   the following figure:<p>   
	 \image html dump.gif "data structure returned by statistical dump mode"
   Three 64-bit counters are provided: the number of packets accepted, the amount of bytes accepted and the 
   effective amount of data (including headers) dumped to file. If no filter has been set, all the packets are 
   dumped to disk. The counters are encapsulated in a bpf_hdr structure, so that they will be parsed correctly 
   by wpcap.
   Look at the NetMeter example in the 
   WinPcap developer's pack to see how to use statistics mode.
*/
BOOLEAN PacketSetMode(LPADAPTER AdapterObject,int mode)
{
	DWORD BytesReturned;
    return DeviceIoControl(AdapterObject->hFile,pBIOCSMODE,&mode,4,NULL,0,&BytesReturned,NULL);
}

/*!
  \brief Sets the name of the file that will receive the packet when the adapter is in dump mode.
  \param AdapterObject Pointer to an _ADAPTER structure.
  \param name the file name, in ASCII or UNICODE.
  \param len the length of the buffer containing the name, in bytes.
  \return If the function succeeds, the return value is nonzero.

  This function defines the file name that the driver will open to store the packets on disk when 
  it works in dump mode. The adapter must be in dump mode, i.e. PacketSetMode() should have been
  called previously with mode = PACKET_MODE_DUMP. otherwise this function will fail.
  If PacketSetDumpName was already invoked on the adapter pointed by AdapterObject, the driver 
  closes the old file and opens the new one.
*/

BOOLEAN PacketSetDumpName(LPADAPTER AdapterObject, void *name, int len)
{
	DWORD		BytesReturned;
	WCHAR	*FileName;
	BOOLEAN	res;
	WCHAR	NameWithPath[1024];
	int		TStrLen;
	WCHAR	*NamePos;

	if(((PUCHAR)name)[1]!=0 && len>1){ //ASCII
		FileName=SChar2WChar(name);
		len*=2;
	} 
	else {	//Unicode
		FileName=name;
	}

	TStrLen=GetFullPathName(FileName,1024,NameWithPath,&NamePos);

	len=TStrLen*2+2;  //add the terminating null character

	// Try to catch malformed strings
	if(len>2048){
		if(((PUCHAR)name)[1]!=0 && len>1) free(FileName);
		return FALSE;
	}

    res = DeviceIoControl(AdapterObject->hFile,pBIOCSETDUMPFILENAME,NameWithPath,len,NULL,0,&BytesReturned,NULL);
	free(FileName);
	return res;
}

/*!
  \brief Set the dump mode limits.
  \param AdapterObject Pointer to an _ADAPTER structure.
  \param maxfilesize The maximum dimension of the dump file, in bytes. 0 means no limit.
  \param maxnpacks The maximum number of packets contained in the dump file. 0 means no limit.
  \return If the function succeeds, the return value is nonzero.

  This function sets the limits after which the NPF driver stops to save the packets to file when an adapter
  is in dump mode. This allows to limit the dump file to a precise number of bytes or packets, avoiding that
  very long dumps fill the disk space. If both maxfilesize and maxnpacks are set, the dump is stopped when
  the first of the two is reached.

  \note When a limit is reached, the dump is stopped, but the file remains opened. In order to flush 
  correctly the data and access the file consistently, you need to close the adapter with PacketCloseAdapter().
*/
BOOLEAN PacketSetDumpLimits(LPADAPTER AdapterObject, UINT maxfilesize, UINT maxnpacks)
{
	DWORD		BytesReturned;
	UINT valbuff[2];

	valbuff[0] = maxfilesize;
	valbuff[1] = maxnpacks;

    return DeviceIoControl(AdapterObject->hFile,
		pBIOCSETDUMPLIMITS,
		valbuff,
		sizeof valbuff,
		NULL,
		0,
		&BytesReturned,
		NULL);	
}

/*!
  \brief Returns the status of the kernel dump process, i.e. tells if one of the limits defined with PacketSetDumpLimits() was reached.
  \param AdapterObject Pointer to an _ADAPTER structure.
  \param sync if TRUE, the function blocks until the dump is finished, otherwise it returns immediately.
  \return TRUE if the dump is ended, FALSE otherwise.

  PacketIsDumpEnded() informs the user about the limits that were set with a previous call to 
  PacketSetDumpLimits().

  \warning If no calls to PacketSetDumpLimits() were performed or if the dump process has no limits 
  (i.e. if the arguments of the last call to PacketSetDumpLimits() were both 0), setting sync to TRUE will
  block the application on this call forever.
*/
BOOLEAN PacketIsDumpEnded(LPADAPTER AdapterObject, BOOLEAN sync)
{
	DWORD		BytesReturned;
	int		IsDumpEnded;
	BOOLEAN	res;

	if(sync)
		WaitForSingleObject(AdapterObject->ReadEvent, INFINITE);

    res = DeviceIoControl(AdapterObject->hFile,
		pBIOCISDUMPENDED,
		NULL,
		0,
		&IsDumpEnded,
		4,
		&BytesReturned,
		NULL);

	if(res == FALSE) return TRUE; // If the IOCTL returns an error we consider the dump finished

	return (BOOLEAN)IsDumpEnded;
}

/*!
  \brief Returns the notification event associated with the read calls on an adapter.
  \param AdapterObject Pointer to an _ADAPTER structure.
  \return The handle of the event that the driver signals when some data is available in the kernel buffer.

  The event returned by this function is signaled by the driver if:
  - The adapter pointed by AdapterObject is in capture mode and an amount of data greater or equal 
  than the one set with the PacketSetMinToCopy() function is received from the network.
  - the adapter pointed by AdapterObject is in capture mode, no data has been received from the network
   but the the timeout set with the PacketSetReadTimeout() function has elapsed.
  - the adapter pointed by AdapterObject is in statics mode and the the timeout set with the 
   PacketSetReadTimeout() function has elapsed. This means that a new statistic sample is available.

  In every case, a call to PacketReceivePacket() will return immediately.
  The event can be passed to standard Win32 functions (like WaitForSingleObject or WaitForMultipleObjects) 
  to wait until the driver's buffer contains some data. It is particularly useful in GUI applications that 
  need to wait concurrently on several events.

*/
HANDLE PacketGetReadEvent(LPADAPTER AdapterObject)
{
    return AdapterObject->ReadEvent;
}

/*!
  \brief Sets the number of times a single packet written with PacketSendPacket() will be repeated on the network.
  \param AdapterObject Pointer to an _ADAPTER structure.
  \param nwrites Number of copies of a packet that will be physically sent by the interface.
  \return If the function succeeds, the return value is nonzero.

	See PacketSendPacket() for details.
*/
BOOLEAN PacketSetNumWrites(LPADAPTER AdapterObject,int nwrites)
{
	DWORD BytesReturned;
    return DeviceIoControl(AdapterObject->hFile,pBIOCSWRITEREP,&nwrites,4,NULL,0,&BytesReturned,NULL);
}

/*!
  \brief Sets the timeout after which a read on an adapter returns.
  \param AdapterObject Pointer to an _ADAPTER structure.
  \param timeout indicates the timeout, in milliseconds, after which a call to PacketReceivePacket() on 
  the adapter pointed by AdapterObject will be released, also if no packets have been captured by the driver. 
  Setting timeout to 0 means no timeout, i.e. PacketReceivePacket() never returns if no packet arrives.  
  A timeout of -1 causes PacketReceivePacket() to always return immediately.
  \return If the function succeeds, the return value is nonzero.

  \note This function works also if the adapter is working in statistics mode, and can be used to set the 
  time interval between two statistic reports.
*/
BOOLEAN PacketSetReadTimeout(LPADAPTER AdapterObject,int timeout)
{
	DWORD BytesReturned;
	int DriverTimeOut=-1;

	AdapterObject->ReadTimeOut=timeout;

    return DeviceIoControl(AdapterObject->hFile,pBIOCSRTIMEOUT,&DriverTimeOut,4,NULL,0,&BytesReturned,NULL);
}

/*!
  \brief Sets the size of the kernel-level buffer associated with a capture.
  \param AdapterObject Pointer to an _ADAPTER structure.
  \param dim New size of the buffer, in \b kilobytes.
  \return The function returns TRUE if successfully completed, FALSE if there is not enough memory to 
   allocate the new buffer.

  When a new dimension is set, the data in the old buffer is discarded and the packets stored in it are 
  lost. 
  
  Note: the dimension of the kernel buffer affects heavily the performances of the capture process.
  An adequate buffer in the driver is able to keep the packets while the application is busy, compensating 
  the delays of the application and avoiding the loss of packets during bursts or high network activity. 
  The buffer size is set to 0 when an instance of the driver is opened: the programmer should remember to 
  set it to a proper value. As an example, wpcap sets the buffer size to 1MB at the beginning of a capture.
*/
BOOLEAN PacketSetBuff(LPADAPTER AdapterObject,int dim)
{
	DWORD BytesReturned;
    return DeviceIoControl(AdapterObject->hFile,pBIOCSETBUFFERSIZE,&dim,4,NULL,0,&BytesReturned,NULL);
}

/*!
  \brief Sets a kernel-level packet filter.
  \param AdapterObject Pointer to an _ADAPTER structure.
  \param fp Pointer to a filtering program that will be associated with this capture or monitoring 
  instance and that will be executed on every incoming packet.
  \return This function returns TRUE if the filter is set successfully, FALSE if an error occurs 
   or if the filter program is not accepted after a safeness check by the driver.  The driver performs 
   the check in order to avoid system crashes due to buggy or malicious filters, and it rejects non
   conformat filters.

  This function associates a new BPF filter to the adapter AdapterObject. The filter, pointed by fp, is a 
  set of bpf_insn instructions.

  A filter can be automatically created by using the pcap_compile() function of wpcap. This function 
  converts a human readable text expression with the syntax of WinDump (see the manual of WinDump at 
  http://netgroup.polito.it/windump for details) into a BPF program. If your program doesn't link wpcap, but 
  you need to know the code of a particular filter, you can launch WinDump with the -d or -dd or -ddd 
  flags to obtain the pseudocode.

*/
BOOLEAN PacketSetBpf(LPADAPTER AdapterObject,struct bpf_program *fp)
{
	DWORD BytesReturned;
    return DeviceIoControl(AdapterObject->hFile,pBIOCSETF,(char*)fp->bf_insns,fp->bf_len*sizeof(struct bpf_insn),NULL,0,&BytesReturned,NULL);
}

/*!
  \brief Returns a couple of statistic values about the current capture session.
  \param AdapterObject Pointer to an _ADAPTER structure.
  \param s Pointer to a user provided bpf_stat structure that will be filled by the function.
  \return If the function succeeds, the return value is nonzero.

  With this function, the programmer can know the value of two internal variables of the driver:

  - the number of packets that have been received by the adapter AdapterObject, starting at the 
   time in which it was opened with PacketOpenAdapter. 
  - the number of packets that have been dropped by the driver. A packet is dropped when the kernel
   buffer associated with the adapter is full. 
*/
BOOLEAN PacketGetStats(LPADAPTER AdapterObject,struct bpf_stat *s)
{
	BOOLEAN Res;
	DWORD BytesReturned;
	struct bpf_stat tmpstat;	// We use a support structure to avoid kernel-level inconsistencies with old or malicious applications

	Res = DeviceIoControl(AdapterObject->hFile,
		pBIOCGSTATS,
		NULL,
		0,
		&tmpstat,
		sizeof(struct bpf_stat),
		&BytesReturned,
		NULL);

	// Copy only the first two values retrieved from the driver
	s->bs_recv = tmpstat.bs_recv;
	s->bs_drop = tmpstat.bs_drop;

	return Res;
}

/*!
  \brief Returns statistic values about the current capture session. Enhanced version of PacketGetStats().
  \param AdapterObject Pointer to an _ADAPTER structure.
  \param s Pointer to a user provided bpf_stat structure that will be filled by the function.
  \return If the function succeeds, the return value is nonzero.

  With this function, the programmer can retireve the sname values provided by PacketGetStats(), plus:

  - the number of drops by interface (not yet supported, always 0). 
  - the number of packets that reached the application, i.e that were accepted by the kernel filter and
  that fitted in the kernel buffer. 
*/
BOOLEAN PacketGetStatsEx(LPADAPTER AdapterObject,struct bpf_stat *s)
{
	BOOLEAN Res;
	DWORD BytesReturned;
	struct bpf_stat tmpstat;	// We use a support structure to avoid kernel-level inconsistencies with old or malicious applications

	Res = DeviceIoControl(AdapterObject->hFile,
		pBIOCGSTATS,
		NULL,
		0,
		&tmpstat,
		sizeof(struct bpf_stat),
		&BytesReturned,
		NULL);

	s->bs_recv = tmpstat.bs_recv;
	s->bs_drop = tmpstat.bs_drop;
	s->ps_ifdrop = tmpstat.ps_ifdrop;
	s->bs_capt = tmpstat.bs_capt;

	return Res;
}

/*!
  \brief Performs a query/set operation on an internal variable of the network card driver.
  \param AdapterObject Pointer to an _ADAPTER structure.
  \param Set Determines if the operation is a set (Set=TRUE) or a query (Set=FALSE).
  \param OidData A pointer to a _PACKET_OID_DATA structure that contains or receives the data.
  \return If the function succeeds, the return value is nonzero.

  \note not all the network adapters implement all the query/set functions. There is a set of mandatory 
  OID functions that is granted to be present on all the adapters, and a set of facultative functions, not 
  provided by all the cards (see the Microsoft DDKs to see which functions are mandatory). If you use a 
  facultative function, be careful to enclose it in an if statement to check the result.
*/
BOOLEAN PacketRequest(LPADAPTER  AdapterObject,BOOLEAN Set,PPACKET_OID_DATA  OidData)
{
	DWORD BytesReturned;
    BOOLEAN Result;

    Result=DeviceIoControl(AdapterObject->hFile,(DWORD) Set ? pBIOCSETOID : pBIOCQUERYOID,
                           OidData,sizeof(PACKET_OID_DATA)-1+OidData->Length,OidData,
                           sizeof(PACKET_OID_DATA)-1+OidData->Length,&BytesReturned,NULL);
    
	// output some debug info
	ODSEx("PacketRequest, OID=%d ", OidData->Oid);
    ODSEx("Length=%d ", OidData->Length);
    ODSEx("Set=%d ", Set);
    ODSEx("Res=%d\n", Result);

	return Result;
}

/*!
  \brief Sets a hardware filter on the incoming packets.
  \param AdapterObject Pointer to an _ADAPTER structure.
  \param Filter The identifier of the filter.
  \return If the function succeeds, the return value is nonzero.

  The filter defined with this filter is evaluated by the network card, at a level that is under the NPF
  device driver. Here is a list of the most useful hardware filters (A complete list can be found in ntddndis.h):

  - NDIS_PACKET_TYPE_PROMISCUOUS: sets promiscuous mode. Every incoming packet is accepted by the adapter. 
  - NDIS_PACKET_TYPE_DIRECTED: only packets directed to the workstation's adapter are accepted. 
  - NDIS_PACKET_TYPE_BROADCAST: only broadcast packets are accepted. 
  - NDIS_PACKET_TYPE_MULTICAST: only multicast packets belonging to groups of which this adapter is a member are accepted. 
  - NDIS_PACKET_TYPE_ALL_MULTICAST: every multicast packet is accepted. 
  - NDIS_PACKET_TYPE_ALL_LOCAL: all local packets, i.e. NDIS_PACKET_TYPE_DIRECTED + NDIS_PACKET_TYPE_BROADCAST + NDIS_PACKET_TYPE_MULTICAST 
*/
BOOLEAN PacketSetHwFilter(LPADAPTER  AdapterObject,ULONG Filter)
{
    BOOLEAN    Status;
    ULONG      IoCtlBufferLength=(sizeof(PACKET_OID_DATA)+sizeof(ULONG)-1);
    PPACKET_OID_DATA  OidData;

    OidData=GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,IoCtlBufferLength);
    if (OidData == NULL) {
        ODS("PacketSetHwFilter: GlobalAlloc Failed\n");
        return FALSE;
    }
    OidData->Oid=OID_GEN_CURRENT_PACKET_FILTER;
    OidData->Length=sizeof(ULONG);
    *((PULONG)OidData->Data)=Filter;
    Status=PacketRequest(AdapterObject,TRUE,OidData);
    GlobalFreePtr(OidData);
    return Status;
}

/*!
  \brief Retrieve the list of available network adapters and their description.
  \param pStr User allocated string that will be filled with the names of the adapters.
  \param BufferSize Length of the buffer pointed by pStr.
  \return If the function succeeds, the return value is nonzero.

  Usually, this is the first function that should be used to communicate with the driver.
  It returns the names of the adapters installed on the system <B>and supported by WinPcap</B>. 
  After the names of the adapters, pStr contains a string that describes each of them.

  \b Warning: 
  the result of this function is obtained querying the registry, therefore the format 
  of the result in Windows NTx is different from the one in Windows 9x. Windows 9x uses the ASCII
  encoding method to store a string, while Windows NTx uses UNICODE. After a call to PacketGetAdapterNames 
  in Windows 95x, pStr contains, in succession:
  - a variable number of ASCII strings, each with the names of an adapter, separated by a "\0"
  - a double "\0"
  - a number of ASCII strings, each with the description of an adapter, separated by a "\0". The number 
   of descriptions is the same of the one of names. The fisrt description corresponds to the first name, and
   so on.
  - a double "\0". 

  In Windows NTx, pStr contains: the names of the adapters, in UNICODE format, separated by a single UNICODE "\0" (i.e. 2 ASCII "\0"), a double UNICODE "\0", followed by the descriptions of the adapters, in ASCII format, separated by a single ASCII "\0" . The string is terminated by a double ASCII "\0".
  - a variable number of UNICODE strings, each with the names of an adapter, separated by a UNICODE "\0"
  - a double UNICODE "\0"
  - a number of ASCII strings, each with the description of an adapter, separated by an ASCII "\0".
  - a double ASCII "\0". 
*/

BOOLEAN PacketGetAdapterNames(PTSTR pStr, PULONG BufferSize)
{
    HKEY       LinkageKey, AdapKey;
	DWORD	   RegKeySize = 0;
    LONG       Status;
	ULONG	   Result;
	PTSTR      BpStr;
	char       *TTpStr;
	char       *DpStr;
	char       *DescBuf;
	LPADAPTER  adapter;
    PPACKET_OID_DATA OidData;
	int		   i = 0, k, rewind;
    DWORD      dim;
	TCHAR	   AdapName[256];

    ODSEx("PacketGetAdapterNames: BufferSize=%d\n",*BufferSize);

    OidData = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, 512);
    if (OidData == NULL) {
        ODS("PacketGetAdapterNames: GlobalAlloc Failed\n");
        return FALSE;
    }

    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		                  TEXT("SYSTEM\\ControlSet001\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}"),
						  0, KEY_READ, &AdapKey);

	// Get the size to allocate for the original device names
	while ((Result = RegEnumKey(AdapKey, i, AdapName, sizeof(AdapName)/2)) == ERROR_SUCCESS) {
		Status = RegOpenKeyEx(AdapKey, AdapName,0, KEY_READ, &LinkageKey);
		Status = RegOpenKeyExW(LinkageKey, L"Linkage",0, KEY_READ, &LinkageKey);
        Status = RegQueryValueExW(LinkageKey, L"Export", NULL, NULL, NULL, &dim);
		i++;
		if (Status!=ERROR_SUCCESS) continue;
		RegKeySize += dim;
	}
	
	// Allocate the memory for the original device names
	ODSEx("Need %d bytes for the names\n", RegKeySize+2);
	BpStr = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, RegKeySize+2);
	if (BpStr == NULL || RegKeySize > *BufferSize) {
        ODS("PacketGetAdapterNames: GlobalAlloc Failed\n");
		GlobalFreePtr(OidData);
		return FALSE;
	}
	k = 0;
	i = 0;

    ODS("PacketGetAdapterNames: Cycling through the adapters:\n");

	// Copy the names to the buffer
	while ((Result = RegEnumKey(AdapKey, i, AdapName, sizeof(AdapName)/2)) == ERROR_SUCCESS) {
		WCHAR UpperBindStr[64];

		i++;
		ODSEx(" %d) ", i);

		Status = RegOpenKeyEx(AdapKey,AdapName,0,KEY_READ,&LinkageKey);
		Status = RegOpenKeyExW(LinkageKey,L"Linkage",0,KEY_READ,&LinkageKey);

		dim=sizeof(UpperBindStr);
        Status=RegQueryValueExW(LinkageKey,L"UpperBind",NULL,NULL,(PUCHAR)UpperBindStr,&dim);
		
		ODSEx("UpperBind=%S ", UpperBindStr);

		if( Status!=ERROR_SUCCESS || _wcsicmp(UpperBindStr,L"NdisWan")==0 ){
			ODS("Name = SKIPPED\n");
			continue;
		}

		dim=RegKeySize-k;
        Status=RegQueryValueExW(LinkageKey,L"Export",NULL,NULL,(LPBYTE)BpStr+k,&dim);
		if(Status!=ERROR_SUCCESS){
			ODS("Name = SKIPPED (error reading the key)\n");
			continue;
		}

		ODSEx("Name = %S\n", (LPBYTE)BpStr+k);

		k+=dim-2;
	}

	CloseHandle(AdapKey);

#ifdef _DEBUG_TO_FILE
	//dump BpStr for debug purposes
	ODS("Dumping BpStr:");
	{
		FILE *f;
		f = fopen("winpcap_debug.txt", "a");
		for(i=0;i<k;i++){
			if(!(i%32))fprintf(f, "\n ");
			fprintf(f, "%c " , *((LPBYTE)BpStr+i));
		}
		fclose(f);
	}
	ODS("\n");
#endif

	
	if (k != 0){
		
		DescBuf=GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, 4096);
		if (DescBuf == NULL) {
			GlobalFreePtr (BpStr);
		    GlobalFreePtr(OidData);
			return FALSE;
		}
		DpStr=DescBuf;
				
		for(i=0,k=0;BpStr[i]!=0 || BpStr[i+1]!=0;){
			
			if(k+wcslen(BpStr+i)+30 > *BufferSize){
				// Input buffer too small
			    GlobalFreePtr(OidData);
				GlobalFreePtr (BpStr);
				GlobalFreePtr (DescBuf);
				ODS("PacketGetAdapterNames: Input buffer too small!\n");
				return FALSE;
			}

			// Create the device name
			rewind=k;
			memcpy(pStr+k,BpStr+i,16);
			memcpy(pStr+k+8,TEXT("NPF_"),8);
			i+=8;
			k+=12;
			while(BpStr[i-1]!=0){
				pStr[k++]=BpStr[i++];
			}

			// Open the adapter
			adapter=PacketOpenAdapter(pStr+rewind);
			if(adapter==NULL){
				k=rewind;
				continue;
			}

			// Retrieve the description
			OidData->Oid = OID_GEN_VENDOR_DESCRIPTION;
			OidData->Length = 256;
			ZeroMemory(OidData->Data,256);
			Status = PacketRequest(adapter,FALSE,OidData);
			if(Status==0 || ((char*)OidData->Data)[0]==0){
				k=rewind;
				continue;
			}

			ODSEx("Adapter Description=%s\n\n",OidData->Data);

			// Copy the description
			TTpStr=(char*)(OidData->Data);
			while(*TTpStr!=0){
				*DpStr++=*TTpStr++;
			}
			*DpStr++=*TTpStr++;
			
			// Close the adapter
			PacketCloseAdapter(adapter);
			
		}
		*DpStr = 0;

		pStr[k++] = 0;
		pStr[k] = 0;

		if ((ULONG)(DpStr - DescBuf + k) < *BufferSize) {
			memcpy(pStr + k, DescBuf, DpStr - DescBuf);
		} else {
		    GlobalFreePtr(OidData);
			GlobalFreePtr(BpStr);
			GlobalFreePtr(DescBuf);
			ODS("\nPacketGetAdapterNames: ended with failure\n");
			return FALSE;
		}

	    GlobalFreePtr(OidData);
		GlobalFreePtr(BpStr);
		GlobalFreePtr(DescBuf);
		ODS("\nPacketGetAdapterNames: ended correctly\n");
		return TRUE;
	}
	else{
	    DWORD RegType;

		ODS("Adapters not found under SYSTEM\\ControlSet001\\Control\\Class. Using the TCP/IP bindings.\n");

		GlobalFreePtr(BpStr);
		Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
						TEXT("SYSTEM\\ControlSet001\\Services\\Tcpip\\Linkage"),
						0, KEY_READ, &LinkageKey);
		if (Status == ERROR_SUCCESS) {
			// Retrieve the length of the key
			Status = RegQueryValueEx(LinkageKey, TEXT("bind"), NULL, &RegType, NULL, &RegKeySize);
			// Allocate the buffer
			BpStr = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, RegKeySize + 2);
			if (BpStr == NULL || RegKeySize > *BufferSize) {
				GlobalFreePtr(OidData);
				return FALSE;
			}
			Status = RegQueryValueEx(LinkageKey, TEXT("bind"), NULL, &RegType, (LPBYTE)BpStr, &RegKeySize);
			RegCloseKey(LinkageKey);
		} else {
            //ODS("SYSTEM\\ControlSet001\\Control\\Class - RegKey not found.\n");
            ODS("SYSTEM\\ControlSet001\\Services\\Tcpip\\Linkage - RegKey not found.\n");
		}
		if (Status == ERROR_SUCCESS) {
			DescBuf = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, 4096);
			if (DescBuf == NULL) {
				GlobalFreePtr(BpStr);
				GlobalFreePtr(OidData);
				return FALSE;
			}
			DpStr = DescBuf;
			for (i = 0, k = 0; BpStr[i] != 0 || BpStr[i+1] != 0; ) {
				if (k + wcslen(BpStr + i) + 30 > *BufferSize) {
					// Input buffer too small
					GlobalFreePtr(OidData);
					GlobalFreePtr(BpStr);
					GlobalFreePtr(DescBuf);
					return FALSE;
				}

				ODS("\tCreating a device name - started.\n");

				// Create the device name
				rewind = k;
				memcpy(pStr + k,BpStr + i,16);
				memcpy(pStr + k + 8, TEXT("NPF_"), 8);
				i += 8;
				k += 12;
				while (BpStr[i - 1] != 0) {
					pStr[k++] = BpStr[i++];
				}
				// Open the adapter
				adapter = PacketOpenAdapter(pStr+rewind);
				if (adapter == NULL) {
					k = rewind;
					continue;
				}
				// Retrieve the description
				OidData->Oid = OID_GEN_VENDOR_DESCRIPTION;
				OidData->Length = 256;
				Status = PacketRequest(adapter, FALSE, OidData);
				if (Status == 0 || ((char*)OidData->Data)[0] == 0) {
					k = rewind;
     				ODS("\tCreating a device name - Retrieve the description.\n");
					continue;
				}
				
				// Copy the description
				TTpStr = (char*)(OidData->Data);
				while (*TTpStr != 0){
					*DpStr++ = *TTpStr++;
				}
				*DpStr++ = *TTpStr++;
				// Close the adapter
				PacketCloseAdapter(adapter);

				ODS("\tCreating a device name - completed.\n");
			}
			*DpStr = 0;
			
			pStr[k++] = 0;
			pStr[k] = 0;
			
			if ((ULONG)(DpStr - DescBuf + k) < *BufferSize) {
				memcpy(pStr + k, DescBuf, DpStr-DescBuf);
			} else {
				GlobalFreePtr(OidData);
				GlobalFreePtr(BpStr);
				GlobalFreePtr(DescBuf);
				return FALSE;
			}
			
			GlobalFreePtr(OidData);
			GlobalFreePtr(BpStr);
			GlobalFreePtr(DescBuf);
			ODS("PacketGetAdapterNames() returning TRUE\n");
			return TRUE;
		} else {
			MessageBox(NULL,TEXT("Can not find TCP/IP bindings.\nIn order to run the packet capture driver you must install TCP/IP."),szWindowTitle,MB_OK);
			ODS("Cannot find the TCP/IP bindings\n");
			return FALSE;
		}
	}
}

/*!
  \brief Returns comprehensive information the addresses of an adapter.
  \param AdapterName String that contain _ADAPTER structure.
  \param buffer A user allocated array of npf_if_addr that will be filled by the function.
  \param NEntries Size of the array (in npf_if_addr).
  \return If the function succeeds, the return value is nonzero.

  This function grabs from the registry information like the IP addresses, the netmasks 
  and the broadcast addresses of an interface. The buffer passed by the user is filled with 
  npf_if_addr structures, each of which contains the data for a single address. If the buffer
  is full, the reaming addresses are dropeed, therefore set its dimension to sizeof(npf_if_addr)
  if you want only the first address.
*/

BOOLEAN PacketGetNetInfoEx(LPTSTR AdapterName, npf_if_addr* buffer, PLONG NEntries)
{
	char	*AdapterNameA;
	WCHAR	*AdapterNameU;
	WCHAR	*ifname;
	HKEY	SystemKey;
	HKEY	InterfaceKey;
	HKEY	ParametersKey;
	HKEY	TcpIpKey;
	HKEY	UnderTcpKey;
	LONG	status;
	WCHAR	String[1024+1];
	DWORD	RegType;
	ULONG	BufLen;
	DWORD	DHCPEnabled;
	struct	sockaddr_in *TmpAddr, *TmpBroad;
	LONG	naddrs,nmasks,StringPos;
	DWORD	ZeroBroadcast;

	AdapterNameA = (char*)AdapterName;
	if(AdapterNameA[1] != 0) {	//ASCII
		AdapterNameU = SChar2WChar(AdapterNameA);
		AdapterName = AdapterNameU;
	} else {				//Unicode
		AdapterNameU = NULL;
	}
    ifname = wcsrchr(AdapterName, '\\');
	if (ifname == NULL)
		ifname = AdapterName;
	else
		ifname++;
	if (wcsncmp(ifname, L"NPF_", 4) == 0)
		ifname += 4;

	if(	RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
						TEXT("SYSTEM\\ControlSet001\\Services\\Tcpip\\Parameters\\Interfaces"),
						0, KEY_READ, &UnderTcpKey) == ERROR_SUCCESS)
	{
		status = RegOpenKeyExW(UnderTcpKey,ifname,0,KEY_READ,&TcpIpKey);
		if (status != ERROR_SUCCESS) {
			RegCloseKey(UnderTcpKey);
			goto fail;
		}
	}
	else
	{
		
		// Query the registry key with the interface's adresses
		status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
						TEXT("SYSTEM\\ControlSet001\\Services"),
						0,KEY_READ,&SystemKey);
		if (status != ERROR_SUCCESS)
			goto fail;
		status = RegOpenKeyExW(SystemKey,ifname,0,KEY_READ,&InterfaceKey);
		if (status != ERROR_SUCCESS) {
			RegCloseKey(SystemKey);
			goto fail;
		}
		RegCloseKey(SystemKey);
		status = RegOpenKeyEx(InterfaceKey,TEXT("Parameters"),0,KEY_READ,&ParametersKey);
		if (status != ERROR_SUCCESS) {
			RegCloseKey(InterfaceKey);
			goto fail;
		}
		RegCloseKey(InterfaceKey);
		status = RegOpenKeyEx(ParametersKey,TEXT("TcpIp"),0,KEY_READ,&TcpIpKey);
		if (status != ERROR_SUCCESS) {
			RegCloseKey(ParametersKey);
			goto fail;
		}
		RegCloseKey(ParametersKey);
		BufLen = sizeof String;
	}

	BufLen = 4;
	/* Try to detect if the interface has a zero broadcast addr */
	status=RegQueryValueEx(TcpIpKey,TEXT("UseZeroBroadcast"),NULL,&RegType,(LPBYTE)&ZeroBroadcast,&BufLen);
	if (status != ERROR_SUCCESS)
		ZeroBroadcast=0;
	
	BufLen = 4;
	/* See if DHCP is used by this system */
	status=RegQueryValueEx(TcpIpKey,TEXT("EnableDHCP"),NULL,&RegType,(LPBYTE)&DHCPEnabled,&BufLen);
	if (status != ERROR_SUCCESS)
		DHCPEnabled=0;
	
	
	/* Retrieve the adrresses */
	if(DHCPEnabled){
		
		BufLen = sizeof String;
		// Open the key with the addresses
		status = RegQueryValueEx(TcpIpKey,TEXT("DhcpIPAddress"),NULL,&RegType,(LPBYTE)String,&BufLen);
		if (status != ERROR_SUCCESS) {
			RegCloseKey(TcpIpKey);
			goto fail;
		}

		// scan the key to obtain the addresses
		StringPos = 0;
		for(naddrs = 0;naddrs <* NEntries;naddrs++){
			TmpAddr = (struct sockaddr_in *) &(buffer[naddrs].IPAddress);
			
			if((TmpAddr->sin_addr.S_un.S_addr = inet_addrU(String + StringPos))!= -1){
				TmpAddr->sin_family = AF_INET;
				
				TmpBroad = (struct sockaddr_in *) &(buffer[naddrs].Broadcast);
				TmpBroad->sin_family = AF_INET;
				if(ZeroBroadcast==0)
					TmpBroad->sin_addr.S_un.S_addr = 0xffffffff; // 255.255.255.255
				else
					TmpBroad->sin_addr.S_un.S_addr = 0; // 0.0.0.0

				while(*(String + StringPos) != 0)StringPos++;
				StringPos++;
				
				if(*(String + StringPos) == 0 || (StringPos * sizeof (WCHAR)) >= BufLen)
					break;				
			}
			else break;
		}		
		
		BufLen = sizeof String;
		// Open the key with the netmasks
		status = RegQueryValueEx(TcpIpKey,TEXT("DhcpSubnetMask"),NULL,&RegType,(LPBYTE)String,&BufLen);
		if (status != ERROR_SUCCESS) {
			RegCloseKey(TcpIpKey);
			goto fail;
		}
		
		// scan the key to obtain the masks
		StringPos = 0;
		for(nmasks = 0;nmasks < *NEntries;nmasks++){
			TmpAddr = (struct sockaddr_in *) &(buffer[nmasks].SubnetMask);
			
			if((TmpAddr->sin_addr.S_un.S_addr = inet_addrU(String + StringPos))!= -1){
				TmpAddr->sin_family = AF_INET;
				
				while(*(String + StringPos) != 0)StringPos++;
				StringPos++;
								
				if(*(String + StringPos) == 0 || (StringPos * sizeof (WCHAR)) >= BufLen)
					break;
			}
			else break;
		}		
		
		// The number of masks MUST be equal to the number of adresses
		if(nmasks != naddrs){
			RegCloseKey(TcpIpKey);
			goto fail;
		}
				
	}
	else{
		
		BufLen = sizeof String;
		// Open the key with the addresses
		status = RegQueryValueEx(TcpIpKey,TEXT("IPAddress"),NULL,&RegType,(LPBYTE)String,&BufLen);
		if (status != ERROR_SUCCESS) {
			RegCloseKey(TcpIpKey);
			goto fail;
		}
		
		// scan the key to obtain the addresses
		StringPos = 0;
		for(naddrs = 0;naddrs < *NEntries;naddrs++){
			TmpAddr = (struct sockaddr_in *) &(buffer[naddrs].IPAddress);
			
			if((TmpAddr->sin_addr.S_un.S_addr = inet_addrU(String + StringPos))!= -1){
				TmpAddr->sin_family = AF_INET;

				TmpBroad = (struct sockaddr_in *) &(buffer[naddrs].Broadcast);
				TmpBroad->sin_family = AF_INET;
				if(ZeroBroadcast==0)
					TmpBroad->sin_addr.S_un.S_addr = 0xffffffff; // 255.255.255.255
				else
					TmpBroad->sin_addr.S_un.S_addr = 0; // 0.0.0.0
				
				while(*(String + StringPos) != 0)StringPos++;
				StringPos++;
				
				if(*(String + StringPos) == 0 || (StringPos * sizeof (WCHAR)) >= BufLen)
					break;
			}
			else break;
		}		
		
		BufLen = sizeof String;
		// Open the key with the netmasks
		status = RegQueryValueEx(TcpIpKey,TEXT("SubnetMask"),NULL,&RegType,(LPBYTE)String,&BufLen);
		if (status != ERROR_SUCCESS) {
			RegCloseKey(TcpIpKey);
			goto fail;
		}
		
		// scan the key to obtain the masks
		StringPos = 0;
		for(nmasks = 0;nmasks <* NEntries;nmasks++){
			TmpAddr = (struct sockaddr_in *) &(buffer[nmasks].SubnetMask);
			
			if((TmpAddr->sin_addr.S_un.S_addr = inet_addrU(String + StringPos))!= -1){
				TmpAddr->sin_family = AF_INET;
				
				while(*(String + StringPos) != 0)StringPos++;
				StringPos++;
				
				if(*(String + StringPos) == 0 || (StringPos * sizeof (WCHAR)) >= BufLen)
					break;
			}
			else break;
		}		
		
		// The number of masks MUST be equal to the number of adresses
		if(nmasks != naddrs){
			RegCloseKey(TcpIpKey);
			goto fail;
		}
				
	}
	
	*NEntries = naddrs + 1;

	RegCloseKey(TcpIpKey);
	
	if (status != ERROR_SUCCESS) {
		goto fail;
	}
	
	
	if (AdapterNameU != NULL)
		free(AdapterNameU);
	return TRUE;
	
fail:
	if (AdapterNameU != NULL)
		free(AdapterNameU);
    return FALSE;
}

/*!
  \brief Returns the IP address and the netmask of an adapter.
  \param AdapterName String that contain _ADAPTER structure.
  \param netp Pointer to a variable that will receive the IP address of the adapter.
  \param maskp Pointer to a variable that will receive the netmask of the adapter.
  \return If the function succeeds, the return value is nonzero.

  \note this function is obsolete and is maintained for backward compatibility. Use PacketGetNetInfoEx() instead. 
*/

BOOLEAN PacketGetNetInfo(LPTSTR AdapterName, PULONG netp, PULONG maskp)
{
	char	*AdapterNameA;
	WCHAR	*AdapterNameU;
	WCHAR	*ifname;
	HKEY	SystemKey;
	HKEY	InterfaceKey;
	HKEY	ParametersKey;
	HKEY	TcpIpKey;
	LONG	status;
	WCHAR	String[1024+1];
	DWORD	RegType;
	ULONG	BufLen;
	DWORD	DHCPEnabled;
	ULONG	TAddr,i;

	AdapterNameA = (char*)AdapterName;
	if(AdapterNameA[1] != 0) {	//ASCII
		AdapterNameU = SChar2WChar(AdapterNameA);
		AdapterName = AdapterNameU;
	} else {				//Unicode
		AdapterNameU = NULL;
	}
	ifname = wcsrchr(AdapterName, '\\');
	if (ifname == NULL)
		ifname = AdapterName;
	else
		ifname++;
	if (wcsncmp(ifname, L"NPF_", 4) == 0)
		ifname += 4;
	status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
						TEXT("SYSTEM\\ControlSet001\\Services"),
						0,KEY_READ,&SystemKey);
	if (status != ERROR_SUCCESS)
		goto fail;
	status = RegOpenKeyExW(SystemKey,ifname,0,KEY_READ,&InterfaceKey);
	if (status != ERROR_SUCCESS) {
		RegCloseKey(SystemKey);
		goto fail;
	}
	RegCloseKey(SystemKey);
	status = RegOpenKeyEx(InterfaceKey,TEXT("Parameters"),0,KEY_READ,&ParametersKey);
	if (status != ERROR_SUCCESS) {
		RegCloseKey(InterfaceKey);
		goto fail;
	}
	RegCloseKey(InterfaceKey);
	status = RegOpenKeyEx(ParametersKey,TEXT("TcpIp"),0,KEY_READ,&TcpIpKey);
	if (status != ERROR_SUCCESS) {
		RegCloseKey(ParametersKey);
		goto fail;
	}
	RegCloseKey(ParametersKey);
		
	BufLen = 4;
	/* See if DHCP is used by this system */
	status=RegQueryValueEx(TcpIpKey,TEXT("EnableDHCP"),NULL,&RegType,(LPBYTE)&DHCPEnabled,&BufLen);
	if (status != ERROR_SUCCESS)
		DHCPEnabled=0;

	
	/* Retrieve the netmask */
	if(DHCPEnabled){
		
		BufLen = sizeof String;
		status = RegQueryValueEx(TcpIpKey,TEXT("DhcpIPAddress"),NULL,&RegType,(LPBYTE)String,&BufLen);
		if (status != ERROR_SUCCESS) {
			RegCloseKey(TcpIpKey);
			goto fail;
		}

		TAddr = inet_addrU(String);
		// swap bytes for backward compatibility
		for(i=0;i<4;i++){
			*((char*)netp+i) = *((char*)&TAddr+3-i);
		}
		
		BufLen = sizeof String;
		status=RegQueryValueEx(TcpIpKey,TEXT("DHCPSubnetMask"),NULL,&RegType,
			(LPBYTE)String,&BufLen);
		
		TAddr = inet_addrU(String);
		// swap bytes for backward compatibility
		for(i=0;i<4;i++){
			*((char*)maskp+i) = *((char*)&TAddr+3-i);
		}
		
		
	}
	else{

		BufLen = sizeof String;
		status = RegQueryValueEx(TcpIpKey,TEXT("IPAddress"),NULL,&RegType,(LPBYTE)String,&BufLen);
		if (status != ERROR_SUCCESS) {
			RegCloseKey(TcpIpKey);
			goto fail;
		}

		TAddr = inet_addrU(String);
		// swap bytes for backward compatibility
		for(i=0;i<4;i++){
			*((char*)netp+i) = *((char*)&TAddr+3-i);
		}
		
		BufLen = sizeof String;
		status=RegQueryValueEx(TcpIpKey,TEXT("SubnetMask"),NULL,&RegType,
			(LPBYTE)String,&BufLen);
		
		TAddr = inet_addrU(String);
		// swap bytes for backward compatibility
		for(i=0;i<4;i++){
			*((char*)maskp+i) = *((char*)&TAddr+3-i);
		}


	}
	
	if (status != ERROR_SUCCESS) {
		RegCloseKey(TcpIpKey);
		goto fail;
	}
	
		
	if (AdapterNameU != NULL)
		free(AdapterNameU);
	return TRUE;
	
fail:
	if (AdapterNameU != NULL)
		free(AdapterNameU);
    return FALSE;
}

/* @} */
