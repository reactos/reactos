// All or parts of this file are from CHAOS (http://www.se.chaosdev.org/).
// CHAOS is also under the GNU General Public License.

/* Timeout in ms for keyboard command acknowledge. */

#define KEYBOARD_TIMEOUT                        1000

/* Timeout in ms for initializing the keyboard. */

#define KEYBOARD_INIT_TIMEOUT                   1000

/* Keyboard commands. */

#define KEYBOARD_COMMAND_SET_LEDS               0xED
#define KEYBOARD_COMMAND_SET_RATE               0xF3
#define KEYBOARD_COMMAND_ENABLE                 0xF4
#define KEYBOARD_COMMAND_DISABLE                0xF5
#define KEYBOARD_COMMAND_RESET                  0xFF

/* Keyboard replies. */
/* Power on reset. */

#define KEYBOARD_REPLY_POWER_ON_RESET           0xAA

/* Acknowledgement of previous command. */

#define KEYBOARD_REPLY_ACK                      0xFA

/* Command NACK, send the command again. */

#define KEYBOARD_REPLY_RESEND                   0xFE

/* Hardware defines. */

#define KEYBOARD_IRQ                            1

/* Return values from keyboard_read_data (). */
/* No data. */

#define KEYBOARD_NO_DATA                        (-1)

/* Parity or other error. */

#define KEYBOARD_BAD_DATA                       (-2)

/* Common variables. */

int mouse_replies_expected;
BOOLEAN has_mouse;
// mailbox_id_type keyboard_target_mailbox_id;
unsigned keyboard_pressed_keys[16];
