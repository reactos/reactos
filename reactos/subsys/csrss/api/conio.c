/* $Id: conio.c,v 1.11 2000/08/05 18:01:58 dwelch Exp $
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
#include <debug.h>

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
       DbgPrint( "CSR: NtDuplicateObject() failed: %x\n", Status );
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
   ConsoleInput *Input;
   PCHAR Buffer;
   int   i = 0;
   ULONG nNumberOfCharsToRead;
   PCSRSS_CONSOLE Console;
   NTSTATUS Status;
   
   nNumberOfCharsToRead = LpcMessage->Data.ReadConsoleRequest.NrCharactersToRead > CSRSS_MAX_READ_CONSOLE_REQUEST ? CSRSS_MAX_READ_CONSOLE_REQUEST : LpcMessage->Data.ReadConsoleRequest.NrCharactersToRead;
   
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
   RtlEnterCriticalSection( &ActiveConsoleLock );
   if( !(Console->ConsoleMode & ENABLE_LINE_INPUT) || Console->WaitingLines )
      for (; i<nNumberOfCharsToRead && Console->InputEvents.Flink != &Console->InputEvents; i++ )     
	 {
	    // remove input event from queue
	    Input = (ConsoleInput *)Console->InputEvents.Flink;
	    Input->ListEntry.Blink->Flink = Input->ListEntry.Flink;
	    Input->ListEntry.Flink->Blink = Input->ListEntry.Blink;
	    // only pay attention to valid ascii chars, on key down
	    if( Input->InputEvent.Event.KeyEvent.bKeyDown == TRUE &&
		Input->InputEvent.Event.KeyEvent.uChar.AsciiChar )
	       {
		  Buffer[i] = Input->InputEvent.Event.KeyEvent.uChar.AsciiChar;
		  // process newline on line buffered mode
		  if( Console->ConsoleMode & ENABLE_LINE_INPUT &&
		      Input->InputEvent.Event.KeyEvent.uChar.AsciiChar == '\n' )
		     {
			Console->WaitingChars--;
			Console->WaitingLines--;
			RtlFreeHeap( CsrssApiHeap, 0, Input );
			i++;
			break;
		     }
	       }
	    else i--;
	    Console->WaitingChars--;
	    RtlFreeHeap( CsrssApiHeap, 0, Input );
	 }
   Buffer[i] = 0;
   RtlLeaveCriticalSection( &ActiveConsoleLock );
   LpcReply->Data.ReadConsoleReply.NrCharactersRead = i;
   LpcReply->Status = i ? STATUS_SUCCESS : STATUS_PENDING;
   return(Status);
}



NTSTATUS CsrpWriteConsole( PCSRSS_CONSOLE Console, CHAR *Buffer, DWORD Length, BOOL Attrib )
{
   IO_STATUS_BLOCK Iosb;
   NTSTATUS Status;
   int i;

   for( i = 0; i < Length; i++ )
      {
	 switch( Buffer[ i ] )
	    {
	    case '\n': {
	       int c;
	       Console->CurrentX = 0;
	       /* slide the viewable screen */
	       if( ((PhysicalConsoleSize.Y + Console->ShowY) % Console->MaxY) == (Console->CurrentY + 1) % Console->MaxY)
		 if( ++Console->ShowY == (Console->MaxY - 1) )
		   Console->ShowY = 0;
	       if( ++Console->CurrentY == Console->MaxY )
		  {
		     Console->CurrentY = 0;
		     for( c = 0; c < Console->MaxX; c++ )
			{
			   /* clear new line */
			   Console->Buffer[ c * 2 ] = ' ';
			   Console->Buffer[ (c * 2) + 1 ] = Console->DefaultAttrib;
			}
		  }
	       else for( c = 0; c < Console->MaxX; c++ )
		  {
		     /* clear new line */
		     Console->Buffer[ 2 * ((Console->CurrentY * Console->MaxX) + c) ] = ' ';
		     Console->Buffer[ (2 * ((Console->CurrentY * Console->MaxX) + c)) + 1 ] = Console->DefaultAttrib;
		  }
	       break;
	    }
	    case '\b': {
	      if( Console->CurrentX == 0 )
		{
		  /* slide viewable screen up */
		  if( Console->ShowY == Console->CurrentY )
		    if( Console->ShowY == 0 )
		      Console->ShowY = Console->MaxY;
		    else Console->ShowY--;
		  /* slide virtual position up */
		  Console->CurrentX = Console->MaxX;
		  if( Console->CurrentY == 0 )
		    Console->CurrentY = Console->MaxY;
		  else Console->CurrentY--;
		}
	      else Console->CurrentX--;
	      Console->Buffer[ 2 * ((Console->CurrentY * Console->MaxX) + Console->CurrentX) ] = ' ';
	      Console->Buffer[ (2 * ((Console->CurrentY * Console->MaxX) + Console->CurrentX)) + 1 ] = Console->DefaultAttrib;
	      break;
	    }
	    default: {
	       int c;
	       Console->Buffer[ 2 * (((Console->CurrentY * Console->MaxX)) + Console->CurrentX) ] = Buffer[ i ];
	       if( Attrib )
		  Console->Buffer[ (2 * ((Console->CurrentY * Console->MaxX) + Console->CurrentX)) + 1 ] = Console->DefaultAttrib;
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
				 Console->Buffer[ 2 * ((Console->CurrentY * Console->MaxX) + c) ] = ' ';
				 Console->Buffer[ (2 * ((Console->CurrentY * Console->MaxX) + c)) + 1 ] = Console->DefaultAttrib;
			      }
			}
		     else {
			/* clear new line */
			for( c = 0; c < Console->MaxX; c += 2 )
			   {
			      Console->Buffer[ 2 * ((Console->CurrentY * Console->MaxX) + c) ] = ' ';
			      Console->Buffer[ (2 * ((Console->CurrentY * Console->MaxX) + c)) + 1 ] = Console->DefaultAttrib;
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
   if( Console == ActiveConsole )
     {    /* only write to screen if Console is Active, and not scrolled up */
	if( Attrib )
	   {
	      Status = NtWriteFile( ConsoleDeviceHandle, NULL, NULL, NULL, &Iosb, Buffer, Length, NULL, 0);
	      if (!NT_SUCCESS(Status))
		 DbgPrint("CSR: Write failed\n");
	   }
	else CsrDrawConsole( Console );
      }
   return(STATUS_SUCCESS);
}

NTSTATUS CsrWriteConsole(PCSRSS_PROCESS_DATA ProcessData,
			 PCSRSS_API_REQUEST LpcMessage,
			 PCSRSS_API_REPLY Reply)
{
   BYTE *Buffer = LpcMessage->Data.WriteConsoleRequest.Buffer;
   PCSRSS_CONSOLE Console;
   
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
     sizeof(LPC_MESSAGE_HEADER);

   if( !NT_SUCCESS( CsrGetObject( ProcessData, LpcMessage->Data.WriteConsoleRequest.ConsoleHandle, (Object_t **)&Console ) ) )
     return Reply->Status = STATUS_INVALID_HANDLE;
   RtlEnterCriticalSection( &ActiveConsoleLock );
   CsrpWriteConsole( Console, Buffer, LpcMessage->Data.WriteConsoleRequest.NrCharactersToWrite, TRUE );
   RtlLeaveCriticalSection( &ActiveConsoleLock );
   return Reply->Status = STATUS_SUCCESS;
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
  Console->WaitingChars = 0;
  Console->WaitingLines = 0;
  Console->Type = CSRSS_CONSOLE_MAGIC;
  Console->ConsoleMode = ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT;
  Console->Buffer = RtlAllocateHeap( CsrssApiHeap, 0, Console->MaxX * Console->MaxY * 2 );
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
	  Console->Buffer[ (Console->CurrentX * 2) + (Console->CurrentY * Console->MaxX * 2) ] = ' ';
	  Console->Buffer[ (Console->CurrentX * 2) + (Console->CurrentY * Console->MaxX * 2)+ 1 ] = Console->DefaultAttrib;
	}
      Console->CurrentX = 0;
    }
  Console->CursorInfo.bVisible = TRUE;
  Console->CursorInfo.dwSize = 5;
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
   IO_STATUS_BLOCK Iosb;
   NTSTATUS Status;
   CONSOLE_SCREEN_BUFFER_INFO ScrInfo;
   CONSOLE_MODE Mode;
   int i, y;

   /* first set position to 0,0 */
   ScrInfo.dwCursorPosition.X = 0;
   ScrInfo.dwCursorPosition.Y = 0;
   ScrInfo.wAttributes = Console->DefaultAttrib;
   Status = NtDeviceIoControlFile( ConsoleDeviceHandle, NULL, NULL, NULL, &Iosb, IOCTL_CONSOLE_SET_SCREEN_BUFFER_INFO, &ScrInfo, sizeof( ScrInfo ), 0, 0 );
   if( !NT_SUCCESS( Status ) )
     {
       DbgPrint( "CSR: Failed to set console info\n" );
       return;
     }
   Mode.dwMode = 0; /* clear ENABLE_PROCESSED_OUTPUT mode */
   Status = NtDeviceIoControlFile( ConsoleDeviceHandle, NULL, NULL, NULL, &Iosb, IOCTL_CONSOLE_SET_MODE, &Mode, sizeof( Mode ), 0, 0 );
   if( !NT_SUCCESS( Status ) )
     {
       DbgPrint( "CSR: Failed to set console mode\n" );
       return;
     }
   /* blast out buffer */
   for( i = 0, y = Console->ShowY; i < PhysicalConsoleSize.Y; i++ )
     {
       Status = NtWriteFile( ConsoleDeviceHandle, NULL, NULL, NULL, &Iosb, &Console->Buffer[ (Console->ShowX * 2) + (y * Console->MaxX * 2) ], PhysicalConsoleSize.X * 2, 0, 0 );
       if( !NT_SUCCESS( Status ) )
	 {
	   DbgPrint( "CSR: Write to console failed\n" );
	   return;
	 }
       /* wrap back around the end of the buffer */
       if( ++y == Console->MaxY )
	 y = 0;
     }
   Mode.dwMode = ENABLE_PROCESSED_OUTPUT;
   Status = NtDeviceIoControlFile( ConsoleDeviceHandle, NULL, NULL, NULL, &Iosb, IOCTL_CONSOLE_SET_MODE, &Mode, sizeof( Mode ), 0, 0 );
   if( !NT_SUCCESS( Status ) )
     {
       DbgPrint( "CSR: Failed to set console mode\n" );
       return;
     }
   ScrInfo.dwCursorPosition.X = Console->CurrentX - Console->ShowX;
   ScrInfo.dwCursorPosition.Y = ((Console->CurrentY + Console->MaxY) - Console->ShowY) % Console->MaxY;
   Status = NtDeviceIoControlFile( ConsoleDeviceHandle, NULL, NULL, NULL, &Iosb, IOCTL_CONSOLE_SET_SCREEN_BUFFER_INFO, &ScrInfo, sizeof( ScrInfo ), 0, 0 );
   if( !NT_SUCCESS( Status ) )
     {
       DbgPrint( "CSR: Failed to set console info\n" );
       return;
     }
   Status = NtDeviceIoControlFile( ConsoleDeviceHandle, NULL, NULL, NULL, &Iosb, IOCTL_CONSOLE_SET_CURSOR_INFO, &Console->CursorInfo, sizeof( Console->CursorInfo ), 0, 0 );
   if( !NT_SUCCESS( Status ) )
     {
       DbgPrint( "CSR: Failed to set cursor info\n" );
       return;
     }
}

       
VOID CsrDeleteConsole( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_CONSOLE Console )
{
   ConsoleInput *Event;
   RtlEnterCriticalSection( &ActiveConsoleLock );
   RtlFreeHeap( CsrssApiHeap, 0, Console->Buffer );
   while( Console->InputEvents.Flink != &Console->InputEvents )
      {
	 Event = (ConsoleInput *)Console->InputEvents.Flink;
	 Console->InputEvents.Flink = Console->InputEvents.Flink->Flink;
	 Console->InputEvents.Flink->Flink->Blink = &Console->InputEvents;
	 RtlFreeHeap( CsrssApiHeap, 0, Event );
      }
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
		       0);
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

