#include <ddk/ntddk.h>
#include <ddk/ntddmou.h>
#include "controller.h"
#include "mouse.h"
#include "psaux.h"

#define PSMOUSE_LOGITECH_SMARTSCROLL	1

/*
 * psmouse_process_packet() anlyzes the PS/2 mouse packet contents and
 * reports relevant events to the input module.
 */

static void psmouse_process_packet(PDEVICE_EXTENSION DeviceExtension, PMOUSE_INPUT_DATA Input)
{
    ULONG ButtonsDiff;
	unsigned char *packet = DeviceExtension->MouseBuffer;

    /* Determine the current state of the buttons */           
    Input->RawButtons  = (packet[0]        & 1) ? GPM_B_LEFT : 0;
    Input->RawButtons |= ((packet[0] >> 1) & 1) ? GPM_B_RIGHT : 0;
    Input->RawButtons |= ((packet[0] >> 2) & 1) ? GPM_B_MIDDLE : 0;

/*
 * The PS2++ protocol is a little bit complex
 */

	if (DeviceExtension->MouseType == PSMOUSE_PS2PP || DeviceExtension->MouseType == PSMOUSE_PS2TPP)
		ps2pp_process_packet(DeviceExtension, Input);

/*
 * Scroll wheel on IntelliMice, scroll buttons on NetMice
 */

	if (DeviceExtension->MouseType == PSMOUSE_IMPS || DeviceExtension->MouseType == PSMOUSE_GENPS)
	{
	  Input->ButtonData = (UINT)((int)((-(signed char) packet[3]) * WHEEL_DELTA));
	}

/*
 * Scroll wheel and buttons on IntelliMouse Explorer
 */

	if (DeviceExtension->MouseType == PSMOUSE_IMEX)
    {
      Input->ButtonData = (UINT)((WHEEL_DELTA) * ((int)(packet[3] & 8) - (int)(packet[3] & 7)));
      Input->RawButtons |= ((packet[3] >> 4) & 1) ? GPM_B_FOURTH : 0;
      Input->RawButtons |= ((packet[3] >> 5) & 1) ? GPM_B_FIFTH : 0;
	}

/*
 * Extra buttons on Genius NewNet 3D
 */

	if (DeviceExtension->MouseType == PSMOUSE_GENPS)
    {
      Input->RawButtons |= ((packet[0] >> 6) & 1) ? GPM_B_FOURTH : 0;
      Input->RawButtons |= ((packet[0] >> 7) & 1) ? GPM_B_FIFTH : 0;
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
    Input->ButtonFlags |= (Input->RawButtons & GPM_B_LEFT) ? MOUSE_BUTTON_1_DOWN : MOUSE_BUTTON_1_UP;
  
  if(ButtonsDiff & GPM_B_RIGHT)
    Input->ButtonFlags |= (Input->RawButtons & GPM_B_RIGHT) ? MOUSE_BUTTON_2_DOWN : MOUSE_BUTTON_2_UP;
  
  if(ButtonsDiff & GPM_B_MIDDLE)
    Input->ButtonFlags |= (Input->RawButtons & GPM_B_MIDDLE) ? MOUSE_BUTTON_3_DOWN : MOUSE_BUTTON_3_UP;
  
  if(ButtonsDiff & GPM_B_FOURTH)
    Input->ButtonFlags |= (Input->RawButtons & GPM_B_FOURTH) ? MOUSE_BUTTON_4_DOWN : MOUSE_BUTTON_4_UP;
  
  if(ButtonsDiff & GPM_B_FIFTH)
    Input->ButtonFlags |= (Input->RawButtons & GPM_B_FIFTH) ? MOUSE_BUTTON_5_DOWN : MOUSE_BUTTON_5_UP;
  
  /* Some PS/2 mice send reports with negative bit set in data[0] and zero for
   * movement.  I think this is a bug in the mouse, but working around it only
   * causes artifacts when the actual report is -256; they'll be treated as zero.
   * This should be rare if the mouse sampling rate is  set to a reasonable value;
   * the default of 100 Hz is plenty. (Stephen Tell) */

  /* Determine LastX */
  Input->LastX = packet[1] ? (int) packet[1] - (int) ((packet[0] << 4) & 0x100) : 0;

  /* Determine LastY */
  Input->LastY = packet[2] ? (int) ((packet[0] << 3) & 0x100) - (int) packet[2] : 0;
  
  /* Copy RawButtons to Previous Buttons for Input */
  DeviceExtension->PreviousButtons = Input->RawButtons;
  
  if(Input->ButtonData)
    Input->ButtonFlags |= MOUSE_WHEEL;
}


// Handle a mouse event

BOOLEAN STDCALL
ps2_mouse_handler(PKINTERRUPT Interrupt, PVOID ServiceContext)
{
  PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)ServiceContext;
  PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
  PMOUSE_INPUT_DATA Input;
  ULONG Queue;
  BOOLEAN ret = FALSE;
  int state_dx, state_dy;
  unsigned scancode;
  unsigned status = controller_read_status();
  scancode = controller_read_input();

  if ((status & CONTROLLER_STATUS_MOUSE_OUTPUT_BUFFER_FULL) == 0)
  {
    return FALSE; // keyboard_handle_event(scancode);
  }
  
  if(DeviceExtension->acking)
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
        if (DeviceExtension->cmdcnt)
          DeviceExtension->cmdbuf[--DeviceExtension->cmdcnt] = scancode;
        break;
    }
    DeviceExtension->acking = 0;    
    return TRUE;
  }
  
  if (DeviceExtension->cmdcnt)
  {
    DeviceExtension->cmdbuf[--DeviceExtension->cmdcnt] = scancode;
    return TRUE;
  }

  /* Don't handle the mouse event if we aren't connected to the mouse class driver */
  if (DeviceExtension->ClassInformation.CallBack == NULL)
  {
    return FALSE;
  }

  /* Add this scancode to the mouse event queue. */
  DeviceExtension->MouseBuffer[DeviceExtension->MouseBufferPosition] = scancode;
  DeviceExtension->MouseBufferPosition++;
  
  Queue = DeviceExtension->ActiveQueue % 2;

  /* If the buffer is full, parse the standard ps/2 packets */
  if (DeviceExtension->MouseBufferPosition == 
      DeviceExtension->MouseBufferSize + (DeviceExtension->MouseType >= PSMOUSE_GENPS))
  {

    /* Reset the buffer state. */
    DeviceExtension->MouseBufferPosition = 0;

    /* Prevent buffer overflow */
    if (DeviceExtension->InputDataCount[Queue] == MOUSE_BUFFER_SIZE)
    {
      return TRUE;
    }

    Input = &DeviceExtension->MouseInputData[Queue]
            [DeviceExtension->InputDataCount[Queue]];

    psmouse_process_packet(DeviceExtension, Input);
    
    goto end;
  }
  
  /*
   * The synaptics driver has its own resync logic,
   * so it needs to receive all bytes one at a time.
   */
  if ((DeviceExtension->MouseBufferPosition == 1) && (DeviceExtension->MouseType == PSMOUSE_SYNAPTICS))
  {
//    synaptics_process_byte(psmouse, regs);
    /* Reset the buffer state. */
    DeviceExtension->MouseBufferPosition = 0;
    
    return TRUE;
  }
  
  /* FIXME */
  if ((DeviceExtension->MouseBufferPosition == 1) && (DeviceExtension->MouseBuffer[0] == PSMOUSE_RET_BAT))
  {
    /* FIXME ????????? */
    DeviceExtension->MouseBufferPosition = 0;
    
    return TRUE;
  }
  
  return TRUE;
  
