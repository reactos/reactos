#include <ddk/ntddk.h>
#include <ddk/ntddmou.h>
#include "controller.h"
#include "mouse.h"
#include "psaux.h"

// Have we got a PS/2 mouse port?
static BOOLEAN has_mouse = FALSE;

// This buffer holds the mouse scan codes. The PS/2 protocol sends three characters for each event.
static unsigned mouse_buffer[3];
static int mouse_buffer_position = 0;

// The number of mouse replies expected
static int mouse_replies_expected = 0;

// Previous button state
static ULONG PreviousButtons = 0;

// Handle a mouse event

BOOLEAN STDCALL
ps2_mouse_handler(PKINTERRUPT Interrupt, PVOID ServiceContext)
{
  PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)ServiceContext;
  PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
  PMOUSE_INPUT_DATA Input;
  ULONG Queue, ButtonsDiff;
  int state_dx, state_dy;
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

  if (mouse_replies_expected > 0)
  {
    if (scancode == MOUSE_ACK)
    {
      mouse_replies_expected--;
      return;
    }

    mouse_replies_expected = 0;
  }

  /* Add this scancode to the mouse event queue. */
  mouse_buffer[mouse_buffer_position] = scancode;
  mouse_buffer_position++;

  /* If the buffer is full, parse this event */
  if (mouse_buffer_position == 3)
  {
    /* Set local variables for DeviceObject and DeviceExtension */
    DeviceObject = (PDEVICE_OBJECT)ServiceContext;
    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    Queue = DeviceExtension->ActiveQueue % 2;

    /* Prevent buffer overflow */
    if (DeviceExtension->InputDataCount[Queue] == MOUSE_BUFFER_SIZE)
    {
      return TRUE;
    }

    Input = &DeviceExtension->MouseInputData[Queue]
            [DeviceExtension->InputDataCount[Queue]];

    mouse_buffer_position = 0;

    /* Determine the current state of the buttons */
    Input->RawButtons = (mouse_buffer[0] & 1) /* * GPM_B_LEFT */ +
                        (mouse_buffer[0] & 2) /* * GPM_B_RIGHT */ +
                        (mouse_buffer[0] & 4) /* * GPM_B_MIDDLE */;

    /* Determine ButtonFlags */
    Input->ButtonFlags = 0;
    ButtonsDiff = PreviousButtons ^ Input->RawButtons;

    if (ButtonsDiff & GPM_B_LEFT)
    {
      if (Input->RawButtons & GPM_B_LEFT)
      {
        Input->ButtonFlags |= MOUSE_BUTTON_1_DOWN;
      } else {
        Input->ButtonFlags |= MOUSE_BUTTON_1_UP;
      }
    }

    if (ButtonsDiff & GPM_B_RIGHT)
    {
      if (Input->RawButtons & GPM_B_RIGHT)
      {
        Input->ButtonFlags |= MOUSE_BUTTON_2_DOWN;
      } else {
        Input->ButtonFlags |= MOUSE_BUTTON_2_UP;
      }
    }

    if (ButtonsDiff & GPM_B_MIDDLE)
    {
      if (Input->RawButtons & GPM_B_MIDDLE)
      {
        Input->ButtonFlags |= MOUSE_BUTTON_3_DOWN;
      } else {
        Input->ButtonFlags |= MOUSE_BUTTON_3_UP;
      }
    }

    /* Some PS/2 mice send reports with negative bit set in data[0] and zero for
     * movement.  I think this is a bug in the mouse, but working around it only
     * causes artifacts when the actual report is -256; they'll be treated as zero.
     * This should be rare if the mouse sampling rate is  set to a reasonable value;
     * the default of 100 Hz is plenty. (Stephen Tell) */

    /* Determine LastX */
    if (mouse_buffer[1] == 0)
    {
      Input->LastX = 0;
    }
    else
    {
      Input->LastX = (mouse_buffer[0] & 0x10) ? mouse_buffer[1] - 256
                                              : mouse_buffer[1];
    }

    /* Determine LastY */
    if (mouse_buffer[2] == 0)
    {
      Input->LastY = 0;
    }
    else
    {
      Input->LastY = -((mouse_buffer[0] & 0x20) ? mouse_buffer[2] - 256
                                                : mouse_buffer[2]);
    }

    /* Send the Input data to the Mouse Class driver */
    DeviceExtension->InputDataCount[Queue]++;
    KeInsertQueueDpc(&DeviceExtension->IsrDpc, DeviceObject->CurrentIrp, NULL);

    /* Copy RawButtons to Previous Buttons for Input */
    PreviousButtons = Input->RawButtons;

    return TRUE;
  }
}