VOID Console_Api( DWORD RefreshEvent )
{
  /* keep reading events from the keyboard and stuffing them into the current
     console's input queue */
  ConsoleInput *KeyEventRecord;
  IO_STATUS_BLOCK Iosb;
  NTSTATUS Status;
  HANDLE Events[2];     // 0 = keyboard, 1 = refresh
  int c;

  Events[0] = 0;
  Status = NtCreateEvent( &Events[0], STANDARD_RIGHTS_ALL, NULL, FALSE, FALSE );
  if( !NT_SUCCESS( Status ) )
    {
      DbgPrint( "CSR: NtCreateEvent failed: %x\n", Status );
      return;
    }
  Events[1] = (HANDLE)RefreshEvent;
  while( 1 )
    {
      KeyEventRecord = RtlAllocateHeap(CsrssApiHeap, 
				       0,
				       sizeof(ConsoleInput));
       if ( KeyEventRecord == 0 )
	{
	  DbgPrint( "CSR: Memory allocation failure!" );
	  continue;
	}
      KeyEventRecord->InputEvent.EventType = KEY_EVENT;
      Status = NtReadFile( KeyboardDeviceHandle, Events[0], NULL, NULL, &Iosb, &KeyEventRecord->InputEvent.Event.KeyEvent, sizeof( KEY_EVENT_RECORD ), NULL, 0 );
      if( !NT_SUCCESS( Status ) )
	{
	  DbgPrint( "CSR: ReadFile on keyboard device failed\n" );
	  RtlFreeHeap( CsrssApiHeap, 0, KeyEventRecord );
	  continue;
	}
      if( Status == STATUS_PENDING )
	{
	  while( 1 )
	    {
	      Status = NtWaitForMultipleObjects( 2, Events, WaitAny, FALSE, NULL );
	      if( Status == STATUS_WAIT_0 + 1 )
		{
		  RtlEnterCriticalSection( &ActiveConsoleLock );
		  CsrDrawConsole( ActiveConsole );
		  RtlLeaveCriticalSection( &ActiveConsoleLock );
		  continue;
		}
	      else if( Status != STATUS_WAIT_0 )
		{
		  DbgPrint( "CSR: NtWaitForMultipleObjects failed: %x, exiting\n", Status );
		  return;
		}
	      else break;
	    }
	}
      //      DbgPrint( "Char: %c\n", KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar );
      if( KeyEventRecord->InputEvent.Event.KeyEvent.dwControlKeyState & ( RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED )&&  KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar == 'q' )
	 if( KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown == TRUE )
	    {
	       /* alt-tab, swap consoles */
	       RtlEnterCriticalSection( &ActiveConsoleLock );
	       if( ActiveConsole->Next != ActiveConsole )
		  ActiveConsole = ActiveConsole->Next;
	       CsrDrawConsole( ActiveConsole );
	       RtlLeaveCriticalSection( &ActiveConsoleLock );
	       RtlFreeHeap( CsrssApiHeap, 0, KeyEventRecord );
	       continue;
	    }
	 else {
	    RtlFreeHeap( CsrssApiHeap, 0, KeyEventRecord );
	    continue;
	 }
      else if( KeyEventRecord->InputEvent.Event.KeyEvent.dwControlKeyState & ( RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED ) && (KeyEventRecord->InputEvent.Event.KeyEvent.wVirtualKeyCode == VK_UP || KeyEventRecord->InputEvent.Event.KeyEvent.wVirtualKeyCode == VK_DOWN) )
	 {
	    if( KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown == TRUE )
	       {
		  /* scroll up or down */
		  RtlEnterCriticalSection( &ActiveConsoleLock );
		  if( ActiveConsole == 0 )
		     {
			DbgPrint( "CSR: No Active Console!\n" );
			RtlFreeHeap( CsrssApiHeap, 0, KeyEventRecord );
			continue;
		     }
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
		  RtlLeaveCriticalSection( &ActiveConsoleLock );
	       }
	    RtlFreeHeap( CsrssApiHeap, 0, KeyEventRecord );
	    continue;
	}
      RtlEnterCriticalSection( &ActiveConsoleLock );
      if( ActiveConsole == 0 )
	 {
	    DbgPrint( "CSR: No Active Console!\n" );
	    RtlFreeHeap( CsrssApiHeap, 0, KeyEventRecord );
	    continue;
	 }
      // echo to screen if enabled, but do not echo '\b' if there are no keys in buffer to delete
      if( ActiveConsole->ConsoleMode & ENABLE_ECHO_INPUT &&
	  KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown == TRUE &&
	  KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar &&
	  ( KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar != '\b' ||
	    ActiveConsole->InputEvents.Flink != &ActiveConsole->InputEvents ) )
	{
	  CsrpWriteConsole( ActiveConsole, &KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar, 1, TRUE );
	}
      // process special keys if enabled
      if( ActiveConsole->ConsoleMode & ENABLE_PROCESSED_INPUT )
	  switch( KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar )
	    {
	    case '\b':
	      if( KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown == TRUE )
		for( c = 0; c < 2; c++ )
		  {
		    ConsoleInput *Input = ActiveConsole->InputEvents.Blink;

		    CHECKPOINT1;
		    ActiveConsole->InputEvents.Blink->Blink->Flink = &ActiveConsole->InputEvents;
		    ActiveConsole->InputEvents.Blink = ActiveConsole->InputEvents.Blink->Blink;
		    RtlFreeHeap( CsrssApiHeap, 0, Input );
		  }
	      RtlFreeHeap( CsrssApiHeap, 0, KeyEventRecord );
	      RtlLeaveCriticalSection( &ActiveConsoleLock );
	      continue;
	    }
      // add event to the queue
      KeyEventRecord->ListEntry.Flink = &ActiveConsole->InputEvents;
      KeyEventRecord->ListEntry.Blink = ActiveConsole->InputEvents.Blink;
      ActiveConsole->InputEvents.Blink->Flink = &KeyEventRecord->ListEntry;
      ActiveConsole->InputEvents.Blink = &KeyEventRecord->ListEntry;
      // if line input mode is enabled, only wake the client on enter key up
      if( !(ActiveConsole->ConsoleMode & ENABLE_LINE_INPUT ) ||
	  ( KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar == '\n' &&
	    KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown == FALSE ) )
	{
	  NtSetEvent( ActiveConsole->ActiveEvent, 0 );
	  ActiveConsole->WaitingLines++;
	}
      ActiveConsole->WaitingChars++;
      RtlLeaveCriticalSection( &ActiveConsoleLock );
    }
}

