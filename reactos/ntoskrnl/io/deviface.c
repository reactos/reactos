/*
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/io/pnpmgr/devintrf.c
 * PURPOSE:        Device interface functions
 * PROGRAMMER:     Filip Navara (xnavara@volny.cz)
 *                 Matthew Brace (ismarc@austin.rr.com)
 * UPDATE HISTORY:
  *  22/09/2003 FiN Created
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <reactos/bugcodes.h>
#include <internal/io.h>
#include <internal/po.h>
#include <internal/ldr.h>
#include <internal/module.h>

//#define NDEBUG
#include <internal/debug.h>

#include <ole32/guiddef.h>
#ifdef DEFINE_GUID
DEFINE_GUID(GUID_CLASS_COMPORT,          0x86e0d1e0L, 0x8089, 0x11d0, 0x9c, 0xe4, 0x08, 0x00, 0x3e, 0x30, 0x1f, 0x73);
DEFINE_GUID(GUID_SERENUM_BUS_ENUMERATOR, 0x4D36E978L, 0xE325, 0x11CE, 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18);
#endif // DEFINE_GUID


/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 *
 * returns a key to a specific device interface
 */
NTSTATUS
STDCALL
IoOpenDeviceInterfaceRegistryKey(
  IN PUNICODE_STRING SymbolicLinkName,
  IN ACCESS_MASK DesiredAccess,
  OUT PHANDLE DeviceInterfaceKey)
{
  NTSTATUS 				Status;
  POBJECT_ATTRIBUTES	ObjectAttributes;
  
  InitializeObjectAttributes( ObjectAttributes,
  							  SymbolicLinkName,
  							  OBJ_CASE_INSENSITIVE,
  							  NULL,
  							  NULL );
  							  
  Status = ZwOpenKey( DeviceInterfaceKey,
  					  DesiredAccess,
  					  ObjectAttributes );
  					  
  if( !NT_SUCCESS( Status ) )
  {
    DPRINT( "ZwOpenKey() failed" );
    return Status;
  }

  return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
IoGetDeviceInterfaceAlias(
  IN PUNICODE_STRING SymbolicLinkName,
  IN CONST GUID *AliasInterfaceClassGuid,
  OUT PUNICODE_STRING AliasSymbolicLinkName)
{
  PUNICODE_STRING GuidString;
  PUNICODE_STRING BaseKeyName;
  PUNICODE_STRING AliasKeyName;
  PUNICODE_STRING bipName;
  PUNICODE_STRING InLinkName;

  PHANDLE InterfaceKey;
  BOOLEAN CaseInsensitive = TRUE;
  
  PKEY_FULL_INFORMATION fip;
  PKEY_BASIC_INFORMATION bip;

  PWCHAR pdest;
    
  LONG status;
  NTSTATUS Status;
  
  ULONG Size;
  ULONG i = 0;
  
  PWCHAR BaseKeyString = L"\\HKEY_LOCAL_MACHINE\\System\\CurrentControlSet\\Control\\Class\\";
  PWCHAR BaseInterfaceString = L"\\HKEY_LOCAL_MACHINE\\System\\CurrentControlSet\\Services\\";
  
  
  status = RtlStringFromGUID( AliasInterfaceClassGuid, GuidString );
  if( !status )
  {
	  DPRINT( "RtlStringFromGUID() Failed.\n" );
	  return STATUS_INVALID_HANDLE;
  }
  RtlInitUnicodeString( BaseKeyName, BaseKeyString );
  RtlInitUnicodeString( AliasKeyName, BaseInterfaceString );
  
  BaseKeyName->MaximumLength += sizeof( GuidString );
  RtlAppendUnicodeStringToString( BaseKeyName, GuidString );
  
  Status = IoOpenDeviceInterfaceRegistryKey( BaseKeyName,
  				 				    		 GENERIC_READ,
  						   					 InterfaceKey);
  						   		
  if( !NT_SUCCESS( Status ) )
  {
    DPRINT( "IoGetDeviceInterfaceKey() failed.\n" );
    return Status;
  }
  
  Status = ZwQueryKey( InterfaceKey,
  				 	   KeyFullInformation,
  					   NULL,
  			  		   0,
  			  		   &Size );
  
  if( !NT_SUCCESS( Status) )
  {
    DPRINT( "ZwQueryKey() failed.\n" );
    return Status;
  }
  
  fip = (PKEY_FULL_INFORMATION) ExAllocatePool(PagedPool, Size);
  							  				 
  ZwQueryKey( InterfaceKey,
  			  KeyFullInformation,
  			  fip,
  			  Size,
  			  &Size );

  while( i < fip->SubKeys )
  {
	i++;
    ZwEnumerateKey( InterfaceKey,
    			    i,
    			    KeyBasicInformation,
    			    NULL,
    			    0,
    			    &Size );
    bip = (PKEY_BASIC_INFORMATION) ExAllocatePool(PagedPool, Size);
    
    ZwEnumerateKey( InterfaceKey,
    				i,
    				KeyBasicInformation,
    				bip,
    				Size,
    				&Size );
    //check bip->Name
    RtlInitUnicodeString( bipName, NULL );
    RtlInitUnicodeString( InLinkName, NULL );

	bipName->Length = wcslen(bip->Name);
	bipName->MaximumLength = wcslen(bip->Name);
	bipName->Buffer = bip->Name;

	AliasKeyName->MaximumLength += sizeof(bipName);
    RtlAppendUnicodeStringToString( AliasKeyName, bipName );

    pdest = wcsstr( SymbolicLinkName->Buffer, L"Services\\" );
    
    InLinkName->Length = wcslen(pdest);
    InLinkName->MaximumLength = wcslen(pdest);
    InLinkName->Buffer = pdest;
    
	status = 1;
	status = RtlCompareUnicodeString( bipName, 
    				 				  InLinkName, 
    						 		  CaseInsensitive );    
    
    if( status == 0 )
    {
	    AliasSymbolicLinkName = AliasKeyName; 
	    return STATUS_SUCCESS;
    }
    
    					
    ExFreePool(bip);
  }

  ExFreePool(fip);
  
  return STATUS_INVALID_HANDLE;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoGetDeviceInterfaces(
  IN CONST GUID *InterfaceClassGuid,
  IN PDEVICE_OBJECT PhysicalDeviceObject  OPTIONAL,
  IN ULONG Flags,
  OUT PWSTR *SymbolicLinkList)
{
  DPRINT("IoGetDeviceInterfaces called (UNIMPLEMENTED)\n");
  return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoRegisterDeviceInterface(
  IN PDEVICE_OBJECT PhysicalDeviceObject,
  IN CONST GUID *InterfaceClassGuid,
  IN PUNICODE_STRING ReferenceString  OPTIONAL,
  OUT PUNICODE_STRING SymbolicLinkName)
{
  PWCHAR KeyNameString = L"\\Device\\Serenum";

  DPRINT("IoRegisterDeviceInterface called (UNIMPLEMENTED)\n");
  if (IsEqualGUID(InterfaceClassGuid, (LPGUID)&GUID_SERENUM_BUS_ENUMERATOR))
  {
    RtlInitUnicodeString(SymbolicLinkName, KeyNameString);
    return STATUS_SUCCESS;
  }
  return STATUS_INVALID_DEVICE_REQUEST;
//    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoSetDeviceInterfaceState(
  IN PUNICODE_STRING SymbolicLinkName,
  IN BOOLEAN Enable)
{
  DPRINT("IoSetDeviceInterfaceState called (UNIMPLEMENTED)\n");
  return STATUS_SUCCESS;

//	return STATUS_OBJECT_NAME_EXISTS;
//	return STATUS_OBJECT_NAME_NOT_FOUND;
//    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
