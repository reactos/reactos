/* $Id: conio.c,v 1.6 2000/05/08 23:27:03 ekohl Exp $
 *
 * reactos/subsys/csrss/api/conio.c
 *
 * Console I/O functions
 *
 * ReactOS Operating System
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>

#include <csrss/csrss.h>
#include "api.h"
#include <ntdll/rtl.h>
#include <ddk/ntddblue.h>

/* GLOBALS *******************************************************************/

static HANDLE ConsoleDeviceHandle;
static HANDLE KeyboardDeviceHandle;
static PCSRSS_CONSOLE ActiveConsole;
CRITICAL_SECTION ActiveConsoleLock;
static COORD PhysicalConsoleSize;

/* FUNCTIONS *****************************************************************/

NTSTATUS CsrAllocConsole(PCSRSS_PROCESS_DATA ProcessData,
			 PCSRSS_API_REQUEST LpcMessage,
			 PCSRSS_API_REPLY LpcReply)
{
   PCSRSS_CONSOLE Console;
   HANDLE Process;
   NTSTATUS Status;
   CLIENT_ID ClientId;

   LpcReply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   LpcReply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
     sizeof(LPC_MESSAGE_HEADER);
   if( ProcessData->Console )
      {
	 LpcReply->Status = STATUS_INVALID_PARAMETER;
	 return STATUS_INVALID_PARAMETER;
      }
   LpcReply->Status = STATUS_SUCCESS;
   Console = RtlAllocateHeap( CsrssApiHeap, 0, sizeof( CSRSS_CONSOLE ) );
   if( Console == 0 )
      {
	LpcReply->Status = STATUS_INSUFFICIENT_RESOURCES;
	return STATUS_INSUFFICIENT_RESOURCES;
      }
   LpcReply->Status = CsrInitConsole( ProcessData, Console );
   if( !NT_SUCCESS( LpcReply->Status ) )
     {
       RtlFreeHeap( CsrssApiHeap, 0, Console );
       return LpcReply->Status;
     }
   ProcessData->Console = Console;
   Console->ReferenceCount++;
   CsrInsertObject( ProcessData, &LpcReply->Data.AllocConsoleReply.ConsoleHandle, (Object_t *)Console );
   ClientId.UniqueProcess = (HANDLE)ProcessData->ProcessId;
   Status = NtOpenProcess( &Process, PROCESS_DUP_HANDLE, 0, &ClientId );
   if( !NT_SUCCESS( Status ) )
     {
       DbgPrint( "CSR: NtOpenProcess() failed for handle duplication\n" );
       Console->ReferenceCount--;
       ProcessData->Console = 0;
       CsrReleaseObject( ProcessData, LpcReply->Data.AllocConsoleReply.ConsoleHandle );
       LpcReply->Status = Status;
       return Status;
     }
   Status = NtDuplicateObject( NtCurrentProcess(), &ProcessData->Console->ActiveEvent, Process, &ProcessData->ConsoleEvent, SYNCHRONIZE, FALSE, 0 );
   if( !NT_SUCCESS( Status ) )
     {
       DbgPrint( "CSR: NtDuplicateObject() failed\n" );
       NtClose( Process );
       Console->ReferenceCount--;
       CsrReleaseObject( ProcessData, LpcReply->Data.AllocConsoleReply.ConsoleHandle );
       ProcessData->Console = 0;
       LpcReply->Status = Status;
       return Status;
     }
   NtClose( Process );
   return STATUS_SUCCESS;
}