NTSTATUS CsrGetScreenBufferInfo( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply )
{
   NTSTATUS Status;
   PCSRSS_CONSOLE Console;
   PCONSOLE_SCREEN_BUFFER_INFO pInfo;
   IO_STATUS_BLOCK Iosb;
   
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
     sizeof(LPC_MESSAGE_HEADER);

   if( !NT_SUCCESS( CsrGetObject( ProcessData, Request->Data.ScreenBufferInfoRequest.ConsoleHandle, (Object_t **)&Console ) ) )
     return Reply->Status = STATUS_INVALID_HANDLE;
   pInfo = &Reply->Data.ScreenBufferInfoReply.Info;
   RtlEnterCriticalSection( &ActiveConsoleLock );   
   if( Console == ActiveConsole )
     {
	Status = NtDeviceIoControlFile( ConsoleDeviceHandle, NULL, NULL, NULL, &Iosb, IOCTL_CONSOLE_GET_SCREEN_BUFFER_INFO, 0, 0, pInfo, sizeof( *pInfo ) );
	if( !NT_SUCCESS( Status ) )
	   DbgPrint( "CSR: Failed to get console info, expect trouble\n" );
	Reply->Status = STATUS_SUCCESS;
     }
   else {
      pInfo->dwSize.X = PhysicalConsoleSize.X;
      pInfo->dwSize.Y = PhysicalConsoleSize.Y;
      pInfo->dwCursorPosition.X = Console->CurrentX - Console->ShowX;
      pInfo->dwCursorPosition.Y = (Console->CurrentY + Console->MaxY - Console->ShowY) % Console->MaxY;
      pInfo->wAttributes = Console->DefaultAttrib;
      pInfo->srWindow.Left = 0;
      pInfo->srWindow.Right = PhysicalConsoleSize.X - 1;
      pInfo->srWindow.Top = 0;
      pInfo->srWindow.Bottom = PhysicalConsoleSize.Y - 1;
      Reply->Status = STATUS_SUCCESS;
   }
   RtlLeaveCriticalSection( &ActiveConsoleLock );
   return Reply->Status;
}

