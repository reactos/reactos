// All or parts of this file are from CHAOS (http://www.se.chaosdev.org/).
// CHAOS is also under the GNU General Public License.

#include <ddk/ntddk.h>

#include "controller.h"
#include "keyboard.h"
#include "mouse.h"

/* This reads the controller status port, and does the appropriate
   action. It requires that we hold the keyboard controller spinlock. */

unsigned handle_event(void)
{
  unsigned status = controller_read_status();
  unsigned int work;
  
  for(work = 0; (work < 10000) && ((status & CONTROLLER_STATUS_OUTPUT_BUFFER_FULL) != 0); work++) 
  {
    unsigned scancode;

    scancode = controller_read_input();
 
#if 0
    /* Ignore error bytes. */

    if((status &(CONTROLLER_STATUS_GENERAL_TIMEOUT |
                   CONTROLLER_STATUS_PARITY_ERROR)) == 0)
#endif
    {
      if((status & CONTROLLER_STATUS_MOUSE_OUTPUT_BUFFER_FULL) != 0)
      {
//        mouse_handle_event(scancode); we just use the mouse handler directly..
      }
      else
      {
//        keyboard_handle_event(scancode);
      }
    }
    
    status = controller_read_status();
    
  }

  if(work == 10000)
  {
    DbgPrint("PSAUX: Keyboard controller jammed\n");
  }
  
  return status;
}

/* Wait for keyboard controller input buffer to drain.
   Quote from PS/2 System Reference Manual:
     "Address hex 0060 and address hex 0064 should be written only
     when the input-buffer-full bit and output-buffer-full bit in the
     Controller Status register are set 0."  */

void controller_wait(void)
{
  unsigned long timeout;
  LARGE_INTEGER Millisecond_Timeout;

  Millisecond_Timeout.QuadPart = 1;

  for(timeout = 0; timeout < CONTROLLER_TIMEOUT; timeout++)
  {
    // "handle_keyboard_event()" will handle any incoming events
    // while we wait -- keypresses or mouse movement

    unsigned char status = handle_event();
                
    if((status & CONTROLLER_STATUS_INPUT_BUFFER_FULL) == 0) return;
    
    // Sleep for one millisecond
    KeDelayExecutionThread (KernelMode, FALSE, &Millisecond_Timeout);
  }
  
  DbgPrint("PSAUX: Keyboard timed out\n");
}

/* Wait for input from the keyboard controller. */

int controller_wait_for_input(void)
{
  int timeout;

  for(timeout = KEYBOARD_INIT_TIMEOUT; timeout > 0; timeout--)
  {
    int return_value = controller_read_data();

    if(return_value >= 0) return return_value;

    // Sleep for one millisecond
    KeDelayExecutionThread (KernelMode, FALSE, 1);
  }

  DbgPrint("PSAUX: Timed out on waiting for input from controller\n");
  return -1;
}

/* Write a command word to the keyboard controller. */

void controller_write_command_word(unsigned data)
{
  controller_wait();
  controller_write_command(data);
}

/* Write an output word to the keyboard controller. */

void controller_write_output_word(unsigned data)
{
  controller_wait();
  controller_write_output(data);
}

/* Empty the keyboard input buffer. */

void keyboard_clear_input(void)
{
  int max_read;

  for(max_read = 0; max_read < 100; max_read++)
  {
    if(controller_read_data() == KEYBOARD_NO_DATA)
    {
      break;
    }
  }
}

/* Read data from the keyboard controller. */

int controller_read_data(void)
{
  int return_value = KEYBOARD_NO_DATA;
  unsigned status;
  
  status = controller_read_status();
  if(status & CONTROLLER_STATUS_OUTPUT_BUFFER_FULL)
  {
    unsigned data = controller_read_input();
    
    return_value = data;
    if(status &(CONTROLLER_STATUS_GENERAL_TIMEOUT |
                CONTROLLER_STATUS_PARITY_ERROR))
    {
      return_value = KEYBOARD_BAD_DATA;
    }
  }
  return return_value;
}