NTSTATUS CsrFreeConsole(PCSRSS_PROCESS_DATA ProcessData,
			PCSRSS_API_REQUEST LpcMessage,
			PCSRSS_API_REPLY LpcReply)
{
   LpcReply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   LpcReply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
     sizeof(LPC_MESSAGE_HEADER);

   LpcReply->Status = STATUS_NOT_IMPLEMENTED;
   
   return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS CsrReadConsole(PCSRSS_PROCESS_DATA ProcessData,
			PCSRSS_API_REQUEST LpcMessage,
			PCSRSS_API_REPLY LpcReply)
{
   PINPUT_RECORD Input;
   PCHAR Buffer;
   int   i;
   ULONG nNumberOfCharsToRead;
   PCSRSS_CONSOLE Console;
   NTSTATUS Status;
   
   nNumberOfCharsToRead = 
     LpcMessage->Data.ReadConsoleRequest.NrCharactersToRead;
   
//   DbgPrint("CSR: NrCharactersToRead %d\n", nNumberOfCharsToRead);
   
   LpcReply->Header.MessageSize = sizeof(CSRSS_API_REPLY) + 
     nNumberOfCharsToRead;
   LpcReply->Header.DataSize = LpcReply->Header.MessageSize -
     sizeof(LPC_MESSAGE_HEADER);
   Buffer = LpcReply->Data.ReadConsoleReply.Buffer;
   LpcReply->Data.ReadConsoleReply.EventHandle = ProcessData->ConsoleEvent;
   
   Status = CsrGetObject( ProcessData, LpcMessage->Data.ReadConsoleRequest.ConsoleHandle, (Object_t **)&Console );
   if( !NT_SUCCESS( Status ) )
      {
	 LpcReply->Status = Status;
	 return Status;
      }
   for (i=0; i<nNumberOfCharsToRead && Console->InputEvents.Flink != &Console->InputEvents; i++ )     
     {
	Input = &((ConsoleInput *)Console->InputEvents.Flink)->InputEvent;
	Console->InputEvents.Flink = Console->InputEvents.Flink->Flink;
	Console->InputEvents.Flink->Blink = &Console->InputEvents;
	Buffer[i] = Input->Event.KeyEvent.uChar.AsciiChar;
	RtlFreeHeap( CsrssApiHeap, 0, Input );
     }
   CsrUnlockObject( (Object_t *)Console );
   LpcReply->Data.ReadConsoleReply.NrCharactersRead = i;
   LpcReply->Status = i ? STATUS_SUCCESS : STATUS_PENDING;
   return(Status);
}



NTSTATUS CsrWriteConsole(PCSRSS_PROCESS_DATA ProcessData,
			 PCSRSS_API_REQUEST Message,
			 PCSRSS_API_REPLY LpcReply)
{
   BYTE *Buffer = Message->Data.WriteConsoleRequest.Buffer;
   PCSRSS_CONSOLE Console;
   int i;
   
   LpcReply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   LpcReply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
     sizeof(LPC_MESSAGE_HEADER);

   if( !NT_SUCCESS( CsrGetObject( ProcessData, Message->Data.WriteConsoleRequest.ConsoleHandle, (Object_t **)&Console ) ) )
     return LpcReply->Status = STATUS_INVALID_HANDLE;
   for( i = 0; i < Message->Data.WriteConsoleRequest.NrCharactersToWrite; i++ )
      {
	 switch( Buffer[ i ] )
	    {
	    case L'\n': {
	       int c;
	       Console->CurrentX = 0;
	       /* slide the viewable screen */
	       if( ((PhysicalConsoleSize.Y + Console->ShowY) % Console->MaxY) == (Console->CurrentY + 1) )
		 if( ++Console->ShowY == (Console->MaxY - 1) )
		   Console->ShowY = 0;
	       if( ++Console->CurrentY == Console->MaxY )
		  {
		     Console->CurrentY = 0;
		     for( c = 0; c < Console->MaxX; c++ )
			{
			   /* clear new line */
			   Console->Buffer[ c ].Char.UnicodeChar = L' ';
			   Console->Buffer[ c ].Attributes = Console->DefaultAttrib;
			}
		  }
	       else for( c = 0; c < Console->MaxX; c++ )
		  {
		     /* clear new line */
		     Console->Buffer[ (Console->CurrentY * Console->MaxX) + c ].Char.UnicodeChar = L' ';
		     Console->Buffer[ (Console->CurrentY * Console->MaxX) + c ].Attributes = Console->DefaultAttrib;
		  }
	       break;
	    }
	    case L'\b': {
	      if( Console->CurrentX == 0 )
		{
		  /* slide viewable screen up */
		  if( Console->ShowY == Console->CurrentY )
		  {
		    if( Console->ShowY == 0 )
		      Console->ShowY = Console->MaxY;
		    else Console->ShowY--;
		  }
		  /* slide virtual position up */
		  Console->CurrentX = Console->MaxX;
		  if( Console->CurrentY == 0 )
		    Console->CurrentY = Console->MaxY;
		  else Console->CurrentY--;
		}
	      else Console->CurrentX--;
	      Console->Buffer[ (Console->CurrentY * Console->MaxX) + Console->CurrentX ].Char.UnicodeChar = L' ';
	      Console->Buffer[ (Console->CurrentY * Console->MaxX) + Console->CurrentX ].Attributes = Console->DefaultAttrib;
	      break;
	    }
	    default: {
	       int c;
	       Console->Buffer[ (Console->CurrentY * Console->MaxX) + Console->CurrentX ].Char.UnicodeChar = (WCHAR)Buffer[ i ];
	       Console->Buffer[ (Console->CurrentY * Console->MaxX) + Console->CurrentX ].Attributes = Console->DefaultAttrib;
	       Console->CurrentX++;
	       if( Console->CurrentX == Console->MaxX )
		  {
		     /* if end of line, go to next */
		     Console->CurrentX = 0;
		     if( ++Console->CurrentY == Console->MaxY )
			{
			   /* if end of buffer, wrap back to beginning */
			   Console->CurrentY = 0;
			   /* clear new line */
			   for( c = 0; c < Console->MaxX; c++ )
			      {
				 Console->Buffer[ (Console->CurrentY * Console->MaxX) + c ].Char.UnicodeChar = L' ';
				 Console->Buffer[ (Console->CurrentY * Console->MaxX) + c ].Attributes = Console->DefaultAttrib;
			      }
			}
		     else {
			/* clear new line */
			for( c = 0; c < Console->MaxX; c += 2 )
			   {
			      Console->Buffer[ (Console->CurrentY * Console->MaxX) + c ].Char.UnicodeChar = L' ';
			      Console->Buffer[ (Console->CurrentY * Console->MaxX) + c ].Attributes = Console->DefaultAttrib;
			   }
		     }
		     /* slide the viewable screen */
		     if( (Console->CurrentY - Console->ShowY) == PhysicalConsoleSize.Y )
		       if( ++Console->ShowY == Console->MaxY )
			 Console->ShowY = 0;
		  }
	    }
	    }
      }
   CsrUnlockObject( (Object_t *)Console );
   RtlEnterCriticalSection( &ActiveConsoleLock );
   if( Console == ActiveConsole )
     {   /* only write to screen if Console is Active, and not scrolled up */
	 CsrDrawConsole ( Console );
     }
   RtlLeaveCriticalSection( &ActiveConsoleLock );
   LpcReply->Data.WriteConsoleReply.NrCharactersWritten = i;
   LpcReply->Status = STATUS_SUCCESS;
   return(STATUS_SUCCESS);
}

NTSTATUS CsrInitConsole(PCSRSS_PROCESS_DATA ProcessData,
		    PCSRSS_CONSOLE Console)
{
  NTSTATUS Status;
  
  Console->MaxX = PhysicalConsoleSize.X;
  Console->MaxY = PhysicalConsoleSize.Y * 2;
  Console->ShowX = 0;
  Console->ShowY = 0;
  Console->CurrentX = 0;
  Console->CurrentY = 0;
  Console->ReferenceCount = 0;
  Console->Type = CSRSS_CONSOLE_MAGIC;
  RtlInitializeCriticalSection( &Console->Lock );
  Console->Buffer = RtlAllocateHeap( CsrssApiHeap, 0, Console->MaxX * Console->MaxY * sizeof(CHAR_INFO) );
  if( Console->Buffer == 0 )
    return STATUS_INSUFFICIENT_RESOURCES;
  Console->InputEvents.Flink = Console->InputEvents.Blink = &Console->InputEvents;
  Status = NtCreateEvent( &Console->ActiveEvent, STANDARD_RIGHTS_ALL, 0, FALSE, FALSE );
  if( !NT_SUCCESS( Status ) )
    {
      RtlFreeHeap( CsrssApiHeap, 0, Console->Buffer );
      return Status;
    }
  Console->DefaultAttrib = 0x17;
  /* initialize buffer to be empty with default attributes */
  for( ; Console->CurrentY < Console->MaxY; Console->CurrentY++ )
    {
      for( ; Console->CurrentX < Console->MaxX; Console->CurrentX++ )
	{
	  Console->Buffer[ Console->CurrentX + (Console->CurrentY * Console->MaxX) ].Char.UnicodeChar = L' ';
	  Console->Buffer[ Console->CurrentX + (Console->CurrentY * Console->MaxX) ].Attributes = Console->DefaultAttrib;
	}
      Console->CurrentX = 0;
    }
  /* make console active, and insert into console list */
  RtlEnterCriticalSection( &ActiveConsoleLock );
  if( ActiveConsole )
     {
	Console->Prev = ActiveConsole;
	Console->Next = ActiveConsole->Next;
	ActiveConsole->Next->Prev = Console;
	ActiveConsole->Next = Console;
     }
  else {
     Console->Prev = Console;
     Console->Next = Console;
  }
  Console->CurrentX = 0;
  Console->CurrentY = 0;
  ActiveConsole = Console;
  /* copy buffer contents to screen */
  CsrDrawConsole( Console );
  RtlLeaveCriticalSection( &ActiveConsoleLock );
  return STATUS_SUCCESS;
}

/***************************************************************
 *  CsrDrawConsole blasts the console buffer onto the screen   *
 *  must be called while holding the active console lock       *
 **************************************************************/
VOID CsrDrawConsole( PCSRSS_CONSOLE Console )
{
   NTSTATUS Status;
   IO_STATUS_BLOCK Iosb;
   CONSOLE_DRAW DrawInfo;
   ULONG BufferSize;
   CONSOLE_SCREEN_BUFFER_INFO ScrInfo;

   DrawInfo.X = Console->ShowX;
   DrawInfo.Y = Console->ShowY;
   DrawInfo.SizeX = Console->MaxX;
   DrawInfo.SizeY = Console->MaxY;

   BufferSize = Console->MaxX * Console->MaxY * sizeof(CHAR_INFO);

   Status = NtDeviceIoControlFile( ConsoleDeviceHandle, NULL, NULL, NULL, &Iosb, IOCTL_CONSOLE_DRAW, &DrawInfo, sizeof( CONSOLE_DRAW ), Console->Buffer, BufferSize);
   if( !NT_SUCCESS( Status ) )
     {
       DbgPrint( "CSR: Failed to set console info\n" );
       return;
     }

   ScrInfo.dwCursorPosition.X = Console->CurrentX - Console->ShowX;
   ScrInfo.dwCursorPosition.Y = ((Console->CurrentY + Console->MaxY) - Console->ShowY) % Console->MaxY;
   ScrInfo.wAttributes = Console->DefaultAttrib;
   Status = NtDeviceIoControlFile( ConsoleDeviceHandle, NULL, NULL, NULL, &Iosb, IOCTL_CONSOLE_SET_SCREEN_BUFFER_INFO, &ScrInfo, sizeof( ScrInfo ), 0, 0 );
   if( !NT_SUCCESS( Status ) )
     {
       DbgPrint( "CSR: Failed to set console info\n" );
       return;
     }
}


VOID CsrDeleteConsole( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_CONSOLE Console )
{
   ConsoleInput *Event;
   RtlFreeHeap( CsrssApiHeap, 0, Console->Buffer );
   while( Console->InputEvents.Flink != &Console->InputEvents )
      {
	 Event = (ConsoleInput *)Console->InputEvents.Flink;
	 Console->InputEvents.Flink = Console->InputEvents.Flink->Flink;
	 Console->InputEvents.Flink->Flink->Blink = &Console->InputEvents;
	 RtlFreeHeap( CsrssApiHeap, 0, Event );
      }
   RtlEnterCriticalSection( &ActiveConsoleLock );
   if( ActiveConsole == Console )
      {
	 if( Console->Next != Console )
	    {
	       ActiveConsole = Console->Next;
	       Console->Prev->Next = Console->Next;
	       Console->Next->Prev = Console->Prev;
	    }
	 else ActiveConsole = 0;
      }
   if( ActiveConsole )
     CsrDrawConsole( ActiveConsole );
   RtlLeaveCriticalSection( &ActiveConsoleLock );
   NtClose( Console->ActiveEvent );
   RtlDeleteCriticalSection( &Console->Lock );
}

VOID CsrInitConsoleSupport(VOID)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING DeviceName;
   NTSTATUS Status;
   IO_STATUS_BLOCK Iosb;
   CONSOLE_SCREEN_BUFFER_INFO ScrInfo;
   
   DbgPrint("CSR: CsrInitConsoleSupport()\n");
   
   RtlInitUnicodeString(&DeviceName, L"\\??\\BlueScreen");
   InitializeObjectAttributes(&ObjectAttributes,
			      &DeviceName,
			      0,
			      NULL,
			      NULL);
   Status = NtOpenFile(&ConsoleDeviceHandle,
		       FILE_ALL_ACCESS,
		       &ObjectAttributes,
		       &Iosb,
		       0,
		       FILE_SYNCHRONOUS_IO_ALERT);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("CSR: Failed to open console. Expect problems.\n");
     }