NTSTATUS CsrSetCursor( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply )
{
   NTSTATUS Status;
   PCSRSS_CONSOLE Console;
   CONSOLE_SCREEN_BUFFER_INFO Info;
   IO_STATUS_BLOCK Iosb;
   
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
     sizeof(LPC_MESSAGE_HEADER);

   if( !NT_SUCCESS( CsrGetObject( ProcessData, Request->Data.SetCursorRequest.ConsoleHandle, (Object_t **)&Console ) ) )
     return Reply->Status = STATUS_INVALID_HANDLE;
   RtlEnterCriticalSection( &ActiveConsoleLock );
   Info.dwCursorPosition = Request->Data.SetCursorRequest.Position;
   Info.wAttributes = Console->DefaultAttrib;
   if( Console == ActiveConsole )
      {
	 Status = NtDeviceIoControlFile( ConsoleDeviceHandle, NULL, NULL, NULL, &Iosb, IOCTL_CONSOLE_SET_SCREEN_BUFFER_INFO, &Info, sizeof( Info ), 0, 0 );
	if( !NT_SUCCESS( Status ) )
	   DbgPrint( "CSR: Failed to set console info, expect trouble\n" );
      }
   Console->CurrentX = Info.dwCursorPosition.X + Console->ShowX;
   Console->CurrentY = (Info.dwCursorPosition.Y + Console->ShowY) % Console->MaxY;
   RtlLeaveCriticalSection( &ActiveConsoleLock );
   return Reply->Status = Status;
}

