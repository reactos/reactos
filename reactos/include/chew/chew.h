#ifndef _REACTOS_CHEW_H
#define _REACTOS_CHEW_H

VOID ChewInit( PDEVICE_OBJECT DeviceObject );
VOID ChewShutdown();
BOOLEAN ChewCreate
( PVOID *Item, UINT Bytes, VOID (*Worker)(PVOID), PVOID UserSpace );
VOID ChewRemove( PVOID Item );

#endif/*_REACTOS_CHEW_H*/
