/*
 ** PS/2 Mouse Driver
 ** Written by Thomas Weidenmueller (w3seek@users.sourceforge.net)
 ** For ReactOS (www.reactos.com)
 ** Adapted from the linux 2.6 mouse driver
*/

#include <ddk/ntddk.h>
#include <ddk/ntddmou.h>
#include "controller.h"
#include "mouse.h"
#include "psaux.h"

#define NDEBUG
#include <debug.h>


int InitSynaptics(PDEVICE_EXTENSION DeviceExtension);
void PS2PPProcessPacket(PDEVICE_EXTENSION DeviceExtension, PMOUSE_INPUT_DATA Input, int *wheel);
void PS2PPSet800dpi(PDEVICE_EXTENSION DeviceExtension);
int PS2PPDetectModel(PDEVICE_EXTENSION DeviceExtension, unsigned char *param);

// Parse incoming packets
BOOLEAN FASTCALL
ParsePackets(PDEVICE_EXTENSION DeviceExtension, PMOUSE_INPUT_DATA Input)
{
  ULONG ButtonsDiff;
  int wheel = 0;
  unsigned char *packet = DeviceExtension->MouseBuffer;
  
  /* Determine the current state of the buttons */           
  Input->RawButtons  = ((packet[0]        & 1) ? GPM_B_LEFT : 0);
  Input->RawButtons |= (((packet[0] >> 1) & 1) ? GPM_B_RIGHT : 0);
  Input->RawButtons |= (((packet[0] >> 2) & 1) ? GPM_B_MIDDLE : 0);
  
  /*
   * The PS2++ protocol is a little bit complex
   */
  
  if(DeviceExtension->MouseType == PSMOUSE_PS2PP || DeviceExtension->MouseType == PSMOUSE_PS2TPP)
    PS2PPProcessPacket(DeviceExtension, Input, &wheel);
  
  /*
   * Scroll wheel on IntelliMice, scroll buttons on NetMice
   */
  
  if(DeviceExtension->MouseType == PSMOUSE_IMPS || DeviceExtension->MouseType == PSMOUSE_GENPS)
  {
    wheel = (int)(-(signed char) packet[3]);
  }
  
  /*
   * Scroll wheel and buttons on IntelliMouse Explorer
   */
  
  if(DeviceExtension->MouseType == PSMOUSE_IMEX)
  {
    wheel = (int)(packet[3] & 8) - (int)(packet[3] & 7);
    Input->RawButtons |= (((packet[3] >> 4) & 1) ? GPM_B_FOURTH : 0);
    Input->RawButtons |= (((packet[3] >> 5) & 1) ? GPM_B_FIFTH : 0);
  }
  
  /*
   * Extra buttons on Genius NewNet 3D
   */
  
  if(DeviceExtension->MouseType == PSMOUSE_GENPS)
  {
    Input->RawButtons |= (((packet[0] >> 6) & 1) ? GPM_B_FOURTH : 0);
    Input->RawButtons |= (((packet[0] >> 7) & 1) ? GPM_B_FIFTH : 0);
  }
  	
  /* 
   * Determine ButtonFlags
   */
  
  Input->ButtonFlags = 0;
  ButtonsDiff = DeviceExtension->PreviousButtons ^ Input->RawButtons;
  
  /*
   * Generic PS/2 Mouse
   */
  
  if(ButtonsDiff & GPM_B_LEFT)
    Input->ButtonFlags |= ((Input->RawButtons & GPM_B_LEFT) ? MOUSE_BUTTON_1_DOWN : MOUSE_BUTTON_1_UP);
  
  if(ButtonsDiff & GPM_B_RIGHT)
    Input->ButtonFlags |= ((Input->RawButtons & GPM_B_RIGHT) ? MOUSE_BUTTON_2_DOWN : MOUSE_BUTTON_2_UP);
  
  if(ButtonsDiff & GPM_B_MIDDLE)
    Input->ButtonFlags |= ((Input->RawButtons & GPM_B_MIDDLE) ? MOUSE_BUTTON_3_DOWN : MOUSE_BUTTON_3_UP);
  
  if(ButtonsDiff & GPM_B_FOURTH)
    Input->ButtonFlags |= ((Input->RawButtons & GPM_B_FOURTH) ? MOUSE_BUTTON_4_DOWN : MOUSE_BUTTON_4_UP);
  
  if(ButtonsDiff & GPM_B_FIFTH)
    Input->ButtonFlags |= ((Input->RawButtons & GPM_B_FIFTH) ? MOUSE_BUTTON_5_DOWN : MOUSE_BUTTON_5_UP);
  
  /* Some PS/2 mice send reports with negative bit set in data[0] and zero for
   * movement.  I think this is a bug in the mouse, but working around it only
   * causes artifacts when the actual report is -256; they'll be treated as zero.
   * This should be rare if the mouse sampling rate is  set to a reasonable value;
   * the default of 100 Hz is plenty. (Stephen Tell) */

  /* Determine LastX */
  if(packet[1])
    Input->LastX = ((packet[0] & 0x10) ? (int)(packet[1] - 256) : (int) packet[1]);
  else
    Input->LastX = 0;
  
  /* Determine LastY */
  if(packet[2])
    Input->LastY = -((packet[0] & 0x20) ? (int)(packet[2] - 256) : (int) packet[2]);
  else
    Input->LastY = 0;
  
  /* Copy RawButtons to Previous Buttons for Input */
  DeviceExtension->PreviousButtons = Input->RawButtons;
  
  if((wheel != 0) && (wheel >= -8) && (wheel <= 7))
  {
    Input->ButtonFlags |= MOUSE_WHEEL;
    Input->ButtonData = (UINT)(wheel * WHEEL_DELTA);
  }
  
  return TRUE;
}