NTSTATUS CsrWriteConsoleOutputChar( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply )
{
   BYTE *Buffer = Request->Data.WriteConsoleOutputCharRequest.String;
   PCSRSS_CONSOLE Console;
   DWORD X, Y;
   
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
     sizeof(LPC_MESSAGE_HEADER);

   if( !NT_SUCCESS( CsrGetObject( ProcessData, Request->Data.WriteConsoleOutputCharRequest.ConsoleHandle, (Object_t **)&Console ) ) )
     return Reply->Status = STATUS_INVALID_HANDLE;
   RtlEnterCriticalSection( &ActiveConsoleLock );
   X = Console->CurrentX;
   Y = Console->CurrentY;
   CsrpWriteConsole( Console, Buffer, Request->Data.WriteConsoleOutputCharRequest.Length, TRUE );
   Reply->Data.WriteConsoleOutputCharReply.EndCoord.X = Console->CurrentX - Console->ShowX;
   Reply->Data.WriteConsoleOutputCharReply.EndCoord.Y = (Console->CurrentY + Console->MaxY - Console->ShowY) % Console->MaxY;
   Console->CurrentY = Y;
   Console->CurrentX = X;
   RtlLeaveCriticalSection( &ActiveConsoleLock );
   return Reply->Status = STATUS_SUCCESS;
}

