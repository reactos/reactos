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

#include <packet32.h>
#include <windows.h>
#include <windowsx.h>
#include <ntddndis.h>


/// Title of error windows
TCHAR   szWindowTitle[] = TEXT("PACKET.DLL");

#if _DBG
#define ODS(_x) OutputDebugString(TEXT(_x))
#define ODSEx(_x, _y)
#else
#ifdef _DEBUG_TO_FILE
#include <stdio.h>
// Macro to print a debug string. The behavior differs depending on the debug level
#define ODS(_x) { \
	FILE *f; \
	f = fopen("winpcap_debug.txt", "a"); \
	fprintf(f, "%s", _x); \
	fclose(f); \
}
// Macro to print debug data with the printf convention. The behavior differs depending on */
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
LPCTSTR NPFRegistryLocation = TEXT("SYSTEM\\CurrentControlSet\\Services\\NPF");


//---------------------------------------------------------------------------

BOOL APIENTRY DllMain (HANDLE DllHandle,DWORD Reason,LPVOID lpReserved)
{
    BOOLEAN Status=TRUE;

    switch ( Reason )
    {
	case DLL_PROCESS_ATTACH:
		
		ODS("\n************Packet32: DllMain************\n");
		
#ifdef _DEBUG_TO_FILE
		// dump a bunch of registry keys useful for debug to file
		PacketDumpRegistryKey("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}",
			"adapters.reg");
		PacketDumpRegistryKey("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip",
			"tcpip.reg");
		PacketDumpRegistryKey("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\NPF",
			"npf.reg");
		PacketDumpRegistryKey("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services",
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

//---------------------------------------------------------------------------

WCHAR* SChar2WChar(char* string)
{
	WCHAR* TmpStr;
	TmpStr=(WCHAR*) malloc ((strlen(string)+2)*sizeof(WCHAR));

	MultiByteToWideChar(CP_ACP, 0, string, -1, TmpStr, (strlen(string)+2));

	return TmpStr;
}

//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------

BOOLEAN PacketSetReadEvt(LPADAPTER AdapterObject)
{
	DWORD BytesReturned;
	TCHAR EventName[39];

	// this tells the terminal service to retrieve the event from the global namespace
	wcsncpy(EventName,L"Global\\",sizeof(L"Global\\"));

	// retrieve the name of the shared event from the driver
	if(DeviceIoControl(AdapterObject->hFile,pBIOCEVNAME,NULL,0,EventName+7,13*sizeof(TCHAR),&BytesReturned,NULL)==FALSE) return FALSE;

	EventName[20]=0; // terminate the string

	// open the shared event
	AdapterObject->ReadEvent=CreateEvent(NULL,
										 TRUE,
										 FALSE,
										 EventName);

	// in NT4 "Global\" is not automatically ignored: try to use simply the event name
	if(GetLastError()!=ERROR_ALREADY_EXISTS){
		if(AdapterObject->ReadEvent != NULL)
			CloseHandle(AdapterObject->ReadEvent);
		
		// open the shared event
		AdapterObject->ReadEvent=CreateEvent(NULL,
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

//---------------------------------------------------------------------------

BOOL PacketInstallDriver(SC_HANDLE ascmHandle,SC_HANDLE *srvHandle,TCHAR *driverPath)
{
	BOOL result = FALSE;
	ULONG err;
	
	ODS("installdriver\n")
	
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
		}
		else {
			//Created service for npf.sys
			result = TRUE;
		}
	}
	if (result == TRUE) 
		if (*srvHandle != NULL)
			CloseServiceHandle(*srvHandle);

	if(result == FALSE){
		err = GetLastError();
		if(err != 2)
			ODSEx("PacketInstallDriver failed, Error=%d\n",err);
	}
	return result;
	
}

//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------

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

/// Current packet.dll Version. It can be retrieved directly or through the PacketGetVersion() function.
char PacketLibraryVersion[] = "2.3"; 

//---------------------------------------------------------------------------

PCHAR PacketGetVersion(){
	return PacketLibraryVersion;
}

//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------

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

    ODSEx("PacketOpenAdapter: trying to open the adapter=%S\n",AdapterName)

	scmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	
	if(scmHandle == NULL){
		error = GetLastError();
		ODSEx("OpenSCManager failed! Error=%d\n", error);
	}
	else{
		*driverPath = 0;
		GetCurrentDirectory(512, driverPath);
		wsprintf(driverPath + wcslen(driverPath), 
			NPFDriverName);
		
		// check if the NPF registry key is already present
		// this means that the driver is already installed and that we don't need to call PacketInstallDriver
		KeyRes=RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			NPFRegistryLocation,
			0,
			KEY_READ,
			&PathKey);
		
		if(KeyRes != ERROR_SUCCESS){
			Result = PacketInstallDriver(scmHandle,&svcHandle,driverPath);
		}
		else{
			Result = TRUE;
			RegCloseKey(PathKey);
		}
		
		if (Result) {
			
			srvHandle = OpenService(scmHandle, NPFServiceName, SERVICE_START | SERVICE_QUERY_STATUS );
			if (srvHandle != NULL){
				
				QuerySStat = QueryServiceStatus(srvHandle, &SStat);
				ODSEx("The status of the driver is:%d\n",SStat.dwCurrentState);
				
				if(!QuerySStat || SStat.dwCurrentState != SERVICE_RUNNING){
					ODS("Calling startservice\n");
					if (StartService(srvHandle, 0, NULL)==0){ 
						error = GetLastError();
						if(error!=ERROR_SERVICE_ALREADY_RUNNING && error!=ERROR_ALREADY_EXISTS){
							SetLastError(error);
							if (scmHandle != NULL) CloseServiceHandle(scmHandle);
							error = GetLastError();
							ODSEx("PacketOpenAdapter: StartService failed, Error=%d\n",error);
							return NULL;
						}
					}				
				}
			}
			else{
				error = GetLastError();
				ODSEx("OpenService failed! Error=%d", error);
			}
		}
		else{
			if( GetSystemDirectory(WinPath, sizeof(WinPath)/sizeof(TCHAR)) == 0) return FALSE;
			wsprintf(driverPath,
				TEXT("%s\\drivers%s"), 
				WinPath,NPFDriverName);
			
			if(KeyRes != ERROR_SUCCESS)
				Result = PacketInstallDriver(scmHandle,&svcHandle,driverPath);
			else
				Result = TRUE;
			
			if (Result) {
				
				srvHandle = OpenService(scmHandle,NPFServiceName,SERVICE_START);
				if (srvHandle != NULL){
					
					QuerySStat = QueryServiceStatus(srvHandle, &SStat);
					ODSEx("The status of the driver is:%d\n",SStat.dwCurrentState);
					
					if(!QuerySStat || SStat.dwCurrentState != SERVICE_RUNNING){
						
						ODS("Calling startservice\n");
						
						if (StartService(srvHandle, 0, NULL)==0){ 
							error = GetLastError();
							if(error!=ERROR_SERVICE_ALREADY_RUNNING && error!=ERROR_ALREADY_EXISTS){
								SetLastError(error);
								if (scmHandle != NULL) CloseServiceHandle(scmHandle);
								ODSEx("PacketOpenAdapter: StartService failed, Error=%d\n",error);
								return NULL;
							}
						}
					}
				}
				else{
					error = GetLastError();
					ODSEx("OpenService failed! Error=%d", error);
				}
			}
		}
	}

    if (scmHandle != NULL) CloseServiceHandle(scmHandle);

	AdapterNameA=(char*)AdapterName;
	if(AdapterNameA[1]!=0){ //ASCII
		AdapterNameU=SChar2WChar(AdapterNameA);
		AdapterName=AdapterNameU;
	} else {			//Unicode
		AdapterNameU=NULL;
	}
	
	lpAdapter=(LPADAPTER)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,sizeof(ADAPTER));
	if (lpAdapter==NULL)
	{
		ODS("PacketOpenAdapter: GlobalAlloc Failed\n");
		error=GetLastError();
		if (AdapterNameU != NULL) free(AdapterNameU);
		//set the error to the one on which we failed
		SetLastError(error);
	    ODS("PacketOpenAdapter: Failed to allocate the adapter structure\n");
		return NULL;
	}
	lpAdapter->NumWrites=1;

	wsprintf(lpAdapter->SymbolicLink,TEXT("\\\\.\\%s%s"),DOSNAMEPREFIX,&AdapterName[8]);
	
	//try if it is possible to open the adapter immediately
	lpAdapter->hFile=CreateFile(lpAdapter->SymbolicLink,GENERIC_WRITE | GENERIC_READ,
		0,NULL,OPEN_EXISTING,0,0);
	
	if (lpAdapter->hFile != INVALID_HANDLE_VALUE) {

		if(PacketSetReadEvt(lpAdapter)==FALSE){
			error=GetLastError();
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
	else{
		Result=DefineDosDevice(DDD_RAW_TARGET_PATH,&lpAdapter->SymbolicLink[4],AdapterName);
		if (Result)
		{

			lpAdapter->hFile=CreateFile(lpAdapter->SymbolicLink,GENERIC_WRITE | GENERIC_READ,
				0,NULL,OPEN_EXISTING,0,0);
			if (lpAdapter->hFile != INVALID_HANDLE_VALUE)
			{		
				
				if(PacketSetReadEvt(lpAdapter)==FALSE){
					error=GetLastError();
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
			}
		}
	}

	error=GetLastError();
	if (AdapterNameU != NULL)
	    free(AdapterNameU);
	GlobalFreePtr(lpAdapter);
	//set the error to the one on which we failed
	SetLastError(error);
    ODSEx("PacketOpenAdapter: CreateFile failed, Error=2,%d\n",error);
	return NULL;

}

//---------------------------------------------------------------------------

VOID PacketCloseAdapter(LPADAPTER lpAdapter)
{
    CloseHandle(lpAdapter->hFile);
	SetEvent(lpAdapter->ReadEvent);
    CloseHandle(lpAdapter->ReadEvent);
    GlobalFreePtr(lpAdapter);
}

//---------------------------------------------------------------------------

LPPACKET PacketAllocatePacket(void)
{

    LPPACKET    lpPacket;
    lpPacket=(LPPACKET)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,sizeof(PACKET));
    if (lpPacket==NULL)
    {
        ODS("PacketAllocatePacket: GlobalAlloc Failed\n");
        return NULL;
    }
    return lpPacket;
}

//---------------------------------------------------------------------------

VOID PacketFreePacket(LPPACKET lpPacket)
{
    GlobalFreePtr(lpPacket);
}

//---------------------------------------------------------------------------

VOID PacketInitPacket(LPPACKET lpPacket,PVOID Buffer,UINT Length)
{
    lpPacket->Buffer = Buffer;
    lpPacket->Length = Length;
	lpPacket->ulBytesReceived = 0;
	lpPacket->bIoComplete = FALSE;
}

//---------------------------------------------------------------------------

BOOLEAN PacketReceivePacket(LPADAPTER AdapterObject,LPPACKET lpPacket,BOOLEAN Sync)
{
	BOOLEAN res;
	if ((int)AdapterObject->ReadTimeOut != -1)
		WaitForSingleObject(AdapterObject->ReadEvent, (AdapterObject->ReadTimeOut==0)?INFINITE:AdapterObject->ReadTimeOut);
    res = ReadFile(AdapterObject->hFile, lpPacket->Buffer, lpPacket->Length, &lpPacket->ulBytesReceived,NULL);
	return res;
}
/* 
ReadFile(
	 HANDLE hFile,
	 LPVOID lpBuffer,
	 DWORD nNumberOfBytesToRead,
	 LPDWORD lpNumberOfBytesRead,
	 LPOVERLAPPED lpOverlapped
	 );
 */
//---------------------------------------------------------------------------

BOOLEAN PacketSendPacket(LPADAPTER AdapterObject,LPPACKET lpPacket,BOOLEAN Sync)
{
    DWORD BytesTransfered;
    return WriteFile(AdapterObject->hFile,lpPacket->Buffer,lpPacket->Length,&BytesTransfered,NULL);
}

//---------------------------------------------------------------------------

BOOLEAN PacketSetMinToCopy(LPADAPTER AdapterObject,int nbytes)
{
	DWORD BytesReturned;
    return DeviceIoControl(AdapterObject->hFile,pBIOCSMINTOCOPY,&nbytes,4,NULL,0,&BytesReturned,NULL);
}

//---------------------------------------------------------------------------

BOOLEAN PacketSetMode(LPADAPTER AdapterObject,int mode)
{
	DWORD BytesReturned;
    return DeviceIoControl(AdapterObject->hFile,pBIOCSMODE,&mode,4,NULL,0,&BytesReturned,NULL);
}

//---------------------------------------------------------------------------

HANDLE PacketGetReadEvent(LPADAPTER AdapterObject)
{
    return AdapterObject->ReadEvent;
}

//---------------------------------------------------------------------------

BOOLEAN PacketSetNumWrites(LPADAPTER AdapterObject,int nwrites)
{
	DWORD BytesReturned;
    return DeviceIoControl(AdapterObject->hFile,pBIOCSWRITEREP,&nwrites,4,NULL,0,&BytesReturned,NULL);
}

//---------------------------------------------------------------------------

BOOLEAN PacketSetReadTimeout(LPADAPTER AdapterObject,int timeout)
{
	DWORD BytesReturned;
	int DriverTimeOut=-1;

	AdapterObject->ReadTimeOut=timeout;

    return DeviceIoControl(AdapterObject->hFile,pBIOCSRTIMEOUT,&DriverTimeOut,4,NULL,0,&BytesReturned,NULL);
}

//---------------------------------------------------------------------------

BOOLEAN PacketSetBuff(LPADAPTER AdapterObject,int dim)
{
	DWORD BytesReturned;
    return DeviceIoControl(AdapterObject->hFile,pBIOCSETBUFFERSIZE,&dim,4,NULL,0,&BytesReturned,NULL);
}

//---------------------------------------------------------------------------

BOOLEAN PacketSetBpf(LPADAPTER AdapterObject,struct bpf_program *fp)
{
	DWORD BytesReturned;
    return DeviceIoControl(AdapterObject->hFile,pBIOCSETF,(char*)fp->bf_insns,fp->bf_len*sizeof(struct bpf_insn),NULL,0,&BytesReturned,NULL);
}

//---------------------------------------------------------------------------

BOOLEAN PacketGetStats(LPADAPTER AdapterObject,struct bpf_stat *s)
{
	DWORD BytesReturned;
	return DeviceIoControl(AdapterObject->hFile,pBIOCGSTATS,NULL,0,s,sizeof(struct bpf_stat),&BytesReturned,NULL);
}

//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------

BOOLEAN PacketGetAdapterNames(PTSTR pStr,PULONG  BufferSize)
{
    HKEY       LinkageKey,AdapKey;
	DWORD	   RegKeySize=0;
    LONG       Status;
	ULONG	   Result;
	PTSTR      BpStr;
	char       *TTpStr,*DpStr,*DescBuf;
	LPADAPTER  adapter;
    PPACKET_OID_DATA  OidData;
	int		   i=0,k,rewind;
    DWORD      dim;
	TCHAR	   AdapName[256];

    ODSEx("PacketGetAdapterNames: BufferSize=%d\n",*BufferSize);

    OidData=GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,512);
    if (OidData == NULL) {
        ODS("PacketGetAdapterNames: GlobalAlloc Failed\n");
        return FALSE;
    }

    Status=RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		                TEXT("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}"),
						0,
						KEY_READ,
						&AdapKey);

	// Get the size to allocate for the original device names
	while((Result=RegEnumKey(AdapKey,i,AdapName,sizeof(AdapName)/2))==ERROR_SUCCESS)
	{
		Status=RegOpenKeyEx(AdapKey,AdapName,0,KEY_READ,&LinkageKey);
		Status=RegOpenKeyEx(LinkageKey,L"Linkage",0,KEY_READ,&LinkageKey);
        Status=RegQueryValueEx(LinkageKey,L"Export",NULL,NULL,NULL,&dim);
		i++;
		if(Status!=ERROR_SUCCESS) continue;
		RegKeySize+=dim;
	}
	
	// Allocate the memory for the original device names
	ODSEx("Need %d bytes for the names\n", RegKeySize+2);
	BpStr=GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,RegKeySize+2);
	if (BpStr == NULL || RegKeySize > *BufferSize) {
        ODS("PacketGetAdapterNames: GlobalAlloc Failed\n");
		GlobalFreePtr(OidData);
		return FALSE;
	}

	k=0;
	i=0;

    ODS("PacketGetAdapterNames: Cycling through the adapters:\n");

	// Copy the names to the buffer
	while((Result=RegEnumKey(AdapKey,i,AdapName,sizeof(AdapName)/2))==ERROR_SUCCESS)
	{
		WCHAR UpperBindStr[64];

		i++;
		ODSEx(" %d) ", i);

		Status=RegOpenKeyEx(AdapKey,AdapName,0,KEY_READ,&LinkageKey);
		Status=RegOpenKeyEx(LinkageKey,L"Linkage",0,KEY_READ,&LinkageKey);

		dim=sizeof(UpperBindStr);
        Status=RegQueryValueEx(LinkageKey,L"UpperBind",NULL,NULL,(PUCHAR)UpperBindStr,&dim);
		
		ODSEx("UpperBind=%S ", UpperBindStr);

		if( Status!=ERROR_SUCCESS || _wcsicmp(UpperBindStr,L"NdisWan")==0 ){
			ODS("Name = SKIPPED\n");
			continue;
		}

		dim=RegKeySize-k;
        Status=RegQueryValueEx(LinkageKey,L"Export",NULL,NULL,(LPBYTE)BpStr+k,&dim);
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
			memcpy(pStr+k+8,TEXT("Packet_"),14);
			i+=8;
			k+=15;
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
		*DpStr=0;

		pStr[k++]=0;
		pStr[k]=0;

		if((ULONG)(DpStr-DescBuf+k) < *BufferSize)
			memcpy(pStr+k,DescBuf,DpStr-DescBuf);
		else{
		    GlobalFreePtr(OidData);
			GlobalFreePtr (BpStr);
			GlobalFreePtr (DescBuf);
			ODS("\nPacketGetAdapterNames: ended with failure\n");
			return FALSE;
		}

	    GlobalFreePtr(OidData);
		GlobalFreePtr (BpStr);
		GlobalFreePtr (DescBuf);
		ODS("\nPacketGetAdapterNames: ended correctly\n");
		return TRUE;
	}
	else{
	    DWORD      RegType;

		ODS("Adapters not found under SYSTEM\\CurrentControlSet\\Control\\Class. Using the TCP/IP bindings.\n");

		GlobalFreePtr (BpStr);

		Status=RegOpenKeyEx(HKEY_LOCAL_MACHINE,TEXT("SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Linkage"),0,KEY_READ,&LinkageKey);
		if (Status == ERROR_SUCCESS)
		{
			// Retrieve the length of the key
			Status=RegQueryValueEx(LinkageKey,TEXT("bind"),NULL,&RegType,NULL,&RegKeySize);
			// Allocate the buffer
			BpStr=GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,RegKeySize+2);
			if (BpStr == NULL || RegKeySize > *BufferSize) {
				GlobalFreePtr(OidData);
				return FALSE;
			}
			Status=RegQueryValueEx(LinkageKey,TEXT("bind"),NULL,&RegType,(LPBYTE)BpStr,&RegKeySize);
			RegCloseKey(LinkageKey);
		}
		
		if (Status==ERROR_SUCCESS){
			
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
					return FALSE;
				}
				
				// Create the device name
				rewind=k;
				memcpy(pStr+k,BpStr+i,16);
				memcpy(pStr+k+8,TEXT("Packet_"),14);
				i+=8;
				k+=15;
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
				Status = PacketRequest(adapter,FALSE,OidData);
				if(Status==0 || ((char*)OidData->Data)[0]==0){
					k=rewind;
					continue;
				}
				
				// Copy the description
				TTpStr=(char*)(OidData->Data);
				while(*TTpStr!=0){
					*DpStr++=*TTpStr++;
				}
				*DpStr++=*TTpStr++;
				
				// Close the adapter
				PacketCloseAdapter(adapter);
				
			}
			*DpStr=0;
			
			pStr[k++]=0;
			pStr[k]=0;
			
			if((ULONG)(DpStr-DescBuf+k) < *BufferSize)
				memcpy(pStr+k,DescBuf,DpStr-DescBuf);
			else{
				GlobalFreePtr(OidData);
				GlobalFreePtr (BpStr);
				GlobalFreePtr (DescBuf);
				return FALSE;
			}
			
			GlobalFreePtr(OidData);
			GlobalFreePtr (BpStr);
			GlobalFreePtr (DescBuf);
			return TRUE;
		}
		else{
			MessageBox(NULL,TEXT("Can not find TCP/IP bindings.\nIn order to run the packet capture driver you must install TCP/IP."),szWindowTitle,MB_OK);
			ODS("Cannot find the TCP/IP bindings");
			return FALSE;
		}
	}
}

//---------------------------------------------------------------------------

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
	if (wcsncmp(ifname, L"Packet_", 7) == 0)
		ifname += 7;

	if(	RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces"), 0, KEY_READ, &UnderTcpKey) == ERROR_SUCCESS)
	{
		status = RegOpenKeyEx(UnderTcpKey,ifname,0,KEY_READ,&TcpIpKey);
		if (status != ERROR_SUCCESS) {
			RegCloseKey(UnderTcpKey);
			goto fail;
		}
	}
	else
	{
		
		// Query the registry key with the interface's adresses
		status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,TEXT("SYSTEM\\CurrentControlSet"),0,KEY_READ,&SystemKey);
		if (status != ERROR_SUCCESS)
			goto fail;
		status = RegOpenKeyEx(SystemKey,ifname,0,KEY_READ,&InterfaceKey);
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

//---------------------------------------------------------------------------

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
	if (wcsncmp(ifname, L"Packet_", 7) == 0)
		ifname += 7;
	status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,TEXT("SYSTEM\\CurrentControlSet\\Services"),0,KEY_READ,&SystemKey);
	if (status != ERROR_SUCCESS)
		goto fail;
	status = RegOpenKeyEx(SystemKey,ifname,0,KEY_READ,&InterfaceKey);
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