// Handle a mouse event
BOOLEAN STDCALL
MouseHandler(PKINTERRUPT Interrupt, PVOID ServiceContext)
{
  PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)ServiceContext;
  PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
  PMOUSE_INPUT_DATA Input;
  ULONG Queue;
  BOOLEAN ret;
  unsigned scancode;
  unsigned status = controller_read_status();
  scancode = controller_read_input();

  /* Don't handle the mouse event if we aren't connected to the mouse class driver */
  if (DeviceExtension->ClassInformation.CallBack == NULL)
  {
    return FALSE;
  }

  if ((status & CONTROLLER_STATUS_MOUSE_OUTPUT_BUFFER_FULL) != 0)
  {
    // mouse_handle_event(scancode); proceed to handle it
  }
  else
  {
    return FALSE; // keyboard_handle_event(scancode);
  }

  /* Add this scancode to the mouse event queue. */
  DeviceExtension->MouseBuffer[DeviceExtension->MouseBufferPosition] = scancode;
  DeviceExtension->MouseBufferPosition++;

  /* If the buffer is full, parse this event */
  if (DeviceExtension->MouseBufferPosition == DeviceExtension->MouseBufferSize)
  {
    Queue = DeviceExtension->ActiveQueue % 2;

    /* Reset the buffer state. */
    DeviceExtension->MouseBufferPosition = 0;

    /* Prevent buffer overflow */
    if (DeviceExtension->InputDataCount[Queue] == MOUSE_BUFFER_SIZE)
    {
      return TRUE;
    }

    Input = &DeviceExtension->MouseInputData[Queue]
            [DeviceExtension->InputDataCount[Queue]];    

    ret = ParsePackets(DeviceExtension, Input);
      
    /* Send the Input data to the Mouse Class driver */
    DeviceExtension->InputDataCount[Queue]++;
    KeInsertQueueDpc(&DeviceExtension->IsrDpc, DeviceObject->CurrentIrp, NULL);
    
    return ret;
  }
  return TRUE;
}


// Write a PS/2 mouse command.
static void mouse_write_command(int command)
{
  controller_wait();
  controller_write_command(CONTROLLER_COMMAND_WRITE_MODE);
  controller_wait();
  controller_write_output(command);
}