end:
  /* Send the Input data to the Mouse Class driver */
  DeviceExtension->InputDataCount[Queue]++;
  KeInsertQueueDpc(&DeviceExtension->IsrDpc, DeviceObject->CurrentIrp, NULL);
  return ret;
}

/*
 * psmouse_sendbyte() sends a byte to the mouse, and waits for acknowledge.
 * It doesn't handle retransmission, though it could - because when there would
 * be need for retransmissions, the mouse has to be replaced anyway.
 */
 
static int psmouse_sendbyte(PDEVICE_EXTENSION DeviceExtension, unsigned char byte)
{
	int timeout = 100; /* 100 msec */
	LARGE_INTEGER Millisecond_Timeout;
	
	Millisecond_Timeout.QuadPart = 1;
	
	DeviceExtension->ack = 0;
	DeviceExtension->acking = 1;

    controller_wait();
    controller_write_command(CONTROLLER_COMMAND_WRITE_MOUSE);
    controller_wait();
    controller_write_output(byte);
    controller_wait();
    while ((DeviceExtension->ack == 0) && timeout--)
    {
      KeDelayExecutionThread (KernelMode, FALSE, &Millisecond_Timeout);
    }
	return -(DeviceExtension->ack <= 0);
}

/*
 * psmouse_command() sends a command and its parameters to the mouse,
 * then waits for the response and puts it in the param array.
 */

