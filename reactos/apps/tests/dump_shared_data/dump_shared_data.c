#include <stdio.h>
#include <ntddk.h>
#include <napi/shared_data.h>

int main()
{
   printf("TickCountLow: %x\n", 
	  ((PKUSER_SHARED_DATA)USER_SHARED_DATA_BASE)->TickCountLow);
}