// Sends a byte to the mouse
static int SendByte(PDEVICE_EXTENSION DeviceExtension, unsigned char byte)
{
  int timeout = 100; /* 100 msec */
  int scancode;
  unsigned char status;
  LARGE_INTEGER Millisecond_Timeout;
	
  Millisecond_Timeout.QuadPart = -10000L;
	
  DeviceExtension->ack = 0;
  DeviceExtension->acking = 1;

  controller_wait();
  controller_write_command(CONTROLLER_COMMAND_WRITE_MOUSE);
  controller_wait();
  controller_write_output(byte);
  while ((DeviceExtension->ack == 0) && timeout--)
  {
    status = controller_read_status();
    if((status & CONTROLLER_STATUS_OUTPUT_BUFFER_FULL) != 0)
    {
      scancode = controller_read_input();
      if(scancode >= 0)
      {
        switch(scancode)
        {
          case PSMOUSE_RET_ACK:
            DeviceExtension->ack = 1;
            break;
          case PSMOUSE_RET_NAK:
            DeviceExtension->ack = -1;
            break;
          default:
            DeviceExtension->ack = 1;	/* Workaround for mice which don't ACK the Get ID command */
            if (DeviceExtension->RepliesExpected)
              DeviceExtension->pkt[--DeviceExtension->RepliesExpected] = scancode;
            break;
        }
        return (int)(-(DeviceExtension->ack <= 0));
      }
    }
    KeDelayExecutionThread (KernelMode, FALSE, &Millisecond_Timeout);
  }
  return (int)(-(DeviceExtension->ack <= 0));
}


// Send a PS/2 command to the mouse.
int SendCommand(PDEVICE_EXTENSION DeviceExtension, unsigned char *param, int command)
{
  LARGE_INTEGER Millisecond_Timeout;
  unsigned char status;
  int scancode;
  int timeout = 500; /* 500 msec */
  int send = (command >> 12) & 0xf;
  int receive = (command >> 8) & 0xf;
  int i;

  Millisecond_Timeout.QuadPart = -10000L;

  DeviceExtension->RepliesExpected = receive;
  if (command == PSMOUSE_CMD_RESET_BAT)
    timeout = 2000; /* 2 sec */

  if (command & 0xff)
    if (SendByte(DeviceExtension, command & 0xff))
    return (int)(DeviceExtension->RepliesExpected = 0) - 1;
	
  for (i = 0; i < send; i++)
    if (SendByte(DeviceExtension, param[i]))
      return (int)(DeviceExtension->RepliesExpected = 0) - 1;

  while (DeviceExtension->RepliesExpected && timeout--)
  {
    if (DeviceExtension->RepliesExpected == 1 && command == PSMOUSE_CMD_RESET_BAT)
      timeout = 100;

    if (DeviceExtension->RepliesExpected == 1 && command == PSMOUSE_CMD_GETID &&
        DeviceExtension->pkt[1] != 0xab && DeviceExtension->pkt[1] != 0xac)
    {
      DeviceExtension->RepliesExpected = 0;
      break;
    }
    
    status = controller_read_status();
    if((status & CONTROLLER_STATUS_OUTPUT_BUFFER_FULL) != 0)
    {
      scancode = controller_read_input();
      if(scancode >= 0)
      {
        DeviceExtension->pkt[--DeviceExtension->RepliesExpected] = scancode;
      }
    }

    KeDelayExecutionThread (KernelMode, FALSE, &Millisecond_Timeout);
  }

  for (i = 0; i < receive; i++)
    param[i] = DeviceExtension->pkt[(receive - 1) - i];

  if (DeviceExtension->RepliesExpected)
    return (int)(DeviceExtension->RepliesExpected = 0) - 1;

  return 0;
}


// changes the resolution of the mouse
void SetResolution(PDEVICE_EXTENSION DeviceExtension)
{
  unsigned char param[1];
  
  if(DeviceExtension->MouseType == PSMOUSE_PS2PP && DeviceExtension->Resolution > 400)
  {
    PS2PPSet800dpi(DeviceExtension);
    return;
  }
  
  if(!DeviceExtension->Resolution || DeviceExtension->Resolution >= 200)
    param[0] = 3;
  else if(DeviceExtension->Resolution >= 100)
    param[0] = 2;
  else if(DeviceExtension->Resolution >= 50)
    param[0] = 1;
  else if(DeviceExtension->Resolution )
    param[0] = 0;
  
  SendCommand(DeviceExtension, param, PSMOUSE_CMD_SETRES);
}