int psmouse_command(PDEVICE_EXTENSION DeviceExtension, unsigned char *param, int command)
{
    LARGE_INTEGER Millisecond_Timeout;
	int timeout = 500; /* 500 msec */
	int send = (command >> 12) & 0xf;
	int receive = (command >> 8) & 0xf;
	int i;
	
	Millisecond_Timeout.QuadPart = 1;

	DeviceExtension->cmdcnt = receive;
	if (command == PSMOUSE_CMD_RESET_BAT)
                timeout = 2000; /* 2 sec */

	if (command & 0xff)
		if (psmouse_sendbyte(DeviceExtension, command & 0xff))
			return (DeviceExtension->cmdcnt = 0) - 1;

	for (i = 0; i < send; i++)
		if (psmouse_sendbyte(DeviceExtension, param[i]))
			return (DeviceExtension->cmdcnt = 0) - 1;

	while (DeviceExtension->cmdcnt && timeout--) {
	
		if (DeviceExtension->cmdcnt == 1 && command == PSMOUSE_CMD_RESET_BAT)
			timeout = 100;

		if (DeviceExtension->cmdcnt == 1 && command == PSMOUSE_CMD_GETID &&
		    DeviceExtension->cmdbuf[1] != 0xab && DeviceExtension->cmdbuf[1] != 0xac) {
			DeviceExtension->cmdcnt = 0;
			break;
		}

		KeDelayExecutionThread (KernelMode, FALSE, &Millisecond_Timeout);
	}

	for (i = 0; i < receive; i++)
		param[i] = DeviceExtension->cmdbuf[(receive - 1) - i];
	if (DeviceExtension->cmdcnt) 
		return (DeviceExtension->cmdcnt = 0) - 1;

	return 0;
}

/*
 * psmouse_extensions() probes for any extensions to the basic PS/2 protocol
 * the mouse may have.
 */

