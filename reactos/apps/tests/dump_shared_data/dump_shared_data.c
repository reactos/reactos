#include <stdio.h>
#include <ntddk.h>

int main()
{
  printf("TickCountLow: %x\n", 
	 SharedUserData->TickCountLow);
}