// Detect mouse models
int TestMouseExtensions(PDEVICE_EXTENSION DeviceExtension)
{
  unsigned char param[4];
  int type = 0;
  
  param[0] = 0;
  
  // vendor = Generic
  // name = Mouse
  DeviceExtension->MouseModel = 0;
  
  /*
   * Try Synaptics TouchPad magic ID
   */
  param[0] = 0;
  SendCommand(DeviceExtension, param, PSMOUSE_CMD_SETRES);
  SendCommand(DeviceExtension, param, PSMOUSE_CMD_SETRES);
  SendCommand(DeviceExtension, param, PSMOUSE_CMD_SETRES);
  SendCommand(DeviceExtension, param, PSMOUSE_CMD_SETRES);
  SendCommand(DeviceExtension, param, PSMOUSE_CMD_GETINFO);
  if(param[1] == 0x47)
  {
    // vendor = Synaptics
    // name = TouchPad
    if(!InitSynaptics(DeviceExtension))
      return PSMOUSE_SYNAPTICS;
    else
      return PSMOUSE_PS2;
  }
  
  /*
   * Try Genius NetMouse magic init.
   */
  param[0] = 3;
  SendCommand(DeviceExtension, param, PSMOUSE_CMD_SETRES);
  SendCommand(DeviceExtension, NULL, PSMOUSE_CMD_SETSCALE11);
  SendCommand(DeviceExtension, NULL, PSMOUSE_CMD_SETSCALE11);
  SendCommand(DeviceExtension, NULL, PSMOUSE_CMD_SETSCALE11);
  SendCommand(DeviceExtension, param, PSMOUSE_CMD_GETINFO);
  if(param[0] == 0x00 && param[1] == 0x33 && param[2] == 0x55)
  {
    // vendor = Genius
    // name = Wheel Mouse
    // Features = 4th, 5th, Wheel
    return PSMOUSE_GENPS;
  }
  
  /*
   * Try Logitech magic ID.
   */
  param[0] = 0;
  SendCommand(DeviceExtension, param, PSMOUSE_CMD_SETRES);
  SendCommand(DeviceExtension, NULL, PSMOUSE_CMD_SETSCALE11);
  SendCommand(DeviceExtension, NULL, PSMOUSE_CMD_SETSCALE11);
  SendCommand(DeviceExtension, NULL, PSMOUSE_CMD_SETSCALE11);
  param[1] = 0;
  SendCommand(DeviceExtension, param, PSMOUSE_CMD_GETINFO);
  if(param[1])
  {
    type = PS2PPDetectModel(DeviceExtension, param);
    if(type)
      return type;
  }
  
  /*
   * Try IntelliMouse magic init.
   */
  param[0] = 200;
  SendCommand(DeviceExtension, param, PSMOUSE_CMD_SETRATE);
  param[0] = 100;
  SendCommand(DeviceExtension, param, PSMOUSE_CMD_SETRATE);
  param[0] =  80;
  SendCommand(DeviceExtension, param, PSMOUSE_CMD_SETRATE);
  SendCommand(DeviceExtension, param, PSMOUSE_CMD_GETID);
  if(param[0] == 3)
  {
    // Features = Wheel
    
    /*
     * Try IntelliMouse/Explorer magic init.
     */
    param[0] = 200;
    SendCommand(DeviceExtension, param, PSMOUSE_CMD_SETRATE);
    param[0] = 200;
    SendCommand(DeviceExtension, param, PSMOUSE_CMD_SETRATE);
    param[0] =  80;
    SendCommand(DeviceExtension, param, PSMOUSE_CMD_SETRATE);
    SendCommand(DeviceExtension, param, PSMOUSE_CMD_GETID);
    if(param[0] == 4)
    {
      // name = Explorer Mouse
      // Features = 4th, 5th, Wheel
      return PSMOUSE_IMEX;
    }
    
    // name = Wheel Mouse
    return PSMOUSE_IMPS;
  }
  
/*
 * Okay, all failed, we have a standard mouse here. The number of the buttons
 * is still a question, though. We assume 3.
 */
  return PSMOUSE_PS2;
}


