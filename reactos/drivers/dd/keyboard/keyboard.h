/*
 * Some defines
 */

#define KEYBOARD_IRQ       1
#define KBD_BUFFER_SIZE    32
#define KBD_WRAP_MASK      0x1F

#define disable()          __asm__("cli\n\t")
#define enable()           __asm__("sti\n\t")

#define ALT_PRESSED			(LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)
#define CTRL_PRESSED			(LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)


/*
 * Keyboard controller ports
 */

#define KBD_DATA_PORT      0x60
#define KBD_CTRL_PORT      0x64


/*
 * Controller commands
 */

#define KBD_READ_MODE      0x20
#define KBD_WRITE_MODE     0x60
#define KBD_SELF_TEST      0xAA
#define KBD_LINE_TEST      0xAB
#define KBD_CTRL_ENABLE    0xAE

/*
 * Keyboard commands
 */

#define KBD_ENABLE         0xF4
#define KBD_DISABLE        0xF5
#define KBD_RESET          0xFF


/*
 * Keyboard responces
 */

#define KBD_ACK            0xFA
#define KBD_BATCC          0xAA


/*
 * Controller status register bits
 */

#define KBD_OBF            0x01
#define KBD_IBF            0x02
#define KBD_GTO            0x40
#define KBD_PERR           0x80


/*
 * LED bits
 */

#define KBD_LED_SCROLL     0x01
#define KBD_LED_NUM        0x02
#define KBD_LED_CAPS       0x04