NTSTATUS CsrFillOutputChar( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply )
{
   PCSRSS_CONSOLE Console;
   DWORD X, Y, i;
   
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
     sizeof(LPC_MESSAGE_HEADER);

   if( !NT_SUCCESS( CsrGetObject( ProcessData, Request->Data.FillOutputRequest.ConsoleHandle, (Object_t **)&Console ) ) )
     return Reply->Status = STATUS_INVALID_HANDLE;
   RtlEnterCriticalSection( &ActiveConsoleLock );
   X = Request->Data.FillOutputRequest.Position.X + Console->ShowX;
   Y = Request->Data.FillOutputRequest.Position.Y + Console->ShowY;
   for( i = 0; i < 20000; i++ );
   for( i = 0; i < Request->Data.FillOutputRequest.Length; i++ )
      {
	 Console->Buffer[ (Y * 2 * Console->MaxX) + (X * 2) ] = Request->Data.FillOutputRequest.Char;
	 if( ++X == Console->MaxX )
	    {
	       if( ++Y == Console->MaxY )
		  Y = 0;
	       X = 0;
	    }
      }
   if( Console == ActiveConsole )
      CsrDrawConsole( Console );
   RtlLeaveCriticalSection( &ActiveConsoleLock );
   return Reply->Status;
}

NTSTATUS CsrReadInputEvent( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply )
{
   PCSRSS_CONSOLE Console;
   NTSTATUS Status;
   ConsoleInput *Input;
   
//   DbgPrint("CSR: NrCharactersToRead %d\n", nNumberOfCharsToRead);
   
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
      sizeof(LPC_MESSAGE_HEADER);
   Reply->Data.ReadInputReply.Event = ProcessData->ConsoleEvent;
   
   Status = CsrGetObject( ProcessData, Request->Data.ReadInputRequest.ConsoleHandle, (Object_t **)&Console );
   if( !NT_SUCCESS( Status ) )
      {
	 Reply->Status = Status;
	 return Status;
      }
   RtlEnterCriticalSection( &ActiveConsoleLock );
   // only get input if there is input, and we are not in line input mode, or if we are, if we have a whole line
   if( Console->InputEvents.Flink != &Console->InputEvents &&
       ( !Console->ConsoleMode & ENABLE_LINE_INPUT || Console->WaitingLines ) )     
     {
	Input = (ConsoleInput *)Console->InputEvents.Flink;
	Input->ListEntry.Blink->Flink = Input->ListEntry.Flink;
	Input->ListEntry.Flink->Blink = Input->ListEntry.Blink;
	Reply->Data.ReadInputReply.Input = Input->InputEvent;
	if( Console->ConsoleMode & ENABLE_LINE_INPUT &&
	    Input->InputEvent.Event.KeyEvent.bKeyDown == FALSE &&
	    Input->InputEvent.Event.KeyEvent.uChar.AsciiChar == '\n' )
	  Console->WaitingLines--;
	Console->WaitingChars--;
	RtlFreeHeap( CsrssApiHeap, 0, Input );
	Reply->Data.ReadInputReply.MoreEvents = (Console->InputEvents.Flink != &Console->InputEvents) ? TRUE : FALSE;
	Status = STATUS_SUCCESS;
     }
   else Status = STATUS_PENDING;
   RtlLeaveCriticalSection( &ActiveConsoleLock );
   return Reply->Status = Status;
}