/* Write a PS/2 mouse command. */

static void mouse_write_command (int command)
{
  controller_wait();
  controller_write_command (CONTROLLER_COMMAND_WRITE_MODE);
  controller_wait();
  controller_write_output (command);
}

/* Send a byte to the PS/2 mouse & handle returned ACK. */

static void mouse_write_ack (int value)
{
  controller_wait();
  controller_write_command (CONTROLLER_COMMAND_WRITE_MOUSE);
  controller_wait();
  controller_write_output (value);

  /* We expect an ACK in response. */

  mouse_replies_expected++;
  controller_wait ();
}
 
/* Check if this is a dual port controller. */

BOOLEAN detect_ps2_port(void)
{
  int loops;
  BOOLEAN return_value = FALSE;
  LARGE_INTEGER Millisecond_Timeout;

  Millisecond_Timeout.QuadPart = 1;

  return TRUE; // The rest of this code fails under BOCHs

  /* Put the value 0x5A in the output buffer using the "WriteAuxiliary Device Output Buffer" command (0xD3).
     Poll the Status Register for a while to see if the value really turns up in the Data Register. If the
     KEYBOARD_STATUS_MOUSE_OBF bit is also set to 1 in the Status Register, we assume this controller has an
     Auxiliary Port (a.k.a. Mouse Port). */

  controller_wait ();
  controller_write_command (CONTROLLER_COMMAND_WRITE_MOUSE_OUTPUT_BUFFER);
  controller_wait ();

  // 0x5A is a random dummy value
  controller_write_output (0x5A);

  for (loops = 0; loops < 10; loops++)
  {
    unsigned char status = controller_read_status();

    if((status & CONTROLLER_STATUS_OUTPUT_BUFFER_FULL) != 0)
    {
      controller_read_input();
      if ((status & CONTROLLER_STATUS_MOUSE_OUTPUT_BUFFER_FULL) != 0)
      {
        return_value = TRUE;
      }
      break;
    }

    KeDelayExecutionThread (KernelMode, FALSE, &Millisecond_Timeout);
  }
  
  return return_value;
}

// Initialize the PS/2 mouse support

BOOLEAN mouse_init (PDEVICE_OBJECT DeviceObject)
{
  PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
  ULONG MappedIrq;
  KIRQL Dirql;
  KAFFINITY Affinity;

  if (!detect_ps2_port ()) return FALSE;

  has_mouse = TRUE;

  DeviceExtension->InputDataCount[0] = 0;
  DeviceExtension->InputDataCount[1] = 0;
  DeviceExtension->ActiveQueue = 0;

  // Enable the PS/2 mouse port
  controller_write_command_word (CONTROLLER_COMMAND_MOUSE_ENABLE);

  // 200 samples/sec
  mouse_write_ack (MOUSE_SET_SAMPLE_RATE);
  mouse_write_ack (200);

  // 8 counts per mm
  mouse_write_ack (MOUSE_SET_RESOLUTION);
  mouse_write_ack (3);

  // 2:1 scaling
  mouse_write_ack (MOUSE_SET_SCALE21);

  // Enable the PS/2 device
  mouse_write_ack (MOUSE_ENABLE_DEVICE);

  // Enable controller interrupts
  mouse_write_command (MOUSE_INTERRUPTS_ON);

  // Connect the interrupt for the mouse irq
  MappedIrq = HalGetInterruptVector(Internal, 0, 0, MOUSE_IRQ, &Dirql, &Affinity);

  IoConnectInterrupt(&DeviceExtension->MouseInterrupt, ps2_mouse_handler, DeviceObject, NULL, MappedIrq,
                     Dirql, Dirql, 0, FALSE, Affinity, FALSE);

  return TRUE;
}