//   DbgPrint("CSR: ConsoleDeviceHandle %x\n", ConsoleDeviceHandle);
   
   RtlInitUnicodeString(&DeviceName, L"\\??\\Keyboard");
   InitializeObjectAttributes(&ObjectAttributes,
			      &DeviceName,
			      0,
			      NULL,
			      NULL);
   Status = NtOpenFile(&KeyboardDeviceHandle,
		       FILE_ALL_ACCESS,
		       &ObjectAttributes,
		       &Iosb,
		       0,
		       FILE_SYNCHRONOUS_IO_ALERT);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("CSR: Failed to open keyboard. Expect problems.\n");
     }
   
   ActiveConsole = 0;
   RtlInitializeCriticalSection( &ActiveConsoleLock );
   Status = NtDeviceIoControlFile( ConsoleDeviceHandle, NULL, NULL, NULL, &Iosb, IOCTL_CONSOLE_GET_SCREEN_BUFFER_INFO, 0, 0, &ScrInfo, sizeof( ScrInfo ) );
   if( !NT_SUCCESS( Status ) )
     {
       DbgPrint( "CSR: Failed to get console info, expect trouble\n" );
       return;
     }
   PhysicalConsoleSize = ScrInfo.dwSize;
}

VOID Console_Api( DWORD Ignored )
{
  /* keep reading events from the keyboard and stuffing them into the current
     console's input queue */
  ConsoleInput *KeyEventRecord;
  IO_STATUS_BLOCK Iosb;
  NTSTATUS Status;
  while( 1 )
    {
      KeyEventRecord = RtlAllocateHeap( CsrssApiHeap, 0, sizeof( ConsoleInput ) );
      if( KeyEventRecord == 0 )
	{
	  DbgPrint( "CSR: Memory allocation failure!" );
	  continue;
	}
      KeyEventRecord->InputEvent.EventType = KEY_EVENT;
      Status = NtReadFile( KeyboardDeviceHandle, NULL, NULL, NULL, &Iosb, &KeyEventRecord->InputEvent.Event.KeyEvent, sizeof( KEY_EVENT_RECORD ), NULL, 0 );
      if( !NT_SUCCESS( Status ) )
	{
	  DbgPrint( "CSR: ReadFile on keyboard device failed\n" );
	  continue;
	}
      //      DbgPrint( "Char: %c\n", KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar );
      if( KeyEventRecord->InputEvent.Event.KeyEvent.dwControlKeyState & ( RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED ) && KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown == TRUE )
	{
	  if( KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar == 'q' )
	    {
	      /* alt-tab, swap consoles */
	      RtlEnterCriticalSection( &ActiveConsoleLock );
	      if( ActiveConsole->Next != ActiveConsole )
		ActiveConsole = ActiveConsole->Next;
	      CsrDrawConsole( ActiveConsole );
	      RtlLeaveCriticalSection( &ActiveConsoleLock );
	      continue;
	    }
	  else if( KeyEventRecord->InputEvent.Event.KeyEvent.wVirtualKeyCode == VK_UP || KeyEventRecord->InputEvent.Event.KeyEvent.wVirtualKeyCode == VK_DOWN )
	    {
	      /* scroll up or down */
	      RtlEnterCriticalSection( &ActiveConsoleLock );
	      if( ActiveConsole == 0 )
		{
		  DbgPrint( "CSR: No Active Console!\n" );
		  continue;
		}
	      RtlEnterCriticalSection( &ActiveConsole->Lock );
	      if( KeyEventRecord->InputEvent.Event.KeyEvent.wVirtualKeyCode == VK_UP )
		{
		  /* only scroll up if there is room to scroll up into */
		  if( ActiveConsole->ShowY != ((ActiveConsole->CurrentY + 1) % ActiveConsole->MaxY) )
		    ActiveConsole->ShowY = (ActiveConsole->ShowY + ActiveConsole->MaxY - 1) % ActiveConsole->MaxY;
		}
	      else if( ActiveConsole->ShowY != ActiveConsole->CurrentY )
		/* only scroll down if there is room to scroll down into */
		if( ActiveConsole->ShowY % ActiveConsole->MaxY != ActiveConsole->CurrentY )
		  if( ((ActiveConsole->CurrentY + 1) % ActiveConsole->MaxY) != (ActiveConsole->ShowY + PhysicalConsoleSize.Y) % ActiveConsole->MaxY )
		    ActiveConsole->ShowY = (ActiveConsole->ShowY + 1) % ActiveConsole->MaxY;
	      CsrDrawConsole( ActiveConsole );
	      RtlLeaveCriticalSection( &ActiveConsole->Lock );
	      RtlLeaveCriticalSection( &ActiveConsoleLock );
	    }
	}
      if( KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown == TRUE )
	 continue;
      /* ignore dead keys */
      if( KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar == 0 )
	 continue;
      RtlEnterCriticalSection( &ActiveConsoleLock );
      if( ActiveConsole == 0 )
	{
	  DbgPrint( "CSR: No Active Console!\n" );
	  continue;
	}
      RtlEnterCriticalSection( &ActiveConsole->Lock );
      KeyEventRecord->ListEntry.Flink = &ActiveConsole->InputEvents;
      KeyEventRecord->ListEntry.Blink = ActiveConsole->InputEvents.Blink;
      ActiveConsole->InputEvents.Blink->Flink = &KeyEventRecord->ListEntry;
      ActiveConsole->InputEvents.Blink = &KeyEventRecord->ListEntry;
      NtSetEvent( ActiveConsole->ActiveEvent, 0 );
    }
  RtlLeaveCriticalSection( &ActiveConsoleLock );
}


/* EOF */