NTSTATUS CsrWriteConsoleOutputAttrib( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply )
{
   int c;
   PCSRSS_CONSOLE Console;
   NTSTATUS Status;
   int X, Y;
   
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
      sizeof(LPC_MESSAGE_HEADER);
   Status = CsrGetObject( ProcessData, Request->Data.WriteConsoleOutputAttribRequest.ConsoleHandle, (Object_t **)&Console );
   if( !NT_SUCCESS( Status ) )
      {
	 Reply->Status = Status;
	 return Status;
      }
   RtlEnterCriticalSection( &ActiveConsoleLock );
   X = Console->CurrentX;
   Y = Console->CurrentY;
   Console->CurrentX = Request->Data.WriteConsoleOutputAttribRequest.Coord.X + Console->ShowX;
   Console->CurrentY = (Request->Data.WriteConsoleOutputAttribRequest.Coord.Y + Console->ShowY) % Console->MaxY;
   for( c = 0; c < Request->Data.WriteConsoleOutputAttribRequest.Length; c++ )
      {
	 Console->Buffer[(Console->CurrentY * Console->MaxX * 2) + (Console->CurrentX * 2) + 1] = Request->Data.WriteConsoleOutputAttribRequest.String[c];
	 if( ++Console->CurrentX == Console->MaxX )
	    {
	       Console->CurrentX = 0;
	       if( ++Console->CurrentY == Console->MaxY )
		  Console->CurrentY = 0;
	    }
      }
   if( Console == ActiveConsole )
      CsrDrawConsole( Console );
   Reply->Data.WriteConsoleOutputAttribReply.EndCoord.X = Console->CurrentX - Console->ShowX;
   Reply->Data.WriteConsoleOutputAttribReply.EndCoord.Y = ( Console->CurrentY + Console->MaxY - Console->ShowY ) % Console->MaxY;
   Console->CurrentX = X;
   Console->CurrentY = Y;
   RtlLeaveCriticalSection( &ActiveConsoleLock );
   return Reply->Status = STATUS_SUCCESS;
}

NTSTATUS CsrFillOutputAttrib( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply )
{
   int c;
   PCSRSS_CONSOLE Console;
   NTSTATUS Status;
   int X, Y;
   
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
      sizeof(LPC_MESSAGE_HEADER);
   Status = CsrGetObject( ProcessData, Request->Data.FillOutputAttribRequest.ConsoleHandle, (Object_t **)&Console );
   if( !NT_SUCCESS( Status ) )
      {
	 Reply->Status = Status;
	 return Status;
      }
   RtlEnterCriticalSection( &ActiveConsoleLock );
   X = Console->CurrentX;
   Y = Console->CurrentY;
   Console->CurrentX = Request->Data.FillOutputAttribRequest.Coord.X + Console->ShowX;
   Console->CurrentY = Request->Data.FillOutputAttribRequest.Coord.Y + Console->ShowY;
   for( c = 0; c < Request->Data.FillOutputAttribRequest.Length; c++ )
      {
	 Console->Buffer[(Console->CurrentY * Console->MaxX * 2) + (Console->CurrentX * 2) + 1] = Request->Data.FillOutputAttribRequest.Attribute;
	 if( ++Console->CurrentX == Console->MaxX )
	    {
	       Console->CurrentX = 0;
	       if( ++Console->CurrentY == Console->MaxY )
		  Console->CurrentY = 0;
	    }
      }
   if( Console == ActiveConsole )
      CsrDrawConsole( Console );
   Console->CurrentX = X;
   Console->CurrentY = Y;
   RtlLeaveCriticalSection( &ActiveConsoleLock );
   return Reply->Status = STATUS_SUCCESS;
}


NTSTATUS CsrGetCursorInfo( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply )
{
   PCSRSS_CONSOLE Console;
   NTSTATUS Status;
   
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
      sizeof(LPC_MESSAGE_HEADER);
   Status = CsrGetObject( ProcessData, Request->Data.GetCursorInfoRequest.ConsoleHandle, (Object_t **)&Console );
   if( !NT_SUCCESS( Status ) )
      {
	 Reply->Status = Status;
	 return Status;
      }
   RtlEnterCriticalSection( &ActiveConsoleLock );
   Reply->Data.GetCursorInfoReply.Info = Console->CursorInfo;
   RtlLeaveCriticalSection( &ActiveConsoleLock );
   return Reply->Status = STATUS_SUCCESS;
}

