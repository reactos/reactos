// All or parts of this file are from CHAOS (http://www.se.chaosdev.org/).
// CHAOS is also under the GNU General Public License.

/* Keyboard controller registers. */

#define CONTROLLER_REGISTER_STATUS                      0x64
#define CONTROLLER_REGISTER_CONTROL                     0x64
#define CONTROLLER_REGISTER_DATA                        0x60

/* Keyboard controller commands. */
/* Read mode bits. */

#define CONTROLLER_COMMAND_READ_MODE                    0x20

/* Write mode bits. */

#define CONTROLLER_COMMAND_WRITE_MODE                   0x60

/* Get controller version. */

#define CONTROLLER_COMMAND_GET_VERSION                  0xA1

/* Disable mouse interface. */

#define CONTROLLER_COMMAND_MOUSE_DISABLE                0xA7

/* Enable mouse interface. */

#define CONTROLLER_COMMAND_MOUSE_ENABLE                 0xA8

/* Mouse interface test. */

#define CONTROLLER_COMMAND_TEST_MOUSE                   0xA9

/* Controller self test. */

#define CONTROLLER_COMMAND_SELF_TEST                    0xAA

/* Keyboard interface test. */

#define CONTROLLER_COMMAND_KEYBOARD_TEST                0xAB

/* Keyboard interface disable. */

#define CONTROLLER_COMMAND_KEYBOARD_DISABLE             0xAD

/* Keyboard interface enable. */

#define CONTROLLER_COMMAND_KEYBOARD_ENABLE              0xAE

/* Write to output buffer as if initiated by the auxiliary device. */

#define CONTROLLER_COMMAND_WRITE_MOUSE_OUTPUT_BUFFER    0xD3

/* Write the following byte to the mouse. */

#define CONTROLLER_COMMAND_WRITE_MOUSE                  0xD4

/* Status Register Bits. */
/* Keyboard output buffer full. */

#define CONTROLLER_STATUS_OUTPUT_BUFFER_FULL            0x01

/* Keyboard input buffer full. */

#define CONTROLLER_STATUS_INPUT_BUFFER_FULL             0x02

/* Self test successful. */

#define CONTROLLER_STATUS_SELF_TEST                     0x04

/* Last write was a command write. (0 = data) */

#define CONTROLLER_STATUS_COMMAND                       0x08

/* Zero if keyboard locked. */

#define CONTROLLER_STATUS_UNLOCKED                      0x10

/* Mouse output buffer full. */

#define CONTROLLER_STATUS_MOUSE_OUTPUT_BUFFER_FULL      0x20

/* General receive/xmit timeout. */

#define CONTROLLER_STATUS_GENERAL_TIMEOUT               0x40

/* Parity error. */

#define CONTROLLER_STATUS_PARITY_ERROR                  0x80

#define AUX_STATUS_OUTPUT_BUFFER_FULL                   (CONTROLLER_STATUS_OUTPUT_BUFFER_FULL | \
                                                         CONTROLLER_STATUS_MOUSE_OUTPUT_BUFFER_FULL)

/* Controller mode register bits. */
/* Keyboard data generate IRQ1. */

#define CONTROLLER_MODE_KEYBOARD_INTERRUPT              0x01

/* Mouse data generate IRQ12. */

#define CONTROLLER_MODE_MOUSE_INTERRUPT                 0x02

/* The system flag. (?) */

#define CONTROLLER_MODE_SYS                             0x04

/* The keylock doesn't affect the keyboard if set. */

#define CONTROLLER_MODE_NO_KEYLOCK                      0x08

/* Disable keyboard interface. */

#define CONTROLLER_MODE_DISABLE_KEYBOARD                0x10

/* Disable mouse interface. */

#define CONTROLLER_MODE_DISABLE_MOUSE                   0x20

/* Scan code conversion to PC format. */

#define CONTROLLER_MODE_KCC                             0x40
#define CONTROLLER_MODE_RFU                             0x80

/* Timeout in ms for sending to keyboard controller. */

#define CONTROLLER_TIMEOUT                              250

/* Hardware defines. */

#define CONTROLLER_PORT_BASE                            0x60
#define CONTROLLER_PORTS                                0x10

/* Macros for accessing the keyboard controller on this platform. */

#define controller_read_input() READ_PORT_UCHAR ((PUCHAR)CONTROLLER_REGISTER_DATA)
#define controller_read_status() READ_PORT_UCHAR ((PUCHAR)CONTROLLER_REGISTER_STATUS)
#define controller_write_output(value) WRITE_PORT_UCHAR ((PUCHAR)CONTROLLER_REGISTER_DATA, value)
#define controller_write_command(value) WRITE_PORT_UCHAR ((PUCHAR)CONTROLLER_REGISTER_CONTROL, value)