// Detect if mouse is just a standard ps/2 mouse
int TestMouse(PDEVICE_EXTENSION DeviceExtension)
{
  unsigned char param[4];
  
  param[0] = param[1] = 0xa5;
  
  /*
   * First, we check if it's a mouse. It should send 0x00 or 0x03
   * in case of an IntelliMouse in 4-byte mode or 0x04 for IM Explorer.
   */  
  if(SendCommand(DeviceExtension, param, PSMOUSE_CMD_GETID))
    return -1;
  
  if(param[0] != 0x00 && param[0] != 0x03 && param[0] != 0x04)
    return -1;
  
  /*
   * Then we reset and disable the mouse so that it doesn't generate events.
   */
  
  if(DeviceExtension->NoExtensions)
  {
    return DeviceExtension->MouseType = PSMOUSE_PS2;
  }
  
  if(SendCommand(DeviceExtension, NULL, PSMOUSE_CMD_RESET_DIS))
    return -1;

  return DeviceExtension->MouseType = TestMouseExtensions(DeviceExtension);
}


// Initialize and enable the mouse
void InitializeMouse(PDEVICE_EXTENSION DeviceExtension)
{
  unsigned char param[4];
  
  /*
   * We set the mouse report rate to a highest possible value.
   * We try 100 first in case mouse fails to set 200.
   */
 
  param[0] = 200;
  SendCommand(DeviceExtension, param, PSMOUSE_CMD_SETRATE);
  
  param[0] = 100;
  SendCommand(DeviceExtension, param, PSMOUSE_CMD_SETRATE);
  
  SetResolution(DeviceExtension);
  SendCommand(DeviceExtension, NULL, PSMOUSE_CMD_SETSCALE11);
  
  /*
   * We set the mouse into streaming mode.
   */
  
  SendCommand(DeviceExtension, param, PSMOUSE_CMD_SETSTREAM);
  
  /*
   * Last, we enable the mouse so that we get reports from it.
   */
  
  if(SendCommand(DeviceExtension, NULL, PSMOUSE_CMD_ENABLE))
    DbgPrint("mouse.c: Failed to enable mouse!\n");
}


