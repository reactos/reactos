#ifndef IPPRIVATE_H
#define IPPRIVATE_H

typedef void (*EnumInterfacesFunc)( HANDLE RegHandle, PWCHAR InterfaceName,
				    PVOID Data );
typedef void (*EnumNameServersFunc)( PWCHAR InterfaceName, PWCHAR Server,
				     PVOID Data );

typedef struct {
  int NumServers;
  int CurrentName;
  PIP_ADDR_STRING AddrString;
} NAME_SERVER_LIST_PRIVATE, *PNAME_SERVER_LIST_PRIVATE;

#endif/*IPPRIVATE_H*/
