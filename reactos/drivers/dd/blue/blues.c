
#undef WIN32_LEAN_AND_MEAN
#include <internal/mmhal.h>
#include <internal/halio.h>
#include <ddk/ntddk.h>
#include <internal/string.h>
#include <defines.h>

//#define NDEBUG
#include <internal/debug.h>



#define FSCTL_GET_CONSOLE_SCREEN_BUFFER_INFO              CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 254, DO_DIRECT_IO, FILE_READ_ACCESS|FILE_WRITE_ACCESS)
#define FSCTL_SET_CONSOLE_SCREEN_BUFFER_INFO              CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 255, DO_DIRECT_IO, FILE_READ_ACCESS|FILE_WRITE_ACCESS)



CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;


VOID ScrStartIo(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION stk = IoGetCurrentIrpStackLocation(Irp);
    
}

NTSTATUS ScrDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION stk = IoGetCurrentIrpStackLocation(Irp);
   ULONG ControlCode;
   NTSTATUS Status;
   char *UserBuf = Irp->UserBuffer;
   int i;

   switch (stk->MajorFunction)
     {
      case IRP_MJ_CREATE:
      case IRP_MJ_CLOSE:
        Status = STATUS_SUCCESS;
	break;
	
      case IRP_MJ_WRITE:
	for(i=0;i<stk->Parameters.Write.Length;i++)	
		__putchar(UserBuf[i]);
	Status = STATUS_SUCCESS;
	break;
      case IRP_MJ_DEVICE_CONTROL:
	ControlCode = stk->Parameters.DeviceIoControl.IoControlCode;
	if ( ControlCode == FSCTL_GET_CONSOLE_SCREEN_BUFFER_INFO  ) {
	//	printk("get console screen buffer info\n");
		ConsoleScreenBufferInfo.dwCursorPosition.X=__wherex();
		ConsoleScreenBufferInfo.dwCursorPosition.Y=__wherey();

		__getscreensize(&ConsoleScreenBufferInfo.dwSize.X, &ConsoleScreenBufferInfo.dwSize.Y );

		memcpy(UserBuf,&ConsoleScreenBufferInfo,sizeof(CONSOLE_SCREEN_BUFFER_INFO));
		Status = STATUS_SUCCESS;
	}
	else if ( ControlCode == FSCTL_SET_CONSOLE_SCREEN_BUFFER_INFO  ) {
	//	printk("set console screen buffer info\n");
		memcpy(&ConsoleScreenBufferInfo,UserBuf,sizeof(CONSOLE_SCREEN_BUFFER_INFO));
		__goxy(ConsoleScreenBufferInfo.dwCursorPosition.X,ConsoleScreenBufferInfo.dwCursorPosition.Y);
		Status = STATUS_SUCCESS;
	
	}
	break;
	
      default:
        Status = STATUS_NOT_IMPLEMENTED;
	break;
     }
   
  
//   DPRINT("Status %d\n",Status);
   return(Status);
}

/*
 * Module entry point
 */
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
   PDEVICE_OBJECT DeviceObject;
   ANSI_STRING adevice_name;
   UNICODE_STRING device_name;
   ANSI_STRING asymlink_name;
   UNICODE_STRING symlink_name;
   
   DbgPrint("Screen Driver 0.0.4\n");
  
   
   DriverObject->MajorFunction[IRP_MJ_CREATE] = ScrDispatch;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = ScrDispatch;
   DriverObject->MajorFunction[IRP_MJ_READ] = ScrDispatch;
   DriverObject->MajorFunction[IRP_MJ_WRITE] = ScrDispatch;
   DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL ] = ScrDispatch;	
   DriverObject->DriverStartIo = ScrStartIo;

   //ScrSwitchToBlueScreen();
   
   
   RtlInitAnsiString(&adevice_name,"\\Device\\BlueScreen");
   RtlAnsiStringToUnicodeString(&device_name,&adevice_name,TRUE);
   IoCreateDevice(DriverObject,0,&device_name,FILE_DEVICE_SCREEN,0,
		  TRUE,&DeviceObject);
  
   
   RtlInitAnsiString(&asymlink_name,"\\??\\BlueScreen");
   RtlAnsiStringToUnicodeString(&symlink_name,&asymlink_name,TRUE);
   IoCreateSymbolicLink(&symlink_name,&device_name);
   
   return(STATUS_SUCCESS);
}




/* FUNCTIONS ***************************************************************/


void ScrSwitchToBlueScreen(void)
/*
 * FUNCTION: Switches the monitor to text mode and writes a blue background
 * NOTE: This function is entirely self contained and can be used from any
 * graphics mode. 
 */
{
 
   /*
    * Reset the cursor position
    */
   ConsoleScreenBufferInfo.dwCursorPosition.X=__wherex();
   ConsoleScreenBufferInfo.dwCursorPosition.Y=__wherey();

   __getscreensize(&ConsoleScreenBufferInfo.dwSize.X, &ConsoleScreenBufferInfo.dwSize.Y );

   
   /*
    * This code section is taken from the sample routines by
    * Jeff Morgan (kinfira@hotmail.com)
    */
   
}