// Load settings from registry (by Filip Navara)
BOOL LoadMouseSettings(PDEVICE_EXTENSION DeviceExtension, PUNICODE_STRING RegistryPath)
{
  /*
   * Get the parameters from registry
   */
  ULONG DefaultMouseResolution = 0;
  ULONG DisableExtensionDetection = 1;
  UNICODE_STRING ParametersPath;
  RTL_QUERY_REGISTRY_TABLE Parameters[3];
  PWSTR ParametersKeyPath = L"\\Parameters";
  NTSTATUS Status;
  
  RtlZeroMemory(Parameters, sizeof(Parameters));
  
  /*
   * Formulate path to registry
   */
  RtlInitUnicodeString(&ParametersPath, NULL);
  ParametersPath.MaximumLength = RegistryPath->Length +
    (wcslen(ParametersKeyPath) * sizeof(WCHAR)) + sizeof(UNICODE_NULL);
  ParametersPath.Buffer = ExAllocatePool(PagedPool,
    ParametersPath.MaximumLength);
  if (!ParametersPath.Buffer)
  {
    DPRINT("Can't allocate buffer for parameters\n");
    return FALSE;
  }
  RtlZeroMemory(ParametersPath.Buffer, ParametersPath.MaximumLength);
  RtlAppendUnicodeToString(&ParametersPath, RegistryPath->Buffer);
  RtlAppendUnicodeToString(&ParametersPath, ParametersKeyPath);
  DPRINT("Parameters Path: %wZ\n", &ParametersPath);
  
  Parameters[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
  Parameters[0].Name = L"Resolution";
  Parameters[0].EntryContext = &DeviceExtension->Resolution;
  Parameters[0].DefaultType = REG_DWORD;
  Parameters[0].DefaultData = &DefaultMouseResolution;
  Parameters[0].DefaultLength = sizeof(ULONG);
  
  Parameters[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
  Parameters[1].Name = L"DisableExtensionDetection";
  Parameters[1].EntryContext = &DeviceExtension->NoExtensions;
  Parameters[1].DefaultType = REG_DWORD;
  Parameters[1].DefaultData = &DisableExtensionDetection;
  Parameters[1].DefaultLength = sizeof(ULONG);
  
   Status = RtlQueryRegistryValues(
     RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
     ParametersPath.Buffer,
     Parameters,
     NULL,
     NULL);
  
  if (!NT_SUCCESS(Status))
  {
    DPRINT("RtlQueryRegistryValues failed (0x%x)\n", Status);
    return FALSE;
  }
  return TRUE;
}


// Initialize the PS/2 mouse support
BOOLEAN SetupMouse(PDEVICE_OBJECT DeviceObject, PUNICODE_STRING RegistryPath)
{
  PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
  ULONG MappedIrq;
  KIRQL Dirql;
  KAFFINITY Affinity;
  LARGE_INTEGER Millisecond_Timeout;
  
  Millisecond_Timeout.QuadPart = -10000L;

  /* setup */
  DeviceExtension->NoExtensions = 0;
  DeviceExtension->InputDataCount[0] = 0;
  DeviceExtension->InputDataCount[1] = 0;
  DeviceExtension->ActiveQueue = 0;
  DeviceExtension->MouseBufferPosition = 0;
  DeviceExtension->MouseBufferSize = 3;
  DeviceExtension->Resolution = 0;
  DeviceExtension->RepliesExpected = 0;
  DeviceExtension->PreviousButtons = 0;
  DeviceExtension->SmartScroll = 1;
  DeviceExtension->ack = 0;
  DeviceExtension->acking = 0;
  
  LoadMouseSettings(DeviceExtension, RegistryPath);

  // Enable the PS/2 mouse port
  controller_write_command_word (CONTROLLER_COMMAND_MOUSE_ENABLE);
  
  //InitializeMouse(DeviceExtension);
  DeviceExtension->MouseType = TestMouse(DeviceExtension);
  
  if(DeviceExtension->MouseType > 0)
  {
    /* set the incoming buffer to 4 if needed */
    DeviceExtension->MouseBufferSize += (DeviceExtension->MouseType >= PSMOUSE_GENPS);
    
    DPRINT("Detected Mouse: 0x%x\n", DeviceExtension->MouseType);
    
    InitializeMouse(DeviceExtension);
    
    // Enable controller interrupts
    mouse_write_command (MOUSE_INTERRUPTS_ON);
    
    // Connect the interrupt for the mouse irq
    MappedIrq = HalGetInterruptVector(Internal, 0, 0, MOUSE_IRQ, &Dirql, &Affinity);

    IoConnectInterrupt(&DeviceExtension->MouseInterrupt, MouseHandler, DeviceObject, NULL, MappedIrq,
                       Dirql, Dirql, 0, FALSE, Affinity, FALSE);
  }
  else
  {
    /* FIXME - this fixes the crash if no mouse was detected */
    
    // Connect the interrupt for the mouse irq
    MappedIrq = HalGetInterruptVector(Internal, 0, 0, MOUSE_IRQ, &Dirql, &Affinity);

    IoConnectInterrupt(&DeviceExtension->MouseInterrupt, MouseHandler, DeviceObject, NULL, MappedIrq,
                       Dirql, Dirql, 0, FALSE, Affinity, FALSE);
  }

  return TRUE;
}


// Check if this is a dual port controller.
BOOLEAN DetectPS2Port(void)
{
  int loops;
  unsigned scancode;
  BOOLEAN return_value = FALSE;
  LARGE_INTEGER Millisecond_Timeout;
  
  Millisecond_Timeout.QuadPart = -10000L;
  
  //return TRUE; // The rest of this code fails under BOCHs
  
  /* Put the value 0x5A in the output buffer using the "WriteAuxiliary Device Output Buffer" command (0xD3).
     Poll the Status Register for a while to see if the value really turns up in the Data Register. If the
     KEYBOARD_STATUS_MOUSE_OBF bit is also set to 1 in the Status Register, we assume this controller has an
     Auxiliary Port (a.k.a. Mouse Port). */
  
  controller_wait ();
  controller_write_command(CONTROLLER_COMMAND_WRITE_MOUSE_OUTPUT_BUFFER);
  controller_wait ();
  
  /* 0x5A is a random dummy value */
  controller_write_output(0x5A);
  
  for (loops = 0; loops < 10; loops++)
  {
    unsigned char status = controller_read_status();
    
    if((status & CONTROLLER_STATUS_OUTPUT_BUFFER_FULL) != 0)
    {
      scancode = controller_read_input();
      if((status & CONTROLLER_STATUS_MOUSE_OUTPUT_BUFFER_FULL) != 0)
      {
        return_value = TRUE;
      }
      break;
    }
    
    KeDelayExecutionThread(KernelMode, FALSE, &Millisecond_Timeout);
  }
  
  return return_value;
}

