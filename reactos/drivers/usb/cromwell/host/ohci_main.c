/*
   ReactOS specific functions for ohci module
   by Aleksey Bragin (aleksey@reactos.com)
*/

#include <ddk/ntddk.h>


/*
 * Standard DriverEntry method.
 */
NTSTATUS STDCALL
DriverEntry(IN PVOID Context1, IN PVOID Context2)
{
	return STATUS_SUCCESS;
}