NTSTATUS CsrSetCursorInfo( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply )
{
   PCSRSS_CONSOLE Console;
   NTSTATUS Status;
   IO_STATUS_BLOCK Iosb;
   
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
      sizeof(LPC_MESSAGE_HEADER);
   Status = CsrGetObject( ProcessData, Request->Data.SetCursorInfoRequest.ConsoleHandle, (Object_t **)&Console );
   if( !NT_SUCCESS( Status ) )
      {
	 Reply->Status = Status;
	 return Status;
      }
   RtlEnterCriticalSection( &ActiveConsoleLock );
   Console->CursorInfo = Request->Data.SetCursorInfoRequest.Info;
   if( Console == ActiveConsole )
      {
	 Status = NtDeviceIoControlFile( ConsoleDeviceHandle, NULL, NULL, NULL, &Iosb, IOCTL_CONSOLE_SET_CURSOR_INFO, &Console->CursorInfo, sizeof( Console->CursorInfo ), 0, 0 );
	 if( !NT_SUCCESS( Status ) )
	    {
	       DbgPrint( "CSR: Failed to set cursor info\n" );
	       return Reply->Status = Status;
	    }
      }
   RtlLeaveCriticalSection( &ActiveConsoleLock );
   return Reply->Status = STATUS_SUCCESS;
}

NTSTATUS CsrSetTextAttrib( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply )
{
   NTSTATUS Status;
   CONSOLE_SCREEN_BUFFER_INFO ScrInfo;
   IO_STATUS_BLOCK Iosb;
   PCSRSS_CONSOLE Console;

   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
      sizeof(LPC_MESSAGE_HEADER);
   Status = CsrGetObject( ProcessData, Request->Data.SetAttribRequest.ConsoleHandle, (Object_t **)&Console );
   if( !NT_SUCCESS( Status ) )
      {
	 Reply->Status = Status;
	 return Status;
      }
   RtlEnterCriticalSection( &ActiveConsoleLock );
   Console->DefaultAttrib = Request->Data.SetAttribRequest.Attrib;
   if( Console == ActiveConsole )
      {
	 ScrInfo.wAttributes = Console->DefaultAttrib;
	 ScrInfo.dwCursorPosition.X = Console->CurrentX - Console->ShowX;   
	 ScrInfo.dwCursorPosition.Y = ((Console->CurrentY + Console->MaxY) - Console->ShowY) % Console->MaxY;
	 Status = NtDeviceIoControlFile( ConsoleDeviceHandle, NULL, NULL, NULL, &Iosb, IOCTL_CONSOLE_SET_SCREEN_BUFFER_INFO, &ScrInfo, sizeof( ScrInfo ), 0, 0 );
	 if( !NT_SUCCESS( Status ) )
	    {
	       DbgPrint( "CSR: Failed to set console info\n" );
	       return Reply->Status = Status;
	    }
      }
   return Reply->Status = STATUS_SUCCESS;
}

NTSTATUS CsrSetConsoleMode( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply )
{
   NTSTATUS Status;
   PCSRSS_CONSOLE Console;

   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE_HEADER);
   if( Request->Data.SetConsoleModeRequest.Mode & (~CONSOLE_MODE_VALID) ||
       ( Request->Data.SetConsoleModeRequest.Mode & (ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT) &&
	 !( Request->Data.SetConsoleModeRequest.Mode & ENABLE_LINE_INPUT ) ) )
     {
       Reply->Status = STATUS_INVALID_PARAMETER;
       return Reply->Status;
     }
   Status = CsrGetObject( ProcessData, Request->Data.SetConsoleModeRequest.ConsoleHandle, (Object_t **)&Console );
   if( !NT_SUCCESS( Status ) )
      {
	 Reply->Status = Status;
	 return Status;
      }
   RtlEnterCriticalSection( &ActiveConsoleLock );
   Console->ConsoleMode = Request->Data.SetConsoleModeRequest.Mode;
   RtlLeaveCriticalSection( &ActiveConsoleLock );
   Reply->Status = STATUS_SUCCESS;
   return Reply->Status;
}

NTSTATUS CsrGetConsoleMode( PCSRSS_PROCESS_DATA ProcessData, PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply )
{
   NTSTATUS Status;
   PCSRSS_CONSOLE Console;

   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - sizeof(LPC_MESSAGE_HEADER);
   Status = CsrGetObject( ProcessData, Request->Data.GetConsoleModeRequest.ConsoleHandle, (Object_t **)&Console );
   if( !NT_SUCCESS( Status ) )
      {
	 Reply->Status = Status;
	 return Status;
      }
   RtlEnterCriticalSection( &ActiveConsoleLock );
   Reply->Data.GetConsoleModeReply.ConsoleMode = Console->ConsoleMode;
   RtlLeaveCriticalSection( &ActiveConsoleLock );
   Reply->Status = STATUS_SUCCESS;
   return Reply->Status;
}