static int psmouse_extensions(PDEVICE_EXTENSION DeviceExtension)
{
	unsigned char param[4];

	param[0] = 0;
	//vendor = "Generic";
	//name = "Mouse";
	DeviceExtension->model = 0;

	if (DeviceExtension->psmouse_noext)
	{
		return PSMOUSE_PS2;
	}

/*
 * Try Synaptics TouchPad magic ID
 */

       param[0] = 0;
       psmouse_command(DeviceExtension, param, PSMOUSE_CMD_SETRES);
       psmouse_command(DeviceExtension, param, PSMOUSE_CMD_SETRES);
       psmouse_command(DeviceExtension, param, PSMOUSE_CMD_SETRES);
       psmouse_command(DeviceExtension, param, PSMOUSE_CMD_SETRES);
       psmouse_command(DeviceExtension, param, PSMOUSE_CMD_GETINFO);

       if (param[1] == 0x47) {
		//vendor = "Synaptics";
		//name = "TouchPad";
		
		if (!synaptics_init(DeviceExtension))
			return PSMOUSE_SYNAPTICS;
		else
			return PSMOUSE_PS2;
       }

/*
 * Try Genius NetMouse magic init.
 */

	param[0] = 3;
	psmouse_command(DeviceExtension, param, PSMOUSE_CMD_SETRES);
	psmouse_command(DeviceExtension,  NULL, PSMOUSE_CMD_SETSCALE11);
	psmouse_command(DeviceExtension,  NULL, PSMOUSE_CMD_SETSCALE11);
	psmouse_command(DeviceExtension,  NULL, PSMOUSE_CMD_SETSCALE11);
	psmouse_command(DeviceExtension, param, PSMOUSE_CMD_GETINFO);

	if (param[0] == 0x00 && param[1] == 0x33 && param[2] == 0x55) {

		//set_bit(BTN_EXTRA, psmouse->dev.keybit);
		//set_bit(BTN_SIDE, psmouse->dev.keybit);
		//set_bit(REL_WHEEL, psmouse->dev.relbit);

		//vendor = "Genius";
		//name = "Wheel Mouse";
		return PSMOUSE_GENPS;
	}

/*
 * Try Logitech magic ID.
 */

	param[0] = 0;
	psmouse_command(DeviceExtension, param, PSMOUSE_CMD_SETRES);
	psmouse_command(DeviceExtension,  NULL, PSMOUSE_CMD_SETSCALE11);
	psmouse_command(DeviceExtension,  NULL, PSMOUSE_CMD_SETSCALE11);
	psmouse_command(DeviceExtension,  NULL, PSMOUSE_CMD_SETSCALE11);
	param[1] = 0;
	psmouse_command(DeviceExtension, param, PSMOUSE_CMD_GETINFO);

	if (param[1]) {
		int type = ps2pp_detect_model(DeviceExtension, param);
		if (type)
		{
			return type;
		}
	}

/*
 * Try IntelliMouse magic init.
 */

	param[0] = 200;
	psmouse_command(DeviceExtension, param, PSMOUSE_CMD_SETRATE);
	param[0] = 100;
	psmouse_command(DeviceExtension, param, PSMOUSE_CMD_SETRATE);
	param[0] =  80;
	psmouse_command(DeviceExtension, param, PSMOUSE_CMD_SETRATE);
	psmouse_command(DeviceExtension, param, PSMOUSE_CMD_GETID);
	
	if (param[0] == 3) {

	//	set_bit(REL_WHEEL, DeviceExtension->dev.relbit);

/*
 * Try IntelliMouse/Explorer magic init.
 */

		param[0] = 200;
		psmouse_command(DeviceExtension, param, PSMOUSE_CMD_SETRATE);
		param[0] = 200;
		psmouse_command(DeviceExtension, param, PSMOUSE_CMD_SETRATE);
		param[0] =  80;
		psmouse_command(DeviceExtension, param, PSMOUSE_CMD_SETRATE);
		psmouse_command(DeviceExtension, param, PSMOUSE_CMD_GETID);

		if (param[0] == 4) {

			//set_bit(BTN_SIDE, psmouse->dev.keybit);
			//set_bit(BTN_EXTRA, psmouse->dev.keybit);

			//name = "Explorer Mouse";
			return PSMOUSE_IMEX;
		}

		//name = "Wheel Mouse";
		return PSMOUSE_IMPS;
	}

/*
 * Okay, all failed, we have a standard mouse here. The number of the buttons
 * is still a question, though. We assume 3.
 */
	return PSMOUSE_PS2;
}

/*
 * psmouse_test() probes for a PS/2 mouse.
 */

static int psmouse_test(PDEVICE_EXTENSION DeviceExtension)
{
	unsigned char param[2];
/*
 * First, we check if it's a mouse. It should send 0x00 or 0x03
 * in case of an IntelliMouse in 4-byte mode or 0x04 for IM Explorer.
 */

	param[0] = param[1] = 0xa5;

	if (psmouse_command(DeviceExtension, param, PSMOUSE_CMD_GETID))
		return -1;

	if (param[0] != 0x00 && param[0] != 0x03 && param[0] != 0x04)
		return -1;

/*
 * Then we reset and disable the mouse so that it doesn't generate events.
 */

	if (psmouse_command(DeviceExtension, NULL, PSMOUSE_CMD_RESET_DIS))
		return -1;

/*
 * And here we try to determine if it has any extensions over the
 * basic PS/2 3-button mouse.
 */
	return DeviceExtension->MouseType = psmouse_extensions(DeviceExtension);
}

/* Check if this is a dual port controller. */

BOOLEAN detect_ps2_port(void)
{
  int loops;
  unsigned scancode;
  BOOLEAN return_value = FALSE;
  LARGE_INTEGER Millisecond_Timeout;

  Millisecond_Timeout.QuadPart = 1;

  //return TRUE; // The rest of this code fails under BOCHs

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
      scancode = controller_read_input();
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

/*
 * Here we set the mouse resolution.
 */

static void psmouse_set_resolution(PDEVICE_EXTENSION DeviceExtension)
{
	unsigned char param[1];

	if (DeviceExtension->MouseType == PSMOUSE_PS2PP && DeviceExtension->Resolution > 400) {
		ps2pp_set_800dpi(DeviceExtension);
		return;
	}

	if (!DeviceExtension->Resolution || DeviceExtension->Resolution >= 200)
		param[0] = 3;
	else if (DeviceExtension->Resolution >= 100)
		param[0] = 2;
	else if (DeviceExtension->Resolution >= 50)
		param[0] = 1;
	else if (DeviceExtension->Resolution)
		param[0] = 0;

        psmouse_command(DeviceExtension, param, PSMOUSE_CMD_SETRES);
}

/*
 * psmouse_initialize() initializes the mouse to a sane state.
 */

static void psmouse_initialize(PDEVICE_EXTENSION DeviceExtension)
{
	unsigned char param[2];
/*
 * We set the mouse report rate to a highest possible value.
 * We try 100 first in case mouse fails to set 200.
 */
 
 	param[0] = 100;
	psmouse_command(DeviceExtension, param, PSMOUSE_CMD_SETRATE);
 
	param[0] = 200;
	psmouse_command(DeviceExtension, param, PSMOUSE_CMD_SETRATE);

/*
 * We also set the resolution and scaling.
 */

	//psmouse_set_resolution(DeviceExtension);
	psmouse_command(DeviceExtension,  NULL, PSMOUSE_CMD_SETSCALE11);

/*
 * We set the mouse into streaming mode.
 */

	//psmouse_command(DeviceExtension, param, PSMOUSE_CMD_SETSTREAM);

/*
 * Last, we enable the mouse so that we get reports from it.
 */

	if (psmouse_command(DeviceExtension, NULL, PSMOUSE_CMD_ENABLE))
		DbgPrint("mouse.c: Failed to enable mouse\n");
}
 
/* Initialize the PS/2 mouse support */
BOOLEAN mouse_init (PDEVICE_OBJECT DeviceObject)
{
  PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
  ULONG MappedIrq;
  KIRQL Dirql;
  KAFFINITY Affinity;
  unsigned scancode;

  DeviceExtension->MouseBufferPosition = 0;
  DeviceExtension->MouseBufferSize = 3; /* standard PS/2 packet size */
  DeviceExtension->PreviousButtons = 0;
  DeviceExtension->MouseType = 0;
  
  DeviceExtension->psmouse_noext = 1; // Set this to 1 if you don't want to detect enhanced mice (BOCHS?)
  DeviceExtension->psmouse_smartscroll = PSMOUSE_LOGITECH_SMARTSCROLL;
  DeviceExtension->Resolution = 0; // Set this to the resolution of the mouse
  
  DeviceExtension->InputDataCount[0] = 0;
  DeviceExtension->InputDataCount[1] = 0;
  DeviceExtension->ActiveQueue = 0;
  
  DeviceExtension->ack = 0;
  DeviceExtension->acking = 0;
  
  DeviceExtension->HasMouse = detect_ps2_port();

  if(!DeviceExtension->HasMouse)
    return FALSE;

  // Enable the PS/2 mouse port
  controller_write_command_word (CONTROLLER_COMMAND_MOUSE_ENABLE);              

  // Enable controller interrupts
  controller_wait();
  controller_write_command (CONTROLLER_COMMAND_WRITE_MODE);
  controller_wait();
  controller_write_output (MOUSE_INTERRUPTS_ON);
  controller_wait();

  // Connect the interrupt for the mouse irq
  MappedIrq = HalGetInterruptVector(Internal, 0, 0, MOUSE_IRQ, &Dirql, &Affinity);

  IoConnectInterrupt(&DeviceExtension->MouseInterrupt, ps2_mouse_handler, DeviceObject, NULL, MappedIrq,
                     Dirql, Dirql, 0, FALSE, Affinity, FALSE);
  
  psmouse_test(DeviceExtension);
  psmouse_initialize(DeviceExtension);

  return TRUE;
}

